/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2player.cpp
	@brief		Implementation of NTV2Player class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2player.h"
#include "ntv2debug.h"
#include "ntv2devicescanner.h"
#include "ntv2audiodefines.h"
#include "ntv2testpatterngen.h"
#include "ajabase/common/timebase.h"
#include "ajabase/system/process.h"
#include "ajabase/system/file_io.h"
#include "ajaanc/includes/ancillarydata_hdr_sdr.h"
#include "ajaanc/includes/ancillarydata_hdr_hdr10.h"
#include "ajaanc/includes/ancillarydata_hdr_hlg.h"
#include <fstream>	//	For ifstream

using namespace std;

//#define NTV2_BUFFER_LOCKING		//	Define this to pre-lock video/audio buffers in kernel

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


NTV2Player::NTV2Player (const PlayerConfig & inConfig)
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
		mTestPatRasters		()
{
}


NTV2Player::~NTV2Player (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit();

	mDevice.UnsubscribeOutputVerticalEvent(mConfig.fOutputChannel);	//	Unsubscribe from output VBI event
}	//	destructor


void NTV2Player::Quit (void)
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


AJAStatus NTV2Player::Init (void)
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

	if (!mConfig.fAncDataFilePath.empty())
		if (!AJAFileIO::FileExists (mConfig.fAncDataFilePath))
		{
			cerr	<< "## ERROR:  Anc data file '" << mConfig.fAncDataFilePath << "' not found" << endl;
			return AJA_STATUS_NOT_FOUND;
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


AJAStatus NTV2Player::SetUpVideo (void)
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

	//  If we own the whole device, make sure all FrameStores are disabled (good practice) 
	if (!mConfig.fDoMultiFormat)
	{
		NTV2ChannelSet enabledChannels;
		mDevice.GetEnabledChannels(enabledChannels);
		mDevice.DisableChannels(enabledChannels);
	}

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

	//	Get current per-field maximum Anc buffer size...
	if (!mDevice.GetAncRegionOffsetFromBottom (gAncMaxSizeBytes, NTV2_AncRgn_Field2))
		gAncMaxSizeBytes = NTV2_ANCSIZE_MAX;

	//	Set output clock reference...
	mDevice.SetReference(mDevice.features().CanDo2110() ? NTV2_REFERENCE_SFP1_PTP : NTV2_REFERENCE_FREERUN);

	//	At this point, video setup is complete (except for widget signal routing).
	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2Player::SetUpAudio (void)
{
	uint16_t numAudioChannels (mDevice.features().GetMaxAudioChannels());
	if (numAudioChannels > 8)										//	If audio system handles more than 8 channels...
		if (!mDevice.features().CanDo2110())						//	...and SDI (i.e. not ST 2110 IP streaming)...
			if (NTV2_IS_2K_1080_VIDEO_FORMAT(mConfig.fVideoFormat))	//	...and 2K (narrower HANC only fits 8 audio channels)
				numAudioChannels = 8;	//	...then reduce to 8 audio channels

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


AJAStatus NTV2Player::SetUpHostBuffers (void)
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
		#ifdef NTV2_BUFFER_LOCKING
		if (frameData.fVideoBuffer)
			mDevice.DMABufferLock(frameData.fVideoBuffer, true);
		#endif

		//	Allocate a page-aligned audio buffer (if transmitting audio)
		if (mConfig.WithAudio())
			if (!frameData.fAudioBuffer.Allocate (gAudMaxSizeBytes, BUFFER_PAGE_ALIGNED))
			{
				PLFAIL("Failed to allocate " << xHEX0N(gAudMaxSizeBytes,8) << "-byte audio buffer");
				return AJA_STATUS_MEMORY;
			}
		#ifdef NTV2_BUFFER_LOCKING
		if (frameData.fAudioBuffer)
			mDevice.DMABufferLock(frameData.fAudioBuffer, /*alsoPreLockSGL*/true);
		#endif
		mFrameDataRing.Add (&frameData);
	}	//	for each NTV2FrameData

	return AJA_STATUS_SUCCESS;

}	//	SetUpHostBuffers


