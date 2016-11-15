/**
	@file		pnp/ajadevicescanner.h
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares AJADeviceScanner and AJADeviceScannerPtr.
**/

#if !defined (__AJADEVICESCANNER_H__)
	#define __AJADEVICESCANNER_H__

	//	Includes
	#include "ajadevice.h"
	#include <list>


	/**
		@brief	I'm a sequence of AJADevice instances.
	**/
	typedef std::list <AJADevicePtr>		AJADeviceList;
	typedef AJADeviceList::iterator			AJADeviceListIter;		///	Convenient non-const iterator
	typedef AJADeviceList::const_iterator	AJADeviceListConstIter;	///	Convenient const iterator


	/**
		@brief		Emits the given AJADeviceList into the given output stream in a human-readable form.
		@param		inOutStr	The output stream to receive the human-readable representation of the device list.
		@param[in]	inList		The AJADeviceList to be streamed to the given output stream.
		@return		A reference to the specified output stream.
	**/
	std::ostream & operator << (std::ostream & inOutStr, const AJADeviceList & inList);


	/**
		@brief	I provide a set of class methods that permit clients to enumerate and/or find AJA devices that are connected to
				the host, and to monitor their coming and going in real time.
				Use GetAllDevices to obtain a snapshot of all devices.
				Use GetAllNTV2Devices or GetAllNTV4Devices to obtain a snapshot of NTV2 or NTV4 devices, respectively.
				Use FindDevices to look for specific AJA devices by model.
				Use FindDevice to look for a specific AJA device by serial number.
				Call InstallNotificationCallback to get called whenever an AJA device is connected, disconnected, sleeps, or wakes.
				Call HasInstalledNotificationCallback to determine if a callback is installed.
				Call RemoveNotificationCallback to stop getting called.
	**/
	class AJADeviceScanner;
	typedef AJARefPtr <AJADeviceScanner>	AJADeviceScannerPtr;

	class AJADeviceScanner
	{
		//	Class Methods
		public:
			/**
				@brief	Returns a list containing a snapshot of all active, connected, and powered-on AJA devices that are known to the host.
				@note	The order of the list is indeterminate, and will vary depending upon host platform, machine BIOS implementations,
						if/when devices were attached or detached after host startup, etc.  Clients should not make any assumptions
						about the order of devices in the returned list.
				@return	A container that has zero or more AJADevicePtrs.
			**/
			static AJADeviceList			GetAllDevices (void);

			/**
				@brief	Returns a list containing a snapshot of all active, connected and powered-on AJA NTV2 devices that are known to the host.
				@note	The order of the list is indeterminate, and will vary depending upon host platform, machine BIOS implementations,
						if/when devices were attached or detached after host startup, etc.  Clients should not make any assumptions
						about the order of devices in the returned list.
				@return	A container that has zero or more AJADevicePtrs.
			**/
			static AJADeviceList			GetAllNTV2Devices (void);

			/**
				@brief	Returns a list containing a snapshot of all active, connected, and powered-on AJA NTV4 devices that are known to the host.
				@note	The order of the list is indeterminate, and will vary depending upon host platform, machine BIOS implementations,
						if/when devices were attached or detached after host startup, etc.  Clients should not make any assumptions
						about the order of devices in the returned list.
				@return	A container that has zero or more AJADevicePtrs.
			**/
			static AJADeviceList			GetAllNTV4Devices (void);

			/**
				@brief	Returns a list containing a snapshot of all active, connected, and powered-on AJA NTV2 devices that are known to the host
						that also have the given 32-bit device identifier value. This allows clients to specifically find, for example, active,
						connected, and powered-on IoXT devices.
				@param[in]	inBoardID	Specifies the 32-bit device identifier value to look for (e.g., BOARD_ID_IOXT).
				@note	The order of the list is indeterminate, and will vary depending upon host platform, machine BIOS implementations,
						if/when devices were attached or detached after host startup, etc.  Clients should not make any assumptions
						about the order of devices in the returned list.
				@return	A container that has zero or more AJADevicePtrs.
			**/
			static AJADeviceList			FindDevices (const uint32_t inDeviceID);

			/**
				@brief	Returns the AJA NTV2 device having the given serial number, assuming it's active, connected, powered-on, and known
						to the host.
				@param[in]	inSerialNumber	Specifies the 64-bit device serial number value of the device to look for.
				@return	An AJADevicePtr that can be used to access the device.
			**/
			static AJADevicePtr				FindDevice (const uint64_t inSerialNumber);

			/**
				@brief	Returns the AJA NTV2 device having the given device identifier, assuming the device is active, connected, powered-on,
						and known to the host.
				@param[in]	inDeviceIdentifier	Specifies the device identifier of the device to look for.
				@return	An AJADevicePtr that can be used to access the device.
			**/
			static AJADevicePtr				FindDevice (const AJADeviceIdentifier inDeviceIdentifier);

			/**
				@brief	Adds the given function and user data value to the list of function/userData pairs to be called when an AJA device
						is connected or disconnected.
				@param[in]	pInCallback		Points to the function that is desired to be called for AJA connect/disconnect events.
				@param[in]	pInUserData		Specifies any data desired to be passed to the callback function.
											WARNING --	If an instance pointer is passed into this parameter, be sure to call
														RemoveNotificationCallback before deleting the instance.
				@param[in]	inPlayAttached	If true, immediately calls the given callback function on the current thread once for
											each device that's currently attached to the host;  otherwise does nothing (the default).
				@return	True if installed successfully;  otherwise false.
				@note	It is an error to attempt to add the same callback function and user data value more than once without an intervening
						call to RemoveNotificationCallback.
				@note	WARNING -- The callback function is likely to be called asynchronously on a thread other than main.
				@note	It is permissible to install the same callback function with different user data values. Upon device attachment
						or detachment, this will cause the callback function to be called multiple times, once for each different user
						data value.
			**/
			static bool						AddNotificationCallback (AJADeviceNotificationCallback pInCallback, void * pInUserData = NULL, const bool inPlayAttached = false);

			/**
				@brief	Returns true if the given function pointer and user data value are currently installed.
				@param[in]	pInCallback		Points to the function.
				@param[in]	pInUserData		Optionally specifies the data value to be passed to the callback function.
				@return	True if the specified function (and user data value) are currently installed;  otherwise false.
			**/
			static bool						HasInstalledNotificationCallback (AJADeviceNotificationCallback pInCallback, void * pInUserData = NULL);

			/**
				@brief	Removes any installed callback function.
				@return	True if successful;  otherwise false.
			**/
			static bool						RemoveNotificationCallback (AJADeviceNotificationCallback pInCallback, void * pInUserData = NULL);

	};	//	AJADeviceScanner


#endif	//	__AJADEVICESCANNER_H__
