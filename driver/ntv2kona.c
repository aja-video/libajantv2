/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2kona.c
//
//==========================================================================

#include "ntv2kona.h"

static const uint32_t	gChannelToGlobalControlRegNum []	= {	kRegGlobalControl, kRegGlobalControlCh2, kRegGlobalControlCh3, kRegGlobalControlCh4,
																kRegGlobalControlCh5, kRegGlobalControlCh6, kRegGlobalControlCh7, kRegGlobalControlCh8, 0};
static const uint32_t	gChannelToSmpte372RegisterNum[]		= { kRegGlobalControl, kRegGlobalControl, kRegGlobalControl2, kRegGlobalControl2,
																kRegGlobalControl2, kRegGlobalControl2, kRegGlobalControl2, kRegGlobalControl2, 0 };
static const uint32_t	gChannelToSmpte372Masks[]			= { kRegMaskSmpte372Enable, kRegMaskSmpte372Enable, kRegMaskSmpte372Enable4, kRegMaskSmpte372Enable4,
																kRegMaskSmpte372Enable6, kRegMaskSmpte372Enable6, kRegMaskSmpte372Enable8, kRegMaskSmpte372Enable8, 0 };
static const uint32_t	gChannelToSmpte372Shifts[]			= { kRegShiftSmpte372, kRegShiftSmpte372, kRegShiftSmpte372Enable4, kRegShiftSmpte372Enable4,
																kRegShiftSmpte372Enable6, kRegShiftSmpte372Enable6, kRegShiftSmpte372Enable8, kRegShiftSmpte372Enable8, 0 };
static const uint32_t	gChannelToControlRegNum []			= {	kRegCh1Control, kRegCh2Control, kRegCh3Control, kRegCh4Control, kRegCh5Control, kRegCh6Control,
																kRegCh7Control, kRegCh8Control, 0};
static const uint32_t	gChannelToOutputFrameRegNum []		= {	kRegCh1OutputFrame, kRegCh2OutputFrame, kRegCh3OutputFrame, kRegCh4OutputFrame,
																kRegCh5OutputFrame, kRegCh6OutputFrame, kRegCh7OutputFrame, kRegCh8OutputFrame, 0};
static const uint32_t	gChannelToInputFrameRegNum []		= {	kRegCh1InputFrame, kRegCh2InputFrame, kRegCh3InputFrame, kRegCh4InputFrame,
																kRegCh5InputFrame, kRegCh6InputFrame, kRegCh7InputFrame, kRegCh8InputFrame, 0};
static const uint32_t	gChannelToPCIAccessFrameRegNum []	= {	kRegCh1PCIAccessFrame, kRegCh2PCIAccessFrame, kRegCh3PCIAccessFrame, kRegCh4PCIAccessFrame,
																kRegCh5PCIAccessFrame, kRegCh6PCIAccessFrame, kRegCh7PCIAccessFrame, kRegCh8PCIAccessFrame, 0};
static const uint32_t	gChannelToSDIOutControlRegNum []	= {	kRegSDIOut1Control, kRegSDIOut2Control, kRegSDIOut3Control, kRegSDIOut4Control,
																kRegSDIOut5Control, kRegSDIOut6Control, kRegSDIOut7Control, kRegSDIOut8Control, 0};
static const ULWord		gChannelToSDIInput3GStatusRegNum []	= {	kRegSDIInput3GStatus,		kRegSDIInput3GStatus,		kRegSDIInput3GStatus2,		kRegSDIInput3GStatus2,
																kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	0};
static const ULWord		gChannelToLevelBMasks[]				= { kRegMaskSDIIn3GbpsSMPTELevelBMode, kRegMaskSDIIn23GbpsSMPTELevelBMode, kRegMaskSDIIn3GbpsSMPTELevelBMode, kRegMaskSDIIn23GbpsSMPTELevelBMode,
																kRegMaskSDIIn53GbpsSMPTELevelBMode, kRegMaskSDIIn63GbpsSMPTELevelBMode, kRegMaskSDIIn73GbpsSMPTELevelBMode, kRegMaskSDIIn83GbpsSMPTELevelBMode };
static const ULWord		gChannelToLevelBShifts[]			= { kRegShiftSDIIn3GbpsSMPTELevelBMode, kRegShiftSDIIn23GbpsSMPTELevelBMode, kRegShiftSDIIn3GbpsSMPTELevelBMode, kRegShiftSDIIn23GbpsSMPTELevelBMode,
																kRegShiftSDIIn53GbpsSMPTELevelBMode, kRegShiftSDIIn63GbpsSMPTELevelBMode, kRegShiftSDIIn73GbpsSMPTELevelBMode, kRegShiftSDIIn83GbpsSMPTELevelBMode };
static const ULWord		gChannelToInputLevelConversionMasks[]	= { kRegMaskSDIIn1LevelBtoLevelA, kRegMaskSDIIn2LevelBtoLevelA, kRegMaskSDIIn3LevelBtoLevelA, kRegMaskSDIIn4LevelBtoLevelA,
																	kRegMaskSDIIn5LevelBtoLevelA, kRegMaskSDIIn6LevelBtoLevelA, kRegMaskSDIIn7LevelBtoLevelA, kRegMaskSDIIn8LevelBtoLevelA };
static const ULWord		gChannelToInputLevelConversionShifts[]	= { kRegShiftSDIIn1LevelBtoLevelA, kRegShiftSDIIn2LevelBtoLevelA, kRegShiftSDIIn3LevelBtoLevelA, kRegShiftSDIIn4LevelBtoLevelA,
																	kRegShiftSDIIn5LevelBtoLevelA, kRegShiftSDIIn6LevelBtoLevelA, kRegShiftSDIIn7LevelBtoLevelA, kRegShiftSDIIn8LevelBtoLevelA };
static const ULWord		gChannelToInputStatesRegs[]			= { kRegInputStatus, kRegInputStatus, kRegInputStatus2, kRegInputStatus2,
																kRegInput56Status, kRegInput56Status, kRegInput78Status, kRegInput78Status };
static const ULWord		gChannelToInputStatusRateMasks[]	= { kRegMaskInput1FrameRate, kRegMaskInput2FrameRate, kRegMaskInput1FrameRate, kRegMaskInput2FrameRate,
																kRegMaskInput1FrameRate, kRegMaskInput2FrameRate, kRegMaskInput1FrameRate, kRegMaskInput2FrameRate };
static const ULWord		gChannelToInputStatusRateShifts[]	= { kRegShiftInput1FrameRate, kRegShiftInput2FrameRate, kRegShiftInput1FrameRate, kRegShiftInput2FrameRate,
																kRegShiftInput1FrameRate, kRegShiftInput2FrameRate, kRegShiftInput1FrameRate, kRegShiftInput2FrameRate };

static const ULWord		gChannelToInputStatusRateHighMasks[]	= { kRegMaskInput1FrameRateHigh, kRegMaskInput2FrameRateHigh, kRegMaskInput1FrameRateHigh, kRegMaskInput2FrameRateHigh,
																	kRegMaskInput1FrameRateHigh, kRegMaskInput2FrameRateHigh, kRegMaskInput1FrameRateHigh, kRegMaskInput2FrameRateHigh };
static const ULWord		gChannelToInputStatusRateHighShifts[]	= { kRegShiftInput1FrameRateHigh, kRegShiftInput2FrameRateHigh, kRegShiftInput1FrameRateHigh, kRegShiftInput2FrameRateHigh,
																	kRegShiftInput1FrameRateHigh, kRegShiftInput2FrameRateHigh, kRegShiftInput1FrameRateHigh, kRegShiftInput2FrameRateHigh };

static const ULWord		gChannelToScanMasks[]				= { kRegMaskInput1Geometry, kRegMaskInput2Geometry, kRegMaskInput1Geometry, kRegMaskInput2Geometry,
																kRegMaskInput1Geometry, kRegMaskInput2Geometry, kRegMaskInput1Geometry, kRegMaskInput2Geometry };
static const ULWord		gChannelToScanShifts[]				= { kRegShiftInput1Geometry, kRegShiftInput2Geometry, kRegShiftInput1Geometry, kRegShiftInput2Geometry,
																kRegShiftInput1Geometry, kRegShiftInput2Geometry, kRegShiftInput1Geometry, kRegShiftInput2Geometry };

static const ULWord		gChannelToScanHighMasks[]			= {	kRegMaskInput1GeometryHigh, (ULWord)kRegMaskInput2GeometryHigh, kRegMaskInput1GeometryHigh, (ULWord)kRegMaskInput2GeometryHigh,
																kRegMaskInput1GeometryHigh, (ULWord)kRegMaskInput2GeometryHigh, kRegMaskInput1GeometryHigh, (ULWord)kRegMaskInput2GeometryHigh };
static const ULWord		gChannelToScanHighShifts[]			= { kRegShiftInput1GeometryHigh, kRegShiftInput2GeometryHigh, kRegShiftInput1GeometryHigh, kRegShiftInput2GeometryHigh,
																kRegShiftInput1GeometryHigh, kRegShiftInput2GeometryHigh, kRegShiftInput1GeometryHigh, kRegShiftInput2GeometryHigh };

static const ULWord		gChannelToProgressiveMasks[]		= { kRegMaskInput1Progressive, kRegMaskInput2Progressive, kRegMaskInput1Progressive, kRegMaskInput2Progressive,
																kRegMaskInput1Progressive, kRegMaskInput2Progressive, kRegMaskInput1Progressive, kRegMaskInput2Progressive };
static const ULWord		gChannelToProgressiveShifts[]		= { kRegShiftInput1Progressive, kRegShiftInput2Progressive, kRegShiftInput1Progressive, kRegShiftInput2Progressive,
																kRegShiftInput1Progressive, kRegShiftInput2Progressive, kRegShiftInput1Progressive, kRegShiftInput2Progressive };
static const VirtualRegisterNum		gShadowRegs[]			= { kVRegVideoFormatCh1, kVRegVideoFormatCh2, kVRegVideoFormatCh3, kVRegVideoFormatCh4,
																kVRegVideoFormatCh5, kVRegVideoFormatCh6, kVRegVideoFormatCh7, kVRegVideoFormatCh8 };

static const ULWord		gChannelToSDIIn6GModeMask[]			= {	kRegMaskSDIIn16GbpsMode, kRegMaskSDIIn26GbpsMode, kRegMaskSDIIn36GbpsMode, kRegMaskSDIIn46GbpsMode,
																kRegMaskSDIIn56GbpsMode, kRegMaskSDIIn66GbpsMode, kRegMaskSDIIn76GbpsMode, kRegMaskSDIIn86GbpsMode };

static const ULWord		gChannelToSDIIn6GModeShift[]		= {	kRegShiftSDIIn16GbpsMode, kRegShiftSDIIn26GbpsMode, kRegShiftSDIIn36GbpsMode, kRegShiftSDIIn46GbpsMode,
																kRegShiftSDIIn56GbpsMode, kRegShiftSDIIn66GbpsMode, kRegShiftSDIIn76GbpsMode, kRegShiftSDIIn86GbpsMode };

static const ULWord		gChannelToSDIIn12GModeMask[]		= {	kRegMaskSDIIn112GbpsMode, kRegMaskSDIIn212GbpsMode , kRegMaskSDIIn312GbpsMode, kRegMaskSDIIn412GbpsMode,
																kRegMaskSDIIn512GbpsMode, kRegMaskSDIIn612GbpsMode, kRegMaskSDIIn712GbpsMode, (ULWord)kRegMaskSDIIn812GbpsMode };

static const ULWord		gChannelToSDIIn12GModeShift[]		= {	kRegShiftSDIIn112GbpsMode, kRegShiftSDIIn212GbpsMode, kRegShiftSDIIn312GbpsMode, kRegShiftSDIIn412GbpsMode,
																kRegShiftSDIIn512GbpsMode, kRegShiftSDIIn612GbpsMode, kRegShiftSDIIn712GbpsMode, kRegShiftSDIIn812GbpsMode };

static const ULWord		gChannelToMRRegNum[]				= {	kRegMRQ1Control, kRegMRQ2Control, kRegMRQ3Control, kRegMRQ4Control};

////////////////////////
//interrupt routines
bool UpdateAudioMixerGainFromRotaryEncoder(Ntv2SystemContext* context)
{
	static uint8_t oldValue = 0;
	int32_t diffVolume = 0;
	uint32_t currentValue = 0;
	//uint32_t gainOverride = ntv2ReadVirtualRegister(context, kVRegRotaryGainOverrideEnable);
	uint32_t currentAuxGain = 0;
	//uint32_t currentMixGain = 0;
	uint8_t diffNew = 0;
	uint8_t diffOld = 0;

	ntv2ReadRegisterMS(context, kRegRotaryEncoder, &currentValue, kRegMaskRotaryEncoderValue, kRegShiftRotaryEncoderValue);
	ntv2ReadRegisterMS(context, kRegRotaryEncoder, &currentAuxGain, kRegMaskRotaryEncoderGain, kRegShiftRotaryEncoderGain);
	//ntv2ReadRegisterMS(context, 0x940, &currentMixGain, gMaskAuxGainValue, gShiftAuxGainValue);
	diffNew = (uint8_t)(currentValue - oldValue);
	diffOld = (uint8_t)(oldValue - currentValue);
	
	if (diffNew < diffOld)
		diffVolume  = (currentValue > oldValue)? (int32_t)diffNew : -(int32_t)diffNew;
	else
		diffVolume = (oldValue > currentValue)? -(int32_t)diffOld : (int32_t)diffOld;
	oldValue = (uint8_t)currentValue;
	
	if(diffVolume > 0)
	{
		ntv2Message("Up diff value: %d gain: %d\n", diffVolume, currentAuxGain);
		while(diffVolume > 0)
		{
			if(currentAuxGain > 0x0)
				currentAuxGain--;
			diffVolume--;
			ntv2Message("Up gain: %d\n", currentAuxGain);
			ntv2WriteRegisterMS(context, kRegRotaryEncoder, currentAuxGain, kRegMaskRotaryEncoderGain, kRegShiftRotaryEncoderGain);
		}
	}	
	else if(diffVolume < 0)
	{
		ntv2Message("Down diff value: %d gain: %d\n", diffVolume, currentAuxGain);
		while(diffVolume < 0)
		{
			if(currentAuxGain < 0x28)
				currentAuxGain++;
			diffVolume++;
			ntv2Message("Down gain: %d\n", currentAuxGain);
			ntv2WriteRegisterMS(context, kRegRotaryEncoder, currentAuxGain, kRegMaskRotaryEncoderGain, kRegShiftRotaryEncoderGain);
		}
	}
	else
	{
		//NoOp
	}
	return true;
}

///////////////////////
//board format routines
NTV2VideoFormat GetBoardVideoFormat(Ntv2SystemContext* context, NTV2Channel channel)
{

	NTV2Standard standard = GetStandard(context, channel);
	NTV2FrameRate frameRate = GetFrameRate(context, channel);
	ULWord is2Kx1080 = NTV2_IS_2K_1080_FRAME_GEOMETRY(GetFrameGeometry(context, channel));
	ULWord smpte372Enabled = GetSmpte372(context, channel)?1:0;

	return GetVideoFormatFromState(standard, frameRate, is2Kx1080, smpte372Enabled);
}

NTV2Standard GetStandard(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t regValue = 0;

	if (!IsMultiFormatActive(context))
		channel = NTV2_CHANNEL1;

	ntv2ReadRegisterMS(context, gChannelToGlobalControlRegNum[channel], &regValue, kRegMaskStandard, kRegShiftStandard);
	return (NTV2Standard)regValue;
}

NTV2FrameGeometry GetFrameGeometry(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t regValue = 0;

	if (!IsMultiFormatActive(context))
		channel = NTV2_CHANNEL1;

	ntv2ReadRegisterMS(context, gChannelToGlobalControlRegNum[channel], &regValue, kRegMaskGeometry, kRegShiftGeometry);
	return (NTV2FrameGeometry)regValue;
}

NTV2FrameRate GetFrameRate(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2FrameRate value = NTV2_FRAMERATE_UNKNOWN;
	uint32_t regValue = 0;
	uint32_t regLowBits = 0;
	uint32_t regHighBit = 0;

	if (!IsMultiFormatActive(context))
		channel = NTV2_CHANNEL1;

	value = NTV2_FRAMERATE_UNKNOWN;
	regValue = ntv2ReadRegister(context, gChannelToGlobalControlRegNum[channel]);
	regLowBits = (regValue & kRegMaskFrameRate) >> kRegShiftFrameRate;
	regHighBit = (regValue & kRegMaskFrameRateHiBit) >> kRegShiftFrameRateHiBit;

	value = (NTV2FrameRate)((regLowBits & 0x7) | ((regHighBit & 0x1) << 3));
	return value;
}

bool IsProgressiveStandard (Ntv2SystemContext* context, NTV2Channel channel)
{
	bool smpte372Enabled = false;
	NTV2Standard standard =	NTV2_STANDARD_INVALID;

	if (!IsMultiFormatActive(context))
		channel = NTV2_CHANNEL1;

	smpte372Enabled = GetSmpte372(context, channel);
	standard =	GetStandard(context, channel);
	return ((NTV2_IS_PROGRESSIVE_STANDARD(standard) || smpte372Enabled) ? true : false);
}

bool GetSmpte372 (Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t returnVal = 0;

	if (!IsMultiFormatActive(context))
		channel = NTV2_CHANNEL1;
 
	ntv2ReadRegisterMS(context, gChannelToSmpte372RegisterNum[channel], &returnVal, gChannelToSmpte372Masks[channel], gChannelToSmpte372Shifts[channel]);

	return (returnVal ? true : false);
}

bool GetQuadFrameEnable(Ntv2SystemContext* context, NTV2Channel channel)
{
	return ((Get4kSquaresEnable(context, channel) || Get425FrameEnable(context, channel)) ? true : false);
}

bool Get4kSquaresEnable (Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t squaresEnabled = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(!NTV2DeviceCanDo4KVideo(deviceID))
		return false;

	if (channel < NTV2_CHANNEL5)
		ntv2ReadRegisterMS(context, kRegGlobalControl2, &squaresEnabled, kRegMaskQuadMode, kRegShiftQuadMode);
	else
		ntv2ReadRegisterMS(context, kRegGlobalControl2, &squaresEnabled, kRegMaskQuadMode2, kRegShiftQuadMode2);
	return (squaresEnabled ? true : false);
}

