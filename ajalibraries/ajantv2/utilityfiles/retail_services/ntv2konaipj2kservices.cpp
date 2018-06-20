//
//  ntv2konaipj2kservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2konaipj2kservices.h"
#if defined (AJALinux) || defined (AJAMac)
	#include <stdlib.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

using namespace std;


//-------------------------------------------------------------------------------------------------------
//	class KonaIPJ2kServices
//-------------------------------------------------------------------------------------------------------

KonaIPJ2kServices::KonaIPJ2kServices()
{
   config = NULL;
}

 KonaIPJ2kServices::~KonaIPJ2kServices()
 {
     if (config != NULL)
     {
         delete config;
         config = NULL;
     }
 }


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void KonaIPJ2kServices::SetDeviceXPointPlayback ()
{
	// We need the device ID for KonaIP J2k because there are three flavors of this device
	if (mDeviceID == DEVICE_ID_KONAIP_2RX_1SFP_J2K)
	{
		// no output for KIPJ2k2rx, should not be here
		mCard->SetDefaultVideoOutMode(kDefaultModeVideoIn);
		return;
	}
	
	// J2K products have unidirectional input and output that share same frame buffer so override any user setting here and force
	// output enable and input disable because we are in playback mode
	mCard->WriteRegister(kVRegRxcEnable1, false);
	mCard->WriteRegister(kVRegRxcEnable2, false);
	mCard->WriteRegister(kVRegTxcEnable3, true);
	mCard->WriteRegister(kVRegTxcEnable4, true);
	
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback();
	
	bool 						bFb1RGB 			= IsRGBFormat(mFb1Format);
	bool						b2FbLevelBHfr		= IsVideoFormatB(mFb1VideoFormat);
	bool						bStereoOut			= mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect;
	bool						b3GbOut				= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	int							bFb1Disable			= 0;						// Assume Channel 1 is NOT disabled by default
	int							bFb2Disable			= 1;						// Assume Channel 2 IS disabled by default
	int							bFb3Disable			= 1;						// Assume Channel 3 IS disabled by default
	int							bFb4Disable			= 1;						// Assume Channel 4 IS disabled by default
	bool						bFb2RGB				= IsRGBFormat(mFb2Format);
	bool						bDSKGraphicMode		= (mDSKMode == NTV2_DSKModeGraphicOverMatte || mDSKMode == NTV2_DSKModeGraphicOverVideoIn || mDSKMode == NTV2_DSKModeGraphicOverFB);
	bool						bDSKOn				= mDSKMode == NTV2_DSKModeFBOverMatte || mDSKMode == NTV2_DSKModeFBOverVideoIn || (bFb2RGB && bDSKGraphicMode);
	NTV2SDIInputFormatSelect	inputFormatSelect	= mSDIInput1FormatSelect;	// Input format select (YUV, RGB, Stereo 3D)
	NTV2CrosspointID			inputXptYuv1		= NTV2_XptBlack;			// Input source selected single stream
	NTV2CrosspointID			inputXptYuv2		= NTV2_XptBlack;			// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
	
	// make sure formats/modes match for multibuffer modes
	if (b2FbLevelBHfr || bStereoOut)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
		bFb2RGB = IsRGBFormat(mFb1Format);
	}
	
	// select square division or 2 pixel interleave in frame buffer
	mCard->Set425FrameEnable(false,NTV2_CHANNEL1);

	// Figure out what our input format is based on what is selected 
	GetSelectedInputVideoFormat(mFb1VideoFormat);
	
	// input 1 select
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		inputXptYuv1 = NTV2_XptSDIIn1;
		inputXptYuv2 = NTV2_XptSDIIn1DS2;
	}
	// input 2 select
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		inputXptYuv1 = NTV2_XptSDIIn2;
		inputXptYuv2 = NTV2_XptSDIIn2DS2;
	}
	// dual link select
	else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
	{
		inputXptYuv1 = NTV2_XptSDIIn1;
		inputXptYuv2 = NTV2_XptSDIIn2;
	}
	
	// Dual Link In 1
	if (inputFormatSelect == NTV2_RGBSelect)
	{
		mCard->Connect (NTV2_XptDualLinkIn1Input, inputXptYuv1);
		mCard->Connect (NTV2_XptDualLinkIn1DSInput, inputXptYuv2);
		
		inputXptYuv1 = NTV2_XptCSC1VidYUV;
		inputXptYuv2 = NTV2_XptBlack;
	}
	else
	{
		mCard->Connect (NTV2_XptDualLinkIn1Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptDualLinkIn1DSInput, NTV2_XptBlack);
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
	else
	{
		frameSync2RGB = NTV2_XptLUT1RGB;
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
	if (bFb2RGB)
	{
        mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptLUT2RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2YUV);
	}
	
	
	// CSC 3 and 4
	mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);
	
	// LUT 1
	// note b4K is same processing as regular formats
	if (bFb1RGB || bDSKOn)
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptFrameBuffer1RGB);
	
		// if RGB-to-RGB apply LUT converter
		if (mVirtualDigitalOutput1Select == NTV2_RgbOutputSelect)
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
	if (bFb2RGB)
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptFrameBuffer2RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);
	}
	else
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptCSC2VidRGB);
		mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL2, kLUTBank_YUV2RGB);
	}
	
	
	// LUT 3
	mCard->Connect (NTV2_XptLUT3Input, NTV2_XptBlack);
	
	// LUT 4
	mCard->Connect (NTV2_XptLUT4Input, NTV2_XptBlack);


	// Frame Buffer 1
	mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_XptBlack);
	
	// Frame Buffer 2
	mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_XptBlack);
	
	// Frame Buffer 3
	mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer3BInput, NTV2_XptBlack);
	
	// Frame Buffer 4
	mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer4BInput, NTV2_XptBlack);
		
	// 4K Down Converter
	mCard->Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptBlack);
	mCard->Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptBlack);
	
	// DualLink Out 1, 2, 3, 4
	mCard->Connect (NTV2_XptDualLinkOut1Input, frameSync2RGB);
	mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptBlack);
	
	// SDI Out 1
	if (b2FbLevelBHfr || bStereoOut)												// Stereo or LevelB
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbOut ? frameSync2YUV : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_RgbOutputSelect)				// RGB Out
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
	else // NTV2_PrimaryOutputSelect												// Primary
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}

	
	// SDI Out 2
	if (b2FbLevelBHfr || bStereoOut)												// Stereo or LevelB
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
	else if (mVirtualDigitalOutput2Select == NTV2_RgbOutputSelect)				// RGB Out
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
	else // NTV2_PrimaryOutputSelect												// Primary
	{
		mCard->Connect (NTV2_XptSDIOut2Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
	}

	// HDMI Out
	NTV2CrosspointID XPt1 = NTV2_XptBlack;
	NTV2CrosspointID XPt2 = NTV2_XptBlack;
	NTV2CrosspointID XPt3 = NTV2_XptBlack;
	NTV2CrosspointID XPt4 = NTV2_XptBlack;
	
	if (bDSKOn)
	{
		XPt1 = frameSync1YUV;
	}
	else if (b2FbLevelBHfr || bStereoOut)
	{
		// Stereo or LevelB
		XPt1 = NTV2_XptLUT1RGB;
		XPt2 = NTV2_XptLUT2RGB;
	}
	else
	{
        XPt1 = NTV2_XptLUT1RGB;
	}

	mCard->Connect (NTV2_XptHDMIOutInput, XPt1);
	mCard->Connect (NTV2_XptHDMIOutQ2Input, XPt2);
	mCard->Connect (NTV2_XptHDMIOutQ3Input, XPt3);
	mCard->Connect (NTV2_XptHDMIOutQ4Input, XPt4);

	// 4K Hdmi-to-Hdmi Bypass always disabled for playback
	mCard->WriteRegister(kRegHDMIOutControl, false, kRegMaskHDMIV2TxBypass, kRegShiftHDMIV2TxBypass);
	
	// Analog Out
	mCard->Connect (NTV2_XptAnalogOutInput, frameSync1YUV);
	
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
				if (mVirtualInputSelect == NTV2_Input1Select)
				{
					// Select input 1 (0x01)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
				}
				else if (mVirtualInputSelect == NTV2_Input2Select)
				{
					// Select input 2 (0x02)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn2);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn2);
				}
				else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
				{
					// Select dual link (0x83)
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
					// Select input 1 (0x01)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn1);
				}
				else if (mVirtualInputSelect == NTV2_Input2Select)
				{
					// Select input 2 (0x02)
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptSDIIn2);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptSDIIn2);
				}
				else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
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
				
				// Background is Frame Buffer 1
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

	// Frame Buffer Disabling
	if (b2FbLevelBHfr || bStereoOut)
	{
		bFb1Disable = bFb2Disable = 0; 
	}
	
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);	
	mCard->WriteRegister(kRegCh3Control, bFb3Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh4Control, bFb4Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);

	// connect muxes
	mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptBlack);

	mCard->Connect(NTV2_Xpt425Mux3AInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux3BInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux4AInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux4BInput, NTV2_XptBlack);
}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void KonaIPJ2kServices::SetDeviceXPointCapture()
{
	// We need the device ID for KonaIP J2k because there are three flavors of this device
	if (mDeviceID == DEVICE_ID_KONAIP_2TX_1SFP_J2K)
	{
		// no input for KIPJ2k2tx, should not be here
		mCard->SetDefaultVideoOutMode(kDefaultModeTestPattern);
		return;
	}
	
	// J2K products have unidirectional input and output that share same frame buffer so override any user setting here and force
	// input enable and output disable because we are in capture mode
	mCard->WriteRegister(kVRegRxcEnable1, true);
	mCard->WriteRegister(kVRegRxcEnable2, true);
	mCard->WriteRegister(kVRegTxcEnable3, false);
	mCard->WriteRegister(kVRegTxcEnable4, false);

	// call superclass first
	DeviceServices::SetDeviceXPointCapture();

	NTV2VideoFormat				inputFormat      	= NTV2_FORMAT_UNKNOWN;
	NTV2RGBRangeMode			frambBufferRange 	= (mRGB10Range == NTV2_RGB10RangeSMPTE) ? NTV2_RGBRangeSMPTE : NTV2_RGBRangeFull;
	bool 						bFb1RGB 			= IsRGBFormat(mFb1Format);
	bool						b3GbOut				= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	bool						b2FbLevelBHfr    	= IsVideoFormatB(mFb1VideoFormat);
	bool						bStereoIn   		= false;
	int							bFb1Disable 		= 0;		// Assume Channel 1 is NOT disabled by default
	int							bFb2Disable 		= 1;		// Assume Channel 2 IS disabled by default

	NTV2CrosspointID			inputXptYUV1 		= NTV2_XptBlack;				// Input source selected single stream
	NTV2CrosspointID			inputXptYUV2 		= NTV2_XptBlack;				// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
	NTV2SDIInputFormatSelect	inputFormatSelect 	= NTV2_YUVSelect;				// Input format select (YUV, RGB, Stereo 3D)

	// Figure out what our input format is based on what is selected
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat, &inputFormatSelect);
	bool inHfrB = IsVideoFormatB(inputFormat);

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
	// dual link select
	else if (mVirtualInputSelect == NTV2_DualLinkInputSelect)
	{
		inputXptYUV1 = NTV2_XptSDIIn1;
		inputXptYUV2 = NTV2_XptSDIIn2;
	}

	// is stereo in?
	bStereoIn = inputFormatSelect == NTV2_Stereo3DSelect;

	// make sure formats/modes match for multibuffer modes
	if (b2FbLevelBHfr || bStereoIn)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
	}

	// SMPTE 425 (2pi)
	ULWord vpida		= 0;
	ULWord vpidb		= 0;
	bool b2x2piIn		= false;
	bool b4x2piInA		= false;
	bool b4x2piInB		= false;
		
	mCard->ReadSDIInVPID(NTV2_CHANNEL1, vpida, vpidb);
	//debugOut("in  vpida = %08x  vpidb = %08x\n", true, vpida, vpidb);
	CNTV2VPID parser;
	parser.SetVPID(vpida);
	VPIDStandard std = parser.GetStandard();
	b2x2piIn  = (std == VPIDStandard_2160_DualLink);
	b4x2piInA = (std == VPIDStandard_2160_QuadLink_3Ga);
	b4x2piInB = (std == VPIDStandard_2160_QuadDualLink_3Gb);

	bool b2piIn = (b2x2piIn || b4x2piInA || b4x2piInB);

	// override inputFormatSelect for SMTE425
	if (b2piIn)
	{
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

	// select square division or 2 pixel interleave in frame buffer
	mCard->Set425FrameEnable(b2piIn, NTV2_CHANNEL1);
	

	// SDI In 1
	bool b3GbInEnabled;
	mCard->GetSDIInput3GbPresent(b3GbInEnabled, NTV2_CHANNEL1);
	mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL1, 
		(b3GbInEnabled) || (inHfrB && !b2FbLevelBHfr && (mVirtualInputSelect==NTV2_Input1Select)));
	
	// SDI In 2
	mCard->GetSDIInput3GbPresent(b3GbInEnabled, NTV2_CHANNEL2);
	mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL2, 
		(b3GbInEnabled) || (inHfrB && !b2FbLevelBHfr && (mVirtualInputSelect==NTV2_Input1Select)));
	
	// SDI In 3
	mCard->GetSDIInput3GbPresent(b3GbInEnabled, NTV2_CHANNEL3);
	mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL3, b3GbInEnabled);
	
	// SDI In 4
	mCard->GetSDIInput3GbPresent(b3GbInEnabled, NTV2_CHANNEL4);
	mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL4, b3GbInEnabled);
	
	
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
	
	// Dual Link In 2
	mCard->Connect (NTV2_XptDualLinkIn2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptDualLinkIn2DSInput, NTV2_XptBlack);
	
	// Dual Link In 3
	mCard->Connect (NTV2_XptDualLinkIn3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptDualLinkIn3DSInput, NTV2_XptBlack);
	
	// Dual Link In 4
	mCard->Connect (NTV2_XptDualLinkIn4Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptDualLinkIn4DSInput, NTV2_XptBlack);
	
	// CSC 1
	if (inputFormatSelect != NTV2_RGBSelect)
	{
		mCard->Connect (NTV2_XptCSC1VidInput, inputXptYUV1);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}
	
	// CSC 2, 3, 4
	mCard->Connect (NTV2_XptCSC2VidInput, inputXptYUV2);
	mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);

	// LUT 1
	// note b4K processing is same
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
	
	
	// LUT 2 
	if (inputFormatSelect == NTV2_RGBSelect)
	{
		// provides SMPTE <-> Full conversion
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptDuallinkIn1);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2,	
											 mSDIInput1RGBRange == NTV2_RGBRangeFull ?
											 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
	}
	else
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptCSC2VidRGB);
		mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL2, kLUTBank_YUV2RGB);
	}
	
	// LUT 3, 4
	mCard->Connect (NTV2_XptLUT3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptLUT4Input, NTV2_XptBlack);

	// Dual Link Out 1,2,3,4 3 Out
	if (inputFormat == mFb1VideoFormat)
	{
		// Input is NOT secondary
	
		if (inputFormatSelect != NTV2_RGBSelect)
		{
			mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptLUT1RGB);
			mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptLUT1RGB);
		}
		else
		{
			if (mSDIInput1RGBRange == mSDIOutput1RGBRange && mLUTType != NTV2_LUTCustom)
			{
				// no range change
				mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptDuallinkIn1);
				mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptDuallinkIn1);
			}
			else
			{
				mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptLUT1RGB);
				mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptLUT1RGB);
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
			mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptLUT1RGB);
			mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptLUT1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptDuallinkIn1);
			mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptDuallinkIn1);
		}
	}
	
	
	// 425 mux
	if (b2piIn)
	{
		if (bFb1RGB)
		{
			if (inputFormatSelect == NTV2_RGBSelect)
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptDuallinkIn1);
				mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptDuallinkIn2);
				mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptDuallinkIn3);
				mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptDuallinkIn4);
			}
			else
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptLUT1RGB);
				mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptLUT2RGB);
				mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptLUT3Out);
				mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptLUT4Out);
			}
		}
		else
		{
			if (inputFormatSelect == NTV2_RGBSelect)
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidYUV);
				mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptCSC2VidYUV);
				mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptCSC3VidYUV);
				mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptCSC4VidYUV);
			}
			else  if (b2x2piIn)
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptSDIIn1);
				mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptSDIIn1DS2);
				mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptSDIIn2);
				mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptSDIIn2DS2);

				mCard->Connect(NTV2_Xpt425Mux3AInput, NTV2_Xpt425Mux1AYUV);
				mCard->Connect(NTV2_Xpt425Mux3BInput, NTV2_Xpt425Mux1BYUV);
				mCard->Connect(NTV2_Xpt425Mux4AInput, NTV2_Xpt425Mux2AYUV);
				mCard->Connect(NTV2_Xpt425Mux4BInput, NTV2_Xpt425Mux2BYUV);
			
			}
			else
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptSDIIn1);
				mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptSDIIn2);
				mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptSDIIn3);
				mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptSDIIn4);
			}
		}
	}
	else
	{
		mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptBlack);

		mCard->Connect(NTV2_Xpt425Mux3AInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux3BInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux4AInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux4BInput, NTV2_XptBlack);
	}

	// Frame Buffer 1
	if (b2piIn)
	{
		if (bFb1RGB)
		{
			mCard->Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1ARGB);
			mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BRGB);
		}
		else
		{
			mCard->Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1AYUV);
			mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BYUV);
		}
	}
	else if (b2FbLevelBHfr || bStereoIn)
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
	

	
	// Frame Buffer 2
	if (b2piIn)
	{
		if (bFb1RGB)
		{
			mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2ARGB);
			mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BRGB);
		}
		else
		{
			mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV);
			mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BYUV);
		}
	}
	else if (b2FbLevelBHfr || bStereoIn)
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, inputXptYUV2);
	}
	else 
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	}
	
	// Frame Buffer 3, 4
	mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
	
	// Frame Buffer Disabling
	if (b2FbLevelBHfr || bStereoIn)
	{
		bFb1Disable = bFb2Disable = false;
	}
	
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);

	// 4K Down Converter
	mCard->Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptBlack);
	mCard->Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptBlack);


	// SDI Out 1
	mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	
	
	// SDI Out 2
	mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);


	// SDI Out 3 - acts like SDI 1
	if(IsVideoFormatB(mFb1VideoFormat) ||												// Dual Stream - p60b
		mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect ||					// Stereo 3D
		mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)						// Video + Key
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, inputXptYUV1);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, inputXptYUV2);
		}
		else 
		{
			mCard->Connect (NTV2_XptSDIOut3Input, inputXptYUV1);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
		}
	}
	else if (mVirtualDigitalOutput1Select == NTV2_RgbOutputSelect)				// Same as RGB in this case
	{		
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);							// 1 3Gb wire
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptDuallinkOut3DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);							// 2 wires
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
		}
	}
	else // NTV2_PrimaryOutputSelect													// Primary
	{
		mCard->Connect (NTV2_XptSDIOut3Input, inputXptYUV1);
		mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
	}

	// SDI Out 4 - acts like SDI 2
	if (IsVideoFormatB(mFb1VideoFormat) ||												// Dual Stream - p60b
		mVirtualDigitalOutput2Select == NTV2_StereoOutputSelect ||					// Stereo 3D
		mVirtualDigitalOutput2Select == NTV2_VideoPlusKeySelect)						// Video + Key
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, inputXptYUV1);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, inputXptYUV2);
		}
		else 
		{
			mCard->Connect (NTV2_XptSDIOut4Input, inputXptYUV2);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
	}
	else if (mVirtualDigitalOutput2Select == NTV2_RgbOutputSelect)				// Same as RGB in this case
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut4);							// 1 3Gb wire
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptDuallinkOut4DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut3DS2);						// 2 wires
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
	}
	else // NTV2_PrimaryOutputSelect													// Primary
	{
		mCard->Connect (NTV2_XptSDIOut4Input, inputXptYUV1);
		mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
	}
	
	// HDMI Out
	NTV2CrosspointID XPt1 = NTV2_XptBlack;
	NTV2CrosspointID XPt2 = NTV2_XptBlack;
	NTV2CrosspointID XPt3 = NTV2_XptBlack;
	NTV2CrosspointID XPt4 = NTV2_XptBlack;
	if (b2FbLevelBHfr || bStereoIn)
	{
		// Stereo or LevelB
		XPt1 = NTV2_XptLUT1RGB;
		XPt2 = NTV2_XptLUT2RGB;
	}
	else
	{
		XPt1 = NTV2_XptLUT1RGB;
	}
	mCard->Connect (NTV2_XptHDMIOutInput, XPt1);
	mCard->Connect (NTV2_XptHDMIOutQ2Input, XPt2);
	mCard->Connect (NTV2_XptHDMIOutQ3Input, XPt3);
	mCard->Connect (NTV2_XptHDMIOutQ4Input, XPt4);

	// Analog Out
	mCard->Connect (NTV2_XptAnalogOutInput, inputXptYUV1);
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void KonaIPJ2kServices::SetDeviceMiscRegisters()
{
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();

	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	bool					rv, rv2, enableChCard;
	uint32_t				enableChServices;
    uint32_t                configErr;
	
	mCard->GetStandard(primaryStandard);
	mCard->GetFrameGeometry(primaryGeometry);

    if (mCard->IsDeviceReady(true) == true)
    {
		// We need the device ID for KonaIP J2k because there are three flavors of this device
		rx_2022_channel		rxHwConfig;
		tx_2022_channel		txHwConfig;
		j2kEncoderConfig	encoderConfig;
		j2kDecoderConfig	decoderConfig;

		if (config == NULL)
		{
			config = new CNTV2Config2022(*mCard);
            config->SetIPServicesControl(true, false);

		}

        bool    ipServiceEnable;
        bool    ipServiceForceConfig;

        config->GetIPServicesControl(ipServiceEnable, ipServiceForceConfig);
        if (ipServiceEnable)
        {		
            // KonaIP network configuration
            string hwIp,hwNet,hwGate;       // current hardware config

            // On J2K IP we just use the top SFP 
            rv = config->GetNetworkConfiguration(SFP_1,hwIp,hwNet,hwGate);
            if (rv)
            {
                uint32_t ip, net, gate;
                ip   = inet_addr(hwIp.c_str());
                net  = inet_addr(hwNet.c_str());
                gate = inet_addr(hwGate.c_str());

                if ((ip != mEth0.ipc_ip) || (net != mEth0.ipc_subnet) || (gate != mEth0.ipc_gateway))
                {
                    SetNetConfig(config, SFP_1);
                }
            }
            else
                printf("GetNetworkConfiguration SFP_TOP - FAILED\n");
            
            // KonaIP input configurations
            // Only config RX for devices that have RX channels
            if ((mDeviceID == DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
                (mDeviceID == DEVICE_ID_KONAIP_2RX_1SFP_J2K))
            {
                if (IsValidConfig(mRx2022Config1, false))
                {
                    rv  = config->GetRxChannelConfiguration(NTV2_CHANNEL1, rxHwConfig);
                    rv2 = config->GetRxChannelEnable(NTV2_CHANNEL1, enableChCard);
                    mCard->ReadRegister(kVRegRxcEnable1, enableChServices);
                    if (rv && rv2)
                    {
                        // if the channel enable toggled
                        if (enableChCard != (enableChServices ? true : false))
                        {
                            config->SetRxChannelEnable(NTV2_CHANNEL1, false);
                            
                            // if the channel is enabled
                            if (enableChServices)
                            {
                                SetRxConfig(config, NTV2_CHANNEL1, false);
                                GetIPError(NTV2_CHANNEL1,kErrRxConfig,configErr);
                                if (!configErr)
                                {
                                    // configure the decoder everytime we configure the RX channel
                                    printf("set j2kDecoder ch 1\n");
                                    mRx2022J2kConfig1.selectionMode = j2kDecoderConfig::eProgSel_AutoFirstProg;
                                    mRx2022J2kConfig1.programNumber = 1;
                                    mRx2022J2kConfig1.programPID = 1;
                                    mRx2022J2kConfig1.audioNumber = 1;
                                    config->SetJ2KDecoderConfiguration(mRx2022J2kConfig1);
                                    
                                    // enable the channel
                                    config->SetRxChannelEnable(NTV2_CHANNEL1, true);
                                }
                                
                            }
                        }
                        // if the channel is already enabled then check to see if a configuration has changed
                        else if (enableChServices)
                        {
                            if (NotEqual(rxHwConfig, mRx2022Config1, false))
                            {
                                config->SetRxChannelEnable(NTV2_CHANNEL1, false);
                                SetRxConfig(config, NTV2_CHANNEL1, false);
                                GetIPError(NTV2_CHANNEL1,kErrRxConfig,configErr);
                                if (!configErr)
                                {
                                    // configure the decoder everytime we configure the RX channel
                                    printf("set j2kDecoder ch 1\n");
                                    mRx2022J2kConfig1.selectionMode = j2kDecoderConfig::eProgSel_AutoFirstProg;
                                    mRx2022J2kConfig1.programNumber = 1;
                                    mRx2022J2kConfig1.programPID = 1;
                                    mRx2022J2kConfig1.audioNumber = 1;
                                    config->SetJ2KDecoderConfiguration(mRx2022J2kConfig1);
                                    
                                    // enable the channel
                                    config->SetRxChannelEnable(NTV2_CHANNEL1, true);
                                }
                            }
                        }
                    }
                    else printf("rxConfig ch 1 read failed\n");
                }
                else SetIPError(NTV2_CHANNEL1,kErrRxConfig,NTV2IpErrInvalidConfig);


                if (mDeviceID == DEVICE_ID_KONAIP_2RX_1SFP_J2K)
                {
                    if (IsValidConfig(mRx2022Config2, false))
                    {
                        rv  = config->GetRxChannelConfiguration(NTV2_CHANNEL2, rxHwConfig);
                        rv2 = config->GetRxChannelEnable(NTV2_CHANNEL2, enableChCard);
                        mCard->ReadRegister(kVRegRxcEnable2, enableChServices);
                        if (rv && rv2)
                        {
                            // if the channel enable toggled
                            if (enableChCard != (enableChServices ? true : false))
                            {
                                config->SetRxChannelEnable(NTV2_CHANNEL2, false);
                                
                                // if the channel is enabled
                                if (enableChServices)
                                {
                                    SetRxConfig(config, NTV2_CHANNEL2, false);
                                    GetIPError(NTV2_CHANNEL2,kErrRxConfig,configErr);
                                    if (!configErr)
                                    {
                                        // configure the decoder everytime we configure the RX channel
                                        printf("set j2kDecoder ch 2\n");
                                        mRx2022J2kConfig2.selectionMode = j2kDecoderConfig::eProgSel_AutoFirstProg;
                                        mRx2022J2kConfig2.programNumber = 1;
                                        mRx2022J2kConfig2.programPID = 1;
                                        mRx2022J2kConfig2.audioNumber = 1;
                                        config->SetJ2KDecoderConfiguration(mRx2022J2kConfig2);
                                        
                                        // enable the channel
                                        config->SetRxChannelEnable(NTV2_CHANNEL2, true);
                                    }
                                    
                                }
                            }
                            // if the channel is already enabled then check to see if a configuration has changed
                            else if (enableChServices)
                            {
                                if (NotEqual(rxHwConfig, mRx2022Config2, false))
                                {
                                    config->SetRxChannelEnable(NTV2_CHANNEL2, false);
                                    SetRxConfig(config, NTV2_CHANNEL2, false);
                                    GetIPError(NTV2_CHANNEL2,kErrRxConfig,configErr);
                                    if (!configErr)
                                    {
                                        // configure the decoder everytime we configure the RX channel
                                        printf("set j2kDecoder ch 2\n");
                                        mRx2022J2kConfig2.selectionMode = j2kDecoderConfig::eProgSel_AutoFirstProg;
                                        mRx2022J2kConfig2.programNumber = 1;
                                        mRx2022J2kConfig2.programPID = 1;
                                        mRx2022J2kConfig2.audioNumber = 1;
                                        config->SetJ2KDecoderConfiguration(mRx2022J2kConfig2);
                                        
                                        // enable the channel
                                        config->SetRxChannelEnable(NTV2_CHANNEL2, true);
                                    }
                                }
                            }
                        }
                        else printf("rxConfig ch 2 read failed\n");
                    }
                    else SetIPError(NTV2_CHANNEL2,kErrRxConfig,NTV2IpErrInvalidConfig);
                }
            }
            
            // KonaIP output configurations
            // Only config TX for devices that have TX channels
            if ((mDeviceID == DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
                (mDeviceID == DEVICE_ID_KONAIP_2TX_1SFP_J2K))
            {
                if (IsValidConfig(mTx2022Config3, false))
                {
                    rv  = config->GetTxChannelConfiguration(NTV2_CHANNEL1, txHwConfig);
                    rv2 = config->GetTxChannelEnable(NTV2_CHANNEL1, enableChCard);
                    GetIPError(NTV2_CHANNEL1,kErrTxConfig,configErr);
                    mCard->ReadRegister(kVRegTxcEnable3, enableChServices);
                    if (rv && rv2)
                    {
                        // if the channel enable toggled
                        if (enableChCard != (enableChServices ? true : false))
                        {
                            config->SetTxChannelEnable(NTV2_CHANNEL1, false);
                            
                            // if the channel is enabled
                            if (enableChServices)
                            {
                                SetTxConfig(config, NTV2_CHANNEL1, false);
                                GetIPError(NTV2_CHANNEL1,kErrTxConfig,configErr);
                                if (!configErr)
                                {
                                    config->SetTxChannelEnable(NTV2_CHANNEL1, true);
                                }
                            }
                        }
                        // if the channel is already enabled then check to see if a configuration has changed
                        else if (enableChServices)
                        {
                            if (NotEqual(txHwConfig, mTx2022Config3, false) || configErr)
                            {
                                config->SetTxChannelEnable(NTV2_CHANNEL1, false);
                                SetTxConfig(config, NTV2_CHANNEL1, false);
                                GetIPError(NTV2_CHANNEL1,kErrTxConfig,configErr);
                                if (!configErr)
                                {
                                    config->SetTxChannelEnable(NTV2_CHANNEL1, true);
                                }
                            }
                        }
                    }
                    else printf("txConfig ch 1 read failed\n");
                    
                    // Configure j2kEncoder for ch1
                    rv  = config->GetJ2KEncoderConfiguration(NTV2_CHANNEL1, encoderConfig);
                    if (rv)
                    {
                        // current video format
                        mTx2022J2kConfig1.videoFormat = mFb1VideoFormat;
                        
                        // current bit depth
                        mTx2022J2kConfig1.bitDepth = 10;
                        if (Is8BitFrameBufferFormat(mFb1Format))
                        {
                            mTx2022J2kConfig1.bitDepth = 8;
                        }
                        // Force this, other settings dont work
                        mTx2022J2kConfig1.chromaSubsamp =  kJ2KChromaSubSamp_422_Standard;
                        
                        
                        //printf("j2kEncoder ch 1 config read\n");
                        //PrintEncoderConfig(mTx2022J2kConfig1, encoderConfig);
                        if (encoderConfig != mTx2022J2kConfig1)
                        {
                            printf("set j2kEncoder ch 1\n");
                            //PrintEncoderConfig(mTx2022J2kConfig1, encoderConfig);
                            config->SetJ2KEncoderConfiguration(NTV2_CHANNEL1, mTx2022J2kConfig1);
                        }
                    }
                    else printf("j2kEncoder ch 1 read failed\n");
                }
                else SetIPError(NTV2_CHANNEL1,kErrTxConfig,NTV2IpErrInvalidConfig);

                
                if (mDeviceID == DEVICE_ID_KONAIP_2TX_1SFP_J2K)
                {
                    if (IsValidConfig(mTx2022Config4, false))
                    {
                        rv  = config->GetTxChannelConfiguration(NTV2_CHANNEL2, txHwConfig);
                        rv2 = config->GetTxChannelEnable(NTV2_CHANNEL2, enableChCard);
                        GetIPError(NTV2_CHANNEL2,kErrTxConfig,configErr);
                        mCard->ReadRegister(kVRegTxcEnable4, enableChServices);
                        if (rv && rv2)
                        {
                            // if the channel enable toggled
                            if (enableChCard != (enableChServices ? true : false))
                            {
                                config->SetTxChannelEnable(NTV2_CHANNEL2, false);
                                
                                // if the channel is enabled
                                if (enableChServices)
                                {
                                    SetTxConfig(config, NTV2_CHANNEL2, false);
                                    GetIPError(NTV2_CHANNEL2,kErrTxConfig,configErr);
                                    if (!configErr)
                                    {
                                        config->SetTxChannelEnable(NTV2_CHANNEL2, true);
                                    }
                                }
                            }
                            // if the channel is already enabled then check to see if a configuration has changed
                            else if (enableChServices)
                            {
                                if (NotEqual(txHwConfig, mTx2022Config4, false) || configErr)
                                {
                                    config->SetTxChannelEnable(NTV2_CHANNEL2, false);
                                    SetTxConfig(config, NTV2_CHANNEL2, false);
                                    GetIPError(NTV2_CHANNEL2,kErrTxConfig,configErr);
                                    if (!configErr)
                                    {
                                        config->SetTxChannelEnable(NTV2_CHANNEL2, true);
                                    }
                                }
                            }
                        }
                        else printf("txConfig ch 2 read failed\n");
                        
                        // Configure j2kEncoder for ch2
                        rv  = config->GetJ2KEncoderConfiguration(NTV2_CHANNEL2, encoderConfig);
                        if (rv)
                        {
                            // current video format
                            mTx2022J2kConfig2.videoFormat = mFb1VideoFormat;
                            
                            // current bit depth
                            mTx2022J2kConfig2.bitDepth = 10;
                            if (Is8BitFrameBufferFormat(mFb1Format))
                            {
                                mTx2022J2kConfig2.bitDepth = 8;
                            }
                            // Force this, other settings dont work
                            mTx2022J2kConfig2.chromaSubsamp =  kJ2KChromaSubSamp_422_Standard;
                            
                            //printf("j2kEncoder ch 2 config read\n");
                            //PrintEncoderConfig(mTx2022J2kConfig2, encoderConfig);
                            if (encoderConfig != mTx2022J2kConfig2)
                            {
                                printf("set j2kEncoder ch 2\n");
                                //PrintEncoderConfig(mTx2022J2kConfig2, encoderConfig);
                                config->SetJ2KEncoderConfiguration(NTV2_CHANNEL2, mTx2022J2kConfig2);
                            }
                        }
                        else printf("j2kEncoder ch 2 read failed\n");
                    }
                    else SetIPError(NTV2_CHANNEL2,kErrTxConfig,NTV2IpErrInvalidConfig);
                }
            }
		}
    }

	// VPID
	bool					bFbLevelA = IsVideoFormatA(mFb1VideoFormat);
	bool					b4K = NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	//bool					bHfr = NTV2_IS_3G_FORMAT(mFb1VideoFormat);

	bool					bSdiOutRGB = (mVirtualDigitalOutput1Select == NTV2_RgbOutputSelect);
	NTV2FrameRate			primaryFrameRate = GetNTV2FrameRateFromVideoFormat(mFb1VideoFormat);

	// single wire 3Gb out
	// 1x3Gb = !4k && (rgb | v+k | 3d | (hfra & 3gb) | hfrb)
	bool b1x3GbOut =
		((bSdiOutRGB == true) ||
		(mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect) ||
		(mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect) ||
		(bFbLevelA == true && mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb) ||
		(IsVideoFormatB(mFb1VideoFormat) == true));

	// all 3Gb transport out
	// b3GbOut = (b1x3GbOut + !2wire) | (4k + rgb) | (4khfr + 3gb)
	bool b3GbOut = (b1x3GbOut == true && mDualStreamTransportType != NTV2_SDITransport_DualLink_1_5);

	// enable/disable transmission (in/out polarity) for each SDI channel
	if (mFb1Mode == NTV2_MODE_CAPTURE)
	{
		ULWord vpida = 0;
		ULWord vpidb = 0;
		mCard->ReadSDIInVPID(NTV2_CHANNEL1, vpida, vpidb);

		if (mCard->ReadSDIInVPID(NTV2_CHANNEL1, vpida, vpidb))
		{
			CNTV2VPID parser;
			parser.SetVPID(vpida);
			VPIDStandard std = parser.GetStandard();
			switch (std)
			{
			case VPIDStandard_2160_DualLink:
				b3GbOut = true;
				break;
			case VPIDStandard_2160_QuadLink_3Ga:
			case VPIDStandard_2160_QuadDualLink_3Gb:
				break;
			default:
				break;
			}
		}

		mCard->SetSDITransmitEnable(NTV2_CHANNEL1, false);
		mCard->SetSDITransmitEnable(NTV2_CHANNEL2, false);
	}
	else
	{
		mCard->SetSDITransmitEnable(NTV2_CHANNEL1, true);
		mCard->SetSDITransmitEnable(NTV2_CHANNEL2, true);
	}

	// HDMI output - initialization sequence
	if (mHDMIStartupCountDown > 0)
	{
		// start initialization
		if (mHDMIStartupCountDown == kHDMIStartupPhase0)
			mCard->WriteRegister(kRegHDMIOutControl, 0x0, 0x0F000000);

		else if (mHDMIStartupCountDown == kHDMIStartupPhase1)
			mCard->WriteRegister(kRegHDMIOutControl, 0xC, 0x0F000000);

		else if (mHDMIStartupCountDown == kHDMIStartupPhase2)
			mCard->WriteRegister(kRegHDMIOutControl, 0xD, 0x0F000000);

		else if (mHDMIStartupCountDown == kHDMIStartupPhase3)
			mCard->WriteRegister(kRegHDMIOutControl, 0xC, 0x0F000000);

		mHDMIStartupCountDown--;
	}
	else
	{
		// set standard / mode
        NTV2Standard standard = GetNTV2StandardFromVideoFormat(mFb1VideoFormat);
		NTV2FrameRate rate = GetNTV2FrameRateFromVideoFormat(mFb1VideoFormat);

		if (b4K == true)
		{
			// 4K mode
			switch (mVirtualHDMIOutputSelect)
			{
			default:
			case NTV2_4kHalfFrameRate:
			case NTV2_PrimaryOutputSelect:
				mCard->SetHDMIV2Mode(NTV2_HDMI_V2_4K_PLAYBACK);
				break;
			case NTV2_Quarter4k:
			case NTV2_Quadrant1Select:
			case NTV2_Quadrant2Select:
			case NTV2_Quadrant3Select:
			case NTV2_Quadrant4Select:
			{
				mCard->SetHDMIV2Mode(NTV2_HDMI_V2_HDSD_BIDIRECTIONAL);
                switch (standard)
				{
				case NTV2_STANDARD_3840x2160p:
				case NTV2_STANDARD_3840HFR:
                    standard = NTV2_STANDARD_1080p;
					break;
				case NTV2_STANDARD_4096x2160p:
				case NTV2_STANDARD_4096HFR:
                    standard = NTV2_STANDARD_2Kx1080p;
					break;
				default:
					break;
				}
			}
			break;
			};
		}
		else
		{
			// SD or HD mode
			mCard->SetHDMIV2Mode(NTV2_HDMI_V2_HDSD_BIDIRECTIONAL);
            switch (standard)
			{
			case NTV2_STANDARD_3840x2160p:
			case NTV2_STANDARD_3840HFR:
                standard = NTV2_STANDARD_1080p;
				break;
			case NTV2_STANDARD_4096x2160p:
			case NTV2_STANDARD_4096HFR:
                standard = NTV2_STANDARD_2Kx1080p;
				break;
			case NTV2_STANDARD_1080:
				switch (rate)
				{
				case NTV2_FRAMERATE_2398:
				case NTV2_FRAMERATE_2400:
                    standard = NTV2_STANDARD_1080p;
					break;
				default:
					break;
				}
			default:
				break;
			}
		}

		// disable two sample interleave i/o
		mCard->SetHDMIOutTsiIO(false);

		// set fps
		if (mVirtualHDMIOutputSelect == NTV2_4kHalfFrameRate)
		{
			//Only do this for formats that half rate supported
			//50,5994,60
			NTV2FrameRate tempRate = primaryFrameRate;
			bool decimate = false;

			switch (primaryFrameRate)
			{
			case NTV2_FRAMERATE_6000:
			case NTV2_FRAMERATE_5994:
			case NTV2_FRAMERATE_5000:
			case NTV2_FRAMERATE_4800:
			case NTV2_FRAMERATE_4795:
				tempRate = HalfFrameRate(primaryFrameRate);
				decimate = true;
				break;
			default:
				break;
			}

            switch (standard)
			{
			case NTV2_STANDARD_4096HFR:
                standard = NTV2_STANDARD_4096x2160p;
				break;
			case NTV2_STANDARD_3840HFR:
                standard = NTV2_STANDARD_3840x2160p;
				break;
			default:
				break;
			}
			mCard->SetHDMIOutVideoFPS(tempRate);
			mCard->SetHDMIOutDecimateMode(decimate); // turning on decimate turns off downconverter
			mCard->SetHDMIOutLevelBMode(IsVideoFormatB(mFb1VideoFormat));
		}
		else
		{
			mCard->SetHDMIOutVideoFPS(primaryFrameRate);
			mCard->SetHDMIOutDecimateMode(false);
			mCard->SetHDMIOutLevelBMode(IsVideoFormatB(mFb1VideoFormat));
		}

		// color space sample rate
		switch (primaryFrameRate)
		{
		case NTV2_FRAMERATE_6000:
		case NTV2_FRAMERATE_5994:
		case NTV2_FRAMERATE_5000:
		case NTV2_FRAMERATE_4800:
		case NTV2_FRAMERATE_4795:
			if (b4K == true && mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect)
				mCard->SetHDMIOutSampleStructure(NTV2_HDMI_420);
			else
				mCard->SetHDMIOutSampleStructure(NTV2_HDMI_422);
			break;
		default:
			mCard->SetHDMIOutSampleStructure(NTV2_HDMI_422);
			break;
		}

        mCard->SetHDMIOutVideoStandard(standard);

		// HDMI out colorspace auto-detect status
		mHDMIOutColorSpaceModeStatus = mHDMIOutColorSpaceModeCtrl;
		if (mHDMIOutColorSpaceModeCtrl == kHDMIOutCSCAutoDetect)
		{
			NTV2HDMIBitDepth bitDepth = NTV2_HDMI10Bit;
			NTV2LHIHDMIColorSpace colorSpace = NTV2_LHIHDMIColorSpaceYCbCr;

			mCard->GetHDMIOutDownstreamColorSpace(colorSpace);
			mCard->GetHDMIOutDownstreamBitDepth(bitDepth);

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
			mCard->SetLHIHDMIOutColorSpace(NTV2_LHIHDMIColorSpaceYCbCr);
			mCard->SetHDMIOutBitDepth(NTV2_HDMI10Bit);
			break;

		case kHDMIOutCSCRGB10bit:
			mCard->SetLHIHDMIOutColorSpace(NTV2_LHIHDMIColorSpaceRGB);
			mCard->SetHDMIOutBitDepth(NTV2_HDMI10Bit);
			break;

		default:
		case kHDMIOutCSCRGB8bit:
			mCard->SetLHIHDMIOutColorSpace(NTV2_LHIHDMIColorSpaceRGB);
			mCard->SetHDMIOutBitDepth(NTV2_HDMI8Bit);
			break;
		}

		// HDMI Out Protocol mode
		switch (mHDMIOutProtocolMode)
		{
		default:
		case kHDMIOutProtocolAutoDetect:
		{
			ULWord detectedProtocol;
			mCard->ReadRegister(kRegHDMIInputStatus, detectedProtocol, kLHIRegMaskHDMIOutputEDIDDVI);
			mCard->WriteRegister(kRegHDMIOutControl, detectedProtocol, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
		}
		break;

		case kHDMIOutProtocolHDMI:
			mCard->WriteRegister(kRegHDMIOutControl, NTV2_HDMIProtocolHDMI, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
			break;

		case kHDMIOutProtocolDVI:
			mCard->WriteRegister(kRegHDMIOutControl, NTV2_HDMIProtocolDVI, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
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
	
	// 4K Down Converter
	bool bPsf = IsPSF(mFb1VideoFormat);
	mCard->Enable4KDCPSFInMode(bPsf);
	mCard->Enable4KPSFOutMode(bPsf);


	// special case - VANC 8bit pixel shift support
	if (mVANCMode && Is8BitFrameBufferFormat(mFb1Format))
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);

	// Figure out what our input format is based on what is selected
	GetSelectedInputVideoFormat(mFb1VideoFormat);

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
	mCard->GetLHIVideoDACMode(curr2Mode);
	mCard->GetLHIVideoDACStandard(curr2Standard);

	// Select DAC mode from framebufferformat
	new2Mode = GetLHIVideoDACMode(mFb1VideoFormat, mVirtualAnalogOutputType, mVirtualAnalogOutBlackLevel);
	new2Standard = primaryStandard;

	// write it only if the new value is different
	if (curr2Mode != new2Mode)
		mCard->SetLHIVideoDACMode(new2Mode);
	if (curr2Standard != new2Standard)
		mCard->SetLHIVideoDACStandard(new2Standard);


	//
	// SDI Out
	//
	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL1, bFbLevelA && b3GbOut);

	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL2, bFbLevelA && b3GbOut);

	// Set HBlack RGB range bits - ALWAYS SMPTE
	mCard->WriteRegister(kRegSDIOut1Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	mCard->WriteRegister(kRegSDIOut2Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);

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

	// audio input delay
	ULWord inputDelay = 0;			// not from hardware
	mCard->ReadRegister(kVRegAudioInputDelay, inputDelay);
	uint32_t offset = GetAudioDelayOffset(inputDelay / 10.0);	// scaled by a factor of 10
	mCard->WriteRegister(kRegAud1Delay, offset, kRegMaskAudioInDelay, kRegShiftAudioInDelay);

	// audio output delay
	ULWord outputDelay = 0;			// not from hardware
	mCard->ReadRegister(kVRegAudioOutputDelay, outputDelay);
	offset = AUDIO_DELAY_WRAPAROUND - GetAudioDelayOffset(outputDelay / 10.0);	// scaled by a factor of 10
	mCard->WriteRegister(kRegAud1Delay, offset, kRegMaskAudioOutDelay, kRegShiftAudioOutDelay);
}
