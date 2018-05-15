/**
	@file	ntv2anc.cpp
	@brief	Implementations of anc-centric NTV2Card methods.
	@copyright	(C) 2004-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2formatdescriptor.h"

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

typedef struct ANCInserterInitParams
{
	uint32_t field1ActiveLine;
	uint32_t field2ActiveLine;
	uint32_t hActivePixels;
	uint32_t hTotalPixels;
	uint32_t totalLines;
	uint32_t fidLow;
	uint32_t fidHigh;
}ANCInserterInitParams;

const static ANCInserterInitParams inserterInitParamsTable[NTV2_NUM_STANDARDS] =
{
	/*NTV2_STANDARD_1080*/			{	21,		564,	1920,	2200,	1125,	1125,	563	},
	/*NTV2_STANDARD_720*/			{	26,		0,		1280,	1280,	750,	0,		0	},
	/*NTV2_STANDARD_525*/			{	21,		283,	720,	720,	525,	3,		265	},
	/*NTV2_STANDARD_625*/			{	23,		336,	720,	864,	625,	625,	312	},
	/*NTV2_STANDARD_1080p*/			{	42,		0,		1920,	2640,	1125,	0,		0	},
	/*NTV2_STANDARD_2K*/			{	42,		0,		2048,	2640,	1125,	0,		0	},
	/*NTV2_STANDARD_2Kx1080p*/		{	42,		0,		2048,	2640,	1125,	0,		0	},
	/*NTV2_STANDARD_2Kx1080i*/		{	21,		564,	2048,	2200,	1125,	1125,	563	},
	/*NTV2_STANDARD_3840x2160p*/	{	42,		0,		1920,	2640,	1125,	0,		0	},
	/*NTV2_STANDARD_4096x2160p*/	{	42,		0,		2048,	2640,	1125,	0,		0	},
	/*NTV2_STANDARD_3840HFR*/		{	42,		0,		1920,	2640,	1125,	0,		0	},
	/*NTV2_STANDARD_4096HFR*/		{	42,		0,		2048,	2640,	1125,	0,		0	}
};



