/**
    @file		info.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
    @brief		Declares the AJASystemInfo class.
**/

#ifndef AJA_INFO_H
#define AJA_INFO_H

#include "ajabase/common/public.h"

// forward declarations
class AJASystemInfoImpl;

enum AJASystemInfoTag
{
    AJA_SystemInfoTag_Host_Name,
    AJA_SystemInfoTag_Host_BootTime,
    AJA_SystemInfoTag_OS_ProductName,
    AJA_SystemInfoTag_OS_Version,
    AJA_SystemInfoTag_OS_VersionBuild,
    AJA_SystemInfoTag_OS_KernelVersion,
    AJA_SystemInfoTag_CPU_Type,
    AJA_SystemInfoTag_CPU_NumCores,
    AJA_SystemInfoTag_Mem_Total,
    AJA_SystemInfoTag_Mem_Free,
    AJA_SystemInfoTag_Mem_Used,

    AJA_SystemInfoTag_LAST
};

/**
 *	Class for getting common information about the system.
 *	@ingroup AJAGroupSystem
 *
 *
 */
class AJA_EXPORT AJASystemInfo
{
public:

    AJASystemInfo();
    virtual ~AJASystemInfo();

    virtual AJAStatus Rescan();
	
    virtual AJAStatus GetTagValue(AJASystemInfoTag tag, std::string& value);
    virtual AJAStatus GetTagValue(AJASystemInfoTag tag, char* value, size_t max_len);

    virtual AJAStatus GetTagDescription(AJASystemInfoTag tag, std::string& desc);
    virtual AJAStatus GetTagDescription(AJASystemInfoTag tag, char* desc, size_t max_len);

private:

    AJASystemInfoImpl* mpImpl;
};

#endif	//	AJA_INFO_H
