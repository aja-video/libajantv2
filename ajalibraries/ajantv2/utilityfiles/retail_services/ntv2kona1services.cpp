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
void Kona1Services::SetDeviceXPointPlayback ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback();

	bool						bFb1RGB				= IsRGBFormat(mFb1Format);
	bool						bFb2RGB				= IsRGBFormat(mFb2Format);
	
	bool						b2FbLevelBHfr		= IsVideoFormatB(mFb1VideoFormat);
	bool						bStereoOut			= mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect;
	bool						bSdiOutRGB			= mVirtualDigitalOutput1Select == NTV2_RgbOutputSelect;
	bool						b3GbOut				= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	int							bFb1Disable			= 0;						// Assume Channel 1 is NOT disabled by default
	int							bFb2Disable			= 1;						// Assume Channel 2 IS disabled by default
	bool						bDSKGraphicMode		= mDSKMode == NTV2_DSKModeGraphicOverMatte || 
													  mDSKMode == NTV2_DSKModeGraphicOverVideoIn || 
													  mDSKMode == NTV2_DSKModeGraphicOverFB;
	bool						bDSKOn				= mDSKMode == NTV2_DSKModeFBOverMatte || 
													  mDSKMode == NTV2_DSKModeFBOverVideoIn || 
													  (bFb2RGB && bDSKGraphicMode);
	NTV2SDIInputFormatSelect	inputFormatSelect	= mSDIInput1FormatSelect;	// Input format select (YUV, RGB, Stereo 3D)
	bool						bInRGB				= inputFormatSelect == NTV2_RGBSelect;
		
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
	
	
	// Dual Link In 1
	if (bInRGB)
	{
		mCard->Connect (NTV2_XptDualLinkIn1Input, NTV2_XptSDIIn1);
		mCard->Connect (NTV2_XptDualLinkIn1DSInput, NTV2_XptSDIIn1DS2);
	}
	else
	{
		mCard->Connect (NTV2_XptDualLinkIn1Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptDualLinkIn1DSInput, NTV2_XptBlack);
	}
	
	
	// CSC 1
	if (bFb1RGB || bDSKOn)
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
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
		if (bSdiOutRGB)
		{
			mCard->SetColorCorrectionOutputBank ( NTV2_CHANNEL1,					
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
	
		// if RGB-to-RGB apply LUT converter
		if (bSdiOutRGB)
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2,					
												 mRGB10Range == NTV2_RGB10RangeFull ?
												 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
		}
		else
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);	
		}
	}
	else
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptCSC2VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_YUV2RGB);
	}
	

	// Frame Buffer 1
	mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptBlack);
	
	
	// Frame Buffer 2
	mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	
	
	// Dual Link Out 1
	mCard->Connect (NTV2_XptDualLinkOut1Input, frameSync2RGB);
	
	
	// SDI Out 1
	if (bSdiOutRGB)
	{
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);
	}
	else if (b2FbLevelBHfr || bStereoOut)										// B format or Stereo 3D
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbOut ? frameSync2YUV : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_PrimaryOutputSelect)			// if our output is "Primary"
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)			// Video+Key
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
	
	
	//
	// Mixer/Keyer
	//
	
	// default video proc mode
	mCard->WriteRegister (kRegVidProc1Control, 0, ~kRegMaskVidProcLimiting);		// FG = Full, BG = Full, VidProc = FG On
	
	// The background video/key depends on the DSK mode
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
	

