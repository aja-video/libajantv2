//---------------------------------------------------------------------------------------------------------------------
//  NTV2CudaCapture.cpp
//
//	Copyright (C) 2012 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
//---------------------------------------------------------------------------------------------------------------------
#include "ntv2cudacapture.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/common/videoutilities.h"
#include "ntv2boardfeatures.h"
#include "ntv2boardscan.h"
#include "ntv2utils.h"
#include <QDate>
#include <string>
#include <sstream>
#include <iomanip>
 
using std::string;


//
//	@brief	The user chooses an Input by NTV2InputSource.
//			This maps it to the proper ChannelSpec used by Autocirculate.
//
static const NTV2Crosspoint inputSourceToChannelSpec [] =
{
	NTV2CROSSPOINT_INPUT1,	//	NTV2_INPUTSOURCE_SDI,
	NTV2CROSSPOINT_INPUT1,	//	NTV2_INPUTSOURCE_ANALOG,
	NTV2CROSSPOINT_INPUT2,	//	NTV2_INPUTSOURCE_SDI2,
	NTV2CROSSPOINT_INPUT1,	//	NTV2_INPUTSOURCE_HDMI,
	NTV2CROSSPOINT_INPUT1,	//	NTV2_INPUTSOURCE_DUALLINK,
	NTV2CROSSPOINT_INPUT1,	//	NTV2_INPUTSOURCE_DUALLINK2,
	NTV2CROSSPOINT_INPUT3,	//	NTV2_INPUTSOURCE_SDI3,
	NTV2CROSSPOINT_INPUT4,	//	NTV2_INPUTSOURCE_SDI4,
	NTV2CROSSPOINT_INPUT1,	//	NTV2_INPUTSOURCE_DUALLINK3,
	NTV2CROSSPOINT_INPUT1,	//	NTV2_INPUTSOURCE_DUALLINK4,
	NTV2CROSSPOINT_INPUT1,	//	NTV2_INPUTSOURCE_SDI1_DS2,
	NTV2CROSSPOINT_INPUT2,	//	NTV2_INPUTSOURCE_SDI2_DS2,
	NTV2CROSSPOINT_INPUT3,	//	NTV2_INPUTSOURCE_SDI3_DS2,
	NTV2CROSSPOINT_INPUT4,	//	NTV2_INPUTSOURCE_SDI4_DS2,
	NTV2CROSSPOINT_INPUT1	//	NTV2_NUM_INPUTSOURCES
};


//
//	@brief	The user chooses an Input by NTV2InputSource.
//			This maps it to the proper Channel. The channel
//			is basically the FrameStore we capture in to.
//
static const NTV2Channel inputSourceToChannel [] =
{
	NTV2_CHANNEL1,	//	NTV2_INPUTSOURCE_SDI,
	NTV2_CHANNEL1,	//	NTV2_INPUTSOURCE_ANALOG,
	NTV2_CHANNEL2,	//	NTV2_INPUTSOURCE_SDI2,
	NTV2_CHANNEL1,	//	NTV2_INPUTSOURCE_HDMI,
	NTV2_CHANNEL1,	//	NTV2_INPUTSOURCE_DUALLINK,
	NTV2_CHANNEL1,	//	NTV2_INPUTSOURCE_DUALLINK2,
	NTV2_CHANNEL3,	//	NTV2_INPUTSOURCE_SDI3,
	NTV2_CHANNEL4,	//	NTV2_INPUTSOURCE_SDI4,
	NTV2_CHANNEL1,	//	NTV2_INPUTSOURCE_DUALLINK3,
	NTV2_CHANNEL1,	//	NTV2_INPUTSOURCE_DUALLINK4,
	NTV2_CHANNEL1,	//	NTV2_INPUTSOURCE_SDI1_DS2,
	NTV2_CHANNEL2,	//	NTV2_INPUTSOURCE_SDI2_DS2,
	NTV2_CHANNEL3,	//	NTV2_INPUTSOURCE_SDI3_DS2,
	NTV2_CHANNEL4,	//	NTV2_INPUTSOURCE_SDI4_DS2,
	NTV2_CHANNEL1	//	NTV2_NUM_INPUTSOURCES
};


