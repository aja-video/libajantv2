/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2aux.cpp
	@brief		Implementations of HDMI aux-centric CNTV2Card methods.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

//==========================================================================
//
//  HDMI Aux Insertion is not yet implemented.  The functions here are copied from
//  Ans Insertion to be used as a potential template.
//
//==========================================================================


#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2formatdescriptor.h"
#include "ntv2utils.h"


/////////////////////////////////////////////////////////////////
/////////////	PER-HDMI-SPIGOT REGISTER NUMBERS		/////////////
/////////////////////////////////////////////////////////////////

//								HDMI PORT:		   1	   2	   3	   4
// CURRENTLY UNUSED // static const ULWord sAuxInsBaseRegNum[] =	{	4608,	4672,	4736,	4800	};
static const ULWord sAuxExtBaseRegNum[] =	{	7616,	7680,	7744,	7808	};
static const ULWord kNumDIDRegisters		(regAuxExtPacketMask_Last - regAuxExtPacketMask_First + 1);

// AUXInsRegisters not yet defined
// static inline ULWord	AuxInsRegNum(const UWord inSDIOutput, const AUXInsRegisters inReg)
// {
// 	return sAuxInsBaseRegNum[inSDIOutput] + inReg;
// }

static inline ULWord	AuxExtRegNum(const UWord inHDMIInput, const AUXExtRegisters inReg)
{
	return sAuxExtBaseRegNum[inHDMIInput] + inReg;
}

/////////////////////////////////////////////////////////////////////
/////////////	PER-VIDEO-STANDARD INSERTER INIT SETUP	/////////////
/////////////////////////////////////////////////////////////////////

// CURRENTLY UNUSED //
// typedef struct AUXInserterInitParams
// {
// 	uint32_t	field1ActiveLine;
// 	uint32_t	field2ActiveLine;
// 	uint32_t	hActivePixels;
// 	uint32_t	hTotalPixels;
// 	uint32_t	totalLines;
// 	uint32_t	fidLow;
// 	uint32_t	fidHigh;
// 	uint32_t	field1SwitchLine;
// 	uint32_t	field2SwitchLine;
// 	uint32_t	pixelDelay;
// } AUXInserterInitParams;

// CURRENTLY UNUSED //

// static const AUXInserterInitParams	inserterInitParamsTable [NTV2_NUM_STANDARDS] = {
// //						F1		F2		Horz									F1		F2
// //						Active	Active	Active	Total	Total	FID		FID		Switch	Switch	Pixel
// //	Standard			Line	Line	Pixels	Pixels	Lines	Lo		Hi		Line	Line	Delay
// /* 1080		  */	{	21,		564,	1920,	2200,	1125,	1125,	563,	7,		569,	8,		},
// /* 720		  */	{	26,		0,		1280,	1280,	750,	0,		0,		7,		0,		8,		},
// /* 525		  */	{	21,		283,	720,	720,	525,	3,		265,	10,		273,	8,		},
// /* 625		  */	{	23,		336,	720,	864,	625,	625,	312,	6,		319,	8,		},
// /* 1080p	  */	{	42,		0,		1920,	2640,	1125,	0,		0,		7,		0,		8,		},
// /* 2K		  */	{	42,		0,		2048,	2640,	1125,	0,		0,		7,		0,		8,		},
// /* 2Kx1080p	  */	{	42,		0,		2048,	2640,	1125,	0,		0,		7,		0,		8,		},
// /* 2Kx1080i	  */	{	21,		564,	2048,	2200,	1125,	1125,	563,	7,		569,	8,		},
// /* 3840x2160p */	{	42,		0,		1920,	2640,	1125,	0,		0,		7,		0,		8,		},
// /* 4096x2160p */	{	42,		0,		2048,	2640,	1125,	0,		0,		7,		0,		8,		},
// /* 3840HFR	  */	{	42,		0,		1920,	2640,	1125,	0,		0,		7,		0,		8,		},
// /* 4096HFR	  */	{	42,		0,		2048,	2640,	1125,	0,		0,		7,		0,		8,		}
// };



/////////////////////////////////////////////////////////////////////
/////////////	PER-VIDEO-STANDARD EXTRACTOR INIT SETUP /////////////
/////////////////////////////////////////////////////////////////////

typedef struct AUXExtractorInitParams
{
	uint32_t	field1StartLine;
	uint32_t	field1CutoffLine;
	uint32_t	field2StartLine;
	uint32_t	field2CutoffLine;
	uint32_t	totalLines;
	uint32_t	fidLow;
	uint32_t	fidHigh;
	uint32_t	field1SwitchLine;
	uint32_t	field2SwitchLine;
	uint32_t	field1AnalogStartLine;
	uint32_t	field2AnalogStartLine;
	uint32_t	field1AnalogYFilter;
	uint32_t	field2AnalogYFilter;
	uint32_t	field1AnalogCFilter;
	uint32_t	field2AnalogCFilter;
	uint32_t	analogActiveLineLength;
} AUXExtractorInitParams;

