/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streamplayer.cpp
	@brief		Implementation of NTV2StreamPlayer class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2streamplayer.h"
#include "ntv2debug.h"
#include "ntv2devicescanner.h"
#include "ntv2testpatterngen.h"
#include "ajabase/common/timebase.h"
#include "ajabase/system/process.h"
#include "ajaanc/includes/ancillarydata_hdr_sdr.h"
#include "ajaanc/includes/ancillarydata_hdr_hdr10.h"
#include "ajaanc/includes/ancillarydata_hdr_hlg.h"
#include <fstream>	//	For ifstream

using namespace std;

#define NTV2_BUFFER_LOCKING		//	Define this to pre-lock video/audio buffers in kernel

//	Convenience macros for EZ logging:
#define	TCFAIL(_expr_)	AJA_sERROR  (AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCWARN(_expr_)	AJA_sWARNING(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCNOTE(_expr_)	AJA_sNOTICE	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCINFO(_expr_)	AJA_sINFO	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCDBG(_expr_)	AJA_sDEBUG	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)

/**
	@brief	The maximum number of bytes of ancillary data that can be transferred for a single field.
			Each driver instance sets this maximum to the 8K default at startup.
			It can be changed at runtime, so it's sampled and reset in SetUpVideo.
**/
static ULWord	gAncMaxSizeBytes (NTV2_ANCSIZE_MAX);	//	Max per-frame anc buffer size, in bytes

/**
	@brief	The maximum number of bytes of 48KHz audio that can be transferred for a single frame.
			Worst case, assuming 16 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			48,000 samples per second requires 3,204 samples x 4 bytes/sample x 16 = 205,056 bytes
			201K min will suffice, with 768 bytes to spare
			But it could be more efficient for page-aligned (and page-locked) memory to round to 256K.
**/
static const uint32_t	gAudMaxSizeBytes (256 * 1024);	//	Max per-frame audio buffer size, in bytes

static const bool		BUFFER_PAGE_ALIGNED	(true);

//	Audio tone generator data
static const double		gFrequencies []	=	{250.0, 500.0, 1000.0, 2000.0};
static const ULWord		gNumFrequencies		(sizeof(gFrequencies) / sizeof(double));
static const double		gAmplitudes []	=	{	0.10, 0.15,		0.20, 0.25,		0.30, 0.35,		0.40, 0.45,		0.50, 0.55,		0.60, 0.65,		0.70, 0.75,		0.80, 0.85,
												0.85, 0.80,		0.75, 0.70,		0.65, 0.60,		0.55, 0.50,		0.45, 0.40,		0.35, 0.30,		0.25, 0.20,		0.15, 0.10};


NTV2StreamPlayer::NTV2StreamPlayer (const PlayerConfig & inConfig)
	:	mConfig				(inConfig),
		mConsumerThread		(),
		mProducerThread		(),
		mDevice				(),
		mDeviceID			(DEVICE_ID_INVALID),
		mSavedTaskMode		(NTV2_TASK_MODE_INVALID),
		mCurrentFrame		(0),
		mCurrentSample		(0),
		mToneFrequency		(440.0),
		mAudioSystem		(NTV2_AUDIOSYSTEM_INVALID),
		mFormatDesc			(),
		mGlobalQuit			(false),
		mTCBurner			(),
		mHostBuffers		(),
		mFrameDataRing		(),
		mTestPatRasters		()
{
}


NTV2StreamPlayer::~NTV2StreamPlayer (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit();

	mDevice.UnsubscribeOutputVerticalEvent(mConfig.fOutputChannel);	//	Unsubscribe from output VBI event
}	//	destructor


void NTV2StreamPlayer::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mProducerThread.Active())
		AJATime::Sleep(10);

	while (mConsumerThread.Active())
		AJATime::Sleep(10);

#if defined(NTV2_BUFFER_LOCKING)
	mDevice.DMABufferUnlockAll();
