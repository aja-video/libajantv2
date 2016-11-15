//---------------------------------------------------------------------------------------------------------------------
//  NTV2RecordDPX.cpp
//
//	Copyright (C) 2012 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
//---------------------------------------------------------------------------------------------------------------------
#include "ntv2recorddpx.h"

#include "ajabase/system/systemtime.h"
#include "ajabase/common/videoutilities.h"
#include "ajabase/common/timer.h"
#include "ajabase/common/pixelformat.h"
#include "ajabase/common/testpatterngen.h"

////#include "ntv4/common/videoformatinfo.h"
////#include "ntv4/common/pixelformatinfo.h"
////#include "ntv4/widgets/sdiinputv3widget.h"

#include <QDate>
////#include <QAudioFormat>
#include <string>
#include <sstream>
#include <iomanip>

#ifndef AJA_LINUX
#define AJA_USESSE
#ifdef AJA_USESSE
#include "ajasse/common/ajasse.h"
#endif 
#endif

#undef RECORD_AUDIO

#define DEBUG_TIMING

using std::string;

NTV2RecordDPX::NTV2RecordDPX(int deviceIndex,
							 QString directoryName,
							 bool previewOnly,
							 bool withAudio,
                             NTV2FrameBufferFormat pixelFormat,
							 bool EtoE,
							 bool crop4KInput
							 )
    : NTV2RecordThread(RECORD_TYPE_DPX,previewOnly),
	mWithAudio(withAudio),
	mPixelFormat(pixelFormat),
	mEtoE(EtoE),
	mInputCrop(crop4KInput)
{
	mSelectedDevice = deviceIndex;

	mDirectoryPath  = directoryName;

	mWaveWriter			= NULL_PTR;
	mAbort          = false;
	mSmallFile      = false;
	mQueueSize      = 17;
	mImageSize      = 0;
	mRasterWidth    = 0;
	mRasterHeight   = 0;
	mNumFramesWritten = 0;
	mNumFramesDropped = 0;
	mNumAudioChannels = 16;
	mWithAudio = false;
	mAudioSize = 0;

	for (int i=0; i<sRecordDPXCircularBufferSize; i++ )
	{
		mQImagePool[i] = NULL_PTR;
	}

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

	//testSSE1();
	//testSSE2();

	start();
	setPriority(QThread::TimeCriticalPriority);	// must set priority when running
}


NTV2RecordDPX::~NTV2RecordDPX()
{
    qDebug() << "NTV2RecordDPX destructor start";

	mAbort = true;


	while ( isRunning())
		; //wait for main thread to finish.



    qDebug() << "NTV2RecordDPX destructor end";
}