bool Get425FrameEnable (Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t returnVal = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(!NTV2DeviceCanDo425Mux(deviceID) && !NTV2DeviceCanDo12gRouting(deviceID))
		return false;

	if (NTV2DeviceCanDo12gRouting(deviceID))
	{
		ntv2ReadRegisterMS(context, gChannelToGlobalControlRegNum[channel], &returnVal, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable);
	}
	else
	{
		if (channel < NTV2_CHANNEL3)
			ntv2ReadRegisterMS(context, kRegGlobalControl2, &returnVal, kRegMask425FB12, kRegShift425FB12);
		else if (channel < NTV2_CHANNEL5)
			ntv2ReadRegisterMS(context, kRegGlobalControl2, &returnVal, kRegMask425FB34, kRegShift425FB34);
		else if (channel < NTV2_CHANNEL7)
			ntv2ReadRegisterMS(context, kRegGlobalControl2, &returnVal, kRegMask425FB56, kRegShift425FB56);
		else
			ntv2ReadRegisterMS(context, kRegGlobalControl2, &returnVal, kRegMask425FB78, kRegShift425FB78);
	}

	return (returnVal ? true : false);
}

bool Get12GTSIFrameEnable (Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t returnVal = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(!NTV2DeviceCanDo12gRouting(deviceID))
		return false;

	ntv2ReadRegisterMS(context, gChannelToGlobalControlRegNum[channel], &returnVal, kRegMaskQuadTsiEnable, kRegShiftQuadTsiEnable);
	return (returnVal ? true : false);
}

bool GetQuadQuadFrameEnable(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t outValue = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	(void)channel;

	if (NTV2DeviceCanDo8KVideo(deviceID))
	{
		ntv2ReadRegisterMS(context, kRegGlobalControl3, &outValue, kRegMaskQuadQuadMode, kRegShiftQuadQuadMode);
		return (outValue ? true : false);
	}
	return false;
}

bool GetQuadQuadSquaresEnable(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t squaresEnabled = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	(void)channel;

	if (!NTV2DeviceCanDo8KVideo(deviceID))
		return false;

	ntv2ReadRegisterMS(context, kRegGlobalControl3, &squaresEnabled, kRegMaskQuadQuadSquaresMode, kRegShiftQuadQuadSquaresMode);
	return (squaresEnabled ? true : false);
}

bool IsMultiFormatActive (Ntv2SystemContext* context)
{
	uint32_t returnVal = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(!NTV2DeviceCanDoMultiFormat(deviceID))
		return false;

	ntv2ReadRegisterMS(context, kRegGlobalControl2, &returnVal, kRegMaskIndependentMode, kRegShiftIndependentMode);
	return (returnVal ? true : false);
}

bool GetEnable4KDCPSFOutMode(Ntv2SystemContext* context)
{
	uint32_t regValue = 0;

	ntv2ReadRegisterMS(context, kRegDC1, &regValue, kRegMask4KDCPSFOutMode, kRegShift4KDCPSFOutMode);
	return (regValue == 0 ? false : true);
}

NTV2FrameBufferFormat GetFrameBufferFormat(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t regNum = gChannelToControlRegNum[channel];
	uint32_t regValue =  ntv2ReadRegister(context, regNum); 
	uint32_t loValue = (regValue & kRegMaskFrameFormat) >> kRegShiftFrameFormat;
	uint32_t hiValue = (regValue & kRegMaskFrameFormatHiBit) >> kRegShiftFrameFormatHiBit;
	uint32_t value = loValue | (hiValue << 4);

	return (NTV2FrameBufferFormat)value;
}

void SetFrameBufferFormat(Ntv2SystemContext* context, NTV2Channel channel, NTV2FrameBufferFormat value)
{
	uint32_t regNum = gChannelToControlRegNum[channel];
	uint32_t loValue = value & 0x0f;
	uint32_t hiValue = (value & 0x10) >> 4;

	ntv2WriteRegisterMS(context, regNum, loValue, kRegMaskFrameFormat, kRegShiftFrameFormat);
	ntv2WriteRegisterMS(context, regNum, hiValue, kRegMaskFrameFormatHiBit, kRegShiftFrameFormatHiBit);
}

NTV2VideoFrameBufferOrientation GetFrameBufferOrientation(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t regNum = gChannelToControlRegNum[channel];
	uint32_t regValue =  0;

	ntv2ReadRegisterMS(context, regNum, &regValue, kRegMaskFrameOrientation, kRegShiftFrameOrientation);
	return (NTV2VideoFrameBufferOrientation)regValue;
}

void SetFrameBufferOrientation(Ntv2SystemContext* context, NTV2Channel channel, NTV2VideoFrameBufferOrientation value)
{
	uint32_t regNum = gChannelToControlRegNum[channel];

	ntv2WriteRegisterMS(context, regNum, value, kRegMaskFrameOrientation, kRegShiftFrameOrientation);
}

bool GetConverterOutStandard(Ntv2SystemContext* context, NTV2Standard* value)
{
	return ntv2ReadRegisterMS(context, kRegConversionControl, (ULWord*)value, kK2RegMaskConverterOutStandard, kK2RegShiftConverterOutStandard);
}

bool ReadFSHDRRegValues(Ntv2SystemContext* context, NTV2Channel channel, HDRDriverValues* hdrRegValues)
{
	(void)channel;
	hdrRegValues->redPrimaryX = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrRedXCh1);
	hdrRegValues->redPrimaryY = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrRedYCh1);
	hdrRegValues->greenPrimaryX = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrGreenXCh1);
	hdrRegValues->greenPrimaryY = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrGreenYCh1);
	hdrRegValues->bluePrimaryX = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrBlueXCh1);
	hdrRegValues->bluePrimaryY = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrBlueYCh1);
	hdrRegValues->whitePointX = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrWhiteXCh1);
	hdrRegValues->whitePointY = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrWhiteYCh1);
	hdrRegValues->minMasteringLuminance = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrMasterLumMinCh1);
	hdrRegValues->maxMasteringLuminance = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrMasterLumMaxCh1);
	hdrRegValues->maxContentLightLevel = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrMaxCLLCh1);
	hdrRegValues->maxFrameAverageLightLevel = (uint16_t)ntv2ReadVirtualRegister(context, kVRegHdrMaxFALLCh1);
	hdrRegValues->electroOpticalTransferFunction = (uint8_t)ntv2ReadVirtualRegister(context, kVRegHdrTransferCh1);
	hdrRegValues->staticMetadataDescriptorID = (uint8_t)ntv2ReadVirtualRegister(context, kVRegHdrColorimetryCh1);
	hdrRegValues->luminance = (uint8_t)ntv2ReadVirtualRegister(context, kVRegHdrLuminanceCh1);
	//hdrRegValues->electroOpticalTransferFunction = (uint8_t)ntv2ReadVirtualRegister(context, kVRegNTV2VPIDTransferCharacteristics);
	//hdrRegValues->staticMetadataDescriptorID = (uint8_t)ntv2ReadVirtualRegister(context, kVRegNTV2VPIDColorimetry);
	//hdrRegValues->luminance = (uint8_t)ntv2ReadVirtualRegister(context, kVRegNTV2VPIDLuminance);
	return true;
}

///////////////////////
NTV2Mode GetMode(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t regNum = gChannelToControlRegNum[channel];
	uint32_t regValue =  0;

	ntv2ReadRegisterMS(context, regNum, &regValue, kRegMaskMode, kRegShiftMode); 
	return (NTV2Mode)regValue;
}

void SetMode(Ntv2SystemContext* context, NTV2Channel channel, NTV2Mode value)
{
	uint32_t regNum = gChannelToControlRegNum[channel];

	ntv2WriteRegisterMS(context, regNum, value, kRegMaskMode, kRegShiftMode);
}

ULWord GetOutputFrame(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t regNum = gChannelToOutputFrameRegNum[channel];

	return ntv2ReadRegister(context, regNum); 
}

void SetOutputFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value)
{
	uint32_t regNum = gChannelToOutputFrameRegNum[channel];

	ntv2WriteRegister(context, regNum, value);
}

uint32_t GetInputFrame(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t regNum = gChannelToInputFrameRegNum[channel];

	return ntv2ReadRegister(context, regNum); 
}

void SetInputFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value)
{
	uint32_t regNum = gChannelToInputFrameRegNum[channel];

	ntv2WriteRegister(context, regNum, value);
}

uint32_t GetPCIAccessFrame(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t regNum = gChannelToPCIAccessFrameRegNum[channel];

	return ntv2ReadRegister(context, regNum); 
}

void SetPCIAccessFrame(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value)
{
	uint32_t regNum = gChannelToPCIAccessFrameRegNum[channel];

	ntv2WriteRegister(context, regNum, value);
}

bool Get2piCSC(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t returnVal = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (!NTV2DeviceCanDo425Mux(deviceID))
		return false;

	switch (channel)
	{
	case NTV2_CHANNEL1:
	case NTV2_CHANNEL2:
	case NTV2_CHANNEL3:
	case NTV2_CHANNEL4:
		ntv2ReadRegisterMS(context, kRegCSCoefficients5_6, &returnVal, kK2RegMask2piCSC1, kK2RegMask2piCSC1);
		break;
	case NTV2_CHANNEL5:
	case NTV2_CHANNEL6:
	case NTV2_CHANNEL7:
	case NTV2_CHANNEL8:
		ntv2ReadRegisterMS(context, kRegCS5Coefficients5_6, &returnVal, kK2RegMask2piCSC5, kK2RegShift2piCSC5);
		break;
	default:
		break;
	}
	return (returnVal ? true : false);
}

bool Set2piCSC(Ntv2SystemContext* context, NTV2Channel channel, bool enable)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (!NTV2DeviceCanDo425Mux(deviceID))
		return false;

	switch (channel)
	{
	case NTV2_CHANNEL1:
	case NTV2_CHANNEL2:
	case NTV2_CHANNEL3:
	case NTV2_CHANNEL4:
		return ntv2WriteRegisterMS(context, kRegCSCoefficients5_6, enable ? 1 : 0, kK2RegMask2piCSC1, kK2RegShift2piCSC1);
		break;
	case NTV2_CHANNEL5:
	case NTV2_CHANNEL6:
	case NTV2_CHANNEL7:
	case NTV2_CHANNEL8:
		return ntv2WriteRegisterMS(context, kRegCS5Coefficients5_6, enable ? 1 : 0, kK2RegMask2piCSC5, kK2RegShift2piCSC5);
		break;
	default:
		return false;
	}
}

NTV2FrameBufferFormat GetDualLink5PixelFormat(Ntv2SystemContext* context)
{
	uint32_t regValue = ntv2ReadRegister(context, kRegDL5Control); 
	uint32_t loValue = (regValue & kRegMaskFrameFormat) >> kRegShiftFrameFormat;
	uint32_t hiValue = (regValue & kRegMaskFrameFormatHiBit) >> kRegShiftFrameFormatHiBit;
	uint32_t value = loValue | (hiValue << 4);

	return (NTV2FrameBufferFormat)value;
}

void SetDualLink5PixelFormat(Ntv2SystemContext* context, NTV2FrameBufferFormat value)
{
	uint32_t loValue = value & 0x0f;
	uint32_t hiValue = (value & 0x10) >> 4;

	ntv2WriteRegisterMS(context, kRegDL5Control, loValue, kRegMaskFrameFormat, kRegShiftFrameFormat);
	ntv2WriteRegisterMS(context, kRegDL5Control, hiValue, kRegMaskFrameFormatHiBit, kRegShiftFrameFormatHiBit);
}

ULWord GetHWFrameBufferSize(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord regValue;
	ULWord size;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(NTV2DeviceCanReportFrameSize(deviceID))
	{
		ULWord quadMultiplier = 1;
		
		if (GetQuadFrameEnable(context, channel))
			quadMultiplier = 4;
		if (GetQuadQuadFrameEnable(context, channel))
			quadMultiplier = 16;
		regValue = ntv2ReadRegister(context, kRegCh1Control);
		regValue &= BIT_20 | BIT_21;
		switch(regValue)
		{
		default:
		case 0:
			size = 2*1024*1024*quadMultiplier;
			break;
		case BIT_20:
			size = 4*1024*1024*quadMultiplier;
			break;
		case BIT_21:
			size = 8*1024*1024*quadMultiplier;
			break;
		case BIT_20 | BIT_21:
			size = 16*1024*1024*quadMultiplier;
			break;
		}
		return size;
	}

	if( !NTV2DeviceSoftwareCanChangeFrameBufferSize(deviceID) )
		return 0;

	if( GetQuadFrameEnable(context, channel) )
		return 0;

	regValue  = ntv2ReadRegister(context, kRegCh1Control);

	if( !(regValue & BIT_29) )
		return 0;

	regValue &= BIT_20 | BIT_21;
	switch(regValue)
	{
	default:
	case 0:
		size = 2*1024*1024;
		break;
	case BIT_20:
		size = 4*1024*1024;
		break;
	case BIT_21:
		size = 8*1024*1024;
		break;
	case BIT_20 | BIT_21:
		size = 16*1024*1024;
		break;
	}
	return size;
}

ULWord GetFrameBufferSize(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord frameSize1;
	ULWord frameSize2;
	ULWord frameBufferSize;
	NTV2FrameGeometry frameGeometry = GetFrameGeometry(context, NTV2_CHANNEL1);
	NTV2FrameBufferFormat frameBufferFormat1 = GetFrameBufferFormat(context, NTV2_CHANNEL1);
	NTV2FrameBufferFormat frameBufferFormat2 = GetFrameBufferFormat(context, NTV2_CHANNEL2);
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	
	if(NTV2DeviceCanReportFrameSize(deviceID))
	{
		return GetHWFrameBufferSize(context, channel);
	}
	else
	{
		ULWord fbSize = GetHWFrameBufferSize(context, NTV2_CHANNEL1);
		if(fbSize)
			return fbSize;

		if(NTV2DeviceCanDo4KVideo(deviceID))
		{
			NTV2FrameBufferFormat frameBufferFormat3, frameBufferFormat4;
			ULWord frameSize3, frameSize4;
			ULWord tempSize1, tempSize2;
			if(GetQuadFrameEnable(context, channel))
			{
				//Kludge: For some reason 2048x1080 used a 16M frame size 
				switch(frameGeometry)
				{
				case NTV2_FG_1920x1080:
					frameGeometry = NTV2_FG_4x1920x1080;
					break;
				case NTV2_FG_2048x1080:
					frameGeometry = NTV2_FG_4x2048x1080;
					break;
				default:
					break;
				}
			}
			frameBufferFormat3 = GetFrameBufferFormat(context, NTV2_CHANNEL3);
			frameBufferFormat4 = GetFrameBufferFormat(context, NTV2_CHANNEL4);
			frameSize1 = NTV2DeviceGetFrameBufferSize(deviceID, frameGeometry, frameBufferFormat1);
			frameSize2 = NTV2DeviceGetFrameBufferSize(deviceID, frameGeometry, frameBufferFormat2);
			frameSize3 = NTV2DeviceGetFrameBufferSize(deviceID, frameGeometry, frameBufferFormat3);
			frameSize4 = NTV2DeviceGetFrameBufferSize(deviceID, frameGeometry, frameBufferFormat4);
			tempSize1 = frameSize1;
			tempSize2 = frameSize3;
			if(frameSize2 > frameSize1)
				tempSize1 = frameSize2;
			if(frameSize4 > frameSize3)
				tempSize2 = frameSize4;
			if(tempSize1 > tempSize2)
				frameBufferSize = tempSize1;
			else
				frameBufferSize = tempSize2;

		}
		else
		{
			frameSize1 = NTV2DeviceGetFrameBufferSize(deviceID, frameGeometry, frameBufferFormat1);
			frameSize2 = NTV2DeviceGetFrameBufferSize(deviceID, frameGeometry, frameBufferFormat2);
			frameBufferSize = frameSize1;
			if(frameSize2 > frameSize1)
			{
				frameBufferSize = frameSize2;
			}
		}
	}

	return frameBufferSize;
}

///////////////////////
bool FieldDenotesStartOfFrame(Ntv2SystemContext* context, NTV2Crosspoint channelSpec)
{
	NTV2Channel channel = NTV2_CHANNEL1;
	NTV2Standard standard = NTV2_STANDARD_INVALID;

	if(IsMultiFormatActive(context))
		channel = GetNTV2ChannelForNTV2Crosspoint(channelSpec);

	standard = GetStandard(context, channel);
	if ( NTV2_IS_PROGRESSIVE_STANDARD(standard))
	{
		return true;
	}
	else
		return IsFieldID0(context, channelSpec);
}

bool IsFieldID0(Ntv2SystemContext* context, NTV2Crosspoint xpt)
{
	static uint32_t outRegNum[] = {kRegStatus, kRegStatus,  kRegStatus,  kRegStatus, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, 0};
	static uint32_t outBit[] = {BIT_23, BIT_5, BIT_3, BIT_1, BIT_9, BIT_7, BIT_5, BIT_3, 0};
	static uint32_t inRegNum[] = {kRegStatus, kRegStatus, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, 0};
	static uint32_t inBit[] = { BIT_21, BIT_19, BIT_21, BIT_19, BIT_17, BIT_15, BIT_13, BIT_11, 0};
	NTV2Channel xptChannel = GetNTV2ChannelForNTV2Crosspoint(xpt);
	bool bField0 = true;
	uint32_t regValue = 0;

	if(NTV2_IS_OUTPUT_CROSSPOINT(xpt))
	{
		xptChannel = IsMultiFormatActive(context) ? xptChannel : NTV2_CHANNEL1;
		regValue = ntv2ReadRegister(context, outRegNum[xptChannel]);
		bField0 = regValue & outBit[xptChannel] ? false : true;
	}
	else
	{
		regValue = ntv2ReadRegister(context, inRegNum[xptChannel]);
		bField0 = regValue & inBit[xptChannel] ? false : true;
	}
	return bField0;
}

