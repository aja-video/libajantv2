/**
	@file		ntv2player4k.cpp
	@brief		Implementation of ntv2player4k class.
	@copyright	(C) 2013-2020 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2player8k.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2devicefeatures.h"
#include "ntv2debug.h"
#include "ntv2testpatterngen.h"
#include "ajabase/common/timecode.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"

using namespace std;

#define NTV2_ANCSIZE_MAX	(0x2000)

/**
	@brief	The alignment of the video and audio buffers has a big impact on the efficiency of
			DMA transfers. When aligned to the page size of the architecture, only one DMA
			descriptor is needed per page. Misalignment will double the number of descriptors
			that need to be fetched and processed, thus reducing bandwidth.
**/
static const uint32_t	BUFFER_ALIGNMENT	(4096);		// The correct size for many systems

/**
	@brief	The maximum number of bytes of 48KHz audio that can be transferred for a single frame.
			Worst case, assuming 16 channels of audio (max), 4 bytes per sample, and 67 msec per frame
			(assuming the lowest possible frame rate of 14.98 fps)...
			48,000 samples per second requires 3,204 samples x 4 bytes/sample x 16 = 205,056 bytes
			201K will suffice, with 768 bytes to spare
**/
static const uint32_t	AUDIOBYTES_MAX_48K	(201 * 1024);


NTV2Player8K::NTV2Player8K (const Player8KConfig & config)
	:	mPlayThread					(NULL),
		mProduceFrameThread			(NULL),
		mCurrentFrame				(0),
		mCurrentSample				(0),
		mToneFrequency				(440.0),
		mDeviceSpecifier			(config.fDeviceSpecifier),
		mWithAudio					(config.fWithAudio),
		mUseHDMIOut					(config.fUseHDMIOut),
		mChannel					(config.fChannel),
		mVideoFormat				(config.fVideoFormat),
		mPixelFormat				(config.fPixelFormat),
		mVancMode					(NTV2_VANCMODE_OFF),
		mAudioSystem				(NTV2_AUDIOSYSTEM_1),
		mDoMultiChannel				(config.fDoMultiChannel),
		mDoRGBOnWire				(config.fDoRGBOnWire),
		mTestPatternVideoBuffers	(NULL),
		mInstance					(NULL),
		mPlayerCallback				(NULL),
		mAncType					(config.fSendAncType)
{
	mGlobalQuit = false;
	::memset (mAVHostBuffer, 0, sizeof (mAVHostBuffer));
}


NTV2Player8K::~NTV2Player8K (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit ();

	mDevice.UnsubscribeOutputVerticalEvent (NTV2_CHANNEL1);

	//	Free my threads and buffers...
	delete mPlayThread;
	mPlayThread = NULL;
	delete mProduceFrameThread;
	mProduceFrameThread = NULL;

	if (mTestPatternVideoBuffers)
	{
		for (int32_t ndx = 0;  ndx < mNumTestPatterns;  ndx++)
			AJAMemory::FreeAligned (mTestPatternVideoBuffers [ndx]);
		delete [] mTestPatternVideoBuffers;
		mTestPatternVideoBuffers = NULL;
		mNumTestPatterns = 0;
	}

	for (unsigned int ndx = 0;  ndx < CIRCULAR_BUFFER_SIZE;  ndx++)
	{
		//	Note that the fVideoBuffer members point to test patterns, which have already been deleted above
		if (mAVHostBuffer [ndx].fAudioBuffer)
		{
			AJAMemory::FreeAligned (mAVHostBuffer [ndx].fAudioBuffer);
			mAVHostBuffer [ndx].fAudioBuffer = NULL;
		}
	}	//	for each buffer in the ring

	mDevice.SetEveryFrameServices (NTV2_STANDARD_TASKS);										//	Restore the saved service level
	mDevice.ReleaseStreamForApplication (AJA_FOURCC ('D','E','M','O'), static_cast<int32_t>(AJAProcess::GetPid()));	//	Release the device

}	//	destructor


