/**
	@file		qaburn.cpp
	@brief		Implementation of QABurn class.
	@copyright	Copyright (C) 2012-2014 AJA Video Systems, Inc.  All rights reserved.
**/

#include "qaburn.h"
#include "ajabase/common/types.h"
#include "ajabase/system/memory.h"
#include <iostream>


#define NTV2_AUDIOSIZE_MAX	(401 * 1024)


QABurn::QABurn (const std::string &			inDeviceSpecifier,
				const NTV2InputSource		inInputSource,
				const NTV2AudioSystem		inAudioSystem,
				const NTV2FrameBufferFormat	inPixelFormat,
				const bool					inWithBurn,
				const bool					inDoLevelConversion,
				const bool					inUseDualLink)

	:	mPlayThread				(NULL),
		mCaptureThread			(NULL),
		mLock					(new AJALock),
		mDeviceID				(DEVICE_ID_NOTFOUND),
		mDeviceSpecifier		(inDeviceSpecifier),
		mWithBurn				(inWithBurn),
		mInputSource			(inInputSource),
		mInputChannel			(NTV2_CHANNEL1),
		mOutputChannel			(NTV2_CHANNEL_INVALID),
		mOutputDestination		(NTV2_OUTPUTDESTINATION_SDI3),
		mPixelFormat			(inPixelFormat),
		mVancMode				(NTV2_VANCMODE_OFF),
		mAudioSystem			(inAudioSystem),
		mDoLevelConversion		(inDoLevelConversion),
		mUseDualLink			(inUseDualLink),
		mGlobalQuit				(false),
		mTCBounce				(true),
		mTCSource				(NTV2_TCINDEX_INVALID),
		mVideoBufferSize		(0),
		mAudioBufferSize		(0)
{
	::memset (mAVHostBuffer, 0x0, sizeof (mAVHostBuffer));

}	//	constructor


QABurn::~QABurn ()
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
	mDevice.UnsubscribeInputVerticalEvent (mInputChannel);

	ReleaseHostBuffers ();
}	//	destructor


void QABurn::Quit (void)
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


AJAStatus QABurn::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

	if (!mDevice.IsDeviceReady ())
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);		//	Set the OEM service level
	mDeviceID = mDevice.GetDeviceID ();					//	Keep the device ID handy since it will be used frequently

	//	Set up the video and audio...
	status = SetupInputVideo ();
	if (AJA_FAILURE (status))
	{
		mDevice.Close();
		return status;
	}

	status = SetupAudio ();
	if (AJA_FAILURE (status))
	{
		mDevice.Close();
		return status;
	}

	return AJA_STATUS_SUCCESS;

}	//	Init


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


bool QABurn::InputSignalHasTimecode (void)
{
	const ULWord	regNum		(::GetRP188RegisterForInput (mInputSource));
	ULWord			regValue	(0);

	//	Bit 16 of the RP188 DBB register will be set if there is timecode embedded in the input signal...
	if (regNum && mDevice.ReadRegister (regNum, &regValue) && regValue & BIT(16))
		return true;
	return false;

}	//	InputSignalHasTimecode


bool QABurn::AnalogLTCInputHasTimecode (void)
{
	ULWord	regMask		(0);
	ULWord	regValue	(0);
	switch (mTCSource)
	{
		case NTV2_TCINDEX_LTC1:		regMask = kRegMaskLTC1InPresent;	break;
		case NTV2_TCINDEX_LTC2:		regMask = kRegMaskLTC2InPresent;	break;
		default:					return false;						break;
	}
	mDevice.ReadRegister (kRegLTCStatusControl, &regValue, regMask);
	return regValue ? true : false;

}	//	AnalogLTCInputHasTimecode


