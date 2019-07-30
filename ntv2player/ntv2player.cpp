/**
	@file		ntv2player.cpp
	@brief		Implementation of NTV2Player class.
	@copyright	Copyright (C) 2013-2019 AJA Video Systems, Inc.  All rights reserved.
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
#define	PLFAIL(_expr_)	AJA_sERROR  (AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	PLWARN(_expr_)	AJA_sWARNING(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	PLNOTE(_expr_)	AJA_sNOTICE	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	PLINFO(_expr_)	AJA_sINFO	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	PLDBG(_expr_)	AJA_sDEBUG	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)

#define NTV2_ANCSIZE_MAX	(0x2000)

/**
	@brief	The maximum number of bytes of 48KHz audio that can be transferred for a single frame.
			Worst case, assuming 16 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			48,000 samples per second requires 3,204 samples x 4 bytes/sample x 16 = 205,056 bytes
			201K will suffice, with 768 bytes to spare
**/
static const uint32_t	AUDIOBYTES_MAX_48K	(201 * 1024);

/**
	@brief	The maximum number of bytes of 96KHz audio that can be transferred for a single frame.
			Worst case, assuming 16 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			96,000 samples per second requires 6,408 samples x 4 bytes/sample x 16 = 410,112 bytes
			401K will suffice, with 512 bytes to spare
**/
static const uint32_t	AUDIOBYTES_MAX_96K	(401 * 1024);

/**
	@brief	Used when reserving the AJA device, this specifies the application signature.
**/
static const ULWord		kAppSignature	(AJA_FOURCC ('D','E','M','O'));



NTV2Player::NTV2Player (const string &				inDeviceSpecifier,
						const bool					inWithAudio,
						const NTV2Channel			inChannel,
						const NTV2FrameBufferFormat	inPixelFormat,
						const NTV2OutputDestination	inOutputDestination,
						const NTV2VideoFormat		inVideoFormat,
						const bool					inEnableVanc,
						const bool					inLevelConversion,
						const bool					inDoMultiChannel,
						const bool					inXmitLTC,
						const AJAAncillaryDataType	inSendHDRType)

	:	mConsumerThread				(AJA_NULL),
		mProducerThread				(AJA_NULL),
		mCurrentFrame				(0),
		mCurrentSample				(0),
		mToneFrequency				(440.0),
		mDeviceSpecifier			(inDeviceSpecifier),
		mDeviceID					(DEVICE_ID_NOTFOUND),
		mOutputChannel				(inChannel),
		mOutputDestination			(inOutputDestination),
		mVideoFormat				(inVideoFormat),
		mPixelFormat				(inPixelFormat),
		mSavedTaskMode				(NTV2_DISABLE_TASKS),
		mAudioSystem				(NTV2_AUDIOSYSTEM_1),
		mVancMode					(NTV2_VANCMODE_OFF),
		mWithAudio					(inWithAudio),
		mEnableVanc					(inEnableVanc),
		mGlobalQuit					(false),
		mDoLevelConversion			(inLevelConversion),
		mDoMultiChannel				(inDoMultiChannel),
		mXmitLTC					(inXmitLTC),
		mVideoBufferSize			(0),
		mAudioBufferSize			(0),
		mTestPatternVideoBuffers	(AJA_NULL),
		mNumTestPatterns			(0),
		mAncType					(inSendHDRType)
{
	::memset (mAVHostBuffer, 0, sizeof (mAVHostBuffer));
}


