/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn.cpp
	@brief		Implementation of NTV2Burn demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2burn.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ajabase/common/types.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include <iostream>

using namespace std;

//#define NTV2_BUFFER_LOCKING		//	Define this to pre-lock video/audio buffers in kernel


/**
	@brief	The maximum number of bytes of ancillary data that can be transferred for a single field.
			Each driver instance sets this maximum to the 8K default at startup.
			It can be changed at runtime, so it's sampled and reset in SetUpVideo.
**/
static ULWord			gAncMaxSizeBytes	(NTV2_ANCSIZE_MAX);	//	Max per-frame anc buffer size, in bytes

/**
	@brief	The maximum number of bytes of 48KHz audio that can be transferred for a single frame.
			Worst case, assuming 16 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			48,000 samples per second requires 3,204 samples x 4 bytes/sample x 16 = 205,056 bytes
			201K min will suffice, with 768 bytes to spare
			But it could be more efficient for page-aligned (and page-locked) memory to round to 256K.
**/
static const uint32_t	gAudMaxSizeBytes	(256 * 1024);	//	Max per-frame audio buffer size, in bytes

/**
	@brief	The alignment of the video buffers has a big impact on the efficiency of DMA transfers.
			When aligned to the host operating systems' page size, only one DMA descriptor is needed
			per page. Misalignment will double the number of descriptors that need to be fetched and
			processed, thus reducing bandwidth.
**/
static const uint32_t	BUFFER_ALIGNMENT	(4096);		//	The optimal size for most modern hosts
static const bool		BUFFER_PAGE_ALIGNED	(true);		//	Set this false to disable page-alignment

static const uint32_t	kAppSignature		(NTV2_FOURCC('B','u','r','n'));


//////////////////////	IMPLEMENTATION


NTV2Burn::NTV2Burn (const BurnConfig & inConfig)
	:	mConfig			(inConfig),
		mPlayThread		(AJAThread()),
		mCaptureThread	(AJAThread()),
		mDeviceID		(DEVICE_ID_NOTFOUND),
		mVideoFormat	(NTV2_FORMAT_UNKNOWN),
		mSavedTaskMode	(NTV2_DISABLE_TASKS),
		mOutputDest		(NTV2_OUTPUTDESTINATION_INVALID),
		mAudioSystem	(inConfig.WithAudio() ? ::NTV2InputSourceToAudioSystem(inConfig.fInputSource) : NTV2_AUDIOSYSTEM_INVALID),
		mGlobalQuit		(false)
{
}	//	constructor


NTV2Burn::~NTV2Burn ()
{
	//	Stop my capture and playout threads, then destroy them...
	Quit();

	mDevice.UnsubscribeInputVerticalEvent(mConfig.fInputChannel);	//	Unsubscribe from input VBI event
}	//	destructor


void NTV2Burn::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mPlayThread.Active())
		AJATime::Sleep(10);

	while (mCaptureThread.Active())
		AJATime::Sleep(10);

	if (!mConfig.fDoMultiFormat  &&  mDevice.IsOpen())
	{
		mDevice.ReleaseStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid()));	//	Release the device
		if (NTV2_IS_VALID_TASK_MODE(mSavedTaskMode))
			mDevice.SetEveryFrameServices(mSavedTaskMode);	//	Restore prior task mode
	}
}	//	Quit


AJAStatus NTV2Burn::Init (void)
{
	AJAStatus	status(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetStreamingApplication (appSignature, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);			//	Save the current device state
	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Set the OEM service level
		mDevice.ClearRouting();									//	Clear the current device routing (since I "own" the device)
	}
	else
		mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Force OEM tasks

	mDeviceID = mDevice.GetDeviceID();							//	Keep the device ID handy since it will be used frequently

	//	Configure the SDI relays if present
	if (::NTV2DeviceHasSDIRelays(mDeviceID))
	{
		//	Note that if the board's jumpers are not set in the position
		//	to enable the watchdog timer, these calls will have no effect.
		mDevice.SetSDIWatchdogEnable(true, 0);	//	SDI 1/2
		mDevice.SetSDIWatchdogEnable(true, 1);	//	SDI 3/4

		//	Set timeout delay to 2 seconds expressed in multiples of 8 ns
		//	and take the relays out of bypass...
		mDevice.SetSDIWatchdogTimeout(2 * 125000000);
		mDevice.KickSDIWatchdog();

		//	Give the mechanical relays some time to switch...
		AJATime::Sleep(500);
	}

	//	Make sure the device actually supports custom anc before using it...
	if (!mConfig.fSuppressAnc)
		mConfig.fSuppressAnc = !::NTV2DeviceCanDoCustomAnc(mDeviceID);

	//	Set up the video and audio...
	status = SetupVideo();
	if (AJA_FAILURE(status))
		return status;
	status = mConfig.WithAudio() ? SetupAudio() : AJA_STATUS_SUCCESS;
	if (AJA_FAILURE(status))
		return status;

	//	Set up the circular buffers...
	status = SetupHostBuffers();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the signal routing...
	RouteInputSignal();
	RouteOutputSignal();

	//	Lastly, prepare my AJATimeCodeBurn instance...
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat(mConfig.fPixelFormat), mFormatDesc.numPixels, mFormatDesc.numLines);

	//	Ready to go...
	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#endif	//	not _DEBUG
	BURNINFO("Configuration: " << mConfig);
	return status;

}	//	Init


