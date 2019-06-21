/**
	@file		ntv2player4k.cpp
	@brief		Implementation of ntv2player4k class.
	@copyright	Copyright (C) 2013-2019 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ntv2player4k.h"
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
#include "ajabase/system/info.h"

using namespace std;

#define					AS_INT32(_x_)		static_cast<int32_t>(_x_)
#define					APP_PROCESS_ID		AS_INT32(AJAProcess::GetPid())

//	Convenience macros for EZ logging:
#define	PLFAIL(_xpr_)	AJA_sERROR  (AJA_DebugUnit_Application, AJAFUNC << ": " << _xpr_)
#define	PLWARN(_xpr_)	AJA_sWARNING(AJA_DebugUnit_Application, AJAFUNC << ": " << _xpr_)
#define	PLNOTE(_xpr_)	AJA_sNOTICE	(AJA_DebugUnit_Application, AJAFUNC << ": " << _xpr_)
#define	PLINFO(_xpr_)	AJA_sINFO	(AJA_DebugUnit_Application, AJAFUNC << ": " << _xpr_)
#define	PLDBG(_xpr_)	AJA_sDEBUG	(AJA_DebugUnit_Application, AJAFUNC << ": " << _xpr_)

static const size_t		NTV2_ANCSIZE_MAX	(0x2000);
static const uint32_t	APP_SIGNATURE		(NTV2_FOURCC('D','E','M','O'));

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


NTV2Player4K::NTV2Player4K (const Player4KConfig & inConfig)
	:	mConfig				(inConfig),
		mConsumerThread		(AJA_NULL),
		mProducerThread		(AJA_NULL),
		mCurrentFrame		(0),
		mCurrentSample		(0),
		mToneFrequency		(440.0),
		mTestPatternBuffers	(AJA_NULL)
{
	mGlobalQuit = false;
	::memset (mHostBuffers, 0, sizeof(mHostBuffers));
}


NTV2Player4K::~NTV2Player4K (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit();

	mDevice.UnsubscribeOutputVerticalEvent(NTV2_CHANNEL1);

	//	Free my threads and buffers...
	delete mConsumerThread;
	mConsumerThread = AJA_NULL;
	delete mProducerThread;
	mProducerThread = AJA_NULL;

	if (mTestPatternBuffers)
	{
		for (uint32_t ndx(0);  ndx < mNumTestPatterns;  ndx++)
			AJAMemory::FreeAligned(mTestPatternBuffers[ndx]);
		delete [] mTestPatternBuffers;
		mTestPatternBuffers = AJA_NULL;
		mNumTestPatterns = 0;
	}

	for (uint32_t ndx(0);  ndx < CIRCULAR_BUFFER_SIZE;  ndx++)
	{
		//	Note that the fVideoBuffer members point to test patterns, which have already been deleted above
		if (mHostBuffers[ndx].fAudioBuffer)
		{
			AJAMemory::FreeAligned (mHostBuffers[ndx].fAudioBuffer);
			mHostBuffers[ndx].fAudioBuffer = AJA_NULL;
		}
	}	//	for each buffer in the ring

	mDevice.SetEveryFrameServices(mSavedTaskMode);	//	Restore the saved service level
	mDevice.ReleaseStreamForApplication (APP_SIGNATURE, APP_PROCESS_ID);	//	Release the device

}	//	destructor


void NTV2Player4K::Quit (void)
{
	//	Set the global 'quit' flag, and wait for the threads to go inactive...
	mGlobalQuit = true;

	if (mProducerThread)
		while (mProducerThread->Active())
			AJATime::Sleep(10);

	if (mConsumerThread)
		while (mConsumerThread->Active())
			AJATime::Sleep(10);

}	//	Quit


AJAStatus NTV2Player4K::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mConfig.fDeviceSpecifier, mDevice))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;}

    if (!mDevice.IsDeviceReady (false))
		{cerr << "## ERROR:  Device '" << mConfig.fDeviceSpecifier << "' not ready" << endl;  return AJA_STATUS_INITIALIZE;}

	//	Keep the board ID handy, as it will be used frequently...
	mDeviceID = mDevice.GetDeviceID();
	const UWord maxNumChannels (::NTV2DeviceGetNumFrameStores(mDeviceID));

	//	Check for an invalid configuration
	if (NTV2_IS_4K_HFR_VIDEO_FORMAT(mConfig.fVideoFormat)  &&  mConfig.fDoRGBOnWire)
		{cerr << "## ERROR:  HFR RGB output not supported" << endl;  return AJA_STATUS_BAD_PARAM;}

	if (UWord(mConfig.fOutputChannel) >= maxNumChannels)
	{
		cerr	<< "## ERROR:  Cannot use channel '" << DEC(mConfig.fOutputChannel+1) << "' -- device only supports channel"
				<< (maxNumChannels > 1 ? "(s) 1 thru " : "1");
		if (maxNumChannels > 1)	cerr << DEC(maxNumChannels) << endl; else cerr << endl;
		return AJA_STATUS_UNSUPPORTED;
	}

	if (::NTV2DeviceCanDo12gRouting(mDeviceID))
		mConfig.fDoTsiRouting = false;	//	Kona5_12G/Corvid44_12G makes TSI routing much easier
	else if (mConfig.fDoTsiRouting)
		switch (mConfig.fOutputChannel)
		{
			default:				mConfig.fOutputChannel = NTV2_CHANNEL1;		break;

			case NTV2_CHANNEL3:
			case NTV2_CHANNEL4:		mConfig.fOutputChannel = NTV2_CHANNEL3;		break;

			case NTV2_CHANNEL5:
			case NTV2_CHANNEL6:		mConfig.fOutputChannel = NTV2_CHANNEL5;		break;

			case NTV2_CHANNEL7:
			case NTV2_CHANNEL8:		mConfig.fOutputChannel = NTV2_CHANNEL7;		break;
		}
	else if (mConfig.fOutputChannel < NTV2_CHANNEL5)
		mConfig.fOutputChannel = NTV2_CHANNEL1;
	else
		mConfig.fOutputChannel = NTV2_CHANNEL5;

	if (mConfig.WithAudio())
		mConfig.fAudioSystem = ::NTV2ChannelToAudioSystem(mConfig.fOutputChannel);

	if (!mConfig.fDoMultiChannel)
	{
		if (!mDevice.AcquireStreamForApplication (APP_SIGNATURE, APP_PROCESS_ID))
			return AJA_STATUS_BUSY;		//	Device is in use by another app -- fail
		mDevice.GetEveryFrameServices(mSavedTaskMode);	//	Save the current service level

        mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);		//	Set OEM service level
        mDevice.SetMultiFormatMode(false);
        for (NTV2Channel chan (NTV2_CHANNEL1); chan < NTV2_CHANNEL8; chan = NTV2Channel (chan + 1))
        {
            mDevice.DisableChannel (chan);
        }
	}
    else
    {
        mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);		//	Set OEM service level
        if (::NTV2DeviceCanDoMultiFormat(mDeviceID))
            mDevice.SetMultiFormatMode(true);
    }

	//	Set up the video and audio...
	status = SetUpVideo();
	if (AJA_FAILURE(status))
		return status;

	status = SetUpAudio ();
	if (AJA_FAILURE(status))
		return status;

	//	Set up the circular buffers, the device signal routing, playout AutoCirculate, and the test pattern buffers...
	SetUpHostBuffers();
	RouteOutputSignal();
	SetUpTestPatternVideoBuffers();

	//	Lastly, prepare my AJATimeCodeBurn instance...
	NTV2FormatDescriptor fd (mConfig.fVideoFormat, mConfig.fPixelFormat);
	mTCBurner.RenderTimeCodeFont (CNTV2DemoCommon::GetAJAPixelFormat(mConfig.fPixelFormat), fd.numPixels, fd.numLines);
	PLINFO("Configuration: " << mConfig);

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2Player4K::SetUpVideo (void)
{
	//	Unless a video format was requested, configure the board for ...
 	if (mConfig.fVideoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_BAD_PARAM;

	if (!::NTV2DeviceCanDoVideoFormat (mDeviceID, mConfig.fVideoFormat))
		{cerr << "## ERROR:  Device can't do " << ::NTV2VideoFormatToString(mConfig.fVideoFormat) << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Configure output for HFR Level A and RGB Level B
	if (!::NTV2DeviceCanDo12gRouting(mDeviceID))
	{
		const NTV2Channel	startChannel	(mConfig.fOutputChannel);
		const NTV2Channel	endChannel		(mConfig.fOutputChannel == NTV2_CHANNEL1 ? NTV2_CHANNEL4 : NTV2_CHANNEL8);

		for (NTV2Channel chan (startChannel); chan < endChannel; chan = NTV2Channel (chan + 1))
		{
            //	Configure the device to handle the requested video format...
            mDevice.SetVideoFormat (mConfig.fVideoFormat, false, false, chan);
            //	VANC data is not processed by this application
            mDevice.SetEnableVANCData (false, false, chan);
            //  Disable SDI output conversions
            mDevice.SetSDIOutLevelAtoLevelBConversion (chan, false);
			mDevice.SetSDIOutRGBLevelAConversion (chan, false);
		}

	}

	//	Set the frame buffer pixel format for all the channels on the device.
	//	If the device doesn't support it, fall back to 8-bit YCbCr...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.fPixelFormat))
		mConfig.fPixelFormat = NTV2_FBF_8BIT_YCBCR;

	if (::NTV2DeviceCanDo12gRouting(mDeviceID))
	{
		mDevice.SetFrameBufferFormat (mConfig.fOutputChannel, mConfig.fPixelFormat);
		mDevice.EnableChannel (mConfig.fOutputChannel);
		mDevice.SubscribeOutputVerticalEvent (mConfig.fOutputChannel);
	}
	else if (mConfig.fDoTsiRouting)
	{
		if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mConfig.fPixelFormat);
			mDevice.EnableChannel (NTV2_CHANNEL1);
			mDevice.EnableChannel (NTV2_CHANNEL2);
			mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL1);
		}
		else if (mConfig.fOutputChannel == NTV2_CHANNEL3)
		{
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mConfig.fPixelFormat);
			mDevice.EnableChannel (NTV2_CHANNEL3);
			mDevice.EnableChannel (NTV2_CHANNEL4);
			mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL3);
		}
		else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
		{
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL5, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL6, mConfig.fPixelFormat);
			mDevice.EnableChannel (NTV2_CHANNEL5);
			mDevice.EnableChannel (NTV2_CHANNEL6);
			mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL5);
		}
		else
		{
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL7, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL8, mConfig.fPixelFormat);
			mDevice.EnableChannel (NTV2_CHANNEL7);
			mDevice.EnableChannel (NTV2_CHANNEL8);
			mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL7);
		}
	}
	else
	{
		if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mConfig.fPixelFormat);
			mDevice.EnableChannel (NTV2_CHANNEL1);
			mDevice.EnableChannel (NTV2_CHANNEL2);
			mDevice.EnableChannel (NTV2_CHANNEL3);
			mDevice.EnableChannel (NTV2_CHANNEL4);
			mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL1);
		}
		else
		{
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL5, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL6, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL7, mConfig.fPixelFormat);
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL8, mConfig.fPixelFormat);
			mDevice.EnableChannel (NTV2_CHANNEL5);
			mDevice.EnableChannel (NTV2_CHANNEL6);
			mDevice.EnableChannel (NTV2_CHANNEL7);
			mDevice.EnableChannel (NTV2_CHANNEL8);
			mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL5);
		}
	}

	if(mDeviceID == DEVICE_ID_KONAIP_1RX_1TX_2110 ||
		mDeviceID == DEVICE_ID_KONAIP_2110 ||
		mDeviceID == DEVICE_ID_KONAIP_2110)
	{
		mDevice.SetReference(NTV2_REFERENCE_SFP1_PTP);
	}
	else
	{
		mDevice.SetReference (NTV2_REFERENCE_FREERUN);
	}

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo


AJAStatus NTV2Player4K::SetUpAudio (void)
{
	uint16_t	numberOfAudioChannels	(::NTV2DeviceGetMaxAudioChannels (mDeviceID));

	//	If there are 4096 pixels on a line instead of 3840, reduce the number of audio channels
	//	This is because HANC is narrower, and has space for only 8 channels
	if (NTV2_IS_4K_4096_VIDEO_FORMAT(mConfig.fVideoFormat) && (numberOfAudioChannels > 8))
	{
		numberOfAudioChannels = 8;
	}

	mDevice.SetNumberAudioChannels (numberOfAudioChannels, mConfig.fAudioSystem);
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mConfig.fAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mConfig.fAudioSystem);
	if (::NTV2DeviceCanDo12gRouting(mDeviceID))
	{
		mDevice.SetSDIOutputAudioSystem (mConfig.fOutputChannel, mConfig.fAudioSystem);
	}
	else
	{
		if((mConfig.fAudioSystem == NTV2_AUDIOSYSTEM_1) ||
		   (mConfig.fAudioSystem == NTV2_AUDIOSYSTEM_3))
		{
			mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL1, mConfig.fAudioSystem);
			mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL2, mConfig.fAudioSystem);
			mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL3, mConfig.fAudioSystem);
			mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL4, mConfig.fAudioSystem);
		}
		else
		{
			mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL5, mConfig.fAudioSystem);
			mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL6, mConfig.fAudioSystem);
			mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL7, mConfig.fAudioSystem);
			mDevice.SetSDIOutputAudioSystem (NTV2_CHANNEL8, mConfig.fAudioSystem);
		}
	}

	//	Disable loopback, in case it was left enabled (from E-E configuration).
	//	Otherwise the output audio won't have our generated tone...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mConfig.fAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio


void NTV2Player4K::SetUpHostBuffers (void)
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	//	Calculate the size of the video buffer, which depends on video format, pixel format, and whether VANC is included or not...
	mVideoBufferSize = ::GetVideoWriteSize (mConfig.fVideoFormat, mConfig.fPixelFormat);
	mAudioBufferSize = AUDIOBYTES_MAX_48K;

	//	Allocate my buffers...
	for (uint32_t ndx(0);  ndx < CIRCULAR_BUFFER_SIZE;  ndx++)
	{
		//	The video buffer address will be filled in by the producer thread
		mHostBuffers[ndx].fVideoBuffer		= AJA_NULL;
		mHostBuffers[ndx].fVideoBufferSize	= mVideoBufferSize;
		mHostBuffers[ndx].fAudioBuffer		= mConfig.WithAudio()
												? reinterpret_cast<uint32_t*>(AJAMemory::AllocateAligned(mAudioBufferSize, BUFFER_ALIGNMENT))
												: AJA_NULL;
		mHostBuffers[ndx].fAudioBufferSize	= mConfig.WithAudio() ? mAudioBufferSize : 0;
		::memset (mHostBuffers[ndx].fAudioBuffer,  0x00,  mConfig.WithAudio() ? mAudioBufferSize : 0);
		mAVCircularBuffer.Add (&mHostBuffers[ndx]);
	}
}	//	SetUpHostBuffers


void NTV2Player4K::RouteOutputSignal (void)
{
	const bool	isRGB (::IsRGBFormat(mConfig.fPixelFormat));

	if (!mConfig.fDoMultiChannel)
		mDevice.ClearRouting();	//	Replace current signal routing

	//	Construct switch value to avoid multiple if-then-else
	int switchValue = 0;
	if (NTV2_IS_4K_HFR_VIDEO_FORMAT(mConfig.fVideoFormat))
		switchValue += 8;
	if (mConfig.fDoTsiRouting)
		switchValue += 4;
	if (isRGB)
		switchValue += 2;
	if (mConfig.fDoRGBOnWire)
		switchValue += 1;

	switch (switchValue)
	{
		case 0:		//	Low Frame Rate, Square, Pixel YCbCr, Wire YCbCr
			RouteFsToSDIOut();
			break;
		case 1:		//	Low Frame Rate, Square, Pixel YCbCr, Wire RGB
			RouteFsToCsc();
			RouteCscToDLOut();
			RouteDLOutToSDIOut();
			break;
		case 2:		//	Low Frame Rate, Square, Pixel RGB, Wire YCbCr
			RouteFsToCsc();
            RouteCscTo4xSDIOut();
			break;
		case 3:		//	Low Frame Rate, Square, Pixel RGB, Wire RGB
			RouteFsToDLOut();
			RouteDLOutToSDIOut();
			break;
		case 4:		//	Low Frame Rate, Tsi, Pixel YCbCr, Wire YCbCr
			RouteFsToTsiMux();
            RouteTsiMuxTo2xSDIOut();
			break;
		case 5:		//	Low Frame Rate, Tsi, Pixel YCbCr, Wire RGB
			RouteFsToTsiMux();
			RouteTsiMuxToCsc();
			RouteCscToDLOut();
			RouteDLOutToSDIOut();
			break;
		case 6:		//	Low Frame Rate, Tsi, Pixel RGB, Wire YCbCr
			RouteFsToTsiMux();
			RouteTsiMuxToCsc();
            RouteCscTo2xSDIOut();
			break;
		case 7:		//	Low Frame Rate, Tsi, Pixel RGB, Wire RGB
			RouteFsToTsiMux();
			RouteTsiMuxToDLOut();
			RouteDLOutToSDIOut();
			break;
		case 8:		//	High Frame Rate, Square, Pixel YCbCr, Wire YCbCr
			RouteFsToSDIOut();
			break;
		case 9:		//	High Frame Rate, Square, Pixel YCbCr, Wire RGB
			//	No valid routing for this case
			break;
		case 10:	//	High Frame Rate, Square, Pixel RGB, Wire YCbCr
			RouteFsToCsc();
            RouteCscTo4xSDIOut();
			break;
		case 11:	//	High Frame Rate, Square, Pixel RGB, Wire RGB
			//	No valid routing for this case
			break;
		case 12:	//	High Frame Rate, Tsi, Pixel YCbCr, Wire YCbCr
			RouteFsToTsiMux();
            RouteTsiMuxTo4xSDIOut();
			break;
		case 13:	//	High Frame Rate, Tsi, Pixel YCbCr, Wire RGB
			//	No valid routing for this case
			break;
		case 14:	//	High Frame Rate, Tsi, Pixel RGB, Wire YCbCr
			RouteFsToTsiMux();
			RouteTsiMuxToCsc();
            RouteCscTo4xSDIOut();
			break;
		case 15:	//	High Frame Rate, Tsi, Pixel RGB, Wire RGB
			//	No valid routing for this case
			break;
		default:
			return;
	}

	if (::NTV2DeviceCanDo12gRouting(mDeviceID))
		mDevice.SetTsiFrameEnable  (true,  mConfig.fOutputChannel);
	else if (mConfig.fDoTsiRouting)
		mDevice.SetTsiFrameEnable  (true,  mConfig.fOutputChannel);
	else
		mDevice.Set4kSquaresEnable (true,  mConfig.fOutputChannel);

	//	Send signal to secondary outputs, if supported
	Route4KDownConverter();
	RouteHDMIOutput();

	//	Enable SDI output from all channels,
	//	but only if the device supports bi-directional SDI.
	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
	{
		if (::NTV2DeviceCanDo12gRouting(mDeviceID))
			mDevice.SetSDITransmitEnable (mConfig.fOutputChannel, true);
		else if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);
		}
		else
		{
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL5, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL6, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL7, true);
			mDevice.SetSDITransmitEnable (NTV2_CHANNEL8, true);
		}
	}

}	//	RouteOutputSignal


void NTV2Player4K::Route4KDownConverter (void)
{
	const bool			isRGB			(::IsRGBFormat(mConfig.fPixelFormat));

	if ( ! ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt4KDownConverter) || ! ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIMonOut1))
		return;

	if (isRGB)
	{
		mDevice.Enable4KDCRGBMode(true);

		if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer1RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer2RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer3RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer4RGB);

			mDevice.Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut);
			mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV);
		}
		else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
		{
			mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer5RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer6RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer7RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer8RGB);

			mDevice.Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut);
			mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV);
		}
	}
	else	//	!RGB
	{
		mDevice.Enable4KDCRGBMode (false);

		if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer1YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer2YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer3YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer4YUV);

			mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut);
		}
		else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
		{
			mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer5YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer6YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer7YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer8YUV);

			mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut);
		}
	}

}	//	Route4KDownConverter


void NTV2Player4K::RouteHDMIOutput (void)
{
	const bool	isRGB (::IsRGBFormat(mConfig.fPixelFormat));

	if (mConfig.fDoHDMIOutput &&
		(::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v2)
			|| ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v3)
			|| ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1v4)) )
	{
		if (::NTV2DeviceCanDo12gRouting(mDeviceID))
			mDevice.Connect (NTV2_XptHDMIOutInput, ::GetFrameBufferOutputXptFromChannel (mConfig.fOutputChannel,  isRGB,  false/*is425*/));
		else if(mConfig.fDoTsiRouting)
		{
			if (isRGB)
			{
				if (mConfig.fOutputChannel == NTV2_CHANNEL1)
				{
					mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptCSC1VidYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptCSC2VidYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptCSC3VidYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptCSC4VidYUV);
				}
				else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
				{
					mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptCSC5VidYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptCSC6VidYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptCSC7VidYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptCSC8VidYUV);
				}
			}
			else
			{
				if (mConfig.fOutputChannel == NTV2_CHANNEL1)
				{
					mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_Xpt425Mux1AYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_Xpt425Mux1BYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_Xpt425Mux2AYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_Xpt425Mux2BYUV);
				}
				else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
				{
					mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_Xpt425Mux1AYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_Xpt425Mux1BYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_Xpt425Mux2AYUV);
					mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_Xpt425Mux2BYUV);
				}
			}
		}
		else
		{
			if (isRGB)
			{
				if (mConfig.fOutputChannel == NTV2_CHANNEL1)
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
				else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
				{
					mDevice.Connect (NTV2_XptCSC5VidInput,		NTV2_XptFrameBuffer5RGB);
					mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptCSC5VidYUV);

					mDevice.Connect (NTV2_XptCSC6VidInput,		NTV2_XptFrameBuffer6RGB);
					mDevice.Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptCSC6VidYUV);

					mDevice.Connect (NTV2_XptCSC7VidInput,		NTV2_XptFrameBuffer7RGB);
					mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptCSC7VidYUV);

					mDevice.Connect (NTV2_XptCSC8VidInput,		NTV2_XptFrameBuffer8RGB);
					mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptCSC8VidYUV);
				}
			}
			else
			{
				if (mConfig.fOutputChannel == NTV2_CHANNEL1)
				{
					mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptFrameBuffer1YUV);
					mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_XptFrameBuffer2YUV);
					mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptFrameBuffer3YUV);
					mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptFrameBuffer4YUV);
				}
				else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
				{
					mDevice.Connect (NTV2_XptHDMIOutInput,		NTV2_XptFrameBuffer5YUV);
					mDevice.Connect (NTV2_XptHDMIOutQ2Input,	NTV2_XptFrameBuffer6YUV);
					mDevice.Connect (NTV2_XptHDMIOutQ3Input,	NTV2_XptFrameBuffer7YUV);
					mDevice.Connect (NTV2_XptHDMIOutQ4Input,	NTV2_XptFrameBuffer8YUV);
				}
			}
		}

		mDevice.SetHDMIV2TxBypass (false);
		mDevice.SetHDMIOutVideoStandard (::GetNTV2StandardFromVideoFormat(mConfig.fVideoFormat));
		mDevice.SetHDMIOutVideoFPS (::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
		mDevice.SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
		mDevice.SetHDMIV2Mode (NTV2_HDMI_V2_4K_PLAYBACK);
	}
	else
		mDevice.SetHDMIV2Mode (NTV2_HDMI_V2_4K_PLAYBACK);

}	//	RouteHDMIOutput


