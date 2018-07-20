/**
	@file	ntv2anc.cpp
	@brief	Implementations of anc-centric NTV2Card methods.
	@copyright	(C) 2004-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2formatdescriptor.h"
#include "ntv2utils.h"



/////////////////////////////////////////////////////////////////
/////////////	PER-SDI-SPIGOT REGISTER NUMBERS		/////////////
/////////////////////////////////////////////////////////////////

//								SDI Spigot:		   1	   2	   3	   4	   5	   6	   7	   8
static const ULWord	sAncInsBaseRegNum[]	=	{	4608,	4672,	4736,	4800,	4864,	4928,	4992,	5056	};
static const ULWord	sAncExtBaseRegNum[]	=	{	4096,	4160,	4224,	4288,	4352,	4416,	4480,	4544	};
static const ULWord	kNumDIDRegisters		(regAncExtIgnorePktsReg_Last - regAncExtIgnorePktsReg_First + 1);


static inline ULWord	AncInsRegNum(const UWord inSDIOutput, const ANCInsRegisters inReg)
{
	return sAncInsBaseRegNum[inSDIOutput] + inReg;
}


static inline ULWord	AncExtRegNum(const UWord inSDIInput, const ANCExtRegisters inReg)
{
	return sAncExtBaseRegNum[inSDIInput] + inReg;
}



/////////////////////////////////////////////////////////////////////
/////////////	PER-VIDEO-STANDARD INSERTER INIT SETUP	/////////////
/////////////////////////////////////////////////////////////////////

typedef struct ANCInserterInitParams
{
	uint32_t	field1ActiveLine;
	uint32_t	field2ActiveLine;
	uint32_t	hActivePixels;
	uint32_t	hTotalPixels;
	uint32_t	totalLines;
	uint32_t	fidLow;
	uint32_t	fidHigh;
} ANCInserterInitParams;

static const ANCInserterInitParams  inserterInitParamsTable [NTV2_NUM_STANDARDS] = {
//						F1		F2		Horz					
//						Active	Active	Active	Total	Total	FID		FID
//	Standard			Line	Line	Pixels	Pixels	Lines	Lo		Hi
/* 1080       */	{	21,		564,	1920,	2200,	1125,	1125,	563	},
/* 720        */	{	26,		0,		1280,	1280,	750,	0,		0	},
/* 525        */	{	21,		283,	720,	720,	525,	3,		265	},
/* 625        */	{	23,		336,	720,	864,	625,	625,	312	},
/* 1080p      */	{	42,		0,		1920,	2640,	1125,	0,		0	},
/* 2K         */	{	42,		0,		2048,	2640,	1125,	0,		0	},
/* 2Kx1080p   */	{	42,		0,		2048,	2640,	1125,	0,		0	},
/* 2Kx1080i   */	{	21,		564,	2048,	2200,	1125,	1125,	563	},
/* 3840x2160p */	{	42,		0,		1920,	2640,	1125,	0,		0	},
/* 4096x2160p */	{	42,		0,		2048,	2640,	1125,	0,		0	},
/* 3840HFR    */	{	42,		0,		1920,	2640,	1125,	0,		0	},
/* 4096HFR    */	{	42,		0,		2048,	2640,	1125,	0,		0	}
};



/////////////////////////////////////////////////////////////////////
/////////////	PER-VIDEO-STANDARD EXTRACTOR INIT SETUP	/////////////
/////////////////////////////////////////////////////////////////////

typedef struct ANCExtractorInitParams
{
	uint32_t	field1StartLine;
	uint32_t	field1CutoffLine;
	uint32_t	field2StartLine;
	uint32_t	field2CutoffLine;
	uint32_t	totalLines;
	uint32_t	fidLow;
	uint32_t	fidHigh;
	uint32_t	field1AnalogStartLine;
	uint32_t	field2AnalogStartLine;
	uint32_t	field1AnalogYFilter;
	uint32_t	field2AnalogYFilter;
	uint32_t	field1AnalogCFilter;
	uint32_t	field2AnalogCFilter;
	uint32_t	analogActiveLineLength;
}ANCExtractorInitParams;

