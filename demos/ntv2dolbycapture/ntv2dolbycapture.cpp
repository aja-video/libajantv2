/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolbycapture.cpp
	@brief		Implementation of NTV2DolbyCapture class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

//#define AJA_RAW_AUDIO_RECORD	//	Uncomment to record raw audio file
//#define AJA_WAV_AUDIO_RECORD	//	Uncomment to record WAV audio file
#include "ntv2dolbycapture.h"
#include "ntv2utils.h"
#include "ntv2debug.h"	//	for NTV2DeviceString
#include "ntv2devicefeatures.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include <iterator>	//	for inserter
#include <fstream>	//	for ofstream

using namespace std;

#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define NTV2_ANCILLARYSIZE_MAX	(256 * 1024)
//#define NTV2_BUFFER_LOCK


//////////////////////////////////////////////////////////////////////////////////////	NTV2DolbyCapture IMPLEMENTATION

NTV2DolbyCapture::NTV2DolbyCapture (const DolbyConfig & inConfig)
	:	mConsumerThread		(AJAThread()),
		mProducerThread		(AJAThread()),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mConfig				(inConfig),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
        mAudioSystem        (NTV2_AUDIOSYSTEM_1),
		mGlobalQuit			(false),
		mHostBuffers		(),
		mAVCircularBuffer	()
{
}	//	constructor


NTV2DolbyCapture::~NTV2DolbyCapture ()
{
	//	Stop my capture and consumer threads, then destroy them...
	Quit();

	//	Unsubscribe from input vertical event...
	mDevice.UnsubscribeInputVerticalEvent(mConfig.fInputChannel);

}	//	destructor


void NTV2DolbyCapture::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mConsumerThread.Active())
		AJATime::Sleep(10);

	while (mProducerThread.Active())
		AJATime::Sleep(10);

	if (!mConfig.fDoMultiFormat)
	{
		mDevice.ReleaseStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));
		mDevice.SetEveryFrameServices(mSavedTaskMode);		//	Restore prior task mode
	}

}	//	Quit


