
#include "qacrc.h"
#include "ntv2utils.h"
#include "ntv2debug.h"
#include "ajabase/common/testpatterngen.h"
#include "ajabase/common/timecode.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/testpatterngen.h"

//	Uncomment to increase ANC payload size by one every 10 seconds
//#define	STEP_ANC_PAYLOAD_SIZE

static const uint32_t	VIDEO_COMPARE_LOCK_COUNT(3);
static const uint32_t	AUDIOBYTES_MAX_48K(201 * 1024);
static const uint32_t	AUDIOBYTES_MAX_96K(401 * 1024);
static const int		AUDIOSAMPLE_MAX(4096);
static const uint32_t	kAppSignature(AJA_FOURCC('Q', 'C', 'R', 'C'));
static int s_AudioMap[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define NTV2_ANCSIZE_MAX	(0x2000)
#define ANC_PAYLOAD_SIZE_MIN (4)
#define ANC_PAYLOAD_SIZE_MAX (40)
#define AJA_OUTPUT(...) AJADebug::Report(0, AJA_DebugSeverity_Debug, NULL, 0, __VA_ARGS__)
#define AUDIO_SAMPLE_MAX	(4096)
#define AUDIO_BUFFER_SIZE	(AUDIO_SAMPLE_MAX * 2 * 16 * 4)

ostream & operator << (ostream & inOutStream, const QACRC_DATA & inObj)
{
	return inOutStream << inObj.qacrcHeader;
}

QACRC_DATA::QACRC_DATA()
	: qacrcHeader(NTV2_FOURCC ('t', 'e', 's', 't'), sizeof(QACRC_DATA)),
	mbAudioData(false),
	mbAudioLevel(false),
	mbLog(false),
	mbOutputImageToFile(false),
	mbTestError(false),
	mbTrickMode(false),
	mbUseReferenceFile(false),
	mbVideoData(false),
	mbUseCSC(false),
	mbDoLevelConversion(false),
	mbEnableVanc(false),
	mbWideVanc(false),
	mbLockToInput(false),
	mbWithAnc(false),
	mbUseStaticImage(false),
	mbBurnTimecode(false),
	mbUseReferenceAsLock(false),
	mAudioDepth(16),
	mDeviceIndex(0),
	mFirstFrameIndex(0),
	mLastFrameIndex(0),
	mVerboseCount(20),
	mVideoRangeLow(0x04),
	mVideoRangeHigh(0xfb),
	mWiggleRoom(0),
	mPlayFrames(0),
	mSkipFrames(0),
	mTestCount(0),
	mAudioChannelFirst(0),
	mAudioChannelLast(1000),
	mAudioLevel(0x100)
{
}

QACrc::QACrc()
	: mbGlobalQuit(false),
	mbTestError(false),
	mACThread(NULL),
	mTestThread(NULL),
	mVideoReferenceData(NULL),
	mAudioReferenceData(NULL),
	mField1AncReferenceData(NULL),
	mField2AncReferenceData(NULL),
	mAudioBufferSize(AUDIO_BUFFER_SIZE),
	mAncBufferSize(NTV2_ANCSIZE_MAX),
	mCurrentAncPayloadSize(ANC_PAYLOAD_SIZE_MIN),
	mAncPayloadSizeLimit(ANC_PAYLOAD_SIZE_MAX),
	mVideoReferenceBufferIndex(0),
	mVideoReferenceBufferIndexLockCount(0),
	mVideoReferenceIndexLocked(false),
	mVideoReferenceIndexNeverLocked(true)
{
	for (unsigned int ndx = 0; ndx < CIRCULAR_BUFFER_SIZE; ndx++)
	{
		mAVHostBuffer[ndx].fVideoBuffer = NULL;
		mAVHostBuffer[ndx].fAudioBuffer = NULL;
		mAVHostBuffer[ndx].fAncBuffer = NULL;
		mAVHostBuffer[ndx].fAncF2Buffer = NULL;
	}
	AJADebug::Open();
}

QACrc::~QACrc(void)
{
	Quit();

	if (mpCrcData->mbSource)
		mDevice.UnsubscribeOutputVerticalEvent(mpCrcData->mVideoChannel);
	else
		mDevice.UnsubscribeInputVerticalEvent(mpCrcData->mVideoChannel);

	delete mACThread;
	mACThread = NULL;
	delete mTestThread;
	mTestThread = NULL;

	for (unsigned int ndx = 0; ndx < CIRCULAR_BUFFER_SIZE; ndx++)
	{
		if (mAVHostBuffer[ndx].fVideoBuffer)
		{
			delete[] mAVHostBuffer[ndx].fVideoBuffer;
			mAVHostBuffer[ndx].fVideoBuffer = NULL;
		}
		if (mAVHostBuffer[ndx].fAudioBuffer)
		{
			delete[] mAVHostBuffer[ndx].fAudioBuffer;
			mAVHostBuffer[ndx].fAudioBuffer = NULL;
		}
		if (mAVHostBuffer[ndx].fAncBuffer)
		{
			delete mAVHostBuffer[ndx].fAncBuffer;
			mAVHostBuffer[ndx].fAncBuffer = NULL;
		}
		if (mAVHostBuffer[ndx].fAncF2Buffer)
		{
			delete mAVHostBuffer[ndx].fAncF2Buffer;
			mAVHostBuffer[ndx].fAncF2Buffer = NULL;
		}
	}

	if (mVideoReferenceData != NULL)
	{
		delete [] mVideoReferenceData;
		mVideoReferenceData = NULL;
	}

	if (mAudioReferenceData != NULL)
	{
		delete [] mAudioReferenceData;
		mAudioReferenceData = NULL;
	}

	if (mField1AncReferenceData != NULL)
	{
		delete [] mField1AncReferenceData;
		mField1AncReferenceData = NULL;
	}

	if (mField2AncReferenceData != NULL)
	{
		delete [] mField2AncReferenceData;
		mField2AncReferenceData = NULL;
	}
}

void 
QACrc::Quit(void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mbGlobalQuit = true;

	if (mACThread)
		while (mACThread->Active())
			AJATime::Sleep(10);

	if (mTestThread)
		while (mTestThread->Active())
			AJATime::Sleep(10);

}	//	Quit



AJAStatus
QACrc::Init(QACRC_DATA & inQaCrcData)
{
	AJAStatus	status(AJA_STATUS_SUCCESS);

	mpCrcData = &inQaCrcData;

	if (!CNTV2DeviceScanner::GetDeviceAtIndex(mpCrcData->mDeviceIndex, mDevice))
	{
		cerr << "## ERROR:  Device '" << mpCrcData->mDeviceIndex << "' not found" << endl;  return AJA_STATUS_OPEN;
	}

	mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);
	mDeviceID = mDevice.GetDeviceID();

	if (::NTV2DeviceCanDoMultiFormat(mDeviceID))
		mDevice.SetMultiFormatMode(true);

	if ((mpCrcData->mVideoChannel == NTV2_CHANNEL1) && (!::NTV2DeviceCanDoFrameStore1Display(mDeviceID)))
		mpCrcData->mVideoChannel = NTV2_CHANNEL2;

	status = SetUpVideo();
	if (AJA_FAILURE(status))
		return status;

	status = SetUpAudio();
	if (AJA_FAILURE(status))
		return status;

	SetUpHostBuffers();

	mFormatDescriptor = GetFormatDescriptor(mpCrcData->mVideoFormat, mpCrcData->mPixelFormat, mpCrcData->mbEnableVanc, mpCrcData->mbWideVanc);
	mFrameRate = GetNTV2FrameRateFromVideoFormat(mpCrcData->mVideoFormat);
	mTCFormat = CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(mFrameRate);

	status = GenerateReferenceVideo();
	if (AJA_SUCCESS(status))
		status = GenerateReferenceAudio();
	if (AJA_SUCCESS(status))
		status = GenerateReferenceAnc(0, mCurrentAncPayloadSize);
	if (AJA_FAILURE(status))
		return status;

	if (mpCrcData->mbSource)
	{
		RouteOutputSignal();
		SetUpOutputAutoCirculate();
		mTCBurner.RenderTimeCodeFont(CNTV2DemoCommon::GetAJAPixelFormat(mpCrcData->mPixelFormat), mFormatDescriptor.numPixels, mFormatDescriptor.numLines);
	}
	else
	{
		RouteInputSignal();
		SetUpInputAutoCirculate();
	}

	return AJA_STATUS_SUCCESS;

}	//	Init

