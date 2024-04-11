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

//-------------------------------------------------------------------------------------------------------
//	InitLUTRegs
//
//	Called at launch to load something reasonable into the color correction tables
//-------------------------------------------------------------------------------------------------------
void InitLUTRegs(Ntv2SystemContext* context)
{
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	
	// LUTs already initialized by firmware with 12bit support
	if (Has12BitLUTSupport(context))
		return;
	
	if ( NTV2DeviceCanDoColorCorrection(deviceID) )
	{
		DebugLog("Initializing LUTs\n");
		switch( NTV2DeviceGetNumLUTs(deviceID) )
		{
		case 8:
			DownloadLinearLUTToHW (context, NTV2_CHANNEL8, 0);
			DownloadLinearLUTToHW (context, NTV2_CHANNEL8, 1);
			// Fall through
		case 7:
			DownloadLinearLUTToHW (context, NTV2_CHANNEL7, 0);
			DownloadLinearLUTToHW (context, NTV2_CHANNEL7, 1);
			// Fall through
		case 6:
			DownloadLinearLUTToHW (context, NTV2_CHANNEL6, 0);
			DownloadLinearLUTToHW (context, NTV2_CHANNEL6, 1);
			// Fall through
		case 5:
			DownloadLinearLUTToHW (context, NTV2_CHANNEL5, 0);
			DownloadLinearLUTToHW (context, NTV2_CHANNEL5, 1);
			// Fall through
		case 4:
			DownloadLinearLUTToHW (context, NTV2_CHANNEL4, 0);
			DownloadLinearLUTToHW (context, NTV2_CHANNEL4, 1);
			// Fall through
		case 3:
			DownloadLinearLUTToHW (context, NTV2_CHANNEL3, 0);
			DownloadLinearLUTToHW (context, NTV2_CHANNEL3, 1);
			// Fall through
		case 2:
			DownloadLinearLUTToHW (context, NTV2_CHANNEL2, 0);
			DownloadLinearLUTToHW (context, NTV2_CHANNEL2, 1);
			// Fall through
		case 1:
			DownloadLinearLUTToHW (context, NTV2_CHANNEL1, 0);
			DownloadLinearLUTToHW (context, NTV2_CHANNEL1, 1);
			break;
		default:
			break;
		}
	}

	ntv2WriteRegister(context, kVRegLUTType, NTV2_LUTUnknown);
}

bool Has12BitLUTSupport(Ntv2SystemContext* context)
{
	uint32_t has12BitLUTSupport(0);
	return ntv2ReadRegisterMS(context, kRegLUTV2Control, &has12BitLUTSupport, kRegMask12BitLUTSupport, kRegShift12BitLUTSupport)  &&  (has12BitLUTSupport ? true : false);
}

bool DownloadLinearLUTToHW(Ntv2SystemContext* context, NTV2Channel inChannel, int inBank)
{
	bool			bResult = true;
	NTV2DeviceID 	deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (NTV2DeviceCanDoColorCorrection(deviceID) )
	{
		NTV2ColorCorrectionHostAccessBank savedBank = GetColorCorrectionHostAccessBank(context, inChannel);

		SetLUTEnable(context, true, inChannel);
		// setup Host Access
		switch(inChannel)
		{
		case NTV2_CHANNEL1:
			SetColorCorrectionHostAccessBank(context, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH1BANK0 + inBank) );
			LoadLUTValues(context);
			break;
		case NTV2_CHANNEL2:
			SetColorCorrectionHostAccessBank(context, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH2BANK0 + inBank) );
			LoadLUTValues(context);
			break;
		case NTV2_CHANNEL3:
			SetColorCorrectionHostAccessBank(context, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH3BANK0 + inBank) );
			LoadLUTValues(context);
			break;
		case NTV2_CHANNEL4:
			SetColorCorrectionHostAccessBank(context, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH4BANK0 + inBank) );
			LoadLUTValues(context);
			break;
		case NTV2_CHANNEL5:
			SetColorCorrectionHostAccessBank(context, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH5BANK0 + inBank) );
			LoadLUTValues(context);
			break;
		case NTV2_CHANNEL6:
			SetColorCorrectionHostAccessBank(context, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH6BANK0 + inBank) );
			LoadLUTValues(context);
			break;
		case NTV2_CHANNEL7:
			SetColorCorrectionHostAccessBank(context, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH7BANK0 + inBank) );
			LoadLUTValues(context);
			break;
		case NTV2_CHANNEL8:
			SetColorCorrectionHostAccessBank(context, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH8BANK0 + inBank) );
			LoadLUTValues(context);
			break;
		default:
			break;
		}

		SetLUTEnable(context, false, inChannel);

		SetColorCorrectionHostAccessBank(context, savedBank);
	}

	return bResult;
}

