/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2encodehevcvif.cpp
	@brief		Implementation of NTV2EncodeHEVCVif class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include <stdio.h>

#include "ntv2encodehevcvif.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2devicefeatures.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"

using namespace std;

#define NTV2_AUDIOSIZE_MAX		(401 * 1024)

#define NUM_OVERLAY_BARS		12
//static const uint32_t sOverlayBar0[] = {
//	0xc00000c0, 0x00000000, 0xc000c000, 0x00000000, 0xc0c00000, 0x00000000,
//	0xc0c000c0, 0x00000000, 0xc0c0c000, 0x00000000, 0xc000c0c0, 0x00000000,
//	0xc00000c0, 0x00000000, 0xc000c000, 0x00000000, 0xc0c00000, 0x00000000,
//	0xc0c000c0, 0x00000000, 0xc0c0c000, 0x00000000, 0xc000c0c0, 0x00000000 };
static const uint32_t sOverlayBar0[] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static const uint32_t sOverlayBar1[] = {
	0x00000000, 0xc00000c0, 0x00000000, 0xc000c000, 0x00000000, 0xc0c00000,
	0x00000000, 0xc0c000c0, 0x00000000, 0xc0c0c000, 0x00000000, 0xc000c0c0,
	0x00000000, 0xc00000c0, 0x00000000, 0xc000c000, 0x00000000, 0xc0c00000,
	0x00000000, 0xc0c000c0, 0x00000000, 0xc0c0c000, 0x00000000, 0xc000c0c0 };
//static const uint32_t sOverlayBar1[] = {
//	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
//	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
//	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
//	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };


NTV2EncodeHEVCVif::NTV2EncodeHEVCVif (const string				inDeviceSpecifier,
                                const M31VideoPreset		inPreset,
                                const NTV2FrameBufferFormat	inPixelFormat,
                                const uint32_t              inAudioChannels,
                                const bool                  inInfoData,
                                const uint32_t              inMaxFrames)

:	mACInputThread          (AJAThread()),
	mCodecHevcThread		(AJAThread()),
    mAVFileThread 			(AJAThread()),
    mM31					(AJA_NULL),
    mHevcCommon             (AJA_NULL),
	mDeviceID				(DEVICE_ID_NOTFOUND),
	mDeviceSpecifier		(inDeviceSpecifier),
    mWithAudio				(inAudioChannels != 0),
    mInputChannel			(NTV2_CHANNEL1),
    mOutputChannel			(NTV2_CHANNEL5),
    mEncodeChannel          (M31_CH0),
    mPreset					(inPreset),
	mInputSource			(NTV2_INPUTSOURCE_SDI1),
    mInputFormat			(NTV2_MAX_NUM_VIDEO_FORMATS),
    mVideoFormat			(NTV2_MAX_NUM_VIDEO_FORMATS),
    mCapturePixelFormat		(NTV2_FBF_8BIT_YCBCR),
    mOverlayPixelFormat		(NTV2_FBF_ABGR),
    mCodecPixelFormat		(inPixelFormat),
    mWithInfo               (inInfoData),
	mAudioSystem			(NTV2_AUDIOSYSTEM_1),
	mSavedTaskMode			(NTV2_STANDARD_TASKS),
    mNumAudioChannels       (0),
    mFileAudioChannels      (inAudioChannels),
    mMaxFrames              (inMaxFrames),
	mLastFrame				(false),
	mLastFrameInput			(false),
	mLastFrameHevc			(false),
    mLastFrameVideo			(false),
    mGlobalQuit				(false),
	mFrameData				(AJA_NULL),
	mSilentBuffer			(AJA_NULL),
	mVideoInputFrameCount	(0),		
	mCodecHevcFrameCount	(0),
    mAVFileFrameCount	 	(0),
    mRawFrameCount		 	(0),
    mInfoFrameCount		 	(0),
	mOverlayIndex			(0)
{
    ::memset (mACInputBuffer, 0x0, sizeof (mACInputBuffer));
    ::memset (mVideoHevcBuffer, 0x0, sizeof (mVideoHevcBuffer));
	::memset (&mOverlayBuffer, 0, sizeof (mOverlayBuffer));
	::memset (&mOverlayFrame, 0, sizeof (mOverlayFrame));

}	//	constructor


