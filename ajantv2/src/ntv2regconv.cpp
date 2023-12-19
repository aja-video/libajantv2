/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2regconv.cpp
	@brief		CNTV2Card UDC/UFC conversion API implementation.
	@copyright	(C) 2004-2023 AJA Video Systems, Inc.
**/

#include "ntv2card.h"


// kRegConversionControl
bool CNTV2Card::SetUpConvertMode (const NTV2UpConvertMode inValue)		{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskUpConvertMode,		kK2RegShiftUpConvertMode);}
bool CNTV2Card::GetUpConvertMode (NTV2UpConvertMode & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskUpConvertMode,		kK2RegShiftUpConvertMode);}
bool CNTV2Card::SetConverterOutStandard (const NTV2Standard inValue)	{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskConverterOutStandard, kK2RegShiftConverterOutStandard);}
bool CNTV2Card::GetConverterOutStandard (NTV2Standard & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskConverterOutStandard, kK2RegShiftConverterOutStandard);}
bool CNTV2Card::SetConverterOutRate (const NTV2FrameRate inValue)		{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskConverterOutRate,		kK2RegShiftConverterOutRate);}
bool CNTV2Card::GetConverterOutRate (NTV2FrameRate & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskConverterOutRate,		kK2RegShiftConverterOutRate);}
bool CNTV2Card::SetConverterInStandard (const NTV2Standard inValue)		{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskConverterInStandard,	kK2RegShiftConverterInStandard);}
bool CNTV2Card::GetConverterInStandard (NTV2Standard & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskConverterInStandard,	kK2RegShiftConverterInStandard);}
bool CNTV2Card::SetConverterInRate (const NTV2FrameRate inValue)		{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskConverterInRate,		kK2RegShiftConverterInRate);}
bool CNTV2Card::GetConverterInRate (NTV2FrameRate & outValue)			{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskConverterInRate,		kK2RegShiftConverterInRate);}
bool CNTV2Card::SetDownConvertMode (const NTV2DownConvertMode inValue)	{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskDownConvertMode,		kK2RegShiftDownConvertMode);}
bool CNTV2Card::GetDownConvertMode (NTV2DownConvertMode & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,		kK2RegMaskDownConvertMode,		kK2RegShiftDownConvertMode);}
bool CNTV2Card::SetIsoConvertMode (const NTV2IsoConvertMode inValue)	{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskIsoConvertMode,		kK2RegShiftIsoConvertMode);}
bool CNTV2Card::SetEnableConverter (const bool inValue)					{return WriteRegister (kRegConversionControl,	ULWord(inValue),		kK2RegMaskEnableConverter,		kK2RegShiftEnableConverter);}

bool CNTV2Card::GetEnableConverter (bool & outValue)
{
	ULWord ULWvalue = ULWord(outValue);
	bool retVal = ReadRegister (kRegConversionControl, ULWvalue, kK2RegMaskEnableConverter, kK2RegShiftEnableConverter);
	outValue = ULWvalue ? true : false;
	return retVal;
}

bool CNTV2Card::GetIsoConvertMode (NTV2IsoConvertMode & outValue)				{return CNTV2DriverInterface::ReadRegister (kRegConversionControl,	outValue,	kK2RegMaskIsoConvertMode,		kK2RegShiftIsoConvertMode);}
bool CNTV2Card::SetDeinterlaceMode (const ULWord inValue)						{return WriteRegister (kRegConversionControl,	ULWord(inValue),	kK2RegMaskDeinterlaceMode,		kK2RegShiftDeinterlaceMode);}
bool CNTV2Card::GetDeinterlaceMode (ULWord & outValue)							{return ReadRegister (kRegConversionControl,	outValue,			kK2RegMaskDeinterlaceMode,		kK2RegShiftDeinterlaceMode);}
bool CNTV2Card::SetConverterPulldown (const ULWord inValue)						{return WriteRegister (kRegConversionControl,	ULWord(inValue),	kK2RegMaskConverterPulldown,	kK2RegShiftConverterPulldown);}
bool CNTV2Card::GetConverterPulldown (ULWord & outValue)						{return ReadRegister (kRegConversionControl,	outValue,			kK2RegMaskConverterPulldown,	kK2RegShiftConverterPulldown);}
bool CNTV2Card::SetUCPassLine21 (const ULWord inValue)							{return WriteRegister (kRegConversionControl,	ULWord(inValue),	kK2RegMaskUCPassLine21,			kK2RegShiftUCPassLine21);}
bool CNTV2Card::GetUCPassLine21 (ULWord & outValue)								{return ReadRegister (kRegConversionControl,	outValue,			kK2RegMaskUCPassLine21,			kK2RegShiftUCPassLine21);}
bool CNTV2Card::SetUCAutoLine21 (const ULWord inValue)							{return WriteRegister (kRegConversionControl,	ULWord(inValue),	kK2RegMaskUCPassLine21,			kK2RegShiftUCAutoLine21);}
bool CNTV2Card::GetUCAutoLine21 (ULWord & outValue)								{return ReadRegister (kRegConversionControl,	outValue,			kK2RegMaskUCPassLine21,			kK2RegShiftUCAutoLine21);}


