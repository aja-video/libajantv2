/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2fieldburn.cpp
	@brief		Implementation of NTV2FieldBurn demonstration class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2fieldburn.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ajabase/common/types.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include "ntv2democommon.h"
#include <iostream>

using namespace std;

#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define ToCharPtr(_p_)	reinterpret_cast<char*>(_p_)
#define ToU32Ptr(_p_)	reinterpret_cast<uint32_t*>(_p_)

const uint32_t	kAppSignature	(NTV2_FOURCC('F','l','d','B'));


//////////////////////	IMPLEMENTATION


NTV2FieldBurn::NTV2FieldBurn (const string &				inDeviceSpecifier,
								const bool					inWithAudio,
								const bool					inFieldMode,
								const NTV2FrameBufferFormat	inPixelFormat,
								const NTV2InputSource		inInputSource,
								const bool					inDoMultiFormat)

	:	mPlayThread			(AJAThread()),
		mCaptureThread		(AJAThread()),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mDeviceSpecifier	(inDeviceSpecifier),
		mInputChannel		(NTV2_CHANNEL_INVALID),
		mOutputChannel		(NTV2_CHANNEL_INVALID),
		mInputSource		(inInputSource),
		mOutputDestination	(NTV2_OUTPUTDESTINATION_INVALID),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mPixelFormat		(inPixelFormat),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mVancMode			(NTV2_VANCMODE_OFF),
		mAudioSystem		(inWithAudio ? NTV2_AUDIOSYSTEM_1 : NTV2_AUDIOSYSTEM_INVALID),
		mIsFieldMode		(inFieldMode),
		mGlobalQuit			(false),
		mDoMultiChannel		(inDoMultiFormat),
		mHostBuffers		(),
		mAVCircularBuffer	()
{
}	//	constructor


NTV2FieldBurn::~NTV2FieldBurn ()
{
	//	Stop my capture and playout threads, then destroy them...
	Quit();

	//	Unsubscribe from input vertical event...
	mDevice.UnsubscribeInputVerticalEvent (mInputChannel);

}	//	destructor


void NTV2FieldBurn::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mPlayThread.Active())
		AJATime::Sleep(10);

	while (mCaptureThread.Active())
		AJATime::Sleep(10);

	if (!mDoMultiChannel)
	{	//	Release the device...
		mDevice.ReleaseStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));
		mDevice.SetEveryFrameServices (mSavedTaskMode);	//	Restore prior task mode
	}

}	//	Quit


