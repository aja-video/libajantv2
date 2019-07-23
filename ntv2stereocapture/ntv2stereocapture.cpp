/**
	@file		ntv2stereocapture.cpp
	@brief		Contains the implementation of the NTV2StereoCapture class.
	@copyright	Copyright (C) 2013 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2stereocapture.h"
#include "ajabase/common/types.h"


NTV2StereoCapture::NTV2StereoCapture (uint16_t boardNumber)
	:	mCaptureThread			(NULL),
		mPreviewStereoThread	(NULL),
		mPixelFormat			(NTV2_FBF_ARGB),
		mGlobalQuit				(false)
{
}	//	constructor


NTV2StereoCapture::~NTV2StereoCapture ()
{
	Quit ();

	delete mCaptureThread;
	mCaptureThread = NULL;

	delete mPreviewStereoThread;
	mPreviewStereoThread = NULL;

    mDevice.UnsubscribeInputVerticalEvent(NTV2_CHANNEL1);
    mDevice.UnsubscribeOutputVerticalEvent(NTV2_CHANNEL1);

	mDevice.SetEveryFrameServices(mPreviousFrameServices);
	mDevice.ReleaseStreamForApplication(AJA_FOURCC('D','E','M','O'), static_cast<int32_t>(AJAProcess::GetPid()));

}	//	destructor


void NTV2StereoCapture::Quit (void)
{
	mGlobalQuit = true;

	if (mCaptureThread)
		while (mCaptureThread->Active ())
			AJATime::Sleep (10);

	if (mPreviewStereoThread)
		while (mPreviewStereoThread->Active ())
			AJATime::Sleep (10);

}	//	Quit


AJAStatus NTV2StereoCapture::Init (const CNTV2DeviceScanner & inBoardScanner, const uint32_t inBoardNumber)
{
	//	Any AJA devices out there?
    if (!inBoardScanner.GetNumDevices ())
		return AJA_STATUS_OPEN;

	//	Using a reference to the discovered board list, and the index number of the device
	//	of interest (mBoardNumber), get information about that particular device...
    NTV2DeviceInfo	info	(inBoardScanner.GetDeviceInfoList () [inBoardNumber]);
    if (!mDevice.Open (info.deviceIndex, false))
		return AJA_STATUS_OPEN;

	if (!mDevice.AcquireStreamForApplication(AJA_FOURCC('D','E','M','O'), static_cast<int32_t>(AJAProcess::GetPid())))
	{
		mDevice.Close();
		return AJA_STATUS_BUSY;
	}
	else
	{
		//Save the current state before we change it
		mDevice.GetEveryFrameServices(&mPreviousFrameServices);
		//Since this is an OEM demo we will set the OEM service level
		mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);
	}

	//	Since this is an OEM demo, I'll set the OEM service level...
	mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);

	//	Store the board ID in a member because it will be used frequently...
    mBoardID = mDevice.GetDeviceID ();

	//	Sometimes the frame buffers get disabled, so turn them all on here...
    if (NTV2DeviceGetNumFrameStores (mBoardID) == 2)
        mDevice.EnableChannel (NTV2_CHANNEL2);

    if (NTV2DeviceGetNumFrameStores (mBoardID) == 4)
	{
        mDevice.EnableChannel (NTV2_CHANNEL2);
        mDevice.EnableChannel (NTV2_CHANNEL3);
        mDevice.EnableChannel (NTV2_CHANNEL4);
	}

	AJAStatus	status	= SetupVideo ();
	if (AJA_FAILURE (status))
		return status;

	SetupHostBuffers ();
	RouteInputSignal ();
	RouteOutputSignal ();
	SetupInputAutoCirculate ();
	SetupOutputAutoCirculate ();

	return AJA_STATUS_SUCCESS;

}	//	Init


AJAStatus NTV2StereoCapture::SetupVideo (void)
{
	//	Check for bi-directional sdi, if so set 1 and 2 to capture, 3 and 4 to display...
	if (NTV2DeviceHasBiDirectionalSDI (mBoardID))
	{
		mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, false);
		mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, false);
		mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
		mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);

		//	Give the board a moment to lock on the input signal...
		AJATime::Sleep (300);
	}

	//	Set the video format to match the incomming video format.
	//	First, check if the board supports this input...
	if (!NTV2DeviceCanDoInputSource (mBoardID, NTV2_INPUTSOURCE_SDI1))
		return AJA_STATUS_BAD_PARAM;

	if (!NTV2DeviceCanDoInputSource (mBoardID, NTV2_INPUTSOURCE_SDI2))
		return AJA_STATUS_BAD_PARAM;

    mVideoFormat = mDevice.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1);

	if (mVideoFormat == NTV2_FORMAT_UNKNOWN)
		return AJA_STATUS_BAD_PARAM;	//	No signal

	//	Now we will set the board format to the input format detected...
	mDevice.SetVideoFormat (mVideoFormat);

	//	Set the frame buffer pixel format for all the channels on the board
	//	if the board supports that pixel format; otherwise, use 8-bit YCbCr...
	if (!NTV2DeviceCanDoFrameBufferFormat (mBoardID, mPixelFormat))
		return AJA_STATUS_BAD_PARAM;

	//	How many frame buffers are there available?
	const uint16_t	numFrameStores	(NTV2DeviceGetNumFrameStores (mBoardID));
	for (int i = 0; i < numFrameStores; i++)
	{
		if (i == 0)
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, NTV2_FBF_ARGB);
		if (i == 1)
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, NTV2_FBF_ARGB);
		if (i == 2)
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, NTV2_FBF_ARGB);
		if (i == 3)
			mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, NTV2_FBF_ARGB);
	}

    mDevice.EnableInputInterrupt (NTV2_CHANNEL1);
    mDevice.SubscribeInputVerticalEvent (NTV2_CHANNEL1);
    mDevice.EnableInputInterrupt (NTV2_CHANNEL2);

	//	Subscribe the output interupt because it's enabled by default...
    mDevice.SubscribeOutputVerticalEvent (NTV2_CHANNEL1);

	return AJA_STATUS_SUCCESS;

}	//	SetupVideo


void NTV2StereoCapture::SetupHostBuffers (void)
{
	//	Allocate video buffers in host RAM...
	mAVCircularBuffer.SetAbortFlag (&mGlobalQuit);
	mVideoBufferSize = GetVideoWriteSize (mVideoFormat, mPixelFormat, false, false);

	for (unsigned int i = 0; i < CIRCULAR_BUFFER_SIZE; i++)
	{
		mAVHostBuffer [i].leftVideoBuffer.AllocateBuffer (mVideoBufferSize, 4096, NULL);
		mAVHostBuffer [i].rightVideoBuffer.AllocateBuffer (mVideoBufferSize, 4096, NULL);
		mAVCircularBuffer.Add (&mAVHostBuffer [i]);
	}

}	//	SetupHostBuffers


void NTV2StereoCapture::RouteInputSignal()
{
	//	Connect Input 1 to FrameStore 1 through color space converter...
    mDevice.SetXptColorSpaceConverterInputSelect (NTV2_XptSDIIn1);
    mDevice.SetXptFrameBuffer1InputSelect (NTV2_XptCSC1VidRGB);

	//	Connect Input 2 to FrameStore 2 through color space converter...
    mDevice.SetXptCSC2VidInputSelect (NTV2_XptSDIIn2);
    mDevice.SetXptFrameBuffer2InputSelect (NTV2_XptCSC2VidRGB);

}	//	RouteInputSignal


void NTV2StereoCapture::RouteOutputSignal (void)
{
	if (NTV2DeviceHasBiDirectionalSDI (mBoardID))
	{
		//	Route FrameStore 3 to SDI Out 3 and HDMI Out if there is a HDMI Out...
        mDevice.SetXptCSC3VidInputSelect (NTV2_XptFrameBuffer3RGB);
        mDevice.SetXptSDIOut3InputSelect (NTV2_XptCSC3VidYUV);
        mDevice.SetXptHDMIOutInputSelect (NTV2_XptCSC3VidYUV);
		//mDevice.SetHDMIOutColorSpace (NTV2_HDMIColorSpaceRGB);
	}
	// else don't do anything for output

}	//	RouteOutputSignal


void NTV2StereoCapture::SetupInputAutoCirculate (void)
{
	mDevice.StopAutoCirculate (NTV2CROSSPOINT_INPUT1);
	mDevice.StopAutoCirculate (NTV2CROSSPOINT_INPUT2);

	::memset (&mInput1TransferStruct,		0,	sizeof (mInput1TransferStruct));
	::memset (&mInput1TransferStatusStruct,	0,	sizeof (mInput1TransferStatusStruct));

	::memset (&mInput2TransferStruct,		0,	sizeof (mInput2TransferStruct));
	::memset (&mInput2TransferStatusStruct,	0,	sizeof (mInput2TransferStatusStruct));

	mInput1TransferStruct.channelSpec			= NTV2CROSSPOINT_INPUT1;
	mInput1TransferStruct.videoBufferSize		= mVideoBufferSize;
	mInput1TransferStruct.frameRepeatCount		= 1;
	mInput1TransferStruct.desiredFrame			= -1;
	mInput1TransferStruct.frameBufferFormat		= mPixelFormat;
	mInput1TransferStruct.bDisableExtraAudioInfo	= true;

	mInput2TransferStruct = mInput1TransferStruct;
	mInput2TransferStruct.channelSpec = NTV2CROSSPOINT_INPUT2;

	const uint32_t	startFrameInput1	(0),	endFrameInput1	(9);
	const uint32_t	startFrameInput2	(10),	endFrameInput2	(19);

	mDevice.InitAutoCirculate (mInput1TransferStruct.channelSpec,
								startFrameInput1,
								endFrameInput1,
								1,
								NTV2_AUDIOSYSTEM_1,
								false,
								false,
								false,
								false,
								false,
								false,
								false,
								false);

	mDevice.InitAutoCirculate (mInput2TransferStruct.channelSpec,
								startFrameInput2,
								endFrameInput2,
								1,
								NTV2_AUDIOSYSTEM_2,
								false,
								false,
								false,
								false,
								false,
								false,
								false,
								false);

}	//	SetupInputAutoCirculate


void NTV2StereoCapture::SetupOutputAutoCirculate (void)
{
}	//	SetupOutputAutoCirculate


AJAStatus NTV2StereoCapture::Run (void)
{
	StartCaptureThread ();
	StartStereoPreviewThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run


//////////////////////////////////////////////
//This is where we start the capture thread

void NTV2StereoCapture::StartCaptureThread (void)
{
	//	Start Thread that will produce frames to play...
	mCaptureThread = new AJAThread ();
	mCaptureThread->Attach (CaptureThreadStatic, this);
	mCaptureThread->SetPriority (AJA_ThreadPriority_High);
	mCaptureThread->Start ();

}	//	StartCaptureThread


void NTV2StereoCapture::CaptureThreadStatic (AJAThread * pThread, void * pContext)
{
	NTV2StereoCapture *	pApp	(reinterpret_cast <NTV2StereoCapture *> (pContext));
	pApp->CaptureFrames (pThread);

}	//	CaptureThreadStatic


void NTV2StereoCapture::CaptureFrames (AJAThread * pThread)
{
	(void) pThread;

    mDevice.WaitForInputVerticalInterrupt ();
	mDevice.StartAutoCirculate (mInput1TransferStruct.channelSpec);
	mDevice.StartAutoCirculate (mInput2TransferStruct.channelSpec);

	while (!mGlobalQuit)
	{
		AUTOCIRCULATE_STATUS_STRUCT	acStatus;
		mDevice.GetAutoCirculate (NTV2CROSSPOINT_INPUT1, &acStatus);

		if (acStatus.state == NTV2_AUTOCIRCULATE_RUNNING && acStatus.bufferLevel > 1)
		{
			NTV2StereoCaptureBuffer *	captureData	(mAVCircularBuffer.StartProduceNextBuffer ());

			mInput1TransferStruct.videoBuffer		= (uint32_t *) captureData->leftVideoBuffer.GetBuffer ();
			mInput1TransferStruct.videoBufferSize	= captureData->leftVideoBuffer.GetBufferSize ();
			mInput2TransferStruct.videoBuffer		= (uint32_t *) captureData->rightVideoBuffer.GetBuffer ();
			mInput2TransferStruct.videoBufferSize	= captureData->rightVideoBuffer.GetBufferSize ();

			mDevice.TransferWithAutoCirculate (&mInput1TransferStruct, &mInput1TransferStatusStruct);
			mDevice.TransferWithAutoCirculate (&mInput2TransferStruct, &mInput2TransferStatusStruct);
			mAVCircularBuffer.EndProduceNextBuffer ();
		}
		else
            mDevice.WaitForInputVerticalInterrupt ();
	}

	mDevice.StopAutoCirculate (mInput1TransferStruct.channelSpec);
	mDevice.StopAutoCirculate (mInput2TransferStruct.channelSpec);

}	//	CaptureFrames


//////////////////////////////////////////////

//////////////////////////////////////////////
//This is where we will start the stereo preview thread

void NTV2StereoCapture::StartStereoPreviewThread (void)
{
	//	Start Thread that will produce frames to play...
	mPreviewStereoThread = new AJAThread ();
	mPreviewStereoThread->Attach (StereoPreviewThreadStatic, this);
	mPreviewStereoThread->SetPriority (AJA_ThreadPriority_High);
	mPreviewStereoThread->Start ();

}	//	StartStereoPreviewThread


void NTV2StereoCapture::StereoPreviewThreadStatic (AJAThread * pThread, void * pContext)
{
	NTV2StereoCapture *	pApp	(reinterpret_cast <NTV2StereoCapture *> (pContext));
	pApp->StereoPreview (pThread);
}


void NTV2StereoCapture::StereoPreview (AJAThread * pThread)
{
	(void) pThread;

	while (!mGlobalQuit)
	{
		NTV2StereoCaptureBuffer *	previewData	(mAVCircularBuffer.StartConsumeNextBuffer ());
		if (previewData)
		{
			QImage			previewImage 		(1920, 2160, QImage::Format_RGB32);
			uint8_t *		previewImageData	(previewImage.bits ());

			::memcpy (previewImageData, previewData->leftVideoBuffer.GetBuffer (), previewData->leftVideoBuffer.GetBufferSize ());
			::memcpy (previewImageData + 1920 * 1080 * 4, previewData->rightVideoBuffer.GetBuffer (), previewData->rightVideoBuffer.GetBufferSize ());

			previewImage = previewImage.scaled (1920, 1080, Qt::IgnoreAspectRatio);
			previewImageData = previewImage.bits ();
            mDevice.DMAWriteFrame (24, (uint32_t *) previewImageData, 1920 * 1080 * 4);
			mDevice.SetOutputFrame (NTV2_CHANNEL3, 24);
			emit newFrame (previewImage, true);

			//	Here's where you'd send to SDI Out and HDMI Out...

			mAVCircularBuffer.EndConsumeNextBuffer ();
		}
	}	//	loop til mGlobalQuit goes true

	mDevice.StopAutoCirculate (mInput1TransferStruct.channelSpec);
	mDevice.StopAutoCirculate (mInput2TransferStruct.channelSpec);

}	//	StereoPreview


//////////////////////////////////////////////


void NTV2StereoCapture::GetACStatus(AUTOCIRCULATE_STATUS_STRUCT & outLeftInputStatus, AUTOCIRCULATE_STATUS_STRUCT & outRightInputStatus)
{
	mDevice.GetAutoCirculate (mInput1TransferStruct.channelSpec, &outLeftInputStatus);
	mDevice.GetAutoCirculate (mInput2TransferStruct.channelSpec, &outRightInputStatus);

}	//	GetACStatus
