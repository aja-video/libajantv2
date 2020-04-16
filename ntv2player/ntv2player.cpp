/**
	@file		ntv2player.cpp
	@brief		Implementation of NTV2Player class.
	@copyright	(C) 2013-2020 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2player.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2debug.h"
#include "ntv2testpatterngen.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/timecode.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"
#include "ajaanc/includes/ancillarydata_hdr_sdr.h"
#include "ajaanc/includes/ancillarydata_hdr_hdr10.h"
#include "ajaanc/includes/ancillarydata_hdr_hlg.h"
#include "ajaanc/includes/ancillarylist.h"

using namespace std;

//	Convenience macros for EZ logging:
#define	TCFAIL(_expr_)	AJA_sERROR  (AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCWARN(_expr_)	AJA_sWARNING(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCNOTE(_expr_)	AJA_sNOTICE	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCINFO(_expr_)	AJA_sINFO	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCDBG(_expr_)	AJA_sDEBUG	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)

#define NTV2_ANCSIZE_MAX	(0x2000)


AJALabelValuePairs PlayerConfig::Get(const bool inCompact) const
{
	AJALabelValuePairs result;
	AJASystemInfo::append(result, "CCPlayer Config");
	AJASystemInfo::append(result, "Device Specifier",	fDeviceSpecifier);
	AJASystemInfo::append(result, "Output Channel",		::NTV2ChannelToString(fOutputChannel, inCompact));
	AJASystemInfo::append(result, "Output Connector",	::NTV2OutputDestinationToString(fOutputDestination, inCompact));
	AJASystemInfo::append(result, "Video Format",		::NTV2VideoFormatToString(fVideoFormat));
	AJASystemInfo::append(result, "Pixel Format",		::NTV2FrameBufferFormatToString(fPixelFormat, inCompact));
	AJASystemInfo::append(result, "AutoCirc Frames",	fFrames.toString());
	AJASystemInfo::append(result, "Anc Data",			::AJAAncillaryDataTypeToString(fTransmitHDRType));
	AJASystemInfo::append(result, "MultiFormat Mode",	fDoMultiFormat ? "Y" : "N");
	AJASystemInfo::append(result, "Suppress Audio",		fSuppressAudio ? "Y" : "N");
	AJASystemInfo::append(result, "Embedded Timecode",	fTransmitLTC ? "LTC" : "VITC");
//	AJASystemInfo::append(result, "Level Conversion",	fDoLevelConversion ? "Y" : "N");
	return result;
}

ostream & operator << (ostream & ioStrm, const PlayerConfig & inObj)
{
	return ioStrm << AJASystemInfo::ToString(inObj.Get());
}

NTV2Player::NTV2Player (const PlayerConfig & inConfig)

	:	mConsumerThread		(),
		mProducerThread		(),
		mDeviceID			(DEVICE_ID_NOTFOUND),
		mConfig				(inConfig),
		mSavedTaskMode		(NTV2_DISABLE_TASKS),
		mAudioSystem		(NTV2_AUDIOSYSTEM_1),
		mGlobalQuit			(false),
		mHostBuffers		(),
		mAVCircularBuffer	(),
		mCurrentFrame		(0),
		mCurrentSample		(0),
		mToneFrequency		(440.0),
		mTCBurner			(),
		mTCIndexes			(),
		mTestPatVidBuffers	()
{
}


NTV2Player::~NTV2Player (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit();

	mDevice.UnsubscribeOutputVerticalEvent(mConfig.fOutputChannel);
}	//	destructor


void NTV2Player::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	while (mProducerThread.Active())
		AJATime::Sleep(10);

	while (mConsumerThread.Active())
		AJATime::Sleep(10);

	if (!mConfig.fDoMultiFormat  &&  mDevice.IsOpen())
	{
		mDevice.ReleaseStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));
		mDevice.SetEveryFrameServices(mSavedTaskMode);		//	Restore prior task mode
	}

}	//	Quit


AJAStatus NTV2Player::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpecifier, mDevice))
		{if (mConfig.fDeviceSpecifier != "list" && mConfig.fDeviceSpecifier != "?")
			cerr << "## ERROR:  Device '" << mConfig.fDeviceSpecifier << "' not found" << endl;
				return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpecifier << "' not ready" << endl;
			return AJA_STATUS_INITIALIZE;}

	if (!mConfig.fDoMultiFormat)
	{
		if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
			return AJA_STATUS_BUSY;		//	Device is in use by another app -- fail

		mDevice.GetEveryFrameServices(mSavedTaskMode);		//	Save the current service level
	}

	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);			//	Set OEM service level

	mDeviceID = mDevice.GetDeviceID();						//	Keep this ID handy -- it's used frequently

	if (::NTV2DeviceCanDoMultiFormat(mDeviceID) && mConfig.fDoMultiFormat)
		mDevice.SetMultiFormatMode(true);
	else if (::NTV2DeviceCanDoMultiFormat(mDeviceID))
		mDevice.SetMultiFormatMode(false);

	//	Beware -- some devices (e.g. Corvid1) can only output from FrameStore 2...
	if ((mConfig.fOutputChannel == NTV2_CHANNEL1) && (!::NTV2DeviceCanDoFrameStore1Display(mDeviceID)))
		mConfig.fOutputChannel = NTV2_CHANNEL2;
	if (UWord(mConfig.fOutputChannel) >= ::NTV2DeviceGetNumFrameStores(mDeviceID))
	{
		cerr	<< "## ERROR:  Cannot use channel '" << DEC(mConfig.fOutputChannel+1) << "' -- device only supports channel 1"
				<< (::NTV2DeviceGetNumFrameStores(mDeviceID) > 1  ?  string(" thru ") + string(1, char(::NTV2DeviceGetNumFrameStores(mDeviceID)+'0'))  :  "") << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	//	Set up the video and audio...
	status = SetUpVideo();
	if (AJA_FAILURE (status))
		return status;

	status = SetUpAudio();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the circular buffers, and the test pattern buffers...
	SetUpHostBuffers();
	status = SetUpTestPatternVideoBuffers ();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the device signal routing, and playout AutoCirculate...
	RouteOutputSignal();

	//	Lastly, prepare my AJATimeCodeBurn instance...
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat(mConfig.fPixelFormat),
									mFormatDesc.numPixels, mFormatDesc.numLines);
	#if defined(_DEBUG)
		cerr << mConfig << endl;
	#endif//	defined(_DEBUG)
	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Player::SetUpVideo (void)
{
	//	Configure the device to output the requested video format...
	if (!::NTV2DeviceCanDoVideoFormat (mDeviceID, mConfig.fVideoFormat))
		{cerr << "## ERROR:  Video format '" << ::NTV2VideoFormatToString(mConfig.fVideoFormat) << "' not supported on this device" << endl;
			return AJA_STATUS_UNSUPPORTED;}
	mDevice.SetVideoFormat (mConfig.fVideoFormat, false, false, mConfig.fOutputChannel);

	if (!::NTV2DeviceCanDo3GLevelConversion(mDeviceID) && mConfig.fDoLevelConversion && ::IsVideoFormatA(mConfig.fVideoFormat))
		mConfig.fDoLevelConversion = false;
	if (mConfig.fDoLevelConversion)
		mDevice.SetSDIOutLevelAtoLevelBConversion (mConfig.fOutputChannel, mConfig.fDoLevelConversion);

	//	Set the frame buffer pixel format for the device FrameStore...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.fPixelFormat))
		{cerr << "## ERROR: Pixel format '" << ::NTV2FrameBufferFormatString(mConfig.fPixelFormat) << "' not supported on this device" << endl;
			return AJA_STATUS_UNSUPPORTED;}
	mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);

	//	This demo doesn't playout dual-link RGB over SDI -- only YCbCr.
	//	Check that this device has a CSC to convert RGB to YUV...
	if (::IsRGBFormat(mConfig.fPixelFormat))	//	If RGB FBF...
		if (UWord(mConfig.fOutputChannel) > ::NTV2DeviceGetNumCSCs(mDeviceID))	//	No CSC for this channel?
			{cerr << "## ERROR: No CSC for channel " << DEC(mConfig.fOutputChannel+1) << " to convert RGB pixel format" << endl;
				return AJA_STATUS_UNSUPPORTED;}

	mFormatDesc = NTV2FormatDescriptor(mConfig.fVideoFormat, mConfig.fPixelFormat);
	if (!mFormatDesc.IsValid())
		return AJA_STATUS_FAIL;

	//	Set output clock reference...
	if (mDeviceID == DEVICE_ID_KONAIP_1RX_1TX_2110  ||  mDeviceID == DEVICE_ID_KONAIP_2110)
		mDevice.SetReference(NTV2_REFERENCE_SFP1_PTP);
	else
		mDevice.SetReference(NTV2_REFERENCE_FREERUN);
	mDevice.EnableChannel(mConfig.fOutputChannel);

	//	This demo assumes VANC is disabled...
	NTV2VANCMode vancMode(NTV2_VANCMODE_INVALID);
	mDevice.GetVANCMode(vancMode, mConfig.fOutputChannel);
	if (NTV2_IS_VANCMODE_ON(vancMode))
	{
		NTV2Standard	standard(NTV2_STANDARD_UNDEFINED);
		NTV2FrameGeometry	fg(NTV2_FG_INVALID);
		mDevice.GetStandard(standard, mConfig.fOutputChannel);
		mDevice.GetFrameGeometry(fg, mConfig.fOutputChannel);
		mDevice.SetVANCMode(NTV2_VANCMODE_OFF, standard, fg, mConfig.fOutputChannel);
		if (::Is8BitFrameBufferFormat(mConfig.fPixelFormat))
			mDevice.SetVANCShiftMode(mConfig.fOutputChannel, NTV2_VANCDATA_NORMAL);
	}

	//	The output interrupt is Enabled by default, but on some platforms, you must subscribe to it...
	mDevice.SubscribeOutputVerticalEvent(mConfig.fOutputChannel);

	//	Check if HDR anc is permissible...
	if (mConfig.fTransmitHDRType != AJAAncillaryDataType_Unknown  &&  !::NTV2DeviceCanDoCustomAnc(mDeviceID))
		{cerr << "## WARNING:  HDR Anc requested, but device can't do custom anc" << endl;
			mConfig.fTransmitHDRType = AJAAncillaryDataType_Unknown;}

	//	At this point, video setup is complete (except for widget signal routing).
	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2Player::SetUpAudio (void)
{
	const uint16_t numAudioChannels (::NTV2DeviceGetMaxAudioChannels(mDeviceID));

	if (mConfig.fSuppressAudio)
	{
		mAudioSystem = NTV2_AUDIOSYSTEM_INVALID;
		return AJA_STATUS_SUCCESS;
	}

	//	Use NTV2_AUDIOSYSTEM_1, unless the device has more than one audio system...
	if (::NTV2DeviceGetNumAudioSystems(mDeviceID) > 1)
		mAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fOutputChannel);	//	...and base it on the channel
	//	However, there are a few older devices that have only 1 audio system,
	//	yet 2 frame stores (or must use channel 2 for playout)...
	if (!::NTV2DeviceCanDoFrameStore1Display(mDeviceID))
		mAudioSystem = NTV2_AUDIOSYSTEM_1;

	mDevice.SetNumberAudioChannels (numAudioChannels, mAudioSystem);
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set the SDI output audio embedders to embed audio samples from the output of mAudioSystem...
	mDevice.SetSDIOutputAudioSystem (mConfig.fOutputChannel, mAudioSystem);
	mDevice.SetSDIOutputDS2AudioSystem (mConfig.fOutputChannel, mAudioSystem);

	//	If the last app using the device left it in E-E mode (input passthru), then loopback
	//	must be disabled --- otherwise the output audio will be pass-thru SDI input audio...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio

#define AsU8Ptr(__p__)	reinterpret_cast<uint8_t*>(__p__)
#define	AJAAncDataType_HDR_SDR		AJAAncillaryDataType_HDR_SDR
#define	AJAAncDataType_HDR_HDR10	AJAAncillaryDataType_HDR_HDR10
#define	AJAAncDataType_HDR_HLG		AJAAncillaryDataType_HDR_HLG


void NTV2Player::SetUpHostBuffers (void)
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	//	HDR anc, if requested, doesn't change per-frame, so make one buffer with the packet data...
	NTV2_POINTER ancBuf;
	if (mConfig.fTransmitHDRType != AJAAncillaryDataType_Unknown)
	{
		AJAAncillaryData_HDR_SDR	sdrPkt;
		AJAAncillaryData_HDR_HDR10	hdr10Pkt;
		AJAAncillaryData_HDR_HLG	hlgPkt;
		ancBuf.Allocate(NTV2_ANCSIZE_MAX);
		uint32_t pktSize(0);
		switch (mConfig.fTransmitHDRType)
		{
			case AJAAncDataType_HDR_SDR:	sdrPkt.GenerateTransmitData(AsU8Ptr(ancBuf.GetHostPointer()),	ancBuf.GetByteCount(), pktSize);	break;
			case AJAAncDataType_HDR_HDR10:	hdr10Pkt.GenerateTransmitData(AsU8Ptr(ancBuf.GetHostPointer()),	ancBuf.GetByteCount(), pktSize);	break;
			case AJAAncDataType_HDR_HLG:	hlgPkt.GenerateTransmitData(AsU8Ptr(ancBuf.GetHostPointer()),	ancBuf.GetByteCount(), pktSize);	break;
			default:						break;
		}
	}

	//	Allocate and add each in-host NTV2FrameData to my circular buffer member variable...
	mHostBuffers.reserve(CIRCULAR_BUFFER_SIZE);
	while (mHostBuffers.size() < CIRCULAR_BUFFER_SIZE)
	{
		mHostBuffers.push_back(NTV2FrameData());		//	Make a new NTV2FrameData...
		NTV2FrameData & frameData(mHostBuffers.back());	//	...and get a reference to it
		//	Allocate a page-aligned video buffer
		frameData.fVideoBuffer.Allocate(mFormatDesc.GetTotalBytes(), /*pageAlign?*/true);
		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))
			frameData.fAudioBuffer.Allocate(1UL*1024UL*1024UL);	//	Allocate audio buffer
		if (ancBuf  &&  mHostBuffers.size() == 1)	//	Only first has Anc buffer
			frameData.fAncBuffer.CopyFrom(ancBuf.GetHostPointer(), ancBuf.GetByteCount());
		mAVCircularBuffer.Add(&frameData);
	}	//	for each NTV2FrameData

}	//	SetUpHostBuffers


