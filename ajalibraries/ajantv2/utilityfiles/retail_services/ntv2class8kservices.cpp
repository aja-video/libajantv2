//
//  ntv2class8kservices.cpp
//
//  Copyright (c) 2019 AJA Video, Inc. All rights reserved.
//

#include "ntv2class8kservices.h"


//--------------------------------------------------------------------------------------------------
//	class Class8kServices
//--------------------------------------------------------------------------------------------------

Class8kServices::Class8kServices(NTV2DeviceID devID) : 
	Class4kServices(devID)
{
}


Class8kServices::~Class8kServices()
{
}


//--------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceXPointPlayback ()
{
	bool 				b8K 				= NTV2_IS_QUAD_QUAD_FORMAT(mFb1VideoFormat);
	bool				b4K					= NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	bool				bFb1RGB				= IsRGBFormat(mFb1Format);
	bool				bQuadSwap			= b8K == true && mDs.bOut4xSdi == true && mQuadSwapOut != 0;	
	bool				bHdmiOutRGB			= mDs.hdmiOutColorSpace == kHDMIOutCSCRGB8bit || mDs.hdmiOutColorSpace == kHDMIOutCSCRGB10bit ||
											  mDs.hdmiOutColorSpace == kHDMIOutCSCRGB12bit;
	//bool				bSdiOutRGB			= false; // for now
	
	if (!b8K && !b4K)
		return Class4kServices::SetDeviceXPointPlayback();
	
	// skip Class4K for now
	DeviceServices::SetDeviceXPointPlayback();
	
	// 
	// 4K-8K support
	//
	
	NTV2CrosspointID XPt1, XPt2, XPt3, XPt4;
	
	// sync 8k quad buffers
	mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
	mCard->SetMode(NTV2_CHANNEL3, NTV2_MODE_DISPLAY);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL3, mFb1Format);
	mCard->SetMode(NTV2_CHANNEL4, NTV2_MODE_DISPLAY);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL4, mFb1Format);
		
	// Frame Buffer 1-4
	mCard->Connect(NTV2_XptFrameBuffer1Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
	mCard->Connect(NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
	if (b8K)
	{
		mCard->EnableChannel(NTV2_CHANNEL1);
		mCard->EnableChannel(NTV2_CHANNEL2);
		mCard->EnableChannel(NTV2_CHANNEL3);
		mCard->EnableChannel(NTV2_CHANNEL4);
	}
	else if (b4K)
	{
		mCard->EnableChannel(NTV2_CHANNEL1);
		mCard->DisableChannel(NTV2_CHANNEL2);
		mCard->DisableChannel(NTV2_CHANNEL3);
		mCard->DisableChannel(NTV2_CHANNEL4);
	}
	
	// SDI Out 1
	if (b8K)
	{
		if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut1Input, bQuadSwap ? NTV2_XptFrameBuffer3RGB : NTV2_XptFrameBuffer1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut1Input, bQuadSwap ? NTV2_XptFrameBuffer3YUV : NTV2_XptFrameBuffer1YUV);
		}
	}
	else if (b4K)
	{
		mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
	}
	mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);

	
	// SDI Out 2
	if (b8K)
	{
		if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut2Input, bQuadSwap ? NTV2_XptFrameBuffer4RGB : NTV2_XptFrameBuffer2RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut2Input, bQuadSwap ? NTV2_XptFrameBuffer4YUV : NTV2_XptFrameBuffer2YUV);
		}
	}
	else if (b4K)
	{
		mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
	}
	mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
	
	
	// SDI Out 3
	if (b8K)
	{
		if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, bQuadSwap ? NTV2_XptFrameBuffer1RGB : NTV2_XptFrameBuffer3RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut3Input, bQuadSwap ? NTV2_XptFrameBuffer1YUV : NTV2_XptFrameBuffer3YUV);
		}
	}
	else if (b4K)
	{
		if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer1YUV);
		}
	}
	mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);


	// SDI Out 4
	if (b8K)
	{
		if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, bQuadSwap ? NTV2_XptFrameBuffer2RGB : NTV2_XptFrameBuffer4RGB);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut4Input, bQuadSwap ? NTV2_XptFrameBuffer2YUV : NTV2_XptFrameBuffer4YUV);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
		}
	}
	else if (b4K)
	{
		if (bFb1RGB)
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer1RGB);
		}
		else
		{
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer1YUV);
		}
	}
	mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
	
	
	// HDMI Out
	if (mHasHdmiOut)
	{
		XPt1 = XPt2 = XPt3 = XPt4 = NTV2_XptBlack;
		if (b4K)
		{
			if (bHdmiOutRGB)
			{
				XPt1 = NTV2_XptFrameBuffer1RGB;
			}
			else
			{
				XPt1 = NTV2_XptFrameBuffer1YUV;
			}
		}
		else if (b8K)
		{
			switch (mVirtualHDMIOutputSelect)
			{
				default:
				case NTV2_Quadrant1Select: XPt1 = bFb1RGB ? NTV2_XptFrameBuffer1RGB : NTV2_XptFrameBuffer1YUV; break;
				case NTV2_Quadrant2Select: XPt1 = bFb1RGB ? NTV2_XptFrameBuffer2RGB : NTV2_XptFrameBuffer2YUV; break;
				case NTV2_Quadrant3Select: XPt1 = bFb1RGB ? NTV2_XptFrameBuffer3RGB : NTV2_XptFrameBuffer3YUV; break;
				case NTV2_Quadrant4Select: XPt1 = bFb1RGB ? NTV2_XptFrameBuffer4RGB : NTV2_XptFrameBuffer4YUV; break;
			}
		}
		mCard->Connect (NTV2_XptHDMIOutQ1Input, XPt1);
		mCard->Connect (NTV2_XptHDMIOutQ2Input, XPt2);
		mCard->Connect (NTV2_XptHDMIOutQ3Input, XPt3);
		mCard->Connect (NTV2_XptHDMIOutQ4Input, XPt4);
	}


	// CSC 1
	mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptBlack);


	// CSC 2
	mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptBlack);
	
	
	// Mixer/Keyer
	mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptBlack);
}

	
//--------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceXPointCapture ()
{
	bool 				b8K 			= NTV2_IS_QUAD_QUAD_FORMAT(mFb1VideoFormat);
	bool				b4K				= NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	bool				bQuadSwap		= b8K == true && mDs.bIn4xSdi == true && mQuadSwapIn != 0;
	//bool				bHdmiIn			= mDs.bInHdmi && mHasHdmiIn;
	//bool				bHdmiInRGB		= bHdmiIn == true && mDs.hdmiIn[0]->cs == NTV2_ColorSpaceModeRgb;
	//bool				bInRGB			= bHdmiInRGB || mDs.bInSdiRgb;
	//bool				bFb1RGB			= IsRGBFormat(mFb1Format);
	
	if (!b8K && !b4K)
		return Class4kServices::SetDeviceXPointCapture();
		
	// skip Class4K for now
	DeviceServices::SetDeviceXPointCapture();
		
	// 
	// 4K-8K support
	//
	
	NTV2CrosspointID XPt1, XPt2, XPt3, XPt4;
		
	NTV2CrosspointID in4k = NTV2_XptSDIIn1;
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		in4k = NTV2_XptSDIIn1;
	}
	else if (mVirtualInputSelect == NTV2_Input2Select)
	{
		in4k = NTV2_XptSDIIn2;
	}
	
	// sync 8k quad buffers
	mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
	mCard->SetMode(NTV2_CHANNEL3, NTV2_MODE_CAPTURE);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL3, mFb1Format);
	mCard->SetMode(NTV2_CHANNEL4, NTV2_MODE_CAPTURE);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL4, mFb1Format);
		
	// Frame Buffer 1
	if (b8K)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, bQuadSwap ? NTV2_XptSDIIn3 : NTV2_XptSDIIn1);
	}
	else if (b4K)
	{
		mCard->Connect (NTV2_XptFrameBuffer1Input, in4k);
	}
	
	// Frame Buffer 2
	if (b8K)
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, bQuadSwap ? NTV2_XptSDIIn4 : NTV2_XptSDIIn2);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	}
	
	// Frame Buffer 3
	if (b8K)
	{
		mCard->Connect (NTV2_XptFrameBuffer3Input, bQuadSwap ? NTV2_XptSDIIn1 : NTV2_XptSDIIn3);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
	}

	// Frame Buffer 4
	if (b8K)
	{
		mCard->Connect (NTV2_XptFrameBuffer4Input, bQuadSwap ? NTV2_XptSDIIn2 : NTV2_XptSDIIn4);
	}
	else
	{
		mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
	}
	
	if (b8K)
	{
		mCard->EnableChannel(NTV2_CHANNEL1);
		mCard->EnableChannel(NTV2_CHANNEL2);
		mCard->EnableChannel(NTV2_CHANNEL3);
		mCard->EnableChannel(NTV2_CHANNEL4);
	}
	else if (b4K)
	{
		mCard->EnableChannel(NTV2_CHANNEL1);
		mCard->DisableChannel(NTV2_CHANNEL2);
		mCard->DisableChannel(NTV2_CHANNEL3);
		mCard->DisableChannel(NTV2_CHANNEL4);
	}
	
	
	// SDI Out
	mCard->Connect (NTV2_XptSDIOut1Input, 		NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut1InputDS2, 	NTV2_XptBlack);
	
	
	// SDI Out 2
	mCard->Connect (NTV2_XptSDIOut2Input, 		NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut2InputDS2, 	NTV2_XptBlack);
	
	
	// SDI Out 3
	if (b8K)
	{
		mCard->Connect (NTV2_XptSDIOut3Input, 	NTV2_XptBlack);
	}
	else if (b4K)
	{
		mCard->Connect (NTV2_XptSDIOut3Input, 	in4k);
	}
	mCard->Connect (NTV2_XptSDIOut3InputDS2, 	NTV2_XptBlack);


	// SDI Out 4
	if (b8K)
	{
		mCard->Connect (NTV2_XptSDIOut4Input, 	NTV2_XptBlack);
	}
	else if (b4K)
	{
		mCard->Connect (NTV2_XptSDIOut4Input, 	in4k);
	}
	mCard->Connect (NTV2_XptSDIOut4InputDS2, 	NTV2_XptBlack);
	
	
	// HDMI Out
	if (mHasHdmiOut)
	{
		XPt1 = XPt2 = XPt3 = XPt4 = NTV2_XptBlack;
		if (b4K)
		{
			XPt1 = in4k;
		}
		else if (b8K)
		{
			switch (mVirtualHDMIOutputSelect)
			{
				default:
				case NTV2_Quadrant1Select: XPt1 = bQuadSwap ? NTV2_XptSDIIn3 : NTV2_XptSDIIn1; break;
				case NTV2_Quadrant2Select: XPt1 = bQuadSwap ? NTV2_XptSDIIn4 : NTV2_XptSDIIn2; break;
				case NTV2_Quadrant3Select: XPt1 = bQuadSwap ? NTV2_XptSDIIn1 : NTV2_XptSDIIn3; break;
				case NTV2_Quadrant4Select: XPt1 = bQuadSwap ? NTV2_XptSDIIn2 : NTV2_XptSDIIn4; break;
			}
		}
		mCard->Connect (NTV2_XptHDMIOutQ1Input, XPt1);
		mCard->Connect (NTV2_XptHDMIOutQ2Input, XPt2);
		mCard->Connect (NTV2_XptHDMIOutQ3Input, XPt3);
		mCard->Connect (NTV2_XptHDMIOutQ4Input, XPt4);
	}

	
	// CSC 1
	mCard->Connect (NTV2_XptCSC1VidInput, 		NTV2_XptBlack);


	// CSC 2
	mCard->Connect (NTV2_XptCSC2VidInput, 		NTV2_XptBlack);
	
	
	// Mixer/Keyer
	mCard->Connect (NTV2_XptMixer1FGVidInput, 	NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1FGKeyInput, 	NTV2_XptBlack);
}


