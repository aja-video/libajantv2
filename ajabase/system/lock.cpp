/* SPDX-License-Identifier: MIT */
/**
	@file		lock.cpp
	@brief		Implements the AJALock class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/lock.h"
#if defined(AJA_USE_CPLUSPLUS11) && !defined(AJA_BAREMETAL)
	#include <chrono>
	using namespace std;
#else
	// include the system dependent implementation class
	#if defined(AJA_WINDOWS)
		#include "ajabase/system/windows/lockimpl.h"
	#elif defined(AJA_LINUX)
		#include "ajabase/system/linux/lockimpl.h"
	#elif defined(AJA_MAC)
		#include "ajabase/system/mac/lockimpl.h"
	#elif defined(AJA_BAREMETAL)
		#include "ajabase/system/bm/lockimpl.h"
	#endif
#endif


AJALock::AJALock(const char* pName)
{
#if defined(AJA_USE_CPLUSPLUS11) && !defined(AJA_BAREMETAL)
	mpMutex = new recursive_timed_mutex;
	if (pName != nullptr)
		mName = pName;
#else
	mpImpl = NULL;
	mpImpl = new AJALockImpl(pName);
#endif
}

AJALock::AJALock (const AJALock & inLock)
{	//	Copy constructor -- only name is copied...
#if defined(AJA_USE_CPLUSPLUS11) && !defined(AJA_BAREMETAL)
	mpMutex = new recursive_timed_mutex;
	mName = inLock.mName;
#else
	mpImpl = NULL;
	mpImpl = new AJALockImpl(NULL);	//	FOR NOW, NAME NOT COPIED -- TBD:  inLock.mpImpl->mName);
#endif
}

AJALock & AJALock::operator = (const AJALock & inLock)
{	//	Assignment operator -- no-op
	(void) inLock;
	return *this;
}


AJALock::~AJALock()
{
#if defined(AJA_USE_CPLUSPLUS11) && !defined(AJA_BAREMETAL)
	delete mpMutex;
	mpMutex = nullptr;
#else
	if(mpImpl)
		delete mpImpl;
#endif
}

// interface to the implementation class

AJAStatus
AJALock::Lock(uint32_t timeout)
{
#if defined(AJA_USE_CPLUSPLUS11) && !defined(AJA_BAREMETAL)
	if (timeout != LOCK_TIME_INFINITE)
	{
		bool success = mpMutex->try_lock_for(std::chrono::milliseconds(timeout));
		return success ? AJA_STATUS_SUCCESS : AJA_STATUS_TIMEOUT;
	}
	else
	{
		mpMutex->lock();
		return AJA_STATUS_SUCCESS;
	}
#else
	return mpImpl->Lock(timeout);
#endif
}


AJAStatus
AJALock::Unlock()
{
#if defined(AJA_USE_CPLUSPLUS11) && !defined(AJA_BAREMETAL)
	mpMutex->unlock();
	return AJA_STATUS_SUCCESS;
#else
	return mpImpl->Unlock();
#endif
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
