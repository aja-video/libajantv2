/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2xpt.c
//
//==========================================================================

#include "ntv2xpt.h"

static const uint32_t	gChannelToSDIOutInSelectRegNum []	= {	kRegXptSelectGroup3, kRegXptSelectGroup3, kRegXptSelectGroup8, kRegXptSelectGroup8,
																kRegXptSelectGroup8, kRegXptSelectGroup22, kRegXptSelectGroup22, kRegXptSelectGroup30, 0};
static const uint32_t	gChannelToSDIOutInSelectMask []	= {	kK2RegMaskSDIOut1InputSelect, kK2RegMaskSDIOut2InputSelect, kK2RegMaskSDIOut3InputSelect, kK2RegMaskSDIOut4InputSelect,
																kK2RegMaskSDIOut5InputSelect, kK2RegMaskSDIOut6InputSelect, kK2RegMaskSDIOut7InputSelect, kK2RegMaskSDIOut8InputSelect, 0};
static const uint32_t	gChannelToSDIOutInSelectShift []	= {	kK2RegShiftSDIOut1InputSelect, kK2RegShiftSDIOut2InputSelect, kK2RegShiftSDIOut3InputSelect, kK2RegShiftSDIOut4InputSelect,
																kK2RegShiftSDIOut5InputSelect, kK2RegShiftSDIOut6InputSelect, kK2RegShiftSDIOut7InputSelect, kK2RegShiftSDIOut8InputSelect, 0};

static const uint32_t	gChannelToSDIOutDS2InSelectRegNum []	= {	kRegXptSelectGroup10, kRegXptSelectGroup10, kRegXptSelectGroup14, kRegXptSelectGroup14,
																kRegXptSelectGroup14, kRegXptSelectGroup22, kRegXptSelectGroup22, kRegXptSelectGroup30, 0};
static const uint32_t	gChannelToSDIOutDS2InSelectMask []	= {	kK2RegMaskSDIOut1DS2InputSelect, kK2RegMaskSDIOut2DS2InputSelect, kK2RegMaskSDIOut3DS2InputSelect, (uint32_t)kK2RegMaskSDIOut4DS2InputSelect,
																kK2RegMaskSDIOut5DS2InputSelect, kK2RegMaskSDIOut6DS2InputSelect, (uint32_t)kK2RegMaskSDIOut7DS2InputSelect, kK2RegMaskSDIOut8DS2InputSelect, 0};
static const uint32_t	gChannelToSDIOutDS2InSelectShift []	= {	kK2RegShiftSDIOut1DS2InputSelect, kK2RegShiftSDIOut2DS2InputSelect, kK2RegShiftSDIOut3DS2InputSelect, kK2RegShiftSDIOut4DS2InputSelect,
																kK2RegShiftSDIOut5DS2InputSelect, kK2RegShiftSDIOut6DS2InputSelect, kK2RegShiftSDIOut7DS2InputSelect, kK2RegShiftSDIOut8DS2InputSelect, 0};

static const uint32_t	gChannelToDLOutInSelectRegNum []	= {	kRegXptSelectGroup2, kRegXptSelectGroup7, kRegXptSelectGroup16, kRegXptSelectGroup16,
																kRegXptSelectGroup16, kRegXptSelectGroup27, kRegXptSelectGroup27, kRegXptSelectGroup27, 0};
static const uint32_t	gChannelToDLOutInSelectMask []	= {	(uint32_t)kK2RegMaskDuallinkOutInputSelect, kK2RegMaskDuallinkOut2InputSelect, kK2RegMaskDuallinkOut3InputSelect, kK2RegMaskDuallinkOut4InputSelect,
																kK2RegMaskDuallinkOut5InputSelect, kK2RegMaskDuallinkOut6InputSelect, kK2RegMaskDuallinkOut7InputSelect, kK2RegMaskDuallinkOut8InputSelect, 0};
static const uint32_t	gChannelToDLOutInSelectShift []	= {	kK2RegShiftDuallinkOutInputSelect, kK2RegShiftDuallinkOut2InputSelect, kK2RegShiftDuallinkOut3InputSelect, kK2RegShiftDuallinkOut4InputSelect,
																kK2RegShiftDuallinkOut5InputSelect, kK2RegShiftDuallinkOut6InputSelect, kK2RegShiftDuallinkOut7InputSelect, kK2RegShiftDuallinkOut8InputSelect, 0};

static const uint32_t	gChannelToDLInInputSelectRegNum []	= {	kRegXptSelectGroup11, kRegXptSelectGroup11, kRegXptSelectGroup15, kRegXptSelectGroup15,
																kRegXptSelectGroup25, kRegXptSelectGroup25, kRegXptSelectGroup26, kRegXptSelectGroup26, 0};
static const uint32_t	gChannelToDLInInputSelectMask []	= {	kK2RegMaskDuallinkIn1InputSelect, kK2RegMaskDuallinkIn2InputSelect, kK2RegMaskDuallinkIn3InputSelect, kK2RegMaskDuallinkIn4InputSelect,
																kK2RegMaskDuallinkIn5InputSelect, kK2RegMaskDuallinkIn6InputSelect, kK2RegMaskDuallinkIn7InputSelect, kK2RegMaskDuallinkIn8InputSelect, 0};
static const uint32_t	gChannelToDLInInputSelectShift []	= {	kK2RegShiftDuallinkIn1InputSelect, kK2RegShiftDuallinkIn2InputSelect, kK2RegShiftDuallinkIn3InputSelect, kK2RegShiftDuallinkIn4InputSelect,
																kK2RegShiftDuallinkIn5InputSelect, kK2RegShiftDuallinkIn6InputSelect, kK2RegShiftDuallinkIn7InputSelect, kK2RegShiftDuallinkIn8InputSelect, 0};

static const uint32_t	gChannelToLUTInputSelectRegNum []	= {	kRegXptSelectGroup1, kRegXptSelectGroup5, kRegXptSelectGroup12, kRegXptSelectGroup12,
																kRegXptSelectGroup12, kRegXptSelectGroup24, kRegXptSelectGroup24, kRegXptSelectGroup24, 0};
static const uint32_t	gChannelToLUTInputSelectMask []	= {	kK2RegMaskXptLUTInputSelect, kK2RegMaskXptLUT2InputSelect, kK2RegMaskXptLUT3InputSelect, kK2RegMaskXptLUT4InputSelect,
																kK2RegMaskXptLUT5InputSelect, kK2RegMaskXptLUT6InputSelect, kK2RegMaskXptLUT7InputSelect, kK2RegMaskXptLUT8InputSelect, 0};
