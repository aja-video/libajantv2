/* SPDX-License-Identifier: MIT */
/**
	@file		atomic.cpp
	@brief		Implements the AJAAtomic class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/atomic.h"


void* AJAAtomic::Exchange(void* volatile* pTarget, void* pValue)
{
	// exchange pointer value with target
#if defined(AJA_WINDOWS)
	#pragma warning(push)
	#pragma warning(disable: 4311)
	#pragma warning(disable: 4312)
	return (void*)InterlockedExchangePointer((void* volatile*)pTarget, (void*)pValue);
	#pragma warning(pop)
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_lock_test_and_set(pTarget, pValue);
#endif
}


int32_t AJAAtomic::Exchange(int32_t volatile* pTarget, int32_t value)
{
	// exchange integer value with target
#if defined(AJA_WINDOWS)
	return (int32_t)InterlockedExchange((LONG volatile*)pTarget, (LONG)value);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_lock_test_and_set(pTarget, value);
#endif
}


uint32_t AJAAtomic::Exchange(uint32_t volatile* pTarget, uint32_t value)
{
	// exchange unsigned integer value with target
#if defined(AJA_WINDOWS)
	return (uint32_t)InterlockedExchange((LONG volatile*)pTarget, (LONG)value);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_lock_test_and_set(pTarget, value);
#endif
}


int64_t AJAAtomic::Exchange(int64_t volatile* pTarget, int64_t value)
{
	// exchange integer value with target
#if defined(AJA_WINDOWS)
	return (int64_t)InterlockedExchange64((LONGLONG volatile*)pTarget, (LONGLONG)value);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_lock_test_and_set(pTarget, value);
#endif
}


uint64_t AJAAtomic::Exchange(uint64_t volatile* pTarget, uint64_t value)
{
	// exchange unsigned integer value with target
#if defined(AJA_WINDOWS)
	return (uint64_t)InterlockedExchange64((LONGLONG volatile*)pTarget, (LONGLONG)value);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_lock_test_and_set(pTarget, value);
#endif
}


int32_t AJAAtomic::Increment(int32_t volatile* pTarget)
{
	// increment target
#if defined(AJA_WINDOWS)
	return (int32_t)InterlockedIncrement((LONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_add_and_fetch(pTarget, 1);
#endif
}


int32_t AJAAtomic::Decrement(int32_t volatile* pTarget)
{
	// decrement target
#if defined(AJA_WINDOWS)
	return (int32_t)InterlockedDecrement((LONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_sub_and_fetch(pTarget, 1);
#endif
}


uint32_t AJAAtomic::Increment(uint32_t volatile* pTarget)
{
	// increment target
#if defined(AJA_WINDOWS)
	return (uint32_t)InterlockedIncrement((LONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_add_and_fetch(pTarget, 1);
#endif
}


uint32_t AJAAtomic::Decrement(uint32_t volatile* pTarget)
{
	// decrement target
#if defined(AJA_WINDOWS)
	return (uint32_t)InterlockedDecrement((LONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_sub_and_fetch(pTarget, 1);
#endif
}


int64_t AJAAtomic::Increment(int64_t volatile* pTarget)
{
	// increment target
#if defined(AJA_WINDOWS)
	return (int64_t)InterlockedIncrement64((LONGLONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_add_and_fetch(pTarget, 1);
#endif
}


int64_t AJAAtomic::Decrement(int64_t volatile* pTarget)
{
	// decrement target
#if defined(AJA_WINDOWS)
	return (int64_t)InterlockedDecrement64((LONGLONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_sub_and_fetch(pTarget, 1);
#endif
}


uint64_t AJAAtomic::Increment(uint64_t volatile* pTarget)
{
	// increment target
#if defined(AJA_WINDOWS)
	return (uint64_t)InterlockedIncrement64((LONGLONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_add_and_fetch(pTarget, 1);
#endif
}


uint64_t AJAAtomic::Decrement(uint64_t volatile* pTarget)
{
	// decrement target
#if defined(AJA_WINDOWS)
	return (uint64_t)InterlockedDecrement64((LONGLONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_sub_and_fetch(pTarget, 1);
#endif
}
