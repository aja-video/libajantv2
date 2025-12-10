/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2driverinterface.cpp
	@brief		Implements the CNTV2DriverInterface class.
	@copyright	(C) 2003-2022 AJA Video Systems, Inc.
**/

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2debug.h"
#include "ntv2driverinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2nubaccess.h"
#include "ntv2bitfile.h"
#include "ntv2registersmb.h"	//	for SAREK_REGS
#include "ntv2spiinterface.h"
#include "ntv2utils.h"
#include "ntv2version.h"
#include "ntv2devicescanner.h"	//	for IsHexDigit, IsAlphaNumeric, etc.
#include "ajabase/system/debug.h"
#include "ajabase/system/atomic.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"
#include "ajabase/common/common.h"	//	aja::join
#include <string.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <map>

using namespace std;

#define INSTP(_p_)			HEX0N(uint64_t(_p_),16)
#define DIFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

//	Stats
static uint32_t gConstructCount(0); //	Number of constructor calls made
static uint32_t gDestructCount(0);	//	Number of destructor calls made
static uint32_t gOpenCount(0);		//	Number of successful Open calls made
static uint32_t gCloseCount(0);		//	Number of Close calls made
//#define	_DEBUGSTATS_			//	Define this to log above construct/destruct & open/close tallies
#if defined(_DEBUGSTATS_)
	#define DIDBGX(__x__)	AJA_sDEBUG	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#else
	#define DIDBGX(__x__)	
#endif

/////////////// CLASS METHODS

NTV2StringList CNTV2DriverInterface::GetLegalSchemeNames (void)
{
	NTV2StringList result;
	result.push_back("ntv2nub"); result.push_back("ntv2"); result.push_back("ntv2local");
	return result;
}

static bool		gSharedMode(false);
void CNTV2DriverInterface::SetShareMode (const bool inSharedMode)		{gSharedMode = inSharedMode;}
bool CNTV2DriverInterface::GetShareMode (void)	{return gSharedMode;}
static bool		gOverlappedMode(false);
void CNTV2DriverInterface::SetOverlappedMode (const bool inOverlapMode) {gOverlappedMode = inOverlapMode;}
bool CNTV2DriverInterface::GetOverlappedMode (void) {return gOverlappedMode;}


/////////////// INSTANCE METHODS

CNTV2DriverInterface::CNTV2DriverInterface ()
	:	_boardNumber					(0)
		,_boardID						(DEVICE_ID_NOTFOUND)
		,_boardOpened					(false)
#if defined(NTV2_WRITEREG_PROFILING)
		,mRecordRegWrites				(false)
		,mSkipRegWrites					(false)
#endif
		,_programStatus					(0)
		,_pRPCAPI						(AJA_NULL)
		,mInterruptEventHandles			()
		,mEventCounts					()
#if defined(NTV2_WRITEREG_PROFILING)
		,mRegWrites						()
		,mRegWritesLock					()
#endif	//	NTV2_WRITEREG_PROFILING
#if !defined(NTV2_DEPRECATE_16_0)
		,_pFrameBaseAddress				(AJA_NULL)
		,_pRegisterBaseAddress			(AJA_NULL)
		,_pRegisterBaseAddressLength	(0)
		,_pXena2FlashBaseAddress		(AJA_NULL)
		,_pCh1FrameBaseAddress			(AJA_NULL)
		,_pCh2FrameBaseAddress			(AJA_NULL)
#endif	//	!defined(NTV2_DEPRECATE_16_0)
		,_ulNumFrameBuffers				(0)
		,_ulFrameBufferSize				(0)
#if !defined(NTV2_DEPRECATE_16_0)
		,_pciSlot						(0)			//	DEPRECATE!
#endif	//	!defined(NTV2_DEPRECATE_16_0)
{
	mInterruptEventHandles.reserve(eNumInterruptTypes);
	while (mInterruptEventHandles.size() < eNumInterruptTypes)
		mInterruptEventHandles.push_back(AJA_NULL);

	mEventCounts.reserve(eNumInterruptTypes);
	while (mEventCounts.size() < eNumInterruptTypes)
		mEventCounts.push_back(0);
	AJAAtomic::Increment(&gConstructCount);
	DIDBGX(DEC(gConstructCount) << " constructed, " << DEC(gDestructCount) << " destroyed");
}	//	constructor


CNTV2DriverInterface::~CNTV2DriverInterface ()
{
	AJAAtomic::Increment(&gDestructCount);
	if (_pRPCAPI)
		delete _pRPCAPI;
	_pRPCAPI = AJA_NULL;
	DIDBGX(DEC(gConstructCount) << " constructed, " << DEC(gDestructCount) << " destroyed");
}	//	destructor

CNTV2DriverInterface & CNTV2DriverInterface::operator = (const CNTV2DriverInterface & inRHS)
{	(void) inRHS;	NTV2_ASSERT(false && "Not assignable"); return *this;}	//	operator =

CNTV2DriverInterface::CNTV2DriverInterface (const CNTV2DriverInterface & inObjToCopy)
{	(void) inObjToCopy; NTV2_ASSERT(false && "Not copyable");}	//	copy constructor


//	Open local physical device (via ajantv2 driver)
bool CNTV2DriverInterface::Open (const UWord inDeviceIndex)
{
	if (IsOpen()  &&  inDeviceIndex == GetIndexNumber())
		return true;	//	Same local device requested, already open
	Close();
	if (inDeviceIndex >= MaxNumDevices())
		{DIFAIL("Requested device index '" << DEC(inDeviceIndex) << "' at/past limit of '" << DEC(MaxNumDevices()) << "'"); return false;}
	if (!OpenLocalPhysical(inDeviceIndex))
	{
		//	Check for virtual device...
		NTV2DeviceInfo info;
		if (!CNTV2DeviceScanner::GetDeviceInfo (inDeviceIndex, info))
			return false;
		if (!Open(info.vdevUrl))
			return false;	//	Open vdev urlSpec failed
		setDeviceIndexNumber(UWord(info.deviceIndex));	//	Patch _boardNumber
		return true;
	}	//	if OpenLocalPhysical failed

#if !defined(NTV2_ALLOW_OPEN_UNSUPPORTED)
	//	Check if device is officially supported...
	const NTV2DeviceIDSet legalDeviceIDs(::NTV2GetSupportedDevices());
	if (legalDeviceIDs.find(_boardID) == legalDeviceIDs.end())
	{
		DIFAIL("Device ID " << xHEX0N(_boardID,8) << " (at device index " << inDeviceIndex << ") is not in list of supported devices");
		Close();
		return false;
	}
#endif	//	NTV2_ALLOW_OPEN_UNSUPPORTED

	//	Read driver version...
	uint16_t	drvrVersComps[4]	=	{0, 0, 0, 0};
	ULWord		driverVersionRaw	(0);
	if (!IsRemote()	 &&	 !ReadRegister (kVRegDriverVersion, driverVersionRaw))
		{DIFAIL("ReadRegister(kVRegDriverVersion) failed");	 Close();  return false;}
	drvrVersComps[0] = uint16_t(NTV2DriverVersionDecode_Major(driverVersionRaw));	//	major
	drvrVersComps[1] = uint16_t(NTV2DriverVersionDecode_Minor(driverVersionRaw));	//	minor
	drvrVersComps[2] = uint16_t(NTV2DriverVersionDecode_Point(driverVersionRaw));	//	point
	drvrVersComps[3] = uint16_t(NTV2DriverVersionDecode_Build(driverVersionRaw));	//	build

	//	Check driver version (local devices only)
	NTV2_ASSERT(!IsRemote());
	if (!(AJA_NTV2_SDK_VERSION_MAJOR))
		DIWARN ("Driver version v" << DEC(drvrVersComps[0]) << "." << DEC(drvrVersComps[1]) << "." << DEC(drvrVersComps[2]) << "."
				<< DEC(drvrVersComps[3]) << " ignored for client SDK v0.0.0.0 (dev mode), driverVersionRaw=" << xHEX0N(driverVersionRaw,8));
	else if (drvrVersComps[0] == uint16_t(AJA_NTV2_SDK_VERSION_MAJOR))
		DIDBG ("Driver v" << DEC(drvrVersComps[0]) << "." << DEC(drvrVersComps[1])
				<< "." << DEC(drvrVersComps[2]) << "." << DEC(drvrVersComps[3]) << " == client SDK v"
				<< DEC(uint16_t(AJA_NTV2_SDK_VERSION_MAJOR)) << "." << DEC(uint16_t(AJA_NTV2_SDK_VERSION_MINOR))
				<< "." << DEC(uint16_t(AJA_NTV2_SDK_VERSION_POINT)) << "." << DEC(uint16_t(AJA_NTV2_SDK_BUILD_NUMBER)));
	else
		DIWARN ("Driver v" << DEC(drvrVersComps[0]) << "." << DEC(drvrVersComps[1])
				<< "." << DEC(drvrVersComps[2]) << "." << DEC(drvrVersComps[3]) << " != client SDK v"
				<< DEC(uint16_t(AJA_NTV2_SDK_VERSION_MAJOR)) << "." << DEC(uint16_t(AJA_NTV2_SDK_VERSION_MINOR)) << "."
				<< DEC(uint16_t(AJA_NTV2_SDK_VERSION_POINT)) << "." << DEC(uint16_t(AJA_NTV2_SDK_BUILD_NUMBER))
				<< ", driverVersionRaw=" << xHEX0N(driverVersionRaw,8));

	FinishOpen();
	AJAAtomic::Increment(&gOpenCount);
	DIDBGX(DEC(gOpenCount) << " opened, " << DEC(gCloseCount) << " closed");
	return true;
}

//	Open remote or virtual device
bool CNTV2DriverInterface::Open (const string & inURLSpec)
{
	Close();
	NTV2_ASSERT(_pRPCAPI == AJA_NULL);
	const NTV2DeviceSpecParser specParser (inURLSpec);
	if (specParser.HasErrors())
		{DIFAIL("Bad device specification '" << inURLSpec << "': " << specParser.Error()); return false;}

	//	URLSpecs can specify locally-attached devices...
	if (specParser.IsLocalDevice())
	{	//	Local device...
		CNTV2Card card;
		if (specParser.HasResult(kConnectParamDevSerial))
		{	if (CNTV2DeviceScanner::GetDeviceWithSerial(specParser.DeviceSerial(), card))
				Open(card.GetIndexNumber());
		}
		else if (specParser.HasResult(kConnectParamDevModel))
		{	if (CNTV2DeviceScanner::GetFirstDeviceWithName(specParser.DeviceModel(), card))
				Open(card.GetIndexNumber());
		}
		else if (specParser.HasResult(kConnectParamDevID))
		{	if (CNTV2DeviceScanner::GetFirstDeviceWithID(specParser.DeviceID(), card))
				Open(card.GetIndexNumber());
		}
		else if (specParser.HasResult(kConnectParamDevIndex))
			Open(specParser.DeviceIndex());
		if (!IsOpen())
			{DIFAIL("Failed to open " << specParser.InfoString());  return false;}
		return true;
	}

	//	Open the remote/virtual device...
	if (!OpenRemote(specParser))
		return false;	//	Failed to open

	FinishOpen();
	AJAAtomic::Increment(&gOpenCount);
	DIDBGX(DEC(gOpenCount) << " opens, " << DEC(gCloseCount) << " closes");
	return true;
}

bool CNTV2DriverInterface::Close (void)
{
	if (IsOpen())
	{
		//	Unsubscribe all...
		for (INTERRUPT_ENUMS eInt(eVerticalInterrupt);  eInt < eNumInterruptTypes;  eInt = INTERRUPT_ENUMS(eInt+1))
			ConfigureSubscription (false, eInt, mInterruptEventHandles[eInt]);

		const bool closeOK(IsRemote() ? CloseRemote() : CloseLocalPhysical());
		if (closeOK)
			AJAAtomic::Increment(&gCloseCount);
		_boardID = DEVICE_ID_NOTFOUND;
		DIDBGX(DEC(gOpenCount) << " opens, " << DEC(gCloseCount) << " closes");
		return closeOK;
	}
	return true;

}	//	Close


bool CNTV2DriverInterface::OpenLocalPhysical (const UWord inDeviceIndex)
{
#if defined(NTV2_NULL_DEVICE)
	DIFAIL("SDK built with 'NTV2_NULL_DEVICE' defined -- cannot OpenLocalPhysical '" << DEC(inDeviceIndex) << "'");
#else	//	else defined(NTV2_NULL_DEVICE)
	(void) inDeviceIndex;
	NTV2_ASSERT(false && "Requires platform-specific implementation");
#endif	//	else NTV2_NULL_DEVICE defined
	return false;
}

