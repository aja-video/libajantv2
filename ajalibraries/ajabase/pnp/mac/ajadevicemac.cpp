/**
	@file		pnp/mac/ajadevicemac.cpp
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJADeviceMac class.
**/

#include "ajadevicemac.h"
#include "masterport.h"
#include <map>
#include <sstream>
#include <IOKit/IOMessage.h>

using namespace std;

//
//	The private interface into the ActiveDeviceMap...
//
extern	AJADevicePtr	ActiveDeviceMap_Find	(AJADevice * pInDevice);
extern	bool			ActiveDeviceMap_Remove	(AJADevice * pInDevice);
extern	bool			ActiveDeviceMap_Add		(AJADevicePtr inNewDevice);


	typedef	map<kern_return_t, string>	KernRetStrMap;
	typedef KernRetStrMap::const_iterator			KernRetStrMapConstIter;
	static const char * GetKernErrStr (const kern_return_t inError)
	{
		static KernRetStrMap	sMap;
		if (sMap.empty ())
		{
			sMap [kIOReturnError] = "general error";
			sMap [kIOReturnNoMemory] = "can't allocate memory";
			sMap [kIOReturnNoResources] = "resource shortage";
			sMap [kIOReturnIPCError] = "error during IPC";
			sMap [kIOReturnNoDevice] = "no such device";
			sMap [kIOReturnNotPrivileged] = "privilege violation";
			sMap [kIOReturnBadArgument] = "invalid argument";
			sMap [kIOReturnLockedRead] = "device read locked";
			sMap [kIOReturnLockedWrite] = "device write locked";
			sMap [kIOReturnExclusiveAccess] = "exclusive access and device already open";
			sMap [kIOReturnBadMessageID] = "sent/received messages had different msg_id";
			sMap [kIOReturnUnsupported] = "unsupported function";
			sMap [kIOReturnVMError] = "misc. VM failure";
			sMap [kIOReturnInternalError] = "internal error";
			sMap [kIOReturnIOError] = "General I/O error";
			sMap [kIOReturnCannotLock] = "can't acquire lock";
			sMap [kIOReturnNotOpen] = "device not open";
			sMap [kIOReturnNotReadable] = "read not supported";
			sMap [kIOReturnNotWritable] = "write not supported";
			sMap [kIOReturnNotAligned] = "alignment error";
			sMap [kIOReturnBadMedia] = "Media Error";
			sMap [kIOReturnStillOpen] = "device(s) still open";
			sMap [kIOReturnRLDError] = "rld failure";
			sMap [kIOReturnDMAError] = "DMA failure";
			sMap [kIOReturnBusy] = "Device Busy";
			sMap [kIOReturnTimeout] = "I/O Timeout";
			sMap [kIOReturnOffline] = "device offline";
			sMap [kIOReturnNotReady] = "not ready";
			sMap [kIOReturnNotAttached] = "device not attached";
			sMap [kIOReturnNoChannels] = "no DMA channels left";
			sMap [kIOReturnNoSpace] = "no space for data";
			sMap [kIOReturnPortExists] = "port already exists";
			sMap [kIOReturnCannotWire] = "can't wire down physical memory";
			sMap [kIOReturnNoInterrupt] = "no interrupt attached";
			sMap [kIOReturnNoFrames] = "no DMA frames enqueued";
			sMap [kIOReturnMessageTooLarge] = "oversized msg received on interrupt port";
			sMap [kIOReturnNotPermitted] = "not permitted";
			sMap [kIOReturnNoPower] = "no power to device";
			sMap [kIOReturnNoMedia] = "media not present";
			sMap [kIOReturnUnformattedMedia] = "media not formatted";
			sMap [kIOReturnUnsupportedMode] = "no such mode";
			sMap [kIOReturnUnderrun] = "data underrun";
			sMap [kIOReturnOverrun] = "data overrun";
			sMap [kIOReturnDeviceError] = "the device is not working properly!";
			sMap [kIOReturnNoCompletion] = "a completion routine is required";
			sMap [kIOReturnAborted] = "operation aborted";
			sMap [kIOReturnNoBandwidth] = "bus bandwidth would be exceeded";
			sMap [kIOReturnNotResponding] = "device not responding";
			sMap [kIOReturnIsoTooOld] = "isochronous I/O request for distant past!";
			sMap [kIOReturnIsoTooNew] = "isochronous I/O request for distant future";
			sMap [kIOReturnNotFound] = "data was not found";
		}
		KernRetStrMapConstIter	iter	(sMap.find (inError));
		return (iter != sMap.end ()) ? iter->second.c_str () : "";
	}	//	GetKernErrStr


