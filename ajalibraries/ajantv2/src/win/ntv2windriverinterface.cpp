 /*
 *
	ntv2windriverinterface.cpp

	Copyright (C) 2003, 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.

	Purpose:

	Platform dependent implementations for NTV2 Driver Interface
	on Windows.

 */

#include "ntv2windriverinterface.h"
#include "ntv2winpublicinterface.h"
#include "ntv2nubtypes.h"
#include "ntv2debug.h"
#include "winioctl.h"
#include "ajabase/system/debug.h"
#include <sstream>

using namespace std;

#ifdef _AJA_COMPILE_WIN2K_SOFT_LINK
typedef WINSETUPAPI
HDEVINFO
(WINAPI *
pfcnSetupDiGetClassDevsA)(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCSTR       Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    );

typedef WINSETUPAPI
HDEVINFO
(WINAPI *
pfcnSetupDiGetClassDevsW)(
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCWSTR      Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    );
#ifdef UNICODE
#define pfcnSetupDiGetClassDevs pfcnSetupDiGetClassDevsW
#else
#define pfcnSetupDiGetClassDevs pfcnSetupDiGetClassDevsA
#endif

typedef WINSETUPAPI
BOOL
(WINAPI *
pfcnSetupDiEnumDeviceInterfaces)(
    IN  HDEVINFO                   DeviceInfoSet,
    IN  PSP_DEVINFO_DATA           DeviceInfoData,     OPTIONAL
    IN  CONST GUID                *InterfaceClassGuid,
    IN  DWORD                      MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA  DeviceInterfaceData
    );


typedef WINSETUPAPI
BOOL
(WINAPI *
pfcnSetupDiGetDeviceInterfaceDetailA)(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
    );

typedef WINSETUPAPI
BOOL
(WINAPI *
pfcnSetupDiGetDeviceInterfaceDetailW)(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL
	);
#ifdef UNICODE
#define pfcnSetupDiGetDeviceInterfaceDetail pfcnSetupDiGetDeviceInterfaceDetailW
#else
#define pfcnSetupDiGetDeviceInterfaceDetail pfcnSetupDiGetDeviceInterfaceDetailA
#endif

typedef WINSETUPAPI
BOOL
(WINAPI *
 pfcnSetupDiDestroyDeviceInfoList)(
 IN HDEVINFO DeviceInfoSet
 );

typedef WINSETUPAPI
BOOL
(WINAPI *
 pfcnSetupDiDestroyDriverInfoList)(
 IN HDEVINFO         DeviceInfoSet,
 IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
 IN DWORD            DriverType
 );

static pfcnSetupDiGetClassDevs pSetupDiGetClassDevs = NULL;
static pfcnSetupDiEnumDeviceInterfaces pSetupDiEnumDeviceInterfaces = NULL;
static pfcnSetupDiGetDeviceInterfaceDetail pSetupDiGetDeviceInterfaceDetail = NULL;
static pfcnSetupDiDestroyDeviceInfoList pSetupDiDestroyDeviceInfoList = NULL;
static pfcnSetupDiDestroyDriverInfoList pSetupDiDestroyDriverInfoList = NULL;

HINSTANCE hSetupAPI = NULL;

#endif

static const ULWord	gChannelToTSLastOutputVertLo []		= {	kVRegTimeStampLastOutputVerticalLo, kVRegTimeStampLastOutput2VerticalLo, kVRegTimeStampLastOutput3VerticalLo, kVRegTimeStampLastOutput4VerticalLo,
															kVRegTimeStampLastOutput5VerticalLo, kVRegTimeStampLastOutput6VerticalLo, kVRegTimeStampLastOutput7VerticalLo, kVRegTimeStampLastOutput8VerticalLo, 0};

static const ULWord	gChannelToTSLastOutputVertHi []		= {	kVRegTimeStampLastOutputVerticalHi, kVRegTimeStampLastOutput2VerticalHi, kVRegTimeStampLastOutput3VerticalHi, kVRegTimeStampLastOutput4VerticalHi,
															kVRegTimeStampLastOutput5VerticalHi, kVRegTimeStampLastOutput6VerticalHi, kVRegTimeStampLastOutput7VerticalHi, kVRegTimeStampLastOutput8VerticalHi, 0};

static const ULWord	gChannelToTSLastInputVertLo []		= {	kVRegTimeStampLastInput1VerticalLo, kVRegTimeStampLastInput2VerticalLo, kVRegTimeStampLastInput3VerticalLo, kVRegTimeStampLastInput4VerticalLo,
															kVRegTimeStampLastInput5VerticalLo, kVRegTimeStampLastInput6VerticalLo, kVRegTimeStampLastInput7VerticalLo, kVRegTimeStampLastInput8VerticalLo, 0};

static const ULWord	gChannelToTSLastInputVertHi []		= {	kVRegTimeStampLastInput1VerticalHi, kVRegTimeStampLastInput2VerticalHi, kVRegTimeStampLastInput3VerticalHi, kVRegTimeStampLastInput4VerticalHi,
															kVRegTimeStampLastInput5VerticalHi, kVRegTimeStampLastInput6VerticalHi, kVRegTimeStampLastInput7VerticalHi, kVRegTimeStampLastInput8VerticalHi, 0};
static std::string GetKernErrStr (const DWORD inError)
{
	LPVOID lpMsgBuf(NULL);
	FormatMessage (	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					inError,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL);
	string result (lpMsgBuf ? reinterpret_cast<const char*>(lpMsgBuf) : "");
	LocalFree (lpMsgBuf);
//	Truncate at <CR><LF>...
	const size_t	crPos(result.find(".\r"));
	if (crPos != string::npos)
		result.resize(crPos);
	return result;
}


//	WinDriverInterface Logging Macros
#define	HEX2(__x__)			"0x" << hex << setw(2)  << setfill('0') << (0xFF       & uint8_t (__x__)) << dec
#define	HEX4(__x__)			"0x" << hex << setw(4)  << setfill('0') << (0xFFFF     & uint16_t(__x__)) << dec
#define	HEX8(__x__)			"0x" << hex << setw(8)  << setfill('0') << (0xFFFFFFFF & uint32_t(__x__)) << dec
#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') <<               uint64_t(__x__)  << dec
#define KR(_kr_)			"kernResult=" << HEX8(_kr_) << "(" << GetKernErrStr(_kr_) << ")"
#define INSTP(_p_)			" instance=" << HEX16(uint64_t(_p_))

#define	WDIFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_DriverInterface, __FUNCTION__ << ": " << __x__)
#define	WDIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface, __FUNCTION__ << ": " << __x__)
#define	WDINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface, __FUNCTION__ << ": " << __x__)
#define	WDIINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_DriverInterface, __FUNCTION__ << ": " << __x__)
#define	WDIDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_DriverInterface, __FUNCTION__ << ": " << __x__)



/////////////////////////////////////////////////////////////////////////////////////
// Board Open / Close methods
/////////////////////////////////////////////////////////////////////////////////////

