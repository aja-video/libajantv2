/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//========================================================================
//
//  ntv2video.c
//
//==========================================================================

#include "ntv2system.h"
#include "ntv2video.h"
#include "ntv2kona.h"

#define NTV2REGWRITEMODEMASK (BIT_20+BIT_21)
#define NTV2REGWRITEMODESHIFT (20)

#define FGVCROSSPOINTMASK (BIT_0+BIT_1+BIT_2+BIT_3)
#define FGVCROSSPOINTSHIFT (0)
#define BGVCROSSPOINTMASK (BIT_4+BIT_5+BIT_6+BIT_7)
#define BGVCROSSPOINTSHIFT (4)
#define FGKCROSSPOINTMASK (BIT_8+BIT_9+BIT_10+BIT_11)
#define FGKCROSSPOINTSHIFT (8)
#define BGKCROSSPOINTMASK (BIT_12+BIT_13+BIT_14+BIT_15)
#define BGKCROSSPOINTSHIFT (12)

static const uint32_t	gChannelToGlobalControlRegNum []	= {	kRegGlobalControl, kRegGlobalControlCh2, kRegGlobalControlCh3, kRegGlobalControlCh4,
															kRegGlobalControlCh5, kRegGlobalControlCh6, kRegGlobalControlCh7, kRegGlobalControlCh8, 0};


void SetRegisterWritemode(Ntv2SystemContext* context, NTV2RegisterWriteMode value, NTV2Channel channel)
{
	if (!IsMultiFormatActive(context))
		channel = NTV2_CHANNEL1;

	uint32_t regNum = gChannelToGlobalControlRegNum[channel];

	ntv2WriteRegisterMS(context, regNum, value, NTV2REGWRITEMODEMASK, NTV2REGWRITEMODESHIFT);
}

int64_t GetFramePeriod(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2FrameRate frameRate;
	int64_t period;
	
	frameRate = GetFrameRate(context, channel);
	switch (frameRate)
	{
	case NTV2_FRAMERATE_12000:
		period = 10000000/120;
		break;
	case NTV2_FRAMERATE_11988:
		period = 10010000/120;
		break;
	case NTV2_FRAMERATE_6000:
		period = 10000000/60;
		break;
	case NTV2_FRAMERATE_5994:
		period = 10010000/60;
		break;
	case NTV2_FRAMERATE_4800:
		period = 10000000/48;
		break;
	case NTV2_FRAMERATE_4795:
		period = 10010000/48;
		break;
	case NTV2_FRAMERATE_3000:
		period = 10000000/30;
		break;
	case NTV2_FRAMERATE_2997:
		period = 10010000/30;
		break;
	case NTV2_FRAMERATE_2500:
		period = 10000000/25;
		break;
	case NTV2_FRAMERATE_2400:
		period = 10000000/24;
		break;
	case NTV2_FRAMERATE_2398:
		period = 10010000/24;
		break;
	case NTV2_FRAMERATE_5000:
		period = 10000000/50;
		break;
#if !defined(NTV2_DEPRECATE_16_0)
	case NTV2_FRAMERATE_1900:
		period = 10000000/19;
		break;
	case NTV2_FRAMERATE_1898:
		period = 10010000/19;
		break;
	case NTV2_FRAMERATE_1800:
		period = 10000000/18;
		break;
	case NTV2_FRAMERATE_1798:
		period = 10010000/18;
		break;
	case NTV2_FRAMERATE_1500:
		period = 10000000/15;
		break;
	case NTV2_FRAMERATE_1498:
		period = 10010000/15;
		break;
#endif	//	!defined(NTV2_DEPRECATE_16_0)
	case NTV2_FRAMERATE_UNKNOWN:
	default:
		period = 10000000;
	}

	return period;
}

