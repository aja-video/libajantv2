/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2fieldburn.cpp
	@brief		Implementation of NTV2FieldBurn demonstration class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2fieldburn.h"
#include "ntv2devicescanner.h"
#include "ajabase/common/types.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include <iostream>

using namespace std;

#define ToCharPtr(_p_)	reinterpret_cast<char*>(_p_)

static const uint32_t	kAppSignature	(NTV2_FOURCC('F','l','d','B'));


//////////////////////	IMPLEMENTATION


NTV2FieldBurn::NTV2FieldBurn (const BurnConfig & inConfig)
	:	mConfig				(inConfig),
		mPlayThread			(AJAThread()),
		mCaptureThread		(AJAThread()),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mOutputDest			(NTV2_OUTPUTDESTINATION_INVALID),
		mAudioSystem		(inConfig.WithAudio() ? NTV2_AUDIOSYSTEM_1 : NTV2_AUDIOSYSTEM_INVALID),
		mGlobalQuit			(false)
{
}	//	constructor


NTV2FieldBurn::~NTV2FieldBurn ()
{
	Quit();	//	Stop my capture and playout threads, then destroy them
	mDevice.UnsubscribeInputVerticalEvent(mConfig.fInputChannel);	//	Unsubscribe from input VBI event
}	//	destructor


void NTV2FieldBurn::Quit (void)
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
			mDevice.SetTaskMode(mSavedTaskMode);	//	Restore prior task mode
	}
}	//	Quit


