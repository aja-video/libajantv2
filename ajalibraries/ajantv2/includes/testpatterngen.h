/**
	@file		testpatterngen.h
	@brief		Declares the AJATestPatternGenEx class.
	@copyright	(C) 2010-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2_TESTPATTERN_GEN_EX
#define NTV2_TESTPATTERN_GEN_EX

#include "ajaexport.h"

#include "videodefines.h"
#include "ntv2enums.h"
#include "ntv2utils.h"
#include <vector>

typedef std::vector <uint8_t>		AJATestPatternBufferEx;
typedef std::vector <const char *>	AJATestPatternListEx;


enum AJATestPatternSelectEx
{
	AJA_TestPattEx_ColorBars100,
	AJA_TestPattEx_ColorBars75,
	AJA_TestPattEx_Ramp,
	AJA_TestPattEx_MultiBurst,
	AJA_TestPattEx_LineSweep,
	AJA_TestPattEx_CheckField,
	AJA_TestPattEx_FlatField,
	AJA_TestPattEx_MultiPattern,
	AJA_TestPattEx_Black,
	AJA_TestPattEx_White,
	AJA_TestPattEx_Border,
	AJA_TestPattEx_LinearRamp,
	AJA_TestPattEx_SlantRamp,
	AJA_TestPattEx_ZonePlate,
	AJA_TestPattEx_ColorQuadrant,
	AJA_TestPattEx_ColorQuadrantBorder,
	AJA_TestPattEx_ColorQuadrantTsi,
	AJA_TestPattEx_All
};


/**
	@brief	Another NTV2 test pattern generator. "Ex" was added to differentiate this
			generator from the one in 'ajastuff'.
**/
class AJAExport AJATestPatternGenEx
{
	// Construction
public:
	AJATestPatternGenEx();

	// Implementation
public:
	virtual ~AJATestPatternGenEx();

	virtual bool DrawTestPattern(AJATestPatternSelectEx pattNum, uint32_t frameWidth, uint32_t frameHeight, NTV2FrameBufferFormat pixelFormat, AJATestPatternBufferEx &testPatternBuffer);

	void setSignalMask(NTV2SignalMask signalMask) { _signalMask = signalMask;}
	void setUseRGBSmpteRange(bool useRGBSmpteRange) {_bRGBSmpteRange = useRGBSmpteRange;}
	bool getUseRGBSmpteRange() {return _bRGBSmpteRange;}
	NTV2SignalMask getSignalMask() { return _signalMask;}
	void setSliderValue(double sliderValue) { _sliderValue = sliderValue;}
	double getSliderValue() { return _sliderValue;}
	void setAlphaFromLuma(bool alphaFromLuma) {_bAlphaFromLuma = alphaFromLuma;}
	bool getAlphaFromLuma() {return _bAlphaFromLuma;}

	AJATestPatternListEx& getTestPatternList() { return _testPatternList;}

protected:

	virtual bool DrawSegmentedTestPattern();
	virtual bool DrawYCbCrFrame(uint16_t Y, uint16_t Cb, uint16_t Cr);	
	virtual bool DrawBorderFrame();
	virtual bool DrawLinearRampFrame();
	virtual bool DrawSlantRampFrame();
	virtual bool DrawZonePlateFrame();
	virtual bool DrawQuandrantBorderFrame();
	virtual bool DrawColorQuandrantFrame();
	virtual bool DrawColorQuandrantFrameTsi();

	void Init();
	bool IsSDStandard();
	bool GetStandard(int &standard, bool &b4K);

protected:
	AJATestPatternSelectEx _patternNumber;
	AJATestPatternListEx _testPatternList;
	uint32_t _frameWidth;     
	uint32_t _frameHeight;     
	uint32_t _linePitch;     
	uint32_t _dataLinePitch; 
	uint32_t _bufferSize; 
	uint8_t* _pTestPatternBuffer;
	uint32_t* _pPackedLineBuffer;
	uint16_t* _pUnPackedLineBuffer;
	bool _bRGBSmpteRange;
	bool _bAlphaFromLuma;

	double _sliderValue;
	NTV2SignalMask _signalMask;
	NTV2FrameBufferFormat _pixelFormat;

};	//	AJATestPatternGenEx

#endif	//	NTV2_TESTPATTERN_GEN_EX
