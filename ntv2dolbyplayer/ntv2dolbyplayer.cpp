/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolbyplayer.cpp
	@brief		Implementation of NTV2DolbyPlayer class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2dolbyplayer.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2debug.h"
#include "ntv2testpatterngen.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/timecode.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"

using namespace std;

//	Convenience macros for EZ logging:
#define	TCFAIL(_expr_)	AJA_sERROR  (AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCWARN(_expr_)	AJA_sWARNING(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCNOTE(_expr_)	AJA_sNOTICE	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCINFO(_expr_)	AJA_sINFO	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)
#define	TCDBG(_expr_)	AJA_sDEBUG	(AJA_DebugUnit_TimecodeGeneric, AJAFUNC << ": " << _expr_)

/**
	@brief	The maximum number of bytes of 48KHz audio that can be transferred for a single frame.
			Worst case, assuming 16 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			48,000 samples per second requires 3,204 samples x 4 bytes/sample x 16 = 205,056 bytes
			201K will suffice, with 768 bytes to spare
**/
static const uint32_t	AUDIOBYTES_MAX_48K	(201 * 1024);

/**
    @brief	The maximum number of bytes of 192KHz audio that can be transferred for a single frame.
            Worst case, assuming 8 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			96,000 samples per second requires 12,864 samples x 4 bytes/sample x 16 = 823,296 bytes
			824K will suffice
**/
static const uint32_t	AUDIOBYTES_MAX_192K	(824 * 1024);

/**
	@brief	Used when reserving the AJA device, this specifies the application signature.
**/
static const ULWord		kAppSignature	(NTV2_FOURCC('D','E','M','O'));



NTV2DolbyPlayer::NTV2DolbyPlayer (const string &				inDeviceSpecifier,
								  const bool					inWithAudio,
                                  const NTV2Channel             inChannel,
								  const NTV2FrameBufferFormat	inPixelFormat,
                                  const NTV2VideoFormat         inVideoFormat,
                                  const bool					inDoMultiChannel,
                                  AJAFileIO *                   inDolbyFile)

	:	mConsumerThread				(AJA_NULL),
		mProducerThread				(AJA_NULL),
		mCurrentFrame				(0),
		mCurrentSample				(0),
		mToneFrequency				(440.0),
		mDeviceSpecifier			(inDeviceSpecifier),
		mDeviceID					(DEVICE_ID_NOTFOUND),
        mOutputChannel              (inChannel),
		mVideoFormat				(inVideoFormat),
		mPixelFormat				(inPixelFormat),
		mSavedTaskMode				(NTV2_DISABLE_TASKS),
		mAudioSystem				(NTV2_AUDIOSYSTEM_1),
        mAudioRate                  (NTV2_AUDIO_192K),
		mWithAudio					(inWithAudio),
		mGlobalQuit					(false),
		mDoMultiChannel				(inDoMultiChannel),
		mVideoBufferSize			(0),
		mAudioBufferSize			(0),
		mTestPatternVideoBuffers	(AJA_NULL),
        mNumTestPatterns			(0),
		mBurstIndex					(0),
		mBurstSamples				(0),
		mBurstBuffer				(NULL),
		mBurstSize					(0),
		mBurstOffset				(0),
		mBurstMax					(0),
		mDolbyFile                  (inDolbyFile),
		mDolbyBuffer				(NULL),
		mDolbySize					(0),
		mDolbyBlocks				(0)
{
	::memset (mAVHostBuffer, 0, sizeof (mAVHostBuffer));
}


NTV2DolbyPlayer::~NTV2DolbyPlayer (void)
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

	if (mDolbyBuffer)
	{
		delete [] mDolbyBuffer;
		mDolbyBuffer = NULL;
	}

	if (mBurstBuffer)
	{
		delete [] mBurstBuffer;
		mBurstBuffer = NULL;
	}

	if (!mDoMultiChannel && mDevice.IsOpen())
	{
		mDevice.SetEveryFrameServices (mSavedTaskMode);			//	Restore the previously saved service level
		mDevice.ReleaseStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));	//	Release the device
	}
}	//	destructor


void NTV2DolbyPlayer::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	if (mProducerThread)
		while (mProducerThread->Active ())
			AJATime::Sleep (10);

	if (mConsumerThread)
		while (mConsumerThread->Active ())
			AJATime::Sleep (10);

    mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);
    mDevice.SetHDMIOutAudioRate(NTV2_AUDIO_48K);
	mDevice.SetHDMIOutAudioFormat(NTV2_AUDIO_FORMAT_LPCM);
}	//	Quit


