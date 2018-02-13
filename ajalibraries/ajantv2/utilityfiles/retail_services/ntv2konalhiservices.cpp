//
//  ntv2konalhiservices.cpp
//
//  Copyright (c) 2011 AJA Video, Inc. All rights reserved.
//

#include "ntv2konalhiservices.h"


//-------------------------------------------------------------------------------------------------------
//	class KonaLHiServices
//-------------------------------------------------------------------------------------------------------

KonaLHiServices::KonaLHiServices()
{
}


//-------------------------------------------------------------------------------------------------------
//	GetSelectedInputVideoFormat
//	Note:	Determine input video format based on input select and fbVideoFormat
//			which currently is videoformat of ch1-framebuffer
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat KonaLHiServices::GetSelectedInputVideoFormat(
											NTV2VideoFormat fbVideoFormat,
											NTV2SDIInputFormatSelect* inputFormatSelect)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	if (inputFormatSelect)
		*inputFormatSelect = NTV2_YUVSelect;
	
	// Figure out what our input format is based on what is selected
	switch (mVirtualInputSelect)
	{
		case NTV2_DualLinkInputSelect:
		case NTV2_Input1Select:
			inputFormat = GetSdiInVideoFormat(0, fbVideoFormat);
			break;
		case NTV2_Input2Select:
			inputFormat = mCard->GetHDMIInputVideoFormat();
			break;
		case NTV2_Input3Select:
			inputFormat = mCard->GetAnalogInputVideoFormat();
			break;
		default:
			break;
	}
	inputFormat = GetTransportCompatibleFormat(inputFormat, fbVideoFormat);
	
	return inputFormat;
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void KonaLHiServices::SetDeviceXPointPlayback (GeneralFrameFormat format)
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback(format);
	
	bool bDualStreamFB = IsVideoFormatB(mFb1VideoFormat);

	NTV2FrameBufferFormat fb1Format;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fb1Format);
	bool bCh1RGB = IsFrameBufferFormatRGB(fb1Format);
		
	NTV2FrameBufferFormat fb2Format;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL2, &fb2Format);
	bool bCh2RGB = IsFrameBufferFormatRGB(fb2Format);
		
	bool bDSKGraphicMode = (mDSKMode == NTV2_DSKModeGraphicOverMatte || mDSKMode == NTV2_DSKModeGraphicOverVideoIn || mDSKMode == NTV2_DSKModeGraphicOverFB);
	bool bDSKOn = (mDSKMode == NTV2_DSKModeFBOverMatte || mDSKMode == NTV2_DSKModeFBOverVideoIn || (bCh2RGB && bDSKGraphicMode));
						
	// don't let the DSK be ON if we're in Mac Desktop mode
	if (!mStreamingAppPID && mDefaultVideoOutMode == kDefaultModeDesktop)
		bDSKOn = false;
	
	bool bPassThrough = !mStreamingAppPID && (mDefaultVideoOutMode == kDefaultModeVideoIn);
	
	// Set (Compression Module) input (reg 136, bits 31-24) 
	// Select frame buffer YUV (0x08)
	mCard->Connect (NTV2_XptCompressionModInput, NTV2_XptFrameBuffer1YUV);
	
	// Set (Up/Down Converter) module input (reg 136, bits 23-16)
	if (!bDSKOn)
	{
		if (format == FORMAT_COMPRESSED)
		{
			// Select compression module (0x07)
			mCard->Connect (NTV2_XptConversionModInput, NTV2_XptCompressionModule);
		}
		else if (format == FORMAT_RGB)
		{
			// Select color space converter (0x05)
			mCard->Connect (NTV2_XptConversionModInput, NTV2_XptCSC1VidYUV);
		}
		else
		{
			// Select frame buffer YUV (0x08)
			mCard->Connect (NTV2_XptConversionModInput, NTV2_XptFrameBuffer1YUV);
		}
	}
	else
	{
		// Select Mixer out (0x12)
		mCard->Connect (NTV2_XptConversionModInput, NTV2_XptMixer1VidYUV);
	}
	
	
	// Set (CSC1) color space converter input (reg 136, bits 15-8)
	if (format == FORMAT_RGB || bDSKOn)
	{
		// CSC?LUT is needed for RGB->YUV conversion - Select LUT RGB out (0x84)
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}
	else if (format == FORMAT_COMPRESSED)
	{
		// Select conversion module (0x07)
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptCompressionModule);
	}
	else
	{
		//if (RGBOutput == Primary)						// We will implement this later
		if (true)
		{
			// Select framebuffer YUV (0x08)
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV);
		}
		else
		{
			// Select Up/Down converter (0x06)
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptConversionModule);
		}
	}
	
	// Set (CSC2) color space converter input (reg 140, bits 16-23)
	if ( bDSKOn && bDSKGraphicMode && bCh2RGB )
	{
		// CSC2?LUT2 is needed for RGB->YUV conversion - Select LUT RGB out (0x8D)
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptLUT2RGB);
	}
	else
	{
		// Select black
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptBlack);
	}
	
	
	// Set (LUT1) input (reg 136, bits 7-0)
	if (format == FORMAT_RGB || bDSKOn )
	{
		/*
		if (bDSKOn && bDSKGraphicMode )
		{
			// Select frame buffer 2 RGB (0x8F)
			mCard->Connect (NTV2_XptLUT1Input, NTV2_XptFrameBuffer2RGB);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
		}
		else
		*/
		{
			// Select frame buffer 1 RGB (0x88)
			mCard->Connect (NTV2_XptLUT1Input, NTV2_XptFrameBuffer1RGB);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
		}
	}
	else
	{
		// Select color space converter (0x85)
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	
	
	// Set (LUT2) input (reg 140, bits 8-15)
	if ( bDSKOn && bDSKGraphicMode && bCh2RGB )
	{
		// Select frame buffer 1 RGB (0x8F)
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptFrameBuffer2RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	else
	{
		// Select black (0x0)
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptBlack);
	}
	

	// Set (Dual Link Out) input (reg 137, bits 31-24)
	// Select frame sync 2 RGB (0x8a)
	mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptFrameSync2RGB);

	// Set (Frame Sync 2) input (reg 137, bits 23-16)
	if (bDualStreamFB)
	{
		if (bPassThrough)
		{
			// Select input 2 (0x02)
			mCard->Connect (NTV2_XptFrameSync2Input, NTV2_XptSDIIn2);
		}
		else
		{
			// Select frame buffer YUV (0x0F)
			mCard->Connect (NTV2_XptFrameSync2Input, NTV2_XptFrameBuffer2YUV);
		}
	}
	else if (format == FORMAT_RGB || bDSKOn)
	{
		if (mGammaMode == NTV2_GammaNone)
		{
			// Support custom LUT for RGB frame buffer -> Dual Link
			// Select LUT1 (0x84)
			mCard->Connect (NTV2_XptFrameSync2Input, NTV2_XptLUT1RGB);
		}
		else
		{
			// Select frame buffer RGB (0x88)
			mCard->Connect (NTV2_XptFrameSync2Input, NTV2_XptFrameBuffer1RGB);
		}
	}
	else
	{
		// Select LUT (0x84)
		mCard->Connect (NTV2_XptFrameSync2Input, NTV2_XptLUT1RGB);
	}

	// Set (Frame Sync 1) input (reg 137, bits 15-8)
	if (bDualStreamFB)
	{
		if (bPassThrough)
		{
			// Select input 2 (0x01)
			mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptSDIIn1);
		}
		else
		{
			// Select frame buffer YUV (0x0F)
			mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptFrameBuffer1YUV);
		}
	}
	else if (!bDSKOn)
	{
		if (format == FORMAT_RGB)
		{
			// Select color space converter (0x05)
			mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptCSC1VidYUV);
		}
		else if (format == FORMAT_COMPRESSED)
		{
			// Select compression module (0x07)
			mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptCompressionModule);
		}
		else
		{
			// Select frame buffer YUV (0x08)
			mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptFrameBuffer1YUV);
		}
	}
	else
	{
		// Select Mixer out (0x12)
		mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptMixer1VidYUV);
	}


	// Set (Frame Buffer 1) input (reg 137, bits 7-0)
	if  (format == FORMAT_COMPRESSED)
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
		else if (mVirtualInputSelect == NTV2_Input3Select)
		{
			// Select input 3 (0x16)
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptAnalogIn);
		}
		else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
		{
			// Select dual link (0x83)
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptDuallinkIn1);
		}
	}
	

	// 3G Link B - NOTE follows SDI output 1 for now
	// Set SDI output 2 input (reg 138, bits 23-16) 
	if (bDualStreamFB)
	{
		// select frame sync 2 output (0x0A)
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameSync2YUV);
	}
	else if ( (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
			  || (   (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
			      && (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
				  && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select Up/Down converter out (0x06)
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptConversionModule);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
	{
		// Select dual link out (0x0b)
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut1);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)
	{
		// if the frame buffer format is one of the 8-bit RGBA types, select the "alpha" extractor output
		if (bCh1RGB)
		{
			// Select "alpha" out (0x0c)
			//mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptAlphaOut);
			mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptCSC1KeyYUV);
		}
		else
		{
			// if we're NOT in an RGBA format, just pick the Primary video output (0x09)
			mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameSync1YUV);
		}
	}


	// 3G Link A
	// Set SDI output 1 input (reg 138, bits 15-8)
	if (bDualStreamFB)
	{
		// select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameSync1YUV);
	}
	else if (   (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)			// if our output is "Primary"
		     || (   (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
				 && (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
				 && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select Up/Down converter out (0x06)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptConversionModule);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
	{		
		// Select dual link out (0x0b)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)
	{		
		// Select frame sync 1 ("Primary") output (0x09)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameSync1YUV);
	}
	
	
	// Set HDMI output input (reg 141, bits 16-23)
	if (   (mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		|| (   (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
			&& (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)
	{
		// Select Up/Down converter out (0x06)
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptConversionModule);
	}
		
		
	// Set (Analog Out) input (reg 138, bits 7-0)
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
		// Select Up/Down converter out (0x06)
		mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptConversionModule);
	}
	
	
	//
	// Set Mixer/Keyer Inputs
	//
	
	// default video proc mode
	mCard->WriteRegister (kRegVidProc1Control, 0, ~kRegMaskVidProcLimiting);			// FG = Full, BG = Full, VidProc = FG On
	
	// The background video/key depends on the DSK mode
	bool bDSKNeedsInputRef = false;				// Assume we're genlocking to display reference source
	int audioLoopbackMode = 0;					// Assume playback mode. Will be set to '1' if we're in Loopback ("E-E") mode
	int bFb1Disable = 0;						// Assume Channel 1 is NOT disabled
	int bFb2Disable = 1;						// Assume Channel 2 IS disabled
	bool bNoKey = false;						// Assume we DO have a foreground key

	if (bDSKOn)
	{	
		switch (mDSKMode)
		{
			case NTV2_DSKModeFBOverMatte:
						// Foreground
						if (bCh1RGB)
						{
							// The foreground video/key comes from the CSC 1 output (0x05/0x0E)
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
						}
						else if (format == FORMAT_COMPRESSED)
						{
							// The foreground video/key needs to come from the Compression Module output (0x07 - key input is "don't care")
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCompressionModule);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCompressionModule);
							bNoKey = true;
						}
						else
						{
							// The foreground video/key comes from the Channel 1 frame buffer (0x08 - key input is "don't care")
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptFrameBuffer1YUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptFrameBuffer1YUV);
							bNoKey = true;
						}

						// Background  (note: FB1 out is used for sync - it will be replaced by matte video
						mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptFrameBuffer1YUV);
						mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptFrameBuffer1YUV);
						mCard->WriteRegister (kRegVidProc1Control, 1, kRegMaskVidProcBGMatteEnable, kRegShiftVidProcBGMatteEnable);
						break;
			
			
			case NTV2_DSKModeFBOverVideoIn:
						// Foreground
						if (bCh1RGB)
						{
							// The foreground video/key comes from the CSC 1 output (0x05/0x0E)
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
						}
						else if (format == FORMAT_COMPRESSED)
						{
							// The foreground video/key needs to come from the Compression Module output (0x07 - key input is "don't care")
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCompressionModule);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCompressionModule);
							bNoKey = true;
						}
						else
						{
							// The foreground video/key comes from the Channel 1 frame buffer (0x08 - key input is "don't care")
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptFrameBuffer1YUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptFrameBuffer1YUV);
							bNoKey = true;
						}

							// Background is video in
						if (mVirtualInputSelect == NTV2_Input1Select)
						{
							// Select input 1 (0x01)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
						}
						else if (mVirtualInputSelect == NTV2_Input2Select)
						{
							// Select input 2 (0x17)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptHDMIIn);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptHDMIIn);
						}
						else if (mVirtualInputSelect == NTV2_Input3Select)
						{
							// Select input 3 (0x16)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptAnalogIn);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptAnalogIn);
						}
						else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
						{
							// Select dual link (0x83)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptDuallinkIn1);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptDuallinkIn1);
						}
						
						// in "Frame Buffer over VideoIn" mode, where should the audio come from?
						if (mDSKAudioMode == NTV2_DSKAudioBackground)
							audioLoopbackMode = 1;							// set audio to "input loopthru" (aka "E-E") mode
						
						bDSKNeedsInputRef = true;		// genlock to input video
						break;


			case NTV2_DSKModeGraphicOverMatte:
						// Foreground
						if (bCh2RGB)
						{
							// The foreground video/key comes from the CSC 1 output (0x05/0x0E)
							//mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
							//mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
							// The foreground video/key comes from the CSC 2 output (0x10/0x11)
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
						}
						else
						{
							// The foreground video/key comes from the Channel 2 frame buffer (0x0F - key input is "don't care")
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptFrameBuffer2YUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptFrameBuffer2YUV);
							bNoKey = true;
						}
						
						// Background (note: FB1 is used for sync - it will be replaced by matte video
						mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptFrameBuffer1YUV);
						mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptFrameBuffer1YUV);
						bFb1Disable = 1;			// disable Ch 1
						bFb2Disable = 0;			// enable Ch 2
						mCard->WriteRegister (kRegVidProc1Control, 1, kRegMaskVidProcBGMatteEnable, kRegShiftVidProcBGMatteEnable);
						break;
			
			
			case NTV2_DSKModeGraphicOverVideoIn:
						// Foreground
						if (bCh2RGB)
						{
							// The foreground video/key comes from the CSC 1 output (0x05/0x0E)
							//mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
							//mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
							// The foreground video/key comes from the CSC 2 output (0x10/0x11)
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
						}
						else
						{
							// The foreground video/key comes from the Channel 2 frame buffer (0x0F - key input is "don't care")
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptFrameBuffer2YUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptFrameBuffer2YUV);
							bNoKey = true;
						}
						
						// Background is video in
						if (mVirtualInputSelect == NTV2_Input1Select)
						{
							// Select input 1 (0x01)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
						}
						else if (mVirtualInputSelect == NTV2_Input2Select)
						{
							// Select input 2 (0x17)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptHDMIIn);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptHDMIIn);
						}
						else if (mVirtualInputSelect == NTV2_Input3Select)
						{
							// Select input 2 (0x16)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptAnalogIn);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptAnalogIn);
						}
						else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
						{
							// Select dual link (0x83)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptDuallinkIn1);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptDuallinkIn1);
						}
						
						bFb1Disable = 1;			// disable Ch 1
						bFb2Disable = 0;			// enable Ch 2
						
						// in "Frame Buffer over VideoIn" mode, where should the audio come from?
						if (mDSKAudioMode == NTV2_DSKAudioBackground)
							audioLoopbackMode = 1;							// set audio to "input loopthru" (aka "E-E") mode
						
						bDSKNeedsInputRef = true;		// genlock to input video
						break;
			
			
			case NTV2_DSKModeGraphicOverFB:			
						// Foreground
						if (bCh2RGB)
						{
							// The foreground video/key comes from the CSC 1 output (0x05/0x0E)
							//mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
							//mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
							// The foreground video/key comes from the CSC 2 output (0x10/0x11)
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
						}
						else
						{
							// The foreground video/key comes from the Channel 2 frame buffer (0x0F - key input is "don't care")
							mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptFrameBuffer2YUV);
							mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptFrameBuffer2YUV);
							bNoKey = true;
						}
						
						// Background is Frame Buffer 1 (with or without compression)
						if (format == FORMAT_COMPRESSED)
						{
							// Select compression module (0x07)
							mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptCompressionModule);
							mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptCompressionModule);
						}
						else
						{
							if (bCh1RGB)
							{
								// Select CSC1 (0x05/0x0E)
								mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptCSC1VidYUV);
								mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptCSC1KeyYUV);
							}
							else
							{
								// Select frame buffer YUV (0x08)
								mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptFrameBuffer1YUV);
								mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptFrameBuffer1YUV);
							}
						}
						bFb2Disable = 0;			// enable Ch 2
						break;
						
					default:
						break;
		}

		if (bNoKey)
			mCard->WriteRegister (kRegVidProc1Control, 0, kRegMaskVidProcFGControl, kRegShiftVidProcFGControl);		// FG = Full Raster
		else
		{
			if (mDSKForegroundMode == NTV2_DSKForegroundShaped)
				mCard->WriteRegister (kRegVidProc1Control, 1, kRegMaskVidProcFGControl, kRegShiftVidProcFGControl);		// FG = Shaped, BG = Full, VidProc = FG On
			else
				mCard->WriteRegister (kRegVidProc1Control, 2, kRegMaskVidProcFGControl, kRegShiftVidProcFGControl);		// FG = Unshaped, BG = Full, VidProc = FG On
		}
		
		// write foreground fade
		mCard->WriteRegister (kRegVidProc1Control, 1, kRegMaskVidProcMode, kRegShiftVidProcMode);					// Mix mode - "mix"
		mCard->WriteRegister (kRegMixer1Coefficient, 0x10000 - mDSKForegroundFade); // Mix value 0x10000 (opaque) - 0x00000 (transparent)
	}
	
	// never disable channels if in dual link 372 mode
	if (bDualStreamFB)
		bFb1Disable = bFb2Disable = false; 
		
	// set Channel disable mode (0 = enable, 1 = disable)
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);		
}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void KonaLHiServices::SetDeviceXPointCapture (GeneralFrameFormat format)
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture(format);

	NTV2VideoFormat		inputFormat = NTV2_FORMAT_UNKNOWN;
	NTV2CrosspointID	inputSelectPrimary = NTV2_XptBlack; 
	NTV2CrosspointID	inputSelectSecondary = NTV2_XptBlack; 
	bool				bDualStreamFB = IsVideoFormatB(mFb1VideoFormat);
														
	// Figure out what our input format is based on what is selected 
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat);

	
	// This is done all over so do it once here so we have the value
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		// Select input 1 (0x01)
		inputSelectPrimary = NTV2_XptSDIIn1;
		inputSelectSecondary = NTV2_XptSDIIn2;
	}
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		// Select input 2 (0x17)
		inputSelectPrimary = NTV2_XptHDMIIn;
		inputSelectSecondary = NTV2_XptBlack;
	}
	else if (mVirtualInputSelect == NTV2_Input3Select)
	{
		// Select input 3 (0x16)
		inputSelectPrimary = NTV2_XptAnalogIn;
		inputSelectSecondary = NTV2_XptBlack;
	}
	else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
	{
		// Select color space converter (0x05)
		inputSelectPrimary = NTV2_XptCSC1VidYUV;
		inputSelectSecondary = NTV2_XptBlack;
	}


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

	
	// Set Up/Down converter module input (reg 136, bits 23-16)
	mCard->Connect (NTV2_XptConversionModInput, inputSelectPrimary);


	// Set color space converter input (reg 136, bits 15-8)
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			// Select input 1 (0x01)
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptSDIIn1);
		}
		else
		{
			// Select Up/Down converter (0x06)
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptConversionModule);
		}
	}
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			// Select input 2 (0x17)
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptHDMIIn);
		}
		else
		{
			// Select Up/Down converter (0x06)
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptConversionModule);
		}
	}
	else if (mVirtualInputSelect == NTV2_Input3Select)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			// Select input 3 (0x16)
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptAnalogIn);
		}
		else
		{
			// Select Up/Down converter (0x06)
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptConversionModule);
		}
	}
	else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
	{
		// Select LUT RGB out (0x84)
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}
	

	// Set LUT input (reg 136, bits 7-0)
	if ((mVirtualInputSelect == NTV2_Input1Select) ||
		(mVirtualInputSelect == NTV2_Input2Select) ||
		(mVirtualInputSelect == NTV2_Input3Select))
	{
		// Select color space converter (0x85)
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
	{
		// Select dual link in (0x83)
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptDuallinkIn1);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}


	// Set DualLink output input (reg 137, bits 31-24) Note: same logic as Frame Buffer RGB input
	if (inputFormat == mFb1VideoFormat)
	{
		// Input is Primary format
		if ((mVirtualInputSelect == NTV2_Input1Select) ||
			(mVirtualInputSelect == NTV2_Input2Select) ||
			(mVirtualInputSelect == NTV2_Input3Select))
		{
			// Select LUT RGB out (0x84)
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
		}
		else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
		{
			// Select dual link in (0x83)
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptDuallinkIn1);		// fixed
			//mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);			// this is a temp hack until we get the above-mentioned crosspoint
		}
	}
	else
	{
		// Input is Secondary format
		// NOTE: This is the same logic as above but we can't do the dual link case because we would
		// need two LUT's to convert RGB to YUB then back again.
		if ((mVirtualInputSelect == NTV2_Input1Select) ||
			(mVirtualInputSelect == NTV2_Input2Select) ||
			(mVirtualInputSelect == NTV2_Input3Select))
		{
			// Select LUT RGB out (0x84)
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
		}
		else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
		{
			// Select dual link in (0x83) - this is wrong since the Dual Link input can't be converted from Secondary to Primary
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptDuallinkIn1);		// fixed
			//mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);			// this is a temp hack until we get the above-mentioned crosspoint
		}
	}
	


	// Set frame sync 2 input (reg 137, bits 23-16)
	if (bDualStreamFB)
	{
		// Select input 2 (0x02)
		mCard->Connect (NTV2_XptFrameSync2Input, NTV2_XptSDIIn2);
	}
	else if (inputFormat == mFb1VideoFormat && !ISO_CONVERT_FMT(inputFormat))
	{
		// Select Up/Down converter (0x06)
		mCard->Connect (NTV2_XptFrameSync2Input, NTV2_XptConversionModule);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameSync2Input, inputSelectPrimary);
	}


	// Set frame sync 1 input (reg 137, bits 15-8)
	if (bDualStreamFB)
	{
		// Select input 1 (0x01)
		mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptSDIIn1);
	}
	else if (inputFormat == mFb1VideoFormat && !ISO_CONVERT_FMT(inputFormat))
	{
		mCard->Connect (NTV2_XptFrameSync1Input, inputSelectPrimary);
	}
	else
	{
		// Select Up/Down converter (0x06)
		mCard->Connect (NTV2_XptFrameSync1Input, NTV2_XptConversionModule);
	}


	// Set frame buffer 1 input (reg 137, bits 7-0)
	if ( bDualStreamFB )
	{
		// Select input 1 (0x01)
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
	}
	else if  (format == FORMAT_RGB)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			if ((mVirtualInputSelect == NTV2_Input1Select) ||
				(mVirtualInputSelect == NTV2_Input2Select) ||
				(mVirtualInputSelect == NTV2_Input3Select))
			{
				// Select LUT RGB out (0x84)
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);
			}
			else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
			{
				// Select dual link in (0x83)
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptDuallinkIn1);
			}
		}
		else
		{
			// NOTE: This is the same logic as above but we can't do the dual link case because we would
			// need two LUT's to convert RGB to YUB then back again.
			if ((mVirtualInputSelect == NTV2_Input1Select) ||
				(mVirtualInputSelect == NTV2_Input2Select) ||
				(mVirtualInputSelect == NTV2_Input3Select))
			{
				// Select LUT RGB out (0x84)
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);
			}
			else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
			{
				// Select dual link in (0x83)
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptDuallinkIn1);
			}
		}
	}
	else if (format == FORMAT_COMPRESSED)
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
	
	// Set frame buffer 2 input (reg 140, bits 7-0)
	if (bDualStreamFB)
	{
		// Select input 2 (0x02)
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptSDIIn2);
	}
	else 
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	}

	// 3G Link A - follows SDI 1 for now
	// Set SDI output 2 input (reg 138, bits 23-16)
	if (bDualStreamFB)
	{
		// Select frame sync 2 output (0x0A)
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameSync2YUV);
	}
	else if ( (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		      || (   (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
			      && (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
				  && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameSync1YUV);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select frame sync 2 output (0x0a)
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptFrameSync2YUV);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
	{
		// Select dual link out (0x0b)
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut1);
	}


	// Set SDI output 1 input (reg 138, bits 15-8)
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
	else if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
	{		
		// Select dual link out (0x0b)
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
	}
	
	// Set HDMI output input (reg 141, bits 16-23)
	if (   (mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		|| (   (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
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
	else if (mVirtualAnalogOutputSelect == NTV2_DualLinkOutputSelect)
	{
		// Select dual link out (0x0b)
		mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptDuallinkOut1);
	}
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void KonaLHiServices::SetDeviceMiscRegisters (NTV2Mode mode)
{	
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters(mode);

	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	NTV2FrameBufferFormat   primaryPixelFormat;
	
	mCard->GetStandard(&primaryStandard);
	mCard->GetFrameGeometry(&primaryGeometry);
	mCard->GetFrameBufferFormat (NTV2_CHANNEL1, &primaryPixelFormat);
	
	NTV2Standard			secondaryStandard = GetNTV2StandardFromVideoFormat (mVirtualSecondaryFormatSelect);
	NTV2FrameGeometry		secondaryGeometry = GetNTV2FrameGeometryFromVideoFormat (mVirtualSecondaryFormatSelect);
	
	NTV2VideoFormat			inputFormat = NTV2_FORMAT_UNKNOWN;
	
	NTV2FrameRate			primaryFrameRate = GetNTV2FrameRateFromVideoFormat (mFb1VideoFormat);
	NTV2FrameRate			secondardFrameRate = GetNTV2FrameRateFromVideoFormat (mVirtualSecondaryFormatSelect);
	bool					bDualStreamFB = IsVideoFormatB(mFb1VideoFormat);
	
	
	// FrameBuffer 2: make sure formats matches FB1 for DualLink B mode (SMPTE 372)
	if (bDualStreamFB)
	{
		NTV2FrameBufferFormat fbFormat;
		mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormat);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, fbFormat);
	}

	// HDMI output - initialization sequence
	if (mHDMIStartupCountDown > 0)
	{
		// start initialization
		if (mHDMIStartupCountDown == kHDMIStartupPhase0)
			mCard->WriteRegister (kRegHDMIOutControl, 0x0, 0x0F000000, 24);
			
		else if (mHDMIStartupCountDown == kHDMIStartupPhase1)
			mCard->WriteRegister (kRegHDMIOutControl, 0xC, 0x0F000000, 24);
		
		else if (mHDMIStartupCountDown == kHDMIStartupPhase2)
			mCard->WriteRegister (kRegHDMIOutControl, 0xD, 0x0F000000, 24);
			
		else if (mHDMIStartupCountDown == kHDMIStartupPhase3)
			mCard->WriteRegister (kRegHDMIOutControl, 0xC, 0x0F000000, 24);
		
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
					mCard->ReadRegister (kRegHDMIInputStatus, &detectedProtocol, kLHIRegMaskHDMIOutputEDIDDVI, kLHIRegShiftHDMIOutputEDIDDVI);
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
	if (mVANCMode && Is8BitFrameBufferFormat(primaryPixelFormat) )
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	
	// Figure out what our input format is based on what is selected
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat);
	
	
	//
	// Analog-In locking state machine (from Hell)
	//

	//if (mVirtualInputSelect == NTV2_Input3Select)
	{
		// analog locked
		if (mCard->GetAnalogInputAutotimed())
		{
			// if locked and stabilized - look for changes in the ADC mode if something has changed
			if (mADCStabilizeCount == 0)
			{
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
			
				// Only write this register if something changes - compare user/detected values against current control values
				if (	(detectedStandard != ctrlAnalogInStandard) ||
						(detectedFrameRate != ctrlAnalogInFrameRate) ||
						(mVirtualAnalogInType != ctrlAnalogInType) ||
						(virtualAnalogInBlackLevel != ctrlAnalogInBlackLevel) )
				{
					//DebugLog("program ADC AnalogInFrameRate = %d, AnalogInStandard = %d\n", inputFrameRate, mVirtualAnalogInStandard);

					// reset stabilization count
					mADCStabilizeCount = kADCStabilizeCount;
					
					// update ADC output control processor
					SetADCRegisters(detectedStandard, mVirtualAnalogInType, virtualAnalogInBlackLevel, detectedFrameRate);
				}
			}
			else
				mADCStabilizeCount--;
		}
		
		// not locked
		else
		{
			if (mADCStabilizeCount == 0)
			{
				// reset stabilization count
				mADCStabilizeCount = kADCStabilizeCount;
			
				// stabilized, but not locked - not smack it on the head, make it try the next format
				// note we rotate through one geom/framerate pair for each applicable analog type 
				switch(mVirtualAnalogInType)
				{
					// composite
					case NTV2_AnlgComposite:
						mADCLockScanTestFormat = (mADCLockScanTestFormat + 1) % 2;
						switch (mADCLockScanTestFormat)
						{
							default: 
							case 0: SetVideoADCMode (NTV2_480iADCCompositeUS);	break;
							case 1: SetVideoADCMode (NTV2_576iADCComposite);		break;
						}
						break;
					
					// s-video
					case NTV2_AnlgSVideo:
						mADCLockScanTestFormat = (mADCLockScanTestFormat + 1) % 2;
						switch (mADCLockScanTestFormat)
						{
							default: 
							case 0: SetVideoADCMode (NTV2_480iADCSVideoUS);		break;
							case 1: SetVideoADCMode (NTV2_576iADCSVideo);			break;
						}
						break;
					
					// component
					default:	
						mADCLockScanTestFormat = (mADCLockScanTestFormat + 1) % 7;
						switch (mADCLockScanTestFormat)
						{
							default: 
							case 0: SetVideoADCMode (NTV2_480iADCComponentBeta);	break;
							case 1: SetVideoADCMode (NTV2_576iADCComponentSMPTE);	break;
							case 2: SetVideoADCMode (NTV2_720p_60);				break;
							case 3: SetVideoADCMode (NTV2_1080i_30);				break;
							case 4: SetVideoADCMode (NTV2_720p_50);				break;
							case 5: SetVideoADCMode (NTV2_1080i_25);				break;
							case 6: SetVideoADCMode (NTV2_1080pSF24);				break;
						}
						break;
				}
			}
			else
				mADCStabilizeCount--;
		}
	}
	//else
		//mADCStabilizeCount = kADCStabilizeCount;
	
	
	//
	// Analog-Out
	//
	
	// Control video DAC mode (reg 128)
	// Note: the hardware takes a video "hit" every time we write a value to this register - whether or not
	//       the new value is different. So we're going to filter it ourselves and ONLY write a value if it
	//       has actually changed. 
	NTV2LHIVideoDACMode curr2Mode, new2Mode;
	NTV2Standard curr2Standard, new2Standard;
	
	// get current value
	mCard->GetLHIVideoDACMode (&curr2Mode);	
	mCard->GetLHIVideoDACStandard (&curr2Standard);
	
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
	
	
	// Set SDI out 1 control video standard (reg 129, bits 2-0)
	if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select secondary standard
		mCard->SetSDIOut2Kx1080Enable( NTV2_CHANNEL1, secondaryGeometry == NTV2_FG_2048x1080 );
		mCard->SetSDIOutputStandard(NTV2_CHANNEL1, secondaryStandard);
		mCard->SetSDIOut3GEnable(NTV2_CHANNEL1, IsVideoFormatA(mVirtualSecondaryFormatSelect));
	}
	else
	{
		// Select primary standard
		mCard->SetSDIOut2Kx1080Enable( NTV2_CHANNEL1, primaryGeometry == NTV2_FG_2048x1080 );
		mCard->SetSDIOutputStandard(NTV2_CHANNEL1, primaryStandard);
		mCard->SetSDIOut3GEnable(NTV2_CHANNEL1, IsVideoFormatA(mFb1VideoFormat));
	}


	// Set SDI out 2 control video standard (reg 130, bits 2-0)
	if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select secondary standard
		mCard->SetSDIOut2Kx1080Enable( NTV2_CHANNEL2, secondaryGeometry == NTV2_FG_2048x1080 );
		mCard->SetSDIOutputStandard(NTV2_CHANNEL2, secondaryStandard);
	}
	else
	{
		// Select primary standard
		mCard->SetSDIOut2Kx1080Enable( NTV2_CHANNEL2, primaryGeometry == NTV2_FG_2048x1080 );
		mCard->SetSDIOutputStandard(NTV2_CHANNEL2, primaryStandard);
	}


	// Set conversion control (reg 131)
	// Set converter output standard (bits 14-12)
	if (mode == NTV2_MODE_DISPLAY)								// playback mode: converter is always on output,
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
	if (mode == NTV2_MODE_DISPLAY)								// playback mode - converter always on output
		mCard->SetConverterInStandard(primaryStandard);				// so converter input = primary format

	else														// capture mode: converter may be on input or output
	{
		if (inputFormat == mFb1VideoFormat)					// no input conversion needed - put converter on output
			mCard->SetConverterInStandard(primaryStandard);
		else
			mCard->SetConverterInStandard(secondaryStandard);		// input conversion needed - need converter on input
	}
}