bool CNTV2Card::SetConversionMode (NTV2ConversionMode mode)
{
	NTV2Standard inStandard;
	NTV2Standard outStandard;
	bool isPulldown=false;
	bool isDeinterlace=false;

	///OK...shouldda been a table....started off as only 8 modes....
	switch ( mode )
	{
	case NTV2_1080i_5994to525_5994:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		break;
	case NTV2_1080i_2500to625_2500:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_625;
		break;
	case NTV2_720p_5994to525_5994:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_525;
		break;
	case NTV2_720p_5000to625_2500:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_625;
		break;
	case NTV2_525_5994to1080i_5994:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_1080;
		break;
	case NTV2_525_5994to720p_5994:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_720;
		break;
	case NTV2_625_2500to1080i_2500:
		inStandard = NTV2_STANDARD_625;
		outStandard = NTV2_STANDARD_1080;
		break;
	case NTV2_625_2500to720p_5000:
		inStandard = NTV2_STANDARD_625;
		outStandard = NTV2_STANDARD_720;
		break;
	case NTV2_720p_5000to1080i_2500:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_1080;
		break;
	case NTV2_720p_5994to1080i_5994:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_1080;
		break;
	case NTV2_720p_6000to1080i_3000:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_1080;
		break;
	case NTV2_1080i2398to525_2398:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		break;
	case NTV2_1080i2398to525_2997:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		isPulldown = true;
		break;
	case NTV2_1080i2400to525_2400:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		break;
	case NTV2_1080p2398to525_2398:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		break;
	case NTV2_1080p2398to525_2997:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		isPulldown = true;
		break;
	case NTV2_1080p2400to525_2400:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_525;
		break;
	case NTV2_1080i_2500to720p_5000:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_720;
		break;
	case NTV2_1080i_5994to720p_5994:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_720;
		break;
	case NTV2_1080i_3000to720p_6000:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_720;
		break;

	case NTV2_1080i_2398to720p_2398:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_720;
		break;
	case NTV2_720p_2398to1080i_2398:
		inStandard = NTV2_STANDARD_720;
		outStandard = NTV2_STANDARD_1080;
		break;
		
	case NTV2_525_2398to1080i_2398:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_1080;
		break;
		
	case NTV2_525_5994to525_5994:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_525;
		break;
	case NTV2_625_2500to625_2500:
		inStandard = NTV2_STANDARD_625;
		outStandard = NTV2_STANDARD_625;
		break;

	case NTV2_525_5994to525psf_2997:
		inStandard = NTV2_STANDARD_525;
		outStandard = NTV2_STANDARD_525;
		isDeinterlace = true;
		break;

	case NTV2_625_5000to625psf_2500:
		inStandard = NTV2_STANDARD_625;
		outStandard = NTV2_STANDARD_625;
		isDeinterlace = true;
		break;
		
	case NTV2_1080i_5000to1080psf_2500:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_1080;
		isDeinterlace = true;
		break;

	case NTV2_1080i_5994to1080psf_2997:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_1080;
		isDeinterlace = true;
		break;
		
	case NTV2_1080i_6000to1080psf_3000:
		inStandard = NTV2_STANDARD_1080;
		outStandard = NTV2_STANDARD_1080;
		isDeinterlace = true;
		break;

	case NTV2_1080p_3000to720p_6000:
		inStandard = NTV2_STANDARD_1080p;
		outStandard = NTV2_STANDARD_720;
		break;

	default:
		return false;
	}

	SetConverterInStandard(inStandard);
	SetConverterOutStandard(outStandard);
	if (GetNumSupported(kDeviceGetUFCVersion) == 2)
	{
		NTV2VideoFormat format = GetInputForConversionMode(mode);
		SetConverterInRate(GetNTV2FrameRateFromVideoFormat(format));
		format = GetOutputForConversionMode(mode);
		SetConverterOutRate(GetNTV2FrameRateFromVideoFormat(format));
	}
	SetConverterPulldown( isPulldown );
	SetDeinterlaceMode(isDeinterlace);

	return true;

}	//	SetK2ConversionMode


