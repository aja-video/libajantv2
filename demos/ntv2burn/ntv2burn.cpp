/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn.cpp
	@brief		Implementation of NTV2Burn demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2burn.h"
#include "ntv2devicescanner.h"
#include "ntv2testpatterngen.h"	//	Needed for --novideo
#include "ajabase/common/types.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include "ajaanc/includes/ancillarylist.h"
#include <iostream>

using namespace std;

//#define NTV2_BUFFER_LOCKING		//	Define this to pre-lock video/audio buffers in kernel

static const uint32_t	kAppSignature	(NTV2_FOURCC('B','u','r','n'));


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
	Quit();	//	Stop my capture and playout threads, then destroy them
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

	if (!mConfig.fDoMultiFormat)
	{	//	Release the device...
		mDevice.ReleaseStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid()));
		if (NTV2_IS_VALID_TASK_MODE(mSavedTaskMode))
			mDevice.SetEveryFrameServices(mSavedTaskMode);	//	Restore prior task mode
	}
}	//	Quit


AJAStatus NTV2Burn::Init (void)
{
	AJAStatus status (AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID();	//	Keep the device ID handy since it will be used frequently
	if (!mDevice.features().CanDoCapture())
		{cerr << "## ERROR:  Device '" << mDeviceID << "' cannot capture" << endl;  return AJA_STATUS_FEATURE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  Device '" << mDeviceID << "' cannot playout" << endl;  return AJA_STATUS_FEATURE;}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetStreamingApplication (appSignature, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);			//	Save the current device state
	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mDevice.ClearRouting();									//	Clear the current device routing (since I "own" the device)
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Force OEM tasks

	//	Configure the SDI relays if present
	if (mDevice.features().HasSDIRelays())
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
	if (mConfig.WithAnc())
		mConfig.fWithAnc = mDevice.features().CanDoCustomAnc();

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
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mConfig.fPixelFormat),
																		mFormatDesc.numPixels,
																		mFormatDesc.numLines);
	//	Ready to go...
	#if defined(_DEBUG)
		cerr << mConfig;
		if (mDevice.IsRemote())
			cerr	<< "Device Description:  " << mDevice.GetDescription() << endl;
		cerr << endl;
	#endif	//	not _DEBUG
	BURNINFO("Configuration: " << mConfig);
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Burn::SetupVideo (void)
{
	const UWord	numFrameStores	(mDevice.features().GetNumFrameStores());

	//	Does this device have the requested input source?
	if (!mDevice.features().CanDoInputSource(mConfig.fInputSource))
		{cerr << "## ERROR:  Device does not have the specified input source" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Pick an input NTV2Channel from the input source, and enable its frame buffer...
	mConfig.fInputChannel = NTV2_INPUT_SOURCE_IS_ANALOG(mConfig.fInputSource) ? NTV2_CHANNEL1 : ::NTV2InputSourceToChannel(mConfig.fInputSource);
	mDevice.EnableChannel (mConfig.fInputChannel);		//	Enable the input frame buffer

	//	Pick an appropriate output NTV2Channel, and enable its frame buffer...
	switch (mConfig.fInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL5 : (numFrameStores == 2 ? NTV2_CHANNEL2 : NTV2_CHANNEL3);
										break;

		case NTV2_INPUTSOURCE_HDMI2:
		case NTV2_INPUTSOURCE_SDI2:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL3 : NTV2_CHANNEL4;
										break;

		case NTV2_INPUTSOURCE_HDMI3:
		case NTV2_INPUTSOURCE_SDI3:		mConfig.fOutputChannel = NTV2_CHANNEL4;
										break;

		case NTV2_INPUTSOURCE_HDMI4:
		case NTV2_INPUTSOURCE_SDI4:		mConfig.fOutputChannel = numFrameStores > 4 ? NTV2_CHANNEL5 : NTV2_CHANNEL3;
										break;

		case NTV2_INPUTSOURCE_SDI5: 	mConfig.fOutputChannel = NTV2_CHANNEL6;		break;
		case NTV2_INPUTSOURCE_SDI6:		mConfig.fOutputChannel = NTV2_CHANNEL7;		break;
		case NTV2_INPUTSOURCE_SDI7:		mConfig.fOutputChannel = NTV2_CHANNEL8;		break;
		case NTV2_INPUTSOURCE_SDI8:		mConfig.fOutputChannel = NTV2_CHANNEL7;		break;

		case NTV2_INPUTSOURCE_ANALOG1:
		case NTV2_INPUTSOURCE_HDMI1:	mConfig.fOutputChannel = numFrameStores < 3 ? NTV2_CHANNEL2 : NTV2_CHANNEL3;
										mAudioSystem = NTV2_AUDIOSYSTEM_2;
										break;
		default:
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
	if (!mDevice.features().CanDoWidget(NTV2_Wgt12GSDIOut2)
		&& !mDevice.features().CanDoWidget(NTV2_Wgt3GSDIOut2)
		&& !mDevice.features().CanDoWidget(NTV2_WgtSDIOut2))
			mOutputDest = NTV2_OUTPUTDESTINATION_SDI1;						//	If device has only one SDI output
	if (mDevice.features().HasBiDirectionalSDI()							//	If device has bidirectional SDI connectors...
		&& NTV2_OUTPUT_DEST_IS_SDI(mOutputDest))							//	...and output destination is SDI...
			mDevice.SetSDITransmitEnable (mConfig.fOutputChannel, true);	//	...then enable transmit mode

	//	Flip the input spigot to "receive" if necessary...
	bool isXmit (false);
	if (mDevice.features().HasBiDirectionalSDI()							//	If device has bidirectional SDI connectors...
		&& NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource)					//	...and desired input source is SDI...
		&& mDevice.GetSDITransmitEnable (mConfig.fInputChannel, isXmit)		//	...and GetSDITransmitEnable succeeds...
		&& isXmit)															//	...and input is set to "transmit"...
		{
			mDevice.SetSDITransmitEnable (mConfig.fInputChannel, false);			//	...then disable transmit mode...
			mDevice.WaitForOutputVerticalInterrupt (mConfig.fOutputChannel, 10);	//	...and allow device to lock to input signal
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
		const NTV2Channel	endNum		(NTV2Channel (mDevice.features().GetNumVideoChannels()));
		if (tcChannel >= endNum)
			{cerr << "## ERROR:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' illegal on this device" << endl;  return AJA_STATUS_BAD_PARAM;}
		if (tcChannel == mConfig.fOutputChannel)
			{cerr << "## ERROR:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' conflicts with output channel" << endl;  return AJA_STATUS_BAD_PARAM;}
		if (mDevice.features().HasBiDirectionalSDI()	//	If device has bidirectional SDI connectors...
			&& mDevice.GetSDITransmitEnable (tcChannel, isXmit)			//	...and GetSDITransmitEnable succeeds...
			&& isXmit)													//	...and the SDI timecode source is set to "transmit"...
			{
				mDevice.SetSDITransmitEnable (tcChannel, false);		//	...then disable transmit mode...
				AJATime::Sleep (500);									//	...and give the device a dozen frames or so to lock to the input signal
			}	//	if input SDI connector needs to switch from transmit mode

		// configure for vitc capture (should the driver do this?)
		mDevice.SetRP188SourceFilter(tcChannel, 0x01);

		const NTV2VideoFormat tcInputVideoFormat (mDevice.GetInputVideoFormat (::NTV2TimecodeIndexToInputSource(mConfig.fTimecodeSource)));
		if (tcInputVideoFormat == NTV2_FORMAT_UNKNOWN)
			cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' has no input signal" << endl;
		if (!InputSignalHasTimecode ())
			cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' has no embedded timecode" << endl;
	}
	else if (NTV2_IS_ANALOG_TIMECODE_INDEX(mConfig.fTimecodeSource) && !AnalogLTCInputHasTimecode ())
		cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' has no embedded timecode" << endl;

	//	If the device supports different per-channel video formats, configure it as requested...
	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);

	//	Set the input/output channel video formats to the video format that was detected earlier...
	mDevice.SetVideoFormat (mVideoFormat, false, false, mDevice.features().CanDoMultiFormat() ? mConfig.fInputChannel : NTV2_CHANNEL1);
	if (mDevice.features().CanDoMultiFormat())											//	If device supports multiple formats per-channel...
		mDevice.SetVideoFormat (mVideoFormat, false, false, mConfig.fOutputChannel);	//	...then also set the output channel format to the detected input format

	//	Can the device handle the requested frame buffer pixel format?
	if (!mDevice.features().CanDoFrameBufferFormat(mConfig.fPixelFormat))
		{cerr << "## ERROR: " << ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat) << " unsupported" << endl;  return AJA_STATUS_UNSUPPORTED;}

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

	//	Now that the video is set up, get information about the current frame geometry...
	mFormatDesc = NTV2FormatDescriptor (mVideoFormat, mConfig.fPixelFormat);
	if (mFormatDesc.IsPlanar())
		{cerr << "## ERROR: This demo doesn't work with planar pixel formats" << endl;  return AJA_STATUS_UNSUPPORTED;}
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2Burn::SetupAudio (void)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
		return AJA_STATUS_SUCCESS;

	//	Have the audio subsystem capture audio from the designated input source...
	mDevice.SetAudioSystemInputSource (mAudioSystem, ::NTV2InputSourceToAudioSource(mConfig.fInputSource),
										::NTV2InputSourceToEmbeddedAudioInput(mConfig.fInputSource));

	//	It's best to use all available audio channels...
	mDevice.SetNumberAudioChannels (mDevice.features().GetMaxAudioChannels(), mAudioSystem);

	//	Assume 48kHz PCM...
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	4MB device audio buffers work best...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set up the output audio embedders...
	if (mDevice.features().GetNumAudioSystems() > 1)
	{
		//	Some devices, like the Kona1, have 2 FrameStores but only 1 SDI output,
		//	which makes mConfig.fOutputChannel == NTV2_CHANNEL2, but need SDIoutput to be NTV2_CHANNEL1...
		UWord	SDIoutput(mConfig.fOutputChannel);
		if (SDIoutput >= mDevice.features().GetNumVideoOutputs())
			SDIoutput = mDevice.features().GetNumVideoOutputs() - 1;
		mDevice.SetSDIOutputAudioSystem (NTV2Channel(SDIoutput), mAudioSystem);

		if (mDevice.features().GetNumHDMIVideoOutputs() > 0)
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
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Let my circular buffer know when it's time to quit...
	mFrameDataRing.SetAbortFlag (&mGlobalQuit);

	//  Determine video buffer size...
	const ULWord vidBuffSizeBytes (mFormatDesc.GetVideoWriteSize(ULWord(NTV2Buffer::DefaultPageSize())));

	//	Determine per-field max Anc buffer size...
	ULWord ancBuffSizeBytes (0);
	if (!mDevice.GetAncRegionOffsetFromBottom (ancBuffSizeBytes, NTV2_AncRgn_Field2))
		ancBuffSizeBytes = NTV2_ANCSIZE_MAX;

	//	Allocate and add each in-host NTV2FrameData to my mFrameDataRing...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());			//	Make a new NTV2FrameData...
		NTV2FrameData & frameData (mHostBuffers.back());	//	...and get a reference to it

		//	Allocate a page-aligned video buffer (if handling video)...
		if (!mConfig.fSuppressVideo)
			if (!frameData.fVideoBuffer.Allocate (vidBuffSizeBytes, /*pageAligned*/true))
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
			if (!frameData.fAudioBuffer.Allocate (NTV2_AUDIOSIZE_MAX, /*pageAligned*/true))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(NTV2_AUDIOSIZE_MAX,8) << "-byte audio buffer");
				return AJA_STATUS_MEMORY;
			}
		if (frameData.AudioBuffer())
			frameData.fAudioBuffer.Fill(ULWord(0));

		if (mConfig.WithAnc())
		{	//	Allocate page-aligned anc buffers...
			if (!frameData.fAncBuffer.Allocate (ancBuffSizeBytes, /*pageAligned*/true))
			{
				BURNFAIL("Failed to allocate " << xHEX0N(ancBuffSizeBytes,8) << "-byte anc buffer");
				return AJA_STATUS_MEMORY;
			}
			if (!::IsProgressivePicture(mVideoFormat))
				if (!frameData.fAncBuffer2.Allocate(ancBuffSizeBytes, /*pageAligned*/true))
				{
					BURNFAIL("Failed to allocate " << xHEX0N(ancBuffSizeBytes,8) << "-byte F2 anc buffer");
					return AJA_STATUS_MEMORY;
				}
		}
		if (frameData.AncBuffer())
			frameData.AncBuffer().Fill(ULWord(0));
		if (frameData.AncBuffer2())
			frameData.AncBuffer2().Fill(ULWord(0));

		//	Add this NTV2FrameData to the ring...
		mFrameDataRing.Add(&frameData);
	}	//	for each NTV2FrameData

	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2Burn::RouteInputSignal (void)
{
	const NTV2OutputCrosspointID	inputOutputXpt	(::GetInputSourceOutputXpt(mConfig.fInputSource));
	const NTV2InputCrosspointID		fbInputXpt		(::GetFrameBufferInputXptFromChannel(mConfig.fInputChannel));
	const bool						isRGB			(::IsRGBFormat(mConfig.fPixelFormat));

	if (isRGB)
	{
		//	If the frame buffer is configured for RGB pixel format, incoming YUV must be converted.
		//	This routes the video signal from the input through a color space converter before
		//	connecting to the RGB frame buffer...
		const NTV2InputCrosspointID		cscVideoInputXpt	(::GetCSCInputXptFromChannel (mConfig.fInputChannel));
		const NTV2OutputCrosspointID	cscOutputXpt		(::GetCSCOutputXptFromChannel (mConfig.fInputChannel, false/*isKey*/, true/*isRGB*/));

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
	const bool						isRGB			(::IsRGBFormat(mConfig.fPixelFormat));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

	if (isRGB)
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
		const NTV2Channel	endNum			(NTV2Channel(mDevice.features().GetNumVideoChannels()));
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
			if (mDevice.features().HasBiDirectionalSDI())
				mDevice.SetSDITransmitEnable (chan, true);
			if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, mDevice.features().CanDoDualLink()), outputWidgetID, mDeviceID))
				if (mDevice.features().CanDoWidget(outputWidgetID))
				{
					mDevice.Connect (::GetSDIOutputInputXpt (chan), outputXpt);
					mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan));
					mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan, true));
				}
		}	//	for each output spigot

		//	If HDMI and/or analog video outputs are available, route them, too...
		if (mDevice.features().GetNumHDMIVideoOutputs() > 0)
			mDevice.Connect (NTV2_XptHDMIOutQ1Input, outputXpt);	//	Route the output signal to the HDMI output
		if (mDevice.features().CanDoWidget(NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, outputXpt);	//	Route the output signal to the Analog output
		if (mDevice.features().CanDoWidget(NTV2_WgtSDIMonOut1))
			mDevice.Connect (::GetSDIOutputInputXpt (NTV2_CHANNEL5), outputXpt);	//	Route the output signal to the SDI monitor output
	}	//	if not multiChannel
	PLDBG(mTCOutputs.size() << " timecode destination(s):  " << mTCOutputs);

}	//	RouteOutputSignal


