/**
	@file		ntv2csclut.cpp
	@brief		Implements most of CNTV2Card's CSC/LUT-related functions.
	@copyright	(C) 2019-2020 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2registerexpert.h"
#include "ajabase/system/debug.h"
#include <math.h>
#include <assert.h>
#if defined (AJALinux)
	#include "ntv2linuxpublicinterface.h"
#elif defined (MSWindows)
	#pragma warning(disable: 4800)
#endif
#include <deque>

#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') <<               uint64_t(__x__)  << dec
#define INSTP(_p_)			HEX16(uint64_t(_p_))
#define	CSCFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_CSC, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	CSCWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_CSC, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	CSCNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_CSC, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	CSCINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_CSC, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	CSCDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_CSC, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

#define	LUTFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_LUT, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	LUTWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_LUT, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	LUTNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_LUT, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	LUTINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_LUT, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	LUTDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_LUT, INSTP(this) << "::" << AJAFUNC << ": " << __x__)

using namespace std;



//	These static tables eliminate a lot of switch statements.
//	CAUTION:	These are predicated on NTV2Channel being ordinal (NTV2_CHANNEL1==0, NTV2_CHANNEL2==1, etc.)

static const ULWord	gChannelToEnhancedCSCRegNum []		= {	kRegEnhancedCSC1Mode,	kRegEnhancedCSC2Mode,	kRegEnhancedCSC3Mode,	kRegEnhancedCSC4Mode,
															kRegEnhancedCSC5Mode,	kRegEnhancedCSC6Mode,	kRegEnhancedCSC7Mode,	kRegEnhancedCSC8Mode,	0};

static const ULWord	gChannelToCSCoeff12RegNum []		= {	kRegCSCoefficients1_2,	kRegCS2Coefficients1_2,	kRegCS3Coefficients1_2,	kRegCS4Coefficients1_2,
															kRegCS5Coefficients1_2,	kRegCS6Coefficients1_2,	kRegCS7Coefficients1_2,	kRegCS8Coefficients1_2,	0};

static const ULWord	gChannelToCSCoeff34RegNum []		= {	kRegCSCoefficients3_4,	kRegCS2Coefficients3_4,	kRegCS3Coefficients3_4,	kRegCS4Coefficients3_4,
															kRegCS5Coefficients3_4,	kRegCS6Coefficients3_4,	kRegCS7Coefficients3_4,	kRegCS8Coefficients3_4,	0};

static const ULWord	gChannelToCSCoeff56RegNum []		= {	kRegCSCoefficients5_6,	kRegCS2Coefficients5_6,	kRegCS3Coefficients5_6,	kRegCS4Coefficients5_6,
															kRegCS5Coefficients5_6,	kRegCS6Coefficients5_6,	kRegCS7Coefficients5_6,	kRegCS8Coefficients5_6,	0};

static const ULWord	gChannelToCSCoeff78RegNum []		= {	kRegCSCoefficients7_8,	kRegCS2Coefficients7_8,	kRegCS3Coefficients7_8,	kRegCS4Coefficients7_8,
															kRegCS5Coefficients7_8,	kRegCS6Coefficients7_8,	kRegCS7Coefficients7_8,	kRegCS8Coefficients7_8,	0};

static const ULWord	gChannelToCSCoeff910RegNum []		= {	kRegCSCoefficients9_10,	kRegCS2Coefficients9_10,	kRegCS3Coefficients9_10,	kRegCS4Coefficients9_10,
															kRegCS5Coefficients9_10,	kRegCS6Coefficients9_10,	kRegCS7Coefficients9_10,	kRegCS8Coefficients9_10,	0};


//////////////////////////////////////////////////////////////////
// OEM Color Correction Methods (KHD-22 only )
//
bool  CNTV2Card::SetColorCorrectionMode(const NTV2Channel inChannel, const NTV2ColorCorrectionMode inMode)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return WriteRegister (inChannel == NTV2_CHANNEL1  ?  kRegCh1ColorCorrectionControl  :  kRegCh2ColorCorrectionControl,
							inMode,  kRegMaskCCMode,  kRegShiftCCMode);

}

bool  CNTV2Card::GetColorCorrectionMode(const NTV2Channel inChannel, NTV2ColorCorrectionMode & outMode)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (inChannel == NTV2_CHANNEL1 ? kRegCh1ColorCorrectionControl : kRegCh2ColorCorrectionControl,
												outMode,  kRegMaskCCMode,  kRegShiftCCMode);
}

bool CNTV2Card::SetColorCorrectionOutputBank (const NTV2Channel inChannel, const ULWord inBank)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	if (::NTV2DeviceGetLUTVersion(_boardID) == 2)
		return SetLUTV2OutputBank(inChannel, inBank);
	switch(inChannel)
	{
		case NTV2_CHANNEL1:		return WriteRegister (kRegCh1ColorCorrectionControl, inBank, kRegMaskCCOutputBankSelect,	kRegShiftCCOutputBankSelect);
		case NTV2_CHANNEL2:		return WriteRegister (kRegCh2ColorCorrectionControl, inBank, kRegMaskCCOutputBankSelect,	kRegShiftCCOutputBankSelect);
		case NTV2_CHANNEL3:		return WriteRegister (kRegCh2ColorCorrectionControl, inBank, kRegMaskCC3OutputBankSelect,	kRegShiftCC3OutputBankSelect);
		case NTV2_CHANNEL4:		return WriteRegister (kRegCh2ColorCorrectionControl, inBank, kRegMaskCC4OutputBankSelect,	kRegShiftCC4OutputBankSelect);
		case NTV2_CHANNEL5:		return WriteRegister (kRegCh1ColorCorrectionControl, inBank, kRegMaskCC5OutputBankSelect,	kRegShiftCC5OutputBankSelect);
		default:				return false;
	}
}

bool CNTV2Card::SetLUTV2OutputBank (const NTV2Channel inChannel, const ULWord inBank)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	switch (inChannel)
	{
		case NTV2_CHANNEL1:		return WriteRegister (kRegLUTV2Control,	inBank, kRegMaskLUT1OutputBankSelect, kRegShiftLUT1OutputBankSelect);
		case NTV2_CHANNEL2:		return WriteRegister (kRegLUTV2Control,	inBank, kRegMaskLUT2OutputBankSelect,	kRegShiftLUT2OutputBankSelect);
		case NTV2_CHANNEL3:		return WriteRegister (kRegLUTV2Control,	inBank, kRegMaskLUT3OutputBankSelect,	kRegShiftLUT3OutputBankSelect);
		case NTV2_CHANNEL4:		return WriteRegister (kRegLUTV2Control,	inBank, kRegMaskLUT4OutputBankSelect,	kRegShiftLUT4OutputBankSelect);
		case NTV2_CHANNEL5:		return WriteRegister (kRegLUTV2Control,	inBank, kRegMaskLUT5OutputBankSelect,	kRegShiftLUT5OutputBankSelect);
		case NTV2_CHANNEL6:		return WriteRegister (kRegLUTV2Control,	inBank, kRegMaskLUT6OutputBankSelect, kRegShiftLUT6OutputBankSelect);
		case NTV2_CHANNEL7:		return WriteRegister (kRegLUTV2Control, inBank, kRegMaskLUT7OutputBankSelect, kRegShiftLUT7OutputBankSelect);
		case NTV2_CHANNEL8:		return WriteRegister (kRegLUTV2Control, inBank, kRegMaskLUT8OutputBankSelect, kRegShiftLUT8OutputBankSelect);
		default:				return false;
	}
}

bool CNTV2Card::GetColorCorrectionOutputBank (const NTV2Channel inChannel, ULWord & outBank)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	if(::NTV2DeviceGetLUTVersion(_boardID) == 2)
		return GetLUTV2OutputBank(inChannel, outBank);
	switch(inChannel)
	{
		case NTV2_CHANNEL1:		return ReadRegister (kRegCh1ColorCorrectionControl,	outBank,	kRegMaskCCOutputBankSelect,		kRegShiftCCOutputBankSelect);
		case NTV2_CHANNEL2:		return ReadRegister (kRegCh2ColorCorrectionControl,	outBank,	kRegMaskCCOutputBankSelect,		kRegShiftCCOutputBankSelect);
		case NTV2_CHANNEL3:		return ReadRegister (kRegCh2ColorCorrectionControl,	outBank,	kRegMaskCC3OutputBankSelect,	kRegShiftCC3OutputBankSelect);
		case NTV2_CHANNEL4:		return ReadRegister (kRegCh2ColorCorrectionControl,	outBank,	kRegMaskCC4OutputBankSelect,	kRegShiftCC4OutputBankSelect);
		case NTV2_CHANNEL5:		return ReadRegister (kRegCh1ColorCorrectionControl,	outBank,	kRegMaskCC5OutputBankSelect,	kRegShiftCC5OutputBankSelect);
		default:				return false;
	}
}

bool CNTV2Card::GetLUTV2OutputBank (const NTV2Channel inChannel, ULWord & outBank)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	switch(inChannel)
	{
		case NTV2_CHANNEL1:		return ReadRegister (kRegLUTV2Control,	outBank,	kRegMaskLUT1OutputBankSelect,	kRegShiftLUT1OutputBankSelect);
		case NTV2_CHANNEL2:		return ReadRegister (kRegLUTV2Control,	outBank,	kRegMaskLUT2OutputBankSelect,	kRegShiftLUT2OutputBankSelect);
		case NTV2_CHANNEL3:		return ReadRegister (kRegLUTV2Control,	outBank,	kRegMaskLUT3OutputBankSelect,	kRegShiftLUT3OutputBankSelect);
		case NTV2_CHANNEL4:		return ReadRegister (kRegLUTV2Control,	outBank,	kRegMaskLUT4OutputBankSelect,	kRegShiftLUT4OutputBankSelect);
		case NTV2_CHANNEL5:		return ReadRegister (kRegLUTV2Control,	outBank,	kRegMaskLUT5OutputBankSelect,	kRegShiftLUT5OutputBankSelect);
		case NTV2_CHANNEL6:		return ReadRegister (kRegLUTV2Control,	outBank,	kRegMaskLUT6OutputBankSelect,	kRegShiftLUT6OutputBankSelect);
		case NTV2_CHANNEL7:		return ReadRegister (kRegLUTV2Control,	outBank,	kRegMaskLUT7OutputBankSelect,	kRegShiftLUT7OutputBankSelect);
		case NTV2_CHANNEL8:		return ReadRegister (kRegLUTV2Control,	outBank,	kRegMaskLUT8OutputBankSelect,	kRegShiftLUT8OutputBankSelect);
		default:				return false;
	}
}

bool CNTV2Card::SetColorCorrectionHostAccessBank (const NTV2ColorCorrectionHostAccessBank inValue)
{
	if(::NTV2DeviceGetLUTVersion(_boardID) == 2)
		return SetLUTV2HostAccessBank(inValue);

	switch(inValue)
	{
		case NTV2_CCHOSTACCESS_CH1BANK0:
		case NTV2_CCHOSTACCESS_CH1BANK1:
		case NTV2_CCHOSTACCESS_CH2BANK0:
		case NTV2_CCHOSTACCESS_CH2BANK1:
			if (::NTV2DeviceGetNumLUTs(GetDeviceID()) == 5 || GetDeviceID() == DEVICE_ID_IO4KUFC)
				if (!WriteRegister(kRegCh1ColorCorrectionControl, 0x0, kRegMaskLUT5Select, kRegShiftLUT5Select))
					return false;
			//Configure LUTs 1 and 2
			return WriteRegister(kRegCh1ColorCorrectionControl, NTV2_LUTCONTROL_1_2, kRegMaskLUTSelect, kRegShiftLUTSelect)
				&& WriteRegister (kRegGlobalControl, inValue, kRegMaskCCHostBankSelect, kRegShiftCCHostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH3BANK0:
		case NTV2_CCHOSTACCESS_CH3BANK1:
		case NTV2_CCHOSTACCESS_CH4BANK0:
		case NTV2_CCHOSTACCESS_CH4BANK1:
			if (::NTV2DeviceGetNumLUTs(GetDeviceID()) == 5 || GetDeviceID() == DEVICE_ID_IO4KUFC)
				if (!WriteRegister(kRegCh1ColorCorrectionControl, 0x0, kRegMaskLUT5Select, kRegShiftLUT5Select))
					return false;
			//Configure LUTs 3 and 4
			return WriteRegister(kRegCh1ColorCorrectionControl, NTV2_LUTCONTROL_3_4, kRegMaskLUTSelect, kRegShiftLUTSelect)
				&& WriteRegister (kRegCh1ColorCorrectionControl, (inValue - NTV2_CCHOSTACCESS_CH3BANK0), kRegMaskCCHostBankSelect, kRegShiftCCHostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH5BANK0:
		case NTV2_CCHOSTACCESS_CH5BANK1:
			return WriteRegister(kRegCh1ColorCorrectionControl, 0x0, kRegMaskLUTSelect, kRegShiftLUTSelect)
				&& WriteRegister (kRegGlobalControl, 0x0, kRegMaskCCHostBankSelect, kRegShiftCCHostAccessBankSelect)
				&& WriteRegister(kRegCh1ColorCorrectionControl, 0x1, kRegMaskLUT5Select, kRegShiftLUT5Select)
				&& WriteRegister(kRegCh1ColorCorrectionControl, (inValue - NTV2_CCHOSTACCESS_CH5BANK0), kRegMaskCC5HostAccessBankSelect, kRegShiftCC5HostAccessBankSelect);

		default:	return false;
	}
}

bool CNTV2Card::SetLUTV2HostAccessBank (const NTV2ColorCorrectionHostAccessBank inValue)
{
	switch(inValue)
	{
		case NTV2_CCHOSTACCESS_CH1BANK0:
		case NTV2_CCHOSTACCESS_CH1BANK1:	return WriteRegister(kRegLUTV2Control,	(inValue - NTV2_CCHOSTACCESS_CH1BANK0),	kRegMaskLUT1HostAccessBankSelect,	kRegShiftLUT1HostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH2BANK0:
		case NTV2_CCHOSTACCESS_CH2BANK1:	return WriteRegister(kRegLUTV2Control,	(inValue - NTV2_CCHOSTACCESS_CH2BANK0),	kRegMaskLUT2HostAccessBankSelect,	kRegShiftLUT2HostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH3BANK0:
		case NTV2_CCHOSTACCESS_CH3BANK1:	return WriteRegister(kRegLUTV2Control,	(inValue - NTV2_CCHOSTACCESS_CH3BANK0),	kRegMaskLUT3HostAccessBankSelect,	kRegShiftLUT3HostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH4BANK0:
		case NTV2_CCHOSTACCESS_CH4BANK1:	return WriteRegister(kRegLUTV2Control,	(inValue - NTV2_CCHOSTACCESS_CH4BANK0),	kRegMaskLUT4HostAccessBankSelect,	kRegShiftLUT4HostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH5BANK0:
		case NTV2_CCHOSTACCESS_CH5BANK1:	return WriteRegister(kRegLUTV2Control,	(inValue - NTV2_CCHOSTACCESS_CH5BANK0),	kRegMaskLUT5HostAccessBankSelect,	kRegShiftLUT5HostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH6BANK0:
		case NTV2_CCHOSTACCESS_CH6BANK1:	return WriteRegister(kRegLUTV2Control,	(inValue - NTV2_CCHOSTACCESS_CH6BANK0),	kRegMaskLUT6HostAccessBankSelect,	kRegShiftLUT6HostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH7BANK0:
		case NTV2_CCHOSTACCESS_CH7BANK1:	return WriteRegister(kRegLUTV2Control,	(inValue - NTV2_CCHOSTACCESS_CH7BANK0),	kRegMaskLUT7HostAccessBankSelect,	kRegShiftLUT7HostAccessBankSelect);

		case NTV2_CCHOSTACCESS_CH8BANK0:
		case NTV2_CCHOSTACCESS_CH8BANK1:	return WriteRegister(kRegLUTV2Control,	(inValue - NTV2_CCHOSTACCESS_CH8BANK0),	kRegMaskLUT8HostAccessBankSelect,	kRegShiftLUT8HostAccessBankSelect);

		default:							return false;
	}
}

bool CNTV2Card::GetColorCorrectionHostAccessBank (NTV2ColorCorrectionHostAccessBank & outValue, const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	if(::NTV2DeviceGetLUTVersion(_boardID) == 2)
		return GetLUTV2HostAccessBank(outValue, inChannel);

	bool	result	(false);
	ULWord	regValue(0);
	switch(inChannel)
	{
		case NTV2_CHANNEL1:
		case NTV2_CHANNEL2:
			return CNTV2DriverInterface::ReadRegister (kRegGlobalControl, outValue, kRegMaskCCHostBankSelect, kRegShiftCCHostAccessBankSelect);

		case NTV2_CHANNEL3:
		case NTV2_CHANNEL4:
			result = ReadRegister (kRegCh1ColorCorrectionControl, regValue, kRegMaskCCHostBankSelect, kRegShiftCCHostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(regValue + NTV2_CCHOSTACCESS_CH3BANK0);
			break;

		case NTV2_CHANNEL5:
			result = ReadRegister (kRegCh1ColorCorrectionControl, regValue, kRegMaskCC5HostAccessBankSelect, kRegShiftCC5HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(regValue + NTV2_CCHOSTACCESS_CH5BANK0);
			break;

		default:	break;
	}
	return result;
}

bool CNTV2Card::GetLUTV2HostAccessBank (NTV2ColorCorrectionHostAccessBank & outValue, const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;

	ULWord	tempVal	(0);
	bool	result	(false);
	switch(inChannel)
	{
		case NTV2_CHANNEL1:
			result = CNTV2DriverInterface::ReadRegister (kRegLUTV2Control, outValue,  kRegMaskLUT1HostAccessBankSelect,  kRegShiftLUT1HostAccessBankSelect);
			break;

		case NTV2_CHANNEL2:
			result = ReadRegister (kRegLUTV2Control, tempVal,  kRegMaskLUT2HostAccessBankSelect,  kRegShiftLUT2HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH2BANK0);
			break;

		case NTV2_CHANNEL3:
			result = ReadRegister (kRegLUTV2Control,  tempVal,  kRegMaskLUT3HostAccessBankSelect,  kRegShiftLUT3HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH3BANK0);
			break;

		case NTV2_CHANNEL4:
			result = ReadRegister (kRegLUTV2Control,  tempVal,  kRegMaskLUT4HostAccessBankSelect,  kRegShiftLUT4HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH4BANK0);
			break;

		case NTV2_CHANNEL5:
			result = ReadRegister (kRegLUTV2Control,  tempVal,  kRegMaskLUT5HostAccessBankSelect,  kRegShiftLUT5HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH5BANK0);
			break;

		case NTV2_CHANNEL6:
			result = ReadRegister (kRegLUTV2Control,  tempVal,  kRegMaskLUT6HostAccessBankSelect,  kRegShiftLUT6HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH6BANK0);
			break;

		case NTV2_CHANNEL7:
			result = ReadRegister (kRegLUTV2Control,  tempVal,  kRegMaskLUT7HostAccessBankSelect,  kRegShiftLUT7HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH7BANK0);
			break;

		case NTV2_CHANNEL8:
			result = ReadRegister (kRegLUTV2Control,  tempVal,  kRegMaskLUT8HostAccessBankSelect,  kRegShiftLUT8HostAccessBankSelect);
			outValue = NTV2ColorCorrectionHostAccessBank(tempVal + NTV2_CCHOSTACCESS_CH8BANK0);
			break;

		default:	break;
	}
	return result;
}

bool CNTV2Card::SetColorCorrectionSaturation (const NTV2Channel inChannel, const ULWord inValue)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;

	return WriteRegister (inChannel == NTV2_CHANNEL1  ?  kRegCh1ColorCorrectionControl  :  kRegCh2ColorCorrectionControl,
							inValue,  kRegMaskSaturationValue,  kRegShiftSaturationValue);
}

bool CNTV2Card::GetColorCorrectionSaturation (const NTV2Channel inChannel, ULWord & outValue)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;

	return ReadRegister (inChannel == NTV2_CHANNEL1  ?  kRegCh1ColorCorrectionControl  :  kRegCh2ColorCorrectionControl,
						outValue,  kRegMaskSaturationValue,  kRegShiftSaturationValue);
}

bool CNTV2Card::SetLUTControlSelect(NTV2LUTControlSelect lutSelect)		{return WriteRegister(kRegCh1ColorCorrectionControl,	(ULWord)lutSelect,	kRegMaskLUTSelect,	kRegShiftLUTSelect);}

bool CNTV2Card::GetLUTControlSelect(NTV2LUTControlSelect & outLUTSelect)
{
	return CNTV2DriverInterface::ReadRegister (kRegCh1ColorCorrectionControl, outLUTSelect, kRegMaskLUTSelect, kRegShiftLUTSelect);
}


///////////////////////////////////////////////////////////////////


bool CNTV2Card::SetColorSpaceMethod (const NTV2ColorSpaceMethod inCSCMethod, const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	if (::NTV2DeviceGetNumCSCs (_boardID) == 0)
		return false;

	if (::NTV2DeviceCanDoEnhancedCSC (_boardID))
	{
		ULWord value (0);

		switch (inCSCMethod)
		{
		case NTV2_CSC_Method_Original:
			//	Disable enhanced mode and 4K
			break;
		case NTV2_CSC_Method_Enhanced:
			//	Enable enhanced mode, but not 4K
			value |= kK2RegMaskEnhancedCSCEnable;
			break;
		case NTV2_CSC_Method_Enhanced_4K:
			//	4K mode uses a block of four CSCs. You must set the first converter in the group.
			if ((inChannel != NTV2_CHANNEL1) && (inChannel != NTV2_CHANNEL5))
				return false;
			//	Enable both enhanced mode and 4K
			value |= kK2RegMaskEnhancedCSCEnable | kK2RegMaskEnhancedCSC4KMode;
			break;
		default:
			return false;
		}

		//	Send the new control value to the hardware
		WriteRegister (gChannelToEnhancedCSCRegNum [inChannel], value, kK2RegMaskEnhancedCSCEnable | kK2RegMaskEnhancedCSC4KMode);

		return true;
	}
	else
	{
		//	It's not an error to set the original converters to the original method
		if (inCSCMethod == NTV2_CSC_Method_Original)
			return true;
	}

	return false;
}

bool CNTV2Card::GetColorSpaceMethod (NTV2ColorSpaceMethod & outMethod, const NTV2Channel inChannel)
{
	outMethod = NTV2_CSC_Method_Unimplemented;
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	if (!::NTV2DeviceGetNumCSCs(_boardID))
		return false;

	outMethod = NTV2_CSC_Method_Original;
	if (!::NTV2DeviceCanDoEnhancedCSC(_boardID))
		return true;

	ULWord tempVal (0);

	//	Check the group leader first, since that's where the 4K status bit is for all...
	if (!ReadRegister (gChannelToEnhancedCSCRegNum[inChannel < NTV2_CHANNEL5 ? NTV2_CHANNEL1 : NTV2_CHANNEL5],  tempVal,  kK2RegMaskEnhancedCSCEnable | kK2RegMaskEnhancedCSC4KMode))
		return false;

	if (tempVal == (kK2RegMaskEnhancedCSCEnable | kK2RegMaskEnhancedCSC4KMode))
	{
		outMethod = NTV2_CSC_Method_Enhanced_4K;		//	The leader is in 4K, so the other group members are as well
		return true;
	}

	//	Each CSC is operating independently, so read the control bits for the given channel
	if (!ReadRegister (gChannelToEnhancedCSCRegNum[inChannel], tempVal, kK2RegMaskEnhancedCSCEnable | kK2RegMaskEnhancedCSC4KMode))
		return false;

	if (tempVal & kK2RegMaskEnhancedCSCEnable)
		outMethod = NTV2_CSC_Method_Enhanced;
	return true;
}


NTV2ColorSpaceMethod CNTV2Card::GetColorSpaceMethod (const NTV2Channel inChannel)
{
	NTV2ColorSpaceMethod	result	(NTV2_CSC_Method_Unimplemented);
	GetColorSpaceMethod(result, inChannel);
	return result;
}


bool CNTV2Card::SetColorSpaceMatrixSelect (NTV2ColorSpaceMatrixType type, NTV2Channel channel)
{
	if (IS_CHANNEL_INVALID (channel))
		return false;
	return WriteRegister (gChannelToCSCoeff12RegNum [channel], type, kK2RegMaskColorSpaceMatrixSelect, kK2RegShiftColorSpaceMatrixSelect);
}


bool CNTV2Card::GetColorSpaceMatrixSelect (NTV2ColorSpaceMatrixType & outType, const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return CNTV2DriverInterface::ReadRegister (gChannelToCSCoeff12RegNum[inChannel], outType, kK2RegMaskColorSpaceMatrixSelect, kK2RegShiftColorSpaceMatrixSelect);
}


//	STATIC:
bool CNTV2Card::GenerateGammaTable (const NTV2LutType inLUTType, const int inBank, NTV2DoubleArray & outTable)
{
	outTable.reserve(1024);
	while (outTable.size() < 1024)
		outTable.push_back(double(0.0));

	size_t ndx(0);
	static const double kGammaMac(1.8);
	double gamma1(0.0), gamma2(0.0), scale(0.0);
	// Notes
	//
	//	1020 / 0x3FC is full-range white on the wire since 0x3FF/1023 is illegal
	//	   4 /   0x4 is full-range black on the wire since 0x0/0 is illegal
	//	 940 / 0x3AC is smpte-range white
	//	  64 /  0x40 is smpte-range black

	switch (inLUTType)
	{
		// Linear
		case NTV2_LUTLinear:
		case NTV2_LUTUnknown:	// huh?
		case NTV2_LUTCustom:
		default:
			for (ndx = 0;  ndx < 1024;  ndx++)
				outTable[ndx] = double(ndx);
			break;

		// RGB Full Range <=> SMPTE Range
		case NTV2_LUTRGBRangeFull_SMPTE:
			if (inBank == kLUTBank_FULL2SMPTE)
			{
				scale = (940.0 - 64.0) / (1023.0 - 0.0);
				
				for (ndx = 0;  ndx < 1024;  ndx++)
					outTable[ndx] = (double(ndx) * scale) + (64.0 - (scale * 0.0));
			}
			else  // inBank == kLUTBank_SMPTE2FULL
			{
				scale = (1023.0 - 0.0) / (940.0 - 64.0);

				for (ndx = 0;  ndx < 64;  ndx++)
					outTable[ndx] = 0.0;
				
				for (ndx = 64;  ndx < 940;  ndx++)
					outTable[ndx] = (double(ndx) * scale) + (0.0 - (scale * 64.0));
					
				for (ndx = 940;  ndx < 1024;  ndx++)
					outTable[ndx] = 1023.0;
			}
			break;	//	NTV2_LUTRGBRangeFull_SMPTE
			
		// kGammaMac <=> Rec 601 Gamma - Full Range
		case NTV2_LUTGamma18_Rec601:
			// "gamma" (the exponent) = srcGamma / dstGamma
			gamma1 = (inBank == kLUTBank_RGB2YUV ? (kGammaMac / 2.2) : (2.2 / kGammaMac) );
			for (ndx = 0;  ndx < 1024;  ndx++)
				outTable[ndx] = 1023.0 * ::pow(double(ndx) / 1023.0, gamma1);
			break;
		
		// kGammaMac <=> Rec 601 Gamma - SMPTE Range
		case NTV2_LUTGamma18_Rec601_SMPTE:
			// "gamma" (the exponent) = srcGamma / dstGamma
			gamma1 = (inBank == kLUTBank_RGB2YUV ? (kGammaMac / 2.2) : (2.2 / kGammaMac) );
			for (ndx = 0;  ndx < 1024;  ndx++)
			{
				if (ndx <= 64  ||  ndx >= 940)
					outTable[ndx] = double(ndx);
				else
					outTable[ndx] = 875.0 * ::pow((double(ndx) - 64.0) / 875.0, gamma1) + 64.0;
			}
			break;	//	NTV2_LUTGamma18_Rec601_SMPTE
			
		// kGammaMac <=> Rec 709 Gamma - Full Range
		case NTV2_LUTGamma18_Rec709:
			if (inBank == kLUTBank_RGB2YUV)
			{
				gamma1 = kGammaMac;
				gamma2 = 0.45;
				for (ndx = 0;  ndx < 1024;  ndx++)
				{	// remove the kGammaMac power gamma
					double f(::pow(double(ndx) / 1023.0, gamma1));

					// add the Rec 709 gamma
					if (f < 0.018)
						outTable[ndx] = 1023.0 * (f * 4.5);
					else
						outTable[ndx] = 1023.0 * ((1.099 * ::pow(f, gamma2)) - 0.099);
				}
			}
			else
			{
				gamma1 = 1.0 / 0.45;
				gamma2 = 1.0 / kGammaMac;
				for (ndx = 0;  ndx < 1024;  ndx++)
				{
					double f(double(ndx) / 1023.0);
					// remove the Rec 709 gamma
					if (f < 0.081)
						f = f / 4.5;
					else
						f = ::pow((f + 0.099) / 1.099, gamma1);

					// add the kGammaMac Power gamma
					outTable[ndx] = 1023.0 * ::pow(f, gamma2);
				}
			}
			break;	//	NTV2_LUTGamma18_Rec709
			
		// kGammaMac <=> Rec 709 Gamma - SMPTE Range
		case NTV2_LUTGamma18_Rec709_SMPTE:
			if (inBank == kLUTBank_RGB2YUV)
			{
				gamma1 = kGammaMac;
				gamma2 = 0.45;
				for (ndx = 0;  ndx < 1024;  ndx++)
				{
					if (ndx <= 64  ||  ndx >= 940)
						outTable[ndx] = double(ndx);	// linear portion - outside SMPTE range
					else
					{	// remove the kGammaMac power gamma
						double f(::pow((double(ndx) - 64.0) / 875.0, gamma1));

						// add the Rec 709 gamma
						if (f < 0.018)
							outTable[ndx] = 875.0 * (f * 4.5) + 64.0;
						else
							outTable[ndx] = 875.0 * ((1.099 * ::pow(f, gamma2)) - 0.099) + 64.0;
					}
				}
			}
			else
			{
				gamma1 = 1.0 / 0.45;
				gamma2 = 1.0 / kGammaMac;
				for (ndx = 0;  ndx < 1024;  ndx++)
				{
					if (ndx <= 64  ||  ndx >= 940)
						outTable[ndx] = double(ndx);	// linear portion - outside SMPTE range
					else
					{
						double f ((double(ndx) - 64.0) / 875.0);
						// remove the Rec 709 gamma
						if (f < 0.081)
							f = f / 4.5;
						else
							f = ::pow((f + 0.099) / 1.099, gamma1);

						// add the kGammaMac Power gamma
						outTable[ndx] = 875.0 * ::pow(f, gamma2) + 64.0;
					}
				}
			}
			break;	//	NTV2_LUTGamma18_Rec709_SMPTE
	}	//	switch on inLUTType
	return true;
}	//	GenerateGammaTable

static inline ULWord intClamp (const ULWord inMin, const ULWord inValue, const ULWord inMax)
{
	return inValue < inMin ? inMin : (inValue > inMax ? inMax : inValue);
}

bool CNTV2Card::GenerateGammaTable (const NTV2LutType inLUTType, const int inBank, UWordSequence & outTable)
{
	NTV2DoubleArray dblTable;
	size_t nonzeroes(0);
	if (!CNTV2Card::GenerateGammaTable (inLUTType, inBank, dblTable))
		return false;
	if (dblTable.size() < 1024)
		return false;
	outTable.reserve(1024);
	while (outTable.size() < 1024)
		outTable.push_back(0);
	for (size_t ndx(0);  ndx < 1024;  ndx++)
		if ((outTable.at(ndx) = UWord(intClamp(0UL, ULWord(dblTable.at(ndx) + 0.5), 1023UL))))
			nonzeroes++;
	NTV2_ASSERT(nonzeroes >= 1023);
	return nonzeroes >= 1023;
}

static const NTV2ColorCorrectionHostAccessBank	gLUTBank0[] =
{	NTV2_CCHOSTACCESS_CH1BANK0,	NTV2_CCHOSTACCESS_CH2BANK0,		//	[0]	0		[1]	2
	NTV2_CCHOSTACCESS_CH3BANK0,	NTV2_CCHOSTACCESS_CH4BANK0,		//	[2]	4		[3]	6
	NTV2_CCHOSTACCESS_CH5BANK0,	NTV2_CCHOSTACCESS_CH6BANK0,		//	[4]	8		[5]	10
	NTV2_CCHOSTACCESS_CH7BANK0,	NTV2_CCHOSTACCESS_CH8BANK0	};	//	[6]	12		[7]	14

static const size_t	kLUTArraySize (NTV2_COLORCORRECTOR_WORDSPERTABLE * 2);



// this allows for three 1024-entry LUTs that we're going to download to all four channels
bool CNTV2Card::DownloadLUTToHW (const NTV2DoubleArray & inRedLUT, const NTV2DoubleArray & inGreenLUT, const NTV2DoubleArray & inBlueLUT, const NTV2Channel inChannel, const int inBank)
{
	if (inRedLUT.size() < kLUTArraySize  ||  inGreenLUT.size() < kLUTArraySize  ||  inBlueLUT.size() < kLUTArraySize)
		{LUTFAIL("Size error (< 1024): R=" << DEC(inRedLUT.size()) << " G=" << DEC(inGreenLUT.size()) << " B=" << DEC(inBlueLUT.size())); return false;}

	if (IS_CHANNEL_INVALID(inChannel))
		{LUTFAIL("Bad LUT/channel (> 7): " << DEC(inChannel)); return false;}

	if (inBank != 0 && inBank != 1)
		{LUTFAIL("Bad bank value (> 1): " << DEC(inBank)); return false;}

	if (::NTV2DeviceGetNumLUTs(_boardID) == 0)
		return true;	//	It's not a sin to have been born with no LUTs

	bool bResult = SetLUTEnable(true, inChannel);
	if (bResult)
	{
		//	Set up Host Access...
		bResult = SetColorCorrectionHostAccessBank (NTV2ColorCorrectionHostAccessBank (gLUTBank0[inChannel] + inBank));
		if (bResult)
			bResult = LoadLUTTables (inRedLUT, inGreenLUT, inBlueLUT);
		SetLUTEnable (false, inChannel);
	}
	return bResult;
}

bool CNTV2Card::LoadLUTTables (const NTV2DoubleArray & inRedLUT, const NTV2DoubleArray & inGreenLUT, const NTV2DoubleArray & inBlueLUT)
{
	if (inRedLUT.size() < kLUTArraySize  ||  inGreenLUT.size() < kLUTArraySize  ||  inBlueLUT.size() < kLUTArraySize)
		{LUTFAIL("Size error (< 1024): R=" << DEC(inRedLUT.size()) << " G=" << DEC(inGreenLUT.size()) << " B=" << DEC(inBlueLUT.size())); return false;}

	UWordSequence redLUT, greenLUT, blueLUT;
	redLUT.resize(kLUTArraySize);
	greenLUT.resize(kLUTArraySize);
	blueLUT.resize(kLUTArraySize);
	for (size_t ndx(0);  ndx < kLUTArraySize;  ndx++)
	{
		redLUT  .push_back(UWord(intClamp(0, ULWord(inRedLUT  [ndx] + 0.5), 1023)));
		greenLUT.push_back(UWord(intClamp(0, ULWord(inGreenLUT[ndx] + 0.5), 1023)));
		blueLUT .push_back(UWord(intClamp(0, ULWord(inBlueLUT [ndx] + 0.5), 1023)));
	}
	return WriteLUTTables(redLUT, greenLUT, blueLUT);
}

bool CNTV2Card::WriteLUTTables (const UWordSequence & inRedLUT, const UWordSequence & inGreenLUT, const UWordSequence & inBlueLUT)
{
	if (inRedLUT.size() < kLUTArraySize  ||  inGreenLUT.size() < kLUTArraySize  ||  inBlueLUT.size() < kLUTArraySize)
		{LUTFAIL("Size error (< 1024): R=" << DEC(inRedLUT.size()) << " G=" << DEC(inGreenLUT.size()) << " B=" << DEC(inBlueLUT.size())); return false;}

	size_t	errorCount(0), nonzeroes(0);
	ULWord	RTableReg(kColorCorrectionLUTOffset_Red / 4);	//	Byte offset to LUT in register bar;  divide by sizeof (ULWord) to get register number
	ULWord	GTableReg(kColorCorrectionLUTOffset_Green / 4);
	ULWord	BTableReg(kColorCorrectionLUTOffset_Blue / 4);

	for (size_t ndx(0);  ndx < NTV2_COLORCORRECTOR_WORDSPERTABLE;  ndx++)
	{
		ULWord	lo(ULWord(inRedLUT[2 * ndx + 0]) & 0x3FF);
		ULWord	hi(ULWord(inRedLUT[2 * ndx + 1]) & 0x3FF);
		ULWord	tmp((hi << kRegColorCorrectionLUTOddShift) + (lo << kRegColorCorrectionLUTEvenShift));
		if (tmp) nonzeroes++;
		if (!WriteRegister(RTableReg++, tmp))
			errorCount++;

		lo = ULWord(inGreenLUT[2 * ndx + 0]) & 0x3FF;
		hi = ULWord(inGreenLUT[2 * ndx + 1]) & 0x3FF;
		tmp = (hi << kRegColorCorrectionLUTOddShift) + (lo << kRegColorCorrectionLUTEvenShift);
		if (tmp) nonzeroes++;
		if (!WriteRegister(GTableReg++, tmp))
			errorCount++;

		lo = ULWord(inBlueLUT[2 * ndx + 0]) & 0x3FF;
		hi = ULWord(inBlueLUT[2 * ndx + 1]) & 0x3FF;
		tmp = (hi << kRegColorCorrectionLUTOddShift) + (lo << kRegColorCorrectionLUTEvenShift);
		if (tmp) nonzeroes++;
		if (!WriteRegister(BTableReg++, tmp))
			errorCount++;
	}
	if (errorCount)	LUTFAIL(DEC(errorCount) << " WriteRegister calls failed");
	else if (!nonzeroes) LUTWARN("All zero LUT table values!");
	return !errorCount;
}

bool CNTV2Card::GetLUTTables (NTV2DoubleArray & outRedLUT, NTV2DoubleArray & outGreenLUT, NTV2DoubleArray & outBlueLUT)
{
	outRedLUT.clear();		outRedLUT.resize (kLUTArraySize);
	outGreenLUT.clear();	outGreenLUT.resize(kLUTArraySize);
	outBlueLUT.clear();		outBlueLUT.resize(kLUTArraySize);

	UWordSequence red, green, blue;
	if (!ReadLUTTables(red, green, blue))
		return false;
	if (red.size() != green.size() || green.size() != blue.size())
		{LUTFAIL("Unexpected size mismatch: R(" << DEC(red.size()) << ")!=G(" << DEC(green.size()) << ")!=B(" << DEC(blue.size()) << ")"); return false;}
	if (red.size() != outRedLUT.size() || green.size() != outGreenLUT.size() || blue.size() != outBlueLUT.size())
		{LUTFAIL("Unexpected size mismatch: R(" << DEC(red.size()) << ")!=oR(" << DEC(outRedLUT.size())
				<< ") G(" << DEC(green.size()) << ")!=oG(" << DEC(outGreenLUT.size())
				<< ") B(" << DEC(blue.size()) << ")!=oB(" << DEC(outBlueLUT.size())
				<< ")"); return false;}

	for (size_t ndx(0);  ndx < kLUTArraySize;  ndx++)
	{
		outRedLUT  [ndx] = red[ndx];
		outGreenLUT[ndx] = green[ndx];
		outBlueLUT [ndx] = blue[ndx];
	}
	return true;
}

bool CNTV2Card::ReadLUTTables (UWordSequence & outRedLUT, UWordSequence & outGreenLUT, UWordSequence & outBlueLUT)
{
	ULWord	RTableReg	(kColorCorrectionLUTOffset_Red / 4);	//	Byte offset to LUT in register bar;  divide by sizeof (ULWord) to get register number
	ULWord	GTableReg	(kColorCorrectionLUTOffset_Green / 4);
	ULWord	BTableReg	(kColorCorrectionLUTOffset_Blue / 4);
	size_t	errors(0), nonzeroes(0);

	outRedLUT.clear();		outRedLUT.resize(kLUTArraySize);
	outGreenLUT.clear();	outGreenLUT.resize(kLUTArraySize);
	outBlueLUT.clear();		outBlueLUT.resize(kLUTArraySize);

	for (size_t ndx(0);  ndx < kLUTArraySize;  ndx += 2)
	{
		ULWord	temp(0);
		if (!ReadRegister(RTableReg++, temp))
			errors++;
		outRedLUT[ndx + 0] = (temp >> kRegColorCorrectionLUTEvenShift) & 0x3FF;
		outRedLUT[ndx + 1] = (temp >> kRegColorCorrectionLUTOddShift ) & 0x3FF;
		if (temp) nonzeroes++;

		if (!ReadRegister(GTableReg++, temp))
			errors++;
		outGreenLUT[ndx + 0] = (temp >> kRegColorCorrectionLUTEvenShift) & 0x3FF;
		outGreenLUT[ndx + 1] = (temp >> kRegColorCorrectionLUTOddShift ) & 0x3FF;
		if (temp) nonzeroes++;

		if (!ReadRegister(BTableReg++, temp))
			errors++;
		outBlueLUT[ndx + 0] = (temp >> kRegColorCorrectionLUTEvenShift) & 0x3FF;
		outBlueLUT[ndx + 1] = (temp >> kRegColorCorrectionLUTOddShift ) & 0x3FF;
		if (temp) nonzeroes++;
	}
	if (errors)	LUTFAIL(DEC(errors) << " ReadRegister calls failed");
	else if (!nonzeroes) LUTWARN("All zero LUT table values!");
	return !errors;
}


bool CNTV2Card::SetLUTEnable (const bool inEnable, const NTV2Channel inLUT)
{
	static const ULWord	LUTEnableMasks []	= {	kRegMaskLUT1Enable,		kRegMaskLUT2Enable,		kRegMaskLUT3Enable,		kRegMaskLUT4Enable,
												kRegMaskLUT5Enable,		kRegMaskLUT6Enable,		kRegMaskLUT7Enable,		kRegMaskLUT8Enable	};
	static const ULWord	LUTEnableShifts []	= {	kRegShiftLUT1Enable,	kRegShiftLUT2Enable,	kRegShiftLUT3Enable,	kRegShiftLUT4Enable,
												kRegShiftLUT5Enable,	kRegShiftLUT6Enable,	kRegShiftLUT7Enable,	kRegShiftLUT8Enable	};
	static const UWord	BitCountNibble[]	= {	0,	1,	1,	2,	1,	2,	2,	3,	1,	2,	2,	3,	2,	3,	3,	4};

	if (IS_CHANNEL_INVALID(inLUT))
		{LUTFAIL("Bad LUT number (> 7): " << DEC(inLUT)); return false;}
	if (::NTV2DeviceGetLUTVersion(_boardID) != 2)
		return true;	//	LUT init not needed

	//	Sanity check...
	const ULWord mask(LUTEnableMasks[inLUT]), shift(LUTEnableShifts[inLUT]); ULWord tmp(0);
	if (ReadRegister(kRegLUTV2Control, tmp))
		if (((tmp & mask)?true:false) == inEnable)
			LUTWARN("V2 LUT" << DEC(inLUT+1) << " Enable bit already " << (inEnable?"set":"clear"));
	tmp &= 0x000000FF;
	if (inEnable)
		if (BitCountNibble[tmp & 0xF] || BitCountNibble[(tmp >> 4) & 0xF])
			LUTWARN("Setting V2 LUT" << DEC(inLUT+1) << " Enable bit: multiple Enable bits set: " << xHEX0N(tmp,4));

	//	Set or Clear the Enable bit...
	if (!WriteRegister (kRegLUTV2Control, inEnable ? 1 : 0, mask, shift))
		{LUTFAIL("WriteRegister kRegLUTV2Control failed, enable=" << DEC(UWord(inEnable))); return false;}

	//	Sanity check...
	if (!inEnable)
		if (ReadRegister(kRegLUTV2Control, tmp, 0x000000FF))	//	all enable masks
			if (tmp)
				LUTWARN("Clearing V2 LUT" << DEC(inLUT+1) << " Enable bit: still has Enable bit(s) set: " << xHEX0N(tmp,4));

	return true;
}


bool CNTV2Card::SetColorSpaceRGBBlackRange (const NTV2_CSC_RGB_Range inRange,	const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	if (!NTV2_IS_VALID_CSCRGBRANGE(inRange))
		return false;
	return WriteRegister (gChannelToCSCoeff34RegNum[inChannel],  inRange,  kK2RegMaskXena2RGBRange,  kK2RegShiftXena2RGBRange);
}

bool CNTV2Card::GetColorSpaceRGBBlackRange (NTV2_CSC_RGB_Range & outRange,	const NTV2Channel inChannel)
{
	outRange = NTV2_CSC_RGB_RANGE_INVALID;
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	ULWord	regValue	(0);
	if (!ReadRegister  (gChannelToCSCoeff34RegNum[inChannel],  regValue,  kK2RegMaskXena2RGBRange,  kK2RegShiftXena2RGBRange))
		return false;
	outRange = NTV2_CSC_RGB_Range(regValue);
	return true;	
}

bool CNTV2Card::SetColorSpaceUseCustomCoefficient (ULWord useCustomCoefficient, const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	return WriteRegister (gChannelToCSCoeff12RegNum [inChannel],	useCustomCoefficient,	kK2RegMaskUseCustomCoefSelect,		kK2RegShiftUseCustomCoefSelect);
}

bool CNTV2Card::GetColorSpaceUseCustomCoefficient (ULWord & outUseCustomCoefficient, const NTV2Channel inChannel)
{
	return !IS_CHANNEL_INVALID(inChannel)
			&&  ReadRegister (gChannelToCSCoeff12RegNum[inChannel], outUseCustomCoefficient,  kK2RegMaskUseCustomCoefSelect,  kK2RegShiftUseCustomCoefSelect);
}

bool CNTV2Card::SetColorSpaceMakeAlphaFromKey (const bool inMakeAlphaFromKey, const NTV2Channel inChannel)
{
	return !IS_CHANNEL_INVALID(inChannel)
			&&  WriteRegister (gChannelToCSCoeff12RegNum[inChannel], inMakeAlphaFromKey?1:0, kK2RegMaskMakeAlphaFromKeySelect, kK2RegShiftMakeAlphaFromKeySelect);
}

bool CNTV2Card::GetColorSpaceMakeAlphaFromKey (ULWord & outMakeAlphaFromKey, const NTV2Channel inChannel)
{
	return !IS_CHANNEL_INVALID(inChannel)
			&&  ReadRegister (gChannelToCSCoeff12RegNum[inChannel], outMakeAlphaFromKey, kK2RegMaskMakeAlphaFromKeySelect, kK2RegShiftMakeAlphaFromKeySelect);
}

bool CNTV2Card::SetColorSpaceCustomCoefficients (const NTV2CSCCustomCoeffs & inCoefficients, const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID(inChannel))
		return false;

	return		WriteRegister (gChannelToCSCoeff12RegNum[inChannel],	inCoefficients.Coefficient1,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	WriteRegister (gChannelToCSCoeff12RegNum[inChannel],	inCoefficients.Coefficient2,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh)
			&&	WriteRegister (gChannelToCSCoeff34RegNum[inChannel],	inCoefficients.Coefficient3,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	WriteRegister (gChannelToCSCoeff34RegNum[inChannel],	inCoefficients.Coefficient4,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh)
			&&	WriteRegister (gChannelToCSCoeff56RegNum[inChannel],	inCoefficients.Coefficient5,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	WriteRegister (gChannelToCSCoeff56RegNum[inChannel],	inCoefficients.Coefficient6,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh)
			&&	WriteRegister (gChannelToCSCoeff78RegNum[inChannel],	inCoefficients.Coefficient7,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	WriteRegister (gChannelToCSCoeff78RegNum[inChannel],	inCoefficients.Coefficient8,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh)
			&&	WriteRegister (gChannelToCSCoeff910RegNum[inChannel],	inCoefficients.Coefficient9,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	WriteRegister (gChannelToCSCoeff910RegNum[inChannel],	inCoefficients.Coefficient10,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh);
}

bool CNTV2Card::GetColorSpaceCustomCoefficients(NTV2CSCCustomCoeffs & outCoefficients, NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;

	return		ReadRegister (gChannelToCSCoeff12RegNum[inChannel],		outCoefficients.Coefficient1,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	ReadRegister (gChannelToCSCoeff12RegNum[inChannel],		outCoefficients.Coefficient2,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh)
			&&	ReadRegister (gChannelToCSCoeff34RegNum[inChannel],		outCoefficients.Coefficient3,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	ReadRegister (gChannelToCSCoeff34RegNum[inChannel],		outCoefficients.Coefficient4,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh)
			&&	ReadRegister (gChannelToCSCoeff56RegNum[inChannel],		outCoefficients.Coefficient5,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	ReadRegister (gChannelToCSCoeff56RegNum[inChannel],		outCoefficients.Coefficient6,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh)
			&&	ReadRegister (gChannelToCSCoeff78RegNum[inChannel],		outCoefficients.Coefficient7,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	ReadRegister (gChannelToCSCoeff78RegNum[inChannel],		outCoefficients.Coefficient8,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh)
			&&	ReadRegister (gChannelToCSCoeff910RegNum[inChannel],	outCoefficients.Coefficient9,	kK2RegMaskCustomCoefficientLow,		kK2RegShiftCustomCoefficientLow)
			&&	ReadRegister (gChannelToCSCoeff910RegNum[inChannel],	outCoefficients.Coefficient10,	kK2RegMaskCustomCoefficientHigh,	kK2RegShiftCustomCoefficientHigh);
}

// 12/7/2006	To increase accuracy and decrease generational degredation, the width of the coefficients
//				for the colorspace converter matrix went from 10 to 12 bit (with the MSB being a sign
//				bit).  So as not to break existing, compiled code, we added new API calls for use with
//				these wider coefficients.  The values had to be munged a bit to fit in the register, since
//				the low coefficient ended on the 0 bit ... the hi coefficient was simply widened to 13 bits
//				total.  The 2 LSBs of the low coefficient are written *in front* of the 11 MSBs, hence the
//				shifting and masking below. - jac
bool CNTV2Card::SetColorSpaceCustomCoefficients12Bit (const NTV2CSCCustomCoeffs & inCoefficients, const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;

	ULWord MSBs(inCoefficients.Coefficient1 >> 2);
	ULWord LSBs(inCoefficients.Coefficient1 & 0x00000003);
	if (!WriteRegister (gChannelToCSCoeff12RegNum[inChannel],	MSBs | (LSBs << 11),			kK2RegMaskCustomCoefficient12BitLow,	kK2RegShiftCustomCoefficient12BitLow)
		||  !WriteRegister (gChannelToCSCoeff12RegNum[inChannel],	inCoefficients.Coefficient2,	kK2RegMaskCustomCoefficient12BitHigh,	kK2RegShiftCustomCoefficient12BitHigh))
			return false;

	MSBs = inCoefficients.Coefficient3 >> 2;
	LSBs = inCoefficients.Coefficient3 & 0x00000003;
	if (!WriteRegister (gChannelToCSCoeff34RegNum[inChannel],	MSBs | (LSBs << 11),			kK2RegMaskCustomCoefficient12BitLow,	kK2RegShiftCustomCoefficient12BitLow)
		||  !WriteRegister (gChannelToCSCoeff34RegNum[inChannel],	inCoefficients.Coefficient4,	kK2RegMaskCustomCoefficient12BitHigh,	kK2RegShiftCustomCoefficient12BitHigh))
			return false;

	MSBs = inCoefficients.Coefficient5 >> 2;
	LSBs = inCoefficients.Coefficient5 & 0x00000003;
	if (!WriteRegister (gChannelToCSCoeff56RegNum[inChannel],	MSBs | (LSBs << 11),			kK2RegMaskCustomCoefficient12BitLow,	kK2RegShiftCustomCoefficient12BitLow)
		||  !WriteRegister (gChannelToCSCoeff56RegNum[inChannel],	inCoefficients.Coefficient6,	kK2RegMaskCustomCoefficient12BitHigh,	kK2RegShiftCustomCoefficient12BitHigh))
			return false;

	MSBs = inCoefficients.Coefficient7 >> 2;
	LSBs = inCoefficients.Coefficient7 & 0x00000003;
	if (!WriteRegister (gChannelToCSCoeff78RegNum[inChannel],	MSBs | (LSBs << 11),			kK2RegMaskCustomCoefficient12BitLow,	kK2RegShiftCustomCoefficient12BitLow)
		||  !WriteRegister (gChannelToCSCoeff78RegNum[inChannel],	inCoefficients.Coefficient8,	kK2RegMaskCustomCoefficient12BitHigh,	kK2RegShiftCustomCoefficient12BitHigh))
			return false;

	MSBs = inCoefficients.Coefficient9 >> 2;
	LSBs = inCoefficients.Coefficient9 & 0x00000003;
	return WriteRegister (gChannelToCSCoeff910RegNum[inChannel],	MSBs | (LSBs << 11),			kK2RegMaskCustomCoefficient12BitLow,	kK2RegShiftCustomCoefficient12BitLow)
		&&  WriteRegister (gChannelToCSCoeff910RegNum[inChannel],	inCoefficients.Coefficient10,	kK2RegMaskCustomCoefficient12BitHigh,	kK2RegShiftCustomCoefficient12BitHigh);
}

// 12/7/2006	To increase accuracy and decrease generational degredation, the width of the coefficients
//				for the colorspace converter matrix went from 10 to 12 bit (with the MSB being a sign
//				bit).  So as not to break existing, compiled code, we added new API calls for use with
//				these wider coefficients.  The values had to be munged a bit to fit in the register, since
//				the low coefficient ended on the 0 bit ... the hi coefficient was simply widened to 13 bits
//				total.  The 2 LSBs of the low coefficient are written *in front* of the 11 MSBs, hence the
//				shifting and masking below. - jac
bool CNTV2Card::GetColorSpaceCustomCoefficients12Bit (NTV2CSCCustomCoeffs & outCoefficients, const NTV2Channel inChannel)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;

	ULWord regVal(0),  MSBs(0),  LSBs(0);

	if (!ReadRegister (gChannelToCSCoeff12RegNum[inChannel], regVal,  kK2RegMaskCustomCoefficient12BitLow, kK2RegShiftCustomCoefficient12BitLow))
		return false;
	LSBs = (regVal >> 11) & 0x00000003;
	MSBs = regVal & 0x000007FF;
	outCoefficients.Coefficient1 = MSBs | LSBs;

	if (!ReadRegister (gChannelToCSCoeff12RegNum[inChannel], outCoefficients.Coefficient2,  kK2RegMaskCustomCoefficient12BitHigh, kK2RegShiftCustomCoefficient12BitHigh)
		||  !ReadRegister (gChannelToCSCoeff34RegNum[inChannel], regVal,  kK2RegMaskCustomCoefficient12BitLow, kK2RegShiftCustomCoefficient12BitLow))
			return false;
	LSBs = (regVal >> 11) & 0x00000003;
	MSBs = regVal & 0x000007FF;
	outCoefficients.Coefficient3 = MSBs | LSBs;

	if (!ReadRegister (gChannelToCSCoeff34RegNum[inChannel], outCoefficients.Coefficient4,  kK2RegMaskCustomCoefficient12BitHigh, kK2RegShiftCustomCoefficient12BitHigh)
		||  !ReadRegister (gChannelToCSCoeff56RegNum[inChannel], regVal,  kK2RegMaskCustomCoefficient12BitLow, kK2RegShiftCustomCoefficient12BitLow))
			return false;
	LSBs = (regVal >> 11) & 0x00000003;
	MSBs = regVal & 0x000007FF;
	outCoefficients.Coefficient5 = MSBs | LSBs;

	if (!ReadRegister (gChannelToCSCoeff56RegNum[inChannel], outCoefficients.Coefficient6,  kK2RegMaskCustomCoefficient12BitHigh, kK2RegShiftCustomCoefficient12BitHigh)
		||  !ReadRegister (gChannelToCSCoeff78RegNum[inChannel], regVal,  kK2RegMaskCustomCoefficient12BitLow, kK2RegShiftCustomCoefficient12BitLow))
			return false;
	LSBs = (regVal >> 11) & 0x00000003;
	MSBs = regVal & 0x000007FF;
	outCoefficients.Coefficient7 = MSBs | LSBs;

	if (!ReadRegister (gChannelToCSCoeff78RegNum[inChannel], outCoefficients.Coefficient8,  kK2RegMaskCustomCoefficient12BitHigh, kK2RegShiftCustomCoefficient12BitHigh)
		||  !ReadRegister (gChannelToCSCoeff910RegNum[inChannel], regVal,  kK2RegMaskCustomCoefficient12BitLow, kK2RegShiftCustomCoefficient12BitLow))
			return false;
	LSBs = (regVal >> 11) & 0x00000003;
	MSBs = regVal & 0x000007FF;
	outCoefficients.Coefficient9 = MSBs | LSBs;
	return ReadRegister (gChannelToCSCoeff910RegNum[inChannel], outCoefficients.Coefficient10,  kK2RegMaskCustomCoefficient12BitHigh, kK2RegShiftCustomCoefficient12BitHigh);
}


bool CNTV2Card::GetColorSpaceVideoKeySyncFail (bool & outVideoKeySyncFail, const NTV2Channel inChannel)
{
	ULWord value(0);
	const bool status (!IS_CHANNEL_INVALID(inChannel)  &&  ReadRegister(gChannelToCSCoeff12RegNum[inChannel], value, kK2RegMaskVidKeySyncStatus, kK2RegShiftVidKeySyncStatus));
	outVideoKeySyncFail = (value == 1);
	return status;
}

//////////////////////////////////////////////////////////////  OLD APIs


bool CNTV2Card::GenerateGammaTable (const NTV2LutType inLUTType, const int inBank, double * pOutTable)
{
	if (!pOutTable)
		return false;
	NTV2DoubleArray	table;
	if (!GenerateGammaTable(inLUTType, inBank, table))
		return false;
	::memcpy(pOutTable, &table[0], table.size() * sizeof(double));
	return true;
}

// this assumes we have one 1024-entry LUT that we're going to download to all four channels
bool CNTV2Card::DownloadLUTToHW (const double * pInTable, const NTV2Channel inChannel, const int inBank)
{
	if (IS_CHANNEL_INVALID (inChannel))
		return false;	//	Invalid channel

	if (!pInTable)
		return false;	//	NULL table pointer

	if (inBank != 0  && inBank != 1)
		return false;	//	Bad bank value (must be 0 or 1)

	if (::NTV2DeviceGetNumLUTs (_boardID) == 0)
		return true;	//	It's not a sin to have been born without any LUTs

	bool bResult = SetLUTEnable (true, inChannel);
	if (bResult)
	{
		//	Set up Host Access...
		bResult = SetColorCorrectionHostAccessBank (NTV2ColorCorrectionHostAccessBank (gLUTBank0 [inChannel] + inBank));
		if (bResult)
			bResult = LoadLUTTable (pInTable);
		SetLUTEnable (false, inChannel);
	}
	return bResult;
}


bool CNTV2Card::LoadLUTTable (const double * pInTable)
{
	if (!pInTable)
		return false;

	//	Hope and pray that the caller's array has at least 1024 elements...
	NTV2DoubleArray rgbLUT;
	rgbLUT.resize(kLUTArraySize);
	for (size_t ndx(0);  ndx < kLUTArraySize;  ndx++)
		rgbLUT.push_back(pInTable[ndx]);

	//	Call the function that accepts NTV2DoubleArrays...
	return LoadLUTTables(rgbLUT, rgbLUT, rgbLUT);
}


#if !defined (NTV2_DEPRECATE)
	// Deprecated: now handled in mac every-frame task
	// based on the user ColorSpace mode and the current video format,
	// set the ColorSpaceMatrixSelect
	bool CNTV2Card::UpdateK2ColorSpaceMatrixSelect(NTV2VideoFormat currFormat, bool ajaRetail)
	{
	#ifdef  MSWindows
		NTV2EveryFrameTaskMode mode;
		GetEveryFrameServices(&mode);
		if(mode == NTV2_STANDARD_TASKS)
			ajaRetail = true;
	#endif

		bool bResult = true;
		
		// Looks like this is only happening on the Mac
		if (ajaRetail == true)
		{
			// if the board doesn't have LUTs, bail
			if ( !::NTV2DeviceCanDoProgrammableCSC(_boardID) )
				return bResult;
				
			int numberCSCs = ::NTV2DeviceGetNumCSCs(_boardID);

			// get the current user mode
			NTV2ColorSpaceType cscType = NTV2_ColorSpaceTypeRec601;
			ReadRegister(kVRegColorSpaceMode, (ULWord *)&cscType);
			
			// if the current video format wasn't passed in to us, go get it
			NTV2VideoFormat vidFormat = currFormat;
			if (vidFormat == NTV2_FORMAT_UNKNOWN)
				GetVideoFormat(&vidFormat);
			
			// figure out what ColorSpace we want to be using
			NTV2ColorSpaceMatrixType matrix = NTV2_Rec601Matrix;
			switch (cscType)
			{
				// force to Rec 601
				default:	
				case NTV2_ColorSpaceTypeRec601:
					matrix = NTV2_Rec601Matrix;
					break;
				
				// force to Rec 709
				case NTV2_ColorSpaceTypeRec709:
					matrix = NTV2_Rec709Matrix;
					break;
				
				// Auto-switch between SD (Rec 601) and HD (Rec 709)
				case NTV2_ColorSpaceTypeAuto:
					if (NTV2_IS_SD_VIDEO_FORMAT(vidFormat) )
						matrix = NTV2_Rec601Matrix;
					else
						matrix = NTV2_Rec709Matrix;
					break;
			}
			
			// set matrix
			bResult = SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL1);
			if (numberCSCs >= 2)
				bResult = SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL2);
			if (numberCSCs >= 4)
			{
				bResult = SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL3);
				bResult = SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL4);
			}
			if (numberCSCs >= 5)
			{
				bResult = SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL5);
			}
			
			// get csc rgb range
			ULWord cscRange = NTV2_RGBRangeFull;
			ReadRegister(kVRegRGBRangeMode, &cscRange);
			NTV2RGBBlackRange blackRange =  cscRange == NTV2_RGBRangeFull ? 
											NTV2_CSC_RGB_RANGE_FULL : NTV2_CSC_RGB_RANGE_SMPTE;
			
			// set csc rgb range
			bResult = SetColorSpaceRGBBlackRange(blackRange, NTV2_CHANNEL1);
			if (numberCSCs >= 2)
				bResult = SetColorSpaceRGBBlackRange(blackRange, NTV2_CHANNEL2);
			if (numberCSCs >= 4)
			{
				bResult = SetColorSpaceRGBBlackRange(blackRange, NTV2_CHANNEL3);
				bResult = SetColorSpaceRGBBlackRange(blackRange, NTV2_CHANNEL4);
			}
			if (numberCSCs >= 5)
			{
				bResult = SetColorSpaceRGBBlackRange(blackRange, NTV2_CHANNEL5);
			}
		}
		
		return bResult;
	}

	// Deprecated: now handled in mac every-frame task
	// based on the user Gamma mode, the current video format,
	// load the LUTs with an appropriate gamma translation
	bool CNTV2Card::UpdateK2LUTSelect(NTV2VideoFormat currFormat, bool ajaRetail)
	{
	#ifdef  MSWindows
		NTV2EveryFrameTaskMode mode;
		GetEveryFrameServices(&mode);
		if(mode == NTV2_STANDARD_TASKS)
			ajaRetail = true;
	#endif

		bool bResult = true;
		
		// Looks like this is only happening on the Mac
		if (ajaRetail == true)
		{
			printf ("UpdateK2LUTSelect deprecated!!!\n");
		
			// if the board doesn't have LUTs, bail
			if ( !::NTV2DeviceCanDoColorCorrection(_boardID) )
				return bResult;
			
			int numberLUTs = ::NTV2DeviceGetNumLUTs(_boardID);

			// get the current user mode
			NTV2GammaType gammaType = NTV2_GammaNone;
			ReadRegister(kVRegGammaMode, (ULWord *)&gammaType);
			
			ULWord cscRange = NTV2_RGBRangeFull;
			ReadRegister(kVRegRGBRangeMode, &cscRange);
		
			// if the current video format wasn't passed in to us, go get it
			NTV2VideoFormat vidFormat = currFormat;
			if (vidFormat == NTV2_FORMAT_UNKNOWN)
				GetVideoFormat(&vidFormat);
		
			// figure out which gamma LUTs we WANT to be using
			NTV2LutType wantedLUT = NTV2_LUTUnknown;
			switch (gammaType)
			{
				// force to Rec 601
				case NTV2_GammaRec601:		
					wantedLUT = cscRange == NTV2_RGBRangeFull ? NTV2_LUTGamma18_Rec601 : NTV2_LUTGamma18_Rec601_SMPTE;	
					break;
			
				// force to Rec 709
				case NTV2_GammaRec709:		
					wantedLUT = cscRange == NTV2_RGBRangeFull ? NTV2_LUTGamma18_Rec709 : NTV2_LUTGamma18_Rec709_SMPTE;	
					break;
			
				// Auto-switch between SD (Rec 601) and HD (Rec 709)
				case NTV2_GammaAuto:		
					if (NTV2_IS_SD_VIDEO_FORMAT(vidFormat) )
						wantedLUT = cscRange == NTV2_RGBRangeFull ? NTV2_LUTGamma18_Rec601 : NTV2_LUTGamma18_Rec601_SMPTE;
					else
						wantedLUT = cscRange == NTV2_RGBRangeFull ? NTV2_LUTGamma18_Rec709 : NTV2_LUTGamma18_Rec709_SMPTE;
					break;
						
				// custom LUT in use - do not change
				case NTV2_GammaNone:		
					wantedLUT = NTV2_LUTCustom;
					break;
										
				default:
				case NTV2_GammaMac:			
					wantedLUT = NTV2_LUTLinear;
					break;
			}
			
			
			// special case: if RGB-to-RGB conversion is required
			NTV2LutType rgbConverterLUTType;
			ReadRegister(kVRegRGBRangeConverterLUTType, (ULWord *)&rgbConverterLUTType);	
			
			if (wantedLUT != NTV2_LUTCustom && rgbConverterLUTType != NTV2_LUTUnknown)
				wantedLUT = rgbConverterLUTType;
		
		
			// what LUT function is CURRENTLY loaded into hardware?
			NTV2LutType currLUT = NTV2_LUTUnknown;
			ReadRegister(kVRegLUTType, (ULWord *)&currLUT);
		
			if (wantedLUT != NTV2_LUTCustom && currLUT != wantedLUT)
			{
				// generate and download new LUTs
				//printf (" Changing from LUT %d to LUT %d  \n", currLUT, wantedLUT);
			
				double table[1024];
			
				// generate RGB=>YUV LUT function 
				GenerateGammaTable(wantedLUT, kLUTBank_RGB2YUV, table);
			
				// download it to HW
				DownloadLUTToHW(table, NTV2_CHANNEL1, kLUTBank_RGB2YUV);
				if (numberLUTs >= 2)
					DownloadLUTToHW(table, NTV2_CHANNEL2, kLUTBank_RGB2YUV);
				
				if (numberLUTs >= 4)
				{
					DownloadLUTToHW(table, NTV2_CHANNEL3, kLUTBank_RGB2YUV);
					DownloadLUTToHW(table, NTV2_CHANNEL4, kLUTBank_RGB2YUV);
				}

				if (numberLUTs >= 5)
				{
					DownloadLUTToHW(table, NTV2_CHANNEL5, kLUTBank_RGB2YUV);
				}

				// generate YUV=>RGB LUT function 
				GenerateGammaTable(wantedLUT, kLUTBank_YUV2RGB, table);
			
				// download it to HW
				DownloadLUTToHW(table, NTV2_CHANNEL1, kLUTBank_YUV2RGB);
				if (numberLUTs >= 2)
					DownloadLUTToHW(table, NTV2_CHANNEL2, kLUTBank_YUV2RGB);
				
				if (numberLUTs >= 4)
				{
					DownloadLUTToHW(table, NTV2_CHANNEL3, kLUTBank_YUV2RGB);
					DownloadLUTToHW(table, NTV2_CHANNEL4, kLUTBank_YUV2RGB);
				}

				if (numberLUTs >= 5)
				{
					DownloadLUTToHW(table, NTV2_CHANNEL5, kLUTBank_YUV2RGB);
				}
			
				WriteRegister(kVRegLUTType, wantedLUT);
			}
		}
		
		return bResult;
	}
#endif	//	!defined (NTV2_DEPRECATE)


//////////////////////////////////////////////////////////////

#ifdef MSWindows
#pragma warning(default: 4800)
#endif