// Method: Open
// Input:  UWord boardNumber(starts at 0)
//         bool displayErrorMessage
//         ULWord shareMode  Specifies how the object can be shared.
//            If dwShareMode is 0, the object cannot be shared.
//            Subsequent open operations on the object will fail,
//            until the handle is closed.
//            defaults to FILE_SHARE_READ | FILE_SHARE_WRITE
//            which allows multiple opens on the same device.
// Output: bool - true if opened ok.
bool CNTV2WinDriverInterface::Open (UWord inDeviceIndexNumber, const string & hostName)
{
	// Check if already opened
	if (IsOpen())
	{
		// Don't do anything if the requested board is the same as last opened, and
		// the requested or last opened board aren't remote boards
		if ( (_boardNumber == inDeviceIndexNumber) && hostName.empty())
			#if defined(NTV2_NUB_CLIENT_SUPPORT)
				if (_remoteHandle == INVALID_NUB_HANDLE)
			#endif	//	NTV2_NUB_CLIENT_SUPPORT
				return true;

		Close();   // Close current board and open desired board
	}

#if defined(NTV2_NUB_CLIENT_SUPPORT)
	_remoteHandle = (LWord)INVALID_NUB_HANDLE;
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	_hDevice = INVALID_HANDLE_VALUE;
	ULWord deviceID = 0x0;

#define BOARDSTRMAX	32
	if (!hostName.empty())	// Non-empty: card on remote host
	{
		ostringstream	oss;
		oss << hostName << ":ntv2" << inDeviceIndexNumber;
	#if defined(NTV2_NUB_CLIENT_SUPPORT)
		if (!OpenRemote(inDeviceIndexNumber, _displayErrorMessage, 256, oss.str().c_str()))
	#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
		{
			WDIFAIL("Failed to open remote device '" << oss.str() << "'");
			return false;
		}
	}
	else
	{
		{
			DEFINE_GUIDSTRUCT("844B39E5-C98E-45a1-84DE-3BAF3F4F9F14", AJAVIDEO_NTV2_PROPSET);
#define AJAVIDEO_NTV2_PROPSET DEFINE_GUIDNAMED(AJAVIDEO_NTV2_PROPSET)
			_GUID_PROPSET = AJAVIDEO_NTV2_PROPSET;
		}

		REFGUID refguid = _GUID_PROPSET;
		_boardNumber = inDeviceIndexNumber;

		DWORD dwShareMode;
		if (_bOpenShared)
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		else
			dwShareMode = 0x0;

		DWORD dwFlagsAndAttributes;
		if (_bOpenOverlapped)
			dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
		else
			dwFlagsAndAttributes = 0x0;

		dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;

		DWORD dwReqSize=0;
		SP_DEVICE_INTERFACE_DATA spDevIFaceData;
		memset(&spDevIFaceData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
		GUID myguid = refguid;  // an un-const guid for compiling with new Platform SDK!
		_hDevInfoSet = SetupDiGetClassDevs(&myguid,NULL,NULL,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
		if(_hDevInfoSet==INVALID_HANDLE_VALUE)
		{
			return false;
		}
		spDevIFaceData.cbSize=sizeof(SP_DEVICE_INTERFACE_DATA);
		myguid = refguid;
		if(!SetupDiEnumDeviceInterfaces(_hDevInfoSet,NULL,&myguid,_boardNumber,&spDevIFaceData))
		{
			SetupDiDestroyDeviceInfoList(_hDevInfoSet);
			return false;
		}

		if(SetupDiGetDeviceInterfaceDetail(_hDevInfoSet,&spDevIFaceData,NULL,0,&dwReqSize,NULL))
		{
			SetupDiDestroyDeviceInfoList(_hDevInfoSet);
			return false; //should have failed!
		}
		if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
		{
			SetupDiDestroyDeviceInfoList(_hDevInfoSet);
			return false;
		}
		_pspDevIFaceDetailData=(PSP_DEVICE_INTERFACE_DETAIL_DATA) new BYTE[dwReqSize];
		if(!(_pspDevIFaceDetailData))
		{
			SetupDiDestroyDeviceInfoList(_hDevInfoSet);
			return false; // out of memory
		}

		memset(&_spDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
		_spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		_pspDevIFaceDetailData->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		//now we are setup to get the info we want!
		if(!SetupDiGetDeviceInterfaceDetail(_hDevInfoSet ,&spDevIFaceData,_pspDevIFaceDetailData, dwReqSize,NULL,&_spDevInfoData))
		{
			delete _pspDevIFaceDetailData;
			_pspDevIFaceDetailData=NULL;
			SetupDiDestroyDeviceInfoList(_hDevInfoSet);
			return false; // out of memory
		}

		ULONG deviceInstanceSize = 0;
		CM_Get_Device_ID_Size(&deviceInstanceSize, _spDevInfoData.DevInst, 0);
		char* deviceInstance = (char*)new BYTE[deviceInstanceSize*2];
		CM_Get_Device_IDA(_spDevInfoData.DevInst, deviceInstance, deviceInstanceSize*2, 0);
		string sDeviceInstance(deviceInstance);
		delete deviceInstance;
		if(sDeviceInstance.find("DB") != string::npos)
		{
			string sDeviceID = sDeviceInstance.substr(sDeviceInstance.find("DB"),4);
			if(sDeviceID.compare("DB01") == 0)
				deviceID = 0xDB01;
			else if(sDeviceID.compare("DB02") == 0)
				deviceID = 0xDB02;
			else if(sDeviceID.compare("DB03") == 0)
				deviceID = 0xDB03;
			else if(sDeviceID.compare("DB04") == 0)
				deviceID = 0xDB04;
			else if(sDeviceID.compare("DB05") == 0)
				deviceID = 0xDB05;
			else if(sDeviceID.compare("DB06") == 0)
				deviceID = 0xDB06;
			else if(sDeviceID.compare("DB07") == 0)
				deviceID = 0xDB07;
			else if(sDeviceID.compare("DB08") == 0)
				deviceID = 0xDB08;
			else if(sDeviceID.compare("DB09") == 0)
				deviceID = 0xDB09;
			else if(sDeviceID.compare("DB10") == 0)
				deviceID = 0xDB10;
			else if(sDeviceID.compare("DB11") == 0)
				deviceID = 0xDB11;
			else
				deviceID = 0x0;
		}
		else
			deviceID = 0x0;

		_hDevice = CreateFile(_pspDevIFaceDetailData->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			dwShareMode,
			NULL,
			OPEN_EXISTING,
			dwFlagsAndAttributes,
			NULL);

		if(_hDevice == INVALID_HANDLE_VALUE)
		{
			delete _pspDevIFaceDetailData;
			_pspDevIFaceDetailData=NULL;
			SetupDiDestroyDeviceInfoList(_hDevInfoSet);
			_hDevInfoSet=NULL;
			return false;
		}
	}

	_boardOpened = true;

#if AJA_NTV2_SDK_VERSION_MAJOR != 0
	ULWord driverVersionMajor;
	if (!ReadRegister (kVRegLinuxDriverVersion, &driverVersionMajor))
	{
		WDIFAIL("Cannot read driver version");
		Close();
		return false;
	}
	driverVersionMajor = NTV2DriverVersionDecode_Major(driverVersionMajor);
	if (driverVersionMajor != (ULWord)AJA_NTV2_SDK_VERSION_MAJOR)
	{
		printf("## ERROR:  Cannot open:  Driver version %d older than SDK version %d\n",
				driverVersionMajor, AJA_NTV2_SDK_VERSION_MAJOR);
		Close();
		return false;
	}
#endif

	CNTV2DriverInterface::ReadRegister(kRegBoardID, _boardID);
	NTV2FrameGeometry fg;
	CNTV2DriverInterface::ReadRegister (kRegGlobalControl, fg, kRegMaskGeometry, kRegShiftGeometry);

	ULWord returnVal1,returnVal2;
	ReadRegister (kRegCh1Control,returnVal1,kRegMaskFrameFormat,kRegShiftFrameFormat);
	ReadRegister (kRegCh1Control,returnVal2,kRegMaskFrameFormatHiBit,kRegShiftFrameFormatHiBit);
	NTV2FrameBufferFormat format = (NTV2FrameBufferFormat)((returnVal1&0x0f) | ((returnVal2&0x1)<<4));

	// Write the device ID
	//WriteRegister(kVRegPCIDeviceID, deviceID);

	InitMemberVariablesOnOpen(fg,format);    // in the base class

	return true;
}

#if !defined(NTV2_DEPRECATE_14_3)
	bool CNTV2WinDriverInterface::Open (	UWord			boardNumber,
											bool			displayError,
											NTV2DeviceType	eBoardType,
											const char 	*	hostname)
	{
		(void) eBoardType;
		const string host(hostname ? hostname : "");
		_displayErrorMessage = displayError;
		return Open(boardNumber, host);
	}
#endif	//	!defined(NTV2_DEPRECATE_14_3)

// Method:	SetShareMode
// Input:	bool mode
bool CNTV2WinDriverInterface::SetShareMode (bool bShared)
{
	_bOpenShared = bShared;
	return true;
}

// Method:	SetOverlappedMode
// Input:	bool mode
bool CNTV2WinDriverInterface::SetOverlappedMode (bool bOverlapped)
{
	_bOpenOverlapped = bOverlapped;
	return true;
}

// Method: Close
// Input:  NONE
// Output: NONE
bool CNTV2WinDriverInterface::Close()
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		return CloseRemote();
	}
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
    // If board already closed, return true
    if (!_boardOpened)
        return true;

	{
		assert( _hDevice );
		if(_pspDevIFaceDetailData)
		{
			delete _pspDevIFaceDetailData;
			_pspDevIFaceDetailData=NULL;
		}
		if(_hDevInfoSet)
		{
#ifndef _AJA_COMPILE_WIN2K_SOFT_LINK
			SetupDiDestroyDeviceInfoList(_hDevInfoSet);
#else
			if(pSetupDiDestroyDeviceInfoList != NULL) {
				pSetupDiDestroyDeviceInfoList(_hDevInfoSet);
			}
#endif
			_hDevInfoSet=NULL;
		}

		// oem additions
		UnmapFrameBuffers ();
		ConfigureSubscription (false, eOutput1, mInterruptEventHandles [eOutput1]);
		ConfigureSubscription (false, eOutput2, mInterruptEventHandles [eOutput2]);
		ConfigureSubscription (false, eOutput3, mInterruptEventHandles [eOutput3]);
		ConfigureSubscription (false, eOutput4, mInterruptEventHandles [eOutput4]);
		ConfigureSubscription (false, eOutput5, mInterruptEventHandles [eOutput5]);
		ConfigureSubscription (false, eOutput6, mInterruptEventHandles [eOutput6]);
		ConfigureSubscription (false, eOutput7, mInterruptEventHandles [eOutput7]);
		ConfigureSubscription (false, eOutput8, mInterruptEventHandles [eOutput8]);
		ConfigureSubscription (false, eInput1, mInterruptEventHandles [eInput1]);
		ConfigureSubscription (false, eInput2, mInterruptEventHandles [eInput2]);
		ConfigureSubscription (false, eInput3, mInterruptEventHandles [eInput3]);
		ConfigureSubscription (false, eInput4, mInterruptEventHandles [eInput4]);
		ConfigureSubscription (false, eInput5, mInterruptEventHandles [eInput5]);
		ConfigureSubscription (false, eInput6, mInterruptEventHandles [eInput6]);
		ConfigureSubscription (false, eInput7, mInterruptEventHandles [eInput7]);
		ConfigureSubscription (false, eInput8, mInterruptEventHandles [eInput8]);
		ConfigureSubscription (false, eChangeEvent, mInterruptEventHandles [eChangeEvent]);
		ConfigureSubscription (false, eAudio, mInterruptEventHandles [eAudio]);
		ConfigureSubscription (false, eAudioInWrap, mInterruptEventHandles [eAudioInWrap]);
		ConfigureSubscription (false, eAudioOutWrap, mInterruptEventHandles [eAudioOutWrap]);
		ConfigureSubscription (false, eUartTx, mInterruptEventHandles [eUartTx]);
		ConfigureSubscription (false, eUartRx, mInterruptEventHandles [eUartRx]);
		ConfigureSubscription (false, eHDMIRxV2HotplugDetect, mInterruptEventHandles [eHDMIRxV2HotplugDetect]);
		ConfigureSubscription (false, eDMA1, mInterruptEventHandles [eDMA1]);
		ConfigureSubscription (false, eDMA2, mInterruptEventHandles [eDMA2]);
		ConfigureSubscription (false, eDMA3, mInterruptEventHandles [eDMA3]);
		ConfigureSubscription (false, eDMA4, mInterruptEventHandles [eDMA4]);
		DmaUnlock ();
		UnmapRegisters();

		if ( _hDevice != INVALID_HANDLE_VALUE )
			CloseHandle(_hDevice);

	}

	_hDevice = INVALID_HANDLE_VALUE;
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	_remoteHandle = (LWord)INVALID_NUB_HANDLE;
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	_boardOpened = false;

	return true;
}

// DmaUnlock
// Called from avclasses destructor to insure the process isn't terminating
//	with memory still locked - a guaranteed cause of a blue screen
bool CNTV2WinDriverInterface::DmaUnlock (void)
{
	// For every locked entry, try to free it
	for (unsigned int i = 0; i < _vecDmaLocked.size (); i++)
		CompleteMemoryForDMA (_vecDmaLocked[i]);

	// Free up any memory held in the stl vector
	_vecDmaLocked.clear ();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
// PrepareMemoryForDMA
// Passes the address of a user space allocated frame buffer and the buffer's size to the
//	kernel, where the kernel
//	creates a MDL and probes & locks the memory.  The framebuffer's userspace address is saved
//	in the kernel, and all subsequent DMA calls to this address will avoid the time penalties
//	of the MDL creation and the probe & lock.
// NOTE: When this method is used to lock new()ed memory, this memory *should*
//	be unlocked with the CompleteMemoryForDMA() method before calling delete(). The avclasses
//	destructor does call DmaUnlock() which attempts to unlock all locked pages with calls
//	to CompleteMemoryForDMA().
// NOTE: Any memory that is new()ed *should* be delete()ed before the process goes out of
//	scope.
bool CNTV2WinDriverInterface::PrepareMemoryForDMA (ULWord * pFrameBuffer, ULWord ulNumBytes)
{
	// Use NTV2_PIO as an overloaded flag to indicate this is not a DMA transfer
	bool bRet = DmaTransfer (NTV2_PIO, true, 0, pFrameBuffer, 0, ulNumBytes, false);
	// If succeeded, add pFrameBuffer to the avclasses' vector of locked memory
	if (bRet)
		_vecDmaLocked.push_back (pFrameBuffer);

	return bRet;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// CompleteMemoryForDMA: unlock and free resources
// The inverse of PrepareMemoryForDMA.  It passes the framebuffer address to the kernel, where
//	the kernel looks up this address and unlocks the memory, unmaps the memory, and deletes the
//	MDL.
// NOTE: this method does not cleanup the call to new() which created the memory.  It is the new()
//	caller's responsibility to call delete() after calling this method to finish the cleanup.
bool CNTV2WinDriverInterface::CompleteMemoryForDMA (ULWord * pFrameBuffer)
{
	// Use NTV2_PIO as an overloaded flag to indicate this is not a DMA transfer
	bool bRet = DmaTransfer (NTV2_PIO, false, 0, pFrameBuffer, 0, 0);
	// If succeeded, remove pFrameBuffer from the avclasses' vector of locked memory
	if (bRet)
	{
		// Find the entry in the avclasses vector that holds this framebuffer's address
		for (vecIter = _vecDmaLocked.begin (); vecIter != _vecDmaLocked.end (); vecIter++)
		{
			// If we've found one, erase (delete) it
			if (*vecIter == pFrameBuffer)
			{
				_vecDmaLocked.erase (vecIter);
				break;
			}
		}
	}

	return bRet;
}

///////////////////////////////////////////////////////////////////////////////////
// Read and Write Register methods
///////////////////////////////////////////////////////////////////////////////////


bool CNTV2WinDriverInterface::ReadRegister(const ULWord registerNumber, ULWord & registerValue, const ULWord registerMask,
							   const ULWord registerShift)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::ReadRegister(
					registerNumber,
					registerValue,
					registerMask,
					registerShift))
		{
			DisplayNTV2Error("NTV2ReadRegisterRemote failed");
			return false;
		}
		return true;
	}
	else
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	{
		NTV2_ASSERT (registerShift < 32);

		KSPROPERTY_AJAPROPS_GETSETREGISTER_S propStruct;
		DWORD dwBytesReturned = 0;

		ZeroMemory(&propStruct,sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S));
		propStruct.Property.Set = _GUID_PROPSET;
		propStruct.Property.Id  = KSPROPERTY_AJAPROPS_GETSETREGISTER;
		propStruct.Property.Flags	= KSPROPERTY_TYPE_GET;
		propStruct.RegisterID		= registerNumber;
		propStruct.ulRegisterMask	= registerMask;
		propStruct.ulRegisterShift	= registerShift;

		BOOL fRet = FALSE;
		fRet = DeviceIoControl(_hDevice,
								IOCTL_AJAPROPS_GETSETREGISTER,
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S),
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S),
								&dwBytesReturned,
								NULL);
		if (fRet)
		{
			registerValue = propStruct.ulRegisterValue;
			return true;
		}
		else
		{
	#if defined (_DEBUG)
			LPVOID lpMsgBuf;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL
				);

			_tprintf (_T("ReadRegister regNumber=%d, regMask=0x%X, regShift=%d failed: %s\n"), registerNumber,
				registerMask, registerShift, (TCHAR *) lpMsgBuf);
			LocalFree (lpMsgBuf);
	#endif
			DisplayNTV2Error ("ReadRegister failed");

			return false;
		}
	}
}


