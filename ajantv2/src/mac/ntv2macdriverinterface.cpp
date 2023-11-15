/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2macdriverinterface.h
	@brief		Implements the MacOS-specific flavor of CNTV2DriverInterface.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc.
**/

#include "ntv2macdriverinterface.h"
#include "ntv2nubaccess.h"
#include "ntv2utils.h"
#include <time.h>
#include <syslog.h>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <iomanip>
#include "ntv2devicefeatures.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/atomic.h"
#include "ajabase/system/systemtime.h"

#if !defined (NTV2_NULL_DEVICE)
	extern "C"
	{
		#include <mach/mach.h>
	}
#endif	//	!defined (NTV2_NULL_DEVICE)

using namespace std;


static const char *		GetKernErrStr (const kern_return_t inError);
static AJALock			gLegalDevIDsLock;
static NTV2DeviceIDSet	gLegalDeviceIDs;
static const string		sNTV2PCIKEXTClassName	("com_aja_iokit_ntv2");	//	AJA NTV2 KEXT's IOService class name
static const string		sNTV2PCIDEXTName		("NTV2PCIe");			//	AJA NTV2 DEXT's IOService name


//	MacDriverInterface-specific Logging Macros

#define HEX2(__x__)			xHEX0N(0xFF		  & uint8_t (__x__),2)
#define HEX4(__x__)			xHEX0N(0xFFFF	  & uint16_t(__x__),4)
#define HEX8(__x__)			xHEX0N(0xFFFFFFFF & uint32_t(__x__),8)
#define HEX16(__x__)		xHEX0N(uint64_t(__x__),16)
#define KR(_kr_)			"kernResult=" << HEX8(_kr_) << "(" << GetKernErrStr(_kr_) << ")"
#define INSTP(_p_)			HEX0N(uint64_t(_p_),16)

#define DIFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

#define MDIFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_DriverInterface, AJAFUNC << ": " << __x__)
#define MDIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface, AJAFUNC << ": " << __x__)
#define MDINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface, AJAFUNC << ": " << __x__)
#define MDIINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_DriverInterface, AJAFUNC << ": " << __x__)
#define MDIDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_DriverInterface, AJAFUNC << ": " << __x__)


#if !defined (NTV2_NULL_DEVICE)
	//	This section builds 'libajantv2.a' with the normal linkage to the IOKit...
	#define OS_IOMasterPort(_x_,_y_)											::IOMasterPort ((_x_), (_y_))
	#define OS_IOServiceOpen(_w_,_x_,_y_,_z_)									::IOServiceOpen((_w_), (_x_), (_y_), (_z_))
	#define OS_IOServiceClose(_x_)												::IOServiceClose ((_x_))
	#define OS_IOServiceMatching(_x_)											::IOServiceMatching ((_x_))
	#define OS_IOServiceNameMatching(_x_)										::IOServiceNameMatching((_x_))
	#define OS_IOServiceGetMatchingServices(_x_,_y_,_z_)						::IOServiceGetMatchingServices ((_x_), (_y_), (_z_))
	#define OS_IOIteratorNext(_x_)												::IOIteratorNext ((_x_))
	#define OS_IOObjectRelease(_x_)												::IOObjectRelease ((_x_))
	#define OS_IORegistryEntryCreateCFProperty(_w_,_x_,_y_,_z_)					::IORegistryEntryCreateCFProperty ((_w_), (_x_), (_y_), (_z_))
	#define OS_IOConnectCallMethod(_q_,_r_,_s_,_t_,_u_,_v_,_w_,_x_,_y_,_z_)		::IOConnectCallMethod ((_q_), (_r_), (_s_), (_t_), (_u_), (_v_), (_w_), (_x_), (_y_), (_z_))
	#define OS_IOConnectCallScalarMethod(_u_,_v_,_w_,_x_,_y_,_z_)				::IOConnectCallScalarMethod ((_u_), (_v_), (_w_), (_x_), (_y_), (_z_))
	#define OS_IOConnectCallStructMethod(_u_,_v_,_w_,_x_,_y_,_z_)				::IOConnectCallStructMethod ((_u_), (_v_), (_w_), (_x_), (_y_), (_z_))
	#define OS_IOConnectMapMemory(_u_,_v_,_w_,_x_,_y_,_z_)						::IOConnectMapMemory ((_u_), (_v_), (_w_), (_x_), (_y_), (_z_))
	#define OS_IOKitGetBusyState(_x_,_y_)										::IOKitGetBusyState ((_x_), (_y_))
	#define OS_IOKitWaitQuiet(_x_,_y_)											::IOKitWaitQuiet ((_x_), (_y_))
#else	//	NTV2_NULL_DEVICE defined
	//	STUB FUNCTIONS TO REPLACE IOKIT CALLS
	//	Eliminates all linkage to IOKit.
	static IOReturn OS_IOMasterPort (const mach_port_t inPort, mach_port_t * pOutPort)
	{	(void)inPort;
		MDIWARN("NTV2_NULL_DEVICE -- will not connect to IOKit");
		if (pOutPort)
			*pOutPort = 1;
		return KERN_SUCCESS;
	}
	static kern_return_t OS_IOServiceOpen (const io_service_t inSvc, const task_port_t inTask, const uint32_t inType, io_connect_t * pOutConn)
	{	(void)inSvc; (void)inTask; (void)inType;
		if (pOutConn) *pOutConn = 0;
		return KERN_SUCCESS;
	}
	static IOReturn OS_IOServiceClose (const io_connect_t inConnection)
	{	(void)inConnection;
		return KERN_SUCCESS;
	}
	static CFMutableDictionaryRef OS_IOServiceMatching (const char * pInName)
	{	(void)pInName;
		return AJA_NULL;
	}
	static IOReturn OS_IOServiceGetMatchingServices (const mach_port_t inPort, const CFDictionaryRef inMatch, io_iterator_t * pOutIter)
	{	(void)inPort; (void)inMatch;
		if (pOutIter)
			*pOutIter = 0;
		return KERN_SUCCESS;
	}
	static io_object_t OS_IOIteratorNext (io_iterator_t inIter)
	{	(void)inIter;
		return 0;
	}
	static IOReturn OS_IOObjectRelease (io_object_t inObj)
	{	(void)inObj;
		return KERN_SUCCESS;
	}
	static CFTypeRef OS_IORegistryEntryCreateCFProperty (const io_registry_entry_t inEntry, const CFStringRef inKey, const CFAllocatorRef inAllocator, const IOOptionBits inOptions)
	{	(void)inEntry; (void)inKey; (void)inAllocator; (void)inOptions;
		return AJA_NULL;
	}
	static IOReturn OS_IOConnectCallScalarMethod (const mach_port_t inConnect, const uint32_t inSelector, const uint64_t * pInput, const uint32_t inCount, uint64_t * pOutput, uint32_t * pOutCount)
	{	(void)inConnect; (void)inSelector; (void)pInput; (void)inCount;
		if (pOutput)	*pOutput = 0;
		if (pOutCount)	*pOutCount = 0;
		return KERN_SUCCESS;
	}
	static kern_return_t OS_IOConnectCallMethod (const mach_port_t inConnect, const uint32_t inSelector, const uint64_t * pInput, uint32_t inputCnt, const void * pInStruct, const size_t inStructCnt, uint64_t * pOutput, uint32_t * pOutputCnt, void * pOutStruct, size_t * pOutStructCnt)
	{	(void)inConnect; (void)inSelector; (void)pInput; (void)inputCnt; (void)pInStruct; (void)inStructCnt; (void)pOutput; (void)pOutputCnt; (void)pOutStruct; (void)pOutStructCnt;
		return KERN_SUCCESS;
	}
	static IOReturn OS_IOKitGetBusyState (const mach_port_t inPort, uint32_t * pOutState)
	{	(void)inPort;
		if (pOutState)
			*pOutState = 0;
		return KERN_SUCCESS;
	}
	static IOReturn OS_IOKitWaitQuiet (const mach_port_t inPort, const mach_timespec_t * pInOutWait)
	{	(void)inPort; (void)pInOutWait;
		return KERN_SUCCESS;
	}
	static IOReturn OS_IOConnectCallStructMethod (const io_connect_t inPort, const uint32_t inSelector, const void *inputStruct, size_t inputStructCnt, void *pOutStruct, size_t *pOutStructCnt)
	{	(void)inPort; (void)inSelector; (void)inputStruct; (void)inputStructCnt; (void)pOutStruct; (void)pOutStructCnt;
		return KERN_SUCCESS;
	}
	static kern_return_t OS_IOConnectMapMemory (const io_connect_t inPort, const uint32_t inMemType, const task_port_t inTask, mach_vm_address_t * pAddr, mach_vm_size_t * pOutSize, const IOOptionBits inOpts)
	{	(void)inPort; (void)inMemType; (void)inTask; (void)pAddr; (void)pOutSize; (void)inOpts;
		return KERN_SUCCESS;
	}
