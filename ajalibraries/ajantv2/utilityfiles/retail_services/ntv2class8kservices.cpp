//
//  ntv2class8kservices.cpp
//
//  Copyright (c) 2019 AJA Video, Inc. All rights reserved.
//

#include "ntv2class8kservices.h"


//--------------------------------------------------------------------------------------------------
//	class Class8kServices
//--------------------------------------------------------------------------------------------------

Class8kServices::Class8kServices(NTV2DeviceID devID) : 
	Class4kServices(devID)
{
}


Class8kServices::~Class8kServices()
{
}


//--------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceXPointPlayback ()
{
	bool 				b8K 				= NTV2_IS_QUAD_QUAD_FORMAT(mFb1VideoFormat);
	bool				bFb1RGB				= IsRGBFormat(mFb1Format);
	bool				bHdmiOutRGB			= mDs.hdmiOutColorSpace == kHDMIOutCSCRGB8bit || mDs.hdmiOutColorSpace == kHDMIOutCSCRGB10bit ||
											  mDs.hdmiOutColorSpace == kHDMIOutCSCRGB12bit;
	
	if (!b8K)
		return Class4kServices::SetDeviceXPointPlayback();
		
	// Frame Buffer
	mCard->Connect(NTV2_XptFrameBuffer1Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
	
	mCard->EnableChannel(NTV2_CHANNEL1);
	mCard->EnableChannel(NTV2_CHANNEL2);
	mCard->EnableChannel(NTV2_CHANNEL3);
	mCard->EnableChannel(NTV2_CHANNEL4);

	
	// SDI Out 1
	if (bFb1RGB)
	{
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1RGB);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}
	else
	{
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}
	
	// SDI Out 2
	if (bFb1RGB)
	{
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameBuffer2RGB);
		mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
	}
	else
	{
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameBuffer2YUV);
		mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
	}
	
	
	// SDI Out 3
	if (bFb1RGB)
	{
		mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer3RGB);
		mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
	}
	else
	{
		mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer3YUV);
		mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
	}


	// SDI Out 4
	if (bFb1RGB)
	{
		mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer4RGB);
		mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
	}
	else
	{
		mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer4YUV);
		mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
	}
	
	
	// HDMI Out
	if (bHdmiOutRGB)
	{
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameBuffer1RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameBuffer1YUV);
	}
	mCard->Connect (NTV2_XptHDMIOutQ2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ4Input, NTV2_XptBlack);


	// CSC 1
	mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptBlack);


	// CSC 2
	mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptBlack);
	
	
	// Mixer/Keyer
	mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptBlack);
}

	
//--------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceXPointCapture ()
{
	bool 				b8K 			= NTV2_IS_QUAD_QUAD_FORMAT(mFb1VideoFormat);
	//bool				bHdmiIn			= mDs.bInHdmi && mHasHdmiIn;
	//bool				bHdmiInRGB		= bHdmiIn == true && mDs.hdmiIn[0]->cs == NTV2_ColorSpaceModeRgb;
	//bool				bInRGB			= bHdmiInRGB || mDs.bInSdiRgb;
	//bool				bFb1RGB			= IsRGBFormat(mFb1Format);
	
	if (!b8K)
		return Class4kServices::SetDeviceXPointCapture();
		
		
	// Frame Buffer 1
	mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
	
	// Frame Buffer 2
	mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptSDIIn2);
	
	// Frame Buffer 3
	mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptSDIIn3);

	// Frame Buffer 4
	mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptSDIIn4);
	
	
	mCard->EnableChannel(NTV2_CHANNEL1);
	mCard->EnableChannel(NTV2_CHANNEL2);
	mCard->EnableChannel(NTV2_CHANNEL3);
	mCard->EnableChannel(NTV2_CHANNEL4);
	
	
	// SDI Out
	mCard->Connect (NTV2_XptSDIOut1Input, 		NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut1InputDS2, 	NTV2_XptBlack);
	
	// SDI Out 2
	mCard->Connect (NTV2_XptSDIOut2Input, 		NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut2InputDS2, 	NTV2_XptBlack);
	
	
	// SDI Out 3
	mCard->Connect (NTV2_XptSDIOut3Input, 		NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut3InputDS2, 	NTV2_XptBlack);


	// SDI Out 4
	mCard->Connect (NTV2_XptSDIOut4Input, 		NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut4InputDS2, 	NTV2_XptBlack);
	
	
	// HDMI Out
	mCard->Connect (NTV2_XptHDMIOutInput, 		NTV2_XptSDIIn1);
	mCard->Connect (NTV2_XptHDMIOutQ2Input, 	NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ3Input, 	NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ4Input, 	NTV2_XptBlack);

	
	// CSC 1
	mCard->Connect (NTV2_XptCSC1VidInput, 		NTV2_XptBlack);


	// CSC 2
	mCard->Connect (NTV2_XptCSC2VidInput, 		NTV2_XptBlack);
	
	
	// Mixer/Keyer
	mCard->Connect (NTV2_XptMixer1FGVidInput, 	NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1FGKeyInput, 	NTV2_XptBlack);
}


//--------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceMiscRegisters ()
{
	bool b8K = NTV2_IS_QUAD_QUAD_FORMAT(mFb1VideoFormat);
	if (!b8K)
		Class4kServices::SetDeviceMiscRegisters();
}