bool ProgramProductCode(Ntv2SystemContext* context)
{
	// NTV2DeviceHasSPIFlashSerial serial number is stored in the spiflash
	// read it and put it into the correct registers
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (NTV2DeviceHasSPIFlashSerial(deviceID))
	{
		bool hasExtendedCommandSupport = false;
		uint32_t flashDeviceID = 0;
		int serialSize = 2, i = 0;
		uint32_t bankSelectNumber = 0x00;
		uint32_t serialNumber = 0;
		uint32_t serialRegister = kRegReserved54;
		uint32_t baseAddress = 0xFC0000; //This is defined in ntv2konaserializer.cpp from classes
		
		ntv2WriteRegister(context, kRegXenaxFlashControlStatus, READID_COMMAND);
		WaitForFlashNOTBusy(context);
		flashDeviceID = ntv2ReadRegister(context, kRegXenaxFlashDOUT);

		if ((flashDeviceID & 0x00ffffff) == 0x0020ba20)//Micron MT25QL512ABB
		{
			hasExtendedCommandSupport = true;
		}

		if (NTV2DeviceROMHasBankSelect(deviceID))
		{
			ntv2WriteRegister(context, kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
			WaitForFlashNOTBusy(context);
			bankSelectNumber = NTV2DeviceGetSPIFlashVersion(deviceID) >= 5 ? 0x03 : 0x01;
			ntv2WriteRegister(context, kRegXenaxFlashAddress, bankSelectNumber);
			ntv2WriteRegister(context, kRegXenaxFlashControlStatus, hasExtendedCommandSupport ? EXTENDEDADDRESS_COMMAND : BANKSELECT_COMMMAND);
			WaitForFlashNOTBusy(context);
		}

		if (NTV2DeviceGetSPIFlashVersion(deviceID) > 5)
			serialSize = 4;
		for (i = 0; i < serialSize; i++, baseAddress += 4, serialRegister++)
		{
			serialNumber = 0;
			ntv2WriteRegister(context, kRegXenaxFlashAddress, baseAddress);
			ntv2WriteRegister(context, kRegXenaxFlashControlStatus, READFAST_COMMAND);
			WaitForFlashNOTBusy(context);
			serialNumber = ntv2ReadRegister(context, kRegXenaxFlashDOUT);
			//We might have a misprogrammed io4k+ check it
			if (deviceID == DEVICE_ID_IO4KPLUS && serialNumber == 0xFFFFFFFF)
			{
				ntv2Message("CNTV2::InitializeBoard Io4K+ %08X incorrect serial location\n", serialNumber);
				ntv2WriteRegister(context, kRegXenaxFlashAddress, 0x2);
				ntv2WriteRegister(context, kRegXenaxFlashControlStatus, hasExtendedCommandSupport ? EXTENDEDADDRESS_COMMAND : BANKSELECT_COMMMAND);
				WaitForFlashNOTBusy(context);
				ntv2WriteRegister(context, kRegXenaxFlashAddress, baseAddress);
				ntv2WriteRegister(context, kRegXenaxFlashControlStatus, READFAST_COMMAND);
				WaitForFlashNOTBusy(context);
				serialNumber = ntv2ReadRegister(context, kRegXenaxFlashDOUT);
			}
			ntv2WriteRegister(context, serialRegister, serialNumber);
		}

		if (NTV2DeviceROMHasBankSelect(deviceID))
		{
			ntv2WriteRegister(context, kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
			WaitForFlashNOTBusy(context);
			bankSelectNumber = 0x00;
			ntv2WriteRegister(context, kRegXenaxFlashAddress, bankSelectNumber);
			ntv2WriteRegister(context, kRegXenaxFlashControlStatus, hasExtendedCommandSupport ? EXTENDEDADDRESS_COMMAND : BANKSELECT_COMMMAND);
			WaitForFlashNOTBusy(context);
		}
	}
	return NTV2DeviceHasSPIFlashSerial(deviceID);
}

bool WaitForFlashNOTBusy(Ntv2SystemContext* context)
{
	bool busy = true;
	int count = 0;
	do
	{
		ULWord regValue = ntv2ReadRegister(context, kRegXenaxFlashControlStatus);
		if (regValue & BIT(8))
			busy = true;
		else
			busy = false;
		ntv2TimeSleep(100);
		count++;
	} while ((busy == true) && (count < 100));
	return count == 100 ? false : true;
}

bool SetVideoOutputStandard(Ntv2SystemContext* context, NTV2Channel channel)
{
	HDRDriverValues hdrRegValues;
	NTV2OutputXptID xptSelect;
	NTV2Standard standard = NTV2_NUM_STANDARDS;
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	ULWord is3GaMode = 0;
	ULWord is3GbMode = 0;
	ULWord isTSIMode = 0;
	bool is2Kx1080Mode = false;
	bool enable3Gb = false;
	bool isQuadMode = false;
	bool isQuadQuadMode = false;
	bool isMultiLinkMode = false;
	bool isMLOut1 = false;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(!FindSDIOutputSource(context, &xptSelect, channel))
	{
		if (ntv2ReadVirtualRegister(context, kVRegDisableAutoVPID) == 0)
			SetSDIOutVPID(context, channel, 0, 0);
		return false;
	}

	//ntv2Message("xptSelect = %d\n", xptSelect);
	switch(xptSelect)
	{
	case NTV2_XptConversionModule:
		if(!GetConverterOutStandard(context, &standard))
		{
			return false;
		}
		break;
	case NTV2_XptMultiLinkOut1DS1:
	case NTV2_XptMultiLinkOut1DS2:
	case NTV2_XptMultiLinkOut1DS3:
	case NTV2_XptMultiLinkOut1DS4:
		isMLOut1 = true;
		// fallthrough
	case NTV2_XptMultiLinkOut2DS1:
	case NTV2_XptMultiLinkOut2DS2:
	case NTV2_XptMultiLinkOut2DS3:
	case NTV2_XptMultiLinkOut2DS4:
		isMultiLinkMode = true;
		GetXptMultiLinkOutInputSelect(context, isMLOut1 ? NTV2_CHANNEL1 : NTV2_CHANNEL2, &xptSelect);
		if(!FindCrosspointSource(context, &xptSelect, xptSelect))
		{
			if (ntv2ReadVirtualRegister(context, kVRegDisableAutoVPID) == 0)
				SetSDIOutVPID(context, channel, 0, 0);
			return false;
		}
		// fallthrough
	default:
		if(GetSourceVideoFormat(context, &videoFormat, xptSelect, &isQuadMode, &isQuadQuadMode, &hdrRegValues))
		{
			if (videoFormat == NTV2_FORMAT_UNKNOWN)
			{
				GetSourceVideoFormat(context, &videoFormat, NTV2_XptBlack, &isQuadMode, &isQuadQuadMode, &hdrRegValues);
			}
			standard = GetNTV2StandardFromVideoFormat(videoFormat);
		}
		else
		{
			return false;
		}
	}

	//ntv2Message("format = %d  standard = %d\n", videoFormat, standard);

	if(standard < NTV2_NUM_STANDARDS)
	{
		bool doLevelABConversion = false;
		bool doRGBLevelAConversion = false;

		is2Kx1080Mode = IsVideoFormat2Kx1080(videoFormat);
		is3GaMode = NTV2_VIDEO_FORMAT_IS_A(videoFormat);
		is3GbMode = NTV2_VIDEO_FORMAT_IS_B(videoFormat);
		isTSIMode = Get425FrameEnable(context, channel);
		GetSDIOutLevelAtoLevelBConversion(context, channel, &doLevelABConversion);
		doLevelABConversion = !is3GaMode ? false: doLevelABConversion;
		GetSDIOutRGBLevelAConversion(context, channel, &doRGBLevelAConversion);

		if (NTV2DeviceCanDo3GOut(deviceID, (UWord)channel) || NTV2DeviceCanDo292Out(deviceID, (UWord)channel) || (channel == NTV2_CHANNEL5 && NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1)))
		{
			if(is3GaMode)
			{
				SetSDIOut3GEnable(context, channel, true);
				if(doLevelABConversion)
				{
					SetSDIOut3GbEnable(context, channel, doLevelABConversion);
					standard = doLevelABConversion?NTV2_STANDARD_1080:standard;
				}
				else
				{
					if (isTSIMode)
					{
						//This is to handle the 425 mux modes
						//The videoformat is A, but you wire up ds1 and ds2 so...
						enable3Gb = (GetXptSDIOutDS2InputSelect(context, channel, &xptSelect) && (xptSelect != NTV2_XptBlack)) ? true : false;
						SetSDIOut3GbEnable(context, channel, enable3Gb);
					}
					else
					{
						SetSDIOut3GbEnable(context, channel, false);
					}
				}
			}
			else
			{
				if(deviceID == DEVICE_ID_KONALHI)
				{
					if(is3GbMode)
					{
						SetSDIOut3GEnable(context, channel, true);
						SetSDIOut3GbEnable(context, channel, true);
					}
					else
					{
						SetSDIOut3GEnable(context, channel, false);
						SetSDIOut3GbEnable(context, channel, false);
					}
				}
				else
				{
					enable3Gb = (GetXptSDIOutDS2InputSelect(context, channel, &xptSelect) && (xptSelect != NTV2_XptBlack)) ? true : false;
					SetSDIOut3GEnable(context, channel, enable3Gb);
					SetSDIOut3GbEnable(context, channel, !doRGBLevelAConversion ? enable3Gb : false);
				}
			}
		}

		if (NTV2DeviceCanDo12GOut(deviceID, (UWord)channel))
		{
			if (is3GaMode)
			{
				SetSDIOut3GEnable(context, channel, true);
				if (doLevelABConversion)
				{
					SetSDIOut3GbEnable(context, channel, doLevelABConversion);
					standard = doLevelABConversion ? NTV2_STANDARD_1080 : standard;
				}
				else
				{
					enable3Gb = (GetXptSDIOutDS2InputSelect(context, channel, &xptSelect) && (xptSelect != NTV2_XptBlack)) ? true : false;
					SetSDIOut3GbEnable(context, channel, enable3Gb);
				}
			}
			else
			{
				enable3Gb = (GetXptSDIOutDS2InputSelect(context, channel, &xptSelect) && (xptSelect != NTV2_XptBlack)) ? true : false;
				SetSDIOut3GEnable(context, channel, enable3Gb);
				SetSDIOut3GbEnable(context, channel, !doRGBLevelAConversion ? enable3Gb : false);
			}

			if (isMultiLinkMode)
			{
				SetSDIOut12GEnable(context, channel, false);
				SetSDIOut6GEnable(context, channel, false);
			}
			else if (NTV2DeviceCanDo12gRouting(deviceID))
			{
				if ((isQuadMode || isQuadQuadMode) && (is3GaMode || enable3Gb))
				{
					SetSDIOut12GEnable(context, channel, true);
					SetSDIOut6GEnable(context, channel, false);
				}
				else if (isQuadMode || isQuadQuadMode)
				{
					SetSDIOut12GEnable(context, channel, false);
					SetSDIOut6GEnable(context, channel, true);
				}
				else if (is3GaMode && enable3Gb) //HFR HD RGB 6G out
				{
					SetSDIOut12GEnable(context, channel, false);
					SetSDIOut6GEnable(context, channel, true);
				}
				else
				{
					SetSDIOut12GEnable(context, channel, false);
					SetSDIOut6GEnable(context, channel, false);
				}
			}
		}

		SetSDIOutStandard(context, channel, standard);
		SetSDIOut_2Kx1080Enable(context, channel, is2Kx1080Mode);

		if (ntv2ReadVirtualRegister(context, kVRegDisableAutoVPID) == 0)
			SetVPIDOutput(context , channel);
		return true;
	}
	else
	{
		return false;
	}
}

bool SetSDIOutStandard(Ntv2SystemContext* context, NTV2Channel channel, NTV2Standard value)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];

	return ntv2WriteRegisterMS(context, regNum, value, kK2RegMaskSDIOutStandard, kK2RegShiftSDIOutStandard);
}

bool SetSDIOut_2Kx1080Enable(Ntv2SystemContext* context, NTV2Channel channel, bool enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];

	return ntv2WriteRegisterMS(context, regNum, enable ? 1 : 0, kK2RegMaskSDI1Out_2Kx1080Mode, kK2RegShiftSDI1Out_2Kx1080Mode);
}

bool GetSDIOut3GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];
	uint32_t tempVal = 0;

	bool retVal = ntv2ReadRegisterMS(context, regNum, &tempVal, kLHIRegMaskSDIOut3GbpsMode, kLHIRegShiftSDIOut3GbpsMode);
	*enable = tempVal == 1 ? true : false;
	return retVal;
}

bool SetSDIOut3GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];

	return ntv2WriteRegisterMS(context, regNum, enable ? 1 : 0, kLHIRegMaskSDIOut3GbpsMode, kLHIRegShiftSDIOut3GbpsMode);
}

bool GetSDIOut3GbEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];
	uint32_t tempVal = 0;

	bool retVal = ntv2ReadRegisterMS(context, regNum, &tempVal, kLHIRegMaskSDIOutSMPTELevelBMode, kLHIRegShiftSDIOutSMPTELevelBMode);
	*enable = tempVal == 1 ? true : false;
	return retVal;
}

bool SetSDIOut3GbEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];

	return ntv2WriteRegisterMS(context, regNum, enable ? 1 : 0, kLHIRegMaskSDIOutSMPTELevelBMode, kLHIRegShiftSDIOutSMPTELevelBMode);
}

bool GetSDIOut6GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];
	uint32_t is6G = 0, is12G = 0;
	bool retVal = false;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (!NTV2DeviceCanDo12GOut(deviceID, (UWord)channel))
		return false;

	retVal = ntv2ReadRegisterMS(context, regNum, &is6G, kRegMaskSDIOut6GbpsMode, kRegShiftSDIOut6GbpsMode);
	retVal = ntv2ReadRegisterMS(context, regNum, &is12G, kRegMaskSDIOut12GbpsMode, kRegShiftSDIOut12GbpsMode);
	*enable = (is6G == 1 && is12G == 0) ? true : false;
	return retVal;
}

bool SetSDIOut6GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];

	return ntv2WriteRegisterMS(context, regNum, enable ? 1 : 0, kRegMaskSDIOut6GbpsMode, kRegShiftSDIOut6GbpsMode);
}

bool GetSDIOut12GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool* enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];
	uint32_t is12G = 0;
	bool retVal = false;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (!NTV2DeviceCanDo12GOut(deviceID, (UWord)channel))
		return false;

	retVal = ntv2ReadRegisterMS(context, regNum, &is12G, kRegMaskSDIOut12GbpsMode, kRegShiftSDIOut12GbpsMode);
	*enable = (is12G == 1) ? true : false;
	return retVal;
}

bool SetSDIOut12GEnable(Ntv2SystemContext* context, NTV2Channel channel, bool enable)
{
	uint32_t regNum = gChannelToSDIOutControlRegNum[channel];

	return ntv2WriteRegisterMS(context, regNum, enable ? 1 : 0, kRegMaskSDIOut12GbpsMode, kRegShiftSDIOut12GbpsMode);
}

bool GetSDIOutRGBLevelAConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable)
{
	ULWord	regNum = gChannelToSDIOutControlRegNum[channel];
	ULWord tempVal = 0;
	bool retVal = false;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (!NTV2DeviceCanDoRGBLevelAConversion(deviceID))
	{
		return false;
	}

	retVal = ntv2ReadRegisterMS(context, regNum, &tempVal, kRegMaskRGBLevelA, kRegShiftRGBLevelA);
	*enable = (tempVal == 1) ? true : false;
	return retVal;
}

bool GetSDIOutLevelAtoLevelBConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable)
{
	ULWord	regNum = gChannelToSDIOutControlRegNum[channel];
	ULWord tempVal = 0;
	bool retVal = false;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(!NTV2DeviceCanDo3GLevelConversion(deviceID))
	{
		return false;
	}

	retVal = ntv2ReadRegisterMS(context, regNum, &tempVal, kRegMaskSDIOutLevelAtoLevelB, kRegShiftSDIOutLevelAtoLevelB);
	*enable = (tempVal == 1) ? true : false;
	return retVal;
}

bool GetSDIInLevelBtoLevelAConversion(Ntv2SystemContext* context, NTV2Channel channel, bool* enable)
{
	ULWord tempVal = 0;
	bool retVal = false;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (!NTV2DeviceCanDo3GLevelConversion(deviceID))
	{
		return false;
	}

	retVal = ntv2ReadRegisterMS(context, gChannelToSDIInput3GStatusRegNum[channel], &tempVal, gChannelToInputLevelConversionMasks[channel], gChannelToInputLevelConversionShifts[channel]);
	*enable = (tempVal == 1) ? true : false;
	return retVal;
}

bool GetSDIIn6GEnable(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord is6G = 0;
	ULWord    regNum = gChannelToSDIInput3GStatusRegNum[channel];
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	if (!NTV2DeviceCanDo12GIn(deviceID, (UWord)channel))
		return false;
	
	ntv2ReadRegisterMS(context, regNum, &is6G, gChannelToSDIIn6GModeMask[channel], gChannelToSDIIn6GModeShift[channel]);
	return (is6G == 1) ? true : false;
}

bool GetSDIIn12GEnable(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord is12G = 0;
	ULWord    regNum = gChannelToSDIInput3GStatusRegNum[channel];
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	if (!NTV2DeviceCanDo12GIn(deviceID, (UWord)channel))
		return false;
	
	ntv2ReadRegisterMS(context, regNum, &is12G, gChannelToSDIIn12GModeMask[channel], gChannelToSDIIn12GModeShift[channel]);
	return (is12G == 1) ? true : false;
}

///////////////////////
//hdmi routines
bool SetLHiHDMIOutputStandard(Ntv2SystemContext* context)
{
	NTV2Standard standard = NTV2_NUM_STANDARDS;
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	NTV2FrameRate videoRate = NTV2_FRAMERATE_UNKNOWN;
	NTV2OutputXptID xptSelect;
	ULWord hdmiOutput = 0;
	ULWord hdmiStatus = 0;
	bool isQuadMode = false;
	bool isQuadQuadMode = false;
	HDRDriverValues hdrRegValues;
	NTV2Standard contStandard;
	NTV2FrameRate contFrameRate;

	if(!FindHDMIOutputSource(context, &xptSelect, NTV2_CHANNEL1))
	{
		return false;
	}
	if(xptSelect == NTV2_XptConversionModule)
	{
		if(!GetConverterOutStandard(context, &standard))
		{
			return false;
		}
		if(!GetK2ConverterOutFormat(context, &videoFormat))
		{
			return false;
		}
	}
	else
	{
		if (GetSourceVideoFormat(context, &videoFormat, xptSelect, &isQuadMode, &isQuadQuadMode, &hdrRegValues))
		{
			standard = GetNTV2StandardFromVideoFormat(videoFormat);
		}
		else
		{
			return false;
		}
	}

	videoRate = GetNTV2FrameRateFromVideoFormat(videoFormat);
	if(videoRate == NTV2_FRAMERATE_UNKNOWN)
	{
		return false;
	}

	hdmiStatus = ntv2ReadRegister(context, kRegHDMIInputStatus);
	hdmiOutput = ntv2ReadRegister(context, kRegHDMIOutControl);
	contStandard = (NTV2Standard)((hdmiOutput & kRegMaskHDMIOutVideoStd) >> kRegShiftHDMIOutVideoStd);
	contFrameRate = (NTV2FrameRate)((hdmiOutput & kLHIRegMaskHDMIOutFPS) >> kLHIRegShiftHDMIOutFPS);

	if((contStandard != standard) ||
		(contFrameRate != videoRate))
	{
		hdmiOutput = (hdmiOutput & ~kRegMaskHDMIOutVideoStd) | (standard << kRegShiftHDMIOutVideoStd);
		hdmiOutput = (hdmiOutput & ~kLHIRegMaskHDMIOutFPS) | (videoRate << kLHIRegShiftHDMIOutFPS);
		ntv2WriteRegister(context, kRegHDMIOutControl, hdmiOutput);
	}

	return true;
}