void NTV2RecordDPX::run()
{
#if 0
	AJAStatus status = mpSetup->waitForVideoInput(200);
	if (status == AJA_STATUS_TIMEOUT)
		return;

	NTV4VideoConfiguration vc;
	mpSetup->determineInputConfig(vc,true);

	vc.GetVideoFormat(mVideoFormat);
	qDebug() << "Record format = " << mpSetup->getFormatInfo(mVideoFormat).c_str();
	Q_ASSERT(mVideoFormat != NTV4_VideoFormat_Unknown);

	NTV4SDITransport sdiTransport = mpSetup->mSDITransport[0];

	// if it is 4K input - crop if configured
	uint32_t width;
	bool crop = false;
	vc.GetVideoWidth(width);
	if (width == 2048 && mInputCrop)
	{
		NTV4SDIInputStatus cropStatus;
		if (mpSetup->mQuadInput)
		{
			NTV4SDIInputV3Widget * pw;
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[0]);
			pw->Set2kVideoCropMode(NTV4_VideoCropMode_Right);
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[1]);
			pw->Set2kVideoCropMode(NTV4_VideoCropMode_Left);
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[2]);
			pw->Set2kVideoCropMode(NTV4_VideoCropMode_Right);
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[3]);
			pw->Set2kVideoCropMode(NTV4_VideoCropMode_Left);
			
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[0]);
			pw->GetCropStatus(&cropStatus, &mpSetup->mInputStatus[0]);

			vc = cropStatus.videoConfig;
			vc.GetVideoFormat(mVideoFormat);
			mVideoFormat = NTV4VideoFormatInfo::GetQuadFormat(mVideoFormat);
			vc.SetVideoFormat(mVideoFormat);
			crop = true;
		}
		else
		{
			NTV4SDIInputV3Widget * pw;
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[0]);
			pw->Set2kVideoCropMode(NTV4_VideoCropMode_Center);
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[1]);
			pw->Set2kVideoCropMode(NTV4_VideoCropMode_Center);
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[2]);
			pw->Set2kVideoCropMode(NTV4_VideoCropMode_Center);
			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[3]);
			pw->Set2kVideoCropMode(NTV4_VideoCropMode_Center);

			pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[0]);
			pw->GetCropStatus(&cropStatus, &mpSetup->mInputStatus[0]);

			vc = cropStatus.videoConfig;
			vc.GetVideoFormat(mVideoFormat);
			crop = true;
		}
	}
	if (!crop)
	{
		NTV4SDIInputV3Widget * pw;
		pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[0]);
		pw->Set2kVideoCropMode(NTV4_VideoCropMode_Off);
		pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[1]);
		pw->Set2kVideoCropMode(NTV4_VideoCropMode_Off);
		pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[2]);
		pw->Set2kVideoCropMode(NTV4_VideoCropMode_Off);
		pw = dynamic_cast<NTV4SDIInputV3Widget *>(mpSetup->mpSDIInput[3]);
		pw->Set2kVideoCropMode(NTV4_VideoCropMode_Off);
	}

	mpSetup->deviceLock(mRefLock,vc,mEtoE, &sdiTransport);

	mpSetup->setupCaptureBuffers(vc, 
		mPixelFormat, 
		mQueueSize,
		mVideoConfig,
		mImageSize);

	mVideoConfig.GetPictureWidth(mRasterWidth);
	mVideoConfig.GetPictureHeight(mRasterHeight);

	// Setup Preview QImages
	for (int i=0; i<sRecordDPXCircularBufferSize; i++ )
	{
		mQImagePool[i] = new QImage(mRasterWidth/2,mRasterHeight/2,QImage::Format_RGB32);
	}
	mImagePoolIndex = 0;


	mpSetup->setupInputPath(&vc,&mVideoConfig,mEtoE, &sdiTransport);

#ifdef RECORD_AUDIO
	mpSetup->setupAudioBuffers(mVideoFormat, mQueueSize,	mAudioSize);
#endif

	setupSequencer();	
	setupCircularBuffer(mImageSize,mAudioSize);
	setupDPXHeader(mRasterWidth, mRasterHeight, 
		mVideoConfig.GetVideoFrameRateDuration(),
		mVideoConfig.GetVideoFrameRateScale());


#ifdef RECORD_AUDIO
	if ( mWithAudio)
		setupAudioRecord();
#endif

	recordFromDevice(); // this will run until thread is terminated.

	qDebug() << "Deleting Image Pool.....";
	for (int i=0; i<sRecordDPXCircularBufferSize; i++ )
	{
		if (mQImagePool[i] != NULL_PTR)
		{
			delete mQImagePool[i];
			mQImagePool[i] = NULL_PTR;
		}
	}
	qDebug() << "Image Pool - deleted";
	qDebug() << "DPX Record Producer - exited";
#endif
}