void NTV2Player4K::RouteFsToDLOut (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
		mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptFrameBuffer2RGB);
		mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer3RGB);
		mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptFrameBuffer4RGB);
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		mDevice.Connect (NTV2_XptDualLinkOut5Input,	NTV2_XptFrameBuffer5RGB);
		mDevice.Connect (NTV2_XptDualLinkOut6Input,	NTV2_XptFrameBuffer6RGB);
		mDevice.Connect (NTV2_XptDualLinkOut7Input,	NTV2_XptFrameBuffer7RGB);
		mDevice.Connect (NTV2_XptDualLinkOut8Input,	NTV2_XptFrameBuffer8RGB);
	}
}	//	RouteFsToDLOut


void NTV2Player4K::RouteFsToCsc (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
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
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			mDevice.Connect (NTV2_XptCSC5VidInput,	NTV2_XptFrameBuffer5RGB);
			mDevice.Connect (NTV2_XptCSC6VidInput,	NTV2_XptFrameBuffer6RGB);
			mDevice.Connect (NTV2_XptCSC7VidInput,	NTV2_XptFrameBuffer7RGB);
			mDevice.Connect (NTV2_XptCSC8VidInput,	NTV2_XptFrameBuffer8RGB);
		}
		else
		{
			mDevice.Connect (NTV2_XptCSC5VidInput,	NTV2_XptFrameBuffer5YUV);
			mDevice.Connect (NTV2_XptCSC6VidInput,	NTV2_XptFrameBuffer6YUV);
			mDevice.Connect (NTV2_XptCSC7VidInput,	NTV2_XptFrameBuffer7YUV);
			mDevice.Connect (NTV2_XptCSC8VidInput,	NTV2_XptFrameBuffer8YUV);
		}
	}
}	//	RouteFsToCsc


