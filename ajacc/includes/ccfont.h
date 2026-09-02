/**
	@file		ccfont.h
	@brief		Declaration of NTV2CCFont.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/


#ifndef CCFONT_H
	#define CCFONT_H

	#include "ajatypes.h"
	#include "ntv2caption608types.h"
	#include "ntv2enums.h"


	/**
		@brief	A zero-based index number that uniquely identifies a glyph.
	**/
	typedef	uint16_t	NTV2GlyphIndex;


	/**
		@brief	This is the font used when rendering CEA-608 captions into a frame buffer.
				I define a "dot" map having a fixed width and height, plus a fixed set of glyphs,
				each with a particular dot pattern. Each glyph corresponds to an NTV2_CC608_CodePoint.
	**/
	class NTV2CCFont
	{
		//	CLASS METHODS
		public:
			/**
				@brief		Returns a constant reference to the NTV2CCFont singleton.
			**/
			static const NTV2CCFont &		GetInstance (void);

			/**
				@brief		Returns the character code that corresponds to the given glyph.
				@param[in]	inGlyphIndex	Specifies the zero-based index number of the glyph of interest.
				@return		The character code that corresponds to the given glyph.
			**/
			static UByte					GlyphIndexToCharacterCode (const NTV2GlyphIndex inGlyphIndex);

			/**
				@brief		Returns the zero-based glyph index number that corresponds to the given character code.
				@param[in]	inCharacterCode		Specifies the character code of interest.
				@return		The zero-based glyph index number that corresponds to the given character code.
			**/
			static NTV2GlyphIndex			CharacterCodeToGlyphIndex (const UByte inCharacterCode);

			/**
				@brief		Returns the character code that corresponds to the given unicode codepoint.
				@param[in]	inCodePoint		Specifies the unicode code point to be converted.
				@return		The character code that corresponds to the given unicode codepoint.
			**/
			static UByte					UnicodeCodePointToCharacterCode (const ULWord inCodePoint);

			/**
				@brief		Converts the given UTF-8 encoded string into a string of CCFont character codes.
				@param[in]	inUtf8Str		Specifies the UTF-8 encoded string to be converted.
				@return		A string of equivalent CCFont-compatible character codes.
			**/
			static std::string				Utf8ToCCFontByteArray (const std::string & inUtf8Str);


		//	INSTANCE METHODS
		public:
			/**
				@brief		Returns true if there is a glyph available for the given CEA-608 code point.
				@param[in]	in608CodePoint	Specifies the CEA-608 code point of interest.
				@return		True if there is a glyph available for the given CEA-608 code point;  otherwise false.
			**/
			virtual bool					HasGlyphFor608CodePoint (const NTV2_CC608_CodePoint in608CodePoint) const;

			/**
				@brief		Returns a pointer to the "dot" bitmap of the glyph that represents the given CEA-608 code point.
				@param[in]	in608CodePoint	Specifies the CEA-608 code point for which a glyph is being requested.
				@return		If available, a valid non-NULL pointer to the character "dot" bitmap for the given codepoint;
							otherwise, NULL.
			**/
			virtual UWord *					GetGlyphFor608CodePoint (const NTV2_CC608_CodePoint in608CodePoint) const;

			/**
				@brief		Returns the 16-bit "dot" bitmap of the row of the character glyph having the given offset.
				@param[in]	inGlyphIndex	Specifies the glyph index, which must be less than the value returned from
											GetGlyphCount().
				@param[in]	inRow			Specifies the row of interest in the "dot" bitmap, which must be less than
											the value returned from GetDotMapHeight().
				@return		The 16-bit "dot" bitmap of the row of the character glyph having the given offset.
			**/
			virtual UWord					GetGlyphRowDots (const NTV2GlyphIndex inGlyphIndex, const unsigned inRow) const;

			/**
				@brief		Returns the "ASCII-like" character code that best represents the given CEA-608 code point.
				@param[in]	in608CodePoint	Specifies the CEA-608 code point for which a CCFont character code is being requested.
				@return		The "ASCII-like" character code that best represents the given CEA-608 code point;
							otherwise, 0x0.
			**/
			virtual UByte					GetCCFontCharCode (const NTV2_CC608_CodePoint in608CodePoint) const;

			/**
				@brief		Returns the set of codepoints that map to a given CCFont character code.
				@param[in]	inCharCode		Specifies the CCFont character code of interest.
											Valid values are from 0x20 and less than NTV2_CCFont_NumChars+0x20.
				@return		The set of NTV2_CC608_CodePoint values that map to the given CCFont character code.
			**/
			virtual NTV2CodePointSet		GetCodePointsForCCFontCharCode (const UByte inCharCode) const;

			/**
				@brief		Returns true if the given glyph index is valid.
				@param[in]	inGlyphIndex	Specifies the glyph index of interest.
				@return		True if there is a glyph available for the given index;  otherwise false.
			**/
			virtual inline bool				IsValidGlyphIndex (const NTV2GlyphIndex inGlyphIndex) const		{return inGlyphIndex < GetGlyphCount ();}

			/**
				@brief		Returns the number of glyphs available in this caption font.
				@return		The number of glyphs in this caption font.
			**/
			virtual inline UWord			GetGlyphCount (void) const									{return mGlyphCount;}

			/**
				@brief		Returns the name of this caption font.
				@return		A string containing the name of this caption font.
			**/
			virtual inline std::string		GetName (void) const										{return mFontName;}

			/**
				@brief		Returns the underline character's zero-based glyph index number.
				@return		The zero-based glyph index of the underline character.
			**/
			virtual inline NTV2GlyphIndex	GetUnderlineGlyphIndex (void) const							{return mUnderlineGlyphIndex;}

			/**
				@brief		Returns the "no underline space" character's zero-based glyph index number.
				@return		The zero-based glyph index of the special "no underline space" character.
			**/
			virtual inline NTV2GlyphIndex	GetNoUnderlineSpaceGlyphIndex (void) const					{return mNoUnderlineSpaceGlyphIndex;}

			/**
				@brief		Returns the character code of the special underline glyph.
				@return		The character code of the special underline glyph.
			**/
			virtual inline UByte			GetUnderlineCharacterCode (void) const						{return GlyphIndexToCharacterCode (mUnderlineGlyphIndex);}

			/**
				@brief		Returns the character code of the special "underline space" glyph.
				@return		The character code of the special "underline space" glyph.
			**/
			virtual inline UByte			GetNoUnderlineSpaceCharacterCode (void) const				{return GlyphIndexToCharacterCode (mNoUnderlineSpaceGlyphIndex);}

			/**
				@brief		Returns the starting row position of the underline, which is below the dot map.
							Also takes into account the top margin, if any.
				@return		The underline row offset, taking into account the top margin (if any) and the dot map height.
			**/
			virtual inline UWord			GetUnderlineStartingDotRow (void) const						{return GetTopMarginDotCount () + GetDotMapHeight () + 2;}

			/**
				@brief		Returns the width of the dot map, in dots.
				@return		The width of the dot map, in dots.
			**/
			virtual inline UWord			GetDotMapWidth (void) const									{return mDotMapWidth;}

			/**
				@brief		Returns the number of dots of space to appear to the left of each blitted glyph.
				@return		The number of dots of space to appear to the left of each blitted glyph.
			**/
			virtual inline UWord			GetLeftMarginDotCount (void) const							{return mLeftMarginDotCount;}

			/**
				@brief		Returns the number of dots of space to appear to the right of each blitted glyph.
				@return		The number of dots of space to appear to the right of each blitted glyph.
			**/
			virtual inline UWord			GetRightMarginDotCount (void) const							{return mRightMarginDotCount;}

			/**
				@brief		Returns the total width, in dots, a blitted glyph will consume, including any left and/or right margin space.
				@return		The total width, in dots, a blitted glyph will consume.
			**/
			virtual inline UWord			GetTotalWidthInDots (void) const							{return GetDotMapWidth () + GetLeftMarginDotCount () + GetRightMarginDotCount ();}

			/**
				@brief		Returns the height of the dot map, in dot rows.
				@return		The height of the dot map, in dot rows.
			**/
			virtual inline UWord			GetDotMapRowCount (void) const								{return GetDotMapHeight ();}

			/**
				@brief		Returns the height of the dot map, in dots.
				@return		The height of the dot map, in dots.
			**/
			virtual inline UWord			GetDotMapHeight (void) const								{return mDotMapHeight;}

			/**
				@brief		Returns the number of dots of space to appear above each blitted glyph.
				@return		The number of dots of space to appear above each blitted glyph.
			**/
			virtual inline UWord			GetTopMarginDotCount (void) const							{return mTopMarginDotCount;}

			/**
				@brief		Returns the number of dots of space to appear below each blitted glyph.
				@return		The number of dots of space to appear below each blitted glyph.
			**/
			virtual inline UWord			GetBottomMarginDotCount (void) const						{return mBottomMarginDotCount;}

			/**
				@brief		Returns the total height, in dots, a blitted glyph will consume, including any top and/or bottom margin space.
				@return		The total height, in dots, a blitted glyph will consume.
			**/
			virtual inline UWord			GetTotalHeightInDots (void) const							{return GetDotMapHeight () + GetTopMarginDotCount () + GetBottomMarginDotCount ();}


			/**
				@brief						Returns a UTF-8 encoded string that contains the dot pattern for the given zero-based row number and zero-based glyph index.
											Each opaque (black) dot is rendered as a "full block" character (u2588), whereas each transparent (white) dot is rendered
											as a simple space character.
				@param[in]	inGlyphIndex	Specifies the glyph of interest, expressed as a zero-based index number.
											This must be less than the value returned from GetGlyphCount.
				@param[in]	inRow			Specifies the zero-based row of interest in the glyph's dot map.
											This must be less than the value returned from GetDotMapHeight.
				@return						The UTF-8 encoded string.
			**/
			virtual std::string				GetGlyphRowDotsAsString (const NTV2GlyphIndex inGlyphIndex, const unsigned inRow) const;


			/**
				@brief						Renders all glyphs in the given range (inclusive) into the specified output stream as multiple rows of UTF-8 encoded strings.
											When displayed in a terminal using a monospaced font, it shows what the glyphs actually look like.
											Each black dot is rendered as a "full block" character (u2588), while white dots are rendered as spaces.
				@param		inOutStream		Specifies the output stream to use.
				@param[in]	inFirstGlyph	Specifies the zero-based index of the first glyph to be printed into the output stream.
											This must be less than the value returned from GetGlyphCount, and also must be less than the value used in "inLastGlyph".
				@param[in]	inLastGlyph		Specifies the zero-based index of the last glyph to be printed into the output stream.
											This must be less than the value returned from GetGlyphCount.
				@return						The output stream that was passed into the "inOutStream" parameter.
			**/
			virtual std::ostream &			PrintGlyphs (std::ostream & inOutStream, const NTV2GlyphIndex inFirstGlyph, const NTV2GlyphIndex inLastGlyph) const;


			/**
				@brief	Renders the given glyph with the given display attributes into the specified 8-bit YCbCr '2vuy' destination buffer.
				@param		pDestBuffer			Specifies the destination buffer that is to receive the "blitted" glyph.
				@param[in]	inBytesPerRow		Specifies the number of bytes per row in the destination buffer.
				@param[in]	inGlyphIndex		Specifies the glyph to be blitted. Must be less than the given font's glyph count.
				@param[in]	inAttribs			Specifies the attributes the blitted glyph should be rendered with (color, italic, underline, etc.).
				@param[in]	inScaledDotWidth	Specifies the number of horizontal pixels per glyph "dot".
				@param[in]	inScaledDotHeight	Specifies the number of vertical pixels (lines) per glyph "dot".
			**/
			virtual bool					RenderGlyph8BitYCbCr (UByte *					pDestBuffer,
																	const ULWord			inBytesPerRow,
																	const NTV2GlyphIndex	inGlyphIndex,
																	const NTV2Line21Attrs &	inAttribs,
																	const ULWord			inScaledDotWidth = 1,
																	const ULWord			inScaledDotHeight = 1) const;


			/**
				@brief	Renders the given glyph with the given display attributes into the specified 8-bit NTV2_FBF_ARGB destination buffer.
				@param[in]	inPixelFormat		Specifies which 8-bit RGB pixel format the destination buffer has. Must be one of NTV2_FBF_ARGB,
												NTV2_FBF_RGBA or NTV2_FBF_ABGR.
				@param		pDestBuffer			Specifies the destination buffer that is to receive the "blitted" glyph.
				@param[in]	inBytesPerRow		Specifies the number of bytes per row in the destination buffer.
				@param[in]	inGlyphIndex		Specifies the glyph to be blitted. Must be less than the given font's glyph count.
				@param[in]	inAttribs			Specifies the attributes the blitted glyph should be rendered with (color, italic, underline, etc.).
				@param[in]	inScaledDotWidth	Specifies the number of horizontal pixels per glyph "dot".
				@param[in]	inScaledDotHeight	Specifies the number of vertical pixels (lines) per glyph "dot".
				@param[in]	inIsHD				Use true for Rec709 color translation;  otherwise false for Rec601. Defaults to false (Rec601).
			**/
			virtual bool					RenderGlyph8BitRGB (const NTV2PixelFormat	inPixelFormat,
																UByte *					pDestBuffer,
																const ULWord			inBytesPerRow,
																const NTV2GlyphIndex	inGlyphIndex,
																const NTV2Line21Attrs &	inAttribs,
																const ULWord			inScaledDotWidth,
																const ULWord			inScaledDotHeight,
																const bool				inIsHD = false) const;

			explicit						NTV2CCFont ();
			virtual 						~NTV2CCFont ();

		private:
			//	Hidden assignment operator to quiet MSVC warning C4512
			virtual inline NTV2CCFont &		operator = (const NTV2CCFont & inRHS)			{(void) inRHS;  return *this;}

		//	INSTANCE DATA
		private:
			const UWord				mGlyphCount;					///< @brief	Total number of glyphs in this font
			const NTV2GlyphIndex	mUnderlineGlyphIndex;			///< @brief	The zero-based index number of the special "underline" glyph
			const NTV2GlyphIndex	mNoUnderlineSpaceGlyphIndex;	///< @brief	The zero-based index number of the special "no underline space" glyph
			const UWord				mDotMapWidth;					///< @brief	The width of the glyph dot map (not including left or right margin spacing)
			const UWord				mLeftMarginDotCount;			///< @brief	The number of dots of space to appear to the left of each blitted glyph
			const UWord				mRightMarginDotCount;			///< @brief	The number of dots of space to appear to the right of each blitted glyph
			const UWord				mDotMapHeight;					///< @brief	The height of the glyph dot map (not including top or bottom margin spacing)
			const UWord				mTopMarginDotCount;				///< @brief	The number of dots of space to appear above each blitted glyph
			const UWord				mBottomMarginDotCount;			///< @brief	The number of dots of space to appear below each blitted glyph
			const std::string		mFontName;						///< @brief	The name of this font

	};	//	NTV2CCFont


	/**
		@brief	Dumps all glyphs in the given CC font to stderr in UTF-8.
				When displayed in a terminal using a monospaced font, it shows what the glyphs actually look like.
				Each black dot is rendered as a "full block" character (u2588), while white dots are rendered as spaces.
		@param	inCCFont	Specifies the font to dump.
	**/
	void DumpCCFont (const NTV2CCFont & inCCFont = NTV2CCFont::GetInstance ());

#endif	//	CCFONT_H