//
//	@brief	This maps an Input to a human-readable name.
//
static const QString inputSourceToQString [] =
{
	"SDI Input 1",	//	NTV2_INPUTSOURCE_SDI,
	"Analog Input",	//	NTV2_INPUTSOURCE_ANALOG,
	"SDI Input 2",	//	NTV2_INPUTSOURCE_SDI2,
	"HDMI",			//	NTV2_INPUTSOURCE_HDMI,
	"DUALLINK",		//	NTV2_INPUTSOURCE_DUALLINK,
	"DUALLINK2",	//	NTV2_INPUTSOURCE_DUALLINK2,
	"SDI Input 3",	//	NTV2_INPUTSOURCE_SDI3,
	"SDI Input 4",	//	NTV2_INPUTSOURCE_SDI4,
	"DUALLINK3",	//	NTV2_INPUTSOURCE_DUALLINK3,
	"DUALLINK4",	//	NTV2_INPUTSOURCE_DUALLINK4,
	"SDI Input 1 DS2",//	NTV2_INPUTSOURCE_SDI1_DS2,
	"SDI Input 2 DS2",//	NTV2_INPUTSOURCE_SDI2_DS2,
	"SDI Input 3 DS2",//	NTV2_INPUTSOURCE_SDI3_DS2,
	"SDI Input 4 DS2",//	NTV2_INPUTSOURCE_SDI4_DS2,
	"No Input"		//	NTV2_NUM_INPUTSOURCES
};
#define NTV2_AUDIOSIZE_MAX	(401*1024)
static bool get4KInputFormat(NTV2VideoFormat & videoFormat);
NTV2CudaCapture::NTV2CudaCapture(int deviceIndex,  ICudaTransfer *gpuTransfer,  bool rgbCapture)
: CVideoProcessor<CCudaObject>(gpuTransfer), mRGBCapture(rgbCapture)
{
	mBoardIndex = deviceIndex;
	mWidth    = 0;
	mHeight   = 0;
	mRestart = true;;	
	mCheckFor4K = false;
	mFrameBufferFormat = NTV2_FBF_ABGR;
	mFormatProgressive=true;
	mInputSource = NTV2_INPUTSOURCE_SDI1;
	mChannelSpec = NTV2CROSSPOINT_INPUT1;
	mMinFrame = 0;
	mMaxFrame = 7;
	mbWithAudio = false;
	mTransferStruct.audioBuffer = NULL;
	mVideoFormatDebounceCounter = 0;
	mRestoreMode = NTV2_STANDARD_TASKS;
		
	mNumCudaObjects = 3;
}

NTV2CudaCapture::~NTV2CudaCapture()
{

}


bool NTV2CudaCapture::Init()
{
	if(mGpuQueue[OUTQ] == NULL || mGpuTransfer == NULL)
		return false;
	mNTV2Card.Close();
	CNTV2BoardScan ntv2BoardScan;
	OurBoardInfo boardInfo;
	ntv2BoardScan.GetBoardInfo(mBoardIndex,boardInfo,false);
	mBoardID = boardInfo.boardID;

	if ( !mNTV2Card.Open(boardInfo.boardNumber,false,boardInfo.boardType))
		return false;

	SetupInput();
	SetupAutoCirculate();

	//create GPU objects 
	GpuObjectDesc desc;
	desc._format = CU_AD_FORMAT_UNSIGNED_INT8;
	desc._numChannels = 4;
	desc._height = mHeight;
	desc._width = mWidth;
	desc._useTexture = true;
	desc._useRenderToTexture = false;	
	mCudaObjects = new CCudaObject[mNumCudaObjects];
	for(int i = 0; i< mNumCudaObjects; i ++)
	{		
		//init each object. This must be done when the OpenGL context is current
		mCudaObjects[i].Init(desc);	
		//regsiter the object with the Gpu transfer. This must be done when the OpenGL context is current
		mGpuTransfer->RegisterTexture(&mCudaObjects[i]);	
		//finally, add it to the circular buffer 
		mGpuQueue[OUTQ]->Add(&mCudaObjects[i]);
	}
	
	
	// create the system memory objects
	//we need to regsiter the objects with the Gpu transfer. This must be done when the OpenGL context is current
	CpuObjectDesc desc2;
	desc2._format = CU_AD_FORMAT_UNSIGNED_INT8;
	desc2._numChannels = 4;
	desc2._height = mHeight;
	desc2._width = mWidth;
	desc2._useTexture = true;
	desc2._stride = desc2._width*desc2._numChannels*cudaArrayFormatToBytes((CUarray_format)desc2._format);
	mGpuTransfer->GetGpuPreferredAllocationConstants(&desc2._addressAlignment, &desc2._strideAlignment);
	 
	mCpuObjects = new CCpuObject[mNumCudaObjects];
	for(int i = 0; i< mNumCudaObjects; i ++)
	{		
		//init each object
		mCpuObjects[i].Init(desc2);	
		//regsiter the object with the Gpu transfer. This must be done when the OpenGL context is current
		mGpuTransfer->RegisterTexture(&mCpuObjects[i]);	
		
		//finally, add it to the circular buffer if the CPU circular buffer exists
		if(mCpuQueue[OUTQ])
			mCpuQueue[OUTQ]->Add(&mCpuObjects[i]);

	}		

	return true;
}