NTV2Player::~NTV2Player (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit ();

	mDevice.UnsubscribeOutputVerticalEvent (mOutputChannel);

	//	Free my threads and buffers...
	delete mConsumerThread;
	mConsumerThread = AJA_NULL;
	delete mProducerThread;
	mProducerThread = AJA_NULL;

	if (mTestPatternVideoBuffers)
	{
		for (uint32_t ndx(0);  ndx < mNumTestPatterns;  ndx++)
			delete [] mTestPatternVideoBuffers [ndx];
		delete [] mTestPatternVideoBuffers;
		mTestPatternVideoBuffers = AJA_NULL;
		mNumTestPatterns = 0;
	}

	for (unsigned int ndx = 0;  ndx < CIRCULAR_BUFFER_SIZE;  ndx++)
	{
		if (mAVHostBuffer [ndx].fVideoBuffer)
		{
			delete [] mAVHostBuffer [ndx].fVideoBuffer;
			mAVHostBuffer [ndx].fVideoBuffer = AJA_NULL;
		}
		if (mAVHostBuffer [ndx].fAudioBuffer)
		{
			delete [] mAVHostBuffer [ndx].fAudioBuffer;
			mAVHostBuffer [ndx].fAudioBuffer = AJA_NULL;
		}
	}	//	for each buffer in the ring

	if (!mDoMultiChannel && mDevice.IsOpen())
	{
		mDevice.SetEveryFrameServices (mSavedTaskMode);			//	Restore the previously saved service level
		mDevice.ReleaseStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));	//	Release the device
	}
}	//	destructor


void NTV2Player::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	if (mProducerThread)
		while (mProducerThread->Active ())
			AJATime::Sleep (10);

	if (mConsumerThread)
		while (mConsumerThread->Active ())
			AJATime::Sleep (10);

}	//	Quit


