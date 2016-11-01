/**
	@file		ntv2macdriverinterface.h
	@brief		Implements the MacOS-specific flavor of CNTV2DriverInterface.
	@copyright	(C) 2007-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2macdriverinterface.h"
#include "ntv2nubaccess.h"
#include "ntv2utils.h"
#include <time.h>
#include <syslog.h>
#include <iostream>
#include <sstream>
#include <string>
#include <assert.h>
#include <map>
#include <iomanip>
#include "ntv2devicefeatures.h"
#if defined (USE_AJALOCK)
	#include "ajastuff/system/lock.h"
	typedef AJALock		PMutex;
	typedef AJAAutoLock	PThreadLocker;
#else
	#include "ntv2macmutex.h"
#endif
#if !defined (NTV2_NULL_DEVICE)
	extern "C"
	{
		#include <mach/mach.h>
	}
#endif	//	!defined (NTV2_NULL_DEVICE)

using namespace std;


static const char * GetKernErrStr (const kern_return_t inError);


//	MacDriverInterface-specific Logging Macros

#define	HEX2(__x__)				"0x" << hex << setw (2)  << setfill ('0') << (0xFF       & uint8_t (__x__)) << dec
#define	HEX4(__x__)				"0x" << hex << setw (4)  << setfill ('0') << (0xFFFF     & uint16_t(__x__)) << dec
#define	HEX8(__x__)				"0x" << hex << setw (8)  << setfill ('0') << (0xFFFFFFFF & uint32_t(__x__)) << dec
#define	HEX16(__x__)			"0x" << hex << setw (16) << setfill ('0') <<               uint64_t(__x__)  << dec
#define KR(_kr_)				"kernResult=" << HEX8(_kr_) << "(" << GetKernErrStr (_kr_) << ")"
#define INSTP(_p_)				" instance=" << HEX16(uint64_t(_p_))

#if defined (_DEBUG)
	#define	MDIDB(__lvl__, __x__)	do {																										\
										ostringstream   oss;																					\
										pthread_t		curThrdID	(::pthread_self ());														\
										oss << "## " << __lvl__ << ":  " << HEX16(curThrdID) << ":  " << string(__func__) << ":  " << __x__;	\
										cerr << oss.str () << endl;																				\
									} while (false)
#else
	#define	MDIDB(__lvl__, __x__)	do {																										\
										ostringstream   oss;																					\
										pthread_t		curThrdID	(::pthread_self ());														\
										oss << "## " << __lvl__ << ":  " << HEX16(curThrdID) << ":  " << string(__func__) << ":  " << __x__;	\
										syslog (3, "%s\n", oss.str ().c_str ());																\
									} while (false)
#endif

#define	MDIFAIL(__x__)			MDIDB ("ERROR",		__x__)
#define	MDIWARN(__x__)			MDIDB ("WARNING",	__x__)
#define	MDINOTE(__x__)			MDIDB ("NOTE",		__x__)
#if defined (_DEBUG)
	#define	MDIDBG(__x__)		MDIDB ("DEBUG",		__x__)
#else
	#define	MDIDBG(__x__)
#endif

#define	MDIFAILIF(__uccc__, __x__)		do {													\
											if (gErrorLogging & (uint64_t (1) << (__uccc__)))	\
												MDIFAIL(__x__);									\
										} while (false)

#define	MDIWARNIF(__uccc__, __x__)		do {													\
											if (gErrorLogging & (uint64_t (1) << (__uccc__)))	\
												MDIWARN(__x__);									\
										} while (false)

#define	MDINOTEIF(__uccc__, __x__)		do {													\
											if (gErrorLogging & (uint64_t (1) << (__uccc__)))	\
												MDINOTE(__x__);									\
										} while (false)

#define	MDIDBGIF(__uccc__, __x__)		do {													\
											if (gErrorLogging & (uint64_t (1) << (__uccc__)))	\
												MDIDBG(__x__);									\
										} while (false)


#define							kMaxNumDevices		(32)						///	Limit to 32 devices
static const string				sNTV2PCIDriverName	("com_aja_iokit_ntv2");		///	This should be the only place the driver's IOService name is defined
static uint64_t					gErrorLogging		(0x0000000000000000);		///	Log errors? (one flag bit per UserClientCommandCode)
static unsigned					gnBoardMaps;									///	Instance counter -- should never exceed one
static unsigned int				FIVE_SECONDS		(5);						///	Maximum wait time for IORegistry to settle for hot plug/unplug
static uint64_t					RECHECK_INTERVAL	(100LL);					///	Number of calls to DeviceMap::GetConnection before connection recheck performed


#if !defined (NTV2_NULL_DEVICE)
	//	This section builds 'classes.a' with the normal linkage to the IOKit...
	#define	OS_IOMasterPort(_x_,_y_)								::IOMasterPort ((_x_), (_y_))
	#define	OS_IOServiceClose(_x_)									::IOServiceClose ((_x_))
	#define	OS_IOServiceMatching(_x_)								::IOServiceMatching ((_x_))
	#define	OS_IOServiceGetMatchingServices(_x_,_y_,_z_)			::IOServiceGetMatchingServices ((_x_), (_y_), (_z_))
	#define	OS_IOIteratorNext(_x_)									::IOIteratorNext ((_x_))
	#define	OS_IOObjectRelease(_x_)									::IOObjectRelease ((_x_))
	#define	OS_IORegistryEntryCreateCFProperty(_w_,_x_,_y_,_z_)		::IORegistryEntryCreateCFProperty ((_w_), (_x_), (_y_), (_z_))
	#define	OS_IOConnectCallScalarMethod(_u_,_v_,_w_,_x_,_y_,_z_)	::IOConnectCallScalarMethod ((_u_), (_v_), (_w_), (_x_), (_y_), (_z_))
	#define	OS_IOKitGetBusyState(_x_,_y_)							::IOKitGetBusyState ((_x_), (_y_))
	#define	OS_IOKitWaitQuiet(_x_,_y_)								::IOKitWaitQuiet ((_x_), (_y_))
#else	//	NTV2_NULL_DEVICE defined
	//	This version builds 'classes.a' that has no linkage to the IOKit...
	static IOReturn OS_IOMasterPort (const mach_port_t inPort, mach_port_t * pOutPort)
	{
		cerr << "## NOTE:  NTV2_NULL_DEVICE -- will not connect to IOKit" << endl;
		(void) inPort;  if (pOutPort) *pOutPort = 1;  return KERN_SUCCESS;
	}
	static IOReturn OS_IOServiceClose (const io_connect_t inConnection)
	{
		(void) inConnection;  return KERN_SUCCESS;
	}
	static CFMutableDictionaryRef OS_IOServiceMatching (const char * pInName)
	{
		(void) pInName;  return NULL;
	}
	static IOReturn OS_IOServiceGetMatchingServices (const mach_port_t inPort, const CFDictionaryRef inMatch, io_iterator_t * pOutIter)
	{
		(void) inPort;  (void) inMatch;  if (pOutIter) *pOutIter = 0;  return KERN_SUCCESS;
	}
	static io_object_t OS_IOIteratorNext (io_iterator_t inIter)
	{
		(void) inIter;  return 0;
	}
	static IOReturn OS_IOObjectRelease (io_object_t inObj)
	{
		(void) inObj;  return KERN_SUCCESS;
	}
	static CFTypeRef OS_IORegistryEntryCreateCFProperty (const io_registry_entry_t inEntry, const CFStringRef inKey, const CFAllocatorRef inAllocator, const IOOptionBits inOptions)
	{
		(void) inEntry;  (void) inKey;  (void) inAllocator;  (void) inOptions;  return NULL;
	}
	static IOReturn OS_IOConnectCallScalarMethod (const mach_port_t inConnect, const uint32_t inSelector, const uint64_t * pInput, const uint32_t inCount, uint64_t * pOutput, uint32_t	* pOutCount)
	{
		(void) inConnect;  (void) inSelector;  (void) pInput;  (void) inCount;
		if (pOutput) *pOutput = 0;  if (pOutCount) *pOutCount = 0;  return KERN_SUCCESS;
	}
	static IOReturn OS_IOKitGetBusyState (const mach_port_t inPort, uint32_t * pOutState)
	{
		(void) inPort;  if (pOutState) *pOutState = 0;  return KERN_SUCCESS;
	}
	static IOReturn OS_IOKitWaitQuiet (const mach_port_t inPort, const mach_timespec_t * pInOutWait)
	{
		(void) inPort;  (void) pInOutWait;  return KERN_SUCCESS;
	}
#endif	//	NTV2_NULL_DEVICE defined


/**
	@details	The DeviceMap global singleton maintains the underlying Mach connection handles used to talk
				to the NTV2 driver instances for each device on the host.
**/
class DeviceMap
{
	public:
		DeviceMap ()
			:	mStopping		(false),
				mDriverVersion	(0)
		{
			assert (gnBoardMaps == 0  &&  "Attempt to create more than one DeviceMap");
			gnBoardMaps++;
			::memset (&mIOConnections, 0, sizeof (mIOConnections));
			::memset (&mRecheckTally, 0, sizeof (mRecheckTally));

			//	Get the master mach port for talking to IOKit...
			IOReturn	error	(OS_IOMasterPort (MACH_PORT_NULL, &mMasterPort));
			if (error != kIOReturnSuccess)
			{
				MDIFAIL (KR(error) << "Unable to get master port");
				return;
			}
			assert (mMasterPort && "No MasterPort!");
			MDINOTEIF (kMacDeviceMapDebugLog_DeviceMapLifespan, "DeviceMap singleton created");
		}


		~DeviceMap ()
		{
			mStopping = true;
			PThreadLocker autoLock (&mMutex);
			Reset ();
			MDINOTEIF (kMacDeviceMapDebugLog_DeviceMapLifespan, "DeviceMap singleton destroyed");
			gnBoardMaps--;
		}


		void Reset (void)
		{
			PThreadLocker autoLock (&mMutex);
			//	Clear the device map...
			for (UWord ndx (0);  ndx < kMaxNumDevices;  ++ndx)
			{
				io_connect_t	connection	(mIOConnections [ndx]);
				if (connection)
				{
					OS_IOServiceClose (connection);
					mIOConnections [ndx] = 0;
					MDINOTEIF (kMacDeviceMapDebugLog_ConnectionClose, "Device " << ndx << " connection " << HEX8 (connection) << " closed");
				}
			}	//	for each connection in the map
		}


