//---------------------------------------------------------------------------------------------------------------------
//  ntv4playbackdpx.cpp
//
//	Copyright (C) 2012 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
//---------------------------------------------------------------------------------------------------------------------
#include <QtGlobal>
#include "ntv2playbackdpx.h"
#include "ajabase/common/timer.h"
#include "ajabase/system/file_io.h"
#include "ajabase/common/dpx_hdr.h"
#include "ajabase/system/process.h"
#include "ajabase/common/videoutilities.h"
#include "ntv2utils.h"
////#include "ntv4/common/videoformatinfo.h"
////#include "ntv4/common/pixelformatinfo.h"

#ifndef AJA_LINUX
#define AJA_USESSE
#ifdef AJA_USESSE
#include "streams/common/sseutil.h"
#endif 
#endif

NTV2PlaybackDPX::NTV2PlaybackDPX(int deviceIndex,const std::string& directoryName, NTV2VideoFormat videoFormat)
    : NTV2PlaybackThread()
{
    mSelectedDevice = deviceIndex;
    mDirectoryPath  = directoryName;

    mAbort          = false;
    mSmallFile      = false;
    mQueueSize      = 16;
    mRasterImageSize= 0;
    mNumFramesRead  = 0;
    mVideoFormat    = videoFormat;
    mPixelFormat    = NTV2_FBF_NUMFRAMEBUFFERFORMATS;
    mNumFiles		= 0;



#ifndef AJA_LINUX
    uint32_t cores = SSE_QueryCoreCount();
    if (cores > 1)
    {
        cores /= 2;	// use half the available cores
    }
    mUseSSE = SSE_Initialize_WithMaxcores(cores);
#else
    mUseSSE = false;
#endif

    start();
    setPriority(QThread::TimeCriticalPriority);	// must set priority when running

}

NTV2PlaybackDPX::~NTV2PlaybackDPX()
{
    mAbort = true;
    while ( isRunning())
        ; //wait for main thread to finish.

    for (int i=0; i<sPlaybackDPXCircularBufferSize; i++ )
    {
        delete mQImagePool[i];
    }

    closeDevice();


}

void NTV2PlaybackDPX::openDevice()
{
    //	Open the device...
    if (!mNTV2Scanner.GetDeviceAtIndex (mSelectedDevice, mDevice))
        return ;

    if (!mDevice.AcquireStreamForApplication (NTV2_FOURCC('D','E','M','O'), static_cast <uint32_t> (AJAProcess::GetPid ())))
        return ;		//	Device is in use by another app -- fail
    mDevice.GetEveryFrameServices (&mPreviousFrameServices);	//	Save the current service level
    mDevice.SetEveryFrameServices (NTV2_OEM_TASKS);				//	Set OEM service level
}

void NTV2PlaybackDPX::closeDevice()
{
    mDevice.Close();
    //    mDevice.SetEveryFrameServices (mPreviousFrameServices);										//	Restore the saved service level
    //    mDevice.ReleaseStreamForApplication (NTV2_FOURCC('D','E','M','O'), static_cast<int32_t>(AJAProcess::GetPid()));	//	Release the device
}
// run()
// entry point for thread starting
// this sets up files,and ntv4 sequencer and calls the Playback Routine that
// does the work.
void NTV2PlaybackDPX::run()
{
    setupFileList();
    mDPXInfo.videoFormat = NTV2_FORMAT_UNKNOWN;
    getDPXFileInfo(mFileList[0].absoluteFilePath());

    if( mVideoFormat == NTV2_FORMAT_UNKNOWN )
    {
        // Same as DPX
        mVideoFormat   = mDPXInfo.videoFormat;
    }
    mPixelFormat   = mDPXInfo.pixelFormat;

    mRasterWidth  = mDPXInfo.width;
    mRasterHeight = mDPXInfo.height;
    mRasterImageSize = mDPXInfo.size;

    // Setup Preview QImages
#define KLUDGE_LINE 0
    for (int i=0; i<sPlaybackDPXCircularBufferSize; i++ )
    {
        mQImagePool[i] = new QImage(mRasterWidth/2,(mRasterHeight/2) + KLUDGE_LINE,QImage::Format_RGB32);
    }
    mImagePoolIndex = 0;
    openDevice();
    setupOutputVideo();
    setupCircularBuffer();
    setupAutoCirculate();
    readDPXFiles(); // this will run until thread is terminated.

}


