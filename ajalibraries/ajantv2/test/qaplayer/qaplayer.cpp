/**
	@file		qaplayer.cpp
	@brief		Implementation of QAPlayer class.
	@copyright	Copyright (C) 2013-2014 AJA Video Systems, Inc.  All rights reserved.
**/

#include "qaplayer.h"
#include "ntv2utils.h"
#include "ntv2debug.h"
#include "ajabase/common/testpatterngen.h"
#include "ajabase/common/timecode.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"


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


static const uint8_t ancPacket[240] =
{
	0xFF, 0xA0, 0x11,
	0x37, 0x73, 0xE9,	// 233 bytes
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x85, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0xC7	//	Checksum
};


QAPlayer::QAPlayer (const QAPlayerControl & inPlayerControl)
	:	mConfig						(inPlayerControl),
		mConsumerThread				(NULL),
		mProducerThread				(NULL),
		mFramesProduced				(0),
		mCurrentSample				(0),
		mDeviceID					(DEVICE_ID_NOTFOUND),
		mSavedTaskMode				(NTV2_DISABLE_TASKS),
		mVancEnabled				(false),
		mWideVanc					(false),
		mAudioSystem				(::NTV2ChannelToAudioSystem (mConfig.channel)),
		mGlobalQuit					(false),
		mVideoBufferSize			(0),
		mTestPatternVideoBuffers	(NULL),
		mNumTestPatterns			(0),
		mInstance					(NULL),
		mPlayerCallback				(NULL),
		mpF1AncBuffer				(NULL),
		mpF2AncBuffer				(NULL),
		mPacketsPerLine				((mConfig.packetsPerLine >= 1 && mConfig.packetsPerLine <= 8) ? mConfig.packetsPerLine : 1)
{
	::memset (mAVHostBuffer, 0, sizeof (mAVHostBuffer));
}


QAPlayer::~QAPlayer (void)
{
	//	Stop my playout and producer threads, then destroy them...
	Quit ();

	mDevice.UnsubscribeOutputVerticalEvent (mConfig.channel);

	//	Free my threads and buffers...
	delete mConsumerThread;
	mConsumerThread = NULL;
	delete mProducerThread;
	mProducerThread = NULL;

	if (mTestPatternVideoBuffers)
	{
		for (int32_t ndx = 0; ndx < mNumTestPatterns; ndx++)
			delete [] mTestPatternVideoBuffers [ndx];
		delete [] mTestPatternVideoBuffers;
		mTestPatternVideoBuffers = NULL;
		mNumTestPatterns = 0;
	}

	for (unsigned int ndx = 0; ndx < CIRCULAR_BUFFER_SIZE; ndx++)
	{
		if (mAVHostBuffer [ndx].fVideoBuffer)
		{
			delete mAVHostBuffer [ndx].fVideoBuffer;
			mAVHostBuffer [ndx].fVideoBuffer = NULL;
		}
		if (mAVHostBuffer [ndx].fAudioBuffer)
		{
			delete mAVHostBuffer [ndx].fAudioBuffer;
			mAVHostBuffer [ndx].fAudioBuffer = NULL;
		}
	}	//	for each buffer in the ring

	mDevice.SetEveryFrameServices (mSavedTaskMode);			//	Restore the previously saved service level
	mDevice.ReleaseStreamForApplication (AJA_FOURCC ('D','E','M','O'), static_cast <uint32_t> (AJAProcess::GetPid ()));	//	Release the device

	delete mpF1AncBuffer;
	delete mpF2AncBuffer;
}	//	destructor


void QAPlayer::Quit (void)
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


