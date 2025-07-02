/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2vcam.h
    @brief		Header file for NTV2VCAM demonstration class
    @copyright	(C) 2025 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2VCAM_H
#define _NTV2VCAM_H

#if defined(AJA_WINDOWS)
	#include <streams.h>
	#include <initguid.h>
	#include <vector>
	#include <mutex>
#endif

//AJA headers
#include <ntv2publicinterface.h>
#include <ntv2devicescanner.h>
#include <ajabase/common/common.h>
#include <ajabase/system/process.h>
#include "ntv2democommon.h"

#if defined(AJALinux)
	//V4L headers
	#include <linux/videodev2.h>
	#include <sys/ioctl.h>
	//ALSA headers
	#include <alsa/asoundlib.h>
#endif

//std headers
#include <signal.h>

//local headers
#if defined(AJALinux)
	#include "v4l2loopback.h"
#endif

#define AUDIO_BYTESPERSAMPLE 4
#if defined(AJALinux)
	#define AJA_MISSING_DEV_V4L2LOOPBACK	//	Uncomment if /dev/v4l2loopback missing
	#if !defined(AJA_MISSING_DEV_V4L2LOOPBACK)
		#define V4L2_DRIVER_NAME "/dev/v4l2loopback"
	#endif
#endif

using namespace std;

#define AJA_VW_SUCCESS									0
#define AJA_VW_INVALIDARGS								1
#define AJA_VW_INVALIDAJADEVICE							2
#define AJA_VW_AJADEVICENOTREADY						3
#define AJA_VW_AJADEVICENOCAPTURE						4
#define AJA_VW_INVALIDPIXELFORMAT						5
#define AJA_VW_AJADEVICENOPIXELFORMAT					6
#define AJA_VW_NOTSISUPPORT								7
#define AJA_VW_INVALIDINPUTCHANNEL						8
#define AJA_VW_INVALIDINPUTSOURCE						9
#define AJA_VW_ENABLECHANNELSFAILED						10
#define AJA_VW_ENABLEINPUTINTERRUPTFAILED				11
#define AJA_VW_SUBSCRIBEINPUTVERTICALEVENTFAILED		12
#define AJA_VW_SUBSCRIBEOUTPUTVERTICALEVENTFAILED		13
#define AJA_VW_SETSDITRANSITENABLEFAILED				14
#define AJA_VW_WAITFOROUTPUTVERTICALINTERRUPTFAILED		14
#define AJA_VW_INVALIDVIDEOFORMAT						15
#define AJA_VW_SETCAPTUREMODEFAILED						16
#define AJA_VW_SETFREERUNREFERENCEFAILED				17
#define AJA_VW_SETVIDEOFORMATFAILED						18
#define AJA_VW_SETVANCMODEOFFFAILED						19
#define AJA_VW_SETTSIFRAMEENABLEFAILED					20
#define AJA_VW_SET4KSQUARESENABLEFAILED					21
#define AJA_VW_SETFRAMEBUFFERFORMATFAILED				22
#define AJA_VW_INCORRECTMULTILINKAUDIOCHANNEL			23
#define AJA_VW_APPLYSIGNALROUTEFAILED					24
#define AJA_VW_ACINITFORINPUTFAILED						25
#define AJA_VW_ACSTARTFAILED							26
#define AJA_VW_ACSETVIDEOBUFFERFAILED					27
#define AJA_VW_ACSETAUDIOBUFFERFAILED					28
#define AJA_VW_ACSETANCBUFFERSFAILED					29
#define AJA_VW_V4L2DRIVEROPENFAILED						30
#define AJA_VW_V4L2DEVICECREATEFAILED					31
#define AJA_VW_V4L2DEVICEOPENFAILED						32
#define AJA_VW_ACTRANSFERFAILED							33
#define AJA_VW_ACGETINPUTTIMECODESFAILED				34
#define AJA_VW_ACSTOPFAILED								35
#define AJA_VW_V4L2DEVICEREMOVEFAILED					36
#define AJA_VW_ALSESETUPFAILED							37
#define AJA_VW_CONFIGAUDIOSYSTEMFAILED					38
#define AJA_VW_NOHDMISUPPORT							39
#define AJA_VW_GETFRAMESERVICESFAILED					40
#define AJA_VW_SETFRAMESERVICESFAILED					41
#define AJA_VW_GETDEVICEOWNERFAILED						42
#define AJA_VW_GETINPUTROUTEFAILED						43
#define AJA_VW_SETACFRAMERANGEFAILED					44
#define AJA_VW_MISSINGARGS								45
#define AJA_VW_GETPOINTERFAILED							46
#define AJA_VW_SAMPLEBUFFERSIZEMISMATCH					47