AJAStatus
QACrc::SetUpVideo(void)
{
	mDevice.EnableChannel(mpCrcData->mVideoChannel);

	if (NTV2DeviceCanDoCustomAnc(mDeviceID))
		mpCrcData->mbWithAnc = true;
	else
		mpCrcData->mbWithAnc = false;

	if (mpCrcData->mbUseReferenceAsLock)
		mDevice.SetReference(NTV2_REFERENCE_EXTERNAL);
	else
		mDevice.SetReference(NTV2_REFERENCE_FREERUN);

	if (mpCrcData->mbSource)
	{
		if (mpCrcData->mVideoFormat == NTV2_FORMAT_UNKNOWN)
			mDevice.GetVideoFormat(&mpCrcData->mVideoFormat, NTV2_CHANNEL1);

		if (!::NTV2DeviceCanDoVideoFormat(mDeviceID, mpCrcData->mVideoFormat))
		{
			cerr << "## ERROR:  This device cannot handle '" << ::NTV2VideoFormatToString(mpCrcData->mVideoFormat) << "'" << endl;  return AJA_STATUS_UNSUPPORTED;
		}

		//	Subscribe the output interrupt -- it's enabled by default...
		mDevice.SubscribeOutputVerticalEvent(mpCrcData->mVideoChannel);
	}
	else
	{
		if (mpCrcData->mVideoFormat == NTV2_FORMAT_UNKNOWN)
		{
			//	Determine the input video signal format...
			mpCrcData->mVideoFormat = mDevice.GetInputVideoFormat(::NTV2ChannelToInputSource(mpCrcData->mVideoChannel));
			if (mpCrcData->mVideoFormat == NTV2_FORMAT_UNKNOWN)
			{
				cerr << "## ERROR:  No input signal or unknown format" << endl;
				return AJA_STATUS_NOINPUT;	//	Sorry, can't handle this format
			}
		}

		//	Subscribe to the interrupts for the channel to be used...
		mDevice.SubscribeInputVerticalEvent(mpCrcData->mVideoChannel);
		//	Set the video format to match the incoming video format.
		//	Does the device support the desired input source?
		if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
			mDevice.SetSDITransmitEnable(mpCrcData->mVideoChannel, false);
	}


	mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL1);

	//	Configure the device to handle the requested video format...
	mDevice.SetVideoFormat(mpCrcData->mVideoFormat, false, false, mpCrcData->mVideoChannel);

	//	Set the frame buffer pixel format for all the channels on the device
	//	(assuming it supports that pixel format -- otherwise default to 8-bit YCbCr)...
	if (!::NTV2DeviceCanDoFrameBufferFormat(mDeviceID, mpCrcData->mPixelFormat))
		mpCrcData->mPixelFormat = NTV2_FBF_8BIT_YCBCR;

	mDevice.SetFrameBufferFormat(mpCrcData->mVideoChannel, mpCrcData->mPixelFormat);

	if (mpCrcData->mbEnableVanc && !::IsRGBFormat(mpCrcData->mPixelFormat) && NTV2_IS_HD_VIDEO_FORMAT(mpCrcData->mVideoFormat))
	{
		//	Try enabling VANC...
		mDevice.SetEnableVANCData(true, mpCrcData->mbWideVanc, mpCrcData->mVideoChannel);		//	Enable VANC for non-SD formats, to pass thru captions, etc.
		if (::Is8BitFrameBufferFormat(mpCrcData->mPixelFormat))
		{
			//	8-bit FBFs require VANC bit shift...
			mDevice.SetVANCShiftMode(mpCrcData->mVideoChannel, NTV2_VANCDATA_8BITSHIFT_ENABLE);
			mDevice.SetVANCShiftMode(mpCrcData->mVideoChannel, NTV2_VANCDATA_8BITSHIFT_ENABLE);
		}
	}	//	if HD video format
	else
	{
		mDevice.SetEnableVANCData(false);	//	No VANC with RGB pixel formats (for now)
	}

	// Wait about 4 verticals to let the receiver lock...
	mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1, 4);
	return AJA_STATUS_SUCCESS;
}

AJAStatus
QACrc::SetUpAudio(void)
{
	if (!::NTV2DeviceCanDoFrameStore1Display(mDeviceID) || ::NTV2DeviceGetNumAudioEngines(mDeviceID) <= 1)
		mpCrcData->mAudioSystem = NTV2_AUDIOSYSTEM_1;
	mDevice.SetNumberAudioChannels(::NTV2DeviceGetMaxAudioChannels(mDeviceID), mpCrcData->mAudioSystem);
	mDevice.SetAudioRate(NTV2_AUDIO_48K, mpCrcData->mAudioSystem);
	mDevice.SetAudioBufferSize(NTV2_AUDIO_BUFFER_BIG, mpCrcData->mAudioSystem);
	mDevice.SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, mpCrcData->mAudioSystem);

	return AJA_STATUS_SUCCESS;

}

void
QACrc::RouteOutputSignal(void)
{
	bool isRGB = ::IsRGBFormat(mpCrcData->mPixelFormat);
	mDevice.SetFrameBufferFormat(mpCrcData->mVideoChannel, mpCrcData->mPixelFormat);

	NTV2OutputCrosspointID	fsVidOutXpt(::GetFrameBufferOutputXptFromChannel(mpCrcData->mVideoChannel, isRGB, false));
	if (isRGB)
	{
		if (mpCrcData->mbUseCSC)
		{
			mDevice.Connect(::GetCSCInputXptFromChannel(mpCrcData->mVideoChannel, false), fsVidOutXpt);
			mDevice.Connect(::GetSDIOutputInputXpt(mpCrcData->mVideoChannel, false), ::GetCSCOutputXptFromChannel(mpCrcData->mVideoChannel, false, isRGB));
		}
		else
		{
			mDevice.Connect(::GetDLOutInputXptFromChannel(mpCrcData->mVideoChannel), fsVidOutXpt);
			mDevice.Connect(::GetSDIOutputInputXpt(mpCrcData->mVideoChannel, false), GetDLOutOutputXptFromChannel(mpCrcData->mVideoChannel, false));
			mDevice.Connect(::GetSDIOutputInputXpt(mpCrcData->mVideoChannel, true), GetDLOutOutputXptFromChannel(mpCrcData->mVideoChannel, true));
		}
	}
	else
	{
		mDevice.Connect(::GetSDIOutputInputXpt(mpCrcData->mVideoChannel, false), fsVidOutXpt);
		mDevice.Connect(::GetSDIOutputInputXpt(mpCrcData->mVideoChannel, true), NTV2_XptBlack);
	}

	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
		mDevice.SetSDITransmitEnable(mpCrcData->mVideoChannel, true);

	mDevice.SetSDIOutputAudioSystem(mpCrcData->mVideoChannel, mpCrcData->mAudioSystem);
	mDevice.SetSDIOutputDS2AudioSystem(mpCrcData->mVideoChannel, mpCrcData->mAudioSystem);

	if (NTV2_IS_QUAD_FRAME_FORMAT(mpCrcData->mVideoFormat))
	{
		uint32_t nextChannel = ::GetIndexForNTV2Channel(mpCrcData->mVideoChannel);
		nextChannel++;
		for (int i = 0; i < 3; i++, nextChannel++)
		{
			NTV2OutputCrosspointID	fsVidOutXpt(::GetFrameBufferOutputXptFromChannel((NTV2Channel)nextChannel, isRGB/*isRGB*/, false/*is425*/));
			if (isRGB)
			{
				if (mpCrcData->mbUseCSC)
				{
					mDevice.Connect(::GetCSCInputXptFromChannel((NTV2Channel)nextChannel, false), fsVidOutXpt);
					mDevice.Connect(::GetSDIOutputInputXpt((NTV2Channel)nextChannel, false), ::GetCSCOutputXptFromChannel((NTV2Channel)nextChannel, false, isRGB));
				}
				else // use DL
				{
					mDevice.Connect(::GetDLOutInputXptFromChannel((NTV2Channel)nextChannel), fsVidOutXpt);
					mDevice.Connect(::GetSDIOutputInputXpt((NTV2Channel)nextChannel, false), GetDLOutOutputXptFromChannel((NTV2Channel)nextChannel, false));
					mDevice.Connect(::GetSDIOutputInputXpt((NTV2Channel)nextChannel, true), GetDLOutOutputXptFromChannel((NTV2Channel)nextChannel, true));
				}
			}
			else
			{
				mDevice.Connect(::GetSDIOutputInputXpt((NTV2Channel)nextChannel, false/*isDS2*/), fsVidOutXpt);
			}

			if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
				mDevice.SetSDITransmitEnable((NTV2Channel)nextChannel, true);

			mDevice.SetFrameBufferFormat((NTV2Channel)nextChannel, mpCrcData->mPixelFormat);

			mDevice.SetSDIOutputAudioSystem((NTV2Channel)nextChannel, mpCrcData->mAudioSystem);
			mDevice.SetSDIOutputDS2AudioSystem((NTV2Channel)nextChannel, mpCrcData->mAudioSystem);
		}
	}
}