AJAStatus NTV2DolbyCapture::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID();						//	Keep the device ID handy, as it's used frequently
	if (!::NTV2DeviceCanDoCapture(mDeviceID))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' cannot capture" << endl;  return AJA_STATUS_FEATURE;}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetStreamingApplication (appSignature, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);			//	Save the current device state
	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Another app is using the device
		}
		mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current state before we change it
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Since this is an OEM demo, use the OEM service level

	if (::NTV2DeviceCanDoMultiFormat(mDeviceID))
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);

	if (!NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  !NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputChannel = NTV2_CHANNEL1;
	else if (!NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputChannel = ::NTV2InputSourceToChannel(mConfig.fInputSource);
	else if (NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  !NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
        mConfig.fInputSource = ::NTV2ChannelToInputSource(mConfig.fInputChannel, NTV2_INPUTSOURCES_HDMI);
	//	On KonaHDMI, map specified SDI input to equivalent HDMI input...
    if (::NTV2DeviceGetNumHDMIVideoInputs(mDeviceID) > 1  &&  NTV2_INPUT_SOURCE_IS_HDMI(mConfig.fInputSource))
		mConfig.fInputSource = ::NTV2ChannelToInputSource(::NTV2InputSourceToChannel(mConfig.fInputSource), NTV2_INPUTSOURCES_HDMI);

	//	Set up the video and audio...
	status = SetupVideo();
	if (AJA_FAILURE(status))
		return status;

    status = SetupAudio();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the circular buffers, the device signal routing, and both playout and capture AutoCirculate...
	SetupHostBuffers();
	RouteInputSignal();

	mDolbyState = 0;

	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#endif	//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2DolbyCapture::SetupVideo (void)
{
	//	Sometimes other applications disable some or all of the frame buffers, so turn on ours now...
	mDevice.EnableChannel(mConfig.fInputChannel);

	//	Enable and subscribe to the interrupts for the channel to be used...
	mDevice.EnableInputInterrupt(mConfig.fInputChannel);
	mDevice.SubscribeInputVerticalEvent(mConfig.fInputChannel);

	//	Determine the input video signal format...
	mVideoFormat = mDevice.GetInputVideoFormat (mConfig.fInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr << "## ERROR:  No input signal or unknown format" << endl;
		return AJA_STATUS_NOINPUT;	//	Sorry, can't handle this format
	}
	mProgressive = NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mVideoFormat);

	//	Set the frame buffer pixel format for all the channels on the device
	//	(assuming it supports that pixel format -- otherwise default to 8-bit YCbCr)...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.fPixelFormat))
	{
		cerr	<< "## WARNING:  " << ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat)
				<< " unsupported, using " << ::NTV2FrameBufferFormatToString(NTV2_FBF_8BIT_YCBCR)
				<< " instead" << endl;
		mConfig.fPixelFormat = NTV2_FBF_8BIT_YCBCR;
	}

	//	Set the device video format to whatever we detected at the input...
	if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat))
	{
		mDevice.SetReference (::NTV2InputSourceToReferenceSource(NTV2_INPUTSOURCE_HDMI1));
		mDevice.SetVideoFormat (mVideoFormat, false, false, NTV2_CHANNEL1);
		mDevice.SetTsiFrameEnable(true, NTV2_CHANNEL1);
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mConfig.fPixelFormat);
	}
	else
	{
		mDevice.SetReference (::NTV2InputSourceToReferenceSource(mConfig.fInputSource));
		mDevice.SetVideoFormat (mVideoFormat, false, false, mConfig.fInputChannel);
		mDevice.SetTsiFrameEnable(false, mConfig.fInputChannel);
		mDevice.SetFrameBufferFormat (mConfig.fInputChannel, mConfig.fPixelFormat);
	}

	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2DolbyCapture::SetupAudio (void)
{
	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


void NTV2DolbyCapture::SetupHostBuffers (void)
{
	NTV2VANCMode	vancMode(NTV2_VANCMODE_INVALID);
	NTV2Standard	standard(NTV2_STANDARD_INVALID);
	ULWord			F1AncSize(0), F2AncSize(0);
	mDevice.GetVANCMode (vancMode);
	mDevice.GetStandard (standard);

	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	// configure anc buffers
	mDevice.AncSetFrameBufferSize(NTV2_ANCILLARYSIZE_MAX, NTV2_ANCILLARYSIZE_MAX);
	F1AncSize = NTV2_ANCILLARYSIZE_MAX;
	F2AncSize = NTV2_ANCILLARYSIZE_MAX;

	mFormatDesc = NTV2FormatDescriptor (standard, mConfig.fPixelFormat, vancMode);

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	mHostBuffers.reserve(size_t(CIRCULAR_BUFFER_SIZE));
	while (mHostBuffers.size() < size_t(CIRCULAR_BUFFER_SIZE))
	{
		mHostBuffers.push_back(NTV2FrameData());
		NTV2FrameData & frameData(mHostBuffers.back());
		frameData.fVideoBuffer.Allocate(::GetVideoWriteSize (mVideoFormat, mConfig.fPixelFormat, vancMode));
		frameData.fAudioBuffer.Allocate(NTV2_AUDIOSIZE_MAX);
		if (F1AncSize)
			frameData.fAncBuffer.Allocate(F1AncSize);
		if (!mProgressive && F2AncSize)
			frameData.fAncBuffer2.Allocate(F2AncSize);
		mAVCircularBuffer.Add(&frameData);

#ifdef NTV2_BUFFER_LOCK
		// Page lock the memory
		if (frameData.fVideoBuffer)
			mDevice.DMABufferLock(frameData.fVideoBuffer, true);
		if (frameData.fAncBuffer)
			mDevice.DMABufferLock(frameData.fAncBuffer, true);
        if (frameData.fAncBuffer2)
            mDevice.DMABufferLock(frameData.fAncBuffer2, true);
#endif
	}	//	for each NTV2FrameData

}	//	SetupHostBuffers


void NTV2DolbyCapture::RouteInputSignal (void)
{
	//	For this simple example, tie the user-selected input to frame buffer 1.
	//	Is this user-selected input supported on the device?
	if (!::NTV2DeviceCanDoInputSource (mDeviceID, mConfig.fInputSource))
		mConfig.fInputSource = NTV2_INPUTSOURCE_SDI1;

	NTV2LHIHDMIColorSpace	inputColor	(NTV2_LHIHDMIColorSpaceYCbCr);
	if (NTV2_INPUT_SOURCE_IS_HDMI(mConfig.fInputSource))
		mDevice.GetHDMIInputColor (inputColor, mConfig.fInputChannel);

	const bool						canVerify				(mDevice.HasCanConnectROM());
	const bool						isInputRGB				(inputColor == NTV2_LHIHDMIColorSpaceRGB);
	const bool						isFrameRGB				(::IsRGBFormat(mConfig.fPixelFormat));
	const NTV2OutputCrosspointID	inputWidgetOutputXpt	(::GetInputSourceOutputXpt(mConfig.fInputSource, false, isInputRGB, 0));
	const NTV2InputCrosspointID		frameBufferInputXpt		(::GetFrameBufferInputXptFromChannel(mConfig.fInputChannel));
	const NTV2InputCrosspointID		cscWidgetVideoInputXpt	(::GetCSCInputXptFromChannel(mConfig.fInputChannel));
	const NTV2OutputCrosspointID	cscWidgetRGBOutputXpt	(::GetCSCOutputXptFromChannel(mConfig.fInputChannel, /*inIsKey*/ false, /*inIsRGB*/ true));
	const NTV2OutputCrosspointID	cscWidgetYUVOutputXpt	(::GetCSCOutputXptFromChannel(mConfig.fInputChannel, /*inIsKey*/ false, /*inIsRGB*/ false));


	if (!mConfig.fDoMultiFormat)
		mDevice.ClearRouting();

	if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat))
	{
		if (isInputRGB && !isFrameRGB)
		{
			mDevice.Connect(NTV2_XptCSC1VidInput, NTV2_XptHDMIIn1RGB, canVerify);
			mDevice.Connect(NTV2_XptCSC2VidInput, NTV2_XptHDMIIn1Q2RGB, canVerify);
			mDevice.Connect(NTV2_XptCSC3VidInput, NTV2_XptHDMIIn1Q3RGB, canVerify);
			mDevice.Connect(NTV2_XptCSC4VidInput, NTV2_XptHDMIIn1Q4RGB, canVerify);

			mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidYUV, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptCSC2VidYUV, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptCSC3VidYUV, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptCSC4VidYUV, canVerify);

			mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1AYUV, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer1DS2Input, NTV2_Xpt425Mux1BYUV, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer2DS2Input, NTV2_Xpt425Mux2BYUV, canVerify);
		}
		else if (!isInputRGB && isFrameRGB)
		{
			mDevice.Connect(NTV2_XptCSC1VidInput, NTV2_XptHDMIIn1, canVerify);
			mDevice.Connect(NTV2_XptCSC2VidInput, NTV2_XptHDMIIn1Q2, canVerify);
			mDevice.Connect(NTV2_XptCSC3VidInput, NTV2_XptHDMIIn1Q3, canVerify);
			mDevice.Connect(NTV2_XptCSC4VidInput, NTV2_XptHDMIIn1Q4, canVerify);

			mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidRGB, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptCSC2VidRGB, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptCSC3VidRGB, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptCSC4VidRGB, canVerify);

			mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1ARGB, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer1DS2Input, NTV2_Xpt425Mux1BRGB, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2ARGB, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer2DS2Input, NTV2_Xpt425Mux2BRGB, canVerify);
		}
		else if (isInputRGB && isFrameRGB)
		{
			mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptHDMIIn1RGB, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptHDMIIn1Q2RGB, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptHDMIIn1Q3RGB, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptHDMIIn1Q4RGB, canVerify);

			mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1ARGB, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer1DS2Input, NTV2_Xpt425Mux1BRGB, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2ARGB, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer2DS2Input, NTV2_Xpt425Mux2BRGB, canVerify);
		}
		else
		{
			mDevice.Connect(NTV2_Xpt425Mux1AInput, NTV2_XptHDMIIn1, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux1BInput, NTV2_XptHDMIIn1Q2, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux2AInput, NTV2_XptHDMIIn1Q3, canVerify);
			mDevice.Connect(NTV2_Xpt425Mux2BInput, NTV2_XptHDMIIn1Q4, canVerify);

			mDevice.Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1AYUV, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer1DS2Input, NTV2_Xpt425Mux1BYUV, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV, canVerify);
			mDevice.Connect(NTV2_XptFrameBuffer2DS2Input, NTV2_Xpt425Mux2BYUV, canVerify);
		}
	}
	else
	{
		if (isInputRGB && !isFrameRGB)
		{
			mDevice.Connect (frameBufferInputXpt,		cscWidgetYUVOutputXpt,	canVerify);	//	Frame store input to CSC widget's YUV output
			mDevice.Connect (cscWidgetVideoInputXpt,	inputWidgetOutputXpt,	canVerify);	//	CSC widget's RGB input to input widget's output
		}
		else if (!isInputRGB && isFrameRGB)
		{
			mDevice.Connect (frameBufferInputXpt,		cscWidgetRGBOutputXpt,	canVerify);	//	Frame store input to CSC widget's RGB output
			mDevice.Connect (cscWidgetVideoInputXpt,	inputWidgetOutputXpt,	canVerify);	//	CSC widget's YUV input to input widget's output
		}
		else
			mDevice.Connect (frameBufferInputXpt,		inputWidgetOutputXpt,	canVerify);	//	Frame store input to input widget's output
	}

}	//	RouteInputSignal


