/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2ccgrabber.cpp
	@brief		Implementation of NTV2CCGrabber class
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2ccgrabber.h"
#include "ntv2captionrenderer.h"
#include "ntv2line21captioner.h"
#include "ajaanc/includes/ancillarylist.h"
#include "ajaanc/includes/ancillarydata_cea608_line21.h"
#include "ajaanc/includes/ancillarydata_cea608_vanc.h"
#include "ajaanc/includes/ancillarydata_cea708.h"
#include "ntv2debug.h"
#include "ntv2transcode.h"
#include "ajabase/common/types.h"
#include "ajabase/system/memory.h"
#include <iostream>
#include <iomanip>
#include <utility>		//	std::rel_ops
#include <algorithm>	//	set_difference
#include <iterator>		//	for inserter

using namespace std;
using namespace std::rel_ops;

#define AsConstUBytePtr(__p__)		reinterpret_cast<const UByte*>(__p__)
#define AsConstULWordPtr(__p__)		reinterpret_cast<const ULWord*>(__p__)
#define AsULWordPtr(__p__)			reinterpret_cast<ULWord*>(__p__)
#define AsUBytePtr(__p__)			reinterpret_cast<UByte*>(__p__)
//#define NTV2_BUFFER_LOCK			//	Define this to use buffer locking in kernel driver

static const ULWord					kAppSignature				NTV2_FOURCC ('C','C','G','R');
static const NTV2Line21Attributes	kRedOnTransparentBG			(NTV2_CC608_Red, NTV2_CC608_Blue, NTV2_CC608_Transparent);
static const NTV2Line21Attributes	kGreenOnTransparentBG		(NTV2_CC608_Green, NTV2_CC608_Blue, NTV2_CC608_Transparent);
static const uint32_t				MAX_ACCUM_FRAME_COUNT		(30);	//	At least every second?


NTV2CCGrabber::NTV2CCGrabber (const CCGrabberConfig & inConfigData)

	:	mConfig				(inConfigData),
		mCaptureThread		(AJAThread()),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mAudioSystem		(mConfig.fWithAudio ? NTV2_AUDIOSYSTEM_1 : NTV2_AUDIOSYSTEM_INVALID),
		mVancMode			(NTV2_VANCMODE_INVALID),
		mGlobalQuit			(false),
		mSquares			(false),
		mLastOutStr			(),
		mLastOutFrame		(0),
		mErrorTally			(0),
		mCaptionDataTally	(0),
		m608Channel			(mConfig.fCaptionChannel),
		m608Mode			(NTV2_CC608_CapModeUnknown),
		m608Decoder			(),
		mVPIDInfoDS1		(0),
		mVPIDInfoDS2		(0),
		mInputFrameStores	(),
		mActiveSDIInputs	(),
		mActiveCSCs			(),
		mInputConnections	(),
		mHostBuffers		(),
		mCircularBuffer		(),
		mInputXferInfo		(),
		mHeadUpDisplayOn	(true),
		mOutputChannel		(NTV2_CHANNEL_INVALID),
		mPlayoutFBF			(NTV2_FBF_ARGB),
		mPlayoutThread		(AJAThread()),
		mOutputConnections	()
{
	CNTV2CaptionDecoder608::Create(m608Decoder);
	CNTV2CaptionDecoder708::Create(m708DecoderAnc);
	CNTV2CaptionDecoder708::Create(m708DecoderVanc);
	NTV2_ASSERT (m608Decoder && m708DecoderAnc && m708DecoderVanc);

}	//	constructor


NTV2CCGrabber::~NTV2CCGrabber ()
{
	if (m608Decoder)
		m608Decoder->UnsubscribeChangeNotification (Caption608ChangedStatic, this);

	//	Stop my capture thread, then destroy it...
	Quit();

	//	Unsubscribe from input vertical event...
	mDevice.UnsubscribeInputVerticalEvent(mInputFrameStores);

	ReleaseHostBuffers();

	if (!mConfig.fDoMultiFormat)
	{
		mDevice.SetEveryFrameServices(mSavedTaskMode);
		mDevice.ReleaseStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));
	}

}	//	destructor


void NTV2CCGrabber::Quit (void)
{
	//	Set the 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mCaptureThread.Active())
		AJATime::Sleep(10);

	while (mPlayoutThread.Active())
		AJATime::Sleep(10);

	if (!mConfig.fDoMultiFormat)
		mDevice.ClearRouting();

}	//	Quit


AJAStatus NTV2CCGrabber::Init (void)
{
	CNTV2DemoCommon::SetDefaultPageSize();	//	Set host-specific page size

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(mConfig.fDeviceSpec, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpec << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	mDeviceID = mDevice.GetDeviceID();	//	Keep this handy because it's used frequently
	const string deviceStr(::NTV2DeviceIDToString(mDeviceID));

	if (mConfig.fBurnCaptions)
	{
		if (mDevice.features().GetNumFrameStores() < 2)						//	Need 2+ frame stores
		{
			cerr	<< "## ERROR:  Device '" << deviceStr << "' can't burn-in captions because at least 2 frame stores are required" << endl;
			return AJA_STATUS_FAIL;
		}
		if (!mDevice.features().CanDoPlayback())
		{
			cerr	<< "## ERROR:  Device '" << deviceStr << "' can't burn-in captions because device can only capture/ingest" << endl;
			return AJA_STATUS_FAIL;
		}
		if (mDevice.features().GetNumVideoInputs() < 1  ||  mDevice.features().GetNumVideoOutputs() < 1)	//	Need 1 input & 1 output
		{
			cerr	<< "## ERROR:  Device '" << deviceStr << "' can't be used -- at least 1 SDI input and 1 SDI output required" << endl;
			return AJA_STATUS_FAIL;
		}
		if (!mDevice.features().GetNumMixers())								//	Need 1 or more mixers
		{
			cerr	<< "## ERROR:  Device '" << deviceStr << "' can't burn-in captions because a mixer/keyer widget is required" << endl;
			return AJA_STATUS_FAIL;
		}
		if (!mDevice.features().CanDoFrameBufferFormat(mPlayoutFBF))		//	FrameStore must handle 8-bit RGB with alpha
		{
			cerr	<< "## ERROR:  Device '" << deviceStr << "' can't burn-in captions because it can't do 8-bit RGB (with alpha)" << endl;
			return AJA_STATUS_FAIL;
		}
	}	//	if --burn

	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
			{cerr << "## ERROR:  Cannot acquire -- device busy" << endl;  return AJA_STATUS_BUSY;}	//	Some other app owns the device
		mDevice.GetEveryFrameServices(mSavedTaskMode);	//	Save the current task mode
	}

	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);		//	Use the OEM service level

	if (mDevice.features().CanDoMultiFormat())
		mDevice.SetMultiFormatMode(mConfig.fDoMultiFormat);

	if (!mConfig.fUseVanc  &&  !mDevice.features().CanDoCustomAnc())	//	--vanc not specified  &&  no anc extractors?
	{
		cerr << "## WARNING:  Enabling VANC because no Anc extractors" << endl;
		mConfig.fUseVanc = true;
	}

	//	Set up the input video...
	AJAStatus status = SetupInputVideo();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the audio...
	status = SetupAudio();
	if (AJA_FAILURE(status))
		return status;

	if (!m608Decoder->SubscribeChangeNotification(Caption608ChangedStatic, this))
		{cerr << "## WARNING:  SubscribeChangeNotification failed" << endl;	return AJA_STATUS_FAIL;}
	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#endif	//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2CCGrabber::SetupHostBuffers (const NTV2VideoFormat inVideoFormat)
{
	NTV2VANCMode vancMode (NTV2_VANCMODE_INVALID);
	mDevice.GetVANCMode (vancMode, mConfig.fInputChannel);
	const ULWord captureBufferSize (::GetVideoWriteSize (inVideoFormat, mConfig.fPixelFormat, vancMode));

	ReleaseHostBuffers();
	mCircularBuffer.SetAbortFlag (&mGlobalQuit);

	//	Allocate and add each NTV2FrameData to my circular buffer member variable...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());		//	Make a new NTV2FrameData...
		NTV2FrameData & frameData(mHostBuffers.back());	//	...and get a reference to it
		//	Allocate a page-aligned video buffer
		if (!frameData.fVideoBuffer.Allocate(captureBufferSize, /*pageAlign?*/true))
			return AJA_STATUS_MEMORY;
		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
			if (!frameData.fAudioBuffer.Allocate(NTV2_AUDIOSIZE_MAX, /*pageAlign?*/true))
				return AJA_STATUS_MEMORY;
		//	Anc data buffers --- only used if Anc extractors supported...
		if (mDevice.features().CanDoCustomAnc())
		{
			if (!frameData.fAncBuffer.Allocate(NTV2_ANCSIZE_MAX, /*pageAlign?*/true))
				return AJA_STATUS_MEMORY;
			if (!frameData.fAncBuffer2.Allocate(NTV2_ANCSIZE_MAX, /*pageAlign?*/true))
				return AJA_STATUS_MEMORY;
		}
		#if defined(NTV2_BUFFER_LOCK)
			frameData.LockAll(mDevice);
		#endif
		mCircularBuffer.Add(&frameData);	//	Add to my circular buffer
	}	//	for each NTV2FrameData

	return AJA_STATUS_SUCCESS;

}	//	SetupHostBuffers


void NTV2CCGrabber::ReleaseHostBuffers (void)
{
	//	Unlock each in-host NTV2FrameData...
	#if defined(NTV2_BUFFER_LOCK)
		for (size_t ndx(0);  ndx < mHostBuffers.size();  ndx++)
		{
			NTV2FrameData & frameData(mHostBuffers.at(ndx));
			frameData.UnlockAll(mDevice);
		}
	#endif	// NTV2_BUFFER_LOCK
	mHostBuffers.clear();
	mCircularBuffer.Clear();
	return;

}	//	ReleaseHostBuffers