AJAStatus QABurn::SetupInputVideo (void)
{
	const UWord	numFrameStores	(::NTV2DeviceGetNumFrameStores (mDeviceID));

	if (!::NTV2DeviceCanDoCapture (mDeviceID))
		{cerr << "## ERROR:  Device cannot capture" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Does this device have the requested input source?
	if (!::NTV2DeviceCanDoInputSource (mDeviceID, mInputSource))
		{cerr << "## ERROR:  Device does not have the specified input source" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Can the device handle the requested frame buffer pixel format?
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPixelFormat))
		{cerr << "## ERROR:  Device not capable of frame buffer pixel format " << ::NTV2FrameBufferFormatToString(mPixelFormat) << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Pick an input NTV2Channel from the input source, and enable its frame buffer...
	mInputChannel = NTV2_INPUT_SOURCE_IS_SDI (mInputSource) ? ::NTV2InputSourceToChannel (mInputSource) : NTV2_CHANNEL2;	//	NTV2_CHANNEL1;
	mDevice.EnableChannel (mInputChannel);		//	Enable the input frame buffer

	//	Pick an appropriate output NTV2Channel, and enable its frame buffer...
	switch (mInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		mOutputChannel = (numFrameStores == 2 || numFrameStores > 4) ? NTV2_CHANNEL2 : NTV2_CHANNEL3;	break;
		case NTV2_INPUTSOURCE_SDI2:		mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL3 : NTV2_CHANNEL4;							break;
		case NTV2_INPUTSOURCE_SDI3:		mOutputChannel = NTV2_CHANNEL4;																	break;
		case NTV2_INPUTSOURCE_SDI4:		mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL5 : NTV2_CHANNEL3;							break;
		case NTV2_INPUTSOURCE_SDI5: 	mOutputChannel = NTV2_CHANNEL6;																	break;
		case NTV2_INPUTSOURCE_SDI6:		mOutputChannel = NTV2_CHANNEL7;																	break;
		case NTV2_INPUTSOURCE_SDI7:		mOutputChannel = NTV2_CHANNEL8;																	break;
		case NTV2_INPUTSOURCE_SDI8:		mOutputChannel = NTV2_CHANNEL7;																	break;

		case NTV2_INPUTSOURCE_ANALOG:
		case NTV2_INPUTSOURCE_HDMI:		mOutputChannel = numFrameStores < 3 ? NTV2_CHANNEL2 : NTV2_CHANNEL3;							break;

		default:
		case NTV2_INPUTSOURCE_INVALID:	cerr << "## ERROR:  Bad input source" << endl;  return AJA_STATUS_BAD_PARAM;
	}

	//	Enable/subscribe interrupts...
	mDevice.EnableInputInterrupt (mInputChannel);
	mDevice.SubscribeInputVerticalEvent (mInputChannel);
	mDevice.EnableOutputInterrupt (mOutputChannel);
	mDevice.SubscribeOutputVerticalEvent (mOutputChannel);

	//	Flip the input spigot to "receive" if necessary...
	bool	isTransmit	(false);
	if (::NTV2DeviceHasBiDirectionalSDI (mDevice.GetDeviceID ())			//	If device has bidirectional SDI connectors...
		&& NTV2_INPUT_SOURCE_IS_SDI (mInputSource)							//	...and desired input source is SDI...
			&& mDevice.GetSDITransmitEnable (mInputChannel, &isTransmit)	//	...and GetSDITransmitEnable succeeds...
				&& isTransmit)												//	...and input is set to "transmit"...
	{
		mDevice.SetSDITransmitEnable (mInputChannel, false);				//	...then disable transmit mode...
		mDevice.WaitForOutputVerticalInterrupt (mOutputChannel, 12);		//	...and give the device a dozen frames or so to lock to the input signal
	}	//	if input SDI connector needs to switch from transmit mode

	//	Free-run the device clock, since E-to-E mode isn't used, nor is a mixer tied to the input...
	mDevice.SetReference (NTV2_REFERENCE_FREERUN);

	//	Set pixel formats...
	mDevice.SetFrameBufferFormat (mInputChannel, mPixelFormat);

	//	Check the timecode source...
	if (NTV2_IS_SDI_TIMECODE_INDEX (mTCSource))
	{
		const NTV2Channel	tcChannel	(::NTV2TimecodeIndexToChannel (mTCSource));
		const NTV2Channel	endNum		(NTV2Channel (::NTV2DeviceGetNumVideoChannels (mDeviceID)));
		if (tcChannel >= endNum)
			{cerr << "## ERROR:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' illegal on this device" << endl;  return AJA_STATUS_BAD_PARAM;}
		if (tcChannel == mOutputChannel)
			{cerr << "## ERROR:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' conflicts with output channel" << endl;  return AJA_STATUS_BAD_PARAM;}

		mDevice.EnableInputInterrupt (tcChannel);
		mDevice.SubscribeInputVerticalEvent (tcChannel);
		if (::NTV2DeviceHasBiDirectionalSDI (mDevice.GetDeviceID ())	//	If device has bidirectional SDI connectors...
			&& mDevice.GetSDITransmitEnable (tcChannel, &isTransmit)	//	...and GetSDITransmitEnable succeeds...
				&& isTransmit)											//	...and the SDI timecode source is set to "transmit"...
		{
			mDevice.SetSDITransmitEnable (tcChannel, false);			//	...then disable transmit mode...
			mDevice.WaitForInputVerticalInterrupt (tcChannel, 12);		//	...and give the device a dozen frames or so to lock to the input signal
		}	//	if input SDI connector needs to switch from transmit mode

		const NTV2VideoFormat	tcInputVideoFormat	(mDevice.GetInputVideoFormat (::NTV2TimecodeIndexToInputSource (mTCSource)));
		if (tcInputVideoFormat == NTV2_FORMAT_UNKNOWN)
			cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' has no input signal" << endl;
		if (!InputSignalHasTimecode ())
			cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' has no embedded timecode" << endl;
	}
	else if (NTV2_IS_ANALOG_TIMECODE_INDEX (mTCSource) && !AnalogLTCInputHasTimecode ())
		cerr << "## WARNING:  Timecode source '" << ::NTV2TCIndexToString (mTCSource, true) << "' has no embedded timecode" << endl;

	//	Disable RP188 bypass mode...
	mDevice.DisableRP188Bypass (mInputChannel);

	if (NTV2_IS_ANALOG_TIMECODE_INDEX (mTCSource))
		mDevice.SetLTCInputEnable (true);	//	Enable analog LTC input (some LTC inputs are shared with reference input)

	//	Set up the device signal routing...
	RouteInputSignal ();
	return AJA_STATUS_SUCCESS;

}	//	SetupInputVideo


AJAStatus QABurn::SetupOutputVideo (const NTV2VideoFormat inVideoFormat)
{
	if (!::NTV2DeviceCanDoPlayback (mDeviceID))
		{cerr << "## ERROR:  Device cannot playout" << endl;  return AJA_STATUS_BAD_PARAM;}

	mDevice.EnableChannel (mOutputChannel);		//	Enable the output frame buffer

	//	Disable RP188 bypass mode...
	mDevice.DisableRP188Bypass (mOutputChannel);

	//	Pick an appropriate output spigot based on the output channel...
	mOutputDestination	= ::NTV2ChannelToOutputDestination (mOutputChannel);
	if (!::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut2) && !::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut2))
		mOutputDestination = NTV2_OUTPUTDESTINATION_SDI1;			//	If device has only one SDI output
	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID)					//	If device has bidirectional SDI connectors...
		&& NTV2_OUTPUT_DEST_IS_SDI (mOutputDestination))			//	...and output destination is SDI...
			mDevice.SetSDITransmitEnable (mOutputChannel, true);	//	...then enable transmit mode

	mDevice.SetFrameBufferFormat (mOutputChannel, mPixelFormat);
	if (mWithBurn)
	{
		//	Prepare my AJATimeCodeBurn instance...
		NTV2FormatDescriptor fd (inVideoFormat, mPixelFormat, mVancMode);
		mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mPixelFormat), fd.numPixels, fd.numLines);
	}
	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))									//	If device supports multiple formats per-channel...
		mDevice.SetVideoFormat (inVideoFormat, false, false, mOutputChannel);		//	...then also set the output channel format to the detected input format

	if (::Is8BitFrameBufferFormat (mPixelFormat))	//	8-bit FBFs require bit shift for VANC geometries...
		mDevice.SetVANCShiftMode (mOutputChannel, NTV2_IS_VANCMODE_ON (mVancMode) ? NTV2_VANCDATA_8BITSHIFT_ENABLE : NTV2_VANCDATA_NORMAL);

	return AJA_STATUS_SUCCESS;
}