AJAStatus NTV2DolbyCapture::Run ()
{
	//	Start the playout and capture threads...
	StartConsumerThread ();
	StartProducerThread ();
	return AJA_STATUS_SUCCESS;

}	//	Run


//////////////////////////////////////////////////////////////////////////////////////////////////////////

//	This is where we will start the consumer thread
void NTV2DolbyCapture::StartConsumerThread (void)
{
	//	Create and start the consumer thread...
	mConsumerThread.Attach(ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The consumer thread function
void NTV2DolbyCapture::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2DolbyCapture instance pointer from the pContext parameter,
	//	then call its ConsumeFrames method...
	NTV2DolbyCapture *	pApp	(reinterpret_cast <NTV2DolbyCapture *> (pContext));
	pApp->ConsumeFrames ();

}	//	ConsumerThreadStatic


void NTV2DolbyCapture::ConsumeFrames (void)
{
    uint64_t ancTally(0);
	uint64_t audioTally(0);
	uint64_t dolbyTally(0);
	ofstream * pOFS(mConfig.fWithAnc ? new ofstream(mConfig.fAncDataFilePath.c_str(), ios::binary) : AJA_NULL);
	ofstream * pAFS(mConfig.fWithAudio ? new ofstream(mConfig.fAudioDataFilePath.c_str(), ios::binary) : AJA_NULL);
	ofstream * pDFS(mConfig.fWithDolby ? new ofstream(mConfig.fDolbyDataFilePath.c_str(), ios::binary) : AJA_NULL);
	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		NTV2FrameData *	pFrameData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (pFrameData)
		{
			if (mConfig.fDoFrameData && pFrameData->AncBuffer())
			{
				uint8_t* pData = (uint8_t*)pFrameData->AncBuffer().GetHostAddress(0);
				uint32_t i;
				for (i = 0; i < pFrameData->AncBufferSize(); i += 32)
				{
					if (pData[i] == 0xff)
							break;
				}
				uint32_t audioSize = RecoverAudio(pFrameData->AncBuffer(), pFrameData->AncBufferSize() /*pFrameData->NumCapturedAncBytes()*/, pFrameData->AudioBuffer());
				printf("f1 size reg %d  ffs %d  samples %d\n", pFrameData->NumCapturedAncBytes(), i, audioSize/4);
				fflush(stdout);
			}

			if (mConfig.fDoFrameData && !mProgressive && pFrameData->AncBuffer2())
			{
				uint8_t* pData = (uint8_t*)pFrameData->AncBuffer2().GetHostAddress(0);
				uint32_t i;
				for (i = 0; i < pFrameData->AncBuffer2Size(); i += 32)
				{
					if (pData[i] == 0xff)
							break;
				}
				uint32_t audioSize = RecoverAudio(pFrameData->AncBuffer2(), pFrameData->AncBuffer2Size() /*pFrameData->NumCapturedAnc2Bytes()*/, pFrameData->AudioBuffer());
				printf("f2 size reg %d  ffs %d  samples %d\n", pFrameData->NumCapturedAnc2Bytes(), i, audioSize/4);
				fflush(stdout);
			}

			if (pOFS && pFrameData->AncBuffer())
			{
				pOFS->write(pFrameData->AncBuffer(), streamsize(pFrameData->NumCapturedAncBytes()));
				ancTally++;
				if (!mProgressive && pFrameData->AncBuffer2())
				{
					if (pOFS  &&  pFrameData->AncBuffer2())
						pOFS->write(pFrameData->AncBuffer2(), streamsize(pFrameData->NumCapturedAnc2Bytes()));
				}
			}

			if (pAFS && pFrameData->AncBuffer() && pFrameData->AudioBuffer())
			{
				uint32_t audioSize = RecoverAudio(pFrameData->AncBuffer(), pFrameData->NumCapturedAncBytes(), pFrameData->AudioBuffer());
				pAFS->write(pFrameData->AudioBuffer(), streamsize(audioSize));
				audioTally++;
				if (!mProgressive && pFrameData->AncBuffer2())
				{
					audioSize = RecoverAudio(pFrameData->AncBuffer2(), pFrameData->NumCapturedAnc2Bytes(), pFrameData->AudioBuffer());
					pAFS->write(pFrameData->AudioBuffer(), streamsize(audioSize));
				}
			}

			if (pDFS && pFrameData->AncBuffer() && pFrameData->AudioBuffer())
			{
				uint32_t audioSize = RecoverAudio(pFrameData->AncBuffer(), pFrameData->NumCapturedAncBytes(), pFrameData->AudioBuffer());
				uint32_t dolbySize = RecoverDolby(pFrameData->AudioBuffer(), audioSize, pFrameData->AncBuffer());
				pDFS->write(pFrameData->AncBuffer(), streamsize(dolbySize));
				dolbyTally++;
				if (!mProgressive && pFrameData->AncBuffer2())
				{
					audioSize = RecoverAudio(pFrameData->AncBuffer2(), pFrameData->NumCapturedAnc2Bytes(), pFrameData->AudioBuffer());
					dolbySize = RecoverDolby(pFrameData->AudioBuffer(), audioSize, pFrameData->AncBuffer2());
					pDFS->write(pFrameData->AncBuffer2(), streamsize(dolbySize));
				}
			}

			//	Now release and recycle the buffer...
			mAVCircularBuffer.EndConsumeNextBuffer ();
		}	//	if pFrameData
	}	//	loop til quit signaled
	if (pOFS)
		{ delete pOFS; cerr << "Wrote " << DEC(ancTally) << " frames of raw anc data" << endl; }
	if (pAFS)
		{ delete pAFS; cerr << "Wrote " << DEC(audioTally) << " frames of raw audio data" << endl; }
	if (pDFS)
		{ delete pDFS; cerr << "Wrote " << DEC(dolbyTally) << " frames of dolby data" << endl; }

}	//	ConsumeFrames


