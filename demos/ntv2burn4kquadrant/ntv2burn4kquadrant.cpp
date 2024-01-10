/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn4kquadrant.cpp
	@brief		Implementation of NTV2Burn4KQuadrant demonstration class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2burn4kquadrant.h"
#include "ntv2devicescanner.h"
#include "ntv2formatdescriptor.h"
#include "ajabase/common/types.h"
#include "ajabase/system/process.h"
#include <iostream>

using namespace std;



NTV2Burn4KQuadrant::NTV2Burn4KQuadrant (const BurnConfig & inConfig)

	:	mConfig					(inConfig),
		mPlayThread				(AJAThread()),
		mCaptureThread			(AJAThread()),
		mInputAudioSystem		(NTV2_AUDIOSYSTEM_1),
		mOutputAudioSystem		(NTV2_AUDIOSYSTEM_1),
		mSingleDevice			(false),
		mGlobalQuit				(false)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	if (mConfig.fDeviceSpec == mConfig.fDeviceSpec2)
		mSingleDevice = true;

}	//	constructor


NTV2Burn4KQuadrant::~NTV2Burn4KQuadrant ()
{
	//	Stop my capture and playout threads, then destroy them...
	Quit ();

	//	Unsubscribe from input vertical event...
	mInputDevice.UnsubscribeInputVerticalEvent (mConfig.fInputChannel);
	mOutputDevice.UnsubscribeOutputVerticalEvent (mConfig.fOutputChannel);

	mInputDevice.SetEveryFrameServices (mInputSavedTaskMode);										//	Restore prior service level
	mInputDevice.ReleaseStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid()));	//	Release the device

	if (!mSingleDevice)
	{
		mOutputDevice.SetEveryFrameServices (mOutputSavedTaskMode);										//	Restore prior service level
		mOutputDevice.ReleaseStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid()));	//	Release the device
	}

}	//	destructor


void NTV2Burn4KQuadrant::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mPlayThread.Active())
		AJATime::Sleep(10);

	while (mCaptureThread.Active())
		AJATime::Sleep(10);

}	//	Quit


