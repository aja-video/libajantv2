/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn4kquadrant.cpp
	@brief		Implementation of NTV2Burn4KQuadrant demonstration class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2burn4kquadrant.h"
#include "ntv2democommon.h"
#include "ntv2formatdescriptor.h"
#include "ajabase/common/types.h"
#include <iostream>

using namespace std;


#define NTV2_AUDIOSIZE_MAX	(401 * 1024)


static const ULWord	gAppSignature	(NTV2_FOURCC('D','E','M','O'));




NTV2Burn4KQuadrant::NTV2Burn4KQuadrant (const string &				inInputDeviceSpecifier,
										const string &				inOutputDeviceSpecifier,
										const bool					inWithAudio,
										const NTV2FrameBufferFormat	inPixelFormat,
										const NTV2TCIndex			inTCIndex)

	:	mPlayThread				(AJAThread()),
		mCaptureThread			(AJAThread()),
		mSingleDevice			(false),
		mInputDeviceSpecifier	(inInputDeviceSpecifier),
		mOutputDeviceSpecifier	(inOutputDeviceSpecifier),
		mWithAudio				(inWithAudio),
		mInputChannel			(NTV2_CHANNEL1),
		mOutputChannel			(NTV2_CHANNEL1),
		mTimecodeIndex			(inTCIndex),
		mPixelFormat			(inPixelFormat),
		mInputAudioSystem		(NTV2_AUDIOSYSTEM_1),
		mOutputAudioSystem		(NTV2_AUDIOSYSTEM_1),
		mGlobalQuit				(false)
{
	::memset (mAVHostBuffer, 0x0, sizeof (mAVHostBuffer));

	if (mInputDeviceSpecifier.compare (mOutputDeviceSpecifier) == 0)
		mSingleDevice = true;

}	//	constructor


