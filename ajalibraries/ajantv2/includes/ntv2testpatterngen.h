/**
	@file		ajantv2/includes/testpatterngen.h
	@brief		Declares the AJATestPatternGenEx class.
	@copyright	(C) 2010-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2_TESTPATTERN_GEN_EX
#define NTV2_TESTPATTERN_GEN_EX

#include "ajaexport.h"
#include "ntv2videodefines.h"
#include "ntv2enums.h"
#include "ntv2utils.h"
#include <vector>
#include <string>

typedef std::vector <uint8_t>		NTV2TestPatternBuffer;
#if !defined(NTV2_DEPRECATE_15_0)
	typedef std::vector <const char *>	NTV2TestPatternList;
#endif//!defined(NTV2_DEPRECATE_15_0)
typedef std::vector <std::string>	NTV2TestPatternNames;


enum NTV2TestPatternSelect
{
	NTV2_TestPatt_ColorBars100,
	NTV2_TestPatt_ColorBars75,
	NTV2_TestPatt_Ramp,
	NTV2_TestPatt_MultiBurst,
	NTV2_TestPatt_LineSweep,
	NTV2_TestPatt_CheckField,
	NTV2_TestPatt_FlatField,
	NTV2_TestPatt_MultiPattern,
	NTV2_TestPatt_Black,
	NTV2_TestPatt_White,
	NTV2_TestPatt_Border,
	NTV2_TestPatt_LinearRamp,
	NTV2_TestPatt_SlantRamp,
	NTV2_TestPatt_ZonePlate,
	NTV2_TestPatt_ColorQuadrant,
	NTV2_TestPatt_ColorQuadrantBorder,
	NTV2_TestPatt_ColorQuadrantTsi,
	NTV2_TestPatt_All
};


/**
	@brief	The NTV2 test pattern generator.
	@bug	::NTV2TestPatternGen doesn't work for planar formats.
	@todo	Needs a new DrawTestPattern method that accepts an ::NTV2FormatDescriptor and ::NTV2_POINTER.
**/
class AJAExport NTV2TestPatternGen
{
public:
							NTV2TestPatternGen ();
	virtual					~NTV2TestPatternGen ();

	virtual bool			DrawTestPattern (NTV2TestPatternSelect pattNum, uint32_t frameWidth, uint32_t frameHeight,
											NTV2FrameBufferFormat pixelFormat, NTV2TestPatternBuffer & outBuffer);

	inline void				setSignalMask (NTV2SignalMask signalMask)		{_signalMask = signalMask;}
	inline void				setUseRGBSmpteRange (bool useRGBSmpteRange)		{_bRGBSmpteRange = useRGBSmpteRange;}
	inline bool				getUseRGBSmpteRange (void) const				{return _bRGBSmpteRange;}
	inline NTV2SignalMask	getSignalMask (void) const						{return _signalMask;}
	inline void				setSliderValue (double sliderValue)				{_sliderValue = sliderValue;}
	inline const double &	getSliderValue (void) const						{return _sliderValue;}
	inline void				setAlphaFromLuma (bool alphaFromLuma)			{_bAlphaFromLuma = alphaFromLuma;}
	inline bool				getAlphaFromLuma (void) const					{return _bAlphaFromLuma;}

	static const NTV2TestPatternNames &	getTestPatternNames (void);
#if !defined(NTV2_DEPRECATE_15_0)
	static NTV2_SHOULD_BE_DEPRECATED (NTV2TestPatternList &		getTestPatternList (void));
#endif	//	!defined(NTV2_DEPRECATE_15_0)

protected:
	virtual bool	DrawSegmentedTestPattern ();
	virtual bool	DrawYCbCrFrame (uint16_t Y, uint16_t Cb, uint16_t Cr);	
	virtual bool	DrawBorderFrame ();
	virtual bool	DrawLinearRampFrame ();
	virtual bool	DrawSlantRampFrame ();
	virtual bool	DrawZonePlateFrame ();
	virtual bool	DrawQuandrantBorderFrame ();
	virtual bool	DrawColorQuandrantFrame ();
	virtual bool	DrawColorQuandrantFrameTsi ();

	void Init();
	bool IsSDStandard();
	bool GetStandard(int &standard, bool &b4K);

protected:
	NTV2TestPatternSelect	_patternNumber;
	uint32_t	_frameWidth;     
	uint32_t	_frameHeight;     
	uint32_t	_linePitch;     
	uint32_t	_dataLinePitch; 
	uint32_t	_bufferSize; 
	uint8_t *	_pTestPatternBuffer;
	uint32_t *	_pPackedLineBuffer;
	uint16_t *	_pUnPackedLineBuffer;
	bool		_bRGBSmpteRange;
	bool		_bAlphaFromLuma;

	double					_sliderValue;
	NTV2SignalMask			_signalMask;
	NTV2FrameBufferFormat	_pixelFormat;

};	//	NTV2TestPatternGen

#endif	//	NTV2_TESTPATTERN_GEN_EX