AJAStatus NTV2Burn4KQuadrant::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mInputDevice))
		{cerr << "## ERROR:  Input device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

	//	Store the input device ID in a member because it will be used frequently...
	mInputDeviceID = mInputDevice.GetDeviceID ();
	if (!mInputDevice.features().CanDo4KVideo())
		{cerr << "## ERROR:  Input device '" << mConfig.fDeviceSpec << "' cannot do 4K/UHD video" << endl;  return AJA_STATUS_UNSUPPORTED;}

    if (!mInputDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Input device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	//	Output device:
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec2, mOutputDevice))
		{cerr << "## ERROR:  Output device '" << mConfig.fDeviceSpec2 << "' not found" << endl;  return AJA_STATUS_OPEN;}

	//	Store the output device ID in a member because it will be used frequently...
	mOutputDeviceID = mOutputDevice.GetDeviceID ();
	if (!mOutputDevice.features().CanDo4KVideo())
		{cerr << "## ERROR:  Output device '" << mConfig.fDeviceSpec2 << "' cannot do 4K/UHD video" << endl;  return AJA_STATUS_UNSUPPORTED;}

    if (!mOutputDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Output device '" << mConfig.fDeviceSpec2 << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	if (mSingleDevice)
	{
		if (mInputDevice.features().GetNumFrameStores() < 8)
			{cerr << "## ERROR:  Single device '" << mConfig.fDeviceSpec2 << "' requires 8 video channels" << endl;  return AJA_STATUS_UNSUPPORTED;}
		mOutputAudioSystem = NTV2_AUDIOSYSTEM_5;
		mConfig.fOutputChannel = NTV2_CHANNEL5;
	}

	if (!mInputDevice.AcquireStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
		{cerr << "## ERROR:  Input device '" << mConfig.fDeviceSpec << "' is in use by another application" << endl;  return AJA_STATUS_BUSY;}
	mInputDevice.GetEveryFrameServices (mInputSavedTaskMode);	//	Save the current state before changing it
	mInputDevice.SetEveryFrameServices (NTV2_OEM_TASKS);		//	Since this is an OEM demo, use the OEM service level

	if (!mSingleDevice)
	{
		if (!mOutputDevice.AcquireStreamForApplication (kDemoAppSignature, static_cast <int32_t>(AJAProcess::GetPid())))
			{cerr << "## ERROR:  Output device '" << mConfig.fDeviceSpec2 << "' is in use by another application" << endl;  return AJA_STATUS_BUSY;}

		mOutputDevice.GetEveryFrameServices (mOutputSavedTaskMode);		//	Save the current state before changing it
		mOutputDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Since this is an OEM demo, use the OEM service level
	}

	if (mInputDevice.features().CanDoMultiFormat())
		mInputDevice.SetMultiFormatMode (false);
	if (mOutputDevice.features().CanDoMultiFormat())
		mOutputDevice.SetMultiFormatMode (false);

	//	Sometimes other applications disable some or all of the frame buffers, so turn them all on here...
	switch (mInputDevice.features().GetNumFrameStores())
	{
		case 8:	mInputDevice.EnableChannel (NTV2_CHANNEL8);
				mInputDevice.EnableChannel (NTV2_CHANNEL7);
				mInputDevice.EnableChannel (NTV2_CHANNEL6);
				mInputDevice.EnableChannel (NTV2_CHANNEL5);
				/* FALLTHRU */
		case 4:	mInputDevice.EnableChannel (NTV2_CHANNEL4);
				mInputDevice.EnableChannel (NTV2_CHANNEL3);
				/* FALLTHRU */
		case 2:	mInputDevice.EnableChannel (NTV2_CHANNEL2);
				/* FALLTHRU */
		case 1:	mInputDevice.EnableChannel (NTV2_CHANNEL1);
				break;
	}

	if (!mSingleDevice)		//	Don't do this twice if Input & Output devices are same device!
		switch (mOutputDevice.features().GetNumFrameStores())
		{
			case 8:	mOutputDevice.EnableChannel (NTV2_CHANNEL8);
					mOutputDevice.EnableChannel (NTV2_CHANNEL7);
					mOutputDevice.EnableChannel (NTV2_CHANNEL6);
					mOutputDevice.EnableChannel (NTV2_CHANNEL5);
					/* FALLTHRU */
			case 4:	mOutputDevice.EnableChannel (NTV2_CHANNEL4);
					mOutputDevice.EnableChannel (NTV2_CHANNEL3);
					/* FALLTHRU */
			case 2:	mOutputDevice.EnableChannel (NTV2_CHANNEL2);
					/* FALLTHRU */
			case 1:	mOutputDevice.EnableChannel (NTV2_CHANNEL1);
					break;
		}

	//	Set up the video and audio...
	status = SetupInputVideo();
	if (AJA_FAILURE (status))
		return status;

	status = SetupOutputVideo();
	if (AJA_FAILURE (status))
		return status;

	status = SetupInputAudio();
	if (AJA_FAILURE (status))
		return status;

	status = SetupOutputAudio();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the circular buffers...
	status = SetupHostBuffers();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the signal routing...
	RouteInputSignal();
	RouteOutputSignal();

	//	Lastly, prepare my AJATimeCodeBurn instance...
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mConfig.fPixelFormat),
																		mFormatDesc.numPixels,
																		mFormatDesc.numLines);
	//	Ready to go...
	#if defined(_DEBUG)
		cerr << mConfig;
		if (mInputDevice.IsRemote())
			cerr	<< "Input Device Desc:   " << mInputDevice.GetDescription() << endl;
		if (mOutputDevice.IsRemote())
			cerr	<< "Output Device Desc:  " << mOutputDevice.GetDescription() << endl;
		cerr << endl;
	#endif	//	not _DEBUG
	BURNINFO("Configuration: " << mConfig);
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Burn4KQuadrant::SetupInputVideo (void)
{
	//	Set the video format to match the incoming video format.
	//	Does the device support the desired input source?
	//	Since this is a 4k Quadrant example, look at one of the inputs and deduce the 4k geometry from the quadrant geometry...

	//	Determine the input video signal format, and set the device's reference source to that input.
	//	If you want to look at one of the quadrants, say on the HDMI output, then lock to one of the
	//	inputs (this assumes all quadrants are timed)...

	//	First, enable all of the necessary interrupts, and subscribe to the interrupts for the channel to be used...
	mInputDevice.EnableInputInterrupt (mConfig.fInputChannel);
	mInputDevice.SubscribeInputVerticalEvent (mConfig.fInputChannel);

	//	Turn multiformat off for this demo -- all multiformat devices will follow channel 1 configuration...

	//	For devices with bi-directional SDI connectors, their transmitter must be turned off before we can read a format...
	if (mInputDevice.features().HasBiDirectionalSDI())
	{
		mInputDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
		mInputDevice.SetSDITransmitEnable (NTV2_CHANNEL2, false);
		mInputDevice.SetSDITransmitEnable (NTV2_CHANNEL3, false);
		mInputDevice.SetSDITransmitEnable (NTV2_CHANNEL4, false);
		AJATime::Sleep(1000);
	}

	mVideoFormat = NTV2_FORMAT_UNKNOWN;
	mVideoFormat = mInputDevice.GetInputVideoFormat (NTV2_INPUTSOURCE_SDI1);
	mInputDevice.SetReference (NTV2_REFERENCE_INPUT1);

	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_NOINPUT;	//	Sorry, can't handle this format

	//	Set the device format to the input format detected...
	CNTV2DemoCommon::Get4KInputFormat (mVideoFormat);
	mInputDevice.SetVideoFormat (mVideoFormat);

	//	Set the frame buffer pixel format for all the channels on the device
	//	(assuming the device supports that pixel format -- otherwise default to 8-bit YCbCr)...
	if (!mInputDevice.features().CanDoFrameBufferFormat(mConfig.fPixelFormat))
		mConfig.fPixelFormat = NTV2_FBF_8BIT_YCBCR;

	//	...and set all buffers pixel format...
	mInputDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mConfig.fPixelFormat);
	mInputDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mConfig.fPixelFormat);
	mInputDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mConfig.fPixelFormat);
	mInputDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mConfig.fPixelFormat);

	mInputDevice.SetEnableVANCData (false, false);

	//	Now that the video is set up, get information about the current frame geometry...
	mFormatDesc = NTV2FormatDescriptor (mVideoFormat, mConfig.fPixelFormat);
	return AJA_STATUS_SUCCESS;

}	//	SetupInputVideo