bool NTV2PlaybackDPX::setupFileList()
{
    // For now, use QT for File I/O.
    QString dirStr(mDirectoryPath.c_str());
    QDir dir(dirStr);
    QStringList filters;
    filters << "*.dpx" << "*.DPX";
    dir.setNameFilters(filters);
    mFileList = dir.entryInfoList();
    mNumFiles = mFileList.size();

    return true;
}

// Assumes progressive playout
void NTV2PlaybackDPX::getDPXFileInfo(QString fileNameStr)
{
    // Open video file to get parameters
    QFile dpxFile(fileNameStr);
    DpxHdr dpxHeader;
    dpxFile.open(QIODevice::ReadOnly);
    dpxFile.read((char*)&(dpxHeader.GetHdr()),dpxHeader.GetHdrSize());
    dpxFile.close();

    mDPXInfo.offset			= (uint32_t)dpxHeader.get_fi_image_offset();
    mDPXInfo.height			= (uint32_t)dpxHeader.get_ii_lines();
    mDPXInfo.width			= (uint32_t)dpxHeader.get_ii_pixels();
    mDPXInfo.size			= (uint32_t)dpxHeader.get_ii_image_size();
    mDPXInfo.numPitsPerCompent = (uint32_t)dpxHeader.get_ie_bit_size();
    ////	double rate			    = 1.0 / dpxHeader.get_film_frame_rate();
    ////	dpxInfo.rate			= NTV4VideoFormatInfo::ConvertTicksPerSecondToVideoRate(rate);
    ////	if (dpxInfo.rate == NTV4_VideoRate_Unknown)
    ////	{
    ////		dpxInfo.rate = NTV4_VideoRate_2997;		// arbitrary default
    ////	}
#if 0
    // try reverse lookup from dpx information
    uint8_t progressive = dpxHeader.get_tv_interlace();
    if (progressive != 1)
    {
        // progressive
        dpxInfo.videoFormat = NTV2_FORMAT_4x1920x1080psf_2400;
        ////		dpxInfo.videoFormat	= NTV4VideoFormatInfo::GetVideoFormat(dpxInfo.width,
        ////		                                                          dpxInfo.height,
        ////																  dpxInfo.rate,
        ////																  NTV4_VideoScan_Progressive);
    }
    else
    {
        // interlaced
        Q_ASSERT (progressive == 1);
        dpxInfo.videoFormat = NTV2_FORMAT_4x1920x1080psf_2400;
        ////		dpxInfo.videoFormat	= NTV4VideoFormatInfo::GetVideoFormat(dpxInfo.width,
        ////			dpxInfo.height,
        ////			dpxInfo.rate,
        ////			NTV4_VideoScan_TopFieldFirst);

        if (dpxInfo.videoFormat == NTV2_FORMAT_UNKNOWN)
        {
            dpxInfo.videoFormat = NTV2_FORMAT_4x1920x1080psf_2400;
            ////			dpxInfo.videoFormat	= NTV4VideoFormatInfo::GetVideoFormat(dpxInfo.width,
            ////				dpxInfo.height,
            ////				dpxInfo.rate,
            ////				NTV4_VideoScan_BottomFieldFirst);
        }
    }
#endif

    if (mDPXInfo.videoFormat == NTV2_FORMAT_UNKNOWN)
    {
        switch ( mDPXInfo.width )
        {
        case 720:
        {
            if ( mDPXInfo.height == 576 )
                mDPXInfo.videoFormat = NTV2_FORMAT_625_5000;
            else
                mDPXInfo.videoFormat = NTV2_FORMAT_525_5994;
        }
            break;
        case 1280:
        {
            mDPXInfo.videoFormat = NTV2_FORMAT_720p_5994;
        }
            break;
        case 1920:
        {
            mDPXInfo.videoFormat = NTV2_FORMAT_1080p_2400;
        }
            break;
        case 2048:
        {
            mDPXInfo.videoFormat = NTV2_FORMAT_1080p_2K_2400;
        }
            break;
        case 3840:
        {
            mDPXInfo.videoFormat = NTV2_FORMAT_4x1920x1080p_2997;
//            dpxInfo.videoFormat = NTV2_FORMAT_4x1920x1080p_2400;
        }
            break;
        case 4096:
        {
            mDPXInfo.videoFormat = NTV2_FORMAT_4x2048x1080p_2997;
//            dpxInfo.videoFormat = NTV2_FORMAT_4x2048x1080p_2400;
        }
            break;
        default:
        {
            mDPXInfo.videoFormat = NTV2_FORMAT_1080p_2400;
        }

        }
    }


    mDPXInfo.pixelFormat = NTV2_FBF_10BIT_DPX;
    if ( dpxHeader.get_ie_descriptor() == 100 )
    {
        if ( dpxHeader.IsBigEndian())
        {
            mDPXInfo.pixelFormat = NTV2_FBF_10BIT_YCBCR_DPX;
        }
        else
        {
            // NOT REALLY SUPPORTED
            mDPXInfo.pixelFormat = NTV2_FBF_10BIT_YCBCR_DPX;
        }
    }
    else if ( dpxHeader.get_ie_descriptor() == 50 )
    {
        if ( dpxHeader.IsBigEndian())
        {
            mDPXInfo.pixelFormat = NTV2_FBF_10BIT_DPX;
        }
        else
        {
            mDPXInfo.pixelFormat = NTV2_FBF_10BIT_DPX_LE;
        }
    }
    if ( mDPXInfo.numPitsPerCompent == 16 )
    {
        mDPXInfo.pixelFormat =  NTV2_FBF_48BIT_RGB;

    }
    ////	if (NTV4VideoFormatInfo::isSD(dpxInfo.videoFormat))
    ////	{
    ////		mSmallFile = true;
    ////	}
}