bool CNTV2WinDriverInterface::WriteRegister (ULWord registerNumber,ULWord registerValue, ULWord registerMask,
								 ULWord registerShift)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::WriteRegister(
					registerNumber,
					registerValue,
					registerMask,
					registerShift))
		{
			DisplayNTV2Error("NTV2WriteRegisterRemote failed");
			return false;
		}
		return true;
	}
	else
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	{
		KSPROPERTY_AJAPROPS_GETSETREGISTER_S propStruct;
		DWORD dwBytesReturned = 0;
		NTV2_ASSERT (registerShift < 32);

		ZeroMemory(&propStruct,sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S));
		propStruct.Property.Set	= _GUID_PROPSET;
		propStruct.Property.Id	= KSPROPERTY_AJAPROPS_GETSETREGISTER;
		propStruct.Property.Flags  = KSPROPERTY_TYPE_SET;
		propStruct.RegisterID	   = registerNumber;
		propStruct.ulRegisterValue = registerValue;
		propStruct.ulRegisterMask  = registerMask;
		propStruct.ulRegisterShift = registerShift;

		BOOL fRet =  FALSE;
		fRet = DeviceIoControl(_hDevice,
								IOCTL_AJAPROPS_GETSETREGISTER,
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S),
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_GETSETREGISTER_S),
								&dwBytesReturned,
								NULL);
		if (fRet)
			return true;
		else
			return false;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Interrupt enabling / disabling method
/////////////////////////////////////////////////////////////////////////////

// Method:	ConfigureInterrupt
// Input:	bool bEnable (turn on/off interrupt), INTERRUPT_ENUMS eInterruptType
// Output:	bool status
// Purpose:	Provides a 1 point connection to driver for interrupt calls
bool CNTV2WinDriverInterface::ConfigureInterrupt (bool bEnable, INTERRUPT_ENUMS eInterruptType)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_INTERRUPTS_S propStruct;	// boilerplate AVStream Property structure
	DWORD dwBytesReturned = 0;

	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_INTERRUPTS_S));
	propStruct.Property.Set= _GUID_PROPSET;
	propStruct.Property.Id = KSPROPERTY_AJAPROPS_INTERRUPTS;
	if (bEnable)
		propStruct.Property.Flags = KSPROPERTY_TYPE_GET;	// driver enable is a "GET"
	else
		propStruct.Property.Flags = KSPROPERTY_TYPE_SET;	// driver disable is a "SET"
	propStruct.eInterrupt = eInterruptType;

	BOOL
	 fRet =  FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_INTERRUPTS,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_INTERRUPTS_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_INTERRUPTS_S),
							&dwBytesReturned,
							NULL);
	if (fRet)
		return true;
	else
	{
#if defined (_DEBUG)
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
			);

		_tprintf (_T("ConfigureInterrupt(bool bEnable = %d, Type = %d) failed: %s\n"), (int) bEnable,
			(int) eInterruptType, (TCHAR *) lpMsgBuf);
		LocalFree (lpMsgBuf);
#endif
		DisplayNTV2Error ("ConfigureInterrupt failed");

		return false;
	}
}

// Method: ConfigureSubscriptions
// Input:  bool bSubscribe (true if subscribing), INTERRUPT_ENUMS eInterruptType,
//		HANDLE & hSubcription
// Output: HANDLE & hSubcription (if subscribing)
// Notes:  collects all driver calls for subscriptions in one place
bool CNTV2WinDriverInterface::ConfigureSubscription (bool bSubscribe, INTERRUPT_ENUMS eInterruptType,
			PULWord & ulSubscription)
{
	CNTV2DriverInterface::ConfigureSubscription (bSubscribe, eInterruptType, ulSubscription);
	ulSubscription = mInterruptEventHandles [eInterruptType];

	// Check for previouse call to subscribe
	if ( bSubscribe && ulSubscription )
		return true;

	// Check for valid handle to unsubscribe
	if ( !bSubscribe && !ulSubscription )
		return true;

	// Assure that the avCard has been properly opened
	HANDLE hSubscription = (HANDLE) ulSubscription;
    assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S propStruct;
	DWORD dwBytesReturned = 0;

	BOOL bRet;
	if ( true)  // 64 bit driver.
	{
		// for support of 64 bit driver only in 5.0...
		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S));
		propStruct.Property.Set= _GUID_PROPSET;
		propStruct.Property.Id = KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS;
		if (bSubscribe)
		{
			hSubscription = CreateEvent (NULL, FALSE, FALSE, NULL);
			propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
		}
		else
			propStruct.Property.Flags = KSPROPERTY_TYPE_SET;
		propStruct.Handle = hSubscription;
		propStruct.eInterrupt = eInterruptType;

		bRet = DeviceIoControl(_hDevice,
								IOCTL_AJAPROPS_NEWSUBSCRIPTIONS,
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S),
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S),
								&dwBytesReturned,
								NULL);
	}

	if (( !bSubscribe && bRet ) || ( bSubscribe && !bRet ))
	{
		CloseHandle (hSubscription);
		ulSubscription = 0;
	}

	if (!bRet)
	{
#if defined (_DEBUG)
		_tprintf (_T("ConfigureSubscription failed (bSubsribe = %d, SubscriptionType = %d\n"),
			(int) bSubscribe, (int) eInterruptType);
#endif
		DisplayNTV2Error("ConfigureSubscription failed");

	    return false;
    }

	if (bSubscribe)
		ulSubscription = (PULWord) hSubscription;

	return true;
}

// Method: getInterruptCount
// Input:  NONE
// Output: ULONG or equivalent(i.e. ULWord).
bool CNTV2WinDriverInterface::GetInterruptCount(INTERRUPT_ENUMS eInterruptType, ULWord *pCount)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	{
		KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S propStruct;
		DWORD dwBytesReturned = 0;

		ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S));
		propStruct.Property.Set	= _GUID_PROPSET;
		propStruct.Property.Id	= KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS;
		propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
		propStruct.eInterrupt = eGetIntCount;
		propStruct.ulIntCount = (ULONG) eInterruptType;

		BOOL ioctlSuccess = FALSE;
		ioctlSuccess = DeviceIoControl(_hDevice,
										IOCTL_AJAPROPS_NEWSUBSCRIPTIONS,
										&propStruct,
										sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S),
										&propStruct,
										sizeof(KSPROPERTY_AJAPROPS_NEWSUBSCRIPTIONS_S),
										&dwBytesReturned,
										NULL);
		if (!ioctlSuccess)
		{
			DisplayNTV2Error("getInterruptCount() failed");
			return false;
		}

		*pCount = propStruct.ulIntCount;
	}

	return true;
}

HANDLE CNTV2WinDriverInterface::GetInterruptEvent( INTERRUPT_ENUMS eInterruptType )
{
	return mInterruptEventHandles [eInterruptType];
}

bool CNTV2WinDriverInterface::WaitForInterrupt (INTERRUPT_ENUMS eInterruptType,
												ULWord timeOutMs)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		return CNTV2DriverInterface::WaitForInterrupt(eInterruptType,timeOutMs);
	}
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
    bool bInterruptHappened = false;    // return value

	HANDLE hEvent = mInterruptEventHandles [eInterruptType];

	if (NULL == hEvent)
	{
		// no interrupt hooked up so just use Sleep function
		Sleep (timeOutMs);
	}
	else
    {
        // interrupt hooked up. Wait
        DWORD status = WaitForSingleObject(hEvent, timeOutMs);
        if ( status == WAIT_OBJECT_0 )
        {
            bInterruptHappened = true;
            BumpEventCount (eInterruptType);
        }
		else
		{
			;//MessageBox (0, "WaitForInterrupt timed out", "CNTV2WinDriverInterface", MB_ICONERROR | MB_OK);
		}
    }

    return bInterruptHappened;
}

//////////////////////////////////////////////////////////////////////////////
// OEM Mapping to Userspace Methods
//////////////////////////////////////////////////////////////////////////////

