/**
	@file		ntv2captionrenderer.h
	@brief		Declares the CNTV2CaptionRenderer class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CAPTIONRENDERER__
#define __NTV2_CAPTIONRENDERER__

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2caption608types.h"
#include "ntv2formatdescriptor.h"
#include "ccfont.h"
#include "ajabase/common/ajarefptr.h"
#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#else
	#include <stdint.h>
#endif


/**
	@brief	I retain a set of pre-rendered glyphs in any desired display attribute for a given pixel format
			and destination frame size, which can be readily "blitted" into an NTV2 frame buffer.
**/
class CNTV2CaptionRenderer;
typedef AJARefPtr <CNTV2CaptionRenderer>	CNTV2CaptionRendererPtr;

class AJAExport CNTV2CaptionRenderer
{
	//	CLASS METHODS
	public:
		/**
			@brief		Returns the CNTV2CaptionRenderer instance using the given video frame buffer format descriptor,
						creating the cache if necessary.
			@param[in]	inFBDescriptor	Describes the video frame buffer.
			@param[in]	inAutoOpen		If true, and the cache needed to be created, it's automatically opened.
			@return		The cache instance.
		**/
		static CNTV2CaptionRendererPtr		GetRenderer (const NTV2FormatDesc & inFBDescriptor,
														 const bool inAutoOpen = true);		//	New in SDK 16.0

		/**
			@brief		Flushes all extant glyph caches.
		**/
		static bool							FlushGlyphCaches (void);

		/**
			@brief		Blits the given 608/ASCII-ish character into a specified position in the given host buffer
						using the given attributes.
			@param[in]	inCharCode	Specifies the character code to be blitted.
			@param[in]	inAttribs	Specifies the desired display attributes (FG color, BG color, italics, etc.).
			@param		inFB		Specifies the host frame buffer to be blitted into.
			@param[in]	inFD		Describes the host frame buffer raster and pixel format.
			@param[in]	inXPos		Specifies the horizontal raster offset to the left edge of the character to be blitted, in pixels.
			@param[in]	inYPos		Specifies the vertical raster offset to the top edge of the character to be blitted, in lines.
			@return		True if successful;  otherwise false.
		**/
		static bool							BurnChar (	const UByte				inCharCode,
														const NTV2Line21Attrs &	inAttribs,
														NTV2Buffer &			inFB,
														const NTV2FormatDesc &	inFD,
														const UWord				inXPos,
														const UWord				inYPos);	//	New in SDK 16.0

		/**
			@brief		Blits the contents of the given UTF-8 encoded string into the given host buffer using the
						given display attributes and starting position, overwriting any existing "on-air" captions.
			@param[in]	inString			Specifies the UTF-8 encoded string to be blitted into the video buffer.
											Unicode codepoints that aren't supported by the caption font in use will
											be rendered as a "full block".
			@param[in]	inAttribs			Specifies the display attributes of the rendered characters.
			@param		inFB				Specifies the host frame buffer to be blitted into.
			@param[in]	inFBDescriptor		Describes the host frame buffer raster and pixel format.
			@param[in]	inRowNum			Specifies the row number at which character rendering will begin.
											The value will be clamped such that it is between 1 and 15 (inclusive).
											Defaults to 15, the bottom-most row position.
			@param[in]	inColumnNum			Specifies the column number at which character rendering will begin.
											The value will be clamped such that it is between 1 and 32 (inclusive).
											Defaults to 1, the left-most column position.
			@return		True if successful;  otherwise False.
			@note		Wrapping and scrolling is not performed. If the string's length exceeds the rightmost screen column,
						the rightmost character position will continue to be overwritten until the last character from the
						string remains there.
		**/
		static bool							BurnString (const std::string &		inString,
														const NTV2Line21Attrs &	inAttribs,
														NTV2Buffer &			inFB,
														const NTV2FormatDesc &	inFBDescriptor,
														const UWord				inRowNum	= 15,
														const UWord				inColumnNum	= 1);	//	New in SDK 16.0

