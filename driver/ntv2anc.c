/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2anc.c
//
//==========================================================================

#include "ntv2anc.h"

//////////////////////////////////////////////////////////////
////For lack of a better place....

//#define ANC_EXT_DEBUG
//#define ANC_READ_BACK
//#define ANC_INS_DEBUG

#define ANC_EXT_1_OFFSET 0x1000
#define ANC_EXT_2_OFFSET 0x1040
#define ANC_EXT_3_OFFSET 0x1080
#define ANC_EXT_4_OFFSET 0x10c0
#define ANC_EXT_5_OFFSET 0x1100
#define ANC_EXT_6_OFFSET 0x1140
#define ANC_EXT_7_OFFSET 0x1180
#define ANC_EXT_8_OFFSET 0x11c0

#define ANC_INS_1_OFFSET 0x1200
#define ANC_INS_2_OFFSET 0x1240
#define ANC_INS_3_OFFSET 0x1280
#define ANC_INS_4_OFFSET 0x12c0
#define ANC_INS_5_OFFSET 0x1300
#define ANC_INS_6_OFFSET 0x1340
#define ANC_INS_7_OFFSET 0x1380
#define ANC_INS_8_OFFSET 0x13c0

static const ULWord	gChannelToAncExtOffset[] = { ANC_EXT_1_OFFSET, ANC_EXT_2_OFFSET, ANC_EXT_3_OFFSET, ANC_EXT_4_OFFSET,
	ANC_EXT_5_OFFSET, ANC_EXT_6_OFFSET, ANC_EXT_7_OFFSET, ANC_EXT_8_OFFSET, 0 };

static const ULWord	gChannelToAncInsOffset[] = { ANC_INS_1_OFFSET, ANC_INS_2_OFFSET, ANC_INS_3_OFFSET, ANC_INS_4_OFFSET,
	ANC_INS_5_OFFSET, ANC_INS_6_OFFSET, ANC_INS_7_OFFSET, ANC_INS_8_OFFSET, 0 };


bool SetupAncExtractor(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2Standard theStandard = GetStandard(context, channel);

	// disable anc enhanced mode
	ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl, 0, maskInsExtendedMode, shiftInsExtendedMode);

	switch (theStandard)
	{
	case NTV2_STANDARD_1080p:
#ifdef ANC_EXT_DEBUG
		ntv2Message("SetupAncExtractor - 1080p\n");
#endif
		SetAncExtProgressive(context, channel, true);
		SetAncExtField1StartLine(context, channel, 1122);
		SetAncExtField1CutoffLine(context, channel, 1125);
		SetAncExtField2StartLine(context, channel, 0);
		SetAncExtField2CutoffLine(context, channel, 0);
		SetAncExtTotalFrameLines(context, channel, 1125);
		SetAncExtFidLow(context, channel, 0);
		SetAncExtFidHi(context, channel, 0);
		SetAncExtField1AnalogStartLine(context, channel, 0);
		SetAncExtField2AnalogStartLine(context, channel, 0);
		SetAncExtField1AnalogYFilter(context, channel, 0x00);
		SetAncExtField2AnalogYFilter(context, channel, 0x00);
		SetAncExtField1AnalogCFilter(context, channel, 0x00);
		SetAncExtField2AnalogCFilter(context, channel, 0x00);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+12, 0xE4E5E6E7);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+13, 0xE0E1E2E3);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+14, 0xA4A5A6A7);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+15, 0xA0A1A2A3);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+16, 0x0);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+27, 0x07800000);	//	Restore default (analog) Active Line Length = 1920 bytes
		break;
	case NTV2_STANDARD_1080:
#ifdef ANC_EXT_DEBUG
		ntv2Message("SetupAncExtractor - 1080i\n");
#endif
		SetAncExtProgressive(context, channel, false);
		SetAncExtField1StartLine(context, channel, 561);
		SetAncExtField1CutoffLine(context, channel, 563);
		SetAncExtField2StartLine(context, channel, 1124);
		SetAncExtField2CutoffLine(context, channel, 1123);
		SetAncExtTotalFrameLines(context, channel, 1125);
		SetAncExtFidLow(context, channel, 1125);
		SetAncExtFidHi(context, channel, 563);
		SetAncExtField1AnalogStartLine(context, channel, 0);
		SetAncExtField2AnalogStartLine(context, channel, 0);
		SetAncExtField1AnalogYFilter(context, channel, 0x00);
		SetAncExtField2AnalogYFilter(context, channel, 0x00);
		SetAncExtField1AnalogCFilter(context, channel, 0x00);
		SetAncExtField2AnalogCFilter(context, channel, 0x00);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+12, 0xE4E5E6E7);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+13, 0xE0E1E2E3);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+14, 0xA4A5A6A7);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+15, 0xA0A1A2A3);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+16, 0x0);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+27, 0x07800000);	//	Restore default (analog) Active Line Length = 1920 bytes
		break;
	case NTV2_STANDARD_720:
#ifdef ANC_EXT_DEBUG
		ntv2Message("SetupAncExtractor - 720p\n");
#endif
		SetAncExtProgressive(context, channel, true);
		SetAncExtField1StartLine(context, channel, 746);
		SetAncExtField1CutoffLine(context, channel, 745);
		SetAncExtField2StartLine(context, channel, 0);
		SetAncExtField2CutoffLine(context, channel, 0);
		SetAncExtTotalFrameLines(context, channel, 750);
		SetAncExtFidLow(context, channel, 0);
		SetAncExtFidHi(context, channel, 0);
		SetAncExtField1AnalogStartLine(context, channel, 0);
		SetAncExtField2AnalogStartLine(context, channel, 0);
		SetAncExtField1AnalogYFilter(context, channel, 0x00);
		SetAncExtField2AnalogYFilter(context, channel, 0x00);
		SetAncExtField1AnalogCFilter(context, channel, 0x00);
		SetAncExtField2AnalogCFilter(context, channel, 0x00);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+12, 0xE4E5E6E7);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+13, 0xE0E1E2E3);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+14, 0xA4A5A6A7);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+15, 0xA0A1A2A3);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+16, 0x0);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+27, 0x05000000);	//	Set (analog) Active Line Length = 1280 bytes
		break;
	case NTV2_STANDARD_625:
#ifdef ANC_EXT_DEBUG
		ntv2Message("SetupAncExtractor - 625\n");
#endif
		SetAncExtProgressive(context, channel, false);
		SetAncExtField1StartLine(context, channel, 311);
		SetAncExtField1CutoffLine(context, channel, 33);
		SetAncExtField2StartLine(context, channel, 1);
		SetAncExtField2CutoffLine(context, channel, 346);
		SetAncExtTotalFrameLines(context, channel, 625);
		SetAncExtFidLow(context, channel, 625);
		SetAncExtFidHi(context, channel, 312);
		SetAncExtField1AnalogStartLine(context, channel, 5);
		SetAncExtField2AnalogStartLine(context, channel, 318);
		SetAncExtField1AnalogYFilter(context, channel, 0x10000);	//	Grab F1 analog Y samples from only Line 21:  bit 16 + Field1AnalogStartLine 5
		SetAncExtField2AnalogYFilter(context, channel, 0x10000);	//	Grab F2 analog Y samples from only Line 334:  bit 16 + Field2AnalogStartLine 318
		SetAncExtField1AnalogCFilter(context, channel, 0x0);
		SetAncExtField2AnalogCFilter(context, channel, 0x0);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+12, 0xF9FBFDFF);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+13, 0xF8FAFCFE);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+14, 0xECEDEEEF);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+15, 0x0);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+16, 0x0);
		//	To prevent grabbing too much analog data, limit the Active Line Length (undocumented Extractor Register 27) to 720 bytes:
		//	The firmware is screwy in that you have to write the value into bits [27:16], but readback happens on bits [11:0]
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+27, 0x02D00000);	//	Limit captured analog data to 720 bytes
		break;
	case NTV2_STANDARD_525:
#ifdef ANC_EXT_DEBUG
		ntv2Message("SetupAncExtractor - 525\n");
#endif
		SetAncExtProgressive(context, channel, false);
		SetAncExtField1StartLine(context, channel, 264);
		SetAncExtField1CutoffLine(context, channel, 30);
		SetAncExtField2StartLine(context, channel, 1);
		SetAncExtField2CutoffLine(context, channel, 293);
		SetAncExtTotalFrameLines(context, channel, 525);
		SetAncExtFidLow(context, channel, 3);
		SetAncExtFidHi(context, channel, 265);
		SetAncExtField1AnalogStartLine(context, channel, 4);
		SetAncExtField2AnalogStartLine(context, channel, 266);
		SetAncExtField1AnalogYFilter(context, channel, 0x20000);	//	Grab F1 analog Y samples from only Line 21:  bit 17 + Field1AnalogStartLine 4
		SetAncExtField2AnalogYFilter(context, channel, 0x40000);	//	Grab F2 analog Y samples from only Line 284:  bit 18 + Field2AnalogStartLine 266
		SetAncExtField1AnalogCFilter(context, channel, 0x0);
		SetAncExtField2AnalogCFilter(context, channel, 0x0);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+12, 0xF9FBFDFF);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+13, 0xF8FAFCFE);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+14, 0xECEDEEEF);	//	Ignore audio
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+15, 0x0);
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+16, 0x0);
		//	To prevent grabbing too much analog data, limit the Active Line Length (undocumented Extractor Register 27) to 720 bytes:
		//	The firmware is screwy in that you have to write the value into bits [27:16], but readback happens on bits [11:0]
		ntv2WriteRegister(context, gChannelToAncExtOffset[channel]+27, 0x02D00000);	//	Limit captured analog data to 720 bytes
		break;
	default:
#ifdef ANC_EXT_DEBUG
		ntv2Message("SetupAncExtractor - default\n");
#endif
		return false;
	}

	SetAncExtSDDemux(context, channel, NTV2_IS_SD_STANDARD(theStandard));
	EnableAncExtHancC(context, channel, true);
	EnableAncExtHancY(context, channel, true);
	EnableAncExtVancC(context, channel, true);
	EnableAncExtVancY(context, channel, true);

	SetAncExtSynchro(context, channel);

	SetAncExtField1StartAddr(context, channel, 0);
	SetAncExtField1EndAddr(context, channel, 0);
	SetAncExtField2StartAddr(context, channel, 0);
	SetAncExtField2EndAddr(context, channel, 0);
#if 0
	KdPrint(("ANC status:\n"));
	KdPrint(("reg 0: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 0)));
	KdPrint(("reg 1: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 1)));
	KdPrint(("reg 2: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 2)));
	KdPrint(("reg 3: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 3)));
	KdPrint(("reg 4: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 4)));
	KdPrint(("reg 5: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 5)));
	KdPrint(("reg 6: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 6)));
	KdPrint(("reg 7: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 7)));
	KdPrint(("reg 8: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 8)));
	KdPrint(("reg 9: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 9)));
	KdPrint(("reg 10: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 10)));
	KdPrint(("reg 11: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 11)));
	KdPrint(("reg 12: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 12)));
	KdPrint(("reg 13: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 13)));
	KdPrint(("reg 14: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 14)));
	KdPrint(("reg 15: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 15)));
	KdPrint(("reg 16: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 16)));
	KdPrint(("reg 17: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 17)));
	KdPrint(("reg 18: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 18)));
	KdPrint(("reg 19: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 19)));
	KdPrint(("reg 20: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 20)));
	KdPrint(("reg 21: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 21)));
	KdPrint(("reg 22: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 22)));
	KdPrint(("reg 23: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 23)));
	KdPrint(("reg 24: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 24)));
	KdPrint(("reg 25: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 25)));
	KdPrint(("reg 26: %08x\n", ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + 26)));
#endif
	return true;
}

bool EnableAncExtractor(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtControl;
	KdPrint(("EnableAncExtractor - channel: %d, %d\n", channel + 1, regNum));
#endif
	if (!bEnable)
	{
		EnableAncExtHancC(context, channel, false);
		EnableAncExtHancY(context, channel, false);
		EnableAncExtVancC(context, channel, false);
		EnableAncExtVancY(context, channel, false);
	}
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 0 : 1, maskDisableExtractor, shiftDisableExtractor);
}

bool SetAncExtWriteParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber)
{
	//Calculate where ANC Extractor will put the data
	ULWord nextFrame = frameNumber+1;//This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord endOfFrameLocation = GetFrameBufferSize(context, (channel < NTV2_CHANNEL5) ? NTV2_CHANNEL1 : NTV2_CHANNEL5)* nextFrame;
	ULWord ANCStartMemory = endOfFrameLocation - ntv2ReadVirtualRegister(context, kVRegAncField1Offset);
	ULWord ANCStopMemory = endOfFrameLocation - ntv2ReadVirtualRegister(context, kVRegAncField2Offset);
	ANCStopMemory -= 1;
	SetAncExtField1StartAddr(context, channel, ANCStartMemory);
	SetAncExtField1EndAddr(context, channel, ANCStopMemory);
	return true;
}

bool SetAncExtField2WriteParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber)
{
	//Calculate where ANC Extractor will put the data
	ULWord nextFrame = frameNumber+1;//This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord endOfFrameLocation = GetFrameBufferSize(context, (channel < NTV2_CHANNEL5) ? NTV2_CHANNEL1 : NTV2_CHANNEL5)* nextFrame;
	ULWord ANCStartMemory = endOfFrameLocation - ntv2ReadVirtualRegister(context, kVRegAncField2Offset);
	ULWord ANCStopMemory = endOfFrameLocation - 1;
	SetAncExtField2StartAddr(context, channel, ANCStartMemory);
	SetAncExtField2EndAddr(context, channel, ANCStopMemory);
	return true;
}

bool EnableAncExtHancY(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtControl;
	ntv2Message("EnableAncExtHancY - channel: %d, %d\n", channel + 1, regNum);
#endif
	bool status =  ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableHancY, shiftEnableHancY);
#ifdef ANC_READ_BACK
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, &value, maskEnableHancY, shiftEnableHancY);
	ntv2Message("EnableAncExtHancY - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
	
}

bool EnableAncExtHancC(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtControl;
	ntv2Message("EnableAncExtHancC - channel: %d, %d\n", channel + 1, regNum);
#endif
	bool status = ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableHancC, shiftEnableHancC);
#ifdef ANC_READ_BACK
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, &value, maskEnableHancC, shiftEnableHancC);
	ntv2Message("EnableAncExtHancC - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool EnableAncExtVancY(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtControl;
	ntv2Message("EnableAncExtVancY - channel: %d, %d\n", channel + 1, regNum);
#endif
	bool status = ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableVancY, shiftEnableVancY);
#ifdef ANC_READ_BACK
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, &value, maskEnableVancY, shiftEnableVancY);
	ntv2Message("EnableAncExtVancY - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool EnableAncExtVancC(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtControl;
	ntv2Message("EnableAncExtVancC - channel: %d, %d\n", channel + 1, regNum);
#endif
	bool status =  ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableVancC, shiftEnableVancC);
#ifdef ANC_READ_BACK
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, &value, maskEnableVancC, shiftEnableVancC);
	ntv2Message("EnableAncExtVancC - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool SetAncExtSDDemux(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableSDMux, shiftEnableSDMux);
}

bool SetAncExtProgressive(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskSetProgressive, shiftSetProgressive);
}

bool SetAncExtSynchro(Ntv2SystemContext* context, NTV2Channel channel)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl,0x1, maskSyncro, shiftSyncro);
}

bool SetAncExtLSBEnable(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, (ULWord)maskGrabLSBs, shiftGrabLSBs);
}

bool SetAncExtField1StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtField1StartAddress;
	ntv2Message("SetAncExtField1StartAddr - channel: %d, reg: %d, addr: %d\n", channel + 1, regNum, addr);
#endif
	bool status = ntv2WriteRegister(context, gChannelToAncExtOffset[channel] + regAncExtField1StartAddress, addr);
#ifdef ANC_READ_BACK
	ULWord value = ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + regAncExtField1StartAddress);
	ntv2Message("SetAncExtField1StartAddr - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool SetAncExtField1EndAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtField1EndAddress;
	ntv2Message("SetAncExtField1EndAddr - channel: %d, reg: %d, addr: %d\n", channel + 1, regNum, addr);
#endif
	bool status = ntv2WriteRegister(context, gChannelToAncExtOffset[channel] + regAncExtField1EndAddress, addr);
#ifdef ANC_READ_BACK
	ULWord value = ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + regAncExtField1EndAddress);
	ntv2Message("SetAncExtField1EndAddr - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool SetAncExtField2StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtField2StartAddress;
	ntv2Message("SetAncExtField2StartAddr - channel: %d, reg: %d, addr: %d\n", channel + 1, regNum, addr);
#endif
	bool status = ntv2WriteRegister(context, gChannelToAncExtOffset[channel] + regAncExtField2StartAddress, addr);
#ifdef ANC_READ_BACK
	ULWord value = ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + regAncExtField2StartAddress);
	ntv2Message("SetAncExtField2StartAddr - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool SetAncExtField2EndAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr)
{
#ifdef ANC_EXT_DEBUG
	ULWord regNum = gChannelToAncExtOffset[channel] + regAncExtField2EndAddress;
	ntv2Message("SetAncExtField2EndAddr - channel: %d, reg: %d, addr: %d\n", channel + 1, regNum, addr);
#endif
	bool status = ntv2WriteRegister(context, gChannelToAncExtOffset[channel] + regAncExtField2EndAddress, addr);
#ifdef ANC_READ_BACK
	ULWord value = ntv2ReadRegister(context, gChannelToAncExtOffset[channel] + regAncExtField2EndAddress);
	ntv2Message("SetAncExtField2EndAddr - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool SetAncExtField1CutoffLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtFieldCutoffLine, lineNumber, maskField1CutoffLine, shiftField1CutoffLine);
}

bool SetAncExtField2CutoffLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtFieldCutoffLine, lineNumber, maskField2CutoffLine, shiftField2CutoffLine);
}

bool IsAncExtOverrun(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtTotalStatus, &value, maskTotalOverrun, shiftTotalOverrun);
	return value == 1 ? true : false;
}

ULWord GetAncExtField1Bytes(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtField1Status, &value, maskField1BytesIn, shiftField1BytesIn);
#ifdef ANC_READ_BACK
	ntv2Message("CNTV2Device::GetAncExtField1Bytes - channel: %d, %d\n", channel + 1, value);
#endif
	return value;
}

bool IsAncExtField1Overrun(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtField1Status, &value, maskField1Overrun, shiftField1Overrun);
	return value == 1 ? true : false;
}

ULWord GetAncExtField2Bytes(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtField2Status, &value, maskField2BytesIn, shiftField2BytesIn);
#ifdef ANC_READ_BACK
	KdPrint(("CNTV2Device::GetAncExtField2Bytes - channel: %d, %d\n", channel + 1, value));
#endif
	return value;
}

bool IsAncExtField2Overrun(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtField2Status, &value, maskField2Overrun, shiftField2Overrun);
	return value == 1 ? true : false;
}

bool SetAncExtField1StartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtFieldVBLStartLine, lineNumber, maskField1StartLine, shiftField1StartLine);
}

bool SetAncExtField2StartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtFieldVBLStartLine, lineNumber, maskField2StartLine, shiftField2StartLine);
}

bool SetAncExtTotalFrameLines(Ntv2SystemContext* context, NTV2Channel channel, ULWord totalFrameLines)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtTotalFrameLines, totalFrameLines, maskTotalFrameLines, shiftTotalFrameLines);
}

bool SetAncExtFidLow(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtFID, lineNumber, maskFIDLow, shiftFIDLow);
}

bool SetAncExtFidHi(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtFID, lineNumber, maskFIDHi, shiftFIDHi);
}

bool SetAncExtField1AnalogStartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtAnalogStartLine, lineNumber, maskField1AnalogStartLine, shiftField1AnalogStartLine);
}

bool SetAncExtField2AnalogStartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAncExtOffset[channel] + regAncExtAnalogStartLine, lineNumber, maskField2AnalogStartLine, shiftField2AnalogStartLine);
}

bool SetAncExtField1AnalogYFilter(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineFilter)
{
	return ntv2WriteRegister(context, gChannelToAncExtOffset[channel] + regAncExtField1AnalogYFilter, lineFilter);
}

bool SetAncExtField2AnalogYFilter(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineFilter)
{
	return ntv2WriteRegister(context, gChannelToAncExtOffset[channel] + regAncExtField2AnalogYFilter, lineFilter);
}

bool SetAncExtField1AnalogCFilter(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineFilter)
{
	return ntv2WriteRegister(context, gChannelToAncExtOffset[channel] + regAncExtField1AnalogCFilter, lineFilter);
}

bool SetAncExtField2AnalogCFilter(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineFilter)
{
	return ntv2WriteRegister(context, gChannelToAncExtOffset[channel] + regAncExtField2AnalogCFilter, lineFilter);
}

bool SetupAncInserter(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2Standard theStandard = GetStandard(context, channel);
	ULWord is2Kx1080 = NTV2_IS_2K_1080_FRAME_GEOMETRY(GetFrameGeometry(context, channel));
	
	// disable anc enhanced mode
	ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl, 0, maskInsExtendedMode, shiftInsExtendedMode);

	switch (theStandard)
	{
	case NTV2_STANDARD_1080p:
#ifdef ANC_INS_DEBUG
		ntv2Message("SetupAncInserter - 1080p\n");
#endif
		SetAncInsField1ActiveLine(context, channel, 42);
		SetAncInsHActivePixels(context, channel, (is2Kx1080 == 0) ? 1920 : 2048);
		SetAncInsHTotalPixels(context, channel, 2640);
		SetAncInsTotalLines(context, channel, 1125);
		SetAncInsFidLow(context, channel, 0);
		SetAncInsFidHi(context, channel, 0);
		break;
	case NTV2_STANDARD_1080:
#ifdef ANC_INS_DEBUG
		ntv2Message("CNTV2Device::SetupAncInserter - 1080i\n");
#endif
		SetAncInsField1ActiveLine(context, channel, 21);
		SetAncInsField2ActiveLine(context, channel, 564);
		SetAncInsHActivePixels(context, channel, 1920);
		SetAncInsHTotalPixels(context, channel, 2200);
		SetAncInsTotalLines(context, channel, 1125);
		SetAncInsFidLow(context, channel, 1125);
		SetAncInsFidHi(context, channel, 563);
		break;
	case NTV2_STANDARD_720:
	{
#ifdef ANC_INS_DEBUG
		ntv2Message("CNTV2Device::SetupAncInserter - 720p\n");
#endif
		SetAncInsField1ActiveLine(context, channel, 26);
		SetAncInsHActivePixels(context, channel, 1280);
		SetAncInsHTotalPixels(context, channel, 1280);
		SetAncInsTotalLines(context, channel, 750);
		SetAncInsFidLow(context, channel, 0);
		SetAncInsFidHi(context, channel, 0);
		break;
	}
	case NTV2_STANDARD_625:
#ifdef ANC_INS_DEBUG
		ntv2Message("CNTV2Device::SetupAncInserter - 625\n");
#endif
		SetAncInsField1ActiveLine(context, channel, 23);
		SetAncInsField2ActiveLine(context, channel, 336);
		SetAncInsHActivePixels(context, channel, 720);
		SetAncInsHTotalPixels(context, channel, 864);
		SetAncInsTotalLines(context, channel, 625);
		SetAncInsFidLow(context, channel, 625);
		SetAncInsFidHi(context, channel, 312);
		break;
	case NTV2_STANDARD_525:
#ifdef ANC_INS_DEBUG
		ntv2Message("CNTV2Device::SetupAncInserter - 525\n");
#endif
		SetAncInsField1ActiveLine(context, channel, 21);
		SetAncInsField2ActiveLine(context, channel, 283);
		SetAncInsHActivePixels(context, channel, 720);
		SetAncInsHTotalPixels(context, channel, 720);
		SetAncInsTotalLines(context, channel, 525);
		SetAncInsFidLow(context, channel, 3);
		SetAncInsFidHi(context, channel, 265);
		break;
	default:
#ifdef ANC_INS_DEBUG
		ntv2Message("CNTV2Device::SetupAncInserter - default\n");
#endif
		return false;
	}
	SetAncInsProgressive(context, channel, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	SetAncInsSDPacketSplit(context, channel, NTV2_IS_SD_STANDARD(theStandard));
	EnableAncInsHancC(context, channel, false);
	EnableAncInsHancY(context, channel, false);
	EnableAncInsVancC(context, channel, true);
	EnableAncInsVancY(context, channel, true);
	SetAncInsHancPixelDelay(context, channel, 0);
	SetAncInsVancPixelDelay(context, channel, 0);
	ntv2WriteRegister(context, gChannelToAncInsOffset[channel] + regAncInsBlankCStartLine, 0);
	ntv2WriteRegister(context, gChannelToAncInsOffset[channel] + regAncInsBlankField1CLines, 0);
	ntv2WriteRegister(context, gChannelToAncInsOffset[channel] + regAncInsBlankField2CLines, 0);
	return true;
}

bool EnableAncInserter(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsControl;
	ntv2Message("EnableAncInserter - channel: %d, reg: %d, enable: %s\n", channel + 1, regNum, bEnable ? "true" : "false");
#endif
	if (!bEnable)
	{
		EnableAncInsHancC(context, channel, false);
		EnableAncInsHancY(context, channel, false);
		EnableAncInsVancC(context, channel, false);
		EnableAncInsVancY(context, channel, false);
	}
	ntv2WriteRegister(context, gChannelToAncInsOffset[channel] + regAncInsBlankCStartLine, 0);
	ntv2WriteRegister(context, gChannelToAncInsOffset[channel] + regAncInsBlankField1CLines, 0);
	ntv2WriteRegister(context, gChannelToAncInsOffset[channel] + regAncInsBlankField2CLines, 0);
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 0 : 1, maskInsDisableInserter, shiftInsDisableInserter);
}

bool SetAncInsReadParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber, ULWord field1Size)
{
	//Calculate where ANC Extractor will put the data
	ULWord nextFrame = frameNumber+1; //Start at the beginning of next frame and subtract offset
	ULWord frameLocation = GetFrameBufferSize(context, channel < NTV2_CHANNEL5 ? NTV2_CHANNEL1 : NTV2_CHANNEL5)* (nextFrame);
	ULWord ANCStartMemory = frameLocation - ntv2ReadVirtualRegister(context, kVRegAncField1Offset);
	ULWord ancField1Size = field1Size;
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

#ifdef ANC_INS_DEBUG
	ntv2Message("SetAncInsReadField1Params - currentFrame: %d, nextFrame: %d, frameLocation: %08X, ANCStartMemory: %08X\n", frameNumber, nextFrame, frameLocation, ANCStartMemory);
#endif

	SetAncInsField1StartAddr(context, channel, ANCStartMemory);
	SetAncInsField1Bytes(context, channel, ancField1Size);

	if (ntv2ReadVirtualRegister(context, kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
	{
		//For retail mode we will setup all the anc inserters to read from the same location
		ULWord i;
		for (i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++)
		{
			if((deviceID == DEVICE_ID_IOIP_2110 || deviceID == DEVICE_ID_IOIP_2110_RGB12) && (NTV2Channel)i == NTV2_CHANNEL5)
			{
				ANCStartMemory = frameLocation - ntv2ReadVirtualRegister(context, kVRegMonAncField1Offset);
				ancField1Size = ntv2ReadVirtualRegister(context, kVRegMonAncField1Offset) - ntv2ReadVirtualRegister(context, kVRegAncField2Offset);
				
			}
			SetAncInsField1StartAddr(context, (NTV2Channel)i, ANCStartMemory);
			SetAncInsField1Bytes(context, (NTV2Channel)i, field1Size);
		}
	}

	return true;
}

bool SetAncInsReadField2Params(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber, ULWord field2Size)
{
	ULWord nextFrame = frameNumber+1; //Start at the beginning of next frame and subtract offset
	ULWord frameLocation = GetFrameBufferSize(context, channel < NTV2_CHANNEL5 ? NTV2_CHANNEL1 : NTV2_CHANNEL5)* (nextFrame);
	ULWord ANCStartMemory = frameLocation - ntv2ReadVirtualRegister(context, kVRegAncField2Offset);
	NTV2DeviceID deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

#ifdef ANC_INS_DEBUG
	ntv2Message("SetAncInsReadField2Params - currentFrame: %d, nextFrame: %d, frameLocation: %08X, ANCStartMemory: %08X\n", frameNumber, nextFrame, frameLocation, ANCStartMemory);
#endif

	SetAncInsField2StartAddr(context, channel, ANCStartMemory);
	SetAncInsField2Bytes(context, channel, field2Size);

	if (ntv2ReadVirtualRegister(context, kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
	{
		//For retail mode we will setup all the anc inserters to read from the same location
		ULWord i;
		for (i = 0; i < NTV2DeviceGetNumVideoOutputs(deviceID); i++)
		{
			if((deviceID == DEVICE_ID_IOIP_2110 || deviceID == DEVICE_ID_IOIP_2110_RGB12) && (NTV2Channel)i == NTV2_CHANNEL5)
			{
				ANCStartMemory = frameLocation - ntv2ReadVirtualRegister(context, kVRegMonAncField2Offset);
			}
			SetAncInsField2StartAddr(context, (NTV2Channel)i, ANCStartMemory);
			SetAncInsField2Bytes(context, (NTV2Channel)i, field2Size);
		}
	}
	return true;
}

bool SetAncInsField1Bytes(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfBytes)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsFieldBytes;
	ntv2Message("SetAncInsField1Bytes - channel: %d, reg: %d, NoB: %d\n", channel + 1, regNum, numberOfBytes);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsFieldBytes, numberOfBytes, maskInsField1Bytes, shiftInsField1Bytes);
}

bool SetAncInsField2Bytes(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfBytes)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsFieldBytes;
	ntv2Message("SetAncInsField2Bytes - channel: %d, reg: %d, NoB: %d\n", channel + 1, regNum, numberOfBytes);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsFieldBytes, numberOfBytes, (ULWord)maskInsField2Bytes, shiftInsField2Bytes);
}

bool EnableAncInsHancY(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsControl;
	ntv2Message("EnableAncInsHancY - channel: %d, reg: %d, enable %s\n", channel + 1, regNum, bEnable ? "true" : "false");
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 1 : 0, maskInsEnableHancY, shiftInsEnableHancY);
}

bool EnableAncInsHancC(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsControl;
	ntv2Message("CNTV2Device::EnableAncInsHancC - channel: %d, reg: %d, enable %s\n", channel + 1, regNum, bEnable ? "true" : "false");
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 1 : 0, maskInsEnableHancC, shiftInsEnableHancY);
}

bool EnableAncInsVancY(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsControl;
	ntv2Message("EnableAncInsVancY - channel: %d, reg: %d, enable %s\n", channel + 1, regNum, bEnable ? "true" : "false");
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 1 : 0, maskInsEnableVancY, shiftInsEnableVancY);
}

bool EnableAncInsVancC(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsControl;
	ntv2Message("EnableAncInsVancC - channel: %d, reg: %d, enable %s\n", channel + 1, regNum, bEnable ? "true" : "false");
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 1 : 0, maskInsEnableVancC, shiftInsEnableVancC);
}

bool SetAncInsProgressive(Ntv2SystemContext* context, NTV2Channel channel, bool isProgressive)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsControl;
	ntv2Message("SetAncInsProgressive - channel: %d, reg: %d, progressive: %s\n", channel + 1, regNum, isProgressive ? "true" : "false");
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl, isProgressive ? 1 : 0, maskInsSetProgressive, shiftInsSetProgressive);
}

bool SetAncInsSDPacketSplit(Ntv2SystemContext* context, NTV2Channel channel, bool inEnable)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsControl;
	ntv2Message("SetAncInsSDPacketSplit - channel: %d, reg: %d, inEnable: %s\n", channel+1, regNum, inEnable ? "true" : "false");
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsControl,  inEnable ? 1 : 0,  (ULWord)maskInsEnablePktSplitSD, shiftInsEnablePktSplitSD);
}

bool SetAncInsField1StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord startAddr)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsField1StartAddr;
	ntv2Message("SetAncInsField1StartAddr - channel: %d, reg: %d, startAddr: %08X\n", channel + 1, regNum, startAddr);
#endif
	return ntv2WriteRegister(context, gChannelToAncInsOffset[channel] + regAncInsField1StartAddr, startAddr);
}

bool SetAncInsField2StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord startAddr)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsField2StartAddr;
	ntv2Message("SetAncInsField2StartAddr - channel: %d, reg: %d, startAddr: %08X\n", channel + 1, regNum, startAddr);
#endif
	return ntv2WriteRegister(context, gChannelToAncInsOffset[channel] + regAncInsField2StartAddr, startAddr);
}

bool SetAncInsHancPixelDelay(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfPixels)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsPixelDelay;
	ntv2Message("SetAncInsHancPixelDelay - channel: %d, reg: %d, NoP: %d\n", channel + 1, regNum, numberOfPixels);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsPixelDelay, numberOfPixels, maskInsHancDelay, shiftINsHancDelay);
}

bool SetAncInsVancPixelDelay(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfPixels)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsPixelDelay;
	ntv2Message("SetAncInsVancPixelDelay - channel: %d, reg: %d, NoP: %d\n", channel + 1, regNum, numberOfPixels);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsPixelDelay, numberOfPixels, maskInsVancDelay, shiftInsVancDelay);
}

bool SetAncInsField1ActiveLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord activeLineNumber)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsActiveStart;
	ntv2Message("SetAncInsField1ActiveLine - channel: %d, reg: %d, ActiveLine: %d\n", channel + 1, regNum, activeLineNumber);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsActiveStart, activeLineNumber, maskInsField1FirstActive, shiftInsField1FirstActive);
}