bool NTV2CudaCapture::Deinit()
{			
	if(mGpuTransfer == NULL)
		return false;

	for( int32_t i = 0; i < mNumCudaObjects ; i++ ) {		
			mGpuTransfer->UnregisterTexture(&mCudaObjects[i]);			
			mGpuTransfer->UnregisterTexture(&mCpuObjects[i]);			
	}	
	delete []  mCudaObjects;	
	delete []  mCpuObjects;
	return true;
}





bool NTV2CudaCapture::SetupThread()
{
	mNumFramesDropped = 0;
	
	mGpuTransfer->BeginTransfers();
	StartAutoCirculate();

	return true;
}
bool NTV2CudaCapture::CleanupThread()
{
	mGpuTransfer->EndTransfers();
	return true;
}

bool NTV2CudaCapture::Process()
{
	AUTOCIRCULATE_STATUS_STRUCT			acStatus;
	mNTV2Card.GetAutoCirculate(mChannelSpec, &acStatus );

	if (acStatus.state == NTV2_AUTOCIRCULATE_RUNNING && acStatus.bufferLevel > 1)
	{
		CCudaObject *gpuObject;
		CCpuObject *cpuObject;
		gpuObject = mGpuQueue[OUTQ]->StartProduceNextBuffer();
		if(gpuObject == NULL) return false;	
		if(mCpuQueue[OUTQ])
		{
			cpuObject = mCpuQueue[OUTQ]->StartProduceNextBuffer();
			if(cpuObject == NULL) return false;		
		}
		else
		{
			cpuObject = &mCpuObjects[0];
		}

		//	// Prepare for DMA transfer
		mGpuTransfer->BeforeRecordTransfer(gpuObject, cpuObject);	
		mTransferStruct.videoBuffer = (ULWord*)cpuObject->GetVideoBuffer();
		mTransferStruct.videoBufferSize = (ULWord)cpuObject->GetSize();
		mNTV2Card.TransferWithAutoCirculate(&mTransferStruct, &mTransferStatusStruct);
		mGpuTransfer->AfterRecordTransfer(gpuObject, cpuObject);
		mGpuQueue[OUTQ]->EndProduceNextBuffer();
		if(mCpuQueue[OUTQ])
		{
			mCpuQueue[OUTQ]->EndProduceNextBuffer();
		}
	}
	else
	{
		mNTV2Card.WaitForVerticalInterrupt();
	}
	
	return true;
}