void SetColorCorrectionHostAccessBank(Ntv2SystemContext* context, NTV2ColorCorrectionHostAccessBank value)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if(NTV2DeviceGetLUTVersion(deviceID) == 2)
	{
		return SetLUTV2HostAccessBank(context, value);
	}
	else
	{
		switch(value)
		{
		case NTV2_CCHOSTACCESS_CH1BANK0:
		case NTV2_CCHOSTACCESS_CH1BANK1:
		case NTV2_CCHOSTACCESS_CH2BANK0:
		case NTV2_CCHOSTACCESS_CH2BANK1:
		{
			if(NTV2DeviceGetNumLUTs(deviceID) > 4)
			{
				ntv2WriteRegisterMS(context,
									kRegCh1ColorCorrectioncontrol,
									0,
									kRegMaskLUT5Select,
									kRegMaskLUT5Select);
			}

			ntv2WriteRegisterMS(context,
								kRegCh1ColorCorrectioncontrol,
								NTV2_LUTCONTROL_1_2,
								kRegMaskLUTSelect,
								kRegShiftLUTSelect);

			ntv2WriteRegisterMS(context,
								kRegGlobalControl,
								value,
								kRegMaskCCHostBankSelect,
								kRegShiftCCHostAccessBankSelect);
		}
		break;
		case NTV2_CCHOSTACCESS_CH3BANK0:
		case NTV2_CCHOSTACCESS_CH3BANK1:
		case NTV2_CCHOSTACCESS_CH4BANK0:
		case NTV2_CCHOSTACCESS_CH4BANK1:
		{
			if(NTV2DeviceGetNumLUTs(deviceID) > 4)
			{
				ntv2WriteRegisterMS(context,
									kRegCh1ColorCorrectioncontrol,
									0,
									kRegMaskLUT5Select,
									kRegMaskLUT5Select);
			}

			ntv2WriteRegisterMS(context,
								kRegCh1ColorCorrectioncontrol,
								NTV2_LUTCONTROL_3_4,
								kRegMaskLUTSelect,
								kRegShiftLUTSelect);

			ntv2WriteRegisterMS(context,
								kRegCh1ColorCorrectioncontrol,
								value - NTV2_CCHOSTACCESS_CH3BANK0,
								kRegMaskCCHostBankSelect,
								kRegShiftCCHostAccessBankSelect);
		}
		break;
		case NTV2_CCHOSTACCESS_CH5BANK0:
		case NTV2_CCHOSTACCESS_CH5BANK1:
		{
			ntv2WriteRegisterMS(context,
								kRegCh1ColorCorrectioncontrol,
								0,
								kRegMaskLUTSelect,
								kRegShiftLUTSelect);

			ntv2WriteRegisterMS(context,
								kRegGlobalControl,
								0,
								kRegMaskCCHostBankSelect,
								kRegShiftCCHostAccessBankSelect);

			ntv2WriteRegisterMS(context,
								kRegCh1ColorCorrectioncontrol,
								0x1,
								kRegMaskLUT5Select,
								kRegMaskLUT5Select);

			ntv2WriteRegisterMS(context,
								kRegCh1ColorCorrectioncontrol,
								value - NTV2_CCHOSTACCESS_CH5BANK0,
								kRegMaskCC5HostAccessBankSelect,
								kRegShiftCC5HostAccessBankSelect);
		}
		break;
		}
	}
}

