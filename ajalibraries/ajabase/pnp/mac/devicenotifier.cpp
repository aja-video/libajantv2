/**
	@file		devicenotifier.cpp
	@brief		Implements the MacOS-specific KonaNotifier and DeviceNotifier classes, which invoke
				a client-registered callback function when devices are attached and/or detached.
	@copyright	(C) 2011-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include <syslog.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <IOKit/IOCFPlugIn.h>
#include "devicenotifier.h"

#include "ajabase/common/common.h"

//#include "IOKit/firewire/IOFireWireFamilyCommon.h"	//	No more FireWire support


using namespace std;

static const char * GetKernErrStr (const kern_return_t inError);


//	DeviceNotifier-specific Logging Macros

#define	HEX2(__x__)				"0x" << hex << setw (2)  << setfill ('0') << (0xFF       & uint8_t (__x__)) << dec
#define	HEX4(__x__)				"0x" << hex << setw (4)  << setfill ('0') << (0xFFFF     & uint16_t(__x__)) << dec
#define	HEX8(__x__)				"0x" << hex << setw (8)  << setfill ('0') << (0xFFFFFFFF & uint32_t(__x__)) << dec
#define	HEX16(__x__)			"0x" << hex << setw (16) << setfill ('0') <<               uint64_t(__x__)  << dec
#define KR(_kr_)				"kernResult=" << HEX8(_kr_) << "(" << GetKernErrStr (_kr_) << ")"
#define INSTP(_p_)				" instance=" << HEX16(uint64_t(_p_))

#if defined (_DEBUG)
	#define	DNDB(__lvl__, __x__)	do {																										\
										ostringstream   oss;																					\
										pthread_t		curThrdID	(::pthread_self ());														\
										oss << "## " << __lvl__ << ":  " << HEX16(curThrdID) << ":  " << string(__func__) << ":  " << __x__;	\
										cerr << oss.str () << endl;																				\
									} while (false)
#else
	#define	DNDB(__lvl__, __x__)	do {																										\
										ostringstream   oss;																					\
										pthread_t		curThrdID	(::pthread_self ());														\
										oss << "## " << __lvl__ << ":  " << HEX16(curThrdID) << ":  " << string(__func__) << ":  " << __x__;	\
										syslog (3, "%s\n", oss.str ().c_str ());																\
									} while (false)
#endif

#define	DNFAIL(__x__)			DNDB ("ERROR",		__x__)
#define	DNWARN(__x__)			DNDB ("WARNING",	__x__)
#define	DNNOTE(__x__)			DNDB ("NOTE",		__x__)
#if defined (_DEBUG)
	#define	DNDBG(__x__)		DNDB ("DEBUG",		__x__)
#else
	#define	DNDBG(__x__)
#endif

#define	DNFAILIF(__uccc__, __x__)		do {													\
											if (gErrorLogging & (uint64_t (1) << (__uccc__)))	\
												DNFAIL(__x__);									\
										} while (false)

#define	DNWARNIF(__uccc__, __x__)		do {													\
											if (gErrorLogging & (uint64_t (1) << (__uccc__)))	\
												DNWARN(__x__);									\
										} while (false)

#define	DNNOTEIF(__uccc__, __x__)		do {													\
											if (gErrorLogging & (uint64_t (1) << (__uccc__)))	\
												DNNOTE(__x__);									\
										} while (false)


static uint64_t		gErrorLogging	(0x0000000000000000);	///	Log errors? (one flag bit per UserClientCommandCode)


// MARK: DeviceNotifier

//-------------------------------------------------------------------------------------------------------------
//	DeviceNotifier
//-------------------------------------------------------------------------------------------------------------
DeviceNotifier::DeviceNotifier (DeviceClientCallback callback, void *refcon)
	:	m_refcon				(refcon),
		m_clientCallback		(callback),
		m_masterPort			(0),
		m_notificationPort		(NULL),
		m_matchingDictionary	(NULL)
{
}


//-------------------------------------------------------------------------------------------------------------
//	~DeviceNotifier
//-------------------------------------------------------------------------------------------------------------
DeviceNotifier::~DeviceNotifier ()
{
	// disable callbacks
	Uninstall ();
	
	m_masterPort = 0;
	m_clientCallback = NULL;
	m_refcon = NULL;
}


//-------------------------------------------------------------------------------------------------------------
//	SetCallback
//-------------------------------------------------------------------------------------------------------------
void DeviceNotifier::SetCallback (DeviceClientCallback callback, void *refcon)
{
	m_clientCallback = callback;
	m_refcon = refcon;
}


//-------------------------------------------------------------------------------------------------------------
//	Install
//-------------------------------------------------------------------------------------------------------------

bool DeviceNotifier::Install (CFMutableDictionaryRef matchingDictionary)
{
	DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "in, m_deviceInterestList.size()=" << m_deviceInterestList.size()
												<< ", m_deviceMatchList.size()=" << m_deviceMatchList.size()
												<< ", matchingDictionary=" << HEX16(matchingDictionary));

	m_matchingDictionary = matchingDictionary;
	
	// if no dictionary given use default
	if (m_matchingDictionary == NULL)
		m_matchingDictionary = CreateMatchingDictionary ();
	else
		::CFRetain (m_matchingDictionary);		// if dictionary was passed in retain it

	// check for NULL dictionary
	if (m_matchingDictionary == NULL)
	{
		DNFAIL ("NULL matchingDictionary");
		return false;
	}
	
	// Retrieve the IOKit's master port so a notification port can be created
	mach_port_t masterPort;
	IOReturn ioReturn = ::IOMasterPort (MACH_PORT_NULL, &masterPort);
	if (kIOReturnSuccess != ioReturn)
	{
		DNFAIL (KR(ioReturn) << " -- IOMasterPort failed");
		return false;
	}

	m_masterPort = masterPort;
	m_notificationPort = ::IONotificationPortCreate (masterPort);
	if (0 == m_notificationPort)
	{
		DNFAIL ("IONotificationPortCreate failed");
		return false;
	}
	
	// Get the CFRunLoopSource for the notification port and add it to the default run loop.
	// It is not necessary to call CFRunLoopRemoveSource() duringing tear down because that is implicitly done when
	// the the notification port is destroyed.
	CFRunLoopSourceRef runLoopSource = ::IONotificationPortGetRunLoopSource (m_notificationPort);
	CFRunLoopAddSource (CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

	// IOServiceAddMatchingNotification 'eats' a matching dictionary, so up the retention count
	CFRetain(m_matchingDictionary);

	// lets create a callback
	io_iterator_t iterator;
	ioReturn = IOServiceAddMatchingNotification(m_notificationPort, 
												kIOMatchedNotification, 
												m_matchingDictionary,
												reinterpret_cast<IOServiceMatchingCallback>(DeviceAddedCallback), 
												this, 
												&iterator);

	if (kIOReturnSuccess != ioReturn)
	{
		DNFAIL (KR(ioReturn) << " -- IOServiceAddMatchingNotification failed");
		return false;
	}
	
	DeviceAddedCallback (this, iterator);
	m_deviceMatchList.push_back(iterator);

	DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "out, m_deviceInterestList.size()=" << m_deviceInterestList.size() << ", m_deviceMatchList.size()=" << m_deviceMatchList.size());
	return (m_deviceInterestList.size() > 0);
}


//--------------------------------------------------------------------------------------------------------------------
//	Uninstall
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::Uninstall ()
{
	DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "in, m_deviceInterestList.size()=" << m_deviceInterestList.size()
												<< ", m_deviceMatchList.size()=" << m_deviceMatchList.size());
	//	Release device-matching list...
	list<io_object_t>::iterator p;
	for (p = m_deviceMatchList.begin(); p != m_deviceMatchList.end(); p++)
		IOObjectRelease (*p);
	m_deviceMatchList.clear();

	//	Release device-interest list...
	for (p = m_deviceInterestList.begin(); p != m_deviceInterestList.end(); p++)
		IOObjectRelease (*p);
	m_deviceInterestList.clear();

	if (m_notificationPort)
	{
		CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource (m_notificationPort);
		CFRunLoopRemoveSource (CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
		IONotificationPortDestroy (m_notificationPort);
		m_notificationPort = 0;
	}

	if (m_matchingDictionary)
	{
		CFRelease (m_matchingDictionary);
		m_matchingDictionary = NULL;
	}
	DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "out, m_deviceInterestList.size()=" << m_deviceInterestList.size() << ", m_deviceMatchList.size()=" << m_deviceMatchList.size());
}


//--------------------------------------------------------------------------------------------------------------------
//	CreateMatchingDictionary
//	This high level callbacks only when a specific driver is loaded, goes offline
//--------------------------------------------------------------------------------------------------------------------
CFMutableDictionaryRef DeviceNotifier::CreateMatchingDictionary (CFStringRef deviceDriverName)
{
	// This high level callbacks only when driver is loaded, goes offline
	CFMutableDictionaryRef matchingDictionary = CFDictionaryCreateMutable (	kCFAllocatorDefault,
																			0,
																			&kCFTypeDictionaryKeyCallBacks,
																			&kCFTypeDictionaryValueCallBacks);
	// Specify class type
	CFDictionaryAddValue (matchingDictionary, CFSTR("IOProviderClass"), deviceDriverName);
	
	return matchingDictionary;
}


// MARK: Callbacks


//--------------------------------------------------------------------------------------------------------------------
//	DeviceAddedCallback
//	matching AJA IOService found
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceAddedCallback (DeviceNotifier* thisObject, io_iterator_t iterator)
{
	DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "in");
	thisObject->DeviceAdded (iterator);
	DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "out");
}


//--------------------------------------------------------------------------------------------------------------------
//	DeviceAdded
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceAdded (io_iterator_t iterator)
{
	DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "in");

	io_object_t	service;
	bool deviceFound = false;
	
	//	This iteration is essential to keep this callback working...
	IOIteratorReset(iterator);
	for ( ;(service = IOIteratorNext(iterator)); IOObjectRelease(service))
	{
		AddGeneralInterest(service);		// optional
		deviceFound = true;
	}
	
	// now notify our callback
	if (deviceFound && m_clientCallback)
		(*(m_clientCallback))(kAJADeviceInitialOpen, m_refcon);

	DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "out");
}


//--------------------------------------------------------------------------------------------------------------------
//	DeviceRemovedCallback
//	matching AJA IOService has been removed
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceRemovedCallback (DeviceNotifier* thisObject, io_iterator_t iterator)
{
	DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "in");
	thisObject->DeviceRemoved (iterator);
	DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "out");
}


//--------------------------------------------------------------------------------------------------------------------
//	DeviceRemoved
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceRemoved (io_iterator_t iterator)
{
	DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "in");

	io_object_t	service;
	bool deviceFound = false;
	
	//	This iteration is essential to keep this callback working...
	IOIteratorReset(iterator);
	for ( ;(service = IOIteratorNext(iterator)); IOObjectRelease(service))
		deviceFound = true;

	// now notify our callback
	if (deviceFound && m_clientCallback)
		(*(m_clientCallback))(kAJADeviceTerminate, m_refcon);

	DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "out");
}


//--------------------------------------------------------------------------------------------------------------------
//	AddGeneralInterest
//	add general interest callback, return true if iterator has one or more items
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::AddGeneralInterest (io_object_t service)
{
	io_object_t notifier;

	// Create a notifier object so 'general interest' notifications can be received for the service.
	// In Kona this is used for debugging only
	IOReturn ioReturn = ::IOServiceAddInterestNotification (m_notificationPort,
															service,
															kIOGeneralInterest,
															reinterpret_cast <IOServiceInterestCallback> (DeviceChangedCallback),
															this,
															&notifier);
	if (kIOReturnSuccess != ioReturn)
	{
		DNFAIL (KR(ioReturn) << " -- IOServiceAddInterestNotification failed");
		return;
	}
	m_deviceInterestList.push_back (notifier);
}


//--------------------------------------------------------------------------------------------------------------------
//  DeviceChangedCallback()
//	notifier messages sent by the driver
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceChangedCallback (DeviceNotifier* thisObject, io_service_t unitService, natural_t messageType, void* message)
{
	//DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "in");
	thisObject->DeviceChanged (unitService, messageType, message);
	//DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "out");
}


//--------------------------------------------------------------------------------------------------------------------
//  DeviceChanged()
//--------------------------------------------------------------------------------------------------------------------
void DeviceNotifier::DeviceChanged (io_service_t unitService, natural_t messageType, void* message)
{
	DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "messageType=" << MessageTypeToStr(messageType) << ", message=" << HEX16(message));
    AJA_UNUSED(unitService);

	if (m_clientCallback)
		(*(m_clientCallback))(messageType, m_refcon);	// notify client

	DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "out");
}



//--------------------------------------------------------------------------------------------------------------------
//  MessageTypeToStr()
//	decode message type
//--------------------------------------------------------------------------------------------------------------------
string DeviceNotifier::MessageTypeToStr (const natural_t messageType)
{
	ostringstream	oss;
	switch (messageType)
	{
		case kIOMessageServiceIsTerminated:			oss << "kIOMessageServiceIsTerminated";			break;
		case kIOMessageServiceIsSuspended:			oss << "kIOMessageServiceIsSuspended";			break;
		case kIOMessageServiceIsResumed:			oss << "kIOMessageServiceIsResumed";			break;
		case kIOMessageServiceIsRequestingClose:	oss << "kIOMessageServiceIsRequestingClose";	break;
		// the more esoteric messages:
		case kIOMessageServiceIsAttemptingOpen:		oss << "kIOMessageServiceIsAttemptingOpen";		break;	//	When another process connects to our device
		case kIOMessageServiceWasClosed:			oss << "kIOMessageServiceWasClosed";			break;	//	When another process disconnects from our device
		case kIOMessageServiceBusyStateChange:		oss << "kIOMessageServiceBusyStateChange";		break;
		case kIOMessageCanDevicePowerOff:			oss << "kIOMessageCanDevicePowerOff";			break;
		case kIOMessageDeviceWillPowerOff:			oss << "kIOMessageDeviceWillPowerOff";			break;
		case kIOMessageDeviceWillNotPowerOff:		oss << "kIOMessageDeviceWillPowerOff";			break;
		case kIOMessageDeviceHasPoweredOn:			oss << "kIOMessageDeviceHasPoweredOn";			break;
		case kIOMessageCanSystemPowerOff:			oss << "kIOMessageCanSystemPowerOff";			break;
		case kIOMessageSystemWillPowerOff:			oss << "kIOMessageSystemWillPowerOff";			break;
		case kIOMessageSystemWillNotPowerOff:		oss << "kIOMessageSystemWillNotPowerOff";		break;
		case kIOMessageCanSystemSleep:				oss << "kIOMessageCanSystemSleep";				break;
		case kIOMessageSystemWillSleep:				oss << "kIOMessageSystemWillSleep";				break;
		case kIOMessageSystemWillNotSleep:			oss << "kIOMessageSystemWillNotSleep";			break;
		case kIOMessageSystemHasPoweredOn:			oss << "kIOMessageSystemHasPoweredOn";			break;
	//	case kIOFWMessageServiceIsRequestingClose:	oss << "kIOFWMessageServiceIsRequestingClose";	break;	//	No more FireWire support
	//	case kIOFWMessageTopologyChanged:			oss << "kIOFWMessageTopologyChanged";			break;	//	No more FireWire support
		default:									oss << "0x" << hex << setw(4) << setfill('0') << messageType;	break;
	}
	return oss.str ();
}


void DeviceNotifier::SetDebugLogging (const uint64_t inWhichUserClientCommands)
{
	gErrorLogging = inWhichUserClientCommands;
	cerr << "DeviceNotifier::SetDebugLogging 0x" << hex << gErrorLogging << dec << endl;
}


// MARK: KonaNotifier


bool KonaNotifier::Install (CFMutableDictionaryRef matchingDictionary)
{
	DNNOTEIF (kMacDeviceDebugLog_EnterFunctions, "m_deviceInterestList.size()=" << m_deviceInterestList.size()
												<< ", m_deviceMatchList.size()=" << m_deviceMatchList.size()
												<< ", matchingDictionary=" << HEX16(matchingDictionary) << " (unused)");
	// Retrieve the IOKit's master port so a notification port can be created
	mach_port_t masterPort;
	IOReturn ioReturn = ::IOMasterPort (MACH_PORT_NULL, &masterPort);
	if (kIOReturnSuccess != ioReturn)
	{
		DNFAIL (KR(ioReturn) << " -- IOMasterPort failed");
		return false;
	}

	m_masterPort = masterPort;
	m_notificationPort = ::IONotificationPortCreate (masterPort);
	if (0 == m_notificationPort)
	{
		DNFAIL ("IONotificationPortCreate failed");
		return false;
	}
	
	// Get the CFRunLoopSource for the notification port and add it to the default run loop.
	// It is not necessary to call CFRunLoopRemoveSource() duringing tear down because that is implicitly done when
	// the the notification port is destroyed.
	CFRunLoopSourceRef runLoopSource = ::IONotificationPortGetRunLoopSource (m_notificationPort);
	::CFRunLoopAddSource (::CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
	

	// walk through each of our devices
	static const std::string driverName ("com_aja_iokit_ntv2");
	io_iterator_t notifyIterator_matched, notifyIterator_terminated;
		
	// Device Added
	ioReturn = ::IOServiceAddMatchingNotification (	m_notificationPort,
													kIOMatchedNotification,
													IOServiceMatching(driverName.c_str ()),
													reinterpret_cast<IOServiceMatchingCallback>(DeviceAddedCallback), 
													this, 
													&notifyIterator_matched);

	if (ioReturn != kIOReturnSuccess)
	{
		DNFAIL (KR(ioReturn) << " -- IOServiceAddMatchingNotification for 'kIOMatchedNotification' failed");
		return false;
	}

	// Device Terminated
	ioReturn = ::IOServiceAddMatchingNotification (	m_notificationPort,
													kIOTerminatedNotification,
													IOServiceMatching(driverName.c_str ()),
													reinterpret_cast<IOServiceMatchingCallback>(DeviceRemovedCallback), 
													this, 
													&notifyIterator_terminated);
	if (ioReturn != kIOReturnSuccess)
	{
		DNFAIL (KR(ioReturn) << " -- IOServiceAddMatchingNotification for 'kIOTerminatedNotification' failed");
		return false;
	}

	DeviceAddedCallback (this, notifyIterator_matched);
	m_deviceMatchList.push_back (notifyIterator_matched);

	DeviceRemovedCallback (this, notifyIterator_terminated);
	m_deviceMatchList.push_back (notifyIterator_terminated);

	DNNOTEIF (kMacDeviceDebugLog_ExitFunctions, "m_deviceInterestList.size()=" << m_deviceInterestList.size() << ", m_deviceMatchList.size()=" << m_deviceMatchList.size());
	return m_deviceInterestList.size() > 0;	//	**MrBill**	SHOULDN'T THIS RETURN m_deviceMatchList.size() > 0	????

}	//	KonaNotifier::Install


//
//	Device Notifier Environment Variable Reader:
//
class DNEnvVarReader
{
	public:
		DNEnvVarReader ()
		{
			//	To allow some logging flexibility at runtime... set the 'NTV2DEVICENOTIFYLOG' environment variable to a
			//	hex string value '0x0000000000000000' that will be interpreted as a uint64_t...
			const char *	pVarName("NTV2DEVICENOTIFYLOG");
			const string	sEnvLog	(::getenv (pVarName) ? ::getenv (pVarName) : "");
			if (sEnvLog.length () == 18 && sEnvLog.find ("0x") == 0)
			{
				uint64_t			logMask (0);
				std::stringstream	ss;
				ss << hex << sEnvLog.substr (2, 16);
				ss >> logMask;
				DeviceNotifier::SetDebugLogging (logMask);
			}
		}
};	//	DNEnvVarReader

static DNEnvVarReader	gDNEnvVarReader;

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
		default:						return "";
	}
}	//	GetKernErrStr