void NTV2RecordDPX::setupDPXHeader(uint32_t frameWidth, uint32_t frameHeight, uint32_t duration, uint32_t scale)
{
#if 0
	memset((void*)&mDPXHeader.GetHdr(), 0xff, mDPXHeader.GetHdrSize());		// SMPTE 268 requirement

    if ( mPixelFormat == NTV2_FBF_10BIT_DPX_LITTLEENDIAN || mPixelFormat == NTV2_FBF_10BIT_YCBCR_DPX)
		mDPXHeader.init(DPX_C_MAGIC);
	else
		mDPXHeader.init(DPX_C_MAGIC_BE);
		
	// File Information
	mDPXHeader.set_fi_project("NTV4");
	mDPXHeader.set_fi_version("V2.0");
    mDPXHeader.set_fi_creator("AJA Video");
	mDPXHeader.set_fi_copyright("2012");
	mDPXHeader.set_fi_image_offset(mDPXHeader.GetHdrSize());
    if ( mPixelFormat == NTV2_FBF_10BIT_DPX_LITTLEENDIAN || mPixelFormat == NTV2_FBF_10BIT_DPX)
		mDPXHeader.set_fi_file_size(mDPXHeader.GetHdrSize()+frameWidth*frameHeight*4);
	else
		mDPXHeader.set_fi_file_size(mDPXHeader.GetHdrSize()+frameWidth*frameHeight*8/3);


	// Image Info
	mDPXHeader.set_ii_orientation(0);
	mDPXHeader.set_ii_element_number(1);
	mDPXHeader.set_ii_pixels(frameWidth);
	mDPXHeader.set_ii_lines(frameHeight);

	// Image Element 0
	mDPXHeader.set_ie_data_sign (0);
	mDPXHeader.set_ie_ref_low_data(0);		
	mDPXHeader.set_ie_ref_high_data(1023); 
	mDPXHeader.set_ie_ref_low_quantity(0.0);
	mDPXHeader.set_ie_ref_high_quantity(0.0);
	if ( mPixelFormat == NTV4_PixelFormat_YCbCr_DPX || mPixelFormat == NTV4_PixelFormat_YCbCr10)
		mDPXHeader.set_ie_descriptor(100);
	else
		mDPXHeader.set_ie_descriptor(50);

	NTV4VideoStandard std = NTV4VideoFormatInfo::GetVideoStandard(mVideoFormat);
	switch (std)
	{
	case NTV4_VideoStandard_486i:	// 525
		mDPXHeader.set_ie_transfer(8);	
		mDPXHeader.set_ie_colorimetric(8);	
		break;

	case NTV4_VideoStandard_576i:	// 625
		mDPXHeader.set_ie_transfer(7);	
		mDPXHeader.set_ie_colorimetric(7);
		break;

	default:	// HD
		mDPXHeader.set_ie_transfer(6);	
		mDPXHeader.set_ie_colorimetric(6);
		break;
	}

	mDPXHeader.set_ie_bit_size(10);
	mDPXHeader.set_ie_packing(1);
	mDPXHeader.set_ie_encoding(0);
	mDPXHeader.set_ie_data_offset(mDPXHeader.GetHdrSize());

	mDPXHeader.set_ie_eol_padding(0);
	mDPXHeader.set_ie_eo_image_padding(0);
	string description = "AJA DPX Capture";
	mDPXHeader.set_ie_description(description);

	// Image Source Info
	mDPXHeader.set_is_filename("");
	mDPXHeader.set_is_input_device("CorvidUltra");
	mDPXHeader.set_is_input_serial("AJA001");

	// Film Header
	mDPXHeader.set_film_mfg_id("");
	mDPXHeader.set_film_type("");
	mDPXHeader.set_film_offset("");
	mDPXHeader.set_film_prefix("");
	mDPXHeader.set_film_count("");
	mDPXHeader.set_film_format("");
	mDPXHeader.set_film_frame_position(1); 
	mDPXHeader.set_film_sequence_len(1 ); 
	mDPXHeader.set_film_held_count(1); 
	mDPXHeader.set_film_frame_rate((float)duration/(float)scale);
	mDPXHeader.set_film_shutter_angle(0.0 );
	mDPXHeader.set_film_frame_id("");
	mDPXHeader.set_film_slate_info("");

	// TV Header
	mDPXHeader.set_tv_timecode(0); 
	mDPXHeader.set_tv_userbits(0); 
	mDPXHeader.set_tv_frame_rate(0.0);

	NTV4VideoScan scan = NTV4VideoFormatInfo::GetVideoScan(mVideoFormat);
	switch (scan)
	{
	default:
	case NTV4_VideoScan_Progressive:
		mDPXHeader.set_tv_interlace(0);
		break;
	case NTV4_VideoScan_TopFieldFirst:
	case NTV4_VideoScan_BottomFieldFirst:
		mDPXHeader.set_tv_interlace(1);
		break;
	}

	if (NTV4VideoFormatInfo::isSD(mVideoFormat))
	{
		mSmallFile = true;
	}
#endif
}