const static ANCExtractorInitParams  extractorInitParamsTable [NTV2_NUM_STANDARDS] = {
//					F1		F1		F2		F2								F1Anlg	F2Anlg	F1			F2			F1		F2		Analog
//					Start	Cutoff	Start	Cutoff	Total	FID		FID		Start	Start	Anlg		Anlg		Anlg	Anlg	Active
// Standard			Line	Line	Line	Line	Lines	Low		High	Line	Line	Y Filt		Y Filt		C Filt	C Filt	LineLength
/* 1080       */{	561,	26,		1124,	588,	1125,	1125,	563,	0,		0,		0,			0,			0,		0,		0x07800000	},
/* 720        */{	746,	745,	0,		0,		750,	0,		0,		0,		0,		0,			0,			0,		0,		0x05000000	},
/* 525        */{	264,	30,		1,		293,	525,	3,		265,	4,		266,	0x20000,	0x40000,	0,		0,		0x02D00000	},
/* 625        */{	23,		625,	336,	625,	625,	1,		313,	5,		318,	0x10000,	0x10000,	0,		0,		0x02D00000	},
/* 1080p      */{	1122,	1125,	0,		0,		1125,	0,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2K         */{	1122,	1125,	0,		0,		1125,	0,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2Kx1080p   */{	1122,	1125,	0,		0,		1125,	0,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2Kx1080i   */{	561,	26,		1124,	588,	1125,	1125,	563,	0,		0,		0,			0,			0,		0,		0x07800000	},
/* 3840x2160p */{	1122,	1125,	0,		0,		1125,	0,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 4096x2160p */{	1122,	1125,	0,		0,		1125,	0,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 3840HFR    */{	1122,	1125,	0,		0,		1125,	0,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 4096HFR    */{	1122,	1125,	0,		0,		1125,	0,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
};



/////////////////////////////////////////////
/////////////	ANC INSERTER	/////////////
/////////////////////////////////////////////

static bool SetAncInsField1Bytes (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfBytes)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldBytes), numberOfBytes, maskInsField1Bytes, shiftInsField1Bytes);
}

static bool SetAncInsField2Bytes (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfBytes)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldBytes), numberOfBytes, (uint32_t)maskInsField2Bytes, shiftInsField2Bytes);
}

static bool EnableAncInsHancY (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), bEnable ? 1 : 0, maskInsEnableHancY, shiftInsEnableHancY);
}

static bool EnableAncInsHancC (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), bEnable ? 1 : 0, maskInsEnableHancC, shiftInsEnableHancY);
}

static bool EnableAncInsVancY (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), bEnable ? 1 : 0, maskInsEnableVancY, shiftInsEnableVancY);
}

static bool EnableAncInsVancC (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), bEnable ? 1 : 0, maskInsEnableVancC, shiftInsEnableVancC);
}

static bool SetAncInsProgressive (CNTV2Card & inDevice, const UWord inSDIOutput, bool isProgressive)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), isProgressive ? 1 : 0, maskInsSetProgressive, shiftInsSetProgressive);
}

static bool SetAncInsSDPacketSplit (CNTV2Card & inDevice, const UWord inSDIOutput, bool inEnable)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl),  inEnable ? 1 : 0,  ULWord(maskInsEnablePktSplitSD), shiftInsEnablePktSplitSD);
}

static bool SetAncInsField1StartAddr (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t startAddr)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsField1StartAddr), startAddr);
}

static bool SetAncInsField2StartAddr (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t startAddr)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsField2StartAddr), startAddr);
}

static bool SetAncInsHancPixelDelay (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfPixels)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsPixelDelay), numberOfPixels, maskInsHancDelay, shiftINsHancDelay);
}