const static AUXExtractorInitParams	 extractorInitParamsTable [NTV2_NUM_STANDARDS] = {
//					F1		F1		F2		F2								F1		F2		F1Anlg	F2Anlg	F1			F2			F1		F2		Analog
//					Start	Cutoff	Start	Cutoff	Total	FID		FID		Switch	Switch	Start	Start	Anlg		Anlg		Anlg	Anlg	Active
// Standard			Line	Line	Line	Line	Lines	Low		High	Line	Line	Line	Line	Y Filt		Y Filt		C Filt	C Filt	LineLength
/* 1080		  */{	561,	26,		1124,	588,	1125,	1125,	563,	7,		569,	0,		0,		0,			0,			0,		0,		0x07800000	},
/* 720		  */{	746,	745,	0,		0,		750,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x05000000	},
/* 525		  */{	264,	30,		1,		293,	525,	3,		265,	10,		273,	4,		266,	0x20000,	0x40000,	0,		0,		0x02D00000	},
/* 625		  */{	311,	33,		1,		346,	625,	625,	312,	6,		319,	5,		318,	0x10000,	0x10000,	0,		0,		0x02D00000	},
/* 1080p	  */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2K		  */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2Kx1080p	  */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 2Kx1080i	  */{	561,	26,		1124,	588,	1125,	1125,	563,	7,		569,	0,		0,		0,			0,			0,		0,		0x07800000	},
/* 3840x2160p */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 4096x2160p */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 3840HFR	  */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
/* 4096HFR	  */{	1122,	1125,	0,		0,		1125,	0,		0,		7,		0,		0,		0,		0,			0,			0,		0,		0x07800000	},
};


/////////////////////////////////////////////
//////////////	 AUX BUFFER	   //////////////
/////////////////////////////////////////////

// As of now, Aux buffers share the same virtual registers than Anc uses: 
// 		kVRegAncField1Offset & kVRegAncField2Offset
// To reduce user confusion, we created AuxSetFrameBufferSize, but it simply calls AncSetFrameBufferSize,
// which sets these two registers.
bool CNTV2Card::AuxSetFrameBufferSize (const ULWord inF1Size, const ULWord inF2Size)
{
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	return AncSetFrameBufferSize(inF1Size,inF2Size);
}

static bool GetAuxOffsets (CNTV2Card & inDevice, ULWord & outF1Offset, ULWord & outF2Offset)
{
	outF1Offset = outF2Offset = 0;
	return inDevice.ReadRegister(kVRegAncField1Offset, outF1Offset)
		&&	inDevice.ReadRegister(kVRegAncField2Offset, outF2Offset);
}

// static bool GetAuxField1Size (CNTV2Card & inDevice, ULWord & outFieldBytes)
// {
	// outFieldBytes = 0;
	// ULWord	auxF1Offset(0), auxF2Offset(0);
	// if (!GetAuxOffsets(inDevice, auxF1Offset, auxF2Offset))
	// 	return false;
	// outFieldBytes = auxF1Offset - auxF2Offset;
	// return true;
// }

// static bool GetAuxField2Size (CNTV2Card & inDevice, ULWord & outFieldBytes)
// {
	// outFieldBytes = 0;
	// ULWord	auxF1Offset(0), auxF2Offset(0);
	// if (!GetAuxOffsets(inDevice, auxF1Offset, auxF2Offset))
	// 	return false;
	// outFieldBytes = auxF2Offset;
	// return true;
// }


/////////////////////////////////////////////
/////////////	AUX INSERTER	/////////////
/////////////////////////////////////////////

// static bool SetAuxInsField1Bytes (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfBytes)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsFieldBytes), numberOfBytes & 0xffff,
	// 							  maskInsField1Bytes, shiftInsField1Bytes) &&
	// 		inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsFieldBytesHigh), numberOfBytes >> 16,
	// 										  maskInsField1Bytes, shiftInsField1Bytes);
// }

// static bool SetAuxInsField2Bytes (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfBytes)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsFieldBytes), numberOfBytes & 0xffff,
	// 							  maskInsField2Bytes, shiftInsField2Bytes) &&
	// 		inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsFieldBytesHigh), numberOfBytes >> 16,
	// 										  maskInsField2Bytes, shiftInsField2Bytes);
// }

// static bool EnableAuxInsHauxY (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), bEnable ? 1 : 0, maskInsEnableHauxY, shiftInsEnableHauxY);
// }

// static bool EnableAuxInsHauxC (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), bEnable ? 1 : 0, maskInsEnableHauxC, shiftInsEnableHauxC);
// }

// static bool EnableAuxInsVauxY (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), bEnable ? 1 : 0, maskInsEnableVauxY, shiftInsEnableVauxY);
// }

// static bool EnableAuxInsVauxC (CNTV2Card & inDevice, const UWord inSDIOutput, bool bEnable)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), bEnable ? 1 : 0, maskInsEnableVauxC, shiftInsEnableVauxC);
// }

// static bool SetAuxInsProgressive (CNTV2Card & inDevice, const UWord inSDIOutput, bool isProgressive)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), isProgressive ? 1 : 0, maskInsSetProgressive, shiftInsSetProgressive);
// }