		io_connect_t GetConnection (const UWord inDeviceIndex, const bool inDoNotAllocate = false)
		{
			if (inDeviceIndex >= kMaxNumDevices)
			{
				MDIWARNIF (kMacDeviceMapDebugLog_ParamError, "Bad device index " << inDeviceIndex << ", GetConnection fail");
				return 0;
			}

			PThreadLocker autoLock (&mMutex);
			const io_connect_t	connection	(mIOConnections [inDeviceIndex]);
			if (connection)
			{
				uint64_t &	recheckTally	(mRecheckTally [inDeviceIndex]);
				if (++recheckTally % RECHECK_INTERVAL == 0)
				{
					MDINOTEIF (kMacDeviceMapDebugLog_CheckConnection, "Device " << inDeviceIndex << " connection " << HEX8 (connection) << " expired, checking connection");
					if (!ConnectionIsStillOkay (inDeviceIndex))
					{
						MDIFAIL ("Device " << inDeviceIndex << " connection " << HEX8 (connection) << " invalid, resetting DeviceMap");
						Reset ();
						return 0;
					}
				}
				return connection;
			}

			if (inDoNotAllocate)
				return connection;

			if (mStopping)
			{
				MDIWARN ("Request to Open device " << inDeviceIndex << " denied because DeviceMap closing");
				return 0;	//	No new connections if my destructor was called
			}

			//	Wait for IORegistry to settle down (if busy)...
			if (!WaitForBusToSettle ())
			{
				MDIWARN ("IORegistry unstable, resetting DeviceMap");
				Reset ();
				return 0;
			}

			//	Make a new connection...
			UWord			ndx			(inDeviceIndex);
			io_iterator_t 	ioIterator	(0);
			IOReturn		error		(kIOReturnSuccess);
			io_object_t		ioObject	(0);
			io_connect_t	ioConnect	(0);

			assert (mMasterPort && "No MasterPort!");

			//	Create an iterator to search for our driver...
			error = OS_IOServiceGetMatchingServices (mMasterPort, OS_IOServiceMatching (CNTV2MacDriverInterface::GetIOServiceName ()), &ioIterator);
			if (error != kIOReturnSuccess)
			{
				MDIFAIL (KR(error) << " -- IOServiceGetMatchingServices failed, no match for '" << sNTV2PCIDriverName << "', device index " << inDeviceIndex << " requested");
				return 0;
			}

			//	Use ndx to find nth device -- and only open that one...
			for ( ; (ioObject = OS_IOIteratorNext (ioIterator));  OS_IOObjectRelease (ioObject))
				if (ndx == 0)
					break;		//	Found it!
				else
					--ndx;

			if (ioIterator)
				OS_IOObjectRelease (ioIterator);
			if (ioObject == 0)
				return 0;		//	No devices found at all
			if (ndx)
				return 0;		//	Requested device index exceeds number of devices found

			//	Found the device we want -- open it...
			error = ::IOServiceOpen (ioObject, ::mach_task_self (), 0, &ioConnect);
			OS_IOObjectRelease (ioObject);
			if (error != kIOReturnSuccess)
			{
				MDIFAIL (KR(error) << " -- IOServiceOpen failed on device " << inDeviceIndex);
				return 0;
			}

			//	Check driver compatibility...
			if (!CheckDriverVersion (ioConnect))
			{
				OS_IOServiceClose (ioConnect);
				return 0;
			}

			//	All good -- cache the connection handle...
			mIOConnections [inDeviceIndex] = ioConnect;
			mRecheckTally [inDeviceIndex] = 0;
			MDINOTEIF (kMacDeviceMapDebugLog_ConnectionOpen, "Device " << inDeviceIndex << " connection " << HEX8 (ioConnect) << " opened");
			return ioConnect;

		}	//	GetConnection


		void Dump (const UWord inMaxNumDevices = 10) const
		{
			PThreadLocker autoLock (&mMutex);
			for (UWord ndx (0);  ndx < inMaxNumDevices;  ++ndx)
				MDIDBG ("    [" << ndx << "]:  con=" << HEX8 (mIOConnections [ndx]));
		}


		UWord GetConnectionCount (void) const
		{
			UWord	tally	(0);
			PThreadLocker autoLock (&mMutex);
			for (UWord ndx (0);  ndx < kMaxNumDevices;  ++ndx)
				if (mIOConnections [ndx])
					tally++;
				else
					break;
			return tally;
		}


		ULWord GetConnectionChecksum (void) const
		{
			ULWord	checksum	(0);
			PThreadLocker autoLock (&mMutex);
			for (UWord ndx (0);  ndx < kMaxNumDevices;  ++ndx)
				if (mIOConnections [ndx])
					checksum += mIOConnections [ndx];
				else
					break;
			return checksum;
		}


		uint32_t	GetDriverVersion (void) const			{ return mDriverVersion; }


	private:
		bool WaitForBusToSettle (void)
		{
			uint32_t	busyState	(0);
			IOReturn	kr			(OS_IOKitGetBusyState (mMasterPort, &busyState));

			if (kr != kIOReturnSuccess)
				MDIFAIL ("IOKitGetBusyState failed -- " << KR(kr));
			else if (busyState)
			{
				mach_timespec_t	maxWaitTime	= {FIVE_SECONDS, 0};
				MDINOTEIF (kMacDeviceMapDebugLog_IORegistryActivity, "IOKitGetBusyState reported BUSY -- waiting for IORegistry to stabilize...");

				kr = OS_IOKitWaitQuiet (mMasterPort, &maxWaitTime);
				if (kr == kIOReturnSuccess)
					return true;
				MDIFAILIF (kMacDeviceMapDebugLog_IORegistryActivity, "IOKitWaitQuiet timed out -- " << KR(kr));
			}
			else
				return true;
			return false;
		}


		bool	ConnectionIsStillOkay (const UWord inDeviceIndex)
		{
			if (inDeviceIndex >= kMaxNumDevices)
			{
				MDIWARNIF (kMacDeviceMapDebugLog_ParamError, "ConnectionIsStillOkay:  bad 'inDeviceIndex' parameter " << inDeviceIndex);
				return 0;
			}

			const io_connect_t	connection	(mIOConnections [inDeviceIndex]);
			if (connection)
			{
				uint64_t		scalarO_64 [2]	= {0, 0};
				uint32_t		outputCount		= 2;
				kern_return_t	kernResult		= OS_IOConnectCallScalarMethod (connection, kDriverGetStreamForApplication, 0, 0, scalarO_64, &outputCount);
				if (kernResult == KERN_SUCCESS)
					return true;
			}
			return false;
		}

		bool	CheckDriverVersion (io_connect_t inConnection)
		{
			if (!inConnection)
			{
				MDIWARNIF (kMacDeviceMapDebugLog_ParamError, "CheckDriverVersion:  bad 'inConnection' parameter " << inConnection);
				return false;
			}

			uint64_t		scalarO_64 [2];
			uint32_t		outputCount	(sizeof (scalarO_64) / sizeof (scalarO_64[0]));
			kern_return_t	kernResult	(OS_IOConnectCallScalarMethod (inConnection, kDriverGetDrvrVersion, 0, 0, scalarO_64, &outputCount));
			if (kernResult == KERN_SUCCESS)
			{
				const UInt32	interfaceVers	(scalarO_64 [0]);
				if (!mDriverVersion)
					mDriverVersion = scalarO_64 [1];
				if (interfaceVers == AJA_MAC_DRIVER_INTERFACE_VERSION)
					return true;
				MDIFAIL ("connection " << inConnection << " -- incompatible driver interface " << interfaceVers << ", expected " << AJA_MAC_DRIVER_INTERFACE_VERSION
						<< ", version " << HEX8(mDriverVersion));
			}
			else
				MDIFAIL ("IOConnectCallScalerMethod returned " << KR(kernResult) << " for 'kDriverGetDrvrVersion' using connection " << HEX8(inConnection));
			return false;
		}

	private:
		io_connect_t	mIOConnections	[kMaxNumDevices];	//	My io_connect_t map
		uint64_t		mRecheckTally	[kMaxNumDevices];	//	Used to calc when it's time to test if connection still ok
		mutable PMutex	mMutex;								//	My guard mutex
		mach_port_t		mMasterPort;						//	Handy master port
		bool			mStopping;							//	Don't open new connections if I'm stopping
		uint32_t		mDriverVersion;						//	Handy driver version

};	//	DeviceMap


static DeviceMap		gDeviceMap;		//	The DeviceMap singleton
#if defined (_DEBUG)
	static MDIStats			gClientStats;	//	Client stats

	MDIStats::~MDIStats()
	{
		//MDIDBG (dec << fConstructCount << " construct(s), " << fDestructCount << " destruct(s), " << fOpenCount << " open(s), " << fCloseCount << " close(s)");
	}

	void CNTV2MacDriverInterface::GetClientStats (MDIStats & outStats)
	{
		outStats = gClientStats;
	}
#endif	//	defined (_DEBUG)


const char * CNTV2MacDriverInterface::GetIOServiceName (void)
{
	return sNTV2PCIDriverName.c_str ();
}



//--------------------------------------------------------------------------------------------------------------------
//	CNTV2MacDriverInterface
//
//	Constructor
//--------------------------------------------------------------------------------------------------------------------
CNTV2MacDriverInterface::CNTV2MacDriverInterface( void )
{
	_boardOpened = false;
	_boardNumber = 0;
#if defined (_DEBUG)
	gClientStats.fConstructCount++;
#endif	//	defined (_DEBUG)
	MDIDBGIF (kMacDeviceMapDebugLog_MDILifespan, " CNTV2MacDriverInterface" << INSTP(this));
}


//--------------------------------------------------------------------------------------------------------------------
//	~CNTV2MacDriverInterface
//--------------------------------------------------------------------------------------------------------------------
CNTV2MacDriverInterface::~CNTV2MacDriverInterface( void )
{
#if defined (_DEBUG)
	gClientStats.fDestructCount++;
#endif	//	defined (_DEBUG)
	MDIDBGIF (kMacDeviceMapDebugLog_MDILifespan, " CNTV2MacDriverInterface" << INSTP(this));
}


io_connect_t CNTV2MacDriverInterface::GetIOConnect (const bool inDoNotAllocate) const
{
	return gDeviceMap.GetConnection (_boardNumber, inDoNotAllocate);
}