bool CNTV2DriverInterface::CloseLocalPhysical (void)
{
	NTV2_ASSERT(false && "Requires platform-specific implementation");
	return false;
}

#if defined(AJA_WINDOWS)
	static bool winsock_inited = false;
	static WSADATA wsaData;

	static void initWinsock(void)
	{
		int wret;
		if (!winsock_inited)
			wret = WSAStartup(MAKEWORD(2,2), &wsaData);
		winsock_inited = true;
	}
#endif	//	AJA_WINDOWS


bool CNTV2DriverInterface::OpenRemote (const NTV2DeviceSpecParser & inParser)
{
#if defined(AJA_WINDOWS)
	initWinsock();
#endif	//	defined(AJA_WINDOWS)
	NTV2_ASSERT(!IsOpen()); //	Must be closed!
	NTV2_ASSERT(_pRPCAPI == AJA_NULL);

#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (inParser.Failed())
		{ostringstream errs; inParser.PrintErrors(errs); DIFAIL("Bad parser: " << errs.str());  return false;}
	if (inParser.IsLocalDevice())
		{DIFAIL("Parser infers local device: " << inParser.InfoString());  return false;}

	NTV2Dictionary connectParams(inParser.Results());
	//	This "connectParams" dictionary has keys/values that determine which plugin to load,
	//	and any other configuration parameters specified by the caller. This dictionary is
	//	modified during the call to "CreateClient" (below) by an NTV2PluginLoader that loads,
	//	interrogates and validates the plugin.  Several new keys/values are added to it during
	//	this process that describe the plugin, its signature, and any query parameters it
	//	requires or accepts for further configuration.
	DIDBG("Opening " << inParser.InfoString());
	NTV2RPCAPI * pClient (NTV2RPCClientAPI::CreateClient(connectParams));
	if (!pClient)
		return false;	//	Failed to instantiate plugin client

	//	At this point, the plugin's NTV2RPCAPI object exists, but may or may not be useable,
	//	depending on if it's "IsConnected". Before SDK 17.1, the plugin's NTV2Connect function
	//	was commonly called directly from its constructor. After SDK 17.1.0, OpenRemote is
	//	responsible for calling NTV2Connect, to allow tools like NTV2Watcher to probe the
	//	plugin in stages via its client interface.
	if (!pClient->IsConnected())
		if (!pClient->NTV2Connect())
		{	DIFAIL("Failed to connect/open '" << inParser.DeviceSpec() << "'");
			delete pClient;
			CloseRemote();
			return false;
		}

	//	NTV2 physical devices always have a hardware identity -- the NTV2DeviceID read from register 50.
	//	This plugin device is considered "open" if ReadRegister is successful, and returns a non-zero
	//	value that's also not DEVICE_ID_NOTFOUND. (Virtual/software devices that have no NTV2 hardware
	//	corollary should return DEVICE_ID_SOFTWARE.)
	_pRPCAPI = pClient;
	_boardOpened = ReadRegister(kRegBoardID, _boardID)  &&  _boardID  &&  _boardID != DEVICE_ID_NOTFOUND;
	if (!IsRemote() || !IsOpen())
		DIFAIL("Failed to open '" << inParser.DeviceSpec() << "'");
	return IsRemote() && IsOpen();	//	Fail if not remote nor open
#else	//	NTV2_NUB_CLIENT_SUPPORT
	DIFAIL("SDK built without 'NTV2_NUB_CLIENT_SUPPORT' -- cannot OpenRemote '" << inParser.DeviceSpec() << "'");
	return false;
#endif	//	NTV2_NUB_CLIENT_SUPPORT
}	//	OpenRemote


bool CNTV2DriverInterface::CloseRemote (void)
{
	if (_pRPCAPI)
	{
		DIDBG("Closing remote: " << *_pRPCAPI);
		if (_pRPCAPI->NTV2Disconnect())
			DIINFO("Remote closed: " << *_pRPCAPI);
		else
			DIFAIL("Remote close (NTV2Disconnect) failed: " << *_pRPCAPI);
		delete _pRPCAPI;
		_pRPCAPI = AJA_NULL;
		_boardOpened = false;
		return true;
	}
	//	Wasn't open
	_boardOpened = false;
	return false;
}


bool CNTV2DriverInterface::GetInterruptEventCount (const INTERRUPT_ENUMS inInterrupt, ULWord & outCount)
{
	outCount = 0;
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(inInterrupt))
		return false;
	outCount = mEventCounts.at(inInterrupt);
	return true;
}

bool CNTV2DriverInterface::SetInterruptEventCount (const INTERRUPT_ENUMS inInterrupt, const ULWord inCount)
{
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(inInterrupt))
		return false;
	mEventCounts.at(inInterrupt) = inCount;
	return true;
}

bool CNTV2DriverInterface::GetInterruptCount (const INTERRUPT_ENUMS eInterrupt,	 ULWord & outCount)
{	(void) eInterrupt;
	outCount = 0;
	NTV2_ASSERT(false && "Needs subclass implementation");
	return false;
}

HANDLE CNTV2DriverInterface::GetInterruptEvent (const INTERRUPT_ENUMS eInterruptType)
{
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(eInterruptType))
		return HANDLE(0);
	return HANDLE(uint64_t(mInterruptEventHandles.at(eInterruptType)));
}

bool CNTV2DriverInterface::ConfigureInterrupt (const bool bEnable, const INTERRUPT_ENUMS eInterruptType)
{	(void) bEnable;	 (void) eInterruptType;
	NTV2_ASSERT(false && "Needs subclass implementation");
	return false;
}

bool CNTV2DriverInterface::ConfigureSubscription (const bool bSubscribe, const INTERRUPT_ENUMS eInterruptType, PULWord & outSubscriptionHdl)
{
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(eInterruptType))
		return false;
	outSubscriptionHdl = mInterruptEventHandles.at(eInterruptType);
	if (bSubscribe)
	{										//	If subscribing,
		mEventCounts [eInterruptType] = 0;	//		clear this interrupt's event counter
		DIDBG("Subscribing '" << ::NTV2InterruptEnumString(eInterruptType) << "' (" << UWord(eInterruptType)
				<< "), event counter reset");
	}
	else
	{
		DIDBGX("Unsubscribing '" << ::NTV2InterruptEnumString(eInterruptType) << "' (" << UWord(eInterruptType) << "), "
				<< mEventCounts[eInterruptType] << " event(s) received");
	}
	return true;

}	//	ConfigureSubscription


NTV2DeviceID CNTV2DriverInterface::GetDeviceID (void)
{
	ULWord value(0);
	if (IsOpen()  &&  ReadRegister(kRegBoardID, value))
	{
		const NTV2DeviceID currentValue(NTV2DeviceID(value+0));
		if (currentValue != _boardID)
			DIWARN(xHEX0N(this,16) << ":  NTV2DeviceID " << xHEX0N(value,8) << " (" << ::NTV2DeviceIDToString(currentValue)
					<< ") read from register " << kRegBoardID << " doesn't match _boardID " << xHEX0N(_boardID,8) << " ("
					<< ::NTV2DeviceIDToString(_boardID) << ")");
		return currentValue;
	}
	return DEVICE_ID_NOTFOUND;
}


// Common remote card read register.  Subclasses have overloaded function
// that does platform-specific read of register on local card.
bool CNTV2DriverInterface::ReadRegister (const ULWord inRegNum, ULWord & outValue, const ULWord inMask, const ULWord inShift)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return _pRPCAPI->NTV2ReadRegisterRemote (inRegNum, outValue, inMask, inShift);
#else
	(void) inRegNum;	(void) outValue;	(void) inMask;	(void) inShift;
#endif
	return false;
}

bool CNTV2DriverInterface::ReadRegisters (NTV2RegisterReads & inOutValues)
{
	if (!IsOpen())
		return false;		//	Device not open!
	if (inOutValues.empty())
		return true;		//	Nothing to do!

	NTV2GetRegisters getRegsParams (inOutValues);
	if (NTV2Message(reinterpret_cast<NTV2_HEADER*>(&getRegsParams)))
	{
		if (!getRegsParams.GetRegisterValues(inOutValues))
			return false;
	}
	else	//	Non-atomic user-space workaround until GETREGS implemented in driver...
		for (NTV2RegisterReadsIter iter(inOutValues.begin());  iter != inOutValues.end();  ++iter)
			if (iter->registerNumber != kRegXenaxFlashDOUT) //	Prevent firmware erase/program/verify failures
				if (!ReadRegister (iter->registerNumber, iter->registerValue))
					return false;
	return true;
}

#if !defined(NTV2_DEPRECATE_16_0)
	// Common remote card read multiple registers.	Subclasses have overloaded function
	bool CNTV2DriverInterface::ReadRegisterMulti (const ULWord inNumRegs, ULWord * pOutWhichRegFailed, NTV2RegInfo pOutRegInfos[])
	{
		if (!pOutWhichRegFailed)
			return false;	//	NULL pointer
		*pOutWhichRegFailed = 0xFFFFFFFF;
		if (!inNumRegs)
			return false;	//	numRegs is zero

		//	New in SDK 16.0:  Use ReadRegs NTV2Message
		NTV2RegReads regReads, result;
		regReads.reserve(inNumRegs);  result.reserve(inNumRegs);
		for (size_t ndx(0);	 ndx < size_t(inNumRegs);  ndx++)
			regReads.push_back(pOutRegInfos[ndx]);
		result = regReads;
		bool retVal (ReadRegisters(result));
		NTV2_ASSERT(result.size() <= regReads.size());
		if (result.size() < regReads.size())
			*pOutWhichRegFailed = result.empty() ? regReads.front().registerNumber : result.back().registerNumber;
		return retVal;
	}

	Word CNTV2DriverInterface::SleepMs (const LWord milliseconds)
	{
		AJATime::Sleep(milliseconds);
		return 0; // Beware, this function always returns zero, even if sleep was interrupted
	}
#endif	//	!defined(NTV2_DEPRECATE_16_0)


// Common remote card write register.  Subclasses overloaded this to do platform-specific register write.
bool CNTV2DriverInterface::WriteRegister (const ULWord inRegNum, const ULWord inValue, const ULWord inMask, const ULWord inShift)
{
#if defined(NTV2_WRITEREG_PROFILING)
	//	Recording is done in platform-specific WriteRegister
#endif	//	NTV2_WRITEREG_PROFILING
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	//	If we get here, must be a non-physical device connection...
	return IsRemote() ? _pRPCAPI->NTV2WriteRegisterRemote(inRegNum, inValue, inMask, inShift) : false;
#else
	(void) inRegNum;	(void) inValue; (void) inMask;	(void) inShift;
	return false;
#endif
}


bool CNTV2DriverInterface::DmaTransfer (const NTV2DMAEngine inDMAEngine,
										const bool			inIsRead,
										const ULWord		inFrameNumber,
										ULWord *			pFrameBuffer,
										const ULWord		inCardOffsetBytes,
										const ULWord		inTotalByteCount,
										const bool			inSynchronous)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	NTV2Buffer buffer(pFrameBuffer, inTotalByteCount);
	return _pRPCAPI->NTV2DMATransferRemote(inDMAEngine, inIsRead, inFrameNumber, buffer, inCardOffsetBytes,
											0/*numSegs*/,	 0/*hostPitch*/,  0/*cardPitch*/, inSynchronous);
#else
	(void) inDMAEngine; (void) inIsRead;	(void) inFrameNumber;	(void) pFrameBuffer;	(void) inCardOffsetBytes;
	(void) inTotalByteCount;	(void) inSynchronous;
	return false;
#endif
}

bool CNTV2DriverInterface::DmaTransfer (const NTV2DMAEngine inDMAEngine,
										const bool			inIsRead,
										const ULWord		inFrameNumber,
										ULWord *			pFrameBuffer,
										const ULWord		inCardOffsetBytes,
										const ULWord		inTotalByteCount,
										const ULWord		inNumSegments,
										const ULWord		inHostPitchPerSeg,
										const ULWord		inCardPitchPerSeg,
										const bool			inSynchronous)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	NTV2Buffer buffer(pFrameBuffer, inTotalByteCount);
	return _pRPCAPI->NTV2DMATransferRemote(inDMAEngine, inIsRead, inFrameNumber, buffer, inCardOffsetBytes,
											inNumSegments, inHostPitchPerSeg, inCardPitchPerSeg, inSynchronous);