void NTV2Player4K::RouteFsToSDIOut (void)
{
	if (::NTV2DeviceCanDo12gRouting(mDeviceID))
	{
		mDevice.Connect (::GetSDIOutputInputXpt (mConfig.fOutputChannel, false/*isDS2*/), ::GetFrameBufferOutputXptFromChannel (mConfig.fOutputChannel,  false/*isRGB*/,  false/*is425*/));
	}
	else
	{
		if (mConfig.fOutputChannel == NTV2_CHANNEL1)
		{
			mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptFrameBuffer1YUV);
			mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptFrameBuffer2YUV);
			mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptFrameBuffer3YUV);
			mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptFrameBuffer4YUV);
		}
		else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
		{
			mDevice.Connect (NTV2_XptSDIOut5Input,	NTV2_XptFrameBuffer5YUV);
			mDevice.Connect (NTV2_XptSDIOut6Input,	NTV2_XptFrameBuffer6YUV);
			mDevice.Connect (NTV2_XptSDIOut7Input,	NTV2_XptFrameBuffer7YUV);
			mDevice.Connect (NTV2_XptSDIOut8Input,	NTV2_XptFrameBuffer8YUV);
		}
	}
}	//	RouteFsToSDIOut


void NTV2Player4K::RouteFsToTsiMux (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			mDevice.Connect (NTV2_Xpt425Mux1AInput,	NTV2_XptFrameBuffer1RGB);
			mDevice.Connect (NTV2_Xpt425Mux1BInput,	NTV2_XptFrameBuffer1_425RGB);
			mDevice.Connect (NTV2_Xpt425Mux2AInput,	NTV2_XptFrameBuffer2RGB);
			mDevice.Connect (NTV2_Xpt425Mux2BInput,	NTV2_XptFrameBuffer2_425RGB);
		}
		else
		{
			mDevice.Connect (NTV2_Xpt425Mux1AInput,	NTV2_XptFrameBuffer1YUV);
			mDevice.Connect (NTV2_Xpt425Mux1BInput,	NTV2_XptFrameBuffer1_425YUV);
			mDevice.Connect (NTV2_Xpt425Mux2AInput,	NTV2_XptFrameBuffer2YUV);
			mDevice.Connect (NTV2_Xpt425Mux2BInput,	NTV2_XptFrameBuffer2_425YUV);
		}
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			mDevice.Connect (NTV2_Xpt425Mux3AInput,	NTV2_XptFrameBuffer5RGB);
			mDevice.Connect (NTV2_Xpt425Mux3BInput,	NTV2_XptFrameBuffer5_425RGB);
			mDevice.Connect (NTV2_Xpt425Mux4AInput,	NTV2_XptFrameBuffer6RGB);
			mDevice.Connect (NTV2_Xpt425Mux4BInput,	NTV2_XptFrameBuffer6_425RGB);
		}
		else
		{
			mDevice.Connect (NTV2_Xpt425Mux3AInput,	NTV2_XptFrameBuffer5YUV);
			mDevice.Connect (NTV2_Xpt425Mux3BInput,	NTV2_XptFrameBuffer5_425YUV);
			mDevice.Connect (NTV2_Xpt425Mux4AInput,	NTV2_XptFrameBuffer6YUV);
			mDevice.Connect (NTV2_Xpt425Mux4BInput,	NTV2_XptFrameBuffer6_425YUV);
		}
	}
}	//	RouteFsToTsiMux


