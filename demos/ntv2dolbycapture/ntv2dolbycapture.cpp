/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolbycapture.cpp
	@brief		Implementation of NTV2DolbyCapture class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#include "ntv2dolbycapture.h"
#include "ntv2devicescanner.h"
#include "ajabase/system/process.h"
#include <fstream>	//	for ofstream

using namespace std;

#define NTV2_BUFFER_LOCK

#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define NTV2_ANCILLARYSIZE_MAX	(256 * 1024)


//////////////////////////////////////////////////////////////////////////////////////	NTV2DolbyCapture IMPLEMENTATION

NTV2DolbyCapture::NTV2DolbyCapture (const DolbyCaptureConfig & inConfig)
	:	mConsumerThread		(AJAThread()),
		mProducerThread		(AJAThread()),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mConfig				(inConfig),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mAudioSystem		(NTV2_AUDIOSYSTEM_1),
		mHostBuffers		(),
		mAVCircularBuffer	(),
		mGlobalQuit			(false)
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

	//	Restore some of the device's former state...
	if (!mConfig.fDoMultiFormat)
	{
		mDevice.ReleaseStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid()));
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
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID();						//	Keep the device ID handy, as it's used frequently
	if (!mDevice.features().CanDoCapture())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' is playback-only" << endl;  return AJA_STATUS_FEATURE;}

	if (!mDevice.features().CanDoFrameBufferFormat(mConfig.fPixelFormat))
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

	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);

	//	This demo permits input source and channel to be specified independently.
	if (!NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  !NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputChannel = NTV2_CHANNEL1;
	else if (!NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputChannel = ::NTV2InputSourceToChannel(mConfig.fInputSource);
	else if (NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel)  &&  !NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputSource = ::NTV2ChannelToInputSource(mConfig.fInputChannel, NTV2_IOKINDS_HDMI);
	if (!mDevice.features().CanDoInputSource(mConfig.fInputSource))
	{
		cerr	<< "## ERROR:  No such input '" << ::NTV2InputSourceToString(mConfig.fInputSource, /*compact?*/true)
				<< "' on '" << mDevice.GetDisplayName() << "'" << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	if (!mConfig.fAncDataFilePath.empty()  &&  !mDevice.features().CanDoHDMIAuxCapture())
		{cerr << "## ERROR: HDMI aux capture requested, but '" << mDevice.GetDisplayName() << "' has no HDMI aux extractors" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Set up the video and audio...
	status = SetupVideo();
	if (AJA_FAILURE(status))
		return status;

	status = SetupAudio();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the circular buffers, the device signal routing, and both playout and capture AutoCirculate...
	SetupHostBuffers();
	if (!RouteInputSignal())
		return AJA_STATUS_FAIL;

	mDolbyState = 0;

	#if defined(_DEBUG)
		cerr << mConfig;
		if (mDevice.IsRemote())
			cerr	<< "Device Description:  " << mDevice.GetDescription() << endl;
		cerr << endl;
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
	mVideoFormat = mDevice.GetInputVideoFormat(mConfig.fInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
	{	cerr << "## ERROR:  No input signal or unknown format on '" << ::NTV2InputSourceToString(mConfig.fInputSource, true)
				<< "'" << endl;
		return AJA_STATUS_NOINPUT;
	}
	if (!mDevice.features().CanDoVideoFormat(mVideoFormat))
	{	cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' cannot handle " << ::NTV2VideoFormatToString(mVideoFormat) << endl;
		return AJA_STATUS_UNSUPPORTED;	//	Device can't handle this format
	}
	CAPNOTE(::NTV2VideoFormatToString(mVideoFormat) << " detected on " << ::NTV2InputSourceToString(mConfig.fInputSource,true) << " on " << mDevice.GetDisplayName());
	mFormatDesc = NTV2FormatDescriptor(mVideoFormat, mConfig.fPixelFormat);

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
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	bool isProgressive = NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mVideoFormat);
	if (isProgressive)
		mDevice.AncSetFrameBufferSize(NTV2_ANCILLARYSIZE_MAX, 0);
	else
		mDevice.AncSetFrameBufferSize(NTV2_ANCILLARYSIZE_MAX / 2, NTV2_ANCILLARYSIZE_MAX / 2);

	ULWord F1AncSize(0), F2AncSize(0);
	ULWord	F1OffsetFromEnd(0), F2OffsetFromEnd(0);
	mDevice.ReadRegister(kVRegAncField1Offset, F1OffsetFromEnd);	//	# bytes from end of 8MB/16MB frame
	mDevice.ReadRegister(kVRegAncField2Offset, F2OffsetFromEnd);	//	# bytes from end of 8MB/16MB frame
	//	Based on the offsets, calculate the max anc capacity
	F1AncSize = F2OffsetFromEnd > F1OffsetFromEnd ? 0 : F1OffsetFromEnd - F2OffsetFromEnd;
	F2AncSize = F2OffsetFromEnd > F1OffsetFromEnd ? F2OffsetFromEnd - F1OffsetFromEnd : F2OffsetFromEnd;

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	const size_t audioBufferSize (NTV2_AUDIOSIZE_MAX);
	mHostBuffers.reserve(size_t(CIRCULAR_BUFFER_SIZE));
	while (mHostBuffers.size() < size_t(CIRCULAR_BUFFER_SIZE))
	{
		mHostBuffers.push_back(NTV2FrameData());
		NTV2FrameData & frameData(mHostBuffers.back());
		frameData.fVideoBuffer.Allocate(mFormatDesc.GetVideoWriteSize());
		frameData.fAudioBuffer.Allocate(audioBufferSize);
		frameData.fAncBuffer.Allocate(F1AncSize);
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


bool NTV2DolbyCapture::RouteInputSignal (void)
{

	NTV2LHIHDMIColorSpace inputColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
	if (NTV2_INPUT_SOURCE_IS_HDMI(mConfig.fInputSource))
		mDevice.GetHDMIInputColor (inputColorSpace, mConfig.fInputChannel);

	const bool isInputRGB (inputColorSpace == NTV2_LHIHDMIColorSpaceRGB);
	NTV2XptConnections connections;

	return CNTV2DemoCommon::GetInputRouting (connections, mConfig, isInputRGB)
		&&  mDevice.ApplySignalRoute(connections, !mConfig.fDoMultiFormat);

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
	pApp->ConsumeFrames();

}	//	ConsumerThreadStatic


void NTV2DolbyCapture::ConsumeFrames (void)
{
	CAPNOTE("Thread started");
	uint64_t ancTally(0);
	uint64_t audioTally(0);
	uint64_t dolbyTally(0);
	ofstream * pOFS(mConfig.fAncDataFilePath.empty() ? AJA_NULL : new ofstream(mConfig.fAncDataFilePath.c_str(), ios::binary));
	ofstream * pAFS(mConfig.fAudioDataFilePath.empty() ? AJA_NULL : new ofstream(mConfig.fAudioDataFilePath.c_str(), ios::binary));
	ofstream * pDFS(mConfig.fDolbyDataFilePath.empty() ? AJA_NULL : new ofstream(mConfig.fDolbyDataFilePath.c_str(), ios::binary));
	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		NTV2FrameData *	pFrameData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (pFrameData)
		{
			//	Do something useful with the frame data...
			//	. . .		. . .		. . .		. . .
			//		. . .		. . .		. . .		. . .
			//			. . .		. . .		. . .		. . .
			if (mConfig.fDoFrameStats && pFrameData->AncBuffer())
			{
				uint8_t* pData = (uint8_t*)pFrameData->AncBuffer().GetHostAddress(0);
				uint32_t i;
				for (i = 0; i < pFrameData->AncBufferSize(); i += 32)
				{
					if (pData[i] == 0xff)
							break;
				}
				uint32_t audioSize = RecoverAudio(pFrameData->AncBuffer(), pFrameData->AncBufferSize() /*pFrameData->NumCapturedAncBytes()*/, pFrameData->AudioBuffer());
				cout << "f1 size reg " << DEC(pFrameData->NumCapturedAncBytes()) << "  ffs " << DEC(i) << "  samples " << DEC(audioSize/4) << endl << flush;
			}

			if (mConfig.fDoFrameStats && /*!mProgressive &&*/ pFrameData->AncBuffer2())
			{
				uint8_t* pData = (uint8_t*)pFrameData->AncBuffer2().GetHostAddress(0);
				uint32_t i;
				for (i = 0; i < pFrameData->AncBuffer2Size(); i += 32)
				{
					if (pData[i] == 0xff)
							break;
				}
				uint32_t audioSize = RecoverAudio(pFrameData->AncBuffer2(), pFrameData->AncBuffer2Size() /*pFrameData->NumCapturedAnc2Bytes()*/, pFrameData->AudioBuffer());
				cout << "f2 size reg " << DEC(pFrameData->NumCapturedAnc2Bytes()) << "  ffs " << DEC(i) << "  samples " << DEC(audioSize/4) << endl << flush;
			}

			if (pOFS && pFrameData->AncBuffer())
			{
				if (pOFS  &&  !ancTally++)
					CAPNOTE("Writing raw anc to '" + mConfig.fAncDataFilePath + "'");
				pOFS->write(pFrameData->AncBuffer(), streamsize(pFrameData->NumCapturedAncBytes()));
				ancTally++;
				if (/*!mProgressive && */ pFrameData->AncBuffer2())
				{
					if (pOFS  &&  pFrameData->AncBuffer2())
						pOFS->write(pFrameData->AncBuffer2(), streamsize(pFrameData->NumCapturedAnc2Bytes()));
				}
			}

			if (pAFS && pFrameData->AncBuffer() && pFrameData->AudioBuffer())
			{
				if (pAFS  &&  !audioTally++)
					CAPNOTE("Writing raw audio to '" + mConfig.fAudioDataFilePath + "'");
				uint32_t audioSize = RecoverAudio(pFrameData->AncBuffer(), pFrameData->NumCapturedAncBytes(), pFrameData->AudioBuffer());
				pAFS->write(pFrameData->AudioBuffer(), streamsize(audioSize));
				audioTally++;
				if (/*!mProgressive &&*/ pFrameData->AncBuffer2())
				{
					audioSize = RecoverAudio(pFrameData->AncBuffer2(), pFrameData->NumCapturedAnc2Bytes(), pFrameData->AudioBuffer());
					pAFS->write(pFrameData->AudioBuffer(), streamsize(audioSize));
				}
			}

			if (pDFS && pFrameData->AncBuffer() && pFrameData->AudioBuffer())
			{
				if (pDFS  &&  !dolbyTally++)
					CAPNOTE("Writing dolby file to '" + mConfig.fDolbyDataFilePath + "'");
				uint32_t audioSize = RecoverAudio(pFrameData->AncBuffer(), pFrameData->NumCapturedAncBytes(), pFrameData->AudioBuffer());
				uint32_t dolbySize = RecoverDolby(pFrameData->AudioBuffer(), audioSize, pFrameData->AncBuffer());
				pDFS->write(pFrameData->AncBuffer(), streamsize(dolbySize));
				dolbyTally++;
				if (/*!mProgressive &&*/ pFrameData->AncBuffer2())
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
	CAPNOTE("Thread completed, will exit");

}	//	ConsumeFrames


//////////////////////////////////////////////////////////////////////////////////////////////////////////

//	This starts the capture (producer) thread
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
	NTV2DolbyCapture * pApp (reinterpret_cast <NTV2DolbyCapture*>(pContext));
	pApp->CaptureFrames ();

}	//	ProducerThreadStatic


void NTV2DolbyCapture::CaptureFrames (void)
{
	AUTOCIRCULATE_TRANSFER	inputXfer;	//	AutoCirculate input transfer info
	ULWord					acOptions (0), overruns(0);
	UWord					hdmiSpigot (UWord(::NTV2InputSourceToChannel(mConfig.fInputSource)));
	acOptions |= AUTOCIRCULATE_WITH_ANC;
	acOptions |= AUTOCIRCULATE_WITH_HDMIAUX;

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
	
	// The user can opt to INCLUDE only Audio Packets
	// AuxExtractGetDefaultPacketFilters() retrieves Audio Packet filters DIDs
	if (mConfig.fDoAudioFilter)
	{
		mDevice.AuxExtractSetFilterInclusionMode(hdmiSpigot, /*include?*/true);
		mDevice.AuxExtractSetPacketFilters(hdmiSpigot, mDevice.AuxExtractGetDefaultPacketFilters());
	}
	else
	{   //Otherwise, exclude only 00 Values (excludes nothing / shows all)
		mDevice.AuxExtractSetFilterInclusionMode(hdmiSpigot,/*include?*/false);
		NTV2DIDSet zeroSet;
		zeroSet.insert(0);
		mDevice.AuxExtractSetPacketFilters(hdmiSpigot, zeroSet);
	}

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
			NTV2FrameData *	pFrameData(mAVCircularBuffer.StartProduceNextBuffer());
			if (!pFrameData)
				continue;

			NTV2FrameData & frameData (*pFrameData);
			inputXfer.SetVideoBuffer (frameData.VideoBuffer(), frameData.VideoBufferSize());
			if (acStatus.WithCustomAnc())
				inputXfer.SetAncBuffers (frameData.AncBuffer(), frameData.AncBufferSize(),
										 frameData.AncBuffer2(), frameData.AncBuffer2Size());

			//	Transfer video/audio/anc from the device into our host buffers...
			mDevice.AutoCirculateTransfer (mConfig.fInputChannel, inputXfer);

			//	If capturing Anc, clear stale anc data from the anc buffers...
			if (acStatus.WithCustomAnc()  &&  frameData.AncBuffer())
			{	bool overrun(false);
				mDevice.AuxExtractGetBufferOverrun (hdmiSpigot, overrun);
				if (overrun)
					{overruns++;  CAPWARN(overruns << " aux overrun(s)");}
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
	AUTOCIRCULATE_STATUS status;
	mDevice.AutoCirculateGetStatus(mConfig.fInputChannel, status);
	outGoodFrames    = status.GetProcessedFrameCount();
	outDroppedFrames = status.GetDroppedFrameCount();
	outBufferLevel   = status.GetBufferLevel();
}

uint32_t NTV2DolbyCapture::RecoverAudio (const NTV2Buffer & inAncBuffer, const uint32_t ancSize, NTV2Buffer & outAudioBuffer)
{
	const uint8_t * pInAncData(inAncBuffer);
	uint8_t * pOutAudioData(outAudioBuffer);
	uint32_t audioSize(0);

	//	Extract first 16-bit stereo pair from the aux data
	for (uint32_t num(0);  num < ancSize / 32;  num++)
	{
		//	Audio data aux packet?
		if (pInAncData[0] == 0x02)
		{
			//	First sample present?
			if ((pInAncData[1] & 0x01))
			{
				//	First sample flat?
				if ((pInAncData[2] & 0x01))
				{
					*pOutAudioData++ = 0;
					*pOutAudioData++ = 0;
					*pOutAudioData++ = 0;
					*pOutAudioData++ = 0;
				}
				else
				{
					*pOutAudioData++ = pInAncData[4];
					*pOutAudioData++ = pInAncData[5];
					*pOutAudioData++ = pInAncData[7];
					*pOutAudioData++ = pInAncData[8];
				}
				audioSize += 4;
			}	//	if first sample present

			//	Only 2 channels?
			if ((pInAncData[1] & 0x10) == 0)
			{
				//	Second sample present?
				if ((pInAncData[1] & 0x02))
				{
					//	Second sample flat?
					if ((pInAncData[2] & 0x02))
					{
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
					}
					else
					{
						*pOutAudioData++ = pInAncData[11];
						*pOutAudioData++ = pInAncData[12];
						*pOutAudioData++ = pInAncData[14];
						*pOutAudioData++ = pInAncData[15];
					}
					audioSize += 4;
				}

				//	Third sample present?
				if ((pInAncData[1] & 0x04))
				{
					//	Third sample flat?
					if ((pInAncData[2] & 0x04))
					{
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
					}
					else
					{
						*pOutAudioData++ = pInAncData[18];
						*pOutAudioData++ = pInAncData[19];
						*pOutAudioData++ = pInAncData[21];
						*pOutAudioData++ = pInAncData[22];
					}
					audioSize += 4;
				}

				//	Fourth sample present?
				if ((pInAncData[1] & 0x08))
				{
					//	Fourth sample flat?
					if ((pInAncData[2] & 0x08))
					{
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
						*pOutAudioData++ = 0;
					}
					else
					{
						*pOutAudioData++ = pInAncData[25];
						*pOutAudioData++ = pInAncData[26];
						*pOutAudioData++ = pInAncData[28];
						*pOutAudioData++ = pInAncData[29];
					}
					audioSize += 4;
				}	//	if 4th sample present
			}	//	if only 2 channels
		}	//	if audio data aux packet
		pInAncData += 32;
	}	//	for loop
	return audioSize;
}	//	RecoverAudio

uint32_t NTV2DolbyCapture::RecoverDolby (const NTV2Buffer & inAudioBuffer, const uint32_t inAudioSize, NTV2Buffer & outDolbyBuffer)
{
	const uint16_t * pInAudioData(inAudioBuffer);
	uint16_t * pOutDolbyData(outDolbyBuffer);
	uint32_t dolbySize(0);

	//	Extract the dolby frames from the IEC61937 bursts...
	for (uint32_t cnt(0);  cnt < inAudioSize / 2;  cnt++)
	{
		switch (mDolbyState)
		{
			//	Find IEC61937 burst preamble
			case 0:		mDolbyState = (*pInAudioData == 0xf872) ? 1 : 0;
						break;

			case 1:		mDolbyState = (*pInAudioData == 0x4e1f) ? 2 : 0;
						break;

			//	Check dolby code
			case 2:		mDolbyState = ((*pInAudioData & 0xff) == 0x15) ? 3 : 0;
						break;

			//	Get burst length
			case 3:		mDolbyLength = uint32_t(*pInAudioData / 2);
						mDolbyState = 4;
						break;

			//	Copy dolby samples
			case 4:		if (mDolbyLength)
						{
							if (dolbySize < outDolbyBuffer.GetByteCount() / 2)
							{
								//	Endian swap the data...
								*pOutDolbyData = (*pInAudioData >> 8) & 0xff;
								*pOutDolbyData |= (*pInAudioData & 0xff) << 8;
								pOutDolbyData++;
								dolbySize++;
								mDolbyLength--;
							}
						}
						else
							mDolbyState = 0;
						break;

			default:	mDolbyState = 0;
						break;
		}	//	switch
		pInAudioData++;
	}	//	for loop
	return dolbySize * 2;
}


//////////////////////////////////////////////


AJALabelValuePairs DolbyCaptureConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result(CaptureConfig::Get(inCompact));
	AJASystemInfo::append(result,	"Audio Capture File",	fAudioDataFilePath.empty() ? "---" : fAudioDataFilePath);
	AJASystemInfo::append(result,	"Dolby Capture File",	fDolbyDataFilePath.empty() ? "---" : fDolbyDataFilePath);
	return result;
}


std::ostream & operator << (std::ostream & ioStrm,  const DolbyCaptureConfig & inObj)
{
	ioStrm	<< AJASystemInfo::ToString(inObj.Get());
	return ioStrm;
}