AJAStatus NTV2DolbyPlayer::Init (void)
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
    const NTV2FormatDescriptor	fd (mVideoFormat, mPixelFormat, NTV2_VANCMODE_OFF);
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mPixelFormat), fd.numPixels, fd.numLines);

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2DolbyPlayer::SetUpVideo ()
{
	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		mDevice.GetVideoFormat (mVideoFormat, NTV2_CHANNEL1);

	if (!::NTV2DeviceCanDoVideoFormat (mDeviceID, mVideoFormat))
		{cerr << "## ERROR:  This device cannot handle '" << ::NTV2VideoFormatToString (mVideoFormat) << "'" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Configure the device to handle the requested video format...
	mDevice.SetVideoFormat (mVideoFormat, false, false, mOutputChannel);

	//	Set the frame buffer pixel format for all the channels on the device.
	//	If the device doesn't support it, fall back to 8-bit YCbCr...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPixelFormat))
	{
		cerr	<< "## NOTE:  Device cannot handle '" << ::NTV2FrameBufferFormatString (mPixelFormat) << "' -- using '"
				<< ::NTV2FrameBufferFormatString (NTV2_FBF_8BIT_YCBCR) << "' instead" << endl;
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;
	}

	mDevice.SetFrameBufferFormat (mOutputChannel, mPixelFormat);
	if (NTV2DeviceCanDo2110(mDeviceID))
	{
		mDevice.SetReference(NTV2_REFERENCE_SFP1_PTP);
	}
	else
	{
		mDevice.SetReference (NTV2_REFERENCE_FREERUN);
	}
	mDevice.EnableChannel (mOutputChannel);

	mDevice.SetEnableVANCData (false);

	//	Subscribe the output interrupt -- it's enabled by default...
	mDevice.SubscribeOutputVerticalEvent (mOutputChannel);

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2DolbyPlayer::SetUpAudio ()
{
    const uint16_t	numberOfAudioChannels	(8);

	//	Use NTV2_AUDIOSYSTEM_1, unless the device has more than one audio system...
	if (::NTV2DeviceGetNumAudioSystems (mDeviceID) > 1)
		mAudioSystem = ::NTV2ChannelToAudioSystem (mOutputChannel);	//	...and base it on the channel

	mDevice.SetNumberAudioChannels (numberOfAudioChannels, mAudioSystem);
    mDevice.SetAudioRate (mAudioRate, mAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	//	Set the SDI output audio embedders to embed audio samples from the output of mAudioSystem...
    mDevice.SetHDMIOutAudioSource2Channel (NTV2_AudioChannel1_2, mAudioSystem);

    mDevice.SetHDMIOutAudioRate(mAudioRate);

	mDevice.SetHDMIOutAudioFormat((mDolbyFile != NULL)? NTV2_AUDIO_FORMAT_DOLBY : NTV2_AUDIO_FORMAT_LPCM);

	//	If the last app using the device left it in end-to-end mode (input passthru),
	//	then loopback must be disabled, or else the output will contain whatever audio
	//	is present in whatever signal is feeding the device's SDI input...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio


void NTV2DolbyPlayer::SetUpHostBuffers ()
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	//	Calculate the size of the video buffer, which depends on video format, pixel format, and whether VANC is included or not...
	mVideoBufferSize = GetVideoWriteSize (mVideoFormat, mPixelFormat);

	//	Calculate the size of the audio buffer, which mostly depends on the sample rate...
    mAudioBufferSize = (mAudioRate == NTV2_AUDIO_192K) ? AUDIOBYTES_MAX_192K : AUDIOBYTES_MAX_48K;

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

    if (mDolbyFile != NULL)
    {
		//  Initialize IEC61937 burst size (32 milliseconds) for HDMI 192 kHz sample rate
		mBurstSamples = 6144;  //  192000 * 0.032 samples
		mBurstMax = mBurstSamples * 2;
		mBurstBuffer = new uint16_t [mBurstMax];
		mDolbyBuffer = new uint16_t [mBurstMax];
    }

}	//	SetUpHostBuffers


void NTV2DolbyPlayer::RouteOutputSignal ()
{
    bool				isRGB			(::IsRGBFormat (mPixelFormat));

    const NTV2OutputCrosspointID	fsVidOutXpt		(::GetFrameBufferOutputXptFromChannel (mOutputChannel,  isRGB/*isRGB*/,  false/*is425*/));

    mDevice.ClearRouting();		//	Start with clean slate

    //	And connect HDMI video output
    mDevice.Connect (::GetOutputDestInputXpt (NTV2_OUTPUTDESTINATION_HDMI),  fsVidOutXpt);

}	//	RouteOutputSignal


AJAStatus NTV2DolbyPlayer::Run ()
{
	//	Start my consumer and producer threads...
	StartConsumerThread ();
	StartProducerThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2DolbyPlayer::StartConsumerThread ()
{
	//	Create and start the playout thread...
	mConsumerThread = new AJAThread ();
	mConsumerThread->Attach (ConsumerThreadStatic, this);
	mConsumerThread->SetPriority (AJA_ThreadPriority_High);
	mConsumerThread->Start ();

}	//	StartConsumerThread


//	The playout thread function
void NTV2DolbyPlayer::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2DolbyPlayer instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2DolbyPlayer *	pApp	(reinterpret_cast <NTV2DolbyPlayer *> (pContext));
	if (pApp)
		pApp->PlayFrames ();

}	//	ConsumerThreadStatic


void NTV2DolbyPlayer::PlayFrames (void)
{
	ULWord					acOpts		(AUTOCIRCULATE_WITH_RP188);	//	Add timecode
	const bool				isInterlace	(!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mVideoFormat));
	const bool				isPAL		(NTV2_IS_PAL_VIDEO_FORMAT(mVideoFormat));
	AUTOCIRCULATE_TRANSFER	xferInfo;

	PLNOTE("Thread started");
	mDevice.AutoCirculateStop (mOutputChannel);
	mDevice.AutoCirculateInitForOutput (mOutputChannel, 7,	//	7 frame cushion sufficient for all devices & FBFs
										mWithAudio ? mAudioSystem : NTV2_AUDIOSYSTEM_INVALID,	//	Which audio system?
										acOpts);

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
	PLNOTE("Thread completed, will exit");

}	//	PlayFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2DolbyPlayer::StartProducerThread ()
{
	//	Create and start the producer thread...
	mProducerThread = new AJAThread ();
	mProducerThread->Attach (ProducerThreadStatic, this);
	mProducerThread->SetPriority (AJA_ThreadPriority_High);
	mProducerThread->Start ();

}	//	StartProducerThread


void NTV2DolbyPlayer::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	NTV2DolbyPlayer *	pApp	(reinterpret_cast <NTV2DolbyPlayer *> (pContext));
	if (pApp)
		pApp->ProduceFrames ();

}	//	ProducerThreadStatic


AJAStatus NTV2DolbyPlayer::SetUpTestPatternVideoBuffers (void)
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
		NTV2TestPatternGen		testPatternGen;
        NTV2FormatDescriptor	formatDesc	(mVideoFormat, mPixelFormat, NTV2_VANCMODE_OFF);
        NTV2_POINTER			vidBuffer	(mTestPatternVideoBuffers [testPatternIndex], mVideoBufferSize);

		if (!testPatternGen.DrawTestPattern (testPatternTypes[testPatternIndex],  formatDesc, vidBuffer))
		{
			cerr << "## ERROR:  DrawTestPattern failed, formatDesc: " << formatDesc << endl;
			return AJA_STATUS_FAIL;
		}

	}	//	for each test pattern

	return AJA_STATUS_SUCCESS;

}	//	SetUpTestPatternVideoBuffers