void NTV2Player4K::RouteDLOutToSDIOut (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		mDevice.Connect (NTV2_XptSDIOut1Input,		NTV2_XptDuallinkOut1);
		mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_XptDuallinkOut1DS2);
		mDevice.Connect (NTV2_XptSDIOut2Input,		NTV2_XptDuallinkOut2);
		mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_XptDuallinkOut2DS2);
		mDevice.Connect (NTV2_XptSDIOut3Input,		NTV2_XptDuallinkOut3);
		mDevice.Connect (NTV2_XptSDIOut3InputDS2,	NTV2_XptDuallinkOut3DS2);
		mDevice.Connect (NTV2_XptSDIOut4Input,		NTV2_XptDuallinkOut4);
		mDevice.Connect (NTV2_XptSDIOut4InputDS2,	NTV2_XptDuallinkOut4DS2);
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		mDevice.Connect (NTV2_XptSDIOut5Input,		NTV2_XptDuallinkOut5);
		mDevice.Connect (NTV2_XptSDIOut5InputDS2,	NTV2_XptDuallinkOut5DS2);
		mDevice.Connect (NTV2_XptSDIOut6Input,		NTV2_XptDuallinkOut6);
		mDevice.Connect (NTV2_XptSDIOut6InputDS2,	NTV2_XptDuallinkOut6DS2);
		mDevice.Connect (NTV2_XptSDIOut7Input,		NTV2_XptDuallinkOut7);
		mDevice.Connect (NTV2_XptSDIOut7InputDS2,	NTV2_XptDuallinkOut7DS2);
		mDevice.Connect (NTV2_XptSDIOut8Input,		NTV2_XptDuallinkOut8);
		mDevice.Connect (NTV2_XptSDIOut8InputDS2,	NTV2_XptDuallinkOut8DS2);
	}
}	//	RouteFsDLOutToSDIOut


