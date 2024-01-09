/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2llburn.cpp
	@brief		Implementation of NTV2LLBurn demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2llburn.h"
#include "ntv2endian.h"
#include "ntv2formatdescriptor.h"
#include "ajabase/common/types.h"
#include "ajaanc/includes/ancillarylist.h"
#include <iostream>

using namespace std;

//	Override NTV2_ANCSIZE_MAX for testing new HANC insertion feature
#if defined(NTV2_ANCSIZE_MAX)
	#undef NTV2_ANCSIZE_MAX
#endif
#define NTV2_ANCSIZE_MAX (256 * 1024)	//	Super Size Me!	0x40000


const uint32_t	kAppSignature	(NTV2_FOURCC('L','l','b','u'));

#define AsULWordPtr(_p_)		reinterpret_cast<ULWord*>(_p_)
#define AsCU8Ptr(_p_)			reinterpret_cast<const uint8_t*>(_p_)


NTV2LLBurn::NTV2LLBurn (const BurnConfig & inConfig)
	:	mConfig					(inConfig),
		mRunThread				(AJAThread()),
		mDeviceID				(DEVICE_ID_NOTFOUND),
		mVideoFormat			(NTV2_FORMAT_UNKNOWN),
		mSavedTaskMode			(NTV2_DISABLE_TASKS),
		mOutputDest				(NTV2_OUTPUTDESTINATION_INVALID),
		mAudioSystem			(NTV2_AUDIOSYSTEM_1),
		mGlobalQuit				(false),
		mAudioInLastAddress		(0),
		mAudioOutLastAddress	(0),
		mFramesProcessed		(0),
		mFramesDropped			(0)
{
}	//	constructor


NTV2LLBurn::~NTV2LLBurn ()
{
	//	Stop my capture and playout threads, then destroy them...
	Quit();

	//	Unsubscribe from input vertical event...
	mDevice.UnsubscribeInputVerticalEvent (mConfig.fInputChannel);

	if (!mConfig.fDoMultiFormat)
	{
		mDevice.SetEveryFrameServices (mSavedTaskMode);										//	Restore prior service level
		mDevice.ReleaseStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));	//	Release the device
	}

	//	Don't leave the audio system active after we exit
	mDevice.StopAudioInput (mAudioSystem);
	mDevice.StopAudioOutput (mAudioSystem);

}	//	destructor


void NTV2LLBurn::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the thread to go inactive...
	mGlobalQuit = true;

	while (mRunThread.Active())
		AJATime::Sleep(10);

}	//	Quit


