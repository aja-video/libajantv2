/**
	@file		mac/processimpl.cpp
	@copyright	Copyright (C) 2011-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAProcessImpl class on the Mac platform.
**/

#include "ajabase/system/mac/processimpl.h"
#include "ajabase/common/timer.h"
#include <signal.h>


// class AJAProcessImpl

AJAProcessImpl::AJAProcessImpl()
{
}


AJAProcessImpl::~AJAProcessImpl()
{
}

uint64_t
AJAProcessImpl::GetPid()
{
	return getpid();
}

bool
AJAProcessImpl::IsValid(uint64_t pid)
{
    if(kill(pid,0)==0)
        return true;
    else
        return false;
}

bool
AJAProcessImpl::Activate(const char* pWindow)
{
    (void)pWindow;
    
	//Dummy place holder
	return false;
}

bool
AJAProcessImpl::Activate(uint64_t handle)
{
    (void)handle;
    
	//Dummy place holder
	return false;
}