AJAStatus QAPlayer::Init (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	//	Open the device...
	CNTV2DeviceScanner	deviceScanner;

	//	Any AJA devices out there?
	if (deviceScanner.GetNumDevices () == 0)
		return AJA_STATUS_OPEN;

	//	Using a reference to the discovered device list, and the index number of the
	//	device of interest (mConfig.deviceIndex), get information about that particular device...
	NTV2DeviceInfo	info	(deviceScanner.GetDeviceInfoList () [mConfig.deviceIndex]);
	if (!mDevice.Open (info.deviceIndex, false))	//	, info.boardType))
		return AJA_STATUS_OPEN;

	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);				//	Set OEM service level

	//	Keep the board ID handy, as it will be used frequently...
	mDeviceID = mDevice.GetDeviceID ();

	if(::NTV2DeviceCanDoMultiFormat(mDeviceID))
		mDevice.SetMultiFormatMode(true);

	//	Set up the video and audio...
	if (!NTV2_IS_4K_VIDEO_FORMAT (mConfig.videoFormat))
		status = SetUpVideo ();
	else
		status = SetUp4kVideo();

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
	if (!NTV2_IS_4K_VIDEO_FORMAT (mConfig.videoFormat))
		RouteOutputSignal ();
	else
		Route4kOutputSignal();

	SetUpOutputAutoCirculate ();

	//	This is for the timecode that we will burn onto the image...
	NTV2FormatDescriptor fd = GetFormatDescriptor (mConfig.videoFormat,
													mConfig.pixelFormat,
													mVancEnabled,
													mWideVanc);

	//	Lastly, prepare my AJATimeCodeBurn instance...
	mTCBurner.RenderTimeCodeFont (GetAJAPixelFormat (mConfig.pixelFormat), fd.numPixels, fd.numLines);

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus QAPlayer::SetUpVideo ()
{
	if (mConfig.videoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_BAD_PARAM;
	if (!::NTV2DeviceCanDoVideoFormat (mDeviceID, mConfig.videoFormat))
		{cerr << "## ERROR:  This device cannot handle '" << ::NTV2VideoFormatString (mConfig.videoFormat) << "'" << endl;  return AJA_STATUS_UNSUPPORTED;}

	//	Configure the device to handle the requested video format...
	mDevice.SetVideoFormat (mConfig.videoFormat, false, false, mConfig.channel);

	//	Set the frame buffer pixel format for all the channels on the device.
	//	If the device doesn't support it, fall back to 8-bit YCbCr...
	if (!::NTV2DeviceCanDoFrameBufferFormat (mDeviceID, mConfig.pixelFormat))
	{
		cerr	<< "## NOTE:  Device cannot handle '" << ::NTV2FrameBufferFormatString (mConfig.pixelFormat) << "' -- using '"
				<< ::NTV2FrameBufferFormatString (NTV2_FBF_8BIT_YCBCR) << "' instead" << endl;
		mConfig.pixelFormat = NTV2_FBF_8BIT_YCBCR;
	}

	mDevice.SetFrameBufferFormat (mConfig.channel, mConfig.pixelFormat);
	mDevice.SetReference (mConfig.referenceSource);
	mDevice.EnableChannel (mConfig.channel);

	if (mConfig.withVanc && !::IsRGBFormat (mConfig.pixelFormat) && NTV2_IS_HD_VIDEO_FORMAT (mConfig.videoFormat))
	{
		//	Try enabling VANC...
		mDevice.SetEnableVANCData (true);		//	Enable VANC for non-SD formats, to pass thru captions, etc.
		if (::Is8BitFrameBufferFormat (mConfig.pixelFormat))
		{
			//	8-bit FBFs require VANC bit shift...
			mDevice.SetVANCShiftMode (mConfig.channel, NTV2_VANCDATA_8BITSHIFT_ENABLE);
			mDevice.SetVANCShiftMode (mConfig.channel, NTV2_VANCDATA_8BITSHIFT_ENABLE);
		}
	}	//	if HD video format
	else
		mDevice.SetEnableVANCData (false);	//	No VANC with RGB pixel formats (for now)

	if(mConfig.bufferSize != 0)
	{
		NTV2Framesize bufferSize = NTV2_MAX_NUM_Framesizes;
		switch(mConfig.bufferSize*2)
		{
		case 2:
			bufferSize = NTV2_FRAMESIZE_2MB;
			break;
		case 4:
			bufferSize = NTV2_FRAMESIZE_4MB;
			break;
		case 6:
			bufferSize = NTV2_FRAMESIZE_6MB;
			break;
		case 8:
			bufferSize = NTV2_FRAMESIZE_8MB;
			break;
		case 16:
			bufferSize = NTV2_FRAMESIZE_16MB;
			break;
		case 10:
			bufferSize = NTV2_FRAMESIZE_10MB;
			break;
		case 12:
			bufferSize = NTV2_FRAMESIZE_12MB;
			break;
		case 14:
			bufferSize = NTV2_FRAMESIZE_14MB;
			break;
		case 18:
			bufferSize = NTV2_FRAMESIZE_18MB;
			break;
		case 20:
			bufferSize = NTV2_FRAMESIZE_20MB;
			break;
		case 22:
			bufferSize = NTV2_FRAMESIZE_22MB;
			break;
		case 24:
			bufferSize = NTV2_FRAMESIZE_24MB;
			break;
		case 26:
			bufferSize = NTV2_FRAMESIZE_26MB;
			break;
		case 28:
			bufferSize = NTV2_FRAMESIZE_28MB;
			break;
		case 30:
			bufferSize = NTV2_FRAMESIZE_30MB;
			break;
		case 32:
			bufferSize = NTV2_FRAMESIZE_32MB;
			break;
		}

		if(bufferSize != NTV2_MAX_NUM_Framesizes)
			mDevice.SetFrameBufferSize(NTV2_CHANNEL1, bufferSize);
	}
	//	Subscribe the output interrupt -- it's enabled by default...
	mDevice.SubscribeOutputVerticalEvent (mConfig.channel);
	if (OutputDestHasRP188BypassEnabled ())
		DisableRP188Bypass ();

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo

AJAStatus QAPlayer::SetUp4kVideo()
{
	//	Unless a video format was requested, configure the board for ...
	if (mConfig.videoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_BAD_PARAM;

	if (!::NTV2DeviceCanDoVideoFormat(mDeviceID, mConfig.videoFormat))
	{
		cerr << "## ERROR:  This device cannot handle '" << ::NTV2VideoFormatToString(mConfig.videoFormat) << "'" << endl;  return AJA_STATUS_UNSUPPORTED;
	}

	//	Configure the device to handle the requested video format...
	mDevice.SetVideoFormat(mConfig.videoFormat, false, false, mConfig.channel);

	//	VANC data is not processed by this application
	mDevice.SetEnableVANCData(false, false);

	//	Set the frame buffer pixel format for all the channels on the device.
	//	If the device doesn't support it, fall back to 8-bit YCbCr...
	if (!::NTV2DeviceCanDoFrameBufferFormat(mDeviceID, mConfig.pixelFormat))
		mConfig.pixelFormat = NTV2_FBF_8BIT_YCBCR;

	if (mConfig.channel == NTV2_CHANNEL1)
	{
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL1, mConfig.pixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL2, mConfig.pixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL3, mConfig.pixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL4, mConfig.pixelFormat);
		mDevice.EnableChannel(NTV2_CHANNEL2);
		mDevice.EnableChannel(NTV2_CHANNEL3);
		mDevice.EnableChannel(NTV2_CHANNEL4);
		mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL1);
	}
	else
	{
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL5, mConfig.pixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL6, mConfig.pixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL7, mConfig.pixelFormat);
		mDevice.SetFrameBufferFormat(NTV2_CHANNEL8, mConfig.pixelFormat);
		mDevice.EnableChannel(NTV2_CHANNEL5);
		mDevice.EnableChannel(NTV2_CHANNEL6);
		mDevice.EnableChannel(NTV2_CHANNEL7);
		mDevice.EnableChannel(NTV2_CHANNEL8);
		mDevice.SubscribeOutputVerticalEvent(NTV2_CHANNEL5);
	}
	mDevice.SetReference(NTV2_REFERENCE_FREERUN);

	if (OutputDestHasRP188BypassEnabled())
		DisableRP188Bypass();

	return AJA_STATUS_SUCCESS;

}	//	SetUpVideo

AJAStatus QAPlayer::SetUpAudio ()
{
	const uint16_t	numberOfAudioChannels	(::NTV2DeviceGetMaxAudioChannels (mDeviceID));

	//	Set up the output audio embedders...
	mDevice.SetNumberAudioChannels (numberOfAudioChannels, mAudioSystem);

	//  Set the audio rate
	mDevice.SetAudioRate (NTV2_AUDIO_48K, mAudioSystem);

	//	How big should the on-device audio buffer be?   1MB? 2MB? 4MB? 8MB?
	//	For this demo, 4MB will work best across all platforms (Windows, Mac & Linux)...
	mDevice.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, mAudioSystem);

	mDevice.SetSDIOutputAudioSystem (mConfig.channel, mAudioSystem);

	//	If the last app using the device left it in end-to-end mode (input passthru),
	//	then loopback must be disabled, or else the output will contain whatever audio
	//	is present in whatever signal is feeding the device's SDI input...
	mDevice.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_OFF, mAudioSystem);

	return AJA_STATUS_SUCCESS;

}	//	SetUpAudio


void QAPlayer::SetUpHostBuffers ()
{
	//	Let my circular buffer know when it's time to quit...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);

	//	Calculate the size of the video buffer, which depends on video format, pixel format, and whether VANC is included or not...
	mVancEnabled = false;
	mWideVanc = false;
	mDevice.GetEnableVANCData (&mVancEnabled, &mWideVanc);
	mVideoBufferSize = GetVideoWriteSize (mConfig.videoFormat, mConfig.pixelFormat, mVancEnabled, mWideVanc);

	//	Calculate the size of the audio buffer, which mostly depends on the sample rate...
	NTV2AudioRate	audioRate	(NTV2_AUDIO_48K);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	const uint32_t	audioBufferSize	((audioRate == NTV2_AUDIO_96K) ? AUDIOBYTES_MAX_96K : AUDIOBYTES_MAX_48K);

	//	Allocate my buffers...
	for (size_t ndx = 0; ndx < CIRCULAR_BUFFER_SIZE; ndx++)
	{
		mAVHostBuffer [ndx].fVideoBuffer		= reinterpret_cast <uint32_t *> (new uint8_t [mVideoBufferSize]);
		mAVHostBuffer [ndx].fVideoBufferSize	= mVideoBufferSize;
		mAVHostBuffer [ndx].fAudioBuffer		= mConfig.withAudio ? reinterpret_cast <uint32_t *> (new uint8_t [audioBufferSize]) : NULL;
		mAVHostBuffer [ndx].fAudioBufferSize	= mConfig.withAudio ? audioBufferSize : 0;

		::memset (mAVHostBuffer [ndx].fVideoBuffer, 0x00, mVideoBufferSize);
		::memset (mAVHostBuffer [ndx].fAudioBuffer, 0x00, mConfig.withAudio ? audioBufferSize : 0);

		mAVCircularBuffer.Add (&mAVHostBuffer [ndx]);
	}	//	for each AV buffer in my circular buffer

	//	Allocate space for the ancillary data
	if (mConfig.ancBufferSize == 0)
	{
		//	User didn't specify a buffer size, so use the current driver setting
		mDevice.ReadRegister (kRegAncField1Offset, &mConfig.ancBufferSize);
	}
	else
	{
		//	Tell the driver how much data to expect
		mDevice.WriteRegister (kRegAncField1Offset, mConfig.ancBufferSize);
		mDevice.WriteRegister (kRegAncField2Offset, mConfig.ancBufferSize);
	}

	mpF1AncBuffer = new uint8_t [mConfig.ancBufferSize];
	mpF2AncBuffer = new uint8_t [mConfig.ancBufferSize];

}	//	SetUpHostBuffers


