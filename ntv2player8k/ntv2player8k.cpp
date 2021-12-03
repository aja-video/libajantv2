/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2player8k.cpp
	@brief		Implementation of ntv2player8k class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2player8k.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2debug.h"
#include "ntv2testpatterngen.h"
#include "ajabase/common/timecode.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/info.h"
#include "ajabase/system/process.h"
#include "ajaanc/includes/ancillarydata_hdr_sdr.h"
#include "ajaanc/includes/ancillarydata_hdr_hdr10.h"
#include "ajaanc/includes/ancillarydata_hdr_hlg.h"

using namespace std;

#define NTV2_BUFFER_LOCKING		//	IMPORTANT FOR 8K: Define this to pre-lock video/audio buffers in kernel

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

/**
	@brief	The alignment of the video and audio buffers has a big impact on the efficiency of
			DMA transfers. When aligned to the page size of the architecture, only one DMA
			descriptor is needed per page. Misalignment will double the number of descriptors
			that need to be fetched and processed, thus reducing bandwidth.
**/
static const uint32_t	BUFFER_ALIGNMENT	(4096);		//	The optimal size for most systems
static const bool		BUFFER_PAGE_ALIGNED	(true);

//	Audio tone generator data
static const double		gFrequencies []	=	{250.0, 500.0, 1000.0, 2000.0};
static const ULWord		gNumFrequencies		(sizeof(gFrequencies) / sizeof(double));
//	Unlike NTV2Player, this demo uses the same waveform amplitude in each audio channel


NTV2Player8K::NTV2Player8K (const Player8KConfig & inConfig)
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


NTV2Player8K::~NTV2Player8K (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit();

	mDevice.UnsubscribeOutputVerticalEvent(mConfig.fOutputChannel);	//	Unsubscribe from output VBI event
}	//	destructor


void NTV2Player8K::Quit (void)
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