#endif	//	NTV2_NULL_DEVICE defined


#if defined(USE_DEVICE_MAP)
static unsigned		gnBoardMaps				(0);		//	Instance counter -- should never exceed one
static const UWord	kMaxNumDevices			(32);		//	Limit to 32 devices
static uint64_t		RECHECK_INTERVAL		(1024LL);	//	Number of calls to DeviceMap::GetConnection before connection recheck performed
static unsigned int	TWO_SECONDS				(2);		//	Max wait times for IORegistry to settle for hot plug/unplug
#define				NTV2_IGNORE_IOREG_BUSY	(true)		//	If defined, ignore IORegistry busy state;
														//	otherwise wait for non-busy IORegistry before making new connections

/**
	@details	The DeviceMap global singleton maintains the underlying Mach connection handles used to talk
				to the NTV2 driver instances for each device on the host.
**/
class DeviceMap
{
	public:
		DeviceMap ()
			:	//mIOConnections,
				//mRecheckTally,
				//mMutex,
				mMasterPort		(0),
				mStopping		(false),
				mDriverVersion	(0)
		{
			NTV2_ASSERT (gnBoardMaps == 0  &&  "Attempt to create more than one DeviceMap");
			AJAAtomic::Increment(&gnBoardMaps);
			::memset (&mIOConnections, 0, sizeof (mIOConnections));
			::memset (&mRecheckTally, 0, sizeof (mRecheckTally));
			mDrvrVersComps[0] = mDrvrVersComps[1] = mDrvrVersComps[2] = mDrvrVersComps[3] = 0;

			//	Get the master mach port for talking to IOKit...
			IOReturn	error	(OS_IOMasterPort (MACH_PORT_NULL, &mMasterPort));
			if (error != kIOReturnSuccess)
			{
				MDIFAIL (KR(error) << "Unable to get master port");
				return;
			}
			NTV2_ASSERT (mMasterPort && "No MasterPort!");
			MDINOTE ("DeviceMap singleton created");
		}


		~DeviceMap ()
		{
			mStopping = true;
			AJAAutoLock autoLock (&mMutex);
			Reset ();
			MDINOTE ("DeviceMap singleton destroyed");
			AJAAtomic::Decrement(&gnBoardMaps);
		}


		void Reset (const bool inResetMasterPort = false)
		{
			AJAAutoLock autoLock (&mMutex);
			//	Clear the device map...
			for (UWord ndx (0);	 ndx < kMaxNumDevices;	++ndx)
			{
				io_connect_t	connection	(mIOConnections [ndx]);
				if (connection)
				{
					OS_IOServiceClose (connection);
					mIOConnections [ndx] = 0;
					MDINOTE ("Device " << ndx << " connection " << HEX8(connection) << " closed");
				}
			}	//	for each connection in the map
			if (inResetMasterPort)
			{
				IOReturn	error	(OS_IOMasterPort (MACH_PORT_NULL, &mMasterPort));
				if (error != kIOReturnSuccess)
					MDIFAIL (KR(error) << "Unable to reset master port");
				else
					MDINOTE ("reset mMasterPort=" << HEX8(mMasterPort));
			}
		}


