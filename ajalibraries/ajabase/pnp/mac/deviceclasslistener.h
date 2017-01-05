/**
	@file		pnp/mac/deviceclasslistener.h
	@copyright	Copyright (C) 2013-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the DeviceClassListener class.
**/

#if !defined (__AJADEVICECLASSLISTENER_H__)
	#define __AJADEVICECLASSLISTENER_H__

	//	Includes
	#include "ajadevicemac.h"
	#include <IOKit/IOKitLib.h>


	/**
		@brief	I listen for new AJA devices of a particular driver class (e.g., 'com_aja_iokit_kona3g').
				I create a new AJADevice instance for each newly discovered AJA device.
	**/
	class DeviceClassListener;
	typedef AJARefPtr <DeviceClassListener>	DeviceClassListenerPtr;

	class DeviceClassListener
	{
		//	Class Methods
		public:
			static DeviceClassListenerPtr		Create (const std::string & inDeviceClassName);

		//	Instance Methods
		public:
			virtual								~DeviceClassListener ();
			virtual inline const std::string &	GetDeviceClassName (void) const										{return mDeviceClassName;}
			virtual bool						IsOkay (void) const													{return mReady;}

		protected:
			static void							MacDevicesAdded (void * inRefCon, io_iterator_t inIterator);

		private:
			explicit							DeviceClassListener (const std::string & inDeviceClassName);
			inline								DeviceClassListener (const DeviceClassListener & inObj)				{if (&inObj != this) assert (false);}
			inline DeviceClassListener &		operator = (const DeviceClassListener & inRHS)						{if (&inRHS != this) assert (false); return *this;}

		//	Instance Data
		private:
			const std::string					mDeviceClassName;	///	The device class I'm looking for (e.g., 'com_aja_iokit_ioxt')
			bool								mReady;				///	True only if ready and listening

	};	//	DeviceClassListener


	std::ostream &	operator << (std::ostream & inOutStr, const DeviceClassListener & inObj);
	std::ostream &	operator << (std::ostream & inOutStr, const DeviceClassListenerPtr & inObj);

#endif	//	__AJADEVICECLASSLISTENER_H__