static bool SetAncInsVancPixelDelay (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfPixels)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsPixelDelay), numberOfPixels, maskInsVancDelay, shiftInsVancDelay);
}

static bool SetAncInsField1ActiveLine (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t activeLineNumber)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsActiveStart), activeLineNumber, maskInsField1FirstActive, shiftInsField1FirstActive);
}

static bool SetAncInsField2ActiveLine (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t activeLineNumber)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsActiveStart), activeLineNumber, maskInsField2FirstActive, shiftInsField2FirstActive);
}

static bool SetAncInsHActivePixels (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfActiveLinePixels)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsLinePixels), numberOfActiveLinePixels, maskInsActivePixelsInLine, shiftInsActivePixelsInLine);
}

static bool SetAncInsHTotalPixels (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfLinePixels)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsLinePixels), numberOfLinePixels, maskInsTotalPixelsInLine, shiftInsTotalPixelsInLine);
}

static bool SetAncInsTotalLines (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfLines)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFrameLines), numberOfLines, maskInsTotalLinesPerFrame, shiftInsTotalLinesPerFrame);
}

static bool SetAncInsFidHi (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldIDLines), lineNumber, maskInsFieldIDHigh, shiftInsFieldIDHigh);
}

static bool SetAncInsFidLow (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncInsRegNum(inSDIOutput, regAncInsFieldIDLines), lineNumber, maskInsFieldIDLow, shiftInsFieldIDLow);
}

static bool GetAncOffsets (CNTV2Card & inDevice, ULWord & outF1Offset, ULWord & outF2Offset)
{
	outF1Offset = outF2Offset = 0;
	return inDevice.ReadRegister(kVRegAncField1Offset, outF1Offset)
		&&  inDevice.ReadRegister(kVRegAncField2Offset, outF2Offset);
}

static bool GetAncFieldByteCount (CNTV2Card & inDevice, ULWord & outFieldBytes)
{
	outFieldBytes = 0;
	ULWord	ancF1Offset(0), ancF2Offset(0);
	if (!GetAncOffsets(inDevice, ancF1Offset, ancF2Offset))
		return false;
	outFieldBytes = (ancF2Offset - ancF1Offset) - 1;
	return true;
}

bool CNTV2Card::AncInsertInit (const UWord inSDIOutput, const NTV2Channel inChannel, const NTV2Standard inStandard)
{
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	NTV2Standard	theStandard	(inStandard);
	if (!NTV2_IS_VALID_STANDARD(theStandard))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetStandard(theStandard, theChannel))
			return false;
		if (!NTV2_IS_VALID_STANDARD(theStandard))
			return false;
	}

	const ANCInserterInitParams & initParams(inserterInitParamsTable[theStandard]);
	bool ok(true);
	if (ok)	ok = SetAncInsField1ActiveLine (*this, inSDIOutput, initParams.field1ActiveLine);
	if (ok)	ok = SetAncInsField2ActiveLine (*this, inSDIOutput, initParams.field2ActiveLine);
	if (ok)	ok = SetAncInsHActivePixels (*this, inSDIOutput, initParams.hActivePixels);
	if (ok)	ok = SetAncInsHTotalPixels (*this, inSDIOutput, initParams.hTotalPixels);
	if (ok)	ok = SetAncInsTotalLines (*this, inSDIOutput, initParams.totalLines);
	if (ok)	ok = SetAncInsFidLow (*this, inSDIOutput, initParams.fidLow);
	if (ok)	ok = SetAncInsFidHi (*this, inSDIOutput, initParams.fidHigh);
	if (ok)	ok = SetAncInsProgressive (*this, inSDIOutput, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	if (ok)	ok = SetAncInsSDPacketSplit (*this, inSDIOutput, NTV2_IS_SD_STANDARD(theStandard));
	if (ok)	ok = EnableAncInsHancC (*this, inSDIOutput, true);
	if (ok)	ok = EnableAncInsHancY (*this, inSDIOutput, true);
	if (ok)	ok = EnableAncInsVancC (*this, inSDIOutput, true);
	if (ok)	ok = EnableAncInsVancY (*this, inSDIOutput, true);
	if (ok)	ok = SetAncInsHancPixelDelay (*this, inSDIOutput, 0);
	if (ok)	ok = SetAncInsVancPixelDelay (*this, inSDIOutput, 0);
	if (ok)	ok = WriteRegister (AncInsRegNum(inSDIOutput, regAncInsBlankCStartLine), 0);
	if (ok)	ok = WriteRegister (AncInsRegNum(inSDIOutput, regAncInsBlankField1CLines), 0);
	if (ok)	ok = WriteRegister (AncInsRegNum(inSDIOutput, regAncInsBlankField2CLines), 0);

	ULWord	fieldBytes(0);
	if (ok)	ok = GetAncFieldByteCount(*this, fieldBytes);
	if (ok)	ok = SetAncInsField1Bytes (*this, inSDIOutput, fieldBytes);
	if (ok)	ok = SetAncInsField2Bytes (*this, inSDIOutput, fieldBytes);
	return ok;
}