AJAStatus NTV2LLBurn::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID ();		//	Keep the device ID handy since it will be used frequently

	//	Burn requires device capable of capturing and playing video...
	if (!(::NTV2DeviceCanDoCapture(mDeviceID)  &&  ::NTV2DeviceCanDoPlayback(mDeviceID)))
		{cerr << "## ERROR:  Device cannot both capture & play video" << endl;	return AJA_STATUS_BAD_PARAM; }

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetEveryFrameServices (mSavedTaskMode);				//	Save the current device state
	mDevice.GetStreamingApplication (appSignature, appPID);		//	Who currently "owns" the device?
	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Set the OEM service level
		mDevice.ClearRouting ();								//	Clear the current device routing (since I "own" the device)
	}
	else
		mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Force OEM tasks

	//	Configure the SDI relays if present
	if (::NTV2DeviceHasSDIRelays (mDeviceID))
	{
		//	Note that if the board's jumpers are not set in the position
		//	to enable the watchdog timer, these calls will have no effect.
		mDevice.SetSDIWatchdogEnable(true, 0);	//	SDI 1/2
		mDevice.SetSDIWatchdogEnable(true, 1);	//	SDI 3/4

		//	Set timeout delay to 2 seconds expressed in multiples of 8 ns
		//	and take the relays out of bypass
		mDevice.SetSDIWatchdogTimeout (2 * 125000000);
		mDevice.KickSDIWatchdog ();

		//	Give the mechanical relays some time to switch
		AJATime::Sleep (500);
	}

	if (mConfig.WithHanc())
	{
		//	Transfer audio using HANC extractor/inserter
		mConfig.fWithAnc = true;
		mConfig.fSuppressAudio = true;
	}

	if (mConfig.WithAnc() && !::NTV2DeviceCanDoCustomAnc(mDeviceID))
		{cerr << "## WARNING: Device doesn't support custom Anc, '-a -h' option ignored" << endl;  mConfig.fWithAnc = false; mConfig.fWithHanc = false;}

	//	Set up the video and audio...
	status = SetupVideo();
	if (AJA_FAILURE (status))
		return status;

	status = SetupAudio();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the circular buffers...
	status = SetupHostBuffers();
	if (AJA_FAILURE (status))
		return status;

	if (NTV2_IS_ANALOG_TIMECODE_INDEX(mConfig.fTimecodeSource))
		mDevice.SetLTCInputEnable(true);	//	Enable analog LTC input (some LTC inputs are shared with reference input)

	//	Lastly, prepare my AJATimeCodeBurn instance...
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mConfig.fPixelFormat), mFormatDesc.numPixels, mFormatDesc.numLines);

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2LLBurn::SetupVideo (void)
{
	const uint16_t	numFrameStores	(::NTV2DeviceGetNumFrameStores (mDeviceID));
	const uint16_t	numSDIOutputs	(::NTV2DeviceGetNumVideoOutputs(mDeviceID));

	//	Can the device support the desired input source?
	if (!::NTV2DeviceCanDoInputSource (mDeviceID, mConfig.fInputSource))
		{cerr << "## ERROR:  This device cannot receive input from the specified source" << endl;	return AJA_STATUS_BAD_PARAM;}

	//	Pick an input NTV2Channel from the input source, and enable its frame buffer...
	mConfig.fInputChannel = NTV2_INPUT_SOURCE_IS_ANALOG(mConfig.fInputSource) ? NTV2_CHANNEL1 : ::NTV2InputSourceToChannel(mConfig.fInputSource);
	mDevice.EnableChannel (mConfig.fInputChannel);		//	Enable the input frame buffer

	//	Pick an appropriate output NTV2Channel, and enable its frame buffer...
	switch (mConfig.fInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		mConfig.fOutputChannel = numSDIOutputs < 3 || numFrameStores == 2 || numFrameStores > 4 ? NTV2_CHANNEL2 : NTV2_CHANNEL3;	break;

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
//		default:
		case NTV2_INPUTSOURCE_INVALID:	cerr << "## ERROR:  Bad input source" << endl;  return AJA_STATUS_BAD_PARAM;
	}

	bool	isTransmit	(false);
	if (::NTV2DeviceHasBiDirectionalSDI (mDevice.GetDeviceID ())			//	If device has bidirectional SDI connectors...
		&& NTV2_INPUT_SOURCE_IS_SDI (mConfig.fInputSource)							//	...and desired input source is SDI...
			&& mDevice.GetSDITransmitEnable (mConfig.fInputChannel, isTransmit)		//	...and GetSDITransmitEnable succeeds...
				&& isTransmit)												//	...and input is set to "transmit"...
	{
		mDevice.SetSDITransmitEnable (mConfig.fInputChannel, false);				//	...then disable transmit mode...
		mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel, 20);			//	...and give the device a dozen frames or so to lock to the input signal
	}	//	if input SDI connector needs to switch from transmit mode

	if (mConfig.WithAnc() && !NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource))
		cerr << "## WARNING: Non-SDI input source, no Anc capture possible" << endl;

	//	Determine the input video signal format, and set the device's reference source to that input...
	mVideoFormat = mDevice.GetInputVideoFormat (mConfig.fInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr << "## ERROR:  The specified input has no signal, or the device cannot handle the signal format" << endl;
		return AJA_STATUS_NOINPUT;	//	Sorry, can't handle this format
	}

	mDevice.SetReference (::NTV2InputSourceToReferenceSource (mConfig.fInputSource));
	mAudioSystem	= ::NTV2InputSourceToAudioSystem (mConfig.fInputSource);
	mOutputDest		= ::NTV2ChannelToOutputDestination (mConfig.fOutputChannel);

	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID)					//	If device has bidirectional SDI connectors...
		&& NTV2_OUTPUT_DEST_IS_SDI (mOutputDest))			//	...and output destination is SDI...
			mDevice.SetSDITransmitEnable (mConfig.fOutputChannel, true);	//	...then enable transmit mode

	if (mConfig.WithAnc() && !NTV2_OUTPUT_DEST_IS_SDI(mOutputDest))
		cerr << "## WARNING: Non-SDI output destination, no Anc playout possible" << endl;

	mDevice.EnableChannel (mConfig.fInputChannel);		//	Enable the input frame buffer
	mDevice.EnableChannel (mConfig.fOutputChannel);		//	Enable the output frame buffer

	if(::NTV2DeviceCanDoMultiFormat (mDeviceID) && mConfig.fDoMultiFormat)
		mDevice.SetMultiFormatMode (true);
	else if(::NTV2DeviceCanDoMultiFormat (mDeviceID))
		mDevice.SetMultiFormatMode (false);

	//	Set the input channel format to the detected input format...
	mDevice.SetVideoFormat (mVideoFormat, false, false, ::NTV2DeviceCanDoMultiFormat(mDeviceID) ? mConfig.fInputChannel : NTV2_CHANNEL1);
	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))									//	If device supports multiple formats per-channel...
		mDevice.SetVideoFormat (mVideoFormat, false, false, mConfig.fOutputChannel);		//	...then also set the output channel format to the detected input format

	//	Set the frame buffer pixel format for both channels on the device, assuming it
	//	supports that pixel format . . . otherwise default to 8-bit YCbCr...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.fPixelFormat))
		mConfig.fPixelFormat = NTV2_FBF_8BIT_YCBCR;

	//	Set the pixel format for both device frame buffers...
	mDevice.SetFrameBufferFormat (mConfig.fInputChannel, mConfig.fPixelFormat);
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);

	//	Enable and subscribe to the interrupts for the channel to be used...
	mDevice.EnableInputInterrupt (mConfig.fInputChannel);
	mDevice.SubscribeInputVerticalEvent (mConfig.fInputChannel);

	//	Enable and subscribe to the output interrupts (though it's enabled by default)...
	mDevice.EnableOutputInterrupt (mConfig.fOutputChannel);
	mDevice.SubscribeOutputVerticalEvent (mConfig.fOutputChannel);

	//	Set the Frame Store modes
	mDevice.SetMode	(mConfig.fInputChannel,  NTV2_MODE_CAPTURE);
	mDevice.SetMode (mConfig.fOutputChannel, NTV2_MODE_DISPLAY);

	if (!NTV2_IS_SD_VIDEO_FORMAT(mVideoFormat))
	{
		//	Enable VANC for non-SD formats, to pass thru captions, etc.
		mDevice.SetEnableVANCData (false);
		if (::Is8BitFrameBufferFormat (mConfig.fPixelFormat))
		{
			//	8-bit FBFs require VANC bit shift...
			mDevice.SetVANCShiftMode (mConfig.fInputChannel, NTV2_VANCDATA_8BITSHIFT_ENABLE);
			mDevice.SetVANCShiftMode (mConfig.fOutputChannel, NTV2_VANCDATA_8BITSHIFT_ENABLE);
		}
	}	//	if not SD video

	//	Set the format descriptor
	mFormatDesc = NTV2FormatDescriptor (mVideoFormat, mConfig.fPixelFormat);

	//	Set up the device signal routing, and both playout and capture AutoCirculate...
	RouteInputSignal();
	RouteOutputSignal();

	//	Be sure the RP188 mode for the SDI input (expressed as an NTV2Channel),
	//	is set to NTV2_RP188_INPUT...
	bool	isBypassEnabled	(false);
	mDevice.IsRP188BypassEnabled (mConfig.fInputChannel, isBypassEnabled);
	if (isBypassEnabled)
		mDevice.DisableRP188Bypass (mConfig.fInputChannel);
	mDevice.SetRP188Mode (mConfig.fInputChannel, NTV2_RP188_INPUT);
	mDevice.SetRP188SourceFilter (mConfig.fInputChannel, 0);	//	0=LTC 1=VITC1 2=VITC2

	//	Make sure the RP188 mode for all SDI outputs is NTV2_RP188_OUTPUT, and that their RP188
	//	registers are not bypassed (i.e. timecode isn't sourced from an SDI input, as for E-E mode)...
	NTV2_ASSERT(!mRP188Outputs.empty());
	for (NTV2ChannelSetConstIter iter(mRP188Outputs.begin());  iter != mRP188Outputs.end();  ++iter)
	{
		mDevice.SetRP188Mode (*iter, NTV2_RP188_OUTPUT);
		mDevice.DisableRP188Bypass (*iter);
	}

	//	Tell the hardware which buffers to use until the main worker thread runs
	mDevice.SetInputFrame (mConfig.fInputChannel,  0);
	mDevice.SetOutputFrame(mConfig.fOutputChannel, 2);

	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2LLBurn::SetupAudio (void)
{
	//	Have the audio subsystem capture audio from the designated input source...
	mDevice.SetAudioSystemInputSource (mAudioSystem, ::NTV2InputSourceToAudioSource (mConfig.fInputSource), ::NTV2InputSourceToEmbeddedAudioInput (mConfig.fInputSource));

	//	It's best to use all available audio channels...
	mDevice.SetNumberAudioChannels (::NTV2DeviceGetMaxAudioChannels (mDeviceID), mAudioSystem);

	//	Assume 48kHz PCM...
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	4MB device audio buffers work best...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set up the output audio embedders...
	if (::NTV2DeviceGetNumAudioSystems (mDeviceID) > 1)
	{
		//	Some devices, like the Kona1, have 2 FrameStores but only 1 SDI output,
		//	which makes mConfig.fOutputChannel == NTV2_CHANNEL2, but need SDIoutput to be NTV2_CHANNEL1...
		UWord	SDIoutput(mConfig.fOutputChannel);
		if (SDIoutput >= ::NTV2DeviceGetNumVideoOutputs(mDeviceID))
			SDIoutput = ::NTV2DeviceGetNumVideoOutputs(mDeviceID) - 1;
		mDevice.SetSDIOutputAudioSystem (NTV2Channel(SDIoutput), mAudioSystem);
	}

	//
	//	Loopback mode plays whatever audio appears in the input signal when it's
	//	connected directly to an output (i.e., "end-to-end" mode). If loopback is
	//	left enabled, the video will lag the audio as video frames get briefly delayed
	//	in our ring buffer. Audio, therefore, needs to come out of the (buffered) frame
	//	data being played, so loopback must be turned off...
	//
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	//	Reset both the input and output sides of the audio system so that the buffer
	//	pointers are reset to zero and inhibited from advancing.
	mDevice.StopAudioOutput	(mAudioSystem);
	mDevice.StopAudioInput	(mAudioSystem);

	//	Ensure that the audio system will capture samples when the reset is removed
	mDevice.SetAudioCaptureEnable (mAudioSystem, true);

	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


AJAStatus NTV2LLBurn::SetupHostBuffers (void)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Allocate and add each in-host buffer to my member variables.
	//	DMA performance can be accelerated slightly by using page-aligned video buffers...
	mpHostVideoBuffer.Allocate(mFormatDesc.GetVideoWriteSize(ULWord(NTV2Buffer::DefaultPageSize())), true);
	mpHostAudioBuffer.Allocate(NTV2_AUDIOSIZE_MAX, /*page-aligned*/true);
	mpHostF1AncBuffer.Allocate(mConfig.WithAnc() ? NTV2_ANCSIZE_MAX : 0, /*pageAligned*/true);
	mpHostF2AncBuffer = NTV2Buffer(mConfig.WithAnc() ? NTV2_ANCSIZE_MAX : 0);

	if (!mpHostVideoBuffer || !mpHostAudioBuffer  ||  (mConfig.WithAnc() && !mpHostF1AncBuffer)  ||  (mConfig.WithAnc() && !mpHostF2AncBuffer))
	{
		cerr << "## ERROR:  Unable to allocate host buffer(s)" << endl;
		return AJA_STATUS_MEMORY;
	}

	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2LLBurn::RouteInputSignal (void)
{
	const NTV2OutputCrosspointID	inputOutputXpt	(::GetInputSourceOutputXpt (mConfig.fInputSource));
	const NTV2InputCrosspointID		fbInputXpt		(::GetFrameBufferInputXptFromChannel (mConfig.fInputChannel));

	if (::IsRGBFormat (mConfig.fPixelFormat))
	{
		//	If the frame buffer is configured for RGB pixel format, incoming YUV must be converted.
		//	This routes the video signal from the input through a color space converter before
		//	connecting to the RGB frame buffer...
		const NTV2InputCrosspointID		cscVideoInputXpt	(::GetCSCInputXptFromChannel (mConfig.fInputChannel));
		const NTV2OutputCrosspointID	cscOutputXpt		(::GetCSCOutputXptFromChannel (mConfig.fInputChannel, false, true));	//	false=video, true=RGB

		mDevice.Connect (cscVideoInputXpt, inputOutputXpt);	//	Connect the CSC's video input to the input spigot's output
		mDevice.Connect (fbInputXpt, cscOutputXpt);			//	Connect the frame store's input to the CSC's output
	}
	else
		mDevice.Connect (fbInputXpt, inputOutputXpt);		//	Route the YCbCr signal directly from the input to the frame buffer's input

}	//	RouteInputSignal


void NTV2LLBurn::RouteOutputSignal (void)
{
	const NTV2InputCrosspointID		outputInputXpt	(::GetOutputDestInputXpt (mOutputDest));
	const NTV2OutputCrosspointID	fbOutputXpt		(::GetFrameBufferOutputXptFromChannel (mConfig.fOutputChannel, ::IsRGBFormat (mConfig.fPixelFormat)));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

	mRP188Outputs.clear();
	mRP188Outputs.insert(mConfig.fOutputChannel);
	if (::IsRGBFormat (mConfig.fPixelFormat))
	{
		const NTV2OutputCrosspointID	cscVidOutputXpt	(::GetCSCOutputXptFromChannel (mConfig.fOutputChannel));	//	Use CSC's YUV video output
		const NTV2InputCrosspointID		cscVidInputXpt	(::GetCSCInputXptFromChannel (mConfig.fOutputChannel));

		mDevice.Connect (cscVidInputXpt, fbOutputXpt);		//	Connect the CSC's video input to the frame store's output
		mDevice.Connect (outputInputXpt, cscVidOutputXpt);	//	Connect the SDI output's input to the CSC's video output
		outputXpt = cscVidOutputXpt;
	}
	else
		mDevice.Connect (outputInputXpt, outputXpt);

	if (!mConfig.fDoMultiFormat)
	{
		//	Route all SDI outputs to the outputXpt...
		const NTV2Channel	startNum		(NTV2_CHANNEL1);
		const NTV2Channel	endNum			(NTV2Channel(::NTV2DeviceGetNumVideoChannels(mDeviceID)));
		NTV2WidgetID		outputWidgetID	(NTV2_WIDGET_INVALID);

		for (NTV2Channel chan(startNum);  chan < endNum;  chan = NTV2Channel(chan+1))
		{
			mDevice.SetRP188SourceFilter (chan, 0);	//	Set all SDI spigots to capture embedded LTC (0==LTC 1==VITC1 2==VITC2)
			if (chan == mConfig.fInputChannel  ||  chan == mConfig.fOutputChannel)
				continue;	//	Skip the input & output channel, already routed
			mRP188Outputs.insert(chan);	//	Add this SDI spigot to those we'll push timecode into
			if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
				mDevice.SetSDITransmitEnable (chan, true);
			if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, ::NTV2DeviceCanDoDualLink(mDeviceID)), outputWidgetID))
				if (::NTV2DeviceCanDoWidget (mDeviceID, outputWidgetID))
					mDevice.Connect (::GetSDIOutputInputXpt(chan), outputXpt);
		}	//	for each output spigot

		//	If HDMI and/or analog video outputs are available, route them, too...
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1))
			mDevice.Connect (NTV2_XptHDMIOutInput, outputXpt);			//	Route the output signal to the HDMI output
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v2))
			mDevice.Connect (NTV2_XptHDMIOutQ1Input, outputXpt);		//	Route the output signal to the HDMI output
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, outputXpt);		//	Route the output signal to the Analog output
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIMonOut1))
			mDevice.Connect (::GetSDIOutputInputXpt (NTV2_CHANNEL5), outputXpt);	//	Route the output signal to the SDI monitor output
	}

}	//	RouteOutputSignal