static const uint32_t	gChannelToLUTInputSelectShift []	= {	kK2RegShiftXptLUTInputSelect, kK2RegShiftXptLUT2InputSelect, kK2RegShiftXptLUT3InputSelect, kK2RegShiftXptLUT4InputSelect,
																kK2RegShiftXptLUT5InputSelect, kK2RegShiftXptLUT6InputSelect, kK2RegShiftXptLUT7InputSelect, kK2RegShiftXptLUT8InputSelect, 0};

static const uint32_t	gChannelToCSCVidInputSelectRegNum []	= {	kRegXptSelectGroup1, kRegXptSelectGroup5, kRegXptSelectGroup17, kRegXptSelectGroup17,
																kRegXptSelectGroup18, kRegXptSelectGroup30, kRegXptSelectGroup23, kRegXptSelectGroup23, 0};
static const uint32_t	gChannelToCSCVidInputSelectMask []	= {	kK2RegMaskColorSpaceConverterInputSelect, kK2RegMaskCSC2VidInputSelect, kK2RegMaskCSC3VidInputSelect, kK2RegMaskCSC4VidInputSelect,
																kK2RegMaskCSC5VidInputSelect, kK2RegMaskCSC6VidInputSelect, kK2RegMaskCSC7VidInputSelect, kK2RegMaskCSC8VidInputSelect, 0};
static const uint32_t	gChannelToCSCVidInputSelectShift []	= {	kK2RegShiftColorSpaceConverterInputSelect, kK2RegShiftCSC2VidInputSelect, kK2RegShiftCSC3VidInputSelect, kK2RegShiftCSC4VidInputSelect,
																kK2RegShiftCSC5VidInputSelect, kK2RegShiftCSC6VidInputSelect, kK2RegShiftCSC7VidInputSelect, kK2RegShiftCSC8VidInputSelect, 0};

//	In some cases, two entries will have the same comtents. For example, NTV2_XptDuallinkOut1 and NTV2_XptDuallinkOut1DS2 both
//	backtrace to the same crosspoint register and field: the Dual Link 1 Out Input Select in register 137.

//	4K down converter  only backtraces its first input.  This should be ok.
//	Mixers only backtrace the foreground video.  Is this good enough?

//	CSC Key backtraces through the video input, since the key will be generated from the alpha channel of the video input.