bool CNTV2Card::AncInsertSetEnable (const UWord inSDIOutput, const bool inIsEnabled)
{
	bool ok(NTV2_IS_VALID_CHANNEL(inSDIOutput));
    if (!inIsEnabled)
    {
        if (ok)	ok = EnableAncInsHancC(*this, inSDIOutput, false);
        if (ok)	ok = EnableAncInsHancY(*this, inSDIOutput, false);
        if (ok)	ok = EnableAncInsVancC(*this, inSDIOutput, false);
        if (ok)	ok = EnableAncInsVancY(*this, inSDIOutput, false);
    }
    if (ok)	ok = WriteRegister(AncInsRegNum(inSDIOutput, regAncInsBlankCStartLine), 0);
    if (ok)	ok = WriteRegister(AncInsRegNum(inSDIOutput, regAncInsBlankField1CLines), 0);
    if (ok)	ok = WriteRegister(AncInsRegNum(inSDIOutput, regAncInsBlankField2CLines), 0);
    if (ok)	ok = WriteRegister(AncInsRegNum(inSDIOutput, regAncInsControl), inIsEnabled ? 0 : 1, maskInsDisableInserter, shiftInsDisableInserter);
    return ok;
}


bool CNTV2Card::AncInsertIsEnabled (const UWord inSDIOutput, bool & outIsRunning)
{
	outIsRunning = false;
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (inSDIOutput >= ::NTV2DeviceGetNumVideoOutputs(_boardID))
		return false;

	ULWord	value(0);
	if (!ReadRegister(AncInsRegNum(inSDIOutput, regAncInsControl), value))
		return false;
	outIsRunning = (value & BIT(28)) ? false : true;
	return true;
}


bool CNTV2Card::AncInsertSetReadParams (const UWord inSDIOutput, const ULWord inFrameNumber, const ULWord inF1Size,
										const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	NTV2Framesize	theFrameSize(inFrameSize);
	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetFrameBufferSize(theChannel, theFrameSize))
			return false;
		if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
			return false;
	}

	bool ok(true);
	//	Calculate where ANC Inserter will read the data
	const ULWord	frameNumber (inFrameNumber + 1);	//	Start at beginning of next frame (then subtract offset later)
	const ULWord	frameLocation (::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	ULWord			F1Offset(0);
	if (ok)	ok = ReadRegister (kVRegAncField1Offset, F1Offset);
	const ULWord	ANCStartMemory (frameLocation - F1Offset);
	if (ok)	ok = SetAncInsField1StartAddr (*this, inChannel, ANCStartMemory);
	if (ok)	ok = SetAncInsField1Bytes (*this, inChannel, inF1Size);
	return ok;
}

