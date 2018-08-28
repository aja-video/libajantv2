//
//  ntv2ioxtservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2ioxtservices.h"


//-------------------------------------------------------------------------------------------------------
//	class IoXTServices
//-------------------------------------------------------------------------------------------------------

IoXTServices::IoXTServices()
{
}


//-------------------------------------------------------------------------------------------------------
//	GetSelectedInputVideoFormat
//	Note:	Determine input video format based on input select and fbVideoFormat
//			which currently is videoformat of ch1-framebuffer
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat IoXTServices::GetSelectedInputVideoFormat(
											NTV2VideoFormat fbVideoFormat,
											NTV2ColorSpaceMode* inputColorSpace)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	if (inputColorSpace)
		*inputColorSpace = NTV2_ColorSpaceModeYCbCr;
	
	// Figure out what our input format is based on what is selected 
	if ((mVirtualInputSelect == NTV2_Input1Select) || (mVirtualInputSelect == NTV2_Input2xDLHDSelect))
	{
		inputFormat = GetSdiInVideoFormat(0, fbVideoFormat);
		if (inputColorSpace)
			*inputColorSpace = mSDIInput1ColorSpace;
	}
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		inputFormat = GetSdiInVideoFormat(1, fbVideoFormat);
		if (inputColorSpace)
			*inputColorSpace = mSDIInput2ColorSpace;
	}
	else if (mVirtualInputSelect == NTV2_Input3Select)
	{
		inputFormat = mCard->GetHDMIInputVideoFormat();
		if (inputColorSpace)
			*inputColorSpace = NTV2_ColorSpaceModeYCbCr;
	}
	inputFormat = GetTransportCompatibleFormat(inputFormat, fbVideoFormat);
	
	return inputFormat;
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void IoXTServices::SetDeviceXPointPlayback ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback();
	
	bool bFb1RGB = IsRGBFormat(mFb1Format);
	bool bFb2RGB = IsRGBFormat(mFb2Format);
	bool bFb1Compressed = IsFormatCompressed(mFb1Format);
		
	bool bDSKGraphicMode = (mDSKMode == NTV2_DSKModeGraphicOverMatte || mDSKMode == NTV2_DSKModeGraphicOverVideoIn || mDSKMode == NTV2_DSKModeGraphicOverFB);
	bool bDSKOn = (mDSKMode == NTV2_DSKModeFBOverMatte || mDSKMode == NTV2_DSKModeFBOverVideoIn || (bFb2RGB && bDSKGraphicMode));
		
		// don't let the DSK be ON if we're in Mac Desktop mode
	if (!mStreamingAppPID && mDefaultVideoOutMode == kDefaultModeDesktop)
		bDSKOn = false;
		
	bool bStereoOut			= mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect;
	bool b2FbLevelBHfr		= IsVideoFormatB(mFb1VideoFormat);
	bool b3GbOut	= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);			// use 2 SDI wires, or just 1 3Gb
	bool bEanbleConverter	= false;
	
	// make sure frame DualLink B mode (SMPTE 372), Stereo
	if (b2FbLevelBHfr || bStereoOut)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
		bFb2RGB = bFb1RGB;
	}

	// Frame Sync 1
	NTV2CrosspointID frameSync1YUV;
	if (bStereoOut || b2FbLevelBHfr)
	{
		if (bFb1RGB)
		{
			frameSync1YUV = NTV2_XptCSC1VidYUV;
		}
		else
		{
			frameSync1YUV = NTV2_XptFrameBuffer1YUV;
		}
	}
	else if (bDSKOn)
	{
		frameSync1YUV = NTV2_XptMixer1VidYUV;
	}
	else 
	{
		if (bFb1RGB)
		{
			frameSync1YUV = NTV2_XptCSC1VidYUV;
		}
		else if (bFb1Compressed)
		{
			frameSync1YUV = NTV2_XptCompressionModule;
		}
		else
		{
			frameSync1YUV = NTV2_XptFrameBuffer1YUV;
		}
	}
	
	
	// Frame Sync 2
	NTV2CrosspointID frameSync2YUV = NTV2_XptBlack;
	NTV2CrosspointID frameSync2RGB = NTV2_XptBlack;
	if (bStereoOut || b2FbLevelBHfr)
	{
		if (bFb1RGB)
		{
			frameSync2YUV = NTV2_XptCSC2VidYUV;
		}
		else
		{
			frameSync2YUV = NTV2_XptFrameBuffer2YUV;
		}
	}
	else if (bDSKOn)
	{
		frameSync2YUV = NTV2_XptMixer1KeyYUV;
	}
	else if (bFb1RGB)
	{
		frameSync2RGB = NTV2_XptLUT1RGB;
	}
	else
	{
		frameSync2RGB = NTV2_XptLUT1RGB;
	}
	
	
	// Compression Module
	mCard->Connect (NTV2_XptCompressionModInput, NTV2_XptFrameBuffer1YUV);
	

	// Up/Down Converter
	if (!bDSKOn)
	{
		if (bFb1Compressed)
		{
			mCard->Connect (NTV2_XptConversionModInput, NTV2_XptCompressionModule);
		}
		else if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptConversionModInput, NTV2_XptCSC1VidYUV);
		}
		else
		{
			mCard->Connect (NTV2_XptConversionModInput, NTV2_XptFrameBuffer1YUV);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptConversionModInput, NTV2_XptMixer1VidYUV);
	}
	

	// CSC 1
	if (bFb1RGB || bDSKOn)
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}
	else if (bFb1Compressed)
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptCompressionModule);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV);
	}
	

	// CSC 2
	if ( bFb2RGB )
	{
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptLUT2RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2YUV);
	}
	

	// LUT 1
	if (bFb1RGB || bDSKOn)
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptFrameBuffer1RGB);
	
		// if RGB-to-RGB apply LUT converter
		if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)
		{
			mCard->SetColorCorrectionOutputBank (  NTV2_CHANNEL1,
											mRGB10Range == NTV2_RGB10RangeFull ? 
											kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);	
		}
		else
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);
	}
	
	
	// LUT 2
	if ( bFb2RGB )
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptFrameBuffer2RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);
	}
	else
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptCSC2VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_YUV2RGB);
	}
	

	// Dual Link Out
	mCard->Connect (NTV2_XptDualLinkOut1Input, frameSync2RGB);


	// Frame Buffer 1
	if  (bFb1Compressed)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCompressionModule);
	}
	else
	{
		if (mVirtualInputSelect == NTV2_Input1Select)
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
		}
		else if (mVirtualInputSelect == NTV2_Input2Select)
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn2);
		}
		else if (mVirtualInputSelect == NTV2_Input3Select)
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptHDMIIn1);
		}
		else if (mVirtualInputSelect == NTV2_Input2xDLHDSelect)
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptDuallinkIn1);
		}
	}
	
	
	// SDI Out 1
	if (b2FbLevelBHfr || bStereoOut)
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbOut ? frameSync2YUV : NTV2_XptBlack);
	}
	else if (   (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)		// if our output is "Primary"
		     || (   (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)	// or if "Secondary" AND Secondary == Primary and not SD format
				 && (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
				 && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)			// Secondary
	{
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptConversionModule);
		bEanbleConverter = true;
	}
	else if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)			// RGB Out
	{		
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbOut ? NTV2_XptDuallinkOut1DS2 : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)				// Video+Key
	{		
		if (bDSKOn)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbOut ? frameSync2YUV : NTV2_XptBlack);
		}
		else if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbOut ? NTV2_XptCSC1KeyYUV : NTV2_XptBlack);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
		}
	}
	
	
	// SDI Out 2
	if (b2FbLevelBHfr || bStereoOut)
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, frameSync2YUV);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut2Input, frameSync2YUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
		}
	}
	else if ( (mVirtualDigitalOutput2Select == NTV2_PrimaryOutputSelect)			// if our output is "Primary"
			  || (   (mVirtualDigitalOutput2Select == NTV2_SecondaryOutputSelect)	// or if "Secondary" AND Secondary == Primary and not SD format
			      && (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
				  && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		mCard->Connect (NTV2_XptSDIOut2Input, frameSync1YUV);
	}
	else if (mVirtualDigitalOutput2Select == NTV2_SecondaryOutputSelect)			// Secondary
	{
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptConversionModule);
		bEanbleConverter = true;
	}
	else if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)			// RGB Out
	{
		mCard->Connect (NTV2_XptSDIOut2Input, b3GbOut ? NTV2_XptDuallinkOut1 : NTV2_XptDuallinkOut1DS2);
		mCard->Connect (NTV2_XptSDIOut2InputDS2, b3GbOut ? NTV2_XptDuallinkOut1DS2 : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput2Select == NTV2_VideoPlusKeySelect)				// Video+Key
	{
		if (bDSKOn)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, b3GbOut ? frameSync1YUV : frameSync2YUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, b3GbOut ? frameSync2YUV : NTV2_XptBlack);
		}
		else if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, b3GbOut ? NTV2_XptCSC1VidYUV : NTV2_XptCSC1KeyYUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, b3GbOut ? NTV2_XptCSC1KeyYUV : NTV2_XptBlack);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut2Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
		}
	}
	

	// HDMI Out
	if (   (mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect)					// if our output is "Primary"
		|| (   (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)			// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
			&& (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		mCard->Connect (NTV2_XptHDMIOutInput, frameSync1YUV);
	}
	else if (bDSKOn)
	{
		mCard->Connect (NTV2_XptHDMIOutInput, frameSync1YUV);
	}
	else if (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)				// Secondary
	{
		mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptConversionModule);
		bEanbleConverter = true;
	}
	

	// Analog Out
	if (   (mVirtualAnalogOutputSelect == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		|| (   (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)			// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
			&& (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		mCard->Connect (NTV2_XptAnalogOutInput, frameSync1YUV);
	}
	else if (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)
	{
		mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptConversionModule);
		bEanbleConverter = true;
	}
	
	
	//
	// Mixer/Keyer
	//

	// default video proc mode
	mCard->WriteRegister (kRegVidProc1Control, 0, ~kRegMaskVidProcLimiting);		// FG = Full, BG = Full, VidProc = FG On
	
	// The background video/key depends on the DSK mode
	int bFb1Disable = 0;						// Assume Channel 1 is NOT disabled
	int bFb2Disable = 1;						// Assume Channel 2 IS disabled
	bool bNoKey = false;						// Assume we DO have a foreground key

	if (bDSKOn)
	{	
		switch (mDSKMode)
		{
			case NTV2_DSKModeFBOverMatte:
				// Foreground
				if (bFb1RGB)
				{
					// The foreground video/key comes from the CSC 1 output (0x05/0x0E)
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
				}
				else if (bFb1Compressed)
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
				if (bFb1RGB)
				{
					// The foreground video/key comes from the CSC 1 output (0x05/0x0E)
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
				}
				else if (bFb1Compressed)
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
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
				}
				else if (mVirtualInputSelect == NTV2_Input2Select)
				{
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn2);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn2);
				}
				else if (mVirtualInputSelect == NTV2_Input3Select)
				{
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptHDMIIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptHDMIIn1);
				}
				else if (mVirtualInputSelect == NTV2_Input2xDLHDSelect)
				{
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptDuallinkIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptDuallinkIn1);
				}
				break;

			case NTV2_DSKModeGraphicOverMatte:
				// Foreground
				if (bFb2RGB)
				{
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
				if (bFb2RGB)
				{
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
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
				}
				else if (mVirtualInputSelect == NTV2_Input2Select)
				{
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn2);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn2);
				}
				else if (mVirtualInputSelect == NTV2_Input3Select)
				{
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptHDMIIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptHDMIIn1);
				}
				else if (mVirtualInputSelect == NTV2_Input2xDLHDSelect)
				{
					// Select dual link (0x83)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptDuallinkIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptDuallinkIn1);
				}
				
				bFb1Disable = 1;			// disable Ch 1
				bFb2Disable = 0;			// enable Ch 2
				break;
			
			case NTV2_DSKModeGraphicOverFB:			
				// Foreground
				if (bFb2RGB)
				{
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
				if (bFb1Compressed)
				{
					// Select compression module (0x07)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptCompressionModule);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptCompressionModule);
				}
				else
				{
					if (bFb1RGB)
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

	// Make sure both channels are enable for stereo, dual-link B
	if (b2FbLevelBHfr || bStereoOut)
	{
		bFb1Disable = bFb2Disable = 0; 
	}
	// set Channel disable mode (0 = enable, 1 = disable)
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);		
	
	
	// UFC enable
	// should be disabled unless it is in use. It can consume significant memory/dma bandwidth
	mCard->SetEnableConverter(bEanbleConverter);
}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void IoXTServices::SetDeviceXPointCapture ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();

	NTV2RGBRangeMode			frambBufferRange	= (mRGB10Range == NTV2_RGB10RangeSMPTE) ? NTV2_RGBRangeSMPTE : NTV2_RGBRangeFull; 
	bool 						bFb1RGB 			= IsRGBFormat(mFb1Format);
	bool 						bFb1Compressed 		= IsFormatCompressed(mFb1Format);
	bool						b2FbLevelBHfr		= IsVideoFormatB(mFb1VideoFormat);
	bool						b3GbOut				= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	bool						bEanbleConverter	= false;
	int							bFb1Disable			= 0;		// Assume Channel 1 is NOT disabled by default
	int							bFb2Disable			= 1;		// Assume Channel 2 IS disabled by default
													  
	NTV2CrosspointID			inputXptYUV1		= NTV2_XptBlack;		// Input source selected single stream
	NTV2CrosspointID			inputXptYUV2		= NTV2_XptBlack;		// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
	NTV2VideoFormat				inputFormat			= mFb1VideoFormat;		// Input source selected format
	NTV2ColorSpaceMode			inputColorSpace		= NTV2_ColorSpaceModeYCbCr;		// Input format select (YUV, RGB, etc)
	
	
	// Figure out what our input format is based on what is selected 
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat, &inputColorSpace);
	
	
	// make sure frame buffer formats match for DualLink B mode (SMPTE 372)
	if (b2FbLevelBHfr)
	{
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
	}
	
	
	// input 1 select
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		inputXptYUV1 = NTV2_XptSDIIn1;
		inputXptYUV2 = NTV2_XptSDIIn1DS2;
	}
	// input 2 select
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		inputXptYUV1 = NTV2_XptSDIIn2;
		inputXptYUV2 = NTV2_XptSDIIn2DS2;
	}
	// input 3 select
	else if (mVirtualInputSelect == NTV2_Input3Select)
	{
		inputXptYUV1 = NTV2_XptHDMIIn1;
		inputXptYUV2 = NTV2_XptBlack;
	}
	// dual link select
	else if (mVirtualInputSelect == NTV2_Input2xDLHDSelect)
	{
		inputXptYUV1 = NTV2_XptSDIIn1;
		inputXptYUV2 = NTV2_XptSDIIn2;
	}
	
	
	// Frame Sync 1
	NTV2CrosspointID frameSync1YUV;
	if (b2FbLevelBHfr)
	{
		frameSync1YUV = inputXptYUV1;
	}
	else if (inputFormat == mFb1VideoFormat && !ISO_CONVERT_FMT(inputFormat))
	{
		frameSync1YUV = inputXptYUV1;
	}
	else
	{
		frameSync1YUV = NTV2_XptConversionModule;
		bEanbleConverter = true;
	}
	
	
	// Frame Sync 2
	NTV2CrosspointID frameSync2YUV;
	if (b2FbLevelBHfr)
	{
		frameSync2YUV = inputXptYUV2;
	}
	else if (inputFormat == mFb1VideoFormat && !ISO_CONVERT_FMT(inputFormat))
	{
		frameSync2YUV = NTV2_XptConversionModule;
		bEanbleConverter = true;
	}
	else
	{
		frameSync2YUV = inputXptYUV1;
	}
	

	// Dual Link In 1
	if (inputColorSpace == NTV2_ColorSpaceModeRgb)
	{
		mCard->Connect (NTV2_XptDualLinkIn1Input, inputXptYUV1);
		mCard->Connect (NTV2_XptDualLinkIn1DSInput, inputXptYUV2);
		
		inputXptYUV1 = NTV2_XptCSC1VidYUV;
		inputXptYUV2 = NTV2_XptBlack;
	}
	else
	{
		mCard->Connect (NTV2_XptDualLinkIn1Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptDualLinkIn1DSInput, NTV2_XptBlack);
	}
	
	
	// Dual Link In 2 - not yet used
	mCard->Connect (NTV2_XptDualLinkIn2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptDualLinkIn2DSInput, NTV2_XptBlack);



	// Compression Module
	if (inputFormat == mFb1VideoFormat)
	{
		mCard->Connect (NTV2_XptCompressionModInput, inputXptYUV1);
	}
	else
	{
		mCard->Connect (NTV2_XptCompressionModInput, NTV2_XptConversionModule);
		bEanbleConverter = true;
	}

	
	// Up/Down Converter
	mCard->Connect (NTV2_XptConversionModInput, inputXptYUV1);


	// CSC 1
	if (inputColorSpace != NTV2_ColorSpaceModeRgb)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			mCard->Connect (NTV2_XptCSC1VidInput, inputXptYUV1);
		}
		else
		{
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptConversionModule);
			bEanbleConverter = true;
		}
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}
	

	// LUT 1
	if (inputColorSpace != NTV2_ColorSpaceModeRgb)
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);	
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptDuallinkIn1);
		
		// if RGB-to-RGB apply LUT converter
		if (bFb1RGB)
		{
			mCard->SetColorCorrectionOutputBank (	NTV2_CHANNEL1,
											mSDIInput1RGBRange == NTV2_RGBRangeFull ? 
											kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
		}
		else 
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);
		}
	}
	
	
	// LUT 2 
	// provides SMPTE <-> Full conversion
	if (inputColorSpace == NTV2_ColorSpaceModeRgb)
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptDuallinkIn1);
		mCard->SetColorCorrectionOutputBank (	NTV2_CHANNEL2,	
										mSDIInput1RGBRange == NTV2_RGBRangeFull ? 
										kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
	}


	// Dual Link Out 1
	if (inputFormat == mFb1VideoFormat)
	{
		// Input is NOT secondary
	
		if (inputColorSpace != NTV2_ColorSpaceModeRgb)
		{
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
		}
		else
		{
			if (mSDIInput1RGBRange == mSDIOutput1RGBRange && mLUTType != NTV2_LUTCustom)
			{
				mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptDuallinkIn1);		// no range change
			}
			else
			{
				mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT2RGB);			// range change needed
			}
		}
	}
	else
	{
		// Input is Secondary format
		// NOTE: This is the same logic as above but we can't do the dual link case because we would
		// need two LUT's to convert RGB to YUB then back again.
		if (inputColorSpace != NTV2_ColorSpaceModeRgb)
		{
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptDuallinkIn1);		// fixed
		}
	}
	
	
	// Dual Link Out 2 - not used for inputs
	

	// Frame Buffer 1
	if (b2FbLevelBHfr)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, inputXptYUV1);
	}
	else if (bFb1RGB)
	{
		if (inputColorSpace == NTV2_ColorSpaceModeRgb)
		{
			if (mSDIInput1RGBRange == frambBufferRange && mLUTType != NTV2_LUTCustom)
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptDuallinkIn1);		// no range change
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);				// range change needed
			}
		}
		else
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);					// CSC converted
		}
	}
	else if (bFb1Compressed)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCompressionModule);
	}
	else 
	{
		if ( (inputFormat == mFb1VideoFormat) &&	// formats are same
			 !(ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect) && ISO_CONVERT_FMT(inputFormat)) )	 // not SD to SD
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, inputXptYUV1);
		}
		else
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptConversionModule);
			bEanbleConverter = true;
		}
	}
	
	
	// Frame Buffer 2
	if (b2FbLevelBHfr)
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, inputXptYUV2);
	}
	else 
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	}
	
	
	// Make sure both channels are enable for stereo, dual-link B
	if (b2FbLevelBHfr)
	{
		bFb1Disable = bFb2Disable = false;
	}
		
	// set Channel disable mode (0 = enable, 1 = disable)
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);		


	// SDI Out 1 
	if (b2FbLevelBHfr ||																// Dual Stream - p60b
		mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect ||					// Stereo 3D
		mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)						// Video + Key
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, frameSync2YUV);
		}
		else 
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
		}
	}
	else if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)				// Same as RGB in this case
	{		
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);							// 1 3Gb wire
			mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);							// 2 wires
		}
	}
	else if (   (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)			// if our output is "Primary"
		|| (   (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)			// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
		    && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		if (inputColorSpace == NTV2_ColorSpaceModeRgb && mSDIOutput1ColorSpace != NTV2_ColorSpaceModeRgb)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
		}
		else 
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		}
	}
	else if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync2YUV);
	}
	
	
	
	// SDI Out 2
	if (b2FbLevelBHfr ||																// Dual Stream - p60b
		mVirtualDigitalOutput2Select == NTV2_StereoOutputSelect ||					// Stereo 3D
		mVirtualDigitalOutput2Select == NTV2_VideoPlusKeySelect)						// Video + Key
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, frameSync2YUV);
		}
		else 
		{
			mCard->Connect (NTV2_XptSDIOut2Input, frameSync2YUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
		}
	}
	else if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)				// Same as RGB in this case
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut1);							// 1 3Gb wire
			mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptDuallinkOut1DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut1DS2);						// 2 wires
		}
	}
	else if ( (mVirtualDigitalOutput2Select == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		      || (   (mVirtualDigitalOutput2Select == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
			      && (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
				  && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		if (inputColorSpace == NTV2_ColorSpaceModeRgb && mSDIOutput1ColorSpace != NTV2_ColorSpaceModeRgb)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptCSC1VidYUV);
		}
		else 
		{
			mCard->Connect (NTV2_XptSDIOut2Input, frameSync1YUV);
		}
	}
	else if (mVirtualDigitalOutput2Select == NTV2_SecondaryOutputSelect)
	{
		mCard->Connect (NTV2_XptSDIOut2Input, frameSync2YUV);
	}

	
	// HDMI Out
	if (   (mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect)						// if our output is "Primary"
		|| (   (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)				// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
			&& (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		if (inputColorSpace == NTV2_ColorSpaceModeRgb)
		{
			mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptCSC1VidYUV);
		}
		else 
		{
			mCard->Connect (NTV2_XptHDMIOutInput, frameSync1YUV);
		}
	}
	else if (mVirtualHDMIOutputSelect == NTV2_SecondaryOutputSelect)
	{
		mCard->Connect (NTV2_XptHDMIOutInput, frameSync2YUV);
	}

	
	// Analog Out
	if (   (mVirtualAnalogOutputSelect == NTV2_PrimaryOutputSelect)					// if our output is "Primary"
		|| (   (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)				// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
		    && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		if (inputColorSpace == NTV2_ColorSpaceModeRgb)
		{
			mCard->Connect (NTV2_XptAnalogOutInput, NTV2_XptCSC1VidYUV);
		}
		else 
		{
			mCard->Connect (NTV2_XptAnalogOutInput, frameSync1YUV);
		}
	}
	else if (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)
	{
		mCard->Connect (NTV2_XptAnalogOutInput, frameSync2YUV);
	}
	
	
	// UFC enable
	// should be disabled unless it is in use. It can consume significant memory/dma bandwidth
	mCard->SetEnableConverter(bEanbleConverter);
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void IoXTServices::SetDeviceMiscRegisters ()
{	
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();

	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	
	mCard->GetStandard(primaryStandard);
	mCard->GetFrameGeometry(primaryGeometry);
	
	// VPID
	//bool					b3GbOut	= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	//bool					bSdiOutRGB			= (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb);
	//bool					bDualStreamOut		= (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect) ||
	//											  (mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect) ||
	//											  IsVideoFormatB(mFb1VideoFormat) ||
	//											  bSdiOutRGB;

	NTV2Standard			secondaryStandard = GetNTV2StandardFromVideoFormat(mVirtualSecondaryFormatSelect);
	//NTV2FrameGeometry		secondaryGeometry = GetNTV2FrameGeometryFromVideoFormat(mVirtualSecondaryFormatSelect);
	
	NTV2FrameRate			primaryFrameRate = GetNTV2FrameRateFromVideoFormat (mFb1VideoFormat);
	NTV2FrameRate			secondardFrameRate = GetNTV2FrameRateFromVideoFormat (mVirtualSecondaryFormatSelect);
	
	NTV2VideoFormat			inputFormat = NTV2_FORMAT_UNKNOWN;
	
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
		
		// HDMI Out Stereo 3D
		HDMIOutStereoSelect stereoSelect = mHDMIOutStereoSelect;
		
		// in auto mode, follow codec settings
		if (stereoSelect == kHDMIOutStereoAuto)
			stereoSelect = mHDMIOutStereoCodecSelect;
		
		switch (stereoSelect)
		{
			case kHDMIOutStereoSideBySide:
				mCard->SetHDMIOut3DPresent(true);
				mCard->SetHDMIOut3DMode(NTV2_HDMI3DSideBySide);
				break;
			case kHDMIOutStereoTopBottom:
				mCard->SetHDMIOut3DPresent(true);
				mCard->SetHDMIOut3DMode(NTV2_HDMI3DTopBottom);
				break;
			case kHDMIOutStereoOff:
			default:
				mCard->SetHDMIOut3DPresent(false);
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
	// Analog-Out
	//
	
	// Control video DAC mode (reg 128)
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

	//
	// Up/Down Converter
	//
	if (mFb1Mode == NTV2_MODE_DISPLAY)								// playback mode: converter is always on output,
	{
		// set pulldown bit
		mCard->SetConverterPulldown( (ULWord)IsPulldownConverterMode(mFb1VideoFormat,mVirtualSecondaryFormatSelect) );
		mCard->SetDeinterlaceMode( (ULWord)IsDeinterlacedMode(mFb1VideoFormat,mVirtualSecondaryFormatSelect) );
		mCard->SetConverterOutStandard(secondaryStandard);			// so converter output = secondary format
		mCard->SetConverterInStandard(primaryStandard);
	}
	else														// capture mode: converter may be on input or output
	{
		if (inputFormat == mFb1VideoFormat)
		{
			// no input conversion needed - put converter on output
			mCard->SetConverterPulldown( (ULWord)IsPulldownConverterMode(mFb1VideoFormat,mVirtualSecondaryFormatSelect) );
			mCard->SetDeinterlaceMode( (ULWord)IsDeinterlacedMode(mFb1VideoFormat,mVirtualSecondaryFormatSelect) );
			mCard->SetConverterOutStandard(secondaryStandard);
			mCard->SetConverterInStandard(primaryStandard);
		}
		else
		{
			// input conversion needed - need converter on input
			mCard->SetConverterPulldown( (ULWord)IsPulldownConverterMode(mVirtualSecondaryFormatSelect, mFb1VideoFormat) );
			mCard->SetDeinterlaceMode( (ULWord)IsDeinterlacedMode(mVirtualSecondaryFormatSelect, mFb1VideoFormat) );
			mCard->SetConverterOutStandard(primaryStandard);
			mCard->SetConverterInStandard(secondaryStandard);
		}
	}
	
	
	// Set VBlank RGB range bits - ALWAYS SMPTE
	// Except when there is a full-range RGB frame buffer, and we go through the color space converter
	if (mRGB10Range == NTV2_RGB10RangeFull && mSDIOutput1ColorSpace != NTV2_ColorSpaceModeRgb)
	{
		mCard->WriteRegister(kRegCh1Control, NTV2_RGB10RangeFull, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
		mCard->WriteRegister(kRegCh2Control, NTV2_RGB10RangeFull, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
	}
	else
	{
		mCard->WriteRegister(kRegCh1Control, NTV2_RGB10RangeSMPTE, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
		mCard->WriteRegister(kRegCh2Control, NTV2_RGB10RangeSMPTE, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
	}
	
	// Set HBlack RGB range bits - ALWAYS SMPTE
	mCard->WriteRegister(kRegSDIOut1Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	mCard->WriteRegister(kRegSDIOut2Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	
	// Set converter output standard (bits 2-0)
	if (mFb1Mode == NTV2_MODE_DISPLAY)								// playback mode - converter always on output
	{
		mCard->SetConverterInStandard(primaryStandard);				// so converter input = primary format
	}
	else														// capture mode: converter may be on input or output
	{
		if (inputFormat == mFb1VideoFormat)					// no input conversion needed - put converter on output
			mCard->SetConverterInStandard(primaryStandard);
		else
			mCard->SetConverterInStandard(secondaryStandard);		// input conversion needed - need converter on input
	}
}