AJAStatus NTV2CCGrabber::SetupInputVideo (void)
{
	bool				isTransmit		(false);
	const UWord			numFrameStores	(mDevice.features().GetNumFrameStores());
	const NTV2DeviceID	deviceID		(mDevice.GetDeviceID());
	const string		deviceName		(::NTV2DeviceIDToString(deviceID));
	const string		channelName		(::NTV2ChannelToString(mConfig.fInputChannel, true));

	//	CCGrabber is SDI only
	//	User must have specified at least --input or --channel
	if (!NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource)  &&  !NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel))
		{cerr << "## ERROR:  Must specify at least input source or input channel" << endl;  return AJA_STATUS_FAIL;}
	else if (!NTV2_IS_VALID_INPUT_SOURCE(mConfig.fInputSource))
		mConfig.fInputSource = ::NTV2ChannelToInputSource(mConfig.fInputChannel);
	else if (!NTV2_IS_VALID_CHANNEL(mConfig.fInputChannel))
		mConfig.fInputChannel = ::NTV2InputSourceToChannel(mConfig.fInputSource);

	//	Check input source
	const string	inputName	(::NTV2InputSourceToString(mConfig.fInputSource,true));
	if (!mDevice.features().CanDoInputSource(mConfig.fInputSource))
		{cerr << "## ERROR:  '" << deviceName << "' cannot grab captions from '" << inputName << "'" << endl;  return AJA_STATUS_FAIL;}
	if (!NTV2_INPUT_SOURCE_IS_SDI(mConfig.fInputSource))
		{cerr << "## ERROR:  Input '" << inputName << "' not SDI" << endl;  return AJA_STATUS_FAIL;}

	//	Check channel
	if (numFrameStores < UWord(mConfig.fInputChannel+1))
		{cerr << "## ERROR:  " << deviceName << " has no " << channelName << " input" << endl;  return AJA_STATUS_FAIL;}

	const NTV2Channel sdiInput(::NTV2InputSourceToChannel(mConfig.fInputSource));
	mActiveSDIInputs.insert(sdiInput);
	if (mDevice.features().HasBiDirectionalSDI())								//	If device has bidirectional SDI connectors...
		if (mDevice.GetSDITransmitEnable(sdiInput, isTransmit))					//	...and GetSDITransmitEnable succeeds...
			if (isTransmit)														//	...and input is set to "transmit"...
			{
				mDevice.SetSDITransmitEnable(sdiInput, false);					//	...then disable transmit mode...
				mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1, 12);		//	...and give the device some time to lock to a signal
			}	//	if input SDI connector needs to switch from transmit mode

	NTV2ReferenceSource	refSrc(NTV2_REFERENCE_INVALID);
	mDevice.GetReference(refSrc);
	if (mConfig.fBurnCaptions  ||  (!mConfig.fBurnCaptions && !mConfig.fDoMultiFormat))
	{
		//	Set the device output clock reference to the SDI input.
		//	This is necessary only when...
		//		-	Doing caption burn-in, which requires syncing the Mixer input to the SDI input; 
		//		-	Routing E-E, which is only done when not doing burn-in and not doing multi-format mode.
		refSrc = ::NTV2InputSourceToReferenceSource(mConfig.fInputSource);
		mDevice.SetReference(refSrc);
	}
	if (mConfig.fBurnCaptions)	//	Caption burn-in/playout requires choosing an output channel
		switch (mConfig.fInputChannel)
		{
			case NTV2_CHANNEL1:	mOutputChannel = (numFrameStores == 2 || numFrameStores > 4) ? NTV2_CHANNEL2 : NTV2_CHANNEL3;	break;
			case NTV2_CHANNEL2:	mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL3 : NTV2_CHANNEL4;							break;
			case NTV2_CHANNEL3:	mOutputChannel = NTV2_CHANNEL4;																	break;
			case NTV2_CHANNEL4:	mOutputChannel = (numFrameStores > 4) ? NTV2_CHANNEL5 : NTV2_CHANNEL3;							break;
			case NTV2_CHANNEL5: mOutputChannel = NTV2_CHANNEL6;																	break;
			case NTV2_CHANNEL6:	mOutputChannel = NTV2_CHANNEL7;																	break;
			case NTV2_CHANNEL7:	mOutputChannel = NTV2_CHANNEL8;																	break;
			case NTV2_CHANNEL8:	mOutputChannel = NTV2_CHANNEL7;																	break;
			case NTV2_CHANNEL_INVALID:	return AJA_STATUS_BAD_PARAM;
		}

	if (!mDevice.features().CanDoFrameBufferFormat(mConfig.fPixelFormat))
		{cerr << "## ERROR:  '" << deviceName << "' doesn't support '" << ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat) << "'" << endl;  return AJA_STATUS_FAIL;}

	//	"Tune" the 608 decoder to the desired channel...
	m608Decoder->SetDisplayChannel(m608Channel);
	m708DecoderAnc->SetDisplayChannel(m608Channel);
	m708DecoderVanc->SetDisplayChannel(m608Channel);

	cerr	<< "## NOTE:  Using " << deviceName << " " << (mConfig.fUseVanc ? "VANC" : "AncExt") << " with spigot=" << inputName
			<< ", ref=" << ::NTV2ReferenceSourceToString(refSrc,true) << ", input=" << channelName;
	if (mConfig.fBurnCaptions)	cerr << ", output=" << ::NTV2ChannelToString(mOutputChannel,true);
	cerr	<< endl;

	return AJA_STATUS_SUCCESS;

}	//	SetupInputVideo


AJAStatus NTV2CCGrabber::SetupAudio (void)
{
	if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
	{
		const UWord	numAudioSystems	(mDevice.features().GetNumAudioSystems());
		if (numAudioSystems > 1  &&  UWord(mConfig.fInputChannel) < numAudioSystems)
			mAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fInputChannel);

		//	Configure the audio system...
		mDevice.SetAudioSystemInputSource (mAudioSystem, NTV2_AUDIO_EMBEDDED, ::NTV2InputSourceToEmbeddedAudioInput(mConfig.fInputSource));
		mDevice.SetNumberAudioChannels (mDevice.features().GetMaxAudioChannels(), mAudioSystem);
		mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);
		mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

		//	Set up the output audio embedders...
		if (mConfig.fBurnCaptions  &&  numAudioSystems > 1)
		{
			if (mConfig.fDoMultiFormat)
			{
				UWord sdiOutput(mOutputChannel);
				if (sdiOutput >= mDevice.features().GetNumVideoOutputs())		//	Kona1 has 2 FrameStores/Channels, but only 1 SDI output
					sdiOutput = mDevice.features().GetNumVideoOutputs() - 1;
				mDevice.SetSDIOutputAudioSystem(NTV2Channel(sdiOutput), mAudioSystem);
			}
			else	//	Have all device outputs use the same audio system...
				for (unsigned sdiOutput(0);  sdiOutput < mDevice.features().GetNumVideoOutputs();  sdiOutput++)
					if (!mDevice.features().HasBiDirectionalSDI()
						||  NTV2Channel(sdiOutput) != ::NTV2InputSourceToChannel(mConfig.fInputSource))
							mDevice.SetSDIOutputAudioSystem(NTV2Channel(sdiOutput), mAudioSystem);
		}

		//	Loopback mode (E-E audio) should be enabled only when not burning-in captions (i.e. when routing E-E).
		//	If caption burn-in and loopback are enabled, output video will lag the audio, as video frames are delayed
		//	in the ring buffer.
		mDevice.SetAudioLoopBack (mConfig.fBurnCaptions ? NTV2_AUDIO_LOOPBACK_ON : NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);
	}
	return AJA_STATUS_SUCCESS;

}	//	SetupAudio


static const NTV2WidgetID	gSDIOutputs[]	= {	NTV2_WgtSDIOut1,	NTV2_WgtSDIOut2,	NTV2_WgtSDIOut3,	NTV2_WgtSDIOut4,
												NTV2_Wgt3GSDIOut5,	NTV2_Wgt3GSDIOut6,	NTV2_Wgt3GSDIOut7,	NTV2_Wgt3GSDIOut8	};

static const NTV2WidgetID	g3GSDIOutputs[]	= {	NTV2_Wgt3GSDIOut1,	NTV2_Wgt3GSDIOut2,	NTV2_Wgt3GSDIOut3,	NTV2_Wgt3GSDIOut4,
												NTV2_Wgt3GSDIOut5,	NTV2_Wgt3GSDIOut6,	NTV2_Wgt3GSDIOut7,	NTV2_Wgt3GSDIOut8	};

static const NTV2WidgetID	g12GSDIOutputs[]= {	NTV2_Wgt12GSDIOut1,	NTV2_Wgt12GSDIOut2,	NTV2_Wgt12GSDIOut3,	NTV2_Wgt12GSDIOut4,
												NTV2_WIDGET_INVALID, NTV2_WIDGET_INVALID, NTV2_WIDGET_INVALID, NTV2_WIDGET_INVALID	};