		/**
			@brief		Blits the contents of the given UTF-8 encoded string into the given host buffer using the
						given display attributes and starting position.
			@param[in]	inString		Specifies the UTF-8 encoded string to be blitted into the host buffer.
										Unicode codepoints that aren't supported by the caption font in use will
										be rendered as a "full block".
			@param[in]	inAttribs		Specifies the display attributes of the rendered characters.
			@param		inFB			Specifies the host frame buffer to be blitted into.
			@param[in]	inFBDescriptor	Describes the host frame buffer raster and pixel format.
			@param[in]	inXPos			Specifies the horizontal pixel offset at which character blitting will begin.
			@param[in]	inYPos			Specifies the vertical pixel offset at which character blitting will begin.
			@return		True if successful;  otherwise False.
			@note		Wrapping and scrolling is not performed. Anything past the raster's right-hand and/or bottom edge(s)
						will be clipped.
		**/
		static bool							BurnStringAtXY (const std::string &		inString,
															const NTV2Line21Attrs &	inAttribs,
															NTV2Buffer &			inFB,
															const NTV2FormatDesc &	inFBDescriptor,
															const UWord				inXPos,
															const UWord				inYPos);
	//	INSTANCE METHODS
	public:
		/**
			@brief		Opens me, and prepares me using my current pixel format and CC Font settings.
			@return		True if successful;  otherwise false.
			@note		Calling this when I'm already open will close and re-open me with the new format.
		**/
		virtual bool					Open (void);

		/**
			@brief		Closes me, releasing my resources. Once closed, I can no longer be used to render characters.
			@return		True if successful;  otherwise false.
			@note		It is not an error to call this function when I'm already closed.
		**/
		virtual bool					Close (void);

		/**
			@brief		Answers true if I'm currently open.
			@return		True if I'm currently open and able to render characters;  otherwise false.
		**/
		virtual inline bool				IsOpen (void) const							{return mGlyphHeightInLines && mGlyphWidthInBytes && mGlyphWidthInPixels;}

		/**
			@brief		Returns the frame buffer format that I'm currently "open" for.
			@return		The frame buffer format that I'm currently "open" for.
		**/
		virtual inline NTV2PixelFormat	GetPixelFormat (void) const					{return mPixelFormat;}

		/**
			@return		The width of my rendered glyphs as a byte count.
		**/
		virtual inline UWord			GetGlyphWidthInBytes (void) const			{return mGlyphWidthInBytes;}

		/**
			@return		The width, in pixels, of any of my rendered glyphs.
		**/
		virtual inline UWord			GetGlyphWidthInPixels (void) const			{return mGlyphWidthInPixels;}

		/**
			@return		The height, in lines, of any of my rendered glyphs.
		**/
		virtual inline UWord			GetGlyphHeightInLines (void) const			{return mGlyphHeightInLines;}

		/**
			@brief		Returns the equivalent number of horizontal raster pixels that correspond to the given number of glyph dots.
			@param[in]	inVHeightInGlyphDots	Specifies the height to be scaled, in glyph dots.
			@return		The number of horizontal raster pixels that are equivalent to the given number of glyph dots.
		**/
		virtual inline UWord			GlyphDotHeightToRasterLines (const UWord inVHeightInGlyphDots) const	{return inVHeightInGlyphDots * mVRasterLinesPerGlyphDot;}

		/**
			@brief		Returns the equivalent number of horizontal raster pixels that correspond to the given number of glyph dots.
			@param[in]	inHWidthInGlyphDots	Specifies the width to be scaled, in glyph dots.
			@return		The number of horizontal raster pixels that are equivalent to the given number of glyph dots.
		**/
		virtual inline UWord			GlyphDotWidthToRasterPixels (const UWord inHWidthInGlyphDots) const		{return inHWidthInGlyphDots * mHRasterPixelsPerGlyphDot;}

		/**
			@brief		Returns the raster position of the top-left corner of a given CEA-608 caption row and column.
			@param[in]	in608CaptionRow		Specifies the CEA-608 caption row, which must be between 1 and 15.
			@param[in]	in608CaptionCol		Specifies the CEA-608 caption column, which must be between 1 and 32.
			@param[out]	outVertLineOffset	Receives the raster line offset from the top raster edge that corresponds to the top edge of the given caption row.
											Receives zero if the function fails.
			@param[out]	outHorzPixelOffset	Receives the raster pixel offset from the left raster edge that corresponds to the left edge of the given caption column.
											Receives zero if the function fails.
			@return		True if successful;  otherwise false.
		**/
		virtual bool					GetCharacterRasterOrigin (const UWord in608CaptionRow, const UWord in608CaptionCol,
																	UWord & outVertLineOffset, UWord & outHorzPixelOffset) const;