NTV2EncodeHEVCVif::~NTV2EncodeHEVCVif ()
{
	//	Stop my capture and consumer threads, then destroy them...
	Quit ();
	
	// unsubscribe from input vertical event...
	mDevice.UnsubscribeInputVerticalEvent (mInputChannel);

	// free all my buffers...
	for (unsigned bufferNdx = 0; bufferNdx < VIDEO_RING_SIZE; bufferNdx++)
	{
        if (mACInputBuffer[bufferNdx].pVideoBuffer)
		{
            delete [] mACInputBuffer[bufferNdx].pVideoBuffer;
            mACInputBuffer[bufferNdx].pVideoBuffer = AJA_NULL;
		}
		if (mACInputBuffer[bufferNdx].pInfoBuffer)
		{
		 	delete [] mACInputBuffer[bufferNdx].pInfoBuffer;
			mACInputBuffer[bufferNdx].pInfoBuffer = AJA_NULL;
		}
        if (mACInputBuffer[bufferNdx].pAudioBuffer)
		{
            delete [] mACInputBuffer[bufferNdx].pAudioBuffer;
            mACInputBuffer[bufferNdx].pAudioBuffer = AJA_NULL;
		}

        if (mVideoHevcBuffer[bufferNdx].pVideoBuffer)
		{
            delete [] mVideoHevcBuffer[bufferNdx].pVideoBuffer;
            mVideoHevcBuffer[bufferNdx].pVideoBuffer = AJA_NULL;
		}
		if (mVideoHevcBuffer[bufferNdx].pInfoBuffer)
		{
		 	delete [] mVideoHevcBuffer[bufferNdx].pInfoBuffer;
			mVideoHevcBuffer[bufferNdx].pInfoBuffer = AJA_NULL;
		}
        if (mVideoHevcBuffer[bufferNdx].pAudioBuffer)
		{
            delete [] mVideoHevcBuffer[bufferNdx].pAudioBuffer;
            mVideoHevcBuffer[bufferNdx].pAudioBuffer = AJA_NULL;
		}
    }

	if (mSilentBuffer != AJA_NULL)
	{
		delete [] mSilentBuffer;
		mSilentBuffer = AJA_NULL;
	}
	if (mOverlayBuffer[0] != AJA_NULL)
	{
		delete [] mOverlayBuffer[0];
		mOverlayBuffer[0] = AJA_NULL;
	}
	if (mOverlayBuffer[1] != AJA_NULL)
	{
		delete [] mOverlayBuffer[1];
		mOverlayBuffer[1] = AJA_NULL;
	}

} // destructor


void NTV2EncodeHEVCVif::Quit (void)
{
    if (mM31 && !mLastFrame && !mGlobalQuit)
	{
        //	Set the last frame flag to start the quit process
		mLastFrame = true;

        //	Stop the encoder stream
        if (!mM31->ChangeEHState(Hevc_EhState_ReadyToStop, mEncodeChannel))
            { cerr << "## ERROR:  ChangeEHState ready to stop failed" << endl; }

		//	Wait for the last frame to be written to disk
		int i;
		int timeout = 300;
		for (i = 0; i < timeout; i++)
		{
            if (mLastFrameVideo) break;
			AJATime::Sleep (10);
		}
		if (i == timeout)
			{ cerr << "## ERROR:  Wait for last frame timeout" << endl; }

        if (!mM31->ChangeEHState(Hevc_EhState_Stop, mEncodeChannel))
            { cerr << "## ERROR:  ChangeEHState stop failed" << endl; }

        //	Stop the video input stream
        if (!mM31->ChangeVInState(Hevc_VinState_Stop, mEncodeChannel))
            { cerr << "## ERROR:  ChangeVInState stop failed" << endl; }

        //	Now go to the init state
        if (!mM31->ChangeMainState(Hevc_MainState_Init, Hevc_EncodeMode_Single))
        { cerr << "## ERROR:  ChangeMainState to init failed" << endl; }
    }

	//	Stop the worker threads
	mGlobalQuit = true;

	while (mACInputThread.Active())
		AJATime::Sleep(10);

	while (mCodecHevcThread.Active())
		AJATime::Sleep(10);

	while (mAVFileThread.Active())
		AJATime::Sleep(10);

    //  Stop video capture
    mDevice.SetMode(mInputChannel, NTV2_MODE_DISPLAY, false);

    //  Release board
	mDevice.ReleaseStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));
	mDevice.SetEveryFrameServices (mSavedTaskMode);		//	Restore prior task mode

    //  Close output files
    mHevcCommon->CloseHevcFile();
    mHevcCommon->CloseRawFile();
    if (mWithInfo)
        mHevcCommon->CloseEncFile();
    if (mWithAudio)
        mHevcCommon->CloseAiffFile();

}	//	Quit


