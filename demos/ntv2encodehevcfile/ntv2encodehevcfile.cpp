/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2encodehevcfile.cpp
	@brief		Implementation of NTV2EncodeHEVCFile class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include <stdio.h>

#include "ntv2encodehevcfile.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"

using namespace std;


NTV2EncodeHEVCFile::NTV2EncodeHEVCFile (const string				inDeviceSpecifier,
                                        const NTV2Channel			inChannel,
                                        const string                fileName,
                                        const uint32_t              frameWidth,
                                        const uint32_t              frameHeight,
                                        const M31VideoPreset		inPreset)

:	mFileInputThread        (AJAThread()),
	mVideoProcessThread		(AJAThread()),
	mCodecRawThread			(AJAThread()),
	mCodecHevcThread		(AJAThread()),
    mVideoFileThread 		(AJAThread()),
    mM31					(AJA_NULL),
    mHevcCommon             (AJA_NULL),
	mDeviceID				(DEVICE_ID_NOTFOUND),
	mDeviceSpecifier		(inDeviceSpecifier),
    mFileName               (fileName),
    mFrameWidth             (frameWidth),
    mFrameHeight            (frameHeight),
    mInputChannel			(inChannel),
    mEncodeChannel          (M31_CH0),
    mPreset					(inPreset),
    mVideoFormat			(NTV2_MAX_NUM_VIDEO_FORMATS),
    mPixelFormat			(NTV2_FBF_8BIT_YCBCR_420PL2),
    mQuad					(false),
	mInterlaced				(false),
    mMultiStream			(false),
	mSavedTaskMode			(NTV2_STANDARD_TASKS),
	mLastFrame				(false),
	mLastFrameInput			(false),
	mLastFrameRaw			(false),
	mLastFrameHevc			(false),
    mLastFrameVideo			(false),
    mGlobalQuit				(false),
	mVideoInputFrameCount	(0),		
	mVideoProcessFrameCount	(0),		
	mCodecRawFrameCount		(0),
	mCodecHevcFrameCount	(0),
    mVideoFileFrameCount 	(0)
{
    ::memset (mFileInputBuffer, 0x0, sizeof (mFileInputBuffer));
    ::memset (mVideoRawBuffer, 0x0, sizeof (mVideoRawBuffer));
    ::memset (mVideoHevcBuffer, 0x0, sizeof (mVideoHevcBuffer));

}	//	constructor


NTV2EncodeHEVCFile::~NTV2EncodeHEVCFile ()
{
	//	Stop my capture and consumer threads, then destroy them...
	Quit ();

    if (mM31 != AJA_NULL)
	{
		delete mM31;
		mM31 = AJA_NULL;
	}
	
    if (mHevcCommon != AJA_NULL)
    {
        delete mHevcCommon;
        mHevcCommon = AJA_NULL;
    }

	// free all my buffers...
	for (unsigned bufferNdx = 0; bufferNdx < VIDEO_RING_SIZE; bufferNdx++)
	{
        if (mFileInputBuffer[bufferNdx].pVideoBuffer)
		{
            delete [] mFileInputBuffer[bufferNdx].pVideoBuffer;
            mFileInputBuffer[bufferNdx].pVideoBuffer = AJA_NULL;
		}
		if (mFileInputBuffer[bufferNdx].pInfoBuffer)
		{
		 	delete [] mFileInputBuffer[bufferNdx].pInfoBuffer;
			mFileInputBuffer[bufferNdx].pInfoBuffer = AJA_NULL;
		}

        if (mVideoRawBuffer[bufferNdx].pVideoBuffer)
		{
            delete [] mVideoRawBuffer[bufferNdx].pVideoBuffer;
            mVideoRawBuffer[bufferNdx].pVideoBuffer = AJA_NULL;
		}
		if (mVideoRawBuffer[bufferNdx].pInfoBuffer)
		{
		 	delete [] mVideoRawBuffer[bufferNdx].pInfoBuffer;
			mVideoRawBuffer[bufferNdx].pInfoBuffer = AJA_NULL;
		}

        if (mVideoHevcBuffer[bufferNdx].pVideoBuffer)
		{
            delete [] mVideoHevcBuffer[bufferNdx].pVideoBuffer;
            mVideoHevcBuffer[bufferNdx].pVideoBuffer = AJA_NULL;
		}
		if (mVideoHevcBuffer[bufferNdx].pInfoBuffer)
		{
		 	delete [] mVideoHevcBuffer[bufferNdx].pInfoBuffer;
			mVideoHevcBuffer[bufferNdx].pInfoBuffer = AJA_NULL;
		}
    }
	
} // destructor


