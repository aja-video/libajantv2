/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2regvpid.cpp
	@brief		CNTV2Card VPID API implementation.
	@copyright	(C) 2004-2023 AJA Video Systems, Inc.
**/

#include "ntv2card.h"

using namespace std;


static const ULWord gChannelToSDIOutVPIDARegNum []		= { kRegSDIOut1VPIDA, kRegSDIOut2VPIDA, kRegSDIOut3VPIDA, kRegSDIOut4VPIDA,
															kRegSDIOut5VPIDA, kRegSDIOut6VPIDA, kRegSDIOut7VPIDA, kRegSDIOut8VPIDA, 0};

static const ULWord gChannelToSDIOutVPIDBRegNum []		= { kRegSDIOut1VPIDB, kRegSDIOut2VPIDB, kRegSDIOut3VPIDB, kRegSDIOut4VPIDB,
															kRegSDIOut5VPIDB, kRegSDIOut6VPIDB, kRegSDIOut7VPIDB, kRegSDIOut8VPIDB, 0};

static const ULWord gChannelToSDIInVPIDARegNum []		= { kRegSDIIn1VPIDA,			kRegSDIIn2VPIDA,			kRegSDIIn3VPIDA,			kRegSDIIn4VPIDA,
															kRegSDIIn5VPIDA,			kRegSDIIn6VPIDA,			kRegSDIIn7VPIDA,			kRegSDIIn8VPIDA,			0};

static const ULWord gChannelToSDIInVPIDBRegNum []		= { kRegSDIIn1VPIDB,			kRegSDIIn2VPIDB,			kRegSDIIn3VPIDB,			kRegSDIIn4VPIDB,
															kRegSDIIn5VPIDB,			kRegSDIIn6VPIDB,			kRegSDIIn7VPIDB,			kRegSDIIn8VPIDB,			0};

static const ULWord gChannelToSDIInVPIDLinkAValidMask[] = { kRegMaskSDIInVPIDLinkAValid,	kRegMaskSDIIn2VPIDLinkAValid,	kRegMaskSDIIn3VPIDLinkAValid,	kRegMaskSDIIn4VPIDLinkAValid,
															kRegMaskSDIIn5VPIDLinkAValid,	kRegMaskSDIIn6VPIDLinkAValid,	kRegMaskSDIIn7VPIDLinkAValid,	kRegMaskSDIIn8VPIDLinkAValid,	0};

static const ULWord gChannelToSDIInVPIDLinkBValidMask[] = { kRegMaskSDIInVPIDLinkBValid,	kRegMaskSDIIn2VPIDLinkBValid,	kRegMaskSDIIn3VPIDLinkBValid,	kRegMaskSDIIn4VPIDLinkBValid,
															kRegMaskSDIIn5VPIDLinkBValid,	kRegMaskSDIIn6VPIDLinkBValid,	kRegMaskSDIIn7VPIDLinkBValid,	kRegMaskSDIIn8VPIDLinkBValid,	0};

static const ULWord gChannelToSDIInput3GStatusRegNum [] = { kRegSDIInput3GStatus,		kRegSDIInput3GStatus,		kRegSDIInput3GStatus2,		kRegSDIInput3GStatus2,
															kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	kRegSDI5678Input3GStatus,	0};

static const ULWord gChannelToSDIOutControlRegNum []	= { kRegSDIOut1Control, kRegSDIOut2Control, kRegSDIOut3Control, kRegSDIOut4Control,
															kRegSDIOut5Control, kRegSDIOut6Control, kRegSDIOut7Control, kRegSDIOut8Control, 0};

static const ULWord gChannelToVPIDTransferCharacteristics []	= { kVRegNTV2VPIDTransferCharacteristics1,		kVRegNTV2VPIDTransferCharacteristics2,		kVRegNTV2VPIDTransferCharacteristics3,		kVRegNTV2VPIDTransferCharacteristics4,
																	kVRegNTV2VPIDTransferCharacteristics5,		kVRegNTV2VPIDTransferCharacteristics6,		kVRegNTV2VPIDTransferCharacteristics7,		kVRegNTV2VPIDTransferCharacteristics8,	0};

static const ULWord gChannelToVPIDColorimetry []	= { kVRegNTV2VPIDColorimetry1,		kVRegNTV2VPIDColorimetry2,		kVRegNTV2VPIDColorimetry3,		kVRegNTV2VPIDColorimetry4,
														kVRegNTV2VPIDColorimetry5,		kVRegNTV2VPIDColorimetry6,		kVRegNTV2VPIDColorimetry7,		kVRegNTV2VPIDColorimetry8,	0};