AJAStatus NTV2Player8K::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}
	mDeviceID = mDevice.GetDeviceID();	//	Keep this ID handy -- it's used frequently

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	const UWord maxNumChannels (::NTV2DeviceGetNumFrameStores(mDeviceID));

	//	Check for an invalid configuration
	if (NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  mConfig.fDoRGBOnWire)
		{cerr << "## ERROR:  HFR RGB output not supported" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Check for valid channel...
	if (UWord(mConfig.fOutputChannel) >= maxNumChannels)
	{
		cerr	<< "## ERROR:  Cannot use channel '" << DEC(mConfig.fOutputChannel+1) << "' -- device only supports channel 1"
				<< (maxNumChannels > 1  ?  string(" thru ") + string(1, char(maxNumChannels+'0'))  :  "") << endl;
		return AJA_STATUS_UNSUPPORTED;
	}
	if (mConfig.fOutputChannel != NTV2_CHANNEL1 && mConfig.fOutputChannel != NTV2_CHANNEL3)
	{
		cerr	<< "## ERROR:  8K/UHD2 requires Ch1 or Ch3, not Ch" << DEC(mConfig.fOutputChannel) << endl;
		return AJA_STATUS_BAD_PARAM;
	}

	if (!mConfig.fDoMultiFormat)
	{
		mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current task mode
		if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, int32_t(AJAProcess::GetPid())))
			return AJA_STATUS_BUSY;		//	Device is in use by another app -- fail
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Set OEM service level

	if (::NTV2DeviceCanDoMultiFormat(mDeviceID))
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
	RouteOutputSignal();

	//	Lastly, prepare my AJATimeCodeBurn instance...
	if (!mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat(mConfig.fPixelFormat), mFormatDesc.numPixels, mFormatDesc.numLines))
		{cerr << "## ERROR:  RenderTimeCodeFont failed for:  " << mFormatDesc << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Ready to go...
	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#else
		PLINFO("Configuration: " << mConfig);
	#endif	//	not _DEBUG
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Player8K::SetUpVideo (void)
{
	//	Configure the device to output the requested video format...
 	if (mConfig.fVideoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_BAD_PARAM;

if(false)///////////////////////////////////////////////////////////////////////	if (!::NTV2DeviceCanDoVideoFormat (mDeviceID, mConfig.fVideoFormat))
		{cerr << "## ERROR:  Device can't do " << ::NTV2VideoFormatToString(mConfig.fVideoFormat) << endl;  return AJA_STATUS_UNSUPPORTED;}

	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.fPixelFormat))
		{cerr << "## ERROR: Pixel format '" << ::NTV2FrameBufferFormatString(mConfig.fPixelFormat) << "' not supported on this device" << endl;
			return AJA_STATUS_UNSUPPORTED;}

	NTV2ChannelSet channels13, frameStores;
	channels13.insert(NTV2_CHANNEL1);  channels13.insert(NTV2_CHANNEL3);
	if (mConfig.fDoTsiRouting)
	{	//	"Tsi" routing requires 2 FrameStores
		if (channels13.find(mConfig.fOutputChannel) == channels13.end())
			return AJA_STATUS_BAD_PARAM;	//	fOutputChannel not Ch1 or Ch3
		frameStores = ::NTV2MakeChannelSet (mConfig.fOutputChannel, 2);	//	2 FrameStores starting at fOutputChannel
	}
	else
	{	//	"Squares" routing requires 4 FrameStores
		if (mConfig.fOutputChannel != NTV2_CHANNEL1)
			return AJA_STATUS_BAD_PARAM;	//	fOutputChannel not Ch1
		frameStores = ::NTV2MakeChannelSet (mConfig.fOutputChannel, 4);	//	4 FrameStores starting at fOutputChannel
	}

	//	Keep the raster description handy...
	mFormatDesc = NTV2FormatDescriptor(mConfig.fVideoFormat, mConfig.fPixelFormat);
	if (!mFormatDesc.IsValid())
		return AJA_STATUS_FAIL;

	//	Turn on the FrameStores (to read frame buffer memory and transmit video)...
	mDevice.EnableChannels (frameStores, /*disableOthers=*/!mConfig.fDoMultiFormat);

	//	This demo requires VANC be disabled...
	mDevice.SetVANCMode (frameStores, NTV2_VANCMODE_OFF);	//	VANC is incompatible with 8K/UHD2 formats

	//	Set the FrameStore video format...
	mDevice.SetVideoFormat (mConfig.fVideoFormat, false, false, mConfig.fOutputChannel);
    mDevice.SetQuadQuadFrameEnable (true, mConfig.fOutputChannel);
    mDevice.SetQuadQuadSquaresEnable (!mConfig.fDoTsiRouting, mConfig.fOutputChannel);

	//	Set the frame buffer pixel format for the device FrameStore(s)...
	mDevice.SetFrameBufferFormat (frameStores, mConfig.fPixelFormat);

	//	The output interrupt is Enabled by default, but on some platforms, you must subscribe to it
	//	in order to be able to wait on its event/semaphore...
	mDevice.SubscribeOutputVerticalEvent (mConfig.fOutputChannel);

	//	Check if HDR anc is permissible...
	if (IS_KNOWN_AJAAncillaryDataType(mConfig.fTransmitHDRType)  &&  !::NTV2DeviceCanDoCustomAnc(mDeviceID))
		{cerr << "## WARNING:  HDR Anc requested, but device can't do custom anc" << endl;
			mConfig.fTransmitHDRType = AJAAncillaryDataType_Unknown;}

	//	Get current per-field maximum Anc buffer size...
	if (!mDevice.GetAncRegionOffsetFromBottom (gAncMaxSizeBytes, NTV2_AncRgn_Field2))
		gAncMaxSizeBytes = NTV2_ANCSIZE_MAX;

	//	Set output clock reference...
	mDevice.SetReference(::NTV2DeviceCanDo2110(mDeviceID) ? NTV2_REFERENCE_SFP1_PTP : NTV2_REFERENCE_FREERUN);

	//	At this point, video setup is complete (except for widget signal routing).
	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2Player8K::SetUpAudio (void)
{
	uint16_t numAudioChannels (::NTV2DeviceGetMaxAudioChannels(mDeviceID));

	//	If there are 8192 pixels on a line instead of 7680, reduce the number of audio channels
	//	This is because HANC is narrower, and has space for only 8 channels
	if (NTV2_IS_UHD2_FULL_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  numAudioChannels > 8)
		numAudioChannels = 8;

	//	Use the NTV2AudioSystem that has the same ordinal value as the output FrameStore/Channel...
	mAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fOutputChannel);

	if (mConfig.fNumAudioLinks > 1)	//	For users that want to send 32 or 64 audio channels on 2 or 4 SDI links
		switch (mAudioSystem)
		{
			default:
			case NTV2_AUDIOSYSTEM_1:
			{	const UWord numChan(NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(mConfig.fVideoFormat) ? 4 : 2);
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
		mDevice.SetSDIOutputAudioSystem (::NTV2MakeChannelSet (NTV2_CHANNEL1, 4), mAudioSystem);
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


AJAStatus NTV2Player8K::SetUpHostBuffers (void)
{
	if (NTV2_POINTER::DefaultPageSize() != BUFFER_ALIGNMENT)
	{
		PLNOTE("Buffer alignment changed from " << xHEX0N(NTV2_POINTER::DefaultPageSize(),8) << " to " << xHEX0N(BUFFER_ALIGNMENT,8));
		NTV2_POINTER::SetDefaultPageSize(BUFFER_ALIGNMENT);
	}

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


AJAStatus NTV2Player8K::SetUpTestPatternBuffers (void)
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
		mTestPatRasters.push_back(NTV2_POINTER());

	if (!mFormatDesc.IsValid())
		{PLFAIL("Bad format descriptor");  return AJA_STATUS_FAIL;}
	if (mFormatDesc.IsVANC())
		{PLFAIL("VANC incompatible with UHD2/8K: " << mFormatDesc);  return AJA_STATUS_FAIL;}

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


void NTV2Player8K::RouteOutputSignal (void)
{
	if (!mConfig.fDoMultiFormat)
		mDevice.ClearRouting();	//	Replace current signal routing

	if (mConfig.fDoTsiRouting)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			if (mConfig.fOutputChannel < NTV2_CHANNEL3)
			{
				mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
				mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptFrameBuffer1_DS2RGB);
				mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer2RGB);
				mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptFrameBuffer2_DS2RGB);
				if (mConfig.fDoHDMIOutput)
					mDevice.Connect (NTV2_XptHDMIOutInput,	NTV2_XptFrameBuffer1RGB);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer3RGB);
				mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptFrameBuffer3_DS2RGB);
				mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer4RGB);
				mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptFrameBuffer4_DS2RGB);
				if (mConfig.fDoHDMIOutput)
					mDevice.Connect (NTV2_XptHDMIOutInput,	NTV2_XptFrameBuffer3RGB);
			}
			mDevice.Connect (NTV2_XptSDIOut1Input,		NTV2_XptDuallinkOut1);
			mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_XptDuallinkOut1DS2);
			mDevice.Connect (NTV2_XptSDIOut2Input,		NTV2_XptDuallinkOut2);
			mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_XptDuallinkOut2DS2);
			mDevice.Connect (NTV2_XptSDIOut3Input,		NTV2_XptDuallinkOut3);
			mDevice.Connect (NTV2_XptSDIOut3InputDS2,	NTV2_XptDuallinkOut3DS2);
			mDevice.Connect (NTV2_XptSDIOut4Input,		NTV2_XptDuallinkOut4);
			mDevice.Connect (NTV2_XptSDIOut4InputDS2,	NTV2_XptDuallinkOut4DS2);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL1, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL2, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL2, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL3, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL3, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL4, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL4, true);
		}
		else
		{
			if (mConfig.fOutputChannel < NTV2_CHANNEL3)
			{
				if (NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(mConfig.fVideoFormat))
				{
					mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptFrameBuffer1YUV);
					mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptFrameBuffer1_DS2YUV);
					mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptFrameBuffer2YUV);
					mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptFrameBuffer2_DS2YUV);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL1, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, false);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL2, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL2, false);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL3, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL3, false);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL4, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL4, false);
					if (mConfig.fDoHDMIOutput)
						mDevice.Connect (NTV2_XptHDMIOutInput,	NTV2_XptFrameBuffer1YUV);
				}
				else
				{
					mDevice.Connect (NTV2_XptSDIOut1Input,		NTV2_XptFrameBuffer1YUV);
					mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_XptFrameBuffer1_DS2YUV);
					mDevice.Connect (NTV2_XptSDIOut2Input,		NTV2_XptFrameBuffer2YUV);
					mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_XptFrameBuffer2_DS2YUV);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL1, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, false);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL2, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL2, false);
					if (mConfig.fDoHDMIOutput)
						mDevice.Connect (NTV2_XptHDMIOutInput,	NTV2_XptFrameBuffer1YUV);
				}
			}
			else
			{
				if (NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(mConfig.fVideoFormat))
				{
					mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptFrameBuffer3YUV);
					mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptFrameBuffer3_DS2YUV);
					mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptFrameBuffer4YUV);
					mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptFrameBuffer4_DS2YUV);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL1, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, false);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL2, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL2, false);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL3, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL3, false);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL4, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL4, false);
					if (mConfig.fDoHDMIOutput)
						mDevice.Connect (NTV2_XptHDMIOutInput,	NTV2_XptFrameBuffer3YUV);
				}
				else
				{
					mDevice.Connect (NTV2_XptSDIOut3Input,		NTV2_XptFrameBuffer3YUV);
					mDevice.Connect (NTV2_XptSDIOut3InputDS2,	NTV2_XptFrameBuffer3_DS2YUV);
					mDevice.Connect (NTV2_XptSDIOut4Input,		NTV2_XptFrameBuffer4YUV);
					mDevice.Connect (NTV2_XptSDIOut4InputDS2,	NTV2_XptFrameBuffer4_DS2YUV);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
					mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL3, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL3, false);
					mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL4, false);
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL4, false);
					if (mConfig.fDoHDMIOutput)
						mDevice.Connect (NTV2_XptHDMIOutInput,	NTV2_XptFrameBuffer3YUV);
				}
			}
		}
	}
	else
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
			mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptFrameBuffer2RGB);
			mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer3RGB);
			mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptFrameBuffer4RGB);
			mDevice.Connect (NTV2_XptSDIOut1Input,		NTV2_XptDuallinkOut1);
			mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_XptDuallinkOut1DS2);
			mDevice.Connect (NTV2_XptSDIOut2Input,		NTV2_XptDuallinkOut2);
			mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_XptDuallinkOut2DS2);
			mDevice.Connect (NTV2_XptSDIOut3Input,		NTV2_XptDuallinkOut3);
			mDevice.Connect (NTV2_XptSDIOut3InputDS2,	NTV2_XptDuallinkOut3DS2);
			mDevice.Connect (NTV2_XptSDIOut4Input,		NTV2_XptDuallinkOut4);
			mDevice.Connect (NTV2_XptSDIOut4InputDS2,	NTV2_XptDuallinkOut4DS2);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL1, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL2, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL2, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL3, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL3, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL4, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL4, true);
		}
		else
		{
			mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptFrameBuffer1YUV);
			mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptFrameBuffer2YUV);
			mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptFrameBuffer3YUV);
			mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptFrameBuffer4YUV);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL1, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, false);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL2, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL2, false);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL3, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL3, false);
			mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL4, false);
			mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL4, false);
		}
	}

}	//	RouteOutputSignal