void NTV2Player8K::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	if (mProduceFrameThread)
		while (mProduceFrameThread->Active ())
			AJATime::Sleep (10);

	if (mPlayThread)
		while (mPlayThread->Active ())
			AJATime::Sleep (10);

	mDevice.DMABufferUnlockAll();
}	//	Quit


AJAStatus NTV2Player8K::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	//	Keep the board ID handy, as it will be used frequently...
	mDeviceID = mDevice.GetDeviceID();

	//	Check for an invalid configuration
	if (NTV2_IS_4K_HFR_VIDEO_FORMAT (mVideoFormat) && mDoRGBOnWire)
		{cerr << "## ERROR:  High frame rate RGB output is not supported" << endl;  return AJA_STATUS_BAD_PARAM;}

	if (UWord (mChannel) >= ::NTV2DeviceGetNumFrameStores (mDeviceID))
	{
		cerr	<< "## ERROR:  Cannot use channel '" << mChannel+1 << "' -- device only supports channel 1"
				<< (::NTV2DeviceGetNumFrameStores (mDeviceID) > 1  ?  string (" thru ") + string (1, uint8_t (::NTV2DeviceGetNumFrameStores (mDeviceID)+'0'))  :  "") << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	mChannel = (mChannel < NTV2_CHANNEL5)? NTV2_CHANNEL1 : NTV2_CHANNEL5;

	mAudioSystem = ::NTV2ChannelToAudioSystem (mChannel);

	if (!mDoMultiChannel)
	{
		if (!mDevice.AcquireStreamForApplication (AJA_FOURCC ('D','E','M','O'), static_cast<int32_t>(AJAProcess::GetPid())))
			return AJA_STATUS_BUSY;		//	Device is in use by another app -- fail
		mDevice.GetEveryFrameServices (mPreviousFrameServices);	//	Save the current service level
	}
	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);				//	Set OEM service level

	if (::NTV2DeviceCanDoMultiFormat (mDeviceID))
		mDevice.SetMultiFormatMode (true);

	//	Set up the video and audio...
	status = SetUpVideo ();
	if (AJA_FAILURE (status))
		return status;

	status = SetUpAudio ();
	if (AJA_FAILURE (status))
		return status;

	//	Set up the circular buffers, the device signal routing, playout AutoCirculate, and the test pattern buffers...
	SetUpHostBuffers ();
	RouteOutputSignal ();
	SetUpTestPatternVideoBuffers ();

	//	Lastly, prepare my AJATimeCodeBurn instance...
	NTV2FormatDescriptor fd (mVideoFormat, mPixelFormat, mVancMode);
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat (mPixelFormat), fd.numPixels, fd.numLines);

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Player8K::SetUpVideo ()
{
	//	Unless a video format was requested, configure the board for ...
 	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_BAD_PARAM;

	if (!::NTV2DeviceCanDoVideoFormat (mDeviceID, mVideoFormat))
		{cerr << "## ERROR:  This device cannot handle '" << ::NTV2VideoFormatToString (mVideoFormat) << "'" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Configure the device to handle the requested video format...
	mDevice.SetVideoFormat (mVideoFormat, false, false, mChannel);
	mDevice.SetQuadQuadSquaresEnable(true);

	//	VANC data is not processed by this application
	mDevice.SetEnableVANCData (false, false);

	//	Configure output for HFR Level A and RGB Level B
	const NTV2Channel	startChannel	(mChannel);
	const NTV2Channel	endChannel		(mChannel == NTV2_CHANNEL1 ? NTV2_CHANNEL4 : NTV2_CHANNEL8);

	for (NTV2Channel chan (startChannel); chan < endChannel; chan = NTV2Channel (chan + 1))
	{
		mDevice.SetSDIOutLevelAtoLevelBConversion (chan, false);
		mDevice.SetSDIOutRGBLevelAConversion (chan, false);
	}

	//	Set the frame buffer pixel format for all the channels on the device.
	//	If the device doesn't support it, fall back to 8-bit YCbCr...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mPixelFormat))
		mPixelFormat = NTV2_FBF_8BIT_YCBCR;

	if (mChannel == NTV2_CHANNEL1)
	{
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mPixelFormat);
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mPixelFormat);
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mPixelFormat);
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mPixelFormat);
		mDevice.EnableChannel (NTV2_CHANNEL1);
		mDevice.EnableChannel (NTV2_CHANNEL2);
		mDevice.EnableChannel (NTV2_CHANNEL3);
		mDevice.EnableChannel (NTV2_CHANNEL4);
		mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL1);
	}
	else
	{
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL5, mPixelFormat);
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL6, mPixelFormat);
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL7, mPixelFormat);
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL8, mPixelFormat);
		mDevice.EnableChannel (NTV2_CHANNEL5);
		mDevice.EnableChannel (NTV2_CHANNEL6);
		mDevice.EnableChannel (NTV2_CHANNEL7);
		mDevice.EnableChannel (NTV2_CHANNEL8);
		mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL5);
	}

	mDevice.SetReference (NTV2_REFERENCE_FREERUN);

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2Player8K::SetUpAudio ()
{
	uint16_t	numberOfAudioChannels	(::NTV2DeviceGetMaxAudioChannels (mDeviceID));

	//	If there are 4096 pixels on a line instead of 3840, reduce the number of audio channels
	//	This is because HANC is narrower, and has space for only 8 channels
	if (NTV2_IS_4K_4096_VIDEO_FORMAT (mVideoFormat) && (numberOfAudioChannels > 8))
	{
		numberOfAudioChannels = 8;
	}

	mDevice.SetNumberAudioChannels (numberOfAudioChannels, mAudioSystem);
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL1, mAudioSystem);
	mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL2, mAudioSystem);
	mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL3, mAudioSystem);
	mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL4, mAudioSystem);

	//	If the last app using the device left it in end-to-end mode (input passthru),
	//	then loopback must be disabled, or else the output will contain whatever audio
	//	is present in whatever signal is feeding the device's SDI input...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio


void NTV2Player8K::SetUpHostBuffers ()
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	//	Calculate the size of the video buffer, which depends on video format, pixel format, and whether VANC is included or not...
	mVideoBufferSize = GetVideoWriteSize (mVideoFormat, mPixelFormat, mVancMode);
	mAudioBufferSize = AUDIOBYTES_MAX_48K;

	//	Allocate my buffers...
	for (unsigned int ndx = 0; ndx < CIRCULAR_BUFFER_SIZE; ndx++)
	{
		//	The video buffer address will be filled in by the producer thread
		mAVHostBuffer [ndx].fVideoBuffer		= NULL;
		mAVHostBuffer [ndx].fVideoBufferSize	= mVideoBufferSize;
		mAVHostBuffer [ndx].fAudioBuffer		= mWithAudio
													? reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (mAudioBufferSize, BUFFER_ALIGNMENT))
													: NULL;
		mAVHostBuffer [ndx].fAudioBufferSize	= mWithAudio ? mAudioBufferSize : 0;

		memset (mAVHostBuffer [ndx].fAudioBuffer, 0x00, mWithAudio ? mAudioBufferSize : 0);

		if (mAVHostBuffer [ndx].fAudioBuffer != NULL)
			mDevice.DMABufferLock(mAVHostBuffer [ndx].fAudioBuffer, mAudioBufferSize);

		mAVCircularBuffer.Add (&mAVHostBuffer [ndx]);
	}
}	//	SetUpHostBuffers


