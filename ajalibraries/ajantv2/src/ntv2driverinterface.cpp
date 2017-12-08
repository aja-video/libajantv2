/**
	@file		ntv2driverinterface.cpp
	@brief		Implements the CNTV2DriverInterface base class.
	@copyright	(C) 2003-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ajatypes.h"
#include "ajaexport.h"
#include "ntv2enums.h"
#include "ntv2driverinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2nubaccess.h"
#include "ntv2bitfile.h"
#include "ntv2registers2022.h"
#include "ntv2spiinterface.h"

#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>

using namespace std;

typedef map <INTERRUPT_ENUMS, string>	InterruptEnumStringMap;
static InterruptEnumStringMap			gInterruptNames;

class DriverInterfaceGlobalInitializer
{
	public:
		DriverInterfaceGlobalInitializer ()
		{
			gInterruptNames [eOutput1]				= "eOutput1";
			gInterruptNames [eInterruptMask]		= "eInterruptMask";
			gInterruptNames [eInput1]				= "eInput1";
			gInterruptNames [eInput2]				= "eInput2";
			gInterruptNames [eAudio]				= "eAudio";
			gInterruptNames [eAudioInWrap]			= "eAudioInWrap";
			gInterruptNames [eAudioOutWrap]			= "eAudioOutWrap";
			gInterruptNames [eDMA1]					= "eDMA1";
			gInterruptNames [eDMA2]					= "eDMA2";
			gInterruptNames [eDMA3]					= "eDMA3";
			gInterruptNames [eDMA4]					= "eDMA4";
			gInterruptNames [eChangeEvent]			= "eChangeEvent";
			gInterruptNames [eGetIntCount]			= "eGetIntCount";
			gInterruptNames [eWrapRate]				= "eWrapRate";
			gInterruptNames [eUart1Tx]				= "eUart1Tx";
			gInterruptNames [eUart1Rx]				= "eUart1Rx";
			gInterruptNames [eAuxVerticalInterrupt]	= "eAuxVerticalInterrupt";
			gInterruptNames [ePushButtonChange]		= "ePushButtonChange";
			gInterruptNames [eLowPower]				= "eLowPower";
			gInterruptNames [eDisplayFIFO]			= "eDisplayFIFO";
			gInterruptNames [eSATAChange]			= "eSATAChange";
			gInterruptNames [eTemp1High]			= "eTemp1High";
			gInterruptNames [eTemp2High]			= "eTemp2High";
			gInterruptNames [ePowerButtonChange]	= "ePowerButtonChange";
			gInterruptNames [eInput3]				= "eInput3";
			gInterruptNames [eInput4]				= "eInput4";
			gInterruptNames [eUart2Tx]				= "eUart2Tx";
			gInterruptNames [eUart2Rx]				= "eUart2Rx";
			gInterruptNames [eHDMIRxV2HotplugDetect]= "eHDMIRxV2HotplugDetect";
			gInterruptNames [eInput5]				= "eInput5";
			gInterruptNames [eInput6]				= "eInput6";
			gInterruptNames [eInput7]				= "eInput7";
			gInterruptNames [eInput8]				= "eInput8";
			gInterruptNames [eInterruptMask2]		= "eInterruptMask2";
			gInterruptNames [eOutput2]				= "eOutput2";
			gInterruptNames [eOutput3]				= "eOutput3";
			gInterruptNames [eOutput4]				= "eOutput4";
			gInterruptNames [eOutput5]				= "eOutput5";
			gInterruptNames [eOutput6]				= "eOutput6";
			gInterruptNames [eOutput7]				= "eOutput7";
			gInterruptNames [eOutput8]				= "eOutput8";
		}
};

static DriverInterfaceGlobalInitializer	gInitializerSingleton;


CNTV2DriverInterface::CNTV2DriverInterface ()
	:	_boardNumber					(0),
		_boardOpened					(false),
		_boardType						(DEVICETYPE_NTV2),
		_boardID						(DEVICE_ID_NOTFOUND),
		_displayErrorMessage			(false),
		_pciSlot						(0),
		_programStatus					(0),
		_pFrameBaseAddress				(NULL),
		_pCh1FrameBaseAddress			(NULL),			//	DEPRECATE!
		_pCh2FrameBaseAddress			(NULL),			//	DEPRECATE!
		_pRegisterBaseAddress			(NULL),
		_pRegisterBaseAddressLength		(0),
		_pFS1FPGARegisterBaseAddress	(NULL),			//	DEPRECATE!
		_pXena2FlashBaseAddress			(NULL),
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

}	//	constructor


CNTV2DriverInterface::~CNTV2DriverInterface ()
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_sockfd != -1)
	{
		#ifdef MSWindows
			closesocket (_sockfd);
		#else
			close (_sockfd);
		#endif
		_sockfd = (AJASocket) -1;
	}
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
}	//	destructor


bool CNTV2DriverInterface::ConfigureSubscription (bool bSubscribe, INTERRUPT_ENUMS eInterruptType, PULWord & hSubscription)
{
	if (bSubscribe)							//	If subscribing,
		mEventCounts [eInterruptType] = 0;	//		clear this interrupt's event counter
// 	#if defined (_DEBUG)
// 	else
// 		cerr << "## DEBUG:  Unsubscribing '" << gInterruptNames [eInterruptType] << "' (" << eInterruptType << "), " << mEventCounts [eInterruptType] << " event(s) received" << endl;
// 	#endif
	(void) hSubscription;
	return true;

}	//	ConfigureSubscription


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
		if (_sockfd != -1)
		{
			#ifdef MSWindows
				closesocket (_sockfd);
			#else
				close (_sockfd);
			#endif
		}
		_sockfd = -1;

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
		if (_sockfd != -1)
		{
			#ifdef MSWindows
				closesocket (_sockfd);
			#else
				close (_sockfd);
			#endif
		}
		_sockfd = -1;

		// Establish connection
		if(NTV2ConnectToNub(hostname, &_sockfd) >= 0)
		{
			_hostname = hostname;
		}
		else
		{
			if (_sockfd != -1)
			{
				#ifdef MSWindows
					closesocket (_sockfd);
				#else
					close (_sockfd);
				#endif
				_sockfd = -1;
			}
			return false;
		}
	}

	// Open the card on the remote system.
	switch(NTV2OpenRemoteCard(_sockfd, boardNumber, ulBoardType, &_remoteHandle, &_nubProtocolVersion))
	{
		case NTV2_REMOTE_ACCESS_SUCCESS:
			// printf("_remoteHandle came back as 0x%08x\n", _remoteHandle);
			return true;

		case NTV2_REMOTE_ACCESS_CONNECTION_CLOSED:
			#ifdef MSWindows
				closesocket (_sockfd);
			#else
				close (_sockfd);
			#endif
			_sockfd = -1;
			// printf("_remoteHandle came back as 0x%08x\n", _remoteHandle);
			_remoteHandle = (LWord)INVALID_NUB_HANDLE;
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
		#ifdef MSWindows
			closesocket(_sockfd);
		#else
			close(_sockfd);
		#endif
		_remoteHandle = (LWord) INVALID_NUB_HANDLE;
		_sockfd = -1;
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
{
	(void) inRHS;
	assert (false && "These are not assignable");
	return *this;
}	//	operator =


CNTV2DriverInterface::CNTV2DriverInterface (const CNTV2DriverInterface & inObjToCopy)
{
	(void) inObjToCopy;
	assert (false && "These are not copyable");
}	//	copy constructor


// Common remote card read register.  Subclasses have overloaded function
// that does platform-specific read of register on local card.
bool CNTV2DriverInterface::ReadRegister (ULWord inRegisterNumber, ULWord * pOutRegisterValue, ULWord inRegisterMask, ULWord inRegisterShift)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	assert( _remoteHandle != INVALID_NUB_HANDLE);

	return !NTV2ReadRegisterRemote(_sockfd,
								_remoteHandle,
								_nubProtocolVersion,
								inRegisterNumber,
								pOutRegisterValue,
								inRegisterMask,
								inRegisterShift);
#else
	(void) inRegisterNumber;
	(void) pOutRegisterValue;
	(void) inRegisterMask;
	(void) inRegisterShift;
	return false;
#endif
}


// Common remote card read multiple registers.  Subclasses have overloaded function
// that does platform-specific read of multiple register on local card.
bool CNTV2DriverInterface::ReadRegisterMulti (ULWord numRegs, ULWord *whichRegisterFailed, NTV2ReadWriteRegisterSingle aRegs[])
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
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
		if (!ReadRegister(	aRegs[i].registerNumber,
							&aRegs[i].registerValue,
							aRegs[i].registerMask,
							aRegs[i].registerShift)
							)
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
	// NOTE: DO NOT REMOVE THIS FUNCTION
	// It's needed for the nub client to work
	(void) inDMAEngine;
	(void) inIsRead;
	(void) inFrameNumber;
	(void) pFrameBuffer;
	(void) inOffsetBytes;
	(void) inByteCount;
	(void) inSynchronous;
	return false;
}

// Remote card GetDriverVersion.  Tested on Mac only, other platforms use
// a virtual register for the driver version.
bool CNTV2DriverInterface::GetDriverVersion (ULWord * driverVersion)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	assert( _remoteHandle != INVALID_NUB_HANDLE);

	return !NTV2GetDriverVersionRemote(	_sockfd,
										_remoteHandle,
										_nubProtocolVersion,
										driverVersion);
#else
	(void) driverVersion;
	return false;
#endif
}


// Common remote card write register.  Subclasses have overloaded function
// that does platform-specific write of register on local card.
bool CNTV2DriverInterface::WriteRegister (ULWord registerNumber, ULWord registerValue, ULWord registerMask, ULWord registerShift)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	assert( _remoteHandle != INVALID_NUB_HANDLE);

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
	assert( _remoteHandle != INVALID_NUB_HANDLE);

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
	assert( _remoteHandle != INVALID_NUB_HANDLE);

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


// Common remote card DriverGetBitFileInformation.  Subclasses have overloaded function
// that does platform-specific function on local cards.
bool CNTV2DriverInterface::DriverGetBitFileInformation (BITFILE_INFO_STRUCT & bitFileInfo, NTV2BitFileType bitFileType)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	//assert( _remoteHandle != INVALID_NUB_HANDLE);
	if (_remoteHandle != INVALID_NUB_HANDLE)
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
				case DEVICE_ID_KONAIP_2RX_1SFP_J2K:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2RX_1SFP_J2K;         break;
				case DEVICE_ID_CORVIDHBR:					bitFileInfo.bitFileType = NTV2_BITFILE_NUMBITFILETYPES;				break;
				case DEVICE_ID_IO4KPLUS:					bitFileInfo.bitFileType = NTV2_BITFILE_IO4KPLUS_MAIN;				break;
                case DEVICE_ID_IOIP_2022:					bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2022;					break;
                case DEVICE_ID_IOIP_2110:					bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2110;					break;
                case DEVICE_ID_KONAIP_1RX_1TX_2110:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_1RX_1TX_2110;			break;
                case DEVICE_ID_KONAIP_2110:                 bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2110;                 break;
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
    if(!IsDeviceReady(false) || !IsKonaIPDevice())
    {
        // cannot read flash
        return false;
    }

    string packInfo;
    ULWord deviceID = (ULWord)_boardID;
    ReadRegister (kRegBoardID, &deviceID);

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
        ULWord* bitFilePtr =  new ULWord[256/4];
        ULWord dwordSizeCount = 256/4;

        WriteRegister(kRegXenaxFlashAddress, (ULWord)1);   // bank 1
        WriteRegister(kRegXenaxFlashControlStatus, 0x17);
        bool busy = true;
        ULWord timeoutCount = 1000;
        do
        {
            ULWord regValue;
            ReadRegister(kRegXenaxFlashControlStatus, &regValue);
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

        for ( ULWord count = 0; count < dwordSizeCount; count++, baseAddress += 4 )
        {
            WriteRegister(kRegXenaxFlashAddress, baseAddress);
            WriteRegister(kRegXenaxFlashControlStatus, 0x0B);
            busy = true;
            timeoutCount = 1000;
            do
            {
                ULWord regValue;
                ReadRegister(kRegXenaxFlashControlStatus, &regValue);
                if ( regValue & BIT(8))
                {
                    busy = true;
                    timeoutCount--;
                }
                else
                    busy = false;
            } while(busy == true && timeoutCount > 0);
            if (timeoutCount == 0)
                return false;
            ReadRegister(kRegXenaxFlashDOUT, &bitFilePtr[count]);
        }

        packInfo = (char*)bitFilePtr;
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
	NTV2_ASSERT (_remoteHandle != INVALID_NUB_HANDLE);
	return ! NTV2DriverGetBuildInformationRemote (_sockfd,  _remoteHandle,  _nubProtocolVersion,  buildInfo);
#else
	(void) buildInfo;
	return false;
#endif
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
		ReadRegister(kRegGlobalControl2, &returnVal1, kRegMaskQuadMode, kRegShiftQuadMode);
	if(::NTV2DeviceCanDo425Mux(_boardID))
		ReadRegister(kRegGlobalControl2, &returnVal2, kRegMask425FB12, kRegShift425FB12);

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
	ULWord* bitFilePtr =  new ULWord[256/4];
	ULWord dwordSizeCount = 256/4;

	if (NTV2DeviceHasSPIv4(_boardID))
    {
        uint32_t val;
        ReadRegister((0x100000 + 0x08) / 4, &val);
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
			ReadRegister(kRegXenaxFlashControlStatus, &regValue);
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

	for ( ULWord count = 0; count < dwordSizeCount; count++, baseAddress += 4 )
	{
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteRegister(kRegXenaxFlashControlStatus, 0x0B);
		bool busy = true;
		ULWord timeoutCount = 1000;
		do
		{
			ULWord regValue;
			ReadRegister(kRegXenaxFlashControlStatus, &regValue);
			if ( regValue & BIT(8))
			{
				busy = true;
				timeoutCount--;
			}
			else
				busy = false;
		} while(busy == true && timeoutCount > 0);
		if (timeoutCount == 0)
			return false;
		ReadRegister(kRegXenaxFlashDOUT, &bitFilePtr[count]);
	}

	CNTV2Bitfile fileInfo;
	std::string headerError = fileInfo.ParseHeaderFromBuffer((uint8_t*)bitFilePtr);
	if (headerError.size() == 0)
	{
		strncpy(bitFileInfo.dateStr, fileInfo.GetDate().c_str(), NTV2_BITFILE_DATETIME_STRINGLENGTH);
		strncpy(bitFileInfo.timeStr, fileInfo.GetTime().c_str(), NTV2_BITFILE_DATETIME_STRINGLENGTH);
		strncpy(bitFileInfo.designNameStr, fileInfo.GetDesignName().c_str(), NTV2_BITFILE_DESIGNNAME_STRINGLENGTH);
		strncpy(bitFileInfo.partNameStr, fileInfo.GetPartName().c_str(), NTV2_BITFILE_PARTNAME_STRINGLENGTH);
		bitFileInfo.numBytes = fileInfo.GetProgramStreamLength();
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
	if (IsKonaIPDevice())
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
	if (IsKonaIPDevice())
	{
        uint32_t val;
        ReadRegister(SAREK_REGS + kRegSarekIfVersion, &val);
        if (val == SAREK_IF_VERSION)
            return true;
        else
            return false;
	}
	return true;
}

bool CNTV2DriverInterface::IsMBSystemReady()
{
	if (IsKonaIPDevice())
	{
        uint32_t val;
        ReadRegister(SAREK_REGS + kRegSarekMBState, &val);
        if (val != 0x01)
        {
            //MB not ready
            return false;
        }
        else
        {
            return true;
        }
	}
	return false;
}

bool CNTV2DriverInterface::IsKonaIPDevice()
{
	ULWord val = 0;
	ULWord hexID = 0x0;
	ReadRegister (kRegBoardID, &hexID);
	switch((NTV2DeviceID)hexID)
	{
	case DEVICE_ID_KONA4:
	case DEVICE_ID_KONA4UFC:
		ReadRegister((0x100000 + 0x80) / 4, &val);
		if (val != 0x00000000 && val != 0xffffffff)
			return true;
		else
			return false;

	case DEVICE_ID_KONAIP_2022:
	case DEVICE_ID_KONAIP_4CH_2SFP:
	case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:
	case DEVICE_ID_KONAIP_2TX_1SFP_J2K:
	case DEVICE_ID_KONAIP_1RX_1TX_2110:
	case DEVICE_ID_KONAIP_2110:
    case DEVICE_ID_IOIP_2022:
    case DEVICE_ID_IOIP_2110:
		return true;
	default:
		return false;
	}
}

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