bool SetHDMIOutputStandard(Ntv2SystemContext* context)
{
	NTV2OutputXptID xptSelect;
	NTV2OutputXptID tempXptSelect;
	HDRDriverValues hdrRegValues;
	NTV2DeviceID deviceID;
	ULWord hdmiVersion;
	NTV2Standard currentStandard;
	NTV2FrameRate currentFrameRate;
	ULWord currentSampling;
	ULWord currentLevelBMode;
	ULWord currentDecimateMode;
	ULWord currentSourceRGB;
	ULWord levelBMode;
	NTV2Standard standard = NTV2_NUM_STANDARDS;
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	NTV2FrameRate videoRate = NTV2_FRAMERATE_UNKNOWN;
	NTV2FrameGeometry hdmiv2fg = NTV2_FG_NUMFRAMEGEOMETRIES;
	bool bFormatIsTSI = false;
	bool isQuadMode = false;
	bool isQuadQuadMode = false;
	bool is4k = false;
	bool isLevelB = false;
	bool isSourceRGB = false;
	bool useHDMI420Mode = false;
	NTV2Standard hdmiv2std = NTV2_STANDARD_INVALID;
	ULWord sampling = NTV2_HDMI_422;
	deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	hdmiVersion = NTV2DeviceGetHDMIVersion(deviceID);
	
	if (hdmiVersion == 0)
		return false;
	if (hdmiVersion == 1)
		return SetLHiHDMIOutputStandard(context);

	memset(&hdrRegValues, 0, sizeof(HDRDriverValues));

	if (!FindHDMIOutputSource(context, &xptSelect, NTV2_CHANNEL1))
	{
		return false;
	}
	if (xptSelect == NTV2_XptConversionModule)
	{
		if (!GetConverterOutStandard(context, &standard))
		{
			return false;
		}
		if (!GetK2ConverterOutFormat(context, &videoFormat))
		{
			return false;
		}
	}
	else
	{
		if (GetSourceVideoFormat(context, &videoFormat, xptSelect, &isQuadMode, &isQuadQuadMode, &hdrRegValues))
		{
			if (NTV2DeviceCanDo12gRouting(deviceID))
			{
				bFormatIsTSI = Get425FrameEnable(context, NTV2_CHANNEL1);
			}
		}
		else
		{
			return false;
		}
	}

	videoRate = GetNTV2FrameRateFromVideoFormat(videoFormat);
	if (videoRate == NTV2_FRAMERATE_UNKNOWN)
	{
		return false;
	}

	is4k = false;
	isLevelB = false;
	GetXptHDMIOutInputSelect(context, &tempXptSelect);
	isSourceRGB = (tempXptSelect & 0x80) != 0 ? true : false;
	useHDMI420Mode = false;
	if (hdmiVersion == 2 || hdmiVersion == 4 || hdmiVersion == 5)
	{
		bool isQuadLink = false;
		NTV2OutputXptID q2connection = NTV2_XptBlack;
		NTV2OutputXptID q3connection = NTV2_XptBlack;
		NTV2OutputXptID q4connection = NTV2_XptBlack;
		GetXptHDMIOutQ2InputSelect(context, &q2connection);
		GetXptHDMIOutQ3InputSelect(context, &q3connection);
		GetXptHDMIOutQ4InputSelect(context, &q4connection);
		if (q2connection != NTV2_XptBlack && q3connection != NTV2_XptBlack && q4connection != NTV2_XptBlack)
		{
			is4k = true;
			isQuadLink = true;
		}
		if (NTV2DeviceCanDo12gRouting(deviceID) && bFormatIsTSI)
			is4k = true;
		if (q2connection != NTV2_XptBlack && q3connection == NTV2_XptBlack && q4connection == NTV2_XptBlack)
			isLevelB = true;
		if (hdmiVersion >= 4)
			useHDMI420Mode = ntv2ReadVirtualRegister(context, kVRegHDMIOutColorSpaceModeCtrl) == kHDMIOutCSCYCbCr8bit ? true : false;
		if (HasMultiRasterWidget(context))
		{
			if (isQuadLink)
			{
				NTV2OutputXptID mrXptSelect;
				NTV2VideoFormat mrVideoFormat;
				NTV2Standard mrStandard;
				bool mrQMode = false, mrQQMode = false;
				HDRDriverValues mrRegValues;
				FindHDMIOutputSource(context, &mrXptSelect, NTV2_CHANNEL1);
				GetSourceVideoFormat(context, &mrVideoFormat, mrXptSelect, &mrQMode, &mrQQMode, &mrRegValues);
				mrStandard = (NTV2_IS_PSF_VIDEO_FORMAT(mrVideoFormat) || mrVideoFormat == NTV2_FORMAT_UNKNOWN) ? NTV2_STANDARD_1080p : GetNTV2StandardFromVideoFormat(mrVideoFormat);
				SetMultiRasterInputStandard(context, mrStandard, NTV2_CHANNEL1);

				FindHDMIOutputSource(context, &mrXptSelect, NTV2_CHANNEL2);
				GetSourceVideoFormat(context, &mrVideoFormat, mrXptSelect, &mrQMode, &mrQQMode, &mrRegValues);
				mrStandard = (NTV2_IS_PSF_VIDEO_FORMAT(mrVideoFormat) || mrVideoFormat == NTV2_FORMAT_UNKNOWN) ? NTV2_STANDARD_1080p : GetNTV2StandardFromVideoFormat(mrVideoFormat);
				SetMultiRasterInputStandard(context, mrStandard, NTV2_CHANNEL2);

				FindHDMIOutputSource(context, &mrXptSelect, NTV2_CHANNEL3);
				GetSourceVideoFormat(context, &mrVideoFormat, mrXptSelect, &mrQMode, &mrQQMode, &mrRegValues);
				mrStandard = (NTV2_IS_PSF_VIDEO_FORMAT(mrVideoFormat) || mrVideoFormat == NTV2_FORMAT_UNKNOWN) ? NTV2_STANDARD_1080p : GetNTV2StandardFromVideoFormat(mrVideoFormat);
				SetMultiRasterInputStandard(context, mrStandard, NTV2_CHANNEL3);

				FindHDMIOutputSource(context, &mrXptSelect, NTV2_CHANNEL4);
				GetSourceVideoFormat(context, &mrVideoFormat, mrXptSelect, &mrQMode, &mrQQMode, &mrRegValues);
				mrStandard = (NTV2_IS_PSF_VIDEO_FORMAT(mrVideoFormat) || mrVideoFormat == NTV2_FORMAT_UNKNOWN) ? NTV2_STANDARD_1080p : GetNTV2StandardFromVideoFormat(mrVideoFormat);
				SetMultiRasterInputStandard(context, mrStandard, NTV2_CHANNEL4);

				SetEnableMultiRasterCapture(context, true);

				videoFormat = NTV2_IS_FRACTIONAL_NTV2FrameRate(videoRate) ? NTV2_FORMAT_1080p_5994_A : NTV2_FORMAT_1080p_6000_A;
			}
			else
			{
				SetEnableMultiRasterCapture(context, false);
			}
		}
	}

	standard = NTV2_IS_PSF_VIDEO_FORMAT(videoFormat) ? NTV2_STANDARD_1080p : GetNTV2StandardFromVideoFormat(videoFormat);
	hdmiv2std = NTV2_STANDARD_INVALID;
	sampling = NTV2_HDMI_422;
	switch(standard)
	{
	case NTV2_STANDARD_1080:
		switch(videoRate)
		{
		case NTV2_FRAMERATE_2398:
		case NTV2_FRAMERATE_2400:
			hdmiv2std = NTV2_STANDARD_1080p;
			break;
		default:
			hdmiv2std = NTV2_STANDARD_1080;
			break;
		}
		if(isLevelB)
			hdmiv2std = NTV2_STANDARD_1080p;
		break;
	case NTV2_STANDARD_720: 
		hdmiv2std = NTV2_STANDARD_720;
		break;
	case NTV2_STANDARD_525: 
		hdmiv2std = NTV2_STANDARD_525;
		break;
	case NTV2_STANDARD_625:
		hdmiv2std = NTV2_STANDARD_625;
		break;
	case NTV2_STANDARD_1080p:
		hdmiv2fg = GetFrameGeometry(context, NTV2_CHANNEL1);
		if (is4k)
		{
			if (hdmiVersion >= 4)
			{
				if (NTV2_IS_2K_1080_FRAME_GEOMETRY(hdmiv2fg) || hdmiv2fg == NTV2_FG_4x2048x1080)
				{
					switch (videoRate)
					{
					case NTV2_FRAMERATE_5000:
					case NTV2_FRAMERATE_5994:
					case NTV2_FRAMERATE_6000:
						if (isSourceRGB)
						{
							hdmiv2std = NTV2_STANDARD_4096x2160p;
							sampling = NTV2_HDMI_422;
						}
						else
						{
							hdmiv2std = useHDMI420Mode ? NTV2_STANDARD_4096HFR : NTV2_STANDARD_4096x2160p;
							sampling = useHDMI420Mode ? NTV2_HDMI_420 : NTV2_HDMI_422;
						}
						break;
					default:
						hdmiv2std = NTV2_STANDARD_4096x2160p;
						sampling = NTV2_HDMI_422;
						break;
					}
				}
				else
				{
					switch (videoRate)
					{
					case NTV2_FRAMERATE_5000:
					case NTV2_FRAMERATE_5994:
					case NTV2_FRAMERATE_6000:
						if (isSourceRGB)
						{
							hdmiv2std = NTV2_STANDARD_3840x2160p;
							sampling = NTV2_HDMI_422;
						}
						else
						{
							hdmiv2std = useHDMI420Mode ? NTV2_STANDARD_3840HFR : NTV2_STANDARD_3840x2160p;
							sampling = useHDMI420Mode ? NTV2_HDMI_420 : NTV2_HDMI_422;
						}
						break;
					default:
						hdmiv2std = NTV2_STANDARD_3840x2160p;
						sampling = NTV2_HDMI_422;
						break;
					}
				}
			}
			else
			{
				if (NTV2_IS_2K_1080_FRAME_GEOMETRY(hdmiv2fg))
				{
					switch (videoRate)
					{
					case NTV2_FRAMERATE_5000:
					case NTV2_FRAMERATE_5994:
					case NTV2_FRAMERATE_6000:
						hdmiv2std = NTV2_STANDARD_4096HFR;
						sampling = NTV2_HDMI_420;
						break;
					default:
						hdmiv2std = NTV2_STANDARD_4096x2160p;
						sampling = NTV2_HDMI_422;
						break;
					}
				}
				else
				{
					switch (videoRate)
					{
					case NTV2_FRAMERATE_5000:
					case NTV2_FRAMERATE_5994:
					case NTV2_FRAMERATE_6000:
						hdmiv2std = NTV2_STANDARD_3840HFR;
						sampling = NTV2_HDMI_420;
						break;
					default:
						hdmiv2std = NTV2_STANDARD_3840x2160p;
						sampling = NTV2_HDMI_422;
						break;
					}
				}
			}
		}
		else
		{
			if (NTV2_IS_2K_1080_FRAME_GEOMETRY(hdmiv2fg))
				hdmiv2std = NTV2_STANDARD_2Kx1080p;
			else
				hdmiv2std = NTV2_STANDARD_1080p;
		}
		break;
	case NTV2_STANDARD_2K:		// 2048x1556psf
	default:
		hdmiv2std = NTV2_STANDARD_INVALID;
		break;
	}

	levelBMode = isLevelB ? 1 : 0;

	ntv2ReadRegisterMS(context, kRegHDMIOutControl, (ULWord*)&currentStandard, kRegMaskHDMIOutV2VideoStd, kRegShiftHDMIOutVideoStd);
	ntv2ReadRegisterMS(context, kRegHDMIOutControl, (ULWord*)&currentFrameRate, kLHIRegMaskHDMIOutFPS, kLHIRegShiftHDMIOutFPS);
	ntv2ReadRegisterMS(context, kRegHDMIOutControl, (ULWord*)&currentSampling, kRegMaskHDMISampling, kRegShiftHDMISampling);
	ntv2ReadRegisterMS(context, kRegHDMIOutControl, (ULWord*)&currentSourceRGB, kRegMaskSourceIsRGB, kRegShiftSourceIsRGB);
	ntv2ReadRegisterMS(context, kRegRasterizerControl, (ULWord*)&currentLevelBMode, kRegMaskRasterLevelB, kRegShiftRasterLevelB);
	ntv2ReadRegisterMS(context, kRegRasterizerControl, (ULWord*)&currentDecimateMode, kRegMaskRasterDecimate, kRegShiftRasterDecimate);

	if(currentDecimateMode != 0)
	{
		switch(videoRate)
		{
		case NTV2_FRAMERATE_5000:
			videoRate = NTV2_FRAMERATE_2500;
			break;
		case NTV2_FRAMERATE_5994:
			videoRate = NTV2_FRAMERATE_2997;
			break;
		case NTV2_FRAMERATE_6000:
			videoRate = NTV2_FRAMERATE_3000;
			break;
		default:
			break;
		}

		// Redo the HDMI standard in case HFR was selected above
		hdmiv2std = NTV2_IS_2K_1080_FRAME_GEOMETRY(hdmiv2fg) ? NTV2_STANDARD_4096x2160p : NTV2_STANDARD_3840x2160p;
		sampling = NTV2_HDMI_422;		// Turn off 4:2:0
	}

	if (levelBMode)
	{
		switch (videoRate)
		{
		case NTV2_FRAMERATE_2500:
			videoRate = NTV2_FRAMERATE_5000;
			break;
		case NTV2_FRAMERATE_2997:
			videoRate = NTV2_FRAMERATE_5994;
			break;
		case NTV2_FRAMERATE_3000:
			videoRate = NTV2_FRAMERATE_6000;
			break;
		default:
			break;
		}
	}

	sampling = isSourceRGB ? NTV2_HDMI_RGB : sampling;

	if((currentStandard != hdmiv2std) ||
		(currentFrameRate != videoRate) ||
		(currentLevelBMode != levelBMode) ||
		(currentSampling != sampling))
	{
		ntv2WriteRegisterMS(context, kRegHDMIOutControl, (ULWord)hdmiv2std, kRegMaskHDMIOutV2VideoStd, kRegShiftHDMIOutVideoStd);
		ntv2WriteRegisterMS(context, kRegHDMIOutControl, videoRate, kLHIRegMaskHDMIOutFPS, kLHIRegShiftHDMIOutFPS);
		ntv2WriteRegisterMS(context, kRegHDMIOutControl, sampling, kRegMaskHDMISampling, kRegShiftHDMISampling);

		SetHDMIV2LevelBEnable(context, levelBMode ? true : false);
	}

	if (sampling == NTV2_HDMI_420)
		ntv2WriteRegisterMS(context, kRegHDMIOutControl, 0, kLHIRegMaskHDMIOutColorSpace, kLHIRegShiftHDMIOutColorSpace);
	else
	{
		if (hdmiVersion >= 4)
			ntv2WriteRegisterMS(context, kRegHDMIOutControl, (isSourceRGB? 1 : 0), kLHIRegMaskHDMIOutColorSpace, kLHIRegShiftHDMIOutColorSpace);
	}

	if(NTV2DeviceCanDoHDMIHDROut(deviceID))
	{
		if(	(hdrRegValues.electroOpticalTransferFunction > 0 && hdrRegValues.electroOpticalTransferFunction <= 3) ||
			(hdrRegValues.staticMetadataDescriptorID > 0 && hdrRegValues.staticMetadataDescriptorID <= 3) ||
			(hdrRegValues.luminance == 1) )
		{
			if (hdrRegValues.electroOpticalTransferFunction > 3 ||
				hdrRegValues.staticMetadataDescriptorID > 3 ||
				hdrRegValues.luminance > 1)
			{
				memset(&hdrRegValues, 0, sizeof(HDRDriverValues));
				SetHDRData(context, hdrRegValues);
				EnableHDMIHDR(context, false);
				return true;
			}
			switch(hdrRegValues.electroOpticalTransferFunction)
			{
			case 1:
				hdrRegValues.electroOpticalTransferFunction = 3;
				break;
			default:
				break;
			}
			if(hdmiVersion == 2 || hdmiVersion == 3)
			{
				HDRDriverValues currentValues;
				GetHDRData(context, &currentValues);
				if(HDRIsChanging(currentValues, hdrRegValues))
				{
					if(GetEnableHDMIHDR(context))
					{
						EnableHDMIHDR(context, false);
						return true;
					}
				}
			}
			SetHDRData(context, hdrRegValues);
			EnableHDMIHDR(context, true);
		}
		else
		{
			memset(&hdrRegValues, 0, sizeof(HDRDriverValues));
			SetHDRData(context, hdrRegValues);
			EnableHDMIHDR(context, false);
		}
	}

	return true;
}

bool SetHDMIV2LevelBEnable (Ntv2SystemContext* context, bool enable)
{
	return ntv2WriteRegisterMS(context, kRegRasterizerControl, enable ? 1 : 0, kRegMaskRasterLevelB, kRegShiftRasterLevelB);
}

bool SetMultiRasterInputStandard(Ntv2SystemContext* context, NTV2Standard mrStandard, NTV2Channel mrChannel)
{
	return ntv2WriteRegisterMS(	context, gChannelToMRRegNum[mrChannel], (ULWord)mrStandard, kRegMaskMRStandard, kRegShiftMRStandard);
}

