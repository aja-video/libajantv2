/* SPDX-License-Identifier: MIT */
/**
    @file		main.cpp
    @brief		Implementation of NTV2VCAM class.
    @copyright	(C) 2025 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2vcam.h"

#if defined(AJALinux)
	int main (int argc, const char** argv)
	{
	    NTV2VCAM vw;
	    if (vw.Initialize(argc, argv))
	        return vw.Run();
	    return 0;
	}
#endif	//	if defined(AJALinux)

#if defined(AJA_WINDOWS)
	const AMOVIESETUP_MEDIATYPE gVcamOutputVideoPinTypes =
	{
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_UYVY
	};

	const AMOVIESETUP_PIN gVcamOutputVideoPin =
	{
		VCAM_VIDEO_PIN_NAME_W,
		FALSE,
		TRUE,
		FALSE,
		FALSE,
		&CLSID_NULL,
		NULL,
		1,
		&gVcamOutputVideoPinTypes
	};

	const AMOVIESETUP_MEDIATYPE gVcamOutputAudioPinTypes =
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_PCM
	};

	const AMOVIESETUP_PIN gVcamOutputAudioPin =
	{
		VCAM_AUDIO_PIN_NAME_W,
		FALSE,
		TRUE,
		FALSE,
		FALSE,
		&CLSID_NULL,
		NULL,
		1,
		&gVcamOutputAudioPinTypes
	};

	const AMOVIESETUP_PIN gVcamOutputPins[] =
	{
		gVcamOutputVideoPin,
		gVcamOutputAudioPin
	};

	const AMOVIESETUP_FILTER gVcamFilter =
	{
		&CLSID_VirtualWebcam,
		VCAM_FILTER_NAME_W,
		MERIT_DO_NOT_USE,
		2,
		gVcamOutputPins
	};

	CFactoryTemplate g_Templates[] =
	{
		{
			VCAM_FILTER_NAME_W,
			&CLSID_VirtualWebcam,
			NTV2VCAM::CreateInstance,
			NULL,
			&gVcamFilter
		}
	};
	int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

	extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
	BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
	{
		return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
	}

	STDAPI DllRegisterServer()
	{
		HRESULT hr = AMovieDllRegisterServer2(TRUE);
		if (SUCCEEDED(hr))
		{
			IFilterMapper2* pMapper = NULL;
			hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void**)&pMapper);
			if (SUCCEEDED(hr))
			{
				REGFILTER2 rf2FilterReg = { 1, MERIT_NORMAL, 1, &gVcamOutputVideoPin };
				pMapper->RegisterFilter(CLSID_VirtualWebcam, VCAM_FILTER_NAME_W, NULL, &CLSID_VideoInputDeviceCategory, VCAM_FILTER_NAME_W, &rf2FilterReg);

				rf2FilterReg = { 1, MERIT_NORMAL, 1, &gVcamOutputAudioPin };
				pMapper->RegisterFilter(CLSID_VirtualWebcam, VCAM_FILTER_NAME_W, NULL, &CLSID_AudioInputDeviceCategory, VCAM_FILTER_NAME_W, &rf2FilterReg);

				pMapper->Release();
			}
		}
		return hr;
	}

	STDAPI DllUnregisterServer()
	{
		HRESULT hr = AMovieDllRegisterServer2(FALSE);
		if (SUCCEEDED(hr))
		{
			IFilterMapper2* pMapper = NULL;
			hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void**)&pMapper);
			if (SUCCEEDED(hr))
			{
				pMapper->UnregisterFilter(&CLSID_VideoInputDeviceCategory, VCAM_FILTER_NAME_W, CLSID_VirtualWebcam);
				pMapper->UnregisterFilter(&CLSID_AudioInputDeviceCategory, VCAM_FILTER_NAME_W, CLSID_VirtualWebcam);
				pMapper->Release();
			}
		}
		return hr;
	}
#endif	//	if defined(AJA_WINDOWS)