AJAStatus NTV2FieldBurn::Init (void)
{
	AJAStatus status (AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID();	//	Keep the device ID handy since it will be used frequently
	if (!mDevice.features().CanDoCapture())
		{cerr << "## ERROR:  Device '" << mDeviceID << "' cannot capture" << endl;  return AJA_STATUS_FEATURE;}
	if (!mDevice.features().CanDoPlayback())
		{cerr << "## ERROR:  Device '" << mDeviceID << "' cannot playout" << endl;  return AJA_STATUS_FEATURE;}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetStreamingApplication (appSignature, appPID);	//	Who currently "owns" the device?
	mDevice.GetTaskMode(mSavedTaskMode);			//	Save the current device state
	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mDevice.ClearRouting();	//	Clear the current device routing (since I "own" the device)
	}
	mDevice.SetTaskMode(NTV2_OEM_TASKS);	//	Force OEM tasks

	//	Set up the video and audio...
	status = SetupVideo();
	if (AJA_FAILURE(status))
		return status;

	if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
		status = SetupAudio();
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
		cerr << mConfig << endl;
	#endif	//	not _DEBUG
	BURNINFO("Configuration: " << mConfig);
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2FieldBurn::SetupVideo (void)
{
	const UWord	numFrameStores	(mDevice.features().GetNumFrameStores());

	//	Does this device have the requested input source?
	if (!mDevice.features().CanDoInputSource(mConfig.fInputSource))
		{cerr << "## ERROR:  Device does not have the specified input source" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Pick an input NTV2Channel from the input source, and enable its frame buffer...
	mConfig.fInputChannel = NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource) ? ::NTV2InputSourceToChannel(mConfig.fInputSource) : NTV2_CHANNEL1;
	mDevice.EnableChannel(mConfig.fInputChannel);		//	Enable the input frame buffer

	//	Pick an appropriate output NTV2Channel, and enable its frame buffer...
	switch (mConfig.fInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		mConfig.fOutputChannel = (numFrameStores == 2 || numFrameStores > 4) ? NTV2_CHANNEL2 : NTV2_CHANNEL3;
										break;

		case NTV2_INPUTSOURCE_SDI2:		mConfig.fOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL3 : NTV2_CHANNEL4;
										break;

		case NTV2_INPUTSOURCE_SDI3:		mConfig.fOutputChannel = NTV2_CHANNEL4;
										break;

		case NTV2_INPUTSOURCE_SDI4:		mConfig.fOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL5 : NTV2_CHANNEL3;
										break;

		case NTV2_INPUTSOURCE_SDI5: 	mConfig.fOutputChannel = NTV2_CHANNEL6;		break;
		case NTV2_INPUTSOURCE_SDI6:		mConfig.fOutputChannel = NTV2_CHANNEL7;		break;
		case NTV2_INPUTSOURCE_SDI7:		mConfig.fOutputChannel = NTV2_CHANNEL8;		break;
		case NTV2_INPUTSOURCE_SDI8:		mConfig.fOutputChannel = NTV2_CHANNEL7;		break;

		case NTV2_INPUTSOURCE_ANALOG1:
		case NTV2_INPUTSOURCE_HDMI1:	mConfig.fOutputChannel = numFrameStores < 3 ? NTV2_CHANNEL2 : NTV2_CHANNEL3;
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
				mOutputDest = NTV2_OUTPUTDESTINATION_SDI1;					//	If device has only one SDI output
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

	//	This demo requires an interlaced signal...
	if (IsProgressiveTransport(mVideoFormat))
		{cerr << "## ERROR:  Input signal is progressive -- no fields" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Free-run the device clock, since E-to-E mode isn't used, nor is a mixer tied to the input...
	mDevice.SetReference(NTV2_REFERENCE_FREERUN);

	//	Check the timecode source...
	if (!InputSignalHasTimecode())
		cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString(mConfig.fTimecodeSource, true) << "' has no embedded timecode" << endl;

	//	If the device supports different per-channel video formats, configure it as requested...
	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode (mConfig.fDoMultiFormat);

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
		mDevice.DisableRP188Bypass(::NTV2InputSourceToChannel(mConfig.fInputSource));

	//	Now that newer AJA devices can capture/play anc data from separate buffers,
	//	there's no need to enable VANC frame geometries...
	mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fInputChannel);
	mDevice.SetVANCMode (NTV2_VANCMODE_OFF, mConfig.fOutputChannel);
	if (::Is8BitFrameBufferFormat (mConfig.fPixelFormat))
	{	//	8-bit FBFs:  since not using VANC geometries, disable bit shift...
		mDevice.SetVANCShiftMode (mConfig.fInputChannel, NTV2_VANCDATA_NORMAL);
		mDevice.SetVANCShiftMode (mConfig.fOutputChannel, NTV2_VANCDATA_NORMAL);
	}

	//	Now that the video is set up, get information about the current frame geometry...
	mFormatDesc = NTV2FormatDescriptor (mVideoFormat, mConfig.fPixelFormat);
	if (mFormatDesc.IsPlanar())
		{cerr << "## ERROR: This demo doesn't work with planar pixel formats" << endl;  return AJA_STATUS_UNSUPPORTED;}
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2FieldBurn::SetupAudio (void)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
		return AJA_STATUS_SUCCESS;

	if (mConfig.fDoMultiFormat)
		if (mDevice.features().GetNumAudioSystems() > 1)
			if (UWord(mConfig.fInputChannel) < mDevice.features().GetNumAudioSystems())
				mAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fInputChannel);

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


AJAStatus NTV2FieldBurn::SetupHostBuffers (void)
{
	ULWordSequence failures;
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Let my circular buffer know when it's time to quit...
	mFrameDataRing.SetAbortFlag(&mGlobalQuit);

	//  Make the video buffers half the size of a full frame (i.e. field-size)...
	const ULWord vidBuffSizeBytes (mFormatDesc.GetTotalBytes() / 2);
	NTV2_ASSERT(mFormatDesc.GetBytesPerRow() ==  mFormatDesc.linePitch * 4);

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());			//	Make a new NTV2FrameData...
		NTV2FrameData & frameData (mHostBuffers.back());	//	...and get a reference to it

		//	In Field Mode, one buffer is used to hold each field's video data.
		frameData.fVideoBuffer.Allocate (vidBuffSizeBytes, /*pageAligned*/true);
		if (!mConfig.FieldMode())
		{	//  In Frame Mode, use two buffers, one for each field.  The DMA transfer
			//	of each field will be done as a group of lines, with each line considered a "segment".
			frameData.fVideoBuffer2.Allocate (vidBuffSizeBytes, /*pageAligned*/true);
		}

		//	Allocate a page-aligned audio buffer (if handling audio)...
		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
			frameData.fAudioBuffer.Allocate (NTV2_AUDIOSIZE_MAX, /*pageAligned*/true);
		if (frameData.AudioBuffer())
			frameData.fAudioBuffer.Fill(ULWord(0));

		//	Check for memory allocation failures...
		if (!frameData.VideoBuffer()
			|| (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem) && !frameData.AudioBuffer())
			|| (!frameData.VideoBuffer2() && !mConfig.FieldMode()))
				failures.push_back(ULWord(mHostBuffers.size()+1));

		//	Add this NTV2FrameData to the ring...
		mFrameDataRing.Add (&frameData);
	}	//	for each NTV2FrameData

	if (!failures.empty())
	{
		cerr << "## ERROR: " << DEC(failures.size()) << " allocation failures in buffer(s): " << failures << endl;
		return AJA_STATUS_MEMORY;
	}
	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2FieldBurn::RouteInputSignal (void)
{
	const NTV2OutputCrosspointID	inputOutputXpt	(::GetInputSourceOutputXpt (mConfig.fInputSource));
	const NTV2InputCrosspointID		fbInputXpt		(::GetFrameStoreInputXptFromChannel (mConfig.fInputChannel));
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


void NTV2FieldBurn::RouteOutputSignal (void)
{
	const NTV2InputCrosspointID		outputInputXpt	(::GetOutputDestInputXpt (mOutputDest));
	const NTV2OutputCrosspointID	fbOutputXpt		(::GetFrameStoreOutputXptFromChannel (mConfig.fOutputChannel, ::IsRGBFormat (mConfig.fPixelFormat)));
	const bool						isRGB			(::IsRGBFormat(mConfig.fPixelFormat));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

	if (isRGB)
	{
		const NTV2OutputCrosspointID	cscVidOutputXpt	(::GetCSCOutputXptFromChannel (mConfig.fOutputChannel, false, true));
		const NTV2InputCrosspointID		cscVidInputXpt	(::GetCSCInputXptFromChannel (mConfig.fOutputChannel));

		mDevice.Connect (cscVidInputXpt, fbOutputXpt);		//	Connect the CSC's video input to the frame store's output
		mDevice.Connect (outputInputXpt, cscVidOutputXpt);	//	Connect the SDI output's input to the CSC's video output
		outputXpt = cscVidOutputXpt;
	}
	else
		mDevice.Connect (outputInputXpt, outputXpt);

	mTCOutputs.clear();
	mTCOutputs.push_back(mConfig.fOutputChannel);

	if (!mConfig.fDoMultiFormat)
	{
		//	Route all SDI outputs to the outputXpt...
		const NTV2Channel	startNum		(NTV2_CHANNEL1);
		const NTV2Channel	endNum			(NTV2Channel(mDevice.features().GetNumVideoChannels()));
		NTV2WidgetID		outputWidgetID	(NTV2_WIDGET_INVALID);

		for (NTV2Channel chan(startNum);  chan < endNum;  chan = NTV2Channel(chan+1))
		{
			if (chan == mConfig.fInputChannel  ||  chan == mConfig.fOutputChannel)
				continue;	//	Skip the input & output channel, already routed
			if (mDevice.features().HasBiDirectionalSDI())
				mDevice.SetSDITransmitEnable (chan, true);
			if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, mDevice.features().CanDoDualLink()), outputWidgetID))
				if (mDevice.features().CanDoWidget(outputWidgetID))
				{
					mDevice.Connect (::GetSDIOutputInputXpt(chan), outputXpt);
					mTCOutputs.push_back(chan);
				}
		}	//	for each output spigot

		//	If HDMI and/or analog video outputs are available, route them, too...
		if (mDevice.features().CanDoWidget(NTV2_WgtHDMIOut1))
			mDevice.Connect (NTV2_XptHDMIOutInput, outputXpt);			//	Route output signal to HDMI output
		if (mDevice.features().CanDoWidget(NTV2_WgtHDMIOut1v2))
			mDevice.Connect (NTV2_XptHDMIOutQ1Input, outputXpt);		//	Route output signal to HDMI output
		if (mDevice.features().CanDoWidget(NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, outputXpt);		//	Route output signal to Analog output
		if (mDevice.features().CanDoWidget(NTV2_WgtSDIMonOut1))
		{	//	SDI Monitor output is spigot 4 (NTV2_CHANNEL5):
			mDevice.Connect (::GetSDIOutputInputXpt(NTV2_CHANNEL5), outputXpt);	//	Route output signal to SDI monitor output
			mTCOutputs.push_back(NTV2_CHANNEL5);
		}
	}	//	if not multiChannel
	PLDBG(mTCOutputs.size() << " timecode destination(s):  " << ::NTV2ChannelListToStr(mTCOutputs));

}	//	RouteOutputSignal


