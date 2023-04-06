/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2capture.cpp
	@brief		Implementation of NTV2Capture class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

//#define AJA_RAW_AUDIO_RECORD	//	Uncomment to record captured raw audio into binary data file
//#define AJA_WAV_AUDIO_RECORD	//	Uncomment to record captured audio into WAV file
#include "ntv2capture.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#include "ajabase/system/process.h"
#include <iterator>	//	for inserter
#include <fstream>	//	for ofstream

using namespace std;

#define NTV2_BUFFER_LOCK


//////////////////////////////////////////////////////////////////////////////////////	NTV2Capture IMPLEMENTATION

NTV2Capture::NTV2Capture (const CaptureConfig & inConfig)
	:	mConsumerThread		(AJAThread()),
		mProducerThread		(AJAThread()),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mConfig				(inConfig),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mAudioSystem		(inConfig.fWithAudio ? NTV2_AUDIOSYSTEM_1 : NTV2_AUDIOSYSTEM_INVALID),
		mHostBuffers		(),
		mAVCircularBuffer	(),
		mGlobalQuit			(false)
{
}	//	constructor


NTV2Capture::~NTV2Capture ()
{
	//	Stop my capture and consumer threads, then destroy them...
	Quit();

	//	Unsubscribe from VBI events...
	mDevice.UnsubscribeInputVerticalEvent(mConfig.fInputChannel);
	mDevice.UnsubscribeOutputVerticalEvent(NTV2_CHANNEL1);

}	//	destructor


void NTV2Capture::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mConsumerThread.Active())
		AJATime::Sleep(10);

	while (mProducerThread.Active())
		AJATime::Sleep(10);

	//	Restore some of the device's former state...
	if (!mConfig.fDoMultiFormat)
	{
		mDevice.ReleaseStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid()));
		mDevice.SetEveryFrameServices(mSavedTaskMode);		//	Restore prior task mode
	}

}	//	Quit