AJAStatus NTV2Player::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	if (!mDoMultiChannel)
	{
		if (!mDevice.AcquireStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
			return AJA_STATUS_BUSY;		//	Device is in use by another app -- fail

		mDevice.GetEveryFrameServices (mSavedTaskMode);		//	Save the current service level
	}

	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Set OEM service level

	mDeviceID = mDevice.GetDeviceID ();						//	Keep this ID handy -- it's used frequently

	if (::NTV2DeviceCanDoMultiFormat (mDeviceID) && mDoMultiChannel)
		mDevice.SetMultiFormatMode (true);
	else if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
		mDevice.SetMultiFormatMode (false);

	//	Beware -- some devices (e.g. Corvid1) can only output from FrameStore 2...
	if ((mOutputChannel == NTV2_CHANNEL1) && (!::NTV2DeviceCanDoFrameStore1Display (mDeviceID)))
		mOutputChannel = NTV2_CHANNEL2;
	if (UWord (mOutputChannel) >= ::NTV2DeviceGetNumFrameStores (mDeviceID))
	{
		cerr	<< "## ERROR:  Cannot use channel '" << mOutputChannel+1 << "' -- device only supports channel 1"
				<< (::NTV2DeviceGetNumFrameStores(mDeviceID) > 1  ?  string(" thru ") + string(1, char(::NTV2DeviceGetNumFrameStores(mDeviceID)+'0'))  :  "") << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	//	Set up the video and audio...
	status = SetUpVideo ();
	if (AJA_FAILURE (status))
		return status;

	status = SetUpAudio ();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the circular buffers, and the test pattern buffers...
	SetUpHostBuffers ();
	status = SetUpTestPatternVideoBuffers ();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the device signal routing, and playout AutoCirculate...
	RouteOutputSignal ();

	//	Lastly, prepare my AJATimeCodeBurn instance...
	const NTV2FormatDescriptor	fd (mVideoFormat, mPixelFormat, mVancMode);
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mPixelFormat), fd.numPixels, fd.numLines);

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Player::SetUpVideo ()
{
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		mDevice.GetVideoFormat (mVideoFormat, NTV2_CHANNEL1);

	if (!::NTV2DeviceCanDoVideoFormat (mDeviceID, mVideoFormat))
		{cerr << "## ERROR:  This device cannot handle '" << ::NTV2VideoFormatToString (mVideoFormat) << "'" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Configure the device to handle the requested video format...
	mDevice.SetVideoFormat (mVideoFormat, false, false, mOutputChannel);

	if (!::NTV2DeviceCanDo3GLevelConversion (mDeviceID) && mDoLevelConversion && ::IsVideoFormatA (mVideoFormat))
		mDoLevelConversion = false;
	if (mDoLevelConversion)
		mDevice.SetSDIOutLevelAtoLevelBConversion (mOutputChannel, mDoLevelConversion);

	//	Set the frame buffer pixel format for all the channels on the device.
	//	If the device doesn't support it, fall back to 8-bit YCbCr...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPixelFormat))
	{
		cerr	<< "## NOTE:  Device cannot handle '" << ::NTV2FrameBufferFormatString (mPixelFormat) << "' -- using '"
				<< ::NTV2FrameBufferFormatString (NTV2_FBF_8BIT_YCBCR) << "' instead" << endl;
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;
	}

	mDevice.SetFrameBufferFormat (mOutputChannel, mPixelFormat);
	if(mDeviceID == DEVICE_ID_KONAIP_1RX_1TX_2110 ||
		mDeviceID == DEVICE_ID_KONAIP_2110)
	{
		mDevice.SetReference(NTV2_REFERENCE_SFP1_PTP);
	}
	else
	{
		mDevice.SetReference (NTV2_REFERENCE_FREERUN);
	}
	mDevice.EnableChannel (mOutputChannel);

	if (mEnableVanc && !::IsRGBFormat (mPixelFormat) && NTV2_IS_HD_VIDEO_FORMAT (mVideoFormat))
	{
		//	Try enabling VANC...
		mDevice.SetEnableVANCData (true);		//	Enable VANC for non-SD formats, to pass thru captions, etc.
		if (::Is8BitFrameBufferFormat (mPixelFormat))
		{
			//	8-bit FBFs require VANC bit shift...
			mDevice.SetVANCShiftMode (mOutputChannel, NTV2_VANCDATA_8BITSHIFT_ENABLE);
			mDevice.SetVANCShiftMode (mOutputChannel, NTV2_VANCDATA_8BITSHIFT_ENABLE);
		}
	}	//	if HD video format
	else
		mDevice.SetEnableVANCData (false);	//	No VANC with RGB pixel formats (for now)

	//	Subscribe the output interrupt -- it's enabled by default...
	mDevice.SubscribeOutputVerticalEvent (mOutputChannel);
	if (OutputDestHasRP188BypassEnabled ())
		DisableRP188Bypass ();

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2Player::SetUpAudio ()
{
	const uint16_t	numberOfAudioChannels	(::NTV2DeviceGetMaxAudioChannels (mDeviceID));

	//	Use NTV2_AUDIOSYSTEM_1, unless the device has more than one audio system...
	if (::NTV2DeviceGetNumAudioSystems (mDeviceID) > 1)
		mAudioSystem = ::NTV2ChannelToAudioSystem (mOutputChannel);	//	...and base it on the channel
	//	However, there are a few older devices that have only 1 audio system, yet 2 frame stores (or must use channel 2 for playout)...
	if (!::NTV2DeviceCanDoFrameStore1Display (mDeviceID))
		mAudioSystem = NTV2_AUDIOSYSTEM_1;

	mDevice.SetNumberAudioChannels (numberOfAudioChannels, mAudioSystem);
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set the SDI output audio embedders to embed audio samples from the output of mAudioSystem...
	mDevice.SetSDIOutputAudioSystem (mOutputChannel, mAudioSystem);
	mDevice.SetSDIOutputDS2AudioSystem (mOutputChannel, mAudioSystem);

	//	If the last app using the device left it in end-to-end mode (input passthru),
	//	then loopback must be disabled, or else the output will contain whatever audio
	//	is present in whatever signal is feeding the device's SDI input...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio


void NTV2Player::SetUpHostBuffers ()
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	//	Calculate the size of the video buffer, which depends on video format, pixel format, and whether VANC is included or not...
	mDevice.GetVANCMode (mVancMode);
	mVideoBufferSize = GetVideoWriteSize (mVideoFormat, mPixelFormat, mVancMode);

	//	Calculate the size of the audio buffer, which mostly depends on the sample rate...
	NTV2AudioRate	audioRate	(NTV2_AUDIO_48K);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mAudioBufferSize = (audioRate == NTV2_AUDIO_96K) ? AUDIOBYTES_MAX_96K : AUDIOBYTES_MAX_48K;

	//	Allocate my buffers...
	for (size_t ndx(0);  ndx < CIRCULAR_BUFFER_SIZE;  ndx++)
	{
		mAVHostBuffer [ndx].fVideoBuffer		= reinterpret_cast <uint32_t *> (new uint8_t [mVideoBufferSize]);
		mAVHostBuffer [ndx].fVideoBufferSize	= mVideoBufferSize;
		mAVHostBuffer [ndx].fAudioBuffer		= mWithAudio ? reinterpret_cast<uint32_t*>(new uint8_t[mAudioBufferSize]) : AJA_NULL;
		mAVHostBuffer [ndx].fAudioBufferSize	= mWithAudio ? mAudioBufferSize : 0;

		::memset (mAVHostBuffer [ndx].fVideoBuffer, 0x00, mVideoBufferSize);
		::memset (mAVHostBuffer [ndx].fAudioBuffer, 0x00, mWithAudio ? mAudioBufferSize : 0);

		mAVCircularBuffer.Add (&mAVHostBuffer [ndx]);
	}	//	for each AV buffer in my circular buffer

}	//	SetUpHostBuffers


void NTV2Player::RouteOutputSignal ()
{
	const NTV2Standard	outputStandard	(::GetNTV2StandardFromVideoFormat (mVideoFormat));
	const UWord			numSDIOutputs	(::NTV2DeviceGetNumVideoOutputs (mDeviceID));
	bool				isRGB			(::IsRGBFormat (mPixelFormat));

	//	Since this function figures out which SDI spigots will be set up for output,
	//	it also sets the "mTCIndexes" member, which determines which timecodes will
	//	be transmitted (and on which SDI spigots)...
	mTCIndexes.clear();

	if (UWord(mOutputChannel) > ::NTV2DeviceGetNumCSCs(mDeviceID))	//	No CSC for this channel?
		isRGB = false;	//	Use YUV, since this device has no RGB conversion capability for this channel

	const NTV2OutputCrosspointID	cscVidOutXpt	(::GetCSCOutputXptFromChannel (mOutputChannel,  false/*isKey*/,  !isRGB/*isRGB*/));
	const NTV2OutputCrosspointID	fsVidOutXpt		(::GetFrameBufferOutputXptFromChannel (mOutputChannel,  isRGB/*isRGB*/,  false/*is425*/));
	if (isRGB)
		mDevice.Connect (::GetCSCInputXptFromChannel (mOutputChannel, false/*isKeyInput*/),  fsVidOutXpt);

	if (mDoMultiChannel)
	{
		//	Multiformat --- We may be sharing the device with other processes, so route only one SDI output...
		if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
			mDevice.SetSDITransmitEnable (mOutputChannel, true);

		mDevice.Connect (::GetSDIOutputInputXpt (mOutputChannel, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt);
		mTCIndexes.insert (::NTV2ChannelToTimecodeIndex (mOutputChannel, /*inEmbeddedLTC=*/mXmitLTC));
		//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically
		mDevice.SetSDIOutputStandard (mOutputChannel, outputStandard);
	}
	else
	{
		//	Not multiformat:  We own the whole device, so connect all possible SDI outputs...
		const UWord	numFrameStores(::NTV2DeviceGetNumFrameStores(mDeviceID));
		mDevice.ClearRouting();		//	Start with clean slate

		if (isRGB)
			mDevice.Connect (::GetCSCInputXptFromChannel (mOutputChannel, false/*isKeyInput*/),  fsVidOutXpt);

		for (NTV2Channel chan(NTV2_CHANNEL1);  ULWord(chan) < numSDIOutputs;  chan = NTV2Channel(chan+1))
		{
			if (chan != mOutputChannel  &&  chan < numFrameStores)
				mDevice.DisableChannel(chan);				//	Disable unused FrameStore
			if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
				mDevice.SetSDITransmitEnable (chan, true);	//	Make it an output

			mDevice.Connect (::GetSDIOutputInputXpt (chan, false/*isDS2*/),  isRGB ? cscVidOutXpt : fsVidOutXpt);
			mDevice.SetSDIOutputStandard (chan, outputStandard);
			mTCIndexes.insert (::NTV2ChannelToTimecodeIndex (chan, /*inEmbeddedLTC=*/mXmitLTC));	//	Add SDI spigot's TC index
			//	NOTE: No need to send VITC2 with VITC1 (for "i" formats) -- firmware does this automatically
		}	//	for each SDI output spigot

		//	And connect analog video output, if the device has one...
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
			mDevice.Connect (::GetOutputDestInputXpt (NTV2_OUTPUTDESTINATION_ANALOG),  isRGB ? cscVidOutXpt : fsVidOutXpt);

		//	And connect HDMI video output, if the device has one...
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1)
			|| ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v2)
            || ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v3)
            || ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v4))
                mDevice.Connect (::GetOutputDestInputXpt (NTV2_OUTPUTDESTINATION_HDMI),  isRGB ? cscVidOutXpt : fsVidOutXpt);
	}
	TCNOTE(mTCIndexes);

}	//	RouteOutputSignal