void QAPlayer::RouteOutputSignal ()
{
	const bool			isRGB			(IsRGBFormat (mConfig.pixelFormat));
	const NTV2Standard	videoStandard	(::GetNTV2StandardFromVideoFormat (mConfig.videoFormat));
	NTV2CrosspointID	cscXpt			(NTV2_XptBlack);
	NTV2CrosspointID	outputXpt		(NTV2_XptBlack);
	const bool			is3Ga			(IsVideoFormatA (mConfig.videoFormat));

	switch (mConfig.channel)
	{
		default:
		case NTV2_CHANNEL1:		outputXpt	= isRGB ? NTV2_XptFrameBuffer1RGB : NTV2_XptFrameBuffer1YUV;
								cscXpt		= NTV2_XptCSC1VidYUV;
								break;
		case NTV2_CHANNEL2:		outputXpt	= isRGB ? NTV2_XptFrameBuffer2RGB : NTV2_XptFrameBuffer2YUV;
								cscXpt		= NTV2_XptCSC2VidYUV;
								break;
		case NTV2_CHANNEL3:		outputXpt	= isRGB ? NTV2_XptFrameBuffer3RGB : NTV2_XptFrameBuffer3YUV;
								cscXpt		= NTV2_XptCSC3VidYUV;
								break;
		case NTV2_CHANNEL4:		outputXpt	= isRGB ? NTV2_XptFrameBuffer4RGB : NTV2_XptFrameBuffer4YUV;
								cscXpt		= NTV2_XptCSC4VidYUV;
								break;
		case NTV2_CHANNEL5:		outputXpt	= isRGB ? NTV2_XptFrameBuffer5RGB : NTV2_XptFrameBuffer5YUV;
								cscXpt		= NTV2_XptCSC5VidYUV;
								break;
		case NTV2_CHANNEL6:		outputXpt	= isRGB ? NTV2_XptFrameBuffer6RGB : NTV2_XptFrameBuffer6YUV;
								cscXpt		= NTV2_XptCSC6VidYUV;
								break;
		case NTV2_CHANNEL7:		outputXpt	= isRGB ? NTV2_XptFrameBuffer7RGB : NTV2_XptFrameBuffer7YUV;
								cscXpt		= NTV2_XptCSC7VidYUV;
								break;
		case NTV2_CHANNEL8:		outputXpt	= isRGB ? NTV2_XptFrameBuffer8RGB : NTV2_XptFrameBuffer8YUV;
								cscXpt		= NTV2_XptCSC8VidYUV;
								break;
	}

	if(!::NTV2DeviceCanDo3GLevelConversion(mDeviceID) && mConfig.levelConversion && is3Ga)
		mConfig.levelConversion = false;

	if(NTV2DeviceCanDo3GLevelConversion(mDeviceID))
		mDevice.SetSDIOutLevelAtoLevelBConversion(mConfig.channel, mConfig.levelConversion);

	if (isRGB)
	{
		//	Connect the frame buffer to the desired SDI output PLUS every non-SDI output.
		//	RGB must go through a Color Space Converter to get YUV output...
		if (mConfig.channel == NTV2_CHANNEL1)
		{
			if(!mConfig.useDL)
			{
				mDevice.Connect (NTV2_XptCSC1VidInput, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut1Input, cscXpt);
				mDevice.Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut1Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
				mDevice.Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);
			}
		}

		if ((mConfig.channel == NTV2_CHANNEL2)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut2) || ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut2)))
		{
			if(!mConfig.useDL)
			{
				mDevice.Connect (NTV2_XptCSC2VidInput, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut2Input, cscXpt);
				mDevice.Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut2Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut2);
				mDevice.Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptDuallinkOut2DS2);
			}
		}

		if ((mConfig.channel == NTV2_CHANNEL3)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut3) || ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut3)))
		{
			if(!mConfig.useDL)
			{
				mDevice.Connect (NTV2_XptCSC3VidInput, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut3Input, cscXpt);
				mDevice.Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut3Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);
				mDevice.Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptDuallinkOut3DS2);
			}
		}

		if ((mConfig.channel == NTV2_CHANNEL4)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut4) || ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut4)))
		{
			if(!mConfig.useDL)
			{
				mDevice.Connect (NTV2_XptCSC4VidInput, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut4Input, cscXpt);
				mDevice.Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut4Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut4);
				mDevice.Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptDuallinkOut4DS2);
			}
		}

		if ((mConfig.channel == NTV2_CHANNEL5)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut5) ))
		{
			if(!mConfig.useDL)
			{
				mDevice.Connect (NTV2_XptCSC5VidInput, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut5Input, cscXpt);
				mDevice.Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut5Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptDuallinkOut5);
				mDevice.Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptDuallinkOut5DS2);
			}
		}

		if ((mConfig.channel == NTV2_CHANNEL6)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut6) ))
		{
			if(!mConfig.useDL)
			{
				mDevice.Connect (NTV2_XptCSC6VidInput, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut6Input, cscXpt);
				mDevice.Connect (NTV2_XptSDIOut6InputDS2, NTV2_XptBlack);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut6Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut6Input, NTV2_XptDuallinkOut6);
				mDevice.Connect (NTV2_XptSDIOut6InputDS2, NTV2_XptDuallinkOut6DS2);
			}
		}

		if ((mConfig.channel == NTV2_CHANNEL7)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut7) ))
		{
			if(!mConfig.useDL)
			{
				mDevice.Connect (NTV2_XptCSC7VidInput, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut7Input, cscXpt);
				mDevice.Connect (NTV2_XptSDIOut7InputDS2, NTV2_XptBlack);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut7Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut7Input, NTV2_XptDuallinkOut7);
				mDevice.Connect (NTV2_XptSDIOut7InputDS2, NTV2_XptDuallinkOut7DS2);
			}
		}

		if ((mConfig.channel == NTV2_CHANNEL8)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut8) ))
		{
			if(!mConfig.useDL)
			{
				mDevice.Connect (NTV2_XptCSC8VidInput, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut8Input, cscXpt);
				mDevice.Connect (NTV2_XptSDIOut8InputDS2, NTV2_XptBlack);
			}
			else
			{
				mDevice.Connect (NTV2_XptDualLinkOut8Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut8Input, NTV2_XptDuallinkOut8);
				mDevice.Connect (NTV2_XptSDIOut8InputDS2, NTV2_XptDuallinkOut8DS2);
			}
		}
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1))
			mDevice.Connect (NTV2_XptHDMIOutInput, cscXpt);
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, cscXpt);
	}	//	if RGB frame buffer format
	else
	{
		//	Connect the frame buffer to the desired SDI output PLUS every non-SDI output.
		//	YUV needs no color space conversion...
		if (mConfig.channel == NTV2_CHANNEL1)
		{
			mDevice.Connect (NTV2_XptSDIOut1Input, outputXpt);
			mDevice.Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
		}


		if ((mConfig.channel == NTV2_CHANNEL2)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut2) || ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut2)))
		{
				mDevice.Connect (NTV2_XptSDIOut2Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
		}

		if ((mConfig.channel == NTV2_CHANNEL3)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut3) || ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut3)))
		{
				mDevice.Connect (NTV2_XptSDIOut3Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
		}

		if ((mConfig.channel == NTV2_CHANNEL4)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut4) || ::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtSDIOut4)))
		{
				mDevice.Connect (NTV2_XptSDIOut4Input, outputXpt);
				mDevice.Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}

		if ((mConfig.channel == NTV2_CHANNEL5)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut5) ))
		{
			mDevice.Connect (NTV2_XptSDIOut5Input, outputXpt);
			mDevice.Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
		}


		if ((mConfig.channel == NTV2_CHANNEL6)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut6) ))
		{
			mDevice.Connect (NTV2_XptSDIOut6Input, outputXpt);
			mDevice.Connect (NTV2_XptSDIOut6InputDS2, NTV2_XptBlack);
		}

		if ((mConfig.channel == NTV2_CHANNEL7)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut7) ))
		{
			mDevice.Connect (NTV2_XptSDIOut7Input, outputXpt);
			mDevice.Connect (NTV2_XptSDIOut7InputDS2, NTV2_XptBlack);
		}

		if ((mConfig.channel == NTV2_CHANNEL8)
			&& (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_Wgt3GSDIOut8) ))
		{
			mDevice.Connect (NTV2_XptSDIOut8Input, outputXpt);
			mDevice.Connect (NTV2_XptSDIOut8InputDS2, NTV2_XptBlack);
		}

		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1))
			mDevice.Connect (NTV2_XptHDMIOutInput, outputXpt);
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtAnalogOut1))
			mDevice.Connect (NTV2_XptAnalogOutInput, outputXpt);
	}	//	else YUV frame buffer format

	mDevice.SetSDIOutputStandard (mConfig.channel, videoStandard);

	//	Enable SDI output from the channel being used, but only if the device supports bi-directional SDI...
	if (::NTV2DeviceHasBiDirectionalSDI (mDeviceID))
		mDevice.SetSDITransmitEnable (mConfig.channel, true);

	#if defined (AJAMac)		//	Bug in Mac driver necessitates this
		if (::NTV2DeviceCanDoWidget (mDeviceID, NTV2_WgtHDMIOut1))
		{
			NTV2FrameRate	frameRate	(NTV2_FRAMERATE_UNKNOWN);

			//	HDMI needs a bit more set-up...
			mDevice.GetFrameRate (&frameRate);
			mDevice.SetHDMIOutVideoStandard (videoStandard);
			mDevice.SetHDMIOutVideoFPS (frameRate);
		}
	#endif	//	AJAMac

}	//	RouteOutputSignal