bool NTV2CCGrabber::RouteInputSignal (const NTV2VideoFormat inVideoFormat)
{
	const bool				isRGBFBF		(::IsRGBFormat(mConfig.fPixelFormat));
	const bool				isRGBWire		(mVPIDInfoDS1.IsRGBSampling());
	const bool				isSquares		(!mVPIDInfoDS1.IsStandardTwoSampleInterleave());
	const bool				is4KHFR			(NTV2_IS_HFR_STANDARD(::GetNTV2StandardFromVideoFormat(inVideoFormat)));
	const NTV2Channel		sdiInput		(::NTV2InputSourceToChannel(mConfig.fInputSource));
	const NTV2ChannelList	frameStores		(::NTV2MakeChannelList(mInputFrameStores));
	NTV2ChannelList			sdiInputs		(::NTV2MakeChannelList(mActiveSDIInputs));
	const NTV2ChannelList	activeCSCs		(::NTV2MakeChannelList(mActiveCSCs));
	static const bool		ShowRoutingProgress(false);

	mInputConnections.clear();
	if (ShowRoutingProgress) mDevice.ClearRouting();

	//	Route frameStore's input to sdiInput's output (possibly through CSC, if required)
	if (isRGBFBF && isRGBWire)
	{
		if (isSquares)	//	SDIIn ==> DLIn ==> FrameStore
		{
			for (size_t ndx(0);  ndx < sdiInputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx)), sdiIn(sdiInputs.at(ndx));
				mInputConnections.insert(NTV2XptConnection(::GetDLInInputXptFromChannel(sdiIn, /*linkB?*/false),
															::GetSDIInputOutputXptFromChannel(sdiIn, /*DS2?*/false)));
				mInputConnections.insert(NTV2XptConnection(::GetDLInInputXptFromChannel(sdiIn, /*linkB?*/true),
															::GetSDIInputOutputXptFromChannel(sdiIn, /*DS2?*/true)));
				mInputConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(sdiIn),
															::GetDLInOutputXptFromChannel(frmSt)));
				if (ShowRoutingProgress) mDevice.ApplySignalRoute(mInputConnections);
			}
		}
		else	// TSI	//	2x SDIIn ==> 2x DLIn ==> 425MUX ==> RGBFrameStore
		{
			NTV2ChannelSet sdiIns	= ::NTV2MakeChannelSet(sdiInput, UWord(2*frameStores.size()));
			sdiInputs	= ::NTV2MakeChannelList(sdiIns);
			NTV2ChannelList tsiMuxes = CNTV2DemoCommon::GetTSIMuxesForFrameStore(mDevice, frameStores.at(0),
																				UWord(frameStores.size()*2));
			mDevice.SetSDITransmitEnable(sdiIns, true);	//	Gotta do this again, since sdiInputs changed
			for (size_t ndx(0);  ndx < sdiInputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx/2)), tsiMux(tsiMuxes.at(ndx/2)), sdiIn(sdiInputs.at(ndx));
				mInputConnections.insert(NTV2XptConnection(::GetDLInInputXptFromChannel(sdiIn, /*DS2?*/false),
															::GetSDIInputOutputXptFromChannel(sdiIn, /*DS2?*/false)));
				mInputConnections.insert(NTV2XptConnection(::GetDLInInputXptFromChannel(sdiIn, /*DS2?*/true),
															::GetSDIInputOutputXptFromChannel(sdiIn, /*DS2?*/true)));
				mInputConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux, /*lnkB?*/ndx & 1),
															::GetDLInOutputXptFromChannel(sdiIn)));
				mInputConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(frmSt, /*lnkB?*/false),
															::GetTSIMuxOutputXptFromChannel(tsiMux,/*lnkB?*/false, /*rgb?*/true)));
				mInputConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(frmSt, /*lnkB?*/true),
															::GetTSIMuxOutputXptFromChannel(tsiMux,/*lnkB?*/true, /*rgb?*/true)));
				if (ShowRoutingProgress) mDevice.ApplySignalRoute(mInputConnections);
			}
		}
	}
	else if (isRGBFBF && !isRGBWire)
	{
		if (isSquares)	//	SDIIn ==> CSC ==> RGBFrameStore
			for (size_t ndx(0);  ndx < sdiInputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx)), sdiIn(sdiInputs.at(ndx));
				mInputConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(frmSt),
															::GetCSCOutputXptFromChannel(frmSt, /*key?*/false, /*RGB?*/true)));
				mInputConnections.insert(NTV2XptConnection(::GetCSCInputXptFromChannel(frmSt),
															::GetSDIInputOutputXptFromChannel(sdiIn)));
			}
		else	// TSI	//	SDIIn ==> 2 x CSC ==> 425MUX ==> RGBFrameStore
		{
			NTV2ChannelSet sdiIns		= ::NTV2MakeChannelSet(sdiInput, UWord(frameStores.size()));
			sdiInputs					= ::NTV2MakeChannelList(sdiIns);
			NTV2ChannelList cscs		= ::NTV2MakeChannelList(frameStores.at(0) > NTV2_CHANNEL4 ? NTV2_CHANNEL5 : NTV2_CHANNEL1,
																UWord(2*sdiInputs.size()));
			NTV2ChannelList tsiMuxes	= CNTV2DemoCommon::GetTSIMuxesForFrameStore(mDevice, frameStores.at(0),
																					UWord(frameStores.size()));
			cerr	<< "FrameStores: "	<< ::NTV2ChannelListToStr(frameStores)	<< endl
					<< "SDIInputs: "	<< ::NTV2ChannelListToStr(sdiInputs)	<< endl
					<< "TSIMuxers: "	<< ::NTV2ChannelListToStr(tsiMuxes)		<< endl;
			mDevice.SetSDITransmitEnable(sdiIns, false);	//	Gotta do this again, since sdiIns changed
			for (size_t ndx(0);  ndx < cscs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx/2)), sdiIn(sdiInputs.at(ndx/2)), tsiMux(tsiMuxes.at(ndx/2)),
							csc(cscs.at(ndx));
				mInputConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(frmSt, /*inputB?*/ndx & 1),
															::GetTSIMuxOutputXptFromChannel(tsiMux,/*linkB?*/ndx & 1,/*rgb?*/true)));
				mInputConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux, /*linkB?*/ndx & 1),
															::GetCSCOutputXptFromChannel(csc, /*key?*/false, /*rgb?*/true)));
				mInputConnections.insert(NTV2XptConnection(::GetCSCInputXptFromChannel(csc),
															::GetSDIInputOutputXptFromChannel(sdiIn, /*DS2?*/ndx & 1)));
			}
		}
		if (ShowRoutingProgress) mDevice.ApplySignalRoute(mInputConnections);
	}
	else if (!isRGBFBF && isRGBWire)
	{
		if (isSquares)	//	SDIIn ==> DLIn ==> CSC ==> FrameStore
		{
			for (size_t ndx(0);  ndx < sdiInputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx)), csc(activeCSCs.at(ndx)), sdi(sdiInputs.at(ndx));
				mInputConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(frmSt, /*BInput?*/false),
															::GetCSCOutputXptFromChannel(csc)));
				mInputConnections.insert(NTV2XptConnection(::GetCSCInputXptFromChannel(csc),
															::GetDLInOutputXptFromChannel(sdi)));
				mInputConnections.insert(NTV2XptConnection(::GetDLInInputXptFromChannel(sdi, /*lnkB?*/false),
															::GetSDIInputOutputXptFromChannel(sdi, /*DS2?*/false)));
				mInputConnections.insert(NTV2XptConnection(::GetDLInInputXptFromChannel(sdi, /*lnkB?*/true),
															::GetSDIInputOutputXptFromChannel(sdi, /*DS2?*/true)));
				if (ShowRoutingProgress) mDevice.ApplySignalRoute(mInputConnections);
			}
		}
		else	// TSI	//	SDIIn ==> DLIn ==> CSC ==> TSIMux ==> FrameStore
		{
		//TBD
		}
	}
	else	//	SDIIn ==> YUVFrameStore
	{
		if (isSquares)	//	SDIIn ==> FrameStore
		{
			for (size_t ndx(0);  ndx < sdiInputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(ndx)), sdiIn(sdiInputs.at(ndx));
				mInputConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(frmSt),
															::GetSDIInputOutputXptFromChannel(sdiIn)));
				if (ShowRoutingProgress) mDevice.ApplySignalRoute(mInputConnections);
			}
		}
		else	// TSI	//	SDIIn (LFR: 2 x DS1&DS2, 4KHFR: 4 x DS1) ==> 425MUX ==> YUVFrameStore
		{
			NTV2XptConnections & mConnections(mInputConnections);
			sdiInputs	= ::NTV2MakeChannelList(sdiInputs.at(0), is4KHFR ? 4 : UWord(frameStores.size()));
			NTV2ChannelList tsiMuxes	= CNTV2DemoCommon::GetTSIMuxesForFrameStore(mDevice, frameStores.at(0),
																					UWord(frameStores.size()));
			cerr	<< (is4KHFR ? "4KHFR" : "")	<< endl	<< "FrameStores: " << ::NTV2ChannelListToStr(frameStores) << endl
					<< "SDIInputs:   " << ::NTV2ChannelListToStr(sdiInputs) << endl << "TSIMuxes:    "	<< ::NTV2ChannelListToStr(tsiMuxes) << endl;
			for (size_t ndx(0);  ndx < sdiInputs.size();  ndx++)
			{	NTV2Channel frmSt(frameStores.at(is4KHFR ? ndx/2 : ndx)),
							tsiMux(tsiMuxes.at(is4KHFR ? ndx/2 : ndx)),
							sdiIn(sdiInputs.at(ndx));
				mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux, /*linkB?*/is4KHFR && (ndx & 1)),
									::GetSDIInputOutputXptFromChannel(sdiIn, /*DS2?*/false)));
				if (!is4KHFR)
					mConnections.insert(NTV2XptConnection(::GetTSIMuxInputXptFromChannel(tsiMux, /*linkB?*/true),
										::GetSDIInputOutputXptFromChannel(sdiIn, /*DS2?*/true)));
				mConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(frmSt, /*inputB?*/is4KHFR && (ndx & 1)),
									::GetTSIMuxOutputXptFromChannel(tsiMux,/*linkB?*/is4KHFR && (ndx & 1))));
				if (!is4KHFR)
					mConnections.insert(NTV2XptConnection(::GetFrameBufferInputXptFromChannel(frmSt, /*inputB?*/true),
										::GetTSIMuxOutputXptFromChannel(tsiMux,/*linkB?*/true)));
				if (ShowRoutingProgress) mDevice.ApplySignalRoute(mInputConnections);
			}
		}
		if (ShowRoutingProgress) mDevice.ApplySignalRoute(mInputConnections);
	}
	//	At this point, sdiInputs is authoritative, so update mActiveSDIInputs...
	mActiveSDIInputs = ::NTV2MakeChannelSet(sdiInputs);

	//	E-E ROUTING
	if ((false) /* not ready for prime-time */ &&  !mConfig.fBurnCaptions  &&  !mConfig.fDoMultiFormat)
	{	//	Not doing caption burn-in:  route E-E pass-thru...
		NTV2ChannelList sdiOutputs;
		if (mDevice.features().HasBiDirectionalSDI())
		{
			const NTV2ChannelSet allSDIs (::NTV2MakeChannelSet(NTV2_CHANNEL1, mDevice.features().GetNumVideoOutputs()));
			NTV2ChannelSet sdiOuts;
			set_difference (allSDIs.begin(), allSDIs.end(),						//	allSDIs
							mActiveSDIInputs.begin(), mActiveSDIInputs.end(),	//		- mActiveSDIInputs
							inserter(sdiOuts, sdiOuts.begin()));				//			==> sdiOuts
			sdiOutputs = ::NTV2MakeChannelList(sdiOuts);
			cerr << "allSDIs: " << ::NTV2ChannelSetToStr(allSDIs) << endl << "actSDIIns: " << ::NTV2ChannelSetToStr(mActiveSDIInputs) << endl;	//	DEBUG
		}
		else
			sdiOutputs = ::NTV2MakeChannelList(NTV2_CHANNEL1, mDevice.features().GetNumVideoOutputs());
		cerr << "SDIOuts: " << ::NTV2ChannelListToStr(sdiOutputs) << endl << "SDIIns: " << ::NTV2ChannelListToStr(sdiInputs) << endl;	//	DEBUG
		for (size_t sdiNdx(0);  sdiNdx < sdiOutputs.size();  sdiNdx++)
		{	const NTV2Channel sdiOut(sdiOutputs.at(sdiNdx));
			const NTV2Channel sdiIn(sdiInputs.at(sdiNdx < sdiInputs.size() ? sdiNdx : sdiInputs.size()-1));
			if (mDevice.features().HasBiDirectionalSDI()  &&  mActiveSDIInputs.find(sdiOut) == mActiveSDIInputs.end())
			{
				cerr << "Switching SDI " << (sdiOut+1) << " to output" << endl;	//	DEBUG
				mDevice.SetSDITransmitEnable(sdiOut, true);
			}
			if (isRGBWire)
			{	//	Route DLIn output to DLOut...
				mInputConnections.insert(NTV2XptConnection(::GetDLOutInputXptFromChannel(sdiOut),
															::GetDLInOutputXptFromChannel(sdiIn)));
				mInputConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut,false),
															::GetDLOutOutputXptFromChannel(sdiOut,false)));
				mInputConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut,true),
															::GetDLOutOutputXptFromChannel(sdiOut,true)));
			}
			else
				mInputConnections.insert(NTV2XptConnection(::GetSDIOutputInputXpt(sdiOut),
															::GetSDIInputOutputXptFromChannel(sdiIn)));
			if (ShowRoutingProgress) mDevice.ApplySignalRoute(mInputConnections);
		}	//	for each output spigot
	}	//	if not burning captions and not multiFormat

	return mDevice.ApplySignalRoute(mInputConnections, /*replaceExistingRouting?*/!mConfig.fDoMultiFormat);

}	//	RouteInputSignal


void NTV2CCGrabber::SetOutputStandards (const NTV2VideoFormat inVideoFormat)
{
	NTV2_ASSERT (!mConfig.fBurnCaptions);		//	Must not be burning captions

	const NTV2Channel	sdiInputAsChan	(::NTV2InputSourceToChannel(mConfig.fInputSource));
	const NTV2Standard	outputStandard	(::GetNTV2StandardFromVideoFormat(inVideoFormat));
	const NTV2Channel	startNum		(sdiInputAsChan == NTV2_CHANNEL1 ? NTV2_CHANNEL1 : NTV2_CHANNEL5);
	const NTV2Channel	endNum			(sdiInputAsChan == NTV2_CHANNEL1 ? NTV2_CHANNEL5 : NTV2_MAX_NUM_CHANNELS);

	for (NTV2Channel chan(startNum);  chan < endNum;  chan = NTV2Channel(chan + 1))
	{
		if (ULWord(chan) >= mDevice.features().GetNumVideoChannels())
			break;
		if (chan != sdiInputAsChan)
			if (mDevice.features().CanDoWidget(g3GSDIOutputs[chan])  ||  mDevice.features().CanDoWidget(gSDIOutputs[chan]))
				mDevice.SetSDIOutputStandard(chan, outputStandard);
	}	//	for each output spigot

}	//	SetOutputStandards


AJAStatus NTV2CCGrabber::Run (void)
{
	//	Start the capture thread...
	AJAStatus result (StartCaptureThread());
	if (AJA_FAILURE(result))
		return result;

	AJATime::Sleep(500);	//	Wait a half second for the capture thread to start (or fail?)...
	return IsCaptureThreadRunning() ? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL;

}	//	Run



//////////////////////////////////////////////

