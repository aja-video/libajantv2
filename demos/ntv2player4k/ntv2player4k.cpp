/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2player4k.cpp
	@brief		Implementation of ntv2player4k class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2player4k.h"
#include "ntv2debug.h"
#include "ntv2devicescanner.h"
#include "ntv2testpatterngen.h"
#include "ajabase/common/timebase.h"
#include "ajabase/system/process.h"
#include "ajaanc/includes/ancillarydata_hdr_sdr.h"
#include "ajaanc/includes/ancillarydata_hdr_hdr10.h"
#include "ajaanc/includes/ancillarydata_hdr_hlg.h"

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
//	Unlike NTV2Player, this demo uses the same waveform amplitude in each audio channel


NTV2Player4K::NTV2Player4K (const PlayerConfig & inConfig)
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


NTV2Player4K::~NTV2Player4K (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit();

	mDevice.UnsubscribeOutputVerticalEvent(mConfig.fOutputChannel);	//	Unsubscribe from output VBI event
}	//	destructor


void NTV2Player4K::Quit (void)
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


AJAStatus NTV2Player4K::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}
	mDeviceID = mDevice.GetDeviceID();	//	Keep this ID handy -- it's used frequently

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  '" << mDevice.GetDisplayName() << "' is capture-only" << endl;  return AJA_STATUS_FEATURE;}

	const UWord maxNumChannels (mDevice.features().GetNumFrameStores());

	//	Check for an invalid configuration
	if (NTV2_IS_4K_HFR_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  mConfig.fDoRGBOnWire)
		{cerr << "## ERROR:  HFR RGB output not supported" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Check for valid channel...
	if (UWord(mConfig.fOutputChannel) >= maxNumChannels)
	{
		cerr	<< "## ERROR:  Cannot use channel '" << DEC(mConfig.fOutputChannel+1) << "' -- device only supports channel 1"
				<< (maxNumChannels > 1  ?  string(" thru ") + string(1, char(maxNumChannels+'0'))  :  "") << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	if (mDevice.features().CanDo12gRouting())
	{
		if (mConfig.fDoTsiRouting)
			cerr	<< "## WARNING:  '--tsi' option used with device that has 12G FrameStores with integral Tsi muxers" << endl;
		mConfig.fDoTsiRouting = false;	//	Kona5 & Corvid44/12G FrameStores have built-in TSI muxers for easier routing
	}
	else if (mConfig.fDoTsiRouting)
	{
		if (mConfig.fOutputChannel & 1)
		{
			cerr	<< "## ERROR:  Tsi requires Ch" << DEC(mConfig.fOutputChannel-1) << " -- not Ch" << DEC(mConfig.fOutputChannel) << endl;
			return AJA_STATUS_BAD_PARAM;
		}
	}
	else if (mConfig.fOutputChannel != NTV2_CHANNEL1 && mConfig.fOutputChannel != NTV2_CHANNEL5)
	{
		cerr	<< "## ERROR:  Squares requires Ch1 or Ch5, not Ch" << DEC(mConfig.fOutputChannel) << endl;
		return AJA_STATUS_BAD_PARAM;
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


AJAStatus NTV2Player4K::SetUpVideo (void)
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

	NTV2ChannelSet channels1357, channels15, frameStores;
	channels1357.insert(NTV2_CHANNEL1);  channels1357.insert(NTV2_CHANNEL3);  channels1357.insert(NTV2_CHANNEL5);  channels1357.insert(NTV2_CHANNEL7);
	channels15.insert(NTV2_CHANNEL1);  channels15.insert(NTV2_CHANNEL5);
	if (mDevice.features().CanDo12gRouting())
		frameStores = ::NTV2MakeChannelSet (mConfig.fOutputChannel, 1);	//	1 x 12G FrameStore (fOutputChannel)
	else if (mConfig.fDoTsiRouting)
	{	//	"Tsi" routing requires 2 FrameStores
		if (channels1357.find(mConfig.fOutputChannel) == channels1357.end())
			return AJA_STATUS_BAD_PARAM;	//	fOutputChannel not Ch1, Ch3, Ch5 or Ch7
		frameStores = ::NTV2MakeChannelSet (mConfig.fOutputChannel, 2);	//	2 x FrameStores starting at Ch1, Ch3, Ch5 or Ch7
	}
	else
	{	//	"Squares" routing requires 4 FrameStores
		if (channels15.find(mConfig.fOutputChannel) == channels15.end())
			return AJA_STATUS_BAD_PARAM;	//	fOutputChannel not Ch1 or Ch5
		frameStores = ::NTV2MakeChannelSet (mConfig.fOutputChannel, 4);	//	4 x FrameStores starting at Ch1 or Ch5
	}
	//  Disable SDI output conversions
	mDevice.SetSDIOutLevelAtoLevelBConversion (frameStores, false);
	mDevice.SetSDIOutRGBLevelAConversion (frameStores, false);

	//	Keep the raster description handy...
	mFormatDesc = NTV2FormatDescriptor(mConfig.fVideoFormat, mConfig.fPixelFormat);
	if (!mFormatDesc.IsValid())
		return AJA_STATUS_FAIL;

	//	Turn on the FrameStores (to read frame buffer memory and transmit video)...
	mDevice.EnableChannels (frameStores, /*disableOthers=*/!mConfig.fDoMultiFormat);

	//	This demo requires VANC be disabled...
	mDevice.SetVANCMode (frameStores, NTV2_VANCMODE_OFF);	//	VANC is incompatible with 4K/UHD formats
	mDevice.SetVANCShiftMode(frameStores, NTV2_VANCDATA_NORMAL);

	//	Set the FrameStore video format...
	mDevice.SetVideoFormat (frameStores, mConfig.fVideoFormat, false);

	//	Set the frame buffer pixel format for the device FrameStore(s)...
	mDevice.SetFrameBufferFormat (frameStores, mConfig.fPixelFormat);

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


AJAStatus NTV2Player4K::SetUpAudio (void)
{
	uint16_t numAudioChannels (mDevice.features().GetMaxAudioChannels());
	if (numAudioChannels > 8)										//	If audio system handles more than 8 channels...
		if (!mDevice.features().CanDo2110())						//	...and SDI (i.e. not ST 2110 IP streaming)...
			if (NTV2_IS_4K_4096_VIDEO_FORMAT(mConfig.fVideoFormat))	//	...and 4K (narrower HANC only fits 8 audio channels)
				numAudioChannels = 8;	//	...then reduce to 8 audio channels

	//	Use the NTV2AudioSystem that has the same ordinal value as the output FrameStore/Channel...
	mAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fOutputChannel);

	if (mConfig.fNumAudioLinks > 1)	//	For users that want to send 32 or 64 audio channels on 2 or 4 SDI links
		switch (mAudioSystem)
		{
			default:
			case NTV2_AUDIOSYSTEM_1:
			{	const UWord numChan(NTV2_IS_4K_HFR_VIDEO_FORMAT(mConfig.fVideoFormat) ? 4 : 2);
				const NTV2AudioSystemSet audSystems (::NTV2MakeAudioSystemSet (mAudioSystem, numChan));
				for (UWord chan(0);  chan < numChan;  chan++)
					mDevice.SetSDIOutputAudioSystem (NTV2Channel(chan), NTV2AudioSystem(chan));
				mDevice.SetNumberAudioChannels (numAudioChannels, audSystems);
				mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, audSystems);
				mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, audSystems);
				break;
			}
			case NTV2_AUDIOSYSTEM_3:
				mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL3, NTV2_AUDIOSYSTEM_3);
				mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL4, NTV2_AUDIOSYSTEM_4);
				break;
		}
	else
	{
		NTV2ChannelSet sdiSpigots (::NTV2MakeChannelSet (mConfig.fOutputChannel, 1));
		if (!mDevice.features().CanDo12gRouting())
			sdiSpigots = ::NTV2MakeChannelSet (mAudioSystem == NTV2_AUDIOSYSTEM_1 || mAudioSystem == NTV2_AUDIOSYSTEM_3
												? NTV2_CHANNEL1
												: NTV2_CHANNEL5,  4);
		mDevice.SetSDIOutputAudioSystem (sdiSpigots, mAudioSystem);
		mDevice.SetNumberAudioChannels (numAudioChannels, mAudioSystem);
		mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);
		mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);
	}

	if (mConfig.fDoHDMIOutput)
	{
		mDevice.SetHDMIOutAudioRate(NTV2_AUDIO_48K);
		mDevice.SetHDMIOutAudioFormat(NTV2_AUDIO_FORMAT_LPCM);
		mDevice.SetHDMIOutAudioSource8Channel(NTV2_AudioChannel1_8, mAudioSystem);
	}

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio


AJAStatus NTV2Player4K::SetUpHostBuffers (void)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Let my circular buffer know when it's time to quit...
	mFrameDataRing.SetAbortFlag (&mGlobalQuit);

	//	Multi-link audio uses stacked buffers for transferring to the board,
	//	the first byte after the end of the first audio link buffer is the start of the second audio link buffer.
	const size_t audioBufferSize (gAudMaxSizeBytes * uint32_t(mConfig.fNumAudioLinks));

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());		//	Make a new NTV2FrameData...
		NTV2FrameData & frameData(mHostBuffers.back());	//	...and get a reference to it

		//	Don't allocate a page-aligned video buffer here.
		//	Instead, the test pattern buffers are used (and re-used) in the consumer thread.
		//	This saves a LOT of memory and time spent copying data with these large 4K/UHD rasters.
		//	NOTE:	This differs substantially from the NTV2Player demo, which pre-allocates the ring of video buffers
		//			here, then in its producer thread, copies a fresh, unmodified test pattern raster into the video
		//			buffer, blits timecode into it, then transfers it to the hardware in its consumer thread.

		//	Allocate a page-aligned audio buffer (if transmitting audio)
		if (mConfig.WithAudio())
			if (!frameData.fAudioBuffer.Allocate (audioBufferSize, BUFFER_PAGE_ALIGNED))
			{
				PLFAIL("Failed to allocate " << xHEX0N(audioBufferSize,8) << "-byte audio buffer");
				return AJA_STATUS_MEMORY;
			}
		if (frameData.fAudioBuffer)
		{
			frameData.fAudioBuffer.Fill(ULWord(0));
			#ifdef NTV2_BUFFER_LOCKING
				mDevice.DMABufferLock(frameData.fAudioBuffer, /*alsoPreLockSGL*/true);
			#endif
		}
		mFrameDataRing.Add (&frameData);
	}	//	for each NTV2FrameData

	return AJA_STATUS_SUCCESS;

}	//	SetUpHostBuffers