void QAPlayer::Route4kOutputSignal()
{
	const bool			isRGB(::IsRGBFormat(mConfig.pixelFormat));
	const NTV2Standard	videoStandard(::GetNTV2StandardFromVideoFormat(mConfig.videoFormat));
	bool mUseHDMIOut = false;
	if (NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtHDMIOut1v2))
		mUseHDMIOut = true;

	if (isRGB && mConfig.channel == NTV2_CHANNEL1)
	{
		//	For RGB, connect CSCs between the frame buffer outputs ans the SDI outputs
		mDevice.Connect (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1RGB);
		mDevice.Connect (NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);

		mDevice.Connect (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2RGB);
		mDevice.Connect (NTV2_XptSDIOut2Input, NTV2_XptCSC2VidYUV);

		mDevice.Connect (NTV2_XptCSC3VidInput, NTV2_XptFrameBuffer3RGB);
		mDevice.Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC3VidYUV);

		mDevice.Connect (NTV2_XptCSC4VidInput, NTV2_XptFrameBuffer4RGB);
		mDevice.Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC4VidYUV);

		if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_Wgt4KDownConverter) && ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtSDIMonOut1))
		{
			mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer1RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer2RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer3RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer4RGB);

			mDevice.Enable4KDCRGBMode(true);

			mDevice.Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut);
			mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV);
			mDevice.SetSDIOutputStandard(NTV2_CHANNEL1, videoStandard);
		}

		if (mUseHDMIOut && ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtHDMIOut1v2))
		{
			mDevice.Connect (NTV2_XptHDMIOutInput, NTV2_XptCSC1VidYUV);
			mDevice.Connect (NTV2_XptHDMIOutQ2Input, NTV2_XptCSC2VidYUV);
			mDevice.Connect (NTV2_XptHDMIOutQ3Input, NTV2_XptCSC3VidYUV);
			mDevice.Connect (NTV2_XptHDMIOutQ4Input, NTV2_XptCSC4VidYUV);

			mDevice.SetHDMIV2TxBypass(false);
			mDevice.SetHDMIOutVideoStandard(::GetNTV2StandardFromVideoFormat(mConfig.videoFormat));
			mDevice.SetHDMIOutVideoFPS(::GetNTV2FrameRateFromVideoFormat(mConfig.videoFormat));
			mDevice.SetLHIHDMIOutColorSpace(NTV2_LHIHDMIColorSpaceYCbCr);
			mDevice.SetHDMIV2Mode(NTV2_HDMI_V2_4K_PLAYBACK);
		}
	}	//	if isRGB
	else if (mConfig.channel == NTV2_CHANNEL1)
	{
		//	For YCbCr, the frame buffer outputs feed the SDI outputs directly
		mDevice.Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);
		mDevice.Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameBuffer2YUV);
		mDevice.Connect (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer3YUV);
		mDevice.Connect (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer4YUV);

		if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_Wgt4KDownConverter) && ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtSDIMonOut1) && mConfig.channel == NTV2_CHANNEL1)
		{
			mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer1YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer2YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer3YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer4YUV);

			mDevice.Enable4KDCRGBMode(false);

			mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut);
			mDevice.SetSDIOutputStandard(NTV2_CHANNEL5, videoStandard);
		}

		if (mUseHDMIOut && ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtHDMIOut1v2) && mConfig.channel == NTV2_CHANNEL1)
		{
			mDevice.Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameBuffer1YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ2Input, NTV2_XptFrameBuffer2YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ3Input, NTV2_XptFrameBuffer3YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ4Input, NTV2_XptFrameBuffer4YUV);

			mDevice.SetHDMIV2TxBypass(false);
			mDevice.SetHDMIOutVideoStandard(::GetNTV2StandardFromVideoFormat(mConfig.videoFormat));
			mDevice.SetHDMIOutVideoFPS(::GetNTV2FrameRateFromVideoFormat(mConfig.videoFormat));
			mDevice.SetLHIHDMIOutColorSpace(NTV2_LHIHDMIColorSpaceYCbCr);
			mDevice.SetHDMIV2Mode(NTV2_HDMI_V2_4K_PLAYBACK);
		}
	}	//	else YUV
	else if (isRGB && mConfig.channel == NTV2_CHANNEL5)
	{
		//	For RGB, connect CSCs between the frame buffer outputs ans the SDI outputs
		mDevice.Connect (NTV2_XptCSC5VidInput, NTV2_XptFrameBuffer5RGB);
		mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV);

		mDevice.Connect (NTV2_XptCSC6VidInput, NTV2_XptFrameBuffer6RGB);
		mDevice.Connect (NTV2_XptSDIOut6Input, NTV2_XptCSC6VidYUV);

		mDevice.Connect (NTV2_XptCSC7VidInput, NTV2_XptFrameBuffer7RGB);
		mDevice.Connect (NTV2_XptSDIOut7Input, NTV2_XptCSC7VidYUV);

		mDevice.Connect (NTV2_XptCSC8VidInput, NTV2_XptFrameBuffer8RGB);
		mDevice.Connect (NTV2_XptSDIOut8Input, NTV2_XptCSC8VidYUV);

		if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_Wgt4KDownConverter) && ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtSDIMonOut1))
		{
			mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer5RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer6RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer7RGB);
			mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer8RGB);

			mDevice.Enable4KDCRGBMode(true);

			mDevice.Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut);
			mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV);
		}

		if (mUseHDMIOut && ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtHDMIOut1v2))
		{
			mDevice.Connect (NTV2_XptHDMIOutInput, NTV2_XptCSC5VidYUV);
			mDevice.Connect (NTV2_XptHDMIOutQ2Input, NTV2_XptCSC6VidYUV);
			mDevice.Connect (NTV2_XptHDMIOutQ3Input, NTV2_XptCSC7VidYUV);
			mDevice.Connect (NTV2_XptHDMIOutQ4Input, NTV2_XptCSC8VidYUV);

			mDevice.SetHDMIV2TxBypass(false);
			mDevice.SetHDMIOutVideoStandard(::GetNTV2StandardFromVideoFormat(mConfig.videoFormat));
			mDevice.SetHDMIOutVideoFPS(::GetNTV2FrameRateFromVideoFormat(mConfig.videoFormat));
			mDevice.SetLHIHDMIOutColorSpace(NTV2_LHIHDMIColorSpaceYCbCr);
			mDevice.SetHDMIV2Mode(NTV2_HDMI_V2_4K_PLAYBACK);
		}
	}
	else
	{
		//	For YCbCr, the frame buffer outputs feed the SDI outputs directly
		mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_XptFrameBuffer5YUV);
		mDevice.Connect (NTV2_XptSDIOut6Input, NTV2_XptFrameBuffer6YUV);
		mDevice.Connect (NTV2_XptSDIOut7Input, NTV2_XptFrameBuffer7YUV);
		mDevice.Connect (NTV2_XptSDIOut8Input, NTV2_XptFrameBuffer8YUV);

		if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_Wgt4KDownConverter) && ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtSDIMonOut1) && mConfig.channel == NTV2_CHANNEL1)
		{
			mDevice.Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptFrameBuffer5YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptFrameBuffer6YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptFrameBuffer7YUV);
			mDevice.Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptFrameBuffer8YUV);

			mDevice.Enable4KDCRGBMode(false);

			mDevice.Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut);
		}

		if (mUseHDMIOut && ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtHDMIOut1v2) && mConfig.channel == NTV2_CHANNEL1)
		{
			mDevice.Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameBuffer5YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ2Input, NTV2_XptFrameBuffer6YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ3Input, NTV2_XptFrameBuffer7YUV);
			mDevice.Connect (NTV2_XptHDMIOutQ4Input, NTV2_XptFrameBuffer8YUV);

			mDevice.SetHDMIV2TxBypass(false);
			mDevice.SetHDMIOutVideoStandard(::GetNTV2StandardFromVideoFormat(mConfig.videoFormat));
			mDevice.SetHDMIOutVideoFPS(::GetNTV2FrameRateFromVideoFormat(mConfig.videoFormat));
			mDevice.SetLHIHDMIOutColorSpace(NTV2_LHIHDMIColorSpaceYCbCr);
			mDevice.SetHDMIV2Mode(NTV2_HDMI_V2_4K_PLAYBACK);
		}
	}


	//	Enable SDI output from all channels,
	//	but only if the device supports bi-directional SDI.
	if (::NTV2DeviceHasBiDirectionalSDI(mDeviceID))
	{
		if (mConfig.channel == NTV2_CHANNEL1)
		{
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL1, true);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL2, true);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL3, true);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL4, true);
		}
		else
		{
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL5, true);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL6, true);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL7, true);
			mDevice.SetSDITransmitEnable(NTV2_CHANNEL8, true);
		}
	}

}	//	RouteOutputSignal