NTV2XptLookupEntry NTV2XptBackTraceTable [264] =
{
	{0, 0, 0},																													//	00
	{XPT_SDI_IN_1,			0,											0},														//	01 NTV2_XptSDIIn1
	{XPT_SDI_IN_2,			0,											0},														//	02 NTV2_XptSDIIn2
	{0, 0, 0},																													//	03
	{kRegXptSelectGroup1,	(ULWord)kK2RegMaskXptLUTInputSelect,				kK2RegShiftXptLUTInputSelect},					//	04 NTV2_XptLUT1YUV
	{kRegXptSelectGroup1,	(ULWord)kK2RegMaskColorSpaceConverterInputSelect,	kK2RegShiftColorSpaceConverterInputSelect},		//	05 NTV2_XptCSC1VidYUV
	{kRegXptSelectGroup1,	(ULWord)kK2RegMaskConversionModInputSelect,			kK2RegShiftConversionModInputSelect},			//	06 NTV2_XptConversionModule
	{kRegXptSelectGroup1,	(ULWord)kK2RegMaskCompressionModInputSelect,		kK2RegShiftCompressionModInputSelect},			//	07 NTV2_XptCompressionModule
	{XPT_FB_YUV_1,			0,											0},														//	08 NTV2_XptFrameBuffer1YUV
	{kRegXptSelectGroup2,	kK2RegMaskFrameSync1InputSelect,					kK2RegShiftFrameSync1InputSelect},				//	09 ! NTV2_XptFrameSync1YUV
	{kRegXptSelectGroup2,	kK2RegMaskFrameSync2InputSelect,					kK2RegShiftFrameSync2InputSelect},				//	0A ! NTV2_XptFrameSync2YUV
	{kRegXptSelectGroup2,	(ULWord)kK2RegMaskDuallinkOutInputSelect,			kK2RegShiftDuallinkOutInputSelect},				//	0B NTV2_XptDuallinkOut1
	{0, 0, 0},																													//	0C ! NTV2_XptAlphOut
	{0, 0, 0},																													//	0D
	{kRegXptSelectGroup1,	(ULWord)kK2RegMaskColorSpaceConverterInputSelect,	kK2RegShiftColorSpaceConverterInputSelect},		//	0E NTV2_XptCSC1KeyYUV
	{XPT_FB_YUV_2,			0,											0},														//	0F NTV2_XptFrameBuffer2YUV

	{kRegXptSelectGroup5,	(ULWord)kK2RegMaskCSC2VidInputSelect,				kK2RegShiftCSC2VidInputSelect},					//	10 NTV2_XptCSC2VidYUV
	{kRegXptSelectGroup5,	(ULWord)kK2RegMaskCSC2VidInputSelect,				kK2RegShiftCSC2VidInputSelect},					//	11 NTV2_XptCSC2KeyYUV
	{kRegXptSelectGroup4,	(ULWord)kK2RegMaskMixerFGVidInputSelect,			kK2RegShiftMixerFGVidInputSelect},				//	12 NTV2_XptMixer1VidYUV
	{kRegXptSelectGroup4,	(ULWord)kK2RegMaskMixerFGKeyInputSelect,			kK2RegShiftMixerFGKeyInputSelect},				//	13 NTV2_XptMixer1KeyYUV
	{kRegXptSelectGroup36,	(ULWord)kK2RegMaskMultiLinkOutInputSelect,			kK2RegShiftMultiLinkOutInputSelect},			//	14 NTV2_XptMultiLinkOut1DS1
	{kRegXptSelectGroup36,	(ULWord)kK2RegMaskMultiLinkOutInputSelect,			kK2RegShiftMultiLinkOutInputSelect},			//	15 NTV2_XptMultiLinkOut1DS2
	{XPT_ANALOG_IN,			0,											0},														//	16 NTV2_XptAnalogIn
	{XPT_HDMI_IN,			0,											0},														//	17 NTV2_XptHDMIIn1
	{kRegXptSelectGroup36,	(ULWord)kK2RegMaskMultiLinkOutInputSelect,			kK2RegShiftMultiLinkOutInputSelect},			//	18 NTV2_XptMultiLinkOut1DS3
	{kRegXptSelectGroup36,	(ULWord)kK2RegMaskMultiLinkOutInputSelect,			kK2RegShiftMultiLinkOutInputSelect},			//	19 NTV2_XptMultiLinkOut1DS4
	{kRegXptSelectGroup37,	(ULWord)kK2RegMaskMultiLinkOutInputSelect,			kK2RegShiftMultiLinkOutInputSelect},			//	1A NTV2_XptMultiLinkOut2DS1
	{kRegXptSelectGroup37,	(ULWord)kK2RegMaskMultiLinkOutInputSelect,			kK2RegShiftMultiLinkOutInputSelect},			//	1B NTV2_XptMultiLinkOut2DS2
	{kRegXptSelectGroup7,	(ULWord)kK2RegMaskDuallinkOut2InputSelect,			kK2RegShiftDuallinkOut2InputSelect},			//	1C NTV2_XptDuallinkOut2
	{0, 0, 0},																													//	1D ! NTV2_XptTestPatternYUV
	{XPT_SDI_IN_1_DS2,		0,											0},														//	1E NTV2_XptSDIIn1DS2
	{XPT_SDI_IN_2_DS2,		0,											0},														//	1F NTV2_XptSDIIn2DS2

	{kRegXptSelectGroup9,	(ULWord)kK2RegMaskMixer2FGVidInputSelect,			kK2RegShiftMixer2FGVidInputSelect},				//	20 NTV2_XptMixer2VidYUV
	{kRegXptSelectGroup9,	(ULWord)kK2RegMaskMixer2FGKeyInputSelect,			kK2RegShiftMixer2FGKeyInputSelect},				//	21 NTV2_XptMixer2KeyYUV
	{0, 0, 0},																													//	22 ! NTV2_XptDCIMixerVidYUV
	{0, 0, 0},																													//	23 ! NTV2_XptStereoCompressorOut
	{XPT_FB_YUV_3,			0,											0},														//	24 NTV2_XptFrameBuffer3YUV
	{XPT_FB_YUV_4,			0,											0},														//	25 NTV2_XptFrameBuffer4YUV
	{kRegXptSelectGroup2,	(ULWord)kK2RegMaskDuallinkOutInputSelect,			kK2RegShiftDuallinkOutInputSelect},				//	26 NTV2_XptDuallinkOut1DS2
	{kRegXptSelectGroup7,	(ULWord)kK2RegMaskDuallinkOut2InputSelect,			kK2RegShiftDuallinkOut2InputSelect},			//	27 NTV2_XptDuallinkOut2DS2
	{0, 0, 0},																													//	28
	{0, 0, 0},																													//	29
	{0, 0, 0},																													//	2A
	{0, 0, 0},																													//	2B
	{kRegXptSelectGroup18,	(ULWord)kK2RegMaskCSC5VidInputSelect,				kK2RegShiftCSC5VidInputSelect},					//	2C NTV2_XptCSC5VidYUV
	{kRegXptSelectGroup18,	(ULWord)kK2RegMaskCSC5VidInputSelect,				kK2RegShiftCSC5VidInputSelect},					//	2D NTV2_XptCSC5KeyYUV
	{kRegXptSelectGroup37,	(ULWord)kK2RegMaskMultiLinkOutInputSelect,			kK2RegShiftMultiLinkOutInputSelect},			//	2E NTV2_XptMultiLinkOut2DS3
	{kRegXptSelectGroup37,	(ULWord)kK2RegMaskMultiLinkOutInputSelect,			kK2RegShiftMultiLinkOutInputSelect},			//	2F NTV2_XptMultiLinkOut2DS4

	{XPT_SDI_IN_3,			0,											0},														//	30 NTV2_XptSDIIn3
	{XPT_SDI_IN_4,			0,											0},														//	31 NTV2_XptSDIIn4
	{XPT_SDI_IN_3_DS2,		0,											0},														//	32 NTV2_XptSDIIn3DS2
	{XPT_SDI_IN_4_DS2,		0,											0},														//	33 NTV2_XptSDIIn4DS2
	{0, 0, 0},																													//	34
	{0, 0, 0},																													//	35
	{kRegXptSelectGroup16,	(ULWord)kK2RegMaskDuallinkOut3InputSelect,			kK2RegShiftDuallinkOut3InputSelect},			//	36 NTV2_XptDuallinkOut3
	{kRegXptSelectGroup16,	(ULWord)kK2RegMaskDuallinkOut3InputSelect,			kK2RegShiftDuallinkOut3InputSelect},			//	37 NTV2_XptDuallinkOut3DS2
	{kRegXptSelectGroup16,	(ULWord)kK2RegMaskDuallinkOut4InputSelect,			kK2RegShiftDuallinkOut4InputSelect},			//	38 NTV2_XptDuallinkOut4
	{kRegXptSelectGroup16,	(ULWord)kK2RegMaskDuallinkOut4InputSelect,			kK2RegShiftDuallinkOut4InputSelect},			//	39 NTV2_XptDuallinkOut4DS2
	{kRegXptSelectGroup17,	(ULWord)kK2RegMaskCSC3VidInputSelect,				kK2RegShiftCSC3VidInputSelect},					//	3A NTV2_XptCSC3VidYUV
	{kRegXptSelectGroup17,	(ULWord)kK2RegMaskCSC3VidInputSelect,				kK2RegShiftCSC3VidInputSelect},					//	3B NTV2_XptCSC3KeyYUV
	{kRegXptSelectGroup17,	(ULWord)kK2RegMaskCSC4VidInputSelect,				kK2RegShiftCSC4VidInputSelect},					//	3C NTV2_XptCSC4VidYUV
	{kRegXptSelectGroup17,	(ULWord)kK2RegMaskCSC4VidInputSelect,				kK2RegShiftCSC4VidInputSelect},					//	3D NTV2_XptCSC4KeyYUV
	{kRegXptSelectGroup16,	(ULWord)kK2RegMaskDuallinkOut5InputSelect,			kK2RegShiftDuallinkOut5InputSelect},			//	3E NTV2_XptDuallinkOut5
	{kRegXptSelectGroup16,	(ULWord)kK2RegMaskDuallinkOut5InputSelect,			kK2RegShiftDuallinkOut5InputSelect},			//	3F NTV2_XptDuallinkOut5DS2

	{kRegXptSelectGroup12,	(ULWord)kK2RegMaskXpt3DLUT1InputSelect,				kK2RegShiftXpt3DLUT1InputSelect},				//	40
	{XPT_HDMI_IN_Q2,			0,											0},													//	41 NTV2_XptHDMIIn1Q2
	{XPT_HDMI_IN_Q3,			0,											0},													//	42 NTV2_XptHDMIIn1Q3
	{XPT_HDMI_IN_Q4,			0,											0},													//	43 NTV2_XptHDMIIn1Q4
	{kRegXptSelectGroup19,	(ULWord)kK2RegMask4KDCQ1InputSelect,				kK2RegShift4KDCQ1InputSelect},					//	44 NTV2_Xpt4KDownConverterOut
	{XPT_SDI_IN_5,			0,											0},														//	45 NTV2_XptSDIIn5
	{XPT_SDI_IN_6,			0,											0},														//	46 NTV2_XptSDIIn6
	{XPT_SDI_IN_5_DS2,		0,											0},														//	47 NTV2_XptSDIIn5DS2
	{XPT_SDI_IN_6_DS2,		0,											0},														//	48 NTV2_XptSDIIn6DS2
	{XPT_SDI_IN_7,			0,											0},														//	49 NTV2_XptSDIIn7
	{XPT_SDI_IN_8,			0,											0},														//	4A NTV2_XptSDIIn8
	{XPT_SDI_IN_7_DS2,		0,											0},														//	4B NTV2_XptSDIIn7DS2
	{XPT_SDI_IN_8_DS2,		0,											0},														//	4C NTV2_XptSDIIn8DS2
	{0, 0, 0},																													//	4D
	{0, 0, 0},																													//	4E
	{0, 0, 0},																													//	4F

	{0, 0, 0},																													//	50
	{XPT_FB_YUV_5,			0,											0},														//	51 NTV2_XptFrameBuffer5YUV
	{XPT_FB_YUV_6,			0,											0},														//	52 NTV2_XptFrameBuffer6YUV
	{XPT_FB_YUV_7,			0,											0},														//	53 NTV2_XptFrameBuffer7YUV
	{XPT_FB_YUV_8,			0,											0},														//	54 NTV2_XptFrameBuffer8YUV
	{kRegXptSelectGroup28,	(ULWord)kK2RegMaskMixer3FGVidInputSelect,			kK2RegShiftMixer3FGVidInputSelect},				//	55 NTV2_XptMixer3VidYUV
	{kRegXptSelectGroup28,	(ULWord)kK2RegMaskMixer3FGKeyInputSelect,			kK2RegShiftMixer3FGKeyInputSelect},				//	56 NTV2_XptMixer3KeyYUV
	{kRegXptSelectGroup29,	(ULWord)kK2RegMaskMixer4FGVidInputSelect,			kK2RegShiftMixer4FGVidInputSelect},				//	57 NTV2_XptMixer4VidYUV
	{kRegXptSelectGroup29,	(ULWord)kK2RegMaskMixer4FGKeyInputSelect,			kK2RegShiftMixer4FGKeyInputSelect},				//	58 NTV2_XptMixer4KeyYUV
	{kRegXptSelectGroup30,	(ULWord)kK2RegMaskCSC6VidInputSelect,				kK2RegShiftCSC6VidInputSelect},					//	59 NTV2_XptCSC6VidYUV
	{kRegXptSelectGroup30,	(ULWord)kK2RegMaskCSC6VidInputSelect,				kK2RegShiftCSC6VidInputSelect},					//	5A NTV2_XptCSC6KeyYUV
	{kRegXptSelectGroup23,	(ULWord)kK2RegMaskCSC7VidInputSelect,				kK2RegShiftCSC7VidInputSelect},					//	5B NTV2_XptCSC7VidYUV
	{kRegXptSelectGroup23,	(ULWord)kK2RegMaskCSC7VidInputSelect,				kK2RegShiftCSC7VidInputSelect},					//	5C NTV2_XptCSC7KeyYUV
	{kRegXptSelectGroup23,	(ULWord)kK2RegMaskCSC8VidInputSelect,				kK2RegShiftCSC8VidInputSelect},					//	5D NTV2_XptCSC8VidYUV
	{kRegXptSelectGroup23,	(ULWord)kK2RegMaskCSC8VidInputSelect,				kK2RegShiftCSC8VidInputSelect},					//	5E NTV2_XptCSC8KeyYUV
	{0, 0, 0},																													//	5F

	{0, 0, 0},																													//	60
	{0, 0, 0},																													//	61
	{kRegXptSelectGroup27,	(ULWord)kK2RegMaskDuallinkOut6InputSelect,			kK2RegShiftDuallinkOut6InputSelect},			//	62 NTV2_XptDuallinkOut6
	{kRegXptSelectGroup27,	(ULWord)kK2RegMaskDuallinkOut6InputSelect,			kK2RegShiftDuallinkOut6InputSelect},			//	63 NTV2_XptDuallinkOut6DS2
	{kRegXptSelectGroup27,	(ULWord)kK2RegMaskDuallinkOut7InputSelect,			kK2RegShiftDuallinkOut7InputSelect},			//	64 NTV2_XptDuallinkOut7
	{kRegXptSelectGroup27,	(ULWord)kK2RegMaskDuallinkOut7InputSelect,			kK2RegShiftDuallinkOut7InputSelect},			//	65 NTV2_XptDuallinkOut7DS2
	{kRegXptSelectGroup27,	(ULWord)kK2RegMaskDuallinkOut8InputSelect,			kK2RegShiftDuallinkOut8InputSelect},			//	66 NTV2_XptDuallinkOut8
	{kRegXptSelectGroup27,	(ULWord)kK2RegMaskDuallinkOut8InputSelect,			kK2RegShiftDuallinkOut8InputSelect},			//	67 NTV2_XptDuallinkOut8DS2
	{kRegXptSelectGroup32,	(ULWord)kK2RegMask425Mux1AInputSelect,				kK2RegShift425Mux1AInputSelect},				//	68 NTV2_Xpt425Mux1AYUV
	{kRegXptSelectGroup32,	(ULWord)kK2RegMask425Mux1BInputSelect,				kK2RegShift425Mux1BInputSelect},				//	69 NTV2_Xpt425Mux1BYUV
	{kRegXptSelectGroup32,	(ULWord)kK2RegMask425Mux2AInputSelect,				kK2RegShift425Mux2AInputSelect},				//	6A NTV2_Xpt425Mux2AYUV
	{kRegXptSelectGroup32,	(ULWord)kK2RegMask425Mux2BInputSelect,				kK2RegShift425Mux2BInputSelect},				//	6B NTV2_Xpt425Mux2BYUV
	{kRegXptSelectGroup33,	(ULWord)kK2RegMask425Mux3AInputSelect,				kK2RegShift425Mux3AInputSelect},				//	6C NTV2_Xpt425Mux3AYUV
	{kRegXptSelectGroup33,	(ULWord)kK2RegMask425Mux3BInputSelect,				kK2RegShift425Mux3BInputSelect},				//	6D NTV2_Xpt425Mux3BYUV
	{kRegXptSelectGroup33,	(ULWord)kK2RegMask425Mux4AInputSelect,				kK2RegShift425Mux4AInputSelect},				//	6E NTV2_Xpt425Mux4AYUV
	{kRegXptSelectGroup33,	(ULWord)kK2RegMask425Mux4BInputSelect,				kK2RegShift425Mux4BInputSelect},				//	6F NTV2_Xpt425Mux4BYUV

	{XPT_FB_425_YUV_1,		0,											0},														//	70 NTV2_XptFrameBuffer1_DS2YUV
	{XPT_FB_425_YUV_2,		0,											0},														//	71 NTV2_XptFrameBuffer2_DS2YUV
	{XPT_FB_425_YUV_3,		0,											0},														//	72 NTV2_XptFrameBuffer3_DS2YUV
	{XPT_FB_425_YUV_4,		0,											0},														//	73 NTV2_XptFrameBuffer4_DS2YUV
	{XPT_FB_425_YUV_5,		0,											0},														//	74 NTV2_XptFrameBuffer5_DS2YUV
	{XPT_FB_425_YUV_6,		0,											0},														//	75 NTV2_XptFrameBuffer6_DS2YUV
	{XPT_FB_425_YUV_7,		0,											0},														//	76 NTV2_XptFrameBuffer7_DS2YUV
	{XPT_FB_425_YUV_8,		0,											0},														//	77 NTV2_XptFrameBuffer8_DS2YUV
	{0, 0, 0},																													//	78
	{0, 0, 0},																													//	79
	{0, 0, 0},																													//	7A
	{0, 0, 0},																													//	7B
	{0, 0, 0},																													//	7C
	{0, 0, 0},																													//	7D
	{0, 0, 0},																													//	7E
	{0, 0, 0},																													//	7F

	{0, 0, 0},																													//	80
	{0, 0, 0},																													//	81
	{0, 0, 0},																													//	82
	{kRegXptSelectGroup11,	(ULWord)kK2RegMaskDuallinkIn1InputSelect,			kK2RegShiftDuallinkIn1InputSelect},				//	83 NTV2_XptDuallinkIn1
	{kRegXptSelectGroup1,	(ULWord)kK2RegMaskXptLUTInputSelect,				kK2RegShiftXptLUTInputSelect},					//	84 NTV2_XptLUT1Out
	{kRegXptSelectGroup1,	(ULWord)kK2RegMaskColorSpaceConverterInputSelect,	kK2RegShiftColorSpaceConverterInputSelect},		//	85 NTV2_XptCSC1VidRGB
	{0, 0, 0},																													//	86
	{0, 0, 0},																													//	87
	{XPT_FB_RGB_1,			0,											0},														//	88 NTV2_XptFrameBuffer1RGB
	{0, 0, 0},																													//	89 ! NTV2_XptFrameSync1RGB
	{0, 0, 0},																													//	8A ! NTV2_XptFrameSync2RGB
	{0, 0, 0},																													//	8B
	{0, 0, 0},																													//	8C
	{kRegXptSelectGroup5,	(ULWord)kK2RegMaskXptLUT2InputSelect,				kK2RegShiftXptLUT2InputSelect},					//	8D NTV2_XptLUT2Out
	{0, 0, 0},																													//	8E
	{XPT_FB_RGB_2,			0,											0},														//	8F NTV2_XptFrameBuffer2RGB

	{kRegXptSelectGroup5,	(ULWord)kK2RegMaskCSC2VidInputSelect,				kK2RegShiftCSC2VidInputSelect},					//	90 NTV2_XptCSC2VidRGB
	{0, 0, 0},																													//	91
	{kRegXptSelectGroup4,	(ULWord)kK2RegMaskMixerFGVidInputSelect,			kK2RegShiftMixerFGVidInputSelect},				//	12 NTV2_XptMixer1VidRGB
	{0, 0, 0},																													//	93
	{0, 0, 0},																													//	94 ! NTV2_XptWaterMarkerRGB
	{0, 0, 0},																													//	95 ! NTV2_XptIICTRGB
	{0, 0, 0},																													//	96
	{XPT_HDMI_IN,			0,											0},														//	97 NTV2_XptHDMIIn1RGB
	{0, 0, 0},																													//	98
	{0, 0, 0},																													//	99
	{0, 0, 0},																													//	9A
	{0, 0, 0},																													//	9B ! NTV2_XptIICT2RGB
	{0, 0, 0},																													//	9C
	{0, 0, 0},																													//	9D
	{0, 0, 0},																													//	9E
	{0, 0, 0},																													//	9F

	{0, 0, 0},																													//	A0
	{0, 0, 0},																													//	A1
	{0, 0, 0},																													//	A2 ! NTV2_XptDCIMixerVidRGB
	{0, 0, 0},																													//	A3
	{XPT_FB_RGB_3,			0,											0},														//	A4 NTV2_XptFrameBuffer3RGB
	{XPT_FB_RGB_4,			0,											0},														//	A5 NTV2_XptFrameBuffer4RGB
	{0, 0, 0},																													//	A6
	{0, 0, 0},																													//	A7
	{kRegXptSelectGroup11,	(ULWord)kK2RegMaskDuallinkIn2InputSelect,			kK2RegShiftDuallinkIn2InputSelect},				//	A8 NTV2_XptDuallinkIn2
	{kRegXptSelectGroup12,	(ULWord)kK2RegMaskXptLUT3InputSelect,				kK2RegShiftXptLUT3InputSelect},					//	A9 NTV2_XptLUT3Out
	{kRegXptSelectGroup12,	(ULWord)kK2RegMaskXptLUT4InputSelect,				kK2RegShiftXptLUT4InputSelect},					//	AA NTV2_XptLUT4Out
	{kRegXptSelectGroup12,	(ULWord)kK2RegMaskXptLUT5InputSelect,				kK2RegShiftXptLUT5InputSelect},					//	AB NTV2_XptLUT5Out
	{kRegXptSelectGroup18,	(ULWord)kK2RegMaskCSC5VidInputSelect,				kK2RegShiftCSC5VidInputSelect},					//	AC NTV2_XptCSC5VidRGB
	{0, 0, 0},																													//	AD
	{0, 0, 0},																													//	AE
	{0, 0, 0},																													//	AF

	{0, 0, 0},																													//	B0
	{0, 0, 0},																													//	B1
	{0, 0, 0},																													//	B2
	{0, 0, 0},																													//	B3
	{kRegXptSelectGroup15,	(ULWord)kK2RegMaskDuallinkIn3InputSelect,			kK2RegShiftDuallinkIn3InputSelect},				//	B4 NTV2_XptDuallinkIn3
	{kRegXptSelectGroup15,	(ULWord)kK2RegMaskDuallinkIn4InputSelect,			kK2RegShiftDuallinkIn4InputSelect},				//	B5 NTV2_XptDuallinkIn4
	{0, 0, 0},																													//	B6
	{0, 0, 0},																													//	B7
	{0, 0, 0},																													//	B8
	{0, 0, 0},																													//	B9
	{kRegXptSelectGroup17,	(ULWord)kK2RegMaskCSC3VidInputSelect,				kK2RegShiftCSC3VidInputSelect},					//	BA NTV2_XptCSC3VidRGB
	{0, 0, 0},																													//	BB
	{kRegXptSelectGroup17,	(ULWord)kK2RegMaskCSC4VidInputSelect,				kK2RegShiftCSC4VidInputSelect},					//	BC NTV2_XptCSC4VidRGB
	{0, 0, 0},																													//	BD
	{0, 0, 0},																													//	BE
	{0, 0, 0},																													//	BF

	{kRegXptSelectGroup12,	(ULWord)kK2RegMaskXpt3DLUT1InputSelect,				kK2RegShiftXpt3DLUT1InputSelect},				//	C0
	{XPT_HDMI_IN_Q2,			0,											0},													//	C1 NTV2_XptHDMIIn1Q2RGB
	{XPT_HDMI_IN_Q3,			0,											0},													//	C2 NTV2_XptHDMIIn1Q3RGB
	{XPT_HDMI_IN_Q4,			0,											0},													//	C3 NTV2_XptHDMIIn1Q4RGB
	{kRegXptSelectGroup19,	(ULWord)kK2RegMask4KDCQ1InputSelect,				kK2RegShift4KDCQ1InputSelect},					//	C4 NTV2_Xpt4KDownConverterOutRGB
	{0, 0, 0},																													//	C5
	{0, 0, 0},																													//	C6
	{0, 0, 0},																													//	C7
	{0, 0, 0},																													//	C8
	{0, 0, 0},																													//	C9
	{0, 0, 0},																													//	CA
	{0, 0, 0},																													//	CB
	{0, 0, 0},																													//	CC
	{kRegXptSelectGroup25,	(ULWord)kK2RegMaskDuallinkIn5InputSelect,			kK2RegShiftDuallinkIn5InputSelect},				//	CD NTV2_XptDuallinkIn5
	{kRegXptSelectGroup25,	(ULWord)kK2RegMaskDuallinkIn6InputSelect,			kK2RegShiftDuallinkIn6InputSelect},				//	CE NTV2_XptDuallinkIn6
	{kRegXptSelectGroup26,	(ULWord)kK2RegMaskDuallinkIn7InputSelect,			kK2RegShiftDuallinkIn7InputSelect},				//	CF NTV2_XptDuallinkIn7

	{kRegXptSelectGroup26,	(ULWord)kK2RegMaskDuallinkIn8InputSelect,			kK2RegShiftDuallinkIn8InputSelect},				//	D0 NTV2_XptDuallinkIn8
	{XPT_FB_RGB_5,			0,											0},														//	D1 NTV2_XptFrameBuffer5RGB
	{XPT_FB_RGB_6,			0,											0},														//	D2 NTV2_XptFrameBuffer6RGB
	{XPT_FB_RGB_7,			0,											0},														//	D3 NTV2_XptFrameBuffer7RGB
	{XPT_FB_RGB_8,			0,											0},														//	D4 NTV2_XptFrameBuffer8RGB
	{0, 0, 0},																													//	D5
	{0, 0, 0},																													//	D6
	{0, 0, 0},																													//	D7
	{0, 0, 0},																													//	D8
	{kRegXptSelectGroup30,	(ULWord)kK2RegMaskCSC6VidInputSelect,				kK2RegShiftCSC6VidInputSelect},					//	D9 NTV2_XptCSC6VidRGB
	{0, 0, 0},																													//	DA
	{kRegXptSelectGroup23,	(ULWord)kK2RegMaskCSC7VidInputSelect,				kK2RegShiftCSC7VidInputSelect},					//	DB NTV2_XptCSC7VidRGB
	{0, 0, 0},																													//	DC
	{kRegXptSelectGroup23,	(ULWord)kK2RegMaskCSC8VidInputSelect,				kK2RegShiftCSC8VidInputSelect},					//	DD NTV2_XptCSC8VidRGB
	{0, 0, 0},																													//	DE
	{kRegXptSelectGroup24,	(ULWord)kK2RegMaskXptLUT6InputSelect,				kK2RegShiftXptLUT6InputSelect},					//	DF NTV2_XptLUT6Out

	{kRegXptSelectGroup24,	(ULWord)kK2RegMaskXptLUT7InputSelect,				kK2RegShiftXptLUT7InputSelect},					//	E0 NTV2_XptLUT7Out
	{kRegXptSelectGroup24,	(ULWord)kK2RegMaskXptLUT8InputSelect,				kK2RegShiftXptLUT8InputSelect},					//	E1 NTV2_XptLUT8Out
	{0, 0, 0},																													//	E2
	{0, 0, 0},																													//	E3
	{0, 0, 0},																													//	E4
	{0, 0, 0},																													//	E5
	{0, 0, 0},																													//	E6
	{0, 0, 0},																													//	E7
	{kRegXptSelectGroup32,	(ULWord)kK2RegMask425Mux1AInputSelect,				kK2RegShift425Mux1AInputSelect},				//	E8 NTV2_Xpt425Mux1ARGB
	{kRegXptSelectGroup32,	(ULWord)kK2RegMask425Mux1BInputSelect,				kK2RegShift425Mux1BInputSelect},				//	E9 NTV2_Xpt425Mux1BRGB
	{kRegXptSelectGroup32,	(ULWord)kK2RegMask425Mux2AInputSelect,				kK2RegShift425Mux2AInputSelect},				//	EA NTV2_Xpt425Mux2ARGB
	{kRegXptSelectGroup32,	(ULWord)kK2RegMask425Mux2BInputSelect,				kK2RegShift425Mux2BInputSelect},				//	EB NTV2_Xpt425Mux2BRGB
	{kRegXptSelectGroup33,	(ULWord)kK2RegMask425Mux3AInputSelect,				kK2RegShift425Mux3AInputSelect},				//	EC NTV2_Xpt425Mux3ARGB
	{kRegXptSelectGroup33,	(ULWord)kK2RegMask425Mux3BInputSelect,				kK2RegShift425Mux3BInputSelect},				//	ED NTV2_Xpt425Mux3BRGB
	{kRegXptSelectGroup33,	(ULWord)kK2RegMask425Mux4AInputSelect,				kK2RegShift425Mux4AInputSelect},				//	EE NTV2_Xpt425Mux4ARGB
	{kRegXptSelectGroup33,	(ULWord)kK2RegMask425Mux4BInputSelect,				kK2RegShift425Mux4BInputSelect},				//	EF NTV2_Xpt425Mux4BRGB

	{XPT_FB_425_RGB_1,		0,											0},														//	F0 NTV2_XptFrameBuffer1_DS2RGB
	{XPT_FB_425_RGB_2,		0,											0},														//	F1 NTV2_XptFrameBuffer2_DS2RGB
	{XPT_FB_425_RGB_3,		0,											0},														//	F2 NTV2_XptFrameBuffer3_DS2RGB
	{XPT_FB_425_RGB_4,		0,											0},														//	F3 NTV2_XptFrameBuffer4_DS2RGB
	{XPT_FB_425_RGB_5,		0,											0},														//	F4 NTV2_XptFrameBuffer5_DS2RGB
	{XPT_FB_425_RGB_6,		0,											0},														//	F5 NTV2_XptFrameBuffer6_DS2RGB
	{XPT_FB_425_RGB_7,		0,											0},														//	F6 NTV2_XptFrameBuffer7_DS2RGB
	{XPT_FB_425_RGB_8,		0,											0},														//	F7 NTV2_XptFrameBuffer8_DS2RGB
	{0, 0, 0},																													//	F8
	{0, 0, 0},																													//	F9
	{0, 0, 0},																													//	FA
	{0, 0, 0},																													//	FB
	{0, 0, 0},																													//	FC
	{0, 0, 0},																													//	FD
	{0, 0, 0},																													//	FE
	{0, 0, 0},																													//	FF
	{kRegXptSelectGroup11,	(ULWord)kK2RegMaskDuallinkIn1DSInputSelect,					kK2RegShiftDuallinkIn1DSInputSelect},	//	100 NTV2_XptDuallinkIn1 Used for passthru
	{kRegXptSelectGroup11,	(ULWord)kK2RegMaskDuallinkIn2DSInputSelect,					kK2RegShiftDuallinkIn2DSInputSelect},	//	101 NTV2_XptDuallinkIn2 Used for passthru
	{kRegXptSelectGroup15,	(ULWord)kK2RegMaskDuallinkIn3DSInputSelect,					kK2RegShiftDuallinkIn3DSInputSelect},	//	102 NTV2_XptDuallinkIn3 Used for passthru
	{kRegXptSelectGroup15,	(ULWord)kK2RegMaskDuallinkIn4DSInputSelect,					kK2RegShiftDuallinkIn4DSInputSelect},	//	103 NTV2_XptDuallinkIn4 Used for passthru
	{kRegXptSelectGroup25,	(ULWord)kK2RegMaskDuallinkIn5DSInputSelect,					kK2RegShiftDuallinkIn5DSInputSelect},	//	104 NTV2_XptDuallinkIn5 Used for passthru
	{kRegXptSelectGroup25,	(ULWord)kK2RegMaskDuallinkIn6DSInputSelect,					kK2RegShiftDuallinkIn6DSInputSelect},	//	105 NTV2_XptDuallinkIn6 Used for passthru
	{kRegXptSelectGroup26,	(ULWord)kK2RegMaskDuallinkIn7DSInputSelect,					kK2RegShiftDuallinkIn7DSInputSelect},	//	106 NTV2_XptDuallinkIn7 Used for passthru
	{kRegXptSelectGroup26,	(ULWord)kK2RegMaskDuallinkIn8DSInputSelect,					kK2RegShiftDuallinkIn8DSInputSelect}	//	107 NTV2_XptDuallinkIn8 Used for passthru
};


