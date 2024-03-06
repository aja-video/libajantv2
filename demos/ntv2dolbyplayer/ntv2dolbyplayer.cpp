/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolbyplayer.cpp
	@brief		Implementation of NTV2DolbyPlayer class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2dolbyplayer.h"
#include "ntv2debug.h"
#include "ntv2devicescanner.h"
#include "ntv2testpatterngen.h"
#include "ntv2audiodefines.h"
#include "ajabase/common/timebase.h"
#include "ajabase/system/process.h"

//#include "ajabase/system/debug.h"


using namespace std;

//	Convenience macros for EZ logging:
#define	TCFAIL(_expr_)	AJA_sERROR  (AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCWARN(_expr_)	AJA_sWARNING(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCNOTE(_expr_)	AJA_sNOTICE	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCINFO(_expr_)	AJA_sINFO	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCDBG(_expr_)	AJA_sDEBUG	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)

/**
	@brief	The maximum number of bytes of 48KHz audio that can be transferred for a single frame.
			Worst case, assuming 16 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			48,000 samples per second requires 3,204 samples x 4 bytes/sample x 16 = 205,056 bytes
			201K will suffice, with 768 bytes to spare
**/
static const uint32_t	AUDIOBYTES_MAX_48K	(201 * 1024);

/**
    @brief	The maximum number of bytes of 192KHz audio that can be transferred for a single frame.
            Worst case, assuming 8 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			96,000 samples per second requires 12,864 samples x 4 bytes/sample x 16 = 823,296 bytes
			824K will suffice
**/
static const uint32_t	AUDIOBYTES_MAX_192K	(824 * 1024);

static const bool		BUFFER_PAGE_ALIGNED	(true);

//	Audio tone generator data
static const double	gFrequencies []	=	{250.0, 500.0, 1000.0, 2000.0};
static const ULWord	gNumFrequencies		(sizeof (gFrequencies) / sizeof (double));
static const double	gAmplitudes []	=	{	0.10, 0.15,		0.20, 0.25,		0.30, 0.35,		0.40, 0.45,		0.50, 0.55,		0.60, 0.65,		0.70, 0.75,		0.80, 0.85,
											0.85, 0.80,		0.75, 0.70,		0.65, 0.60,		0.55, 0.50,		0.45, 0.40,		0.35, 0.30,		0.25, 0.20,		0.15, 0.10};

NTV2DolbyPlayer::NTV2DolbyPlayer (const DolbyPlayerConfig & inConfig)
	:	mConfig				(inConfig),
		mConsumerThread		(),
		mProducerThread		(),
		mDevice				(),
		mSavedTaskMode		(NTV2_TASK_MODE_INVALID),
		mCurrentFrame		(0),
		mCurrentSample		(0),
		mToneFrequency		(440.0),
		mAudioSystem		(NTV2_AUDIOSYSTEM_INVALID),
		mFormatDesc			(),
		mTCIndexes			(),
		mGlobalQuit			(false),
		mTCBurner			(),
		mHostBuffers		(),
		mFrameDataRing		(),
		mTestPatRasters		(),
        mAudioRate			(NTV2_AUDIO_192K),
		mRampSample			(0),
		mBurstIndex			(0),
		mBurstSamples		(0),
		mBurstBuffer		(NULL),
		mBurstSize			(0),
		mBurstOffset		(0),
		mBurstMax			(0),
		mDolbyBuffer		(NULL),
		mDolbySize			(0),
		mDolbyBlocks		(0)
{ }


NTV2DolbyPlayer::~NTV2DolbyPlayer (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit ();

	mDevice.UnsubscribeOutputVerticalEvent (mConfig.fOutputChannel);

	if (mDolbyBuffer)
	{
		delete [] mDolbyBuffer;
		mDolbyBuffer = NULL;
	}

	if (mBurstBuffer)
	{
		delete [] mBurstBuffer;
		mBurstBuffer = NULL;
	}

}	//	destructor


void NTV2DolbyPlayer::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mProducerThread.Active())
		AJATime::Sleep(10);

	while (mConsumerThread.Active())
		AJATime::Sleep(10);

    mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);
    mDevice.SetHDMIOutAudioRate(NTV2_AUDIO_48K);
	mDevice.SetHDMIOutAudioFormat(NTV2_AUDIO_FORMAT_LPCM);

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