//////////////////////////////////////////////////////////////////////////////////////////////////////////

//	This is where we start the capture thread
void NTV2DolbyCapture::StartProducerThread (void)
{
	//	Create and start the capture thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


//	The capture thread function
void NTV2DolbyCapture::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2DolbyCapture instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2DolbyCapture *	pApp	(reinterpret_cast <NTV2DolbyCapture *> (pContext));
	pApp->CaptureFrames ();

}	//	ProducerThreadStatic


void NTV2DolbyCapture::CaptureFrames (void)
{
	AUTOCIRCULATE_TRANSFER	inputXfer;	//	My A/C input transfer info
	ULWord					acOptions (0);
	acOptions |= AUTOCIRCULATE_WITH_ANC;
	acOptions |= AUTOCIRCULATE_WITH_HDMIAUX;

	CAPNOTE("Thread started");
	//	Initialize and start capture AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fInputChannel);	//	Just in case
	mDevice.AutoCirculateInitForInput (	mConfig.fInputChannel,		//	primary channel
										mConfig.fFrames.count(),	//	numFrames (zero if specifying range)
										mAudioSystem,				//	audio system
										acOptions,					//	flags
										1,							//	numChannels to gang
										mConfig.fFrames.firstFrame(), mConfig.fFrames.lastFrame());

	//	This needs an ntv2card api
	if (mConfig.fDoAudioFilter)
	{
		mDevice.WriteRegister(7616, 0x1, maskAuxFilterInvert, shiftAuxFilterInvert);
		mDevice.WriteRegister(7628, 0x2);
	}
	else
	{
		mDevice.WriteRegister(7616, 0x0, maskAuxFilterInvert, shiftAuxFilterInvert);
		mDevice.WriteRegister(7628, 0x0);
	}

	mDevice.AutoCirculateStart(mConfig.fInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS acStatus;
		mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, acStatus);

		if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
		{
			//	At this point, there's at least one fully-formed frame available in the device's
			//	frame buffer to transfer to the host. Reserve an NTV2FrameData to "produce", and
			//	use it in the next transfer from the device...
			NTV2FrameData *	pCaptureData(mAVCircularBuffer.StartProduceNextBuffer());

			inputXfer.SetVideoBuffer (pCaptureData->VideoBuffer(), pCaptureData->VideoBufferSize());
			if (acStatus.WithCustomAnc())
				inputXfer.SetAncBuffers (pCaptureData->AncBuffer(), pCaptureData->AncBufferSize(),
										 pCaptureData->AncBuffer2(), pCaptureData->AncBuffer2Size());

			//	Clear anc buffers
			if (pCaptureData->AncBuffer())
				pCaptureData->AncBuffer().Fill(uint8_t(0));
			if (pCaptureData->AncBuffer2())
				pCaptureData->AncBuffer2().Fill(uint8_t(0));

			//	Transfer video/audio/anc from the device into our host buffers...
			mDevice.AutoCirculateTransfer (mConfig.fInputChannel, inputXfer);

			//	Get number of anc bytes
			if (pCaptureData->AncBuffer())
				pCaptureData->fNumAncBytes = inputXfer.GetCapturedAncByteCount(/*isF2*/false);
			if (pCaptureData->AncBuffer2())
				pCaptureData->fNumAnc2Bytes = inputXfer.GetCapturedAncByteCount(/*isF2*/true);

			//	Signal that we're done "producing" the frame, making it available for future "consumption"...
			mAVCircularBuffer.EndProduceNextBuffer();
		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel);
		}

	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fInputChannel);
	CAPNOTE("Thread completed, will exit");

}	//	CaptureFrames