void NTV2PlaybackDPX::setupCircularBuffer()
{
    AJAStatus status;

    uint32_t allocateSize = (mRasterImageSize + 4095)& (~4095);

    mPlaybackCircularBuffer.Init(&mAbort);

    for ( int i=0; i<sPlaybackDPXCircularBufferSize; i++ )
    {
        status = mAVDataBuffer[i].videoBuffer.AllocateBuffer(allocateSize,4096,NULL);
        if ( AJA_STATUS_SUCCESS != status ) qDebug() << "Couldn't allocate video";
        //status = mAVDataBuffer[i].audioBuffer.AllocateBuffer(audioSize,4096,NULL);
        //if ( AJA_STATUS_SUCCESS != status ) qDebug() << "Couldn't allocate audio";
        mPlaybackCircularBuffer.Add(&mAVDataBuffer[i]);
    }

    // Setup Preview Buffer
     //	Allocate my buffers for the preview window...
     //	No video buffer addresses are assigned, as the capture thread will populate the address
     //	with the same one used by the file writer ring.
     mPreviewCircularBuffer.Init(&mAbort);
     for (int i = 0; i < sPlaybackDPXCircularBufferSize; i++)
     {
         mPreviewDataBuffer [i].videoBuffer		= NULL;
         mPreviewDataBuffer [i].videoBufferSize	= mRasterImageSize;

         mPreviewCircularBuffer.Add (&mPreviewDataBuffer [i]);
     }	//	for each AV buffer in my circular buffer

}

void NTV2PlaybackDPX::setupOutputVideo()
{

    if (::NTV2DeviceCanDoMultiFormat (mDevice.GetDeviceID ()))
        mDevice.SetMultiFormatMode (false);

    mDevice.SetVideoFormat(mVideoFormat);

    mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, mPixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, mPixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, mPixelFormat);
    mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, mPixelFormat);
    mDevice.EnableChannel (NTV2_CHANNEL1);
    mDevice.EnableChannel (NTV2_CHANNEL2);
    mDevice.EnableChannel (NTV2_CHANNEL3);
    mDevice.EnableChannel (NTV2_CHANNEL4);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL3, true);
    mDevice.SetSDITransmitEnable (NTV2_CHANNEL4, true);

    CNTV2SignalRouter	router;
    if ( ::IsRGBFormat (mPixelFormat) )
    {

        router.addWithValue (::GetDualLinkOut1InputSelectEntry (), NTV2_XptFrameBuffer1RGB);
        router.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptDuallinkOut1);
        router.addWithValue (::GetSDIOut1InputDS2SelectEntry (), NTV2_XptDuallinkOut1DS2);

        router.addWithValue (::GetDualLinkOut2InputSelectEntry (), NTV2_XptFrameBuffer2RGB);
        router.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptDuallinkOut2);
        router.addWithValue (::GetSDIOut2InputDS2SelectEntry (), NTV2_XptDuallinkOut2DS2);

        router.addWithValue (::GetDualLinkOut3InputSelectEntry (), NTV2_XptFrameBuffer3RGB);
        router.addWithValue (::GetSDIOut3InputSelectEntry (), NTV2_XptDuallinkOut3);
        router.addWithValue (::GetSDIOut3InputDS2SelectEntry (), NTV2_XptDuallinkOut3DS2);

        router.addWithValue (::GetDualLinkOut4InputSelectEntry (), NTV2_XptFrameBuffer4RGB);
        router.addWithValue (::GetSDIOut4InputSelectEntry (), NTV2_XptDuallinkOut4);
        router.addWithValue (::GetSDIOut4InputDS2SelectEntry (), NTV2_XptDuallinkOut4DS2);
    }
    else
    {
        router.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptFrameBuffer1YUV);
         router.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptFrameBuffer2YUV);
        router.addWithValue (::GetSDIOut3InputSelectEntry (), NTV2_XptFrameBuffer3YUV);
        router.addWithValue (::GetSDIOut4InputSelectEntry (), NTV2_XptFrameBuffer4YUV);

    }

    mDevice.ApplySignalRoute (router, true);	//	Replace current signal routing


}

