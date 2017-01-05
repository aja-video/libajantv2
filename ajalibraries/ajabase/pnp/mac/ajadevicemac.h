/**
	@file		pnp/mac/ajadevicemac.h
	@copyright	Copyright (C) 2013-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJADeviceMac class.
**/

#if !defined (AJADEVICEMAC_H)
	#define	AJADEVICEMAC_H

	#if !defined (AJAMac)
		#error "This must be compiled on MacOS platform"
	#endif	//	not AJAMac


	#include "ajabase/pnp/ajadevice.h"
	#include <IOKit/IOKitLib.h>


	/**
		@brief	I am the MacOS-specific flavor of AJADevice.
				I provide a Create class method for creating new instances.
				I override IsOpen, Print, NTV2ReadRegister and NTV4SendMessage.
	**/
	class AJADeviceMac;

	class AJADeviceMac : public AJADevice
	{
		//	Class Methods
		public:
			static AJADevicePtr				Create (const std::string & inDeviceClassName, io_service_t inDeviceNub);

		//	Instance Methods
		public:
			virtual inline io_connect_t		GetDriverHandle (void) const							{return mDriverHandle;}
			virtual inline io_object_t		GetIORegistryHandle (void) const						{return mNotificationHandle;}
			virtual inline					operator io_connect_t () const							{return GetDriverHandle ();}

		//	Instance Methods
		public:
			virtual							~AJADeviceMac ();
			virtual inline bool				IsOpen (void) const										{return mDriverHandle ? true : false;}
			virtual std::ostream &			Print (std::ostream & inOutStr) const;
			virtual void					pvt_Terminated (void);

		protected:
			static void						MacDeviceNotificationStatic (void * pInRefCon, io_service_t inService, natural_t inMessageType, void * pInMessageArgument);

		private:
			explicit						AJADeviceMac (const std::string & inDeviceClassName, io_service_t inDeviceInstance);
			explicit inline					AJADeviceMac ()											{assert (false);}
			inline							AJADeviceMac (const AJADeviceMac & inObj)				{if (&inObj != this) assert (false);}
			inline AJADeviceMac &			operator = (const AJADeviceMac & inRHS)					{if (&inRHS != this) assert (false); return *this;}

			virtual bool					NTV2ReadRegister (const uint64_t inRegisterNumber, uint32_t & outRegisterValue, const uint64_t inRegisterMask = 0xFFFFFFFFFFFFFFFF) const;
			virtual bool					NTV4SendMessage (void * pInMessage) const;

		//	Instance Data
		private:
			io_connect_t					mDriverHandle;			///	Handle to my device's userClient
			io_object_t						mNotificationHandle;	///	Handle to my IORegistry object that informs me if/when my device disappears

	};	//	AJADevice

#endif	//	AJADEVICEMAC_H
