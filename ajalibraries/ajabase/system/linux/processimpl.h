/**
	@file		linux/processimpl.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJAProcessImpl class.
**/

#ifndef AJA_PROCESS_IMPL_H
#define AJA_PROCESS_IMPL_H

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/process.h"


class AJAProcessImpl
{
public:

	AJAProcessImpl();
	virtual ~AJAProcessImpl();

static		uint64_t	GetPid();
static		bool		IsValid(uint64_t pid);
static		bool		Activate(uint64_t handle);
static		bool		Activate(const char* pWindow);
};

#endif	//	AJA_PROCESS_IMPL_H
