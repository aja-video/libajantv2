/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2vcam.cpp
	@brief		Implementation of NTV2VCAM class.
	@copyright	(C) 2025 AJA Video Systems, Inc.  All rights reserved.
**/
#include "ntv2vcam.h"

static const ULWord	gDemoAppSignature NTV2_FOURCC('V', 'C', 'A', 'M');
static const uint32_t gAudMaxSizeBytes(256 * 1024);	//	Max audio bytes for single frame (14.98fps 16ch 48kHz)

#if !defined(AJA_WINDOWS)
	static bool gGlobalQuit(false);
	static void SignalHandler(int inSignal)
	{
		(void)inSignal;
		gGlobalQuit = true;
	}
#else
	OutputVideoPin::OutputVideoPin(HRESULT* phr, CSource* pFilter, LPCWSTR pPinName)
		: CSourceStream(VCAM_VIDEO_PIN_NAME, phr, pFilter, pPinName)
	{
	}

	OutputVideoPin::~OutputVideoPin()
	{
	}

	STDMETHODIMP OutputVideoPin::QueryInterface(REFIID riid, void** ppv)
	{
		if (!ppv)
			return E_POINTER;

		*ppv = nullptr;

		if (riid == IID_IKsPropertySet)
			*ppv = static_cast<IKsPropertySet*>(this);
		else if (riid == IID_IAMStreamConfig)
			*ppv = static_cast<IAMStreamConfig*>(this);
		else
			return CSourceStream::QueryInterface(riid, ppv);

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) OutputVideoPin::AddRef()
	{
		return CSourceStream::AddRef();
	}

	STDMETHODIMP_(ULONG) OutputVideoPin::Release()
	{
		return CSourceStream::Release();
	}

	STDMETHODIMP OutputVideoPin::CheckMediaType(const CMediaType* pmt)
	{
		if (!pmt)
			return E_POINTER;

		if (*pmt->Type() != MEDIATYPE_Video)
			return E_INVALIDARG;

		if (*pmt->FormatType() != FORMAT_VideoInfo)
			return E_INVALIDARG;

		if (*pmt->Subtype() != MEDIASUBTYPE_UYVY)
			return E_INVALIDARG;

		return S_OK;
	}

	HRESULT OutputVideoPin::GetMediaType(CMediaType* pMediaType)
	{
		if (reinterpret_cast<NTV2VCAM*>(m_pFilter)->Initialize())
			return CopyMediaType(pMediaType, &m_mt);

		return E_FAIL;
	}

	HRESULT OutputVideoPin::FillBuffer(IMediaSample* pSample)
	{
		return reinterpret_cast<NTV2VCAM*>(m_pFilter)->GetNextFrame(this, pSample);
	}

	HRESULT OutputVideoPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pRequest)
	{
		CAutoLock cAutoLock(m_pFilter->pStateLock());

		if (!pAlloc)
			return E_POINTER;

		VIDEOINFO* pvi = reinterpret_cast<VIDEOINFO*>(m_mt.Format());
		if (!pvi)
			return E_UNEXPECTED;

		pRequest->cBuffers = 1;
		pRequest->cbBuffer = pvi->bmiHeader.biSizeImage;

		ALLOCATOR_PROPERTIES Actual;
		HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
		if (FAILED(hr))
			return hr;

		if (Actual.cbBuffer < pRequest->cbBuffer)
			return E_FAIL;

		if (Actual.cBuffers != 1)
			return E_FAIL;

		return NOERROR;
	}

	STDMETHODIMP OutputVideoPin::EnumMediaTypes(IEnumMediaTypes** ppEnum)
	{
		return CSourceStream::EnumMediaTypes(ppEnum);
	}

	STDMETHODIMP OutputVideoPin::Notify(IBaseFilter* pSender, Quality q)
	{
		return CSourceStream::Notify(pSender, q);
	}

	STDMETHODIMP OutputVideoPin::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OutputVideoPin::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD* pcbReturned)
	{
		if (guidPropSet != AMPROPSETID_Pin)
			return E_PROP_SET_UNSUPPORTED;

		if (dwPropID != AMPROPERTY_PIN_CATEGORY)
			return E_PROP_ID_UNSUPPORTED;

		if (pPropData == NULL && pcbReturned == NULL)
			return E_POINTER;

		if (pcbReturned)
			*pcbReturned = sizeof(GUID);

		if (pPropData == NULL)
			return S_OK;

		if (cbPropData < sizeof(GUID))
			return E_UNEXPECTED;

		*(GUID*)pPropData = PIN_CATEGORY_CAPTURE;

		return S_OK;
	}

	STDMETHODIMP OutputVideoPin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport)
	{
		if (guidPropSet != AMPROPSETID_Pin)
			return E_PROP_SET_UNSUPPORTED;

		if (dwPropID != AMPROPERTY_PIN_CATEGORY)
			return E_PROP_ID_UNSUPPORTED;

		if (pTypeSupport)
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;

		return S_OK;
	}

	STDMETHODIMP OutputVideoPin::SetFormat(AM_MEDIA_TYPE* pmt)
	{
		if (!pmt)
			return E_POINTER;

		if (pmt->majortype == m_mt.majortype &&
			pmt->subtype == m_mt.subtype &&
			pmt->formattype == m_mt.formattype)
		{
			m_mt = *pmt;
			return S_OK;
		}

		return VFW_E_INVALIDMEDIATYPE;
	}

	STDMETHODIMP OutputVideoPin::GetFormat(AM_MEDIA_TYPE** ppmt)
	{
		if (!ppmt)
			return E_POINTER;

		*ppmt = CreateMediaType(&m_mt);
		if (!*ppmt)
			return E_OUTOFMEMORY;

		return S_OK;
	}

	STDMETHODIMP OutputVideoPin::GetNumberOfCapabilities(int* piCount, int* piSize)
	{
		if (!piCount || !piSize)
			return E_POINTER;

		*piCount = 1;
		*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);

		return S_OK;
	}

	STDMETHODIMP OutputVideoPin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** ppmt, BYTE* pSCC)
	{
		if (!ppmt || !pSCC)
			return E_POINTER;

		if (iIndex < 0 || iIndex >= 1)
			return S_FALSE;

		CMediaType mediaType;
		if (FAILED(GetMediaType(&mediaType)))
			return E_FAIL;
		*ppmt = CreateMediaType(&mediaType);
		if (!*ppmt)
			return E_OUTOFMEMORY;

		VIDEOINFO* vih = reinterpret_cast<decltype(vih)>((*ppmt)->pbFormat);
		VIDEO_STREAM_CONFIG_CAPS* caps = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(pSCC);
		ZeroMemory(caps, sizeof(VIDEO_STREAM_CONFIG_CAPS));
		caps->guid = FORMAT_VideoInfo;
		caps->MinFrameInterval = vih->AvgTimePerFrame;
		caps->MaxFrameInterval = vih->AvgTimePerFrame;
		caps->MinOutputSize.cx = vih->bmiHeader.biWidth;
		caps->MinOutputSize.cy = vih->bmiHeader.biHeight;
		caps->MaxOutputSize = caps->MinOutputSize;
		caps->InputSize = caps->MinOutputSize;
		caps->MinCroppingSize = caps->MinOutputSize;
		caps->MaxCroppingSize = caps->MinOutputSize;
		caps->CropGranularityX = vih->bmiHeader.biWidth;
		caps->CropGranularityY = vih->bmiHeader.biHeight;
		caps->MinBitsPerSecond = vih->dwBitRate;
		caps->MaxBitsPerSecond = caps->MinBitsPerSecond;

		return S_OK;
	}

	OutputAudioPin::OutputAudioPin(HRESULT* phr, CSource* pFilter, LPCWSTR pPinName)
		: CSourceStream(VCAM_AUDIO_PIN_NAME, phr, pFilter, pPinName)
	{
	}

	OutputAudioPin::~OutputAudioPin()
	{
	}

	STDMETHODIMP OutputAudioPin::QueryInterface(REFIID riid, void** ppv)
	{
		if (!ppv)
			return E_POINTER;

		*ppv = nullptr;

		if (riid == IID_IKsPropertySet)
			*ppv = static_cast<IKsPropertySet*>(this);
		else if (riid == IID_IAMStreamConfig)
			*ppv = static_cast<IAMStreamConfig*>(this);
		else
			return CSourceStream::QueryInterface(riid, ppv);

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) OutputAudioPin::AddRef()
	{
		return CSourceStream::AddRef();
	}

	STDMETHODIMP_(ULONG) OutputAudioPin::Release()
	{
		return CSourceStream::Release();
	}

	STDMETHODIMP OutputAudioPin::CheckMediaType(const CMediaType* pmt)
	{
		if (!pmt)
			return E_POINTER;

		if (*pmt->Type() != MEDIATYPE_Audio)
			return E_INVALIDARG;

		if (*pmt->Subtype() != MEDIASUBTYPE_PCM)
			return E_INVALIDARG;

		if (*pmt->FormatType() != FORMAT_WaveFormatEx)
			return E_INVALIDARG;

		WAVEFORMATEX* pwfex = reinterpret_cast<WAVEFORMATEX*>(pmt->Format());
		if (!pwfex)
			return E_INVALIDARG;

		if (pwfex->wFormatTag != WAVE_FORMAT_PCM && pwfex->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
			return E_INVALIDARG;

		if (pwfex->nChannels != reinterpret_cast<NTV2VCAM*>(m_pFilter)->GetNumAudioChannels())
			return E_INVALIDARG;

		if (pwfex->nSamplesPerSec != reinterpret_cast<NTV2VCAM*>(m_pFilter)->GetAudioSampleRate())
			return E_INVALIDARG;

		if (pwfex->wBitsPerSample != reinterpret_cast<NTV2VCAM*>(m_pFilter)->GetAudioBitsPerSample())
			return E_INVALIDARG;

		if (pwfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		{
			WAVEFORMATEXTENSIBLE* pwfxext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfex);
			if (pwfxext->SubFormat != KSDATAFORMAT_SUBTYPE_PCM)
				return E_INVALIDARG;
		}

		return S_OK;
	}

	HRESULT OutputAudioPin::GetMediaType(CMediaType* pMediaType)
	{
		if (reinterpret_cast<NTV2VCAM*>(m_pFilter)->Initialize())
			return CopyMediaType(pMediaType, &m_mt);

		return E_FAIL;
	}

	HRESULT OutputAudioPin::FillBuffer(IMediaSample* pSample)
	{
		return reinterpret_cast<NTV2VCAM*>(m_pFilter)->GetNextFrame(this, pSample);
	}

	HRESULT OutputAudioPin::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pRequest)
	{
		CAutoLock cAutoLock(m_pFilter->pStateLock());

		if (!pAlloc)
			return E_POINTER;

		WAVEFORMATEX* pwfx = reinterpret_cast<WAVEFORMATEX*>(m_mt.Format());
		if (!pwfx)
			return E_UNEXPECTED;

		pRequest->cBuffers = 1;
		pRequest->cbBuffer = gAudMaxSizeBytes * reinterpret_cast<NTV2VCAM*>(m_pFilter)->GetNumAudioLinks();

		ALLOCATOR_PROPERTIES Actual;
		HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
		if (FAILED(hr))
			return hr;

		if (Actual.cbBuffer < pRequest->cbBuffer)
			return E_FAIL;

		if (Actual.cBuffers != 1)
			return E_FAIL;

		return S_OK;
	}

	STDMETHODIMP OutputAudioPin::EnumMediaTypes(IEnumMediaTypes** ppEnum)
	{
		return CSourceStream::EnumMediaTypes(ppEnum);
	}

	STDMETHODIMP OutputAudioPin::Notify(IBaseFilter* pSender, Quality q)
	{
		return S_OK;
	}

	STDMETHODIMP OutputAudioPin::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP OutputAudioPin::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD* pcbReturned)
	{
		if (guidPropSet != AMPROPSETID_Pin)
			return E_PROP_SET_UNSUPPORTED;

		if (dwPropID != AMPROPERTY_PIN_CATEGORY)
			return E_PROP_ID_UNSUPPORTED;

		if (pPropData == NULL && pcbReturned == NULL)
			return E_POINTER;

		if (pcbReturned)
			*pcbReturned = sizeof(GUID);

		if (pPropData == NULL)
			return S_OK;

		if (cbPropData < sizeof(GUID))
			return E_UNEXPECTED;

		*(GUID*)pPropData = PIN_CATEGORY_CAPTURE;

		return S_OK;
	}

	STDMETHODIMP OutputAudioPin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport)
	{
		if (guidPropSet != AMPROPSETID_Pin)
			return E_PROP_SET_UNSUPPORTED;

		if (dwPropID != AMPROPERTY_PIN_CATEGORY)
			return E_PROP_ID_UNSUPPORTED;

		if (pTypeSupport)
			*pTypeSupport = KSPROPERTY_SUPPORT_GET;

		return S_OK;
	}

	STDMETHODIMP OutputAudioPin::SetFormat(AM_MEDIA_TYPE* pmt)
	{
		if (!pmt)
			return E_POINTER;

		if (pmt->majortype == m_mt.majortype &&
			pmt->subtype == m_mt.subtype &&
			pmt->formattype == m_mt.formattype)
		{
			m_mt = *pmt;
			return S_OK;
		}

		return VFW_E_INVALIDMEDIATYPE;
	}

	STDMETHODIMP OutputAudioPin::GetFormat(AM_MEDIA_TYPE** ppmt)
	{
		if (!ppmt)
			return E_POINTER;

		*ppmt = CreateMediaType(&m_mt);
		if (!*ppmt)
			return E_OUTOFMEMORY;

		return S_OK;
	}

	STDMETHODIMP OutputAudioPin::GetNumberOfCapabilities(int* piCount, int* piSize)
	{
		if (!piCount || !piSize)
			return E_POINTER;

		*piCount = 1;
		*piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);

		return S_OK;
	}

	STDMETHODIMP OutputAudioPin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** ppmt, BYTE* pSCC)
	{
		if (!ppmt || !pSCC)
			return E_POINTER;

		if (iIndex < 0 || iIndex >= 1)
			return E_INVALIDARG;

		CMediaType mediaType;
		if (FAILED(GetMediaType(&mediaType)))
			return E_FAIL;
		*ppmt = CreateMediaType(&mediaType);
		if (!*ppmt)
			return E_OUTOFMEMORY;

		WAVEFORMATEX* wfex = reinterpret_cast<decltype(wfex)>((*ppmt)->pbFormat);
		AUDIO_STREAM_CONFIG_CAPS* caps = reinterpret_cast<AUDIO_STREAM_CONFIG_CAPS*>(pSCC);
		ZeroMemory(caps, sizeof(AUDIO_STREAM_CONFIG_CAPS));
		caps->guid = FORMAT_WaveFormatEx;
		caps->MinimumChannels = wfex->nChannels;
		caps->MaximumChannels = wfex->nChannels;
		caps->MinimumBitsPerSample = wfex->wBitsPerSample;
		caps->MaximumBitsPerSample = wfex->wBitsPerSample;
		caps->MinimumSampleFrequency = wfex->nSamplesPerSec;
		caps->MaximumSampleFrequency = wfex->nSamplesPerSec;

		return S_OK;
	}

	CUnknown* WINAPI NTV2VCAM::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr)
	{
		NTV2VCAM* pFilter = new NTV2VCAM(pUnk, phr);
		if (pFilter == NULL)
			*phr = E_OUTOFMEMORY;
		return pFilter;
	}

	STDMETHODIMP NTV2VCAM::QueryInterface(REFIID riid, void** ppv)
	{
		return CSource::QueryInterface(riid, ppv);
	}

	STDMETHODIMP_(ULONG) NTV2VCAM::AddRef()
	{
		return CSource::AddRef();
	}

	STDMETHODIMP_(ULONG) NTV2VCAM::Release()
	{
		return CSource::Release();
	}

	STDMETHODIMP NTV2VCAM::EnumPins(IEnumPins** ppEnum)
	{
		return CSource::EnumPins(ppEnum);
	}

	STDMETHODIMP NTV2VCAM::FindPin(LPCWSTR Id, IPin** ppPin)
	{
		if (Id == nullptr || ppPin == nullptr)
			return E_POINTER;

		if (lstrcmpW(Id, VCAM_VIDEO_PIN_NAME_W) == 0)
		{
			*ppPin = GetPin(0);
			(*ppPin)->AddRef();
			return S_OK;
		}

		*ppPin = nullptr;
		return VFW_E_NOT_FOUND;
	}

	STDMETHODIMP NTV2VCAM::QueryFilterInfo(FILTER_INFO* pInfo)
	{
		if (!pInfo)
			return E_POINTER;

		memcpy(pInfo->achName, VCAM_FILTER_NAME_W, sizeof(VCAM_FILTER_NAME_W));

		pInfo->pGraph = m_pGraph;
		if (m_pGraph)
			m_pGraph->AddRef();
		return NOERROR;
	}

	STDMETHODIMP NTV2VCAM::JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName)
	{
		m_pGraph = pGraph;
		if (m_pGraph)
			m_pGraph->AddRef();
		return NOERROR;
	}

	STDMETHODIMP NTV2VCAM::QueryVendorInfo(LPWSTR* pVendorInfo)
	{
		return S_OK;
	}

	NTV2VCAM::NTV2VCAM (LPUNKNOWN pUnk, HRESULT* phr)
		: CSource(NAME(VCAM_FILTER_NAME), pUnk, CLSID_VirtualWebcam)
		, mVideos(MAX_VIDEOS)
		, mAudios(MAX_AUDIOS)
	{
		CAutoLock cAutoLock(&m_cStateLock);
		m_paStreams = (CSourceStream**) new OutputPin * [2];
		if (m_paStreams == NULL)
		{
			if (phr)
				*phr = E_OUTOFMEMORY;
			return;
		}

		m_paStreams[0] = new OutputVideoPin(phr, this, VCAM_VIDEO_PIN_NAME_W);
		if (m_paStreams[0] == NULL)
		{
			if (phr)
				*phr = E_OUTOFMEMORY;
			return;
		}

		m_paStreams[1] = new OutputAudioPin(phr, this, VCAM_AUDIO_PIN_NAME_W);
		if (m_paStreams[1] == NULL)
		{
			if (phr)
				*phr = E_OUTOFMEMORY;

			return;
		}

		mAjaDevice = "2";
		mInputType = "hdmi";
		mInputChannel = NTV2_CHANNEL1;
		mPixelFormatStr = "uyvy";
	}