static const ULWord gChannelToVPIDLuminance []	= { kVRegNTV2VPIDLuminance1,		kVRegNTV2VPIDLuminance2,		kVRegNTV2VPIDLuminance3,		kVRegNTV2VPIDLuminance4,
													kVRegNTV2VPIDLuminance5,		kVRegNTV2VPIDLuminance6,		kVRegNTV2VPIDLuminance7,		kVRegNTV2VPIDLuminance8,	0};

static const ULWord gChannelToVPIDRGBRange []	= { kVRegNTV2VPIDRGBRange1,		kVRegNTV2VPIDRGBRange2,		kVRegNTV2VPIDRGBRange3,		kVRegNTV2VPIDRGBRange4,
													kVRegNTV2VPIDRGBRange5,		kVRegNTV2VPIDRGBRange6,		kVRegNTV2VPIDRGBRange7,		kVRegNTV2VPIDRGBRange8, 0};

static const ULWord	gChannelToSDIOutVPIDTransferCharacteristics[] = {	kVRegSDIOutVPIDTransferCharacteristics1, kVRegSDIOutVPIDTransferCharacteristics2, kVRegSDIOutVPIDTransferCharacteristics3, kVRegSDIOutVPIDTransferCharacteristics4,
																		kVRegSDIOutVPIDTransferCharacteristics5, kVRegSDIOutVPIDTransferCharacteristics6, kVRegSDIOutVPIDTransferCharacteristics7, kVRegSDIOutVPIDTransferCharacteristics8, 0 };

static const ULWord	gChannelToSDIOutVPIDColorimetry[] =	{	kVRegSDIOutVPIDColorimetry1, kVRegSDIOutVPIDColorimetry2, kVRegSDIOutVPIDColorimetry3, kVRegSDIOutVPIDColorimetry4,
															kVRegSDIOutVPIDColorimetry5, kVRegSDIOutVPIDColorimetry6, kVRegSDIOutVPIDColorimetry7, kVRegSDIOutVPIDColorimetry8, 0 };

static const ULWord	gChannelToSDIOutVPIDLuminance[] = {	kVRegSDIOutVPIDLuminance1, kVRegSDIOutVPIDLuminance2, kVRegSDIOutVPIDLuminance3, kVRegSDIOutVPIDLuminance4,
														kVRegSDIOutVPIDLuminance5, kVRegSDIOutVPIDLuminance6, kVRegSDIOutVPIDLuminance7, kVRegSDIOutVPIDLuminance8, 0 };

static const ULWord	gChannelToSDIOutVPIDRGBRange[] = {	kVRegSDIOutVPIDRGBRange1, kVRegSDIOutVPIDRGBRange2, kVRegSDIOutVPIDRGBRange3, kVRegSDIOutVPIDRGBRange4,
														kVRegSDIOutVPIDRGBRange5, kVRegSDIOutVPIDRGBRange6, kVRegSDIOutVPIDRGBRange7, kVRegSDIOutVPIDRGBRange8, 0 };


bool CNTV2Card::GetVPIDValidA (const NTV2Channel inChannel)
{
	ULWord value(0);
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return ReadRegister(gChannelToSDIInput3GStatusRegNum[inChannel], value, gChannelToSDIInVPIDLinkAValidMask[inChannel])
			&&	value;
}

bool CNTV2Card::GetVPIDValidB (const NTV2Channel inChannel)
{
	ULWord value(0);
	if (IS_CHANNEL_INVALID(inChannel))
		return false;
	return ReadRegister(gChannelToSDIInput3GStatusRegNum[inChannel], value, gChannelToSDIInVPIDLinkBValidMask[inChannel])
			&&	value;
}

bool CNTV2Card::ReadSDIInVPID (const NTV2Channel inChannel, ULWord & outValue_A, ULWord & outValue_B)
{
	ULWord	status	(0);
	ULWord	valA	(0);
	ULWord	valB	(0);

	if (IS_CHANNEL_INVALID (inChannel))
		return false;
	if (!ReadRegister (gChannelToSDIInput3GStatusRegNum [inChannel], status))
		return false;
	if (status & gChannelToSDIInVPIDLinkAValidMask [inChannel])
	{
		if (!ReadRegister (gChannelToSDIInVPIDARegNum [inChannel], valA))
			return false;
	}
	else
	{
		outValue_A = 0;
		outValue_B = 0;
		return false;
	}

	if (!ReadRegister (gChannelToSDIInput3GStatusRegNum [inChannel], status))
		return false;
	if (status & gChannelToSDIInVPIDLinkBValidMask [inChannel])
	{
		if (!ReadRegister (gChannelToSDIInVPIDBRegNum [inChannel], valB))
			return false;
	}

	// reverse byte order
	if (GetDeviceID() == DEVICE_ID_KONALHI)
	{
		outValue_A = valA;
		outValue_B = valB;
	}
	else
	{
		outValue_A = NTV2EndianSwap32(valA);
		outValue_B = NTV2EndianSwap32(valB);
	}
	
	return true;

}	//	ReadSDIInVPID