void NTV2DolbyCapture::GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel)
{
	AUTOCIRCULATE_STATUS	status;
	mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, status);
	outGoodFrames = status.acFramesProcessed;
	outDroppedFrames = status.acFramesDropped;
	outBufferLevel = status.acBufferLevel;
}

uint32_t NTV2DolbyCapture::RecoverAudio(NTV2Buffer & anc, uint32_t ancSize, NTV2Buffer & audio)
{
	uint8_t* audioData = (uint8_t*)audio.GetHostAddress(0);
	uint32_t audioSize = 0;
	uint8_t* ancData = (uint8_t*)anc.GetHostAddress(0);

	// extract the first 16 bit stereo pair from the aux data
	for (uint32_t i = 0; i < ancSize/32; i++)
	{
		// audio data aux packet?
		if (ancData[0] == 0x02)
		{
			// first sample present?
			if ((ancData[1] & 0x01) != 0)
			{
				// first sample flat?
				if ((ancData[2] & 0x01) != 0)
				{
					*audioData++ = 0;
					*audioData++ = 0;
					*audioData++ = 0;
					*audioData++ = 0;
				}
				else
				{
					*audioData++ = ancData[4];
					*audioData++ = ancData[5];
					*audioData++ = ancData[7];
					*audioData++ = ancData[8];
				}
				audioSize += 4;
			}
			// only 2 channels?
			if ((ancData[1] & 0x10) == 0)
			{
				// second sample present?
				if ((ancData[1] & 0x02) != 0)
				{
					// second sampel flat?
					if ((ancData[2] & 0x02) != 0)
					{
						*audioData++ = 0;
						*audioData++ = 0;
						*audioData++ = 0;
						*audioData++ = 0;
					}
					else
					{
						*audioData++ = ancData[11];
						*audioData++ = ancData[12];
						*audioData++ = ancData[14];
						*audioData++ = ancData[15];
					}
					audioSize += 4;
				}
				// third sample present?
				if ((ancData[1] & 0x04) != 0)
				{
					// third sample flat?
					if ((ancData[2] & 0x04) != 0)
					{
						*audioData++ = 0;
						*audioData++ = 0;
						*audioData++ = 0;
						*audioData++ = 0;
					}
					else
					{
						*audioData++ = ancData[18];
						*audioData++ = ancData[19];
						*audioData++ = ancData[21];
						*audioData++ = ancData[22];
					}
					audioSize += 4;
				}
				// fourth sample present?
				if ((ancData[1] & 0x08) != 0)
				{
					// fourth sample flat?
					if ((ancData[2] & 0x08) != 0)
					{
						*audioData++ = 0;
						*audioData++ = 0;
						*audioData++ = 0;
						*audioData++ = 0;
					}
					else
					{
						*audioData++ = ancData[25];
						*audioData++ = ancData[26];
						*audioData++ = ancData[28];
						*audioData++ = ancData[29];
					}
					audioSize += 4;
				}
			}
		}
		ancData += 32;
	}

	return audioSize;
}