void NTV2Player::RouteOutputSignal (void)
{
	const NTV2Standard	outputStandard	(::GetNTV2StandardFromVideoFormat(mConfig.fVideoFormat));
	const UWord			numSDIOutputs	(::NTV2DeviceGetNumVideoOutputs (mDeviceID));
	bool				isRGB			(::IsRGBFormat(mConfig.fPixelFormat));

	//	Since this function figures out which SDI spigots will be set up for output,
	//	it also sets the "mTCIndexes" member, which determines which timecodes will
	//	be transmitted (and on which SDI spigots)...
	mTCIndexes.clear();

	const NTV2OutputCrosspointID cscVidOutXpt(::GetCSCOutputXptFromChannel(mConfig.fOutputChannel,  false/*isKey*/,  !isRGB/*isRGB*/));
	const NTV2OutputCrosspointID fsVidOutXpt (::GetFrameBufferOutputXptFromChannel(mConfig.fOutputChannel,  isRGB/*isRGB*/,  false/*is425*/));
	if (isRGB)
		mDevice.Connect (::GetCSCInputXptFromChannel(mConfig.fOutputChannel, false/*isKeyInput*/),  fsVidOutXpt);

	if (mConfig.fDoMultiFormat)
	{
		//	Multiformat --- We may be sharing the device with other processes, so route only one SDI output...
		if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
			mDevice.SetSDITransmitEnable(mConfig.fOutputChannel, true);

		mDevice.Connect (::GetSDIOutputInputXpt (mConfig.fOutputChannel, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt);
		mTCIndexes.insert (::NTV2ChannelToTimecodeIndex(mConfig.fOutputChannel, /*inEmbeddedLTC=*/mConfig.fTransmitLTC));
		//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically
		mDevice.SetSDIOutputStandard (mConfig.fOutputChannel, outputStandard);
	}
	else
	{
		//	Not multiformat:  We own the whole device, so connect all possible SDI outputs...
		const UWord	numFrameStores(::NTV2DeviceGetNumFrameStores(mDeviceID));
		mDevice.ClearRouting();		//	Start with clean slate

		if (isRGB)
			mDevice.Connect (::GetCSCInputXptFromChannel (mConfig.fOutputChannel, false/*isKeyInput*/),  fsVidOutXpt);

		for (NTV2Channel chan(NTV2_CHANNEL1);  ULWord(chan) < numSDIOutputs;  chan = NTV2Channel(chan+1))
		{
			if (chan != mConfig.fOutputChannel  &&  chan < numFrameStores)
				mDevice.DisableChannel(chan);				//	Disable unused FrameStore
			if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
				mDevice.SetSDITransmitEnable (chan, true);	//	Make it an output

			const NTV2OutputDestination sdiOutput(::NTV2ChannelToOutputDestination(chan));
			if (NTV2_OUTPUT_DEST_IS_SDI(sdiOutput))
				if (OutputDestHasRP188BypassEnabled(sdiOutput))
					DisableRP188Bypass(sdiOutput);
			mDevice.Connect (::GetSDIOutputInputXpt (chan, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt);
			mDevice.SetSDIOutputStandard (chan, outputStandard);
			mTCIndexes.insert (::NTV2ChannelToTimecodeIndex (chan, /*inEmbeddedLTC=*/mConfig.fTransmitLTC));	//	Add SDI spigot's TC index
			//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically
		}	//	for each SDI output spigot

		//	And connect analog video output, if the device has one...
		if (::NTV2DeviceGetNumAnalogVideoOutputs(mDeviceID))
			mDevice.Connect (::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_ANALOG),  isRGB ? cscVidOutXpt : fsVidOutXpt);

		//	And connect HDMI video output, if the device has one...
		if (::NTV2DeviceGetNumHDMIVideoOutputs(mDeviceID))
			mDevice.Connect (::GetOutputDestInputXpt(NTV2_OUTPUTDESTINATION_HDMI),  isRGB ? cscVidOutXpt : fsVidOutXpt);
	}
	TCNOTE(mTCIndexes);

}	//	RouteOutputSignal


AJAStatus NTV2Player::Run (void)
{
	//	Start my consumer and producer threads...
	StartConsumerThread();
	StartProducerThread();
	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2Player::StartConsumerThread (void)
{
	//	Create and start the playout thread...
	mConsumerThread.Attach (ConsumerThreadStatic, this);
	mConsumerThread.SetPriority(AJA_ThreadPriority_High);
	mConsumerThread.Start();

}	//	StartConsumerThread


//	The playout thread function
void NTV2Player::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Player instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2Player * pApp (reinterpret_cast<NTV2Player*>(pContext));
	if (pApp)
		pApp->PlayFrames();

}	//	ConsumerThreadStatic


void NTV2Player::PlayFrames (void)
{
	ULWord	acOptions(AUTOCIRCULATE_WITH_RP188), goodXfers(0), badXfers(0), prodWaits(0), noRoomWaits(0);
	AUTOCIRCULATE_TRANSFER	xferInfo;

	PLNOTE("Thread started");
	if (mHostBuffers.at(0).fAncBuffer)	//	HDR anc in 1st NTV2FrameData's anc buffer (see SetUpHostBuffers)
	{	//	Since HDR anc packet data doesn't change per-frame, set it once here...
		NTV2_POINTER & ancBuf(mHostBuffers.at(0).fAncBuffer);
		xferInfo.acANCBuffer.CopyFrom(ancBuf.GetHostPointer(), ancBuf.GetByteCount());
		acOptions |= AUTOCIRCULATE_WITH_ANC;
	}

	//	Initialize AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,
										mConfig.fFrames.count(),
										mAudioSystem,
										acOptions,
										1,	// numChannels to gang
										mConfig.fFrames.firstFrame(), mConfig.fFrames.lastFrame());
	//	Start AutoCirculate...
	mDevice.AutoCirculateStart(mConfig.fOutputChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS	outputStatus;
		mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outputStatus);

		//	Check if there's room for another frame on the card...
		if (outputStatus.GetNumAvailableOutputFrames() > 1)
		{
			//	Device has one or more free frame buffers that can be filled for playout.
			//	Wait for the next frame in our ring to become ready to "consume"...
			NTV2FrameData *	pFrameData (mAVCircularBuffer.StartConsumeNextBuffer());
			if (!pFrameData)
				{prodWaits++;  continue;}

			xferInfo.SetOutputTimeCodes(pFrameData->fTimecodes);

			//	Transfer the timecode-burned frame to the device for playout...
			xferInfo.SetVideoBuffer (pFrameData->VideoBuffer(), pFrameData->VideoBufferSize());
			//	If also playing audio...
			if (pFrameData->fAudioBuffer)	//	...also xfer this frame's audio samples...
				xferInfo.SetAudioBuffer (pFrameData->AudioBuffer(), pFrameData->fNumAudioBytes);

			//	Perform the DMA transfer to the device...
			if (mDevice.AutoCirculateTransfer (mConfig.fOutputChannel, xferInfo))
				goodXfers++;
			else
				badXfers++;

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer();
			continue;	//	Back to top of while loop
		}

		//	Wait for one or more buffers to become available on the device, which should occur at next VBI...
		noRoomWaits++;
		mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel);
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	PLNOTE("Thread completed: " << DEC(goodXfers) << " xfers, " << DEC(badXfers) << " failed, "
			<< DEC(prodWaits) << " starves, " << DEC(noRoomWaits) << " VBI waits");

}	//	PlayFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2Player::StartProducerThread (void)
{
	//	Create and start the producer thread...
	mProducerThread.Attach (ProducerThreadStatic, this);
	mProducerThread.SetPriority(AJA_ThreadPriority_High);
	mProducerThread.Start();

}	//	StartProducerThread


