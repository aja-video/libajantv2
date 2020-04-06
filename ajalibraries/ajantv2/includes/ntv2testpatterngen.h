/**
	@file		ajantv2/includes/ntv2testpatterngen.h
	@brief		Declares the NTV2TestPatternGen class.
	@copyright	(C) 2010-2020 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2_TESTPATTERN_GEN_
#define NTV2_TESTPATTERN_GEN_

#include "ajaexport.h"
#include "ntv2videodefines.h"
#include "ntv2enums.h"
#include "ntv2utils.h"
#include <vector>
#include <set>
#include <string>

typedef std::vector <uint8_t>		NTV2TestPatternBuffer, NTV2TestPatBuffer;	///< @brief	A byte vector that stores a complete video frame.
#if !defined(NTV2_DEPRECATE_15_0)
	typedef std::vector <const char *>	NTV2TestPatternList;	///< @deprecated	Use NTV2TestPatternNames instead.
#endif//!defined(NTV2_DEPRECATE_15_0)
typedef NTV2StringList					NTV2TestPatternNames;	///< @brief	A list (std::vector) of pattern names.

/**
	@brief	Identifies a predefined NTV2 test pattern.
**/
enum NTV2TestPatternSelect
{
	NTV2_TestPatt_ColorBars100,
	NTV2_TestPatt_FIRST			= NTV2_TestPatt_ColorBars100,
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
	NTV2_TestPatt_ZonePlate_12b_RGB,
	NTV2_TestPatt_LinearRamp_12b_RGB,
	NTV2_TestPatt_HLG_Narrow_12b_RGB,
	NTV2_TestPatt_PQ_Narrow_12b_RGB,
	NTV2_TestPatt_PQ_Wide_12b_RGB,
	NTV2_TestPatt_All,
	NTV2_TestPatt_INVALID	= NTV2_TestPatt_All,
	NTV2_TestPatt_LAST		= NTV2_TestPatt_All
};

#define NTV2_IS_VALID_PATTERN(__S__) ((__S__) >= NTV2_TestPatt_ColorBars100  &&  (__S__) < NTV2_TestPatt_All)
#define NTV2_IS_12B_PATTERN(__S__)	 ((__S__) >= NTV2_TestPatt_ZonePlate_12b_RGB  &&  (__S__) < NTV2_TestPatt_All)

typedef std::set<NTV2TestPatternSelect>		NTV2TestPatternSet;
typedef NTV2TestPatternSet::const_iterator	NTV2TestPatternSetConstIter;


/**
	@brief	The NTV2 test pattern generator.
	@bug	::NTV2TestPatternGen doesn't work for planar formats.
**/
class AJAExport NTV2TestPatternGen
{
public:
							NTV2TestPatternGen ();
	virtual inline			~NTV2TestPatternGen ()	{}

	/**
		@brief		Renders the given test pattern or color into a host raster buffer.
		@param[in]	inTPName		Specifies the name of the test pattern or color to be drawn.
		@param[in]	inFormatDesc	Describes the raster memory.
		@param		inBuffer		Specifies the host memory buffer to be written.
		@return		True if successful;  otherwise false.
	**/
	virtual bool			DrawTestPattern (const std::string & inTPName,
											const NTV2FormatDescriptor & inFormatDesc,
											NTV2_POINTER & inBuffer);
	/**
		@brief		Renders the given test pattern into a host raster buffer.
		@param[in]	inPattern		Specifies the test pattern to be drawn.
		@param[in]	inFormatDesc	Describes the raster memory.
		@param		inBuffer		Specifies the host memory buffer to be written.
		@return		True if successful;  otherwise false.
	**/
	virtual bool			DrawTestPattern (const NTV2TestPatternSelect inPattern,
											const NTV2FormatDescriptor & inFormatDesc,
											NTV2_POINTER & inBuffer);