bool NTV2CudaCapture::SetupInput()
{
	mCurrentVideoFormat = GetVideoFormatFromInputSource();
	mChannelSpec = inputSourceToChannelSpec[mInputSource];

	// Check for bi-directional sdi, if so set it to capture.
	if(NTV2BoardHasBiDirectionalSDI(mBoardID) && mInputSource != NTV2_INPUTSOURCE_HDMI && mInputSource != NTV2_INPUTSOURCE_ANALOG)
	{
		switch(mInputSource)
		{
		case NTV2_INPUTSOURCE_SDI1:
			mNTV2Card.SetSDITransmitEnable(NTV2_CHANNEL1, false);
			mNTV2Card.SetMode(inputSourceToChannel[mInputSource],NTV2_MODE_CAPTURE);
			if(mCheckFor4K)
			{
				mNTV2Card.SetSDITransmitEnable(NTV2_CHANNEL2, false);
				mNTV2Card.SetSDITransmitEnable(NTV2_CHANNEL3, false);
				mNTV2Card.SetSDITransmitEnable(NTV2_CHANNEL4, false);

				mNTV2Card.SetMode(inputSourceToChannel[NTV2_INPUTSOURCE_SDI2],NTV2_MODE_CAPTURE);
				mNTV2Card.SetMode(inputSourceToChannel[NTV2_INPUTSOURCE_SDI3],NTV2_MODE_CAPTURE);
				mNTV2Card.SetMode(inputSourceToChannel[NTV2_INPUTSOURCE_SDI4],NTV2_MODE_CAPTURE);
			}
			break;
		case NTV2_INPUTSOURCE_SDI2:
			mNTV2Card.SetSDITransmitEnable(NTV2_CHANNEL2, false);
			mNTV2Card.SetMode(inputSourceToChannel[mInputSource],NTV2_MODE_CAPTURE);
			break;
		case NTV2_INPUTSOURCE_SDI3:
			mNTV2Card.SetSDITransmitEnable(NTV2_CHANNEL3, false);
			mNTV2Card.SetMode(inputSourceToChannel[mInputSource],NTV2_MODE_CAPTURE);
			break;
		case NTV2_INPUTSOURCE_SDI4:
			mNTV2Card.SetSDITransmitEnable(NTV2_CHANNEL4, false);
			mNTV2Card.SetMode(inputSourceToChannel[mInputSource],NTV2_MODE_CAPTURE);
			break;
		default:
			//qDebug("NTV2FrameGrabber::SetupInput bad input switch value %d\n", mInputSource);
			break;
		}
	}

	mWidth = 1920;
	mHeight = 1080;

	bool validInput = false;
	if ( mCurrentVideoFormat != NTV2_FORMAT_UNKNOWN)
	{
		validInput = true;
		mNTV2Card.SetVideoFormat(mCurrentVideoFormat);
		mFormatProgressive = IsProgressivePicture(mCurrentVideoFormat);
		NTV2Standard standard=GetNTV2StandardFromVideoFormat(mCurrentVideoFormat);
		NTV2FormatDescriptor fd = GetFormatDescriptor(standard,mFrameBufferFormat,false,false);
		if ( NTV2_IS_4K_VIDEO_FORMAT(mCurrentVideoFormat) )
		{
			fd.numLines *= 2;
			fd.numPixels  *= 2;
			fd.linePitch *= 2;

		}
		mHeight = fd.numLines;
		mWidth = fd.numPixels;

		switch ( inputSourceToChannelSpec[mInputSource])
		{
		case NTV2CROSSPOINT_INPUT1:
			if ( IsRGBFormat(mFrameBufferFormat))
			{
				mNTV2Card.SetK2Xpt1ColorSpaceConverterInputSelect(NTV2K2_XptSDIIn1);
				mNTV2Card.SetK2Xpt2FrameBuffer1InputSelect(NTV2K2_XptCSCRGB);	
				mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL1,mFrameBufferFormat);
				if ( NTV2_IS_4K_VIDEO_FORMAT(mCurrentVideoFormat) )
				{
					mNTV2Card.SetK2Xpt5CSC2VidInputSelect(NTV2K2_XptSDIIn2);
					mNTV2Card.SetK2Xpt5FrameBuffer2InputSelect(NTV2K2_XptCSC2VidRGB);
					mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL2,mFrameBufferFormat);
					mNTV2Card.SetChannel2Disable(false);

					mNTV2Card.SetK2Xpt17CSC3VidInputSelect(NTV2K2_XptSDIIn3);
					mNTV2Card.SetK2Xpt13FrameBuffer3InputSelect(NTV2K2_XptCSC3VidRGB);
					mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL3,mFrameBufferFormat);
					mNTV2Card.SetChannel3Disable(false);

					mNTV2Card.SetK2Xpt17CSC4VidInputSelect(NTV2K2_XptSDIIn4);
					mNTV2Card.SetK2Xpt13FrameBuffer4InputSelect(NTV2K2_XptCSC4VidRGB);
					mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL4,mFrameBufferFormat);
					mNTV2Card.SetChannel4Disable(false);

				}

			}
			else
			{
				mNTV2Card.SetK2Xpt2FrameBuffer1InputSelect(NTV2K2_XptSDIIn1);	
				mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL1,mFrameBufferFormat);
				if ( NTV2_IS_4K_VIDEO_FORMAT(mCurrentVideoFormat) )
				{
					mNTV2Card.SetK2Xpt5FrameBuffer2InputSelect(NTV2K2_XptSDIIn2);
					mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL2,mFrameBufferFormat);
					mNTV2Card.SetChannel2Disable(false);

					mNTV2Card.SetK2Xpt13FrameBuffer3InputSelect(NTV2K2_XptSDIIn3);
					mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL3,mFrameBufferFormat);
					mNTV2Card.SetChannel3Disable(false);

					mNTV2Card.SetK2Xpt13FrameBuffer4InputSelect(NTV2K2_XptSDIIn4);
					mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL4,mFrameBufferFormat);
					mNTV2Card.SetChannel4Disable(false);
				}
			}
			break;
		case NTV2CROSSPOINT_INPUT2:			
			if ( IsRGBFormat(mFrameBufferFormat))
			{
				mNTV2Card.SetK2Xpt5CSC2VidInputSelect(NTV2K2_XptSDIIn2);
				mNTV2Card.SetK2Xpt5FrameBuffer2InputSelect(NTV2K2_XptCSC2VidRGB);
				mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL2,mFrameBufferFormat);
			}
			else
			{
				mNTV2Card.SetK2Xpt5FrameBuffer2InputSelect(NTV2K2_XptSDIIn2);
				mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL2,mFrameBufferFormat);
			}
			mNTV2Card.SetChannel2Disable(false);
			break;
		case NTV2CROSSPOINT_INPUT3:			
			if ( IsRGBFormat(mFrameBufferFormat))
			{
				mNTV2Card.SetK2Xpt17CSC3VidInputSelect(NTV2K2_XptSDIIn3);
				mNTV2Card.SetK2Xpt13FrameBuffer3InputSelect(NTV2K2_XptCSC3VidRGB);
				mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL3,mFrameBufferFormat);
			}
			else
			{
				mNTV2Card.SetK2Xpt13FrameBuffer3InputSelect(NTV2K2_XptSDIIn3);
				mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL3,mFrameBufferFormat);
			}
			mNTV2Card.SetChannel3Disable(false);
			break;
		case NTV2CROSSPOINT_INPUT4:
			if ( IsRGBFormat(mFrameBufferFormat))
			{
				mNTV2Card.SetK2Xpt17CSC4VidInputSelect(NTV2K2_XptSDIIn4);
				mNTV2Card.SetK2Xpt13FrameBuffer4InputSelect(NTV2K2_XptCSC4VidRGB);
				mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL4,mFrameBufferFormat);
			}
			else
			{
				mNTV2Card.SetK2Xpt13FrameBuffer4InputSelect(NTV2K2_XptSDIIn4);
				mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL4,mFrameBufferFormat);
			}
			mNTV2Card.SetChannel4Disable(false);
			break;
		default:
			//qDebug("NTV2FrameGrabber::SetupInput bad channel spec switch value %d\n", inputSourceToChannelSpec[mInputSource]);
			break;
		}



	}

	return validInput;

}

