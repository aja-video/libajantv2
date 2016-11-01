/**
	@file		ntv2procamp.cpp
	@brief		Implements the CNTV2ProcAmp class, intended for devices having analog inputs.
	@copyright	(C) 2005-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2procamp.h"

using namespace std;


#if !defined (NTV2_DEPRECATE)

CNTV2ProcAmp::CNTV2ProcAmp(	UWord boardNumber,
							bool displayErrorMessage, 
							UWord ulBoardType)
:   CNTV2Status(boardNumber, displayErrorMessage,  ulBoardType) 
{
}

CNTV2ProcAmp::~CNTV2ProcAmp()
{
}

void CNTV2ProcAmp::SetupDefaultProcAmp()
{
	SetupDefaultProcAmpSD();
	SetupDefaultProcAmpHD();
}

bool CNTV2ProcAmp::ProcAmpAllControlsInitialized()
{
	return ProcAmpSDControlsInitialized() && ProcAmpHDControlsInitialized();
}

bool CNTV2ProcAmp::ProcAmpSDControlsInitialized()
{
	ULWord value;
	if ( ReadSDProcAmpControlsInitialized(&value))
		return((value == 0) ? false : true);
	return false;
}

bool CNTV2ProcAmp::ProcAmpHDControlsInitialized()
{
	ULWord value;
	if ( ReadHDProcAmpControlsInitialized(&value))
		return((value == 0) ? false : true);
	return false;
}

void CNTV2ProcAmp::SetupDefaultProcAmpSD()
{
	NTV2Range range;

	if (GetSDBrightnessAdjustmentRange(&range))
	{
		SetSDBrightnessAdjustment(range.nominal);
	}
	if (GetSDContrastAdjustmentRange(&range))
	{
		SetSDContrastAdjustment(range.nominal);
	}
	if (GetSDSaturationAdjustmentRange(&range))
	{
		SetSDSaturationAdjustment(range.nominal);
		WriteSDCbOffsetAdjustment(0x80);			// Initialize the following controls to nominal values.
		WriteSDCrOffsetAdjustment(0x80);			// These are not intended to be exposed to the user.
	}
	if (GetSDHueAdjustmentRange(&range))
	{
		SetSDHueAdjustment(range.nominal);
	}
	WriteSDProcAmpControlsInitialized();
}

void CNTV2ProcAmp::SetupDefaultProcAmpHD()
{
	NTV2Range range;

	if (GetHDBrightnessAdjustmentRange(&range))
	{
		SetHDBrightnessAdjustment(range.nominal);
	}
	if (GetHDContrastAdjustmentRange(&range))
	{
		SetHDContrastAdjustment(range.nominal);
	}
	if (GetHDSaturationAdjustmentRangeCb(&range))
	{
		SetHDSaturationAdjustmentCb(range.nominal);
		WriteHDCbOffsetAdjustment(0x200);			// These are not intended to be exposed to the user.
	}
	if (GetHDSaturationAdjustmentRangeCr(&range))
	{
		SetHDSaturationAdjustmentCr(range.nominal);
		WriteHDCrOffsetAdjustment(0x200);			// These are not intended to be exposed to the user.
	}
	if (GetHDHueAdjustmentRange(&range))
	{
		SetHDHueAdjustment(range.nominal);
	}
	WriteHDProcAmpControlsInitialized();
}

bool CNTV2ProcAmp::CalculateUIValue(double *uiValue, 
									NTV2Range &range,
									NTV2RegisterRange &registerRange,
									LWord registerValue)
{

	// Avoid divide by zero error and rounding issues for defaults
	if (registerValue == registerRange.nominal)
	{
		*uiValue = range.nominal;
	}
	else
	{
		if (registerRange.max == registerRange.min && registerValue != registerRange.min)
		{
			return false;	// man == min != current
		}
		else
		{
			double ratio = (double)(registerValue - registerRange.min)/(double)(registerRange.max - registerRange.min);
			*uiValue = (ratio * double(range.max - range.min)) + (double)range.min;
		}
	}

	if (*uiValue < range.min || *uiValue > range.max)
		return false;

	return true;
}

bool CNTV2ProcAmp::CalculateRegisterValue(	double newValue, 
											NTV2Range &range,
											NTV2RegisterRange &registerRange,
											LWord &newRegisterValue)
{
	if (newValue < range.min || newValue > range.max)
		return false;

	// Avoid divide by zero error and rounding issues for defaults
	if (range.max == range.min || newValue == range.nominal)
	{
		newRegisterValue = registerRange.nominal;
	}
	else
	{
		double newRatio = (newValue - range.min)/(range.max - range.min);
		newRegisterValue = LWord ((newRatio * double(registerRange.max - registerRange.min)) + (double)registerRange.min);
	}
	return true;
}

bool CNTV2ProcAmp::SetSDBrightnessAdjustment(double plusPercent)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;
	LWord newRegisterValue;

	if( GetSDBrightnessAdjustmentRange(&range) &&
		GetSDBrightnessAdjustmentRegisterRange(&registerRange)
	  )
	{
		if (CalculateRegisterValue(plusPercent, range, registerRange, newRegisterValue))
		{
			// Note: SD Brightness register is 2's complement signed value
			// Assumption: the host machine stores signed values as 2's complement.
			return WriteSDBrightnessAdjustment(newRegisterValue);	// 8 bit register
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::GetSDBrightnessAdjustment(double *plusPercent)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;

	if( GetSDBrightnessAdjustmentRange(&range) &&
		GetSDBrightnessAdjustmentRegisterRange(&registerRange)
	  )
	{
		ULWord registerValue;
		if (ReadSDBrightnessAdjustment(&registerValue))
		{
			return CalculateUIValue(plusPercent, range, registerRange, (LWord)registerValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::SetSDContrastAdjustment(double multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;
	LWord newRegisterValue;

	if( GetSDContrastAdjustmentRange(&range) &&
		GetSDContrastAdjustmentRegisterRange(&registerRange)
	  )
	{
		if (CalculateRegisterValue(multFactor, range, registerRange, newRegisterValue))
		{
			return WriteSDContrastAdjustment(newRegisterValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::GetSDContrastAdjustment(double *multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;

	if( GetSDContrastAdjustmentRange(&range) &&
		GetSDContrastAdjustmentRegisterRange(&registerRange)
	  )
	{
		ULWord registerValue;
		if (ReadSDContrastAdjustment(&registerValue))
		{
			return CalculateUIValue(multFactor, range, registerRange, (LWord)registerValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::SetSDSaturationAdjustment(double multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;
	LWord newRegisterValue;

	if( GetSDSaturationAdjustmentRange(&range) &&
		GetSDSaturationAdjustmentRegisterRange(&registerRange)
	  )
	{
		if (CalculateRegisterValue(multFactor, range, registerRange, newRegisterValue))
		{
			return WriteSDSaturationAdjustment(newRegisterValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::GetSDSaturationAdjustment(double *multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;

	if( GetSDSaturationAdjustmentRange(&range) &&
		GetSDSaturationAdjustmentRegisterRange(&registerRange)
	  )
	{
		ULWord registerValue;
		if (ReadSDSaturationAdjustment(&registerValue))
		{
			return CalculateUIValue(multFactor, range, registerRange, (LWord)registerValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::SetSDHueAdjustment(double degrees)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;
	LWord newRegisterValue;


	if( GetSDHueAdjustmentRange(&range) &&
		GetSDHueAdjustmentRegisterRange(&registerRange)
	  )
	{
		if (CalculateRegisterValue(degrees, range, registerRange, newRegisterValue))
		{
			return WriteSDHueAdjustment(newRegisterValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::GetSDHueAdjustment(double *degrees)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;

	if( GetSDHueAdjustmentRange(&range) &&
		GetSDHueAdjustmentRegisterRange(&registerRange)
	  )
	{
		ULWord registerValue;
		if (ReadSDHueAdjustment(&registerValue))
		{
			return CalculateUIValue(degrees, range, registerRange, (LWord)registerValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::SetHDBrightnessAdjustment(double plusPercent)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;
	LWord newRegisterValue;

	if( GetHDBrightnessAdjustmentRange(&range) &&
		GetHDBrightnessAdjustmentRegisterRange(&registerRange)
	  )
	{
		if (CalculateRegisterValue(plusPercent, range, registerRange, newRegisterValue))
		{
			return WriteHDBrightnessAdjustment(newRegisterValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::GetHDBrightnessAdjustment(double *plusPercent)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;

	if( GetHDBrightnessAdjustmentRange(&range) &&
		GetHDBrightnessAdjustmentRegisterRange(&registerRange)
	  )
	{
		ULWord registerValue;
		if (ReadHDBrightnessAdjustment(&registerValue))
		{
			return CalculateUIValue(plusPercent, range, registerRange, (LWord)registerValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::SetHDContrastAdjustment(double multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;
	LWord newRegisterValue;

	if( GetHDContrastAdjustmentRange(&range) &&
		GetHDContrastAdjustmentRegisterRange(&registerRange)
	  )
	{
		if (CalculateRegisterValue(multFactor, range, registerRange, newRegisterValue))
		{
			return WriteHDContrastAdjustment(newRegisterValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::GetHDContrastAdjustment(double *multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;

	if( GetHDContrastAdjustmentRange(&range) &&
		GetHDContrastAdjustmentRegisterRange(&registerRange)
	  )
	{
		ULWord registerValue;
		if (ReadHDContrastAdjustment(&registerValue))
		{
			return CalculateUIValue(multFactor, range, registerRange, (LWord)registerValue);
		}
	}
		
	return false;
}

// Set both HD Cb and Cr saturation to same value
bool CNTV2ProcAmp::SetHDSaturationAdjustment(double multFactor)
{
	if (SetHDSaturationAdjustmentCb(multFactor))
	{
		return SetHDSaturationAdjustmentCr(multFactor);
	}
	return false;
}

bool CNTV2ProcAmp::SetHDSaturationAdjustmentCb(double multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;
	LWord newRegisterValue;

	if( GetHDSaturationAdjustmentRangeCb(&range) &&
		GetHDSaturationAdjustmentRegisterRangeCb(&registerRange)
	  )
	{
		if (CalculateRegisterValue(multFactor, range, registerRange, newRegisterValue))
		{
			return WriteHDSaturationAdjustmentCb(newRegisterValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::SetHDSaturationAdjustmentCr(double multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;
	LWord newRegisterValue;

	if( GetHDSaturationAdjustmentRangeCr(&range) &&
		GetHDSaturationAdjustmentRegisterRangeCr(&registerRange)
	  )
	{
		if (CalculateRegisterValue(multFactor, range, registerRange, newRegisterValue))
		{
			return WriteHDSaturationAdjustmentCr(newRegisterValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::GetHDSaturationAdjustmentCb(double *multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;

	if( GetHDSaturationAdjustmentRangeCb(&range) &&
		GetHDSaturationAdjustmentRegisterRangeCb(&registerRange)
	  )
	{
		ULWord registerValue;
		if (ReadHDSaturationAdjustmentCb(&registerValue))
		{
			return CalculateUIValue(multFactor, range, registerRange, (LWord)registerValue);
		}
	}
		
	return false;
}

bool CNTV2ProcAmp::GetHDSaturationAdjustmentCr(double *multFactor)
{
	NTV2Range range;
	NTV2RegisterRange registerRange;

	if( GetHDSaturationAdjustmentRangeCr(&range) &&
		GetHDSaturationAdjustmentRegisterRangeCr(&registerRange)
	  )
	{
		ULWord registerValue;
		if (ReadHDSaturationAdjustmentCr(&registerValue))
		{
			return CalculateUIValue(multFactor, range, registerRange, (LWord)registerValue);
		}
	}
		
	return false;
}


bool CNTV2ProcAmp::GetSDBrightnessAdjustmentRange(NTV2Range *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetSDContrastAdjustmentRange(NTV2Range *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetSDSaturationAdjustmentRange(NTV2Range *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetSDHueAdjustmentRange(NTV2Range *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetHDBrightnessAdjustmentRange(NTV2Range *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetHDContrastAdjustmentRange(NTV2Range *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetHDSaturationAdjustmentRangeCb(NTV2Range *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetHDSaturationAdjustmentRangeCr(NTV2Range *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}


bool CNTV2ProcAmp::GetSDBrightnessAdjustmentRegisterRange(NTV2RegisterRange *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetSDContrastAdjustmentRegisterRange(NTV2RegisterRange *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetSDSaturationAdjustmentRegisterRange(NTV2RegisterRange *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetSDHueAdjustmentRegisterRange(NTV2RegisterRange *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetHDBrightnessAdjustmentRegisterRange(NTV2RegisterRange *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetHDContrastAdjustmentRegisterRange(NTV2RegisterRange *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetHDSaturationAdjustmentRegisterRangeCb(NTV2RegisterRange *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

bool CNTV2ProcAmp::GetHDSaturationAdjustmentRegisterRangeCr(NTV2RegisterRange *range)
{
	range->min = 0; range->max = 0; range->nominal = 0;
	return false;
}

#endif	//	!defined (NTV2_DEPRECATE)