void QAPlayer::SetUpOutputAutoCirculate ()
{
	uint32_t	startFrame	(0);
	uint32_t	endFrame	(7);

	uint32_t		numChannels(::NTV2DeviceGetNumVideoChannels(mDeviceID));
	NTV2Framesize	bufferSize(NTV2_MAX_NUM_Framesizes);
	uint32_t		numBuffers(::NTV2DeviceGetNumberFrameBuffers(mDeviceID));

	//	This is just some stuff to manage channel boundaries
	mDevice.GetFrameBufferSize(NTV2_CHANNEL1, &bufferSize);
	numBuffers -= ::NTV2DeviceGetNumAudioSystems(mDeviceID);
	if (bufferSize == NTV2_FRAMESIZE_16MB)
		numBuffers /= 2;

	uint32_t		buffersPerChannel(numBuffers / numChannels);

	startFrame = mConfig.channel * buffersPerChannel;
	endFrame = (startFrame + buffersPerChannel - 1);

	mDevice.AutoCirculateStop(mConfig.channel);
	mDevice.WaitForOutputFieldID(NTV2_FIELD0, NTV2_CHANNEL1);
	mDevice.AutoCirculateInitForOutput(mConfig.channel, buffersPerChannel,						//	Request 10 frame buffers
										mConfig.withAudio ? mAudioSystem : NTV2_AUDIOSYSTEM_INVALID,	//	Which audio system?
										AUTOCIRCULATE_WITH_RP188 |								//	Add RP188 timecode!
										(mConfig.insertAncLine ? AUTOCIRCULATE_WITH_ANC : 0),	//	and possibly Anc packets
										1, startFrame, endFrame);

}	//	SetUpOutputAutoCirculate