AJAStatus QABurn::SetupAudio (void)
{
	const UWord		numAudioSystems	(::NTV2DeviceGetNumAudioSystems (mDeviceID));
	for (UWord audioSystem (0);  audioSystem < numAudioSystems;  audioSystem++)
	{
		mDevice.SetAudioOutputReset (NTV2AudioSystem(audioSystem), false);
		mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, NTV2AudioSystem(audioSystem));
	}
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
		return AJA_STATUS_SUCCESS;
	if (mAudioSystem >= numAudioSystems)
		{cerr << "## ERROR:  SetupAudio:  '" << ::NTV2AudioSystemToString(mAudioSystem) << "' not on this device" << endl; return AJA_STATUS_RANGE;}

	//	Have the audio subsystem capture audio from the designated input source...
	mDevice.SetAudioSystemInputSource (mAudioSystem, ::NTV2InputSourceToAudioSource (mInputSource), ::NTV2InputSourceToEmbeddedAudioInput (mInputSource));

	//	It's best to use all available audio channels...
	mDevice.SetNumberAudioChannels (::NTV2DeviceGetMaxAudioChannels (mDeviceID), mAudioSystem);

	//	Assume 48kHz PCM...
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	4MB device audio buffers work best...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set up the output audio embedders...
	if (::NTV2DeviceGetNumAudioSystems (mDeviceID) > 1)
		mDevice.SetSDIOutputAudioSystem (mOutputChannel, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


AJAStatus QABurn::SetupHostBuffers (const NTV2VideoFormat inVideoFormat)
{
	ReleaseHostBuffers ();
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	mVideoBufferSize = GetVideoWriteSize (inVideoFormat, mPixelFormat, mVancMode);
	mAudioBufferSize = NTV2_AUDIOSIZE_MAX;

	//	Allocate and add each in-host AVDataBuffer to my circular buffer member variable.
	//	Note that DMA performance can be accelerated slightly by using page-aligned video buffers...
	for (unsigned bufferNdx = 0;  bufferNdx < CIRCULAR_BUFFER_SIZE;  bufferNdx++ )
	{
		mAVHostBuffer [bufferNdx].fVideoBuffer = reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mVideoBufferSize, AJA_PAGE_SIZE));
		mAVHostBuffer [bufferNdx].fVideoBufferSize = mVideoBufferSize;
		if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
		{
			mAVHostBuffer [bufferNdx].fAudioBuffer = reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mAudioBufferSize, AJA_PAGE_SIZE));
			mAVHostBuffer [bufferNdx].fAudioBufferSize = mAudioBufferSize;
		}
		mAVCircularBuffer.Add (& mAVHostBuffer [bufferNdx]);
		if (mAVHostBuffer [bufferNdx].fVideoBuffer == NULL || (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem) && mAVHostBuffer [bufferNdx].fAudioBuffer == NULL))
		{
			cerr << "## ERROR:  Unable to allocate video and/or audio buffer " << (bufferNdx + 1) << " of " << CIRCULAR_BUFFER_SIZE << endl;
			return AJA_STATUS_MEMORY;
		}
	}	//	for each AVDataBuffer
	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void QABurn::ReleaseHostBuffers (void)
{
	//	Free all my buffers...
	for (unsigned bufferNdx (0);  bufferNdx < CIRCULAR_BUFFER_SIZE;  bufferNdx++)
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

	mAVCircularBuffer.Clear ();
	return;

}	//	ReleaseHostBuffers


