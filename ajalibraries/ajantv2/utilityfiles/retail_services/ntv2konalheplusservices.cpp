//
//  ntv2konalheplusservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2konalheplusservices.h"


//-------------------------------------------------------------------------------------------------------
//	class KonaLHePlusServices
//-------------------------------------------------------------------------------------------------------

KonaLHePlusServices::KonaLHePlusServices()
{
}


//-------------------------------------------------------------------------------------------------------
//	GetSelectedInputVideoFormat
//	Note:	Determine input video format based on input select and fbVideoFormat
//			which currently is videoformat of ch1-framebuffer
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat KonaLHePlusServices::GetSelectedInputVideoFormat(
											NTV2VideoFormat fbVideoFormat,
											NTV2SDIInputFormatSelect* inputFormatSelect)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	if (inputFormatSelect)
		*inputFormatSelect = NTV2_YUVSelect;
	
	// Figure out what our input format is based on what is selected 
	switch (mVirtualInputSelect)
	{
		case NTV2_Input1Select:
			inputFormat = GetSdiInVideoFormat(0, fbVideoFormat);
			break;
		case NTV2_Input2Select:
			inputFormat = mCard->GetAnalogInputVideoFormat();
			break;
		default: break;
	}
	inputFormat = GetTransportCompatibleFormat(inputFormat, fbVideoFormat);
	
	return inputFormat;
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void KonaLHePlusServices::SetDeviceXPointPlayback ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback();
	
	bool bFb1RGB = IsFormatRGB(mFb1Format);
	bool bFb2RGB = IsFormatRGB(mFb2Format);
	bool bFb1Compressed = IsFormatCompressed(mFb1Format);
		
	bool bDSKGraphicMode = (mDSKMode == NTV2_DSKModeGraphicOverMatte || mDSKMode == NTV2_DSKModeGraphicOverVideoIn || mDSKMode == NTV2_DSKModeGraphicOverFB);
	bool bDSKOn = (mDSKMode == NTV2_DSKModeFBOverMatte || mDSKMode == NTV2_DSKModeFBOverVideoIn || (bFb2RGB && bDSKGraphicMode));
						
	// don't let the DSK be ON if we're in Mac Desktop mode
	if (!mStreamingAppPID && mDefaultVideoOutMode == kDefaultModeDesktop)
		bDSKOn = false;
	
	bool bStereoOut			= mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect;
	bool b2FbLevelBHfr		= IsVideoFormatB(mFb1VideoFormat);
	
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
		if (mGammaMode == NTV2_GammaNone)
		{
			frameSync2RGB = NTV2_XptLUT1RGB;
		}
		else
		{
			frameSync2RGB = NTV2_XptFrameBuffer1RGB;
		}
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
	
	
	// LUT 1
	if (bFb1RGB || bDSKOn )
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptFrameBuffer1RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);
	}


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
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptAnalogIn);
		}
	}
	
	
	// Frame Buffer 2
	mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	

	// SDI Out 1
	if (b2FbLevelBHfr || bStereoOut)												// B format or Stereo 3D
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
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
	}
	else if (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)				// Video+Key
	{		
		if (bDSKOn)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		}
		else if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		}
	}
	
	
	// SDI Out 2
	if (b2FbLevelBHfr || bStereoOut)												// B format or Stereo 3D
	{
		mCard->Connect (NTV2_XptSDIOut2Input, frameSync2YUV);
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
	}
	else if (mVirtualDigitalOutput2Select == NTV2_VideoPlusKeySelect)				// Video+Key
	{
		if (bDSKOn)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, frameSync2YUV);
		}
		else if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptCSC1KeyYUV);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut2Input, frameSync1YUV);
		}
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
	}
	
	
	//
	// Set Mixer/Keyer Inputs
	//
	
	// default video proc mode
	mCard->WriteRegister (kRegVidProc1Control, 0, ~kRegMaskVidProcLimiting);		// FG = Full, BG = Full, VidProc = FG On
	
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
					// Select input 1 (0x01)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
				}
				else if (mVirtualInputSelect == NTV2_Input2Select)
				{
					// Select input 2 (0x02)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptAnalogIn);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptAnalogIn);
				}
				
				// in "Frame Buffer over VideoIn" mode, where should the audio come from?
				if (mDSKAudioMode == NTV2_DSKAudioBackground)
					audioLoopbackMode = 1;							// set audio to "input loopthru" (aka "E-E") mode
				
				bDSKNeedsInputRef = true;		// genlock to input video
				break;

			case NTV2_DSKModeGraphicOverMatte:
				// Foreground
				if (bFb2RGB)
				{
					// The foreground video/key comes from the CSC 2 output (0x10/0x11)
					//mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
					//mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
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
					//mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
					//mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
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
					// Select input 2 (0x02)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptAnalogIn);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptAnalogIn);
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
				if (bFb2RGB)
				{
					// The foreground video/key comes from the CSC 2 output (0x10/0x11)
					//mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
					//mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
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
			mCard->WriteRegister (kRegVidProc1Control, 0, kRegMaskVidProcFGControl, kRegShiftVidProcFGControl);			// FG = Full Raster
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
	if (b2FbLevelBHfr || bStereoOut)
	{
		bFb1Disable = bFb2Disable = 0; 
	}
	// set Channel disable mode (0 = enable, 1 = disable)
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);		
}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void KonaLHePlusServices::SetDeviceXPointCapture ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();

	bool 				bFb1RGB = IsFormatRGB(mFb1Format);
	bool 				bFb1Compressed = IsFormatCompressed(mFb1Format);
	NTV2VideoFormat		inputFormat = NTV2_FORMAT_UNKNOWN;
	NTV2CrosspointID	inputXptYUV1;
	
	// Figure out what our input format is based on what is selected 
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat);
	
	
	// This is done all over so do it once here so we have the value
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		inputXptYUV1 = NTV2_XptSDIIn1;
	}
	else
	{
		inputXptYUV1 = NTV2_XptAnalogIn;
	}
	
	
	// Frame Sync 1
	NTV2CrosspointID frameSync1YUV;
	if (inputFormat == mFb1VideoFormat && !ISO_CONVERT_FMT(inputFormat))
	{
		frameSync1YUV = inputXptYUV1;
	}
	else
	{
		frameSync1YUV = NTV2_XptConversionModule;
	}
	
	
	// Frame Sync 2
	NTV2CrosspointID frameSync2YUV;
	if (inputFormat == mFb1VideoFormat && !ISO_CONVERT_FMT(inputFormat))
	{
		frameSync2YUV = NTV2_XptConversionModule;
	}
	else
	{
		frameSync2YUV = inputXptYUV1;
	}


	// Compression
	if (inputFormat == mFb1VideoFormat)
	{
		mCard->Connect (NTV2_XptCompressionModInput, inputXptYUV1);
	}
	else
	{
		mCard->Connect (NTV2_XptCompressionModInput, NTV2_XptConversionModule);
	}

	
	// Up/Down converter
	mCard->Connect (NTV2_XptConversionModInput, inputXptYUV1);


	// CSC 1
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptSDIIn1);
		}
		else
		{
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptConversionModule);
		}
	}
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptAnalogIn);
		}
		else
		{
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptConversionModule);
		}
	}
	

	// LUT 1
	if ((mVirtualInputSelect == NTV2_Input1Select) ||
		(mVirtualInputSelect == NTV2_Input2Select))
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);
	}


	// Frame Buffer 1
	if  (bFb1RGB)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			if ((mVirtualInputSelect == NTV2_Input1Select) ||
				(mVirtualInputSelect == NTV2_Input2Select))
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);
			}
		}
		else
		{
			// NOTE: This is the same logic as above but we can't do the dual link case because we would
			// need two LUT's to convert RGB to YUB then back again.
			if ((mVirtualInputSelect == NTV2_Input1Select) ||
				(mVirtualInputSelect == NTV2_Input2Select))
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);
			}
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
		}
	}
	
	
	// Frame Buffer 2
	mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);


	// SDI Out 1 
	if (   (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		|| (   (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
		    && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select frame sync 2 output (0x0a)
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync2YUV);
	}
	
	
	// SDI Out 2 
	if (	(mVirtualDigitalOutput2Select == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		      || (   (mVirtualDigitalOutput2Select == NTV2_SecondaryOutputSelect)		// or if "Secondary" AND Secondary == Primary and not SD format
			      && (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
				  && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		mCard->Connect (NTV2_XptSDIOut2Input, frameSync1YUV);
	}
	else if (mVirtualDigitalOutput2Select == NTV2_SecondaryOutputSelect)
	{
		mCard->Connect (NTV2_XptSDIOut2Input, frameSync2YUV);
	}

	
	// Analog Out
	if (   (mVirtualAnalogOutputSelect == NTV2_PrimaryOutputSelect)				// if our output is "Primary"
		|| (   (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)			// or if "Secondary" AND Secondary == Primary and not SD format
			&& (mVirtualSecondaryFormatSelect == mFb1VideoFormat)
		    && (!ISO_CONVERT_FMT(mVirtualSecondaryFormatSelect)) ) )
	{
		// Select frame sync 1 output (0x09)
		mCard->Connect (NTV2_XptAnalogOutInput, frameSync1YUV);
	}
	else if (mVirtualAnalogOutputSelect == NTV2_SecondaryOutputSelect)
	{
		// Select frame sync 2 output (0x0a)
		mCard->Connect (NTV2_XptAnalogOutInput, frameSync2YUV);
	}
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void KonaLHePlusServices::SetDeviceMiscRegisters ()
{	
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();

	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	
	mCard->GetStandard(&primaryStandard);
	mCard->GetFrameGeometry(&primaryGeometry);
	
	NTV2Standard			secondaryStandard = GetNTV2StandardFromVideoFormat (mVirtualSecondaryFormatSelect);
	NTV2FrameGeometry		secondaryGeometry = GetNTV2FrameGeometryFromVideoFormat (mVirtualSecondaryFormatSelect);
	
	NTV2VideoFormat			inputFormat = NTV2_FORMAT_UNKNOWN;
	
	bool					bDualStreamOut	= (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect) ||
											  (mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect) ||
											  IsVideoFormatB(mFb1VideoFormat);
	
	// FrameBuffer 2: make sure formats matches FB1 for DualLink B mode (SMPTE 372)
	if (bDualStreamOut)
	{
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
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


	//
	// Up/Down Converter
	//
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

	
	//
	// SDI Out 1
	//
	if (mVirtualDigitalOutput1Select == NTV2_SecondaryOutputSelect)
	{
		// Select secondary standard
		mCard->SetSDIOut2Kx1080Enable( NTV2_CHANNEL1, secondaryGeometry == NTV2_FG_2048x1080 );
		mCard->SetSDIOutputStandard(NTV2_CHANNEL1, secondaryStandard);
	}
	else
	{
		// Select primary standard
		mCard->SetSDIOut2Kx1080Enable( NTV2_CHANNEL1, primaryGeometry == NTV2_FG_2048x1080 );
		mCard->SetSDIOutputStandard(NTV2_CHANNEL1, primaryStandard);
	}


	//
	// SDI Out 2
	//
	if (mVirtualDigitalOutput2Select == NTV2_SecondaryOutputSelect)
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
	
	
	// Finish VPID for SDI 1 Out / SDI 2 Out 
	/* Not supported yet
	{
		// don't overwrite if e-to-e and input and outputs match
		ULWord overwrite =	!(mFb1Mode == NTV2_MODE_CAPTURE);
		
		mCard->WriteRegister(kRegSDIOut1Control, overwrite, kRegMaskVPIDInsertionOverwrite);
		mCard->WriteRegister(kRegSDIOut2Control, overwrite, kRegMaskVPIDInsertionOverwrite);
		
		// enable VPID write
		mCard->WriteRegister(kRegSDIOut1Control, 1, kRegMaskVPIDInsertionEnable);
		mCard->WriteRegister(kRegSDIOut2Control, 1, kRegMaskVPIDInsertionEnable);

		// write VPID for SDI 1
		mCard->WriteRegister(kRegSDIOut1VPIDA, vpidOut1a);
		
		// write VPID for SDI 2
		mCard->WriteRegister(kRegSDIOut2VPIDA, vpidOut2a);
	}
	*/
}