bool CNTV2Card::AncInsertSetField2ReadParams (const UWord inSDIOutput, const ULWord inFrameNumber, const ULWord inF2Size,
												const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!::NTV2DeviceCanDoPlayback(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	NTV2Framesize	theFrameSize(inFrameSize);
	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetFrameBufferSize(theChannel, theFrameSize))
			return false;
		if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
			return false;
	}

	bool ok(true);
	//	Calculate where ANC Inserter will read the data
	const ULWord	frameNumber (inFrameNumber + 1);	//	Start at beginning of next frame (then subtract offset later)
	const ULWord	frameLocation (::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	ULWord			F2Offset(0);
	if (ok)	ok = ReadRegister (kVRegAncField2Offset, F2Offset);
	const ULWord	ANCStartMemory (frameLocation - F2Offset);
	if (ok)	ok = SetAncInsField2StartAddr (*this, inChannel, ANCStartMemory);
	if (ok)	ok = SetAncInsField2Bytes (*this, inChannel, inF2Size);
    return true;
}



/////////////////////////////////////////////
/////////////	ANC EXTRACTOR	/////////////
/////////////////////////////////////////////

static bool EnableAncExtHancY (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableHancY, shiftEnableHancY);
}

static bool EnableAncExtHancC (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableHancC, shiftEnableHancC);
}

static bool EnableAncExtVancY (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableVancY, shiftEnableVancY);
}

static bool EnableAncExtVancC (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableVancC, shiftEnableVancC);
}

static bool SetAncExtSDDemux (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskEnableSDMux, shiftEnableSDMux);
}

static bool SetAncExtProgressive (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, maskSetProgressive, shiftSetProgressive);
}

static bool SetAncExtSynchro (CNTV2Card & inDevice, const UWord inSDIInput)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), 0x1, maskSyncro, shiftSyncro);
}

/* currently unused
static bool SetAncExtLSBEnable (CNTV2Card & inDevice, const UWord inSDIInput, bool bEnable)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtControl), bEnable ? 1 : 0, (ULWord)maskGrabLSBs, shiftGrabLSBs);
}
*/

static bool SetAncExtField1StartAddr (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t addr)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField1StartAddress), addr);
}

static bool SetAncExtField1EndAddr (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t addr)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField1EndAddress), addr);
}

static bool SetAncExtField2StartAddr (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t addr)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField2StartAddress), addr);
}

static bool SetAncExtField2EndAddr (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t addr)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField2EndAddress), addr);
}

static bool SetAncExtField1CutoffLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFieldCutoffLine), lineNumber, maskField1CutoffLine, shiftField1CutoffLine);
}

static bool SetAncExtField2CutoffLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFieldCutoffLine), lineNumber, maskField2CutoffLine, shiftField2CutoffLine);
}

static bool IsAncExtOverrun (CNTV2DriverInterface & inDevice, const UWord inSDIInput, bool & outIsOverrun)
{
	return inDevice.ReadRegister(AncExtRegNum(inSDIInput, regAncExtTotalStatus), outIsOverrun, maskTotalOverrun, shiftTotalOverrun);
}

static bool SetAncExtField1StartLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFieldVBLStartLine), lineNumber, maskField1StartLine, shiftField1StartLine);
}

static bool SetAncExtField2StartLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFieldVBLStartLine), lineNumber, maskField2StartLine, shiftField2StartLine);
}

static bool SetAncExtTotalFrameLines (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t totalFrameLines)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtTotalFrameLines), totalFrameLines, maskTotalFrameLines, shiftTotalFrameLines);
}

static bool SetAncExtFidLow (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFID), lineNumber, maskFIDLow, shiftFIDLow);
}

static bool SetAncExtFidHi (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtFID), lineNumber, maskFIDHi, shiftFIDHi);
}