AJAStatus NTV2Burn4KQuadrant::SetupOutputVideo (void)
{
	//	We turned off the transmit for the capture device, so now turn them on for the playback device...
	if (mOutputDevice.features().HasBiDirectionalSDI())
	{
		//	Devices having bidirectional SDI must be set to "transmit"...
		if (mSingleDevice)
		{
			mOutputDevice.SetSDITransmitEnable (NTV2_CHANNEL5, true);
			mOutputDevice.SetSDITransmitEnable (NTV2_CHANNEL6, true);
			mOutputDevice.SetSDITransmitEnable (NTV2_CHANNEL7, true);
			mOutputDevice.SetSDITransmitEnable (NTV2_CHANNEL8, true);
		}
		else
		{
			mOutputDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
			mOutputDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
			mOutputDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
			mOutputDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);
		}
	}

	//	Set the video format to match the incoming video format...
	mOutputDevice.SetVideoFormat (mVideoFormat);
	mOutputDevice.SetReference (NTV2_REFERENCE_FREERUN);

	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_NOINPUT;	//	Sorry, can't handle this format

	//	Set the frame buffer pixel format for all the channels on the device
	//	(assuming the device supports that pixel format -- otherwise default to 8-bit YCbCr)...
	if (!mOutputDevice.features().CanDoFrameBufferFormat(mConfig.fPixelFormat))
		mConfig.fPixelFormat = NTV2_FBF_8BIT_YCBCR;

	//	...and set the pixel format for all frame stores...
	if (mSingleDevice)
	{
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL5, mConfig.fPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL6, mConfig.fPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL7, mConfig.fPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL8, mConfig.fPixelFormat);
	}
	else
	{
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mConfig.fPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mConfig.fPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mConfig.fPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mConfig.fPixelFormat);
	}

	//	Set all frame buffers to playback
	if (mSingleDevice)
	{
		mOutputDevice.SetMode (NTV2_CHANNEL5, NTV2_MODE_DISPLAY);
		mOutputDevice.SetMode (NTV2_CHANNEL6, NTV2_MODE_DISPLAY);
		mOutputDevice.SetMode (NTV2_CHANNEL7, NTV2_MODE_DISPLAY);
		mOutputDevice.SetMode (NTV2_CHANNEL8, NTV2_MODE_DISPLAY);
	}
	else
	{
		mOutputDevice.SetMode (NTV2_CHANNEL1, NTV2_MODE_DISPLAY);
		mOutputDevice.SetMode (NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
		mOutputDevice.SetMode (NTV2_CHANNEL3, NTV2_MODE_DISPLAY);
		mOutputDevice.SetMode (NTV2_CHANNEL4, NTV2_MODE_DISPLAY);
	}

	//	Subscribe the output interrupt (it's enabled by default).
	//	If using a single-device, then subscribe the output channel to
	//	channel 5's interrupts, otherwise channel 1's...
	if (mSingleDevice)
	{
		mOutputDevice.EnableOutputInterrupt (NTV2_CHANNEL5);
		mOutputDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL5);
	}
	else
	{
		mOutputDevice.EnableOutputInterrupt(NTV2_CHANNEL1);
		mOutputDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL1);
	}

	mOutputDevice.SetEnableVANCData (false, false);

	return AJA_STATUS_SUCCESS;

}	//	SetupOutputVideo


