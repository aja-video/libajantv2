/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2hdmi.cpp
    @brief		Implements most of CNTV2Card's HDMI-related functions.
    @copyright	(C) 2004-2022 AJA Video Systems, Inc.
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


static const ULWord gHDMIChannelToInputStatusRegNum [] =	{ kRegHDMIInputStatus1, kRegHDMIInputStatus2, kRegHDMIInputStatus3, kRegHDMIInputStatus4, 0 };
static const ULWord gHDMIChannelToControlRegNum [] =	{ kRegHDMIControl1, kRegHDMIControl2, kRegHDMIControl3, kRegHDMIControl4, 0 };

// hdmi version 6 mulitple output virtual registers
static const ULWord gHDMIChannelToOutControlVRegNum [] =	{ kVRegHDMIOutControl1, kVRegHDMIOutControl2, kVRegHDMIOutControl3, kVRegHDMIOutControl4, kVRegHDMIOutControl1, kVRegHDMIOutControl1, kVRegHDMIOutControl1, kVRegHDMIOutControl1 };
static const ULWord gHDMIChannelToInputStatusVRegNum [] =	{ kVRegHDMIInputStatus1, kVRegHDMIInputStatus2, kVRegHDMIInputStatus3, kVRegHDMIInputStatus4, kVRegHDMIInputStatus1, kVRegHDMIInputStatus1, kVRegHDMIInputStatus1, kVRegHDMIInputStatus1 };
static const ULWord gHDMIChannelToInputControlVRegNum [] =	{ kVRegHDMIInputControl1, kVRegHDMIInputControl2, kVRegHDMIInputControl3, kVRegHDMIInputControl4, kVRegHDMIInputControl1, kVRegHDMIInputControl1, kVRegHDMIInputControl1, kVRegHDMIInputControl1 };
static const ULWord gHDMIChannelToOutStatusVRegNum [] =	{ kVRegHDMIOutStatus1, kVRegHDMIOutStatus2, kVRegHDMIOutStatus3, kVRegHDMIOutStatus4, kVRegHDMIOutStatus1, kVRegHDMIOutStatus1, kVRegHDMIOutStatus1, kVRegHDMIOutStatus1 };
static const ULWord gHDMIChannelToOutHDRGreenPrimaryVRegNum [] =	{ kVRegHDMIOutHDRGreenPrimary1, kVRegHDMIOutHDRGreenPrimary2, kVRegHDMIOutHDRGreenPrimary3, kVRegHDMIOutHDRGreenPrimary4, kVRegHDMIOutHDRGreenPrimary1, kVRegHDMIOutHDRGreenPrimary1, kVRegHDMIOutHDRGreenPrimary1, kVRegHDMIOutHDRGreenPrimary1 };
static const ULWord gHDMIChannelToOutHDRRedPrimaryVRegNum [] =	{ kVRegHDMIOutHDRRedPrimary1, kVRegHDMIOutHDRRedPrimary2, kVRegHDMIOutHDRRedPrimary3, kVRegHDMIOutHDRRedPrimary4, kVRegHDMIOutHDRRedPrimary1, kVRegHDMIOutHDRRedPrimary1, kVRegHDMIOutHDRRedPrimary1, kVRegHDMIOutHDRRedPrimary1 };
static const ULWord gHDMIChannelToOutHDRBluePrimaryVRegNum [] =	{ kVRegHDMIOutHDRBluePrimary1, kVRegHDMIOutHDRBluePrimary2, kVRegHDMIOutHDRBluePrimary3, kVRegHDMIOutHDRBluePrimary4, kVRegHDMIOutHDRBluePrimary1, kVRegHDMIOutHDRBluePrimary1, kVRegHDMIOutHDRBluePrimary1, kVRegHDMIOutHDRBluePrimary1 };
static const ULWord gHDMIChannelToOutHDRWhitePointVRegNum [] =	{ kVRegHDMIOutHDRWhitePoint1, kVRegHDMIOutHDRWhitePoint2, kVRegHDMIOutHDRWhitePoint3, kVRegHDMIOutHDRWhitePoint4, kVRegHDMIOutHDRWhitePoint1, kVRegHDMIOutHDRWhitePoint1, kVRegHDMIOutHDRWhitePoint1, kVRegHDMIOutHDRWhitePoint1 };
static const ULWord gHDMIChannelToOutHDRMasterLuminanceVRegNum [] =	{ kVRegHDMIOutHDRMasterLuminance1, kVRegHDMIOutHDRMasterLuminance2, kVRegHDMIOutHDRMasterLuminance3, kVRegHDMIOutHDRMasterLuminance4, kVRegHDMIOutHDRMasterLuminance1, kVRegHDMIOutHDRMasterLuminance1, kVRegHDMIOutHDRMasterLuminance1, kVRegHDMIOutHDRMasterLuminance1 };
static const ULWord gHDMIChannelToOutHDRLightLevelVRegNum [] =	{ kVRegHDMIOutHDRLightLevel1, kVRegHDMIOutHDRLightLevel2, kVRegHDMIOutHDRLightLevel3, kVRegHDMIOutHDRLightLevel4, kVRegHDMIOutHDRLightLevel1, kVRegHDMIOutHDRLightLevel1, kVRegHDMIOutHDRLightLevel1, kVRegHDMIOutHDRLightLevel1 };
static const ULWord gHDMIChannelToOutHDRControlVRegNum [] =	{ kVRegHDMIOutHDRControl1, kVRegHDMIOutHDRControl2, kVRegHDMIOutHDRControl3, kVRegHDMIOutHDRControl4, kVRegHDMIOutHDRControl1, kVRegHDMIOutHDRControl1, kVRegHDMIOutHDRControl1, kVRegHDMIOutHDRControl1 };

//////////////////////////////////////////////////////////////////
// HDMI