bool CNTV2Card::WriteSDIInVPID (const NTV2Channel inChannel, const ULWord inValA, const ULWord inValB)
{
	ULWord	valA(inValA), valB(inValB);

	if (IS_CHANNEL_INVALID(inChannel))
		return false;

	// reverse byte order
	if (GetDeviceID() != DEVICE_ID_KONALHI)
	{
		valA = NTV2EndianSwap32(valA);
		valB = NTV2EndianSwap32(valB);
	}

	return WriteRegister (gChannelToSDIInVPIDARegNum[inChannel], valA)
			&&	WriteRegister (gChannelToSDIInVPIDBRegNum[inChannel], valB);

}	//	WriteSDIInVPID


bool CNTV2Card::SetSDIOutVPID (const ULWord inValueA, const ULWord inValueB, const UWord inOutputSpigot)
{
	if (IS_OUTPUT_SPIGOT_INVALID (inOutputSpigot))
		return false;
	if (inValueA)
	{
		if (!WriteRegister (gChannelToSDIOutVPIDARegNum [inOutputSpigot], inValueA)) return false;
		if (!WriteRegister (gChannelToSDIOutVPIDBRegNum [inOutputSpigot], inValueB)) return false;
		if (!WriteRegister (gChannelToSDIOutControlRegNum [inOutputSpigot], 1, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite))
			return false;
		if (!WriteRegister (gChannelToSDIOutControlRegNum [inOutputSpigot], 1, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable))
			return false;
		return true;
	}

	if (!WriteRegister (gChannelToSDIOutControlRegNum [inOutputSpigot], 0, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite))
		return false;
	if (!WriteRegister (gChannelToSDIOutControlRegNum [inOutputSpigot], 0, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable))
		return false;
	if (!WriteRegister (gChannelToSDIOutVPIDARegNum [inOutputSpigot], 0))
		return false;
	if (!WriteRegister (gChannelToSDIOutVPIDBRegNum [inOutputSpigot], 0))
		return false;
	return true;

}	//	SetSDIOutVPID


bool CNTV2Card::GetSDIOutVPID (ULWord & outValueA, ULWord & outValueB, const UWord inOutputSpigot)
{
	if (IS_OUTPUT_SPIGOT_INVALID(inOutputSpigot))
		return false;

	return ReadRegister(gChannelToSDIOutVPIDARegNum[inOutputSpigot], outValueA)
			&&	ReadRegister(gChannelToSDIOutVPIDBRegNum[inOutputSpigot], outValueB);

}	//	GetSDIOutVPID

// channel default VPID parameters
bool CNTV2Card::SetVPIDTransferCharacteristics (const NTV2VPIDTransferCharacteristics inValue, const NTV2Channel inChannel)
{
	return IS_CHANNEL_VALID(inChannel) && WriteRegister(gChannelToVPIDTransferCharacteristics[inChannel], inValue);
}

bool CNTV2Card::GetVPIDTransferCharacteristics (NTV2VPIDTransferCharacteristics & outValue, const NTV2Channel inChannel)
{
	return IS_CHANNEL_VALID(inChannel) && CNTV2DriverInterface::ReadRegister(gChannelToVPIDTransferCharacteristics[inChannel], outValue);
}

bool CNTV2Card::SetVPIDColorimetry (const NTV2VPIDColorimetry inValue, const NTV2Channel inChannel)
{
	return IS_CHANNEL_VALID(inChannel) && WriteRegister(gChannelToVPIDColorimetry[inChannel], inValue);
}

bool CNTV2Card::GetVPIDColorimetry (NTV2VPIDColorimetry & outValue, const NTV2Channel inChannel)
{
	return IS_CHANNEL_VALID(inChannel) && CNTV2DriverInterface::ReadRegister(gChannelToVPIDColorimetry[inChannel], outValue);
}

bool CNTV2Card::SetVPIDLuminance (const NTV2VPIDLuminance inValue, const NTV2Channel inChannel)
{
	return IS_CHANNEL_VALID(inChannel) && WriteRegister(gChannelToVPIDLuminance[inChannel], inValue);
}

bool CNTV2Card::GetVPIDLuminance (NTV2VPIDLuminance & outValue, const NTV2Channel inChannel)
{
	return IS_CHANNEL_VALID(inChannel) && CNTV2DriverInterface::ReadRegister(gChannelToVPIDLuminance[inChannel], outValue);
}

bool CNTV2Card::SetVPIDRGBRange (const NTV2VPIDRGBRange inValue, const NTV2Channel inChannel)
{
	return IS_CHANNEL_VALID(inChannel) && WriteRegister(gChannelToVPIDRGBRange[inChannel], inValue);
}