// Method:	MapFrameBuffers
// Input:	None
// Output:	bool, and sets member _pBaseFrameAddress
bool CNTV2WinDriverInterface::MapFrameBuffers (void)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
	DWORD dwBytesReturned = 0;

	_pFrameBaseAddress = 0;
    _pCh1FrameBaseAddress = 0;
	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
	propStruct.Property.Set= _GUID_PROPSET;
	propStruct.Property.Id = KSPROPERTY_AJAPROPS_MAPMEMORY;
	propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
	propStruct.bMapType = NTV2_MAPMEMORY_FRAMEBUFFER;

	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_MAPMEMORY,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&dwBytesReturned,
							NULL);
	if (fRet)
	{
		ULWord boardIDRegister;
		ReadRegister(kRegBoardID, boardIDRegister);	//unfortunately GetBoardID is in ntv2card...ooops.
		_pFrameBaseAddress = (ULWord *) propStruct.mapMemory.Address;
		_pCh1FrameBaseAddress = _pFrameBaseAddress;
		_pCh2FrameBaseAddress = _pFrameBaseAddress;
        return true;
	}
	else
	{
#if defined (_DEBUG)
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
			);

		_tprintf (_T("MapFrameBuffers failed: %s\n"), (TCHAR *) lpMsgBuf);
		LocalFree (lpMsgBuf);
#endif
		DisplayNTV2Error ("MapFrameBuffers failed");

		return false;
	}
}

// Method:	UnmapFrameBuffers
// Input:	None
// Output:	bool status
bool CNTV2WinDriverInterface::UnmapFrameBuffers (void)
{
    ULWord boardIDRegister;
    ReadRegister(kRegBoardID, boardIDRegister);	//unfortunately GetBoardID is in ntv2card...ooops.
    ULWord * pFrameBaseAddress;
    pFrameBaseAddress = _pFrameBaseAddress;
    if (pFrameBaseAddress == 0)
        return true;

    assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

    KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
    DWORD dwBytesReturned = 0;

    ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
	propStruct.Property.Set=_GUID_PROPSET;
	propStruct.Property.Id=KSPROPERTY_AJAPROPS_MAPMEMORY;
	propStruct.Property.Flags = KSPROPERTY_TYPE_SET;
	propStruct.bMapType = NTV2_MAPMEMORY_FRAMEBUFFER;
	propStruct.mapMemory.Address = pFrameBaseAddress;

	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_MAPMEMORY,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&dwBytesReturned,
							NULL);
	_pFrameBaseAddress = 0;
    _pCh1FrameBaseAddress = 0;
    _pCh2FrameBaseAddress = 0;

	if (fRet)
		return true;
	else
	{
#if defined (_DEBUG)
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
			);

		_tprintf (_T("UnmapFrameBuffers failed: %s\n"), (char *) lpMsgBuf);
		LocalFree (lpMsgBuf);
#endif
		DisplayNTV2Error ("UnmapFrameBuffers failed");


		return false;
	}
}

// Method:	MapRegisters
// Input:	None
// Output:	bool, and sets member _pBaseFrameAddress
bool CNTV2WinDriverInterface::MapRegisters (void)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
	DWORD dwBytesReturned = 0;

	_pRegisterBaseAddress = 0;
	_pRegisterBaseAddressLength = 0;
	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
	propStruct.Property.Set=_GUID_PROPSET;
	propStruct.Property.Id =KSPROPERTY_AJAPROPS_MAPMEMORY;
	propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
	propStruct.bMapType = NTV2_MAPMEMORY_REGISTER;

	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_MAPMEMORY,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&dwBytesReturned,
							NULL);
	if (fRet)
	{
		_pRegisterBaseAddress = (ULWord *) propStruct.mapMemory.Address;
		_pRegisterBaseAddressLength = propStruct.mapMemory.Length;

		return true;
	}
	else
		return false;
}

// Method:	UnmapRegisters
// Input:	None
// Output:	bool status
bool CNTV2WinDriverInterface::UnmapRegisters (void)
{
	if (_pRegisterBaseAddress == 0)
		return true;

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
	DWORD dwBytesReturned = 0;

	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
	propStruct.Property.Set=_GUID_PROPSET;
	propStruct.Property.Id=KSPROPERTY_AJAPROPS_MAPMEMORY;
	propStruct.Property.Flags = KSPROPERTY_TYPE_SET;
	propStruct.bMapType = NTV2_MAPMEMORY_REGISTER;
	propStruct.mapMemory.Address = _pRegisterBaseAddress;

	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_MAPMEMORY,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&dwBytesReturned,
							NULL);
	_pRegisterBaseAddress = 0;
	_pRegisterBaseAddressLength = 0;
	if (fRet)
		return true;
	else
		return false;
}



// Method:	MapRegisters
// Input:	None
// Output:	bool, and sets member _pBaseFrameAddress
bool CNTV2WinDriverInterface::MapXena2Flash (void)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
	DWORD dwBytesReturned = 0;

	_pRegisterBaseAddress = 0;
	_pRegisterBaseAddressLength = 0;
	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
	propStruct.Property.Set=_GUID_PROPSET;
	propStruct.Property.Id =KSPROPERTY_AJAPROPS_MAPMEMORY;
	propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
	propStruct.bMapType = NTV2_MAPMEMORY_PCIFLASHPROGRAM;

	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_MAPMEMORY,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&dwBytesReturned,
							NULL);
	if (fRet)
	{
		_pXena2FlashBaseAddress = (ULWord *) propStruct.mapMemory.Address;
		_pRegisterBaseAddressLength = propStruct.mapMemory.Length;

		return true;
	}
	else
		return false;
}

// Method:	UnmapRegisters
// Input:	None
// Output:	bool status
bool CNTV2WinDriverInterface::UnmapXena2Flash (void)
{
	if (_pRegisterBaseAddress == 0)
		return true;

	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_MAPMEMORY_S propStruct;
	DWORD dwBytesReturned = 0;

	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S));
	propStruct.Property.Set=_GUID_PROPSET;
	propStruct.Property.Id=KSPROPERTY_AJAPROPS_MAPMEMORY;
	propStruct.Property.Flags = KSPROPERTY_TYPE_SET;
	propStruct.bMapType = NTV2_MAPMEMORY_PCIFLASHPROGRAM;
	propStruct.mapMemory.Address = _pRegisterBaseAddress;

	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_MAPMEMORY,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_MAPMEMORY_S),
							&dwBytesReturned,
							NULL);
	_pRegisterBaseAddress = 0;
	_pRegisterBaseAddressLength = 0;
	if (fRet)
		return true;
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////////////////
// DMA

bool CNTV2WinDriverInterface::DmaTransfer (NTV2DMAEngine DMAEngine, bool bRead, ULWord frameNumber,
							   ULWord * pFrameBuffer, ULWord offsetBytes, ULWord bytes,
							   bool bSync)
{
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_DMA_S propStruct;
	DWORD dwBytesReturned = 0;

	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_DMA_S));
	propStruct.Property.Set   = _GUID_PROPSET;
	propStruct.Property.Id    = KSPROPERTY_AJAPROPS_DMA;
	if (bRead)
		propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
	else
		propStruct.Property.Flags = KSPROPERTY_TYPE_SET;

	propStruct.dmaEngine = DMAEngine;
	propStruct.pvVidUserVa   = (PVOID) pFrameBuffer;
	propStruct.ulFrameNumber = frameNumber;
	propStruct.ulFrameOffset = offsetBytes;
	propStruct.ulVidNumBytes = bytes;
	propStruct.ulAudNumBytes = 0;
	propStruct.bSync		 = bSync;

	BOOL fRet = DeviceIoControl(_hDevice,
								IOCTL_AJAPROPS_DMA,
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_DMA_S),
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_DMA_S),
								&dwBytesReturned,
								NULL);
	const DWORD kernResult(GetLastError());
	if (!fRet)
		WDIFAIL (KR(kernResult) << INSTP(this) << ", eng=" << DMAEngine << ", frm=" << frameNumber
				<< ", off=" << HEX8(offsetBytes) << ", len=" << HEX8(bytes) << ", " << (bRead ? "R" : "W"));
	return fRet ? true : false;
}


 bool CNTV2WinDriverInterface::DmaTransfer (NTV2DMAEngine DMAEngine, bool bRead, ULWord frameNumber,
							   ULWord* pFrameBuffer, ULWord offsetBytes, ULWord bytes,
							   ULWord videoNumSegments, ULWord videoSegmentHostPitch, ULWord videoSegmentCardPitch,
							   bool bSync)
{
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_DMA_EX_S propStruct;
	DWORD dwBytesReturned = 0;

	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_DMA_EX_S));
	propStruct.Property.Set   = _GUID_PROPSET;
	propStruct.Property.Id    = KSPROPERTY_AJAPROPS_DMA_EX;
	if (bRead)
		propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
	else
		propStruct.Property.Flags = KSPROPERTY_TYPE_SET;

	propStruct.dmaEngine = DMAEngine;
	propStruct.pvVidUserVa = (PVOID) pFrameBuffer;
	propStruct.ulFrameNumber = frameNumber;
	propStruct.ulFrameOffset = offsetBytes;
	propStruct.ulVidNumBytes = bytes;
	propStruct.ulAudNumBytes = 0;
	propStruct.ulVidNumSegments = videoNumSegments;
	propStruct.ulVidSegmentHostPitch = videoSegmentHostPitch;
	propStruct.ulVidSegmentCardPitch = videoSegmentCardPitch;
	propStruct.bSync = bSync;

  	BOOL fRet = DeviceIoControl(_hDevice,
								IOCTL_AJAPROPS_DMA_EX,
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_DMA_EX_S),
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_DMA_EX_S),
								&dwBytesReturned,
								NULL);
	const DWORD kernResult(GetLastError());
	if (!fRet)
		WDIFAIL (KR(kernResult) << INSTP(this) << ", eng=" << DMAEngine << ", frm=" << frameNumber
				<< ", off=" << HEX8(offsetBytes) << ", len=" << HEX8(bytes) << ", " << (bRead ? "R" : "W"));
	return fRet ? true : false;
}