AJAStatus NTV2EncodeHEVCVif::Init (void)
{
    AJAStatus	status	(AJA_STATUS_SUCCESS);
    
    //	Open the device...
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
    { cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN; }
    
    //  Grab board in a shared environment
	if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
		return AJA_STATUS_BUSY;							//	Another app is using the device
	mDevice.GetEveryFrameServices (mSavedTaskMode);		//	Save the current state before we change it
    mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);		//	Since this is an OEM demo, use the OEM service level
    
    mDeviceID = mDevice.GetDeviceID ();					//	Keep the device ID handy, as it's used frequently
    
    // Make sure this device has an M31
    if (!NTV2DeviceHasHEVCM31 (mDeviceID))
	{
  	 	cerr << "## ERROR:  M31 not found" << endl;
        return AJA_STATUS_FAIL;
	}

    // Allocate our M31 helper class and our HEVC common class
    mM31 = new CNTV2m31 (&mDevice);
    mHevcCommon = new CNTV2DemoHevcCommon ();
    
    if ((mM31 == AJA_NULL) || (mHevcCommon == AJA_NULL))
    {
        return AJA_STATUS_FAIL;
    }
    
    //  Preset specification takes precedence
    if (mPreset < M31_NUMVIDEOPRESETS)
    {
        // This class only handles vif based presets so make sure they didn't pass in a file one
        if (!CNTV2m31::IsPresetVIF(mPreset))
            return AJA_STATUS_FAIL;
        
        //	Get NTV2 formats to match codec preset
        mInputFormat = CNTV2m31::GetPresetVideoFormat(mPreset);
        mCodecPixelFormat = CNTV2m31::GetPresetFrameBufferFormat(mPreset);
    }
    //  Otherwise use the pixel format and SDI input format
    else if (mCodecPixelFormat >= NTV2_FBF_NUMFRAMEBUFFERFORMATS)
    {
         mCodecPixelFormat = NTV2_FBF_8BIT_YCBCR_420PL2;
    }

    //  When video format is unknown determine from SDI input
    if (mInputFormat >= NTV2_MAX_NUM_VIDEO_FORMATS)
    {
		bool is3Gb = false;
		mDevice.GetSDIInput3GbPresent (is3Gb, mInputChannel);

		//  Get SDI input format
		status = mHevcCommon->DetermineInputFormat(mDevice.GetSDIInputVideoFormat(mInputChannel, is3Gb), true, mInputFormat);
        if (AJA_FAILURE(status))
            return status;

        //  Get codec preset for input format
        if(!CNTV2m31::ConvertVideoFormatToPreset(mInputFormat, mCodecPixelFormat, true, mPreset))
            return AJA_STATUS_FAIL;
    }

	// Capture format is scaled HD
	switch (mInputFormat)
	{
	case NTV2_FORMAT_4x1920x1080p_5000: mVideoFormat = NTV2_FORMAT_1080p_5000_A; break;
	case NTV2_FORMAT_4x1920x1080p_5994: mVideoFormat = NTV2_FORMAT_1080p_5994_A; break;
	case NTV2_FORMAT_4x1920x1080p_6000: mVideoFormat = NTV2_FORMAT_1080p_6000_A; break;
	default:
		return AJA_STATUS_FAIL;
	}

	//	Setup the circular buffers
	SetupHostBuffers ();
  
    //	Setup frame buffer
	status = SetupVideo ();
	if (AJA_FAILURE (status))
		return status;

	//	Setup audio buffer
	status = SetupAudio ();
	if (AJA_FAILURE (status))
		return status;

	//	Route input signals
	RouteInputSignal ();

	//	Setup to capture video/audio/anc input
    SetupAutoCirculate ();

	//	Setup codec
    status = mHevcCommon->SetupHEVC (mM31, mPreset, mEncodeChannel, false, mWithInfo);
    if (AJA_FAILURE (status))
        return status;

	//	Create encoded video output file
    status = mHevcCommon->CreateHevcFile ("raw.hevc", mMaxFrames);
    if (AJA_FAILURE (status))
        return status;

	//	Create rw video output file
    status = mHevcCommon->CreateRawFile ("raw.yuv", mMaxFrames);
    if (AJA_FAILURE (status))
        return status;

    if (mWithInfo)
    {
        //	Create encoded data output file
        status = mHevcCommon->CreateEncFile ("raw.txt", mMaxFrames);
        if (AJA_FAILURE (status))
            return status;
    }

    if (mWithAudio)
    {
        //	Create audio output file
        status = mHevcCommon->CreateAiffFile ("raw.aiff", mFileAudioChannels, mMaxFrames, NTV2_AUDIOSIZE_MAX);
        if (AJA_FAILURE (status))
            return status;
    }

    return AJA_STATUS_SUCCESS;

}	//	Init


M31VideoPreset	NTV2EncodeHEVCVif::GetCodecPreset (void)
{
    return mPreset;
}
    
    
AJAStatus NTV2EncodeHEVCVif::SetupAudio (void)
{
	//	Have the audio system capture audio from the designated device input (i.e., ch1 uses SDIIn1, ch2 uses SDIIn2, etc.)...
	mDevice.SetAudioSystemInputSource (mAudioSystem, NTV2_AUDIO_EMBEDDED, ::NTV2ChannelToEmbeddedAudioInput (mInputChannel));

    mNumAudioChannels = ::NTV2DeviceGetMaxAudioChannels (mDeviceID);
    mDevice.SetNumberAudioChannels (mNumAudioChannels, mAudioSystem);
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);
    mDevice.SetEmbeddedAudioClock (NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT, mAudioSystem);
	mDevice.GetAudioRate (mAudioRate, mAudioSystem);

	//	The on-device audio buffer should be 4MB to work best across all devices & platforms...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