NTV2Burn4KQuadrant::~NTV2Burn4KQuadrant ()
{
	//	Stop my capture and playout threads, then destroy them...
	Quit ();

	//	Unsubscribe from input vertical event...
	mInputDevice.UnsubscribeInputVerticalEvent (mInputChannel);
	mOutputDevice.UnsubscribeOutputVerticalEvent (mOutputChannel);

	//	Free all my buffers...
	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++)
	{
		if (mAVHostBuffer[bufferNdx].fVideoBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fVideoBuffer;
			mAVHostBuffer[bufferNdx].fVideoBuffer = AJA_NULL;
		}
		if (mAVHostBuffer[bufferNdx].fAudioBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fAudioBuffer;
			mAVHostBuffer[bufferNdx].fAudioBuffer = AJA_NULL;
		}
	}	//	for each buffer in the ring

	mInputDevice.SetEveryFrameServices (mInputSavedTaskMode);										//	Restore prior service level
	mInputDevice.ReleaseStreamForApplication (gAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));	//	Release the device

	if (!mSingleDevice)
	{
		mOutputDevice.SetEveryFrameServices (mOutputSavedTaskMode);										//	Restore prior service level
		mOutputDevice.ReleaseStreamForApplication (gAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));	//	Release the device
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
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mInputDeviceSpecifier, mInputDevice))
		{cerr << "## ERROR:  Input device '" << mInputDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

	//	Store the input device ID in a member because it will be used frequently...
	mInputDeviceID = mInputDevice.GetDeviceID ();
	if (!::NTV2DeviceCanDo4KVideo (mInputDeviceID))
		{cerr << "## ERROR:  Input device '" << mInputDeviceSpecifier << "' cannot do 4K/UHD video" << endl;  return AJA_STATUS_UNSUPPORTED;}

    if (!mInputDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Input device '" << mInputDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	//	Output device:
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mOutputDeviceSpecifier, mOutputDevice))
		{cerr << "## ERROR:  Output device '" << mOutputDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

	//	Store the output device ID in a member because it will be used frequently...
	mOutputDeviceID = mOutputDevice.GetDeviceID ();
	if (!::NTV2DeviceCanDo4KVideo (mOutputDeviceID))
		{cerr << "## ERROR:  Output device '" << mOutputDeviceSpecifier << "' cannot do 4K/UHD video" << endl;  return AJA_STATUS_UNSUPPORTED;}

    if (!mOutputDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Output device '" << mOutputDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	if (mSingleDevice)
	{
		if (::NTV2DeviceGetNumFrameStores (mInputDeviceID) < 8)
			{cerr << "## ERROR:  Single device '" << mOutputDeviceSpecifier << "' requires 8 video channels" << endl;  return AJA_STATUS_UNSUPPORTED;}
		mOutputAudioSystem = NTV2_AUDIOSYSTEM_5;
		mOutputChannel = NTV2_CHANNEL5;
	}

	if (!mInputDevice.AcquireStreamForApplication (gAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
		{cerr << "## ERROR:  Input device '" << mInputDeviceSpecifier << "' is in use by another application" << endl;  return AJA_STATUS_BUSY;}
	mInputDevice.GetEveryFrameServices (mInputSavedTaskMode);	//	Save the current state before changing it
	mInputDevice.SetEveryFrameServices (NTV2_OEM_TASKS);		//	Since this is an OEM demo, use the OEM service level

	if (!mSingleDevice)
	{
		if (!mOutputDevice.AcquireStreamForApplication (gAppSignature, static_cast <int32_t>(AJAProcess::GetPid())))
			{cerr << "## ERROR:  Output device '" << mOutputDeviceSpecifier << "' is in use by another application" << endl;  return AJA_STATUS_BUSY;}

		mOutputDevice.GetEveryFrameServices (mOutputSavedTaskMode);		//	Save the current state before changing it
		mOutputDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Since this is an OEM demo, use the OEM service level
	}

	if (::NTV2DeviceCanDoMultiFormat (mInputDeviceID))
		mInputDevice.SetMultiFormatMode (false);
	if (::NTV2DeviceCanDoMultiFormat (mOutputDeviceID))
		mOutputDevice.SetMultiFormatMode (false);

	//	Sometimes other applications disable some or all of the frame buffers, so turn them all on here...
	switch (::NTV2DeviceGetNumFrameStores (mInputDeviceID))
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
		switch (::NTV2DeviceGetNumFrameStores (mOutputDeviceID))
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
	status = SetupInputVideo ();
	if (AJA_FAILURE (status))
		return status;

	status = SetupOutputVideo ();
	if (AJA_FAILURE (status))
		return status;

	status = SetupInputAudio ();
	if (AJA_FAILURE (status))
		return status;

	status = SetupOutputAudio ();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the circular buffers, the device signal routing, and both playout and capture AutoCirculate...
	SetupHostBuffers ();
	RouteInputSignal ();
	RouteOutputSignal ();

	//	Lastly, prepare my timecode burner instance...
	NTV2FormatDescriptor	fd	(mVideoFormat, mPixelFormat, mVancMode);
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mPixelFormat), fd.numPixels, fd.numLines);

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
	mInputDevice.EnableInputInterrupt (mInputChannel);
	mInputDevice.SubscribeInputVerticalEvent (mInputChannel);

	//	Turn multiformat off for this demo -- all multiformat devices will follow channel 1 configuration...

	//	For devices with bi-directional SDI connectors, their transmitter must be turned off before we can read a format...
	if (::NTV2DeviceHasBiDirectionalSDI (mInputDeviceID))
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
	if (!::NTV2DeviceCanDoFrameBufferFormat (mInputDeviceID, mPixelFormat))
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;

	//	...and set all buffers pixel format...
	mInputDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mPixelFormat);
	mInputDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mPixelFormat);
	mInputDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mPixelFormat);
	mInputDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mPixelFormat);

	mInputDevice.SetEnableVANCData (false, false);

	return AJA_STATUS_SUCCESS;

}	//	SetupInputVideo