// kRegHDMIInputStatus
bool CNTV2Card::GetHDMIInputStatusRegNum (ULWord & outRegNum, const NTV2Channel inChannel, const bool in12BitDetection)
{
    const ULWord	numInputs	(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    outRegNum = 0;
    if (numInputs < 1)
        return false;
    if (inChannel >= NTV2Channel(numInputs))
        return false;
    if (numInputs == 1)
    {
        outRegNum = in12BitDetection ?	kRegHDMIInputControl : kRegHDMIInputStatus;
        return true;
    }
    outRegNum = in12BitDetection ? gHDMIChannelToControlRegNum[inChannel] : gHDMIChannelToInputStatusRegNum[inChannel];
    return true;
}
bool CNTV2Card::GetHDMIInputStatus (ULWord & outValue, const NTV2Channel inChannel, const bool in12BitDetection)
{
    ULWord regNum (0);
    if (!GetHDMIInputStatusRegNum(regNum, inChannel, in12BitDetection))
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

bool CNTV2Card::GetHDMIInDynamicRange (HDRRegValues & outRegValues, const NTV2Channel inChannel)
{
    ULWord outValue;
    memset(&outRegValues, 0, sizeof(HDRRegValues));

    if (inChannel == NTV2_CHANNEL1)
    {
        if (!ReadRegister(kVRegHDMIInDrmInfo1, outValue) || ((outValue & kVRegMaskHDMIInPresent) == 0))
            return false;
        outRegValues.electroOpticalTransferFunction = uint8_t((outValue & kVRegMaskHDMIInEOTF) >> kVRegShiftHDMIInEOTF);
        outRegValues.staticMetadataDescriptorID = uint8_t((outValue & kVRegMaskHDMIInMetadataID) >> kVRegShiftHDMIInMetadataID);
        ReadRegister(kVRegHDMIInDrmGreenPrimary1, outValue);
        outRegValues.greenPrimaryX = uint16_t((outValue & kRegMaskHDMIHDRGreenPrimaryX) >> kRegShiftHDMIHDRGreenPrimaryX);
        outRegValues.greenPrimaryY = uint16_t((outValue & kRegMaskHDMIHDRGreenPrimaryY) >> kRegShiftHDMIHDRGreenPrimaryY);
        ReadRegister(kVRegHDMIInDrmBluePrimary1, outValue);
        outRegValues.bluePrimaryX = uint16_t((outValue & kRegMaskHDMIHDRBluePrimaryX) >> kRegShiftHDMIHDRBluePrimaryX);
        outRegValues.bluePrimaryY = uint16_t((outValue & kRegMaskHDMIHDRBluePrimaryY) >> kRegShiftHDMIHDRBluePrimaryY);
        ReadRegister(kVRegHDMIInDrmRedPrimary1, outValue);
        outRegValues.redPrimaryX = uint16_t((outValue & kRegMaskHDMIHDRRedPrimaryX) >> kRegShiftHDMIHDRRedPrimaryX);
        outRegValues.redPrimaryY = uint16_t((outValue & kRegMaskHDMIHDRRedPrimaryY) >> kRegShiftHDMIHDRRedPrimaryY);
        ReadRegister(kVRegHDMIInDrmWhitePoint1, outValue);
        outRegValues.whitePointX = uint16_t((outValue & kRegMaskHDMIHDRWhitePointX) >> kRegShiftHDMIHDRWhitePointX);
        outRegValues.whitePointY = uint16_t((outValue & kRegMaskHDMIHDRWhitePointY) >> kRegShiftHDMIHDRWhitePointY);
        ReadRegister(kVRegHDMIInDrmMasteringLuminence1, outValue);
        outRegValues.maxMasteringLuminance = uint16_t((outValue & kRegMaskHDMIHDRMaxMasteringLuminance) >> kRegShiftHDMIHDRMaxMasteringLuminance);
        outRegValues.minMasteringLuminance = uint16_t((outValue & kRegMaskHDMIHDRMinMasteringLuminance) >> kRegShiftHDMIHDRMinMasteringLuminance);
        ReadRegister(kVRegHDMIInDrmLightLevel1, outValue);
        outRegValues.maxContentLightLevel = uint16_t((outValue & kRegMaskHDMIHDRMaxContentLightLevel) >> kRegShiftHDMIHDRMaxContentLightLevel);
        outRegValues.maxFrameAverageLightLevel = uint16_t((outValue & kRegMaskHDMIHDRMaxFrameAverageLightLevel) >> kRegShiftHDMIHDRMaxFrameAverageLightLevel);
    }
    else if (inChannel == NTV2_CHANNEL2)
    {
        if (!ReadRegister(kVRegHDMIInDrmInfo2, outValue) || ((outValue & kVRegMaskHDMIInPresent) == 0))
            return false;
        outRegValues.electroOpticalTransferFunction = uint8_t((outValue & kVRegMaskHDMIInEOTF) >> kVRegShiftHDMIInEOTF);
        outRegValues.staticMetadataDescriptorID = uint8_t((outValue & kVRegMaskHDMIInMetadataID) >> kVRegShiftHDMIInMetadataID);
        ReadRegister(kVRegHDMIInDrmGreenPrimary2, outValue);
        outRegValues.greenPrimaryX = uint16_t((outValue & kRegMaskHDMIHDRGreenPrimaryX) >> kRegShiftHDMIHDRGreenPrimaryX);
        outRegValues.greenPrimaryY = uint16_t((outValue & kRegMaskHDMIHDRGreenPrimaryY) >> kRegShiftHDMIHDRGreenPrimaryY);
        ReadRegister(kVRegHDMIInDrmBluePrimary2, outValue);
        outRegValues.bluePrimaryX = uint16_t((outValue & kRegMaskHDMIHDRBluePrimaryX) >> kRegShiftHDMIHDRBluePrimaryX);
        outRegValues.bluePrimaryY = uint16_t((outValue & kRegMaskHDMIHDRBluePrimaryY) >> kRegShiftHDMIHDRBluePrimaryY);
        ReadRegister(kVRegHDMIInDrmRedPrimary2, outValue);
        outRegValues.redPrimaryX = uint16_t((outValue & kRegMaskHDMIHDRRedPrimaryX) >> kRegShiftHDMIHDRRedPrimaryX);
        outRegValues.redPrimaryY = uint16_t((outValue & kRegMaskHDMIHDRRedPrimaryY) >> kRegShiftHDMIHDRRedPrimaryY);
        ReadRegister(kVRegHDMIInDrmWhitePoint2, outValue);
        outRegValues.whitePointX = uint16_t((outValue & kRegMaskHDMIHDRWhitePointX) >> kRegShiftHDMIHDRWhitePointX);
        outRegValues.whitePointY = uint16_t((outValue & kRegMaskHDMIHDRWhitePointY) >> kRegShiftHDMIHDRWhitePointY);
        ReadRegister(kVRegHDMIInDrmMasteringLuminence2, outValue);
        outRegValues.maxMasteringLuminance = uint16_t((outValue & kRegMaskHDMIHDRMaxMasteringLuminance) >> kRegShiftHDMIHDRMaxMasteringLuminance);
        outRegValues.minMasteringLuminance = uint16_t((outValue & kRegMaskHDMIHDRMinMasteringLuminance) >> kRegShiftHDMIHDRMinMasteringLuminance);
        ReadRegister(kVRegHDMIInDrmLightLevel2, outValue);
        outRegValues.maxContentLightLevel = uint16_t((outValue & kRegMaskHDMIHDRMaxContentLightLevel) >> kRegShiftHDMIHDRMaxContentLightLevel);
        outRegValues.maxFrameAverageLightLevel = uint16_t((outValue & kRegMaskHDMIHDRMaxFrameAverageLightLevel) >> kRegShiftHDMIHDRMaxFrameAverageLightLevel);
    }
    else
        return false;
    return true;
}

bool CNTV2Card::GetHDMIInDynamicRange (HDRFloatValues & outFloatValues, const NTV2Channel inChannel)
{
    HDRRegValues registerValues;

    memset(&outFloatValues, 0, sizeof(HDRFloatValues));

    if (!GetHDMIInDynamicRange(registerValues, inChannel))
        return false;

    return convertHDRRegisterToFloatValues(registerValues, outFloatValues);
}

bool CNTV2Card::GetHDMIInColorimetry (NTV2HDMIColorimetry & outColorimetry, const NTV2Channel inChannel)
{
    outColorimetry = NTV2_HDMIColorimetryNoData;
    if (inChannel > NTV2_CHANNEL2)
        return false;
    return CNTV2DriverInterface::ReadRegister (inChannel == NTV2_CHANNEL1 ? kVRegHDMIInAviInfo1 : kVRegHDMIInAviInfo2,
                                                outColorimetry, kVRegMaskHDMIInColorimetry, kVRegShiftHDMIInColorimetry);
}

bool CNTV2Card::GetHDMIInDolbyVision (bool & outIsDolbyVision, const NTV2Channel inChannel)
{
    outIsDolbyVision = false;
    if (inChannel > NTV2_CHANNEL2)
        return false;
    return CNTV2DriverInterface::ReadRegister (inChannel == NTV2_CHANNEL1 ? kVRegHDMIInAviInfo1 : kVRegHDMIInAviInfo2,
                                                outIsDolbyVision, kVRegMaskHDMIInDolbyVision, kVRegShiftHDMIInDolbyVision);
}

// kRegHDMIInputControl
bool CNTV2Card::SetHDMIInputRange (const NTV2HDMIRange inValue, const NTV2Channel inChannel)
{
    const ULWord numInputs(::NTV2DeviceGetNumHDMIVideoInputs(_boardID));
    if (numInputs < 1)
        return false;
    return inChannel == NTV2_CHANNEL1	//	FIX THIS!	MrBill
            && WriteRegister (kRegHDMIInputControl, ULWord(inValue), kRegMaskHDMIInputRange, kRegShiftHDMIInputRange);
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
            && WriteRegister (kRegHDMIInputControl, ULWord(inValue), kRegMaskHDMIColorSpace, kRegShiftHDMIColorSpace);
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
    ULWord status(0), maskVal, shiftVal;
    bool bV2(NTV2DeviceGetHDMIVersion(_boardID) >= 2);
    maskVal = bV2 ? kRegMaskHDMIInColorDepth : kLHIRegMaskHDMIInputBitDepth;
    shiftVal = bV2 ? kRegShiftHDMIInColorDepth : kLHIRegShiftHDMIInputBitDepth;
    if (!GetHDMIInputStatus(status, inChannel, bV2))
        return false;
    outValue = NTV2HDMIBitDepth((status & maskVal) >> shiftVal);
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
    const ULWord	tempVal (!value);	// this is high to disable sample rate conversion
    return inChannel == NTV2_CHANNEL1	//	FIX THIS	MrBill
            && WriteRegister (kRegHDMIInputControl, tempVal, kRegMaskHDMISampleRateConverterEnable, kRegShiftHDMISampleRateConverterEnable);
}


bool CNTV2Card::GetHDMIInAudioSampleRateConverterEnable (bool & outEnabled, const NTV2Channel inChannel)
{
    if (inChannel != NTV2_CHANNEL1)
        return false;	//	FIX THIS	MrBill
    ULWord	tempVal (0);
    bool	retVal	(ReadRegister (kRegHDMIInputControl, tempVal, kRegMaskHDMISampleRateConverterEnable, kRegShiftHDMISampleRateConverterEnable));
    if (retVal)
        outEnabled = !(static_cast <bool> (tempVal));		// this is high to disable sample rate conversion
    return retVal;
}


bool CNTV2Card::GetHDMIInputAudioChannels (NTV2HDMIAudioChannels & outValue, const NTV2Channel inChannel)
{
    if (inChannel != NTV2_CHANNEL1)
        return false;	//	FIX THIS	MrBill
    ULWord	tempVal (0);
    outValue = NTV2_INVALID_HDMI_AUDIO_CHANNELS;
    if (!ReadRegister(kRegHDMIInputStatus, tempVal))
        return false;
    outValue = (tempVal & kLHIRegMaskHDMIInput2ChAudio) ? NTV2_HDMIAudio2Channels : NTV2_HDMIAudio8Channels;
    return true;
}

static const ULWord gKonaHDMICtrlRegs[] = {0x1d16, 0x2516, 0x2c14, 0x3014}; //	KonaHDMI only

bool CNTV2Card::GetHDMIInAudioChannel34Swap (bool & outIsSwapped, const NTV2Channel inChannel)
{
    outIsSwapped = false;
    if (inChannel >= ::NTV2DeviceGetNumHDMIVideoInputs(_boardID))
        return false;	//	No such HDMI input
    if (_boardID == DEVICE_ID_KONAHDMI)
        return CNTV2DriverInterface::ReadRegister(gKonaHDMICtrlRegs[inChannel], outIsSwapped, kRegMaskHDMISwapInputAudCh34, kRegShiftHDMISwapInputAudCh34);	//	TBD
    return CNTV2DriverInterface::ReadRegister(kRegHDMIInputControl, outIsSwapped, kRegMaskHDMISwapInputAudCh34, kRegShiftHDMISwapInputAudCh34);
}

bool CNTV2Card::SetHDMIInAudioChannel34Swap (const bool inIsSwapped, const NTV2Channel inChannel)
{
    if (inChannel >= ::NTV2DeviceGetNumHDMIVideoInputs(_boardID))
        return false;	//	No such HDMI input
    if (_boardID == DEVICE_ID_KONAHDMI)
        return WriteRegister(gKonaHDMICtrlRegs[inChannel], inIsSwapped ? 1 : 0, kRegMaskHDMISwapInputAudCh34, kRegShiftHDMISwapInputAudCh34);	//	TBD
    return WriteRegister(kRegHDMIInputControl, inIsSwapped ? 1 : 0, kRegMaskHDMISwapInputAudCh34, kRegShiftHDMISwapInputAudCh34);
}

// kRegHDMIOut3DControl
bool CNTV2Card::SetHDMIOut3DPresent (const bool value)
{
    return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
            && WriteRegister (kRegHDMIOut3DControl, ULWord(value), kRegMaskHDMIOut3DPresent, kRegShiftHDMIOut3DPresent);
}

bool CNTV2Card::GetHDMIOut3DPresent (bool & outValue)
{
    return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
            && CNTV2DriverInterface::ReadRegister (kRegHDMIOut3DControl, outValue, kRegMaskHDMIOut3DPresent, kRegShiftHDMIOut3DPresent);
}

bool CNTV2Card::SetHDMIOut3DMode (const NTV2HDMIOut3DMode inValue)
{
    return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
            && WriteRegister (kRegHDMIOut3DControl, ULWord(inValue), kRegMaskHDMIOut3DMode, kRegShiftHDMIOut3DMode);
}

bool CNTV2Card::GetHDMIOut3DMode (NTV2HDMIOut3DMode & outValue)
{
    return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
            && CNTV2DriverInterface::ReadRegister (kRegHDMIOut3DControl, outValue, kRegMaskHDMIOut3DMode, kRegShiftHDMIOut3DMode);
}

// kRegHDMIOutControl
bool CNTV2Card::SetHDMIOutVideoStandard (const NTV2Standard inValue, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers    (::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    ULWord reg                  (kRegHDMIOutControl);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;
        
    return WriteRegister (reg, ULWord(inValue),	hdmiVers == 1  ?  kRegMaskHDMIOutVideoStd  :  kRegMaskHDMIOutV2VideoStd,	kRegShiftHDMIOutVideoStd);
}

bool CNTV2Card::GetHDMIOutVideoStandard (NTV2Standard & outValue, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers    (::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    ULWord reg                  (kRegHDMIOutControl);
    
    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
    {
        outValue = NTV2_STANDARD_INVALID;
        return false;
    }

    return CNTV2DriverInterface::ReadRegister (reg, outValue,  hdmiVers == 1	 ?	kRegMaskHDMIOutVideoStd	 :	kRegMaskHDMIOutV2VideoStd,	kRegShiftHDMIOutVideoStd);
}

bool CNTV2Card::SetHDMIV2TxBypass (const bool bypass)
{
    return ::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()) > 0
            && WriteRegister (kRegHDMIOutControl,		bypass,				kRegMaskHDMIV2TxBypass,					kRegShiftHDMIV2TxBypass);
}

bool CNTV2Card::SetHDMIOutSampleStructure (const NTV2HDMISampleStructure inValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg                  (kRegHDMIOutControl);

    if (!NTV2_IS_VALID_HDMI_SAMPLE_STRUCT(inValue))
        return false;
    
    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;
        
    return WriteRegister (reg, ULWord(inValue), kRegMaskHDMISampling, kRegShiftHDMISampling);
}

bool CNTV2Card::GetHDMIOutSampleStructure (NTV2HDMISampleStructure & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg                  (kRegHDMIOutControl);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister (reg, outValue, kRegMaskHDMISampling, kRegShiftHDMISampling);
}

bool CNTV2Card::SetHDMIOutVideoFPS (const NTV2FrameRate value, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg                  (kRegHDMIOutControl);

    if (!NTV2_IS_VALID_NTV2FrameRate(value))
        return false;
    
    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister (reg, ULWord(value), kLHIRegMaskHDMIOutFPS, kLHIRegShiftHDMIOutFPS);
}

bool CNTV2Card::GetHDMIOutVideoFPS (NTV2FrameRate & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg                  (kRegHDMIOutControl);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister (reg, outValue, kLHIRegMaskHDMIOutFPS, kLHIRegShiftHDMIOutFPS);
}

bool CNTV2Card::SetHDMIOutRange (const NTV2HDMIRange value, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg                  (kRegHDMIOutControl);

    if (!NTV2_IS_VALID_HDMI_RANGE(value))
        return false;

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister (reg, ULWord(value), kRegMaskHDMIOutRange, kRegShiftHDMIOutRange);
}

bool CNTV2Card::GetHDMIOutRange (NTV2HDMIRange & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg                  (kRegHDMIOutControl);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister (reg, outValue, kRegMaskHDMIOutRange, kRegShiftHDMIOutRange);
}



bool CNTV2Card::SetHDMIOutColorSpace (const NTV2HDMIColorSpace inValue, const NTV2Channel inWhichHDMIOut)
{	//	Register 125		This function used to touch bits 4 & 5 (from the old FS/1 days) --- now obsolete.
    //return WriteRegister (kRegHDMIOutControl,	 ULWord(value),	 kRegMaskHDMIColorSpace,  kRegShiftHDMIColorSpace);
    //	Fixed in SDK 14.1 to work with modern NTV2 devices, but using the old NTV2HDMIColorSpace enum:
    
    ULWord reg                  (kRegHDMIOutControl);
    ULWord newCorrectValue      (0);
    
    switch (inValue)
    {
        case NTV2_HDMIColorSpaceRGB:	newCorrectValue = NTV2_LHIHDMIColorSpaceRGB;	break;
        case NTV2_HDMIColorSpaceYCbCr:	newCorrectValue = NTV2_LHIHDMIColorSpaceYCbCr;	break;
        default:	return false;	//	Illegal value
    }

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    //						Register 125							Bit 8
    return WriteRegister (reg,  newCorrectValue,	 kLHIRegMaskHDMIOutColorSpace,	kLHIRegShiftHDMIOutColorSpace);
}

bool CNTV2Card::GetHDMIOutColorSpace (NTV2HDMIColorSpace & outValue, const NTV2Channel inWhichHDMIOut)
{	//	Register 125		This function used to read bits 4 & 5 (from the old FS/1 days) --- now obsolete.
    //return ReadRegister (kRegHDMIOutControl,	outValue,  kRegMaskHDMIColorSpace,	kRegShiftHDMIColorSpace);
    //	Fixed in SDK 14.1 to work with modern NTV2 devices, but using the old NTV2HDMIColorSpace enum:

    ULWord reg                  (kRegHDMIOutControl);
    ULWord correctValue         (0);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    //					Register 125						Bit 8
    if (!ReadRegister (reg,	correctValue,  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace))
        return false;	//	Fail
    
    switch(correctValue)
    {
        case NTV2_LHIHDMIColorSpaceYCbCr:	outValue = NTV2_HDMIColorSpaceYCbCr;	break;
        case NTV2_LHIHDMIColorSpaceRGB:		outValue = NTV2_HDMIColorSpaceRGB;		break;
        default:							return false;	//	Bad value?!
    }
    return true;
}


bool CNTV2Card::SetLHIHDMIOutColorSpace (NTV2LHIHDMIColorSpace value, const NTV2Channel inWhichHDMIOut)
{	//	Register 125											Bit 8
    ULWord reg                  (kRegHDMIOutControl);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister (reg,  ULWord(value),  kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace);
}

bool CNTV2Card::GetLHIHDMIOutColorSpace (NTV2LHIHDMIColorSpace & outValue, const NTV2Channel inWhichHDMIOut)
{	//	Register 125												Bit 8
    ULWord reg(kRegHDMIOutControl);
    ULWord	value(0);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;
    
    bool result (ReadRegister (reg,	value,	kLHIRegMaskHDMIOutColorSpace,  kLHIRegShiftHDMIOutColorSpace));
    outValue = NTV2LHIHDMIColorSpace(value);
    return result;
}



bool CNTV2Card::GetHDMIOutDownstreamBitDepth (NTV2HDMIBitDepth & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputStatus);

    if (!GetHDMIInputStatusReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister (reg, outValue, kLHIRegMaskHDMIOutputEDID10Bit, kLHIRegShiftHDMIOutputEDID10Bit);
}

bool CNTV2Card::GetHDMIOutDownstreamColorSpace (NTV2LHIHDMIColorSpace & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputStatus);

    if (!GetHDMIInputStatusReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister (reg, outValue, kLHIRegMaskHDMIOutputEDIDRGB, kLHIRegShiftHDMIOutputEDIDRGB);
}

bool CNTV2Card::SetHDMIOutBitDepth (const NTV2HDMIBitDepth value, const NTV2Channel inWhichHDMIOut)
{
    ULWord regOC(kRegHDMIOutControl);
    ULWord regIC(kRegHDMIInputControl);
    bool ret = true;

    if (!NTV2_IS_VALID_HDMI_BITDEPTH(value))
        return false;

    if (!GetHDMIOutControlReg(regOC, inWhichHDMIOut))
        return false;
    if (!GetHDMIInputControlReg(regIC, inWhichHDMIOut))
        return false;

    if (value == NTV2_HDMI12Bit)
    {
        ret &= WriteRegister (regOC, 0, kLHIRegMaskHDMIOutBitDepth, kLHIRegShiftHDMIOutBitDepth);
        ret &= WriteRegister (regOC, 2, kRegMaskHDMIVOBD, kRegShiftHDMIVOBD);
        ret &= WriteRegister (regIC, 1, kRegMaskHDMIOut12Bit, kRegShiftHDMIOut12Bit);
    }
    else if (value == NTV2_HDMI10Bit)
    {
        ret &= WriteRegister (regOC, 1, kLHIRegMaskHDMIOutBitDepth, kLHIRegShiftHDMIOutBitDepth);
        ret &= WriteRegister (regOC, 0, kRegMaskHDMIVOBD, kRegShiftHDMIVOBD);
        ret &= WriteRegister (regIC, 0, kRegMaskHDMIOut12Bit, kRegShiftHDMIOut12Bit);
    }
    else
    {
        ret &= WriteRegister (regOC, 0, kLHIRegMaskHDMIOutBitDepth, kLHIRegShiftHDMIOutBitDepth);
        ret &= WriteRegister (regOC, 0, kRegMaskHDMIVOBD, kRegShiftHDMIVOBD);
        ret &= WriteRegister (regIC, 0, kRegMaskHDMIOut12Bit, kRegShiftHDMIOut12Bit);
    }

    return ret;
}

bool CNTV2Card::GetHDMIOutBitDepth (NTV2HDMIBitDepth & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord regOC(kRegHDMIOutControl);
    ULWord regIC(kRegHDMIInputControl);
    ULWord d10(0), d12(0);

    outValue = NTV2_INVALID_HDMIBitDepth;

    if (!GetHDMIOutControlReg(regOC, inWhichHDMIOut))
        return false;
    if (!GetHDMIInputControlReg(regIC, inWhichHDMIOut))
        return false;

    if (!(ReadRegister(regOC, d10, kLHIRegMaskHDMIOutBitDepth, kLHIRegShiftHDMIOutBitDepth)
            &&	ReadRegister (regIC, d12,  kRegMaskHDMIOut12Bit, kRegShiftHDMIOut12Bit)))
        return false;

    if (d12 > 0)
        outValue = NTV2_HDMI12Bit;
    else if (d10 > 0)
        outValue = NTV2_HDMI10Bit;
    else
        outValue = NTV2_HDMI8Bit;
    return true;
}

bool CNTV2Card::SetHDMIOutProtocol (const NTV2HDMIProtocol value, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIOutControl);

    if (!NTV2_IS_VALID_HDMI_PROTOCOL(value))
        return false;

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister (reg, ULWord(value), kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
}

bool CNTV2Card::GetHDMIOutProtocol (NTV2HDMIProtocol & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIOutControl);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;
    
    return CNTV2DriverInterface::ReadRegister (reg, outValue, kLHIRegMaskHDMIOutDVI, kLHIRegShiftHDMIOutDVI);
}

bool CNTV2Card::SetHDMIOutForceConfig (const bool value, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIOutControl);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister (reg, ULWord(value), kRegMaskHDMIOutForceConfig, kRegShiftHDMIOutForceConfig);
}

bool CNTV2Card::GetHDMIOutForceConfig (bool & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIOutControl);

    if (!GetHDMIOutControlReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister (reg, outValue, kRegMaskHDMIOutForceConfig, kRegShiftHDMIOutForceConfig);
}

bool CNTV2Card::SetHDMIOutPrefer420 (const bool value, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputControl);

    if (!GetHDMIInputControlReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister (reg, ULWord(value), kRegMaskHDMIOutPrefer420, kRegShiftHDMIOutPrefer420);
}

bool CNTV2Card::GetHDMIOutPrefer420 (bool & outValue, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputControl);

    if (!GetHDMIInputControlReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister (reg, outValue, kRegMaskHDMIOutPrefer420, kRegShiftHDMIOutPrefer420);
}


bool CNTV2Card::SetHDMIOutDecimateMode (const bool inIsEnabled)
{
    return GetNumSupported(kDeviceGetHDMIVersion) > 1
            && GetNumSupported(kDeviceGetNumHDMIVideoOutputs) > 0
            && WriteRegister(kRegRasterizerControl, ULWord(inIsEnabled), kRegMaskRasterDecimate, kRegShiftRasterDecimate);
}

bool CNTV2Card::GetHDMIOutDecimateMode(bool & outIsEnabled)
{
    return GetNumSupported(kDeviceGetHDMIVersion) > 1
            && GetNumSupported(kDeviceGetNumHDMIVideoOutputs) > 0
            && CNTV2DriverInterface::ReadRegister (kRegRasterizerControl, outIsEnabled, kRegMaskRasterDecimate, kRegShiftRasterDecimate);
}

bool CNTV2Card::SetHDMIOutTsiIO (const bool inIsEnabled)
{
    return GetNumSupported(kDeviceGetHDMIVersion) > 1
            && GetNumSupported(kDeviceGetNumHDMIVideoOutputs) > 0
            && WriteRegister(kRegRasterizerControl, ULWord(inIsEnabled), kRegMaskTsiIO, kRegShiftTsiIO);
}

bool CNTV2Card::GetHDMIOutTsiIO (bool & outIsEnabled)
{
    return GetNumSupported(kDeviceGetHDMIVersion) > 1
            && GetNumSupported(kDeviceGetNumHDMIVideoOutputs) > 0
            && CNTV2DriverInterface::ReadRegister(kRegRasterizerControl, outIsEnabled, kRegMaskTsiIO, kRegShiftTsiIO);
}

bool CNTV2Card::SetHDMIOutLevelBMode (const bool inIsEnabled)
{
    return GetNumSupported(kDeviceGetHDMIVersion) > 1
            && GetNumSupported(kDeviceGetNumHDMIVideoOutputs) > 0
            && WriteRegister(kRegRasterizerControl, ULWord(inIsEnabled), kRegMaskRasterLevelB, kRegShiftRasterLevelB);
}

bool CNTV2Card::GetHDMIOutLevelBMode (bool & outIsEnabled)
{
    return GetNumSupported(kDeviceGetHDMIVersion) > 1
            && GetNumSupported(kDeviceGetNumHDMIVideoOutputs) > 0
            && CNTV2DriverInterface::ReadRegister (kRegRasterizerControl, outIsEnabled, kRegMaskRasterLevelB, kRegShiftRasterLevelB);
}

bool CNTV2Card::SetHDMIV2Mode (const NTV2HDMIV2Mode inMode)
{
    return GetNumSupported(kDeviceGetHDMIVersion) > 1
            && WriteRegister(kRegRasterizerControl, inMode, kRegMaskRasterMode, kRegShiftRasterMode);
}


bool CNTV2Card::GetHDMIV2Mode (NTV2HDMIV2Mode & outMode)
{
    return GetNumSupported(kDeviceGetHDMIVersion) > 1
            && CNTV2DriverInterface::ReadRegister(kRegRasterizerControl, outMode, kRegMaskRasterMode, kRegShiftRasterMode);
}


bool CNTV2Card::GetHDMIOutStatus (NTV2HDMIOutputStatus & outStatus, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kVRegHDMIOutStatus1);
    ULWord data(0);

    outStatus.Clear();
    if (GetNumSupported(kDeviceGetHDMIVersion) < 4)
        return false;	//	must have HDMI version 4 or higher

    if (!GetHDMIOutStatusReg(reg, inWhichHDMIOut))
        return false;

    if (!ReadRegister(reg, data))
        return false;	//	ReadRegister failed

    return outStatus.SetFromRegValue(data);
}