bool CNTV2Card::AncInsertInit(NTV2Channel channel, NTV2VideoFormat videoFormat)
{
	NTV2Standard theStandard = GetNTV2StandardFromVideoFormat(videoFormat);
	ANCInserterInitParams initParams = inserterInitParamsTable[theStandard];
	SetAncInsField1ActiveLine(channel, initParams.field1ActiveLine);
	SetAncInsField2ActiveLine(channel, initParams.field2ActiveLine);
	SetAncInsHActivePixels(channel, initParams.hActivePixels);
	SetAncInsHTotalPixels(channel, initParams.hTotalPixels);
	SetAncInsTotalLines(channel, initParams.totalLines);
	SetAncInsFidLow(channel, initParams.fidLow);
	SetAncInsFidHi(channel, initParams.fidHigh);
	SetAncInsProgressive(channel, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	SetAncInsSDPacketSplit(channel, NTV2_IS_SD_STANDARD(theStandard));
	EnableAncInsHancC(channel, true);
	EnableAncInsHancY(channel, true);
	EnableAncInsVancC(channel, true);
	EnableAncInsVancY(channel, true);
	SetAncInsHancPixelDelay(channel, 0);
	SetAncInsVancPixelDelay(channel, 0);
	WriteRegister(gChannelToAncInsOffset[channel] + regAncInsBlankCStartLine, 0);
	WriteRegister(gChannelToAncInsOffset[channel] + regAncInsBlankField1CLines, 0);
	WriteRegister(gChannelToAncInsOffset[channel] + regAncInsBlankField2CLines, 0);
    uint32 ancField1Offset = 0;
    ReadRegister(kVRegAncField1Offset, &ancField1Offset);
    uint32 ancField2Offset = 0;
    ReadRegister(kVRegAncField2Offset, &ancField2Offset);
    uint32 fieldBytes = (ancField2Offset - ancField1Offset)-1;
    SetAncInsField1Bytes(channel, fieldBytes);
    SetAncInsField2Bytes(channel, fieldBytes);
	return true;
}

typedef struct ANCExtractorInitParams
{
	uint32_t field1StartLine;
	uint32_t field1CutoffLine;
	uint32_t field2StartLine;
	uint32_t field2CutoffLine;
	uint32_t totalLines;
	uint32_t fidLow;
	uint32_t fidHigh;
	uint32_t field1AnalogStartLine;
	uint32_t field2AnalogStartLine;
	uint32_t field1AnalogYFilter;
	uint32_t field2AnalogYFilter;
	uint32_t field1AnalogCFilter;
	uint32_t field2AnalogCFilter;
	uint32_t analogActiveLineLength;
}ANCExtractorInitParams;

const static ANCExtractorInitParams extractorInitParamsTable[NTV2_NUM_STANDARDS] =
{
	/*NTV2_STANDARD_1080*/			{	561,	26,		1124,	588,	1125,	1125,	563,	0,	0,		0,			0,			0,	0,	0x07800000	},
	/*NTV2_STANDARD_720*/			{	746,	745,	0,		0,		750,	0,		0,		0,	0,		0,			0,			0,	0,	0x05000000	},
	/*NTV2_STANDARD_525*/			{	264,	30,		1,		293,	525,	3,		265,	4,	266,	0x20000,	0x40000,	0,	0,	0x02D00000	},
	/*NTV2_STANDARD_625*/			{	23,		625,	336,	625,	625,	1,		313,	5,	318,	0x10000,	0x10000,	0,	0,	0x02D00000	},
	/*NTV2_STANDARD_1080p*/			{	1122,	1125,	0,		0,		1125,	0,		0,		0,	0,		0,			0,			0,	0,	0x07800000	},
	/*NTV2_STANDARD_2K*/			{	1122,	1125,	0,		0,		1125,	0,		0,		0,	0,		0,			0,			0,	0,	0x07800000	},
	/*NTV2_STANDARD_2Kx1080p*/		{	1122,	1125,	0,		0,		1125,	0,		0,		0,	0,		0,			0,			0,	0,	0x07800000	},
	/*NTV2_STANDARD_2Kx1080i*/		{	561,	26,		1124,	588,	1125,	1125,	563,	0,	0,		0,			0,			0,	0,	0x07800000	},
	/*NTV2_STANDARD_3840x2160p*/	{	1122,	1125,	0,		0,		1125,	0,		0,		0,	0,		0,			0,			0,	0,	0x07800000	},
	/*NTV2_STANDARD_4096x2160p*/	{	1122,	1125,	0,		0,		1125,	0,		0,		0,	0,		0,			0,			0,	0,	0x07800000	},
	/*NTV2_STANDARD_3840HFR*/		{	1122,	1125,	0,		0,		1125,	0,		0,		0,	0,		0,			0,			0,	0,	0x07800000	},
	/*NTV2_STANDARD_4096HFR*/		{	1122,	1125,	0,		0,		1125,	0,		0,		0,	0,		0,			0,			0,	0,	0x07800000	},
};

bool CNTV2Card::AncExtractInit(NTV2Channel channel, NTV2VideoFormat videoFormat)
{
	NTV2Standard theStandard = GetNTV2StandardFromVideoFormat(videoFormat);
	ANCExtractorInitParams extractorParams = extractorInitParamsTable[theStandard];
	SetAncExtProgressive(channel, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	SetAncExtField1StartLine(channel, extractorParams.field1StartLine);
	SetAncExtField1CutoffLine(channel, extractorParams.field1CutoffLine);
	SetAncExtField2StartLine(channel, extractorParams.field2StartLine);
	SetAncExtField2CutoffLine(channel, extractorParams.field2CutoffLine);
	SetAncExtTotalFrameLines(channel, extractorParams.totalLines);
	SetAncExtFidLow(channel, extractorParams.fidLow);
	SetAncExtFidHi(channel, extractorParams.fidHigh);
	SetAncExtField1AnalogStartLine(channel, extractorParams.field1AnalogStartLine);
	SetAncExtField2AnalogStartLine(channel, extractorParams.field2AnalogStartLine);
	SetAncExtField1AnalogYFilter(channel, extractorParams.field1AnalogYFilter);
	SetAncExtField2AnalogYFilter(channel, extractorParams.field2AnalogYFilter);
	SetAncExtField1AnalogCFilter(channel, extractorParams.field1AnalogCFilter);
	SetAncExtField2AnalogCFilter(channel, extractorParams.field2AnalogCFilter);
	WriteRegister(gChannelToAncExtOffset[channel]+12, 0xE4E5E6E7);	//	Ignore audio
	WriteRegister(gChannelToAncExtOffset[channel]+13, 0xE0E1E2E3);	//	Ignore audio
	WriteRegister(gChannelToAncExtOffset[channel]+14, 0xA4A5A6A7);	//	Ignore audio
	WriteRegister(gChannelToAncExtOffset[channel]+15, 0xA0A1A2A3);	//	Ignore audio
	WriteRegister(gChannelToAncExtOffset[channel]+16, 0x0);
	WriteRegister(gChannelToAncExtOffset[channel]+27, extractorParams.analogActiveLineLength);
	SetAncExtSDDemux(channel, NTV2_IS_SD_STANDARD(theStandard));
	EnableAncExtHancC(channel, true);
	EnableAncExtHancY(channel, true);
	EnableAncExtVancC(channel, true);
	EnableAncExtVancY(channel, true);
	SetAncExtSynchro(channel);
	SetAncExtField1StartAddr(channel, 0);
	SetAncExtField1EndAddr(channel, 0);
	SetAncExtField2StartAddr(channel, 0);
	SetAncExtField2EndAddr(channel, 0);

	return true;
}









bool CNTV2Card::EnableAncExtractor(NTV2Channel channel, bool bEnable)
{
    if (!bEnable)
    {
        EnableAncExtHancC(channel, false);
        EnableAncExtHancY(channel, false);
        EnableAncExtVancC(channel, false);
        EnableAncExtVancY(channel, false);
    }
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 0 : 1, maskDisableExtractor, shiftDisableExtractor);
}

bool CNTV2Card::SetAncExtWriteParams(NTV2Channel channel, ULWord frameNumber)
{
    //Calculate where ANC Extractor will put the data
    frameNumber++;//This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
    ULWord endOfFrameLocation = 0;
    NTV2Framesize frameSize = 0
    GetFrameBufferSize(channel, frameSize);
    endOfFrameLocation = ::NTV2FramesizeToByteCount(fbSize);
    endOfFrameLocation *= frameNumber;
    ULWord quadFormatEnabled = 0;
    GetQuadFrameEnable(&quadFormatEnabled, channel);
    if(quadFormatEnabled)
        endOfFrameLocation *= 4;
    ULWord ancField1Offset = 0;
    ReadRegister(kVRegAncField1Offset, &ancField1Offset);
    ULWord ancField2Offset = 0;
    ReadRegister(kVRegAncField2Offset, &ancField2Offset);
    ULWord ANCStartMemory = endOfFrameLocation - ancField1Offset;
    ULWord ANCStopMemory = endOfFrameLocation - ancField2Offset;
    ANCStopMemory -= 1;
    SetAncExtField1StartAddr(channel, ANCStartMemory);
    SetAncExtField1EndAddr(channel, ANCStopMemory);
    return true;
}

bool CNTV2Card::SetAncExtField2WriteParams(NTV2Channel channel, ULWord frameNumber)
{
    frameNumber++;//This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
    ULWord endOfFrameLocation = 0;
    NTV2Framesize frameSize = 0
    GetFrameBufferSize(channel, frameSize);
    endOfFrameLocation = ::NTV2FramesizeToByteCount(fbSize);
    endOfFrameLocation *= frameNumber;
    ULWord quadFormatEnabled = 0;
    GetQuadFrameEnable(&quadFormatEnabled, channel);
    if(quadFormatEnabled)
        endOfFrameLocation *= 4;
    ULWord ancField2Offset = 0;
    ReadRegister(kVRegAncField2Offset, &ancField2Offset);
    ULWord ANCStartMemory = endOfFrameLocation - ancField2Offset;
    ULWord ANCStopMemory = endOfFrameLocation - 1;
    SetAncExtField2StartAddr(channel, ANCStartMemory);
    SetAncExtField2EndAddr(channel, ANCStopMemory);
    return true;
}

bool CNTV2Card::EnableAncExtHancY(NTV2Channel channel, bool bEnable)
{
    bool status =  m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableHancY, shiftEnableHancY);
    return status;
}

