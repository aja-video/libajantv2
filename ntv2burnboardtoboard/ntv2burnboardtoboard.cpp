/**
	@file		ntv2burnboardtoboard.cpp
	@brief		Implementation of NTV2Burn demonstration class.
	@copyright	Copyright (C) 2012-2018 AJA Video Systems, Inc.  All rights reserved.
**/
#define _WINSOCKAPI_
#include "ntv2burnboardtoboard.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ajabase/common/types.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include <iostream>

#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define NTV2_ANCSIZE_MAX	(0x2000)

const uint32_t	kAppSignature	(AJA_FOURCC ('B','u','r','n'));


//////////////////////	IMPLEMENTATION


NTV2BurnBoardToBoard::NTV2BurnBoardToBoard (const string &				inDeviceSpecifier,
					const string &				outDeviceSpecifier,
					const bool					inWithAudio,
					const bool					disableBurn ,
					const NTV2FrameBufferFormat	inPixelFormat,
					const NTV2InputSource		inInputSource,
					const bool					inDoMultiFormat,
					const NTV2TCIndex			inTCSource,
					const bool					inWithAnc)

	:	mPlayThread			(NULL),
		mCaptureThread		(NULL),
		mLock				(new AJALock (CNTV2DemoCommon::GetGlobalMutexName ())),
		mInDeviceID			(DEVICE_ID_NOTFOUND),
		mInDeviceSpecifier(inDeviceSpecifier),
		mOutDeviceSpecifier(outDeviceSpecifier),
		mInputChannel(NTV2_CHANNEL_INVALID),
		mOutputChannel		(NTV2_CHANNEL_INVALID),
		mInputSource		(inInputSource),
		mOutputDestination	(NTV2_OUTPUTDESTINATION_INVALID),
		mVideoFormat		(NTV2_FORMAT_UNKNOWN),
		mPixelFormat		(inPixelFormat),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mVancEnabled		(false),
		mWideVanc			(false),
		mAudioSystem		(inWithAudio ? ::NTV2InputSourceToAudioSystem (inInputSource) : NTV2_AUDIOSYSTEM_INVALID),
		mDisableBurn		(disableBurn),
		mGlobalQuit			(false),
		mDoMultiChannel		(inDoMultiFormat),
		mVideoBufferSize	(0),
		mTCSource			(inTCSource),
		mWithAnc			(inWithAnc)
{
	::memset (mAVHostBuffer, 0, sizeof (mAVHostBuffer));

}	//	constructor


NTV2BurnBoardToBoard::~NTV2BurnBoardToBoard ()
{
	//	Stop my capture and playout threads, then destroy them...
	Quit ();

	delete mPlayThread;
	mPlayThread = NULL;

	delete mCaptureThread;
	mCaptureThread = NULL;

	delete mLock;
	mLock = NULL;

	//	Unsubscribe from input vertical event...
	mInDevice.UnsubscribeInputVerticalEvent (mInputChannel);

	//	Free all my buffers...
	for (unsigned bufferNdx = 0;  bufferNdx < CIRCULAR_BUFFER_SIZE;  bufferNdx++)
	{
		if (mAVHostBuffer[bufferNdx].fVideoBuffer)
		{
			AJAMemory::FreeAligned (mAVHostBuffer[bufferNdx].fVideoBuffer);
			mAVHostBuffer[bufferNdx].fVideoBuffer = NULL;
		}
		if (mAVHostBuffer[bufferNdx].fAudioBuffer)
		{
			AJAMemory::FreeAligned (mAVHostBuffer[bufferNdx].fAudioBuffer);
			mAVHostBuffer[bufferNdx].fAudioBuffer = NULL;
		}
	}	//	for each buffer in the ring

	if (!mDoMultiChannel)
	{
		mInDevice.SetEveryFrameServices (mSavedTaskMode);										//	Restore prior service level
		mInDevice.ReleaseStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ()));	//	Release the device
	}

}	//	destructor


void NTV2BurnBoardToBoard::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	if (mPlayThread)
		while (mPlayThread->Active ())
			AJATime::Sleep (10);

	if (mCaptureThread)
		while (mCaptureThread->Active ())
			AJATime::Sleep (10);

}	//	Quit