AJAStatus NTV2FieldBurn::Run (void)
{
	//	Start the playout and capture threads...
	StartPlayThread();
	StartCaptureThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we will start the play thread
void NTV2FieldBurn::StartPlayThread (void)
{
	//	Create and start the playout thread...
	mPlayThread.Attach(PlayThreadStatic, this);
	mPlayThread.SetPriority(AJA_ThreadPriority_High);
	mPlayThread.Start();

}	//	StartPlayThread


//	The playout thread function
void NTV2FieldBurn::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2FieldBurn instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2FieldBurn *	pApp (reinterpret_cast<NTV2FieldBurn*>(pContext));
	pApp->PlayFrames();

}	//	PlayThreadStatic


void NTV2FieldBurn::PlayFrames (void)
{
	const ULWord			acOptions (AUTOCIRCULATE_WITH_RP188 | (mConfig.FieldMode() ? AUTOCIRCULATE_WITH_FIELDS : 0));
	ULWord					goodXfers(0), badXfers(0), starves(0), noRoomWaits(0);
	AUTOCIRCULATE_TRANSFER	outputXferInfo;		//	F1 transfer info
	AUTOCIRCULATE_TRANSFER	outputXferInfo2;	//	F2 transfer info (unused in Field Mode)
	AUTOCIRCULATE_STATUS	outputStatus;

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel, 4);	//	Let it stop
	BURNNOTE("Thread started");

	if (!mConfig.FieldMode())
	{
		//	In Frame Mode, tell AutoCirculate to DMA F1's data as a group of "segments".
		//	Each segment is one line long, and the segments are contiguous in host
		//	memory, but are stored on every other line in the device frame buffer...
		outputXferInfo.EnableSegmentedDMAs (mFormatDesc.numLines / 2,		//	number of segments:  number of lines per field, i.e. half the line count
												mFormatDesc.linePitch * 4,	//	number of active bytes per line
												mFormatDesc.linePitch * 4,	//	host bytes per line:  normal line pitch when reading from our half-height buffer
												mFormatDesc.linePitch * 8);	//	device bytes per line:  skip every other line when writing into device memory

		//	F2 is identical to F1, except that F2 starts on 2nd line in device frame buffer...
		outputXferInfo2.EnableSegmentedDMAs (mFormatDesc.numLines / 2,		//	number of segments:  number of lines per field, i.e. half the line count
												mFormatDesc.linePitch * 4,	//	number of active bytes per line
												mFormatDesc.linePitch * 4,	//	host bytes per line:  normal line pitch when reading from our half-height buffer
												mFormatDesc.linePitch * 8);	//	device bytes per line:  skip every other line when writing into device memory
		outputXferInfo2.acInVideoDMAOffset	= mFormatDesc.linePitch * 4;	//  F2 starts on 2nd line in device buffer
	}

	//	Initialize AutoCirculate...
	if (!mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel, mConfig.fOutputFrames, mAudioSystem, acOptions))
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
				{starves++;  continue;}	//	Producer thread isn't producing frames fast enough

			//	Prepare to transfer the timecode-burned field (F1) to the device for playout.
			//	Set the outputXfer struct's video and audio buffers from playData's buffers...
			//  IMPORTANT:	In Frame Mode, for segmented DMAs, AutoCirculateTransfer expects the video
			//				buffer size to be set to the segment size, in bytes, which is one raster line length.
			outputXferInfo.SetVideoBuffer (pFrameData->VideoBuffer(),
											mConfig.FieldMode()	? pFrameData->VideoBufferSize()		//	Field Mode
																: mFormatDesc.GetBytesPerRow());	//	Frame Mode
			if (pFrameData->AudioBuffer())
				outputXferInfo.SetAudioBuffer (pFrameData->AudioBuffer(), pFrameData->AudioBufferSize());
			if (mConfig.FieldMode())
				outputXferInfo.acPeerToPeerFlags = pFrameData->fFrameFlags;	//	Which field was this?

			//	Tell AutoCirculate to embed this frame's timecode into the SDI output(s)...
			outputXferInfo.SetOutputTimeCodes(pFrameData->fTimecodes);
			PLDBG(pFrameData->fTimecodes);

			//	Transfer field to the device...
			if (mDevice.AutoCirculateTransfer (mConfig.fOutputChannel, outputXferInfo))
				goodXfers++;
			else
				badXfers++;

			if (!mConfig.FieldMode())
			{	//  Frame Mode:  Additionally transfer F2 to the same device frame buffer used for F1.
				//  Again, for segmented DMAs, AutoCirculateTransfer expects the video buffer size to be
				//	set to the segment size, in bytes, which is one raster line length.
				outputXferInfo2.acDesiredFrame = outputXferInfo.acTransferStatus.acTransferFrame;
				outputXferInfo2.SetVideoBuffer (pFrameData->VideoBuffer2(), mFormatDesc.GetBytesPerRow());
				if (mDevice.AutoCirculateTransfer (mConfig.fOutputChannel, outputXferInfo2))
					goodXfers++;
				else
					badXfers++;
			}

			if (goodXfers == 6)	//	Start AutoCirculate playout once 6 fields are buffered on the device...
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
	mDevice.AutoCirculateStop (mConfig.fOutputChannel);
	BURNNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(starves) << " ring starves, " << DEC(noRoomWaits) << " device starves");
}	//	PlayFrames


