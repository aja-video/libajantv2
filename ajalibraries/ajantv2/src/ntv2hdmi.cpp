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


//////////////////////////////////////////////////////////////////
// HDMI

// kRegHDMIInputStatus
bool CNTV2Card::GetHDMIInputStatusRegister			(ULWord & outValue, const NTV2Channel inChannel)
{
    ULWord numInputs = NTV2DeviceGetNumHDMIVideoInputs(_boardID);
    if (numInputs < 1) return false;
    if (numInputs == 1)
    {
        return ReadRegister  (kRegHDMIInputStatus,		&outValue);
    }
	if (inChannel < (NTV2Channel)numInputs)
	{
		return ReadRegister  (gHDMIChannelToInputStatusRegNum[inChannel],		&outValue);
	}
	return false;
}

bool CNTV2Card::GetHDMIInputColor					(NTV2LHIHDMIColorSpace & outValue, const NTV2Channel inChannel)
{
    ULWord numInputs = NTV2DeviceGetNumHDMIVideoInputs(_boardID);
    if (numInputs < 1) return false;
    if (numInputs == 1)
    {
        return ReadRegister  (kRegHDMIInputStatus,		(ULWord*)&outValue,	kLHIRegMaskHDMIInputColorSpace, kLHIRegShiftHDMIInputColorSpace);
    }
	if (inChannel <= (NTV2Channel)numInputs)
	{
		return ReadRegister  (gHDMIChannelToInputStatusRegNum[inChannel],		(ULWord*)&outValue,	kLHIRegMaskHDMIInputColorSpace, kLHIRegShiftHDMIInputColorSpace);
	}
	return false;
}

// kRegHDMIInputControl
bool CNTV2Card::SetHDMIInputRange (const NTV2HDMIRange inValue)
{
	return WriteRegister (kRegHDMIInputControl,	ULWord(inValue), kRegMaskHDMIInputRange, kRegShiftHDMIInputRange);
}

bool CNTV2Card::GetHDMIInputRange (NTV2HDMIRange & outValue)
{
	return ReadRegister  (kRegHDMIInputControl,	(ULWord*)&outValue, kRegMaskHDMIInputRange, kRegShiftHDMIInputRange);
}

bool CNTV2Card::SetHDMIInColorSpace (const NTV2HDMIColorSpace inValue)
{
	return WriteRegister (kRegHDMIInputControl,	ULWord(inValue), kRegMaskHDMIColorSpace, kRegShiftHDMIColorSpace);
}

bool CNTV2Card::GetHDMIInColorSpace (NTV2HDMIColorSpace & outValue)
{
	return ReadRegister  (kRegHDMIInputControl,	(ULWord*)&outValue,	kRegMaskHDMIColorSpace, kRegShiftHDMIColorSpace);
}

// kRegHDMIOut3DControl
bool CNTV2Card::SetHDMIOut3DPresent (bool value)
{
	return WriteRegister (kRegHDMIOut3DControl,	(ULWord)value, kRegMaskHDMIOut3DPresent, kRegShiftHDMIOut3DPresent);
}

bool CNTV2Card::GetHDMIOut3DPresent (bool & outValue)
{
	ULWord	tempVal	(0);
	bool	retVal	(ReadRegister (kRegHDMIOut3DControl, &tempVal, kRegMaskHDMIOut3DPresent, kRegShiftHDMIOut3DPresent));
	if (retVal)
		outValue = (bool) tempVal;
	return retVal;
}

bool CNTV2Card::SetHDMIOut3DMode (const NTV2HDMIOut3DMode inValue)
{
	return WriteRegister (kRegHDMIOut3DControl,	ULWord(inValue), kRegMaskHDMIOut3DMode, kRegShiftHDMIOut3DMode);
}

bool CNTV2Card::GetHDMIOut3DMode (NTV2HDMIOut3DMode & outValue)
{
	return ReadRegister (kRegHDMIOut3DControl, (ULWord*)&outValue, kRegMaskHDMIOut3DMode, kRegShiftHDMIOut3DMode);
}