void NTV2EncodeHEVCVif::SetupHostBuffers (void)
{
    mVideoBufferSize = GetVideoActiveSize (mVideoFormat, mCapturePixelFormat, NTV2_VANCMODE_OFF);
    mPicInfoBufferSize = sizeof(HevcPictureInfo)*2;
    mEncInfoBufferSize = sizeof(HevcEncodedInfo)*2;
    mAudioBufferSize = NTV2_AUDIOSIZE_MAX;
	mOverlayBufferSize = GetVideoActiveSize(mVideoFormat, mOverlayPixelFormat, NTV2_VANCMODE_OFF) * 4;
	NTV2FormatDescriptor overlayD (mVideoFormat, mOverlayPixelFormat);
	
	// audio/video input ring
    mACInputCircularBuffer.SetAbortFlag (&mGlobalQuit);
	for (unsigned bufferNdx = 0; bufferNdx < VIDEO_RING_SIZE; bufferNdx++ )
	{
        memset (&mACInputBuffer[bufferNdx], 0, sizeof(AVHevcDataBuffer));
        mACInputBuffer[bufferNdx].pVideoBuffer		= new uint32_t [mVideoBufferSize/4];
        mACInputBuffer[bufferNdx].videoBufferSize	= mVideoBufferSize;
        mACInputBuffer[bufferNdx].videoDataSize		= 0;
        mACInputBuffer[bufferNdx].videoDataSize2	= 0;
		mACInputBuffer[bufferNdx].pAudioBuffer		= new uint32_t [mAudioBufferSize/4];
		mACInputBuffer[bufferNdx].audioBufferSize	= mAudioBufferSize;
		mACInputBuffer[bufferNdx].audioDataSize		= 0;
		mACInputBuffer[bufferNdx].pInfoBuffer		= new uint32_t [mPicInfoBufferSize/4];
		mACInputBuffer[bufferNdx].infoBufferSize    = mPicInfoBufferSize;
		mACInputBuffer[bufferNdx].infoDataSize		= 0;
		mACInputBuffer[bufferNdx].infoDataSize2		= 0;
        mACInputCircularBuffer.Add (& mACInputBuffer[bufferNdx]);
	}

    // video hevc ring
    mVideoHevcCircularBuffer.SetAbortFlag (&mGlobalQuit);
    for (unsigned bufferNdx = 0; bufferNdx < VIDEO_RING_SIZE; bufferNdx++ )
    {
        memset (&mVideoHevcBuffer[bufferNdx], 0, sizeof(AVHevcDataBuffer));
        mVideoHevcBuffer[bufferNdx].pVideoBuffer	= new uint32_t [mVideoBufferSize/4];
        mVideoHevcBuffer[bufferNdx].videoBufferSize	= mVideoBufferSize;
        mVideoHevcBuffer[bufferNdx].videoDataSize	= 0;
        mVideoHevcBuffer[bufferNdx].videoDataSize2	= 0;
        mVideoHevcBuffer[bufferNdx].pInfoBuffer		= new uint32_t [mEncInfoBufferSize/4];
        mVideoHevcBuffer[bufferNdx].infoBufferSize  = mEncInfoBufferSize;
        mVideoHevcBuffer[bufferNdx].infoDataSize	= 0;
        mVideoHevcBuffer[bufferNdx].infoDataSize2	= 0;
        mVideoHevcCircularBuffer.Add (& mVideoHevcBuffer[bufferNdx]);
    }

	// audio silent buffer
	mSilentBuffer = new uint32_t [mAudioBufferSize/4];
	memset(mSilentBuffer, 0, mAudioBufferSize);

	// overlay buffers
	mOverlayBuffer[0] = new uint32_t [mOverlayBufferSize/4];
	mOverlayBuffer[1] = new uint32_t [mOverlayBufferSize/4];

	uint32_t* buf0 = mOverlayBuffer[0];
	uint32_t* buf1 = mOverlayBuffer[1];
	uint32_t pixelsPerLine = overlayD.linePitch*2;
	uint32_t linesPerBar = overlayD.numLines*2/NUM_OVERLAY_BARS;
	
	for (uint32_t i = 0; i < NUM_OVERLAY_BARS; i++)
	{
		for (uint32_t j = 0; j < linesPerBar; j++)
		{
			for (uint32_t k = 0; k < pixelsPerLine; k++)
			{
				*buf0++ = sOverlayBar0[i];
				*buf1++ = sOverlayBar1[i];
			}
		}
	}

}	//	SetupHostBuffers


