/* SPDX-License-Identifier: MIT */
/**
	@file		windows/infoimpl.cpp
	@brief		Implements the AJASystemInfoImpl class on the Windows platform.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/system.h"
#include "ajabase/system/info.h"
#include "ajabase/system/windows/infoimpl.h"

// need to link with Shlwapi.lib & Netapi32.lib
#pragma warning(disable:4996)
#include <intrin.h>
#include <io.h>
#include <Knownfolders.h>
#include <LM.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <time.h>
#include <iomanip>
#include <comdef.h>
#include <Wbemidl.h>  // links with wbemuuid.lib
#include <sstream>

struct WindowsVersionEntry
{
	int major;
	int minor;
	char* workstationName;
	char* serverName;
};

const WindowsVersionEntry WindowsVersionTable[] =
{
	{ 5, 0, (char*)"Windows 2000",		  (char*)"Windows 2000"},
	{ 5, 1, (char*)"Windows XP",		  (char*)"Windows XP"},
	{ 5, 2, (char*)"Windows Server 2003", (char*)"Windows Server 2003"}, // This one is special cased in code
	{ 6, 0, (char*)"Windows Vista",		  (char*)"Windows Server 2008"},
	{ 6, 1, (char*)"Windows 7",			  (char*)"Windows Server 2008 R2"},
	{ 6, 2, (char*)"Windows 8",			  (char*)"Windows Server 2012"},
	{ 6, 3, (char*)"Windows 8.1",		  (char*)"Windows Server 2012 R2"}
	//{10, 0, (char*)"Windows 10",		  (char*)"Windows Server 2016"} // Use Registry to retrieve from this point forward
};
const int WindowsVersionTableSize = sizeof(WindowsVersionTable) / sizeof(WindowsVersionEntry);

std::string
aja_getsystemmodel()
{
	std::string outVal;
	std::string key_path = "HARDWARE\\DESCRIPTION\\System\\BIOS";

	outVal = aja::read_registry_string(HKEY_LOCAL_MACHINE, key_path, "SystemManufacturer");
	if (outVal.empty() == false)
		outVal += " ";
	outVal += aja::read_registry_string(HKEY_LOCAL_MACHINE, key_path, "SystemProductName");
	if (outVal.empty() == false)
		outVal += " (";
	outVal += aja::read_registry_string(HKEY_LOCAL_MACHINE, key_path, "SystemFamily");
	if (outVal.empty() == false)
		outVal += ")";

	return outVal;
}

std::string
aja_getsystembios()
{
	std::string outVal;
	std::string key_path = "HARDWARE\\DESCRIPTION\\System\\BIOS";
	outVal = aja::read_registry_string(HKEY_LOCAL_MACHINE, key_path, "BIOSVendor");
	if (outVal.empty() == false)
		outVal += " ";
	outVal += aja::read_registry_string(HKEY_LOCAL_MACHINE, key_path, "BIOSVersion");
	if (outVal.empty() == false)
		outVal += ", ";
	outVal += aja::read_registry_string(HKEY_LOCAL_MACHINE, key_path, "BIOSReleaseDate");

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
	bootTime.dwLowDateTime	= bootTimeLi.LowPart;

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

// Uses WMI (Windows Management Instrumentation), to retrieve a pretty OS name complete with edition.
//		WMI Object=Win32_OperatingSystem ; Property=Caption
// The Method used below was largely taken from an MSDN example.  This method was chosen for it's use of native Win32 API
// The same property can be retrieved with a single line powershell command: (Get-WmiObject Win32_OperatingSystem).Caption
static std::string getOSName_WMI()
{
	std::string retVal = "Microsoft Windows";

	// Initialize COM
	HRESULT hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	if (!SUCCEEDED(hres) && hres != RPC_E_CHANGED_MODE) {
		// Bail out...CoUninitialize should only be called if the call to CoInitialize was successful.
		return retVal;
	}

	// Obtain the initial locator
	IWbemLocator *pLoc = NULL;
	hres = CoCreateInstance(
			CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *) &pLoc);
	if (FAILED(hres)){
		goto cleanup;
	}

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	IWbemServices *pSvc = NULL;
	hres = pLoc->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (for example, Kerberos)
			0,                       // Context object
			&pSvc                    // pointer to IWbemServices proxy
			);
	if (FAILED(hres)){
		goto cleanup;
	}

	IClientSecurity* pSecurity = NULL;
	hres = pSvc->QueryInterface(IID_IClientSecurity, (LPVOID*)&pSecurity);
	if (FAILED(hres) || !pSecurity) {
		goto cleanup;
	}

	// Querying the current authentication information
	DWORD authnSvc = 0;
	DWORD authzSvc = 0;
	LPOLESTR serverPrincName = NULL;
	DWORD authnLevel = 0;
	DWORD impLevel = 0;
	RPC_AUTH_IDENTITY_HANDLE authInfo = NULL;
	DWORD ifCapabilites = 0;

	hres = pSecurity->QueryBlanket(pSvc,
								&authnSvc,
								&authzSvc,
								&serverPrincName,
								&authnLevel,
								&impLevel,
								&authInfo,
								&ifCapabilites);
	if (FAILED(hres)) {
		goto cleanup;
	}

	// Setting authentication information on proxy interface
	hres = pSecurity->SetBlanket(pSvc,
								authnSvc,
								authzSvc,
								serverPrincName,
								RPC_C_AUTHN_LEVEL_DEFAULT,
								RPC_C_IMP_LEVEL_IMPERSONATE,
								authInfo,
								EOAC_NONE);

	if (FAILED(hres)) {
		goto cleanup;
	}

	//Execute Query agianst WMI Object: Win32_OperatingSystem
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT Caption FROM Win32_OperatingSystem"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);
	if (FAILED(hres)) {
		goto cleanup;
	}

	// Get the data from the query -------------------
	IWbemClassObject *pClsObj = NULL;
	ULONG uReturn = 0;
	hres = pEnumerator->Next(WBEM_INFINITE, 1, &pClsObj, &uReturn);
	if(FAILED(hres) || 0 == uReturn) {
		goto cleanup;
	}

	// Get the value of the Name property
	VARIANT vtProp;
	VariantInit(&vtProp);
	hres = pClsObj->Get(L"Caption", 0, &vtProp, 0, 0);
	if (SUCCEEDED(hres)) {
		retVal = _bstr_t(vtProp.bstrVal);
	}
	VariantClear(&vtProp);

cleanup:
	if (pSvc != NULL) {
		pSvc->Release();
		pSvc = NULL;
	}
	if (pLoc != NULL) {
		pLoc->Release();
		pLoc = NULL;
	}
	if (pEnumerator != NULL) {
		pEnumerator->Release();
		pEnumerator = NULL;
	}
	if (pClsObj != NULL) {
		pClsObj->Release();
		pClsObj = NULL;
	}

	CoUninitialize();

	return retVal;
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
	//
	// Update: Windows 10 and corresponding Server versions have been removed from WindowsVersionTable
	// due to the variety of builds under Windows 10, while the majorVesrion and minorVersion apparently do not update beyond 10.0
	// For Windows 10 and beyond, getting "Caption" from the WMI (Windows Management Instrumentation) Object Win32_OperatingSystem

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

	if (!foundVersion)
	{
		//Microsoft is not updating the below key for Windows 11, it still reads as Windows 10.
		//osname = aja::read_registry_string(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		//								  "ProductName");

		//Changed to retrieve a pretty OS Name with Edition from Win32_Operatingystem WMI class
		osname = getOSName_WMI();
	}



	// append the service pack info if available
	osname += " ";
	osname += osInfo.szCSDVersion;

	return osname;
}

std::string
aja_getcputype()
{
	std::string CPUBrandString = aja::read_registry_string(HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		"ProcessorNameString");
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

std::string
aja_getgputype()
{
	std::ostringstream oss;
	DISPLAY_DEVICE devInfo;
	devInfo.cb = sizeof(DISPLAY_DEVICE);
	DWORD loopDevNum = 0;
	std::map<std::string, int> foundMap;
	while (EnumDisplayDevices(NULL, loopDevNum, &devInfo, 0))
	{
		std::string name = devInfo.DeviceString;
		if (foundMap.find(name) == foundMap.end())
		{
			if (foundMap.empty() == false)
				oss << ", ";

			oss << name;
			foundMap[name] = 1;
		}
		loopDevNum++;
	}
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
	//At some point circa Windows 10, Reg Key DisplayVersion began to be used instead of ReleaseId. Check for that first.
	std::string outVal;
	outVal = aja::read_registry_string(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
									  "DisplayVersion");
	if (outVal == "")
		outVal = aja::read_registry_string(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
										  "ReleaseId");

	return outVal;
}

std::string
aja_getosversionbuild()
{
	std::ostringstream oss;
	std::string key_path = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
	oss << aja::read_registry_string(HKEY_LOCAL_MACHINE, key_path, "CurrentBuild") <<
		"." << aja::read_registry_dword(HKEY_LOCAL_MACHINE, key_path, "UBR");

	return oss.str();
}

std::string
aja_mkpath_to_user_dir(const std::string& username)
{
	std::string path;

	path.append(getenv("SystemDrive"));
	path.append("\\Users\\");
	if (username.find('\\') != std::string::npos)
	{
		//strip off anything before a "\\"
		path.append(username.substr(username.find('\\') + 1));
	}
	else
	{
		path.append(username);
	}

	return path;
}



AJASystemInfoImpl::AJASystemInfoImpl(int units)
{
	mMemoryUnits = units;
}

AJASystemInfoImpl::~AJASystemInfoImpl()
{

}

AJAStatus
AJASystemInfoImpl::Rescan(AJASystemInfoSections sections)


{
	AJAStatus ret = AJA_STATUS_FAIL;

	if (sections & AJA_SystemInfoSection_System)
	{
		mValueMap[int(AJA_SystemInfoTag_System_Model)] = aja_getsystemmodel();
		mValueMap[int(AJA_SystemInfoTag_System_Bios)] = aja_getsystembios();
		mValueMap[int(AJA_SystemInfoTag_System_Name)] = aja_getsystemname();
		mValueMap[int(AJA_SystemInfoTag_System_BootTime)] = aja_getboottime();

		ret = AJA_STATUS_SUCCESS;
	}

	if (sections & AJA_SystemInfoSection_OS)
	{
		mValueMap[int(AJA_SystemInfoTag_OS_ProductName)] = aja_getosname();
		mValueMap[int(AJA_SystemInfoTag_OS_Version)] = aja_getosversion();
		mValueMap[int(AJA_SystemInfoTag_OS_VersionBuild)] = aja_getosversionbuild();
		//mValueMap[int(AJA_SystemInfoTag_OS_KernelVersion)] // don't really have anything for this on Windows

		ret = AJA_STATUS_SUCCESS;
	}

	if (sections & AJA_SystemInfoSection_CPU)
	{
		mValueMap[int(AJA_SystemInfoTag_CPU_Type)] = aja_getcputype();
		mValueMap[int(AJA_SystemInfoTag_CPU_NumCores)] = aja_getcpucores();

		ret = AJA_STATUS_SUCCESS;
	}

	if (sections & AJA_SystemInfoSection_Mem)
	{
		aja_getmemory(AJASystemInfoMemoryUnit(mMemoryUnits),
					  mValueMap[int(AJA_SystemInfoTag_Mem_Total)],
					  mValueMap[int(AJA_SystemInfoTag_Mem_Used)],
					  mValueMap[int(AJA_SystemInfoTag_Mem_Free)]);

		ret = AJA_STATUS_SUCCESS;
	}

	if (sections & AJA_SystemInfoSection_GPU)
	{
		mValueMap[int(AJA_SystemInfoTag_GPU_Type)] = aja_getgputype();

		ret = AJA_STATUS_SUCCESS;
	}

	if (sections & AJA_SystemInfoSection_Path)
	{
		std::string path;

		// We try 4 different ways to get the path to the user's home directory,
		// We start by using a system call and the try reading directly from the registry
		// so we can use this within a service.
		// 0) Read the path to the user home directory via SHGetFolderPathA()
		//	  ISSUES
		//	  - Does not work so well if used from a service, in this case it will return:
		//		C:\WINDOWS\system32\config\systemprofile
		// 1) Read a value from HKEY_CURRENT_USER\Volatile Environment
		//	  ISSUES
		//	  - Does not work so well if used from a service, in this case it will return user 'SYSTEM'
		// 2) Read a value from HKEY_LOCAL_MACHINE...\Authentication
		//	  ISSUES
		//	  - Does not work so well if multiple users logged in and the user using the desktop was
		//		not the last one logged into the machine.
		// 3) As a last attempt use an older method that does not work with microsoft id logins
		//	  http://forums.codeguru.com/showthread.php?317367-To-get-current-Logged-in-user-name-from-within-a-service
		//	  ISSUES
		//	  - Same as #2 above

		bool usernameFound = false;

		// try method 0
		{

			char szPath[MAX_PATH];
			HRESULT hresult = SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, szPath);
			if (hresult == S_OK)
			{
				path.append(szPath);

				std::vector<std::string> parts;
				aja::split(path, '\\', parts);
				std::string username;
				if (parts.size() > 0)
					username = parts.at(parts.size()-1);

				if (!path.empty() && username != "systemprofile" && PathFileExistsA(path.c_str()))
					usernameFound = true;
			}
		}

		// try method 1
		if (!usernameFound)
		{
			std::string regVal = aja::read_registry_string(HKEY_CURRENT_USER,
														   "Volatile Environment",
														   "USERNAME");
			if (!regVal.empty() && regVal != "SYSTEM")
			{
				path = aja_mkpath_to_user_dir(regVal);
				if (!path.empty() && PathFileExistsA(path.c_str()))
					usernameFound = true;
			}
		}

		// try method 2
		if (!usernameFound)
		{
			std::string regVal = aja::read_registry_string(HKEY_LOCAL_MACHINE,
														   "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI",
														   "LastLoggedOnUser");
			if (!regVal.empty())
			{
				path = aja_mkpath_to_user_dir(regVal);
				if (!path.empty() && PathFileExistsA(path.c_str()))
					usernameFound = true;
			}
		}

		// try method 3
		if (!usernameFound)
		{
			std::string regVal = aja::read_registry_string(HKEY_LOCAL_MACHINE,
														   "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
														   "LastUsedUsername");
			if (!regVal.empty())
			{
				path = aja_mkpath_to_user_dir(regVal);
				if (!path.empty() && PathFileExistsA(path.c_str()))
					usernameFound = true;
			}
		}

		if (!usernameFound)
		{
			// as a last resort set to nothing, if nothing else makes the error more obvious
			path = "";
		}
		mValueMap[int(AJA_SystemInfoTag_Path_UserHome)] = path;

		if (usernameFound)
		{
			path.append("\\AppData\\Local\\AJA\\");
		}
		mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreUser)] = path;
		mValueMap[int(AJA_SystemInfoTag_Path_NTV2VirtualDevices)] = path;

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
			PathAppend(szPath, L"AJA\\");
			char tmpPath[MAX_PATH];
			::wcstombs(tmpPath,szPath,MAX_PATH);
			path.append(tmpPath);
	#else
			PathAppend(szPath, "AJA\\");
			path.append(szPath);
	#endif
			mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreSystem)] = path;
			path.append("ntv2\\");
			mValueMap[int(AJA_SystemInfoTag_Path_NTV2Plugins)] = path;
			path.append("Firmware\\");
			mValueMap[int(AJA_SystemInfoTag_Path_Firmware)] = path;
		}

		mValueMap[int(AJA_SystemInfoTag_Path_Applications)] = "C:\\Program Files\\AJA\\windows\\Applications\\";
		mValueMap[int(AJA_SystemInfoTag_Path_Utilities)] = "C:\\Program Files\\AJA\\windows\\Applications\\";

		ret = AJA_STATUS_SUCCESS;
	} //end if (sections & AJA_SystemInfoSection_Path)

	return ret;
}

//