bool CNTV2Card::SetHDMIHDRGreenPrimaryX(const uint16_t inGreenPrimaryX, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRGreenPrimary);

    if(!NTV2_IS_VALID_HDR_PRIMARY(inGreenPrimaryX) ||  !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrGreenXCh1, uint32_t(inGreenPrimaryX))))
        return false;
    
    if (!GetHDMIOutHDRGreenPrimaryReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister(reg, uint32_t(inGreenPrimaryX), kRegMaskHDMIHDRGreenPrimaryX, kRegShiftHDMIHDRGreenPrimaryX);
}

bool CNTV2Card::GetHDMIHDRGreenPrimaryX(uint16_t & outGreenPrimaryX, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRGreenPrimary);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if (!GetHDMIOutHDRGreenPrimaryReg(reg, inWhichHDMIOut))
        return false;
    
    return CNTV2DriverInterface::ReadRegister(reg, outGreenPrimaryX, kRegMaskHDMIHDRGreenPrimaryX, kRegShiftHDMIHDRGreenPrimaryX);
}

bool CNTV2Card::SetHDMIHDRGreenPrimaryY(const uint16_t inGreenPrimaryY, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRGreenPrimary);

    if(!NTV2_IS_VALID_HDR_PRIMARY(inGreenPrimaryY) ||  !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrGreenYCh1, uint32_t(inGreenPrimaryY))))
        return false;
    
    if (!GetHDMIOutHDRGreenPrimaryReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, uint32_t(inGreenPrimaryY), kRegMaskHDMIHDRGreenPrimaryY, kRegShiftHDMIHDRGreenPrimaryY);
}