#endif	//	else AJA_WINDOWS

NTV2VCAM::~NTV2VCAM()
{
	cout << "## NOTE: NTV2VCAM terminated" << endl;

#if defined (AJALinux)
	if (mLbDisplay > 0)
	{
		close(mLbDisplay);
		#if !defined(AJA_MISSING_DEV_V4L2LOOPBACK)
		if (ioctl(mLbDevice, V4L2LOOPBACK_CTL_REMOVE, mLbDeviceNR) == -1)
		{
			cerr << "## ERROR (" << errno << "): failed to remove V4L2 device for output" << endl;
			mErrorCode = AJA_VW_V4L2DEVICEREMOVEFAILED;
			return;
		}
		#endif
	}
	#if !defined(AJA_MISSING_DEV_V4L2LOOPBACK)
	if (mLbDevice > 0)
		close(mLbDevice);
	#endif

	if (mPcmHandle)
	{
		snd_pcm_drain(mPcmHandle);
		snd_pcm_close(mPcmHandle);
	}
#endif	//	AJALinux

	mDevice.AutoCirculateStop(mInputChannel);

	if (!mDoMultiFormat)
	{
		mDevice.ReleaseStreamForApplication(gDemoAppSignature, int32_t(AJAProcess::GetPid()));
		mDevice.SetEveryFrameServices(mSavedTaskMode);
	}
}