AJAStatus NTV2DolbyPlayer::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

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
		cerr << mConfig;
		if (mDevice.IsRemote())
			cerr	<< "Device Description:  " << mDevice.GetDescription() << endl;
		cerr << endl;
	#endif	//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2DolbyPlayer::SetUpVideo ()
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

	//	Keep the raster description handy...
	mFormatDesc = NTV2FormatDescriptor(mConfig.fVideoFormat, mConfig.fPixelFormat);
	if (!mFormatDesc.IsValid())
		return AJA_STATUS_FAIL;

	//	Turn on the FrameStore (to read frame buffer memory and transmit video)...
	mDevice.EnableChannel(mConfig.fOutputChannel);

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

	//	Set output clock reference...
	mDevice.SetReference(mDevice.features().CanDo2110() ? NTV2_REFERENCE_SFP1_PTP : NTV2_REFERENCE_FREERUN);

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2DolbyPlayer::SetUpAudio (void)
{
    const uint16_t	numberOfAudioChannels	(8);

	mAudioSystem = NTV2_AUDIOSYSTEM_1;										//	Use NTV2_AUDIOSYSTEM_1...
	if (mDevice.features().GetNumAudioSystems() > 1)						//	...but if the device has more than one audio system...
		mAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fOutputChannel);	//	...base it on the channel
	//	However, there are a few older devices that have only 1 audio system,
	//	yet 2 frame stores (or must use channel 2 for playout)...
	if (!mDevice.features().CanDoFrameStore1Display())
		mAudioSystem = NTV2_AUDIOSYSTEM_1;

	mDevice.SetNumberAudioChannels (numberOfAudioChannels, mAudioSystem);
    mDevice.SetAudioRate (mAudioRate, mAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set the SDI output audio embedders to embed audio samples from the output of mAudioSystem...
    mDevice.SetHDMIOutAudioSource2Channel (NTV2_AudioChannel1_2, mAudioSystem);

    mDevice.SetHDMIOutAudioRate(mAudioRate);

	 mDevice.SetHDMIOutAudioFormat(( mConfig.fDolbyFile != NULL) ? NTV2_AUDIO_FORMAT_DOLBY : NTV2_AUDIO_FORMAT_LPCM);

	//	If the last app using the device left it in end-to-end mode (input passthru),
	//	then loopback must be disabled, or else the output will contain whatever audio
	//	is present in whatever signal is feeding the device's SDI input...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio


AJAStatus NTV2DolbyPlayer::SetUpHostBuffers (void)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Let my circular buffer know when it's time to quit...
	mFrameDataRing.SetAbortFlag (&mGlobalQuit);

	//	Calculate the size of the audio buffer, which mostly depends on the sample rate...
    const uint32_t AUDIOBYTES_MAX = (mAudioRate == NTV2_AUDIO_192K) ? AUDIOBYTES_MAX_192K : AUDIOBYTES_MAX_48K;

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
		#ifdef NTV2_BUFFER_LOCKING
		if (frameData.fVideoBuffer)
			mDevice.DMABufferLock(frameData.fVideoBuffer, true);
		#endif

		//	Allocate a page-aligned audio buffer (if transmitting audio)
		if (mConfig.WithAudio())
			if (!frameData.fAudioBuffer.Allocate (AUDIOBYTES_MAX, BUFFER_PAGE_ALIGNED))
			{
				PLFAIL("Failed to allocate " << xHEX0N(AUDIOBYTES_MAX,8) << "-byte audio buffer");
				return AJA_STATUS_MEMORY;
			}
		#ifdef NTV2_BUFFER_LOCKING
		if (frameData.fAudioBuffer)
			mDevice.DMABufferLock(frameData.fAudioBuffer, /*alsoPreLockSGL*/true);
		#endif
		mFrameDataRing.Add (&frameData);
	}	//	for each NTV2FrameData
	
    if (mConfig.fDolbyFile != NULL)
    {
		//  Initialize IEC61937 burst size (32 milliseconds) for HDMI 192 kHz sample rate
		mBurstSamples = 6144;  //  192000 * 0.032 samples
		mBurstMax = mBurstSamples * 2;
		mBurstBuffer = new uint16_t [mBurstMax];
		mDolbyBuffer = new uint16_t [mBurstMax];
    }

	return AJA_STATUS_SUCCESS;

}	//	SetUpHostBuffers

AJAStatus NTV2DolbyPlayer::SetUpTestPatternBuffers (void)
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
			if (!mDevice.DMABufferLock(mTestPatRasters.at(tpNdx), /*alsoLockSegmentMap=*/true))
				PLWARN("Test pattern buffer " << DEC(tpNdx+1) << " of " << DEC(testPatIDs.size()) << ": failed to pre-lock");
		#endif
	}	//	loop for each predefined pattern

	return AJA_STATUS_SUCCESS;

}	//	SetUpTestPatternBuffers


bool NTV2DolbyPlayer::RouteOutputSignal (void)
{
	const bool			isRGB			(::IsRGBFormat(mConfig.fPixelFormat));

    const NTV2OutputCrosspointID	fsVidOutXpt		(::GetFrameBufferOutputXptFromChannel (mConfig.fOutputChannel,  isRGB/*isRGB*/,  false/*is425*/));

    mDevice.ClearRouting();		//	Start with clean slate

    //	And connect HDMI video output
    return mDevice.Connect (::GetOutputDestInputXpt (NTV2_OUTPUTDESTINATION_HDMI),  fsVidOutXpt);

}	//	RouteOutputSignal


AJAStatus NTV2DolbyPlayer::Run ()
{
	//	Start my consumer and producer threads...
	StartConsumerThread ();
	StartProducerThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2DolbyPlayer::StartConsumerThread ()
{
	//	Create and start the playout thread...
	mConsumerThread.Attach (ConsumerThreadStatic, this);
	mConsumerThread.SetPriority (AJA_ThreadPriority_High);
	mConsumerThread.Start ();

}	//	StartConsumerThread


//	The playout thread function
void NTV2DolbyPlayer::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2DolbyPlayer instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2DolbyPlayer *	pApp	(reinterpret_cast <NTV2DolbyPlayer *> (pContext));
	if (pApp)
		pApp->ConsumeFrames ();

}	//	ConsumerThreadStatic