AJAStatus NTV2Burn4KQuadrant::SetupInputAudio (void)
{
	//	We will be capturing and playing back with audio system 1.
	//	First, determine how many channels the device is capable of capturing or playing out...
	const uint16_t numberOfAudioChannels (mInputDevice.features().GetMaxAudioChannels());

	//	Have the input audio system grab audio from the designated input source...
	mInputDevice.SetAudioSystemInputSource (mInputAudioSystem, NTV2_AUDIO_EMBEDDED, NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1);

	mInputDevice.SetNumberAudioChannels (numberOfAudioChannels, mInputAudioSystem);
	mInputDevice.SetAudioRate (NTV2_AUDIO_48K, mInputAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mInputDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mInputAudioSystem);

	//	Loopback mode is used to play whatever audio appears in the input signal when
	//	it's connected directly to an output (i.e., "end-to-end" mode). If loopback is
	//	left enabled, the video will lag the audio as video frames get briefly delayed
	//	in our ring buffer. Audio, therefore, needs to come out of the (buffered) frame
	//	data being played, so loopback must be turned off...
	mInputDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mInputAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetupInputAudio


AJAStatus NTV2Burn4KQuadrant::SetupOutputAudio (void)
{
	//	Audio system 1 will be used to capture and playback audio.
	//	First, determine how many channels the device is capable of capturing or playing out...
	const uint16_t numberOfAudioChannels (mOutputDevice.features().GetMaxAudioChannels());

	mOutputDevice.SetNumberAudioChannels (numberOfAudioChannels, mOutputAudioSystem);
	mOutputDevice.SetAudioRate (NTV2_AUDIO_48K, mOutputAudioSystem);

	//	AJA recommends using a 4MB on-device audio buffer...
	mOutputDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mOutputAudioSystem);

	//	Finally, set up the output audio embedders...
	if (mSingleDevice)
	{
		mOutputDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL5, mOutputAudioSystem);
		mOutputDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL6, mOutputAudioSystem);
		mOutputDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL7, mOutputAudioSystem);
		mOutputDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL8, mOutputAudioSystem);
	}
	else
	{
		mOutputDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL1, mOutputAudioSystem);
		mOutputDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL2, mOutputAudioSystem);
		mOutputDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL3, mOutputAudioSystem);
		mOutputDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL4, mOutputAudioSystem);
	}

	//
	//	Loopback mode is used to play whatever audio appears in the input signal when
	//	it's connected directly to an output (i.e., "end-to-end" mode). If loopback is
	//	left enabled, the video will lag the audio as video frames get briefly delayed
	//	in our ring buffer. Audio, therefore, needs to come out of the (buffered) frame
	//	data being played, so loopback must be turned off...
	//
	mOutputDevice.SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, mOutputAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