//
//	Create method	(CLASS METHOD)
//	Privately called, only by Mac-only DeviceClassListener, only when device attached
//
AJADevicePtr AJADeviceMac::Create (const string & inDeviceClassName, io_service_t inDeviceNub)
{
	AJADevicePtr	result;
	try
	{
		result = new AJADeviceMac (inDeviceClassName, inDeviceNub);
		if (result)
		{
			if (!result->IsOpen ())
				result = NULL;
			else if (!ActiveDeviceMap_Add (result))		//	Add it to the global ActiveDeviceMap
				result = NULL;
		}
	}
	catch (std::bad_alloc)
	{
	}
	return result;

}	//	Create


//
//	Constructor	(EXPLICIT -- construct MacOS-specific AJADevice from string & io_service_t)
//
AJADeviceMac::AJADeviceMac (const string & inDeviceClassName, io_service_t inDeviceNub)
	:	AJADevice (inDeviceClassName),
        mDriverHandle		(0),
        mNotificationHandle	(0)
{
	kern_return_t	kernResult	(KERN_SUCCESS);

	//
	//	Attempt to open the device, and access its user client...
	//
	kernResult = IOServiceOpen (inDeviceNub, mach_task_self (), 0, &mDriverHandle);
	if (kernResult || !mDriverHandle)
	{
		cerr << "IOServiceOpen returned 0x" << hex << kernResult << dec << " (" << kernResult << " -- " << GetKernErrStr (kernResult) << ")" << endl;
		return;
	}

	//
	//	Determine interface name...
	//
	if (inDeviceClassName.find ("com_aja_iokit_") != string::npos)
	{
		uint32_t	boardID (0), bsnLo (0), bsnHi (0);
		uint64_t	serialNumber (0);

		mInterfaceName = AJAInterfaceName_NTV2;	//	NTV2 device -- try to get boardID & serial number from registers 50, 54 and 55...
		NTV2ReadRegister (50, boardID);
		if (NTV2ReadRegister (54, bsnLo) && NTV2ReadRegister (55, bsnHi))
			serialNumber = (uint64_t (bsnHi) << 32) | uint64_t (bsnLo);
		mIdentifierKey = EncodeDeviceIdentifier (boardID, serialNumber);
	}	//	if NTV2 device
	else if (inDeviceClassName == "com_aja_ntv4_pci")
	{
		mInterfaceName = AJAInterfaceName_NTV4;
		cerr << "Need to implement code to fetch NTV4 device type and serial number" << endl;
	}	//	else if NTV4 device
	else
	{
		cerr << "Unknown device" << endl;
		IOServiceClose (mDriverHandle);
        mDriverHandle = 0;
	}

	if (mDriverHandle)
	{
		//	Register for an interest notification of this device being removed. Use a reference to our
		//	private data as the refCon which will be passed to the notification callback.
		kernResult = IOServiceAddInterestNotification (MasterPort::Get (),			// notifyPort
														inDeviceNub,				// service
														kIOGeneralInterest,			// interestType
														MacDeviceNotificationStatic,// callback
														this,						// refCon
														&mNotificationHandle);		// notification
		if (kernResult)
			cerr << "IOServiceAddInterestNotification returned 0x" << hex << kernResult << dec << " (" << kernResult << " -- " << GetKernErrStr (kernResult) << ")" << endl;
	}	//	if known device

}	//	constructor


//
//	Destructor
//
AJADeviceMac::~AJADeviceMac ()
{
	cerr << "AJADeviceMac instance " << hex << this << dec << " destructed" << endl;
	pvt_Terminated ();

}	//	destructor


//
//	MacDeviceNotificationStatic method		(CLASS METHOD)
//	
void AJADeviceMac::MacDeviceNotificationStatic (void * pInRefCon, io_service_t inNub, natural_t inMessageType, void * pInMessageArgument)
{
    (void)inNub;
    (void)pInMessageArgument;

	AJADevice *		pDevice		(reinterpret_cast <AJADevice *> (pInRefCon));
	assert (pDevice && "Bad refCon");

	if (inMessageType == kIOMessageServiceIsTerminated)
	{
		if (pDevice)
		{
			cerr << "Device '" << *pDevice << "' unplugged" << endl;
			pDevice->pvt_Terminated ();
			ActiveDeviceMap_Remove (pDevice);
		}
	}
}	//	MacDeviceNotificationStatic


//
//	Print method	(OVERRIDE)
//
ostream & AJADeviceMac::Print (ostream & inOutStr) const
{
	return AJADevice::Print (inOutStr)	<< "-OSDataPort=0x" << hex << mDriverHandle << dec
										<< "-OSNotification=0x" << hex << mNotificationHandle << dec;
}	//	Print