bool CNTV2Card::GetConversionMode(NTV2ConversionMode & outMode)
{
   NTV2Standard inStandard;
   NTV2Standard outStandard;

   GetConverterInStandard(inStandard);
   GetConverterOutStandard(outStandard);

   outMode = NTV2_CONVERSIONMODE_INVALID;

   switch (inStandard)
   {
   case NTV2_STANDARD_525:
	   if ( outStandard == NTV2_STANDARD_1080)
		   outMode = NTV2_525_5994to1080i_5994;
	   else	 if ( outStandard == NTV2_STANDARD_720)
		   outMode = NTV2_525_5994to720p_5994;
	   else	 if ( outStandard == NTV2_STANDARD_525)
		   outMode = NTV2_525_5994to525_5994;
	   break;
   case NTV2_STANDARD_625:
	   if ( outStandard == NTV2_STANDARD_1080)
		   outMode = NTV2_625_2500to1080i_2500;
	   else	 if ( outStandard == NTV2_STANDARD_720)
		   outMode = NTV2_625_2500to720p_5000;
	   else	 if ( outStandard == NTV2_STANDARD_625)
		   outMode = NTV2_625_2500to625_2500;
	   break;
   case NTV2_STANDARD_720:
	   if ( outStandard == NTV2_STANDARD_525)
		   outMode = NTV2_720p_5994to525_5994;
	   else if (outStandard == NTV2_STANDARD_625)
		   outMode = NTV2_720p_5000to625_2500;
	   break;
   case NTV2_STANDARD_1080:
	   if ( outStandard == NTV2_STANDARD_525)
		   outMode = NTV2_1080i_5994to525_5994;
	   else if (outStandard == NTV2_STANDARD_625)
		   outMode = NTV2_1080i_2500to625_2500;
	   break;

   case NTV2_STANDARD_1080p:
	   if ( outStandard == NTV2_STANDARD_720)
		   outMode = NTV2_1080p_3000to720p_6000;
	   break;

   default:
	   return false;
   }
	 return true;
}


/////////////////////////////////////////////////////////////////////
// Analog
#if !defined(R2_DEPRECATE)
bool CNTV2Card::SetAnalogInputADCMode				(const NTV2LSVideoADCMode inValue)			{return WriteRegister (kRegAnalogInputControl,	ULWord(inValue),		kRegMaskAnalogInputADCMode,				kRegShiftAnalogInputADCMode);}
bool CNTV2Card::GetAnalogInputADCMode				(NTV2LSVideoADCMode & outValue)				{return CNTV2DriverInterface::ReadRegister	(kRegAnalogInputControl,	outValue,	kRegMaskAnalogInputADCMode,				kRegShiftAnalogInputADCMode);}
#endif

/////////////////////////////////////////////////////////
// LHI/IoExpress related methods
bool CNTV2Card::SetLHIVideoDACStandard (const NTV2Standard inValue)		{return WriteRegister (kRegAnalogOutControl, ULWord(inValue), kLHIRegMaskVideoDACStandard, kLHIRegShiftVideoDACStandard);}
bool CNTV2Card::GetLHIVideoDACStandard (NTV2Standard & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegAnalogOutControl, outValue, kLHIRegMaskVideoDACStandard, kLHIRegShiftVideoDACStandard);}
bool CNTV2Card::SetLHIVideoDACMode (const NTV2LHIVideoDACMode inValue)	{return WriteRegister (kRegAnalogOutControl, ULWord(inValue), kLHIRegMaskVideoDACMode, kLHIRegShiftVideoDACMode);}
bool CNTV2Card::GetLHIVideoDACMode (NTV2LHIVideoDACMode & outValue)		{return CNTV2DriverInterface::ReadRegister (kRegAnalogOutControl, outValue, kLHIRegMaskVideoDACMode, kLHIRegShiftVideoDACMode);}