AJAStatus NTV2Player4K::SetUpTestPatternBuffers (void)
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
		testPatIDs.push_back(NTV2_TestPatt_Black);
		testPatIDs.push_back(NTV2_TestPatt_White);
		testPatIDs.push_back(NTV2_TestPatt_Border);
		testPatIDs.push_back(NTV2_TestPatt_LinearRamp);
		testPatIDs.push_back(NTV2_TestPatt_SlantRamp);
		testPatIDs.push_back(NTV2_TestPatt_ZonePlate);
		testPatIDs.push_back(NTV2_TestPatt_ColorQuadrant);
		testPatIDs.push_back(NTV2_TestPatt_ColorQuadrantBorder);

	mTestPatRasters.clear();
	for (size_t tpNdx(0);  tpNdx < testPatIDs.size();  tpNdx++)
		mTestPatRasters.push_back(NTV2Buffer());

	if (!mFormatDesc.IsValid())
		{PLFAIL("Bad format descriptor");  return AJA_STATUS_FAIL;}
	if (mFormatDesc.IsVANC())
		{PLFAIL("VANC incompatible with UHD/4K: " << mFormatDesc);  return AJA_STATUS_FAIL;}

	//	Set up one video buffer for each test pattern...
	for (size_t tpNdx(0);  tpNdx < testPatIDs.size();  tpNdx++)
	{
		//	Allocate the buffer memory...
		if (!mTestPatRasters.at(tpNdx).Allocate (mFormatDesc.GetVideoWriteSize(), BUFFER_PAGE_ALIGNED))
		{	PLFAIL("Test pattern buffer " << DEC(tpNdx+1) << " of " << DEC(testPatIDs.size()) << ": "
					<< xHEX0N(mFormatDesc.GetVideoWriteSize(),8) << "-byte page-aligned alloc failed");
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


bool NTV2Player4K::RouteOutputSignal (void)
{
	const bool	isRGB (::IsRGBFormat(mConfig.fPixelFormat));
	bool useLinkGrouping = false;
	UWord failures(0);	//	tally of routing failures

	if (!mConfig.fDoMultiFormat)
		mDevice.ClearRouting();	//	Replace current signal routing

	//	Construct switch value to avoid multiple if-then-else
	int switchValue = 0;
	if (NTV2_IS_4K_HFR_VIDEO_FORMAT(mConfig.fVideoFormat))
		switchValue += 8;
	if (mConfig.fDoTsiRouting)
		switchValue += 4;
	if (isRGB)
		switchValue += 2;
	if (mConfig.fDoRGBOnWire)
		switchValue += 1;
	if (mDevice.features().CanDo12GSDI() && !mDevice.features().CanDo12gRouting())
	{
		mDevice.SetSDIOut6GEnable(NTV2_CHANNEL3, false);
		mDevice.SetSDIOut12GEnable(NTV2_CHANNEL3, false);
		if (mConfig.fDoLinkGrouping)
			useLinkGrouping = true;
	}

	switch (switchValue)
	{
		case 0:		//	Low Frame Rate, Square, Pixel YCbCr, Wire YCbCr
			if (!RouteFsToSDIOut()) failures++;
			SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 1:		//	Low Frame Rate, Square, Pixel YCbCr, Wire RGB
			if (!RouteFsToCsc()) failures++;
			if (!RouteCscToDLOut()) failures++;
			if (!RouteDLOutToSDIOut()) failures++;
			SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 2:		//	Low Frame Rate, Square, Pixel RGB, Wire YCbCr
			if (!RouteFsToCsc()) failures++;
			if (!RouteCscTo4xSDIOut()) failures++;
			SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 3:		//	Low Frame Rate, Square, Pixel RGB, Wire RGB
			if (!RouteFsToDLOut()) failures++;
			if (!RouteDLOutToSDIOut()) failures++;
			SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 4:		//	Low Frame Rate, Tsi, Pixel YCbCr, Wire YCbCr
			if (!RouteFsToTsiMux()) failures++;
			if (!RouteTsiMuxTo2xSDIOut()) failures++;
			if (useLinkGrouping)
			{
				mDevice.SetSDIOut6GEnable(NTV2_CHANNEL3, true);
				SetupSDITransmitters(NTV2_CHANNEL3, 1);
			}
			else
				SetupSDITransmitters(mConfig.fOutputChannel, 2);
			break;
		case 5:		//	Low Frame Rate, Tsi, Pixel YCbCr, Wire RGB
			if (!RouteFsToTsiMux()) failures++;
			if (!RouteTsiMuxToCsc()) failures++;
			if (!RouteCscToDLOut()) failures++;
			if (!RouteDLOutToSDIOut()) failures++;
			if (useLinkGrouping)
			{
				mDevice.SetSDIOut12GEnable(NTV2_CHANNEL3, true);
				SetupSDITransmitters(NTV2_CHANNEL3, 1);
			}
			else
				SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 6:		//	Low Frame Rate, Tsi, Pixel RGB, Wire YCbCr
			if (!RouteFsToTsiMux()) failures++;
			if (!RouteTsiMuxToCsc()) failures++;
			if (!RouteCscTo2xSDIOut()) failures++;
			if (useLinkGrouping)
			{
				mDevice.SetSDIOut6GEnable(NTV2_CHANNEL3, true);
				SetupSDITransmitters(NTV2_CHANNEL3, 1);
			}
			else
				SetupSDITransmitters(mConfig.fOutputChannel, 2);
			break;
		case 7:		//	Low Frame Rate, Tsi, Pixel RGB, Wire RGB
			if (!RouteFsToTsiMux()) failures++;
			if (!RouteTsiMuxToDLOut()) failures++;
			if (!RouteDLOutToSDIOut()) failures++;
			if (useLinkGrouping)
			{
				mDevice.SetSDIOut12GEnable(NTV2_CHANNEL3, true);
				SetupSDITransmitters(NTV2_CHANNEL3, 1);
			}
			else
				SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 8:		//	High Frame Rate, Square, Pixel YCbCr, Wire YCbCr
			if (!RouteFsToSDIOut()) failures++;
			SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 9:		//	High Frame Rate, Square, Pixel YCbCr, Wire RGB
			//	No valid routing for this case
			break;
		case 10:	//	High Frame Rate, Square, Pixel RGB, Wire YCbCr
			if (!RouteFsToCsc()) failures++;
			if (!RouteCscTo4xSDIOut()) failures++;
			SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 11:	//	High Frame Rate, Square, Pixel RGB, Wire RGB
			//	No valid routing for this case
			break;
		case 12:	//	High Frame Rate, Tsi, Pixel YCbCr, Wire YCbCr
			if (!RouteFsToTsiMux()) failures++;
			if (!RouteTsiMuxTo4xSDIOut()) failures++;
			if (useLinkGrouping)
			{
				mDevice.SetSDIOut12GEnable(NTV2_CHANNEL3, true);
				SetupSDITransmitters(NTV2_CHANNEL3, 1);
			}
			else
				SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 13:	//	High Frame Rate, Tsi, Pixel YCbCr, Wire RGB
			//	No valid routing for this case
			break;
		case 14:	//	High Frame Rate, Tsi, Pixel RGB, Wire YCbCr
			if (!RouteFsToTsiMux()) failures++;
			if (!RouteTsiMuxToCsc()) failures++;
			if (!RouteCscTo4xSDIOut()) failures++;
			if(useLinkGrouping)
			{
				mDevice.SetSDIOut12GEnable(NTV2_CHANNEL3, true);
				SetupSDITransmitters(NTV2_CHANNEL3, 1);
			}
			else
				SetupSDITransmitters(mConfig.fOutputChannel, 4);
			break;
		case 15:	//	High Frame Rate, Tsi, Pixel RGB, Wire RGB
			//	No valid routing for this case
			break;
		default:
			return false;	//	Fail
	}

	if (mDevice.features().CanDo12gRouting())
		mDevice.SetTsiFrameEnable  (true,  mConfig.fOutputChannel);
	else if (mConfig.fDoTsiRouting)
		mDevice.SetTsiFrameEnable  (true,  mConfig.fOutputChannel);
	else
		mDevice.Set4kSquaresEnable (true,  mConfig.fOutputChannel);

	//	Send signal to secondary outputs, if supported
	if (!Route4KDownConverter())  failures++;
	if (!RouteHDMIOutput())  failures++;
	return failures == 0;

}	//	RouteOutputSignal


void NTV2Player4K::SetupSDITransmitters (const NTV2Channel inFirstSDI, const UWord inNumSDIs)
{
	if (mDevice.features().HasBiDirectionalSDI())
		mDevice.SetSDITransmitEnable(::NTV2MakeChannelSet (inFirstSDI, mDevice.features().CanDo12gRouting() ? 1 : inNumSDIs),
									/*transmit=*/true);
}


bool NTV2Player4K::Route4KDownConverter (void)
{
	if (!mDevice.features().CanDoWidget(NTV2_Wgt4KDownConverter)  ||  !mDevice.features().CanDoWidget(NTV2_WgtSDIMonOut1))
		return true;

	const bool	canVerify		(mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures	(0);
	if (::IsRGBFormat(mConfig.fPixelFormat))
	{
		mDevice.Enable4KDCRGBMode(true);

		if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			if (!mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer1RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer2RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer3RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer4RGB, canVerify)) connectFailures++;

			if (!mDevice.Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV, canVerify)) connectFailures++;
		}
		else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
		{
			if (!mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer5RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer6RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer7RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer8RGB, canVerify)) connectFailures++;

			if (!mDevice.Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV, canVerify)) connectFailures++;
		}
	}
	else	//	YUV FBF
	{
		mDevice.Enable4KDCRGBMode (false);

		if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			if (!mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer1YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer2YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer3YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer4YUV, canVerify)) connectFailures++;

			if (!mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut, canVerify)) connectFailures++;
		}
		else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
		{
			if (!mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer5YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer6YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer7YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer8YUV, canVerify)) connectFailures++;

			if (!mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut, canVerify)) connectFailures++;
		}
	}
	return connectFailures == 0;

}	//	Route4KDownConverter