// static bool SetAuxInsSDPacketSplit (CNTV2Card & inDevice, const UWord inSDIOutput, bool inEnable)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl),	inEnable ? 1 : 0,  ULWord(maskInsEnablePktSplitSD), shiftInsEnablePktSplitSD);
// }

// static bool GetAuxInsStartAddrs (CNTV2Card & inDevice, const UWord inSDIOutput, uint64_t & outStartAddrF1, uint64_t & outStartAddrF2)
// {
	// uint32_t startAddrF1(0), startAddrF2(0);
	// bool ok = inDevice.ReadRegister(AuxInsRegNum(inSDIOutput, regAuxInsField1StartAddr), startAddrF1)
	// 		&&  inDevice.ReadRegister(AuxInsRegNum(inSDIOutput, regAuxInsField2StartAddr), startAddrF2);
	// outStartAddrF1 = ok ? uint64_t(startAddrF1) : 0;
	// outStartAddrF2 = ok ? uint64_t(startAddrF2) : 0;
	// return ok;
// }

// static bool SetAuxInsField1StartAddr (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t startAddr)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsField1StartAddr), startAddr);
// }

// static bool SetAuxInsField2StartAddr (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t startAddr)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsField2StartAddr), startAddr);
// }

// static bool SetAuxInsHauxPixelDelay (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfPixels)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsPixelDelay), numberOfPixels, maskInsHauxDelay, shiftINsHauxDelay);
// }

// static bool SetAuxInsVauxPixelDelay (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfPixels)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsPixelDelay), numberOfPixels, maskInsVauxDelay, shiftInsVauxDelay);
// }

// static bool SetAuxInsField1ActiveLine (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t activeLineNumber)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsActiveStart), activeLineNumber, maskInsField1FirstActive, shiftInsField1FirstActive);
// }

// static bool SetAuxInsField2ActiveLine (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t activeLineNumber)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsActiveStart), activeLineNumber, maskInsField2FirstActive, shiftInsField2FirstActive);
// }

// static bool SetAuxInsHActivePixels (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfActiveLinePixels)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsLinePixels), numberOfActiveLinePixels, maskInsActivePixelsInLine, shiftInsActivePixelsInLine);
// }

// static bool SetAuxInsHTotalPixels (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfLinePixels)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsLinePixels), numberOfLinePixels, maskInsTotalPixelsInLine, shiftInsTotalPixelsInLine);
// }

// static bool SetAuxInsTotalLines (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t numberOfLines)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsFrameLines), numberOfLines, maskInsTotalLinesPerFrame, shiftInsTotalLinesPerFrame);
// }

// static bool SetAuxInsFidHi (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t lineNumber)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsFieldIDLines), lineNumber, maskInsFieldIDHigh, shiftInsFieldIDHigh);
// }

// static bool SetAuxInsFidLow (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t lineNumber)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsFieldIDLines), lineNumber, maskInsFieldIDLow, shiftInsFieldIDLow);
// }

// static bool SetAuxInsRtpPayloadID (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t payloadID)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsRtpPayloadID), payloadID);
// }

// static bool SetAuxInsRtpSSRC (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t ssrc)
// {
	// return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsRtpSSRC), ssrc);
// }

// static bool SetAuxInsIPChannel (CNTV2Card & inDevice, const UWord inSDIOutput, uint32_t channel)
// {
// 	return inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsIpChannel), channel);
// }

// static bool GetAuxInsExtendedMode (CNTV2Card & inDevice, const UWord inSDIOutput, bool& extendedMode)
// {
	// bool		ok(true);
	// uint32_t	regValue(0);
	// extendedMode = false;
	// if (ok) ok = inDevice.WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), 1, maskInsExtendedMode, shiftInsExtendedMode);
	// if (ok) ok = inDevice.ReadRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), regValue, maskInsExtendedMode, shiftInsExtendedMode);
	// if (ok) extendedMode = (regValue == 1);
	// return ok;
// }