//////////////////////////////////////////////



//////////////////////////////////////////////
//
//	This is where the capture thread gets started
//
void NTV2FieldBurn::StartCaptureThread (void)
{
	//	Create and start the capture thread...
	mCaptureThread.Attach(CaptureThreadStatic, this);
	mCaptureThread.SetPriority(AJA_ThreadPriority_High);
	mCaptureThread.Start();

}	//	StartCaptureThread


//
//	The static capture thread function
//
void NTV2FieldBurn::CaptureThreadStatic (AJAThread * pThread, void * pContext)		//	static
{	(void) pThread;
	//	Grab the NTV2FieldBurn instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2FieldBurn *	pApp(reinterpret_cast<NTV2FieldBurn*>(pContext));
	pApp->CaptureFrames();
}	//	CaptureThreadStatic


//
//	Repeatedly captures frames until told to stop
//
void NTV2FieldBurn::CaptureFrames (void)
{
	AUTOCIRCULATE_TRANSFER	inputXferInfo;	//	F1 transfer info
	AUTOCIRCULATE_TRANSFER	inputXferInfo2;	//	F2 transfer info (unused in Field Mode)
	NTV2TCIndexes			F1TCIndexes, F2TCIndexes;
	const ULWord			acOptions ((mConfig.WithTimecode() ? AUTOCIRCULATE_WITH_RP188 : 0)  |  (mConfig.FieldMode() ? AUTOCIRCULATE_WITH_FIELDS : 0));
	ULWord					goodXfers(0), badXfers(0), ringFulls(0), devWaits(0);
	BURNNOTE("Thread started");

	//	Prepare the Timecode Indexes we'll be setting for playout...
	for (size_t ndx(0);  ndx < mTCOutputs.size();  ndx++)
	{	const NTV2Channel sdiSpigot(mTCOutputs.at(ndx));
		F1TCIndexes.insert(::NTV2ChannelToTimecodeIndex(sdiSpigot, /*LTC?*/true));			//	F1 LTC
		F1TCIndexes.insert(::NTV2ChannelToTimecodeIndex(sdiSpigot, /*LTC?*/false));			//	F1 VITC
		F2TCIndexes.insert(::NTV2ChannelToTimecodeIndex(sdiSpigot, /*LTC?*/false, true));	//	F2 VITC
	}

	if (!mConfig.FieldMode())
	{
		//	In Frame Mode, use AutoCirculate's "segmented DMA" feature to transfer each field
		//	out of the device's full-frame video buffer as a group of "segments".
		//	Each segment is one line long, and the segments are contiguous in host memory,
		//	but originate on alternating lines in the device's frame buffer...
		inputXferInfo.EnableSegmentedDMAs (mFormatDesc.numLines / 2,		//	Number of segments:		number of lines per field, i.e. half the line count
											mFormatDesc.linePitch * 4,		//	Segment size, in bytes:	transfer this many bytes per segment (normal line pitch)
											mFormatDesc.linePitch * 4,		//	Host bytes per line:	normal line pitch when writing into our half-height buffer
											mFormatDesc.linePitch * 8);		//	Device bytes per line:	skip every other line when reading from device memory

		//	IMPORTANT:	For segmented DMAs, the video buffer size must contain the number of bytes to
		//				transfer per segment. This will be done just prior to calling AutoCirculateTransfer.
		inputXferInfo2.EnableSegmentedDMAs (mFormatDesc.numLines / 2,		//	Number of segments:		number of lines per field, i.e. half the line count
											mFormatDesc.linePitch * 4,		//	Segment size, in bytes:	transfer this many bytes per segment (normal line pitch)
											mFormatDesc.linePitch * 4,		//	Host bytes per line:	normal line pitch when writing into our half-height buffer
											mFormatDesc.linePitch * 8);		//	Device bytes per line:	skip every other line when reading from device memory
		inputXferInfo2.acInVideoDMAOffset	= mFormatDesc.linePitch * 4;	//  Field 2 starts on second line in device buffer
	}

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop (mConfig.fInputChannel);

	//	Initialize AutoCirculate...
	if (!mDevice.AutoCirculateInitForInput (mConfig.fInputChannel,	//	channel
											mConfig.fInputFrames,	//	frame count/range
											mAudioSystem,			//	audio system
											acOptions))				//	flags
		{BURNFAIL("AutoCirculateInitForInput failed");  mGlobalQuit = true;}
	else
		//	Start AutoCirculate running...
		mDevice.AutoCirculateStart (mConfig.fInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS acStatus;
		mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, acStatus);

		if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
		{
			//	At this point, there's at least one fully-formed frame available in the device's frame buffer
			//	memory waiting to be transferred to the host. Reserve an NTV2FrameData to fill ("produce"),
			//	and use it in the next frame transferred from the device...
			NTV2FrameData *	pFrameData	(mFrameDataRing.StartProduceNextBuffer ());
			if (!pFrameData)
				{ringFulls++;  continue;}	//	Ring full -- consumer thread isn't consuming frames fast enough

			inputXferInfo.SetVideoBuffer (pFrameData->VideoBuffer(),
											mConfig.FieldMode()	? pFrameData->VideoBufferSize()
																: mFormatDesc.GetBytesPerRow());
			if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
				inputXferInfo.SetAudioBuffer (pFrameData->AudioBuffer(), NTV2_AUDIOSIZE_MAX);

			//	Transfer this Field (Field Mode) or F1 (Frame Mode) from the device into our host buffers...
			if (mDevice.AutoCirculateTransfer (mConfig.fInputChannel, inputXferInfo)) goodXfers++;
			else badXfers++;

			//	Remember the amount, in bytes, of captured audio data...
			pFrameData->fNumAudioBytes	= pFrameData->AudioBuffer()	? inputXferInfo.GetCapturedAudioByteCount()	: 0;
			if (mConfig.FieldMode())
				pFrameData->fFrameFlags = inputXferInfo.acPeerToPeerFlags;	//	Remember which field this was

			//	Get a timecode to use for burn-in...
			NTV2_RP188	thisFrameTC;
			inputXferInfo.GetInputTimeCodes (pFrameData->fTimecodes, mConfig.fInputChannel);
			if (!pFrameData->fTimecodes.empty())
			{
				thisFrameTC = pFrameData->fTimecodes.begin()->second;	//	Use 1st "good" timecode for burn-in
				CAPDBG("Captured TC: " << ::NTV2TCIndexToString(pFrameData->fTimecodes.begin()->first,true) << " " << thisFrameTC);
			}
			else
			{	//	Invent a timecode (based on frame count)...
				const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
				const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
				const	CRP188			inventedTC		(inputXferInfo.GetTransferStatus().GetProcessedFrameCount(), 0, 0, 10, tcFormat);
				inventedTC.GetRP188Reg(thisFrameTC);
			}
			CRP188 tc(thisFrameTC);
			string tcStr;
			tc.GetRP188Str(tcStr);

			if (!mConfig.FieldMode())
			{	//  Frame Mode: Transfer F2 segments from same device frame buffer used for F1...
				inputXferInfo2.acDesiredFrame = inputXferInfo.acTransferStatus.acTransferFrame;
				inputXferInfo2.SetVideoBuffer (pFrameData->VideoBuffer2(), mFormatDesc.GetBytesPerRow());
				if (mDevice.AutoCirculateTransfer (mConfig.fInputChannel, inputXferInfo2)) goodXfers++;
				else badXfers++;
			}

			//	While this NTV2FrameData's buffers are locked, "burn" identical timecode into each field...
			//	F1 goes into top half, F2 into bottom half...
			mTCBurner.BurnTimeCode (ToCharPtr(inputXferInfo.acVideoBuffer.GetHostPointer()), tcStr.c_str(),
									!mConfig.FieldMode() || (pFrameData->fFrameFlags & AUTOCIRCULATE_FRAME_FIELD0) ? 10 : 30);
			if (!mConfig.FieldMode())	//	Frame Mode: "burn" F2 timecode
				mTCBurner.BurnTimeCode (ToCharPtr(inputXferInfo2.acVideoBuffer.GetHostPointer()), tcStr.c_str(), 30);

			//	Set NTV2FrameData::fTimecodes map for playout...
			for (NTV2TCIndexesConstIter it(F1TCIndexes.begin());  it != F1TCIndexes.end();  ++it)
				pFrameData->fTimecodes[*it] = thisFrameTC;
			for (NTV2TCIndexesConstIter it(F2TCIndexes.begin());  it != F2TCIndexes.end();  ++it)
				pFrameData->fTimecodes[*it] = thisFrameTC;

			//	Signal that we're done "producing" the frame, making it available for future "consumption"...
			mFrameDataRing.EndProduceNextBuffer();
		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a field/frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			devWaits++;
			if (mConfig.FieldMode())
				mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel);
			else
				mDevice.WaitForInputFieldID (NTV2_FIELD0, mConfig.fInputChannel);
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fInputChannel);
	BURNNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(ringFulls) << " ring full(s), " << DEC(devWaits) << " device waits");

}	//	CaptureFrames


//////////////////////////////////////////////


void NTV2FieldBurn::GetStatus (ULWord & outFramesProcessed, ULWord & outCaptureFramesDropped, ULWord & outPlayoutFramesDropped,
								ULWord & outCaptureBufferLevel, ULWord & outPlayoutBufferLevel)
{
	AUTOCIRCULATE_STATUS	inputStatus,  outputStatus;

	mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, inputStatus);
	mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outputStatus);

	outFramesProcessed		= inputStatus.acFramesProcessed;
	outCaptureFramesDropped	= inputStatus.acFramesDropped;
	outPlayoutFramesDropped	= outputStatus.acFramesDropped;
	outCaptureBufferLevel	= inputStatus.acBufferLevel;
	outPlayoutBufferLevel	= outputStatus.acBufferLevel;

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


bool NTV2FieldBurn::InputSignalHasTimecode (void)
{
	const ULWord	regNum		(::GetRP188RegisterForInput (mConfig.fInputSource));
	ULWord			regValue	(0);

	//	Bit 16 of the RP188 DBB register will be set if there is timecode embedded in the input signal...
	if (regNum  &&  mDevice.ReadRegister(regNum, regValue)  &&  regValue & BIT(16))
		return true;
	return false;

}	//	InputSignalHasTimecode