bool NTV2Player4K::RouteHDMIOutput (void)
{
	const bool	isRGB (::IsRGBFormat(mConfig.fPixelFormat));
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);

	if (mConfig.fDoHDMIOutput &&
		(mDevice.features().CanDoWidget(NTV2_WgtHDMIOut1v2)
			|| mDevice.features().CanDoWidget(NTV2_WgtHDMIOut1v3)
			|| mDevice.features().CanDoWidget(NTV2_WgtHDMIOut1v4)
			|| mDevice.features().CanDoWidget(NTV2_WgtHDMIOut1v5)) )
	{
		if (mDevice.features().CanDo12gRouting())
		{
			if (!mDevice.Connect (NTV2_XptHDMIOutInput, ::GetFrameBufferOutputXptFromChannel (mConfig.fOutputChannel,  isRGB,  false/*is425*/), canVerify))
				connectFailures++;
		}
		else if(mConfig.fDoTsiRouting)
		{
			if (isRGB)
			{
				if (mConfig.fOutputChannel == NTV2_CHANNEL1)
				{
					if (!mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptCSC1VidYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptCSC2VidYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptCSC3VidYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptCSC4VidYUV, canVerify)) connectFailures++;
				}
				else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
				{
					if (!mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptCSC5VidYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptCSC6VidYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptCSC7VidYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptCSC8VidYUV, canVerify)) connectFailures++;
				}
			}
			else
			{
				if (mConfig.fOutputChannel == NTV2_CHANNEL1)
				{
					if (!mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_Xpt425Mux1AYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_Xpt425Mux1BYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_Xpt425Mux2AYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_Xpt425Mux2BYUV, canVerify)) connectFailures++;
				}
				else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
				{
					if (!mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_Xpt425Mux1AYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_Xpt425Mux1BYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_Xpt425Mux2AYUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_Xpt425Mux2BYUV, canVerify)) connectFailures++;
				}
			}
		}
		else
		{
			if (isRGB)
			{
				if (mConfig.fOutputChannel == NTV2_CHANNEL1)
				{
					if (!mDevice.Connect (NTV2_XptCSC1VidInput,		NTV2_XptFrameBuffer1RGB, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptCSC1VidYUV,		canVerify)) connectFailures++;

					if (!mDevice.Connect (NTV2_XptCSC2VidInput,		NTV2_XptFrameBuffer2RGB, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptCSC2VidYUV,		canVerify)) connectFailures++;

					if (!mDevice.Connect (NTV2_XptCSC3VidInput,		NTV2_XptFrameBuffer3RGB, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptCSC3VidYUV,		canVerify)) connectFailures++;

					if (!mDevice.Connect (NTV2_XptCSC4VidInput,		NTV2_XptFrameBuffer4RGB, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptCSC4VidYUV,		canVerify)) connectFailures++;
				}
				else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
				{
					if (!mDevice.Connect (NTV2_XptCSC5VidInput,		NTV2_XptFrameBuffer5RGB, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptCSC5VidYUV,		canVerify)) connectFailures++;

					if (!mDevice.Connect (NTV2_XptCSC6VidInput,		NTV2_XptFrameBuffer6RGB, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptCSC6VidYUV,		canVerify)) connectFailures++;

					if (!mDevice.Connect (NTV2_XptCSC7VidInput,		NTV2_XptFrameBuffer7RGB, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptCSC7VidYUV,		canVerify)) connectFailures++;

					if (!mDevice.Connect (NTV2_XptCSC8VidInput,		NTV2_XptFrameBuffer8RGB, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptCSC8VidYUV,		canVerify)) connectFailures++;
				}
			}
			else
			{
				if (mConfig.fOutputChannel == NTV2_CHANNEL1)
				{
					if (!mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptFrameBuffer1YUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_XptFrameBuffer2YUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptFrameBuffer3YUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptFrameBuffer4YUV, canVerify)) connectFailures++;
				}
				else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
				{
					if (!mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptFrameBuffer5YUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_XptFrameBuffer6YUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptFrameBuffer7YUV, canVerify)) connectFailures++;
					if (!mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptFrameBuffer8YUV, canVerify)) connectFailures++;
				}
			}
		}

		mDevice.SetHDMIV2TxBypass (false);
		mDevice.SetHDMIOutVideoStandard (::GetNTV2StandardFromVideoFormat(mConfig.fVideoFormat));
		mDevice.SetHDMIOutVideoFPS (::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
		mDevice.SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
		mDevice.SetHDMIV2Mode (NTV2_HDMI_V2_4K_PLAYBACK);
        mDevice.SetHDMIOutAudioRate(NTV2_AUDIO_48K);
		mDevice.SetHDMIOutAudioFormat(NTV2_AUDIO_FORMAT_LPCM);
		mDevice.SetHDMIOutAudioSource8Channel(NTV2_AudioChannel1_8, mAudioSystem);
    }
	else
		mDevice.SetHDMIV2Mode (NTV2_HDMI_V2_4K_PLAYBACK);
	return connectFailures == 0;

}	//	RouteHDMIOutput