//--------------------------------------------------------------------------------------------------------------------
//	Open
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::Open (UWord inDeviceIndexNumber, bool displayError, NTV2DeviceType eBoardType,	const char * pInHostName)
{
	if (inDeviceIndexNumber >= kMaxNumDevices)
		return false;

	const string	hostName	(pInHostName ? pInHostName : "");
	if (IsOpen ()  &&  inDeviceIndexNumber == _boardNumber)
	{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
		if (hostName.empty ()  &&  _remoteHandle == INVALID_NUB_HANDLE)
			return true;	//	Same local device requested, already open
		if (_hostname == hostName  &&  _remoteHandle != INVALID_NUB_HANDLE)
			return true;	//	Same remote device requested, already open
#else
		return true;	//	Same local device requested, already open
#endif
	}

	if (IsOpen ())
		Close ();		//	Close if different device requested

#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (!hostName.empty())
		_boardOpened = OpenRemote (inDeviceIndexNumber, displayError, eBoardType, pInHostName);	//	Remote host open
	else
#else
	(void) pInHostName;
	(void) eBoardType;
	(void) displayError;
#endif
	{
		_boardOpened = gDeviceMap.GetConnection (inDeviceIndexNumber) != 0;						//	Local host open -- get a Mach connection
		#if defined (_DEBUG)
			gClientStats.fOpenCount++;
		#endif	//	defined (_DEBUG)
	}

	if (IsOpen ())
	{
		_boardNumber = inDeviceIndexNumber;	//	Set _boardNumber now, because ReadRegister needs it to talk to the correct device
		const NTV2DeviceIDSet	legalDeviceIDs	(::NTV2GetSupportedDevices ());
		if (!ReadRegister (kRegBoardID, reinterpret_cast<ULWord*>(&_boardID)))
		{
			MDIFAIL ("ReadRegister failed for 'kRegBoardID' -- " << INSTP(this) << ", ndx=" << inDeviceIndexNumber << ", con=" << HEX8(gDeviceMap.GetConnection (inDeviceIndexNumber, false)) << ", id=" << HEX4(_boardID));
			_boardNumber = 0;
			_boardOpened = false;
			return false;
		}
		if (legalDeviceIDs.find (_boardID) == legalDeviceIDs.end ())
		{
			_boardNumber = 0;
			_boardOpened = false;
			MDIFAIL ("Unsupported _boardID " << HEX4(_boardID) << INSTP(this) << ", ndx=" << inDeviceIndexNumber << ", con=" << HEX8(gDeviceMap.GetConnection (inDeviceIndexNumber, false)));
			return false;
		}

		// Kludge Warning.....
		// InitMemberVariablesOnOpen needs frame geometry to determine frame buffer size and number.
		NTV2FrameGeometry fg;
		ULWord returnVal1, returnVal2;

		ReadRegister (kRegGlobalControl, (ULWord*)&fg, kRegMaskGeometry, kRegShiftGeometry);
		ReadRegister (kRegCh1Control, &returnVal1, kRegMaskFrameFormat, kRegShiftFrameFormat);
		ReadRegister (kRegCh1Control, &returnVal2, kRegMaskFrameFormatHiBit, kRegShiftFrameFormatHiBit);

		InitMemberVariablesOnOpen (fg, (NTV2FrameBufferFormat)((returnVal1&0x0f) | ((returnVal2&0x1)<<4)));
	}
	MDIDBGIF (kMacDeviceMapDebugLog_OpenClose, " CNTV2MacDriverInterface" << INSTP(this) << ", ndx=" << _boardNumber << ", con=" << HEX8(gDeviceMap.GetConnection (_boardNumber, false)) << ", id=" << ::NTV2DeviceIDToString(_boardID));

	return IsOpen ();

}	//	Open


//--------------------------------------------------------------------------------------------------------------------
//	TestOpen
//
//	return true if device is still present
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::TestOpen()
{
	ULWord appType;
	int32_t pid;
	return GetStreamingApplication (&appType, &pid);	//	A cheap & fast way to test if the device is still present
}


//--------------------------------------------------------------------------------------------------------------------
//	Close
//
//	Close our connection (use mutex)
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::Close (void)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
		return CloseRemote ();
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)

#if defined (_DEBUG)
	gClientStats.fCloseCount++;
#endif	//	defined (_DEBUG)
	MDIDBGIF (kMacDeviceMapDebugLog_OpenClose, "CNTV2MacDriverInterface" << INSTP(this) << ", ndx=" << _boardNumber << ", con=" << HEX8(gDeviceMap.GetConnection (_boardNumber)) << ", id=" << ::NTV2DeviceIDToString(_boardID));

	_boardOpened = false;
	_boardNumber = 0;
	return true; // success

}	//	Close


//--------------------------------------------------------------------------------------------------------------------
//	GetPCISlotNumber
//
//	Returns my PCI slot number, if known; otherwise returns zero.
//--------------------------------------------------------------------------------------------------------------------
ULWord CNTV2MacDriverInterface::GetPCISlotNumber (void) const
{
	ULWord				result (0);
	io_registry_entry_t	ioRegistryEntry	(0);
	CFTypeRef			pDataObj (NULL);
	const char *		pSlotName (NULL);

	if (_boardOpened && GetIOConnect ())
	{
		//	TODO:	Figure out where in the IORegistry the io_connect_t is, then navigate up to the io_registry_entry
		//			for our driver that contains the "AJAPCISlot" property. Then proceed as before...
		ioRegistryEntry = static_cast <io_registry_entry_t> (GetIOConnect ());
		return 0;		//	FINISH THIS

		//	Ask our driver for the "AJAPCISlot" property...
		pDataObj = OS_IORegistryEntryCreateCFProperty (ioRegistryEntry, CFSTR ("AJAPCISlot"), NULL, 0);
		if (pDataObj)
		{
			//	Cast the property data to a standard "C" string...
			pSlotName = ::CFStringGetCStringPtr (static_cast <CFStringRef> (pDataObj), kCFStringEncodingMacRoman);
			if (pSlotName)
			{
				//	Extract the slot number from the name...
				const string	slotName (pSlotName);
				if (slotName.find ("Slot") == 0)
				{
					const char	lastChar (slotName.at (slotName.length () - 1));
					if (lastChar >= '0' && lastChar <= '9')
						result = static_cast <ULWord> (lastChar) - static_cast <ULWord> ('0');

				}	//	if slotName starts with "Slot"

			}	//	if pSlotName non-NULL

			::CFRelease (pDataObj);

		}	//	if AJAPCISlot property obtained okay

	}	//	if _boardOpened && GetIOConnect()

	return result;

}	//	GetPCISlotNumber


//--------------------------------------------------------------------------------------------------------------------
//	MapMemory
//
//	Return a pointer and size of either the register map or frame buffer memory.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::MapFrameBuffers( void )
{
    UByte	*baseAddr;
    if ( MapMemory (kFrameBufferMemory, (void**)&baseAddr) )
    {
		_pFrameBaseAddress = (ULWord *) baseAddr;
        return true;
	}
    else
    {
		_pFrameBaseAddress = 0;
		_pCh1FrameBaseAddress = _pCh2FrameBaseAddress = 0;	//	DEPRECATE!
        return false;
    }
    return true;
}

bool CNTV2MacDriverInterface::UnmapFrameBuffers ()
{
    _pFrameBaseAddress = 0;
	_pCh1FrameBaseAddress = _pCh2FrameBaseAddress = 0;		//	DEPRECATE!
    return true;
}


//--------------------------------------------------------------------------------------------------------------------
//	MapRegisters
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::MapRegisters( void )
{
    ULWord	*baseAddr;
    if ( MapMemory (kRegisterMemory, (void**)&baseAddr) )
    {
	_pRegisterBaseAddress = (ULWord*)baseAddr;
    }
    else
    {
	_pRegisterBaseAddress = 0;
        return false;
    }
    return true;

}

bool CNTV2MacDriverInterface::UnmapRegisters ()
{
	_pRegisterBaseAddress = 0;
	return true;
}


//--------------------------------------------------------------------------------------------------------------------
//	Map / Unmap Xena2Flash
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::MapXena2Flash( void )
{
    ULWord	*baseAddr;
    if ( MapMemory (kXena2FlashMemory, (void**)&baseAddr) )
    {
	_pXena2FlashBaseAddress = (ULWord*)baseAddr;
    }
    else
    {
	_pXena2FlashBaseAddress = 0;
        return false;
    }
    return true;

}

bool CNTV2MacDriverInterface::UnmapXena2Flash ()
{
	_pXena2FlashBaseAddress = 0;
	return true;
}


//--------------------------------------------------------------------------------------------------------------------
//	MapMemory
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::MapMemory( MemoryType memType, void **memPtr )
{
	if (GetIOConnect())
	{
#ifndef __LP64__
		vm_size_t			size = 0;

		IOConnectMapMemory (GetIOConnect(),
							memType,
							mach_task_self(),
							(vm_address_t*)memPtr,
							(vm_size_t*)&size,
							kIOMapDefaultCache | kIOMapAnywhere);
		return (size > 0);

#else
		mach_vm_size_t		size = 0;

		IOConnectMapMemory (GetIOConnect(),
							memType,
							mach_task_self(),
							(mach_vm_address_t*)memPtr,
							&size,
							kIOMapDefaultCache | kIOMapAnywhere);
		return (size > 0);
#endif
	}
	return false;
}

#pragma mark - New Driver Calls

//--------------------------------------------------------------------------------------------------------------------
//	ReadRegister
//
//	Return the value of specified register after masking and shifting the value.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::ReadRegister( ULWord registerNumber,
											ULWord *registerValue,
											ULWord registerMask,
											ULWord registerShift )

{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::ReadRegister (registerNumber, registerValue, registerMask, registerShift))
		{
			MDIFAILIF (kDriverReadRegister, INSTP(this) << ":  NTV2ReadRegisterRemote failed");
			return false;
		}
		return true;
	}
	else
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT (registerShift < 32);
	{
		kern_return_t 	kernResult			= KERN_FAILURE;

		uint64_t	scalarI_64[2];
		uint64_t	scalarO_64 = 0;
		uint32_t	outputCount = 1;

		scalarI_64[0] = registerNumber;
		scalarI_64[1] = registerMask;

		if (GetIOConnect())
			kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
												   kDriverReadRegister,		// selector of the function to be called via the user client.
												   scalarI_64,				// array of scalar (64-bit) input values.
												   2,						// the number of scalar input values.
												   &scalarO_64,				// array of scalar (64-bit) output values.
												   &outputCount);			// pointer to the number of scalar output values.
		*registerValue = (uint32_t) scalarO_64;
		if (kernResult == KERN_SUCCESS)
			return true;
		else
		{
			MDIFAILIF (kDriverReadRegister, KR(kernResult) << INSTP(this) << ", ndx=" << _boardNumber << ", con=" << HEX8(GetIOConnect(false))
											<< " -- reg=" << registerNumber << ", mask=" << HEX8(registerMask) << ", shift=" << HEX8(registerShift) << ", WILL RESET DEVICE MAP");
			gDeviceMap.Reset ();
			SleepMs (30);
			return false;
		}
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	WriteRegister
//
//	Set the specified register value taking into accout the bit mask.
//	If the bit mask is not 0xFFFFFFFF (default) or 0, then this does a read-modify-write.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::WriteRegister( ULWord registerNumber,
											 ULWord registerValue,
											 ULWord registerMask,
											 ULWord registerShift)
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::WriteRegister (registerNumber, registerValue, registerMask, registerShift))
		{
			MDIFAILIF (kDriverWriteRegister, INSTP(this) << ":  NTV2WriteRegisterRemote failed");
			return false;
		}
		return true;
	}
	else
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT (registerShift < 32);
	{
		kern_return_t 	kernResult			= KERN_FAILURE;

		uint64_t	scalarI_64[3];
		uint32_t	outputCount = 0;

		scalarI_64[0] = registerNumber;
		scalarI_64[1] = registerValue;
		scalarI_64[2] = registerMask;

		if (GetIOConnect())
			kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
												   kDriverWriteRegister,	// selector of the function to be called via the user client.
												   scalarI_64,				// array of scalar (64-bit) input values.
												   3,						// the number of scalar input values.
												   0,						// array of scalar (64-bit) output values.
												   &outputCount);			// pointer to the number of scalar output values.
		if (kernResult == KERN_SUCCESS)
			return true;
		else
		{
			MDIFAILIF (kDriverWriteRegister, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()) << " -- reg=" << registerNumber << ", val=" << HEX8(registerValue)
												 << ", mask=" << HEX8(registerMask) << ", shift=" << HEX8(registerShift));
			return false;
		}
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	GetDriverVersion
//
//	Return the driver interface version and release version.
//--------------------------------------------------------------------------------------------------------------------
UInt32 CNTV2MacDriverInterface::GetDriverVersion( NumVersion *version )
{
	UInt32 interfaceVers = 0;
	UInt32 driverVers = 0;

	if (GetIOConnect())
	{
		kern_return_t kernResult = KERN_FAILURE;

		uint64_t	scalarO_64[2];
		uint32_t	outputCount = 2;

		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverGetDrvrVersion,	// selector of the function to be called via the user client.
											   0,						// array of scalar (64-bit) input values.
											   0,						// the number of scalar input values.
											   scalarO_64,				// array of scalar (64-bit) output values.
											   &outputCount);				// pointer to the number of scalar output values.
		if (kernResult == KERN_SUCCESS)
		{
			interfaceVers = (UInt32) scalarO_64[0];
			driverVers = (UInt32) scalarO_64[1];
		}
		else
			MDIFAILIF (kDriverGetDrvrVersion, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
	}

	if (version)
		*version = *((NumVersion *) &driverVers);

	return interfaceVers;
}


