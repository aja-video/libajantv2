//---------------------------------------------------------------------------------------------------------------------
//  ntv2ogloutput.cpp
//
//	Copyright (C) 2012 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
//---------------------------------------------------------------------------------------------------------------------
#include "ntv2ogloutput.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/common/videoutilities.h"
#include "ntv2boardfeatures.h"
#include "ntv2boardscan.h"
#include "ntv2utils.h"
#include "ntv2signalrouter.h"	//xena2routing.h"

#include <string>
#include <sstream>
#include <iomanip>


using std::string;

NTV2OglOutput::NTV2OglOutput(int deviceIndex, uint32_t numBitsPerComponent, IOglTransfer *gpuTransfer)
: CVideoProcessor<COglObject>(gpuTransfer)
{
	mBoardIndex = deviceIndex;
	mWidth    = 0;
	mHeight   = 0;
	mFrameBufferFormat = NTV2_FBF_ABGR;
	mChannel = NTV2_CHANNEL1;
	mChannelSpec = NTV2CROSSPOINT_CHANNEL1;
	mMinFrame = 8;
	mMaxFrame = 14;

	mNumOglObjects = 3;
	mNumBitsPerComponent = numBitsPerComponent;
}

NTV2OglOutput::~NTV2OglOutput()
{

}


bool NTV2OglOutput::Init()
{
	if(mGpuQueue[INQ] == NULL || mGpuTransfer == NULL)
		return false;

	mNTV2Card.Close();
	CNTV2DeviceScanner ntv2BoardScan;
	NTV2DeviceInfo boardInfo;
	ntv2BoardScan.GetDeviceInfo(mBoardIndex,boardInfo);
	mBoardID = boardInfo.deviceID;

	if ( !mNTV2Card.Open(boardInfo.deviceIndex))
		return false;

	mNTV2Card.GetVideoFormat(&mVideoFormat);
	SetupOutput();
	SetupAutoCirculate();

	//	This is for the timecode that we will burn onto the image...
	NTV2FormatDescriptor fd = GetFormatDescriptor (GetNTV2StandardFromVideoFormat (mVideoFormat),
		mFrameBufferFormat,
		false,
		false,
		false);

	mHeight = fd.numLines;
	mWidth  = fd.numPixels;

	//create GPU objects
	GpuObjectDesc desc;
	switch(mNumBitsPerComponent)
	{
		case 8:
			desc._internalformat = GL_RGBA8;
			desc._format = GL_RGBA;
			desc._type = GL_UNSIGNED_BYTE;
			break;
		case 10:
			desc._internalformat = GL_RGB10_A2UI;
			desc._format = GL_RGBA_INTEGER;
			desc._type = GL_UNSIGNED_INT_10_10_10_2;
			break;
		default:
			desc._internalformat = GL_RGBA8;
			desc._format = GL_RGBA;
			desc._type = GL_UNSIGNED_BYTE;
			break;
	}
	desc._height = mHeight;
	desc._width = mWidth;
	desc._useTexture = true;
	desc._useRenderToTexture = true;
	mOglObjects = new COglObject[mNumOglObjects];
	for(int i = 0; i< mNumOglObjects; i ++)
	{
		//init each object. This must be done when the OpenGL context is current
		mOglObjects[i].Init(desc);
		//register the object with the Gpu transfer. This must be done when the OpenGL context is current
		mGpuTransfer->RegisterTexture(&mOglObjects[i]);
		//finally, add it to the circular buffer
		mGpuQueue[INQ]->Add(&mOglObjects[i]);
	}


	// create the system memory objects
	//we need to regsiter the objects with the Gpu transfer. This must be done when the OpenGL context is current
	CpuObjectDesc desc2;
	desc2._format = GL_RGBA;
	switch(mNumBitsPerComponent)
	{
		case 8:
			desc2._type = GL_UNSIGNED_BYTE;
			break;
		case 10:
			desc2._type = GL_UNSIGNED_INT_10_10_10_2;
			break;
		default:
			desc2._type = GL_UNSIGNED_BYTE;
			break;
	}
	desc2._height = mHeight;
	desc2._width = mWidth;
	desc2._useTexture = true;
	desc2._stride = desc2._width*oglFormatToBytes(desc2._format,desc2._type);
	mGpuTransfer->GetGpuPreferredAllocationConstants(&desc2._addressAlignment, &desc2._strideAlignment);

	mCpuObjects = new CCpuObject[mNumOglObjects];
	for(int i = 0; i< mNumOglObjects; i ++)
	{
		//init each object
		mCpuObjects[i].Init(desc2);
		//register the object with the Gpu transfer. This must be done when the OpenGL context is current
		mGpuTransfer->RegisterTexture(&mCpuObjects[i]);

		//finally, add it to the circular buffer if the CPU circular buffer exists
		if(mCpuQueue[INQ])
			mCpuQueue[INQ]->Add(&mCpuObjects[i]);

	}
	return true;
}

