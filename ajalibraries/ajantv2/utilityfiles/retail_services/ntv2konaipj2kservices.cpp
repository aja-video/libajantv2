//
//  ntv2konaipj2kservices.cpp
//
//  Copyright (c) 2017 AJA Video, Inc. All rights reserved.
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
   target = NULL;
}

 KonaIPJ2kServices::~KonaIPJ2kServices()
 {
     if (target)
     {
         delete target;
         target = NULL;
     }
 }

//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void KonaIPJ2kServices::SetDeviceXPointPlayback (GeneralFrameFormat genFrameFormat)
{
	// We need the device ID for KonaIP J2k because there are three flavors of this device
	NTV2DeviceID deviceID = mCard->GetDeviceID();

	if (deviceID == DEVICE_ID_KONAIP_2RX_1SFP_J2K)
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
	DeviceServices::SetDeviceXPointPlayback(genFrameFormat);

	NTV2FrameBufferFormat		fbFormatCh1, fbFormatCh2;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormatCh1);
	mCard->GetFrameBufferFormat(NTV2_CHANNEL2, &fbFormatCh2);

	bool						bLevelBFormat		= IsVideoFormatB(mFb1VideoFormat);
	bool						bStereoOut			= mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect;
	bool						b3GbTransportOut	= (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	int							bCh1Disable			= 0;						// Assume Channel 1 is NOT disabled by default
	int							bCh2Disable			= 1;						// Assume Channel 2 IS disabled by default
	int							bCh3Disable			= 1;						// Assume Channel 3 IS disabled by default
	int							bCh4Disable			= 1;						// Assume Channel 4 IS disabled by default
	bool						bCh2RGB				= IsFrameBufferFormatRGB(fbFormatCh2);
	bool						bDSKGraphicMode		= (mDSKMode == NTV2_DSKModeGraphicOverMatte || mDSKMode == NTV2_DSKModeGraphicOverVideoIn || mDSKMode == NTV2_DSKModeGraphicOverFB);
	bool						bDSKOn				= mDSKMode == NTV2_DSKModeFBOverMatte || mDSKMode == NTV2_DSKModeFBOverVideoIn || (bCh2RGB && bDSKGraphicMode);
	NTV2SDIInputFormatSelect	inputFormatSelect	= mSDIInput1FormatSelect;	// Input format select (YUV, RGB, Stereo 3D)
	NTV2VideoFormat				inputFormat;									// Input video format
	NTV2CrosspointID			inputXptYuv1		= NTV2_XptBlack;			// Input source selected single stream
	NTV2CrosspointID			inputXptYuv2		= NTV2_XptBlack;			// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
	
	// make sure formats/modes match for multibuffer modes
	if (bLevelBFormat || bStereoOut)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, fbFormatCh1);
		bCh2RGB = IsFrameBufferFormatRGB(fbFormatCh1);
	}
	
	// select square division or 2 pixel interleave in frame buffer
	mCard->Set425FrameEnable(false,NTV2_CHANNEL1);

	// Figure out what our input format is based on what is selected 
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat);
	
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
	else
	{
		frameSync2RGB = NTV2_XptLUT1RGB;
	}
	
	
	// CSC 1
	if (genFrameFormat == FORMAT_RGB || bDSKOn)
	{
        mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV);
	}
	
	
	// CSC 2
	if (bCh2RGB)
	{
        mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptLUT2RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2YUV);
	}
	
	
	// CSC 3
	mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	
	// CSC 4
	mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);
	
	// LUT 1
	// note b4K is same processing as regular formats
	if (genFrameFormat == FORMAT_RGB || bDSKOn)
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptFrameBuffer1RGB);
	
		// if RGB-to-RGB apply LUT converter
		if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
		{
			mCard->SetColorCorrectionOutputBank (  NTV2_CHANNEL1,					// NOTE: this conflicts with using AutoCirculate Color Correction!
											mRGB10Range == NTV2_RGB10RangeFull ? 
											kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);	
		}
		else
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
		}	
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);
	}
	
	
	// LUT 2
	if (bCh2RGB)
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptFrameBuffer2RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
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
	
    // DualLink Out 1
	mCard->Connect (NTV2_XptDualLinkOut1Input, frameSync2RGB);

    // DualLink Out 2
	mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptBlack);

    // DualLink Out 3
	mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptBlack);

    // DualLink Out 4
	mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptBlack);
	
	// SDI Out 1
	if (bLevelBFormat || bStereoOut)												// Stereo or LevelB
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbTransportOut ? frameSync2YUV : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)				// RGB Out
	{		
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, b3GbTransportOut ? NTV2_XptDuallinkOut1DS2 : NTV2_XptBlack);
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
	else // NTV2_PrimaryOutputSelect												// Primary
	{
		mCard->Connect (NTV2_XptSDIOut1Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}

	
	// SDI Out 2
	if (bLevelBFormat || bStereoOut)												// Stereo or LevelB
	{
		if (b3GbTransportOut)
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
	else if (mVirtualDigitalOutput2Select == NTV2_DualLinkOutputSelect)				// RGB Out
	{
		mCard->Connect (NTV2_XptSDIOut2Input, b3GbTransportOut ? NTV2_XptDuallinkOut1 : NTV2_XptDuallinkOut1DS2);
		mCard->Connect (NTV2_XptSDIOut2InputDS2, b3GbTransportOut ? NTV2_XptDuallinkOut1DS2 : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput2Select == NTV2_VideoPlusKeySelect)				// Video+Key
	{
		if (bDSKOn)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, b3GbTransportOut ? frameSync1YUV : frameSync2YUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, b3GbTransportOut ? frameSync2YUV : NTV2_XptBlack);
		}
		else if (genFrameFormat == FORMAT_RGB)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, b3GbTransportOut ? NTV2_XptCSC1VidYUV : NTV2_XptCSC1KeyYUV);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, b3GbTransportOut ? NTV2_XptCSC1KeyYUV : NTV2_XptBlack);
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
	else if (bLevelBFormat || bStereoOut)
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
	bool bDSKNeedsInputRef = false;				// Assume we're genlocking to display reference source
	int audioLoopbackMode = 0;					// Assume playback mode. Will be set to '1' if we're in Loopback ("E-E") mode
	bool bNoKey = false;						// Assume we DO have a foreground key

	if (bDSKOn)
	{	
		switch (mDSKMode)
		{
			case NTV2_DSKModeFBOverMatte:
				// Foreground
				if (genFrameFormat == FORMAT_RGB)
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
				if (genFrameFormat == FORMAT_RGB)
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
				
				// Background is Frame Buffer 1
				if (genFrameFormat == FORMAT_RGB)
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

	// Frame Buffer Disabling
	if (bLevelBFormat || bStereoOut)
	{
		bCh1Disable = bCh2Disable = 0; 
	}
	
	mCard->WriteRegister(kRegCh1Control, bCh1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bCh2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);	
	mCard->WriteRegister(kRegCh3Control, bCh3Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh4Control, bCh4Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);

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
void KonaIPJ2kServices::SetDeviceXPointCapture(GeneralFrameFormat genFrameFormat)
{
	// We need the device ID for KonaIP J2k because there are three flavors of this device
	NTV2DeviceID deviceID = mCard->GetDeviceID();

	if (deviceID == DEVICE_ID_KONAIP_2TX_1SFP_J2K)
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
	DeviceServices::SetDeviceXPointCapture(genFrameFormat);

	NTV2VideoFormat				inputFormat      = NTV2_FORMAT_UNKNOWN;
	NTV2RGBRangeMode			frambBufferRange = (mRGB10Range == NTV2_RGB10RangeSMPTE) ? NTV2_RGBRangeSMPTE : NTV2_RGBRangeFull;
	bool						b3GbTransportOut = (mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb);
	bool						bLevelBFormat    = IsVideoFormatB(mFb1VideoFormat);
	bool						bStereoIn   = false;
	int							bCh1Disable = 0;		// Assume Channel 1 is NOT disabled by default
	int							bCh2Disable = 1;		// Assume Channel 2 IS disabled by default

	NTV2CrosspointID			inputXptYUV1 = NTV2_XptBlack;				// Input source selected single stream
	NTV2CrosspointID			inputXptYUV2 = NTV2_XptBlack;				// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
	NTV2SDIInputFormatSelect	inputFormatSelect = NTV2_YUVSelect;				// Input format select (YUV, RGB, Stereo 3D)
	NTV2FrameBufferFormat		fbFormatCh1;

	// frame buffer format
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormatCh1);

	// Figure out what our input format is based on what is selected
	inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat, &inputFormatSelect);
	bool levelBInput = NTV2_IS_3Gb_FORMAT(inputFormat);

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
	if (bLevelBFormat || bStereoIn || m2XTransferMode)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, fbFormatCh1);
	}

	// SMPTE 425
	ULWord vpida           = 0;
	ULWord vpidb           = 0;
	bool b425_2wire        = false;
	bool b425_4wireA       = false;
	bool b425_4wireB       = false;
		
	mCard->ReadSDIInVPID(NTV2_CHANNEL1, vpida, vpidb);
	//debugOut("in  vpida = %08x  vpidb = %08x\n", true, vpida, vpidb);
	CNTV2VPID parser;
	parser.SetVPID(vpida);
	VPIDStandard std = parser.GetStandard();
	b425_2wire  = (std == VPIDStandard_2160_DualLink);
	b425_4wireA = (std == VPIDStandard_2160_QuadLink_3Ga);
	b425_4wireB = (std == VPIDStandard_2160_QuadDualLink_3Gb);

	bool b425 = (b425_2wire || b425_4wireA || b425_4wireB);

	// override inputFormatSelect for SMTE425
	if (b425)
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
	mCard->Set425FrameEnable(b425, NTV2_CHANNEL1);
	

	// SDI In 1
	bool b3GbInEnabled;
	mCard->GetSDIInput3GbPresent(b3GbInEnabled, NTV2_CHANNEL1);
	mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL1, (b3GbInEnabled) || (levelBInput && !bLevelBFormat));
	
	// SDI In 2
	mCard->GetSDIInput3GbPresent(b3GbInEnabled, NTV2_CHANNEL2);
	mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL2, (b3GbInEnabled) || (levelBInput && !bLevelBFormat));
	
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
	
	// CSC 2
	mCard->Connect (NTV2_XptCSC2VidInput, inputXptYUV2);

	// CSC 3
	mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	
	// CSC 4
	mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);

	// LUT 1
	// note b4K processing is same
	if (inputFormatSelect != NTV2_RGBSelect)
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptDuallinkIn1);
		
		// if RGB-to-RGB apply LUT converter
		if (genFrameFormat == FORMAT_RGB)
		{
			mCard->SetColorCorrectionOutputBank (	NTV2_CHANNEL1,					// NOTE: this conflicts with using AutoCirculate Color Correction!
											mSDIInput1RGBRange == NTV2_RGBRangeFull ? 
											kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
		}
		else 
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);	// NOTE: this conflicts with using AutoCirculate Color Correction!
		}	
	}
	
	
	// LUT 2 
	if (inputFormatSelect == NTV2_RGBSelect)
	{
		// provides SMPTE <-> Full conversion
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptDuallinkIn1);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2,						// NOTE: this conflicts with using AutoCirculate Color Correction!
											 mSDIInput1RGBRange == NTV2_RGBRangeFull ?
											 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
	}
	else
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptCSC2VidRGB);
		mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL2, kLUTBank_YUV2RGB);	// NOTE: this conflicts with using AutoCirculate Color Correction!
	}
	
	// LUT 3 
	mCard->Connect (NTV2_XptLUT3Input, NTV2_XptBlack);
	
	// LUT 4 
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
	if (b425)
	{
		if (genFrameFormat == FORMAT_RGB)
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
			else  if (b425_2wire)
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
	if (b425)
	{
		if (genFrameFormat == FORMAT_RGB)
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
	else if (bLevelBFormat || bStereoIn || m2XTransferMode)
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
	if (b425)
	{
		if (genFrameFormat == FORMAT_RGB)
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
	else if (bLevelBFormat || bStereoIn || m2XTransferMode)
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, inputXptYUV2);
	}
	else 
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	}
	
	// Frame Buffer 3
	mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
	
	// Frame Buffer 4
	mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
	
	// Frame Buffer Disabling
	if (bLevelBFormat || bStereoIn || m2XTransferMode)
	{
		bCh1Disable = bCh2Disable = false;
	}
	
	mCard->WriteRegister(kRegCh1Control, bCh1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bCh2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);

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
		if (b3GbTransportOut)
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
	else if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)				// Same as RGB in this case
	{		
		if (b3GbTransportOut)
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
		if (b3GbTransportOut)
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
	else if (mVirtualDigitalOutput2Select == NTV2_DualLinkOutputSelect)				// Same as RGB in this case
	{
		if (b3GbTransportOut)
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
	if (bLevelBFormat || bStereoIn)
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
void KonaIPJ2kServices::SetDeviceMiscRegisters(NTV2Mode mode)
{
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters(mode);

	NTV2Standard			primaryStandard;
	NTV2FrameGeometry		primaryGeometry;
	NTV2FrameBufferFormat   primaryPixelFormat;

	mCard->GetStandard(&primaryStandard);
	mCard->GetFrameGeometry(&primaryGeometry);
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &primaryPixelFormat);
	bool rv, rv2, enable;
	uint32_t enableHw;
	
    if (mCard->IsDeviceReady() == true)
    {
		// We need the device ID for KonaIP J2k because there are three flavors of this device
		NTV2DeviceID deviceID = mCard->GetDeviceID();
		rx_2022_channel		rxHwConfig;
		tx_2022_channel		txHwConfig;
		j2kEncoderConfig	encoderConfig;
		j2kDecoderConfig	decoderConfig;

        if (target == NULL)
        {
            target = new CNTV2Config2022(*mCard);
        }

        // KonaIP network configuration
        string hwIp,hwNet,hwGate;       // current hardware config

		// On J2K IP we just use the top SFP 
        rv = target->GetNetworkConfiguration(SFP_TOP,hwIp,hwNet,hwGate);
        if (rv)
        {
            uint32_t ip, net, gate;
            ip   = inet_addr(hwIp.c_str());
            net  = inet_addr(hwNet.c_str());
            gate = inet_addr(hwGate.c_str());

            if ((ip != mEth0.ipc_ip) || (net != mEth0.ipc_subnet) || (gate != mEth0.ipc_gateway))
            {
                setNetConfig(SFP_TOP);
            }
        }
        else
            printf("GetNetworkConfiguration SFP_TOP - FAILED\n");
		
		// KonaIP input configurations
		// Only config RX for devices that have RX channels
		if ((deviceID == DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
			(deviceID == DEVICE_ID_KONAIP_2RX_1SFP_J2K))
		{
			rv  = target->GetRxChannelConfiguration(NTV2_CHANNEL1,rxHwConfig);
			rv2 = target->GetRxChannelEnable(NTV2_CHANNEL1,enable);
			mCard->ReadRegister(kVRegRxcEnable1, (ULWord*)&enableHw);

			if (rv && rv2)
			{
				if ((enable != enableHw) || notEqualPrimary(rxHwConfig,mRx2022Config1))
				{
					// Special case we handle channel enables at service level automatically
					mRx2022Config1.rxc_enable = enableHw;
					setRxConfig(NTV2_CHANNEL1);
				}
			}
			else
				printf("rxConfig ch 1 config read failed\n");
			
			// Configure j2kDecoder for ch1
			rv  = target->GetJ2KDecoderConfiguration(decoderConfig);
			if (rv)
			{
				
				//printf("j2kDecoder ch 1 config read\n");
				//printDecoderConfig(mRx2022J2kConfig1, decoderConfig);
				
				if (decoderConfig != mRx2022J2kConfig1)
				{
					printf("set j2kDecoder ch 1\n");
					target->SetJ2KDecoderConfiguration(mRx2022J2kConfig1);
				}
			}
			else
				printf("j2kEncoder ch 1 config read failed\n");

			
			
			if (deviceID == DEVICE_ID_KONAIP_2RX_1SFP_J2K)
			{
				rv  = target->GetRxChannelConfiguration(NTV2_CHANNEL2,rxHwConfig);
				rv2 = target->GetRxChannelEnable(NTV2_CHANNEL2,enable);
				mCard->ReadRegister(kVRegRxcEnable2, (ULWord*)&enableHw);

				if (rv && rv2)
				{
					if ((enable != enableHw) || notEqualPrimary(rxHwConfig,mRx2022Config2))
					{
						// Special case we handle channel enables at service level automatically
						mRx2022Config2.rxc_enable = enableHw;
						setRxConfig(NTV2_CHANNEL2);
					}
				}
				else
					printf("rxConfig ch 2 config read failed\n");
			}
		}
		
		// KonaIP output configurations
		// Only config TX for devices that have TX channels
		if ((deviceID == DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
			(deviceID == DEVICE_ID_KONAIP_2TX_1SFP_J2K))
		{
			// Configure tx for ch1
			rv  = target->GetTxChannelConfiguration(NTV2_CHANNEL1,txHwConfig);
			rv2 = target->GetTxChannelEnable(NTV2_CHANNEL1,enable);
			mCard->ReadRegister(kVRegTxcEnable3, (ULWord*)&enableHw);

			if (rv && rv2)
			{
				if ((enable != enableHw) || notEqualPrimary(txHwConfig,mTx2022Config3))
				{
					// Special case we handle channel enables at service level automatically
					mTx2022Config3.txc_enable = enableHw;
					setTxConfig(NTV2_CHANNEL1);
				}
				else
				{
					if (mTx2022Config3.txc_primaryAutoMac)
					{
						uint32_t hi = (txHwConfig.primaryRemoteMAC.mac[0] << 8) + txHwConfig.primaryRemoteMAC.mac[1];
						uint32_t lo =  (txHwConfig.primaryRemoteMAC.mac[2] << 24) + (txHwConfig.primaryRemoteMAC.mac[3] << 16) + (txHwConfig.primaryRemoteMAC.mac[4] << 8) + txHwConfig.primaryRemoteMAC.mac[5];
						mCard->WriteRegister(kVRegTxcPrimaryRemoteMAC_lo3, lo);
						mCard->WriteRegister(kVRegTxcPrimaryRemoteMAC_hi3, hi);
					}
					
					if (mTx2022Config3.txc_secondaryAutoMac)
					{
						uint32_t hi = (txHwConfig.secondaryRemoteMAC.mac[0] << 8) + txHwConfig.secondaryRemoteMAC.mac[1];
						uint32_t lo =  (txHwConfig.secondaryRemoteMAC.mac[2] << 24) + (txHwConfig.secondaryRemoteMAC.mac[3] << 16) + (txHwConfig.secondaryRemoteMAC.mac[4] << 8) + txHwConfig.secondaryRemoteMAC.mac[5];
						mCard->WriteRegister(kVRegTxcSecondaryRemoteMAC_lo3, lo);
						mCard->WriteRegister(kVRegTxcSecondaryRemoteMAC_hi3, hi);
					}
				}
			}
			else
				printf("txConfig ch 1 config read failed\n");
			
			// Configure j2kEncoder for ch1
			rv  = target->GetJ2KEncoderConfiguration(NTV2_CHANNEL1,encoderConfig);
			if (rv)
			{
				// current video format
				mTx2022J2kConfig1.videoFormat = mFb1VideoFormat;
				
				// current bit depth
				mTx2022J2kConfig1.bitDepth = 10;
				if (Is8BitFrameBufferFormat(primaryPixelFormat))
				{
					mTx2022J2kConfig1.bitDepth = 8;
				}
				
				//printf("j2kEncoder ch 1 config read\n");
				//printEncoderConfig(mTx2022J2kConfig1, encoderConfig);
				
				if (encoderConfig != mTx2022J2kConfig1)
				{
					printf("set j2kEncoder ch 1\n");
					target->SetJ2KEncoderConfiguration(NTV2_CHANNEL1,mTx2022J2kConfig1);
				}
			}
			else
				printf("j2kEncoder ch 1 config read failed\n");
			
			if (deviceID == DEVICE_ID_KONAIP_2TX_1SFP_J2K)
			{

				rv  = target->GetTxChannelConfiguration(NTV2_CHANNEL2,txHwConfig);
				rv2 = target->GetTxChannelEnable(NTV2_CHANNEL2,enable);
				mCard->ReadRegister(kVRegTxcEnable4, (ULWord*)&enableHw);

				if (rv && rv2)
				{
					if ((enable != enableHw) || notEqualPrimary(txHwConfig,mTx2022Config4))
					{
						// Special case we handle channel enables at service level automatically
						mTx2022Config4.txc_enable = enableHw;
						setTxConfig(NTV2_CHANNEL2);
					}
					else
					{
						if (mTx2022Config4.txc_primaryAutoMac)
						{
							uint32_t hi = (txHwConfig.primaryRemoteMAC.mac[0] << 8) + txHwConfig.primaryRemoteMAC.mac[1];
							uint32_t lo =  (txHwConfig.primaryRemoteMAC.mac[2] << 24) + (txHwConfig.primaryRemoteMAC.mac[3] << 16) + (txHwConfig.primaryRemoteMAC.mac[4] << 8) + txHwConfig.primaryRemoteMAC.mac[5];
							mCard->WriteRegister(kVRegTxcPrimaryRemoteMAC_lo4, lo);
							mCard->WriteRegister(kVRegTxcPrimaryRemoteMAC_hi4, hi);
						}
						
						if (mTx2022Config4.txc_secondaryAutoMac)
						{
							uint32_t hi = (txHwConfig.secondaryRemoteMAC.mac[0] << 8) + txHwConfig.secondaryRemoteMAC.mac[1];
							uint32_t lo =  (txHwConfig.secondaryRemoteMAC.mac[2] << 24) + (txHwConfig.secondaryRemoteMAC.mac[3] << 16) + (txHwConfig.secondaryRemoteMAC.mac[4] << 8) + txHwConfig.secondaryRemoteMAC.mac[5];
							mCard->WriteRegister(kVRegTxcSecondaryRemoteMAC_lo4, lo);
							mCard->WriteRegister(kVRegTxcSecondaryRemoteMAC_hi4, hi);
						}
					}
				}
				else
					printf("txConfig ch 2 config read failed\n");
				
				// Configure j2kEncoder for ch2
				rv  = target->GetJ2KEncoderConfiguration(NTV2_CHANNEL2,encoderConfig);
				if (rv)
				{
					// current video format
					mTx2022J2kConfig2.videoFormat = mFb1VideoFormat;
					
					// current bit depth
					mTx2022J2kConfig2.bitDepth = 10;
					if (Is8BitFrameBufferFormat(primaryPixelFormat))
					{
						mTx2022J2kConfig2.bitDepth = 8;
					}
					
					//printf("j2kEncoder ch 2 config read\n");
					//printEncoderConfig(mTx2022J2kConfig2, encoderConfig);

					if (encoderConfig != mTx2022J2kConfig2)
					{
						printf("set j2kEncoder ch 2\n");
						target->SetJ2KEncoderConfiguration(NTV2_CHANNEL2,mTx2022J2kConfig2);
					}
				}
				else
					printf("j2kEncoder ch 2 config read failed\n");
			}
		}
    }

	// VPID
	bool					bLevelA = IsVideoFormatA(mFb1VideoFormat);
	bool					b4K = NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	bool					bHfr = NTV2_IS_3G_FORMAT(mFb1VideoFormat);

	bool					bRGBOut = (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect);
	ULWord					vpidOut1a, vpidOut1b, vpidOut2a, vpidOut2b;
	NTV2FrameRate			primaryFrameRate = GetNTV2FrameRateFromVideoFormat(mFb1VideoFormat);
	NTV2VideoFormat			inputFormat = NTV2_FORMAT_UNKNOWN;

	// single wire 3Gb out
	// 1x3Gb = !4k && (rgb | v+k | 3d | (hfra & 3gb) | hfrb)
	bool b1x3Gb =
		((bRGBOut == true) ||
		(mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect) ||
		(mVirtualDigitalOutput1Select == NTV2_StereoOutputSelect) ||
		(bLevelA == true && mDualStreamTransportType == NTV2_SDITransport_DualLink_3Gb) ||
		(IsVideoFormatB(mFb1VideoFormat) == true));

	// all 3Gb transport out
	// b3GbTransportOut = (b1x3Gb + !2wire) | (4k + rgb) | (4khfr + 3gb)
	bool b3GbTransportOut = (b1x3Gb == true && mDualStreamTransportType != NTV2_SDITransport_DualLink_1_5);

	// enable/disable transmission (in/out polarity) for each SDI channel
	if (mode == NTV2_MODE_CAPTURE)
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
				b3GbTransportOut = true;
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
		mCard->SetHDMIV2TsiIO(false);

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
			mCard->SetHDMIV2DecimateMode(decimate); // turning on decimate turns off downconverter
			mCard->SetHDMIV2LevelBMode(NTV2_IS_3Gb_FORMAT(mFb1VideoFormat));
		}
		else
		{
			mCard->SetHDMIOutVideoFPS(primaryFrameRate);
			mCard->SetHDMIV2DecimateMode(false);
			mCard->SetHDMIV2LevelBMode(NTV2_IS_3Gb_FORMAT(mFb1VideoFormat));
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
				mCard->SetHDMISampleStructure(NTV2_HDMI_420);
			else
				mCard->SetHDMISampleStructure(NTV2_HDMI_422);
			break;
		default:
			mCard->SetHDMISampleStructure(NTV2_HDMI_422);
			break;
		}

        mCard->SetHDMIOutVideoStandard(standard);

		// HDMI out colorspace auto-detect status
		mHDMIOutColorSpaceModeStatus = mHDMIOutColorSpaceModeCtrl;
		if (mHDMIOutColorSpaceModeCtrl == kHDMIOutCSCAutoDetect)
		{
			NTV2HDMIBitDepth bitDepth = NTV2_HDMI10Bit;
			NTV2LHIHDMIColorSpace colorSpace = NTV2_LHIHDMIColorSpaceYCbCr;

			mCard->GetHDMIOutDownstreamColorSpace(&colorSpace);
			mCard->GetHDMIOutDownstreamBitDepth(&bitDepth);

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
			mCard->ReadRegister(kRegHDMIInputStatus, &detectedProtocol, kLHIRegMaskHDMIOutputEDIDDVI);
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
	if (mVANCMode && Is8BitFrameBufferFormat(primaryPixelFormat))
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
	mCard->GetLHIVideoDACMode(&curr2Mode);
	mCard->GetLHIVideoDACStandard(&curr2Standard);

	// Select DAC mode from framebufferformat
	new2Mode = GetLHIVideoDACMode(mFb1VideoFormat, mVirtualAnalogOutputType, mVirtualAnalogOutBlackLevel);
	new2Standard = primaryStandard;

	// write it only if the new value is different
	if (curr2Mode != new2Mode)
		mCard->SetLHIVideoDACMode(new2Mode);
	if (curr2Standard != new2Standard)
		mCard->SetLHIVideoDACStandard(new2Standard);


	//
	// SDI Out 1
	//

	// is 2K frame buffer geometry, includes 4K mode
	bool b2KFbGeom = NTV2_IS_2K_1080_FRAME_GEOMETRY(primaryGeometry) || primaryGeometry == NTV2_FG_4x2048x1080;
	NTV2Standard transportStandard = b3GbTransportOut && bHfr ? NTV2_STANDARD_1080 : primaryStandard;

	// Select primary standard
	mCard->SetSDIOut2Kx1080Enable(NTV2_CHANNEL1, b2KFbGeom);
	mCard->SetSDIOutputStandard(NTV2_CHANNEL1, transportStandard);
	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL1, bLevelA && b3GbTransportOut);

	// 3Ga / 3Gb / Neither
	if (b3GbTransportOut)
	{
		mCard->SetSDIOut3GEnable(NTV2_CHANNEL1, true);
		mCard->SetSDIOut3GbEnable(NTV2_CHANNEL1, true);
	}
	else
	{
		mCard->SetSDIOut3GEnable(NTV2_CHANNEL1, bLevelA);
		mCard->SetSDIOut3GbEnable(NTV2_CHANNEL1, false);
	}

	// Set VPID 1
	if (b4K)
	{
		SetVPIDData(vpidOut1a, mFb1VideoFormat, bRGBOut, false, b3GbTransportOut, false, VPIDChannel_1);
		if (b3GbTransportOut)
		{
			SetVPIDData(vpidOut1b, mFb1VideoFormat, bRGBOut, false, b3GbTransportOut, false, VPIDChannel_2);
		}
	}


	//
	// SDI Out 2
	//

	// Select primary standard
	mCard->SetSDIOut2Kx1080Enable(NTV2_CHANNEL2, b2KFbGeom);
	mCard->SetSDIOutputStandard(NTV2_CHANNEL2, transportStandard);
	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL2, bLevelA && b3GbTransportOut);

	// 3Ga / 3Gb / Neither
	if (b3GbTransportOut)
	{
		mCard->SetSDIOut3GEnable(NTV2_CHANNEL2, true);
		mCard->SetSDIOut3GbEnable(NTV2_CHANNEL2, true);
	}
	else
	{
		mCard->SetSDIOut3GEnable(NTV2_CHANNEL2, bLevelA);
		mCard->SetSDIOut3GbEnable(NTV2_CHANNEL2, false);
	}

	// Set VPID 2
	if (b4K)
	{
		if (b3GbTransportOut)
		{
			SetVPIDData(vpidOut2a, mFb1VideoFormat, bRGBOut, false, b3GbTransportOut, false, VPIDChannel_3);
			SetVPIDData(vpidOut2b, mFb1VideoFormat, bRGBOut, false, b3GbTransportOut, false, VPIDChannel_4);
		}
		else
		{
			SetVPIDData(vpidOut2a, mFb1VideoFormat, bRGBOut, false, b3GbTransportOut, false, VPIDChannel_2);
		}
	}
	
	
	// Finish VPID for SDI Out 1-4 Out
	{
		// don't overwrite if e-to-e and input and outputs match
		ULWord overwrite =	!(	(mode == NTV2_MODE_CAPTURE) &&
								((mVirtualInputSelect == NTV2_DualLinkInputSelect && bRGBOut == true) ||
								 (mVirtualInputSelect != NTV2_DualLinkInputSelect && bRGBOut != true)   ));
		
		// enable overwrite
		if (b4K)
		{
			mCard->WriteRegister(kRegSDIOut1Control, overwrite, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite);
			mCard->WriteRegister(kRegSDIOut2Control, overwrite, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite);
		}
		
		// enable VPID write
		if (b4K)
		{
			mCard->WriteRegister(kRegSDIOut1Control, 1, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable);
			mCard->WriteRegister(kRegSDIOut2Control, 1, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable);
		}

		// write VPID value
		if (b4K)
		{
			// write VPID for SDI 1
			//debugOut("out vpid = %08x  %08x  %08x  %08x  %08x  %08x  %08x %08x\n", true, vpidOut1a, vpidOut1b, vpidOut2a, vpidOut2b, vpidOut3a, vpidOut3b, vpidOut4a, vpidOut4b);
			mCard->WriteRegister(kRegSDIOut1VPIDA, vpidOut1a);
			if (b3GbTransportOut)
				mCard->WriteRegister(kRegSDIOut1VPIDB, vpidOut1b);
			
			// write VPID for SDI 2
			mCard->WriteRegister(kRegSDIOut2VPIDA, vpidOut2a);
			if (b3GbTransportOut)
				mCard->WriteRegister(kRegSDIOut2VPIDB, vpidOut2b);
		}

	}
	
	// Set HBlack RGB range bits - ALWAYS SMPTE
	mCard->WriteRegister(kRegSDIOut1Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	mCard->WriteRegister(kRegSDIOut2Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);

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

	// audio input delay
	ULWord inputDelay = 0;			// not from hardware
	mCard->ReadRegister(kVRegAudioInputDelay, &inputDelay);
	uint32_t offset = GetAudioDelayOffset(inputDelay / 10.0);	// scaled by a factor of 10
	mCard->WriteRegister(kRegAud1Delay, offset, kRegMaskAudioInDelay, kRegShiftAudioInDelay);

	// audio output delay
	ULWord outputDelay = 0;			// not from hardware
	mCard->ReadRegister(kVRegAudioOutputDelay, &outputDelay);
	offset = AUDIO_DELAY_WRAPAROUND - GetAudioDelayOffset(outputDelay / 10.0);	// scaled by a factor of 10
	mCard->WriteRegister(kRegAud1Delay, offset, kRegMaskAudioOutDelay, kRegShiftAudioOutDelay);
}

//-------------------------------------------------------------------------------------------------------
//	Support Routines
//-------------------------------------------------------------------------------------------------------
void  KonaIPJ2kServices::setNetConfig(eSFP  port)
{
    printf("set NetConfig port=%d\n",(int)port);

    string ip,sub,gate;
    struct in_addr addr;

    switch (port)
    {
    case SFP_BOTTOM:
        addr.s_addr = mEth1.ipc_ip;
        ip = inet_ntoa(addr);
        addr.s_addr = mEth1.ipc_subnet;
        sub = inet_ntoa(addr);
        addr.s_addr = mEth1.ipc_gateway;
        gate = inet_ntoa(addr);
        break;
    case SFP_TOP:
    default:
        addr.s_addr = mEth0.ipc_ip;
        ip = inet_ntoa(addr);
        addr.s_addr = mEth0.ipc_subnet;
        sub = inet_ntoa(addr);
        addr.s_addr = mEth0.ipc_gateway;
        gate = inet_ntoa(addr);
        break;
    }
    target->SetNetworkConfiguration(port,ip,sub,gate);
}

void   KonaIPJ2kServices::setRxConfig(NTV2Channel channel)
{
	printf("set RxConfig chn=%d\n",(int)channel);
	
	rx_2022_channel chan;
	struct in_addr addr;
	bool enable;
	switch ((int)channel)
	{
		case NTV2_CHANNEL2:
			addr.s_addr                 = mRx2022Config2.rxc_primarySourceIp;
			chan.primarySourceIP        = inet_ntoa(addr);
			addr.s_addr                 = mRx2022Config2.rxc_primaryDestIp;
			chan.primaryDestIP          = inet_ntoa(addr);;
			chan.primaryRxMatch         = mRx2022Config2.rxc_primaryRxMatch;
			chan.primarySourcePort      = mRx2022Config2.rxc_primarySourcePort;
			chan.primaryDestPort        = mRx2022Config2.rxc_primaryDestPort;
			chan.primarySsrc            = mRx2022Config2.rxc_primarySsrc;
			chan.primaryVlan            = mRx2022Config2.rxc_primaryVlan;
			
			addr.s_addr                 = mRx2022Config2.rxc_secondarySourceIp;
			chan.secondarySourceIP      = inet_ntoa(addr);
			addr.s_addr                 = mRx2022Config2.rxc_secondaryDestIp;
			chan.secondaryDestIP        = inet_ntoa(addr);;
			chan.secondaryRxMatch       = mRx2022Config2.rxc_secondaryRxMatch;
			chan.secondarySourcePort    = mRx2022Config2.rxc_secondarySourcePort;
			chan.secondaryDestPort      = mRx2022Config2.rxc_secondaryDestPort;
			chan.secondarySsrc          = mRx2022Config2.rxc_secondarySsrc;
			chan.secondaryVlan          = mRx2022Config2.rxc_secondaryVlan;
			
			chan.networkPathDiff        = mRx2022Config2.rxc_networkPathDiff;
			chan.playoutDelay           = mRx2022Config2.rxc_playoutDelay;
			
			enable                      = mRx2022Config2.rxc_enable;
			break;
		default:
		case NTV2_CHANNEL1:
			addr.s_addr                 = mRx2022Config1.rxc_primarySourceIp;
			chan.primarySourceIP        = inet_ntoa(addr);
			addr.s_addr                 = mRx2022Config1.rxc_primaryDestIp;
			chan.primaryDestIP          = inet_ntoa(addr);;
			chan.primaryRxMatch         = mRx2022Config1.rxc_primaryRxMatch;
			chan.primarySourcePort      = mRx2022Config1.rxc_primarySourcePort;
			chan.primaryDestPort        = mRx2022Config1.rxc_primaryDestPort;
			chan.primarySsrc            = mRx2022Config1.rxc_primarySsrc;
			chan.primaryVlan            = mRx2022Config1.rxc_primaryVlan;
			
			addr.s_addr                 = mRx2022Config1.rxc_secondarySourceIp;
			chan.secondarySourceIP      = inet_ntoa(addr);
			addr.s_addr                 = mRx2022Config1.rxc_secondaryDestIp;
			chan.secondaryDestIP        = inet_ntoa(addr);;
			chan.secondaryRxMatch       = mRx2022Config1.rxc_secondaryRxMatch;
			chan.secondarySourcePort    = mRx2022Config1.rxc_secondarySourcePort;
			chan.secondaryDestPort      = mRx2022Config1.rxc_secondaryDestPort;
			chan.secondarySsrc          = mRx2022Config1.rxc_secondarySsrc;
			chan.secondaryVlan          = mRx2022Config1.rxc_secondaryVlan;
			
			chan.networkPathDiff        = mRx2022Config1.rxc_networkPathDiff;
			chan.playoutDelay           = mRx2022Config1.rxc_playoutDelay;
			
			enable                      = mRx2022Config1.rxc_enable;
			break;
	}
	
	target->SetRxChannelConfiguration(channel,chan);
	target->SetRxChannelEnable(channel,enable,m2022_7Mode);
}

void  KonaIPJ2kServices::setTxConfig(NTV2Channel channel)
{
	printf("set TxConfig chn=%d\n",(int)channel);
	tx_2022_channel chan;
	MACAddr remoteMAC;
	struct in_addr addr;
	bool enable;
	
	switch((int)channel)
	{
		case NTV2_CHANNEL2:
			addr.s_addr                 = mTx2022Config4.txc_primaryRemoteIp;
			chan.primaryRemoteIP        = inet_ntoa(addr);
			remoteMAC.mac[0]            = (mTx2022Config4.txc_primaryRemoteMAC_hi >> 8)  & 0xff;
			remoteMAC.mac[1]            =  mTx2022Config4.txc_primaryRemoteMAC_hi        & 0xff;
			remoteMAC.mac[2]            = (mTx2022Config4.txc_primaryRemoteMAC_lo >> 24) & 0xff;
			remoteMAC.mac[3]            = (mTx2022Config4.txc_primaryRemoteMAC_lo >> 16) & 0xff;
			remoteMAC.mac[4]            = (mTx2022Config4.txc_primaryRemoteMAC_lo >> 8)  & 0xff;
			remoteMAC.mac[5]            =  mTx2022Config4.txc_primaryRemoteMAC_lo        & 0xff;
			chan.primaryRemoteMAC       = remoteMAC;
			chan.primaryAutoMAC         = mTx2022Config4.txc_primaryAutoMac;
			chan.primaryLocalPort       = mTx2022Config4.txc_primaryLocalPort;
			chan.primaryRemotePort      = mTx2022Config4.txc_primaryRemotePort;
			
			addr.s_addr                 = mTx2022Config4.txc_secondaryRemoteIp;
			chan.secondaryRemoteIP      = inet_ntoa(addr);
			remoteMAC.mac[0]            = (mTx2022Config4.txc_secondaryRemoteMAC_hi >> 8)  & 0xff;
			remoteMAC.mac[1]            =  mTx2022Config4.txc_secondaryRemoteMAC_hi        & 0xff;
			remoteMAC.mac[2]            = (mTx2022Config4.txc_secondaryRemoteMAC_lo >> 24) & 0xff;
			remoteMAC.mac[3]            = (mTx2022Config4.txc_secondaryRemoteMAC_lo >> 16) & 0xff;
			remoteMAC.mac[4]            = (mTx2022Config4.txc_secondaryRemoteMAC_lo >> 8)  & 0xff;
			remoteMAC.mac[5]            =  mTx2022Config4.txc_secondaryRemoteMAC_lo        & 0xff;
			chan.secondaryRemoteMAC     = remoteMAC;
			chan.secondaryAutoMAC       = mTx2022Config4.txc_secondaryAutoMac;
			chan.secondaryLocalPort     = mTx2022Config4.txc_secondaryLocalPort;
			chan.secondaryRemotePort    = mTx2022Config4.txc_secondaryRemotePort;
			
			enable                      = mTx2022Config4.txc_enable;
			break;
		default:
			
		case NTV2_CHANNEL1:
			addr.s_addr                 = mTx2022Config3.txc_primaryRemoteIp;
			chan.primaryRemoteIP        = inet_ntoa(addr);
			remoteMAC.mac[0]            = (mTx2022Config3.txc_primaryRemoteMAC_hi >> 8)  & 0xff;
			remoteMAC.mac[1]            =  mTx2022Config3.txc_primaryRemoteMAC_hi        & 0xff;
			remoteMAC.mac[2]            = (mTx2022Config3.txc_primaryRemoteMAC_lo >> 24) & 0xff;
			remoteMAC.mac[3]            = (mTx2022Config3.txc_primaryRemoteMAC_lo >> 16) & 0xff;
			remoteMAC.mac[4]            = (mTx2022Config3.txc_primaryRemoteMAC_lo >> 8)  & 0xff;
			remoteMAC.mac[5]            =  mTx2022Config3.txc_primaryRemoteMAC_lo        & 0xff;
			chan.primaryRemoteMAC       = remoteMAC;
			chan.primaryAutoMAC         = mTx2022Config3.txc_primaryAutoMac;
			chan.primaryLocalPort       = mTx2022Config3.txc_primaryLocalPort;
			chan.primaryRemotePort      = mTx2022Config3.txc_primaryRemotePort;
			
			addr.s_addr                 = mTx2022Config3.txc_secondaryRemoteIp;
			chan.secondaryRemoteIP      = inet_ntoa(addr);
			remoteMAC.mac[0]            = (mTx2022Config3.txc_secondaryRemoteMAC_hi >> 8)  & 0xff;
			remoteMAC.mac[1]            =  mTx2022Config3.txc_secondaryRemoteMAC_hi        & 0xff;
			remoteMAC.mac[2]            = (mTx2022Config3.txc_secondaryRemoteMAC_lo >> 24) & 0xff;
			remoteMAC.mac[3]            = (mTx2022Config3.txc_secondaryRemoteMAC_lo >> 16) & 0xff;
			remoteMAC.mac[4]            = (mTx2022Config3.txc_secondaryRemoteMAC_lo >> 8)  & 0xff;
			remoteMAC.mac[5]            =  mTx2022Config3.txc_secondaryRemoteMAC_lo        & 0xff;
			chan.secondaryRemoteMAC     = remoteMAC;
			chan.secondaryAutoMAC       = mTx2022Config3.txc_secondaryAutoMac;
			chan.secondaryLocalPort     = mTx2022Config3.txc_secondaryLocalPort;
			chan.secondaryRemotePort    = mTx2022Config3.txc_secondaryRemotePort;
			
			enable                      = mTx2022Config3.txc_enable;
			break;
	}
	target->SetTxChannelConfiguration(channel,chan);
	target->SetTxChannelEnable(channel,enable,m2022_7Mode);
}

bool  KonaIPJ2kServices::notEqualPrimary(const rx_2022_channel & hw_channel, const rx2022Config & virtual_config)
{
	uint32_t addr;
	
	if (virtual_config.rxc_primarySourcePort != hw_channel.primarySourcePort)return true;
	if (virtual_config.rxc_primaryDestPort   != hw_channel.primaryDestPort)  return true;
	if (virtual_config.rxc_primaryRxMatch    != hw_channel.primaryRxMatch)   return true;
	
	addr = inet_addr(hw_channel.primaryDestIP.c_str());
	if (virtual_config.rxc_primaryDestIp     != addr) return true;
	
	addr = inet_addr(hw_channel.primarySourceIP.c_str());
	if (virtual_config.rxc_primarySourceIp   != addr) return true;
	
	return false;
}

bool  KonaIPJ2kServices::notEqualPrimary(const tx_2022_channel & hw_channel, const tx2022Config & virtual_config)
{
	uint32_t addr;
	if (virtual_config.txc_primaryLocalPort    != hw_channel.primaryLocalPort)  return true;
	if (virtual_config.txc_primaryRemotePort   != hw_channel.primaryRemotePort) return true;
	if (virtual_config.txc_primaryAutoMac      != hw_channel.primaryAutoMAC)    return true;
	
	addr = inet_addr(hw_channel.primaryRemoteIP.c_str());
	if (virtual_config.txc_primaryRemoteIp     != addr) return true;
	
	// don't compare automac, but if it is false, do compare the mac addresses
	if (virtual_config.txc_primaryAutoMac == false)
	{
		// only examine mac when automac is off
		if (notEqualMAC(virtual_config.txc_primaryRemoteMAC_lo,virtual_config.txc_primaryRemoteMAC_hi,hw_channel.primaryRemoteMAC)) return true;
	}
	
	return false;
}

bool  KonaIPJ2kServices::notEqualMAC(uint32_t lo, uint32_t hi, const MACAddr & macaddr)
{
	if (macaddr.mac[0]   != ((hi >> 8 ) & 0xff)) return true;
	if (macaddr.mac[1]   != ( hi        & 0xff)) return true;
	if (macaddr.mac[2]   != ((lo >> 24) & 0xff)) return true;
	if (macaddr.mac[3]   != ((lo >> 16) & 0xff)) return true;
	if (macaddr.mac[4]   != ((lo >> 8)  & 0xff)) return true;
	if (macaddr.mac[5]   != ( lo        & 0xff)) return true;
	
	return false;
}

void KonaIPJ2kServices::printEncoderConfig(j2kEncoderConfig modelConfig, j2kEncoderConfig encoderConfig)
{
	printf("videoFormat	   %6d%6d\n", modelConfig.videoFormat, encoderConfig.videoFormat);
	printf("ullMode		   %6d%6d\n", modelConfig.ullMode, encoderConfig.ullMode);
	printf("bitDepth	   %6d%6d\n", modelConfig.bitDepth, encoderConfig.bitDepth);
	printf("chromaSubsamp  %6d%6d\n", modelConfig.chromaSubsamp, encoderConfig.chromaSubsamp);
	printf("mbps		   %6d%6d\n", modelConfig.mbps, encoderConfig.mbps);
	printf("streamType	   %6d%6d\n", modelConfig.streamType, encoderConfig.streamType);
	printf("pmtPid		   %6d%6d\n", modelConfig.pmtPid, encoderConfig.pmtPid);
	printf("videoPid	   %6d%6d\n", modelConfig.videoPid, encoderConfig.videoPid);
	printf("pcrPid		   %6d%6d\n", modelConfig.pcrPid, encoderConfig.pcrPid);
	printf("audio1Pid	   %6d%6d\n", modelConfig.audio1Pid, encoderConfig.audio1Pid);
}

void KonaIPJ2kServices::printDecoderConfig(j2kDecoderConfig modelConfig, j2kDecoderConfig encoderConfig)
{
	printf("selectionMode  %6d%6d\n", modelConfig.selectionMode, encoderConfig.selectionMode);
	printf("programNumber  %6d%6d\n", modelConfig.programNumber, encoderConfig.programNumber);
	printf("programPID	   %6d%6d\n", modelConfig.programPID, encoderConfig.programPID);
	printf("audioNumber    %6d%6d\n", modelConfig.audioNumber, encoderConfig.audioNumber);
}