AJAStatus NTV2Capture::Init (void)
{
	AJAStatus status (AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

	if (!mDevice.IsDeviceReady())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID();						//	Keep the device ID handy, as it's used frequently
	const bool isKonaHDMI (::NTV2DeviceGetNumHDMIVideoInputs(mDeviceID) > 1);
	const NTV2IOKinds inputType (isKonaHDMI ? NTV2_IOKINDS_HDMI : NTV2_IOKINDS_SDI);
	if (!::NTV2DeviceCanDoCapture(mDeviceID))
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' is playback-only" << endl;  return AJA_STATUS_FEATURE;}

	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.fPixelFormat))
	{	cerr	<< "## ERROR:  '" << mDevice.GetDisplayName() << "' doesn't support '"
				<< ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat, true) << "' ("
				<< ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat, false) << ", " << DEC(mConfig.fPixelFormat) << ")" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetStreamingApplication (appSignature, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);			//	Save the current device state
	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire '" << mDevice.GetDisplayName() << "' because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Another app is using the device
		}
		mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current state before we change it
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Prevent interference from AJA retail services

	if (::NTV2DeviceCanDoMultiFormat(mDeviceID))
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);

	//	This demo permits input source and channel to be specified independently.
	if (!NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  !NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputChannel = NTV2_CHANNEL1;	//	Neither -i or -c specified:  default to Ch1
	if (!NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputChannel = ::NTV2InputSourceToChannel(mConfig.fInputSource);	//	Only -i specified:  use that channel
	else if (NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  !NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputSource = ::NTV2ChannelToInputSource ( mConfig.fInputChannel, inputType);	//	Only -c specified: use that input source
	//	On KonaHDMI, map specified SDI input to equivalent HDMI input...
	if (isKonaHDMI  &&  NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource))
		mConfig.fInputSource = ::NTV2ChannelToInputSource(::NTV2InputSourceToChannel(mConfig.fInputSource), inputType);
	if (!::NTV2DeviceCanDoInputSource(mDeviceID, mConfig.fInputSource))
	{
		cerr	<< "## ERROR:  No such input '" << ::NTV2InputSourceToString(mConfig.fInputSource, /*compact?*/true)
				<< "' on '" << mDevice.GetDisplayName() << "'" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	if (mConfig.fWithAnc  &&  !::NTV2DeviceCanDoCustomAnc(mDeviceID))
		{cerr << "## ERROR: Anc capture requested, but '" << mDevice.GetDisplayName() << "' has no anc extractors";  return AJA_STATUS_UNSUPPORTED;}

	//	Set up the video and audio...
	status = SetupVideo();
	if (AJA_FAILURE(status))
		return status;

	if (mConfig.fWithAudio)
		status = SetupAudio();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the circular buffers, the device signal routing, and both playout and capture AutoCirculate...
	SetupHostBuffers();
	if (!RouteInputSignal())
		return AJA_STATUS_FAIL;

	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#endif	//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Capture::SetupVideo (void)
{
	//	Enable the FrameStore we intend to use...
	mDevice.EnableChannel(mConfig.fInputChannel);

	//	Enable and subscribe to VBIs (critical on Windows)...
	mDevice.EnableInputInterrupt(mConfig.fInputChannel);
	mDevice.SubscribeInputVerticalEvent(mConfig.fInputChannel);
	mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL1);

	//	If the device supports bi-directional SDI and the requested input is SDI,
	//	ensure the SDI connector(s) are configured to receive...
	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID) && NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource))
	{
		mDevice.SetSDITransmitEnable (mConfig.fInputChannel, false);	//	Set SDI connector(s) to receive
		mDevice.WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 10);		//	Wait 10 VBIs to allow reciever to lock
	}

	//	Determine the input video signal format...
	mVideoFormat = mDevice.GetInputVideoFormat(mConfig.fInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		{cerr << "## ERROR:  No input signal or unknown format" << endl;  return AJA_STATUS_NOINPUT;}
	if (!::NTV2DeviceCanDoVideoFormat(mDeviceID, mVideoFormat))
	{
		cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' cannot handle " << ::NTV2VideoFormatToString(mVideoFormat) << endl;
		return AJA_STATUS_UNSUPPORTED;	//	Device can't handle this format
	}
	if (NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat) || NTV2_IS_QUAD_QUAD_FORMAT(mVideoFormat))
	{
		cerr << "## ERROR:  'ntv2capture' cannot handle " << ::NTV2VideoFormatToString(mVideoFormat)
            << " input signal -- try 'ntv2capture4k' or 'ntv2capture8k' demo" << endl;
		return AJA_STATUS_UNSUPPORTED;	//	This demo can't handle this format
	}
	CAPNOTE(::NTV2VideoFormatToString(mVideoFormat) << " detected on " << ::NTV2InputSourceToString(mConfig.fInputSource,true) << " on " << mDevice.GetDisplayName());
	mFormatDesc = NTV2FormatDescriptor(mVideoFormat, mConfig.fPixelFormat);

	//	Setting SDI output clock timing/reference is unimportant for capture-only apps...
	if (!mConfig.fDoMultiFormat)						//	...if not sharing the device...
		mDevice.SetReference(NTV2_REFERENCE_FREERUN);	//	...let it free-run

	//	Set the device video format to whatever was detected at the input...
	mDevice.SetVideoFormat (mVideoFormat, false, false, mConfig.fInputChannel);
	mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fInputChannel);	//	Disable VANC

	//	Set the frame buffer pixel format for the FrameStore to be used on the device...
	mDevice.SetFrameBufferFormat (mConfig.fInputChannel, mConfig.fPixelFormat);
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2Capture::SetupAudio (void)
{
	//	In multiformat mode, base the audio system on the channel...
	if (mConfig.fDoMultiFormat)
		if (::NTV2DeviceGetNumAudioSystems(mDeviceID) > 1)
			if (UWord(mConfig.fInputChannel) < ::NTV2DeviceGetNumAudioSystems(mDeviceID))
				mAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fInputChannel);

	NTV2AudioSystemSet audSystems (::NTV2MakeAudioSystemSet (mAudioSystem, 1));
	CNTV2DemoCommon::ConfigureAudioSystems (mDevice, mConfig, audSystems);
	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


