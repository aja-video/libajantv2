/**
    @file		linux/infoimpl.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
    @brief		Implements the AJASystemInfoImpl class on the Linux platform.
**/

#include "ajabase/system/system.h"
#include "ajabase/system/info.h"
#include "ajabase/system/linux/infoimpl.h"

#include <cstdlib>
#include <stdexcept>
#include <unistd.h>

std::string aja_cmd(const char* cmd)
{
    const int maxbufsize = 256;
    char buffer[maxbufsize];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }

    try
    {
        while (!feof(pipe))
        {
            if (fgets(buffer, maxbufsize, pipe) != NULL)
            {
                try
                {
                    result += buffer;
                }
                catch (...)
                {
                    pclose(pipe);
                    throw;
                }
            }
        }
    }
    catch (...)
    {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

std::string aja_procfs(const char* procfs_file, const char* value_key)
{
    // cat /proc/cpuinfo outputs something like:
    // vendor_id     : GenuineIntel
    // model name    : Intel(R) Xeon(R) CPU       X5650  @ 2.67GHz
    // ...
    // so use grep to get the lines matching key
    // use head to only select the first line returned
    // use cut to get just the part after ':'
    // use xargs to remove leading and trailing spaces
    // use tr to remove linefeeds
    // use tr to remove repeated spaces
    std::ostringstream oss;
    oss << "cat /proc/" << procfs_file << " | grep '" << value_key << "' | head -n 1 | cut -d ':' -f 2 | xargs | tr -d '\n' | tr -s ' '";

    return aja_cmd(oss.str().c_str());
}

std::string& replace(std::string& s, const std::string& from, const std::string& to)
{
    if (!from.empty())
        for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
            s.replace(pos, from.size(), to);
    return s;
}

AJASystemInfoImpl::AJASystemInfoImpl(int units)
{
    mMemoryUnits = units;
}

AJASystemInfoImpl::~AJASystemInfoImpl()
{

}

AJAStatus
AJASystemInfoImpl::Rescan()
{
    AJAStatus ret = AJA_STATUS_FAIL;

    static char tmp_buf[4096];

    mValueMap[int(AJA_SystemInfoTag_System_Model)] = aja_cmd("uname -m | tr -d '\n'");
    gethostname(tmp_buf, sizeof(tmp_buf));
    mValueMap[int(AJA_SystemInfoTag_System_Name)] = tmp_buf;
    mValueMap[int(AJA_SystemInfoTag_System_BootTime)] = aja_cmd("uptime -s | tr -d '\n'");
    mValueMap[int(AJA_SystemInfoTag_OS_ProductName)] = aja_cmd("cat /etc/os-release | grep 'PRETTY_NAME' | head -n 1 | cut -d '=' -f 2 | tr -d '\"' | tr -d '\n'");
    // alt way == mValueMap[int(AJA_SystemInfoTag_OS_ProductName)] = aja_cmd("cat /etc/lsb-release | grep 'DISTRIB_DESCRIPTION' | head -n 1 | cut -d '=' -f 2 | tr -d '\"' | tr -d '\n'");
    mValueMap[int(AJA_SystemInfoTag_OS_Version)] = aja_cmd("cat /etc/os-release | grep 'VERSION_ID' | head -n 1 | cut -d '=' -f 2 | tr -d '\"' | tr -d '\n'");
    // alt way == mValueMap[int(AJA_SystemInfoTag_OS_Version)] = aja_cmd("cat /etc/LSB-release | grep 'DISTRIB_RELEASE' | head -n 1 | cut -d '=' -f 2 | tr -d '\"' | tr -d '\n'");

    mValueMap[int(AJA_SystemInfoTag_OS_VersionBuild)] = aja_cmd("uname -v | tr -d '\n'");
    mValueMap[int(AJA_SystemInfoTag_OS_KernelVersion)] = aja_cmd("uname -r | tr -d '\n'");
    mValueMap[int(AJA_SystemInfoTag_CPU_Type)] = aja_procfs("cpuinfo", "model name");
    long int numProcs = sysconf(_SC_NPROCESSORS_ONLN);
    std::ostringstream num_cores;
    num_cores << numProcs;
    mValueMap[int(AJA_SystemInfoTag_CPU_NumCores)] = num_cores.str();

    ////
    std::string memTotalStr = aja_procfs("meminfo", "MemTotal");
    int64_t memtotalbytes=0;
    if (memTotalStr.find(" kB") != std::string::npos)
    {
        // convert from kilobytes to bytes
        replace(memTotalStr, " kB", "");
        std::istringstream(memTotalStr) >> memtotalbytes;
        memtotalbytes *= 1024;
    }
    else
    {
        // assume it is in bytes?
        std::istringstream(memTotalStr) >> memtotalbytes;
    }

    std::string memFreeStr = aja_procfs("meminfo", "MemFree");
    int64_t memfreebytes=0;
    if (memFreeStr.find(" kB") != std::string::npos)
    {
        // convert from kilobytes to bytes
        replace(memFreeStr, " kB", "");
        std::istringstream(memFreeStr) >> memfreebytes;
        memfreebytes *= 1024;
    }
    else
    {
        // assume it is in bytes?
        std::istringstream(memFreeStr) >> memfreebytes;
    }

    std::string unitsLabel;
    double divisor = 1.0;
    switch(mMemoryUnits)
    {
        default:
        case AJA_SystemInfoMemoryUnit_Bytes:
            unitsLabel = "B";
            break;
        case AJA_SystemInfoMemoryUnit_Kilobytes:
            unitsLabel = "KB";
            divisor = 1024.0;
            break;
        case AJA_SystemInfoMemoryUnit_Megabytes:
            unitsLabel = "MB";
            divisor = 1048576.0;
            break;
        case AJA_SystemInfoMemoryUnit_Gigabytes:
            unitsLabel = "GB";
            divisor = 1073741824.0;
            break;
    }

    int64_t memusedbytes = memtotalbytes - memfreebytes;

    std::ostringstream t,u,f;
    t << int64_t(memtotalbytes / divisor) << " " << unitsLabel;
    u << int64_t(memusedbytes / divisor) << " " << unitsLabel;
    f << int64_t(memfreebytes / divisor) << " " << unitsLabel;

    mValueMap[int(AJA_SystemInfoTag_Mem_Total)] = t.str();
    mValueMap[int(AJA_SystemInfoTag_Mem_Used)] = u.str();
    mValueMap[int(AJA_SystemInfoTag_Mem_Free)] = f.str();

    // Paths
    const char* homePath = getenv("HOME");
    if (homePath != NULL)
    {
        mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreUser)] = homePath;
        mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreUser)].append("/.aja/config/");
    }

    mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreSystem)] = "/opt/aja/config/";

    mValueMap[int(AJA_SystemInfoTag_Path_Applications)] = "/opt/aja/bin/";
    mValueMap[int(AJA_SystemInfoTag_Path_Utilities)] = "/opt/aja/bin/";

    return ret;
}