//--------------------------------------------------------------------------------------------------------------------
//	StartDriver
//
//	For IoHD this is currently a no-op
//	For Kona this starts the driver after all of the bitFiles have been sent to the driver.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::StartDriver( DriverStartPhase phase )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[1];
	uint32_t	outputCount = 0;

	scalarI_64[0] = phase;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverStartDriver,		// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   1,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);				// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverStartDriver, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	AcquireStreamForApplication
//
//	Aquire board by by waiting for current board to release its resources
//
//	Note: When quicktime is using the board, desktop output on the board is disabled
//	by the driver.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::AcquireStreamForApplication( ULWord appType, int32_t pid )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = (uint64_t) appType;
	scalarI_64[1] = pid;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverAcquireStreamForApplication,	// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);				// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverAcquireStreamForApplication, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	ReleaseStreamForApplication
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::ReleaseStreamForApplication( ULWord appType, int32_t pid )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = (uint64_t) appType;
	scalarI_64[1] = pid;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverReleaseStreamForApplication,	// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.

	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverReleaseStreamForApplication, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	AcquireStreamForApplicationWithReference
//
//  Do a reference counted acquire
//  Use this call ONLY with ReleaseStreamForApplicationWithReference
//	Aquire board by by waiting for current board to release its resources
//
//	Note: When quicktime is using the board, desktop output on the board is disabled
//	by the driver.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::AcquireStreamForApplicationWithReference( ULWord appType, int32_t pid )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = (uint64_t) appType;
	scalarI_64[1] = pid;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverAcquireStreamForApplicationWithReference,	// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverAcquireStreamForApplicationWithReference, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	ReleaseStreamForApplicationWithReference
//
//  Do a reference counted release
//  Use this call ONLY with AcquireStreamForApplicationWithReference
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::ReleaseStreamForApplicationWithReference( ULWord appType, int32_t pid )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = (uint64_t) appType;
	scalarI_64[1] = pid;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverReleaseStreamForApplicationWithReference,	// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverReleaseStreamForApplicationWithReference, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	KernelLog
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::KernelLog( void* dataPtr, UInt32 dataSize )
{
	kern_return_t kernResult = KERN_FAILURE;
	
	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;
	
	scalarI_64[0] = (uint64_t) dataPtr;
	scalarI_64[1] = dataSize;
	
	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverKernelLog,		// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);				// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverKernelLog, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Streaming Application
//
//	Forced aquisition of board for exclusive use by app
//		Use with care - better to use AcquireStreamForApplication
//
//	Note: When quicktime is using the board, desktop output on the board is disabled
//	by the driver.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetStreamingApplication( ULWord appType, int32_t pid )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = (uint64_t) appType;
	scalarI_64[1] = pid;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverSetStreamForApplication,	// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);				// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverSetStreamForApplication, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	GetStreamingApplication
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::GetStreamingApplication( ULWord *appType, int32_t *pid )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarO_64[2];
	uint32_t	outputCount = 2;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverGetStreamForApplication,	// selector of the function to be called via the user client.
											   0,						// array of scalar (64-bit) input values.
											   0,						// the number of scalar input values.
											   scalarO_64,				// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	*appType = (ULWord) scalarO_64[0];
	*pid = (int32_t) scalarO_64[1];

	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverGetStreamForApplication, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	SetDefaultDeviceForPID
//
//	Used for multicard support with QT components. (Also see IsDefaultDeviceForPID)
//	QT components are created and destroyed multiple times in the lifetime of running app. This call allows a component
//	to mark a device as its default device, so that each time the component reopens, it can find the device it used last.
//
//	This call adds the input PID to list of PIDs that is tracked by
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetDefaultDeviceForPID( int32_t pid )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	uint64_t		scalarI_64[1];
	uint32_t		outputCount = 0;

	scalarI_64[0] = pid;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverSetDefaultDeviceForPID,// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   1,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverSetDefaultDeviceForPID, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	IsDefaultDeviceForPID
//
//	Used for multicard support with QT components. (Also see SetDefaultDeviceForPID)
//	QT components are created and destroyed multiple times in the lifetime of running app. This call allows a component
//	to mark a device as its default device, so that each time the component reopens, it can find the device it used last.
//
//	Returns true if the input process ID was previously marked as the default device using SetDefaultDeviceForPID.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::IsDefaultDeviceForPID( int32_t pid )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	uint64_t		scalarI_64[1];
	uint64_t		scalarO_64 = 0;
	uint32_t		outputCount = 1;

	scalarI_64[0] = pid;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverIsDefaultDeviceForPID,// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   1,						// the number of scalar input values.
											   &scalarO_64,				// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	return (bool) scalarO_64;
}


//--------------------------------------------------------------------------------------------------------------------
//	LockFormat
//
//	For Kona this is currently a no-op
//	For IoHD this will for bitfile swaps / Isoch channel rebuilds based on vidoe mode / video format
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::LockFormat( void )
{
	kern_return_t	kernResult	= KERN_FAILURE;
	uint32_t		outputCount	= 0;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverLockFormat,		// selector of the function to be called via the user client.
											   0,						// array of scalar (64-bit) input values.
											   0,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverLockFormat, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	SetAVSyncPattern
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetAVSyncPattern( void* dataPtr, UInt32 dataSize )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = (uint64_t) dataPtr;
	scalarI_64[1] = dataSize;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverSetAVSyncPattern,	// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverSetAVSyncPattern, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	TriggerAVSync
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::TriggerAVSync( NTV2Crosspoint channelSpec, UInt32 count )
{
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = channelSpec;
	scalarI_64[1] = count;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverTriggerAVSync,	// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverTriggerAVSync, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	WaitForInterrupt
//
//	Block the current thread until the specified interrupt occurs.
//	Supply a timeout in milliseconds - if 0 (default), then never time out.
//	Returns true if interrupt occurs, false if timeout.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::WaitForInterrupt( INTERRUPT_ENUMS type, unsigned int  timeout )
{
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
		return CNTV2DriverInterface::WaitForInterrupt(type, timeout);
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)

	if (type != eChangeEvent)
	{
		kern_return_t 	kernResult = KERN_FAILURE;

		uint64_t	scalarI_64[2];
		uint64_t	scalarO_64 = 0;
		uint32_t	outputCount = 1;

		scalarI_64[0] = type;
		scalarI_64[1] = timeout;


		if (!NTV2_IS_VALID_INTERRUPT_ENUM (type))
			kernResult = KERN_INVALID_VALUE;
		else if (GetIOConnect())
			kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
												   kDriverWaitForInterrupt,	// selector of the function to be called via the user client.
												   scalarI_64,				// array of scalar (64-bit) input values.
												   2,						// the number of scalar input values.
												   &scalarO_64,				// array of scalar (64-bit) output values.
												   &outputCount);				// pointer to the number of scalar output values.
		UInt32 interruptOccured = (uint32_t) scalarO_64;

		if (kernResult != KERN_SUCCESS)
			MDIFAILIF (kDriverWaitForInterrupt, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));

		if (kernResult == KERN_SUCCESS && interruptOccured)
		{
            BumpEventCount (type);
			return true;
		}
		else
			return false;
	}
	else
		return WaitForChangeEvent(timeout);
}


//--------------------------------------------------------------------------------------------------------------------
//	GetInterruptCount
//
//	Returns the number of interrupts that have occured for the specified interrupt type.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::GetInterruptCount( INTERRUPT_ENUMS eInterrupt, ULWord *pCount )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[1];
	uint64_t	scalarO_64 = 0;
	uint32_t	outputCount = 1;

	scalarI_64[0] = eInterrupt;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverGetInterruptCount,// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   1,						// the number of scalar input values.
											   &scalarO_64,				// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	*pCount = (ULWord) scalarO_64;

	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverGetInterruptCount, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	WaitForChangeEvent
//
//	Block the current thread until a register changes.
//	Supply a timeout in milliseconds - if 0 (default), then never time out.
//	Returns true if change occurs, false if timeout.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::WaitForChangeEvent( UInt32 timeout )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[1];
	uint64_t	scalarO_64 = 0;
	uint32_t	outputCount = 1;

	scalarI_64[0] = timeout;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverWaitForChangeEvent,// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   1,						// the number of scalar input values.
											   &scalarO_64,				// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult != KERN_SUCCESS)
		MDIFAILIF (kDriverWaitForChangeEvent, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
	return (bool) scalarO_64;
}


//--------------------------------------------------------------------------------------------------------------------
//	GetTime
//
//	Return the time.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::GetTime( UInt32 *time, UInt32 *scale )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	uint64_t	scalarO_64[2];
	uint32_t	outputCount = 2;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverGetTime,			// selector of the function to be called via the user client.
											   0,						// array of scalar (64-bit) input values.
											   0,						// the number of scalar input values.
											   scalarO_64,				// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	*time = (UInt32) scalarO_64[0];
	*scale = (UInt32) scalarO_64[1];

	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverGetTime, KR(kernResult) << INSTP(this));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	DmaTransfer
//
//	Start a memory transfer using the specified DMA engine.
//	Optional - call PrepareDMAMemory on the dataPtr before the first use of memory block
//	for DMA and CompleteDMAMemory when done.  This will speed up the DMA by not requiring
//	memory to be prepared for each DMA.  Otherwise, it takes about 2 to 5 ms (sometimes
//	much more) for the memory block to be prepared.
//	This function will sleep (block) until the DMA finishes.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::DmaTransfer (	const NTV2DMAEngine	inDMAEngine,
											const bool			inIsRead,
											const ULWord		inFrameNumber,
											ULWord *			pFrameBuffer,
											const ULWord		inOffsetBytes,
											const ULWord		inByteCount,
											const bool			inSynchronous)
{
	(void)inSynchronous;
	
	kern_return_t kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[6];
	uint32_t	outputCount = 0;

	scalarI_64[0] = inDMAEngine;
	scalarI_64[1] = (uint64_t) pFrameBuffer;
	scalarI_64[2] = inFrameNumber;
	scalarI_64[3] = inOffsetBytes;
	scalarI_64[4] = inByteCount;
	scalarI_64[5] = !inIsRead;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverDMATransfer,		// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   6,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverDMATransfer, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect())
					<< ", eng=" << inDMAEngine << ", frm=" << inFrameNumber << ", off=" << HEX8(inOffsetBytes) << ", len=" << HEX8(inByteCount) << ", " << (inIsRead ? "R" : "W"));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	DmaTransfer
