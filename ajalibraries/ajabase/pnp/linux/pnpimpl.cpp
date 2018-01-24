/**
	@file		pnp/linux/pnpimpl.cpp
	@copyright	Copyright (C) 2011-2018 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAPnpImpl class on the Linux platform.
**/

#include "ajabase/pnp/linux/pnpimpl.h"


AJAPnpImpl::AJAPnpImpl() : mRefCon(NULL), mCallback(NULL), mDevices(0)
{
}


AJAPnpImpl::~AJAPnpImpl()
{
	Uninstall();
}


AJAStatus 
AJAPnpImpl::Install(AJAPnpCallback callback, void* refCon, uint32_t devices)
{
	mCallback = callback;
	mRefCon = refCon;
	mDevices = devices;
	
	if (mCallback)
		(*(mCallback))(AJA_Pnp_DeviceAdded, mRefCon);
	
	return AJA_STATUS_SUCCESS;
}
	

AJAStatus 
AJAPnpImpl::Uninstall(void)
{
	mCallback = NULL;
	mRefCon = NULL;
	mDevices = 0;
	
	return AJA_STATUS_SUCCESS;
}


AJAPnpCallback 
AJAPnpImpl::GetCallback()
{
	return mCallback;
}


void* 
AJAPnpImpl::GetRefCon()
{
	return mRefCon;
}


uint32_t
AJAPnpImpl::GetPnpDevices()
{
	return mDevices;
}