NTV2ColorCorrectionHostAccessBank GetColorCorrectionHostAccessBank(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	NTV2ColorCorrectionHostAccessBank value = NTV2_CCHOSTACCESS_CH1BANK0;
	uint32_t regValue = 0;

	if(NTV2DeviceGetLUTVersion(deviceID) == 1)
	{
		switch(channel)
		{
		default:
		case NTV2_CHANNEL1:
		case NTV2_CHANNEL2:
			regValue = ntv2ReadRegister(context, kRegGlobalControl);
			regValue &= kRegMaskCCHostBankSelect;
			value =  (NTV2ColorCorrectionHostAccessBank)(regValue >> kRegShiftCCHostAccessBankSelect);
			break;
		case NTV2_CHANNEL3:
		case NTV2_CHANNEL4:
			regValue = ntv2ReadRegister(context, kRegCh1ColorCorrectioncontrol);
			regValue &= kRegMaskCCHostBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH3BANK0) >> kRegShiftCCHostAccessBankSelect);
			break;
		case NTV2_CHANNEL5:
			regValue = ntv2ReadRegister(context, kRegCh1ColorCorrectioncontrol);
			regValue &= kRegMaskCC5HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH5BANK0) >> kRegShiftCC5HostAccessBankSelect );
			break;
		}
	}
	else
	{
		regValue = ntv2ReadRegister(context, kRegLUTV2Control);
		switch(channel)
		{
		case NTV2_CHANNEL1:
			regValue &= kRegMaskLUT1HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)(regValue >> kRegShiftLUT1HostAccessBankSelect);
			break;
		case NTV2_CHANNEL2:
			regValue &= kRegMaskLUT2HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH2BANK0) >> kRegShiftLUT2HostAccessBankSelect);
			break;
		case NTV2_CHANNEL3:
			regValue &= kRegMaskLUT3HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH3BANK0) >>kRegShiftLUT3HostAccessBankSelect);
			break;
		case NTV2_CHANNEL4:
			regValue &= kRegMaskLUT4HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH4BANK0) >>kRegShiftLUT4HostAccessBankSelect);
			break;
		case NTV2_CHANNEL5:
			regValue &= kRegMaskLUT5HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH5BANK0) >>kRegShiftLUT5HostAccessBankSelect);
			break;
		case NTV2_CHANNEL6:
			regValue &= kRegMaskLUT6HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH6BANK0) >>kRegShiftLUT6HostAccessBankSelect);
			break;
		case NTV2_CHANNEL7:
			regValue &= kRegMaskLUT7HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH7BANK0) >>kRegShiftLUT7HostAccessBankSelect);
			break;
		case NTV2_CHANNEL8:
			regValue &= kRegMaskLUT8HostAccessBankSelect;
			value = (NTV2ColorCorrectionHostAccessBank)((regValue+NTV2_CCHOSTACCESS_CH8BANK0) >>kRegShiftLUT8HostAccessBankSelect);
			break;
		}
	}
	return value;
}

void SetColorCorrectionSaturation(Ntv2SystemContext* context, NTV2Channel channel, uint32_t value)
{
	if (channel == NTV2_CHANNEL1)
	{
		ntv2WriteRegisterMS(context, kRegCh1ColorCorrectioncontrol, value,
							kRegMaskSaturationValue, kRegShiftSaturationValue);
	}
	else
	{
		ntv2WriteRegisterMS(context, kRegCh2ColorCorrectioncontrol, value,
							kRegMaskSaturationValue, kRegShiftSaturationValue);
	}	
}

uint32_t GetColorCorrectionSaturation(Ntv2SystemContext* context, NTV2Channel channel)
{
	uint32_t value;
	uint32_t regValue;
	
	if (channel == NTV2_CHANNEL1)
	{	
		regValue = ntv2ReadRegister(context, kRegCh1ColorCorrectioncontrol);
	}
	else
	{
		regValue = ntv2ReadRegister(context, kRegCh2ColorCorrectioncontrol);
	}
	regValue &= kRegMaskSaturationValue;
	value =  (uint32_t)(regValue >> kRegShiftSaturationValue);

	return value;
}