void NTV2PlaybackDPX::setupAutoCirculate()
{
    uint32_t	startFrame	(0);
    uint32_t	endFrame	(7);
    mChannelSpec = NTV2CROSSPOINT_CHANNEL1;

    mDevice.StopAutoCirculate (mChannelSpec);	//	Just in case someone else left it running

    ::memset (&mOutputTransferStruct,		0,	sizeof (mOutputTransferStruct));
    ::memset (&mOutputTransferStatusStruct,	0,	sizeof (mOutputTransferStatusStruct));

    mOutputTransferStruct.channelSpec				= mChannelSpec;
    mOutputTransferStruct.videoBufferSize			= mRasterImageSize;
    mOutputTransferStruct.videoDmaOffset			= 0;
    mOutputTransferStruct.audioBufferSize			= 0;
    mOutputTransferStruct.frameRepeatCount			= 1;
    mOutputTransferStruct.desiredFrame				= -1;
    mOutputTransferStruct.frameBufferFormat			= mPixelFormat;
    mOutputTransferStruct.bDisableExtraAudioInfo	= true;

    mDevice.InitAutoCirculate (mOutputTransferStruct.channelSpec, startFrame, endFrame,
                               1,				//	Number of channels
                               NTV2_AUDIOSYSTEM_1,	//	Which audio system?
                               false,		//	With audio?
                               true,			//	Add timecode!
                               false,			//	Allow frame buffer format changes?
                               false,			//	With color correction?
                               false,			//	With vidProc?
                               false,			//	With custom ANC data?
                               false,			//	With LTC?
                               false);			//	With audio2?
}	//	SetUpOutputAutoCirculate



void NTV2PlaybackDPX::readDPXFiles()
{
    mPause = false;
    mCurrentFrameNumber = 0;
    mNumFramesRead = 0;

    // Start Thread that will write file data to Device using NTV4 Sequencer
    mPlaybacktoBoardThraed = new PlayBackToBoardThread(this,NULL);
    mPlaybacktoBoardThraed->start();

    // Start Thread that will write file data to Device using NTV4 Sequencer
    mPlayPreviewThread = new PlayPreviewThread(this,NULL);
    mPlayPreviewThread->start();

    // This loop reads files from disk into Circular buffer.
    while ( !mAbort)
    {
        AJAPlaybackDataBuffer* playData;
        playData = mPlaybackCircularBuffer.StartProduceNextBuffer();
        if ( playData == NULL ) break;

        AJATimer timer;
        timer.Start();

        AJAFileProperties props = (mSmallFile) ? eAJABuffered : eAJAUnbuffered;
        AJAFileIO file;
        AJAStatus status = file.Open(mFileList[mCurrentFrameNumber].absoluteFilePath().toStdString(),eAJAReadOnly,props);
        if ( status == AJA_STATUS_SUCCESS )
        {
            status = file.Seek(mDPXInfo.offset,eAJASeekCurrent);
            if ( status != AJA_STATUS_SUCCESS )
                qDebug() << "DPX Play -  Seek failed";
            uint64_t numBytes = file.Read(playData->videoBuffer.GetBuffer(),mRasterImageSize);
            if ( numBytes != mRasterImageSize )
                qDebug() << "DPX Play - Read failed";
            status = file.Close();
            if (numBytes == 0 )
            {
                mPlaybackCircularBuffer.EndProduceNextBuffer();
                continue;
            }

        }
        else
        {
            qDebug() << "Couldn't Open " << mFileList[mCurrentFrameNumber].absoluteFilePath();
            mAbort = true;
        }

        qDebug("Producer: %d",timer.ElapsedTime());
        timer.Stop();
        AJAVideoPreviewBuffer* previewData = mPreviewCircularBuffer.StartProduceNextBuffer();
        previewData->videoBuffer = (uint32_t*)playData->videoBuffer.GetBuffer();;
        previewData->videoBufferSize = mOutputTransferStruct.videoBufferSize;
        mPreviewCircularBuffer.EndProduceNextBuffer();

        mPlaybackCircularBuffer.EndProduceNextBuffer();

        if ( !mPause )
            mCurrentFrameNumber++;

        if (mCurrentFrameNumber == mNumFiles )
            mCurrentFrameNumber = 0;

        mNumFramesRead++;
    }
    while ( mPlaybacktoBoardThraed->isRunning())
        ;
    while ( mPlayPreviewThread->isRunning())
        ;

}



