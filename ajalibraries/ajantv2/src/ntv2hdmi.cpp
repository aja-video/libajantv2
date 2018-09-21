/**
	@file		ntv2hdmi.cpp
	@brief		Implements most of CNTV2Card's HDMI-related functions.
	@copyright	(C) 2004-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#if defined (AJALinux)
	//#include "ntv2linuxpublicinterface.h"
#elif defined (MSWindows)
	#pragma warning(disable: 4800)
#endif

using namespace std;


static const ULWord gHDMIChannelToInputStatusRegNum [] =    { kRegHDMIInputStatus1, kRegHDMIInputStatus2, kRegHDMIInputStatus3, kRegHDMIInputStatus4, 0 };
static const ULWord gHDMIChannelToControlRegNum [] =    { kRegHDMIControl1, kRegHDMIControl2, kRegHDMIControl3, kRegHDMIControl4, 0 };


//////////////////////////////////////////////////////////////////
// HDMI

// kRegHDMIInputStatus
bool CNTV2Card::GetHDMIInputStatusRegNum (ULWord & outRegNum, const NTV2Channel inChannel)
{
    const ULWord	numInputs	(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
	outRegNum = 0;
    if (numInputs < 1)
		return false;
	if (inChannel >= NTV2Channel(numInputs))
		return false;
    if (numInputs == 1)
	{
        outRegNum = kRegHDMIInputStatus;
		return true;
	}
	outRegNum = gHDMIChannelToInputStatusRegNum[inChannel];
	return true;
}
bool CNTV2Card::GetHDMIInputStatus (ULWord & outValue, const NTV2Channel inChannel)
{
    ULWord regNum (0);
    if (!GetHDMIInputStatusRegNum(regNum, inChannel))
		return false;
	return ReadRegister (regNum, outValue);
}

bool CNTV2Card::GetHDMIInputColor (NTV2LHIHDMIColorSpace & outValue, const NTV2Channel inChannel)
{
    const ULWord numInputs(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    if (numInputs < 1)
		return false;
    if (numInputs == 1)
        return CNTV2DriverInterface::ReadRegister (kRegHDMIInputStatus, outValue, kLHIRegMaskHDMIInputColorSpace, kLHIRegShiftHDMIInputColorSpace);
	if (inChannel <= NTV2Channel(numInputs))
		return CNTV2DriverInterface::ReadRegister (gHDMIChannelToInputStatusRegNum[inChannel], outValue, kLHIRegMaskHDMIInputColorSpace, kLHIRegShiftHDMIInputColorSpace);
	return false;
}

bool CNTV2Card::GetHDMIInVideoRange (NTV2HDMIRange & outValue, const NTV2Channel inChannel)
{
    const ULWord numInputs(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    if (numInputs < 1)
		return false;
    if (numInputs == 1)
        return CNTV2DriverInterface::ReadRegister (kRegHDMIInputControl, outValue, kRegMaskHDMIInfoRange, kRegShiftHDMIInfoRange);
	if (inChannel <= NTV2Channel(numInputs))
		return CNTV2DriverInterface::ReadRegister (gHDMIChannelToControlRegNum[inChannel], outValue, kRegMaskHDMIInfoRange, kRegShiftHDMIInfoRange);
	return false;
}

// kRegHDMIInputControl
bool CNTV2Card::SetHDMIInputRange (const NTV2HDMIRange inValue, const NTV2Channel inChannel)
{
    const ULWord numInputs(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    if (numInputs < 1)
		return false;
	return inChannel == NTV2_CHANNEL1	//	FIX THIS!	MrBill
			&& WriteRegister (kRegHDMIInputControl,	ULWord(inValue), kRegMaskHDMIInputRange, kRegShiftHDMIInputRange);
}

bool CNTV2Card::GetHDMIInputRange (NTV2HDMIRange & outValue, const NTV2Channel inChannel)
{
    const ULWord numInputs(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    if (numInputs < 1)
		return false;
	return inChannel == NTV2_CHANNEL1	//	FIX THIS!	MrBill
			&& CNTV2DriverInterface::ReadRegister(kRegHDMIInputControl, outValue, kRegMaskHDMIInputRange, kRegShiftHDMIInputRange);
}

bool CNTV2Card::SetHDMIInColorSpace (const NTV2HDMIColorSpace inValue, const NTV2Channel inChannel)
{
    const ULWord numInputs(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    if (numInputs < 1)
		return false;
	return inChannel == NTV2_CHANNEL1	//	FIX THIS!	MrBill
			&& WriteRegister (kRegHDMIInputControl,	ULWord(inValue), kRegMaskHDMIColorSpace, kRegShiftHDMIColorSpace);
}

bool CNTV2Card::GetHDMIInColorSpace (NTV2HDMIColorSpace & outValue, const NTV2Channel inChannel)
{
    const ULWord numInputs(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    if (numInputs < 1)
		return false;
	return inChannel == NTV2_CHANNEL1	//	FIX THIS!	MrBill
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIInputControl, outValue, kRegMaskHDMIColorSpace, kRegShiftHDMIColorSpace);
}

bool CNTV2Card::SetHDMIInBitDepth (const NTV2HDMIBitDepth inNewValue, const NTV2Channel inChannel)
{
    const UWord numInputs(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    if (numInputs < 1)
		return false;
	if (numInputs <= UWord(inChannel))
		return false;
	if (NTV2_IS_VALID_HDMI_BITDEPTH(inNewValue))
		return false;
	//	FINISH THIS		MrBill
	return true;
}

bool CNTV2Card::GetHDMIInBitDepth (NTV2HDMIBitDepth & outValue, const NTV2Channel inChannel)
{
	outValue = NTV2_INVALID_HDMIBitDepth;
	ULWord status(0);
	if (!GetHDMIInputStatus(status, inChannel))
		return false;
	outValue = NTV2HDMIBitDepth((status & kLHIRegMaskHDMIInputBitDepth) >> kLHIRegShiftHDMIInputBitDepth);
	return NTV2_IS_VALID_HDMI_BITDEPTH(outValue);
}

bool CNTV2Card::GetHDMIInProtocol (NTV2HDMIProtocol & outValue, const NTV2Channel inChannel)
{
	outValue = NTV2_INVALID_HDMI_PROTOCOL;
	ULWord status(0);
	if (!GetHDMIInputStatus(status, inChannel))
		return false;
	outValue = NTV2HDMIProtocol((status & kLHIRegMaskHDMIInputProtocol) >> kLHIRegShiftHDMIInputProtocol);
	return NTV2_IS_VALID_HDMI_PROTOCOL(outValue);
}

bool CNTV2Card::GetHDMIInIsLocked (bool & outIsLocked, const NTV2Channel inChannel)
{
	outIsLocked = false;
	ULWord status(0);
	if (!GetHDMIInputStatus(status, inChannel))
		return false;
	if (GetDeviceID() == DEVICE_ID_KONALHIDVI)
		outIsLocked = (status & (BIT(0) | BIT(1))) == (BIT(0) | BIT(1));
	else
		outIsLocked = (status & kRegMaskInputStatusLock) ? true : false;
	return true;
}

bool CNTV2Card::SetHDMIInAudioSampleRateConverterEnable (const bool value, const NTV2Channel inChannel)
{
	const ULWord	tempVal	(!value);	// this is high to disable sample rate conversion
	return inChannel == NTV2_CHANNEL1	//	FIX THIS	MrBill
			&& WriteRegister (kRegHDMIInputControl, tempVal, kRegMaskHDMISampleRateConverterEnable, kRegShiftHDMISampleRateConverterEnable);
}


bool CNTV2Card::GetHDMIInAudioSampleRateConverterEnable (bool & outEnabled, const NTV2Channel inChannel)
{
	if (inChannel != NTV2_CHANNEL1)
		return false;	//	FIX THIS	MrBill
	ULWord	tempVal	(0);
	bool	retVal	(ReadRegister (kRegHDMIInputControl, tempVal, kRegMaskHDMISampleRateConverterEnable, kRegShiftHDMISampleRateConverterEnable));
	if (retVal)
		outEnabled = !(static_cast <bool> (tempVal));		// this is high to disable sample rate conversion
	return retVal;
}


bool CNTV2Card::GetHDMIInputAudioChannels (NTV2HDMIAudioChannels & outValue, const NTV2Channel inChannel)
{
	if (inChannel != NTV2_CHANNEL1)
		return false;	//	FIX THIS	MrBill
	ULWord	tempVal	(0);
	outValue = NTV2_INVALID_HDMI_AUDIO_CHANNELS;
	if (!ReadRegister(kRegHDMIInputStatus, tempVal))
		return false;
	outValue = (tempVal & kLHIRegMaskHDMIInput2ChAudio) ? NTV2_HDMIAudio2Channels : NTV2_HDMIAudio8Channels;
	return true;
}

// kRegHDMIOut3DControl
bool CNTV2Card::SetHDMIOut3DPresent (const bool value)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& WriteRegister (kRegHDMIOut3DControl,	ULWord(value), kRegMaskHDMIOut3DPresent, kRegShiftHDMIOut3DPresent);
}

bool CNTV2Card::GetHDMIOut3DPresent (bool & outValue)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIOut3DControl, outValue, kRegMaskHDMIOut3DPresent, kRegShiftHDMIOut3DPresent);
}

bool CNTV2Card::SetHDMIOut3DMode (const NTV2HDMIOut3DMode inValue)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& WriteRegister (kRegHDMIOut3DControl,	ULWord(inValue), kRegMaskHDMIOut3DMode, kRegShiftHDMIOut3DMode);
}

bool CNTV2Card::GetHDMIOut3DMode (NTV2HDMIOut3DMode & outValue)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIOut3DControl, outValue, kRegMaskHDMIOut3DMode, kRegShiftHDMIOut3DMode);
}

// kRegHDMIOutControl
bool CNTV2Card::SetHDMIOutVideoStandard (const NTV2Standard inValue)
{
	const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
	return hdmiVers > 0
			&& ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& WriteRegister (kRegHDMIOutControl, ULWord(inValue),	hdmiVers == 1  ?  kRegMaskHDMIOutVideoStd  :  kRegMaskHDMIOutV2VideoStd,	kRegShiftHDMIOutVideoStd);
}

bool CNTV2Card::GetHDMIOutVideoStandard (NTV2Standard & outValue)
{
	const ULWord	hdmiVers(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
	if (hdmiVers)
		return CNTV2DriverInterface::ReadRegister (kRegHDMIOutControl, outValue,  hdmiVers == 1  ?  kRegMaskHDMIOutVideoStd  :  kRegMaskHDMIOutV2VideoStd,	kRegShiftHDMIOutVideoStd);
	outValue = NTV2_STANDARD_INVALID;
	return false;
}

bool CNTV2Card::SetHDMIV2TxBypass (const bool bypass)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& WriteRegister (kRegHDMIOutControl,		bypass,				kRegMaskHDMIV2TxBypass,					kRegShiftHDMIV2TxBypass);
}

// HDMI V2 includes an extra bit for quad formats
#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::SetHDMIV2OutVideoStandard (NTV2V2Standard value)				
	{
		return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
				&& WriteRegister (kRegHDMIOutControl,		(ULWord)value,		kRegMaskHDMIOutV2VideoStd,				kRegShiftHDMIOutVideoStd);
	}

	bool CNTV2Card::GetHDMIV2OutVideoStandard		(NTV2V2Standard * pOutValue)		
	{
		return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
				&& ReadRegister  (kRegHDMIOutControl,		(ULWord*)pOutValue,	kRegMaskHDMIOutV2VideoStd,				kRegShiftHDMIOutVideoStd);
	}
#endif	//	!defined (NTV2_DEPRECATE)

bool CNTV2Card::SetHDMIOutSampleStructure (const NTV2HDMISampleStructure inValue)		
{
	if (!NTV2_IS_VALID_HDMI_SAMPLE_STRUCT(inValue))
		return false;
	if (::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) == 0)
		return false;
	return WriteRegister (kRegHDMIOutControl, ULWord(inValue), kRegMaskHDMISampling, kRegShiftHDMISampling);
}

bool CNTV2Card::GetHDMIOutSampleStructure (NTV2HDMISampleStructure & outValue)
{
	if (::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) == 0)
		return false;
	return CNTV2DriverInterface::ReadRegister (kRegHDMIOutControl, outValue, kRegMaskHDMISampling, kRegShiftHDMISampling);
}

bool CNTV2Card::SetHDMIOutVideoFPS (const NTV2FrameRate value)				
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& NTV2_IS_VALID_NTV2FrameRate(value)
			&& WriteRegister (kRegHDMIOutControl, ULWord(value), kLHIRegMaskHDMIOutFPS, kLHIRegShiftHDMIOutFPS);
}

bool CNTV2Card::GetHDMIOutVideoFPS (NTV2FrameRate & outValue)			
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIOutControl, outValue, kLHIRegMaskHDMIOutFPS, kLHIRegShiftHDMIOutFPS);
}

bool CNTV2Card::SetHDMIOutRange (const NTV2HDMIRange value)				
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& NTV2_IS_VALID_HDMI_RANGE(value)
			&& WriteRegister (kRegHDMIOutControl, ULWord(value), kRegMaskHDMIOutRange, kRegShiftHDMIOutRange);
}

bool CNTV2Card::GetHDMIOutRange (NTV2HDMIRange & outValue)			
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIOutControl, outValue, kRegMaskHDMIOutRange, kRegShiftHDMIOutRange);
}



bool CNTV2Card::SetHDMIOutColorSpace (const NTV2HDMIColorSpace inValue)			
{	//	Register 125		This function used to touch bits 4 & 5 (from the old FS/1 days) --- now obsolete.
	//return WriteRegister (kRegHDMIOutControl,  ULWord(value),  kRegMaskHDMIColorSpace,  kRegShiftHDMIColorSpace);
	//	Fixed in SDK 14.1 to work with modern NTV2 devices, but using the old NTV2HDMIColorSpace enum:
	ULWord	newCorrectValue	(0);
	switch (inValue)
	{
		case NTV2_HDMIColorSpaceRGB:	newCorrectValue = NTV2_LHIHDMIColorSpaceRGB;	break;
		case NTV2_HDMIColorSpaceYCbCr:	newCorrectValue = NTV2_LHIHDMIColorSpaceYCbCr;	break;
		default:	return false;	//	Illegal value
	}
	//						Register 125							Bit 8
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& WriteRegister (kRegHDMIOutControl,  newCorrectValue,  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace);
}

bool CNTV2Card::GetHDMIOutColorSpace (NTV2HDMIColorSpace & outValue)		
{	//	Register 125		This function used to read bits 4 & 5 (from the old FS/1 days) --- now obsolete.
	//return ReadRegister (kRegHDMIOutControl,  outValue,  kRegMaskHDMIColorSpace,  kRegShiftHDMIColorSpace);
	//	Fixed in SDK 14.1 to work with modern NTV2 devices, but using the old NTV2HDMIColorSpace enum:
	if (!::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()))
		return false;	//	No HDMI outputs

	ULWord	correctValue(0);
	//					Register 125						Bit 8
	if (!ReadRegister (kRegHDMIOutControl,  correctValue,  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace))
		return false;	//	Fail
	switch(correctValue)
	{
		case NTV2_LHIHDMIColorSpaceYCbCr:	outValue = NTV2_HDMIColorSpaceYCbCr;	break;
		case NTV2_LHIHDMIColorSpaceRGB:		outValue = NTV2_HDMIColorSpaceRGB;		break;
		default:							return false;	//	Bad value?!
	}
	return true;
}


bool CNTV2Card::SetLHIHDMIOutColorSpace (NTV2LHIHDMIColorSpace value)
{	//	Register 125											Bit 8
	return WriteRegister (kRegHDMIOutControl,  ULWord(value),  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace);
}

bool CNTV2Card::GetLHIHDMIOutColorSpace (NTV2LHIHDMIColorSpace & outValue)	
{	//	Register 125												Bit 8
	ULWord	value(0);
	bool result (ReadRegister (kRegHDMIOutControl,  value,  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace));
	outValue = NTV2LHIHDMIColorSpace(value);
	return result;
}



bool CNTV2Card::GetHDMIOutDownstreamBitDepth (NTV2HDMIBitDepth & outValue)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIInputStatus, outValue, kLHIRegMaskHDMIOutputEDID10Bit, kLHIRegShiftHDMIOutputEDID10Bit);
}

bool CNTV2Card::GetHDMIOutDownstreamColorSpace (NTV2LHIHDMIColorSpace & outValue)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIInputStatus, outValue, kLHIRegMaskHDMIOutputEDIDRGB, kLHIRegShiftHDMIOutputEDIDRGB);
}

bool CNTV2Card::SetHDMIOutBitDepth (const NTV2HDMIBitDepth value)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& NTV2_IS_VALID_HDMI_BITDEPTH(value)
			&& WriteRegister (kRegHDMIOutControl, ULWord(value), kLHIRegMaskHDMIOutBitDepth, kLHIRegShiftHDMIOutBitDepth);
}

bool CNTV2Card::GetHDMIOutBitDepth (NTV2HDMIBitDepth & outValue)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIOutControl, outValue, kLHIRegMaskHDMIOutBitDepth, kLHIRegShiftHDMIOutBitDepth);
}

bool CNTV2Card::SetHDMIOutProtocol (const NTV2HDMIProtocol value)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& NTV2_IS_VALID_HDMI_PROTOCOL(value)
			&& WriteRegister (kRegHDMIOutControl, ULWord(value), kRegMaskHDMIProtocol, kRegShiftHDMIProtocol);
}

bool CNTV2Card::GetHDMIOutProtocol (NTV2HDMIProtocol & outValue)
{
	return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegHDMIOutControl, outValue, kRegMaskHDMIProtocol, kRegShiftHDMIProtocol);
}



bool CNTV2Card::SetHDMIOutDecimateMode (const bool inIsEnabled)
{
	return ::NTV2DeviceGetHDMIVersion(_boardID) > 1
			&& ::NTV2DeviceGetNumHDMIVideoOutputs(_boardID) > 0
			&& WriteRegister(kRegRasterizerControl, ULWord(inIsEnabled), kRegMaskRasterDecimate, kRegShiftRasterDecimate);
}

bool CNTV2Card::GetHDMIOutDecimateMode(bool & outIsEnabled)
{
	return ::NTV2DeviceGetHDMIVersion(_boardID) > 1
			&& ::NTV2DeviceGetNumHDMIVideoOutputs(_boardID) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegRasterizerControl, outIsEnabled, kRegMaskRasterDecimate, kRegShiftRasterDecimate);
}

bool CNTV2Card::SetHDMIOutTsiIO (const bool inIsEnabled)
{
	return ::NTV2DeviceGetHDMIVersion(_boardID) > 1
			&& ::NTV2DeviceGetNumHDMIVideoOutputs(_boardID) > 0
			&& WriteRegister(kRegRasterizerControl, ULWord(inIsEnabled), kRegMaskTsiIO, kRegShiftTsiIO);
}

bool CNTV2Card::GetHDMIOutTsiIO (bool & outIsEnabled)
{
	return ::NTV2DeviceGetHDMIVersion(_boardID) > 1
			&& ::NTV2DeviceGetNumHDMIVideoOutputs(_boardID) > 0
			&& CNTV2DriverInterface::ReadRegister(kRegRasterizerControl, outIsEnabled, kRegMaskTsiIO, kRegShiftTsiIO);
}

bool CNTV2Card::SetHDMIOutLevelBMode (const bool inIsEnabled)
{
	return ::NTV2DeviceGetHDMIVersion(_boardID) > 1
			&& ::NTV2DeviceGetNumHDMIVideoOutputs(_boardID) > 0
			&& WriteRegister(kRegRasterizerControl, ULWord(inIsEnabled), kRegMaskRasterLevelB, kRegShiftRasterLevelB);
}

bool CNTV2Card::GetHDMIOutLevelBMode (bool & outIsEnabled)
{
	return ::NTV2DeviceGetHDMIVersion(_boardID) > 1
			&& ::NTV2DeviceGetNumHDMIVideoOutputs(_boardID) > 0
			&& CNTV2DriverInterface::ReadRegister (kRegRasterizerControl, outIsEnabled, kRegMaskRasterLevelB, kRegShiftRasterLevelB);
}

bool CNTV2Card::SetHDMIV2Mode (const NTV2HDMIV2Mode inMode)
{
	return ::NTV2DeviceGetHDMIVersion(_boardID) > 1
			&& WriteRegister(kRegRasterizerControl, inMode, kRegMaskRasterMode, kRegShiftRasterMode);
}

bool CNTV2Card::GetHDMIV2Mode (NTV2HDMIV2Mode & outMode)
{
	return ::NTV2DeviceGetHDMIVersion(_boardID) > 1
			&& CNTV2DriverInterface::ReadRegister(kRegRasterizerControl, outMode, kRegMaskRasterMode, kRegShiftRasterMode);
}

bool CNTV2Card::SetHDMIHDRGreenPrimaryX(const uint16_t inGreenPrimaryX)
{
	return NTV2_IS_VALID_HDR_PRIMARY(inGreenPrimaryX)
		&&  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRGreenPrimary, uint32_t(inGreenPrimaryX), kRegMaskHDMIHDRGreenPrimaryX, kRegShiftHDMIHDRGreenPrimaryX);
}

bool CNTV2Card::GetHDMIHDRGreenPrimaryX(uint16_t & outGreenPrimaryX)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRGreenPrimary, outGreenPrimaryX, kRegMaskHDMIHDRGreenPrimaryX, kRegShiftHDMIHDRGreenPrimaryX);
}

bool CNTV2Card::SetHDMIHDRGreenPrimaryY(const uint16_t inGreenPrimaryY)
{
	return NTV2_IS_VALID_HDR_PRIMARY(inGreenPrimaryY)
		&&  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRGreenPrimary, uint32_t(inGreenPrimaryY), kRegMaskHDMIHDRGreenPrimaryY, kRegShiftHDMIHDRGreenPrimaryY);
}

bool CNTV2Card::GetHDMIHDRGreenPrimaryY(uint16_t & outGreenPrimaryY)
{
	return  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRGreenPrimary, outGreenPrimaryY, kRegMaskHDMIHDRGreenPrimaryY, kRegShiftHDMIHDRGreenPrimaryY);
}

bool CNTV2Card::SetHDMIHDRBluePrimaryX(const uint16_t inBluePrimaryX)
{
	return NTV2_IS_VALID_HDR_PRIMARY(inBluePrimaryX)
		&&  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRBluePrimary, uint32_t(inBluePrimaryX), kRegMaskHDMIHDRBluePrimaryX, kRegShiftHDMIHDRBluePrimaryX);
}

bool CNTV2Card::GetHDMIHDRBluePrimaryX(uint16_t & outBluePrimaryX)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRBluePrimary, outBluePrimaryX, kRegMaskHDMIHDRBluePrimaryX, kRegShiftHDMIHDRBluePrimaryX);
}

bool CNTV2Card::SetHDMIHDRBluePrimaryY(const uint16_t inBluePrimaryY)
{
	return NTV2_IS_VALID_HDR_PRIMARY(inBluePrimaryY)
		&&  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRBluePrimary, uint32_t(inBluePrimaryY), kRegMaskHDMIHDRBluePrimaryY, kRegShiftHDMIHDRBluePrimaryY);
}

bool CNTV2Card::GetHDMIHDRBluePrimaryY(uint16_t & outBluePrimaryY)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRBluePrimary, outBluePrimaryY, kRegMaskHDMIHDRBluePrimaryY, kRegShiftHDMIHDRBluePrimaryY);
}

bool CNTV2Card::SetHDMIHDRRedPrimaryX(const uint16_t inRedPrimaryX)
{
	return NTV2_IS_VALID_HDR_PRIMARY(inRedPrimaryX)
		&&  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&& WriteRegister(kRegHDMIHDRRedPrimary, uint32_t(inRedPrimaryX), kRegMaskHDMIHDRRedPrimaryX, kRegShiftHDMIHDRRedPrimaryX);
}

bool CNTV2Card::GetHDMIHDRRedPrimaryX(uint16_t & outRedPrimaryX)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRRedPrimary, outRedPrimaryX, kRegMaskHDMIHDRRedPrimaryX, kRegShiftHDMIHDRRedPrimaryX);
}

bool CNTV2Card::SetHDMIHDRRedPrimaryY(const uint16_t inRedPrimaryY)
{
	return NTV2_IS_VALID_HDR_PRIMARY(inRedPrimaryY)
		&&  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRRedPrimary, uint32_t(inRedPrimaryY), kRegMaskHDMIHDRRedPrimaryY, kRegShiftHDMIHDRRedPrimaryY);
}

bool CNTV2Card::GetHDMIHDRRedPrimaryY(uint16_t & outRedPrimaryY)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRRedPrimary, outRedPrimaryY, kRegMaskHDMIHDRRedPrimaryY, kRegShiftHDMIHDRRedPrimaryY);
}

bool CNTV2Card::SetHDMIHDRWhitePointX(const uint16_t inWhitePointX)
{
	return NTV2_IS_VALID_HDR_PRIMARY(inWhitePointX)
		&&  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRWhitePoint, uint32_t(inWhitePointX), kRegMaskHDMIHDRWhitePointX, kRegShiftHDMIHDRWhitePointX);
}

bool CNTV2Card::GetHDMIHDRWhitePointX(uint16_t & outWhitePointX)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRWhitePoint, outWhitePointX, kRegMaskHDMIHDRWhitePointX, kRegShiftHDMIHDRWhitePointX);
}

bool CNTV2Card::SetHDMIHDRWhitePointY(const uint16_t inWhitePointY)
{
	return NTV2_IS_VALID_HDR_PRIMARY(inWhitePointY)
		&&  ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRWhitePoint, uint32_t(inWhitePointY), kRegMaskHDMIHDRWhitePointY, kRegShiftHDMIHDRWhitePointY);
}

bool CNTV2Card::GetHDMIHDRWhitePointY(uint16_t & outWhitePointY)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRWhitePoint, outWhitePointY, kRegMaskHDMIHDRWhitePointY, kRegShiftHDMIHDRWhitePointY);
}

bool CNTV2Card::SetHDMIHDRMaxMasteringLuminance(const uint16_t inMaxMasteringLuminance)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  NTV2_IS_VALID_HDR_MASTERING_LUMINENCE(inMaxMasteringLuminance)
		&&  WriteRegister(kRegHDMIHDRMasteringLuminence, uint32_t(inMaxMasteringLuminance), kRegMaskHDMIHDRMaxMasteringLuminance, kRegShiftHDMIHDRMaxMasteringLuminance);
}

bool CNTV2Card::GetHDMIHDRMaxMasteringLuminance(uint16_t & outMaxMasteringLuminance)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRMasteringLuminence, outMaxMasteringLuminance, kRegMaskHDMIHDRMaxMasteringLuminance, kRegShiftHDMIHDRMaxMasteringLuminance);
}

bool CNTV2Card::SetHDMIHDRMinMasteringLuminance(const uint16_t inMinMasteringLuminance)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  NTV2_IS_VALID_HDR_MASTERING_LUMINENCE(inMinMasteringLuminance)
		&&  WriteRegister(kRegHDMIHDRMasteringLuminence, uint32_t(inMinMasteringLuminance), kRegMaskHDMIHDRMinMasteringLuminance, kRegShiftHDMIHDRMinMasteringLuminance);
}

bool CNTV2Card::GetHDMIHDRMinMasteringLuminance(uint16_t & outMinMasteringLuminance)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRMasteringLuminence, outMinMasteringLuminance, kRegMaskHDMIHDRMinMasteringLuminance, kRegShiftHDMIHDRMinMasteringLuminance);
}

bool CNTV2Card::SetHDMIHDRMaxContentLightLevel(const uint16_t inMaxContentLightLevel)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  NTV2_IS_VALID_HDR_LIGHT_LEVEL(inMaxContentLightLevel)
		&&  WriteRegister(kRegHDMIHDRLightLevel, uint32_t(inMaxContentLightLevel), kRegMaskHDMIHDRMaxContentLightLevel, kRegShiftHDMIHDRMaxContentLightLevel);
}

bool CNTV2Card::GetHDMIHDRMaxContentLightLevel(uint16_t & outMaxContentLightLevel)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRLightLevel, outMaxContentLightLevel, kRegMaskHDMIHDRMaxContentLightLevel, kRegShiftHDMIHDRMaxContentLightLevel);
}

bool CNTV2Card::SetHDMIHDRMaxFrameAverageLightLevel(const uint16_t inMaxFrameAverageLightLevel)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  NTV2_IS_VALID_HDR_LIGHT_LEVEL(inMaxFrameAverageLightLevel)
		&&  WriteRegister(kRegHDMIHDRLightLevel, uint32_t(inMaxFrameAverageLightLevel), kRegMaskHDMIHDRMaxFrameAverageLightLevel, kRegShiftHDMIHDRMaxFrameAverageLightLevel);
}

bool CNTV2Card::GetHDMIHDRMaxFrameAverageLightLevel(uint16_t & outMaxFrameAverageLightLevel)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRLightLevel, outMaxFrameAverageLightLevel, kRegMaskHDMIHDRMaxFrameAverageLightLevel, kRegShiftHDMIHDRMaxFrameAverageLightLevel);
}

bool CNTV2Card::SetHDMIHDRConstantLuminance(const bool inEnableConstantLuminance)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRControl, inEnableConstantLuminance ? 1 : 0, kRegMaskHDMIHDRNonContantLuminance, kRegShiftHDMIHDRNonContantLuminance);
}

bool CNTV2Card::GetHDMIHDRConstantLuminance()
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t regValue = 0;
	ReadRegister(kRegHDMIHDRControl, regValue, kRegMaskHDMIHDRNonContantLuminance, kRegShiftHDMIHDRNonContantLuminance);
	return regValue ? true : false;
}

bool CNTV2Card::SetHDMIHDRElectroOpticalTransferFunction(const uint8_t inEOTFByte)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	return WriteRegister(kRegHDMIHDRControl, inEOTFByte, kRegMaskElectroOpticalTransferFunction, kRegShiftElectroOpticalTransferFunction);
}

bool CNTV2Card::GetHDMIHDRElectroOpticalTransferFunction(uint8_t & outEOTFByte)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRControl, outEOTFByte, kRegMaskElectroOpticalTransferFunction, kRegShiftElectroOpticalTransferFunction);
}

bool CNTV2Card::SetHDMIHDRStaticMetadataDescriptorID(const uint8_t inSMDId)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  WriteRegister(kRegHDMIHDRControl, uint32_t(inSMDId), kRegMaskHDRStaticMetadataDescriptorID, kRegShiftHDRStaticMetadataDescriptorID);
}

bool CNTV2Card::GetHDMIHDRStaticMetadataDescriptorID(uint8_t & outSMDId)
{
	return ::NTV2DeviceCanDoHDMIHDROut(_boardID)
		&&  CNTV2DriverInterface::ReadRegister(kRegHDMIHDRControl, outSMDId, kRegMaskHDRStaticMetadataDescriptorID, kRegShiftHDRStaticMetadataDescriptorID);
}

bool CNTV2Card::EnableHDMIHDR(const bool inEnableHDMIHDR)
{
	bool status = true;
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	status = WriteRegister(kRegHDMIHDRControl, inEnableHDMIHDR ? 1 : 0, kRegMaskHDMIHDREnable, kRegShiftHDMIHDREnable);
	WaitForOutputFieldID(NTV2_FIELD0, NTV2_CHANNEL1);
	return status;
}

bool CNTV2Card::GetHDMIHDREnabled (void)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t regValue = 0;
	ReadRegister(kRegHDMIHDRControl, regValue, kRegMaskHDMIHDREnable, kRegShiftHDMIHDREnable);
	return regValue ? true : false;
}

bool CNTV2Card::EnableHDMIHDRDolbyVision(const bool inEnable)
{
    bool status = true;
    if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
        return false;
    status = WriteRegister(kRegHDMIHDRControl, inEnable ? 1 : 0, kRegMaskHDMIHDRDolbyVisionEnable, kRegShiftHDMIHDRDolbyVisionEnable);
    WaitForOutputFieldID(NTV2_FIELD0, NTV2_CHANNEL1);
    return status;
}

bool CNTV2Card::GetHDMIHDRDolbyVisionEnabled (void)
{
    if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
        return false;
    uint32_t regValue = 0;
    ReadRegister(kRegHDMIHDRControl, regValue, kRegMaskHDMIHDRDolbyVisionEnable, kRegShiftHDMIHDRDolbyVisionEnable);
    return regValue ? true : false;
}

bool CNTV2Card::SetHDRData (const HDRFloatValues & inFloatValues)
{
	HDRRegValues registerValues;
	convertHDRFloatToRegisterValues(inFloatValues, registerValues);
    SetHDRData(registerValues);
	return true;
}

bool CNTV2Card::SetHDRData(const HDRRegValues & inRegisterValues)
{
	SetHDMIHDRGreenPrimaryX(inRegisterValues.greenPrimaryX);
	SetHDMIHDRGreenPrimaryY(inRegisterValues.greenPrimaryY);
	SetHDMIHDRBluePrimaryX(inRegisterValues.bluePrimaryX);
	SetHDMIHDRBluePrimaryY(inRegisterValues.bluePrimaryY);
	SetHDMIHDRRedPrimaryX(inRegisterValues.redPrimaryX);
	SetHDMIHDRRedPrimaryY(inRegisterValues.redPrimaryY);
	SetHDMIHDRWhitePointX(inRegisterValues.whitePointX);
	SetHDMIHDRWhitePointY(inRegisterValues.whitePointY);
    SetHDMIHDRMaxMasteringLuminance(inRegisterValues.maxMasteringLuminance);
    SetHDMIHDRMinMasteringLuminance(inRegisterValues.minMasteringLuminance);
    SetHDMIHDRMaxContentLightLevel(inRegisterValues.maxContentLightLevel);
    SetHDMIHDRMaxFrameAverageLightLevel(inRegisterValues.maxFrameAverageLightLevel);
    SetHDMIHDRElectroOpticalTransferFunction(inRegisterValues.electroOpticalTransferFunction);
    SetHDMIHDRStaticMetadataDescriptorID(inRegisterValues.staticMetadataDescriptorID);
	return true;
}

bool CNTV2Card::GetHDRData(HDRFloatValues & outFloatValues)
{
    HDRRegValues registerValues;
    GetHDRData(registerValues);
    return convertHDRRegisterToFloatValues(registerValues, outFloatValues);
}

bool CNTV2Card::GetHDRData(HDRRegValues & outRegisterValues)
{
    GetHDMIHDRGreenPrimaryX(outRegisterValues.greenPrimaryX);
    GetHDMIHDRGreenPrimaryY(outRegisterValues.greenPrimaryY);
    GetHDMIHDRBluePrimaryX(outRegisterValues.bluePrimaryX);
    GetHDMIHDRBluePrimaryY(outRegisterValues.bluePrimaryY);
    GetHDMIHDRRedPrimaryX(outRegisterValues.redPrimaryX);
    GetHDMIHDRRedPrimaryY(outRegisterValues.redPrimaryY);
    GetHDMIHDRWhitePointX(outRegisterValues.whitePointX);
    GetHDMIHDRWhitePointY(outRegisterValues.whitePointY);
    GetHDMIHDRMaxMasteringLuminance(outRegisterValues.maxMasteringLuminance);
    GetHDMIHDRMinMasteringLuminance(outRegisterValues.minMasteringLuminance);
    GetHDMIHDRMaxContentLightLevel(outRegisterValues.maxContentLightLevel);
    GetHDMIHDRMaxFrameAverageLightLevel(outRegisterValues.maxFrameAverageLightLevel);
    GetHDMIHDRElectroOpticalTransferFunction(outRegisterValues.electroOpticalTransferFunction);
    GetHDMIHDRStaticMetadataDescriptorID(outRegisterValues.staticMetadataDescriptorID);
    return true;
}

bool CNTV2Card::SetHDMIHDRBT2020()
{
	HDRRegValues registerValues;
    setHDRDefaultsForBT2020(registerValues);
	EnableHDMIHDR(false);
	SetHDRData(registerValues);
	EnableHDMIHDR(true);
	return true;
}

bool CNTV2Card::SetHDMIHDRDCIP3()
{
	HDRRegValues registerValues;
    setHDRDefaultsForDCIP3(registerValues);
	EnableHDMIHDR(false);
	SetHDRData(registerValues);
	EnableHDMIHDR(true);
	return true;
}

bool CNTV2Card::EnableHDMIOutUserOverride(bool enable)
{
	return WriteRegister(kRegHDMIInputControl, enable ? 1 : 0, kRegMaskHDMIOutUserOveride, kRegShiftHDMIOutUserOveride);
}

bool CNTV2Card::GetEnableHDMIOutUserOverride(bool & isEnabled)
{
	ULWord enable = 0;
	bool status = ReadRegister(kRegHDMIInputControl, enable, kRegMaskHDMIOutUserOveride, kRegShiftHDMIOutUserOveride);
	isEnabled = enable ? true : false;
	return status;
}

bool CNTV2Card::EnableHDMIOutCenterCrop(bool enable)
{
	return WriteRegister(kRegHDMIInputControl, enable ? 1 : 0, kRegMaskHDMIOutCropMode, kRegShiftHDMIOutCropMode);
}

bool CNTV2Card::GetEnableHDMIOutCenterCrop(bool & isEnabled)
{
	ULWord enable = 0;
	bool status = ReadRegister(kRegHDMIInputControl, enable, kRegMaskHDMIOutCropMode, kRegShiftHDMIOutCropMode);
	isEnabled = enable ? true : false;
	return status;
}



//////////////////////////////////////////////////////////////

#ifdef MSWindows
#pragma warning(default: 4800)
#endif