		/**
			@brief		Returns a const pointer to the raster that contains the preloaded glyph renderings for the given attribute.
			@param[in]	inAttrs			Specifies the desired display attributes.
			@return		A valid, non-NULL const pointer to the specified glyph family's raster image;  or NULL if failed.
			@note		This can be a time-consuming function call, as it causes all CCFont glyphs to be rendered with the given attributes
						for my current pixel format and frame dimensions.
		**/
		virtual const UByte *			GetPreloadedGlyphsRasterAddress (const NTV2Line21Attrs & inAttrs) const;

		/**
			@brief		Returns a copy of the glyphs bitmap raster pixel data for the given attribute.
			@param[out]	outBuffer			Receives the copy of the glyphs bitmap raster pixel data.
			@param[in]	inAttrs				Specifies the desired character display attributes.
			@return		True if successful;  otherwise false upon failure.
		**/
		virtual bool					GetGlyphsRaster (NTV2Buffer & outBuffer, const NTV2Line21Attrs & inAttrs) const;	//	New in SDK 16.0

		/**
			@return		A format descriptor of the glyphs bitmap raster.
		**/
		virtual NTV2FormatDescriptor	GetFormatDescriptor (void) const;	//	New in SDK 16.0

		/**
			@return		The number of bytes per row in the raster that contains the preloaded glyph renderings.
		**/
		virtual inline ULWord			GetPreloadedGlyphsRasterRowBytes (void) const			{return ULWord(mGlyphWidthInBytes) * ULWord(mCCFont.GetGlyphCount());}

		/**
			@return		The height, in lines, of the raster that contains the preloaded glyph renderings.
		**/
		virtual inline UWord			GetPreloadedGlyphsRasterHeightInLines (void) const		{return GetGlyphHeightInLines();}

		/**
			@return		The width, in pixels, of the raster that contains the preloaded glyph renderings.
		**/
		virtual inline UWord			GetPreloadedGlyphsRasterWidthInPixels (void) const		{return GetGlyphWidthInPixels() * mCCFont.GetGlyphCount();}

		/**
			@return		The total memory consumed by my preloaded glyph renderings, in bytes.
		**/
		virtual inline ULWord			GetTotalBytes (void) const								{return mBytesPerAttribute * GetNumActivePreloadedGlyphsRasters();}

		/**
			@return		The number of active, pre-loaded glyph renderings.
		**/
		virtual inline ULWord			GetNumActivePreloadedGlyphsRasters (void) const			{return ULWord(mpRasters.size());}

		/**
			@return		A list of NTV2Line21Attributes of each of my active, pre-loaded glyph renderings.
		**/
		virtual ULWordSequence			GetActivePreloadedGlyphsRastersAttributes (void) const;

		/**
			@brief	Emits my human-readable representation into the given output stream.
			@param[in]	inOutStream			The output stream that is to receive my human-readable representation.
			@param[in]	inIncludeActives	Optionally specifies if active permutations (with attributes)
											should be included in the dump. Defaults to false.
			@return		The output stream that was used.
		**/
		virtual std::ostream &			Print (std::ostream & inOutStream, const bool inIncludeActives = false) const;

		/**
			@brief	My destructor.
		**/
		virtual 						~CNTV2CaptionRenderer ();


	//	PRIVATE METHODS
	private:
		//	Hidden constructors & assignment operators
		/**
			@brief		Constructs me from a given pixel format, CC font, and intended frame dimensions.
			@param[in]	inPixelFormat		Specifies the frame buffer format to be used.
			@param[in]	inCCFont			Specifies the font to be used.
			@param[in]	inFrameDimensions	Specifies the intended frame dimensions to be used, which determines any scale factor to be used.
		**/
		explicit						CNTV2CaptionRenderer (	const NTV2PixelFormat	inPixelFormat,
																const NTV2CCFont &		inCCFont,
																const NTV2FrameSize &	inFrameDimensions);

		explicit inline					CNTV2CaptionRenderer (const CNTV2CaptionRenderer & inRendererToCopy);

		virtual CNTV2CaptionRenderer &	operator = (const CNTV2CaptionRenderer & inRendererToCopy);