static bool SetAncExtField1AnalogStartLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtAnalogStartLine), lineNumber, maskField1AnalogStartLine, shiftField1AnalogStartLine);
}

static bool SetAncExtField2AnalogStartLine (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineNumber)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtAnalogStartLine), lineNumber, maskField2AnalogStartLine, shiftField2AnalogStartLine);
}

static bool SetAncExtField1AnalogYFilter (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineFilter)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField1AnalogYFilter), lineFilter);
}

static bool SetAncExtField2AnalogYFilter (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineFilter)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField2AnalogYFilter), lineFilter);
}

static bool SetAncExtField1AnalogCFilter (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineFilter)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField1AnalogCFilter), lineFilter);
}

static bool SetAncExtField2AnalogCFilter (CNTV2Card & inDevice, const UWord inSDIInput, uint32_t lineFilter)
{
    return inDevice.WriteRegister(AncExtRegNum(inSDIInput, regAncExtField2AnalogCFilter), lineFilter);
}


bool CNTV2Card::AncExtractInit (const UWord inSDIInput, const NTV2Channel inChannel, const NTV2Standard inStandard)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIInput));
	NTV2Standard	theStandard	(inStandard);
	if (!NTV2_IS_VALID_STANDARD(theStandard))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetStandard(theStandard, theChannel))
			return false;
		if (!NTV2_IS_VALID_STANDARD(theStandard))
			return false;
	}

	const ANCExtractorInitParams &  extractorParams (extractorInitParamsTable[theStandard]);
	bool ok(true);
	if (ok)	ok = SetAncExtProgressive (*this, inSDIInput, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	if (ok)	ok = SetAncExtField1StartLine (*this, inSDIInput, extractorParams.field1StartLine);
	if (ok)	ok = SetAncExtField1CutoffLine (*this, inSDIInput, extractorParams.field1CutoffLine);
	if (ok)	ok = SetAncExtField2StartLine (*this, inSDIInput, extractorParams.field2StartLine);
	if (ok)	ok = SetAncExtField2CutoffLine (*this, inSDIInput, extractorParams.field2CutoffLine);
	if (ok)	ok = SetAncExtTotalFrameLines (*this, inSDIInput, extractorParams.totalLines);
	if (ok)	ok = SetAncExtFidLow (*this, inSDIInput, extractorParams.fidLow);
	if (ok)	ok = SetAncExtFidHi (*this, inSDIInput, extractorParams.fidHigh);
	if (ok)	ok = SetAncExtField1AnalogStartLine (*this, inSDIInput, extractorParams.field1AnalogStartLine);
	if (ok)	ok = SetAncExtField2AnalogStartLine (*this, inSDIInput, extractorParams.field2AnalogStartLine);
	if (ok)	ok = SetAncExtField1AnalogYFilter (*this, inSDIInput, extractorParams.field1AnalogYFilter);
	if (ok)	ok = SetAncExtField2AnalogYFilter (*this, inSDIInput, extractorParams.field2AnalogYFilter);
	if (ok)	ok = SetAncExtField1AnalogCFilter (*this, inSDIInput, extractorParams.field1AnalogCFilter);
	if (ok)	ok = SetAncExtField2AnalogCFilter (*this, inSDIInput, extractorParams.field2AnalogCFilter);
	if (ok)	ok = AncExtractSetFilterDIDs (inSDIInput, AncExtractGetDefaultDIDs());
	if (ok)	ok = WriteRegister (AncExtRegNum(inSDIInput, regAncExtAnalogActiveLineLength), extractorParams.analogActiveLineLength);
	if (ok)	ok = SetAncExtSDDemux (*this, inSDIInput, NTV2_IS_SD_STANDARD(theStandard));
	if (ok)	ok = EnableAncExtHancC (*this, inSDIInput, true);
	if (ok)	ok = EnableAncExtHancY (*this, inSDIInput, true);
	if (ok)	ok = EnableAncExtVancC (*this, inSDIInput, true);
	if (ok)	ok = EnableAncExtVancY (*this, inSDIInput, true);
	if (ok)	ok = SetAncExtSynchro (*this, inSDIInput);
	if (ok)	ok = SetAncExtField1StartAddr (*this, inSDIInput, 0);
	if (ok)	ok = SetAncExtField1EndAddr (*this, inSDIInput, 0);
	if (ok)	ok = SetAncExtField2StartAddr (*this, inSDIInput, 0);
	if (ok)	ok = SetAncExtField2EndAddr (*this, inSDIInput, 0);
	return ok;
}