bool CNTV2WinDriverInterface::DmaTransfer (NTV2DMAEngine DMAEngine,
										   NTV2Channel DMAChannel,
										   bool bTarget,
										   ULWord frameNumber,
										   ULWord frameOffset,
										   ULWord videoSize,
										   ULWord videoNumSegments,
										   ULWord videoSegmentHostPitch,
										   ULWord videoSegmentCardPitch,
										   PCHANNEL_P2P_STRUCT pP2PData)
{
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	if (pP2PData == NULL)
	{
		WDIFAIL ("pP2PData == NULL");
		return false;
	}

	KSPROPERTY_AJAPROPS_DMA_P2P_S propStruct;
	DWORD dwBytesReturned = 0;

	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S));
	propStruct.Property.Set   = _GUID_PROPSET;
	propStruct.Property.Id    = KSPROPERTY_AJAPROPS_DMA_P2P;
	if (bTarget)
	{
		// reset p2p struct
		memset(pP2PData, 0, sizeof(CHANNEL_P2P_STRUCT));
		pP2PData->p2pSize = sizeof(CHANNEL_P2P_STRUCT);

		// get does target
		propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
	}
	else
	{
		// check for valid p2p struct
		if (pP2PData->p2pSize != sizeof(CHANNEL_P2P_STRUCT))
		{
			WDIFAIL ("pP2PData->p2pSize " << pP2PData->p2pSize << " != sizeof(CHANNEL_P2P_STRUCT) " <<  sizeof(CHANNEL_P2P_STRUCT));
			return false;
		}

		// set does transfer
		propStruct.Property.Flags = KSPROPERTY_TYPE_SET;
	}

	propStruct.dmaEngine = DMAEngine;
	propStruct.dmaChannel = DMAChannel;
	propStruct.ulFrameNumber = frameNumber;
	propStruct.ulFrameOffset = frameOffset;
	propStruct.ulVidNumBytes = videoSize;
	propStruct.ulVidNumSegments = videoNumSegments;
	propStruct.ulVidSegmentHostPitch = videoSegmentHostPitch;
	propStruct.ulVidSegmentCardPitch = videoSegmentCardPitch;
	propStruct.ullVideoBusAddress = pP2PData->videoBusAddress;
	propStruct.ullMessageBusAddress = pP2PData->messageBusAddress;
	propStruct.ulVideoBusSize = pP2PData->videoBusSize;
	propStruct.ulMessageData = pP2PData->messageData;

	BOOL fRet = DeviceIoControl(_hDevice,
								IOCTL_AJAPROPS_DMA_P2P,
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S),
								&propStruct,
								sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S),
								&dwBytesReturned,
								NULL);
	const DWORD kernResult(GetLastError());
	if (!fRet)
	{
		WDIFAIL (KR(kernResult) << INSTP(this) << ", eng=" << DMAEngine << " ch=" << DMAChannel << ", frm=" << frameNumber
				<< ", off=" << HEX8(frameOffset) << ", vSiz=" << HEX8(videoSize) << "#segs=" << videoNumSegments
				<< " hostPitch=" << videoSegmentHostPitch << " cardPitch=" << videoSegmentCardPitch
				<< ", target=" << (bTarget ? "T" : "F"));
		return false;
	}
	if (bTarget)
	{
		// check for data returned
		if (dwBytesReturned != sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S))
		{
			WDIFAIL (KR(kernResult) << INSTP(this) << ", eng=" << DMAEngine << " ch=" << DMAChannel << ", frm=" << frameNumber
					<< ", off=" << HEX8(frameOffset) << ", vSiz=" << HEX8(videoSize) << "#segs=" << videoNumSegments
					<< " hostPitch=" << videoSegmentHostPitch << " cardPitch=" << videoSegmentCardPitch
					<< ", target=" << (bTarget ? "T" : "F") << " p2pBytesRet=" << HEX8(dwBytesReturned)
					<< " p2pSize=" << HEX8(sizeof(KSPROPERTY_AJAPROPS_DMA_P2P_S)));
			return false;
		}

		// fill in p2p data
		pP2PData->videoBusAddress = propStruct.ullVideoBusAddress;
		pP2PData->messageBusAddress = propStruct.ullMessageBusAddress;
		pP2PData->videoBusSize = propStruct.ulVideoBusSize;
		pP2PData->messageData = propStruct.ulMessageData;
	}
	return true;
}

bool CNTV2WinDriverInterface::MapMemory (PVOID pvUserVa, ULWord ulNumBytes, bool bMap, ULWord* ulUser)
{
	assert( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	KSPROPERTY_AJAPROPS_DMA_S propStruct;
	DWORD dwBytesReturned = 0;

	ZeroMemory (&propStruct,sizeof(KSPROPERTY_AJAPROPS_DMA_S));
	propStruct.Property.Set   = _GUID_PROPSET;
	propStruct.Property.Id    = KSPROPERTY_AJAPROPS_DMA;
	if (bMap)
		propStruct.Property.Flags = KSPROPERTY_TYPE_GET;
	else
		propStruct.Property.Flags = KSPROPERTY_TYPE_SET;

	propStruct.dmaEngine = NTV2_PIO;
	propStruct.pvVidUserVa   = pvUserVa;
	propStruct.ulVidNumBytes = ulNumBytes;

	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_DMA,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_DMA_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_DMA_S),
							&dwBytesReturned,
							NULL);
	if (fRet)
	{
		if(ulUser != NULL)
		{
			*ulUser = propStruct.ulFrameOffset;
		}
		return true;
	}
	else
	{
#if defined (_DEBUG)
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
			);

		_tprintf (_T("DmaTransfer failed: %s\n"), (TCHAR *) lpMsgBuf);
		LocalFree (lpMsgBuf);
#endif
		DisplayNTV2Error ("DmaTransfer failed");

		return false;
	}
}

///////////////////////////////////////////////////////////////////////////
// AutoCirculate
bool CNTV2WinDriverInterface::AutoCirculate (AUTOCIRCULATE_DATA &autoCircData)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::AutoCirculate(autoCircData))
		{
			DisplayNTV2Error("NTV2AutoCirculateRemote failed");
			return false;
		}
		return true;
	}
	else
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	{
		bool bRes = true;
		DWORD	dwBytesReturned = 0;

		switch (autoCircData.eCommand)
		{
		case eInitAutoCirc:
			if((autoCircData.lVal4 <= 1) && 
			   (autoCircData.lVal5 == 0) &&
			   (autoCircData.lVal6 == 0))
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S autoCircControl;
				memset(&autoCircControl, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S));
				autoCircControl.Property.Set	= _GUID_PROPSET;
				autoCircControl.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL;
				autoCircControl.Property.Flags	= KSPROPERTY_TYPE_SET;
				autoCircControl.channelSpec		= autoCircData.channelSpec;
				autoCircControl.eCommand		= autoCircData.eCommand;

				autoCircControl.lVal1 = autoCircData.lVal1;
				autoCircControl.lVal2 = autoCircData.lVal2;
				autoCircControl.lVal3 = autoCircData.lVal3;
				autoCircControl.bVal1 = autoCircData.bVal1;
				autoCircControl.bVal2 = autoCircData.bVal2;
				autoCircControl.bVal3 = autoCircData.bVal3;
				autoCircControl.bVal4 = autoCircData.bVal4;
				autoCircControl.bVal5 = autoCircData.bVal5;
				autoCircControl.bVal6 = autoCircData.bVal6;
				autoCircControl.bVal7 = autoCircData.bVal7;
				autoCircControl.bVal8 = autoCircData.bVal8;

				BOOL fRet = FALSE;
				fRet = DeviceIoControl(_hDevice,
										IOCTL_AJAPROPS_AUTOCIRC_CONTROL,
										&autoCircControl,
										sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S),
										&autoCircControl,
										sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S),
										&dwBytesReturned,
										NULL);
				if (fRet)
				{
					bRes = true;
				}
				else
				{
					bRes = false;
#if defined (_DEBUG)
					LPVOID lpMsgBuf;
					FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER |
						FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL
						);

					printf ("AutoCirculate initialize failed: %s\n", (char *) lpMsgBuf);
					LocalFree (lpMsgBuf);
#endif
				}
			}
			else
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S autoCircControl;
				memset(&autoCircControl, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S));
				autoCircControl.Property.Set	= _GUID_PROPSET;
				autoCircControl.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX;
				autoCircControl.Property.Flags	= KSPROPERTY_TYPE_SET;
				autoCircControl.channelSpec		= autoCircData.channelSpec;
				autoCircControl.eCommand		= autoCircData.eCommand;

				autoCircControl.lVal1 = autoCircData.lVal1;
				autoCircControl.lVal2 = autoCircData.lVal2;
				autoCircControl.lVal3 = autoCircData.lVal3;
				autoCircControl.lVal4 = autoCircData.lVal4;
				autoCircControl.lVal5 = autoCircData.lVal5;
				autoCircControl.lVal6 = autoCircData.lVal6;
				autoCircControl.bVal1 = autoCircData.bVal1;
				autoCircControl.bVal2 = autoCircData.bVal2;
				autoCircControl.bVal3 = autoCircData.bVal3;
				autoCircControl.bVal4 = autoCircData.bVal4;
				autoCircControl.bVal5 = autoCircData.bVal5;
				autoCircControl.bVal6 = autoCircData.bVal6;
				autoCircControl.bVal7 = autoCircData.bVal7;
				autoCircControl.bVal8 = autoCircData.bVal8;

				BOOL fRet = FALSE;
				fRet = DeviceIoControl(_hDevice,
										IOCTL_AJAPROPS_AUTOCIRC_CONTROL_EX,
										&autoCircControl,
										sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S),
										&autoCircControl,
										sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_EX_S),
										&dwBytesReturned,
										NULL);
				if (fRet)
				{
					bRes = true;
				}
				else
				{
					bRes = false;
#if defined (_DEBUG)
					LPVOID lpMsgBuf;
					FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER |
						FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL
						);

					printf ("AutoCirculate initialize failed: %s\n", (char *) lpMsgBuf);
					LocalFree (lpMsgBuf);
#endif
				}
			}
			break;
		case eStartAutoCirc:
		case eStopAutoCirc:
		case eAbortAutoCirc:
		case ePauseAutoCirc:
		case eFlushAutoCirculate:
		case ePrerollAutoCirculate:
		case eStartAutoCircAtTime:
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S autoCircControl;
				memset(&autoCircControl, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S));
				autoCircControl.Property.Set	= _GUID_PROPSET;
				autoCircControl.Property.Id		= KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL;
				autoCircControl.Property.Flags	= KSPROPERTY_TYPE_SET;
				autoCircControl.channelSpec		= autoCircData.channelSpec;
				autoCircControl.eCommand		= autoCircData.eCommand;

				switch (autoCircData.eCommand)
				{
				case ePauseAutoCirc:
					autoCircControl.bVal1 = autoCircData.bVal1;
					break;

				case ePrerollAutoCirculate:
					autoCircControl.lVal1 = autoCircData.lVal1;
					break;

				case eStartAutoCircAtTime:
					autoCircControl.lVal1 = autoCircData.lVal1;
					autoCircControl.lVal2 = autoCircData.lVal2;
					break;
				}

				BOOL fRet = FALSE;
				fRet = DeviceIoControl(_hDevice,
										IOCTL_AJAPROPS_AUTOCIRC_CONTROL,
										&autoCircControl,
										sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S),
										&autoCircControl,
										sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_CONTROL_S),
										&dwBytesReturned,
										NULL);
				if (fRet)
				{
					bRes = true;
				}
				else
				{
					bRes = false;
#if defined (_DEBUG)
					LPVOID lpMsgBuf;
					FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER |
						FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL
						);

					printf ("AutoCirculate command failed: %s\n", (char *) lpMsgBuf);
					LocalFree (lpMsgBuf);