AJAStatus NTV2Player8K::Run (void)
{
	//	Start my consumer and producer threads...
	StartConsumerThread();
	StartProducerThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2Player8K::StartConsumerThread (void)
{
	//	Create and start the playout thread...
	mConsumerThread.Attach (ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The playout thread function
void NTV2Player8K::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Player8K instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2Player8K * pApp (reinterpret_cast<NTV2Player8K*>(pContext));
	if (pApp)
		pApp->ConsumeFrames();

}	//	ConsumerThreadStatic


void NTV2Player8K::ConsumeFrames (void)
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

	if (IS_KNOWN_AJAAncillaryDataType(mConfig.fTransmitHDRType))
	{	//	HDR anc doesn't change per-frame, so fill outputXfer.acANCBuffer with the packet data...
		static AJAAncillaryData_HDR_SDR		sdrPkt;
		static AJAAncillaryData_HDR_HDR10	hdr10Pkt;
		static AJAAncillaryData_HDR_HLG		hlgPkt;

		switch (mConfig.fTransmitHDRType)
		{
			case AJAAncillaryDataType_HDR_SDR:		pPkt = &sdrPkt;		break;
			case AJAAncillaryDataType_HDR_HDR10:	pPkt = &hdr10Pkt;	break;
			case AJAAncillaryDataType_HDR_HLG:		pPkt = &hlgPkt;		break;
			default:								break;
		}
	}
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

	//	Calculate start & end frame numbers...
	const UWord	startNum	(mConfig.fOutputChannel < 2	?						0	:	numACFramesPerChannel);		//	Ch1: frames 0-6
	const UWord	endNum		(mConfig.fOutputChannel < 2	?	numACFramesPerChannel-1	:	numACFramesPerChannel*2-1);	//	Ch5: frames 7-13
	if (mConfig.fNumAudioLinks > 1)
	{
		acOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO1;
		if (NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(mConfig.fVideoFormat))
		{
			acOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO2;
			acOptions |= AUTOCIRCULATE_WITH_MULTILINK_AUDIO3;
		}
	}

	//	Initialize & start AutoCirculate...
	bool initOK (mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,  0,  mAudioSystem,  acOptions,
													1 /*numChannels*/,  startNum,  endNum));
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
			//	earlier into this FrameData in my Producer thread.  This is done to avoid copying large 8K/UHD2 rasters.
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

void NTV2Player8K::StartProducerThread (void)
{
	//	Create and start the producer thread...
	mProducerThread.Attach(ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


void NTV2Player8K::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	NTV2Player8K *	pApp (reinterpret_cast<NTV2Player8K*>(pContext));
	if (pApp)
		pApp->ProduceFrames();

}	//	ProducerThreadStatic


void NTV2Player8K::ProduceFrames (void)
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

		//	Unlike NTV2Player::ProduceFrames, NTV2Player8K::ProduceFrames doesn't touch this frame's video buffer.
		//	Instead, to avoid wasting time copying large 8K/UHD2 rasters, in this thread we simply note which test
		//	pattern buffer is to be modified and subsequently transferred to the hardware. This happens later, in
		//	NTV2Player8K::ConsumeFrames...
		NTV2_POINTER & testPatVidBuffer(mTestPatRasters.at(testPatNdx));
		pFrameData->fVideoBuffer.Set(testPatVidBuffer.GetHostPointer(), testPatVidBuffer.GetByteCount());

		//	If also playing audio...
		if (pFrameData->AudioBuffer())	//	...then generate audio tone data for this frame...
			pFrameData->fNumAudioBytes = AddTone(pFrameData->fAudioBuffer);	//	...and remember number of audio bytes to xfer

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


uint32_t NTV2Player8K::AddTone (ULWord * audioBuffer)
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
	ULWord bytesWritten(0), startSample(mCurrentSample);
	for (UWord linkNdx(0);  linkNdx < mConfig.fNumAudioLinks;  linkNdx++)
	{
		mCurrentSample = startSample;
		bytesWritten += ::AddAudioTone (audioBuffer + (bytesWritten/4),	//	audio buffer to fill
									   mCurrentSample,					//	which sample for continuing the waveform
									   numSamples,						//	number of samples to generate
									   sampleRateHertz,					//	sample rate [Hz]
									   0.1,								//	amplitude
									   mToneFrequency,					//	tone frequency [Hz]
									   31,								//	bits per sample
									   false,							//	don't byte swap
									   numChannels);					//	number of audio channels to generate
	}	//	for each SDI audio link
	return bytesWritten;

}	//	AddTone


void NTV2Player8K::GetACStatus (AUTOCIRCULATE_STATUS & outStatus)
{
	mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outStatus);
}