void NTV2Player4K::RouteCscTo2xSDIOut (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
        mDevice.Connect (NTV2_XptSDIOut1Input,      NTV2_XptCSC1VidYUV);
        mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_XptCSC2VidYUV);
        mDevice.Connect (NTV2_XptSDIOut2Input,      NTV2_XptCSC3VidYUV);
        mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_XptCSC4VidYUV);
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
        mDevice.Connect (NTV2_XptSDIOut5Input,      NTV2_XptCSC5VidYUV);
        mDevice.Connect (NTV2_XptSDIOut5InputDS2,	NTV2_XptCSC6VidYUV);
        mDevice.Connect (NTV2_XptSDIOut6Input,      NTV2_XptCSC7VidYUV);
        mDevice.Connect (NTV2_XptSDIOut6InputDS2,	NTV2_XptCSC8VidYUV);
	}
}	//	RouteCscToSDIOut


void NTV2Player4K::RouteCscTo4xSDIOut (void)
{
    if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_XptCSC1VidYUV);
		mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_XptCSC2VidYUV);
		mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_XptCSC3VidYUV);
		mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_XptCSC4VidYUV);
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		mDevice.Connect (NTV2_XptSDIOut5Input,	NTV2_XptCSC5VidYUV);
		mDevice.Connect (NTV2_XptSDIOut6Input,	NTV2_XptCSC6VidYUV);
		mDevice.Connect (NTV2_XptSDIOut7Input,	NTV2_XptCSC7VidYUV);
		mDevice.Connect (NTV2_XptSDIOut8Input,	NTV2_XptCSC8VidYUV);
	}
}	//	RouteCscToSDIOut


