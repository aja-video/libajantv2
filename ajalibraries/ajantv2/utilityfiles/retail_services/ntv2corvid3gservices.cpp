//
//  ntv2corvid3gservices.cpp
//
//  Copyright (c) 2013 AJA Video, Inc. All rights reserved.
//

#include "ntv2corvid3gservices.h"


//-------------------------------------------------------------------------------------------------------
//	class Corvid3GServices
//-------------------------------------------------------------------------------------------------------

Corvid3GServices::Corvid3GServices()
{
}

//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void Corvid3GServices::SetDeviceXPointPlayback ()
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
	bool b3GbTransportOut	= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);			// use 2 SDI wires, or just 1 3Gb
	
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
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);
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
	

	// Frame Buffer 1
	if  (bFb1Compressed)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCompressionModule);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
	}
	
	
	// SDI Out 1
	if (b2FbLevelBHfr || bStereoOut)												// B format or Stereo 3D
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbTransportOut ? frameSync2YUV : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)			// if our output is "Primary"
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)				// Video+Key
	{		
		if (bDSKOn)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbTransportOut ? frameSync2YUV : NTV2_XptBlack);
		}
		else if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbTransportOut ? NTV2_XptCSC1KeyYUV : NTV2_XptBlack);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
		}
	}
	
	
	//
	// Mixer/Keyer
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
				mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
				mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
				
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
				mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
				mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
				
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

}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void Corvid3GServices::SetDeviceXPointCapture ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();

	NTV2RGBRangeMode			frambBufferRange	= (mRGB10Range == NTV2_RGB10RangeSMPTE) ? NTV2_RGBRangeSMPTE : NTV2_RGBRangeFull;
	bool 						bFb1RGB 			= IsFormatRGB(mFb1Format);
	bool 						bFb1Compressed 		= IsFormatCompressed(mFb1Format);
	bool						b2FbLevelBHfr		= IsVideoFormatB(mFb1VideoFormat);
	bool						b3GbTransportOut	= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	bool						bStereoIn			= mSDIInput1FormatSelect == NTV2_Stereo3DSelect;
	int							bFb1Disable			= 0;					// Assume Channel 1 is NOT disabled by default
	int							bFb2Disable			= 1;					// Assume Channel 2 IS disabled by default
													  
	NTV2CrosspointID	inputXptYUV1		= NTV2_XptBlack;		// Input source selected single stream
	NTV2CrosspointID	inputXptYUV2		= NTV2_XptBlack;		// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
	NTV2VideoFormat				inputFormat			= mFb1VideoFormat;	// Input source selected format
	NTV2SDIInputFormatSelect	inputFormatSelect	= NTV2_YUVSelect;		// Input format select (YUV, RGB, Stereo 3D)
	
	
	// Figure out what our input format is based on what is selected 
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat, &inputFormatSelect);
	
	
	// make sure frame buffer formats match for DualLink B mode (SMPTE 372)
	if (b2FbLevelBHfr || bStereoIn)
	{
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
	}
	
	
	// input 1 select
	inputXptYUV1 = NTV2_XptSDIIn1;
	inputXptYUV2 = NTV2_XptSDIIn1DS2;
	
	
	// Frame Sync 1
	NTV2CrosspointID frameSync1YUV = inputXptYUV1;
	
	
	// Frame Sync 2
	NTV2CrosspointID frameSync2YUV = inputXptYUV1;


	// Compression Module
	if (inputFormat == mFb1VideoFormat)
	{
		mCard->Connect (NTV2_XptCompressionModInput, inputXptYUV1);
	}
	else
	{
		mCard->Connect (NTV2_XptCompressionModInput, NTV2_XptConversionModule);
	}

	
	// CSC 1
	if (inputFormatSelect != NTV2_RGBSelect)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			mCard->Connect (NTV2_XptCSC1VidInput, inputXptYUV1);
		}
		else
		{
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptConversionModule);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}
	

	// LUT 1
	if (inputFormatSelect != NTV2_RGBSelect)
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);	
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptDuallinkIn1);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);
	}
	
	
	// LUT 2 
	// provides SMPTE <-> Full conversion
	if (inputFormatSelect == NTV2_RGBSelect)
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptDuallinkIn1);
		mCard->SetColorCorrectionOutputBank (	NTV2_CHANNEL2,	
										mSDIInput1RGBRange == NTV2_RGBRangeFull ? 
										kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
	}
	

	// Frame Buffer 1
	if (b2FbLevelBHfr || bStereoIn)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, inputXptYUV1);
	}
	else if (bFb1RGB)
	{
		if (inputFormatSelect == NTV2_RGBSelect)
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
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);				// CSC converted
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
	if (b2FbLevelBHfr || bStereoIn)
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, inputXptYUV2);
	}
	else 
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	}
	
	
	// Make sure both channels are enable for stereo, level B
	if (b2FbLevelBHfr || bStereoIn)
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
		if (b3GbTransportOut)
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
	else if (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)			// if our output is "Primary"
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
	}
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void Corvid3GServices::SetDeviceMiscRegisters ()
{	
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();

	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	
	mCard->GetStandard(&primaryStandard);
	mCard->GetFrameGeometry(&primaryGeometry);
	
	// VPID
	bool					b3GbTransportOut	= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	bool					bSdiOutRGB			= (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect);
	bool					bDualStreamOut		= (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect) ||
												  (mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect) ||
												  IsVideoFormatB(mFb1VideoFormat) ||
												  bSdiOutRGB;
	
	// special case - VANC 8bit pixel shift support
	if (mVANCMode && Is8BitFrameBufferFormat(mFb1Format) )
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	
	
	//
	// SDI Out 1
	//
	// Select primary standard
	mCard->SetSDIOut2Kx1080Enable( NTV2_CHANNEL1, primaryGeometry == NTV2_FG_2048x1080 );
	mCard->SetSDIOutputStandard(NTV2_CHANNEL1, primaryStandard);
	
	// 3Ga / 3Gb / Neither
	if (bDualStreamOut && b3GbTransportOut)
	{
		mCard->SetSDIOut3GEnable(NTV2_CHANNEL1, true);
		mCard->SetSDIOut3GbEnable(NTV2_CHANNEL1, b3GbTransportOut);
	}
	else
	{
		mCard->SetSDIOut3GEnable(NTV2_CHANNEL1, IsVideoFormatA(mFb1VideoFormat));
		mCard->SetSDIOut3GbEnable(NTV2_CHANNEL1, false);
	}
	
		
	// Set VBlank RGB range bits - ALWAYS SMPTE
	// Except when there is a full-range RGB frame buffer, and we go through the color space converter
	if (mRGB10Range == NTV2_RGB10RangeFull && mVirtualDigitalOutput1Select != NTV2_DualLinkOutputSelect)
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

}