AJAStatus NTV2Player::Run ()
{
	//	Start my consumer and producer threads...
	StartConsumerThread ();
	StartProducerThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2Player::StartConsumerThread ()
{
	//	Create and start the playout thread...
	mConsumerThread = new AJAThread ();
	mConsumerThread->Attach (ConsumerThreadStatic, this);
	mConsumerThread->SetPriority (AJA_ThreadPriority_High);
	mConsumerThread->Start ();

}	//	StartConsumerThread


//	The playout thread function
void NTV2Player::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Player instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2Player *	pApp	(reinterpret_cast <NTV2Player *> (pContext));
	if (pApp)
		pApp->PlayFrames ();

}	//	ConsumerThreadStatic


void NTV2Player::PlayFrames (void)
{
	ULWord					acOpts		(AUTOCIRCULATE_WITH_RP188);	//	Add timecode
	const bool				isInterlace	(!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mVideoFormat));
	const bool				isPAL		(NTV2_IS_PAL_VIDEO_FORMAT(mVideoFormat));
	AUTOCIRCULATE_TRANSFER	xferInfo;

	PLNOTE("Started");
	if (mAncType != AJAAncillaryDataType_Unknown)
	{
		xferInfo.acANCBuffer.Allocate(NTV2_ANCSIZE_MAX);
		if (::NTV2DeviceCanDoCustomAnc(mDeviceID))
			acOpts |= AUTOCIRCULATE_WITH_ANC;
	}
	mDevice.AutoCirculateStop (mOutputChannel);
	mDevice.AutoCirculateInitForOutput (mOutputChannel, 7,	//	7 frame cushion sufficient for all devices & FBFs
										mWithAudio ? mAudioSystem : NTV2_AUDIOSYSTEM_INVALID,	//	Which audio system?
										acOpts);
	uint32_t	packetSize = 0;
	switch(mAncType)
	{
		case AJAAncillaryDataType_HDR_SDR:
		{
			AJAAncillaryData_HDR_SDR sdrPacket;
			sdrPacket.GenerateTransmitData((uint8_t*)xferInfo.acANCBuffer.GetHostPointer(), xferInfo.acANCBuffer.GetByteCount(), packetSize);
			break;
		}
		case AJAAncillaryDataType_HDR_HDR10:
		{
			AJAAncillaryData_HDR_HDR10 hdr10Packet;
			hdr10Packet.GenerateTransmitData((uint8_t*)xferInfo.acANCBuffer.GetHostPointer(), xferInfo.acANCBuffer.GetByteCount(), packetSize);
			break;
		}
		case AJAAncillaryDataType_HDR_HLG:
		{
			AJAAncillaryData_HDR_HLG hlgPacket;
			hlgPacket.GenerateTransmitData((uint8_t*)xferInfo.acANCBuffer.GetHostPointer(), xferInfo.acANCBuffer.GetByteCount(), packetSize);
			break;
		}
		default:	break;
	}

	mDevice.AutoCirculateStart(mOutputChannel);	//	Start it running

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS	outputStatus;
		mDevice.AutoCirculateGetStatus (mOutputChannel, outputStatus);

		//	Check if there's room for another frame on the card...
		if (outputStatus.GetNumAvailableOutputFrames() > 1)
		{
			//	Wait for the next frame to become ready to "consume"...
			AVDataBuffer *	playData	(mAVCircularBuffer.StartConsumeNextBuffer ());
			if (playData)
			{
				//	Include timecode in output signal...
				NTV2TimeCodes		tcMap;
				const NTV2_RP188	tcF1	(playData->fRP188Data);
				NTV2_RP188			tcF2	(tcF1);
				if (isInterlace)
				{	//	Set bit 27 of Hi word (PAL) or Lo word (NTSC)
					if (isPAL) tcF2.fHi |=  BIT(27);	else tcF2.fLo |=  BIT(27);
				}
				TCDBG("Playing " << tcF1);

				//	Put timecodes into xfer object...
				for (NTV2TCIndexesConstIter it(mTCIndexes.begin());  it != mTCIndexes.end();  ++it)
					tcMap[*it] = NTV2_IS_ATC_VITC2_TIMECODE_INDEX(*it) ? tcF2 : tcF1;
				xferInfo.SetOutputTimeCodes(tcMap);

				//	Transfer the timecode-burned frame to the device for playout...
				xferInfo.SetVideoBuffer (playData->fVideoBuffer, playData->fVideoBufferSize);
				xferInfo.SetAudioBuffer (mWithAudio ? playData->fAudioBuffer : AJA_NULL, mWithAudio ? playData->fAudioBufferSize : 0);
				mDevice.AutoCirculateTransfer (mOutputChannel, xferInfo);
				mAVCircularBuffer.EndConsumeNextBuffer();	//	Signal that the frame has been "consumed"
			}
		}
		else
			mDevice.WaitForOutputVerticalInterrupt(mOutputChannel);
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mOutputChannel);
	//delete [] fAncBuffer;
	PLNOTE("Ended");

}	//	PlayFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2Player::StartProducerThread ()
{
	//	Create and start the producer thread...
	mProducerThread = new AJAThread ();
	mProducerThread->Attach (ProducerThreadStatic, this);
	mProducerThread->SetPriority (AJA_ThreadPriority_High);
	mProducerThread->Start ();

}	//	StartProducerThread