AJAStatus NTV2Burn::SetupVideo (void)
{
	const UWord	numFrameStores	(::NTV2DeviceGetNumFrameStores (mDeviceID));

	//	Does this device have the requested input source?
	if (!::NTV2DeviceCanDoInputSource (mDeviceID, mConfig.fInputSource))
		{cerr << "## ERROR:  Device does not have the specified input source" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Pick an input NTV2Channel from the input source, and enable its frame buffer...
	mConfig.fInputChannel = NTV2_INPUT_SOURCE_IS_ANALOG(mConfig.fInputSource) ? NTV2_CHANNEL1 : ::NTV2InputSourceToChannel(mConfig.fInputSource);
	mDevice.EnableChannel (mConfig.fInputChannel);		//	Enable the input frame buffer

	//	Pick an appropriate output NTV2Channel, and enable its frame buffer...
	switch (mConfig.fInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		mConfig.fOutputChannel = numFrameStores == 2 || numFrameStores > 4 ? NTV2_CHANNEL2 : NTV2_CHANNEL3;	break;

		case NTV2_INPUTSOURCE_HDMI2:
		case NTV2_INPUTSOURCE_SDI2:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL3 : NTV2_CHANNEL4;						break;

		case NTV2_INPUTSOURCE_HDMI3:
		case NTV2_INPUTSOURCE_SDI3:		mConfig.fOutputChannel = NTV2_CHANNEL4;																break;

		case NTV2_INPUTSOURCE_HDMI4:
		case NTV2_INPUTSOURCE_SDI4:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL5 : NTV2_CHANNEL3;						break;

		case NTV2_INPUTSOURCE_SDI5: 	mConfig.fOutputChannel = NTV2_CHANNEL6;																break;
		case NTV2_INPUTSOURCE_SDI6:		mConfig.fOutputChannel = NTV2_CHANNEL7;																break;
		case NTV2_INPUTSOURCE_SDI7:		mConfig.fOutputChannel = NTV2_CHANNEL8;																break;
		case NTV2_INPUTSOURCE_SDI8:		mConfig.fOutputChannel = NTV2_CHANNEL7;																break;

		case NTV2_INPUTSOURCE_ANALOG1:
		case NTV2_INPUTSOURCE_HDMI1:	mConfig.fOutputChannel = numFrameStores < 3 ? NTV2_CHANNEL2 : NTV2_CHANNEL3;
										mAudioSystem = NTV2_AUDIOSYSTEM_2;
										break;

		case NTV2_INPUTSOURCE_INVALID:	cerr << "## ERROR:  Bad input source" << endl;  return AJA_STATUS_BAD_PARAM;
	}
	mDevice.EnableChannel(mConfig.fOutputChannel);	//	Enable the output frame buffer

	//	Enable/subscribe interrupts...
	mDevice.EnableInputInterrupt(mConfig.fInputChannel);
	mDevice.SubscribeInputVerticalEvent(mConfig.fInputChannel);
	mDevice.EnableOutputInterrupt(mConfig.fOutputChannel);
	mDevice.SubscribeOutputVerticalEvent(mConfig.fOutputChannel);

	//	Pick an appropriate output spigot based on the output channel...
	mOutputDest	= ::NTV2ChannelToOutputDestination(mConfig.fOutputChannel);
	if (!::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt12GSDIOut2) && !::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut2) && !::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut2))
		mOutputDest = NTV2_OUTPUTDESTINATION_SDI1;							//	If device has only one SDI output
	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID)							//	If device has bidirectional SDI connectors...
		&& NTV2_OUTPUT_DEST_IS_SDI(mOutputDest))							//	...and output destination is SDI...
			mDevice.SetSDITransmitEnable (mConfig.fOutputChannel, true);	//	...then enable transmit mode

	//	Flip the input spigot to "receive" if necessary...
	bool	isTransmit	(false);
	if (::NTV2DeviceHasBiDirectionalSDI (mDevice.GetDeviceID ())				//	If device has bidirectional SDI connectors...
		&& NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource)						//	...and desired input source is SDI...
			&& mDevice.GetSDITransmitEnable (mConfig.fInputChannel, isTransmit)	//	...and GetSDITransmitEnable succeeds...
				&& isTransmit)													//	...and input is set to "transmit"...
	{
		mDevice.SetSDITransmitEnable (mConfig.fInputChannel, false);			//	...then disable transmit mode...
		mDevice.WaitForOutputVerticalInterrupt (mConfig.fOutputChannel, 12);	//	...and give the device a dozen frames or so to lock to the input signal
	}	//	if input SDI connector needs to switch from transmit mode

	//	Is there an input signal?  What format is it?
	mVideoFormat = mDevice.GetInputVideoFormat(mConfig.fInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		{cerr << "## ERROR:  No input signal, or can't handle its format" << endl;  return AJA_STATUS_NOINPUT;}

	//	Free-run the device clock, since E-to-E mode isn't used, nor is a mixer tied to the input...
	mDevice.SetReference (NTV2_REFERENCE_FREERUN);

	//	Check the timecode source...
	if (NTV2_IS_SDI_TIMECODE_INDEX(mConfig.fTimecodeSource))
	{
		const NTV2Channel	tcChannel	(::NTV2TimecodeIndexToChannel(mConfig.fTimecodeSource));
		const NTV2Channel	endNum		(NTV2Channel (::NTV2DeviceGetNumVideoChannels (mDeviceID)));
		if (tcChannel >= endNum)
			{cerr << "## ERROR:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' illegal on this device" << endl;  return AJA_STATUS_BAD_PARAM;}
		if (tcChannel == mConfig.fOutputChannel)
			{cerr << "## ERROR:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' conflicts with output channel" << endl;  return AJA_STATUS_BAD_PARAM;}
		if (::NTV2DeviceHasBiDirectionalSDI (mDevice.GetDeviceID ())	//	If device has bidirectional SDI connectors...
			&& mDevice.GetSDITransmitEnable (tcChannel, isTransmit)		//	...and GetSDITransmitEnable succeeds...
				&& isTransmit)											//	...and the SDI timecode source is set to "transmit"...
		{
			mDevice.SetSDITransmitEnable (tcChannel, false);			//	...then disable transmit mode...
			AJATime::Sleep (500);										//	...and give the device a dozen frames or so to lock to the input signal
		}	//	if input SDI connector needs to switch from transmit mode

		// configure for vitc capture (should the driver do this?)
		mDevice.SetRP188SourceFilter(tcChannel, 0x01);

		const NTV2VideoFormat	tcInputVideoFormat	(mDevice.GetInputVideoFormat (::NTV2TimecodeIndexToInputSource(mConfig.fTimecodeSource)));
		if (tcInputVideoFormat == NTV2_FORMAT_UNKNOWN)
			cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' has no input signal" << endl;
		if (!InputSignalHasTimecode ())
			cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' has no embedded timecode" << endl;
	}
	else if (NTV2_IS_ANALOG_TIMECODE_INDEX(mConfig.fTimecodeSource) && !AnalogLTCInputHasTimecode ())
		cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' has no embedded timecode" << endl;

	//	If the device supports different per-channel video formats, configure it as requested...
	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);

	//	Set the input/output channel video formats to the video format that was detected earlier...
	mDevice.SetVideoFormat (mVideoFormat, false, false, ::NTV2DeviceCanDoMultiFormat(mDeviceID) ? mConfig.fInputChannel : NTV2_CHANNEL1);
	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))										//	If device supports multiple formats per-channel...
		mDevice.SetVideoFormat (mVideoFormat, false, false, mConfig.fOutputChannel);	//	...then also set the output channel format to the detected input format

	//	Can the device handle the requested frame buffer pixel format?
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.fPixelFormat))
		{cerr << "## ERROR:  Device doesn't support '" << ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat, true) << "'" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Set both input and output frame buffers' pixel formats...
	mDevice.SetFrameBufferFormat (mConfig.fInputChannel, mConfig.fPixelFormat);
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);

	//	Normally, timecode embedded in the output signal comes from whatever is written into the RP188
	//	registers (30/31 for SDI out 1, 65/66 for SDIout2, etc.).
	//	AutoCirculate automatically writes the timecode in the AUTOCIRCULATE_TRANSFER's acRP188 field
	//	into these registers (if AutoCirculateInitForOutput was called with AUTOCIRCULATE_WITH_RP188 set).
	//	Newer AJA devices can also bypass these RP188 registers, and simply copy whatever timecode appears
	//	at any SDI input (called the "bypass source"). To ensure that AutoCirculate's playout timecode
	//	will actually be seen in the output signal, "bypass mode" must be disabled...
	bool	bypassIsEnabled	(false);
	mDevice.IsRP188BypassEnabled (::NTV2InputSourceToChannel(mConfig.fInputSource), bypassIsEnabled);
	if (bypassIsEnabled)
		mDevice.DisableRP188Bypass (::NTV2InputSourceToChannel(mConfig.fInputSource));

	//	Now that newer AJA devices can capture/play anc data from separate buffers,
	//	there's no need to enable VANC frame geometries...
	mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fInputChannel);
	mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fOutputChannel);
	if (::Is8BitFrameBufferFormat(mConfig.fPixelFormat))
	{	//	8-bit FBFs:  since not using VANC geometries, disable bit shift...
		mDevice.SetVANCShiftMode (mConfig.fInputChannel, NTV2_VANCDATA_NORMAL);
		mDevice.SetVANCShiftMode (mConfig.fOutputChannel, NTV2_VANCDATA_NORMAL);
	}

	if (NTV2_IS_ANALOG_TIMECODE_INDEX(mConfig.fTimecodeSource))
		mDevice.SetLTCInputEnable (true);	//	Enable analog LTC input (some LTC inputs are shared with reference input)

	//	Get current per-field maximum Anc buffer size...
	if (!mDevice.GetAncRegionOffsetFromBottom (gAncMaxSizeBytes, NTV2_AncRgn_Field2))
		gAncMaxSizeBytes = NTV2_ANCSIZE_MAX;

	//	Now that the video is set up, get information about the current frame geometry...
	mFormatDesc = NTV2FormatDescriptor (mVideoFormat, mConfig.fPixelFormat, NTV2_VANCMODE_OFF);
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2Burn::SetupAudio (void)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
		return AJA_STATUS_SUCCESS;

	//	Have the audio subsystem capture audio from the designated input source...
	mDevice.SetAudioSystemInputSource (mAudioSystem, ::NTV2InputSourceToAudioSource(mConfig.fInputSource), ::NTV2InputSourceToEmbeddedAudioInput(mConfig.fInputSource));

	//	It's best to use all available audio channels...
	mDevice.SetNumberAudioChannels (::NTV2DeviceGetMaxAudioChannels(mDeviceID), mAudioSystem);

	//	Assume 48kHz PCM...
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	4MB device audio buffers work best...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set up the output audio embedders...
	if (::NTV2DeviceGetNumAudioSystems(mDeviceID) > 1)
	{
		//	Some devices, like the Kona1, have 2 FrameStores but only 1 SDI output,
		//	which makes mConfig.fOutputChannel == NTV2_CHANNEL2, but need SDIoutput to be NTV2_CHANNEL1...
		UWord	SDIoutput(mConfig.fOutputChannel);
		if (SDIoutput >= ::NTV2DeviceGetNumVideoOutputs(mDeviceID))
			SDIoutput = ::NTV2DeviceGetNumVideoOutputs(mDeviceID) - 1;
		mDevice.SetSDIOutputAudioSystem (NTV2Channel(SDIoutput), mAudioSystem);

		if (::NTV2DeviceGetNumHDMIVideoOutputs(mDeviceID) > 0)
			mDevice.SetHDMIOutAudioSource2Channel(NTV2_AudioChannel1_2, mAudioSystem);
	}

	//
	//	Loopback mode plays whatever audio appears in the input signal when it's
	//	connected directly to an output (i.e., "end-to-end" mode). If loopback is
	//	left enabled, the video will lag the audio as video frames get briefly delayed
	//	in our ring buffer. Audio, therefore, needs to come out of the (buffered) frame
	//	data being played, so loopback must be turned off...
	//
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


