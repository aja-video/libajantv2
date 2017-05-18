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
#include <cstring>
#include <iomanip>
#include <iostream>

AJASystemInfo::AJASystemInfo(AJASystemInfoMemoryUnit units)
{
	// create the implementation class
    mpImpl = new AJASystemInfoImpl(units);

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
        // labels
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_System_Model)] = "System Model";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_System_Bios)] = "System BIOS";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_System_Name)] = "System Name";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_System_BootTime)] = "System Boot Time";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_OS_ProductName)] = "OS Product Name";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_OS_Version)] = "OS Version";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_OS_VersionBuild)] = "OS Build";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_OS_KernelVersion)] = "OS Kernel Version";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_CPU_Type)] = "CPU Type";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_CPU_NumCores)] = "CPU Num Cores";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Mem_Total)] = "Memory Total";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Mem_Used)] = "Memory Used";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Mem_Free)] = "Memory Free";

        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_PersistenceStoreUser)] = "User Persistence Store Path";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_PersistenceStoreSystem)] = "System Persistence Store Path";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_Applications)] = "AJA Applications Path";
        mpImpl->mLabelMap[int(AJA_SystemInfoTag_Path_Utilities)] = "AJA Utilities Path";

        ret = mpImpl->Rescan();
    }

    return ret;
}

AJAStatus
AJASystemInfo::GetValue(AJASystemInfoTag tag, std::string &value)
{
    AJAStatus ret = AJA_STATUS_FAIL;
    value = "";
    if (mpImpl && mpImpl->mValueMap.count(int(tag)) != 0)
    {
        value = mpImpl->mValueMap[int(tag)];
        ret = AJA_STATUS_SUCCESS;
    }

    return ret;
}

AJAStatus
AJASystemInfo::GetLabel(AJASystemInfoTag tag, std::string& label)
{
    AJAStatus ret = AJA_STATUS_FAIL;
    label = "";
    if (mpImpl && mpImpl->mLabelMap.count(int(tag)) != 0)
    {
        label = mpImpl->mLabelMap[int(tag)];
        ret = AJA_STATUS_SUCCESS;
    }

    return ret;
}

std::string
AJASystemInfo::ToString(int maxLength)
{
    std::ostringstream oss;

    int longestLabelLen = 0;
    for (int i=0;i<(int)AJA_SystemInfoTag_LAST;i++)
    {
        AJASystemInfoTag tag = (AJASystemInfoTag)i;
        std::string label, value;
        AJAStatus retLabel = GetLabel(tag, label);
        AJAStatus retValue = GetValue(tag, value);
        if (retLabel == AJA_STATUS_SUCCESS && retValue == AJA_STATUS_SUCCESS)
        {
            if ((int)label.length() > longestLabelLen)
                longestLabelLen = (int)label.length();
        }
    }

    longestLabelLen += 3;

    for (int i=0;i<(int)AJA_SystemInfoTag_LAST;i++)
    {
        AJASystemInfoTag tag = (AJASystemInfoTag)i;
        std::string label, value;
        AJAStatus retLabel = GetLabel(tag, label);
        AJAStatus retValue = GetValue(tag, value);

        if (retLabel == AJA_STATUS_SUCCESS && retValue == AJA_STATUS_SUCCESS)
        {
            label += ":";
            std::ostringstream l;
            l << std::setw(longestLabelLen) << std::left << label << " " << value;

            int longestLabelPlusColonLen = longestLabelLen+1;

            if (maxLength > 0 &&
                maxLength > longestLabelPlusColonLen &&
                (int)l.str().length() > maxLength)
            {
                // this will wrap the values at maxLength to use multiple lines
                // a maxLength of -1 will skip this functionality
                // i.e:
                //
                // some label:   this is a long string of text
                //
                // with a maxLength of 36 would become
                //
                // some label:   this is a long string
                //               of text
                //

                std::string tmp = l.str();

                oss << tmp.substr(0, maxLength) << std::endl;
                tmp = tmp.substr(maxLength, std::string::npos);

                int maxGrabAmount = maxLength - longestLabelPlusColonLen;
                while(!tmp.empty())
                {
                    int grab = maxGrabAmount;
                    if (grab > (int)tmp.length())
                        grab = (int)tmp.length();

                    oss << std::setw(longestLabelLen) << std::left << "" << " " << tmp.substr(0, grab) << std::endl;
                    tmp = tmp.substr(grab, std::string::npos);
                }
            }
            else
            {
                oss << l.str() << std::endl;
            }
        }
    }

    return oss.str();
}

void
AJASystemInfo::ToString(std::string& allLabelsAndValues)
{
    allLabelsAndValues = ToString();
}

std::ostream & operator << (std::ostream & outStream, const AJASystemInfo & inData)
{
    AJASystemInfo* instance = (AJASystemInfo*)&inData;
    outStream << instance->ToString();
    return outStream;
}