bool CNTV2Card::GetHDMIHDRGreenPrimaryY(uint16_t & outGreenPrimaryY, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRGreenPrimary);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if (!GetHDMIOutHDRGreenPrimaryReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outGreenPrimaryY, kRegMaskHDMIHDRGreenPrimaryY, kRegShiftHDMIHDRGreenPrimaryY);
}

bool CNTV2Card::SetHDMIHDRBluePrimaryX(const uint16_t inBluePrimaryX, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRBluePrimary);

    if(!NTV2_IS_VALID_HDR_PRIMARY(inBluePrimaryX) ||  !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrBlueXCh1, uint32_t(inBluePrimaryX))))
        return false;
    
    if (!GetHDMIOutHDRBluePrimaryReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, uint32_t(inBluePrimaryX), kRegMaskHDMIHDRBluePrimaryX, kRegShiftHDMIHDRBluePrimaryX);
}

bool CNTV2Card::GetHDMIHDRBluePrimaryX(uint16_t & outBluePrimaryX, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRBluePrimary);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRBluePrimaryReg(reg, inWhichHDMIOut))
        return false;
    
    return CNTV2DriverInterface::ReadRegister(reg, outBluePrimaryX, kRegMaskHDMIHDRBluePrimaryX, kRegShiftHDMIHDRBluePrimaryX);
}