static const double	gFrequencies []	=	{250.0, 500.0, 1000.0, 2000.0};
static const ULWord	gNumFrequencies		(sizeof (gFrequencies) / sizeof (double));
static const double	gAmplitudes []	=	{	0.10, 0.15,		0.20, 0.25,		0.30, 0.35,		0.40, 0.45,		0.50, 0.55,		0.60, 0.65,		0.70, 0.75,		0.80, 0.85,
											0.85, 0.80,		0.75, 0.70,		0.65, 0.60,		0.55, 0.50,		0.45, 0.40,		0.35, 0.30,		0.25, 0.20,		0.15, 0.10};


void NTV2DolbyPlayer::ProduceFrames (void)
{
	ULWord	frequencyIndex		(0);
	double	timeOfLastSwitch	(0.0);
	ULWord	testPatternIndex	(0);

	AJATimeBase	timeBase (CNTV2DemoCommon::GetAJAFrameRate (::GetNTV2FrameRateFromVideoFormat (mVideoFormat)));
	NTV2TestPatternNames tpNames(NTV2TestPatternGen::getTestPatternNames());

	PLNOTE("Thread started");
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
        if (mDolbyFile != NULL)
            frameData->fAudioBufferSize		= mWithAudio ? AddDolby(frameData->fAudioBuffer) : 0;
        else
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
	PLNOTE("Thread completed, will exit");

}	//	ProduceFrames


void NTV2DolbyPlayer::GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel)
{
	AUTOCIRCULATE_STATUS	status;
	mDevice.AutoCirculateGetStatus (mOutputChannel, status);
	outGoodFrames = status.acFramesProcessed;
	outDroppedFrames = status.acFramesDropped;
	outBufferLevel = status.acBufferLevel;
}


uint32_t NTV2DolbyPlayer::AddTone (ULWord * pInAudioBuffer)
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
    const double	sampleRateHertz	(audioRate == NTV2_AUDIO_192K ? 192000.0 : 48000.0);

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

#ifdef DOLBY_FULL_PARSER