void NTV2EncodeHEVCFile::Quit (void)
{
    if (mM31 && !mLastFrame && !mGlobalQuit)
	{
		//	Set the last frame flag to start the quit process
		mLastFrame = true;

		//	Wait for the last frame to be written to disk
		int i;
		int timeout = 300;
		for (i = 0; i < timeout; i++)
		{
            if (mLastFrameVideo) break;
			AJATime::Sleep (10);
		}
		if (i == timeout)
			{ cerr << "## ERROR:  Wait for last frame timeout" << endl; }

		//	Stop the encoder stream
        if (!mM31->ChangeEHState(Hevc_EhState_ReadyToStop, mEncodeChannel))
			{ cerr << "## ERROR:  ChangeEHState ready to stop failed" << endl; }

        if (!mM31->ChangeEHState(Hevc_EhState_Stop, mEncodeChannel))
			{ cerr << "## ERROR:  ChangeEHState stop failed" << endl; }

		// stop the video input stream
        if (!mM31->ChangeVInState(Hevc_VinState_Stop, mEncodeChannel))
			{ cerr << "## ERROR:  ChangeVInState stop failed" << endl; }

        if(!mMultiStream)
        {
            //	Now go to the init state
            if (!mM31->ChangeMainState(Hevc_MainState_Init, Hevc_EncodeMode_Single))
                { cerr << "## ERROR:  ChangeMainState to init failed" << endl; }
        }
	}

	//	Stop the worker threads
	mGlobalQuit = true;

	while (mFileInputThread.Active())
		AJATime::Sleep(10);

	while (mVideoProcessThread.Active())
		AJATime::Sleep(10);

	while (mCodecRawThread.Active())
		AJATime::Sleep(10);

	while (mCodecHevcThread.Active())
		AJATime::Sleep(10);

	while (mVideoFileThread.Active())
		AJATime::Sleep(10);

    //  Release board
    if (!mMultiStream)
	{
		mDevice.ReleaseStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));
		mDevice.SetEveryFrameServices (mSavedTaskMode);		//	Restore prior task mode
	}

    //  Close output file
    mHevcCommon->CloseHevcFile();
    
    //  Close YUV input file
    mHevcCommon->CloseYuv420File();

}	//	Quit