// overloaded - alternately takes NTV2K2VideoDACMode instead of NTV2LHIVideoDACMode
bool CNTV2Card::SetLHIVideoDACMode(const NTV2VideoDACMode inValue)
{
	switch(inValue)
	{
		case NTV2_480iRGB:					return SetLHIVideoDACMode(NTV2LHI_480iRGB)					&&	SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iYPbPrSMPTE:			return SetLHIVideoDACMode(NTV2LHI_480iYPbPrSMPTE)			&&	SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iYPbPrBetacam525:		return SetLHIVideoDACMode(NTV2LHI_480iYPbPrBetacam525)		&&	SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iYPbPrBetacamJapan:	return SetLHIVideoDACMode(NTV2LHI_480iYPbPrBetacamJapan)	&&	SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iNTSC_US_Composite:	return SetLHIVideoDACMode(NTV2LHI_480iNTSC_US_Composite)	&&	SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_480iNTSC_Japan_Composite: return SetLHIVideoDACMode(NTV2LHI_480iNTSC_Japan_Composite) &&	SetLHIVideoDACStandard(NTV2_STANDARD_525);
		case NTV2_576iRGB:					return SetLHIVideoDACMode(NTV2LHI_576iRGB)					&&	SetLHIVideoDACStandard(NTV2_STANDARD_625);
		case NTV2_576iYPbPrSMPTE:			return SetLHIVideoDACMode(NTV2LHI_576iYPbPrSMPTE)			&&	SetLHIVideoDACStandard(NTV2_STANDARD_625);
		case NTV2_576iPAL_Composite:		return SetLHIVideoDACMode(NTV2LHI_576iPAL_Composite)		&&	SetLHIVideoDACStandard(NTV2_STANDARD_625);
	
		case NTV2_1080iRGB:
		case NTV2_1080psfRGB:				return SetLHIVideoDACMode(NTV2LHI_1080iRGB)					&&	SetLHIVideoDACStandard(NTV2_STANDARD_1080);
	
		case NTV2_1080iSMPTE:
		case NTV2_1080psfSMPTE:				return SetLHIVideoDACMode(NTV2LHI_1080iSMPTE)				&&	SetLHIVideoDACStandard(NTV2_STANDARD_1080);
	
		case NTV2_720pRGB:					return SetLHIVideoDACMode(NTV2LHI_720pRGB)					&&	SetLHIVideoDACStandard(NTV2_STANDARD_720);
		case NTV2_720pSMPTE:				return SetLHIVideoDACMode(NTV2LHI_720pSMPTE)				&&	SetLHIVideoDACStandard(NTV2_STANDARD_720);
	
		// not yet supported
		case NTV2_1080iXVGA:
		case NTV2_1080psfXVGA:
		case NTV2_720pXVGA:
		case NTV2_2Kx1080RGB:
		case NTV2_2Kx1080SMPTE:
		case NTV2_2Kx1080XVGA:
		default:							break;
	}
	return false;
}