AJAStatus QAPlayer::Run ()
{
	//	Start my consumer and producer threads...
	StartConsumerThread ();
	StartProducerThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run



//////////////////////////////////////////////
//	This is where the play thread starts

void QAPlayer::StartConsumerThread ()
{
	//	Create and start the playout thread...
	mConsumerThread = new AJAThread ();
	mConsumerThread->Attach (ConsumerThreadStatic, this);
	mConsumerThread->SetPriority (AJA_ThreadPriority_High);
	mConsumerThread->Start ();

}	//	StartConsumerThread


//	The playout thread function
void QAPlayer::ConsumerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	//	Grab the NTV2Player instance pointer from the pContext parameter,
	//	then call its PlayFrames method...
	QAPlayer *	pApp	(reinterpret_cast <QAPlayer *> (pContext));
	if (pApp)
		pApp->PlayFrames ();

}	//	ConsumerThreadStatic


void QAPlayer::PlayFrames (void)
{
	AUTOCIRCULATE_TRANSFER	xferStruct;

	//	Start AutoCirculate running...
	mDevice.AutoCirculateStart(mConfig.channel);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS	outputStatus;
		GetACStatus (outputStatus);

		//	Check if there's room for another frame on the card...
		if ((outputStatus.acBufferLevel < (ULWord)(outputStatus.acEndFrame - outputStatus.acStartFrame - 1)))
		{
			//	Wait for the next frame to become ready to "consume"...
			AVDataBuffer *	playData	(mAVCircularBuffer.StartConsumeNextBuffer ());

			//	Transfer the timecode-burned frame to the device for playout...
			xferStruct.SetVideoBuffer(playData->fVideoBuffer, playData->fVideoBufferSize);
			if (mConfig.withAudio)
				xferStruct.SetAudioBuffer(playData->fAudioBuffer, playData->fAudioBufferSize);
			else
				xferStruct.SetAudioBuffer(NULL, 0);
			xferStruct.SetOutputTimeCode(NTV2_RP188(playData->fRP188Data), ::NTV2ChannelToTimecodeIndex(mConfig.channel, true));
			xferStruct.SetOutputTimeCode(NTV2_RP188(playData->fRP188Data), ::NTV2ChannelToTimecodeIndex(mConfig.channel, false));
			if (mConfig.channel == NTV2_CHANNEL1)
				xferStruct.SetOutputTimeCode(NTV2_RP188(playData->fRP188Data), NTV2_TCINDEX_LTC1);

			if (mConfig.insertAncLine)
			{
				uint8_t * pF1PacketAddr = mpF1AncBuffer;
				uint8_t * pF2PacketAddr = mpF2AncBuffer;

				for (uint32_t i = 0;  i < mConfig.numberOfAnc;  i++)
				{
					memcpy((void*)pF1PacketAddr, (void*)&ancPacket, sizeof (ancPacket));
					*(pF1PacketAddr + 2) = mConfig.insertAncLine + (i / mPacketsPerLine);

					memcpy((void*)pF2PacketAddr, (void*)pF1PacketAddr, sizeof (ancPacket));

					pF1PacketAddr += sizeof (ancPacket);
					pF2PacketAddr += sizeof (ancPacket);
				}

				xferStruct.SetAncBuffers(reinterpret_cast <ULWord *> (mpF1AncBuffer),
													sizeof (ancPacket)* mConfig.numberOfAnc,
													reinterpret_cast <ULWord *> (mpF2AncBuffer),
													sizeof (ancPacket)* mConfig.numberOfAnc);
			}

			mDevice.AutoCirculateTransfer(mConfig.channel, xferStruct);

			//	Signal that the frame has been "consumed"...
			mAVCircularBuffer.EndConsumeNextBuffer ();
		}
		else
			mDevice.WaitForOutputVerticalInterrupt (mConfig.channel);
	}	//	loop til quit signaled

	//	Stop AutoCirculate...
	mDevice.AutoCirculateStop(mConfig.channel);

}	//	PlayFrames



//////////////////////////////////////////////
//	This is where the producer thread starts

void QAPlayer::StartProducerThread ()
{
	//	Create and start the producer thread...
	mProducerThread = new AJAThread ();
	mProducerThread->Attach (ProducerThreadStatic, this);
	mProducerThread->SetPriority (AJA_ThreadPriority_High);
	mProducerThread->Start ();

}	//	StartProducerThread


void QAPlayer::ProducerThreadStatic (AJAThread * pThread, void * pContext)		//	static
{
	(void) pThread;

	QAPlayer *	pApp	(reinterpret_cast <QAPlayer *> (pContext));
	if (pApp)
		pApp->ProduceFrames ();

}	//	ProducerThreadStatic


AJAStatus QAPlayer::SetUpTestPatternVideoBuffers (void)
{
	AJATestPatternSelect	testPatternTypes []	=	{AJA_TestPatt_ColorBars100,
													AJA_TestPatt_MultiPattern,
													AJA_TestPatt_Ramp,
													AJA_TestPatt_ColorBars75,
													AJA_TestPatt_LineSweep,
													AJA_TestPatt_CheckField,
													AJA_TestPatt_FlatField,
													AJA_TestPatt_MultiBurst};

	mNumTestPatterns = sizeof (testPatternTypes) / sizeof (AJATestPatternSelect);
	mTestPatternVideoBuffers = new uint8_t * [mNumTestPatterns];
	::memset (mTestPatternVideoBuffers, 0, mNumTestPatterns * sizeof (uint8_t *));

	//	Set up one video buffer for each of the several predefined patterns...
	for (int testPatternIndex = 0; testPatternIndex < mNumTestPatterns; testPatternIndex++)
	{
		//	Allocate the buffer memory...
		mTestPatternVideoBuffers [testPatternIndex] = new uint8_t [mVideoBufferSize];

		//	Use a convenient AJA test pattern generator object to populate an AJATestPatternBuffer with test pattern data...
		AJATestPatternBuffer	testPatternBuffer;
		AJATestPatternGen		testPatternGen;

		NTV2FormatDescriptor	formatDesc(GetFormatDescriptor(mConfig.videoFormat, mConfig.pixelFormat, mVancEnabled, mWideVanc));

		if (!testPatternGen.DrawTestPattern (testPatternTypes [testPatternIndex],
											formatDesc.numPixels,
											formatDesc.numLines - formatDesc.firstActiveLine,
											GetAJAPixelFormat (mConfig.pixelFormat),
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
				if (mConfig.pixelFormat == NTV2_FBF_10BIT_YCBCR)
				{
					::Make10BitBlackLine (reinterpret_cast <UWord *> (pVancLine), formatDesc.numPixels);
					::PackLine_16BitYUVto10BitYUV (reinterpret_cast <const UWord *> (pVancLine), reinterpret_cast <ULWord *> (pVancLine), formatDesc.numPixels);
				}
				else if (mConfig.pixelFormat == NTV2_FBF_8BIT_YCBCR)
					::Make8BitBlackLine (pVancLine, formatDesc.numPixels);
				else
				{
					cerr << "## ERROR:  Cannot initialize video buffer's VANC area" << endl;
					return AJA_STATUS_FAIL;
				}
				pVancLine += formatDesc.linePitch * 4;
			}	//	for each VANC line
		}	//	if has VANC area

		//	Copy the contents of the AJATestPatternBuffer into my video buffer...
		uint8_t *	pVideoBuffer	(mTestPatternVideoBuffers [testPatternIndex] + formatDesc.firstActiveLine * formatDesc.linePitch * 4);
		for (size_t ndx = 0; ndx < testPatternSize; ndx++)
			pVideoBuffer [ndx] = testPatternBuffer [ndx];

	}	//	loop for each predefined pattern

	return AJA_STATUS_SUCCESS;

}	//	SetUpTestPatternVideoBuffers