AJAStatus NTV2Burn::SetupHostBuffers (void)
{
	if (NTV2_POINTER::DefaultPageSize() != BUFFER_ALIGNMENT)
	{
		PLNOTE("Buffer alignment changed from " << xHEX0N(NTV2_POINTER::DefaultPageSize(),8) << " to " << xHEX0N(BUFFER_ALIGNMENT,8));
		NTV2_POINTER::SetDefaultPageSize(BUFFER_ALIGNMENT);
	}

	//	Let my circular buffer know when it's time to quit...
	mFrameDataRing.SetAbortFlag (&mGlobalQuit);

	//	Allocate and add each in-host NTV2FrameData to my mFrameDataRing...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());		//	Make a new NTV2FrameData...
		NTV2FrameData & frameData(mHostBuffers.back());	//	...and get a reference to it

		//	Allocate a page-aligned video buffer (if handling video)...
		const ULWord vidBuffSizeBytes (mFormatDesc.GetVideoWriteSize(ULWord(NTV2_POINTER::DefaultPageSize())));
		if (!mConfig.fSuppressVideo)
			if (!frameData.fVideoBuffer.Allocate (vidBuffSizeBytes, BUFFER_PAGE_ALIGNED))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(vidBuffSizeBytes,8) << "-byte video buffer");
				return AJA_STATUS_MEMORY;
			}
		#ifdef NTV2_BUFFER_LOCKING
		if (frameData.fVideoBuffer)
			mDevice.DMABufferLock(frameData.fVideoBuffer, true);
		#endif

		//	Allocate a page-aligned audio buffer (if handling audio)...
		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem)  &&  mConfig.WithAudio())
			if (!frameData.fAudioBuffer.Allocate (gAudMaxSizeBytes, BUFFER_PAGE_ALIGNED))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(gAudMaxSizeBytes,8) << "-byte audio buffer");
				return AJA_STATUS_MEMORY;
			}
		if (frameData.AudioBuffer())
			frameData.fAudioBuffer.Fill(ULWord(0));

		if (mConfig.WithAnc())
		{	//	Allocate page-aligned anc buffers...
			if (!frameData.fAncBuffer.Allocate(gAncMaxSizeBytes, BUFFER_PAGE_ALIGNED))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(gAncMaxSizeBytes,8) << "-byte anc buffer");
				return AJA_STATUS_MEMORY;
			}
			if (!::IsProgressivePicture(mVideoFormat))
				if (!frameData.fAncBuffer2.Allocate(gAncMaxSizeBytes, BUFFER_PAGE_ALIGNED))
				{
					BURNFAIL("Failed to allocate " << xHEX0N(gAncMaxSizeBytes,8) << "-byte F2 anc buffer");
					return AJA_STATUS_MEMORY;
				}
		}
		if (frameData.AncBuffer())
			frameData.AncBuffer().Fill(ULWord(0));
		if (frameData.AncBuffer2())
			frameData.AncBuffer2().Fill(ULWord(0));

		//	Add this NTV2FrameData to the ring...
		mFrameDataRing.Add (&frameData);
	}	//	for each NTV2FrameData

	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2Burn::RouteInputSignal (void)
{
	const NTV2OutputCrosspointID	inputOutputXpt	(::GetInputSourceOutputXpt(mConfig.fInputSource));
	const NTV2InputCrosspointID		fbInputXpt		(::GetFrameBufferInputXptFromChannel(mConfig.fInputChannel));

	if (::IsRGBFormat(mConfig.fPixelFormat))
	{
		//	If the frame buffer is configured for RGB pixel format, incoming YUV must be converted.
		//	This routes the video signal from the input through a color space converter before
		//	connecting to the RGB frame buffer...
		const NTV2InputCrosspointID		cscVideoInputXpt	(::GetCSCInputXptFromChannel (mConfig.fInputChannel));
		const NTV2OutputCrosspointID	cscOutputXpt		(::GetCSCOutputXptFromChannel (mConfig.fInputChannel, false/*isKey*/, true/*isRGB*/));	//	Use CSC's RGB video output

		mDevice.Connect (cscVideoInputXpt, inputOutputXpt);	//	Connect the CSC's video input to the input spigot's output
		mDevice.Connect (fbInputXpt, cscOutputXpt);			//	Connect the frame store's input to the CSC's output
	}
	else
		mDevice.Connect (fbInputXpt, inputOutputXpt);		//	Route the YCbCr signal directly from the input to the frame buffer's input

}	//	RouteInputSignal


