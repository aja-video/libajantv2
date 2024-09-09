/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/pnp.h
	@brief		Declares the AJAPnp (plug-and-play) class.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
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


/** 
	@brief		If installed (see AJAPnp::Install) in an AJAPnp instance, this function is called when
				an AJA device is attached/detached, powered on/off, etc.
	@param[in]	inMessage	Specifies the message (i.e., added, removed, etc.).
	@param[in]	inRefCon	Specifies the reference cookie that was passed to AJAPnp::Install.
**/
typedef void (*AJAPnpCallback)(AJAPnpMessage inMessage, void * inRefCon);


// forward declarations.
class AJAPnpImpl;

/** 
	@brief		This is a platform-agnostic plug-and-play class that notifies a client when AJA devices are
				attached/detached, powered on/off, sleep/wake, etc.
	@ingroup	AJAGroupPnp
**/
class AJA_EXPORT AJAPnp
{
public:	//	INSTANCE METHODS

	/**
	 *	@brief	Default constructor.
	 */
	AJAPnp();

	/**
	 *	@brief	Default destructor.
	 */
	virtual ~AJAPnp();

	/**
	 *  @brief		Installs the given plug & play notification callback function, replacing any callback function that
	 *				may have been installed earlier.
	 *
	 *	@param[in]	pInCallback		Specifies a valid (non-NULL) pointer to a client-defined function to be called when
	 *								AJA devices are attached/detached to/from the host.
	 *	@param[in]	inRefCon		Optionally specifies a pointer-sized reference value that gets passed to the callback
	 *								function. Defaults to zero.
	 *	@param[in]	inDeviceMask	Optionally specifies a bit mask that filters the types of devices to include/ignore
	 *								(see AJAPnpDevice). Defaults to no filtering (i.e. notify for any/all device types).
	 *
	 *	@note		On macOS, the callback function is immediately called for each currently-attached matching device.
	 *
	 *	@bug		macOS: doesn't work on programs that don't provide a run loop.
	 *
	 *	@bug		Windows: doesn't work if the calling process is a Windows service.
	 *
	 *	@return		AJA_STATUS_SUCCESS		Install succeeded
	 *				AJA_STATUS_FAIL			Install failed
	 */
	virtual AJAStatus Install (AJAPnpCallback pInCallback, void * inRefCon = NULL, uint32_t inDeviceMask = 0xFFFFFFFF);

	/**
	 *	@return		the address of the currently-installed callback (NULL if none installed).
	 */
	virtual AJAPnpCallback GetCallback() const;

	/**
	 *	@brief		Uninstalls any previously-installed callback notifier.
	 *	@return		AJA_STATUS_SUCCESS		Uninstall succeeded
	 *				AJA_STATUS_FAIL			Uninstall failed
	 */
	virtual AJAStatus Uninstall();

	/**
	 *	@return		the currently installed reference cookie.
	 */
	virtual void* GetRefCon() const;

	/**
	 *	@return		the current bit mask that filters which devices to include or ignore (see implementation).
	 */
	virtual uint32_t GetPnpDevices() const;


private:	//	INSTANCE METHODS
	/**
	 *	@brief		Hidden copy constructor.
	 *
	 *	@param[in]	inObjToCopy		Specifies the object to be copied.
	**/
	AJAPnp (const AJAPnp & inObjToCopy);


	/**
	 *	@brief		Hidden assignment operator.
	 *
	 *	@param[in]	inObjToCopy		Specifies the object to be copied.
	**/
	virtual AJAPnp &	operator= (const AJAPnp & inObjToCopy);


private:	//	INSTANCE DATA
	AJAPnpImpl *	mpImpl;		///< @brief	My platform-specific implementation object

};	//	AJAPnp

#endif	//	AJA_PNP_H