void NTV2Player8K::RouteOutputSignal (void)
{
	const bool	isRGB	(::IsRGBFormat (mPixelFormat));

	if (!mDoMultiChannel)
		mDevice.ClearRouting ();	//	Replace current signal routing

	//	Construct switch value to avoid multiple if-then-else
	int switchValue = 0;
	if (NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT (mVideoFormat))
		switchValue += 8;
	if (isRGB)
		switchValue += 2;
	if (mDoRGBOnWire)
		switchValue += 1;

	switch (switchValue)
	{
		case 0:		//	Low Frame Rate, Square, Pixel YCbCr, Wire YCbCr
			RouteFsToSDIOut ();
			break;
		case 1:		//	Low Frame Rate, Square, Pixel YCbCr, Wire RGB
			RouteFsToCsc ();
			RouteCscToDLOut ();
			RouteDLOutToSDIOut ();
			break;
		case 2:		//	Low Frame Rate, Square, Pixel RGB, Wire YCbCr
			RouteFsToCsc ();
			RouteCscToSDIOut ();
			break;
		case 3:		//	Low Frame Rate, Square, Pixel RGB, Wire RGB
			RouteFsToDLOut ();
			RouteDLOutToSDIOut ();
			break;
		case 8:		//	High Frame Rate, Square, Pixel YCbCr, Wire YCbCr
			RouteFsToSDIOut ();
			break;
		case 9:		//	High Frame Rate, Square, Pixel YCbCr, Wire RGB
			//	No valid routing for this case
			break;
		case 10:	//	High Frame Rate, Square, Pixel RGB, Wire YCbCr
			RouteFsToCsc ();
			RouteCscToSDIOut ();
			break;
		case 11:	//	High Frame Rate, Square, Pixel RGB, Wire RGB
			//	No valid routing for this case
			break;
		case 15:	//	High Frame Rate, Tsi, Pixel RGB, Wire RGB
			//	No valid routing for this case
			break;
		default:
			return;
	}

	mDevice.SetTsiFrameEnable  (true,  mChannel);

	//	Send signal to secondary outputs, if supported
	Route4KDownConverter ();
	RouteHDMIOutput ();

	//	Enable SDI output from all channels,
	//	but only if the device supports bi-directional SDI.
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
	mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);

}	//	RouteOutputSignal


void NTV2Player8K::Route4KDownConverter (void)
{
	const bool			isRGB			(::IsRGBFormat (mPixelFormat));

	if ( ! ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt4KDownConverter) || ! ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIMonOut1))
		return;

	if (isRGB)
	{
		mDevice.Enable4KDCRGBMode(true);
		mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer1RGB);
		mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer2RGB);
		mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer3RGB);
		mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer4RGB);
		mDevice.Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut);
		mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV);
	}
	else	//	!RGB
	{
		mDevice.Enable4KDCRGBMode (false);
		mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer1YUV);
		mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer2YUV);
		mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer3YUV);
		mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer4YUV);
		mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut);
	}

}	//	Route4KDownConverter


void NTV2Player8K::RouteHDMIOutput (void)
{
	const bool			isRGB			(::IsRGBFormat (mPixelFormat));

	if (mUseHDMIOut &&
		(::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v2)
			|| ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v3)
			|| ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v4)) )
	{
		if (isRGB)
		{
			mDevice.Connect (NTV2_XptCSC1VidInput,		NTV2_XptFrameBuffer1RGB);
			mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptCSC1VidYUV);

			mDevice.Connect (NTV2_XptCSC2VidInput,		NTV2_XptFrameBuffer2RGB);
			mDevice.Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptCSC2VidYUV);

			mDevice.Connect (NTV2_XptCSC3VidInput,		NTV2_XptFrameBuffer3RGB);
			mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptCSC3VidYUV);

			mDevice.Connect (NTV2_XptCSC4VidInput,		NTV2_XptFrameBuffer4RGB);
			mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptCSC4VidYUV);
		}
		else
		{
			mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptFrameBuffer1YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_XptFrameBuffer2YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptFrameBuffer3YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptFrameBuffer4YUV);
		}

		mDevice.SetHDMIV2TxBypass (false);
		mDevice.SetHDMIOutVideoStandard (::GetNTV2StandardFromVideoFormat (mVideoFormat));
		mDevice.SetHDMIOutVideoFPS (::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
		mDevice.SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
		mDevice.SetHDMIV2Mode (NTV2_HDMI_V2_4K_PLAYBACK);
	}
	else
		mDevice.SetHDMIV2Mode (NTV2_HDMI_V2_4K_PLAYBACK);

}	//	RouteHDMIOutput