void NTV2PlaybackDPX::playback()
{
    uint32_t framesTransferred = 0;
    bool flush;

    // Start up Preview Thread
//    CDPXPreviewThread* previewThread = new CDPXPreviewThread();
//    previewThread->SetAbortFlag(&mAbort);
//    previewThread->SetCircularBuffer(mPreviewCircularBuffer);
//    previewThread->SetPreviewInfo(dpxInfo.width,dpxInfo.height,dpxInfo.pixelFormat);



    // This loop reads data from the circular buffer and puts it into the sequencer.
    while ( !mAbort )
    {
        bool pause = mPause;
        AJAPlaybackDataBuffer* playData;
        flush = false;
        AUTOCIRCULATE_STATUS_STRUCT	outputStatus;
        mDevice.GetAutoCirculate (mChannelSpec,&outputStatus);

        //	Check if there's room for another frame on the card...
        if ((outputStatus.bufferLevel < (ULWord)(outputStatus.endFrame - outputStatus.startFrame - 1)))
        {

            if ( pause )
            {

                // get latest playData in circular buffer.
                while ( mPlaybackCircularBuffer.GetCircBufferCount() > 1 )
                {
                    playData = mPlaybackCircularBuffer.StartConsumeNextBuffer();
                    mPlaybackCircularBuffer.EndConsumeNextBuffer();
                }

                flush = true;

            }

            playData = mPlaybackCircularBuffer.StartConsumeNextBuffer();
            if ( playData == NULL ) break;

            AJATimer timer;
            timer.Start();

            //swap B and R
#if 0
                uint16_t* lineBuffer = (uint16_t*)playData->videoBuffer.GetBuffer();
                for ( uint32_t pixel = 0; pixel < mRasterWidth*mRasterHeight; pixel++ )
                {
                    uint16_t red = lineBuffer[0];
                    lineBuffer[0] = lineBuffer[2];
                    lineBuffer[2] = red;
                     lineBuffer += 3; // do every other pixel
                }
#endif


            mOutputTransferStruct.videoBuffer		= (uint32_t*)playData->videoBuffer.GetBuffer();
            mOutputTransferStruct.videoBufferSize	= (uint32_t)mRasterImageSize;
            mOutputTransferStruct.audioBuffer		= NULL;
            mOutputTransferStruct.audioBufferSize	= 0;

            mDevice.TransferWithAutoCirculate (&mOutputTransferStruct, &mOutputTransferStatusStruct);
            qDebug("   Consumer: %d",timer.ElapsedTime());
            timer.Stop();


            //// TransferViaAutoCirculate();!!!!
            ///
            framesTransferred++;
            if ( framesTransferred == 5)
            {
                mDevice.StartAutoCirculate(mChannelSpec);
//                previewThread->start();
            }


            if ( pause )
            {
                ;//emit newStatusString("Scrubbing");
  //              previewFrame(playData->videoBuffer.GetBuffer());

            }
            else if ( outputStatus.bufferLevel > 4 )
            {
                ;//emit newStatusString("Playing");
//                previewFrame(playData->videoBuffer.GetBuffer());
            }

            mPlaybackCircularBuffer.EndConsumeNextBuffer();



        }
        else
            mDevice.WaitForOutputVerticalInterrupt ();
    }

    ////StopAutoCirculate()


}