#if defined(AJA_WINDOWS)
#define VCAM_FILTER_NAME_W L"AJA Virtual Webcam"
#define VCAM_VIDEO_PIN_NAME_W L"AJA Virtual Webcam Video Output Pin"
#define VCAM_AUDIO_PIN_NAME_W L"AJA Virtual Webcam Audio Output Pin"

#define VCAM_FILTER_NAME "AJA Virtual Webcam"
#define VCAM_VIDEO_PIN_NAME "AJA Virtual Webcam Video Output Pin"
#define VCAM_AUDIO_PIN_NAME "AJA Virtual Webcam Audio Output Pin"

const GUID CLSID_VirtualWebcam=
{ 0xac313179, 0xa0af, 0x4110, { 0xa7, 0x15, 0xe5, 0xff, 0xe7, 0x26, 0x3c, 0xf0 } };

DEFINE_GUID(IID_IAjaFilterInterface,
    0x97d9d47a, 0x3851, 0x4298, 0xb8, 0x7c, 0x8a, 0xd, 0x11, 0xac, 0x9a, 0x19);
#endif	//	if defined(AJA_WINDOWS)

/*
 * Assumptions:
 * app requires root access to create V4L2 device
 * TSI only, no squares
 * set OEM tasks to prevent interference from AJA retail services
 * multi-channel not required for the time being
 * tested with vlc for video, ffplay for both video and audio
 * tested with obs for both video and audio; make sure the AJA plugins are not included in obs
 */

