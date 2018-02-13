//
//  ntv2ttapservices.cpp
//
//  Copyright (c) 2011 AJA Video, Inc. All rights reserved.
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

	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	
	mCard->GetStandard(&primaryStandard);
	mCard->GetFrameGeometry(&primaryGeometry);

	NTV2FrameRate			primaryFrameRate = GetNTV2FrameRateFromVideoFormat (mFb1VideoFormat);
	
	
	// HDMI output - initialization sequence
	if (mHDMIStartupCountDown > 0)
	{
		// start initialization
		if (mHDMIStartupCountDown == kHDMIStartupPhase0)
			mCard->WriteRegister (kRegHDMIOutControl, 0x0, 0x0F000000);
			
		else if (mHDMIStartupCountDown == kHDMIStartupPhase1)
			mCard->WriteRegister (kRegHDMIOutControl, 0xC, 0x0F000000);
		
		else if (mHDMIStartupCountDown == kHDMIStartupPhase2)
			mCard->WriteRegister (kRegHDMIOutControl, 0xD, 0x0F000000);
			
		else if (mHDMIStartupCountDown == kHDMIStartupPhase3)
			mCard->WriteRegister (kRegHDMIOutControl, 0xC, 0x0F000000);
		
		mHDMIStartupCountDown--;
	}
	else
	{
		// HDMI out fully initialized - now do setup
	
		// Set HDMI Out (reg 125)
		mCard->SetHDMIOutVideoStandard (primaryStandard);
		mCard->SetHDMIOutVideoFPS (primaryFrameRate);
		
		// HDMI out colorspace auto-detect status 
		mHDMIOutColorSpaceModeStatus = mHDMIOutColorSpaceModeCtrl;
		if (mHDMIOutColorSpaceModeCtrl == kHDMIOutCSCAutoDetect)
		{
			NTV2HDMIBitDepth bitDepth = NTV2_HDMI10Bit;
			NTV2LHIHDMIColorSpace colorSpace = NTV2_LHIHDMIColorSpaceYCbCr;
			
			mCard->GetHDMIOutDownstreamColorSpace (&colorSpace);
			mCard->GetHDMIOutDownstreamBitDepth (&bitDepth);
			
			if (colorSpace == NTV2_LHIHDMIColorSpaceYCbCr)
				mHDMIOutColorSpaceModeStatus = kHDMIOutCSCYCbCr10bit;
			
			else if (bitDepth == NTV2_HDMI10Bit)
				mHDMIOutColorSpaceModeStatus = kHDMIOutCSCRGB10bit;
			
			else
				mHDMIOutColorSpaceModeStatus = kHDMIOutCSCRGB8bit;
		}
		
		// set color space bits as specified
		switch (mHDMIOutColorSpaceModeStatus)
		{
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
		switch (mHDMIOutProtocolMode)
		{
			default:
			case kHDMIOutProtocolAutoDetect:
				{
					ULWord detectedProtocol;
					mCard->ReadRegister (kRegHDMIInputStatus, &detectedProtocol, kLHIRegMaskHDMIOutputEDIDDVI);
					mCard->WriteRegister (kRegHDMIOutControl, detectedProtocol, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);				
				}
				break;
			
			case kHDMIOutProtocolHDMI:
				mCard->WriteRegister (kRegHDMIOutControl, NTV2_HDMIProtocolHDMI, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
				break;
			
			case kHDMIOutProtocolDVI:
				mCard->WriteRegister (kRegHDMIOutControl, NTV2_HDMIProtocolDVI, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
				break;
		}
	}
	
	// special case - VANC 8bit pixel shift support
	if (mVANCMode && Is8BitFrameBufferFormat(mFb1Format) )
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	
	// Set SDI out control video standard (reg 129, bits 2-0)
	mCard->SetSDIOut2Kx1080Enable( NTV2_CHANNEL1, primaryGeometry == NTV2_FG_2048x1080 );
	mCard->SetSDIOutputStandard(NTV2_CHANNEL1, primaryStandard);
	
	// VPID insertion
	{
		const bool				kRGBOut				= false;
		const bool				kNot48Bit			= false;
		const bool				kNot3Gb				= false;
		ULWord					vpidOut1a(0);
	
		// enable overwrite
		mCard->WriteRegister(kRegSDIOut1Control,	1, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite);
		
		// enable VPID write
		mCard->WriteRegister(kRegSDIOut1Control, 1, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable);
		
		// generate vpid register value
        // get video format
		SetVPIDData(vpidOut1a, mFb1VideoFormat, kRGBOut, kNot48Bit, kNot3Gb, false, VPIDChannel_1);
		
		// write VPID for SDI 1
		mCard->WriteRegister(kRegSDIOut1VPIDA, vpidOut1a);
	}
}