// overloaded - alternately returns NTV2K2VideoDACMode instead of NTV2LHIVideoDACMode
bool CNTV2Card::GetLHIVideoDACMode(NTV2VideoDACMode & outValue)
{
	NTV2LHIVideoDACMode lhiValue	(NTV2_MAX_NUM_LHIVideoDACModes);
	NTV2Standard		standard	(NTV2_STANDARD_UNDEFINED);
	bool				result		(GetLHIVideoDACMode(lhiValue)  &&  GetLHIVideoDACStandard(standard));
	
	if (result)
		switch(standard)
		{
			case NTV2_STANDARD_525:
				switch (lhiValue)
				{
					case NTV2LHI_480iRGB:					outValue = NTV2_480iRGB;					break;
					case NTV2LHI_480iYPbPrSMPTE:			outValue = NTV2_480iYPbPrSMPTE;				break;
					case NTV2LHI_480iYPbPrBetacam525:		outValue = NTV2_480iYPbPrBetacam525;		break;
					case NTV2LHI_480iYPbPrBetacamJapan:		outValue = NTV2_480iYPbPrBetacamJapan;		break;
					case NTV2LHI_480iNTSC_US_Composite:		outValue = NTV2_480iNTSC_US_Composite;		break;
					case NTV2LHI_480iNTSC_Japan_Composite:	outValue = NTV2_480iNTSC_Japan_Composite;	break;
					default:								result = false;								break;
				}
				break;
			
			case NTV2_STANDARD_625:
				switch (lhiValue)
				{
					case NTV2LHI_576iRGB:					outValue = NTV2_576iRGB;					break;
					case NTV2LHI_576iYPbPrSMPTE:			outValue = NTV2_576iYPbPrSMPTE;				break;
					case NTV2LHI_576iPAL_Composite:			outValue = NTV2_576iPAL_Composite;			break;
					default:								result = false;								break;
				}
				break;
			
			case NTV2_STANDARD_1080:
				switch (lhiValue)
				{
					case NTV2LHI_1080iRGB:					outValue = NTV2_1080iRGB;					break;	// also NTV2LHI_1080psfRGB
					case NTV2LHI_1080iSMPTE:				outValue = NTV2_1080iSMPTE;					break;	// also NTV2LHI_1080psfSMPTE
					default:								result = false;								break;
				}
				break;
				
			case NTV2_STANDARD_720:
				switch (lhiValue)
				{
					case NTV2LHI_720pRGB:					outValue = NTV2_720pRGB;					break;
					case NTV2LHI_720pSMPTE:					outValue = NTV2_720pSMPTE;					break;
					default:								result = false;								break;
				}
				break;
	
			case NTV2_STANDARD_1080p:
			case NTV2_STANDARD_2K:
			case NTV2_STANDARD_2Kx1080p:
			case NTV2_STANDARD_2Kx1080i:
			case NTV2_STANDARD_3840x2160p:
			case NTV2_STANDARD_4096x2160p:
			case NTV2_STANDARD_3840HFR:
			case NTV2_STANDARD_4096HFR:
			case NTV2_STANDARD_3840i:
			case NTV2_STANDARD_4096i:
			case NTV2_STANDARD_INVALID:
			default:
				result = false;
				break;
		}
	return result;
}


// ProcAmp controls.  Only work on boards with analog inputs.
bool CNTV2Card::WriteSDProcAmpControlsInitialized	(const ULWord inValue)		{return WriteRegister (kVRegProcAmpSDRegsInitialized,		inValue);}
bool CNTV2Card::WriteSDBrightnessAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefBrightness,	inValue);}
bool CNTV2Card::WriteSDContrastAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefContrast,		inValue);}
bool CNTV2Card::WriteSDSaturationAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefSaturation,	inValue);}
bool CNTV2Card::WriteSDHueAdjustment				(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefHue,			inValue);}
bool CNTV2Card::WriteSDCbOffsetAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefCbOffset,		inValue);}
bool CNTV2Card::WriteSDCrOffsetAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpStandardDefCrOffset,		inValue);}
bool CNTV2Card::WriteHDProcAmpControlsInitialized	(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHDRegsInitialized,		inValue);}
bool CNTV2Card::WriteHDBrightnessAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefBrightness,		inValue);}
bool CNTV2Card::WriteHDContrastAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefContrast,			inValue);}
bool CNTV2Card::WriteHDSaturationAdjustmentCb		(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefSaturationCb,		inValue);}
bool CNTV2Card::WriteHDSaturationAdjustmentCr		(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefSaturationCr,		inValue);}
bool CNTV2Card::WriteHDCbOffsetAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefCbOffset,			inValue);}
bool CNTV2Card::WriteHDCrOffsetAdjustment			(const ULWord inValue)		{return WriteRegister (kVRegProcAmpHighDefCrOffset,			inValue);}
bool CNTV2Card::ReadSDProcAmpControlsInitialized	(ULWord & outValue)			{return ReadRegister (kVRegProcAmpSDRegsInitialized,		outValue);}
bool CNTV2Card::ReadSDBrightnessAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefBrightness,	outValue);}
bool CNTV2Card::ReadSDContrastAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefContrast,		outValue);}
bool CNTV2Card::ReadSDSaturationAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefSaturation,	outValue);}
bool CNTV2Card::ReadSDHueAdjustment					(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefHue,			outValue);}
bool CNTV2Card::ReadSDCbOffsetAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefCbOffset,		outValue);}
bool CNTV2Card::ReadSDCrOffsetAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpStandardDefCrOffset,		outValue);}
bool CNTV2Card::ReadHDProcAmpControlsInitialized	(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHDRegsInitialized,		outValue);}
bool CNTV2Card::ReadHDBrightnessAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefBrightness,		outValue);}
bool CNTV2Card::ReadHDContrastAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefContrast,			outValue);}
bool CNTV2Card::ReadHDSaturationAdjustmentCb		(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefSaturationCb,		outValue);}
bool CNTV2Card::ReadHDSaturationAdjustmentCr		(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefSaturationCr,		outValue);}
bool CNTV2Card::ReadHDCbOffsetAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefCbOffset,			outValue);}
bool CNTV2Card::ReadHDCrOffsetAdjustment			(ULWord & outValue)			{return ReadRegister (kVRegProcAmpHighDefCrOffset,			outValue);}