uint32_t NTV2DolbyCapture::RecoverDolby(NTV2Buffer & audio, uint32_t audioSize, NTV2Buffer & dolby)
{
	uint16_t* dolbyData = (uint16_t*)dolby.GetHostAddress(0);
	uint32_t dolbySize = 0;
	uint16_t* audioData = (uint16_t*)audio.GetHostAddress(0);

	// extract the dolby frames from the IEC61937 bursts
	for (uint32_t i = 0; i < audioSize / 2; i++)
	{
		switch (mDolbyState)
		{
		// find IEC61937 burst preamble
		case 0:
			mDolbyState = (*audioData == 0xf872)? 1 : 0;
			break;
		case 1:
			mDolbyState = (*audioData == 0x4e1f)? 2 : 0;
			break;
		// check dolby code
		case 2:
			mDolbyState = ((*audioData & 0xff) == 0x15)? 3 : 0;
			break;
		// get burst length
		case 3:
			mDolbyLength = (uint32_t)*audioData / 2;
			mDolbyState = 4;
			break;
		// copy dolby samples
		case 4:
			if (mDolbyLength > 0)
			{
				if (dolbySize < dolby.GetByteCount() / 2)
				{
					// endian swap the data
					*dolbyData = (*audioData >> 8) & 0xff;
					*dolbyData |= (*audioData & 0xff) << 8;
					dolbyData++;
					dolbySize++;
					mDolbyLength--;
				}
			}
			else
			{
				mDolbyState = 0;
			}
			break;
		default:
			mDolbyState = 0;
			break;
		}
		audioData++;
	}

	return dolbySize * 2;
}