void NTV2Player8K::RouteFsToDLOut (void)
{
	mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
	mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptFrameBuffer2RGB);
	mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer3RGB);
	mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptFrameBuffer4RGB);
}	//	RouteFsToDLOut


void NTV2Player8K::RouteFsToCsc (void)
{
	if (::IsRGBFormat (mPixelFormat))
	{
		mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_XptFrameBuffer1RGB);
		mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_XptFrameBuffer2RGB);
		mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_XptFrameBuffer3RGB);
		mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_XptFrameBuffer4RGB);
	}
	else
	{
		mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_XptFrameBuffer1YUV);
		mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_XptFrameBuffer2YUV);
		mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_XptFrameBuffer3YUV);
		mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_XptFrameBuffer4YUV);
	}
}	//	RouteFsToCsc


void NTV2Player8K::RouteFsToSDIOut (void)
{
	mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptFrameBuffer1YUV);
	mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptFrameBuffer2YUV);
	mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptFrameBuffer3YUV);
	mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptFrameBuffer4YUV);
}	//	RouteFsToSDIOut


void NTV2Player8K::RouteDLOutToSDIOut (void)
{
	mDevice.Connect (NTV2_XptSDIOut1Input,		NTV2_XptDuallinkOut1);
	mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_XptDuallinkOut1DS2);
	mDevice.Connect (NTV2_XptSDIOut2Input,		NTV2_XptDuallinkOut2);
	mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_XptDuallinkOut2DS2);
	mDevice.Connect (NTV2_XptSDIOut3Input,		NTV2_XptDuallinkOut3);
	mDevice.Connect (NTV2_XptSDIOut3InputDS2,	NTV2_XptDuallinkOut3DS2);
	mDevice.Connect (NTV2_XptSDIOut4Input,		NTV2_XptDuallinkOut4);
	mDevice.Connect (NTV2_XptSDIOut4InputDS2,	NTV2_XptDuallinkOut4DS2);
}	//	RouteFsDLOutToSDIOut


void NTV2Player8K::RouteCscToSDIOut (void)
{
	mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptCSC1VidYUV);
	mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptCSC2VidYUV);
	mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptCSC3VidYUV);
	mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptCSC4VidYUV);
}	//	RouteCscToSDIOut


void NTV2Player8K::RouteCscToDLOut (void)
{
	mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptCSC1VidRGB);
	mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptCSC2VidRGB);
	mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptCSC3VidRGB);
	mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptCSC4VidRGB);
}	//	RouteCscToDLOut

AJAStatus NTV2Player8K::Run ()
{
	//	Start the playout and producer threads...
	StartPlayThread ();
	StartProduceFrameThread ();

	return AJA_STATUS_SUCCESS;
}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2Player8K::StartPlayThread ()
{
	//	Create and start the playout thread...
	mPlayThread = new AJAThread ();
	mPlayThread->Attach (PlayThreadStatic, this);
	mPlayThread->SetPriority (AJA_ThreadPriority_High);
	mPlayThread->Start ();

}	//	StartPlayThread


//	The playout thread function
void NTV2Player8K::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Player4K instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2Player8K *	pApp	(reinterpret_cast <NTV2Player8K *> (pContext));
	pApp->PlayFrames ();

}	//	PlayThreadStatic