uint32_t NTV2DolbyPlayer::AddDolby (ULWord * pInAudioBuffer)
{
    NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
    NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
    ULWord			numChannels	(0);
    ULWord          sampleOffset(0);
	ULWord			sampleCount	(0);
	NTV2DolbyBSI	bsi;

    mDevice.GetFrameRate (frameRate, mOutputChannel);
    mDevice.GetAudioRate (audioRate, mAudioSystem);
    mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);
    const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));

	if ((mDolbyFile == NULL) || (mDolbyBuffer == NULL))
		goto silence;

    //  Generate the samples for this frame
    while (sampleOffset < numSamples)
    {
		if ((mBurstSize != 0) && (mBurstIndex < mBurstSamples))
		{
			if(mBurstOffset < mBurstSize)
			{
				uint32_t data0 = 0;
				uint32_t data1 = 0;
				if (mBurstIndex == 0)
				{
					//  Add IEC61937 burst preamble
					data0 = 0xf872;             //  Sync stuff
					data1 = 0x4e1f;
				}
				else if (mBurstIndex == 1)
				{
					//  Add more IEC61937 burst preamble
					data0 = ((bsi.bsmod & 0x7) << 8) | 0x0015;	//  This is Dolby
					data1 = mBurstSize * 2;						//  Data size in bytes
				}
				else
				{
					//  Add sync frame data
					data0 = (uint32_t)(mBurstBuffer[mBurstOffset]);
					data0 = ((data0 & 0x00ff) << 8) | ((data0 & 0xff00) >> 8);
					mBurstOffset++;
					data1 = (uint32_t)(mBurstBuffer[mBurstOffset]);
					data1 = ((data1 & 0x00ff) << 8) | ((data1 & 0xff00) >> 8);
					mBurstOffset++;
				}

				//  Write data into 16 msbs of all audio channel pairs
				data0 <<= 16;
				data1 <<= 16;
				for (ULWord i = 0; i < numChannels; i += 2)
				{
					pInAudioBuffer[sampleOffset * numChannels + i] = data0;
					pInAudioBuffer[sampleOffset * numChannels + i + 1] = data1;
				}
			}
			else
			{
				//  Pad samples out to burst size
				for (ULWord i = 0; i < numChannels; i++)
				{
					pInAudioBuffer[sampleOffset * numChannels + i] = 0;
				}
			}
			sampleOffset++;
			mBurstIndex++;
		}
		else
		{
			ULWord dolbyOffset = 0;
			ULWord burstOffset = 0;
			ULWord numBlocks = 0;

			mBurstIndex = 0;
			mBurstOffset = 0;

			if (mDolbySize == 0)
			{
				// Find first Dolby Digital Plus burst frame
				while (true)
				{
					if (!GetDolbyFrame(&mDolbyBuffer[0], sampleCount))
					{
						cerr << "## ERROR:  Dolby frame not found" << endl;
						mDolbyFile = NULL;
						goto silence;
					}

					if (!ParseBSI(&mDolbyBuffer[1], sampleCount - 1, &bsi))
						continue;

					if ((bsi.strmtyp == 0) &&
						(bsi.substreamid == 0) &&
						(bsi.bsid == 16) &&
						((bsi.numblkscod == 3) || (bsi.convsync == 1)))
						break;
				}

				mDolbySize = sampleCount;
				switch (bsi.numblkscod)
				{
				case 0: mDolbyBlocks = 1; break;
				case 1: mDolbyBlocks = 2; break;
				case 2: mDolbyBlocks = 3; break;
				case 3: mDolbyBlocks = 6; break;
				default: goto silence;
				}
			}

			while (numBlocks <= 6)
			{
				// Copy the Dolby frame into the burst buffer
				while (dolbyOffset < mDolbySize)
				{
					// Check for burst size overrun
					if (burstOffset >= mBurstMax)
					{
						cerr << "## ERROR:  Dolby burst too large" << endl;
						mDolbyFile = NULL;
						goto silence;
					}

					// Copy sample
					mBurstBuffer[burstOffset] = mDolbyBuffer[dolbyOffset];
					burstOffset++;
					dolbyOffset++;
				}

				// Get the next Dolby frame
				if (!GetDolbyFrame(&mDolbyBuffer[0], sampleCount))
				{
					// try to loop
					if (!GetDolbyFrame(&mDolbyBuffer[0], sampleCount))
					{
						cerr << "## ERROR:  Dolby frame not found" << endl;
						mDolbyFile = NULL;
						goto silence;
					}
				}

				// Parse the Dolby bitstream header
				if (!ParseBSI(&mDolbyBuffer[1], sampleCount - 1, &bsi))
					continue;

				// Only Dolby Digital Plus
				if (bsi.bsid != 16)
				{
					cerr << "## ERROR:  Dolby frame bad bsid = " << bsi.bsid << endl;
					continue;
				}

				mDolbySize = sampleCount;
				dolbyOffset = 0;

				// Increment block count on first substream
				if ((bsi.strmtyp == 0) && (bsi.substreamid == 0))
				{
					// increment block count
					numBlocks += mDolbyBlocks;

					switch (bsi.numblkscod)
					{
					case 0: mDolbyBlocks = 1; break;
					case 1: mDolbyBlocks = 2; break;
					case 2: mDolbyBlocks = 3; break;
					case 3: mDolbyBlocks = 6; break;
					default:
						cerr << "## ERROR:  Dolby frame bad numblkscod = " << bsi.numblkscod << endl;
						goto silence;
					}
				}

				//	Are we done?
				if (numBlocks >= 6)
				{
					// First frame of new burst must have convsync == 1
					if ((bsi.numblkscod != 3) &&
						(bsi.convsync != 1))
					{
						cerr << "## ERROR:  Dolby frame unexpected convsync = " << bsi.convsync << endl;
						mDolbySize = 0;
						mDolbyBlocks = 0;
					}

					//	Keep the burst size
					mBurstSize = burstOffset;
					break;
				}
			}
		}
	}

    return numSamples * numChannels * 4;