	virtual inline bool		DrawTestPattern (const NTV2TestPatternSelect inPattern,
											const NTV2FormatDescriptor & inDesc,
											NTV2TestPatBuffer & outBuffer)
																			{return DrawTestPattern(inPattern,
																									inDesc.GetRasterWidth(),
																									inDesc.GetVisibleRasterHeight(),
																									inDesc.GetPixelFormat(),
																									outBuffer);}
	virtual bool			DrawTestPattern (const NTV2TestPatternSelect inPattern,
											uint32_t inFrameWidth, uint32_t inFrameHeight,
											const NTV2FrameBufferFormat inPixelFormat,
											NTV2TestPatBuffer & outBuffer);

	inline void				setSignalMask (NTV2SignalMask signalMask)		{_signalMask = signalMask;}
	inline void				setUseRGBSmpteRange (bool useRGBSmpteRange)		{_bRGBSmpteRange = useRGBSmpteRange;}
	inline bool				getUseRGBSmpteRange (void) const				{return _bRGBSmpteRange;}
	inline NTV2SignalMask	getSignalMask (void) const						{return _signalMask;}
	inline void				setSliderValue (double sliderValue)				{_sliderValue = sliderValue;}
	inline const double &	getSliderValue (void) const						{return _sliderValue;}
	inline void				setAlphaFromLuma (bool alphaFromLuma)			{_bAlphaFromLuma = alphaFromLuma;}
	inline bool				getAlphaFromLuma (void) const					{return _bAlphaFromLuma;}

	/**
		@param[in]	inPattern	Specifies the test pattern of interest.
		@param[in]	inDesc		Specifies a description of the raster being used.
		@return		True, if the given test pattern can be drawn for the given raster description.
	**/
	static bool						canDrawTestPattern (const NTV2TestPatternSelect inPattern,
														const NTV2FormatDescriptor & inDesc);

	/**
		@return		An ordered collection of strings containing the names of all available test patterns.
	**/
	static NTV2TestPatternNames		getTestPatternNames (void);

	/**
		@return		An ordered collection of strings containing the names of all available flat-field colors.
	**/
	static NTV2StringList			getColorNames (void);

	/**
		@return		The test pattern that corresponds to the given name.
		@param[in]	inName	Specifies the test pattern name. The search is done case-insensitively.
	**/
	static NTV2TestPatternSelect	findTestPatternByName (const std::string & inName);

	/**
		@return		The flat-field RGB color value that corresponds to the given name.
					The highest-order byte in the 32-bit result is 0x00; the next-lower byte
					is the Red value (0-255); the next-lower byte is the Green value (0-255);
					the lowest-order byte is the Blue value (0-255). A zero return value
					means "not found".
		@param[in]	inName	Specifies the color name. The search is done case-insensitively.
		@note		"Black" and "White" are not returned, as these are available as ordinary
					test patterns.
	**/
	static ULWord					findRGBColorByName (const std::string & inStartsWith);

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
	virtual bool	DrawQuadrantBorderFrame ();
	virtual bool	DrawColorQuadrantFrame ();
	virtual bool	DrawColorQuadrantFrameTsi ();

	//12b patterns
	virtual bool	DrawTestPatternNarrowHLG();
	virtual bool	DrawTestPatternNarrowPQ();
	virtual bool	DrawTestPatternWidePQ();
	virtual bool	Draw12BitRamp();
	virtual bool	Draw12BitZonePlate();
	virtual void	PrepareForOutput();

	bool			IsSDStandard();
	bool			GetStandard (int & outStandard, bool & outIs4K, bool & outIs8K);
	virtual bool	drawIt (void);

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
	
	uint32_t mNumPixels;
    uint32_t mNumLines;
    uint32_t mBitsPerComponent;
    uint32_t mSamplesPerPixel;
	uint16_t mCompressionType;
    uint32_t mDataOffset;
    uint32_t mDataSize;
    std::vector<char> mData;
    std::vector<uint16_t> mUnPackedRAWBuffer;
    std::vector<uint16_t> mRGBBuffer;

};	//	NTV2TestPatternGen