void NTV2DolbyPlayer::ConsumeFrames (void)
{
	ULWord					acOptions		(AUTOCIRCULATE_WITH_RP188);	//	Add timecode
	AUTOCIRCULATE_TRANSFER	outputXfer;
	AUTOCIRCULATE_STATUS	outputStatus;
	ULWord					goodXfers(0), badXfers(0), starves(0), noRoomWaits(0);

	//	Stop AutoCirculate, just in case someone else left it running...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel, 4);	//	Let it stop
	PLNOTE("Thread started");

	//	Initialize & start AutoCirculate...
	bool initOK = mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,  mConfig.fFrames.count(),  mAudioSystem,  acOptions,
														1 /*numChannels*/,  mConfig.fFrames.firstFrame(),  mConfig.fFrames.lastFrame());
	if (!initOK)
		{PLFAIL("AutoCirculateInitForOutput failed");  mGlobalQuit = true;}
	else if (!mConfig.WithVideo())
	{	//	Video suppressed --
		//	Clear device frame buffers being AutoCirculated (prevent garbage output frames)
		NTV2Buffer tmpFrame (mFormatDesc.GetVideoWriteSize());
		NTV2TestPatternGen blackPatternGen;
		blackPatternGen.DrawTestPattern (NTV2_TestPatt_Black, mFormatDesc, tmpFrame);
		mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outputStatus);
		for (uint16_t frmNum(outputStatus.GetStartFrame());  frmNum <= outputStatus.GetEndFrame();  frmNum++)
			mDevice.DMAWriteFrame(ULWord(frmNum), tmpFrame, mFormatDesc.GetTotalBytes());
	}	//	else if --novideo


	while (!mGlobalQuit)
	{
		mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outputStatus);

		//	Check if there's room for another frame on the card...
		if (outputStatus.CanAcceptMoreOutputFrames())
		{
			//	Device has at least one free frame buffer that can be filled.
			//	Wait for the next frame in our ring to become ready to "consume"...
			NTV2FrameData *	pFrameData (mFrameDataRing.StartConsumeNextBuffer());
			if (!pFrameData)
				{starves++;  continue;}

			outputXfer.SetOutputTimeCodes(pFrameData->fTimecodes);

			if (pFrameData->VideoBuffer())	//	Transfer the timecode-burned frame to the device for playout...
				outputXfer.SetVideoBuffer (pFrameData->VideoBuffer(), pFrameData->VideoBufferSize());
			if (pFrameData->AudioBuffer())	//	If also playing audio...
				outputXfer.SetAudioBuffer (pFrameData->AudioBuffer(), pFrameData->fNumAudioBytes);


			//	Perform the DMA transfer to the device...
			if (mDevice.AutoCirculateTransfer (mConfig.fOutputChannel, outputXfer))
				goodXfers++;
			else
				badXfers++;

			if (goodXfers == 3)
				mDevice.AutoCirculateStart(mConfig.fOutputChannel);

			//	Signal that the frame has been "consumed"...
			mFrameDataRing.EndConsumeNextBuffer();
			continue;	//	Back to top of while loop

		}

		//	Wait for one or more buffers to become available on the device, which should occur at next VBI...
		noRoomWaits++;
		mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel);
	}	//	loop til quit signaled

		//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	PLNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(starves) << " starves, " << DEC(noRoomWaits) << " VBI waits");

}	//	ConsumeFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2DolbyPlayer::StartProducerThread ()
{
	//	Create and start the producer thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


void NTV2DolbyPlayer::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	NTV2DolbyPlayer *	pApp	(reinterpret_cast <NTV2DolbyPlayer *> (pContext));
	if (pApp)
		pApp->ProduceFrames ();

}	//	ProducerThreadStatic


void NTV2DolbyPlayer::ProduceFrames (void)
{
	ULWord	freqNdx(0), testPatNdx(0), badTally(0);
	double	timeOfLastSwitch	(0.0);

	const AJATimeBase		timeBase		(CNTV2DemoCommon::GetAJAFrameRate(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat)));
	const NTV2StringList	tpNames			(NTV2TestPatternGen::getTestPatternNames());
	//const bool				isInterlace		(!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mConfig.fVideoFormat));
	//const bool				isPAL			(NTV2_IS_PAL_VIDEO_FORMAT(mConfig.fVideoFormat));
	const NTV2FrameRate		ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
	const TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));

	PLNOTE("Thread started");
	while (!mGlobalQuit)
	{
		NTV2FrameData *	pFrameData (mFrameDataRing.StartProduceNextBuffer());
		//  If no frame is available, wait and try again
		if (!pFrameData)
		{	badTally++;			//	No frame available!
			AJATime::Sleep(10);	//	Wait a bit for the consumer thread to free one up for me...
			continue;			//	...then try again
		}

		//	Copy my pre-made test pattern into my video buffer...
		if (pFrameData->VideoBuffer())	//	Copy fresh, unmodified, pre-rendered test pattern into this frame's video buffer...
			pFrameData->fVideoBuffer.CopyFrom (mTestPatRasters.at(testPatNdx),
												/*srcOffset*/ 0,
												/*dstOffset*/ 0,
												/*byteCount*/ pFrameData->fVideoBuffer.GetByteCount());

		const	CRP188	rp188Info		(mCurrentFrame++, 0, 0, 10, tcFormat);
		NTV2_RP188		tcF1;
		string			tcString;

		rp188Info.GetRP188Reg (tcF1);
		rp188Info.GetRP188Str (tcString);

		if (pFrameData->VideoBuffer())	//	Burn current timecode into the video buffer...
			mTCBurner.BurnTimeCode (pFrameData->VideoBuffer(), tcString.c_str(), 80);
		TCDBG("F" << DEC0N(mCurrentFrame-1,6) << ": " << tcF1 << ": " << tcString);

		//	If also playing audio...
		if (pFrameData->AudioBuffer())	//	...then generate audio tone data for this frame...
		{
			if (mConfig.fDolbyFile != NULL)
				pFrameData->fNumAudioBytes = AddDolby(*pFrameData);
			else
			{
				if (mConfig.fdoRamp)
					pFrameData->fNumAudioBytes = AddRamp(*pFrameData);
				else
					pFrameData->fNumAudioBytes = AddTone(*pFrameData);
			}
		}

		//	Every few seconds, change the test pattern and tone frequency...
		const double currentTime (timeBase.FramesToSeconds(mCurrentFrame));
		if (currentTime > timeOfLastSwitch + 4.0)
		{
			freqNdx = (freqNdx + 1) % gNumFrequencies;
			testPatNdx = (testPatNdx + 1) % ULWord(mTestPatRasters.size());
			mToneFrequency = gFrequencies[freqNdx];
			timeOfLastSwitch = currentTime;
			PLINFO("F" << DEC0N(mCurrentFrame,6) << ": " << tcString << ": tone=" << mToneFrequency << "Hz, pattern='" << tpNames.at(testPatNdx) << "'");
		}	//	if time to switch test pattern & tone frequency

		//	Signal that I'm done producing this FrameData, making it immediately available for transfer/playout...
		mFrameDataRing.EndProduceNextBuffer();

	}	//	loop til mGlobalQuit goes true
	PLNOTE("Thread completed: " << DEC(mCurrentFrame) << " frame(s) produced, " << DEC(badTally) << " failed");

}	//	ProduceFrames