AJAStatus NTV2FieldBurn::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID();	//	Keep the device ID handy since it will be used frequently
	if (!::NTV2DeviceCanDoCapture(mDeviceID))
		{cerr << "## ERROR:  Device '" << mDeviceID << "' cannot capture" << endl;  return AJA_STATUS_FEATURE;}
	if (!::NTV2DeviceCanDoPlayback(mDeviceID))
		{cerr << "## ERROR:  Device '" << mDeviceID << "' cannot playout" << endl;  return AJA_STATUS_FEATURE;}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetStreamingApplication (appSignature, appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices(mSavedTaskMode);			//	Save the current device state
	if (!mDoMultiChannel)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mDevice.ClearRouting();	//	Clear the current device routing (since I "own" the device)
	}
	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);	//	Force OEM tasks

	//	If the device supports different per-channel video formats, configure it as requested...
	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
		mDevice.SetMultiFormatMode (mDoMultiChannel);

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
	mTCBurner.RenderTimeCodeFont(CNTV2DemoCommon::GetAJAPixelFormat(mPixelFormat),
																	mFormatDescriptor.numPixels,
																	mFormatDescriptor.numLines);
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2FieldBurn::SetupVideo (void)
{
	const UWord	numFrameStores	(::NTV2DeviceGetNumFrameStores (mDeviceID));

	//	Does this device have the requested input source?
	if (!::NTV2DeviceCanDoInputSource (mDeviceID, mInputSource))
		{cerr << "## ERROR:  Device does not have the specified input source" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Pick an input NTV2Channel from the input source, and enable its frame buffer...
	mInputChannel = NTV2_INPUT_SOURCE_IS_SDI(mInputSource) ? ::NTV2InputSourceToChannel(mInputSource) : NTV2_CHANNEL1;
	mDevice.EnableChannel(mInputChannel);		//	Enable the input frame buffer

	//	Pick an appropriate output NTV2Channel, and enable its frame buffer...
	switch (mInputSource)
	{
		case NTV2_INPUTSOURCE_ANALOG1:	mOutputChannel = NTV2_CHANNEL3;		break;
		case NTV2_INPUTSOURCE_HDMI1:	mOutputChannel = NTV2_CHANNEL3;		break;
		case NTV2_INPUTSOURCE_SDI1:		mOutputChannel = (numFrameStores == 2 || numFrameStores > 4) ? NTV2_CHANNEL2 : NTV2_CHANNEL3;	break;
		case NTV2_INPUTSOURCE_SDI2:		mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL3 : NTV2_CHANNEL4;	break;
		case NTV2_INPUTSOURCE_SDI3:		mOutputChannel = NTV2_CHANNEL4;		break;
		case NTV2_INPUTSOURCE_SDI4:		mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL5 : NTV2_CHANNEL3;	break;
		case NTV2_INPUTSOURCE_SDI5: 	mOutputChannel = NTV2_CHANNEL6;		break;
		case NTV2_INPUTSOURCE_SDI6:		mOutputChannel = NTV2_CHANNEL7;		break;
		case NTV2_INPUTSOURCE_SDI7:		mOutputChannel = NTV2_CHANNEL8;		break;
		case NTV2_INPUTSOURCE_SDI8:		mOutputChannel = NTV2_CHANNEL7;		break;
		default:
		case NTV2_INPUTSOURCE_INVALID:	cerr << "## ERROR:  Bad input source" << endl;
										return AJA_STATUS_BAD_PARAM;
	}
	mDevice.EnableChannel(mOutputChannel);		//	Enable the output frame buffer

	//	Pick an appropriate output spigot based on the output channel...
	mOutputDestination	= ::NTV2ChannelToOutputDestination(mOutputChannel);
	if (!::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut2) && !::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut2))
		mOutputDestination = NTV2_OUTPUTDESTINATION_SDI1;			//	If device has only one SDI output
	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID)					//	If device has bidirectional SDI connectors...
		&& NTV2_OUTPUT_DEST_IS_SDI(mOutputDestination))			//	...and output destination is SDI...
			mDevice.SetSDITransmitEnable (mOutputChannel, true);	//	...then enable transmit mode

	//	Flip the input spigot to "receive" if necessary...
	bool isXmit(false);
	if (::NTV2DeviceHasBiDirectionalSDI(mDevice.GetDeviceID())		//	If device has bidirectional SDI connectors...
		&& NTV2_INPUT_SOURCE_IS_SDI(mInputSource)					//	...and desired input source is SDI...
			&& mDevice.GetSDITransmitEnable(mInputChannel, isXmit)	//	...and GetSDITransmitEnable succeeds...
				&& isXmit)											//	...and input is set to "transmit"...
	{
		mDevice.SetSDITransmitEnable (mInputChannel, false);		//	...then disable transmit mode...
		mDevice.WaitForInputVerticalInterrupt(mInputChannel, 10);	//	...and give the device a dozen frames or so to lock to the input signal
	}	//	if input SDI connector needs to switch from transmit mode

	//	Is there an input signal?  What format is it?
	mVideoFormat = mDevice.GetInputVideoFormat(mInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		{cerr << "## ERROR:  No input signal, or can't handle its format" << endl;  return AJA_STATUS_NOINPUT;}

	//	This demo requires an interlaced signal...
	if (IsProgressiveTransport(mVideoFormat))
		{cerr << "## ERROR:  Input signal is progressive -- no fields" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Free-run the device clock, since E-to-E mode isn't used, nor is a mixer tied to the input...
	mDevice.SetReference(NTV2_REFERENCE_FREERUN);

	//	Check the timecode source...
	if (!InputSignalHasTimecode())
		cerr << "## WARNING:  Timecode source has no embedded timecode" << endl;

	//	Set the input/output channel video formats to the video format that was detected earlier...
	mDevice.SetVideoFormat (mVideoFormat, false, false, ::NTV2DeviceCanDoMultiFormat(mDeviceID) ? mInputChannel : NTV2_CHANNEL1);
	if (::NTV2DeviceCanDoMultiFormat(mDeviceID))								//	If device supports multiple formats per-channel...
		mDevice.SetVideoFormat (mVideoFormat, false, false, mOutputChannel);	//	...then also set the output channel format to the detected input format

	//	Can the device handle the requested frame buffer pixel format?
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPixelFormat))
	{
		cerr	<< "## WARNING:  " << ::NTV2FrameBufferFormatToString(mPixelFormat)
				<< " unsupported, using " << ::NTV2FrameBufferFormatToString(NTV2_FBF_8BIT_YCBCR)
				<< " instead" << endl;
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;		//	Fall back to 8-bit YCbCr
	}

	//	Set both input and output frame buffers' pixel formats...
	mDevice.SetFrameBufferFormat (mInputChannel, mPixelFormat);
	mDevice.SetFrameBufferFormat (mOutputChannel, mPixelFormat);

	//	Enable and subscribe to the input interrupts...
	mDevice.EnableInputInterrupt(mInputChannel);
	mDevice.SubscribeInputVerticalEvent(mInputChannel);

	//	Enable and subscribe to the output interrupts...
	mDevice.EnableOutputInterrupt(mOutputChannel);
	mDevice.SubscribeOutputVerticalEvent(mOutputChannel);

	//	Normally, timecode embedded in the output signal comes from whatever is written into the RP188
	//	registers (30/31 for SDI out 1, 65/66 for SDIout2, etc.).
	//	AutoCirculate automatically writes the timecode in the AUTOCIRCULATE_TRANSFER's acRP188 field
	//	into these registers (if AutoCirculateInitForOutput was called with AUTOCIRCULATE_WITH_RP188 set).
	//	Newer AJA devices can also bypass these RP188 registers, and simply copy whatever timecode appears
	//	at any SDI input (called the "bypass source"). To ensure that AutoCirculate's playout timecode
	//	will actually be seen in the output signal, "bypass mode" must be disabled...
	bool	bypassIsEnabled	(false);
	mDevice.IsRP188BypassEnabled (::NTV2InputSourceToChannel(mInputSource), bypassIsEnabled);
	if (bypassIsEnabled)
		mDevice.DisableRP188Bypass(::NTV2InputSourceToChannel(mInputSource));

	//	Now that newer AJA devices can capture/play anc data from separate buffers,
	//	there's no need to enable VANC frame geometries...
	mDevice.SetVANCMode (mVancMode, mInputChannel);
	mDevice.SetVANCMode (mVancMode, mOutputChannel);
	if (::Is8BitFrameBufferFormat (mPixelFormat))
	{
		//	8-bit FBFs require bit shift for VANC geometries, or no shift for normal geometries...
		mDevice.SetVANCShiftMode (mInputChannel, NTV2_IS_VANCMODE_ON(mVancMode) ? NTV2_VANCDATA_8BITSHIFT_ENABLE : NTV2_VANCDATA_NORMAL);
		mDevice.SetVANCShiftMode (mOutputChannel, NTV2_IS_VANCMODE_ON(mVancMode) ? NTV2_VANCDATA_8BITSHIFT_ENABLE : NTV2_VANCDATA_NORMAL);
	}

	//	Now that the video is set up, get information about the current frame geometry...
	mFormatDescriptor = NTV2FormatDescriptor (mVideoFormat, mPixelFormat, mVancMode);
	if (mFormatDescriptor.IsPlanar())
		{cerr << "## ERROR: This demo doesn't work with planar pixel formats" << endl;  return AJA_STATUS_UNSUPPORTED;}
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2FieldBurn::SetupAudio (void)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
		return AJA_STATUS_SUCCESS;

	if (mDoMultiChannel)
		if (::NTV2DeviceGetNumAudioSystems(mDeviceID) > 1)
			if (UWord(mInputChannel) < ::NTV2DeviceGetNumAudioSystems(mDeviceID))
				mAudioSystem = ::NTV2ChannelToAudioSystem(mInputChannel);

	//	Have the audio subsystem capture audio from the designated input source...
	mDevice.SetAudioSystemInputSource (mAudioSystem, ::NTV2InputSourceToAudioSource(mInputSource),
										::NTV2InputSourceToEmbeddedAudioInput(mInputSource));

	//	It's best to use all available audio channels...
	mDevice.SetNumberAudioChannels(::NTV2DeviceGetMaxAudioChannels(mDeviceID), mAudioSystem);

	//	Assume 48kHz PCM...
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	4MB device audio buffers work best...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set up the output audio embedders...
	if (::NTV2DeviceGetNumAudioSystems(mDeviceID) > 1)
	{
		//	Some devices, like the Kona1, have 2 FrameStores but only 1 SDI output,
		//	which makes mOutputChannel == NTV2_CHANNEL2, but need SDIoutput to be NTV2_CHANNEL1...
		UWord	SDIoutput(mOutputChannel);
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
	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


AJAStatus NTV2FieldBurn::SetupHostBuffers (void)
{
	ULWordSequence failures;

	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag(&mGlobalQuit);

	//  Make the video buffers half the size of a full frame (i.e. field-size)...
	const ULWord videoBufferSize(mFormatDescriptor.GetTotalBytes() / 2);
	NTV2_ASSERT(mFormatDescriptor.GetBytesPerRow() ==  mFormatDescriptor.linePitch * 4);

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	mHostBuffers.reserve(size_t(CIRCULAR_BUFFER_SIZE));
	while (mHostBuffers.size() < size_t(CIRCULAR_BUFFER_SIZE))
	{
		mHostBuffers.push_back(NTV2FrameData());
		NTV2FrameData & frameData(mHostBuffers.back());
		//	In Field Mode, one buffer is used to hold each field's video data.
		frameData.fVideoBuffer.Allocate(videoBufferSize, /*pageAligned*/true);
		if (!mIsFieldMode)
		{	//  In Frame Mode, use two buffers, one for each field.  The DMA transfer
			//	of each field will be done as a group of lines, with each line considered a "segment".
			frameData.fVideoBuffer2.Allocate(videoBufferSize, /*pageAligned*/true);
		}
		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
			frameData.fAudioBuffer.Allocate(NTV2_AUDIOSIZE_MAX, /*pageAligned*/true);

		//	Check for memory allocation failures...
		if (!frameData.VideoBuffer()
			|| (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem) && !frameData.AudioBuffer())
			|| (!frameData.VideoBuffer2() && !mIsFieldMode))
				failures.push_back(ULWord(mHostBuffers.size()+1));
		mAVCircularBuffer.Add(&frameData);
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
	const NTV2OutputCrosspointID	inputOutputXpt	(::GetInputSourceOutputXpt (mInputSource));
	const NTV2InputCrosspointID		fbInputXpt		(::GetFrameBufferInputXptFromChannel (mInputChannel));
	const bool						isRGB			(::IsRGBFormat(mPixelFormat));

	if (isRGB)
	{
		//	If the frame buffer is configured for RGB pixel format, incoming YUV must be converted.
		//	This routes the video signal from the input through a color space converter before
		//	connecting to the RGB frame buffer...
		const NTV2InputCrosspointID		cscVideoInputXpt	(::GetCSCInputXptFromChannel (mInputChannel));
		const NTV2OutputCrosspointID	cscOutputXpt		(::GetCSCOutputXptFromChannel (mInputChannel, false, true));	//	false=video, true=RGB

		mDevice.Connect (cscVideoInputXpt, inputOutputXpt);	//	Connect the CSC's video input to the input spigot's output
		mDevice.Connect (fbInputXpt, cscOutputXpt);			//	Connect the frame store's input to the CSC's output
	}
	else
		mDevice.Connect (fbInputXpt, inputOutputXpt);		//	Route the YCbCr signal directly from the input to the frame buffer's input

}	//	RouteInputSignal


void NTV2FieldBurn::RouteOutputSignal (void)
{
	const NTV2InputCrosspointID		outputInputXpt	(::GetOutputDestInputXpt (mOutputDestination));
	const NTV2OutputCrosspointID	fbOutputXpt		(::GetFrameBufferOutputXptFromChannel (mOutputChannel, ::IsRGBFormat (mPixelFormat)));
	const bool						isRGB			(::IsRGBFormat(mPixelFormat));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

	if (isRGB)
	{
		const NTV2OutputCrosspointID	cscVidOutputXpt	(::GetCSCOutputXptFromChannel (mOutputChannel, false, true));
		const NTV2InputCrosspointID		cscVidInputXpt	(::GetCSCInputXptFromChannel (mOutputChannel));

		mDevice.Connect (cscVidInputXpt, fbOutputXpt);		//	Connect the CSC's video input to the frame store's output
		mDevice.Connect (outputInputXpt, cscVidOutputXpt);	//	Connect the SDI output's input to the CSC's video output
		outputXpt = cscVidOutputXpt;
	}
	else
		mDevice.Connect (outputInputXpt, outputXpt);

	mTCOutputs.clear();
	mTCOutputs.push_back(mOutputChannel);

	if (!mDoMultiChannel)
	{
		//	Route all SDI outputs to the outputXpt...
		const NTV2Channel	startNum		(NTV2_CHANNEL1);
		const NTV2Channel	endNum			(NTV2Channel(::NTV2DeviceGetNumVideoChannels(mDeviceID)));
		NTV2WidgetID		outputWidgetID	(NTV2_WIDGET_INVALID);

		for (NTV2Channel chan(startNum);  chan < endNum;  chan = NTV2Channel(chan+1))
		{
			if (chan == mInputChannel  ||  chan == mOutputChannel)
				continue;	//	Skip the input & output channel, already routed
			if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
				mDevice.SetSDITransmitEnable (chan, true);
			if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, ::NTV2DeviceCanDoDualLink(mDeviceID)), outputWidgetID))
				if (::NTV2DeviceCanDoWidget (mDeviceID, outputWidgetID))
				{
					mDevice.Connect (::GetSDIOutputInputXpt(chan), outputXpt);
					mTCOutputs.push_back(chan);
				}
		}	//	for each output spigot

		//	If HDMI and/or analog video outputs are available, route them, too...
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1))
			mDevice.Connect (NTV2_XptHDMIOutInput, outputXpt);			//	Route output signal to HDMI output
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v2))
			mDevice.Connect (NTV2_XptHDMIOutQ1Input, outputXpt);		//	Route output signal to HDMI output
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, outputXpt);		//	Route output signal to Analog output
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIMonOut1))
		{	//	SDI Monitor output is spigot 4 (NTV2_CHANNEL5):
			mDevice.Connect (::GetSDIOutputInputXpt(NTV2_CHANNEL5), outputXpt);	//	Route output signal to SDI monitor output
			mTCOutputs.push_back(NTV2_CHANNEL5);
		}
	}	//	if not multiChannel
	PLDBG(mTCOutputs.size() << " timecode destination(s):  " << ::NTV2ChannelListToStr(mTCOutputs));

}	//	RouteOutputSignal