bool NTV2OglOutput::Deinit()
{
	if(mGpuTransfer == NULL)
		return false;

	for( int32_t i = 0; i < mNumOglObjects ; i++ ) {
			mGpuTransfer->UnregisterTexture(&mOglObjects[i]);
			mGpuTransfer->UnregisterTexture(&mCpuObjects[i]);
	}
	delete []  mOglObjects;
	delete []  mCpuObjects;


	StopAutoCirculate();

	return true;
}

//
bool NTV2OglOutput::SetupOutput()
{
	/**
		Output routing should be based on the number of frame stores.
		Two channel boards will input on channel 1 and output on channel 2.
		Four channel boards will input on channel 1 and output on channel 3.
		This was done because:
			1.	When you switch between a Kona3G and a Kona3GQuad firmware,
				the physical cabling must be changed;
			2.	The crosspoint selections were too large to fit into the FPGA,
				so we had to limit routing options on 4 channel boards.
		Boards that support HDMI and/or analog outputs will also be connected.
	**/


	CNTV2SignalRouter	router;
#if 1
	if (NTV2BoardGetNumVideoChannels (mBoardID) <= 2)
	{
		if (IsRGBFormat (mFrameBufferFormat))
		{
			//	Add routes from the frame buffer to a color space converter to all outputs...
			router.addWithValue (GetCSC2VidInputSelectEntry(), NTV2_XptFrameBuffer2RGB);
			if (NTV2BoardCanDoWidget (mBoardID, NTV2_Wgt3GSDIOut2) || NTV2BoardCanDoWidget (mBoardID, NTV2_WgtSDIOut2))
				router.addWithValue (GetSDIOut2InputSelectEntry(), NTV2_XptCSC2VidYUV);
			else
				router.addWithValue (GetSDIOut1InputSelectEntry(), NTV2_XptCSC2VidYUV);
			if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtHDMIOut1))
				router.addWithValue (GetHDMIOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
			if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtAnalogOut1))
				router.addWithValue (GetAnalogOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
		}
		else
		{
			//	Add the route from frame buffer 2 to output 2...
			if (NTV2BoardCanDoWidget (mBoardID, NTV2_Wgt3GSDIOut2) || NTV2BoardCanDoWidget (mBoardID, NTV2_WgtSDIOut2))
				router.addWithValue (GetSDIOut2InputSelectEntry(), NTV2_XptFrameBuffer2YUV);
			else
				router.addWithValue (GetSDIOut1InputSelectEntry(), NTV2_XptFrameBuffer2YUV);
			if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtHDMIOut1))
				router.addWithValue (GetHDMIOutInputSelectEntry(), NTV2_XptFrameBuffer2YUV);
			if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtAnalogOut1))
				router.addWithValue (GetAnalogOutInputSelectEntry(), NTV2_XptFrameBuffer2YUV);
		}
	}	//	if 1 or 2 channel board
	else
	{
		if (IsRGBFormat (mFrameBufferFormat))
		{
			//	Add routes from the frame buffer to a color space converter to all outputs...
			router.addWithValue (GetCSC3VidInputSelectEntry(), NTV2_XptFrameBuffer3RGB);
			router.addWithValue (GetSDIOut3InputSelectEntry(), NTV2_XptCSC3VidYUV);

			if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtHDMIOut1))
				router.addWithValue (GetHDMIOutInputSelectEntry(), NTV2_XptCSC3VidYUV);
			if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtAnalogOut1))
				router.addWithValue (GetAnalogOutInputSelectEntry(), NTV2_XptCSC3VidYUV);
		}
		else
		{
			//	Add the route from frame buffer 3 to output 3...
			router.addWithValue (GetSDIOut3InputSelectEntry(), NTV2_XptFrameBuffer3YUV);

			if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtHDMIOut1))
				router.addWithValue (GetHDMIOutInputSelectEntry(), NTV2_XptFrameBuffer3YUV);
			if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtAnalogOut1))
				router.addWithValue (GetAnalogOutInputSelectEntry(), NTV2_XptFrameBuffer3YUV);
		}
	}	//	else 4-channel board