silence:
    //  Output silence when done with file
    memset(&pInAudioBuffer[sampleOffset * numChannels], 0, (numSamples - sampleOffset) * numChannels * 4);
    return numSamples * numChannels * 4;
}


bool NTV2DolbyPlayer::GetDolbyFrame (uint16_t * pInDolbyBuffer, uint32_t & numSamples)
{
	uint32_t bytes;
	bool done = false;

	while (!done)
	{
		bytes = mDolbyFile->Read((uint8_t*)(&pInDolbyBuffer[0]), 2);
		if (bytes != 2)
		{
			//  Reset file
			mDolbyFile->Seek(0, eAJASeekSet);
			return false;
		}

		//  Check sync word
		if ((mDolbyBuffer[0] == 0x7705) ||
			(mDolbyBuffer[0] == 0x770b))
			done = true;
	}

	//  Read more of the sync frame header
	bytes = mDolbyFile->Read((uint8_t*)(&pInDolbyBuffer[1]), 4);
	if (bytes != 4)
		return false;

	//  Get frame size - 16 bit words plus sync word
	uint32_t size = (uint32_t)mDolbyBuffer[1];
	size = (((size & 0x00ff) << 8) | ((size & 0xff00) >> 8));
	size = (size & 0x7ff) + 1;

	//  Read the rest of the sync frame
	uint32_t len = (size - 3) * 2;
	bytes = mDolbyFile->Read((uint8_t*)(&pInDolbyBuffer[3]), len);
	if (bytes != len)
		return false;

	numSamples = size;

	return true;
}


