/**
    @file		info.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
    @brief		Implements the AJASystemInfo class.
**/

// include the system dependent implementation class
#if defined(AJA_WINDOWS)
    #include "ajabase/system/windows/infoimpl.h"
#endif
#if defined(AJA_LINUX)
    #include "ajabase/system/linux/infoimpl.h"
#endif
#if defined(AJA_MAC)
    #include "ajabase/system/mac/infoimpl.h"
#endif


#include "ajabase/system/info.h"


AJASystemInfo::AJASystemInfo()
{
	// create the implementation class
    mpImpl = new AJASystemInfoImpl();

    Rescan();
}

AJASystemInfo::~AJASystemInfo()
{
    if(mpImpl)
	{
        delete mpImpl;
        mpImpl = NULL;
	}
}

AJAStatus
AJASystemInfo::Rescan()
{
    AJAStatus ret = AJA_STATUS_FAIL;
    if(mpImpl)
    {
        ret = mpImpl->Rescan();
    }

    return ret;
}

AJAStatus
AJASystemInfo::GetTagValue(AJASystemInfoTag tag, std::string &value)
{
    AJAStatus ret = AJA_STATUS_FAIL;
    if (mpImpl && mpImpl->mValueMap.count(int(tag)) != 0)
    {
        value = mpImpl->mValueMap[int(tag)];
        ret = AJA_STATUS_SUCCESS;
    }

    return ret;
}

AJAStatus
AJASystemInfo::GetTagValue(AJASystemInfoTag tag, char* value, size_t max_len)
{
    AJAStatus ret = AJA_STATUS_FAIL;

    std::string tmp;
    ret = GetTagValue(tag, tmp);
    strncpy(value, tmp.c_str(), max_len);

    return ret;
}

AJAStatus
AJASystemInfo::GetTagDescription(AJASystemInfoTag tag, std::string& desc)
{
    AJAStatus ret = AJA_STATUS_FAIL;
    if (mpImpl && mpImpl->mDescMap.count(int(tag)) != 0)
    {
        desc = mpImpl->mDescMap[int(tag)];
        ret = AJA_STATUS_SUCCESS;
    }

    return ret;
}

AJAStatus
AJASystemInfo::GetTagDescription(AJASystemInfoTag tag, char* desc, size_t max_len)
{
    AJAStatus ret = AJA_STATUS_FAIL;

    std::string tmp;
    ret = GetTagDescription(tag, tmp);
    strncpy(desc, tmp.c_str(), max_len);

    return ret;
}