AJAStatus NTV2Burn4KQuadrant::SetupHostBuffers (void)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Let my circular buffer know when it's time to quit...
	mFrameDataRing.SetAbortFlag (&mGlobalQuit);

	//  Determine video buffer size...
	const ULWord vidBuffSizeBytes (mFormatDesc.GetVideoWriteSize(ULWord(NTV2Buffer::DefaultPageSize())));

	//	Allocate and add each in-host NTV2FrameData to my mFrameDataRing...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());			//	Make a new NTV2FrameData...
		NTV2FrameData & frameData (mHostBuffers.back());	//	...and get a reference to it

		//	Allocate a page-aligned video buffer (if handling video)...
		if (!mConfig.fSuppressVideo)
			if (!frameData.fVideoBuffer.Allocate (vidBuffSizeBytes, /*pageAligned*/true))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(vidBuffSizeBytes,8) << "-byte video buffer");
				return AJA_STATUS_MEMORY;
			}

		//	Allocate a page-aligned audio buffer (if handling audio)...
		if (mConfig.WithAudio())
			if (!frameData.fAudioBuffer.Allocate (NTV2_AUDIOSIZE_MAX, /*pageAligned*/true))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(NTV2_AUDIOSIZE_MAX,8) << "-byte audio buffer");
				return AJA_STATUS_MEMORY;
			}
		if (frameData.AudioBuffer())
			frameData.fAudioBuffer.Fill(ULWord(0));

		//	Add this NTV2FrameData to the ring...
		mFrameDataRing.Add(&frameData);
	}	//	for each NTV2FrameData

	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2Burn4KQuadrant::RouteInputSignal (void)
{
	//	Since this is only a 4k example, we will route all inputs to framebuffers
	//	and color space convert when necessary...
	mInputDevice.ClearRouting ();
	if(IsRGBFormat(mConfig.fPixelFormat))
	{
		mInputDevice.Connect (NTV2_XptCSC1VidInput, NTV2_XptSDIIn1);
		mInputDevice.Connect (NTV2_XptCSC2VidInput, NTV2_XptSDIIn2);
		mInputDevice.Connect (NTV2_XptCSC3VidInput, NTV2_XptSDIIn3);
		mInputDevice.Connect (NTV2_XptCSC4VidInput, NTV2_XptSDIIn4);

		mInputDevice.Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCSC1VidRGB);
		mInputDevice.Connect (NTV2_XptFrameBuffer2Input, NTV2_XptCSC2VidRGB);
		mInputDevice.Connect (NTV2_XptFrameBuffer3Input, NTV2_XptCSC3VidRGB);
		mInputDevice.Connect (NTV2_XptFrameBuffer4Input, NTV2_XptCSC4VidRGB);
	}
	else
	{
		mInputDevice.Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
		mInputDevice.Connect (NTV2_XptFrameBuffer2Input, NTV2_XptSDIIn2);
		mInputDevice.Connect (NTV2_XptFrameBuffer3Input, NTV2_XptSDIIn3);
		mInputDevice.Connect (NTV2_XptFrameBuffer4Input, NTV2_XptSDIIn4);
	}

}	//	RouteInputSignal


void NTV2Burn4KQuadrant::RouteOutputSignal (void)
{
	//	Since this is only a 4k example, route all framebuffers to SDI outputs, and colorspace convert when necessary...
	if (!mSingleDevice)
		mOutputDevice.ClearRouting ();
	if (!mSingleDevice)
	{
		if (::IsRGBFormat (mConfig.fPixelFormat))
		{
			mOutputDevice.Connect (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1RGB);
			mOutputDevice.Connect (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2RGB);
			mOutputDevice.Connect (NTV2_XptCSC3VidInput, NTV2_XptFrameBuffer3RGB);
			mOutputDevice.Connect (NTV2_XptCSC4VidInput, NTV2_XptFrameBuffer4RGB);

			mOutputDevice.Connect (NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
			mOutputDevice.Connect (NTV2_XptSDIOut2Input, NTV2_XptCSC2VidYUV);
			mOutputDevice.Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC3VidYUV);
			mOutputDevice.Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC4VidYUV);
		}
		else
		{
			mOutputDevice.Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);
			mOutputDevice.Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameBuffer2YUV);
			mOutputDevice.Connect (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer3YUV);
			mOutputDevice.Connect (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer4YUV);
		}
	}
	else
	{
		if (::IsRGBFormat (mConfig.fPixelFormat))
		{
			mOutputDevice.Connect (NTV2_XptCSC5VidInput, NTV2_XptFrameBuffer5RGB);
			mOutputDevice.Connect (NTV2_XptCSC6VidInput, NTV2_XptFrameBuffer6RGB);
			mOutputDevice.Connect (NTV2_XptCSC7VidInput, NTV2_XptFrameBuffer7RGB);
			mOutputDevice.Connect (NTV2_XptCSC8VidInput, NTV2_XptFrameBuffer8RGB);

			mOutputDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV);
			mOutputDevice.Connect (NTV2_XptSDIOut6Input, NTV2_XptCSC6VidYUV);
			mOutputDevice.Connect (NTV2_XptSDIOut7Input, NTV2_XptCSC7VidYUV);
			mOutputDevice.Connect (NTV2_XptSDIOut8Input, NTV2_XptCSC8VidYUV);
		}
		else
		{
			mOutputDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptFrameBuffer5YUV);
			mOutputDevice.Connect (NTV2_XptSDIOut6Input, NTV2_XptFrameBuffer6YUV);
			mOutputDevice.Connect (NTV2_XptSDIOut7Input, NTV2_XptFrameBuffer7YUV);
			mOutputDevice.Connect (NTV2_XptSDIOut8Input, NTV2_XptFrameBuffer8YUV);
		}
	}

}	//	RouteOutputSignal


