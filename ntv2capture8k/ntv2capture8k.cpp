/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2capture8k.cpp
	@brief		Implementation of NTV2Capture class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2capture8k.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/memory.h"

using namespace std;

/**
	@brief	The alignment of the video and audio buffers has a big impact on the efficiency of
			DMA transfers. When aligned to the page size of the architecture, only one DMA
			descriptor is needed per page. Misalignment will double the number of descriptors
			that need to be fetched and processed, thus reducing bandwidth.
**/
static const uint32_t	BUFFER_ALIGNMENT	(4096);		// The correct size for many systems


NTV2Capture8K::NTV2Capture8K (const string					inDeviceSpecifier,
							  const bool					withAudio,
							  const NTV2Channel				channel,
							  const NTV2FrameBufferFormat	pixelFormat,
							  const bool					inLevelConversion,
							  const bool					inDoMultiFormat,
                              const bool					inWithAnc,
                              const bool                    inDoTsiRouting)

	:	mConsumerThread		(AJAThread()),
		mProducerThread		(AJAThread()),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mDeviceSpecifier	(inDeviceSpecifier),
		mWithAudio			(withAudio),
		mInputChannel		(channel),
		mInputSource		(::NTV2ChannelToInputSource (mInputChannel)),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mPixelFormat		(pixelFormat),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
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


NTV2Capture8K::~NTV2Capture8K ()
{
	//	Stop my capture and consumer threads, then destroy them...
	Quit ();

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
			mAVHostBuffer[bufferNdx].fVideoBuffer = AJA_NULL;
		}
		if (mAVHostBuffer[bufferNdx].fAudioBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fAudioBuffer;
			mAVHostBuffer[bufferNdx].fAudioBuffer = AJA_NULL;
		}
		if (mAVHostBuffer[bufferNdx].fAncBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fAncBuffer;
			mAVHostBuffer[bufferNdx].fAncBuffer = AJA_NULL;
		}
	}	//	for each buffer in the ring

	if (!mDoMultiFormat)
	{
		mDevice.ReleaseStreamForApplication(kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));
		mDevice.SetEveryFrameServices(mSavedTaskMode);		//	Restore prior task mode
	}

}	//	destructor


void NTV2Capture8K::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mConsumerThread.Active())
		AJATime::Sleep(10);

	while (mProducerThread.Active())
		AJATime::Sleep(10);

	mDevice.DMABufferUnlockAll();
}	//	Quit


