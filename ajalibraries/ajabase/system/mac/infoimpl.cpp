/**
    @file		mac/infoimpl.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
    @brief		Implements the AJASystemInfoImpl class on the Mac platform.
**/

#include "ajabase/system/system.h"
#include "ajabase/system/info.h"
#include "ajabase/system/mac/infoimpl.h"

#include <sys/utsname.h>

AJASystemInfoImpl::AJASystemInfoImpl()
{

}

AJASystemInfoImpl::~AJASystemInfoImpl()
{

}

AJAStatus
AJASystemInfoImpl::Rescan()
{
    AJAStatus ret = AJA_STATUS_FAIL;

    // descriptions
    mDescMap[int(AJA_SystemInfoTag_Host_Name)] = "Host name";

    char tmp_buf[512];

    // values
    if(gethostname(tmp_buf,sizeof(tmp_buf)) == 0)
    {
        mValueMap[int(AJA_SystemInfoTag_Host_Name)] = tmp_buf;
        ret = AJA_STATUS_SUCCESS;
    }
    else
    {
        mValueMap[int(AJA_SystemInfoTag_Host_Name)] = "unknown";
    }

    // use sysctl
    // AJA_SystemInfoTag_Host_Name = kern.hostname
    // AJA_SystemInfoTag_OS_VersionBuild = kern.osversion
    // AJA_SystemInfoTag_OS_KernelVersion = kern.ostype + kern.osrelease
    // AJA_SystemInfoTag_CPU_NumCores = hw.ncpu or hw.logicalcpu
    // AJA_SystemInfoTag_Host_BootTime = kern.boottime
    // AJA_SystemInfoTag_CPU_Type = machdep.cpu.brand_string
    return ret;
}