AJAStatus NTV2Burn4KQuadrant::Run ()
{
	//	Start the playout and capture threads...
	StartPlayThread ();
	StartCaptureThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we will start the play thread
void NTV2Burn4KQuadrant::StartPlayThread (void)
{
	//	Create and start the playout thread...
	mPlayThread.Attach(PlayThreadStatic, this);
	mPlayThread.SetPriority(AJA_ThreadPriority_High);
	mPlayThread.Start();

}	//	StartPlayThread


//	The playout thread function
void NTV2Burn4KQuadrant::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Burn4K instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2Burn4KQuadrant * pApp(reinterpret_cast<NTV2Burn4KQuadrant*>(pContext));
	pApp->PlayFrames();

}	//	PlayThreadStatic


void NTV2Burn4KQuadrant::PlayFrames (void)
{
	const ULWord			acOptions (AUTOCIRCULATE_WITH_RP188);
	ULWord					goodXfers(0), badXfers(0), starves(0), noRoomWaits(0);
	AUTOCIRCULATE_TRANSFER	outputXferInfo;
	AUTOCIRCULATE_STATUS	outputStatus;

	if (mSingleDevice)
	{
		mOutputDevice.AutoCirculateStop (NTV2_CHANNEL5);
		mOutputDevice.AutoCirculateStop (NTV2_CHANNEL6);
		mOutputDevice.AutoCirculateStop (NTV2_CHANNEL7);
		mOutputDevice.AutoCirculateStop (NTV2_CHANNEL8);
	}
	else
	{
		mOutputDevice.AutoCirculateStop (NTV2_CHANNEL1);
		mOutputDevice.AutoCirculateStop (NTV2_CHANNEL2);
		mOutputDevice.AutoCirculateStop (NTV2_CHANNEL3);
		mOutputDevice.AutoCirculateStop (NTV2_CHANNEL4);
	}
	mOutputDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel, 4);	//	Let it stop
	BURNNOTE("Thread started");

	//	Initialize AutoCirculate...
	if (!mOutputDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel, mConfig.fOutputFrames.count(), mOutputAudioSystem, acOptions,
													1 /*numChannels*/,  mConfig.fOutputFrames.firstFrame(),  mConfig.fOutputFrames.lastFrame()))
		{BURNFAIL("AutoCirculateInitForOutput failed");  mGlobalQuit = true;}

	while (!mGlobalQuit)
	{
		mOutputDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outputStatus);

		//	Check if there's room for another frame on the card...
		if (outputStatus.CanAcceptMoreOutputFrames())
		{
			//	Device has at least one free frame buffer that can be filled.
			//	Wait for the next frame in our ring to become ready to "consume"...
			NTV2FrameData *	pFrameData (mFrameDataRing.StartConsumeNextBuffer());
			if (!pFrameData)
				{starves++;  continue;}	//	Producer thread isn't producing frames fast enough

			//	Transfer the timecode-burned frame to the device for playout...
			outputXferInfo.SetVideoBuffer (pFrameData->VideoBuffer(), pFrameData->VideoBufferSize());
			if (pFrameData->AudioBuffer())
				outputXferInfo.SetAudioBuffer(pFrameData->AudioBuffer(), pFrameData->AudioBufferSize());
			outputXferInfo.SetOutputTimeCode(pFrameData->Timecode(NTV2_TCINDEX_SDI1), NTV2_TCINDEX_SDI1);
			outputXferInfo.SetOutputTimeCode(pFrameData->Timecode(NTV2_TCINDEX_SDI1_LTC), NTV2_TCINDEX_SDI1_LTC);

			//	Transfer the frame to the device for eventual playout...
			if (mOutputDevice.AutoCirculateTransfer (mConfig.fOutputChannel, outputXferInfo))
				goodXfers++;
			else
				badXfers++;

			if (goodXfers == 3)	//	Start AutoCirculate playout once 3 frames are buffered on the device...
				mOutputDevice.AutoCirculateStart(mConfig.fOutputChannel);

			//	Signal that the frame has been "consumed"...
			mFrameDataRing.EndConsumeNextBuffer();
			continue;	//	Back to top of while loop
		}	//	if CanAcceptMoreOutputFrames

		//	Wait for one or more buffers to become available on the device, which should occur at next VBI...
		noRoomWaits++;
		mOutputDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel);
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mOutputDevice.AutoCirculateStop(mConfig.fOutputChannel);
	BURNNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(starves) << " ring starves, " << DEC(noRoomWaits) << " device starves");
}	//	PlayFrames