AJAStatus NTV2BurnBoardToBoard::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the input(capture) device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mInDeviceSpecifier, mInDevice))
		{cerr << "## ERROR:  Input Device '" << mInDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mInDevice.GetStreamingApplication (&appSignature, &appPID);	//	Who currently "owns" the device?
	mInDevice.GetEveryFrameServices (mSavedTaskMode);				//	Save the current device state
	if (!mDoMultiChannel)
	{
		if (!mInDevice.AcquireStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mInDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Set the OEM service level
		mInDevice.ClearRouting ();								//	Clear the current device routing (since I "own" the device)
	}
	else
		mInDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Force OEM tasks

	mInDeviceID = mInDevice.GetDeviceID ();							//	Keep the device ID handy since it will be used frequently


	//	Open the input(capture) device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(mOutDeviceSpecifier, mOutDevice))
	{
		cerr << "## ERROR:  Output Device '" << mOutDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;
	}

	mOutDevice.GetStreamingApplication(&appSignature, &appPID);	//	Who currently "owns" the device?
	mOutDevice.GetEveryFrameServices(mSavedTaskMode);				//	Save the current device state
	if (!mDoMultiChannel)
	{
		if (!mOutDevice.AcquireStreamForApplication(kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mOutDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Set the OEM service level
		mOutDevice.ClearRouting();								//	Clear the current device routing (since I "own" the device)
	}
	else
		mOutDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Force OEM tasks

	mOutDeviceID = mOutDevice.GetDeviceID();							//	Keep the device ID handy since it will be used frequently



	//	Make sure the device actually supports custom anc before using it...
	if (mWithAnc)
		mWithAnc = ::NTV2DeviceCanDoCustomAnc (mInDeviceID);

	//	Set up the video and audio...
	status = SetupVideo ();
	if (AJA_SUCCESS (status))
		status = SetupAudio ();

	//	Set up the circular buffers...
	if (AJA_SUCCESS (status))
		status = SetupHostBuffers ();

	//	Set up the signal routing...
	if (AJA_SUCCESS (status))
		RouteInputSignal ();
	if (AJA_SUCCESS (status))
		RouteOutputSignal ();

	//	Lastly, prepare my AJATimeCodeBurn instance...
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mPixelFormat), mFormatDescriptor.numPixels, mFormatDescriptor.numLines);

	return status;

}	//	Init


AJAStatus NTV2BurnBoardToBoard::SetupVideo (void)
{
	const UWord	numFrameStores	(::NTV2DeviceGetNumFrameStores (mInDeviceID));

	//	Does this device have the requested input source?
	if (!::NTV2DeviceCanDoInputSource (mInDeviceID, mInputSource))
		{cerr << "## ERROR:  Device does not have the specified input source" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Pick an input NTV2Channel from the input source, and enable its frame buffer...
	mInputChannel = NTV2_INPUT_SOURCE_IS_SDI (mInputSource) ? ::NTV2InputSourceToChannel (mInputSource) : NTV2_CHANNEL2;	//	NTV2_CHANNEL1;
	mInDevice.EnableChannel (mInputChannel);		//	Enable the input frame buffer

	//	Pick an appropriate output NTV2Channel, and enable its frame buffer...
	switch (mInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		mOutputChannel = NTV2_CHANNEL1;			break;
		case NTV2_INPUTSOURCE_SDI2:		mOutputChannel = NTV2_CHANNEL2;			break;
		case NTV2_INPUTSOURCE_SDI3:		mOutputChannel = NTV2_CHANNEL3;			break;
		case NTV2_INPUTSOURCE_SDI4:		mOutputChannel = NTV2_CHANNEL4;			break;
		case NTV2_INPUTSOURCE_SDI5: 	mOutputChannel = NTV2_CHANNEL5;			break;
		case NTV2_INPUTSOURCE_SDI6:		mOutputChannel = NTV2_CHANNEL6;			break;
		case NTV2_INPUTSOURCE_SDI7:		mOutputChannel = NTV2_CHANNEL7;			break;
		case NTV2_INPUTSOURCE_SDI8:		mOutputChannel = NTV2_CHANNEL8;			break;

		default:
		case NTV2_INPUTSOURCE_INVALID:	cerr << "## ERROR:  Bad input source" << endl;  return AJA_STATUS_BAD_PARAM;
	}
	mOutDevice.EnableChannel (mOutputChannel);		//	Enable the output frame buffer

	//	Pick an appropriate output spigot based on the output channel...
	mOutputDestination	= ::NTV2ChannelToOutputDestination (mOutputChannel);

	//	Flip the input spigot to "receive" if necessary...
	bool	isTransmit	(false);
	if (::NTV2DeviceHasBiDirectionalSDI (mInDevice.GetDeviceID ())			//	If device has bidirectional SDI connectors...
		&& NTV2_INPUT_SOURCE_IS_SDI (mInputSource)							//	...and desired input source is SDI...
			&& mInDevice.GetSDITransmitEnable (mInputChannel, &isTransmit)	//	...and GetSDITransmitEnable succeeds...
				&& isTransmit)												//	...and input is set to "transmit"...
	{
		mInDevice.SetSDITransmitEnable (mInputChannel, false);				//	...then disable transmit mode...
		AJATime::Sleep (500);												//	...and give the device a dozen frames or so to lock to the input signal
	}	//	if input SDI connector needs to switch from transmit mode

	if (::NTV2DeviceHasBiDirectionalSDI(mInDeviceID))
		mOutDevice.SetSDITransmitEnable(mOutputChannel, true);

	//	Is there an input signal?  What format is it?
	mVideoFormat = mInDevice.GetInputVideoFormat (mInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		{cerr << "## ERROR:  No input signal, or can't handle its format" << endl;  return AJA_STATUS_NOINPUT;}

	//	Free-run the device clock, since E-to-E mode isn't used, nor is a mixer tied to the input...
	mInDevice.SetReference (NTV2_REFERENCE_FREERUN);
#if 0
	//	Check the timecode source...
	if (NTV2_IS_SDI_TIMECODE_INDEX (mTCSource))
	{
		const NTV2Channel	tcChannel	(::NTV2TimecodeIndexToChannel (mTCSource));
		const NTV2Channel	endNum		(NTV2Channel (::NTV2DeviceGetNumVideoChannels (mInDeviceID)));
		if (tcChannel >= endNum)
			{cerr << "## ERROR:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' illegal on this device" << endl;  return AJA_STATUS_BAD_PARAM;}
		if (::NTV2DeviceHasBiDirectionalSDI (mInDevice.GetDeviceID ())	//	If device has bidirectional SDI connectors...
			&& mInDevice.GetSDITransmitEnable (tcChannel, &isTransmit)	//	...and GetSDITransmitEnable succeeds...
				&& isTransmit)											//	...and the SDI timecode source is set to "transmit"...
		{
			mInDevice.SetSDITransmitEnable (tcChannel, false);			//	...then disable transmit mode...
			AJATime::Sleep (500);										//	...and give the device a dozen frames or so to lock to the input signal
		}	//	if input SDI connector needs to switch from transmit mode

		const NTV2VideoFormat	tcInputVideoFormat	(mInDevice.GetInputVideoFormat (::NTV2TimecodeIndexToInputSource (mTCSource)));
		if (tcInputVideoFormat == NTV2_FORMAT_UNKNOWN)
			cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' has no input signal" << endl;
		if (!InputSignalHasTimecode ())
			cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' has no embedded timecode" << endl;
	}
	else if (NTV2_IS_ANALOG_TIMECODE_INDEX (mTCSource) && !AnalogLTCInputHasTimecode ())
		cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' has no embedded timecode" << endl;
#endif
	//	If the device supports different per-channel video formats, configure it as requested...
	if (::NTV2DeviceCanDoMultiFormat (mInDeviceID))
		mInDevice.SetMultiFormatMode (mDoMultiChannel);

	//	If the device supports different per-channel video formats, configure it as requested...
	if (::NTV2DeviceCanDoMultiFormat(mOutDeviceID))
		mOutDevice.SetMultiFormatMode(mDoMultiChannel);

	//	Set the input/output channel video formats to the video format that was detected earlier...
	mInDevice.SetVideoFormat (mVideoFormat, false, false, ::NTV2DeviceCanDoMultiFormat (mInDeviceID) ? mInputChannel : NTV2_CHANNEL1);
	if (::NTV2DeviceCanDoMultiFormat (mOutDeviceID))									//	If device supports multiple formats per-channel...
		mOutDevice.SetVideoFormat (mVideoFormat, false, false, mOutputChannel);		//	...then also set the output channel format to the detected input format

	//	Can the device handle the requested frame buffer pixel format?
	if (!::NTV2DeviceCanDoFrameBufferFormat (mInDeviceID, mPixelFormat))
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;		//	Fall back to 8-bit YCbCr

	//	Set both input and output frame buffers' pixel formats...
	mInDevice.SetFrameBufferFormat (mInputChannel, mPixelFormat);
	mOutDevice.SetFrameBufferFormat (mOutputChannel, mPixelFormat);

	//	Enable and subscribe to the input interrupts...
	mInDevice.EnableInputInterrupt (mInputChannel);
	mInDevice.SubscribeInputVerticalEvent (mInputChannel);

	//	Enable and subscribe to the output interrupts...
	mInDevice.EnableOutputInterrupt (mOutputChannel);
	mInDevice.SubscribeOutputVerticalEvent (mOutputChannel);

	//	Enable and subscribe to the output interrupts...
	mOutDevice.EnableOutputInterrupt(mOutputChannel);
	mOutDevice.SubscribeOutputVerticalEvent(mOutputChannel);

	//	Normally, timecode embedded in the output signal comes from whatever is written into the RP188
	//	registers (30/31 for SDI out 1, 65/66 for SDIout2, etc.).
	//	AutoCirculate automatically writes the timecode in the AUTOCIRCULATE_TRANSFER's acRP188 field
	//	into these registers (if AutoCirculateInitForOutput was called with AUTOCIRCULATE_WITH_RP188 set).
	//	Newer AJA devices can also bypass these RP188 registers, and simply copy whatever timecode appears
	//	at any SDI input (called the "bypass source"). To ensure that AutoCirculate's playout timecode
	//	will actually be seen in the output signal, "bypass mode" must be disabled...
	bool	bypassIsEnabled	(false);
	mInDevice.IsRP188BypassEnabled (::NTV2InputSourceToChannel (mInputSource), bypassIsEnabled);
	if (bypassIsEnabled)
		mInDevice.DisableRP188Bypass (::NTV2InputSourceToChannel (mInputSource));


	if (NTV2_IS_ANALOG_TIMECODE_INDEX (mTCSource))
		mInDevice.SetLTCInputEnable (true);	//	Enable analog LTC input (some LTC inputs are shared with reference input)

	//	Now that the video is set up, get information about the current frame geometry...
	mFormatDescriptor = GetFormatDescriptor (::GetNTV2StandardFromVideoFormat (mVideoFormat),
											mPixelFormat,
											mVancEnabled,
											Is2KFormat (mVideoFormat),
											mWideVanc);
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2BurnBoardToBoard::SetupAudio (void)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
		return AJA_STATUS_SUCCESS;

	//	Have the audio subsystem capture audio from the designated input source...
	mInDevice.SetAudioSystemInputSource (mAudioSystem, ::NTV2InputSourceToAudioSource (mInputSource), ::NTV2InputSourceToEmbeddedAudioInput (mInputSource));

	//	It's best to use all available audio channels...
	mInDevice.SetNumberAudioChannels (::NTV2DeviceGetMaxAudioChannels (mInDeviceID), mAudioSystem);
	mOutDevice.SetNumberAudioChannels(::NTV2DeviceGetMaxAudioChannels(mInDeviceID), mAudioSystem);

	//	Assume 48kHz PCM...
	mInDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);
	mOutDevice.SetAudioRate(NTV2_AUDIO_48K, mAudioSystem);

	//	4MB device audio buffers work best...
	mInDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);
	mOutDevice.SetAudioBufferSize(NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set up the output audio embedders...
	if (::NTV2DeviceGetNumAudioSystems (mOutDeviceID) > 1)
		mOutDevice.SetSDIOutputAudioSystem (mOutputChannel, mAudioSystem);

	//
	//	Loopback mode plays whatever audio appears in the input signal when it's
	//	connected directly to an output (i.e., "end-to-end" mode). If loopback is
	//	left enabled, the video will lag the audio as video frames get briefly delayed
	//	in our ring buffer. Audio, therefore, needs to come out of the (buffered) frame
	//	data being played, so loopback must be turned off...
	//
	mInDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);
	mOutDevice.SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


AJAStatus NTV2BurnBoardToBoard::SetupHostBuffers (void)
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	mVideoBufferSize = GetVideoWriteSize (mVideoFormat, mPixelFormat, mVancEnabled, mWideVanc);

	//	Allocate and add each in-host AVDataBuffer to my circular buffer member variable.
	//	Note that DMA performance can be accelerated slightly by using page-aligned video buffers...
	for (unsigned bufferNdx (0);  bufferNdx < CIRCULAR_BUFFER_SIZE;  bufferNdx++)
	{
		//	Allocate full-frame video frame buffer...
		mAVHostBuffer [bufferNdx].fVideoBuffer = reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mVideoBufferSize, AJA_PAGE_SIZE));
		mAVHostBuffer [bufferNdx].fVideoBufferSize = mVideoBufferSize;

		mAVHostBuffer [bufferNdx].fAncBuffer = mWithAnc ? reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (NTV2_ANCSIZE_MAX, AJA_PAGE_SIZE)) : NULL;
		mAVHostBuffer [bufferNdx].fAncBufferSize = mWithAnc ? NTV2_ANCSIZE_MAX : 0;

		mAVHostBuffer [bufferNdx].fAncF2Buffer = mWithAnc ? reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (NTV2_ANCSIZE_MAX, AJA_PAGE_SIZE)) : NULL;
		mAVHostBuffer [bufferNdx].fAncF2BufferSize = mWithAnc ? NTV2_ANCSIZE_MAX : 0;

		//	Allocate audio buffer (unless --noaudio requested)...
		if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
		{
			mAVHostBuffer [bufferNdx].fAudioBuffer		= reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (NTV2_AUDIOSIZE_MAX, AJA_PAGE_SIZE));
			mAVHostBuffer [bufferNdx].fAudioBufferSize	= NTV2_AUDIOSIZE_MAX;
		}

		//	Add it to my circular buffer...
		mAVCircularBuffer.Add (& mAVHostBuffer [bufferNdx]);

		//	Check for memory allocation failures...
		if (!mAVHostBuffer[bufferNdx].fVideoBuffer
			|| (mWithAnc && !mAVHostBuffer[bufferNdx].fAncBuffer && !mAVHostBuffer[bufferNdx].fAncF2Buffer)
			|| (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem) && !mAVHostBuffer[bufferNdx].fAudioBuffer))
				{
					cerr << "## ERROR:  Allocation failed:  buffer " << (bufferNdx + 1) << " of " << CIRCULAR_BUFFER_SIZE << endl;
					return AJA_STATUS_MEMORY;
				}
	}	//	for each AVDataBuffer

	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2BurnBoardToBoard::RouteInputSignal (void)
{
	const NTV2OutputCrosspointID	inputOutputXpt	(::GetInputSourceOutputXpt (mInputSource));
	const NTV2InputCrosspointID		fbInputXpt		(::GetFrameBufferInputXptFromChannel (mInputChannel));

	if (::IsRGBFormat (mPixelFormat))
	{
		//	If the frame buffer is configured for RGB pixel format, incoming YUV must be converted.
		//	This routes the video signal from the input through a color space converter before
		//	connecting to the RGB frame buffer...
		const NTV2InputCrosspointID		cscVideoInputXpt	(::GetCSCInputXptFromChannel (mInputChannel));
		const NTV2OutputCrosspointID	cscOutputXpt		(::GetCSCOutputXptFromChannel (mInputChannel, false, true));	//	false=video, true=RGB

		mInDevice.Connect (cscVideoInputXpt, inputOutputXpt);	//	Connect the CSC's video input to the input spigot's output
		mInDevice.Connect (fbInputXpt, cscOutputXpt);			//	Connect the frame store's input to the CSC's output
	}
	else
		mInDevice.Connect (fbInputXpt, inputOutputXpt);		//	Route the YCbCr signal directly from the input to the frame buffer's input

}	//	RouteInputSignal