bool CNTV2Card::AncExtractSetEnable (const UWord inSDIInput, const bool inIsEnabled)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	bool ok(true);
	if (!inIsEnabled)
	{
		if (ok)	ok = EnableAncExtHancC (*this, inSDIInput, false);
		if (ok)	ok = EnableAncExtHancY (*this, inSDIInput, false);
		if (ok)	ok = EnableAncExtVancC (*this, inSDIInput, false);
		if (ok)	ok = EnableAncExtVancY (*this, inSDIInput, false);
	}
	if (ok)	ok = WriteRegister (AncExtRegNum(inSDIInput, regAncExtControl), inIsEnabled ? 0 : 1, maskDisableExtractor, shiftDisableExtractor);
	return ok;
}

bool CNTV2Card::AncExtractIsEnabled (const UWord inSDIInput, bool & outIsRunning)
{
	outIsRunning = false;
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	ULWord	value(0);
	if (!ReadRegister(AncExtRegNum(inSDIInput, regAncExtControl), value))
		return false;
	outIsRunning = (value & BIT(28)) ? false : true;
	return true;
}

bool CNTV2Card::AncExtractSetWriteParams (const UWord inSDIInput, const ULWord inFrameNumber,
											const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIInput));
	NTV2Framesize	theFrameSize(inFrameSize);
	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetFrameBufferSize(theChannel, theFrameSize))
			return false;
		if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
			return false;
	}
	if (IS_CHANNEL_INVALID(theChannel))
		return false;

	//	Calculate where ANC Extractor will put the data...
	bool			ok					(true);
	const ULWord	frameNumber			(inFrameNumber + 1);	//	This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord			endOfFrameLocation	(::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	ULWord			isQuadFormatEnabled	(0);
	if (ok)	ok = GetQuadFrameEnable (isQuadFormatEnabled, theChannel);
	if (isQuadFormatEnabled)
		endOfFrameLocation *= 4;

	ULWord F1Offset(0), F2Offset(0);
	if (ok)	ok = GetAncOffsets (*this, F1Offset, F2Offset);

	const ULWord	ANCStartMemory	(endOfFrameLocation - F1Offset);
	const ULWord	ANCStopMemory	(endOfFrameLocation - F2Offset - 1);
	if (ok)	ok = SetAncExtField1StartAddr (*this, inSDIInput, ANCStartMemory);
	if (ok)	ok = SetAncExtField1EndAddr (*this, inSDIInput, ANCStopMemory);
	return ok;
}

bool CNTV2Card::AncExtractSetField2WriteParams (const UWord inSDIInput, const ULWord inFrameNumber,
												const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIInput));
	NTV2Framesize	theFrameSize(inFrameSize);
	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetFrameBufferSize(theChannel, theFrameSize))
			return false;
		if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
			return false;
	}
	if (IS_CHANNEL_INVALID(theChannel))
		return false;

	//	Calculate where ANC Extractor will put the data...
	bool			ok					(true);
	const ULWord	frameNumber			(inFrameNumber + 1);	//	This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord			endOfFrameLocation	(::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	ULWord			isQuadFormatEnabled	(0);
	if (ok)	ok = GetQuadFrameEnable (isQuadFormatEnabled, theChannel);
	if (isQuadFormatEnabled)
		endOfFrameLocation *= 4;

	ULWord	F2Offset(0);
	if (ok)	ok = ReadRegister(kVRegAncField2Offset, F2Offset);

	const ULWord	ANCStartMemory	(endOfFrameLocation - F2Offset);
	const ULWord	ANCStopMemory	(endOfFrameLocation - 1);
	if (ok)	ok = SetAncExtField2StartAddr (*this, inSDIInput, ANCStartMemory);
	if (ok)	ok = SetAncExtField2EndAddr (*this, inSDIInput, ANCStopMemory);
    return true;
}