#endif
	if (NTV2BoardHasBiDirectionalSDI (mBoardID))
	{
		mNTV2Card.SetSDITransmitEnable (NTV2_CHANNEL3, true);

	}

	#if defined (AJAMac)		//	Bug in Mac driver 10.4.5 or earlier necessitates this
		if (NTV2BoardCanDoWidget (mBoardID, NTV2_WgtHDMIOut1))
		{
			NTV2FrameRate		frameRate		(NTV2_FRAMERATE_UNKNOWN);
			const NTV2Standard	videoStandard	(::GetNTV2StandardFromVideoFormat (mVideoFormat));

			//	HDMI needs a bit more set-up...
			mNTV2Card.GetFrameRate (&frameRate);
			mNTV2Card.SetHDMIOutVideoStandard (videoStandard);
			mNTV2Card.SetHDMIOutVideoFPS (frameRate);
		}
	#endif	//	AJAMac
	mNTV2Card.ApplySignalRoute (router);
	return true;

}
void NTV2OglOutput::SetupAutoCirculate()
{
	::memset (&mTransferStruct, 0x0, sizeof (mTransferStruct));
	::memset (&mTransferStatusStruct, 0x0, sizeof (mTransferStatusStruct));

	if (NTV2BoardGetNumVideoChannels (mBoardID) <= 2)
	{
		mTransferStruct.channelSpec = NTV2CROSSPOINT_CHANNEL2;
		mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL2,mFrameBufferFormat);
	}
	else
	{
		mTransferStruct.channelSpec = NTV2CROSSPOINT_CHANNEL3;
		mNTV2Card.SetFrameBufferFormat(NTV2_CHANNEL3,mFrameBufferFormat);
	}
	mChannelSpec = mTransferStruct.channelSpec;

	mNTV2Card.StopAutoCirculate (mTransferStruct.channelSpec);

	mTransferStruct.videoBufferSize		    = 0;
	mTransferStruct.videoDmaOffset		    = 0;
	mTransferStruct.audioBufferSize		    = 0;
	mTransferStruct.frameRepeatCount		= 1;
	mTransferStruct.desiredFrame			= -1;
	mTransferStruct.frameBufferFormat		= mFrameBufferFormat;
	mTransferStruct.bDisableExtraAudioInfo  = true;

	//	Tell playout AutoCirculate to use frame buffers 10-19 on the board...
	const uint32_t	startFrame	(10);
	const uint32_t	endFrame	(19);

	mNTV2Card.InitAutoCirculate (mTransferStruct.channelSpec,
		startFrame,
		endFrame,
		1,						//	Number of channels
		NTV2_AUDIOSYSTEM_1,		//	Which audio system?
		false,				//	With audio?
		true,					//	With RP188?
		false,					//	Allow frame buffer format changes?
		false,					//	With color correction?
		false,					//	With vidProc?
		false,					//	With custom ANC data?
		false,					//	With LTC?
		false);					//	With audio2?
}

void NTV2OglOutput::StartAutoCirculate()
{
	mNTV2Card.StartAutoCirculate(mChannelSpec);
}

void NTV2OglOutput::StopAutoCirculate()
{

}



bool NTV2OglOutput::SetupThread()
{
	mNumFramesDropped = 0;

	// Start Thread that will write file data to Device using NTV2 Autocirculate
	mGpuTransfer->BeginTransfers();

	return true;
}

bool NTV2OglOutput::CleanupThread()
{

	mGpuTransfer->EndTransfers();
	mNTV2Card.StopAutoCirculate(mChannelSpec);

	return true;
}

bool NTV2OglOutput::Process()
{
	////Check Buffer Level
	static int numTransferred = 0;

	AUTOCIRCULATE_STATUS_STRUCT	outputStatus;
	mNTV2Card.GetAutoCirculate(mChannelSpec, &outputStatus );


	//	Check if there's room for another frame on the card...
	if ((outputStatus.bufferLevel < (ULWord)(outputStatus.endFrame - outputStatus.startFrame - 1)))
	{

		COglObject *gpuObject;
		CCpuObject *cpuObject;
		gpuObject = mGpuQueue[INQ]->StartConsumeNextBuffer();
		if(gpuObject == NULL) return false;
		if(mCpuQueue[INQ])
		{
			cpuObject = mCpuQueue[INQ]->StartConsumeNextBuffer();
			if(cpuObject == NULL) return false;
		}
		else
		{
			cpuObject = &mCpuObjects[0];
		}
		//	// Prepare for DMA transfer
		mGpuTransfer->BeforePlaybackTransfer(gpuObject, cpuObject);
		mTransferStruct.videoBuffer = (ULWord*)cpuObject->GetVideoBuffer();
		mTransferStruct.videoBufferSize = (ULWord)cpuObject->GetSize();
		mNTV2Card.TransferWithAutoCirculate(&mTransferStruct, &mTransferStatusStruct);
		numTransferred++;
		if (numTransferred == 1)
			StartAutoCirculate();
		mGpuTransfer->AfterPlaybackTransfer(gpuObject, cpuObject);
		mGpuQueue[INQ]->EndConsumeNextBuffer();
		if(mCpuQueue[INQ])
		{
			mCpuQueue[INQ]->EndConsumeNextBuffer();
		}
	}
	else
	{
		mNTV2Card.WaitForOutputVerticalInterrupt();

	}
	return true;
}


uint32_t NTV2OglOutput::getFramesDropped()
{
	return mNumFramesDropped;
}