void SetColorCorrectionOutputBank(Ntv2SystemContext* context, NTV2Channel channel, uint32_t bank)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (NTV2DeviceGetLUTVersion(deviceID) == 2 )
	{
		return SetLUTV2OutputBank(context, channel, bank);
	}

	switch(channel)
	{
	default:
	case NTV2_CHANNEL1:
		ntv2WriteRegisterMS(context,
							kRegCh1ColorCorrectioncontrol,
							bank,
							kRegMaskCCOutputBankSelect,
							kRegShiftCCOutputBankSelect);
		break;

	case NTV2_CHANNEL2:
		ntv2WriteRegisterMS(context,
							kRegCh2ColorCorrectioncontrol,
							bank,
							kRegMaskCCOutputBankSelect,
							kRegShiftCCOutputBankSelect);
		break;

	case NTV2_CHANNEL3:
		ntv2WriteRegisterMS(context,
							kRegCh2ColorCorrectioncontrol,
							bank,
							kRegMaskCC3OutputBankSelect,
							kRegShiftCC3OutputBankSelect);
		break;

	case NTV2_CHANNEL4:
		ntv2WriteRegisterMS(context,
							kRegCh2ColorCorrectioncontrol,
							bank,
							kRegMaskCC4OutputBankSelect,
							kRegShiftCC4OutputBankSelect);
		break;
	}
}

uint32_t GetColorCorrectionOutputBank(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	uint32_t value = 0;
	
	if( NTV2DeviceGetLUTVersion(deviceID) == 2 )
	{
		return GetLUTV2OutputBank(context, channel);
	}

	switch(channel)
	{
	default:
	case NTV2_CHANNEL1:
		ntv2ReadRegisterMS(context,
						   kRegCh1ColorCorrectioncontrol,
						   &value,
						   kRegMaskCCOutputBankSelect,
						   kRegShiftCCOutputBankSelect);
		break;

	case NTV2_CHANNEL2:
		ntv2ReadRegisterMS(context,
						   kRegCh2ColorCorrectioncontrol,
						   &value,
						   kRegMaskCCOutputBankSelect,
						   kRegShiftCCOutputBankSelect);
		break;

	case NTV2_CHANNEL3:
		ntv2ReadRegisterMS(context,
						   kRegCh2ColorCorrectioncontrol,
						   &value,
						   kRegMaskCC3OutputBankSelect,
						   kRegShiftCC3OutputBankSelect);
		break;

	case NTV2_CHANNEL4:
		ntv2ReadRegisterMS(context,
						   kRegCh2ColorCorrectioncontrol,
						   &value,
						   kRegMaskCC4OutputBankSelect,
						   kRegShiftCC4OutputBankSelect);
		break;
	}

	return value;
}