void NTV2Burn::RouteOutputSignal (void)
{
	const NTV2InputCrosspointID		outputInputXpt	(::GetOutputDestInputXpt(mOutputDest));
	const NTV2OutputCrosspointID	fbOutputXpt		(::GetFrameBufferOutputXptFromChannel (mConfig.fOutputChannel, ::IsRGBFormat(mConfig.fPixelFormat)));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

	if (::IsRGBFormat(mConfig.fPixelFormat))
	{
		const NTV2OutputCrosspointID	cscVidOutputXpt	(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel));	//	Use CSC's YUV video output
		const NTV2InputCrosspointID		cscVidInputXpt	(::GetCSCInputXptFromChannel(mConfig.fOutputChannel));

		mDevice.Connect (cscVidInputXpt, fbOutputXpt);		//	Connect the CSC's video input to the frame store's output
		mDevice.Connect (outputInputXpt, cscVidOutputXpt);	//	Connect the SDI output's input to the CSC's video output
		outputXpt = cscVidOutputXpt;
	}
	else
		mDevice.Connect (outputInputXpt, outputXpt);

	mTCOutputs.clear ();
	mTCOutputs.insert (::NTV2ChannelToTimecodeIndex(mConfig.fOutputChannel));

	if (!mConfig.fDoMultiFormat)
	{
		//	Route all SDI outputs to the outputXpt...
		const NTV2Channel	startNum		(NTV2_CHANNEL1);
		const NTV2Channel	endNum			(NTV2Channel (::NTV2DeviceGetNumVideoChannels (mDeviceID)));
		const NTV2Channel	tcInputChannel	(NTV2_IS_SDI_TIMECODE_INDEX(mConfig.fTimecodeSource) ? ::NTV2TimecodeIndexToChannel(mConfig.fTimecodeSource) : NTV2_CHANNEL_INVALID);
		NTV2WidgetID		outputWidgetID	(NTV2_WIDGET_INVALID);

		for (NTV2Channel chan (startNum);  chan < endNum;  chan = NTV2Channel (chan + 1))
		{
			// this kills vitc capture
//			mDevice.SetRP188SourceFilter (chan, 0);	//	Set all SDI spigots to capture embedded LTC (VITC could be an option)

			if (chan == mConfig.fInputChannel  ||  chan == mConfig.fOutputChannel)
				continue;	//	Skip the input & output channel, already routed
			if (NTV2_IS_VALID_CHANNEL (tcInputChannel) && chan == tcInputChannel)
				continue;	//	Skip the timecode input channel
			if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
				mDevice.SetSDITransmitEnable (chan, true);
			if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, ::NTV2DeviceCanDoDualLink (mDeviceID)), outputWidgetID, mDeviceID))
				if (::NTV2DeviceCanDoWidget (mDeviceID, outputWidgetID))
				{
					mDevice.Connect (::GetSDIOutputInputXpt (chan), outputXpt);
					mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan));
					mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan, true));
				}
		}	//	for each output spigot

		//	If HDMI and/or analog video outputs are available, route them, too...
		if (::NTV2DeviceGetNumHDMIVideoOutputs(mDeviceID) > 0)
			mDevice.Connect (NTV2_XptHDMIOutQ1Input, outputXpt);	//	Route the output signal to the HDMI output
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, outputXpt);		//	Route the output signal to the Analog output
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIMonOut1))
			mDevice.Connect (::GetSDIOutputInputXpt (NTV2_CHANNEL5), outputXpt);	//	Route the output signal to the SDI monitor output
	}