bool CNTV2Card::SetHDMIHDRBluePrimaryY(const uint16_t inBluePrimaryY, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRBluePrimary);

    if(!NTV2_IS_VALID_HDR_PRIMARY(inBluePrimaryY) ||  !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrBlueYCh1, uint32_t(inBluePrimaryY))))
        return false;
    
    if (!GetHDMIOutHDRBluePrimaryReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, uint32_t(inBluePrimaryY), kRegMaskHDMIHDRBluePrimaryY, kRegShiftHDMIHDRBluePrimaryY);
}

bool CNTV2Card::GetHDMIHDRBluePrimaryY(uint16_t & outBluePrimaryY, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRBluePrimary);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRBluePrimaryReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outBluePrimaryY, kRegMaskHDMIHDRBluePrimaryY, kRegShiftHDMIHDRBluePrimaryY);
}

bool CNTV2Card::SetHDMIHDRRedPrimaryX(const uint16_t inRedPrimaryX, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRRedPrimary);

    if(!NTV2_IS_VALID_HDR_PRIMARY(inRedPrimaryX) ||	 !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrRedXCh1, uint32_t(inRedPrimaryX))))
        
    if (!GetHDMIOutHDRRedPrimaryReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, uint32_t(inRedPrimaryX), kRegMaskHDMIHDRRedPrimaryX, kRegShiftHDMIHDRRedPrimaryX);
}

