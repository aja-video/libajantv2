//
//  ntv2corvidservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2corvidservices.h"


//-------------------------------------------------------------------------------------------------------
//	class CorvidServices
//-------------------------------------------------------------------------------------------------------

CorvidServices::CorvidServices()
{
}


//-------------------------------------------------------------------------------------------------------
//	GetSelectedInputVideoFormat
//	Note:	Determine input video format based on input select and fbVideoFormat
//			which currently is videoformat of ch1-framebuffer
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat CorvidServices::GetSelectedInputVideoFormat(
											NTV2VideoFormat fbVideoFormat,
											NTV2SDIInputFormatSelect* inputFormatSelect)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	if (inputFormatSelect)
		*inputFormatSelect = NTV2_YUVSelect;
	
	if (mVirtualInputSelect == NTV2_Input1Select)
	{
		inputFormat = GetSdiInVideoFormat(0, fbVideoFormat);
		if (inputFormatSelect)
			*inputFormatSelect = mSDIInput1FormatSelect;
	}
	inputFormat = GetTransportCompatibleFormat(inputFormat, fbVideoFormat);
	
	return inputFormat;
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void CorvidServices::SetDeviceXPointPlayback ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointPlayback();

	int bFb1Disable = 0;							// Assume Channel 1 is NOT disabled
	int bFb2Disable = 1;							// Assume Channel 2 IS disabled
	
	// Select input 1 (0x01)
	mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);

	// Select frame sync 1 output (0x08)
	mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);

	// set Channel disable mode (0 = enable, 1 = disable)
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable);		

}
	
	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void CorvidServices::SetDeviceXPointCapture ()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();

	// Select input 1 (0x01)
	mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);

	// Set SDI output 1 input (reg 138, bits 15-8)
	mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptSDIIn1);

}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void CorvidServices::SetDeviceMiscRegisters ()
{	
	// call superclass first
	DeviceServices::SetDeviceMiscRegisters();

	NTV2Standard			primaryStandard;
	mCard->GetStandard(primaryStandard);
}