bool SetEnableMultiRasterCapture(Ntv2SystemContext* context, bool bEnable)
{
	ULWord ulEnabled = bEnable ? 1 : 0;
	ntv2WriteRegisterMS( context, gChannelToMRRegNum[NTV2_CHANNEL1], ulEnabled, kRegMaskMREnable, kRegShiftMREnable);
	ntv2WriteRegisterMS( context, gChannelToMRRegNum[NTV2_CHANNEL2], ulEnabled, kRegMaskMREnable, kRegShiftMREnable);
	ntv2WriteRegisterMS( context, gChannelToMRRegNum[NTV2_CHANNEL3], ulEnabled, kRegMaskMREnable, kRegShiftMREnable);
	ntv2WriteRegisterMS( context, gChannelToMRRegNum[NTV2_CHANNEL4], ulEnabled, kRegMaskMREnable, kRegShiftMREnable);
	 return ntv2WriteRegisterMS( context, kRegMROutControl, ulEnabled, kRegMaskMREnable, kRegShiftMREnable);
}

bool HasMultiRasterWidget(Ntv2SystemContext* context)
{
	ULWord hasMultiRasterWidget = 0;
	ntv2ReadRegisterMS(context, kRegMRSupport, &hasMultiRasterWidget, kRegMaskMRSupport, kRegShiftMRSupport);
	return hasMultiRasterWidget > 0 ? true : false;
}

bool IsMultiRasterEnabled(Ntv2SystemContext* context)
{
	ULWord ulEnabled = 0;
	ntv2ReadRegisterMS(context, kRegMROutControl, &ulEnabled, kRegMaskMRBypass, kRegShiftMRBypass);
	return ulEnabled == 0 ? true : false;
}

///////////////////////
//hdr routines
bool EnableHDMIHDR(Ntv2SystemContext* context, bool inEnableHDMIHDR)
{
	bool status = true;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	if (!NTV2DeviceCanDoHDMIHDROut(deviceID))
		return false;
	status = ntv2WriteRegisterMS(context, kRegHDMIHDRControl, (inEnableHDMIHDR ? 1 : 0), kRegMaskHDMIHDREnable, kRegShiftHDMIHDREnable);
	return status;
}

bool GetEnableHDMIHDR(Ntv2SystemContext* context)
{
	ULWord HDREnabled = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	if (!NTV2DeviceCanDoHDMIHDROut(deviceID))
		return false;
	ntv2ReadRegisterMS(context, kRegHDMIHDRControl, &HDREnabled, kRegMaskHDMIHDREnable, kRegShiftHDMIHDREnable);
	return HDREnabled == 1 ? true : false;
}

bool SetHDRData(Ntv2SystemContext* context, HDRDriverValues inRegisterValues)
{
	ntv2WriteRegisterMS(context, kRegHDMIHDRGreenPrimary, (ULWord)inRegisterValues.greenPrimaryX, kRegMaskHDMIHDRGreenPrimaryX, kRegShiftHDMIHDRGreenPrimaryX);
	ntv2WriteRegisterMS(context, kRegHDMIHDRGreenPrimary, (ULWord)inRegisterValues.greenPrimaryY, (ULWord)kRegMaskHDMIHDRGreenPrimaryY, kRegShiftHDMIHDRGreenPrimaryY);
	ntv2WriteRegisterMS(context, kRegHDMIHDRBluePrimary, (ULWord)inRegisterValues.bluePrimaryX, kRegMaskHDMIHDRBluePrimaryX, kRegShiftHDMIHDRBluePrimaryX);
	ntv2WriteRegisterMS(context, kRegHDMIHDRBluePrimary, (ULWord)inRegisterValues.bluePrimaryY, (ULWord)kRegMaskHDMIHDRBluePrimaryY, kRegShiftHDMIHDRBluePrimaryY);
	ntv2WriteRegisterMS(context, kRegHDMIHDRRedPrimary, (ULWord)inRegisterValues.redPrimaryX, kRegMaskHDMIHDRRedPrimaryX, kRegShiftHDMIHDRRedPrimaryX);
	ntv2WriteRegisterMS(context, kRegHDMIHDRRedPrimary, (ULWord)inRegisterValues.redPrimaryY, (ULWord)kRegMaskHDMIHDRRedPrimaryY, kRegShiftHDMIHDRRedPrimaryY);
	ntv2WriteRegisterMS(context, kRegHDMIHDRWhitePoint, (ULWord)inRegisterValues.whitePointX, kRegMaskHDMIHDRWhitePointX, kRegShiftHDMIHDRWhitePointX);
	ntv2WriteRegisterMS(context, kRegHDMIHDRWhitePoint, (ULWord)inRegisterValues.whitePointY, (ULWord)kRegMaskHDMIHDRWhitePointY, kRegShiftHDMIHDRWhitePointY);
	ntv2WriteRegisterMS(context, kRegHDMIHDRMasteringLuminence, (ULWord)inRegisterValues.maxMasteringLuminance, kRegMaskHDMIHDRMaxMasteringLuminance, kRegShiftHDMIHDRMaxMasteringLuminance);
	ntv2WriteRegisterMS(context, kRegHDMIHDRMasteringLuminence, (ULWord)inRegisterValues.minMasteringLuminance, (ULWord)kRegMaskHDMIHDRMinMasteringLuminance, kRegShiftHDMIHDRMinMasteringLuminance);
	ntv2WriteRegisterMS(context, kRegHDMIHDRLightLevel, (ULWord)inRegisterValues.maxContentLightLevel, kRegMaskHDMIHDRMaxContentLightLevel, kRegShiftHDMIHDRMaxContentLightLevel);
	ntv2WriteRegisterMS(context, kRegHDMIHDRLightLevel, (ULWord)inRegisterValues.maxFrameAverageLightLevel, (ULWord)kRegMaskHDMIHDRMaxFrameAverageLightLevel, kRegShiftHDMIHDRMaxFrameAverageLightLevel);
	ntv2WriteRegisterMS(context, kRegHDMIHDRControl, (ULWord)inRegisterValues.electroOpticalTransferFunction, kRegMaskElectroOpticalTransferFunction, kRegShiftElectroOpticalTransferFunction);
	ntv2WriteRegisterMS(context, kRegHDMIHDRControl, (ULWord)inRegisterValues.staticMetadataDescriptorID, (ULWord)kRegMaskHDRStaticMetadataDescriptorID, kRegShiftHDRStaticMetadataDescriptorID);
	ntv2WriteRegisterMS(context, kRegHDMIHDRControl, (ULWord)inRegisterValues.luminance, kRegMaskHDMIHDRNonContantLuminance, kRegShiftHDMIHDRNonContantLuminance);
	return true;
}

bool GetHDRData(Ntv2SystemContext* context, HDRDriverValues* inRegisterValues)
{
	ULWord temp = 0;
	ntv2ReadRegisterMS(context, kRegHDMIHDRGreenPrimary, &temp, kRegMaskHDMIHDRGreenPrimaryX, kRegShiftHDMIHDRGreenPrimaryX);
	inRegisterValues->greenPrimaryX = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRGreenPrimary, &temp, (ULWord)kRegMaskHDMIHDRGreenPrimaryY, kRegShiftHDMIHDRGreenPrimaryY);
	inRegisterValues->greenPrimaryY = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRBluePrimary, &temp, kRegMaskHDMIHDRBluePrimaryX, kRegShiftHDMIHDRBluePrimaryX);
	inRegisterValues->bluePrimaryX = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRBluePrimary, &temp, (ULWord)kRegMaskHDMIHDRBluePrimaryY, kRegShiftHDMIHDRBluePrimaryY);
	inRegisterValues->bluePrimaryY = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRRedPrimary, &temp, kRegMaskHDMIHDRRedPrimaryX, kRegShiftHDMIHDRRedPrimaryX);
	inRegisterValues->redPrimaryX = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRRedPrimary, &temp, (ULWord)kRegMaskHDMIHDRRedPrimaryY, kRegShiftHDMIHDRRedPrimaryY);
	inRegisterValues->redPrimaryY = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRWhitePoint, &temp, kRegMaskHDMIHDRWhitePointX, kRegShiftHDMIHDRWhitePointX);
	inRegisterValues->whitePointX = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRWhitePoint, &temp, (ULWord)kRegMaskHDMIHDRWhitePointY, kRegShiftHDMIHDRWhitePointY);
	inRegisterValues->whitePointY = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRMasteringLuminence, &temp, kRegMaskHDMIHDRMaxMasteringLuminance, kRegShiftHDMIHDRMaxMasteringLuminance);
	inRegisterValues->maxMasteringLuminance = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRMasteringLuminence, &temp, (ULWord)kRegMaskHDMIHDRMinMasteringLuminance, kRegShiftHDMIHDRMinMasteringLuminance);
	inRegisterValues->minMasteringLuminance = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRLightLevel, &temp, kRegMaskHDMIHDRMaxContentLightLevel, kRegShiftHDMIHDRMaxContentLightLevel);
	inRegisterValues->maxContentLightLevel = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRLightLevel, &temp, (ULWord)kRegMaskHDMIHDRMaxFrameAverageLightLevel, kRegShiftHDMIHDRMaxFrameAverageLightLevel);
	inRegisterValues->maxFrameAverageLightLevel = (uint16_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRControl, &temp, kRegMaskElectroOpticalTransferFunction, kRegShiftElectroOpticalTransferFunction);
	inRegisterValues->electroOpticalTransferFunction = (uint8_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRControl, &temp, (ULWord)kRegMaskHDRStaticMetadataDescriptorID, kRegShiftHDRStaticMetadataDescriptorID);
	inRegisterValues->staticMetadataDescriptorID = (uint8_t)temp;
	ntv2ReadRegisterMS(context, kRegHDMIHDRControl, &temp, kRegMaskHDMIHDRNonContantLuminance, kRegShiftHDMIHDRNonContantLuminance);
	inRegisterValues->luminance = (uint8_t)temp;
	return true;
}

///////////////////////
//input routines
bool SetLHiAnalogOutputStandard(Ntv2SystemContext* context)
{
	NTV2Standard standard = NTV2_NUM_STANDARDS;
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	NTV2OutputXptID xptSelect;
	ULWord analogOutput = 0;
	bool isQuadMode = false;
	bool isQuadQuadMode = false;
	HDRDriverValues hdrRegValues;
	NTV2LHIVideoDACMode dacMode;
	NTV2Standard dacStandard;

	if(!FindAnalogOutputSource(context, &xptSelect))
	{
		return false;
	}

	if(xptSelect == NTV2_XptConversionModule)
	{
		if(!GetConverterOutStandard(context, &standard))
		{
			return false;
		}
	}
	else
	{
		if (GetSourceVideoFormat(context, &videoFormat, xptSelect, &isQuadMode, &isQuadQuadMode, &hdrRegValues))
		{
			standard = GetNTV2StandardFromVideoFormat(videoFormat);
		}
		else
		{
			return false;
		}
	}

	analogOutput = ntv2ReadRegister(context, kRegAnalogOutControl);
	dacMode = (NTV2LHIVideoDACMode)((analogOutput & kLHIRegMaskVideoDACMode) >> kLHIRegShiftVideoDACMode);
	dacStandard = (NTV2Standard)((analogOutput & kLHIRegMaskVideoDACStandard) >> kLHIRegShiftVideoDACStandard);

	if(dacStandard != standard)
	{
		ntv2WriteRegisterMS(context, kRegAnalogOutControl, standard, kLHIRegMaskVideoDACStandard, kLHIRegShiftVideoDACStandard);
	}

	switch(standard)
	{
	case NTV2_STANDARD_1080:
		{
			if((dacMode == NTV2LHI_1080iRGB) ||
				(dacMode == NTV2LHI_1080psfRGB) ||
				(dacMode == NTV2LHI_1080iSMPTE) ||
				(dacMode == NTV2LHI_1080psfSMPTE))
			{
				return true;
			}
			ntv2WriteRegisterMS(context, kRegAnalogOutControl, dacMode, kLHIRegMaskVideoDACMode, kLHIRegShiftVideoDACMode);
			return true;
		}
	case NTV2_STANDARD_720:
		{
			if((dacMode == NTV2LHI_720pRGB) ||
				(dacMode == NTV2LHI_720pSMPTE))
			{
				return true;
			}
			ntv2WriteRegisterMS(context, kRegAnalogOutControl, dacMode, kLHIRegMaskVideoDACMode, kLHIRegShiftVideoDACMode);
			return true;
		}
	case NTV2_STANDARD_525: 
		{
			if((dacMode == NTV2LHI_480iRGB) ||
				(dacMode == NTV2LHI_480iYPbPrSMPTE) ||
				(dacMode == NTV2LHI_480iYPbPrBetacam525) ||
				(dacMode == NTV2LHI_480iYPbPrBetacamJapan) ||
				(dacMode == NTV2LHI_480iNTSC_US_Composite) ||
				(dacMode == NTV2LHI_480iNTSC_Japan_Composite))
			{
				return true;
			}
			ntv2WriteRegisterMS(context, kRegAnalogOutControl, dacMode, kLHIRegMaskVideoDACMode, kLHIRegShiftVideoDACMode);
			return true;
		}
	case NTV2_STANDARD_625:
		{
			if((dacMode == NTV2LHI_576iRGB) ||
				(dacMode == NTV2LHI_576iYPbPrSMPTE) ||
				(dacMode == NTV2LHI_576iPAL_Composite))
			{
				return true;
			}
			ntv2WriteRegisterMS(context, kRegAnalogOutControl, dacMode, kLHIRegMaskVideoDACMode, kLHIRegShiftVideoDACMode);
			return true;
		}
	default:
		break;
	}

	return false;
}


///////////////////////
//input routines
bool GetSourceVideoFormat(Ntv2SystemContext* context, NTV2VideoFormat* format, NTV2OutputXptID crosspoint, bool* quadMode, bool* quadQuadMode, HDRDriverValues* hdrRegValues)
{	
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	NTV2VideoFormat shadowFormat = NTV2_FORMAT_UNKNOWN;
	NTV2Channel multiFormatModeChannel = NTV2_CHANNEL1;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	bool multiFormatActive = IsMultiFormatActive(context);
	memset(hdrRegValues, 0, sizeof(HDRDriverValues));

	switch(crosspoint)
	{
	case NTV2_XptSDIIn1:
	case NTV2_XptSDIIn1DS2:
	case NTV2_XptSDIIn2:
	case NTV2_XptSDIIn2DS2:
	case NTV2_XptSDIIn3:
	case NTV2_XptSDIIn3DS2:
	case NTV2_XptSDIIn4:
	case NTV2_XptSDIIn4DS2:
	case NTV2_XptSDIIn5:
	case NTV2_XptSDIIn5DS2:
	case NTV2_XptSDIIn6:
	case NTV2_XptSDIIn6DS2:
	case NTV2_XptSDIIn7:
	case NTV2_XptSDIIn7DS2:
	case NTV2_XptSDIIn8:
	case NTV2_XptSDIIn8DS2:
		videoFormat = GetInputVideoFormat(context, GetOutXptChannel(crosspoint, multiFormatActive));
		ReadFSHDRRegValues(context, NTV2_CHANNEL1, hdrRegValues);
		*quadMode = (GetSDIIn6GEnable(context, GetOutXptChannel(crosspoint, multiFormatActive)) || GetSDIIn12GEnable(context, GetOutXptChannel(crosspoint, multiFormatActive))) ? true : false;
		break;
	case NTV2_XptHDMIIn1:
	case NTV2_XptHDMIIn1Q2:
	case NTV2_XptHDMIIn1Q3:
	case NTV2_XptHDMIIn1Q4:
	case NTV2_XptHDMIIn1RGB:
	case NTV2_XptHDMIIn1Q2RGB:
	case NTV2_XptHDMIIn1Q3RGB:
	case NTV2_XptHDMIIn1Q4RGB:
		videoFormat = GetHDMIInputVideoFormat(context);
		*quadMode = NTV2_IS_QUAD_FRAME_FORMAT(videoFormat) && NTV2DeviceCanDo12gRouting(deviceID);
		ReadFSHDRRegValues(context, NTV2_CHANNEL1, hdrRegValues);
		break;
	case NTV2_XptAnalogIn:
		videoFormat = GetAnalogInputVideoFormat(context);
		break;
	case NTV2_XptDuallinkIn1:
	case NTV2_XptDuallinkIn2:
	case NTV2_XptDuallinkIn3:
	case NTV2_XptDuallinkIn4:
	case NTV2_XptDuallinkIn5:
	case NTV2_XptDuallinkIn6:
	case NTV2_XptDuallinkIn7:
	case NTV2_XptDuallinkIn8:
		videoFormat = GetInputVideoFormat(context, GetOutXptChannel(crosspoint, multiFormatActive));
		ReadFSHDRRegValues(context, NTV2_CHANNEL1, hdrRegValues);
		break;
	case NTV2_XptFrameBuffer1YUV:
	case NTV2_XptFrameBuffer1RGB:
	case NTV2_XptFrameBuffer2YUV:
	case NTV2_XptFrameBuffer2RGB:
	case NTV2_XptFrameBuffer3YUV:
	case NTV2_XptFrameBuffer3RGB:
	case NTV2_XptFrameBuffer4YUV:
	case NTV2_XptFrameBuffer4RGB:
	case NTV2_XptFrameBuffer5YUV:
	case NTV2_XptFrameBuffer5RGB:
	case NTV2_XptFrameBuffer6YUV:
	case NTV2_XptFrameBuffer6RGB:
	case NTV2_XptFrameBuffer7YUV:
	case NTV2_XptFrameBuffer7RGB:
	case NTV2_XptFrameBuffer8YUV:
	case NTV2_XptFrameBuffer8RGB:
	case NTV2_XptFrameBuffer1_DS2YUV:
	case NTV2_XptFrameBuffer1_DS2RGB:
	case NTV2_XptFrameBuffer2_DS2YUV:
	case NTV2_XptFrameBuffer2_DS2RGB:
	case NTV2_XptFrameBuffer3_DS2YUV:
	case NTV2_XptFrameBuffer3_DS2RGB:
	case NTV2_XptFrameBuffer4_DS2YUV:
	case NTV2_XptFrameBuffer4_DS2RGB:
	case NTV2_XptFrameBuffer5_DS2YUV:
	case NTV2_XptFrameBuffer5_DS2RGB:
	case NTV2_XptFrameBuffer6_DS2YUV:
	case NTV2_XptFrameBuffer6_DS2RGB:
	case NTV2_XptFrameBuffer7_DS2YUV:
	case NTV2_XptFrameBuffer7_DS2RGB:
	case NTV2_XptFrameBuffer8_DS2YUV:
	case NTV2_XptFrameBuffer8_DS2RGB:
		multiFormatModeChannel = GetOutXptChannel(crosspoint, multiFormatActive);
		videoFormat = GetBoardVideoFormat(context, multiFormatModeChannel);
		shadowFormat = (NTV2VideoFormat)ntv2ReadVirtualRegister(context, gShadowRegs[multiFormatModeChannel]);
		if (!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(videoFormat) && NTV2_IS_PSF_VIDEO_FORMAT(shadowFormat))
		{
			videoFormat = shadowFormat;
		}
		*quadMode = GetQuadFrameEnable(context, multiFormatModeChannel);
		*quadQuadMode = GetQuadQuadFrameEnable(context, multiFormatModeChannel);
		ReadFSHDRRegValues(context, multiFormatModeChannel, hdrRegValues);
		break;
	case NTV2_XptBlack:
	case NTV2_Xpt4KDownConverterOut:
	case NTV2_Xpt4KDownConverterOutRGB:
		videoFormat = GetBoardVideoFormat(context, NTV2_CHANNEL1);
		break;
	default:
		return false;
	};

	if(format != NULL)
	{
		*format = videoFormat;
	}

	return true;
}

