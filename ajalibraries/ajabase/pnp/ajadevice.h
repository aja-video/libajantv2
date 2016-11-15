/**
	@file		pnp/ajadevice.h
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJADevice class.
**/

#if !defined (AJADEVICE_H)
	#define	AJADEVICE_H


	#include <assert.h>
	#include <iostream>
	#include <string>
	#include "ajabase/common/ajarefptr.h"


	/**
		@brief	These are the device notification callback messages.
	**/
	typedef enum
	{
		AJA_DeviceAdded,			///	The given device was connected/attached
		AJA_DeviceRemoved,			///	The given device was disconnected/detached/unplugged
		AJA_DeviceOnline,			///	Future -- the given device became available for use.
		AJA_DeviceOffline,			///	Future -- the given device became unavailable for use.
		AJA_DeviceGoingToSleep,		///	Future -- the given device went to sleep (entered a low-power state).
		AJA_DeviceWakingUp			///	Future -- the given device woke up (left its low-power state).

	} AJADeviceNotificationMessage;


	/**
		@brief	An interface name is simply a character string that names an interface that's supported by an AJA device.
				Currently, there are only two:  "NTV2" and "NTV4".
	**/
	typedef std::string				AJAInterfaceName;


	extern const std::string &		AJAInterfaceName_NTV2;		///	This identifies the "NTV2" interface
	extern const std::string &		AJAInterfaceName_NTV4;		///	This identifies the "NTV4" interface


	/**
		@brief	Converts the given AJAPnpMessage into a human-readable string.
		@param[in]	inMessage	Specifies the AJAPnpMessage to be converted.
		@return		A string containing the given message in human-readable form.
	**/
	std::string AJADeviceNotificationMessageToString (const AJADeviceNotificationMessage inMessage);


	/**
		@brief	A device identifier is simply a character string that can be used to persistently and
				uniquely identify an AJA device even across multiple hosts.
				Currently, the format of the string is as follows:
				-	NTV2 devices:	"0x" + (8-character lower-case hexadecimal board ID) + "/" + (16-character lower-case hexadecimal board serial number)
				-	NTV4 devices:	TBD
	**/
	typedef std::string				AJADeviceIdentifier;


	/**
		@brief	I maintain communications with an AJA hardware device via a platform-specific device driver.
				My subclasses implement the requisite platform-specific behavior.
	**/
	class AJADevice;
	typedef AJARefPtr <AJADevice>	AJADevicePtr;


	class AJADevice
	{
		//	Class Methods
		public:
			/**
				@brief	Returns the given 32-bit device ID (aka board ID) and 64-bit serial number encoded into
						an AJADeviceIdentifier.
				@param[in]	inDeviceID		Specifies the 32-bit device ID (aka board ID) to be encoded.
				@param[in]	inSerialNumber	Specifies the device's 64-bit serial number to be encoded.
				@return		If successful, an AJADeviceIdentifier encoded with the given device ID and serial number;
							otherwise an empty string.
			**/
			static AJADeviceIdentifier					EncodeDeviceIdentifier (const uint32_t inDeviceID, const uint64_t inSerialNumber);


			/**
				@brief	Decodes the 32-bit device ID and 64-bit serial number from the given AJADeviceIdentifer.
				@param[in]		inDeviceID			Specifies the AJADeviceIdentifier to be decoded into its device ID and serial number
													components.
				@param[out]		outDeviceID			Receives the 32-bit device ID (aka board ID) decoded from the AJADeviceIdentifier.
				@param[out]		outSerialNumber		Receives the 64-bit serial number decoded from the AJADeviceIdentifier.
				@return			True if successful;  otherwise false.
			**/
			static bool									DecodeDeviceIdentifier (const AJADeviceIdentifier & inDeviceID, uint32_t & outDeviceID, uint64_t outSerialNumber);


			/**
				@brief	Decodes only the 32-bit device ID from the given AJADeviceIdentifer.
				@param[in]		inDeviceID			Specifies the AJADeviceIdentifier to be decoded.
				@param[out]		outDeviceID			Receives the 32-bit device ID (aka board ID) decoded from the AJADeviceIdentifier.
				@return			True if successful;  otherwise false.
			**/
			static bool									DecodeDeviceID (const AJADeviceIdentifier & inDeviceID, uint32_t & outDeviceID);


			/**
				@brief	Decodes the 64-bit serial number from the given AJADeviceIdentifer.
				@param[in]		inDeviceID			Specifies the AJADeviceIdentifier to be decoded.
				@param[out]		outSerialNumber		Receives the 64-bit serial number decoded from the AJADeviceIdentifier.
				@return			True if successful;  otherwise false.
			**/
			static bool									DecodeSerialNumber (const AJADeviceIdentifier & inDeviceID, uint64_t outSerialNumber);


			/**
				@brief	Returns a character string that contains an encoded representation of the 32-bit device ID.
				@param[in]		inDeviceID			Specifies the 32-bit device ID to be encoded.
				@return			If successful, a character string that contains an encoded representation of the given device ID;  otherwise an empty string.
			**/
			static std::string							EncodeDeviceID (const uint32_t inDeviceID);


			/**
				@brief	Returns a character string that contains an encoded representation of the 64-bit serial number.
				@param[in]		inSerialNumber			Specifies the 64-bit serial number to be encoded.
				@return			If successful, a character string that contains an encoded representation of the given serial number;  otherwise an empty string.
			**/
			static std::string							EncodeSerialNumber (const uint64_t inSerialNumber);


		//	Instance Methods
		public:
			/**
				@brief		My destructor.
			**/
			virtual										~AJADevice ();

			/**
				@brief		Returns true if I'm open -- i.e., still connected, awake and powered on.
			**/
			virtual inline bool							IsOpen (void) const										{return false;}

			/**
				@brief		Returns a string containing my device class name.
			**/
			virtual inline const std::string &			GetDeviceClassName (void) const							{return mDeviceClassName;}

			/**
				@brief		Returns a string containing my interface name ("NTV2" or "NTV4").
			**/
			virtual inline const std::string &			GetInterfaceName (void) const							{return mInterfaceName;}

			/**
				@brief		Returns true if I am an NTV2 device.
			**/
			virtual inline bool							HasNTV2Interface (void) const							{return GetInterfaceName () == AJAInterfaceName_NTV2;}

			/**
				@brief		Returns true if I am an NTV4 device.
			**/
			virtual inline bool							HasNTV4Interface (void) const							{return GetInterfaceName () == AJAInterfaceName_NTV4;}

			/**
				@brief		Returns my unique device identifier.
			**/
			virtual inline const AJADeviceIdentifier &	GetIdentifier (void) const								{return mIdentifierKey;}

			/**
				@brief		Streams my current state in a human-readable format to the given output stream.
			**/
			virtual std::ostream &						Print (std::ostream & inOutStr) const;

			/**
				@brief		This gets called if/when I am powered off or disconnected. Clients should never call this method.
			**/
			virtual void								pvt_Terminated (void);

		protected:
			explicit inline								AJADevice ()											{assert (false);}
			explicit									AJADevice (const std::string & inDeviceClassName);
		private:
			inline										AJADevice (const AJADevice & inObj)						{if (&inObj != this) assert (false);}
			inline AJADevice &							operator = (const AJADevice & inRHS)					{if (&inRHS != this) assert (false); return *this;}
			virtual bool								NTV2ReadRegister (const uint64_t inRegisterNumber, uint32_t & outRegisterValue, const uint64_t inRegisterMask = 0xFFFFFFFFFFFFFFFF) const;
			virtual bool								NTV4SendMessage (void * pInMessage) const;


		//	Instance Data
		protected:
			const std::string							mDeviceClassName;	///	My name
			std::string									mInterfaceName;		///	Interface name (e.g., "NTV2", "NTV4", etc.)
			AJADeviceIdentifier							mIdentifierKey;		///	My identifier key

	};	//	AJADevice


	std::ostream &	operator << (std::ostream & inOutStr, const AJADevice & inObj);
	std::ostream &	operator << (std::ostream & inOutStr, const AJADevicePtr & inObj);


	/**
		@brief	I'm the function prototype for the "device attached" and "device detached" callbacks.
		@param[in]	inMessage	Specifies what happened -- e.g., AJA_DeviceAdded, AJA_DeviceRemoved, etc.
		@param[in]	inRefCon	Specifies the "refCon" value that was specified when the callback function was installed.
		@param[in]	inDevice	Specifies the affected AJADevice.
	**/
	typedef void (*AJADeviceNotificationCallback) (const AJADeviceNotificationMessage inMessage, void * refcon, AJADevicePtr inDevice);

#endif	//	AJADEVICE_H