void NTV2Player8K::PlayFrames (void)
{
	uint8_t	numberOfACFramesPerChannel	(7);

	mDevice.AutoCirculateStop (mChannel);	//	Just in case someone else left it running

	mDevice.WaitForOutputFieldID (NTV2_FIELD0, mChannel);
	mDevice.WaitForOutputFieldID (NTV2_FIELD0, mChannel);

	uint32_t*	fAncBuffer = mAncType != AJAAncillaryDataType_Unknown ? reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (NTV2_ANCSIZE_MAX, AJA_PAGE_SIZE)) : NULL;
	uint32_t	fAncBufferSize = mAncType != AJAAncillaryDataType_Unknown ? NTV2_ANCSIZE_MAX : 0;
	::memset((void*)fAncBuffer, 0x00, fAncBufferSize);

	if (fAncBuffer != NULL)
		mDevice.DMABufferLock(fAncBuffer, fAncBufferSize);

	uint32_t	packetSize = 0;
	switch(mAncType)
	{
	case AJAAncillaryDataType_HDR_SDR:
	{
		AJAAncillaryData_HDR_SDR sdrPacket;
		sdrPacket.GenerateTransmitData((uint8_t*)fAncBuffer, fAncBufferSize, packetSize);
		break;
	}
	case AJAAncillaryDataType_HDR_HDR10:
	{
		AJAAncillaryData_HDR_HDR10 hdr10Packet;
		hdr10Packet.GenerateTransmitData((uint8_t*)fAncBuffer, fAncBufferSize, packetSize);
		break;
	}
	case AJAAncillaryDataType_HDR_HLG:
	{
		AJAAncillaryData_HDR_HLG hlgPacket;
		hlgPacket.GenerateTransmitData((uint8_t*)fAncBuffer, fAncBufferSize, packetSize);
		break;
	}
	default:	break;
	}

	//	Initialize & start AutoCirculate...

	const uint8_t	startNum	(mChannel < 4	?								0	:	numberOfACFramesPerChannel);		//	Ch1: frames 0-6
	const uint8_t	endNum		(mChannel < 4	?	numberOfACFramesPerChannel-1	:	numberOfACFramesPerChannel*2-1);	//	Ch5: frames 7-13
	mDevice.AutoCirculateInitForOutput (mChannel, 0, mAudioSystem, AUTOCIRCULATE_WITH_RP188 | AUTOCIRCULATE_WITH_ANC,
										1 /*numChannels*/, startNum,  endNum);
	mDevice.AutoCirculateStart (mChannel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS	outputStatus;
		mDevice.AutoCirculateGetStatus (mChannel, outputStatus);

		//	Check if there's room for another frame on the card...
		if (outputStatus.CanAcceptMoreOutputFrames ())
		{
			//	Wait for the next frame to become ready to "consume"...
			AVDataBuffer *	playData	(mAVCircularBuffer.StartConsumeNextBuffer ());

			//	Burn the current timecode into the test pattern image that's now in my video buffer...
			const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat (mVideoFormat));
			const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
			const	CRP188			rp188Info		(mCurrentFrame++, 0, 0, 10, tcFormat);
			string					timeCodeString;

			rp188Info.GetRP188Reg (playData->fRP188Data);
			rp188Info.GetRP188Str (timeCodeString);

			mTCBurner.BurnTimeCode (reinterpret_cast <char *> (playData->fVideoBuffer), timeCodeString.c_str (), 80);

			//	Transfer the timecode-burned frame to the device for playout...
			mOutputTransferStruct.acVideoBuffer.Set (playData->fVideoBuffer, playData->fVideoBufferSize);
			mOutputTransferStruct.acAudioBuffer.Set (playData->fAudioBuffer, playData->fAudioBufferSize);
			mOutputTransferStruct.acANCBuffer.Set(fAncBuffer, NTV2_ANCSIZE_MAX);
            mOutputTransferStruct.SetOutputTimeCode (NTV2_RP188 (playData->fRP188Data), NTV2ChannelToTimecodeIndex (mChannel, false, false));
            mOutputTransferStruct.SetOutputTimeCode (NTV2_RP188 (playData->fRP188Data), NTV2ChannelToTimecodeIndex (mChannel, true, false));

			mDevice.AutoCirculateTransfer (mChannel, mOutputTransferStruct);

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer ();
		}
		else
			mDevice.WaitForOutputVerticalInterrupt (mChannel);
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop (mChannel);

}	//	PlayFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2Player8K::StartProduceFrameThread ()
{
	//	Create and start the producer thread...
	mProduceFrameThread = new AJAThread ();
	mProduceFrameThread->Attach (ProduceFrameThreadStatic, this);
	mProduceFrameThread->SetPriority (AJA_ThreadPriority_High);
	mProduceFrameThread->Start ();

}	//	StartProduceFrameThread


