/**
	@file		ntv2enhancedcsc.cpp
	@brief		Implementation of CNTV2EnhancedCSC class.
	@copyright	(C) 2015-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2publicinterface.h"
#include "ntv2enhancedcsc.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"


static const ULWord	gChannelToEnhancedCSCRegNum []		= {	kRegEnhancedCSC1Mode,	kRegEnhancedCSC2Mode,	kRegEnhancedCSC3Mode,	kRegEnhancedCSC4Mode,
															kRegEnhancedCSC5Mode,	kRegEnhancedCSC6Mode,	kRegEnhancedCSC7Mode,	kRegEnhancedCSC8Mode,	0};

static const double kTwo24th = (double)(1 << 24);


static ULWord ConvertCoeffDoubleToULWord (const double inCoefficient)
{
	return ((ULWord)(inCoefficient * kTwo24th));
}


static double ConvertCoeffULWordToDouble (const ULWord inCoefficient)
{
	return ((double) inCoefficient / kTwo24th);
}


bool CNTV2EnhancedCSC::SetInputPixelFormat (const NTV2EnhancedCSCPixelFormat inPixelFormat)
{
	switch (inPixelFormat)
	{
	case NTV2_Enhanced_CSC_Pixel_Format_RGB444:
	case NTV2_Enhanced_CSC_Pixel_Format_YCbCr444:
	case NTV2_Enhanced_CSC_Pixel_Format_YCbCr422:
		mInputPixelFormat = inPixelFormat;
		break;
	default:
		return false;
		break;
	}

	return true;
}	//	SetInputPixelFormat


bool CNTV2EnhancedCSC::SetOutputPixelFormat (const NTV2EnhancedCSCPixelFormat inPixelFormat)
{
	switch (inPixelFormat)
	{
	case NTV2_Enhanced_CSC_Pixel_Format_RGB444:
	case NTV2_Enhanced_CSC_Pixel_Format_YCbCr444:
	case NTV2_Enhanced_CSC_Pixel_Format_YCbCr422:
		mOutputPixelFormat = inPixelFormat;
		break;
	default:
		return false;
		break;
	}

	return true;
}	//	SetOutputPixelFormat


bool CNTV2EnhancedCSC::SetChromaFilterSelect (const NTV2EnhancedCSCChromaFilterSelect inChromaFilterSelect)
{
	switch (inChromaFilterSelect)
	{
	case NTV2_Enhanced_CSC_Chroma_Filter_Select_Full:
	case NTV2_Enhanced_CSC_Chroma_Filter_Select_Simple:
	case NTV2_Enhanced_CSC_Chroma_Filter_Select_None:
		mChromaFilterSelect = inChromaFilterSelect;
		break;
	default:
		return false;
		break;
	}

	return true;
}	//	SetChromaFilterSelect


bool CNTV2EnhancedCSC::SetChromaEdgeControl (const NTV2EnhancedCSCChromaEdgeControl inChromaEdgeControl)
{
	switch (inChromaEdgeControl)
	{
	case NTV2_Enhanced_CSC_Chroma_Filter_Select_Full:
	case NTV2_Enhanced_CSC_Chroma_Filter_Select_Simple:
	case NTV2_Enhanced_CSC_Chroma_Filter_Select_None:
		mChromaEdgeControl = inChromaEdgeControl;
		break;
	default:
		return false;
		break;
	}

	return true;
}	//	SetChromaEdgeControl


bool CNTV2EnhancedCSC::SetKeySource (const NTV2EnhancedCSCKeySource inKeySource)
{
	switch (inKeySource)
	{
	case NTV2_Enhanced_CSC_Key_Source_Key_Input:
	case NTV2_Enhanced_CSC_Key_Spurce_A0_Input:
		mKeySource = inKeySource;
		break;
	default:
		return false;
		break;
	}

	return true;
}	//	SetKeySource


bool CNTV2EnhancedCSC::SetKeyOutputRange (const NTV2EnhancedCSCKeyOutputRange inKeyOutputRange)
{
	switch (inKeyOutputRange)
	{
	case NTV2_Enhanced_CSC_Key_Output_Range_Full:
	case NTV2_Enhanced_CSC_Key_Output_Range_SMPTE:
		mKeyOutputRange = inKeyOutputRange;
		break;
	default:
		return false;
		break;
	}

	return true;
}	//	SetKeyOutputRange


bool CNTV2EnhancedCSC::SetKeyInputOffset (const int16_t inKeyInputOffset)
{
	//	To do: add some parameter checking
	mKeyInputOffset = inKeyInputOffset;

	return true;
}	//	SetKeyInputOffset


bool CNTV2EnhancedCSC::SetKeyOutputOffset (const uint16_t inKeyOutputOffset)
{
	//	To do: add some parameter checking
	mKeyOutputOffset = inKeyOutputOffset;

	return true;
}	//	SetKeyOutputOffset


bool CNTV2EnhancedCSC::SetKeyGain (const double inKeyGain)
{
	//	To do: add some parameter checking
	mKeyGain = inKeyGain;

	return true;
}	//	SetKeyGain


bool CNTV2EnhancedCSC::SendToHardware (CNTV2Card & inDevice, const NTV2Channel inChannel) const
{
	if (!inDevice.IsOpen ())
		return false;

	ULWord cscBaseAddress = gChannelToEnhancedCSCRegNum [inChannel];
	ULWord cscRegs [kRegNumEnhancedCSCRegisters];

	//	Read-modify-write only the relevent control bits
	if (!inDevice.ReadRegister (cscBaseAddress, &cscRegs [0]))
		return false;

	cscRegs [0] = (cscRegs [0] & ~kK2RegMaskEnhancedCSCInputPixelFormat)   | (mInputPixelFormat   << kK2RegShiftEnhancedCSCInputPixelFormat);
	cscRegs [0] = (cscRegs [0] & ~kK2RegMaskEnhancedCSCOutputPixelFormat)  | (mOutputPixelFormat  << kK2RegShiftEnhancedCSCOutputPixelFormat);
	cscRegs [0] = (cscRegs [0] & ~kK2RegMaskEnhancedCSCChromaFilterSelect) | (mChromaFilterSelect << kK2RegShiftEnhancedCSCChromaFilterSelect);
	cscRegs [0] = (cscRegs [0] & ~kK2RegMaskEnhancedCSCChromaEdgeControl)  | (mChromaEdgeControl  << kK2RegShiftEnhancedCSCChromaEdgeControl);

	cscRegs [1] = (Matrix ().GetOffset (NTV2CSCOffsetIndex_Pre1) << 16) | Matrix ().GetOffset (NTV2CSCOffsetIndex_Pre0);

	cscRegs [2] = Matrix ().GetOffset (NTV2CSCOffsetIndex_Pre2);

	cscRegs [3] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_A0));

	cscRegs [4] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_A1));

	cscRegs [5] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_A2));

	cscRegs [6] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_B0));

	cscRegs [7] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_B1));

	cscRegs [8] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_B2));

	cscRegs [9] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_C0));

	cscRegs [10] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_C1));

	cscRegs [11] = ConvertCoeffDoubleToULWord (Matrix ().GetCoefficient (NTV2CSCCoeffIndex_C2));

	cscRegs [12] = (Matrix ().GetOffset (NTV2CSCOffsetIndex_PostB) << 16) | Matrix ().GetOffset (NTV2CSCOffsetIndex_PostA);

	cscRegs [13] = Matrix ().GetOffset (NTV2CSCOffsetIndex_PostC);

	cscRegs [14] = ((mKeyOutputRange  & kK2RegMaskEnhancedCSCKeyOutputRange)  << kK2RegShiftEnhancedCSCKeyOutputRange) |
				   ((mKeySource       & kK2RegMaskEnhancedCSCKeySource)       << kK2RegShiftEnhancedCSCKeySource);

	cscRegs [15] = (mKeyOutputOffset << 16) | mKeyInputOffset;

	cscRegs [16] = ((ULWord)(mKeyGain * 4096.0));

	NTV2RegInfo regInfo;
	NTV2RegisterWrites regVector;
	for (int i = 0;  i < kRegNumEnhancedCSCRegisters;  i++)
	{
		regInfo.registerNumber	= cscBaseAddress + i;
		regInfo.registerValue	= cscRegs [i];
		regInfo.registerMask	= 0xFFFFFFFF;
		regInfo.registerShift	= 0;

		regVector.push_back (regInfo);
	}

	if (!inDevice.WriteRegisters (regVector))
		return false;

	return true;
}	//	SendToHardware


bool CNTV2EnhancedCSC::GetFromHardware (CNTV2Card & inDevice, const NTV2Channel inChannel)
{
	if (!inDevice.IsOpen ())
		return false;

	ULWord cscBaseAddress = gChannelToEnhancedCSCRegNum [inChannel];
	ULWord cscRegs [kRegNumEnhancedCSCRegisters];

	NTV2RegNumSet regSet;
	for (int i = 0;  i < kRegNumEnhancedCSCRegisters;  i++)
	{
		regSet.insert (cscBaseAddress + i);
	}

	NTV2RegisterValueMap regMap;
	if (!inDevice.ReadRegisters (regSet, regMap))
		return false;

	//	Check that all registers were read successfully
	if (regMap.size () != kRegNumEnhancedCSCRegisters)
		return false;

	int i = 0;
	for (NTV2RegValueMapConstIter iter = regMap.begin ();  iter != regMap.end ();  ++iter)
	{
		cscRegs [i++] = iter->second;
	}

	mInputPixelFormat	= (NTV2EnhancedCSCPixelFormat)			((cscRegs [0] & kK2RegMaskEnhancedCSCInputPixelFormat)   >> kK2RegShiftEnhancedCSCInputPixelFormat);
	mOutputPixelFormat	= (NTV2EnhancedCSCPixelFormat)			((cscRegs [0] & kK2RegMaskEnhancedCSCOutputPixelFormat)  >> kK2RegShiftEnhancedCSCOutputPixelFormat);
	mChromaFilterSelect	= (NTV2EnhancedCSCChromaFilterSelect)	((cscRegs [0] & kK2RegMaskEnhancedCSCChromaFilterSelect) >> kK2RegShiftEnhancedCSCChromaFilterSelect);
	mChromaEdgeControl	= (NTV2EnhancedCSCChromaEdgeControl)	((cscRegs [0] & kK2RegMaskEnhancedCSCChromaEdgeControl)  >> kK2RegShiftEnhancedCSCChromaEdgeControl);

	Matrix ().SetOffset (NTV2CSCOffsetIndex_Pre0, cscRegs [1] & 0xFFFF);
	Matrix ().SetOffset (NTV2CSCOffsetIndex_Pre1, cscRegs [1] >> 16);

	Matrix ().SetOffset (NTV2CSCOffsetIndex_Pre1, cscRegs [2] & 0xFFFF);

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_A0, ConvertCoeffULWordToDouble (cscRegs [3]));

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_A1, ConvertCoeffULWordToDouble (cscRegs [4]));

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_A2, ConvertCoeffULWordToDouble (cscRegs [5]));

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_B0, ConvertCoeffULWordToDouble (cscRegs [6]));

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_B1, ConvertCoeffULWordToDouble (cscRegs [7]));

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_B2, ConvertCoeffULWordToDouble (cscRegs [8]));

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_C0, ConvertCoeffULWordToDouble (cscRegs [9]));

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_C1, ConvertCoeffULWordToDouble (cscRegs [10]));

	Matrix ().SetCoefficient (NTV2CSCCoeffIndex_C2, ConvertCoeffULWordToDouble (cscRegs [11]));

	Matrix ().SetOffset (NTV2CSCOffsetIndex_PostA, cscRegs [12] & 0xFFFF);
	Matrix ().SetOffset (NTV2CSCOffsetIndex_PostB, cscRegs [12] >> 16);

	Matrix ().SetOffset (NTV2CSCOffsetIndex_PostC, cscRegs [13] & 0xFFFF);

	mKeyOutputRange	= (NTV2EnhancedCSCKeyOutputRange)	((cscRegs [14] & kK2RegMaskEnhancedCSCKeyOutputRange)	>> kK2RegShiftEnhancedCSCKeyOutputRange);
	mKeySource		= (NTV2EnhancedCSCKeySource)		((cscRegs [14] & kK2RegMaskEnhancedCSCKeySource)		>> kK2RegShiftEnhancedCSCKeySource);

	mKeyInputOffset		= (int16_t)  (cscRegs [15] & 0xFFFF);
	mKeyOutputOffset	= (uint16_t) (cscRegs [15] >> 16);

	mKeyGain = ((double) cscRegs [16] / 4096.0);

	return true;
}	//	GetFromaHardware