#if defined (AJALinux)
bool NTV2VCAM::Initialize (int argc, const char** argv)
{
	{
		int showVersion(0), useHDMI(0), inputChannel(0);
		char * pAjaDevSpec = AJA_NULL;
		char * pPixFormat = AJA_NULL;
		char * pVideoDevice = AJA_NULL;
		char * pAudioDevice = AJA_NULL;
		const CNTV2DemoCommon::PoptOpts optionsTable[] =
		{
			{"version",		'v', POPT_ARG_NONE,		&showVersion,	0,	"NTV2 version",				AJA_NULL},
			{"device",		'd', POPT_ARG_STRING,	&pAjaDevSpec,	0,	"AJA device",				"index#, serial#, or model"},
			{"hdmi",		'h', POPT_ARG_NONE,		&useHDMI,		0,	"use HDMI input?",			AJA_NULL},
			{"channel",		'c', POPT_ARG_INT,		&inputChannel,	0,	"input channel",			"1-8"},
			{"pixelformat",	'p', POPT_ARG_STRING,	&pPixFormat,	0,	"pixel format",				"FourCC string or 'list'"},
			{"vdev",		'i', POPT_ARG_STRING,	&pVideoDevice,	0,	"video device",				"/dev/video1"},
			{"adev",		'u', POPT_ARG_STRING,	&pAudioDevice,	0,	"audio device",				"hw:Loopback,1,1"},
			{"audiolinks",	'a', POPT_ARG_INT,		&mNumAudioLinks,0,	"multilink audio systems",	"0 for silence or 1-4"},
			POPT_AUTOHELP
			POPT_TABLEEND
		};
		CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
		if (!popt)
		{
			cerr << "## ERROR (" << errno << "): " << popt.errorStr() << endl;
			mErrorCode = AJA_VW_INVALIDARGS;
			return false;
		}
		mAjaDevice = pAjaDevSpec ? pAjaDevSpec : "0";
		mInputType = useHDMI ? "hdmi" : "sdi";
		mPixelFormatStr = pPixFormat ? pPixFormat : "2vuy";
		mVideoDevice = pVideoDevice ? pVideoDevice : "";
		mAudioDevice = pAudioDevice ? pAudioDevice : "";
		if (showVersion)
		{
			cout << argv[0] << ", NTV2 SDK " << NTV2Version() << endl;
			return false;
		}
		mInputChannel = NTV2Channel(inputChannel ? inputChannel-1 : 0);
	}
#elif defined (AJA_WINDOWS)
bool NTV2VCAM::Initialize()
{
	if (mInitialized)
		return true;

#endif	//	AJA_WINDOWS or AJALinux
	if (mAjaDevice.empty())
	{
		cerr << "## ERROR (" << errno << "): Parameter 'AJA device' is required." << endl;
		mErrorCode = AJA_VW_MISSINGARGS;
		return false;
	}
	if (mInputType.empty())
	{
		cerr << "## ERROR (" << errno << "): Parameter 'input type' is required." << endl;
		mErrorCode = AJA_VW_MISSINGARGS;
		return false;
	}
	if (mPixelFormatStr.empty())
	{
		cerr << "## ERROR (" << errno << "): Parameter 'pixel format' is required." << endl;
		mErrorCode = AJA_VW_MISSINGARGS;
		return false;
	}
	#if defined (AJALinux)
	if (mVideoDevice.empty())
	{
		cerr << "## ERROR (" << errno << "): Parameter 'video device' is required." << endl;
		mErrorCode = AJA_VW_MISSINGARGS;
		return false;
	}
	#endif	//	AJALinux
	if (mPixelFormatStr == "list")
	{
		cout << CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, mAjaDevice) << endl;
		return false;
	}
	mPixelFormat = mPixelFormatStr.empty() ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(mPixelFormatStr);
	if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT(mPixelFormat))
	{
		cerr << "## ERROR (" << errno << "): invalid pixel format" << endl;
		mErrorCode = AJA_VW_INVALIDPIXELFORMAT;
		return false;
	}

	if (!NTV2_IS_VALID_CHANNEL(mInputChannel))
	{
		cerr << "## ERROR (" << errno << "): invalid channel '" << DEC(mInputChannel+1) << "'" << endl;
		mErrorCode = AJA_VW_INVALIDINPUTCHANNEL;
		return false;
	}

	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(mAjaDevice, mDevice))
	{
		cerr << "## ERROR (" << errno << "): cannot find AJA device '" << mAjaDevice << "'" << endl;
		mErrorCode = AJA_VW_INVALIDAJADEVICE;
		return false;
	}

	if (!mDevice.IsDeviceReady())
	{
		cerr << "## ERROR (" << errno << "): AJA device not ready" << endl;
		mErrorCode = AJA_VW_AJADEVICENOTREADY;
		return false;
	}

	if (!mDevice.features().CanDoCapture())
	{
		cerr << "## ERROR (" << errno << "): AJA device does not support capture" << endl;
		mErrorCode = AJA_VW_AJADEVICENOCAPTURE;
		return false;
	}

	if (mInputType == "hdmi")
	{
		if (mDevice.features().GetNumHDMIVideoInputs() <= 0)
		{
			cerr << "## ERROR (" << errno << "): AJA device does not support HDMI" << endl;
			mErrorCode = AJA_VW_NOHDMISUPPORT;
			return false;
		}
		mIsKonaHDMI = true;
		mIOKinds = NTV2_IOKINDS_HDMI;
	}

	if (!mDevice.features().CanDoFrameBufferFormat(mPixelFormat))
	{
		cerr << "## ERROR (" << errno << "): AJA device does not support pixel format" << endl;
		mErrorCode = AJA_VW_AJADEVICENOPIXELFORMAT;
		return false;
	}

	ULWord appSignature(0);
	int32_t	appPID(0);
	if (!mDevice.GetStreamingApplication(appSignature, appPID))
	{
		cerr << "## ERROR: (" << errno << ") Unable to get device's current owner" << endl;
		mErrorCode = AJA_VW_GETDEVICEOWNERFAILED;
		return false;
	}
	if (!mDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication(gDemoAppSignature, int32_t(AJAProcess::GetPid())))
		{
			cerr << "## ERROR: (" << errno << ") Unable to acquire AJA device because another app (pid " << appPID << ") owns it" << endl;
			mErrorCode = AJA_STATUS_BUSY;
			return false;
		}
	}
	if (!mDevice.GetEveryFrameServices(mSavedTaskMode))
	{
		cerr << "## ERROR: (" << errno << ") Unable to get device's retail service task mode" << endl;
		mErrorCode = AJA_VW_GETFRAMESERVICESFAILED;
		return false;
	}
	else
	{
		if (!mDevice.SetEveryFrameServices(NTV2_OEM_TASKS))
		{
			cerr << "## ERROR: (" << errno << ") Unable to set device's retail service task mode to oem tasks" << endl;
			mErrorCode = AJA_VW_SETFRAMESERVICESFAILED;
			return false;
		}
	}

	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(mDoMultiFormat);

	mVideoFormat = mDevice.GetInputVideoFormat(NTV2ChannelToInputSource(mInputChannel, mIOKinds));
	if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat) || NTV2_IS_QUAD_QUAD_FORMAT(mVideoFormat))
		Get4KInputFormat(mVideoFormat);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr << "## ERROR (" << errno << "): invalid video format '" << mVideoFormat << "'" << endl;
		mErrorCode = AJA_VW_INVALIDVIDEOFORMAT;
		return false;
	}
	if (NTV2_IS_HD_VIDEO_FORMAT(mVideoFormat))
	{
		if (!NTV2_IS_VALID_CHANNEL(mInputChannel))
		{
			cerr << "## ERROR (" << errno << "): invalid channel '" << DEC(mInputChannel) << "'" << endl;
			mErrorCode = AJA_VW_INVALIDINPUTCHANNEL;
			return false;
		}
		mInputSource = NTV2ChannelToInputSource(mInputChannel, mIOKinds);
		if (mIsKonaHDMI && NTV2_INPUT_SOURCE_IS_SDI(mInputSource))
			mInputSource = NTV2ChannelToInputSource(NTV2InputSourceToChannel(mInputSource), mIOKinds);
		if (!mDevice.features().CanDoInputSource(mInputSource))
		{
			cerr << "## ERROR (" << errno << "): invalid input source" << endl;
			mErrorCode = AJA_VW_INVALIDINPUTSOURCE;
			return false;
		}

		mNumSpigots = 1;
		mActiveFrameStores = NTV2MakeChannelSet(mInputChannel, mNumSpigots);
		mActiveSDIs = NTV2MakeChannelSet(mInputChannel, mNumSpigots);
	}
	else if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat) || NTV2_IS_QUAD_QUAD_FORMAT(mVideoFormat))
	{
		const NTV2Channel origCh(mInputChannel);
		if (mIsKonaHDMI)
		{
			if (!mDoTSIRouting)
			{
				cerr << "## ERROR (" << errno << "):  AJA device requires TSI support" << endl;
				mErrorCode = AJA_VW_NOTSISUPPORT;
				return false;
			}
			if (mInputChannel != NTV2_CHANNEL1 && mInputChannel != NTV2_CHANNEL3)
				mInputChannel = NTV2_CHANNEL3;
			mInputSource = mInputChannel ? NTV2_INPUTSOURCE_HDMI2 : NTV2_INPUTSOURCE_HDMI1;
		}
		else if (mDevice.features().CanDo12gRouting())
		{
			mDoTSIRouting = false;
			if (UWord(origCh) >= mDevice.features().GetNumFrameStores())
			{
				cerr << "## ERROR (" << errno << "): invalid channel '" << DEC(mInputChannel) << "'" << endl;
				mErrorCode = AJA_VW_INVALIDINPUTCHANNEL;
				return false;
			}
			mInputSource = NTV2ChannelToInputSource(mInputChannel);
		}
		else if (mDoTSIRouting)
		{
			if (mInputChannel < NTV2_CHANNEL3)
				mInputChannel = NTV2_CHANNEL1;
			else if (mInputChannel < NTV2_CHANNEL5)
				mInputChannel = NTV2_CHANNEL3;
			else if (mInputChannel < NTV2_CHANNEL7)
				mInputChannel = NTV2_CHANNEL5;
			else
				mInputChannel = NTV2_CHANNEL7;
		}
		else
		{
			mInputChannel = mInputChannel < NTV2_CHANNEL5 ? NTV2_CHANNEL1 : NTV2_CHANNEL5;
		}

		if (!NTV2_IS_VALID_INPUT_SOURCE(mInputSource))
			mInputSource = NTV2ChannelToInputSource(mInputChannel);

		if (!NTV2_IS_VALID_CHANNEL(mInputChannel))
		{
			cerr << "## ERROR (" << errno << "): invalid channel '" << DEC(mInputChannel) << "'" << endl;
			mErrorCode = AJA_VW_INVALIDINPUTCHANNEL;
			return false;
		}
		if (!NTV2_IS_VALID_INPUT_SOURCE(mInputSource))
		{
			cerr << "## ERROR (" << errno << "): invalid input source '" << DEC(mInputSource) << "'" << endl;
			mErrorCode = AJA_VW_INVALIDINPUTSOURCE;
			return false;
		}

		if (mInputChannel != origCh)
			cerr << "## WARNING:  Specified channel Ch" << DEC(origCh + 1) << " corrected to use Ch" << DEC(mInputChannel + 1) << " to work for UHD/4K on '" << mDevice.GetDisplayName() << "'" << endl;

		mNumSpigots = mDevice.features().CanDo12gRouting() ? 1 : (mDoTSIRouting ? 2 : 4);
		mActiveFrameStores = NTV2MakeChannelSet(mInputChannel, mNumSpigots);
		mActiveSDIs = NTV2MakeChannelSet(mInputChannel, mNumSpigots);
	}

	if (!mDevice.EnableChannels(mActiveFrameStores, !mDoMultiFormat))
	{
		cerr << "## ERROR (" << errno << "): enable channels failed" << endl;
		mErrorCode = AJA_VW_ENABLECHANNELSFAILED;
		return false;
	}

	if (!mDevice.EnableInputInterrupt(mInputChannel))
	{
		cerr << "## ERROR (" << errno << "): enable input interrupt failed" << endl;
		mErrorCode = AJA_VW_ENABLEINPUTINTERRUPTFAILED;
		return false;
	}

	if (!mDevice.SubscribeInputVerticalEvent(mInputChannel))
	{
		cerr << "## ERROR (" << errno << "): subscribe input vertical event failed" << endl;
		mErrorCode = AJA_VW_SUBSCRIBEINPUTVERTICALEVENTFAILED;
		return false;
	}

	if (!mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL1))
	{
		cerr << "## ERROR (" << errno << "): subscribe output vertical event failed" << endl;
		mErrorCode = AJA_VW_SUBSCRIBEOUTPUTVERTICALEVENTFAILED;
		return false;
	}

	if (mDevice.features().HasBiDirectionalSDI() && NTV2_INPUT_SOURCE_IS_SDI(mInputSource))
	{
		if (!mDevice.SetSDITransmitEnable(mActiveSDIs, false))
		{
			cerr << "## ERROR (" << errno << "): set SDI transmit enabled failed" << endl;
			mErrorCode = AJA_VW_SETSDITRANSITENABLEFAILED;
			return false;
		}
		if (!mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1, 10))
		{
			cerr << "## ERROR (" << errno << "): wait for output vertical interrupt failed" << endl;
			mErrorCode = AJA_VW_WAITFOROUTPUTVERTICALINTERRUPTFAILED;
			return false;
		}
	}

	mFormatDesc = NTV2FormatDescriptor(mVideoFormat, mPixelFormat);

	if (!mDevice.SetMode(mInputChannel, NTV2_MODE_CAPTURE))
	{
		cerr << "## ERROR (" << errno << "): failed to set capture mode" << endl;
		mErrorCode = AJA_VW_SETCAPTUREMODEFAILED;
		return false;
	}

	if (!mDoMultiFormat)
		if (!mDevice.SetReference(NTV2_REFERENCE_FREERUN))
		{
			cerr << "## ERROR (" << errno << "): set freerun reference failed" << endl;
			mErrorCode = AJA_VW_SETFREERUNREFERENCEFAILED;
			return false;
		}

	if (!mDevice.SetVideoFormat(mVideoFormat, false, false, mInputChannel))
	{
		cerr << "## ERROR (" << errno << "): failed to set video format on AJA device" << endl;
		mErrorCode = AJA_VW_SETVIDEOFORMATFAILED;
		return false;
	}

	if (!mDevice.SetVANCMode(mActiveFrameStores, NTV2_VANCMODE_OFF))
	{
		cerr << "## ERROR (" << errno << "): set VANC mode off failed" << endl;
		mErrorCode = AJA_VW_SETVANCMODEOFFFAILED;
		return false;
	}

	if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat) || NTV2_IS_QUAD_QUAD_FORMAT(mVideoFormat))
	{
		if (mDevice.features().CanDo12gRouting() || mDoTSIRouting)
		{
			if (NTV2_IS_QUAD_FRAME_FORMAT(mVideoFormat) || NTV2_IS_QUAD_QUAD_FORMAT(mVideoFormat))
				if (!mDevice.SetTsiFrameEnable(true, mInputChannel))
				{
					cerr << "## ERROR (" << errno << "): set TSI frame enable failed" << endl;
					mErrorCode = AJA_VW_SETTSIFRAMEENABLEFAILED;
					return false;
				}
		}
		else
		{
			if (!mDevice.Set4kSquaresEnable(true, mInputChannel))
			{
				cerr << "## ERROR (" << errno << "): set 4k squares enable failed" << endl;
				mErrorCode = AJA_VW_SET4KSQUARESENABLEFAILED;
				return false;
			}
		}
	}

	if (!mDevice.SetFrameBufferFormat(mActiveFrameStores, mPixelFormat))
	{
		cerr << "## ERROR (" << errno << "): set frame buffer format failed" << endl;
		mErrorCode = AJA_VW_SETFRAMEBUFFERFORMATFAILED;
		return false;
	}

	mVideoBuffer.Allocate(mFormatDesc.GetTotalRasterBytes());
	mDevice.DMABufferLock(mVideoBuffer, true);

	SetupAudio();

	NTV2LHIHDMIColorSpace inputColorSpace(NTV2_LHIHDMIColorSpaceYCbCr);
	if (NTV2_INPUT_SOURCE_IS_HDMI(mInputSource))
		mDevice.GetHDMIInputColor(inputColorSpace, mInputChannel);
	NTV2XptConnections connections;
	if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat) || NTV2_IS_QUAD_QUAD_FORMAT(mVideoFormat))
	{
		if (!GetInputRouting4K(connections, inputColorSpace == NTV2_LHIHDMIColorSpaceRGB))
		{
			cerr << "## ERROR (" << errno << "):  get input routing failed" << endl;
			mErrorCode = AJA_VW_GETINPUTROUTEFAILED;
			return false;
		}
	}
	else
	{
		if (!GetInputRouting(connections, inputColorSpace == NTV2_LHIHDMIColorSpaceRGB))
		{
			cerr << "## ERROR (" << errno << "):  get input routing failed" << endl;
			mErrorCode = AJA_VW_GETINPUTROUTEFAILED;
			return false;
		}
	}
	if (!mDevice.ApplySignalRoute(connections, !mDoMultiFormat))
	{
		cerr << "## ERROR (" << errno << "):  apply signal route failed" << endl;
		mErrorCode = AJA_VW_APPLYSIGNALROUTEFAILED;
		return false;
	}

	#if defined (AJALinux)
		#if !defined(AJA_MISSING_DEV_V4L2LOOPBACK)
		mLbDevice = open(V4L2_DRIVER_NAME, O_RDONLY);
		if (mLbDevice == -1)
		{
			cerr << "## ERROR (" << errno << "): failed to open V4L2 driver" << endl;
			mErrorCode = AJA_VW_V4L2DRIVEROPENFAILED;
			return false;
		}
		v4l2_loopback_config cfg;
		memset(&cfg, 0, sizeof(v4l2_loopback_config));
		cfg.output_nr = mLbDeviceNR = ExtractNumber(mVideoDevice.c_str());
		string labelName = "AJA virtual webcam device " + to_string(mLbDeviceNR);
		strcpy(cfg.card_label, labelName.c_str());
		mLbDeviceNR = ioctl(mLbDevice, V4L2LOOPBACK_CTL_ADD, &cfg);
		if (mLbDeviceNR == -1)
		{
			cerr << "## ERROR (" << errno << "): failed to create V4L2 device for output" << endl;
			mErrorCode = AJA_VW_V4L2DEVICECREATEFAILED;
			return false;
		}
		#endif	//	!defined(AJA_MISSING_DEV_V4L2LOOPBACK)
	mLbDisplay = open(mVideoDevice.c_str(), O_RDWR);
	if (mLbDisplay == -1)
	{
		cerr << "## ERROR (" << errno << "): failed to open V4L2 output device" << endl;
		mErrorCode = AJA_VW_V4L2DEVICEOPENFAILED;
		return false;
	}

	struct v4l2_format lbFormat;
	memset(&lbFormat, 0, sizeof(lbFormat));
	lbFormat.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	lbFormat.fmt.pix.width = mFormatDesc.GetRasterWidth();
	lbFormat.fmt.pix.height = mFormatDesc.GetRasterHeight();
	lbFormat.fmt.pix.field = V4L2_FIELD_NONE;
	lbFormat.fmt.pix.colorspace = V4L2_COLORSPACE_REC709;
	lbFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	if (ioctl(mLbDisplay, VIDIOC_S_FMT, &lbFormat) == -1)
		cerr << "## ERROR (" << errno << "): cannot set video format on video loopback device" << endl;

	struct v4l2_streamparm streamParm;
	memset(&streamParm, 0, sizeof(streamParm));
	streamParm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	streamParm.parm.output.timeperframe.numerator = 1;
	streamParm.parm.output.timeperframe.denominator = GetFps();
	if (ioctl(mLbDisplay, VIDIOC_S_PARM, &streamParm) == -1)
		cerr << "## ERROR (" << errno << "): cannot set frame rate on video loopback device" << endl;
	#endif	//	AJALinux

	bool retVal = false;
	if (NTV2_IS_8K_VIDEO_FORMAT(mVideoFormat))
		retVal = mFrames.setExactRange(0, 6);
	else if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat))
	{
		const UWord startFrame12g[] = { 0, 7, 64, 71 };
		const UWord startFrame[] = { 0, 7, 14, 21 };
		if (mDevice.features().CanDo12gRouting())
			retVal = mFrames.setRangeWithCount(7, startFrame12g[mInputChannel]);
		else
			retVal = mFrames.setRangeWithCount(7, startFrame[mInputChannel / 2]);
	}
	else
		retVal = mFrames.setCountOnly(7);
	if (!retVal)
	{
		cerr << "## ERROR (" << errno << "): failed to set AC frame range" << endl;
		mErrorCode = AJA_VW_SETACFRAMERANGEFAILED;
		return false;
	}

	#if defined(AJA_WINDOWS)
	if (OutputVideoPin* pVideoPin = dynamic_cast<OutputVideoPin*>(m_paStreams[0]))
	{
		VIDEOINFO vi;
		ZeroMemory(&vi, sizeof(VIDEOINFO));
		vi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		vi.bmiHeader.biWidth = mFormatDesc.GetRasterWidth();
		vi.bmiHeader.biHeight = mFormatDesc.GetRasterHeight();
		vi.bmiHeader.biPlanes = mFormatDesc.GetNumPlanes();
		vi.bmiHeader.biBitCount = mFormatDesc.GetNumBitsLuma() + mFormatDesc.GetNumBitsChroma() + mFormatDesc.GetNumBitsAlpha();
		vi.bmiHeader.biCompression = 0x59565955; //UYVY
		vi.bmiHeader.biSizeImage = mFormatDesc.GetTotalRasterBytes();
		vi.AvgTimePerFrame = GetFps();

		pVideoPin->m_mt.SetType(&MEDIATYPE_Video);
		pVideoPin->m_mt.SetSubtype(&MEDIASUBTYPE_UYVY);
		pVideoPin->m_mt.SetFormatType(&FORMAT_VideoInfo);
		pVideoPin->m_mt.SetSampleSize(vi.bmiHeader.biSizeImage);
		pVideoPin->m_mt.SetTemporalCompression(FALSE);
		pVideoPin->m_mt.SetFormat(reinterpret_cast<BYTE*>(&vi), sizeof(VIDEOINFO));
	}

	if (OutputAudioPin* pAudioPin = dynamic_cast<OutputAudioPin*>(m_paStreams[1]))
	{
		WAVEFORMATEXTENSIBLE wfx;
		ZeroMemory(&wfx, sizeof(WAVEFORMATEXTENSIBLE));
		wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfx.Format.nChannels = mNumAudioChannels;
		wfx.Format.nSamplesPerSec = mSampleRate;
		wfx.Format.wBitsPerSample = mBitsPerSample;
		wfx.Format.nBlockAlign = (wfx.Format.nChannels * wfx.Format.wBitsPerSample) / 8;
		wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
		wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

		wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;
		if (mNumAudioChannels == 2)
		{
			wfx.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
		}
		else if (mNumAudioChannels == 16)
		{
			wfx.dwChannelMask =
				SPEAKER_FRONT_LEFT |
				SPEAKER_FRONT_RIGHT |
				SPEAKER_FRONT_CENTER |
				SPEAKER_LOW_FREQUENCY |
				SPEAKER_BACK_LEFT |
				SPEAKER_BACK_RIGHT |
				SPEAKER_FRONT_LEFT_OF_CENTER |
				SPEAKER_FRONT_RIGHT_OF_CENTER |
				SPEAKER_BACK_CENTER |
				SPEAKER_SIDE_LEFT |
				SPEAKER_SIDE_RIGHT |
				SPEAKER_TOP_CENTER |
				SPEAKER_TOP_FRONT_LEFT |
				SPEAKER_TOP_FRONT_CENTER |
				SPEAKER_TOP_FRONT_RIGHT |
				SPEAKER_TOP_BACK_CENTER;
		}
		wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

		pAudioPin->m_mt.ResetFormatBuffer();
		pAudioPin->m_mt.SetType(&MEDIATYPE_Audio);
		pAudioPin->m_mt.SetSubtype(&MEDIASUBTYPE_PCM);
		pAudioPin->m_mt.SetFormatType(&FORMAT_WaveFormatEx);
		pAudioPin->m_mt.SetTemporalCompression(FALSE);
		pAudioPin->m_mt.SetSampleSize(wfx.Format.nBlockAlign);

		BYTE* pFormat = (BYTE*)pAudioPin->m_mt.AllocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));
		memcpy(pFormat, &wfx, sizeof(WAVEFORMATEXTENSIBLE));
	}

	mInitialized = true;
	#endif	//	AJA_WINDOWS
	return true;
}	//	NTV2VCAM::Initialize