// kRegHDMIOutControl
bool CNTV2Card::SetHDMIOutVideoStandard (const NTV2Standard inValue)
{
	const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
	if (hdmiVers > 0)
		return WriteRegister (kRegHDMIOutControl,	(ULWord)inValue,	hdmiVers == 1  ?  kRegMaskHDMIOutVideoStd  :  kRegMaskHDMIOutV2VideoStd,	kRegShiftHDMIOutVideoStd);
	return false;
}

bool CNTV2Card::GetHDMIOutVideoStandard (NTV2Standard & outValue)
{
	const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
	if (hdmiVers > 0)
		return ReadRegister  (kRegHDMIOutControl,	(ULWord*)&outValue,		hdmiVers == 1  ?  kRegMaskHDMIOutVideoStd  :  kRegMaskHDMIOutV2VideoStd,	kRegShiftHDMIOutVideoStd);
	else
		outValue = NTV2_STANDARD_INVALID;
	return false;
}

bool CNTV2Card::SetHDMIV2TxBypass (bool bypass)
{
	return WriteRegister (kRegHDMIOutControl,		bypass,				kRegMaskHDMIV2TxBypass,					kRegShiftHDMIV2TxBypass);
}

// HDMI V2 includes an extra bit for quad formats
#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::SetHDMIV2OutVideoStandard (NTV2V2Standard value)				
	{
		return WriteRegister (kRegHDMIOutControl,		(ULWord)value,		kRegMaskHDMIOutV2VideoStd,				kRegShiftHDMIOutVideoStd);
	}

	bool CNTV2Card::GetHDMIV2OutVideoStandard		(NTV2V2Standard * pOutValue)		
	{
		return ReadRegister  (kRegHDMIOutControl,		(ULWord*)pOutValue,	kRegMaskHDMIOutV2VideoStd,				kRegShiftHDMIOutVideoStd);
	}
#endif	//	!defined (NTV2_DEPRECATE)

bool CNTV2Card::SetHDMISampleStructure (NTV2HDMISampleStructure value)		
{
	return WriteRegister (kRegHDMIOutControl,		(ULWord)value,		kRegMaskHDMISampling,					kRegShiftHDMISampling);
}

bool CNTV2Card::GetHDMISampleStructure (NTV2HDMISampleStructure & outValue)
{
	return ReadRegister  (kRegHDMIOutControl,		(ULWord*)&outValue,	kRegMaskHDMISampling,					kRegShiftHDMISampling);
}

bool CNTV2Card::SetHDMIOutVideoFPS (NTV2FrameRate value)				
{
	return WriteRegister (kRegHDMIOutControl,		(ULWord)value,		kLHIRegMaskHDMIOutFPS,					kLHIRegShiftHDMIOutFPS);
}

bool CNTV2Card::GetHDMIOutVideoFPS (NTV2FrameRate & outValue)			
{
	return ReadRegister  (kRegHDMIOutControl,		(ULWord*)&outValue,	kLHIRegMaskHDMIOutFPS,					kLHIRegShiftHDMIOutFPS);
}

bool CNTV2Card::SetHDMIOutRange (NTV2HDMIRange value)				
{
	return WriteRegister (kRegHDMIOutControl,		(ULWord)value,		kRegMaskHDMIOutRange,					kRegShiftHDMIOutRange);
}

bool CNTV2Card::GetHDMIOutRange (NTV2HDMIRange & outValue)			
{
	return ReadRegister  (kRegHDMIOutControl,		(ULWord*)&outValue,	kRegMaskHDMIOutRange,					kRegShiftHDMIOutRange);
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
	return WriteRegister (kRegHDMIOutControl,  newCorrectValue,  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace);
}

bool CNTV2Card::GetHDMIOutColorSpace (NTV2HDMIColorSpace & outValue)		
{	//	Register 125		This function used to read bits 4 & 5 (from the old FS/1 days) --- now obsolete.
	//return ReadRegister (kRegHDMIOutControl,  (ULWord*)&outValue,  kRegMaskHDMIColorSpace,  kRegShiftHDMIColorSpace);
	//	Fixed in SDK 14.1 to work with modern NTV2 devices, but using the old NTV2HDMIColorSpace enum:
	ULWord	correctValue(0);
	//					Register 125						Bit 8
	if (!ReadRegister (kRegHDMIOutControl,  &correctValue,  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace))
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
	bool result (ReadRegister (kRegHDMIOutControl,  &value,  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace));
	outValue = NTV2LHIHDMIColorSpace(value);
	return result;
}