//	cerr << "## DEBUG:  " << mTCOutputs.size () << " timecode destination(s):  " << mTCOutputs << endl;

}	//	RouteOutputSignal


AJAStatus NTV2Burn::Run ()
{
	//	Start the playout and capture threads...
	StartPlayThread();
	StartCaptureThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we will start the play thread
void NTV2Burn::StartPlayThread (void)
{
	//	Create and start the playout thread...
	mPlayThread.Attach(PlayThreadStatic, this);
	mPlayThread.SetPriority(AJA_ThreadPriority_High);
	mPlayThread.Start();

}	//	StartPlayThread


//	The playout thread function
void NTV2Burn::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Burn instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2Burn * pApp(reinterpret_cast<NTV2Burn*>(pContext));
	pApp->PlayFrames();

}	//	PlayThreadStatic


void NTV2Burn::PlayFrames (void)
{
	const ULWord			acOptions (AUTOCIRCULATE_WITH_RP188 | (mConfig.fSuppressAnc ? 0 : AUTOCIRCULATE_WITH_ANC));
	ULWord					goodXfers(0), badXfers(0), starves(0), noRoomWaits(0);
	AUTOCIRCULATE_TRANSFER	outputXferInfo;
	AUTOCIRCULATE_STATUS	outputStatus;

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel, 4);	//	Let it stop
	BURNNOTE("Thread started");

	//	Initialize AutoCirculate...
	if (!mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,  mConfig.fOutputFrames.count(),  mAudioSystem,  acOptions,
											1 /*numChannels*/,  mConfig.fOutputFrames.firstFrame(),  mConfig.fOutputFrames.lastFrame()))
		{PLFAIL("AutoCirculateInitForOutput failed");  mGlobalQuit = true;}
	else if (!mConfig.WithVideo())
	{	//	Video suppressed --
		//	Clear device frame buffers being AutoCirculated (prevent garbage output frames)
		NTV2_POINTER tmpFrame (mFormatDesc.GetVideoWriteSize());
		NTV2TestPatternGen blackPatternGen;
		blackPatternGen.DrawTestPattern (NTV2_TestPatt_Black, mFormatDesc, tmpFrame);
		mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outputStatus);
		for (uint16_t frmNum(outputStatus.GetStartFrame());  frmNum <= outputStatus.GetEndFrame();  frmNum++)
			mDevice.DMAWriteFrame(ULWord(frmNum), tmpFrame, mFormatDesc.GetTotalBytes(), mConfig.fOutputChannel);
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
				{starves++;  continue;}	//	Producer thread isn't producing frames fast enough

			//	Prepare to transfer this timecode-burned frame to the device for playout.
			//	Set the XferInfo struct's video, audio and anc buffers from playData's buffers...
			if (pFrameData->VideoBuffer())
				outputXferInfo.SetVideoBuffer (pFrameData->VideoBuffer(), pFrameData->VideoBufferSize());
			if (pFrameData->AudioBuffer())
				outputXferInfo.SetAudioBuffer (pFrameData->AudioBuffer(), pFrameData->NumCapturedAudioBytes());
			if (pFrameData->AncBuffer() || pFrameData->AncBuffer2())
				outputXferInfo.SetAncBuffers (pFrameData->AncBuffer(), pFrameData->NumCapturedAncBytes(), pFrameData->AncBuffer2(), pFrameData->NumCapturedAnc2Bytes());

			//	Tell AutoCirculate to embed this frame's timecode(s) into the SDI output(s)...
			outputXferInfo.SetOutputTimeCodes(pFrameData->fTimecodes);

			//	Transfer the frame to the device for eventual playout...
			if (mDevice.AutoCirculateTransfer (mConfig.fOutputChannel, outputXferInfo))
				goodXfers++;
			else
				badXfers++;

			if (goodXfers == 3)	//	Start AutoCirculate playout once 3 frames are buffered on the device...
				mDevice.AutoCirculateStart(mConfig.fOutputChannel);

			//	Signal that the frame has been "consumed"...
			mFrameDataRing.EndConsumeNextBuffer ();
			continue;	//	Back to top of while loop
		}	//	if CanAcceptMoreOutputFrames

		//	Wait for one or more buffers to become available on the device, which should occur at next VBI...
		noRoomWaits++;
		mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel);
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	PLNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(starves) << " ring starves, " << DEC(noRoomWaits) << " device starves");
}	//	PlayFrames