bool CNTV2Card::GetVPIDRGBRange (NTV2VPIDRGBRange & outValue, const NTV2Channel inChannel)
{
	return IS_CHANNEL_VALID(inChannel) && CNTV2DriverInterface::ReadRegister(gChannelToVPIDRGBRange[inChannel], outValue);
}

// sdi output VPID parameter overrides
bool CNTV2Card::SetSDIOutVPIDTransferCharacteristics(bool override, NTV2VPIDTransferCharacteristics inValue, const NTV2Channel inChannel)
{
    ULWord regValue = (override? kVRegMaskSDIOutVPIDOverride : 0) | (inValue & kVRegMaskSDIOutVPIDValue);
	return IS_CHANNEL_VALID(inChannel) && WriteRegister(gChannelToSDIOutVPIDTransferCharacteristics[inChannel], regValue);    
}

bool CNTV2Card::GetSDIOutVPIDTransferCharacteristics(bool & override, NTV2VPIDTransferCharacteristics & outValue, const NTV2Channel inChannel)
{
    if (!IS_CHANNEL_VALID(inChannel))
        return false;
    ULWord regValue = 0;
    if (!CNTV2DriverInterface::ReadRegister(gChannelToSDIOutVPIDTransferCharacteristics[inChannel], regValue))
        return false;
    override = (regValue & kVRegMaskSDIOutVPIDOverride) != 0;
    outValue = (NTV2VPIDTransferCharacteristics)(regValue & kVRegMaskSDIOutVPIDValue);
}

bool CNTV2Card::SetSDIOutVPIDColorimetry(bool override, NTV2VPIDColorimetry inValue, const NTV2Channel inChannel)
{
    ULWord regValue = (override? kVRegMaskSDIOutVPIDOverride : 0) | (inValue & kVRegMaskSDIOutVPIDValue);
	return IS_CHANNEL_VALID(inChannel) && WriteRegister(gChannelToSDIOutVPIDColorimetry[inChannel], regValue);    
}

bool CNTV2Card::GetSDIOutVPIDColorimetry(bool & override, NTV2VPIDColorimetry & outValue, const NTV2Channel inChannel)
{
    if (!IS_CHANNEL_VALID(inChannel))
        return false;
    ULWord regValue = 0;
    if (!CNTV2DriverInterface::ReadRegister(gChannelToSDIOutVPIDColorimetry[inChannel], regValue))
        return false;
    override = (regValue & kVRegMaskSDIOutVPIDOverride) != 0;
    outValue = (NTV2VPIDColorimetry)(regValue & kVRegMaskSDIOutVPIDValue);
}

bool CNTV2Card::SetSDIOutVPIDLuminance(bool override, NTV2VPIDLuminance inValue, const NTV2Channel inChannel)
{
    ULWord regValue = (override? kVRegMaskSDIOutVPIDOverride : 0) | (inValue & kVRegMaskSDIOutVPIDValue);
	return IS_CHANNEL_VALID(inChannel) && WriteRegister(gChannelToSDIOutVPIDLuminance[inChannel], regValue);    
}

bool CNTV2Card::GetSDIOutVPIDLuminance(bool & override, NTV2VPIDLuminance & outValue, const NTV2Channel inChannel)
{
    if (!IS_CHANNEL_VALID(inChannel))
        return false;
    ULWord regValue = 0;
    if (!CNTV2DriverInterface::ReadRegister(gChannelToSDIOutVPIDLuminance[inChannel], regValue))
        return false;
    override = (regValue & kVRegMaskSDIOutVPIDOverride) != 0;
    outValue = (NTV2VPIDLuminance)(regValue & kVRegMaskSDIOutVPIDValue);
}

bool CNTV2Card::SetSDIOutVPIDRGBRange(bool override, NTV2VPIDRGBRange inValue, const NTV2Channel inChannel)
{
    ULWord regValue = (override? kVRegMaskSDIOutVPIDOverride : 0) | (inValue & kVRegMaskSDIOutVPIDValue);
	return IS_CHANNEL_VALID(inChannel) && WriteRegister(gChannelToSDIOutVPIDRGBRange[inChannel], regValue);    
}

bool CNTV2Card::GetSDIOutVPIDRGBRange(bool & override, NTV2VPIDRGBRange & outValue, const NTV2Channel inChannel)
{
    if (!IS_CHANNEL_VALID(inChannel))
        return false;
    ULWord regValue = 0;
    if (!CNTV2DriverInterface::ReadRegister(gChannelToSDIOutVPIDRGBRange[inChannel], regValue))
        return false;
    override = (regValue & kVRegMaskSDIOutVPIDOverride) != 0;
    outValue = (NTV2VPIDRGBRange)(regValue & kVRegMaskSDIOutVPIDValue);
}