////////////////////////////////////////////////////////////////////////
//xpt routines
bool FindSDIOutputSource(Ntv2SystemContext* context, NTV2OutputXptID* source, NTV2Channel channel)
{
	NTV2OutputXptID xptSelect = NTV2_XptBlack;
	GetXptSDIOutInputSelect(context, channel, &xptSelect);
	if(xptSelect == NTV2_XptMultiLinkOut1DS1 ||
		xptSelect == NTV2_XptMultiLinkOut1DS2 ||
		xptSelect == NTV2_XptMultiLinkOut1DS3 ||
		xptSelect == NTV2_XptMultiLinkOut1DS4)
	{
		//Noop but the caller wants this xptSelect for configuration
	}
	else if(xptSelect != NTV2_XptConversionModule)
	{
		if(!FindCrosspointSource(context, &xptSelect, xptSelect))
		{
			return false;
		}
	}

	if(source != NULL)
	{
		*source = xptSelect;
	}

	return true;
}

bool FindAnalogOutputSource(Ntv2SystemContext* context, NTV2OutputXptID* source)
{
	NTV2OutputXptID xptSelect = NTV2_XptBlack;

	GetXptAnalogOutInputSelect(context, &xptSelect);

	if(xptSelect != NTV2_XptConversionModule)
	{
		if(!FindCrosspointSource(context, &xptSelect, xptSelect))
		{
			return false;
		}
	}

	if(source != NULL)
	{
		*source = xptSelect;
	}

	return true;
}