bool CNTV2Card::EnableAncExtHancC(NTV2Channel channel, bool bEnable)
{
    bool status = m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableHancC, shiftEnableHancC);
    return status;
}

bool CNTV2Card::EnableAncExtVancY(NTV2Channel channel, bool bEnable)
{
    bool status = m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableVancY, shiftEnableVancY);
    return status;
}

bool CNTV2Card::EnableAncExtVancC(NTV2Channel channel, bool bEnable)
{
    bool status =  m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableVancC, shiftEnableVancC);
    return status;
}

bool CNTV2Card::SetAncExtSDDemux(NTV2Channel channel, bool bEnable)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskEnableSDMux, shiftEnableSDMux);
}

bool CNTV2Card::SetAncExtProgressive(NTV2Channel channel, bool bEnable)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, maskSetProgressive, shiftSetProgressive);
}

bool CNTV2Card::SetAncExtSynchro(NTV2Channel channel)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl,0x1, maskSyncro, shiftSyncro);
}

bool CNTV2Card::SetAncExtLSBEnable(NTV2Channel channel, bool bEnable)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtControl, bEnable ? 1 : 0, (ULWord)maskGrabLSBs, shiftGrabLSBs);
}

bool CNTV2Card::SetAncExtField1StartAddr(NTV2Channel channel, ULWord addr)
{
    bool status = m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtField1StartAddress, addr);
    return status;
}