#else
	(void) inDMAEngine; (void) inIsRead;	(void) inFrameNumber;	(void) pFrameBuffer;	(void) inCardOffsetBytes;
	(void) inTotalByteCount;	(void) inNumSegments;	(void) inHostPitchPerSeg;	(void) inCardPitchPerSeg;	(void) inSynchronous;
	return false;
#endif
}

bool CNTV2DriverInterface::DmaTransfer (const NTV2DMAEngine inDMAEngine,
									const NTV2Channel inDMAChannel,
									const bool inIsTarget,
									const ULWord inFrameNumber,
									const ULWord inCardOffsetBytes,
									const ULWord inByteCount,
									const ULWord inNumSegments,
									const ULWord inSegmentHostPitch,
									const ULWord inSegmentCardPitch,
									const PCHANNEL_P2P_STRUCT & inP2PData)
{	(void) inDMAEngine; (void) inDMAChannel; (void) inIsTarget; (void) inFrameNumber; (void) inCardOffsetBytes;
	(void) inByteCount; (void) inNumSegments; (void) inSegmentHostPitch; (void) inSegmentCardPitch; (void) inP2PData;
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT(IsRemote());
	//	No NTV2DMATransferP2PRemote implementation yet
#endif
	return false;
}

// Common remote card waitforinterrupt.	 Subclasses have overloaded function
// that does platform-specific waitforinterrupt on local cards.
bool CNTV2DriverInterface::WaitForInterrupt (INTERRUPT_ENUMS eInterrupt, ULWord timeOutMs)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	return _pRPCAPI ? _pRPCAPI->NTV2WaitForInterruptRemote(eInterrupt, timeOutMs) : false;
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
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		switch (autoCircData.eCommand)
		{
			case eStartAutoCirc:
			case eAbortAutoCirc:
			case ePauseAutoCirc:
			case eFlushAutoCirculate:
			case eGetAutoCirc:
			case eStopAutoCirc:
			case eInitAutoCirc:
			case eSetActiveFrame:
				return _pRPCAPI->NTV2AutoCirculateRemote(autoCircData);
			default:	// Others not handled
				return false;
		}
	return false;
#else
	(void) autoCircData;
	return false;
#endif
}

bool CNTV2DriverInterface::NTV2Message (NTV2_HEADER * pInMessage)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	return _pRPCAPI ? _pRPCAPI->NTV2MessageRemote(pInMessage) : false;
#else
	(void) pInMessage;
	return false;
#endif
}


// Common remote card DriverGetBitFileInformation.	Subclasses have overloaded function
// that does platform-specific function on local cards.
bool CNTV2DriverInterface::DriverGetBitFileInformation (BITFILE_INFO_STRUCT & bitFileInfo, const NTV2BitFileType bitFileType)
{	(void)bitFileType;
	::memset(&bitFileInfo, 0, sizeof(bitFileInfo));
	if (IsRemote())
		return false;
	if (!::NTV2DeviceHasSPIFlash(_boardID))
		return false;

	ParseFlashHeader(bitFileInfo);
	bitFileInfo.bitFileType = 0;
	switch (_boardID)
	{
		case DEVICE_ID_CORVID1:						bitFileInfo.bitFileType = NTV2_BITFILE_CORVID1_MAIN;				break;
		case DEVICE_ID_CORVID22:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID22_MAIN;				break;
		case DEVICE_ID_CORVID24:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID24_MAIN;				break;
		case DEVICE_ID_CORVID3G:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID3G_MAIN;				break;
		case DEVICE_ID_CORVID44:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44;					break;
		case DEVICE_ID_CORVID44_2X4K:				bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_2X4K_MAIN;			break;
		case DEVICE_ID_CORVID44_8K:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_8K_MAIN;			break;
		case DEVICE_ID_CORVID44_8KMK:				bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_8KMK_MAIN;			break;
		case DEVICE_ID_CORVID44_PLNR:				bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_PLNR_MAIN;			break;
		case DEVICE_ID_CORVID88:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVID88;					break;
		case DEVICE_ID_CORVIDHBR:					bitFileInfo.bitFileType = NTV2_BITFILE_NUMBITFILETYPES;				break;
		case DEVICE_ID_CORVIDHEVC:					bitFileInfo.bitFileType = NTV2_BITFILE_CORVIDHEVC;					break;
		case DEVICE_ID_IO4K:						bitFileInfo.bitFileType = NTV2_BITFILE_IO4K_MAIN;					break;
		case DEVICE_ID_IO4KPLUS:					bitFileInfo.bitFileType = NTV2_BITFILE_IO4KPLUS_MAIN;				break;
		case DEVICE_ID_IO4KUFC:						bitFileInfo.bitFileType = NTV2_BITFILE_IO4KUFC_MAIN;				break;
		case DEVICE_ID_IOEXPRESS:					bitFileInfo.bitFileType = NTV2_BITFILE_IOEXPRESS_MAIN;				break;
		case DEVICE_ID_IOIP_2022:					bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2022;					break;
		case DEVICE_ID_IOIP_2110:					bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2110;					break;
		case DEVICE_ID_IOIP_2110_RGB12:				bitFileInfo.bitFileType = NTV2_BITFILE_IOIP_2110_RGB12;				break;
		case DEVICE_ID_IOXT:						bitFileInfo.bitFileType = NTV2_BITFILE_IOXT_MAIN;					break;
		case DEVICE_ID_KONA1:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA1;						break;
		case DEVICE_ID_KONA3G:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA3G_MAIN;					break;
		case DEVICE_ID_KONA3GQUAD:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA3G_QUAD;					break;
		case DEVICE_ID_KONA4:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA4_MAIN;					break;
		case DEVICE_ID_KONA4UFC:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA4UFC_MAIN;				break;
		case DEVICE_ID_KONA5:						bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_MAIN;					break;
		case DEVICE_ID_KONA5_2X4K:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_2X4K_MAIN;				break;
		case DEVICE_ID_KONA5_3DLUT:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_3DLUT_MAIN;			break;
		case DEVICE_ID_KONA5_8K:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_8K_MAIN;				break;
		case DEVICE_ID_KONA5_8KMK:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_8KMK_MAIN;				break;
		case DEVICE_ID_KONA5_OE1:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE1_MAIN;				break;
		case DEVICE_ID_KONA5_OE2:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE2_MAIN;				break;
		case DEVICE_ID_KONA5_OE3:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE3_MAIN;				break;
		case DEVICE_ID_KONA5_OE4:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE4_MAIN;				break;
		case DEVICE_ID_KONA5_OE5:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE5_MAIN;				break;
		case DEVICE_ID_KONA5_OE6:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE6_MAIN;				break;
		case DEVICE_ID_KONA5_OE7:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE7_MAIN;				break;
		case DEVICE_ID_KONA5_OE8:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE8_MAIN;				break;
		case DEVICE_ID_KONA5_OE9:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE9_MAIN;				break;
		case DEVICE_ID_KONA5_OE10:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE10_MAIN;				break;
		case DEVICE_ID_KONA5_OE11:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE11_MAIN;				break;
		case DEVICE_ID_KONA5_OE12:					bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_OE12_MAIN;				break;
		case DEVICE_ID_SOJI_OE1:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_OE1_MAIN;				break;
		case DEVICE_ID_SOJI_OE2:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_OE2_MAIN;				break;
		case DEVICE_ID_SOJI_OE3:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_OE3_MAIN;				break;
		case DEVICE_ID_SOJI_OE4:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_OE4_MAIN;				break;
		case DEVICE_ID_SOJI_OE5:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_OE5_MAIN;				break;
		case DEVICE_ID_SOJI_OE6:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_OE6_MAIN;				break;
		case DEVICE_ID_SOJI_OE7:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_OE7_MAIN;				break;
		case DEVICE_ID_SOJI_3DLUT:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_3DLUT_MAIN;				break;
		case DEVICE_ID_SOJI_DIAGS:					bitFileInfo.bitFileType = NTV2_BITFILE_SOJI_DIAGS_MAIN;				break;
		case DEVICE_ID_KONA5_8K_MV_TX:				bitFileInfo.bitFileType = NTV2_BITFILE_KONA5_8K_MV_TX_MAIN;			break;
		case DEVICE_ID_KONAHDMI:					bitFileInfo.bitFileType = NTV2_BITFILE_KONAHDMI;					break;
		case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:		bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_1RX_1TX_1SFP_J2K;		break;
		case DEVICE_ID_KONAIP_1RX_1TX_2110:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_1RX_1TX_2110;			break;
		case DEVICE_ID_KONAIP_2022:					bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2022;					break;
		case DEVICE_ID_KONAIP_2110:					bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2110;					break;
		case DEVICE_ID_KONAIP_2110_RGB12:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2110_RGB12;			break;
		case DEVICE_ID_KONAIP_2TX_1SFP_J2K:			bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_2TX_1SFP_J2K;			break;
		case DEVICE_ID_KONAIP_4CH_2SFP:				bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_4CH_2SFP;				break;
		case DEVICE_ID_KONALHEPLUS:					bitFileInfo.bitFileType = NTV2_BITFILE_KONALHE_PLUS;				break;
		case DEVICE_ID_KONALHI:						bitFileInfo.bitFileType = NTV2_BITFILE_LHI_MAIN;					break;
		case DEVICE_ID_KONALHIDVI:					bitFileInfo.bitFileType = NTV2_BITFILE_NUMBITFILETYPES;				break;
		case DEVICE_ID_TTAP:						bitFileInfo.bitFileType = NTV2_BITFILE_TTAP_MAIN;					break;
		case DEVICE_ID_TTAP_PRO:					bitFileInfo.bitFileType = NTV2_BITFILE_TTAP_PRO_MAIN;				break;
		case DEVICE_ID_IOX3:						bitFileInfo.bitFileType = NTV2_BITFILE_IOX3_MAIN;					break;
		case DEVICE_ID_KONAX:						bitFileInfo.bitFileType = NTV2_BITFILE_KONAX;						break;
		case DEVICE_ID_KONAXM:						bitFileInfo.bitFileType = NTV2_BITFILE_KONAXM;						break;
        case DEVICE_ID_KONAIP_25G:					bitFileInfo.bitFileType = NTV2_BITFILE_KONAIP_25G;					break;
    case DEVICE_ID_IP25_T:
		case DEVICE_ID_IP25_R:						bitFileInfo.bitFileType = NTV2_BITFILE_TYPE_INVALID;				break;
		case DEVICE_ID_CORVID44_GEN3:				bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_GEN3;				break;
		case DEVICE_ID_CORVID88_GEN3:				bitFileInfo.bitFileType = NTV2_BITFILE_CORVID44_GEN3;				break;
		case DEVICE_ID_SOFTWARE:
		case DEVICE_ID_NOTFOUND:					bitFileInfo.bitFileType = NTV2_BITFILE_TYPE_INVALID;				break;
	#if !defined (_DEBUG)
		default:					break;
	#endif
	}
	bitFileInfo.checksum = 0;
	bitFileInfo.structVersion = 0;
	bitFileInfo.structSize = sizeof(BITFILE_INFO_STRUCT);
	bitFileInfo.whichFPGA = eFPGAVideoProc;

	const string bitFileDesignNameString = string(bitFileInfo.designNameStr) + ".bit";
	::strncpy(bitFileInfo.designNameStr, bitFileDesignNameString.c_str(), sizeof(bitFileInfo.designNameStr)-1);
	return true;
}

bool CNTV2DriverInterface::GetPackageInformation (PACKAGE_INFO_STRUCT & packageInfo)
{
	if (!IsDeviceReady(false) || !IsIPDevice())
		return false;	// cannot read flash

	string packInfo;
	ULWord deviceID = ULWord(_boardID);
	ReadRegister (kRegBoardID, deviceID);

	if (CNTV2AxiSpiFlash::DeviceSupported(NTV2DeviceID(deviceID)))
	{
		CNTV2AxiSpiFlash spiFlash(GetIndexNumber(), false);

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
			return false;
	}
	else
	{
		ULWord baseAddress = (16 * 1024 * 1024) - (3 * 256 * 1024);
		const ULWord dwordSizeCount = 256/4;

		WriteRegister(kRegXenaxFlashAddress, ULWord(1));   // bank 1
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

		packInfo = reinterpret_cast<char*>(bitFilePtr);
		delete [] bitFilePtr;
	}

	istringstream iss(packInfo);
	vector<string> results;
	string token;
	while (getline(iss,token, ' '))
		results.push_back(token);

	if (results.size() < 8)
		return false;

	packageInfo.date = results[1];
	token = results[2];
	token.erase(remove(token.begin(), token.end(), '\n'), token.end());
	packageInfo.time = token;
	packageInfo.buildNumber	  = results[4];
	packageInfo.packageNumber = results[7];
	return true;
}