		io_connect_t GetConnection (const UWord inDeviceIndex, const bool inDoNotAllocate = false)
		{
			if (inDeviceIndex >= kMaxNumDevices)
			{
				MDIWARN ("Bad device index " << inDeviceIndex << ", GetConnection fail");
				return 0;
			}

			AJAAutoLock autoLock (&mMutex);
			const io_connect_t	connection	(mIOConnections [inDeviceIndex]);
			if (connection	&&	RECHECK_INTERVAL)
			{
				uint64_t &	recheckTally	(mRecheckTally [inDeviceIndex]);
				if (++recheckTally % RECHECK_INTERVAL == 0)
				{
//					MDIDBG ("Device " << inDeviceIndex << " connection " << HEX8(connection) << " expired, checking connection");
					if (!ConnectionIsStillOkay (inDeviceIndex))
					{
						MDIFAIL ("Device " << inDeviceIndex << " connection " << HEX8(connection) << " invalid, resetting DeviceMap");
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
			io_iterator_t	ioIterator	(0);
			IOReturn		error		(kIOReturnSuccess);
			io_object_t		ioObject	(0);
			io_connect_t	ioConnect	(0);
			const string &	className	(sNTV2PCIKEXTClassName);

			NTV2_ASSERT (mMasterPort && "No MasterPort!");

			//	Create an iterator to search for our driver...
			error = OS_IOServiceGetMatchingServices (mMasterPort, OS_IOServiceMatching(className.c_str()), &ioIterator);
			if (error != kIOReturnSuccess)
			{
				MDIFAIL (KR(error) << " -- IOServiceGetMatchingServices failed, no match for '" << className << "', device index " << inDeviceIndex << " requested");
				return 0;
			}

			//	Use ndx to find nth device -- and only open that one...
			for ( ; (ioObject = OS_IOIteratorNext(ioIterator));  OS_IOObjectRelease(ioObject))
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
			error = OS_IOServiceOpen (ioObject, ::mach_task_self(), 0, &ioConnect);
			OS_IOObjectRelease (ioObject);
			if (error != kIOReturnSuccess)
			{
				MDIFAIL (KR(error) << " -- IOServiceOpen failed on device " << inDeviceIndex);
				return 0;
			}

			//	All good -- cache the connection handle...
			mIOConnections [inDeviceIndex] = ioConnect;
			mRecheckTally [inDeviceIndex] = 0;
			MDINOTE ("Device " << inDeviceIndex << " connection " << HEX8(ioConnect) << " opened");
			return ioConnect;

		}	//	GetConnection


		void Dump (const UWord inMaxNumDevices = 10) const
		{
			AJAAutoLock autoLock (&mMutex);
			for (UWord ndx (0);	 ndx < inMaxNumDevices;	 ++ndx)
				MDIDBG ("	 [" << ndx << "]:  con=" << HEX8(mIOConnections [ndx]));
		}


		UWord GetConnectionCount (void) const
		{
			UWord	tally	(0);
			AJAAutoLock autoLock (&mMutex);
			for (UWord ndx (0);	 ndx < kMaxNumDevices;	++ndx)
				if (mIOConnections [ndx])
					tally++;
				else
					break;
			return tally;
		}


		ULWord GetConnectionChecksum (void) const
		{
			ULWord	checksum	(0);
			AJAAutoLock autoLock (&mMutex);
			for (UWord ndx (0);	 ndx < kMaxNumDevices;	++ndx)
				if (mIOConnections [ndx])
					checksum += mIOConnections [ndx];
				else
					break;
			return checksum;
		}


		uint32_t	GetDriverVersion (void) const			{ return mDriverVersion; }


		bool	ConnectionIsStillOkay (const UWord inDeviceIndex)
		{
			if (inDeviceIndex >= kMaxNumDevices)
			{
				MDIWARN ("ConnectionIsStillOkay:  bad 'inDeviceIndex' parameter " << inDeviceIndex);
				return 0;
			}

			const io_connect_t	connection	(mIOConnections [inDeviceIndex]);
			if (connection)
			{
				uint64_t		scalarO_64 [2]	= {0, 0};
				uint32_t		outputCount		= 2;
				kern_return_t	kernResult		= OS_IOConnectCallScalarMethod (connection, kDriverGetStreamForApplication, AJA_NULL, 0, scalarO_64, &outputCount);
				if (kernResult == KERN_SUCCESS)
					return true;
			}
			return false;
		}

		uint64_t	SetConnectionCheckInterval (const uint64_t inNewInterval)
		{
			uint64_t	oldValue	(RECHECK_INTERVAL);
			if (oldValue != inNewInterval)
			{
				RECHECK_INTERVAL = inNewInterval;
				if (RECHECK_INTERVAL)
					MDINOTE ("connection recheck interval changed to" << HEX16(RECHECK_INTERVAL) << ", was" << HEX16(oldValue));
				else
					MDINOTE ("connection rechecking disabled, was" << HEX16(oldValue));
			}
			return oldValue;
		}

	private:
		bool WaitForBusToSettle (void)
		{
			uint32_t	busyState	(0);
			IOReturn	kr			(OS_IOKitGetBusyState (mMasterPort, &busyState));

			if (kr != kIOReturnSuccess)
				MDIFAIL ("IOKitGetBusyState failed -- " << KR(kr));
			else if (busyState)
			{
				#if defined (NTV2_IGNORE_IOREG_BUSY)
					MDINOTE ("IOKitGetBusyState reported BUSY");
					return true;	//	IORegistry busy, but so what?
				#else
					mach_timespec_t maxWaitTime = {TWO_SECONDS, 0};
					MDINOTE ("IOKitGetBusyState reported BUSY -- waiting for IORegistry to stabilize...");

					kr = OS_IOKitWaitQuiet (mMasterPort, &maxWaitTime);
					if (kr == kIOReturnSuccess)
						return true;
					MDIFAIL ("IOKitWaitQuiet timed out -- " << KR(kr));
				#endif	//	defined (NTV2_IGNORE_IOREG_BUSY)
			}
			else
				return true;
			return false;
		}

	private:
		io_connect_t	mIOConnections	[kMaxNumDevices];	//	My io_connect_t map
		uint64_t		mRecheckTally	[kMaxNumDevices];	//	Used to calc when it's time to test if connection still ok
		mutable AJALock mMutex;								//	My guard mutex
		mach_port_t		mMasterPort;						//	Handy master port
		bool			mStopping;							//	Don't open new connections if I'm stopping
		uint32_t		mDriverVersion;						//	Handy (packed) driver version
		uint16_t		mDrvrVersComps[4];					//	Handy (unpacked) driver version components

};	//	DeviceMap


static DeviceMap	gDeviceMap;		//	The DeviceMap singleton


io_connect_t CNTV2MacDriverInterface::GetIOConnect (const bool inDoNotAllocate) const
{
	return gDeviceMap.GetConnection (_boardNumber, inDoNotAllocate);
}
#endif	//	defined(USE_DEVICE_MAP)


#if defined(_DEBUG)
////////#define	AJA_MULTIRASTER_TEST
#endif

//--------------------------------------------------------------------------------------------------------------------
//	CNTV2MacDriverInterface
//
//	Constructor
//--------------------------------------------------------------------------------------------------------------------
CNTV2MacDriverInterface::CNTV2MacDriverInterface (void)
{
	mIsDEXT = false;
#if !defined(USE_DEVICE_MAP)
	mConnection = 0;
#endif
}


//--------------------------------------------------------------------------------------------------------------------
//	~CNTV2MacDriverInterface
//--------------------------------------------------------------------------------------------------------------------
CNTV2MacDriverInterface::~CNTV2MacDriverInterface (void)
{
	if (IsOpen())
		Close();
}

#if !defined(NTV2_NULL_DEVICE)
	//--------------------------------------------------------------------------------------------------------------------
	//	Open
	//--------------------------------------------------------------------------------------------------------------------
	bool CNTV2MacDriverInterface::OpenLocalPhysical (const UWord inDeviceIndex)
	{
	#if defined(USE_DEVICE_MAP)
		// Local host open -- get a Mach connection
		_boardOpened = gDeviceMap.GetConnection (inDeviceIndex) != 0;								
	
		// When device is unplugged, saved static io_connect_t value goes stale, yet remains non-zero.
		// This resets it it to zero, reestablishes a connection on replug. Fixes many pnp/sleep issues.
		if (IsOpen())
		{
			if (!gDeviceMap.ConnectionIsStillOkay(inDeviceIndex))
			{
				gDeviceMap.Reset();
				_boardOpened = gDeviceMap.GetConnection(inDeviceIndex) != 0;
			}
		}
	#else	//	!defined(USE_DEVICE_MAP)
		//	Make a new connection...
		io_iterator_t	ioIterator	(0);
		IOReturn		error		(kIOReturnSuccess);
		io_object_t		ioObject	(0);
		static const string kSvcNames[] = {sNTV2PCIDEXTName, sNTV2PCIKEXTClassName, ""};	//	Try DEXT first, then KEXT

		for (size_t svcNdx(0);  svcNdx < 2  &&  !mConnection;  svcNdx++)
		{
			const string &	svcName (kSvcNames[svcNdx]);
			const char *	pSvcName(svcName.c_str());
			const bool		tryKEXT	(svcName.find("com_aja_iokit") != string::npos);

			//	Create an iterator to search for our driver instances...
			error = OS_IOServiceGetMatchingServices (kIOMasterPortDefault,
													tryKEXT ? OS_IOServiceMatching(pSvcName) : OS_IOServiceNameMatching(pSvcName),
													&ioIterator);
			if (error != kIOReturnSuccess)
				{DIWARN(KR(error) << ": No '" << svcName << "' driver");  continue;}

			//	Find nth device -- and only use that one...
			for (UWord ndx(inDeviceIndex);  (ioObject = OS_IOIteratorNext(ioIterator));  OS_IOObjectRelease(ioObject))
				if (ndx == 0)
					break;	//	Found a match!
				else
					--ndx;

			if (ioIterator)
				OS_IOObjectRelease(ioIterator);
			if (!ioObject)
			{
				if (!inDeviceIndex)	//	Warn only if requesting first device, to show "no devices"
					DIWARN("No '" << svcName << "' devices");
				continue;	//	No service object
			}

			//	Found the device we want -- open it...
			error = OS_IOServiceOpen (ioObject, ::mach_task_self(), 0, &mConnection);
			OS_IOObjectRelease(ioObject);
			if (error != kIOReturnSuccess)
				{DIWARN(KR(error) << ": IOServiceOpen failed for '" << svcName << "' ndx=" << inDeviceIndex);  continue;}

			mIsDEXT = !tryKEXT;
		}	//	for each in kServiceNames

		_boardOpened = mConnection != 0;
		if (IsOpen())
			DIDBG((mIsDEXT ? "DEXT" : "KEXT") << " ndx=" << inDeviceIndex << " conn=" << HEX8(GetIOConnect()) << " opened");
	#endif	//	!defined(USE_DEVICE_MAP)
		if (!IsOpen())
			{DIFAIL(INSTP(this) << ": No connection: ndx=" << inDeviceIndex);  return false;}

		//	If USE_DEVICE_MAP defined, _boardNumber must be set before first ReadReg call, or it won't work:
		_boardNumber = inDeviceIndex;
		if (!CNTV2DriverInterface::ReadRegister (kRegBoardID, _boardID))
		{
			DIFAIL("ReadRegister(kRegBoardID) failed: ndx=" << inDeviceIndex << " con=" << HEX8(GetIOConnect()) << " boardID=" << HEX8(_boardID));
			Close();
			return false;
		}

		//	Good to go...
		DIDBG("Opened ndx=" << _boardNumber << " con=" << HEX8(GetIOConnect()) << " id=" << ::NTV2DeviceIDToString(_boardID));
		return true;

	}	//	OpenLocalPhysical


	bool CNTV2MacDriverInterface::CloseLocalPhysical (void)
	{
		NTV2_ASSERT(!IsRemote());
		DIDBG("Closed " << (mIsDEXT ? "DEXT" : "KEXT") << " ndx=" << _boardNumber << " con=" << HEX8(GetIOConnect()) << " id=" << ::NTV2DeviceIDToString(_boardID));
		_boardOpened = false;
		_boardNumber = 0;
	#if !defined(USE_DEVICE_MAP)
		if (mConnection)
			OS_IOServiceClose(mConnection);
		mConnection = 0;
	#endif	//	!defined(USE_DEVICE_MAP)
		mIsDEXT = false;
		return true;

	}	//	CloseLocalPhysical
#endif	//	!defined(NTV2_NULL_DEVICE)


#if !defined(NTV2_DEPRECATE_16_0)
	//--------------------------------------------------------------------------------------------------------------------
	//	GetPCISlotNumber
	//
	//	Returns my PCI slot number, if known; otherwise returns zero.
	//--------------------------------------------------------------------------------------------------------------------
	ULWord CNTV2MacDriverInterface::GetPCISlotNumber (void) const
	{
		//	TBD:	Determine where in the IORegistry the io_connect_t is, then navigate up to the io_registry_entry
		//			for our driver that contains the "AJAPCISlot" property. Then proceed as before...
		return 0;		//	FINISH THIS
	}	//	GetPCISlotNumber

	//--------------------------------------------------------------------------------------------------------------------
	//	MapFrameBuffers
	//
	//	Return a pointer and size of either the register map or frame buffer memory.
	//--------------------------------------------------------------------------------------------------------------------
	bool CNTV2MacDriverInterface::MapFrameBuffers (void)
	{
		UByte	*baseAddr;
		if (!MapMemory (kFrameBufferMemory, reinterpret_cast<void**>(&baseAddr)))
		{
			_pFrameBaseAddress = AJA_NULL;
			_pCh1FrameBaseAddress = _pCh2FrameBaseAddress = AJA_NULL;	//	DEPRECATE!
			return false;
		}
		_pFrameBaseAddress = reinterpret_cast<ULWord*>(baseAddr);
		return true;
	}

	bool CNTV2MacDriverInterface::UnmapFrameBuffers (void)
	{
		_pFrameBaseAddress = AJA_NULL;
		_pCh1FrameBaseAddress = _pCh2FrameBaseAddress = AJA_NULL;	//	DEPRECATE!
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------
	//	MapRegisters
	//--------------------------------------------------------------------------------------------------------------------
	bool CNTV2MacDriverInterface::MapRegisters (void)
	{
		ULWord	*baseAddr;
		if (!MapMemory (kRegisterMemory, reinterpret_cast<void**>(&baseAddr)))
		{
			_pRegisterBaseAddress = AJA_NULL;
			return false;
		}
		_pRegisterBaseAddress = reinterpret_cast<ULWord*>(baseAddr);
		return true;
	}

	bool CNTV2MacDriverInterface::UnmapRegisters (void)
	{
		_pRegisterBaseAddress = AJA_NULL;
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------
	//	Map / Unmap Xena2Flash
	//--------------------------------------------------------------------------------------------------------------------
	bool CNTV2MacDriverInterface::MapXena2Flash (void)
	{
		ULWord	*baseAddr;
		if (!MapMemory (kXena2FlashMemory, reinterpret_cast<void**>(&baseAddr)))
		{
			_pXena2FlashBaseAddress = AJA_NULL;
			return false;
		}
		_pXena2FlashBaseAddress = reinterpret_cast<ULWord*>(baseAddr);
		return true;
	}

	bool CNTV2MacDriverInterface::UnmapXena2Flash (void)
	{
		_pXena2FlashBaseAddress = AJA_NULL;
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------
	//	MapMemory
	//--------------------------------------------------------------------------------------------------------------------
	bool CNTV2MacDriverInterface::MapMemory (const MemoryType memType, void **memPtr)
	{
#ifndef __LP64__
		return false;
#endif
		if (GetIOConnect())
		{
			mach_vm_size_t	size(0);
			OS_IOConnectMapMemory (	GetIOConnect(), memType, mach_task_self(),
									reinterpret_cast<mach_vm_address_t*>(memPtr),
									&size, kIOMapDefaultCache | kIOMapAnywhere);
			return size > 0;
		}
		return false;
	}

	//--------------------------------------------------------------------------------------------------------------------
	//	SystemControl
	//--------------------------------------------------------------------------------------------------------------------
	bool CNTV2MacDriverInterface::SystemControl (void * dataPtr, SystemControlCode controlCode)
	{
		kern_return_t	kernResult		= KERN_FAILURE;
		uint64_t		scalarI_64[2]	= {uint64_t(dataPtr), controlCode};
		uint32_t		outputCount		= 0;
		if (controlCode != SCC_Test)
			kernResult = KERN_INVALID_ARGUMENT;
		else if (GetIOConnect())
			kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
														kDriverSystemControl,	// selector of the function to be called via the user client.
														scalarI_64,				// array of scalar (64-bit) input values.
														2,						// the number of scalar input values.
														AJA_NULL,				// array of scalar (64-bit) output values.
														&outputCount);			// pointer to the number of scalar output values.
		if (kernResult == KERN_SUCCESS)
			return true;
		DIFAIL (KR(kernResult) << ", con=" << HEX8(GetIOConnect()));
		return false;
	}
#endif	//	!defined(NTV2_DEPRECATE_16_0)

#pragma mark - New Driver Calls
#if defined(AJA_MULTIRASTER_TEST)
	//							kRegMRQ1Control	kRegMRQ2Control	kRegMRQ3Control	kRegMRQ4Control	kRegMROutControl							kRegMRSupport
	static ULWord gMRRegs[] = {	0x01000004,		0x01000004,		0x01000004,		0x01000004,		0x01101D48,			0x00444400,	0x00000000,	0x00000001};
#endif

//--------------------------------------------------------------------------------------------------------------------
//	ReadRegister
//
//	Return the value of specified register after masking and shifting the value.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::ReadRegister (const ULWord inRegNum, ULWord & outValue, const ULWord inMask, const ULWord inShift)
{
	if (inShift >= 32)
	{
		DIFAIL("Shift " << DEC(inShift) << " > 31, reg=" << DEC(inRegNum) << " msk=" << xHEX0N(inMask,8));
		return false;
	}
#if defined (NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return CNTV2DriverInterface::ReadRegister(inRegNum, outValue, inMask, inShift);
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
#if defined(AJA_MULTIRASTER_TEST)
	if (inRegNum >= kRegMRQ1Control  &&  inRegNum <= kRegMRSupport)
		{outValue = (gMRRegs[inRegNum - kRegMRQ1Control] & inMask) >> inShift;	return true;}
#endif
	kern_return_t kernResult(KERN_FAILURE);
	uint64_t	scalarI_64[2] = {inRegNum, inMask};
	uint64_t	scalarO_64 = outValue;
	uint32_t	outputCount = 1;
	if (GetIOConnect())
	{
		AJADebug::StatTimerStart(AJA_DebugStat_ReadRegister);
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverReadRegister,	// selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													2,						// the number of scalar input values.
													&scalarO_64,			// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
		AJADebug::StatTimerStop(AJA_DebugStat_ReadRegister);
	}
	outValue = uint32_t(scalarO_64);
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL(KR(kernResult) << ": ndx=" << _boardNumber << ", con=" << HEX8(GetIOConnect())
			<< " -- reg=" << DEC(inRegNum) << ", mask=" << HEX8(inMask) << ", shift=" << HEX8(inShift));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	WriteRegister
//
//	Set the specified register value taking into accout the bit mask.
//	If the bit mask is not 0xFFFFFFFF (default) or 0, then this does a read-modify-write.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::WriteRegister (const ULWord inRegNum, const ULWord inValue, const ULWord inMask, const ULWord inShift)
{
	if (inShift >= 32)
	{
		DIFAIL("Shift " << DEC(inShift) << " > 31, reg=" << DEC(inRegNum) << " msk=" << xHEX0N(inMask,8));
		return false;
	}
#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	if (mRecordRegWrites)
	{
		AJAAutoLock autoLock(&mRegWritesLock);
		mRegWrites.push_back(NTV2RegInfo(inRegNum, inValue, inMask, inShift));
		if (mSkipRegWrites)
			return true;
	}
#endif	//	defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return CNTV2DriverInterface::WriteRegister(inRegNum, inValue, inMask, inShift);
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
#if defined(AJA_MULTIRASTER_TEST)
	if (inRegNum >= kRegMRQ1Control  &&  inRegNum <= kRegMRSupport)
		{gMRRegs[inRegNum - kRegMRQ1Control] = ((inValue << inShift) & inMask) | ((~inMask) & gMRRegs[inRegNum - kRegMRQ1Control]);	return true;}
#endif
	kern_return_t kernResult(KERN_FAILURE);
	uint64_t	scalarI_64[3] = {inRegNum, inValue, inMask};
	uint32_t	outputCount = 0;
	if (GetIOConnect())
	{
		AJADebug::StatTimerStart(AJA_DebugStat_WriteRegister);
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverWriteRegister,	// selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													3,						// the number of scalar input values.
													AJA_NULL,				// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
		AJADebug::StatTimerStop(AJA_DebugStat_WriteRegister);
	}
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()) << " -- reg=" << inRegNum
			<< ", val=" << HEX8(inValue) << ", mask=" << HEX8(inMask) << ", shift=" << HEX8(inShift));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	AcquireStreamForApplication
//
//	Aquire board by by waiting for current board to release its resources
//
//	Note: When quicktime is using the board, desktop output on the board is disabled
//	by the driver.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::AcquireStreamForApplication (ULWord appType, int32_t pid)
{
	kern_return_t kernResult = KERN_FAILURE;
	uint64_t	scalarI_64[2] = {uint64_t(appType), uint64_t(pid)};
	uint32_t	outputCount = 0;
	if (GetIOConnect())
	{
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverAcquireStreamForApplication,	// selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													2,						// the number of scalar input values.
													AJA_NULL,				// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
	}
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL(KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	ReleaseStreamForApplication
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::ReleaseStreamForApplication (ULWord appType, int32_t pid)
{
	kern_return_t kernResult = KERN_FAILURE;
	uint64_t	scalarI_64[2] = {uint64_t(appType), uint64_t(pid)};
	uint32_t	outputCount = 0;
	if (GetIOConnect())
	{
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverReleaseStreamForApplication,	// selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													2,						// the number of scalar input values.
													AJA_NULL,				// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
	}
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	AcquireStreamForApplicationWithReference
//
//	Do a reference counted acquire
//	Use this call ONLY with ReleaseStreamForApplicationWithReference
//	Aquire board by by waiting for current board to release its resources
//
//	Note: When quicktime is using the board, desktop output on the board is disabled
//	by the driver.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::AcquireStreamForApplicationWithReference (ULWord appType, int32_t pid)
{
	kern_return_t kernResult = KERN_FAILURE;
	uint64_t	scalarI_64[2] = {uint64_t(appType), uint64_t(pid)};
	uint32_t	outputCount = 0;
	if (GetIOConnect())
	{
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverAcquireStreamForApplicationWithReference, // selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													2,						// the number of scalar input values.
													AJA_NULL,				// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
	}
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	ReleaseStreamForApplicationWithReference
//
//	Do a reference counted release
//	Use this call ONLY with AcquireStreamForApplicationWithReference
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::ReleaseStreamForApplicationWithReference (ULWord appType, int32_t pid)
{
	kern_return_t kernResult = KERN_FAILURE;
	uint64_t	scalarI_64[2] = {uint64_t(appType), uint64_t(pid)};
	uint32_t	outputCount = 0;
	if (GetIOConnect())
	{
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverReleaseStreamForApplicationWithReference, // selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													2,						// the number of scalar input values.
													AJA_NULL,				// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
	}
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
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
bool CNTV2MacDriverInterface::SetStreamingApplication (ULWord appType, int32_t pid)
{
	kern_return_t kernResult = KERN_FAILURE;
	uint64_t	scalarI_64[2] = {uint64_t(appType), uint64_t(pid)};
	uint32_t	outputCount = 0;
	if (GetIOConnect())
	{
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverSetStreamForApplication,	// selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													2,						// the number of scalar input values.
													AJA_NULL,				// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
	}
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	GetStreamingApplication
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::GetStreamingApplication (ULWord & outAppType, int32_t & outProcessID)
{
	kern_return_t kernResult = KERN_FAILURE;
	uint64_t	scalarO_64[2]	= {0, 0};
	uint32_t	outputCount(2);
	if (GetIOConnect())
	{
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverGetStreamForApplication,	// selector of the function to be called via the user client.
													AJA_NULL,				// array of scalar (64-bit) input values.
													0,						// the number of scalar input values.
													scalarO_64,				// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
	}
	outAppType	 = ULWord(scalarO_64[0]);
	outProcessID = int32_t(scalarO_64[1]);
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	KernelLog
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::KernelLog (void * dataPtr, UInt32 dataSize)
{
	kern_return_t kernResult	= KERN_FAILURE;
	uint64_t	scalarI_64[2]	= {uint64_t(dataPtr), dataSize};
	uint32_t	outputCount		= 0;
	if (GetIOConnect())
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),		// an io_connect_t returned from IOServiceOpen().
													kDriverKernelLog,	// selector of the function to be called via the user client.
													scalarI_64,			// array of scalar (64-bit) input values.
													2,					// the number of scalar input values.
													AJA_NULL,			// array of scalar (64-bit) output values.
													&outputCount);		// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}


static const uint32_t sIntEnumToStatKeys[] = {	AJA_DebugStat_WaitForInterruptOut1,		//	eOutput1	//	0
												AJA_DebugStat_WaitForInterruptOthers,					//	1
												AJA_DebugStat_WaitForInterruptIn1,		//	eInput1		//	2
												AJA_DebugStat_WaitForInterruptIn2,		//	eInput2		//	3
												AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers,	//	4 thru 13
												AJA_DebugStat_WaitForInterruptUartTx1,	//	eUart1Tx	//	14
												AJA_DebugStat_WaitForInterruptUartRx1,	//	eUart1Rx	//	15
												AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers,	//	16 thru 23
												AJA_DebugStat_WaitForInterruptIn3,		//	eInput3		//	24
												AJA_DebugStat_WaitForInterruptIn4,		//	eInput4		//	25
												AJA_DebugStat_WaitForInterruptUartTx2,	//	eUartTx2	//	26
												AJA_DebugStat_WaitForInterruptUartRx2,	//	eUartRx2	//	27
												AJA_DebugStat_WaitForInterruptOthers,					//	28
												AJA_DebugStat_WaitForInterruptIn5,		//	eInput5		//	29
												AJA_DebugStat_WaitForInterruptIn6,		//	eInput6		//	30
												AJA_DebugStat_WaitForInterruptIn7,		//	eInput7		//	31
												AJA_DebugStat_WaitForInterruptIn8,		//	eInput8		//	32
												AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers, AJA_DebugStat_WaitForInterruptOthers};	//	33 thru 41

//--------------------------------------------------------------------------------------------------------------------
//	WaitForInterrupt
//
//	Block the current thread until the specified interrupt occurs.
//	Supply a timeout in milliseconds - if 0 (default), then never time out.
//	Returns true if interrupt occurs, false if timeout.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::WaitForInterrupt (const INTERRUPT_ENUMS type, const ULWord timeout)
{
	if (IsRemote())
		return CNTV2DriverInterface::WaitForInterrupt(type, timeout);
	if (type == eChangeEvent)
		return WaitForChangeEvent(timeout);

	kern_return_t	kernResult	= KERN_FAILURE;
	uint64_t	scalarI_64[2]	= {type, timeout};
	uint64_t	scalarO_64		= 0;
	uint32_t	outputCount		= 1;

	if (!NTV2_IS_VALID_INTERRUPT_ENUM(type))
		kernResult = KERN_INVALID_VALUE;
	else if (GetIOConnect())
	{
		AJADebug::StatTimerStart(sIntEnumToStatKeys[type]);
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverWaitForInterrupt,// selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													2,						// the number of scalar input values.
													&scalarO_64,			// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
		AJADebug::StatTimerStop(sIntEnumToStatKeys[type]);
	}
	UInt32 interruptOccurred = uint32_t(scalarO_64);
	if (kernResult != KERN_SUCCESS)
		{DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));  return false;}
	if (interruptOccurred)
		BumpEventCount(type);
	return interruptOccurred;
}

//--------------------------------------------------------------------------------------------------------------------
//	GetInterruptCount
//
//	Returns the number of interrupts that have occured for the specified interrupt type.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::GetInterruptCount (const INTERRUPT_ENUMS eInterrupt, ULWord & outCount)
{
	kern_return_t	kernResult	= KERN_FAILURE;
	uint64_t	scalarI_64[1]	= {eInterrupt};
	uint64_t	scalarO_64		= 0;
	uint32_t	outputCount		= 1;
	if (GetIOConnect())
	{
		AJADebug::StatTimerStart(AJA_DebugStat_GetInterruptCount);
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),				// an io_connect_t returned from IOServiceOpen().
													kDriverGetInterruptCount,	// selector of the function to be called via the user client.
													scalarI_64,					// array of scalar (64-bit) input values.
													1,							// the number of scalar input values.
													&scalarO_64,				// array of scalar (64-bit) output values.
													&outputCount);				// pointer to the number of scalar output values.
		AJADebug::StatTimerStop(AJA_DebugStat_GetInterruptCount);
	}
	outCount = ULWord(scalarO_64);
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL(KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	WaitForChangeEvent
//
//	Block the current thread until a register changes.
//	Supply a timeout in milliseconds - if 0 (default), then never time out.
//	Returns true if change occurs, false if timeout.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::WaitForChangeEvent (UInt32 timeout)
{
	kern_return_t	kernResult	= KERN_FAILURE;
	uint64_t	scalarI_64[1]	= {timeout};
	uint64_t	scalarO_64		= 0;
	uint32_t	outputCount		= 1;
	if (GetIOConnect())
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),				// an io_connect_t returned from IOServiceOpen().
													kDriverWaitForChangeEvent,	// selector of the function to be called via the user client.
													scalarI_64,					// array of scalar (64-bit) input values.
													1,							// the number of scalar input values.
													&scalarO_64,				// array of scalar (64-bit) output values.
													&outputCount);				// pointer to the number of scalar output values.
	if (kernResult != KERN_SUCCESS)
		DIFAIL(KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return bool(scalarO_64);
}


//--------------------------------------------------------------------------------------------------------------------
//	DmaTransfer
//
//	Start a memory transfer using the specified DMA engine.
//	Optional - call PrepareDMAMemory on the dataPtr before the first use of memory block
//	for DMA and CompleteDMAMemory when done.  This will speed up the DMA by not requiring
//	memory to be prepared for each DMA.	 Otherwise, it takes about 2 to 5 ms (sometimes
//	much more) for the memory block to be prepared.
//	This function will sleep (block) until the DMA finishes.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::DmaTransfer ( const NTV2DMAEngine inDMAEngine,
											const bool			inIsRead,
											const ULWord		inFrameNumber,
											ULWord *			pFrameBuffer,
											const ULWord		inOffsetBytes,
											const ULWord		inByteCount,
											const bool			inSynchronous)
{
	if (IsRemote())
		return CNTV2DriverInterface::DmaTransfer(inDMAEngine, inIsRead, inFrameNumber, pFrameBuffer,
												inOffsetBytes, inByteCount, inSynchronous);
	if (!IsOpen())
		return false;
	kern_return_t kernResult = KERN_FAILURE;
	uint64_t	scalarI_64[6] = {inDMAEngine, uint64_t(pFrameBuffer), inFrameNumber, inOffsetBytes, inByteCount, !inIsRead};
	uint32_t	outputCount = 0;
	if (GetIOConnect())
	{
		AJADebug::StatTimerStart(AJA_DebugStat_DMATransfer);
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),		// an io_connect_t returned from IOServiceOpen().
													kDriverDMATransfer,	// selector of the function to be called via the user client.
													scalarI_64,			// array of scalar (64-bit) input values.
													6,					// the number of scalar input values.
													AJA_NULL,			// array of scalar (64-bit) output values.
													&outputCount);		// pointer to the number of scalar output values.
		AJADebug::StatTimerStop(AJA_DebugStat_DMATransfer);
	}
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL(KR(kernResult) << ": con=" << HEX8(GetIOConnect()) << ", eng=" << inDMAEngine << ", frm=" << inFrameNumber
			<< ", off=" << HEX8(inOffsetBytes) << ", len=" << HEX8(inByteCount) << ", " << (inIsRead ? "R" : "W"));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	DmaTransfer
//
//	Start a memory transfer using the specified DMA engine.
//	Optional - call PrepareDMAMemory on the dataPtr before the first use of memory block
//	for DMA and CompleteDMAMemory when done.  This will speed up the DMA by not requiring
//	memory to be prepared for each DMA.	 Otherwise, it takes about 2 to 5 ms (sometimes
//	much more) for the memory block to be prepared.
//	This function will sleep (block) until the DMA finishes.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::DmaTransfer ( const NTV2DMAEngine inDMAEngine,
											const bool inIsRead,
											const ULWord inFrameNumber,
											ULWord * pFrameBuffer,
											const ULWord inCardOffsetBytes,
											const ULWord inByteCount,
											const ULWord inNumSegments,
											const ULWord inSegmentHostPitch,
											const ULWord inSegmentCardPitch,
											const bool inSynchronous)
{
	if (IsRemote())
		return CNTV2DriverInterface::DmaTransfer (inDMAEngine, inIsRead, inFrameNumber, pFrameBuffer, inCardOffsetBytes, inByteCount,
													inNumSegments, inSegmentHostPitch, inSegmentCardPitch, inSynchronous);
	if (!IsOpen())
		return false;
	kern_return_t kernResult = KERN_FAILURE;
	size_t	outputStructSize = 0;

	DMA_TRANSFER_STRUCT_64 dmaTransfer64;
	dmaTransfer64.dmaEngine				= inDMAEngine;
	dmaTransfer64.dmaFlags				= 0;
	dmaTransfer64.dmaHostBuffer			= Pointer64(pFrameBuffer);			// virtual address of host buffer
	dmaTransfer64.dmaSize				= inByteCount;						// total number of bytes to DMA
	dmaTransfer64.dmaCardFrameNumber	= inFrameNumber;					// card frame number
	dmaTransfer64.dmaCardFrameOffset	= inCardOffsetBytes;				// offset (in bytes) into card frame to begin DMA
	dmaTransfer64.dmaNumberOfSegments	= inNumSegments;					// number of segments of size videoBufferSize to DMA
	dmaTransfer64.dmaSegmentSize		= (inByteCount / inNumSegments);	// size of each segment (if videoNumSegments > 1)
	dmaTransfer64.dmaSegmentHostPitch	= inSegmentHostPitch;				// offset between the beginning of one host-memory segment and the next host-memory segment
	dmaTransfer64.dmaSegmentCardPitch	= inSegmentCardPitch;				// offset between the beginning of one Kona-memory segment and the next Kona-memory segment
	dmaTransfer64.dmaToCard				= !inIsRead;						// direction of DMA transfer

	if (GetIOConnect())
	{
		AJADebug::StatTimerStart(AJA_DebugStat_DMATransferEx);
		kernResult = OS_IOConnectCallStructMethod (GetIOConnect(),					// an io_connect_t returned from IOServiceOpen().
												   kDriverDMATransferEx,			// selector of the function to be called via the user client.
												   &dmaTransfer64,					// pointer to the input structure
												   sizeof(DMA_TRANSFER_STRUCT_64),	// size of input structure
												   AJA_NULL,						// pointer to the output structure
												   &outputStructSize);				// size of output structure
		AJADebug::StatTimerStop(AJA_DebugStat_DMATransferEx);
	}
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}


bool CNTV2MacDriverInterface::DmaTransfer ( const NTV2DMAEngine			inDMAEngine,
											const NTV2Channel			inDMAChannel,
											const bool					inIsTarget,
											const ULWord				inFrameNumber,
											const ULWord				inCardOffsetBytes,
											const ULWord				inByteCount,
											const ULWord				inNumSegments,
											const ULWord				inSegmentHostPitch,
											const ULWord				inSegmentCardPitch,
											const PCHANNEL_P2P_STRUCT & inP2PData)
{
	if (IsRemote())
		return CNTV2DriverInterface::DmaTransfer (inDMAEngine, inDMAChannel, inIsTarget, inFrameNumber, inCardOffsetBytes, inByteCount,
													inNumSegments, inSegmentHostPitch, inSegmentCardPitch, inP2PData);
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	RestoreHardwareProcampRegisters
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::RestoreHardwareProcampRegisters (void)
{
	kern_return_t	kernResult	= KERN_FAILURE;
	uint32_t		outputCount = 0;
	if (GetIOConnect())
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),					// an io_connect_t returned from IOServiceOpen().
													kDriverRestoreProcAmpRegisters,	// selector of the function to be called via the user client.
													AJA_NULL,						// array of scalar (64-bit) input values.
													0,								// the number of scalar input values.
													AJA_NULL,						// array of scalar (64-bit) output values.
													&outputCount);					// pointer to the number of scalar output values.
	if (kernResult == KERN_SUCCESS)
		return true;
	DIFAIL (KR(kernResult) << ": con=" << HEX8(GetIOConnect()));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
//	SystemStatus
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::SystemStatus ( void* dataPtr, SystemStatusCode statusCode)
{
	kern_return_t	kernResult		= KERN_FAILURE;
	uint64_t		scalarI_64[2]	= {uint64_t(dataPtr), statusCode};
	uint32_t		outputCount		= 0;
	if (GetIOConnect())
		kernResult = OS_IOConnectCallScalarMethod (	GetIOConnect(),			// an io_connect_t returned from IOServiceOpen().
													kDriverSystemStatus,	// selector of the function to be called via the user client.
													scalarI_64,				// array of scalar (64-bit) input values.
													2,						// the number of scalar input values.
													AJA_NULL,				// array of scalar (64-bit) output values.
													&outputCount);			// pointer to the number of scalar output values.
	if (statusCode != SSC_GetFirmwareProgress)
		return false;
	if (kernResult == KERN_SUCCESS)
		return true;
	MDIFAIL (KR(kernResult) << INSTP(this) << ", con=" << HEX8(GetIOConnect()));
	return false;
}


//--------------------------------------------------------------------------------------------------------------------
// AutoCirculate
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2MacDriverInterface::AutoCirculate (AUTOCIRCULATE_DATA & autoCircData)
{
	bool success = true;
	if (IsRemote())
		return CNTV2DriverInterface::AutoCirculate(autoCircData);

	kern_return_t	kernResult = KERN_FAILURE;
	io_connect_t	conn(GetIOConnect());
	if (!conn)
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
		case eStartAutoCircAtTime:
		{
			// Pass the autoCircData structure to the driver. The driver knows the implicit meanings of the
			// members of the structure based on the the command contained within it.
			size_t	outputStructSize = 0;
			AUTOCIRCULATE_DATA_64 autoCircData64;
			CopyTo_AUTOCIRCULATE_DATA_64 (&autoCircData, &autoCircData64);

			AJADebug::StatTimerStart(AJA_DebugStat_AutoCirculate);
			kernResult = OS_IOConnectCallStructMethod (	conn,							// an io_connect_t returned from IOServiceOpen().
														kDriverAutoCirculateControl,	// selector of the function to be called via the user client.
														&autoCircData64,				// pointer to the input structure
														sizeof(AUTOCIRCULATE_DATA_64),	// size of input structure
														AJA_NULL,						// pointer to the output structure
														&outputStructSize);				// size of output structure
			AJADebug::StatTimerStop(AJA_DebugStat_AutoCirculate);
			break;
		}	//	eInit, eStart, eStop, eAbort, etc...

		case eGetAutoCirc:
		{
			uint64_t	scalarI_64[1];
			uint32_t	outputCount = 0;
			size_t		outputStructSize = sizeof(AUTOCIRCULATE_STATUS_STRUCT);
			scalarI_64[0] = autoCircData.channelSpec;
			AJADebug::StatTimerStart(AJA_DebugStat_AutoCirculate);
			kernResult = OS_IOConnectCallMethod (	conn,						// an io_connect_t returned from IOServiceOpen().
													kDriverAutoCirculateStatus,	// selector of the function to be called via the user client.
													scalarI_64,					// array of scalar (64-bit) input values.
													1,							// the number of scalar input values.
													AJA_NULL,					// pointer to the input structure
													0,							// size of input structure
													AJA_NULL,					// array of scalar (64-bit) output values.
													&outputCount,				// the number of scalar output values.
													autoCircData.pvVal1,		// pointer to the output structure
													&outputStructSize);			// size of output structure
			AJADebug::StatTimerStop(AJA_DebugStat_AutoCirculate);
			break;
		}	//	eGetAutoCirc

		case eGetFrameStamp:
		case eGetFrameStampEx2:
		{
			// Make sure task structure does not get passed in with eGetFrameStamp call.
			if ( autoCircData.eCommand == eGetFrameStamp)
				autoCircData.pvVal2 = AJA_NULL;

			size_t	outputStructSize = sizeof(AUTOCIRCULATE_DATA_64);

			// promote base data structure
			AUTOCIRCULATE_DATA_64 autoCircData64;
			CopyTo_AUTOCIRCULATE_DATA_64 (&autoCircData, &autoCircData64);

			AJADebug::StatTimerStart(AJA_DebugStat_AutoCirculate);
			kernResult = OS_IOConnectCallStructMethod (	conn,							// an io_connect_t returned from IOServiceOpen().
														kDriverAutoCirculateFramestamp,	// selector of the function to be called via the user client.
														&autoCircData64,				// pointer to the input structure
														sizeof(AUTOCIRCULATE_DATA_64),	// size of input structure
														&autoCircData64,				// pointer to the output structure
														&outputStructSize);				// size of output structure
			AJADebug::StatTimerStop(AJA_DebugStat_AutoCirculate);
			break;
		}	//	eGetFrameStamp, eGetFrameStampEx2

		case eTransferAutoCirculate:
		case eTransferAutoCirculateEx:
		case eTransferAutoCirculateEx2:
		{
			// Pass the autoCircData structure to the driver. The driver knows the implicit meanings of the
			// members of the structure based on the the command contained within it.
			// Make sure routing table and task structure does not get passed in with eTransferAutoCirculate call.
			if (autoCircData.eCommand == eTransferAutoCirculate)
			{
				autoCircData.pvVal3 = AJA_NULL;
				autoCircData.pvVal4 = AJA_NULL;
			}

			// Make sure task structure does not get passed in with eTransferAutoCirculateEx call.
			if (autoCircData.eCommand == eTransferAutoCirculateEx)
				autoCircData.pvVal4 = AJA_NULL;

			size_t	outputStructSize = sizeof(AUTOCIRCULATE_TRANSFER_STATUS_STRUCT);

			// promote base data structure
			AUTOCIRCULATE_DATA_64 autoCircData64;
			CopyTo_AUTOCIRCULATE_DATA_64 (&autoCircData, &autoCircData64);

			// promote AUTOCIRCULATE_TRANSFER_STRUCT
			AUTOCIRCULATE_TRANSFER_STRUCT_64 autoCircTransfer64;
			CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64 (reinterpret_cast<AUTOCIRCULATE_TRANSFER_STRUCT*>(autoCircData.pvVal1), &autoCircTransfer64);
			autoCircData64.pvVal1 = Pointer64(&autoCircTransfer64);

			AUTOCIRCULATE_TASK_STRUCT_64 autoCircTask64;
			if (autoCircData.pvVal4 != AJA_NULL)
			{
				CopyTo_AUTOCIRCULATE_TASK_STRUCT_64 (reinterpret_cast<AUTOCIRCULATE_TASK_STRUCT*>(autoCircData.pvVal4), &autoCircTask64);
				autoCircData64.pvVal4 = Pointer64(&autoCircTask64);
			}

			AJADebug::StatTimerStart(AJA_DebugStat_AutoCirculateXfer);
			kernResult = OS_IOConnectCallStructMethod (	conn,							// an io_connect_t returned from IOServiceOpen().
														kDriverAutoCirculateTransfer,	// selector of the function to be called via the user client.
														&autoCircData64,				// pointer to the input structure
														sizeof(AUTOCIRCULATE_DATA_64),	// size of input structure
														autoCircData.pvVal2,			// pointer to the output structure
														&outputStructSize);				// size of output structure
			AJADebug::StatTimerStop(AJA_DebugStat_AutoCirculateXfer);
			break;
		}	//	eTransferAutoCirculate, eTransferAutoCirculateEx, eTransferAutoCirculateEx2

		default:
			//DisplayNTV2Error("Unsupported AC command type in AutoCirculate()\n");
			kernResult =  KERN_INVALID_ARGUMENT;
			break;
	}	//	switch

	success = (kernResult == KERN_SUCCESS);
	if (kernResult != KERN_SUCCESS && kernResult != kIOReturnOffline)
		MDIFAIL (KR(kernResult) << INSTP(this) << ", con=" << HEX8(conn) << ", eCmd=" << autoCircData.eCommand);
	return success;
}	//	AutoCirculate


bool CNTV2MacDriverInterface::NTV2Message (NTV2_HEADER * pInOutMessage)
{
	if (!pInOutMessage)
		return false;
	if (!pInOutMessage->IsValid())
		return false;
	if (!pInOutMessage->GetSizeInBytes())
		return false;
	if (IsRemote())
		return CNTV2DriverInterface::NTV2Message (pInOutMessage);

	//	Force fOperation = 0 in SDK 16.3, to allow RPCs from 16.3 or later clients to work on servers running 16.2 or earlier drivers:
	ULWord* pU32 = reinterpret_cast<ULWord*>(pInOutMessage); pU32[6] = 0;

	kern_return_t	kernResult	(KERN_FAILURE);
	io_connect_t	connection	(GetIOConnect ());
	uint64_t		scalarI_64 [2]	= {uint64_t(pInOutMessage), pInOutMessage->GetSizeInBytes()};
	uint32_t		numScalarOutputs(0);
	if (connection)
	{
		AJADebug::StatTimerStart(AJA_DebugStat_NTV2Message);
		kernResult = OS_IOConnectCallScalarMethod (	connection,			//	an io_connect_t returned from IOServiceOpen
													kDriverNTV2Message,	//	selector of the function to be called via the user client
													scalarI_64,			//	array of scalar (64-bit) input values
													2,					//	the number of scalar input values
													AJA_NULL,			//	array of scalar (64-bit) output values
													&numScalarOutputs);	//	pointer (in: number of scalar output values capable of receiving;  out: actual number of scalar output values)
		AJADebug::StatTimerStop(AJA_DebugStat_NTV2Message);
	}
	if (kernResult != KERN_SUCCESS	&&	kernResult != kIOReturnOffline)
		MDIFAIL (KR(kernResult) << INSTP(this) << ", con=" << HEX8(connection) << endl << *pInOutMessage);
	return kernResult == KERN_SUCCESS;

}	//	NTV2Message



#pragma mark Old Driver Calls

bool CNTV2MacDriverInterface::SetAudioOutputMode (NTV2_GlobalAudioPlaybackMode mode)
{
	return WriteRegister(kVRegGlobalAudioPlaybackMode,mode);
}

bool CNTV2MacDriverInterface::GetAudioOutputMode (NTV2_GlobalAudioPlaybackMode* mode)
{
	return mode ? CNTV2DriverInterface::ReadRegister(kVRegGlobalAudioPlaybackMode, *mode) : false;
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
	p64->lVal5			= p->lVal5;
	p64->lVal6			= p->lVal6;

	p64->bVal1			= p->bVal1;
	p64->bVal2			= p->bVal2;
	p64->bVal3			= p->bVal3;
	p64->bVal4			= p->bVal4;
	p64->bVal5			= p->bVal5;
	p64->bVal6			= p->bVal6;
	p64->bVal7			= p->bVal7;
	p64->bVal8			= p->bVal8;

	p64->pvVal1			= Pointer64(p->pvVal1); // native to 64 bit
	p64->pvVal2			= Pointer64(p->pvVal2); // native to 64 bit
	p64->pvVal3			= Pointer64(p->pvVal3); // native to 64 bit
	p64->pvVal4			= Pointer64(p->pvVal4); // native to 64 bit
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
	p->lVal5			= p64->lVal5;
	p->lVal6			= p64->lVal6;

	p->bVal1			= p64->bVal1;
	p->bVal2			= p64->bVal2;
	p->bVal3			= p64->bVal3;
	p->bVal4			= p64->bVal4;
	p->bVal5			= p64->bVal5;
	p->bVal6			= p64->bVal6;
	p->bVal7			= p64->bVal7;
	p->bVal8			= p64->bVal8;

	p->pvVal1			= reinterpret_cast<void*>(p64->pvVal1); // 64 bit to native
	p->pvVal2			= reinterpret_cast<void*>(p64->pvVal2); // 64 bit to native
	p->pvVal3			= reinterpret_cast<void*>(p64->pvVal3); // 64 bit to native
	p->pvVal4			= reinterpret_cast<void*>(p64->pvVal4); // 64 bit to native
}


//-------------------------------------------------------------------------------------------------------
//	CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64
//-------------------------------------------------------------------------------------------------------
void CNTV2MacDriverInterface::CopyTo_AUTOCIRCULATE_TRANSFER_STRUCT_64 (AUTOCIRCULATE_TRANSFER_STRUCT *p, AUTOCIRCULATE_TRANSFER_STRUCT_64 *p64)
{
	// note that p is native structure, either 64 or 32 bit
	p64->channelSpec				= p->channelSpec;
	p64->videoBuffer				= Pointer64(p->videoBuffer);	// native to 64 bit
	p64->videoBufferSize			= p->videoBufferSize;
	p64->videoDmaOffset				= p->videoDmaOffset;
	p64->audioBuffer				= Pointer64(p->audioBuffer);	// native to 64 bit
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
	p64->colorCorrectionInfo.ccLookupTables		= Pointer64(p->colorCorrectionInfo.ccLookupTables);

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
	p->videoBuffer					= reinterpret_cast<ULWord*>(p64->videoBuffer);	// 64 bit to native
	p->videoBufferSize				= p64->videoBufferSize;
	p->videoDmaOffset				= p64->videoDmaOffset;
	p->audioBuffer					= reinterpret_cast<ULWord*>(p64->audioBuffer);	// 64 bit to native
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
	p->colorCorrectionInfo.ccLookupTables		= reinterpret_cast<ULWord*>(p64->colorCorrectionInfo.ccLookupTables);

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
	p64->taskArray				= Pointer64(p->taskArray);
	p64->reserved0				= p->reserved0;
	p64->reserved1				= p->reserved1;
	p64->reserved2				= p->reserved2;
	p64->reserved3				= p->reserved3;
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
		case kIOReturnUnformattedMedia: return "media not formatted";
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
		case MACH_SEND_INVALID_DEST:	return "MACH_SEND_INVALID_DEST";
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