#if defined(AJALinux)
	bool NTV2VCAM::Run(void)
	{
		mDevice.AutoCirculateStop(mActiveFrameStores);
		if (!mDevice.AutoCirculateInitForInput(mInputChannel, mFrames.count(), mAudioSystem, mACOptions, 1, mFrames.firstFrame(), mFrames.lastFrame()))
			{cerr << "## ERROR: AC init for input failed" << endl;  mErrorCode = AJA_VW_ACINITFORINPUTFAILED;  return false;}
		if (!mDevice.AutoCirculateStart(mInputChannel))
			{cerr << "## ERROR: AC start failed" << endl;  mErrorCode = AJA_VW_ACSTARTFAILED;  return false;}
		if (!mAcTransfer.SetVideoBuffer(mVideoBuffer, mVideoBuffer.GetByteCount()))
			{cerr << "## ERROR: AC set video buffer failed" << endl;  mErrorCode = AJA_VW_ACSETVIDEOBUFFERFAILED;  return false;}
		if (mAudioBuffer && !mAcTransfer.SetAudioBuffer(mAudioBuffer, mAudioBuffer.GetByteCount()))
			{cerr << "## ERROR: AC set audio buffer failed" << endl;  mErrorCode = AJA_VW_ACSETAUDIOBUFFERFAILED;  return false;}

		signal(SIGINT, SignalHandler);

		while (!gGlobalQuit)
		{
			AUTOCIRCULATE_STATUS acStatus;
			if (!mDevice.AutoCirculateGetStatus(mInputChannel, acStatus))
				return false;
			if (acStatus.IsRunning() && acStatus.HasAvailableInputFrame())
			{
				if (!mDevice.AutoCirculateTransfer(mInputChannel, mAcTransfer))
					{cerr << "## ERROR: AC transfer failed" << endl;  mErrorCode = AJA_VW_ACTRANSFERFAILED;  return false;}

				if (write(mLbDisplay, mVideoBuffer, mVideoBuffer.GetByteCount()) == -1)
					cerr << "## ERROR (" << errno << "): write to video loopback device failed" << endl;

				if (mAudioBuffer)
				{
					mAudioFrames = mAcTransfer.GetCapturedAudioByteCount() / (AUDIO_BYTESPERSAMPLE * mNumAudioChannels);
					unsigned long pcmReturn = snd_pcm_writei(mPcmHandle, mAudioBuffer, mAudioFrames);
					if (pcmReturn < 0)
						pcmReturn = snd_pcm_recover(mPcmHandle, pcmReturn, 0);
					if (pcmReturn < 0)
						cerr << "## ERROR (" << errno << "): snd_pcm_writei failed" << endl;
					if (pcmReturn > 0 && pcmReturn < mAudioFrames)
						cerr << "## ERROR (" << errno << "): short write - expected '" << mAudioFrames << "' frames, wrote '" << pcmReturn << "' frames" << endl;
				}
			}
			else
				mDevice.WaitForInputVerticalInterrupt(mInputChannel);
		}	//	while !mGlobalQuit
		return true;
	}	//	NTV2VCAM::Run