bool CNTV2Card::SetAncExtField1EndAddr(NTV2Channel channel, ULWord addr)
{
    bool status = m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtField1EndAddress, addr);
    return status;
}

bool CNTV2Card::SetAncExtField2StartAddr(NTV2Channel channel, ULWord addr)
{
    bool status = m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtField2StartAddress, addr);
    return status;
}

bool CNTV2Card::SetAncExtField2EndAddr(NTV2Channel channel, ULWord addr)
{
    bool status = m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtField2EndAddress, addr);
    return status;
}

bool CNTV2Card::SetAncExtField1CutoffLine(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtFieldCutoffLine, lineNumber, maskField1CutoffLine, shiftField1CutoffLine);
}

bool CNTV2Card::SetAncExtField2CutoffLine(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtFieldCutoffLine, lineNumber, maskField2CutoffLine, shiftField2CutoffLine);
}

bool CNTV2Card::IsAncExtOverrun(NTV2Channel channel)
{
    ULWord value = 0;
    m_pRegisters->ReadRegister(gChannelToAncExtOffset[channel] + regAncExtTotalStatus, &value, maskTotalOverrun, shiftTotalOverrun);
    return value == 1 ? true : false;
}

ULWord CNTV2Card::GetAncExtField1Bytes(NTV2Channel channel)
{
    ULWord value = 0;
    m_pRegisters->ReadRegister(gChannelToAncExtOffset[channel] + regAncExtField1Status, &value, maskField1BytesIn, shiftField1BytesIn);
    return value;
}

bool CNTV2Card::IsAncExtField1Overrun(NTV2Channel channel)
{
    ULWord value = 0;
    m_pRegisters->ReadRegister(gChannelToAncExtOffset[channel] + regAncExtField1Status, &value, maskField1Overrun, shiftField1Overrun);
    return value == 1 ? true : false;
}

ULWord CNTV2Card::GetAncExtField2Bytes(NTV2Channel channel)
{
    ULWord value = 0;
    m_pRegisters->ReadRegister(gChannelToAncExtOffset[channel] + regAncExtField2Status, &value, maskField2BytesIn, shiftField2BytesIn);
    return value;
}

bool CNTV2Card::IsAncExtField2Overrun(NTV2Channel channel)
{
    ULWord value = 0;
    m_pRegisters->ReadRegister(gChannelToAncExtOffset[channel] + regAncExtField2Status, &value, maskField2Overrun, shiftField2Overrun);
    return value == 1 ? true : false;
}

bool CNTV2Card::SetAncExtField1StartLine(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtFieldVBLStartLine, lineNumber, maskField1StartLine, shiftField1StartLine);
}

bool CNTV2Card::SetAncExtField2StartLine(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtFieldVBLStartLine, lineNumber, maskField2StartLine, shiftField2StartLine);
}