//	Starts the capture thread
AJAStatus NTV2CCGrabber::StartCaptureThread (void)
{
	//	Create and start the capture thread...
	AJAStatus result (mCaptureThread.Attach(CaptureThreadStatic, this));
	if (AJA_SUCCESS(result))
		result = mCaptureThread.SetPriority(AJA_ThreadPriority_High);
	if (AJA_SUCCESS(result))
		result = mCaptureThread.Start();
	return result;

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
	ULWord xferTally(0), xferFails(0), noVideoTally(0), waitTally(0);
	bool bUsingVanc (mConfig.fUseVanc);		//	To detect fUseVanc changes
	CAPNOTE("Thread started");
	NTV2_ASSERT(!mActiveSDIInputs.empty());

	//	Loop until time to quit...
	while (!mGlobalQuit)
	{
		NTV2PixelFormat	currentPF (mConfig.fPixelFormat);
		NTV2VideoFormat	currentVF (WaitForStableInputSignal());
		if (currentVF == NTV2_FORMAT_UNKNOWN)
			break;	//	Quit

		//	At this point, the input video format is stable.

		//	Configure the input FrameStore(s):  pixel format, mode...
		mDevice.EnableChannels(mInputFrameStores);
		mDevice.SetFrameBufferFormat(mInputFrameStores, mConfig.fPixelFormat);

		//	Enable and subscribe to the interrupts for the channel to be used...
		mDevice.EnableInputInterrupt (mInputFrameStores);
		mDevice.SubscribeInputVerticalEvent (mInputFrameStores);

		//	Set up the device signal routing...
		RouteInputSignal(currentVF);

		//	Set the device format to the input format detected, and set VANC mode...
		mDevice.SetVideoFormat (mInputFrameStores, currentVF, /*retailMode*/false);
		if (NTV2_IS_4K_VIDEO_FORMAT(currentVF))
		{
			if (mDevice.features().CanDo12gRouting())
				mDevice.SetTsiFrameEnable(true, mConfig.fInputChannel);
			else if (mSquares)
				mDevice.Set4kSquaresEnable(true, mConfig.fInputChannel);
			else
				mDevice.SetTsiFrameEnable(true, mConfig.fInputChannel);
			mVancMode = NTV2_VANCMODE_OFF;
			if (mConfig.fUseVanc)
				CAPWARN("VANC mode incompatible with 4K/UHD format");
		}
		else
			mVancMode = mConfig.fUseVanc ? NTV2_VANCMODE_TALL : NTV2_VANCMODE_OFF;	//	"Tall" mode is sufficient to grab captions
		mDevice.SetVANCMode(mInputFrameStores, mVancMode);
		if (::Is8BitFrameBufferFormat(mConfig.fPixelFormat))
			mDevice.SetVANCShiftMode (mConfig.fInputChannel,	//	8-bit FBFs require VANC bit shift
				NTV2_IS_VANCMODE_ON(mVancMode) ? NTV2_VANCDATA_8BITSHIFT_ENABLE : NTV2_VANCDATA_8BITSHIFT_DISABLE);

		//	Set up the circular buffers based on the detected currentVF...
		AJAStatus status (SetupHostBuffers(currentVF));
		if (AJA_FAILURE(status))
			return;

		if (mConfig.fBurnCaptions)			//	If burning-in captions...
			StartPlayThread();				//	...start a new playout thread
		else								//	else E-E mode...
			SetOutputStandards(currentVF);	//	...output standard may need changing

		mDevice.AutoCirculateStop(mConfig.fInputChannel);
		if (!mDevice.AutoCirculateInitForInput(	mConfig.fInputChannel,		//	primary channel
												mConfig.fFrames.count(),	//	numFrames (zero if specifying range)
												mAudioSystem,				//	audio system
												AUTOCIRCULATE_WITH_RP188
													| (mDevice.features().CanDoCustomAnc() ? AUTOCIRCULATE_WITH_ANC : 0),	//	flags
												1,	//	numChannels to gang
												mConfig.fFrames.firstFrame(), mConfig.fFrames.lastFrame()))
			{CAPFAIL("Failed to init Ch" << DEC(mConfig.fInputChannel+1) << " for input"); break;}

		//	Start AutoCirculate...
		if (!mDevice.AutoCirculateStart(mConfig.fInputChannel))
			{CAPFAIL("Failed to start Ch" << DEC(mConfig.fInputChannel+1)); break;}

		//	Process frames until signal format changes...
		while (!mGlobalQuit)
		{
			AUTOCIRCULATE_STATUS acStatus;
			mDevice.AutoCirculateGetStatus (mConfig.fInputChannel, acStatus);
			if (acStatus.IsRunning()  &&  acStatus.HasAvailableInputFrame())
			{
				//	At this point, there's at least one fully-formed frame available in the device's
				//	frame buffer to transfer to the host. Reserve an NTV2FrameData to "produce", and
				//	use it to store the next frame to be transferred from the device...
				NTV2FrameData *	pCaptureData (mCircularBuffer.StartProduceNextBuffer());
				if (pCaptureData)
				{
					mInputXferInfo.acFrameBufferFormat = mConfig.fPixelFormat;
					mInputXferInfo.SetBuffers (pCaptureData->VideoBuffer(), pCaptureData->VideoBufferSize(),
												pCaptureData->AudioBuffer(), pCaptureData->AudioBufferSize(),
												pCaptureData->AncBuffer(), pCaptureData->AncBufferSize(),
												pCaptureData->AncBuffer2(), pCaptureData->AncBuffer2Size());

					//	Transfer the frame data from the device into our host NTV2FrameData...
					if (mDevice.AutoCirculateTransfer (mConfig.fInputChannel, mInputXferInfo))
					{
						xferTally++;
						if (mConfig.fBurnCaptions)
							mInputXferInfo.GetInputTimeCodes(pCaptureData->fTimecodes, ::NTV2InputSourceToChannel(mConfig.fInputSource));	//	Captured timecodes

						//	Extract closed-captioning data from the host NTV2FrameData while we have full access to it...
						ExtractClosedCaptionData (mInputXferInfo.GetTransferStatus().GetProcessedFrameCount(), currentVF);
					}
					else
						xferFails++;

					//	Signal that we're done "producing" the frame, making it available for future "consumption"...
					mCircularBuffer.EndProduceNextBuffer();

					if (!mConfig.fBurnCaptions)
					{
						//	If no caption burn-in is taking place, there's nobody to consume the buffer.
						//	In this case, simply consume it now, thus recycling it immediately...
						mCircularBuffer.StartConsumeNextBuffer();
						mCircularBuffer.EndConsumeNextBuffer();
					}
				}	//	if pCaptureData != NULL
			}	//	if A/C running and frame(s) are available for transfer
			else
			{
				//	Either AutoCirculate is not running, or there were no frames available on the device to transfer.
				//	Rather than waste CPU cycles spinning, waiting until a frame becomes available, it's far more
				//	efficient to wait for the next input vertical interrupt event to get signaled...
				mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel);
				++waitTally;

				//	Did incoming video format or VPID(s) change?
				const NTV2Channel sdiConnector (mActiveSDIInputs.empty()  ?  NTV2_CHANNEL1  :  *(mActiveSDIInputs.begin()));
				ULWord vpidDS1(0), vpidDS2(0);
				if (mDevice.GetVPIDValidA(sdiConnector))
					mDevice.ReadSDIInVPID (sdiConnector, vpidDS1, vpidDS2);

				NTV2VideoFormat newVF (mDevice.GetInputVideoFormat(mConfig.fInputSource));
				const bool vfChanged(newVF != currentVF);
				const bool vpidChgd(mVPIDInfoDS1.GetVPID() != vpidDS1  ||  mVPIDInfoDS2.GetVPID() != vpidDS2);
				if (vfChanged  ||  vpidChgd)	//  ||  squares != mSquares)
				{
					++noVideoTally;
					if (vfChanged)
						CAPWARN("Input video format changed from '" << ::NTV2VideoFormatToString(currentVF)
								<< "' to '" << ::NTV2VideoFormatToString(newVF) << "'");
					else if (vpidChgd)
						CAPWARN("Input VPID changed: DS1 (" << xHEX0N(mVPIDInfoDS1.GetVPID(),8) << " to " << xHEX0N(vpidDS1,8)
								<< "), DS2 (" << xHEX0N(mVPIDInfoDS2.GetVPID(),8) << " to " << xHEX0N(vpidDS2,8) << ")");

					//	Terminate the playout thread...
					mCircularBuffer.StartProduceNextBuffer();
					mCircularBuffer.EndProduceNextBuffer();
					break;	//	exit frame processing loop to restart AutoCirculate
				}	//	if incoming video format changed
			}	//	else not running or no frames available

			//	Check if pixel format change requested (user pressed P key)...
			if (mConfig.fPixelFormat != currentPF)
			{
				CAPWARN("FrameStore pixel format changed from '"
					<< ::NTV2FrameBufferFormatToString(currentPF,true) << "' to '"
					<< ::NTV2FrameBufferFormatToString(mConfig.fPixelFormat,true) << "'");
				break;	//	exit frame processing loop -- restart AutoCirculate
			}
			if (mConfig.fUseVanc != bUsingVanc)
			{
				CAPWARN("Vanc " << (mConfig.fUseVanc?"enabled":"disabled"));
				bUsingVanc = mConfig.fUseVanc;
				break;	//	exit frame processing loop -- restart AutoCirculate
			}
		}	//	normal frame processing loop -- loop until signal or pixel format change

		//	Stop AutoCirculate...
		mDevice.AutoCirculateStop(mConfig.fInputChannel);

	}	//	loop til quit signaled
	CAPNOTE("Thread completed, " << DEC(xferTally) << " of " << DEC(xferTally+xferFails) << " frms xferred, "
			<< DEC(waitTally) << " waits, " << DEC(noVideoTally) << " sig chgs");

}	//	CaptureFrames