void NTV2Player4K::RouteCscToDLOut (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_XptCSC1VidRGB);
		mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_XptCSC2VidRGB);
		mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptCSC3VidRGB);
		mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_XptCSC4VidRGB);
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		mDevice.Connect (NTV2_XptDualLinkOut5Input,	NTV2_XptCSC5VidRGB);
		mDevice.Connect (NTV2_XptDualLinkOut6Input,	NTV2_XptCSC6VidRGB);
		mDevice.Connect (NTV2_XptDualLinkOut7Input,	NTV2_XptCSC7VidRGB);
		mDevice.Connect (NTV2_XptDualLinkOut8Input,	NTV2_XptCSC8VidRGB);
	}
}	//	RouteCscToDLOut


void NTV2Player4K::RouteTsiMuxToDLOut (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		mDevice.Connect (NTV2_XptDualLinkOut1Input,	NTV2_Xpt425Mux1ARGB);
		mDevice.Connect (NTV2_XptDualLinkOut2Input,	NTV2_Xpt425Mux1BRGB);
		mDevice.Connect (NTV2_XptDualLinkOut3Input,	NTV2_Xpt425Mux2ARGB);
		mDevice.Connect (NTV2_XptDualLinkOut4Input,	NTV2_Xpt425Mux2BRGB);
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		mDevice.Connect (NTV2_XptDualLinkOut5Input,	NTV2_Xpt425Mux3ARGB);
		mDevice.Connect (NTV2_XptDualLinkOut6Input,	NTV2_Xpt425Mux3BRGB);
		mDevice.Connect (NTV2_XptDualLinkOut7Input,	NTV2_Xpt425Mux4ARGB);
		mDevice.Connect (NTV2_XptDualLinkOut8Input,	NTV2_Xpt425Mux4BRGB);
	}
}	//	RouteTsiMuxToDLOut


void NTV2Player4K::RouteTsiMuxToCsc (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_Xpt425Mux1ARGB);
			mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_Xpt425Mux1BRGB);
			mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_Xpt425Mux2ARGB);
			mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_Xpt425Mux2BRGB);
		}
		else
		{
			mDevice.Connect (NTV2_XptCSC1VidInput,	NTV2_Xpt425Mux1AYUV);
			mDevice.Connect (NTV2_XptCSC2VidInput,	NTV2_Xpt425Mux1BYUV);
			mDevice.Connect (NTV2_XptCSC3VidInput,	NTV2_Xpt425Mux2AYUV);
			mDevice.Connect (NTV2_XptCSC4VidInput,	NTV2_Xpt425Mux2BYUV);
		}
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
		if (::IsRGBFormat(mConfig.fPixelFormat))
		{
			mDevice.Connect (NTV2_XptCSC5VidInput,	NTV2_Xpt425Mux3ARGB);
			mDevice.Connect (NTV2_XptCSC6VidInput,	NTV2_Xpt425Mux3BRGB);
			mDevice.Connect (NTV2_XptCSC7VidInput,	NTV2_Xpt425Mux4ARGB);
			mDevice.Connect (NTV2_XptCSC8VidInput,	NTV2_Xpt425Mux4BRGB);
		}
		else
		{
			mDevice.Connect (NTV2_XptCSC5VidInput,	NTV2_Xpt425Mux3AYUV);
			mDevice.Connect (NTV2_XptCSC6VidInput,	NTV2_Xpt425Mux3BYUV);
			mDevice.Connect (NTV2_XptCSC7VidInput,	NTV2_Xpt425Mux4AYUV);
			mDevice.Connect (NTV2_XptCSC8VidInput,	NTV2_Xpt425Mux4BYUV);
		}
	}
}	//	RouteTsiMuxToCsc


void NTV2Player4K::RouteTsiMuxTo2xSDIOut (void)
{
	if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
        mDevice.Connect (NTV2_XptSDIOut1Input,      NTV2_Xpt425Mux1AYUV);
        mDevice.Connect (NTV2_XptSDIOut1InputDS2,	NTV2_Xpt425Mux1BYUV);
        mDevice.Connect (NTV2_XptSDIOut2Input,      NTV2_Xpt425Mux2AYUV);
        mDevice.Connect (NTV2_XptSDIOut2InputDS2,	NTV2_Xpt425Mux2BYUV);
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
        mDevice.Connect (NTV2_XptSDIOut5Input,      NTV2_Xpt425Mux3AYUV);
        mDevice.Connect (NTV2_XptSDIOut5InputDS2,	NTV2_Xpt425Mux3BYUV);
        mDevice.Connect (NTV2_XptSDIOut6Input,      NTV2_Xpt425Mux4AYUV);
        mDevice.Connect (NTV2_XptSDIOut6InputDS2,	NTV2_Xpt425Mux4BYUV);
	}
}	//	RouteTsiMuxToSDIOut


void NTV2Player4K::RouteTsiMuxTo4xSDIOut (void)
{
    if (mConfig.fOutputChannel == NTV2_CHANNEL1)
	{
        mDevice.Connect (NTV2_XptSDIOut1Input,	NTV2_Xpt425Mux1AYUV);
        mDevice.Connect (NTV2_XptSDIOut2Input,	NTV2_Xpt425Mux1BYUV);
        mDevice.Connect (NTV2_XptSDIOut3Input,	NTV2_Xpt425Mux2AYUV);
        mDevice.Connect (NTV2_XptSDIOut4Input,	NTV2_Xpt425Mux2BYUV);
	}
	else if (mConfig.fOutputChannel == NTV2_CHANNEL5)
	{
        mDevice.Connect (NTV2_XptSDIOut5Input,	NTV2_Xpt425Mux3AYUV);
        mDevice.Connect (NTV2_XptSDIOut6Input,	NTV2_Xpt425Mux3BYUV);
        mDevice.Connect (NTV2_XptSDIOut7Input,	NTV2_Xpt425Mux4AYUV);
        mDevice.Connect (NTV2_XptSDIOut8Input,	NTV2_Xpt425Mux4BYUV);
	}
}	//	RouteTsiMuxToSDIOut