void NTV2BurnBoardToBoard::RouteOutputSignal (void)
{
	const NTV2InputCrosspointID		outputInputXpt	(::GetOutputDestInputXpt (mOutputDestination));
	const NTV2OutputCrosspointID	fbOutputXpt		(::GetFrameBufferOutputXptFromChannel (mOutputChannel, ::IsRGBFormat (mPixelFormat)));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

	if (::IsRGBFormat (mPixelFormat))
	{
		const NTV2OutputCrosspointID	cscVidOutputXpt	(::GetCSCOutputXptFromChannel (mOutputChannel, false, true));
		const NTV2InputCrosspointID		cscVidInputXpt	(::GetCSCInputXptFromChannel (mOutputChannel));

		mOutDevice.Connect (cscVidInputXpt, fbOutputXpt);		//	Connect the CSC's video input to the frame store's output
		mOutDevice.Connect(outputInputXpt, cscVidOutputXpt);	//	Connect the SDI output's input to the CSC's video output
		outputXpt = cscVidOutputXpt;
	}
	else
		mOutDevice.Connect(outputInputXpt, outputXpt);

	mTCOutputs.clear ();
	mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (mOutputChannel));
#if 0
	if (!mDoMultiChannel)
	{
		//	Route all SDI outputs to the outputXpt...
		const NTV2Channel	startNum		(NTV2_CHANNEL1);
		const NTV2Channel	endNum			(NTV2Channel (::NTV2DeviceGetNumVideoChannels (mInDeviceID)));
		const NTV2Channel	tcInputChannel	(NTV2_IS_SDI_TIMECODE_INDEX (mTCSource) ? ::NTV2TimecodeIndexToChannel (mTCSource) : NTV2_CHANNEL_INVALID);
		NTV2WidgetID		outputWidgetID	(NTV2_WIDGET_INVALID);

		for (NTV2Channel chan (startNum);  chan < endNum;  chan = NTV2Channel (chan + 1))
		{
			mInDevice.SetRP188Source (chan, 0);	//	Set all SDI spigots to capture embedded LTC (VITC could be an option)

			if (chan == mInputChannel || chan == mOutputChannel)
				continue;	//	Skip the input & output channel, already routed
			if (NTV2_IS_VALID_CHANNEL (tcInputChannel) && chan == tcInputChannel)
				continue;	//	Skip the timecode input channel
			if (::NTV2DeviceHasBiDirectionalSDI (mInDeviceID))
				mOutDevice.SetSDITransmitEnable (chan, true);
			if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, ::NTV2DeviceCanDoDualLink (mOutDeviceID)), outputWidgetID))
				if (::NTV2DeviceCanDoWidget (mOutDeviceID, outputWidgetID))
				{
					mInDevice.Connect (::GetSDIOutputInputXpt (chan), outputXpt);
					mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan));
					mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan, true));
				}
		}	//	for each output spigot

		//	If HDMI and/or analog video outputs are available, route them, too...
		if (::NTV2DeviceCanDoWidget (mInDeviceID, NTV2_WgtHDMIOut1))
			mOutDevice.Connect(NTV2_XptHDMIOutInput, outputXpt);			//	Route the output signal to the HDMI output
		if (::NTV2DeviceCanDoWidget (mInDeviceID, NTV2_WgtHDMIOut1v2))
			mOutDevice.Connect(NTV2_XptHDMIOutQ1Input, outputXpt);		//	Route the output signal to the HDMI output
		if (::NTV2DeviceCanDoWidget (mInDeviceID, NTV2_WgtAnalogOut1))
			mOutDevice.Connect(NTV2_XptAnalogOutInput, outputXpt);		//	Route the output signal to the Analog output
		if (::NTV2DeviceCanDoWidget (mInDeviceID, NTV2_WgtSDIMonOut1))
			mOutDevice.Connect(::GetSDIOutputInputXpt(NTV2_CHANNEL5), outputXpt);	//	Route the output signal to the SDI monitor output
	}
	cerr << "## DEBUG:  " << mTCOutputs.size () << " timecode destination(s):  " << mTCOutputs << endl;