bool CNTV2Card::GetHDMIHDRRedPrimaryX(uint16_t & outRedPrimaryX, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRRedPrimary);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRRedPrimaryReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outRedPrimaryX, kRegMaskHDMIHDRRedPrimaryX, kRegShiftHDMIHDRRedPrimaryX);
}

bool CNTV2Card::SetHDMIHDRRedPrimaryY(const uint16_t inRedPrimaryY, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRRedPrimary);

    if(!NTV2_IS_VALID_HDR_PRIMARY(inRedPrimaryY) ||	 !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrRedYCh1, uint32_t(inRedPrimaryY))))
        return false;
    
    if (!GetHDMIOutHDRRedPrimaryReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, uint32_t(inRedPrimaryY), kRegMaskHDMIHDRRedPrimaryY, kRegShiftHDMIHDRRedPrimaryY);
}

bool CNTV2Card::GetHDMIHDRRedPrimaryY(uint16_t & outRedPrimaryY, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRRedPrimary);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRRedPrimaryReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outRedPrimaryY, kRegMaskHDMIHDRRedPrimaryY, kRegShiftHDMIHDRRedPrimaryY);
}

bool CNTV2Card::SetHDMIHDRWhitePointX(const uint16_t inWhitePointX, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRWhitePoint);

    if(!NTV2_IS_VALID_HDR_PRIMARY(inWhitePointX) ||	 !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrWhiteXCh1, uint32_t(inWhitePointX))))

    if (!GetHDMIOutHDRWhitePointReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister(reg, uint32_t(inWhitePointX), kRegMaskHDMIHDRWhitePointX, kRegShiftHDMIHDRWhitePointX);
}

bool CNTV2Card::GetHDMIHDRWhitePointX(uint16_t & outWhitePointX, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRWhitePoint);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRWhitePointReg(reg, inWhichHDMIOut))
        return false;
    
    return CNTV2DriverInterface::ReadRegister(reg, outWhitePointX, kRegMaskHDMIHDRWhitePointX, kRegShiftHDMIHDRWhitePointX);
}

bool CNTV2Card::SetHDMIHDRWhitePointY(const uint16_t inWhitePointY, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRWhitePoint);

    if(!NTV2_IS_VALID_HDR_PRIMARY(inWhitePointY) ||	 !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrWhiteYCh1, uint32_t(inWhitePointY))))
        return false;

    if (!GetHDMIOutHDRWhitePointReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister(reg, uint32_t(inWhitePointY), kRegMaskHDMIHDRWhitePointY, kRegShiftHDMIHDRWhitePointY);
}

bool CNTV2Card::GetHDMIHDRWhitePointY(uint16_t & outWhitePointY, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRWhitePoint);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRWhitePointReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outWhitePointY, kRegMaskHDMIHDRWhitePointY, kRegShiftHDMIHDRWhitePointY);
}

bool CNTV2Card::SetHDMIHDRMaxMasteringLuminance(const uint16_t inMaxMasteringLuminance, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRMasteringLuminence);

    if(!NTV2_IS_VALID_HDR_MASTERING_LUMINENCE(inMaxMasteringLuminance) ||  !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrMasterLumMaxCh1, uint32_t(inMaxMasteringLuminance))))
        return false;
            
    if (!GetHDMIOutHDRMasterLuminanceReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister(reg, uint32_t(inMaxMasteringLuminance), kRegMaskHDMIHDRMaxMasteringLuminance, kRegShiftHDMIHDRMaxMasteringLuminance);
}

bool CNTV2Card::GetHDMIHDRMaxMasteringLuminance(uint16_t & outMaxMasteringLuminance, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRMasteringLuminence);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if (!GetHDMIOutHDRMasterLuminanceReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outMaxMasteringLuminance, kRegMaskHDMIHDRMaxMasteringLuminance, kRegShiftHDMIHDRMaxMasteringLuminance);
}

bool CNTV2Card::SetHDMIHDRMinMasteringLuminance(const uint16_t inMinMasteringLuminance, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRMasteringLuminence);

    if(!NTV2_IS_VALID_HDR_MASTERING_LUMINENCE(inMinMasteringLuminance) ||  !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrMasterLumMinCh1, uint32_t(inMinMasteringLuminance))))
        
    if (!GetHDMIOutHDRMasterLuminanceReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister(reg, uint32_t(inMinMasteringLuminance), kRegMaskHDMIHDRMinMasteringLuminance, kRegShiftHDMIHDRMinMasteringLuminance);
}

bool CNTV2Card::GetHDMIHDRMinMasteringLuminance(uint16_t & outMinMasteringLuminance, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRMasteringLuminence);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if (!GetHDMIOutHDRMasterLuminanceReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outMinMasteringLuminance, kRegMaskHDMIHDRMinMasteringLuminance, kRegShiftHDMIHDRMinMasteringLuminance);
}

bool CNTV2Card::SetHDMIHDRMaxContentLightLevel(const uint16_t inMaxContentLightLevel, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRLightLevel);

    if(!NTV2_IS_VALID_HDR_LIGHT_LEVEL(inMaxContentLightLevel) ||  !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrMaxCLLCh1, uint32_t(inMaxContentLightLevel))))
        return false;
    
    if (!GetHDMIOutHDRLightLevelReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, uint32_t(inMaxContentLightLevel), kRegMaskHDMIHDRMaxContentLightLevel, kRegShiftHDMIHDRMaxContentLightLevel);
}

bool CNTV2Card::GetHDMIHDRMaxContentLightLevel(uint16_t & outMaxContentLightLevel, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRLightLevel);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if (!GetHDMIOutHDRLightLevelReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outMaxContentLightLevel, kRegMaskHDMIHDRMaxContentLightLevel, kRegShiftHDMIHDRMaxContentLightLevel);
}

