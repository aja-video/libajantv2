/**
	@file		ntv2procamp.h
	@brief		Declares the CNTV2ProcAmp class.
	@copyright	(C) 2005-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2ProcAmp_H
#define NTV2ProcAmp_H

#include "ajaexport.h"
#include "ntv2status.h"

#if defined (NTV2_DEPRECATE)
	typedef	CNTV2Card	CNTV2ProcAmp;	///< @deprecated	Use CNTV2Card instead.
#else

typedef	struct 
{ 
	double min;
	double max;
	double nominal;
} NTV2Range;

typedef	struct 
{ 
	LWord min;
	LWord max;
	LWord nominal;
} NTV2RegisterRange;

/**
	@deprecated	Use CNTV2Card instead.
**/
class AJAExport NTV2_DEPRECATED CNTV2ProcAmp : public CNTV2Status
{
public:  // Constructor - Deconstructor
    CNTV2ProcAmp() {};
	CNTV2ProcAmp(UWord inDeviceIndex, bool displayErrorMessage = false, 
		UWord ulBoardType = DEVICETYPE_NTV2);

	virtual ~CNTV2ProcAmp();

public: // Methods

	void SetupDefaultProcAmp();		// Both SD and HD
	void SetupDefaultProcAmpSD();	// SD Only
	void SetupDefaultProcAmpHD();	// HD Only

	bool ProcAmpAllControlsInitialized();
	bool ProcAmpSDControlsInitialized();
	bool ProcAmpHDControlsInitialized();

	// Proc amp controls for boards with analog inputs
	bool SetSDBrightnessAdjustment(double plusPercent);
	bool GetSDBrightnessAdjustment(double *plusPercent);
	bool SetSDContrastAdjustment(double multFactor);
	bool GetSDContrastAdjustment(double *multFactor);
	bool SetSDSaturationAdjustment(double multFactor);
	bool GetSDSaturationAdjustment(double *multFactor);
	bool SetSDHueAdjustment(double degrees);
	bool GetSDHueAdjustment(double *degrees);

	bool SetHDBrightnessAdjustment(double plusPercent);
	bool GetHDBrightnessAdjustment(double *plusPercent);
	bool SetHDContrastAdjustment(double multFactor);
	bool GetHDContrastAdjustment(double *multFactor);
	bool SetHDSaturationAdjustment(double multFactor);	// Set both Cb and Cr gain to same value.  
	bool SetHDSaturationAdjustmentCb(double multFactor);
	bool GetHDSaturationAdjustmentCb(double *multFactor);
	bool SetHDSaturationAdjustmentCr(double multFactor);
	bool GetHDSaturationAdjustmentCr(double *multFactor);
	bool SetHDHueAdjustment(double degrees) { (void) degrees; return false; };
	bool GetHDHueAdjustment(double *degrees) { (void) degrees; return false; };

	bool GetSDBrightnessAdjustmentRange(NTV2Range *range);	// offset percent 
	bool GetSDContrastAdjustmentRange(NTV2Range *range);	// gain factor	
	bool GetSDSaturationAdjustmentRange(NTV2Range *range);	// gain factor
	bool GetSDHueAdjustmentRange(NTV2Range *range);			// degrees

	bool GetHDBrightnessAdjustmentRange(NTV2Range *range);	// offset percent
	bool GetHDContrastAdjustmentRange(NTV2Range *range);	// gain factor
	bool GetHDSaturationAdjustmentRangeCb(NTV2Range *range);	// gain factor
	bool GetHDSaturationAdjustmentRangeCr(NTV2Range *range);	// gain factor
	bool GetHDHueAdjustmentRange(NTV2Range *range) {  (void) range; return false; };	// Not supported in HW

protected:  // Methods

	bool CalculateRegisterValue(double newValue, 
								NTV2Range &range,
								NTV2RegisterRange &registerRange,
								LWord &newRegisterValue);

	bool CalculateUIValue(	double *uiValue, 
							NTV2Range &range,
							NTV2RegisterRange &registerRange,
							LWord registerValue);

	bool GetSDBrightnessAdjustmentRegisterRange(NTV2RegisterRange *range);	// offset percent 
	bool GetSDContrastAdjustmentRegisterRange(NTV2RegisterRange *range);	// gain factor	
	bool GetSDSaturationAdjustmentRegisterRange(NTV2RegisterRange *range);	// gain factor
	bool GetSDHueAdjustmentRegisterRange(NTV2RegisterRange *range);			// degrees

	bool GetHDBrightnessAdjustmentRegisterRange(NTV2RegisterRange *range);	// offset percent
	bool GetHDContrastAdjustmentRegisterRange(NTV2RegisterRange *range);	// gain factor
	bool GetHDSaturationAdjustmentRegisterRangeCb(NTV2RegisterRange *range);	// gain factor
	bool GetHDSaturationAdjustmentRegisterRangeCr(NTV2RegisterRange *range);	// gain factor
	bool GetHDHueAdjustmentRegisterRange(NTV2RegisterRange *range) { (void) range;  return false; };	// Not supported in HW

};	//	CNTV2ProcAmp

#endif	//	else !defined (NTV2_DEPRECATE)


#endif	//	NTV2ProcAmp_H