#endif
}	//	RouteOutputSignal


AJAStatus NTV2BurnBoardToBoard::Run ()
{
	//	Start the playout and capture threads...
	StartPlayThread ();
	StartCaptureThread ();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we will start the play thread
void NTV2BurnBoardToBoard::StartPlayThread (void)
{
	//	Create and start the playout thread...
	mPlayThread = new AJAThread ();
	mPlayThread->Attach (PlayThreadStatic, this);
	mPlayThread->SetPriority (AJA_ThreadPriority_High);
	mPlayThread->Start ();

}	//	StartPlayThread


//	The playout thread function
void NTV2BurnBoardToBoard::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	//	Grab the NTV2Burn instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2BurnBoardToBoard *	pApp	(reinterpret_cast <NTV2BurnBoardToBoard *> (pContext));
	pApp->PlayFrames ();

}	//	PlayThreadStatic


void NTV2BurnBoardToBoard::PlayFrames (void)
{
	AUTOCIRCULATE_TRANSFER	outputXferInfo;	//	A/C output transfer info

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mOutDevice.AutoCirculateStop (mOutputChannel);

	//	Initialize the AutoCirculate output channel...
	{
		AJAAutoLock	autoLock (mLock);	//	Avoid AutoCirculate buffer collisions
		mOutDevice.AutoCirculateInitForOutput(mOutputChannel, 7, mAudioSystem, AUTOCIRCULATE_WITH_RP188 | (mWithAnc ? AUTOCIRCULATE_WITH_ANC : 0));
	}

	//	Start AutoCirculate running...
	mOutDevice.AutoCirculateStart(mOutputChannel);

	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		AVDataBuffer *	playData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (playData)
		{
			//	Prepare to transfer this timecode-burned frame to the device for playout.
			//	Set the XferInfo struct's video, audio and anc buffers from playData's buffers...
			outputXferInfo.SetVideoBuffer (playData->fVideoBuffer, playData->fVideoBufferSize);
			if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
				outputXferInfo.SetAudioBuffer (playData->fAudioBuffer, playData->fAudioBufferSize);
			if (mWithAnc)
				outputXferInfo.SetAncBuffers (playData->fAncBuffer, playData->fAncBufferSize, playData->fAncF2Buffer, playData->fAncF2BufferSize);

			//	Tell AutoCirculate to embed this frame's timecode into the SDI output.
			//	To embed this same timecode into other SDI outputs, set the appropriate members of the acOutputTimeCodes array...
			for (NTV2TCIndexesConstIter iter (mTCOutputs.begin ());  iter != mTCOutputs.end ();  ++iter)
				outputXferInfo.SetOutputTimeCode (NTV2_RP188 (playData->fRP188Data), *iter);

			//	Transfer the frame to the device for eventual playout...
			mOutDevice.AutoCirculateTransfer(mOutputChannel, outputXferInfo);

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer ();
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mOutDevice.AutoCirculateStop(mOutputChannel);

}	//	PlayFrames


//////////////////////////////////////////////



//////////////////////////////////////////////
//
//	This is where the capture thread gets started
//
void NTV2BurnBoardToBoard::StartCaptureThread (void)
{
	//	Create and start the capture thread...
	mCaptureThread = new AJAThread ();
	mCaptureThread->Attach (CaptureThreadStatic, this);
	mCaptureThread->SetPriority (AJA_ThreadPriority_High);
	mCaptureThread->Start ();

}	//	StartCaptureThread


//
//	The static capture thread function
//
void NTV2BurnBoardToBoard::CaptureThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	//	Grab the NTV2Burn instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2BurnBoardToBoard *	pApp	(reinterpret_cast <NTV2BurnBoardToBoard *> (pContext));
	pApp->CaptureFrames ();

}	//	CaptureThreadStatic


