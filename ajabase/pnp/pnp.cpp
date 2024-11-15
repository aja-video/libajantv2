/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/pnp.cpp
	@brief		Implements the AJAPnp (plug-and-play) class.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include <assert.h>
#include "ajabase/pnp/pnp.h"

#if defined(AJA_WINDOWS)
	#include "ajabase/pnp/windows/pnpimpl.h"
#endif

#if defined(AJA_LINUX)
	#include "ajabase/pnp/linux/pnpimpl.h"
#endif

#if defined(AJA_MAC)
	#include "ajabase/pnp/mac/pnpimpl.h"
#endif


std::string AJAPnp::MessageToString (const AJAPnpMessage inMsg)		//	STATIC
{
	switch (inMsg)
	{
		case AJA_Pnp_DeviceAdded:			return "AJA_Pnp_DeviceAdded";
		case AJA_Pnp_DeviceRemoved:			return "AJA_Pnp_DeviceRemoved";
		case AJA_Pnp_DeviceOnline:			return "AJA_Pnp_DeviceOnline";
		case AJA_Pnp_DeviceOffline:			return "AJA_Pnp_DeviceOffline";
		case AJA_Pnp_DeviceGoingToSleep:	return "AJA_Pnp_DeviceGoingToSleep";
		case AJA_Pnp_DeviceWakingUp:		return "AJA_Pnp_DeviceWakingUp";
	}
	return "";
}

AJAPnp::AJAPnp()
{
	mpImpl = new AJAPnpImpl();
}


AJAPnp::~AJAPnp()
{
	delete mpImpl;
}


AJAStatus 
AJAPnp::Install(AJAPnpCallback callback, void* refCon, uint32_t devices)
{
	return mpImpl->Install(callback, refCon, devices);
}


AJAStatus
AJAPnp::Uninstall()
{
	return mpImpl->Uninstall();
}


AJAPnpCallback 
AJAPnp::GetCallback() const
{
	return mpImpl->GetCallback();
}


void* 
AJAPnp::GetRefCon() const
{
	return mpImpl->GetRefCon();
}


uint32_t
AJAPnp::GetPnpDevices() const
{
	return mpImpl->GetPnpDevices();
}


AJAPnp::AJAPnp (const AJAPnp & inObjToCopy)
{
	AJA_UNUSED(inObjToCopy); assert (false && "hidden copy constructor");	//	mpImpl=inObjToCopy.mpImpl;
}


AJAPnp &
AJAPnp::operator= (const AJAPnp & inObjToCopy)
{
	if (&inObjToCopy != this)
		assert (false && "hidden assignment operator"); //	mpImpl = inObjToCopy.mpImpl;

	return *this;

}	//	copy constructor