AJAStatus NTV2Capture8K::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

	if (!mDevice.IsDeviceReady ())
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	if (!mDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
			return AJA_STATUS_BUSY;							//	Another app is using the device
		mDevice.GetEveryFrameServices (mSavedTaskMode);		//	Save the current state before we change it
	}
	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Since this is an OEM demo, use the OEM service level

	mDeviceID = mDevice.GetDeviceID ();						//	Keep the device ID handy, as it's used frequently

	//	Sometimes other applications disable some or all of the frame buffers, so turn them all on here...
	mDevice.EnableChannel(NTV2_CHANNEL4);
	mDevice.EnableChannel(NTV2_CHANNEL3);
	mDevice.EnableChannel(NTV2_CHANNEL2);
	mDevice.EnableChannel(NTV2_CHANNEL1);

	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
    {
		mDevice.SetMultiFormatMode (mDoMultiFormat);
    }
    else
    {
        mDoMultiFormat = false;
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


AJAStatus NTV2Capture8K::SetupVideo (void)
{
	//	Enable and subscribe to the interrupts for the channel to be used...
	mDevice.EnableInputInterrupt(mInputChannel);
	mDevice.SubscribeInputVerticalEvent(mInputChannel);
	//	The input vertical is not always available so we like to use the output for timing - sometimes
	mDevice.SubscribeOutputVerticalEvent(mInputChannel);

    // disable SDI transmitter
    mDevice.SetSDITransmitEnable(mInputChannel, false);

	//	Wait for four verticals to let the reciever lock...
    mDevice.WaitForOutputVerticalInterrupt(mInputChannel, 10);

	//	Set the video format to match the incomming video format.
	//	Does the device support the desired input source?
	//	Determine the input video signal format...
	mVideoFormat = mDevice.GetInputVideoFormat (mInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr << "## ERROR:  No input signal or unknown format" << endl;
		return AJA_STATUS_NOINPUT;	//	Sorry, can't handle this format
	}

    // Convert the signal wire format to a 8k format
	CNTV2DemoCommon::Get8KInputFormat(mVideoFormat);
	mDevice.SetVideoFormat(mVideoFormat, false, false, mInputChannel);

    mDevice.SetQuadQuadFrameEnable(true, mInputChannel);
    mDevice.SetQuadQuadSquaresEnable(!mDoTsiRouting, mInputChannel);

	//	Set the device video format to whatever we detected at the input...
	//	The user has an option here. If doing multi-format, we are, lock to the board.
	//	If the user wants to E-E the signal then lock to input.
	mDevice.SetReference(NTV2_REFERENCE_FREERUN);

	//	Set the frame buffer pixel format for all the channels on the device
	//	(assuming it supports that pixel format -- otherwise default to 8-bit YCbCr)...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPixelFormat))
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;

	//	...and set all buffers pixel format...
    if (mDoTsiRouting)
    {
        if (mInputChannel < NTV2_CHANNEL3)
        {
            mDevice.SetFrameBufferFormat(NTV2_CHANNEL1, mPixelFormat);
            mDevice.SetFrameBufferFormat(NTV2_CHANNEL2, mPixelFormat);
            mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL1);
            mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL2);
            mDevice.EnableChannel(NTV2_CHANNEL1);
            mDevice.EnableChannel(NTV2_CHANNEL2);
            if (!mDoMultiFormat)
            {
                mDevice.DisableChannel(NTV2_CHANNEL3);
                mDevice.DisableChannel(NTV2_CHANNEL4);
            }
        }
        else
        {
            mDevice.SetFrameBufferFormat(NTV2_CHANNEL3, mPixelFormat);
            mDevice.SetFrameBufferFormat(NTV2_CHANNEL4, mPixelFormat);
            mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL3);
            mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL4);
            mDevice.EnableChannel(NTV2_CHANNEL3);
            mDevice.EnableChannel(NTV2_CHANNEL4);
            if (!mDoMultiFormat)
            {
                mDevice.DisableChannel(NTV2_CHANNEL1);
                mDevice.DisableChannel(NTV2_CHANNEL2);
            }
        }
    }
    else
    {
        mDevice.SetFrameBufferFormat(NTV2_CHANNEL1, mPixelFormat);
        mDevice.SetFrameBufferFormat(NTV2_CHANNEL2, mPixelFormat);
        mDevice.SetFrameBufferFormat(NTV2_CHANNEL3, mPixelFormat);
        mDevice.SetFrameBufferFormat(NTV2_CHANNEL4, mPixelFormat);
        mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL1);
        mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL2);
        mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL3);
        mDevice.SetEnableVANCData(false, false, NTV2_CHANNEL4);
        mDevice.EnableChannel(NTV2_CHANNEL1);
        mDevice.EnableChannel(NTV2_CHANNEL2);
        mDevice.EnableChannel(NTV2_CHANNEL3);
        mDevice.EnableChannel(NTV2_CHANNEL4);
    }
	
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2Capture8K::SetupAudio (void)
{
	//	In multiformat mode, base the audio system on the channel...
	if (mDoMultiFormat && ::NTV2DeviceGetNumAudioSystems (mDeviceID) > 1 && UWord (mInputChannel) < ::NTV2DeviceGetNumAudioSystems (mDeviceID))
		mAudioSystem = ::NTV2ChannelToAudioSystem (mInputChannel);

	//	Have the audio system capture audio from the designated device input (i.e., ch1 uses SDIIn1, ch2 uses SDIIn2, etc.)...
	mDevice.SetAudioSystemInputSource (mAudioSystem, NTV2_AUDIO_EMBEDDED, ::NTV2InputSourceToEmbeddedAudioInput (mInputSource));

	mDevice.SetNumberAudioChannels (::NTV2DeviceGetMaxAudioChannels (mDeviceID), mAudioSystem);
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	The on-device audio buffer should be 4MB to work best across all devices & platforms...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	mDevice.SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


void NTV2Capture8K::SetupHostBuffers (void)
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	mVideoBufferSize = ::GetVideoWriteSize (mVideoFormat, mPixelFormat);
	printf("video size = %d\n", mVideoBufferSize);
	mAudioBufferSize = NTV2_AUDIOSIZE_MAX;
	mAncBufferSize = NTV2_ANCSIZE_MAX;

	//	Allocate and add each in-host AVDataBuffer to my circular buffer member variable...
	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++ )
	{
		mAVHostBuffer [bufferNdx].fVideoBuffer		= reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mVideoBufferSize, BUFFER_ALIGNMENT));
		mAVHostBuffer [bufferNdx].fVideoBufferSize	= mVideoBufferSize;
		mAVHostBuffer [bufferNdx].fAudioBuffer		= mWithAudio ? reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mAudioBufferSize, BUFFER_ALIGNMENT)) : 0;
		mAVHostBuffer [bufferNdx].fAudioBufferSize	= mWithAudio ? mAudioBufferSize : 0;
		mAVHostBuffer [bufferNdx].fAncBuffer		= mWithAnc ? reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mAncBufferSize, BUFFER_ALIGNMENT)) : 0;
		mAVHostBuffer [bufferNdx].fAncBufferSize	= mAncBufferSize;
		mAVCircularBuffer.Add (& mAVHostBuffer [bufferNdx]);

		// Page lock the memory
		if (mAVHostBuffer [bufferNdx].fVideoBuffer != AJA_NULL)
			mDevice.DMABufferLock((ULWord*)mAVHostBuffer [bufferNdx].fVideoBuffer, mVideoBufferSize, true);
		if (mAVHostBuffer [bufferNdx].fAudioBuffer)
            mDevice.DMABufferLock((ULWord*)mAVHostBuffer [bufferNdx].fAudioBuffer, mAudioBufferSize, true);
		if (mAVHostBuffer [bufferNdx].fAncBuffer)
            mDevice.DMABufferLock((ULWord*)mAVHostBuffer [bufferNdx].fAncBuffer, mAncBufferSize, true);
	}	//	for each AVDataBuffer

}	//	SetupHostBuffers