NTV2VideoFormat NTV2CCGrabber::WaitForStableInputSignal (void)
{
	NTV2VideoFormat	result(NTV2_FORMAT_UNKNOWN), lastVF(NTV2_FORMAT_UNKNOWN);
	ULWord	numConsecutiveFrames(0), MIN_NUM_CONSECUTIVE_FRAMES(6);
	NTV2_ASSERT(!mActiveSDIInputs.empty());
	NTV2Channel sdiConnector(*(mActiveSDIInputs.begin()));

	//	Detection loop:
	while (result == NTV2_FORMAT_UNKNOWN)
	{
		//	Determine the input video signal format...
		//	Warning:  if there's no input signal, this loop won't exit until mGlobalQuit goes true!
		mVPIDInfoDS1.MakeInvalid();  mVPIDInfoDS2.MakeInvalid();	//	Reset VPID info
		UWord loopCount(0);
		while (result == NTV2_FORMAT_UNKNOWN)
		{
			mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel);
			if (mGlobalQuit)
				return NTV2_FORMAT_UNKNOWN;	//	Terminate if asked to do so

			const NTV2VideoFormat currVF (mDevice.GetInputVideoFormat(mConfig.fInputSource));
			if (currVF == NTV2_FORMAT_UNKNOWN)
			{	//	Wait for video signal to appear
				if (++loopCount % 500 == 0)	//	Log message every minute or so at ~50ms
					CAPDBG("Waiting for valid video signal to appear at "
							<< ::NTV2InputSourceToString(mConfig.fInputSource,true));
			}
			else if (numConsecutiveFrames == 0)
			{
				lastVF = currVF;		//	First valid video format to appear
				numConsecutiveFrames++;	//	Start debounce counter
			}
			else if (numConsecutiveFrames == MIN_NUM_CONSECUTIVE_FRAMES)
			{
				numConsecutiveFrames = 0;	//	Reset for next signal outage
				result = currVF;			//	Set official video format to use
			}
			else
				numConsecutiveFrames = (lastVF == currVF) ? numConsecutiveFrames + 1 : 0;
		}	//	loop while input video format is unstable

		//	At this point, the video format is stable and valid.

		//	Grab input VPID info...
		if (mDevice.GetVPIDValidA(sdiConnector))
		{	ULWord vpidDS1(0), vpidDS2(0);
			mDevice.ReadSDIInVPID (sdiConnector, vpidDS1, vpidDS2);
			mVPIDInfoDS1.SetVPID(vpidDS1);
			mVPIDInfoDS2.SetVPID(vpidDS2);
		}
		if (mVPIDInfoDS1.IsValid())
		{	//	DS1 VPID valid --- 
			CAPNOTE(::NTV2InputSourceToString(mConfig.fInputSource,true) << " DS1: " << mVPIDInfoDS1);
			NTV2VideoFormat vfVPID (mVPIDInfoDS1.GetVideoFormat());
			if (mVPIDInfoDS2.IsValid())
				CAPNOTE(::NTV2InputSourceToString(mConfig.fInputSource,true) << " DS2: " << mVPIDInfoDS2);
			if (vfVPID != result  &&  !mSquares)
			{
				CAPWARN("VPID=" << ::NTV2VideoFormatToString(vfVPID) << " != " << ::NTV2VideoFormatToString(result));
				result = vfVPID;
			}
		}

		ostringstream osserr;
		if (!mDevice.features().CanDoVideoFormat(result))	//	Can this device handle this video format?
			osserr << mDevice.GetModelName() << " can't handle " << ::NTV2VideoFormatToString(result);
		else if (mVPIDInfoDS1.IsValid()  &&  mVPIDInfoDS1.IsStandardTwoSampleInterleave()  &&  !mDevice.features().CanDo425Mux())
			osserr << mDevice.GetModelName() << " can't handle TSI";
		if (!osserr.str().empty())
		{
			CAPWARN(osserr.str());
			result = NTV2_FORMAT_UNKNOWN;
			mDevice.WaitForInputVerticalInterrupt(mConfig.fInputChannel, 30);	//	Wait 30 frames
			continue;	//	Retry
		}

		CAPNOTE(::NTV2InputSourceToString(mConfig.fInputSource,true) << " video format: " << ::NTV2VideoFormatToString(result));
		cerr << endl << "## NOTE:  " << ::NTV2InputSourceToString(mConfig.fInputSource,true)
									<< " video format is " << ::NTV2VideoFormatToString(result) << endl;
		break;	//	Done!
	}	//	loop 

	//	Using 'result' & possibly mVPIDInfoDS1 & mVPIDInfoDS2 -- 
	//	Does this format require another SDI wire? So we should check for another SDI input?
	//	For now, use just 1 framestore, 1 SDI input, 1 CSC...
	mDevice.DisableChannels(mInputFrameStores);
	mInputFrameStores.clear();  mInputFrameStores.insert(mConfig.fInputChannel);
	mActiveSDIInputs.clear();  mActiveSDIInputs.insert(sdiConnector);
	if (NTV2_IS_4K_VIDEO_FORMAT(result))
	{
		mInputFrameStores = ::NTV2MakeChannelSet(mConfig.fInputChannel,
									mVPIDInfoDS1.IsValid() && mVPIDInfoDS1.IsStandardTwoSampleInterleave() ? 2 : 4);
		mActiveSDIInputs = ::NTV2MakeChannelSet(sdiConnector, UWord(mInputFrameStores.size()));
	}
	mActiveCSCs = mActiveSDIInputs;
	CAPDBG(::NTV2VideoFormatToString(result) << ": SDIs=" << ::NTV2ChannelSetToStr(mActiveSDIInputs)
			<< "  FrameStores=" << ::NTV2ChannelSetToStr(mInputFrameStores)
			<< "  CSCs=" << ::NTV2ChannelSetToStr(mActiveCSCs));
	cerr << "WaitForStableInputSignal: " << ::NTV2VideoFormatToString(result) << ": SDIIns=" << ::NTV2ChannelSetToStr(mActiveSDIInputs)
			<< "  FrameStores=" << ::NTV2ChannelSetToStr(mInputFrameStores)
			<< "  CSCs=" << ::NTV2ChannelSetToStr(mActiveCSCs) << endl;
	NTV2_ASSERT(result != NTV2_FORMAT_UNKNOWN);
	return result;
}	//	WaitForStableInputSignal


//////////////////////////////////////////////


void NTV2CCGrabber::ToggleVANC (void)
{
	mConfig.fUseVanc = !mConfig.fUseVanc;
	cerr	<< endl << "## NOTE:  VANC frame geometry " << (mConfig.fUseVanc ? "enabled" : "disabled") << endl;
}


void NTV2CCGrabber::SwitchOutput (void)
{
	OutputMode	mode (mConfig.fOutputMode);
	mode = OutputMode(mode+1);
	if (!IS_VALID_OutputMode(mode))
		mode = kOutputMode_CaptionStream;
	if (mConfig.fOutputMode != mode)
		cerr << endl << "## NOTE: Output changed to '" << CCGrabberConfig::OutputModeToString(mode) << "'" << endl;
	mConfig.fOutputMode = mode;
}

void NTV2CCGrabber::Switch608Source (void)
{
	CaptionDataSrc	src (mConfig.fCaptionSrc);
	src = CaptionDataSrc(src+1);
	if (!IS_VALID_CaptionDataSrc(src))
		src = kCaptionDataSrc_Default;
	if (mConfig.fCaptionSrc != src)
		cerr << endl << "## NOTE: CC source changed to '" << CCGrabberConfig::CaptionDataSrcToString(src) << "'" << endl;
	mConfig.fCaptionSrc = src;
}

void NTV2CCGrabber::SwitchPixelFormat (void)
{
	NTV2PixelFormat pf (mConfig.fPixelFormat);
	do
	{
		pf = NTV2PixelFormat(pf+1);
		if (pf == NTV2_FBF_LAST)
			pf = NTV2_FBF_FIRST;
	} while (NTV2_IS_FBF_PLANAR(pf)  ||  !mDevice.features().CanDoFrameBufferFormat(pf));
	if (mConfig.fPixelFormat != pf)
		cerr << endl << "## NOTE: Pixel format changed to " << ::NTV2FrameBufferFormatToString(pf) << endl;
	mConfig.fPixelFormat = pf;
}

