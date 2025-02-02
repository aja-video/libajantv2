/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/windows/pnpimpl.cpp
	@brief		Implements the AJAPnpImpl class on the Windows platform.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/pnp/windows/pnpimpl.h"
#include "ajabase/system/systemtime.h"

void
CALLBACK SignaledAddRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	AJAPnpImpl *context = (AJAPnpImpl*)lpParam;
	context->AddSignaled();

}

void
CALLBACK SignaledRemoveRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	AJAPnpImpl *context = (AJAPnpImpl*)lpParam;
	context->RemoveSignaled();

}

AJAPnpImpl::AJAPnpImpl() : mRefCon(NULL), mCallback(NULL), mDevices(0), mAddEventHandle(NULL), mAddWaitHandle(NULL), mRemoveEventHandle(NULL), mRemoveWaitHandle(NULL)
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
	mRefCon   = refCon;
	mDevices  = devices;

	if (!mCallback)
		return AJA_STATUS_NULL;	//	NULL callback

	//	Windows only handles PCIe devices
	if (mDevices & AJA_Pnp_PciVideoDevices)
	{
		mAddEventHandle = CreateEventW(NULL, FALSE, FALSE, L"Global\\AJAPNPAddEvent");
		if (!mAddEventHandle)
			return AJA_STATUS_FAIL;
		if (!RegisterWaitForSingleObject(&mAddWaitHandle, mAddEventHandle, (WAITORTIMERCALLBACK)&SignaledAddRoutine, this, INFINITE, NULL))
			return AJA_STATUS_FAIL;

		mRemoveEventHandle = CreateEventW(NULL, FALSE, FALSE, L"Global\\AJAPNPRemoveEvent");
		if (!mRemoveEventHandle)
			return AJA_STATUS_FAIL;
		if (!RegisterWaitForSingleObject(&mRemoveWaitHandle, mRemoveEventHandle, (WAITORTIMERCALLBACK)&SignaledRemoveRoutine, this, INFINITE, NULL))
			return AJA_STATUS_FAIL;

		return AJA_STATUS_SUCCESS;
	}
	return AJA_STATUS_FAIL;	//	Nothing installed
}
	
AJAStatus 
AJAPnpImpl::Uninstall(void)
{
	mCallback = NULL;
	mRefCon = NULL;
	mDevices = 0;
	
	if(mAddWaitHandle != NULL)
	{
		UnregisterWait(mAddWaitHandle);
		mAddWaitHandle = NULL;
	}
	if(mRemoveWaitHandle != NULL)
	{
		UnregisterWait(mRemoveWaitHandle);
		mRemoveWaitHandle = NULL;
	}
	
	return AJA_STATUS_SUCCESS;
}

void
AJAPnpImpl::AddSignaled()
{
	AJATime::Sleep(1000);
 	if (mCallback)
 		(*(mCallback))(AJA_Pnp_DeviceAdded, mRefCon);

	RegisterWaitForSingleObject(&mAddWaitHandle, mAddEventHandle, (WAITORTIMERCALLBACK)SignaledAddRoutine, this, INFINITE, NULL);
}

void
AJAPnpImpl::RemoveSignaled()
{
	if (mCallback)
		(*(mCallback))(AJA_Pnp_DeviceRemoved, mRefCon);

	RegisterWaitForSingleObject(&mRemoveWaitHandle, mRemoveEventHandle, (WAITORTIMERCALLBACK)SignaledRemoveRoutine, this, INFINITE, NULL);
}
