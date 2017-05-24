/**
    @file		windows/infoimpl.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
    @brief		Implements the AJASystemInfoImpl class on the Windows platform.
**/

#include "ajabase/system/system.h"
#include "ajabase/system/info.h"
#include "ajabase/system/windows/infoimpl.h"

// need to link with Shlwapi.lib & Netapi32.lib
#pragma warning(disable:4996)
#include <io.h>
#include <Lm.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <time.h>
#include <Windows.h>

#include <iomanip>

struct WindowsVersionEntry
{
    int major;
    int minor;
    char* workstationName;
    char* serverName;
};

const WindowsVersionEntry WindowsVersionTable[] =
{
    { 5, 0, (char*)"Windows 2000",        (char*)"Windows 2000"},
    { 5, 1, (char*)"Windows XP",          (char*)"Windows XP"},
    { 5, 2, (char*)"Windows Server 2003", (char*)"Windows Server 2003"}, // This one is special cased in code
    { 6, 0, (char*)"Windows Vista",       (char*)"Windows Server 2008"},
    { 6, 1, (char*)"Windows 7",           (char*)"Windows Server 2008 R2"},
    { 6, 2, (char*)"Windows 8",           (char*)"Windows Server 2012"},
    { 6, 3, (char*)"Windows 8.1",         (char*)"Windows Server 2012 R2"},
    {10, 0, (char*)"Windows 10",          (char*)"Windows Server 2016"}
};
const int WindowsVersionTableSize = sizeof(WindowsVersionTable) / sizeof(WindowsVersionEntry);

std::string
aja_read_registry_string(HKEY hkey, std::string key_path, std::string key)
{
    std::string outValue;

    const DWORD buffSize = 128;
    char szBuff[buffSize];
    memset(szBuff, 0, buffSize);

    DWORD dwType = 0;
    DWORD dwInOutSize = buffSize;

    HKEY openedPathHkey;
    long lResult = RegOpenKeyExA(hkey, key_path.c_str(), 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &openedPathHkey);
    if(ERROR_SUCCESS == lResult)
    {
        lResult = RegQueryValueExA(openedPathHkey, key.c_str(), NULL, &dwType, (BYTE*)szBuff, &dwInOutSize);
        if (lResult == ERROR_SUCCESS)
        {
            outValue = szBuff;
        }

        RegCloseKey(openedPathHkey);
    }

    return outValue;
}

DWORD
aja_read_registry_dword(HKEY hkey, std::string key_path, std::string key)
{
    DWORD outValue=0;

    DWORD dwType = 0;
    DWORD dwInOutSize = sizeof(outValue);

    HKEY openedPathHkey;
    long lResult = RegOpenKeyExA(hkey, key_path.c_str(), 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &openedPathHkey);
    if(ERROR_SUCCESS == lResult)
    {
        lResult = RegQueryValueExA(openedPathHkey, key.c_str(), NULL, &dwType, (BYTE*)&outValue, &dwInOutSize);
        RegCloseKey(openedPathHkey);
    }

    return outValue;
}

std::string
aja_getsystemmodel()
{
    std::string outVal;
    std::string key_path = "HARDWARE\\DESCRIPTION\\System\\BIOS";

    outVal = aja_read_registry_string(HKEY_LOCAL_MACHINE, key_path, "SystemManufacturer");
    if (outVal.empty() == false)
        outVal += " ";
    outVal += aja_read_registry_string(HKEY_LOCAL_MACHINE, key_path, "SystemProductName");
    if (outVal.empty() == false)
        outVal += " (";
    outVal += aja_read_registry_string(HKEY_LOCAL_MACHINE, key_path, "SystemFamily");
    if (outVal.empty() == false)
        outVal += ")";

    return outVal;
}

std::string
aja_getsystembios()
{
    std::string outVal;
    std::string key_path = "HARDWARE\\DESCRIPTION\\System\\BIOS";
    outVal = aja_read_registry_string(HKEY_LOCAL_MACHINE, key_path, "BIOSVendor");
    if (outVal.empty() == false)
        outVal += " ";
    outVal += aja_read_registry_string(HKEY_LOCAL_MACHINE, key_path, "BIOSVersion");
    if (outVal.empty() == false)
        outVal += ", ";
    outVal += aja_read_registry_string(HKEY_LOCAL_MACHINE, key_path, "BIOSReleaseDate");

    return outVal;
}

std::string
aja_getsystemname()
{
    std::string outVal;
    TCHAR buffer[256] = TEXT("");
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH*2;
    if (GetComputerNameEx(ComputerNameDnsHostname, buffer, &dwSize))
    {
        outVal = buffer;
    }

    return outVal;
}