void NTV2CCGrabber::ExtractClosedCaptionData (const uint32_t inFrameNum, const NTV2VideoFormat inVideoFormat)
{
	AJAAncillaryList	ancPackets, vancPackets;
	CaptionData			captionData708Anc, captionData708Vanc, captionData608Anc, captionData608Vanc, captionDataL21Anc, captionDataL21;	//	The 608 caption byte pairs (one pair per field)
	const NTV2FormatDescriptor formatDesc (inVideoFormat, mConfig.fPixelFormat, mVancMode);

	if (NTV2_IS_VANCMODE_ON(mVancMode) || mDevice.features().CanDoCustomAnc())	//	Gotta have at least VANC or AncExt
	{
		//	Get all VANC packets...
		if (NTV2_IS_VANCMODE_ON(mVancMode))
		{
			AJAAncillaryList::SetFromVANCData (mInputXferInfo.acVideoBuffer, formatDesc, vancPackets, inFrameNum);
			vancPackets.ParseAllAncillaryData();
		}

		//	Get all anc extractor packets...
		if (mDevice.features().CanDoCustomAnc())
		{
			const NTV2Buffer validAncF1 (mInputXferInfo.acANCBuffer.GetHostAddress(0), mInputXferInfo.GetCapturedAncByteCount(false));
			const NTV2Buffer validAncF2 (mInputXferInfo.acANCField2Buffer.GetHostAddress(0), mInputXferInfo.GetCapturedAncByteCount(true));
			AJAAncillaryList::SetFromDeviceAncBuffers (validAncF1, validAncF2, ancPackets, inFrameNum);
			ancPackets.ParseAllAncillaryData();
		}

		if (NTV2_IS_VANCMODE_ON(mVancMode))
		{	//	Compare with what we got from VANC lines:
			NTV2StringList diffs;
			if (!ancPackets.CompareWithInfo (diffs, vancPackets, true/*ignoreLoc*/, false /*ignoreChksum*/))
			{
				NTV2StringList lines(aja::split(diffs.at(0), "\n"));	//	Only log first diff
				CAPDBG(DEC(diffs.size()) << " VANC/AncExt diff(s): " << lines.at(0));
				for (size_t n(1);  n < lines.size();  n++)
					CAPDBG(lines.at(n));
			}
		}
	}

	//	Get Line21 CC data...
	if (NTV2_IS_SD_VIDEO_FORMAT(inVideoFormat)  &&  (mConfig.fPixelFormat == NTV2_FBF_8BIT_YCBCR || mConfig.fPixelFormat == NTV2_FBF_10BIT_YCBCR))
	{	//	Anything encoded in Line 21?
		ULWord	line21RowOffset(0);
		const UByte *	pLine21(AJA_NULL);
		formatDesc.GetLineOffsetFromSMPTELine (21, line21RowOffset);
		pLine21 = AsConstUBytePtr(formatDesc.GetRowAddress(mInputXferInfo.acVideoBuffer.GetHostPointer(),
																			line21RowOffset));
		if (pLine21)
		{
			if (mConfig.fPixelFormat == NTV2_FBF_10BIT_YCBCR)	//	CNTV2Line21Captioner::DecodeLine requires 8-bit YUV
				::ConvertLine_v210_to_2vuy(AsConstULWordPtr(pLine21), (UByte*)(pLine21), 720);	//	Convert YUV10 to YUV8 in-place
			captionDataL21.bGotField1Data = CNTV2Line21Captioner::DecodeLine(pLine21, captionDataL21.f1_char1, captionDataL21.f1_char2);
		}
		pLine21 = AsConstUBytePtr(formatDesc.GetRowAddress(mInputXferInfo.acVideoBuffer.GetHostPointer(),
															line21RowOffset+1));	//	F2 should be on next row
		if (pLine21)
		{
			if (mConfig.fPixelFormat == NTV2_FBF_10BIT_YCBCR)	//	CNTV2Line21Captioner::DecodeLine requires 8-bit YUV
				::ConvertLine_v210_to_2vuy(AsConstULWordPtr(pLine21), (UByte*)(pLine21), 720);	//	Convert YUV10 to YUV8 in-place
			captionDataL21.bGotField2Data = CNTV2Line21Captioner::DecodeLine(pLine21, captionDataL21.f2_char1, captionDataL21.f2_char2);
		}
	}

	//	Any 608 packets (anc extractor)?
	if (ancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Vanc, 0))	//	F1
	{
		AJAAncillaryData_Cea608_Vanc	pkt608F1	(ancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Vanc, 0));
		if (pkt608F1.GetPayloadData() && pkt608F1.GetPayloadByteCount()  &&  AJA_SUCCESS(pkt608F1.ParsePayloadData()))
			pkt608F1.GetCEA608Bytes (captionData608Anc.f1_char1, captionData608Anc.f1_char2, captionData608Anc.bGotField1Data);
	}
	if (ancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Vanc, 1))	//	F2
	{
		AJAAncillaryData_Cea608_Vanc	pkt608F2	(ancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Vanc, 1));
		if (pkt608F2.GetPayloadData() && pkt608F2.GetPayloadByteCount()  &&  AJA_SUCCESS(pkt608F2.ParsePayloadData()))
			pkt608F2.GetCEA608Bytes(captionData608Anc.f2_char1, captionData608Anc.f2_char2, captionData608Anc.bGotField2Data);
	}

	//	Any 608 packets (Vanc)?
	if (vancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Vanc, 0))
	{
		AJAAncillaryData_Cea608_Vanc	pkt608F1	(vancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Vanc, 0));
		if (pkt608F1.GetPayloadData() && pkt608F1.GetPayloadByteCount()  &&  AJA_SUCCESS(pkt608F1.ParsePayloadData()))
			pkt608F1.GetCEA608Bytes (captionData608Vanc.f1_char1, captionData608Vanc.f1_char2, captionData608Vanc.bGotField1Data);
	}
	if (vancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Vanc, 1))
	{
		AJAAncillaryData_Cea608_Vanc	pkt608F2	(vancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Vanc, 1));
		if (pkt608F2.GetPayloadData() && pkt608F2.GetPayloadByteCount()  &&  AJA_SUCCESS(pkt608F2.ParsePayloadData()))
			pkt608F2.GetCEA608Bytes(captionData608Vanc.f2_char1, captionData608Vanc.f2_char2, captionData608Vanc.bGotField2Data);
	}

	//	Any 708 packets (vanc)?
	if (vancPackets.CountAncillaryDataWithType(AJAAncDataType_Cea708))
	{
		AJAAncillaryData	vancCEA708DataIn	(vancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea708));
		bool				hasParityErrors (false);
		if (vancCEA708DataIn.GetPayloadData() && vancCEA708DataIn.GetPayloadByteCount()  &&  AJA_SUCCESS(vancCEA708DataIn.ParsePayloadData()))
			if (m708DecoderVanc->SetSMPTE334AncData (vancCEA708DataIn.GetPayloadData(), vancCEA708DataIn.GetPayloadByteCount()))
				if (m708DecoderVanc->ParseSMPTE334AncPacket(hasParityErrors))
				{
					vector<UByte>	svcBlk;
					size_t			blkSize(0), byteCount(0);
					int				svcNum(0);
					bool			isExtendedSvc(false);
					if (!hasParityErrors)
						captionData708Vanc = m708DecoderVanc->GetCC608CaptionData();
					if (m708DecoderVanc->GetNextServiceBlockInfoFromQueue (NTV2_CC708PrimaryCaptionServiceNum, blkSize, byteCount, svcNum, isExtendedSvc))
						m708DecoderVanc->GetNextServiceBlockFromQueue(NTV2_CC708PrimaryCaptionServiceNum, svcBlk);	//	Pop queued service block
				}
	}
	//	Any 708 packets (anc extractor)?
	if (ancPackets.CountAncillaryDataWithType(AJAAncDataType_Cea708))
	{
		AJAAncillaryData	ancCEA708DataIn	(ancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea708));
		bool				hasParityErrors (false);
		if (ancCEA708DataIn.GetPayloadData() && ancCEA708DataIn.GetPayloadByteCount()  &&  AJA_SUCCESS(ancCEA708DataIn.ParsePayloadData()))
			if (m708DecoderAnc->SetSMPTE334AncData (ancCEA708DataIn.GetPayloadData(), ancCEA708DataIn.GetPayloadByteCount()))
				if (m708DecoderAnc->ParseSMPTE334AncPacket(hasParityErrors))
				{
					vector<UByte>	svcBlk;
					size_t			blkSize(0), byteCount(0);
					int				svcNum(0);
					bool			isExtendedSvc(false);
					if (!hasParityErrors)
						captionData708Anc = m708DecoderAnc->GetCC608CaptionData();
					if (m708DecoderAnc->GetNextServiceBlockInfoFromQueue (NTV2_CC708PrimaryCaptionServiceNum, blkSize, byteCount, svcNum, isExtendedSvc))
						m708DecoderAnc->GetNextServiceBlockFromQueue(NTV2_CC708PrimaryCaptionServiceNum, svcBlk);	//	Pop queued service block
				}
	}
	//	Any "analog" packets?
	if (ancPackets.CountAncillaryDataWithType(AJAAncDataType_Cea608_Line21))
	{
		AJAAncillaryData_Cea608_Line21	ancEIA608DataIn	(ancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Line21, 0));	//	F1
		if (AJA_SUCCESS(ancEIA608DataIn.ParsePayloadData()))
			ancEIA608DataIn.GetCEA608Bytes(captionDataL21Anc.f1_char1, captionDataL21Anc.f1_char2, captionDataL21Anc.bGotField1Data);
		if (ancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Line21, 1))
		{
			AJAAncillaryData_Cea608_Line21	ancEIA608F2	(ancPackets.GetAncillaryDataWithType(AJAAncDataType_Cea608_Line21, 1));	//	F2
			if (AJA_SUCCESS(ancEIA608F2.ParsePayloadData()))
			ancEIA608F2.GetCEA608Bytes(captionDataL21Anc.f2_char1, captionDataL21Anc.f2_char2, captionDataL21Anc.bGotField2Data);
		}
	}

	//	Compare CaptionData results...
	ostringstream		ossCompare;
	const CaptionData *	p608CaptionData(AJA_NULL);
	if (NTV2_IS_SD_VIDEO_FORMAT(inVideoFormat))
	{
		const CaptionData *	pL21CaptionData (AJA_NULL);
		if (captionDataL21.HasData() && captionDataL21Anc.HasData())
			if (captionDataL21 != captionDataL21Anc)
				ossCompare << "L21Vanc != L21Anlg: " << captionDataL21 << " " << captionDataL21Anc << endl;
		if (captionDataL21.HasData())
			pL21CaptionData = &captionDataL21;
		else if (captionDataL21Anc.HasData())
			pL21CaptionData = &captionDataL21Anc;
		if (captionData608Anc.HasData() && pL21CaptionData)
			if (captionData608Anc != *pL21CaptionData)
				ossCompare << "608 != L21: " << captionData608Anc << " " << *pL21CaptionData << endl;
		if (captionData608Anc.HasData())
			p608CaptionData = &captionData608Anc;	//	608 anc has precedence
		else if (pL21CaptionData)
			p608CaptionData = pL21CaptionData;		//	followed by line 21
	}
	else
	{
		if (captionData608Anc.HasData() && captionData708Anc.HasData())
			if (captionData608Anc != captionData708Anc)
				ossCompare << "608anc != 708anc: " << captionData608Anc << " " << captionData708Anc << endl;
		if (captionData608Vanc.HasData() && captionData708Vanc.HasData())
			if (captionData608Vanc != captionData708Vanc)
				ossCompare << "608vanc != 708vanc: " << captionData608Vanc << " " << captionData708Vanc << endl;
		if (captionData708Anc.HasData())
			p608CaptionData = &captionData708Anc;	//	608-in-708anc has highest precedence
		else if (captionData708Vanc.HasData())
			p608CaptionData = &captionData708Vanc;	//	followed by 608-in-708vanc
		else if (captionData608Anc.HasData())
			p608CaptionData = &captionData608Anc;	//	followed by 608anc
		else if (captionData608Vanc.HasData())
			p608CaptionData = &captionData608Vanc;	//	followed by 608vanc
	}
	if (!ossCompare.str().empty())
		CAPDBG("CaptionData mis-compare(s): " << ossCompare.str());

	//	Set p608CaptionData based on mConfig.fCaptionSrc...
	switch (mConfig.fCaptionSrc)
	{	//	captionData708Anc, captionData708Vanc, captionData608Anc, captionData608Vanc, captionDataL21Anc, captionDataL21
		case kCaptionDataSrc_Line21:		p608CaptionData = &captionDataL21Anc;	break;
		case kCaptionDataSrc_608FBVanc:		p608CaptionData = &captionDataL21;		break;
		case kCaptionDataSrc_708FBVanc:		p608CaptionData = &captionData708Vanc;	break;
		case kCaptionDataSrc_608Anc:		p608CaptionData = &captionData608Anc;	break;
		case kCaptionDataSrc_708Anc:		p608CaptionData = &captionData708Anc;	break;
		default:							break;
	}

	if (!p608CaptionData)
		return;	//	Got nothing

	//	This demo only handles CEA-608 captions (no Teletext, IBU, etc.)
	//	The 608 decoder expects to be called once per frame (to implement flashing characters, smooth-scroll roll-up, etc.).
	//	Pass the caption byte pairs to it for processing (even if captionData.HasData returns false)...
	m608Decoder->ProcessNew608FrameData(*p608CaptionData);

	DoCCOutput(inFrameNum, *p608CaptionData, inVideoFormat);

}	//	ExtractClosedCaptionData


void NTV2CCGrabber::DoCCOutput (const uint32_t inFrameNum, const CaptionData & inCCData, const NTV2VideoFormat inVideoFormat)
{
	//	Print out the caption characters...
	const bool	isFirstTime (mLastOutFrame == 0  &&  inFrameNum > mLastOutFrame);
	char		char1(0), char2(0);
	const bool	showField2	(IsField2Line21CaptionChannel(m608Channel));
	const bool	gotData		(showField2 ? inCCData.bGotField2Data : inCCData.bGotField1Data);
	switch (mConfig.fOutputMode)
	{
		case kOutputMode_CaptionStream:
			if (gotData)
			{	//	Can't distinguish between CC1/CC2/Tx1/Tx2 for F1 (nor CC3/CC4/Tx3/Tx4 for F2).
				//	They'll just have to be interspersed for now...
				char1 = (char(showField2 ? inCCData.f2_char1 : inCCData.f1_char1) & 0x7F);
				char2 = (char(showField2 ? inCCData.f2_char2 : inCCData.f1_char2) & 0x7F);
				if (char1 >= ' ' && char1 <= '~')
				{
					if (mLastOutStr != " " || char1 != ' ')
						{cout << char1 << flush;  mLastOutStr = string(1,char1);}
					if (char2 >= ' ' && char2 <= '~')
					{
						if (mLastOutStr != " " || char2 != ' ')
							{cout << char2 << flush;  mLastOutStr = string(1,char2);}
					}
				}
				else if (mLastOutStr != " ")
					{cout << " " << flush;	mLastOutStr = " ";}
			}
			break;
		case kOutputMode_CaptionScreen:
		{
			if (inFrameNum == (mLastOutFrame+1))
			{
				mLastOutFrame = inFrameNum;
				if ((mLastOutFrame - mFirstOutFrame) < MAX_ACCUM_FRAME_COUNT)
					break;	//	Don't spew yet -- keep going
			}
			const string currentScreen(m608Decoder->GetOnAirCharacters());
			if (currentScreen != mLastOutStr)
				cout << currentScreen << endl;
			mLastOutStr = currentScreen;
			mFirstOutFrame = mLastOutFrame = inFrameNum;
			break;
		}
		case kOutputMode_CaptionFileSCC:
		{
			uint16_t	u16(uint16_t(uint16_t(showField2 ? inCCData.f2_char1 : inCCData.f1_char1) << 8) | uint16_t(showField2 ? inCCData.f2_char2 : inCCData.f1_char2));
			ostringstream oss;
			oss << Hex0N(u16,4);
			mLastOutStr.append(mLastOutStr.empty() ? "\t" : " ");
			mLastOutStr.append(oss.str());
			if (inFrameNum == (mLastOutFrame+1))
			{	//	Save for later
				mLastOutFrame = inFrameNum;
				if ((mLastOutFrame - mFirstOutFrame) < MAX_ACCUM_FRAME_COUNT)
					break;	//	Don't spew yet -- keep going
			}
			if (isFirstTime)
				cout << "Scenarist_SCC V1.0" << endl;

			string		tcStr;
			AJATimeCode	tcFirst(mFirstOutFrame);
			AJATimeBase	tcBase(CNTV2DemoCommon::GetAJAFrameRate(::GetNTV2FrameRateFromVideoFormat(inVideoFormat)));
			tcFirst.QueryString(tcStr, tcBase, false);
			cout << endl << tcStr << ":" << mLastOutStr << endl;
			mFirstOutFrame = mLastOutFrame = inFrameNum;
			mLastOutStr.clear();
			break;
		}
		case kOutputMode_Stats:
			if (inFrameNum % 60 == 0)	//	Every 60 frames
			{
				CNTV2CaptionDecodeChannel608Ptr chDecoder (m608Decoder->Get608ChannelDecoder(m608Decoder->GetDisplayChannel()));
				if (chDecoder)
				{
					const vector<uint32_t> stats (chDecoder->GetStats());
					AJALabelValuePairs	info;
					AJASystemInfo::append(info, ::NTV2Line21ChannelToStr(m608Decoder->GetDisplayChannel()));
					for (size_t num(0);  num < stats.size();  num++)
					{
						const string title(CNTV2CaptionDecodeChannel608::GetStatTitle(CaptionDecode608Stats(num)));
						if (title.empty())
							break;
						if (!stats[num])
							continue;
						ostringstream oss;  oss << DEC(stats[num]);
						AJASystemInfo::append(info, title, oss.str());
					}
					cout << AJASystemInfo::ToString(info) << endl;
				}
			}
			break;
		default:	break;
	}
}	//	DoCCOutput


