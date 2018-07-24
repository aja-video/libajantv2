//
//  ntv2ioexpressservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2ioexpressservices.h"


//-------------------------------------------------------------------------------------------------------
//	class IoExpressServices
//-------------------------------------------------------------------------------------------------------

IoExpressServices::IoExpressServices()
{
}


//-------------------------------------------------------------------------------------------------------
//	GetSelectedInputVideoFormat
//	Note:	Determine input video format based on input select and fbVideoFormat
//			which currently is videoformat of ch1-framebuffer
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat IoExpressServices::GetSelectedInputVideoFormat(
											NTV2VideoFormat fbVideoFormat,
											NTV2ColorSpaceMode* inputColorSpace)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	if (inputColorSpace)
		*inputColorSpace = NTV2_ColorSpaceModeYCbCr;
	
	// Figure out what our input format is based on what is selected 
	switch (mVirtualInputSelect)
	{
		case NTV2_Input1Select:
			inputFormat = GetSdiInVideoFormat(0, fbVideoFormat);
			break;
		case NTV2_Input2Select:
			inputFormat = mCard->GetHDMIInputVideoFormat();
			break;
		default: break;
	}
	inputFormat = GetTransportCompatibleFormat(inputFormat, fbVideoFormat);
	
	return inputFormat;
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void IoExpressServices::SetDeviceXPointPlayback ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback();
	bool bFb1Compressed = IsFormatCompressed(mFb1Format);
	
	// Turn off LTC loopback during playback
	mCard->WriteRegister (kRegFS1ReferenceSelect, 0, kRegMaskLTCLoopback, kRegShiftLTCLoopback);
			
	// Set (Compression Module) input (reg 136, bits 31-24) 
	// Select frame buffer YUV (0x08)
	mCard->Connect (NTV2_XptCompressionModInput, NTV2_XptFrameBuffer1YUV);
	
	
	// Set (Up/Down Converter) module input (reg 136, bits 23-16)
	if (bFb1Compressed)
	{
		// Select compression module (0x07)
		mCard->Connect (NTV2_XptConversionModInput, NTV2_XptCompressionModule);
	}
	else
	{
		// Select frame buffer YUV (0x08)
		mCard->Connect (NTV2_XptConversionModInput, NTV2_XptFrameBuffer1YUV);
	}


	// Set (Frame Sync 1) input (reg 137, bits 15-8)
	if (bFb1Compressed)
	{
		// Select compression module (0x07)
		mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptCompressionModule);
	}
	else
	{
		// Select frame buffer YUV (0x08)
		mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptFrameBuffer1YUV);
	}


	// Set (Frame Buffer 1) input (reg 137, bits 7-0)
	if  (bFb1Compressed)
	{
		// Select compression module out (0x07)
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCompressionModule);
	}
	else
	{
		if (mVirtualInputSelect == NTV2_Input1Select)
		{
			// Select input 1 (0x01)
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
		}
		else if (mVirtualInputSelect == NTV2_Input2Select)
		{
			// Select input 2 (0x17)
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptHDMIIn);
		}
	}


	// Set SDI output 1 input (reg 138, bits 15-8)
	if (	(mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect) ||			// if our output is "Primary"
			((mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect) &&		// or if "Secondary" AND Secondary == Primary and not SD format
			 (mVirtualSecondaryFormatSelect == mFb1VideoFormat)))
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select Up/Down converter out (0x06)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptConversionModule);
	}
	else
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameSync1YUV);
	}
	
	
	// Set HDMI output input (reg 141, bits 16-23)
	if (	(mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect) ||			// if our output is "Primary"
			((mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect) &&		// or if "Secondary" AND Secondary == Primary and not SD format
			 (mVirtualSecondaryFormatSelect == mFb1VideoFormat)))
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)
	{
		// Select Up/Down converter out (0x06)
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptConversionModule);
	}
	else
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameSync1YUV);
	}
		
		
	// Set (Analog Out) input (reg 138, bits 7-0)
	if (	(mVirtualAnalogOutputSelect == NTV2_PrimaryOutputSelect) ||			// if our output is "Primary"
			((mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect) &&		// or if "Secondary" AND Secondary == Primary and not SD format
			 (mVirtualSecondaryFormatSelect == mFb1VideoFormat)))
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)
	{
		// Select Up/Down converter out (0x06)
		mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptConversionModule);
	}
	else
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptFrameSync1YUV);
	}
	
	
	// default video proc mode
	mCard->WriteRegister (kRegVidProc1Control, 0, ~kRegMaskVidProcLimiting, kRegShiftVidProcLimiting);		// FG = Full, BG = Full, VidProc = FG On
	
	int bFb1Disable = 0;							// Assume Channel 1 is NOT disabled
	int bFb2Disable = 1;							// Assume Channel 2 IS disabled
		
	// set Channel disable mode (0 = enable, 1 = disable)
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);		
}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void IoExpressServices::SetDeviceXPointCapture()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();

	NTV2VideoFormat				inputFormat = NTV2_FORMAT_UNKNOWN;
	NTV2CrosspointID			inputSelectPrimary = NTV2_XptSDIIn1;
	//NTV2CrosspointID			inputSelectSecondary = NTV2_XptSDIIn2;
	bool 						bFb1Compressed = IsFormatCompressed(mFb1Format);
	
	// if user select LTC port as input - loop it back during capture
	uint32_t enabled = false;
	mCard->ReadRegister(kRegFS1ReferenceSelect, enabled, kFS1RefMaskLTCOnRefInSelect, kRegShiftLTCOnRefInSelect);
	mCard->WriteRegister (kRegFS1ReferenceSelect, enabled, kRegMaskLTCLoopback, kRegShiftLTCLoopback);
	
	// Figure out what our input format is based on what is selected
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat);
	
	
	// This is done all over so do it once here so we have the value
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		// Select input 1 (0x01)
		inputSelectPrimary = NTV2_XptSDIIn1;
		//inputSelectSecondary = NTV2_XptSDIIn2;
	}
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		// Select input 2 (0x17)
		inputSelectPrimary = NTV2_XptHDMIIn;
		//inputSelectSecondary = NTV2_XptBlack;
	}
	
	// Set Up/Down converter module input (reg 136, bits 23-16)
	mCard->Connect (NTV2_XptConversionModInput, inputSelectPrimary);

	// Set compression module input (reg 136, bits 31-24)
	if (inputFormat == mFb1VideoFormat)
	{
		mCard->Connect (NTV2_XptCompressionModInput, inputSelectPrimary);
	}
	else
	{
		// Select Up/Down converter (0x06)
		mCard->Connect (NTV2_XptCompressionModInput, NTV2_XptConversionModule);
	}
	
	// Set frame sync 2 input (reg 137, bits 23-16)
	if (inputFormat == mFb1VideoFormat)
	{
		// Select Up/Down converter (0x06)
		mCard->Connect (NTV2_XptFrameSync2Input, NTV2_XptConversionModule);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameSync2Input, inputSelectPrimary);
	}

	// Set frame sync 1 input (reg 137, bits 15-8)
	if (inputFormat == mFb1VideoFormat)
	{
		mCard->Connect (NTV2_XptFrameSync1Input, inputSelectPrimary);
	}
	else
	{
		// Select Up/Down converter (0x06)
		mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptConversionModule);
	}


	// Set frame buffer 1 input (reg 137, bits 7-0)
	if (bFb1Compressed)
	{
		// Select compression out (0x07)
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCompressionModule);
	}
	else 
	{
		if ( (inputFormat == mFb1VideoFormat) &&	// formats are same
			 !(ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect) && ISO_CONVERT_FMT(inputFormat)) )	 // not SD to SD
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, inputSelectPrimary);
		}
		else
		{
			// Select Up/Down converter (0x06)
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptConversionModule);
		}
	}

	// Set SDI 1 input (reg 138, bits 15-8)
	if (   (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		|| (   (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
		    && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select frame sync 2 output (0x0a)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameSync2YUV);
	}

	// Set HDMI output input (reg 141, bits 16-23)
	if (   (mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect)					// if our output is "Primary"
		|| (   (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)			// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
			&& (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)
	{
		// Select Up/Down converter out (0x06)
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameSync2YUV);
	}
	
	// Set analog output input (reg 138, bits 7-0)
	if (   (mVirtualAnalogOutputSelect == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		|| (   (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)			// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
		    && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)
	{
		// Select frame sync 2 output (0x0a)
		mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptFrameSync2YUV);
	}
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void IoExpressServices::SetDeviceMiscRegisters ()
{
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();

	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	
	mCard->GetStandard(primaryStandard);
	mCard->GetFrameGeometry(primaryGeometry);

	NTV2Standard			secondaryStandard = GetNTV2StandardFromVideoFormat (mVirtualSecondaryFormatSelect);
	//NTV2FrameGeometry		secondaryGeometry = GetNTV2FrameGeometryFromVideoFormat (mVirtualSecondaryFormatSelect);
	
	NTV2VideoFormat			inputFormat = NTV2_FORMAT_UNKNOWN;
	
	NTV2FrameRate			primaryFrameRate = GetNTV2FrameRateFromVideoFormat (mFb1VideoFormat);
	NTV2FrameRate			secondardFrameRate = GetNTV2FrameRateFromVideoFormat (mVirtualSecondaryFormatSelect);
	
	
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
		if (mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect)
		{
			mCard->SetHDMIOutVideoStandard (primaryStandard);
			mCard->SetHDMIOutVideoFPS (primaryFrameRate);
		}
		else if (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)
		{
			mCard->SetHDMIOutVideoStandard (secondaryStandard);
			mCard->SetHDMIOutVideoFPS (secondardFrameRate);
		}
		
		// HDMI out colorspace auto-detect status 
		mHDMIOutColorSpaceModeStatus = mHDMIOutColorSpaceModeCtrl;
		if (mHDMIOutColorSpaceModeCtrl == kHDMIOutCSCAutoDetect)
		{
			NTV2HDMIBitDepth bitDepth = NTV2_HDMI10Bit;
			NTV2LHIHDMIColorSpace colorSpace = NTV2_LHIHDMIColorSpaceYCbCr;
			
			mCard->GetHDMIOutDownstreamColorSpace (colorSpace);
			mCard->GetHDMIOutDownstreamBitDepth (bitDepth);
			
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
					mCard->ReadRegister (kRegHDMIInputStatus, detectedProtocol, kLHIRegMaskHDMIOutputEDIDDVI);
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
	

	// Figure out what our input format is based on what is selected
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat);
	
	//
	// Analog-In locking state machine
	//

	NTV2Standard			ctrlAnalogInStandard;
	NTV2AnalogType		ctrlAnalogInType;							// Composite, S-Video, Cmpt beta, Cmpt smpte
	NTV2AnalogBlackLevel	ctrlAnalogInBlackLevel;						// US 0, Japan 7.5
	NTV2FrameRate			ctrlAnalogInFrameRate;
	
	// Read current ADC mode control register (ADC reg 82), then calculate video-std, analog-type, blacklevel, frame-rate base on ADC mode
	// This is the ADC output processor
	GetADCRegisters(&ctrlAnalogInStandard, &ctrlAnalogInType, &ctrlAnalogInBlackLevel, &ctrlAnalogInFrameRate);
	
	// Read the standard / framerate that was detected on ADC input processor (reg 81)
	NTV2Standard			detectedStandard = GetAnalogInputVideoStandard ();
	NTV2FrameRate			detectedFrameRate = GetAnalogInputVideoFrameRate ();
	NTV2VideoFormat			detectedVideoFormat = GetNTV2VideoFormat (detectedFrameRate, detectedStandard, false, 0, false);
	
	// if input is PAL then force the IRE to be 7.5 regardless of virtual setting
	NTV2AnalogBlackLevel virtualAnalogInBlackLevel = mVirtualAnalogInBlackLevel;
	if (detectedVideoFormat == NTV2_FORMAT_625_5000)
		virtualAnalogInBlackLevel = NTV2_Black75IRE;
	
	if (mADCStabilizeCount == 0)
	{
		// Only write this register if something changes - compare user/detected values against current control values
		if (	(detectedStandard != ctrlAnalogInStandard) ||
				(detectedFrameRate != ctrlAnalogInFrameRate) ||
				(mVirtualAnalogInType != ctrlAnalogInType) ||
				(virtualAnalogInBlackLevel != ctrlAnalogInBlackLevel) )
		{
			//DebugLog("program ADC AnalogInFrameRate = %d, AnalogInStandard = %d\n", inputFrameRate, mVirtualAnalogInStandard);

			// insure we wait at least 10 frames to allow the ADC chip to stabilize
			mADCStabilizeCount = 10;
			
			// update ADC output control processor
			SetADCRegisters(detectedStandard, mVirtualAnalogInType, virtualAnalogInBlackLevel, detectedFrameRate);
		}
	}
	else
		mADCStabilizeCount--;

	
	// Set analog output control video DAC mode (reg 128)
	// Note: the hardware takes a video "hit" every time we write a value to this register - whether or not
	//       the new value is different. So we're going to filter it ourselves and ONLY write a value if it
	//       has actually changed.
	NTV2LHIVideoDACMode curr2Mode, new2Mode;
	NTV2Standard curr2Standard, new2Standard;
	
	// get current value
	mCard->GetLHIVideoDACMode (curr2Mode);	
	mCard->GetLHIVideoDACStandard (curr2Standard);
	
	if (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)	
	{
		// Select DAC mode from secondary format
		new2Mode = GetLHIVideoDACMode (mVirtualSecondaryFormatSelect, mVirtualAnalogOutputType, mVirtualAnalogOutBlackLevel);
		new2Standard = secondaryStandard;
	}
	else
	{
		// Select DAC mode from framebufferformat
		new2Mode = GetLHIVideoDACMode (mFb1VideoFormat, mVirtualAnalogOutputType, mVirtualAnalogOutBlackLevel);
		new2Standard = primaryStandard;
	}
	
	// write it only if the new value is different
	if (curr2Mode != new2Mode)
		mCard->SetLHIVideoDACMode (new2Mode);
	if (curr2Standard != new2Standard)
		mCard->SetLHIVideoDACStandard (new2Standard);

	// Set conversion control (reg 131)
	// Set converter output standard (bits 14-12)
	if (mFb1Mode == NTV2_MODE_DISPLAY)								// playback mode: converter is always on output,
	{
		// set pulldown bit
		mCard->SetConverterPulldown( (ULWord)IsPulldownConverterMode(mFb1VideoFormat,mVirtualSecondaryFormatSelect) );
		mCard->SetConverterOutStandard(secondaryStandard);			// so converter output = secondary format
	}
	else														// capture mode: converter may be on input or output
	{
		if (inputFormat == mFb1VideoFormat)
		{
			// no input conversion needed - put converter on output
			mCard->SetConverterPulldown( (ULWord)IsPulldownConverterMode(mFb1VideoFormat,mVirtualSecondaryFormatSelect) );
			mCard->SetConverterOutStandard(secondaryStandard);
		}
		else
		{
			// input conversion needed - need converter on input
			mCard->SetConverterPulldown( (ULWord)IsPulldownConverterMode(mVirtualSecondaryFormatSelect, mFb1VideoFormat) );
			mCard->SetConverterOutStandard(primaryStandard);
		}
	}

	// Set converter output standard (bits 2-0)
	if (mFb1Mode == NTV2_MODE_DISPLAY)								// playback mode - converter always on output
		mCard->SetConverterInStandard(primaryStandard);				// so converter input = primary format

	else														// capture mode: converter may be on input or output
	{
		if (inputFormat == mFb1VideoFormat)					// no input conversion needed - put converter on output
			mCard->SetConverterInStandard(primaryStandard);
		else
			mCard->SetConverterInStandard(secondaryStandard);		// input conversion needed - need converter on input
	}
}