bool NTV2Player4K::RouteFsToDLOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (!mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB, canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptFrameBuffer2RGB, canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer3RGB, canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptFrameBuffer4RGB, canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (!mDevice.Connect (NTV2_XptDualLinkOut5Input,	NTV2_XptFrameBuffer5RGB, canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut6Input,	NTV2_XptFrameBuffer6RGB, canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut7Input,	NTV2_XptFrameBuffer7RGB, canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut8Input,	NTV2_XptFrameBuffer8RGB, canVerify)) connectFailures++;
	}
	return connectFailures == 0;
}	//	RouteFsToDLOut


bool NTV2Player4K::RouteFsToCsc (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (!mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_XptFrameBuffer1RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_XptFrameBuffer2RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_XptFrameBuffer3RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_XptFrameBuffer4RGB, canVerify)) connectFailures++;
		}
		else
		{
			if (!mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_XptFrameBuffer1YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_XptFrameBuffer2YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_XptFrameBuffer3YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_XptFrameBuffer4YUV, canVerify)) connectFailures++;
		}
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (!mDevice.Connect (NTV2_XptCSC5VidInput,	NTV2_XptFrameBuffer5RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC6VidInput,	NTV2_XptFrameBuffer6RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC7VidInput,	NTV2_XptFrameBuffer7RGB, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC8VidInput,	NTV2_XptFrameBuffer8RGB, canVerify)) connectFailures++;
		}
		else
		{
			if (!mDevice.Connect (NTV2_XptCSC5VidInput,	NTV2_XptFrameBuffer5YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC6VidInput,	NTV2_XptFrameBuffer6YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC7VidInput,	NTV2_XptFrameBuffer7YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC8VidInput,	NTV2_XptFrameBuffer8YUV, canVerify)) connectFailures++;
		}
	}
	return connectFailures == 0;
}	//	RouteFsToCsc


bool NTV2Player4K::RouteFsToSDIOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mDevice.features().CanDo12gRouting())
	{
		if (!mDevice.Connect (	::GetSDIOutputInputXpt (mConfig.fOutputChannel, false/*isDS2*/),
								::GetFrameBufferOutputXptFromChannel (mConfig.fOutputChannel,  false/*isRGB*/,  false/*is425*/),
								canVerify))
			connectFailures++;
	}
	else
	{
		if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			if (!mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptFrameBuffer1YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptFrameBuffer2YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptFrameBuffer3YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptFrameBuffer4YUV, canVerify)) connectFailures++;
		}
		else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
		{
			if (!mDevice.Connect (NTV2_XptSDIOut5Input,	NTV2_XptFrameBuffer5YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptSDIOut6Input,	NTV2_XptFrameBuffer6YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptSDIOut7Input,	NTV2_XptFrameBuffer7YUV, canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptSDIOut8Input,	NTV2_XptFrameBuffer8YUV, canVerify)) connectFailures++;
		}
	}
	return connectFailures == 0;
}	//	RouteFsToSDIOut


bool NTV2Player4K::RouteFsToTsiMux (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (!mDevice.Connect (NTV2_Xpt425Mux1AInput,	NTV2_XptFrameBuffer1RGB,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux1BInput,	NTV2_XptFrameBuffer1_DS2RGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux2AInput,	NTV2_XptFrameBuffer2RGB,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux2BInput,	NTV2_XptFrameBuffer2_DS2RGB,	canVerify)) connectFailures++;
		}
		else
		{
			if (!mDevice.Connect (NTV2_Xpt425Mux1AInput,	NTV2_XptFrameBuffer1YUV,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux1BInput,	NTV2_XptFrameBuffer1_DS2YUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux2AInput,	NTV2_XptFrameBuffer2YUV,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux2BInput,	NTV2_XptFrameBuffer2_DS2YUV,	canVerify)) connectFailures++;
		}
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL3)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (!mDevice.Connect (NTV2_Xpt425Mux3AInput,	NTV2_XptFrameBuffer3RGB,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux3BInput,	NTV2_XptFrameBuffer3_DS2RGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux4AInput,	NTV2_XptFrameBuffer4RGB,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux4BInput,	NTV2_XptFrameBuffer4_DS2RGB,	canVerify)) connectFailures++;
		}
		else
		{
			if (!mDevice.Connect (NTV2_Xpt425Mux3AInput,	NTV2_XptFrameBuffer3YUV,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux3BInput,	NTV2_XptFrameBuffer3_DS2YUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux4AInput,	NTV2_XptFrameBuffer4YUV,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux4BInput,	NTV2_XptFrameBuffer4_DS2YUV,	canVerify)) connectFailures++;
		}
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (!mDevice.Connect (NTV2_Xpt425Mux3AInput,	NTV2_XptFrameBuffer5RGB,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux3BInput,	NTV2_XptFrameBuffer5_DS2RGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux4AInput,	NTV2_XptFrameBuffer6RGB,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux4BInput,	NTV2_XptFrameBuffer6_DS2RGB,	canVerify)) connectFailures++;
		}
		else
		{
			if (!mDevice.Connect (NTV2_Xpt425Mux3AInput,	NTV2_XptFrameBuffer5YUV,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux3BInput,	NTV2_XptFrameBuffer5_DS2YUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux4AInput,	NTV2_XptFrameBuffer6YUV,		canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_Xpt425Mux4BInput,	NTV2_XptFrameBuffer6_DS2YUV,	canVerify)) connectFailures++;
		}
	}
	return connectFailures == 0;
}	//	RouteFsToTsiMux


