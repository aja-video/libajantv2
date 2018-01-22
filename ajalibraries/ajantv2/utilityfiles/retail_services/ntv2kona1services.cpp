//
//  ntv2kona1services.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2kona1services.h"


//-------------------------------------------------------------------------------------------------------
//	class Corvid3GServices
//-------------------------------------------------------------------------------------------------------

Kona1Services::Kona1Services()
{
}

//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void Kona1Services::SetDeviceXPointPlayback (GeneralFrameFormat genFrameFormat)
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback(genFrameFormat);

	NTV2FrameBufferFormat fbFormatCh1;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormatCh1);
	bool bCh1RGB = IsFrameBufferFormatRGB(fbFormatCh1);
		
	NTV2FrameBufferFormat fbFormatCh2;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL2, &fbFormatCh2);
	bool bCh2RGB = IsFrameBufferFormatRGB(fbFormatCh2);
		
	bool bDSKGraphicMode = (mDSKMode == NTV2_DSKModeGraphicOverMatte || mDSKMode == NTV2_DSKModeGraphicOverVideoIn || mDSKMode == NTV2_DSKModeGraphicOverFB);
	bool bDSKOn = (mDSKMode == NTV2_DSKModeFBOverMatte || mDSKMode == NTV2_DSKModeFBOverVideoIn || (bCh2RGB && bDSKGraphicMode));
		
	// don't let the DSK be ON if we're in Mac Desktop mode
	if (!mStreamingAppPID && mDefaultVideoOutMode == kDefaultModeDesktop)
		bDSKOn = false;
		
	bool bStereoOut			= mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect;
	bool bLevelBFormat		= IsVideoFormatB(mFb1VideoFormat);
	bool b3GbTransportOut	= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);			// use 2 SDI wires, or just 1 3Gb
	
	// make sure frame DualLink B mode (SMPTE 372), Stereo
	if (bLevelBFormat || bStereoOut)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, fbFormatCh1);
		bCh2RGB = bCh1RGB;
	}

	// Frame Sync 1
	NTV2CrosspointID frameSync1YUV;
	if (bStereoOut || bLevelBFormat)
	{
		if (genFrameFormat == FORMAT_RGB)
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
		if (genFrameFormat == FORMAT_RGB)
		{
			frameSync1YUV = NTV2_XptCSC1VidYUV;
		}
		else if (genFrameFormat == FORMAT_COMPRESSED)
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
	if (bStereoOut || bLevelBFormat)
	{
		if (genFrameFormat == FORMAT_RGB)
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
	else if (genFrameFormat == FORMAT_RGB)
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
	if (genFrameFormat == FORMAT_RGB || bDSKOn)
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}
	else if (genFrameFormat == FORMAT_COMPRESSED)
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptCompressionModule);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV);
	}
	
	
	// CSC 2
	if ( bCh2RGB )
	{
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptLUT2RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2YUV);
	}
	
	
	// LUT 1
	if (genFrameFormat == FORMAT_RGB || bDSKOn)
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptFrameBuffer1RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	
	
	// LUT 2
	if ( bCh2RGB )
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptFrameBuffer2RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	else
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptCSC2VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_YUV2RGB);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	

	// Frame Buffer 1
	if  (genFrameFormat == FORMAT_COMPRESSED)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCompressionModule);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
	}
	
	
	// SDI Out 1
	if (bLevelBFormat || bStereoOut)												// B format or Stereo 3D
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
		else if (genFrameFormat == FORMAT_RGB)
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
	int bCh1Disable = 0;						// Assume Channel 1 is NOT disabled
	int bCh2Disable = 1;						// Assume Channel 2 IS disabled
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
				else if (genFrameFormat == FORMAT_COMPRESSED)
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
				else if (genFrameFormat == FORMAT_COMPRESSED)
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
				if (bCh2RGB)
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
				bCh1Disable = 1;			// disable Ch 1
				bCh2Disable = 0;			// enable Ch 2
				mCard->WriteRegister (kRegVidProc1Control, 1, kRegMaskVidProcBGMatteEnable, kRegShiftVidProcBGMatteEnable);
				break;
			
			case NTV2_DSKModeGraphicOverVideoIn:
				// Foreground
				if (bCh2RGB)
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
				
				bCh1Disable = 1;			// disable Ch 1
				bCh2Disable = 0;			// enable Ch 2
				
				// in "Frame Buffer over VideoIn" mode, where should the audio come from?
				if (mDSKAudioMode == NTV2_DSKAudioBackground)
					audioLoopbackMode = 1;							// set audio to "input loopthru" (aka "E-E") mode
				
				bDSKNeedsInputRef = true;		// genlock to input video
				break;
			
			case NTV2_DSKModeGraphicOverFB:			
				// Foreground
				if (bCh2RGB)
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
				if (genFrameFormat == FORMAT_COMPRESSED)
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
				bCh2Disable = 0;			// enable Ch 2
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
	if (bLevelBFormat || bStereoOut)
	{
		bCh1Disable = bCh2Disable = 0; 
	}
	// set Channel disable mode (0 = enable, 1 = disable)
	mCard->WriteRegister(kRegCh1Control, bCh1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bCh2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);		

}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void Kona1Services::SetDeviceXPointCapture (GeneralFrameFormat genFrameFormat)
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture(genFrameFormat);

	NTV2RGBRangeMode			frambBufferRange	= (mRGB10Range == NTV2_RGB10RangeSMPTE) ? NTV2_RGBRangeSMPTE : NTV2_RGBRangeFull;

	bool						bLevelBFormat		= IsVideoFormatB(mFb1VideoFormat);
	bool						b3GbTransportOut	= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	bool						bStereoIn			= mSDIInput1FormatSelect == NTV2_Stereo3DSelect;
	int							bCh1Disable			= 0;					// Assume Channel 1 is NOT disabled by default
	int							bCh2Disable			= 1;					// Assume Channel 2 IS disabled by default
													  
	NTV2CrosspointID	inputXptYUV1		= NTV2_XptBlack;		// Input source selected single stream
	NTV2CrosspointID	inputXptYUV2		= NTV2_XptBlack;		// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
	NTV2VideoFormat				inputFormat			= mFb1VideoFormat;	// Input source selected format
	NTV2SDIInputFormatSelect	inputFormatSelect	= NTV2_YUVSelect;		// Input format select (YUV, RGB, Stereo 3D)
	
	
	// Figure out what our input format is based on what is selected 
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat, &inputFormatSelect);
	
	
	// make sure frame buffer formats match for DualLink B mode (SMPTE 372)
	if (bLevelBFormat || bStereoIn)
	{
		NTV2FrameBufferFormat fbFormat;
		mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormat);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, fbFormat);
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
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);		// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptDuallinkIn1);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	
	
	// LUT 2 
	// provides SMPTE <-> Full conversion
	if (inputFormatSelect == NTV2_RGBSelect)
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptDuallinkIn1);
		mCard->SetColorCorrectionOutputBank (	NTV2_CHANNEL2,						// NOTE: this conflicts with using AutoCirculate Color Correction!
										mSDIInput1RGBRange == NTV2_RGBRangeFull ? 
										kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
	}
	

	// Frame Buffer 1
	if (bLevelBFormat || bStereoIn)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, inputXptYUV1);
	}
	else if (genFrameFormat == FORMAT_RGB)
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
	else if (genFrameFormat == FORMAT_COMPRESSED)
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
	if (bLevelBFormat || bStereoIn)
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, inputXptYUV2);
	}
	else 
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	}
	
	
	// Make sure both channels are enable for stereo, level B
	if (bLevelBFormat || bStereoIn)
	{
		bCh1Disable = bCh2Disable = false;
	}
		
	// set Channel disable mode (0 = enable, 1 = disable)
	mCard->WriteRegister(kRegCh1Control, bCh1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bCh2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);		


	// SDI Out 1 
	if (bLevelBFormat ||																// Dual Stream - p60b
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
void Kona1Services::SetDeviceMiscRegisters (NTV2Mode mode)
{	
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters(mode);
	NTV2FrameBufferFormat   primaryPixelFormat;

	mCard->GetFrameBufferFormat (NTV2_CHANNEL1, &primaryPixelFormat);
	
	// special case - VANC 8bit pixel shift support
	if (mVANCMode && Is8BitFrameBufferFormat(primaryPixelFormat) )
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	
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