bool SetAncInsField2ActiveLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord activeLineNumber)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsActiveStart;
	ntv2Message("SetAncInsField2ActiveLine - channel: %d, reg: %d, ActiveLine: %d\n", channel + 1, regNum, activeLineNumber);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsActiveStart, activeLineNumber, maskInsField2FirstActive, shiftInsField2FirstActive);
}

bool SetAncInsHActivePixels(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfActiveLinePixels)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsLinePixels;
	ntv2Message("SetAncInsHActivePixels - channel: %d, reg: %d, NoP: %d\n", channel + 1, regNum, numberOfActiveLinePixels);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsLinePixels, numberOfActiveLinePixels, maskInsActivePixelsInLine, shiftInsActivePixelsInLine);
}

bool SetAncInsHTotalPixels(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfLinePixels)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsLinePixels;
	ntv2Message("SetAncInsHTotalPixels - channel: %d, reg: %d, NoP: %d\n", channel + 1, regNum, numberOfLinePixels);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsLinePixels, numberOfLinePixels, maskInsTotalPixelsInLine, shiftInsTotalPixelsInLine);
}

bool SetAncInsTotalLines(Ntv2SystemContext* context, NTV2Channel channel, ULWord numberOfLines)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsFrameLines;
	ntv2Message("SetAncInsTotalLines - channel: %d, reg: %d, NoP: %d\n", channel + 1, regNum, numberOfLines);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsFrameLines, numberOfLines, maskInsTotalLinesPerFrame, shiftInsTotalLinesPerFrame);
}

bool SetAncInsFidHi(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsFieldIDLines;
	ntv2Message("SetAncInsFidHi - channel: %d, reg: %d, line#: %d\n", channel + 1, regNum, lineNumber);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsFieldIDLines, lineNumber, maskInsFieldIDHigh, shiftInsFieldIDHigh);
}

bool SetAncInsFidLow(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
#ifdef ANC_INS_DEBUG
	ULWord regNum = gChannelToAncInsOffset[channel] + regAncInsFieldIDLines;
	ntv2Message("SetAncInsFidLow - channel: %d, reg: %d, line#: %d\n", channel + 1, regNum, lineNumber);
#endif
	return ntv2WriteRegisterMS(context, gChannelToAncInsOffset[channel] + regAncInsFieldIDLines, lineNumber, maskInsFieldIDLow, shiftInsFieldIDLow);
}