bool CNTV2Card::SetAncExtTotalFrameLines(NTV2Channel channel, ULWord totalFrameLines)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtTotalFrameLines, totalFrameLines, maskTotalFrameLines, shiftTotalFrameLines);
}

bool CNTV2Card::SetAncExtFidLow(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtFID, lineNumber, maskFIDLow, shiftFIDLow);
}

bool CNTV2Card::SetAncExtFidHi(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtFID, lineNumber, maskFIDHi, shiftFIDHi);
}

bool CNTV2Card::SetAncExtField1AnalogStartLine(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtAnalogStartLine, lineNumber, maskField1AnalogStartLine, shiftField1AnalogStartLine);
}

bool CNTV2Card::SetAncExtField2AnalogStartLine(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtAnalogStartLine, lineNumber, maskField2AnalogStartLine, shiftField2AnalogStartLine);
}

bool CNTV2Card::SetAncExtField1AnalogYFilter(NTV2Channel channel, ULWord lineFilter)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtField1AnalogYFilter, lineFilter);
}

bool CNTV2Card::SetAncExtField2AnalogYFilter(NTV2Channel channel, ULWord lineFilter)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtField2AnalogYFilter, lineFilter);
}

bool CNTV2Card::SetAncExtField1AnalogCFilter(NTV2Channel channel, ULWord lineFilter)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtField1AnalogCFilter, lineFilter);
}

bool CNTV2Card::SetAncExtField2AnalogCFilter(NTV2Channel channel, ULWord lineFilter)
{
    return m_pRegisters->WriteRegister(gChannelToAncExtOffset[channel] + regAncExtField2AnalogCFilter, lineFilter);
}



bool CNTV2Card::EnableAncInserter(NTV2Channel channel, bool bEnable)
{
    if (!bEnable)
    {
        EnableAncInsHancC(channel, false);
        EnableAncInsHancY(channel, false);
        EnableAncInsVancC(channel, false);
        EnableAncInsVancY(channel, false);
    }
    m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsBlankCStartLine, 0);
    m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsBlankField1CLines, 0);
    m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsBlankField2CLines, 0);
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 0 : 1, maskInsDisableInserter, shiftInsDisableInserter);
}

bool CNTV2Card::SetAncInsReadParams(NTV2Channel channel, ULWord frameNumber, ULWord field1Size, ULWord field2Size)
{
    //Calculate where ANC Extractor will put the data
    frameNumber++; //Start at the beginning of next frame and subtract offset
    ULWord frameLocation = GetFrameBufferSize(channel < NTV2_CHANNEL5 ? NTV2_CHANNEL1 : NTV2_CHANNEL5)* (frameNumber);
    ULWord ANCStartMemory = frameLocation - m_pRegisters->ReadVirtualRegister(kVRegAncField1Offset);
    SetAncInsField1StartAddr(channel, ANCStartMemory);
    SetAncInsField1Bytes(channel, field1Size);
    mAncF2StartMemory[channel] = frameLocation - m_pRegisters->ReadVirtualRegister(kVRegAncField2Offset);
    mAncF2Size[channel] = field2Size;
    if (m_pRegisters->ReadVirtualRegister(kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
    {
        //For retail mode we will setup all the anc inserters to read from the same location
        for (ULWord i = 0; i < NTV2DeviceGetNumVideoOutputs(m_DeviceID); i++)
        {
            SetAncInsField1StartAddr((NTV2Channel)i, ANCStartMemory);
            SetAncInsField1Bytes((NTV2Channel)i, field1Size);
        }
    }
    return true;
}

bool CNTV2Card::SetAncInsReadField2Params(NTV2Channel channel)
{
    SetAncInsField2StartAddr(channel, mAncF2StartMemory[channel]);
    SetAncInsField2Bytes(channel, mAncF2Size[channel]);
    if (m_pRegisters->ReadVirtualRegister(kVRegEveryFrameTaskFilter) == NTV2_STANDARD_TASKS)
    {
        //For retail mode we will setup all the anc inserters to read from the same location
        for (ULWord i = 0; i < NTV2DeviceGetNumVideoOutputs(m_DeviceID); i++)
        {
            SetAncInsField2StartAddr((NTV2Channel)i, mAncF2StartMemory[channel]);
            SetAncInsField2Bytes((NTV2Channel)i, mAncF2Size[channel]);
        }
    }
    return true;
}

bool CNTV2Card::SetAncInsField1Bytes(NTV2Channel channel, ULWord numberOfBytes)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsFieldBytes, numberOfBytes, maskInsField1Bytes, shiftInsField1Bytes);
}