// Common remote card DriverGetBuildInformation.  Subclasses have overloaded function
// that does platform-specific function on local cards.
bool CNTV2DriverInterface::DriverGetBuildInformation (BUILD_INFO_STRUCT & buildInfo)
{
	::memset(&buildInfo, 0, sizeof(buildInfo));
	return false;
}

bool CNTV2DriverInterface::BitstreamWrite (const NTV2Buffer & inBuffer, const bool inFragment, const bool inSwap)
{
	NTV2Bitstream bsMsg (inBuffer,
						 BITSTREAM_WRITE |
						 (inFragment? BITSTREAM_FRAGMENT : 0) |
						 (inSwap? BITSTREAM_SWAP : 0));
	ULWord counts(0);
	ReadRegister(kVRegDynFirmwareUpdateCounts, counts);
	ULWord attempts(counts >> 16), successes(counts & 0x0000FFFF);
	attempts++;
	const bool result (NTV2Message (reinterpret_cast<NTV2_HEADER*>(&bsMsg)));
	if (result)
		successes++;
	counts = (attempts << 16) | successes;
	WriteRegister(kVRegDynFirmwareUpdateCounts, counts);
	return result;
}

bool CNTV2DriverInterface::BitstreamReset (const bool inConfiguration, const bool inInterface)
{
	NTV2Buffer inBuffer;
	NTV2Bitstream bsMsg (inBuffer,
						 (inConfiguration? BITSTREAM_RESET_CONFIG : 0) |
						 (inInterface? BITSTREAM_RESET_MODULE : 0));
	return NTV2Message(bsMsg);
}

bool CNTV2DriverInterface::BitstreamStatus (NTV2ULWordVector & outRegValues)
{
	outRegValues.reserve(BITSTREAM_MCAP_DATA);
	outRegValues.clear();

	NTV2Buffer inBuffer;
	NTV2Bitstream bsMsg (inBuffer, BITSTREAM_READ_REGISTERS);
	if (!NTV2Message(bsMsg))
		return false;

	for (UWord ndx(0);	ndx < BITSTREAM_MCAP_DATA;	ndx++)
		outRegValues.push_back(bsMsg.mRegisters[ndx]);

	return true;
}

bool CNTV2DriverInterface::BitstreamLoad (const bool inSuspend, const bool inResume)
{
	NTV2Buffer inBuffer;
	NTV2Bitstream bsMsg (inBuffer,
				(inSuspend? BITSTREAM_SUSPEND : 0) |
				(inResume? BITSTREAM_RESUME : 0));
	return NTV2Message(bsMsg);
}

bool CNTV2DriverInterface::StreamChannelOps (const NTV2Channel inChannel,
												ULWord flags,
												NTV2StreamChannel& status)
{
	status.mChannel = inChannel;
	status.mFlags = flags;

	return NTV2Message(status);
}

bool CNTV2DriverInterface::StreamBufferOps (const NTV2Channel inChannel,
												NTV2Buffer& inBuffer,
												ULWord64 bufferCookie,
												ULWord flags,
												NTV2StreamBuffer& status)
{
	status.mChannel = inChannel;
	status.mBuffer.Set (inBuffer.GetHostPointer(), inBuffer.GetByteCount());
	status.mBufferCookie = bufferCookie;
	status.mFlags = flags;

	return NTV2Message(status);
}

bool CNTV2DriverInterface::MailBufferOps (const NTV2Channel inChannel,
										  NTV2Buffer& inBuffer,
                                          ULWord dataSize,
                                          ULWord flags,
                                          ULWord delay,
                                          ULWord timeout,
                                          NTV2MailBuffer& status)
{
	status.mChannel = inChannel;
	status.mBuffer.Set (inBuffer.GetHostPointer(), inBuffer.GetByteCount());
	status.mDataSize = dataSize;
	status.mDelay = delay;
	status.mTimeout = timeout;
	status.mFlags = flags;

	return NTV2Message(status);
}

// FinishOpen
// NOTE _boardID must be set before calling this routine.
void CNTV2DriverInterface::FinishOpen (void)
{
	// HACK! FinishOpen needs frame geometry to determine frame buffer size and number.
	NTV2FrameGeometry fg(NTV2_FG_INVALID);
	ULWord val1(0), val2(0);
	ReadRegister (kRegGlobalControl, fg, kRegMaskGeometry, kRegShiftGeometry);	//	Read FrameGeometry
	ReadRegister (kRegCh1Control, val1, kRegMaskFrameFormat, kRegShiftFrameFormat); //	Read PixelFormat
	ReadRegister (kRegCh1Control, val2, kRegMaskFrameFormatHiBit, kRegShiftFrameFormatHiBit);
	NTV2PixelFormat pf(NTV2PixelFormat((val1 & 0x0F) | ((val2 & 0x1) << 4)));
	_ulFrameBufferSize = ::NTV2DeviceGetFrameBufferSize(_boardID, fg, pf);
	_ulNumFrameBuffers = ::NTV2DeviceGetNumberFrameBuffers(_boardID, fg, pf);

	ULWord returnVal1 = false;
	ULWord returnVal2 = false;
	if (::NTV2DeviceCanDo4KVideo(_boardID))
		ReadRegister(kRegGlobalControl2, returnVal1, kRegMaskQuadMode, kRegShiftQuadMode);
	if (::NTV2DeviceCanDo425Mux(_boardID))
		ReadRegister(kRegGlobalControl2, returnVal2, kRegMask425FB12, kRegShift425FB12);

#if !defined(NTV2_DEPRECATE_16_0)
	_pFrameBaseAddress = AJA_NULL;
	_pRegisterBaseAddress = AJA_NULL;
	_pRegisterBaseAddressLength = 0;
	_pXena2FlashBaseAddress	 = AJA_NULL;
	_pCh1FrameBaseAddress = AJA_NULL;
	_pCh2FrameBaseAddress = AJA_NULL;
#endif	//	!defined(NTV2_DEPRECATE_16_0)

}	//	FinishOpen


bool CNTV2DriverInterface::ParseFlashHeader (BITFILE_INFO_STRUCT & bitFileInfo)
{
	if (!IsDeviceReady(false))
		return false;	// cannot read flash

	if (::NTV2DeviceGetSPIFlashVersion(_boardID) == 4)
	{
		uint32_t val;
		ReadRegister((0x100000 + 0x08) / 4, val);
		if (val != 0x01)
			return false;	// cannot read flash
	}

	if (::NTV2DeviceGetSPIFlashVersion(_boardID) >= 3  &&  ::NTV2DeviceGetSPIFlashVersion(_boardID) <= 5)
	{
		WriteRegister(kRegXenaxFlashAddress, 0ULL);
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
		} while (busy  &&  timeoutCount);
		if (!timeoutCount)
			return false;
	}

	//	Allocate header buffer, read/fill from SPI-flash...
	static const ULWord dwordCount(256/4);
	NTV2Buffer bitFileHdrBuffer(dwordCount * sizeof(ULWord));
	if (!bitFileHdrBuffer)
		return false;

	ULWord* pULWord(bitFileHdrBuffer),	baseAddress(0);
	for (ULWord count(0);  count < dwordCount;	count++, baseAddress += 4)
		if (!ReadFlashULWord(baseAddress, pULWord[count]))
			return false;

	CNTV2Bitfile fileInfo;
	std::string headerError;
#if 0	//	Fake out:
	if (_boardID == DEVICE_ID_TTAP_PRO) //	Fake TTapPro -- load "flash" from on-disk bitfile:
	{	fileInfo.Open("/Users/demo/dev-svn/firmware/T3_Tap/t_tap_pro.bit");
		headerError = fileInfo.GetLastError();
	} else
#endif
	headerError = fileInfo.ParseHeaderFromBuffer(bitFileHdrBuffer);
	if (headerError.empty())
	{
		::strncpy(bitFileInfo.dateStr, fileInfo.GetDate().c_str(), NTV2_BITFILE_DATETIME_STRINGLENGTH);
		::strncpy(bitFileInfo.timeStr, fileInfo.GetTime().c_str(), NTV2_BITFILE_DATETIME_STRINGLENGTH);
		::strncpy(bitFileInfo.designNameStr, fileInfo.GetDesignName().c_str(), NTV2_BITFILE_DESIGNNAME_STRINGLENGTH);
		::strncpy(bitFileInfo.partNameStr, fileInfo.GetPartName().c_str(), NTV2_BITFILE_PARTNAME_STRINGLENGTH);
		bitFileInfo.numBytes = ULWord(fileInfo.GetProgramStreamLength());
	}
	return headerError.empty();
}	//	ParseFlashHeader

bool CNTV2DriverInterface::ReadFlashULWord (const ULWord inAddress, ULWord & outValue, const ULWord inRetryCount)
{
	if (!WriteRegister(kRegXenaxFlashAddress, inAddress))
		return false;
	if (!WriteRegister(kRegXenaxFlashControlStatus, 0x0B))
		return false;
	bool busy(true);
	ULWord timeoutCount(inRetryCount);
	do
	{
		ULWord regValue(0);
		ReadRegister(kRegXenaxFlashControlStatus, regValue);
		if (regValue & BIT(8))
		{
			busy = true;
			timeoutCount--;
		}
		else
			busy = false;
	} while (busy  &&  timeoutCount);
	if (!timeoutCount)
		return false;
	return ReadRegister(kRegXenaxFlashDOUT, outValue);
}

void CNTV2DriverInterface::setDeviceIndexNumber (const UWord num)
{
	_boardNumber = num;
}


//--------------------------------------------------------------------------------------------------------------------
//	Application acquire and release stuff
//--------------------------------------------------------------------------------------------------------------------
const uint32_t	kAgentAppFcc (NTV2_FOURCC('A','j','a','A'));


bool CNTV2DriverInterface::AcquireStreamForApplicationWithReference (const ULWord inAppCode, const int32_t inProcessID)
{
	ULWord svcInitialized(0);
	if (ReadRegister(kVRegServicesInitialized, svcInitialized))
		if (!svcInitialized)	//	if services have never initialized the device
			if (inAppCode != kAgentAppFcc)	//	if not AJA Agent
				DIWARN(::NTV2DeviceIDToString(GetDeviceID()) << "-" << DEC(GetIndexNumber())
					<< " uninitialized by AJAAgent, requesting app " << xHEX0N(inAppCode,8) << ", pid=" << DEC(inProcessID));

	ULWord currentCode(0), currentPID(0);
	if (!ReadRegister(kVRegApplicationCode, currentCode) || !ReadRegister(kVRegApplicationPID, currentPID))
		return false;

	// Check if owner is deceased
	if (!AJAProcess::IsValid(currentPID))
	{
		// Process doesn't exist, so make the board our own
		ReleaseStreamForApplication (currentCode, int32_t(currentPID));
	}

	if (!ReadRegister(kVRegApplicationCode, currentCode) || !ReadRegister(kVRegApplicationPID, currentPID))
		return false;

	for (int count(0);	count < 20;	 count++)
	{
		if (!currentPID)
		{
			// Nothing has the board
			if (!WriteRegister(kVRegApplicationCode, inAppCode))
				return false;
			// Just in case this is not zero
			WriteRegister(kVRegAcquireLinuxReferenceCount, 0);
			WriteRegister(kVRegAcquireLinuxReferenceCount, 1);
			return WriteRegister(kVRegApplicationPID, ULWord(inProcessID));
		}
		else if (currentCode == inAppCode  &&  currentPID == ULWord(inProcessID))
			return WriteRegister(kVRegAcquireLinuxReferenceCount, 1);	// Process already acquired, so bump the count
		// Someone else has the board, so wait and try again
		AJATime::Sleep(50);
	}
	return false;
}