AJAStatus NTV2Player4K::Run (void)
{
	//	Start the playout and producer threads...
	StartConsumerThread();	//	Start playing frames
	StartProducerThread();	//	Start producing frames

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void NTV2Player4K::StartConsumerThread (void)
{
	//	Create and start the playout thread...
	mConsumerThread = new AJAThread ();
	mConsumerThread->Attach (PlayThreadStatic, this);
	mConsumerThread->SetPriority (AJA_ThreadPriority_High);
	mConsumerThread->Start ();

}	//	StartPlayThread


//	The playout thread function
void NTV2Player4K::PlayThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Player4K instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	NTV2Player4K *	pApp (reinterpret_cast<NTV2Player4K*>(pContext));
	pApp->ConsumeFrames();

}	//	PlayThreadStatic


void NTV2Player4K::ConsumeFrames (void)
{
	uint8_t					numACFramesPerChannel(7);
	ULWord					acOptions (AUTOCIRCULATE_WITH_RP188);
	uint32_t				hdrPktSize	(0);
	AUTOCIRCULATE_TRANSFER	outputXferInfo;
	AUTOCIRCULATE_STATUS	outputStatus;

	//	Stop AutoCirculate, just in case someone else left it running...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel, 4);	//	Let it stop
	PLNOTE("Started");

	if (IS_KNOWN_AJAAncillaryDataType(mConfig.fSendAncType))
	{	//	Insert one of these HDR anc packets...
		AJAAncillaryData_HDR_SDR	sdrPkt;
		AJAAncillaryData_HDR_HDR10	hdr10Pkt;
		AJAAncillaryData_HDR_HLG	hlgPkt;
		AJAAncillaryData *			pPkt	(AJA_NULL);

		switch (mConfig.fSendAncType)
		{
			case AJAAncillaryDataType_HDR_SDR:		pPkt = &sdrPkt;		break;
			case AJAAncillaryDataType_HDR_HDR10:	pPkt = &hdr10Pkt;	break;
			case AJAAncillaryDataType_HDR_HLG:		pPkt = &hlgPkt;		break;
			default:
				break;
		}
		if (pPkt)
		{	//	Allocate page-aligned host Anc buffer...
			outputXferInfo.acANCBuffer.Allocate(NTV2_ANCSIZE_MAX, /*pageAligned=*/true);
			outputXferInfo.acANCBuffer.Fill(0LL);	//	Zero it
			pPkt->GenerateTransmitData (reinterpret_cast<uint8_t*>(outputXferInfo.acANCBuffer.GetHostPointer()),
										outputXferInfo.acANCBuffer.GetByteCount(),  hdrPktSize);
			acOptions |= AUTOCIRCULATE_WITH_ANC;
		}
	}

	//	Initialize & start AutoCirculate...
    if (::NTV2DeviceCanDo12gRouting(mDeviceID))
    {
        uint32_t startNum(0), endNum(0);
		switch (mConfig.fOutputChannel)
		{
			case NTV2_CHANNEL2:	startNum = numACFramesPerChannel;		break;
			case NTV2_CHANNEL3:	startNum = numACFramesPerChannel * 2;	break;
			case NTV2_CHANNEL4:	startNum = numACFramesPerChannel * 3;	break;
			default:			break;
		}
        endNum = startNum + numACFramesPerChannel - 1;
        mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,  0,	//	0 frameCount: we'll specify start & end frame numbers
											mConfig.fAudioSystem,  acOptions,
                                            1 /*numChannels*/,  UByte(startNum),  UByte(endNum));
    }
    else
    {
		const uint8_t	startNum	(mConfig.fOutputChannel < 4	?						0	:	numACFramesPerChannel);		//	Ch1: frames 0-6
		const uint8_t	endNum		(mConfig.fOutputChannel < 4	?	numACFramesPerChannel-1	:	numACFramesPerChannel*2-1);	//	Ch5: frames 7-13
        mDevice.AutoCirculateInitForOutput (mConfig.fOutputChannel,  0,	//	0 frameCount: we'll specify start & end frame numbers
											mConfig.fAudioSystem,  acOptions,
											1 /*numChannels*/,  startNum,  endNum);
	}
	mDevice.AutoCirculateStart(mConfig.fOutputChannel);

	while (!mGlobalQuit)
	{
		mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outputStatus);

		//	Check if there's room for another frame on the card...
		if (outputStatus.CanAcceptMoreOutputFrames())
		{
			//	Wait for the next frame to become ready to "consume"...
			AVDataBuffer *	playData	(mAVCircularBuffer.StartConsumeNextBuffer ());

			//	Burn the current timecode into the test pattern image that's now in my video buffer...
			const	NTV2FrameRate	ntv2FrameRate	(::GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat));
			const	TimecodeFormat	tcFormat		(CNTV2DemoCommon::NTV2FrameRate2TimecodeFormat(ntv2FrameRate));
			const	CRP188			rp188Info		(mCurrentFrame++, 0, 0, 10, tcFormat);
			string					timeCodeStr;

			rp188Info.GetRP188Reg (playData->fRP188Data);	//	Get the DBB/Hi/Lo register values
			rp188Info.GetRP188Str (timeCodeStr);
			const NTV2_RP188 rp188(playData->fRP188Data);
			mTCBurner.BurnTimeCode (reinterpret_cast<char*>(playData->fVideoBuffer), timeCodeStr.c_str(), 80);

			//	Transfer the timecode-burned frame (plus audio) to the device for playout...
			outputXferInfo.SetVideoBuffer (playData->fVideoBuffer, playData->fVideoBufferSize);
			outputXferInfo.SetAudioBuffer (playData->fAudioBuffer, playData->fAudioBufferSize);
			outputXferInfo.SetOutputTimeCode (rp188, ::NTV2ChannelToTimecodeIndex(mConfig.fOutputChannel, /*LTC=*/false, /*F2=*/false));
			outputXferInfo.SetOutputTimeCode (rp188, ::NTV2ChannelToTimecodeIndex(mConfig.fOutputChannel, /*LTC=*/true,  /*F2=*/false));
			mDevice.AutoCirculateTransfer (mConfig.fOutputChannel, outputXferInfo);

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer();
		}
		else
			mDevice.WaitForOutputVerticalInterrupt(mConfig.fOutputChannel);
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.fOutputChannel);
	PLNOTE("Ended, " << DEC(mCurrentFrame) << " frames played, " << DEC(outputStatus.GetDroppedFrameCount()) << " dropped");

}	//	ConsumeFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void NTV2Player4K::StartProducerThread (void)
{
	//	Create and start the producer thread...
	mProducerThread = new AJAThread ();
	mProducerThread->Attach (ProduceFrameThreadStatic, this);
	mProducerThread->SetPriority (AJA_ThreadPriority_High);
	mProducerThread->Start ();

}	//	StartProduceFrameThread