#endif	//	AJALinux

#if defined(AJA_WINDOWS)
	HRESULT NTV2VCAM::GetNextFrame (OutputPin* pPin, IMediaSample* pSample)
	{
		ASSERT(mInitialized);
		if (!mRunning)
		{
			mDevice.AutoCirculateStop(mActiveFrameStores);
			if (!mDevice.AutoCirculateInitForInput(mInputChannel, mFrames.count(), mAudioSystem, mACOptions, 1, mFrames.firstFrame(), mFrames.lastFrame()))
				{cerr << "## ERROR: AC init for input failed" << endl;  mErrorCode = AJA_VW_ACINITFORINPUTFAILED;  return S_FALSE;}
			if (!mDevice.AutoCirculateStart(mInputChannel))
				{cerr << "## ERROR: AC start failed" << endl;  mErrorCode = AJA_VW_ACSTARTFAILED;  return S_FALSE;}
			if (!mAcTransfer.SetVideoBuffer(mVideoBuffer, mVideoBuffer.GetByteCount()))
				{cerr << "## ERROR: AC set video buffer failed" << endl;  mErrorCode = AJA_VW_ACSETVIDEOBUFFERFAILED;  return S_FALSE;}
			if (mAudioBuffer && !mAcTransfer.SetAudioBuffer(mAudioBuffer, mAudioBuffer.GetByteCount()))
				{cerr << "## ERROR: AC set audio buffer failed" << endl;  mErrorCode = AJA_VW_ACSETAUDIOBUFFERFAILED;  return S_FALSE;}
		}

		AUTOCIRCULATE_STATUS acStatus;
		if (!mDevice.AutoCirculateGetStatus(mInputChannel, acStatus))
			return S_FALSE;

		if (acStatus.IsRunning() && acStatus.HasAvailableInputFrame())
		{
			if (!mDevice.AutoCirculateTransfer(mInputChannel, mAcTransfer))
				{cerr << "## ERROR: AC transfer failed" << endl;  mErrorCode = AJA_VW_ACTRANSFERFAILED;  return S_FALSE;}
	
			mVideos.push(mVideoBuffer);
			mAudioBuffer ? mAudios.push(mAudioBuffer) : 0;
		}
		else
			mDevice.WaitForInputVerticalInterrupt(mInputChannel);

		BYTE* pData;
		HRESULT hr = pSample->GetPointer(&pData);
		if (FAILED(hr))
		{
			cerr << "## ERROR (" << errno << "): failed to get pointer to sample buffer" << endl;
			mErrorCode = AJA_VW_GETPOINTERFAILED;
			return S_FALSE;
		}
		ULWord cbData = pSample->GetSize();
		if (OutputVideoPin* pVideoPin = dynamic_cast<OutputVideoPin*>(pPin))
		{
			NTV2Buffer tmpVideoBuffer;
			if (mVideos.pop(tmpVideoBuffer))
			{
				if (tmpVideoBuffer.GetByteCount() != cbData)
				{
					cerr << "## ERROR: sample buffer size mismatch: AJA is " << tmpVideoBuffer.GetByteCount() << ", output is " << cbData << endl;
					mErrorCode = AJA_VW_SAMPLEBUFFERSIZEMISMATCH;
					return S_FALSE;
				}
				memcpy(pData, tmpVideoBuffer, cbData);
				pSample->SetActualDataLength(tmpVideoBuffer.GetByteCount());
			}
		}
		if (OutputAudioPin* pAudioPin = dynamic_cast<OutputAudioPin*>(pPin))
		{
			NTV2Buffer tmpAudioBuffer;
			if (mAudios.pop(tmpAudioBuffer))
			{
				if (mAcTransfer.GetCapturedAudioByteCount() > cbData)
				{
					cerr << "## ERROR: sample buffer size mismatch: AJA is " << tmpAudioBuffer.GetByteCount() << ", output is " << cbData << endl;
					mErrorCode = AJA_VW_SAMPLEBUFFERSIZEMISMATCH;
					return S_FALSE;
				}
				memcpy(pData, tmpAudioBuffer, mAcTransfer.GetCapturedAudioByteCount());
				pSample->SetActualDataLength(mAcTransfer.GetCapturedAudioByteCount());
			}
		}
		pSample->SetSyncPoint(TRUE);
		mRunning = true;
		return S_OK;
	}	//	GetNextFrame