AJAStatus NTV2EncodeHEVCVif::SetupVideo (void)
{
	//	Disable multiformat
	mDevice.SetMultiFormatMode (false);

	//	Set the board video format
	mDevice.SetVideoFormat (mVideoFormat, false, false, NTV2_CHANNEL1);
	mDevice.GetFrameRate (mFrameRate, NTV2_CHANNEL1);
	mDevice.SetQuadFrameEnable (true, NTV2_CHANNEL5);
	mDevice.Set4kSquaresEnable (true, NTV2_CHANNEL5);

	//	Set frame buffer format
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mCapturePixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mCapturePixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mCapturePixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mCapturePixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL5, mOverlayPixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL6, mOverlayPixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL7, mOverlayPixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL8, mOverlayPixelFormat);

	// Setup overlay
	mOverlayFrame[0] = 32;
	mOverlayFrame[1] = 36;
	mDevice.DMAWriteFrame (mOverlayFrame[0], mOverlayBuffer[0], mOverlayBufferSize);
	mDevice.DMAWriteFrame (mOverlayFrame[1], mOverlayBuffer[1], mOverlayBufferSize);
	mDevice.SetOutputFrame (NTV2_CHANNEL5, mOverlayFrame[0]/4);
	mOverlayIndex = 0;

	//	Set catpure mode
	mDevice.SetMode (NTV2_CHANNEL1, NTV2_MODE_CAPTURE, false);
	mDevice.SetMode (NTV2_CHANNEL2, NTV2_MODE_DISPLAY, false);
	mDevice.SetMode (NTV2_CHANNEL3, NTV2_MODE_DISPLAY, false);
	mDevice.SetMode (NTV2_CHANNEL4, NTV2_MODE_DISPLAY, false);
	mDevice.SetMode (NTV2_CHANNEL5, NTV2_MODE_DISPLAY, false);
	mDevice.SetMode (NTV2_CHANNEL6, NTV2_MODE_DISPLAY, false);
	mDevice.SetMode (NTV2_CHANNEL7, NTV2_MODE_DISPLAY, false);
	mDevice.SetMode (NTV2_CHANNEL8, NTV2_MODE_DISPLAY, false);

	//	Enable frame buffers
	mDevice.EnableChannel (NTV2_CHANNEL1);
	mDevice.DisableChannel (NTV2_CHANNEL2);
	mDevice.DisableChannel (NTV2_CHANNEL3);
	mDevice.DisableChannel (NTV2_CHANNEL4);
	mDevice.EnableChannel (NTV2_CHANNEL5);
	mDevice.EnableChannel (NTV2_CHANNEL6);
	mDevice.EnableChannel (NTV2_CHANNEL7);
	mDevice.EnableChannel (NTV2_CHANNEL8);

	//	Setup mixers to key fg over bg
	for (UWord i = 0; i < 4; i++)
	{
		mDevice.SetMixerVancOutputFromForeground (i, true);
		mDevice.SetMixerFGInputControl (i, NTV2MIXERINPUTCONTROL_UNSHAPED);
		mDevice.SetMixerBGInputControl (i, NTV2MIXERINPUTCONTROL_FULLRASTER);
		mDevice.SetMixerCoefficient (i, 0x00000); // 0x10000 forground - 0x00000 background
		mDevice.SetMixerMode (i, NTV2MIXERMODE_FOREGROUND_ON);
	}

	//	Save input source
	mInputSource = ::NTV2ChannelToInputSource (NTV2_CHANNEL1);

	//	Set the device reference to the input...
	mDevice.SetReference (::NTV2InputSourceToReferenceSource (mInputSource));

	//	Enable and subscribe to the interrupts for the channel to be used...
	mDevice.EnableInputInterrupt (mInputChannel);
	mDevice.SubscribeInputVerticalEvent (mInputChannel);

    //  Setup for picture info
    mTimeBase.SetAJAFrameRate (mHevcCommon->GetAJAFrameRate(GetNTV2FrameRateFromVideoFormat (mVideoFormat)));

	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