void NTV2CudaCapture::SetupAutoCirculate()
{
	StopAutoCirculate();
	memset ((void *) &mTransferStruct, 0, sizeof(mTransferStruct));
	mTransferStruct.channelSpec = mChannelSpec;

	mTransferStruct.transferFlags = 0;
	mTransferStruct.bDisableExtraAudioInfo = true;
	mTransferStruct.audioStartSample = 0;
	mTransferStruct.audioNumChannels = 16;
	mTransferStruct.frameRepeatCount = 1;
	mTransferStruct.desiredFrame = -1;
	mTransferStruct.frameBufferFormat = mFrameBufferFormat;

	NTV2AudioSystem audioSystem = NTV2_AUDIOSYSTEM_1;

	switch ( mChannelSpec )
	{
	case NTV2CROSSPOINT_INPUT1:
		mMinFrame = 0;
		mMaxFrame = 7;
		audioSystem = NTV2_AUDIOSYSTEM_1;
		break;
	case NTV2CROSSPOINT_INPUT2:
		mMinFrame = 8;
		mMaxFrame = 14;
		audioSystem = NTV2_AUDIOSYSTEM_2;
		break;
	case NTV2CROSSPOINT_INPUT3:
		mMinFrame = 15;
		mMaxFrame = 21;
		audioSystem = NTV2_AUDIOSYSTEM_3;
		break;
	case NTV2CROSSPOINT_INPUT4:
		mMinFrame = 22;
		mMaxFrame = 28;
		audioSystem = NTV2_AUDIOSYSTEM_4;
		break;
	default:
		//qDebug("NTV2FrameGrabber::SetupAutoCirculate bad switch value %d\n", mChannelSpec);
		break;
	}

	// Reserve Space for largest audio packet/frame
	mTransferStruct.audioBuffer = new ULWord[NTV2_AUDIOSIZE_MAX/4];
	mTransferStruct.audioBufferSize = NTV2_AUDIOSIZE_MAX;			

	mVideoBuffer.AllocateBuffer(mHeight*mWidth*4);

	mNTV2Card.InitAutoCirculate(mChannelSpec,
		mMinFrame,
		mMaxFrame,
		1,
		audioSystem,
		true,
		false,
		false,
		false,
		false,
		false,
		false,
		false);


}