#if defined(AJA_WINDOWS)
	#define MAX_VIDEOS 24
	#define MAX_AUDIOS 24

	template <typename T>
	class CircularBuffer
	{
		private:
			std::vector<T> buffer;
			size_t head;
			size_t tail;
			size_t maxSize;
			bool isFull;
			std::mutex mtx;

		public:
			CircularBuffer(size_t size)
				: buffer(size), head(0), tail(0), maxSize(size), isFull(false)
			{
			}
	
			void reset()
			{
				std::lock_guard<std::mutex> lock(mtx);
				head = 0;
				tail = 0;
				isFull = false;
			}
	
			bool push (const T& item)
			{
				std::lock_guard<std::mutex> lock(mtx);
				buffer[head] = item;
				head = (head + 1) % maxSize;
				if (isFull)
					tail = (tail + 1) % maxSize;
				isFull = head == tail;
				return true;
			}
	
			bool pop (T& item)
			{
				std::lock_guard<std::mutex> lock(mtx);
				if (isEmpty())
					return false;
			
				item = buffer[tail];
				tail = (tail + 1) % maxSize;
				isFull = false;
				return true;
			}
	
			bool isEmpty() const
			{
				return !isFull && (head == tail);
			}
	
			size_t size() const
			{
				size_t size = maxSize;
				if (!isFull)
				{
					if (head >= tail)
						size = head - tail;
					else
						size = maxSize + head - tail;
				}
				return size;
			}
	
			size_t capacity() const
			{
				return maxSize;
			}
	};	//	CircularBuffer

	class OutputPin
	{
		public:
			virtual ~OutputPin() {}
	};

	class OutputVideoPin : public OutputPin, public CSourceStream, public IKsPropertySet, public IAMStreamConfig
	{
		friend class NTV2VCAM;
		public:
			OutputVideoPin (HRESULT* phr, CSource* pFilter, LPCWSTR pPinName);
			~OutputVideoPin();
	
		STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
		STDMETHODIMP_(ULONG) AddRef() override;
		STDMETHODIMP_(ULONG) Release() override;
	
		// CSourceStream methods
		STDMETHODIMP CheckMediaType(const CMediaType* pMediaType) override;
		STDMETHODIMP GetMediaType(CMediaType* pMediaType) override;
		STDMETHODIMP FillBuffer(IMediaSample* pSample) override;
	
		// CBaseOutputPin methods
		STDMETHODIMP DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pRequest) override;
	
		// CBasePin methods
		STDMETHODIMP EnumMediaTypes(IEnumMediaTypes** ppEnum) override;
	
		// IQualityControl methods
		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) override;
	
		// IKsPropertySet methods
		STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData) override;
		STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD* pcbReturned) override;
		STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport) override;
	
		// IAMStreamConfig methods
		STDMETHODIMP SetFormat(AM_MEDIA_TYPE* pmt) override;
		STDMETHODIMP GetFormat(AM_MEDIA_TYPE** ppmt) override;
		STDMETHODIMP GetNumberOfCapabilities(int* piCount, int* piSize) override;
		STDMETHODIMP GetStreamCaps(int iIndex, AM_MEDIA_TYPE** ppmt, BYTE* pSCC) override;
	};	//	OutputVideoPin

	class OutputAudioPin : public OutputPin, public CSourceStream, public IKsPropertySet, public IAMStreamConfig
	{
		friend class NTV2VCAM;
		public:
			OutputAudioPin(HRESULT* phr, CSource* pFilter, LPCWSTR pPinName);
			~OutputAudioPin();

			STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
			STDMETHODIMP_(ULONG) AddRef() override;
			STDMETHODIMP_(ULONG) Release() override;

			// CSourceStream methods
			STDMETHODIMP CheckMediaType(const CMediaType* pMediaType) override;
			STDMETHODIMP GetMediaType(CMediaType* pMediaType) override;
			STDMETHODIMP FillBuffer(IMediaSample* pSample) override;

			// CBaseOutputPin methods
			STDMETHODIMP DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pRequest) override;

			// CBasePin methods
			STDMETHODIMP EnumMediaTypes(IEnumMediaTypes** ppEnum) override;

			// IQualityControl methods
			STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) override;

			// IKsPropertySet methods
			STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData) override;
			STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD* pcbReturned) override;
			STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport) override;

			// IAMStreamConfig methods
			STDMETHODIMP SetFormat(AM_MEDIA_TYPE* pmt) override;
			STDMETHODIMP GetFormat(AM_MEDIA_TYPE** ppmt) override;
			STDMETHODIMP GetNumberOfCapabilities(int* piCount, int* piSize) override;
			STDMETHODIMP GetStreamCaps(int iIndex, AM_MEDIA_TYPE** ppmt, BYTE* pSCC) override;
	};	//	OutputAudioPin

	class NTV2VCAM : public CSource
	{
		public:
			static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);

			// CSource methods
			STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
			STDMETHODIMP_(ULONG) AddRef() override;
			STDMETHODIMP_(ULONG) Release() override;

			// CBaseFilter methods
			STDMETHODIMP EnumPins(IEnumPins** ppEnum) override;
			STDMETHODIMP FindPin(LPCWSTR Id, IPin** ppPin) override;
			STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo) override;
			STDMETHODIMP JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName) override;
			STDMETHODIMP QueryVendorInfo(LPWSTR* pVendorInfo) override;

			NTV2VCAM(LPUNKNOWN pUnk, HRESULT* phr);
			~NTV2VCAM();

			bool Initialize();
			HRESULT GetNextFrame(OutputPin* pPin, IMediaSample* pSample);
			ULWord GetNumAudioChannels() { return mNumAudioChannels; }
			ULWord GetAudioSampleRate() { return mSampleRate; }
			ULWord GetAudioBitsPerSample() { return mBitsPerSample; }
			ULWord GetNumAudioLinks() { return mNumAudioLinks; }
#endif	//	if defined(AJA_WINDOWS)