AJAStatus NTV2EncodeHEVCFile::Init (void)
{
    AJAStatus	status	(AJA_STATUS_SUCCESS);
    
    //	Open the device...
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, mDevice))
    { cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN; }
    
    //  Grab board in a shared environment
    if (!mMultiStream)
    {
        if (!mDevice.AcquireStreamForApplication (kDemoAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
            return AJA_STATUS_BUSY;							//	Another app is using the device
        mDevice.GetEveryFrameServices (mSavedTaskMode);	//	Save the current state before we change it
    }
    mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Since this is an OEM demo, use the OEM service level
    
    mDeviceID = mDevice.GetDeviceID ();						//	Keep the device ID handy, as it's used frequently
    
    // Make sure this device has an M31
    if (!NTV2DeviceHasHEVCM31 (mDeviceID))
	{
  	 	cerr << "## ERROR:  M31 not found" << endl;
        return AJA_STATUS_FAIL;
	}

    // Allocate our M31 helper class and our HEVC common class
    mM31 = new CNTV2m31 (&mDevice);
    mHevcCommon = new CNTV2DemoHevcCommon ();
    
    if ((mM31 == AJA_NULL) || (mHevcCommon == AJA_NULL))
        return AJA_STATUS_FAIL;
    
    // Sanity check
    if (mPreset >= M31_NUMVIDEOPRESETS)
        return AJA_STATUS_FAIL;

    // This class only handles file based presets so make sure they didn't pass in a vif one
    if (CNTV2m31::IsPresetVIF(mPreset))
        return AJA_STATUS_FAIL;
    
    //	Get NTV2 formats to match codec preset
    mVideoFormat = CNTV2m31::GetPresetVideoFormat(mPreset);
    mPixelFormat = CNTV2m31::GetPresetFrameBufferFormat(mPreset);
    mQuad = CNTV2m31::IsPresetUHD(mPreset);
    mInterlaced = CNTV2m31::IsPresetInterlaced(mPreset);

    //  Quad mode must be channel 1
    if (mQuad)
    {
        mInputChannel = NTV2_CHANNEL1;
        mEncodeChannel = M31_CH0;
    }
    else
    {
        //  When input channel specified we are multistream
        switch (mInputChannel)
        {
        case NTV2_CHANNEL1: { mEncodeChannel = M31_CH0; mMultiStream = true; break; }
        case NTV2_CHANNEL2: { mEncodeChannel = M31_CH1; mMultiStream = true; break; }
        case NTV2_CHANNEL3: { mEncodeChannel = M31_CH2; mMultiStream = true; break; }
        case NTV2_CHANNEL4: { mEncodeChannel = M31_CH3; mMultiStream = true; break; }
        default: { mInputChannel = NTV2_CHANNEL1;  mEncodeChannel = M31_CH0; }
        }
    }

	//	Setup codec
	status = mHevcCommon->SetupHEVC (mM31, mPreset, mEncodeChannel, mMultiStream, false);
	if (AJA_FAILURE (status))
		return status;

	//	Setup the circular buffers
	SetupHostBuffers ();

	//	Open the YUV input file
	status = mHevcCommon->OpenYuv420File (mFileName, RAWFILEWIDTH, RAWFILEHEIGHT);
	if (AJA_FAILURE (status))
	{
		cerr << "OpenYuv420File " << mFileName << " failed " << status << endl;
		return status;
	}

	{
		//	Create encoded video output file
		ostringstream	fileName;
		if (mMultiStream)
			fileName << "raw_" << (mInputChannel+1) << ".hevc";
		else
			fileName << "raw.hevc";
		status = mHevcCommon->CreateHevcFile (fileName.str(), mHevcCommon->YuvNumFrames());
		if (AJA_FAILURE (status))
			return status;
	}
	return AJA_STATUS_SUCCESS;

}	//	Init


M31VideoPreset	NTV2EncodeHEVCFile::GetCodecPreset (void)
{
    return mPreset;
}


void NTV2EncodeHEVCFile::SetupHostBuffers (void)
{
	mVideoBufferSize = GetVideoActiveSize (mVideoFormat, mPixelFormat, NTV2_VANCMODE_OFF);
    mPicInfoBufferSize = sizeof(HevcPictureInfo)*2;
    mEncInfoBufferSize = sizeof(HevcEncodedInfo)*2;
	
	// video input ring
    mFileInputCircularBuffer.SetAbortFlag (&mGlobalQuit);
	for (unsigned bufferNdx = 0; bufferNdx < VIDEO_RING_SIZE; bufferNdx++ )
	{
        memset (&mFileInputBuffer[bufferNdx], 0, sizeof(AVHevcDataBuffer));
        mFileInputBuffer[bufferNdx].pVideoBuffer    = new uint32_t [mVideoBufferSize/4];
        mFileInputBuffer[bufferNdx].videoBufferSize	= mVideoBufferSize;
        mFileInputBuffer[bufferNdx].videoDataSize   = 0;
        mFileInputBuffer[bufferNdx].videoDataSize2  = 0;
        mFileInputBuffer[bufferNdx].pInfoBuffer		= new uint32_t [mPicInfoBufferSize/4];
        mFileInputBuffer[bufferNdx].infoBufferSize  = mPicInfoBufferSize;
        mFileInputBuffer[bufferNdx].infoDataSize    = 0;
        mFileInputBuffer[bufferNdx].infoDataSize2   = 0;
        mFileInputCircularBuffer.Add (& mFileInputBuffer[bufferNdx]);
	}

    // video raw ring
    mVideoRawCircularBuffer.SetAbortFlag (&mGlobalQuit);
	for (unsigned bufferNdx = 0; bufferNdx < VIDEO_RING_SIZE; bufferNdx++ )
	{
        memset (&mVideoRawBuffer[bufferNdx], 0, sizeof(AVHevcDataBuffer));
        mVideoRawBuffer[bufferNdx].pVideoBuffer		= new uint32_t [mVideoBufferSize/4];
        mVideoRawBuffer[bufferNdx].videoBufferSize	= mVideoBufferSize;
        mVideoRawBuffer[bufferNdx].videoDataSize	= 0;
        mVideoRawBuffer[bufferNdx].videoDataSize2	= 0;
        mVideoRawBuffer[bufferNdx].pInfoBuffer		= new uint32_t [mPicInfoBufferSize/4];
        mVideoRawBuffer[bufferNdx].infoBufferSize   = mPicInfoBufferSize;
        mVideoRawBuffer[bufferNdx].infoDataSize		= 0;
        mVideoRawBuffer[bufferNdx].infoDataSize2	= 0;
        mVideoRawCircularBuffer.Add (& mVideoRawBuffer[bufferNdx]);
	}

    // video hevc ring
    mVideoHevcCircularBuffer.SetAbortFlag (&mGlobalQuit);
    for (unsigned bufferNdx = 0; bufferNdx < VIDEO_RING_SIZE; bufferNdx++ )
    {
        memset (&mVideoHevcBuffer[bufferNdx], 0, sizeof(AVHevcDataBuffer));
        mVideoHevcBuffer[bufferNdx].pVideoBuffer	= new uint32_t [mVideoBufferSize/4];
        mVideoHevcBuffer[bufferNdx].videoBufferSize	= mVideoBufferSize;
        mVideoHevcBuffer[bufferNdx].videoDataSize	= 0;
        mVideoHevcBuffer[bufferNdx].videoDataSize2	= 0;
        mVideoHevcBuffer[bufferNdx].pInfoBuffer		= new uint32_t [mEncInfoBufferSize/4];
        mVideoHevcBuffer[bufferNdx].infoBufferSize  = mEncInfoBufferSize;
        mVideoHevcBuffer[bufferNdx].infoDataSize	= 0;
        mVideoHevcBuffer[bufferNdx].infoDataSize2	= 0;
        mVideoHevcCircularBuffer.Add (& mVideoHevcBuffer[bufferNdx]);
    }

}	//	SetupHostBuffers


AJAStatus NTV2EncodeHEVCFile::Run ()
{
	//	Start the playout and capture threads...
    StartVideoInputThread ();
    StartVideoProcessThread ();
    StartCodecRawThread ();
    StartCodecHevcThread ();
    StartVideoFileThread ();

	return AJA_STATUS_SUCCESS;

}	//	Run


// This is where we will start the video input thread
void NTV2EncodeHEVCFile::StartVideoInputThread (void)
{
    mFileInputThread.Attach(VideoInputThreadStatic, this);
    mFileInputThread.SetPriority(AJA_ThreadPriority_High);
    mFileInputThread.Start();

}	// StartVideoInputThread


// The video input thread static callback
void NTV2EncodeHEVCFile::VideoInputThreadStatic (AJAThread * pThread, void * pContext)
{
	(void) pThread;

	NTV2EncodeHEVCFile *	pApp (reinterpret_cast <NTV2EncodeHEVCFile *> (pContext));
    pApp->VideoInputWorker ();

}	// VideoInputThreadStatic


void NTV2EncodeHEVCFile::VideoInputWorker (void)
{
    AJAStatus status;
    
	while (!mGlobalQuit)
	{
        // Have we read all the frames of the file in
        if (mVideoInputFrameCount < mHevcCommon->YuvNumFrames())
        {
            // No so lets read another frame
            // First grab a local buffer so we can read a frame of data into it
            AVHevcDataBuffer *	pVideoData	(mFileInputCircularBuffer.StartProduceNextBuffer ());
            if (pVideoData)
            {
                status = mHevcCommon->ReadYuv420Frame(pVideoData->pVideoBuffer, mVideoInputFrameCount);
                if (status != AJA_STATUS_SUCCESS)
                {
                    printf("Error reading frame %d\n", mVideoInputFrameCount);
                }

                pVideoData->videoDataSize = pVideoData->videoBufferSize;
                pVideoData->videoDataSize2 = 0;

                // if we are done mark the last frame for the other threads
                if (mVideoInputFrameCount == mHevcCommon->YuvNumFrames())
                {
                    pVideoData->lastFrame = true;
                }
                
                if(pVideoData->lastFrame && !mLastFrameInput)
                {
                    printf ( "Read last frame number %d\n", mVideoInputFrameCount );
                    mLastFrameInput = true;
                }
				mVideoInputFrameCount++;
                
                // signal that we're done "producing" the frame, making it available for future "consumption"...
                mFileInputCircularBuffer.EndProduceNextBuffer ();
            }
        }
        // Looks like we are done
        else
        {
            mGlobalQuit = true;
        }
    }
}	// VideoInputWorker


// This is where we start the video process thread
void NTV2EncodeHEVCFile::StartVideoProcessThread (void)
{
    mVideoProcessThread.Attach(VideoProcessThreadStatic, this);
    mVideoProcessThread.SetPriority(AJA_ThreadPriority_High);
    mVideoProcessThread.Start();

}	// StartVideoProcessThread


// The video process static callback
void NTV2EncodeHEVCFile::VideoProcessThreadStatic (AJAThread * pThread, void * pContext)
{
	(void) pThread;

	NTV2EncodeHEVCFile *	pApp (reinterpret_cast <NTV2EncodeHEVCFile *> (pContext));
    pApp->VideoProcessWorker ();

}	// VideoProcessThreadStatic


void NTV2EncodeHEVCFile::VideoProcessWorker (void)
{
	while (!mGlobalQuit)
	{
		// wait for the next video input buffer
        AVHevcDataBuffer *	pSrcFrameData (mFileInputCircularBuffer.StartConsumeNextBuffer ());
		if (pSrcFrameData)
		{
			// wait for the next video raw buffer
			AVHevcDataBuffer *	pDstFrameData (mVideoRawCircularBuffer.StartProduceNextBuffer ());
			if (pDstFrameData)
			{
				// do something useful with the frame data...
                ProcessVideoFrame(pSrcFrameData, pDstFrameData);

                mVideoProcessFrameCount++;

				// release the video raw buffer
				mVideoRawCircularBuffer.EndProduceNextBuffer ();
            }

			// release the video input buffer
            mFileInputCircularBuffer.EndConsumeNextBuffer ();

        }
	}	// loop til quit signaled

}	// VideoProcessWorker


// This is where we start the codec raw thread
void NTV2EncodeHEVCFile::StartCodecRawThread (void)
{
    mCodecRawThread.Attach(CodecRawThreadStatic, this);
    mCodecRawThread.SetPriority(AJA_ThreadPriority_High);
    mCodecRawThread.Start();

}	// StartCodecRawThread


// The codec raw static callback
void NTV2EncodeHEVCFile::CodecRawThreadStatic (AJAThread * pThread, void * pContext)
{
	(void) pThread;

	NTV2EncodeHEVCFile *	pApp (reinterpret_cast <NTV2EncodeHEVCFile *> (pContext));
    pApp->CodecRawWorker ();

}	// CodecRawThreadStatic


void NTV2EncodeHEVCFile::CodecRawWorker (void)
{
    CNTV2Card       ntv2Device;
    CNTV2m31 *      m31;
    
    //	Open the device...
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, ntv2Device))
    { cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return; }
    
    // Allocate our M31 helper class and our HEVC common class
    m31 = new CNTV2m31 (&ntv2Device);

	while (!mGlobalQuit)
	{
		// wait for the next raw video frame
        AVHevcDataBuffer *	pFrameData (mVideoRawCircularBuffer.StartConsumeNextBuffer ());
		if (pFrameData)
		{
			if (!mLastFrameRaw)
			{
				// transfer the raw video frame to the codec
				if (mInterlaced)
				{
                    m31->RawTransfer(mPreset, mEncodeChannel,
                                        (uint8_t*)pFrameData->pVideoBuffer,
                                        pFrameData->videoDataSize,
                                        false, false);

                    m31->RawTransfer(mPreset, mEncodeChannel,
                                        (uint8_t*)pFrameData->pVideoBuffer,
                                        pFrameData->videoDataSize,
                                        true, pFrameData->lastFrame);
				}
				else
				{
                    m31->RawTransfer(mEncodeChannel,
                                        (uint8_t*)pFrameData->pVideoBuffer,
                                        pFrameData->videoDataSize,
                                        pFrameData->lastFrame);
				}
                if (pFrameData->lastFrame)
				{
					mLastFrameRaw = true;
				}

                mCodecRawFrameCount++;
            }

			// release the raw video frame
            mVideoRawCircularBuffer.EndConsumeNextBuffer ();
		}
	}  // loop til quit signaled

    delete m31;

} // CodecRawWorker