void Kona1Services::SetDeviceXPointCapture ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();

	NTV2RGBRangeMode			frambBufferRange	= (mRGB10Range == NTV2_RGB10RangeSMPTE) ? NTV2_RGBRangeSMPTE : NTV2_RGBRangeFull;
	bool 						bFb1RGB 			= IsRGBFormat(mFb1Format);
	bool						b3GbOut				= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	//bool						b4K					= NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	//bool						b4kHfr              = NTV2_IS_4K_HFR_VIDEO_FORMAT(mFb1VideoFormat);
	//bool						b2FbLevelBHfr		= IsVideoFormatB(mFb1VideoFormat);
	//bool						b2xQuadOut          = (b4K && !b4kHfr  && (mVirtualInputSelect == NTV2_DualLink2xSdi4k));
	//bool						bStereoIn			= false;
	//int							bFb1Disable			= 0;		// Assume Channel 1 is NOT disabled by default
	//int							bFb2Disable			= 1;		// Assume Channel 2 IS disabled by default
	//int							bFb3Disable			= 1;		// Assume Channel 2 IS disabled by default
	//int							bFb4Disable			= 1;		// Assume Channel 2 IS disabled by default

	NTV2CrosspointID			inputXptYUV1		= NTV2_XptBlack;				// Input source selected single stream
	NTV2CrosspointID			inputXptYUV2		= NTV2_XptBlack;				// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
	NTV2SDIInputFormatSelect	inputFormatSelect	= NTV2_YUVSelect;				// Input format select (YUV, RGB, Stereo 3D)

	// get selected input video format
	NTV2VideoFormat	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat, &inputFormatSelect);
	//bool inHfrB = IsVideoFormatB(inputFormat);

	inputXptYUV1 = NTV2_XptSDIIn1;
	inputXptYUV2 = NTV2_XptSDIIn1DS2;

	// SDI In 1
	bool b3GbInEnabled;
	mCard->GetSDIInput3GbPresent(b3GbInEnabled, NTV2_CHANNEL1);
	mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL1, (b3GbInEnabled && inputFormatSelect != NTV2_RGBSelect));

    if(mCard->GetVPIDValidA(NTV2_CHANNEL1))
    {
        ULWord vpida = 0, vpidb	= 0;
        mCard->ReadSDIInVPID(NTV2_CHANNEL1, vpida, vpidb);
        //debugOut("in  vpida = %08x  vpidb = %08x\n", true, vpida, vpidb);

        CNTV2VPID parser;
        parser.SetVPID(vpida);
        VPIDSampling sample = parser.GetSampling();
        if (sample == VPIDSampling_YUV_422)
        {
            inputFormatSelect = NTV2_YUVSelect;
        }
        else
        {
            inputFormatSelect = NTV2_RGBSelect;
        }
    }

	// Dual Link In 1
	if (inputFormatSelect == NTV2_RGBSelect)
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

	// CSC 1
	if (inputFormatSelect != NTV2_RGBSelect)
	{
			mCard->Connect (NTV2_XptCSC1VidInput, inputXptYUV1);
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

	// Dual Link Out 1,2,3,4 3 Out
	if (inputFormat == mFb1VideoFormat)
	{
		// Input is NOT secondary
		if (inputFormatSelect != NTV2_RGBSelect)
		{
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
		}
		else
		{
			if (mSDIInput1RGBRange == mSDIOutput1RGBRange && mLUTType != NTV2_LUTCustom)
			{
				// no range change
				mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptDuallinkIn1);
			}
			else
			{
				mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
			}
		}
	}
	else
	{
		// Input is Secondary format
		// NOTE: This is the same logic as above but we can't do the dual link case because we would
		// need two LUT's to convert RGB to YUB then back again.
		if (inputFormatSelect != NTV2_RGBSelect)
		{
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptDuallinkIn1);
		}
	}

	// Frame Buffer 1
	if (bFb1RGB)
	{
		if (inputFormatSelect == NTV2_RGBSelect)
		{
			if (mSDIInput1RGBRange == frambBufferRange && mLUTType != NTV2_LUTCustom)
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptDuallinkIn1);		// no range change
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);			// range change needed
			}
		}
		else
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);				// CSC converted
		}
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, inputXptYUV1);
	}

	// Frame Buffer Disabling
	mCard->WriteRegister(kRegCh2Control, true, kRegMaskChannelDisable, kRegShiftChannelDisable);

	// SDI Out 1
	if (mVirtualDigitalOutput1Select == NTV2_RgbOutputSelect)				// Same as RGB in this case
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);							// 1 3Gb wire
			mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);							// 2 wires
			mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
		}
	}
	else // NTV2_PrimaryOutputSelect													// Primary
	{
		mCard->Connect (NTV2_XptSDIOut1Input, inputXptYUV1);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void Kona1Services::SetDeviceMiscRegisters ()
{	
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();
	
	// special case - VANC 8bit pixel shift support
	if (mVANCMode && Is8BitFrameBufferFormat(mFb1Format) )
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	
	// Set VBlank RGB range bits - ALWAYS SMPTE
	// Except when there is a full-range RGB frame buffer, and we go through the color space converter
	if (mRGB10Range == NTV2_RGB10RangeFull && mVirtualDigitalOutput1Select != NTV2_RgbOutputSelect)
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