void QABurn::RouteInputSignal (void)
{
	const NTV2OutputCrosspointID	inputOutputXpt	(::GetInputSourceOutputXpt (mInputSource));
	const NTV2InputCrosspointID		fbInputXpt		(::GetFrameBufferInputXptFromChannel (mInputChannel));

	mDevice.ClearRouting ();
	if (::IsRGBFormat (mPixelFormat))
	{
		//	If the frame buffer is configured for RGB, incoming YUV must be routed thru a CSC...
		const NTV2InputCrosspointID		cscVideoInputXpt	(::GetCSCInputXptFromChannel (mInputChannel));
		const NTV2OutputCrosspointID	cscOutputXpt		(::GetCSCOutputXptFromChannel (mInputChannel, false/*isKey*/, true/*isRGB*/));

		mDevice.Connect (cscVideoInputXpt, inputOutputXpt);	//	Connect CSC's video input to inSpigot's output
		mDevice.Connect (fbInputXpt, cscOutputXpt);			//	Connect FB's input to CSC's output
	}
	else
		mDevice.Connect (fbInputXpt, inputOutputXpt);		//	Route YCbCr directly from the input to the frame buffer input

}	//	RouteInputSignal


void QABurn::RouteOutputSignal (const NTV2VideoFormat inVideoFormat)
{
	const NTV2InputCrosspointID		outputInputXpt	(::GetOutputDestInputXpt (mOutputDestination));
	const NTV2OutputCrosspointID	fbOutputXpt		(::GetFrameBufferOutputXptFromChannel (mOutputChannel, ::IsRGBFormat (mPixelFormat)));
	const NTV2Standard				outputStandard	(::GetNTV2StandardFromVideoFormat (inVideoFormat));
	NTV2OutputCrosspointID			outputXpt		(fbOutputXpt);

	if (::IsRGBFormat (mPixelFormat))
	{
		const NTV2OutputCrosspointID	cscVidOutputXpt	(::GetCSCOutputXptFromChannel (mOutputChannel, false, true));
		const NTV2InputCrosspointID		cscVidInputXpt	(::GetCSCInputXptFromChannel (mOutputChannel));

		mDevice.Connect (cscVidInputXpt, fbOutputXpt);		//	Connect the CSC's video input to the frame store's output
		mDevice.Connect (outputInputXpt, cscVidOutputXpt);	//	Connect the SDI output's input to the CSC's video output
		outputXpt = cscVidOutputXpt;
	}
	else
		mDevice.Connect (outputInputXpt, outputXpt);

	mTCOutputs.clear ();
	mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (mOutputChannel, false));
	mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (mOutputChannel, true));

	//	Route all SDI outputs to the outputXpt...
	const NTV2Channel	startChan		(NTV2_CHANNEL1);
	const NTV2Channel	endChan			(NTV2Channel (::NTV2DeviceGetNumVideoChannels (mDeviceID)));
	const NTV2Channel	tcInputChannel	(NTV2_IS_SDI_TIMECODE_INDEX (mTCSource) ? ::NTV2TimecodeIndexToChannel (mTCSource) : NTV2_CHANNEL_INVALID);
	NTV2WidgetID		outputWidgetID	(NTV2_WIDGET_INVALID);

	for (NTV2Channel chan (startChan);  chan < endChan;  chan = NTV2Channel(chan + 1))
	{
		mDevice.SetRP188Source (chan, 0);	//	Set all SDI spigots to capture embedded LTC (VITC could be an option)

		if (chan == mOutputChannel)
			continue;	//	Skip the output channel, already routed
		if (chan == mInputChannel && NTV2_INPUT_SOURCE_IS_SDI(mInputSource))
			continue;	//	Skip the input channel (if SDI)
		if (NTV2_IS_VALID_CHANNEL (tcInputChannel) && chan == tcInputChannel)
			continue;	//	Skip the timecode input channel
		if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
			mDevice.SetSDITransmitEnable (chan, true);
		if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, ::NTV2DeviceCanDoDualLink (mDeviceID)), outputWidgetID))
			if (::NTV2DeviceCanDoWidget (mDeviceID, outputWidgetID))
			{
				mDevice.Connect (::GetSDIOutputInputXpt (chan), outputXpt);
				mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan, false));
				mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan, true));
				mDevice.SetSDIOutputStandard (chan, outputStandard);
				if (::NTV2DeviceGetNumAudioSystems (mDeviceID) > 1  &&  NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
					mDevice.SetSDIOutputAudioSystem (chan, mAudioSystem);
			}
	}	//	for each output spigot

	//	Connect more outputs -- HDMI, analog, SDI monitor, etc...
	if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1))
		mDevice.Connect (NTV2_XptHDMIOutInput, outputXpt);
	if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v2))
		mDevice.Connect (NTV2_XptHDMIOutQ1Input, outputXpt);
	if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
		mDevice.Connect (NTV2_XptAnalogOutInput, outputXpt);
	if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIMonOut1))
		mDevice.Connect (::GetSDIOutputInputXpt (NTV2_CHANNEL5), outputXpt);

	CNTV2SignalRouter	routing;
	mDevice.GetRouting (routing);
	cerr	<< "## NOTE:  Routing:" << endl << routing
			<< mTCOutputs.size () << " timecode destination(s):  " << mTCOutputs << endl;

}	//	RouteOutputSignal