bool NTV2DolbyPlayer::ParseBSI(uint16_t * pInDolbyBuffer, uint32_t numSamples, NTV2DolbyBSI * pBsi)
{
	if ((pInDolbyBuffer == NULL) || (pBsi == NULL))
		return false;

	memset(pBsi, 0, sizeof(NTV2DolbyBSI));

	SetBitBuffer((uint8_t*)pInDolbyBuffer, numSamples * 2);

	if (!GetBits(pBsi->strmtyp, 2)) return false;
	if (!GetBits(pBsi->substreamid, 3)) return false;
	if (!GetBits(pBsi->frmsiz, 11)) return false;
	if (!GetBits(pBsi->fscod, 2)) return false;
	if (!GetBits(pBsi->numblkscod, 2)) return false;
	if (!GetBits(pBsi->acmod, 3)) return false;
	if (!GetBits(pBsi->lfeon, 1)) return false;
	if (!GetBits(pBsi->bsid, 5)) return false;
	if (!GetBits(pBsi->dialnorm, 5)) return false;
	if (!GetBits(pBsi->compre, 1)) return false;
	if (pBsi->compre)
	{
		if (!GetBits(pBsi->compr, 8)) return false;
	}
	if (pBsi->acmod == 0x0) /* if 1+1 mode (dual mono, so some items need a second value) */
	{
		if (!GetBits(pBsi->dialnorm2, 5)) return false;
		if (!GetBits(pBsi->compr2e, 1)) return false;
		if (pBsi->compr2e)
		{
			if (!GetBits(pBsi->compr2, 8)) return false;
		}
	}
	if (pBsi->strmtyp == 0x1) /* if dependent stream */
	{
		if (!GetBits(pBsi->chanmape, 1)) return false;
		if (pBsi->chanmape)
		{
			if (!GetBits(pBsi->chanmap, 16)) return false;
		}
	}
	if (!GetBits(pBsi->mixmdate, 1)) return false;
	if (pBsi->mixmdate) /* mixing metadata */
	{
		if (pBsi->acmod > 0x2) /* if more than 2 channels */
		{
			if (!GetBits(pBsi->dmixmod, 2)) return false;
		}
		if ((pBsi->acmod & 0x1) && (pBsi->acmod > 0x2)) /* if three front channels exist */
		{
			if (!GetBits(pBsi->ltrtcmixlev, 3)) return false;
			if (!GetBits(pBsi->lorocmixlev, 3)) return false;
		}
		if (pBsi->acmod & 0x4) /* if a surround channel exists */
		{
			if (!GetBits(pBsi->ltrtsurmixlev, 3)) return false;
			if (!GetBits(pBsi->lorosurmixlev, 3)) return false;
		}
		if (pBsi->lfeon) /* if the LFE channel exists */
		{
			if (!GetBits(pBsi->lfemixlevcode, 1)) return false;
			if (pBsi->lfemixlevcode)
			{
						 if (!GetBits(pBsi->lfemixlevcod, 5)) return false;
			}
		}
		if (pBsi->strmtyp == 0x0) /* if independent stream */
		{
			if (!GetBits(pBsi->pgmscle, 1)) return false;
			if (pBsi->pgmscle)
			{
				if (!GetBits(pBsi->pgmscl, 6)) return false;
			}
			if (pBsi->acmod == 0x0) /* if 1+1 mode (dual mono, so some items need a second value) */
			{
				if (!GetBits(pBsi->pgmscl2e, 1)) return false;
				if (pBsi->pgmscl2e)
				{
					if (!GetBits(pBsi->pgmscl2, 6)) return false;
				}
			}
			if (!GetBits(pBsi->extpgmscle, 1)) return false;
			if (pBsi->extpgmscle)
			{
				if (!GetBits(pBsi->extpgmscl, 6)) return false;
			}
			if (!GetBits(pBsi->mixdef, 2)) return false;
			if (pBsi->mixdef == 0x1) /* mixing option 2 */
			{
				if (!GetBits(pBsi->premixcmpsel, 1)) return false;
				if (!GetBits(pBsi->drcsrc, 1)) return false;
				if (!GetBits(pBsi->premixcmpscl, 3)) return false;
			}
			else if (pBsi->mixdef == 0x2) /* mixing option 3 */
			{
				if (!GetBits(pBsi->mixdata, 12)) return false;
			}
			else if (pBsi->mixdef == 0x3) /* mixing option 4 */
			{
				if (!GetBits(pBsi->mixdeflen, 5)) return false;
				if (!GetBits(pBsi->mixdata2e, 1)) return false;
				if (pBsi->mixdata2e)
				{
					if (!GetBits(pBsi->premixcmpsel, 1)) return false;
					if (!GetBits(pBsi->drcsrc, 1)) return false;
					if (!GetBits(pBsi->premixcmpscl, 3)) return false;
					if (!GetBits(pBsi->extpgmlscle, 1)) return false;
					if (pBsi->extpgmlscle)
					{
						if (!GetBits(pBsi->extpgmlscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmcscle, 1)) return false;
					if (pBsi->extpgmcscle)
					{
						if (!GetBits(pBsi->extpgmcscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmrscle, 1)) return false;
					if (pBsi->extpgmrscle)
					{
						if (!GetBits(pBsi->extpgmrscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmlsscle, 1)) return false;
					if (pBsi->extpgmlsscle)
					{
						if (!GetBits(pBsi->extpgmlsscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmrsscle, 1)) return false;
					if (pBsi->extpgmrsscle)
					{
						if (!GetBits(pBsi->extpgmrsscl, 4)) return false;
					}
					if (!GetBits(pBsi->extpgmlfescle, 1)) return false;
					if (pBsi->extpgmlfescle)
					{
						if (!GetBits(pBsi->extpgmlfescl, 4)) return false;
					}
					if (!GetBits(pBsi->dmixscle, 1)) return false;
					if (pBsi->dmixscle)
					{
						if (!GetBits(pBsi->dmixscl, 4)) return false;
					}
					if (!GetBits(pBsi->addche, 1)) return false;
					if (pBsi->addche)
					{
						if (!GetBits(pBsi->extpgmaux1scle, 1)) return false;
						if (pBsi->extpgmaux1scle)
						{
							if (!GetBits(pBsi->extpgmaux1scl, 4)) return false;
						}
						if (!GetBits(pBsi->extpgmaux2scle, 1)) return false;
						if (pBsi->extpgmaux2scle)
						{
							if (!GetBits(pBsi->extpgmaux2scl, 4)) return false;
						}
					}
				}
				if (!GetBits(pBsi->mixdata3e, 1)) return false;
				if (pBsi->mixdata3e)
				{
					if (!GetBits(pBsi->spchdat, 5)) return false;
					if (!GetBits(pBsi->addspchdate, 1)) return false;
					if (pBsi->addspchdate)
					{
						if (!GetBits(pBsi->spchdat1, 5)) return false;
						if (!GetBits(pBsi->spchan1att, 2)) return false;
						if (!GetBits(pBsi->addspchdat1e, 1)) return false;
						if (pBsi->addspdat1e)
						{
							if (!GetBits(pBsi->spchdat2, 5)) return false;
							if (!GetBits(pBsi->spchan2att, 3)) return false;
						}
					}
				}
				{
					uint32_t data;
					uint32_t size = 8 * (pBsi->mixdeflen + 2);
					size = (size + 7) / 8 * 8;
					uint32_t index = 0;
					while (size > 0)
					{
						if (!GetBits(data, (size > 8)? 8 : size)) return false;
						pBsi->mixdatabuffer[index++] = (uint8_t)data;
						size = size - 8;
					}
				}
			}
			if (pBsi->acmod < 0x2) /* if mono or dual mono source */
			{
				if (!GetBits(pBsi->paninfoe, 1)) return false;
				if (pBsi->paninfoe)
				{
					if (!GetBits(pBsi->panmean, 8)) return false;
					if (!GetBits(pBsi->paninfo, 6)) return false;
				}
				if (pBsi->acmod == 0x0) /* if 1+1 mode (dual mono - some items need a second value) */
				{
					if (!GetBits(pBsi->paninfo2e, 1)) return false;
					if (pBsi->paninfo2e)
					{
						if (!GetBits(pBsi->panmean2, 8)) return false;
						if (!GetBits(pBsi->paninfo2, 6)) return false;
					}
				}
			}
			if (!GetBits(pBsi->frmmixcfginfoe, 1)) return false;
			if (pBsi->frmmixcfginfoe) /* mixing configuration information */
			{
				if (pBsi->numblkscod == 0x0)
				{
					if (!GetBits(pBsi->blkmixcfginfo[0], 5)) return false;
				}
				else
				{
					uint32_t blk;
					uint32_t numblk;
					if (pBsi->numblkscod == 0x1)
						numblk = 2;
					else if (pBsi->numblkscod == 0x2)
						numblk = 3;
					else if (pBsi->numblkscod == 0x3)
						numblk = 6;
					else
						return false;
					for(blk = 0; blk < numblk; blk++)
					{
						if (!GetBits(pBsi->blkmixcfginfoe, 1)) return false;
						if (pBsi->blkmixcfginfoe)
						{
							if (!GetBits(pBsi->blkmixcfginfo[blk], 5)) return false;
						}
					}
				}
			}
		}
	}
	if (!GetBits(pBsi->infomdate, 1)) return false;
	if (pBsi->infomdate) /* informational metadata */
	{
		if (!GetBits(pBsi->bsmod, 3)) return false;
		if (!GetBits(pBsi->copyrightb, 1)) return false;
		if (!GetBits(pBsi->origbs, 1)) return false;
		if (pBsi->acmod == 0x2) /* if in 2/0 mode */
		{
			if (!GetBits(pBsi->dsurmod, 2)) return false;
			if (!GetBits(pBsi->dheadphonmod, 2)) return false;
		}
		if (pBsi->acmod >= 0x6) /* if both surround channels exist */
		{
			if (!GetBits(pBsi->dsurexmod, 2)) return false;
		}
		if (!GetBits(pBsi->audprodie, 1)) return false;
		if (pBsi->audprodie)
		{
			if (!GetBits(pBsi->mixlevel, 5)) return false;
			if (!GetBits(pBsi->roomtyp, 2)) return false;
			if (!GetBits(pBsi->adconvtyp, 1)) return false;
		}
		if (pBsi->acmod == 0x0) /* if 1+1 mode (dual mono, so some items need a second value) */
		{
			if (!GetBits(pBsi->audprodi2e, 1)) return false;
			if (pBsi->audprodi2e)
			{
				if (!GetBits(pBsi->mixlevel2, 5)) return false;
				if (!GetBits(pBsi->roomtyp2, 2)) return false;
				if (!GetBits(pBsi->adconvtyp2, 1)) return false;
			}
		}
		if (pBsi->fscod < 0x3) /* if not half sample rate */
		{
			if (!GetBits(pBsi->sourcefscod, 1)) return false;
		}
	}
	if ((pBsi->strmtyp == 0x0) && (pBsi->numblkscod != 0x3))
	{
		if (!GetBits(pBsi->convsync, 1)) return false;
	}
	if (pBsi->strmtyp == 0x2) /* if bit stream converted from AC-3 */
	{
		if (pBsi->numblkscod == 0x3) /* 6 blocks per syncframe */
		{
			pBsi->blkid = 1;
		}
		else
		{
			if (!GetBits(pBsi->blkid, 1)) return false;
		}
		if (pBsi->blkid)
		{
			if (!GetBits(pBsi->frmsizecod, 6)) return false;
		}
	}
	if (!GetBits(pBsi->addbsie, 1)) return false;
	if (pBsi->addbsie)
	{
		if (!GetBits(pBsi->addbsil, 6)) return false;
		{
			uint32_t data;
			uint32_t size = 8 * (pBsi->addbsil + 1);
			uint32_t index = 0;
			while (size > 0)
			{
				if (!GetBits(data, 8)) return false;
				pBsi->addbsibuffer[index++] = (uint8_t)data;
				size = size - 8;
			}
		}
	}

	return true;
}

void NTV2DolbyPlayer::SetBitBuffer(uint8_t * pBuffer, uint32_t size)
{
	mBitBuffer = pBuffer;
	mBitSize = size;
	mBitIndex = 8;
}

bool NTV2DolbyPlayer::GetBits(uint32_t & data, uint32_t bits)
{
	uint32_t cb;
	static uint8_t bitMask[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

	data = 0;
	while (bits > 0)
	{
		if (mBitSize == 0)
			return false;

		if (mBitIndex == 0)
		{
			mBitBuffer++;
			mBitSize--;
			mBitIndex = 8;
		}

		cb = bits;
		if (cb > mBitIndex)
			cb = mBitIndex;
		bits -= cb;
		mBitIndex -= cb;
		data |= ((*mBitBuffer >> mBitIndex) & bitMask[cb]) << bits;
	}

	return true;
}

#else

uint32_t NTV2DolbyPlayer::AddDolby (ULWord * pInAudioBuffer)
{
	NTV2FrameRate	frameRate	(NTV2_FRAMERATE_INVALID);
	NTV2AudioRate	audioRate	(NTV2_AUDIO_RATE_INVALID);
	ULWord			numChannels	(0);
	ULWord          sampleOffset(0);

	mDevice.GetFrameRate (frameRate, mOutputChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));

	if ((mDolbyFile == NULL) || (mDolbyBuffer == NULL))
		goto silence;

	//  Generate the samples for this frame
	while (sampleOffset < numSamples)
	{
		//  Time for a new IEC61937 burst
		if (mBurstIndex >= mBurstSamples)
			mBurstIndex = 0;

		//  Get a new Dolby Digital Plus sync frame
		if (mBurstIndex == 0)
		{
			//  Read the sync word (all big endian)
			uint32_t bytes = mDolbyFile->Read((uint8_t*)(&mDolbyBuffer[0]), 2);
			if (bytes != 2)
			{
				//  Try to loop
				mDolbyFile->Seek(0, eAJASeekSet);
				bytes = mDolbyFile->Read((uint8_t*)(&mDolbyBuffer[0]), 2);
				if (bytes != 2)
					goto silence;
			}

			//  Check sync word
			if ((mDolbyBuffer[0] != 0x7705) &&
				(mDolbyBuffer[0] != 0x770b))
				goto silence;

			//  Read more of the sync frame header
			bytes = mDolbyFile->Read((uint8_t*)(&mDolbyBuffer[1]), 4);
			if (bytes != 4)
				goto silence;

			//  Get frame size - 16 bit words plus sync word
			uint32_t size = (uint32_t)mDolbyBuffer[1];
			size = (((size & 0x00ff) << 8) | ((size & 0xff00) >> 8)) + 1;

			//  Read the rest of the sync frame
			uint32_t len = (size - 3) * 2;
			bytes = mDolbyFile->Read((uint8_t*)(&mDolbyBuffer[3]), len);
			if (bytes != len)
				goto silence;

			//  Good frame
			mBurstOffset = 0;
			mBurstSize = size;
		}

		//  Add the Dolby data to the audio stream
		if (mBurstOffset < mBurstSize)
		{
			uint32_t data0 = 0;
			uint32_t data1 = 0;
			if (mBurstIndex == 0)
			{
				//  Add IEC61937 burst preamble
				data0 = 0xf872;             //  Sync stuff
				data1 = 0x4e1f;
			}
			else if (mBurstIndex == 1)
			{
				//  Add more IEC61937 burst preamble
				data0 = 0x0015;             //  This is Dolby
				data1 = mBurstSize * 2;     //  Data size in bytes
			}
			else
			{
				//  Add sync frame data
				data0 = (uint32_t)(mDolbyBuffer[mBurstOffset]);
				data0 = ((data0 & 0x00ff) << 8) | ((data0 & 0xff00) >> 8);
				mBurstOffset++;
				data1 = (uint32_t)(mDolbyBuffer[mBurstOffset]);
				data1 = ((data1 & 0x00ff) << 8) | ((data1 & 0xff00) >> 8);
				mBurstOffset++;
			}

			//  Write data into 16 msbs of all audio channel pairs
			data0 <<= 16;
			data1 <<= 16;
			for (ULWord i = 0; i < numChannels; i += 2)
			{
				pInAudioBuffer[sampleOffset * numChannels + i] = data0;
				pInAudioBuffer[sampleOffset * numChannels + i + 1] = data1;
			}
		}
		else
		{
			//  Pad samples out to burst size
			for (ULWord i = 0; i < numChannels; i++)
			{
				pInAudioBuffer[sampleOffset * numChannels + i] = 0;
			}
		}

		sampleOffset++;
		mBurstIndex++;
	}

	return numSamples * numChannels * 4;

silence:
	//  Output silence when done with file
	memset(&pInAudioBuffer[sampleOffset * numChannels], 0, (numSamples - sampleOffset) * numChannels * 4);
	return numSamples * numChannels * 4;
}

#endif