void AJADeviceMac::pvt_Terminated (void)
{
	if (mNotificationHandle)
	{
		kern_return_t	kernResult	(IOObjectRelease (mNotificationHandle));
		if (kernResult)
			cerr << "IOObjectRelease returned 0x" << hex << kernResult << dec << " (" << kernResult << " -- " << GetKernErrStr (kernResult) << ")" << endl;
        mNotificationHandle = 0;
	}
	if (mDriverHandle)
	{
		IOServiceClose (mDriverHandle);
        mDriverHandle = 0;
	}

}	//	pvt_Terminated


//
//	NTV2ReadRegister method	(OVERRIDE)
//
bool AJADeviceMac::NTV2ReadRegister (const uint64_t inRegisterNumber, uint32_t & outRegisterValue, const uint64_t inRegisterMask) const
{
	kern_return_t 	kernResult		(KERN_FAILURE);
	uint64_t		scalarO_64		(0);
	uint32_t		outputCount		(1);
	const uint64_t	scalarI_64 [2]	= {inRegisterNumber, inRegisterMask};

	if (GetDriverHandle ())
		kernResult = IOConnectCallScalarMethod (GetDriverHandle (),		//	io_connect_t returned from IOServiceOpen
											   0,						//	User client function selector (0=ReadRegister)
											   scalarI_64,				//	Array of scalar (64-bit) input values
											   2,						//	Number of scalar input values
											   &scalarO_64,				//	Array of scalar (64-bit) output values
											   &outputCount);			//	Pointer to number of scalar output values
	outRegisterValue = static_cast <uint32_t> (scalarO_64);
	return kernResult == KERN_SUCCESS;

}	//	NTV2ReadRegister


#define My_NTV4_BUILD_TIME_SIZE		64


typedef struct _My_NTV4MessageHeader
{
	uint64_t	type;
	uint64_t	target;
	uint64_t	status;
	uint64_t	size;
	uint64_t	userContext;
	uint64_t	driverContext;
	uint64_t	flags;

}	My_NTV4MessageHeader;


typedef struct _My_NTV4MessageDeviceInfo
{
	My_NTV4MessageHeader	header;
	uint64_t				deviceInfoFlags;
	uint64_t				deviceType;
	uint64_t				deviceVersion;
	uint64_t				driverVersionMajor;
	uint64_t				driverVersionMinor;
	uint64_t				driverVersionRelease;
	uint64_t				driverVersionBuild;
	char					driverBuildDate [My_NTV4_BUILD_TIME_SIZE];
	char					driverBuildTime [My_NTV4_BUILD_TIME_SIZE];
	uint64_t				driverType;
	uint64_t				driverInterfaceVersion;

} My_NTV4MessageDeviceInfo;


#define My_NTV4_DRIVER_INTERFACE_VERSION	5

#define NTV4_MSG_DRIVER_INIT(_pMsg_, _type_)		memset (_pMsg_, 0, sizeof (*(_pMsg_)));							\
													(_pMsg_)->header.type = (uint64_t) _type_;						\
													(_pMsg_)->header.target = (uint64_t) NTV4_MessageTarget_Driver;	\
													(_pMsg_)->header.status = (uint64_t) AJA_STATUS_UNKNOWN;		\
													(_pMsg_)->header.size = (uint64_t) sizeof (*(_pMsg_));



//
//	NTV4SendMessage method	(OVERRIDE)
//
bool AJADeviceMac::NTV4SendMessage (void * pInMessage) const
{
	kern_return_t 			kernResult		(pInMessage ? KERN_FAILURE : KERN_INVALID_ARGUMENT);
	My_NTV4MessageHeader *	pMessage		(reinterpret_cast <My_NTV4MessageHeader *> (pInMessage));
	uint32_t				outputCount		(0);
	uint64_t				scalarI_64 [2]	= {0, 0};
	uint64_t				scalarO_64[1]	= {0};

	if (pMessage && GetDriverHandle ())
	{
		scalarI_64 [0] = reinterpret_cast <uint64_t> (pMessage);
		scalarI_64 [1] = pMessage->size;
		kernResult = IOConnectCallScalarMethod (GetDriverHandle (),	//	io_connect_t returned from IOServiceOpen
											   0,					//	User client function selector (0=SendNTV4Message)
											   scalarI_64,			//	Array of scalar (64-bit) input values
											   2,					//	Number of scalar input values
											   scalarO_64,			//	Array of scalar (64-bit) output values
											   &outputCount);		//	Pointer to the number of scalar output values
	}
	return kernResult == KERN_SUCCESS;

}	//	NTV4SendMessage