NTV2VideoFormat GetInputVideoFormat(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2FrameRate frameRate;
	NTV2Standard standard;
	NTV2ScanGeometry scanGeometry;
	ULWord inputStatus = 0;
	ULWord input3GStatus = 0;
	ULWord progressive = 0;
	ULWord is2Kx1080 = 0;
	ULWord is3Gb = 0;

	ULWord regNum = gChannelToInputStatesRegs[channel];
	ULWord rateMask = gChannelToInputStatusRateMasks[channel];
	ULWord rateShift = gChannelToInputStatusRateShifts[channel];
	ULWord rateHighShift = gChannelToInputStatusRateHighShifts[channel];
	ULWord rateHighMask = gChannelToInputStatusRateHighMasks[channel];
	ULWord scanMask = gChannelToScanMasks[channel];
	ULWord scanShift = gChannelToScanShifts[channel];
	ULWord scanHighMask = gChannelToScanHighMasks[channel];
	ULWord scanHighShift = gChannelToScanHighShifts[channel];
	ULWord progressiveMask = gChannelToProgressiveMasks[channel];
	ULWord progressiveShift = gChannelToProgressiveShifts[channel];
	ULWord regNum3g = gChannelToSDIInput3GStatusRegNum[channel];
	ULWord mask3g = gChannelToLevelBMasks[channel];
	ULWord shift3g = gChannelToLevelBShifts[channel];
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);


	inputStatus = ntv2ReadRegister(context, regNum);
	frameRate = (NTV2FrameRate)(((inputStatus & rateMask) >> rateShift) |
		((inputStatus & rateHighMask) >> rateHighShift << 3));
	scanGeometry = (NTV2ScanGeometry)(((inputStatus & scanMask) >> scanShift) |
		((inputStatus & scanHighMask) >> scanHighShift << 3));
	progressive = (inputStatus & progressiveMask) >> progressiveShift;

	standard = GetStandardFromScanGeometry(scanGeometry, progressive);
	is2Kx1080 = IsScanGeometry2Kx1080(scanGeometry);

	if(NTV2DeviceCanDo3GIn(deviceID, (UWord)channel) || NTV2DeviceCanDo12GIn(deviceID, (UWord)channel))
	{
		input3GStatus = ntv2ReadRegister(context, regNum3g);
		if (NTV2DeviceCanDo3GLevelConversion(deviceID))
		{
			bool bBToAEnabled = false;
			GetSDIInLevelBtoLevelAConversion(context, channel, &bBToAEnabled);
			is3Gb = bBToAEnabled ? false : (input3GStatus & mask3g) >> shift3g;
			standard = bBToAEnabled ? GetStandardFromScanGeometry(scanGeometry, true) : standard;
			if (bBToAEnabled)
			{
				switch (frameRate)
				{
					case NTV2_FRAMERATE_3000:
						frameRate = NTV2_FRAMERATE_6000;
						break;
					case NTV2_FRAMERATE_2997:
						frameRate = NTV2_FRAMERATE_5994;
						break;
					case NTV2_FRAMERATE_2500:
						frameRate = NTV2_FRAMERATE_5000;
						break;
					case NTV2_FRAMERATE_2400:
						frameRate = NTV2_FRAMERATE_4800;
						break;
					case NTV2_FRAMERATE_2398:
						frameRate = NTV2_FRAMERATE_4795;
						break;
					default:
						break;
				}
			}
		}
		else
			is3Gb = (input3GStatus & mask3g) >> shift3g;
	}

	return GetVideoFormatFromState(standard, frameRate, is2Kx1080, is3Gb);
}

NTV2VideoFormat GetHDMIInputVideoFormat(Ntv2SystemContext* context)
{
	NTV2Standard standard;
	NTV2Standard v2Standard;
	NTV2FrameRate frameRate;
	NTV2VideoFormat format;
	ULWord status, is2kx1080;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	is2kx1080 = 0;

	if(NTV2DeviceCanDoInputSource(deviceID, NTV2_INPUTSOURCE_HDMI1))
	{
		ULWord hdmiVersion = NTV2DeviceGetHDMIVersion(deviceID);
		status = ntv2ReadRegister(context, kRegHDMIInputStatus);

		if(hdmiVersion == 1)
		{
			if((status & (BIT_0 | BIT_1)) != (BIT_0 | BIT_1))
			{
				return NTV2_FORMAT_UNKNOWN;
			}
			standard = (NTV2Standard)((status & kRegMaskInputStatusStd) >> kRegShiftInputStatusStd);
			frameRate = (NTV2FrameRate)((status & kRegMaskInputStatusFPS) >> kRegShiftInputStatusFPS);
			return GetVideoFormatFromState(standard, frameRate, 0, 0);
		}
		else if(hdmiVersion >= 2)
		{
			if((status & (BIT_0)) != (BIT_0))
			{
				return NTV2_FORMAT_UNKNOWN;
			}
			frameRate = (NTV2FrameRate)((status &kRegMaskInputStatusFPS) >> kRegShiftInputStatusFPS);
			v2Standard = (NTV2Standard)((status & kRegMaskHDMIInV2VideoStd) >> kRegShiftHDMIInV2VideoStd);
			//ntv2Message("GHI rate = %d, standard = %d", frameRate, v2Standard);
			switch(v2Standard)
			{
			case NTV2_STANDARD_1080:
				standard = NTV2_STANDARD_1080;
				break;
			case NTV2_STANDARD_720:
				standard = NTV2_STANDARD_720;
				break;
			case NTV2_STANDARD_525:
				standard = NTV2_STANDARD_525;
				break;
			case NTV2_STANDARD_625:
				standard = NTV2_STANDARD_625;
				break;
			case NTV2_STANDARD_1080p:
			case NTV2_STANDARD_3840x2160p:
				standard = NTV2_STANDARD_1080p;
				break;
			case NTV2_STANDARD_2Kx1080p:
			case NTV2_STANDARD_4096x2160p:
				standard = NTV2_STANDARD_1080p;
				is2kx1080 = 1;
				break;
			default:
				return NTV2_FORMAT_UNKNOWN;
			}

			format = GetVideoFormatFromState(standard, frameRate, is2kx1080, 0);
			if(NTV2_IS_QUAD_STANDARD(v2Standard))
				format = GetQuadSizedVideoFormat(format);
			return format;
		}
		else
			return NTV2_FORMAT_UNKNOWN;
	}
	else
	{
		return NTV2_FORMAT_UNKNOWN;
	}
}

NTV2VideoFormat GetAnalogInputVideoFormat(Ntv2SystemContext* context)
{
	NTV2Standard standard;
	NTV2FrameRate frameRate;
	ULWord status = 0;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(NTV2DeviceCanDoInputSource(deviceID, NTV2_INPUTSOURCE_ANALOG1))
	{
		status = ntv2ReadRegister(context, kRegAnalogInputStatus);
		if((status & kRegMaskInputStatusLock) == 0)
		{
			return NTV2_FORMAT_UNKNOWN;
		}

		standard = (NTV2Standard)((status & kRegMaskInputStatusStd) >> kRegShiftInputStatusStd);
		frameRate = (NTV2FrameRate)((status & kRegMaskInputStatusFPS) >> kRegShiftInputStatusFPS);

		return GetVideoFormatFromState(standard, frameRate, 0, 0);
	}

	return NTV2_FORMAT_UNKNOWN;
}

///////////////////////
//converter routines
bool GetK2ConverterOutFormat(Ntv2SystemContext* context, NTV2VideoFormat* format)
{
	NTV2VideoFormat outFormat = NTV2_FORMAT_UNKNOWN;
	NTV2Standard standard = NTV2_NUM_STANDARDS;
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	NTV2OutputXptID xptSelect;
	bool isQuadMode = false;
	bool isQuadQuadMode = false;
	HDRDriverValues hdrRegValues;

	if(!FindCrosspointSource(context, &xptSelect, NTV2_XptConversionModule))
	{
		return false;
	}
	if (!GetSourceVideoFormat(context, &videoFormat, xptSelect, &isQuadMode, &isQuadQuadMode, &hdrRegValues))
	{
		return false;
	}

	if(!GetConverterOutStandard(context, &standard))
	{
		return false;
	}

	switch(videoFormat)
	{
	case NTV2_FORMAT_1080i_6000:
	case NTV2_FORMAT_1080p_6000_A:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_720p_6000:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080i_6000;
			break;
		case NTV2_STANDARD_720:
			outFormat = NTV2_FORMAT_720p_6000;
			break;
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_6000_B;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_525_5994:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080i_5994;
			break;
		case NTV2_STANDARD_720:
			outFormat = NTV2_FORMAT_720p_5994;
			break;
		case NTV2_STANDARD_525:
			outFormat = NTV2_FORMAT_525_5994;
			break;
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_5994_B;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_625_5000:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080i_5000;
			break;
		case NTV2_STANDARD_720:
			outFormat = NTV2_FORMAT_720p_5000;
			break;
		case NTV2_STANDARD_625:
			outFormat = NTV2_FORMAT_625_5000;
			break;
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_5000_B;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2K_2400:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080psf_2400;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080psf_2K_2400;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080psf_2K_2398:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080psf_2398;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080psf_2K_2398;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_2K_1500:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_3000;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_2K_1500;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_2K_1498:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_2997;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_2K_1498;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_2500:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_2500;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080p_2K_2500;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2K_2400:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_2400;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080p_2K_2400;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2K_2398:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_2398;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080p_2K_2398;
			break;
		default:
			return false;
		}
		break;
	default:
		return false;
	}

	if(format != NULL)
	{
		*format = outFormat;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////
//util routines
ULWord IsScanGeometry2Kx1080(NTV2ScanGeometry scanGeometry)
{
	switch (scanGeometry)
	{
	case NTV2_SG_2Kx1080:
		return 1;
	default:
		break;
	}

	return 0;
}

bool IsVideoFormat2Kx1080(NTV2VideoFormat videoFormat)

{

	if (NTV2_IS_2K_1080_VIDEO_FORMAT(videoFormat) || NTV2_IS_4K_4096_VIDEO_FORMAT(videoFormat) || NTV2_IS_UHD2_FULL_VIDEO_FORMAT(videoFormat))
		return true;
	else
		return false;

}

NTV2Crosspoint GetNTV2CrosspointChannelForIndex(ULWord index)
{
	switch(index)
	{
	default:
	case 0:	return NTV2CROSSPOINT_CHANNEL1;
	case 1:	return NTV2CROSSPOINT_CHANNEL2;
	case 2:	return NTV2CROSSPOINT_CHANNEL3;
	case 3:	return NTV2CROSSPOINT_CHANNEL4;
	case 4: return NTV2CROSSPOINT_CHANNEL5;
	case 5: return NTV2CROSSPOINT_CHANNEL6;
	case 6: return NTV2CROSSPOINT_CHANNEL7;
	case 7: return NTV2CROSSPOINT_CHANNEL8;
	}
}

ULWord GetIndexForNTV2CrosspointChannel(NTV2Crosspoint channel)
{
	switch(channel)
	{
	default:
	case NTV2CROSSPOINT_CHANNEL1:	return 0;
	case NTV2CROSSPOINT_CHANNEL2:	return 1;
	case NTV2CROSSPOINT_CHANNEL3:	return 2;
	case NTV2CROSSPOINT_CHANNEL4:	return 3;
	case NTV2CROSSPOINT_CHANNEL5:	return 4;
	case NTV2CROSSPOINT_CHANNEL6:	return 5;
	case NTV2CROSSPOINT_CHANNEL7:	return 6;
	case NTV2CROSSPOINT_CHANNEL8:	return 7;
	}
}

NTV2Crosspoint GetNTV2CrosspointInputForIndex(ULWord index)
{
	switch(index)
	{
	default:
	case 0:	return NTV2CROSSPOINT_INPUT1;
	case 1:	return NTV2CROSSPOINT_INPUT2;
	case 2:	return NTV2CROSSPOINT_INPUT3;
	case 3:	return NTV2CROSSPOINT_INPUT4;
	case 4:	return NTV2CROSSPOINT_INPUT5;
	case 5:	return NTV2CROSSPOINT_INPUT6;
	case 6:	return NTV2CROSSPOINT_INPUT7;
	case 7:	return NTV2CROSSPOINT_INPUT8;
	}
}

ULWord GetIndexForNTV2CrosspointInput(NTV2Crosspoint channel)
{
	switch(channel)
	{
	default:
	case NTV2CROSSPOINT_INPUT1:	return 0;
	case NTV2CROSSPOINT_INPUT2:	return 1;
	case NTV2CROSSPOINT_INPUT3:	return 2;
	case NTV2CROSSPOINT_INPUT4:	return 3;
	case NTV2CROSSPOINT_INPUT5:	return 4;
	case NTV2CROSSPOINT_INPUT6:	return 5;
	case NTV2CROSSPOINT_INPUT7:	return 6;
	case NTV2CROSSPOINT_INPUT8:	return 7;
	}
}

NTV2Crosspoint GetNTV2CrosspointForIndex(ULWord index)
{
	switch(index)
	{
	default:
	case 0:	return NTV2CROSSPOINT_CHANNEL1;
	case 1:	return NTV2CROSSPOINT_CHANNEL2;
	case 2:	return NTV2CROSSPOINT_CHANNEL3;
	case 3:	return NTV2CROSSPOINT_CHANNEL4;
	case 4:	return NTV2CROSSPOINT_INPUT1;
	case 5:	return NTV2CROSSPOINT_INPUT2;
	case 6:	return NTV2CROSSPOINT_INPUT3;
	case 7:	return NTV2CROSSPOINT_INPUT4;
	case 8:	return NTV2CROSSPOINT_CHANNEL5;
	case 9:	return NTV2CROSSPOINT_CHANNEL6;
	case 10: return NTV2CROSSPOINT_CHANNEL7;
	case 11: return NTV2CROSSPOINT_CHANNEL8;
	case 12: return NTV2CROSSPOINT_INPUT5;
	case 13: return NTV2CROSSPOINT_INPUT6;
	case 14: return NTV2CROSSPOINT_INPUT7;
	case 15: return NTV2CROSSPOINT_INPUT8;
	}
}

ULWord GetIndexForNTV2Crosspoint(NTV2Crosspoint channel)
{
	switch(channel)
	{
	default:
	case NTV2CROSSPOINT_CHANNEL1:	return 0;
	case NTV2CROSSPOINT_CHANNEL2:	return 1;
	case NTV2CROSSPOINT_CHANNEL3:	return 2;
	case NTV2CROSSPOINT_CHANNEL4:	return 3;
	case NTV2CROSSPOINT_INPUT1:		return 4;
	case NTV2CROSSPOINT_INPUT2:		return 5;
	case NTV2CROSSPOINT_INPUT3:		return 6;
	case NTV2CROSSPOINT_INPUT4:		return 7;
	case NTV2CROSSPOINT_CHANNEL5:	return 8;
	case NTV2CROSSPOINT_CHANNEL6:	return 9;
	case NTV2CROSSPOINT_CHANNEL7:	return 10;
	case NTV2CROSSPOINT_CHANNEL8:	return 11;
	case NTV2CROSSPOINT_INPUT5:		return 12;
	case NTV2CROSSPOINT_INPUT6:		return 13;
	case NTV2CROSSPOINT_INPUT7:		return 14;
	case NTV2CROSSPOINT_INPUT8:		return 15;
	}
}

NTV2Channel GetNTV2ChannelForNTV2Crosspoint(NTV2Crosspoint crosspoint)
{
	NTV2Channel channel;
	switch(crosspoint)
	{
	default:
	case NTV2CROSSPOINT_CHANNEL1:
	case NTV2CROSSPOINT_INPUT1:
		channel = NTV2_CHANNEL1;
		break;
	case NTV2CROSSPOINT_CHANNEL2:
	case NTV2CROSSPOINT_INPUT2:
		channel = NTV2_CHANNEL2;
		break;
	case NTV2CROSSPOINT_CHANNEL3:
	case NTV2CROSSPOINT_INPUT3:
		channel = NTV2_CHANNEL3;
		break;
	case NTV2CROSSPOINT_CHANNEL4:
	case NTV2CROSSPOINT_INPUT4:
		channel = NTV2_CHANNEL4;
		break;
	case NTV2CROSSPOINT_CHANNEL5:
	case NTV2CROSSPOINT_INPUT5:
		channel = NTV2_CHANNEL5;
		break;
	case NTV2CROSSPOINT_CHANNEL6:
	case NTV2CROSSPOINT_INPUT6:
		channel = NTV2_CHANNEL6;
		break;
	case NTV2CROSSPOINT_CHANNEL7:
	case NTV2CROSSPOINT_INPUT7:
		channel = NTV2_CHANNEL7;
		break;
	case NTV2CROSSPOINT_CHANNEL8:
	case NTV2CROSSPOINT_INPUT8:
		channel = NTV2_CHANNEL8;
		break;
	}
	return channel;
}

NTV2VideoFormat GetVideoFormatFromState(NTV2Standard standard,
									NTV2FrameRate frameRate,
									ULWord is2Kx1080,
									ULWord smpte372Enabled)
{
	NTV2VideoFormat format = NTV2_FORMAT_UNKNOWN;

	switch (standard)
	{
	case NTV2_STANDARD_1080:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_3000:
			if (smpte372Enabled)
				format = NTV2_FORMAT_1080p_6000_B;
			else
				format = NTV2_FORMAT_1080i_6000;
			break;
		case NTV2_FRAMERATE_2997:
			if (smpte372Enabled)
				format = NTV2_FORMAT_1080p_5994_B;
			else
				format = NTV2_FORMAT_1080i_5994;
			break;
		case NTV2_FRAMERATE_2500:
			if (smpte372Enabled)
				format = NTV2_FORMAT_1080p_5000_B;
			else if (is2Kx1080)
				format = NTV2_FORMAT_1080psf_2K_2500;
			else
				format = NTV2_FORMAT_1080i_5000;
			break;
		case NTV2_FRAMERATE_2400:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080psf_2K_2400;
			else
				format = NTV2_FORMAT_1080psf_2400;
			break;
		case NTV2_FRAMERATE_2398:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080psf_2K_2398;
			else
				format = NTV2_FORMAT_1080psf_2398;
			break;
		default:
			break;
		}
		break;

	case NTV2_STANDARD_1080p:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_3000:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_3000;
			else
				format = NTV2_FORMAT_1080p_3000;
			break;
		case NTV2_FRAMERATE_2997:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_2997;
			else
				format = NTV2_FORMAT_1080p_2997;
			break;
		case NTV2_FRAMERATE_2500:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_2500;
			else
				format = NTV2_FORMAT_1080p_2500;
			break;
		case NTV2_FRAMERATE_2400:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_2400;
			else
				format = NTV2_FORMAT_1080p_2400;
			break;
		case NTV2_FRAMERATE_2398:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_2398;
			else
				format = NTV2_FORMAT_1080p_2398;
			break;
		case NTV2_FRAMERATE_4795:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_4795_A;
			else
				format = NTV2_FORMAT_UNKNOWN;
			break;
		case NTV2_FRAMERATE_4800:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_4800_A;
			else
				format = NTV2_FORMAT_UNKNOWN;
			break;
		case NTV2_FRAMERATE_5000:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_5000_A;
			else
				format = NTV2_FORMAT_1080p_5000_A;
			break;
		case NTV2_FRAMERATE_5994:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_5994_A;
			else
				format = NTV2_FORMAT_1080p_5994_A;
			break;
		case NTV2_FRAMERATE_6000:
			if (is2Kx1080)
				format = NTV2_FORMAT_1080p_2K_6000_A;
			else
				format = NTV2_FORMAT_1080p_6000_A;
			break;
		default:
			break;
		}
		break;

	case NTV2_STANDARD_720:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_6000:
			format = NTV2_FORMAT_720p_6000;
			break;
		case NTV2_FRAMERATE_5994:
			format = NTV2_FORMAT_720p_5994;
			break;
		case NTV2_FRAMERATE_5000:
			format = NTV2_FORMAT_720p_5000;
			break;
		case NTV2_FRAMERATE_2398:
			format = NTV2_FORMAT_720p_2398;
			break;
		default:
			break;;
		}
		break;

	case NTV2_STANDARD_525:
		switch ( frameRate )
		{
		case NTV2_FRAMERATE_2997:
			format = NTV2_FORMAT_525_5994;
			break;
		case NTV2_FRAMERATE_2400:
			format = NTV2_FORMAT_525_2400;
			break;
		case NTV2_FRAMERATE_2398:
			format = NTV2_FORMAT_525_2398;
			break;
		default:
			break;
		}
		break;

	case NTV2_STANDARD_625:
		format = NTV2_FORMAT_625_5000;
		break;

	case NTV2_STANDARD_2K:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_1498:
			format = NTV2_FORMAT_2K_1498;
			break;
		case NTV2_FRAMERATE_1500:
			format = NTV2_FORMAT_2K_1500;
			break;
		case NTV2_FRAMERATE_2398:
			format = NTV2_FORMAT_2K_2398;
			break;
		case NTV2_FRAMERATE_2400:
			format = NTV2_FORMAT_2K_2400;
			break;
		default:
			break;
		}
		break;
	default: 
		break;;
	}

	return format;
}