void NTV2Player::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	NTV2Player * pApp (reinterpret_cast<NTV2Player*>(pContext));
	if (pApp)
		pApp->ProduceFrames();

}	//	ProducerThreadStatic


AJAStatus NTV2Player::SetUpTestPatternVideoBuffers (void)
{
	static const NTV2TestPatternSelect	sTestPatternTypes[] = {	NTV2_TestPatt_ColorBars100,
																NTV2_TestPatt_ColorBars75,
																NTV2_TestPatt_Ramp,
																NTV2_TestPatt_MultiBurst,
																NTV2_TestPatt_LineSweep,
																NTV2_TestPatt_CheckField,
																NTV2_TestPatt_FlatField,
																NTV2_TestPatt_MultiPattern,
																NTV2_TestPatt_INVALID};
	if (!mFormatDesc.IsValid())
		return AJA_STATUS_FAIL;
	if (mFormatDesc.firstActiveLine)
		{cerr << "## ERROR:  VANC should have been disabled: " << mFormatDesc << endl;
			return AJA_STATUS_FAIL;}	//	if VANC somehow enabled

	//	Set up one video buffer for each of the several predefined patterns...
	for (uint32_t ndx(0);  sTestPatternTypes[ndx] != NTV2_TestPatt_INVALID;  ndx++)
	{
		//	Allocate the buffer memory...
		mTestPatVidBuffers.push_back(NTV2_POINTER(mFormatDesc.GetTotalBytes()));
		NTV2_POINTER & theBuffer(mTestPatVidBuffers.at(mTestPatVidBuffers.size()-1));

		//	Use the test pattern generator to fill the buffer...
		NTV2TestPatternGen	testPatternGen;
		if (!testPatternGen.DrawTestPattern (sTestPatternTypes[ndx], mFormatDesc, theBuffer))
		{
			cerr << "## ERROR:  DrawTestPattern failed, formatDesc: " << mFormatDesc << endl;
			return AJA_STATUS_FAIL;
		}
	}	//	for each test pattern
	return AJA_STATUS_SUCCESS;

}	//	SetUpTestPatternVideoBuffers