void NTV2Player8K::ProduceFrameThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	NTV2Player8K *	pApp	(reinterpret_cast <NTV2Player8K *> (pContext));
	pApp->ProduceFrames ();

}	//	ProduceFrameThreadStatic


void NTV2Player8K::SetUpTestPatternVideoBuffers (void)
{
	NTV2TestPatternSelect	testPatternTypes []	=	{NTV2_TestPatt_ColorBars100,
													NTV2_TestPatt_ColorBars75,
													NTV2_TestPatt_Ramp,
													NTV2_TestPatt_MultiBurst,
													NTV2_TestPatt_LineSweep,
													NTV2_TestPatt_CheckField,
													NTV2_TestPatt_FlatField,
													NTV2_TestPatt_MultiPattern,
													NTV2_TestPatt_Black,
													NTV2_TestPatt_White,
													NTV2_TestPatt_Border,
													NTV2_TestPatt_LinearRamp,
													NTV2_TestPatt_SlantRamp,
													NTV2_TestPatt_ZonePlate,
													NTV2_TestPatt_ColorQuadrant,
													NTV2_TestPatt_ColorQuadrantBorder};
	mNumTestPatterns = sizeof (testPatternTypes) / sizeof (NTV2TestPatternSelect);
	mTestPatternVideoBuffers = new uint8_t * [mNumTestPatterns];

	//	Set up one video buffer for each of the several predefined patterns...
	for (int testPatternIndex = 0; testPatternIndex < mNumTestPatterns; testPatternIndex++)
	{
		//	Allocate the buffer memory...
		mTestPatternVideoBuffers [testPatternIndex] = reinterpret_cast <uint8_t *>
													  (AJAMemory::AllocateAligned (mVideoBufferSize, BUFFER_ALIGNMENT));

		// Page lock the memory
		if (mTestPatternVideoBuffers [testPatternIndex] != NULL)
			mDevice.DMABufferLock((ULWord*)mTestPatternVideoBuffers [testPatternIndex], mVideoBufferSize);

		//	Use the test pattern generator to fill an NTV2TestPatternBuffer...
		NTV2TestPatternBuffer	testPatternBuffer;
		NTV2TestPatternGen		testPatternGen;
		NTV2FormatDescriptor	formatDesc	(mVideoFormat, mPixelFormat, mVancMode);

		testPatternGen.DrawTestPattern (testPatternTypes [testPatternIndex],
										formatDesc.numPixels,
										formatDesc.numLines,
										mPixelFormat,
										testPatternBuffer);

		//	Copy the contents of the test pattern buffer into my 'C' array, for quick "memcpy" into each frame...
		const size_t	testPatternSize		(testPatternBuffer.size ());
		uint8_t * const	pVideoBuffer		(mTestPatternVideoBuffers [testPatternIndex]);
		for (size_t ndx = 0; ndx < testPatternSize; ndx++)
			pVideoBuffer [ndx] = testPatternBuffer [ndx];

	}	//	loop for each predefined pattern

}	//	SetUpTestPatternVideoBuffers


