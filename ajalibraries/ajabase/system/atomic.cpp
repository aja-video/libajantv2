/**
	@file		atomic.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAAtomic class.
**/

#if defined(AJA_MAC)
#include <libkern/OSAtomic.h>
#endif


#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/atomic.h"


AJAAtomic::AJAAtomic()
{
}


AJAAtomic::~AJAAtomic()
{
}


void* 
AJAAtomic::Exchange(void* volatile* pTarget, void* pValue)
{
	// exchange pointer value with target
#if defined(AJA_WINDOWS)
	#pragma warning(push)
	#pragma warning(disable: 4311)
	#pragma warning(disable: 4312)
	return (void*)InterlockedExchangePointer((void* volatile*)pTarget, (void*)pValue);
	#pragma warning(pop)
#endif

#if defined(AJA_LINUX)
	return __sync_lock_test_and_set(pTarget, pValue);
#endif
	
#if defined(AJA_MAC)
	OSAtomicCompareAndSwapPtrBarrier( *pTarget, pValue, pTarget );
	return *pTarget;
#endif
}


int32_t 
AJAAtomic::Exchange(int32_t volatile* pTarget, int32_t value)
{
	// exchange integer value with target
#if defined(AJA_WINDOWS)
	return (int32_t)InterlockedExchange((LONG volatile*)pTarget, (LONG)value);
#endif

#if defined(AJA_LINUX)
	return __sync_lock_test_and_set(pTarget, value);
#endif
	
#if defined(AJA_MAC)
	OSAtomicCompareAndSwap32Barrier( *pTarget, value, pTarget );
	return *pTarget;
#endif
}


uint32_t 
AJAAtomic::Exchange(uint32_t volatile* pTarget, uint32_t value)
{
	// exchange unsigned integer value with target
#if defined(AJA_WINDOWS)
	return (uint32_t)InterlockedExchange((LONG volatile*)pTarget, (LONG)value);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
	return __sync_lock_test_and_set(pTarget, value);
#endif
}


int64_t
AJAAtomic::Exchange(int64_t volatile* pTarget, int64_t value)
{
    // exchange integer value with target
#if defined(AJA_WINDOWS)
    return (int64_t)InterlockedExchange64((LONGLONG volatile*)pTarget, (LONGLONG)value);
#endif

#if defined(AJA_LINUX)
    return __sync_lock_test_and_set(pTarget, value);
#endif

#if defined(AJA_MAC)
    OSAtomicCompareAndSwap64Barrier( *pTarget, value, pTarget );
    return *pTarget;
#endif
}


uint64_t
AJAAtomic::Exchange(uint64_t volatile* pTarget, uint64_t value)
{
    // exchange unsigned integer value with target
#if defined(AJA_WINDOWS)
    return (uint64_t)InterlockedExchange64((LONGLONG volatile*)pTarget, (LONGLONG)value);
#endif

#if defined(AJA_LINUX) || defined(AJA_MAC)
    return __sync_lock_test_and_set(pTarget, value);
#endif
}


int32_t 
AJAAtomic::Increment(int32_t volatile* pTarget)
{
	// increment target
#if defined(AJA_WINDOWS)
	return (int32_t)InterlockedIncrement((LONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX)
	return __sync_add_and_fetch(pTarget, 1);
#endif

#if defined(AJA_MAC)
	return OSAtomicIncrement32Barrier(pTarget);
#endif
}


int32_t 
AJAAtomic::Decrement(int32_t volatile* pTarget)
{
	// decrement target
#if defined(AJA_WINDOWS)
	return (int32_t)InterlockedDecrement((LONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX)
	return __sync_sub_and_fetch(pTarget, 1);
#endif

#if defined(AJA_MAC)
	return OSAtomicDecrement32Barrier(pTarget);
#endif
}


uint32_t 
AJAAtomic::Increment(uint32_t volatile* pTarget)
{
	// increment target
#if defined(AJA_WINDOWS)
	return (uint32_t)InterlockedIncrement((LONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX)
	return __sync_add_and_fetch(pTarget, 1);
#endif

#if defined(AJA_MAC)
	return (uint32_t) OSAtomicIncrement32Barrier((int32_t *)pTarget);
#endif
}


uint32_t 
AJAAtomic::Decrement(uint32_t volatile* pTarget)
{
	// decrement target
#if defined(AJA_WINDOWS)
	return (uint32_t)InterlockedDecrement((LONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX)
	return __sync_sub_and_fetch(pTarget, 1);
#endif

#if defined(AJA_MAC)
	return (uint32_t) OSAtomicDecrement32Barrier((int32_t *)pTarget);
#endif
}


int64_t
AJAAtomic::Increment(int64_t volatile* pTarget)
{
    // increment target
#if defined(AJA_WINDOWS)
    return (int64_t)InterlockedIncrement64((LONGLONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX)
    return __sync_add_and_fetch(pTarget, 1);
#endif

#if defined(AJA_MAC)
    return OSAtomicIncrement64Barrier(pTarget);
#endif
}


int64_t
AJAAtomic::Decrement(int64_t volatile* pTarget)
{
    // decrement target
#if defined(AJA_WINDOWS)
    return (int64_t)InterlockedDecrement64((LONGLONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX)
    return __sync_sub_and_fetch(pTarget, 1);
#endif

#if defined(AJA_MAC)
    return OSAtomicDecrement64Barrier(pTarget);
#endif
}


uint64_t
AJAAtomic::Increment(uint64_t volatile* pTarget)
{
    // increment target
#if defined(AJA_WINDOWS)
    return (uint64_t)InterlockedIncrement64((LONGLONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX)
    return __sync_add_and_fetch(pTarget, 1);
#endif

#if defined(AJA_MAC)
    return (uint64_t) OSAtomicIncrement64Barrier((int64_t *)pTarget);
#endif
}


uint64_t
AJAAtomic::Decrement(uint64_t volatile* pTarget)
{
    // decrement target
#if defined(AJA_WINDOWS)
    return (uint64_t)InterlockedDecrement64((LONGLONG volatile*)pTarget);
#endif

#if defined(AJA_LINUX)
    return __sync_sub_and_fetch(pTarget, 1);
#endif

#if defined(AJA_MAC)
    return (uint64_t) OSAtomicDecrement64Barrier((int64_t *)pTarget);
#endif
}