		//	Per-Attribute Preloaded Glyphs Rasters
		virtual bool					CreatePreloadedGlyphsRasterForAttribute (const NTV2Line21Attributes & inAttribs) const;
		virtual bool					HasPreloadedGlyphsRasterForAttribute (const NTV2Line21Attributes & inAttribs) const;
		virtual const UByte *			GetPreloadedGlyphsRasterForAttribute (const NTV2Line21Attributes & inAttribs) const;


#if !defined(NTV2_DEPRECATE_16_0)	//	Old APIs
	static NTV2_DEPRECATED_16_0(CNTV2CaptionRendererPtr GetRenderer (const NTV2PixelFormat pf, const NTV2FrameDimensions fd, const bool autoOpen = true));	///< @deprecated	Use the overloaded method that accepts an NTV2FormatDescriptor instead.
	static NTV2_DEPRECATED_16_0(bool BurnChar (const UByte charCode, const NTV2Line21Attrs & att, UByte* fb, const NTV2FrameDimensions fd, const NTV2PixelFormat pf, const UWord x, const UWord y, const UWord bpr));	///< @deprecated	Use the overloaded method that accepts an NTV2Buffer and NTV2FormatDescriptor instead.
	static NTV2_DEPRECATED_16_0(bool BurnString (const std::string & st, const NTV2Line21Attrs& att, UByte* fb, const NTV2FrameDimensions fd, const NTV2PixelFormat pf, const UWord bpr, const UWord r = 15, const UWord c = 1));	///< @deprecated	Use the overloaded method that accepts an NTV2Buffer and NTV2FormatDescriptor instead.
	static NTV2_DEPRECATED_16_0(bool BurnStringAtXY (const std::string & st, const NTV2Line21Attrs & att, UByte* fb, const NTV2FrameDimensions fd, const NTV2PixelFormat pf, const UWord bpr, const UWord x, const UWord y));	/// @deprecated	Use the overloaded method that accepts an NTV2Buffer and NTV2FormatDescriptor instead.
#endif	//	!defined(NTV2_DEPRECATE_16_0)

	//	PRIVATE CLASS METHODS
	private:
		/**
			@brief		Creates and opens a new CNTV2CaptionRenderer instance for a given frame buffer format and CCFont.
			@param[out]	outCache			Receives the newly-created cache instance.
			@param[in]	inPixelFormat		Specifies the frame buffer format to be used.
			@param[in]	inCCFont			Specifies the font to be used.
			@param[in]	inFrameDimensions	Specifies the intended frame dimensions to be used.
											This determines the scale factor to be used (if any).
			@return		True if successful; otherwise False.
		**/
		static bool						Create (CNTV2CaptionRendererPtr &	outCache,
												const NTV2PixelFormat		inPixelFormat,
												const NTV2CCFont &			inCCFont,
												const NTV2FrameSize &		inFrameDimensions);

	//	INSTANCE DATA
	private:
		typedef	std::map <ULWord, UByte *>			AttribToBufferMap;
		typedef	AttribToBufferMap::iterator			AttribToBufferMapIter;
		typedef	AttribToBufferMap::const_iterator	AttribToBufferMapConstIter;

		const NTV2CCFont			mCCFont;					///< @brief	The caption font I'm using
		NTV2PixelFormat				mPixelFormat;				///< @brief	My pixel format
		mutable AttribToBufferMap	mpRasters;					///< @brief	My array of rasters containing pre-rendered glyphs
		UWord						mHRasterPixelsPerGlyphDot;	///< @brief	Horizontal scaling factor -- raster pixels per glyph dot
		UWord						mVRasterLinesPerGlyphDot;	///< @brief	Vertical scaling factor -- raster lines per glyph dot
		UWord						mVCaptionRasterOrigin;		///< @brief	Vertical raster line offset of top edge of caption drawing area
		UWord						mHCaptionRasterOrigin;		///< @brief	Horizontal raster pixel offset of left edge of caption drawing area
		UWord						mGlyphWidthInPixels;		///< @brief	Fully scaled glyph width, in pixels
		UWord						mGlyphWidthInBytes;			///< @brief	Fully scaled glyph width, in bytes (this is "bytes per row" for the raster)
		UWord						mGlyphHeightInLines;		///< @brief	Fully scaled glyph height, in lines
		ULWord						mBytesPerAttribute;			///< @brief	Total bytes per attribute
		ULWord						mTotalBytes;				///< @brief	Size of my cache raster, in bytes
		NTV2FrameSize				mFrameDimensions;			///< @brief	Target raster dimensions (used to determine font size)

};	//	CNTV2CaptionRenderer

AJAExport std::ostream & operator << (std::ostream & inOutStream, const CNTV2CaptionRendererPtr & inObjPtr);
AJAExport std::ostream & operator << (std::ostream & inOutStream, const CNTV2CaptionRenderer & inObj);

#endif	// __NTV2_CAPTIONRENDERER__