bool CNTV2Card::GetHDMIOutDownstreamBitDepth (NTV2HDMIBitDepth & outValue)		
{
	return ReadRegister  (kRegHDMIInputStatus,		(ULWord*)&outValue,	kLHIRegMaskHDMIOutputEDID10Bit,			kLHIRegShiftHDMIOutputEDID10Bit);
}

bool CNTV2Card::GetHDMIOutDownstreamColorSpace (NTV2LHIHDMIColorSpace & outValue)	
{
	return ReadRegister  (kRegHDMIInputStatus,		(ULWord*)&outValue,	kLHIRegMaskHDMIOutputEDIDRGB,			kLHIRegShiftHDMIOutputEDIDRGB);
}

bool CNTV2Card::SetHDMIOutBitDepth (NTV2HDMIBitDepth value)			
{
	return WriteRegister (kRegHDMIOutControl,		(ULWord)value,		kLHIRegMaskHDMIOutBitDepth,				kLHIRegShiftHDMIOutBitDepth);
}

bool CNTV2Card::GetHDMIOutBitDepth (NTV2HDMIBitDepth & outValue)		
{
	return ReadRegister  (kRegHDMIOutControl,		(ULWord*)&outValue,	kLHIRegMaskHDMIOutBitDepth,				kLHIRegShiftHDMIOutBitDepth);
}

bool CNTV2Card::SetHDMIOutProtocol (NTV2HDMIProtocol value)			
{
	return WriteRegister (kRegHDMIOutControl,		(ULWord)value,		kRegMaskHDMIProtocol,					kRegShiftHDMIProtocol);
}

bool CNTV2Card::GetHDMIOutProtocol (NTV2HDMIProtocol & outValue)		
{
	return ReadRegister  (kRegHDMIOutControl,		(ULWord*)&outValue,	kRegMaskHDMIProtocol,					kRegShiftHDMIProtocol);
}



bool CNTV2Card::SetHDMIV2DecimateMode(bool enable)
{
	return WriteRegister(kRegRasterizerControl, enable, kRegMaskRasterDecimate, kRegShiftRasterDecimate);
}