void NTV2Capture8K::RouteInputSignal(void)
{
    if (mDoTsiRouting)
    {
        if (::IsRGBFormat (mPixelFormat))
        {
            if (mInputChannel < NTV2_CHANNEL3)
            {
                mDevice.Connect(NTV2_XptDualLinkIn1Input,       NTV2_XptSDIIn1);
                mDevice.Connect(NTV2_XptDualLinkIn1DSInput,     NTV2_XptSDIIn1DS2);
                mDevice.Connect(NTV2_XptDualLinkIn2Input,       NTV2_XptSDIIn2);
                mDevice.Connect(NTV2_XptDualLinkIn2DSInput,     NTV2_XptSDIIn2DS2);
                mDevice.Connect(NTV2_XptDualLinkIn3Input,       NTV2_XptSDIIn3);
                mDevice.Connect(NTV2_XptDualLinkIn3DSInput,     NTV2_XptSDIIn3DS2);
                mDevice.Connect(NTV2_XptDualLinkIn4Input,       NTV2_XptSDIIn4);
                mDevice.Connect(NTV2_XptDualLinkIn4DSInput,     NTV2_XptSDIIn4DS2);
                mDevice.Connect(NTV2_XptFrameBuffer1Input,      NTV2_XptDuallinkIn1);
                mDevice.Connect(NTV2_XptFrameBuffer1DS2Input,   NTV2_XptDuallinkIn2);
                mDevice.Connect(NTV2_XptFrameBuffer2Input,      NTV2_XptDuallinkIn3);
                mDevice.Connect(NTV2_XptFrameBuffer2DS2Input,   NTV2_XptDuallinkIn4);
                mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, false);
                mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, false);
                mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, false);
                mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, false);
            }
            else
            {
                mDevice.Connect(NTV2_XptDualLinkIn1Input,       NTV2_XptSDIIn1);
                mDevice.Connect(NTV2_XptDualLinkIn1DSInput,     NTV2_XptSDIIn1DS2);
                mDevice.Connect(NTV2_XptDualLinkIn2Input,       NTV2_XptSDIIn2);
                mDevice.Connect(NTV2_XptDualLinkIn2DSInput,     NTV2_XptSDIIn2DS2);
                mDevice.Connect(NTV2_XptDualLinkIn3Input,       NTV2_XptSDIIn3);
                mDevice.Connect(NTV2_XptDualLinkIn3DSInput,     NTV2_XptSDIIn3DS2);
                mDevice.Connect(NTV2_XptDualLinkIn4Input,       NTV2_XptSDIIn4);
                mDevice.Connect(NTV2_XptDualLinkIn4DSInput,     NTV2_XptSDIIn4DS2);
                mDevice.Connect(NTV2_XptFrameBuffer3Input,      NTV2_XptDuallinkIn1);
                mDevice.Connect(NTV2_XptFrameBuffer3DS2Input,   NTV2_XptDuallinkIn2);
                mDevice.Connect(NTV2_XptFrameBuffer4Input,      NTV2_XptDuallinkIn3);
                mDevice.Connect(NTV2_XptFrameBuffer4DS2Input,   NTV2_XptDuallinkIn4);
                mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, false);
                mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, false);
                mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, false);
                mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, false);
            }
        }
        else
        {
            if (mInputChannel < NTV2_CHANNEL3)
            {
                if (NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(mVideoFormat))
                {
                    mDevice.Connect(NTV2_XptFrameBuffer1Input,      NTV2_XptSDIIn1);
                    mDevice.Connect(NTV2_XptFrameBuffer1DS2Input,   NTV2_XptSDIIn2);
                    mDevice.Connect(NTV2_XptFrameBuffer2Input,      NTV2_XptSDIIn3);
                    mDevice.Connect(NTV2_XptFrameBuffer2DS2Input,   NTV2_XptSDIIn4);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, false);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, false);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, false);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, false);
                }
                else
                {
                    mDevice.Connect(NTV2_XptFrameBuffer1Input,      NTV2_XptSDIIn1);
                    mDevice.Connect(NTV2_XptFrameBuffer1DS2Input,   NTV2_XptSDIIn1DS2);
                    mDevice.Connect(NTV2_XptFrameBuffer2Input,      NTV2_XptSDIIn2);
                    mDevice.Connect(NTV2_XptFrameBuffer2DS2Input,   NTV2_XptSDIIn2DS2);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, false);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, false);
                }
            }
            else
            {
                if (NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(mVideoFormat))
                {
                    mDevice.Connect(NTV2_XptFrameBuffer3Input,      NTV2_XptSDIIn1);
                    mDevice.Connect(NTV2_XptFrameBuffer3DS2Input,   NTV2_XptSDIIn2);
                    mDevice.Connect(NTV2_XptFrameBuffer4Input,      NTV2_XptSDIIn3);
                    mDevice.Connect(NTV2_XptFrameBuffer4DS2Input,   NTV2_XptSDIIn4);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, false);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, false);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, false);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, false);
                }
                else
                {
                    mDevice.Connect(NTV2_XptFrameBuffer3Input,      NTV2_XptSDIIn3);
                    mDevice.Connect(NTV2_XptFrameBuffer3DS2Input,   NTV2_XptSDIIn3DS2);
                    mDevice.Connect(NTV2_XptFrameBuffer4Input,      NTV2_XptSDIIn4);
                    mDevice.Connect(NTV2_XptFrameBuffer4DS2Input,   NTV2_XptSDIIn4DS2);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, false);
                    mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, false);
                }
            }
        }
	}
	else
	{
        if (::IsRGBFormat (mPixelFormat))
        {
            mDevice.Connect(NTV2_XptDualLinkIn1Input,        NTV2_XptSDIIn1);
            mDevice.Connect(NTV2_XptDualLinkIn1DSInput, NTV2_XptSDIIn1DS2);
            mDevice.Connect(NTV2_XptDualLinkIn2Input,        NTV2_XptSDIIn2);
            mDevice.Connect(NTV2_XptDualLinkIn2DSInput, NTV2_XptSDIIn2DS2);
            mDevice.Connect(NTV2_XptDualLinkIn3Input,        NTV2_XptSDIIn3);
            mDevice.Connect(NTV2_XptDualLinkIn3DSInput, NTV2_XptSDIIn3DS2);
            mDevice.Connect(NTV2_XptDualLinkIn4Input,        NTV2_XptSDIIn4);
            mDevice.Connect(NTV2_XptDualLinkIn4DSInput, NTV2_XptSDIIn4DS2);
            mDevice.Connect(NTV2_XptFrameBuffer1Input,  NTV2_XptDuallinkIn1);
            mDevice.Connect(NTV2_XptFrameBuffer2Input,  NTV2_XptDuallinkIn2);
            mDevice.Connect(NTV2_XptFrameBuffer3Input,  NTV2_XptDuallinkIn3);
            mDevice.Connect(NTV2_XptFrameBuffer4Input,  NTV2_XptDuallinkIn4);
            mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, false);
            mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, false);
            mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, false);
            mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, false);
        }
        else
        {
            mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
            mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_XptSDIIn2);
            mDevice.Connect(NTV2_XptFrameBuffer3Input, NTV2_XptSDIIn3);
            mDevice.Connect(NTV2_XptFrameBuffer4Input, NTV2_XptSDIIn4);
            mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, false);
            mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, false);
            mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, false);
            mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, false);
        }
	}
}	//	RouteInputSignal