void NTV2DolbyPlayer::GetACStatus (AUTOCIRCULATE_STATUS & outStatus)
{
	mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outStatus);
}


uint32_t NTV2DolbyPlayer::AddTone (NTV2FrameData &  inFrameData)
{
	NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
	NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
	ULWord			numChannels	(0);

	mDevice.GetFrameRate (frameRate, mConfig.fOutputChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);

	//	Set per-channel tone frequencies...
	double	pFrequencies [kNumAudioChannelsMax];
	pFrequencies [0] = (mToneFrequency / 2.0);
	for (ULWord chan (1);  chan < numChannels;  chan++)
		//	The 1.154782 value is the 16th root of 10, to ensure that if mToneFrequency is 2000,
		//	that the calculated frequency of audio channel 16 will be 20kHz...
		pFrequencies [chan] = pFrequencies [chan - 1] * 1.154782;

	//	Because audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will necessarily
	//	result in some frames having more audio samples than others. The GetAudioSamplesPerFrame function
	//	is used to calculate the correct sample count...
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));
	const double	sampleRateHertz	(::GetAudioSamplesPerSecond(audioRate));

	return ::AddAudioTone (	inFrameData.AudioBuffer(),	//	audio buffer to fill
							mCurrentSample,				//	which sample for continuing the waveform
							numSamples,					//	number of samples to generate
							sampleRateHertz,			//	sample rate [Hz]
							gAmplitudes,				//	per-channel amplitudes
							pFrequencies,				//	per-channel tone frequencies [Hz]
							31,							//	bits per sample
							false,						//	don't byte swap
							numChannels);				//	number of audio channels to generate
}	//	AddTone

uint32_t NTV2DolbyPlayer::AddRamp (NTV2FrameData & inFrameData)
{
	ULWord*			audioBuffer	(inFrameData.AudioBuffer());
	NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
	NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
	ULWord			numChannels	(0);

	mDevice.GetFrameRate (frameRate, mConfig.fOutputChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);

	//	Because audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will necessarily
	//	result in some frames having more audio samples than others. The GetAudioSamplesPerFrame function
	//	is used to calculate the correct sample count...
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));

	for (uint32_t i = 0; i < numSamples; i++)
	{
		for (uint32_t j = 0; j < numChannels; j++)
		{
			*audioBuffer = ((uint32_t)mRampSample) << 16;
			audioBuffer++;
			mRampSample++;
		}
	}

	return numSamples * numChannels * 4;
}	//	AddRamp

#ifdef DOLBY_FULL_PARSER

uint32_t NTV2DolbyPlayer::AddDolby (NTV2FrameData & inFrameData)
{
	ULWord*			audioBuffer	(inFrameData.AudioBuffer());
    NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
    NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
    ULWord			numChannels	(0);
    ULWord          sampleOffset(0);
	ULWord			sampleCount	(0);
	NTV2DolbyBSI	bsi;

	mDevice.GetFrameRate (frameRate, mConfig.fOutputChannel);
    mDevice.GetAudioRate (audioRate, mAudioSystem);
    mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);
    const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));

	if ((mConfig.fDolbyFile == NULL) || (mDolbyBuffer == NULL))
		goto silence;

    //  Generate the samples for this frame
    while (sampleOffset < numSamples)
    {
		if ((mBurstSize != 0) && (mBurstIndex < mBurstSamples))
		{
			if(mBurstOffset < mBurstSize)
			{
				uint32_t data0 = 0;
				uint32_t data1 = 0;
				if (mBurstIndex == 0)
				{
					//  Add IEC61937 burst preamble
					data0 = 0xf872;             //  Sync stuff
					data1 = 0x4e1f;
				}
				else if (mBurstIndex == 1)
				{
					//  Add more IEC61937 burst preamble
					data0 = ((bsi.bsmod & 0x7) << 8) | 0x0015;	//  This is Dolby
					data1 = mBurstSize * 2;						//  Data size in bytes
				}
				else
				{
					//  Add sync frame data
					data0 = (uint32_t)(mBurstBuffer[mBurstOffset]);
					data0 = ((data0 & 0x00ff) << 8) | ((data0 & 0xff00) >> 8);
					mBurstOffset++;
					data1 = (uint32_t)(mBurstBuffer[mBurstOffset]);
					data1 = ((data1 & 0x00ff) << 8) | ((data1 & 0xff00) >> 8);
					mBurstOffset++;
				}

				//  Write data into 16 msbs of all audio channel pairs
				data0 <<= 16;
				data1 <<= 16;
				for (ULWord i = 0; i < numChannels; i += 2)
				{
					audioBuffer[sampleOffset * numChannels + i] = data0;
					audioBuffer[sampleOffset * numChannels + i + 1] = data1;
				}
			}
			else
			{
				//  Pad samples out to burst size
				for (ULWord i = 0; i < numChannels; i++)
				{
					audioBuffer[sampleOffset * numChannels + i] = 0;
				}
			}
			sampleOffset++;
			mBurstIndex++;
		}
		else
		{
			ULWord dolbyOffset = 0;
			ULWord burstOffset = 0;
			ULWord numBlocks = 0;

			mBurstIndex = 0;
			mBurstOffset = 0;

			if (mDolbySize == 0)
			{
				// Find first Dolby Digital Plus burst frame
				while (true)
				{
					if (!GetDolbyFrame(&mDolbyBuffer[0], sampleCount))
					{
						cerr << "## ERROR:  Dolby frame not found" << endl;
						mConfig.fDolbyFile = NULL;
						goto silence;
					}

					if (!ParseBSI(&mDolbyBuffer[1], sampleCount - 1, &bsi))
						continue;

					if ((bsi.strmtyp == 0) &&
						(bsi.substreamid == 0) &&
						(bsi.bsid == 16) &&
						((bsi.numblkscod == 3) || (bsi.convsync == 1)))
						break;
				}

				mDolbySize = sampleCount;
				switch (bsi.numblkscod)
				{
				case 0: mDolbyBlocks = 1; break;
				case 1: mDolbyBlocks = 2; break;
				case 2: mDolbyBlocks = 3; break;
				case 3: mDolbyBlocks = 6; break;
				default: goto silence;
				}
			}

			while (numBlocks <= 6)
			{
				// Copy the Dolby frame into the burst buffer
				while (dolbyOffset < mDolbySize)
				{
					// Check for burst size overrun
					if (burstOffset >= mBurstMax)
					{
						cerr << "## ERROR:  Dolby burst too large" << endl;
						mConfig.fDolbyFile = NULL;
						goto silence;
					}

					// Copy sample
					mBurstBuffer[burstOffset] = mDolbyBuffer[dolbyOffset];
					burstOffset++;
					dolbyOffset++;
				}

				// Get the next Dolby frame
				if (!GetDolbyFrame(&mDolbyBuffer[0], sampleCount))
				{
					// try to loop
					if (!GetDolbyFrame(&mDolbyBuffer[0], sampleCount))
					{
						cerr << "## ERROR:  Dolby frame not found" << endl;
						mConfig.fDolbyFile = NULL;
						goto silence;
					}
				}

				// Parse the Dolby bitstream header
				if (!ParseBSI(&mDolbyBuffer[1], sampleCount - 1, &bsi))
					continue;

				// Only Dolby Digital Plus
				if (bsi.bsid != 16)
				{
					cerr << "## ERROR:  Dolby frame bad bsid = " << bsi.bsid << endl;
					continue;
				}

				mDolbySize = sampleCount;
				dolbyOffset = 0;

				// Increment block count on first substream
				if ((bsi.strmtyp == 0) && (bsi.substreamid == 0))
				{
					// increment block count
					numBlocks += mDolbyBlocks;

					switch (bsi.numblkscod)
					{
					case 0: mDolbyBlocks = 1; break;
					case 1: mDolbyBlocks = 2; break;
					case 2: mDolbyBlocks = 3; break;
					case 3: mDolbyBlocks = 6; break;
					default:
						cerr << "## ERROR:  Dolby frame bad numblkscod = " << bsi.numblkscod << endl;
						goto silence;
					}
				}

				//	Are we done?
				if (numBlocks >= 6)
				{
					// First frame of new burst must have convsync == 1
					if ((bsi.numblkscod != 3) &&
						(bsi.convsync != 1))
					{
						cerr << "## ERROR:  Dolby frame unexpected convsync = " << bsi.convsync << endl;
						mDolbySize = 0;
						mDolbyBlocks = 0;
					}

					//	Keep the burst size
					mBurstSize = burstOffset;
					break;
				}
			}
		}
	}

    return numSamples * numChannels * 4;