void QAPlayer::ProduceFrames (void)
{
	AJATimeBase	timeBase (GetAJAFrameRate (GetNTV2FrameRateFromVideoFormat (mConfig.videoFormat)));

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
			//	Copy my pre-made test pattern into my video buffer...
			::memcpy (frameData->fVideoBuffer, mTestPatternVideoBuffers [mConfig.channel], mVideoBufferSize);

			const	NTV2FrameRate	ntv2FrameRate	(GetNTV2FrameRateFromVideoFormat (mConfig.videoFormat));
			const	TimecodeFormat	tcFormat		(NTV2FrameRate2TimecodeFormat (ntv2FrameRate));
			const	CRP188			rp188Info		(mFramesProduced++, tcFormat);
			string					timeCodeString;

			rp188Info.GetRP188Reg (frameData->fRP188Data);
			rp188Info.GetRP188Str (timeCodeString);

			if(mConfig.withBurn)
			{
				//	Burn the current timecode into the test pattern image that's now in my video buffer...
				mTCBurner.BurnTimeCode (reinterpret_cast <char *> (frameData->fVideoBuffer), timeCodeString.c_str (), 80);
			}

			//	Generate audio tone data...
			frameData->fAudioBufferSize		= mConfig.withAudio ? AddTone (frameData->fAudioBuffer) : 0;
		}

		//	Signal that I'm done producing the buffer -- it's now available for playout...
		mAVCircularBuffer.EndProduceNextBuffer ();

	}	//	loop til mGlobalQuit goes true

}	//	ProduceFrames


void QAPlayer::GetACStatus (AUTOCIRCULATE_STATUS & outputStatus)
{
	mDevice.AutoCirculateGetStatus(mConfig.channel, outputStatus);
}


static uint32_t					gFrameTally (500);					//	Non-PCM audio channel pair testing
static NTV2AudioChannelPair		gNonPCMPair	(NTV2_AudioChannel1_2);	//	Non-PCM audio channel pair testing


uint32_t QAPlayer::AddTone (ULWord * pInAudioBuffer)
{
	NTV2FrameRate	frameRate;
	NTV2AudioRate	audioRate;
	ULWord			numChannels;

	mDevice.GetFrameRate (&frameRate, mConfig.channel);
	mDevice.GetAudioRate (audioRate, mAudioSystem);
	mDevice.GetNumberAudioChannels (numChannels, mAudioSystem);

	/**
		Because audio on AJA devices use fixed sample rates (typically 48KHz), certain video frame rates will necessarily
		result in some frames having more audio samples than others. The GetAudioSamplesPerFrame function
		is used to calculate the correct sample count...
	**/
	const ULWord	numSamples		(::GetAudioSamplesPerFrame (frameRate, audioRate, mFramesProduced));
	const double	audioSampleRate	(audioRate == NTV2_AUDIO_96K ? 96000.0 : 48000.0);

	double toneFrequency[16] = { 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0, 440.0 };
	double	gAmplitudes [32] = {	0.010, 0.015,	0.020, 0.025,	0.030, 0.035,	0.040, 0.045,	0.050, 0.055,	0.060, 0.065,	0.070, 0.075,	0.080, 0.085,
									0.085, 0.080,	0.075, 0.070,	0.065, 0.060,	0.055, 0.050,	0.045, 0.040,	0.035, 0.030,	0.025, 0.020,	0.015, 0.010};

	const uint32_t	totalNumBytes = ::AddAudioTone (pInAudioBuffer,		//	audio buffer to fill
													mCurrentSample,		//	which sample for continuing the waveform
													numSamples,			//	number of samples to generate
													audioSampleRate,	//	sample rate [Hz]
													gAmplitudes,		//	amplitude
													&toneFrequency[0],	//	tone frequency [Hz]
													31,					//	bits per sample
													false,				//	don't byte swap
													numChannels);		//	number of audio channels to generate

	if (mConfig.doNonPCMAudio && ::NTV2DeviceCanDoPCMControl (mDeviceID))
	{
		//////////////////////////////////////////////////////////	BEGIN NON-PCM CHANNEL PAIR	////////////////////////////////////////////////
		NTV2AudioChannelPairs	nonPCMPairs, desiredNonPCMPairs;
		mDevice.GetAudioPCMControl (mAudioSystem, nonPCMPairs);
		if (++gFrameTally > 500)
		{
			gNonPCMPair = NTV2AudioChannelPair (gNonPCMPair + 1);
			if (!NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16 (gNonPCMPair))
				gNonPCMPair = NTV2_AudioChannel1_2;
			desiredNonPCMPairs.insert (gNonPCMPair);
			mDevice.SetAudioPCMControl (mAudioSystem, desiredNonPCMPairs);
			cerr << "## NOTE:  Switched non-PCM audio channel pairs from '" << nonPCMPairs << "' to '" << desiredNonPCMPairs << "'" << endl;
			gFrameTally = 0;
		}

		//	Insert non-PCM data into audio buffer...
		if (NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16 (gNonPCMPair) && totalNumBytes)
		{
			const uint32_t	bytesPerSample	(numChannels * sizeof (uint32_t));
			uint32_t		bytesRemaining	(totalNumBytes);
			uint32_t *		pSample			(pInAudioBuffer);
			while (bytesRemaining > bytesPerSample)
			{
				pSample [2 * gNonPCMPair + 0] = 0x7BFBFBFB;
				pSample [2 * gNonPCMPair + 1] = 0x7AFAFAFA;
				pSample += numChannels;
				bytesRemaining -= bytesPerSample;
			}
		}
		//////////////////////////////////////////////////////////	END NON-PCM CHANNEL PAIR	////////////////////////////////////////////////
	}
	return totalNumBytes;

}	//	AddTone


ULWord QAPlayer::GetRP188RegisterForOutput (const NTV2OutputDestination inOutputDest)		//	static
{
	switch (inOutputDest)
	{
		case NTV2_OUTPUTDESTINATION_SDI1:	return kRegRP188InOut1DBB;	break;	//	reg 29
		case NTV2_OUTPUTDESTINATION_SDI2:	return kRegRP188InOut2DBB;	break;	//	reg 64
		case NTV2_OUTPUTDESTINATION_SDI3:	return kRegRP188InOut3DBB;	break;	//	reg 268
		case NTV2_OUTPUTDESTINATION_SDI4:	return kRegRP188InOut4DBB;	break;	//	reg 273
		case NTV2_OUTPUTDESTINATION_SDI5:	return kRegRP188InOut5DBB;	break;	//	reg 29
		case NTV2_OUTPUTDESTINATION_SDI6:	return kRegRP188InOut6DBB;	break;	//	reg 64
		case NTV2_OUTPUTDESTINATION_SDI7:	return kRegRP188InOut7DBB;	break;	//	reg 268
		case NTV2_OUTPUTDESTINATION_SDI8:	return kRegRP188InOut8DBB;	break;	//	reg 273
		default:							return 0;					break;
	}	//	switch on output destination

}	//	GetRP188RegisterForOutput