#endif
				}
			}
			break;

		case eGetAutoCirc:
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S autoCircStatus;
				memset(&autoCircStatus, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S));
				autoCircStatus.Property.Set	  = _GUID_PROPSET;
				autoCircStatus.Property.Id	  = KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS;
				autoCircStatus.Property.Flags = KSPROPERTY_TYPE_GET;
				autoCircStatus.channelSpec		= autoCircData.channelSpec;
				autoCircStatus.eCommand			= autoCircData.eCommand;

				if(autoCircData.pvVal1 != NULL)
				{
					BOOL fRet = FALSE;
					fRet = DeviceIoControl(_hDevice,
											IOCTL_AJAPROPS_AUTOCIRC_STATUS,
											&autoCircStatus,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S),
											&autoCircStatus,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_STATUS_S),
											&dwBytesReturned,
											NULL);
					if (fRet)
					{
						*(AUTOCIRCULATE_STATUS_STRUCT *)autoCircData.pvVal1 = autoCircStatus.autoCircStatus;
					}
					else
					{
						bRes = false;
					}
				}
				else
				{
					bRes = false;
				}
			}
			break;

		case eGetFrameStamp:
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S autoCircFrame;
				memset(&autoCircFrame, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S));
				autoCircFrame.Property.Set	  = _GUID_PROPSET;
				autoCircFrame.Property.Id	  = KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME;
				autoCircFrame.Property.Flags  = KSPROPERTY_TYPE_GET;
				autoCircFrame.channelSpec	= autoCircData.channelSpec;
				autoCircFrame.eCommand		= autoCircData.eCommand;
				autoCircFrame.lFrameNum		= autoCircData.lVal1;

				if(autoCircData.pvVal1 != NULL)
				{
					autoCircFrame.frameStamp	= *(FRAME_STAMP_STRUCT *) autoCircData.pvVal1;

					BOOL fRet = FALSE;
					fRet = DeviceIoControl(_hDevice,
											IOCTL_AJAPROPS_AUTOCIRC_FRAME,
											&autoCircFrame,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S),
											&autoCircFrame,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_S),
											&dwBytesReturned,
											NULL);
					if (fRet)
					{
						*(FRAME_STAMP_STRUCT *)autoCircData.pvVal1 = autoCircFrame.frameStamp;
					}
					else
					{
						bRes = false;
					}
				}
				else
				{
					bRes = false;
				}
			}
			break;

		case eGetFrameStampEx2:
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S autoCircFrame;
				memset(&autoCircFrame, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S));
				autoCircFrame.Property.Set	  = _GUID_PROPSET;
				autoCircFrame.Property.Id	  = KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2;
				autoCircFrame.Property.Flags  = KSPROPERTY_TYPE_GET;
				autoCircFrame.channelSpec	= autoCircData.channelSpec;
				autoCircFrame.eCommand		= autoCircData.eCommand;
				autoCircFrame.lFrameNum		= autoCircData.lVal1;

				if(autoCircData.pvVal1 != NULL)
				{
					autoCircFrame.frameStamp	= *(FRAME_STAMP_STRUCT *) autoCircData.pvVal1;
					if(autoCircData.pvVal2 != NULL)
					{
						autoCircFrame.acTask = *(AUTOCIRCULATE_TASK_STRUCT *) autoCircData.pvVal2;
					}

					BOOL fRet = FALSE;
					fRet = DeviceIoControl(_hDevice,
											IOCTL_AJAPROPS_AUTOCIRC_FRAME_EX2,
											&autoCircFrame,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S),
											&autoCircFrame,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S),
											&dwBytesReturned,
											NULL);
					if (fRet)
					{
						*(FRAME_STAMP_STRUCT *)autoCircData.pvVal1 = autoCircFrame.frameStamp;
						if(autoCircData.pvVal2 != NULL)
						{
							*(AUTOCIRCULATE_TASK_STRUCT *) autoCircData.pvVal2 = autoCircFrame.acTask;
						}
					}
					else
					{
						bRes = false;
					}
				}
				else
				{
					bRes = false;
				}
			}
			break;

		case eTransferAutoCirculate:
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S autoCircTransfer;
				memset(&autoCircTransfer, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S));
				autoCircTransfer.Property.Set	= _GUID_PROPSET;
				autoCircTransfer.Property.Id	= KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER;
				autoCircTransfer.Property.Flags = KSPROPERTY_TYPE_GET;
				autoCircTransfer.eCommand		= autoCircData.eCommand;

				if((autoCircData.pvVal1 != NULL) &&
					(autoCircData.pvVal2 != NULL))
				{
					autoCircTransfer.acTransfer = *(PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;
					// if doing audio, insure buffer alignment is OK
					if (autoCircTransfer.acTransfer.audioBufferSize)
					{
						if (autoCircTransfer.acTransfer.audioBufferSize % 4)
						{
							bRes = false;
							DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer size not mod 4");
							break;
						}

						if ((ULWord64) autoCircTransfer.acTransfer.audioBuffer % 4)
						{
							bRes = false;
							DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer address not mod 4");
							break;
						}
					}

					AUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = *(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT) autoCircData.pvVal2;

					BOOL fRet = FALSE;
					fRet = DeviceIoControl(_hDevice,
											IOCTL_AJAPROPS_AUTOCIRC_TRANSFER,
											&autoCircTransfer,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_S),
											&acStatus,
											sizeof (AUTOCIRCULATE_TRANSFER_STATUS_STRUCT),
											&dwBytesReturned,
											NULL);
					if (fRet)
					{
						*(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2 = acStatus;
					}
					else
					{
						if ( _displayErrorMessage )
						{
							LPVOID lpMsgBuf;
							FormatMessage(
								FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_SYSTEM |
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL
								);
							MessageBox (0, (const TCHAR*)lpMsgBuf, _T("CNTV2WinDriverInterface"), MB_ICONERROR | MB_OK);

							_tprintf (_T("AutoCirculate transfer failed: %s\n"), (const TCHAR *) lpMsgBuf);
							LocalFree (lpMsgBuf);
						}
						bRes = false;
					}
				}
				else
				{
					bRes = false;
				}
			}
			break;

		case eTransferAutoCirculateEx:
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S autoCircTransfer;
				memset(&autoCircTransfer, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S));
				autoCircTransfer.Property.Set	= _GUID_PROPSET;
				autoCircTransfer.Property.Id	= KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX;
				autoCircTransfer.Property.Flags = KSPROPERTY_TYPE_GET;
				autoCircTransfer.eCommand		= autoCircData.eCommand;

				if((autoCircData.pvVal1 != NULL) &&
					(autoCircData.pvVal2 != NULL))
				{
					autoCircTransfer.acTransfer = *(PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;
					if(autoCircData.pvVal3 != NULL)
					{
						autoCircTransfer.acTransferRoute = *(NTV2RoutingTable*) autoCircData.pvVal3;
					}
					// if doing audio, insure buffer alignment is OK
					if (autoCircTransfer.acTransfer.audioBufferSize)
					{
						if (autoCircTransfer.acTransfer.audioBufferSize % 4)
						{
							bRes = false;
							DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer size not mod 4");
							break;
						}

						if ((ULWord64) autoCircTransfer.acTransfer.audioBuffer % 4)
						{
							bRes = false;
							DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer address not mod 4");
							break;
						}
					}

					AUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = *(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT) autoCircData.pvVal2;

					BOOL fRet = FALSE;
					fRet = DeviceIoControl(_hDevice,
											IOCTL_AJAPROPS_AUTOCIRC_TRANSFER_EX,
											&autoCircTransfer,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX_S),
											&acStatus,
											sizeof (AUTOCIRCULATE_TRANSFER_STATUS_STRUCT),
											&dwBytesReturned,
											NULL);
					if (fRet)
					{
						*(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2 = acStatus;
					}
					else
					{
						if ( _displayErrorMessage )
						{
							LPVOID lpMsgBuf;
							FormatMessage(
								FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_SYSTEM |
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL
								);
							MessageBox (0, (const TCHAR*)lpMsgBuf, _T("CNTV2WinDriverInterface"), MB_ICONERROR | MB_OK);

							_tprintf (_T("AutoCirculate transfer failed: %s\n"), (const TCHAR *) lpMsgBuf);
							LocalFree (lpMsgBuf);
						}
						bRes = false;
					}
				}
				else
				{
					bRes = false;
				}
			}
			break;

		case eTransferAutoCirculateEx2:
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S autoCircTransfer;
				memset(&autoCircTransfer, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S));
				autoCircTransfer.Property.Set	= _GUID_PROPSET;
				autoCircTransfer.Property.Id	= KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2;
				autoCircTransfer.Property.Flags = KSPROPERTY_TYPE_GET;
				autoCircTransfer.eCommand		= autoCircData.eCommand;

				if((autoCircData.pvVal1 != NULL) &&
					(autoCircData.pvVal2 != NULL))
				{
					autoCircTransfer.acTransfer = *(PAUTOCIRCULATE_TRANSFER_STRUCT) autoCircData.pvVal1;
					if(autoCircData.pvVal3 != NULL)
					{
						autoCircTransfer.acTransferRoute = *(NTV2RoutingTable*) autoCircData.pvVal3;
					}
					if(autoCircData.pvVal4 != NULL)
					{
						autoCircTransfer.acTask = *(PAUTOCIRCULATE_TASK_STRUCT) autoCircData.pvVal4;
					}
					// if doing audio, insure buffer alignment is OK
					if (autoCircTransfer.acTransfer.audioBufferSize)
					{
						if (autoCircTransfer.acTransfer.audioBufferSize % 4)
						{
							bRes = false;
							DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer size not mod 4");
							break;
						}

						if ((ULWord64) autoCircTransfer.acTransfer.audioBuffer % 4)
						{
							bRes = false;
							DisplayNTV2Error ("TransferAutoCirculate failed - audio buffer address not mod 4");
							break;
						}
					}

					AUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = *(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT) autoCircData.pvVal2;

					BOOL fRet = FALSE;
					fRet = DeviceIoControl(_hDevice,
											IOCTL_AJAPROPS_AUTOCIRC_TRANSFER_EX2,
											&autoCircTransfer,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_TRANSFER_EX2_S),
											&acStatus,
											sizeof (AUTOCIRCULATE_TRANSFER_STATUS_STRUCT),
											&dwBytesReturned,
											NULL);
					if (fRet)
					{
						*(PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT)autoCircData.pvVal2 = acStatus;
					}
					else
					{
						if ( _displayErrorMessage )
						{
							LPVOID lpMsgBuf;
							FormatMessage(
								FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_SYSTEM |
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								GetLastError(),
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL
								);
							MessageBox (0, (const TCHAR*)lpMsgBuf, _T("CNTV2WinDriverInterface"), MB_ICONERROR | MB_OK);

							_tprintf (_T("AutoCirculate transfer failed: %s\n"), (const TCHAR *) lpMsgBuf);
							LocalFree (lpMsgBuf);
						}
						bRes = false;
					}
				}
				else
				{
					bRes = false;
				}
			}
			break;

		case eSetCaptureTask:
			{
				KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S autoCircFrame;
				memset(&autoCircFrame, 0, sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S));
				autoCircFrame.Property.Set	  = _GUID_PROPSET;
				autoCircFrame.Property.Id	  = KSPROPERTY_AJAPROPS_AUTOCIRC_CAPTURE_TASK;
				autoCircFrame.Property.Flags  = KSPROPERTY_TYPE_SET;
				autoCircFrame.channelSpec	= autoCircData.channelSpec;
				autoCircFrame.eCommand		= autoCircData.eCommand;
				autoCircFrame.lFrameNum		= 0;

				if(autoCircData.pvVal1 != NULL)
				{
					autoCircFrame.acTask = *(AUTOCIRCULATE_TASK_STRUCT *) autoCircData.pvVal1;

					BOOL fRet = FALSE;
					fRet = DeviceIoControl(_hDevice,
											IOCTL_AJAPROPS_AUTOCIRC_CAPTURE_TASK,
											&autoCircFrame,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S),
											&autoCircFrame,
											sizeof(KSPROPERTY_AJAPROPS_AUTOCIRC_FRAME_EX2_S),
											&dwBytesReturned,
											NULL);
					if (!fRet)
					{
						bRes = false;
					}
				}
				else
				{
					bRes = false;
				}
			}
			break;
		}

		return bRes;
	}
}