void SetLUTV2HostAccessBank(Ntv2SystemContext* context, NTV2ColorCorrectionHostAccessBank value)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	uint32_t numLUT = NTV2DeviceGetNumLUTs(deviceID);

	switch(value)
	{
	default:
	case NTV2_CCHOSTACCESS_CH1BANK0:
	case NTV2_CCHOSTACCESS_CH1BANK1:
		if(numLUT > 0)
			ntv2WriteRegisterMS(context,
								kRegLUTV2Control,
								value - NTV2_CCHOSTACCESS_CH1BANK0,
								kRegMaskLUT1HostAccessBankSelect,
								kRegShiftLUT1HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH2BANK0:
	case NTV2_CCHOSTACCESS_CH2BANK1:
		if(numLUT > 1)
			ntv2WriteRegisterMS(context,
								kRegLUTV2Control,
								value - NTV2_CCHOSTACCESS_CH2BANK0,
								kRegMaskLUT2HostAccessBankSelect,
								kRegShiftLUT2HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH3BANK0:
	case NTV2_CCHOSTACCESS_CH3BANK1:
		if(numLUT > 2)
			ntv2WriteRegisterMS(context,
								kRegLUTV2Control,
								value - NTV2_CCHOSTACCESS_CH3BANK0,
								kRegMaskLUT3HostAccessBankSelect,
								kRegShiftLUT3HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH4BANK0:
	case NTV2_CCHOSTACCESS_CH4BANK1:
		if(numLUT > 3)
			ntv2WriteRegisterMS(context,
								kRegLUTV2Control,
								value - NTV2_CCHOSTACCESS_CH4BANK0,
								kRegMaskLUT4HostAccessBankSelect,
								kRegShiftLUT4HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH5BANK0:
	case NTV2_CCHOSTACCESS_CH5BANK1:
		if(numLUT > 4)
			ntv2WriteRegisterMS(context,
								kRegLUTV2Control,
								value - NTV2_CCHOSTACCESS_CH5BANK0,
								kRegMaskLUT5HostAccessBankSelect,
								kRegShiftLUT5HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH6BANK0:
	case NTV2_CCHOSTACCESS_CH6BANK1:
		if(numLUT > 5)
			ntv2WriteRegisterMS(context,
								kRegLUTV2Control,
								value - NTV2_CCHOSTACCESS_CH6BANK0,
								kRegMaskLUT6HostAccessBankSelect,
								kRegShiftLUT6HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH7BANK0:
	case NTV2_CCHOSTACCESS_CH7BANK1:
		if(numLUT > 6)
			ntv2WriteRegisterMS(context,
								kRegLUTV2Control,
								value - NTV2_CCHOSTACCESS_CH7BANK0,
								kRegMaskLUT7HostAccessBankSelect,
								kRegShiftLUT7HostAccessBankSelect);
		break;
	case NTV2_CCHOSTACCESS_CH8BANK0:
	case NTV2_CCHOSTACCESS_CH8BANK1:
		if(numLUT > 7)
			ntv2WriteRegisterMS(context,
								kRegLUTV2Control,
								value - NTV2_CCHOSTACCESS_CH8BANK0,
								kRegMaskLUT8HostAccessBankSelect,
								kRegShiftLUT8HostAccessBankSelect);
		break;
	}
}

void SetLUTV2OutputBank(Ntv2SystemContext* context, NTV2Channel channel, uint32_t bank)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	uint32_t numLUT = NTV2DeviceGetNumLUTs(deviceID);
	
	switch(channel)
	{
	case NTV2_CHANNEL1:
		if(numLUT > 0)
			ntv2WriteRegisterMS(context, kRegLUTV2Control, bank, kRegMaskLUT1OutputBankSelect, kRegShiftLUT1OutputBankSelect);
		break;
	case NTV2_CHANNEL2:
		if(numLUT > 1)
			ntv2WriteRegisterMS(context, kRegLUTV2Control, bank, kRegMaskLUT2OutputBankSelect, kRegShiftLUT2OutputBankSelect);
		break;
	case NTV2_CHANNEL3:
		if(numLUT > 2)
			ntv2WriteRegisterMS(context, kRegLUTV2Control, bank, kRegMaskLUT3OutputBankSelect, kRegShiftLUT3OutputBankSelect);
		break;
	case NTV2_CHANNEL4:
		if(numLUT > 3)
			ntv2WriteRegisterMS(context, kRegLUTV2Control, bank, kRegMaskLUT4OutputBankSelect, kRegShiftLUT4OutputBankSelect);
		break;
	case NTV2_CHANNEL5:
		if(numLUT > 4)
			ntv2WriteRegisterMS(context, kRegLUTV2Control, bank, kRegMaskLUT5OutputBankSelect, kRegShiftLUT5OutputBankSelect);
		break;
	case NTV2_CHANNEL6:
		if(numLUT > 5)
			ntv2WriteRegisterMS(context, kRegLUTV2Control, bank, kRegMaskLUT6OutputBankSelect, kRegShiftLUT6OutputBankSelect);
		break;
	case NTV2_CHANNEL7:
		if(numLUT > 6)
			ntv2WriteRegisterMS(context, kRegLUTV2Control, bank, kRegMaskLUT7OutputBankSelect, kRegShiftLUT7OutputBankSelect);
		break;
	case NTV2_CHANNEL8:
		if(numLUT > 7)
			ntv2WriteRegisterMS(context, kRegLUTV2Control, bank, kRegMaskLUT8OutputBankSelect, kRegShiftLUT8OutputBankSelect);
		break;
	}
}

uint32_t GetLUTV2OutputBank(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	uint32_t numLUT = NTV2DeviceGetNumLUTs(deviceID);
	uint32_t bank = 0;
	
	switch(channel)
	{
	default:
	case NTV2_CHANNEL1:
		if(numLUT > 0)
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &bank, kRegMaskLUT1OutputBankSelect, kRegShiftLUT1OutputBankSelect);
		break;
	case NTV2_CHANNEL2:
		if(numLUT > 1)
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &bank, kRegMaskLUT2OutputBankSelect, kRegShiftLUT2OutputBankSelect);
		break;
	case NTV2_CHANNEL3:
		if(numLUT > 2)
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &bank, kRegMaskLUT3OutputBankSelect, kRegShiftLUT3OutputBankSelect);
		break;
	case NTV2_CHANNEL4:
		if(numLUT > 3)
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &bank, kRegMaskLUT4OutputBankSelect, kRegShiftLUT4OutputBankSelect);
		break;
	case NTV2_CHANNEL5:
		if(numLUT > 4)
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &bank, kRegMaskLUT5OutputBankSelect, kRegShiftLUT5OutputBankSelect);
		break;
	case NTV2_CHANNEL6:
		if(numLUT > 5)
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &bank, kRegMaskLUT6OutputBankSelect, kRegShiftLUT6OutputBankSelect);
		break;
	case NTV2_CHANNEL7:
		if(numLUT > 6)
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &bank, kRegMaskLUT7OutputBankSelect, kRegShiftLUT7OutputBankSelect);
		break;
	case NTV2_CHANNEL8:
		if(numLUT > 7)
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &bank, kRegMaskLUT8OutputBankSelect, kRegShiftLUT8OutputBankSelect);
		break;
	}
	
	return bank;
}