bool CNTV2Card::GetHDMIV2DecimateMode(bool & outIsEnabled)
{
	ULWord		tempVal	(0);
	const bool	retVal	(ReadRegister (kRegRasterizerControl, &tempVal, kRegMaskRasterDecimate, kRegShiftRasterDecimate));
	outIsEnabled = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::SetHDMIV2TsiIO(bool tsiEnable)
{
	return WriteRegister(kRegRasterizerControl, tsiEnable, kRegMaskTsiIO, kRegShiftTsiIO);
}

bool CNTV2Card::GetHDMIV2TsiIO(bool & tsiEnabled)
{
	ULWord		tempVal(0);
	const bool	retVal(ReadRegister(kRegRasterizerControl, &tempVal, kRegMaskTsiIO, kRegShiftTsiIO));
	tsiEnabled = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::SetHDMIV2LevelBMode(bool enable)
{
	return WriteRegister(kRegRasterizerControl, enable, kRegMaskRasterLevelB, kRegShiftRasterLevelB);
}

bool CNTV2Card::GetHDMIV2LevelBMode(bool & outIsEnabled)
{
	ULWord		tempVal	(0);
	const bool	retVal	(ReadRegister (kRegRasterizerControl, &tempVal, kRegMaskRasterLevelB, kRegShiftRasterLevelB));
	outIsEnabled = static_cast <bool> (tempVal);
	return retVal;
}

bool CNTV2Card::SetHDMIV2Mode(NTV2HDMIV2Mode mode)
{
	return WriteRegister(kRegRasterizerControl, mode, kRegMaskRasterMode, kRegShiftRasterMode);
}

bool CNTV2Card::GetHDMIV2Mode(NTV2HDMIV2Mode & outMode)
{
	ULWord		tmpValue	(0);
	const bool	result		(ReadRegister(kRegRasterizerControl, &tmpValue, kRegMaskRasterMode, kRegShiftRasterMode));
	outMode = static_cast <NTV2HDMIV2Mode> (tmpValue);
	return result;
}

bool CNTV2Card::SetHDMIHDRGreenPrimaryX(const uint16_t inGreenPrimaryX)
{
	if(!NTV2_IS_VALID_HDR_PRIMARY(inGreenPrimaryX) && NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	return WriteRegister(kRegHDMIHDRGreenPrimary, (uint32_t)inGreenPrimaryX, kRegMaskHDMIHDRGreenPrimaryX, kRegShiftHDMIHDRGreenPrimaryX);
}

bool CNTV2Card::GetHDMIHDRGreenPrimaryX(uint16_t & outGreenPrimaryX)
{
	if(!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outGreenPrimaryX;
	bool result = ReadRegister(kRegHDMIHDRGreenPrimary, &value, kRegMaskHDMIHDRGreenPrimaryX, kRegShiftHDMIHDRGreenPrimaryX);
	outGreenPrimaryX = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRGreenPrimaryY(const uint16_t inGreenPrimaryY)
{
	if (!NTV2_IS_VALID_HDR_PRIMARY(inGreenPrimaryY) && NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	return WriteRegister(kRegHDMIHDRGreenPrimary, (uint32_t)inGreenPrimaryY, kRegMaskHDMIHDRGreenPrimaryY, kRegShiftHDMIHDRGreenPrimaryY);
}

bool CNTV2Card::GetHDMIHDRGreenPrimaryY(uint16_t & outGreenPrimaryY)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outGreenPrimaryY;
	bool result = ReadRegister(kRegHDMIHDRGreenPrimary, &value, kRegMaskHDMIHDRGreenPrimaryY, kRegShiftHDMIHDRGreenPrimaryY);
	outGreenPrimaryY = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRBluePrimaryX(const uint16_t inBluePrimaryX)
{
	if (!NTV2_IS_VALID_HDR_PRIMARY(inBluePrimaryX) && NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
    return WriteRegister(kRegHDMIHDRBluePrimary, (uint32_t)inBluePrimaryX, kRegMaskHDMIHDRBluePrimaryX, kRegShiftHDMIHDRBluePrimaryX);
}

bool CNTV2Card::GetHDMIHDRBluePrimaryX(uint16_t & outBluePrimaryX)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outBluePrimaryX;
	bool result = ReadRegister(kRegHDMIHDRBluePrimary, &value, kRegMaskHDMIHDRBluePrimaryX, kRegShiftHDMIHDRBluePrimaryX);
	outBluePrimaryX = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRBluePrimaryY(const uint16_t inBluePrimaryY)
{
	if (!NTV2_IS_VALID_HDR_PRIMARY(inBluePrimaryY) && NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
    return WriteRegister(kRegHDMIHDRBluePrimary, (uint32_t)inBluePrimaryY, kRegMaskHDMIHDRBluePrimaryY, kRegShiftHDMIHDRBluePrimaryY);
}

bool CNTV2Card::GetHDMIHDRBluePrimaryY(uint16_t & outBluePrimaryY)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outBluePrimaryY;
	bool result = ReadRegister(kRegHDMIHDRBluePrimary, &value, kRegMaskHDMIHDRBluePrimaryY, kRegShiftHDMIHDRBluePrimaryY);
	outBluePrimaryY = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRRedPrimaryX(const uint16_t inRedPrimaryX)
{
	if (!NTV2_IS_VALID_HDR_PRIMARY(inRedPrimaryX) && NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
    return WriteRegister(kRegHDMIHDRRedPrimary, (uint32_t)inRedPrimaryX, kRegMaskHDMIHDRRedPrimaryX, kRegShiftHDMIHDRRedPrimaryX);
}

bool CNTV2Card::GetHDMIHDRRedPrimaryX(uint16_t & outRedPrimaryX)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outRedPrimaryX;
	bool result = ReadRegister(kRegHDMIHDRRedPrimary, &value, kRegMaskHDMIHDRRedPrimaryX, kRegShiftHDMIHDRRedPrimaryX);
	outRedPrimaryX = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRRedPrimaryY(const uint16_t inRedPrimaryY)
{
	if (!NTV2_IS_VALID_HDR_PRIMARY(inRedPrimaryY) && NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
    return WriteRegister(kRegHDMIHDRRedPrimary, (uint32_t)inRedPrimaryY, kRegMaskHDMIHDRRedPrimaryY, kRegShiftHDMIHDRRedPrimaryY);
}

bool CNTV2Card::GetHDMIHDRRedPrimaryY(uint16_t & outRedPrimaryY)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outRedPrimaryY;
	bool result = ReadRegister(kRegHDMIHDRRedPrimary, &value, kRegMaskHDMIHDRRedPrimaryY, kRegShiftHDMIHDRRedPrimaryY);
	outRedPrimaryY = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRWhitePointX(const uint16_t inWhitePointX)
{
	if (!NTV2_IS_VALID_HDR_PRIMARY(inWhitePointX) && NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
    return WriteRegister(kRegHDMIHDRWhitePoint, (uint32_t)inWhitePointX, kRegMaskHDMIHDRWhitePointX, kRegShiftHDMIHDRWhitePointX);
}

bool CNTV2Card::GetHDMIHDRWhitePointX(uint16_t & outWhitePointX)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outWhitePointX;
	bool result = ReadRegister(kRegHDMIHDRWhitePoint, &value, kRegMaskHDMIHDRWhitePointX, kRegShiftHDMIHDRWhitePointX);
	outWhitePointX = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRWhitePointY(const uint16_t inWhitePointY)
{
	if (!NTV2_IS_VALID_HDR_PRIMARY(inWhitePointY) && NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
    return WriteRegister(kRegHDMIHDRWhitePoint, (uint32_t)inWhitePointY, kRegMaskHDMIHDRWhitePointY, kRegShiftHDMIHDRWhitePointY);
}

bool CNTV2Card::GetHDMIHDRWhitePointY(uint16_t & outWhitePointY)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outWhitePointY;
	bool result = ReadRegister(kRegHDMIHDRWhitePoint, &value, kRegMaskHDMIHDRWhitePointY, kRegShiftHDMIHDRWhitePointY);
	outWhitePointY = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRMaxMasteringLuminance(const uint16_t inMaxMasteringLuminance)
{
	if (::NTV2DeviceCanDoHDMIHDROut(_boardID)  &&  !NTV2_IS_VALID_HDR_MASTERING_LUMINENCE(inMaxMasteringLuminance))
		return false;
	return WriteRegister(kRegHDMIHDRMasteringLuminence, (uint32_t)inMaxMasteringLuminance, kRegMaskHDMIHDRMaxMasteringLuminance, kRegShiftHDMIHDRMaxMasteringLuminance);
}

bool CNTV2Card::GetHDMIHDRMaxMasteringLuminance(uint16_t & outMaxMasteringLuminance)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outMaxMasteringLuminance;
	bool result = ReadRegister(kRegHDMIHDRMasteringLuminence, &value, kRegMaskHDMIHDRMaxMasteringLuminance, kRegShiftHDMIHDRMaxMasteringLuminance);
	outMaxMasteringLuminance = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRMinMasteringLuminance(const uint16_t inMinMasteringLuminance)
{
	if (::NTV2DeviceCanDoHDMIHDROut(_boardID)  &&  !NTV2_IS_VALID_HDR_MASTERING_LUMINENCE(inMinMasteringLuminance))
		return false;
	return WriteRegister(kRegHDMIHDRMasteringLuminence, (uint32_t)inMinMasteringLuminance, kRegMaskHDMIHDRMinMasteringLuminance, kRegShiftHDMIHDRMinMasteringLuminance);
}

bool CNTV2Card::GetHDMIHDRMinMasteringLuminance(uint16_t & outMinMasteringLuminance)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outMinMasteringLuminance;
	bool result = ReadRegister(kRegHDMIHDRMasteringLuminence, &value, kRegMaskHDMIHDRMinMasteringLuminance, kRegShiftHDMIHDRMinMasteringLuminance);
	outMinMasteringLuminance = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRMaxContentLightLevel(const uint16_t inMaxContentLightLevel)
{
	if (::NTV2DeviceCanDoHDMIHDROut(_boardID)  &&  !NTV2_IS_VALID_HDR_LIGHT_LEVEL(inMaxContentLightLevel))
		return false;
	return WriteRegister(kRegHDMIHDRLightLevel, (uint32_t)inMaxContentLightLevel, kRegMaskHDMIHDRMaxContentLightLevel, kRegShiftHDMIHDRMaxContentLightLevel);
}

bool CNTV2Card::GetHDMIHDRMaxContentLightLevel(uint16_t & outMaxContentLightLevel)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outMaxContentLightLevel;
	bool result = ReadRegister(kRegHDMIHDRLightLevel, &value, kRegMaskHDMIHDRMaxContentLightLevel, kRegShiftHDMIHDRMaxContentLightLevel);
	outMaxContentLightLevel = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRMaxFrameAverageLightLevel(const uint16_t inMaxFrameAverageLightLevel)
{
	if (::NTV2DeviceCanDoHDMIHDROut(_boardID)  &&  !NTV2_IS_VALID_HDR_LIGHT_LEVEL(inMaxFrameAverageLightLevel))
		return false;
	return WriteRegister(kRegHDMIHDRLightLevel, (uint32_t)inMaxFrameAverageLightLevel, kRegMaskHDMIHDRMaxFrameAverageLightLevel, kRegShiftHDMIHDRMaxFrameAverageLightLevel);
}

bool CNTV2Card::GetHDMIHDRMaxFrameAverageLightLevel(uint16_t & outMaxFrameAverageLightLevel)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outMaxFrameAverageLightLevel;
	bool result = ReadRegister(kRegHDMIHDRLightLevel, &value, kRegMaskHDMIHDRMaxFrameAverageLightLevel, kRegShiftHDMIHDRMaxFrameAverageLightLevel);
	outMaxFrameAverageLightLevel = (uint16_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRConstantLuminance(const bool inEnableConstantLuminance)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	return WriteRegister(kRegHDMIHDRControl, inEnableConstantLuminance ? 1 : 0, kRegMaskHDMIHDRNonContantLuminance, kRegShiftHDMIHDRNonContantLuminance);
}

bool CNTV2Card::GetHDMIHDRConstantLuminance()
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t regValue = 0;
	ReadRegister(kRegHDMIHDRControl, &regValue, kRegMaskHDMIHDRNonContantLuminance, kRegShiftHDMIHDRNonContantLuminance);
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
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outEOTFByte;
	bool result = ReadRegister(kRegHDMIHDRControl, &value, kRegMaskElectroOpticalTransferFunction, kRegShiftElectroOpticalTransferFunction);
	outEOTFByte = (uint8_t) value;
	return result;
}

bool CNTV2Card::SetHDMIHDRStaticMetadataDescriptorID(const uint8_t inSMDId)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	return WriteRegister(kRegHDMIHDRControl, inSMDId, kRegMaskHDRStaticMetadataDescriptorID, kRegShiftHDRStaticMetadataDescriptorID);
}

bool CNTV2Card::GetHDMIHDRStaticMetadataDescriptorID(uint8_t & outSMDId)
{
	if (!NTV2DeviceCanDoHDMIHDROut(_boardID))
		return false;
	uint32_t value = outSMDId;
	bool result = ReadRegister(kRegHDMIHDRControl, &value, kRegMaskHDRStaticMetadataDescriptorID, kRegShiftHDRStaticMetadataDescriptorID);
	outSMDId = (uint8_t) value;
	return result;
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
	ReadRegister(kRegHDMIHDRControl, &regValue, kRegMaskHDMIHDREnable, kRegShiftHDMIHDREnable);
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
    ReadRegister(kRegHDMIHDRControl, &regValue, kRegMaskHDMIHDRDolbyVisionEnable, kRegShiftHDMIHDRDolbyVisionEnable);
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
	bool status = ReadRegister(kRegHDMIInputControl, &enable, kRegMaskHDMIOutUserOveride, kRegShiftHDMIOutUserOveride);
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
	bool status = ReadRegister(kRegHDMIInputControl, &enable, kRegMaskHDMIOutCropMode, kRegShiftHDMIOutCropMode);
	isEnabled = enable ? true : false;
	return status;
}



//////////////////////////////////////////////////////////////

#ifdef MSWindows
#pragma warning(default: 4800)
#endif