void
QACrc::RouteInputSignal(void)
{
	NTV2OutputCrosspointID	sdiInputWidgetOutputXpt(::GetSDIInputOutputXptFromChannel(mpCrcData->mVideoChannel));
	NTV2InputCrosspointID		frameBufferInputXpt(::GetFrameBufferInputXptFromChannel(mpCrcData->mVideoChannel));
	NTV2InputCrosspointID		cscWidgetVideoInputXpt(::GetCSCInputXptFromChannel(mpCrcData->mVideoChannel));
	NTV2OutputCrosspointID	cscWidgetRGBOutputXpt(::GetCSCOutputXptFromChannel(mpCrcData->mVideoChannel, /*inIsKey*/ false, /*inIsRGB*/ true));
	bool isRGB = ::IsRGBFormat(mpCrcData->mPixelFormat);

	if (isRGB)
	{
		if (mpCrcData->mbUseCSC)
		{
			mDevice.Connect(frameBufferInputXpt, cscWidgetRGBOutputXpt);		//	Frame store input to CSC widget's RGB output
			mDevice.Connect(cscWidgetVideoInputXpt, sdiInputWidgetOutputXpt);	//	CSC widget's YUV input to SDI-In widget's output
		}
		else
		{
			mDevice.Connect(::GetDLInInputXptFromChannel(mpCrcData->mVideoChannel, false), GetSDIInputOutputXptFromChannel(mpCrcData->mVideoChannel, false));
			mDevice.Connect(::GetDLInInputXptFromChannel(mpCrcData->mVideoChannel, true), GetSDIInputOutputXptFromChannel(mpCrcData->mVideoChannel, true));
			mDevice.Connect(frameBufferInputXpt, GetDLInOutputXptFromChannel(mpCrcData->mVideoChannel));
		}
	}
	else
	{
		mDevice.Connect(frameBufferInputXpt, sdiInputWidgetOutputXpt);
	}

	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
		mDevice.SetSDITransmitEnable(mpCrcData->mVideoChannel, false);

	mDevice.SetFrameBufferFormat(mpCrcData->mVideoChannel, mpCrcData->mPixelFormat);

	if (NTV2_IS_QUAD_FRAME_FORMAT(mpCrcData->mVideoFormat))
	{
		uint32_t nextChannel = ::GetIndexForNTV2Channel(mpCrcData->mVideoChannel);
		nextChannel++;
		for (int i = 0; i < 3; i++, nextChannel++)
		{
			sdiInputWidgetOutputXpt = ::GetSDIInputOutputXptFromChannel((NTV2Channel)nextChannel);
			frameBufferInputXpt = ::GetFrameBufferInputXptFromChannel((NTV2Channel)nextChannel);
			cscWidgetVideoInputXpt = ::GetCSCInputXptFromChannel((NTV2Channel)nextChannel);
			cscWidgetRGBOutputXpt = ::GetCSCOutputXptFromChannel((NTV2Channel)nextChannel, /*inIsKey*/ false, /*inIsRGB*/ true);

			if (isRGB)
			{
				if (mpCrcData->mbUseCSC)
				{
					mDevice.Connect(frameBufferInputXpt, cscWidgetRGBOutputXpt);		//	Frame store input to CSC widget's RGB output
					mDevice.Connect(cscWidgetVideoInputXpt, sdiInputWidgetOutputXpt);	//	CSC widget's YUV input to SDI-In widget's output
				}
				else
				{
					mDevice.Connect(::GetDLInInputXptFromChannel((NTV2Channel)nextChannel, false), GetSDIInputOutputXptFromChannel((NTV2Channel)nextChannel, false));
					mDevice.Connect(::GetDLInInputXptFromChannel((NTV2Channel)nextChannel, true), GetSDIInputOutputXptFromChannel((NTV2Channel)nextChannel, true));
					mDevice.Connect(frameBufferInputXpt, GetDLInOutputXptFromChannel((NTV2Channel)nextChannel));
				}
			}
			else
			{
				mDevice.Connect(frameBufferInputXpt, sdiInputWidgetOutputXpt);	//	Frame store input to SDI-In widget's output
			}

			if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
				mDevice.SetSDITransmitEnable((NTV2Channel)nextChannel, false);
		}
	}

	mDevice.SetAudioSystemInputSource(mpCrcData->mAudioSystem, NTV2_AUDIO_EMBEDDED, ::NTV2ChannelToEmbeddedAudioInput(mpCrcData->mVideoChannel));
}

void
QACrc::SetUpHostBuffers(void)
{
	mAVCircularBuffer.SetAbortFlag(&mbGlobalQuit);

	NTV2Framesize frameSize;
	mDevice.GetFrameBufferSize(mpCrcData->mVideoChannel, frameSize);
	mVideoBufferSize = ::NTV2FramesizeToByteCount(frameSize);
	if (NTV2_IS_QUAD_FRAME_FORMAT(mpCrcData->mVideoFormat))
		mVideoBufferSize *= 4;

	for (unsigned bufferNdx = 0; bufferNdx < CIRCULAR_BUFFER_SIZE; bufferNdx++)
	{
		mAVHostBuffer[bufferNdx].fVideoBuffer = reinterpret_cast <uint32_t *> (new uint8_t[mVideoBufferSize]);
		mAVHostBuffer[bufferNdx].fVideoBufferSize = mVideoBufferSize;
		mAVHostBuffer[bufferNdx].fAudioBuffer = reinterpret_cast <uint32_t *> (new uint8_t[mAudioBufferSize]);
		mAVHostBuffer[bufferNdx].fAudioBufferSize = mAudioBufferSize;
		mAVHostBuffer[bufferNdx].fAncBuffer = reinterpret_cast <uint32_t *> (new uint8_t[mAncBufferSize]);
		mAVHostBuffer[bufferNdx].fAncBufferSize = mAncBufferSize;
		mAVHostBuffer[bufferNdx].fAncF2Buffer = reinterpret_cast <uint32_t *> (new uint8_t[mAncBufferSize]);
		mAVHostBuffer[bufferNdx].fAncF2BufferSize = mAncBufferSize;
		mAVCircularBuffer.Add(&mAVHostBuffer[bufferNdx]);
	}

	mVideoReferenceBufferSize = mVideoBufferSize + 1024;
	mVideoReferenceData = (uint32_t*)new uint8_t[mVideoReferenceBufferSize];
	memset(mVideoReferenceData, 0, mVideoReferenceBufferSize);

	mAudioReferenceData = (uint32_t*)new uint8_t[mAudioBufferSize];
	memset(mAudioReferenceData, 0, mAudioBufferSize);

	mField1AncReferenceData = new uint8_t[mAncBufferSize];
	memset(mField1AncReferenceData, 0, mAncBufferSize);

	mField2AncReferenceData = new uint8_t[mAncBufferSize];
	memset(mField2AncReferenceData, 0, mAncBufferSize);

}

AJAStatus
QACrc::AddHostBuffers(const uint32_t inNumberOfBuffersToAdd)
{
	for (unsigned bufferNdx = 0; bufferNdx < inNumberOfBuffersToAdd; bufferNdx++)
	{
		mAVHostBuffer[bufferNdx].fVideoBuffer = reinterpret_cast <uint32_t *> (new uint8_t[mVideoBufferSize]);
		mAVHostBuffer[bufferNdx].fVideoBufferSize = mVideoBufferSize;
		mAVHostBuffer[bufferNdx].fAudioBuffer = reinterpret_cast <uint32_t *> (new uint8_t[mAudioBufferSize]);
		mAVHostBuffer[bufferNdx].fAudioBufferSize = mAudioBufferSize;
		mAVHostBuffer[bufferNdx].fAncBuffer = reinterpret_cast <uint32_t *> (new uint8_t[mAncBufferSize]);
		mAVHostBuffer[bufferNdx].fAncBufferSize = mAncBufferSize;
		mAVHostBuffer[bufferNdx].fAncF2Buffer = reinterpret_cast <uint32_t *> (new uint8_t[mAncBufferSize]);
		mAVHostBuffer[bufferNdx].fAncF2BufferSize = mAncBufferSize;
		if(AJA_FAILURE(mAVCircularBuffer.Add(&mAVHostBuffer[bufferNdx])))
			return AJA_STATUS_NOBUFFER;
	}
	return AJA_STATUS_SUCCESS;
}

void
QACrc::SetUpOutputAutoCirculate(void)
{
	mDevice.AutoCirculateStop(mpCrcData->mVideoChannel);

	uint32_t		numChannels(::NTV2DeviceGetNumVideoChannels(mDeviceID));
	NTV2Framesize	bufferSize(NTV2_MAX_NUM_Framesizes);
	uint32_t		numBuffers(::NTV2DeviceGetNumberFrameBuffers(mDeviceID));

	//	This is just some stuff to manage channel boundaries
	mDevice.GetFrameBufferSize(NTV2_CHANNEL1, &bufferSize);
	numBuffers -= ::NTV2DeviceGetNumAudioSystems(mDeviceID);
	if (bufferSize == NTV2_FRAMESIZE_16MB)
		numBuffers /= 2;

	uint32_t		buffersPerChannel(numBuffers / numChannels);

	mpCrcData->mFirstFrameIndex = mpCrcData->mVideoChannel * buffersPerChannel;
	mpCrcData->mLastFrameIndex = (mpCrcData->mFirstFrameIndex + buffersPerChannel - 1);

	mDevice.AutoCirculateInitForOutput(mpCrcData->mVideoChannel, 0,
		mpCrcData->mAudioSystem,
		AUTOCIRCULATE_WITH_RP188 | (mpCrcData->mbWithAnc ? AUTOCIRCULATE_WITH_ANC : 0),
		1,
		mpCrcData->mFirstFrameIndex,
		mpCrcData->mLastFrameIndex);
}