bool NTV2Player4K::RouteDLOutToSDIOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (!mDevice.Connect (NTV2_XptSDIOut1Input,		NTV2_XptDuallinkOut1,		canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_XptDuallinkOut1DS2,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut2Input,		NTV2_XptDuallinkOut2,		canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_XptDuallinkOut2DS2,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut3Input,		NTV2_XptDuallinkOut3,		canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut3InputDS2,	NTV2_XptDuallinkOut3DS2,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut4Input,		NTV2_XptDuallinkOut4,		canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut4InputDS2,	NTV2_XptDuallinkOut4DS2,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (!mDevice.Connect (NTV2_XptSDIOut5Input,		NTV2_XptDuallinkOut5,		canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut5InputDS2,	NTV2_XptDuallinkOut5DS2,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut6Input,		NTV2_XptDuallinkOut6,		canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut6InputDS2,	NTV2_XptDuallinkOut6DS2,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut7Input,		NTV2_XptDuallinkOut7,		canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut7InputDS2,	NTV2_XptDuallinkOut7DS2,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut8Input,		NTV2_XptDuallinkOut8,		canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut8InputDS2,	NTV2_XptDuallinkOut8DS2,	canVerify)) connectFailures++;
	}
	return connectFailures == 0;
}	//	RouteDLOutToSDIOut


bool NTV2Player4K::RouteCscTo2xSDIOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
        if (!mDevice.Connect (NTV2_XptSDIOut1Input,		NTV2_XptCSC1VidYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_XptCSC2VidYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut2Input,		NTV2_XptCSC3VidYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_XptCSC4VidYUV,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
        if (!mDevice.Connect (NTV2_XptSDIOut5Input,		NTV2_XptCSC5VidYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut5InputDS2,	NTV2_XptCSC6VidYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut6Input,		NTV2_XptCSC7VidYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut6InputDS2,	NTV2_XptCSC8VidYUV,	canVerify)) connectFailures++;
	}
	return connectFailures == 0;
}	//	RouteCscTo2xSDIOut


bool NTV2Player4K::RouteCscTo4xSDIOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
    if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (!mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptCSC1VidYUV,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptCSC2VidYUV,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptCSC3VidYUV,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptCSC4VidYUV,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (!mDevice.Connect (NTV2_XptSDIOut5Input,	NTV2_XptCSC5VidYUV,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut6Input,	NTV2_XptCSC6VidYUV,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut7Input,	NTV2_XptCSC7VidYUV,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptSDIOut8Input,	NTV2_XptCSC8VidYUV,	canVerify)) connectFailures++;
	}
	return connectFailures == 0;
}	//	RouteCscTo4xSDIOut


bool NTV2Player4K::RouteCscToDLOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (!mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptCSC1VidRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptCSC2VidRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptCSC3VidRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptCSC4VidRGB,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (!mDevice.Connect (NTV2_XptDualLinkOut5Input,	NTV2_XptCSC5VidRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut6Input,	NTV2_XptCSC6VidRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut7Input,	NTV2_XptCSC7VidRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut8Input,	NTV2_XptCSC8VidRGB,	canVerify)) connectFailures++;
	}
	return connectFailures == 0;
}	//	RouteCscToDLOut


bool NTV2Player4K::RouteTsiMuxToDLOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (!mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_Xpt425Mux1ARGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_Xpt425Mux1BRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_Xpt425Mux2ARGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_Xpt425Mux2BRGB,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL3)
	{
		if (!mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_Xpt425Mux3ARGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_Xpt425Mux3BRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_Xpt425Mux4ARGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_Xpt425Mux4BRGB,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (!mDevice.Connect (NTV2_XptDualLinkOut5Input,	NTV2_Xpt425Mux3ARGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut6Input,	NTV2_Xpt425Mux3BRGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut7Input,	NTV2_Xpt425Mux4ARGB,	canVerify)) connectFailures++;
		if (!mDevice.Connect (NTV2_XptDualLinkOut8Input,	NTV2_Xpt425Mux4BRGB,	canVerify)) connectFailures++;
	}
	return connectFailures == 0;
}	//	RouteTsiMuxToDLOut


bool NTV2Player4K::RouteTsiMuxToCsc (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (!mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_Xpt425Mux1ARGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_Xpt425Mux1BRGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_Xpt425Mux2ARGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_Xpt425Mux2BRGB,	canVerify)) connectFailures++;
		}
		else
		{
			if (!mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_Xpt425Mux1AYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_Xpt425Mux1BYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_Xpt425Mux2AYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_Xpt425Mux2BYUV,	canVerify)) connectFailures++;
		}
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL3)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (!mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_Xpt425Mux3ARGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_Xpt425Mux3BRGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_Xpt425Mux4ARGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_Xpt425Mux4BRGB,	canVerify)) connectFailures++;
		}
		else
		{
			if (!mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_Xpt425Mux3AYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_Xpt425Mux3BYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_Xpt425Mux4AYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_Xpt425Mux4BYUV,	canVerify)) connectFailures++;
		}
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (!mDevice.Connect (NTV2_XptCSC5VidInput,	NTV2_Xpt425Mux3ARGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC6VidInput,	NTV2_Xpt425Mux3BRGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC7VidInput,	NTV2_Xpt425Mux4ARGB,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC8VidInput,	NTV2_Xpt425Mux4BRGB,	canVerify)) connectFailures++;
		}
		else
		{
			if (!mDevice.Connect (NTV2_XptCSC5VidInput,	NTV2_Xpt425Mux3AYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC6VidInput,	NTV2_Xpt425Mux3BYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC7VidInput,	NTV2_Xpt425Mux4AYUV,	canVerify)) connectFailures++;
			if (!mDevice.Connect (NTV2_XptCSC8VidInput,	NTV2_Xpt425Mux4BYUV,	canVerify)) connectFailures++;
		}
	}
	return connectFailures == 0;
}	//	RouteTsiMuxToCsc