void NTV2Player4K::ProduceFrameThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;
	NTV2Player4K *	pApp (reinterpret_cast<NTV2Player4K*>(pContext));
	pApp->ProduceFrames();

}	//	ProduceFrameThreadStatic


void NTV2Player4K::SetUpTestPatternVideoBuffers (void)
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
	mTestPatternBuffers = new uint8_t * [mNumTestPatterns];

	//	Set up one video buffer for each of the several predefined patterns...
	for (unsigned tpIndex(0);  tpIndex < mNumTestPatterns;  tpIndex++)
	{
		//	Allocate the buffer memory...
		mTestPatternBuffers[tpIndex] = reinterpret_cast<uint8_t*>(AJAMemory::AllocateAligned (mVideoBufferSize, BUFFER_ALIGNMENT));

		//	Use the test pattern generator to fill an NTV2TestPatternBuffer...
		NTV2TestPatternBuffer	testPatternBuffer;
		NTV2TestPatternGen		testPatternGen;
		NTV2FormatDescriptor	formatDesc	(mConfig.fVideoFormat, mConfig.fPixelFormat);

		testPatternGen.DrawTestPattern (testPatternTypes [tpIndex],
										formatDesc.numPixels,
										formatDesc.numLines,
										mConfig.fPixelFormat,
										testPatternBuffer);

		//	Copy the contents of the test pattern buffer into my 'C' array, for quick "memcpy" into each frame...
		const size_t	testPatternSize	(testPatternBuffer.size());
		uint8_t * const	pVideoBuffer	(mTestPatternBuffers[tpIndex]);
		for (size_t ndx(0);  ndx < testPatternSize;  ndx++)
			pVideoBuffer[ndx] = testPatternBuffer[ndx];
	}	//	loop for each predefined pattern

}	//	SetUpTestPatternVideoBuffers


void NTV2Player4K::ProduceFrames (void)
{

	const double	frequencies [] =	{250.0, 500.0, 1000.0, 2000.0};
	double			timeOfLastSwitch	(0.0);
	const ULWord	numFrequencies		(sizeof (frequencies) / sizeof (double));
	ULWord			frequencyIndex		(0);
	ULWord			testPatternIndex	(0);

	PLNOTE("Started");
	AJATimeBase	timeBase (CNTV2DemoCommon::GetAJAFrameRate(GetNTV2FrameRateFromVideoFormat(mConfig.fVideoFormat)));

	while (!mGlobalQuit)
	{
		AVDataBuffer *	frameData	(mAVCircularBuffer.StartProduceNextBuffer());

		//  If no frame is available, wait and try again
		if (!frameData)
		{
			AJATime::Sleep (10);
			continue;
		}

		//	Set the video buffer pointer to the test pattern to display
		frameData->fVideoBuffer = reinterpret_cast<uint32_t*>(mTestPatternBuffers[testPatternIndex]);

		//	Generate audio tone data...
		frameData->fAudioBufferSize		= mConfig.WithAudio() ? AddTone(frameData->fAudioBuffer) : 0;

		//	Every few seconds, change the test pattern and tone frequency...
		const double	currentTime	(timeBase.FramesToSeconds(mCurrentFrame));
		if (currentTime > timeOfLastSwitch + 4.0)
		{
			frequencyIndex = (frequencyIndex + 1) % numFrequencies;
			testPatternIndex = (testPatternIndex + 1) % mNumTestPatterns;
			mToneFrequency = frequencies [frequencyIndex];
			timeOfLastSwitch = currentTime;
		}	//	if time to switch test pattern & tone frequency

		//	Signal that I'm done producing the buffer -- it's now available for playout...
		mAVCircularBuffer.EndProduceNextBuffer();

	}	//	loop til mGlobalQuit goes true
	PLNOTE("Ended");

}	//	ProduceFrames


void NTV2Player4K::GetACStatus (AUTOCIRCULATE_STATUS & outputStatus)
{
	mDevice.AutoCirculateGetStatus (mConfig.fOutputChannel, outputStatus);
}


uint32_t NTV2Player4K::AddTone (ULWord * audioBuffer)
{
	NTV2FrameRate	frameRate;
	NTV2AudioRate	audioRate;
	ULWord			numChannels;

	mDevice.GetFrameRate (frameRate, mConfig.fOutputChannel);
	mDevice.GetAudioRate (audioRate, mConfig.fAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mConfig.fAudioSystem);

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


ULWord NTV2Player4K::GetRP188RegisterForOutput (const NTV2OutputDestination inOutputDest)		//	static
{
	switch (inOutputDest)
	{
		case NTV2_OUTPUTDESTINATION_SDI1:	return kRegRP188InOut1DBB;	//	reg 29
		case NTV2_OUTPUTDESTINATION_SDI2:	return kRegRP188InOut2DBB;	//	reg 64
		case NTV2_OUTPUTDESTINATION_SDI3:	return kRegRP188InOut3DBB;	//	reg 268
		case NTV2_OUTPUTDESTINATION_SDI4:	return kRegRP188InOut4DBB;	//	reg 273
		default:							return 0;
	}	//	switch on output destination

}	//	GetRP188RegisterForOutput


ostream & Player4KConfig::Print (std::ostream & strm) const
{
	AJALabelValuePairs table;
	AJASystemInfo::append (table, "Device",			fDeviceSpecifier);
	AJASystemInfo::append (table, "Video Format",	::NTV2VideoFormatToString(fVideoFormat));
	AJASystemInfo::append (table, "Pixel Format",	::NTV2FrameBufferFormatToString(fPixelFormat));
	AJASystemInfo::append (table, "Channel",		::NTV2ChannelToString(fOutputChannel));
	AJASystemInfo::append (table, "Audio",			NTV2_IS_VALID_AUDIO_SYSTEM(fAudioSystem) ? "Yes" : "No");
	AJASystemInfo::append (table, "MultiChannel",	fDoMultiChannel ? "Yes" : "No");
	AJASystemInfo::append (table, "HDR Anc Type",	::AJAAncillaryDataTypeToString(fSendAncType));
	AJASystemInfo::append (table, "HDMI Output",	fDoHDMIOutput ? "Yes" : "No");
	AJASystemInfo::append (table, "Tsi",			fDoTsiRouting ? "Yes" : "No");
	AJASystemInfo::append (table, "RGB Output",		fDoRGBOnWire ? "Yes" : "No");
	strm << AJASystemInfo::ToString(table);
	return strm;
}