AJAStatus NTV2Player::SetUpTestPatternBuffers (void)
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


bool NTV2Player::RouteOutputSignal (void)
{
	const NTV2Standard	outputStandard	(::GetNTV2StandardFromVideoFormat(mConfig.fVideoFormat));
	const bool			isRGB			(::IsRGBFormat(mConfig.fPixelFormat));
	const bool			canVerify		(mDevice.features().HasCrosspointConnectROM());
	UWord				connectFailures	(0);

	//	Since this function figures out which SDI spigots will be set up for output,
	//	it also sets the "mTCIndexes" member, which determines which timecodes will
	//	be transmitted (and on which SDI spigots)...
	mTCIndexes.clear();

	const NTV2OutputXptID	cscVidOutXpt(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel,  false/*isKey*/,  !isRGB/*isRGB*/));
	const NTV2OutputXptID	fsVidOutXpt (::GetFrameBufferOutputXptFromChannel(mConfig.fOutputChannel,  isRGB/*isRGB*/,  false/*is425*/));
	const NTV2InputXptID	cscInputXpt (isRGB ? ::GetCSCInputXptFromChannel(mConfig.fOutputChannel, false/*isKeyInput*/) : NTV2_INPUT_CROSSPOINT_INVALID);

	if (!mConfig.fDoMultiFormat)  //	Not multiformat:  We own the whole device...
	{
		mDevice.ClearRouting();		//	Start with clean slate
	}

	if (isRGB)
		if (!mDevice.Connect (cscInputXpt,  fsVidOutXpt,  canVerify))
			connectFailures++;

	if (mDevice.features().HasBiDirectionalSDI())
		mDevice.SetSDITransmitEnable(mConfig.fOutputChannel, true);

	if (!mDevice.Connect (::GetSDIOutputInputXpt (mConfig.fOutputChannel, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
		connectFailures++;

	mDevice.SetSDIOutputStandard (mConfig.fOutputChannel, outputStandard);

	mTCIndexes.insert (::NTV2ChannelToTimecodeIndex(mConfig.fOutputChannel, /*inEmbeddedLTC=*/mConfig.fTransmitLTC));
	//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically

	if (!mConfig.fDoMultiFormat)  //	Not multiformat:  We own the whole device...
	{
		//	Also connect analog video output, if the device has one...
		if (mDevice.features().GetNumAnalogVideoOutputs() && mConfig.fOutputChannel == NTV2_CHANNEL1)
			if (!mDevice.Connect (::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_ANALOG1),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
				connectFailures++;

		//	Also connect HDMI video output, if the device has one...
		if (mDevice.features().GetNumHDMIVideoOutputs() && mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			if (!mDevice.Connect (::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_HDMI1),  isRGB ? cscVidOutXpt : fsVidOutXpt,  canVerify))
				connectFailures++;
		}
	}
	TCNOTE(mTCIndexes);
	if (connectFailures)
		PLWARN(DEC(connectFailures) << " 'Connect' call(s) failed");
	return connectFailures == 0;

}	//	RouteOutputSignal


AJAStatus NTV2Player::Run (void)
{
	//	Start my consumer and producer threads...
	StartConsumerThread();
	StartProducerThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2Player::StartConsumerThread (void)
{
	//	Create and start the playout thread...
	mConsumerThread.Attach (ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The playout thread function
void NTV2Player::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Player instance pointer from the pContext parameter,
	//	then call its ConsumeFrames method...
	NTV2Player * pApp (reinterpret_cast<NTV2Player*>(pContext));
	if (pApp)
		pApp->ConsumeFrames();

}	//	ConsumerThreadStatic


static uint64_t sTotalAncFileBytes(0), sCurrentAncFileBytes(0);


void NTV2Player::ConsumeFrames (void)
{
	ULWord					acOptions (AUTOCIRCULATE_WITH_RP188);
	AUTOCIRCULATE_TRANSFER	outputXfer;
	AUTOCIRCULATE_STATUS	outputStatus;
	AJAAncillaryData *		pPkt (AJA_NULL);
	ULWord					goodXfers(0), badXfers(0), starves(0), noRoomWaits(0);
	ifstream *				pAncStrm (AJA_NULL);

	//	Stop AutoCirculate, just in case someone else left it running...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel, 4);	//	Let it stop
	PLNOTE("Thread started");

	if (pPkt)
	{	//	Allocate page-aligned host Anc buffer...
		uint32_t hdrPktSize	(0);
		if (!outputXfer.acANCBuffer.Allocate(gAncMaxSizeBytes, BUFFER_PAGE_ALIGNED)  ||  !outputXfer.acANCBuffer.Fill(0LL))
			PLWARN("Anc buffer " << xHEX0N(gAncMaxSizeBytes,8) << "(" << DEC(gAncMaxSizeBytes) << ")-byte allocate failed -- HDR anc insertion disabled");
		else if (AJA_FAILURE(pPkt->GenerateTransmitData (outputXfer.acANCBuffer, outputXfer.acANCBuffer,  hdrPktSize)))
		{
			PLWARN("HDR anc insertion disabled -- GenerateTransmitData failed");
			outputXfer.acANCBuffer.Deallocate();
		}
		else
			acOptions |= AUTOCIRCULATE_WITH_ANC;
	}
	else if (!mConfig.fAncDataFilePath.empty())
	{	//	Open raw anc file for reading...
		pAncStrm = new ifstream(mConfig.fAncDataFilePath.c_str(), ios::binary);
		do
		{
			if (!pAncStrm->good())
			{	PLFAIL("Cannot open anc file '" << mConfig.fAncDataFilePath << "' -- anc insertion disabled");
				delete pAncStrm;  pAncStrm = AJA_NULL;
				break;
			}
			pAncStrm->seekg(0, std::ios_base::end);
			if (pAncStrm->tellg() == -1)
			{	PLFAIL("Cannot determine anc file size -- anc insertion disabled");
				delete pAncStrm;  pAncStrm = AJA_NULL;
				break;
			}
			sTotalAncFileBytes = uint64_t(pAncStrm->tellg());
			pAncStrm->seekg(0);	//	Rewind to start
			if (sTotalAncFileBytes < gAncMaxSizeBytes*2)
			{	PLFAIL("anc buffer size " << DEC(gAncMaxSizeBytes*2) << " exceeds anc file size " << DEC(sTotalAncFileBytes)
						<< " -- anc insertion disabled");
				delete pAncStrm;  pAncStrm = AJA_NULL;
				break;
			}
			//	Allocate anc buffers...
			bool ancOK = outputXfer.acANCBuffer.Allocate(gAncMaxSizeBytes, BUFFER_PAGE_ALIGNED);
			if (NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mConfig.fVideoFormat)  &&  ancOK)
				ancOK = outputXfer.acANCField2Buffer.Allocate(gAncMaxSizeBytes, BUFFER_PAGE_ALIGNED);
			if (!ancOK)
			{
				PLFAIL("Anc buffer " << xHEX0N(gAncMaxSizeBytes,8) << "(" << DEC(gAncMaxSizeBytes) << ")-byte"
						" allocation failed -- anc insertion disabled");
				outputXfer.acANCBuffer.Deallocate();
				outputXfer.acANCField2Buffer.Deallocate();
				delete pAncStrm;
				pAncStrm = AJA_NULL;
				break;
			}
			acOptions |= AUTOCIRCULATE_WITH_ANC;
			break;	//	Good to go!
		} while (false);	//	Once thru
	}
#ifdef NTV2_BUFFER_LOCKING
	if (outputXferInfo.acANCBuffer)
		mDevice.DMABufferLock(outputXfer.acANCBuffer, /*alsoLockSGL*/true);
	if (outputXferInfo.acANCField2Buffer)
		mDevice.DMABufferLock(outputXfer.acANCField2Buffer, /*alsoLockSGL*/true);
#endif

	//	Initialize & start AutoCirculate...
	bool initOK = mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,  mConfig.fFrames,  mAudioSystem,  acOptions);
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

			if (pAncStrm  &&  pAncStrm->good()  &&  outputXfer.acANCBuffer)
			{	//	Read pre-recorded anc from binary data file, and inject it into this frame...
				pAncStrm->read(outputXfer.acANCBuffer, streamsize(outputXfer.acANCBuffer.GetByteCount()));
				sCurrentAncFileBytes += pAncStrm->good() ? outputXfer.acANCBuffer.GetByteCount() : 0;
				if (pAncStrm->good()  &&  outputXfer.acANCField2Buffer)
				{
					pAncStrm->read(outputXfer.acANCField2Buffer, streamsize(outputXfer.acANCField2Buffer.GetByteCount()));
					sCurrentAncFileBytes += pAncStrm->good() ? outputXfer.acANCField2Buffer.GetByteCount() : 0;
				}
				if (pAncStrm->eof())
				{	pAncStrm->clear();
					pAncStrm->seekg(0);	//	Rewind upon EOF
					sCurrentAncFileBytes = 0;
					PLDBG("Anc file rewound: " << DEC(goodXfers) << " goodXfers, " << DEC(badXfers) << " badXfers");
				}
			}	//	if playing anc from binary data file

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
		}	//	if CanAcceptMoreOutputFrames

		//	Wait for one or more buffers to become available on the device, which should occur at next VBI...
		noRoomWaits++;
		mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel);
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	PLNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(starves) << " starves, " << DEC(noRoomWaits) << " VBI waits");
	if (pAncStrm)
		delete pAncStrm;

}	//	ConsumeFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2Player::StartProducerThread (void)
{
	//	Create and start the producer thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


void NTV2Player::ProducerThreadStatic (AJAThread * pThread, void * pContext)	//	static
{	(void) pThread;
	NTV2Player * pApp (reinterpret_cast<NTV2Player*>(pContext));
	if (pApp)
		pApp->ProduceFrames();

}	//	ProducerThreadStatic


void NTV2Player::ProduceFrames (void)
{
	ULWord	freqNdx(0), testPatNdx(0), badTally(0);
	double	timeOfLastSwitch	(0.0);

	const AJATimeBase		timeBase		(CNTV2DemoCommon::GetAJAFrameRate(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat)));
	const NTV2StringList	tpNames			(NTV2TestPatternGen::getTestPatternNames());
	const bool				isInterlace		(!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mConfig.fVideoFormat));
	const bool				isPAL			(NTV2_IS_PAL_VIDEO_FORMAT(mConfig.fVideoFormat));
	const NTV2FrameRate		ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
	const TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));

	PLNOTE("Thread started");
	while (!mGlobalQuit)
	{
		NTV2FrameData *	pFrameData (mFrameDataRing.StartProduceNextBuffer());
		if (!pFrameData)
		{	badTally++;			//	No frame available!
			AJATime::Sleep(10);	//	Wait a bit for the consumer thread to free one up for me...
			continue;			//	...then try again
		}

		if (pFrameData->VideoBuffer())	//	Copy fresh, unmodified, pre-rendered test pattern into this frame's video buffer...
			pFrameData->fVideoBuffer.CopyFrom (mTestPatRasters.at(testPatNdx),
												/*srcOffset*/ 0,
												/*dstOffset*/ 0,
												/*byteCount*/ pFrameData->fVideoBuffer.GetByteCount());

		const	CRP188	rp188Info (mCurrentFrame++, 0, 0, 10, tcFormat);
		NTV2_RP188		tcF1, tcF2;
		string			tcString;

		rp188Info.GetRP188Reg(tcF1);
		rp188Info.GetRP188Str(tcString);

		//	Include timecode in output signal...
		tcF2 = tcF1;
		if (isInterlace)
		{	//	Set bit 27 of Hi word (PAL) or Lo word (NTSC)
			if (isPAL) tcF2.fHi |=  BIT(27);	else tcF2.fLo |=  BIT(27);
		}

		//	Add timecodes for each SDI output...
		for (NTV2TCIndexesConstIter it(mTCIndexes.begin());  it != mTCIndexes.end();  ++it)
			pFrameData->fTimecodes[*it] = NTV2_IS_ATC_VITC2_TIMECODE_INDEX(*it) ? tcF2 : tcF1;

		if (pFrameData->VideoBuffer())	//	Burn current timecode into the video buffer...
			mTCBurner.BurnTimeCode (pFrameData->VideoBuffer(), tcString.c_str(), 80);
		TCDBG("F" << DEC0N(mCurrentFrame-1,6) << ": " << tcF1 << ": " << tcString);

		//	If also playing audio...
		if (pFrameData->AudioBuffer())	//	...then generate audio tone data for this frame...
			pFrameData->fNumAudioBytes = AddTone(*pFrameData);	//	...and remember number of audio bytes to xfer

		//	Every few seconds, change the test pattern and tone frequency...
		const double currentTime (timeBase.FramesToSeconds(mCurrentFrame));
		if (currentTime > timeOfLastSwitch + 4.0)
		{
			freqNdx = (freqNdx + 1) % gNumFrequencies;
			testPatNdx = (testPatNdx + 1) % ULWord(mTestPatRasters.size());
			mToneFrequency = gFrequencies[freqNdx];
			timeOfLastSwitch = currentTime;
			if (sTotalAncFileBytes)
				PLINFO("F" << DEC0N(mCurrentFrame,6) << ": " << tcString << ": tone=" << mToneFrequency << "Hz, pattern='"
						<< tpNames.at(testPatNdx) << "', anc file " << DEC(sCurrentAncFileBytes * 100ULL / sTotalAncFileBytes) << "%");
			else
				PLINFO("F" << DEC0N(mCurrentFrame,6) << ": " << tcString << ": tone=" << mToneFrequency << "Hz, pattern='"
						<< tpNames.at(testPatNdx) << "'");
		}	//	if time to switch test pattern & tone frequency

		//	Signal that I'm done producing this FrameData, making it immediately available for transfer/playout...
		mFrameDataRing.EndProduceNextBuffer();

	}	//	loop til mGlobalQuit goes true
	PLNOTE("Thread completed: " << DEC(mCurrentFrame) << " frame(s) produced, " << DEC(badTally) << " failed");

}	//	ProduceFrames