#endif	//	defined(AJA_WINDOWS)

string NTV2VCAM::ULWordToString(const ULWord inNum)
{
	ostringstream oss;
	oss << inNum;
	return oss.str();
}

bool NTV2VCAM::Get4KInputFormat(NTV2VideoFormat& inOutVideoFormat)
{
	static struct VideoFormatPair
	{
		NTV2VideoFormat	vIn;
		NTV2VideoFormat	vOut;
	} VideoFormatPairs[] = {	//			vIn								vOut
								{NTV2_FORMAT_1080psf_2398,		NTV2_FORMAT_4x1920x1080psf_2398},
								{NTV2_FORMAT_1080psf_2400,		NTV2_FORMAT_4x1920x1080psf_2400},
								{NTV2_FORMAT_1080p_2398,		NTV2_FORMAT_4x1920x1080p_2398},
								{NTV2_FORMAT_1080p_2400,		NTV2_FORMAT_4x1920x1080p_2400},
								{NTV2_FORMAT_1080p_2500,		NTV2_FORMAT_4x1920x1080p_2500},
								{NTV2_FORMAT_1080p_2997,		NTV2_FORMAT_4x1920x1080p_2997},
								{NTV2_FORMAT_1080p_3000,		NTV2_FORMAT_4x1920x1080p_3000},
								{NTV2_FORMAT_1080p_5000_B,		NTV2_FORMAT_4x1920x1080p_5000},
								{NTV2_FORMAT_1080p_5994_B,		NTV2_FORMAT_4x1920x1080p_5994},
								{NTV2_FORMAT_1080p_6000_B,		NTV2_FORMAT_4x1920x1080p_6000},
								{NTV2_FORMAT_1080p_2K_2398,		NTV2_FORMAT_4x2048x1080p_2398},
								{NTV2_FORMAT_1080p_2K_2400,		NTV2_FORMAT_4x2048x1080p_2400},
								{NTV2_FORMAT_1080p_2K_2500,		NTV2_FORMAT_4x2048x1080p_2500},
								{NTV2_FORMAT_1080p_2K_2997,		NTV2_FORMAT_4x2048x1080p_2997},
								{NTV2_FORMAT_1080p_2K_3000,		NTV2_FORMAT_4x2048x1080p_3000},
								{NTV2_FORMAT_1080p_2K_5000_A,	NTV2_FORMAT_4x2048x1080p_5000},
								{NTV2_FORMAT_1080p_2K_5994_A,	NTV2_FORMAT_4x2048x1080p_5994},
								{NTV2_FORMAT_1080p_2K_6000_A,	NTV2_FORMAT_4x2048x1080p_6000},

								{NTV2_FORMAT_1080p_5000_A,		NTV2_FORMAT_4x1920x1080p_5000},
								{NTV2_FORMAT_1080p_5994_A,		NTV2_FORMAT_4x1920x1080p_5994},
								{NTV2_FORMAT_1080p_6000_A,		NTV2_FORMAT_4x1920x1080p_6000},

								{NTV2_FORMAT_1080p_2K_5000_A,	NTV2_FORMAT_4x2048x1080p_5000},
								{NTV2_FORMAT_1080p_2K_5994_A,	NTV2_FORMAT_4x2048x1080p_5994},
								{NTV2_FORMAT_1080p_2K_6000_A,	NTV2_FORMAT_4x2048x1080p_6000}
	};
	for (size_t formatNdx(0); formatNdx < sizeof(VideoFormatPairs) / sizeof(VideoFormatPair); formatNdx++)
		if (VideoFormatPairs[formatNdx].vIn == inOutVideoFormat)
		{
			inOutVideoFormat = VideoFormatPairs[formatNdx].vOut;
			return true;
		}
	return false;
}

