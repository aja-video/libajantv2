/* SPDX-License-Identifier: MIT */
/**
	@file		ajantv2/includes/ntv2testpatterngen.h
	@brief		Declares the NTV2TestPatternGen class.
	@copyright	(C) 2010-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2_TESTPATTERN_GEN_
#define NTV2_TESTPATTERN_GEN_

#include "ajaexport.h"
#include "ntv2enums.h"
#include "ntv2utils.h"
#include <vector>
#include <set>
#include <string>

typedef std::vector <uint8_t>	NTV2TestPatternBuffer, NTV2TestPatBuffer;	///< @brief	A byte vector that stores a complete video frame.
typedef NTV2StringList			NTV2TestPatternNames;	///< @brief	A list (std::vector) of pattern names.

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
	NTV2_TestPatt_TsiAlignment,
	NTV2_TestPatt_ZonePlate_12b_RGB,
	NTV2_TestPatt_LinearRamp_12b_RGB,
	NTV2_TestPatt_HLG_Narrow_12b_RGB,
	NTV2_TestPatt_PQ_Narrow_12b_RGB,
	NTV2_TestPatt_PQ_Wide_12b_RGB,
	NTV2_TestPatt_All,
	NTV2_TestPatt_INVALID	= NTV2_TestPatt_All,
	NTV2_TestPatt_LAST		= NTV2_TestPatt_All
};
typedef NTV2TestPatternSelect	NTV2TestPatternID;

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
	//	CLASS METHODS
	public:
		/**
			@name	Class Methods
		**/
		///@{

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
			@return		An ordered collection of strings containing the names of all available flat-field "web colors".
		**/
		static NTV2StringList			getColorNames (void);	//	New in SDK 16.0

		/**
			@return		The test pattern that corresponds to the given name.
			@param[in]	inName	Specifies the test pattern name. The search is done case-insensitively.
		**/
		static NTV2TestPatternSelect	findTestPatternByName (const std::string & inName);	//	New in SDK 16.0

		/**
			@return		The flat-field RGB "web color" value that corresponds to the given name.
						The highest-order byte in the 32-bit result is 0x00; the next-lower byte
						is the Red value (0-255); the next-lower byte is the Green value (0-255);
						the lowest-order byte is the Blue value (0-255). A zero return value
						means "not found".
			@param[in]	inName	Specifies the "web color" name. The search is done case-insensitively.
			@note		"Black" and "White" are not returned, as these are available as ordinary
						test patterns NTV2_TestPatt_Black and NTV2_TestPatt_White, respectively.
			@bug		As currently implemented and documented, this function only returns
						8-bit deep color values.
		**/
		static ULWord					findRGBColorByName (const std::string & inName);	//	New in SDK 16.0
		///@}

	//	INSTANCE METHODS
	public:
		/**
			@name	Constructors & Destructors
		**/
		///@{
								NTV2TestPatternGen ();
		virtual inline			~NTV2TestPatternGen ()	{}
		///@}

		/**
			@name	Drawing Methods
		**/
		///@{
		/**
			@brief		Renders the given test pattern or color into a host raster buffer.
			@param[in]	inTPName		Specifies the name of the test pattern or web color to be drawn.
			@param[in]	inFormatDesc	Describes the raster memory.
			@param		inBuffer		Specifies the host memory buffer to be written.
			@note		If the format descriptor describes a "tall" or "taller" (VANC) raster geometry,
						the first byte in the specified buffer is presumed to be the start of the VANC region.
			@note		If my mSetDstVancBlack member is true, the buffer's VANC region will also be cleared
						to legal black.
			@return		True if successful;  otherwise false.
			@bug		Needs planar pixel format implementations.
		**/
		virtual bool			DrawTestPattern (const std::string & inTPName,
												const NTV2FormatDescriptor & inFormatDesc,
												NTV2Buffer & inBuffer);	//	New in SDK 16.0
		/**
			@brief		Renders the given test pattern into a host raster buffer.
			@param[in]	inPattern		Specifies the test pattern to be drawn.
			@param[in]	inFormatDesc	Describes the raster memory.
			@param		inBuffer		Specifies the host memory buffer to be written.
			@note		If the format descriptor describes a "tall" or "taller" (VANC) geometry, the
						first byte in the specified buffer is presumed to be the start of the VANC region.
			@note		If my mSetDstVancBlack member is true, the buffer's VANC region will also be cleared
						to legal black.
			@return		True if successful;  otherwise false.
			@bug		Needs planar pixel format implementations.
		**/
		virtual bool			DrawTestPattern (const NTV2TestPatternSelect inPattern,
												const NTV2FormatDescriptor & inFormatDesc,
												NTV2Buffer & inBuffer);	//	New in SDK 16.0