void NTV2RecordDPX::setupCircularBuffer(uint32_t imageSize,uint32_t audioSize)
{
	AJAStatus status;
	mRecordCircularBuffer.Init(&mAbort);
	
	uint32_t allocateSize = (imageSize + 4095)& (~4095);

	for ( int i=0; i<sRecordDPXCircularBufferSize; i++ )
	{		
		status = mAVDataBuffer[i].videoBuffer.AllocateBuffer(allocateSize,4096,NULL);
		if ( AJA_STATUS_SUCCESS != status ) qDebug() << "Couldn't allocate video";
#ifdef RECORD_AUDIO
		status = mAVDataBuffer[i].audioBuffer.AllocateBuffer(audioSize,4096,NULL);
		if ( AJA_STATUS_SUCCESS != status ) qDebug() << "Couldn't allocate audio";
#endif
		mRecordCircularBuffer.Add(&mAVDataBuffer[i]);
	}
}

void NTV2RecordDPX::setupAutoCirculate()
{
#if 0
	AJAStatus status;

#ifdef RECORD_AUDIO
	// setup capture audio track
	mpAudioCaptureTrack = new NTV4AudioTrack();
	status = mpAudioCaptureTrack->Initialize(mpSetup->mpDevice);
	if (AJA_FAILURE(status))
	{
		return;
	}
	mpAudioCaptureTrack->AddWidget(mpSetup->mpAudioBuf);
	mpAudioCaptureTrack->SetCaptureMode(true);
#endif

	// setup video track
	mpVideoCaptureTrack = new NTV4VideoTrack();
	status = mpVideoCaptureTrack->Initialize(mpSetup->mpDevice);
	if (AJA_FAILURE(status))
	{
		return;
	}

	mpVideoCaptureTrack->AddWidget(mpSetup->mpFBCapture[0]);
	uint32_t numVideoChannels;
	mVideoConfig.GetNumberOfVideoChannels(numVideoChannels);
	if (numVideoChannels == 4) 
	{
		mpVideoCaptureTrack->AddWidget(mpSetup->mpFBCapture[1]);
		mpVideoCaptureTrack->AddWidget(mpSetup->mpFBCapture[2]);
		mpVideoCaptureTrack->AddWidget(mpSetup->mpFBCapture[3]);
	}
	mpVideoCaptureTrack->SetCaptureMode(true);
	mpVideoCaptureTrack->SetTrackAdvance(1);


	// setup playout video track for hdmi output
	if (numVideoChannels == 4) 
	{
		mpVideoPlayoutTrack = new NTV4VideoTrack();
		status = mpVideoPlayoutTrack->Initialize(mpSetup->mpDevice);
		if (AJA_FAILURE(status))
		{
			return;
		}
		mpVideoPlayoutTrack->AddWidget(mpSetup->mpFBPlay[4]);
		mpVideoPlayoutTrack->AddWidget(mpSetup->mpFBPlay[5]);
		mpVideoPlayoutTrack->SetCaptureMode(false);
		mpVideoPlayoutTrack->SetTransferMode(false);
	}

	// setup sequencer
	mpCaptureSequencer = new NTV4Sequencer("DPX Record");
	status = mpCaptureSequencer->Initialize(mpSetup->mpDevice);
	if (AJA_FAILURE(status))
	{
		return;
	}
	mpCaptureSequencer->AddTrack(mpVideoCaptureTrack);
	if (numVideoChannels == 4) 
	{
		mpCaptureSequencer->AddTrack(mpVideoPlayoutTrack);
	}
#ifdef RECORD_AUDIO
	mpCaptureSequencer->AddTrack(mpAudioCaptureTrack);
#endif
	mpCaptureSequencer->SetClockSource(NTV4_InterruptSource_SDIInput1Frame);
	mpCaptureSequencer->SetClockRate(mVideoConfig.GetVideoFrameRateScale(),mVideoConfig.GetVideoFrameRateDuration());
	mpCaptureSequencer->SetQueueSize(mQueueSize);

	// prepare for capture
	status = mpCaptureSequencer->Prepare();
	if (AJA_FAILURE(status))
	{
		return;
	}

	// wait for preparation complete
	status = mpCaptureSequencer->WaitForState(NTV4_SequencerState_Stop,2000);
	if (AJA_FAILURE(status))
	{
		return;
	}
#endif
}