bool CNTV2Card::AuxInsertInit (const UWord inSDIOutput, const NTV2Channel inChannel, const NTV2Standard inStandard)
{
	// if (!IsSupported(kDeviceCanDoPlayback))
	// 	return false;
	// if (!IsSupported(kDeviceCanDoCustomAux))
	// 	return false;
	// if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
	// 	return false;

	// NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	// NTV2Standard	theStandard (inStandard);
	// if (!NTV2_IS_VALID_STANDARD(theStandard))
	// {
	// 	if (IS_CHANNEL_INVALID(theChannel))
	// 		return false;
	// 	if (!GetStandard(theStandard, theChannel))
	// 		return false;
	// 	if (!NTV2_IS_VALID_STANDARD(theStandard))
	// 		return false;
	// }

	// const AUXInserterInitParams & initParams(inserterInitParamsTable[theStandard]);
	// bool ok(true);
	// bool extendedMode(false);
	// if (ok) ok = GetAuxInsExtendedMode (*this, inSDIOutput, extendedMode);
	// if (ok) ok = SetAuxInsField1ActiveLine (*this, inSDIOutput, initParams.field1ActiveLine);
	// if (ok) ok = SetAuxInsField2ActiveLine (*this, inSDIOutput, initParams.field2ActiveLine);
	// if (ok) ok = SetAuxInsHActivePixels (*this, inSDIOutput, initParams.hActivePixels);
	// if (ok) ok = SetAuxInsHTotalPixels (*this, inSDIOutput, initParams.hTotalPixels);
	// if (ok) ok = SetAuxInsTotalLines (*this, inSDIOutput, initParams.totalLines);
	// if (ok) ok = SetAuxInsFidLow (*this, inSDIOutput, extendedMode? initParams.field1SwitchLine : initParams.fidLow);
	// if (ok) ok = SetAuxInsFidHi (*this, inSDIOutput, extendedMode? initParams.field2SwitchLine : initParams.fidHigh);
	// if (ok) ok = SetAuxInsProgressive (*this, inSDIOutput, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	// if (ok) ok = SetAuxInsSDPacketSplit (*this, inSDIOutput, NTV2_IS_SD_STANDARD(theStandard));
	// if (ok) ok = EnableAuxInsHauxC (*this, inSDIOutput, false);
	// if (ok) ok = EnableAuxInsHauxY (*this, inSDIOutput, false);
	// if (ok) ok = EnableAuxInsVauxC (*this, inSDIOutput, true);
	// if (ok) ok = EnableAuxInsVauxY (*this, inSDIOutput, true);
	// if (ok) ok = SetAuxInsHauxPixelDelay (*this, inSDIOutput, 0);
	// if (ok) ok = SetAuxInsVauxPixelDelay (*this, inSDIOutput, 0);
	// if (ok) ok = WriteRegister (AuxInsRegNum(inSDIOutput, regAuxInsBlankCStartLine), 0);
	// if (ok) ok = WriteRegister (AuxInsRegNum(inSDIOutput, regAuxInsBlankField1CLines), 0);
	// if (ok) ok = WriteRegister (AuxInsRegNum(inSDIOutput, regAuxInsBlankField2CLines), 0);
	// if (ok) ok = WriteRegister (AuxInsRegNum(inSDIOutput, regAuxInsPixelDelay), extendedMode? initParams.pixelDelay : 0);

	// ULWord	field1Bytes(0);
	// ULWord	field2Bytes(0);
	// if (ok) ok = GetAuxField1Size(*this, field1Bytes);
	// if (ok) ok = GetAuxField2Size(*this, field2Bytes);
	// if (ok) ok = SetAuxInsField1Bytes (*this, inSDIOutput, field1Bytes);
	// if (ok) ok = SetAuxInsField2Bytes (*this, inSDIOutput, field2Bytes);
	// return ok;
	
	return false;
}

bool CNTV2Card::AuxInsertSetComponents (const UWord inSDIOutput,
										const bool inVauxY, const bool inVauxC,
										const bool inHauxY, const bool inHauxC)
{
	// bool ok(true);
	// bool extendedMode(false);
	// if (ok) ok = EnableAuxInsVauxY(*this, inSDIOutput, inVauxY);
	// if (ok) ok = EnableAuxInsVauxC(*this, inSDIOutput, inVauxC);
	// if (ok) ok = GetAuxInsExtendedMode (*this, inSDIOutput, extendedMode);
	// if (extendedMode)
	// {
	// 	if (ok) ok = EnableAuxInsHauxY(*this, inSDIOutput, inHauxY);
	// 	if (ok) ok = EnableAuxInsHauxC(*this, inSDIOutput, inHauxC);
	// }
	// return ok;
	
	return false;
}

bool CNTV2Card::AuxInsertSetEnable (const UWord inSDIOutput, const bool inIsEnabled)
{
	// if (!IsSupported(kDeviceCanDoPlayback))
	// 	return false;
	// if (!IsSupported(kDeviceCanDoCustomAux))
	// 	return false;
	// if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
	// 	return false;
	// bool ok(true);
	// if (!inIsEnabled)
	// {
	// 	if (ok) ok = EnableAuxInsHauxC(*this, inSDIOutput, false);
	// 	if (ok) ok = EnableAuxInsHauxY(*this, inSDIOutput, false);
	// 	if (ok) ok = EnableAuxInsVauxC(*this, inSDIOutput, false);
	// 	if (ok) ok = EnableAuxInsVauxY(*this, inSDIOutput, false);
	// }
	// if (ok) ok = WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsBlankCStartLine), 0);
	// if (ok) ok = WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsBlankField1CLines), 0);
	// if (ok) ok = WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsBlankField2CLines), 0);
	// if (ok) ok = WriteRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), inIsEnabled ? 0 : 1, maskInsDisableInserter, shiftInsDisableInserter);
	// return ok;
	
	return false;
}

bool CNTV2Card::AuxInsertIsEnabled (const UWord inSDIOutput, bool & outIsRunning)
{
	// outIsRunning = false;
	// if (!IsSupported(kDeviceCanDoPlayback))
	// 	return false;
	// if (!IsSupported(kDeviceCanDoCustomAux))
	// 	return false;
	// if (ULWord(inSDIOutput) >= GetNumSupported(kDeviceGetNumVideoOutputs))
	// 	return false;

	// ULWord	value(0);
	// if (!ReadRegister(AuxInsRegNum(inSDIOutput, regAuxInsControl), value))
	// 	return false;
	// outIsRunning = (value & BIT(28)) ? false : true;
	// return true;
	
	return false;
}