void NTV2VCAM::SetupAudio()
{
	#if defined(AJALinux)
	if (mAudioDevice.empty())
		return;
	#endif	//	AJALinux

	mAudioSystem = NTV2InputSourceToAudioSystem(mInputSource);
	if (mIsKonaHDMI)
		mAudioSystem = NTV2_AUDIOSYSTEM_2;

	if (!mDevice.features().CanDoMultiLinkAudio())
		mNumAudioLinks = 1;

	int fNumAudioSystems = mDevice.features().GetNumAudioSystems();
	if (mDoMultiFormat && fNumAudioSystems > 1 && UWord(mInputChannel) < fNumAudioSystems)
		mAudioSystem = NTV2ChannelToAudioSystem(mInputChannel);
	NTV2AudioSystemSet multiLinkAudioSystems(NTV2MakeAudioSystemSet(mAudioSystem, 1));
	if (mNumAudioLinks > 1)
	{
		multiLinkAudioSystems = NTV2MakeAudioSystemSet(NTV2_AUDIOSYSTEM_1, NTV2_IS_4K_HFR_VIDEO_FORMAT(mVideoFormat) ? 4 : 2);
		if (mInputChannel != NTV2_CHANNEL1)
		{
			cerr << "## ERROR (" << errno << "):  multi-Link audio only intended for input Ch1, not Ch" << DEC(mInputChannel) << endl;
			mErrorCode = AJA_VW_INCORRECTMULTILINKAUDIOCHANNEL;
			return;
		}
		mACOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO1;
		if (NTV2_IS_4K_HFR_VIDEO_FORMAT(mVideoFormat))
		{
			mACOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO2;
			mACOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO3;
		}
	}

	UWord failures = 0;
	mNumAudioChannels = mDevice.features().GetMaxAudioChannels();
	for (NTV2AudioSystemSetConstIter it = multiLinkAudioSystems.begin(); it != multiLinkAudioSystems.end(); ++it)
	{
		const NTV2AudioSystem audSys(*it);
		if (!mDevice.SetAudioSystemInputSource(audSys, NTV2InputSourceToAudioSource(mInputSource), NTV2InputSourceToEmbeddedAudioInput(mInputSource)))
			failures++;
		if (!mDevice.SetNumberAudioChannels(mNumAudioChannels, audSys))
			failures++;
		if (!mDevice.SetAudioRate(NTV2_AUDIO_48K, audSys))
			failures++;
		if (!mDevice.SetAudioBufferSize(NTV2_AUDIO_BUFFER_SIZE_4MB, audSys))
			failures++;
		if (!mDevice.SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, audSys))
			failures++;
	}
	if (failures > 0)
	{
		cerr << "## ERROR (" << errno << "): configure audio system failed" << endl;
		mErrorCode = AJA_VW_CONFIGAUDIOSYSTEMFAILED;
		return;
	}
	#if defined (AJALinux)
	int pcmReturn = snd_pcm_open(&mPcmHandle, mAudioDevice.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
	if (pcmReturn < 0)
	{
		cerr << "## ERROR (" << errno << "): snd_pcm_open failed" << endl;
		return;
	}
	pcmReturn = snd_pcm_set_params(mPcmHandle, SND_PCM_FORMAT_S32_LE, SND_PCM_ACCESS_RW_INTERLEAVED, mNumAudioChannels, mSampleRate, 1, 500000);
	if (pcmReturn < 0)
	{
		cerr << "## ERROR (" << errno << "): snd_pcm_set_params failed" << endl;
		return;
	}
	#endif	//	AJALinux
	if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
	{
		mAudioBuffer.Allocate(gAudMaxSizeBytes * mNumAudioLinks);
		mDevice.DMABufferLock(mAudioBuffer, true);
	}
}	//	SetupAudio

bool NTV2VCAM::GetInputRouting(NTV2XptConnections& conns, const bool isInputRGB)
{
	const bool				isFrameRGB(::IsRGBFormat(mPixelFormat));
	const NTV2InputXptID	fbIXpt(::GetFrameBufferInputXptFromChannel(mInputChannel));
	const NTV2OutputXptID	inputOXpt(::GetInputSourceOutputXpt(mInputSource, false, isInputRGB));
	const NTV2InputXptID	cscVidIXpt(::GetCSCInputXptFromChannel(mInputChannel));
	NTV2OutputXptID			cscOXpt(::GetCSCOutputXptFromChannel(mInputChannel, /*key?*/false, /*RGB?*/isFrameRGB));

	conns.clear();
	if (isInputRGB && !isFrameRGB)
	{
		conns.insert(NTV2Connection(fbIXpt, cscOXpt));		//	FB <== CSC
		conns.insert(NTV2Connection(cscVidIXpt, inputOXpt));	//	CSC <== SDIIn/HDMIin
	}
	else if (!isInputRGB && isFrameRGB)
	{
		conns.insert(NTV2Connection(fbIXpt, cscOXpt));		//	FB <== CSC
		conns.insert(NTV2Connection(cscVidIXpt, inputOXpt));	//	CSC <== SDIIn/HDMIIn
	}
	else
		conns.insert(NTV2Connection(fbIXpt, inputOXpt));	//	FB <== SDIIn/HDMIin

	return !conns.empty();
}

bool NTV2VCAM::GetInputRouting4K(NTV2XptConnections& conns, const bool isInputRGB)
{
	UWord sdi(0), mux(0), csc(0), fb(0), path(0);
	NTV2InputXptID in(NTV2_INPUT_CROSSPOINT_INVALID);
	NTV2OutputXptID out(NTV2_OUTPUT_CROSSPOINT_INVALID);
	const bool	isFrameRGB(::IsRGBFormat(mPixelFormat));
	conns.clear();
	if (NTV2_INPUT_SOURCE_IS_HDMI(mInputSource))
	{	//	HDMI
		if (mDevice.features().CanDo12gRouting())
		{	//	FB <== SDIIn
			in = ::GetFrameBufferInputXptFromChannel(mInputChannel);
			out = ::GetInputSourceOutputXpt(mInputSource);
			conns.insert(NTV2Connection(in, out));
		}
		else
		{
			if (mInputChannel == NTV2_CHANNEL1)
			{	//	HDMI CH1234
				if (isInputRGB == isFrameRGB)
				{	//	HDMI CH1234 RGB SIGNAL AND RGB FBF  OR  YUV SIGNAL AND YUV FBF
					for (path = 0; path < 4; path++)
					{	//	MUX <== HDMIIn
						in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1);
						out = ::GetInputSourceOutputXpt(mInputSource, /*DS2*/false, isInputRGB, /*quadrant*/path);
						conns.insert(NTV2Connection(in, out));
						//	FB <== MUX
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb + path / 2), /*Binput*/path & 1);
						out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1, /*RGB*/isInputRGB);
						conns.insert(NTV2Connection(in, out));
					}
				}	//	HDMI CH1234 RGB SIGNAL AND RGB FBF
				else if (isInputRGB && !isFrameRGB)
				{	//	HDMI CH1234 RGB SIGNAL AND YUV FBF
					for (path = 0; path < 4; path++)
					{
						//	CSC <== HDMIIn
						in = ::GetCSCInputXptFromChannel(NTV2Channel(csc + path));
						out = ::GetInputSourceOutputXpt(mInputSource, /*DS2*/false, isInputRGB, /*quadrant*/path);
						conns.insert(NTV2Connection(in, out));
						//	MUX <== CSC
						in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1);
						out = ::GetCSCOutputXptFromChannel(NTV2Channel(csc + path), /*key*/false, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
						//	FB <== MUX
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb + path / 2), /*DS2*/path & 1);
						out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
					}
				}	//	HDMI CH1234 RGB SIGNAL AND YUV FBF
				else	//	!isInputRGB && isFrameRGB
				{	//	HDMI CH1234 YUV SIGNAL AND RGB FBF
					for (path = 0; path < 4; path++)
					{
						//	CSC <== HDMIIn
						in = ::GetCSCInputXptFromChannel(NTV2Channel(csc + path));
						out = ::GetInputSourceOutputXpt(mInputSource, /*DS2*/false, isInputRGB, /*quadrant*/path);
						conns.insert(NTV2Connection(in, out));
						//	MUX <== CSC
						in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1);
						out = ::GetCSCOutputXptFromChannel(NTV2Channel(csc + path), /*key*/false, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
						//	FB <== MUX
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb + path / 2), /*DS2*/path & 1);
						out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
					}
				}	//	HDMI CH1234 YUV SIGNAL AND RGB FBF
			}	//	HDMI CH1234
			else
			{	//	HDMI CH5678
				cerr << "## ERROR (" << errno << "): Ch5678 must be for Corvid88, but no HDMI on that device" << endl;
			}	//	HDMI CH5678
		}
	}	//	HDMI
	else
	{	//	SDI
		if (mDevice.features().CanDo12gRouting())
		{	//	FB <== SDIIn
			in = ::GetFrameBufferInputXptFromChannel(mInputChannel);
			out = ::GetInputSourceOutputXpt(mInputSource);
			conns.insert(NTV2Connection(in, out));
		}
		else
		{	//	SDI CH1234 or CH5678
			if (mInputChannel != NTV2_CHANNEL1)
			{
				fb = 4;  sdi = fb;  mux = fb / 2;  csc = fb;
			}
			if (isFrameRGB)
			{	//	RGB FB
				if (mDoTSIRouting)
				{	//	SDI CH1234 RGB TSI
					for (path = 0; path < 4; path++)
					{
						//	CSC <== SDIIn
						in = ::GetCSCInputXptFromChannel(NTV2Channel(csc + path));
						out = ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(NTV2Channel(sdi + path)));
						conns.insert(NTV2Connection(in, out));
						//	MUX <== CSC
						in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1);
						out = ::GetCSCOutputXptFromChannel(NTV2Channel(csc + path), /*key*/false, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
						//	FB <== MUX
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb + path / 2), /*DS2*/path & 1);
						out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
					}	//	for each spigot
				}	//	SDI CH1234 RGB TSI
				else
				{	//	SDI CH1234 RGB SQUARES
					for (path = 0; path < 4; path++)
					{
						//	CSC <== SDIIn
						in = ::GetCSCInputXptFromChannel(NTV2Channel(csc + path));
						out = ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(NTV2Channel(sdi + path)));
						conns.insert(NTV2Connection(in, out));
						//	FB <== CSC
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb + path));
						out = ::GetCSCOutputXptFromChannel(NTV2Channel(csc + path), /*key*/false, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
					}	//	for each spigot
				}	//	SDI CH1234 RGB SQUARES
			}	//	SDI CH1234 RGB FBF
			else	//	YUV FBF
			{
				if (mDoTSIRouting)
				{	//	SDI CH1234 YUV TSI
					for (path = 0; path < 4; path++)
					{
						//	MUX <== SDIIn
						in = ::GetTSIMuxInputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1);
						out = ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(NTV2Channel(sdi + path)));
						conns.insert(NTV2Connection(in, out));
						//	FB <== MUX
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb + path / 2), /*DS2*/path & 1);
						out = ::GetTSIMuxOutputXptFromChannel(NTV2Channel(mux + path / 2), /*LinkB*/path & 1, /*rgb*/isFrameRGB);
						conns.insert(NTV2Connection(in, out));
					}	//	for each spigot
				}	//	SDI CH1234 YUV TSI
				else
				{
					for (path = 0; path < 4; path++)
					{	//	FB <== SDIIn
						in = ::GetFrameBufferInputXptFromChannel(NTV2Channel(fb + path));
						out = ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(NTV2Channel(sdi + path)));
						conns.insert(NTV2Connection(in, out));
					}	//	for each path
				}	//	SDI CH1234 YUV SQUARES
			}	//	YUV FBF
		}	//	3G SDI CH1234 or CH5678
	}	//	SDI
	return !conns.empty();
}