//////////////////////////////////////////////	P L A Y O U T


static const UWord	gMixerNums []	=	{0, 0, 1, 1, 2, 2, 3, 3};


AJAStatus NTV2CCGrabber::SetupOutputVideo (const NTV2VideoFormat inVideoFormat)
{
	NTV2_ASSERT (mConfig.fBurnCaptions);	//	Must be burning captions

	//	Configure the output FrameStore...
	mDevice.EnableChannel (mOutputChannel);
	mDevice.SetMode (mOutputChannel, NTV2_MODE_DISPLAY);
	mDevice.SetFrameBufferFormat (mOutputChannel, mPlayoutFBF);
	mDevice.SetVideoFormat (inVideoFormat,  /*retailMode?*/false,  /*keepVANC?*/false,  /*channel*/mOutputChannel);
	mDevice.SetEnableVANCData(NTV2_IS_VANCMODE_TALL(mVancMode), NTV2_IS_VANCMODE_TALLER(mVancMode), mOutputChannel);

	//	RGB:  Set up mixer to "mix" mode, FG raster "unshaped", BG raster "full raster" and VANC pass-thru from BG...
	const UWord	mixerNumber	(gMixerNums[mOutputChannel]);
	mDevice.SetMixerMode (mixerNumber, NTV2MIXERMODE_FOREGROUND_ON);
	mDevice.SetMixerFGInputControl (mixerNumber, NTV2MIXERINPUTCONTROL_UNSHAPED);
	mDevice.SetMixerBGInputControl (mixerNumber, NTV2MIXERINPUTCONTROL_FULLRASTER);
	mDevice.SetMixerVancOutputFromForeground (mixerNumber, false);	//	false means "use BG VANC, not FG"
	cerr	<< "## NOTE:  Caption burn-in using mixer/keyer " << (mixerNumber+1) << " on " << ::NTV2ChannelToString(mOutputChannel)
			<< ", " << ::NTV2FrameBufferFormatToString(mPlayoutFBF)
			<< ", " << ::NTV2VideoFormatToString(inVideoFormat) << endl;

	return AJA_STATUS_SUCCESS;

}	//	SetupOutputVideo