bool CNTV2WinDriverInterface::NTV2Message(NTV2_HEADER * pInMessage)
{
	DWORD	dwBytesReturned = 0;
	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJANTV2_MESSAGE,
							pInMessage,
							pInMessage->GetSizeInBytes (),
							pInMessage,
							pInMessage->GetSizeInBytes (),
							&dwBytesReturned,
							NULL);
	return fRet ? true : false;
}


bool CNTV2WinDriverInterface::HevcSendMessage(HevcMessageHeader* pInMessage)
{
	DWORD	dwBytesReturned = 0;
	BOOL	fRet = FALSE;

	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAHEVC_MESSAGE,
							pInMessage,
							pInMessage->size,
							pInMessage,
							pInMessage->size,
							&dwBytesReturned,
							NULL);
	return fRet ? true : false;
}


bool CNTV2WinDriverInterface::ControlDriverDebugMessages(NTV2_DriverDebugMessageSet msgSet, bool enable )
{
    return false;
}


bool CNTV2WinDriverInterface::SetRelativeVideoPlaybackDelay(ULWord frameDelay)
{
	return WriteRegister (kVRegRelativeVideoPlaybackDelay, frameDelay);
}

bool CNTV2WinDriverInterface::GetRelativeVideoPlaybackDelay(ULWord* frameDelay)
{
	return frameDelay ? ReadRegister (kVRegRelativeVideoPlaybackDelay, *frameDelay) : false;
}

bool CNTV2WinDriverInterface::SetAudioRecordPinDelay(ULWord millisecondDelay)
{
	return WriteRegister (kVRegAudioRecordPinDelay, millisecondDelay);
}

bool CNTV2WinDriverInterface::GetStrictTiming(ULWord* strictTiming)
{
	return strictTiming ? ReadRegister (kVRegStrictTiming, *strictTiming) : false;
}

bool CNTV2WinDriverInterface::SetStrictTiming(ULWord strictTiming)
{
	return WriteRegister (kVRegStrictTiming, strictTiming);
}


bool CNTV2WinDriverInterface::GetAudioRecordPinDelay(ULWord* millisecondDelay)
{
	return millisecondDelay ? ReadRegister (kVRegAudioRecordPinDelay, *millisecondDelay) : false;
}

bool CNTV2WinDriverInterface::SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode)
{
	return WriteRegister(kVRegGlobalAudioPlaybackMode,mode);
}

bool CNTV2WinDriverInterface::GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode)
{
	return mode ? CNTV2DriverInterface::ReadRegister(kVRegGlobalAudioPlaybackMode, *mode) : false;
}

bool CNTV2WinDriverInterface::DisplayNTV2Error (const char *str)
{
	if ( _displayErrorMessage )
	{
        TCHAR* buf = 0;
#if defined (UNICODE) || defined (_UNICODE)
        int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
        buf = new TCHAR[len];
        MultiByteToWideChar(CP_UTF8, 0, str, -1, buf, len);
        const TCHAR* msg = buf;
#else
        const TCHAR* msg = str;
#endif
#if defined (_CONSOLE)
		_tprintf (_T("%s\n"), msg);
#else
		MessageBox (0, msg, _T("CNTV2WinDriverInterface"), MB_ICONERROR | MB_OK);
#endif
        delete[] buf;
        return true;
	}

	return false;
}

//
// Management of downloaded Xilinx bitfile
//
//
bool
CNTV2WinDriverInterface::DriverGetBitFileInformation(
		BITFILE_INFO_STRUCT &bitFileInfo,
		NTV2BitFileType bitFileType)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (_remoteHandle != INVALID_NUB_HANDLE)
	{
		if (!CNTV2DriverInterface::DriverGetBitFileInformation(
				bitFileInfo,
				bitFileType))
		{
			DisplayNTV2Error("NTV2DriverGetBitFileInformationRemote failed");
			return false;
		}
		return true;
	}
	else
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	{
		//The boards below only have one bitfile and there is no need to query the driver
		if(NTV2DeviceHasSPIFlash(_boardID))
		{
			if (!CNTV2DriverInterface::DriverGetBitFileInformation(
					bitFileInfo,
					bitFileType))
			{
				DisplayNTV2Error("DriverGetBitFileInformation failed");
				return false;
			}
			return true;
		}
		else
		{
			KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S propStruct;
			DWORD dwBytesReturned = 0;

			ZeroMemory(&propStruct,sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S));
			propStruct.Property.Set = _GUID_PROPSET;
			propStruct.Property.Id  = KSPROPERTY_AJAPROPS_GETSETBITFILEINFO;
			propStruct.Property.Flags	= KSPROPERTY_TYPE_GET;

			BOOL fRet = FALSE;
			fRet = DeviceIoControl(_hDevice,
									IOCTL_AJAPROPS_GETSETBITFILEINFO,
									&propStruct,
									sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S),
									&propStruct,
									sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S),
									&dwBytesReturned,
									NULL);
			if (fRet)
			{
				bitFileInfo = propStruct.bitFileInfoStruct;
				return true;
			}
			else
			{
				DisplayNTV2Error ("DriverGetBitFileInformation failed");
				return false;
			}
		}
	}
}

bool
CNTV2WinDriverInterface::DriverSetBitFileInformation(
		BITFILE_INFO_STRUCT &bitFileInfo)
{
 	KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S propStruct;
	DWORD dwBytesReturned = 0;

	ZeroMemory(&propStruct,sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S));
	propStruct.Property.Set = _GUID_PROPSET;
	propStruct.Property.Id  = KSPROPERTY_AJAPROPS_GETSETBITFILEINFO;
	propStruct.Property.Flags	= KSPROPERTY_TYPE_SET;
	propStruct.bitFileInfoStruct = bitFileInfo;

	BOOL fRet = FALSE;
	fRet = DeviceIoControl(_hDevice,
							IOCTL_AJAPROPS_GETSETBITFILEINFO,
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S),
							&propStruct,
							sizeof(KSPROPERTY_AJAPROPS_GETSETBITFILEINFO_S),
							&dwBytesReturned,
							NULL);
	if (fRet)
	{
		return true;
	}
	else
	{
		DisplayNTV2Error ("DriverGetBitFileInformation failed");
		return false;
	}
}


#include <ntv2devicefeatures.h>

bool CNTV2WinDriverInterface::RestoreHardwareProcampRegisters()
{
	bool status = WriteRegister(kVRegRestoreHardwareProcampRegisters, 0);

	return status;
}

PerfCounterTimestampMode CNTV2WinDriverInterface::GetPerfCounterTimestampMode()
{
	ULWord mode;
	ReadRegister(kVRegTimeStampMode,mode);

	return (PerfCounterTimestampMode)mode;
}

void CNTV2WinDriverInterface::SetPerfCounterTimestampMode(PerfCounterTimestampMode mode)
{
	WriteRegister(kVRegTimeStampMode,mode);
}

LWord64 CNTV2WinDriverInterface::GetLastOutputVerticalTimestamp()
{
	LWord64 value;
	ULWord loValue;
	ULWord hiValue;
	ReadRegister(kVRegTimeStampLastOutputVerticalLo,loValue);
	ReadRegister(kVRegTimeStampLastOutputVerticalHi,hiValue);
	value = loValue | ((LWord64)hiValue<<32);

	return value;
}

LWord64 CNTV2WinDriverInterface::GetLastInput1VerticalTimestamp()
{
	LWord64 value;
	ULWord loValue;
	ULWord hiValue;
	ReadRegister(kVRegTimeStampLastInput1VerticalLo,loValue);
	ReadRegister(kVRegTimeStampLastInput1VerticalHi,hiValue);
	value = loValue | ((LWord64)hiValue<<32);

	return value;
}

LWord64 CNTV2WinDriverInterface::GetLastInput2VerticalTimestamp()
{
	LWord64 value;
	ULWord loValue;
	ULWord hiValue;
	ReadRegister(kVRegTimeStampLastInput2VerticalLo,loValue);
	ReadRegister(kVRegTimeStampLastInput2VerticalHi,hiValue);
	value = loValue | ((LWord64)hiValue<<32);

	return value;
}

LWord64 CNTV2WinDriverInterface::GetLastOutputVerticalTimestamp(NTV2Channel channel)
{
	LWord64 value;
	ULWord loValue;
	ULWord hiValue;
	ReadRegister(gChannelToTSLastOutputVertLo[channel],loValue);
	ReadRegister(gChannelToTSLastOutputVertHi[channel],hiValue);
	value = loValue | ((LWord64)hiValue<<32);

	return value;
}

LWord64 CNTV2WinDriverInterface::GetLastInputVerticalTimestamp(NTV2Channel channel)
{
	LWord64 value;
	ULWord loValue;
	ULWord hiValue;
	ReadRegister(gChannelToTSLastInputVertLo[channel],loValue);
	ReadRegister(gChannelToTSLastInputVertHi[channel],hiValue);
	value = loValue | ((LWord64)hiValue<<32);

	return value;
}


bool CNTV2WinDriverInterface::NeedToLimitDMAToOneMegabyte()
{
	return false;
}