NTV2Standard GetNTV2StandardFromVideoFormat(NTV2VideoFormat videoFormat)
{
	NTV2Standard standard = NTV2_STANDARD_525;

	switch(videoFormat)
	{
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080i_6000:
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2K_2398:
	case NTV2_FORMAT_1080psf_2K_2400:
	case NTV2_FORMAT_1080psf_2K_2500:
	case NTV2_FORMAT_1080psf_2500_2:
	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_1080p_2K_5000_B:
	case NTV2_FORMAT_1080p_2K_5994_B:
	case NTV2_FORMAT_1080p_2K_6000_B:
	case NTV2_FORMAT_4x1920x1080psf_2398:
	case NTV2_FORMAT_4x1920x1080psf_2400:
	case NTV2_FORMAT_4x1920x1080psf_2500:
	case NTV2_FORMAT_4x1920x1080psf_2997:
	case NTV2_FORMAT_4x1920x1080psf_3000:
	case NTV2_FORMAT_4x2048x1080psf_2398:
	case NTV2_FORMAT_4x2048x1080psf_2400:
	case NTV2_FORMAT_4x2048x1080psf_2500:
	case NTV2_FORMAT_4x2048x1080psf_2997:
	case NTV2_FORMAT_4x2048x1080psf_3000:
		standard = NTV2_STANDARD_1080;
		break;
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2K_2398:
	case NTV2_FORMAT_1080p_2K_2400:
	case NTV2_FORMAT_1080p_2K_2500:
	case NTV2_FORMAT_1080p_2K_2997:
	case NTV2_FORMAT_1080p_2K_3000:
	case NTV2_FORMAT_1080p_2K_4795_A:
	case NTV2_FORMAT_1080p_2K_4800_A:
	case NTV2_FORMAT_1080p_2K_5000_A:
	case NTV2_FORMAT_1080p_2K_5994_A:
	case NTV2_FORMAT_1080p_2K_6000_A:
	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_6000_A:
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_4x1920x1080p_2400:
	case NTV2_FORMAT_4x1920x1080p_2500:
	case NTV2_FORMAT_4x1920x1080p_2997:
	case NTV2_FORMAT_4x1920x1080p_3000:
	case NTV2_FORMAT_4x2048x1080p_2398:
	case NTV2_FORMAT_4x2048x1080p_2400:
	case NTV2_FORMAT_4x2048x1080p_2500:
	case NTV2_FORMAT_4x2048x1080p_2997:
	case NTV2_FORMAT_4x2048x1080p_3000:
	case NTV2_FORMAT_4x2048x1080p_4795:
	case NTV2_FORMAT_4x2048x1080p_4800:
	case NTV2_FORMAT_4x1920x1080p_5000:
	case NTV2_FORMAT_4x1920x1080p_5994:
	case NTV2_FORMAT_4x1920x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_5000:
	case NTV2_FORMAT_4x2048x1080p_5994:
	case NTV2_FORMAT_4x2048x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_11988:
	case NTV2_FORMAT_4x2048x1080p_12000:
		standard = NTV2_STANDARD_1080p;
		break;
	case NTV2_FORMAT_720p_2398:
	case NTV2_FORMAT_720p_2500:
	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_720p_6000:
		standard = NTV2_STANDARD_720;
		break;
	case NTV2_FORMAT_525_2398:
	case NTV2_FORMAT_525_2400:
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_525psf_2997:
		standard = NTV2_STANDARD_525;
		break;
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_625psf_2500:
		standard = NTV2_STANDARD_625;
		break;
	case NTV2_FORMAT_2K_2500:
	case NTV2_FORMAT_2K_2400:
	case NTV2_FORMAT_2K_2398:
	case NTV2_FORMAT_2K_1498:
	case NTV2_FORMAT_2K_1500:
		standard = NTV2_STANDARD_2K;
		break;
	default:
		break;	// Unsupported
	}

	return standard;
}

NTV2FrameRate GetNTV2FrameRateFromVideoFormat(NTV2VideoFormat videoFormat)
{
    NTV2FrameRate frameRate = NTV2_FRAMERATE_UNKNOWN;
	switch ( videoFormat )
	{
	
	case NTV2_FORMAT_2K_1498:
		frameRate = NTV2_FRAMERATE_1498;
		break;
		
	case NTV2_FORMAT_2K_1500:
		frameRate = NTV2_FRAMERATE_1500;
		break;
	
	case NTV2_FORMAT_525_2398:
	case NTV2_FORMAT_720p_2398:
	case NTV2_FORMAT_1080psf_2K_2398:
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2K_2398:
	case NTV2_FORMAT_2K_2398:
	case NTV2_FORMAT_4x1920x1080psf_2398:
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_4x2048x1080psf_2398:
	case NTV2_FORMAT_4x2048x1080p_2398:
	case NTV2_FORMAT_4x2048x1080p_4795_B:
	case NTV2_FORMAT_3840x2160p_2398:
	case NTV2_FORMAT_3840x2160psf_2398:
	case NTV2_FORMAT_4096x2160p_2398:
	case NTV2_FORMAT_4096x2160psf_2398:
	case NTV2_FORMAT_4096x2160p_4795_B:
	case NTV2_FORMAT_4x3840x2160p_2398:
	case NTV2_FORMAT_4x4096x2160p_2398:
	case NTV2_FORMAT_4x4096x2160p_4795_B:
		frameRate = NTV2_FRAMERATE_2398;
		break;
		
	case NTV2_FORMAT_525_2400:
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2K_2400:
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2K_2400:
	case NTV2_FORMAT_2K_2400:
	case NTV2_FORMAT_4x1920x1080psf_2400:
	case NTV2_FORMAT_4x1920x1080p_2400:
	case NTV2_FORMAT_4x2048x1080psf_2400:
	case NTV2_FORMAT_4x2048x1080p_2400:
	case NTV2_FORMAT_4x2048x1080p_4800_B:
	case NTV2_FORMAT_3840x2160p_2400:
	case NTV2_FORMAT_3840x2160psf_2400:
	case NTV2_FORMAT_4096x2160p_2400:
	case NTV2_FORMAT_4096x2160psf_2400:
	case NTV2_FORMAT_4096x2160p_4800_B:
	case NTV2_FORMAT_4x3840x2160p_2400:
	case NTV2_FORMAT_4x4096x2160p_2400:
	case NTV2_FORMAT_4x4096x2160p_4800_B:
		frameRate = NTV2_FRAMERATE_2400;
		break;
		
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_625psf_2500:
	case NTV2_FORMAT_720p_2500:
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080psf_2500_2:
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_1080psf_2K_2500:
	case NTV2_FORMAT_1080p_2K_2500:
	case NTV2_FORMAT_2K_2500:
	case NTV2_FORMAT_4x1920x1080psf_2500:
	case NTV2_FORMAT_4x1920x1080p_2500:
	case NTV2_FORMAT_4x1920x1080p_5000_B:
	case NTV2_FORMAT_4x2048x1080psf_2500:
	case NTV2_FORMAT_4x2048x1080p_2500:
	case NTV2_FORMAT_4x2048x1080p_5000_B:
	case NTV2_FORMAT_3840x2160psf_2500:
	case NTV2_FORMAT_3840x2160p_2500:
	case NTV2_FORMAT_3840x2160p_5000_B:
	case NTV2_FORMAT_4096x2160psf_2500:
	case NTV2_FORMAT_4096x2160p_2500:
	case NTV2_FORMAT_4096x2160p_5000_B:
	case NTV2_FORMAT_4x3840x2160p_2500:
	case NTV2_FORMAT_4x3840x2160p_5000_B:
	case NTV2_FORMAT_4x4096x2160p_2500:
	case NTV2_FORMAT_4x4096x2160p_5000_B:
		frameRate = NTV2_FRAMERATE_2500;
		break;
	
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_525psf_2997:
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_1080p_2K_2997:
	case NTV2_FORMAT_4x1920x1080p_2997:
	case NTV2_FORMAT_4x1920x1080psf_2997:
	case NTV2_FORMAT_4x1920x1080p_5994_B:
	case NTV2_FORMAT_4x2048x1080p_2997:
	case NTV2_FORMAT_4x2048x1080psf_2997:
	case NTV2_FORMAT_4x2048x1080p_5994_B:
	case NTV2_FORMAT_3840x2160p_2997:
	case NTV2_FORMAT_3840x2160psf_2997:
	case NTV2_FORMAT_3840x2160p_5994_B:
	case NTV2_FORMAT_4096x2160p_2997:
	case NTV2_FORMAT_4096x2160psf_2997:
	case NTV2_FORMAT_4096x2160p_5994_B:
	case NTV2_FORMAT_4x3840x2160p_2997:
	case NTV2_FORMAT_4x3840x2160p_5994_B:
	case NTV2_FORMAT_4x4096x2160p_2997:
	case NTV2_FORMAT_4x4096x2160p_5994_B:
		frameRate = NTV2_FRAMERATE_2997;
		break;
		
	case NTV2_FORMAT_1080i_6000:
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080p_2K_3000:
	case NTV2_FORMAT_4x1920x1080p_3000:
	case NTV2_FORMAT_4x1920x1080psf_3000:
	case NTV2_FORMAT_4x1920x1080p_6000_B:
	case NTV2_FORMAT_4x2048x1080p_3000:
	case NTV2_FORMAT_4x2048x1080psf_3000:
	case NTV2_FORMAT_4x2048x1080p_6000_B:
	case NTV2_FORMAT_3840x2160p_3000:
	case NTV2_FORMAT_3840x2160psf_3000:
	case NTV2_FORMAT_3840x2160p_6000_B:
	case NTV2_FORMAT_4096x2160p_3000:
	case NTV2_FORMAT_4096x2160psf_3000:
	case NTV2_FORMAT_4096x2160p_6000_B:
	case NTV2_FORMAT_4x3840x2160p_3000:
	case NTV2_FORMAT_4x3840x2160p_6000_B:
	case NTV2_FORMAT_4x4096x2160p_3000:
	case NTV2_FORMAT_4x4096x2160p_6000_B:
		frameRate = NTV2_FRAMERATE_3000;
		break;
		
	case NTV2_FORMAT_1080p_2K_4795_A:
	case NTV2_FORMAT_4x2048x1080p_4795:
	case NTV2_FORMAT_4096x2160p_4795:
	case NTV2_FORMAT_4x4096x2160p_4795:
	case NTV2_FORMAT_1080p_2K_4795_B:
		frameRate = NTV2_FRAMERATE_4795;
		break;
		
	case NTV2_FORMAT_1080p_2K_4800_A:
	case NTV2_FORMAT_4x2048x1080p_4800:
	case NTV2_FORMAT_4096x2160p_4800:
	case NTV2_FORMAT_4x4096x2160p_4800:
	case NTV2_FORMAT_1080p_2K_4800_B:
		frameRate = NTV2_FRAMERATE_4800;
		break;

	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_2K_5000_A:
	case NTV2_FORMAT_4x1920x1080p_5000:
	case NTV2_FORMAT_4x2048x1080p_5000:
    case NTV2_FORMAT_3840x2160p_5000:
    case NTV2_FORMAT_4096x2160p_5000:
	case NTV2_FORMAT_4x3840x2160p_5000:
	case NTV2_FORMAT_4x4096x2160p_5000:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_1080p_2K_5000_B:
		frameRate = NTV2_FRAMERATE_5000;
		break;
		
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_2K_5994_A:
	case NTV2_FORMAT_4x1920x1080p_5994:
	case NTV2_FORMAT_4x2048x1080p_5994:
    case NTV2_FORMAT_3840x2160p_5994:
    case NTV2_FORMAT_4096x2160p_5994:
	case NTV2_FORMAT_4x3840x2160p_5994:
	case NTV2_FORMAT_4x4096x2160p_5994:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_1080p_2K_5994_B:
		frameRate = NTV2_FRAMERATE_5994;
		break;

	case NTV2_FORMAT_720p_6000:
	case NTV2_FORMAT_1080p_6000_A:
	case NTV2_FORMAT_1080p_2K_6000_A:
	case NTV2_FORMAT_4x1920x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_6000:
    case NTV2_FORMAT_3840x2160p_6000:
    case NTV2_FORMAT_4096x2160p_6000:
	case NTV2_FORMAT_4x3840x2160p_6000:
	case NTV2_FORMAT_4x4096x2160p_6000:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_1080p_2K_6000_B:
		frameRate = NTV2_FRAMERATE_6000;
		break;

	case NTV2_FORMAT_4x2048x1080p_11988:
    case NTV2_FORMAT_4096x2160p_11988:
		frameRate = NTV2_FRAMERATE_11988;
		break;
	case NTV2_FORMAT_4x2048x1080p_12000:
    case NTV2_FORMAT_4096x2160p_12000:
		frameRate = NTV2_FRAMERATE_12000;
		break;

#if defined (_DEBUG)
	//	Debug builds warn about missing values
	case NTV2_FORMAT_UNKNOWN:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
	case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
	case NTV2_FORMAT_END_2K_DEF_FORMATS:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
	case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
	case NTV2_FORMAT_END_4K_DEF_FORMATS2:
	case NTV2_FORMAT_END_UHD2_DEF_FORMATS:
	case NTV2_FORMAT_END_UHD2_FULL_DEF_FORMATS:
		break;
#else
	default:
		break;	// Unsupported -- fail
#endif
	}

	return frameRate;

}	//	GetNTV2FrameRateFromVideoFormat