//////////////////////////////////////////////


AJALabelValuePairs DolbyConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result;
	AJASystemInfo::append(result, "Capture Config");
	AJASystemInfo::append(result, "Device Specifier",	fDeviceSpec);
	AJASystemInfo::append(result, "Input Channel",		::NTV2ChannelToString(fInputChannel, inCompact));
	AJASystemInfo::append(result, "Input Source",		::NTV2InputSourceToString(fInputSource, inCompact));
	AJASystemInfo::append(result, "Pixel Format",		::NTV2FrameBufferFormatToString(fPixelFormat, inCompact));
	AJASystemInfo::append(result, "AutoCirc Frames",	fFrames.toString());
	AJASystemInfo::append(result, "MultiFormat Mode",	fDoMultiFormat ? "Y" : "N");
	AJASystemInfo::append(result, "Anc Capture File",	fAncDataFilePath);
	AJASystemInfo::append(result, "Audio Capture File",	fAudioDataFilePath);
	AJASystemInfo::append(result, "Dolby Capture File",	fDolbyDataFilePath);
	return result;
}


std::ostream & operator << (std::ostream & ioStrm,  const DolbyConfig & inObj)
{
	ioStrm	<< AJASystemInfo::ToString(inObj.Get());
	return ioStrm;
}
