/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2v4l2loopback.h
    @brief		Header file for NTV2V4L2Loopback demonstration class
    @copyright	(C) 2025 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2V4L2LOOPBACK_H
#define _NTV2V4L2LOOPBACK_H

#if defined (AJA_WINDOWS)
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

#if defined (AJALinux)
//V4L headers
#include <linux/videodev2.h>
#include <sys/ioctl.h>

//ALSA headers
#include <alsa/asoundlib.h>
#endif

//std headers
#include <signal.h>

//local headers
#if defined (AJALinux)
#include "v4l2loopback.h"
#endif
#include "error.h"

#define AUDIO_BYTESPERSAMPLE 4
#if defined (AJALinux)
#define V4L2_DRIVER_NAME "/dev/v4l2loopback"
#endif

using namespace std;

#if defined (AJA_WINDOWS)
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
#endif

/*
 * Assumptions:
 * app requires root access to create V4L2 device
 * TSI only, no squares
 * set OEM tasks to prevent interference from AJA retail services
 * multi-channel not required for the time being
 * tested with vlc for video, ffplay for both video and audio
 * tested with obs for both video and audio; make sure the AJA plugins are not included in obs
 */

#if defined (AJA_WINDOWS)
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

    bool push(const T& item)
    {
        std::lock_guard<std::mutex> lock(mtx);

        buffer[head] = item;
        head = (head + 1) % maxSize;

        if (isFull)
        {
            tail = (tail + 1) % maxSize;
        }

        isFull = head == tail;
        return true;
    }

    bool pop(T& item)
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (isEmpty())
        {
            return false;
        }

        item = buffer[tail];
        tail = (tail + 1) % maxSize;
        isFull = false;

        return true;
    }

    bool isEmpty() const
    {
        return (!isFull && (head == tail));
    }

    size_t size() const
    {
        size_t size = maxSize;

        if (!isFull)
        {
            if (head >= tail)
            {
                size = head - tail;
            }
            else
            {
                size = maxSize + head - tail;
            }
        }

        return size;
    }

    size_t capacity() const
    {
        return maxSize;
    }
};

class OutputPin
{
public:
    virtual ~OutputPin() {}
};

class OutputVideoPin : public OutputPin, public CSourceStream, public IKsPropertySet, public IAMStreamConfig
{
    friend class NTV2V4L2Loopback;
public:
    OutputVideoPin(HRESULT* phr, CSource* pFilter, LPCWSTR pPinName);
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
};

class OutputAudioPin : public OutputPin, public CSourceStream, public IKsPropertySet, public IAMStreamConfig
{
	friend class NTV2V4L2Loopback;
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
};

class NTV2V4L2Loopback : public CSource
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

    NTV2V4L2Loopback(LPUNKNOWN pUnk, HRESULT* phr);
    ~NTV2V4L2Loopback();

    bool Initialize();
	HRESULT GetNextFrame(OutputPin* pPin, IMediaSample* pSample);
	ULWord GetNumAudioChannels() { return mNumAudioChannels; }
	ULWord GetAudioSampleRate() { return mSampleRate; }
	ULWord GetAudioBitsPerSample() { return mBitsPerSample; }
	ULWord GetNumAudioLinks() { return mNumAudioLinks; }
#endif

#if defined (AJALinux)
class NTV2V4L2Loopback
{
public:
    ~NTV2V4L2Loopback();

    bool Initialize(int argc, const char** argv);
    bool Run(void);
#endif

private:
    string ULWordToString(const ULWord inNum);
    bool Get4KInputFormat(NTV2VideoFormat& inOutVideoFormat);
    void SetupAudio();
    bool GetInputRouting(NTV2XptConnections& conns, const bool isInputRGB);
    bool GetInputRouting4K(NTV2XptConnections& conns, const bool isInputRGB);
#if defined (AJALinux)
    int ExtractNumber(const char* str);
#endif
    int GetFps();

    int mErrorCode = AJA_VW_SUCCESS;
#if defined (AJALinux)
	char* mAjaDevice;
	char* mInputType;
	char* mPixelFormatStr;
#endif
#if defined (AJA_WINDOWS)
	string mAjaDevice;
    string mInputType;
    string mPixelFormatStr;
#endif
#if defined (AJALinux)
    char* mVideoDevice = AJA_NULL;
#endif
    CNTV2Card mDevice;
    NTV2DeviceID mDeviceID = DEVICE_ID_NOTFOUND;
    bool mIsKonaHDMI = false;
    NTV2PixelFormat mPixelFormat = NTV2_FBF_8BIT_YCBCR;
    bool mDoMultiFormat = false;
    bool mDoTSIRouting = true;
    NTV2Channel mInputChannel = NTV2_CHANNEL_INVALID;
    NTV2Channel mInputChannelArg = NTV2_CHANNEL_INVALID;
    NTV2InputSource mInputSource = NTV2_INPUTSOURCE_INVALID;
    UWord mNumSpigots = 0;
    NTV2ChannelSet mActiveFrameStores;
    NTV2ChannelSet mActiveSDIs;
    NTV2VideoFormat mVideoFormat = NTV2_FORMAT_UNKNOWN;
    NTV2FormatDesc mFormatDesc;
    ULWord mACOptions = AUTOCIRCULATE_WITH_RP188 | AUTOCIRCULATE_WITH_ANC;
    NTV2IOKinds mIOKinds = NTV2_IOKINDS_SDI;
    NTV2Buffer mVideoBuffer = 0;
    NTV2TaskMode mSavedTaskMode = NTV2_TASK_MODE_INVALID;
#if defined (AJALinux)
    char* mAudioDevice = AJA_NULL;
#endif
    NTV2Buffer mAudioBuffer = 0;
    NTV2AudioSystem mAudioSystem = NTV2_AUDIOSYSTEM_INVALID;
    UWord mNumAudioLinks = 1;
    ULWord mNumAudioChannels = 1;
    unsigned int mSampleRate = 48000;
    ULWord mBitsPerSample = 32;
    NTV2ACFrameRange mFrames;
    AUTOCIRCULATE_TRANSFER mAcTransfer;
#if defined (AJALinux)
    snd_pcm_t* mPcmHandle = NULL;
    snd_pcm_uframes_t mAudioFrames = 2;
    int mLbDevice = -1;
    int mLbDeviceNR = -1;
    int mLbDisplay = -1;
#endif
#if defined (AJA_WINDOWS)
	bool mInitialized = false;
	bool mRunning = false;
    CircularBuffer<NTV2Buffer> mVideos;
    CircularBuffer<NTV2Buffer> mAudios;
#endif
};

#if defined (AJA_WINDOWS)
class EnumPins : public IEnumPins
{
private:
    volatile long mRefCount = 1;
    NTV2V4L2Loopback* mpFilter = nullptr;
    int mCurPin = 0;

public:
    EnumPins(NTV2V4L2Loopback* mpFilter, EnumPins* pEnum);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumPins
    STDMETHODIMP Next(ULONG cPins, IPin** ppPins, ULONG* pcFetched);
    STDMETHODIMP Skip(ULONG cPins);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumPins** ppEnum);
};

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
};
#endif

#endif // _NTV2V4L2LOOPBACK_H