void NTV2RecordDPX::recordFromDevice()
{
#if 0
	mAbort = false;
	mNumFramesDropped = 0;
	uint32_t numFramesFromDevice = 0;
	qRegisterMetaType<QImage>("QImage");
	// Start Thread that will write DPX File and Preview a decoded Frame now and then
	mWriteDPXThread = new WriteDPXFileThread(this,NULL);

	AJAStatus status = mpCaptureSequencer->Start();
	if (AJA_FAILURE(status))
	{
		return;
	}

	// This loop reads data from the circular buffer and puts it into the sequencer.
	while ( !mAbort )
	{
		AJARecordDPXDataBuffer* recordData;
		recordData = mRecordCircularBuffer.StartProduceNextBuffer();
		if ( recordData == NULL ) 
		{
			mAbort = true;
			break;
		}

		NTV4VideoTrackTransfer videoTransfer(recordData->videoBuffer.GetBuffer(), mImageSize);
		mpVideoCaptureTrack->WaitForVideoBuffer(500);

		mpVideoCaptureTrack->TransferVideoBuffer(videoTransfer);

#ifdef RECORD_AUDIO
		NTV4AudioTrackTransfer audioTransfer(recordData->audioBuffer.GetBuffer(), recordData->audioBuffer.GetBufferSize(),recordData->audioRecordSize);
		mpAudioCaptureTrack->WaitForAudioBuffer(0); // if we got a video buffer a corresponding audio buffer must be ready.
		status = mpAudioCaptureTrack->TransferAudioBuffer(audioTransfer);
		recordData->audioRecordSize = audioTransfer.audioTransferSize;
#endif

		NTV4SequencerStatus recordStatus;
		mpCaptureSequencer->GetStatus(recordStatus);
		mNumFramesDropped = recordStatus.dropCount;

		numFramesFromDevice++;
	
		if ( (getPreview() == false) && (recordStatus.driverLevel > 8) )
		{
			emit newStatusString("Recording");
			previewFrame(recordData->videoBuffer.GetBuffer(),"Recording");
		}


		mRecordCircularBuffer.EndProduceNextBuffer();
	}


	qDebug() << "Waiting for Record DPX Consumer exit......";
	while ( mWriteDPXThread->isRunning())
		; // wait for writeDPX Thread to finish.....
	qDebug() << "Record DPX Consumer exited";

	mpCaptureSequencer->Stop();
	mpCaptureSequencer->Release();

	emit newStatusString("");

	qDebug() << "recordFromDevice() - exited";
#endif


}



void NTV2RecordDPX::writeDPXFiles(uint32_t frameWidth, uint32_t frameHeight)
{
	mNumFramesWritten = 0;
	qRegisterMetaType<QImage>("QImage");
	// This loop writes files to disk into Circular buffer.
	while ( !mAbort )
	{
		AJARecordDPXDataBuffer* recordData;
		recordData = mRecordCircularBuffer.StartConsumeNextBuffer();
		if ( recordData == NULL ) 
		{
			mAbort = true;
			break;
		}


		if ( true == getPreview() )
		{
			emit newStatusString("Previewing");

			//previewFrame((uint8_t*)recordData->videoBuffer,"Previewing");
			previewFrame(recordData->videoBuffer.GetBuffer(),"Previewing");

			mRecordCircularBuffer.EndConsumeNextBuffer();

			while ( !mRecordCircularBuffer.IsEmpty() )
			{
				recordData = mRecordCircularBuffer.StartConsumeNextBuffer();
				mRecordCircularBuffer.EndConsumeNextBuffer();
			}
			//mNumFramesWritten = 0;
		}
		else
		{
			// Just write DPX file per frame as usual

			QChar fillChar = '0';
			QString fileNumberStr = QString("%1").arg(mNumFramesWritten+1,8,10,fillChar);
			QString fileNameStr = getDirectoryPath()+ '/' + fileNumberStr + ".dpx";
			//QString seqDirString = QString("Sequence%1").arg(dirNum,4,10,fillChar);
			//nextSequenceDirectory = currentDirectory + '/' + seqDirString;

			//std::stringstream fns;
			//fns << std::setw(7) << std::setfill('0') << mNumFramesWritten ;
			//std::string fileNameStr = mDirectoryPath + '/' + fns.str() + ".dpx";

			QDate date = QDate::currentDate(); 
			QTime time = QTime::currentTime();
			QDateTime dt(date,time);
			QString dtStr = dt.toString(Qt::ISODate);

			mDPXHeader.set_fi_file_name(fileNameStr.toStdString());
			mDPXHeader.set_is_creation_time(dtStr.toStdString());
			mDPXHeader.set_fi_create_time(dtStr.toStdString());

			//uint32_t timecode = 0;//GetTimecodeFromFrameCount(0);
			//endian_swap(timecode);
			//mDPXHeader.set_tv_timecode(timecode);
//AJATimer timer;
//timer.Start();
			AJAFileProperties props = (mSmallFile) ? eAJABuffered : eAJAUnbuffered;
			AJAFileIO file;	
			if (AJA_SUCCESS(file.Open(fileNameStr.toStdString(),eAJACreateAlways | eAJAWriteOnly, props)))
			{
				if (file.Write((uint8_t*)&(mDPXHeader.GetHdr()),sizeof(DpxHdr)) != sizeof(DpxHdr))
				{
					qDebug() << "DPX write header error";

				}
				if (file.Write(recordData->videoBuffer.GetBuffer(),mImageSize) != mImageSize)
				{
					qDebug() << "DPX write video error";
				}
				file.Close();
				mNumFramesWritten++;

#ifdef RECORD_AUDIO
			if ( mWithAudio)
				outputAudio((uint32_t*)recordData->audioBuffer.GetBuffer(),recordData->audioRecordSize);
#endif
			//qDebug("Consume %d %d",mNumFramesWritten,timer.ElapsedTime());
			//timer.Stop();
			}
			else
			{
				qDebug("File open error: writeDPXFiles");
			}
			mRecordCircularBuffer.EndConsumeNextBuffer();

		}
	}
	if (mWaveWriter)
		mWaveWriter->close();

	qDebug() << "Record DPX Consumer exit";

}