void NTV2Player::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	NTV2Player *	pApp	(reinterpret_cast <NTV2Player *> (pContext));
	if (pApp)
		pApp->ProduceFrames ();

}	//	ProducerThreadStatic


AJAStatus NTV2Player::SetUpTestPatternVideoBuffers (void)
{
	NTV2TestPatternSelect	testPatternTypes []	=	{NTV2_TestPatt_ColorBars100,
													NTV2_TestPatt_ColorBars75,
													NTV2_TestPatt_Ramp,
													NTV2_TestPatt_MultiBurst,
													NTV2_TestPatt_LineSweep,
													NTV2_TestPatt_CheckField,
													NTV2_TestPatt_FlatField,
													NTV2_TestPatt_MultiPattern};

	mNumTestPatterns = sizeof (testPatternTypes) / sizeof (NTV2TestPatternSelect);
	mTestPatternVideoBuffers = new uint8_t * [mNumTestPatterns];
	::memset (mTestPatternVideoBuffers, 0, mNumTestPatterns * sizeof(uint8_t *));

	//	Set up one video buffer for each of the several predefined patterns...
	for (uint32_t testPatternIndex(0);  testPatternIndex < mNumTestPatterns;  testPatternIndex++)
	{
		//	Allocate the buffer memory...
		mTestPatternVideoBuffers [testPatternIndex] = new uint8_t[mVideoBufferSize];

		//	Use the test pattern generator to fill an NTV2TestPatternBuffer...
		NTV2TestPatternBuffer	testPatternBuffer;
		NTV2TestPatternGen		testPatternGen;
		NTV2FormatDescriptor	formatDesc	(mVideoFormat, mPixelFormat, mVancMode);

		if (!testPatternGen.DrawTestPattern (testPatternTypes[testPatternIndex],
											formatDesc.numPixels,
											formatDesc.numLines - formatDesc.firstActiveLine,
											mPixelFormat,
											testPatternBuffer))
		{
			cerr << "## ERROR:  DrawTestPattern failed, formatDesc: " << formatDesc << endl;
			return AJA_STATUS_FAIL;
		}

		const size_t	testPatternSize	(testPatternBuffer.size ());

		if (formatDesc.firstActiveLine)
		{
			//	Fill the VANC area with something valid -- otherwise the device won't emit a correctly-timed signal...
			unsigned	nVancLines	(formatDesc.firstActiveLine);
			uint8_t *	pVancLine	(mTestPatternVideoBuffers [testPatternIndex]);
			while (nVancLines--)
			{
				if (mPixelFormat == NTV2_FBF_10BIT_YCBCR)
				{
					::Make10BitBlackLine (reinterpret_cast<UWord*>(pVancLine), UWord(formatDesc.numPixels));
					::PackLine_16BitYUVto10BitYUV (reinterpret_cast <const UWord *> (pVancLine), reinterpret_cast <ULWord *> (pVancLine), formatDesc.numPixels);
				}
				else if (mPixelFormat == NTV2_FBF_8BIT_YCBCR)
					::Make8BitBlackLine (pVancLine, UWord(formatDesc.numPixels));
				else
				{
					cerr << "## ERROR:  Cannot initialize video buffer's VANC area" << endl;
					return AJA_STATUS_FAIL;
				}
				pVancLine += formatDesc.linePitch * 4;
			}	//	for each VANC line
		}	//	if has VANC area

		//	Copy the contents of the test pattern buffer into my video buffer...
		uint8_t *	pVideoBuffer	(mTestPatternVideoBuffers[testPatternIndex] + formatDesc.firstActiveLine * formatDesc.linePitch * 4);
		for (size_t ndx(0);  ndx < testPatternSize;  ndx++)
			pVideoBuffer[ndx] = testPatternBuffer[ndx];

	}	//	for each test pattern

	return AJA_STATUS_SUCCESS;

}	//	SetUpTestPatternVideoBuffers