AJAStatus NTV2Burn4KQuadrant::SetupOutputVideo (void)
{
	//	We turned off the transmit for the capture device, so now turn them on for the playback device...
	if (::NTV2DeviceHasBiDirectionalSDI (mOutputDeviceID))
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
	if (!::NTV2DeviceCanDoFrameBufferFormat (mOutputDeviceID, mPixelFormat))
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;

	//	...and set the pixel format for all frame stores...
	if (mSingleDevice)
	{
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL5, mPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL6, mPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL7, mPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL8, mPixelFormat);
	}
	else
	{
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mPixelFormat);
		mOutputDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mPixelFormat);
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
	const uint16_t	numberOfAudioChannels	(::NTV2DeviceGetMaxAudioChannels (mInputDeviceID));

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
	const uint16_t	numberOfAudioChannels	(::NTV2DeviceGetMaxAudioChannels (mOutputDeviceID));

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


void NTV2Burn4KQuadrant::SetupHostBuffers (void)
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	mInputDevice.GetVANCMode (mVancMode);
	mVideoBufferSize = GetVideoWriteSize (mVideoFormat, mPixelFormat, mVancMode);
	mAudioBufferSize = NTV2_AUDIOSIZE_MAX;

	//	Allocate and add each in-host AVDataBuffer to my circular buffer member variable...
	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++ )
	{
		mAVHostBuffer [bufferNdx].fVideoBuffer = reinterpret_cast <uint32_t *> (new uint8_t [mVideoBufferSize]);
		mAVHostBuffer [bufferNdx].fVideoBufferSize = mVideoBufferSize;
		mAVHostBuffer [bufferNdx].fAudioBuffer = mWithAudio ? reinterpret_cast <uint32_t *> (new uint8_t [mAudioBufferSize]) : AJA_NULL;
		mAVHostBuffer [bufferNdx].fAudioBufferSize = mWithAudio ? mAudioBufferSize : 0;
		mAVCircularBuffer.Add (& mAVHostBuffer [bufferNdx]);
	}	//	for each AVDataBuffer

}	//	SetupHostBuffers