void NTV2Capture::SetupHostBuffers (void)
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag(&mGlobalQuit);

	ULWord F1AncSize(0), F2AncSize(0);
	if (mConfig.fWithAnc)
	{	//	Use the max anc size stipulated by the AncFieldOffset VReg values...
		ULWord	F1OffsetFromEnd(0), F2OffsetFromEnd(0);
		mDevice.ReadRegister(kVRegAncField1Offset, F1OffsetFromEnd);	//	# bytes from end of 8MB/16MB frame
		mDevice.ReadRegister(kVRegAncField2Offset, F2OffsetFromEnd);	//	# bytes from end of 8MB/16MB frame
		//	Based on the offsets, calculate the max anc capacity
		F1AncSize = F2OffsetFromEnd > F1OffsetFromEnd ? 0 : F1OffsetFromEnd - F2OffsetFromEnd;
		F2AncSize = F2OffsetFromEnd > F1OffsetFromEnd ? F2OffsetFromEnd - F1OffsetFromEnd : F2OffsetFromEnd;
	}

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	const size_t audioBufferSize (NTV2_AUDIOSIZE_MAX);
	mHostBuffers.reserve(size_t(CIRCULAR_BUFFER_SIZE));
	while (mHostBuffers.size() < size_t(CIRCULAR_BUFFER_SIZE))
	{
		mHostBuffers.push_back(NTV2FrameData());
		NTV2FrameData & frameData(mHostBuffers.back());
		frameData.fVideoBuffer.Allocate(mFormatDesc.GetVideoWriteSize());
		frameData.fAudioBuffer.Allocate(NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem) ? audioBufferSize : 0);
		frameData.fAncBuffer.Allocate(F1AncSize);
		frameData.fAncBuffer2.Allocate(F2AncSize);
		mAVCircularBuffer.Add(&frameData);

#ifdef NTV2_BUFFER_LOCK
		// Page lock the memory
		if (frameData.fVideoBuffer)
			mDevice.DMABufferLock(frameData.fVideoBuffer, true);
		if (frameData.fAudioBuffer)
			mDevice.DMABufferLock(frameData.fAudioBuffer, true);
		if (frameData.fAncBuffer)
			mDevice.DMABufferLock(frameData.fAncBuffer, true);
#endif
	}	//	for each NTV2FrameData

}	//	SetupHostBuffers


bool NTV2Capture::RouteInputSignal (void)
{
	NTV2LHIHDMIColorSpace inputColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
	if (NTV2_INPUT_SOURCE_IS_HDMI(mConfig.fInputSource))
		mDevice.GetHDMIInputColor (inputColorSpace, mConfig.fInputChannel);

	const bool isInputRGB (inputColorSpace == NTV2_LHIHDMIColorSpaceRGB);
	NTV2XptConnections connections;

	return CNTV2DemoCommon::GetInputRouting (connections, mConfig, isInputRGB)
		&&  mDevice.ApplySignalRoute(connections, !mConfig.fDoMultiFormat);

}	//	RouteInputSignal


AJAStatus NTV2Capture::Run (void)
{
	//	Start the playout and capture threads...
	StartConsumerThread();
	StartProducerThread();
	return AJA_STATUS_SUCCESS;

}	//	Run


//////////////////////////////////////////////////////////////////////////////////////////////////////////