bool CNTV2Card::SetAncInsField2Bytes(NTV2Channel channel, ULWord numberOfBytes)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsFieldBytes, numberOfBytes, (ULWord)maskInsField2Bytes, shiftInsField2Bytes);
}

bool CNTV2Card::EnableAncInsHancY(NTV2Channel channel, bool bEnable)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 1 : 0, maskInsEnableHancY, shiftInsEnableHancY);
}

bool CNTV2Card::EnableAncInsHancC(NTV2Channel channel, bool bEnable)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 1 : 0, maskInsEnableHancC, shiftInsEnableHancY);
}

bool CNTV2Card::EnableAncInsVancY(NTV2Channel channel, bool bEnable)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 1 : 0, maskInsEnableVancY, shiftInsEnableVancY);
}

bool CNTV2Card::EnableAncInsVancC(NTV2Channel channel, bool bEnable)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsControl, bEnable ? 1 : 0, maskInsEnableVancC, shiftInsEnableVancC);
}

bool CNTV2Card::SetAncInsProgressive(NTV2Channel channel, bool isProgressive)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsControl, isProgressive ? 1 : 0, maskInsSetProgressive, shiftInsSetProgressive);
}

bool CNTV2Card::SetAncInsSDPacketSplit(NTV2Channel channel, bool inEnable)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsControl,  inEnable ? 1 : 0,  ULWord(maskInsEnablePktSplitSD), shiftInsEnablePktSplitSD);
}

bool CNTV2Card::SetAncInsField1StartAddr(NTV2Channel channel, ULWord startAddr)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsField1StartAddr, startAddr);
}

bool CNTV2Card::SetAncInsField2StartAddr(NTV2Channel channel, ULWord startAddr)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsField2StartAddr, startAddr);
}

bool CNTV2Card::SetAncInsHancPixelDelay(NTV2Channel channel, ULWord numberOfPixels)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsPixelDelay, numberOfPixels, maskInsHancDelay, shiftINsHancDelay);
}

bool CNTV2Card::SetAncInsVancPixelDelay(NTV2Channel channel, ULWord numberOfPixels)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsPixelDelay, numberOfPixels, maskInsVancDelay, shiftInsVancDelay);
}

bool CNTV2Card::SetAncInsField1ActiveLine(NTV2Channel channel, ULWord activeLineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsActiveStart, activeLineNumber, maskInsField1FirstActive, shiftInsField1FirstActive);
}

bool CNTV2Card::SetAncInsField2ActiveLine(NTV2Channel channel, ULWord activeLineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsActiveStart, activeLineNumber, maskInsField2FirstActive, shiftInsField2FirstActive);
}

bool CNTV2Card::SetAncInsHActivePixels(NTV2Channel channel, ULWord numberOfActiveLinePixels)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsLinePixels, numberOfActiveLinePixels, maskInsActivePixelsInLine, shiftInsActivePixelsInLine);
}

bool CNTV2Card::SetAncInsHTotalPixels(NTV2Channel channel, ULWord numberOfLinePixels)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsLinePixels, numberOfLinePixels, maskInsTotalPixelsInLine, shiftInsTotalPixelsInLine);
}

bool CNTV2Card::SetAncInsTotalLines(NTV2Channel channel, ULWord numberOfLines)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsFrameLines, numberOfLines, maskInsTotalLinesPerFrame, shiftInsTotalLinesPerFrame);
}

bool CNTV2Card::SetAncInsFidHi(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsFieldIDLines, lineNumber, maskInsFieldIDHigh, shiftInsFieldIDHigh);
}

bool CNTV2Card::SetAncInsFidLow(NTV2Channel channel, ULWord lineNumber)
{
    return m_pRegisters->WriteRegister(gChannelToAncInsOffset[channel] + regAncInsFieldIDLines, lineNumber, maskInsFieldIDLow, shiftInsFieldIDLow);
}