AJAStatus QABurn::Run ()
{
	//	Start the playout and capture threads...
	StartCaptureThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we will start the play thread
void QABurn::StartPlayThread (void)
{
	//	Create and start the playout thread...
	mPlayThread = new AJAThread ();
	mPlayThread->Attach (PlayThreadStatic, this);
	mPlayThread->SetPriority (AJA_ThreadPriority_High);
	mPlayThread->Start ();

}	//	StartPlayThread


//	The playout thread function
void QABurn::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	//	Grab the NTV2Burn instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	QABurn *	pApp	(reinterpret_cast <QABurn *> (pContext));
	pApp->PlayFrames ();

}	//	PlayThreadStatic


void QABurn::PlayFrames (void)
{
	const NTV2VideoFormat	videoFormat		(mDevice.GetInputVideoFormat (mInputSource));
	NTV2VANCMode			vancMode		(NTV2_VANCMODE_INVALID);
	const UByte				startFrame[]	= {0, 7, 14, 21, 28, 35, 42, 49};
	const string			strVideoFormat	(CNTV2DemoCommon::StripFormatString (::NTV2VideoFormatToString (videoFormat)));
	AUTOCIRCULATE_TRANSFER	outputXfer;

	SetupOutputVideo (videoFormat);		//	Set up device output
	RouteOutputSignal (videoFormat);	//	Set up output signal routing
	mDevice.GetVANCMode (vancMode, mInputChannel);

	cerr	<< "## NOTE:  Playout of '" << ::NTV2VideoFormatToString(videoFormat) << "' on " << ::NTV2ChannelToString(mOutputChannel)
			<< " using " << ::NTV2FrameBufferFormatToString(mPixelFormat)
			<< " with " << ::NTV2VANCModeToString(mVancMode)
			<< " and buffers " << UWord(startFrame[mOutputChannel]) << "-" << UWord(startFrame[mOutputChannel]+6)
			<< " and " << ::NTV2AudioSystemToString(mAudioSystem) << endl;

	mDevice.AutoCirculateStop (mOutputChannel);
	mDevice.AutoCirculateInitForOutput (mOutputChannel, 0,	mAudioSystem,				//	Which audio system?
															AUTOCIRCULATE_WITH_RP188,	//	A/C options
															1,							//	Number of channels
															startFrame[mOutputChannel], startFrame[mOutputChannel] + 6);	//	Custom frame range
	mDevice.AutoCirculateStart (mOutputChannel);

	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		AVDataBuffer *	playData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (playData)
		{
			//	Transfer the timecode-burned frame to the device for playout...
			outputXfer.SetVideoBuffer (playData->fVideoBuffer, playData->fVideoBufferSize);
			if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
				outputXfer.SetAudioBuffer (playData->fAudioBuffer, playData->fAudioBufferSize);
			//mOutputXfer.SetAllOutputTimeCodes (NTV2_RP188 (playData->fRP188Data));	//	All output spigots get timecode
			for (NTV2TCIndexesConstIter iter (mTCOutputs.begin ());  iter != mTCOutputs.end ();  ++iter)
				outputXfer.SetOutputTimeCode (NTV2_RP188 (playData->fRP188Data), *iter);
			mDevice.AutoCirculateTransfer (mOutputChannel, outputXfer);

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer ();

			//	Check for format change...
			if (videoFormat != mDevice.GetInputVideoFormat (mInputSource))
			{
				cerr	<< "## NOTE:  Playout stopped due to input video format change -- was '" << strVideoFormat
						<< "', now '" << ::NTV2VideoFormatToString (mDevice.GetInputVideoFormat (mInputSource)) << "'" << endl;
				break;
			}
		}	//	if playData
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop (mOutputChannel);

}	//	PlayFrames