uint32_t NTV2Player::AddTone (NTV2FrameData & inFrameData)
{
	NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
	NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
	ULWord			numChannels	(0);

	mDevice.GetFrameRate (frameRate, mConfig.fOutputChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);

	//	Set per-channel tone frequencies...
	double	pFrequencies [kNumAudioChannelsMax];
	pFrequencies[0] = (mToneFrequency / 2.0);
	for (ULWord chan(1);  chan < numChannels;  chan++)
		//	The 1.154782 value is the 16th root of 10, to ensure that if mToneFrequency is 2000,
		//	that the calculated frequency of audio channel 16 will be 20kHz...
		pFrequencies[chan] = pFrequencies[chan-1] * 1.154782;

	//	Since audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will
	//	necessarily result in some frames having more audio samples than others. GetAudioSamplesPerFrame is
	//	called to calculate the correct sample count for the current frame...
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));
	const double	sampleRateHertz	(::GetAudioSamplesPerSecond(audioRate));

	return ::AddAudioTone (	inFrameData.AudioBuffer(),	//	audio buffer to fill
							mCurrentSample,				//	which sample for continuing the waveform
							numSamples,					//	number of samples to generate
							sampleRateHertz,			//	sample rate [Hz]
							gAmplitudes,				//	per-channel amplitudes
							pFrequencies,				//	per-channel tone frequencies [Hz]
							31,							//	bits per sample
							false,						//	don't byte-swap
							numChannels);				//	number of audio channels to generate
}	//	AddTone


void NTV2Player::GetACStatus (AUTOCIRCULATE_STATUS & outStatus)
{
	mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outStatus);
}
