/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2aux.c
//
//==========================================================================

#include "ntv2aux.h"

//////////////////////////////////////////////////////////////
////For lack of a better place....

//#define AUX_EXT_DEBUG
//#define AUX_READ_BACK
//#define AUX_INS_DEBUG

#define AUX_EXT_1_OFFSET 0x1dc0
#define AUX_EXT_2_OFFSET 0x1e00
#define AUX_EXT_3_OFFSET 0x1e40
#define AUX_EXT_4_OFFSET 0x1e80

#define AUX_INS_1_OFFSET 0x1200
#define AUX_INS_2_OFFSET 0x1240
#define AUX_INS_3_OFFSET 0x1280
#define AUX_INS_4_OFFSET 0x12c0

static const ULWord	gChannelToAuxExtOffset[] = { AUX_EXT_1_OFFSET, AUX_EXT_2_OFFSET, AUX_EXT_3_OFFSET, AUX_EXT_4_OFFSET, 0 };

static const ULWord	gChannelToAuxInsOffset[] = { AUX_INS_1_OFFSET, AUX_INS_2_OFFSET, AUX_INS_3_OFFSET, AUX_INS_4_OFFSET, 0 };


bool SetupAuxExtractor(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2Standard theStandard = GetStandard(context, channel);

	switch (theStandard)
	{
	case NTV2_STANDARD_1080p:
		if (GetQuadFrameEnable(context, channel))
		{
#ifdef AUX_EXT_DEBUG
			ntv2Message("SetupAuxExtractor - 2160p\n");
#endif
			SetAuxExtProgressive(context, channel, true);
			SetAuxExtField1StartLine(context, channel, 2244);
			SetAuxExtField2StartLine(context, channel, 0);
			SetAuxExtTotalFrameLines(context, channel, 2250);
			SetAuxExtFidLow(context, channel, 0);
			SetAuxExtFidHi(context, channel, 0);
		}
		else
		{
#ifdef AUX_EXT_DEBUG
			ntv2Message("SetupAuxExtractor - 1080p\n");
#endif
			SetAuxExtProgressive(context, channel, true);
			SetAuxExtField1StartLine(context, channel, 1122);
			SetAuxExtField2StartLine(context, channel, 0);
			SetAuxExtTotalFrameLines(context, channel, 1125);
			SetAuxExtFidLow(context, channel, 0);
			SetAuxExtFidHi(context, channel, 0);
		}
		break;
	case NTV2_STANDARD_1080:
#ifdef AUX_EXT_DEBUG
		ntv2Message("SetupAuxExtractor - 1080i\n");
#endif
		SetAuxExtProgressive(context, channel, false);
		SetAuxExtField1StartLine(context, channel, 561);
		SetAuxExtField2StartLine(context, channel, 1124);
		SetAuxExtTotalFrameLines(context, channel, 1125);
		SetAuxExtFidLow(context, channel, 1125);
		SetAuxExtFidHi(context, channel, 563);
		break;
	case NTV2_STANDARD_720:
#ifdef AUX_EXT_DEBUG
		ntv2Message("SetupAuxExtractor - 720p\n");
#endif
		SetAuxExtProgressive(context, channel, true);
		SetAuxExtField1StartLine(context, channel, 746);
		SetAuxExtField2StartLine(context, channel, 0);
		SetAuxExtTotalFrameLines(context, channel, 750);
		SetAuxExtFidLow(context, channel, 0);
		SetAuxExtFidHi(context, channel, 0);
		break;
	case NTV2_STANDARD_625:
#ifdef AUX_EXT_DEBUG
		ntv2Message("SetupAuxExtractor - 625\n");
#endif
		SetAuxExtProgressive(context, channel, false);
		SetAuxExtField1StartLine(context, channel, 311);
		SetAuxExtField2StartLine(context, channel, 1);
		SetAuxExtTotalFrameLines(context, channel, 625);
		SetAuxExtFidLow(context, channel, 625);
		SetAuxExtFidHi(context, channel, 312);
		break;
	case NTV2_STANDARD_525:
#ifdef AUX_EXT_DEBUG
		ntv2Message("SetupAuxExtractor - 525\n");
#endif
		SetAuxExtProgressive(context, channel, false);
		SetAuxExtField1StartLine(context, channel, 264);
		SetAuxExtField2StartLine(context, channel, 1);
		SetAuxExtTotalFrameLines(context, channel, 525);
		SetAuxExtFidLow(context, channel, 3);
		SetAuxExtFidHi(context, channel, 265);
		break;
	default:
#ifdef AUX_EXT_DEBUG
		ntv2Message("SetupAuxExtractor - default\n");
#endif
		return false;
	}

	// clear ignore
	ntv2WriteRegister(context, gChannelToAuxExtOffset[channel]+12, 0x0);
	ntv2WriteRegister(context, gChannelToAuxExtOffset[channel]+13, 0x0);
	ntv2WriteRegister(context, gChannelToAuxExtOffset[channel]+14, 0x0);
	ntv2WriteRegister(context, gChannelToAuxExtOffset[channel]+15, 0x0);

	SetAuxExtSynchro(context, channel);

	SetAuxExtField1StartAddr(context, channel, 0);
	SetAuxExtField1EndAddr(context, channel, 0);
	SetAuxExtField2StartAddr(context, channel, 0);
	SetAuxExtField2EndAddr(context, channel, 0);
#if 0
	KdPrint(("AUX status:\n"));
	KdPrint(("reg 0: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 0)));
	KdPrint(("reg 1: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 1)));
	KdPrint(("reg 2: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 2)));
	KdPrint(("reg 3: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 3)));
	KdPrint(("reg 4: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 4)));
	KdPrint(("reg 5: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 5)));
	KdPrint(("reg 6: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 6)));
	KdPrint(("reg 7: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 7)));
	KdPrint(("reg 8: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 8)));
	KdPrint(("reg 9: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 9)));
	KdPrint(("reg 10: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 10)));
	KdPrint(("reg 11: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 11)));
	KdPrint(("reg 12: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 12)));
	KdPrint(("reg 13: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 13)));
	KdPrint(("reg 14: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 14)));
	KdPrint(("reg 15: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 15)));
	KdPrint(("reg 16: %08x\n", ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + 16)));
#endif
	return true;
}

bool EnableAuxExtractor(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
#ifdef AUX_EXT_DEBUG
	ULWord regNum = gChannelToAuxExtOffset[channel] + regAuxExtControl;
	KdPrint(("EnableAuxExtractor - channel: %d, reg: %d, enable %d\n", channel + 1, regNum, (int)bEnable));
#endif

	return ntv2WriteRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtControl, bEnable ? 0 : 1, maskAuxDisableExtractor, shiftAuxDisableExtractor);
}

bool SetAuxExtWriteParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber)
{
	//Calculate where AUX Extractor will put the data
	ULWord nextFrame = frameNumber+1;//This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord endOfFrameLocation = GetFrameBufferSize(context, (channel < NTV2_CHANNEL5) ? NTV2_CHANNEL1 : NTV2_CHANNEL5)* nextFrame;
	ULWord AUXStartMemory = endOfFrameLocation - ntv2ReadVirtualRegister(context, kVRegAncField1Offset);
	ULWord AUXStopMemory = endOfFrameLocation - ntv2ReadVirtualRegister(context, kVRegAncField2Offset);
	AUXStopMemory -= 1;
	SetAuxExtField1StartAddr(context, channel, AUXStartMemory);
	SetAuxExtField1EndAddr(context, channel, AUXStopMemory);
	return true;
}

bool SetAuxExtField2WriteParams(Ntv2SystemContext* context, NTV2Channel channel, ULWord frameNumber)
{
	//Calculate where AUX Extractor will put the data
	ULWord nextFrame = frameNumber+1;//This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord endOfFrameLocation = GetFrameBufferSize(context, (channel < NTV2_CHANNEL5) ? NTV2_CHANNEL1 : NTV2_CHANNEL5)* nextFrame;
	ULWord AUXStartMemory = endOfFrameLocation - ntv2ReadVirtualRegister(context, kVRegAncField2Offset);
	ULWord AUXStopMemory = endOfFrameLocation - 1;
	SetAuxExtField2StartAddr(context, channel, AUXStartMemory);
	SetAuxExtField2EndAddr(context, channel, AUXStopMemory);
	return true;
}

bool SetAuxExtProgressive(Ntv2SystemContext* context, NTV2Channel channel, bool bEnable)
{
	return ntv2WriteRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtControl, bEnable ? 1 : 0, maskAuxSetProgressive, shiftAuxSetProgressive);
}

bool SetAuxExtSynchro(Ntv2SystemContext* context, NTV2Channel channel)
{
	return ntv2WriteRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtControl,0x1, maskAuxSyncro, shiftAuxSyncro);
}

bool SetAuxExtField1StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr)
{
#ifdef AUX_EXT_DEBUG
	ULWord regNum = gChannelToAuxExtOffset[channel] + regAuxExtField1StartAddress;
	ntv2Message("SetAuxExtField1StartAddr - channel: %d, reg: %d, addr: %08x\n", channel + 1, regNum, addr);
#endif
	bool status = ntv2WriteRegister(context, gChannelToAuxExtOffset[channel] + regAuxExtField1StartAddress, addr);
#ifdef AUX_READ_BACK
	ULWord value = ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + regAuxExtField1StartAddress);
	ntv2Message("SetAuxExtField1StartAddr - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool SetAuxExtField1EndAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr)
{
#ifdef AUX_EXT_DEBUG
	ULWord regNum = gChannelToAuxExtOffset[channel] + regAuxExtField1EndAddress;
	ntv2Message("SetAuxExtField1EndAddr - channel: %d, reg: %d, addr: %08x\n", channel + 1, regNum, addr);
#endif
	bool status = ntv2WriteRegister(context, gChannelToAuxExtOffset[channel] + regAuxExtField1EndAddress, addr);
#ifdef AUX_READ_BACK
	ULWord value = ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + regAuxExtField1EndAddress);
	ntv2Message("SetAuxExtField1EndAddr - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool SetAuxExtField2StartAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr)
{
#ifdef AUX_EXT_DEBUG
	ULWord regNum = gChannelToAuxExtOffset[channel] + regAuxExtField2StartAddress;
	ntv2Message("SetAuxExtField2StartAddr - channel: %d, reg: %d, addr: %08x\n", channel + 1, regNum, addr);
#endif
	bool status = ntv2WriteRegister(context, gChannelToAuxExtOffset[channel] + regAuxExtField2StartAddress, addr);
#ifdef AUX_READ_BACK
	ULWord value = ntv2ReadRegister(context, gChannelToAuxExtOffset[channel] + regAuxExtField2StartAddress);
	ntv2Message("SetAuxExtField2StartAddr - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool SetAuxExtField2EndAddr(Ntv2SystemContext* context, NTV2Channel channel, ULWord addr)
{
	bool status = true;

	(void)context;
	(void)channel;
	(void)addr;
#ifdef AUX_EXT_DEBUG
	ntv2Message("SetAuxExtField2EndAddr - channel: %d, reg: %d, addr: %08x\n", channel + 1, 0, addr);
#endif
#ifdef AUX_READ_BACK
	ULWord value = 0
	ntv2Message("SetAuxExtField2EndAddr - channel: %d, %d\n", channel + 1, value);
#endif
	return status;
}

bool IsAuxExtOverrun(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtTotalStatus, &value, maskAuxTotalOverrun, shiftAuxTotalOverrun);
#ifdef AUX_EXT_DEBUG
	if (value)
	{
		ntv2Message("IsAuxExtOverrun - channel: %d overrun\n", channel + 1);
	}
#endif
	return value == 1 ? true : false;
}

ULWord GetAuxExtField1Bytes(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtField1Status, &value, maskAuxFieldBytesIn, shiftAuxFieldBytesIn);
#ifdef AUX_EXT_DEBUG
	ntv2Message("CNTV2Device::GetAuxExtField1Bytes - channel: %d, %d\n", channel + 1, value);
#endif
	return value;
}

bool IsAuxExtField1Overrun(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtField1Status, &value, maskAuxFieldOverrun, shiftAuxFieldOverrun);
	return value == 1 ? true : false;
}

ULWord GetAuxExtField2Bytes(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtField2Status, &value, maskAuxFieldBytesIn, shiftAuxFieldBytesIn);
#ifdef AUX_EXT_DEBUG
	KdPrint(("CNTV2Device::GetAuxExtField2Bytes - channel: %d, %d\n", channel + 1, value));
#endif
	return value;
}

bool IsAuxExtField2Overrun(Ntv2SystemContext* context, NTV2Channel channel)
{
	ULWord value = 0;
	ntv2ReadRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtField2Status, &value, maskAuxFieldOverrun, shiftAuxFieldOverrun);
#ifdef AUX_EXT_DEBUG
	if (value)
	{
		ntv2Message("IsAuxExtField2Overrun - channel: %d field 2 overrun\n", channel + 1);
	}
#endif
	return value == 1 ? true : false;
}

bool SetAuxExtField1StartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtFieldVBLStartLine, lineNumber, maskAuxField1StartLine, shiftAuxField1StartLine);
}

bool SetAuxExtField2StartLine(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtFieldVBLStartLine, lineNumber, maskAuxField2StartLine, shiftAuxField2StartLine);
}

bool SetAuxExtTotalFrameLines(Ntv2SystemContext* context, NTV2Channel channel, ULWord totalFrameLines)
{
	return ntv2WriteRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtTotalFrameLines, totalFrameLines, maskAuxTotalFrameLines, shiftAuxTotalFrameLines);
}

bool SetAuxExtFidLow(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtFID, lineNumber, maskAuxFIDLow, shiftAuxFIDLow);
}

bool SetAuxExtFidHi(Ntv2SystemContext* context, NTV2Channel channel, ULWord lineNumber)
{
	return ntv2WriteRegisterMS(context, gChannelToAuxExtOffset[channel] + regAuxExtFID, lineNumber, maskAuxFIDHi, shiftAuxFIDHi);
}