silence:
    //  Output silence when done with file
    memset(&audioBuffer[sampleOffset * numChannels], 0, (numSamples - sampleOffset) * numChannels * 4);
    return numSamples * numChannels * 4;
}


bool NTV2DolbyPlayer::GetDolbyFrame (uint16_t * pInDolbyBuffer, uint32_t & numSamples)
{
	uint32_t bytes;
	bool done = false;

	while (!done)
	{
		bytes = mConfig.fDolbyFile->Read((uint8_t*)(&pInDolbyBuffer[0]), 2);
		if (bytes != 2)
		{
			//  Reset file
			mConfig.fDolbyFile->Seek(0, eAJASeekSet);
			return false;
		}

		//  Check sync word
		if ((mDolbyBuffer[0] == 0x7705) ||
			(mDolbyBuffer[0] == 0x770b))
			done = true;
	}

	//  Read more of the sync frame header
	bytes = mConfig.fDolbyFile->Read((uint8_t*)(&pInDolbyBuffer[1]), 4);
	if (bytes != 4)
		return false;

	//  Get frame size - 16 bit words plus sync word
	uint32_t size = (uint32_t)mDolbyBuffer[1];
	size = (((size & 0x00ff) << 8) | ((size & 0xff00) >> 8));
	size = (size & 0x7ff) + 1;

	//  Read the rest of the sync frame
	uint32_t len = (size - 3) * 2;
	bytes = mConfig.fDolbyFile->Read((uint8_t*)(&pInDolbyBuffer[3]), len);
	if (bytes != len)
		return false;

	numSamples = size;

	return true;
}