//////////////////////////////////////////////



//////////////////////////////////////////////
//
//	This is where the capture thread gets started
//
void NTV2Burn::StartCaptureThread (void)
{
	//	Create and start the capture thread...
	mCaptureThread.Attach(CaptureThreadStatic, this);
	mCaptureThread.SetPriority(AJA_ThreadPriority_High);
	mCaptureThread.Start();

}	//	StartCaptureThread


//
//	The static capture thread function
//
void NTV2Burn::CaptureThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2Burn instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2Burn *	pApp (reinterpret_cast<NTV2Burn*>(pContext));
	pApp->CaptureFrames();
}	//	CaptureThreadStatic


//
//	Repeatedly captures frames until told to stop
//
void NTV2Burn::CaptureFrames (void)
{
	AUTOCIRCULATE_TRANSFER	inputXferInfo;
	const ULWord			acOptions ((mConfig.WithTimecode() ? AUTOCIRCULATE_WITH_RP188 : 0)  |  (mConfig.WithAnc() ? AUTOCIRCULATE_WITH_ANC : 0));
	ULWord					goodXfers(0), badXfers(0), ringFulls(0), devWaits(0);
	Bouncer<UWord>			yPercent	(85/*upperLimit*/, 1/*lowerLimit*/, 1/*startValue*/);	//	"Bounces" timecode up & down in raster
	BURNNOTE("Thread started");

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop(mConfig.fInputChannel);

	//	Initialize AutoCirculate...
	mDevice.AutoCirculateInitForInput (	mConfig.fInputChannel,			//	primary channel
										mConfig.fInputFrames.count(),	//	numFrames (zero if specifying range)
										mAudioSystem,					//	audio system
										acOptions,						//	flags
										1,								//	numChannels to gang
										mConfig.fInputFrames.firstFrame(), mConfig.fInputFrames.lastFrame());

	//	Start AutoCirculate running...
	mDevice.AutoCirculateStart(mConfig.fInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS acStatus;
		mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, acStatus);

		if (::NTV2DeviceHasSDIRelays(mDeviceID))
			mDevice.KickSDIWatchdog();	//	Prevent watchdog from timing out and putting the relays into bypass mode

		if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
		{
			//	At this point, there's at least one fully-formed frame available in the device's frame buffer
			//	memory waiting to be transferred to the host. Reserve an NTV2FrameData to fill ("produce"),
			//	and use it in the next frame transferred from the device...
			NTV2FrameData *	pFrameData	(mFrameDataRing.StartProduceNextBuffer ());
			if (!pFrameData)
				{ringFulls++;  continue;}	//	Ring full -- consumer thread isn't consuming frames fast enough

			if (pFrameData->VideoBuffer())
				inputXferInfo.SetVideoBuffer (pFrameData->VideoBuffer(), pFrameData->VideoBufferSize());
			if (pFrameData->AudioBuffer())
				inputXferInfo.SetAudioBuffer (pFrameData->AudioBuffer(), pFrameData->AudioBufferSize());
			if (pFrameData->AncBuffer()  ||  pFrameData->AncBuffer2())
				inputXferInfo.SetAncBuffers (pFrameData->AncBuffer(), pFrameData->AncBufferSize(), pFrameData->AncBuffer2(), pFrameData->AncBuffer2Size());

			//	Transfer the frame from the device into our host AVDataBuffer...
			if (mDevice.AutoCirculateTransfer (mConfig.fInputChannel, inputXferInfo))
				goodXfers++;
			else
				badXfers++;

			//	Remember the amount, in bytes, of captured audio & anc data...
			if (pFrameData->AudioBuffer())
				pFrameData->fNumAudioBytes = inputXferInfo.GetCapturedAudioByteCount();
			if (pFrameData->AncBuffer())
				pFrameData->fNumAncBytes = inputXferInfo.GetCapturedAncByteCount(false/*F1*/);
			if (pFrameData->AncBuffer2())
				pFrameData->fNumAnc2Bytes = inputXferInfo.GetCapturedAncByteCount(true/*F2*/);

			//	Get a timecode to use for burn-in...
			NTV2_RP188	thisFrameTC;
			inputXferInfo.GetInputTimeCodes (pFrameData->fTimecodes, mConfig.fInputChannel, /*ValidOnly*/ true);
			if (!pFrameData->HasValidTimecode(mConfig.fTimecodeSource))
			{	//	Invent a timecode (based on frame count)...
				const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
				const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
				const	CRP188			inventedTC		(inputXferInfo.GetTransferStatus().GetProcessedFrameCount(), 0, 0, 10, tcFormat);
				inventedTC.GetRP188Reg(thisFrameTC);
			}
			CRP188 tc(thisFrameTC);
			string tcStr;
			tc.GetRP188Str(tcStr);

			//	"Burn" the timecode into the host AVDataBuffer while it's locked for our exclusive access...
			mTCBurner.BurnTimeCode (pFrameData->VideoBuffer(), tcStr.c_str(), yPercent.Next());

			//	Signal that we're done "producing" this frame, making it available for future "consumption"...
			mFrameDataRing.EndProduceNextBuffer();
		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			devWaits++;
			mDevice.WaitForInputFieldID (NTV2_FIELD0, mConfig.fInputChannel);
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop (mConfig.fInputChannel);
	BURNNOTE("Thread completed, will exit");
	CAPNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(ringFulls) << " ring full(s), " << DEC(devWaits) << " device waits");

}	//	CaptureFrames