void NTV2Capture8K::SetupInputAutoCirculate (void)
{
	//	Tell capture AutoCirculate to use 7 frame buffers on the device...
	ULWord startFrame = 0, endFrame = 7;
	mDevice.AutoCirculateStop(NTV2_CHANNEL1);
	mDevice.AutoCirculateStop(NTV2_CHANNEL2);
	mDevice.AutoCirculateStop(NTV2_CHANNEL3);
	mDevice.AutoCirculateStop(NTV2_CHANNEL4);

	mDevice.AutoCirculateInitForInput (mInputChannel,	0,	//	0 frames == explicitly set start & end frames
										mWithAudio ? mAudioSystem : NTV2_AUDIOSYSTEM_INVALID,	//	Which audio system (if any)?
										AUTOCIRCULATE_WITH_RP188 | AUTOCIRCULATE_WITH_ANC,		//	Include timecode & custom Anc
										1, startFrame, endFrame);
}	//	SetupInputAutoCirculate


AJAStatus NTV2Capture8K::Run ()
{
	//	Start the playout and capture threads...
	StartConsumerThread ();
	StartProducerThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we will start the consumer thread
void NTV2Capture8K::StartConsumerThread (void)
{
	//	Create and start the consumer thread...
	mConsumerThread.Attach(ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The consumer thread function
void NTV2Capture8K::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Capture instance pointer from the pContext parameter,
	//	then call its ConsumeFrames method...
	NTV2Capture8K *	pApp	(reinterpret_cast <NTV2Capture8K *> (pContext));
	pApp->ConsumeFrames ();

}	//	ConsumerThreadStatic


void NTV2Capture8K::ConsumeFrames (void)
{
	CAPNOTE("Thread started");
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
	CAPNOTE("Thread completed, will exit");

}	//	ConsumeFrames


//////////////////////////////////////////////



//////////////////////////////////////////////

//	This is where we start the capture thread
void NTV2Capture8K::StartProducerThread (void)
{
	//	Create and start the capture thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


//	The capture thread function
void NTV2Capture8K::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Capture instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2Capture8K *	pApp	(reinterpret_cast <NTV2Capture8K *> (pContext));
	pApp->CaptureFrames ();

}	//	ProducerThreadStatic


void NTV2Capture8K::CaptureFrames (void)
{
	NTV2AudioChannelPairs	nonPcmPairs, oldNonPcmPairs;
	CAPNOTE("Thread started");

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
			// mInputTransfer.acTransferStatus.acAudioTransferSize;   // this is the amount of audio captured

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
	CAPNOTE("Thread completed, will exit");

}	//	CaptureFrames


//////////////////////////////////////////////


void NTV2Capture8K::GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel)
{
	AUTOCIRCULATE_STATUS	status;
	mDevice.AutoCirculateGetStatus (mInputChannel, status);
	outGoodFrames = status.acFramesProcessed;
	outDroppedFrames = status.acFramesDropped;
	outBufferLevel = status.acBufferLevel;
}