void
QACrc::SetUpInputAutoCirculate(void)
{
	mDevice.AutoCirculateStop(mpCrcData->mVideoChannel);

	uint32_t		numberOfChannels(::NTV2DeviceGetNumVideoChannels(mDeviceID));
	NTV2Framesize	bufferSize(NTV2_MAX_NUM_Framesizes);
	uint32_t		numberOfBuffers(::NTV2DeviceGetNumberFrameBuffers(mDeviceID));

	//	This is just some stuff to manage channel boundaries
	mDevice.GetFrameBufferSize(NTV2_CHANNEL1, &bufferSize);
	numberOfBuffers -= ::NTV2DeviceGetNumAudioSystems(mDeviceID);
	if (bufferSize == NTV2_FRAMESIZE_16MB)
		numberOfBuffers /= 2;

	uint32_t		buffersPerChannel(numberOfBuffers / numberOfChannels);

	mpCrcData->mFirstFrameIndex = mpCrcData->mVideoChannel * buffersPerChannel;
	mpCrcData->mLastFrameIndex = (mpCrcData->mFirstFrameIndex + buffersPerChannel - 1);

	mDevice.AutoCirculateInitForInput(mpCrcData->mVideoChannel, 0,
		mpCrcData->mAudioSystem,
		AUTOCIRCULATE_WITH_RP188 | (mpCrcData->mbWithAnc ? AUTOCIRCULATE_WITH_ANC : 0),
		1,
		mpCrcData->mFirstFrameIndex,
		mpCrcData->mLastFrameIndex);
}

