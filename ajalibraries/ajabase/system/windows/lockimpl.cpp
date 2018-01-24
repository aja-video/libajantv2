/**
	@file		windows/lockimpl.cpp
	@copyright	Copyright (C) 2009-2018 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJALockImpl class on the Windows platform.
**/

#include "ajabase/system/windows/lockimpl.h"
#include "ajabase/common/timer.h"


// class AJALockImpl

AJALockImpl::AJALockImpl(const char* pName)
{
	// initialize the windows lock
	mMutex = CreateMutex(NULL, FALSE, pName);
}


AJALockImpl::~AJALockImpl()
{
	// delete the windows lock
	if(mMutex != NULL)
	{
		CloseHandle(mMutex);
	}
}


AJAStatus
AJALockImpl::Lock(uint32_t timeout)
{
	if (mMutex == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	// check for infinite timeout
	if (timeout == 0xffffffff)
	{
		timeout = INFINITE;
	}

	// obtain the lock
	DWORD retCode = WaitForSingleObject(mMutex, (DWORD)timeout);

	// we got the lock
	if (retCode == WAIT_OBJECT_0)
	{
		return AJA_STATUS_SUCCESS;
	}
	// the wait timed out
	else if (retCode == WAIT_TIMEOUT)
	{
		return AJA_STATUS_TIMEOUT;
	}

	// some type of error occurred
	AJA_REPORT(0, AJA_DebugSeverity_Error, "AJALock::Lock WaitForSingleObject() returns error %08x", retCode);

	return AJA_STATUS_FAIL;
}


AJAStatus
AJALockImpl::Unlock()
{
	if (mMutex == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	// release the lock
	DWORD retCode = ReleaseMutex(mMutex);

	return AJA_STATUS_SUCCESS;
}