bool QAPlayer::OutputDestHasRP188BypassEnabled (void)
{
	bool			result		(false);
	const ULWord	regNum		(GetRP188RegisterForOutput (mConfig.outputDestination));
	ULWord			regValue	(0);

	//
	//	Bit 23 of the RP188 DBB register will be set if output timecode will be
	//	grabbed directly from an input (bypass source)...
	//
	if (regNum && mDevice.ReadRegister (regNum, &regValue) && regValue & BIT(23))
		result = true;

	return result;

}	//	OutputDestHasRP188BypassEnabled


void QAPlayer::DisableRP188Bypass (void)
{
	const ULWord	regNum	(GetRP188RegisterForOutput (mConfig.outputDestination));

	//
	//	Clear bit 23 of my output destination's RP188 DBB register...
	//
	if (regNum)
		mDevice.WriteRegister (regNum, 0, BIT(23), BIT(23));

}	//	DisableRP188Bypass


AJA_FrameRate QAPlayer::GetAJAFrameRate (NTV2FrameRate frameRate)
{
	switch (frameRate)
	{
		default:
		case NTV2_FRAMERATE_UNKNOWN:	return AJA_FrameRate_Unknown;
		case NTV2_FRAMERATE_1498:		return AJA_FrameRate_1498;
		case NTV2_FRAMERATE_1500:		return AJA_FrameRate_1500;
		case NTV2_FRAMERATE_1798:		return AJA_FrameRate_1798;
		case NTV2_FRAMERATE_1800:		return AJA_FrameRate_1800;
		case NTV2_FRAMERATE_1898:		return AJA_FrameRate_1898;
		case NTV2_FRAMERATE_1900:		return AJA_FrameRate_1900;
		case NTV2_FRAMERATE_5000:		return AJA_FrameRate_5000;
		case NTV2_FRAMERATE_2398:		return AJA_FrameRate_2398;
		case NTV2_FRAMERATE_2400:		return AJA_FrameRate_2400;
		case NTV2_FRAMERATE_2500:		return AJA_FrameRate_2500;
		case NTV2_FRAMERATE_2997:		return AJA_FrameRate_2997;
		case NTV2_FRAMERATE_3000:		return AJA_FrameRate_3000;
		case NTV2_FRAMERATE_4795:		return AJA_FrameRate_4795;
		case NTV2_FRAMERATE_4800:		return AJA_FrameRate_4800;
		case NTV2_FRAMERATE_5994:		return AJA_FrameRate_5994;
		case NTV2_FRAMERATE_6000:		return AJA_FrameRate_6000;
	}
}	//	GetAJAFrameRate


AJA_PixelFormat QAPlayer::GetAJAPixelFormat (NTV2FrameBufferFormat format)
{
	switch (format)
	{
		default:
		case NTV2_FBF_NUMFRAMEBUFFERFORMATS:	return AJA_PixelFormat_Unknown;
		case NTV2_FBF_10BIT_YCBCR:				return AJA_PixelFormat_YCbCr10;
		case NTV2_FBF_8BIT_YCBCR:				return AJA_PixelFormat_YCbCr8;
		case NTV2_FBF_ARGB:						return AJA_PixelFormat_ARGB8;
		case NTV2_FBF_RGBA:						return AJA_PixelFormat_RGBA8;
		case NTV2_FBF_10BIT_RGB:				return AJA_PixelFormat_RGB10;
		case NTV2_FBF_8BIT_YCBCR_YUY2:			return AJA_PixelFormat_YUY28;
		case NTV2_FBF_ABGR:						return AJA_PixelFormat_ABGR8;
		case NTV2_FBF_10BIT_DPX:				return AJA_PixelFormat_RGB_DPX;
		case NTV2_FBF_10BIT_YCBCR_DPX:			return AJA_PixelFormat_YCbCr_DPX;
		case NTV2_FBF_8BIT_DVCPRO:				return AJA_PixelFormat_DVCPRO;
		case NTV2_FBF_8BIT_QREZ:				return AJA_PixelFormat_QREZ;
		case NTV2_FBF_8BIT_HDV:					return AJA_PixelFormat_HDV;
		case NTV2_FBF_24BIT_RGB:				return AJA_PixelFormat_RGB8_PACK;
		case NTV2_FBF_24BIT_BGR:				return AJA_PixelFormat_BGR8_PACK;
		case NTV2_FBF_10BIT_YCBCRA:				return AJA_PixelFormat_YCbCrA10;
		case NTV2_FBF_10BIT_DPX_LITTLEENDIAN:	return AJA_PixelFormat_RGB_DPX_LE;
		case NTV2_FBF_48BIT_RGB:				return AJA_PixelFormat_RGB12;
		case NTV2_FBF_PRORES:					return AJA_PixelFormat_PRORES;
		case NTV2_FBF_PRORES_DVCPRO:			return AJA_PixelFormat_PRORES_DVPRO;
		case NTV2_FBF_PRORES_HDV:				return AJA_PixelFormat_PRORES_HDV;
		case NTV2_FBF_10BIT_RGB_PACKED:			return AJA_PixelFormat_RGB10_PACK;
	}
}	//	GetAJAPixelFormat


TimecodeFormat QAPlayer::NTV2FrameRate2TimecodeFormat (const NTV2FrameRate inFrameRate)	//	static
{
	TimecodeFormat	result (kTCFormatUnknown);
	switch (inFrameRate)
	{
		case NTV2_FRAMERATE_6000:	result = kTCFormat60fps;	break;
		case NTV2_FRAMERATE_5994:	result = kTCFormat60fpsDF;	break;
		case NTV2_FRAMERATE_4800:	result = kTCFormat48fps;	break;
		case NTV2_FRAMERATE_4795:	result = kTCFormat48fps;	break;
		case NTV2_FRAMERATE_3000:	result = kTCFormat30fps;	break;
		case NTV2_FRAMERATE_2997:	result = kTCFormat30fpsDF;	break;
		case NTV2_FRAMERATE_2500:	result = kTCFormat25fps;	break;
		case NTV2_FRAMERATE_2400:	result = kTCFormat24fps;	break;
		case NTV2_FRAMERATE_2398:	result = kTCFormat24fps;	break;
		case NTV2_FRAMERATE_5000:	result = kTCFormat50fps;	break;
		default:					break;
	}
	return result;

}	//	NTV2FrameRate2TimecodeFormat


void QAPlayer::GetCallback (void ** const pInstance, QAPlayerCallback ** const callback)
{
	*pInstance = mInstance;
	*callback = mPlayerCallback;
}	//	GetCallback


bool QAPlayer::SetCallback (void * const pInstance, QAPlayerCallback * const callback)
{
	mInstance = pInstance;
	mPlayerCallback = callback;

	return true;
}	//	SetCallback