#if defined(AJALinux)
	class NTV2VCAM
	{
		public:		//	PUBLIC INSTANCE METHODS
			~NTV2VCAM();

			bool Initialize (int argc, const char** argv);
			bool Run (void);
#endif	//	if defined(AJALinux)

		private:	//	PRIVATE INSTANCE METHODS
			string	ULWordToString(const ULWord inNum);
			bool	Get4KInputFormat(NTV2VideoFormat& inOutVideoFormat);
			void	SetupAudio();
			bool	GetInputRouting(NTV2XptConnections& conns, const bool isInputRGB);
			bool	GetInputRouting4K(NTV2XptConnections& conns, const bool isInputRGB);
#if defined(AJALinux)
			int		ExtractNumber(const char* str);
#endif	//	if defined(AJALinux)
			int		GetFps();

		private:	//	PRIVATE INSTANCE DATA
			int							mErrorCode			= AJA_VW_SUCCESS;
			string						mAjaDevice;
			string						mInputType;
			string						mPixelFormatStr;
#if defined(AJALinux)
			string						mVideoDevice;
			string						mAudioDevice;
#endif	//	if defined(AJALinux)
			CNTV2Card					mDevice;
			bool						mIsKonaHDMI			= false;
			NTV2PixelFormat				mPixelFormat		= NTV2_FBF_8BIT_YCBCR;
			bool						mDoMultiFormat		= false;
			bool						mDoTSIRouting		= true;
			NTV2Channel					mInputChannel		= NTV2_CHANNEL_INVALID;
			NTV2InputSource				mInputSource		= NTV2_INPUTSOURCE_INVALID;
			UWord						mNumSpigots			= 0;
			NTV2ChannelSet				mActiveFrameStores;
			NTV2ChannelSet				mActiveSDIs;
			NTV2VideoFormat				mVideoFormat		= NTV2_FORMAT_UNKNOWN;
			NTV2FormatDesc				mFormatDesc;
			ULWord						mACOptions			= AUTOCIRCULATE_WITH_RP188 | AUTOCIRCULATE_WITH_ANC;
			NTV2IOKinds					mIOKinds			= NTV2_IOKINDS_SDI;
			NTV2Buffer					mVideoBuffer;
			NTV2TaskMode				mSavedTaskMode		= NTV2_TASK_MODE_INVALID;
			NTV2Buffer					mAudioBuffer;
			NTV2AudioSystem				mAudioSystem		= NTV2_AUDIOSYSTEM_INVALID;
			UWord						mNumAudioLinks		= 1;
			ULWord						mNumAudioChannels	= 1;
			unsigned int				mSampleRate			= 48000;
			ULWord						mBitsPerSample		= 32;
			NTV2ACFrameRange			mFrames;
			AUTOCIRCULATE_TRANSFER		mAcTransfer;
#if defined(AJALinux)
			snd_pcm_t *					mPcmHandle			= AJA_NULL;
			snd_pcm_uframes_t			mAudioFrames		= 2;
	#if !defined(AJA_MISSING_DEV_V4L2LOOPBACK)
			int							mLbDevice			= -1;
	#endif
			int							mLbDeviceNR			= -1;
			int							mLbDisplay			= -1;
#endif	//	if defined(AJALinux)
#if defined(AJA_WINDOWS)
			bool						mInitialized		= false;
			bool						mRunning			= false;
			CircularBuffer<NTV2Buffer>	mVideos;
			CircularBuffer<NTV2Buffer>	mAudios;
#endif	//	if defined(AJA_WINDOWS)
};	//	NTV2VCAM

#if defined(AJA_WINDOWS)
	class EnumPins : public IEnumPins
	{
		private:
			volatile long mRefCount = 1;
			NTV2VCAM* mpFilter = nullptr;
			int mCurPin = 0;

		public:
			EnumPins(NTV2VCAM* mpFilter, EnumPins* pEnum);

			// IUnknown
			STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
			STDMETHODIMP_(ULONG) AddRef();
			STDMETHODIMP_(ULONG) Release();

			// IEnumPins
			STDMETHODIMP Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched);
			STDMETHODIMP Skip(ULONG cPins);
			STDMETHODIMP Reset();
			STDMETHODIMP Clone(IEnumPins** ppEnum);
	};	//	EnumPins

	class EnumMediaTypes : public IEnumMediaTypes
	{
		private:
			volatile long mRefCount = 1;
			OutputVideoPin* pin = nullptr;
			UINT curMT = 0;

		public:
			EnumMediaTypes(OutputVideoPin* pin);

			// IUnknown
			STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
			STDMETHODIMP_(ULONG) AddRef();
			STDMETHODIMP_(ULONG) Release();

			// IEnumMediaTypes
			STDMETHODIMP Next(ULONG cMediaTypes, AM_MEDIA_TYPE** ppMediaTypes, ULONG* pcFetched);
			STDMETHODIMP Skip(ULONG cMediaTypes);
			STDMETHODIMP Reset();
			STDMETHODIMP Clone(IEnumMediaTypes** ppEnum);
	};	//	EnumMediaTypes
#endif	//	if defined(AJA_WINDOWS)

#endif // _NTV2VCAM_H