void LoadLUTValues(Ntv2SystemContext* context)
{
	uint32_t lutValue;
	for (uint32_t i = 0; i < 1024;  i+=2)
	{
		// Tables are already converted to ints and endian swapped for the Mac
		lutValue = ((i+1)<<22) + (i<<6) ;
		ntv2WriteRegister(context, (kColorCorrectionLUTOffset_Red/4) + (i/2), lutValue);
		ntv2WriteRegister(context, (kColorCorrectionLUTOffset_Green/4) + (i/2), lutValue);
		ntv2WriteRegister(context, (kColorCorrectionLUTOffset_Blue/4) + (i/2), lutValue);
	}
}

bool SetLUTEnable(Ntv2SystemContext* context, bool inEnable, NTV2Channel inChannel)
{
	NTV2DeviceID 	deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	if(NTV2DeviceGetLUTVersion(deviceID) == 2)
	{
		switch(inChannel)
		{
			case NTV2_CHANNEL1:
				return ntv2WriteRegisterMS(context, kRegLUTV2Control, inEnable,  kRegMaskLUT1Enable, kRegShiftLUT1Enable);
			case NTV2_CHANNEL2:
				return ntv2WriteRegisterMS(context, kRegLUTV2Control, inEnable,  kRegMaskLUT2Enable, kRegShiftLUT2Enable);
			case NTV2_CHANNEL3:
				return ntv2WriteRegisterMS(context, kRegLUTV2Control, inEnable,  kRegMaskLUT3Enable, kRegShiftLUT3Enable);
			case NTV2_CHANNEL4:
				return ntv2WriteRegisterMS(context, kRegLUTV2Control, inEnable,  kRegMaskLUT4Enable, kRegShiftLUT4Enable);
			case NTV2_CHANNEL5:
				return ntv2WriteRegisterMS(context, kRegLUTV2Control, inEnable,  kRegMaskLUT5Enable, kRegShiftLUT5Enable);
			case NTV2_CHANNEL6:
				return ntv2WriteRegisterMS(context, kRegLUTV2Control, inEnable,  kRegMaskLUT6Enable, kRegShiftLUT6Enable);
			case NTV2_CHANNEL7:
				return ntv2WriteRegisterMS(context, kRegLUTV2Control, inEnable,  kRegMaskLUT7Enable, kRegShiftLUT7Enable);
			case NTV2_CHANNEL8:
				return ntv2WriteRegisterMS(context, kRegLUTV2Control, inEnable,  kRegMaskLUT8Enable, kRegShiftLUT8Enable);
			default:
				return false;
		}
	}
	return false;
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
		default:	break;
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
		default:	break;
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

uint32_t GetLUTV2HostAccessBank(Ntv2SystemContext* context, NTV2Channel inChannel)
{
	NTV2ColorCorrectionHostAccessBank outValue = NTV2_CCHOSTACCESS_CH1BANK0;

	uint32_t tempVal = 0;
	switch(inChannel)
	{
		case NTV2_CHANNEL1:
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &tempVal,  kRegMaskLUT1HostAccessBankSelect,  kRegShiftLUT1HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal);
			break;

		case NTV2_CHANNEL2:
			ntv2ReadRegisterMS(context, kRegLUTV2Control, &tempVal,  kRegMaskLUT2HostAccessBankSelect,  kRegShiftLUT2HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH2BANK0);
			break;

		case NTV2_CHANNEL3:
			ntv2ReadRegisterMS(context, kRegLUTV2Control,  &tempVal,  kRegMaskLUT3HostAccessBankSelect,  kRegShiftLUT3HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH3BANK0);
			break;

		case NTV2_CHANNEL4:
			ntv2ReadRegisterMS(context, kRegLUTV2Control,  &tempVal,  kRegMaskLUT4HostAccessBankSelect,  kRegShiftLUT4HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH4BANK0);
			break;

		case NTV2_CHANNEL5:
			ntv2ReadRegisterMS(context, kRegLUTV2Control,  &tempVal,  kRegMaskLUT5HostAccessBankSelect,  kRegShiftLUT5HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH5BANK0);
			break;

		case NTV2_CHANNEL6:
			ntv2ReadRegisterMS(context, kRegLUTV2Control,  &tempVal,  kRegMaskLUT6HostAccessBankSelect,  kRegShiftLUT6HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH6BANK0);
			break;

		case NTV2_CHANNEL7:
			ntv2ReadRegisterMS(context, kRegLUTV2Control,  &tempVal,  kRegMaskLUT7HostAccessBankSelect,  kRegShiftLUT7HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH7BANK0);
			break;

		case NTV2_CHANNEL8:
			ntv2ReadRegisterMS(context, kRegLUTV2Control,  &tempVal,  kRegMaskLUT8HostAccessBankSelect,  kRegShiftLUT8HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH8BANK0);
			break;

		default:	break;
	}
	return outValue;
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
	default:	break;
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