void NTV2Player8K::ProduceFrames (void)
{

	const double	frequencies [] =	{250.0, 500.0, 1000.0, 2000.0};
	double			timeOfLastSwitch	(0.0);
	const ULWord	numFrequencies		(sizeof (frequencies) / sizeof (double));
	ULWord			frequencyIndex		(0);
	ULWord			testPatternIndex	(0);

	AJATimeBase	timeBase (CNTV2DemoCommon::GetAJAFrameRate (GetNTV2FrameRateFromVideoFormat (mVideoFormat)));

	while (!mGlobalQuit)
	{
		AVDataBuffer *	frameData	(mAVCircularBuffer.StartProduceNextBuffer ());

		//  If no frame is available, wait and try again
		if (!frameData)
		{
			AJATime::Sleep (10);
			continue;
		}

		if (mPlayerCallback)
		{
			// Get the frame to play from whomever requested the callback
			mPlayerCallback (mInstance, frameData);
		}
		else
		{
			//	Set the video buffer pointer to the test pattern to display
			frameData->fVideoBuffer = (uint32_t*) mTestPatternVideoBuffers [testPatternIndex];

			//	Generate audio tone data...
			frameData->fAudioBufferSize		= mWithAudio ? AddTone (frameData->fAudioBuffer) : 0;

			//	Every few seconds, change the test pattern and tone frequency...
			const double	currentTime	(timeBase.FramesToSeconds (mCurrentFrame));
			if (currentTime > timeOfLastSwitch + 4.0)
			{
				frequencyIndex = (frequencyIndex + 1) % numFrequencies;
				testPatternIndex = (testPatternIndex + 1) % mNumTestPatterns;
				mToneFrequency = frequencies [frequencyIndex];
				timeOfLastSwitch = currentTime;
			}	//	if time to switch test pattern & tone frequency
		}

		//	Signal that I'm done producing the buffer -- it's now available for playout...
		mAVCircularBuffer.EndProduceNextBuffer ();

	}	//	loop til mGlobalQuit goes true

}	//	ProduceFrames


void NTV2Player8K::GetACStatus (AUTOCIRCULATE_STATUS & outputStatus)
{
	mDevice.AutoCirculateGetStatus (mChannel, outputStatus);
}


uint32_t NTV2Player8K::AddTone (ULWord * audioBuffer)
{
	NTV2FrameRate	frameRate;
	NTV2AudioRate	audioRate;
	ULWord			numChannels;

	mDevice.GetFrameRate (frameRate, mChannel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);

	/**
		Because audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will necessarily
		result in some frames having more audio samples than others. The GetAudioSamplesPerFrame function
		is used to calculate the correct sample count...
	**/
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mCurrentFrame));
	const double	audioSampleRate	(audioRate == NTV2_AUDIO_96K ? 96000.0 : 48000.0);

	return ::AddAudioTone (audioBuffer,			//	audio buffer to fill
							mCurrentSample,		//	which sample for continuing the waveform
							numSamples,			//	number of samples to generate
							audioSampleRate,	//	sample rate [Hz]
							0.1,				//	amplitude
							mToneFrequency,		//	tone frequency [Hz]
							31,					//	bits per sample
							false,				//	don't byte swap
							numChannels);		//	number of audio channels to generate
}	//	AddTone


ULWord NTV2Player8K::GetRP188RegisterForOutput (const NTV2OutputDestination inOutputDest)		//	static
{
	switch (inOutputDest)
	{
		case NTV2_OUTPUTDESTINATION_SDI1:	return kRegRP188InOut1DBB;	break;	//	reg 29
		case NTV2_OUTPUTDESTINATION_SDI2:	return kRegRP188InOut2DBB;	break;	//	reg 64
		case NTV2_OUTPUTDESTINATION_SDI3:	return kRegRP188InOut3DBB;	break;	//	reg 268
		case NTV2_OUTPUTDESTINATION_SDI4:	return kRegRP188InOut4DBB;	break;	//	reg 273
		default:							return 0;					break;
	}	//	switch on output destination

}	//	GetRP188RegisterForOutput


void NTV2Player8K::GetCallback (void ** const pInstance, NTV2Player8KCallback ** const callback)
{
	*pInstance = mInstance;
	*callback = mPlayerCallback;
}	//	GetCallback


bool NTV2Player8K::SetCallback (void * const pInstance, NTV2Player8KCallback * const callback)
{
	mInstance = pInstance;
	mPlayerCallback = callback;

	return true;
}	//	SetCallback

