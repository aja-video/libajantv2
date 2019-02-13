//
//  ntv2ttapservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2ttapservices.h"


//-------------------------------------------------------------------------------------------------------
//	class TTapServices
//-------------------------------------------------------------------------------------------------------

TTapServices::TTapServices()
{
}

//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void TTapServices::SetDeviceXPointPlayback ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback();
	bool bFb1Compressed = IsFormatCompressed(mFb1Format);
	
	// get video format
	//NTV2VideoFormat frameBufferVideoFormat = GetFrameBufferVideoFormat();
	
	// Compression Module
	mCard->Connect (NTV2_XptCompressionModInput, NTV2_XptFrameBuffer1YUV);


	// Frame Sync
	NTV2CrosspointID frameSyncYUV;
	if (bFb1Compressed)
	{
		frameSyncYUV = NTV2_XptCompressionModule;
	}
	else
	{
		frameSyncYUV = NTV2_XptFrameBuffer1YUV;
	}
	
	// frame
#if 0  // believe this is wrong, check with Stan PSM
	if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect &&
		mVirtualSecondaryFormatSelect != frameBufferVideoFormat)
	{
		mCard->SetPulldownMode(NTV2_CHANNEL1, true);
	}
	else
	{
		mCard->SetPulldownMode(NTV2_CHANNEL1, false);
	}
#endif
    
    mCard->SetPulldownMode(NTV2_CHANNEL1, false);


	// SDI Out
	mCard->Connect (NTV2_XptSDIOut1Input, frameSyncYUV);
	
	
	// HDMI Out
	mCard->Connect (NTV2_XptHDMIOutInput, frameSyncYUV);
	
	// set Channel disable mode (0 = enable, 1 = disable)
	int bFb1Disable = 0;
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void TTapServices::SetDeviceXPointCapture ()
{
	// no input for T-Tap, should not be here
	mCard->SetDefaultVideoOutMode(kDefaultModeTestPattern);
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void TTapServices::SetDeviceMiscRegisters ()
{	
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();

	NTV2FrameRate			primaryFrameRate = GetNTV2FrameRateFromVideoFormat (mFb1VideoFormat);
	
	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	mCard->GetStandard(primaryStandard);
	mCard->GetFrameGeometry(primaryGeometry);
	mCard->SetHDMIOutVideoStandard (primaryStandard);
	mCard->SetHDMIOutVideoFPS (primaryFrameRate);
	
	// set color-space bit-depth
	switch (mDs.hdmiOutColorSpace)
	{
		case kHDMIOutCSCYCbCr8bit:
			mCard->SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
			mCard->SetHDMIOutBitDepth(NTV2_HDMI8Bit);
			break;
	
		case kHDMIOutCSCYCbCr10bit:
			mCard->SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
			mCard->SetHDMIOutBitDepth (NTV2_HDMI10Bit);
			break;
			
		case kHDMIOutCSCRGB10bit:
			mCard->SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceRGB);
			mCard->SetHDMIOutBitDepth (NTV2_HDMI10Bit);
			break;
			
		default:
		case kHDMIOutCSCRGB8bit:
			mCard->SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceRGB);
			mCard->SetHDMIOutBitDepth (NTV2_HDMI8Bit);
			break;
	}
	
	// HDMI Out Protocol mode
	switch (mDs.hdmiOutProtocol)
	{
		default:
		case kHDMIOutProtocolHDMI:
			mCard->WriteRegister (kRegHDMIOutControl, NTV2_HDMIProtocolHDMI, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
			break;
			
		case kHDMIOutProtocolDVI:
			mCard->WriteRegister (kRegHDMIOutControl, NTV2_HDMIProtocolDVI, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
			break;
	}

	
	// special case - VANC 8bit pixel shift support
	if (mVANCMode && Is8BitFrameBufferFormat(mFb1Format) )
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
}
