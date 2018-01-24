/**
	@file		devicenotifier.h
	@brief		Declares the MacOS-specific KonaNotifier and DeviceNotifier classes, which invoke
				a client-registered callback function when devices are attached and/or detached.
	@copyright	(C) 2011-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#if !defined (__DEVICE_NOTIFIER_H__)
	#define	__DEVICE_NOTIFIER_H__

	#include <CoreFoundation/CoreFoundation.h>
	#include <Carbon/Carbon.h>
	#include <IOKit/IOMessage.h>
	#include <IOKit/usb/IOUSBLib.h>
	#include <vector>
	#include <map>
	#include <list>

	#define kAJADeviceInitialOpen	0xAA1
	#define kAJADeviceTerminate		0xAA2


	/**
		@brief	Mac-specific device add/change/remove event notification callback function.
	**/
	typedef void (*DeviceClientCallback)(unsigned long message, void * refcon);


	/**
		@brief	These bit numbers control debug logging to stderr/cerr via "SetDebugLogging" class member function
				or via "NTV2DEVICENOTIFYLOG" environment variable at client startup.
	**/
	enum
	{
		kMacDeviceDebugLog_DeviceRemoved		= 58,	///< @brief	Log device removed events
		kMacDeviceDebugLog_DeviceAdded			= 59,	///< @brief	Log device added events
		kMacDeviceDebugLog_DeviceChanged		= 60,	///< @brief	Log device change events
		kMacDeviceDebugLog_ExitFunctions		= 61,	///< @brief	Log function exit
		kMacDeviceDebugLog_EnterFunctions		= 62,	///< @brief	Log function entry
		kMacDeviceDebugLog_RawMessages			= 63	///< @brief	Log raw events
	};


	/**
		@brief	Mac-specific class that notifies clients when AJA devices are attached/detached, etc.
	**/
	class DeviceNotifier
	{
	//	Instance Methods
	public:
										
												DeviceNotifier (DeviceClientCallback callback, void *refcon);
		virtual									~DeviceNotifier ();
		virtual bool							Install (CFMutableDictionaryRef dict = NULL);
		virtual void							Uninstall ();
		
		// alternate way to set callback
		virtual void							SetCallback (DeviceClientCallback callback, void *refcon);
		
	protected:
		// override these
		inline virtual CFMutableDictionaryRef	CreateMatchingDictionary ()									{return NULL;}
		virtual	CFMutableDictionaryRef			CreateMatchingDictionary (CFStringRef deviceDriverName);

		// callbacks
		virtual void							DeviceAdded (io_iterator_t iterator);
		virtual void							DeviceRemoved (io_iterator_t iterator);
		virtual void							DeviceChanged (io_service_t unitService, natural_t messageType, void* message);

		// util
		virtual void							AddGeneralInterest (io_object_t service);

	//	Class Methods
	public:
		static void								SetDebugLogging (const uint64_t inWhichUserClientCommands);
		static std::string						MessageTypeToStr (const natural_t messageType);

		//	Non-Public Class Methods
		static void								DeviceAddedCallback (	DeviceNotifier* thisObject, io_iterator_t iterator);
		static void								DeviceRemovedCallback ( DeviceNotifier* thisObject, io_iterator_t iterator);
		static void								DeviceChangedCallback (	DeviceNotifier *	thisObject,
																		io_service_t		unitService,
																		natural_t			messageType,
																		void *				message);
	//	Instance Data
	protected:
		void *							m_refcon;
		DeviceClientCallback			m_clientCallback;
		mach_port_t						m_masterPort;
		IONotificationPortRef			m_notificationPort;
		CFMutableDictionaryRef			m_matchingDictionary;
		std::list<io_object_t>			m_deviceMatchList;
		std::list<io_object_t>			m_deviceInterestList;

	};	//	DeviceNotifier


	/**
		@brief	Subclass of DeviceNotifier that notifies clients when Kona/Corvid/Io/TTap devices are attached/detached.
	**/
	class KonaNotifier : public DeviceNotifier
	{
	public:
		inline					KonaNotifier (DeviceClientCallback callback, void *refcon)
									:	DeviceNotifier (callback, refcon)	{}
		inline virtual			~KonaNotifier ()							{}
		virtual bool			Install (CFMutableDictionaryRef dict = NULL);
	};

#endif	//	__DEVICE_NOTIFIER_H__
