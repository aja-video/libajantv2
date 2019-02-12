//
//  ntv2class4kservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2class4kservices.h"


//-------------------------------------------------------------------------------------------------------
//	class Class4kServices
//-------------------------------------------------------------------------------------------------------

Class4kServices::Class4kServices(NTV2DeviceID devID) : DeviceServices()
{
	mHasHdmiIn 		= NTV2DeviceGetNumHDMIVideoInputs(devID) > 0;
	mHasHdmiOut 	= NTV2DeviceGetNumHDMIVideoOutputs(devID) > 0;
	mHasSdiOut5 	= NTV2DeviceGetNumVideoOutputs(devID) == 5;
	mHasAnalogOut	= NTV2DeviceGetNumAnalogVideoOutputs(devID) > 0;
	mHas12G 		= NTV2DeviceCanDo12GSDI(devID);
	mHasCSC5		= mHasLUT5	= mHasHdmiOut || mHasSdiOut5 || mHasAnalogOut;
	mHas4kQuarter	= mHasSdiOut5 == false;
}


Class4kServices::~Class4kServices()
{
	for(uint32_t i = 0; i < 8; i++)
		mCard->EnableChannel((NTV2Channel)i);
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void Class4kServices::SetDeviceXPointPlayback ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback();
	
	//
	// Io4K
	//
	
	bool				bFb1RGB				= IsRGBFormat(mFb1Format);
	bool				bFb2RGB				= IsRGBFormat(mFb2Format);
	bool				b4K					= NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	bool				b4kHfr				= NTV2_IS_4K_HFR_VIDEO_FORMAT(mFb1VideoFormat);
	bool				b2FbLevelBHfr		= IsVideoFormatB(mFb1VideoFormat);
	bool				bSdiOutRGB			= mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb;
	bool				b3GaOutRGB			= (mSdiOutTransportType == NTV2_SDITransport_3Ga) && bSdiOutRGB;
	bool				b1wireQ4k			= b4K && (m4kTransportOutSelection == NTV2_4kTransport_Quarter_1wire);
	bool				b3GbOut				= (mSdiOutTransportType == NTV2_SDITransport_DualLink_3Gb) || b3GaOutRGB;
	bool				b2pi                = (b4K && m4kTransportOutSelection == NTV2_4kTransport_PixelInterleave);
	bool				b2x2piOut			= b4K && !b4kHfr && !bSdiOutRGB && (m4kTransportOutSelection == NTV2_4kTransport_PixelInterleave);
	bool				b2xQuadOut			= b4K && !b4kHfr && !bSdiOutRGB && (m4kTransportOutSelection == NTV2_4kTransport_Quadrants_2wire);
	bool				b4k6gOut			= b4K && !b4kHfr && !bSdiOutRGB && (m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire);
	bool				b4k12gOut			= b4K && (b4kHfr || bSdiOutRGB) && (m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire);
	uint32_t			bFb1Disable			= 0, bFb2Disable = 1, bFb3Disable = 1, bFb4Disable = 1;
	bool				bQuadSwap			= b4K && !b4k12gOut && !b4k6gOut && (mQuadSwapOut != 0);	
	bool				bDSKGraphicMode		= mDSKMode == NTV2_DSKModeGraphicOverMatte || 
											  mDSKMode == NTV2_DSKModeGraphicOverVideoIn ||
											  mDSKMode == NTV2_DSKModeGraphicOverFB;
	bool				bDSKOn				= mDSKMode == NTV2_DSKModeFBOverMatte || 
											  mDSKMode == NTV2_DSKModeFBOverVideoIn || 
											  (bFb2RGB && bDSKGraphicMode);
	bDSKOn									= bDSKOn && !b4K;			// DSK not supported with 4K formats, yet
	NTV2ColorSpaceMode	inputColorSpace		= mSDIInput1ColorSpace;		// Input format select (YUV, RGB, etc)
	NTV2CrosspointID	inputXptYuv1		= NTV2_XptBlack;			// Input source selected single stream
	NTV2CrosspointID	inputXptYuv2		= NTV2_XptBlack;			// Input source selected for 2nd stream (dual-stream, e.g. DualLink / 3Gb)
    bool				bFb1HdrRGB			= mFb1Format == NTV2_FBF_48BIT_RGB;
    bool				bFb2HdrRGB			= mFb2Format == NTV2_FBF_48BIT_RGB;
	bool				bHdmiOutRGB			= mDs.hdmiOutColorSpace == kHDMIOutCSCRGB8bit || mDs.hdmiOutColorSpace == kHDMIOutCSCRGB10bit;
	bool				bInRGB				= inputColorSpace == NTV2_ColorSpaceModeRgb;
													    
	b4k6gOut								= b4k6gOut && mHas12G;
	b4k12gOut								= b4k12gOut && mHas12G;
	b1wireQ4k								= b1wireQ4k && mHas4kQuarter;
	
	// XPoint Init 
	NTV2CrosspointID	XPt1, XPt2, XPt3, XPt4;

	if(b4k12gOut || b4k6gOut) b2pi = true;
	// make sure formats/modes match for multibuffer modes
	if (b4K || b2FbLevelBHfr)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
		bFb2RGB = IsRGBFormat(mFb1Format);
		
		if (b4K)
		{
			mCard->SetMode(NTV2_CHANNEL3, NTV2_MODE_DISPLAY);
			mCard->SetFrameBufferFormat(NTV2_CHANNEL3, mFb1Format);
			
			mCard->SetMode(NTV2_CHANNEL4, NTV2_MODE_DISPLAY);
			mCard->SetFrameBufferFormat(NTV2_CHANNEL4, mFb1Format);
		}
	}
	
	// select square division or 2 pixel interleave in frame buffer
    AdjustFor4kQuadOrTpiOut();
	
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
	// input 5 select
	else if (mVirtualInputSelect == NTV2_Input5Select)
	{
		inputXptYuv1 = NTV2_XptHDMIIn1;
		inputXptYuv2 = NTV2_XptBlack;
	}
	// dual link select
	else if (mVirtualInputSelect == NTV2_Input2xDLHDSelect)
	{
		inputXptYuv1 = NTV2_XptSDIIn1;
		inputXptYuv2 = NTV2_XptSDIIn2;
	}
	
	// Dual Link In 1
	if (bInRGB)
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
	if (b2FbLevelBHfr)
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
	if (b2FbLevelBHfr)
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
        frameSync2RGB = bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux1ARGB : NTV2_XptFrameBuffer1RGB) : NTV2_XptLUT1RGB;
	}
	else
	{
		frameSync2RGB = NTV2_XptLUT1RGB;
	}
	
	
	// CSC 1
	if (b4K)
	{
		if (bFb1RGB)
		{
            mCard->Connect (NTV2_XptCSC1VidInput, bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux1ARGB : NTV2_XptFrameBuffer1RGB) : NTV2_XptLUT1RGB);
		}
		else
		{
			if (!b2pi)
			{
				mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV);
			}
			else
			{
				mCard->Connect (NTV2_XptCSC1VidInput, NTV2_Xpt425Mux1AYUV);
			}
		}
	}
	else if (bFb1RGB || bDSKOn)
	{
        mCard->Connect (NTV2_XptCSC1VidInput, bFb1HdrRGB ? NTV2_XptFrameBuffer1RGB : NTV2_XptLUT1RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptFrameBuffer1YUV);
	}
	
	
	// CSC 2
	if (b4K)
	{
		if (bFb1RGB)
		{
            mCard->Connect (NTV2_XptCSC2VidInput, bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux1BRGB : NTV2_XptFrameBuffer2RGB) : NTV2_XptLUT2RGB);
		}
		else
		{
			if (!b2pi)
			{
				mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2YUV);
			}
			else
			{
				mCard->Connect (NTV2_XptCSC2VidInput, NTV2_Xpt425Mux1BYUV);
			}
		}
	}
	else if (bFb2RGB)
	{
        mCard->Connect (NTV2_XptCSC2VidInput, bFb2HdrRGB ? NTV2_XptFrameBuffer2RGB : NTV2_XptLUT2RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptFrameBuffer2YUV);
	}
	
	
	// CSC 3
	if (b4K)
	{
		if (bFb1RGB)
		{
            mCard->Connect (NTV2_XptCSC3VidInput, bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux2ARGB : NTV2_XptFrameBuffer3RGB) : NTV2_XptLUT3Out);
		}
		else
		{
			if (!b2pi)
			{
				mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptFrameBuffer3YUV);
			}
			else
			{
				mCard->Connect (NTV2_XptCSC3VidInput, NTV2_Xpt425Mux2AYUV);
			}
		}
	}
	else
	{
		mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	}
	
	
	// CSC 4
	if (b4K)
	{
		if (bFb1RGB)
		{
            mCard->Connect (NTV2_XptCSC4VidInput, bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux2BRGB : NTV2_XptFrameBuffer4RGB) : NTV2_XptLUT4Out);
		}
		else
		{
			if (!b2pi)
			{
				mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptFrameBuffer4YUV);
			}
			else
			{
				mCard->Connect (NTV2_XptCSC4VidInput, NTV2_Xpt425Mux2BYUV);
			}
		}
	}
	else
	{
		mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);
	}
	
	
	// CSC 5
	if (mHasCSC5)
	{
		if (b4K && !b2pi)
		{
			if (bSdiOutRGB)
			{
				mCard->Connect (NTV2_XptCSC5VidInput, NTV2_XptLUT5Out);
			}
			else
			{
				mCard->Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptCSC5VidInput, NTV2_XptBlack);
		}
	}
	
	// LUT 1
	// note b4K is same processing as regular formats
	if (bFb1RGB || bDSKOn)
	{
		if (!b2pi)
		{
			mCard->Connect (NTV2_XptLUT1Input, NTV2_XptFrameBuffer1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptLUT1Input, NTV2_Xpt425Mux1ARGB);
		}
		
		// if RGB-to-RGB apply LUT converter
		if (bSdiOutRGB)
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
	if (b4K)
	{
		if (bFb1RGB)
		{
			if (!b2pi)
			{
				mCard->Connect (NTV2_XptLUT2Input, NTV2_XptFrameBuffer2RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptLUT2Input, NTV2_Xpt425Mux1BRGB);
			}
	
		
			// if RGB-to-RGB apply LUT converter
			if (bSdiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank (  NTV2_CHANNEL2,					
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
	}
	else if (bFb2RGB)
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
	if (b4K)
	{
		if (bFb1RGB)
		{
			if (!b2pi)
			{
				mCard->Connect (NTV2_XptLUT3Input, NTV2_XptFrameBuffer3RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptLUT3Input, NTV2_Xpt425Mux2ARGB);
			}
			
			// if RGB-to-RGB apply LUT converter
			if (bSdiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank(  NTV2_CHANNEL3,					
													 mRGB10Range == NTV2_RGB10RangeFull ?
													 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
			}
			else
			{
				mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL3, kLUTBank_RGB2YUV);	
			}
		}
		else
		{
			mCard->Connect (NTV2_XptLUT3Input, NTV2_XptCSC3VidRGB);
			mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL3, kLUTBank_YUV2RGB);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptLUT3Input, NTV2_XptBlack);
	}
	
	
	// LUT 4
	if (b4K)
	{
		if (bFb1RGB)
		{
			if (!b2pi)
			{
				mCard->Connect (NTV2_XptLUT4Input, NTV2_XptFrameBuffer4RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptLUT4Input, NTV2_Xpt425Mux2BRGB);
			}

			// if RGB-to-RGB apply LUT converter
			if (bSdiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank(  NTV2_CHANNEL4,					
													 mRGB10Range == NTV2_RGB10RangeFull ?
													 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
			}
			else
			{
				mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL4, kLUTBank_RGB2YUV);	
			}
		}
		else
		{
			mCard->Connect (NTV2_XptLUT4Input, NTV2_XptCSC4VidRGB);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL4, kLUTBank_YUV2RGB);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptLUT4Input, NTV2_XptBlack);
	}
	
	
	// LUT 5
	if (mHasLUT5)
	{
		if (b4K && !b2pi)
		{
			if (bSdiOutRGB)
			{
				mCard->Connect (NTV2_XptLUT5Input, NTV2_Xpt4KDownConverterOutRGB);
			}
			else
			{
				mCard->Connect (NTV2_XptLUT5Input, NTV2_XptCSC5VidRGB);
			}
			
			if (bSdiOutRGB && bHdmiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank( NTV2_CHANNEL5, mRGB10Range == NTV2_RGB10RangeFull ?
													 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
			}
			else if (bSdiOutRGB && !bHdmiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL5, kLUTBank_RGB2YUV);
			}
			else if (!bSdiOutRGB && bHdmiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL5, kLUTBank_YUV2RGB);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptLUT5Input, NTV2_XptBlack);
		}
	}

	
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
	XPt1 = XPt2 = XPt3 = XPt4 = NTV2_XptBlack;
	// DC applies only to 4K Quad - for now
	if (b4K && !b2pi)
	{
		// SDIOut-RGB
		if (bSdiOutRGB)
		{
			// (FB-RGB: FB<-LUT<-DC) or (FB-YUV: FB<-CSC<-LUT<-DC)
			XPt1 = NTV2_XptLUT1RGB;
			XPt2 = NTV2_XptLUT2RGB;
			XPt3 = NTV2_XptLUT3Out;
			XPt4 = NTV2_XptLUT4Out;
			mCard->Enable4KDCRGBMode(true);
		}
		// SDIOut-YUV
		else
		{
			// (FB-RGB: FB<-LUT<-CSC<-DC) or (FB-YUV: FB<-DC)
			XPt1 =  bFb1RGB ? NTV2_XptCSC1VidYUV : NTV2_XptFrameBuffer1YUV;
			XPt2 =  bFb1RGB ? NTV2_XptCSC2VidYUV : NTV2_XptFrameBuffer2YUV;
			XPt3 =  bFb1RGB ? NTV2_XptCSC3VidYUV : NTV2_XptFrameBuffer3YUV;
			XPt4 =  bFb1RGB ? NTV2_XptCSC4VidYUV : NTV2_XptFrameBuffer4YUV;
			mCard->Enable4KDCRGBMode(false);
		}
	}
	mCard->Connect (NTV2_Xpt4KDCQ1Input, XPt1);
	mCard->Connect (NTV2_Xpt4KDCQ2Input, XPt2);
	mCard->Connect (NTV2_Xpt4KDCQ3Input, XPt3);
	mCard->Connect (NTV2_Xpt4KDCQ4Input, XPt4);

		
	// Dual Link Out 1
	if (b4K)
	{
		if (bFb1RGB)
		{
            mCard->Connect (NTV2_XptDualLinkOut1Input, bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux1ARGB : NTV2_XptFrameBuffer1RGB) : NTV2_XptLUT1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
		}
	}
	else
	{
        mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptBlack);
	}
	
	
	// Dual Link Out 2
	if (b4K)
	{
		if (bFb1RGB)
		{
            mCard->Connect (NTV2_XptDualLinkOut2Input, bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux1BRGB : NTV2_XptFrameBuffer2RGB) : NTV2_XptLUT2RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptLUT2RGB);
		}
	}
	else
	{
        mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptBlack);
	}
	
	
	// Dual Link Out 3
	if (b4K)
	{
		if (bFb1RGB)
		{
            mCard->Connect (NTV2_XptDualLinkOut3Input, bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux2ARGB : NTV2_XptFrameBuffer3RGB) : NTV2_XptLUT3Out);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut3Input, b1wireQ4k ? NTV2_Xpt4KDownConverterOutRGB : NTV2_XptLUT3Out);
		}
	}
	else if (bSdiOutRGB)
	{
        mCard->Connect (NTV2_XptDualLinkOut3Input, bFb1HdrRGB ? NTV2_XptFrameBuffer1RGB : frameSync2RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptBlack);
	}

	
	// Dual Link Out 4
	if (b4K)
	{
		if (bFb1RGB)
		{
            mCard->Connect (NTV2_XptDualLinkOut4Input, bFb1HdrRGB ? (b2pi ? NTV2_Xpt425Mux2BRGB : NTV2_XptFrameBuffer4RGB) : NTV2_XptLUT4Out);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptLUT4Out);
		}
	}
	else if (bSdiOutRGB)
	{
        mCard->Connect (NTV2_XptDualLinkOut4Input, bFb2HdrRGB ? NTV2_XptFrameBuffer1RGB : frameSync2RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptBlack);
	}
	
	
	// Dual Link Out 5
	if (b4K && !b2pi)
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, bSdiOutRGB ? NTV2_Xpt4KDownConverterOutRGB : NTV2_XptBlack);
	}
	else if (b4K && b2pi)
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, bSdiOutRGB ? NTV2_XptLUT1RGB : NTV2_XptBlack);
	}
	else
	{
		if (bSdiOutRGB)
		{
			mCard->Connect (NTV2_XptDualLinkOut5Input, bFb1HdrRGB ? NTV2_XptFrameBuffer1RGB : NTV2_XptLUT1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut5Input, NTV2_XptBlack);
		}
	}
	
	
	// SDI Out 1
	if (b4K)
	{
		if (b1wireQ4k)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
		}
        else if (bSdiOutRGB || bFb1HdrRGB)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, bQuadSwap ? NTV2_XptDuallinkOut3 : NTV2_XptDuallinkOut1);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, bQuadSwap ? NTV2_XptDuallinkOut3DS2 : NTV2_XptDuallinkOut1DS2);
		}
		else if (!b2pi)
		{
			if (!b2xQuadOut)
			{
				// is 4k quad 4-wire
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut1Input, bQuadSwap ? NTV2_XptCSC3VidYUV : NTV2_XptCSC1VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut1Input, bQuadSwap ? NTV2_XptFrameBuffer3YUV : NTV2_XptFrameBuffer1YUV);
				}
				mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
			}
			else
			{
				// is 4k quad 2-wire
				mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
				mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
			}
		}
		else
		{
			// is SMPTE 425 YUV
			if (!b2x2piOut)
			{
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut1Input, bQuadSwap ? NTV2_XptCSC3VidYUV : NTV2_XptCSC1VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut1Input, bQuadSwap ? NTV2_Xpt425Mux2AYUV : NTV2_Xpt425Mux1AYUV);
				}
				mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
				mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
			}
		}
	}
	else
	{
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}
	
	
	// SDI Out 2
	if (b4K)
	{
		if (b1wireQ4k)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
		}
        else if (bSdiOutRGB || bFb1HdrRGB)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, bQuadSwap ?  NTV2_XptDuallinkOut4 : NTV2_XptDuallinkOut2);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, bQuadSwap ?  NTV2_XptDuallinkOut4DS2 : NTV2_XptDuallinkOut2DS2);
		}
		else if (!b2pi)
		{
			if (!b2xQuadOut)
			{
				// is 4k quad 4-wire
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut2Input, bQuadSwap ? NTV2_XptCSC4VidYUV : NTV2_XptCSC2VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut2Input, bQuadSwap ?  NTV2_XptFrameBuffer4YUV : NTV2_XptFrameBuffer2YUV);
				}
				mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
			}
			else
			{
				// is 4k quad 2-wire
				mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
				mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
			}
		}
		else
		{
			if (!b2x2piOut)
			{ 
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut2Input, bQuadSwap ?  NTV2_XptCSC4VidYUV : NTV2_XptCSC2VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut2Input, bQuadSwap ?  NTV2_Xpt425Mux2BYUV : NTV2_Xpt425Mux1BYUV);
				}
				mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
				mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
			}
		}
	}
	else
	{
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
	}
	
	
	// SDI Out 3 - acts like SDI 1 in non-4K mode
	if (b4K)
	{
		if (b1wireQ4k)
		{
			if (bSdiOutRGB)
			{
				mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);
				mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptDuallinkOut3DS2);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut3Input, NTV2_Xpt4KDownConverterOut);
				mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
			}
		}
        else if (bSdiOutRGB || bFb1HdrRGB)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, bQuadSwap ?  NTV2_XptDuallinkOut1 : NTV2_XptDuallinkOut3);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, bQuadSwap ?  NTV2_XptDuallinkOut1DS2 : NTV2_XptDuallinkOut3DS2);
		}
		else if (!b2pi)
		{
			if (!b2xQuadOut)
			{
				// is 4k quad 4-wire
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut3Input, bQuadSwap ? NTV2_XptCSC1VidYUV : NTV2_XptCSC3VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut3Input, bQuadSwap ?  NTV2_XptFrameBuffer1YUV : NTV2_XptFrameBuffer3YUV);
				}
				mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
			}
			else
			{
				// is 4k quad 2-wire
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptCSC2VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer1YUV);
					mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptFrameBuffer2YUV);
				}
			}
		}
		else
		{
			// smpte 425
			if (!b2x2piOut)
			{ 
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut3Input, bQuadSwap ?  NTV2_XptCSC1VidYUV : NTV2_XptCSC3VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut3Input, bQuadSwap ? NTV2_Xpt425Mux1AYUV : NTV2_Xpt425Mux2AYUV);
				}
				mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
			}
			else
			{
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptCSC2VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut3Input, NTV2_Xpt425Mux1AYUV);
					mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_Xpt425Mux1BYUV);
				}
			}
		}
	}
	else if (b2FbLevelBHfr)	
	{
		mCard->Connect (NTV2_XptSDIOut3Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut3InputDS2, b3GbOut ? frameSync2YUV : NTV2_XptBlack);
	}
	else if (bSdiOutRGB)
	{
		mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);
		mCard->Connect (NTV2_XptSDIOut3InputDS2, b3GbOut ? NTV2_XptDuallinkOut3DS2 : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)
	{
		if (bDSKOn)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, b3GbOut ? frameSync2YUV : NTV2_XptBlack);
		}
		else if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC1VidYUV);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, b3GbOut ? NTV2_XptCSC1KeyYUV : NTV2_XptBlack);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut3Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
		}
	}
	else // NTV2_PrimaryOutputSelect
	{
        if (bFb1HdrRGB)
        {
            mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut1);
            mCard->Connect (NTV2_XptSDIOut3InputDS2, b3GbOut ? NTV2_XptDuallinkOut1DS2 : NTV2_XptBlack);
        }
        else
        {
            mCard->Connect (NTV2_XptSDIOut3Input, frameSync1YUV);
            mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
        }
	}
	
	
	// SDI Out 4 - acts like SDI 2 in non-4K mode
	if (b4K)
	{
		if (b1wireQ4k)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
        else if (bSdiOutRGB || bFb1HdrRGB)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, bQuadSwap ? NTV2_XptDuallinkOut2 : NTV2_XptDuallinkOut4);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, bQuadSwap ? NTV2_XptDuallinkOut2DS2 : NTV2_XptDuallinkOut4DS2);
		}
		else if (!b2pi)
		{
			if (!b2xQuadOut)
			{
				// is 4k quad 4-wire
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut4Input, bQuadSwap ? NTV2_XptCSC2VidYUV : NTV2_XptCSC4VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut4Input, bQuadSwap ? NTV2_XptFrameBuffer2YUV : NTV2_XptFrameBuffer4YUV);
				}
				mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
			}
			else
			{
				// is 4k quad 2-wire
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC3VidYUV);
					mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptCSC4VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer3YUV);
					mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptFrameBuffer4YUV);
				}
			}
		}
		else
		{
			// is 2 pixel interleaved - YUV output
			if (!b2x2piOut)
			{
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut4Input, bQuadSwap ? NTV2_XptCSC2VidYUV : NTV2_XptCSC4VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut4Input, bQuadSwap ? NTV2_Xpt425Mux1BYUV : NTV2_Xpt425Mux2BYUV);
				}
				mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
			}
			else
			{
				// is 2 pixel interleaved - YUV output
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC3VidYUV);
					mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptCSC4VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut4Input, NTV2_Xpt425Mux2AYUV);
					mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_Xpt425Mux2BYUV);
				}				
			}
		}
	}
	else if (b2FbLevelBHfr)
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, frameSync2YUV);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut4Input, frameSync2YUV);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
	}
	else if (bSdiOutRGB)
	{
		mCard->Connect (NTV2_XptSDIOut4Input, b3GbOut ? NTV2_XptDuallinkOut4 : NTV2_XptDuallinkOut3DS2);
		mCard->Connect (NTV2_XptSDIOut4InputDS2, b3GbOut ? NTV2_XptDuallinkOut4DS2 : NTV2_XptBlack);
	}
	else if (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)				// Video+Key
	{
		if (bDSKOn)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, b3GbOut ? frameSync1YUV : frameSync2YUV);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, b3GbOut ? frameSync2YUV : NTV2_XptBlack);
		}
		else if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, b3GbOut ? NTV2_XptCSC1VidYUV : NTV2_XptCSC1KeyYUV);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, b3GbOut ? NTV2_XptCSC1KeyYUV : NTV2_XptBlack);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut4Input, frameSync1YUV);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
	}
	else // NTV2_PrimaryOutputSelect												// Primary
	{
        if (bFb1HdrRGB)
        {
            mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut1);
            mCard->Connect (NTV2_XptSDIOut4InputDS2, b3GbOut ? NTV2_XptDuallinkOut1DS2 : NTV2_XptBlack);
        }
        else
        {
            mCard->Connect (NTV2_XptSDIOut4Input, frameSync1YUV);
            mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
        }
	}
	
	
	// SDI Out 5
	if (bSdiOutRGB)			// RGB Out - alway use DL
	{
		mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptDuallinkOut5);
		mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptDuallinkOut5DS2);
	}
	// YUV Out 4K Quads 
	else if (b4K && !b2pi)
	{
		mCard->Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut);
		mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
	}
	// LevelB
	else if (b2FbLevelBHfr)											
	{
		mCard->Connect (NTV2_XptSDIOut5Input, frameSync1YUV);
		mCard->Connect (NTV2_XptSDIOut5InputDS2, frameSync2YUV);
	}
	// YUV Out
	else
	{
		if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC1VidYUV);
		}
		
		// FB-YUV 
		else
		{
			mCard->Connect (NTV2_XptSDIOut5Input, b2pi ? NTV2_Xpt425Mux1AYUV : NTV2_XptFrameBuffer1YUV);
		}
		
		mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
	}

	
	// HDMI Out
	if (mHasHdmiOut)
	{
		XPt1 = XPt2 = XPt3 = XPt4 = NTV2_XptBlack;
		if (b4K)
		{
			// 2si mode
			if (b2pi)
			{
				// RGB-mode set
				if (bHdmiOutRGB || (bFb1HdrRGB || bFb2HdrRGB))
				{
					switch (mVirtualHDMIOutputSelect)
					{
					default:
					case NTV2_PrimaryOutputSelect:
					case NTV2_4kHalfFrameRate:		// unsupported for now
						XPt1 = bFb1HdrRGB ? NTV2_Xpt425Mux1ARGB : NTV2_XptLUT1RGB;
						XPt2 = bFb1HdrRGB ? NTV2_Xpt425Mux1BRGB : NTV2_XptLUT2RGB;
						XPt3 = bFb2HdrRGB ? NTV2_Xpt425Mux2ARGB : NTV2_XptLUT3Out;
						XPt4 = bFb2HdrRGB ? NTV2_Xpt425Mux2BRGB : NTV2_XptLUT4Out;
						break;
					case NTV2_Quarter4k:       XPt1 = NTV2_XptLUT1RGB; break;
					case NTV2_Quadrant1Select: XPt1 = NTV2_XptLUT1RGB; break;
					case NTV2_Quadrant2Select: XPt1 = NTV2_XptLUT2RGB; break;
					case NTV2_Quadrant3Select: XPt1 = NTV2_XptLUT3Out; break;
					case NTV2_Quadrant4Select: XPt1 = NTV2_XptLUT4Out; break;
					}
				}
				
				// YUV-mode set
				else 
				{
					switch (mVirtualHDMIOutputSelect)
					{
					default:
					case NTV2_PrimaryOutputSelect:
					case NTV2_4kHalfFrameRate:		// unsupported for now
						XPt1 = bFb1RGB ? NTV2_XptCSC1VidYUV : NTV2_Xpt425Mux1AYUV;
						XPt2 = bFb1RGB ? NTV2_XptCSC2VidYUV : NTV2_Xpt425Mux1BYUV;
						XPt3 = bFb1RGB ? NTV2_XptCSC3VidYUV : NTV2_Xpt425Mux2AYUV;
						XPt4 = bFb1RGB ? NTV2_XptCSC4VidYUV : NTV2_Xpt425Mux2BYUV;
						break;
					case NTV2_Quarter4k:       XPt1 = bFb1RGB ? NTV2_XptCSC1VidYUV : NTV2_Xpt425Mux1AYUV; break;
					case NTV2_Quadrant1Select: XPt1 = bFb1RGB ? NTV2_XptCSC1VidYUV : NTV2_Xpt425Mux1AYUV; break;
					case NTV2_Quadrant2Select: XPt1 = bFb1RGB ? NTV2_XptCSC2VidYUV : NTV2_Xpt425Mux1BYUV; break;
					case NTV2_Quadrant3Select: XPt1 = bFb1RGB ? NTV2_XptCSC3VidYUV : NTV2_Xpt425Mux2AYUV; break;
					case NTV2_Quadrant4Select: XPt1 = bFb1RGB ? NTV2_XptCSC4VidYUV : NTV2_Xpt425Mux2BYUV; break;
					}
				}
			}
			
			// quadrant mode
			else 
			{
				// RGB-mode set
				if (bHdmiOutRGB)
				{
					switch (mVirtualHDMIOutputSelect)
					{
					default:
					case NTV2_PrimaryOutputSelect:
					case NTV2_4kHalfFrameRate:		// unsupported for now
					case NTV2_Quarter4k:       
						XPt1 = bSdiOutRGB ? NTV2_Xpt4KDownConverterOutRGB : NTV2_XptLUT5Out;
						break;
					case NTV2_Quadrant1Select: XPt1 = NTV2_XptLUT1RGB; break;
					case NTV2_Quadrant2Select: XPt1 = NTV2_XptLUT2RGB; break;
					case NTV2_Quadrant3Select: XPt1 = NTV2_XptLUT3Out; break;
					case NTV2_Quadrant4Select: XPt1 = NTV2_XptLUT4Out; break;
					}
				}
				
				// YUV-mode set
				else 
				{
					switch (mVirtualHDMIOutputSelect)
					{
					default:
					case NTV2_PrimaryOutputSelect:
					case NTV2_4kHalfFrameRate:		// unsupported for now
					case NTV2_Quarter4k:
						XPt1 = bSdiOutRGB ? NTV2_XptCSC5VidYUV :  NTV2_Xpt4KDownConverterOut;
						break;
					case NTV2_Quadrant1Select: XPt1 = NTV2_XptCSC1VidYUV; break;
					case NTV2_Quadrant2Select: XPt1 = NTV2_XptCSC2VidYUV; break;
					case NTV2_Quadrant3Select: XPt1 = NTV2_XptCSC3VidYUV; break;
					case NTV2_Quadrant4Select: XPt1 = NTV2_XptCSC4VidYUV; break;
					}
				}
			}
		}
		else if (bDSKOn)
		{
			XPt1 = frameSync1YUV;
		}
		else if (b2FbLevelBHfr)
		{
			// LevelB
			XPt1 = NTV2_XptLUT1RGB;
			XPt2 = NTV2_XptLUT2RGB;
		}
		else
		{
			if (bHdmiOutRGB)
			{
				XPt1 = bFb1HdrRGB ? frameSync2RGB : NTV2_XptLUT1RGB;
			}
			else
			{
				if (bFb1RGB)
				{
					XPt1 = NTV2_XptCSC1VidYUV;
				}
				
				// FB-YUV 
				else
				{
					XPt1 = b2pi ? NTV2_Xpt425Mux1AYUV : NTV2_XptFrameBuffer1YUV;
				}
			}
		}

		mCard->Connect (NTV2_XptHDMIOutInput,	XPt1);
		mCard->Connect (NTV2_XptHDMIOutQ2Input, XPt2);
		mCard->Connect (NTV2_XptHDMIOutQ3Input, XPt3);
		mCard->Connect (NTV2_XptHDMIOutQ4Input, XPt4);

		// 4K Hdmi-to-Hdmi Bypass always disabled for playback
		mCard->WriteRegister(kRegHDMIOutControl, false, kRegMaskHDMIV2TxBypass, kRegShiftHDMIV2TxBypass);
	}
	
	
	// Analog Out
	if (mHasAnalogOut)
	{
		if (b4K)
		{
			switch (mVirtualAnalogOutputSelect)
			{
			default:
			case NTV2_Quarter4k:
				if (b2pi)
					mCard->Connect (NTV2_XptAnalogOutInput, bSdiOutRGB ? NTV2_XptCSC1VidYUV : NTV2_XptFrameBuffer1_425YUV); 
				else
					mCard->Connect (NTV2_XptAnalogOutInput, bSdiOutRGB ? NTV2_XptCSC5VidYUV : NTV2_Xpt4KDownConverterOut); 
				break;
			};
		}
		else
		{
			mCard->Connect (NTV2_XptAnalogOutInput, frameSync1YUV);
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
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptFrameBuffer1YUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptFrameBuffer1YUV);
					bNoKey = true;
				}
				
				mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptFrameBuffer1YUV);
				mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptFrameBuffer1YUV);
				mCard->WriteRegister (kRegVidProc1Control, 1, kRegMaskVidProcBGMatteEnable, kRegShiftVidProcBGMatteEnable);
				break;
				
			case NTV2_DSKModeFBOverVideoIn:
				// Foreground
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC1KeyYUV);
				}
				else
				{
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
				else if (mVirtualInputSelect == NTV2_Input5Select)
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
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
				}
				else
				{
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
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
				}
				else
				{
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
				else if (mVirtualInputSelect == NTV2_Input5Select)
				{
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptHDMIIn1);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptHDMIIn1);
				}
				else if (mVirtualInputSelect == NTV2_Input2xDLHDSelect)
				{
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
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptCSC2VidYUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptCSC2KeyYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptFrameBuffer2YUV);
					mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptFrameBuffer2YUV);
					bNoKey = true;
				}
				
				if (bFb1RGB)
				{
					mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptCSC1KeyYUV);
				}
				else
				{
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
	if (b2FbLevelBHfr)
	{
		bFb1Disable = bFb2Disable = 0;
	}
	else if (b4K)
	{
		if (b2pi)
		{
			bFb1Disable = bFb2Disable = 0;
		}
		else
		{
			bFb1Disable = bFb2Disable = bFb3Disable = bFb4Disable = 0;
		}
	}
	if(bFb1Disable)
	{
		mCard->DisableChannel(NTV2_CHANNEL1);
		mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptBlack);
	}
	else
		mCard->EnableChannel(NTV2_CHANNEL1);

	if(bFb2Disable)
	{
		mCard->DisableChannel(NTV2_CHANNEL2);
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	}
	else
		mCard->EnableChannel(NTV2_CHANNEL2);

	if(bFb3Disable)
	{
		mCard->DisableChannel(NTV2_CHANNEL3);
		mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
	}
	else
		mCard->EnableChannel(NTV2_CHANNEL3);

	if(bFb4Disable)
	{
		mCard->DisableChannel(NTV2_CHANNEL4);
		mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
	}
	else
		mCard->EnableChannel(NTV2_CHANNEL4);


	// 425 Mux
	if (b2pi)
	{
		if (bFb1RGB)
		{
			mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptFrameBuffer1RGB);
			mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptFrameBuffer1_425RGB);
			mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptFrameBuffer2RGB);
			mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptFrameBuffer2_425RGB);
		}
		else
		{
			mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptFrameBuffer1YUV);
			mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptFrameBuffer1_425YUV);
			mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptFrameBuffer2YUV);
			mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptFrameBuffer2_425YUV);
		}
	}
	else
	{
		mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptBlack);
		mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptBlack);
	}
	mCard->Connect(NTV2_Xpt425Mux3AInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux3BInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux4AInput, NTV2_XptBlack);
	mCard->Connect(NTV2_Xpt425Mux4BInput, NTV2_XptBlack);

}

	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void Class4kServices::SetDeviceXPointCapture ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();
	
	bool				bFb1RGB			= IsRGBFormat(mFb1Format);
	NTV2VideoFormat		inputFormat		= mDs.inputVideoFormatSelect;
	NTV2RGBRangeMode	frambBufferRange= mRGB10Range == NTV2_RGB10RangeSMPTE ? NTV2_RGBRangeSMPTE : NTV2_RGBRangeFull;
	SdiState*			sdiIn			= mVirtualInputSelect == NTV2_Input2Select ? mDs.sdiIn[1] : mDs.sdiIn[0];
	bool				b3GbOut			= mSdiOutTransportType == NTV2_SDITransport_DualLink_3Gb;
	bool				bSdiOutRGB		= mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb;
	bool				b4K				= NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	bool				b4kHfr			= NTV2_IS_4K_HFR_VIDEO_FORMAT(mFb1VideoFormat);
	bool				b2FbLevelBHfr	= IsVideoFormatB(mFb1VideoFormat);
	//bool				b4k6gOut		= b4K && !b4kHfr && !bSdiOutRGB && (m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire);
	//bool				b4k12gOut		= b4K && (b4kHfr || bSdiOutRGB) && m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire;
	bool				b2x2piOut		= b4K && !b4kHfr && !bSdiOutRGB && (m4kTransportOutSelection == NTV2_4kTransport_PixelInterleave);
	bool				b2xQuadOut		= b4K && (m4kTransportOutSelection == NTV2_4kTransport_Quadrants_2wire);
	bool 				b2x2piIn  		= mDs.input2si && mDs.bIn2xSdi && !sdiIn->is6G;
	bool 				b2pi			= mDs.input2si;						// input: hdmi-4k or sdi-4k-2pi (2x or 4x)
	bool				b2xQuadIn		= b4K && !b4kHfr && !b2pi && (mVirtualInputSelect == NTV2_Input2x4kSelect);
	uint32_t			bFb1Disable		= 0, bFb2Disable = 1, bFb3Disable = 1, bFb4Disable = 1;
	//bool 				bFbLevelA 		= IsVideoFormatA(mFb1sVideoFormat); 
	bool				bQuadSwap		= b4K == true && mVirtualInputSelect == NTV2_Input4x4kSelect && mQuadSwapIn != 0;
	bool				bHdmiIn			= mDs.bInHdmi && mHasHdmiIn;
	bool				bHdmiInRGB		= bHdmiIn == true && mDs.hdmiIn[0]->cs == NTV2_ColorSpaceModeRgb;
	bool				bHdmiOutRGB		= mDs.hdmiOutColorSpace == kHDMIOutCSCRGB8bit || mDs.hdmiOutColorSpace == kHDMIOutCSCRGB10bit;
	bool				bInRGB			= bHdmiInRGB || mDs.bInSdiRgb;
	bool				bFb1HdrRGB		= mFb1Format == NTV2_FBF_48BIT_RGB;
    //bool				bFb2HdrRGB		= mFb2Format == NTV2_FBF_48BIT_RGB;
	
	bHdmiIn								= bHdmiIn && mHasHdmiIn;
	bHdmiOutRGB							= bHdmiOutRGB && mHasHdmiOut;
	//b4k6gOut							= b4k6gOut && mHas12G;
	//b4k12gOut							= b4k12gOut && mHas12G;
	
	// SMPTE 425 (2pi)
	NTV2CrosspointID	XPt1, XPt2, XPt3, XPt4;
	NTV2CrosspointID	inHdYUV1, inHdYUV2;	
	NTV2CrosspointID	inHdRGB1;	
	NTV2CrosspointID	in4kRGB1, in4kRGB2, in4kRGB3, in4kRGB4;
	NTV2CrosspointID	in4kYUV1, in4kYUV2, in4kYUV3, in4kYUV4;

	// input 1 select
	inHdYUV1 = inHdYUV2 = inHdRGB1 = NTV2_XptBlack;
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		inHdYUV1	= NTV2_XptSDIIn1;
		inHdYUV2	= NTV2_XptSDIIn1DS2;
		inHdRGB1	= NTV2_XptDuallinkIn1;
	}
	// input 2 select
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		inHdYUV1	= NTV2_XptSDIIn2;
		inHdYUV2	= NTV2_XptSDIIn2DS2;
		inHdRGB1	= NTV2_XptDuallinkIn1;
	}
	// input 5 select HDMI
	else if (mVirtualInputSelect == NTV2_Input5Select)
	{
		inHdYUV1 = NTV2_XptHDMIIn1;
		inHdYUV2 = NTV2_XptHDMIIn1Q2;
		inHdRGB1 = NTV2_XptHDMIIn1RGB;
	}
	// dual link select
	else if (mVirtualInputSelect == NTV2_Input2xDLHDSelect)
	{
		inHdYUV1 = NTV2_XptSDIIn1;
		inHdYUV2 = NTV2_XptSDIIn2;
		inHdRGB1 = NTV2_XptDuallinkIn1;
	}

	// 4K input routing
	in4kRGB1 = in4kRGB2 = in4kRGB3 = in4kRGB4 = NTV2_XptBlack;
	in4kYUV1 = in4kYUV2 = in4kYUV3 = in4kYUV4 = NTV2_XptBlack;
	if (b4K)
	{
		if (bHdmiIn)
		{
			if (bInRGB)
			{
				in4kRGB1 = NTV2_XptHDMIIn1RGB;		in4kRGB2 = NTV2_XptHDMIIn1Q2RGB;
				in4kRGB3 = NTV2_XptHDMIIn1Q3RGB;	in4kRGB4 = NTV2_XptHDMIIn1Q4RGB;
			}
			else
			{
				in4kYUV1 = NTV2_XptHDMIIn1;		in4kYUV2 = NTV2_XptHDMIIn1Q2;
				in4kYUV3 = NTV2_XptHDMIIn1Q3;	in4kYUV4 = NTV2_XptHDMIIn1Q4;
			}
		}
		else if (bInRGB)	// SDI-RGB
		{
			in4kRGB1 = NTV2_XptDuallinkIn1;	in4kRGB2 = NTV2_XptDuallinkIn2;
			in4kRGB3 = NTV2_XptDuallinkIn3;	in4kRGB4 = NTV2_XptDuallinkIn4;
		}
		else if (b2x2piIn || b2xQuadIn)	// SDI-2Wire-YUV
		{
			in4kYUV1 = NTV2_XptSDIIn1;		in4kYUV2 = NTV2_XptSDIIn1DS2;
			in4kYUV3 = NTV2_XptSDIIn2;		in4kYUV4 = NTV2_XptSDIIn2DS2;
		}
		else								// SDI-4Wire-YUV
		{
			in4kYUV1 = NTV2_XptSDIIn1;		in4kYUV2 = NTV2_XptSDIIn2;
			in4kYUV3 = NTV2_XptSDIIn3;		in4kYUV4 = NTV2_XptSDIIn4;
		}
	}
	
	// make sure formats/modes match for multibuffer modes
	if (b4K || b2FbLevelBHfr)
	{
		mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
		if (b4K)
		{
			mCard->SetMode(NTV2_CHANNEL3, NTV2_MODE_CAPTURE);
			mCard->SetFrameBufferFormat(NTV2_CHANNEL3, mFb1Format);
			
			mCard->SetMode(NTV2_CHANNEL4, NTV2_MODE_CAPTURE);
			mCard->SetFrameBufferFormat(NTV2_CHANNEL4, mFb1Format);
		}
	}
	
	// select square division or 2 pixel interleave in frame buffer
    AdjustFor4kQuadOrTpiIn(inputFormat, b2pi);
	
	// Mixer/Keyer
	mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer2FGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer2FGKeyInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer2BGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer2BGKeyInput, NTV2_XptBlack);


	// Dual Link In 1
	if (bHdmiIn)
	{
		mCard->Connect (NTV2_XptDualLinkIn1Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptDualLinkIn1DSInput, NTV2_XptBlack);
	}
	else
	{
		if (b4K && bInRGB)
		{
			mCard->Connect (NTV2_XptDualLinkIn1Input, bQuadSwap ? NTV2_XptSDIIn3 : NTV2_XptSDIIn1);
			mCard->Connect (NTV2_XptDualLinkIn1DSInput, bQuadSwap ? NTV2_XptSDIIn3DS2 : NTV2_XptSDIIn1DS2);
		}
		else if (bInRGB)
		{
			mCard->Connect (NTV2_XptDualLinkIn1Input, inHdYUV1);
			mCard->Connect (NTV2_XptDualLinkIn1DSInput, inHdYUV2);

			inHdYUV1 = NTV2_XptCSC1VidYUV;
			inHdYUV2 = NTV2_XptBlack;
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkIn1Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptDualLinkIn1DSInput, NTV2_XptBlack);
		}
	}

	// Dual Link In 2
	if (bHdmiIn)
	{
		mCard->Connect (NTV2_XptDualLinkIn2Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptDualLinkIn2DSInput, NTV2_XptBlack);
	}
	else
	{
		if (b4K && bInRGB)
		{
			mCard->Connect (NTV2_XptDualLinkIn2Input, bQuadSwap ? NTV2_XptSDIIn4 : NTV2_XptSDIIn2);
			mCard->Connect (NTV2_XptDualLinkIn2DSInput, bQuadSwap ? NTV2_XptSDIIn4DS2 : NTV2_XptSDIIn2DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkIn2Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptDualLinkIn2DSInput, NTV2_XptBlack);
		}
	}


	// Dual Link In 3
	if (bHdmiIn)
	{
		mCard->Connect (NTV2_XptDualLinkIn3Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptDualLinkIn3DSInput, NTV2_XptBlack);
	}
	else
	{
		if (b4K && bInRGB)
		{
			mCard->Connect (NTV2_XptDualLinkIn3Input, bQuadSwap ? NTV2_XptSDIIn1 : NTV2_XptSDIIn3);
			mCard->Connect (NTV2_XptDualLinkIn3DSInput, bQuadSwap ? NTV2_XptSDIIn1DS2 : NTV2_XptSDIIn3DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkIn3Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptDualLinkIn3DSInput, NTV2_XptBlack);
		}
	}


	// Dual Link In 4
	if (bHdmiIn)
	{
		mCard->Connect (NTV2_XptDualLinkIn4Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptDualLinkIn4DSInput, NTV2_XptBlack);
	}
	else
	{
		if (b4K && bInRGB)
		{
			mCard->Connect (NTV2_XptDualLinkIn4Input, bQuadSwap ? NTV2_XptSDIIn2 : NTV2_XptSDIIn4);
			mCard->Connect (NTV2_XptDualLinkIn4DSInput, bQuadSwap ? NTV2_XptSDIIn2DS2 : NTV2_XptSDIIn4DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkIn4Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptDualLinkIn4DSInput, NTV2_XptBlack);
		}
	}


	// CSC 1
	if (b4K)
	{
		if (bInRGB)
		{
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
		}
		else
		{
			if(bHdmiIn)
			{
				mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptHDMIIn1);
			}
			else
			{
				mCard->Connect (NTV2_XptCSC1VidInput, bQuadSwap ? NTV2_XptSDIIn3 : NTV2_XptSDIIn1);
			}
		}
	}
	else if (!bInRGB)
	{
		if (inputFormat == mFb1VideoFormat)
		{
			mCard->Connect (NTV2_XptCSC1VidInput, inHdYUV1);
		}
		else
		{
			mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptBlack);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptLUT1RGB);
	}


	// CSC 2
	if (b4K)
	{
		if (bInRGB)
		{
			mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptLUT2RGB);
		}
		else if (bQuadSwap)
		{
			mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptSDIIn4);
		}
		else
		{
			mCard->Connect (NTV2_XptCSC2VidInput, in4kYUV2);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptCSC2VidInput, inHdYUV2);
	}


	// CSC 3
	if (b4K)
	{
		if (bInRGB)
		{
			mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptLUT3Out);
		}
		else if (bQuadSwap)
		{
			mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptSDIIn1);
		}
		else
		{
			mCard->Connect (NTV2_XptCSC3VidInput, in4kYUV3);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	}


	// CSC 4
	if (b4K)
	{
		if (bInRGB)
		{
			mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptLUT4Out);
		}
		else if (bQuadSwap)
		{
			mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptSDIIn2);
		}
		else
		{
			mCard->Connect (NTV2_XptCSC4VidInput, in4kYUV4);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);
	}


	// CSC 5
	if (mHasCSC5)
	{
		if (b4K && !b2pi)
		{
			if (bSdiOutRGB)
			{
				mCard->Connect (NTV2_XptCSC5VidInput, NTV2_XptLUT5Out);
			}
			else
			{
				mCard->Connect (NTV2_XptCSC5VidInput, NTV2_Xpt4KDownConverterOut);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptCSC5VidInput, NTV2_XptBlack);
		}
	}


	// LUT 1
	// note b4K processing is same
	if (!bInRGB)
	{
		mCard->Connect (NTV2_XptLUT1Input, NTV2_XptCSC1VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_YUV2RGB);	
	}
	else
	{
		mCard->Connect(NTV2_XptLUT1Input, b4K ? in4kRGB1 : inHdRGB1);

		// if RGB-to-RGB apply LUT converter
		if (bFb1RGB)
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1,					
												 mSDIInput1RGBRange == NTV2_RGBRangeFull ?
												 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
		}
		else
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL1, kLUTBank_RGB2YUV);	
		}
	}


	// LUT 2
	if (!bInRGB)
	{
		mCard->Connect (NTV2_XptLUT2Input, NTV2_XptCSC2VidRGB);
		mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_YUV2RGB);
	}
	else
	{
		mCard->Connect(NTV2_XptLUT2Input, in4kRGB2);
		
		// if RGB-to-RGB apply LUT converter
		if (bFb1RGB)
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2,						
												 mSDIInput1RGBRange == NTV2_RGBRangeFull ?
												 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
		}
		else
		{
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL2, kLUTBank_RGB2YUV);	
		}
	}
	

	// LUT 3
	if (b4K)
	{
		if (!bInRGB)
		{
			mCard->Connect (NTV2_XptLUT3Input, NTV2_XptCSC3VidRGB);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL3, kLUTBank_YUV2RGB);
		}
		else 
		{
			mCard->Connect(NTV2_XptLUT3Input, in4kRGB3);

			// if RGB-to-RGB apply LUT converter
			if (bFb1RGB)
			{
				mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL3,					
													 mSDIInput1RGBRange == NTV2_RGBRangeFull ?
													 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
			}
			else
			{
				mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL3, kLUTBank_RGB2YUV);	
			}
		}
	}
	else
	{
		mCard->Connect (NTV2_XptLUT3Input, NTV2_XptBlack);
	}


	// LUT 4
	if (b4K)
	{
		if (!bInRGB)
		{
			mCard->Connect (NTV2_XptLUT4Input, NTV2_XptCSC4VidRGB);
			mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL4, kLUTBank_YUV2RGB);
		}
		else
		{
			mCard->Connect(NTV2_XptLUT4Input, in4kRGB4);

			// if RGB-to-RGB apply LUT converter
			if (bFb1RGB)
			{
				mCard->SetColorCorrectionOutputBank( NTV2_CHANNEL4,					
													 mSDIInput1RGBRange == NTV2_RGBRangeFull ?
													 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
			}
			else
			{
				mCard->SetColorCorrectionOutputBank (NTV2_CHANNEL4, kLUTBank_RGB2YUV);	
			}
		}
	}
	else
	{
		mCard->Connect (NTV2_XptLUT4Input, NTV2_XptBlack);
	}


	// LUT 5
	if (mHasLUT5)
	{
		if (b4K && !b2pi)
		{
			if (bSdiOutRGB)
			{
				mCard->Connect (NTV2_XptLUT5Input, NTV2_Xpt4KDownConverterOutRGB);
			}
			else
			{
				mCard->Connect (NTV2_XptLUT5Input, NTV2_XptCSC5VidRGB);
			}
			
			if (bSdiOutRGB && bHdmiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank( NTV2_CHANNEL5, mRGB10Range == NTV2_RGB10RangeFull ?
													 kLUTBank_FULL2SMPTE : kLUTBank_SMPTE2FULL);
			}
			else if (bSdiOutRGB && !bHdmiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL5, kLUTBank_RGB2YUV);
			}
			else if (!bSdiOutRGB && bHdmiOutRGB)
			{
				mCard->SetColorCorrectionOutputBank(NTV2_CHANNEL5, kLUTBank_YUV2RGB);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptLUT5Input, NTV2_XptBlack);
		}
	}
	

	// Dual Link Out 1
	if (!bSdiOutRGB || !b4K)
	{
		mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptBlack);
	}
	else if (b4K)
	{
		if (bInRGB)
		{
			if (bFb1HdrRGB)
			{
				mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptDuallinkIn1);
			}
			else if (bFb1RGB)
			{
				mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptDualLinkOut1Input, in4kRGB1);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptLUT1RGB);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptBlack);
	}
	
	
	// Dual Link Out 2
	if (!bSdiOutRGB || !b4K)
	{
		mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptBlack);
	}
	else if (b4K)
	{
		if (bInRGB)
		{
			if (bFb1HdrRGB)
			{
				mCard->Connect (NTV2_XptDualLinkOut2Input, b4K ? NTV2_XptDuallinkIn2 : NTV2_XptDuallinkIn1);
			}
			else if (bFb1RGB)
			{
				mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptLUT2RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptDualLinkOut2Input, in4kRGB2);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptLUT2RGB);
		}
	}
	else
	{
		mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptBlack);
	}
	
	
	// Dual Link Out 3
	if (!bSdiOutRGB)
	{
		mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptBlack);
	}
	else
	{
		if (bInRGB)
		{
			if (bFb1HdrRGB)
			{
				mCard->Connect (NTV2_XptDualLinkOut3Input, b4K ? NTV2_XptDuallinkIn3 : NTV2_XptDuallinkIn1);
			}
			else if (bFb1RGB)
			{
				mCard->Connect (NTV2_XptDualLinkOut3Input, b4K ? NTV2_XptLUT3Out : NTV2_XptLUT1RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptDualLinkOut3Input, b4K ? in4kRGB3 : inHdRGB1);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut3Input, b4K ? NTV2_XptLUT3Out : NTV2_XptLUT1RGB);
		}
	}
	
	
	
	// Dual Link Out 4
	if (!bSdiOutRGB)
	{
		mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptBlack);
	}
	else
	{
		if (bInRGB)
		{
			if (bFb1HdrRGB)
			{
				mCard->Connect (NTV2_XptDualLinkOut4Input, b4K ? NTV2_XptDuallinkIn4 : NTV2_XptDuallinkIn1);
			}
			else if (bFb1RGB)
			{
				mCard->Connect (NTV2_XptDualLinkOut4Input, b4K ? NTV2_XptLUT4Out : NTV2_XptLUT1RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptDualLinkOut4Input, b4K ? in4kRGB4 : inHdRGB1);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptDualLinkOut4Input, b4K ? NTV2_XptLUT4Out : NTV2_XptLUT1RGB);
		}
	}


	// Dual Link Out 5
	if (!bSdiOutRGB)
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, NTV2_XptBlack);
	}
	else if (b4K && !b2pi) // RGB-Quad
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, bSdiOutRGB ? NTV2_Xpt4KDownConverterOutRGB : NTV2_XptBlack);
	}
	else if (b4K && b2pi) // RGB-425
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, bInRGB ? in4kRGB1 : NTV2_XptLUT1RGB);
	}
	else
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, bInRGB ? inHdRGB1 : NTV2_XptLUT1RGB);
	}


	// 425 Mux
	if (b4K && b2pi)
	{
		if (bFb1RGB)
		{
			if (bInRGB)
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, in4kRGB1);
				mCard->Connect(NTV2_Xpt425Mux1BInput, in4kRGB2);
				mCard->Connect(NTV2_Xpt425Mux2AInput, in4kRGB3);
				mCard->Connect(NTV2_Xpt425Mux2BInput, in4kRGB4);
			}
			else
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptLUT1RGB);
				mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptLUT2RGB);
				mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptLUT3Out);
				mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptLUT4Out);
			}
		}
		else // FB-YUV
		{
			if (bInRGB)
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, NTV2_XptCSC1VidYUV);
				mCard->Connect(NTV2_Xpt425Mux1BInput, NTV2_XptCSC2VidYUV);
				mCard->Connect(NTV2_Xpt425Mux2AInput, NTV2_XptCSC3VidYUV);
				mCard->Connect(NTV2_Xpt425Mux2BInput, NTV2_XptCSC4VidYUV);
			}
			else 
			{
				mCard->Connect(NTV2_Xpt425Mux1AInput, in4kYUV1);
				mCard->Connect(NTV2_Xpt425Mux1BInput, in4kYUV2);
				mCard->Connect(NTV2_Xpt425Mux2AInput, in4kYUV3);
				mCard->Connect(NTV2_Xpt425Mux2BInput, in4kYUV4);
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
	if (b2pi)
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
	else if (b4K) // 4k-quad
	{
		if (bFb1RGB) // 4k-Quad-RGB
		{
			if (bInRGB)
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, bFb1HdrRGB ? in4kRGB1 : NTV2_XptLUT1RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);
			}
		}
		else // 4K-Quad-YUV
		{
			if (b2xQuadIn)
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
			}
			else if (bInRGB)
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCSC1VidYUV);
				mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_XptBlack);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, bQuadSwap ? NTV2_XptSDIIn3 : NTV2_XptSDIIn1);
				mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_XptBlack);
			}
		}
		mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_XptBlack);

	}
	else if (b2FbLevelBHfr)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, inHdYUV1);
		mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_XptBlack);
	}
	else if (bFb1RGB)
	{
		if (bInRGB)
		{
			if (bFb1HdrRGB)
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptDuallinkIn1);
			}
			else if (mSDIInput1RGBRange == frambBufferRange && mLUTType != NTV2_LUTCustom)
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, bHdmiIn ? NTV2_XptHDMIIn1 : NTV2_XptDuallinkIn1);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);
			}
		}
		else
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptLUT1RGB);
		}
		mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_XptBlack);
	}
	else
	{
		if (bInRGB)
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptCSC1VidYUV);
		}
		else
		{
			mCard->Connect (NTV2_XptFrameBuffer1Input, bHdmiIn ? NTV2_XptHDMIIn1 : inHdYUV1);
		}
		mCard->Connect(NTV2_XptFrameBuffer1BInput, NTV2_XptBlack);
	}


	// Frame Buffer 2
	if (b2pi)
	{
		if (bFb1RGB) // 4K-2pi-RGB
		{
			mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2ARGB);
			mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BRGB);
		}
		else // 4K-2pi-YUV
		{
			mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_Xpt425Mux2AYUV);
			mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_Xpt425Mux2BYUV);
		}
	}
	else if (b4K) // 4K-Quad
	{
		if (bFb1RGB) // 4K-Quad-RGB
		{
			if (bInRGB)
			{
				mCard->Connect (NTV2_XptFrameBuffer2Input, bFb1HdrRGB ? in4kRGB2 : NTV2_XptLUT2RGB);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptLUT2RGB);
			}
		}
		else // 4K-Quad-YUV
		{
			if (b2xQuadIn)
			{
				mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptSDIIn1DS2);
			}
			else if (bInRGB)
			{
				mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptCSC2VidYUV);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer2Input, bQuadSwap ? NTV2_XptSDIIn4 : NTV2_XptSDIIn2);
			}
		}
		mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_XptBlack);
	}
	else if (b2FbLevelBHfr)
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, inHdYUV2);
		mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_XptBlack);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
		mCard->Connect(NTV2_XptFrameBuffer2BInput, NTV2_XptBlack);
	}


	// Frame Buffer 3
	if (b4K && !b2pi)
	{
		if (bFb1RGB) // 4k-Quad-RGB
		{
			if (bInRGB)
			{
				mCard->Connect (NTV2_XptFrameBuffer3Input, bFb1HdrRGB ? in4kRGB3 : NTV2_XptLUT3Out);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptLUT3Out);				// CSC converted
			}
			mCard->Connect(NTV2_XptFrameBuffer3BInput, NTV2_XptBlack);
		}
		else // 4k-Quad-YUV
		{
			if (b2xQuadIn)
			{
				mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptSDIIn2);
			}
			else if (bInRGB)
			{
				mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptCSC3VidYUV);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer3Input, bQuadSwap ? NTV2_XptSDIIn1 : NTV2_XptSDIIn3);
			}
		}
		mCard->Connect (NTV2_XptFrameBuffer3BInput, NTV2_XptBlack);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptFrameBuffer3BInput, NTV2_XptBlack);
	}


	// Frame Buffer 4
	if (b4K && !b2pi)
	{
		if (bFb1RGB)
		{
			if (bInRGB) // 4k-Quad-RGB
			{
				mCard->Connect (NTV2_XptFrameBuffer4Input, bFb1HdrRGB ? in4kRGB4 : NTV2_XptLUT4Out);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptLUT4Out);				// CSC converted
			}
		}
		else // 4k-Quad-YUV
		{
			if (b2xQuadIn)
			{
				mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptSDIIn2DS2);
			}
			else if (bInRGB)
			{
				mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptCSC4VidYUV);
			}
			else
			{
				mCard->Connect (NTV2_XptFrameBuffer4Input, bQuadSwap ? NTV2_XptSDIIn2 : NTV2_XptSDIIn4);
			}
		}
		mCard->Connect (NTV2_XptFrameBuffer4BInput, NTV2_XptBlack);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptFrameBuffer4BInput, NTV2_XptBlack);
	}


	// Frame Buffer Disabling
	if (b2FbLevelBHfr)
	{
		bFb1Disable = bFb2Disable = false;
	}
	else if (b4K)
	{
		if (b2pi)
		{
			bFb1Disable = bFb2Disable = 0;
		}
		else
		{
			bFb1Disable = bFb2Disable = bFb3Disable = bFb4Disable = 0;
		}
	}
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh3Control, bFb3Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh4Control, bFb4Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	//No need to enable these
	mCard->DisableChannel(NTV2_CHANNEL5);
	mCard->DisableChannel(NTV2_CHANNEL6);
	mCard->DisableChannel(NTV2_CHANNEL7);
	mCard->DisableChannel(NTV2_CHANNEL8);


	// 4K Down Converter
	XPt1 = XPt2 = XPt3 = XPt4 = NTV2_XptBlack;
	// DC applies only to 4K Quad - for now
	if (b4K && !b2pi)
	{
		// SDIOut-RGB
		if (bSdiOutRGB)
		{
			XPt1 = bQuadSwap ? NTV2_XptLUT3Out : NTV2_XptLUT1RGB;
			XPt2 = bQuadSwap ? NTV2_XptLUT4Out : NTV2_XptLUT2RGB;
			XPt3 = bQuadSwap ? NTV2_XptLUT1RGB : NTV2_XptLUT3Out;
			XPt4 = bQuadSwap ? NTV2_XptLUT2RGB : NTV2_XptLUT4Out;
			mCard->Enable4KDCRGBMode(true);
		}
		// SDIOut-YUV
		else
		{
			if (bQuadSwap)
			{
				XPt1 =  bInRGB ? NTV2_XptCSC3VidYUV : in4kYUV3;
				XPt2 =  bInRGB ? NTV2_XptCSC4VidYUV : in4kYUV4;
				XPt3 =  bInRGB ? NTV2_XptCSC1VidYUV : in4kYUV1;
				XPt4 =  bInRGB ? NTV2_XptCSC2VidYUV : in4kYUV2;
			}
			else
			{
				XPt1 =  bInRGB ? NTV2_XptCSC1VidYUV : in4kYUV1;
				XPt2 =  bInRGB ? NTV2_XptCSC2VidYUV : in4kYUV2;
				XPt3 =  bInRGB ? NTV2_XptCSC3VidYUV : in4kYUV3;
				XPt4 =  bInRGB ? NTV2_XptCSC4VidYUV : in4kYUV4;
			}
			mCard->Enable4KDCRGBMode(false);
		}
	}
	mCard->Connect (NTV2_Xpt4KDCQ1Input, XPt1);
	mCard->Connect (NTV2_Xpt4KDCQ2Input, XPt2);
	mCard->Connect (NTV2_Xpt4KDCQ3Input, XPt3);
	mCard->Connect (NTV2_Xpt4KDCQ4Input, XPt4);



	// SDI Out 1
	if (b4K)
	{
		if (bSdiOutRGB)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptDuallinkOut1);
			mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptDuallinkOut1DS2);
		}
		else
		{
			if (b2x2piOut || b2xQuadOut || b2xQuadIn || b2x2piIn)
			{
				mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
				mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
			}
			else if (bInRGB)
			{
				mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptCSC1VidYUV);
				mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut1Input, in4kYUV1);
				mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
			}
		}
	}
	else
	{
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	}


	// SDI Out 2
	XPt1 = XPt2 = NTV2_XptBlack;
	if (b4K)
	{
		if (bSdiOutRGB)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptDuallinkOut2);
			mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptDuallinkOut2DS2);
		}
		else // YUV
		{
			if (b2x2piOut || b2xQuadOut || b2xQuadIn || b2x2piIn)
			{
				mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
				mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
			}
			else if (bInRGB)
			{
				mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptCSC2VidYUV);
				mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut2Input, in4kYUV2);
				mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
			}
		}
	}
	else
	{
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
	}

	


	// SDI Out 3 - acts like SDI 1
	if (b4K)
	{
		if (bSdiOutRGB)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptDuallinkOut3DS2);
		}
		else // YUV
		{
			if (b2x2piOut || b2xQuadOut || b2xQuadIn || b2x2piIn)
			{
				if (bInRGB)
				{
					mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptCSC2VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut3Input, in4kYUV1);
					mCard->Connect (NTV2_XptSDIOut3InputDS2, in4kYUV2);
				}
			}
			else if (bInRGB)
			{
				mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC3VidYUV);
				mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut3Input, in4kYUV3);
				mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
			}
		}
	}
	else if (IsVideoFormatB(mFb1VideoFormat) ||											// p60b
			 mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, inHdYUV1);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, inHdYUV2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut3Input, inHdYUV1);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
		}
	}
	else if (bSdiOutRGB)				// Same as RGB in this case
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);				// 1 3Gb wire
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptDuallinkOut3DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptDuallinkOut3);				// 2 wires
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
		}
	}
	else // NTV2_PrimaryOutputSelect													// Primary
	{
		if (bInRGB)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC1VidYUV);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut3Input, bHdmiIn ? NTV2_XptHDMIIn1 : inHdYUV1);
		}
		mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
	}


	// SDI Out 4 - acts like SDI 2
	if (b4K)
	{
		if (bSdiOutRGB)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut4);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptDuallinkOut4DS2);
		}
		else // YUV
		{
			if (b2x2piOut || b2xQuadOut || b2xQuadIn || b2x2piIn)
			{
				if (bInRGB)
				{
					mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC3VidYUV);
					mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptCSC4VidYUV);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut4Input, in4kYUV3);
					mCard->Connect (NTV2_XptSDIOut4InputDS2, in4kYUV4);
				}
			}
			else if (bInRGB)
			{
				mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC4VidYUV);
				mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut4Input, in4kYUV4);
				mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
			}
		}
	}
	else if (IsVideoFormatB(mFb1VideoFormat) ||									// p60b
			 mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect)			// Video + Key
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, inHdYUV1);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, inHdYUV2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut4Input, inHdYUV2);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
	}
	else if (bSdiOutRGB)			// Same as RGB in this case
	{
		if (b3GbOut)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut4);		// 1 3Gb wire
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptDuallinkOut4DS2);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptDuallinkOut3DS2);		// 2 wires
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
	}
	else // NTV2_PrimaryOutputSelect											// Primary
	{
		if (bInRGB)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC1VidYUV);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut4Input, bHdmiIn ? NTV2_XptHDMIIn1 : inHdYUV1);
		}
		mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
	}


	// SDI Out 5
	if (bSdiOutRGB)			// RGB Out - alway use DL
	{
		mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptDuallinkOut5);
		mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptDuallinkOut5DS2);
	}
	// YUV Out 4K Quads 
	else if (b4K && !b2pi)
	{
		mCard->Connect (NTV2_XptSDIOut5Input, NTV2_Xpt4KDownConverterOut);
		mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
	}
	// YUV Out 4K 2pi 
	else if (b4K && b2pi)
	{
		if (b2x2piOut || b2xQuadOut || b2xQuadIn || b2x2piIn)
		{
			if (bInRGB)
			{
				mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC1VidYUV);
				mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut5Input, in4kYUV1);
				mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
			}
		}
		else if (bInRGB)
		{
			mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC1VidYUV);
			mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut5Input, in4kYUV1);
			mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
		}
	}
	// LevelB
	else if (b2FbLevelBHfr)											
	{
		mCard->Connect (NTV2_XptSDIOut5Input, inHdYUV1);
		mCard->Connect (NTV2_XptSDIOut5InputDS2, inHdYUV2);
	}
	// YUV Out
	else
	{
		if (bInRGB)
		{
			mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC1VidYUV);
		}
		
		// FB-YUV 
		else
		{
			mCard->Connect (NTV2_XptSDIOut5Input, inHdYUV1);
		}
		
		mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
	}


	// HDMI Out
	if (mHasHdmiOut)
	{
		XPt1 = XPt2 = XPt3 = XPt4 = NTV2_XptBlack;
		if (b4K)
		{
			// 2si mode
			if (b2pi)
			{
				// RGB-mode set
				if (bHdmiOutRGB)
				{
					switch (mVirtualHDMIOutputSelect)
					{
					default:
					case NTV2_PrimaryOutputSelect:
					case NTV2_4kHalfFrameRate:		// unsupported for now
						XPt1 = bInRGB ? in4kRGB1 : NTV2_XptLUT1RGB;
						XPt2 = bInRGB ? in4kRGB2 : NTV2_XptLUT2RGB;
						XPt3 = bInRGB ? in4kRGB3 : NTV2_XptLUT3Out;
						XPt4 = bInRGB ? in4kRGB4 : NTV2_XptLUT4Out;
						break;
					case NTV2_Quarter4k:       XPt1 = bInRGB ? in4kRGB1 : NTV2_XptLUT1RGB; break;
					case NTV2_Quadrant1Select: XPt1 = bInRGB ? in4kRGB1 : NTV2_XptLUT1RGB; break;
					case NTV2_Quadrant2Select: XPt1 = bInRGB ? in4kRGB2 : NTV2_XptLUT2RGB; break;
					case NTV2_Quadrant3Select: XPt1 = bInRGB ? in4kRGB3 : NTV2_XptLUT3Out; break;
					case NTV2_Quadrant4Select: XPt1 = bInRGB ? in4kRGB4 : NTV2_XptLUT4Out; break;
					}
				}
				
				// YUV-mode set
				else 
				{
					switch (mVirtualHDMIOutputSelect)
					{
					default:
					case NTV2_PrimaryOutputSelect:
					case NTV2_4kHalfFrameRate:		// unsupported for now
						XPt1 = bInRGB ? NTV2_XptCSC1VidYUV : in4kYUV1;
						XPt2 = bInRGB ? NTV2_XptCSC2VidYUV : in4kYUV2;
						XPt3 = bInRGB ? NTV2_XptCSC3VidYUV : in4kYUV3;
						XPt4 = bInRGB ? NTV2_XptCSC4VidYUV : in4kYUV4;
						break;
					case NTV2_Quarter4k:       XPt1 = bInRGB ? NTV2_XptCSC1VidYUV : in4kYUV1; break;
					case NTV2_Quadrant1Select: XPt1 = bInRGB ? NTV2_XptCSC1VidYUV : in4kYUV1; break;
					case NTV2_Quadrant2Select: XPt1 = bInRGB ? NTV2_XptCSC2VidYUV : in4kYUV2; break;
					case NTV2_Quadrant3Select: XPt1 = bInRGB ? NTV2_XptCSC3VidYUV : in4kYUV3; break;
					case NTV2_Quadrant4Select: XPt1 = bInRGB ? NTV2_XptCSC4VidYUV : in4kYUV4; break;
					}
				}
			}
			
			// quadrant mode
			else 
			{
				// RGB-mode set
				if (bHdmiOutRGB)
				{
					switch (mVirtualHDMIOutputSelect)
					{
					default:
					case NTV2_PrimaryOutputSelect:
					case NTV2_4kHalfFrameRate:		// unsupported for now
					case NTV2_Quarter4k:       
						XPt1 = bSdiOutRGB ? NTV2_Xpt4KDownConverterOutRGB : NTV2_XptLUT5Out;
						break;
					case NTV2_Quadrant1Select: XPt1 = NTV2_XptLUT1RGB; break;
					case NTV2_Quadrant2Select: XPt1 = NTV2_XptLUT2RGB; break;
					case NTV2_Quadrant3Select: XPt1 = NTV2_XptLUT3Out; break;
					case NTV2_Quadrant4Select: XPt1 = NTV2_XptLUT4Out; break;
					}
				}
				
				// YUV-mode set
				else 
				{
					switch (mVirtualHDMIOutputSelect)
					{
					default:
					case NTV2_PrimaryOutputSelect:
					case NTV2_4kHalfFrameRate:		// unsupported for now
					case NTV2_Quarter4k:
						XPt1 = bSdiOutRGB ? NTV2_XptCSC5VidYUV :  NTV2_Xpt4KDownConverterOut;
						break;
					case NTV2_Quadrant1Select: XPt1 = NTV2_XptCSC1VidYUV; break;
					case NTV2_Quadrant2Select: XPt1 = NTV2_XptCSC2VidYUV; break;
					case NTV2_Quadrant3Select: XPt1 = NTV2_XptCSC3VidYUV; break;
					case NTV2_Quadrant4Select: XPt1 = NTV2_XptCSC4VidYUV; break;
					}
				}
			}
		}
		else if (b2FbLevelBHfr)
		{
			// LevelB
			XPt1 = NTV2_XptLUT1RGB;
			XPt2 = NTV2_XptLUT2RGB;
		}
		else
		{
			if (bFb1HdrRGB)
			{
				XPt1 = NTV2_XptDuallinkIn1;
			}
			else if (bHdmiOutRGB)
			{
				XPt1 = bInRGB ? inHdRGB1 : NTV2_XptLUT1RGB;
			}
			else
			{
				if (bInRGB)
				{
					XPt1 = NTV2_XptCSC1VidYUV;
				}
				
				// FB-YUV 
				else
				{
					XPt1 = inHdYUV1;
				}
			}
		}
		mCard->Connect (NTV2_XptHDMIOutInput,	XPt1);
		mCard->Connect (NTV2_XptHDMIOutQ2Input, XPt2);
		mCard->Connect (NTV2_XptHDMIOutQ3Input, XPt3);
		mCard->Connect (NTV2_XptHDMIOutQ4Input, XPt4);

		// 4K Hdmi-to-Hdmi Bypass always disabled for playback
		mCard->WriteRegister(kRegHDMIOutControl, false, kRegMaskHDMIV2TxBypass, kRegShiftHDMIV2TxBypass);
	}
	
	
	// Analog Out
	if (mHasAnalogOut)
	{
		if (b4K)
		{
			switch (mVirtualAnalogOutputSelect)
			{
			default:
			case NTV2_Quarter4k:
				if (b2pi)
					mCard->Connect (NTV2_XptAnalogOutInput, bSdiOutRGB ? NTV2_XptCSC1VidYUV : in4kYUV1); 
				else
					mCard->Connect (NTV2_XptAnalogOutInput, bSdiOutRGB ? NTV2_XptCSC5VidYUV : NTV2_Xpt4KDownConverterOut); 
				break;
			};
		}
		else
		{
			mCard->Connect (NTV2_XptAnalogOutInput, inHdYUV1);
		}
	}
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void Class4kServices::SetDeviceMiscRegisters ()
{
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();
	
	NTV2Standard		primaryStandard;
	NTV2FrameGeometry 	primaryGeometry;
	mCard->GetStandard(primaryStandard);
	mCard->GetFrameGeometry(primaryGeometry);
	
	bool				bHdmiIn				= mDs.bInHdmi;
	bool				bFbLevelA			= IsVideoFormatA(mFb1VideoFormat);
	bool				b4K					= NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	bool				b4kHfr				= NTV2_IS_4K_HFR_VIDEO_FORMAT(mFb1VideoFormat);
	bool				bHfr				= NTV2_IS_3G_FORMAT(mFb1VideoFormat);
	bool				bSdiOutRGB			= mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb;
	bool				b3GaOutRGB			= (mSdiOutTransportType == NTV2_SDITransport_3Ga) && bSdiOutRGB;
	bool				b4k6gOut			= b4K && !b4kHfr && !bSdiOutRGB && (m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire);
	bool				b4k12gOut			= b4K && (b4kHfr || bSdiOutRGB) && (m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire);
	NTV2FrameRate		primaryFrameRate	= GetNTV2FrameRateFromVideoFormat(mFb1VideoFormat);
	
	bHdmiIn									= bHdmiIn && mHasHdmiIn;
	b4k6gOut								= b4k6gOut && mHas12G;
	b4k12gOut								= b4k12gOut && mHas12G;
	
	// single wire 3Gb out
	// 1x3Gb = !4k && (rgb | v+k | 3d | (hfra & 3gb) | hfrb)
	bool b1x3GbOut =		(b4K == false) &&
							((bSdiOutRGB == true) ||
							 (mVirtualDigitalOutput1Select == NTV2_VideoPlusKeySelect) ||
							 (bFbLevelA == true && mSdiOutTransportType == NTV2_SDITransport_DualLink_3Gb) ||
							 (IsVideoFormatB(mFb1VideoFormat) == true)  );

	bool b2xQuadOut = (    ((mFb1Mode != NTV2_MODE_CAPTURE) && (b4K && !b4kHfr && m4kTransportOutSelection == NTV2_4kTransport_Quadrants_2wire))
		                 || ((mFb1Mode == NTV2_MODE_CAPTURE) && bHdmiIn && b4K && !b4kHfr && m4kTransportOutSelection == NTV2_4kTransport_Quadrants_2wire));
	bool b2xQuadIn =  (mFb1Mode == NTV2_MODE_CAPTURE) && (b4K && !b4kHfr && mVirtualInputSelect  == NTV2_Input2x4kSelect);
	
	// all 3Gb transport out
	// b3GbOut = (b1x3GbOut + !2wire) | (4k + rgb) | (4khfr + 3gb)
	bool b3GbOut 	= (b1x3GbOut == true && mSdiOutTransportType != NTV2_SDITransport_DualLink_1_5) ||
					  (b4K == true && bSdiOutRGB == true) ||
					  (b4kHfr == true && mSdiOutTransportType == NTV2_SDITransport_DualLink_3Gb) ||
					   b2xQuadOut || 
					   b2xQuadIn;
					   
	bool b2pi 		= b4K && (m4kTransportOutSelection == NTV2_4kTransport_PixelInterleave ||
						  	  m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire);
	
	// SDI Transmit
	for (int i=0; i<4; i++)
		mCard->SetSDITransmitEnable((NTV2Channel)i, mDs.sdiOut[i]->isOut);
	
	// SDI In levelB -> levelA conversion
	for (int i=0; i<4; i++)
	{
		if (mDs.sdiIn[i]->isOut == false)
		{
			bool bConvertBToA = bFbLevelA && mDs.sdiIn[i]->is3Gb;
			mCard->SetSDIInLevelBtoLevelAConversion(i, bConvertBToA);
		}
	}
	
	if (b4k12gOut)
	{
		mCard->SetSDIOut12GEnable(NTV2_CHANNEL3, true);
	}
	else if (b4k6gOut)
	{
		mCard->SetSDIOut6GEnable(NTV2_CHANNEL3, true);
	}
	else
	{
		mCard->SetSDIOut6GEnable(NTV2_CHANNEL3, false);
		mCard->SetSDIOut12GEnable(NTV2_CHANNEL3, false);
	}
	
	// HDMI Out 
	if (mHasHdmiOut)
	{
		// set standard / mode
		NTV2Standard v2Standard = GetHdmiV2StandardFromVideoFormat(mFb1VideoFormat);
		NTV2FrameRate rate = GetNTV2FrameRateFromVideoFormat(mFb1VideoFormat);
		
		if (b4K && mFb1Mode == NTV2_MODE_CAPTURE && bHdmiIn)
		{
			// 4K mode and doing capture and HDMI is selected as input
			mCard->SetHDMIV2Mode(NTV2_HDMI_V2_4K_CAPTURE);
			
			// 4K mode downconverted
			if (mVirtualHDMIOutputSelect == NTV2_Quarter4k)
			{
				switch(v2Standard)
				{
				case NTV2_STANDARD_3840x2160p:
				case NTV2_STANDARD_3840HFR:
					v2Standard = NTV2_STANDARD_1080p;
					break;
				case NTV2_STANDARD_4096x2160p:
				case NTV2_STANDARD_4096HFR:
					v2Standard = NTV2_STANDARD_2Kx1080p;
					break;
				default:
					break;
				}
			}
		}
		else if (b4K == true)
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
					switch(v2Standard)
					{
						case NTV2_STANDARD_3840x2160p:
						case NTV2_STANDARD_3840HFR:
							v2Standard = NTV2_STANDARD_1080p;
							break;
						case NTV2_STANDARD_4096x2160p:
						case NTV2_STANDARD_4096HFR:
							v2Standard = NTV2_STANDARD_2Kx1080p;
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
			switch (v2Standard)
			{
				case NTV2_STANDARD_3840x2160p:
				case NTV2_STANDARD_3840HFR:
					v2Standard = NTV2_STANDARD_1080p;
					break;
				case NTV2_STANDARD_4096x2160p:
				case NTV2_STANDARD_4096HFR:
					v2Standard = NTV2_STANDARD_2Kx1080p;
					break;
				case NTV2_STANDARD_1080:
					switch(rate)
					{
					case NTV2_FRAMERATE_2398:
					case NTV2_FRAMERATE_2400:
						v2Standard = NTV2_STANDARD_1080p;
						break;
					default:
						break;
					}
				default:
					break;
			}
		}

		// enable/disable two sample interleave i/o
		if (b2pi)
		{
			if (mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect || mVirtualHDMIOutputSelect == NTV2_4kHalfFrameRate)
				mCard->SetHDMIOutTsiIO(true);
			else
				mCard->SetHDMIOutTsiIO(false);
		}
		else
		{
			mCard->SetHDMIOutTsiIO(false);
		}
		
		// HFPS
		if (mVirtualHDMIOutputSelect == NTV2_4kHalfFrameRate)
		{
			bool bDecimate = b4kHfr;
			switch(v2Standard)
			{
			case NTV2_STANDARD_4096HFR: v2Standard = NTV2_STANDARD_4096x2160p; break;
			case NTV2_STANDARD_3840HFR: v2Standard = NTV2_STANDARD_3840x2160p; break;
			default: break;
			}
			
			//mCard->SetHDMIOutVideoFPS(tempRate);
			mCard->SetHDMIOutDecimateMode(bDecimate);
			mCard->SetHDMIOutLevelBMode(IsVideoFormatB(mFb1VideoFormat));
		}
		else
		{	
			mCard->SetHDMIOutVideoFPS(primaryFrameRate);
			mCard->SetHDMIOutDecimateMode(false);
			mCard->SetHDMIOutLevelBMode(IsVideoFormatB(mFb1VideoFormat));
		}
		
		// color space sample rate
		if (mDs.hdmiOutColorSpace == kHDMIOutCSCYCbCr8bit ||
			mDs.hdmiOutColorSpace == kHDMIOutCSCYCbCr10bit)
		{
			if (b4kHfr == true && mVirtualHDMIOutputSelect == NTV2_PrimaryOutputSelect)
				mCard->SetHDMIOutSampleStructure(NTV2_HDMI_YC420);
			else
				mCard->SetHDMIOutSampleStructure(NTV2_HDMI_YC422);
		}
		else // rgb
		{
			mCard->SetHDMIOutSampleStructure(NTV2_HDMI_RGB);
		}
		
		// set color-space bit-depth 
		switch (mDs.hdmiOutColorSpace)
		{
			case kHDMIOutCSCYCbCr8bit:
				mCard->SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
				mCard->SetHDMIOutBitDepth(NTV2_HDMI8Bit);
				break;
		
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
		switch (mDs.hdmiOutProtocol_)
		{
			default:
			case kHDMIOutProtocolAutoDetect:
				mCard->WriteRegister(kRegHDMIOutControl, mDs.hdmiOutDsProtocol, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
				break;
				
			case kHDMIOutProtocolHDMI:
				mCard->WriteRegister (kRegHDMIOutControl, NTV2_HDMIProtocolHDMI, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
				break;
				
			case kHDMIOutProtocolDVI:
				mCard->WriteRegister (kRegHDMIOutControl, NTV2_HDMIProtocolDVI, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
				break;
		}
		
		// HDMI Out rgb range
		switch (mDs.hdmiOutRange)
		{
			default:
			case NTV2_RGBRangeSMPTE:	mCard->SetHDMIOutRange(NTV2_HDMIRangeSMPTE);	break;
			case NTV2_RGBRangeFull:		mCard->SetHDMIOutRange(NTV2_HDMIRangeFull);		break;
		}
		
		// HDMI Out Stereo - false
		mCard->SetHDMIOut3DPresent(false);
	}
	
	// HDMI In 
	if (mHasHdmiIn)
	{
		// HDMI In rgb range
		switch (mDs.hdmiInRange)
		{
			default:
			case NTV2_RGBRangeSMPTE:	mCard->SetHDMIInputRange(NTV2_HDMIRangeSMPTE);	break;
			case NTV2_RGBRangeFull:		mCard->SetHDMIInputRange(NTV2_HDMIRangeFull);	break;
		}
	}
	
	
	//
	// Analog-Out
	//
	
	// Control video DAC mode
	// Note: the hardware takes a video "hit" every time we write a value to this register - whether or not
	// the new value is different. So we're going to filter it ourselves and ONLY write a value if it
	// has actually changed. 
	if (mHasAnalogOut)
	{
		NTV2LHIVideoDACMode curr2Mode, new2Mode;
		NTV2Standard curr2Standard, new2Standard;
		
		// get current value
		mCard->GetLHIVideoDACMode (curr2Mode);	
		mCard->GetLHIVideoDACStandard (curr2Standard);
		
		// Select DAC mode from framebufferformat
		new2Mode = GetLHIVideoDACMode (mFb1VideoFormat, mVirtualAnalogOutputType, mVirtualAnalogOutBlackLevel);
		new2Standard = primaryStandard;
		
		// write it only if the new value is different
		if (curr2Mode != new2Mode)
			mCard->SetLHIVideoDACMode (new2Mode);
		if (curr2Standard != new2Standard)
			mCard->SetLHIVideoDACStandard (new2Standard);
	}
	
	// 4K Down Converter
	bool bPsf = IsPSF(mFb1VideoFormat);
	mCard->Enable4KDCPSFInMode(bPsf);
	mCard->Enable4KPSFOutMode(bPsf);

	// special case - VANC 8bit pixel shift support
	if (mVANCMode && Is8BitFrameBufferFormat(mFb1Format) )
		mCard->WriteRegister(kRegCh1Control, 1, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	else
		mCard->WriteRegister(kRegCh1Control, 0, kRegMaskVidProcVANCShift, kRegShiftVidProcVANCShift);
	
	//
	// SDI Out
	//
	
	// Level A to B conversion
	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL1, bFbLevelA && b3GbOut);
	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL2, bFbLevelA && b3GbOut);
	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL3, (bFbLevelA && b3GbOut) || ((mFb1Mode == NTV2_MODE_CAPTURE) && bHdmiIn && b4K && !b4kHfr && m4kTransportOutSelection == NTV2_4kTransport_Quadrants_2wire));
	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL4, (bFbLevelA && b3GbOut) || ((mFb1Mode == NTV2_MODE_CAPTURE) && bHdmiIn && b4K && !b4kHfr && m4kTransportOutSelection == NTV2_4kTransport_Quadrants_2wire));
	bool sdi5_3GbTransportOut = false;
	if (b4K)
	{
		if (b4kHfr)
			sdi5_3GbTransportOut = 	(mSdiOutTransportType == NTV2_SDITransport_DualLink_3Gb) ||
									(mSdiOutTransportType == NTV2_SDITransport_OctLink_3Gb);
		else
			sdi5_3GbTransportOut = 	(bSdiOutRGB && !b2pi);	// UHD 29.97 YUV playback and RGB but not if TSI
	}
	else
	{
		if (bHfr)
			sdi5_3GbTransportOut = 	IsVideoFormatB(mFb1VideoFormat) || 
									(mSdiOutTransportType == NTV2_SDITransport_DualLink_3Gb);
		else
			sdi5_3GbTransportOut = 	b3GbOut || bSdiOutRGB;
	}
	mCard->SetSDIOutLevelAtoLevelBConversion(NTV2_CHANNEL5, (bFbLevelA && sdi5_3GbTransportOut) || (b4K && bSdiOutRGB));
	
	
	// RGB LevelA option
	mCard->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL1, !bFbLevelA && b3GaOutRGB);
	mCard->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL2, !bFbLevelA && b3GaOutRGB);
	mCard->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL3, !bFbLevelA && b3GaOutRGB);
	mCard->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL4, !bFbLevelA && b3GaOutRGB);
	mCard->SetSDIOutRGBLevelAConversion(NTV2_CHANNEL5, !bFbLevelA && b3GaOutRGB);
	
	
	// Set HBlack RGB range bits - ALWAYS SMPTE
	mCard->WriteRegister(kRegSDIOut1Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	mCard->WriteRegister(kRegSDIOut2Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	mCard->WriteRegister(kRegSDIOut3Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	mCard->WriteRegister(kRegSDIOut4Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	mCard->WriteRegister(kRegSDIOut5Control, NTV2_RGB10RangeSMPTE, kK2RegMaskSDIOutHBlankRGBRange, kK2RegShiftSDIOutHBlankRGBRange);
	
	
	// Set VBlank RGB range bits - ALWAYS SMPTE
	// Except when there is a full-range RGB frame buffer, and we go through the color space converter
	if (mRGB10Range == NTV2_RGB10RangeFull && mSDIOutput1ColorSpace != NTV2_ColorSpaceModeRgb)
	{
		mCard->WriteRegister(kRegCh1Control, NTV2_RGB10RangeFull, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
		mCard->WriteRegister(kRegCh2Control, NTV2_RGB10RangeFull, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
		if (b4K)
		{
			mCard->WriteRegister(kRegCh3Control, NTV2_RGB10RangeFull, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
			mCard->WriteRegister(kRegCh4Control, NTV2_RGB10RangeFull, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
		}
	}
	else
	{
		mCard->WriteRegister(kRegCh1Control, NTV2_RGB10RangeSMPTE, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
		mCard->WriteRegister(kRegCh2Control, NTV2_RGB10RangeSMPTE, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
		if (b4K)
		{
			mCard->WriteRegister(kRegCh3Control, NTV2_RGB10RangeSMPTE, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
			mCard->WriteRegister(kRegCh4Control, NTV2_RGB10RangeSMPTE, kRegMaskVBlankRGBRange, kRegShiftVBlankRGBRange);
		}
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

	//ULWord analogIOConfig = 0;
	//mCard->ReadRegister(kVRegAnalogAudioIOConfiguration, &analogIOConfig);
	//mCard->SetAnalogAudioIOConfiguration(NTV2_AnalogAudioIO_4In_4Out);
}