//////////////////////////////////////////////



//////////////////////////////////////////////
//
//	This is where the capture thread gets started
//
void QABurn::StartCaptureThread (void)
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
void QABurn::CaptureThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	//	Grab the NTV2Burn instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	QABurn *	pApp	(reinterpret_cast <QABurn *> (pContext));
	pApp->CaptureFrames ();

}	//	CaptureThreadStatic


//
//	Repeatedly captures frames until told to stop
//
void QABurn::CaptureFrames (void)
{
	const ULWord	acOptions		(AUTOCIRCULATE_WITH_RP188);	//	AUTOCIRCULATE_WITH_LTC
	const UByte		startFrame[] =	{0, 7, 14, 21, 28, 35, 42, 49};

	while (!mGlobalQuit)
	{
		NTV2VideoFormat			currentVideoFormat	(NTV2_FORMAT_UNKNOWN);
		NTV2VideoFormat			lastVideoFormat		(NTV2_FORMAT_UNKNOWN);
		ULWord					debounceCounter		(0);
		AJAStatus				status			(AJA_STATUS_SUCCESS);
		AUTOCIRCULATE_TRANSFER	mInputXfer;

		//	Determine the input video signal format...
		while (currentVideoFormat == NTV2_FORMAT_UNKNOWN)
		{
			mDevice.WaitForInputVerticalInterrupt (mInputChannel);
			if (mGlobalQuit)
				return;		//	Terminate if asked to do so

			NTV2VideoFormat	videoFormat	(mDevice.GetInputVideoFormat (mInputSource));
			if (videoFormat == NTV2_FORMAT_UNKNOWN)
				;	//	Wait for video signal to appear
			else if (debounceCounter == 0)
			{
				lastVideoFormat = videoFormat;		//	First valid video format to appear
				debounceCounter++;					//	Start debounce counter
			}
			else if (debounceCounter == 6)
			{
				debounceCounter = 0;				//	Reset for next signal outage
				currentVideoFormat = videoFormat;	//	Set official video format to use
			}
			else
				debounceCounter = (lastVideoFormat == videoFormat) ? debounceCounter + 1 : 0;
		}	//	loop while input video format is unstable

		//	At this point, the input video format is stable.
		//	Set the device format to the input format detected...
		mDevice.SetVideoFormat (currentVideoFormat,	false,			//	OEM mode, not retail mode
													false,			//	Don't keep VANC settings
													mInputChannel);	//	Specify channel (in case this is a multichannel device)
		mDevice.SetEnableVANCData (NTV2_IS_VANCMODE_TALL(mVancMode), NTV2_IS_VANCMODE_TALLER(mVancMode), mInputChannel);
		if (::Is8BitFrameBufferFormat (mPixelFormat))
			mDevice.SetVANCShiftMode (mInputChannel, NTV2_IS_VANCMODE_ON(mVancMode) ? NTV2_VANCDATA_8BITSHIFT_ENABLE : NTV2_VANCDATA_NORMAL);

		//	Set up the circular buffers based on the detected currentVideoFormat...
		status = SetupHostBuffers (currentVideoFormat);
		if (AJA_FAILURE (status))
			return;

		{
			AJAAutoLock	autoLock (mLock);	//	Avoid AutoCirculate buffer collisions
			cerr	<< "## NOTE:  Capture of '" << ::NTV2VideoFormatToString(currentVideoFormat) << "' on " << ::NTV2ChannelToString(mInputChannel)
					<< " using " << ::NTV2FrameBufferFormatToString(mPixelFormat)
					<< " with " << ::NTV2VANCModeToString(mVancMode)
					<< " and buffers " << UWord(startFrame[mInputChannel]) << "-" << UWord(startFrame[mInputChannel]+6)
					<< " and " << ::NTV2AudioSystemToString(mAudioSystem) << endl;
		}

		StartPlayThread ();			//	Start a new playout thread
		mDevice.AutoCirculateStop (mInputChannel);

		Bouncer			yPercent		(85/*upperLimit*/, 1/*lowerLimit*/, 1/*startValue*/);	//	Used to "bounce" timecode up & down
		NTV2TCIndex		timecodeIndex	(::NTV2ChannelToTimecodeIndex (mInputChannel, true));
		string			timecodeStr;

		mDevice.AutoCirculateInitForInput (mInputChannel, 0,	mAudioSystem,	//	Which audio system?
																acOptions,		//	A/C options
																1,				//	Number of channels
																startFrame[mInputChannel], startFrame[mInputChannel] + 6);	//	Custom frame range

		//	Enable analog LTC input (some LTC inputs are shared with reference input)
		mDevice.SetLTCInputEnable (true);

		//	Start AutoCirculate running...
		mDevice.AutoCirculateStart (mInputChannel);

		while (!mGlobalQuit)
		{
			AUTOCIRCULATE_STATUS	acStatus;
			mDevice.AutoCirculateGetStatus (mInputChannel, acStatus);
			if (acStatus.IsRunning ()  &&  acStatus.HasAvailableInputFrame ())
			{
				//	At this point, there's at least one fully-formed frame available in the device's
				//	frame buffer to transfer to the host. Reserve an AVDataBuffer to "produce", and
				//	use it to hold the next frame to be transferred from the device...
				AVDataBuffer *	pCaptureData	(mAVCircularBuffer.StartProduceNextBuffer ());

				mInputXfer.SetVideoBuffer (pCaptureData->fVideoBuffer, pCaptureData->fVideoBufferSize);
				if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
					mInputXfer.SetAudioBuffer (pCaptureData->fAudioBuffer, pCaptureData->fAudioBufferSize);

				//	Do the transfer from the device into our host AVDataBuffer...
				mDevice.AutoCirculateTransfer (mInputChannel, mInputXfer);

				//	Remember the audio byte count...
				pCaptureData->fAudioBufferSize	= mInputXfer.GetCapturedAudioByteCount ();

				//	Remember the timecode(s) of interest...
				NTV2_RP188	tc;
				NTV2TimeCodeList	tcList;
				mInputXfer.GetInputTimeCodes (tcList);
				mInputXfer.GetInputTimeCode (tc, timecodeIndex);
				pCaptureData->fRP188Data = tc;

				CRP188	rp188	(tc);
				rp188.GetRP188Str (timecodeStr);

				if (!tc.IsValid ())
				{
					//	Invent a timecode (based on frame count)...
					const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (currentVideoFormat));
					const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat (ntv2FrameRate));
					const	CRP188			frameRP188Info	(mInputXfer.GetTransferStatus().acFramesProcessed, tcFormat);

					frameRP188Info.GetRP188Reg (pCaptureData->fRP188Data);
					frameRP188Info.GetRP188Str (timecodeStr);
					//if (acStatus.GetProcessedFrameCount () % 200 == 0) cerr << "## DEBUG:  InventedTC: " << timecodeStr << "\r";
				}
				//else if (acStatus.GetProcessedFrameCount () % 200 == 0) cerr << "## DEBUG:  CapturedTC: " << timecodeStr << "\r";

				if (mWithBurn)
				{
					//	"Burn" the timecode into the host AVDataBuffer while we have full access to it...
					mTCBurner.BurnTimeCode (reinterpret_cast <char *> (mInputXfer.acVideoBuffer.GetHostPointer()),
											timecodeStr.c_str (),
											mTCBounce  ?  yPercent.Next()  :  80);
				}

				//	Signal that we're done "producing" the frame, making it available for future "consumption"...
				mAVCircularBuffer.EndProduceNextBuffer ();

			}	//	if A/C running and frame(s) are available for transfer
			else
			{
				mDevice.WaitForInputVerticalInterrupt (mInputChannel);

				//	Did incoming video format change?
				if (mDevice.GetInputVideoFormat (mInputSource) != currentVideoFormat)
				{
					cerr << endl << "## WARNING:  Input video format changed" << endl;

					//	These next 2 calls signal the playout thread to allow it to terminate...
					mAVCircularBuffer.StartProduceNextBuffer ();
					mAVCircularBuffer.EndProduceNextBuffer ();
					break;	//	exit frame processing loop
				}	//	if input video format changed
			}	//	else not running or no frames available for xfer
		}	//	normal frame processing loop

		//	Stop AutoCirculate...
		mDevice.AutoCirculateStop (mInputChannel);
	}	//	loop til quit signaled
	cerr << endl << "## NOTE:  CaptureFrames thread exit" << endl;

}	//	CaptureFrames