// This is where we will start the codec hevc thread
void NTV2EncodeHEVCFile::StartCodecHevcThread (void)
{
    mCodecHevcThread.Attach(CodecHevcThreadStatic, this);
    mCodecHevcThread.SetPriority(AJA_ThreadPriority_High);
    mCodecHevcThread.Start();

} // StartCodecHevcThread


// The codec hevc static callback
void NTV2EncodeHEVCFile::CodecHevcThreadStatic (AJAThread * pThread, void * pContext)
{
    (void) pThread;

    NTV2EncodeHEVCFile *	pApp (reinterpret_cast <NTV2EncodeHEVCFile *> (pContext));
    pApp->CodecHevcWorker ();

}	//	CodecHevcThreadStatic


void NTV2EncodeHEVCFile::CodecHevcWorker (void)
{
    CNTV2Card       ntv2Device;
    CNTV2m31 *      m31;
    
    //	Open the device...
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (mDeviceSpecifier, ntv2Device))
    { cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return; }
    
    // Allocate our M31 helper class and our HEVC common class
    m31 = new CNTV2m31 (&ntv2Device);

    while (!mGlobalQuit)
    {
        // wait for the next hevc frame 
        AVHevcDataBuffer *	pFrameData (mVideoHevcCircularBuffer.StartProduceNextBuffer ());
        if (pFrameData)
        {
			if (!mLastFrameHevc)
			{
				if (mInterlaced)
				{
					// get field 1 video and info buffer and size
					uint8_t* pVideoBuffer = (uint8_t*)pFrameData->pVideoBuffer;
					uint8_t* pInfoBuffer = (uint8_t*)pFrameData->pInfoBuffer;
					uint32_t videoBufferSize = pFrameData->videoBufferSize;
					uint32_t infoBufferSize = sizeof(HevcEncodedInfo);

					// transfer an hevc field 1 from the codec including encoded information
					m31->EncTransfer(mEncodeChannel,
										pVideoBuffer,
										videoBufferSize,
										pInfoBuffer,
										infoBufferSize,
										pFrameData->videoDataSize,
										pFrameData->infoDataSize,
										pFrameData->lastFrame);

					// round the video size up
					pFrameData->videoDataSize = mHevcCommon->AlignDataBuffer(pVideoBuffer,
																videoBufferSize,
																pFrameData->videoDataSize,
																8, 0xff);
					// round the info size up
					pFrameData->infoDataSize = mHevcCommon->AlignDataBuffer(pInfoBuffer,
																infoBufferSize,
																pFrameData->infoDataSize,
																8, 0);

					// get field 2 video and info buffer and size
					pVideoBuffer = ((uint8_t*)pFrameData->pVideoBuffer) + pFrameData->videoDataSize;
					pInfoBuffer = ((uint8_t*)pFrameData->pInfoBuffer) + sizeof(HevcEncodedInfo);
					videoBufferSize = pFrameData->videoBufferSize - pFrameData->videoDataSize;
					infoBufferSize = sizeof(HevcEncodedInfo);

					// transfer an hevc field 2 from the codec including encoded information
					m31->EncTransfer(mEncodeChannel,
										pVideoBuffer,
										videoBufferSize,
										pInfoBuffer,
										infoBufferSize,
										pFrameData->videoDataSize2,
										pFrameData->infoDataSize2,
										pFrameData->lastFrame);

					// round the video size up
					pFrameData->videoDataSize2 = mHevcCommon->AlignDataBuffer(pVideoBuffer,
																videoBufferSize,
																pFrameData->videoDataSize2,
																8, 0xff);
					// round the info size up
					pFrameData->infoDataSize2 = mHevcCommon->AlignDataBuffer(pInfoBuffer,
																infoBufferSize,
																pFrameData->infoDataSize2,
																8, 0);
				}
				else
				{
					// transfer an hevc frame from the codec including encoded information
					m31->EncTransfer(mEncodeChannel,
										(uint8_t*)pFrameData->pVideoBuffer,
										pFrameData->videoBufferSize,
										(uint8_t*)pFrameData->pInfoBuffer,
										pFrameData->infoBufferSize,
										pFrameData->videoDataSize,
										pFrameData->infoDataSize,
										pFrameData->lastFrame);

					// round the video size up
					pFrameData->videoDataSize = mHevcCommon->AlignDataBuffer(pFrameData->pVideoBuffer,
																pFrameData->videoBufferSize,
																pFrameData->videoDataSize,
																8, 0xff);
					// round the info size up
					pFrameData->infoDataSize = mHevcCommon->AlignDataBuffer(pFrameData->pInfoBuffer,
																pFrameData->infoBufferSize,
																pFrameData->infoDataSize,
																8, 0);
				}

                if (pFrameData->lastFrame)
				{
					mLastFrameHevc = true;
                }

                mCodecHevcFrameCount++;
            }

            // release and recycle the buffer...
            mVideoHevcCircularBuffer.EndProduceNextBuffer ();
        }
    }	//	loop til quit signaled
    
    delete m31;
}	//	EncTransferFrames