static const double	gFrequencies []	=	{250.0, 500.0, 1000.0, 2000.0};
static const ULWord	gNumFrequencies		(sizeof (gFrequencies) / sizeof (double));
static const double	gAmplitudes []	=	{	0.10, 0.15,		0.20, 0.25,		0.30, 0.35,		0.40, 0.45,		0.50, 0.55,		0.60, 0.65,		0.70, 0.75,		0.80, 0.85,
											0.85, 0.80,		0.75, 0.70,		0.65, 0.60,		0.55, 0.50,		0.45, 0.40,		0.35, 0.30,		0.25, 0.20,		0.15, 0.10};


void NTV2Player::ProduceFrames (void)
{
	ULWord	frequencyIndex(0), testPatternIndex(0), badTally(0);
	double	timeOfLastSwitch	(0.0);

	const AJATimeBase			timeBase		(CNTV2DemoCommon::GetAJAFrameRate(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat)));
	const NTV2TestPatternNames	tpNames			(NTV2TestPatternGen::getTestPatternNames());
	const bool					isInterlace		(!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mConfig.fVideoFormat));
	const bool					isPAL			(NTV2_IS_PAL_VIDEO_FORMAT(mConfig.fVideoFormat));
	const NTV2FrameRate			ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
	const TimecodeFormat		tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));

	PLNOTE("Thread started");
	while (!mGlobalQuit)
	{
		NTV2FrameData *	pFrameData(mAVCircularBuffer.StartProduceNextBuffer());
		if (!pFrameData)
		{	badTally++;			//	No frame available!
			AJATime::Sleep(10);	//	Wait a bit...
			continue;			//	...try again later
		}

		//	Copy pre-made test pattern into this frame's video buffer...
		pFrameData->fVideoBuffer.CopyFrom (mTestPatVidBuffers.at(testPatternIndex),
											/*srcOffset*/ 0,
											/*dstOffset*/ 0,
											/*byteCount*/ pFrameData->fVideoBuffer.GetByteCount());

		const	CRP188	rp188Info (mCurrentFrame++, 0, 0, 10, tcFormat);
		NTV2_RP188		tcF1, tcF2;
		string			tcString;

		rp188Info.GetRP188Reg(tcF1);
		rp188Info.GetRP188Str(tcString);

		//	Include timecode in output signal...
		tcF2 = tcF1;
		if (isInterlace)
		{	//	Set bit 27 of Hi word (PAL) or Lo word (NTSC)
			if (isPAL) tcF2.fHi |=  BIT(27);	else tcF2.fLo |=  BIT(27);
		}

		//	Add timecodes for each SDI output...
		for (NTV2TCIndexesConstIter it(mTCIndexes.begin());  it != mTCIndexes.end();  ++it)
			pFrameData->fTimecodes[*it] = NTV2_IS_ATC_VITC2_TIMECODE_INDEX(*it) ? tcF2 : tcF1;

		//	Burn current timecode into the video buffer...
		mTCBurner.BurnTimeCode (pFrameData->VideoBytes(), tcString.c_str(), 80);
		TCDBG("F" << DEC0N(mCurrentFrame-1,6) << ": " << tcF1 << ": " << tcString);

		//	If also playing audio...
		if (pFrameData->fAudioBuffer)	//	...generate audio tone data for this frame...
			pFrameData->fNumAudioBytes = AddTone(*pFrameData);	//	...and remember number of audio bytes to xfer

		//	Every few seconds, change the test pattern and tone frequency...
		const double currentTime (timeBase.FramesToSeconds(mCurrentFrame));
		if (currentTime > timeOfLastSwitch + 4.0)
		{
			frequencyIndex = (frequencyIndex + 1) % gNumFrequencies;
			testPatternIndex = (testPatternIndex + 1) % mTestPatVidBuffers.size();
			mToneFrequency = gFrequencies[frequencyIndex];
			timeOfLastSwitch = currentTime;
			PLINFO("F" << DEC0N(mCurrentFrame,6) << ": " << tcString << ": tone=" << mToneFrequency
					<< "Hz, pattern='" << tpNames.at(testPatternIndex) << "'");
		}	//	if time to switch test pattern & tone frequency

		//	Signal that I'm done producing the buffer, making it immediately available for transfer/playout...
		mAVCircularBuffer.EndProduceNextBuffer ();

	}	//	loop til mGlobalQuit goes true
	PLNOTE("Thread completed: " << DEC(mCurrentFrame) << " frame(s) produced, " << DEC(badTally) << " failed");

}	//	ProduceFrames