//
//	Start a memory transfer using the specified DMA engine.
//	Optional - call PrepareDMAMemory on the dataPtr before the first use of memory block
//	for DMA and CompleteDMAMemory when done.  This will speed up the DMA by not requiring
//	memory to be prepared for each DMA.  Otherwise, it takes about 2 to 5 ms (sometimes
//	much more) for the memory block to be prepared.
//	This function will sleep (block) until the DMA finishes.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::DmaTransfer( NTV2DMAEngine DMAEngine,
										   bool bRead,
										   ULWord frameNumber,
										   ULWord * pFrameBuffer,
										   ULWord offsetBytes,
										   ULWord bytes,
										   ULWord videoNumSegments,
									       ULWord videoSegmentHostPitch,
									       ULWord videoSegmentCardPitch,
										   bool bSync )
{
	(void) bSync;
	kern_return_t kernResult = KERN_FAILURE;
	size_t	outputStructSize = 0;

	DMA_TRANSFER_STRUCT_64 dmaTransfer64;

	dmaTransfer64.dmaEngine = DMAEngine;
	dmaTransfer64.dmaFlags = 0;

	dmaTransfer64.dmaHostBuffer = (Pointer64) pFrameBuffer;		// virtual address of host buffer
	dmaTransfer64.dmaSize = bytes;								// total number of bytes to DMA
	dmaTransfer64.dmaCardFrameNumber = frameNumber;				// card frame number
	dmaTransfer64.dmaCardFrameOffset = offsetBytes;				// offset (in bytes) into card frame to begin DMA
	dmaTransfer64.dmaNumberOfSegments = videoNumSegments;		// number of segments of size videoBufferSize to DMA
	dmaTransfer64.dmaSegmentSize = (bytes / videoNumSegments);	// size of each segment (if videoNumSegments > 1)
	dmaTransfer64.dmaSegmentHostPitch = videoSegmentHostPitch;	// offset between the beginning of one host-memory segment and the next host-memory segment
	dmaTransfer64.dmaSegmentCardPitch = videoSegmentCardPitch;	// offset between the beginning of one Kona-memory segment and the next Kona-memory segment
	dmaTransfer64.dmaToCard = !bRead;							// direction of DMA transfer

	if (GetIOConnect())
		kernResult = IOConnectCallStructMethod(GetIOConnect(),					// an io_connect_t returned from IOServiceOpen().
											   kDriverDMATransferEx,			// selector of the function to be called via the user client.
											   &dmaTransfer64,					// pointer to the input structure
											   sizeof(DMA_TRANSFER_STRUCT_64),	// size of input structure
											   0,								// pointer to the output structure
											   &outputStructSize);				// size of output structure
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverDMATransferEx, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


bool CNTV2MacDriverInterface::DmaTransfer (NTV2DMAEngine DMAEngine,
											NTV2Channel DMAChannel,
											bool bTarget,
											ULWord frameNumber,
											ULWord frameOffset,
											ULWord videoSize,
											ULWord videoNumSegments,
											ULWord videoSegmentHostPitch,
											ULWord videoSegmentCardPitch,
											PCHANNEL_P2P_STRUCT pP2PData)
{
	(void) DMAEngine;
	(void) DMAChannel;
	(void) bTarget;
	(void) frameNumber;
	(void) frameOffset;
	(void) videoSize;
	(void) videoNumSegments;
	(void) videoSegmentHostPitch;
	(void) videoSegmentCardPitch;
	(void) pP2PData;
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	RestoreHardwareProcampRegisters
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::RestoreHardwareProcampRegisters( void )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	uint32_t	outputCount = 0;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverRestoreProcAmpRegisters,	// selector of the function to be called via the user client.
											   0,						// array of scalar (64-bit) input values.
											   0,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverRestoreProcAmpRegisters, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	SystemControl
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SystemControl( void* dataPtr, SystemControlCode controlCode )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = (uint64_t) dataPtr;
	scalarI_64[1] = controlCode;

	switch (controlCode)
	{
		case SCC_Test:
			break;

		default:
			return false;
	}

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverSystemControl,	// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverSystemControl, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	SystemStatus
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SystemStatus( void* dataPtr, SystemStatusCode statusCode )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	uint64_t	scalarI_64[2];
	uint32_t	outputCount = 0;

	scalarI_64[0] = (uint64_t) dataPtr;
	scalarI_64[1] = statusCode;

	if (GetIOConnect())
		kernResult = IOConnectCallScalarMethod(GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
											   kDriverSystemStatus,		// selector of the function to be called via the user client.
											   scalarI_64,				// array of scalar (64-bit) input values.
											   2,						// the number of scalar input values.
											   0,						// array of scalar (64-bit) output values.
											   &outputCount);			// pointer to the number of scalar output values.
	switch (statusCode)
	{
		case SSC_GetFirmwareProgress:
			break;

		default:
			return false;
	}

	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverSystemStatus, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	Get/Set include/exclude debug filter strings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetDebugFilterStrings( const char* includeString,const char* excludeString )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	size_t	outputStructSize = 0;

	KonaDebugFilterStringInfo filterStringInfo;
	memcpy(&filterStringInfo.includeString[0],includeString,KONA_DEBUGFILTER_STRINGLENGTH);
	memcpy(&filterStringInfo.excludeString[0],excludeString,KONA_DEBUGFILTER_STRINGLENGTH);


	if (GetIOConnect())
		kernResult = IOConnectCallStructMethod(GetIOConnect(),					// an io_connect_t returned from IOServiceOpen().
											   kDriverSetDebugFilterStrings,	// selector of the function to be called via the user client.
											   &filterStringInfo,				// pointer to the input structure
											   sizeof(KonaDebugFilterStringInfo),// size of input structure
											   0,								// pointer to the output structure
											   &outputStructSize);				// size of output structure
	if (kernResult == KERN_SUCCESS)
        return true;
	else
	{
		MDIFAILIF (kDriverSetDebugFilterStrings, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
//	GetDebugFilterStrings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::GetDebugFilterStrings( char* includeString,char* excludeString )
{
	kern_return_t 	kernResult = KERN_FAILURE;

	size_t	outputStructSize = sizeof(KonaDebugFilterStringInfo);

	KonaDebugFilterStringInfo filterStringInfo;

	if (GetIOConnect())
		kernResult = IOConnectCallStructMethod(GetIOConnect(),					// an io_connect_t returned from IOServiceOpen().
											   kDriverGetDebugFilterStrings,	// selector of the function to be called via the user client.
											   0,								// pointer to the input structure
											   0,								// size of input structure
											   &filterStringInfo,				// pointer to the output structure
											   &outputStructSize);				// size of output structure
	if (kernResult == KERN_SUCCESS)
	{
		memcpy(includeString, &filterStringInfo.includeString[0],KONA_DEBUGFILTER_STRINGLENGTH);
		memcpy(excludeString, &filterStringInfo.excludeString[0],KONA_DEBUGFILTER_STRINGLENGTH);
        return true;
	}
	else
	{
		MDIFAILIF (kDriverGetDebugFilterStrings, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
}


//--------------------------------------------------------------------------------------------------------------------
// AutoCirculate
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::AutoCirculate( AUTOCIRCULATE_DATA &autoCircData )
{
	bool success = true;
	UserClientCommandCodes	whichMethod	(kNumberUserClientCommands);
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::AutoCirculate (autoCircData))
		{
			MDIFAILIF (kDriverAutoCirculateControl, INSTP(this) << ":  NTV2AutoCirculateRemote failed");
			success = false;
		}
	}
	else
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	{
		kern_return_t 	kernResult = KERN_FAILURE;

		if (!GetIOConnect())
			return false;

		switch (autoCircData.eCommand)
		{
			case eInitAutoCirc:
			case eStartAutoCirc:
			case eStopAutoCirc:
			case eAbortAutoCirc:
			case ePauseAutoCirc:
			case eFlushAutoCirculate:
			case ePrerollAutoCirculate:
			case eSetActiveFrame:
			{
				// Pass the autoCircData structure to the driver. The driver knows the implicit meanings of the
				// members of the structure based on the the command contained within it.
				size_t	outputStructSize = 0;
				whichMethod = kDriverAutoCirculateControl;
				AUTOCIRCULATE_DATA_64 autoCircData64;
				CopyTo_AUTOCIRCULATE_DATA_64 (&autoCircData, &autoCircData64);

				if (GetIOConnect())
					kernResult = IOConnectCallStructMethod(GetIOConnect(),					// an io_connect_t returned from IOServiceOpen().
														   kDriverAutoCirculateControl,		// selector of the function to be called via the user client.
														   &autoCircData64,					// pointer to the input structure
														   sizeof(AUTOCIRCULATE_DATA_64),	// size of input structure
														   0,								// pointer to the output structure
														   &outputStructSize);				// size of output structure
			}
			break;

			case eGetAutoCirc:
			{
				uint64_t	scalarI_64[1];
				uint32_t	outputCount = 0;
				size_t		outputStructSize = sizeof(AUTOCIRCULATE_STATUS_STRUCT);
				whichMethod = kDriverAutoCirculateStatus;

				scalarI_64[0] = autoCircData.channelSpec;

				if (GetIOConnect())
					kernResult = IOConnectCallMethod(GetIOConnect(),				// an io_connect_t returned from IOServiceOpen().
													 kDriverAutoCirculateStatus,	// selector of the function to be called via the user client.
													 scalarI_64,					// array of scalar (64-bit) input values.
													 1,								// the number of scalar input values.
													 0,								// pointer to the input structure
													 0,								// size of input structure
													 0,								// array of scalar (64-bit) output values.
													 &outputCount,					// the number of scalar output values.
													 autoCircData.pvVal1,			// pointer to the output structure
													 &outputStructSize);			// size of output structure
			}
			break;


			case eGetFrameStamp:
			case eGetFrameStampEx2:
			{
				whichMethod = kDriverAutoCirculateFramestamp;
				// Make sure task structure does not get passed in with eGetFrameStamp call.
				if ( autoCircData.eCommand == eGetFrameStamp)
					autoCircData.pvVal2 = NULL;

				size_t	outputStructSize = sizeof(AUTOCIRCULATE_DATA_64);

				// promote base data structure
				AUTOCIRCULATE_DATA_64 autoCircData64;
				CopyTo_AUTOCIRCULATE_DATA_64 (&autoCircData, &autoCircData64);

				if (GetIOConnect())
					kernResult = IOConnectCallStructMethod(GetIOConnect(),					// an io_connect_t returned from IOServiceOpen().
														   kDriverAutoCirculateFramestamp,	// selector of the function to be called via the user client.
														   &autoCircData64,					// pointer to the input structure
														   sizeof(AUTOCIRCULATE_DATA_64),	// size of input structure
														   &autoCircData64,					// pointer to the output structure
														   &outputStructSize);				// size of output structure
			}
			break;


			case eTransferAutoCirculate:
			case eTransferAutoCirculateEx:
			case eTransferAutoCirculateEx2:
			{
				whichMethod = kDriverAutoCirculateTransfer;
				// Pass the autoCircData structure to the driver. The driver knows the implicit meanings of the
				// members of the structure based on the the command contained within it.
				// Make sure routing table and task structure does not get passed in with eTransferAutoCirculate call.
				if ( autoCircData.eCommand == eTransferAutoCirculate)
				{
					autoCircData.pvVal3 = NULL;
					autoCircData.pvVal4 = NULL;
				}

				// Make sure task structure does not get passed in with eTransferAutoCirculateEx call.
				if ( autoCircData.eCommand == eTransferAutoCirculateEx)
				{
					autoCircData.pvVal4 = NULL;
				}


				size_t	outputStructSize = sizeof(AUTOCIRCULATE_TRANSFER_STATUS_STRUCT);

				// promote base data structure
				AUTOCIRCULATE_DATA_64 autoCircData64;
				CopyTo_AUTOCIRCULATE_DATA_64 (&autoCircData, &autoCircData64);

				// promote AUTOCIRCULATE_TRANSFER_STRUCT
				AUTOCIRCULATE_TRANSFER_STRUCT_64 autoCircTransfer64;
				CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64 ((AUTOCIRCULATE_TRANSFER_STRUCT *)autoCircData.pvVal1, &autoCircTransfer64);
				autoCircData64.pvVal1 = (Pointer64) &autoCircTransfer64;

				AUTOCIRCULATE_TASK_STRUCT_64 autoCircTask64;
				if (autoCircData.pvVal4 != NULL)
				{
					CopyTo_AUTOCIRCULATE_TASK_STRUCT_64 ((AUTOCIRCULATE_TASK_STRUCT *)autoCircData.pvVal4, &autoCircTask64);
					autoCircData64.pvVal4 = (Pointer64) &autoCircTask64;
				}

				if (GetIOConnect())
					kernResult = IOConnectCallStructMethod(GetIOConnect(),					// an io_connect_t returned from IOServiceOpen().
														   kDriverAutoCirculateTransfer,	// selector of the function to be called via the user client.
														   &autoCircData64,					// pointer to the input structure
														   sizeof(AUTOCIRCULATE_DATA_64),	// size of input structure
														   autoCircData.pvVal2,				// pointer to the output structure
														   &outputStructSize);				// size of output structure
			}
			break;

			default:
				//DisplayNTV2Error("Unsupported AC command type in AutoCirculate()\n");
				kernResult =  KERN_INVALID_ARGUMENT;
				break;
		}

		success = (kernResult == KERN_SUCCESS);
		if (kernResult != KERN_SUCCESS && kernResult != kIOReturnOffline)
			MDIFAILIF (whichMethod, KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()) << " -- eCommand=" << autoCircData.eCommand);
	}

	return success;
}


bool CNTV2MacDriverInterface::NTV2Message (NTV2_HEADER * pInOutMessage)
{
	kern_return_t 	kernResult	(KERN_FAILURE);
	io_connect_t	connection	(GetIOConnect ());

#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
		return CNTV2DriverInterface::NTV2Message (pInOutMessage);
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	if (!pInOutMessage)
		return false;
	if (pInOutMessage->GetSizeInBytes () == 0)
		return false;

	uint64_t	scalarI_64 [2];
	uint32_t	numScalarOutputs (0);

	scalarI_64[0] = (uint64_t) pInOutMessage;			//	The NTV2 Message/struct
	scalarI_64[1] = pInOutMessage->GetSizeInBytes ();	//	The size of the message/struct

	if (connection)
		kernResult = IOConnectCallScalarMethod (connection,			//	an io_connect_t returned from IOServiceOpen
											   kDriverNTV2Message,	//	selector of the function to be called via the user client
											   scalarI_64,			//	array of scalar (64-bit) input values
											   2,					//	the number of scalar input values
											   NULL,				//	array of scalar (64-bit) output values
											   &numScalarOutputs);	//	pointer (in: number of scalar output values capable of receiving;  out: actual number of scalar output values)
	if (kernResult != KERN_SUCCESS && kernResult != kIOReturnOffline)
		MDIFAILIF (kDriverNTV2Message, KR(kernResult) << INSTP(this) << ", con=" << HEX8(connection) << endl << "     msg:  " << *pInOutMessage);

	//cerr << (kernResult == KERN_SUCCESS ? "KERN_SUCCESS: " : "(Failed): ") << *pInOutMessage << endl;
	return kernResult == KERN_SUCCESS;

}	//	NTV2Message



#pragma mark Old Driver Calls


//--------------------------------------------------------------------------------------------------------------------
//	GetDriverVersion
//
//	Return the driver interface version and release version.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::GetDriverVersion (ULWord * pOutDriverVersion)
{
	if (!pOutDriverVersion)
		return false;
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
		{MDIFAIL (INSTP(this) << ": GetDriverVersion:  Cannot retrieve driver version for this remote device type");	return false;}
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	*pOutDriverVersion = gDeviceMap.GetDriverVersion ();
	return true;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Debug Levels
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetUserModeDebugLevel( ULWord level )
{
	return WriteRegister(kVRegMacUserModeDebugLevel, level);
}

bool CNTV2MacDriverInterface::GetUserModeDebugLevel( ULWord* level )
{
	return ReadRegister(kVRegMacUserModeDebugLevel, level);
}

bool CNTV2MacDriverInterface::SetKernelModeDebugLevel( ULWord level )
{
	return WriteRegister(kVRegMacKernelModeDebugLevel, level);
}

bool CNTV2MacDriverInterface::GetKernelModeDebugLevel( ULWord* level )
{
	return ReadRegister(kVRegMacKernelModeDebugLevel, level);
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Ping Levels
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetUserModePingLevel( ULWord level )
{
	return WriteRegister(kVRegMacUserModePingLevel,level);
}

bool CNTV2MacDriverInterface::GetUserModePingLevel( ULWord* level )
{
	return ReadRegister(kVRegMacUserModePingLevel,level);
}

bool CNTV2MacDriverInterface::SetKernelModePingLevel( ULWord level )
{
	return WriteRegister(kVRegMacKernelModePingLevel,level);
}

bool CNTV2MacDriverInterface::GetKernelModePingLevel( ULWord* level )
{
	return ReadRegister(kVRegMacKernelModePingLevel,level);
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Latency Timer
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetLatencyTimerValue( ULWord value )
{
	return WriteRegister(kVRegLatencyTimerValue,value);
}

bool CNTV2MacDriverInterface::GetLatencyTimerValue( ULWord* value )
{
	return ReadRegister(kVRegLatencyTimerValue,value);
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Output Timecode settings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetOutputTimecodeOffset( ULWord frames )
{
	return WriteRegister(kVRegOutputTimecodeOffset, frames);
}

bool CNTV2MacDriverInterface::GetOutputTimecodeOffset( ULWord* pFrames )
{
	return ReadRegister(kVRegOutputTimecodeOffset, pFrames);
}

bool CNTV2MacDriverInterface::SetOutputTimecodeType( ULWord type )
{
	return WriteRegister( kVRegOutputTimecodeType, type );
}

bool CNTV2MacDriverInterface::GetOutputTimecodeType( ULWord* pType )
{
	return ReadRegister(kVRegOutputTimecodeType, pType);
}


//--------------------------------------------------------------------------------------------------------------------
//	ReadRP188Registers
//
//	Read the current RP188 registers (which typically give you the timecode corresponding
//   to the LAST frame).
//
//   NOTE: this is a hack to avoid making a "real" driver call!
//   Since the RP188 data requires three ReadRegister() calls, there is a chance that it
//   can straddle a VBI, which could give skewed results. To try to avoid this, we read the
//   3 registers until we get two consecutive passes that give us the same data (someday it
//   would be nice if the driver automatically read these as part of its VBI IRQ handler...)
//--------------------------------------------------------------------------------------------------------------------

bool CNTV2MacDriverInterface::ReadRP188Registers( NTV2Channel /*channel-not-used*/, RP188_STRUCT* pRP188Data )
{
	bool bSuccess = false;
	RP188_STRUCT rp188;
	NTV2DeviceID boardID = DEVICE_ID_NOTFOUND;
	RP188SourceSelect source = kRP188SourceEmbeddedLTC;
	ULWord dbbReg, msReg, lsReg;

	ReadRegister(kRegBoardID, (ULWord *)&boardID);
	ReadRegister(kVRegRP188SourceSelect, (ULWord *)&source);
	bool bLTCPort = (source == kRP188SourceLTCPort);

	// values come from LTC port registers
	if (bLTCPort)
	{
		ULWord ltcPresent;
		ReadRegister (kRegStatus, &ltcPresent, kRegMaskLTCInPresent, kRegShiftLTCInPresent);

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

		if(NTV2DeviceGetNumVideoInputs(boardID) > 1)
		{

			ReadRegister (kVRegInputSelect, (ULWord *)&inputSelect);
			channel = (inputSelect == NTV2_Input1Select) ? NTV2_CHANNEL1 : NTV2_CHANNEL2;
		}
		else
		{
			channel = NTV2_CHANNEL1;
		}

		// rp188 registers
		dbbReg = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1DBB : kRegRP188InOut2DBB);
		//Check to see if TC is received
		uint32_t tcReceived = 0;
		ReadRegister(dbbReg, &tcReceived, BIT(16), 16);
		if(tcReceived == 0)
			return false;//No TC recevied

		ReadRegister (dbbReg, &rp188.DBB, kRegMaskRP188DBB, kRegShiftRP188DBB );
		switch(rp188.DBB)//What do we have?
		{
		default:
		case 0x01:
		case 0x02:
			{
				//We have VITC - what do we want?
				if(pRP188Data->DBB == 0x01 || pRP188Data->DBB == 0x02)
				{
					//We want VITC
					msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
					lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
					break;
				}
				else
				{
					//We want Embedded LTC, so we should check one other place
					uint32_t ltcPresent = 0;
					ReadRegister(dbbReg, &ltcPresent, BIT(18), 18);
					if(ltcPresent == 1)
					{
						//Read LTC registers
						msReg  = (channel == NTV2_CHANNEL1 ? kRegLTCEmbeddedBits0_31  : kRegLTC2EmbeddedBits0_31 );
						lsReg  = (channel == NTV2_CHANNEL1 ? kRegLTCEmbeddedBits32_63 : kRegLTC2EmbeddedBits32_63);
						break;
					}
					else
						return false;
				}
			}
		case 0x00:
			{
				//We have LTC - do we want it?
				if(pRP188Data->DBB != 0x00)
					return false;
				else
				{
					msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
					lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
				}
				break;
			}
		}
		//Re-Read the whole register just in case something is expecting other status values
		ReadRegister (dbbReg, &rp188.DBB);
	}
	ReadRegister (msReg,  &rp188.Low );
	ReadRegister (lsReg,  &rp188.High);

	// register stability filter
	do
	{
		// struct copy to result
		*pRP188Data = rp188;

		// read again into local struct
		if (!bLTCPort)
			ReadRegister (dbbReg, &rp188.DBB );
		ReadRegister (msReg,  &rp188.Low );
		ReadRegister (lsReg,  &rp188.High);

		// if the new read equals the previous read, consider it done
		if ( (rp188.DBB  == pRP188Data->DBB) &&
			 (rp188.Low  == pRP188Data->Low) &&
			 (rp188.High == pRP188Data->High) )
		{
			bSuccess = true;
		}

	} while (bSuccess == false);

	return true;
}


//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Audio AV Sync Enable
//
//	Note: Our core audio driver looks at this virtual register during playback and if it is
//   set it will wait for a markFirstFrame call from the muxer so that audio and video will
//   be in sync.  Otherwise it will start right away, the assumption is no muxer is present
//   and we are in stand alone audio mode.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetAudioAVSyncEnable( bool enable )
{
	return WriteRegister(kVRegAudioAVSyncEnable, enable);
}

bool CNTV2MacDriverInterface::GetAudioAVSyncEnable( bool* enable )
{
	return ReadRegister(kVRegAudioAVSyncEnable, (ULWord *)enable);
}


//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Audio Output Mode
//
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode)
{
	return WriteRegister(kVRegGlobalAudioPlaybackMode,mode);
}

bool CNTV2MacDriverInterface::GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode)
{
	return ReadRegister(kVRegGlobalAudioPlaybackMode,(ULWord*)mode);
}


//--------------------------------------------------------------------------------------------------------------------
//	Sleep
//	Input:  Time to sleep in milliseconds
//	Output: NONE
//	Returns: 0 on success, -1 if interrupted by a signal
//	Notes: Millisecond sleep function not available on Linux.  usleep() is obsolete.
//		  This method uses the POSIX.1b-compliant nanosleep() function.
//--------------------------------------------------------------------------------------------------------------------
Word CNTV2MacDriverInterface::SleepMs( LWord milliseconds ) const
{
   timespec req;

   req.tv_sec = milliseconds / 1000;
   req.tv_nsec = 1000000UL *(milliseconds - (1000 * req.tv_sec));
   return ::nanosleep(&req, NULL); // NULL: don't care about remaining time if interrupted for now
}


//--------------------------------------------------------------------------------------------------------------------
// Method: SwitchBitfile
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SwitchBitfile( NTV2DeviceID boardID, NTV2BitfileType bitfile )
{
	(void) boardID;
	(void) bitfile;
	return false;
}


bool CNTV2MacDriverInterface::ConfigureSubscription (bool bSubscribe, INTERRUPT_ENUMS eInterruptType, PULWord & hSubscription)
{
	return CNTV2DriverInterface::ConfigureSubscription (bSubscribe, eInterruptType, hSubscription);
}


//-------------------------------------------------------------------------------------------------------
//	CopyTo_AUTOCIRCULATE_DATA_64
//-------------------------------------------------------------------------------------------------------
void CNTV2MacDriverInterface::CopyTo_AUTOCIRCULATE_DATA_64 (AUTOCIRCULATE_DATA *p, AUTOCIRCULATE_DATA_64 *p64)
{
	// note that p is native structure, either 64 or 32 bit
	p64->eCommand		= p->eCommand;
	p64->channelSpec	= p->channelSpec;

	p64->lVal1			= p->lVal1;
	p64->lVal2			= p->lVal2;
	p64->lVal3			= p->lVal3;
	p64->lVal4			= p->lVal4;
	p64->lVal5			= p->lVal4;
	p64->lVal6			= p->lVal5;

	p64->bVal1			= p->bVal1;
	p64->bVal2			= p->bVal2;
	p64->bVal3			= p->bVal3;
	p64->bVal4			= p->bVal4;
	p64->bVal5			= p->bVal5;
	p64->bVal6			= p->bVal6;
	p64->bVal7			= p->bVal7;
	p64->bVal8			= p->bVal8;

	p64->pvVal1			= (Pointer64) p->pvVal1;	// native to 64 bit
	p64->pvVal2			= (Pointer64) p->pvVal2;	// native to 64 bit
	p64->pvVal3			= (Pointer64) p->pvVal3;	// native to 64 bit
	p64->pvVal4			= (Pointer64) p->pvVal4;	// native to 64 bit
}


//-------------------------------------------------------------------------------------------------------
//	CopyTo_AUTOCIRCULATE_DATA
//-------------------------------------------------------------------------------------------------------
void CNTV2MacDriverInterface::CopyTo_AUTOCIRCULATE_DATA (AUTOCIRCULATE_DATA_64 *p64, AUTOCIRCULATE_DATA *p)
{
	// note that p is native structure, either 64 or 32 bit
	p->eCommand			= p64->eCommand;
	p->channelSpec		= p64->channelSpec;

	p->lVal1			= p64->lVal1;
	p->lVal2			= p64->lVal2;
	p->lVal3			= p64->lVal3;
	p->lVal4			= p64->lVal4;
	p->lVal5			= p64->lVal4;
	p->lVal6			= p64->lVal5;

	p->bVal1			= p64->bVal1;
	p->bVal2			= p64->bVal2;
	p->bVal3			= p64->bVal3;
	p->bVal4			= p64->bVal4;
	p->bVal5			= p64->bVal5;
	p->bVal6			= p64->bVal6;
	p->bVal7			= p64->bVal7;
	p->bVal8			= p64->bVal8;

	p->pvVal1			= (void*) p64->pvVal1;	// 64 bit to native
	p->pvVal2			= (void*) p64->pvVal2;	// 64 bit to native
	p->pvVal3			= (void*) p64->pvVal3;	// 64 bit to native
	p->pvVal4			= (void*) p64->pvVal4;	// 64 bit to native
}


//-------------------------------------------------------------------------------------------------------
//	CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64
//-------------------------------------------------------------------------------------------------------
void CNTV2MacDriverInterface::CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64 (AUTOCIRCULATE_TRANSFER_STRUCT *p, AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64)
{
	// note that p is native structure, either 64 or 32 bit
	p64->channelSpec				= p->channelSpec;
	p64->videoBuffer				= (Pointer64) p->videoBuffer;	// native to 64 bit
	p64->videoBufferSize			= p->videoBufferSize;
	p64->videoDmaOffset				= p->videoDmaOffset;
	p64->audioBuffer				= (Pointer64) p->audioBuffer;	// native to 64 bit
	p64->audioBufferSize			= p->audioBufferSize;
	p64->audioStartSample			= p->audioStartSample;
	p64->audioNumChannels			= p->audioNumChannels;
	p64->frameRepeatCount			= p->frameRepeatCount;

	p64->rp188.DBB					= p->rp188.DBB;
	p64->rp188.Low					= p->rp188.Low;
	p64->rp188.High					= p->rp188.High;

	p64->desiredFrame				= p->desiredFrame;
	p64->hUser						= p->hUser;
	p64->transferFlags				= p->transferFlags;
	p64->bDisableExtraAudioInfo		= p->bDisableExtraAudioInfo;
	p64->frameBufferFormat			= p->frameBufferFormat;
	p64->frameBufferOrientation		= p->frameBufferOrientation;

	p64->colorCorrectionInfo.mode				= p->colorCorrectionInfo.mode;
	p64->colorCorrectionInfo.saturationValue	= p->colorCorrectionInfo.saturationValue;
	p64->colorCorrectionInfo.ccLookupTables		= (Pointer64) p->colorCorrectionInfo.ccLookupTables;

	p64->vidProcInfo.mode						= p->vidProcInfo.mode;
	p64->vidProcInfo.foregroundVideoCrosspoint	= p->vidProcInfo.foregroundVideoCrosspoint;
	p64->vidProcInfo.backgroundVideoCrosspoint	= p->vidProcInfo.backgroundVideoCrosspoint;
	p64->vidProcInfo.foregroundKeyCrosspoint	= p->vidProcInfo.foregroundKeyCrosspoint;
	p64->vidProcInfo.backgroundKeyCrosspoint	= p->vidProcInfo.backgroundKeyCrosspoint;
	p64->vidProcInfo.transitionCoefficient		= p->vidProcInfo.transitionCoefficient;
	p64->vidProcInfo.transitionSoftness			= p->vidProcInfo.transitionSoftness;

	p64->customAncInfo.Group1					= p->customAncInfo.Group1;
	p64->customAncInfo.Group2					= p->customAncInfo.Group2;
	p64->customAncInfo.Group3					= p->customAncInfo.Group3;
	p64->customAncInfo.Group4					= p->customAncInfo.Group4;

	p64->videoNumSegments			= p->videoNumSegments;
	p64->videoSegmentHostPitch		= p->videoSegmentHostPitch;
	p64->videoSegmentCardPitch		= p->videoSegmentCardPitch;

	p64->videoQuarterSizeExpand		= p->videoQuarterSizeExpand;

#if 0

	printf("----------------------\n");
	printf("sizeof					= %d\n", (int)sizeof(AUTOCIRCULATE_TRANSFER_STRUCT_64));

	uint8_t * ptr = (uint8_t *)p64;
	for (int i = 0; i < (int)sizeof(AUTOCIRCULATE_TRANSFER_STRUCT_64); i++)
	{
		if ((i % 4) == 0)
			printf("\n");
		printf("%x ", *ptr++);
	}
	printf("\n\n", *ptr++);

#endif


	#if 0
		printf("----------------------\n");
		printf("sizeof					= %d\n", (int)sizeof(AUTOCIRCULATE_TRANSFER_STRUCT_64));


	// note that p is native structure, either 64 or 32 bit
	printf("channelSpec %x\n", p64->channelSpec);
	printf("videoBuffer %lx\n",p64->videoBuffer);
	printf("videoBufferSize %x\n",p64->videoBufferSize);
	printf("videoDmaOffset %x\n",p64->videoDmaOffset);
	printf("audioBuffer %lx\n",p64->audioBuffer);
	printf("audioBufferSize %x\n",p64->audioBufferSize);
	printf("audioStartSample %x\n",p64->audioStartSample);
	printf("audioNumChannels %x\n",p64->audioNumChannels);
	printf("frameRepeatCount %x\n",p64->frameRepeatCount);

	printf("rp188.DBB %x\n",p64->rp188.DBB);
	printf("rp188.Low %x\n",p64->rp188.Low);
	printf("rp188.High %x\n",p64->rp188.High);

	printf("desiredFrame %x\n",p64->desiredFrame);
	printf("hUser %x\n",p64->hUser);
	printf("transferFlags %x\n",p64->transferFlags);
	printf("bDisableExtraAudioInfo %x\n",p64->bDisableExtraAudioInfo);
	printf("frameBufferFormat %x\n",p64->frameBufferFormat);
	printf("frameBufferOrientation %x\n",p64->frameBufferOrientation);

	printf("colorCorrectionInfo.mode %x\n",p64->colorCorrectionInfo.mode);
	printf("colorCorrectionInfo.saturationValue %x\n",p64->colorCorrectionInfo.saturationValue);
	printf("colorCorrectionInfo.ccLookupTables %x\n",p64->colorCorrectionInfo.ccLookupTables);

	printf("vidProcInfo.mode %x\n",p64->vidProcInfo.mode);
	printf("vidProcInfo.foregroundVideoCrosspoint %x\n",p64->vidProcInfo.foregroundVideoCrosspoint);
	printf("vidProcInfo.backgroundVideoCrosspoint %x\n",p64->vidProcInfo.backgroundVideoCrosspoint);
	printf("vidProcInfo.foregroundKeyCrosspoint %x\n",p64->vidProcInfo.foregroundKeyCrosspoint);
	printf("vidProcInfo.backgroundKeyCrosspoint %x\n",p64->vidProcInfo.backgroundKeyCrosspoint);
	printf("vidProcInfo.transitionCoefficient %x\n",p64->vidProcInfo.transitionCoefficient);
	printf("vidProcInfo.transitionSoftness %x\n",p64->vidProcInfo.transitionSoftness);

	printf("customAncInfo.Group1 %x\n",p64->customAncInfo.Group1);
	printf("customAncInfo.Group2 %x\n",p64->customAncInfo.Group2);
	printf("customAncInfo.Group3 %x\n",p64->customAncInfo.Group3);
	printf("customAncInfo.Group4 %x\n",p64->customAncInfo.Group4);

	printf("videoNumSegments %x\n",p64->videoNumSegments);
	printf("videoSegmentHostPitch %x\n",p64->videoSegmentHostPitch);
	printf("videoSegmentCardPitch %x\n",p64->videoSegmentCardPitch);

	printf("videoQuarterSizeExpand %x\n",p64->videoQuarterSizeExpand);

	#endif



}


//-------------------------------------------------------------------------------------------------------
//	CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT
//-------------------------------------------------------------------------------------------------------
void CNTV2MacDriverInterface::CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT (AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64, AUTOCIRCULATE_TRANSFER_STRUCT *p)
{
	// note that p is native structure, either 64 or 32 bit
	p->channelSpec					= p64->channelSpec;
	p->videoBuffer					= (ULWord *) p64->videoBuffer;	// 64 bit to native
	p->videoBufferSize				= p64->videoBufferSize;
	p->videoDmaOffset				= p64->videoDmaOffset;
	p->audioBuffer					= (ULWord *) p64->audioBuffer;	// 64 bit to native
	p->audioBufferSize				= p64->audioBufferSize;
	p->audioStartSample				= p64->audioStartSample;
	p->audioNumChannels				= p64->audioNumChannels;
	p->frameRepeatCount				= p64->frameRepeatCount;

	p->rp188.DBB					= p64->rp188.DBB;
	p->rp188.Low					= p64->rp188.Low;
	p->rp188.High					= p64->rp188.High;

	p->desiredFrame					= p64->desiredFrame;
	p->hUser						= p64->hUser;
	p->transferFlags				= p64->transferFlags;
	p->bDisableExtraAudioInfo		= p64->bDisableExtraAudioInfo;
	p->frameBufferFormat			= p64->frameBufferFormat;
	p->frameBufferOrientation		= p64->frameBufferOrientation;

	p->colorCorrectionInfo.mode					= p64->colorCorrectionInfo.mode;
	p->colorCorrectionInfo.saturationValue		= p64->colorCorrectionInfo.saturationValue;
	p->colorCorrectionInfo.ccLookupTables		= (ULWord *) p64->colorCorrectionInfo.ccLookupTables;

	p->vidProcInfo.mode							= p64->vidProcInfo.mode;
	p->vidProcInfo.foregroundVideoCrosspoint	= p64->vidProcInfo.foregroundVideoCrosspoint;
	p->vidProcInfo.backgroundVideoCrosspoint	= p64->vidProcInfo.backgroundVideoCrosspoint;
	p->vidProcInfo.foregroundKeyCrosspoint		= p64->vidProcInfo.foregroundKeyCrosspoint;
	p->vidProcInfo.backgroundKeyCrosspoint		= p64->vidProcInfo.backgroundKeyCrosspoint;
	p->vidProcInfo.transitionCoefficient		= p64->vidProcInfo.transitionCoefficient;
	p->vidProcInfo.transitionSoftness			= p64->vidProcInfo.transitionSoftness;

	p->customAncInfo.Group1					= p64->customAncInfo.Group1;
	p->customAncInfo.Group2					= p64->customAncInfo.Group2;
	p->customAncInfo.Group3					= p64->customAncInfo.Group3;
	p->customAncInfo.Group4					= p64->customAncInfo.Group4;

	p->videoNumSegments				= p64->videoNumSegments;
	p->videoSegmentHostPitch		= p64->videoSegmentHostPitch;
	p->videoSegmentCardPitch		= p64->videoSegmentCardPitch;

	p->videoQuarterSizeExpand		= p64->videoQuarterSizeExpand;
}


//-------------------------------------------------------------------------------------------------------
//	CopyTo_AUTOCIRCULATE_TASK_STRUCT_64
//-------------------------------------------------------------------------------------------------------
void CNTV2MacDriverInterface::CopyTo_AUTOCIRCULATE_TASK_STRUCT_64 (AUTOCIRCULATE_TASK_STRUCT *p, AUTOCIRCULATE_TASK_STRUCT_64 *p64)
{
	p64->taskVersion			= p->taskVersion;
	p64->taskSize				= p->taskSize;
	p64->numTasks				= p->numTasks;
	p64->maxTasks				= p->maxTasks;
	p64->taskArray				= (Pointer64)p->taskArray;
	p64->reserved0				= p->reserved0;
	p64->reserved1				= p->reserved1;
	p64->reserved2				= p->reserved2;
	p64->reserved3				= p->reserved3;
}


void CNTV2MacDriverInterface::SetDebugLogging (const uint64_t inWhichUserClientCommands)
{
	gErrorLogging = inWhichUserClientCommands;
	cerr << "CNTV2MacDriverInterface::SetDebugLogging 0x" << hex << gErrorLogging << dec << endl;
}


void CNTV2MacDriverInterface::DumpDeviceMap (void)
{
	gDeviceMap.Dump ();
}

UWord CNTV2MacDriverInterface::GetConnectionCount (void)
{
	return gDeviceMap.GetConnectionCount ();
}

ULWord CNTV2MacDriverInterface::GetConnectionChecksum (void)
{
	return gDeviceMap.GetConnectionChecksum ();
}


static const char * GetKernErrStr (const kern_return_t inError)
{
	switch (inError)
	{
		case kIOReturnError:			return "general error";
		case kIOReturnNoMemory:			return "can't allocate memory";
		case kIOReturnNoResources:		return "resource shortage";
		case kIOReturnIPCError:			return "error during IPC";
		case kIOReturnNoDevice:			return "no such device";
		case kIOReturnNotPrivileged:	return "privilege violation";
		case kIOReturnBadArgument:		return "invalid argument";
		case kIOReturnLockedRead:		return "device read locked";
		case kIOReturnLockedWrite:		return "device write locked";
		case kIOReturnExclusiveAccess:	return "exclusive access and device already open";
		case kIOReturnBadMessageID:		return "sent/received messages had different msg_id";
		case kIOReturnUnsupported:		return "unsupported function";
		case kIOReturnVMError:			return "misc. VM failure";
		case kIOReturnInternalError:	return "internal error";
		case kIOReturnIOError:			return "General I/O error";
		case kIOReturnCannotLock:		return "can't acquire lock";
		case kIOReturnNotOpen:			return "device not open";
		case kIOReturnNotReadable:		return "read not supported";
		case kIOReturnNotWritable:		return "write not supported";
		case kIOReturnNotAligned:		return "alignment error";
		case kIOReturnBadMedia:			return "Media Error";
		case kIOReturnStillOpen:		return "device(s) still open";
		case kIOReturnRLDError:			return "rld failure";
		case kIOReturnDMAError:			return "DMA failure";
		case kIOReturnBusy:				return "Device Busy";
		case kIOReturnTimeout:			return "I/O Timeout";
		case kIOReturnOffline:			return "device offline";
		case kIOReturnNotReady:			return "not ready";
		case kIOReturnNotAttached:		return "device not attached";
		case kIOReturnNoChannels:		return "no DMA channels left";
		case kIOReturnNoSpace:			return "no space for data";
		case kIOReturnPortExists:		return "port already exists";
		case kIOReturnCannotWire:		return "can't wire down physical memory";
		case kIOReturnNoInterrupt:		return "no interrupt attached";
		case kIOReturnNoFrames:			return "no DMA frames enqueued";
		case kIOReturnMessageTooLarge:	return "oversized msg received on interrupt port";
		case kIOReturnNotPermitted:		return "not permitted";
		case kIOReturnNoPower:			return "no power to device";
		case kIOReturnNoMedia:			return "media not present";
		case kIOReturnUnformattedMedia:	return "media not formatted";
		case kIOReturnUnsupportedMode:	return "no such mode";
		case kIOReturnUnderrun:			return "data underrun";
		case kIOReturnOverrun:			return "data overrun";
		case kIOReturnDeviceError:		return "the device is not working properly!";
		case kIOReturnNoCompletion:		return "a completion routine is required";
		case kIOReturnAborted:			return "operation aborted";
		case kIOReturnNoBandwidth:		return "bus bandwidth would be exceeded";
		case kIOReturnNotResponding:	return "device not responding";
		case kIOReturnIsoTooOld:		return "isochronous I/O request for distant past!";
		case kIOReturnIsoTooNew:		return "isochronous I/O request for distant future";
		case kIOReturnNotFound:			return "data was not found";
		case kNTV2DriverBadDMA:			return "bad dma engine num";
		case kNTV2DriverDMABusy:		return "dma engine busy, or none available";
		case kNTV2DriverParamErr:		return "bad aja parameter (out of range)";
		case kNTV2DriverPgmXilinxErr:	return "xilinx programming error";
		case kNTV2DriverNotReadyErr:	return "xilinx not yet programmed";
		case kNTV2DriverPrepMemErr:		return "error preparing memory (no room?)";
		case kNTV2DriverDMATooLarge:	return "dma xfer too large, or out of range";
		case kNTV2DriverBadHeaderTag:	return "bad NTV2 header";
		case kNTV2UnknownStructType:	return "unknown NTV2 struct type";
		case kNTV2HeaderVersionErr:		return "bad or unsupported NTV2 header version";
		case kNTV2DriverBadTrailerTag:	return "bad NTV2 trailer";
		case kNTV2DriverMapperErr:		return "failure while mapping NTV2 struct ptrs";
		case kNTV2DriverUnmapperErr:	return "failure while unmapping NTV2 struct ptrs";
		default:						return "";
	}
}	//	GetKernErrStr


class MDIEnvVarReader
{
	public:
		MDIEnvVarReader ()
		{
			//	To allow some logging flexibility at runtime... set the 'NTV2CLIENTLOG' environment variable to a
			//	hex string value '0x0000000000000000' that will be interpreted as a uint64_t...
			const char *	pVarName("NTV2CLIENTLOG");
			const string	sEnvLog	(::getenv (pVarName) ? ::getenv (pVarName) : "");
			if (sEnvLog.length () == 18 && sEnvLog.find ("0x") == 0)
			{
				ULWord64			logMask (0);
				std::stringstream	ss;
				ss << hex << sEnvLog.substr (2, 16);
				ss >> logMask;
				CNTV2MacDriverInterface::SetDebugLogging (logMask);
			}
		}
};	//	MDIEnvVarReader

static MDIEnvVarReader	gMDIEnvVarReader;