void NTV2Burn4KQuadrant::RouteInputSignal (void)
{
	//	Since this is only a 4k example, we will route all inputs to framebuffers
	//	and color space convert when necessary...
	mInputDevice.ClearRouting ();
	if(IsRGBFormat(mPixelFormat))
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
		if (::IsRGBFormat (mPixelFormat))
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
		if (::IsRGBFormat (mPixelFormat))
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
	AUTOCIRCULATE_TRANSFER	outputXferInfo;
	BURNNOTE("Thread started");

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
	AJATime::Sleep (1000);
	mOutputDevice.AutoCirculateInitForOutput (mOutputChannel, 5, mOutputAudioSystem, AUTOCIRCULATE_WITH_RP188);

	//	Start AutoCirculate running...
	mOutputDevice.AutoCirculateStart (mOutputChannel);

	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		AVDataBuffer *	playData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (playData)
		{
			//	Transfer the timecode-burned frame to the device for playout...
			outputXferInfo.SetVideoBuffer(playData->fVideoBuffer, playData->fVideoBufferSize);
			outputXferInfo.SetAudioBuffer(playData->fAudioBuffer, playData->fAudioBufferSize);
			outputXferInfo.SetOutputTimeCode(NTV2_RP188(playData->fRP188Data), NTV2_TCINDEX_SDI1);
			outputXferInfo.SetOutputTimeCode(NTV2_RP188(playData->fRP188Data), NTV2_TCINDEX_SDI1_LTC);
			outputXferInfo.acRP188 = playData->fRP188Data;

			// Output the time code in the autocirculate struct
			mOutputDevice.AutoCirculateTransfer (mOutputChannel, outputXferInfo);

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer ();
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mOutputDevice.AutoCirculateStop (mOutputChannel);
	BURNNOTE("Thread completed, will exit");

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
	BURNNOTE("Thread started");

	mInputDevice.AutoCirculateStop (NTV2_CHANNEL1);
	mInputDevice.AutoCirculateStop (NTV2_CHANNEL2);
	mInputDevice.AutoCirculateStop (NTV2_CHANNEL3);
	mInputDevice.AutoCirculateStop (NTV2_CHANNEL4);
	AJATime::Sleep (1000);
	mInputDevice.AutoCirculateInitForInput (mInputChannel, 5, mInputAudioSystem, AUTOCIRCULATE_WITH_RP188);

	//	Enable analog LTC input (some LTC inputs are shared with reference input)
	mInputDevice.SetLTCInputEnable (true);

	//	Set all sources to capture embedded LTC (Use 1 for VITC1)
	mInputDevice.SetRP188SourceFilter (NTV2_CHANNEL1, 0);
	mInputDevice.SetRP188SourceFilter (NTV2_CHANNEL2, 0);
	mInputDevice.SetRP188SourceFilter (NTV2_CHANNEL3, 0);
	mInputDevice.SetRP188SourceFilter (NTV2_CHANNEL4, 0);

	//	Start AutoCirculate running...
	mInputDevice.AutoCirculateStart (mInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS	acStatus;
		mInputDevice.AutoCirculateGetStatus (mInputChannel, acStatus);

		if (acStatus.acState == NTV2_AUTOCIRCULATE_RUNNING && acStatus.acBufferLevel > 1)
		{
			string	timeCodeString;

			//	At this point, there's at least one fully-formed frame available in the device's
			//	frame buffer to transfer to the host. Reserve an AVDataBuffer to "produce", and
			//	use it in the next transfer from the device...
			AVDataBuffer *	captureData	(mAVCircularBuffer.StartProduceNextBuffer ());

			inputXferInfo.SetVideoBuffer(captureData->fVideoBuffer, captureData->fVideoBufferSize);
			inputXferInfo.SetAudioBuffer(captureData->fAudioBuffer, captureData->fAudioBufferSize);

			//	Do the transfer from the device into our host AVDataBuffer...
			mInputDevice.AutoCirculateTransfer (mInputChannel, inputXferInfo);

			//	Remember the audio & anc data byte counts...
			NTV2_RP188	defaultTimeCode;
			inputXferInfo.acTransferStatus.acFrameStamp.GetInputTimeCode (defaultTimeCode, NTV2_TCINDEX_SDI1);
			captureData->fAudioBufferSize = inputXferInfo.GetCapturedAudioByteCount ();
			captureData->fRP188Data = defaultTimeCode;
			
			if (InputSignalHasTimecode ())
			{
				CRP188	inputRP188Info	(captureData->fRP188Data);
				inputRP188Info.GetRP188Str (timeCodeString);
			}
			else
			{
				//	Invent a timecode (based on frame count)...
				const	NTV2FrameRate	ntv2FrameRate	(GetNTV2FrameRateFromVideoFormat (mVideoFormat));
				const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
				const	CRP188			frameRP188Info	(inputXferInfo.acTransferStatus.acFramesProcessed, 0, 0, 10, tcFormat);

				frameRP188Info.GetRP188Reg (captureData->fRP188Data);
				frameRP188Info.GetRP188Str (timeCodeString);
			}

			//	"Burn" the timecode into the host AVDataBuffer while we have full access to it...
			mTCBurner.BurnTimeCode(reinterpret_cast <char *> (inputXferInfo.acVideoBuffer.GetHostPointer()), timeCodeString.c_str(), 80);

			//	Signal that we're done "producing" the frame, making it available for future "consumption"...
			mAVCircularBuffer.EndProduceNextBuffer ();

		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			mInputDevice.WaitForInputVerticalInterrupt (mInputChannel);
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mInputDevice.AutoCirculateStop (mInputChannel);
	BURNNOTE("Thread completed, will exit");

}	//	CaptureFrames


//////////////////////////////////////////////


void NTV2Burn4KQuadrant::GetACStatus (AUTOCIRCULATE_STATUS & inputStatus, AUTOCIRCULATE_STATUS & outputStatus)
{
	mInputDevice.AutoCirculateGetStatus (mInputChannel, inputStatus);
	mOutputDevice.AutoCirculateGetStatus(mOutputChannel, outputStatus);

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