//	This starts the consumer thread
void NTV2Capture::StartConsumerThread (void)
{
	//	Create and start the consumer thread...
	mConsumerThread.Attach(ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The consumer thread function
void NTV2Capture::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Capture instance pointer from the pContext parameter,
	//	then call its ConsumeFrames method...
	NTV2Capture *	pApp (reinterpret_cast<NTV2Capture*>(pContext));
	pApp->ConsumeFrames();

}	//	ConsumerThreadStatic


void NTV2Capture::ConsumeFrames (void)
{
	CAPNOTE("Thread started");
	AJA_NTV2_AUDIO_RECORD_BEGIN	//	Active when AJA_RAW_AUDIO_RECORD or AJA_WAV_AUDIO_RECORD defined
	uint64_t ancTally(0);
	ofstream * pOFS(mConfig.fAncDataFilePath.empty() ? AJA_NULL : new ofstream(mConfig.fAncDataFilePath.c_str(), ios::binary));
	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		NTV2FrameData *	pFrameData(mAVCircularBuffer.StartConsumeNextBuffer());
		if (pFrameData)
		{
			//	Do something useful with the frame data...
			//	. . .		. . .		. . .		. . .
			//		. . .		. . .		. . .		. . .
			//			. . .		. . .		. . .		. . .
			AJA_NTV2_AUDIO_RECORD_DO	//	Active when AJA_RAW_AUDIO_RECORD or AJA_WAV_AUDIO_RECORD defined
			if (pOFS  &&  !ancTally++)
				cerr << "Writing raw anc to '" << mConfig.fAncDataFilePath << "', "
					<< DEC(pFrameData->AncBufferSize() + pFrameData->AncBuffer2Size())
					<< " bytes per frame" << endl;
			if (pOFS  &&  pFrameData->AncBuffer())
				pOFS->write(pFrameData->AncBuffer(), streamsize(pFrameData->AncBufferSize()));
			if (pOFS  &&  pFrameData->AncBuffer2())
				pOFS->write(pFrameData->AncBuffer2(), streamsize(pFrameData->AncBuffer2Size()));

			//	Now release and recycle the buffer...
			mAVCircularBuffer.EndConsumeNextBuffer();
		}	//	if pFrameData
	}	//	loop til quit signaled
	if (pOFS)
		{delete pOFS; cerr << "Wrote " << DEC(ancTally) << " frames of raw anc data" << endl;}
	AJA_NTV2_AUDIO_RECORD_END	//	Active when AJA_RAW_AUDIO_RECORD or AJA_WAV_AUDIO_RECORD defined
	CAPNOTE("Thread completed, will exit");

}	//	ConsumeFrames


//////////////////////////////////////////////////////////////////////////////////////////////////////////

//	This starts the capture (producer) thread
void NTV2Capture::StartProducerThread (void)
{
	//	Create and start the capture thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


//	The capture thread function
void NTV2Capture::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Capture instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2Capture * pApp (reinterpret_cast<NTV2Capture*>(pContext));
	pApp->CaptureFrames ();

}	//	ProducerThreadStatic