void NTV2CudaCapture::StartAutoCirculate()
{
	mNTV2Card.StartAutoCirculate(mChannelSpec);
}

void NTV2CudaCapture::StopAutoCirculate()
{
	mNTV2Card.StopAutoCirculate(mChannelSpec);

}




NTV2VideoFormat NTV2CudaCapture::GetVideoFormatFromInputSource()
{
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;

	switch ( mInputSource )
	{
	case NTV2_INPUTSOURCE_SDI1:
		{
			videoFormat = mNTV2Card.GetInput1VideoFormat();
			NTV2Standard videoStandard = GetNTV2StandardFromVideoFormat(videoFormat);
			if ( mCheckFor4K && (videoStandard ==  NTV2_STANDARD_1080p))
			{
				NTV2VideoFormat videoFormatNext;
				videoFormatNext = mNTV2Card.GetInput2VideoFormat();
				if ( videoFormatNext == videoFormat )
				{
					videoFormatNext = mNTV2Card.GetInput3VideoFormat();
					if ( videoFormatNext == videoFormat )
					{
						videoFormatNext = mNTV2Card.GetInput4VideoFormat();
						if ( videoFormatNext == videoFormat )
						{
							get4KInputFormat(videoFormat);
						}
					}
				}
			}
		}
		break;
	case NTV2_INPUTSOURCE_SDI2:
		videoFormat = mNTV2Card.GetInput2VideoFormat();
		break;
	case NTV2_INPUTSOURCE_SDI3:
		videoFormat = mNTV2Card.GetInput3VideoFormat();
		break;
	case NTV2_INPUTSOURCE_SDI4:
		videoFormat = mNTV2Card.GetInput4VideoFormat();
		break;
	case NTV2_INPUTSOURCE_ANALOG:
		videoFormat = mNTV2Card.GetAnalogInputVideoFormat();
		break;
	case NTV2_INPUTSOURCE_HDMI:
		videoFormat = mNTV2Card.GetHDMIInputVideoFormat();
		break;
	case NTV2_INPUTSOURCE_DUALLINK:
		videoFormat = mNTV2Card.GetInput1VideoFormat();
		break;
	case NTV2_NUM_INPUTSOURCES:
		// indicates no source is currently selected
		break;
	default:
		//qDebug("NTV2FrameGrabber::GetVideoFormatFromInputSource bad switch value %d\n", mInputSource);
		break;
	}

	return videoFormat;
}

uint32_t NTV2CudaCapture::getFramesDropped()
{
	return mNumFramesDropped;
}

//
//get4KInputFormat
//If all 4 inputs are the same and promotable to 4K this does the promotion.
bool get4KInputFormat(NTV2VideoFormat & videoFormat)
{
	bool status = false;

	struct VideoFormatPair {NTV2VideoFormat vIn; NTV2VideoFormat vOut;} VideoFormatPairs[] = 
	{
		{NTV2_FORMAT_1080psf_2398, NTV2_FORMAT_4x1920x1080psf_2398},
		{NTV2_FORMAT_1080psf_2400, NTV2_FORMAT_4x1920x1080psf_2400},
		{NTV2_FORMAT_1080p_2398,   NTV2_FORMAT_4x1920x1080p_2398},
		{NTV2_FORMAT_1080p_2400,   NTV2_FORMAT_4x1920x1080p_2400},
		{NTV2_FORMAT_1080p_2500,   NTV2_FORMAT_4x1920x1080p_2500},
		{NTV2_FORMAT_1080p_2997,   NTV2_FORMAT_4x1920x1080p_2997},
		{NTV2_FORMAT_1080p_3000,   NTV2_FORMAT_4x1920x1080p_3000},
		{NTV2_FORMAT_1080p_5994,   NTV2_FORMAT_4x1920x1080p_5994},
		{NTV2_FORMAT_1080p_6000,   NTV2_FORMAT_4x1920x1080p_6000}

	} ;

	for(size_t i=0; i<sizeof(VideoFormatPairs)/sizeof(VideoFormatPair); i++ )
	{
		if ( VideoFormatPairs[i].vIn == videoFormat )
		{
			videoFormat = VideoFormatPairs[i].vOut;
			status = true;
		}
	}

	return status;

}