void NTV2RecordDPX::previewFrame(uint8_t* videoBuffer,QString statusString)
{
#if 0
	QImage* currentImage = mQImagePool[mImagePoolIndex];
	mImagePoolIndex++;
	if ( mImagePoolIndex >= sRecordDPXCircularBufferSize)
		mImagePoolIndex = 0;
	uint8_t* pBits = (uint8_t*) currentImage->bits();

	if ( pBits == NULL )
	{
		qDebug() << "pBits = NULL (record pvw)";
		return;
	}

	uint32_t line, pixel;
	switch (mPixelFormat)
	{
	case NTV4_PixelFormat_RGB_DPX:
		{
			AJATimer timer;
			timer.Start();
			if (mUseSSE)
			{
#ifndef AJA_LINUX
				SSE_Frame(SSE_DPX_BE_To_BGRA8_HalfRes,
					pBits,					// target
					(mRasterWidth/2)*4,		// target pitch
					videoBuffer,			// source
					(mRasterWidth*4*2),		// source pitch
					mRasterWidth,			// xRes
					mRasterHeight/2);		// yRes
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
	case NTV4_PixelFormat_RGB_DPX_LE:
		{
			AJATimer timer;
			timer.Start();
			if (mUseSSE)
			{
#ifndef AJA_LINUX
				SSE_Frame(SSE_DPX_LE_To_BGRA8_HalfRes,
					      pBits,				// target
						  (mRasterWidth/2)*4,	// target pitch
						  videoBuffer,			// source
						  (mRasterWidth*4*2),	// source pitch
						  mRasterWidth,			// xRes
						  mRasterHeight/2);		// yRes

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

			timer.Stop();

		}
		break;
	case NTV4_PixelFormat_YCbCr_DPX:
		{
			AJATimer timer;
			timer.Start();
#ifndef AJA_LINUX
			// Big Endian.
////			uint32_t pitch = NTV4PixelFormatInfo::ConvertPixelsToBytesPitch(NTV4_PixelFormat_YCbCr_DPX, mRasterWidth);
			SSE_Frame_HalfRes(SSE_DPX_BE_CbYCrY10_709_To_BGRA8_HalfRes,
								pBits,						// target
								(mRasterWidth/2)*4,			// target pitch
								videoBuffer,				// source
								pitch,				       // source pitch
								mRasterWidth,				// source width
								mRasterHeight);				// source height
			qDebug("SSE NTV4_PixelFormat_YCbCr_DPX Preview: %d",timer.ElapsedTime());
#endif
			timer.Stop();

		}
		break;
	default:
		break;

	}

	emit newFrameSignal(*currentImage,true);
#endif
}

void NTV2RecordDPX::setupAudioRecord()
{
	AJAWavWriterAudioFormat audioFormat;
	audioFormat.sampleRate   = 48000;
	audioFormat.channelCount = 2;
	audioFormat.sampleSize   = 16;

	QChar fillChar = '0';
	QString fileNumberStr = QString("%1").arg(1,8,10,fillChar);
	QString fileNameStr = getDirectoryPath()+ '/' + fileNumberStr + ".wav";

	mWaveWriter = new AJAWavWriter(fileNameStr.toStdString(),audioFormat);
	mWaveWriter->open();
}

void NTV2RecordDPX::outputAudio(uint32_t* buffer, uint32_t bufferSize)
{
	// Now copy over only channel 1 and 2
	uint16_t channel = 0;
	uint16_t sampleCount = 0;
	uint32_t numSamplesRead = bufferSize/(mNumAudioChannels*4);
	uint16_t* audioBuffer = (uint16_t *)buffer;
	for ( uint32_t count = 0; count < bufferSize/4; count++ )
	{
		if ( channel == 0 || channel == 1 )
		{
			audioBuffer[sampleCount] = (buffer[count]>>16);
			sampleCount++;
		}
		channel++;
		if ( channel == mNumAudioChannels )
			channel = 0;

	}

	if ( mWaveWriter )
		mWaveWriter->write((const char*)audioBuffer,numSamplesRead*4);

}

WriteDPXFileThread::WriteDPXFileThread(NTV2RecordDPX* recordDPXThread,QObject *parent)
: QThread(parent),mRecordDPXThread(recordDPXThread)
{
	start();
	setPriority(QThread::TimeCriticalPriority);

}

void WriteDPXFileThread::run()
{
	mRecordDPXThread->writeDPXFiles(mRecordDPXThread->mRasterWidth, 
		                            mRecordDPXThread->mRasterHeight);
}

WriteDPXFileThread::~WriteDPXFileThread()
{

}


#ifdef AJA_USESSE
//#define TEST_SSE
#ifdef TEST_SSE

void testSSE1()
{
	struct tst1
	{
		uint8_t  source [3456*720];
		uint8_t  pad1   [4096];
		uint8_t  dest   [((1280/2)*4) * (720/2)];
		uint8_t  pad2   [4096];
	};
	
	struct tst1 * pt = (struct tst1*)malloc (sizeof tst1);
	
	memset(pt->source,0x01, 3456*720);
	memset(pt->pad1,  0x02, 4096);
	memset(pt->dest,  0x03, ((1280/2)*4) * (720/2));
	memset(pt->pad2,  0x04, 4096);
	
	SSE_Frame_HalfRes(SSE_DPX_BE_CbYCrY10_709_To_BGRA8_HalfRes,
					  pt->dest,						// target
					  (1280/2)*4,					// target pitch
					  pt->source,						// source
					  3456,						// source pitch
					  1280,						// source width
					  720);						// source height
}

void testSSE2()
{
	#define WIDTH  720
	#define HEIGHT 486
	#define PITCH  1920
	struct tst1
	{
		uint8_t  source [PITCH*HEIGHT];
		uint8_t  pad1   [4096];
		uint8_t  dest   [((WIDTH/2)*4) * (HEIGHT/2)];
		uint8_t  pad2   [4096];
	};
	
	struct tst1 * pt = (struct tst1*)malloc (sizeof tst1);
	
	memset(pt->source,0x01, PITCH*HEIGHT);
	memset(pt->pad1,  0x02, 4096);
	memset(pt->dest,  0x03, ((WIDTH/2)*4) * (HEIGHT/2));
	memset(pt->pad2,  0x04, 4096);
	
	SSE_Frame_HalfRes(SSE_DPX_BE_CbYCrY10_709_To_BGRA8_HalfRes,
					  pt->dest,						// target
					  (WIDTH/2)*4,					// target pitch
					  pt->source,						// source
					  PITCH,						// source pitch
					  WIDTH,						// source width
					  HEIGHT);						// source height

}
#endif
#endif