bool CNTV2Card::AuxInsertSetReadParams (const UWord inSDIOutput, const ULWord inFrameNumber, const ULWord inF1Size,
										const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	// if (!IsSupported(kDeviceCanDoPlayback))
	// 	return false;
	// if (!IsSupported(kDeviceCanDoCustomAux))
	// 	return false;
	// if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
	// 	return false;

	// NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	// NTV2Framesize	theFrameSize(inFrameSize);
	// if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	// {
	// 	if (IS_CHANNEL_INVALID(theChannel))
	// 		return false;
	// 	if (!GetFrameBufferSize(theChannel, theFrameSize))
	// 		return false;
	// 	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	// 		return false;
	// }

	// bool ok(true);
	// //	Calculate where AUX Inserter will read the data
	// const ULWord	frameNumber (inFrameNumber + 1);	//	Start at beginning of next frame (then subtract offset later)
	// ULWord	frameLocation (::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	// bool quadEnabled(false), quadQuadEnabled(false);
	// GetQuadFrameEnable(quadEnabled, inChannel);
	// GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	// if (quadEnabled)
	// 	frameLocation *= 4;
	// if (quadQuadEnabled)
	// 	frameLocation *= 4;

	// ULWord			F1Offset(0);
	// if (ok) ok = ReadRegister (kVRegAuxField1Offset, F1Offset);
	// const ULWord	AUXStartMemory (frameLocation - F1Offset);
	// if (ok) ok = SetAuxInsField1StartAddr (*this, inSDIOutput, AUXStartMemory);
	// if (ok) ok = SetAuxInsField1Bytes (*this, inSDIOutput, inF1Size);
	// return ok;
	
	return false;
}

bool CNTV2Card::AuxInsertSetField2ReadParams (const UWord inSDIOutput, const ULWord inFrameNumber, const ULWord inF2Size,
												const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	// if (!IsSupported(kDeviceCanDoPlayback))
	// 	return false;
	// if (!IsSupported(kDeviceCanDoCustomAux))
	// 	return false;
	// if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
	// 	return false;

	// NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inSDIOutput));
	// NTV2Framesize	theFrameSize(inFrameSize);
	// if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	// {
	// 	if (IS_CHANNEL_INVALID(theChannel))
	// 		return false;
	// 	if (!GetFrameBufferSize(theChannel, theFrameSize))
	// 		return false;
	// 	if (!NTV2_IS_VALID_8MB_FRAMESIZE(theFrameSize))
	// 		return false;
	// }

	// bool ok(true);
	// //	Calculate where AUX Inserter will read the data
	// const ULWord	frameNumber (inFrameNumber + 1);	//	Start at beginning of next frame (then subtract offset later)
	// ULWord	frameLocation (::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);
	// bool quadEnabled(false), quadQuadEnabled(false);
	// GetQuadFrameEnable(quadEnabled, inChannel);
	// GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	// if (quadEnabled)
	// 	frameLocation *= 4;
	// if (quadQuadEnabled)
	// 	frameLocation *= 4;

	// ULWord			F2Offset(0);
	// if (ok) ok = ReadRegister (kVRegAuxField2Offset, F2Offset);
	// const ULWord	AUXStartMemory (frameLocation - F2Offset);
	// if (ok) ok = SetAuxInsField2StartAddr (*this, inSDIOutput, AUXStartMemory);
	// if (ok) ok = SetAuxInsField2Bytes (*this, inSDIOutput, inF2Size);
	// return ok;
	
	return false;
}

bool CNTV2Card::AuxInsertSetIPParams (const UWord inSDIOutput, const UWord auxChannel, const ULWord payloadID, const ULWord ssrc)
{
	// bool ok(false);

	// if (IsSupported(kDeviceCanDoIP))
	// {
	// 	ok = SetAuxInsIPChannel (*this, inSDIOutput, auxChannel);
	// 	if (ok) ok = SetAuxInsRtpPayloadID (*this, inSDIOutput, payloadID);
	// 	if (ok) ok = SetAuxInsRtpSSRC (*this, inSDIOutput, ssrc);
	// }
	// return ok;
		
	return false;
}

bool CNTV2Card::AuxInsertGetReadInfo (const UWord inSDIOutput, uint64_t & outF1StartAddr, uint64_t & outF2StartAddr)
{
	// outF1StartAddr = outF2StartAddr = 0;
	// if (!IsSupported(kDeviceCanDoPlayback))
	// 	return false;
	// if (!IsSupported(kDeviceCanDoCustomAux))
	// 	return false;
	// if (IS_OUTPUT_SPIGOT_INVALID(inSDIOutput))
	// 	return false;
	// return GetAuxInsStartAddrs (*this, inSDIOutput, outF1StartAddr, outF2StartAddr);
	
	return false;
}


/////////////////////////////////////////////
/////////////	AUX EXTRACTOR	/////////////
/////////////////////////////////////////////