//
//	Repeatedly captures frames until told to stop
//
void NTV2BurnBoardToBoard::CaptureFrames (void)
{
	AUTOCIRCULATE_TRANSFER	inputXferInfo;		//	A/C input transfer info
	Bouncer					yPercent	(85/*upperLimit*/, 1/*lowerLimit*/, 1/*startValue*/);	//	Used to "bounce" timecode up & down in raster

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mInDevice.AutoCirculateStop (mInputChannel);

	//	Initialize AutoCirculate...
	{
		AJAAutoLock	autoLock (mLock);	//	Avoid AutoCirculate buffer collisions
		mInDevice.AutoCirculateInitForInput (mInputChannel, 7, mAudioSystem,
											(NTV2_IS_VALID_TIMECODE_INDEX (mTCSource) ? AUTOCIRCULATE_WITH_RP188 : 0)  |  (mWithAnc ? AUTOCIRCULATE_WITH_ANC : 0));
	}

	//	Start AutoCirculate running...
	mInDevice.AutoCirculateStart (mInputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS	acStatus;
		mInDevice.AutoCirculateGetStatus (mInputChannel, acStatus);

		if (::NTV2DeviceHasSDIRelays (mInDeviceID))
			mInDevice.KickSDIWatchdog ();		//	Prevent watchdog from timing out and putting the relays into bypass mode

		if (acStatus.IsRunning ()  &&  acStatus.HasAvailableInputFrame ())
		{
			//	At this point, there's at least one fully-formed frame available in the device's
			//	frame buffer to transfer to the host. Reserve an AVDataBuffer to "produce", and
			//	use it in the next transfer from the device...
			AVDataBuffer *	captureData	(mAVCircularBuffer.StartProduceNextBuffer ());

			inputXferInfo.SetVideoBuffer (captureData->fVideoBuffer, captureData->fVideoBufferSize);
			if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
				inputXferInfo.SetAudioBuffer (captureData->fAudioBuffer, captureData->fAudioBufferSize);
			if (mWithAnc)
				inputXferInfo.SetAncBuffers (captureData->fAncBuffer, NTV2_ANCSIZE_MAX, captureData->fAncF2Buffer, NTV2_ANCSIZE_MAX);

			//	Transfer the frame from the device into our host AVDataBuffer...
			mInDevice.AutoCirculateTransfer (mInputChannel, inputXferInfo);

			//	Remember the audio & anc data byte counts...
			captureData->fAudioBufferSize	= NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem)  ?  inputXferInfo.GetCapturedAudioByteCount ()  :  0;
			captureData->fAncBufferSize		= mWithAnc  ?  inputXferInfo.GetAncByteCount (false/*F1*/)  :  0;
			captureData->fAncF2BufferSize	= mWithAnc  ?  inputXferInfo.GetAncByteCount ( true/*F2*/)  :  0;

			NTV2_RP188	defaultTC;
			if (NTV2_IS_VALID_TIMECODE_INDEX (mTCSource) && InputSignalHasTimecode ())
			{
				//	Use the timecode that was captured by AutoCirculate...
				inputXferInfo.GetInputTimeCode (defaultTC, mTCSource);
			}
			if (defaultTC.IsValid ())
				captureData->fRP188Data	= defaultTC;	//	Stuff it in the captureData
			else
			{
				//	Invent a timecode (based on frame count)...
				const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
				const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
				const	CRP188			inventedTC		(inputXferInfo.acTransferStatus.acFramesProcessed, 0, 0, 10, tcFormat);
				inventedTC.GetRP188Reg (captureData->fRP188Data);	//	Stuff it in the captureData
				//cerr << "## DEBUG:  InventedTC: " << inventedTC << "\r";
			}
			CRP188	tc	(captureData->fRP188Data);
			string	tcStr;
			tc.GetRP188Str (tcStr);

			//	"Burn" the timecode into the host AVDataBuffer while it's locked for our exclusive access...
			if ( !mDisableBurn)
				mTCBurner.BurnTimeCode (reinterpret_cast <char *> (inputXferInfo.acVideoBuffer.GetHostPointer ()), tcStr.c_str (), yPercent.Next ());

			//	Signal that we're done "producing" the frame, making it available for future "consumption"...
			mAVCircularBuffer.EndProduceNextBuffer ();
		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			mInDevice.WaitForInputVerticalInterrupt (mInputChannel);
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mInDevice.AutoCirculateStop (mInputChannel);

}	//	CaptureFrames


//////////////////////////////////////////////


void NTV2BurnBoardToBoard::GetStatus (ULWord & outFramesProcessed, ULWord & outCaptureFramesDropped, ULWord & outPlayoutFramesDropped,
	ULWord & outCaptureBufferLevel, ULWord & outPlayoutBufferLevel, NTV2VideoFormat &inputVideoFormat)
{
	AUTOCIRCULATE_STATUS	inputStatus,  outputStatus;

	mInDevice.AutoCirculateGetStatus (mInputChannel, inputStatus);
	mOutDevice.AutoCirculateGetStatus (mOutputChannel, outputStatus);

	outFramesProcessed		= inputStatus.acFramesProcessed;
	outCaptureFramesDropped	= inputStatus.acFramesDropped;
	outPlayoutFramesDropped	= outputStatus.acFramesDropped;
	outCaptureBufferLevel	= inputStatus.acBufferLevel;
	outPlayoutBufferLevel	= outputStatus.acBufferLevel;

	inputVideoFormat = mInDevice.GetSDIInputVideoFormat(mInputChannel);


}	//	GetStatus


static ULWord GetRP188RegisterForInput (const NTV2InputSource inInputSource)
{
	switch (inInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		return kRegRP188InOut1DBB;	break;	//	reg 29
		case NTV2_INPUTSOURCE_SDI2:		return kRegRP188InOut2DBB;	break;	//	reg 64
		case NTV2_INPUTSOURCE_SDI3:		return kRegRP188InOut3DBB;	break;	//	reg 268
		case NTV2_INPUTSOURCE_SDI4:		return kRegRP188InOut4DBB;	break;	//	reg 273
		case NTV2_INPUTSOURCE_SDI5:		return kRegRP188InOut5DBB;	break;	//	reg 342
		case NTV2_INPUTSOURCE_SDI6:		return kRegRP188InOut6DBB;	break;	//	reg 418
		case NTV2_INPUTSOURCE_SDI7:		return kRegRP188InOut7DBB;	break;	//	reg 427
		case NTV2_INPUTSOURCE_SDI8:		return kRegRP188InOut8DBB;	break;	//	reg 436
		default:						return 0;					break;
	}	//	switch on input source

}	//	GetRP188RegisterForInput


bool NTV2BurnBoardToBoard::InputSignalHasTimecode (void)
{
	const ULWord	regNum		(::GetRP188RegisterForInput (mInputSource));
	ULWord			regValue	(0);

	//	Bit 16 of the RP188 DBB register will be set if there is timecode embedded in the input signal...
	if (regNum && mInDevice.ReadRegister (regNum, &regValue) && regValue & BIT(16))
		return true;
	return false;

}	//	InputSignalHasTimecode


bool NTV2BurnBoardToBoard::AnalogLTCInputHasTimecode (void)
{
	ULWord	regMask		(0);
	ULWord	regValue	(0);
	switch (mTCSource)
	{
		case NTV2_TCINDEX_LTC1:		regMask = kRegMaskLTC1InPresent;	break;
		case NTV2_TCINDEX_LTC2:		regMask = kRegMaskLTC2InPresent;	break;
		default:					return false;						break;
	}
	mInDevice.ReadRegister (kRegLTCStatusControl, &regValue, regMask);
	return regValue ? true : false;

}	//	AnalogLTCInputHasTimecode