void NTV2Player::GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel)
{
	AUTOCIRCULATE_STATUS	status;
	mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, status);
	outGoodFrames = status.acFramesProcessed;
	outDroppedFrames = status.acFramesDropped;
	outBufferLevel = status.acBufferLevel;
}


uint32_t NTV2Player::AddTone (NTV2FrameData & inFrameData)
{
	NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
	NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
	ULWord			numChannels	(0);

	mDevice.GetFrameRate (frameRate, mConfig.fOutputChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);

	//	Set per-channel tone frequencies...
	double	pFrequencies [kNumAudioChannelsMax];
	pFrequencies[0] = (mToneFrequency / 2.0);
	for (ULWord chan(1);  chan < numChannels;  chan++)
		//	The 1.154782 value is the 16th root of 10, to ensure that if mToneFrequency is 2000,
		//	that the calculated frequency of audio channel 16 will be 20kHz...
		pFrequencies[chan] = pFrequencies[chan-1] * 1.154782;

	//	Since audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will
	//	necessarily result in some frames having more audio samples than others. GetAudioSamplesPerFrame is
	//	called to calculate the correct sample count for the current frame...
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));
	const double	sampleRateHertz	(audioRate == NTV2_AUDIO_96K ? 96000.0 : 48000.0);

	return ::AddAudioTone (	inFrameData.AudioBuffer(),	//	buffer to fill
							mCurrentSample,				//	which sample for continuing the waveform
							numSamples,					//	number of samples to generate
							sampleRateHertz,			//	sample rate [Hz]
							gAmplitudes,				//	per-channel amplitudes
							pFrequencies,				//	per-channel tone frequencies [Hz]
							31,							//	bits per sample
							false,						//	don't do byte swapping
							numChannels);				//	number of audio channels to generate
}	//	AddTone