#endif	//	NTV2_BUFFER_LOCKING
	if (!mConfig.fDoMultiFormat  &&  mDevice.IsOpen())
	{
		mDevice.ReleaseStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid()));
		if (NTV2_IS_VALID_TASK_MODE(mSavedTaskMode))
			mDevice.SetEveryFrameServices(mSavedTaskMode);		//	Restore prior task mode
	}
}	//	Quit


AJAStatus NTV2StreamPlayer::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}
	mDeviceID = mDevice.GetDeviceID();	//	Keep this ID handy -- it's used frequently

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mDevice.GetDisplayName() << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' is capture-only" << endl;  return AJA_STATUS_FEATURE;}

	const UWord maxNumChannels (mDevice.features().GetNumFrameStores());

	//	Beware -- some older devices (e.g. Corvid1) can only output from FrameStore 2...
	if ((mConfig.fOutputChannel == NTV2_CHANNEL1) && (!mDevice.features().CanDoFrameStore1Display()))
		mConfig.fOutputChannel = NTV2_CHANNEL2;
	if (UWord(mConfig.fOutputChannel) >= maxNumChannels)
	{
		cerr	<< "## ERROR:  '" << mDevice.GetDisplayName() << "' can't use Ch" << DEC(mConfig.fOutputChannel+1)
				<< " -- only supports Ch1" << (maxNumChannels > 1  ?  string("-Ch") + string(1, char(maxNumChannels+'0'))  :  "") << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	if (!mConfig.fDoMultiFormat)
	{
		mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current task mode
		if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
			return AJA_STATUS_BUSY;		//	Device is in use by another app -- fail
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Set OEM service level

	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);
	else
		mConfig.fDoMultiFormat = false;

	//	Set up the video and audio...
	status = SetUpVideo();
	if (AJA_FAILURE(status))
		return status;
	status = mConfig.WithAudio() ? SetUpAudio() : AJA_STATUS_SUCCESS;
	if (AJA_FAILURE(status))
		return status;

	//	Set up the circular buffers, and the test pattern buffers...
	status = SetUpHostBuffers();
	if (AJA_FAILURE(status))
		return status;
	status = SetUpTestPatternBuffers();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the device signal routing...
	if (!RouteOutputSignal())
		return AJA_STATUS_FAIL;

	//	Lastly, prepare my AJATimeCodeBurn instance...
	if (!mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat(mConfig.fPixelFormat), mFormatDesc.numPixels, mFormatDesc.numLines))
		{cerr << "## ERROR:  RenderTimeCodeFont failed for:  " << mFormatDesc << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Ready to go...
	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#endif	//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2StreamPlayer::SetUpVideo (void)
{
	//	Configure the device to output the requested video format...
 	if (mConfig.fVideoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_BAD_PARAM;
	if (!mDevice.features().CanDoVideoFormat(mConfig.fVideoFormat))
	{	cerr	<< "## ERROR:  '" << mDevice.GetDisplayName() << "' doesn't support "
				<< ::NTV2VideoFormatToString(mConfig.fVideoFormat) << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	if (!mDevice.features().CanDoFrameBufferFormat(mConfig.fPixelFormat))
	{	cerr	<< "## ERROR: '" << mDevice.GetDisplayName() << "' doesn't support "
				<< ::NTV2FrameBufferFormatString(mConfig.fPixelFormat) << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	//	This demo doesn't playout dual-link RGB over SDI -- only YCbCr.
	//	Check that this device has a CSC to convert RGB to YUV...
	if (::IsRGBFormat(mConfig.fPixelFormat))	//	If RGB FBF...
		if (UWord(mConfig.fOutputChannel) > mDevice.features().GetNumCSCs())	//	No CSC for this channel?
			{cerr << "## ERROR: No CSC for channel " << DEC(mConfig.fOutputChannel+1) << " to convert RGB pixel format" << endl;
				return AJA_STATUS_UNSUPPORTED;}

	if (!mDevice.features().CanDo3GLevelConversion() && mConfig.fDoABConversion && ::IsVideoFormatA(mConfig.fVideoFormat))
		mConfig.fDoABConversion = false;
	if (mConfig.fDoABConversion)
		mDevice.SetSDIOutLevelAtoLevelBConversion (mConfig.fOutputChannel, mConfig.fDoABConversion);

	//	Keep the raster description handy...
	mFormatDesc = NTV2FormatDescriptor(mConfig.fVideoFormat, mConfig.fPixelFormat);
	if (!mFormatDesc.IsValid())
		return AJA_STATUS_FAIL;

	//	Turn on the FrameStore (to read frame buffer memory and transmit video)...
	mDevice.EnableChannel(mConfig.fOutputChannel);
	mDevice.SetMode(mConfig.fOutputChannel, NTV2_MODE_DISPLAY);

	//	This demo assumes VANC is disabled...
	mDevice.SetVANCMode(NTV2_VANCMODE_OFF, mConfig.fOutputChannel);
	mDevice.SetVANCShiftMode(mConfig.fOutputChannel, NTV2_VANCDATA_NORMAL);

	//	Set the FrameStore video format...
	mDevice.SetVideoFormat (mConfig.fVideoFormat, false, false, mConfig.fOutputChannel);

	//	Set the frame buffer pixel format for the device FrameStore...
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);

	//	The output interrupt is Enabled by default, but on some platforms, you must subscribe to it
	//	in order to be able to wait on its event/semaphore...
	mDevice.SubscribeOutputVerticalEvent (mConfig.fOutputChannel);

	//	Check if HDR anc is permissible...
	if (IS_KNOWN_AJAAncDataType(mConfig.fTransmitHDRType)  &&  !mDevice.features().CanDoCustomAnc())
		{cerr << "## WARNING:  HDR Anc requested, but device can't do custom anc" << endl;
			mConfig.fTransmitHDRType = AJAAncDataType_Unknown;}

	//	Get current per-field maximum Anc buffer size...
	if (!mDevice.GetAncRegionOffsetFromBottom (gAncMaxSizeBytes, NTV2_AncRgn_Field2))
		gAncMaxSizeBytes = NTV2_ANCSIZE_MAX;

	//	Set output clock reference...
	mDevice.SetReference(mDevice.features().CanDo2110() ? NTV2_REFERENCE_SFP1_PTP : NTV2_REFERENCE_FREERUN);

	//	At this point, video setup is complete (except for widget signal routing).
	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2StreamPlayer::SetUpAudio (void)
{
	uint16_t numAudioChannels (mDevice.features().GetMaxAudioChannels());

	//	If there are 2048 pixels on a line instead of 1920, reduce the number of audio channels
	//	This is because HANC is narrower, and has space for only 8 channels
	if (NTV2_IS_2K_1080_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  numAudioChannels > 8)
		numAudioChannels = 8;

	mAudioSystem = NTV2_AUDIOSYSTEM_1;										//	Use NTV2_AUDIOSYSTEM_1...
	if (mDevice.features().GetNumAudioSystems() > 1)						//	...but if the device has more than one audio system...
		mAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fOutputChannel);	//	...base it on the channel
	//	However, there are a few older devices that have only 1 audio system,
	//	yet 2 frame stores (or must use channel 2 for playout)...
	if (!mDevice.features().CanDoFrameStore1Display())
		mAudioSystem = NTV2_AUDIOSYSTEM_1;

	mDevice.SetNumberAudioChannels (numAudioChannels, mAudioSystem);
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set the SDI output audio embedders to embed audio samples from the output of mAudioSystem...
	mDevice.SetSDIOutputAudioSystem (mConfig.fOutputChannel, mAudioSystem);
	mDevice.SetSDIOutputDS2AudioSystem (mConfig.fOutputChannel, mAudioSystem);

	//	If the last app using the device left it in E-E mode (input passthru), then loopback
	//	must be disabled --- otherwise the output audio will be pass-thru SDI input audio...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	if (!mConfig.fDoMultiFormat  &&  mDevice.features().GetNumHDMIVideoOutputs())
	{
		mDevice.SetHDMIOutAudioRate(NTV2_AUDIO_48K);
		mDevice.SetHDMIOutAudioFormat(NTV2_AUDIO_FORMAT_LPCM);
		mDevice.SetHDMIOutAudioSource8Channel(NTV2_AudioChannel1_8, mAudioSystem);
	}

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio


AJAStatus NTV2StreamPlayer::SetUpHostBuffers (void)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Let my circular buffer know when it's time to quit...
	mFrameDataRing.SetAbortFlag (&mGlobalQuit);

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());		//	Make a new NTV2FrameData...
		NTV2FrameData & frameData(mHostBuffers.back());	//	...and get a reference to it

		//	Allocate a page-aligned video buffer
		if (mConfig.WithVideo())
			if (!frameData.fVideoBuffer.Allocate (mFormatDesc.GetTotalBytes(), BUFFER_PAGE_ALIGNED))
			{
				PLFAIL("Failed to allocate " << xHEX0N(mFormatDesc.GetTotalBytes(),8) << "-byte video buffer");
				return AJA_STATUS_MEMORY;
			}
		if (frameData.fVideoBuffer)
		{
			frameData.fVideoBuffer.Fill(ULWord(0));
            mDevice.DMABufferLock(frameData.fVideoBuffer, true);
		}

		//	Allocate a page-aligned audio buffer (if transmitting audio)
		if (mConfig.WithAudio())
			if (!frameData.fAudioBuffer.Allocate (gAudMaxSizeBytes, BUFFER_PAGE_ALIGNED))
			{
				PLFAIL("Failed to allocate " << xHEX0N(gAudMaxSizeBytes,8) << "-byte audio buffer");
				return AJA_STATUS_MEMORY;
			}
		if (frameData.fAudioBuffer)
		{
			frameData.fAudioBuffer.Fill(ULWord(0));
			mDevice.DMABufferLock(frameData.fAudioBuffer, /*alsoPreLockSGL*/true);
        }
		mFrameDataRing.Add (&frameData);
	}	//	for each NTV2FrameData

	return AJA_STATUS_SUCCESS;

}	//	SetUpHostBuffers


AJAStatus NTV2StreamPlayer::SetUpTestPatternBuffers (void)
{
	vector<NTV2TestPatternSelect>	testPatIDs;
		testPatIDs.push_back(NTV2_TestPatt_ColorBars100);
		testPatIDs.push_back(NTV2_TestPatt_ColorBars75);
		testPatIDs.push_back(NTV2_TestPatt_Ramp);
		testPatIDs.push_back(NTV2_TestPatt_MultiBurst);
		testPatIDs.push_back(NTV2_TestPatt_LineSweep);
		testPatIDs.push_back(NTV2_TestPatt_CheckField);
		testPatIDs.push_back(NTV2_TestPatt_FlatField);
		testPatIDs.push_back(NTV2_TestPatt_MultiPattern);
		testPatIDs.push_back(NTV2_TestPatt_Border);

	mTestPatRasters.clear();
	for (size_t tpNdx(0);  tpNdx < testPatIDs.size();  tpNdx++)
		mTestPatRasters.push_back(NTV2Buffer());

	if (!mFormatDesc.IsValid())
		{PLFAIL("Bad format descriptor");  return AJA_STATUS_FAIL;}
	if (mFormatDesc.IsVANC())
		{PLFAIL("VANC should have been disabled: " << mFormatDesc);  return AJA_STATUS_FAIL;}

	//	Set up one video buffer for each test pattern...
	for (size_t tpNdx(0);  tpNdx < testPatIDs.size();  tpNdx++)
	{
		//	Allocate the buffer memory...
		if (!mTestPatRasters.at(tpNdx).Allocate (mFormatDesc.GetTotalBytes(), BUFFER_PAGE_ALIGNED))
		{	PLFAIL("Test pattern buffer " << DEC(tpNdx+1) << " of " << DEC(testPatIDs.size()) << ": "
					<< xHEX0N(mFormatDesc.GetTotalBytes(),8) << "-byte page-aligned alloc failed");
			return AJA_STATUS_MEMORY;
		}

		//	Fill the buffer with test pattern...
		NTV2TestPatternGen	testPatternGen;
		if (!testPatternGen.DrawTestPattern (testPatIDs.at(tpNdx),  mFormatDesc,  mTestPatRasters.at(tpNdx)))
		{
			cerr << "## ERROR:  DrawTestPattern " << DEC(tpNdx) << " failed: " << mFormatDesc << endl;
			return AJA_STATUS_FAIL;
		}

		#ifdef NTV2_BUFFER_LOCKING
			//	Try to prelock the memory, including its scatter-gather list...
//			if (!mDevice.DMABufferLock(mTestPatRasters.at(tpNdx), /*alsoLockSegmentMap=*/true))
//				PLWARN("Test pattern buffer " << DEC(tpNdx+1) << " of " << DEC(testPatIDs.size()) << ": failed to pre-lock");
		#endif
	}	//	loop for each predefined pattern

	return AJA_STATUS_SUCCESS;

}	//	SetUpTestPatternBuffers


bool NTV2StreamPlayer::RouteOutputSignal (void)
{
	const NTV2Standard	outputStandard	(::GetNTV2StandardFromVideoFormat(mConfig.fVideoFormat));
	const UWord			numSDIOutputs	(mDevice.features().GetNumVideoOutputs());
	const bool			isRGB			(::IsRGBFormat(mConfig.fPixelFormat));
	const bool			canVerify		(mDevice.features().HasCrosspointConnectROM());
	UWord				connectFailures	(0);

	const NTV2OutputXptID	cscVidOutXpt(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel,  false/*isKey*/,  !isRGB/*isRGB*/));
	const NTV2OutputXptID	fsVidOutXpt (::GetFrameBufferOutputXptFromChannel(mConfig.fOutputChannel,  isRGB/*isRGB*/,  false/*is425*/));
	const NTV2InputXptID	cscInputXpt (isRGB ? ::GetCSCInputXptFromChannel(mConfig.fOutputChannel, false/*isKeyInput*/) : NTV2_INPUT_CROSSPOINT_INVALID);
	if (isRGB)
		if (!mDevice.Connect (cscInputXpt,  fsVidOutXpt,  canVerify))
			connectFailures++;

	if (mConfig.fDoMultiFormat)
	{
		//	Multiformat --- We may be sharing the device with other processes, so route only one SDI output...
		if (mDevice.features().HasBiDirectionalSDI())
			mDevice.SetSDITransmitEnable(mConfig.fOutputChannel, true);

		if (!mDevice.Connect (::GetSDIOutputInputXpt (mConfig.fOutputChannel, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
			connectFailures++;
		//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically
		mDevice.SetSDIOutputStandard (mConfig.fOutputChannel, outputStandard);
	}
	else
	{
		//	Not multiformat:  We own the whole device, so connect all possible SDI outputs...
		const UWord	numFrameStores(mDevice.features().GetNumFrameStores());
		mDevice.ClearRouting();		//	Start with clean slate

		if (isRGB)
			if (!mDevice.Connect (cscInputXpt,  fsVidOutXpt,  canVerify))
				connectFailures++;

		for (NTV2Channel chan(NTV2_CHANNEL1);  ULWord(chan) < numSDIOutputs;  chan = NTV2Channel(chan+1))
		{
			if (chan != mConfig.fOutputChannel  &&  chan < numFrameStores)
				mDevice.DisableChannel(chan);				//	Disable unused FrameStore
			if (mDevice.features().HasBiDirectionalSDI())
				mDevice.SetSDITransmitEnable (chan, true);	//	Make it an output

			if (!mDevice.Connect (::GetSDIOutputInputXpt (chan, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
				connectFailures++;
			mDevice.SetSDIOutputStandard (chan, outputStandard);
			//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically

			if (mConfig.WithAudio())
				mDevice.SetSDIOutputAudioSystem (chan, mAudioSystem);	//	Ensure SDIOut embedder gets audio from mAudioSystem
		}	//	for each SDI output spigot

		//	And connect analog video output, if the device has one...
		if (mDevice.features().GetNumAnalogVideoOutputs())
			if (!mDevice.Connect (::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_ANALOG),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
				connectFailures++;

		//	And connect HDMI video output, if the device has one...
		if (mDevice.features().GetNumHDMIVideoOutputs())
			if (!mDevice.Connect (::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_HDMI),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
				connectFailures++;
	}
	if (connectFailures)
		PLWARN(DEC(connectFailures) << " 'Connect' call(s) failed");
	return connectFailures == 0;

}	//	RouteOutputSignal


AJAStatus NTV2StreamPlayer::Run (void)
{
	//	Start my consumer and producer threads...
	StartConsumerThread();
	StartProducerThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2StreamPlayer::StartConsumerThread (void)
{
	//	Create and start the playout thread...
	mConsumerThread.Attach (ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The playout thread function
void NTV2StreamPlayer::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2StreamPlayer instance pointer from the pContext parameter,
	//	then call its ConsumeFrames method...
	NTV2StreamPlayer * pApp (reinterpret_cast<NTV2StreamPlayer*>(pContext));
	if (pApp)
		pApp->ConsumeFrames();

}	//	ConsumerThreadStatic


void NTV2StreamPlayer::ConsumeFrames (void)
{
	NTV2StreamChannel	strStatus;
	NTV2StreamBuffer	bfrStatus;
	ULWord				goodQueue(0), badQueue(0), goodRelease(0), starves(0), noRoomWaits(0);
	ULWord				status;

	//	Initialize and claim ownership of the stream
	status = mDevice.StreamChannelInitialize(mConfig.fOutputChannel);
	if (status != NTV2_STREAM_STATUS_SUCCESS)
	{
		cerr << "## ERROR:  Stream initialize failed: " << status << endl;
		return;
	}
	PLNOTE("Thread started");

	while (!mGlobalQuit)
	{
        //  Get stream status
		status = mDevice.StreamChannelStatus(mConfig.fOutputChannel, strStatus);
		if (status != NTV2_STREAM_STATUS_SUCCESS)
		{
			cerr << "## ERROR:  Stream status failed: " << status << endl;
			return;
		}

		if (strStatus.GetQueueDepth() < 8)
		{
			NTV2FrameData *	pFrameData (mFrameDataRing.StartConsumeNextBuffer());
			if (pFrameData)
            {
				//  Queue frame to stream
				NTV2_POINTER buffer(pFrameData->fVideoBuffer.GetHostAddress(0), pFrameData->fVideoBuffer.GetByteCount());
				status = mDevice.StreamBufferQueue(mConfig.fOutputChannel,
													buffer,
													goodQueue,
													bfrStatus);
				if (status == NTV2_STREAM_STATUS_SUCCESS)
				{
					goodQueue++;
				}
				else
				{
					badQueue++;
					cerr << "## ERROR:  Stream buffer add failed: " << status << endl;
				}

				if (goodQueue == 3)
				{
					//  Start the stream
					status = mDevice.StreamChannelStart(mConfig.fOutputChannel, strStatus);
					if (status != NTV2_STREAM_STATUS_SUCCESS)
					{
						cerr << "## ERROR:  Stream initialize failed: " << status << endl;
						return;
					}
				}
			}
			else
			{
                starves++;
            }
			continue;	//	Back to top of while loop
		}
		else
		{
			noRoomWaits++;
		}

		//  Look for buffers to release
        status = mDevice.StreamBufferRelease(mConfig.fOutputChannel, bfrStatus);
		while (status == NTV2_STREAM_STATUS_SUCCESS)
		{
			mFrameDataRing.EndConsumeNextBuffer();
			goodRelease++;
            status = mDevice.StreamBufferRelease(mConfig.fOutputChannel, bfrStatus);
		}

		//	Wait for one or more buffers to become available on the device, which should occur at next VBI...
		mDevice.StreamChannelWait(mConfig.fOutputChannel, strStatus);
	}	//	loop til quit signaled

	//	Stop streaming...
	status = mDevice.StreamChannelInitialize(mConfig.fOutputChannel);
	if (status != NTV2_STREAM_STATUS_SUCCESS)
	{
		cerr << "## ERROR:  Stream initialize failed: " << status << endl;
		return;
	}

    //  Release all buffers
    status = mDevice.StreamBufferRelease(mConfig.fOutputChannel, bfrStatus);
    while (status == NTV2_STREAM_STATUS_SUCCESS)
    {
        mFrameDataRing.EndConsumeNextBuffer();
        goodRelease++;
        status = mDevice.StreamBufferRelease(mConfig.fOutputChannel, bfrStatus);
    }

    //  Release stream ownership
	status = mDevice.StreamChannelRelease(mConfig.fOutputChannel);
	if (status != NTV2_STREAM_STATUS_SUCCESS)
	{
		cerr << "## ERROR:  Stream release failed: " << status << endl;
		return;
	}    

	PLNOTE("Thread completed: " << DEC(goodQueue) << " queued, " << DEC(badQueue) << " failed, "
			<< DEC(starves) << " starves, " << DEC(noRoomWaits) << " VBI waits");

}	//	ConsumeFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2StreamPlayer::StartProducerThread (void)
{
	//	Create and start the producer thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


void NTV2StreamPlayer::ProducerThreadStatic (AJAThread * pThread, void * pContext)	//	static
{	(void) pThread;
	NTV2StreamPlayer * pApp (reinterpret_cast<NTV2StreamPlayer*>(pContext));
	if (pApp)
		pApp->ProduceFrames();

}	//	ProducerThreadStatic


void NTV2StreamPlayer::ProduceFrames (void)
{
	ULWord	testPatNdx(0), badTally(0);

	const AJATimeBase		timeBase		(CNTV2DemoCommon::GetAJAFrameRate(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat)));
	const NTV2StringList	tpNames			(NTV2TestPatternGen::getTestPatternNames());

	PLNOTE("Thread started");
	NTV2FrameData *	pFrameData (mFrameDataRing.StartProduceNextBuffer());
	mFrameDataRing.EndProduceNextBuffer();
	while (!mGlobalQuit)
	{
#if 0
		NTV2FrameData *	pFrameData (mFrameDataRing.StartProduceNextBuffer());
		if (!pFrameData)
		{	badTally++;			//	No frame available!
			AJATime::Sleep(10);	//	Wait a bit for the consumer thread to free one up for me...
			continue;			//	...then try again
		}
#endif
		//	Copy fresh, unmodified, pre-rendered test pattern into this frame's video buffer...
		pFrameData->fVideoBuffer.CopyFrom (mTestPatRasters.at(testPatNdx),
											/*srcOffset*/ 0,
											/*dstOffset*/ 0,
											/*byteCount*/ pFrameData->fVideoBuffer.GetByteCount());
		testPatNdx = (testPatNdx + 1) % ULWord(mTestPatRasters.size());
		AJATime::Sleep(5000);
	}	//	loop til mGlobalQuit goes true
	PLNOTE("Thread completed: " << DEC(mCurrentFrame) << " frame(s) produced, " << DEC(badTally) << " failed");

}	//	ProduceFrames


void NTV2StreamPlayer::GetStreamStatus (NTV2StreamChannel & outStatus)
{
	mDevice.StreamChannelStatus(mConfig.fOutputChannel, outStatus);
}

