/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/windows/pnpimpl.cpp
	@brief		Implements the AJAPnpImpl class on the Windows platform.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/pnp/windows/pnpimpl.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/debug.h"

#define PNPFAIL(__x__) AJA_sREPORT(AJA_DebugUnit_PnP, AJA_DebugSeverity_Error,		__func__ << ": " << __x__)
#define PNPWARN(__x__) AJA_sREPORT(AJA_DebugUnit_PnP, AJA_DebugSeverity_Warning,	__func__ << ": " << __x__)
#define PNPNOTE(__x__) AJA_sREPORT(AJA_DebugUnit_PnP, AJA_DebugSeverity_Notice,		__func__ << ": " << __x__)
#define PNPINFO(__x__) AJA_sREPORT(AJA_DebugUnit_PnP, AJA_DebugSeverity_Info,		__func__ << ": " << __x__)
#define PNPDBG(__x__)  AJA_sREPORT(AJA_DebugUnit_PnP, AJA_DebugSeverity_Debug,		__func__ << ": " << __x__)


static void CALLBACK SignaledAddRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	AJAPnpImpl *context = (AJAPnpImpl*)lpParam;
	context->AddSignaled();

}

static void CALLBACK SignaledRemoveRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
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


AJAStatus AJAPnpImpl::Install(AJAPnpCallback callback, void* refCon, uint32_t devices)
{
	mCallback = callback;
	mRefCon   = refCon;
	mDevices  = devices;

	if (!mCallback)
		{PNPFAIL("NULL callback");  return AJA_STATUS_NULL;}

	//	Windows only handles PCIe devices
	if (mDevices & AJA_Pnp_PciVideoDevices)
	{
		mAddEventHandle = CreateEventW(NULL, FALSE, FALSE, L"Global\\AJAPNPAddEvent");
		if (!mAddEventHandle)
			{PNPFAIL("NULL handle from CreateEventW for 'Global\\AJAPNPAddEvent'");  return AJA_STATUS_FAIL;}
		if (!RegisterWaitForSingleObject(&mAddWaitHandle, mAddEventHandle, (WAITORTIMERCALLBACK)&SignaledAddRoutine, this, INFINITE, NULL))
			{PNPFAIL("RegisterWaitForSingleObject failed for 'Global\\AJAPNPAddEvent'");  return AJA_STATUS_FAIL;}

		mRemoveEventHandle = CreateEventW(NULL, FALSE, FALSE, L"Global\\AJAPNPRemoveEvent");
		if (!mRemoveEventHandle)
			{PNPFAIL("NULL handle from CreateEventW for 'Global\\AJAPNPRemoveEvent'");  return AJA_STATUS_FAIL;}
		if (!RegisterWaitForSingleObject(&mRemoveWaitHandle, mRemoveEventHandle, (WAITORTIMERCALLBACK)&SignaledRemoveRoutine, this, INFINITE, NULL))
			{PNPFAIL("RegisterWaitForSingleObject failed for 'Global\\AJAPNPRemoveEvent'");  return AJA_STATUS_FAIL;}

		PNPINFO("Callback installation succeeded");
		return AJA_STATUS_SUCCESS;
	}
	PNPFAIL("'AJA_Pnp_PciVideoDevices' not set, AJAPnp callback not installed");
	return AJA_STATUS_FAIL;	//	Nothing installed
}
	
AJAStatus AJAPnpImpl::Uninstall(void)
{
	mCallback = NULL;
	mRefCon = NULL;
	mDevices = 0;
	
	if (mAddWaitHandle || mRemoveWaitHandle)
		PNPINFO("Callback uninstalled");
	if (mAddWaitHandle != NULL)
	{
		UnregisterWait(mAddWaitHandle);
		mAddWaitHandle = NULL;
	}
	if (mRemoveWaitHandle != NULL)
	{
		UnregisterWait(mRemoveWaitHandle);
		mRemoveWaitHandle = NULL;
	}
	return AJA_STATUS_SUCCESS;
}

void AJAPnpImpl::AddSignaled()
{
	PNPNOTE("'Device added' signaled");
	AJATime::Sleep(1000);
 	if (mCallback)
 		(*(mCallback))(AJA_Pnp_DeviceAdded, mRefCon);

	RegisterWaitForSingleObject(&mAddWaitHandle, mAddEventHandle, (WAITORTIMERCALLBACK)SignaledAddRoutine, this, INFINITE, NULL);
}

void AJAPnpImpl::RemoveSignaled()
{
	PNPNOTE("'Device removed' signaled");
	if (mCallback)
		(*(mCallback))(AJA_Pnp_DeviceRemoved, mRefCon);

	RegisterWaitForSingleObject(&mRemoveWaitHandle, mRemoveEventHandle, (WAITORTIMERCALLBACK)SignaledRemoveRoutine, this, INFINITE, NULL);
}
