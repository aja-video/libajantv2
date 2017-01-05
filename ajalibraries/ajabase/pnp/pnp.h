/**
	@file		pnp/pnp.h
	@copyright	Copyright (C) 2011-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJAPnp (plug-and-play) class.
**/

#ifndef AJA_PNP_H
#define AJA_PNP_H

#include "ajabase/common/public.h"


typedef enum 
{
	AJA_Pnp_PciVideoDevices		= (1 << 0),
	AJA_Pnp_UsbSerialDevices	= (1 << 1),
	AJA_Pnp_FireWireDevices		= (1 << 2)
} AJAPnpDevice;


typedef enum 
{
	AJA_Pnp_DeviceAdded,
	AJA_Pnp_DeviceRemoved,
	AJA_Pnp_DeviceOnline,
	AJA_Pnp_DeviceOffline,
	AJA_Pnp_DeviceGoingToSleep,
	AJA_Pnp_DeviceWakingUp
} AJAPnpMessage;


typedef void (*AJAPnpCallback)(AJAPnpMessage message, void * refcon);


// forward declarations.
class AJAPnpImpl;

/** 
 *	System independent plug-n-play class notifying client when AJA devices are
 *	added, removed, going to sleep or waking up.
 *	@ingroup AJAGroupPnp
 */
class AJA_EXPORT AJAPnp
{
public:	//	INSTANCE METHODS

	/**
	 *	Constructor create.
	 *
	 *	Plug-n-play object constructor. 
	 *
	 */
	AJAPnp();
	virtual ~AJAPnp();

	/**
	 *  Install plug-n-play notifier(s). Previous installs are automatically uninstalled.
	 *
	 *	@param[in]	callback		client defined callback used for plug-n-play notification. May be NULL.
	 *	@param[in]	refCon			reference cookie returned to client in callback.
	 *	@param[in]	devices			bitfield indicating which type of AJA device(s) are applicable (see AJAPnpDevice).
	 *
	 *	@return		AJA_STATUS_SUCCESS		Install succeeded
	 *				AJA_STATUS_FAIL			Install failed
	 */
	virtual AJAStatus Install(AJAPnpCallback callback, void* refCon= NULL, uint32_t devices = 0);

	/**
	 *	Return the currently installed callback.
	 *
	 *	@return		AJAPnpCallback			Currently installed callback, may be null
	 */
	virtual AJAPnpCallback GetCallback();

	/**
	 *	Uninstall plug-n-play callback notifier(s).
	 *	@return		AJA_STATUS_SUCCESS		Uninstall succeeded
	 *				AJA_STATUS_FAIL			Uninstall failed
	 */
	virtual AJAStatus Uninstall();

	/**
	 *	Return the currently installed reference cookie.
	 *
	 *	@return		void*					Currently installed reference cookie
	 */
	virtual void* GetRefCon();

	/**
	 *	Return bitfield of devices currently installed (see AJAPnpDevice)
	 *
	 *	@return		uint32_t				Currently installed Or'd bitfield of pnp devices (AJAPnpDevice)
	 */
	virtual uint32_t GetPnpDevices();


private:	//	INSTANCE METHODS
	/**
	 *	Hidden copy constructor.
	 *
	 *	@param[in]	inObjToCopy		Specifies the object to be copied.
	**/
	AJAPnp (const AJAPnp & inObjToCopy);


	/**
	 *	Hidden assignment operator.
	 *
	 *	@param[in]	inObjToCopy		Specifies the object to be copied.
	**/
	virtual AJAPnp &	operator= (const AJAPnp & inObjToCopy);


private:	//	INSTANCE DATA
	AJAPnpImpl *	mpImpl;	///	My AJAPnpImpl platform-specific implementation object

};	//	AJAPnp

#endif	//	AJA_PNP_H