//////////////////////////////////////////////


void NTV2Burn::GetStatus (AUTOCIRCULATE_STATUS & outInputStatus, AUTOCIRCULATE_STATUS & outOutputStatus)
{
	mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, outInputStatus);
	mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outOutputStatus);

}	//	GetStatus


static ULWord GetRP188RegisterForInput (const NTV2InputSource inInputSource)
{
	switch (inInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		return kRegRP188InOut1DBB;	//	reg 29
		case NTV2_INPUTSOURCE_SDI2:		return kRegRP188InOut2DBB;	//	reg 64
		case NTV2_INPUTSOURCE_SDI3:		return kRegRP188InOut3DBB;	//	reg 268
		case NTV2_INPUTSOURCE_SDI4:		return kRegRP188InOut4DBB;	//	reg 273
		case NTV2_INPUTSOURCE_SDI5:		return kRegRP188InOut5DBB;	//	reg 342
		case NTV2_INPUTSOURCE_SDI6:		return kRegRP188InOut6DBB;	//	reg 418
		case NTV2_INPUTSOURCE_SDI7:		return kRegRP188InOut7DBB;	//	reg 427
		case NTV2_INPUTSOURCE_SDI8:		return kRegRP188InOut8DBB;	//	reg 436
		default:						return 0;
	}	//	switch on input source

}	//	GetRP188RegisterForInput