void NTV2EncodeHEVCVif::RouteInputSignal (void)
{
    // setup sdi io
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, false);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, false);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, false);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL5, true);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL6, true);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL7, true);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL8, true);

	//	Give the device some time to lock to the input signal...
	mDevice.WaitForOutputVerticalInterrupt (mInputChannel, 8);

	//	When input is 3Gb convert to 3Ga for capture (no RGB support?)
	bool is3Gb = false;
	mDevice.GetSDIInput3GbPresent (is3Gb, mInputChannel);

	mDevice.SetSDIInLevelBtoLevelAConversion (NTV2_CHANNEL1, is3Gb);
	mDevice.SetSDIInLevelBtoLevelAConversion (NTV2_CHANNEL2, is3Gb);
	mDevice.SetSDIInLevelBtoLevelAConversion (NTV2_CHANNEL3, is3Gb);
	mDevice.SetSDIInLevelBtoLevelAConversion (NTV2_CHANNEL4, is3Gb);
	mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL5, false);
	mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL6, false);
	mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL7, false);
	mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL8, false);

	//	Use a "Routing" object, which handles the details of writing
	//	the appropriate values into the appropriate device registers...
	CNTV2SignalRouter	router;

	// quad csc from frame buffer
    router.AddConnection (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer5RGB);
    router.AddConnection (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer6RGB);
    router.AddConnection (NTV2_XptCSC3VidInput, NTV2_XptFrameBuffer7RGB);
    router.AddConnection (NTV2_XptCSC4VidInput, NTV2_XptFrameBuffer8RGB);
	// quad mixer foreground from csc
	router.AddConnection (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
	router.AddConnection (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
	router.AddConnection (NTV2_XptMixer2FGVidInput, NTV2_XptCSC2VidYUV);
	router.AddConnection (NTV2_XptMixer2FGKeyInput, NTV2_XptCSC2KeyYUV);
	router.AddConnection (NTV2_XptMixer3FGVidInput, NTV2_XptCSC3VidYUV);
	router.AddConnection (NTV2_XptMixer3FGKeyInput, NTV2_XptCSC3KeyYUV);
	router.AddConnection (NTV2_XptMixer4FGVidInput, NTV2_XptCSC4VidYUV);
	router.AddConnection (NTV2_XptMixer4FGKeyInput, NTV2_XptCSC4KeyYUV);
	// quad mixer background from sdi input
	router.AddConnection (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
	router.AddConnection (NTV2_XptMixer1BGKeyInput, NTV2_XptBlack);
	router.AddConnection (NTV2_XptMixer2BGVidInput, NTV2_XptSDIIn2);
	router.AddConnection (NTV2_XptMixer2BGKeyInput, NTV2_XptBlack);
	router.AddConnection (NTV2_XptMixer3BGVidInput, NTV2_XptSDIIn3);
	router.AddConnection (NTV2_XptMixer3BGKeyInput, NTV2_XptBlack);
	router.AddConnection (NTV2_XptMixer4BGVidInput, NTV2_XptSDIIn4);
	router.AddConnection (NTV2_XptMixer4BGKeyInput, NTV2_XptBlack);
	// quad sdi output (to codec) from mixer
	router.AddConnection (NTV2_XptSDIOut5Input, NTV2_XptMixer1VidYUV);
	router.AddConnection (NTV2_XptSDIOut6Input, NTV2_XptMixer2VidYUV);
	router.AddConnection (NTV2_XptSDIOut7Input, NTV2_XptMixer3VidYUV);
	router.AddConnection (NTV2_XptSDIOut8Input, NTV2_XptMixer4VidYUV);
	// 4k down converter from mixer
	router.AddConnection (NTV2_Xpt4KDCQ1Input, NTV2_XptMixer1VidYUV);
	router.AddConnection (NTV2_Xpt4KDCQ2Input, NTV2_XptMixer2VidYUV);
	router.AddConnection (NTV2_Xpt4KDCQ3Input, NTV2_XptMixer3VidYUV);
	router.AddConnection (NTV2_Xpt4KDCQ4Input, NTV2_XptMixer4VidYUV);
	// frame buffer from 4k down converter
	router.AddConnection (NTV2_XptFrameBuffer1Input, NTV2_Xpt4KDownConverterOut);

    //	Add this signal routing (or replace if not doing multistream)...
    mDevice.ApplySignalRoute (router, true);

	//	Give the device some time to lock to the input signal...
	mDevice.WaitForOutputVerticalInterrupt (mInputChannel, 8);

}	//	RouteInputSignal


void NTV2EncodeHEVCVif::SetupAutoCirculate (void)
{
	//	Tell capture AutoCirculate to use 8 frame buffers on the device...
	mDevice.AutoCirculateStop (mInputChannel);
	mDevice.AutoCirculateInitForInput (mInputChannel,	16,	//	Frames to circulate
									   mWithAudio ? mAudioSystem : NTV2_AUDIOSYSTEM_INVALID);	//	Which audio system (if any)?
}	//	SetupInputAutoCirculate


AJAStatus NTV2EncodeHEVCVif::Run ()
{
	if (mDevice.GetInputVideoFormat (mInputSource) == NTV2_FORMAT_UNKNOWN)
		cout << endl << "## WARNING:  No video signal present on the input connector" << endl;

	//	Start the playout and capture threads...
    StartAVFileThread ();
    StartVideoInputThread ();

	if (mWithInfo)
	{
		//	Transfer initial picture info
		for (int i = 0; i < 32; i++)
		{
			TransferPictureInfo(mM31);
		}
	}

    StartCodecHevcThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run


// This is where we will start the video input thread
void NTV2EncodeHEVCVif::StartVideoInputThread (void)
{
    mACInputThread.Attach(VideoInputThreadStatic, this);
    mACInputThread.SetPriority(AJA_ThreadPriority_High);
    mACInputThread.Start();

}	// StartVideoInputThread


// The video input thread static callback
void NTV2EncodeHEVCVif::VideoInputThreadStatic (AJAThread * pThread, void * pContext)
{
	(void) pThread;

	NTV2EncodeHEVCVif *	pApp (reinterpret_cast <NTV2EncodeHEVCVif *> (pContext));
    pApp->VideoInputWorker ();

}	// VideoInputThreadStatic


void NTV2EncodeHEVCVif::VideoInputWorker (void)
{
	CNTV2Card device;
	CNTV2m31 *	m31;
    AUTOCIRCULATE_TRANSFER	inputXfer;

    //	Open the device...
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, device))
    { cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return; }

    // Allocate our M31 helper class and our HEVC common class
    m31 = new CNTV2m31 (&device);

	// start AutoCirculate running...
	device.AutoCirculateStart (mInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS	acStatus;
		device.AutoCirculateGetStatus (mInputChannel, acStatus);

        // wait for captured frame
		if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
		{
			// At this point, there's at least one fully-formed frame available in the device's
			// frame buffer to transfer to the host. Reserve an AvaDataBuffer to "produce", and
			// use it in the next transfer from the device...
            AVHevcDataBuffer *	pVideoData	(mACInputCircularBuffer.StartProduceNextBuffer ());
            if (pVideoData)
            {
                // setup buffer pointers for transfer
                inputXfer.SetBuffers (pVideoData->pVideoBuffer, pVideoData->videoBufferSize, AJA_NULL, 0, AJA_NULL, 0);

                if (mWithAudio)
                {
					inputXfer.SetAudioBuffer (pVideoData->pAudioBuffer, pVideoData->audioBufferSize);
                }

                // do the transfer from the device into our host AvaDataBuffer...
                device.AutoCirculateTransfer (mInputChannel, inputXfer);

                // get the video data size
                pVideoData->videoDataSize = pVideoData->videoBufferSize;
                pVideoData->audioDataSize = 0;
				pVideoData->frameTime = inputXfer.GetFrameInfo().acFrameTime;
                pVideoData->lastFrame = mLastFrame;

                if (mWithAudio)
                {
                    // get the audio data size
                    pVideoData->audioDataSize = inputXfer.GetCapturedAudioByteCount();
                }

                if (pVideoData->lastFrame && !mLastFrameInput)
                {
                    printf ( "\nCapture last frame number %d\n", mVideoInputFrameCount );
                    mLastFrameInput = true;
                }

                mVideoInputFrameCount++;

                // signal that we're done "producing" the frame, making it available for future "consumption"...
                mACInputCircularBuffer.EndProduceNextBuffer ();

				if ((mVideoInputFrameCount%60) == 0)
				{
					if (mOverlayIndex == 0)
					{
						mOverlayIndex = 1;
					}
					else
					{
						mOverlayIndex = 0;
					}
					device.SetOutputFrame (NTV2_CHANNEL5, mOverlayFrame[mOverlayIndex]/4);
				}
            }	// if A/C running and frame(s) are available for transfer
        }
		else
		{
			// Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			// Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
			// efficient to wait for the next input vertical interrupt event to get signaled...
            device.WaitForInputVerticalInterrupt (mInputChannel);
		}
	}	// loop til quit signaled

	// Stop AutoCirculate...
	device.AutoCirculateStop (mInputChannel);

    if (m31 != AJA_NULL)
	{
		delete m31;
	}
}	// VideoInputWorker


// This is where we will start the codec hevc thread
void NTV2EncodeHEVCVif::StartCodecHevcThread (void)
{
    mCodecHevcThread.Attach(CodecHevcThreadStatic, this);
    mCodecHevcThread.SetPriority(AJA_ThreadPriority_High);
    mCodecHevcThread.Start();

} // StartCodecHevcThread


// The codec hevc static callback
void NTV2EncodeHEVCVif::CodecHevcThreadStatic (AJAThread * pThread, void * pContext)
{
    (void) pThread;

    NTV2EncodeHEVCVif *	pApp (reinterpret_cast <NTV2EncodeHEVCVif *> (pContext));
    pApp->CodecHevcWorker ();

}	//	CodecHevcThreadStatic


void NTV2EncodeHEVCVif::CodecHevcWorker ()
{
	CNTV2Card device;
	CNTV2m31 *	m31;

    //	Open the device...
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, device))
    { cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return; }

    // Allocate our M31 helper class and our HEVC common class
    m31 = new CNTV2m31 (&device);

    while (!mGlobalQuit)
    {
        // wait for the next hevc frame 
        AVHevcDataBuffer *	pFrameData (mVideoHevcCircularBuffer.StartProduceNextBuffer ());
        if (pFrameData)
        {
			if (!mLastFrameHevc)
			{
				// transfer an hevc frame from the codec including encoded information
				m31->EncTransfer(mEncodeChannel,
								 (uint8_t*)pFrameData->pVideoBuffer,
								 pFrameData->videoBufferSize,
								 (uint8_t*)pFrameData->pInfoBuffer,
								 pFrameData->infoBufferSize,
								 pFrameData->videoDataSize,
								 pFrameData->infoDataSize,
								 pFrameData->frameTime,
								 pFrameData->lastFrame);

				// round the video size up
				pFrameData->videoDataSize = mHevcCommon->AlignDataBuffer(pFrameData->pVideoBuffer,
																		 pFrameData->videoBufferSize,
																		 pFrameData->videoDataSize,
																		 8, 0xff);
				// round the info size up
				pFrameData->infoDataSize = mHevcCommon->AlignDataBuffer(pFrameData->pInfoBuffer,
																		pFrameData->infoBufferSize,
																		pFrameData->infoDataSize,
																		8, 0);

                if (mWithInfo && !mLastFrame)
                {
					// transfer more picture info
					TransferPictureInfo(m31);
				}
			}

            if (pFrameData->lastFrame)
            {
                mLastFrameHevc = true;
            }
			mCodecHevcFrameCount++;

            // release and recycle the buffer...
            mVideoHevcCircularBuffer.EndProduceNextBuffer ();
        }
    }	//	loop til quit signaled

    if (m31 != AJA_NULL)
	{
		delete m31;
	}
}	//	EncTransferFrames