void NTV2PlaybackDPX::preview()
{
    // This loop reads data from the circular buffer and puts it into the sequencer.
    while ( !mAbort )
    {
                while (mPreviewCircularBuffer.GetCircBufferCount () > 1)
                {
                    qDebug() << "Eating Preview Frames";
                    mPreviewCircularBuffer.StartConsumeNextBuffer ();
                    mPreviewCircularBuffer.EndConsumeNextBuffer ();
                }

        AJAVideoPreviewBuffer* previewData = mPreviewCircularBuffer.StartConsumeNextBuffer();
        if ( previewData == NULL ) break;
        previewFrame( (uint8_t*)previewData->videoBuffer);
        mPreviewCircularBuffer.EndConsumeNextBuffer();
         mDevice.WaitForOutputVerticalInterrupt ();
    }


}

void NTV2PlaybackDPX::previewFrame(uint8_t* videoBuffer)
{

    QImage* currentImage = mQImagePool[mImagePoolIndex];
    mImagePoolIndex++;
    if ( mImagePoolIndex == sPlaybackDPXCircularBufferSize)
        mImagePoolIndex = 0;
    uint8_t* pBits = (uint8_t*) currentImage->bits();

    if ( pBits == NULL )
    {
        qDebug() << "pBits = NULL (play 1 pvw)";
        return;
    }

    if ( videoBuffer == NULL )
    {
        qDebug() << "videoBuffer = NULL";
        return;
    }

    uint32_t line, pixel;
    switch (mPixelFormat)
    {
    case NTV2_FBF_10BIT_DPX:
    {
        if (mUseSSE)
        {
#ifndef AJA_LINUX
            SSE_Frame(SSE_DPX_BE_To_BGRA8_HalfRes,pBits,(mRasterWidth/2)*4,videoBuffer,(mRasterWidth*4*2),mRasterWidth,mRasterHeight/2);
            //qDebug("SSE NTV4_PixelFormat_RGB_DPX Preview: %d",timer.ElapsedTime());
#endif
        }
        else
        {
            for ( line = 0; line < mRasterHeight; line +=2 )
            {
                uint32_t* buffer = (uint32_t*)(videoBuffer + (line*mRasterWidth*4));
                for ( pixel = 0; pixel < mRasterWidth; pixel += 2 )
                {
                    uint32_t value = *buffer;
                    *pBits++ = ((value & 0xF0000000)>>28) + ((value&0x000F0000)>>12); //Blue
                    *pBits++ = ((value & 0x3F00)>>6) + ((value & 0xC00000)>>22);	  //Green
                    *pBits++ = (value&0xFF);										  //Red
                    *pBits++ = 0xFF;
                    buffer += 2;
                }
            }
            //qDebug("Non SSE NTV4_PixelFormat_RGB_DPX Preview: %d",timer.ElapsedTime());
        }

    }
        break;

    case NTV2_FBF_10BIT_DPX_LE:
    {
        if (mUseSSE)
        {
#ifndef AJA_LINUX
            SSE_Frame(SSE_DPX_LE_To_BGRA8_HalfRes,pBits,(mRasterWidth/2)*4,videoBuffer,(mRasterWidth*4*2),mRasterWidth,mRasterHeight/2);
            //qDebug("SSE NTV4_PixelFormat_RGB_DPX_LE Preview: %d",timer.ElapsedTime());
#endif
        }
        else
        {
            for ( line = 0; line < mRasterHeight; line +=2 )
            {
                uint32_t* buffer = (uint32_t*)(videoBuffer + (line*mRasterWidth*4));
                for ( pixel = 0; pixel < mRasterWidth; pixel += 2 )
                {
                    uint32_t value = *buffer;
                    *pBits++ = (value>>4)&0xFF; //Blue
                    *pBits++ = (value>>14)&0xFF;//Green
                    *pBits++ = (value>>24)&0xFF;//Red
                    *pBits++ = 0xFF;
                    buffer += 2;
                }
            }
            //qDebug("Non SSE NTV4_PixelFormat_RGB_DPX_LE Preview: %d",timer.ElapsedTime());

        }
    }
        break;

    case NTV2_FBF_10BIT_YCBCR_DPX:
    {
        // Big Endian.
        //SSE_Frame_HalfRes(SSE_DPX_BE_CbYCrY10_709_To_BGRA8_HalfRes,pBits,(mRasterWidth/2)*4,videoBuffer,(mRasterWidth / 6) * 4 * 4,mRasterWidth,mRasterHeight);
        AJATimer timer;
        timer.Start();
        if ( pBits != NULL )
        {
#ifndef AJA_LINUX
            int pitch =  AJA_CalcRowBytesForFormat(AJA_PixelFormat_YCbCr10,mRasterWidth);
            SSE_Frame_HalfRes(SSE_DPX_BE_CbYCrY10_709_To_BGRA8_HalfRes,
                              pBits,						// target
                              (mRasterWidth/2)*4,			// target pitch
                              videoBuffer,					// source
                              pitch,						// source pitch
                              mRasterWidth,					// source width
                              mRasterHeight);				// source height
            qDebug("SSE NTV4_PixelFormat_YCbCr_DPX Preview: %d",timer.ElapsedTime());
#endif
        }
        else
        {
            qDebug() << "pBits = NULL (play 2 pvw )";
        }
    }
        break;
    case NTV2_FBF_48BIT_RGB:
    {
        AJATimer timer;
        timer.Start();
        for ( line = 0; line < mRasterHeight; line +=2 )
        {
            uint16_t* lineBuffer = (uint16_t*)(videoBuffer + (line*mRasterWidth*6));
            for ( pixel = 0; pixel < mRasterWidth; pixel += 2 )
            {
//                                *pBits++ =  ((*lineBuffer++)>>8); //Blue
//                                *pBits++ =  ((*lineBuffer++)>>8);//Green
//                                *pBits++ =  ((*lineBuffer++)>>8);//Red
                 *pBits++ =  (*(lineBuffer+2)>>8); //Blue
                 *pBits++ =  (*(lineBuffer+1)>>8);//Green
                 *pBits++ =  (*(lineBuffer+0)>>8);//Red
                *pBits++ = 0xFF;
                lineBuffer += 6; // do every other pixel
            }
        }
        timer.Stop();
        qDebug("NTV2_FBF_48BIT_RGB Preview: %d",timer.ElapsedTime());

        break;
    }
    default:
        break;

    }

    emit newFrameSignal(*currentImage,true);
}