static bool SetAuxExtProgressive (CNTV2Card & inDevice, const UWord inHDMIInput, bool bEnable)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtControl), bEnable ? 1 : 0, maskAuxSetProgressive, shiftAuxSetProgressive);
}

static bool IsAuxExtProgressive (CNTV2DriverInterface & inDevice, const UWord inHDMIInput, bool & outIsProgressive)
{
	return inDevice.ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtControl), outIsProgressive, maskAuxSetProgressive, shiftAuxSetProgressive);
}

static bool SetAuxExtFilterMode (CNTV2Card & inDevice, const UWord inHDMIInput, bool bInclude)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtControl), bInclude ? 1 : 0, maskAuxFilterInvert, shiftAuxFilterInvert);
}	

static bool IsAuxExtFilterModeInclude (CNTV2DriverInterface & inDevice, const UWord inHDMIInput, bool & outIsInclude)
{
	return inDevice.ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtControl), outIsInclude, maskAuxFilterInvert, shiftAuxFilterInvert);
}

static bool SetAuxExtSynchro (CNTV2Card & inDevice, const UWord inHDMIInput)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtControl), 0x1, maskAuxSyncro, shiftAuxSyncro);
}

static bool GetAuxExtField1StartAddr (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t & outAddr)
{
	return inDevice.ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtField1StartAddress), outAddr);
}

static bool SetAuxExtField1StartAddr (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t addr)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtField1StartAddress), addr);
}

static bool GetAuxExtField1EndAddr (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t & outAddr)
{
	return inDevice.ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtField1EndAddress), outAddr);
}

static bool SetAuxExtField1EndAddr (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t addr)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtField1EndAddress), addr);
}

static bool GetAuxExtField2StartAddr (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t & outAddr)
{
	return inDevice.ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtField2StartAddress), outAddr);
}

static bool SetAuxExtField2StartAddr (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t addr)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtField2StartAddress), addr);
}

static bool GetAuxExtField1Status (CNTV2DriverInterface & inDevice, const UWord inHDMIInput, ULWord & outF1Status)
{
	return inDevice.ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtField1Status), outF1Status);
}

static bool GetAuxExtField2Status (CNTV2DriverInterface & inDevice, const UWord inHDMIInput, ULWord & outF2Status)
{
	return inDevice.ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtField2Status), outF2Status);
}

static bool IsAuxExtOverrun (CNTV2DriverInterface & inDevice, const UWord inHDMIInput, bool & outIsOverrun)
{
	return inDevice.ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtTotalStatus), outIsOverrun, maskAuxTotalOverrun, shiftAuxTotalOverrun);
}

static bool SetAuxExtField1StartLine (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t lineNumber)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtFieldVBLStartLine), lineNumber, maskAuxField1StartLine, shiftAuxField1StartLine);
}

static bool SetAuxExtField2StartLine (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t lineNumber)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtFieldVBLStartLine), lineNumber, maskAuxField2StartLine, shiftAuxField2StartLine);
}

static bool SetAuxExtTotalFrameLines (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t totalFrameLines)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtTotalFrameLines), totalFrameLines, maskAuxTotalFrameLines, shiftAuxTotalFrameLines);
}

static bool SetAuxExtFidLow (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t lineNumber)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtFID), lineNumber, maskAuxFIDLow, shiftAuxFIDLow);
}

static bool SetAuxExtFidHi (CNTV2Card & inDevice, const UWord inHDMIInput, uint32_t lineNumber)
{
	return inDevice.WriteRegister(AuxExtRegNum(inHDMIInput, regAuxExtFID), lineNumber, maskAuxFIDHi, shiftAuxFIDHi);
}

bool CNTV2Card::AuxExtractInit (const UWord inHDMIInput, const NTV2Channel inChannel, const NTV2Standard inStandard)
{
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inHDMIInput));
	NTV2Standard	theStandard (inStandard);
	if (!NTV2_IS_VALID_STANDARD(theStandard))
	{
		if (IS_CHANNEL_INVALID(theChannel))
			return false;
		if (!GetStandard(theStandard, theChannel))
			return false;
		if (!NTV2_IS_VALID_STANDARD(theStandard))
			return false;
	}

	const AUXExtractorInitParams &	extractorParams (extractorInitParamsTable[theStandard]);
	bool ok(true);
	if (ok) ok = SetAuxExtProgressive (*this, inHDMIInput, NTV2_IS_PROGRESSIVE_STANDARD(theStandard));
	if (ok) ok = SetAuxExtField1StartLine (*this, inHDMIInput, extractorParams.field1StartLine);
	if (ok) ok = SetAuxExtField2StartLine (*this, inHDMIInput, extractorParams.field2StartLine);
	if (ok) ok = SetAuxExtTotalFrameLines (*this, inHDMIInput, extractorParams.totalLines);
	if (ok) ok = SetAuxExtFidLow (*this, inHDMIInput, extractorParams.fidLow);
	if (ok) ok = SetAuxExtFidHi (*this, inHDMIInput, extractorParams.fidHigh);
	if (ok) ok = AuxExtractSetPacketFilters (inHDMIInput, AuxExtractGetDefaultPacketFilters());
	if (ok) ok = SetAuxExtSynchro (*this, inHDMIInput);
	if (ok) ok = SetAuxExtField1StartAddr (*this, inHDMIInput, 0);
	if (ok) ok = SetAuxExtField1EndAddr (*this, inHDMIInput, 0);
	if (ok) ok = SetAuxExtField2StartAddr (*this, inHDMIInput, 0);
	return ok;
}

