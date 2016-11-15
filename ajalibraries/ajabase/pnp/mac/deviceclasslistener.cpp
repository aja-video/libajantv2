/**
	@file		pnp/mac/deviceclasslistener.cpp
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the DeviceClassListener class.
**/

#include "deviceclasslistener.h"
#include "masterport.h"
#include <map>

using namespace std;


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


ostream &	operator << (ostream & inOutStr, const DeviceClassListener & inObj)
{
	return inOutStr << "DeviceClassListener instance " << hex << &inObj << dec << " '" << inObj.GetDeviceClassName () << "' " << (inObj.IsOkay () ? "Ready" : "Not Ready");

}	//	DeviceClassListener ostream operator <<


ostream &	operator << (ostream & inOutStr, const DeviceClassListenerPtr & inObj)
{
	return inObj ? inOutStr << *inObj : inOutStr;

}	//	DeviceClassListenerPtr ostream operator <<


DeviceClassListenerPtr DeviceClassListener::Create (const string & inDeviceClassName)
{
	DeviceClassListenerPtr	outObj;
	try
	{
		if (!inDeviceClassName.empty ())
			outObj = new DeviceClassListener (inDeviceClassName);
		if (outObj)
			if (!outObj->IsOkay ())
				outObj = NULL;
	}
	catch (std::bad_alloc)
	{
	}
	return outObj;

}	//	Create


DeviceClassListener::~DeviceClassListener ()
{
	cerr << *this << " destructed" << endl;

}	//	destructor


DeviceClassListener::DeviceClassListener (const string & inDeviceClassName)
	:	mDeviceClassName	(inDeviceClassName),
		mReady				(false)
{
	//
	//	Set up the matching criteria for the devices we're interested in. The matching criteria needs to follow
	//	the same rules as kernel drivers. For USB devices, it needs to follow the USB Common Class Specification, pp. 6-7.
	//	See also Technical Q&A QA1076 "Tips on USB driver matching on Mac OS X"	<http://developer.apple.com/qa/qa2001/qa1076.html>.
	//	One exception is that you can use the matching dictionary "as is", i.e. without adding any matching criteria to it,
	//	and it will match every IODevice in the system. IOServiceAddMatchingNotification will consume this dictionary
	//	reference, so there's no need to release it later on.
	//
	CFMutableDictionaryRef	matchingDict	(IOServiceMatching (GetDeviceClassName ().c_str ()));	//	Interested in instances of specific class (and its subclasses)
	if (!matchingDict)
	{
		cerr << "IOServiceMatching returned NULL" << endl;
		return;
	}

	//
	//	Set up a notification to be called when a device is first matched by IOKit...
	//
    io_object_t		pNotification	(0);		//	Actually an iterator that will enumerate a list of matching devices
	kern_return_t	kernResult	(IOServiceAddMatchingNotification (MasterPort::Get (),			// notifyPort
																  kIOFirstMatchNotification,	// notificationType
																  matchingDict,					// matching
																  MacDevicesAdded,				// callback
																  this,							// refCon
																  &pNotification));				// notification
	if (kernResult || !pNotification)
	{
		cerr	<< "IOServiceAddMatchingNotification failed for '" << inDeviceClassName << "', error " << kernResult << " (0x" << hex << kernResult << dec
				<< ", " << GetKernErrStr (kernResult) << "), pNotification = 0x" << hex << pNotification << dec << endl;
		return;
	}

	//
	//	Iterate once to get already-present devices and arm the notification...
	//
	MacDevicesAdded (this, pNotification);

	mReady = true;

}	//	constructor


/**
	@brief	This is the IOServiceAddMatchingNotification callback.  When it's called, a new
			AJADevice will be created for every device referenced in the given iterator.
**/
void DeviceClassListener::MacDevicesAdded (void * pInRefCon, io_iterator_t inIterator)			//	STATIC
{
    (void)pInRefCon;

	kern_return_t	kernResult		(KERN_SUCCESS);
    io_service_t	pDeviceInstance	(0);

	while ((pDeviceInstance = IOIteratorNext (inIterator)))
	{
		io_name_t	deviceClassNameCStr;
		memset (deviceClassNameCStr, 0, sizeof (deviceClassNameCStr));

		//	Get the name of the new device's device class...
		kernResult = IORegistryEntryGetName (pDeviceInstance, deviceClassNameCStr);
		if (kernResult)
			deviceClassNameCStr [0] = '\0';

		const string	deviceClassName	(deviceClassNameCStr);
		AJADevicePtr	theNewDevice	(AJADeviceMac::Create (deviceClassName, pDeviceInstance));
		if (theNewDevice)
			cerr << "AJADevice " << theNewDevice << " connected" << endl;
		else
			cerr << "Failed to create AJADevice '" << deviceClassName << "'" << endl;

		//	Release the reference added by IOIteratorNext
		kernResult = IOObjectRelease (pDeviceInstance);
	}	//	for each device added

}	//	MacDevicesAdded