uint32_t NTV2PlaybackDPX::getFramesRead()
{
    return mNumFramesRead;
}


uint32_t NTV2PlaybackDPX::getFramesDropped()
{
    AUTOCIRCULATE_STATUS_STRUCT	outputStatus;
    mDevice.GetAutoCirculate (mChannelSpec,&outputStatus);

    return outputStatus.framesDropped;
}


void NTV2PlaybackDPX::setPause(bool pause)
{
    mPause = pause;
}


void NTV2PlaybackDPX::nextFrame()
{
    if ( mPause )
    {
        if ((mCurrentFrameNumber+1) != mNumFiles )
            mCurrentFrameNumber++;
    }
}


void NTV2PlaybackDPX::previousFrame()
{
    if ( mPause )
    {
        if (mCurrentFrameNumber != 0)
            --mCurrentFrameNumber;
    }
}


void NTV2PlaybackDPX::setCurrentFrame(int32_t frame)
{
    if ( frame >=0 && frame < (int32_t)mNumFiles )
        mCurrentFrameNumber = frame;
}


int32_t NTV2PlaybackDPX::getCurrentFrameNumber()
{
    return mCurrentFrameNumber;
}


int32_t NTV2PlaybackDPX::getNumberOfFrames()
{
    return mNumFiles;
}

PlayBackToBoardThread::PlayBackToBoardThread(NTV2PlaybackDPX* playbackDPXThread,QObject *parent)
    : QThread(parent),mPlaybackDPXThread(playbackDPXThread)
{

}

void PlayBackToBoardThread::run()
{
    setPriority(QThread::TimeCriticalPriority);
    mPlaybackDPXThread->playback( );
}

PlayBackToBoardThread::~PlayBackToBoardThread()
{

}


PlayPreviewThread::PlayPreviewThread(NTV2PlaybackDPX* playbackDPXThread,QObject *parent)
    : QThread(parent),mPlaybackDPXThread(playbackDPXThread)
{

}

void PlayPreviewThread::run()
{
    setPriority(QThread::TimeCriticalPriority);
    mPlaybackDPXThread->preview( );
}

PlayPreviewThread::~PlayPreviewThread()
{

}
