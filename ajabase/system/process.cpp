/* SPDX-License-Identifier: MIT */
/**
	@file		process.cpp
	@brief		Implements the AJAProcess class.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/

// include the system dependent implementation class

#if defined(AJA_WINDOWS)
	#include "ajabase/system/windows/processimpl.h"
#elif defined(AJA_MAC)
	#include "ajabase/system/mac/processimpl.h"
#elif defined(AJA_LINUX)
	#include "ajabase/system/linux/processimpl.h"
#elif defined(AJA_BAREMETAL)
	#include "ajabase/system/bm/processimpl.h"
#endif

#include "process.h"

AJAProcess::AJAProcess() : mpImpl(NULL)
{
	mpImpl = new AJAProcessImpl();
}

AJAProcess::~AJAProcess()
{
	if(mpImpl)
		delete mpImpl;
	mpImpl = NULL;
}

uint64_t
AJAProcess::GetPid()
{
	return AJAProcessImpl::GetPid();
}

bool
AJAProcess::IsValid(uint64_t pid)
{
	return AJAProcessImpl::IsValid(pid);
}

bool
AJAProcess::Activate(uint64_t handle)
{
	return AJAProcessImpl::Activate(handle);
}

bool
AJAProcess::Activate(const char* pWindow)
{
	return AJAProcessImpl::Activate(pWindow);
}