NTV2Channel GetOutXptChannel(NTV2OutputCrosspointID inXpt, bool multiFormatActive)
{
	switch (inXpt)
	{
	case NTV2_XptSDIIn1:
	case NTV2_XptSDIIn1DS2:
	case NTV2_XptDuallinkIn1:
		return NTV2_CHANNEL1;
	case NTV2_XptSDIIn2:
	case NTV2_XptSDIIn2DS2:
	case NTV2_XptDuallinkIn2:
		return NTV2_CHANNEL2;
	case NTV2_XptSDIIn3:
	case NTV2_XptSDIIn3DS2:
	case NTV2_XptDuallinkIn3:
		return NTV2_CHANNEL3;
	case NTV2_XptSDIIn4:
	case NTV2_XptSDIIn4DS2:
	case NTV2_XptDuallinkIn4:
		return NTV2_CHANNEL4;
	case NTV2_XptSDIIn5:
	case NTV2_XptSDIIn5DS2:
	case NTV2_XptDuallinkIn5:
		return NTV2_CHANNEL5;
	case NTV2_XptSDIIn6:
	case NTV2_XptSDIIn6DS2:
	case NTV2_XptDuallinkIn6:
		return NTV2_CHANNEL6;
	case NTV2_XptSDIIn7:
	case NTV2_XptSDIIn7DS2:
	case NTV2_XptDuallinkIn7:
		return NTV2_CHANNEL7;
	case NTV2_XptSDIIn8:
	case NTV2_XptSDIIn8DS2:
	case NTV2_XptDuallinkIn8:
		return NTV2_CHANNEL8;
	case NTV2_XptFrameBuffer1YUV:
	case NTV2_XptFrameBuffer1RGB:
	case NTV2_XptFrameBuffer1_DS2YUV:
	case NTV2_XptFrameBuffer1_DS2RGB:
		return NTV2_CHANNEL1;
	case NTV2_XptFrameBuffer2YUV:
	case NTV2_XptFrameBuffer2RGB:
	case NTV2_XptFrameBuffer2_DS2YUV:
	case NTV2_XptFrameBuffer2_DS2RGB:
		return multiFormatActive ? NTV2_CHANNEL2 : NTV2_CHANNEL1;
	case NTV2_XptFrameBuffer3YUV:
	case NTV2_XptFrameBuffer3RGB:
	case NTV2_XptFrameBuffer3_DS2YUV:
	case NTV2_XptFrameBuffer3_DS2RGB:
		return multiFormatActive ? NTV2_CHANNEL3 : NTV2_CHANNEL1;
	case NTV2_XptFrameBuffer4YUV:
	case NTV2_XptFrameBuffer4RGB:
	case NTV2_XptFrameBuffer4_DS2YUV:
	case NTV2_XptFrameBuffer4_DS2RGB:
		return multiFormatActive ? NTV2_CHANNEL4 : NTV2_CHANNEL1;
	case NTV2_XptFrameBuffer5YUV:
	case NTV2_XptFrameBuffer5RGB:
		return multiFormatActive ? NTV2_CHANNEL5 : NTV2_CHANNEL1;
	case NTV2_XptFrameBuffer6YUV:
	case NTV2_XptFrameBuffer6RGB:
	case NTV2_XptFrameBuffer6_DS2YUV:
	case NTV2_XptFrameBuffer6_DS2RGB:
		return multiFormatActive ? NTV2_CHANNEL6 : NTV2_CHANNEL1;
	case NTV2_XptFrameBuffer7YUV:
	case NTV2_XptFrameBuffer7RGB:
	case NTV2_XptFrameBuffer7_DS2YUV:
	case NTV2_XptFrameBuffer7_DS2RGB:
		return multiFormatActive ? NTV2_CHANNEL7 : NTV2_CHANNEL1;
	case NTV2_XptFrameBuffer8YUV:
	case NTV2_XptFrameBuffer8RGB:
	case NTV2_XptFrameBuffer8_DS2YUV:
	case NTV2_XptFrameBuffer8_DS2RGB:
		return multiFormatActive ? NTV2_CHANNEL8 : NTV2_CHANNEL1;
	default:
		return NTV2_CHANNEL1;
	}
}

NTV2Standard GetStandardFromScanGeometry(NTV2ScanGeometry scanGeometry, ULWord progressive)
{
	NTV2Standard standard = NTV2_NUM_STANDARDS;

	switch(scanGeometry)
	{
	case NTV2_SG_525:
		standard = NTV2_STANDARD_525;
		break;
	case NTV2_SG_625:
		standard = NTV2_STANDARD_625;
		break;
	case NTV2_SG_750:
		standard = NTV2_STANDARD_720;
		break;
	case NTV2_SG_1125:
	case NTV2_SG_2Kx1080:
		if(progressive)
			standard = NTV2_STANDARD_1080p;
		else
			standard = NTV2_STANDARD_1080;
		break;
	case NTV2_SG_2Kx1556:
		standard = NTV2_STANDARD_2K;
		break;
	default:
		break;
	}

	return standard;
}

NTV2VideoFormat GetQuadSizedVideoFormat(NTV2VideoFormat videoFormat)
{
	NTV2VideoFormat quadSizedFormat;

	switch (videoFormat)
	{
	case  NTV2_FORMAT_1080p_2398:		quadSizedFormat = NTV2_FORMAT_4x1920x1080p_2398; break;
	case  NTV2_FORMAT_1080p_2400: 		quadSizedFormat = NTV2_FORMAT_4x1920x1080p_2400; break;
	case  NTV2_FORMAT_1080p_2500: 		quadSizedFormat = NTV2_FORMAT_4x1920x1080p_2500; break;
	case  NTV2_FORMAT_1080p_2997: 		quadSizedFormat = NTV2_FORMAT_4x1920x1080p_2997; break;
	case  NTV2_FORMAT_1080p_3000: 		quadSizedFormat = NTV2_FORMAT_4x1920x1080p_3000; break;
	case  NTV2_FORMAT_1080p_5000_A: 	quadSizedFormat = NTV2_FORMAT_4x1920x1080p_5000; break;
	case  NTV2_FORMAT_1080p_5994_A: 	quadSizedFormat = NTV2_FORMAT_4x1920x1080p_5994; break;
	case  NTV2_FORMAT_1080p_6000_A: 	quadSizedFormat = NTV2_FORMAT_4x1920x1080p_6000; break;
	case  NTV2_FORMAT_1080p_5000_B:		quadSizedFormat = NTV2_FORMAT_4x1920x1080p_5000; break;
	case  NTV2_FORMAT_1080p_5994_B:		quadSizedFormat = NTV2_FORMAT_4x1920x1080p_5994; break;
	case  NTV2_FORMAT_1080p_6000_B:		quadSizedFormat = NTV2_FORMAT_4x1920x1080p_6000; break;
	case  NTV2_FORMAT_1080p_2K_2398: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_2398; break;
	case  NTV2_FORMAT_1080p_2K_2400: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_2400; break;
	case  NTV2_FORMAT_1080p_2K_2500: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_2500; break;
	case  NTV2_FORMAT_1080p_2K_2997: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_2997; break;
	case  NTV2_FORMAT_1080p_2K_3000: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_3000; break;
	case  NTV2_FORMAT_1080p_2K_4795_A: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_4795; break;
	case  NTV2_FORMAT_1080p_2K_4800_A: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_4800; break;
	case  NTV2_FORMAT_1080p_2K_5000_A:	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_5000; break;
	case  NTV2_FORMAT_1080p_2K_5994_A:	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_5994; break;
	case  NTV2_FORMAT_1080p_2K_6000_A:	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_6000; break;
	case  NTV2_FORMAT_1080p_2K_4795_B: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_4795; break;
	case  NTV2_FORMAT_1080p_2K_4800_B: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_4800; break;
	case  NTV2_FORMAT_1080p_2K_5000_B:	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_5000; break;
	case  NTV2_FORMAT_1080p_2K_5994_B:	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_5994; break;
	case  NTV2_FORMAT_1080p_2K_6000_B:	quadSizedFormat = NTV2_FORMAT_4x2048x1080p_6000; break;
	case  NTV2_FORMAT_1080psf_2398:		quadSizedFormat = NTV2_FORMAT_4x1920x1080psf_2398; break;
	case  NTV2_FORMAT_1080psf_2400: 	quadSizedFormat = NTV2_FORMAT_4x1920x1080psf_2400; break;
	case  NTV2_FORMAT_1080i_5000:	 	quadSizedFormat = NTV2_FORMAT_4x1920x1080psf_2500; break;
	case  NTV2_FORMAT_1080i_5994:	 	quadSizedFormat = NTV2_FORMAT_4x1920x1080psf_2997; break;
	case  NTV2_FORMAT_1080i_6000:	 	quadSizedFormat = NTV2_FORMAT_4x1920x1080psf_3000; break;
	case  NTV2_FORMAT_1080psf_2K_2398:	quadSizedFormat = NTV2_FORMAT_4x2048x1080psf_2398; break;
	case  NTV2_FORMAT_1080psf_2K_2400: 	quadSizedFormat = NTV2_FORMAT_4x2048x1080psf_2400; break;
	case  NTV2_FORMAT_1080psf_2K_2500:	quadSizedFormat = NTV2_FORMAT_4x2048x1080psf_2500; break;
	default:							quadSizedFormat = videoFormat; break;
	}

	return quadSizedFormat;
}

NTV2VideoFormat Get12GVideoFormat(NTV2VideoFormat videoFormat)
{
	NTV2VideoFormat quadQuadSizedFormat;

	switch (videoFormat)
	{
	case NTV2_FORMAT_4x1920x1080p_2398: quadQuadSizedFormat = NTV2_FORMAT_3840x2160p_2398; break;
	case NTV2_FORMAT_4x1920x1080p_2400: quadQuadSizedFormat = NTV2_FORMAT_3840x2160p_2400; break;
	case NTV2_FORMAT_4x1920x1080p_2500: quadQuadSizedFormat = NTV2_FORMAT_3840x2160p_2500; break;
	case NTV2_FORMAT_4x1920x1080p_2997: quadQuadSizedFormat = NTV2_FORMAT_3840x2160p_2997; break;
	case NTV2_FORMAT_4x1920x1080p_3000: quadQuadSizedFormat = NTV2_FORMAT_3840x2160p_3000; break;
	case NTV2_FORMAT_4x1920x1080p_5000: quadQuadSizedFormat = NTV2_FORMAT_3840x2160p_5000; break;
	case NTV2_FORMAT_4x1920x1080p_5994: quadQuadSizedFormat = NTV2_FORMAT_3840x2160p_5994; break;
	case NTV2_FORMAT_4x1920x1080p_6000: quadQuadSizedFormat = NTV2_FORMAT_3840x2160p_6000; break;
	case NTV2_FORMAT_4x2048x1080p_2398: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_2398; break;
	case NTV2_FORMAT_4x2048x1080p_2400: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_2400; break;
	case NTV2_FORMAT_4x2048x1080p_2500: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_2500; break;
	case NTV2_FORMAT_4x2048x1080p_2997: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_2997; break;
	case NTV2_FORMAT_4x2048x1080p_3000: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_3000; break;
	case NTV2_FORMAT_4x2048x1080p_4795: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_4795; break;
	case NTV2_FORMAT_4x2048x1080p_4800: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_4800; break;
	case NTV2_FORMAT_4x2048x1080p_5000: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_5000; break;
	case NTV2_FORMAT_4x2048x1080p_5994: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_5994; break;
	case NTV2_FORMAT_4x2048x1080p_6000: quadQuadSizedFormat = NTV2_FORMAT_4096x2160p_6000; break;
	default:							quadQuadSizedFormat = videoFormat; break;
	}

	return quadQuadSizedFormat;
}

NTV2VideoFormat GetQuadQuadSizedVideoFormat(NTV2VideoFormat videoFormat)
{
	NTV2VideoFormat quadQuadSizedFormat;

	switch (videoFormat)
	{
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_3840x2160p_2398: quadQuadSizedFormat = NTV2_FORMAT_4x3840x2160p_2398; break;
	case NTV2_FORMAT_4x1920x1080p_2400: 
	case NTV2_FORMAT_3840x2160p_2400: quadQuadSizedFormat = NTV2_FORMAT_4x3840x2160p_2400; break;
	case NTV2_FORMAT_4x1920x1080p_2500: 
	case NTV2_FORMAT_3840x2160p_2500: quadQuadSizedFormat = NTV2_FORMAT_4x3840x2160p_2500; break;
	case NTV2_FORMAT_4x1920x1080p_2997: 
	case NTV2_FORMAT_3840x2160p_2997: quadQuadSizedFormat = NTV2_FORMAT_4x3840x2160p_2997; break;
	case NTV2_FORMAT_4x1920x1080p_3000: 
	case NTV2_FORMAT_3840x2160p_3000: quadQuadSizedFormat = NTV2_FORMAT_4x3840x2160p_3000; break;
	case NTV2_FORMAT_4x1920x1080p_5000: 
	case NTV2_FORMAT_3840x2160p_5000: quadQuadSizedFormat = NTV2_FORMAT_4x3840x2160p_5000; break;
	case NTV2_FORMAT_4x1920x1080p_5994: 
	case NTV2_FORMAT_3840x2160p_5994: quadQuadSizedFormat = NTV2_FORMAT_4x3840x2160p_5994; break;
	case NTV2_FORMAT_4x1920x1080p_6000: 
	case NTV2_FORMAT_3840x2160p_6000: quadQuadSizedFormat = NTV2_FORMAT_4x3840x2160p_6000; break;
	case NTV2_FORMAT_4x2048x1080p_2398: 
	case NTV2_FORMAT_4096x2160p_2398: quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_2398; break;
	case NTV2_FORMAT_4x2048x1080p_2400: 
	case NTV2_FORMAT_4096x2160p_2400:  quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_2400; break;
	case NTV2_FORMAT_4x2048x1080p_2500:  
	case NTV2_FORMAT_4096x2160p_2500: quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_2500; break;
	case NTV2_FORMAT_4x2048x1080p_2997: 
	case NTV2_FORMAT_4096x2160p_2997:  quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_2997; break;
	case NTV2_FORMAT_4x2048x1080p_3000:  
	case NTV2_FORMAT_4096x2160p_3000: quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_3000; break;
	case NTV2_FORMAT_4x2048x1080p_4795:  
	case NTV2_FORMAT_4096x2160p_4795: quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_4795; break;
	case NTV2_FORMAT_4x2048x1080p_4800:  
	case NTV2_FORMAT_4096x2160p_4800: quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_4800; break;
	case NTV2_FORMAT_4x2048x1080p_5000:  
	case NTV2_FORMAT_4096x2160p_5000: quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_5000; break;
	case NTV2_FORMAT_4x2048x1080p_5994:  
	case NTV2_FORMAT_4096x2160p_5994: quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_5994; break;
	case NTV2_FORMAT_4x2048x1080p_6000:  
	case NTV2_FORMAT_4096x2160p_6000: quadQuadSizedFormat = NTV2_FORMAT_4x4096x2160p_6000; break;
	default:							quadQuadSizedFormat = videoFormat; break;
	}

	return quadQuadSizedFormat;
}

NTV2VideoFormat GetHDSizedVideoFormat(NTV2VideoFormat videoFormat)
{
	NTV2VideoFormat hdSizedFormat;

	switch (videoFormat)
	{
	case NTV2_FORMAT_4x1920x1080p_2398:  hdSizedFormat = NTV2_FORMAT_1080p_2398; break;
	case NTV2_FORMAT_4x1920x1080p_2400:  hdSizedFormat = NTV2_FORMAT_1080p_2400; break;
	case NTV2_FORMAT_4x1920x1080p_2500:  hdSizedFormat = NTV2_FORMAT_1080p_2500; break;
	case NTV2_FORMAT_4x1920x1080p_2997:  hdSizedFormat = NTV2_FORMAT_1080p_2997; break;
	case NTV2_FORMAT_4x1920x1080p_3000:  hdSizedFormat = NTV2_FORMAT_1080p_3000; break;
	case NTV2_FORMAT_4x1920x1080p_5000:  hdSizedFormat = NTV2_FORMAT_1080p_5000_A; break;
	case NTV2_FORMAT_4x1920x1080p_5994:  hdSizedFormat = NTV2_FORMAT_1080p_5994_A; break;
	case NTV2_FORMAT_4x1920x1080p_6000:  hdSizedFormat = NTV2_FORMAT_1080p_6000_A; break;
	case NTV2_FORMAT_4x2048x1080p_2398:  hdSizedFormat = NTV2_FORMAT_1080p_2K_2398; break;
	case NTV2_FORMAT_4x2048x1080p_2400:  hdSizedFormat = NTV2_FORMAT_1080p_2K_2400; break;
	case NTV2_FORMAT_4x2048x1080p_2500:  hdSizedFormat = NTV2_FORMAT_1080p_2K_2500; break;
	case NTV2_FORMAT_4x2048x1080p_2997:  hdSizedFormat = NTV2_FORMAT_1080p_2K_2997; break;
	case NTV2_FORMAT_4x2048x1080p_3000:  hdSizedFormat = NTV2_FORMAT_1080p_2K_3000; break;
	case NTV2_FORMAT_4x2048x1080p_4795:  hdSizedFormat = NTV2_FORMAT_1080p_2K_4795_A; break;
	case NTV2_FORMAT_4x2048x1080p_4800:  hdSizedFormat = NTV2_FORMAT_1080p_2K_4800_A; break;
	case NTV2_FORMAT_4x2048x1080p_5000:  hdSizedFormat = NTV2_FORMAT_1080p_2K_5000_A; break;
	case NTV2_FORMAT_4x2048x1080p_5994:  hdSizedFormat = NTV2_FORMAT_1080p_2K_5994_A; break;
	case NTV2_FORMAT_4x2048x1080p_6000:  hdSizedFormat = NTV2_FORMAT_1080p_2K_6000_A; break;
	default:							hdSizedFormat = videoFormat; break;
	}

	return hdSizedFormat;
}

bool HDRIsChanging(HDRDriverValues inCurrentHDR, HDRDriverValues inNewHDR)
{
	if(	inCurrentHDR.greenPrimaryX != inNewHDR.greenPrimaryX ||
		inCurrentHDR.greenPrimaryY != inNewHDR.greenPrimaryY ||
		inCurrentHDR.bluePrimaryX != inNewHDR.bluePrimaryX ||
		inCurrentHDR.bluePrimaryY != inNewHDR.bluePrimaryY ||
		inCurrentHDR.redPrimaryX != inNewHDR.redPrimaryX ||
		inCurrentHDR.redPrimaryY != inNewHDR.redPrimaryY ||
		inCurrentHDR.whitePointX != inNewHDR.whitePointX ||
		inCurrentHDR.whitePointY != inNewHDR.whitePointY ||
		inCurrentHDR.maxMasteringLuminance != inNewHDR.maxMasteringLuminance ||
		inCurrentHDR.minMasteringLuminance != inNewHDR.minMasteringLuminance ||
		inCurrentHDR.maxContentLightLevel != inNewHDR.maxContentLightLevel ||
		inCurrentHDR.maxFrameAverageLightLevel != inNewHDR.maxFrameAverageLightLevel ||
		inCurrentHDR.electroOpticalTransferFunction != inNewHDR.electroOpticalTransferFunction ||
		inCurrentHDR.staticMetadataDescriptorID != inNewHDR.staticMetadataDescriptorID ||
		inCurrentHDR.luminance != inNewHDR.luminance )
		return true;
	else
		return false;
}
