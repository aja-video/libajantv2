//
//  ntv2konahdmiservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2konahdmiservices.h"


//-------------------------------------------------------------------------------------------------------
//	class Corvid44Services
//-------------------------------------------------------------------------------------------------------

KonaHDMIServices::KonaHDMIServices()
{
}


//-------------------------------------------------------------------------------------------------------
//	GetSelectedInputVideoFormat
//	Note:	Determine input video format based on input select and fbVideoFormat
//			which currently is videoformat of ch1-framebuffer
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat KonaHDMIServices::GetSelectedInputVideoFormat(
											NTV2VideoFormat fbVideoFormat,
											NTV2SDIInputFormatSelect* inputFormatSelect)
{
	(void)fbVideoFormat;
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	if (inputFormatSelect)
		*inputFormatSelect = NTV2_YUVSelect;
	
	// Figure out what our input format is based on what is selected
    switch (mVirtualInputSelect)
    {
		case NTV2_Input1Select:
		case NTV2_Input2Select:
		case NTV2_Input3Select:
		case NTV2_Input4Select:
			{
				inputFormat = mCard->GetHDMIInputVideoFormat((NTV2Channel)mVirtualInputSelect);
			
				NTV2LHIHDMIColorSpace hdmiInputColor;
				mCard->GetHDMIInputColor(hdmiInputColor, (NTV2Channel)mVirtualInputSelect);

				if (inputFormatSelect)
					*inputFormatSelect = (hdmiInputColor == NTV2_LHIHDMIColorSpaceYCbCr) ? NTV2_YUVSelect : NTV2_RGBSelect;
			}
			break;

		default:
			break;
	}
	
	return inputFormat;
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void KonaHDMIServices::SetDeviceXPointPlayback ()
{
	// no output for KonaHDMI
	mCard->SetDefaultVideoOutMode(kDefaultModeVideoIn);
}

	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void KonaHDMIServices::SetDeviceXPointCapture ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();

	//NTV2RGBRangeMode			frambBufferRange	= (mRGB10Range == NTV2_RGB10RangeSMPTE) ? NTV2_RGBRangeSMPTE : NTV2_RGBRangeFull;
	bool 						bFb1RGB 			= IsRGBFormat(mFb1Format);
	bool						b4K					= NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);

	NTV2CrosspointID			inputXpt1		= NTV2_XptBlack;
	NTV2CrosspointID			inputXpt2		= NTV2_XptBlack;
	NTV2CrosspointID			inputXpt3		= NTV2_XptBlack;
	NTV2CrosspointID			inputXpt4		= NTV2_XptBlack;
	NTV2SDIInputFormatSelect	inputFormatSelect	= NTV2_YUVSelect;				// Input format select (YUV, RGB, Stereo 3D)
	
	NTV2LHIHDMIColorSpace hdmiInputColor;
	mCard->GetHDMIInputColor(hdmiInputColor, (NTV2Channel)mVirtualInputSelect);
	if(hdmiInputColor == NTV2_LHIHDMIColorSpaceYCbCr)
		inputFormatSelect = NTV2_YUVSelect;
	else
		inputFormatSelect = NTV2_RGBSelect;

	NTV2VideoFormat hdmiInputFormat = mCard->GetHDMIInputVideoFormat((NTV2Channel)mVirtualInputSelect);
	b4K = NTV2_IS_QUAD_FRAME_FORMAT(hdmiInputFormat);

	switch(mVirtualInputSelect)
	{
	case NTV2_Input1Select:
		inputXpt1 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn1 : NTV2_XptHDMIIn1RGB;
		inputXpt2 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn1Q2 : NTV2_XptHDMIIn1Q2RGB;
		inputXpt3 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn1Q3 : NTV2_XptHDMIIn1Q3RGB;
		inputXpt4 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn1Q4 : NTV2_XptHDMIIn1Q4RGB;
		break;
	case NTV2_Input2Select:
		inputXpt1 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn2 : NTV2_XptHDMIIn2RGB;
		inputXpt2 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn2Q2 : NTV2_XptHDMIIn2Q2RGB;
		inputXpt3 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn2Q3 : NTV2_XptHDMIIn2Q3RGB;
		inputXpt4 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn2Q4 : NTV2_XptHDMIIn2Q4RGB;
		break;
	case NTV2_Input3Select:
		inputXpt1 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn3 : NTV2_XptHDMIIn3RGB;
		inputXpt2 = NTV2_XptBlack;
		inputXpt3 = NTV2_XptBlack;
		inputXpt4 = NTV2_XptBlack;
		break;
	case NTV2_Input4Select:
		inputXpt1 = inputFormatSelect == NTV2_YUVSelect ? NTV2_XptHDMIIn4 : NTV2_XptHDMIIn4RGB;
		inputXpt2 = NTV2_XptBlack;
		inputXpt3 = NTV2_XptBlack;
		inputXpt4 = NTV2_XptBlack;
		break;
	default:
		break;
	}

	// make sure formats/modes match for multibuffer modes
	if (b4K)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
	}

	// 2 pixel interleave for 4k else not
	mCard->SetTsiFrameEnable(b4K, NTV2_CHANNEL1);
	mCard->SetTsiFrameEnable(b4K, NTV2_CHANNEL2);
	
	// CSCs
	if (inputFormatSelect == NTV2_RGBSelect)
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
		mCard->Connect (NTV2_XptCSC2VidInput, b4K ? NTV2_XptLUT2RGB : NTV2_XptBlack);
		mCard->Connect (NTV2_XptCSC3VidInput, b4K ? NTV2_XptLUT3Out : NTV2_XptBlack);
		mCard->Connect (NTV2_XptCSC4VidInput, b4K ? NTV2_XptLUT4Out : NTV2_XptBlack);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, inputXpt1);
		mCard->Connect (NTV2_XptCSC2VidInput, b4K ? inputXpt2 : NTV2_XptBlack);
		mCard->Connect (NTV2_XptCSC3VidInput, b4K ? inputXpt3 : NTV2_XptBlack);
		mCard->Connect (NTV2_XptCSC4VidInput, b4K ? inputXpt4 : NTV2_XptBlack);
	}
	

	// LUTs
	if (inputFormatSelect == NTV2_RGBSelect)
	{
		mCard->Connect (NTV2_XptLUT1Input, inputXpt1);
		mCard->Connect (NTV2_XptLUT2Input, b4K ? inputXpt2 : NTV2_XptBlack);
		mCard->Connect (NTV2_XptLUT3Input, b4K ? inputXpt3 : NTV2_XptBlack);
		mCard->Connect (NTV2_XptLUT4Input, b4K ? inputXpt4 : NTV2_XptBlack);

		// if RGB-to-RGB apply LUT converter
		if (bFb1RGB)
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_FULL2SMPTE);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_FULL2SMPTE);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL3, kLUTBank_FULL2SMPTE);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL4, kLUTBank_FULL2SMPTE);
		}
		else
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL3, kLUTBank_RGB2YUV);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL4, kLUTBank_RGB2YUV);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->Connect (NTV2_XptLUT2Input, b4K ? NTV2_XptCSC2VidRGB : NTV2_XptBlack);
		mCard->Connect (NTV2_XptLUT3Input, b4K ? NTV2_XptCSC3VidRGB : NTV2_XptBlack);
		mCard->Connect (NTV2_XptLUT4Input, b4K ? NTV2_XptCSC4VidRGB : NTV2_XptBlack);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_YUV2RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL3, kLUTBank_YUV2RGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL4, kLUTBank_YUV2RGB);
	}

	// 425 muxs
	if(inputFormatSelect == NTV2_RGBSelect)
	{
		if(bFb1RGB)
		{
			mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptLUT1RGB);
			mCard->Connect(NTV2_Xpt425Mux1BInput, b4K ? NTV2_XptLUT2RGB : NTV2_XptBlack);
			mCard->Connect(NTV2_Xpt425Mux2AInput, b4K ? NTV2_XptLUT3Out: NTV2_XptBlack);
			mCard->Connect(NTV2_Xpt425Mux2BInput, b4K ? NTV2_XptLUT4Out : NTV2_XptBlack);
		}
		else
		{
			mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidRGB);
			mCard->Connect(NTV2_Xpt425Mux1BInput, b4K ? NTV2_XptCSC2VidRGB : NTV2_XptBlack);
			mCard->Connect(NTV2_Xpt425Mux2AInput, b4K ? NTV2_XptCSC3VidRGB : NTV2_XptBlack);
			mCard->Connect(NTV2_Xpt425Mux2BInput, b4K ? NTV2_XptCSC4VidRGB : NTV2_XptBlack);
		}
	}
	else//inputFormatSelect == NTV2_YUVSelect
	{
		if(bFb1RGB)
		{
			mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptLUT1RGB);
			mCard->Connect(NTV2_Xpt425Mux1BInput, b4K ? NTV2_XptLUT2RGB : NTV2_XptBlack);
			mCard->Connect(NTV2_Xpt425Mux2AInput, b4K ? NTV2_XptLUT3Out : NTV2_XptBlack);
			mCard->Connect(NTV2_Xpt425Mux2BInput, b4K ? NTV2_XptLUT4Out : NTV2_XptBlack);
		}
		else
		{
			mCard->Connect(NTV2_Xpt425Mux1AInput, inputXpt1);
			mCard->Connect(NTV2_Xpt425Mux1BInput, b4K ? inputXpt2 : NTV2_XptBlack);
			mCard->Connect(NTV2_Xpt425Mux2AInput, b4K ? inputXpt3 : NTV2_XptBlack);
			mCard->Connect(NTV2_Xpt425Mux2BInput, b4K ? inputXpt4 : NTV2_XptBlack);
		}
	}
	mCard->Connect(NTV2_Xpt425Mux3AInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux3BInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux4AInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux4BInput, NTV2_XptBlack);

	// FrameStores
	if (b4K)
	{
		if (bFb1RGB)
		{
			mCard->Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1ARGB);
			mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BRGB);
			mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2ARGB);
			mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BRGB);
		}
		else
		{
			mCard->Connect(NTV2_XptFrameBuffer1Input, NTV2_Xpt425Mux1AYUV);
			mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_Xpt425Mux1BYUV);
			mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV);
			mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BYUV);
		}
	}
	else
	{
		if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);				// CSC converted
		}
		else
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, inputFormatSelect == NTV2_RGBSelect ? NTV2_XptCSC1VidRGB : inputXpt1);
		}
		mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_XptBlack);
		mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
		mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_XptBlack);
	}
	
	// Frame Buffer Disabling
	mCard->WriteRegister(kRegCh1Control, 0, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, b4K ? 0 : 1, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh3Control, 1, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh4Control, 1, kRegMaskChannelDisable, kRegShiftChannelDisable);
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void KonaHDMIServices::SetDeviceMiscRegisters ()
{
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();

	// special case - VANC 8bit pixel shift support
	if (mVANCMode && Is8BitFrameBufferFormat(mFb1Format) )
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
}
