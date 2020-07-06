/**
	@file		ntv2driverinterface.cpp
	@brief		Implements the CNTV2DriverInterface class.
	@copyright	(C) 2003-2020 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ajatypes.h"
#include "ajaexport.h"
#include "ntv2enums.h"
#include "ntv2debug.h"
#include "ntv2driverinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2nubaccess.h"
#include "ntv2bitfile.h"
#include "ntv2registers2022.h"
#include "ntv2spiinterface.h"
#include "ntv2utils.h"
#include "ajabase/system/debug.h"
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>

using namespace std;

#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') << uint64_t(__x__)  << dec
#define INSTP(_p_)			HEX16(uint64_t(_p_))
#define	DIFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DIINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DIDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)


CNTV2DriverInterface::CNTV2DriverInterface ()
	:	_boardNumber					(0),
		_boardOpened					(false),
		_boardID						(DEVICE_ID_NOTFOUND),
		_displayErrorMessage			(false),
		_pciSlot						(0),
		_programStatus					(0),
		_pFrameBaseAddress				(AJA_NULL),
		_pCh1FrameBaseAddress			(AJA_NULL),			//	DEPRECATE!
		_pCh2FrameBaseAddress			(AJA_NULL),			//	DEPRECATE!
		_pRegisterBaseAddress			(AJA_NULL),
		_pRegisterBaseAddressLength		(0),
		_pFS1FPGARegisterBaseAddress	(AJA_NULL),			//	DEPRECATE!
		_pXena2FlashBaseAddress			(AJA_NULL),
		_ulNumFrameBuffers				(0),
		_ulFrameBufferSize				(0)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	_sockfd					= -1;
	_remoteHandle			= INVALID_NUB_HANDLE;
	_nubProtocolVersion		= ntv2NubProtocolVersionNone;
	::memset (&_sockAddr, 0, sizeof(struct sockaddr_in));
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	::memset (mInterruptEventHandles, 0, sizeof (mInterruptEventHandles));
	::memset (mEventCounts, 0, sizeof (mEventCounts));
#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	mRecordRegWrites = mSkipRegWrites = false;
#endif	//	NTV2_WRITEREG_PROFILING

}	//	constructor


CNTV2DriverInterface::~CNTV2DriverInterface ()
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2DisconnectFromNub(_sockfd);
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
}	//	destructor


bool CNTV2DriverInterface::ConfigureSubscription (bool bSubscribe, INTERRUPT_ENUMS eInterruptType, PULWord & hSubscription)
{
	(void) hSubscription;
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(eInterruptType))
		return false;
	if (bSubscribe)
	{										//	If subscribing,
		mEventCounts [eInterruptType] = 0;	//		clear this interrupt's event counter
		DIDBG("Subscribing '" << ::NTV2InterruptEnumString(eInterruptType) << "' (" << UWord(eInterruptType)
				<< "), event counter reset");
	}
 	else
		DIDBG("Unsubscribing '" << ::NTV2InterruptEnumString(eInterruptType) << "' (" << UWord(eInterruptType) << "), "
				<< mEventCounts[eInterruptType] << " event(s) received");
	return true;

}	//	ConfigureSubscription


NTV2DeviceID CNTV2DriverInterface::GetDeviceID (void)
{
	ULWord	value	(0);
	if (_boardOpened && ReadRegister (kRegBoardID, value))
	{
		const NTV2DeviceID	currentValue (static_cast <NTV2DeviceID> (value));
		if (currentValue != _boardID)
			DIWARN(xHEX0N(this,16) << ":  NTV2DeviceID " << xHEX0N(value,8) << " (" << ::NTV2DeviceIDToString(currentValue)
					<< ") read from register " << kRegBoardID << " doesn't match _boardID " << xHEX0N(_boardID,8) << " ("
					<< ::NTV2DeviceIDToString(_boardID) << ")");
		return currentValue;
	}
	else
		return DEVICE_ID_NOTFOUND;
}


#if defined (NTV2_NUB_CLIENT_SUPPORT)

#ifdef AJA_WINDOWS

static bool winsock_inited = false;
static WSADATA wsaData;

static void initWinsock(void) {
    int wret;
    if (!winsock_inited)
        wret = WSAStartup(MAKEWORD(2,2), &wsaData);
    winsock_inited = true;
}

#endif

bool CNTV2DriverInterface::OpenRemote (UWord boardNumber, bool displayErrorMessage, UWord ulBoardType, const char *hostname)
{
#ifdef AJA_WINDOWS
    initWinsock();
#endif
    (void) displayErrorMessage;
	if (!hostname)
	{
		NTV2DisconnectFromNub(_sockfd);
		return false;
	}

	// If connected and hostname the same, remain open, else close and reconnect.
	if (_sockfd != -1 && !strcmp(hostname, GetHostName()))
	{
		// Already connected to desired host.
	}
	else
	{
		// Disconnect if connected.
		NTV2DisconnectFromNub(_sockfd);

		// Establish connection
		if (NTV2ConnectToNub(hostname, &_sockfd) < 0)
		{
			NTV2DisconnectFromNub(_sockfd);
			return false;
		}
		_hostname = hostname;
	}

	// Open the card on the remote system.
	switch (NTV2OpenRemoteCard(_sockfd, boardNumber, ulBoardType, &_remoteHandle, &_nubProtocolVersion))
	{
		case NTV2_REMOTE_ACCESS_SUCCESS:
			DINOTE("OpenRemoteCard succeeded, _remoteHandle=" << xHEX0N(_remoteHandle,8));
			#if defined(NTV2_FORCE_NO_DEVICE)
				NTV2NoDevInitRegisters();
			#endif
			return true;

		case NTV2_REMOTE_ACCESS_CONNECTION_CLOSED:
			NTV2DisconnectFromNub(_sockfd);
			DIFAIL("OpenRemoteCard failed, _remoteHandle=" xHEX0N(_remoteHandle,8));
			_remoteHandle = LWord(INVALID_NUB_HANDLE);
			break;

		case NTV2_REMOTE_ACCESS_NOT_CONNECTED:
		case NTV2_REMOTE_ACCESS_OUT_OF_MEMORY:
		case NTV2_REMOTE_ACCESS_SEND_ERR:
		case NTV2_REMOTE_ACCESS_RECV_ERR:
		case NTV2_REMOTE_ACCESS_TIMEDOUT:
		case NTV2_REMOTE_ACCESS_NO_CARD:
		case NTV2_REMOTE_ACCESS_NOT_OPEN_RESP:
		case NTV2_REMOTE_ACCESS_NON_NUB_PKT:
		default:
			// Failed, but don't close connection, can try with another card on same connection.
			// printf("_remoteHandle came back as 0x%08x\n", _remoteHandle);
			_remoteHandle = (LWord)INVALID_NUB_HANDLE;
	}

	return false;
}


bool CNTV2DriverInterface::CloseRemote()
{
	if (_sockfd != -1)
	{
		DIINFO("Remote closed, socket=" << _sockfd << " remoteHandle=" << xHEX0N(_remoteHandle,8));
		NTV2DisconnectFromNub(_sockfd);
		_remoteHandle = LWord(INVALID_NUB_HANDLE);
		_boardOpened = false;
		_nubProtocolVersion = ntv2NubProtocolVersionNone;
		return true;
	}
	// Wasn't open.
	_boardOpened = false;
	return false;
}
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)


CNTV2DriverInterface & CNTV2DriverInterface::operator = (const CNTV2DriverInterface & inRHS)
{	(void) inRHS;
	NTV2_ASSERT(false && "Not assignable");
	return *this;
}	//	operator =


CNTV2DriverInterface::CNTV2DriverInterface (const CNTV2DriverInterface & inObjToCopy)
{	(void) inObjToCopy;
	NTV2_ASSERT(false && "Not copyable");
}	//	copy constructor


// Common remote card read register.  Subclasses have overloaded function
// that does platform-specific read of register on local card.
bool CNTV2DriverInterface::ReadRegister (const ULWord inRegisterNumber, ULWord & outRegisterValue, const ULWord inRegisterMask, const ULWord inRegisterShift)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return !NTV2ReadRegisterRemote (_sockfd, _remoteHandle, _nubProtocolVersion,
										inRegisterNumber, &outRegisterValue, inRegisterMask, inRegisterShift);
#else
	(void) inRegisterNumber;
	(void) outRegisterValue;
	(void) inRegisterMask;
	(void) inRegisterShift;
#endif
	return false;
}


// Common remote card read multiple registers.  Subclasses have overloaded function
// that does platform-specific read of multiple register on local card.
bool CNTV2DriverInterface::ReadRegisterMulti (ULWord numRegs, ULWord *whichRegisterFailed, NTV2ReadWriteRegisterSingle aRegs[])
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
	{
		return !NTV2ReadRegisterMultiRemote(_sockfd,
								_remoteHandle,
								_nubProtocolVersion,
								numRegs,
								whichRegisterFailed,
								aRegs);
	}
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)

	// TODO: Make platform specific versions that pass the whole shebang
	// down to the drivers which fills in the desired values in a single
	// context switch OR just get them from register set mapped into userspace
	for (ULWord i = 0; i < numRegs; i++)
	{
		if (!ReadRegister (aRegs[i].registerNumber, aRegs[i].registerValue, aRegs[i].registerMask, aRegs[i].registerShift))
		{
			*whichRegisterFailed = aRegs[i].registerNumber;
			return false;
		}
	}
	return true;
}

bool CNTV2DriverInterface::DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool			inIsRead,
										const ULWord		inFrameNumber,
										ULWord *			pFrameBuffer,
										const ULWord		inOffsetBytes,
										const ULWord		inByteCount,
										const bool			inSynchronous)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	return !NTV2DMATransferRemote(_sockfd,
								_remoteHandle,
								_nubProtocolVersion,
								inDMAEngine,
								inIsRead,
								inFrameNumber,
								pFrameBuffer,
								inOffsetBytes,
								inByteCount,
								inSynchronous);
#else
	(void) inDMAEngine;
	(void) inIsRead;
	(void) inFrameNumber;
	(void) pFrameBuffer;
	(void) inOffsetBytes;
	(void) inByteCount;
	(void) inSynchronous;
	return false;
#endif
}


// Common remote card write register.  Subclasses have overloaded function
// that does platform-specific write of register on local card.
bool CNTV2DriverInterface::WriteRegister (ULWord registerNumber, ULWord registerValue, ULWord registerMask, ULWord registerShift)
{
#if defined(NTV2_WRITEREG_PROFILING)
	//	Recording is done in platform-specific WriteRegister
#endif	//	NTV2_WRITEREG_PROFILING
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());

	return !NTV2WriteRegisterRemote(_sockfd,
								_remoteHandle,
								_nubProtocolVersion,
								registerNumber,
								registerValue,
								registerMask,
								registerShift);
#else
	(void) registerNumber;
	(void) registerValue;
	(void) registerMask;
	(void) registerShift;
	return false;
#endif
}

// Common remote card waitforinterrupt.  Subclasses have overloaded function
// that does platform-specific waitforinterrupt on local cards.
bool CNTV2DriverInterface::WaitForInterrupt (INTERRUPT_ENUMS eInterrupt, ULWord timeOutMs)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());

	return !NTV2WaitForInterruptRemote(	_sockfd,
										_remoteHandle,
										_nubProtocolVersion,
										eInterrupt,
										timeOutMs);
#else
	(void) eInterrupt;
	(void) timeOutMs;
	return false;
#endif
}

// Common remote card autocirculate.  Subclasses have overloaded function
// that does platform-specific autocirculate on local cards.
bool CNTV2DriverInterface::AutoCirculate (AUTOCIRCULATE_DATA & autoCircData)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());

	switch(autoCircData.eCommand)
	{
		case eStartAutoCirc:
		case eAbortAutoCirc:
		case ePauseAutoCirc:
		case eFlushAutoCirculate:
		case eGetAutoCirc:
		case eStopAutoCirc:
			return !NTV2AutoCirculateRemote(_sockfd, _remoteHandle,	_nubProtocolVersion, autoCircData);
		default:	// Others not handled
			return false;
	}
#else
	(void) autoCircData;
	return false;
#endif
}

bool CNTV2DriverInterface::NTV2Message (NTV2_HEADER * pInMessage)
{
	(void) pInMessage;
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	return !NTV2MessageRemote(_sockfd, _remoteHandle, _nubProtocolVersion, pInMessage);
#else
	return false;
#endif
}


// Common remote card DriverGetBitFileInformation.  Subclasses have overloaded function
// that does platform-specific function on local cards.
bool CNTV2DriverInterface::DriverGetBitFileInformation (BITFILE_INFO_STRUCT & bitFileInfo, NTV2BitFileType bitFileType)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	//NTV2_ASSERT(IsRemote());
	if (IsRemote())
	{
		return ! NTV2DriverGetBitFileInformationRemote(	_sockfd,
														_remoteHandle,
														_nubProtocolVersion,
														bitFileInfo,
														bitFileType);
	}
	else
#endif
	{
		if (::NTV2DeviceHasSPIFlash(_boardID))
		{
			ParseFlashHeader(bitFileInfo);
			bitFileInfo.bitFileType = 0;
			switch (_boardID)
			{
				case DEVICE_ID_CORVID1:						bitFileInfo.bitFileType = NTV2_BITFILE_CORVID1_MAIN;				break;
				case DEVICE_ID_CORVID22:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID22_MAIN;				break;
				case DEVICE_ID_CORVID24:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID24_MAIN;				break;
				case DEVICE_ID_CORVID3G:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID3G_MAIN;				break;
				case DEVICE_ID_CORVID44:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44;					break;
				case DEVICE_ID_CORVID88:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID88;					break;
				case DEVICE_ID_CORVIDHEVC:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVIDHEVC;					break;
				case DEVICE_ID_IO4K:						bitFileInfo.bitFileType = NTV2_BITFILE_IO4K_MAIN;					break;
				case DEVICE_ID_IO4KUFC:						bitFileInfo.bitFileType = NTV2_BITFILE_IO4KUFC_MAIN;				break;
				case DEVICE_ID_IOEXPRESS:					bitFileInfo.bitFileType = NTV2_BITFILE_IOEXPRESS_MAIN;				break;
				case DEVICE_ID_IOXT:						bitFileInfo.bitFileType = NTV2_BITFILE_IOXT_MAIN;					break;
				case DEVICE_ID_KONA3G:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA3G_MAIN;					break;
				case DEVICE_ID_KONA3GQUAD:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA3G_QUAD;					break;
				case DEVICE_ID_KONA4:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA4_MAIN;					break;
				case DEVICE_ID_KONA4UFC:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA4UFC_MAIN;				break;
				case DEVICE_ID_KONALHEPLUS:					bitFileInfo.bitFileType = NTV2_BITFILE_KONALHE_PLUS;				break;
				case DEVICE_ID_KONALHI:						bitFileInfo.bitFileType = NTV2_BITFILE_LHI_MAIN;					break;
				case DEVICE_ID_TTAP:						bitFileInfo.bitFileType = NTV2_BITFILE_TTAP_MAIN;					break;

				case DEVICE_ID_KONALHIDVI:					bitFileInfo.bitFileType = NTV2_BITFILE_NUMBITFILETYPES;				break;
				case DEVICE_ID_KONAIP_2022:                 bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2022;                 break;
				case DEVICE_ID_KONAIP_4CH_2SFP:				bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_4CH_2SFP;				break;
				case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:		bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_1RX_1TX_1SFP_J2K;		break;
				case DEVICE_ID_KONAIP_2TX_1SFP_J2K:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2TX_1SFP_J2K;			break;
				case DEVICE_ID_CORVIDHBR:					bitFileInfo.bitFileType = NTV2_BITFILE_NUMBITFILETYPES;				break;
				case DEVICE_ID_IO4KPLUS:					bitFileInfo.bitFileType = NTV2_BITFILE_IO4KPLUS_MAIN;				break;
                case DEVICE_ID_IOIP_2022:					bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2022;					break;
                case DEVICE_ID_IOIP_2110:					bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2110;					break;
				case DEVICE_ID_KONAIP_1RX_1TX_2110:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_1RX_1TX_2110;			break;
				case DEVICE_ID_KONA1:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA1;						break;
                case DEVICE_ID_KONAIP_2110:                 bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2110;                 break;
                case DEVICE_ID_KONAHDMI:					bitFileInfo.bitFileType = NTV2_BITFILE_KONAHDMI;					break;
				case DEVICE_ID_KONA5:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_MAIN;					break;
                case DEVICE_ID_KONA5_8KMK:                  bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_8KMK_MAIN;				break;
				case DEVICE_ID_KONA5_2X4K:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_2X4K_MAIN;				break;
				case DEVICE_ID_KONA5_3DLUT:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_3DLUT_MAIN;			break;
				case DEVICE_ID_CORVID44_8KMK:               bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_8KMK_MAIN;			break;
				case DEVICE_ID_KONA5_8K:                    bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_8K_MAIN;				break;
				case DEVICE_ID_CORVID44_8K:                 bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_8K_MAIN;			break;
				case DEVICE_ID_CORVID44_2X4K:               bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_2X4K_MAIN;			break;
				case DEVICE_ID_TTAP_PRO:					bitFileInfo.bitFileType = NTV2_BITFILE_TTAP_PRO_MAIN;					break;
                case DEVICE_ID_NOTFOUND:					bitFileInfo.bitFileType = NTV2_BITFILE_TYPE_INVALID;				break;
			#if !defined (_DEBUG)
				default:					break;
			#endif
			}
			bitFileInfo.checksum = 0;
			bitFileInfo.structVersion = 0;
			bitFileInfo.structSize = sizeof(BITFILE_INFO_STRUCT);
			bitFileInfo.whichFPGA = eFPGAVideoProc;

			string bitFileDesignNameString = bitFileInfo.designNameStr;
			bitFileDesignNameString += ".bit";
			strcpy(bitFileInfo.designNameStr, bitFileDesignNameString.c_str());

			return true;
		}
		return false;
	}
}

bool CNTV2DriverInterface::GetPackageInformation(PACKAGE_INFO_STRUCT & packageInfo)
{
    if(!IsDeviceReady(false) || !IsIPDevice())
    {
        // cannot read flash
        return false;
    }

    string packInfo;
    ULWord deviceID = (ULWord)_boardID;
    ReadRegister (kRegBoardID, deviceID);

    if (CNTV2AxiSpiFlash::DeviceSupported((NTV2DeviceID)deviceID))
    {
        CNTV2AxiSpiFlash spiFlash(_boardNumber, false);

        uint32_t offset = spiFlash.Offset(SPI_FLASH_SECTION_MCSINFO);
        vector<uint8_t> mcsInfoData;
        if (spiFlash.Read(offset, mcsInfoData, 256))
        {
            packInfo.assign(mcsInfoData.begin(), mcsInfoData.end());

            // remove any trailing nulls
            size_t found = packInfo.find('\0');
            if (found != string::npos)
            {
                packInfo.resize(found);
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        ULWord baseAddress = (16 * 1024 * 1024) - (3 * 256 * 1024);
		const ULWord dwordSizeCount = 256/4;

        WriteRegister(kRegXenaxFlashAddress, (ULWord)1);   // bank 1
        WriteRegister(kRegXenaxFlashControlStatus, 0x17);
        bool busy = true;
        ULWord timeoutCount = 1000;
        do
        {
            ULWord regValue;
            ReadRegister(kRegXenaxFlashControlStatus, regValue);
            if (regValue & BIT(8))
            {
                busy = true;
                timeoutCount--;
            }
            else
                busy = false;
        } while (busy == true && timeoutCount > 0);
        if (timeoutCount == 0)
            return false;

		ULWord* bitFilePtr =  new ULWord[dwordSizeCount];
        for ( ULWord count = 0; count < dwordSizeCount; count++, baseAddress += 4 )
        {
            WriteRegister(kRegXenaxFlashAddress, baseAddress);
            WriteRegister(kRegXenaxFlashControlStatus, 0x0B);
            busy = true;
            timeoutCount = 1000;
            do
            {
                ULWord regValue;
                ReadRegister(kRegXenaxFlashControlStatus, regValue);
                if ( regValue & BIT(8))
                {
                    busy = true;
                    timeoutCount--;
                }
                else
                    busy = false;
            } while(busy == true && timeoutCount > 0);
            if (timeoutCount == 0)
			{
				delete [] bitFilePtr;
                return false;
			}
            ReadRegister(kRegXenaxFlashDOUT, bitFilePtr[count]);
        }

        packInfo = (char*)bitFilePtr;

		delete [] bitFilePtr;
    }

    istringstream iss(packInfo);
    vector<string> results;
    string token;
    while (getline(iss,token, ' '))
    {
        results.push_back(token);
    }

    if (results.size() < 8)
    {
        return false;
    }

    packageInfo.date = results[1];
    token = results[2];
    token.erase(remove(token.begin(), token.end(), '\n'), token.end());
    packageInfo.time = token;
    packageInfo.buildNumber   = results[4];
    packageInfo.packageNumber = results[7];

    return true;
}

// Common remote card DriverGetBuildInformation.  Subclasses have overloaded function
// that does platform-specific function on local cards.
bool CNTV2DriverInterface::DriverGetBuildInformation (BUILD_INFO_STRUCT & buildInfo)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT (IsRemote());
	return ! NTV2DriverGetBuildInformationRemote (_sockfd,  _remoteHandle,  _nubProtocolVersion,  buildInfo);
#else
	(void) buildInfo;
	return false;
#endif
}

bool CNTV2DriverInterface::BitstreamWrite (const NTV2_POINTER & inBuffer, const bool inFragment, const bool inSwap)
{
	NTV2Bitstream bsMsg (inBuffer,
						 BITSTREAM_WRITE |
						 (inFragment? BITSTREAM_FRAGMENT : 0) |
						 (inSwap? BITSTREAM_SWAP : 0));
	return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&bsMsg));
}

bool CNTV2DriverInterface::BitstreamReset (const bool inConfiguration, const bool inInterface)
{
	NTV2_POINTER inBuffer;
	NTV2Bitstream bsMsg (inBuffer,
						 (inConfiguration? BITSTREAM_RESET_CONFIG : 0) |
						 (inInterface? BITSTREAM_RESET_MODULE : 0));
	return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&bsMsg));
}

bool CNTV2DriverInterface::BitstreamStatus (NTV2ULWordVector & outRegValues)
{
	outRegValues.reserve(BITSTREAM_MCAP_DATA);
	outRegValues.clear();

	NTV2_POINTER inBuffer;
	NTV2Bitstream bsMsg (inBuffer, BITSTREAM_READ_REGISTERS);
	if (!NTV2Message (reinterpret_cast<NTV2_HEADER*>(&bsMsg)))
		return false;

	for (UWord ndx(0);  ndx < BITSTREAM_MCAP_DATA;  ndx++)
		outRegValues.push_back(bsMsg.mRegisters[ndx]);

	return true;
}


//
// InitMemberVariablesOnOpen
// NOTE _boardID must be set before calling this routine.
void CNTV2DriverInterface::InitMemberVariablesOnOpen (NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat)
{
	_ulFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID,frameGeometry,frameFormat);
	_ulNumFrameBuffers = ::NTV2DeviceGetNumberFrameBuffers(_boardID,frameGeometry,frameFormat);

	ULWord returnVal1 = false;
	ULWord returnVal2 = false;
	if(::NTV2DeviceCanDo4KVideo(_boardID))
		ReadRegister(kRegGlobalControl2, returnVal1, kRegMaskQuadMode, kRegShiftQuadMode);
	if(::NTV2DeviceCanDo425Mux(_boardID))
		ReadRegister(kRegGlobalControl2, returnVal2, kRegMask425FB12, kRegShift425FB12);

    _pFrameBaseAddress = 0;
	//	for old KSD and KHD boards
	_pCh1FrameBaseAddress = 0;			//	DEPRECATE!
	_pCh2FrameBaseAddress = 0;			//	DEPRECATE!

    _pRegisterBaseAddress = 0;
	_pRegisterBaseAddressLength = 0;

	_pFS1FPGARegisterBaseAddress = 0;	//	DEPRECATE!
	_pXena2FlashBaseAddress  = 0;

}	//	InitMemberVariablesOnOpen


bool CNTV2DriverInterface::ParseFlashHeader (BITFILE_INFO_STRUCT & bitFileInfo)
{
	ULWord baseAddress = 0;
	const ULWord dwordSizeCount = 256/4;

	if(!IsDeviceReady(false))
	{
		// cannot read flash
		return false;
	}

	if (NTV2DeviceHasSPIv4(_boardID))
    {
        uint32_t val;
        ReadRegister((0x100000 + 0x08) / 4, val);
        if (val != 0x01)
        {
            // cannot read flash
            return false;
        }
    }

    if (::NTV2DeviceHasSPIv3(_boardID) || ::NTV2DeviceHasSPIv4(_boardID) || ::NTV2DeviceHasSPIv5(_boardID))
	{
		WriteRegister(kRegXenaxFlashAddress, (ULWord)0);
		WriteRegister(kRegXenaxFlashControlStatus, 0x17);
		bool busy = true;
		ULWord timeoutCount = 1000;
		do
		{
			ULWord regValue;
			ReadRegister(kRegXenaxFlashControlStatus, regValue);
			if (regValue & BIT(8))
			{
				busy = true;
				timeoutCount--;
			}
			else
				busy = false;
		} while (busy == true && timeoutCount > 0);
		if (timeoutCount == 0)
			return false;
	}

	ULWord* bitFilePtr =  new ULWord[dwordSizeCount];

	for ( ULWord count = 0; count < dwordSizeCount; count++, baseAddress += 4 )
	{
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteRegister(kRegXenaxFlashControlStatus, 0x0B);
		bool busy = true;
		ULWord timeoutCount = 1000;
		do
		{
			ULWord regValue;
			ReadRegister(kRegXenaxFlashControlStatus, regValue);
			if ( regValue & BIT(8))
			{
				busy = true;
				timeoutCount--;
			}
			else
				busy = false;
		} while(busy == true && timeoutCount > 0);
		if (timeoutCount == 0)
		{
			delete [] bitFilePtr;
			return false;
		}
		ReadRegister(kRegXenaxFlashDOUT, bitFilePtr[count]);
	}

	CNTV2Bitfile fileInfo;
	std::string headerError = fileInfo.ParseHeaderFromBuffer((uint8_t*)bitFilePtr);
	if (headerError.size() == 0)
	{
		strncpy(bitFileInfo.dateStr, fileInfo.GetDate().c_str(), NTV2_BITFILE_DATETIME_STRINGLENGTH);
		strncpy(bitFileInfo.timeStr, fileInfo.GetTime().c_str(), NTV2_BITFILE_DATETIME_STRINGLENGTH);
		strncpy(bitFileInfo.designNameStr, fileInfo.GetDesignName().c_str(), NTV2_BITFILE_DESIGNNAME_STRINGLENGTH);
		strncpy(bitFileInfo.partNameStr, fileInfo.GetPartName().c_str(), NTV2_BITFILE_PARTNAME_STRINGLENGTH);
		bitFileInfo.numBytes = ULWord(fileInfo.GetProgramStreamLength());
	}

	delete [] bitFilePtr;

	if (headerError.size() == 0)
		return true;
	else
		return false;
}

void CNTV2DriverInterface::BumpEventCount (const INTERRUPT_ENUMS eInterruptType)
{
	mEventCounts [eInterruptType] = mEventCounts [eInterruptType] + 1;

}	//	BumpEventCount

bool CNTV2DriverInterface::IsDeviceReady(bool checkValid)
{
	if (IsIPDevice())
	{
		if(!IsMBSystemReady())
			return false;

        if(checkValid && !IsMBSystemValid())
			return false;
	}
	return true;
}

bool CNTV2DriverInterface::IsMBSystemValid()
{
	if (IsIPDevice())
	{
        uint32_t val;
        ReadRegister(SAREK_REGS + kRegSarekIfVersion, val);
        if (val == SAREK_IF_VERSION)
            return true;
        else
            return false;
	}
	return true;
}

bool CNTV2DriverInterface::IsMBSystemReady()
{
	if (IsIPDevice())
	{
		uint32_t val;
		ReadRegister(SAREK_REGS + kRegSarekMBState, val);
		if (val != 0x01)
		{
			//MB not ready
			return false;
		}
		else
		{
            // Not enough to read MB State, we need to make sure MB is running
            ReadRegister(SAREK_REGS + kRegSarekMBUptime, val);
            if (val < 2)
                return false;
            else
                return true;
		}
	}
	return false;
}

#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	bool CNTV2DriverInterface::GetRecordedRegisterWrites (NTV2RegisterWrites & outRegWrites) const
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		outRegWrites = mRegWrites;
		return true;
	}

	bool CNTV2DriverInterface::StartRecordRegisterWrites (const bool inSkipActualWrites)
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		if (mRecordRegWrites)
			return false;	//	Already recording
		mRegWrites.clear();
		mRecordRegWrites = true;
		mSkipRegWrites = inSkipActualWrites;
		return true;
	}

	bool CNTV2DriverInterface::ResumeRecordRegisterWrites (void)
	{	//	Identical to Start, but don't clear mRegWrites nor change mSkipRegWrites
		AJAAutoLock	autoLock(&mRegWritesLock);
		if (mRecordRegWrites)
			return false;	//	Already recording
		mRecordRegWrites = true;
		return true;
	}

	bool CNTV2DriverInterface::IsRecordingRegisterWrites (void) const
	{	//	NB: This will return false if paused
		AJAAutoLock	autoLock(&mRegWritesLock);
		return mRecordRegWrites;
	}

	bool CNTV2DriverInterface::StopRecordRegisterWrites (void)
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		mRecordRegWrites = mSkipRegWrites = false;
		return true;
	}

	bool CNTV2DriverInterface::PauseRecordRegisterWrites (void)
	{	//	Identical to Stop, but don't change mSkipRegWrites
		AJAAutoLock	autoLock(&mRegWritesLock);
		if (!mRecordRegWrites)
			return false;	//	Already stopped/paused
		mRecordRegWrites = false;
		return true;
	}

	ULWord CNTV2DriverInterface::GetNumRecordedRegisterWrites (void) const
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		return ULWord(mRegWrites.size());
	}
#endif	//	NTV2_WRITEREG_PROFILING


#if !defined (NTV2_DEPRECATE)
NTV2BoardType CNTV2DriverInterface::GetCompileFlag ()
{
	NTV2BoardType eBoardType = BOARDTYPE_UNKNOWN;

#ifdef HDNTV
	eBoardType = BOARDTYPE_HDNTV;
#elif defined KSD
	eBoardType = BOARDTYPE_KSD;
#elif defined KHD
	eBoardType = BOARDTYPE_KHD;
#elif defined XENA2
	eBoardType = BOARDTYPE_AJAXENA2;
#elif defined BORG
	eBoardType = BOARDTYPE_BORG;
#endif

	return eBoardType;
}
#endif	//	!NTV2_DEPRECATE


#if defined (AJADLL_BUILD) || defined (AJASTATIC)
	//	This code forces link/load errors if the SDK client was built with NTV2_DEPRECATE defined,
	//	but the SDK lib/dylib/DLL was built without NTV2_DEPRECATE defined, ... or vice-versa...
	#if defined (NTV2_DEPRECATE)
		AJAExport	int	gNTV2_DEPRECATE	(void);
		AJAExport	int	gNTV2_DEPRECATE	(void){return 0;}
	#else
		AJAExport	int	gNTV2_NON_DEPRECATE	(void);
		AJAExport	int	gNTV2_NON_DEPRECATE	(void){return 0;}
	#endif
#endif	//	AJADLL_BUILD or AJASTATIC

#if defined(NTV2_FORCE_NO_DEVICE)
void CNTV2DriverInterface::NTV2NoDevInitRegisters(void)
{
	// 2554 registers:
	WriteRegister (kRegGlobalControl, 0x30000202);  // Reg 0  // Frame Rate: 59.94, Frame Geometry: 1920x1080, Standard: 1080p, Reference Source: Reference In, Ch 2 link B 1080p 50/60: Off, LEDs ...., Register Clocking: Sync To Field, Ch 1 RP-188 output: Enabled, Ch 2 RP-188 output: Enabled, Color Correction: Channel: 1 Bank 0
	WriteRegister (kRegCh1Control, 0x00200080);  // Reg 1  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 8 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	WriteRegister (kRegCh1PCIAccessFrame, 0x00000001);  // Reg 2
	WriteRegister (kRegCh1OutputFrame, 0x0000000E);  // Reg 3
	WriteRegister (kRegCh1InputFrame, 0x00000003);  // Reg 4
	WriteRegister (kRegCh2Control, 0x00200080);  // Reg 5  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 8 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	WriteRegister (kRegCh2PCIAccessFrame, 0x00000005);  // Reg 6
	WriteRegister (kRegCh2OutputFrame, 0x00000004);  // Reg 7
	WriteRegister (kRegCh2InputFrame, 0x00000007);  // Reg 8
	WriteRegister (kRegVidProc1Control, 0x00000000);  // Reg 9  // Mode: Full Raster, FG Control: Full Raster, BG Control: Full Raster, VANC Pass-Thru: Foreground, FG Matte: Disabled, BG Matte: Disabled, Input Sync: in sync, Limiting: Legal SDI, Split Video Std: 1080i
	WriteRegister (kRegVidProcXptControl, 0x00000000);  // Reg 10
	WriteRegister (kRegMixer1Coefficient, 0x00010000);  // Reg 11
	WriteRegister (kRegSplitControl, 0x00000000);  // Reg 12  // Split Start: 0000 0000, Split Slope: 0000 0000, Split Type: Horizontal
	WriteRegister (kRegFlatMatteValue, 0x20080200);  // Reg 13  // Flat Matte Cb: 200, Flat Matte Y: 1C0, Flat Matte Cr: 200
	WriteRegister (kRegOutputTimingControl, 0x08001000);  // Reg 14
	WriteRegister (kRegReserved15, 0x00000000);  // Reg 15
	WriteRegister (kRegReserved16, 0x00000000);  // Reg 16
	WriteRegister (kRegFlashProgramReg, 0x12345678);  // Reg 17
	WriteRegister (kRegLineCount, 0x00000142);  // Reg 18
	WriteRegister (kRegAud1Delay, 0x1FDF0000);  // Reg 19
	WriteRegister (kRegVidIntControl, 0x001C01C7);  // Reg 20  // Output 1 Vertical Enable: Y, Input 1 Vertical Enable: Y, Input 2 Vertical Enable: Y, Audio Out Wrap Interrupt Enable: N, Audio In Wrap Interrupt Enable: N, Wrap Rate Interrupt Enable: Y, UART Tx Interrupt EnableY, UART Rx Interrupt EnableY, UART Rx Interrupt ClearInactive, UART 2 Tx Interrupt EnableN, Output 2 Vertical Enable: Y, Output 3 Vertical Enable: Y, Output 4 Vertical Enable: Y, Output 4 Vertical Clear: Inactive, Output 3 Vertical Clear: Inactive, Output 2 Vertical Clear: Inactive, UART Tx Interrupt ClearInactive, Wrap Rate Interrupt ClearInactive, UART 2 Tx Interrupt ClearInactive, Audio Out Wrap Interrupt ClearInactive, Input 2 Vertical Clear: Inactive, Input 1 Vertical Clear: Inactive, Output 1 Vertical Clear: Inactive
	WriteRegister (kRegStatus, 0x00200000);  // Reg 21  // Input 1 Vertical Blank: Inactive, Input 1 Field ID: 1, Input 1 Vertical Interrupt: Inactive, Input 2 Vertical Blank: Inactive, Input 2 Field ID: 0, Input 2 Vertical Interrupt: Inactive, Output 1 Vertical Blank: Inactive, Output 1 Field ID: 0, Output 1 Vertical Interrupt: Inactive, Output 2 Vertical Blank: Inactive, Output 2 Field ID: 0, Output 2 Vertical Interrupt: Inactive, Aux Vertical Interrupt: Inactive, I2C 1 Interrupt: Inactive, I2C 2 Interrupt: Inactive, Chunk Rate Interrupt: Inactive, Wrap Rate Interrupt: Inactive, Audio Out Wrap Interrupt: Inactive, Audio 50Hz Interrupt: Inactive
	WriteRegister (kRegInputStatus, 0x0F000000);  // Reg 22  // Input 1 Frame Rate: Unknown, Input 1 Geometry: Unknown, Input 1 Scan Mode: Interlaced, Input 2 Frame Rate: Unknown, Input 2 Geometry: Unknown, Input 2 Scan Mode: Interlaced, Reference Frame Rate: Unknown, Reference Geometry: Unknown, Reference Scan Mode: Interlaced, AES Channel 1-2: Invalid, AES Channel 3-4: Invalid, AES Channel 5-6: Invalid, AES Channel 7-8: Invalid
	WriteRegister (kRegAud1Detect, 0x00000000);  // Reg 23  // Group 0 CH 1-2: Absent, Group 0 CH 3-4: Absent, Group 1 CH 1-2: Absent, Group 1 CH 3-4: Absent, Group 2 CH 1-2: Absent, Group 2 CH 3-4: Absent, Group 3 CH 1-2: Absent, Group 3 CH 3-4: Absent
	WriteRegister (kRegAud1Control, 0xA8F00300);  // Reg 24  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut1: Enabled, Audio Embedder SDIOut2: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 16-Channel , 48kHz, 48kHz Support, Embedded Support, 8-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Present, Cable: BNC, Audio Buffer Size: 4 MB
	WriteRegister (kRegAud1SourceSelect, 0xF0004321);  // Reg 25  // Audio Source: Embedded Groups 1 and 2, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	WriteRegister (kRegAud1OutputLastAddr, 0x00000000);  // Reg 26
	WriteRegister (kRegAud1InputLastAddr, 0x00000000);  // Reg 27
	WriteRegister (kRegAud1Counter, 0xE618F83A);  // Reg 28
	WriteRegister (kRegRP188InOut1DBB, 0xFF000000);  // Reg 29  // RP188: No RP-188 received, Bypass: Disabled, Filter: FF, DBB: 00 00
	WriteRegister (kRegRP188InOut1Bits0_31, 0x00000000);  // Reg 30
	WriteRegister (kRegRP188InOut1Bits32_63, 0x00000000);  // Reg 31
	WriteRegister (kRegDMA1HostAddr, 0x01D22F78);  // Reg 32
	WriteRegister (kRegDMA1LocalAddr, 0x01525840);  // Reg 33
	WriteRegister (kRegDMA1XferCount, 0x00000000);  // Reg 34
	WriteRegister (kRegDMA1NextDesc, 0x00000000);  // Reg 35
	WriteRegister (kRegDMA2HostAddr, 0x0103E210);  // Reg 36
	WriteRegister (kRegDMA2LocalAddr, 0x007E9000);  // Reg 37
	WriteRegister (kRegDMA2XferCount, 0x00000000);  // Reg 38
	WriteRegister (kRegDMA2NextDesc, 0x00000000);  // Reg 39
	WriteRegister (kRegDMA3HostAddr, 0x00000000);  // Reg 40
	WriteRegister (kRegDMA3LocalAddr, 0x00000000);  // Reg 41
	WriteRegister (kRegDMA3XferCount, 0x00000000);  // Reg 42
	WriteRegister (kRegDMA3NextDesc, 0x00000000);  // Reg 43
	WriteRegister (kRegDMA4HostAddr, 0x00000000);  // Reg 44
	WriteRegister (kRegDMA4LocalAddr, 0x00000000);  // Reg 45
	WriteRegister (kRegDMA4XferCount, 0x00000000);  // Reg 46
	WriteRegister (kRegDMA4NextDesc, 0x00000000);  // Reg 47
	WriteRegister (kRegDMAControl, 0x01243C00);  // Reg 48  // DMA 1 Int Active?: N, DMA 2 Int Active?: N, DMA 3 Int Active?: N, DMA 4 Int Active?: N, Bus Error Int Active?: N, DMA 1 Busy?: N, DMA 2 Busy?: N, DMA 3 Busy?: N, DMA 4 Busy?: N, Strap: Not Installed, Firmware Rev: 0x3C (60), Gen: 2, Lanes: 4
	WriteRegister (kRegDMAIntControl, 0x00000000);  // Reg 49  // DMA 1 Enabled?: N, DMA 2 Enabled?: N, DMA 3 Enabled?: N, DMA 4 Enabled?: N, Bus Error Enabled?: N, DMA 1 Active?: N, DMA 2 Active?: N, DMA 3 Active?: N, DMA 4 Active?: N, Bus Error: N
	WriteRegister (kRegBoardID, 0x10538200);  // Reg 50
	WriteRegister (kRegReserved51, 0x00000000);  // Reg 51
	WriteRegister (kRegReserved52, 0x00000000);  // Reg 52
	WriteRegister (kRegReserved53, 0x00000000);  // Reg 53
	WriteRegister (kRegReserved54, 0x38543030);  // Reg 54
	WriteRegister (kRegReserved55, 0x32393735);  // Reg 55
	WriteRegister (kRegReserved56, 0x00000000);  // Reg 56
	WriteRegister (kRegReserved57, 0x00000000);  // Reg 57
	WriteRegister (kRegXenaxFlashControlStatus, 0x40009C0B);  // Reg 58
	WriteRegister (kRegXenaxFlashAddress, 0x000000FC);  // Reg 59
	WriteRegister (kRegXenaxFlashDIN, 0x00000000);  // Reg 60
	WriteRegister (kRegXenaxFlashDOUT, 0x6B26B241);  // Reg 61
	WriteRegister (kRegReserved62, 0x00000000);  // Reg 62
	WriteRegister (kRegCPLDVersion, 0x00000003);  // Reg 63  // CPLD Version: 3, Failsafe Bitfile Loaded: No, Force Reload: N
	WriteRegister (kRegRP188InOut2DBB, 0xFF0000FF);  // Reg 64  // RP188: No RP-188 received, Bypass: Disabled, Filter: FF, DBB: 00 FF
	WriteRegister (kRegRP188InOut2Bits0_31, 0x00000000);  // Reg 65
	WriteRegister (kRegRP188InOut2Bits32_63, 0x00000000);  // Reg 66
	WriteRegister (kRegCanDoStatus, 0x00000003);  // Reg 67  // Has CanConnect Xpt Route ROM: Y
	WriteRegister (kRegCh1ColorCorrectionControl, 0x00000000);  // Reg 68  // (Register data relevant for V1 LUT, this device has V0 LUT), LUT3 Bank Select: Not Set, LUT4 Bank Select: Not Set
	WriteRegister (kRegCh2ColorCorrectionControl, 0x00000000);  // Reg 69  // (Register data relevant for V1 LUT, this device has V0 LUT), LUT3 Bank Select: Not Set, LUT4 Bank Select: Not Set
	WriteRegister (kRegRS422Transmit, 0x00000000);  // Reg 70
	WriteRegister (kRegRS422Receive, 0x00000000);  // Reg 71
	WriteRegister (kRegRS422Control, 0x0000010B);  // Reg 72
	WriteRegister (kRegReserved73, 0x00000000);  // Reg 73
	WriteRegister (kRegReserved74, 0x00000000);  // Reg 74
	WriteRegister (kRegReserved75, 0x00000000);  // Reg 75
	WriteRegister (kRegReserved76, 0x00000000);  // Reg 76
	WriteRegister (kRegReserved77, 0x00000000);  // Reg 77
	WriteRegister (kRegReserved78, 0x00000000);  // Reg 78
	WriteRegister (kRegReserved79, 0x00000000);  // Reg 79
	WriteRegister (kRegReserved80, 0x00000000);  // Reg 80
	WriteRegister (kRegAnalogInputStatus, 0x00000000);  // Reg 81
	WriteRegister (kRegAnalogInputControl, 0x00000000);  // Reg 82
	WriteRegister (kRegReserved83, 0x00000000);  // Reg 83
	WriteRegister (kRegFS1ProcAmpC1Y_C1CB, 0x00000000);  // Reg 84
	WriteRegister (kRegFS1ProcAmpC1CR_C2CB, 0x00000000);  // Reg 85
	WriteRegister (kRegFS1ProcAmpC2CROffsetY, 0x00000000);  // Reg 86
	WriteRegister (kRegAud2Delay, 0x00000000);  // Reg 87
	WriteRegister (kRegBitfileDate, 0x20200325);  // Reg 88  // Bitfile Date: 03/25/2020
	WriteRegister (kRegBitfileTime, 0x00151600);  // Reg 89  // Bitfile Time: 15:16:00
	WriteRegister (kRegFS1I2CControl, 0x00000000);  // Reg 90
	WriteRegister (kRegFS1I2C1Address, 0x00000000);  // Reg 91
	WriteRegister (kRegFS1I2C1Data, 0x00000000);  // Reg 92
	WriteRegister (kRegFS1I2C2Address, 0x00000000);  // Reg 93
	WriteRegister (kRegFS1I2C2Data, 0x00000000);  // Reg 94
	WriteRegister (kRegFS1ReferenceSelect, 0x00000000);  // Reg 95  // BNC Select(LHi): Ref, Ref BNC (Corvid): Disabled, LTC Present (also Reg 21): N, LTC Emb Out Enable: N, LTC Emb In Enable: N, LTC Emb In Received: N, LTC BNC Out Source: Reg112/113
	WriteRegister (kRegAverageAudioLevelChan1_2, 0x00000000);  // Reg 96
	WriteRegister (kRegAverageAudioLevelChan3_4, 0x00000000);  // Reg 97
	WriteRegister (kRegAverageAudioLevelChan5_6, 0x00000000);  // Reg 98
	WriteRegister (kRegAverageAudioLevelChan7_8, 0x00000000);  // Reg 99
	WriteRegister (kRegDMA1HostAddrHigh, 0x00000000);  // Reg 100
	WriteRegister (kRegDMA1NextDescHigh, 0x00000000);  // Reg 101
	WriteRegister (kRegDMA2HostAddrHigh, 0x00000000);  // Reg 102
	WriteRegister (kRegDMA2NextDescHigh, 0x00000000);  // Reg 103
	WriteRegister (kRegDMA3HostAddrHigh, 0x00000000);  // Reg 104
	WriteRegister (kRegDMA3NextDescHigh, 0x00000000);  // Reg 105
	WriteRegister (kRegDMA4HostAddrHigh, 0x00000000);  // Reg 106
	WriteRegister (kRegDMA4NextDescHigh, 0x00000000);  // Reg 107
	WriteRegister (kRegGlobalControl3, 0x00000000);  // Reg 108  // Bidirectional analog 1-4 input: Not Set, Quad Quad Mode Channel 1-4: Not Set, Quad Quad Squares Mode 1-4: Not Set, 
	WriteRegister (kRegReserved109, 0x00000000);  // Reg 109
	WriteRegister (kRegLTCEmbeddedBits0_31, 0x00000000);  // Reg 110
	WriteRegister (kRegLTCEmbeddedBits32_63, 0x00000000);  // Reg 111
	WriteRegister (kRegLTCAnalogBits0_31, 0x00000000);  // Reg 112
	WriteRegister (kRegLTCAnalogBits32_63, 0x00000000);  // Reg 113
	WriteRegister (kRegReserved114, 0x00000000);  // Reg 114
	WriteRegister (kRegAudioControl2, 0x00000000);  // Reg 115
	WriteRegister (kRegSysmonControl, 0x00000000);  // Reg 116
	WriteRegister (kRegSysmonConfig1_0, 0x00000000);  // Reg 117
	WriteRegister (kRegSysmonConfig2, 0x00000000);  // Reg 118
	WriteRegister (kRegSysmonVccIntDieTemp, 0x53C1ABE1);  // Reg 119  // Die Temperature: 64.97 Celcius  (148.94 Fahrenheit, Core Voltage:  0.98 Volts DC
	WriteRegister (kRegInternalExternalVoltage, 0x000798A4);  // Reg 120
	WriteRegister (kRegFlashProgramReg2, 0x00000000);  // Reg 121
	WriteRegister (kRegHDMIOut3DStatus1, 0x00000000);  // Reg 122
	WriteRegister (kRegHDMIOut3DStatus2, 0x00000000);  // Reg 123
	WriteRegister (kRegHDMIOut3DControl, 0x00000080);  // Reg 124
	WriteRegister (kRegHDMIOutControl, 0x00000000);  // Reg 125  // Video Standard:  (1080i), Color Mode: YCbCr, Video Rate: , Scan Mode: Interlaced, Bit Depth: 8-bit, Output Color Sampling: 4:2:2, Output Bit Depth: 8, Src Color Sampling: YC422, Src Bits Per Component: 8, Output Range: SMPTE, Audio Channels: 2, Output: HDMI
	WriteRegister (kRegHDMIInputStatus, 0x00000000);  // Reg 126  // HDMI Input: Unlocked, HDMI Input: Unstable, Color Mode: YCbCr, Bitdepth: 8-bit, Audio Channels: 8, Scan Mode: Interlaced, Standard: HD, Video Standard: 1080i, Protocol: HDMI, Video Rate : invalid
	WriteRegister (kRegHDMIInputControl, 0x00000000);  // Reg 127  // HDMI In EDID Write-Enable: Disabled, HDMI Force Output Params: Not Set, HDMI In Audio Chan Select: 1-2, hdmi_rx_8ch_src_off: N, Swap HDMI In Audio Ch. 3/4: N, Swap HDMI Out Audio Ch. 3/4: N, HDMI Prefer 420: Not Set, hdmi_rx_spdif_err: Not Set, hdmi_rx_afifo_under: Not Set, hdmi_rx_afifo_empty: Not Set, H polarity: Normal, V polarity: Normal, F polarity: Normal, DE polarity: Normal, Tx Src Sel: 0 (0x0000), Tx Center Cut: Not Set, Tx 12 bit: Not Set, RGB Input Gamut: Narrow Range (SMPTE), Tx_ch12_sel: 0 (0x0000), Input AVI Gamut: Narrow Range (SMPTE), EDID: Not Set
	WriteRegister (kRegAnalogOutControl, 0x00000000);  // Reg 128
	WriteRegister (kRegSDIOut1Control, 0x01040084);  // Reg 129  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	WriteRegister (kRegSDIOut2Control, 0x01040084);  // Reg 130  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	WriteRegister (kRegConversionControl, 0x00000000);  // Reg 131
	WriteRegister (kRegFrameSync1Control, 0x00000000);  // Reg 132
	WriteRegister (kRegI2CWriteData, 0x00000000);  // Reg 133
	WriteRegister (kRegFrameSync2Control, 0x00000000);  // Reg 134
	WriteRegister (kRegI2CWriteControl, 0x00000000);  // Reg 135
	WriteRegister (kRegXptSelectGroup1, 0x00000000);  // Reg 136  // NTV2_XptLUT1Input <== NTV2_XptBlack, NTV2_XptCSC1VidInput <== NTV2_XptBlack, NTV2_XptConversionModInput <== NTV2_XptBlack, NTV2_XptCompressionModInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup2, 0x00000000);  // Reg 137  // NTV2_XptFrameBuffer1Input <== NTV2_XptBlack, NTV2_XptFrameSync1Input <== NTV2_XptBlack, NTV2_XptFrameSync2Input <== NTV2_XptBlack, NTV2_XptDualLinkOut1Input <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup3, 0x00000000);  // Reg 138  // NTV2_XptAnalogOutInput <== NTV2_XptBlack, NTV2_XptSDIOut1Input <== NTV2_XptBlack, NTV2_XptSDIOut2Input <== NTV2_XptBlack, NTV2_XptCSC1KeyInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup4, 0x00000000);  // Reg 139  // NTV2_XptMixer1FGVidInput <== NTV2_XptBlack, NTV2_XptMixer1FGKeyInput <== NTV2_XptBlack, NTV2_XptMixer1BGVidInput <== NTV2_XptBlack, NTV2_XptMixer1BGKeyInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup5, 0x00000000);  // Reg 140  // NTV2_XptFrameBuffer2Input <== NTV2_XptBlack, NTV2_XptLUT2Input <== NTV2_XptBlack, NTV2_XptCSC2VidInput <== NTV2_XptBlack, NTV2_XptCSC2KeyInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup6, 0x00000000);  // Reg 141  // NTV2_XptWaterMarker1Input <== NTV2_XptBlack, NTV2_XptIICT1Input <== NTV2_XptBlack, NTV2_XptHDMIOutInput <== NTV2_XptBlack, NTV2_XptConversionMod2Input <== NTV2_XptBlack
	WriteRegister (kRegCSCoefficients1_2, 0x00000000);  // Reg 142  // Video Key Sync Status: OK, Make Alpha From Key Input: Disabled, Matrix Select: Rec709, Use Custom Coeffs: N, Coefficient1: 0x0000, Coefficient2: 0x0000
	WriteRegister (kRegCSCoefficients3_4, 0x00000000);  // Reg 143  // RGB Range: Full (0x000-0x3FF), Coefficient3: 0x0000, Coefficient4: 0x0000
	WriteRegister (kRegCSCoefficients5_6, 0x00000000);  // Reg 144  // Coefficient5: 0x0000, Coefficient6: 0x0000
	WriteRegister (kRegCSCoefficients7_8, 0x00000000);  // Reg 145  // Coefficient7: 0x0000, Coefficient8: 0x0000
	WriteRegister (kRegCSCoefficients9_10, 0x00000000);  // Reg 146  // Coefficient9: 0x0000, Coefficient10: 0x0000
	WriteRegister (kRegCS2Coefficients1_2, 0x00000000);  // Reg 147  // Video Key Sync Status: OK, Make Alpha From Key Input: Disabled, Matrix Select: Rec709, Use Custom Coeffs: N, Coefficient1: 0x0000, Coefficient2: 0x0000
	WriteRegister (kRegCS2Coefficients3_4, 0x00000000);  // Reg 148  // RGB Range: Full (0x000-0x3FF), Coefficient3: 0x0000, Coefficient4: 0x0000
	WriteRegister (kRegCS2Coefficients5_6, 0x00000000);  // Reg 149  // Coefficient5: 0x0000, Coefficient6: 0x0000
	WriteRegister (kRegCS2Coefficients7_8, 0x00000000);  // Reg 150  // Coefficient7: 0x0000, Coefficient8: 0x0000
	WriteRegister (kRegCS2Coefficients9_10, 0x00000000);  // Reg 151  // Coefficient9: 0x0000, Coefficient10: 0x0000
	WriteRegister (kRegField1Line21CaptionDecode, 0x00000000);  // Reg 152
	WriteRegister (kRegField2Line21CaptionDecode, 0x00000000);  // Reg 153
	WriteRegister (kRegField1Line21CaptionEncode, 0x00000000);  // Reg 154
	WriteRegister (kRegField2Line21CaptionEncode, 0x00000000);  // Reg 155
	WriteRegister (kRegVANCGrabberSetup, 0x00000000);  // Reg 156
	WriteRegister (kRegVANCGrabberStatus1, 0x00000000);  // Reg 157
	WriteRegister (kRegVANCGrabberStatus2, 0x00000000);  // Reg 158
	WriteRegister (kRegVANCGrabberDataBuffer, 0x00000000);  // Reg 159
	WriteRegister (kRegVANCInserterSetup1, 0x00000000);  // Reg 160
	WriteRegister (kRegVANCInserterSetup2, 0x00000000);  // Reg 161
	WriteRegister (kRegVANCInserterDataBuffer, 0x00000000);  // Reg 162
	WriteRegister (kRegXptSelectGroup7, 0x00000000);  // Reg 163  // NTV2_XptWaterMarker2Input <== NTV2_XptBlack, NTV2_XptIICT2Input <== NTV2_XptBlack, NTV2_XptDualLinkOut2Input <== NTV2_XptBlack, 
	WriteRegister (kRegXptSelectGroup8, 0x00000000);  // Reg 164  // NTV2_XptSDIOut3Input <== NTV2_XptBlack, NTV2_XptSDIOut4Input <== NTV2_XptBlack, NTV2_XptSDIOut5Input <== NTV2_XptBlack, 
	WriteRegister (kRegCh1ControlExtended, 0x00000000);  // Reg 165  // Input Video 2:1 Decimate: Disabled, HDMI Rx Direct: Disabled, 3:2 Pulldown Mode: Disabled
	WriteRegister (kRegCh2ControlExtended, 0x00000000);  // Reg 166  // Input Video 2:1 Decimate: Disabled, HDMI Rx Direct: Disabled, 3:2 Pulldown Mode: Disabled
	WriteRegister (kRegAFDVANCGrabber, 0x00000000);  // Reg 167
	WriteRegister (kRegFS1DownConverter2Control, 0x00000000);  // Reg 168
	WriteRegister (kRegSDIOut3Control, 0x01040004);  // Reg 169  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	WriteRegister (kRegSDIOut4Control, 0x01040004);  // Reg 170  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	WriteRegister (kRegAFDVANCInserterSDI1, 0x00000000);  // Reg 171
	WriteRegister (kRegAFDVANCInserterSDI2, 0x00000000);  // Reg 172
	WriteRegister (kRegAudioChannelMappingCh1, 0x00000000);  // Reg 173
	WriteRegister (kRegAudioChannelMappingCh2, 0x00000000);  // Reg 174
	WriteRegister (kRegAudioChannelMappingCh3, 0x00000000);  // Reg 175
	WriteRegister (kRegAudioChannelMappingCh4, 0x00000000);  // Reg 176
	WriteRegister (kRegAudioChannelMappingCh5, 0x00000000);  // Reg 177
	WriteRegister (kRegAudioChannelMappingCh6, 0x00000000);  // Reg 178
	WriteRegister (kRegAudioChannelMappingCh7, 0x00000000);  // Reg 179
	WriteRegister (kRegAudioChannelMappingCh8, 0x00000000);  // Reg 180
	WriteRegister (kRegReserved181, 0x00000000);  // Reg 181
	WriteRegister (kRegReserved182, 0x00000000);  // Reg 182
	WriteRegister (kRegReserved183, 0x00000000);  // Reg 183
	WriteRegister (kRegReserved184, 0x00000000);  // Reg 184
	WriteRegister (kRegReserved185, 0x00000000);  // Reg 185
	WriteRegister (kRegReserved186, 0x00000000);  // Reg 186
	WriteRegister (kRegReserved187, 0x00000000);  // Reg 187
	WriteRegister (kRegSDIIn1VPIDA, 0x00000000);  // Reg 188
	WriteRegister (kRegSDIIn1VPIDB, 0x00000000);  // Reg 189
	WriteRegister (kRegAudioOutputSourceMap, 0x00003210);  // Reg 190  // AES Outputs 1-4 Source: AudSys1, Audio Channels 1-4, AES Outputs 5-8 Source: AudSys1, Audio Channels 5-8, AES Outputs 9-12 Source: AudSys1, Audio Channels 9-12, AES Outputs 13-16 Source: AudSys1, Audio Channels 13-16, Analog Audio Monitor Output Source: AudSys1, Channels 1-2, HDMI 2-Chl Audio Output Source: AudSys1, Channels 1-2, or HDMI 8-Chl Audio Output 1-4 Source: AudSys1, Channels 1-4, or HDMI 8-Chl Audio Output 5-8 Source: AudSys1, Channels 1-4
	WriteRegister (kRegXptSelectGroup11, 0x00000000);  // Reg 191  // NTV2_XptDualLinkIn1Input <== NTV2_XptBlack, NTV2_XptDualLinkIn1DSInput <== NTV2_XptBlack, NTV2_XptDualLinkIn2Input <== NTV2_XptBlack, NTV2_XptDualLinkIn2DSInput <== NTV2_XptBlack
	WriteRegister (kRegStereoCompressor, 0x00000000);  // Reg 192
	WriteRegister (kRegXptSelectGroup12, 0x00000000);  // Reg 193  // NTV2_XptLUT3Input <== NTV2_XptBlack, NTV2_XptLUT4Input <== NTV2_XptBlack, NTV2_XptLUT5Input <== NTV2_XptBlack, 
	WriteRegister (kRegFrameApertureOffset, 0x00000000);  // Reg 194
	WriteRegister (kRegReserved195, 0x00000000);  // Reg 195
	WriteRegister (kRegReserved196, 0x00000000);  // Reg 196
	WriteRegister (kRegReserved197, 0x00000000);  // Reg 197
	WriteRegister (kRegReserved198, 0x00000000);  // Reg 198
	WriteRegister (kRegReserved199, 0x00000000);  // Reg 199
	WriteRegister (kRegReserved200, 0x00000000);  // Reg 200
	WriteRegister (kRegReserved201, 0x00000000);  // Reg 201
	WriteRegister (kRegRP188InOut1Bits0_31_2, 0x00000000);  // Reg 202
	WriteRegister (kRegRP188InOut1Bits32_63_2, 0x00000000);  // Reg 203
	WriteRegister (kRegRP188InOut2Bits0_31_2, 0x00000000);  // Reg 204
	WriteRegister (kRegRP188InOut2Bits32_63_2, 0x00000000);  // Reg 205
	WriteRegister (kRegRP188InOut3Bits0_31_2, 0x00000000);  // Reg 206
	WriteRegister (kRegRP188InOut3Bits32_63_2, 0x00000000);  // Reg 207
	WriteRegister (kRegRP188InOut4Bits0_31_2, 0x00000000);  // Reg 208
	WriteRegister (kRegRP188InOut4Bits32_63_2, 0x00000000);  // Reg 209
	WriteRegister (kRegRP188InOut5Bits0_31_2, 0x63030608);  // Reg 210
	WriteRegister (kRegRP188InOut5Bits32_63_2, 0x00000004);  // Reg 211
	WriteRegister (kRegRP188InOut6Bits0_31_2, 0x63030608);  // Reg 212
	WriteRegister (kRegRP188InOut6Bits32_63_2, 0x00000004);  // Reg 213
	WriteRegister (kRegRP188InOut7Bits0_31_2, 0x63030608);  // Reg 214
	WriteRegister (kRegRP188InOut7Bits32_63_2, 0x00000004);  // Reg 215
	WriteRegister (kRegRP188InOut8Bits0_31_2, 0x63030608);  // Reg 216
	WriteRegister (kRegRP188InOut8Bits32_63_2, 0x00000004);  // Reg 217
	WriteRegister (kRegReserved218, 0x00000000);  // Reg 218
	WriteRegister (kRegReserved219, 0x00000000);  // Reg 219
	WriteRegister (kRegReserved220, 0x00000000);  // Reg 220
	WriteRegister (kRegReserved221, 0x00000000);  // Reg 221
	WriteRegister (kRegReserved222, 0x00000000);  // Reg 222
	WriteRegister (kRegReserved223, 0x00000000);  // Reg 223
	WriteRegister (kRegReserved224, 0x00000000);  // Reg 224
	WriteRegister (kRegReserved225, 0x00000000);  // Reg 225
	WriteRegister (kRegReserved226, 0x00000000);  // Reg 226
	WriteRegister (kRegReserved227, 0x00000000);  // Reg 227
	WriteRegister (kRegReserved228, 0x00000000);  // Reg 228
	WriteRegister (kRegReserved229, 0x00000000);  // Reg 229
	WriteRegister (kRegReserved230, 0x00000000);  // Reg 230
	WriteRegister (kRegReserved231, 0x00000000);  // Reg 231
	WriteRegister (kRegSDIInput3GStatus, 0x000C0000);  // Reg 232
	WriteRegister (kRegLTCStatusControl, 0x00000000);  // Reg 233  // LTC 1 Input Present: N, LTC 1 Input FB Timing Select): 0x00 (0), LTC 1 Bypass: Disabled, LTC 1 Bypass Select: 0, LTC 2 Input Present: N, LTC 2 Input FB Timing Select): 0x00 (0), LTC 2 Bypass: Disabled, LTC 2 Bypass Select: 0, LTC 1 Output FB Timing Select): 0x00 (0), LTC 2 Output FB Timing Select): 0x00 (0)
	WriteRegister (kRegSDIOut1VPIDA, 0x00000000);  // Reg 234
	WriteRegister (kRegSDIOut1VPIDB, 0x00000000);  // Reg 235
	WriteRegister (kRegSDIOut2VPIDA, 0x00000000);  // Reg 236
	WriteRegister (kRegSDIOut2VPIDB, 0x00000000);  // Reg 237
	WriteRegister (kRegSDIIn2VPIDA, 0x00000000);  // Reg 238
	WriteRegister (kRegSDIIn2VPIDB, 0x00000000);  // Reg 239
	WriteRegister (kRegAud2Control, 0x80C00300);  // Reg 240  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, Embedded Support, 8-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	WriteRegister (kRegAud2SourceSelect, 0x00000000);  // Reg 241  // Audio Source: AES Input, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	WriteRegister (kRegAud2OutputLastAddr, 0x00000000);  // Reg 242
	WriteRegister (kRegAud2InputLastAddr, 0x00000000);  // Reg 243
	WriteRegister (kRegRS4222Transmit, 0x00000000);  // Reg 244
	WriteRegister (kRegRS4222Receive, 0x00000000);  // Reg 245
	WriteRegister (kRegRS4222Control, 0x00000010);  // Reg 246
	WriteRegister (kRegVidProc2Control, 0x00000000);  // Reg 247  // Mode: Full Raster, FG Control: Full Raster, BG Control: Full Raster, VANC Pass-Thru: Foreground, FG Matte: Disabled, BG Matte: Disabled, Input Sync: in sync, Limiting: Legal SDI, Split Video Std: 1080i
	WriteRegister (kRegMixer2Coefficient, 0x00000000);  // Reg 248
	WriteRegister (kRegFlatMatte2Value, 0x18080240);  // Reg 249  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	WriteRegister (kRegXptSelectGroup9, 0x00000000);  // Reg 250  // NTV2_XptMixer2FGVidInput <== NTV2_XptBlack, NTV2_XptMixer2FGKeyInput <== NTV2_XptBlack, NTV2_XptMixer2BGVidInput <== NTV2_XptBlack, NTV2_XptMixer2BGKeyInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup10, 0x00000000);  // Reg 251  // NTV2_XptSDIOut1InputDS2 <== NTV2_XptBlack, NTV2_XptSDIOut2InputDS2 <== NTV2_XptBlack, , 
	WriteRegister (kRegLTC2EmbeddedBits0_31, 0x00000000);  // Reg 252
	WriteRegister (kRegLTC2EmbeddedBits32_63, 0x00000000);  // Reg 253
	WriteRegister (kRegLTC2AnalogBits0_31, 0x00000000);  // Reg 254
	WriteRegister (kRegLTC2AnalogBits32_63, 0x00000000);  // Reg 255
	WriteRegister (kRegSDITransmitControl, 0xF0000000);  // Reg 256  // (Bi-directional SDI not supported)
	WriteRegister (kRegCh3Control, 0x00000080);  // Reg 257  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	WriteRegister (kRegCh3OutputFrame, 0x00000000);  // Reg 258
	WriteRegister (kRegCh3InputFrame, 0x00000000);  // Reg 259
	WriteRegister (kRegCh4Control, 0x00000080);  // Reg 260  // Mode: Display, Format: NTV2_FBF_10BIT_YCBCR, Channel: Disabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	WriteRegister (kRegCh4OutputFrame, 0x00000000);  // Reg 261
	WriteRegister (kRegCh4InputFrame, 0x00000000);  // Reg 262
	WriteRegister (kRegXptSelectGroup13, 0x00000000);  // Reg 263  // NTV2_XptFrameBuffer3Input <== NTV2_XptBlack, , NTV2_XptFrameBuffer4Input <== NTV2_XptBlack, 
	WriteRegister (kRegXptSelectGroup14, 0x00000000);  // Reg 264  // , NTV2_XptSDIOut3InputDS2 <== NTV2_XptBlack, NTV2_XptSDIOut5InputDS2 <== NTV2_XptBlack, NTV2_XptSDIOut4InputDS2 <== NTV2_XptBlack
	WriteRegister (kRegStatus2, 0x00015400);  // Reg 265  // Input 3 Vertical Blank: Inactive, Input 3 Field ID: 0, Input 3 Vertical Interrupt: Inactive, Input 4 Vertical Blank: Inactive, Input 4 Field ID: 0, Input 4 Vertical Interrupt: Inactive, Input 5 Vertical Blank: Active, Input 5 Field ID: 0, Input 5 Vertical Interrupt: Inactive, Input 6 Vertical Blank: Active, Input 6 Field ID: 0, Input 6 Vertical Interrupt: Inactive, Input 7 Vertical Blank: Active, Input 7 Field ID: 0, Input 7 Vertical Interrupt: Inactive, Input 8 Vertical Blank: Active, Input 8 Field ID: 0, Input 8 Vertical Interrupt: Inactive, Output 5 Vertical Blank: Inactive, Output 5 Field ID: 0, Output 5 Vertical Interrupt: Inactive, Output 6 Vertical Blank: Inactive, Output 6 Field ID: 0, Output 6 Vertical Interrupt: Inactive, Output 7 Vertical Blank: Inactive, Output 7 Field ID: 0, Output 7 Vertical Interrupt: Inactive, Output 8 Vertical Blank: Inactive, Output 8 Field ID: 0, Output 8 Vertical Interrupt: Inactive, HDMI In Hot-Plug Detect Interrupt: Inactive, HDMI In Chip Interrupt: Inactive
	WriteRegister (kRegVidIntControl2, 0x0000FF06);  // Reg 266  // Input 3 Vertical Enable: Y, Input 4 Vertical Enable: Y, Input 5 Vertical Enable: Y, Input 6 Vertical Enable: Y, Input 7 Vertical Enable: Y, Input 8 Vertical Enable: Y, Output 5 Vertical Enable: Y, Output 6 Vertical Enable: Y, Output 7 Vertical Enable: Y, Output 8 Vertical Enable: Y, Output 8 Vertical Clear: Inactive, Output 7 Vertical Clear: Inactive, Output 6 Vertical Clear: Inactive, Output 5 Vertical Clear: Inactive, Input 8 Vertical Clear: Inactive, Input 7 Vertical Clear: Inactive, Input 6 Vertical Clear: Inactive, Input 5 Vertical Clear: Inactive, Input 4 Vertical Clear: Inactive, Input 3 Vertical Clear: Inactive
	WriteRegister (kRegGlobalControl2, 0xBC021009);  // Reg 267  // Reference source bit 4: Set, Quad Mode Channel 1-4: Set, Quad Mode Channel 5-8: Set, Independent Channel Mode: Not Set, 2MB Frame Support: Supported, Audio Mixer: Not Present, Is DNXIV Product: N, Audio 1 Play/Capture Mode: Off, Audio 2 Play/Capture Mode: Off, Audio 3 Play/Capture Mode: Off, Audio 4 Play/Capture Mode: Off, Audio 5 Play/Capture Mode: Off, Audio 6 Play/Capture Mode: Off, Audio 7 Play/Capture Mode: Off, Audio 8 Play/Capture Mode: Off, Ch 3 RP188 Output: Enabled, Ch 4 RP188 Output: Enabled, Ch 5 RP188 Output: Disabled, Ch 6 RP188 Output: Enabled, Ch 7 RP188 Output: Enabled, Ch 8 RP188 Output: Enabled, Ch 4 1080p50/p60 Link-B Mode: Disabled, Ch 6 1080p50/p60 Link-B Mode: Disabled, Ch 8 1080p50/p60 Link-B Mode: Disabled, Ch 1/2 2SI Mode: Disabled, Ch 2/3 2SI Mode: Disabled, Ch 3/4 2SI Mode: Disabled, Ch 4/5 2SI Mode: Disabled, 2SI Min Align Delay 1-4: Disabled, 2SI Min Align Delay 5-8: Disabled
	WriteRegister (kRegRP188InOut3DBB, 0x020000FF);  // Reg 268  // RP188: No RP-188 received, Bypass: Disabled, Filter: 02, DBB: 00 FF
	WriteRegister (kRegRP188InOut3Bits0_31, 0x00000000);  // Reg 269
	WriteRegister (kRegRP188InOut3Bits32_63, 0x00000000);  // Reg 270
	WriteRegister (kRegSDIOut3VPIDA, 0x00000000);  // Reg 271
	WriteRegister (kRegSDIOut3VPIDB, 0x00000000);  // Reg 272
	WriteRegister (kRegRP188InOut4DBB, 0x020000FF);  // Reg 273  // RP188: No RP-188 received, Bypass: Disabled, Filter: 02, DBB: 00 FF
	WriteRegister (kRegRP188InOut4Bits0_31, 0x00000000);  // Reg 274
	WriteRegister (kRegRP188InOut4Bits32_63, 0x00000000);  // Reg 275
	WriteRegister (kRegSDIOut4VPIDA, 0x00000000);  // Reg 276
	WriteRegister (kRegSDIOut4VPIDB, 0x00000000);  // Reg 277
	WriteRegister (kRegAud3Control, 0x80000300);  // Reg 278  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut3: Enabled, Audio Embedder SDIOut4: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	WriteRegister (kRegAud4Control, 0x80000300);  // Reg 279  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	WriteRegister (kRegAud3SourceSelect, 0x00000000);  // Reg 280  // Audio Source: AES Input, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	WriteRegister (kRegAud4SourceSelect, 0x00000000);  // Reg 281  // Audio Source: AES Input, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	WriteRegister (kRegAudDetect2, 0x00000000);  // Reg 282  // Group 0 CH 1-2: Absent, Group 0 CH 3-4: Absent, Group 1 CH 1-2: Absent, Group 1 CH 3-4: Absent, Group 2 CH 1-2: Absent, Group 2 CH 3-4: Absent, Group 3 CH 1-2: Absent, Group 3 CH 3-4: Absent
	WriteRegister (kRegAud3OutputLastAddr, 0x00000000);  // Reg 283
	WriteRegister (kRegAud3InputLastAddr, 0x00000000);  // Reg 284
	WriteRegister (kRegAud4OutputLastAddr, 0x00000000);  // Reg 285
	WriteRegister (kRegAud4InputLastAddr, 0x00000000);  // Reg 286
	WriteRegister (kRegSDIInput3GStatus2, 0x00000100);  // Reg 287
	WriteRegister (kRegInputStatus2, 0x00008000);  // Reg 288  // Input 3 Scan Mode: Interlaced, Input 3 Frame Rate: Unknown, Input 3 Geometry: Unknown, Input 4 Scan Mode: Progressive, Input 4 Frame Rate: Unknown, Input 4 Geometry: Unknown
	WriteRegister (kRegCh3PCIAccessFrame, 0x00000000);  // Reg 289
	WriteRegister (kRegCh4PCIAccessFrame, 0x00000000);  // Reg 290
	WriteRegister (kRegCS3Coefficients1_2, 0x00000000);  // Reg 291  // Video Key Sync Status: OK, Make Alpha From Key Input: Disabled, Matrix Select: Rec709, Use Custom Coeffs: N, Coefficient1: 0x0000, Coefficient2: 0x0000
	WriteRegister (kRegCS3Coefficients3_4, 0x00000000);  // Reg 292  // RGB Range: Full (0x000-0x3FF), Coefficient3: 0x0000, Coefficient4: 0x0000
	WriteRegister (kRegCS3Coefficients5_6, 0x00000000);  // Reg 293  // Coefficient5: 0x0000, Coefficient6: 0x0000
	WriteRegister (kRegCS3Coefficients7_8, 0x00000000);  // Reg 294  // Coefficient7: 0x0000, Coefficient8: 0x0000
	WriteRegister (kRegCS3Coefficients9_10, 0x00000000);  // Reg 295  // Coefficient9: 0x0000, Coefficient10: 0x0000
	WriteRegister (kRegCS4Coefficients1_2, 0x00000000);  // Reg 296  // Video Key Sync Status: OK, Make Alpha From Key Input: Disabled, Matrix Select: Rec709, Use Custom Coeffs: N, Coefficient1: 0x0000, Coefficient2: 0x0000
	WriteRegister (kRegCS4Coefficients3_4, 0x00000000);  // Reg 297  // RGB Range: Full (0x000-0x3FF), Coefficient3: 0x0000, Coefficient4: 0x0000
	WriteRegister (kRegCS4Coefficients5_6, 0x00000000);  // Reg 298  // Coefficient5: 0x0000, Coefficient6: 0x0000
	WriteRegister (kRegCS4Coefficients7_8, 0x00000000);  // Reg 299  // Coefficient7: 0x0000, Coefficient8: 0x0000
	WriteRegister (kRegCS4Coefficients9_10, 0x00000000);  // Reg 300  // Coefficient9: 0x0000, Coefficient10: 0x0000
	WriteRegister (kRegXptSelectGroup17, 0x00000000);  // Reg 301  // NTV2_XptCSC3VidInput <== NTV2_XptBlack, NTV2_XptCSC3KeyInput <== NTV2_XptBlack, NTV2_XptCSC4VidInput <== NTV2_XptBlack, NTV2_XptCSC4KeyInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup15, 0x00000000);  // Reg 302  // NTV2_XptDualLinkIn3Input <== NTV2_XptBlack, NTV2_XptDualLinkIn3DSInput <== NTV2_XptBlack, NTV2_XptDualLinkIn4Input <== NTV2_XptBlack, NTV2_XptDualLinkIn4DSInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup16, 0x00000000);  // Reg 303  // NTV2_XptDualLinkOut3Input <== NTV2_XptBlack, NTV2_XptDualLinkOut4Input <== NTV2_XptBlack, NTV2_XptDualLinkOut5Input <== NTV2_XptBlack, 
	WriteRegister (kRegAud3Delay, 0x00000000);  // Reg 304
	WriteRegister (kRegAud4Delay, 0x00000000);  // Reg 305
	WriteRegister (kRegSDIIn3VPIDA, 0x00000000);  // Reg 306
	WriteRegister (kRegSDIIn3VPIDB, 0x00000000);  // Reg 307
	WriteRegister (kRegSDIIn4VPIDA, 0x00000000);  // Reg 308
	WriteRegister (kRegSDIIn4VPIDB, 0x00000000);  // Reg 309
	WriteRegister (kRegSDIWatchdogControlStatus, 0x00000000);  // Reg 310  // (SDI bypass relays not supported)
	WriteRegister (kRegSDIWatchdogTimeout, 0x00000000);  // Reg 311  // (SDI bypass relays not supported)
	WriteRegister (kRegSDIWatchdogKick1, 0x00000000);  // Reg 312  // (SDI bypass relays not supported)
	WriteRegister (kRegSDIWatchdogKick2, 0x00000000);  // Reg 313  // (SDI bypass relays not supported)
	WriteRegister (kRegReserved314, 0x00000000);  // Reg 314
	WriteRegister (kRegReserved315, 0x00000000);  // Reg 315
	WriteRegister (kRegLTC3EmbeddedBits0_31, 0x00000000);  // Reg 316
	WriteRegister (kRegLTC3EmbeddedBits32_63, 0x00000000);  // Reg 317
	WriteRegister (kRegLTC4EmbeddedBits0_31, 0x00000000);  // Reg 318
	WriteRegister (kRegLTC4EmbeddedBits32_63, 0x00000000);  // Reg 319
	WriteRegister (kRegReserved320, 0x00000000);  // Reg 320
	WriteRegister (kRegReserved321, 0x00000000);  // Reg 321
	WriteRegister (kRegReserved322, 0x00000000);  // Reg 322
	WriteRegister (kRegReserved323, 0x00000000);  // Reg 323
	WriteRegister (kRegReserved324, 0x00000000);  // Reg 324
	WriteRegister (kRegReserved325, 0x00000000);  // Reg 325
	WriteRegister (kRegReserved326, 0x00000000);  // Reg 326
	WriteRegister (kRegReserved327, 0x00000000);  // Reg 327
	WriteRegister (kRegReserved328, 0x00000000);  // Reg 328
	WriteRegister (kRegHDMITimeCode, 0x00000000);  // Reg 329
	WriteRegister (kRegHDMIHDRGreenPrimary, 0x00000000);  // Reg 330
	WriteRegister (kRegHDMIHDRBluePrimary, 0x00000000);  // Reg 331
	WriteRegister (kRegHDMIHDRRedPrimary, 0x00000000);  // Reg 332
	WriteRegister (kRegHDMIHDRWhitePoint, 0x00000000);  // Reg 333
	WriteRegister (kRegHDMIHDRMasteringLuminence, 0x00000000);  // Reg 334
	WriteRegister (kRegHDMIHDRLightLevel, 0x00000000);  // Reg 335
	WriteRegister (kRegHDMIHDRControl, 0x00000000);  // Reg 336
	WriteRegister (kRegSDIOut5Control, 0x01000084);  // Reg 337  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x40, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 1, DS 2 audio source: Subsystem 1
	WriteRegister (kRegSDIOut5VPIDA, 0x00000000);  // Reg 338
	WriteRegister (kRegSDIOut5VPIDB, 0x00000000);  // Reg 339
	WriteRegister (kRegRP188InOut5Bits0_31, 0x6B030607);  // Reg 340
	WriteRegister (kRegRP188InOut5Bits32_63, 0x00000004);  // Reg 341
	WriteRegister (kRegRP188InOut5DBB, 0x020D0002);  // Reg 342  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 00 02
	WriteRegister (kRegReserved343, 0x00000000);  // Reg 343
	WriteRegister (kRegLTC5EmbeddedBits0_31, 0x63030608);  // Reg 344
	WriteRegister (kRegLTC5EmbeddedBits32_63, 0x00000004);  // Reg 345
	WriteRegister (kRegDL5Control, 0x00000000);  // Reg 346
	WriteRegister (kRegCS5Coefficients1_2, 0x00000000);  // Reg 347  // Video Key Sync Status: OK, Make Alpha From Key Input: Disabled, Matrix Select: Rec709, Use Custom Coeffs: N, Coefficient1: 0x0000, Coefficient2: 0x0000
	WriteRegister (kRegCS5Coefficients3_4, 0x00000000);  // Reg 348  // RGB Range: Full (0x000-0x3FF), Coefficient3: 0x0000, Coefficient4: 0x0000
	WriteRegister (kRegCS5Coefficients5_6, 0x00000000);  // Reg 349  // Coefficient5: 0x0000, Coefficient6: 0x0000
	WriteRegister (kRegCS5Coefficients7_8, 0x00000000);  // Reg 350  // Coefficient7: 0x0000, Coefficient8: 0x0000
	WriteRegister (kRegCS5Coefficients9_10, 0x00000000);  // Reg 351  // Coefficient9: 0x0000, Coefficient10: 0x0000
	WriteRegister (kRegXptSelectGroup18, 0x00000000);  // Reg 352  // NTV2_XptCSC5VidInput <== NTV2_XptBlack, NTV2_XptCSC5KeyInput <== NTV2_XptBlack, , 
	WriteRegister (kRegReserved353, 0x00000CA3);  // Reg 353
	WriteRegister (kRegDC1, 0x00000000);  // Reg 354
	WriteRegister (kRegDC2, 0x00000000);  // Reg 355
	WriteRegister (kRegXptSelectGroup19, 0x00000000);  // Reg 356  // NTV2_Xpt4KDCQ1Input <== NTV2_XptBlack, NTV2_Xpt4KDCQ2Input <== NTV2_XptBlack, NTV2_Xpt4KDCQ3Input <== NTV2_XptBlack, NTV2_Xpt4KDCQ4Input <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup20, 0x00000000);  // Reg 357  // , NTV2_XptHDMIOutQ2Input <== NTV2_XptBlack, NTV2_XptHDMIOutQ3Input <== NTV2_XptBlack, NTV2_XptHDMIOutQ4Input <== NTV2_XptBlack
	WriteRegister (kRegRasterizerControl, 0x00000000);  // Reg 358
	WriteRegister (kRegHDMIV2I2C1Control, 0x00000000);  // Reg 359
	WriteRegister (kRegHDMIV2I2C1Data, 0x00000000);  // Reg 360
	WriteRegister (kRegHDMIV2VideoSetup, 0x00000000);  // Reg 361
	WriteRegister (kRegHDMIV2HSyncDurationAndBackPorch, 0x00000000);  // Reg 362
	WriteRegister (kRegHDMIV2HActive, 0x00000000);  // Reg 363
	WriteRegister (kRegHDMIV2VSyncDurationAndBackPorchField1, 0x00000000);  // Reg 364
	WriteRegister (kRegHDMIV2VSyncDurationAndBackPorchField2, 0x00000000);  // Reg 365
	WriteRegister (kRegHDMIV2VActiveField1, 0x00000000);  // Reg 366
	WriteRegister (kRegHDMIV2VActiveField2, 0x00000000);  // Reg 367
	WriteRegister (kRegHDMIV2VideoStatus, 0x00000000);  // Reg 368
	WriteRegister (kRegHDMIV2HorizontalMeasurements, 0x00000000);  // Reg 369
	WriteRegister (kRegHDMIV2HBlankingMeasurements, 0x00000000);  // Reg 370
	WriteRegister (kRegHDMIV2HBlankingMeasurements1, 0x00000000);  // Reg 371
	WriteRegister (kRegHDMIV2VerticalMeasurementsField0, 0x00000000);  // Reg 372
	WriteRegister (kRegHDMIV2VerticalMeasurementsField1, 0x00000000);  // Reg 373
	WriteRegister (kRegHDMIV2i2c2Control, 0x00000000);  // Reg 374
	WriteRegister (kRegHDMIV2i2c2Data, 0x00000000);  // Reg 375
	WriteRegister (kRegLUTV2Control, 0x000F1F00);  // Reg 376  // (Register data relevant for V2 LUT, this device has V0LUT)
	WriteRegister (kRegGlobalControlCh2, 0x00000202);  // Reg 377  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	WriteRegister (kRegGlobalControlCh3, 0x00000202);  // Reg 378  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	WriteRegister (kRegGlobalControlCh4, 0x00000202);  // Reg 379  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	WriteRegister (kRegGlobalControlCh5, 0x00000202);  // Reg 380  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	WriteRegister (kRegGlobalControlCh6, 0x00000202);  // Reg 381  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	WriteRegister (kRegGlobalControlCh7, 0x00000202);  // Reg 382  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	WriteRegister (kRegGlobalControlCh8, 0x00000202);  // Reg 383  // Frame Rate: NTV2_FRAMERATE_5994, Frame Geometry: NTV2_FG_1920x1080, Standard: NTV2_STANDARD_1080p
	WriteRegister (kRegCh5Control, 0x00000001);  // Reg 384  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	WriteRegister (kRegCh5OutputFrame, 0x00000000);  // Reg 385
	WriteRegister (kRegCh5InputFrame, 0x00000006);  // Reg 386
	WriteRegister (kRegCh5PCIAccessFrame, 0x00000000);  // Reg 387
	WriteRegister (kRegCh6Control, 0x00000001);  // Reg 388  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	WriteRegister (kRegCh6OutputFrame, 0x00000008);  // Reg 389
	WriteRegister (kRegCh6InputFrame, 0x00000000);  // Reg 390
	WriteRegister (kRegCh6PCIAccessFrame, 0x00000000);  // Reg 391
	WriteRegister (kRegCh7Control, 0x00000001);  // Reg 392  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	WriteRegister (kRegCh7OutputFrame, 0x00000000);  // Reg 393
	WriteRegister (kRegCh7InputFrame, 0x00000000);  // Reg 394
	WriteRegister (kRegCh7PCIAccessFrame, 0x00000000);  // Reg 395
	WriteRegister (kRegCh8Control, 0x00000001);  // Reg 396  // Mode: Capture, Format: NTV2_FBF_10BIT_YCBCR, Channel: Enabled, Viper Squeeze: Normal, Flip Vertical: Normal, DRT Display: Off, Frame Buffer Mode: Frame, Dither: No dithering, Frame Size: 2 MB, RGB Range: Black = 0, VANC Data Shift: Normal 8 bit conversion
	WriteRegister (kRegCh8OutputFrame, 0x00000000);  // Reg 397
	WriteRegister (kRegCh8InputFrame, 0x00000000);  // Reg 398
	WriteRegister (kRegCh8PCIAccessFrame, 0x00000000);  // Reg 399
	WriteRegister (kRegXptSelectGroup21, 0x00000000);  // Reg 400  // NTV2_XptFrameBuffer5Input <== NTV2_XptBlack, NTV2_XptFrameBuffer6Input <== NTV2_XptBlack, NTV2_XptFrameBuffer7Input <== NTV2_XptBlack, NTV2_XptFrameBuffer8Input <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup22, 0x00000000);  // Reg 401  // NTV2_XptSDIOut6Input <== NTV2_XptBlack, NTV2_XptSDIOut6InputDS2 <== NTV2_XptBlack, NTV2_XptSDIOut7Input <== NTV2_XptBlack, NTV2_XptSDIOut7InputDS2 <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup30, 0x00000000);  // Reg 402  // NTV2_XptSDIOut8Input <== NTV2_XptBlack, NTV2_XptSDIOut8InputDS2 <== NTV2_XptBlack, NTV2_XptCSC6VidInput <== NTV2_XptBlack, NTV2_XptCSC6KeyInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup23, 0x00000000);  // Reg 403  // NTV2_XptCSC7VidInput <== NTV2_XptBlack, NTV2_XptCSC7KeyInput <== NTV2_XptBlack, NTV2_XptCSC8VidInput <== NTV2_XptBlack, NTV2_XptCSC8KeyInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup24, 0x00000000);  // Reg 404  // NTV2_XptLUT6Input <== NTV2_XptBlack, NTV2_XptLUT7Input <== NTV2_XptBlack, NTV2_XptLUT8Input <== NTV2_XptBlack, 
	WriteRegister (kRegXptSelectGroup25, 0x00000000);  // Reg 405  // NTV2_XptDualLinkIn5Input <== NTV2_XptBlack, NTV2_XptDualLinkIn5DSInput <== NTV2_XptBlack, NTV2_XptDualLinkIn6Input <== NTV2_XptBlack, NTV2_XptDualLinkIn6DSInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup26, 0x00000000);  // Reg 406  // NTV2_XptDualLinkIn7Input <== NTV2_XptBlack, NTV2_XptDualLinkIn7DSInput <== NTV2_XptBlack, NTV2_XptDualLinkIn8Input <== NTV2_XptBlack, NTV2_XptDualLinkIn8DSInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup27, 0x00000000);  // Reg 407  // NTV2_XptDualLinkOut6Input <== NTV2_XptBlack, NTV2_XptDualLinkOut7Input <== NTV2_XptBlack, NTV2_XptDualLinkOut8Input <== NTV2_XptBlack, 
	WriteRegister (kRegXptSelectGroup28, 0x00000000);  // Reg 408  // NTV2_XptMixer3FGVidInput <== NTV2_XptBlack, NTV2_XptMixer3FGKeyInput <== NTV2_XptBlack, NTV2_XptMixer3BGVidInput <== NTV2_XptBlack, NTV2_XptMixer3BGKeyInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup29, 0x00000000);  // Reg 409  // NTV2_XptMixer4FGVidInput <== NTV2_XptBlack, NTV2_XptMixer4FGKeyInput <== NTV2_XptBlack, NTV2_XptMixer4BGVidInput <== NTV2_XptBlack, NTV2_XptMixer4BGKeyInput <== NTV2_XptBlack
	WriteRegister (kRegSDIIn5VPIDA, 0x0180CA89);  // Reg 410  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	WriteRegister (kRegSDIIn5VPIDB, 0x4180CA89);  // Reg 411  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	WriteRegister (kRegSDIIn6VPIDA, 0x0180CA89);  // Reg 412  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	WriteRegister (kRegSDIIn6VPIDB, 0x4180CA89);  // Reg 413  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	WriteRegister (kRegSDIOut6VPIDA, 0x00000000);  // Reg 414
	WriteRegister (kRegSDIOut6VPIDB, 0x00000000);  // Reg 415
	WriteRegister (kRegRP188InOut6Bits0_31, 0x38000400);  // Reg 416
	WriteRegister (kRegRP188InOut6Bits32_63, 0x00000000);  // Reg 417
	WriteRegister (kRegRP188InOut6DBB, 0x020D0200);  // Reg 418  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 02 00
	WriteRegister (kRegLTC6EmbeddedBits0_31, 0x63030608);  // Reg 419
	WriteRegister (kRegLTC6EmbeddedBits32_63, 0x00000004);  // Reg 420
	WriteRegister (kRegSDIIn7VPIDA, 0x0180CA89);  // Reg 421  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	WriteRegister (kRegSDIIn7VPIDB, 0x4180CA89);  // Reg 422  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	WriteRegister (kRegSDIOut7VPIDA, 0x00000000);  // Reg 423
	WriteRegister (kRegSDIOut7VPIDB, 0x00000000);  // Reg 424
	WriteRegister (kRegRP188InOut7Bits0_31, 0x38000400);  // Reg 425
	WriteRegister (kRegRP188InOut7Bits32_63, 0x00000000);  // Reg 426
	WriteRegister (kRegRP188InOut7DBB, 0x020D00FF);  // Reg 427  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 00 FF
	WriteRegister (kRegLTC7EmbeddedBits0_31, 0x63030608);  // Reg 428
	WriteRegister (kRegLTC7EmbeddedBits32_63, 0x00000004);  // Reg 429
	WriteRegister (kRegSDIIn8VPIDA, 0x0180CA89);  // Reg 430  // Raw Value: 0x89CA8001, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 1, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	WriteRegister (kRegSDIIn8VPIDB, 0x4180CA89);  // Reg 431  // Raw Value: 0x89CA8041, Version: 1, Standard: 1080 3G Level A, Video Format: 1080p59.94a, Progressive Transport: Yes, Progressive Picture: Yes, Picture Rate: 59.94, Aspect Ratio: 16x9, Sampling: YCbCr 4:2:2, Channel: 2, Bit Depth: 10, 3Ga: Yes, Two Sample Interleave: No, Xfer Characteristics: SDR, Colorimetry: Rec709, Luminance: YCbCr
	WriteRegister (kRegSDIOut8VPIDA, 0x00000000);  // Reg 432
	WriteRegister (kRegSDIOut8VPIDB, 0x00000000);  // Reg 433
	WriteRegister (kRegRP188InOut8Bits0_31, 0x38000400);  // Reg 434
	WriteRegister (kRegRP188InOut8Bits32_63, 0x00000000);  // Reg 435
	WriteRegister (kRegRP188InOut8DBB, 0x020D0200);  // Reg 436  // RP188: Unselected RP-188 received +LTC +VITC, Bypass: Disabled, Filter: 02, DBB: 02 00
	WriteRegister (kRegLTC8EmbeddedBits0_31, 0x63030608);  // Reg 437
	WriteRegister (kRegLTC8EmbeddedBits32_63, 0x00000004);  // Reg 438
	WriteRegister (kRegXptSelectGroup31, 0x00000000);  // Reg 439
	WriteRegister (kRegAud5Control, 0x80100300);  // Reg 440  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut5: Enabled, Audio Embedder SDIOut6: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 16-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 4 MB
	WriteRegister (kRegAud5SourceSelect, 0x00400001);  // Reg 441  // Audio Source: Embedded Groups 1 and 2, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Video Input, 3G audio source: Data stream 1
	WriteRegister (kRegAud5OutputLastAddr, 0x00000000);  // Reg 442
	WriteRegister (kRegAud5InputLastAddr, 0x00000000);  // Reg 443
	WriteRegister (kRegAud6Control, 0x00000300);  // Reg 444  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	WriteRegister (kRegAud6SourceSelect, 0x00000000);  // Reg 445  // Audio Source: AES Input, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	WriteRegister (kRegAud6OutputLastAddr, 0x00000000);  // Reg 446
	WriteRegister (kRegAud6InputLastAddr, 0x00000000);  // Reg 447
	WriteRegister (kRegAud7Control, 0x00000300);  // Reg 448  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, Audio Embedder SDIOut7: Enabled, Audio Embedder SDIOut8: Enabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	WriteRegister (kRegAud7SourceSelect, 0x00000000);  // Reg 449  // Audio Source: AES Input, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	WriteRegister (kRegAud7OutputLastAddr, 0x00000000);  // Reg 450
	WriteRegister (kRegAud7InputLastAddr, 0x00000000);  // Reg 451
	WriteRegister (kRegAud8Control, 0x00000300);  // Reg 452  // Audio Capture: Disabled, Audio Loopback: Disabled, Audio Input: Disabled, Audio Output: Disabled, A/V Sync Mode: Disabled, AES Rate Converter: Enabled, Audio Buffer Format: 6-Channel , 48kHz, 48kHz Support, No Embedded Support, 6-Channel Support, K-box, Monitor: Ch 1/2, K-Box Input: BNC, K-Box: Absent, Cable: BNC, Audio Buffer Size: 1 MB
	WriteRegister (kRegAud8SourceSelect, 0x00000000);  // Reg 453  // Audio Source: AES Input, Embedded Source Select: Video Input 1, AES Sync Mode bit (fib): Disabled, PCM disabled: N, Erase head enable: N, Embedded Clock Select: Board Reference, 3G audio source: Data stream 1
	WriteRegister (kRegAud8OutputLastAddr, 0x00000000);  // Reg 454
	WriteRegister (kRegAud8InputLastAddr, 0x00000000);  // Reg 455
	WriteRegister (kRegAudioDetect5678, 0xFFFFFFFF);  // Reg 456
	WriteRegister (kRegSDI5678Input3GStatus, 0x31313131);  // Reg 457
	WriteRegister (kRegInput56Status, 0x0000C2C2);  // Reg 458  // Input 5 Scan Mode: Progressive, Input 5 Frame Rate: 59.94, Input 5 Geometry: 1125, Input 6 Scan Mode: Progressive, Input 6 Frame Rate: 59.94, Input 6 Geometry: 1125
	WriteRegister (kRegInput78Status, 0x0000C2C2);  // Reg 459  // Input 7 Scan Mode: Progressive, Input 7 Frame Rate: 59.94, Input 7 Geometry: 1125, Input 8 Scan Mode: Progressive, Input 8 Frame Rate: 59.94, Input 8 Geometry: 1125
	WriteRegister (kRegCS6Coefficients1_2, 0x00000000);  // Reg 460  // Video Key Sync Status: OK, Make Alpha From Key Input: Disabled, Matrix Select: Rec709, Use Custom Coeffs: N, Coefficient1: 0x0000, Coefficient2: 0x0000
	WriteRegister (kRegCS6Coefficients3_4, 0x00000000);  // Reg 461  // RGB Range: Full (0x000-0x3FF), Coefficient3: 0x0000, Coefficient4: 0x0000
	WriteRegister (kRegCS6Coefficients5_6, 0x00000000);  // Reg 462  // Coefficient5: 0x0000, Coefficient6: 0x0000
	WriteRegister (kRegCS6Coefficients7_8, 0x00000000);  // Reg 463  // Coefficient7: 0x0000, Coefficient8: 0x0000
	WriteRegister (kRegCS6Coefficients9_10, 0x00000000);  // Reg 464  // Coefficient9: 0x0000, Coefficient10: 0x0000
	WriteRegister (kRegCS7Coefficients1_2, 0x00000000);  // Reg 465  // Video Key Sync Status: OK, Make Alpha From Key Input: Disabled, Matrix Select: Rec709, Use Custom Coeffs: N, Coefficient1: 0x0000, Coefficient2: 0x0000
	WriteRegister (kRegCS7Coefficients3_4, 0x00000000);  // Reg 466  // RGB Range: Full (0x000-0x3FF), Coefficient3: 0x0000, Coefficient4: 0x0000
	WriteRegister (kRegCS7Coefficients5_6, 0x00000000);  // Reg 467  // Coefficient5: 0x0000, Coefficient6: 0x0000
	WriteRegister (kRegCS7Coefficients7_8, 0x00000000);  // Reg 468  // Coefficient7: 0x0000, Coefficient8: 0x0000
	WriteRegister (kRegCS7Coefficients9_10, 0x00000000);  // Reg 469  // Coefficient9: 0x0000, Coefficient10: 0x0000
	WriteRegister (kRegCS8Coefficients1_2, 0x00000000);  // Reg 470  // Video Key Sync Status: OK, Make Alpha From Key Input: Disabled, Matrix Select: Rec709, Use Custom Coeffs: N, Coefficient1: 0x0000, Coefficient2: 0x0000
	WriteRegister (kRegCS8Coefficients3_4, 0x00000000);  // Reg 471  // RGB Range: Full (0x000-0x3FF), Coefficient3: 0x0000, Coefficient4: 0x0000
	WriteRegister (kRegCS8Coefficients5_6, 0x00000000);  // Reg 472  // Coefficient5: 0x0000, Coefficient6: 0x0000
	WriteRegister (kRegCS8Coefficients7_8, 0x00000000);  // Reg 473  // Coefficient7: 0x0000, Coefficient8: 0x0000
	WriteRegister (kRegCS8Coefficients9_10, 0x00000000);  // Reg 474  // Coefficient9: 0x0000, Coefficient10: 0x0000
	WriteRegister (kRegSDIOut6Control, 0x01040004);  // Reg 475  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	WriteRegister (kRegSDIOut7Control, 0x01040004);  // Reg 476  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	WriteRegister (kRegSDIOut8Control, 0x01040004);  // Reg 477  // Video Standard : 1080p, 2Kx1080 mode: 1920x1080, HBlank RGB Range: Black=0x04, 12G enable: N, 6G enalbe: N, 3G enable: Y, 3G mode: a, VPID insert enable: N, VPID overwrite enable: N, DS 1 audio source: Subsystem 5, DS 2 audio source: Subsystem 1
	WriteRegister (kRegOutputTimingControlch2, 0x08001000);  // Reg 478
	WriteRegister (kRegOutputTimingControlch3, 0x08001000);  // Reg 479
	WriteRegister (kRegOutputTimingControlch4, 0x08001000);  // Reg 480
	WriteRegister (kRegOutputTimingControlch5, 0x08001000);  // Reg 481
	WriteRegister (kRegOutputTimingControlch6, 0x08001000);  // Reg 482
	WriteRegister (kRegOutputTimingControlch7, 0x08001000);  // Reg 483
	WriteRegister (kRegOutputTimingControlch8, 0x08001000);  // Reg 484
	WriteRegister (kRegVidProc3Control, 0x00200000);  // Reg 485  // Mode: Full Raster, FG Control: Unshaped, BG Control: Full Raster, VANC Pass-Thru: Foreground, FG Matte: Disabled, BG Matte: Disabled, Input Sync: in sync, Limiting: Legal SDI, Split Video Std: 1080i
	WriteRegister (kRegMixer3Coefficient, 0x00000000);  // Reg 486
	WriteRegister (kRegFlatMatte3Value, 0x18080240);  // Reg 487  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	WriteRegister (kRegVidProc4Control, 0x00000000);  // Reg 488  // Mode: Full Raster, FG Control: Full Raster, BG Control: Full Raster, VANC Pass-Thru: Foreground, FG Matte: Disabled, BG Matte: Disabled, Input Sync: in sync, Limiting: Legal SDI, Split Video Std: 1080i
	WriteRegister (kRegMixer4Coefficient, 0x00000000);  // Reg 489
	WriteRegister (kRegFlatMatte4Value, 0x18080240);  // Reg 490  // Flat Matte Cb: 240, Flat Matte Y: 1C0, Flat Matte Cr: 180
	WriteRegister (kRegTRSErrorStatus, 0x00F00000);  // Reg 491
	WriteRegister (kRegAud5Delay, 0x00000000);  // Reg 492
	WriteRegister (kRegAud6Delay, 0x00000000);  // Reg 493
	WriteRegister (kRegAud7Delay, 0x00000000);  // Reg 494
	WriteRegister (kRegAud8Delay, 0x00000000);  // Reg 495
	WriteRegister (kRegPCMControl4321, 0x00000000);  // Reg 496  // Audio System 1: normal, Audio System 2: normal, Audio System 3: normal, Audio System 4: normal
	WriteRegister (kRegPCMControl8765, 0x00000000);  // Reg 497  // Audio System 5: normal, Audio System 6: normal, Audio System 7: normal, Audio System 8: normal
	WriteRegister (kRegCh1Control2MFrame, 0x00000000);  // Reg 498
	WriteRegister (kRegCh2Control2MFrame, 0x00000000);  // Reg 499
	WriteRegister (kRegCh3Control2MFrame, 0x00000000);  // Reg 500
	WriteRegister (kRegCh4Control2MFrame, 0x00000000);  // Reg 501
	WriteRegister (kRegCh5Control2MFrame, 0x00000000);  // Reg 502
	WriteRegister (kRegCh6Control2MFrame, 0x00000000);  // Reg 503
	WriteRegister (kRegCh7Control2MFrame, 0x00000000);  // Reg 504
	WriteRegister (kRegCh8Control2MFrame, 0x00000000);  // Reg 505
	WriteRegister (kRegXptSelectGroup32, 0x00000000);  // Reg 506  // NTV2_Xpt425Mux1AInput <== NTV2_XptBlack, NTV2_Xpt425Mux1BInput <== NTV2_XptBlack, NTV2_Xpt425Mux2AInput <== NTV2_XptBlack, NTV2_Xpt425Mux2BInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup33, 0x00000000);  // Reg 507  // NTV2_Xpt425Mux3AInput <== NTV2_XptBlack, NTV2_Xpt425Mux3BInput <== NTV2_XptBlack, NTV2_Xpt425Mux4AInput <== NTV2_XptBlack, NTV2_Xpt425Mux4BInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup34, 0x00000000);  // Reg 508  // NTV2_XptFrameBuffer1BInput <== NTV2_XptBlack, NTV2_XptFrameBuffer2BInput <== NTV2_XptBlack, NTV2_XptFrameBuffer3BInput <== NTV2_XptBlack, NTV2_XptFrameBuffer4BInput <== NTV2_XptBlack
	WriteRegister (kRegXptSelectGroup35, 0x00000000);  // Reg 509  // NTV2_XptFrameBuffer5BInput <== NTV2_XptBlack, NTV2_XptFrameBuffer6BInput <== NTV2_XptBlack, NTV2_XptFrameBuffer7BInput <== NTV2_XptBlack, NTV2_XptFrameBuffer8BInput <== NTV2_XptBlack
	WriteRegister (kRegReserved510, 0x00000000);  // Reg 510
	WriteRegister (kRegReserved511, 0x00000000);  // Reg 511
	WriteRegister (kRegRXSDI1Status, 0x00000000);  // Reg 2048
	WriteRegister (kRegRXSDI1CRCErrorCount, 0x00000000);  // Reg 2049
	WriteRegister (kRegRXSDI1FrameCountLow, 0x00D56B2B);  // Reg 2050
	WriteRegister (kRegRXSDI1FrameCountHigh, 0x00000000);  // Reg 2051
	WriteRegister (kRegRXSDI1FrameRefCountLow, 0x9C3966CF);  // Reg 2052
	WriteRegister (kRegRXSDI1FrameRefCountHigh, 0x00000003);  // Reg 2053
	WriteRegister (kRegRXSDI2Status, 0x00000000);  // Reg 2056
	WriteRegister (kRegRXSDI2CRCErrorCount, 0x00000000);  // Reg 2057
	WriteRegister (kRegRXSDI2FrameCountLow, 0x00A2E900);  // Reg 2058
	WriteRegister (kRegRXSDI2FrameCountHigh, 0x00000000);  // Reg 2059
	WriteRegister (kRegRXSDI2FrameRefCountLow, 0x1AB48C3B);  // Reg 2060
	WriteRegister (kRegRXSDI2FrameRefCountHigh, 0x00000000);  // Reg 2061
	WriteRegister (kRegRXSDI3Status, 0x00000000);  // Reg 2064
	WriteRegister (kRegRXSDI3CRCErrorCount, 0x00000000);  // Reg 2065
	WriteRegister (kRegRXSDI3FrameCountLow, 0x00F6602B);  // Reg 2066
	WriteRegister (kRegRXSDI3FrameCountHigh, 0x00000000);  // Reg 2067
	WriteRegister (kRegRXSDI3FrameRefCountLow, 0x1AF13D4F);  // Reg 2068
	WriteRegister (kRegRXSDI3FrameRefCountHigh, 0x00000000);  // Reg 2069
	WriteRegister (kRegRXSDI4Status, 0x00000000);  // Reg 2072
	WriteRegister (kRegRXSDI4CRCErrorCount, 0x00000000);  // Reg 2073
	WriteRegister (kRegRXSDI4FrameCountLow, 0x000480B4);  // Reg 2074
	WriteRegister (kRegRXSDI4FrameCountHigh, 0x00000000);  // Reg 2075
	WriteRegister (kRegRXSDI4FrameRefCountLow, 0x1AAD9B03);  // Reg 2076
	WriteRegister (kRegRXSDI4FrameRefCountHigh, 0x00000000);  // Reg 2077
	WriteRegister (kRegRXSDI5Status, 0x00310010);  // Reg 2080
	WriteRegister (kRegRXSDI5CRCErrorCount, 0x00000015);  // Reg 2081
	WriteRegister (kRegRXSDI5FrameCountLow, 0x00516EF4);  // Reg 2082
	WriteRegister (kRegRXSDI5FrameCountHigh, 0x00000000);  // Reg 2083
	WriteRegister (kRegRXSDI5FrameRefCountLow, 0xDABA55D6);  // Reg 2084
	WriteRegister (kRegRXSDI5FrameRefCountHigh, 0x000024C8);  // Reg 2085
	WriteRegister (kRegRXSDI6Status, 0x0031000C);  // Reg 2088
	WriteRegister (kRegRXSDI6CRCErrorCount, 0x0000CC70);  // Reg 2089
	WriteRegister (kRegRXSDI6FrameCountLow, 0x020AD1CE);  // Reg 2090
	WriteRegister (kRegRXSDI6FrameCountHigh, 0x00000000);  // Reg 2091
	WriteRegister (kRegRXSDI6FrameRefCountLow, 0xDABA55D7);  // Reg 2092
	WriteRegister (kRegRXSDI6FrameRefCountHigh, 0x000024C8);  // Reg 2093
	WriteRegister (kRegRXSDI7Status, 0x00310007);  // Reg 2096
	WriteRegister (kRegRXSDI7CRCErrorCount, 0x00003CB2);  // Reg 2097
	WriteRegister (kRegRXSDI7FrameCountLow, 0x00C00088);  // Reg 2098
	WriteRegister (kRegRXSDI7FrameCountHigh, 0x00000000);  // Reg 2099
	WriteRegister (kRegRXSDI7FrameRefCountLow, 0xDABA55D7);  // Reg 2100
	WriteRegister (kRegRXSDI7FrameRefCountHigh, 0x000024C8);  // Reg 2101
	WriteRegister (kRegRXSDI8Status, 0x00310007);  // Reg 2104
	WriteRegister (kRegRXSDI8CRCErrorCount, 0x000164D5);  // Reg 2105
	WriteRegister (kRegRXSDI8FrameCountLow, 0x00642D78);  // Reg 2106
	WriteRegister (kRegRXSDI8FrameCountHigh, 0x00000000);  // Reg 2107
	WriteRegister (kRegRXSDI8FrameRefCountLow, 0xDABA55D8);  // Reg 2108
	WriteRegister (kRegRXSDI8FrameRefCountHigh, 0x000024C8);  // Reg 2109
	WriteRegister (kRegRXSDIFreeRunningClockLow, 0x18188497);  // Reg 2112
	WriteRegister (kRegRXSDIFreeRunningClockHigh, 0x00002F12);  // Reg 2113
	WriteRegister (4096, 0x10011111);  // Extract 1 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	WriteRegister (4097, 0x30000000);  // Extract 1 F1 Start Address
	WriteRegister (4098, 0x301FFFFF);  // Extract 1 F1 End Address
	WriteRegister (4099, 0x30200000);  // Extract 1 F2 Start Address
	WriteRegister (4100, 0x303FFFFF);  // Extract 1 F2 End Address
	WriteRegister (4101, 0x00640064);  // Extract 1 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	WriteRegister (4102, 0x00000000);  // Extract 1 Memory Total  // Total bytes: 0, Overrun: N
	WriteRegister (4103, 0x00000000);  // Extract 1 F1 Memory Usage  // Total F1 bytes: 0, Overrun: N
	WriteRegister (4104, 0x00000000);  // Extract 1 F2 Memory Usage  // Total F2 bytes: 0, Overrun: N
	WriteRegister (4105, 0x00000462);  // Extract 1 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	WriteRegister (4106, 0x00000465);  // Extract 1 Lines Per Frame
	WriteRegister (4107, 0x00000000);  // Extract 1 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4108, 0xE4E5E6E7);  // Extract 1 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	WriteRegister (4109, 0xE0E1E2E3);  // Extract 1 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	WriteRegister (4110, 0xA4A5A6A7);  // Extract 1 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	WriteRegister (4111, 0xA0A1A2A3);  // Extract 1 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	WriteRegister (4112, 0xE7E7E7E7);  // Extract 1 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	WriteRegister (4113, 0x010A0004);  // Extract 1 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	WriteRegister (4114, 0x00000000);  // Extract 1 Analog F1 Y Filter  // Each 1 bit specifies capturing F1 Y line as analog, else digital
	WriteRegister (4115, 0x00000000);  // Extract 1 Analog F2 Y Filter  // Each 1 bit specifies capturing F2 Y line as analog, else digital
	WriteRegister (4116, 0x00000000);  // Extract 1 Analog F1 C Filter  // Each 1 bit specifies capturing F1 C line as analog, else digital
	WriteRegister (4117, 0x00000000);  // Extract 1 Analog F2 C Filter  // Each 1 bit specifies capturing F2 C line as analog, else digital
	WriteRegister (4123, 0x00000780);  // Reg 0x101B
	WriteRegister (4160, 0x10011111);  // Extract 2 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	WriteRegister (4161, 0x30800000);  // Extract 2 F1 Start Address
	WriteRegister (4162, 0x309FFFFF);  // Extract 2 F1 End Address
	WriteRegister (4163, 0x30A00000);  // Extract 2 F2 Start Address
	WriteRegister (4164, 0x30BFFFFF);  // Extract 2 F2 End Address
	WriteRegister (4165, 0x00640064);  // Extract 2 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	WriteRegister (4166, 0x00000000);  // Extract 2 Memory Total  // Total bytes: 0, Overrun: N
	WriteRegister (4167, 0x00000000);  // Extract 2 F1 Memory Usage  // Total F1 bytes: 0, Overrun: N
	WriteRegister (4168, 0x00000000);  // Extract 2 F2 Memory Usage  // Total F2 bytes: 0, Overrun: N
	WriteRegister (4169, 0x00000462);  // Extract 2 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	WriteRegister (4170, 0x00000465);  // Extract 2 Lines Per Frame
	WriteRegister (4171, 0x00000000);  // Extract 2 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4172, 0xE4E5E6E7);  // Extract 2 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	WriteRegister (4173, 0xE0E1E2E3);  // Extract 2 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	WriteRegister (4174, 0xA4A5A6A7);  // Extract 2 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	WriteRegister (4175, 0xA0A1A2A3);  // Extract 2 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	WriteRegister (4176, 0xE7E7E7E7);  // Extract 2 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	WriteRegister (4177, 0x010A0004);  // Extract 2 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	WriteRegister (4178, 0x00000000);  // Extract 2 Analog F1 Y Filter  // Each 1 bit specifies capturing F1 Y line as analog, else digital
	WriteRegister (4179, 0x00000000);  // Extract 2 Analog F2 Y Filter  // Each 1 bit specifies capturing F2 Y line as analog, else digital
	WriteRegister (4180, 0x00000000);  // Extract 2 Analog F1 C Filter  // Each 1 bit specifies capturing F1 C line as analog, else digital
	WriteRegister (4181, 0x00000000);  // Extract 2 Analog F2 C Filter  // Each 1 bit specifies capturing F2 C line as analog, else digital
	WriteRegister (4187, 0x00000780);  // Reg 0x105B
	WriteRegister (4224, 0x10011111);  // Extract 3 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	WriteRegister (4225, 0x31000000);  // Extract 3 F1 Start Address
	WriteRegister (4226, 0x311FFFFF);  // Extract 3 F1 End Address
	WriteRegister (4227, 0x31200000);  // Extract 3 F2 Start Address
	WriteRegister (4228, 0x313FFFFF);  // Extract 3 F2 End Address
	WriteRegister (4229, 0x00640064);  // Extract 3 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	WriteRegister (4230, 0x00000000);  // Extract 3 Memory Total  // Total bytes: 0, Overrun: N
	WriteRegister (4231, 0x00000000);  // Extract 3 F1 Memory Usage  // Total F1 bytes: 0, Overrun: N
	WriteRegister (4232, 0x00000000);  // Extract 3 F2 Memory Usage  // Total F2 bytes: 0, Overrun: N
	WriteRegister (4233, 0x00000462);  // Extract 3 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	WriteRegister (4234, 0x00000465);  // Extract 3 Lines Per Frame
	WriteRegister (4235, 0x00000000);  // Extract 3 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4236, 0xE4E5E6E7);  // Extract 3 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	WriteRegister (4237, 0xE0E1E2E3);  // Extract 3 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	WriteRegister (4238, 0xA4A5A6A7);  // Extract 3 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	WriteRegister (4239, 0xA0A1A2A3);  // Extract 3 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	WriteRegister (4240, 0xE7E7E7E7);  // Extract 3 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	WriteRegister (4241, 0x010A0004);  // Extract 3 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	WriteRegister (4242, 0x00000000);  // Extract 3 Analog F1 Y Filter  // Each 1 bit specifies capturing F1 Y line as analog, else digital
	WriteRegister (4243, 0x00000000);  // Extract 3 Analog F2 Y Filter  // Each 1 bit specifies capturing F2 Y line as analog, else digital
	WriteRegister (4244, 0x00000000);  // Extract 3 Analog F1 C Filter  // Each 1 bit specifies capturing F1 C line as analog, else digital
	WriteRegister (4245, 0x00000000);  // Extract 3 Analog F2 C Filter  // Each 1 bit specifies capturing F2 C line as analog, else digital
	WriteRegister (4251, 0x00000780);  // Reg 0x109B
	WriteRegister (4288, 0x10011111);  // Extract 4 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	WriteRegister (4289, 0x31800000);  // Extract 4 F1 Start Address
	WriteRegister (4290, 0x319FFFFF);  // Extract 4 F1 End Address
	WriteRegister (4291, 0x31A00000);  // Extract 4 F2 Start Address
	WriteRegister (4292, 0x31BFFFFF);  // Extract 4 F2 End Address
	WriteRegister (4293, 0x00640064);  // Extract 4 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	WriteRegister (4294, 0x00000000);  // Extract 4 Memory Total  // Total bytes: 0, Overrun: N
	WriteRegister (4295, 0x00000000);  // Extract 4 F1 Memory Usage  // Total F1 bytes: 0, Overrun: N
	WriteRegister (4296, 0x00000000);  // Extract 4 F2 Memory Usage  // Total F2 bytes: 0, Overrun: N
	WriteRegister (4297, 0x00000462);  // Extract 4 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	WriteRegister (4298, 0x00000465);  // Extract 4 Lines Per Frame
	WriteRegister (4299, 0x00000000);  // Extract 4 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4300, 0xE4E5E6E7);  // Extract 4 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	WriteRegister (4301, 0xE0E1E2E3);  // Extract 4 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	WriteRegister (4302, 0xA4A5A6A7);  // Extract 4 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	WriteRegister (4303, 0xA0A1A2A3);  // Extract 4 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	WriteRegister (4304, 0xE7E7E7E7);  // Extract 4 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	WriteRegister (4305, 0x010A0004);  // Extract 4 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	WriteRegister (4306, 0x00000000);  // Extract 4 Analog F1 Y Filter  // Each 1 bit specifies capturing F1 Y line as analog, else digital
	WriteRegister (4307, 0x00000000);  // Extract 4 Analog F2 Y Filter  // Each 1 bit specifies capturing F2 Y line as analog, else digital
	WriteRegister (4308, 0x00000000);  // Extract 4 Analog F1 C Filter  // Each 1 bit specifies capturing F1 C line as analog, else digital
	WriteRegister (4309, 0x00000000);  // Extract 4 Analog F2 C Filter  // Each 1 bit specifies capturing F2 C line as analog, else digital
	WriteRegister (4315, 0x00000780);  // Reg 0x10DB
	WriteRegister (4352, 0x11010000);  // Extract 5 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Progressive video: Y, Synchronize: frame, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	WriteRegister (4353, 0x0DFFC000);  // Extract 5 F1 Start Address
	WriteRegister (4354, 0x0DFFDFFF);  // Extract 5 F1 End Address
	WriteRegister (4355, 0x00000000);  // Extract 5 F2 Start Address
	WriteRegister (4356, 0x00000000);  // Extract 5 F2 End Address
	WriteRegister (4357, 0x00000465);  // Extract 5 Field Cutoff Lines  // F1 cutoff line: 1125, F2 cutoff line: 0
	WriteRegister (4358, 0x00000000);  // Extract 5 Memory Total  // Total bytes: 0, Overrun: N
	WriteRegister (4359, 0x00000000);  // Extract 5 F1 Memory Usage  // Total F1 bytes: 0, Overrun: N
	WriteRegister (4360, 0x00000017);  // Extract 5 F2 Memory Usage  // Total F2 bytes: 23, Overrun: N
	WriteRegister (4361, 0x00000462);  // Extract 5 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	WriteRegister (4362, 0x00000465);  // Extract 5 Lines Per Frame
	WriteRegister (4363, 0x00000000);  // Extract 5 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4364, 0xE4E5E6E7);  // Extract 5 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	WriteRegister (4365, 0xE0E1E2E3);  // Extract 5 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	WriteRegister (4366, 0xA4A5A6A7);  // Extract 5 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	WriteRegister (4367, 0xA0A1A2A3);  // Extract 5 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	WriteRegister (4368, 0x00000000);  // Extract 5 Ignore DID 17-20  // Ignoring DIDs 00, 00, 00, 00
	WriteRegister (4369, 0x00000000);  // Extract 5 Analog Start Line  // F1 analog start line: 0, F2 analog start line: 0
	WriteRegister (4370, 0x00000000);  // Extract 5 Analog F1 Y Filter  // Each 1 bit specifies capturing F1 Y line as analog, else digital
	WriteRegister (4371, 0x00000000);  // Extract 5 Analog F2 Y Filter  // Each 1 bit specifies capturing F2 Y line as analog, else digital
	WriteRegister (4372, 0x00000000);  // Extract 5 Analog F1 C Filter  // Each 1 bit specifies capturing F1 C line as analog, else digital
	WriteRegister (4373, 0x00000000);  // Extract 5 Analog F2 C Filter  // Each 1 bit specifies capturing F2 C line as analog, else digital
	WriteRegister (4379, 0x00000780);  // Reg 0x111B
	WriteRegister (4416, 0x10011111);  // Extract 6 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	WriteRegister (4417, 0x3A800000);  // Extract 6 F1 Start Address
	WriteRegister (4418, 0x3A9FFFFF);  // Extract 6 F1 End Address
	WriteRegister (4419, 0x3AA00000);  // Extract 6 F2 Start Address
	WriteRegister (4420, 0x3ABFFFFF);  // Extract 6 F2 End Address
	WriteRegister (4421, 0x00640064);  // Extract 6 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	WriteRegister (4422, 0x00000000);  // Extract 6 Memory Total  // Total bytes: 0, Overrun: N
	WriteRegister (4423, 0x00000000);  // Extract 6 F1 Memory Usage  // Total F1 bytes: 0, Overrun: N
	WriteRegister (4424, 0x00000000);  // Extract 6 F2 Memory Usage  // Total F2 bytes: 0, Overrun: N
	WriteRegister (4425, 0x00000462);  // Extract 6 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	WriteRegister (4426, 0x00000465);  // Extract 6 Lines Per Frame
	WriteRegister (4427, 0x00000000);  // Extract 6 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4428, 0xE4E5E6E7);  // Extract 6 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	WriteRegister (4429, 0xE0E1E2E3);  // Extract 6 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	WriteRegister (4430, 0xA4A5A6A7);  // Extract 6 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	WriteRegister (4431, 0xA0A1A2A3);  // Extract 6 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	WriteRegister (4432, 0xE7E7E7E7);  // Extract 6 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	WriteRegister (4433, 0x010A0004);  // Extract 6 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	WriteRegister (4434, 0x00000000);  // Extract 6 Analog F1 Y Filter  // Each 1 bit specifies capturing F1 Y line as analog, else digital
	WriteRegister (4435, 0x00000000);  // Extract 6 Analog F2 Y Filter  // Each 1 bit specifies capturing F2 Y line as analog, else digital
	WriteRegister (4436, 0x00000000);  // Extract 6 Analog F1 C Filter  // Each 1 bit specifies capturing F1 C line as analog, else digital
	WriteRegister (4437, 0x00000000);  // Extract 6 Analog F2 C Filter  // Each 1 bit specifies capturing F2 C line as analog, else digital
	WriteRegister (4443, 0x00000780);  // Reg 0x115B
	WriteRegister (4480, 0x10011111);  // Extract 7 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	WriteRegister (4481, 0x3B000000);  // Extract 7 F1 Start Address
	WriteRegister (4482, 0x3B1FFFFF);  // Extract 7 F1 End Address
	WriteRegister (4483, 0x3B200000);  // Extract 7 F2 Start Address
	WriteRegister (4484, 0x3B3FFFFF);  // Extract 7 F2 End Address
	WriteRegister (4485, 0x00640064);  // Extract 7 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	WriteRegister (4486, 0x00000000);  // Extract 7 Memory Total  // Total bytes: 0, Overrun: N
	WriteRegister (4487, 0x00000000);  // Extract 7 F1 Memory Usage  // Total F1 bytes: 0, Overrun: N
	WriteRegister (4488, 0x00000000);  // Extract 7 F2 Memory Usage  // Total F2 bytes: 0, Overrun: N
	WriteRegister (4489, 0x00000462);  // Extract 7 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	WriteRegister (4490, 0x00000465);  // Extract 7 Lines Per Frame
	WriteRegister (4491, 0x00000000);  // Extract 7 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4492, 0xE4E5E6E7);  // Extract 7 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	WriteRegister (4493, 0xE0E1E2E3);  // Extract 7 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	WriteRegister (4494, 0xA4A5A6A7);  // Extract 7 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	WriteRegister (4495, 0xA0A1A2A3);  // Extract 7 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	WriteRegister (4496, 0xE7E7E7E7);  // Extract 7 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	WriteRegister (4497, 0x010A0004);  // Extract 7 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	WriteRegister (4498, 0x00000000);  // Extract 7 Analog F1 Y Filter  // Each 1 bit specifies capturing F1 Y line as analog, else digital
	WriteRegister (4499, 0x00000000);  // Extract 7 Analog F2 Y Filter  // Each 1 bit specifies capturing F2 Y line as analog, else digital
	WriteRegister (4500, 0x00000000);  // Extract 7 Analog F1 C Filter  // Each 1 bit specifies capturing F1 C line as analog, else digital
	WriteRegister (4501, 0x00000000);  // Extract 7 Analog F2 C Filter  // Each 1 bit specifies capturing F2 C line as analog, else digital
	WriteRegister (4507, 0x00000780);  // Reg 0x119B
	WriteRegister (4544, 0x10011111);  // Extract 8 Control  // HANC Y enable: Y, VANC Y enable: Y, HANC C enable: Y, VANC C enable: Y, Progressive video: Y, Synchronize: field, Memory writes: Disabled, SD Y+C Demux: Disabled, Metadata from: MSBs
	WriteRegister (4545, 0x3B800000);  // Extract 8 F1 Start Address
	WriteRegister (4546, 0x3B9FFFFF);  // Extract 8 F1 End Address
	WriteRegister (4547, 0x3BA00000);  // Extract 8 F2 Start Address
	WriteRegister (4548, 0x3BBFFFFF);  // Extract 8 F2 End Address
	WriteRegister (4549, 0x00640064);  // Extract 8 Field Cutoff Lines  // F1 cutoff line: 100, F2 cutoff line: 100
	WriteRegister (4550, 0x00000000);  // Extract 8 Memory Total  // Total bytes: 0, Overrun: N
	WriteRegister (4551, 0x00000000);  // Extract 8 F1 Memory Usage  // Total F1 bytes: 0, Overrun: N
	WriteRegister (4552, 0x00000000);  // Extract 8 F2 Memory Usage  // Total F2 bytes: 0, Overrun: N
	WriteRegister (4553, 0x00000462);  // Extract 8 V Blank Lines  // F1 VBL start line: 1122, F2 VBL start line: 0
	WriteRegister (4554, 0x00000465);  // Extract 8 Lines Per Frame
	WriteRegister (4555, 0x00000000);  // Extract 8 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4556, 0xE4E5E6E7);  // Extract 8 Ignore DID 1-4  // Ignoring DIDs E7, E6, E5, E4
	WriteRegister (4557, 0xE0E1E2E3);  // Extract 8 Ignore DID 5-8  // Ignoring DIDs E3, E2, E1, E0
	WriteRegister (4558, 0xA4A5A6A7);  // Extract 8 Ignore DID 9-12  // Ignoring DIDs A7, A6, A5, A4
	WriteRegister (4559, 0xA0A1A2A3);  // Extract 8 Ignore DID 13-16  // Ignoring DIDs A3, A2, A1, A0
	WriteRegister (4560, 0xE7E7E7E7);  // Extract 8 Ignore DID 17-20  // Ignoring DIDs E7, E7, E7, E7
	WriteRegister (4561, 0x010A0004);  // Extract 8 Analog Start Line  // F1 analog start line: 4, F2 analog start line: 266
	WriteRegister (4562, 0x00000000);  // Extract 8 Analog F1 Y Filter  // Each 1 bit specifies capturing F1 Y line as analog, else digital
	WriteRegister (4563, 0x00000000);  // Extract 8 Analog F2 Y Filter  // Each 1 bit specifies capturing F2 Y line as analog, else digital
	WriteRegister (4564, 0x00000000);  // Extract 8 Analog F1 C Filter  // Each 1 bit specifies capturing F1 C line as analog, else digital
	WriteRegister (4565, 0x00000000);  // Extract 8 Analog F2 C Filter  // Each 1 bit specifies capturing F2 C line as analog, else digital
	WriteRegister (4571, 0x00000780);  // Reg 0x11DB
	WriteRegister (4608, 0x00000000);  // Insert 1 Field Bytes  // F1 byte count: 0, F2 byte count: 0
	WriteRegister (4609, 0x11000000);  // Insert 1 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	WriteRegister (4610, 0x30000000);  // Insert 1 F1 Start Address
	WriteRegister (4611, 0x30400000);  // Insert 1 F2 Start Address
	WriteRegister (4612, 0x00000008);  // Insert 1 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	WriteRegister (4613, 0x0000002A);  // Insert 1 Active Start  // F1 first active line: 42, F2 first active line: 0
	WriteRegister (4614, 0x08980780);  // Insert 1 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	WriteRegister (4615, 0x00000465);  // Insert 1 Lines Per Frame
	WriteRegister (4616, 0x00000000);  // Insert 1 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4617, 0x0000000A);  // Insert 1 Payload ID Control
	WriteRegister (4618, 0x0100CA59);  // Insert 1 Payload ID
	WriteRegister (4619, 0x00000000);  // Insert 1 Chroma Blank Lines  // F1 chroma blnk start line: 0, F2 chroma blnk start line: 0
	WriteRegister (4620, 0x00000000);  // Insert 1 F1 C Blanking Mask  // Each 1 bit specifies if chroma in F1 should be blanked or passed thru
	WriteRegister (4621, 0x00000000);  // Insert 1 F2 C Blanking Mask  // Each 1 bit specifies if chroma in F2 should be blanked or passed thru
	WriteRegister (4624, 0x00000000);  // Insert 1 RTP Payload ID
	WriteRegister (4625, 0x00000000);  // Insert 1 RTP SSRC
	WriteRegister (4626, 0x00000000);  // Insert 1 IP Channel
	WriteRegister (4672, 0x00000000);  // Insert 2 Field Bytes  // F1 byte count: 0, F2 byte count: 0
	WriteRegister (4673, 0x11000000);  // Insert 2 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	WriteRegister (4674, 0x30800000);  // Insert 2 F1 Start Address
	WriteRegister (4675, 0x30C00000);  // Insert 2 F2 Start Address
	WriteRegister (4676, 0x00000008);  // Insert 2 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	WriteRegister (4677, 0x0000002A);  // Insert 2 Active Start  // F1 first active line: 42, F2 first active line: 0
	WriteRegister (4678, 0x08980780);  // Insert 2 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	WriteRegister (4679, 0x00000465);  // Insert 2 Lines Per Frame
	WriteRegister (4680, 0x00000000);  // Insert 2 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4681, 0x0000000A);  // Insert 2 Payload ID Control
	WriteRegister (4682, 0x0100CA59);  // Insert 2 Payload ID
	WriteRegister (4683, 0x00000000);  // Insert 2 Chroma Blank Lines  // F1 chroma blnk start line: 0, F2 chroma blnk start line: 0
	WriteRegister (4684, 0x00000000);  // Insert 2 F1 C Blanking Mask  // Each 1 bit specifies if chroma in F1 should be blanked or passed thru
	WriteRegister (4685, 0x00000000);  // Insert 2 F2 C Blanking Mask  // Each 1 bit specifies if chroma in F2 should be blanked or passed thru
	WriteRegister (4688, 0x00000000);  // Insert 2 RTP Payload ID
	WriteRegister (4689, 0x00000000);  // Insert 2 RTP SSRC
	WriteRegister (4690, 0x00000000);  // Insert 2 IP Channel
	WriteRegister (4736, 0x00000000);  // Insert 3 Field Bytes  // F1 byte count: 0, F2 byte count: 0
	WriteRegister (4737, 0x11000000);  // Insert 3 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	WriteRegister (4738, 0x31000000);  // Insert 3 F1 Start Address
	WriteRegister (4739, 0x31400000);  // Insert 3 F2 Start Address
	WriteRegister (4740, 0x00000008);  // Insert 3 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	WriteRegister (4741, 0x0000002A);  // Insert 3 Active Start  // F1 first active line: 42, F2 first active line: 0
	WriteRegister (4742, 0x08980780);  // Insert 3 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	WriteRegister (4743, 0x00000465);  // Insert 3 Lines Per Frame
	WriteRegister (4744, 0x00000000);  // Insert 3 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4745, 0x0000000A);  // Insert 3 Payload ID Control
	WriteRegister (4746, 0x0100CA59);  // Insert 3 Payload ID
	WriteRegister (4747, 0x00000000);  // Insert 3 Chroma Blank Lines  // F1 chroma blnk start line: 0, F2 chroma blnk start line: 0
	WriteRegister (4748, 0x00000000);  // Insert 3 F1 C Blanking Mask  // Each 1 bit specifies if chroma in F1 should be blanked or passed thru
	WriteRegister (4749, 0x00000000);  // Insert 3 F2 C Blanking Mask  // Each 1 bit specifies if chroma in F2 should be blanked or passed thru
	WriteRegister (4752, 0x00000000);  // Insert 3 RTP Payload ID
	WriteRegister (4753, 0x00000000);  // Insert 3 RTP SSRC
	WriteRegister (4754, 0x00000000);  // Insert 3 IP Channel
	WriteRegister (4800, 0x00000000);  // Insert 4 Field Bytes  // F1 byte count: 0, F2 byte count: 0
	WriteRegister (4801, 0x11000000);  // Insert 4 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	WriteRegister (4802, 0x31800000);  // Insert 4 F1 Start Address
	WriteRegister (4803, 0x31C00000);  // Insert 4 F2 Start Address
	WriteRegister (4804, 0x00000008);  // Insert 4 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	WriteRegister (4805, 0x0000002A);  // Insert 4 Active Start  // F1 first active line: 42, F2 first active line: 0
	WriteRegister (4806, 0x08980780);  // Insert 4 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	WriteRegister (4807, 0x00000465);  // Insert 4 Lines Per Frame
	WriteRegister (4808, 0x00000000);  // Insert 4 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4809, 0x0000000A);  // Insert 4 Payload ID Control
	WriteRegister (4810, 0x0100CA59);  // Insert 4 Payload ID
	WriteRegister (4811, 0x00000000);  // Insert 4 Chroma Blank Lines  // F1 chroma blnk start line: 0, F2 chroma blnk start line: 0
	WriteRegister (4812, 0x00000000);  // Insert 4 F1 C Blanking Mask  // Each 1 bit specifies if chroma in F1 should be blanked or passed thru
	WriteRegister (4813, 0x00000000);  // Insert 4 F2 C Blanking Mask  // Each 1 bit specifies if chroma in F2 should be blanked or passed thru
	WriteRegister (4816, 0x00000000);  // Insert 4 RTP Payload ID
	WriteRegister (4817, 0x00000000);  // Insert 4 RTP SSRC
	WriteRegister (4818, 0x00000000);  // Insert 4 IP Channel
	WriteRegister (4864, 0x00000000);  // Insert 5 Field Bytes  // F1 byte count: 0, F2 byte count: 0
	WriteRegister (4865, 0x11000000);  // Insert 5 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	WriteRegister (4866, 0x30A00000);  // Insert 5 F1 Start Address
	WriteRegister (4867, 0x30E00000);  // Insert 5 F2 Start Address
	WriteRegister (4868, 0x00000008);  // Insert 5 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	WriteRegister (4869, 0x0000002A);  // Insert 5 Active Start  // F1 first active line: 42, F2 first active line: 0
	WriteRegister (4870, 0x08980780);  // Insert 5 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	WriteRegister (4871, 0x00000465);  // Insert 5 Lines Per Frame
	WriteRegister (4872, 0x00000000);  // Insert 5 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4873, 0x0000000A);  // Insert 5 Payload ID Control
	WriteRegister (4874, 0x0100CA59);  // Insert 5 Payload ID
	WriteRegister (4875, 0x00000000);  // Insert 5 Chroma Blank Lines  // F1 chroma blnk start line: 0, F2 chroma blnk start line: 0
	WriteRegister (4876, 0x00000000);  // Insert 5 F1 C Blanking Mask  // Each 1 bit specifies if chroma in F1 should be blanked or passed thru
	WriteRegister (4877, 0x00000000);  // Insert 5 F2 C Blanking Mask  // Each 1 bit specifies if chroma in F2 should be blanked or passed thru
	WriteRegister (4880, 0x00000000);  // Insert 5 RTP Payload ID
	WriteRegister (4881, 0x00000000);  // Insert 5 RTP SSRC
	WriteRegister (4882, 0x00000000);  // Insert 5 IP Channel
	WriteRegister (4928, 0x00000000);  // Insert 6 Field Bytes  // F1 byte count: 0, F2 byte count: 0
	WriteRegister (4929, 0x11000000);  // Insert 6 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	WriteRegister (4930, 0x31200000);  // Insert 6 F1 Start Address
	WriteRegister (4931, 0x31600000);  // Insert 6 F2 Start Address
	WriteRegister (4932, 0x00000008);  // Insert 6 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	WriteRegister (4933, 0x0000002A);  // Insert 6 Active Start  // F1 first active line: 42, F2 first active line: 0
	WriteRegister (4934, 0x08980780);  // Insert 6 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	WriteRegister (4935, 0x00000465);  // Insert 6 Lines Per Frame
	WriteRegister (4936, 0x00000000);  // Insert 6 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (4937, 0x0000000A);  // Insert 6 Payload ID Control
	WriteRegister (4938, 0x0100CA59);  // Insert 6 Payload ID
	WriteRegister (4939, 0x00000000);  // Insert 6 Chroma Blank Lines  // F1 chroma blnk start line: 0, F2 chroma blnk start line: 0
	WriteRegister (4940, 0x00000000);  // Insert 6 F1 C Blanking Mask  // Each 1 bit specifies if chroma in F1 should be blanked or passed thru
	WriteRegister (4941, 0x00000000);  // Insert 6 F2 C Blanking Mask  // Each 1 bit specifies if chroma in F2 should be blanked or passed thru
	WriteRegister (4944, 0x00000000);  // Insert 6 RTP Payload ID
	WriteRegister (4945, 0x00000000);  // Insert 6 RTP SSRC
	WriteRegister (4946, 0x00000000);  // Insert 6 IP Channel
	WriteRegister (4992, 0x00000000);  // Insert 7 Field Bytes  // F1 byte count: 0, F2 byte count: 0
	WriteRegister (4993, 0x11000000);  // Insert 7 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	WriteRegister (4994, 0x31A00000);  // Insert 7 F1 Start Address
	WriteRegister (4995, 0x31E00000);  // Insert 7 F2 Start Address
	WriteRegister (4996, 0x00000008);  // Insert 7 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	WriteRegister (4997, 0x0000002A);  // Insert 7 Active Start  // F1 first active line: 42, F2 first active line: 0
	WriteRegister (4998, 0x08980780);  // Insert 7 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	WriteRegister (4999, 0x00000465);  // Insert 7 Lines Per Frame
	WriteRegister (5000, 0x00000000);  // Insert 7 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (5001, 0x0000000A);  // Insert 7 Payload ID Control
	WriteRegister (5002, 0x0100CA59);  // Insert 7 Payload ID
	WriteRegister (5003, 0x00000000);  // Insert 7 Chroma Blank Lines  // F1 chroma blnk start line: 0, F2 chroma blnk start line: 0
	WriteRegister (5004, 0x00000000);  // Insert 7 F1 C Blanking Mask  // Each 1 bit specifies if chroma in F1 should be blanked or passed thru
	WriteRegister (5005, 0x00000000);  // Insert 7 F2 C Blanking Mask  // Each 1 bit specifies if chroma in F2 should be blanked or passed thru
	WriteRegister (5008, 0x00000000);  // Insert 7 RTP Payload ID
	WriteRegister (5009, 0x00000000);  // Insert 7 RTP SSRC
	WriteRegister (5010, 0x00000000);  // Insert 7 IP Channel
	WriteRegister (5056, 0x00000000);  // Insert 8 Field Bytes  // F1 byte count: 0, F2 byte count: 0
	WriteRegister (5057, 0x11000000);  // Insert 8 Control  // HANC Y enable: N, VANC Y enable: N, HANC C enable: N, VANC C enable: N, Payload Y insert: N, Payload C insert: N, Payload F1 insert: N, Payload F2 insert: N, Progressive video: Y, Memory reads: Disabled, SD Packet Split: Disabled
	WriteRegister (5058, 0x32200000);  // Insert 8 F1 Start Address
	WriteRegister (5059, 0x32600000);  // Insert 8 F2 Start Address
	WriteRegister (5060, 0x00000008);  // Insert 8 Pixel Delay  // HANC pixel delay: 8, VANC pixel delay: 0
	WriteRegister (5061, 0x0000002A);  // Insert 8 Active Start  // F1 first active line: 42, F2 first active line: 0
	WriteRegister (5062, 0x08980780);  // Insert 8 Pixels Per Line  // Active line length: 1920, Total line length: 2200
	WriteRegister (5063, 0x00000465);  // Insert 8 Lines Per Frame
	WriteRegister (5064, 0x00000000);  // Insert 8 Field ID Lines  // Field ID high on line: 0, Field ID low on line: 0
	WriteRegister (5065, 0x0000000A);  // Insert 8 Payload ID Control
	WriteRegister (5066, 0x0100CA59);  // Insert 8 Payload ID
	WriteRegister (5067, 0x00000000);  // Insert 8 Chroma Blank Lines  // F1 chroma blnk start line: 0, F2 chroma blnk start line: 0
	WriteRegister (5068, 0x00000000);  // Insert 8 F1 C Blanking Mask  // Each 1 bit specifies if chroma in F1 should be blanked or passed thru
	WriteRegister (5069, 0x00000000);  // Insert 8 F2 C Blanking Mask  // Each 1 bit specifies if chroma in F2 should be blanked or passed thru
	WriteRegister (5072, 0x00000000);  // Insert 8 RTP Payload ID
	WriteRegister (5073, 0x00000000);  // Insert 8 RTP SSRC
	WriteRegister (5074, 0x00000000);  // Insert 8 IP Channel
	WriteRegister (kRegEnhancedCSC1Mode, 0x00000000);  // Reg 5120  // Filter select: Full, Filter edge control: Filter to black, Output pixel format: RGB 4:4:4, Input pixel format: RGB 4:4:4
	WriteRegister (kRegEnhancedCSC1InOffset0_1, 0x00000000);  // Reg 5121  // Component 0 input offset: 0.0000 (12-bit), 0.00000 (10-bit), Component 1 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC1InOffset2, 0x00000000);  // Reg 5122  // Component 2 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC1CoeffA0, 0x00000000);  // Reg 5123  // A0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1CoeffA1, 0x00000000);  // Reg 5124  // A1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1CoeffA2, 0x00000000);  // Reg 5125  // A2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1CoeffB0, 0x00000000);  // Reg 5126  // B0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1CoeffB1, 0x00000000);  // Reg 5127  // B1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1CoeffB2, 0x00000000);  // Reg 5128  // B2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1CoeffC0, 0x00000000);  // Reg 5129  // C0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1CoeffC1, 0x00000000);  // Reg 5130  // C1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1CoeffC2, 0x00000000);  // Reg 5131  // C2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC1OutOffsetA_B, 0x00000000);  // Reg 5132  // Component A output offset: 0.0000 (12-bit), 0.00000 (10-bit), Component B output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC1OutOffsetC, 0x00000000);  // Reg 5133  // Component C output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC1KeyMode, 0x00000000);  // Reg 5134  // Key Source Select: Key Input, Key Output Range: Full Range
	WriteRegister (kRegEnhancedCSC1KeyClipOffset, 0x00000000);  // Reg 5135  // Key input offset: 0.00 (12-bit), 0.0000 (10-bit), Key output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC1KeyGain, 0x00000000);  // Reg 5136  // Key gain: 0.000000 (00000000)
	WriteRegister (kRegEnhancedCSC2Mode, 0x00000000);  // Reg 5184  // Filter select: Full, Filter edge control: Filter to black, Output pixel format: RGB 4:4:4, Input pixel format: RGB 4:4:4
	WriteRegister (kRegEnhancedCSC2InOffset0_1, 0x00000000);  // Reg 5185  // Component 0 input offset: 0.0000 (12-bit), 0.00000 (10-bit), Component 1 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC2InOffset2, 0x00000000);  // Reg 5186  // Component 2 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC2CoeffA0, 0x00000000);  // Reg 5187  // A0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2CoeffA1, 0x00000000);  // Reg 5188  // A1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2CoeffA2, 0x00000000);  // Reg 5189  // A2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2CoeffB0, 0x00000000);  // Reg 5190  // B0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2CoeffB1, 0x00000000);  // Reg 5191  // B1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2CoeffB2, 0x00000000);  // Reg 5192  // B2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2CoeffC0, 0x00000000);  // Reg 5193  // C0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2CoeffC1, 0x00000000);  // Reg 5194  // C1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2CoeffC2, 0x00000000);  // Reg 5195  // C2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC2OutOffsetA_B, 0x00000000);  // Reg 5196  // Component A output offset: 0.0000 (12-bit), 0.00000 (10-bit), Component B output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC2OutOffsetC, 0x00000000);  // Reg 5197  // Component C output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC2KeyMode, 0x00000000);  // Reg 5198  // Key Source Select: Key Input, Key Output Range: Full Range
	WriteRegister (kRegEnhancedCSC2KeyClipOffset, 0x00000000);  // Reg 5199  // Key input offset: 0.00 (12-bit), 0.0000 (10-bit), Key output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC2KeyGain, 0x00000000);  // Reg 5200  // Key gain: 0.000000 (00000000)
	WriteRegister (kRegEnhancedCSC3Mode, 0x00000000);  // Reg 5248  // Filter select: Full, Filter edge control: Filter to black, Output pixel format: RGB 4:4:4, Input pixel format: RGB 4:4:4
	WriteRegister (kRegEnhancedCSC3InOffset0_1, 0x00000000);  // Reg 5249  // Component 0 input offset: 0.0000 (12-bit), 0.00000 (10-bit), Component 1 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC3InOffset2, 0x00000000);  // Reg 5250  // Component 2 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC3CoeffA0, 0x00000000);  // Reg 5251  // A0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3CoeffA1, 0x00000000);  // Reg 5252  // A1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3CoeffA2, 0x00000000);  // Reg 5253  // A2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3CoeffB0, 0x00000000);  // Reg 5254  // B0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3CoeffB1, 0x00000000);  // Reg 5255  // B1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3CoeffB2, 0x00000000);  // Reg 5256  // B2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3CoeffC0, 0x00000000);  // Reg 5257  // C0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3CoeffC1, 0x00000000);  // Reg 5258  // C1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3CoeffC2, 0x00000000);  // Reg 5259  // C2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC3OutOffsetA_B, 0x00000000);  // Reg 5260  // Component A output offset: 0.0000 (12-bit), 0.00000 (10-bit), Component B output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC3OutOffsetC, 0x00000000);  // Reg 5261  // Component C output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC3KeyMode, 0x00000000);  // Reg 5262  // Key Source Select: Key Input, Key Output Range: Full Range
	WriteRegister (kRegEnhancedCSC3KeyClipOffset, 0x00000000);  // Reg 5263  // Key input offset: 0.00 (12-bit), 0.0000 (10-bit), Key output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC3KeyGain, 0x00000000);  // Reg 5264  // Key gain: 0.000000 (00000000)
	WriteRegister (kRegEnhancedCSC4Mode, 0x00000000);  // Reg 5312  // Filter select: Full, Filter edge control: Filter to black, Output pixel format: RGB 4:4:4, Input pixel format: RGB 4:4:4
	WriteRegister (kRegEnhancedCSC4InOffset0_1, 0x00000000);  // Reg 5313  // Component 0 input offset: 0.0000 (12-bit), 0.00000 (10-bit), Component 1 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC4InOffset2, 0x00000000);  // Reg 5314  // Component 2 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC4CoeffA0, 0x00000000);  // Reg 5315  // A0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4CoeffA1, 0x00000000);  // Reg 5316  // A1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4CoeffA2, 0x00000000);  // Reg 5317  // A2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4CoeffB0, 0x00000000);  // Reg 5318  // B0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4CoeffB1, 0x00000000);  // Reg 5319  // B1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4CoeffB2, 0x00000000);  // Reg 5320  // B2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4CoeffC0, 0x00000000);  // Reg 5321  // C0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4CoeffC1, 0x00000000);  // Reg 5322  // C1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4CoeffC2, 0x00000000);  // Reg 5323  // C2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC4OutOffsetA_B, 0x00000000);  // Reg 5324  // Component A output offset: 0.0000 (12-bit), 0.00000 (10-bit), Component B output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC4OutOffsetC, 0x00000000);  // Reg 5325  // Component C output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC4KeyMode, 0x00000000);  // Reg 5326  // Key Source Select: Key Input, Key Output Range: Full Range
	WriteRegister (kRegEnhancedCSC4KeyClipOffset, 0x00000000);  // Reg 5327  // Key input offset: 0.00 (12-bit), 0.0000 (10-bit), Key output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC4KeyGain, 0x00000000);  // Reg 5328  // Key gain: 0.000000 (00000000)
	WriteRegister (kRegEnhancedCSC5Mode, 0x00000000);  // Reg 5376  // Filter select: Full, Filter edge control: Filter to black, Output pixel format: RGB 4:4:4, Input pixel format: RGB 4:4:4
	WriteRegister (kRegEnhancedCSC5InOffset0_1, 0x00000000);  // Reg 5377  // Component 0 input offset: 0.0000 (12-bit), 0.00000 (10-bit), Component 1 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC5InOffset2, 0x00000000);  // Reg 5378  // Component 2 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC5CoeffA0, 0x00000000);  // Reg 5379  // A0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5CoeffA1, 0x00000000);  // Reg 5380  // A1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5CoeffA2, 0x00000000);  // Reg 5381  // A2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5CoeffB0, 0x00000000);  // Reg 5382  // B0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5CoeffB1, 0x00000000);  // Reg 5383  // B1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5CoeffB2, 0x00000000);  // Reg 5384  // B2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5CoeffC0, 0x00000000);  // Reg 5385  // C0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5CoeffC1, 0x00000000);  // Reg 5386  // C1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5CoeffC2, 0x00000000);  // Reg 5387  // C2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC5OutOffsetA_B, 0x00000000);  // Reg 5388  // Component A output offset: 0.0000 (12-bit), 0.00000 (10-bit), Component B output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC5OutOffsetC, 0x00000000);  // Reg 5389  // Component C output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC5KeyMode, 0x00000000);  // Reg 5390  // Key Source Select: Key Input, Key Output Range: Full Range
	WriteRegister (kRegEnhancedCSC5KeyClipOffset, 0x00000000);  // Reg 5391  // Key input offset: 0.00 (12-bit), 0.0000 (10-bit), Key output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC5KeyGain, 0x00000000);  // Reg 5392  // Key gain: 0.000000 (00000000)
	WriteRegister (kRegEnhancedCSC6Mode, 0x00000000);  // Reg 5440  // Filter select: Full, Filter edge control: Filter to black, Output pixel format: RGB 4:4:4, Input pixel format: RGB 4:4:4
	WriteRegister (kRegEnhancedCSC6InOffset0_1, 0x00000000);  // Reg 5441  // Component 0 input offset: 0.0000 (12-bit), 0.00000 (10-bit), Component 1 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC6InOffset2, 0x00000000);  // Reg 5442  // Component 2 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC6CoeffA0, 0x00000000);  // Reg 5443  // A0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6CoeffA1, 0x00000000);  // Reg 5444  // A1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6CoeffA2, 0x00000000);  // Reg 5445  // A2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6CoeffB0, 0x00000000);  // Reg 5446  // B0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6CoeffB1, 0x00000000);  // Reg 5447  // B1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6CoeffB2, 0x00000000);  // Reg 5448  // B2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6CoeffC0, 0x00000000);  // Reg 5449  // C0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6CoeffC1, 0x00000000);  // Reg 5450  // C1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6CoeffC2, 0x00000000);  // Reg 5451  // C2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC6OutOffsetA_B, 0x00000000);  // Reg 5452  // Component A output offset: 0.0000 (12-bit), 0.00000 (10-bit), Component B output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC6OutOffsetC, 0x00000000);  // Reg 5453  // Component C output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC6KeyMode, 0x00000000);  // Reg 5454  // Key Source Select: Key Input, Key Output Range: Full Range
	WriteRegister (kRegEnhancedCSC6KeyClipOffset, 0x00000000);  // Reg 5455  // Key input offset: 0.00 (12-bit), 0.0000 (10-bit), Key output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC6KeyGain, 0x00000000);  // Reg 5456  // Key gain: 0.000000 (00000000)
	WriteRegister (kRegEnhancedCSC7Mode, 0x00000000);  // Reg 5504  // Filter select: Full, Filter edge control: Filter to black, Output pixel format: RGB 4:4:4, Input pixel format: RGB 4:4:4
	WriteRegister (kRegEnhancedCSC7InOffset0_1, 0x00000000);  // Reg 5505  // Component 0 input offset: 0.0000 (12-bit), 0.00000 (10-bit), Component 1 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC7InOffset2, 0x00000000);  // Reg 5506  // Component 2 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC7CoeffA0, 0x00000000);  // Reg 5507  // A0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7CoeffA1, 0x00000000);  // Reg 5508  // A1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7CoeffA2, 0x00000000);  // Reg 5509  // A2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7CoeffB0, 0x00000000);  // Reg 5510  // B0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7CoeffB1, 0x00000000);  // Reg 5511  // B1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7CoeffB2, 0x00000000);  // Reg 5512  // B2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7CoeffC0, 0x00000000);  // Reg 5513  // C0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7CoeffC1, 0x00000000);  // Reg 5514  // C1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7CoeffC2, 0x00000000);  // Reg 5515  // C2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC7OutOffsetA_B, 0x00000000);  // Reg 5516  // Component A output offset: 0.0000 (12-bit), 0.00000 (10-bit), Component B output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC7OutOffsetC, 0x00000000);  // Reg 5517  // Component C output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC7KeyMode, 0x00000000);  // Reg 5518  // Key Source Select: Key Input, Key Output Range: Full Range
	WriteRegister (kRegEnhancedCSC7KeyClipOffset, 0x00000000);  // Reg 5519  // Key input offset: 0.00 (12-bit), 0.0000 (10-bit), Key output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC7KeyGain, 0x00000000);  // Reg 5520  // Key gain: 0.000000 (00000000)
	WriteRegister (kRegEnhancedCSC8Mode, 0x00000000);  // Reg 5568  // Filter select: Full, Filter edge control: Filter to black, Output pixel format: RGB 4:4:4, Input pixel format: RGB 4:4:4
	WriteRegister (kRegEnhancedCSC8InOffset0_1, 0x00000000);  // Reg 5569  // Component 0 input offset: 0.0000 (12-bit), 0.00000 (10-bit), Component 1 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC8InOffset2, 0x00000000);  // Reg 5570  // Component 2 input offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC8CoeffA0, 0x00000000);  // Reg 5571  // A0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8CoeffA1, 0x00000000);  // Reg 5572  // A1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8CoeffA2, 0x00000000);  // Reg 5573  // A2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8CoeffB0, 0x00000000);  // Reg 5574  // B0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8CoeffB1, 0x00000000);  // Reg 5575  // B1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8CoeffB2, 0x00000000);  // Reg 5576  // B2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8CoeffC0, 0x00000000);  // Reg 5577  // C0 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8CoeffC1, 0x00000000);  // Reg 5578  // C1 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8CoeffC2, 0x00000000);  // Reg 5579  // C2 coefficient: 0.0000000000 (0x00000000)
	WriteRegister (kRegEnhancedCSC8OutOffsetA_B, 0x00000000);  // Reg 5580  // Component A output offset: 0.0000 (12-bit), 0.00000 (10-bit), Component B output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC8OutOffsetC, 0x00000000);  // Reg 5581  // Component C output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC8KeyMode, 0x00000000);  // Reg 5582  // Key Source Select: Key Input, Key Output Range: Full Range
	WriteRegister (kRegEnhancedCSC8KeyClipOffset, 0x00000000);  // Reg 5583  // Key input offset: 0.00 (12-bit), 0.0000 (10-bit), Key output offset: 0.0000 (12-bit), 0.00000 (10-bit)
	WriteRegister (kRegEnhancedCSC8KeyGain, 0x00000000);  // Reg 5584  // Key gain: 0.000000 (00000000)
	// 1024 registers:
	WriteRegister (kVRegDriverVersion, 0x03C50C08);  // Reg 10000
	WriteRegister (kVRegAudioRecordPinDelay, 0x00000000);  // Reg 10001
	WriteRegister (kVRegRelativeVideoPlaybackDelay, 0x00000000);  // Reg 10002
	WriteRegister (kVRegGlobalAudioPlaybackMode, 0x00000000);  // Reg 10003
	WriteRegister (kVRegFlashProgramKey, 0x00000000);  // Reg 10004
	WriteRegister (kVRegStrictTiming, 0x00000000);  // Reg 10005
	WriteRegister (VIRTUALREG_START+6, 0x00000000);  // Reg 10006
	WriteRegister (VIRTUALREG_START+7, 0x00000000);  // Reg 10007
	WriteRegister (VIRTUALREG_START+8, 0x00000000);  // Reg 10008
	WriteRegister (VIRTUALREG_START+9, 0x00000000);  // Reg 10009
	WriteRegister (VIRTUALREG_START+10, 0x00000000);  // Reg 10010
	WriteRegister (VIRTUALREG_START+11, 0x00000000);  // Reg 10011
	WriteRegister (VIRTUALREG_START+12, 0x00000000);  // Reg 10012
	WriteRegister (VIRTUALREG_START+13, 0x00000000);  // Reg 10013
	WriteRegister (VIRTUALREG_START+14, 0x00000000);  // Reg 10014
	WriteRegister (VIRTUALREG_START+15, 0x00000000);  // Reg 10015
	WriteRegister (VIRTUALREG_START+16, 0x00000000);  // Reg 10016
	WriteRegister (VIRTUALREG_START+17, 0x00000000);  // Reg 10017
	WriteRegister (VIRTUALREG_START+18, 0x00000000);  // Reg 10018
	WriteRegister (VIRTUALREG_START+19, 0x00000000);  // Reg 10019
	WriteRegister (kVRegInputSelect, 0x00000000);  // Reg 10020
	WriteRegister (kVRegSecondaryFormatSelect, 0x00000002);  // Reg 10021
	WriteRegister (kVRegDigitalOutput1Select, 0x00000000);  // Reg 10022
	WriteRegister (kVRegDigitalOutput2Select, 0x00000000);  // Reg 10023
	WriteRegister (kVRegAnalogOutputSelect, 0x00000000);  // Reg 10024
	WriteRegister (kVRegAnalogOutputType, 0x00000001);  // Reg 10025
	WriteRegister (kVRegAnalogOutBlackLevel, 0x00000000);  // Reg 10026
	WriteRegister (kVRegInputSelectUser, 0x00000000);  // Reg 10027
	WriteRegister (VIRTUALREG_START+28, 0x00000000);  // Reg 10028
	WriteRegister (VIRTUALREG_START+29, 0x00000000);  // Reg 10029
	WriteRegister (VIRTUALREG_START+30, 0x00000000);  // Reg 10030
	WriteRegister (VIRTUALREG_START+31, 0x00000000);  // Reg 10031
	WriteRegister (VIRTUALREG_START+32, 0x00000000);  // Reg 10032
	WriteRegister (VIRTUALREG_START+33, 0x00000000);  // Reg 10033
	WriteRegister (VIRTUALREG_START+34, 0x00000000);  // Reg 10034
	WriteRegister (VIRTUALREG_START+35, 0x00000000);  // Reg 10035
	WriteRegister (VIRTUALREG_START+36, 0x00000000);  // Reg 10036
	WriteRegister (VIRTUALREG_START+37, 0x00000000);  // Reg 10037
	WriteRegister (VIRTUALREG_START+38, 0x00000000);  // Reg 10038
	WriteRegister (VIRTUALREG_START+39, 0x00000000);  // Reg 10039
	WriteRegister (kVRegVideoOutPauseMode, 0x00000000);  // Reg 10040
	WriteRegister (kVRegPulldownPattern, 0x00000000);  // Reg 10041
	WriteRegister (kVRegColorSpaceMode, 0x00000000);  // Reg 10042
	WriteRegister (kVRegGammaMode, 0x00000001);  // Reg 10043
	WriteRegister (kVRegLUTType, 0x00000002);  // Reg 10044
	WriteRegister (kVRegRGB10Range, 0x00000001);  // Reg 10045
	WriteRegister (kVRegRGB10Endian, 0x00000001);  // Reg 10046
	WriteRegister (kVRegFanControl, 0x00000000);  // Reg 10047
	WriteRegister (VIRTUALREG_START+48, 0x00000000);  // Reg 10048
	WriteRegister (VIRTUALREG_START+49, 0x00000000);  // Reg 10049
	WriteRegister (kVRegBitFileDownload, 0x00000000);  // Reg 10050
	WriteRegister (kVRegSaveRegistersToRegistry, 0x00000000);  // Reg 10051
	WriteRegister (kVRegRecallRegistersFromRegistry, 0x00000000);  // Reg 10052
	WriteRegister (kVRegClearAllSubscriptions, 0x00000000);  // Reg 10053
	WriteRegister (kVRegRestoreHardwareProcampRegisters, 0x00000000);  // Reg 10054
	WriteRegister (kVRegAcquireReferenceCount, 0x00000000);  // Reg 10055
	WriteRegister (kVRegReleaseReferenceCount, 0x00000000);  // Reg 10056
	WriteRegister (VIRTUALREG_START+57, 0x00000000);  // Reg 10057
	WriteRegister (VIRTUALREG_START+58, 0x00000000);  // Reg 10058
	WriteRegister (VIRTUALREG_START+59, 0x00000000);  // Reg 10059
	WriteRegister (kVRegDTAudioMux0, 0x00000000);  // Reg 10060
	WriteRegister (kVRegDTAudioMux1, 0x00000000);  // Reg 10061
	WriteRegister (kVRegDTAudioMux2, 0x00000000);  // Reg 10062
	WriteRegister (kVRegDTFirmware, 0x00000000);  // Reg 10063
	WriteRegister (kVRegDTVersionAja, 0x00000000);  // Reg 10064
	WriteRegister (kVRegDTVersionDurian, 0x00000000);  // Reg 10065
	WriteRegister (kVRegDTAudioCapturePinConnected, 0x00000000);  // Reg 10066
	WriteRegister (VIRTUALREG_START+67, 0x00000000);  // Reg 10067
	WriteRegister (VIRTUALREG_START+68, 0x00000000);  // Reg 10068
	WriteRegister (VIRTUALREG_START+69, 0x00000000);  // Reg 10069
	WriteRegister (kVRegTimeStampMode, 0x00000000);  // Reg 10070
	WriteRegister (kVRegTimeStampLastOutputVerticalLo, 0x00000000);  // Reg 10071
	WriteRegister (kVRegTimeStampLastOutputVerticalHi, 0x00000000);  // Reg 10072
	WriteRegister (kVRegTimeStampLastInput1VerticalLo, 0x00000000);  // Reg 10073
	WriteRegister (kVRegTimeStampLastInput1VerticalHi, 0x00000000);  // Reg 10074
	WriteRegister (kVRegTimeStampLastInput2VerticalLo, 0x00000000);  // Reg 10075
	WriteRegister (kVRegTimeStampLastInput2VerticalHi, 0x00000000);  // Reg 10076
	WriteRegister (kVRegNumberVideoMappingRegisters, 0x00000000);  // Reg 10077
	WriteRegister (kVRegNumberAudioMappingRegisters, 0x00000000);  // Reg 10078
	WriteRegister (kVRegAudioSyncTolerance, 0x00002710);  // Reg 10079
	WriteRegister (kVRegDmaSerialize, 0x00000000);  // Reg 10080
	WriteRegister (kVRegSyncChannel, 0x00000000);  // Reg 10081
	WriteRegister (kVRegSoftwareUartFifo, 0x00000000);  // Reg 10082
	WriteRegister (kVRegTimeCodeCh1Delay, 0x00000000);  // Reg 10083
	WriteRegister (kVRegTimeCodeCh2Delay, 0x00000000);  // Reg 10084
	WriteRegister (kVRegTimeCodeIn1Delay, 0x00000000);  // Reg 10085
	WriteRegister (kVRegTimeCodeIn2Delay, 0x00000000);  // Reg 10086
	WriteRegister (kVRegTimeCodeCh3Delay, 0x00000000);  // Reg 10087
	WriteRegister (kVRegTimeCodeCh4Delay, 0x00000000);  // Reg 10088
	WriteRegister (kVRegTimeCodeIn3Delay, 0x00000000);  // Reg 10089
	WriteRegister (kVRegTimeCodeIn4Delay, 0x00000000);  // Reg 10090
	WriteRegister (kVRegTimeCodeCh5Delay, 0x00000000);  // Reg 10091
	WriteRegister (kVRegTimeCodeIn5Delay, 0x00000000);  // Reg 10092
	WriteRegister (kVRegTimeCodeCh6Delay, 0x00000000);  // Reg 10093
	WriteRegister (kVRegTimeCodeIn6Delay, 0x00000000);  // Reg 10094
	WriteRegister (kVRegTimeCodeCh7Delay, 0x00000000);  // Reg 10095
	WriteRegister (kVRegTimeCodeIn7Delay, 0x00000000);  // Reg 10096
	WriteRegister (kVRegTimeCodeCh8Delay, 0x00000000);  // Reg 10097
	WriteRegister (kVRegTimeCodeIn8Delay, 0x00000000);  // Reg 10098
	WriteRegister (VIRTUALREG_START+99, 0x00000000);  // Reg 10099
	WriteRegister (kVRegDebug1, 0x00000000);  // Reg 10100
	WriteRegister (kVRegDebugLastFormat, 0x00000000);  // Reg 10101
	WriteRegister (kVRegDebugIPConfigTimeMS, 0x00000000);  // Reg 10102
	WriteRegister (VIRTUALREG_START+103, 0x00000000);  // Reg 10103
	WriteRegister (VIRTUALREG_START+104, 0x00000000);  // Reg 10104
	WriteRegister (VIRTUALREG_START+105, 0x00000000);  // Reg 10105
	WriteRegister (VIRTUALREG_START+106, 0x00000000);  // Reg 10106
	WriteRegister (VIRTUALREG_START+107, 0x00000000);  // Reg 10107
	WriteRegister (VIRTUALREG_START+108, 0x00000000);  // Reg 10108
	WriteRegister (VIRTUALREG_START+109, 0x00000000);  // Reg 10109
	WriteRegister (VIRTUALREG_START+110, 0x00000000);  // Reg 10110
	WriteRegister (VIRTUALREG_START+111, 0x00000000);  // Reg 10111
	WriteRegister (VIRTUALREG_START+112, 0x00000000);  // Reg 10112
	WriteRegister (VIRTUALREG_START+113, 0x00000000);  // Reg 10113
	WriteRegister (VIRTUALREG_START+114, 0x00000000);  // Reg 10114
	WriteRegister (VIRTUALREG_START+115, 0x00000000);  // Reg 10115
	WriteRegister (VIRTUALREG_START+116, 0x00000000);  // Reg 10116
	WriteRegister (VIRTUALREG_START+117, 0x00000000);  // Reg 10117
	WriteRegister (VIRTUALREG_START+118, 0x00000000);  // Reg 10118
	WriteRegister (VIRTUALREG_START+119, 0x00000000);  // Reg 10119
	WriteRegister (kVRegDisplayReferenceSelect, 0x00000000);  // Reg 10120
	WriteRegister (kVRegVANCMode, 0x00000000);  // Reg 10121
	WriteRegister (kVRegDualStreamTransportType, 0x00000000);  // Reg 10122
	WriteRegister (kVRegDSKMode, 0x00000000);  // Reg 10123
	WriteRegister (kVRegIsoConvertEnable, 0x00000000);  // Reg 10124
	WriteRegister (kVRegDSKAudioMode, 0x00000001);  // Reg 10125
	WriteRegister (kVRegDSKForegroundMode, 0x00000000);  // Reg 10126
	WriteRegister (kVRegDSKForegroundFade, 0x00000000);  // Reg 10127
	WriteRegister (kVRegCaptureReferenceSelect, 0x00000002);  // Reg 10128
	WriteRegister (VIRTUALREG_START+129, 0x00000000);  // Reg 10129
	WriteRegister (kVReg2XTransferMode, 0x00000000);  // Reg 10130
	WriteRegister (kVRegSDIOutput1RGBRange, 0x00000000);  // Reg 10131
	WriteRegister (kVRegSDIInput1FormatSelect, 0x00000000);  // Reg 10132
	WriteRegister (kVRegSDIInput2FormatSelect, 0x00000000);  // Reg 10133
	WriteRegister (kVRegSDIInput1RGBRange, 0x00000000);  // Reg 10134
	WriteRegister (kVRegSDIInput2RGBRange, 0x00000002);  // Reg 10135
	WriteRegister (kVRegSDIInput1Stereo3DMode, 0x00000001);  // Reg 10136
	WriteRegister (kVRegSDIInput2Stereo3DMode, 0x00000001);  // Reg 10137
	WriteRegister (kVRegFrameBuffer1RGBRange, 0x00000001);  // Reg 10138
	WriteRegister (kVRegFrameBuffer1Stereo3DMode, 0x00000001);  // Reg 10139
	WriteRegister (kVRegHDMIInRgbRange, 0x00000000);  // Reg 10140
	WriteRegister (kVRegHDMIOutRgbRange, 0x00000000);  // Reg 10141
	WriteRegister (kVRegAnalogInBlackLevel, 0x00000000);  // Reg 10142
	WriteRegister (kVRegAnalogInputType, 0x00000001);  // Reg 10143
	WriteRegister (kVRegHDMIOutColorSpaceModeCtrl, 0x00000000);  // Reg 10144
	WriteRegister (kVRegHDMIOutProtocolMode, 0x00000000);  // Reg 10145
	WriteRegister (kVRegHDMIOutStereoSelect, 0x00000000);  // Reg 10146
	WriteRegister (kVRegHDMIOutStereoCodecSelect, 0x00000000);  // Reg 10147
	WriteRegister (kVRegReversePulldownOffset, 0x00000000);  // Reg 10148
	WriteRegister (kVRegSDIInput1ColorSpaceMode, 0x00000000);  // Reg 10149
	WriteRegister (kVRegSDIInput2ColorSpaceMode, 0x00000001);  // Reg 10150
	WriteRegister (kVRegSDIOutput2RGBRange, 0x00000000);  // Reg 10151
	WriteRegister (kVRegSDIOutput1Stereo3DMode, 0x00000001);  // Reg 10152
	WriteRegister (kVRegSDIOutput2Stereo3DMode, 0x00000001);  // Reg 10153
	WriteRegister (kVRegFrameBuffer2RGBRange, 0x00000001);  // Reg 10154
	WriteRegister (kVRegFrameBuffer2Stereo3DMode, 0x00000001);  // Reg 10155
	WriteRegister (kVRegAudioGainDisable, 0x00000001);  // Reg 10156
	WriteRegister (kVRegDBLAudioEnable, 0x00000000);  // Reg 10157
	WriteRegister (kVRegActiveVideoOutFilter, 0x0000007F);  // Reg 10158
	WriteRegister (kVRegAudioInputMapSelect, 0x00000000);  // Reg 10159
	WriteRegister (kVRegAudioInputDelay, 0x00000000);  // Reg 10160
	WriteRegister (kVRegDSKGraphicFileIndex, 0x00000000);  // Reg 10161
	WriteRegister (kVRegTimecodeBurnInMode, 0x00000000);  // Reg 10162
	WriteRegister (kVRegUseQTTimecode, 0x00000000);  // Reg 10163
	WriteRegister (kVRegAvailable164, 0x00000000);  // Reg 10164
	WriteRegister (kVRegRP188SourceSelect, 0x00000000);  // Reg 10165
	WriteRegister (kVRegQTCodecModeDebug, 0x00000000);  // Reg 10166
	WriteRegister (kVRegHDMIOutColorSpaceModeStatus, 0x00000000);  // Reg 10167
	WriteRegister (kVRegDeviceOnline, 0x00000001);  // Reg 10168
	WriteRegister (kVRegIsDefaultDevice, 0x00000000);  // Reg 10169
	WriteRegister (kVRegDesktopFrameBufferStatus, 0x00000000);  // Reg 10170
	WriteRegister (kVRegSDIOutput1ColorSpaceMode, 0x00000000);  // Reg 10171
	WriteRegister (kVRegSDIOutput2ColorSpaceMode, 0x00000000);  // Reg 10172
	WriteRegister (kVRegAudioOutputDelay, 0x00000000);  // Reg 10173
	WriteRegister (kVRegTimelapseEnable, 0x00000000);  // Reg 10174
	WriteRegister (kVRegTimelapseCaptureValue, 0x00000001);  // Reg 10175
	WriteRegister (kVRegTimelapseCaptureUnits, 0x00000000);  // Reg 10176
	WriteRegister (kVRegTimelapseIntervalValue, 0x00000001);  // Reg 10177
	WriteRegister (kVRegTimelapseIntervalUnits, 0x00000001);  // Reg 10178
	WriteRegister (kVRegFrameBufferInstalled, 0x00000000);  // Reg 10179
	WriteRegister (kVRegAnalogInStandard, 0x00000002);  // Reg 10180
	WriteRegister (kVRegOutputTimecodeOffset, 0x00000000);  // Reg 10181
	WriteRegister (kVRegOutputTimecodeType, 0x00000000);  // Reg 10182
	WriteRegister (kVRegQuicktimeUsingBoard, 0x00000000);  // Reg 10183
	WriteRegister (kVRegApplicationPID, 0x00000000);  // Reg 10184
	WriteRegister (kVRegApplicationCode, 0x00000000);  // Reg 10185
	WriteRegister (kVRegReleaseApplication, 0x00000000);  // Reg 10186
	WriteRegister (kVRegForceApplicationPID, 0x00000000);  // Reg 10187
	WriteRegister (kVRegForceApplicationCode, 0x00000000);  // Reg 10188
	WriteRegister (kVRegIpConfigStreamRefresh, 0x00000000);  // Reg 10189
	WriteRegister (kVRegSDIInput1Raster, 0x00000000);  // Reg 10190
	WriteRegister (kVRegInputChangedCount, 0x00000000);  // Reg 10191
	WriteRegister (kVReg8kOutputTransportSelection, 0x00000000);  // Reg 10192
	WriteRegister (kVRegAnalogIoSelect, 0x00000002);  // Reg 10193
	WriteRegister (VIRTUALREG_START+194, 0x00000000);  // Reg 10194
	WriteRegister (VIRTUALREG_START+195, 0x00000000);  // Reg 10195
	WriteRegister (VIRTUALREG_START+196, 0x00000000);  // Reg 10196
	WriteRegister (VIRTUALREG_START+197, 0x00000000);  // Reg 10197
	WriteRegister (VIRTUALREG_START+198, 0x00000000);  // Reg 10198
	WriteRegister (VIRTUALREG_START+199, 0x00000000);  // Reg 10199
	WriteRegister (kVRegProcAmpSDRegsInitialized, 0x00000000);  // Reg 10200
	WriteRegister (kVRegProcAmpStandardDefBrightness, 0x00000000);  // Reg 10201
	WriteRegister (kVRegProcAmpStandardDefContrast, 0x00000000);  // Reg 10202
	WriteRegister (kVRegProcAmpStandardDefSaturation, 0x00000000);  // Reg 10203
	WriteRegister (kVRegProcAmpStandardDefHue, 0x00000000);  // Reg 10204
	WriteRegister (kVRegProcAmpStandardDefCbOffset, 0x00000000);  // Reg 10205
	WriteRegister (kVRegProcAmpStandardDefCrOffset, 0x00000000);  // Reg 10206
	WriteRegister (kVRegProcAmpEndStandardDefRange, 0x00000000);  // Reg 10207
	WriteRegister (VIRTUALREG_START+208, 0x00000000);  // Reg 10208
	WriteRegister (VIRTUALREG_START+209, 0x00000000);  // Reg 10209
	WriteRegister (VIRTUALREG_START+210, 0x00000000);  // Reg 10210
	WriteRegister (VIRTUALREG_START+211, 0x00000000);  // Reg 10211
	WriteRegister (VIRTUALREG_START+212, 0x00000000);  // Reg 10212
	WriteRegister (VIRTUALREG_START+213, 0x00000000);  // Reg 10213
	WriteRegister (VIRTUALREG_START+214, 0x00000000);  // Reg 10214
	WriteRegister (VIRTUALREG_START+215, 0x00000000);  // Reg 10215
	WriteRegister (VIRTUALREG_START+216, 0x00000000);  // Reg 10216
	WriteRegister (VIRTUALREG_START+217, 0x00000000);  // Reg 10217
	WriteRegister (VIRTUALREG_START+218, 0x00000000);  // Reg 10218
	WriteRegister (VIRTUALREG_START+219, 0x00000000);  // Reg 10219
	WriteRegister (kVRegProcAmpHDRegsInitialized, 0x00000000);  // Reg 10220
	WriteRegister (kVRegProcAmpHighDefBrightness, 0x00000000);  // Reg 10221
	WriteRegister (kVRegProcAmpHighDefContrast, 0x00000000);  // Reg 10222
	WriteRegister (kVRegProcAmpHighDefSaturationCb, 0x00000000);  // Reg 10223
	WriteRegister (kVRegProcAmpHighDefSaturationCr, 0x00000000);  // Reg 10224
	WriteRegister (kVRegProcAmpHighDefHue, 0x00000000);  // Reg 10225
	WriteRegister (kVRegProcAmpHighDefCbOffset, 0x00000000);  // Reg 10226
	WriteRegister (kVRegProcAmpHighDefCrOffset, 0x00000000);  // Reg 10227
	WriteRegister (kVRegProcAmpEndHighDefRange, 0x00000000);  // Reg 10228
	WriteRegister (VIRTUALREG_START+229, 0x00000000);  // Reg 10229
	WriteRegister (VIRTUALREG_START+230, 0x00000000);  // Reg 10230
	WriteRegister (VIRTUALREG_START+231, 0x00000000);  // Reg 10231
	WriteRegister (VIRTUALREG_START+232, 0x00000000);  // Reg 10232
	WriteRegister (VIRTUALREG_START+233, 0x00000000);  // Reg 10233
	WriteRegister (VIRTUALREG_START+234, 0x00000000);  // Reg 10234
	WriteRegister (VIRTUALREG_START+235, 0x00000000);  // Reg 10235
	WriteRegister (VIRTUALREG_START+236, 0x00000000);  // Reg 10236
	WriteRegister (VIRTUALREG_START+237, 0x00000000);  // Reg 10237
	WriteRegister (VIRTUALREG_START+238, 0x00000000);  // Reg 10238
	WriteRegister (VIRTUALREG_START+239, 0x00000000);  // Reg 10239
	WriteRegister (kVRegChannel1UserBufferLevel, 0x00000000);  // Reg 10240
	WriteRegister (kVRegChannel2UserBufferLevel, 0x00000000);  // Reg 10241
	WriteRegister (kVRegInput1UserBufferLevel, 0x00000000);  // Reg 10242
	WriteRegister (kVRegInput2UserBufferLevel, 0x00000000);  // Reg 10243
	WriteRegister (VIRTUALREG_START+244, 0x00000000);  // Reg 10244
	WriteRegister (VIRTUALREG_START+245, 0x00000000);  // Reg 10245
	WriteRegister (VIRTUALREG_START+246, 0x00000000);  // Reg 10246
	WriteRegister (VIRTUALREG_START+247, 0x00000000);  // Reg 10247
	WriteRegister (VIRTUALREG_START+248, 0x00000000);  // Reg 10248
	WriteRegister (VIRTUALREG_START+249, 0x00000000);  // Reg 10249
	WriteRegister (VIRTUALREG_START+250, 0x00000000);  // Reg 10250
	WriteRegister (VIRTUALREG_START+251, 0x00000000);  // Reg 10251
	WriteRegister (VIRTUALREG_START+252, 0x00000000);  // Reg 10252
	WriteRegister (VIRTUALREG_START+253, 0x00000000);  // Reg 10253
	WriteRegister (VIRTUALREG_START+254, 0x00000000);  // Reg 10254
	WriteRegister (VIRTUALREG_START+255, 0x00000000);  // Reg 10255
	WriteRegister (VIRTUALREG_START+256, 0x00000000);  // Reg 10256
	WriteRegister (VIRTUALREG_START+257, 0x00000000);  // Reg 10257
	WriteRegister (VIRTUALREG_START+258, 0x00000000);  // Reg 10258
	WriteRegister (VIRTUALREG_START+259, 0x00000000);  // Reg 10259
	WriteRegister (kVRegProgressivePicture, 0x00000001);  // Reg 10260
	WriteRegister (kVRegLUT2Type, 0x00000002);  // Reg 10261
	WriteRegister (kVRegLUT3Type, 0x00000002);  // Reg 10262
	WriteRegister (kVRegLUT4Type, 0x00000002);  // Reg 10263
	WriteRegister (kVRegDigitalOutput3Select, 0x00000000);  // Reg 10264
	WriteRegister (kVRegDigitalOutput4Select, 0x00000000);  // Reg 10265
	WriteRegister (kVRegHDMIOutputSelect, 0x00000000);  // Reg 10266
	WriteRegister (kVRegRGBRangeConverterLUTType, 0x00000002);  // Reg 10267
	WriteRegister (kVRegTestPatternChoice, 0x0000000C);  // Reg 10268
	WriteRegister (kVRegTestPatternFormat, 0x00000000);  // Reg 10269
	WriteRegister (kVRegEveryFrameTaskFilter, 0x00000001);  // Reg 10270
	WriteRegister (kVRegDefaultInput, 0x00000001);  // Reg 10271
	WriteRegister (kVRegDefaultVideoOutMode, 0x00000004);  // Reg 10272
	WriteRegister (kVRegDefaultVideoFormat, 0x00000065);  // Reg 10273
	WriteRegister (kVRegDigitalOutput5Select, 0x0000000B);  // Reg 10274
	WriteRegister (kVRegLUT5Type, 0x00000002);  // Reg 10275
	WriteRegister (VIRTUALREG_START+276, 0x00000000);  // Reg 10276
	WriteRegister (VIRTUALREG_START+277, 0x00000000);  // Reg 10277
	WriteRegister (VIRTUALREG_START+278, 0x00000000);  // Reg 10278
	WriteRegister (VIRTUALREG_START+279, 0x00000000);  // Reg 10279
	WriteRegister (VIRTUALREG_START+280, 0x00000000);  // Reg 10280
	WriteRegister (VIRTUALREG_START+281, 0x00000000);  // Reg 10281
	WriteRegister (VIRTUALREG_START+282, 0x00000000);  // Reg 10282
	WriteRegister (VIRTUALREG_START+283, 0x00000000);  // Reg 10283
	WriteRegister (VIRTUALREG_START+284, 0x00000000);  // Reg 10284
	WriteRegister (VIRTUALREG_START+285, 0x00000000);  // Reg 10285
	WriteRegister (VIRTUALREG_START+286, 0x00000000);  // Reg 10286
	WriteRegister (VIRTUALREG_START+287, 0x00000000);  // Reg 10287
	WriteRegister (VIRTUALREG_START+288, 0x00000000);  // Reg 10288
	WriteRegister (VIRTUALREG_START+289, 0x00000000);  // Reg 10289
	WriteRegister (VIRTUALREG_START+290, 0x00000000);  // Reg 10290
	WriteRegister (VIRTUALREG_START+291, 0x00000000);  // Reg 10291
	WriteRegister (VIRTUALREG_START+292, 0x00000000);  // Reg 10292
	WriteRegister (VIRTUALREG_START+293, 0x00000000);  // Reg 10293
	WriteRegister (VIRTUALREG_START+294, 0x00000000);  // Reg 10294
	WriteRegister (VIRTUALREG_START+295, 0x00000000);  // Reg 10295
	WriteRegister (VIRTUALREG_START+296, 0x00000000);  // Reg 10296
	WriteRegister (VIRTUALREG_START+297, 0x00000000);  // Reg 10297
	WriteRegister (VIRTUALREG_START+298, 0x00000000);  // Reg 10298
	WriteRegister (VIRTUALREG_START+299, 0x00000000);  // Reg 10299
	WriteRegister (kVRegMacUserModeDebugLevel, 0x00000003);  // Reg 10300
	WriteRegister (kVRegMacKernelModeDebugLevel, 0x00000000);  // Reg 10301
	WriteRegister (kVRegMacUserModePingLevel, 0x00000003);  // Reg 10302
	WriteRegister (kVRegMacKernelModePingLevel, 0x00000000);  // Reg 10303
	WriteRegister (kVRegLatencyTimerValue, 0x00000000);  // Reg 10304
	WriteRegister (VIRTUALREG_START+305, 0x00000000);  // Reg 10305
	WriteRegister (kVRegAudioInputSelect, 0x00000002);  // Reg 10306
	WriteRegister (kVRegSerialSuspended, 0x00000000);  // Reg 10307
	WriteRegister (kVRegXilinxProgramming, 0x00000000);  // Reg 10308
	WriteRegister (kVRegETTDiagLastSerialTimestamp, 0x00000000);  // Reg 10309
	WriteRegister (kVRegETTDiagLastSerialTimecode, 0x00000000);  // Reg 10310
	WriteRegister (kVRegStartupStatusFlags, 0x00000007);  // Reg 10311
	WriteRegister (kVRegRGBRangeMode, 0x00000001);  // Reg 10312
	WriteRegister (kVRegEnableQueuedDMAs, 0x00000000);  // Reg 10313
	WriteRegister (VIRTUALREG_START+314, 0x00000000);  // Reg 10314
	WriteRegister (VIRTUALREG_START+315, 0x00000000);  // Reg 10315
	WriteRegister (VIRTUALREG_START+316, 0x00000000);  // Reg 10316
	WriteRegister (VIRTUALREG_START+317, 0x00000000);  // Reg 10317
	WriteRegister (VIRTUALREG_START+318, 0x00000000);  // Reg 10318
	WriteRegister (VIRTUALREG_START+319, 0x00000000);  // Reg 10319
	WriteRegister (kVRegBA0MemorySize, 0x00000000);  // Reg 10320
	WriteRegister (kVRegBA1MemorySize, 0x00000000);  // Reg 10321
	WriteRegister (kVRegBA4MemorySize, 0x00000000);  // Reg 10322
	WriteRegister (kVRegNumDmaDriverBuffers, 0x00000000);  // Reg 10323
	WriteRegister (kVRegDMADriverBufferPhysicalAddress, 0x00000000);  // Reg 10324
	WriteRegister (kVRegBA2MemorySize, 0x00000000);  // Reg 10325
	WriteRegister (kVRegAcquireLinuxReferenceCount, 0x00000000);  // Reg 10326
	WriteRegister (kVRegReleaseLinuxReferenceCount, 0x00000000);  // Reg 10327
	WriteRegister (VIRTUALREG_START+328, 0x00000000);  // Reg 10328
	WriteRegister (VIRTUALREG_START+329, 0x00000000);  // Reg 10329
	WriteRegister (VIRTUALREG_START+330, 0x00000000);  // Reg 10330
	WriteRegister (VIRTUALREG_START+331, 0x00000000);  // Reg 10331
	WriteRegister (VIRTUALREG_START+332, 0x00000000);  // Reg 10332
	WriteRegister (VIRTUALREG_START+333, 0x00000000);  // Reg 10333
	WriteRegister (VIRTUALREG_START+334, 0x00000000);  // Reg 10334
	WriteRegister (VIRTUALREG_START+335, 0x00000000);  // Reg 10335
	WriteRegister (VIRTUALREG_START+336, 0x00000000);  // Reg 10336
	WriteRegister (VIRTUALREG_START+337, 0x00000000);  // Reg 10337
	WriteRegister (VIRTUALREG_START+338, 0x00000000);  // Reg 10338
	WriteRegister (VIRTUALREG_START+339, 0x00000000);  // Reg 10339
	WriteRegister (kVRegAdvancedIndexing, 0x00000001);  // Reg 10340
	WriteRegister (kVRegTimeStampLastInput3VerticalLo, 0x00000000);  // Reg 10341
	WriteRegister (kVRegTimeStampLastInput3VerticalHi, 0x00000000);  // Reg 10342
	WriteRegister (kVRegTimeStampLastInput4VerticalLo, 0x00000000);  // Reg 10343
	WriteRegister (kVRegTimeStampLastInput4VerticalHi, 0x00000000);  // Reg 10344
	WriteRegister (kVRegTimeStampLastInput5VerticalLo, 0x00000000);  // Reg 10345
	WriteRegister (kVRegTimeStampLastInput5VerticalHi, 0x00000000);  // Reg 10346
	WriteRegister (kVRegTimeStampLastInput6VerticalLo, 0x00000000);  // Reg 10347
	WriteRegister (kVRegTimeStampLastInput6VerticalHi, 0x00000000);  // Reg 10348
	WriteRegister (kVRegTimeStampLastInput7VerticalLo, 0x00000000);  // Reg 10349
	WriteRegister (kVRegTimeStampLastInput7VerticalHi, 0x00000000);  // Reg 10350
	WriteRegister (kVRegTimeStampLastInput8VerticalLo, 0x00000000);  // Reg 10351
	WriteRegister (kVRegTimeStampLastInput8VerticalHi, 0x00000000);  // Reg 10352
	WriteRegister (kVRegTimeStampLastOutput2VerticalLo, 0x00000000);  // Reg 10353
	WriteRegister (kVRegTimeStampLastOutput2VerticalHi, 0x00000000);  // Reg 10354
	WriteRegister (kVRegTimeStampLastOutput3VerticalLo, 0x00000000);  // Reg 10355
	WriteRegister (kVRegTimeStampLastOutput3VerticalHi, 0x00000000);  // Reg 10356
	WriteRegister (kVRegTimeStampLastOutput4VerticalLo, 0x00000000);  // Reg 10357
	WriteRegister (kVRegTimeStampLastOutput4VerticalHi, 0x00000000);  // Reg 10358
	WriteRegister (kVRegTimeStampLastOutput5VerticalLo, 0x00000000);  // Reg 10359
	WriteRegister (kVRegTimeStampLastOutput5VerticalHi, 0x00000000);  // Reg 10360
	WriteRegister (kVRegTimeStampLastOutput6VerticalLo, 0x00000000);  // Reg 10361
	WriteRegister (kVRegTimeStampLastOutput6VerticalHi, 0x00000000);  // Reg 10362
	WriteRegister (kVRegTimeStampLastOutput7VerticalLo, 0x00000000);  // Reg 10363
	WriteRegister (kVRegTimeStampLastOutput7VerticalHi, 0x00000000);  // Reg 10364
	WriteRegister (kVRegTimeStampLastOutput8VerticalLo, 0x00000000);  // Reg 10365
	WriteRegister (kVRegResetCycleCount, 0x00000001);  // Reg 10366
	WriteRegister (kVRegUseProgressive, 0x00000001);  // Reg 10367
	WriteRegister (kVRegFlashSize, 0x00000000);  // Reg 10368
	WriteRegister (kVRegFlashStatus, 0x00000000);  // Reg 10369
	WriteRegister (kVRegFlashState, 0x00000000);  // Reg 10370
	WriteRegister (kVRegPCIDeviceID, 0x00000000);  // Reg 10371
	WriteRegister (kVRegUartRxFifoSize, 0x00000000);  // Reg 10372
	WriteRegister (kVRegEFTNeedsUpdating, 0x00000001);  // Reg 10373
	WriteRegister (kVRegSuspendSystemAudio, 0x00000000);  // Reg 10374
	WriteRegister (kVRegAcquireReferenceCounter, 0x00000000);  // Reg 10375
	WriteRegister (kVRegTimeStampLastOutput8VerticalHi, 0x00000000);  // Reg 10376
	WriteRegister (kVRegFramesPerVertical, 0x00000000);  // Reg 10377
	WriteRegister (kVRegServicesInitialized, 0x00000001);  // Reg 10378
	WriteRegister (kVRegFrameBufferGangCount, 0x00000000);  // Reg 10379
	WriteRegister (kVRegChannelCrosspointFirst, 0x00000012);  // Reg 10380
	WriteRegister (VIRTUALREG_START+381, 0x00000012);  // Reg 10381
	WriteRegister (VIRTUALREG_START+382, 0x00000012);  // Reg 10382
	WriteRegister (VIRTUALREG_START+383, 0x00000012);  // Reg 10383
	WriteRegister (VIRTUALREG_START+384, 0x00000012);  // Reg 10384
	WriteRegister (VIRTUALREG_START+385, 0x00000012);  // Reg 10385
	WriteRegister (VIRTUALREG_START+386, 0x00000012);  // Reg 10386
	WriteRegister (kVRegChannelCrosspointLast, 0x00000012);  // Reg 10387
	WriteRegister (VIRTUALREG_START+388, 0x00000000);  // Reg 10388
	WriteRegister (kVRegMonAncField1Offset, 0x00004000);  // Reg 10389
	WriteRegister (kVRegMonAncField2Offset, 0x00002000);  // Reg 10390
	WriteRegister (kVRegFollowInputFormat, 0x00000000);  // Reg 10391
	WriteRegister (kVRegAncField1Offset, 0x00004000);  // Reg 10392
	WriteRegister (kVRegAncField2Offset, 0x00002000);  // Reg 10393
	WriteRegister (kVRegAgentCheck, 0x00000007);  // Reg 10394
	WriteRegister (kVRegUnused_2, 0x00000000);  // Reg 10395
	WriteRegister (kVReg4kOutputTransportSelection, 0x00000002);  // Reg 10396
	WriteRegister (kVRegCustomAncInputSelect, 0x00000000);  // Reg 10397
	WriteRegister (kVRegUseThermostat, 0x00000000);  // Reg 10398
	WriteRegister (kVRegThermalSamplingRate, 0x00000000);  // Reg 10399
	WriteRegister (kVRegFanSpeed, 0x00000000);  // Reg 10400
	WriteRegister (kVRegVideoFormatCh1, 0x00000065);  // Reg 10401
	WriteRegister (kVRegVideoFormatCh2, 0x00000065);  // Reg 10402
	WriteRegister (kVRegVideoFormatCh3, 0x00000065);  // Reg 10403
	WriteRegister (kVRegVideoFormatCh4, 0x00000065);  // Reg 10404
	WriteRegister (kVRegVideoFormatCh5, 0x00000065);  // Reg 10405
	WriteRegister (kVRegVideoFormatCh6, 0x00000065);  // Reg 10406
	WriteRegister (kVRegVideoFormatCh7, 0x00000065);  // Reg 10407
	WriteRegister (kVRegVideoFormatCh8, 0x00000065);  // Reg 10408
	WriteRegister (kVRegIPAddrEth0, 0x00000000);  // Reg 10409
	WriteRegister (kVRegSubnetEth0, 0x00000000);  // Reg 10410
	WriteRegister (kVRegGatewayEth0, 0x00000000);  // Reg 10411
	WriteRegister (kVRegIPAddrEth1, 0x00000000);  // Reg 10412
	WriteRegister (kVRegSubnetEth1, 0x00000000);  // Reg 10413
	WriteRegister (kVRegGatewayEth1, 0x00000000);  // Reg 10414
	WriteRegister (kVRegRxcEnable1, 0x00000000);  // Reg 10415
	WriteRegister (kVRegRxcSfp1RxMatch1, 0x00000000);  // Reg 10416
	WriteRegister (kVRegRxcSfp1SourceIp1, 0x00000000);  // Reg 10417
	WriteRegister (kVRegRxcSfp1DestIp1, 0x00000000);  // Reg 10418
	WriteRegister (kVRegRxcSfp1SourcePort1, 0x00000000);  // Reg 10419
	WriteRegister (kVRegRxcSfp1DestPort1, 0x00000000);  // Reg 10420
	WriteRegister (kVRegRxcSfp1Vlan1, 0x00000000);  // Reg 10421
	WriteRegister (kVRegRxcSfp2RxMatch1, 0x00000000);  // Reg 10422
	WriteRegister (kVRegRxcSfp2SourceIp1, 0x00000000);  // Reg 10423
	WriteRegister (kVRegRxcSfp2DestIp1, 0x00000000);  // Reg 10424
	WriteRegister (kVRegRxcSfp2SourcePort1, 0x00000000);  // Reg 10425
	WriteRegister (kVRegRxcSfp2DestPort1, 0x00000000);  // Reg 10426
	WriteRegister (kVRegRxcSfp2Vlan1, 0x00000000);  // Reg 10427
	WriteRegister (kVRegRxcSsrc1, 0x00000000);  // Reg 10428
	WriteRegister (kVRegRxcPlayoutDelay1, 0x00000000);  // Reg 10429
	WriteRegister (kVRegRxcEnable2, 0x00000000);  // Reg 10430
	WriteRegister (kVRegRxcSfp1RxMatch2, 0x00000000);  // Reg 10431
	WriteRegister (kVRegRxcSfp1SourceIp2, 0x00000000);  // Reg 10432
	WriteRegister (kVRegRxcSfp1DestIp2, 0x00000000);  // Reg 10433
	WriteRegister (kVRegRxcSfp1SourcePort2, 0x00000000);  // Reg 10434
	WriteRegister (kVRegRxcSfp1DestPort2, 0x00000000);  // Reg 10435
	WriteRegister (kVRegRxcSfp1Vlan2, 0x00000000);  // Reg 10436
	WriteRegister (kVRegRxcSfp2RxMatch2, 0x00000000);  // Reg 10437
	WriteRegister (kVRegRxcSfp2SourceIp2, 0x00000000);  // Reg 10438
	WriteRegister (kVRegRxcSfp2DestIp2, 0x00000000);  // Reg 10439
	WriteRegister (kVRegRxcSfp2SourcePort2, 0x00000000);  // Reg 10440
	WriteRegister (kVRegRxcSfp2DestPort2, 0x00000000);  // Reg 10441
	WriteRegister (kVRegRxcSfp2Vlan2, 0x00000000);  // Reg 10442
	WriteRegister (kVRegRxcSsrc2, 0x00000000);  // Reg 10443
	WriteRegister (kVRegRxcPlayoutDelay2, 0x00000000);  // Reg 10444
	WriteRegister (kVRegTxcEnable3, 0x00000000);  // Reg 10445
	WriteRegister (kVRegTxcSfp1LocalPort3, 0x00000000);  // Reg 10446
	WriteRegister (kVRegTxcSfp1RemoteIp3, 0x00000000);  // Reg 10447
	WriteRegister (kVRegTxcSfp1RemotePort3, 0x00000000);  // Reg 10448
	WriteRegister (kVRegTxcSfp2LocalPort3, 0x00000000);  // Reg 10449
	WriteRegister (kVRegTxcSfp2RemoteIp3, 0x00000000);  // Reg 10450
	WriteRegister (kVRegTxcSfp2RemotePort3, 0x00000000);  // Reg 10451
	WriteRegister (kVRegTxcEnable4, 0x00000000);  // Reg 10452
	WriteRegister (kVRegTxcSfp1LocalPort4, 0x00000000);  // Reg 10453
	WriteRegister (kVRegTxcSfp1RemoteIp4, 0x00000000);  // Reg 10454
	WriteRegister (kVRegTxcSfp1RemotePort4, 0x00000000);  // Reg 10455
	WriteRegister (kVRegTxcSfp2LocalPort4, 0x00000000);  // Reg 10456
	WriteRegister (kVRegTxcSfp2RemoteIp4, 0x00000000);  // Reg 10457
	WriteRegister (kVRegTxcSfp2RemotePort4, 0x00000000);  // Reg 10458
	WriteRegister (kVRegMailBoxAcquire, 0x00000001);  // Reg 10459
	WriteRegister (kVRegMailBoxRelease, 0x00000001);  // Reg 10460
	WriteRegister (kVRegMailBoxAbort, 0x00000001);  // Reg 10461
	WriteRegister (kVRegMailBoxTimeoutNS, 0x00000000);  // Reg 10462
	WriteRegister (kVRegRxc_2DecodeSelectionMode1, 0x00000000);  // Reg 10463
	WriteRegister (kVRegRxc_2DecodeProgramNumber1, 0x00000000);  // Reg 10464
	WriteRegister (kVRegRxc_2DecodeProgramPID1, 0x00000000);  // Reg 10465
	WriteRegister (kVRegRxc_2DecodeAudioNumber1, 0x00000000);  // Reg 10466
	WriteRegister (kVRegRxc_2DecodeSelectionMode2, 0x00000000);  // Reg 10467
	WriteRegister (kVRegRxc_2DecodeProgramNumber2, 0x00000000);  // Reg 10468
	WriteRegister (kVRegRxc_2DecodeProgramPID2, 0x00000000);  // Reg 10469
	WriteRegister (kVRegRxc_2DecodeAudioNumber2, 0x00000000);  // Reg 10470
	WriteRegister (kVRegTxc_2EncodeVideoFormat1, 0x00000000);  // Reg 10471
	WriteRegister (kVRegTxc_2EncodeUllMode1, 0x00000000);  // Reg 10472
	WriteRegister (kVRegTxc_2EncodeBitDepth1, 0x00000000);  // Reg 10473
	WriteRegister (kVRegTxc_2EncodeChromaSubSamp1, 0x00000000);  // Reg 10474
	WriteRegister (kVRegTxc_2EncodeMbps1, 0x00000000);  // Reg 10475
	WriteRegister (kVRegTxc_2EncodeAudioChannels1, 0x00000000);  // Reg 10476
	WriteRegister (kVRegTxc_2EncodeStreamType1, 0x00000000);  // Reg 10477
	WriteRegister (kVRegTxc_2EncodeProgramPid1, 0x00000000);  // Reg 10478
	WriteRegister (kVRegTxc_2EncodeVideoPid1, 0x00000000);  // Reg 10479
	WriteRegister (kVRegTxc_2EncodePcrPid1, 0x00000000);  // Reg 10480
	WriteRegister (kVRegTxc_2EncodeAudio1Pid1, 0x00000000);  // Reg 10481
	WriteRegister (kVRegTxc_2EncodeVideoFormat2, 0x00000000);  // Reg 10482
	WriteRegister (kVRegTxc_2EncodeUllMode2, 0x00000000);  // Reg 10483
	WriteRegister (kVRegTxc_2EncodeBitDepth2, 0x00000000);  // Reg 10484
	WriteRegister (kVRegTxc_2EncodeChromaSubSamp2, 0x00000000);  // Reg 10485
	WriteRegister (kVRegTxc_2EncodeMbps2, 0x00000000);  // Reg 10486
	WriteRegister (kVRegTxc_2EncodeAudioChannels2, 0x00000000);  // Reg 10487
	WriteRegister (kVRegTxc_2EncodeStreamType2, 0x00000000);  // Reg 10488
	WriteRegister (kVRegTxc_2EncodeProgramPid2, 0x00000000);  // Reg 10489
	WriteRegister (kVRegTxc_2EncodeVideoPid2, 0x00000000);  // Reg 10490
	WriteRegister (kVRegTxc_2EncodePcrPid2, 0x00000000);  // Reg 10491
	WriteRegister (kVRegTxc_2EncodeAudio1Pid2, 0x00000000);  // Reg 10492
	WriteRegister (kVReg2022_7Enable, 0x00000000);  // Reg 10493
	WriteRegister (kVReg2022_7NetworkPathDiff, 0x00000000);  // Reg 10494
	WriteRegister (kVRegKIPRxCfgError, 0x00000000);  // Reg 10495
	WriteRegister (kVRegKIPTxCfgError, 0x00000000);  // Reg 10496
	WriteRegister (kVRegKIPEncCfgError, 0x00000000);  // Reg 10497
	WriteRegister (kVRegKIPDecCfgError, 0x00000000);  // Reg 10498
	WriteRegister (kVRegKIPNetCfgError, 0x00000000);  // Reg 10499
	WriteRegister (kVRegUseHDMI420Mode, 0x00000000);  // Reg 10500
	WriteRegister (kVRegUnused501, 0x00000000);  // Reg 10501
	WriteRegister (kVRegUserDefinedDBB, 0x00000000);  // Reg 10502
	WriteRegister (kVRegHDMIOutAudioChannels, 0x00000000);  // Reg 10503
	WriteRegister (VIRTUALREG_START+504, 0x00000000);  // Reg 10504
	WriteRegister (kVRegZeroHostAncPostCapture, 0x00000000);  // Reg 10505
	WriteRegister (kVRegZeroDeviceAncPostCapture, 0x00000000);  // Reg 10506
	WriteRegister (kVRegAudioMonitorChannelSelect, 0x00000000);  // Reg 10507
	WriteRegister (kVRegAudioMixerOverrideState, 0x00000000);  // Reg 10508
	WriteRegister (kVRegAudioMixerSourceMainEnable, 0x00000001);  // Reg 10509
	WriteRegister (kVRegAudioMixerSourceAux1Enable, 0x00000001);  // Reg 10510
	WriteRegister (kVRegAudioMixerSourceAux2Enable, 0x00000000);  // Reg 10511
	WriteRegister (kVRegAudioMixerSourceMainGain, 0x00010000);  // Reg 10512
	WriteRegister (kVRegAudioMixerSourceAux1Gain, 0x00010000);  // Reg 10513
	WriteRegister (kVRegAudioMixerSourceAux2Gain, 0x00010000);  // Reg 10514
	WriteRegister (kVRegAudioCapMixerSourceMainEnable, 0x00000001);  // Reg 10515
	WriteRegister (kVRegAudioCapMixerSourceAux1Enable, 0x00000000);  // Reg 10516
	WriteRegister (kVRegAudioCapMixerSourceAux2Enable, 0x00000000);  // Reg 10517
	WriteRegister (kVRegAudioCapMixerSourceMainGain, 0x00010000);  // Reg 10518
	WriteRegister (kVRegAudioCapMixerSourceAux1Gain, 0x00010000);  // Reg 10519
	WriteRegister (kVRegAudioCapMixerSourceAux2Gain, 0x00010000);  // Reg 10520
	WriteRegister (kVRegSwizzle4kInput, 0x00000001);  // Reg 10521
	WriteRegister (kVRegSwizzle4kOutput, 0x00000000);  // Reg 10522
	WriteRegister (kVRegAnalogAudioIOConfiguration, 0x00000000);  // Reg 10523
	WriteRegister (kVRegHdmiHdrOutChanged, 0x00000000);  // Reg 10524
	WriteRegister (kVRegDisableAutoVPID, 0x00000000);  // Reg 10525
	WriteRegister (kVRegEnableBT2020, 0x00000000);  // Reg 10526
	WriteRegister (kVRegHdmiHdrOutMode, 0x00000000);  // Reg 10527
	WriteRegister (kVRegServicesForceInit, 0x00000000);  // Reg 10528
	WriteRegister (kVRegServicesModeFinal, 0x00000000);  // Reg 10529
	WriteRegister (kVRegNTV2VPIDTransferCharacteristics, 0x00000000);  // Reg 10530
	WriteRegister (kVRegNTV2VPIDColorimetry, 0x00000000);  // Reg 10531
	WriteRegister (kVRegNTV2VPIDLuminance, 0x00000000);  // Reg 10532
	WriteRegister (kVRegNTV2VPIDTransferCharacteristics2, 0x00000000);  // Reg 10533
	WriteRegister (kVRegNTV2VPIDColorimetry2, 0x00000000);  // Reg 10534
	WriteRegister (kVRegNTV2VPIDLuminance2, 0x00000000);  // Reg 10535
	WriteRegister (kVRegNTV2VPIDTransferCharacteristics3, 0x00000000);  // Reg 10536
	WriteRegister (kVRegNTV2VPIDColorimetry3, 0x00000000);  // Reg 10537
	WriteRegister (kVRegNTV2VPIDLuminance3, 0x00000000);  // Reg 10538
	WriteRegister (kVRegNTV2VPIDTransferCharacteristics4, 0x00000000);  // Reg 10539
	WriteRegister (kVRegNTV2VPIDColorimetry4, 0x00000000);  // Reg 10540
	WriteRegister (kVRegNTV2VPIDLuminance4, 0x00000000);  // Reg 10541
	WriteRegister (kVRegNTV2VPIDTransferCharacteristics5, 0x00000000);  // Reg 10542
	WriteRegister (kVRegNTV2VPIDColorimetry5, 0x00000000);  // Reg 10543
	WriteRegister (kVRegNTV2VPIDLuminance5, 0x00000000);  // Reg 10544
	WriteRegister (kVRegNTV2VPIDTransferCharacteristics6, 0x00000000);  // Reg 10545
	WriteRegister (kVRegNTV2VPIDColorimetry6, 0x00000000);  // Reg 10546
	WriteRegister (kVRegNTV2VPIDLuminance6, 0x00000000);  // Reg 10547
	WriteRegister (kVRegNTV2VPIDTransferCharacteristics7, 0x00000000);  // Reg 10548
	WriteRegister (kVRegNTV2VPIDColorimetry7, 0x00000000);  // Reg 10549
	WriteRegister (kVRegNTV2VPIDLuminance7, 0x00000000);  // Reg 10550
	WriteRegister (kVRegNTV2VPIDTransferCharacteristics8, 0x00000000);  // Reg 10551
	WriteRegister (kVRegNTV2VPIDColorimetry8, 0x00000000);  // Reg 10552
	WriteRegister (kVRegNTV2VPIDLuminance8, 0x00000000);  // Reg 10553
	WriteRegister (kVRegUserColorimetry, 0x00000000);  // Reg 10554
	WriteRegister (kVRegUserTransfer, 0x00000000);  // Reg 10555
	WriteRegister (kVRegUserLuminance, 0x00000000);  // Reg 10556
	WriteRegister (kVRegHdrColorimetryCh1, 0x0000FFFF);  // Reg 10557
	WriteRegister (kVRegHdrTransferCh1, 0x0000FFFF);  // Reg 10558
	WriteRegister (kVRegHdrLuminanceCh1, 0x0000FFFF);  // Reg 10559
	WriteRegister (kVRegHdrGreenXCh1, 0x0000FFFF);  // Reg 10560
	WriteRegister (kVRegHdrGreenYCh1, 0x0000FFFF);  // Reg 10561
	WriteRegister (kVRegHdrBlueXCh1, 0x0000FFFF);  // Reg 10562
	WriteRegister (kVRegHdrBlueYCh1, 0x0000FFFF);  // Reg 10563
	WriteRegister (kVRegHdrRedXCh1, 0x0000FFFF);  // Reg 10564
	WriteRegister (kVRegHdrRedYCh1, 0x0000FFFF);  // Reg 10565
	WriteRegister (kVRegHdrWhiteXCh1, 0x0000FFFF);  // Reg 10566
	WriteRegister (kVRegHdrWhiteYCh1, 0x0000FFFF);  // Reg 10567
	WriteRegister (kVRegHdrMasterLumMaxCh1, 0x0000FFFF);  // Reg 10568
	WriteRegister (kVRegHdrMasterLumMinCh1, 0x0000FFFF);  // Reg 10569
	WriteRegister (kVRegHdrMaxCLLCh1, 0x0000FFFF);  // Reg 10570
	WriteRegister (kVRegHdrMaxFALLCh1, 0x0000FFFF);  // Reg 10571
	WriteRegister (kVRegHDROverrideState, 0x00000000);  // Reg 10572
	WriteRegister (kVRegPCIMaxReadRequestSize, 0x00000000);  // Reg 10573
	WriteRegister (kVRegUserInColorimetry, 0x00000000);  // Reg 10574
	WriteRegister (kVRegUserInTransfer, 0x00000000);  // Reg 10575
	WriteRegister (kVRegUserInLuminance, 0x00000000);  // Reg 10576
	WriteRegister (kVRegHdrInColorimetryCh1, 0x00000000);  // Reg 10577
	WriteRegister (kVRegHdrInTransferCh1, 0x00000000);  // Reg 10578
	WriteRegister (kVRegHdrInLuminanceCh1, 0x00000000);  // Reg 10579
	WriteRegister (kVRegHdrInGreenXCh1, 0x00000000);  // Reg 10580
	WriteRegister (kVRegHdrInGreenYCh1, 0x00000000);  // Reg 10581
	WriteRegister (kVRegHdrInBlueXCh1, 0x00000000);  // Reg 10582
	WriteRegister (kVRegHdrInBlueYCh1, 0x00000000);  // Reg 10583
	WriteRegister (kVRegHdrInRedXCh1, 0x00000000);  // Reg 10584
	WriteRegister (kVRegHdrInRedYCh1, 0x00000000);  // Reg 10585
	WriteRegister (kVRegHdrInWhiteXCh1, 0x00000000);  // Reg 10586
	WriteRegister (kVRegHdrInWhiteYCh1, 0x00000000);  // Reg 10587
	WriteRegister (kVRegHdrInMasterLumMaxCh1, 0x00000000);  // Reg 10588
	WriteRegister (kVRegHdrInMasterLumMinCh1, 0x00000000);  // Reg 10589
	WriteRegister (kVRegHdrInMaxCLLCh1, 0x00000000);  // Reg 10590
	WriteRegister (kVRegHdrInMaxFALLCh1, 0x00000000);  // Reg 10591
	WriteRegister (kVRegHDRInOverrideState, 0x00000000);  // Reg 10592
	WriteRegister (kVRegLastAJA, 0x00000000);  // Reg 10593
	WriteRegister (kVRegFirstOEM, 0x00000000);  // Reg 10594
	WriteRegister (VIRTUALREG_START+595, 0x00000000);  // Reg 10595
	WriteRegister (VIRTUALREG_START+596, 0x00000000);  // Reg 10596
	WriteRegister (VIRTUALREG_START+597, 0x00000000);  // Reg 10597
	WriteRegister (VIRTUALREG_START+598, 0x00000000);  // Reg 10598
	WriteRegister (VIRTUALREG_START+599, 0x00000000);  // Reg 10599
	WriteRegister (VIRTUALREG_START+600, 0x00000000);  // Reg 10600
	WriteRegister (VIRTUALREG_START+601, 0x00000000);  // Reg 10601
	WriteRegister (VIRTUALREG_START+602, 0x00000000);  // Reg 10602
	WriteRegister (VIRTUALREG_START+603, 0x00000000);  // Reg 10603
	WriteRegister (VIRTUALREG_START+604, 0x00000000);  // Reg 10604
	WriteRegister (VIRTUALREG_START+605, 0x00000000);  // Reg 10605
	WriteRegister (VIRTUALREG_START+606, 0x00000000);  // Reg 10606
	WriteRegister (VIRTUALREG_START+607, 0x00000000);  // Reg 10607
	WriteRegister (VIRTUALREG_START+608, 0x00000000);  // Reg 10608
	WriteRegister (VIRTUALREG_START+609, 0x00000000);  // Reg 10609
	WriteRegister (VIRTUALREG_START+610, 0x00000000);  // Reg 10610
	WriteRegister (VIRTUALREG_START+611, 0x00000000);  // Reg 10611
	WriteRegister (VIRTUALREG_START+612, 0x00000000);  // Reg 10612
	WriteRegister (VIRTUALREG_START+613, 0x00000000);  // Reg 10613
	WriteRegister (VIRTUALREG_START+614, 0x00000000);  // Reg 10614
	WriteRegister (VIRTUALREG_START+615, 0x00000000);  // Reg 10615
	WriteRegister (VIRTUALREG_START+616, 0x00000000);  // Reg 10616
	WriteRegister (VIRTUALREG_START+617, 0x00000000);  // Reg 10617
	WriteRegister (VIRTUALREG_START+618, 0x00000000);  // Reg 10618
	WriteRegister (VIRTUALREG_START+619, 0x00000000);  // Reg 10619
	WriteRegister (VIRTUALREG_START+620, 0x00000000);  // Reg 10620
	WriteRegister (VIRTUALREG_START+621, 0x00000000);  // Reg 10621
	WriteRegister (VIRTUALREG_START+622, 0x00000000);  // Reg 10622
	WriteRegister (VIRTUALREG_START+623, 0x00000000);  // Reg 10623
	WriteRegister (VIRTUALREG_START+624, 0x00000000);  // Reg 10624
	WriteRegister (VIRTUALREG_START+625, 0x00000000);  // Reg 10625
	WriteRegister (VIRTUALREG_START+626, 0x00000000);  // Reg 10626
	WriteRegister (VIRTUALREG_START+627, 0x00000000);  // Reg 10627
	WriteRegister (VIRTUALREG_START+628, 0x00000000);  // Reg 10628
	WriteRegister (VIRTUALREG_START+629, 0x00000000);  // Reg 10629
	WriteRegister (VIRTUALREG_START+630, 0x00000000);  // Reg 10630
	WriteRegister (VIRTUALREG_START+631, 0x00000000);  // Reg 10631
	WriteRegister (VIRTUALREG_START+632, 0x00000000);  // Reg 10632
	WriteRegister (VIRTUALREG_START+633, 0x00000000);  // Reg 10633
	WriteRegister (VIRTUALREG_START+634, 0x00000000);  // Reg 10634
	WriteRegister (VIRTUALREG_START+635, 0x00000000);  // Reg 10635
	WriteRegister (VIRTUALREG_START+636, 0x00000000);  // Reg 10636
	WriteRegister (VIRTUALREG_START+637, 0x00000000);  // Reg 10637
	WriteRegister (VIRTUALREG_START+638, 0x00000000);  // Reg 10638
	WriteRegister (VIRTUALREG_START+639, 0x00000000);  // Reg 10639
	WriteRegister (VIRTUALREG_START+640, 0x00000000);  // Reg 10640
	WriteRegister (VIRTUALREG_START+641, 0x00000000);  // Reg 10641
	WriteRegister (VIRTUALREG_START+642, 0x00000000);  // Reg 10642
	WriteRegister (VIRTUALREG_START+643, 0x00000000);  // Reg 10643
	WriteRegister (VIRTUALREG_START+644, 0x00000000);  // Reg 10644
	WriteRegister (VIRTUALREG_START+645, 0x00000000);  // Reg 10645
	WriteRegister (VIRTUALREG_START+646, 0x00000000);  // Reg 10646
	WriteRegister (VIRTUALREG_START+647, 0x00000000);  // Reg 10647
	WriteRegister (VIRTUALREG_START+648, 0x00000000);  // Reg 10648
	WriteRegister (VIRTUALREG_START+649, 0x00000000);  // Reg 10649
	WriteRegister (VIRTUALREG_START+650, 0x00000000);  // Reg 10650
	WriteRegister (VIRTUALREG_START+651, 0x00000000);  // Reg 10651
	WriteRegister (VIRTUALREG_START+652, 0x00000000);  // Reg 10652
	WriteRegister (VIRTUALREG_START+653, 0x00000000);  // Reg 10653
	WriteRegister (VIRTUALREG_START+654, 0x00000000);  // Reg 10654
	WriteRegister (VIRTUALREG_START+655, 0x00000000);  // Reg 10655
	WriteRegister (VIRTUALREG_START+656, 0x00000000);  // Reg 10656
	WriteRegister (VIRTUALREG_START+657, 0x00000000);  // Reg 10657
	WriteRegister (VIRTUALREG_START+658, 0x00000000);  // Reg 10658
	WriteRegister (VIRTUALREG_START+659, 0x00000000);  // Reg 10659
	WriteRegister (VIRTUALREG_START+660, 0x00000000);  // Reg 10660
	WriteRegister (VIRTUALREG_START+661, 0x00000000);  // Reg 10661
	WriteRegister (VIRTUALREG_START+662, 0x00000000);  // Reg 10662
	WriteRegister (VIRTUALREG_START+663, 0x00000000);  // Reg 10663
	WriteRegister (VIRTUALREG_START+664, 0x00000000);  // Reg 10664
	WriteRegister (VIRTUALREG_START+665, 0x00000000);  // Reg 10665
	WriteRegister (VIRTUALREG_START+666, 0x00000000);  // Reg 10666
	WriteRegister (VIRTUALREG_START+667, 0x00000000);  // Reg 10667
	WriteRegister (VIRTUALREG_START+668, 0x00000000);  // Reg 10668
	WriteRegister (VIRTUALREG_START+669, 0x00000000);  // Reg 10669
	WriteRegister (VIRTUALREG_START+670, 0x00000000);  // Reg 10670
	WriteRegister (VIRTUALREG_START+671, 0x00000000);  // Reg 10671
	WriteRegister (VIRTUALREG_START+672, 0x00000000);  // Reg 10672
	WriteRegister (VIRTUALREG_START+673, 0x00000000);  // Reg 10673
	WriteRegister (VIRTUALREG_START+674, 0x00000000);  // Reg 10674
	WriteRegister (VIRTUALREG_START+675, 0x00000000);  // Reg 10675
	WriteRegister (VIRTUALREG_START+676, 0x00000000);  // Reg 10676
	WriteRegister (VIRTUALREG_START+677, 0x00000000);  // Reg 10677
	WriteRegister (VIRTUALREG_START+678, 0x00000000);  // Reg 10678
	WriteRegister (VIRTUALREG_START+679, 0x00000000);  // Reg 10679
	WriteRegister (VIRTUALREG_START+680, 0x00000000);  // Reg 10680
	WriteRegister (VIRTUALREG_START+681, 0x00000000);  // Reg 10681
	WriteRegister (VIRTUALREG_START+682, 0x00000000);  // Reg 10682
	WriteRegister (VIRTUALREG_START+683, 0x00000000);  // Reg 10683
	WriteRegister (VIRTUALREG_START+684, 0x00000000);  // Reg 10684
	WriteRegister (VIRTUALREG_START+685, 0x00000000);  // Reg 10685
	WriteRegister (VIRTUALREG_START+686, 0x00000000);  // Reg 10686
	WriteRegister (VIRTUALREG_START+687, 0x00000000);  // Reg 10687
	WriteRegister (VIRTUALREG_START+688, 0x00000000);  // Reg 10688
	WriteRegister (VIRTUALREG_START+689, 0x00000000);  // Reg 10689
	WriteRegister (VIRTUALREG_START+690, 0x00000000);  // Reg 10690
	WriteRegister (VIRTUALREG_START+691, 0x00000000);  // Reg 10691
	WriteRegister (VIRTUALREG_START+692, 0x00000000);  // Reg 10692
	WriteRegister (VIRTUALREG_START+693, 0x00000000);  // Reg 10693
	WriteRegister (VIRTUALREG_START+694, 0x00000000);  // Reg 10694
	WriteRegister (VIRTUALREG_START+695, 0x00000000);  // Reg 10695
	WriteRegister (VIRTUALREG_START+696, 0x00000000);  // Reg 10696
	WriteRegister (VIRTUALREG_START+697, 0x00000000);  // Reg 10697
	WriteRegister (VIRTUALREG_START+698, 0x00000000);  // Reg 10698
	WriteRegister (VIRTUALREG_START+699, 0x00000000);  // Reg 10699
	WriteRegister (VIRTUALREG_START+700, 0x00000000);  // Reg 10700
	WriteRegister (VIRTUALREG_START+701, 0x00000000);  // Reg 10701
	WriteRegister (VIRTUALREG_START+702, 0x00000000);  // Reg 10702
	WriteRegister (VIRTUALREG_START+703, 0x00000000);  // Reg 10703
	WriteRegister (VIRTUALREG_START+704, 0x00000000);  // Reg 10704
	WriteRegister (VIRTUALREG_START+705, 0x00000000);  // Reg 10705
	WriteRegister (VIRTUALREG_START+706, 0x00000000);  // Reg 10706
	WriteRegister (VIRTUALREG_START+707, 0x00000000);  // Reg 10707
	WriteRegister (VIRTUALREG_START+708, 0x00000000);  // Reg 10708
	WriteRegister (VIRTUALREG_START+709, 0x00000000);  // Reg 10709
	WriteRegister (VIRTUALREG_START+710, 0x00000000);  // Reg 10710
	WriteRegister (VIRTUALREG_START+711, 0x00000000);  // Reg 10711
	WriteRegister (VIRTUALREG_START+712, 0x00000000);  // Reg 10712
	WriteRegister (VIRTUALREG_START+713, 0x00000000);  // Reg 10713
	WriteRegister (VIRTUALREG_START+714, 0x00000000);  // Reg 10714
	WriteRegister (VIRTUALREG_START+715, 0x00000000);  // Reg 10715
	WriteRegister (VIRTUALREG_START+716, 0x00000000);  // Reg 10716
	WriteRegister (VIRTUALREG_START+717, 0x00000000);  // Reg 10717
	WriteRegister (VIRTUALREG_START+718, 0x00000000);  // Reg 10718
	WriteRegister (VIRTUALREG_START+719, 0x00000000);  // Reg 10719
	WriteRegister (VIRTUALREG_START+720, 0x00000000);  // Reg 10720
	WriteRegister (VIRTUALREG_START+721, 0x00000000);  // Reg 10721
	WriteRegister (VIRTUALREG_START+722, 0x00000000);  // Reg 10722
	WriteRegister (VIRTUALREG_START+723, 0x00000000);  // Reg 10723
	WriteRegister (VIRTUALREG_START+724, 0x00000000);  // Reg 10724
	WriteRegister (VIRTUALREG_START+725, 0x00000000);  // Reg 10725
	WriteRegister (VIRTUALREG_START+726, 0x00000000);  // Reg 10726
	WriteRegister (VIRTUALREG_START+727, 0x00000000);  // Reg 10727
	WriteRegister (VIRTUALREG_START+728, 0x00000000);  // Reg 10728
	WriteRegister (VIRTUALREG_START+729, 0x00000000);  // Reg 10729
	WriteRegister (VIRTUALREG_START+730, 0x00000000);  // Reg 10730
	WriteRegister (VIRTUALREG_START+731, 0x00000000);  // Reg 10731
	WriteRegister (VIRTUALREG_START+732, 0x00000000);  // Reg 10732
	WriteRegister (VIRTUALREG_START+733, 0x00000000);  // Reg 10733
	WriteRegister (VIRTUALREG_START+734, 0x00000000);  // Reg 10734
	WriteRegister (VIRTUALREG_START+735, 0x00000000);  // Reg 10735
	WriteRegister (VIRTUALREG_START+736, 0x00000000);  // Reg 10736
	WriteRegister (VIRTUALREG_START+737, 0x00000000);  // Reg 10737
	WriteRegister (VIRTUALREG_START+738, 0x00000000);  // Reg 10738
	WriteRegister (VIRTUALREG_START+739, 0x00000000);  // Reg 10739
	WriteRegister (VIRTUALREG_START+740, 0x00000000);  // Reg 10740
	WriteRegister (VIRTUALREG_START+741, 0x00000000);  // Reg 10741
	WriteRegister (VIRTUALREG_START+742, 0x00000000);  // Reg 10742
	WriteRegister (VIRTUALREG_START+743, 0x00000000);  // Reg 10743
	WriteRegister (VIRTUALREG_START+744, 0x00000000);  // Reg 10744
	WriteRegister (VIRTUALREG_START+745, 0x00000000);  // Reg 10745
	WriteRegister (VIRTUALREG_START+746, 0x00000000);  // Reg 10746
	WriteRegister (VIRTUALREG_START+747, 0x00000000);  // Reg 10747
	WriteRegister (VIRTUALREG_START+748, 0x00000000);  // Reg 10748
	WriteRegister (VIRTUALREG_START+749, 0x00000000);  // Reg 10749
	WriteRegister (VIRTUALREG_START+750, 0x00000000);  // Reg 10750
	WriteRegister (VIRTUALREG_START+751, 0x00000000);  // Reg 10751
	WriteRegister (VIRTUALREG_START+752, 0x00000000);  // Reg 10752
	WriteRegister (VIRTUALREG_START+753, 0x00000000);  // Reg 10753
	WriteRegister (VIRTUALREG_START+754, 0x00000000);  // Reg 10754
	WriteRegister (VIRTUALREG_START+755, 0x00000000);  // Reg 10755
	WriteRegister (VIRTUALREG_START+756, 0x00000000);  // Reg 10756
	WriteRegister (VIRTUALREG_START+757, 0x00000000);  // Reg 10757
	WriteRegister (VIRTUALREG_START+758, 0x00000000);  // Reg 10758
	WriteRegister (VIRTUALREG_START+759, 0x00000000);  // Reg 10759
	WriteRegister (VIRTUALREG_START+760, 0x00000000);  // Reg 10760
	WriteRegister (VIRTUALREG_START+761, 0x00000000);  // Reg 10761
	WriteRegister (VIRTUALREG_START+762, 0x00000000);  // Reg 10762
	WriteRegister (VIRTUALREG_START+763, 0x00000000);  // Reg 10763
	WriteRegister (VIRTUALREG_START+764, 0x00000000);  // Reg 10764
	WriteRegister (VIRTUALREG_START+765, 0x00000000);  // Reg 10765
	WriteRegister (VIRTUALREG_START+766, 0x00000000);  // Reg 10766
	WriteRegister (VIRTUALREG_START+767, 0x00000000);  // Reg 10767
	WriteRegister (VIRTUALREG_START+768, 0x00000000);  // Reg 10768
	WriteRegister (VIRTUALREG_START+769, 0x00000000);  // Reg 10769
	WriteRegister (VIRTUALREG_START+770, 0x00000000);  // Reg 10770
	WriteRegister (VIRTUALREG_START+771, 0x00000000);  // Reg 10771
	WriteRegister (VIRTUALREG_START+772, 0x00000000);  // Reg 10772
	WriteRegister (VIRTUALREG_START+773, 0x00000000);  // Reg 10773
	WriteRegister (VIRTUALREG_START+774, 0x00000000);  // Reg 10774
	WriteRegister (VIRTUALREG_START+775, 0x00000000);  // Reg 10775
	WriteRegister (VIRTUALREG_START+776, 0x00000000);  // Reg 10776
	WriteRegister (VIRTUALREG_START+777, 0x00000000);  // Reg 10777
	WriteRegister (VIRTUALREG_START+778, 0x00000000);  // Reg 10778
	WriteRegister (VIRTUALREG_START+779, 0x00000000);  // Reg 10779
	WriteRegister (VIRTUALREG_START+780, 0x00000000);  // Reg 10780
	WriteRegister (VIRTUALREG_START+781, 0x00000000);  // Reg 10781
	WriteRegister (VIRTUALREG_START+782, 0x00000000);  // Reg 10782
	WriteRegister (VIRTUALREG_START+783, 0x00000000);  // Reg 10783
	WriteRegister (VIRTUALREG_START+784, 0x00000000);  // Reg 10784
	WriteRegister (VIRTUALREG_START+785, 0x00000000);  // Reg 10785
	WriteRegister (VIRTUALREG_START+786, 0x00000000);  // Reg 10786
	WriteRegister (VIRTUALREG_START+787, 0x00000000);  // Reg 10787
	WriteRegister (VIRTUALREG_START+788, 0x00000000);  // Reg 10788
	WriteRegister (VIRTUALREG_START+789, 0x00000000);  // Reg 10789
	WriteRegister (VIRTUALREG_START+790, 0x00000000);  // Reg 10790
	WriteRegister (VIRTUALREG_START+791, 0x00000000);  // Reg 10791
	WriteRegister (VIRTUALREG_START+792, 0x00000000);  // Reg 10792
	WriteRegister (VIRTUALREG_START+793, 0x00000000);  // Reg 10793
	WriteRegister (VIRTUALREG_START+794, 0x00000000);  // Reg 10794
	WriteRegister (VIRTUALREG_START+795, 0x00000000);  // Reg 10795
	WriteRegister (VIRTUALREG_START+796, 0x00000000);  // Reg 10796
	WriteRegister (VIRTUALREG_START+797, 0x00000000);  // Reg 10797
	WriteRegister (VIRTUALREG_START+798, 0x00000000);  // Reg 10798
	WriteRegister (VIRTUALREG_START+799, 0x00000000);  // Reg 10799
	WriteRegister (VIRTUALREG_START+800, 0x00000000);  // Reg 10800
	WriteRegister (VIRTUALREG_START+801, 0x00000000);  // Reg 10801
	WriteRegister (VIRTUALREG_START+802, 0x00000000);  // Reg 10802
	WriteRegister (VIRTUALREG_START+803, 0x00000000);  // Reg 10803
	WriteRegister (VIRTUALREG_START+804, 0x00000000);  // Reg 10804
	WriteRegister (VIRTUALREG_START+805, 0x00000000);  // Reg 10805
	WriteRegister (VIRTUALREG_START+806, 0x00000000);  // Reg 10806
	WriteRegister (VIRTUALREG_START+807, 0x00000000);  // Reg 10807
	WriteRegister (VIRTUALREG_START+808, 0x00000000);  // Reg 10808
	WriteRegister (VIRTUALREG_START+809, 0x00000000);  // Reg 10809
	WriteRegister (VIRTUALREG_START+810, 0x00000000);  // Reg 10810
	WriteRegister (VIRTUALREG_START+811, 0x00000000);  // Reg 10811
	WriteRegister (VIRTUALREG_START+812, 0x00000000);  // Reg 10812
	WriteRegister (VIRTUALREG_START+813, 0x00000000);  // Reg 10813
	WriteRegister (VIRTUALREG_START+814, 0x00000000);  // Reg 10814
	WriteRegister (VIRTUALREG_START+815, 0x00000000);  // Reg 10815
	WriteRegister (VIRTUALREG_START+816, 0x00000000);  // Reg 10816
	WriteRegister (VIRTUALREG_START+817, 0x00000000);  // Reg 10817
	WriteRegister (VIRTUALREG_START+818, 0x00000000);  // Reg 10818
	WriteRegister (VIRTUALREG_START+819, 0x00000000);  // Reg 10819
	WriteRegister (VIRTUALREG_START+820, 0x00000000);  // Reg 10820
	WriteRegister (VIRTUALREG_START+821, 0x00000000);  // Reg 10821
	WriteRegister (VIRTUALREG_START+822, 0x00000000);  // Reg 10822
	WriteRegister (VIRTUALREG_START+823, 0x00000000);  // Reg 10823
	WriteRegister (VIRTUALREG_START+824, 0x00000000);  // Reg 10824
	WriteRegister (VIRTUALREG_START+825, 0x00000000);  // Reg 10825
	WriteRegister (VIRTUALREG_START+826, 0x00000000);  // Reg 10826
	WriteRegister (VIRTUALREG_START+827, 0x00000000);  // Reg 10827
	WriteRegister (VIRTUALREG_START+828, 0x00000000);  // Reg 10828
	WriteRegister (VIRTUALREG_START+829, 0x00000000);  // Reg 10829
	WriteRegister (VIRTUALREG_START+830, 0x00000000);  // Reg 10830
	WriteRegister (VIRTUALREG_START+831, 0x00000000);  // Reg 10831
	WriteRegister (VIRTUALREG_START+832, 0x00000000);  // Reg 10832
	WriteRegister (VIRTUALREG_START+833, 0x00000000);  // Reg 10833
	WriteRegister (VIRTUALREG_START+834, 0x00000000);  // Reg 10834
	WriteRegister (VIRTUALREG_START+835, 0x00000000);  // Reg 10835
	WriteRegister (VIRTUALREG_START+836, 0x00000000);  // Reg 10836
	WriteRegister (VIRTUALREG_START+837, 0x00000000);  // Reg 10837
	WriteRegister (VIRTUALREG_START+838, 0x00000000);  // Reg 10838
	WriteRegister (VIRTUALREG_START+839, 0x00000000);  // Reg 10839
	WriteRegister (VIRTUALREG_START+840, 0x00000000);  // Reg 10840
	WriteRegister (VIRTUALREG_START+841, 0x00000000);  // Reg 10841
	WriteRegister (VIRTUALREG_START+842, 0x00000000);  // Reg 10842
	WriteRegister (VIRTUALREG_START+843, 0x00000000);  // Reg 10843
	WriteRegister (VIRTUALREG_START+844, 0x00000000);  // Reg 10844
	WriteRegister (VIRTUALREG_START+845, 0x00000000);  // Reg 10845
	WriteRegister (VIRTUALREG_START+846, 0x00000000);  // Reg 10846
	WriteRegister (VIRTUALREG_START+847, 0x00000000);  // Reg 10847
	WriteRegister (VIRTUALREG_START+848, 0x00000000);  // Reg 10848
	WriteRegister (VIRTUALREG_START+849, 0x00000000);  // Reg 10849
	WriteRegister (VIRTUALREG_START+850, 0x00000000);  // Reg 10850
	WriteRegister (VIRTUALREG_START+851, 0x00000000);  // Reg 10851
	WriteRegister (VIRTUALREG_START+852, 0x00000000);  // Reg 10852
	WriteRegister (VIRTUALREG_START+853, 0x00000000);  // Reg 10853
	WriteRegister (VIRTUALREG_START+854, 0x00000000);  // Reg 10854
	WriteRegister (VIRTUALREG_START+855, 0x00000000);  // Reg 10855
	WriteRegister (VIRTUALREG_START+856, 0x00000000);  // Reg 10856
	WriteRegister (VIRTUALREG_START+857, 0x00000000);  // Reg 10857
	WriteRegister (VIRTUALREG_START+858, 0x00000000);  // Reg 10858
	WriteRegister (VIRTUALREG_START+859, 0x00000000);  // Reg 10859
	WriteRegister (VIRTUALREG_START+860, 0x00000000);  // Reg 10860
	WriteRegister (VIRTUALREG_START+861, 0x00000000);  // Reg 10861
	WriteRegister (VIRTUALREG_START+862, 0x00000000);  // Reg 10862
	WriteRegister (VIRTUALREG_START+863, 0x00000000);  // Reg 10863
	WriteRegister (VIRTUALREG_START+864, 0x00000000);  // Reg 10864
	WriteRegister (VIRTUALREG_START+865, 0x00000000);  // Reg 10865
	WriteRegister (VIRTUALREG_START+866, 0x00000000);  // Reg 10866
	WriteRegister (VIRTUALREG_START+867, 0x00000000);  // Reg 10867
	WriteRegister (VIRTUALREG_START+868, 0x00000000);  // Reg 10868
	WriteRegister (VIRTUALREG_START+869, 0x00000000);  // Reg 10869
	WriteRegister (VIRTUALREG_START+870, 0x00000000);  // Reg 10870
	WriteRegister (VIRTUALREG_START+871, 0x00000000);  // Reg 10871
	WriteRegister (VIRTUALREG_START+872, 0x00000000);  // Reg 10872
	WriteRegister (VIRTUALREG_START+873, 0x00000000);  // Reg 10873
	WriteRegister (VIRTUALREG_START+874, 0x00000000);  // Reg 10874
	WriteRegister (VIRTUALREG_START+875, 0x00000000);  // Reg 10875
	WriteRegister (VIRTUALREG_START+876, 0x00000000);  // Reg 10876
	WriteRegister (VIRTUALREG_START+877, 0x00000000);  // Reg 10877
	WriteRegister (VIRTUALREG_START+878, 0x00000000);  // Reg 10878
	WriteRegister (VIRTUALREG_START+879, 0x00000000);  // Reg 10879
	WriteRegister (VIRTUALREG_START+880, 0x00000000);  // Reg 10880
	WriteRegister (VIRTUALREG_START+881, 0x00000000);  // Reg 10881
	WriteRegister (VIRTUALREG_START+882, 0x00000000);  // Reg 10882
	WriteRegister (VIRTUALREG_START+883, 0x00000000);  // Reg 10883
	WriteRegister (VIRTUALREG_START+884, 0x00000000);  // Reg 10884
	WriteRegister (VIRTUALREG_START+885, 0x00000000);  // Reg 10885
	WriteRegister (VIRTUALREG_START+886, 0x00000000);  // Reg 10886
	WriteRegister (VIRTUALREG_START+887, 0x00000000);  // Reg 10887
	WriteRegister (VIRTUALREG_START+888, 0x00000000);  // Reg 10888
	WriteRegister (VIRTUALREG_START+889, 0x00000000);  // Reg 10889
	WriteRegister (VIRTUALREG_START+890, 0x00000000);  // Reg 10890
	WriteRegister (VIRTUALREG_START+891, 0x00000000);  // Reg 10891
	WriteRegister (VIRTUALREG_START+892, 0x00000000);  // Reg 10892
	WriteRegister (VIRTUALREG_START+893, 0x00000000);  // Reg 10893
	WriteRegister (VIRTUALREG_START+894, 0x00000000);  // Reg 10894
	WriteRegister (VIRTUALREG_START+895, 0x00000000);  // Reg 10895
	WriteRegister (VIRTUALREG_START+896, 0x00000000);  // Reg 10896
	WriteRegister (VIRTUALREG_START+897, 0x00000000);  // Reg 10897
	WriteRegister (VIRTUALREG_START+898, 0x00000000);  // Reg 10898
	WriteRegister (VIRTUALREG_START+899, 0x00000000);  // Reg 10899
	WriteRegister (VIRTUALREG_START+900, 0x00000000);  // Reg 10900
	WriteRegister (VIRTUALREG_START+901, 0x00000000);  // Reg 10901
	WriteRegister (VIRTUALREG_START+902, 0x00000000);  // Reg 10902
	WriteRegister (VIRTUALREG_START+903, 0x00000000);  // Reg 10903
	WriteRegister (VIRTUALREG_START+904, 0x00000000);  // Reg 10904
	WriteRegister (VIRTUALREG_START+905, 0x00000000);  // Reg 10905
	WriteRegister (VIRTUALREG_START+906, 0x00000000);  // Reg 10906
	WriteRegister (VIRTUALREG_START+907, 0x00000000);  // Reg 10907
	WriteRegister (VIRTUALREG_START+908, 0x00000000);  // Reg 10908
	WriteRegister (VIRTUALREG_START+909, 0x00000000);  // Reg 10909
	WriteRegister (VIRTUALREG_START+910, 0x00000000);  // Reg 10910
	WriteRegister (VIRTUALREG_START+911, 0x00000000);  // Reg 10911
	WriteRegister (VIRTUALREG_START+912, 0x00000000);  // Reg 10912
	WriteRegister (VIRTUALREG_START+913, 0x00000000);  // Reg 10913
	WriteRegister (VIRTUALREG_START+914, 0x00000000);  // Reg 10914
	WriteRegister (VIRTUALREG_START+915, 0x00000000);  // Reg 10915
	WriteRegister (VIRTUALREG_START+916, 0x00000000);  // Reg 10916
	WriteRegister (VIRTUALREG_START+917, 0x00000000);  // Reg 10917
	WriteRegister (VIRTUALREG_START+918, 0x00000000);  // Reg 10918
	WriteRegister (VIRTUALREG_START+919, 0x00000000);  // Reg 10919
	WriteRegister (VIRTUALREG_START+920, 0x00000000);  // Reg 10920
	WriteRegister (VIRTUALREG_START+921, 0x00000000);  // Reg 10921
	WriteRegister (VIRTUALREG_START+922, 0x00000000);  // Reg 10922
	WriteRegister (VIRTUALREG_START+923, 0x00000000);  // Reg 10923
	WriteRegister (VIRTUALREG_START+924, 0x00000000);  // Reg 10924
	WriteRegister (VIRTUALREG_START+925, 0x00000000);  // Reg 10925
	WriteRegister (VIRTUALREG_START+926, 0x00000000);  // Reg 10926
	WriteRegister (VIRTUALREG_START+927, 0x00000000);  // Reg 10927
	WriteRegister (VIRTUALREG_START+928, 0x00000000);  // Reg 10928
	WriteRegister (VIRTUALREG_START+929, 0x00000000);  // Reg 10929
	WriteRegister (VIRTUALREG_START+930, 0x00000000);  // Reg 10930
	WriteRegister (VIRTUALREG_START+931, 0x00000000);  // Reg 10931
	WriteRegister (VIRTUALREG_START+932, 0x00000000);  // Reg 10932
	WriteRegister (VIRTUALREG_START+933, 0x00000000);  // Reg 10933
	WriteRegister (VIRTUALREG_START+934, 0x00000000);  // Reg 10934
	WriteRegister (VIRTUALREG_START+935, 0x00000000);  // Reg 10935
	WriteRegister (VIRTUALREG_START+936, 0x00000000);  // Reg 10936
	WriteRegister (VIRTUALREG_START+937, 0x00000000);  // Reg 10937
	WriteRegister (VIRTUALREG_START+938, 0x00000000);  // Reg 10938
	WriteRegister (VIRTUALREG_START+939, 0x00000000);  // Reg 10939
	WriteRegister (VIRTUALREG_START+940, 0x00000000);  // Reg 10940
	WriteRegister (VIRTUALREG_START+941, 0x00000000);  // Reg 10941
	WriteRegister (VIRTUALREG_START+942, 0x00000000);  // Reg 10942
	WriteRegister (VIRTUALREG_START+943, 0x00000000);  // Reg 10943
	WriteRegister (VIRTUALREG_START+944, 0x00000000);  // Reg 10944
	WriteRegister (VIRTUALREG_START+945, 0x00000000);  // Reg 10945
	WriteRegister (VIRTUALREG_START+946, 0x00000000);  // Reg 10946
	WriteRegister (VIRTUALREG_START+947, 0x00000000);  // Reg 10947
	WriteRegister (VIRTUALREG_START+948, 0x00000000);  // Reg 10948
	WriteRegister (VIRTUALREG_START+949, 0x00000000);  // Reg 10949
	WriteRegister (VIRTUALREG_START+950, 0x00000000);  // Reg 10950
	WriteRegister (VIRTUALREG_START+951, 0x00000000);  // Reg 10951
	WriteRegister (VIRTUALREG_START+952, 0x00000000);  // Reg 10952
	WriteRegister (VIRTUALREG_START+953, 0x00000000);  // Reg 10953
	WriteRegister (VIRTUALREG_START+954, 0x00000000);  // Reg 10954
	WriteRegister (VIRTUALREG_START+955, 0x00000000);  // Reg 10955
	WriteRegister (VIRTUALREG_START+956, 0x00000000);  // Reg 10956
	WriteRegister (VIRTUALREG_START+957, 0x00000000);  // Reg 10957
	WriteRegister (VIRTUALREG_START+958, 0x00000000);  // Reg 10958
	WriteRegister (VIRTUALREG_START+959, 0x00000000);  // Reg 10959
	WriteRegister (VIRTUALREG_START+960, 0x00000000);  // Reg 10960
	WriteRegister (VIRTUALREG_START+961, 0x00000000);  // Reg 10961
	WriteRegister (VIRTUALREG_START+962, 0x00000000);  // Reg 10962
	WriteRegister (VIRTUALREG_START+963, 0x00000000);  // Reg 10963
	WriteRegister (VIRTUALREG_START+964, 0x00000000);  // Reg 10964
	WriteRegister (VIRTUALREG_START+965, 0x00000000);  // Reg 10965
	WriteRegister (VIRTUALREG_START+966, 0x00000000);  // Reg 10966
	WriteRegister (VIRTUALREG_START+967, 0x00000000);  // Reg 10967
	WriteRegister (VIRTUALREG_START+968, 0x00000000);  // Reg 10968
	WriteRegister (VIRTUALREG_START+969, 0x00000000);  // Reg 10969
	WriteRegister (VIRTUALREG_START+970, 0x00000000);  // Reg 10970
	WriteRegister (VIRTUALREG_START+971, 0x00000000);  // Reg 10971
	WriteRegister (VIRTUALREG_START+972, 0x00000000);  // Reg 10972
	WriteRegister (VIRTUALREG_START+973, 0x00000000);  // Reg 10973
	WriteRegister (VIRTUALREG_START+974, 0x00000000);  // Reg 10974
	WriteRegister (VIRTUALREG_START+975, 0x00000000);  // Reg 10975
	WriteRegister (VIRTUALREG_START+976, 0x00000000);  // Reg 10976
	WriteRegister (VIRTUALREG_START+977, 0x00000000);  // Reg 10977
	WriteRegister (VIRTUALREG_START+978, 0x00000000);  // Reg 10978
	WriteRegister (VIRTUALREG_START+979, 0x00000000);  // Reg 10979
	WriteRegister (VIRTUALREG_START+980, 0x00000000);  // Reg 10980
	WriteRegister (VIRTUALREG_START+981, 0x00000000);  // Reg 10981
	WriteRegister (VIRTUALREG_START+982, 0x00000000);  // Reg 10982
	WriteRegister (VIRTUALREG_START+983, 0x00000000);  // Reg 10983
	WriteRegister (VIRTUALREG_START+984, 0x00000000);  // Reg 10984
	WriteRegister (VIRTUALREG_START+985, 0x00000000);  // Reg 10985
	WriteRegister (VIRTUALREG_START+986, 0x00000000);  // Reg 10986
	WriteRegister (VIRTUALREG_START+987, 0x00000000);  // Reg 10987
	WriteRegister (VIRTUALREG_START+988, 0x00000000);  // Reg 10988
	WriteRegister (VIRTUALREG_START+989, 0x00000000);  // Reg 10989
	WriteRegister (VIRTUALREG_START+990, 0x00000000);  // Reg 10990
	WriteRegister (VIRTUALREG_START+991, 0x00000000);  // Reg 10991
	WriteRegister (VIRTUALREG_START+992, 0x00000000);  // Reg 10992
	WriteRegister (VIRTUALREG_START+993, 0x00000000);  // Reg 10993
	WriteRegister (VIRTUALREG_START+994, 0x00000000);  // Reg 10994
	WriteRegister (VIRTUALREG_START+995, 0x00000000);  // Reg 10995
	WriteRegister (VIRTUALREG_START+996, 0x00000000);  // Reg 10996
	WriteRegister (VIRTUALREG_START+997, 0x00000000);  // Reg 10997
	WriteRegister (VIRTUALREG_START+998, 0x00000000);  // Reg 10998
	WriteRegister (VIRTUALREG_START+999, 0x00000000);  // Reg 10999
	WriteRegister (VIRTUALREG_START+1000, 0x00000000);  // Reg 11000
	WriteRegister (VIRTUALREG_START+1001, 0x00000000);  // Reg 11001
	WriteRegister (VIRTUALREG_START+1002, 0x00000000);  // Reg 11002
	WriteRegister (VIRTUALREG_START+1003, 0x00000000);  // Reg 11003
	WriteRegister (VIRTUALREG_START+1004, 0x00000000);  // Reg 11004
	WriteRegister (VIRTUALREG_START+1005, 0x00000000);  // Reg 11005
	WriteRegister (VIRTUALREG_START+1006, 0x00000000);  // Reg 11006
	WriteRegister (VIRTUALREG_START+1007, 0x00000000);  // Reg 11007
	WriteRegister (VIRTUALREG_START+1008, 0x00000000);  // Reg 11008
	WriteRegister (VIRTUALREG_START+1009, 0x00000000);  // Reg 11009
	WriteRegister (VIRTUALREG_START+1010, 0x00000000);  // Reg 11010
	WriteRegister (VIRTUALREG_START+1011, 0x00000000);  // Reg 11011
	WriteRegister (VIRTUALREG_START+1012, 0x00000000);  // Reg 11012
	WriteRegister (VIRTUALREG_START+1013, 0x00000000);  // Reg 11013
	WriteRegister (VIRTUALREG_START+1014, 0x00000000);  // Reg 11014
	WriteRegister (VIRTUALREG_START+1015, 0x00000000);  // Reg 11015
	WriteRegister (VIRTUALREG_START+1016, 0x00000000);  // Reg 11016
	WriteRegister (VIRTUALREG_START+1017, 0x00000000);  // Reg 11017
	WriteRegister (VIRTUALREG_START+1018, 0x00000000);  // Reg 11018
	WriteRegister (VIRTUALREG_START+1019, 0x00000000);  // Reg 11019
	WriteRegister (VIRTUALREG_START+1020, 0x00000000);  // Reg 11020
	WriteRegister (VIRTUALREG_START+1021, 0x00000000);  // Reg 11021
	WriteRegister (VIRTUALREG_START+1022, 0x00000000);  // Reg 11022
	WriteRegister (VIRTUALREG_START+1023, 0x00000000);  // Reg 11023
}
#endif	//	NTV2_FORCE_NO_DEVICE