AJAStatus
QACrc::GenerateReferenceVideo(void)
{
	if (mFormatDescriptor.numPixels == 0 || mFormatDescriptor.numLines == 0 || mFormatDescriptor.linePitch == 0)
		return AJA_STATUS_BADBUFFERSIZE;

	if (mpCrcData->mbVideoData && mpCrcData->mbUseReferenceFile)
	{
		FILE* pRefFile = fopen(mpCrcData->msRefFileName.c_str(), "rb");
		if (pRefFile == NULL)
		{
			fprintf(stderr, "## ERROR:  Cannot open reference image file '%s'\n", mpCrcData->msRefFileName.c_str());
			throw 0;
		}

		uint32_t iSize = (uint32_t)fread((void*)mVideoReferenceData, 4, mVideoReferenceBufferSize / 4, pRefFile);
		if (ferror(pRefFile))
		{
			fprintf(stderr, "## ERROR:  Cannot read reference image file '%s'\n", mpCrcData->msRefFileName.c_str());
			throw 0;
		}

		fclose(pRefFile);

		iSize *= 4;
		if (iSize < mVideoReferenceBufferSize)
		{
			mVideoReferenceBufferSize = iSize;
		}
	}
	else if (mpCrcData->mbVideoData && mpCrcData->mbUseStaticImage)
	{
		//	Use a convenient AJA test pattern generator object to populate an AJATestPatternBuffer with test pattern data...
		AJATestPatternBuffer	testPatternBuffer;
		AJATestPatternGen		testPatternGen;

		if (!testPatternGen.DrawTestPattern((AJATestPatternSelect)mpCrcData->mVideoChannel,
											mFormatDescriptor.numPixels,
											mFormatDescriptor.numLines - mFormatDescriptor.firstActiveLine,
											CNTV2DemoCommon::GetAJAPixelFormat(mpCrcData->mPixelFormat),
											testPatternBuffer))
		{
			cerr << "## ERROR:  DrawTestPattern failed, formatDesc: " << mFormatDescriptor << endl;
			return AJA_STATUS_FAIL;
		}

		const size_t	testPatternSize(testPatternBuffer.size());

		if (mFormatDescriptor.firstActiveLine)
		{
			//	Fill the VANC area with something valid -- otherwise the device won't emit a correctly-timed signal...
			unsigned	nVancLines(mFormatDescriptor.firstActiveLine);
			uint8_t *	pVancLine((uint8_t*)mVideoReferenceData);
			while (nVancLines--)
			{
				if (mpCrcData->mPixelFormat == NTV2_FBF_10BIT_YCBCR)
				{
					::Make10BitBlackLine(reinterpret_cast <UWord *> (pVancLine), mFormatDescriptor.numPixels);
					::PackLine_16BitYUVto10BitYUV(reinterpret_cast <const UWord *> (pVancLine), reinterpret_cast <ULWord *> (pVancLine), mFormatDescriptor.numPixels);
				}
				else if (mpCrcData->mPixelFormat == NTV2_FBF_8BIT_YCBCR)
					::Make8BitBlackLine(pVancLine, mFormatDescriptor.numPixels);
				else
				{
					cerr << "## ERROR:  Cannot initialize video buffer's VANC area" << endl;
					return AJA_STATUS_FAIL;
				}
				pVancLine += mFormatDescriptor.linePitch * 4;
			}	//	for each VANC line
		}	//	if has VANC area

		for (size_t ndx = 0; ndx < testPatternSize; ndx++)
			((uint8_t*)mVideoReferenceData)[ndx] = testPatternBuffer[ndx];

	}
	else if (mpCrcData->mbVideoData)
	{
		switch (mpCrcData->mPixelFormat)
		{
		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
			if (!mpCrcData->mWiggleRoom)
				mpCrcData->mWiggleRoom = 3;
		case NTV2_FBF_8BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:
		{
			UByte* pRef = (UByte*)mVideoReferenceData;
			UByte* pEnd = (UByte*)mVideoReferenceData + mVideoBufferSize + 256;
			UByte uLow = mpCrcData->mVideoRangeLow;
			UByte uHigh = mpCrcData->mVideoRangeHigh;
			UByte uCnt = uLow;
			while (pRef < pEnd)
			{
				if (uCnt > uHigh)
				{
					uCnt = uLow;
				}
				*pRef = uCnt;
				pRef++;
				uCnt++;
			}
			break;
		}
		case NTV2_FBF_10BIT_YCBCR:
		{
			uint32_t uNumBlocks = (mpCrcData->mVideoRangeHigh - mpCrcData->mVideoRangeLow) / 12;
			if (!uNumBlocks)
				uNumBlocks = 1;
			if (uNumBlocks > 1)
				uNumBlocks--;

			uint32_t uCnt = mpCrcData->mVideoRangeLow;
			for (int i = 0; i < 4; i++)
			{
				mVideoReferenceData[i] = uCnt++;
				mVideoReferenceData[i] |= uCnt++ << 10;
				mVideoReferenceData[i] |= uCnt++ << 20;
			}

			uint32_t* pPrev = mVideoReferenceData;
			uint32_t* pRef = mVideoReferenceData + 4;
			uint32_t  uIncrement = 0x00C0300C;		//	Adds 12 to all three components
			uint32_t  uBlockCount = 0;
			for (uint32_t i = 0; i < ((mVideoBufferSize / 16) - 1); i++)
			{
				if (uBlockCount == uNumBlocks)
				{
					*pRef++ = mVideoReferenceData[0];
					*pRef++ = mVideoReferenceData[1];
					*pRef++ = mVideoReferenceData[2];
					*pRef++ = mVideoReferenceData[3];

					pPrev += 4;
					uBlockCount = 0;
				}
				else
				{
					*pRef++ = (*pPrev++) + uIncrement;
					*pRef++ = (*pPrev++) + uIncrement;
					*pRef++ = (*pPrev++) + uIncrement;
					*pRef++ = (*pPrev++) + uIncrement;

					uBlockCount++;
				}
			}
			break;
		}
		default:
			fprintf(stderr, "## ERROR:  Pixel format not supported!\n");
			break;
		}
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus
QACrc::GenerateReferenceAudio(void)
{
	if (mpCrcData->mbUseStaticImage)
	{
		NTV2FrameRate	frameRate;
		NTV2AudioRate	audioRate;
		ULWord			numChannels;

		mDevice.GetFrameRate(&frameRate, mpCrcData->mVideoChannel);
		mDevice.GetAudioRate(audioRate, mpCrcData->mAudioSystem);
		mDevice.GetNumberAudioChannels(numChannels, mpCrcData->mAudioSystem);

		/**
		Because audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will necessarily
		result in some frames having more audio samples than others. The GetAudioSamplesPerFrame function
		is used to calculate the correct sample count...
		**/
		const double	audioSampleRate(audioRate == NTV2_AUDIO_96K ? 96000.0 : 48000.0);

		double toneFrequency[16] = { 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0 };
		double	gAmplitudes[32] = { 0.010, 0.015, 0.020, 0.025, 0.030, 0.035, 0.040, 0.045, 0.050, 0.055, 0.060, 0.065, 0.070, 0.075, 0.080, 0.085,
			0.085, 0.080, 0.075, 0.070, 0.065, 0.060, 0.055, 0.050, 0.045, 0.040, 0.035, 0.030, 0.025, 0.020, 0.015, 0.010 };
		uint32_t mCurrentSample = 0;
		::AddAudioTone(mAudioReferenceData,	//	audio buffer to fill
						mCurrentSample,		//	which sample for continuing the waveform
						4096,				//	number of samples to generate
						audioSampleRate,	//	sample rate [Hz]
						gAmplitudes,		//	amplitude
						&toneFrequency[0],	//	tone frequency [Hz]
						31,					//	bits per sample
						false,				//	don't byte swap
						numChannels);		//	number of audio channels to generate
	}
	else
	{
		ULWord numberOfAudioChannels = 0;
		mDevice.GetNumberAudioChannels(numberOfAudioChannels, mpCrcData->mAudioSystem);

		ULWord ulAudioInput = 0;
		mDevice.ReadAudioSource(&ulAudioInput, mpCrcData->mVideoChannel);

		if (numberOfAudioChannels < 16)
		{
			if (::NTV2DeviceNeedsRoutingSetup(mDeviceID))
			{
				if ((ulAudioInput & 0xf) == 5)
				{
					s_AudioMap[0] = 8;
					s_AudioMap[1] = 9;
					s_AudioMap[2] = 10;
					s_AudioMap[3] = 11;
					s_AudioMap[4] = 12;
					s_AudioMap[5] = 13;
					s_AudioMap[6] = 14;
					s_AudioMap[7] = 15;
				}
			}
			else
			{
				int audioPair;
				for (audioPair = 0; audioPair < 4; audioPair++)
				{
					ULWord ulMap = (ulAudioInput >> (audioPair * 4)) & 0xf;
					s_AudioMap[audioPair * 2] = (audioPair)* 2;
					s_AudioMap[audioPair * 2 + 1] = (audioPair)* 2 + 1;
					if (ulMap != 0)
					{
						s_AudioMap[audioPair * 2] = (ulMap - 1) * 2;
						s_AudioMap[audioPair * 2 + 1] = (ulMap - 1) * 2 + 1;
					}
				}
			}
		}

		if (mpCrcData->mAudioChannelFirst >= numberOfAudioChannels)
		{
			mpCrcData->mAudioChannelFirst = numberOfAudioChannels - 1;
		}
		if (mpCrcData->mAudioChannelLast >= numberOfAudioChannels)
		{
			mpCrcData->mAudioChannelLast = numberOfAudioChannels - 1;
		}
		if (mpCrcData->mAudioChannelFirst < 0)
		{
			mpCrcData->mAudioChannelFirst = 0;
		}
		if (mpCrcData->mAudioChannelLast < 0)
		{
			mpCrcData->mAudioChannelLast = 0;
		}

		int audioSample = 0;
		uint32_t audioChannel = 0;
		int dataShift = 20;
		int channelShift = 16;
		if (mpCrcData->mAudioDepth == 20)
		{
			dataShift = 16;
			channelShift = 12;
		}
		if (mpCrcData->mAudioDepth == 24)
		{
			dataShift = 12;
			channelShift = 8;
		}
		for (audioSample = 0; audioSample < AUDIO_SAMPLE_MAX; audioSample++)
		{
			for (audioChannel = 0; audioChannel < numberOfAudioChannels; audioChannel++)
			{
				int iOffset = audioSample*numberOfAudioChannels + audioChannel;
				mAudioReferenceData[iOffset] = (audioSample << dataShift) | (s_AudioMap[audioChannel] << channelShift);
				mAudioReferenceData[AUDIO_SAMPLE_MAX*numberOfAudioChannels + iOffset] = mAudioReferenceData[iOffset];
			}
		}
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus QACrc::GenerateReferenceAnc( uint32_t inVideoReferenceBufferIndex, uint32_t inPayloadSize )
{
	//	Fill in anc buffers with a payload matching the video buffer
	uint8_t *	pVideoBytes	= (uint8_t*)&mVideoReferenceData[inVideoReferenceBufferIndex];
	uint32_t	ancOffset1	= 0;
	uint32_t	ancOffset2	= 0;

	memset(mField1AncReferenceData, 0, mAncBufferSize);
	memset(mField2AncReferenceData, 0, mAncBufferSize);
	
	//	Y VANC - field 1
	mField1AncReferenceData[ancOffset1++] = 0xFF;
	mField1AncReferenceData[ancOffset1++] = 0xA0;			//	Valid, digital, Y, VANC
	mField1AncReferenceData[ancOffset1++] = 0x10;			//	Line 16
	mField1AncReferenceData[ancOffset1++] = 0xC0;			//	DID
	mField1AncReferenceData[ancOffset1++] = 0x01;			//	SDID
	mField1AncReferenceData[ancOffset1++] = inPayloadSize;	//	DC
	for (uint32_t i = 0; i < inPayloadSize; i++)
		mField1AncReferenceData[ancOffset1++] = *pVideoBytes++;
	mField1AncReferenceData[ancOffset1++] = 0x00;			//	CS filled in by firmware

	//	C VANC - field 1
	mField1AncReferenceData[ancOffset1++] = 0xFF;
	mField1AncReferenceData[ancOffset1++] = 0x80;			//	Valid, digital, C, VANC
	mField1AncReferenceData[ancOffset1++] = 0x11;			//	Line 17
	mField1AncReferenceData[ancOffset1++] = 0xC0;			//	DID
	mField1AncReferenceData[ancOffset1++] = 0x02;			//	SDID
	mField1AncReferenceData[ancOffset1++] = inPayloadSize;	//	DC
	for (uint32_t i = 0; i < inPayloadSize; i++)
		mField1AncReferenceData[ancOffset1++] = *pVideoBytes++;
	mField1AncReferenceData[ancOffset1++] = 0x00;			//	CS filled in by firmware

	//	Y VANC - field 2
	mField2AncReferenceData[ancOffset2++] = 0xFF;
	mField2AncReferenceData[ancOffset2++] = 0xA4;			//	Valid, digital, Y, VANC, LN10
	mField2AncReferenceData[ancOffset2++] = 0x45;			//	Line 581 (field 2 line 18)
	mField2AncReferenceData[ancOffset2++] = 0xC0;			//	DID
	mField2AncReferenceData[ancOffset2++] = 0x03;			//	SDID
	mField2AncReferenceData[ancOffset2++] = inPayloadSize;	//	DC
	for (uint32_t i = 0; i < inPayloadSize; i++)
		mField2AncReferenceData[ancOffset2++] = *pVideoBytes++;
	mField2AncReferenceData[ancOffset2++] = 0x00;			//	CS filled in by firmware

	//	Note there are no HANC packets being inserted as the firmware
	//	does not currently support this feature. Also, C channel HANC
	//	packets would require extra logic to avoid a collision with audio.
	return AJA_STATUS_SUCCESS;
}

AJAStatus
QACrc::GenerateReferenceTimeCode(uint32_t framesProcessed)
{
	CRP188 rp188Info(framesProcessed+1, mTCFormat);
	rp188Info.GetRP188Reg(mRP188ReferenceData);
	rp188Info.GetRP188Str(mRP188String);
	return AJA_STATUS_SUCCESS;
}

AJAStatus
QACrc::Run(void)
{
	StartThreads();
	return AJA_STATUS_SUCCESS;
}

void
QACrc::StartThreads(void)
{
	mACThread = new AJAThread();
	if (mpCrcData->mbSource)
	{
		mACThread->Attach(SourceThreadStatic, this);
		mACThread->SetPriority(AJA_ThreadPriority_High);
		mACThread->Start();
	}
	else
	{
		mACThread->Attach(SinkThreadStatic, this);
		mACThread->SetPriority(AJA_ThreadPriority_High);
		mACThread->Start();

		mTestThread = new AJAThread();
		mTestThread->Attach(DataInspectionThreadStatic, this);
		mTestThread->SetPriority(AJA_ThreadPriority_High);
		mTestThread->Start();
	}
}

void
QACrc::SourceThreadStatic(AJAThread * pThread, void * pContext)
{
	(void)pThread;
	QACrc *	pApp(reinterpret_cast <QACrc *> (pContext));
	pApp->ACSource();
}

void
QACrc::ACSource(void)
{
	uint32_t ulActiveVideoSize = GetVideoActiveSize(mpCrcData->mVideoFormat, mpCrcData->mPixelFormat, mpCrcData->mbEnableVanc, mpCrcData->mbWideVanc);
	uint32_t runCount = 0;
	int audioSample = 0;
	int audioSampleMax = 4096;
	int samplesPerFrame = 0;
	NTV2FrameRate frameRate = GetNTV2FrameRateFromVideoFormat(mpCrcData->mVideoFormat);
	uint32_t numAudioChannels = 0;
	uint32_t referenceBufferIndex = 0;
	uint32_t trickModePlayCount = 0;
	mVideoErrorInsertionCount = 0;
	mAudioErrorInsertionCount = 0;
	mDevice.GetNumberAudioChannels(numAudioChannels, mpCrcData->mAudioSystem);
	mDevice.AutoCirculateStart(mpCrcData->mVideoChannel);
	mDevice.WaitForOutputFieldID(NTV2_FIELD0, mpCrcData->mVideoChannel);

	if (mVideoBufferSize > ulActiveVideoSize)
		mVideoBufferSize = ulActiveVideoSize;

	while (!mbGlobalQuit && (mpCrcData->mTestCount == 0 || runCount < mpCrcData->mTestCount))
	{
		mDevice.AutoCirculateGetStatus(mpCrcData->mVideoChannel, mACStatus);
		uint32_t numACBuffers = mpCrcData->mLastFrameIndex - mpCrcData->mFirstFrameIndex;
		if (mACStatus.acBufferLevel >= numACBuffers)
		{
			mDevice.WaitForOutputVerticalInterrupt(mpCrcData->mVideoChannel);
			continue;
		}

		if (referenceBufferIndex > 61)
		{
			referenceBufferIndex = 0;
		}

		GenerateReferenceTimeCode(runCount);

		if (mpCrcData->mbUseStaticImage)
		{
			referenceBufferIndex = 0;
			if (mpCrcData->mbBurnTimecode)
				mTCBurner.BurnTimeCode(reinterpret_cast <char *> (mVideoReferenceData), mRP188String.c_str(), 80);
		}

		mTransfer.SetVideoBuffer((uint32_t*)&mVideoReferenceData[referenceBufferIndex], ulActiveVideoSize);
		mTransfer.SetOutputTimeCode(NTV2_RP188(mRP188ReferenceData), ::NTV2ChannelToTimecodeIndex(mpCrcData->mVideoChannel, false));
		mTransfer.SetOutputTimeCode(NTV2_RP188(mRP188ReferenceData), ::NTV2ChannelToTimecodeIndex(mpCrcData->mVideoChannel, true));
		if (mpCrcData->mbUseStaticImage)
			mTransfer.SetOutputTimeCode(NTV2_RP188(mRP188ReferenceData), NTV2_TCINDEX_LTC1);

		GenerateReferenceAnc(referenceBufferIndex, mCurrentAncPayloadSize);
		mTransfer.SetAncBuffers(reinterpret_cast <ULWord *> (mField1AncReferenceData), mAncBufferSize,		//	Actual transfer sizes rounded
								reinterpret_cast <ULWord *> (mField2AncReferenceData), mAncBufferSize);		//	up to a DWORD count

		referenceBufferIndex++;

#if !defined(STEP_ANC_PAYLOAD_SIZE)
		mCurrentAncPayloadSize++;
		if (mCurrentAncPayloadSize > mAncPayloadSizeLimit)
			mCurrentAncPayloadSize = ANC_PAYLOAD_SIZE_MIN;
#endif

		if (audioSample >= audioSampleMax)
		{
			audioSample %= audioSampleMax;
		}
		samplesPerFrame = GetAudioSamplesPerFrame(frameRate, NTV2_AUDIO_48K, runCount);
		mTransfer.SetAudioBuffer((ULWord*)&mAudioReferenceData[audioSample*numAudioChannels], samplesPerFrame*numAudioChannels * 4);
		audioSample += samplesPerFrame;

		if (mpCrcData->mbTrickMode)
		{
			if (trickModePlayCount <= mpCrcData->mPlayFrames)
			{
				bool status = mDevice.AutoCirculateTransfer(mpCrcData->mVideoChannel, mTransfer);
				if (!status)
				{
					fprintf(stderr, "## ERROR:  TransferWithAutoCirculate failed!\n");
					throw 0;
				}

				runCount++;
				trickModePlayCount++;
			}
			else
			{
				uint32_t originalVideoValue = 0;
				if (mpCrcData->mbVideoData)
				{
					originalVideoValue = reinterpret_cast <uint32_t *> (mTransfer.acVideoBuffer.GetHostPointer())[10];
					reinterpret_cast <uint32_t *> (mTransfer.acVideoBuffer.GetHostPointer())[10] = 0xabababab;
					mVideoErrorInsertionCount++;
				}

				bool status = mDevice.AutoCirculateTransfer(mpCrcData->mVideoChannel, mTransfer);
				if (!status)
				{
					fprintf(stderr, "## ERROR:  TransferWithAutoCirculate failed!\n");
					throw 0;
				}
				if (mpCrcData->mbVideoData)
					reinterpret_cast <uint32_t *> (mTransfer.acVideoBuffer.GetHostPointer())[10] = originalVideoValue;

				runCount++;
				trickModePlayCount = 0;
			}
		}	//	mpCrcData->mbTrickMode
		else
		{
			bool status = mDevice.AutoCirculateTransfer(mpCrcData->mVideoChannel, mTransfer);
			if (!status)
			{
				fprintf(stderr, "## ERROR:  TransferWithAutoCirculate failed!\n");
			}

			runCount++;
		}	//	!mpCrcData->mbTrickMode

#if defined(STEP_ANC_PAYLOAD_SIZE)
		if ((runCount % 300) == 0)
		{
			mCurrentAncPayloadSize++;
			if (mCurrentAncPayloadSize > mAncPayloadSizeLimit)
				mCurrentAncPayloadSize = ANC_PAYLOAD_SIZE_MIN;

			printf("\n\nAnc payload count now %d bytes\n", mCurrentAncPayloadSize);
		}
#endif
	}	//	loop til mGlobalQuit goes true
	mDevice.AutoCirculateStop(mpCrcData->mVideoChannel);
}

void
QACrc::SinkThreadStatic(AJAThread * pThread, void * pContext)
{
	(void)pThread;
	QACrc *	pApp(reinterpret_cast <QACrc *> (pContext));
	pApp->ACSink();
}

void
QACrc::ACSink(void)
{
	mDevice.AutoCirculateStart(mpCrcData->mVideoChannel);
	while (!mbGlobalQuit)
	{
		mDevice.AutoCirculateGetStatus(mpCrcData->mVideoChannel, mACStatus);
		if (mACStatus.IsRunning() && mACStatus.HasAvailableInputFrame())
		{
			AVDataBuffer * captureData(mAVCircularBuffer.StartProduceNextBuffer());
			if (captureData)
			{
				mTransfer.SetVideoBuffer(captureData->fVideoBuffer, captureData->fVideoBufferSize);
				mTransfer.SetAudioBuffer(captureData->fAudioBuffer, mAudioBufferSize);
				mTransfer.SetAncBuffers(captureData->fAncBuffer, NTV2_ANCSIZE_MAX,
										captureData->fAncF2Buffer, NTV2_ANCSIZE_MAX);

				if (!mDevice.AutoCirculateTransfer(mpCrcData->mVideoChannel, mTransfer))
				{
					fprintf(stderr, "## ERROR:  TransferWithAutoCirculate failed!\n");
				}
				else
				{
					captureData->fAudioRecordSize = mTransfer.acTransferStatus.acAudioTransferSize;
					captureData->fAncBufferSize = mTransfer.acTransferStatus.acAncTransferSize;
					captureData->fAncF2BufferSize = mTransfer.acTransferStatus.acAncField2TransferSize;
				}
			}
			mAVCircularBuffer.EndProduceNextBuffer();
		}
		else
		{
			mDevice.WaitForInputVerticalInterrupt(mpCrcData->mVideoChannel);
		}
	}	//	loop til quit signaled
	mDevice.AutoCirculateStop(mpCrcData->mVideoChannel);
}

void
QACrc::DataInspectionThreadStatic(AJAThread * pThread, void * pContext)
{
	(void)pThread;
	QACrc *	pApp(reinterpret_cast <QACrc *> (pContext));
	pApp->InspectData();
}

AJAStatus
QACrc::InspectData()
{
	uint32_t runCount = 0;
	int acTransferFrameNumber = mTransfer.acTransferStatus.acTransferFrame;
	int numberOfAudioSamples = 0;
	uint32_t numberOfAudioChannels = 0;
	uint32_t ulActiveVideoSize = 0;
	bool bVideoError = false;
	bool bAudioError = false;
	bool bAncF1Error = false;
	bool bAncF2Error = false;
	bool bVideoErrorInsertionDetection = false;
	bool bAudioErrorInsertionDetected = false;
	int audioCount[16] = { 0 };
	int referenceBufferIndex = 0;
	int numberOfCompareDifferences = 0;
	ULWord64 AudioSum[16];
	bool bSyncToReference = true;
	FILE* pLogFile = fopen(mpCrcData->msLogFileName.c_str(), "a");
	mVideoErrorCount = 0;
	mVideoErrorInsertionCount = 0;
	mAudioErrorInsertionCount = 0;
	mAudioErrorCount = 0;
	mAncF1ErrorCount = 0;
	mAncF2ErrorCount = 0;

	ulActiveVideoSize = GetVideoActiveSize(mpCrcData->mVideoFormat, mpCrcData->mPixelFormat, mpCrcData->mbEnableVanc, mpCrcData->mbWideVanc);
	mDevice.GetNumberAudioChannels(numberOfAudioChannels, mpCrcData->mAudioSystem);

	while (!mbGlobalQuit)
	{
		AVDataBuffer * testData(mAVCircularBuffer.StartConsumeNextBuffer());
		if (testData)
		{
			bVideoError = false;
			bVideoErrorInsertionDetection = false;
			bAudioErrorInsertionDetected = false;
			bAudioError = false;
			bAncF1Error = false;
			bAncF2Error = false;
			numberOfCompareDifferences = 0;

			if (runCount == 0)
			{
// 				this cn be optionally supported. Would need a new cli flag and crcdata member
// 				{
// 					memcpy(mVideoReferenceData, testData->fVideoBuffer, testData->fVideoBufferSize);
// 				}

				if (mpCrcData->mbOutputImageToFile)
				{
					FILE* pOutFile = fopen(mpCrcData->msOutputFileName.c_str(), "wb");
					if (pOutFile == NULL)
					{
						fprintf(stderr, "error: can not open output image file %s\n", mpCrcData->msOutputFileName.c_str());
						throw 0;
					}

					fwrite(mVideoReferenceData, 4, ulActiveVideoSize / 4, pOutFile);
					if (ferror(pOutFile))
					{
						fprintf(stderr, "error: can not write output image file %s\n", mpCrcData->msOutputFileName.c_str());
						throw 0;
					}

					fclose(pOutFile);
				}
				mVideoReferenceBufferIndex = 0;
				mVideoReferenceBufferIndexLockCount = 0;
				mVideoReferenceIndexLocked = false;
			}

			switch (mpCrcData->mDataInspectionLevel)
			{
			default:
			case 0:
				{	
					if (mpCrcData->mbVideoData)
					{
						int iVCount = 0;
						switch (mpCrcData->mPixelFormat)
						{
						default:
						case NTV2_FBF_ARGB:
						case NTV2_FBF_RGBA:
						case NTV2_FBF_ABGR:
							if (bSyncToReference)
							{
								referenceBufferIndex = ((testData->fVideoBuffer[0] & 0x000000fc) >> 2) - 1;
								if (referenceBufferIndex < 0)
								{
									referenceBufferIndex = 0;
								}
							}
							break;
						case NTV2_FBF_10BIT_YCBCR:
							referenceBufferIndex = ((testData->fVideoBuffer[0] & 0x000003FF) - 4) / 3;
							break;
						}

						if (referenceBufferIndex > 62)
						{
							referenceBufferIndex = 1;
						}

						uint32_t nextBufferIndex = mVideoReferenceBufferIndex + 1;
						if (nextBufferIndex > 61)
						{
							nextBufferIndex = 0;
						}

						if (!mVideoReferenceIndexLocked)
						{
							if (referenceBufferIndex == nextBufferIndex)
							{
								mVideoReferenceBufferIndex = nextBufferIndex;
								mVideoReferenceBufferIndexLockCount++;
							}
							else
							{
								if (!mVideoReferenceIndexNeverLocked)
									referenceBufferIndex = 0;	//	Force a compare error
								mVideoReferenceBufferIndexLockCount = 0;
							}

							if (mVideoReferenceBufferIndexLockCount == VIDEO_COMPARE_LOCK_COUNT)
							{
								mVideoReferenceIndexLocked = true;
								mVideoReferenceIndexNeverLocked = false;
							}
						}
						else
						{
							if (referenceBufferIndex != nextBufferIndex)
							{
								if (pLogFile != NULL)
								{
									fprintf(pLogFile, "Lost input cadence lock at frame %d, ref index %x, actual index %x\n",
										 runCount, referenceBufferIndex, nextBufferIndex);
								}
								mVideoReferenceIndexLocked = false;
								mVideoReferenceBufferIndex = 0;
								mVideoReferenceBufferIndexLockCount = 0;
							}
							else
								mVideoReferenceBufferIndex = nextBufferIndex;
						}

						for (uint32_t sourceIndex = 0; sourceIndex < ulActiveVideoSize/4; sourceIndex++)
						{
							switch (mpCrcData->mPixelFormat)
							{
							case NTV2_FBF_ARGB:
							case NTV2_FBF_RGBA:
							case NTV2_FBF_ABGR:
							{
								int8_t * pSrcBytes = (int8_t*)&testData->fVideoBuffer[sourceIndex];
								int8_t * pRefBytes = (int8_t*)&mVideoReferenceData[sourceIndex + referenceBufferIndex];
								for (int i = 0; i < 4; i++)
								{
									if (std::abs((long)*pSrcBytes++ - *pRefBytes++) > mpCrcData->mWiggleRoom)
									{
										bVideoError = true;
										break;
									}
								}
							}
								break;
							default:
							case NTV2_FBF_8BIT_YCBCR:
							case NTV2_FBF_10BIT_YCBCR:
								if (testData->fVideoBuffer[sourceIndex] != mVideoReferenceData[sourceIndex + referenceBufferIndex])
								{
									if (testData->fVideoBuffer[sourceIndex] == 0xabababab)
										bVideoErrorInsertionDetection = true;
									else
										bVideoError = true;
								}
								break;
							}
	
							if (bVideoError || bVideoErrorInsertionDetection)
							{
								mbTestError = true;
	
								if (iVCount < mpCrcData->mVerboseCount)
								{
									if (mpCrcData->mbLogVideoErrors)
									{
										AJA_OUTPUT("frm %9d  buf %2d  off %08x  srx %08x %08x %08x\n",
											runCount, acTransferFrameNumber, sourceIndex * 4, testData->fVideoBuffer[sourceIndex], mVideoReferenceData[sourceIndex + referenceBufferIndex],
											testData->fVideoBuffer[sourceIndex] ^ mVideoReferenceData[sourceIndex + referenceBufferIndex]);
									}
									if (pLogFile != NULL)
									{
										fprintf(pLogFile, "bad frame %9d  fb %2d  off %08x  src %08x  ref %08x  xor %08x\n",
											runCount, acTransferFrameNumber, sourceIndex * 4, testData->fVideoBuffer[sourceIndex], mVideoReferenceData[sourceIndex + referenceBufferIndex],
											testData->fVideoBuffer[sourceIndex] ^ mVideoReferenceData[sourceIndex + referenceBufferIndex]);
									}
									if (mpCrcData->mVerboseCount > 0)
									{
										iVCount++;
									}
								}
								numberOfCompareDifferences++;
							}
	
							if ((mpCrcData->mTestCount != 0) && (runCount > mpCrcData->mTestCount))
							{
								break;
							}
						}
	
						if (mpCrcData->mbVideoData)
						{
							referenceBufferIndex++;
						}
	
						if (numberOfCompareDifferences > 0)
						{
							if (mpCrcData->mbLogVideoErrors)
							{
								AJA_OUTPUT("frm %9d  fb %2d  diffs %9d\n", runCount, acTransferFrameNumber, numberOfCompareDifferences);
							}
							if (pLogFile != NULL)
							{
								fprintf(pLogFile, "frm %9d  fb %2d  diffs %9d\n", runCount, acTransferFrameNumber, numberOfCompareDifferences);
								fflush(pLogFile);
							}
						}
					}			
				} 
				break;
			case 1:
				{
					if (mpCrcData->mbVideoData)
					{
						switch (mpCrcData->mPixelFormat)
						{
						default:
						case NTV2_FBF_ARGB:
						case NTV2_FBF_RGBA:
						case NTV2_FBF_ABGR:
							if (bSyncToReference)
							{
								referenceBufferIndex = ((testData->fVideoBuffer[0] & 0x000000fc) >> 2) - 1;
								if (referenceBufferIndex < 0)
								{
									referenceBufferIndex = 0;
								}
							}
							break;
						case NTV2_FBF_10BIT_YCBCR:
							referenceBufferIndex = ((testData->fVideoBuffer[0] & 0x000003FF) - 4) / 3;
							break;
						}

						if (referenceBufferIndex > 62)
						{
							referenceBufferIndex = 1;
						}

						int8_t * pSrcBytes = (int8_t*)&testData->fVideoBuffer[0];
						int8_t * pRefBytes = (int8_t*)&mVideoReferenceData[referenceBufferIndex];
						if (memcmp(pSrcBytes, pRefBytes, ulActiveVideoSize) != 0)
							bVideoError = true;

						referenceBufferIndex++;

						if (bVideoError)
						{
							if (mpCrcData->mbLogVideoErrors)
							{
								AJA_OUTPUT("frm %9d  fb %2d  diffs %9d\n", runCount, acTransferFrameNumber, numberOfCompareDifferences);
							}
							if (pLogFile != NULL)
							{
								fprintf(pLogFile, "frm %9d  fb %2d  diffs %9d\n", runCount, acTransferFrameNumber, numberOfCompareDifferences);
								fflush(pLogFile);
							}
						}

						if ((mpCrcData->mTestCount != 0) && (runCount > mpCrcData->mTestCount))
						{
							break;
						}
					}
				}
				break;
			}

			if (mpCrcData->mbWithAnc)
			{
				uint8_t * pAnc1 = (uint8_t*)testData->fAncBuffer;
				uint8_t * pAnc2 = (uint8_t*)testData->fAncF2Buffer;
				uint8_t * pVid  = (uint8_t*)testData->fVideoBuffer;

				if (CheckAncPacketAndVideo (pAnc1, &pVid, false) != AJA_STATUS_SUCCESS)
					bAncF1Error = true;

				if (!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(mpCrcData->mVideoFormat) &&
					CheckAncPacketAndVideo (pAnc2, &pVid, true) != AJA_STATUS_SUCCESS)
					bAncF2Error = true;
			}

			if (mpCrcData->mbAudioData)
			{
				if (mpCrcData->mbAudioData | mpCrcData->mbAudioLevel)
				{
					numberOfAudioSamples = testData->fAudioRecordSize / numberOfAudioChannels / 4;
				}
				if (bSyncToReference)
				{
					int dataShift = 20;
					ULWord dataMask = 0xfff00000;
					if (mpCrcData->mAudioDepth == 20)
					{
						dataShift = 16;
						dataMask = 0xffff0000;
					}
					if (mpCrcData->mAudioDepth == 24)
					{
						dataShift = 12;
						dataMask = 0xfffff000;
					}
					for (uint32_t audioChannel = mpCrcData->mAudioChannelFirst; audioChannel <= mpCrcData->mAudioChannelLast; audioChannel++)
					{
						int audioSample = 0;
						if (!mpCrcData->mbAudioAlignment || (audioChannel == mpCrcData->mAudioChannelFirst))
						{
							audioSample = ((testData->fAudioBuffer[audioChannel] & dataMask) >> dataShift);
						}
						audioCount[audioChannel] = audioSample;
					}
				}

				int iVCount = 0;
				for (int sourceIndex = 0; sourceIndex < numberOfAudioSamples; sourceIndex++)
				{
					for (uint32_t audioChannel = mpCrcData->mAudioChannelFirst; audioChannel <= mpCrcData->mAudioChannelLast; audioChannel++)
					{
						if (audioCount[audioChannel] >= AUDIOSAMPLE_MAX)
						{
							audioCount[audioChannel] = 0;
						}
						int sourceOffset = sourceIndex*numberOfAudioChannels + audioChannel;
						int referenceOffset = audioCount[audioChannel] * numberOfAudioChannels + audioChannel;
						if (testData->fAudioBuffer[sourceOffset] != mAudioReferenceData[referenceOffset])
						{
							if (testData->fAudioBuffer[sourceOffset] == 0xabababab)
							{
								bAudioErrorInsertionDetected = true;
							}
							else
							{
								mbTestError = true;
								bAudioError = true;
							}

							if (iVCount < mpCrcData->mVerboseCount)
							{
								if (mpCrcData->mbLogAudioErrors)
								{
									AJA_OUTPUT("frm %9d sam %4d chn %2d srx %08x %08x %08x\n",
										runCount, sourceIndex, audioChannel, testData->fAudioBuffer[sourceOffset], mAudioReferenceData[referenceOffset],
										testData->fAudioBuffer[sourceOffset] ^ mAudioReferenceData[referenceOffset]);
								}
								if (pLogFile != NULL)
								{
									fprintf(pLogFile, "bad frame %9d  sam %4d  chn %2d  src %08x  ref %08x  xor %08x\n",
										runCount, sourceIndex, audioChannel, testData->fAudioBuffer[sourceOffset], mAudioReferenceData[referenceOffset],
										testData->fAudioBuffer[sourceOffset] ^ mAudioReferenceData[referenceOffset]);
								}
								if (mpCrcData->mVerboseCount > 0)
								{
									iVCount++;
								}
							}
						}
						audioCount[audioChannel]++;
					}

					if ((mpCrcData->mTestCount != 0) && (runCount > mpCrcData->mTestCount))
					{
						break;
					}
				}
			}

			if (mpCrcData->mbAudioLevel)
			{
				int iVCount = 0;
				for (int sourceIndex = 0; sourceIndex < numberOfAudioSamples; sourceIndex++)
				{
					for (uint32_t audioChannel = 0; audioChannel < numberOfAudioChannels; audioChannel++)
					{
						AudioSum[audioChannel] = 0;
						if ((audioChannel < mpCrcData->mAudioChannelFirst) || (audioChannel > mpCrcData->mAudioChannelLast))
						{
							continue;
						}
						int iSrcOffset = sourceIndex*numberOfAudioChannels + audioChannel;
						LWord lAudioSample = testData->fAudioBuffer[iSrcOffset] >> 16;
						if (lAudioSample < 0)
						{
							lAudioSample = -lAudioSample;
						}
						AudioSum[audioChannel] += lAudioSample;
					}
				}

				for (uint32_t audioChannel = 0; audioChannel < numberOfAudioChannels; audioChannel++)
				{
					if ((audioChannel < mpCrcData->mAudioChannelFirst) || (audioChannel > mpCrcData->mAudioChannelLast))
					{
						continue;
					}

					if ((AudioSum[audioChannel] / numberOfAudioSamples) < mpCrcData->mAudioLevel)
					{
						mbTestError = true;
						bAudioError = true;
						if (iVCount < mpCrcData->mVerboseCount)
						{
							if (mpCrcData->mbLogAudioErrors)
							{
								AJA_OUTPUT("frm %9d  chn %2d  level %08x < %08x\n",
									runCount, audioChannel, (ULWord)(AudioSum[audioChannel] / numberOfAudioSamples), mpCrcData->mAudioLevel);
							}
							if (pLogFile != NULL)
							{
								fprintf(pLogFile, "bad frame %9d chn %2d  level %08x < %08x\n",
									runCount, audioChannel, (ULWord)(AudioSum[audioChannel] / numberOfAudioSamples), mpCrcData->mAudioLevel);
							}
							if (mpCrcData->mVerboseCount > 0)
							{
								iVCount++;
							}
						}
					}

					if ((mpCrcData->mTestCount != 0) && (runCount > mpCrcData->mTestCount))
					{
						break;
					}
				}
			}

			runCount++;

			if (bVideoError)
			{
				mVideoErrorCount++;
				bSyncToReference = true;
				if (pLogFile != NULL)
				{
					fflush(pLogFile);
				}
			}
			else
			{
				if (bVideoErrorInsertionDetection)
					mVideoErrorInsertionCount++;
				bSyncToReference = false;
			}

			if (bAudioError)
				mAudioErrorCount++;
			else
			{
				if (bAudioErrorInsertionDetected)
					mAudioErrorInsertionCount++;
			}

			if (bAncF1Error)
				mAncF1ErrorCount++;

			if (bAncF2Error)
				mAncF2ErrorCount++;
		mAVCircularBuffer.EndConsumeNextBuffer();
		}
	}

	return mbTestError ? AJA_STATUS_FAIL : AJA_STATUS_SUCCESS;
}

void QACrc::GetStatus(ULWord & outFramesProcessed, ULWord & outFramesDropped, ULWord & outBufferLevel, uint32_t & outVideoErrorCount, uint32_t & outAudioErrorCount, uint32_t & outAncF1ErrorCount, uint32_t & outAncF2ErrorCount, uint32_t & outVideoErrorInsertionCount)
{
	AUTOCIRCULATE_STATUS	status;
	mDevice.AutoCirculateGetStatus(mpCrcData->mVideoChannel, status);
	outFramesProcessed = status.acFramesProcessed;
	outFramesDropped = status.acFramesDropped;
	outBufferLevel = status.acBufferLevel;
	outVideoErrorCount = mVideoErrorCount;
	outAudioErrorCount = mAudioErrorCount;
	outAncF1ErrorCount = mAncF1ErrorCount;
	outAncF2ErrorCount = mAncF2ErrorCount;
	outVideoErrorInsertionCount = mVideoErrorInsertionCount;
}

AJAStatus QACrc::CheckAncPacketAndVideo (const uint8_t * pInAnc, uint8_t ** ppInOutVid, const bool bIsField2)
{
	uint8_t	checksum		= 0;
	bool	lineNumberOk	= false;

	//	Scan through the ancillary buffer looking for a qacrc generated packet
	while (1)
	{
		//	Packet must start with 0xFF
		if (*pInAnc != 0xFF)
			break;

		//	Check for our DID
		if (pInAnc[3] == 0xC0)
		{
			if (bIsField2)
			{
				//	SDID of 3 denotes field 2 packet
				if (pInAnc[4] == 3)
				{
					//	Extract line number from scattered header bits
					uint32_t fullLineNumber = (((uint32_t)(pInAnc[1] & 0xF)) << 7) | pInAnc[2];

					//	Line 581 (field 2 line 18)
					if (fullLineNumber == 0x245)
						lineNumberOk = true;
				}
				else
				{
					if (mpCrcData->mbLogAncErrors)
						AJA_OUTPUT("Bad SDID %x\n", pInAnc[4]);

					return AJA_STATUS_UNSUPPORTED;
				}
			}
			else
			{
				//	SDID 1 is Y VANC line 16
				if ((pInAnc[4] == 1) && (pInAnc[2] == 0x10))
				{
					lineNumberOk = true;
				}

				//	SDID 2 is Y VANC line 17
				else if ((pInAnc[4] == 2) && (pInAnc[2] == 0x11))
				{
					lineNumberOk = true;
				}

				else
				{
					if (mpCrcData->mbLogAncErrors)
						AJA_OUTPUT("Bad SDID %x\n", pInAnc[4]);

					return AJA_STATUS_UNSUPPORTED;
				}
			}

			//	Check line number
			if (!lineNumberOk)
			{
				if (mpCrcData->mbLogAncErrors)
					AJA_OUTPUT("Bad line count %x on field %d\n", pInAnc[2], bIsField2 ? 2 : 1);

				return AJA_STATUS_RANGE;
				break;
			}

			//	Payload in Anc packet should match the bytes in the video buffer
			//	Calculate the checksum as we go
			checksum = pInAnc[3] + pInAnc[4] + pInAnc[5];
			for (int i = 0; i < pInAnc[5]; i++)
			{
				if (pInAnc[6 + i] != (*ppInOutVid)[i])
				{
					if (mpCrcData->mbLogAncErrors)
						AJA_OUTPUT("F1 i %d anc %x vid %x\n", i, pInAnc[6 + i], (*ppInOutVid)[i]);

					return AJA_STATUS_FAIL;
				}

				checksum += pInAnc[6 + i];
			}

			//	Verify checksum
			if (checksum != pInAnc[6 + pInAnc[5]])
				return AJA_STATUS_FALSE;

			*ppInOutVid += pInAnc[5];
		}

		//	Skip this packet, and on to the next
		pInAnc += 5 + pInAnc[5] + 2;	//	5 to get to DC, DC itself, 2 for DC and CS 
	}

	return AJA_STATUS_SUCCESS;
}