void NTV2Capture::CaptureFrames (void)
{
	AUTOCIRCULATE_TRANSFER	inputXfer;	//	AutoCirculate input transfer info
	NTV2AudioChannelPairs	nonPcmPairs, oldNonPcmPairs;
	ULWord					acOptions (AUTOCIRCULATE_WITH_RP188), overruns(0);
	UWord					sdiSpigot (UWord(::NTV2InputSourceToChannel(mConfig.fInputSource)));
	if (mConfig.fWithAnc)
		acOptions |= AUTOCIRCULATE_WITH_ANC;

	CAPNOTE("Thread started");
	//	Initialize and start capture AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fInputChannel);	//	Just in case
	if (!mDevice.AutoCirculateInitForInput (mConfig.fInputChannel,		//	primary channel
											mConfig.fFrames.count(),	//	numFrames (zero if specifying range)
											mAudioSystem,				//	audio system (if any)
											acOptions,					//	AutoCirculate options
											1,							//	numChannels to gang
											mConfig.fFrames.firstFrame(), mConfig.fFrames.lastFrame()))
		mGlobalQuit = true;
	if (!mGlobalQuit  &&  !mDevice.AutoCirculateStart(mConfig.fInputChannel))
		mGlobalQuit = true;

	//	Ingest frames til Quit signaled...
	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS acStatus;
		mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, acStatus);

		if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
		{
			//	At this point, there's at least one fully-formed frame available in the device's
			//	frame buffer to transfer to the host. Reserve an NTV2FrameData to "produce", and
			//	use it in the next transfer from the device...
			NTV2FrameData *	pFrameData (mAVCircularBuffer.StartProduceNextBuffer());
			if (!pFrameData)
				continue;

			NTV2FrameData & frameData (*pFrameData);
			inputXfer.SetVideoBuffer (frameData.VideoBuffer(), frameData.VideoBufferSize());
			if (acStatus.WithAudio())
				inputXfer.SetAudioBuffer (frameData.AudioBuffer(), frameData.AudioBufferSize());
			if (acStatus.WithCustomAnc())
				inputXfer.SetAncBuffers (frameData.AncBuffer(), frameData.AncBufferSize(),
										 frameData.AncBuffer2(), frameData.AncBuffer2Size());

			//	Transfer video/audio/anc from the device into our host buffers...
			mDevice.AutoCirculateTransfer (mConfig.fInputChannel, inputXfer);

			//	Remember the actual amount of audio captured...
			if (acStatus.WithAudio())
				frameData.fNumAudioBytes = inputXfer.GetCapturedAudioByteCount();

			//	If capturing Anc, clear stale anc data from the anc buffers...
			if (acStatus.WithCustomAnc()  &&  frameData.AncBuffer())
			{	bool overrun(false);
				mDevice.AncExtractGetBufferOverrun (sdiSpigot, overrun);
				if (overrun)
					{overruns++;  CAPWARN(overruns << " anc overrun(s)");}
				frameData.fNumAncBytes = inputXfer.GetCapturedAncByteCount(/*isF2*/false);
				NTV2Buffer stale (frameData.fAncBuffer.GetHostAddress(frameData.fNumAncBytes),
									frameData.fAncBuffer.GetByteCount() - frameData.fNumAncBytes);
				stale.Fill(uint8_t(0));
			}
			if (acStatus.WithCustomAnc()  &&  frameData.AncBuffer2())
			{
				frameData.fNumAnc2Bytes = inputXfer.GetCapturedAncByteCount(/*isF2*/true);
				NTV2Buffer stale (frameData.fAncBuffer2.GetHostAddress(frameData.fNumAnc2Bytes),
									frameData.fAncBuffer2.GetByteCount() - frameData.fNumAnc2Bytes);
				stale.Fill(uint8_t(0));
			}

			//	Grab all valid timecodes that were captured...
			inputXfer.GetInputTimeCodes (frameData.fTimecodes, mConfig.fInputChannel, /*ValidOnly*/ true);

			if (acStatus.WithAudio())
				//	Look for PCM/NonPCM changes in the audio stream...
				if (mDevice.GetInputAudioChannelPairsWithoutPCM (mConfig.fInputChannel, nonPcmPairs))
				{
					NTV2AudioChannelPairs becomingNonPCM, becomingPCM;
					set_difference (oldNonPcmPairs.begin(), oldNonPcmPairs.end(),  nonPcmPairs.begin(), nonPcmPairs.end(),  inserter(becomingPCM, becomingPCM.begin()));
					set_difference (nonPcmPairs.begin(), nonPcmPairs.end(),  oldNonPcmPairs.begin(), oldNonPcmPairs.end(),  inserter(becomingNonPCM, becomingNonPCM.begin()));
					if (!becomingNonPCM.empty())
						CAPNOTE("Audio channel pair(s) '" << becomingNonPCM << "' now non-PCM");
					if (!becomingPCM.empty())
						CAPNOTE("Audio channel pair(s) '" << becomingPCM << "' now PCM");
					oldNonPcmPairs = nonPcmPairs;
				}

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

		//	Log SDI input CRC/VPID/TRS errors...
		if (::NTV2DeviceCanDoSDIErrorChecks(mDeviceID) && NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource))
		{
			NTV2SDIInStatistics	sdiStats;
			NTV2SDIInputStatus	inputStatus;
			if (mDevice.ReadSDIStatistics(sdiStats))
			{
				sdiStats.GetSDIInputStatus(inputStatus, UWord(::GetIndexForNTV2InputSource(mConfig.fInputSource)));
				if (!inputStatus.mLocked)
					CAPWARN(inputStatus);
			}
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fInputChannel);
	CAPNOTE("Thread completed, will exit");

}	//	CaptureFrames


void NTV2Capture::GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel)
{
	AUTOCIRCULATE_STATUS status;
	mDevice.AutoCirculateGetStatus(mConfig.fInputChannel, status);
	outGoodFrames    = status.GetProcessedFrameCount();
	outDroppedFrames = status.GetDroppedFrameCount();
	outBufferLevel   = status.GetBufferLevel();
}