bool CNTV2Card::SetHDMIHDRMaxFrameAverageLightLevel(const uint16_t inMaxFrameAverageLightLevel, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRLightLevel);

    if(!NTV2_IS_VALID_HDR_LIGHT_LEVEL(inMaxFrameAverageLightLevel) ||  !IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrMaxFALLCh1, uint32_t(inMaxFrameAverageLightLevel))))
        return false;
        
    if (!GetHDMIOutHDRLightLevelReg(reg, inWhichHDMIOut))
        return false;
        
    return WriteRegister(reg, uint32_t(inMaxFrameAverageLightLevel), kRegMaskHDMIHDRMaxFrameAverageLightLevel, kRegShiftHDMIHDRMaxFrameAverageLightLevel);
}

bool CNTV2Card::GetHDMIHDRMaxFrameAverageLightLevel(uint16_t & outMaxFrameAverageLightLevel, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRLightLevel);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if (!GetHDMIOutHDRLightLevelReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outMaxFrameAverageLightLevel, kRegMaskHDMIHDRMaxFrameAverageLightLevel, kRegShiftHDMIHDRMaxFrameAverageLightLevel);
}

bool CNTV2Card::SetHDMIHDRConstantLuminance(const bool inEnableConstantLuminance, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrLuminanceCh1, inEnableConstantLuminance ? 1 : 0)))
        return false;
    
    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister(reg, inEnableConstantLuminance ? 1 : 0, kRegMaskHDMIHDRNonContantLuminance, kRegShiftHDMIHDRNonContantLuminance);
}

bool CNTV2Card::GetHDMIHDRConstantLuminance(const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;

    uint32_t regValue = 0;
    ReadRegister(reg, regValue, kRegMaskHDMIHDRNonContantLuminance, kRegShiftHDMIHDRNonContantLuminance);
    return regValue ? true : false;
}

bool CNTV2Card::SetHDMIHDRElectroOpticalTransferFunction(const uint8_t inEOTFByte, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrTransferCh1, inEOTFByte)))
        return false;
    
    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister(reg, inEOTFByte, kRegMaskElectroOpticalTransferFunction, kRegShiftElectroOpticalTransferFunction);
}

bool CNTV2Card::GetHDMIHDRElectroOpticalTransferFunction(uint8_t & outEOTFByte, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outEOTFByte, kRegMaskElectroOpticalTransferFunction, kRegShiftElectroOpticalTransferFunction);
}

bool CNTV2Card::SetHDMIHDRStaticMetadataDescriptorID(const uint8_t inSMDId, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if ((inWhichHDMIOut == NTV2_CHANNEL1) &&
        (!WriteRegister(kVRegHdrColorimetryCh1, uint32_t(inSMDId))))
        return false;
    
    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;
    
    return WriteRegister(reg, uint32_t(inSMDId), kRegMaskHDRStaticMetadataDescriptorID, kRegShiftHDRStaticMetadataDescriptorID);
}

bool CNTV2Card::GetHDMIHDRStaticMetadataDescriptorID(uint8_t & outSMDId, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outSMDId, kRegMaskHDRStaticMetadataDescriptorID, kRegShiftHDRStaticMetadataDescriptorID);
}

bool CNTV2Card::EnableHDMIHDR(const bool inEnableHDMIHDR, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);
    bool status = true;

    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;

    status = WriteRegister(reg, inEnableHDMIHDR ? 1 : 0, kRegMaskHDMIHDREnable, kRegShiftHDMIHDREnable);
    if (GetNumSupported(kDeviceGetHDMIVersion) < 6)
        WaitForOutputFieldID(NTV2_FIELD0, NTV2_CHANNEL1);
    return status;
}

bool CNTV2Card::GetHDMIHDREnabled (const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;

    uint32_t regValue = 0;
    ReadRegister(reg, regValue, kRegMaskHDMIHDREnable, kRegShiftHDMIHDREnable);
    return regValue ? true : false;
}

bool CNTV2Card::EnableHDMIHDRDolbyVision(const bool inEnable, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);
    bool status = true;
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;

    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;

    status = WriteRegister(reg, inEnable ? 1 : 0, kRegMaskHDMIHDRDolbyVisionEnable, kRegShiftHDMIHDRDolbyVisionEnable);
    if (GetNumSupported(kDeviceGetHDMIVersion) < 6)
        WaitForOutputFieldID(NTV2_FIELD0, NTV2_CHANNEL1);
    return status;
}

bool CNTV2Card::GetHDMIHDRDolbyVisionEnabled (const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIHDRControl);
    
    if (!IsSupported(kDeviceCanDoHDMIHDROut))
        return false;
    
    if (!GetHDMIOutHDRControlReg(reg, inWhichHDMIOut))
        return false;

    uint32_t regValue = 0;
    ReadRegister(reg, regValue, kRegMaskHDMIHDRDolbyVisionEnable, kRegShiftHDMIHDRDolbyVisionEnable);
    return regValue ? true : false;
}

bool CNTV2Card::SetHDRData (const HDRFloatValues & inFloatValues, const NTV2Channel inWhichHDMIOut)
{
    HDRRegValues registerValues;
    convertHDRFloatToRegisterValues(inFloatValues, registerValues);
    SetHDRData(registerValues, inWhichHDMIOut);
    return true;
}

bool CNTV2Card::SetHDRData(const HDRRegValues & inRegisterValues, const NTV2Channel inWhichHDMIOut)
{
    SetHDMIHDRGreenPrimaryX(inRegisterValues.greenPrimaryX, inWhichHDMIOut);
    SetHDMIHDRGreenPrimaryY(inRegisterValues.greenPrimaryY, inWhichHDMIOut);
    SetHDMIHDRBluePrimaryX(inRegisterValues.bluePrimaryX, inWhichHDMIOut);
    SetHDMIHDRBluePrimaryY(inRegisterValues.bluePrimaryY, inWhichHDMIOut);
    SetHDMIHDRRedPrimaryX(inRegisterValues.redPrimaryX, inWhichHDMIOut);
    SetHDMIHDRRedPrimaryY(inRegisterValues.redPrimaryY, inWhichHDMIOut);
    SetHDMIHDRWhitePointX(inRegisterValues.whitePointX, inWhichHDMIOut);
    SetHDMIHDRWhitePointY(inRegisterValues.whitePointY, inWhichHDMIOut);
    SetHDMIHDRMaxMasteringLuminance(inRegisterValues.maxMasteringLuminance, inWhichHDMIOut);
    SetHDMIHDRMinMasteringLuminance(inRegisterValues.minMasteringLuminance, inWhichHDMIOut);
    SetHDMIHDRMaxContentLightLevel(inRegisterValues.maxContentLightLevel, inWhichHDMIOut);
    SetHDMIHDRMaxFrameAverageLightLevel(inRegisterValues.maxFrameAverageLightLevel, inWhichHDMIOut);
    SetHDMIHDRElectroOpticalTransferFunction(inRegisterValues.electroOpticalTransferFunction, inWhichHDMIOut);
    SetHDMIHDRStaticMetadataDescriptorID(inRegisterValues.staticMetadataDescriptorID, inWhichHDMIOut);
    return true;
}

bool CNTV2Card::GetHDRData(HDRFloatValues & outFloatValues, const NTV2Channel inWhichHDMIOut)
{
    HDRRegValues registerValues;
    GetHDRData(registerValues, inWhichHDMIOut);
    return convertHDRRegisterToFloatValues(registerValues, outFloatValues);
}