//////////////////////////////////////////////



//////////////////////////////////////////////
//
//	This is where the capture thread gets started
//
void NTV2Burn4KQuadrant::StartCaptureThread (void)
{
	//	Create and start the capture thread...
	mCaptureThread.Attach(CaptureThreadStatic, this);
	mCaptureThread.SetPriority(AJA_ThreadPriority_High);
	mCaptureThread.Start();

}	//	StartCaptureThread


//
//	The static capture thread function
//
void NTV2Burn4KQuadrant::CaptureThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Burn4K instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2Burn4KQuadrant * pApp (reinterpret_cast<NTV2Burn4KQuadrant*>(pContext));
	pApp->CaptureFrames();
}	//	CaptureThreadStatic


//
//	Repeatedly captures frames until told to stop
//
void NTV2Burn4KQuadrant::CaptureFrames (void)
{
	AUTOCIRCULATE_TRANSFER	inputXferInfo;
	ULWord					goodXfers(0), badXfers(0), ringFulls(0), devWaits(0);
	BURNNOTE("Thread started");

	mInputDevice.AutoCirculateStop (NTV2_CHANNEL1);
	mInputDevice.AutoCirculateStop (NTV2_CHANNEL2);
	mInputDevice.AutoCirculateStop (NTV2_CHANNEL3);
	mInputDevice.AutoCirculateStop (NTV2_CHANNEL4);
	AJATime::Sleep (1000);

	//	Enable analog LTC input (some LTC inputs are shared with reference input)
	mInputDevice.SetLTCInputEnable (true);

	//	Set all sources to capture embedded LTC (Use 1 for VITC1)
	mInputDevice.SetRP188SourceFilter (NTV2_CHANNEL1, 0);
	mInputDevice.SetRP188SourceFilter (NTV2_CHANNEL2, 0);
	mInputDevice.SetRP188SourceFilter (NTV2_CHANNEL3, 0);
	mInputDevice.SetRP188SourceFilter (NTV2_CHANNEL4, 0);

	//	Initialize AutoCirculate...
	if (!mInputDevice.AutoCirculateInitForInput (mConfig.fInputChannel,		//	channel
											mConfig.fInputFrames.count(),	//	numFrames (zero if specifying range)
											mInputAudioSystem,				//	audio system
											AUTOCIRCULATE_WITH_RP188,		//	flags
											1,								//	frameStores to gang
											mConfig.fInputFrames.firstFrame(), mConfig.fInputFrames.lastFrame()))
		{BURNFAIL("AutoCirculateInitForInput failed");  mGlobalQuit = true;}
	else
		//	Start AutoCirculate running...
		mInputDevice.AutoCirculateStart (mConfig.fInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS acStatus;
		mInputDevice.AutoCirculateGetStatus (mConfig.fInputChannel, acStatus);

		if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
		{
			//	At this point, there's at least one fully-formed frame available in the device's frame buffer
			//	memory waiting to be transferred to the host. Reserve an NTV2FrameData to fill ("produce"),
			//	and use it in the next frame transferred from the device...
			NTV2FrameData *	pFrameData	(mFrameDataRing.StartProduceNextBuffer ());
			if (!pFrameData)
				{ringFulls++;  continue;}	//	Ring full -- consumer thread isn't consuming frames fast enough

			inputXferInfo.SetVideoBuffer(pFrameData->VideoBuffer(), pFrameData->VideoBufferSize());
			if (pFrameData->AudioBuffer())
				inputXferInfo.SetAudioBuffer(pFrameData->AudioBuffer(), pFrameData->AudioBufferSize());

			//	Transfer the frame from the device into our host buffers...
			if (mInputDevice.AutoCirculateTransfer (mConfig.fInputChannel, inputXferInfo)) goodXfers++;
			else badXfers++;

			//	Remember the amount, in bytes, of captured audio & anc data...
			pFrameData->fNumAudioBytes	= pFrameData->AudioBuffer()	? inputXferInfo.GetCapturedAudioByteCount()			: 0;

			//	Get a timecode to use for burn-in...
			NTV2_RP188	thisFrameTC;
			inputXferInfo.GetInputTimeCodes (pFrameData->fTimecodes, mConfig.fInputChannel);
			if (!pFrameData->HasValidTimecode(mConfig.fTimecodeSource))
			{	//	Invent a timecode (based on frame count)...
				const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
				const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
				const	CRP188			inventedTC		(inputXferInfo.GetTransferStatus().GetProcessedFrameCount(), 0, 0, 10, tcFormat);
				inventedTC.GetRP188Reg(thisFrameTC);
			}
			CRP188 tc(thisFrameTC);
			string tcStr;
			tc.GetRP188Str(tcStr);

			//	While this NTV2FrameData's buffers are locked, "burn" timecode into the raster...
			mTCBurner.BurnTimeCode (pFrameData->VideoBuffer(), tcStr.c_str(), 80);

			//	Signal that we're done "producing" this frame, making it available for future "consumption"...
			mFrameDataRing.EndProduceNextBuffer();
		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			devWaits++;
			mInputDevice.WaitForInputVerticalInterrupt (mConfig.fInputChannel);
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mInputDevice.AutoCirculateStop (mConfig.fInputChannel);
	BURNNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(ringFulls) << " ring full(s), " << DEC(devWaits) << " device waits");

}	//	CaptureFrames


//////////////////////////////////////////////


void NTV2Burn4KQuadrant::GetACStatus (AUTOCIRCULATE_STATUS & inputStatus, AUTOCIRCULATE_STATUS & outputStatus)
{
	mInputDevice.AutoCirculateGetStatus (mConfig.fInputChannel, inputStatus);
	mOutputDevice.AutoCirculateGetStatus(mConfig.fOutputChannel, outputStatus);

}	//	GetACStatus


ULWord NTV2Burn4KQuadrant::GetRP188RegisterForInput (const NTV2InputSource inInputSource)		//	static
{
	switch (inInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		return kRegRP188InOut1DBB;	//	reg 29
		case NTV2_INPUTSOURCE_SDI2:		return kRegRP188InOut2DBB;	//	reg 64
		case NTV2_INPUTSOURCE_SDI3:		return kRegRP188InOut3DBB;	//	reg 268
		case NTV2_INPUTSOURCE_SDI4:		return kRegRP188InOut4DBB;	//	reg 273
		default:						return 0;
	}	//	switch on input source

}	//	GetRP188RegisterForInput


bool NTV2Burn4KQuadrant::InputSignalHasTimecode (void)
{
	bool			result		(false);
	const ULWord	regNum		(GetRP188RegisterForInput (NTV2_INPUTSOURCE_SDI1));
	ULWord			regValue	(0);

	//
	//	Bit 16 of the RP188 DBB register will be set if there is timecode embedded in the input signal...
	//
	if (regNum  &&  mInputDevice.ReadRegister(regNum, regValue)  &&  regValue & BIT(16))
		result = true;
	return result;

}	//	InputSignalHasTimecode