static const double	gFrequencies []	=	{250.0, 500.0, 1000.0, 2000.0};
static const ULWord	gNumFrequencies		(sizeof (gFrequencies) / sizeof (double));
static const double	gAmplitudes []	=	{	0.10, 0.15,		0.20, 0.25,		0.30, 0.35,		0.40, 0.45,		0.50, 0.55,		0.60, 0.65,		0.70, 0.75,		0.80, 0.85,
											0.85, 0.80,		0.75, 0.70,		0.65, 0.60,		0.55, 0.50,		0.45, 0.40,		0.35, 0.30,		0.25, 0.20,		0.15, 0.10};


void NTV2Player::ProduceFrames (void)
{
	ULWord	frequencyIndex		(0);
	double	timeOfLastSwitch	(0.0);
	ULWord	testPatternIndex	(0);

	AJATimeBase	timeBase (CNTV2DemoCommon::GetAJAFrameRate (::GetNTV2FrameRateFromVideoFormat (mVideoFormat)));
	NTV2TestPatternNames tpNames(NTV2TestPatternGen::getTestPatternNames());

	PLNOTE("Started");
	while (!mGlobalQuit)
	{
		AVDataBuffer *	frameData	(mAVCircularBuffer.StartProduceNextBuffer ());

		//  If no frame is available, wait and try again
		if (!frameData)
		{
			AJATime::Sleep (10);
			continue;
		}

		//	Copy my pre-made test pattern into my video buffer...
		::memcpy (frameData->fVideoBuffer, mTestPatternVideoBuffers [testPatternIndex], mVideoBufferSize);

		const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
		const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat (ntv2FrameRate));
		const	CRP188			rp188Info		(mCurrentFrame++, 0, 0, 10, tcFormat);
		string					timeCodeString;

		rp188Info.GetRP188Reg (frameData->fRP188Data);
		rp188Info.GetRP188Str (timeCodeString);

		//	Burn the current timecode into the test pattern image that's now in my video buffer...
		mTCBurner.BurnTimeCode (reinterpret_cast <char *> (frameData->fVideoBuffer), timeCodeString.c_str (), 80);
		TCDBG("F" << DEC0N(mCurrentFrame-1,6) << ": " << NTV2_RP188(frameData->fRP188Data) << ": " << timeCodeString);

		//	Generate audio tone data...
		frameData->fAudioBufferSize		= mWithAudio ? AddTone(frameData->fAudioBuffer) : 0;

		//	Every few seconds, change the test pattern and tone frequency...
		const double	currentTime	(timeBase.FramesToSeconds(mCurrentFrame));
		if (currentTime > timeOfLastSwitch + 4.0)
		{
			frequencyIndex = (frequencyIndex + 1) % gNumFrequencies;
			testPatternIndex = (testPatternIndex + 1) % mNumTestPatterns;
			mToneFrequency = gFrequencies[frequencyIndex];
			timeOfLastSwitch = currentTime;
			PLINFO("F" << DEC0N(mCurrentFrame,6) << ": " << timeCodeString << ": tone=" << mToneFrequency
					<< "Hz, pattern='" << tpNames.at(testPatternIndex) << "'");
		}	//	if time to switch test pattern & tone frequency

		//	Signal that I'm done producing the buffer -- it's now available for playout...
		mAVCircularBuffer.EndProduceNextBuffer ();

	}	//	loop til mGlobalQuit goes true
	PLNOTE("Ended");

}	//	ProduceFrames