#if !defined(NTV2_DEPRECATE_16_0)
		/**
			@deprecated	Use the DrawTestPattern method that requires an NTV2Buffer to specify the buffer to fill.
		**/
		virtual NTV2_DEPRECATED_f(bool DrawTestPattern (const NTV2TestPatternSelect inPattern, const NTV2FormatDescriptor & inDesc, NTV2TestPatBuffer & outBuffer));
		/**
			@deprecated	Use the DrawTestPattern method that requires an NTV2Buffer to specify the buffer to fill.
		**/
		virtual NTV2_DEPRECATED_f(bool DrawTestPattern (const NTV2TestPatternSelect inPat, const uint32_t inWdth, const uint32_t inHght, const NTV2FrameBufferFormat inPF, NTV2TestPatBuffer & outBuf));
#endif	//	!defined(NTV2_DEPRECATE_16_0)
		///@}

		/**
			@name	Getters
		**/
		///@{
		inline bool				getUseRGBSmpteRange (void) const		{return mSetRGBSmpteRange;}
		inline NTV2SignalMask	getSignalMask (void) const				{return mSignalMask;}
		inline const double &	getSliderValue (void) const				{return mSliderValue;}
		inline bool				getAlphaFromLuma (void) const			{return mSetAlphaFromLuma;}
		inline bool				setVANCToLegalBlack (void) const		{return mSetDstVancBlack;}	///< @return	True if DrawTestPattern will also set VANC lines (if any) to legal black.
		///@}

		/**
			@name	Setters
		**/
		///@{
		inline NTV2TestPatternGen &	setUseRGBSmpteRange (const bool useRGBSmpteRange)	{mSetRGBSmpteRange = useRGBSmpteRange; return *this;}
		inline NTV2TestPatternGen &	setSignalMask (const NTV2SignalMask signalMask)		{mSignalMask = signalMask; return *this;}
		inline NTV2TestPatternGen &	setSliderValue (const double & sliderValue)			{mSliderValue = sliderValue; return *this;}
		inline NTV2TestPatternGen &	setAlphaFromLuma (const bool alphaFromLuma)			{mSetAlphaFromLuma = alphaFromLuma; return *this;}
		/**
			@brief		Changes my "clear VANC lines to legal black" setting.
			@param[in]	inClearVANC		Specify true to have my DrawTestPattern method automatically clear the VANC region (if any) to legal black.
			@return		A non-constant reference to me.
		**/
		inline NTV2TestPatternGen &	setVANCToLegalBlack (const bool inClearVANC)		{mSetDstVancBlack = inClearVANC; return *this;}
		///@}

	//	INTERNAL METHODS
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
		virtual bool	DrawColorQuadrantFrameTsi2 ();

		//	12-bit patterns:
		virtual bool	DrawTestPatternNarrowHLG();
		virtual bool	DrawTestPatternNarrowPQ();
		virtual bool	DrawTestPatternWidePQ();
		virtual bool	Draw12BitRamp();
		virtual bool	Draw12BitZonePlate();
		virtual void	PrepareForOutput();

		bool			IsSDStandard(void) const;
		bool			GetStandard (int & outStandard, bool & outIs4K, bool & outIs8K) const;
		virtual bool	drawIt (void);

	//	INSTANCE DATA
	protected:
		NTV2TestPatternID	mPatternID;			///< @brief	Pattern number
//		NTV2FormatDesc		mDstFormat;			///< @brief	Dest format
		NTV2PixelFormat		mDstPixelFormat;	///< @brief	Dest pixel format
		uint32_t			mDstFrameWidth;		///< @brief	Dest width (pixels)
		uint32_t			mDstFrameHeight;	///< @brief	Dest height (lines)
		uint32_t			mDstLinePitch;		///< @brief	Dest bytes per row
		uint32_t			mSrcLinePitch;		///< @brief	Src bytes per row
		uint32_t			mDstBufferSize;		///< @brief	Dest visible buffer size (bytes)
		uint8_t *			mpDstBuffer;		///< @brief	Dest buffer (start of active video)
		uint32_t *			mpPackedLineBuffer;
		uint16_t *			mpUnpackedLineBuffer;
		bool				mSetRGBSmpteRange;
		bool				mSetAlphaFromLuma;
		bool				mSetDstVancBlack;	///< @brief	Set destination VANC lines to legal black?
		double				mSliderValue;		///< @brief	Used for Zone Plate
		NTV2SignalMask		mSignalMask;		///< @brief	Component mask for MultiBurst, LineSweep

		uint32_t			mNumPixels;
		uint32_t			mNumLines;
		uint32_t			mBitsPerComponent;
		uint32_t			mSamplesPerPixel;
		uint16_t			mCompressionType;
		uint32_t			mDataOffset;
		uint32_t			mDataSize;
		std::vector<char>		mData;
		std::vector<uint16_t>	mUnPackedRAWBuffer;
		std::vector<uint16_t>	mRGBBuffer;

};	//	NTV2TestPatternGen

#endif	//	NTV2_TESTPATTERN_GEN_
