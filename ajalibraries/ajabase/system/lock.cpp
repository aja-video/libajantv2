/**
	@file		lock.cpp
	@copyright	Copyright (C) 2009-2019 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJALock class.
**/

// include the system dependent implementation class
#if defined(AJA_WINDOWS)
	#include "ajabase/system/windows/lockimpl.h"
#endif

#if defined(AJA_LINUX)
	#include "ajabase/system/linux/lockimpl.h"
#endif

#if defined(AJA_MAC)
	#include "ajabase/system/mac/lockimpl.h"
#endif

AJALock::AJALock(const char* pName)
{
	mpImpl = NULL;
	// create the implementation class
	mpImpl = new AJALockImpl(pName);
}


AJALock::~AJALock()
{
	if(mpImpl)
		delete mpImpl;
}

// interface to the implementation class

AJAStatus
AJALock::Lock(uint32_t timeout)
{
	return mpImpl->Lock(timeout);
}


AJAStatus
AJALock::Unlock()
{
	return mpImpl->Unlock();
}


AJAAutoLock::AJAAutoLock(AJALock* pLock)
{
	mpLock = pLock;
	if (mpLock)
	{
		mpLock->Lock();
	}
}


AJAAutoLock::~AJAAutoLock()
{
	if (mpLock)
	{
		mpLock->Unlock();
	}
}