ULWord NTV2Player::GetRP188RegisterForOutput (const NTV2OutputDestination inOutputDest)		//	static
{
	switch (inOutputDest)
	{
		case NTV2_OUTPUTDESTINATION_SDI1:	return kRegRP188InOut1DBB;	//	reg 29
		case NTV2_OUTPUTDESTINATION_SDI2:	return kRegRP188InOut2DBB;	//	reg 64
		case NTV2_OUTPUTDESTINATION_SDI3:	return kRegRP188InOut3DBB;	//	reg 268
		case NTV2_OUTPUTDESTINATION_SDI4:	return kRegRP188InOut4DBB;	//	reg 273
		case NTV2_OUTPUTDESTINATION_SDI5:	return kRegRP188InOut5DBB;	//	reg 29
		case NTV2_OUTPUTDESTINATION_SDI6:	return kRegRP188InOut6DBB;	//	reg 64
		case NTV2_OUTPUTDESTINATION_SDI7:	return kRegRP188InOut7DBB;	//	reg 268
		case NTV2_OUTPUTDESTINATION_SDI8:	return kRegRP188InOut8DBB;	//	reg 273
		default:							return 0;
	}	//	switch on output destination

}	//	GetRP188RegisterForOutput


bool NTV2Player::OutputDestHasRP188BypassEnabled (const NTV2OutputDestination inOutputDest)
{
	bool			result	(false);
	const ULWord	regNum	(GetRP188RegisterForOutput(inOutputDest));
	ULWord			regValue(0);

	//	Bit 23 of the RP188 DBB register will be set if output timecode is pulled
	//	directly from an SDI input (bypass source)...
	if (regNum  &&  mDevice.ReadRegister(regNum, regValue)  &&  regValue & BIT(23))
		result = true;

	return result;

}	//	OutputDestHasRP188BypassEnabled


void NTV2Player::DisableRP188Bypass (const NTV2OutputDestination inOutputDest)
{
	//	Clear bit 23 of SDI output's RP188 DBB register...
	const ULWord regNum (GetRP188RegisterForOutput(inOutputDest));
	if (regNum)
		mDevice.WriteRegister (regNum, 0, BIT(23), 23);

}	//	DisableRP188Bypass