bool NTV2CCGrabber::RouteOutputSignal (const NTV2VideoFormat inVideoFormat)
{
	NTV2_ASSERT (mConfig.fBurnCaptions);	//	Must be burning captions
	const NTV2OutputCrosspointID	frameStoreOutputRGB	(::GetFrameBufferOutputXptFromChannel (mOutputChannel, true));	//	true=RGB
	const NTV2OutputCrosspointID	cscOutputYUV		(::GetCSCOutputXptFromChannel (mOutputChannel));
	const NTV2OutputCrosspointID	cscOutputKey		(::GetCSCOutputXptFromChannel (mOutputChannel, true));	//	true=key
	const NTV2OutputCrosspointID	mixerOutputYUV		(::GetMixerOutputXptFromChannel (mOutputChannel));
	const NTV2OutputCrosspointID	signalInput			(::GetSDIInputOutputXptFromChannel (mConfig.fInputChannel));
	const NTV2Standard				outputStandard		(::GetNTV2StandardFromVideoFormat (inVideoFormat));
	const bool						canVerify			(mDevice.features().HasCrosspointConnectROM());
	UWord							connectFailures		(0);

	if (mConfig.fDoMultiFormat)
	{
		//	Multiformat --- route the one SDI output to the mixer's YUV output, and set its output standard...
		if (mDevice.features().HasBiDirectionalSDI())
			mDevice.SetSDITransmitEnable (mOutputChannel, true);
		if (mDevice.features().CanDoWidget(g12GSDIOutputs[mOutputChannel])
			|| mDevice.features().CanDoWidget(g3GSDIOutputs[mOutputChannel])
			|| mDevice.features().CanDoWidget(gSDIOutputs[mOutputChannel]))
		{
			if (!mDevice.Connect (::GetSDIOutputInputXpt(mOutputChannel), mixerOutputYUV, canVerify)) connectFailures++;
			mDevice.SetSDIOutputStandard (mOutputChannel, outputStandard);
		}
	}
	else
	{
		//	If not multiformat:  Route all SDI outputs to the mixer's YUV output...
		const ULWord		numVideoOutputs	(mDevice.features().GetNumVideoOutputs());
		const NTV2Channel	startNum		(NTV2_CHANNEL1);
		const NTV2Channel	endNum			(NTV2_CHANNEL_INVALID);
		const NTV2Channel	sdiInputAsChan	(::NTV2InputSourceToChannel(mConfig.fInputSource));

		for (NTV2Channel chan(startNum);  chan < endNum;  chan = NTV2Channel(chan + 1))
		{
			if (ULWord(chan) >= numVideoOutputs)
				break;
			if (mDevice.features().HasBiDirectionalSDI())
			{
				if (chan == sdiInputAsChan)
					continue;	//	Skip the input
				mDevice.SetSDITransmitEnable (chan, true);
			}
			if (mDevice.features().CanDoWidget(g12GSDIOutputs[chan])
				|| mDevice.features().CanDoWidget(g3GSDIOutputs[chan])
				|| mDevice.features().CanDoWidget(gSDIOutputs[chan]))
			{
				if (!mDevice.Connect (::GetSDIOutputInputXpt(chan), mixerOutputYUV, canVerify)) connectFailures++;
				mDevice.SetSDIOutputStandard (chan, outputStandard);
			}
		}	//	for each output spigot
	}

	//	Connect CSC video input to frame buffer's RGB output:
	if (!mDevice.Connect (::GetCSCInputXptFromChannel(mOutputChannel),	frameStoreOutputRGB,	canVerify)) connectFailures++;
	//	Connect mixer's foreground video input to the CSC's YUV video output:
	if (!mDevice.Connect (::GetMixerFGInputXpt(mOutputChannel),			cscOutputYUV,			canVerify)) connectFailures++;
	//	Connect mixer's foreground key input to the CSC's YUV key output:
	if (!mDevice.Connect (::GetMixerFGInputXpt(mOutputChannel, true),	cscOutputKey,			canVerify)) connectFailures++;
	//	Connect mixer's background video input to the SDI input:
	if (!mDevice.Connect (::GetMixerBGInputXpt(mOutputChannel),			signalInput,			canVerify)) connectFailures++;

	if (!mConfig.fDoMultiFormat)
	{
		//	Connect more outputs -- HDMI, analog, SDI monitor, etc...	(Don't bother to verify these connections)
		if (mDevice.features().CanDoWidget(NTV2_WgtHDMIOut1))
			mDevice.Connect (NTV2_XptHDMIOutInput, mixerOutputYUV);
		if (mDevice.features().CanDoWidget(NTV2_WgtHDMIOut1v2))
			mDevice.Connect (NTV2_XptHDMIOutQ1Input, mixerOutputYUV);
		if (mDevice.features().CanDoWidget(NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, mixerOutputYUV);
		if (mDevice.features().CanDoWidget(NTV2_WgtSDIMonOut1))
			mDevice.Connect (::GetSDIOutputInputXpt (NTV2_CHANNEL5), mixerOutputYUV);
	}
	return connectFailures == 0;

}	//	RouteOutputSignal


//	Starts the play thread
AJAStatus NTV2CCGrabber::StartPlayThread (void)
{
	//	Create and start the playout thread...
	NTV2_ASSERT(mConfig.fBurnCaptions);
	AJAStatus result (mPlayoutThread.Attach(PlayThreadStatic, this));
	if (AJA_SUCCESS(result))
		result = mPlayoutThread.SetPriority(AJA_ThreadPriority_High);
	if (AJA_SUCCESS(result))
		result = mPlayoutThread.Start();
	return result;

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
	const NTV2VideoFormat	videoFormat		(mDevice.GetInputVideoFormat(mConfig.fInputSource));
	NTV2VANCMode			vancMode		(NTV2_VANCMODE_INVALID);
	ULWord					fbNum			(10);	//	Bounce between frames 10 & 11
	const string			indicators []	= {"/", "-", "\\", "|", ""};
	AUTOCIRCULATE_STATUS	acStatus;

	CAPNOTE("Thread started");
	SetupOutputVideo (videoFormat);		//	Set up device output
	RouteOutputSignal (videoFormat);	//	Set up output signal routing
	mDevice.GetVANCMode (vancMode, mConfig.fInputChannel);
	if (mDevice.AutoCirculateInitForOutput (mOutputChannel, 2) && mDevice.AutoCirculateGetStatus (mOutputChannel, acStatus))	//	Find out which buffers we got
		fbNum = ULWord(acStatus.acStartFrame);	//	Use them
	else if (mDevice.AutoCirculateGetStatus(mConfig.fInputChannel, acStatus))	//	Use the frame just past the last input A/C frame
		fbNum = ULWord(acStatus.GetEndFrame()) + 1;

	const NTV2FormatDesc	formatDesc		(videoFormat, mPlayoutFBF, vancMode);
	const uint32_t			bufferSizeBytes	(formatDesc.GetTotalRasterBytes ());
	const uint32_t			activeSizeBytes	(formatDesc.GetVisibleRasterBytes ());
	ULWord					pingPong		(0);	//	Bounce between 0 and 1
	UWord					consecSyncErrs	(0);
	//	Caption status head-up-display...
	static uint64_t			frameTally		(0);
	const string			strVideoFormat	(CNTV2DemoCommon::StripFormatString (::NTV2VideoFormatToString (videoFormat)));
	ULWord					lastErrorTally	(0);

	//	Allocate host frame buffer for blitting captions into...
	NTV2Buffer hostBuffer;
	if (!hostBuffer.Allocate(bufferSizeBytes, /*pageAligned*/true))
		{cerr << "## NOTE:  Caption burn-in failed -- unable to allocate " << bufferSizeBytes << "-byte caption video buffer" << endl;	return;}
	NTV2Buffer visibleRgn (formatDesc.GetTopVisibleRowAddress(AsUBytePtr(hostBuffer.GetHostPointer())),activeSizeBytes);

	//	Clear both device ping/pong buffers to fully transparent, all black...
	hostBuffer.Fill(ULWord(0));
	mDevice.DMAWriteFrame (fbNum + 0, AsULWordPtr(hostBuffer.GetHostPointer()), bufferSizeBytes);
	mDevice.DMAWriteFrame (fbNum + 1, AsULWordPtr(hostBuffer.GetHostPointer()), bufferSizeBytes);
	mDevice.SetOutputFrame (mOutputChannel, fbNum + pingPong);
	pingPong = pingPong ? 0 : 1;

	//	Do forever (until Quit)...
	while (!mGlobalQuit)
	{
		//	Wait for the next frame to become ready to "consume"...
		NTV2FrameData *	pFrameData(mCircularBuffer.StartConsumeNextBuffer());
		if (pFrameData)
		{
			//	"Burn" captions into the host buffer before it gets sent to the AJA device...
			m608Decoder->BurnCaptions (hostBuffer, formatDesc);
			m608Decoder->IdleFrame();	//	This is needed for captions that flash/blink

			if (mHeadUpDisplayOn)
			{
				ostringstream	oss;
				const string	strCaptionChan	(::NTV2Line21ChannelToStr(NTV2_IS_HD_VIDEO_FORMAT(videoFormat)
													?	m708DecoderAnc->GetDisplayChannel()
													:	m608Decoder->GetDisplayChannel()));
				oss << indicators [mCaptionDataTally % 4] << " " << strCaptionChan << " " << frameTally++ << " "
					<< formatDesc.GetRasterWidth() << "x" << formatDesc.GetFullRasterHeight() << (mConfig.fUseVanc ? "v" : " ") << strVideoFormat;

				const ULWord					newErrorTally	(mErrorTally);
				const NTV2Line21Attributes &	color			(lastErrorTally != newErrorTally ? kRedOnTransparentBG : kGreenOnTransparentBG);
				CNTV2CaptionRenderer::BurnString (oss.str(), color, hostBuffer, formatDesc, 7, 1);
				lastErrorTally = newErrorTally;

				if (!pFrameData->fTimecodes.empty())
				{
					const NTV2_RP188 & tc (pFrameData->fTimecodes.begin()->second);
					if (tc.IsValid()  &&  tc.fDBB & BIT(16))	//	Bit 16 will be set if this frame had timecode
					{
						CRP188	rp188(tc);
						CNTV2CaptionRenderer::BurnString (rp188.GetRP188CString(), kGreenOnTransparentBG, hostBuffer, formatDesc, 8, 1);
					}
				}
			}

			//	Transfer the caption raster to the device, then ping-pong it into the mixer foreground...
			mDevice.DMAWriteFrame (fbNum + pingPong, AsULWordPtr(visibleRgn.GetHostPointer()), activeSizeBytes);
			mDevice.SetOutputFrame (mOutputChannel, fbNum + pingPong);	//	Toggle device frame buffer
			pingPong = pingPong ? 0 : 1;	//	Switch device frame numbers for next time
			visibleRgn.Fill(ULWord(0));		//	Clear visible region of host buffer to fully-transparent, all-black again

			//	Signal that the frame has been "consumed"...
			mCircularBuffer.EndConsumeNextBuffer ();

			//	Check for format change...
			if (videoFormat != mDevice.GetInputVideoFormat(mConfig.fInputSource))
			{
				cerr	<< "## NOTE:  Caption burn-in stopped due to video format change -- was " << strVideoFormat
						<< ", now " << ::NTV2VideoFormatToString (mDevice.GetInputVideoFormat(mConfig.fInputSource)) << endl;
				m608Decoder->Reset();	//	Don't leave captions from old video stream on-screen
				m708DecoderAnc->Reset();
				m708DecoderVanc->Reset();
				break;
			}

			//	Check mixer sync...
			bool	mixerSyncOK	(false);
			mDevice.GetMixerSyncStatus (gMixerNums[mOutputChannel], mixerSyncOK);
			if (mixerSyncOK)
				consecSyncErrs = 0;
			else if (++consecSyncErrs > 60)		//	Bail if lack of mixer sync for longer than ~1 sec
				{cerr << "#MXSYNC#";  consecSyncErrs = 0;}	//	break;}
		}	//	if pPlayData
	}	//	loop til quit signaled

	if (mDevice.AutoCirculateGetStatus (mOutputChannel, acStatus)  &&  !acStatus.IsStopped())
		mDevice.AutoCirculateStop(mOutputChannel);
	CAPNOTE("Thread completed, will exit");

}	//	PlayFrames


void NTV2CCGrabber::SetCaptionDisplayChannel (const NTV2Line21Channel inNewChannel)
{
	if (m608Channel != inNewChannel)
	{
		m608Channel = inNewChannel;
		if (m608Decoder)
			m608Decoder->SetDisplayChannel(m608Channel);
		if (m708DecoderAnc)
			m708DecoderAnc->SetDisplayChannel(m608Channel);
		if (m708DecoderVanc)
			m708DecoderVanc->SetDisplayChannel(m608Channel);
		cerr << endl << "## NOTE: Caption display channel changed to '" << ::NTV2Line21ChannelToStr(m608Channel) << "'" << endl;
	}
}


void NTV2CCGrabber::CaptioningChanged (const NTV2Caption608ChangeInfo & inChangeInfo)
{	(void) inChangeInfo;
	mCaptionDataTally++;
}


void NTV2CCGrabber::Caption608ChangedStatic (void * pInstance, const NTV2Caption608ChangeInfo & inChangeInfo)	//	STATIC
{
	NTV2CCGrabber *	pGrabber(reinterpret_cast<NTV2CCGrabber*>(pInstance));
	if (pGrabber)
		pGrabber->CaptioningChanged(inChangeInfo);
}


string NTV2CCGrabber::GetLine21ChannelNames (string inDelimiterStr)		//	static
{
	string	result;
	for (unsigned enumVal(0);  enumVal < NTV2_CC608_ChannelMax;  )
	{
		result += ::NTV2Line21ChannelToStr (NTV2Line21Channel(enumVal++));
		if (enumVal < NTV2_CC608_ChannelMax)
			result += inDelimiterStr;
		else
			break;
	}
	return result;

}	//	GetLine21ChannelNames

AJALabelValuePairs CCGrabberConfig::Get (const bool inCompact) const
{
	AJALabelValuePairs result (CaptureConfig::Get(inCompact));
	AJASystemInfo::append(result, "Output Mode",		IS_VALID_OutputMode(fOutputMode) ? OutputModeToString(fOutputMode) : "(invalid)");
	AJASystemInfo::append(result, "Caption Source",		IS_VALID_CaptionDataSrc(fCaptionSrc) ? CaptionDataSrcToString(fCaptionSrc) : "(invalid)");
	AJASystemInfo::append(result, "Timecode Source",	::NTV2TCIndexToString(fTimecodeSrc, inCompact));
	AJASystemInfo::append(result, "Caption Channel",	::NTV2Line21ChannelToStr(fCaptionChannel, inCompact));
	AJASystemInfo::append(result, "Burn-In Captions",	fBurnCaptions ? "Y" : "N");
	AJASystemInfo::append(result, "MultiFormat Mode",	fDoMultiFormat ? "Y" : "N");
	AJASystemInfo::append(result, "Use Vanc",			fUseVanc ? "Y" : "N");
	return result;
}


string	CCGrabberConfig::GetLegalOutputModes (void)
{
	string	result;
	for (OutputMode mode(kOutputMode_CaptionStream);  mode < kOutputMode_INVALID;  )
	{
		result += OutputModeToString(mode);
		mode = OutputMode(mode+1);
		if (mode < kOutputMode_INVALID)
			result += ",";
	}
	return result;
}

string	CCGrabberConfig::OutputModeToString (const OutputMode inMode)
{
	static const string gModeStrs[]	= {"stream","screen","scc","mcc","stats",""};
	if (IS_VALID_OutputMode(inMode))
		return gModeStrs[inMode];
	string result;
	for (unsigned ndx(0);  ndx < 5;  )
	{
		result += gModeStrs[ndx];
		if (!(gModeStrs[++ndx].empty()))
			result += "|";
	}
	return result;
}

OutputMode	CCGrabberConfig::StringToOutputMode (const string & inModeStr)
{
	typedef pair<string,OutputMode>	StringToOutputModePair;
	typedef map<string,OutputMode>	StringToOutputModeMap;
	typedef StringToOutputModeMap::const_iterator	StringToOutputModeConstIter;
	static StringToOutputModeMap	sStringToOutputModeMap;
	static AJALock					sStringToOutputModeMapLock;
	if (sStringToOutputModeMap.empty())
	{
		AJAAutoLock	autoLock(&sStringToOutputModeMapLock);
		for (OutputMode om(kOutputMode_CaptionStream);  om < kOutputMode_INVALID;  om = OutputMode(om+1))
		{
			string	outputModeStr (OutputModeToString(om));
			aja::lower(outputModeStr);
			sStringToOutputModeMap.insert(StringToOutputModePair(outputModeStr,om));
		}
	}
	string	modeStr(inModeStr);
	aja::lower(aja::strip(modeStr));
	StringToOutputModeConstIter iter(sStringToOutputModeMap.find(modeStr));
	return iter != sStringToOutputModeMap.end()  ?  iter->second  :  kOutputMode_INVALID;
}

string	CCGrabberConfig::GetLegalCaptionDataSources (void)
{
	string	result;
	for (CaptionDataSrc src(kCaptionDataSrc_Default);  src < kCaptionDataSrc_INVALID;  )
	{
		result += CaptionDataSrcToString(src);
		src = CaptionDataSrc(src+1);
		if (src < kCaptionDataSrc_INVALID)
			result += ",";
	}
	return result;
}

string	CCGrabberConfig::CaptionDataSrcToString (const CaptionDataSrc inDataSrc)
{
	static const string gSrcStrs[]	= {"default","line21","608vanc","708vanc","608anc","708anc",""};
	if (IS_VALID_CaptionDataSrc(inDataSrc))
		return gSrcStrs[inDataSrc];
	string result;
	for (unsigned ndx(0);  ndx < 6;  )
	{
		result += gSrcStrs[ndx];
		if (!gSrcStrs[++ndx].empty())
			result += "|";
	}
	return result;
}

CaptionDataSrc	CCGrabberConfig::StringToCaptionDataSrc (const string & inDataSrcStr)
{
	typedef pair<string,CaptionDataSrc>	StringToCaptionDataSrcPair;
	typedef map<string,CaptionDataSrc>	StringToCaptionDataSrcMap;
	typedef StringToCaptionDataSrcMap::const_iterator	StringToCaptionDataSrcConstIter;
	static StringToCaptionDataSrcMap	sStringToCaptionDataSrcMap;
	static AJALock						sStringToCaptionDataSrcMapLock;
	if (sStringToCaptionDataSrcMap.empty())
	{
		AJAAutoLock	autoLock(&sStringToCaptionDataSrcMapLock);
		for (CaptionDataSrc cds(kCaptionDataSrc_Default);  cds < kCaptionDataSrc_INVALID;  cds = CaptionDataSrc(cds+1))
		{
			string	captionDataSrcStr (CaptionDataSrcToString(cds));
			aja::lower(captionDataSrcStr);
			sStringToCaptionDataSrcMap.insert(StringToCaptionDataSrcPair(captionDataSrcStr,cds));
		}
	}
	string	cdsStr(inDataSrcStr);
	aja::lower(aja::strip(cdsStr));
	StringToCaptionDataSrcConstIter iter(sStringToCaptionDataSrcMap.find(cdsStr));
	return iter != sStringToCaptionDataSrcMap.end()  ?  iter->second  :  kCaptionDataSrc_INVALID;
}


std::ostream & operator << (std::ostream & ioStrm,  const CCGrabberConfig & inObj)
{
	ioStrm	<< AJASystemInfo::ToString(inObj.Get());
	return ioStrm;
}