bool NTV2Player4K::RouteTsiMuxTo2xSDIOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
        if (!mDevice.Connect (NTV2_XptSDIOut1Input,		NTV2_Xpt425Mux1AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_Xpt425Mux1BYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut2Input,		NTV2_Xpt425Mux2AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_Xpt425Mux2BYUV,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL3)
	{
        if (!mDevice.Connect (NTV2_XptSDIOut3Input,		NTV2_Xpt425Mux3AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut3InputDS2,	NTV2_Xpt425Mux3BYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut4Input,		NTV2_Xpt425Mux4AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut4InputDS2,	NTV2_Xpt425Mux4BYUV,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
        if (!mDevice.Connect (NTV2_XptSDIOut5Input,		NTV2_Xpt425Mux3AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut5InputDS2,	NTV2_Xpt425Mux3BYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut6Input,		NTV2_Xpt425Mux4AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut6InputDS2,	NTV2_Xpt425Mux4BYUV,	canVerify)) connectFailures++;
	}
	return connectFailures == 0;
}	//	RouteTsiMuxTo2xSDIOut


bool NTV2Player4K::RouteTsiMuxTo4xSDIOut (void)
{
	const bool	canVerify (mDevice.features().HasCrosspointConnectROM());
	UWord		connectFailures (0);
    if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
        if (!mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_Xpt425Mux1AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_Xpt425Mux1BYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_Xpt425Mux2AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_Xpt425Mux2BYUV,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL3)
	{	//	Io4k+ 12G output
        if (!mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_Xpt425Mux3AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_Xpt425Mux3BYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_Xpt425Mux4AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_Xpt425Mux4BYUV,	canVerify)) connectFailures++;
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
        if (!mDevice.Connect (NTV2_XptSDIOut5Input,	NTV2_Xpt425Mux3AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut6Input,	NTV2_Xpt425Mux3BYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut7Input,	NTV2_Xpt425Mux4AYUV,	canVerify)) connectFailures++;
        if (!mDevice.Connect (NTV2_XptSDIOut8Input,	NTV2_Xpt425Mux4BYUV,	canVerify)) connectFailures++;
	}
	return connectFailures == 0;
}	//	RouteTsiMuxTo4xSDIOut


AJAStatus NTV2Player4K::Run (void)
{
	//	Start my consumer and producer threads...
	StartConsumerThread();
	StartProducerThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2Player4K::StartConsumerThread (void)
{
	//	Create and start the playout thread...
	mConsumerThread.Attach (ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The playout thread function
void NTV2Player4K::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Player4K instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2Player4K * pApp (reinterpret_cast<NTV2Player4K*>(pContext));
	if (pApp)
		pApp->ConsumeFrames();

}	//	ConsumerThreadStatic


void NTV2Player4K::ConsumeFrames (void)
{
	ULWord					acOptions (AUTOCIRCULATE_WITH_RP188);
	AUTOCIRCULATE_TRANSFER	outputXfer;
	AUTOCIRCULATE_STATUS	outputStatus;
	AJAAncillaryData *		pPkt (AJA_NULL);
	ULWord					goodXfers(0), badXfers(0), prodWaits(0), noRoomWaits(0);
	const UWord				numACFramesPerChannel(7);

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
#ifdef NTV2_BUFFER_LOCKING
	if (outputXfer.acANCBuffer)
		mDevice.DMABufferLock(outputXfer.acANCBuffer, /*alsoLockSGL*/true);
#endif

	//	Calculate start & end frame numbers (if mConfig.fFrames not used)...
	UWord startNum(0), endNum(0);
    if (mDevice.features().CanDo12gRouting())
		startNum = numACFramesPerChannel * mConfig.fOutputChannel;
    else switch (mConfig.fOutputChannel)
	{	default:
		case NTV2_CHANNEL1:		if (mConfig.fNumAudioLinks > 1)
								{
									acOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO1;
									if (NTV2_IS_4K_HFR_VIDEO_FORMAT(mConfig.fVideoFormat))
									{
										acOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO2;
										acOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO3;
									}
								}
								AJA_FALL_THRU;
		case NTV2_CHANNEL2:		startNum = numACFramesPerChannel * 0;		break;

		case NTV2_CHANNEL3:		if (mConfig.fNumAudioLinks > 1)
									acOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO1;
								AJA_FALL_THRU;
		case NTV2_CHANNEL4:		startNum = numACFramesPerChannel * 1;		break;

		case NTV2_CHANNEL5:		AJA_FALL_THRU;
		case NTV2_CHANNEL6:		startNum = numACFramesPerChannel * 2;		break;

		case NTV2_CHANNEL7:		AJA_FALL_THRU;
		case NTV2_CHANNEL8:		startNum = numACFramesPerChannel * 3;		break;
	}
	endNum = startNum + numACFramesPerChannel - 1;

	//	Initialize & start AutoCirculate...
	bool initOK(false);
	if (mConfig.fFrames.valid())	//	--frames option controls everything:
		initOK = mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,  mConfig.fFrames.count(),  mAudioSystem,  acOptions,
														1 /*numChannels*/,  mConfig.fFrames.firstFrame(),  mConfig.fFrames.lastFrame());
	else	//	--frames option not used -- explicitly specify start & end frame numbers (as calculated above)
		initOK = mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,  0,  mAudioSystem,  acOptions,
														1 /*numChannels*/,  startNum,  endNum);
	if (!initOK)
		{PLFAIL("AutoCirculateInitForOutput failed");  mGlobalQuit = true;}

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
				{prodWaits++;  continue;}

			//	Unlike in the NTV2Player demo, I now burn the current timecode into the test pattern buffer that was noted
			//	earlier into this FrameData in my Producer thread.  This is done to avoid copying large 4K/UHD rasters.
			const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
			const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
			const	CRP188			rp188Info		(mCurrentFrame++, 0, 0, 10, tcFormat);
			NTV2_RP188				tcData;
			string					timeCodeString;

			rp188Info.GetRP188Reg (tcData);
			rp188Info.GetRP188Str (timeCodeString);
			mTCBurner.BurnTimeCode (pFrameData->fVideoBuffer, timeCodeString.c_str(), 80);

			//	Transfer the timecode-burned frame (plus audio) to the device for playout...
			outputXfer.acVideoBuffer.Set (pFrameData->fVideoBuffer, pFrameData->fVideoBuffer);
			outputXfer.acAudioBuffer.Set (pFrameData->fAudioBuffer, pFrameData->fNumAudioBytes);
			outputXfer.SetOutputTimeCode (tcData, ::NTV2ChannelToTimecodeIndex(mConfig.fOutputChannel, /*LTC=*/false, /*F2=*/false));
			outputXfer.SetOutputTimeCode (tcData, ::NTV2ChannelToTimecodeIndex(mConfig.fOutputChannel, /*LTC=*/true,  /*F2=*/false));

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
			<< DEC(prodWaits) << " starves, " << DEC(noRoomWaits) << " VBI waits");

}	//	ConsumeFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2Player4K::StartProducerThread (void)
{
	//	Create and start the producer thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


void NTV2Player4K::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	NTV2Player4K *	pApp (reinterpret_cast<NTV2Player4K*>(pContext));
	if (pApp)
		pApp->ProduceFrames();

}	//	ProducerThreadStatic