#if defined(AJALinux)
	int NTV2VCAM::ExtractNumber(const char* str)
	{
		string s(str);
		int i = s.length() - 1;
		while (i >= 0 && isdigit(s[i]))
			i--;
		return stoi(s.substr(i + 1));
	}
#endif	//	AJALinux

int NTV2VCAM::GetFps()
{
	NTV2FrameRate frameRate(NTV2_FRAMERATE_INVALID);
	mDevice.GetFrameRate(frameRate, mInputChannel);
	switch (frameRate)
	{
		case NTV2_FRAMERATE_1500:
		case NTV2_FRAMERATE_1498:			return 15;

		case NTV2_FRAMERATE_2500:			return 25;

		case NTV2_FRAMERATE_3000:
		case NTV2_FRAMERATE_2997:			return 30;

		case NTV2_FRAMERATE_4800:
		case NTV2_FRAMERATE_4795:			return 48;

		case NTV2_FRAMERATE_5000:			return 50;

		case NTV2_FRAMERATE_5994:
		case NTV2_FRAMERATE_6000:			return 60;

		case NTV2_FRAMERATE_12000:
		case NTV2_FRAMERATE_11988:			return 120;

		case NTV2_FRAMERATE_UNKNOWN:	//	All others assume 24fps
		case NTV2_FRAMERATE_1900:
		case NTV2_FRAMERATE_1898:
		case NTV2_FRAMERATE_1800:
		case NTV2_FRAMERATE_1798:
		case NTV2_NUM_FRAMERATES:
		case NTV2_FRAMERATE_2400:
		case NTV2_FRAMERATE_2398:			break;
	}
	return 24;
}

#if defined(AJA_WINDOWS)
	EnumPins::EnumPins(NTV2VCAM* filter_, EnumPins* pEnum)
		: mpFilter(filter_)
		, mCurPin(pEnum ? pEnum->mCurPin : 0)
	{
	}

	STDMETHODIMP EnumPins::QueryInterface(REFIID riid, void** ppv)
	{
		if (!ppv)
			return E_POINTER;

		if (riid == IID_IUnknown || riid == IID_IEnumPins)
		{
			*ppv = static_cast<IEnumPins*>(this);
			AddRef();
			return S_OK;
		}

		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) EnumPins::AddRef()
	{
		return (ULONG)InterlockedIncrement(&mRefCount);
	}

	STDMETHODIMP_(ULONG) EnumPins::Release()
	{
		long ref = InterlockedDecrement(&mRefCount);
		if (ref == 0)
		{
			delete this;
			return 0;
		}

		return ref;
	}

	STDMETHODIMP EnumPins::Next(ULONG cPins, IPin * *ppPins, ULONG * pcFetched)
	{
		if (!ppPins || (cPins > 1 && !pcFetched))
			return E_POINTER;

		ULONG fetched = 0;
		for (int i = 0; i < cPins && mCurPin < mpFilter->GetPinCount(); ++i)
		{
			IPin* pPin = mpFilter->GetPin(mCurPin);
			if (!pPin)
				return S_FALSE;

			ppPins[fetched++] = pPin;
			pPin->AddRef();
			mCurPin++;
		}

		if (pcFetched)
			*pcFetched = fetched;

		return fetched == cPins ? S_OK : S_FALSE;
	}

	STDMETHODIMP EnumPins::Skip(ULONG cPins)
	{
		mCurPin += cPins;
		if (mCurPin >= mpFilter->GetPinCount())
		{
			mCurPin = mpFilter->GetPinCount();
			return S_FALSE;
		}
		return S_OK;
	}

	STDMETHODIMP EnumPins::Reset()
	{
		mCurPin = 0;
		return S_OK;
	}

	STDMETHODIMP EnumPins::Clone(IEnumPins * *ppEnum)
	{
		if (!ppEnum)
			return E_POINTER;

		EnumPins* pNew = new EnumPins(mpFilter, this);
		if (!pNew)
			return E_OUTOFMEMORY;

		pNew->AddRef();
		*ppEnum = pNew;
		return S_OK;
	}

	EnumMediaTypes::EnumMediaTypes(OutputVideoPin * pin_) : pin(pin_)
	{
	}

	STDMETHODIMP EnumMediaTypes::QueryInterface(REFIID riid, void** ppv)
	{
		if (riid == IID_IUnknown || riid == IID_IEnumMediaTypes)
		{
			AddRef();
			*ppv = static_cast<IEnumMediaTypes*>(this);
			return NOERROR;
		}

		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) EnumMediaTypes::AddRef()
	{
		return (ULONG)InterlockedIncrement(&mRefCount);
	}

	STDMETHODIMP_(ULONG) EnumMediaTypes::Release()
	{
		long ref = InterlockedDecrement(&mRefCount);
		if (ref == 0)
		{
			delete this;
			return 0;
		}
		return ref;
	}

	STDMETHODIMP EnumMediaTypes::Next(ULONG cMediaTypes, AM_MEDIA_TYPE** ppMediaTypes, ULONG * pcFetched)
	{
		UINT nFetched = 0;
		if (curMT == 0 && cMediaTypes > 0)
		{
			AM_MEDIA_TYPE* pmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
			if (!pmt)
				return E_OUTOFMEMORY;

			ZeroMemory(pmt, sizeof(AM_MEDIA_TYPE));
			CMediaType mt;
			HRESULT hr = pin->GetMediaType(&mt);
			if (FAILED(hr))
			{
				CoTaskMemFree(pmt);
				pmt = nullptr;
				return hr == VFW_S_NO_MORE_ITEMS ? S_FALSE : hr;
			}
			CopyMediaType(pmt, &mt);

			ppMediaTypes[0] = pmt;
			nFetched = 1;
			curMT++;
		}

		if (pcFetched)
			*pcFetched = nFetched;

		return (nFetched == cMediaTypes) ? S_OK : S_FALSE;
	}

	STDMETHODIMP EnumMediaTypes::Skip(ULONG cMediaTypes)
	{
		if ((curMT + cMediaTypes) > 1)
		{
			curMT = 1;
			return S_FALSE;
		}
		curMT += cMediaTypes;
		return S_OK;
	}

	STDMETHODIMP EnumMediaTypes::Reset()
	{
		curMT = 0;
		return S_OK;
	}

	STDMETHODIMP EnumMediaTypes::Clone(IEnumMediaTypes** ppEnum)
	{
		if (!ppEnum)
			return E_POINTER;

		EnumMediaTypes* pNew = new EnumMediaTypes(pin);
		if (!pNew)
			return E_OUTOFMEMORY;

		pNew->curMT = curMT;
		pNew->AddRef();
		*ppEnum = pNew;
		return S_OK;
	}
#endif	//	defined(AJA_WINDOWS)