bool FindHDMIOutputSource(Ntv2SystemContext* context, NTV2OutputXptID* source, NTV2Channel channel)
{
	NTV2OutputXptID xptSelect = NTV2_XptBlack;

	switch (channel)
	{
	default:
	case NTV2_CHANNEL1:
		GetXptHDMIOutInputSelect(context, &xptSelect);
		break;
	case NTV2_CHANNEL2:
		GetXptHDMIOutQ2InputSelect(context, &xptSelect);
		break;
	case NTV2_CHANNEL3:
		GetXptHDMIOutQ3InputSelect(context, &xptSelect);
		break;
	case NTV2_CHANNEL4:
		GetXptHDMIOutQ4InputSelect(context, &xptSelect);
		break;
	}

	if(xptSelect != NTV2_XptConversionModule)
	{
		if(!FindCrosspointSource(context, &xptSelect, xptSelect))
		{
			return false;
		}
	}

	if(source != NULL)
	{
		*source = xptSelect;
	}

	return true;
}

bool FindCrosspointSource(Ntv2SystemContext* context, NTV2OutputXptID* source, NTV2OutputXptID crosspoint)
{
	bool				sourceFound = false;
	NTV2DeviceID		deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	int					loopCount = 0;
	NTV2OutputXptID		currentXpt = crosspoint;
	const int			kMaxLoopCount = 10;

	if(!NTV2DeviceNeedsRoutingSetup(deviceID))
	{
		return false;
	}

	for (loopCount = 0; currentXpt != NTV2_XptBlack && loopCount < kMaxLoopCount; loopCount++)
	{	
		NTV2XptLookupEntry src = GetCrosspointIDInput(currentXpt);

		switch(currentXpt)
		{
		case NTV2_XptSDIIn1:
		case NTV2_XptSDIIn2:
		case NTV2_XptSDIIn3:
		case NTV2_XptSDIIn4:
		case NTV2_XptSDIIn5:
		case NTV2_XptSDIIn6:
		case NTV2_XptSDIIn7:
		case NTV2_XptSDIIn8:
		case NTV2_XptSDIIn1DS2:
		case NTV2_XptSDIIn2DS2:
		case NTV2_XptSDIIn3DS2:
		case NTV2_XptSDIIn4DS2:
		case NTV2_XptSDIIn5DS2:
		case NTV2_XptSDIIn6DS2:
		case NTV2_XptSDIIn7DS2:
		case NTV2_XptSDIIn8DS2:
		case NTV2_XptDuallinkIn1:
		case NTV2_XptDuallinkIn2:
		case NTV2_XptDuallinkIn3:
		case NTV2_XptDuallinkIn4:
		case NTV2_XptDuallinkIn5:
		case NTV2_XptDuallinkIn6:
		case NTV2_XptDuallinkIn7:
		case NTV2_XptDuallinkIn8:
		case NTV2_XptAnalogIn:
		case NTV2_XptHDMIIn1:
		case NTV2_XptHDMIIn1Q2:
		case NTV2_XptHDMIIn1Q3:
		case NTV2_XptHDMIIn1Q4:
		case NTV2_XptHDMIIn1RGB:
		case NTV2_XptHDMIIn1Q2RGB:
		case NTV2_XptHDMIIn1Q3RGB:
		case NTV2_XptHDMIIn1Q4RGB:
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
		case NTV2_XptBlack:
		case NTV2_XptConversionModule:
		case NTV2_Xpt4KDownConverterOut:
		case NTV2_Xpt4KDownConverterOutRGB:
			sourceFound = true;
			break;
		default:
			break;
		}

		if(sourceFound)
			break;

		ntv2ReadRegisterMS(
			context,
			src.registerNumber,
			(ULWord*)&currentXpt,
			src.registerMask,
			src.registerShift);

	}

	if(!sourceFound)
	{
		return false;
	}

	if(source != NULL)
	{
		*source = currentXpt;
	}

	return true;
}