bool CNTV2DriverInterface::ReleaseStreamForApplicationWithReference (const ULWord inAppCode, const int32_t inProcessID)
{
	ULWord currentCode(0), currentPID(0), currentCount(0);
	if (!ReadRegister(kVRegApplicationCode, currentCode)
		|| !ReadRegister(kVRegApplicationPID, currentPID)
		|| !ReadRegister(kVRegAcquireLinuxReferenceCount, currentCount))
			return false;

	if (currentCode == inAppCode  &&  currentPID == ULWord(inProcessID))
	{
		if (currentCount > 1)
			return WriteRegister(kVRegReleaseLinuxReferenceCount, 1);
		if (currentCount == 1)
			return ReleaseStreamForApplication(inAppCode, inProcessID);
		return true;
	}
	return false;
}

bool CNTV2DriverInterface::AcquireStreamForApplication (const ULWord inAppCode, const int32_t inProcessID)
{
	ULWord svcInitialized(0);
	if (ReadRegister(kVRegServicesInitialized, svcInitialized))
		if (!svcInitialized)	//	if services have never initialized the device
			if (inAppCode != kAgentAppFcc)	//	if not AJA Agent
				DIWARN(::NTV2DeviceIDToString(GetDeviceID()) << "-" << DEC(GetIndexNumber())
					<< " uninitialized by AJAAgent, requesting app " << xHEX0N(inAppCode,8) << ", pid=" << DEC(inProcessID));

	//	Loop for a while trying to acquire the board
	for (int count(0);	count < 20;	 count++)
	{
		if (WriteRegister(kVRegApplicationCode, inAppCode))
			return WriteRegister(kVRegApplicationPID, ULWord(inProcessID));
		AJATime::Sleep(50);
	}

	// Get data about current owner
	ULWord currentCode(0), currentPID(0);
	if (!ReadRegister(kVRegApplicationCode, currentCode) || !ReadRegister(kVRegApplicationPID, currentPID))
		return false;

	//	Check if owner is deceased
	if (!AJAProcess::IsValid(currentPID))
	{	// Process doesn't exist, so make the board our own
		ReleaseStreamForApplication (currentCode, int32_t(currentPID));
		for (int count(0);	count < 20;	 count++)
		{
			if (WriteRegister(kVRegApplicationCode, inAppCode))
				return WriteRegister(kVRegApplicationPID, ULWord(inProcessID));
			AJATime::Sleep(50);
		}
	}
	// Current owner is alive, so don't interfere
	return false;
}

bool CNTV2DriverInterface::ReleaseStreamForApplication (const ULWord inAppCode, const int32_t inProcessID)
{	(void)inAppCode;	//	Don't care which appCode
	if (WriteRegister(kVRegReleaseApplication, ULWord(inProcessID)))
	{
		WriteRegister(kVRegAcquireLinuxReferenceCount, 0);
		return true;	// We don't care if the above call failed
	}
	return false;
}

bool CNTV2DriverInterface::SetStreamingApplication (const ULWord inAppCode, const int32_t inProcessID)
{
	if (!WriteRegister(kVRegForceApplicationCode, inAppCode))
		return false;
	return WriteRegister(kVRegForceApplicationPID, ULWord(inProcessID));
}

bool CNTV2DriverInterface::GetStreamingApplication (ULWord & outAppType, int32_t & outProcessID)
{
	if (!ReadRegister(kVRegApplicationCode, outAppType))
		return false;
	return CNTV2DriverInterface::ReadRegister(kVRegApplicationPID, outProcessID);
}

string CNTV2DriverInterface::GetDescription (void) const
{
	if (!IsRemote())
		return "";	//	This implementation is intended for remote/plugin devices
	string desc(_pRPCAPI->Description());
	if (desc.empty())
	{	//	Plugin/remote device didn't provide description
		const NTV2Dictionary parms(ConnectParams());
		NTV2StringList strs;
		if (parms.hasKey(kNTV2PluginRegInfoKey_LongName))
			strs.push_back("\"" + parms.valueForKey(kNTV2PluginRegInfoKey_LongName) + "\"");
		else if (parms.hasKey(kNTV2PluginRegInfoKey_ShortName))
			strs.push_back(parms.valueForKey(kNTV2PluginRegInfoKey_ShortName));
		if (parms.hasKey(kNTV2PluginRegInfoKey_Description))
			strs.push_back(parms.valueForKey(kNTV2PluginRegInfoKey_Description));
		else if (parms.hasKey(kNTV2PluginRegInfoKey_Copyright))
			strs.push_back(parms.valueForKey(kNTV2PluginRegInfoKey_Copyright));
		else if (parms.hasKey(kNTV2PluginRegInfoKey_Vendor))
			strs.push_back(parms.valueForKey(kNTV2PluginRegInfoKey_Vendor));
		if (!strs.empty())
			desc = aja::join(strs, ", ");
	}
	return desc;
}

NTV2Dictionary CNTV2DriverInterface::ConnectParams (void) const
{
	return IsRemote() ? _pRPCAPI->ConnectParams() : NTV2Dictionary();
}

//	This function is used by the retail ControlPanel.
//	Read the current RP188 registers (which typically give you the timecode corresponding to the LAST frame).
//	NOTE:	This is a hack to avoid making a "real" driver call! Since the RP188 data requires three ReadRegister()
//			calls, there is a chance that it can straddle a VBI, which could give bad results. To avoid this, we
//			read the 3 registers until we get two consecutive passes that give us the same data. (Someday it'd
//			be nice if the driver automatically read these as part of its VBI IRQ handler...
bool CNTV2DriverInterface::ReadRP188Registers (const NTV2Channel inChannel, RP188_STRUCT * pRP188Data)
{	(void) inChannel;
	if (!pRP188Data)
		return false;

	RP188_STRUCT rp188;
	NTV2DeviceID boardID = DEVICE_ID_NOTFOUND;
	RP188SourceFilterSelect source = kRP188SourceEmbeddedLTC;
	ULWord dbbReg(0), msReg(0), lsReg(0);

	CNTV2DriverInterface::ReadRegister(kRegBoardID, boardID);
	CNTV2DriverInterface::ReadRegister(kVRegRP188SourceSelect, source);
	bool bLTCPort = (source == kRP188SourceLTCPort);

	// values come from LTC port registers
	if (bLTCPort)
	{
		ULWord ltcPresent;
		ReadRegister (kRegStatus, ltcPresent, kRegMaskLTCInPresent, kRegShiftLTCInPresent);

		// there is no equivalent DBB for LTC port - we synthesize it here
		rp188.DBB = (ltcPresent) ? 0xFE000000 | NEW_SELECT_RP188_RCVD : 0xFE000000;

		// LTC port registers
		dbbReg = 0; // don't care - does not exist
		msReg = kRegLTCAnalogBits0_31;
		lsReg  = kRegLTCAnalogBits32_63;
	}
	else
	{
		// values come from RP188 registers
		NTV2Channel channel = NTV2_CHANNEL1;
		NTV2InputVideoSelect inputSelect = NTV2_Input1Select;

		if (::NTV2DeviceGetNumVideoInputs(boardID) > 1)
		{
			CNTV2DriverInterface::ReadRegister (kVRegInputSelect, inputSelect);
			channel = (inputSelect == NTV2_Input2Select) ? NTV2_CHANNEL2 : NTV2_CHANNEL1;
		}
		else
			channel = NTV2_CHANNEL1;

		// rp188 registers
		dbbReg = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1DBB : kRegRP188InOut2DBB);
		//Check to see if TC is received
		uint32_t tcReceived = 0;
		ReadRegister(dbbReg, tcReceived, BIT(16), 16);
		if(tcReceived == 0)
			return false;//No TC recevied

		ReadRegister (dbbReg, rp188.DBB, kRegMaskRP188DBB, kRegShiftRP188DBB );
		switch (rp188.DBB)//What do we have?
		{
			default:
			case 0x01:
			case 0x02:
			{
				//We have VITC - what do we want?
				if (pRP188Data->DBB == 0x01 || pRP188Data->DBB == 0x02)
				{	//	We want VITC
					msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
					lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
				}
				else
				{	//	We want Embedded LTC, so we should check one other place
					uint32_t ltcPresent = 0;
					ReadRegister(dbbReg, ltcPresent, BIT(18), 18);
					if (ltcPresent != 1)
						return false;
					//Read LTC registers
					msReg  = (channel == NTV2_CHANNEL1 ? kRegLTCEmbeddedBits0_31  : kRegLTC2EmbeddedBits0_31 );
					lsReg  = (channel == NTV2_CHANNEL1 ? kRegLTCEmbeddedBits32_63 : kRegLTC2EmbeddedBits32_63);
				}
				break;
			}
			case 0x00:
				//We have LTC - do we want it?
				if (pRP188Data->DBB != 0x00)
					return false;
				msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
				lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
				break;
		}
		//Re-Read the whole register just in case something is expecting other status values
		ReadRegister (dbbReg, rp188.DBB);
	}
	ReadRegister (msReg,  rp188.Low );
	ReadRegister (lsReg,  rp188.High);

	// register stability filter
	do
	{
		*pRP188Data = rp188;	// struct copy to result

		// read again into local struct
		if (!bLTCPort)
			ReadRegister (dbbReg, rp188.DBB);
		ReadRegister (msReg,  rp188.Low );
		ReadRegister (lsReg,  rp188.High);

		// if the new read equals the previous read, consider it done
		if (rp188.DBB  == pRP188Data->DBB  &&
			rp188.Low  == pRP188Data->Low  &&
			rp188.High == pRP188Data->High)
				break;
	} while (true);

	return true;
}

void CNTV2DriverInterface::BumpEventCount (const INTERRUPT_ENUMS eInterruptType)
{
	if (NTV2_IS_VALID_INTERRUPT_ENUM(eInterruptType))
		mEventCounts[eInterruptType] += 1;

}	//	BumpEventCount


bool CNTV2DriverInterface::IsDeviceReady (const bool checkValid)
{
    if (!IsIPDevice() && !::NTV2DeviceCanDo25GIP(GetDeviceID()))
		return true;	//	Non-IP devices always ready
    
    if (IsIPDevice())
    {
        if (!IsMBSystemReady())
            return false;
    
        if (checkValid && !IsMBSystemValid())
            return false;
    }
    else
    {
        if (!IsLPSystemReady())
            return false;
    }

	return true;	//	Ready!
}

bool CNTV2DriverInterface::IsMBSystemValid (void)
{
	if (IsIPDevice())
	{
		uint32_t val;
		ReadRegister(SAREK_REGS + kRegSarekIfVersion, val);
		return val == SAREK_IF_VERSION;
	}
	return true;
}

bool CNTV2DriverInterface::IsMBSystemReady (void)
{
	if (!IsIPDevice())
		return false;	//	No microblaze

	uint32_t val;
	ReadRegister(SAREK_REGS + kRegSarekMBState, val);
	if (val != 0x01)
		return false;	//	MB not ready

	// Not enough to read MB State, we need to make sure MB is running
	ReadRegister(SAREK_REGS + kRegSarekMBUptime, val);
	return (val < 2) ? false : true;
}

bool CNTV2DriverInterface::IsLPSystemReady (void)
{
    if (!::NTV2DeviceCanDo25GIP(GetDeviceID()))
        return false;	//	No local proc
    
    uint32_t val;
    ReadRegister(kRegReserved83, val);
    if (val == 0x00)
        return false;	//	MB not ready

    return true;
}

#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	bool CNTV2DriverInterface::GetRecordedRegisterWrites (NTV2RegisterWrites & outRegWrites) const
	{
		AJAAutoLock autoLock(&mRegWritesLock);
		outRegWrites = mRegWrites;
		return true;
	}

	bool CNTV2DriverInterface::StartRecordRegisterWrites (const bool inSkipActualWrites)
	{
		AJAAutoLock autoLock(&mRegWritesLock);
		if (mRecordRegWrites)
			return false;	//	Already recording
		mRegWrites.clear();
		mRecordRegWrites = true;
		mSkipRegWrites = inSkipActualWrites;
		return true;
	}

	bool CNTV2DriverInterface::ResumeRecordRegisterWrites (void)
	{	//	Identical to Start, but don't clear mRegWrites nor change mSkipRegWrites
		AJAAutoLock autoLock(&mRegWritesLock);
		if (mRecordRegWrites)
			return false;	//	Already recording
		mRecordRegWrites = true;
		return true;
	}

	bool CNTV2DriverInterface::IsRecordingRegisterWrites (void) const
	{	//	NB: This will return false if paused
		AJAAutoLock autoLock(&mRegWritesLock);
		return mRecordRegWrites;
	}

	bool CNTV2DriverInterface::StopRecordRegisterWrites (void)
	{
		AJAAutoLock autoLock(&mRegWritesLock);
		mRecordRegWrites = mSkipRegWrites = false;
		return true;
	}

	bool CNTV2DriverInterface::PauseRecordRegisterWrites (void)
	{	//	Identical to Stop, but don't change mSkipRegWrites
		AJAAutoLock autoLock(&mRegWritesLock);
		if (!mRecordRegWrites)
			return false;	//	Already stopped/paused
		mRecordRegWrites = false;
		return true;
	}

	ULWord CNTV2DriverInterface::GetNumRecordedRegisterWrites (void) const
	{
		AJAAutoLock autoLock(&mRegWritesLock);
		return ULWord(mRegWrites.size());
	}