AJAStatus NTV2FieldBurn::Run ()
{
	//	Start the playout and capture threads...
	StartPlayThread ();
	StartCaptureThread ();
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
{
	(void) pThread;
	//	Grab the NTV2FieldBurn instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2FieldBurn *	pApp	(reinterpret_cast <NTV2FieldBurn *> (pContext));
	pApp->PlayFrames ();

}	//	PlayThreadStatic


void NTV2FieldBurn::PlayFrames (void)
{
	AUTOCIRCULATE_TRANSFER	outputXferField1;	//	Field A/C output transfer info
	AUTOCIRCULATE_TRANSFER	outputXferField2;	//	Field 2 A/C output transfer info (not used in Field Mode)
	PLNOTE("Thread started");

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop (mOutputChannel);

	if (!mIsFieldMode)
	{
		//	In Frame Mode, tell AutoCirculate to DMA F1's data as a group of "segments".
		//	Each segment is one line long, and the segments are contiguous in host
		//	memory, but are stored on every other line in the device frame buffer...
		outputXferField1.EnableSegmentedDMAs (mFormatDescriptor.numLines / 2,		//	number of segments:  number of lines per field, i.e. half the line count
												mFormatDescriptor.linePitch * 4,	//	number of active bytes per line
												mFormatDescriptor.linePitch * 4,	//	host bytes per line:  normal line pitch when reading from our half-height buffer
												mFormatDescriptor.linePitch * 8);	//	device bytes per line:  skip every other line when writing into device memory

		//	F2 is identical to F1, except that F2 starts on 2nd line in device frame buffer...
		outputXferField2.EnableSegmentedDMAs (mFormatDescriptor.numLines / 2,		//	number of segments:  number of lines per field, i.e. half the line count
												mFormatDescriptor.linePitch * 4,	//	number of active bytes per line
												mFormatDescriptor.linePitch * 4,	//	host bytes per line:  normal line pitch when reading from our half-height buffer
												mFormatDescriptor.linePitch * 8);	//	device bytes per line:  skip every other line when writing into device memory
		outputXferField2.acInVideoDMAOffset	= mFormatDescriptor.linePitch * 4;	//  F2 starts on 2nd line in device buffer
	}

	//	Initialize the AutoCirculate channel...
	mDevice.AutoCirculateInitForOutput (mOutputChannel, 7, mAudioSystem,
										AUTOCIRCULATE_WITH_RP188 | (mIsFieldMode ? AUTOCIRCULATE_WITH_FIELDS : 0));
	//	Start AutoCirculate running...
	mDevice.AutoCirculateStart (mOutputChannel);

	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		NTV2FrameData *	pFrameData	(mAVCircularBuffer.StartConsumeNextBuffer());
		if (pFrameData)
		{
			//	Prepare to transfer the timecode-burned field (F1) to the device for playout.
			//	Set the outputXfer struct's video and audio buffers from playData's buffers...
			//  IMPORTANT:	In Frame Mode, for segmented DMAs, AutoCirculateTransfer expects the video
			//				buffer size to be set to the segment size, in bytes, which is one raster line length.
			outputXferField1.SetVideoBuffer (pFrameData->VideoBuffer(),
											mIsFieldMode	? pFrameData->VideoBufferSize()			//	Field Mode
															: mFormatDescriptor.GetBytesPerRow());	//	Frame Mode
			if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
				outputXferField1.SetAudioBuffer (pFrameData->AudioBuffer(), pFrameData->AudioBufferSize());
			if (mIsFieldMode)
				outputXferField1.acPeerToPeerFlags = pFrameData->fFrameFlags;	//	Which field was this?

			//	Tell AutoCirculate to embed this frame's timecode into the SDI output(s)...
			outputXferField1.SetOutputTimeCodes(pFrameData->fTimecodes);
			PLDBG(pFrameData->fTimecodes);

			//	Transfer field to the device...
			mDevice.AutoCirculateTransfer (mOutputChannel, outputXferField1);
			if (!mIsFieldMode)
			{	//  Frame Mode:  Additionally transfer Field2 to the same device frame buffer used for F1.
				//  Again, for segmented DMAs, AutoCirculateTransfer expects the video buffer size to be
				//	set to the segment size, in bytes, which is one raster line length.
				outputXferField2.acDesiredFrame = outputXferField1.acTransferStatus.acTransferFrame;
				outputXferField2.SetVideoBuffer (pFrameData->VideoBuffer2(), mFormatDescriptor.GetBytesPerRow());
				mDevice.AutoCirculateTransfer (mOutputChannel, outputXferField2);
			}

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer();
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop (mOutputChannel);
	PLNOTE("Thread completed, will exit");

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
	AUTOCIRCULATE_TRANSFER	inputXferField1;	//	Field A/C input transfer info
	AUTOCIRCULATE_TRANSFER	inputXferField2;	//	Field 2 A/C input transfer info (unused in Field Mode)
	NTV2TCIndexes	F1TCIndexes, F2TCIndexes;
	ULWord xferTally(0), xferFails(0), waitTally(0);
	CAPNOTE("Thread started");

	//	Prepare the Timecode Indexes we'll be setting for playout...
	for (size_t ndx(0);  ndx < mTCOutputs.size();  ndx++)
	{	const NTV2Channel sdiSpigot(mTCOutputs.at(ndx));
		F1TCIndexes.insert(::NTV2ChannelToTimecodeIndex(sdiSpigot, /*LTC?*/true));			//	F1 LTC
		F1TCIndexes.insert(::NTV2ChannelToTimecodeIndex(sdiSpigot, /*LTC?*/false));			//	F1 VITC
		F2TCIndexes.insert(::NTV2ChannelToTimecodeIndex(sdiSpigot, /*LTC?*/false, true));	//	F2 VITC
	}

	if (!mIsFieldMode)
	{
		//	In Frame Mode, use AutoCirculate's "segmented DMA" feature to transfer each field
		//	out of the device's full-frame video buffer as a group of "segments".
		//	Each segment is one line long, and the segments are contiguous in host memory,
		//	but originate on alternating lines in the device's frame buffer...
		inputXferField1.EnableSegmentedDMAs (mFormatDescriptor.numLines / 2,		//	Number of segments:		number of lines per field, i.e. half the line count
												mFormatDescriptor.linePitch * 4,	//	Segment size, in bytes:	transfer this many bytes per segment (normal line pitch)
												mFormatDescriptor.linePitch * 4,	//	Host bytes per line:	normal line pitch when writing into our half-height buffer
												mFormatDescriptor.linePitch * 8);	//	Device bytes per line:	skip every other line when reading from device memory

		//	IMPORTANT:	For segmented DMAs, the video buffer size must contain the number of bytes to
		//				transfer per segment. This will be done just prior to calling AutoCirculateTransfer.
		inputXferField2.EnableSegmentedDMAs (mFormatDescriptor.numLines / 2,		//	Number of segments:		number of lines per field, i.e. half the line count
												mFormatDescriptor.linePitch * 4,	//	Segment size, in bytes:	transfer this many bytes per segment (normal line pitch)
												mFormatDescriptor.linePitch * 4,	//	Host bytes per line:	normal line pitch when writing into our half-height buffer
												mFormatDescriptor.linePitch * 8);	//	Device bytes per line:	skip every other line when reading from device memory
		inputXferField2.acInVideoDMAOffset	= mFormatDescriptor.linePitch * 4;		//  Field 2 starts on second line in device buffer
	}

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop (mInputChannel);

	//	Initialize AutoCirculate...
	mDevice.AutoCirculateInitForInput (mInputChannel, 7, mAudioSystem, AUTOCIRCULATE_WITH_RP188 | (mIsFieldMode ? AUTOCIRCULATE_WITH_FIELDS : 0));

	//	Start AutoCirculate running...
	mDevice.AutoCirculateStart (mInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS acStatus;
		mDevice.AutoCirculateGetStatus (mInputChannel, acStatus);

		if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
		{
			//	At this point, there's at least one fully-formed frame available in the device's
			//	frame buffer to transfer to the host. Reserve an NTV2FrameData to "produce", and
			//	use it in the next transfer from the device...
			NTV2FrameData *	pFrameData	(mAVCircularBuffer.StartProduceNextBuffer());

			inputXferField1.SetVideoBuffer (pFrameData->VideoBuffer(),
											mIsFieldMode	? pFrameData->VideoBufferSize()
															: mFormatDescriptor.GetBytesPerRow());
			if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
				inputXferField1.SetAudioBuffer (pFrameData->AudioBuffer(), NTV2_AUDIOSIZE_MAX);

			//	Transfer this Field (Field Mode) or F1 (Frame Mode) from the device into our host buffers...
			if (mDevice.AutoCirculateTransfer (mInputChannel, inputXferField1)) xferTally++;
			else xferFails++;

			//	Remember the audio byte count, which can vary frame-by-frame...
			pFrameData->fNumAudioBytes = NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem) ? inputXferField1.GetCapturedAudioByteCount() : 0;
			if (mIsFieldMode)
				pFrameData->fFrameFlags = inputXferField1.acPeerToPeerFlags;	//	Remember which field this was

			//	Obtain a timecode for this field/frame to burn into the captured field/frame...
			NTV2_RP188	theTimecode;
			NTV2TimeCodes	capturedTCs;
			inputXferField1.GetInputTimeCodes (capturedTCs, mInputChannel);	//	Valid Only
			if (!capturedTCs.empty())
			{
				theTimecode = capturedTCs.begin()->second;	//	Use 1st "good" timecode for burn-in
				CAPDBG("Captured TC: " << ::NTV2TCIndexToString(capturedTCs.begin()->first,true) << " " << theTimecode);
			}
			else
			{	//	Invent a timecode (based on frame count)...
				const NTV2FrameRate		frameRate	(::GetNTV2FrameRateFromVideoFormat(mVideoFormat));
				const TimecodeFormat	tcFormat	(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(frameRate));
				const ULWord			count		(inputXferField1.acTransferStatus.acFramesProcessed);
				const CRP188			inventedTC	(count / (mIsFieldMode ? 2 : 1), 0, 0, 10, tcFormat);
				inventedTC.GetRP188Reg(theTimecode);	//	Stuff it in the captureData
				CAPDBG("Invented TC: " << theTimecode);
			}
			CRP188 tc(theTimecode);
			string tcStr;
			tc.GetRP188Str(tcStr);

			if (!mIsFieldMode)
			{	//  Frame Mode: Transfer F2 segments from same device frame buffer used for F1...
				inputXferField2.acDesiredFrame = inputXferField1.acTransferStatus.acTransferFrame;
				inputXferField2.SetVideoBuffer (pFrameData->VideoBuffer2(), mFormatDescriptor.GetBytesPerRow());
				if (mDevice.AutoCirculateTransfer (mInputChannel, inputXferField2)) xferTally++;
				else xferFails++;
			}

			//	While this NTV2FrameData's buffers are locked, "burn" identical timecode into each field.
			//	F1 goes into top half, F2 into bottom half...
			mTCBurner.BurnTimeCode (ToCharPtr(inputXferField1.acVideoBuffer.GetHostPointer()), tcStr.c_str(),
									!mIsFieldMode || (pFrameData->fFrameFlags & AUTOCIRCULATE_FRAME_FIELD0) ? 10 : 30);
			if (!mIsFieldMode)	//	Frame Mode: "burn" F2 timecode
				mTCBurner.BurnTimeCode (ToCharPtr(inputXferField2.acVideoBuffer.GetHostPointer()), tcStr.c_str(), 30);

			//	Set NTV2FrameData::fTimecodes map for playout...
			for (NTV2TCIndexesConstIter it(F1TCIndexes.begin());  it != F1TCIndexes.end();  ++it)
				pFrameData->fTimecodes[*it] = theTimecode;
			for (NTV2TCIndexesConstIter it(F2TCIndexes.begin());  it != F2TCIndexes.end();  ++it)
				pFrameData->fTimecodes[*it] = theTimecode;

			//	Signal that we're done "producing" the frame, making it available for future "consumption"...
			mAVCircularBuffer.EndProduceNextBuffer();
		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a field/frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			mDevice.WaitForInputVerticalInterrupt(mInputChannel);
			++waitTally;
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mInputChannel);
	CAPNOTE("Thread completed, " << DEC(xferTally) << " of " << DEC(xferTally+xferFails) << " frms xferred, "
			<< DEC(waitTally) << " wait(s)");

}	//	CaptureFrames


//////////////////////////////////////////////


void NTV2FieldBurn::GetStatus (ULWord & outFramesProcessed, ULWord & outCaptureFramesDropped, ULWord & outPlayoutFramesDropped,
								ULWord & outCaptureBufferLevel, ULWord & outPlayoutBufferLevel)
{
	AUTOCIRCULATE_STATUS	inputStatus,  outputStatus;

	mDevice.AutoCirculateGetStatus (mInputChannel, inputStatus);
	mDevice.AutoCirculateGetStatus (mOutputChannel, outputStatus);

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
	const ULWord	regNum		(::GetRP188RegisterForInput (mInputSource));
	ULWord			regValue	(0);

	//	Bit 16 of the RP188 DBB register will be set if there is timecode embedded in the input signal...
	if (regNum  &&  mDevice.ReadRegister(regNum, regValue)  &&  regValue & BIT(16))
		return true;
	return false;

}	//	InputSignalHasTimecode