AJAStatus NTV2LLBurn::Run ()
{
	//	Start the main worker thread...
	StartRunThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run


//////////////////////////////////////////////

//	This is where we will start the worker thread
void NTV2LLBurn::StartRunThread (void)
{
	//	Create and start the worker thread...
	mRunThread.Attach(RunThreadStatic, this);
	mRunThread.SetPriority(AJA_ThreadPriority_High);
	mRunThread.Start();

}	//	StartRunThread


//	The worker thread function
void NTV2LLBurn::RunThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	//	Grab the NTV2LLBurn instance pointer from the pContext parameter,
	//	then call its ProcessFrames method...
	NTV2LLBurn *	pApp	(reinterpret_cast <NTV2LLBurn *> (pContext));
	pApp->ProcessFrames ();

}	//	RunThreadStatic


static const bool	CLEAR_HOST_ANC_BUFFER_BEFORE_READ			(false);
static const bool	PRINT_ANC_PACKETS_AFTER_READ				(false);
static const bool	CLEAR_DEVICE_ANC_BUFFER_AFTER_READ			(false);
static const bool	REPLACE_OUTGOING_ANC_WITH_CUSTOM_PACKETS	(false);


void NTV2LLBurn::ProcessFrames (void)
{
	const bool	doAncInput				(mConfig.WithAnc() && NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource));
	const bool	doAncOutput				(mConfig.WithAnc() && NTV2_OUTPUT_DEST_IS_SDI(mOutputDest));
	const UWord	sdiInput				(UWord(::GetIndexForNTV2InputSource(mConfig.fInputSource)));
	const UWord	sdiOutput				(UWord(::NTV2OutputDestinationToChannel(mOutputDest)));
	const bool	isInterlace				(!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mVideoFormat));
	uint32_t	currentInFrame			(0);	//	Will ping-pong between 0 and 1
	uint32_t	currentOutFrame			(2);	//	Will ping-pong between 2 and 3
	uint32_t	currentAudioInAddress	(0);
	uint32_t	audioReadOffset			(0);
	uint32_t	audioInWrapAddress		(0);
	uint32_t	audioOutWrapAddress		(0);
	uint32_t	audioBytesCaptured		(0);
	uint32_t	ancBytesCapturedF1		(0);
	uint32_t	ancBytesCapturedF2		(0);
	bool		audioIsReset			(true);
	NTV2DIDSet	savedDIDs;
	string		timeCodeString;

	const NTV2Standard			videoStandard		(::GetNTV2StandardFromVideoFormat(mVideoFormat));
	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(videoStandard));
	Bouncer<UWord>				yPercent			(85/*upperLimit*/, 1/*lowerLimit*/, 1/*startValue*/);	//	Vertically "bounces" timecode in raster
	const AJAAncDataLoc			F1AncDataLoc		(AJAAncDataLink_A, AJAAncDataChannel_Y, AJAAncDataSpace_VANC, 10, 0, AJAAncDataStream_1);
	NTV2Buffer	zeroesBuffer(mpHostF1AncBuffer.GetByteCount());
	zeroesBuffer.Fill(ULWord64(0));
	BURNNOTE("Thread started");

	//	Save current Anc buffer capacity (to restore it later), then max it out...
	ULWord savedF1ByteCapacity(0), savedF2ByteCapacity(0), offset(0);
	mDevice.GetAncRegionOffsetAndSize (offset, savedF1ByteCapacity, NTV2_AncRgn_Field1);
	mDevice.GetAncRegionOffsetAndSize (offset, savedF2ByteCapacity, NTV2_AncRgn_Field2);
	mDevice.AncSetFrameBufferSize (NTV2_ANCSIZE_MAX, NTV2_ANCSIZE_MAX);

	if (doAncInput)
	{
		// Configure extractor
		mDevice.AncExtractInit(sdiInput, mConfig.fInputChannel);

		if (mConfig.WithHanc())
		{
			mDevice.AncExtractGetFilterDIDs(sdiInput, savedDIDs);	//	Save current ANC filter (so it can be restored later)

			//	Configure ANC extractor not to filter audio
			NTV2DIDSet dids;
//			dids.insert(0xe0);	// HD audio group 4 control
//			dids.insert(0xe1);	// HD audio group 3 control
//			dids.insert(0xe2);	// HD audio group 2 control
//			dids.insert(0xe3);	// HD audio group 1 control
//			dids.insert(0xe4);	// HD audio group 4 data
//			dids.insert(0xe5);	// HD audio group 3 data
//			dids.insert(0xe6);	// HD audio group 2 data
//			dids.insert(0xe7);	// HD audio group 1 data
			dids.insert(0xa0);	// 3G audio group 8 control
			dids.insert(0xa1);	// 3G audio group 7 control
			dids.insert(0xa2);	// 3G audio group 6 control
			dids.insert(0xa3);	// 3G audio group 5 control
			dids.insert(0xa4);	// 3G audio group 8 data
			dids.insert(0xa5);	// 3G audio group 7 data
			dids.insert(0xa6);	// 3G audio group 6 data
			dids.insert(0xa7);	// 3G audio group 5 data
			dids.insert(0x41);	// Filter vpid (inserted by sdi output)
			dids.insert(0x60);	// Filter timecode (inserted by sdi output)
			mDevice.AncExtractSetFilterDIDs (sdiInput, dids);

			//	Enable HANC extraction
			mDevice.AncExtractSetComponents (sdiInput, true, true, true, true);
		}
		else
		{
			//	Disable HANC extraction
			mDevice.AncExtractSetComponents (sdiInput, true, true, false, false);
		}

		// Start extractor
		mDevice.AncExtractSetEnable (sdiInput, true);
	}

	if (doAncOutput)
	{
		// Configure inserter
		mDevice.AncInsertInit (sdiOutput, mConfig.fInputChannel);
		mDevice.AncInsertSetComponents (sdiOutput, true, true, mConfig.WithHanc(), mConfig.WithHanc());
		mDevice.AncInsertSetReadParams (sdiOutput, 0, 0, mConfig.fOutputChannel);
		mDevice.AncInsertSetField2ReadParams (sdiOutput, 0, 0, mConfig.fOutputChannel);
	}

	mDevice.SetAudioOutputEmbedderState (::NTV2OutputDestinationToChannel(mOutputDest), mConfig.WithAudio());

	mFramesProcessed = mFramesDropped = 0;	//	Start with a fresh frame count

	mDevice.GetAudioReadOffset	(audioReadOffset,		mAudioSystem);
	mDevice.GetAudioWrapAddress	(audioOutWrapAddress,	mAudioSystem);

	//	Wait to make sure the next two SDK calls will be made during the same frame...
	mDevice.WaitForInputFieldID (NTV2_FIELD0, mConfig.fInputChannel);
    mDevice.DMAWriteAnc (0, zeroesBuffer, zeroesBuffer);
    mDevice.DMAWriteAnc (1, zeroesBuffer, zeroesBuffer);
    mDevice.DMAWriteAnc (2, zeroesBuffer, zeroesBuffer);
    mDevice.DMAWriteAnc (3, zeroesBuffer, zeroesBuffer);

	//	Before the main loop starts, ping-pong the buffers so the hardware will use
	//	different buffers than the ones it was using while idling...
	currentInFrame	^= 1;
	currentOutFrame	^= 1;

	mDevice.SetInputFrame	(mConfig.fInputChannel,  currentInFrame);
	if (doAncInput)
		mDevice.AncExtractSetWriteParams (sdiInput, currentInFrame, mConfig.fInputChannel);
	if (doAncInput && isInterlace)
		mDevice.AncExtractSetField2WriteParams (sdiInput, currentInFrame, mConfig.fInputChannel);

	mDevice.SetOutputFrame	(mConfig.fOutputChannel, currentOutFrame);

	//	Wait until the hardware starts filling the new buffers, and then start audio
	//	capture as soon as possible to match the video...
	mDevice.WaitForInputFieldID (NTV2_FIELD0, mConfig.fInputChannel);
	mDevice.StartAudioInput	(mAudioSystem);

	mAudioInLastAddress		= audioReadOffset;
	audioInWrapAddress		= audioOutWrapAddress + audioReadOffset;
	mAudioOutLastAddress	= 0;

	currentInFrame	^= 1;
	currentOutFrame	^= 1;

	mDevice.SetInputFrame	(mConfig.fInputChannel,  currentInFrame);
	if (doAncInput)
		mDevice.AncExtractSetWriteParams (sdiInput, currentInFrame, mConfig.fInputChannel);
	if (doAncInput && isInterlace)
		mDevice.AncExtractSetField2WriteParams (sdiInput, currentInFrame, mConfig.fInputChannel);

	mDevice.SetOutputFrame	(mConfig.fOutputChannel, currentOutFrame);

	while (!mGlobalQuit)
	{
		//	Wait until the input has completed capturing a frame...
		mDevice.WaitForInputFieldID (NTV2_FIELD0, mConfig.fInputChannel);

		//	Flip sense of the buffers again to refer to the buffers that the hardware isn't using (i.e. the off-screen buffers)...
		currentInFrame	^= 1;
		currentOutFrame	^= 1;

		if (mConfig.WithAudio())
		{
			//	Read the audio position registers as close to the interrupt as possible...
			mDevice.ReadAudioLastIn (currentAudioInAddress, NTV2AudioSystem(mConfig.fInputChannel));
			currentAudioInAddress &= ~0x3UL;	//	Force DWORD alignment
			currentAudioInAddress += audioReadOffset;

			if (audioIsReset && mAudioOutLastAddress)
			{
				//	Now that the audio system has some samples to play, playback can be started...
				mDevice.StartAudioOutput (mAudioSystem);
				audioIsReset = false;
			}

			if (currentAudioInAddress < mAudioInLastAddress)
			{
				//	Audio address has wrapped around the end of the buffer.
				//	Do the calculations and transfer from the last address to the end of the buffer...
				audioBytesCaptured 	= audioInWrapAddress - mAudioInLastAddress;

				mDevice.DMAReadAudio (mAudioSystem, mpHostAudioBuffer, mAudioInLastAddress, audioBytesCaptured);

				//	Transfer the new samples from the start of the buffer to the current address...
				mDevice.DMAReadAudio (mAudioSystem, AsULWordPtr(mpHostAudioBuffer.GetHostAddress(audioBytesCaptured)),
										audioReadOffset, currentAudioInAddress - audioReadOffset);

				audioBytesCaptured += currentAudioInAddress - audioReadOffset;
			}
			else
			{
				audioBytesCaptured = currentAudioInAddress - mAudioInLastAddress;

				//	No wrap, so just perform a linear DMA from the buffer...
				mDevice.DMAReadAudio (mAudioSystem, mpHostAudioBuffer, mAudioInLastAddress, audioBytesCaptured);
			}

			mAudioInLastAddress = currentAudioInAddress;
		}	//	if mConfig.WithAudio()

		//	Transfer the new frame to system memory...
		mDevice.DMAReadFrame (currentInFrame, mpHostVideoBuffer, mpHostVideoBuffer.GetByteCount());

		if (doAncInput)
		{	//	Transfer received Anc data into my F1 & F2 buffers...
			AJAAncillaryList	capturedPackets;
			//	Read ANC bytes captured
			mDevice.AncExtractGetField1Size (sdiInput, ancBytesCapturedF1);
			if (isInterlace)
				mDevice.AncExtractGetField2Size (sdiInput, ancBytesCapturedF2);
			if (CLEAR_HOST_ANC_BUFFER_BEFORE_READ)
			{
				mpHostF1AncBuffer.Fill(0);
				if (isInterlace)
					mpHostF2AncBuffer.Fill(0);
			}
			//	Read ANC data
			mDevice.DMAReadAnc (currentInFrame, mpHostF1AncBuffer, mpHostF2AncBuffer);

			if (PRINT_ANC_PACKETS_AFTER_READ)
			{
				AJAAncillaryList::SetFromDeviceAncBuffers (mpHostF1AncBuffer, mpHostF2AncBuffer, capturedPackets);
				if (capturedPackets.CountAncillaryData())	capturedPackets.Print(cerr, false);		//	Dump packets
			}
			if (CLEAR_DEVICE_ANC_BUFFER_AFTER_READ)
				mDevice.DMAWriteAnc (currentInFrame, zeroesBuffer, zeroesBuffer);
		}

		//	Determine which timecode value should be burned in to the video frame
		NTV2_RP188	timecodeValue;
		if (!NTV2_IS_ANALOG_TIMECODE_INDEX(mConfig.fTimecodeSource)  &&  InputSignalHasTimecode())
		{
			//	Use the embedded input time code...
			mDevice.GetRP188Data (mConfig.fInputChannel, timecodeValue);
			CRP188	inputRP188Info	(timecodeValue);
			inputRP188Info.GetRP188Str(timeCodeString);
			//cerr << "SDI" << DEC(mConfig.fTimecodeSource) << ":" << timeCodeString << ":" << timecodeValue << endl;
		}
		else if (NTV2_IS_ANALOG_TIMECODE_INDEX(mConfig.fTimecodeSource)  &&  AnalogLTCInputHasTimecode())
		{
			//	Use the analog input time code...
			mDevice.ReadAnalogLTCInput (mConfig.fTimecodeSource == NTV2_TCINDEX_LTC1 ? 0 : 1, timecodeValue);
			CRP188	analogRP188Info	(timecodeValue);
			analogRP188Info.GetRP188Str(timeCodeString);
			//cerr << "Ana" << DEC(mConfig.fTimecodeSource) << ":" << timeCodeString << ":" << timecodeValue << endl;
		}
		else
		{
			//	Invent a timecode (based on the number of frames procesed)...
			const	NTV2FrameRate	ntv2FrameRate	(GetNTV2FrameRateFromVideoFormat (mVideoFormat));
			const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
			const	CRP188			frameRP188Info	(mFramesProcessed, tcFormat);

			frameRP188Info.GetRP188Reg(timecodeValue);
			frameRP188Info.GetRP188Str(timeCodeString);
			//cerr << "Inv:" << timeCodeString << ":" << timecodeValue << endl;
		}

		//	"Burn" the timecode into the host buffer while we have full access to it...
		mTCBurner.BurnTimeCode (reinterpret_cast <char *> (mpHostVideoBuffer.GetHostPointer()), timeCodeString.c_str(), yPercent.Next());

		if (mConfig.WithAudio())
		{
			//	Calculate where the next audio samples should go in the buffer, taking wraparound into account...
			if ((mAudioOutLastAddress + audioBytesCaptured) > audioOutWrapAddress)
			{
				//	The audio will wrap. Transfer enough bytes to fill the buffer to the end...
				mDevice.DMAWriteAudio (mAudioSystem, mpHostAudioBuffer, mAudioOutLastAddress, audioOutWrapAddress - mAudioOutLastAddress);

				//	Now transfer the remaining bytes to the front of the buffer...
				mDevice.DMAWriteAudio (mAudioSystem, AsULWordPtr(mpHostAudioBuffer.GetHostAddress(audioOutWrapAddress - mAudioOutLastAddress)),
									   0, audioBytesCaptured - (audioOutWrapAddress - mAudioOutLastAddress));

				mAudioOutLastAddress = audioBytesCaptured - (audioOutWrapAddress - mAudioOutLastAddress);
			}
			else
			{
				//	No wrap, so just do a linear DMA from the buffer...
				mDevice.DMAWriteAudio (mAudioSystem, mpHostAudioBuffer, mAudioOutLastAddress, audioBytesCaptured);

				mAudioOutLastAddress += audioBytesCaptured;
			}
		}	//	if mConfig.WithAudio()

		//	Send the updated frame back to the board for display...
		mDevice.DMAWriteFrame (currentOutFrame, mpHostVideoBuffer, mpHostVideoBuffer.GetByteCount());

		if (doAncOutput)
		{
			if (REPLACE_OUTGOING_ANC_WITH_CUSTOM_PACKETS)
			{
				AJAAncillaryData pkt;	AJAAncillaryList pkts;
				AJAAncDataLoc F2Loc(F1AncDataLoc);
				ULWord pktData(NTV2EndianSwap32(mFramesProcessed));
				pkt.SetDID(0xC0);  pkt.SetSID(0x00);  pkt.SetDataLocation(F1AncDataLoc);  pkt.SetDataCoding(AJAAncDataCoding_Digital);
				pkt.SetPayloadData(AsCU8Ptr(&pktData), 4);
				pkts.AddAncillaryData(pkt);
				if (isInterlace)
				{
					F2Loc.SetLineNumber(uint16_t(smpteLineNumInfo.GetFirstActiveLine(NTV2_FIELD1)
													+ ULWord(F1AncDataLoc.GetLineNumber())
													- smpteLineNumInfo.GetFirstActiveLine(NTV2_FIELD0)));
					pkt.SetDID(0xC1);  pkt.SetSID(0x01);  pkt.SetDataLocation(F2Loc);
					pktData = ULWord(pktData << 16) | ULWord(pktData >> 16);
					pkt.SetPayloadData(AsCU8Ptr(&pktData), 4);
					pkts.AddAncillaryData(pkt);
				}
				//pkts.Print(cerr, true); cerr << endl;
				pkts.GetTransmitData (mpHostF1AncBuffer, mpHostF2AncBuffer, !isInterlace, isInterlace ? smpteLineNumInfo.GetLastLine(NTV2_FIELD0)+1 : 0);
			}
			//	Write ANC data
            mDevice.DMAWriteAnc (currentOutFrame, mpHostF1AncBuffer, mpHostF2AncBuffer);
		}

		//	Write the output timecode (for all SDI output spigots)...
		for (NTV2ChannelSetConstIter iter(mRP188Outputs.begin());  iter != mRP188Outputs.end();  ++iter)
			mDevice.SetRP188Data (*iter, timecodeValue);

		//	Check for dropped frames by ensuring the hardware has not started to process
		//	the buffers that were just filled....
		uint32_t readBackIn;
		uint32_t readBackOut;
		mDevice.GetInputFrame	(mConfig.fInputChannel,		readBackIn);
		mDevice.GetOutputFrame	(mConfig.fOutputChannel,	readBackOut);

		if ((readBackIn == currentInFrame) || (readBackOut == currentOutFrame))
		{
			cerr	<< "## WARNING:  Drop detected:  current in " << currentInFrame << ", readback in " << readBackIn
					<< ", current out " << currentOutFrame << ", readback out " << readBackOut << endl;
			mFramesDropped++;
		}
		else
			mFramesProcessed++;

		//	Tell the hardware which buffers to start using at the beginning of the next frame...
		mDevice.SetInputFrame (mConfig.fInputChannel,  currentInFrame);
		if (doAncInput)
			mDevice.AncExtractSetWriteParams (sdiInput, currentInFrame, mConfig.fInputChannel);
		if (doAncInput && isInterlace)
			mDevice.AncExtractSetField2WriteParams (sdiInput, currentInFrame, mConfig.fInputChannel);

		mDevice.SetOutputFrame (mConfig.fOutputChannel, currentOutFrame);
		if (doAncOutput)
			mDevice.AncInsertSetReadParams (sdiOutput, currentOutFrame, ancBytesCapturedF1, mConfig.fOutputChannel);
		if (doAncOutput && isInterlace)
			mDevice.AncInsertSetField2ReadParams (sdiOutput, currentOutFrame, ancBytesCapturedF2, mConfig.fOutputChannel);

		//	Enable ANC insertion on first output frame
		if (mFramesProcessed == 1)
			mDevice.AncInsertSetEnable (sdiOutput, true);

	}	//	loop til quit signaled

	if (doAncInput)
		mDevice.AncExtractSetEnable (sdiInput, false);	//	Stop ANC extractor
	if (doAncOutput)
		mDevice.AncInsertSetEnable (sdiOutput, false);	//	Stop ANC inserter

	if (doAncInput  &&  mConfig.WithHanc())
		mDevice.AncExtractSetFilterDIDs (sdiInput, savedDIDs);	//	Restore prior ANC filtering

	//	Restore previous Anc buffer capacity...
	mDevice.AncSetFrameBufferSize (savedF1ByteCapacity, savedF2ByteCapacity);
	BURNNOTE("Thread completed, will exit");

}	//	ProcessFrames