// This is where we start the audio/video file writer thread
void NTV2EncodeHEVCVif::StartAVFileThread (void)
{
    mAVFileThread.Attach(AVFileThreadStatic, this);
    mAVFileThread.SetPriority(AJA_ThreadPriority_High);
    mAVFileThread.Start();

} // StartAVFileThread


// The file writer static callback
void NTV2EncodeHEVCVif::AVFileThreadStatic (AJAThread * pThread, void * pContext)
{
    (void) pThread;

    NTV2EncodeHEVCVif *	pApp (reinterpret_cast <NTV2EncodeHEVCVif *> (pContext));
    pApp->AVFileWorker ();

} // AVFileStatic


void NTV2EncodeHEVCVif::AVFileWorker (void)
{
	int64_t	encodeTime;
	bool addData;

	mFrameData = AJA_NULL;

    while (!mGlobalQuit)
    {
		encodeTime = 0;

        // wait for the next codec hevc frame
        AVHevcDataBuffer *	pHevcData (mVideoHevcCircularBuffer.StartConsumeNextBuffer ());
        if (pHevcData)
        {
            if (!mLastFrameVideo)
			{
				// write the frame / fields hevc to the output file
                mHevcCommon->WriteHevcData(pHevcData->pVideoBuffer, pHevcData->videoDataSize);
				encodeTime = pHevcData->frameTime;

                if (mWithInfo)
                {
                    // write the frame encoded data to the output file
                    mHevcCommon->WriteEncData(pHevcData->pInfoBuffer, pHevcData->infoDataSize);
                }

                if (pHevcData->lastFrame)
				{
                    printf ( "Video file last frame number %d\n", mAVFileFrameCount );
                    mLastFrameVideo = true;
				}

                mAVFileFrameCount++;
            }

            // release the hevc buffer
            mVideoHevcCircularBuffer.EndConsumeNextBuffer ();
        }

		if (encodeTime != 0)
		{
			while (true)
			{
				addData = false;
				if (mFrameData == AJA_NULL)
				{
					mFrameData = mACInputCircularBuffer.StartConsumeNextBuffer ();
				}
				if (mFrameData)
				{
					if (abs(mFrameData->frameTime - encodeTime) < 50000)
					{
						if (mWithAudio)
						{
							// write the audio samples to the output file
							mHevcCommon->WriteAiffData(mFrameData->pAudioBuffer, mNumAudioChannels,
													   mFrameData->audioDataSize/mNumAudioChannels/4);
						}
						if (mRawFrameCount == 0)
						{
							mHevcCommon->WriteRawData(mFrameData->pVideoBuffer, mFrameData->videoDataSize);
							mRawFrameCount++;
						}

						// release the hevc buffer
//						printf ( "Found autocirculate raw audio/video frame %d\n", (int32_t)(mFrameData->frameTime - encodeTime));
						mACInputCircularBuffer.EndConsumeNextBuffer ();
						mFrameData = AJA_NULL;
						break;
					}
					else if (mFrameData->frameTime < encodeTime)
					{
						printf ( "Skip autocirculate raw audio/video frame - time diff %d us\n",
								 (int32_t)(mFrameData->frameTime - encodeTime)/10);
						mACInputCircularBuffer.EndConsumeNextBuffer ();
						mFrameData = AJA_NULL;
						continue;
					}
					else
					{
						printf ( "Add autocirculate raw audio/video frame - time diff %d us\n",
								 (int32_t)(mFrameData->frameTime - encodeTime)/10);
						addData = true;
					}
				}
				else
				{
					printf ( "Add autocirculate raw audio/video frame - no input\n");
					addData = true;
				}
				if (addData)
				{
					if (mWithAudio)
					{
						uint32_t numSamples = GetAudioSamplesPerFrame (mFrameRate, mAudioRate, mAVFileFrameCount);
						// write the audio samples to the output file
						mHevcCommon->WriteAiffData(mSilentBuffer, mNumAudioChannels, numSamples);
					}
					break;
				}
			}
		}
    } // loop til quit signaled

} // VideoFileWorker