void QABurn::GetACStatus (ULWord & outInputFramesProcessed, ULWord & outInputFramesDropped, ULWord & outInputBufferLevel, ULWord & outOutputFramesDropped, ULWord & outOutputBufferLevel)
{
	AUTOCIRCULATE_STATUS	inputStatus, outputStatus;

	if (mDevice.AutoCirculateGetStatus (mInputChannel, inputStatus)  &&  inputStatus.IsRunning())
	{
		outInputFramesProcessed	= inputStatus.acFramesProcessed;
		outInputFramesDropped	= inputStatus.acFramesDropped;
		outInputBufferLevel		= inputStatus.acBufferLevel;
	}
	else
		outInputFramesProcessed = outInputFramesDropped = outInputBufferLevel = 0;

	if (mDevice.AutoCirculateGetStatus (mOutputChannel, outputStatus)  &&  outputStatus.IsRunning())
	{
		outOutputFramesDropped	= outputStatus.acFramesDropped;
		outOutputBufferLevel	= outputStatus.acBufferLevel;
	}
	else
		outOutputFramesDropped = outOutputBufferLevel = 0;

}	//	GetACStatus


ULWord QABurn::GetRP188RegisterForInput (const NTV2InputSource inInputSource)		//	static
{
	switch (inInputSource)
	{
		case NTV2_INPUTSOURCE_SDI1:		return kRegRP188InOut1DBB;	break;	//	reg 29
		case NTV2_INPUTSOURCE_SDI2:		return kRegRP188InOut2DBB;	break;	//	reg 64
		case NTV2_INPUTSOURCE_SDI3:		return kRegRP188InOut3DBB;	break;	//	reg 268
		case NTV2_INPUTSOURCE_SDI4:		return kRegRP188InOut4DBB;	break;	//	reg 273
		default:						return 0;					break;
	}	//	switch on input source

}	//	GetRP188RegisterForInput


bool QABurn::InputSignalHasRP188BypassEnabled (void)
{
	bool			result		(false);
	const ULWord	regNum		(GetRP188RegisterForInput (mInputSource));
	ULWord			regValue	(0);

	//
	//	Bit 23 of the RP188 DBB register will be set if output timecode will be grabbed directly from the input...
	//
	if (regNum && mDevice.ReadRegister (regNum, &regValue) && regValue & BIT(23))
		result = true;

	return result;

}	//	InputSignalHasRP188BypassEnabled


void QABurn::DisableRP188Bypass (void)
{
	const ULWord	regNum	(GetRP188RegisterForInput (mInputSource));

	//
	//	Clear bit 23 of the input source's RP188 DBB register...
	//
	if (regNum)
		mDevice.WriteRegister (regNum, 0, BIT(23), BIT(23));

}	//	DisableRP188Bypass