bool NTV2Burn::InputSignalHasTimecode (void)
{
	const ULWord regNum (::GetRP188RegisterForInput(mConfig.fInputSource));
	ULWord regValue(0);

	//	Bit 16 of the RP188 DBB register will be set if there is timecode embedded in the input signal...
	return regNum  &&  mDevice.ReadRegister(regNum, regValue)  &&  regValue & BIT(16);

}	//	InputSignalHasTimecode


bool NTV2Burn::AnalogLTCInputHasTimecode (void)
{
	if (mConfig.fTimecodeSource != NTV2_TCINDEX_LTC1  &&  mConfig.fTimecodeSource != NTV2_TCINDEX_LTC2)
		return false;
	const ULWord regMask(mConfig.fTimecodeSource == NTV2_TCINDEX_LTC1 ? kRegMaskLTC1InPresent : kRegMaskLTC2InPresent);
	bool result(false);
	reinterpret_cast<CNTV2DriverInterface&>(mDevice).ReadRegister(kRegLTCStatusControl, result, regMask);
	return result;

}	//	AnalogLTCInputHasTimecode

AJALabelValuePairs BurnConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result;
	AJASystemInfo::append(result, "NTV2Burn Config");
	AJASystemInfo::append(result, "Device Specifier",	fDeviceSpec);
	AJASystemInfo::append(result, "Input Channel",		::NTV2ChannelToString(fInputChannel, inCompact));
	AJASystemInfo::append(result, "Output Channel",		::NTV2ChannelToString(fOutputChannel, inCompact));
	AJASystemInfo::append(result, "Input Source",		::NTV2InputSourceToString(fInputSource, inCompact));
	if (WithTimecode())
		AJASystemInfo::append(result, "Timecode Source",	::NTV2TCIndexToString(fTimecodeSource, inCompact));
	AJASystemInfo::append(result, "Pixel Format",		::NTV2FrameBufferFormatToString(fPixelFormat, inCompact));
	AJASystemInfo::append(result, "AC Input Frames",	fInputFrames.toString());
	AJASystemInfo::append(result, "AC Output Frames",	fOutputFrames.toString());
	AJASystemInfo::append(result, "Transfer Video",		WithVideo() ? "Y" : "N");
	AJASystemInfo::append(result, "Transfer Audio",		WithAudio() ? "Y" : "N");
	AJASystemInfo::append(result, "Transfer Anc",		WithAnc() ? "Y" : "N");
	AJASystemInfo::append(result, "MultiFormat Mode",	fDoMultiFormat ? "Y" : "N");
	return result;
}
