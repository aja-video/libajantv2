/**
	@file		ntv2ccgrabber.cpp
	@brief		Implementation of NTV2CCGrabber class
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2ccgrabber.h"
#include "ajacc/includes/ntv2captionrenderer.h"
#include "ajaanc/includes/ancillarylist.h"
#include "ajaanc/includes/ancillarydata_cea608_line21.h"
#include "ntv2debug.h"
#include "ajastuff/common/types.h"
#include "ajastuff/system/memory.h"
#include <iostream>
#include <iomanip>


#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define NTV2_ANCSIZE_MAX	(2048)

static const ULWord					kAppSignature				AJA_FOURCC ('C','C','G','R');
static const NTV2Line21Attributes	kRedOnTransparentBG			(NTV2_CC608_Red, NTV2_CC608_Blue, NTV2_CC608_Transparent);
static const NTV2Line21Attributes	kGreenOnTransparentBG		(NTV2_CC608_Green, NTV2_CC608_Blue, NTV2_CC608_Transparent);


NTV2CCGrabber::NTV2CCGrabber (	const string			inDeviceSpecifier,
								const NTV2Line21Channel	in608Channel,
								const bool				inBurnCaptions,
								const bool				inMultiFormat,
								const bool				inForceVanc,
								const bool				inWithAudio,
								const NTV2Channel		inInputChannel)

	:	mCaptureThread		(NULL),
		mLock				(new AJALock (CNTV2DemoCommon::GetGlobalMutexName ())),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mDeviceSpecifier	(inDeviceSpecifier),
		mInputChannel		(inInputChannel),
		mInputSource		(NTV2_INPUTSOURCE_INVALID),
		mCaptureFBF			(NTV2_FBF_10BIT_YCBCR),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mAudioSystem		(inWithAudio ? NTV2_AUDIOSYSTEM_1 : NTV2_AUDIOSYSTEM_INVALID),
		mDoMultiFormat		(inMultiFormat),
		mForceVanc			(inForceVanc),
		mGlobalQuit			(false),
		mCaptureBufferSize	(0),
		mErrorTally			(0),
		mCaptionDataTally	(0),
		m608Channel			(in608Channel),
		m608Mode			(NTV2_CC608_CapModeUnknown),
		mBurnCaptions		(inBurnCaptions),
		mHeadUpDisplayOn	(true),
		mOutputChannel		(NTV2_CHANNEL_INVALID),
		mPlayoutFBF			(NTV2_FBF_ARGB),
		mPlayoutThread		(NULL)
{
	::memset (mAVHostBuffer, 0x0, sizeof (mAVHostBuffer));

	CNTV2CaptionDecoder608::Create (m608Decoder);
	CNTV2CaptionDecoder708::Create (m708Decoder);
	NTV2_ASSERT (m608Decoder && m708Decoder);

}	//	constructor


NTV2CCGrabber::~NTV2CCGrabber ()
{
	if (m608Decoder)
		m608Decoder->UnsubscribeChangeNotification (Caption608ChangedStatic, this);

	//	Stop my capture thread, then destroy it...
	Quit ();

	delete mCaptureThread;
	mCaptureThread = NULL;

	//	Unsubscribe from input vertical event...
	mDevice.UnsubscribeInputVerticalEvent (mInputChannel);

	//	Free all my buffers...
	ReleaseHostBuffers ();

	if (!mDoMultiFormat)
	{
		mDevice.SetEveryFrameServices (mSavedTaskMode);
		mDevice.ReleaseStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ()));
	}

}	//	destructor


void NTV2CCGrabber::Quit (void)
{
	//	Set the 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	if (mCaptureThread)
		while (mCaptureThread->Active ())
			AJATime::Sleep (10);

	if (mPlayoutThread)
		while (mPlayoutThread->Active ())
			AJATime::Sleep (10);

	if (!mDoMultiFormat)
		mDevice.ClearRouting ();

}	//	Quit


AJAStatus NTV2CCGrabber::Init (void)
{
	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

	if (!mDevice.IsDeviceReady ())
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID ();	//	Keep this handy because it's used frequently

	if (mBurnCaptions)
	{
		if (::NTV2DeviceGetNumFrameStores (mDeviceID) < 2)						//	Requires 2+ frame stores
		{
			cerr	<< "## ERROR:  Device '" << ::NTV2DeviceIDToString (mDevice.GetDeviceID ())
					<< "' cannot burn-in captions because at least 2 frame stores are required" << endl;
			return AJA_STATUS_FAIL;
		}
		if (::NTV2DeviceGetNumVideoInputs (mDeviceID) < 1  ||  ::NTV2DeviceGetNumVideoInputs (mDeviceID) < 1)	//	Requires at least 1 input and 1 output
		{
			cerr	<< "## ERROR:  Device '" << ::NTV2DeviceIDToString (mDevice.GetDeviceID ())
					<< "' cannot be used -- at least 1 SDI input and 1 SDI output required" << endl;
			return AJA_STATUS_FAIL;
		}
		if (!::NTV2DeviceGetNumMixers (mDeviceID))								//	Requires 1+ mixers
		{
			cerr	<< "## ERROR:  Device '" << ::NTV2DeviceIDToString (mDevice.GetDeviceID ())
					<< "' cannot burn-in captions because a mixer/keyer widget is required" << endl;
			return AJA_STATUS_FAIL;
		}
		if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPlayoutFBF))		//	Frame store must handle 8-bit RGB with alpha
		{
			cerr	<< "## ERROR:  Device '" << ::NTV2DeviceIDToString (mDevice.GetDeviceID ())
					<< "' cannot burn-in captions because it can't do 8-bit RGB (with alpha)" << endl;
			return AJA_STATUS_FAIL;
		}

	}	//	if --burn

	if (!mDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ())))
		{
			cerr << "## ERROR:  Cannot acquire -- device busy" << endl;
			return AJA_STATUS_BUSY;		//	Some other app owns the device
		}
		mDevice.GetEveryFrameServices (mSavedTaskMode);	//	Save the current task mode
	}

	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);		//	Use the OEM service level

	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
		mDevice.SetMultiFormatMode (mDoMultiFormat);

	if (!mForceVanc)								//	if user didn't use --vanc option...
		if (!DeviceAncExtractorIsAvailable ())		//	and anc extractor isn't available...
			mForceVanc = true;						//	then force Vanc anyway

	//	Set up the audio...
	AJAStatus	status = SetupAudio ();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the input video...
	status = SetupInputVideo ();
	if (AJA_FAILURE (status))
		return status;

	if (!m608Decoder->SubscribeChangeNotification (Caption608ChangedStatic, this))
		{cerr	<< "## WARNING:  SubscribeChangeNotification failed" << endl;	return AJA_STATUS_FAIL;}

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2CCGrabber::SetupHostBuffers (const NTV2VideoFormat inVideoFormat)
{
	NTV2VANCMode	vancMode	(NTV2_VANCMODE_INVALID);

	ReleaseHostBuffers ();
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);
	mDevice.GetVANCMode (vancMode, mInputChannel);
	NTV2_ASSERT (NTV2_IS_VANCMODE_TALL (vancMode));		//	Should be "tall" Vanc only!
	mCaptureBufferSize = ::GetVideoWriteSize (inVideoFormat, mCaptureFBF, vancMode);

	//	Allocate and add each in-host AVDataBuffer to my circular buffer member variable...
	for (unsigned bufferNdx (0);  bufferNdx < CIRCULAR_BUFFER_SIZE;  bufferNdx++)
	{
		//	Video buffer...
		mAVHostBuffer [bufferNdx].fVideoBufferSize = mCaptureBufferSize;
		mAVHostBuffer [bufferNdx].fVideoBuffer = reinterpret_cast <uint32_t *> (new uint8_t [mCaptureBufferSize]);
		if (mAVHostBuffer [bufferNdx].fVideoBufferSize && mAVHostBuffer [bufferNdx].fVideoBuffer == NULL)
			return AJA_STATUS_MEMORY;

		//	Audio buffer...
		mAVHostBuffer [bufferNdx].fAudioBuffer = NULL;		//	No need to capture audio
		mAVHostBuffer [bufferNdx].fAudioBufferSize = 0;
		if (mAVHostBuffer [bufferNdx].fAudioBufferSize && mAVHostBuffer [bufferNdx].fAudioBuffer == NULL)
			return AJA_STATUS_MEMORY;

		//	Ancillary data buffer --- don't bother allocating storage for it if mForceVanc is true, since anc packets will be in Vanc lines in video buffer...
		mAVHostBuffer [bufferNdx].fAncBuffer = mForceVanc ? NULL : reinterpret_cast <uint32_t *> (new uint8_t [NTV2_ANCSIZE_MAX]);
		mAVHostBuffer [bufferNdx].fAncBufferSize = mForceVanc ? 0 : NTV2_ANCSIZE_MAX;
		if (mAVHostBuffer [bufferNdx].fAncBufferSize && mAVHostBuffer [bufferNdx].fAncBuffer == NULL)
			return AJA_STATUS_MEMORY;
		mAVHostBuffer [bufferNdx].fAncF2Buffer = mForceVanc ? NULL : reinterpret_cast <uint32_t *> (new uint8_t [NTV2_ANCSIZE_MAX]);
		mAVHostBuffer [bufferNdx].fAncF2BufferSize = mForceVanc ? 0 : NTV2_ANCSIZE_MAX;
		if (mAVHostBuffer [bufferNdx].fAncF2BufferSize && mAVHostBuffer [bufferNdx].fAncF2Buffer == NULL)
			return AJA_STATUS_MEMORY;

		//	Add this buffer to my circular buffer...
		mAVCircularBuffer.Add (& mAVHostBuffer [bufferNdx]);
	}	//	for each AVDataBuffer

	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2CCGrabber::ReleaseHostBuffers (void)
{
	//	Release each in-host AVDataBuffer...
	for (unsigned bufferNdx = 0;  bufferNdx < CIRCULAR_BUFFER_SIZE;  bufferNdx++)
	{
		if (mAVHostBuffer[bufferNdx].fVideoBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fVideoBuffer;
			mAVHostBuffer[bufferNdx].fVideoBuffer = NULL;
		}
		mAVHostBuffer [bufferNdx].fVideoBufferSize = 0;
		if (mAVHostBuffer[bufferNdx].fAudioBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fAudioBuffer;
			mAVHostBuffer[bufferNdx].fAudioBuffer = NULL;
		}
		mAVHostBuffer [bufferNdx].fAudioBufferSize = 0;
		if (mAVHostBuffer[bufferNdx].fAncBuffer)
		{
			delete mAVHostBuffer[bufferNdx].fAncBuffer;
			mAVHostBuffer[bufferNdx].fAncBuffer = NULL;
		}
		mAVHostBuffer [bufferNdx].fAncBufferSize = 0;
		if (mAVHostBuffer[bufferNdx].fAncF2Buffer)
		{
			delete mAVHostBuffer[bufferNdx].fAncF2Buffer;
			mAVHostBuffer[bufferNdx].fAncF2Buffer = NULL;
		}
		mAVHostBuffer [bufferNdx].fAncF2BufferSize = 0;
	}	//	for each AVDataBuffer

	mAVCircularBuffer.Clear ();
	return;

}	//	ReleaseHostBuffers


AJAStatus NTV2CCGrabber::SetupInputVideo (void)
{
	bool			isTransmit			(false);
	const ULWord	numVideoChannels	(::NTV2DeviceGetNumVideoChannels (mDeviceID));
	const UWord		numFrameStores		(::NTV2DeviceGetNumFrameStores (mDeviceID));

	//	CCGrabber is SDI only
	if (numVideoChannels < (ULWord (mInputChannel) + 1))
	{
		cerr	<< "## ERROR:  " << ::NTV2DeviceIDToString (mDevice.GetDeviceID ()) << " has no " << ::NTV2ChannelToString (mInputChannel, true) << " input" << endl;
		return AJA_STATUS_FAIL;
	}

	mInputSource = ::NTV2ChannelToInputSource (mInputChannel);
	if (!::NTV2DeviceCanDoInputSource (mDeviceID, mInputSource))
	{
		cerr	<< "## ERROR:  Device '" << ::NTV2DeviceIDToString (mDevice.GetDeviceID ())
				<< "' cannot grab captions from '" << ::NTV2InputSourceToString (mInputSource) << "'" << endl;
		return AJA_STATUS_FAIL;
	}
	NTV2_ASSERT (NTV2_INPUT_SOURCE_IS_SDI (mInputSource));	//	CCGrabber is SDI only

	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))												//	If device has bidirectional SDI (Io4K, Corvid24, etc.)...
		if (mDevice.GetSDITransmitEnable (::NTV2InputSourceToChannel (mInputSource), isTransmit))	//	...and GetSDITransmitEnable succeeds...
			if (isTransmit)																			//	...and input is set to "transmit"...
			{
				mDevice.SetSDITransmitEnable (::NTV2InputSourceToChannel (mInputSource), false);	//	...then disable transmit mode...
				AJATime::Sleep (500);																//	...and give the device some time to lock to a signal
			}	//	if input SDI connector needs to switch from transmit mode

	//	Set the device's reference source to that input...
	const NTV2ReferenceSource	refSrc	(mDoMultiFormat ? NTV2_REFERENCE_FREERUN : ::NTV2InputSourceToReferenceSource (mInputSource));
	mDevice.SetReference (refSrc);

	//	Set the output channel to use...
	switch (mInputChannel)
	{
		case NTV2_CHANNEL1:		mOutputChannel = (numFrameStores == 2 || numFrameStores > 4) ? NTV2_CHANNEL2 : NTV2_CHANNEL3;	break;
		case NTV2_CHANNEL2:		mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL3 : NTV2_CHANNEL4;							break;
		case NTV2_CHANNEL3:		mOutputChannel = NTV2_CHANNEL4;																	break;
		case NTV2_CHANNEL4:		mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL5 : NTV2_CHANNEL3;							break;
		case NTV2_CHANNEL5: 	mOutputChannel = NTV2_CHANNEL6;																	break;
		case NTV2_CHANNEL6:		mOutputChannel = NTV2_CHANNEL7;																	break;
		case NTV2_CHANNEL7:		mOutputChannel = NTV2_CHANNEL8;																	break;
		case NTV2_CHANNEL8:		mOutputChannel = NTV2_CHANNEL7;																	break;
		case NTV2_CHANNEL_INVALID:	return AJA_STATUS_BAD_PARAM;
	}

	//	Enable the frame buffer...
	mDevice.EnableChannel (mInputChannel);

	NTV2_ASSERT (::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mCaptureFBF));
	mDevice.SetFrameBufferFormat (mInputChannel, mCaptureFBF);

	//	Enable and subscribe to the interrupts for the channel to be used...
	mDevice.EnableInputInterrupt (mInputChannel);
	mDevice.SubscribeInputVerticalEvent (mInputChannel);

	//	"Tune" the 608 decoder to the desired channel...
	m608Decoder->SetDisplayChannel (m608Channel);
	m708Decoder->SetDisplayChannel (m608Channel);

	//	Set up the device signal routing...
	RouteInputSignal ();

	cerr	<< "## NOTE:  Using " << (mForceVanc ? "VANC" : "device Anc extraction") << " with " << ::NTV2InputSourceToString (mInputSource)
			<< ", " << ::NTV2ReferenceSourceToString (refSrc) << ", input " << ::NTV2ChannelToString (mInputChannel)
			<< ", output " << ::NTV2ChannelToString (mOutputChannel) << endl;

	return AJA_STATUS_SUCCESS;

}	//	SetupInputVideo


AJAStatus NTV2CCGrabber::SetupAudio (void)
{
	if (NTV2_IS_VALID_AUDIO_SYSTEM (mAudioSystem))
	{
		const UWord	numAudioSystems	(::NTV2DeviceGetNumAudioSystems (mDeviceID));
		if (numAudioSystems > 1  &&  UWord (mInputChannel) < numAudioSystems)
			mAudioSystem = ::NTV2ChannelToAudioSystem (mInputChannel);

		//	Configure the audio system...
		mDevice.SetAudioSystemInputSource (mAudioSystem, NTV2_AUDIO_EMBEDDED, ::NTV2ChannelToEmbeddedAudioInput (mInputChannel));
		mDevice.SetNumberAudioChannels (::NTV2DeviceGetMaxAudioChannels (mDeviceID), mAudioSystem);
		mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);
		mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

		//	Set up the output audio embedders...
		if (!mBurnCaptions  &&  numAudioSystems > 1)
		{
			//	Have all device outputs use the same audio system...
			if (mDoMultiFormat)
				mDevice.SetSDIOutputAudioSystem (mOutputChannel, mAudioSystem);
			else
				for (unsigned channelNum (0);  channelNum < ::NTV2DeviceGetNumVideoChannels (mDeviceID);  channelNum++)
					if (NTV2Channel (channelNum) != mInputChannel)
						mDevice.SetSDIOutputAudioSystem (NTV2Channel (channelNum), mAudioSystem);
		}

		//
		//	Loopback mode is used to play whatever audio appears in the input signal when
		//	it's connected directly to an output (i.e., "end-to-end" mode), which is the case
		//	when mBurnCaptions is false. If mBurnCaptions is true, and loopback is enabled,
		//	the output video will lag the audio, as video frames get briefly delayed in the
		//	ring buffer. Audio, therefore, needs to come out of the (buffered) frame data
		//	being played, so loopback must be turned off when burning captions.
		//
		mDevice.SetAudioLoopBack (mBurnCaptions ? NTV2_AUDIO_LOOPBACK_ON : NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);
	}
	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


static const NTV2WidgetID	gSDIOutputs []		=	{	NTV2_WgtSDIOut1,			NTV2_WgtSDIOut2,			NTV2_WgtSDIOut3,			NTV2_WgtSDIOut4,
														NTV2_Wgt3GSDIOut5,			NTV2_Wgt3GSDIOut6,			NTV2_Wgt3GSDIOut7,			NTV2_Wgt3GSDIOut8	};

static const NTV2WidgetID	g3GSDIOutputs []	=	{	NTV2_Wgt3GSDIOut1,			NTV2_Wgt3GSDIOut2,			NTV2_Wgt3GSDIOut3,			NTV2_Wgt3GSDIOut4,
														NTV2_Wgt3GSDIOut5,			NTV2_Wgt3GSDIOut6,			NTV2_Wgt3GSDIOut7,			NTV2_Wgt3GSDIOut8	};


void NTV2CCGrabber::RouteInputSignal (void)
{
	const NTV2OutputCrosspointID	inputSignalOutputXpt	(::GetSDIInputOutputXptFromChannel (mInputChannel));
	if (!mDoMultiFormat)
		mDevice.ClearRouting ();
	mDevice.Connect (::GetFrameBufferInputXptFromChannel (mInputChannel), inputSignalOutputXpt);	//	Frame store input to signal input

	//	Pass the input signal through to the output, unless the --burn option was specified...
	if (!mBurnCaptions)
	{
		if (mDoMultiFormat)
		{
			mDevice.SetSDITransmitEnable (mOutputChannel, true);
			if (::NTV2DeviceCanDoWidget (mDeviceID, g3GSDIOutputs [mOutputChannel]) || ::NTV2DeviceCanDoWidget (mDeviceID, gSDIOutputs [mOutputChannel]))
				mDevice.Connect (::GetSDIOutputInputXpt (mOutputChannel), inputSignalOutputXpt);
		}	//	if multiformat/multichannel mode
		else
		{
			//	Route End-to-End...
			const NTV2Channel	startNum	(NTV2_CHANNEL1);
			const NTV2Channel	endNum		(NTV2_MAX_NUM_CHANNELS);

			for (NTV2Channel chan (startNum);  chan < endNum;  chan = NTV2Channel (chan + 1))
			{
				if (ULWord (chan) >= ::NTV2DeviceGetNumVideoChannels (mDeviceID))
					break;
				if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID) && chan != mInputChannel)
					mDevice.SetSDITransmitEnable (chan, true);
				if (chan != mInputChannel)
					if (::NTV2DeviceCanDoWidget (mDeviceID, g3GSDIOutputs [chan]) || ::NTV2DeviceCanDoWidget (mDeviceID, gSDIOutputs [chan]))
						mDevice.Connect (::GetSDIOutputInputXpt (chan), inputSignalOutputXpt);
			}	//	for each output spigot
		}	//	else not multiformat/multichannel mode
	}	//	if not burning captions

}	//	RouteInputSignal


void NTV2CCGrabber::SetOutputStandards (const NTV2VideoFormat inVideoFormat)
{
	NTV2_ASSERT (!mBurnCaptions);	//	Shouldn't be here if not end-to-end routing!
	const NTV2Standard	outputStandard	(::GetNTV2StandardFromVideoFormat (inVideoFormat));
	const NTV2Channel	startNum		(mInputChannel == NTV2_CHANNEL1 ? NTV2_CHANNEL1 : NTV2_CHANNEL5);
	const NTV2Channel	endNum			(mInputChannel == NTV2_CHANNEL1 ? NTV2_CHANNEL5 : NTV2_MAX_NUM_CHANNELS);

	for (NTV2Channel chan (startNum);  chan < endNum;  chan = NTV2Channel (chan + 1))
	{
		if (ULWord (chan) >= ::NTV2DeviceGetNumVideoChannels (mDeviceID))
			break;
		if (chan != mInputChannel)
			if (::NTV2DeviceCanDoWidget (mDeviceID, g3GSDIOutputs [chan]) || ::NTV2DeviceCanDoWidget (mDeviceID, gSDIOutputs [chan]))
				mDevice.SetSDIOutputStandard (chan, outputStandard);
	}	//	for each output spigot

}	//	SetOutputStandards


AJAStatus NTV2CCGrabber::Run ()
{
	//	Start the capture thread...
	StartCaptureThread ();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////

//	This is where we start the capture thread
void NTV2CCGrabber::StartCaptureThread (void)
{
	//	Create and start the capture thread...
	mCaptureThread = new AJAThread ();
	mCaptureThread->Attach (CaptureThreadStatic, this);
	mCaptureThread->SetPriority (AJA_ThreadPriority_High);
	mCaptureThread->Start ();

}	//	StartCaptureThread


//	The capture thread function
void NTV2CCGrabber::CaptureThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	//	Grab the NTV2CCGrabber instance pointer from the pContext parameter,
	//	then call its CaptureFrames method...
	NTV2CCGrabber *	pApp	(reinterpret_cast <NTV2CCGrabber *> (pContext));
	pApp->CaptureFrames ();

}	//	CaptureThreadStatic


//	The capture function -- capture frames until told to quit...
void NTV2CCGrabber::CaptureFrames (void)
{
	while (!mGlobalQuit)
	{
		NTV2VideoFormat		currentVideoFormat	(NTV2_FORMAT_UNKNOWN);
		NTV2VideoFormat		lastVideoFormat		(NTV2_FORMAT_UNKNOWN);
		ULWord				debounceCounter		(0);

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
		cerr << endl << "## NOTE:  Input video format is " << ::NTV2VideoFormatToString (currentVideoFormat) << endl;

		//	At this point, the input video format is stable.
		//	Set the device format to the input format detected...
		mDevice.SetVideoFormat (currentVideoFormat,	false,			//	OEM mode, not retail mode
													false,			//	Don't keep VANC settings
													mInputChannel);	//	Specify channel (in case this is a multichannel device)

		//	To simplify things, always enable VANC geometry...
		mDevice.SetEnableVANCData (true, false, mInputChannel);		//	"Tall" format is sufficient to grab captions
		if (::Is8BitFrameBufferFormat (mCaptureFBF))
			mDevice.SetVANCShiftMode (mInputChannel, NTV2_VANCDATA_8BITSHIFT_ENABLE);	//	8-bit FBFs require VANC bit shift

		//	Set up the circular buffers based on the detected currentVideoFormat...
		AJAStatus	status	(SetupHostBuffers (currentVideoFormat));
		if (AJA_FAILURE (status))
			return;

		if (!mBurnCaptions)								//	In E-E mode...
			SetOutputStandards (currentVideoFormat);	//	...output standard may need changing

		if (mBurnCaptions)
			StartPlayThread ();			//	Start a new playout thread

		mDevice.AutoCirculateStop (mInputChannel);
		{
			AJAAutoLock	autoLock (mLock);	//	Avoid AutoCirculate buffer reservation collisions
			mDevice.AutoCirculateInitForInput (mInputChannel, 7, mAudioSystem, AUTOCIRCULATE_WITH_RP188 | (mForceVanc ? 0 : AUTOCIRCULATE_WITH_ANC));
		}

		//	Start AutoCirculate running...
		mDevice.AutoCirculateStart (mInputChannel);

		while (!mGlobalQuit)
		{
			AUTOCIRCULATE_STATUS	acStatus;
			mDevice.AutoCirculateGetStatus (mInputChannel, acStatus);
			if (acStatus.IsRunning () && acStatus.HasAvailableInputFrame ())
			{
				//	At this point, there's at least one fully-formed frame available in the device's
				//	frame buffer to transfer to the host. Reserve an AVDataBuffer to "produce", and
			//	use it to hold the next frame to be transferred from the device...
				AVDataBuffer *	pCaptureData	(mAVCircularBuffer.StartProduceNextBuffer ());
				if (pCaptureData)
				{
					mInputXferInfo.acFrameBufferFormat = mCaptureFBF;
					mInputXferInfo.SetBuffers (pCaptureData->fVideoBuffer, pCaptureData->fVideoBufferSize,
												pCaptureData->fAudioBuffer, pCaptureData->fAudioBufferSize,
												pCaptureData->fAncBuffer, NTV2_ANCSIZE_MAX,
												pCaptureData->fAncF2Buffer, NTV2_ANCSIZE_MAX);

					//	Transfer the frame data from the device into our host AVDataBuffer...
					mDevice.AutoCirculateTransfer (mInputChannel, mInputXferInfo);

					if (mBurnCaptions)
					{
						pCaptureData->fAudioBufferSize	= mInputXferInfo.GetCapturedAudioByteCount ();	//	Actual audio byte count goes with captured frame
						pCaptureData->fAncBufferSize	= mInputXferInfo.GetAncByteCount (false);		//	Actual F1 anc byte count goes with captured frame
						pCaptureData->fAncF2BufferSize	= mInputXferInfo.GetAncByteCount (true);		//	Actual F2 anc byte count goes with captured frame
						NTV2_RP188	tc;
						mInputXferInfo.GetInputTimeCode (tc, ::NTV2InputSourceToTimecodeIndex (mInputSource, false));		//	Try VITC
						if (!tc.IsValid ())
							mInputXferInfo.GetInputTimeCode (tc, ::NTV2InputSourceToTimecodeIndex (mInputSource, true));	//	Try ATC LTC
						if (tc.IsValid ())
							pCaptureData->fRP188Data = tc;
						else
							pCaptureData->fRP188Data.DBB = pCaptureData->fRP188Data.Low = pCaptureData->fRP188Data.High = 0xFFFFFFFF;
					}

					//	Extract closed-captioning data from the host AVDataBuffer while we have full access
					//	to the buffer, and write the CC data to stdout...
					ExtractClosedCaptionData (currentVideoFormat);

					//	Signal that we're done "producing" the frame, making it available for future "consumption"...
					mAVCircularBuffer.EndProduceNextBuffer ();

					if (!mBurnCaptions)
					{
						//	If no caption burn-in is taking place, there's nobody to consume the buffer.
						//	In this case, simply consume it now, thus recycling it immediately...
						mAVCircularBuffer.StartConsumeNextBuffer ();
						mAVCircularBuffer.EndConsumeNextBuffer ();
					}
				}	//	if pCaptureData != NULL
			}	//	if A/C running and frame(s) are available for transfer
			else
			{
				//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
				//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
				//	efficient to wait for the next input vertical interrupt event to get signaled...
				mDevice.WaitForInputVerticalInterrupt (mInputChannel);

				//	Did incoming video format change?
				if (mDevice.GetInputVideoFormat (mInputSource) != currentVideoFormat)
				{
					cerr << endl << "## WARNING:  Input video format changed" << endl;

					//	These next 2 calls signal the playout thread to allow it to terminate...
					mAVCircularBuffer.StartProduceNextBuffer ();
					mAVCircularBuffer.EndProduceNextBuffer ();
					break;	//	exit frame processing loop
				}
			}
		}	//	normal frame processing loop

		//	Stop AutoCirculate...
		mDevice.AutoCirculateStop (mInputChannel);

	}	//	loop til quit signaled
	cerr << endl << "## NOTE:  CaptureFrames thread exit" << endl;

}	//	CaptureFrames


//////////////////////////////////////////////


const UByte * NTV2CCGrabber::GetVideoData (void) const
{
	return reinterpret_cast <const UByte *> (mInputXferInfo.acVideoBuffer.GetHostPointer ());
}


bool NTV2CCGrabber::DeviceAncExtractorIsAvailable (void)
{
	UWord	majorVersion (0),	minorVersion (0),	pointVersion (0),	buildNumber (0);
	mDevice.GetDriverVersionComponents (majorVersion, minorVersion, pointVersion, buildNumber);
	//	Device Anc extraction requires driver version 12.3 minimum  (or 0.0.0.0 for internal development)...
	if ((majorVersion >= 12 && minorVersion >= 3) || (majorVersion == 0 && minorVersion == 0 && pointVersion == 0 && buildNumber == 0))
		//	The device must also support it...
		if (::NTV2DeviceCanDoCustomAnc (mDeviceID))
			//	And perhaps even do firmware version/date checks??
			return true;
	return false;
}


void NTV2CCGrabber::ToggleVANC (void)
{
	mForceVanc = !mForceVanc;
	cerr	<< endl << "## NOTE:  Now using " << (mForceVanc ? "VANC" : "device Anc extraction") << endl;
}


unsigned NTV2CCGrabber::ExtractClosedCaptionData (const NTV2VideoFormat inVideoFormat)
{
	int				result				(0);
	bool			gotCaptionPacket	(false);
	CaptionData		captionData;		//	The two byte pairs (one pair per field) our 608 decoder is looking for
	ostringstream	oss;				//	DEBUG

	if (!mForceVanc)					//	::NTV2DeviceCanDoCustomAnc (mDeviceID))
	{
		//	See what's in the AUTOCIRCULATE_TRANSFER's F1 & F2 anc buffers...
		AJAAncillaryList	ancPacketsF1, ancPacketsF2;
		ancPacketsF1.SetAnalogAncillaryDataTypeForLine (21, AJAAncillaryDataType_Cea608_Line21);
		ancPacketsF2.SetAnalogAncillaryDataTypeForLine (21, AJAAncillaryDataType_Cea608_Line21);
		ancPacketsF1.AddReceivedAncillaryData ((uint8_t *) mInputXferInfo.acANCBuffer.GetHostPointer (), mInputXferInfo.acANCBuffer.GetByteCount ());
		ancPacketsF2.AddReceivedAncillaryData ((uint8_t *) mInputXferInfo.acANCField2Buffer.GetHostPointer (), mInputXferInfo.acANCField2Buffer.GetByteCount ());
		ancPacketsF1.ParseAllAncillaryData ();
		ancPacketsF2.ParseAllAncillaryData ();

		if (ancPacketsF1.CountAncillaryDataWithType (AJAAncillaryDataType_Cea708))
		{
			AJAAncillaryData	ancCEA708DataIn	(ancPacketsF1.GetAncillaryDataWithType (AJAAncillaryDataType_Cea708));
			if (ancCEA708DataIn.GetPayloadData () && ancCEA708DataIn.GetPayloadByteCount ())
				gotCaptionPacket = m708Decoder->SetSMPTE334AncData (ancCEA708DataIn.GetPayloadData (), ancCEA708DataIn.GetPayloadByteCount ());
			//if (gotCaptionPacket)	oss << "Using F1 AJAAnc CEA708 packet(s)";
		}
		if (ancPacketsF1.CountAncillaryDataWithType (AJAAncillaryDataType_Cea608_Vanc))
		{
			AJAAncillaryData	ancCEA608DataIn	(ancPacketsF1.GetAncillaryDataWithType (AJAAncillaryDataType_Cea608_Vanc));
			if (ancCEA608DataIn.GetPayloadData () && ancCEA608DataIn.GetPayloadByteCount ())
				gotCaptionPacket = m708Decoder->SetSMPTE334AncData (ancCEA608DataIn.GetPayloadData (), ancCEA608DataIn.GetPayloadByteCount ());
			//if (gotCaptionPacket)	oss << "Using F1 AJAAnc CEA608 packet(s)";
		}
		if (ancPacketsF1.CountAncillaryDataWithType (AJAAncillaryDataType_Cea608_Line21))
		{
			AJAAncillaryData_Cea608_Line21	ancEIA608DataIn	(ancPacketsF1.GetAncillaryDataWithType (AJAAncillaryDataType_Cea608_Line21));
			if (AJA_SUCCESS (ancEIA608DataIn.ParsePayloadData ()))
				if (AJA_SUCCESS (ancEIA608DataIn.GetCEA608Bytes (captionData.f1_char1, captionData.f1_char2, captionData.bGotField1Data)))
					gotCaptionPacket = true;
			//if (gotCaptionPacket)	oss << "Using F1 AJAAnc EIA608 packet(s)";
		}
		if (ancPacketsF2.CountAncillaryDataWithType (AJAAncillaryDataType_Cea608_Line21))
		{
			AJAAncillaryData_Cea608_Line21	ancEIA608DataIn	(ancPacketsF2.GetAncillaryDataWithType (AJAAncillaryDataType_Cea608_Line21));
			if (AJA_SUCCESS (ancEIA608DataIn.ParsePayloadData ()))
				if (AJA_SUCCESS (ancEIA608DataIn.GetCEA608Bytes (captionData.f2_char1, captionData.f2_char2, captionData.bGotField2Data)))
					gotCaptionPacket = true;
			//if (gotCaptionPacket)	oss << "Using F2 AJAAnc EIA608 packet(s)";
		}
	}	//	if able to use Anc buffers

	if (NTV2_IS_SD_VIDEO_FORMAT (inVideoFormat) && !gotCaptionPacket)
	{
		NTV2FrameGeometry	fg;
		mDevice.GetFrameGeometry (fg);

		//	Decode the closed-caption data bytes from from the host frame buffer (full-frame, both lines 21 & 22)...
		captionData = m608Decoder->DecodeCaptionData (GetVideoData (), mCaptureFBF, inVideoFormat, fg);
		if (captionData.IsError ())
			mErrorTally++;
		//else	oss << "Using CNTV2CaptionDecoder608::DecodeCaptionData";
	}	//	if SD
	else if (NTV2_IS_HD_VIDEO_FORMAT (inVideoFormat))
	{
		bool	hdCaptionsParsedOkay	(false);
		if (!gotCaptionPacket)
			gotCaptionPacket = m708Decoder->FindSMPTE334AncPacketInVideoFrame (GetVideoData (), inVideoFormat, mCaptureFBF);	//	Look in VANC area

		if (gotCaptionPacket)
			hdCaptionsParsedOkay = m708Decoder->ParseSMPTE334AncPacket ();

		if (gotCaptionPacket && hdCaptionsParsedOkay)
			captionData = m708Decoder->GetCC608CaptionData ();	//	Extract the 608 caption byte pairs
		else
			mErrorTally++;

	}	//	else if HD

	//	This demo only handles 608 captions. (Sorry, no Teletext, IBU, etc. at this time.)
	//	The 608 decoder expects to be called once per frame (to implement flashing characters, smooth-scroll roll-up, etc.).
	//	Pass the caption byte pairs to it for processing (even if captionData.HasData returns false)...
	m608Decoder->ProcessNew608FrameData (captionData);
	//if (!oss.str ().empty ())	cerr << oss.str () << endl;	//	DEBUG DEBUG DEBUG DEBUG

	//	Make the data available to display on standard output stream...
	const bool	showField2	(IsField2Line21CaptionChannel (m608Channel));
	char		char1		(showField2 ? captionData.f2_char1 : captionData.f1_char1);
	char		char2		(showField2 ? captionData.f2_char2 : captionData.f1_char2);
	const bool	gotData		(showField2 ? captionData.bGotField2Data : captionData.bGotField1Data);
	if (gotData && (IsLine21RollUpMode (m608Mode) || !IsValidLine21Mode (m608Mode)))
	{
		char1 &= 0x7F;	//	Strip parity
		char2 &= 0x7F;	//	Strip parity
		if (char1 >= ' ' && char1 <= '~')
		{
			EmitCharacter (string (1, char1));
			if (char2 >= ' ' && char2 <= '~')
				EmitCharacter (string (1, char2));
		}
		else
			EmitCharacter (" ");
	}

	return result;

}	//	ExtractClosedCaptionData


void NTV2CCGrabber::EmitCharacter (const string & inCharacter)
{
	if (mLastChar != " " || inCharacter != " ")
	{
		cout << inCharacter << flush;
		mLastChar = inCharacter;
	}

}	//	EmitCharacter


//////////////////////////////////////////////	P L A Y O U T


static const UWord	gMixerNums []	=	{0, 0, 1, 1, 2, 2, 3, 3};


AJAStatus NTV2CCGrabber::SetupOutputVideo (const NTV2VideoFormat inVideoFormat)
{
	NTV2_ASSERT (mBurnCaptions);

	//	Enable the caption frame store...
	mDevice.EnableChannel (mOutputChannel);

	//	Set the caption frame store for playout, not capture...
	mDevice.SetMode (mOutputChannel, NTV2_MODE_DISPLAY);

	//	Set the caption channel video format...
	mDevice.SetVideoFormat (inVideoFormat,	false,				//	OEM mode, not retail mode
											true,				//	Don't mess with channel 1's VANC settings
											mOutputChannel);	//	Output channel (in case this is a multichannel device)

	//	Set the caption frame store's pixel format (RGBA)...
	mDevice.SetFrameBufferFormat (mOutputChannel, mPlayoutFBF);

	//	RGB:  Set up mixer to "mix" mode, FG raster "unshaped", BG raster "full raster" and VANC pass-thru from BG...
	const UWord	mixerNumber	(gMixerNums [mOutputChannel]);
	mDevice.SetMixerMode (mixerNumber, NTV2MIXERMODE_FOREGROUND_ON);
	mDevice.SetMixerFGInputControl (mixerNumber, NTV2MIXERINPUTCONTROL_UNSHAPED);
	mDevice.SetMixerBGInputControl (mixerNumber, NTV2MIXERINPUTCONTROL_FULLRASTER);
	mDevice.SetMixerVancOutputFromForeground (mixerNumber, false);	//	false means "use BG VANC, not FG"
	cerr	<< "## NOTE:  Caption burn-in using mixer/keyer " << (mixerNumber+1) << " on " << ::NTV2ChannelToString (mOutputChannel)
			<< ", " << ::NTV2FrameBufferFormatToString (mPlayoutFBF)
			<< ", " << ::NTV2VideoFormatToString (inVideoFormat) << endl;

	return AJA_STATUS_SUCCESS;

}	//	SetupOutputVideo


void NTV2CCGrabber::RouteOutputSignal (const NTV2VideoFormat inVideoFormat)
{
	NTV2_ASSERT (mBurnCaptions);			//	Must be burning captions
	const NTV2OutputCrosspointID	frameStoreOutputRGB	(::GetFrameBufferOutputXptFromChannel (mOutputChannel, true));	//	true=RGB
	const NTV2OutputCrosspointID	cscOutputYUV		(::GetCSCOutputXptFromChannel (mOutputChannel));
	const NTV2OutputCrosspointID	cscOutputKey		(::GetCSCOutputXptFromChannel (mOutputChannel, true));	//	true=key
	const NTV2OutputCrosspointID	mixerOutputYUV		(::GetMixerOutputXptFromChannel (mOutputChannel));
	const NTV2OutputCrosspointID	signalInput			(::GetSDIInputOutputXptFromChannel (mInputChannel));
	const NTV2Standard				outputStandard		(::GetNTV2StandardFromVideoFormat (inVideoFormat));

	if (mDoMultiFormat)
	{
		//	Multiformat --- route the one SDI output to the mixer's YUV output, and set its output standard...
		if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
			mDevice.SetSDITransmitEnable (mOutputChannel, true);
		if (::NTV2DeviceCanDoWidget (mDeviceID, g3GSDIOutputs [mOutputChannel]) || ::NTV2DeviceCanDoWidget (mDeviceID, gSDIOutputs [mOutputChannel]))
		{
			mDevice.Connect (::GetSDIOutputInputXpt (mOutputChannel), mixerOutputYUV);
			mDevice.SetSDIOutputStandard (mOutputChannel, outputStandard);
		}
	}
	else
	{
		//	If not multiformat:  Route all SDI outputs to the mixer's YUV output...
		const ULWord		numVideoOutputs	(::NTV2DeviceGetNumVideoOutputs (mDeviceID));
		const NTV2Channel	startNum		(NTV2_CHANNEL1);
		const NTV2Channel	endNum			(NTV2_CHANNEL_INVALID);

		for (NTV2Channel chan (startNum);  chan < endNum;  chan = NTV2Channel (chan + 1))
		{
			if (ULWord (chan) >= numVideoOutputs)
				break;
			if (chan == mInputChannel)
				continue;	//	Skip the input
			if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
				mDevice.SetSDITransmitEnable (chan, true);
			if (::NTV2DeviceCanDoWidget (mDeviceID, g3GSDIOutputs [chan]) || ::NTV2DeviceCanDoWidget (mDeviceID, gSDIOutputs [chan]))
			{
				mDevice.Connect (::GetSDIOutputInputXpt (chan), mixerOutputYUV);
				mDevice.SetSDIOutputStandard (chan, outputStandard);
			}
		}	//	for each output spigot
	}

	mDevice.Connect (::GetCSCInputXptFromChannel (mOutputChannel),	frameStoreOutputRGB);	//	Connect CSC video input to frame buffer's RGB output
	mDevice.Connect (::GetMixerFGInputXpt (mOutputChannel),			cscOutputYUV);			//	Connect mixer's foreground video input to the CSC's YUV video output
	mDevice.Connect (::GetMixerFGInputXpt (mOutputChannel, true),	cscOutputKey);			//	Connect mixer's foreground key input to the CSC's YUV key output
	mDevice.Connect (::GetMixerBGInputXpt (mOutputChannel),			signalInput);			//	Connect mixer's background video input to the SDI input

	if (!mDoMultiFormat)
	{
		//	Connect more outputs -- HDMI, analog, SDI monitor, etc...
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1))
			mDevice.Connect (NTV2_XptHDMIOutInput, mixerOutputYUV);
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v2))
			mDevice.Connect (NTV2_XptHDMIOutQ1Input, mixerOutputYUV);
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, mixerOutputYUV);
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIMonOut1))
			mDevice.Connect (::GetSDIOutputInputXpt (NTV2_CHANNEL5), mixerOutputYUV);
	}

}	//	RouteOutputSignal


//	This is where the play thread will be started
void NTV2CCGrabber::StartPlayThread (void)
{
	//	Create and start the playout thread...
	NTV2_ASSERT (mBurnCaptions);
	mPlayoutThread = new AJAThread ();
	mPlayoutThread->Attach (PlayThreadStatic, this);
	mPlayoutThread->SetPriority (AJA_ThreadPriority_High);
	mPlayoutThread->Start ();

}	//	StartPlayThread


//	The playout thread function
void NTV2CCGrabber::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	//	Grab the NTV2Burn instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2CCGrabber *	pApp	(reinterpret_cast <NTV2CCGrabber *> (pContext));
	pApp->PlayFrames ();

}	//	PlayThreadStatic


void NTV2CCGrabber::PlayFrames (void)
{
	const NTV2VideoFormat	videoFormat		(mDevice.GetInputVideoFormat (mInputSource));
	NTV2VANCMode			vancMode		(NTV2_VANCMODE_INVALID);
	ULWord					fbNum			(10);	//	Bounce between frames 10 & 11
	const string			indicators []	= {"/", "-", "\\", "|", ""};
	AUTOCIRCULATE_STATUS	acStatus;

	SetupOutputVideo (videoFormat);		//	Set up device output
	RouteOutputSignal (videoFormat);	//	Set up output signal routing
	mDevice.GetVANCMode (vancMode, mInputChannel);
	{
		AJAAutoLock	autoLock (mLock);	//	Mutex avoids A/C buffer reservation collisions
		if (mDevice.AutoCirculateInitForOutput (mOutputChannel, 2))	//	Let A/C reserve buffer pair
			if (mDevice.AutoCirculateGetStatus (mOutputChannel, acStatus))	//	Find out which buffers we got
				fbNum = acStatus.acStartFrame;	//	Use them
	}

	const NTV2FormatDescriptor	formatDesc		(::GetFormatDescriptor (videoFormat, mPlayoutFBF, vancMode));
	const ULWord				bytesPerRow		(formatDesc.GetBytesPerRow ());
	const uint32_t				bufferSizeBytes	(formatDesc.GetTotalRasterBytes ());
	const uint32_t				activeSizeBytes	(formatDesc.GetVisibleRasterBytes ());
	ULWord						pingPong		(0);	//	Bounce between 0 and 1
	UByte *						pHostBuffer		(reinterpret_cast <UByte *> (AJAMemory::AllocateAligned (bufferSizeBytes, AJA_PAGE_SIZE)));	//	Top of buffer
	UByte *						pActiveBuffer	(formatDesc.GetTopVisibleRowAddress (pHostBuffer));											//	Points to SAV
	UWord						consecSyncErrs	(0);
	//	Caption status head-up-display...
	static uint64_t				frameTally		(0);
	const string				strVideoFormat	(CNTV2DemoCommon::StripFormatString (::NTV2VideoFormatToString (videoFormat)));
	ULWord						lastErrorTally	(0);

	//	Allocate host frame buffer for blitting captions into...
	if (!pHostBuffer)
		{cerr << "## NOTE:  Caption burn-in failed -- unable to allocate " << bufferSizeBytes << "-byte caption video buffer" << endl;	return;}

	//	Clear it (to fully transparent, all black), then transfer it to both device ping/pong buffers...
	::memset (pHostBuffer, 0, bufferSizeBytes);
	mDevice.DMAWriteFrame (fbNum + 0, reinterpret_cast <ULWord *> (pHostBuffer), bufferSizeBytes);
	mDevice.DMAWriteFrame (fbNum + 1, reinterpret_cast <ULWord *> (pHostBuffer), bufferSizeBytes);
	mDevice.SetOutputFrame (mOutputChannel, fbNum + pingPong);
	pingPong = pingPong ? 0 : 1;

	//	Do forever (until Quit)...
	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		AVDataBuffer *	pPlayData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (pPlayData)
		{
			//	"Burn" captions into the host buffer before it gets sent to the AJA device...
			m608Decoder->BurnCaptions (pActiveBuffer, formatDesc.GetVisibleRasterDimensions (), mPlayoutFBF, bytesPerRow);
			m608Decoder->IdleFrame ();	//	This is needed for captions that flash/blink

			if (mHeadUpDisplayOn)
			{
				ostringstream	oss;
				const string	strCaptionChan	(::NTV2Line21ChannelToStr (NTV2_IS_HD_VIDEO_FORMAT (videoFormat)
													?	m708Decoder->GetDisplayChannel ()
													:	m608Decoder->GetDisplayChannel ()));
				oss << indicators [mCaptionDataTally % 4] << " " << strCaptionChan << " " << frameTally++ << " "
					<< formatDesc.GetRasterWidth() << "x" << formatDesc.GetFullRasterHeight() << (mForceVanc ? "v" : " ") << strVideoFormat;

				const ULWord					newErrorTally	(mErrorTally);
				const NTV2Line21Attributes &	color			(lastErrorTally != newErrorTally ? kRedOnTransparentBG : kGreenOnTransparentBG);
				CNTV2CaptionRenderer::BurnString (oss.str (), color, pActiveBuffer, formatDesc.GetVisibleRasterDimensions(), mPlayoutFBF, bytesPerRow, 7, 1);
				lastErrorTally = newErrorTally;

				const NTV2_RP188	tc	(pPlayData->fRP188Data);
				if (tc.IsValid ()  &&  tc.fDBB & BIT(16))	//	Bit 16 will be set if this frame had timecode
				{
					CRP188	rp188	(tc);
					CNTV2CaptionRenderer::BurnString (string (rp188.GetRP188CString()), kGreenOnTransparentBG, pActiveBuffer,
														formatDesc.GetVisibleRasterDimensions(), mPlayoutFBF, bytesPerRow, 8, 1);
				}
			}

			//	Transfer the caption raster to the device, then ping-pong it into the mixer foreground...
			mDevice.DMAWriteFrame (fbNum + pingPong, reinterpret_cast <ULWord *> (pActiveBuffer), activeSizeBytes);
			mDevice.SetOutputFrame (mOutputChannel, fbNum + pingPong);
			pingPong = pingPong ? 0 : 1;
			::memset (pActiveBuffer, 0, activeSizeBytes);	//	Clear host buffer to fully-transparent, all-black again

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer ();

			//	Check for format change...
			if (videoFormat != mDevice.GetInputVideoFormat (mInputSource))
			{
				cerr	<< "## NOTE:  Caption burn-in stopped due to video format change -- was " << strVideoFormat
						<< ", now " << ::NTV2VideoFormatToString (mDevice.GetInputVideoFormat (mInputSource)) << endl;
				m608Decoder->Reset ();	//	Don't leave captions from old video stream on-screen
				m708Decoder->Reset ();
				break;
			}

			//	Check mixer sync...
			bool	mixerSyncOK	(false);
			mDevice.GetMixerSyncStatus (gMixerNums [mOutputChannel], mixerSyncOK);
			if (mixerSyncOK)
				consecSyncErrs = 0;
			else if (++consecSyncErrs > 60)		//	Bail if lack of mixer sync for longer than ~1 sec
				{cerr << "#MXSYNC#";  consecSyncErrs = 0;}	//	break;}
		}	//	if pPlayData
	}	//	loop til quit signaled

	if (!acStatus.IsStopped ())
		mDevice.AutoCirculateStop (mOutputChannel);
	delete [] pHostBuffer;

}	//	PlayFrames


void NTV2CCGrabber::SetCaptionDisplayChannel (const NTV2Line21Channel inNewChannel)
{
	if (m608Channel != inNewChannel)
	{
		m608Channel = inNewChannel;
		if (m608Decoder)
			m608Decoder->SetDisplayChannel (m608Channel);
		if (m708Decoder)
			m708Decoder->SetDisplayChannel (m608Channel);
	}
}


void NTV2CCGrabber::CaptioningChanged (const NTV2Caption608ChangeInfo & inChangeInfo)
{
	switch (inChangeInfo.mWhatChanged)
	{
		case NTV2Caption608ChangeInfo::NTV2DecoderChange_ScreenCharacter:
			NTV2_ASSERT (inChangeInfo.mChannel == m608Channel);
			if (IsValidLine21Mode (m608Mode) && !IsLine21RollUpMode (m608Mode))
				EmitCharacter (::NTV2CC608CodePointToUtf8String (inChangeInfo.u.screenChar.mNew));
			break;
		case NTV2Caption608ChangeInfo::NTV2DecoderChange_CaptionMode:
			NTV2_ASSERT (inChangeInfo.mChannel == m608Channel);
			m608Mode = NTV2Line21Mode (inChangeInfo.u.captionMode.mNew);
			NTV2_ASSERT (IsValidLine21Mode (m608Mode));
			EmitCharacter (" ");
			break;
		case NTV2Caption608ChangeInfo::NTV2DecoderChange_CurrentRow:
			NTV2_ASSERT (inChangeInfo.mChannel == m608Channel);
			EmitCharacter (" ");
			break;
	}
	mCaptionDataTally++;
}


void NTV2CCGrabber::Caption608ChangedStatic (void * pInstance, const NTV2Caption608ChangeInfo & inChangeInfo)
{
	NTV2CCGrabber *	pGrabber	(reinterpret_cast <NTV2CCGrabber *> (pInstance));
	if (pGrabber)
		pGrabber->CaptioningChanged (inChangeInfo);
}


string NTV2CCGrabber::GetLine21ChannelNames (string inDelimiterStr)		//	static
{
	string	result;
	for (unsigned enumVal (0);  enumVal < NTV2_CC608_ChannelMax; )
	{
		result += ::NTV2Line21ChannelToStr (static_cast <NTV2Line21Channel> (enumVal++));
		if (enumVal < NTV2_CC608_ChannelMax)
			result += inDelimiterStr;
		else
			break;
	}
	return result;

}	//	GetLine21ChannelNames