#endif	//	NTV2_WRITEREG_PROFILING


ULWordSet CNTV2DriverInterface::GetSupportedItems (const NTV2EnumsID inEnumsID)
{
	ULWordSet result;
	if (!IsOpen())
		return result;

	//	Remote/virtual device gets first dibs on answering...
	if (IsRemote()  &&  _pRPCAPI->NTV2GetSupportedRemote (inEnumsID, result))
		return result;

	const NTV2DeviceID devID(GetDeviceID());
	switch (inEnumsID)
	{
		case kNTV2EnumsID_DeviceID:
		{	const NTV2DeviceIDSet devIDs(::NTV2GetSupportedDevices());
			for (NTV2DeviceIDSetConstIter it(devIDs.begin());  it != devIDs.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_Standard:
		{	NTV2StandardSet standards;
			::NTV2DeviceGetSupportedStandards (devID, standards);
			for (NTV2StandardSetConstIter it(standards.begin());  it != standards.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_PixelFormat:
		{	NTV2FrameBufferFormatSet pfs;
			::NTV2DeviceGetSupportedPixelFormats (devID, pfs);
			for (NTV2FrameBufferFormatSetConstIter it(pfs.begin());  it != pfs.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_FrameGeometry:
		{	NTV2GeometrySet fgs;
			::NTV2DeviceGetSupportedGeometries (devID, fgs);
			for (NTV2GeometrySetConstIter it(fgs.begin());  it != fgs.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_FrameRate:
		{	NTV2FrameRateSet frs;
			::NTV2DeviceGetSupportedFrameRates (devID, frs);
			for (NTV2FrameRateSetConstIter it(frs.begin());  it != frs.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_ScanGeometry:
		{	//	Needs implementation
			break;
		}
		case kNTV2EnumsID_VideoFormat:
		{	NTV2VideoFormatSet vfs;
			::NTV2DeviceGetSupportedVideoFormats (devID, vfs);
			for (NTV2VideoFormatSetConstIter it(vfs.begin());  it != vfs.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_Mode:
		{	if (IsSupported(kDeviceCanDoPlayback))	result.insert(ULWord(NTV2_MODE_OUTPUT));
			if (IsSupported(kDeviceCanDoCapture))	result.insert(ULWord(NTV2_MODE_INPUT));
			break;
		}
		case kNTV2EnumsID_InputSource:
		{	NTV2InputSourceSet srcs;
			::NTV2DeviceGetSupportedInputSources (devID, srcs);
			for (NTV2InputSourceSetConstIter it(srcs.begin());  it != srcs.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_OutputDest:
		{	NTV2OutputDestinations dsts;
			::NTV2DeviceGetSupportedOutputDests (devID, dsts);
			for (NTV2OutputDestinationsConstIter it(dsts.begin());  it != dsts.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_Channel:
		{	for (ULWord ch(0);  ch < GetNumSupported(kDeviceGetNumFrameStores);  ch++)
				result.insert(ch);
			break;
		}
		case kNTV2EnumsID_RefSource:
		{	if (!IsSupported(kDeviceCanDoPlayback))
				break;
			NTV2InputSourceSet inpSrcs;
			::NTV2DeviceGetSupportedInputSources (devID, inpSrcs);
			for (NTV2InputSourceSetConstIter it(inpSrcs.begin());  it != inpSrcs.end();  ++it)
			{	const NTV2ReferenceSource refSrc (::NTV2InputSourceToReferenceSource(*it));
				if (NTV2_IS_VALID_NTV2ReferenceSource(refSrc))
					if (result.find(ULWord(refSrc)) == result.end())
						result.insert(ULWord(refSrc));
			}
			result.insert(ULWord(NTV2_REFERENCE_FREERUN));	//	Always include Free-Run
			if (GetNumSupported(kDeviceGetNumReferenceVideoInputs))
				result.insert(ULWord(NTV2_REFERENCE_EXTERNAL));	//	Has external reference
			break;
		}
		case kNTV2EnumsID_AudioRate:
		{	result.insert(ULWord(NTV2_AUDIO_48K));	//	All boards support 48KHz PCM
			if (IsSupported(kDeviceCanDoAudio96K))	result.insert(ULWord(NTV2_AUDIO_96K));
			if (IsSupported(kDeviceCanDoAudio192K))	result.insert(ULWord(NTV2_AUDIO_192K));
			break;
		}
		case kNTV2EnumsID_AudioSource:
		{	if (!IsSupported(kDeviceCanDoCapture))
				break;
			NTV2InputSourceSet inpSrcs;
			::NTV2DeviceGetSupportedInputSources (devID, inpSrcs);
			for (NTV2InputSourceSetConstIter it(inpSrcs.begin());  it != inpSrcs.end();  ++it)
			{	const NTV2AudioSource audSrc (::NTV2InputSourceToAudioSource(*it));
				if (NTV2_IS_VALID_AUDIO_SOURCE(audSrc))
					if (result.find(ULWord(audSrc)) == result.end())
						result.insert(ULWord(audSrc));
			}
			break;
		}
		case kNTV2EnumsID_WidgetID:
		{	NTV2WidgetIDSet wgts;
			CNTV2SignalRouter::GetWidgetIDs (devID, wgts);
			for (NTV2WidgetIDSetConstIter it(wgts.begin());  it != wgts.end();  ++it)
				result.insert(ULWord(*it));
			break;
		}
		case kNTV2EnumsID_ConversionMode:
		{	for (NTV2ConversionMode cm(NTV2_1080i_5994to525_5994);  cm < NTV2_NUM_CONVERSIONMODES;  cm = NTV2ConversionMode(cm+1))
				if (::NTV2DeviceCanDoConversionMode (devID, cm))
					result.insert(ULWord(cm));
			break;
		}
		case kNTV2EnumsID_InputTCIndex:
		{	for (NTV2TCIndex ndx(NTV2_TCINDEX_DEFAULT);  ndx < NTV2_MAX_NUM_TIMECODE_INDEXES;  ndx = NTV2TCIndex(ndx+1))
				if (::NTV2DeviceCanDoInputTCIndex (devID, ndx))
					result.insert(ULWord(ndx));
			break;
		}
		case kNTV2EnumsID_OutputTCIndex:
		{	for (NTV2TCIndex ndx(NTV2_TCINDEX_DEFAULT);  ndx < NTV2_MAX_NUM_TIMECODE_INDEXES;  ndx = NTV2TCIndex(ndx+1))
				if (::NTV2DeviceCanDoOutputTCIndex (devID, ndx))
					result.insert(ULWord(ndx));
			break;
		}
		default:	break;
	}
	return result;
}

bool CNTV2DriverInterface::GetBoolParam (const ULWord inParamID, ULWord & outValue)
{
	//	Remote/virtual device gets first dibs on answering...
	if (IsRemote()  &&  _pRPCAPI->NTV2GetBoolParamRemote (inParamID, outValue))
		return true;

	//	Is there a register/bit that will answer this query?
	const NTV2BoolParamID paramID (NTV2BoolParamID(inParamID+0));
	{	NTV2RegInfo regInfo;   bool invertSense(false);
		if (GetRegInfoForBoolParam (paramID, regInfo, invertSense))
		{
			if (!ReadRegister (regInfo.registerNumber, regInfo.registerValue, regInfo.registerMask, regInfo.registerShift))
				return false;
			if (invertSense)
				outValue = regInfo.registerValue ? 0 : 1;
			else
				outValue = regInfo.registerValue ? 1 : 0;
			return true;
		}
	}

	//	Call classic device features function...
	const NTV2DeviceID devID (GetDeviceID());
	switch (inParamID)
	{
		case kDeviceCanChangeEmbeddedAudioClock:	outValue = ::NTV2DeviceCanChangeEmbeddedAudioClock(devID);			break;	//	Deprecate?
		case kDeviceCanChangeFrameBufferSize:		outValue = ::NTV2DeviceCanChangeFrameBufferSize(devID);				break;
		case kDeviceCanDisableUFC:					outValue = ::NTV2DeviceCanDisableUFC(devID);						break;
		case kDeviceCanDo2KVideo:					outValue = ::NTV2DeviceCanDo2KVideo(devID);							break;
		case kDeviceCanDo3GLevelConversion:			outValue = ::NTV2DeviceCanDo3GLevelConversion(devID);				break;
		case kDeviceCanDoRGBLevelAConversion:		outValue = ::NTV2DeviceCanDoRGBLevelAConversion(devID);				break;
		case kDeviceCanDo425Mux:					outValue = ::NTV2DeviceCanDo425Mux(devID);							break;
		case kDeviceCanDo4KVideo:					outValue = ::NTV2DeviceCanDo4KVideo(devID);							break;
		case kDeviceCanDoAESAudioIn:				outValue = GetNumSupported(kDeviceGetNumAESAudioInputChannels) > 0;	break;
		case kDeviceCanDoAESAudioOut:				outValue = GetNumSupported(kDeviceGetNumAESAudioOutputChannels) > 0;break;
		case kDeviceCanDoAnalogAudio:				outValue = ::NTV2DeviceCanDoAnalogAudio(devID);						break;
		case kDeviceCanDoAnalogVideoIn:				outValue = ::NTV2DeviceCanDoAnalogVideoIn(devID);					break;
		case kDeviceCanDoAnalogVideoOut:			outValue = ::NTV2DeviceCanDoAnalogVideoOut(devID);					break;
		case kDeviceCanDoAudio2Channels:			outValue = GetNumSupported(kDeviceGetMaxAudioChannels) >= 2;		break;	//	Deprecate?
		case kDeviceCanDoAudio6Channels:			outValue = GetNumSupported(kDeviceGetMaxAudioChannels) >= 6;		break;	//	Deprecate?
		case kDeviceCanDoAudio8Channels:			outValue = GetNumSupported(kDeviceGetMaxAudioChannels) >= 8;		break;	//	Deprecate?
		case kDeviceCanDoAudio96K:					outValue = ::NTV2DeviceCanDoAudio96K(devID);						break;	//	Deprecate?
		case kDeviceCanDoAudioDelay:				outValue = ::NTV2DeviceCanDoAudioDelay(devID);						break;	//	Deprecate?
		case kDeviceCanDoAudioInput:				outValue =	(GetNumSupported(kDeviceGetNumVideoInputs)
																+ GetNumSupported(kDeviceGetNumHDMIAudioInputChannels)
																+ GetNumSupported(kDeviceGetNumAESAudioInputChannels)
																+ GetNumSupported(kDeviceGetNumAnalogAudioInputChannels)) > 0;break;
		case kDeviceCanDoAudioOutput:				outValue =	(GetNumSupported(kDeviceGetNumVideoOutputs)
																+ GetNumSupported(kDeviceGetNumHDMIAudioOutputChannels)
																+ GetNumSupported(kDeviceGetNumAESAudioOutputChannels)
																+ GetNumSupported(kDeviceGetNumAnalogAudioOutputChannels)) > 0;break;
		case kDeviceCanDoBreakoutBoard:				outValue = ::NTV2DeviceCanDoBreakoutBoard(devID);					break;
		case kDeviceCanDoBreakoutBox:				outValue = ::NTV2DeviceCanDoBreakoutBox(devID);						break;
		case kDeviceCanDoCapture:					outValue =	(GetNumSupported(kDeviceGetNumVideoInputs)
																+ GetNumSupported(kDeviceGetNumHDMIVideoInputs)
																+ GetNumSupported(kDeviceGetNumAnalogVideoInputs)) > 0;	break;
		case kDeviceCanDoColorCorrection:			outValue = GetNumSupported(kDeviceGetNumLUTs) > 0;					break;	//	Deprecate?
		case kDeviceCanDoCustomAnc:					outValue = ::NTV2DeviceCanDoCustomAnc(devID);						break;	//	Deprecate?

		//	FOR NOW:	kDeviceCanDoCustomHancInsertion
		//				REMOVE THIS CASE ONCE ALL KONA5 & CORVID44/12G & KONAX FIRMWARE SETS kRegCanDoStatus BIT(2):
		case kDeviceCanDoCustomHancInsertion:		outValue =	   devID == DEVICE_ID_IO4KPLUS
																|| devID == DEVICE_ID_KONA5				|| devID == DEVICE_ID_KONA5_2X4K
																|| devID == DEVICE_ID_KONA5_8K			|| devID == DEVICE_ID_KONA5_3DLUT
																|| devID == DEVICE_ID_KONA5_8K_MV_TX	|| devID == DEVICE_ID_CORVID44_8KMK
																|| devID == DEVICE_ID_CORVID44_8K		|| devID == DEVICE_ID_CORVID44_2X4K
																|| devID == DEVICE_ID_CORVID44_PLNR		|| devID == DEVICE_ID_KONAX;
													break;

		case kDeviceCanDoDSKOpacity:				outValue = ::NTV2DeviceCanDoDSKOpacity(devID);						break;	//	Deprecate?
		case kDeviceCanDoDualLink:					outValue = ::NTV2DeviceCanDoDualLink(devID);						break;	//	Deprecate?
		case kDeviceCanDoDVCProHD:					outValue = ::NTV2DeviceCanDoDVCProHD(devID);						break;	//	Deprecate?
		case kDeviceCanDoEnhancedCSC:				outValue = ::NTV2DeviceCanDoEnhancedCSC(devID);						break;	//	Deprecate?
		case kDeviceCanDoFrameStore1Display:		outValue = ::NTV2DeviceCanDoFrameStore1Display(devID);				break;	//	Deprecate?
		case kDeviceCanDoGPIO:						outValue = ::NTV2DeviceCanDoGPIO(devID);							break;
		case kDeviceCanDoHDMIOutStereo:				outValue = ::NTV2DeviceCanDoHDMIOutStereo(devID);					break;	//	Deprecate?
		case kDeviceCanDoHDV:						outValue = ::NTV2DeviceCanDoHDV(devID);								break;	//	Deprecate?
		case kDeviceCanDoHDVideo:					outValue = ::NTV2DeviceCanDoHDVideo(devID);							break;	//	Deprecate?
		case kDeviceCanDoIsoConvert:				outValue = ::NTV2DeviceCanDoIsoConvert(devID);						break;
		case kDeviceCanDoLTC:						outValue = ::NTV2DeviceCanDoLTC(devID);								break;
		case kDeviceCanDoLTCInOnRefPort:			outValue = ::NTV2DeviceCanDoLTCInOnRefPort(devID);					break;
		case kDeviceCanDoMSI:						outValue = ::NTV2DeviceCanDoMSI(devID);								break;
		case kDeviceCanDoMultiFormat:				outValue = ::NTV2DeviceCanDoMultiFormat(devID);						break;
		case kDeviceCanDoPCMControl:				outValue = ::NTV2DeviceCanDoPCMControl(devID);						break;
		case kDeviceCanDoPCMDetection:				outValue = ::NTV2DeviceCanDoPCMDetection(devID);					break;
		case kDeviceCanDoPIO:						outValue = ::NTV2DeviceCanDoPIO(devID);								break;	//	Deprecate?
		case kDeviceCanDoPlayback:					outValue =	(GetNumSupported(kDeviceGetNumVideoOutputs)
																+ GetNumSupported(kDeviceGetNumHDMIVideoOutputs)
																+ GetNumSupported(kDeviceGetNumAnalogVideoOutputs)) > 0;break;
		case kDeviceCanDoProgrammableCSC:			outValue = GetNumSupported(kDeviceGetNumCSCs) > 0;					break;
		case kDeviceCanDoProgrammableRS422:			outValue = ::NTV2DeviceCanDoProgrammableRS422(devID);				break;
		case kDeviceCanDoProRes:					outValue = ::NTV2DeviceCanDoProRes(devID);							break;
		case kDeviceCanDoQREZ:						outValue = ::NTV2DeviceCanDoQREZ(devID);							break;
		case kDeviceCanDoQuarterExpand:				outValue = ::NTV2DeviceCanDoQuarterExpand(devID);					break;
		case kDeviceCanDoRateConvert:				outValue = ::NTV2DeviceCanDoRateConvert(devID);						break;	//	Deprecate?
		case kDeviceCanDoRGBPlusAlphaOut:			outValue = ::NTV2DeviceCanDoRGBPlusAlphaOut(devID);					break;	//	Deprecate?
		case kDeviceCanDoRP188:						outValue = ::NTV2DeviceCanDoRP188(devID);							break;	//	Deprecate?
		case kDeviceCanDoSDVideo:					outValue = ::NTV2DeviceCanDoSDVideo(devID);							break;	//	Deprecate?
		case kDeviceCanDoSDIErrorChecks:			outValue = ::NTV2DeviceCanDoSDIErrorChecks(devID);					break;
		case kDeviceCanDoStackedAudio:				outValue = ::NTV2DeviceCanDoStackedAudio(devID);					break;	//	Deprecate?
		case kDeviceCanDoStereoIn:					outValue = ::NTV2DeviceCanDoStereoIn(devID);						break;	//	Deprecate?
		case kDeviceCanDoStereoOut:					outValue = ::NTV2DeviceCanDoStereoOut(devID);						break;	//	Deprecate?
		case kDeviceCanDoThunderbolt:				outValue = ::NTV2DeviceCanDoThunderbolt(devID);						break;
		case kDeviceCanDoVideoProcessing:			outValue = ::NTV2DeviceCanDoVideoProcessing(devID);					break;
		case kDeviceCanMeasureTemperature:			outValue = ::NTV2DeviceCanMeasureTemperature(devID);				break;
		case kDeviceCanReportFrameSize:				outValue = ::NTV2DeviceCanReportFrameSize(devID);					break;
		case kDeviceHasBiDirectionalSDI:			outValue = ::NTV2DeviceHasBiDirectionalSDI(devID);					break;
		case kDeviceHasBracketLED:					outValue = ::NTV2DeviceHasBracketLED(devID);						break;
		case kDeviceHasColorSpaceConverterOnChannel2:	outValue = ::NTV2DeviceCanDoWidget(devID, NTV2_WgtCSC2);		break;	//	Deprecate?
		case kDeviceHasIDSwitch:					outValue = ::NTV2DeviceCanDoIDSwitch(devID);						break;
        case kDeviceHasNTV4FrameStores:				outValue = ::NTV2DeviceHasNTV4FrameStores(devID);                   break;
		case kDeviceHasNWL:							outValue = ::NTV2DeviceHasNWL(devID);								break;
		case kDeviceHasPCIeGen2:					outValue = ::NTV2DeviceHasPCIeGen2(devID);							break;
		case kDeviceHasRetailSupport:				outValue = ::NTV2DeviceHasRetailSupport(devID);						break;
		case kDeviceHasSDIRelays:					outValue = ::NTV2DeviceHasSDIRelays(devID);							break;
		case kDeviceHasSPIFlash:					outValue = ::NTV2DeviceHasSPIFlash(devID);							break;	//	Deprecate?
		case kDeviceHasSPIFlashSerial:				outValue = ::NTV2DeviceHasSPIFlashSerial(devID);					break;	//	Deprecate?
		case kDeviceHasSPIv2:						outValue = GetNumSupported(kDeviceGetSPIFlashVersion) == 2;			break;
		case kDeviceHasSPIv3:						outValue = GetNumSupported(kDeviceGetSPIFlashVersion) == 3;			break;
		case kDeviceHasSPIv4:						outValue = GetNumSupported(kDeviceGetSPIFlashVersion) == 4;			break;
		case kDeviceIs64Bit:						outValue = ::NTV2DeviceIs64Bit(devID);								break;	//	Deprecate?
		case kDeviceIsDirectAddressable:			outValue = ::NTV2DeviceIsDirectAddressable(devID);					break;	//	Deprecate?
		case kDeviceIsExternalToHost:				outValue = ::NTV2DeviceIsExternalToHost(devID);						break;
		case kDeviceIsLocalPhysical:				outValue = !IsRemote();												break;
		case kDeviceIsSupported:					outValue = ::NTV2DeviceIsSupported(devID);							break;
		case kDeviceNeedsRoutingSetup:				outValue = ::NTV2DeviceNeedsRoutingSetup(devID);					break;	//	Deprecate?
		case kDeviceSoftwareCanChangeFrameBufferSize:	outValue = ::NTV2DeviceSoftwareCanChangeFrameBufferSize(devID);	break;
		case kDeviceCanThermostat:					outValue = ::NTV2DeviceCanThermostat(devID);						break;
		case kDeviceHasHEVCM31:						outValue = ::NTV2DeviceHasHEVCM31(devID);							break;
		case kDeviceHasHEVCM30:						outValue = ::NTV2DeviceHasHEVCM30(devID);							break;
		case kDeviceCanDoVITC2:						outValue = ::NTV2DeviceCanDoVITC2(devID);							break;
		case kDeviceCanDoHDMIHDROut:				outValue = ::NTV2DeviceCanDoHDMIHDROut(devID);						break;
		case kDeviceCanDoJ2K:						outValue = ::NTV2DeviceCanDoJ2K(devID);								break;

		case kDeviceCanDo12gRouting:				outValue = ::NTV2DeviceCanDo12gRouting(devID);						break;
		case kDeviceCanDo12GSDI:					outValue = ::NTV2DeviceCanDo12GSDI(devID);							break;
		case kDeviceCanDo2110:						outValue = ::NTV2DeviceCanDo2110(devID);							break;
		case kDeviceCanDo8KVideo:					outValue = ::NTV2DeviceCanDo8KVideo(devID);							break;
		case kDeviceCanDoAudio192K:					outValue = ::NTV2DeviceCanDoAudio192K(devID);						break;
		case kDeviceCanDoHDMIAuxCapture:			outValue = ::NTV2DeviceCanDoCustomAux(devID);						break;
		case kDeviceCanDoHDMIAuxPlayback:			outValue = false && ::NTV2DeviceCanDoCustomAux(devID);				break;	//	SDK 17.1 HDMI AUX is Capture-Only
		case kDeviceCanDoFramePulseSelect:			outValue = ::NTV2DeviceCanDoFramePulseSelect(devID);				break;
		case kDeviceCanDoHDMIMultiView:				outValue = ::NTV2DeviceCanDoHDMIMultiView(devID);					break;
		case kDeviceCanDoHFRRGB:					outValue = ::NTV2DeviceCanDoHFRRGB(devID);							break;
		case kDeviceCanDoIP:						outValue = ::NTV2DeviceCanDoIP(devID);								break;
		case kDeviceCanDoMultiLinkAudio:			outValue = ::NTV2DeviceCanDoMultiLinkAudio(devID);					break;
		case kDeviceCanDoWarmBootFPGA:				outValue = ::NTV2DeviceCanDoWarmBootFPGA(devID);					break;
		case kDeviceCanReportFailSafeLoaded:		outValue = ::NTV2DeviceCanReportFailSafeLoaded(devID);				break;
		case kDeviceCanReportRunningFirmwareDate:	outValue = ::NTV2DeviceCanReportRunningFirmwareDate(devID);			break;
		case kDeviceHasAudioMonitorRCAJacks:		outValue = ::NTV2DeviceHasAudioMonitorRCAJacks(devID);				break;
		case kDeviceHasBiDirectionalAnalogAudio:	outValue = ::NTV2DeviceHasBiDirectionalAnalogAudio(devID);			break;
		case kDeviceHasGenlockv2:					outValue = GetNumSupported(kDeviceGetGenlockVersion) == 2;			break;	//	Deprecate
		case kDeviceHasGenlockv3:					outValue = GetNumSupported(kDeviceGetGenlockVersion) == 3;			break;	//	Deprecate
		case kDeviceHasHeadphoneJack:				outValue = ::NTV2DeviceHasHeadphoneJack(devID);						break;
		case kDeviceHasLEDAudioMeters:				outValue = ::NTV2DeviceHasLEDAudioMeters(devID);					break;
		case kDeviceHasRotaryEncoder:				outValue = ::NTV2DeviceHasRotaryEncoder(devID);						break;
		case kDeviceHasSPIv5:						outValue = ::NTV2DeviceGetSPIFlashVersion(devID) == 5;				break;
		case kDeviceHasXilinxDMA:					outValue = ::NTV2DeviceHasXilinxDMA(devID);							break;
		case kDeviceCanDoStreamingDMA:				outValue = GetDeviceID() == DEVICE_ID_KONAXM;						break;
		case kDeviceHasPWMFanControl:				outValue = ::NTV2DeviceHasPWMFanControl(devID);						break;
		case kDeviceCanDoHDMIQuadRasterConversion:	outValue = (GetNumSupported(kDeviceGetNumHDMIVideoInputs)
																	||  GetNumSupported(kDeviceGetNumHDMIVideoOutputs))	//	At least 1 HDMI in/out
																&& (GetDeviceID() != DEVICE_ID_KONAHDMI)				//	Not a KonaHDMI
																&& (!IsSupported(kDeviceCanDoAudioMixer));				//	No audio mixer
													break;
		case kDeviceROMHasBankSelect:				outValue = GetNumSupported(kDeviceGetSPIFlashVersion) >= 3
																&&  GetNumSupported(kDeviceGetSPIFlashVersion) <= 6;	break;
		case kDeviceCanDoVersalSysMon:				outValue = ::NTV2DeviceCanDoVersalSysMon(devID);					break;
		case kDeviceCanDoAudioMixer:
		case kDeviceHasMicrophoneInput:
		default:									return false;	//	Bad param
	}
	return true;	//	Successfully used old ::NTV2DeviceCanDo function

}	//	GetBoolParam


bool CNTV2DriverInterface::GetNumericParam (const ULWord inParamID, ULWord & outVal)
{
	outVal = 0;

	//	Remote/virtual device gets first dibs on answering...
	if (IsRemote()  &&  _pRPCAPI->NTV2GetNumericParamRemote (inParamID, outVal))
		return true;

	//	Is there a register that will answer this query?
	const NTV2NumericParamID paramID (NTV2NumericParamID(inParamID+0));
	{	NTV2RegInfo regInfo;
		if (GetRegInfoForNumericParam (paramID, regInfo))
			return ReadRegister (regInfo.registerNumber, outVal, regInfo.registerMask, regInfo.registerShift);
	}

	//	Call classic device features function...
	const NTV2DeviceID devID (GetDeviceID());
	switch (paramID)
	{
		case kDeviceGetActiveMemorySize:				outVal = ::NTV2DeviceGetActiveMemorySize (devID);				break;
		case kDeviceGetDACVersion:						outVal = ::NTV2DeviceGetDACVersion (devID);						break;
		case kDeviceGetDownConverterDelay:				outVal = ::NTV2DeviceGetDownConverterDelay (devID);				break;
		case kDeviceGetGenlockVersion:					outVal = ULWord(::NTV2DeviceGetGenlockVersion (devID));			break;
		case kDeviceGetHDMIVersion:						outVal = ::NTV2DeviceGetHDMIVersion (devID);					break;
		case kDeviceGetLUTVersion:						outVal = ::NTV2DeviceGetLUTVersion (devID);						break;
		case kDeviceGetMaxAudioChannels:				outVal = ::NTV2DeviceGetMaxAudioChannels (devID);				break;
		case kDeviceGetMaxRegisterNumber:				outVal = ::NTV2DeviceGetMaxRegisterNumber (devID);				break;
		case kDeviceGetMaxTransferCount:				outVal = ::NTV2DeviceGetMaxTransferCount (devID);				break;
		case kDeviceGetNum2022ChannelsSFP1:				outVal = ::NTV2DeviceGetNum2022ChannelsSFP1 (devID);			break;
		case kDeviceGetNum2022ChannelsSFP2:				outVal = ::NTV2DeviceGetNum2022ChannelsSFP2 (devID);			break;
		case kDeviceGetNum4kQuarterSizeConverters:		outVal = ::NTV2DeviceGetNum4kQuarterSizeConverters (devID);		break;
		case kDeviceGetNumAESAudioInputChannels:		outVal = ::NTV2DeviceGetNumAESAudioInputChannels (devID);		break;
		case kDeviceGetNumAESAudioOutputChannels:		outVal = ::NTV2DeviceGetNumAESAudioOutputChannels (devID);		break;
		case kDeviceGetNumAnalogAudioInputChannels:		outVal = ::NTV2DeviceGetNumAnalogAudioInputChannels (devID);	break;
		case kDeviceGetNumAnalogAudioOutputChannels:	outVal = ::NTV2DeviceGetNumAnalogAudioOutputChannels (devID);	break;
		case kDeviceGetNumAnalogVideoInputs:			outVal = ::NTV2DeviceGetNumAnalogVideoInputs (devID);			break;
		case kDeviceGetNumAnalogVideoOutputs:			outVal = ::NTV2DeviceGetNumAnalogVideoOutputs (devID);			break;
		case kDeviceGetNumAudioSystems:					outVal = ::NTV2DeviceGetNumAudioSystems (devID);				break;
		case kDeviceGetNumBufferedAudioSystems:			outVal = ::NTV2DeviceGetNumAudioSystems(devID)
																	+ (IsSupported(kDeviceCanDoAudioMixer) ? 1 : 0);	break;
		case kDeviceGetNumCrossConverters:				outVal = ::NTV2DeviceGetNumCrossConverters (devID);				break;
		case kDeviceGetNumCSCs:							outVal = ::NTV2DeviceGetNumCSCs (devID);						break;
		case kDeviceGetNumDMAEngines:					outVal = ::NTV2DeviceGetNumDMAEngines (devID);					break;
		case kDeviceGetNumDownConverters:				outVal = ::NTV2DeviceGetNumDownConverters (devID);				break;
		case kDeviceGetNumEmbeddedAudioInputChannels:	outVal = ::NTV2DeviceGetNumEmbeddedAudioInputChannels (devID);	break;
		case kDeviceGetNumEmbeddedAudioOutputChannels:	outVal = ::NTV2DeviceGetNumEmbeddedAudioOutputChannels (devID);	break;
		case kDeviceGetNumFrameStores:					outVal = ::NTV2DeviceGetNumFrameStores (devID);					break;
		case kDeviceGetNumFrameSyncs:					outVal = ::NTV2DeviceGetNumFrameSyncs (devID);					break;
		case kDeviceGetNumHDMIAudioInputChannels:		outVal = ::NTV2DeviceGetNumHDMIAudioInputChannels (devID);		break;
		case kDeviceGetNumHDMIAudioOutputChannels:		outVal = ::NTV2DeviceGetNumHDMIAudioOutputChannels (devID);		break;
		case kDeviceGetNumHDMIVideoInputs:				outVal = ::NTV2DeviceGetNumHDMIVideoInputs (devID);				break;
		case kDeviceGetNumHDMIVideoOutputs:				outVal = ::NTV2DeviceGetNumHDMIVideoOutputs (devID);			break;
		case kDeviceGetNumInputConverters:				outVal = ::NTV2DeviceGetNumInputConverters (devID);				break;
		case kDeviceGetNumLTCInputs:					outVal = ::NTV2DeviceGetNumLTCInputs (devID);					break;
		case kDeviceGetNumLTCOutputs:					outVal = ::NTV2DeviceGetNumLTCOutputs (devID);					break;
		case kDeviceGetNumLUTs:							outVal = ::NTV2DeviceGetNumLUTs (devID);						break;
		case kDeviceGetNumMixers:						outVal = ::NTV2DeviceGetNumMixers (devID);						break;
		case kDeviceGetNumOutputConverters:				outVal = ::NTV2DeviceGetNumOutputConverters (devID);			break;
		case kDeviceGetNumReferenceVideoInputs:			outVal = ::NTV2DeviceGetNumReferenceVideoInputs (devID);		break;
		case kDeviceGetNumSerialPorts:					outVal = ::NTV2DeviceGetNumSerialPorts (devID);					break;
		case kDeviceGetNumTSIMuxers:					{	static const NTV2WidgetID s425MuxerIDs[] = {NTV2_Wgt425Mux1, NTV2_Wgt425Mux2,
																										NTV2_Wgt425Mux3, NTV2_Wgt425Mux4};
															const ULWordSet wgtIDs(GetSupportedItems(kNTV2EnumsID_WidgetID));
															for (size_t ndx(0);  ndx < sizeof(s425MuxerIDs)/sizeof(NTV2WidgetID);  ndx++)
																if (wgtIDs.find(s425MuxerIDs[ndx]) != wgtIDs.end())
																	outVal++;
															break;
														}
		case kDeviceGetNumUpConverters:					outVal = ::NTV2DeviceGetNumUpConverters (devID);				break;
		case kDeviceGetNumVideoChannels:				outVal = ::NTV2DeviceGetNumVideoChannels (devID);				break;
		case kDeviceGetNumVideoInputs:					outVal = ::NTV2DeviceGetNumVideoInputs (devID);					break;
		case kDeviceGetNumVideoOutputs:					outVal = ::NTV2DeviceGetNumVideoOutputs (devID);				break;
		case kDeviceGetPingLED:							outVal = ::NTV2DeviceGetPingLED (devID);						break;
		case kDeviceGetSPIFlashVersion:					outVal = ::NTV2DeviceGetSPIFlashVersion (devID);				break;
		case kDeviceGetTotalNumAudioSystems:			outVal = ::NTV2DeviceGetNumAudioSystems(devID)
																	+ (IsSupported(kDeviceCanDoAudioMixer) ? 2 : 0);	break;
		case kDeviceGetUFCVersion:						outVal = ::NTV2DeviceGetUFCVersion (devID);						break;
		default:	return false;	//	Bad param
	}
	return true;	//	Successfully used old ::NTV2DeviceGetNum function

}	//	GetNumericParam


bool CNTV2DriverInterface::GetRegInfoForBoolParam (const NTV2BoolParamID inParamID, NTV2RegInfo & outRegInfo, bool & outFlipSense)
{
	outRegInfo.MakeInvalid();
	outFlipSense = false;
	switch (inParamID)
	{
		case kDeviceCanDoAudioMixer:			outRegInfo.Set(kRegGlobalControl2, 0, kRegMaskAudioMixerPresent, kRegShiftAudioMixerPresent);		break;
		case kDeviceHasMultiRasterWidget:		outRegInfo.Set(kRegMRSupport, 0, kRegMaskMRSupport, kRegShiftMRSupport);							break;
		case kDeviceHasMicrophoneInput:			outRegInfo.Set(kRegGlobalControl2, 0, kRegMaskIsDNXIV, kRegShiftIsDNXIV);							break;
		case kDeviceAudioCanWaitForVBI:			outRegInfo.Set(kRegCanDoStatus, 0, kRegMaskCanDoAudioWaitForVBI, kRegShiftCanDoAudioWaitForVBI);	break;
		case kDeviceHasXptConnectROM:			outRegInfo.Set(kRegCanDoStatus, 0, kRegMaskCanDoValidXptROM, kRegShiftCanDoValidXptROM);			break;

//	BIT(2) IN kRegCanDoStatus NOT YET IN ALL KONA5, CORVID44/12G, KONAX FIRMWARE:
//		case kDeviceCanDoCustomHancInsertion:	outRegInfo.Set(kRegCanDoStatus, 0, kRegMaskCanDoHancInsertion, kRegShiftCanDoHancInsertion);		break;

		//	kDeviceHasBreakoutBoard's sense is opposite of "BOBAbsent", so set outFlipSense 'true':
		case kDeviceHasBreakoutBoard:			outRegInfo.Set(kRegBOBStatus, 0, kRegMaskBOBAbsent, kRegShiftBOBAbsent);	outFlipSense = true;	break;

		default:	break;
	}
	return outRegInfo.IsValid();
}


bool CNTV2DriverInterface::GetRegInfoForNumericParam (const NTV2NumericParamID inParamID, NTV2RegInfo & outRegInfo)
{
	outRegInfo.MakeInvalid();
	switch (inParamID)
	{
		case kDeviceGetNumMicInputs:	outRegInfo.Set(kRegGlobalControl2, 0, kRegMaskIsDNXIV, kRegShiftIsDNXIV);	break;
		default:	break;
	}
	return outRegInfo.IsValid();
}


bool CNTV2DriverInterface::ControlDriverDebugMessages (NTV2_DriverDebugMessageSet msgSet,  bool enable)
{	(void)msgSet; (void) enable;
	return false;
}