void SetColorCorrectionMode(Ntv2SystemContext* context, NTV2Channel channel, NTV2ColorCorrectionMode mode)
{
	if ( channel == NTV2_CHANNEL1 )
	{
		ntv2WriteRegisterMS(context, kRegCh1ColorCorrectioncontrol, (uint32_t)mode, kRegMaskCCMode, kRegShiftCCMode);
	}
	else
	{
		ntv2WriteRegisterMS(context, kRegCh2ColorCorrectioncontrol, (uint32_t)mode, kRegMaskCCMode, kRegShiftCCMode);
	}
}

NTV2ColorCorrectionMode GetColorCorrectionMode(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2ColorCorrectionMode value;
	uint32_t regValue;
	
	if ( channel == NTV2_CHANNEL1 )
	{	
		regValue = ntv2ReadRegister(context, kRegCh1ColorCorrectioncontrol);
	}
	else
	{
		regValue = ntv2ReadRegister(context, kRegCh2ColorCorrectioncontrol);
	}
	regValue &= kRegMaskCCMode;
	value =  (NTV2ColorCorrectionMode)(regValue >> kRegShiftCCMode);

	return value;
}

void SetForegroundVideoCrosspoint(Ntv2SystemContext* context, NTV2Crosspoint crosspoint)
{
	ntv2WriteRegisterMS(context, kRegVidProcXptControl, (uint32_t)crosspoint,
						FGVCROSSPOINTMASK, FGVCROSSPOINTSHIFT);
}

void SetForegroundKeyCrosspoint(Ntv2SystemContext* context, NTV2Crosspoint crosspoint)
{
	ntv2WriteRegisterMS(context, kRegVidProcXptControl, (uint32_t)crosspoint,
						FGKCROSSPOINTMASK, FGKCROSSPOINTSHIFT);
}

void SetBackgroundVideoCrosspoint(Ntv2SystemContext* context, NTV2Crosspoint crosspoint)
{
	ntv2WriteRegisterMS(context, kRegVidProcXptControl, (uint32_t)crosspoint,
						BGVCROSSPOINTMASK, BGVCROSSPOINTSHIFT);
}

void SetBackgroundKeyCrosspoint(Ntv2SystemContext* context, NTV2Crosspoint crosspoint)
{
	ntv2WriteRegisterMS(context, kRegVidProcXptControl, (uint32_t)crosspoint,
						BGKCROSSPOINTMASK, BGKCROSSPOINTSHIFT);
}