bool CNTV2WinDriverInterface::ReadRP188Registers( NTV2Channel /*channel-not-used*/, RP188_STRUCT* pRP188Data )
{
	bool bSuccess = false;
	RP188_STRUCT rp188;
	NTV2DeviceID boardID = DEVICE_ID_NOTFOUND;
	RP188SourceSelect source = kRP188SourceEmbeddedLTC;
	ULWord dbbReg, msReg, lsReg;

	CNTV2DriverInterface::ReadRegister(kRegBoardID, boardID);
	CNTV2DriverInterface::ReadRegister(kVRegRP188SourceSelect, source);
	bool bLTCPort = (source == kRP188SourceLTCPort);

	// values come from LTC port registers
	if (bLTCPort)
	{
		ULWord ltcPresent;
		ReadRegister (kRegStatus, ltcPresent, kRegMaskLTCInPresent, kRegShiftLTCInPresent);

		// there is no equivalent DBB for LTC port - we synthesize it here
		rp188.DBB = (ltcPresent) ? 0xFE000000 | NEW_SELECT_RP188_RCVD : 0xFE000000;

		// LTC port registers
		dbbReg = 0; // don't care - does not exist
		msReg = kRegLTCAnalogBits0_31;
		lsReg  = kRegLTCAnalogBits32_63;
	}
	else
	{
		// values come from RP188 registers
		NTV2Channel channel = NTV2_CHANNEL1;
		NTV2InputVideoSelect inputSelect = NTV2_Input1Select;

		if(NTV2DeviceGetNumVideoInputs(boardID) > 1)
		{

			CNTV2DriverInterface::ReadRegister (kVRegInputSelect, inputSelect);
			channel = (inputSelect == NTV2_Input1Select) ? NTV2_CHANNEL1 : NTV2_CHANNEL2;
		}
		else
		{
			channel = NTV2_CHANNEL1;
		}

		// rp188 registers
		dbbReg = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1DBB : kRegRP188InOut2DBB);
		//Check to see if TC is received
		uint32_t tcReceived = 0;
		ReadRegister(dbbReg, tcReceived, BIT(16), 16);
		if(tcReceived == 0)
			return false;//No TC recevied

		ReadRegister (dbbReg, rp188.DBB, kRegMaskRP188DBB, kRegShiftRP188DBB );
		switch(rp188.DBB)//What do we have?
		{
		default:
		case 0x01:
		case 0x02:
			{
				//We have VITC - what do we want?
				if(pRP188Data->DBB == 0x01 || pRP188Data->DBB == 0x02)
				{
					//We want VITC
					msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
					lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
					break;
				}
				else
				{
					//We want Embedded LTC, so we should check one other place
					uint32_t ltcPresent = 0;
					ReadRegister(dbbReg, ltcPresent, BIT(18), 18);
					if(ltcPresent == 1)
					{
						//Read LTC registers
						msReg  = (channel == NTV2_CHANNEL1 ? kRegLTCEmbeddedBits0_31  : kRegLTC2EmbeddedBits0_31 );
						lsReg  = (channel == NTV2_CHANNEL1 ? kRegLTCEmbeddedBits32_63 : kRegLTC2EmbeddedBits32_63);
						break;
					}
					else
						return false;
				}
			}
		case 0x00:
			{
				//We have LTC - do we want it?
				if(pRP188Data->DBB != 0x00)
					return false;
				else
				{
					msReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits0_31  : kRegRP188InOut2Bits0_31 );
					lsReg  = (channel == NTV2_CHANNEL1 ? kRegRP188InOut1Bits32_63 : kRegRP188InOut2Bits32_63);
				}
				break;
			}
		}
		//Re-Read the whole register just in case something is expecting other status values
		ReadRegister (dbbReg, rp188.DBB);
	}
	ReadRegister (msReg,  rp188.Low );
	ReadRegister (lsReg,  rp188.High);

	// register stability filter
	do
	{
		// struct copy to result
		*pRP188Data = rp188;

		// read again into local struct
		if (!bLTCPort)
			ReadRegister (dbbReg, rp188.DBB );
		ReadRegister (msReg,  rp188.Low );
		ReadRegister (lsReg,  rp188.High);

		// if the new read equals the previous read, consider it done
		if ( (rp188.DBB  == pRP188Data->DBB) &&
			(rp188.Low  == pRP188Data->Low) &&
			(rp188.High == pRP188Data->High) )
		{
			bSuccess = true;
		}

	} while (bSuccess == false);

	return true;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Output Timecode settings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::SetOutputTimecodeOffset( ULWord frames )
{
	return WriteRegister(kVRegOutputTimecodeOffset, frames);
}

bool CNTV2WinDriverInterface::GetOutputTimecodeOffset( ULWord* pFrames )
{
	return pFrames ? ReadRegister(kVRegOutputTimecodeOffset, *pFrames) : false;
}

bool CNTV2WinDriverInterface::SetOutputTimecodeType( ULWord type )
{
	return WriteRegister( kVRegOutputTimecodeType, type );
}

bool CNTV2WinDriverInterface::GetOutputTimecodeType( ULWord* pType )
{
	return pType ? ReadRegister(kVRegOutputTimecodeType, *pType) : false;
}

//--------------------------------------------------------------------------------------------------------------------
//	LockFormat
//
//	For Kona this is currently a no-op
//	For IoHD this will for bitfile swaps / Isoch channel rebuilds based on vidoe mode / video format
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::LockFormat( void )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	StartDriver
//
//	For IoHD this is currently a no-op
//	For Kona this starts the driver after all of the bitFiles have been sent to the driver.
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::StartDriver( DriverStartPhase phase )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Debug Levels
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::SetUserModeDebugLevel( ULWord level )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::GetUserModeDebugLevel( ULWord* level )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::SetKernelModeDebugLevel( ULWord level )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::GetKernelModeDebugLevel( ULWord* level )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Ping Levels
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::SetUserModePingLevel( ULWord level )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::GetUserModePingLevel( ULWord* level )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::SetKernelModePingLevel( ULWord level )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::GetKernelModePingLevel( ULWord* level )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set Latency Timer
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::SetLatencyTimerValue( ULWord value )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::GetLatencyTimerValue( ULWord* value )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Get/Set include/exclude debug filter strings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::SetDebugFilterStrings( const char* includeString,const char* excludeString )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	GetDebugFilterStrings
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::GetDebugFilterStrings( char* includeString,char* excludeString )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::ProgramXilinx( void* dataPtr, uint32_t dataSize )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::LoadBitFile( void* dataPtr, uint32_t dataSize, NTV2BitfileType bitFileType )
{
	//stub
	return false;
}

//--------------------------------------------------------------------------------------------------------------------
//	Application acquire and release stuff
//--------------------------------------------------------------------------------------------------------------------
bool CNTV2WinDriverInterface::AcquireStreamForApplicationWithReference( ULWord appCode, int32_t pid )
{
	ULWord currentCode, currentPID;
	ReadRegister(kVRegApplicationCode, currentCode);
	ReadRegister(kVRegApplicationPID, currentPID);

	HANDLE pH = OpenProcess(READ_CONTROL, false, (DWORD)currentPID);
	if(INVALID_HANDLE_VALUE != pH && NULL != pH)
	{
		CloseHandle(pH);
	}
	else
	{
		ReleaseStreamForApplication(currentCode, currentPID);
	}

	ReadRegister(kVRegApplicationCode, currentCode);
	ReadRegister(kVRegApplicationPID, currentPID);

	for(int count = 0; count < 20; count++)
	{
		if(currentPID == 0)
		{
			//Nothing has the board
			if(!WriteRegister(kVRegApplicationCode, appCode))
			{
				return false;
			}
			else
			{
				//Just in case this is not zero?
				WriteRegister(kVRegAcquireReferenceCount, 0);
				WriteRegister(kVRegAcquireReferenceCount, 1);
				return WriteRegister(kVRegApplicationPID, (ULWord)pid);
			}
		}
		else if(currentCode == appCode && currentPID == (ULWord)pid)
		{
			//This process has already acquired so bump the count
			return WriteRegister(kVRegAcquireReferenceCount, 1);
		}
		else
		{
			::Sleep(50);
			//Something has the board
//			return false;
		}
	}
	return false;
}

bool CNTV2WinDriverInterface::ReleaseStreamForApplicationWithReference( ULWord appCode, int32_t pid )
{
	ULWord currentCode, currentPID, currentCount;
	ReadRegister(kVRegApplicationCode, currentCode);
	ReadRegister(kVRegApplicationPID, currentPID);
	ReadRegister(kVRegAcquireReferenceCount, currentCount);

	if(currentCode == appCode && currentPID == (ULWord)pid)
	{
		if(currentCount > 1)
		{
			return WriteRegister(kVRegReleaseReferenceCount, 1);
		}
		else if(currentCount == 1)
		{
			return ReleaseStreamForApplication(appCode, pid);
		}
		else
		{
			return true;
		}
	}
	else
		return false;
}

bool CNTV2WinDriverInterface::AcquireStreamForApplication( ULWord appCode, int32_t pid )
{
	for (int count = 0; count < 20; count++)
	{
		if (!WriteRegister(kVRegApplicationCode, appCode))
			::Sleep(50);
		else
			return WriteRegister(kVRegApplicationPID, (ULWord)pid);
	}

	ULWord currentAppCode, currentPID;
	ReadRegister(kVRegApplicationCode, currentAppCode);
	ReadRegister(kVRegApplicationPID, currentPID);

	HANDLE hProcess = OpenProcess(SYNCHRONIZE, false, DWORD(currentPID));
	if (hProcess != NULL && hProcess != INVALID_HANDLE_VALUE)
	{
		// The process with acquistion has been opened, so test if it really is still running (a Windows process becomes signalled on termination) and close the process again.
		// If the process isn't signalled, either the test itself has failed, so we can't tell the state of the process, or the process is running and legitimately owns the
		// card; in either case, give up.
		DWORD result = WaitForSingleObject(hProcess, 0 /* No wait, just a polling test */);
		CloseHandle(hProcess);
		if (result != WAIT_OBJECT_0)
			return false;

		// The process that has acquired the card is no longer actually running, although it still exists to the OS. Release the card in its name so that we can take it over.
		ReleaseStreamForApplication(currentAppCode, currentPID);
		for (int count = 0; count < 20; count++)
		{
			if (!WriteRegister(kVRegApplicationCode, appCode))
				::Sleep(50);
			else
				return WriteRegister(kVRegApplicationPID, (ULWord)pid);
		}
	}
	else
	{// The process that has acquired the card is no longer actually running, although it still exists to the OS. Release the card in its name so that we can take it over.
		ReleaseStreamForApplication(currentAppCode, currentPID);
		for (int count = 0; count < 20; count++)
		{
			if (!WriteRegister(kVRegApplicationCode, appCode))
				::Sleep(50);
			else
				return WriteRegister(kVRegApplicationPID, (ULWord)pid);
		}
	}

	return false;
}

bool CNTV2WinDriverInterface::ReleaseStreamForApplication( ULWord appCode, int32_t pid )
{
	if(WriteRegister(kVRegReleaseApplication, (ULWord)pid))
	{
		WriteRegister(kVRegAcquireReferenceCount, 0);
		return true;//We don't really care if the count fails
	}
	else
		return false;
}

bool CNTV2WinDriverInterface::SetStreamingApplication( ULWord appCode, int32_t pid )
{
	if(!WriteRegister(kVRegForceApplicationCode, appCode))
		return false;
	else
		return WriteRegister(kVRegForceApplicationPID, (ULWord)pid);
}

bool CNTV2WinDriverInterface::GetStreamingApplication( ULWord *appCode, int32_t  *pid )
{
	if(!(appCode && ReadRegister(kVRegApplicationCode, *appCode)))
		return false;
	return pid && CNTV2DriverInterface::ReadRegister(kVRegApplicationPID, *pid);
}

bool CNTV2WinDriverInterface::SetDefaultDeviceForPID( int32_t pid )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::IsDefaultDeviceForPID( int32_t pid )
{
	//stub
	return false;
}

bool CNTV2WinDriverInterface::SuspendAudio()
{
	ReadRegister(kRegAud1Control, _previousAudioState);
	ReadRegister(kRegAud1SourceSelect, _previousAudioSelection);
	return true;
}

bool CNTV2WinDriverInterface::ResumeAudio()
{
	WriteRegister(kRegAud1Control, _previousAudioState);
	WriteRegister(kRegAud1SourceSelect, _previousAudioSelection);
	return true;
}
