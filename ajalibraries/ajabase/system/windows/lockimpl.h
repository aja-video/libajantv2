/**
	@file		windows/lockimpl.h
	@copyright	Copyright (C) 2009-2019 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJALockImpl class.
**/

#ifndef AJA_LOCK_IMPL_H
#define AJA_LOCK_IMPL_H

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/lock.h"


class AJALockImpl
{
public:

	AJALockImpl(const char* pName);
	virtual ~AJALockImpl();

	AJAStatus Lock(uint32_t uTimeout = 0xffffffff);
	AJAStatus Unlock();

	HANDLE mMutex;
};

#endif	//	AJA_LOCK_IMPL_H