std::string
aja_getboottime()
{
    ULARGE_INTEGER li;
    li.QuadPart = GetTickCount64() * 10000;

    FILETIME sinceBoot;
    sinceBoot.dwLowDateTime = li.LowPart;
    sinceBoot.dwHighDateTime = li.HighPart;

    SYSTEMTIME stNow;
    GetLocalTime(&stNow);
    FILETIME nowTime;

    SystemTimeToFileTime(&stNow, &nowTime);
    FILETIME bootTime;

    // Need to combine the high and low parts before doing the time arithmetic
    ULARGE_INTEGER nowLi, sinceBootLi, bootTimeLi;
    nowLi.HighPart = nowTime.dwHighDateTime;
    nowLi.LowPart = nowTime.dwLowDateTime;
    sinceBootLi.HighPart = sinceBoot.dwHighDateTime;
    sinceBootLi.LowPart = sinceBoot.dwLowDateTime;

    bootTimeLi.QuadPart = nowLi.QuadPart - sinceBootLi.QuadPart;

    bootTime.dwHighDateTime = bootTimeLi.HighPart;
    bootTime.dwLowDateTime  = bootTimeLi.LowPart;

    SYSTEMTIME sysBootTime;
    FileTimeToSystemTime(&bootTime, &sysBootTime);

    std::ostringstream t;
    t << std::setfill('0') << std::setw(4) << sysBootTime.wYear << "-" <<
         std::setfill('0') << std::setw(2) << sysBootTime.wMonth << "-" <<
         std::setfill('0') << std::setw(2) << sysBootTime.wDay << " " <<
         std::setfill('0') << std::setw(2) << sysBootTime.wHour << ":" <<
         std::setfill('0') << std::setw(2) << sysBootTime.wMinute << ":" <<
         std::setfill('0') << std::setw(2) << sysBootTime.wSecond;

    return t.str();
}

std::string
aja_getosname()
{
    // get OS info
    std::string osname = "Unknown";

    OSVERSIONINFOEX osInfo;
    ZeroMemory(&osInfo, sizeof(OSVERSIONINFOEX));
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO)&osInfo);

    int majorVersion = (int)osInfo.dwMajorVersion;
    int minorVersion = (int)osInfo.dwMinorVersion;

    // Starting with Windows 8.1 the call to GetVersionEx() no longer returns
    // the correct major and minor version numbers, instead it returns 6.2,
    // which is Windows 8
    //
    // They forgot to "break" NetWkstaGetInfo(), so use that to get the
    // major and minor versions for Windows 8.1 and beyound
    if (majorVersion >=6 && minorVersion >= 2)
    {
        LPBYTE pinfoRawData;
        if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
        {
            WKSTA_INFO_100 *pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
            majorVersion = (int)pworkstationInfo->wki100_ver_major;
            minorVersion = (int)pworkstationInfo->wki100_ver_minor;
            ::NetApiBufferFree(pinfoRawData);
        }
    }

    bool foundVersion = false;
    for(int i=0;i<WindowsVersionTableSize;i++)
    {
        if (WindowsVersionTable[i].major == majorVersion &&
            WindowsVersionTable[i].minor == minorVersion)
        {
            if (majorVersion == 5 && minorVersion == 2)
            {
                // This one is a strange beast, special case it
                int ver2 = GetSystemMetrics(SM_SERVERR2);
                SYSTEM_INFO sysInfo;
                ZeroMemory(&sysInfo, sizeof(SYSTEM_INFO));
                GetSystemInfo(&sysInfo);

                if (osInfo.wSuiteMask & VER_SUITE_WH_SERVER)
                    osname = "Windows Home Server";
                else if(osInfo.wProductType == VER_NT_WORKSTATION &&
                        sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                    osname = "Windows XP Professional x64 Edition";
                else if(ver2 == 0)
                    osname = "Windows Server 2003";
                else
                    osname = "Windows Server 2003 R2";
            }
            else
            {
                if (osInfo.wProductType == VER_NT_WORKSTATION)
                    osname = WindowsVersionTable[i].workstationName;
                else
                   osname = WindowsVersionTable[i].serverName;
            }
            foundVersion = true;
            break;
        }
    }

    // append the service pack info if available
    osname += " ";
    osname += osInfo.szCSDVersion;

    return osname;
}