bool NTV2DolbyPlayer::ParseBSI(uint16_t * pInDolbyBuffer, uint32_t numSamples, NTV2DolbyBSI * pBsi)
{
	if ((pInDolbyBuffer == NULL) || (pBsi == NULL))
		return false;

	memset(pBsi, 0, sizeof(NTV2DolbyBSI));

	SetBitBuffer((uint8_t*)pInDolbyBuffer, numSamples * 2);

	if (!GetBits(pBsi->strmtyp, 2)) return false;
	if (!GetBits(pBsi->substreamid, 3)) return false;
	if (!GetBits(pBsi->frmsiz, 11)) return false;
	if (!GetBits(pBsi->fscod, 2)) return false;
	if (!GetBits(pBsi->numblkscod, 2)) return false;
	if (!GetBits(pBsi->acmod, 3)) return false;
	if (!GetBits(pBsi->lfeon, 1)) return false;
	if (!GetBits(pBsi->bsid, 5)) return false;
	if (!GetBits(pBsi->dialnorm, 5)) return false;
	if (!GetBits(pBsi->compre, 1)) return false;
	if (pBsi->compre)
	{
		if (!GetBits(pBsi->compr, 8)) return false;
	}
	if (pBsi->acmod == 0x0) /* if 1+1 mode (dual mono, so some items need a second value) */
	{
		if (!GetBits(pBsi->dialnorm2, 5)) return false;
		if (!GetBits(pBsi->compr2e, 1)) return false;
		if (pBsi->compr2e)
		{
			if (!GetBits(pBsi->compr2, 8)) return false;
		}
	}
	if (pBsi->strmtyp == 0x1) /* if dependent stream */
	{
		if (!GetBits(pBsi->chanmape, 1)) return false;
		if (pBsi->chanmape)
		{
			if (!GetBits(pBsi->chanmap, 16)) return false;
		}
	}
	if (!GetBits(pBsi->mixmdate, 1)) return false;
	if (pBsi->mixmdate) /* mixing metadata */
	{
		if (pBsi->acmod > 0x2) /* if more than 2 channels */
		{
			if (!GetBits(pBsi->dmixmod, 2)) return false;
		}
		if ((pBsi->acmod & 0x1) && (pBsi->acmod > 0x2)) /* if three front channels exist */
		{
			if (!GetBits(pBsi->ltrtcmixlev, 3)) return false;
			if (!GetBits(pBsi->lorocmixlev, 3)) return false;
		}
		if (pBsi->acmod & 0x4) /* if a surround channel exists */
		{
			if (!GetBits(pBsi->ltrtsurmixlev, 3)) return false;
			if (!GetBits(pBsi->lorosurmixlev, 3)) return false;
		}
		if (pBsi->lfeon) /* if the LFE channel exists */
		{
			if (!GetBits(pBsi->lfemixlevcode, 1)) return false;
			if (pBsi->lfemixlevcode)
			{
						 if (!GetBits(pBsi->lfemixlevcod, 5)) return false;
			}
		}
		if (pBsi->strmtyp == 0x0) /* if independent stream */
		{
			if (!GetBits(pBsi->pgmscle, 1)) return false;
			if (pBsi->pgmscle)
			{
				if (!GetBits(pBsi->pgmscl, 6)) return false;
			}
			if (pBsi->acmod == 0x0) /* if 1+1 mode (dual mono, so some items need a second value) */
			{
				if (!GetBits(pBsi->pgmscl2e, 1)) return false;
				if (pBsi->pgmscl2e)
				{
					if (!GetBits(pBsi->pgmscl2, 6)) return false;
				}
			}
			if (!GetBits(pBsi->extpgmscle, 1)) return false;
			if (pBsi->extpgmscle)
			{
				if (!GetBits(pBsi->extpgmscl, 6)) return false;
			}
			if (!GetBits(pBsi->mixdef, 2)) return false;
			if (pBsi->mixdef == 0x1) /* mixing option 2 */
			{
				if (!GetBits(pBsi->premixcmpsel, 1)) return false;
				if (!GetBits(pBsi->drcsrc, 1)) return false;
				if (!GetBits(pBsi->premixcmpscl, 3)) return false;
			}
			else if (pBsi->mixdef == 0x2) /* mixing option 3 */
			{
				if (!GetBits(pBsi->mixdata, 12)) return false;
			}
			else if (pBsi->mixdef == 0x3) /* mixing option 4 */
			{
				if (!GetBits(pBsi->mixdeflen, 5)) return false;
				if (!GetBits(pBsi->mixdata2e, 1)) return false;
				if (pBsi->mixdata2e)
				{
					if (!GetBits(pBsi->premixcmpsel, 1)) return false;
					if (!GetBits(pBsi->drcsrc, 1)) return false;
					if (!GetBits(pBsi->premixcmpscl, 3)) return false;
					if (!GetBits(pBsi->extpgmlscle, 1)) return false;
					if (pBsi->extpgmlscle)
					{
						if (!GetBits(pBsi->extpgmlscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmcscle, 1)) return false;
					if (pBsi->extpgmcscle)
					{
						if (!GetBits(pBsi->extpgmcscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmrscle, 1)) return false;
					if (pBsi->extpgmrscle)
					{
						if (!GetBits(pBsi->extpgmrscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmlsscle, 1)) return false;
					if (pBsi->extpgmlsscle)
					{
						if (!GetBits(pBsi->extpgmlsscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmrsscle, 1)) return false;
					if (pBsi->extpgmrsscle)
					{
						if (!GetBits(pBsi->extpgmrsscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmlfescle, 1)) return false;
					if (pBsi->extpgmlfescle)
					{
						if (!GetBits(pBsi->extpgmlfescl, 4)) return false;
					}
					if (!GetBits(pBsi->dmixscle, 1)) return false;
					if (pBsi->dmixscle)
					{
						if (!GetBits(pBsi->dmixscl, 4)) return false;
					}
					if (!GetBits(pBsi->addche, 1)) return false;
					if (pBsi->addche)
					{
						if (!GetBits(pBsi->extpgmaux1scle, 1)) return false;
						if (pBsi->extpgmaux1scle)
						{
							if (!GetBits(pBsi->extpgmaux1scl, 4)) return false;
						}
						if (!GetBits(pBsi->extpgmaux2scle, 1)) return false;
						if (pBsi->extpgmaux2scle)
						{
							if (!GetBits(pBsi->extpgmaux2scl, 4)) return false;
						}
					}
				}
				if (!GetBits(pBsi->mixdata3e, 1)) return false;
				if (pBsi->mixdata3e)
				{
					if (!GetBits(pBsi->spchdat, 5)) return false;
					if (!GetBits(pBsi->addspchdate, 1)) return false;
					if (pBsi->addspchdate)
					{
						if (!GetBits(pBsi->spchdat1, 5)) return false;
						if (!GetBits(pBsi->spchan1att, 2)) return false;
						if (!GetBits(pBsi->addspchdat1e, 1)) return false;
						if (pBsi->addspdat1e)
						{
							if (!GetBits(pBsi->spchdat2, 5)) return false;
							if (!GetBits(pBsi->spchan2att, 3)) return false;
						}
					}
				}
				{
					uint32_t data;
					uint32_t size = 8 * (pBsi->mixdeflen + 2);
					size = (size + 7) / 8 * 8;
					uint32_t index = 0;
					while (size > 0)
					{
						if (!GetBits(data, (size > 8)? 8 : size)) return false;
						pBsi->mixdatabuffer[index++] = (uint8_t)data;
						size = size - 8;
					}
				}
			}
			if (pBsi->acmod < 0x2) /* if mono or dual mono source */
			{
				if (!GetBits(pBsi->paninfoe, 1)) return false;
				if (pBsi->paninfoe)
				{
					if (!GetBits(pBsi->panmean, 8)) return false;
					if (!GetBits(pBsi->paninfo, 6)) return false;
				}
				if (pBsi->acmod == 0x0) /* if 1+1 mode (dual mono - some items need a second value) */
				{
					if (!GetBits(pBsi->paninfo2e, 1)) return false;
					if (pBsi->paninfo2e)
					{
						if (!GetBits(pBsi->panmean2, 8)) return false;
						if (!GetBits(pBsi->paninfo2, 6)) return false;
					}
				}
			}
			if (!GetBits(pBsi->frmmixcfginfoe, 1)) return false;
			if (pBsi->frmmixcfginfoe) /* mixing configuration information */
			{
				if (pBsi->numblkscod == 0x0)
				{
					if (!GetBits(pBsi->blkmixcfginfo[0], 5)) return false;
				}
				else
				{
					uint32_t blk;
					uint32_t numblk;
					if (pBsi->numblkscod == 0x1)
						numblk = 2;
					else if (pBsi->numblkscod == 0x2)
						numblk = 3;
					else if (pBsi->numblkscod == 0x3)
						numblk = 6;
					else
						return false;
					for(blk = 0; blk < numblk; blk++)
					{
						if (!GetBits(pBsi->blkmixcfginfoe, 1)) return false;
						if (pBsi->blkmixcfginfoe)
						{
							if (!GetBits(pBsi->blkmixcfginfo[blk], 5)) return false;
						}
					}
				}
			}
		}
	}
	if (!GetBits(pBsi->infomdate, 1)) return false;
	if (pBsi->infomdate) /* informational metadata */
	{
		if (!GetBits(pBsi->bsmod, 3)) return false;
		if (!GetBits(pBsi->copyrightb, 1)) return false;
		if (!GetBits(pBsi->origbs, 1)) return false;
		if (pBsi->acmod == 0x2) /* if in 2/0 mode */
		{
			if (!GetBits(pBsi->dsurmod, 2)) return false;
			if (!GetBits(pBsi->dheadphonmod, 2)) return false;
		}
		if (pBsi->acmod >= 0x6) /* if both surround channels exist */
		{
			if (!GetBits(pBsi->dsurexmod, 2)) return false;
		}
		if (!GetBits(pBsi->audprodie, 1)) return false;
		if (pBsi->audprodie)
		{
			if (!GetBits(pBsi->mixlevel, 5)) return false;
			if (!GetBits(pBsi->roomtyp, 2)) return false;
			if (!GetBits(pBsi->adconvtyp, 1)) return false;
		}
		if (pBsi->acmod == 0x0) /* if 1+1 mode (dual mono, so some items need a second value) */
		{
			if (!GetBits(pBsi->audprodi2e, 1)) return false;
			if (pBsi->audprodi2e)
			{
				if (!GetBits(pBsi->mixlevel2, 5)) return false;
				if (!GetBits(pBsi->roomtyp2, 2)) return false;
				if (!GetBits(pBsi->adconvtyp2, 1)) return false;
			}
		}
		if (pBsi->fscod < 0x3) /* if not half sample rate */
		{
			if (!GetBits(pBsi->sourcefscod, 1)) return false;
		}
	}
	if ((pBsi->strmtyp == 0x0) && (pBsi->numblkscod != 0x3))
	{
		if (!GetBits(pBsi->convsync, 1)) return false;
	}
	if (pBsi->strmtyp == 0x2) /* if bit stream converted from AC-3 */
	{
		if (pBsi->numblkscod == 0x3) /* 6 blocks per syncframe */
		{
			pBsi->blkid = 1;
		}
		else
		{
			if (!GetBits(pBsi->blkid, 1)) return false;
		}
		if (pBsi->blkid)
		{
			if (!GetBits(pBsi->frmsizecod, 6)) return false;
		}
	}
	if (!GetBits(pBsi->addbsie, 1)) return false;
	if (pBsi->addbsie)
	{
		if (!GetBits(pBsi->addbsil, 6)) return false;
		{
			uint32_t data;
			uint32_t size = 8 * (pBsi->addbsil + 1);
			uint32_t index = 0;
			while (size > 0)
			{
				if (!GetBits(data, 8)) return false;
				pBsi->addbsibuffer[index++] = (uint8_t)data;
				size = size - 8;
			}
		}
	}

	return true;
}

void NTV2DolbyPlayer::SetBitBuffer(uint8_t * pBuffer, uint32_t size)
{
	mBitBuffer = pBuffer;
	mBitSize = size;
	mBitIndex = 8;
}

bool NTV2DolbyPlayer::GetBits(uint32_t & data, uint32_t bits)
{
	uint32_t cb;
	static uint8_t bitMask[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

	data = 0;
	while (bits > 0)
	{
		if (mBitSize == 0)
			return false;

		if (mBitIndex == 0)
		{
			mBitBuffer++;
			mBitSize--;
			mBitIndex = 8;
		}

		cb = bits;
		if (cb > mBitIndex)
			cb = mBitIndex;
		bits -= cb;
		mBitIndex -= cb;
		data |= ((*mBitBuffer >> mBitIndex) & bitMask[cb]) << bits;
	}

	return true;
}

#else
// NOTE:  The code under this #else is out of date and not maintaiend
uint32_t NTV2DolbyPlayer::AddDolby (ULWord * pInAudioBuffer)
{
	NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
	NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
	ULWord			numChannels	(0);
	ULWord          sampleOffset(0);

	mDevice.GetFrameRate (frameRate, mOutputChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));

	if ((mConfig.fDolbyFile == NULL) || (mDolbyBuffer == NULL))
		goto silence;

	//  Generate the samples for this frame
	while (sampleOffset < numSamples)
	{
		//  Time for a new IEC61937 burst
		if (mBurstIndex >= mBurstSamples)
			mBurstIndex = 0;

		//  Get a new Dolby Digital Plus sync frame
		if (mBurstIndex == 0)
		{
			//  Read the sync word (all big endian)
			uint32_t bytes = mConfig.fDolbyFile->Read((uint8_t*)(&mDolbyBuffer[0]), 2);
			if (bytes != 2)
			{
				//  Try to loop
				mConfig.fDolbyFile->Seek(0, eAJASeekSet);
				bytes = mConfig.fDolbyFile->Read((uint8_t*)(&mDolbyBuffer[0]), 2);
				if (bytes != 2)
					goto silence;
			}

			//  Check sync word
			if ((mDolbyBuffer[0] != 0x7705) &&
				(mDolbyBuffer[0] != 0x770b))
				goto silence;

			//  Read more of the sync frame header
			bytes = mConfig.fDolbyFile->Read((uint8_t*)(&mDolbyBuffer[1]), 4);
			if (bytes != 4)
				goto silence;

			//  Get frame size - 16 bit words plus sync word
			uint32_t size = (uint32_t)mDolbyBuffer[1];
			size = (((size & 0x00ff) << 8) | ((size & 0xff00) >> 8)) + 1;

			//  Read the rest of the sync frame
			uint32_t len = (size - 3) * 2;
			bytes = mConfig.fDolbyFile->Read((uint8_t*)(&mDolbyBuffer[3]), len);
			if (bytes != len)
				goto silence;

			//  Good frame
			mBurstOffset = 0;
			mBurstSize = size;
		}

		//  Add the Dolby data to the audio stream
		if (mBurstOffset < mBurstSize)
		{
			uint32_t data0 = 0;
			uint32_t data1 = 0;
			if (mBurstIndex == 0)
			{
				//  Add IEC61937 burst preamble
				data0 = 0xf872;             //  Sync stuff
				data1 = 0x4e1f;
			}
			else if (mBurstIndex == 1)
			{
				//  Add more IEC61937 burst preamble
				data0 = 0x0015;             //  This is Dolby
				data1 = mBurstSize * 2;     //  Data size in bytes
			}
			else
			{
				//  Add sync frame data
				data0 = (uint32_t)(mDolbyBuffer[mBurstOffset]);
				data0 = ((data0 & 0x00ff) << 8) | ((data0 & 0xff00) >> 8);
				mBurstOffset++;
				data1 = (uint32_t)(mDolbyBuffer[mBurstOffset]);
				data1 = ((data1 & 0x00ff) << 8) | ((data1 & 0xff00) >> 8);
				mBurstOffset++;
			}

			//  Write data into 16 msbs of all audio channel pairs
			data0 <<= 16;
			data1 <<= 16;
			for (ULWord i = 0; i < numChannels; i += 2)
			{
				pInAudioBuffer[sampleOffset * numChannels + i] = data0;
				pInAudioBuffer[sampleOffset * numChannels + i + 1] = data1;
			}
		}
		else
		{
			//  Pad samples out to burst size
			for (ULWord i = 0; i < numChannels; i++)
			{
				pInAudioBuffer[sampleOffset * numChannels + i] = 0;
			}
		}

		sampleOffset++;
		mBurstIndex++;
	}

	return numSamples * numChannels * 4;

silence:
	//  Output silence when done with file
	memset(&pInAudioBuffer[sampleOffset * numChannels], 0, (numSamples - sampleOffset) * numChannels * 4);
	return numSamples * numChannels * 4;
}

#endif

AJALabelValuePairs DolbyPlayerConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result;
	AJASystemInfo::append (result,	"NTV2DolbyPlayer Config");
	AJASystemInfo::append (result,		"Device Specifier",		fDeviceSpec);
	AJASystemInfo::append (result,		"Video Format",			::NTV2VideoFormatToString(fVideoFormat));
	AJASystemInfo::append (result,		"Pixel Format",			::NTV2FrameBufferFormatToString(fPixelFormat, inCompact));
	AJASystemInfo::append (result,		"AutoCirc Frames",		fFrames.toString());
	AJASystemInfo::append (result,		"MultiFormat Mode",		fDoMultiFormat ? "Y" : "N");
	AJASystemInfo::append (result,		"VANC Mode",			::NTV2VANCModeToString(fVancMode));
	AJASystemInfo::append (result,		"HDR Anc Type",			::AJAAncDataTypeToString(fTransmitHDRType));
	AJASystemInfo::append (result,		"Output Channel",		::NTV2ChannelToString(fOutputChannel, inCompact));
	AJASystemInfo::append (result,		"Output Connector",		::NTV2OutputDestinationToString(fOutputDest, inCompact));
	AJASystemInfo::append (result,		"HDMI Output",			fDoHDMIOutput ? "Yes" : "No");
	AJASystemInfo::append (result,		"Dolby Playback File", 	fDolbyFilePath);
	AJASystemInfo::append (result,		"Aux Data Ramp",		fdoRamp ? "Yes" : "No");
	AJASystemInfo::append (result,		"Num Audio Links",		aja::to_string(fNumAudioLinks));
	AJASystemInfo::append (result,		"Suppress Video",		fSuppressVideo ? "Y" : "N");
	AJASystemInfo::append (result,		"Suppress Audio",		fSuppressAudio ? "Y" : "N");
	//AJASystemInfo::append (result,		"Embedded Timecode",	fTransmitLTC ? "LTC" : "VITC");
	//AJASystemInfo::append (result,		"Level Conversion",		fDoABConversion ? "Y" : "N");
	//AJASystemInfo::append (result,		"RGB-On-SDI",			fDoRGBOnWire ? "Yes" : "No");
	//AJASystemInfo::append (result,		"TSI Routing",			fDoTsiRouting ? "Yes" : "No");
	//AJASystemInfo::append (result,		"6G/12G Output",		fDoLinkGrouping ? "Yes" : "No");

	return result;
}

std::ostream & operator << (std::ostream & ioStrm,  const DolbyPlayerConfig & inObj)
{
	ioStrm	<< AJASystemInfo::ToString(inObj.Get());
	return ioStrm;
}
