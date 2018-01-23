/**
	@file		ntv2fieldburn.cpp
	@brief		Implementation of NTV2FieldBurn demonstration class.
	@copyright	Copyright (C) 2013-2017 AJA Video Systems, Inc.  All rights reserved.
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

#define NTV2_AUDIOSIZE_MAX	(401 * 1024)

const uint32_t	kAppSignature	(AJA_FOURCC ('F','l','d','B'));


//////////////////////	IMPLEMENTATION


NTV2FieldBurn::NTV2FieldBurn (const string &				inDeviceSpecifier,
								const bool					inWithAudio,
								const NTV2FrameBufferFormat	inPixelFormat,
								const NTV2InputSource		inInputSource,
								const bool					inDoMultiFormat)

	:	mPlayThread			(NULL),
		mCaptureThread		(NULL),
		mLock				(new AJALock (CNTV2DemoCommon::GetGlobalMutexName ())),
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
		mAudioSystem		(inWithAudio ? ::NTV2InputSourceToAudioSystem (inInputSource) : NTV2_AUDIOSYSTEM_INVALID),
		mGlobalQuit			(false),
		mDoMultiChannel		(inDoMultiFormat),
		mVideoBufferSize	(0)
{
	::memset (mAVHostBuffer, 0, sizeof (mAVHostBuffer));

}	//	constructor


NTV2FieldBurn::~NTV2FieldBurn ()
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

	//	Free all my buffers...
	for (unsigned bufferNdx = 0;  bufferNdx < CIRCULAR_BUFFER_SIZE;  bufferNdx++)
	{
		if (mAVHostBuffer[bufferNdx].fVideoBuffer)
		{
			AJAMemory::FreeAligned (mAVHostBuffer[bufferNdx].fVideoBuffer);
			mAVHostBuffer[bufferNdx].fVideoBuffer = NULL;
		}
		if (mAVHostBuffer[bufferNdx].fVideoBuffer2)
		{
			AJAMemory::FreeAligned (mAVHostBuffer[bufferNdx].fVideoBuffer2);
			mAVHostBuffer[bufferNdx].fVideoBuffer2 = NULL;
		}
		if (mAVHostBuffer[bufferNdx].fAudioBuffer)
		{
			AJAMemory::FreeAligned (mAVHostBuffer[bufferNdx].fAudioBuffer);
			mAVHostBuffer[bufferNdx].fAudioBuffer = NULL;
		}
	}	//	for each buffer in the ring

	if (!mDoMultiChannel)
	{
		mDevice.SetEveryFrameServices (mSavedTaskMode);										//	Restore prior service level
		mDevice.ReleaseStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ()));	//	Release the device
	}

}	//	destructor


void NTV2FieldBurn::Quit (void)
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


AJAStatus NTV2FieldBurn::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	ULWord	appSignature	(0);
	int32_t	appPID			(0);
	mDevice.GetStreamingApplication (&appSignature, &appPID);	//	Who currently "owns" the device?
	mDevice.GetEveryFrameServices (mSavedTaskMode);				//	Save the current device state
	if (!mDoMultiChannel)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;		//	Some other app is using the device
		}
		mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Set the OEM service level
		mDevice.ClearRouting ();								//	Clear the current device routing (since I "own" the device)
	}
	else
		mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Force OEM tasks

	mDeviceID = mDevice.GetDeviceID ();							//	Keep the device ID handy since it will be used frequently

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


AJAStatus NTV2FieldBurn::SetupVideo (void)
{
	const UWord	numFrameStores	(::NTV2DeviceGetNumFrameStores (mDeviceID));

	//	Does this device have the requested input source?
	if (!::NTV2DeviceCanDoInputSource (mDeviceID, mInputSource))
		{cerr << "## ERROR:  Device does not have the specified input source" << endl;  return AJA_STATUS_BAD_PARAM;}

	//	Pick an input NTV2Channel from the input source, and enable its frame buffer...
	mInputChannel = NTV2_INPUT_SOURCE_IS_SDI (mInputSource) ? ::NTV2InputSourceToChannel (mInputSource) : NTV2_CHANNEL1;
	mDevice.EnableChannel (mInputChannel);		//	Enable the input frame buffer

	//	Pick an appropriate output NTV2Channel, and enable its frame buffer...
	switch (mInputSource)
	{
		case NTV2_INPUTSOURCE_ANALOG1:	mOutputChannel = NTV2_CHANNEL3;																	break;
		case NTV2_INPUTSOURCE_HDMI1:	mOutputChannel = NTV2_CHANNEL3;																	break;
		case NTV2_INPUTSOURCE_SDI1:		mOutputChannel = (numFrameStores == 2 || numFrameStores > 4) ? NTV2_CHANNEL2 : NTV2_CHANNEL3;	break;
		case NTV2_INPUTSOURCE_SDI2:		mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL3 : NTV2_CHANNEL4;							break;
		case NTV2_INPUTSOURCE_SDI3:		mOutputChannel = NTV2_CHANNEL4;																	break;
		case NTV2_INPUTSOURCE_SDI4:		mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL5 : NTV2_CHANNEL3;							break;
		case NTV2_INPUTSOURCE_SDI5: 	mOutputChannel = NTV2_CHANNEL6;																	break;
		case NTV2_INPUTSOURCE_SDI6:		mOutputChannel = NTV2_CHANNEL7;																	break;
		case NTV2_INPUTSOURCE_SDI7:		mOutputChannel = NTV2_CHANNEL8;																	break;
		case NTV2_INPUTSOURCE_SDI8:		mOutputChannel = NTV2_CHANNEL7;																	break;
		default:
		case NTV2_INPUTSOURCE_INVALID:	cerr << "## ERROR:  Bad input source" << endl;  return AJA_STATUS_BAD_PARAM;
	}
	mDevice.EnableChannel (mOutputChannel);		//	Enable the output frame buffer

	//	Pick an appropriate output spigot based on the output channel...
	mOutputDestination	= ::NTV2ChannelToOutputDestination (mOutputChannel);
	if (!::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut2) && !::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut2))
		mOutputDestination = NTV2_OUTPUTDESTINATION_SDI1;			//	If device has only one SDI output
	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID)					//	If device has bidirectional SDI connectors...
		&& NTV2_OUTPUT_DEST_IS_SDI (mOutputDestination))			//	...and output destination is SDI...
			mDevice.SetSDITransmitEnable (mOutputChannel, true);	//	...then enable transmit mode

	//	Flip the input spigot to "receive" if necessary...
	bool	isTransmit	(false);
	if (::NTV2DeviceHasBiDirectionalSDI (mDevice.GetDeviceID ())			//	If device has bidirectional SDI connectors...
		&& NTV2_INPUT_SOURCE_IS_SDI (mInputSource)							//	...and desired input source is SDI...
			&& mDevice.GetSDITransmitEnable (mInputChannel, &isTransmit)	//	...and GetSDITransmitEnable succeeds...
				&& isTransmit)												//	...and input is set to "transmit"...
	{
		mDevice.SetSDITransmitEnable (mInputChannel, false);				//	...then disable transmit mode...
		AJATime::Sleep (500);												//	...and give the device a dozen frames or so to lock to the input signal
	}	//	if input SDI connector needs to switch from transmit mode

	//	Is there an input signal?  What format is it?
	mVideoFormat = mDevice.GetInputVideoFormat (mInputSource);
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		{cerr << "## ERROR:  No input signal, or can't handle its format" << endl;  return AJA_STATUS_NOINPUT;}

	//	This demo requires an interlaced signal...
	if (IsProgressiveTransport (mVideoFormat))
		{cerr << "## ERROR:  Input signal is progressive -- no fields" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Free-run the device clock, since E-to-E mode isn't used, nor is a mixer tied to the input...
	mDevice.SetReference (NTV2_REFERENCE_FREERUN);

	//	Check the timecode source...
	if (!InputSignalHasTimecode ())
		cerr << "## WARNING:  Timecode source has no embedded timecode" << endl;

	//	If the device supports different per-channel video formats, configure it as requested...
	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
		mDevice.SetMultiFormatMode (mDoMultiChannel);

	//	Set the input/output channel video formats to the video format that was detected earlier...
	mDevice.SetVideoFormat (mVideoFormat, false, false, ::NTV2DeviceCanDoMultiFormat (mDeviceID) ? mInputChannel : NTV2_CHANNEL1);
	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))									//	If device supports multiple formats per-channel...
		mDevice.SetVideoFormat (mVideoFormat, false, false, mOutputChannel);		//	...then also set the output channel format to the detected input format

	//	Can the device handle the requested frame buffer pixel format?
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPixelFormat))
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;		//	Fall back to 8-bit YCbCr

	//	Set both input and output frame buffers' pixel formats...
	mDevice.SetFrameBufferFormat (mInputChannel, mPixelFormat);
	mDevice.SetFrameBufferFormat (mOutputChannel, mPixelFormat);

	//	Enable and subscribe to the input interrupts...
	mDevice.EnableInputInterrupt (mInputChannel);
	mDevice.SubscribeInputVerticalEvent (mInputChannel);

	//	Enable and subscribe to the output interrupts...
	mDevice.EnableOutputInterrupt (mOutputChannel);
	mDevice.SubscribeOutputVerticalEvent (mOutputChannel);

	//	Normally, timecode embedded in the output signal comes from whatever is written into the RP188
	//	registers (30/31 for SDI out 1, 65/66 for SDIout2, etc.).
	//	AutoCirculate automatically writes the timecode in the AUTOCIRCULATE_TRANSFER's acRP188 field
	//	into these registers (if AutoCirculateInitForOutput was called with AUTOCIRCULATE_WITH_RP188 set).
	//	Newer AJA devices can also bypass these RP188 registers, and simply copy whatever timecode appears
	//	at any SDI input (called the "bypass source"). To ensure that AutoCirculate's playout timecode
	//	will actually be seen in the output signal, "bypass mode" must be disabled...
	bool	bypassIsEnabled	(false);
	mDevice.IsRP188BypassEnabled (::NTV2InputSourceToChannel (mInputSource), bypassIsEnabled);
	if (bypassIsEnabled)
		mDevice.DisableRP188Bypass (::NTV2InputSourceToChannel (mInputSource));

	//	Now that newer AJA devices can capture/play anc data from separate buffers,
	//	there's no need to enable VANC frame geometries...
	NTV2FrameGeometry	geometry	(NTV2_FG_INVALID);
	mDevice.GetFrameGeometry (geometry, mInputChannel);
	mDevice.SetVANCMode (mVancMode, ::GetNTV2StandardFromVideoFormat (mVideoFormat), geometry, mInputChannel);
	mDevice.SetVANCMode (mVancMode, ::GetNTV2StandardFromVideoFormat (mVideoFormat), geometry, mOutputChannel);
	if (::Is8BitFrameBufferFormat (mPixelFormat))
	{
		//	8-bit FBFs require bit shift for VANC geometries, or no shift for normal geometries...
		mDevice.SetVANCShiftMode (mInputChannel, NTV2_IS_VANCMODE_ON(mVancMode) ? NTV2_VANCDATA_8BITSHIFT_ENABLE : NTV2_VANCDATA_NORMAL);
		mDevice.SetVANCShiftMode (mOutputChannel, NTV2_IS_VANCMODE_ON(mVancMode) ? NTV2_VANCDATA_8BITSHIFT_ENABLE : NTV2_VANCDATA_NORMAL);
	}

	//	Now that the video is set up, get information about the current frame geometry...
	mFormatDescriptor = NTV2FormatDescriptor (mVideoFormat, mPixelFormat, mVancMode);
	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


AJAStatus NTV2FieldBurn::SetupAudio (void)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
		return AJA_STATUS_SUCCESS;

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
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	//  Divide by two to make the buffers the size of a video field (half of a full frame)...
	mVideoBufferSize = GetVideoWriteSize (mVideoFormat, mPixelFormat, mVancMode) / 2;

	//	Allocate and add each in-host AVDataBuffer to my circular buffer member variable.
	//	Note that DMA performance can be accelerated slightly by using page-aligned video buffers...
	for (unsigned bufferNdx (0);  bufferNdx < CIRCULAR_BUFFER_SIZE;  bufferNdx++)
	{
		//	Allocate F1 & F2 video frame buffers...
		mAVHostBuffer [bufferNdx].fVideoBuffer  = reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mVideoBufferSize, AJA_PAGE_SIZE));
		mAVHostBuffer [bufferNdx].fVideoBuffer2 = reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mVideoBufferSize, AJA_PAGE_SIZE));

		//  The DMA transfer of the field will be done as a group of lines, with each line considered a "segment".
		//  IMPORTANT:	For segmented DMAs, AutoCirculateTransfer expects the video buffer size to be set to the segment size, in bytes.
		//				The format descriptor's line pitch is the size of the line in 4-byte words, so multiply that by sizeof(ULWord)...
		mAVHostBuffer [bufferNdx].fVideoBufferSize = mFormatDescriptor.linePitch * 4;

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
			|| !mAVHostBuffer[bufferNdx].fVideoBuffer2
			|| (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem) && !mAVHostBuffer[bufferNdx].fAudioBuffer))
				{
					cerr << "## ERROR:  Allocation failed:  buffer " << (bufferNdx + 1) << " of " << CIRCULAR_BUFFER_SIZE << endl;
					return AJA_STATUS_MEMORY;
				}
	}	//	for each AVDataBuffer

	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2FieldBurn::RouteInputSignal (void)
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
	mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (mOutputChannel));

	if (!mDoMultiChannel)
	{
		//	Route all SDI outputs to the outputXpt...
		const NTV2Channel	startNum		(NTV2_CHANNEL1);
		const NTV2Channel	endNum			(NTV2Channel (::NTV2DeviceGetNumVideoChannels (mDeviceID)));
		NTV2WidgetID		outputWidgetID	(NTV2_WIDGET_INVALID);

		for (NTV2Channel chan (startNum);  chan < endNum;  chan = NTV2Channel (chan + 1))
		{
			if (chan == mInputChannel || chan == mOutputChannel)
				continue;	//	Skip the input & output channel, already routed
			if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
				mDevice.SetSDITransmitEnable (chan, true);
			if (CNTV2SignalRouter::GetWidgetForInput (::GetSDIOutputInputXpt (chan, ::NTV2DeviceCanDoDualLink (mDeviceID)), outputWidgetID))
				if (::NTV2DeviceCanDoWidget (mDeviceID, outputWidgetID))
				{
					mDevice.Connect (::GetSDIOutputInputXpt (chan), outputXpt);
					mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan));
					mTCOutputs.insert (::NTV2ChannelToTimecodeIndex (chan, true));
				}
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
	cerr << "## DEBUG:  " << mTCOutputs.size () << " timecode destination(s):  " << mTCOutputs << endl;

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
	mPlayThread = new AJAThread ();
	mPlayThread->Attach (PlayThreadStatic, this);
	mPlayThread->SetPriority (AJA_ThreadPriority_High);
	mPlayThread->Start ();

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
	AUTOCIRCULATE_TRANSFER	outputXferField1;	//	Field 1 A/C output transfer info
	AUTOCIRCULATE_TRANSFER	outputXferField2;	//	Field 2 A/C output transfer info

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop (mOutputChannel);

	//	Tell AutoCirculate to DMA F1's data as a group of "segments".
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

	//	Initialize the AutoCirculate output channel...
	{
		AJAAutoLock	autoLock (mLock);	//	Avoid AutoCirculate buffer collisions
		mDevice.AutoCirculateInitForOutput (mOutputChannel, 7, mAudioSystem, AUTOCIRCULATE_WITH_RP188);
	}

	//	Start AutoCirculate running...
	mDevice.AutoCirculateStart (mOutputChannel);

	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		AVDataBuffer *	playData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (playData)
		{
			//	Prepare to transfer this timecode-burned field (F1) to the device for playout.
			//	Set the outputXfer struct's video and audio buffers from playData's buffers...
			outputXferField1.SetVideoBuffer (playData->fVideoBuffer, playData->fVideoBufferSize);
			if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
				outputXferField1.SetAudioBuffer (playData->fAudioBuffer, playData->fAudioBufferSize);	//	Audio transfer is done on F1 only

			//	Tell AutoCirculate to embed this frame's timecode into the SDI output.
			//	To embed this same timecode into other SDI outputs, set the appropriate members of the acOutputTimeCodes array...
			for (NTV2TCIndexesConstIter iter (mTCOutputs.begin ());  iter != mTCOutputs.end ();  ++iter)
				outputXferField1.SetOutputTimeCode (NTV2_RP188 (playData->fRP188Data), *iter);

			//	Transfer F1 to the device...
			mDevice.AutoCirculateTransfer (mOutputChannel, outputXferField1);

			//  Now transfer F2 to the same device frame buffer used for F1...
			outputXferField2.acDesiredFrame = outputXferField1.acTransferStatus.acTransferFrame;
			outputXferField2.SetVideoBuffer (playData->fVideoBuffer2, playData->fVideoBufferSize);
			mDevice.AutoCirculateTransfer (mOutputChannel, outputXferField2);

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer ();
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop (mOutputChannel);

}	//	PlayFrames


//////////////////////////////////////////////



//////////////////////////////////////////////
//
//	This is where the capture thread gets started
//
void NTV2FieldBurn::StartCaptureThread (void)
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
void NTV2FieldBurn::CaptureThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	//	Grab the NTV2FieldBurn instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2FieldBurn *	pApp	(reinterpret_cast <NTV2FieldBurn *> (pContext));
	pApp->CaptureFrames ();

}	//	CaptureThreadStatic


//
//	Repeatedly captures frames until told to stop
//
void NTV2FieldBurn::CaptureFrames (void)
{
	AUTOCIRCULATE_TRANSFER	inputXferField1;	//	Field 1 A/C input transfer info
	AUTOCIRCULATE_TRANSFER	inputXferField2;	//	Field 2 A/C input transfer info
	const NTV2TCIndex		timeCodeIndex	(::NTV2ChannelToTimecodeIndex (mInputChannel));

	//	Use AutoCirculate's "segmented DMA" feature to transfer each field out of the device's
	//	full-frame video buffer as a group of "segments".
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

	//	Stop AutoCirculate on this channel, just in case some other app left it running...
	mDevice.AutoCirculateStop (mInputChannel);

	//	Initialize AutoCirculate...
	{
		AJAAutoLock	autoLock (mLock);	//	Avoid AutoCirculate buffer collisions
		mDevice.AutoCirculateInitForInput (mInputChannel, 7, mAudioSystem, AUTOCIRCULATE_WITH_RP188);
	}

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
			//	use it in the next transfer from the device...
			AVDataBuffer *	captureData	(mAVCircularBuffer.StartProduceNextBuffer ());

			inputXferField1.SetVideoBuffer (captureData->fVideoBuffer, captureData->fVideoBufferSize);
			if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
				inputXferField1.SetAudioBuffer (captureData->fAudioBuffer, captureData->fAudioBufferSize);

			//	Transfer Field 1 from the device into our host AVDataBuffer...
			mDevice.AutoCirculateTransfer (mInputChannel, inputXferField1);

			//	Remember the audio byte count, which can vary frame-by-frame...
			captureData->fAudioBufferSize	= NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem)  ?  inputXferField1.GetCapturedAudioByteCount ()  :  0;

			NTV2_RP188	defaultTC;
			if (NTV2_IS_VALID_TIMECODE_INDEX (timeCodeIndex) && InputSignalHasTimecode ())
			{
				//	Use the timecode that was captured by AutoCirculate...
				inputXferField1.GetInputTimeCode (defaultTC, timeCodeIndex);
				//NTV2TimeCodeList	tcList;																			//	DEBUG
				//inputXferField1.GetInputTimeCodes (tcList);														//	DEBUG
				//cerr << tcList << ", acRP188: " << inputXferField1.acTransferStatus.acFrameStamp.acRP188 << endl;	//	DEBUG
			}
			if (defaultTC.IsValid ())
				captureData->fRP188Data	= defaultTC;	//	Stuff it in the captureData
			else
			{
				//	Invent a timecode (based on frame count)...
				const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
				const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
				const	CRP188			inventedTC		(inputXferField1.acTransferStatus.acFramesProcessed, 0, 0, 10, tcFormat);
				inventedTC.GetRP188Reg (captureData->fRP188Data);	//	Stuff it in the captureData
				//cerr << "## DEBUG:  InventedTC: " << inventedTC << "\r";
			}
			CRP188	tc	(captureData->fRP188Data);
			string	tcStr;
			tc.GetRP188Str (tcStr);

			//  Transfer Field 2 from the same device frame buffer used for the Field 1...
			inputXferField2.acDesiredFrame = inputXferField1.acTransferStatus.acTransferFrame;
			inputXferField2.SetVideoBuffer (captureData->fVideoBuffer2, captureData->fVideoBufferSize);

			mDevice.AutoCirculateTransfer (mInputChannel, inputXferField2);

			//	"Burn" the same timecode into both fields in the host AVDataBuffer while it's locked for our exclusive access...
			mTCBurner.BurnTimeCode (reinterpret_cast <char *> (inputXferField1.acVideoBuffer.GetHostPointer ()), tcStr.c_str (), 10);
			mTCBurner.BurnTimeCode (reinterpret_cast <char *> (inputXferField2.acVideoBuffer.GetHostPointer ()), tcStr.c_str (), 30);

			//	Signal that we're done "producing" the frame, making it available for future "consumption"...
			mAVCircularBuffer.EndProduceNextBuffer ();
		}	//	if A/C running and frame(s) are available for transfer
		else
		{
			//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
			//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
			//	efficient to wait for the next input vertical interrupt event to get signaled...
			mDevice.WaitForInputVerticalInterrupt (mInputChannel);
		}
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop (mInputChannel);

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


bool NTV2FieldBurn::InputSignalHasTimecode (void)
{
	const ULWord	regNum		(::GetRP188RegisterForInput (mInputSource));
	ULWord			regValue	(0);

	//	Bit 16 of the RP188 DBB register will be set if there is timecode embedded in the input signal...
	if (regNum && mDevice.ReadRegister (regNum, &regValue) && regValue & BIT(16))
		return true;
	return false;

}	//	InputSignalHasTimecode
