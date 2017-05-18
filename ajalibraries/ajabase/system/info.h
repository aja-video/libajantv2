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

enum AJASystemInfoMemoryUnit
{
    AJA_SystemInfoMemoryUnit_Bytes,
    AJA_SystemInfoMemoryUnit_Kilobytes,
    AJA_SystemInfoMemoryUnit_Megabytes,
    AJA_SystemInfoMemoryUnit_Gigabytes,

    AJA_SystemInfoMemoryUnit_LAST
};

enum AJASystemInfoTag
{
    AJA_SystemInfoTag_System_Model,
    AJA_SystemInfoTag_System_Bios,
    AJA_SystemInfoTag_System_Name,
    AJA_SystemInfoTag_System_BootTime,
    AJA_SystemInfoTag_OS_ProductName,
    AJA_SystemInfoTag_OS_Version,
    AJA_SystemInfoTag_OS_VersionBuild,
    AJA_SystemInfoTag_OS_KernelVersion,
    AJA_SystemInfoTag_CPU_Type,
    AJA_SystemInfoTag_CPU_NumCores,
    AJA_SystemInfoTag_Mem_Total,
    AJA_SystemInfoTag_Mem_Used,
    AJA_SystemInfoTag_Mem_Free,
    AJA_SystemInfoTag_Path_PersistenceStoreUser,
    AJA_SystemInfoTag_Path_PersistenceStoreSystem,
    AJA_SystemInfoTag_Path_Applications,
    AJA_SystemInfoTag_Path_Utilities,

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

    AJASystemInfo(AJASystemInfoMemoryUnit units = AJA_SystemInfoMemoryUnit_Megabytes);
    virtual ~AJASystemInfo();

    virtual AJAStatus Rescan();
	
    AJAStatus GetValue(AJASystemInfoTag tag, std::string& value);
    AJAStatus GetLabel(AJASystemInfoTag tag, std::string& label);

    void ToString(std::string& allLabelsAndValues);
    std::string ToString(int maxLength = -1);
private:

    AJASystemInfoImpl* mpImpl;
};

AJA_EXPORT std::ostream & operator << (std::ostream & outStream, const AJASystemInfo & inData);

#endif	//	AJA_INFO_H