// FS1 (and other?) ProcAmp functions
// ProcAmp controls.
bool CNTV2Card::WriteProcAmpC1YAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC1Y_C1CB,		inValue,	kFS1RegMaskProcAmpC1Y,		kFS1RegShiftProcAmpC1Y);}
bool CNTV2Card::WriteProcAmpC1CBAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC1Y_C1CB,		inValue,	kFS1RegMaskProcAmpC1CB,		kFS1RegShiftProcAmpC1CB);}
bool CNTV2Card::WriteProcAmpC1CRAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC1CR_C2CB,		inValue,	kFS1RegMaskProcAmpC1CR,		kFS1RegShiftProcAmpC1CR);}
bool CNTV2Card::WriteProcAmpC2CBAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC1CR_C2CB,		inValue,	kFS1RegMaskProcAmpC2CB,		kFS1RegShiftProcAmpC2CB);}
bool CNTV2Card::WriteProcAmpC2CRAdjustment			(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC2CROffsetY,	inValue,	kFS1RegMaskProcAmpC2CR,		kFS1RegShiftProcAmpC2CR);}
bool CNTV2Card::WriteProcAmpOffsetYAdjustment		(const ULWord inValue)		{return WriteRegister (kRegFS1ProcAmpC2CROffsetY,	inValue,	kFS1RegMaskProcAmpOffsetY,	kFS1RegShiftProcAmpOffsetY);}

bool CNTV2Card::ReadProcAmpC1YAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC1Y_C1CB,		outValue,	kFS1RegMaskProcAmpC1Y,		kFS1RegShiftProcAmpC1Y);}
bool CNTV2Card::ReadProcAmpC1CBAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC1Y_C1CB,		outValue,	kFS1RegMaskProcAmpC1CB,		kFS1RegShiftProcAmpC1CB);}
bool CNTV2Card::ReadProcAmpC1CRAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC1CR_C2CB,		outValue,	kFS1RegMaskProcAmpC1CR,		kFS1RegShiftProcAmpC1CR);}
bool CNTV2Card::ReadProcAmpC2CBAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC1CR_C2CB,		outValue,	kFS1RegMaskProcAmpC2CB,		kFS1RegShiftProcAmpC2CB);}
bool CNTV2Card::ReadProcAmpC2CRAdjustment			(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC2CROffsetY,	outValue,	kFS1RegMaskProcAmpC2CR,		kFS1RegShiftProcAmpC2CR);}
bool CNTV2Card::ReadProcAmpOffsetYAdjustment		(ULWord & outValue)			{return ReadRegister (kRegFS1ProcAmpC2CROffsetY,	outValue,	kFS1RegMaskProcAmpOffsetY,	kFS1RegShiftProcAmpOffsetY);}