ostream & Player8KConfig::Print (ostream & strm, const bool inCompact) const
{
	AJALabelValuePairs result;
	AJASystemInfo::append (result, "NTV2Player8K Config");
	AJASystemInfo::append (result, "Device Specifier",	fDeviceSpecifier);
	AJASystemInfo::append (result, "Video Format",		::NTV2VideoFormatToString(fVideoFormat));
	AJASystemInfo::append (result, "Pixel Format",		::NTV2FrameBufferFormatToString(fPixelFormat, inCompact));
	AJASystemInfo::append (result, "MultiFormat Mode",	fDoMultiFormat ? "Y" : "N");
	AJASystemInfo::append (result, "HDR Anc Type",		::AJAAncillaryDataTypeToString(fTransmitHDRType));
	AJASystemInfo::append (result, "Output Channel",	::NTV2ChannelToString(fOutputChannel, inCompact));
	AJASystemInfo::append (result, "HDMI Output",		fDoHDMIOutput ? "Yes" : "No");
	AJASystemInfo::append (result, "Tsi Routing",		fDoTsiRouting ? "Yes" : "No");
	AJASystemInfo::append (result, "RGB-On-SDI",		fDoRGBOnWire ? "Yes" : "No");
	AJASystemInfo::append (result, "Audio",				WithAudio() ? "Yes" : "No");
	ostringstream numLinks;  numLinks << DEC(fNumAudioLinks);
	AJASystemInfo::append (result, "Num Audio Links",	numLinks.str());
	strm << AJASystemInfo::ToString(result);
	return strm;
}
