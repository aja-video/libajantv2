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
}



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
}

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