//////////////////////////////////////////////


void NTV2EncodeHEVCVif::TransferPictureInfo(CNTV2m31 *	pM31)
{
	HevcPictureData picData;

	// initialize info buffer to 0
	memset(&picData, 0, sizeof(HevcPictureData));

	// calculate pts based on 90 Khz clock tick
	uint64_t pts = (uint64_t)mTimeBase.FramesToMicroseconds(mInfoFrameCount)*90000/1000000;

	// set serial number, pts and picture number
	picData.serialNumber = mInfoFrameCount;         		// can be anything
	picData.ptsValueLow = (uint32_t)(pts & 0xffffffff);
	picData.ptsValueHigh = (uint32_t)((pts >> 32) & 0x1);	// roll over at 33 bits
	picData.pictureNumber = mInfoFrameCount + 1;    		// must count starting with 1
	mInfoFrameCount++;

	// transfer only picture information 
	pM31->RawTransfer(mEncodeChannel,
					  AJA_NULL,
					  0,
					  (uint8_t*)&picData,
					  sizeof(HevcPictureData),
					  false);
}


void NTV2EncodeHEVCVif::GetStatus (AVHevcStatus * outInputStatus)
{
    AUTOCIRCULATE_STATUS	inputACStatus;
    
    mDevice.AutoCirculateGetStatus (mInputChannel, inputACStatus);
    outInputStatus->framesProcessed = inputACStatus.GetProcessedFrameCount();
    outInputStatus->framesDropped = inputACStatus.GetDroppedFrameCount();
    outInputStatus->bufferLevel = inputACStatus.GetBufferLevel();
    
}	//	GetStatus