// This is where we start the video file writer thread
void NTV2EncodeHEVCFile::StartVideoFileThread (void)
{
    mVideoFileThread.Attach(VideoFileThreadStatic, this);
    mVideoFileThread.SetPriority(AJA_ThreadPriority_High);
    mVideoFileThread.Start();

} // StartVideoFileThread


// The file writer static callback
void NTV2EncodeHEVCFile::VideoFileThreadStatic (AJAThread * pThread, void * pContext)
{
    (void) pThread;

    NTV2EncodeHEVCFile *	pApp (reinterpret_cast <NTV2EncodeHEVCFile *> (pContext));
    pApp->VideoFileWorker ();

} // VideoFileStatic


void NTV2EncodeHEVCFile::VideoFileWorker (void)
{
    while (!mGlobalQuit)
    {
        // wait for the next codec hevc frame
        AVHevcDataBuffer *	pFrameData (mVideoHevcCircularBuffer.StartConsumeNextBuffer ());
        if (pFrameData)
        {
            if (!mLastFrameVideo)
			{
				// write the frame / fields hevc to the output file
                mHevcCommon->WriteHevcData(pFrameData->pVideoBuffer, pFrameData->videoDataSize + pFrameData->videoDataSize2);

                if (pFrameData->lastFrame)
				{
                    printf ( "Video file last frame number %d\n", mVideoFileFrameCount );
                    mLastFrameVideo = true;
				}

                mVideoFileFrameCount++;
            }

            // release the hevc buffer
            mVideoHevcCircularBuffer.EndConsumeNextBuffer ();
        }
    } // loop til quit signaled

} // VideoFileWorker


//////////////////////////////////////////////


void NTV2EncodeHEVCFile::GetStatus (AVHevcStatus & outStatus)
{
	//	Just report frames processed for file demo, as there's never a dropped frame,
	//	since there's no requirement to run in realtime, though this typically runs
	//	faster than real time.  Also the buffer level is not important.
	outStatus.framesProcessed = mVideoFileFrameCount;
	outStatus.framesDropped = outStatus.bufferLevel = 0;
}	//	GetStatus


AJAStatus NTV2EncodeHEVCFile::ProcessVideoFrame (AVHevcDataBuffer * pSrcFrame, AVHevcDataBuffer * pDstFrame)
{
    // Convert the RAW YUV tri-planar frame in to an NV12 bi-planar fame
    AJAStatus status = mHevcCommon->ConvertYuv420FrameToNV12(pSrcFrame->pVideoBuffer, pDstFrame->pVideoBuffer, pSrcFrame->videoDataSize);

    pDstFrame->videoDataSize = pSrcFrame->videoDataSize;
    pDstFrame->lastFrame = pSrcFrame->lastFrame;

    return status;

}	//	ProcessVideoFrame