AJAStatus NTV2Burn::Run (void)
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
	NTV2Burn * pApp (reinterpret_cast<NTV2Burn*>(pContext));
	pApp->PlayFrames();

}	//	PlayThreadStatic


void NTV2Burn::PlayFrames (void)
{
	const ULWord			acOptions (AUTOCIRCULATE_WITH_RP188 | (mConfig.WithAnc() ? AUTOCIRCULATE_WITH_ANC : 0));
	ULWord					goodXfers(0), badXfers(0), starves(0), noRoomWaits(0);
	AUTOCIRCULATE_TRANSFER	outputXferInfo;
	AUTOCIRCULATE_STATUS	outputStatus;

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel, 4);	//	Let it stop
	BURNNOTE("Thread started");

	//	Initialize AutoCirculate...
	if (!mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel, mConfig.fOutputFrames.count(), mAudioSystem, acOptions,
											1 /*numChannels*/,  mConfig.fOutputFrames.firstFrame(),  mConfig.fOutputFrames.lastFrame()))
		{BURNFAIL("AutoCirculateInitForOutput failed");  mGlobalQuit = true;}
	else if (!mConfig.WithVideo())
	{	//	Video suppressed --
		//	Clear device frame buffers being AutoCirculated (prevent garbage output frames)
		NTV2Buffer tmpFrame (mFormatDesc.GetVideoWriteSize());
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
				outputXferInfo.SetAncBuffers (pFrameData->AncBuffer(), pFrameData->AncBufferSize(), pFrameData->AncBuffer2(), pFrameData->AncBuffer2Size());

			//	Tell AutoCirculate to embed this frame's timecode(s) into the SDI output(s)...
			outputXferInfo.SetOutputTimeCodes(pFrameData->fTimecodes);
			PLDBG(pFrameData->fTimecodes);

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
	BURNNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
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
	if (!mDevice.AutoCirculateInitForInput (mConfig.fInputChannel,			//	channel
											mConfig.fInputFrames.count(),	//	numFrames (zero if specifying range)
											mAudioSystem,					//	audio system
											acOptions,						//	flags
											1,								//	frameStores to gang
											mConfig.fInputFrames.firstFrame(), mConfig.fInputFrames.lastFrame()))
		{BURNFAIL("AutoCirculateInitForInput failed");  mGlobalQuit = true;}
	else
		//	Start AutoCirculate running...
		mDevice.AutoCirculateStart (mConfig.fInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS acStatus;
		mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, acStatus);

		if (mDevice.features().HasSDIRelays())
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

			//	Transfer the frame from the device into our host buffers...
			if (mDevice.AutoCirculateTransfer (mConfig.fInputChannel, inputXferInfo)) goodXfers++;
			else badXfers++;

			//	Remember the amount, in bytes, of captured audio & anc data...
			pFrameData->fNumAudioBytes	= pFrameData->AudioBuffer()	? inputXferInfo.GetCapturedAudioByteCount()			: 0;
			pFrameData->fNumAncBytes	= pFrameData->AncBuffer()	? inputXferInfo.GetCapturedAncByteCount(false/*F1*/): 0;
			pFrameData->fNumAnc2Bytes	= pFrameData->AncBuffer2()	? inputXferInfo.GetCapturedAncByteCount(true/*F2*/)	: 0;
			if (pFrameData->AncBuffer())
			{	//	Zero F1 anc buffer memory past last byte written by anc extractor
				NTV2Buffer stale (pFrameData->fAncBuffer.GetHostAddress(pFrameData->fNumAncBytes),
									pFrameData->fAncBuffer.GetByteCount() - pFrameData->fNumAncBytes);
				stale.Fill(uint8_t(0));
			}
			if (pFrameData->AncBuffer2())
			{	//	Zero F2 anc buffer memory past last byte written by anc extractor
				NTV2Buffer stale (pFrameData->AncBuffer2().GetHostAddress(pFrameData->fNumAnc2Bytes),
									pFrameData->AncBuffer2().GetByteCount() - pFrameData->fNumAnc2Bytes);
				stale.Fill(uint8_t(0));
			}

			if (pFrameData->AncBuffer())
				AJAAncList::StripNativeInserterGUMPPackets (pFrameData->AncBuffer(), pFrameData->AncBuffer());
			if (pFrameData->AncBuffer2())
				AJAAncList::StripNativeInserterGUMPPackets (pFrameData->AncBuffer2(), pFrameData->AncBuffer2());

			//	Determine which timecode value should be burned in to the video frame
			string timeCodeString;
			NTV2_RP188	timecodeValue;
			if (!NTV2_IS_ANALOG_TIMECODE_INDEX(mConfig.fTimecodeSource)  &&  InputSignalHasTimecode())
			{
				//	Use the embedded input time code...
				mDevice.GetRP188Data (mConfig.fInputChannel, timecodeValue);
				CRP188	inputRP188Info	(timecodeValue);
				inputRP188Info.GetRP188Str(timeCodeString);
			}
			else if (NTV2_IS_ANALOG_TIMECODE_INDEX(mConfig.fTimecodeSource)  &&  AnalogLTCInputHasTimecode())
			{
				//	Use the analog input time code...
				mDevice.ReadAnalogLTCInput (mConfig.fTimecodeSource == NTV2_TCINDEX_LTC1 ? 0 : 1, timecodeValue);
				CRP188	analogRP188Info	(timecodeValue);
				analogRP188Info.GetRP188Str(timeCodeString);
			}
			else
			{
				//	Invent a timecode (based on the number of frames procesed)...
				const	NTV2FrameRate	ntv2FrameRate	(GetNTV2FrameRateFromVideoFormat (mVideoFormat));
				const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
				const	CRP188			frameRP188Info	(inputXferInfo.GetTransferStatus().GetProcessedFrameCount(), 0, 0, 10, tcFormat);

				frameRP188Info.GetRP188Reg(timecodeValue);
				frameRP188Info.GetRP188Str(timeCodeString);
			}

			//	Get a timecode to use for burn-in...
			NTV2_RP188	thisFrameTC;
			inputXferInfo.GetInputTimeCodes (pFrameData->fTimecodes, mConfig.fInputChannel);
			if (pFrameData->HasValidTimecode(mConfig.fTimecodeSource))
			{
				thisFrameTC = pFrameData->fTimecodes[mConfig.fTimecodeSource];
			}
			else
			{	//	Invent a timecode (based on frame count)...
				const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
				const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
				const	CRP188			inventedTC		(inputXferInfo.GetTransferStatus().GetProcessedFrameCount(), 0, 0, 10, tcFormat);
				inventedTC.GetRP188Reg(thisFrameTC);
			}

			//	While this NTV2FrameData's buffers are locked, "burn" timecode into the raster...
			mTCBurner.BurnTimeCode (pFrameData->VideoBuffer(), timeCodeString.c_str(), yPercent.Next());

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
	BURNNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
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
	mDevice.driverInterface().ReadRegister(kRegLTCStatusControl, result, regMask);
	return result;

}	//	AnalogLTCInputHasTimecode