//////////////////////////////////////////////


void NTV2LLBurn::GetStatus (ULWord & outFramesProcessed, ULWord & outFramesDropped)
{
	outFramesProcessed = mFramesProcessed;
	outFramesDropped = mFramesDropped;

}	//	GetACStatus


static ULWord GetRP188DBBRegNumForInput (const NTV2InputSource inInputSource)
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

}	//	GetRP188DBBRegNumForInput


bool NTV2LLBurn::InputSignalHasTimecode (void)
{
	bool			result		(false);
	const ULWord	regNum		(GetRP188DBBRegNumForInput(mConfig.fInputSource));
	ULWord			regValue	(0);

	//	Bit 16 of the RP188 DBB register will be set if there is timecode embedded in the input signal...
	if (regNum  &&  mDevice.ReadRegister(regNum, regValue)  &&  regValue & BIT(16))
		result = true;
	return result;

}	//	InputSignalHasTimecode


bool NTV2LLBurn::AnalogLTCInputHasTimecode (void)
{
	ULWord	regMask		(kRegMaskLTC1InPresent);
	ULWord	regValue	(0);
	switch (mConfig.fTimecodeSource)
	{
		case NTV2_TCINDEX_LTC1:										break;
		case NTV2_TCINDEX_LTC2:	regMask = kRegMaskLTC2InPresent;	break;
		default:				return false;
	}
	mDevice.ReadRegister (kRegLTCStatusControl, regValue, regMask);
	return regValue ? true : false;

}	//	AnalogLTCInputHasTimecode