NTV2XptLookupEntry GetCrosspointIDInput(NTV2OutputXptID outputXpt)
{
	if (outputXpt >= 0 && outputXpt <= 0x107)
		return NTV2XptBackTraceTable[outputXpt];
	return NTV2XptBackTraceTable[0];
}

bool GetXptSDIOutInputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, gChannelToSDIOutInSelectRegNum[channel], (ULWord*)value, gChannelToSDIOutInSelectMask[channel], gChannelToSDIOutInSelectShift[channel]);
}

bool GetXptSDIOutDS2InputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, gChannelToSDIOutDS2InSelectRegNum[channel], (ULWord*)value, gChannelToSDIOutDS2InSelectMask[channel], gChannelToSDIOutDS2InSelectShift[channel]);
}

bool SetXptSDIOutInputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID value)
{
	return ntv2WriteRegisterMS(context, gChannelToSDIOutInSelectRegNum[channel], (ULWord)value, gChannelToSDIOutInSelectMask[channel], gChannelToSDIOutInSelectShift[channel]);
}

bool GetXptConversionModInputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, kRegXptSelectGroup1, (ULWord*)value, kK2RegMaskConversionModInputSelect, kK2RegShiftConversionModInputSelect);
}

bool GetXptDuallinkInInputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, gChannelToDLInInputSelectRegNum[channel], (ULWord*)value, (ULWord)gChannelToDLInInputSelectMask[channel], gChannelToDLInInputSelectShift[channel]);
}

