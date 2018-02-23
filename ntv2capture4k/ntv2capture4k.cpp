/**
	@file		ntv2capture4k.cpp
	@brief		Implementation of NTV2Capture class.
	@copyright	Copyright (C) 2012-2018 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2capture4k.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"


#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define NTV2_ANCSIZE_MAX	(0x2000)


static const ULWord	kAppSignature	AJA_FOURCC ('D','E','M','O');


NTV2Capture4K::NTV2Capture4K (const string					inDeviceSpecifier,
						  const bool					withAudio,
						  const NTV2Channel				channel,
						  const NTV2FrameBufferFormat	pixelFormat,
						  const bool					inLevelConversion,
						  const bool					inDoMultiFormat,
						  const bool					inWithAnc,
							  const bool				inDoTsiRouting)

	:	mConsumerThread		(NULL),
		mProducerThread		(NULL),
		mLock				(new AJALock (CNTV2DemoCommon::GetGlobalMutexName ())),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mDeviceSpecifier	(inDeviceSpecifier),
		mWithAudio			(withAudio),
		mInputChannel		(channel),
		mInputSource		(::NTV2ChannelToInputSource (mInputChannel)),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mPixelFormat		(pixelFormat),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mVancEnabled		(false),
		mWideVanc			(false),
		mAudioSystem		(NTV2_AUDIOSYSTEM_1),
		mDoLevelConversion	(inLevelConversion),
		mDoMultiFormat		(inDoMultiFormat),
		mGlobalQuit			(false),
		mWithAnc			(inWithAnc),
		mVideoBufferSize	(0),
		mAudioBufferSize	(0),
		mDoTsiRouting		(inDoTsiRouting)

{
	::memset (mAVHostBuffer, 0x0, sizeof (mAVHostBuffer));

}	//	constructor


NTV2Capture4K::~NTV2Capture4K ()
{
	//	Stop my capture and consumer threads, then destroy them...
	Quit ();

	delete mConsumerThread;
	mConsumerThread = NULL;

	delete mProducerThread;
	mProducerThread = NULL;

	//	Unsubscribe from input vertical event...
	mDevice.UnsubscribeInputVerticalEvent (mInputChannel);
	//	Unsubscribe from output vertical
	mDevice.UnsubscribeOutputVerticalEvent(NTV2_CHANNEL1);

	//	Free all my buffers...
	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++)
	{
		if (mAVHostBuffer[bufferNdx].fVideoBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fVideoBuffer;
			mAVHostBuffer[bufferNdx].fVideoBuffer = NULL;
		}
		if (mAVHostBuffer[bufferNdx].fAudioBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fAudioBuffer;
			mAVHostBuffer[bufferNdx].fAudioBuffer = NULL;
		}
		if (mAVHostBuffer[bufferNdx].fAncBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fAncBuffer;
			mAVHostBuffer[bufferNdx].fAncBuffer = NULL;
		}
	}	//	for each buffer in the ring

	if (!mDoMultiFormat)
	{
		mDevice.ReleaseStreamForApplication(kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid()));
		mDevice.SetEveryFrameServices(mSavedTaskMode);		//	Restore prior task mode
	}

}	//	destructor


void NTV2Capture4K::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	if (mConsumerThread)
		while (mConsumerThread->Active ())
			AJATime::Sleep (10);

	if (mProducerThread)
		while (mProducerThread->Active ())
			AJATime::Sleep (10);

}	//	Quit


AJAStatus NTV2Capture4K::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

	if (!mDevice.IsDeviceReady ())
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	if (!mDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ())))
			return AJA_STATUS_BUSY;							//	Another app is using the device
		mDevice.GetEveryFrameServices (&mSavedTaskMode);	//	Save the current state before we change it
	}
	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Since this is an OEM demo, use the OEM service level

	mDeviceID = mDevice.GetDeviceID ();						//	Keep the device ID handy, as it's used frequently

	//	Sometimes other applications disable some or all of the frame buffers, so turn them all on here...
	switch (::NTV2DeviceGetNumFrameStores(mDeviceID))
	{
	case 8:
		mDevice.EnableChannel(NTV2_CHANNEL8);
		mDevice.EnableChannel(NTV2_CHANNEL7);
		mDevice.EnableChannel(NTV2_CHANNEL6);
		mDevice.EnableChannel(NTV2_CHANNEL5);
	case 4:
		mDevice.EnableChannel(NTV2_CHANNEL4);
		mDevice.EnableChannel(NTV2_CHANNEL3);
		mDevice.EnableChannel(NTV2_CHANNEL2);
		mDevice.EnableChannel(NTV2_CHANNEL1);
		break;
	}

	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
		mDevice.SetMultiFormatMode (mDoMultiFormat);

	if (::NTV2DeviceGetNumHDMIVideoInputs (mDeviceID) > 1)
	{
		mInputChannel = NTV2_CHANNEL1;
		mInputSource = ::NTV2ChannelToInputSource (mInputChannel, NTV2_INPUTSOURCES_HDMI);
		mDoTsiRouting = true;
	}

	//	Set up the video and audio...
	status = SetupVideo ();
	if (AJA_FAILURE (status))
		return status;

	status = SetupAudio ();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the circular buffers, the device signal routing, and both playout and capture AutoCirculate...
	SetupHostBuffers ();
	RouteInputSignal ();
	SetupInputAutoCirculate ();

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Capture4K::SetupVideo (void)
{
	//	Enable and subscribe to the interrupts for the channel to be used...
	mDevice.EnableInputInterrupt(mInputChannel);
	mDevice.SubscribeInputVerticalEvent(mInputChannel);
	//	The input vertical is not always available so we like to use the output for timing - sometimes
	mDevice.SubscribeOutputVerticalEvent(mInputChannel);

	//	Disable SDI output from the SDI input being used,
	//	but only if the device supports bi-directional SDI,
	//	and only if the input being used is an SDI input...
	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
	{
		switch (::NTV2DeviceGetNumFrameStores(mDeviceID))
		{
		case 8:
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL8, false);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL7, false);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL6, false);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL5, false);
		case 4:
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, false);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, false);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, false);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, false);
			break;
		}
	}

	//	Wait for four verticals to let the reciever lock...
	mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1, 10);

	//	Set the video format to match the incomming video format.
	//	Does the device support the desired input source?
	//	Determine the input video signal format...
	mVideoFormat = mDevice.GetInputVideoFormat (mInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr << "## ERROR:  No input signal or unknown format" << endl;
		return AJA_STATUS_NOINPUT;	//	Sorry, can't handle this format
	}

	// Convert the signal wire format to a 4k format
	CNTV2DemoCommon::Get4KInputFormat(mVideoFormat);
	mDevice.SetVideoFormat(mVideoFormat, false, false, mInputChannel);

	if(mDoTsiRouting)
		mDevice.SetTsiFrameEnable(true, mInputChannel);
	else
		mDevice.Set4kSquaresEnable(true, mInputChannel);
	//	Set the device video format to whatever we detected at the input...
	//	The user has an option here. If doing multi-format, we are, lock to the board.
	//	If the user wants to E-E the signal then lock to input.
	mDevice.SetReference(NTV2_REFERENCE_FREERUN);

	//	Set the frame buffer pixel format for all the channels on the device
	//	(assuming it supports that pixel format -- otherwise default to 8-bit YCbCr)...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPixelFormat))
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;

	//	...and set all buffers pixel format...
	if (mInputChannel == NTV2_CHANNEL1)
	{
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL1, mPixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL2, mPixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL3, mPixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL4, mPixelFormat);
		mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL1);
		mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL2);
		mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL3);
		mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL4);
	}
	else
	{
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL5, mPixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL6, mPixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL7, mPixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL8, mPixelFormat);
		mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL5);
		mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL6);
		mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL7);
		mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL8);
	}

	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2Capture4K::SetupAudio (void)
{
	//	In multiformat mode, base the audio system on the channel...
	if (mDoMultiFormat && ::NTV2DeviceGetNumAudioSystems (mDeviceID) > 1 && UWord (mInputChannel) < ::NTV2DeviceGetNumAudioSystems (mDeviceID))
		mAudioSystem = ::NTV2ChannelToAudioSystem (mInputChannel);

	//	Have the audio system capture audio from the designated device input (i.e., ch1 uses SDIIn1, ch2 uses SDIIn2, etc.)...
	mDevice.SetAudioSystemInputSource (mAudioSystem, NTV2_AUDIO_EMBEDDED, ::NTV2ChannelToEmbeddedAudioInput (mInputChannel));

	mDevice.SetNumberAudioChannels (::NTV2DeviceGetMaxAudioChannels (mDeviceID), mAudioSystem);
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	The on-device audio buffer should be 4MB to work best across all devices & platforms...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	mDevice.SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


void NTV2Capture4K::SetupHostBuffers (void)
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	mVancEnabled = false;
	mWideVanc = false;
	mVideoBufferSize = ::GetVideoWriteSize (mVideoFormat, mPixelFormat, mVancEnabled, mWideVanc);
	printf("video size = %d\n", mVideoBufferSize);
	mAudioBufferSize = NTV2_AUDIOSIZE_MAX;
	mAncBufferSize = NTV2_ANCSIZE_MAX;

	//	Allocate and add each in-host AVDataBuffer to my circular buffer member variable...
	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++ )
	{
		mAVHostBuffer [bufferNdx].fVideoBuffer		= reinterpret_cast <uint32_t *> (new uint8_t [mVideoBufferSize]);
		mAVHostBuffer [bufferNdx].fVideoBufferSize	= mVideoBufferSize;
		mAVHostBuffer [bufferNdx].fAudioBuffer		= mWithAudio ? reinterpret_cast <uint32_t *> (new uint8_t [mAudioBufferSize]) : 0;
		mAVHostBuffer [bufferNdx].fAudioBufferSize	= mWithAudio ? mAudioBufferSize : 0;
		mAVHostBuffer [bufferNdx].fAncBuffer		= mWithAnc ? reinterpret_cast <uint32_t *> (new uint8_t [mAncBufferSize]) : 0;
		mAVHostBuffer [bufferNdx].fAncBufferSize	= mAncBufferSize;
		mAVCircularBuffer.Add (& mAVHostBuffer [bufferNdx]);
	}	//	for each AVDataBuffer

}	//	SetupHostBuffers


void NTV2Capture4K::RouteInputSignal(void)
{
	if (NTV2_INPUT_SOURCE_IS_HDMI (mInputSource))
	{
		NTV2LHIHDMIColorSpace	inputColor	(NTV2_LHIHDMIColorSpaceYCbCr);
		mDevice.GetHDMIInputColor (inputColor, mInputChannel);

		const bool						isInputRGB				(inputColor == NTV2_LHIHDMIColorSpaceRGB);
		const bool						isFrameRGB				(::IsRGBFormat (mPixelFormat));

		if (isInputRGB && isFrameRGB)
		{
			mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptHDMIIn1RGB);
			mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptHDMIIn1Q2RGB);
			mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptHDMIIn1Q3RGB);
			mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptHDMIIn1Q4RGB);

			mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1ARGB);
			mDevice.Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BRGB);
			mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2ARGB);
			mDevice.Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BRGB);
		}
		else if (isInputRGB && !isFrameRGB)
		{
			mDevice.Connect(NTV2_XptCSC1VidInput, NTV2_XptHDMIIn1RGB);
			mDevice.Connect(NTV2_XptCSC2VidInput, NTV2_XptHDMIIn1Q2RGB);
			mDevice.Connect(NTV2_XptCSC3VidInput, NTV2_XptHDMIIn1Q3RGB);
			mDevice.Connect(NTV2_XptCSC4VidInput, NTV2_XptHDMIIn1Q4RGB);

			mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidYUV);
			mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptCSC2VidYUV);
			mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptCSC3VidYUV);
			mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptCSC4VidYUV);

			mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1AYUV);
			mDevice.Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BYUV);
			mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV);
			mDevice.Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BYUV);
		}
		else if (!isInputRGB && isFrameRGB)
		{
			mDevice.Connect(NTV2_XptCSC1VidInput, NTV2_XptHDMIIn1);
			mDevice.Connect(NTV2_XptCSC2VidInput, NTV2_XptHDMIIn1Q2);
			mDevice.Connect(NTV2_XptCSC3VidInput, NTV2_XptHDMIIn1Q3);
			mDevice.Connect(NTV2_XptCSC4VidInput, NTV2_XptHDMIIn1Q4);

			mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidRGB);
			mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptCSC2VidRGB);
			mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptCSC3VidRGB);
			mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptCSC4VidRGB);

			mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1ARGB);
			mDevice.Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BRGB);
			mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2ARGB);
			mDevice.Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BRGB);
		}
		else
		{
			mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptHDMIIn1);
			mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptHDMIIn1Q2);
			mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptHDMIIn1Q3);
			mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptHDMIIn1Q4);

			mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1AYUV);
			mDevice.Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BYUV);
			mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV);
			mDevice.Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BYUV);
		}
	}
	else
	{
		if (mInputChannel == NTV2_CHANNEL1)
		{
			if (IsRGBFormat(mPixelFormat))
			{
				if(mDoTsiRouting)
				{
					mDevice.Connect(NTV2_XptCSC1VidInput, NTV2_XptSDIIn1);
					mDevice.Connect(NTV2_XptCSC2VidInput, NTV2_XptSDIIn2);
					mDevice.Connect(NTV2_XptCSC3VidInput, NTV2_XptSDIIn3);
					mDevice.Connect(NTV2_XptCSC4VidInput, NTV2_XptSDIIn4);
	
					mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidRGB);
					mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptCSC2VidRGB);
					mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptCSC3VidRGB);
					mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptCSC4VidRGB);
	
					mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1ARGB);
					mDevice.Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BRGB);
					mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2ARGB);
					mDevice.Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BRGB);
				}
				else
				{
					mDevice.Connect(NTV2_XptCSC1VidInput, NTV2_XptSDIIn1);
					mDevice.Connect(NTV2_XptCSC2VidInput, NTV2_XptSDIIn2);
					mDevice.Connect(NTV2_XptCSC3VidInput, NTV2_XptSDIIn3);
					mDevice.Connect(NTV2_XptCSC4VidInput, NTV2_XptSDIIn4);
	
					mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_XptCSC1VidRGB);
					mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_XptCSC2VidRGB);
					mDevice.Connect(NTV2_XptFrameBuffer3Input, NTV2_XptCSC3VidRGB);
					mDevice.Connect(NTV2_XptFrameBuffer4Input, NTV2_XptCSC4VidRGB);
				}
			}
			else
			{
				if(mDoTsiRouting)
				{
					mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptSDIIn1);
					mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptSDIIn2);
					mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptSDIIn3);
					mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptSDIIn4);
	
					mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1AYUV);
					mDevice.Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BYUV);
					mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV);
					mDevice.Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BYUV);
				}
				else
				{
					mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
					mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_XptSDIIn2);
					mDevice.Connect(NTV2_XptFrameBuffer3Input, NTV2_XptSDIIn3);
					mDevice.Connect(NTV2_XptFrameBuffer4Input, NTV2_XptSDIIn4);
				}
			}
		}
		else
		{
			if (IsRGBFormat(mPixelFormat))
			{
				if(mDoTsiRouting)
				{
					mDevice.Connect(NTV2_XptCSC5VidInput, NTV2_XptSDIIn5);
					mDevice.Connect(NTV2_XptCSC6VidInput, NTV2_XptSDIIn6);
					mDevice.Connect(NTV2_XptCSC7VidInput, NTV2_XptSDIIn7);
					mDevice.Connect(NTV2_XptCSC8VidInput, NTV2_XptSDIIn8);
	
					mDevice.Connect(NTV2_Xpt425Mux3AInput, NTV2_XptCSC5VidRGB);
					mDevice.Connect(NTV2_Xpt425Mux3BInput, NTV2_XptCSC6VidRGB);
					mDevice.Connect(NTV2_Xpt425Mux4AInput, NTV2_XptCSC7VidRGB);
					mDevice.Connect(NTV2_Xpt425Mux4BInput, NTV2_XptCSC8VidRGB);
	
					mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux3ARGB);
					mDevice.Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux3BRGB);
					mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux4ARGB);
					mDevice.Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux4BRGB);
				}
				else
				{
					mDevice.Connect(NTV2_XptCSC5VidInput, NTV2_XptSDIIn5);
					mDevice.Connect(NTV2_XptCSC6VidInput, NTV2_XptSDIIn6);
					mDevice.Connect(NTV2_XptCSC7VidInput, NTV2_XptSDIIn7);
					mDevice.Connect(NTV2_XptCSC8VidInput, NTV2_XptSDIIn8);
	
					mDevice.Connect(NTV2_XptFrameBuffer5Input, NTV2_XptCSC5VidRGB);
					mDevice.Connect(NTV2_XptFrameBuffer6Input, NTV2_XptCSC6VidRGB);
					mDevice.Connect(NTV2_XptFrameBuffer7Input, NTV2_XptCSC7VidRGB);
					mDevice.Connect(NTV2_XptFrameBuffer8Input, NTV2_XptCSC8VidRGB);
				}
			}
			else
			{
				if(mDoTsiRouting)
				{
					mDevice.Connect(NTV2_Xpt425Mux3AInput, NTV2_XptSDIIn5);
					mDevice.Connect(NTV2_Xpt425Mux3BInput, NTV2_XptSDIIn6);
					mDevice.Connect(NTV2_Xpt425Mux4AInput, NTV2_XptSDIIn7);
					mDevice.Connect(NTV2_Xpt425Mux4BInput, NTV2_XptSDIIn8);
	
					mDevice.Connect(NTV2_XptFrameBuffer5Input, NTV2_Xpt425Mux3AYUV);
					mDevice.Connect(NTV2_XptFrameBuffer5BInput, NTV2_Xpt425Mux3BYUV);
					mDevice.Connect(NTV2_XptFrameBuffer6Input, NTV2_Xpt425Mux4AYUV);
					mDevice.Connect(NTV2_XptFrameBuffer6BInput, NTV2_Xpt425Mux4BYUV);
				}
				else
				{
					mDevice.Connect(NTV2_XptFrameBuffer5Input, NTV2_XptSDIIn5);
					mDevice.Connect(NTV2_XptFrameBuffer6Input, NTV2_XptSDIIn6);
					mDevice.Connect(NTV2_XptFrameBuffer7Input, NTV2_XptSDIIn7);
					mDevice.Connect(NTV2_XptFrameBuffer8Input, NTV2_XptSDIIn8);
				}
			}
		}
	}
}	//	RouteInputSignal


void NTV2Capture4K::SetupInputAutoCirculate (void)
{
	//	Tell capture AutoCirculate to use 7 frame buffers on the device...
	ULWord startFrame = 0, endFrame = 7;
	if (mInputChannel == NTV2_CHANNEL1)
	{
		mDevice.AutoCirculateStop(NTV2_CHANNEL1);
		mDevice.AutoCirculateStop(NTV2_CHANNEL2);
		mDevice.AutoCirculateStop(NTV2_CHANNEL3);
		mDevice.AutoCirculateStop(NTV2_CHANNEL4);

	}
	else
	{
		mDevice.AutoCirculateStop(NTV2_CHANNEL5);
		mDevice.AutoCirculateStop(NTV2_CHANNEL6);
		mDevice.AutoCirculateStop(NTV2_CHANNEL7);
		mDevice.AutoCirculateStop(NTV2_CHANNEL8);
		startFrame = 8;
		endFrame = 15;
	}
	mDevice.AutoCirculateInitForInput (mInputChannel,	7,	//	Number of frames to circulate
											mWithAudio ? mAudioSystem : NTV2_AUDIOSYSTEM_INVALID,	//	Which audio system (if any)?
											AUTOCIRCULATE_WITH_RP188,
											1, startFrame, endFrame);								//	Include timecode
}	//	SetupInputAutoCirculate


AJAStatus NTV2Capture4K::Run ()
{
	//	Start the playout and capture threads...
	StartConsumerThread ();
	StartProducerThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we will start the consumer thread
void NTV2Capture4K::StartConsumerThread (void)
{
	//	Create and start the consumer thread...
	mConsumerThread = new AJAThread ();
	mConsumerThread->Attach (ConsumerThreadStatic, this);
	mConsumerThread->SetPriority (AJA_ThreadPriority_High);
	mConsumerThread->Start ();

}	//	StartConsumerThread


//	The consumer thread function
void NTV2Capture4K::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Capture instance pointer from the pContext parameter,
	//	then call its ConsumeFrames method...
	NTV2Capture4K *	pApp	(reinterpret_cast <NTV2Capture4K *> (pContext));
	pApp->ConsumeFrames ();

}	//	ConsumerThreadStatic


void NTV2Capture4K::ConsumeFrames (void)
{
	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		AVDataBuffer *	pFrameData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (pFrameData)
		{
			//	Do something useful with the frame data...
			//	. . .		. . .		. . .		. . .
			//		. . .		. . .		. . .		. . .
			//			. . .		. . .		. . .		. . .

			//	Now release and recycle the buffer...
			mAVCircularBuffer.EndConsumeNextBuffer ();
		}
	}	//	loop til quit signaled

}	//	ConsumeFrames


//////////////////////////////////////////////



//////////////////////////////////////////////

//	This is where we start the capture thread
void NTV2Capture4K::StartProducerThread (void)
{
	//	Create and start the capture thread...
	mProducerThread = new AJAThread ();
	mProducerThread->Attach (ProducerThreadStatic, this);
	mProducerThread->SetPriority (AJA_ThreadPriority_High);
	mProducerThread->Start ();

}	//	StartProducerThread


//	The capture thread function
void NTV2Capture4K::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Capture instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2Capture4K *	pApp	(reinterpret_cast <NTV2Capture4K *> (pContext));
	pApp->CaptureFrames ();

}	//	ProducerThreadStatic


void NTV2Capture4K::CaptureFrames (void)
{
	NTV2AudioChannelPairs	nonPcmPairs, oldNonPcmPairs;

	//	Start AutoCirculate running...
	mDevice.AutoCirculateStart (mInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS	acStatus;
		mDevice.AutoCirculateGetStatus (mInputChannel, acStatus);

		if (acStatus.IsRunning () && acStatus.HasAvailableInputFrame ())
		{
			//	At this point, there's at least one fully-formed frame available in the device's
			//	frame buffer to transfer to the host. Reserve an AVDataBuffer to "produce", and
			//	use it in the next transfer from the device...
			AVDataBuffer *	captureData	(mAVCircularBuffer.StartProduceNextBuffer ());

			mInputTransfer.SetBuffers (captureData->fVideoBuffer, captureData->fVideoBufferSize,
										captureData->fAudioBuffer, captureData->fAudioBufferSize,
										captureData->fAncBuffer, captureData->fAncBufferSize);

			//	Do the transfer from the device into our host AVDataBuffer...
			mDevice.AutoCirculateTransfer (mInputChannel, mInputTransfer);
			captureData->fAudioBufferSize = mInputTransfer.acTransferStatus.acAudioTransferSize;

			NTV2SDIInStatistics	sdiStats;
			mDevice.ReadSDIStatistics (sdiStats);

			//	"Capture" timecode into the host AVDataBuffer while we have full access to it...
			NTV2_RP188	timecode;
			mInputTransfer.GetInputTimeCode (timecode);
			captureData->fRP188Data = timecode;

			//	Signal that we're done "producing" the frame, making it available for future "consumption"...
			mAVCircularBuffer.EndProduceNextBuffer ();
		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			mDevice.WaitForInputVerticalInterrupt (mInputChannel);
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop (mInputChannel);

}	//	CaptureFrames


//////////////////////////////////////////////


void NTV2Capture4K::GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel)
{
	AUTOCIRCULATE_STATUS	status;
	mDevice.AutoCirculateGetStatus (mInputChannel, status);
	outGoodFrames = status.acFramesProcessed;
	outDroppedFrames = status.acFramesDropped;
	outBufferLevel = status.acBufferLevel;
}