#if !defined(NTV2_DEPRECATE_14_3)
	typedef	NTV2TestPatternGen					AJATestPatternGenEx;				///< @deprecated	Use NTV2TestPatternGen instead.
	typedef NTV2TestPatternBuffer				AJATestPatternBufferEx;				///< @deprecated	Use NTV2TestPatternBuffer instead.
	typedef	NTV2TestPatternList					AJATestPatternListEx;				///< @deprecated	Use NTV2TestPatternNames instead.
	typedef NTV2TestPatternSelect				AJATestPatternSelectEx;				///< @deprecated	Use NTV2TestPatternSelect instead.
	#define	AJA_TestPattEx_ColorBars100			NTV2_TestPatt_ColorBars100			///< @deprecated	Use NTV2_TestPatt_ColorBars100 instead.
	#define	AJA_TestPattEx_ColorBars75			NTV2_TestPatt_ColorBars75			///< @deprecated	Use NTV2_TestPatt_ColorBars75 instead.
	#define	AJA_TestPattEx_Ramp					NTV2_TestPatt_Ramp					///< @deprecated	Use NTV2_TestPatt_Ramp instead.
	#define	AJA_TestPattEx_MultiBurst			NTV2_TestPatt_MultiBurst			///< @deprecated	Use NTV2_TestPatt_MultiBurst instead.
	#define	AJA_TestPattEx_LineSweep			NTV2_TestPatt_LineSweep				///< @deprecated	Use NTV2_TestPatt_LineSweep instead.
	#define	AJA_TestPattEx_CheckField			NTV2_TestPatt_CheckField			///< @deprecated	Use NTV2_TestPatt_CheckField instead.
	#define	AJA_TestPattEx_FlatField			NTV2_TestPatt_FlatField				///< @deprecated	Use NTV2_TestPatt_FlatField instead.
	#define	AJA_TestPattEx_MultiPattern			NTV2_TestPatt_MultiPattern			///< @deprecated	Use NTV2_TestPatt_MultiPattern instead.
	#define	AJA_TestPattEx_Black				NTV2_TestPatt_Black					///< @deprecated	Use NTV2_TestPatt_Black instead.
	#define	AJA_TestPattEx_White				NTV2_TestPatt_White					///< @deprecated	Use NTV2_TestPatt_White instead.
	#define	AJA_TestPattEx_Border				NTV2_TestPatt_Border				///< @deprecated	Use NTV2_TestPatt_Border instead.
	#define	AJA_TestPattEx_LinearRamp			NTV2_TestPatt_LinearRamp			///< @deprecated	Use NTV2_TestPatt_LinearRamp instead.
	#define	AJA_TestPattEx_SlantRamp			NTV2_TestPatt_SlantRamp				///< @deprecated	Use NTV2_TestPatt_SlantRamp instead.
	#define	AJA_TestPattEx_ZonePlate			NTV2_TestPatt_ZonePlate				///< @deprecated	Use NTV2_TestPatt_ZonePlate instead.
	#define	AJA_TestPattEx_ColorQuadrant		NTV2_TestPatt_ColorQuadrant			///< @deprecated	Use NTV2_TestPatt_ColorQuadrant instead.
	#define	AJA_TestPattEx_ColorQuadrantBorder	NTV2_TestPatt_ColorQuadrantBorder	///< @deprecated	Use NTV2_TestPatt_ColorQuadrantBorder instead.
	#define	AJA_TestPattEx_ColorQuadrantTsi		NTV2_TestPatt_ColorQuadrantTsi		///< @deprecated	Use NTV2_TestPatt_ColorQuadrantTsi instead.
	#define	AJA_TestPattEx_All					NTV2_TestPatt_All					///< @deprecated	Use NTV2_TestPatt_All instead.
#endif	//	NTV2_DEPRECATE_14_3

#endif	//	NTV2_TESTPATTERN_GEN_