bool GetXptAnalogOutInputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, kRegXptSelectGroup3, (ULWord*)value, kK2RegMaskAnalogOutInputSelect, kK2RegShiftAnalogOutInputSelect);
}

bool GetXptFrameBuffer1InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, kRegXptSelectGroup2, (ULWord*)value, kK2RegMaskFrameBuffer1InputSelect, kK2RegShiftFrameBuffer1InputSelect);
}

bool GetXptFrameBuffer2InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, kRegXptSelectGroup5, (ULWord*)value, kK2RegMaskFrameBuffer2InputSelect, kK2RegShiftFrameBuffer2InputSelect);
}

bool GetXptHDMIOutInputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, kRegXptSelectGroup6, (ULWord*)value, kK2RegMaskHDMIOutInputSelect, kK2RegShiftHDMIOutInputSelect);
}

bool GetXptHDMIOutQ2InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, kRegXptSelectGroup20, (ULWord*)value, kK2RegMaskHDMIOutV2Q2InputSelect, kK2RegShiftHDMIOutV2Q2InputSelect);
}

bool GetXptHDMIOutQ3InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, kRegXptSelectGroup20, (ULWord*)value, kK2RegMaskHDMIOutV2Q3InputSelect, kK2RegShiftHDMIOutV2Q3InputSelect);
}

bool GetXptHDMIOutQ4InputSelect(Ntv2SystemContext* context, NTV2OutputXptID* value)
{
	return ntv2ReadRegisterMS(context, kRegXptSelectGroup20, (ULWord*)value, (ULWord)kK2RegMaskHDMIOutV2Q4InputSelect, (ULWord)kK2RegShiftHDMIOutV2Q4InputSelect);
}

bool GetXptMultiLinkOutInputSelect(Ntv2SystemContext* context, NTV2Channel channel, NTV2OutputXptID* value)
{
	NTV2XptLookupEntry source = GetCrosspointIDInput(channel == NTV2_CHANNEL1 ? NTV2_XptMultiLinkOut1DS1 : NTV2_XptMultiLinkOut2DS1);
	return ntv2ReadRegisterMS(context, source.registerNumber, (ULWord*)value, (ULWord)source.registerMask, (ULWord)source.registerShift);
}