void NTV2Player4K::ProduceFrames (void)
{
	ULWord	freqNdx(0), testPatNdx(0), badTally(0);
	double	timeOfLastSwitch	(0.0);

	const AJATimeBase			timeBase	(CNTV2DemoCommon::GetAJAFrameRate(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat)));
	const NTV2TestPatternNames	tpNames		(NTV2TestPatternGen::getTestPatternNames());

	PLNOTE("Thread started");
	while (!mGlobalQuit)
	{
		NTV2FrameData *	pFrameData (mFrameDataRing.StartProduceNextBuffer());
		if (!pFrameData)
		{	badTally++;			//	No frame available!
			AJATime::Sleep(10);	//	Wait a bit for the consumer thread to free one up for me...
			continue;			//	...then try again
		}

		//	Unlike NTV2Player::ProduceFrames, NTV2Player4K::ProduceFrames doesn't touch this frame's video buffer.
		//	Instead, to avoid wasting time copying large 4K/UHD rasters, in this thread we simply note which test
		//	pattern buffer is to be modified and subsequently transferred to the hardware. This happens later, in
		//	NTV2Player4K::ConsumeFrames...
		NTV2Buffer & testPatVidBuffer(mTestPatRasters.at(testPatNdx));
		pFrameData->fVideoBuffer.Set(testPatVidBuffer.GetHostPointer(), testPatVidBuffer.GetByteCount());

		//	If also playing audio...
		if (pFrameData->AudioBuffer())	//	...then generate audio tone data for this frame...
			pFrameData->fNumAudioBytes = AddTone(pFrameData->AudioBuffer());	//	...and remember number of audio bytes to xfer

		//	Every few seconds, change the test pattern and tone frequency...
		const double currentTime (timeBase.FramesToSeconds(mCurrentFrame++));
		if (currentTime > timeOfLastSwitch + 4.0)
		{
			freqNdx = (freqNdx + 1) % gNumFrequencies;
			testPatNdx = (testPatNdx + 1) % ULWord(mTestPatRasters.size());
			mToneFrequency = gFrequencies[freqNdx];
			timeOfLastSwitch = currentTime;
			PLINFO("F" << DEC0N(mCurrentFrame,6) << ": tone=" << mToneFrequency << "Hz, pattern='" << tpNames.at(testPatNdx) << "'");
		}	//	if time to switch test pattern & tone frequency

		//	Signal that I'm done producing this FrameData, making it immediately available for transfer/playout...
		mFrameDataRing.EndProduceNextBuffer();

	}	//	loop til mGlobalQuit goes true
	PLNOTE("Thread completed: " << DEC(mCurrentFrame) << " frame(s) produced, " << DEC(badTally) << " failed");

}	//	ProduceFrames


uint32_t NTV2Player4K::AddTone (ULWord * audioBuffer)
{
	NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
	NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
	ULWord			numChannels	(0);

	mDevice.GetFrameRate (frameRate, mConfig.fOutputChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);

	//	Since audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will
	//	necessarily result in some frames having more audio samples than others. GetAudioSamplesPerFrame is
	//	called to calculate the correct sample count for the current frame...
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));
	const double	sampleRateHertz	(::GetAudioSamplesPerSecond(audioRate));

	//	Unlike NTV2Player::AddTone, NTV2Player4K::AddTone handles multi-link audio:
	ULWord bytesWritten(0), startSample(mCurrentSample);
	for (UWord linkNdx(0);  linkNdx < mConfig.fNumAudioLinks;  linkNdx++)
	{
		mCurrentSample = startSample;
		bytesWritten += ::AddAudioTone (audioBuffer + (bytesWritten/4),	//	audio buffer to fill
									   mCurrentSample,					//	which sample for continuing the waveform
									   numSamples,						//	number of samples to generate
									   sampleRateHertz,					//	sample rate [Hz]
									   0.50,							//	uniform amplitude
									   mToneFrequency,					//	uniform tone frequency [Hz]
									   31,								//	bits per sample
									   false,							//	don't byte swap
									   numChannels);					//	number of audio channels to generate
	}	//	for each SDI audio link
	return bytesWritten;

}	//	AddTone


void NTV2Player4K::GetACStatus (AUTOCIRCULATE_STATUS & outStatus)
{
	mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outStatus);
}