std::string
aja_getcputype()
{
    // get CPU info
    int CPUInfo[4] = {-1};
    char CPUBrandString[0x40];
    __cpuid(CPUInfo, 0x80000000);
    unsigned int nExIds = CPUInfo[0];
    memset(CPUBrandString, 0, sizeof(CPUBrandString));
    for (unsigned int i=0x80000000; i<=nExIds; ++i)
    {
        // Get the information associated with each extended ID.
        __cpuid(CPUInfo, i);
        // Interpret CPU brand string.
        if  (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if  (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if  (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }

    return CPUBrandString;
}

std::string
aja_getcpucores()
{
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    std::ostringstream oss;
    oss << siSysInfo.dwNumberOfProcessors;
    return oss.str();
}

void
aja_getmemory(AJASystemInfoMemoryUnit units, std::string &total, std::string &used, std::string &free)
{
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

    int64_t memtotalbytes = statex.ullTotalPhys;
    int64_t memfreebytes = statex.ullAvailPhys;
    int64_t memusedbytes = memtotalbytes - memfreebytes;

    std::string unitsLabel;
    double divisor = 1.0;
    switch(units)
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

    std::ostringstream t,u,f;
    t << int64_t(memtotalbytes / divisor) << " " << unitsLabel;
    u << int64_t(memusedbytes / divisor) << " " << unitsLabel;
    f << int64_t(memfreebytes / divisor) << " " << unitsLabel;

    total = t.str();
    used = u.str();
    free = f.str();
}

std::string
aja_getosversion()
{
    std::string outVal;
    outVal = aja_read_registry_string(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                                      "ReleaseId");

    return outVal;
}

std::string
aja_getosversionbuild()
{
    std::ostringstream oss;
    std::string key_path = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    oss << aja_read_registry_string(HKEY_LOCAL_MACHINE, key_path, "CurrentBuild") <<
        "." << aja_read_registry_dword(HKEY_LOCAL_MACHINE, key_path, "UBR");

    return oss.str();
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

    mValueMap[int(AJA_SystemInfoTag_System_Model)] = aja_getsystemmodel();
    mValueMap[int(AJA_SystemInfoTag_System_Bios)] = aja_getsystembios();
    mValueMap[int(AJA_SystemInfoTag_System_Name)] = aja_getsystemname();
    mValueMap[int(AJA_SystemInfoTag_System_BootTime)] = aja_getboottime();
    mValueMap[int(AJA_SystemInfoTag_OS_ProductName)] = aja_getosname();
    mValueMap[int(AJA_SystemInfoTag_OS_Version)] = aja_getosversion();
    mValueMap[int(AJA_SystemInfoTag_OS_VersionBuild)] = aja_getosversionbuild();
    //mValueMap[int(AJA_SystemInfoTag_OS_KernelVersion)] // don't really have anything for this on Windows
    mValueMap[int(AJA_SystemInfoTag_CPU_Type)] = aja_getcputype();
    mValueMap[int(AJA_SystemInfoTag_CPU_NumCores)] = aja_getcpucores();
    aja_getmemory(AJASystemInfoMemoryUnit(mMemoryUnits),
                  mValueMap[int(AJA_SystemInfoTag_Mem_Total)],
                  mValueMap[int(AJA_SystemInfoTag_Mem_Used)],
                  mValueMap[int(AJA_SystemInfoTag_Mem_Free)]);

    // Paths
    std::string path;

    // Need to get the user from the registry since when this is run as a service all the normal
    // calls to get the user name return SYSTEM

    // Method 1 of getting username from registry
    std::string tmpStr = aja_read_registry_string(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI",
                                                  "LastLoggedOnUser");

    path.erase();
    path.append(getenv("SystemDrive"));
    path.append("\\Users\\");
    if(tmpStr.find('\\') != std::string::npos )
    {
        //strip off anything before a "\\"
        path.append(tmpStr.substr(tmpStr.find('\\')+1));
    }
    else
    {
        path.append(tmpStr);
    }

    //check it directory exists, if not try Method 2
    if(PathFileExistsA(path.c_str())==false)
    {
        // Method 2 of getting username from registry (will not work if logged in with a Microsoft ID)
        // http://forums.codeguru.com/showthread.php?317367-To-get-current-Logged-in-user-name-from-within-a-service
        path.erase();
        path.append(getenv("SystemDrive"));
        path.append("\\Users\\");
        path.append(aja_read_registry_string(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                             "LastUsedUsername"));
    }

    path.append("\\AppData\\Local\\Aja\\");

    mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreUser)] = path;

    TCHAR szPath[MAX_PATH];
    HRESULT r;
    r = SHGetFolderPath(NULL,CSIDL_COMMON_APPDATA,NULL,0,szPath);
    if(r != S_OK)
    {
        //error
    }
    else
    {
        path.erase();
#ifdef UNICODE
        PathAppend(szPath, L"Aja\\");
        char tmpPath[MAX_PATH];
        ::wcstombs(tmpPath,szPath,MAX_PATH);
        path.append(tmpPath);
#else
        PathAppend(szPath, "Aja\\");
        path.append(szPath);
#endif
        mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreSystem)] = path;
    }

    mValueMap[int(AJA_SystemInfoTag_Path_Applications)] = "C:\\Program Files\\AJA\\windows\\Applications\\";
    mValueMap[int(AJA_SystemInfoTag_Path_Utilities)] = "C:\\Program Files\\AJA\\windows\\Applications\\";

    ret = AJA_STATUS_SUCCESS;
    return ret;
}