/////////////////////////////////////////////////////////////////////
// Stereo Compressor
bool CNTV2Card::SetStereoCompressorOutputMode		(NTV2StereoCompressorOutputMode inValue)	{return WriteRegister (kRegStereoCompressor,	ULWord(inValue),		kRegMaskStereoCompressorOutputMode,		kRegShiftStereoCompressorOutputMode);}
bool CNTV2Card::GetStereoCompressorOutputMode		(NTV2StereoCompressorOutputMode & outValue) {return CNTV2DriverInterface::ReadRegister	(kRegStereoCompressor,	outValue,	kRegMaskStereoCompressorOutputMode,		kRegShiftStereoCompressorOutputMode);}
bool CNTV2Card::SetStereoCompressorFlipMode			(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipMode,		kRegShiftStereoCompressorFlipMode);}
bool CNTV2Card::GetStereoCompressorFlipMode			(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipMode,		kRegShiftStereoCompressorFlipMode);}
bool CNTV2Card::SetStereoCompressorFlipLeftHorz		(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipLeftHorz,	kRegShiftStereoCompressorFlipLeftHorz);}
bool CNTV2Card::GetStereoCompressorFlipLeftHorz		(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipLeftHorz,	kRegShiftStereoCompressorFlipLeftHorz);}
bool CNTV2Card::SetStereoCompressorFlipLeftVert		(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipLeftVert,	kRegShiftStereoCompressorFlipLeftVert);}
bool CNTV2Card::GetStereoCompressorFlipLeftVert		(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipLeftVert,	kRegShiftStereoCompressorFlipLeftVert);}
bool CNTV2Card::SetStereoCompressorFlipRightHorz	(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipRightHorz,	kRegShiftStereoCompressorFlipRightHorz);}
bool CNTV2Card::GetStereoCompressorFlipRightHorz	(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipRightHorz,	kRegShiftStereoCompressorFlipRightHorz);}
bool CNTV2Card::SetStereoCompressorFlipRightVert	(const ULWord inValue)						{return WriteRegister (kRegStereoCompressor,	inValue,				kRegMaskStereoCompressorFlipRightVert,	kRegShiftStereoCompressorFlipRightVert);}
bool CNTV2Card::GetStereoCompressorFlipRightVert	(ULWord & outValue)							{return ReadRegister  (kRegStereoCompressor,	outValue,				kRegMaskStereoCompressorFlipRightVert,	kRegShiftStereoCompressorFlipRightVert);}
bool CNTV2Card::SetStereoCompressorStandard			(const NTV2Standard inValue)				{return WriteRegister (kRegStereoCompressor,	ULWord(inValue),		kRegMaskStereoCompressorFormat,			kRegShiftStereoCompressorFormat);}
bool CNTV2Card::GetStereoCompressorStandard			(NTV2Standard & outValue)					{return CNTV2DriverInterface::ReadRegister	(kRegStereoCompressor,	outValue,	kRegMaskStereoCompressorFormat,			kRegShiftStereoCompressorFormat);}
bool CNTV2Card::SetStereoCompressorLeftSource		(const NTV2OutputCrosspointID inValue)		{return WriteRegister (kRegStereoCompressor,	ULWord(inValue),		kRegMaskStereoCompressorLeftSource,		kRegShiftStereoCompressorLeftSource);}
bool CNTV2Card::GetStereoCompressorLeftSource		(NTV2OutputCrosspointID & outValue)			{return CNTV2DriverInterface::ReadRegister	(kRegStereoCompressor,	outValue,	kRegMaskStereoCompressorLeftSource,		kRegShiftStereoCompressorLeftSource);}
bool CNTV2Card::SetStereoCompressorRightSource		(const NTV2OutputCrosspointID inValue)		{return WriteRegister (kRegStereoCompressor,	ULWord(inValue),		kRegMaskStereoCompressorRightSource,	kRegShiftStereoCompressorRightSource);}
bool CNTV2Card::GetStereoCompressorRightSource		(NTV2OutputCrosspointID & outValue)			{return CNTV2DriverInterface::ReadRegister	(kRegStereoCompressor,	outValue,	kRegMaskStereoCompressorRightSource,	kRegShiftStereoCompressorRightSource);}