void NTV2Player::GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel)
{
	AUTOCIRCULATE_STATUS	status;
	mDevice.AutoCirculateGetStatus (mOutputChannel, status);
	outGoodFrames = status.acFramesProcessed;
	outDroppedFrames = status.acFramesDropped;
	outBufferLevel = status.acBufferLevel;
}


uint32_t NTV2Player::AddTone (ULWord * pInAudioBuffer)
{
	NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
	NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
	ULWord			numChannels	(0);

	mDevice.GetFrameRate (frameRate, mOutputChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);

	//	Set per-channel tone frequencies...
	double	pFrequencies [kNumAudioChannelsMax];
	pFrequencies [0] = (mToneFrequency / 2.0);
	for (ULWord chan (1);  chan < numChannels;  chan++)
		//	The 1.154782 value is the 16th root of 10, to ensure that if mToneFrequency is 2000,
		//	that the calculated frequency of audio channel 16 will be 20kHz...
		pFrequencies [chan] = pFrequencies [chan - 1] * 1.154782;

	//	Because audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will necessarily
	//	result in some frames having more audio samples than others. The GetAudioSamplesPerFrame function
	//	is used to calculate the correct sample count...
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));
	const double	sampleRateHertz	(audioRate == NTV2_AUDIO_96K ? 96000.0 : 48000.0);

	return ::AddAudioTone (	pInAudioBuffer,		//	audio buffer to fill
							mCurrentSample,		//	which sample for continuing the waveform
							numSamples,			//	number of samples to generate
							sampleRateHertz,	//	sample rate [Hz]
							gAmplitudes,		//	per-channel amplitudes
							pFrequencies,		//	per-channel tone frequencies [Hz]
							31,					//	bits per sample
							false,				//	don't byte swap
							numChannels);		//	number of audio channels to generate
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


bool NTV2Player::OutputDestHasRP188BypassEnabled (void)
{
	bool			result		(false);
	const ULWord	regNum		(GetRP188RegisterForOutput (mOutputDestination));
	ULWord			regValue	(0);

	//
	//	Bit 23 of the RP188 DBB register will be set if output timecode will be
	//	grabbed directly from an input (bypass source)...
	//
	if (regNum  &&  mDevice.ReadRegister(regNum, regValue)  &&  regValue & BIT(23))
		result = true;

	return result;

}	//	OutputDestHasRP188BypassEnabled


void NTV2Player::DisableRP188Bypass (void)
{
	const ULWord	regNum	(GetRP188RegisterForOutput (mOutputDestination));

	//
	//	Clear bit 23 of my output destination's RP188 DBB register...
	//
	if (regNum)
		mDevice.WriteRegister (regNum, 0, BIT(23), 23);

}	//	DisableRP188Bypass