bool CNTV2Card::AncExtractGetFilterDIDs (const UWord inSDIInput, NTV2DIDSet & outDIDs)
{
	outDIDs.clear();
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	const ULWord	firstIgnoreRegNum	(AncExtRegNum(inSDIInput, regAncExtIgnorePktsReg_First));
	for (ULWord regNdx(0);  regNdx < kNumDIDRegisters;  regNdx++)
	{
		ULWord	regValue	(0);
		ReadRegister (firstIgnoreRegNum + regNdx,  regValue);
		for (unsigned regByte(0);  regByte < 4;  regByte++)
		{
			const NTV2DID	theDID	((regValue >> (regByte*8)) & 0x000000FF);
			if (theDID)
				outDIDs.insert(theDID);
		}
	}
	return true;
}

bool CNTV2Card::AncExtractSetFilterDIDs (const UWord inSDIInput, const NTV2DIDSet & inDIDs)
{
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;

	const ULWord		firstIgnoreRegNum	(AncExtRegNum(inSDIInput, regAncExtIgnorePktsReg_First));
	NTV2DIDSetConstIter iter				(inDIDs.begin());

	for (ULWord regNdx(0);  regNdx < kNumDIDRegisters;  regNdx++)
	{
		ULWord	regValue	(0);
		for (unsigned regByte(0);  regByte < 4;  regByte++)
		{
			const NTV2DID	theDID	(iter != inDIDs.end()  ?  *iter++  :  0);
			regValue |= (ULWord(theDID) << (regByte*8));
		}
		WriteRegister (firstIgnoreRegNum + regNdx,  regValue);
	}
	return true;
}

bool CNTV2Card::AncExtractGetBufferOverrun (const UWord inSDIInput, bool & outIsOverrun)
{
	outIsOverrun = false;
	if (!::NTV2DeviceCanDoCapture(_boardID))
		return false;
	if (!::NTV2DeviceCanDoCustomAnc(_boardID))
		return false;
	if (IS_INPUT_SPIGOT_INVALID(inSDIInput))
		return false;
	return IsAncExtOverrun (*this, inSDIInput, outIsOverrun);
}



/////////////////////////////////////////////////
/////////////	STATIC FUNCTIONS	/////////////
/////////////////////////////////////////////////

UWord CNTV2Card::AncExtractGetMaxNumFilterDIDs (void)
{
	static const ULWord	kNumDIDsPerRegister	(4);
	return UWord(kNumDIDsPerRegister * kNumDIDRegisters);
}


NTV2DIDSet CNTV2Card::AncExtractGetDefaultDIDs (void)
{
	//												SMPTE299 HD Audio Grp 1-4	SMPTE299 HD Audio Ctrl Grp 1-4
	static const NTV2DID	sDefaultDIDs[]	=	{	0xE7,0xE6,0xE5,0xE4,		0xE3,0xE2,0xE1,0xE0,
	//												SMPTE299 HD Audio Grp 5-8	SMPTE299 HD Audio Ctrl Grp 5-8
													0xA7,0xA6,0xA5,0xA4,		0xA3,0xA2,0xA1,0xA0,
													0x00};
	NTV2DIDSet	result;
	for (unsigned ndx(0);  sDefaultDIDs[ndx];  ndx++)
		result.insert(sDefaultDIDs[ndx]);

	return result;
}