bool CNTV2Card::GetHDRData(HDRRegValues & outRegisterValues, const NTV2Channel inWhichHDMIOut)
{
    GetHDMIHDRGreenPrimaryX(outRegisterValues.greenPrimaryX, inWhichHDMIOut);
    GetHDMIHDRGreenPrimaryY(outRegisterValues.greenPrimaryY, inWhichHDMIOut);
    GetHDMIHDRBluePrimaryX(outRegisterValues.bluePrimaryX, inWhichHDMIOut);
    GetHDMIHDRBluePrimaryY(outRegisterValues.bluePrimaryY, inWhichHDMIOut);
    GetHDMIHDRRedPrimaryX(outRegisterValues.redPrimaryX, inWhichHDMIOut);
    GetHDMIHDRRedPrimaryY(outRegisterValues.redPrimaryY, inWhichHDMIOut);
    GetHDMIHDRWhitePointX(outRegisterValues.whitePointX, inWhichHDMIOut);
    GetHDMIHDRWhitePointY(outRegisterValues.whitePointY, inWhichHDMIOut);
    GetHDMIHDRMaxMasteringLuminance(outRegisterValues.maxMasteringLuminance, inWhichHDMIOut);
    GetHDMIHDRMinMasteringLuminance(outRegisterValues.minMasteringLuminance, inWhichHDMIOut);
    GetHDMIHDRMaxContentLightLevel(outRegisterValues.maxContentLightLevel, inWhichHDMIOut);
    GetHDMIHDRMaxFrameAverageLightLevel(outRegisterValues.maxFrameAverageLightLevel, inWhichHDMIOut);
    GetHDMIHDRElectroOpticalTransferFunction(outRegisterValues.electroOpticalTransferFunction, inWhichHDMIOut);
    GetHDMIHDRStaticMetadataDescriptorID(outRegisterValues.staticMetadataDescriptorID, inWhichHDMIOut);
    return true;
}

bool CNTV2Card::SetHDMIHDRBT2020(const NTV2Channel inWhichHDMIOut)
{
    HDRRegValues registerValues;
    setHDRDefaultsForBT2020(registerValues);
    EnableHDMIHDR(false, inWhichHDMIOut);
    SetHDRData(registerValues, inWhichHDMIOut);
    EnableHDMIHDR(true, inWhichHDMIOut);
    return true;
}

bool CNTV2Card::SetHDMIHDRDCIP3(const NTV2Channel inWhichHDMIOut)
{
    HDRRegValues registerValues;
    setHDRDefaultsForDCIP3(registerValues);
    EnableHDMIHDR(false, inWhichHDMIOut);
    SetHDRData(registerValues, inWhichHDMIOut);
    EnableHDMIHDR(true, inWhichHDMIOut);
    return true;
}

bool CNTV2Card::GetHDMIOutAudioChannel34Swap (bool & outIsSwapped, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputControl);
    
    outIsSwapped = false;
    
    if (!GetNumSupported(kDeviceGetNumHDMIVideoOutputs))
        return false;

    if (!GetHDMIInputControlReg(reg, inWhichHDMIOut))
        return false;

    return CNTV2DriverInterface::ReadRegister(reg, outIsSwapped, kRegMaskHDMISwapOutputAudCh34, kRegShiftHDMISwapOutputAudCh34);
}

bool CNTV2Card::SetHDMIOutAudioChannel34Swap (const bool inIsSwapped, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputControl);

    if (!GetNumSupported(kDeviceGetNumHDMIVideoOutputs))
        return false;
    
    if (!GetHDMIInputControlReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, inIsSwapped ? 1 : 0, kRegMaskHDMISwapOutputAudCh34, kRegShiftHDMISwapOutputAudCh34);
}

bool CNTV2Card::EnableHDMIOutUserOverride(bool enable, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputControl);

    if (!GetNumSupported(kDeviceGetNumHDMIVideoOutputs))
        return false;
    
    if (!GetHDMIInputControlReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, enable ? 1 : 0, kRegMaskHDMIOutForceConfig, kRegShiftHDMIOutForceConfig);
}

bool CNTV2Card::GetEnableHDMIOutUserOverride(bool & isEnabled, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputControl);

    if (!GetNumSupported(kDeviceGetNumHDMIVideoOutputs))
        return false;
    
    if (!GetHDMIInputControlReg(reg, inWhichHDMIOut))
        return false;

    ULWord enable = 0;
    bool status = ReadRegister(reg, enable, kRegMaskHDMIOutForceConfig, kRegShiftHDMIOutForceConfig);
    isEnabled = enable ? true : false;
    return status;
}

bool CNTV2Card::EnableHDMIOutCenterCrop(bool enable, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputControl);

    if (!GetNumSupported(kDeviceGetNumHDMIVideoOutputs))
        return false;
    
    if (!GetHDMIInputControlReg(reg, inWhichHDMIOut))
        return false;

    return WriteRegister(reg, enable ? 1 : 0, kRegMaskHDMIOutCropEnable, kRegShiftHDMIOutCropEnable);
}

bool CNTV2Card::GetEnableHDMIOutCenterCrop(bool & isEnabled, const NTV2Channel inWhichHDMIOut)
{
    ULWord reg(kRegHDMIInputControl);

    if (!GetNumSupported(kDeviceGetNumHDMIVideoOutputs))
        return false;
    
    if (!GetHDMIInputControlReg(reg, inWhichHDMIOut))
        return false;

    ULWord enable = 0;
    bool status = ReadRegister(reg, enable, kRegMaskHDMIOutCropEnable, kRegShiftHDMIOutCropEnable);
    isEnabled = enable ? true : false;
    return status;
}

bool CNTV2Card::GetHDMIOutControlReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutControlVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIOutControl;
    }
    return true;
}

bool CNTV2Card::GetHDMIInputStatusReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToInputStatusVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIInputStatus;
    }
    return true;
}

bool CNTV2Card::GetHDMIInputControlReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToInputControlVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIInputControl;
    }
    return true;
}

bool CNTV2Card::GetHDMIOutStatusReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutStatusVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kVRegHDMIOutStatus1;
    }
    return true;
}

bool CNTV2Card::GetHDMIOutHDRGreenPrimaryReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutHDRGreenPrimaryVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIHDRGreenPrimary;
    }
    return true;
}

bool CNTV2Card::GetHDMIOutHDRBluePrimaryReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutHDRBluePrimaryVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIHDRBluePrimary;
    }
    return true;
}

bool CNTV2Card::GetHDMIOutHDRRedPrimaryReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutHDRRedPrimaryVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIHDRRedPrimary;
    }
    return true;
}

bool CNTV2Card::GetHDMIOutHDRWhitePointReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutHDRWhitePointVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIHDRWhitePoint;
    }
    return true;
}

bool CNTV2Card::GetHDMIOutHDRMasterLuminanceReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutHDRMasterLuminanceVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIHDRMasteringLuminence;
    }
    return true;
}

bool CNTV2Card::GetHDMIOutHDRLightLevelReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutHDRLightLevelVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIHDRLightLevel;
    }
    return true;
}

bool CNTV2Card::GetHDMIOutHDRControlReg(ULWord& reg, const NTV2Channel inWhichHDMIOut)
{
    const ULWord	hdmiVers	(::NTV2DeviceGetHDMIVersion(GetDeviceID()));
    const ULWord	hdmiNum 	(::NTV2DeviceGetNumHDMIVideoOutputs(GetDeviceID()));

    if (hdmiVers >= 6)
    {
        if ((ULWord)inWhichHDMIOut >= hdmiNum)
            return false;
        reg = gHDMIChannelToOutHDRControlVRegNum[(ULWord)inWhichHDMIOut];
    }
    else
    {
        if (hdmiVers == 0)
            return false;
        reg = kRegHDMIHDRControl;
    }
    return true;
}

//////////////////////////////////////////////////////////////

#ifdef MSWindows
#pragma warning(default: 4800)
#endif