bool CNTV2Card::AuxExtractSetEnable (const UWord inHDMIInput, const bool inIsEnabled)
{
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	bool ok(true);
	if (ok) ok = WriteRegister (AuxExtRegNum(inHDMIInput, regAuxExtControl), inIsEnabled ? 0 : 1, maskAuxDisableExtractor, shiftAuxDisableExtractor);
	return ok;
}

bool CNTV2Card::AuxExtractIsEnabled (const UWord inHDMIInput, bool & outIsRunning)
{
	outIsRunning = false;
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	ULWord	value(0);
	if (!ReadRegister(AuxExtRegNum(inHDMIInput, regAuxExtControl), value))
		return false;
	outIsRunning = (value & maskAuxDisableExtractor) ? false : true;
	return true;
}

bool CNTV2Card::AuxExtractSetWriteParams (const UWord inHDMIInput, const ULWord inFrameNumber,
											const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inHDMIInput));
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

	//	Calculate where AUX Extractor will put the data...
	bool			ok					(true);
	const ULWord	nextFrame			(inFrameNumber + 1);	//	This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord			endOfFrameLocation	(::NTV2FramesizeToByteCount(theFrameSize) * nextFrame);

	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		endOfFrameLocation *= 4;
	if (quadQuadEnabled)
		endOfFrameLocation *= 4;

	ULWord F1Offset(0), F2Offset(0);
	if (ok) ok = GetAuxOffsets (*this, F1Offset, F2Offset);

	const ULWord	AUXStartMemory	(endOfFrameLocation - F1Offset);
	const ULWord	AUXStopMemory	(endOfFrameLocation - F2Offset - 1);
	if (ok) ok = SetAuxExtField1StartAddr (*this, inHDMIInput, AUXStartMemory);
	if (ok) ok = SetAuxExtField1EndAddr (*this, inHDMIInput, AUXStopMemory);
	return ok;
}

bool CNTV2Card::AuxExtractSetField2WriteParams (const UWord inHDMIInput, const ULWord inFrameNumber,
												const NTV2Channel inChannel, const NTV2Framesize inFrameSize)
{
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	NTV2Channel		theChannel	(NTV2_IS_VALID_CHANNEL(inChannel) ? inChannel : NTV2Channel(inHDMIInput));
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

	//	Calculate where AUX Extractor will put the data...
	bool			ok					(true);
	const ULWord	frameNumber			(inFrameNumber + 1);	//	This is so the next calculation will point to the beginning of the next frame - subtract offset for memory start
	ULWord			frameLocation	(::NTV2FramesizeToByteCount(theFrameSize) * frameNumber);

	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		frameLocation *= 4;
	if (quadQuadEnabled)
		frameLocation *= 4;

	ULWord	F2Offset(0);
	if (ok) ok = ReadRegister(kVRegAncField2Offset, F2Offset);

	const ULWord	AUXStartMemory	(frameLocation - F2Offset);
	//const ULWord	AUXStopMemory	(frameLocation - 1);  
	if (ok) ok = SetAuxExtField2StartAddr (*this, inHDMIInput, AUXStartMemory);
	// For HDMI Aux, we do not have a register defined to store the end of Field 2 Aux Buffer.  
	// It is assumed to be the same size as field 1.
	// if (ok) ok = SetAuxExtField2EndAddr (*this, inHDMIInput, AUXStopMemory);
	return true;
}

bool CNTV2Card::AuxExtractGetWriteInfo (const UWord inHDMIInput,
										uint64_t & outF1StartAddr, uint64_t & outF1EndAddr,
										uint64_t & outF2StartAddr, uint64_t & outF2EndAddr)
{
	outF1StartAddr = outF1EndAddr = outF2StartAddr = outF2EndAddr = 0;
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	ULWord	startAddr(0), endAddr(0);
	bool ok = GetAuxExtField1StartAddr(*this, inHDMIInput, startAddr) && GetAuxExtField1EndAddr(*this, inHDMIInput, endAddr);
	outF1StartAddr = uint64_t(startAddr);
	outF1EndAddr = uint64_t(endAddr);
	//Question: Ok to return 0 for outF2EndAddr.. Answer: No
	ok = ok  &&  GetAuxExtField2StartAddr(*this, inHDMIInput, startAddr); // && GetAuxExtField2EndAddr(*this, inHDMIInput, endAddr);
	outF2StartAddr = uint64_t(startAddr);
	//For HDMI Aux, we are not storing this value in a register.  Calcuate based on the same buffer size as Field 1
	outF2EndAddr = uint64_t(startAddr) + (outF1EndAddr - outF1StartAddr);
	return ok;
}