//--------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceMiscRegisters ()
{
	bool 				b8K 				= NTV2_IS_QUAD_QUAD_FORMAT(mFb1VideoFormat);
	bool				b4K					= NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	//bool				b8kHfr				= NTV2_IS_QUAD_QUAD_HFR_VIDEO_FORMAT(mFb1VideoFormat);
	//bool				bSdiOutRGB			= mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb;
	//bool				b3GaOutRGB			= (mSdiOutTransportType == NTV2_SDITransport_3Ga) && bSdiOutRGB;
	//bool				b4k6gOut			= b8K && !b8kHfr && !bSdiOutRGB; //&& (m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire);
	//bool				b4k12gOut			= b8K && (b8kHfr || bSdiOutRGB); //&& (m4kTransportOutSelection == NTV2_4kTransport_12g_6g_1wire);
	bool				bFbLevelA			= true; //IsVideoFormatA(mFb1VideoFormat);
	NTV2FrameRate		primaryFrameRate	= GetNTV2FrameRateFromVideoFormat(mFb1VideoFormat);

	if (!b8K && !b4K)
		return Class4kServices::SetDeviceMiscRegisters();
	
	// skip a class4k - for now
	DeviceServices::SetDeviceMiscRegisters();
	
	//
	// SDI Out
	//
	
	// SDI In - levelB to levelA conversion
	for (int i=0; i<4; i++)
	{
		if (mDs.sdiIn[i]->isOut == false)
		{
			bool bConvertBToA = bFbLevelA && mDs.sdiIn[i]->is3Gb;
			mCard->SetSDIInLevelBtoLevelAConversion(i, bConvertBToA);
		}
	}
	
	// SDI Out - Transmit
	bool bTransmit;
	for (int i=0; i<4; i++)
	{
		bTransmit = mDs.sdiOut[i]->isOut;
		mCard->SetSDITransmitEnable((NTV2Channel)i, bTransmit);
	}
	
	// SDI Out - 6G/12G - SetSDIOut12GEnable - handled in driver
	
	//
	// HDMI Out
	// 
	
	// local hacks for now
	b4K = true;
	bool b2pi = true;
	bool bHdmiIn = false;
	bool b4kHfr	= NTV2_IS_QUAD_HFR_VIDEO_FORMAT(mFb1VideoFormat);
					
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
		
		// HFR
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
			case kHDMIOutCSCYCbCr10bit:
				mCard->SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
				mCard->SetHDMIOutBitDepth (NTV2_HDMI10Bit);
				break;
		
			case kHDMIOutCSCYCbCr8bit:
				mCard->SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceYCbCr);
				mCard->SetHDMIOutBitDepth(NTV2_HDMI8Bit);
				break;
			case kHDMIOutCSCRGB12bit:
				mCard->SetLHIHDMIOutColorSpace (NTV2_LHIHDMIColorSpaceRGB);
				mCard->SetHDMIOutBitDepth (NTV2_HDMI12Bit);
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
		switch (mDs.hdmiOutProtocol)
		{
			default:
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

}