bool CNTV2Card::AuxExtractGetPacketFilters (const UWord inHDMIInput, NTV2DIDSet & outFilters)
{
	outFilters.clear();
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	const ULWord	firstIgnoreRegNum	(AuxExtRegNum(inHDMIInput, regAuxExtPacketMask_First));
	for (ULWord regNdx(0);	regNdx < kNumDIDRegisters;	regNdx++)
	{
		ULWord	regValue	(0);
		ReadRegister (firstIgnoreRegNum + regNdx,  regValue);
		for (unsigned regByte(0);  regByte < 4;	 regByte++)
		{
			const NTV2DID	theDID	((regValue >> (regByte*8)) & 0x000000FF);
			if (theDID)
				outFilters.insert(theDID);
		}
	}

	return true;
}

bool CNTV2Card::AuxExtractSetPacketFilters (const UWord inHDMIInput, const NTV2DIDSet & inDIDs)
{
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	const ULWord		firstIgnoreRegNum	(AuxExtRegNum(inHDMIInput, regAuxExtPacketMask_First));
	NTV2DIDSetConstIter iter				(inDIDs.begin());

	for (ULWord regNdx(0);	regNdx < kNumDIDRegisters;	regNdx++)
	{
		ULWord	regValue	(0);
		for (unsigned regByte(0);  regByte < 4;	 regByte++)
		{
			const NTV2DID	theDID	(iter != inDIDs.end()  ?  *iter++  :  0);
			regValue |= (ULWord(theDID) << (regByte*8));
		}
		WriteRegister (firstIgnoreRegNum + regNdx,	regValue);
	}
	return true;
}


bool CNTV2Card::AuxExtractSetFilterInclusionMode (const UWord inHDMIInput, const bool inIncludePackets)
{
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;	

	bool ok = SetAuxExtFilterMode(*this, inHDMIInput, inIncludePackets);
	return ok;

}

bool CNTV2Card::AuxExtractGetFilterInclusionMode (const UWord inHDMIInput, bool & outIncludePackets)
{
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;	

	bool ok = IsAuxExtFilterModeInclude(*this, inHDMIInput, outIncludePackets);
	return ok;
}


bool CNTV2Card::AuxExtractGetField1Size (const UWord inHDMIInput, ULWord & outF1Size)
{
	outF1Size = 0;
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	bool	ok			(true);
	ULWord	regValue	(0);

	ok = GetAuxExtField1Status(*this, inHDMIInput, regValue);
	if (!ok || ((regValue & maskAuxFieldOverrun) != 0))
		return false;
	outF1Size = regValue & maskAuxFieldBytesIn;

	return ok;
}

bool CNTV2Card::AuxExtractGetField2Size (const UWord inHDMIInput, ULWord & outF2Size)
{
	outF2Size = 0;
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	bool	ok			(true);
	ULWord	regValue	(0);

	ok = GetAuxExtField2Status(*this, inHDMIInput, regValue);
	if (!ok || ((regValue & maskAuxFieldOverrun) != 0))
		return false;
	outF2Size = regValue & maskAuxFieldBytesIn;

	return ok;
}

bool CNTV2Card::AuxExtractGetBufferOverrun (const UWord inHDMIInput, bool & outIsOverrun, const UWord inField)
{
	outIsOverrun = false;
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;
	if (inField > 2)
		return false;
	ULWord status(0);
	switch (inField)
	{
		case 0: return IsAuxExtOverrun (*this, inHDMIInput, outIsOverrun);
		case 1: if (!GetAuxExtField1Status(*this, inHDMIInput, status))
					return false;
				outIsOverrun = (status & maskAuxFieldOverrun) ? true : false;
				return true;
		case 2: if (!GetAuxExtField2Status(*this, inHDMIInput, status))
					return false;
				outIsOverrun = (status & maskAuxFieldOverrun) ? true : false;
				return true;
		default: break;
	}
	return false;
}

bool CNTV2Card::AuxExtractIsProgressive (const UWord inHDMIInput, bool & outIsProgressive)
{
	if (!IsSupported(kDeviceCanDoCapture))
		return false;
	if (!IsSupported(kDeviceCanDoCustomAux))
		return false;
	if (IS_HDMI_INPUT_SPIGOT_INVALID(inHDMIInput))
		return false;

	return IsAuxExtProgressive (*this, inHDMIInput, outIsProgressive);
}



/////////////////////////////////////////////////
/////////////	STATIC FUNCTIONS	/////////////
/////////////////////////////////////////////////

UWord CNTV2Card::AuxExtractGetMaxNumPacketFilters (void)
{
	static const ULWord kNumDIDsPerRegister (4);
	return UWord(kNumDIDsPerRegister * kNumDIDRegisters);
}

NTV2DIDSet CNTV2Card::AuxExtractGetDefaultPacketFilters (void)
{						
	static const NTV2DID	sDefaultHDDIDs[]	=	{0x02}; 
	// Packet type 0x02: Audio Sample (L-PCM and IEC 61937 compressed formats)

	NTV2DIDSet	result;
	const NTV2DID * pDIDArray (sDefaultHDDIDs);
	for (unsigned ndx(0);  pDIDArray[ndx];	ndx++)
		result.insert(pDIDArray[ndx]);

	return result;
}
