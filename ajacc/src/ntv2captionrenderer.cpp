/**
	@file		ntv2captionrenderer.cpp
	@brief		Implements the NTV2CaptionRenderer class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include "ntv2captionrenderer.h"
#include "ntv2captionlogging.h"
#include "ntv2utils.h"
#include "ntv2transcode.h"
#include "ajabase/system/debug.h"
#include <string.h>	//	for memset

using namespace std;

#define MYERR(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CCFont, AJA_DebugSeverity_Error, AJAFUNC << ": " << __xpr__)
#define MYWARN(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CCFont, AJA_DebugSeverity_Warning, AJAFUNC << ": " << __xpr__)
#define MYNOTE(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CCFont, AJA_DebugSeverity_Notice, AJAFUNC << ": " << __xpr__)
#define MYINFO(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CCFont, AJA_DebugSeverity_Info, AJAFUNC << ": " << __xpr__)
#define MYDBG(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CCFont, AJA_DebugSeverity_Debug, AJAFUNC << ": " << __xpr__)


class FBFSize
{
	public:
		inline FBFSize (const NTV2PixelFormat inPixelFormat, const NTV2FrameSize inFrameDimensions)
			:	mFBF(inPixelFormat), mDimensions(inFrameDimensions)
			{}
		inline FBFSize (const FBFSize & inFBFSize)
			:	mFBF(inFBFSize.mFBF), mDimensions(inFBFSize.mDimensions)
			{}
		inline bool	operator < (const FBFSize & inRHS) const
		{
			const ULWord64 LHS ((ULWord64(mFBF      ) << 48) | (ULWord64(mDimensions.width()      ) << 24) | (ULWord64(mDimensions.height()      ) & 0x0000000000FFFFFF));
			const ULWord64 RHS ((ULWord64(inRHS.mFBF) << 48) | (ULWord64(inRHS.mDimensions.width()) << 24) | (ULWord64(inRHS.mDimensions.height()) & 0x0000000000FFFFFF));
			return LHS < RHS;
		}
		inline bool	operator == (const FBFSize & inRHS) const
		{
			return mFBF == inRHS.mFBF  &&  mDimensions.width() == inRHS.mDimensions.width()  &&  mDimensions.height() == inRHS.mDimensions.height();
		}
		inline ULWord			GetWidth (void) const		{return mDimensions.width();}
		inline ULWord			GetHeight (void) const		{return mDimensions.height();}
		inline NTV2PixelFormat	GetFormat (void) const		{return mFBF;}
	private:
		NTV2PixelFormat	mFBF;
		NTV2FrameSize	mDimensions;

};	//	FBFSize


typedef	std::pair <FBFSize, CNTV2CaptionRendererPtr>	FBFSizeToRendererPair;			//	Insertion key
typedef	std::map <FBFSize, CNTV2CaptionRendererPtr>		FBFSizeToRendererMap;			//	Map
typedef FBFSizeToRendererMap::iterator					FBFSizeToRendererMapIter;		//	Map iterator
typedef FBFSizeToRendererMap::const_iterator			FBFSizeToRendererMapConstIter;	//	Map iterator

static FBFSizeToRendererMap						gRenderers;
static const NTV2Line21AttributePermutations	gAllAttributePermutations;


#if !defined(NTV2_DEPRECATE_16_0)	//	Old APIs
	CNTV2CaptionRendererPtr CNTV2CaptionRenderer::GetRenderer (	const NTV2PixelFormat inPixelFormat,
																const NTV2FrameDimensions inFrameDimensions,
																const bool inAutoOpen)
	{
		const FBFSize					frameKind	(inPixelFormat, inFrameDimensions);
		FBFSizeToRendererMapConstIter	iter		(gRenderers.find(frameKind));
		CNTV2CaptionRendererPtr			result;

		if (iter == gRenderers.end())
		{
			Create (result, inPixelFormat, NTV2CCFont::GetInstance(), inFrameDimensions);
			if (result)
			{
				gRenderers.insert(FBFSizeToRendererPair (frameKind, result));
				if (inAutoOpen)
				{
					result->Open();
					MYDBG("Caption renderer created for " << inFrameDimensions << " " << ::NTV2FrameBufferFormatToString(inPixelFormat) << ":  " << result);
				}
			}
		}
		else
		{
			result = iter->second;
			if (result)
			{
				const FBFSize	fk	(iter->first);
				AJACC_ASSERT (fk.GetFormat() == inPixelFormat);
				AJACC_ASSERT (fk.GetWidth() == inFrameDimensions.GetWidth());
				AJACC_ASSERT (fk.GetHeight() == inFrameDimensions.GetHeight());
			}
		}
		if (!result)
			MYDBG("Returning null renderer for " << inFrameDimensions << " " << ::NTV2FrameBufferFormatToString(inPixelFormat));
		return result;

	}	//	GetRenderer

	bool CNTV2CaptionRenderer::BurnChar (const UByte				inCharacterCode,		//	ASCII-ish character code to draw
										const NTV2Line21Attrs &		inAttribs,				//	attributes (italic? underline? color?)
										UByte *						pFrameBuffer,			//	pointer to first line of host frame buffer
										const NTV2FrameDimensions	inFBPixelDimensions,	//	dimensions of frame buffer raster (pixels wide x lines tall)
										const NTV2PixelFormat		inFBFormat,				//	pixel format of frame buffer raster
										const UWord					inXPos,					//	horiz pixel offset from left edge into frame buffer raster
										const UWord					inYPos,					//	vert line offset from top edge into frame buffer raster
										const UWord					inFBBytesPerRow)		//	frame buffer raster bytes-per-row
	{
		const NTV2CCFont &		ccFont		(NTV2CCFont::GetInstance());
		NTV2GlyphIndex			glyphIndex	(NTV2CCFont::CharacterCodeToGlyphIndex(0x7f));	//	Default = "block"
		NTV2FrameGeometry		fg			(::GetGeometryFromFrameDimensions(inFBPixelDimensions));
		NTV2Standard			st			(::GetStandardFromGeometry(fg));
		NTV2FormatDesc			fd			(st, inFBFormat, ::GetVANCModeForGeometry(fg));
		CNTV2CaptionRendererPtr	glyphCache	(CNTV2CaptionRenderer::GetRenderer(fd));
		bool					bResult		(false);

		if (!glyphCache)
			return false;

		if (ccFont.IsValidGlyphIndex(NTV2CCFont::CharacterCodeToGlyphIndex(inCharacterCode)))
			glyphIndex = NTV2CCFont::CharacterCodeToGlyphIndex(inCharacterCode);

		bResult = ::CopyRaster (inFBFormat,												//	Pixel format of both src and dst buffers
								pFrameBuffer,											//	Dest buffer to be modified
								ULWord(inFBBytesPerRow),								//	Dest buffer bytes per raster line (determines max width)
								UWord(inFBPixelDimensions.Height()),					//	Dest buffer total lines in raster (max height)
								inYPos,													//	Vertical line offset into the dest raster where the top edge of the src image will appear
								inXPos,													//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
								glyphCache->GetPreloadedGlyphsRasterAddress(inAttribs),	//	Src buffer
								glyphCache->GetPreloadedGlyphsRasterRowBytes(),			//	Src buffer raster row bytes (determines max width)
								glyphCache->GetPreloadedGlyphsRasterHeightInLines(),	//	Src buffer raster height (max)
								0,														//	Top edge of src image to copy
								glyphCache->GetGlyphHeightInLines(),					//	Height of src image to copy
								glyphCache->GetGlyphWidthInPixels() * glyphIndex,		//	Left edge of src image to copy
								glyphCache->GetGlyphWidthInPixels());					//	Width of src image to copy
		if (!bResult)
			MYWARN("CopyRaster failed at Y=" << inYPos << ", X=" << inXPos << ", " << glyphCache);
		return bResult;
	}	//	BurnChar

	bool CNTV2CaptionRenderer::BurnString (const string &				inString,
											const NTV2Line21Attrs &		inAttrs,
											UByte *						pBaseVideoAddress,
											const NTV2FrameDimensions	inFBPixelDimensions,
											const NTV2PixelFormat		inFBFormat,
											const UWord					inBytesPerRow,
											const UWord					inRow,
											const UWord					inCol)
	{
		if (inString.empty())
			return true;
		if (!pBaseVideoAddress)
			return false;

		const string			theString	(NTV2CCFont::Utf8ToCCFontByteArray(inString));
		const size_t			numChars	(theString.length());
		const NTV2FrameGeometry	fg			(::GetGeometryFromFrameDimensions(inFBPixelDimensions));
		const NTV2Standard		standard	(::GetStandardFromGeometry(fg));
		const NTV2FormatDesc	fd			(standard, inFBFormat, ::GetVANCModeForGeometry(fg));
		const UWord				row			(inRow < NTV2_CC608_MinRow ? NTV2_CC608_MinRow : (inRow > NTV2_CC608_MaxRow ? NTV2_CC608_MaxRow : inRow));	//	Clamp row
		UWord					col			(inCol < NTV2_CC608_MinCol ? NTV2_CC608_MinCol : (inCol > NTV2_CC608_MaxCol ? NTV2_CC608_MaxCol : inCol));	//	Clamp column
		NTV2Buffer				vidBuf		(pBaseVideoAddress, fd.GetTotalBytes());
		CNTV2CaptionRendererPtr	renderer	(CNTV2CaptionRenderer::GetRenderer(fd));

		if (!renderer)
			return false;

		for (size_t charNdx(0);  charNdx < numChars;  charNdx++)
		{
			UWord yPos(0), xPos(0);
			if (!renderer->GetCharacterRasterOrigin (row, col, yPos, xPos))
				return false;
			if (!renderer->BurnChar(UByte(theString.at(charNdx)), inAttrs, vidBuf, fd, xPos, yPos))
				return false;
			if (col < NTV2_CC608_MaxCol)	//	Any room to increment to the next column?
				col++;						//	No -- any remaining characters will overwrite the last column)
		}	//	for each character in the string
		return true;
	}	//	BurnString

	bool CNTV2CaptionRenderer::BurnStringAtXY (const string &				inString,
												const NTV2Line21Attrs &		inAttribs,
												UByte *						pBaseVideoAddress,
												const NTV2FrameDimensions	inFBPixelDimensions,
												const NTV2PixelFormat		inFBFormat,
												const UWord					inBytesPerRow,
												const UWord					inXPos,	//	pixel offset from left edge into frame buffer raster
												const UWord					inYPos)	//	line offset from top edge into frame buffer raster
	{
		if (inString.empty ())
			return true;

		const string			theString	(NTV2CCFont::Utf8ToCCFontByteArray(inString));
		const size_t			numChars	(theString.length());
		const NTV2FrameGeometry	fg			(::GetGeometryFromFrameDimensions(inFBPixelDimensions));
		const NTV2Standard		standard	(::GetStandardFromGeometry(fg));
		const NTV2FormatDesc	fd			(standard, inFBFormat, ::GetVANCModeForGeometry(fg));
		CNTV2CaptionRendererPtr	renderer	(CNTV2CaptionRenderer::GetRenderer(fd));
		UWord					xPos		(inXPos);
		const UWord				charWidth	(renderer ? renderer->GetGlyphWidthInPixels() : 0);
		const UWord				maxPixWdth	(UWord(fd.GetRasterWidth()));
		NTV2Buffer				vidBuf		(pBaseVideoAddress, fd.GetTotalBytes());

		if (!renderer)
			return false;

		for (unsigned charNdx(0);  charNdx < numChars;  charNdx++)
		{
			renderer->BurnChar (UByte(theString.at(charNdx)), inAttribs, vidBuf, fd, xPos, inYPos);
			if (xPos >= maxPixWdth)	//	Any room to increment to the next column?
				break;				//	Offscreen -- bail
			xPos += charWidth;
		}	//	for each character in the string
		return true;

	}	//	BurnStringAtXY
#endif	//	!defined(NTV2_DEPRECATE_16_0)	//	Old APIs

CNTV2CaptionRendererPtr CNTV2CaptionRenderer::GetRenderer (const NTV2FormatDesc & inFD, const bool inAutoOpen)
{
	CNTV2CaptionRendererPtr	result;
	if (inFD.IsValid()  &&  NTV2_IS_VALID_FBF(inFD.GetPixelFormat()))
	{
		const NTV2PixelFormat	pixelFormat	(inFD.GetPixelFormat());
		const NTV2FrameSize		frameDims	(inFD.GetVisibleRasterDimensions());
		const FBFSize			frameKind	(pixelFormat, frameDims);
		FBFSizeToRendererMapConstIter	iter(gRenderers.find(frameKind));
		if (iter == gRenderers.end())
		{
			Create (result, pixelFormat, NTV2CCFont::GetInstance(), frameDims);
			if (result)
			{
				gRenderers.insert(FBFSizeToRendererPair (frameKind, result));
				if (inAutoOpen)
				{
					result->Open();
					MYDBG("Caption renderer created for " << frameDims << " " << ::NTV2FrameBufferFormatToString(pixelFormat) << ":  " << result);
				}
			}
		}
		else
		{
			result = iter->second;
			if (result)
			{
				const FBFSize	fk	(iter->first);
				AJACC_ASSERT (fk.GetFormat() == pixelFormat);
				AJACC_ASSERT (fk.GetWidth() == frameDims.width());
				AJACC_ASSERT (fk.GetHeight() == frameDims.height());
			}
		}
		if (!result)
			MYDBG("Returning null renderer for " << frameDims << " " << ::NTV2FrameBufferFormatToString(pixelFormat));
	}
	else
		MYDBG("Returning null renderer for invalid format descriptor");
	return result;
}


bool CNTV2CaptionRenderer::FlushGlyphCaches (void)
{
	MYNOTE(gRenderers.size() << " glyph cache(s) flushed");
	gRenderers.clear();
	return true;

}	//	FlushGlyphCaches

typedef UWordSequence NTV2GlyphIndexes;

static NTV2GlyphIndexes CharCodesToGlyphIndexes (const string & inCharCodes)
{
	NTV2GlyphIndexes result;
	const NTV2CCFont & ccFont(NTV2CCFont::GetInstance());
	for (size_t pos(0);  pos < inCharCodes.size();  pos++)
	{	const NTV2GlyphIndex glyphNdx(NTV2CCFont::CharacterCodeToGlyphIndex(UByte(inCharCodes.at(pos))));
		result.push_back (ccFont.IsValidGlyphIndex(glyphNdx) ? glyphNdx : 0x007f);	//	Use "block" if glyphNdx invalid
	}
	return result;
}

bool CNTV2CaptionRenderer::BurnChar (const UByte			inCharCode,	//	ASCII-ish character code to draw
									const NTV2Line21Attrs &	inAttribs,	//	attributes (italic? underline? color?)
									NTV2Buffer &			inFB,		//	pointer to first line of host buffer
									const NTV2FormatDesc &	inFD,		//	describes raster & pixel format of host buffer
									const UWord				inXPos,		//	horiz pixel offset from left edge into frame buffer raster
									const UWord				inYPos)		//	vert line offset from top edge into frame buffer raster
{
	const NTV2CCFont &		ccFont		(NTV2CCFont::GetInstance());
	NTV2GlyphIndex			glyphIndex	(NTV2CCFont::CharacterCodeToGlyphIndex(0x7f));	//	Default = "block"
	CNTV2CaptionRendererPtr	glyphCache	(CNTV2CaptionRenderer::GetRenderer(inFD));
	bool					bResult		(false);

	if (!glyphCache)
		return false;

	if (ccFont.IsValidGlyphIndex(NTV2CCFont::CharacterCodeToGlyphIndex(inCharCode)))
		glyphIndex = NTV2CCFont::CharacterCodeToGlyphIndex(inCharCode);

	bResult = ::CopyRaster (inFD.GetPixelFormat(),									//	Pixel format of both src and dst buffers
							(UByte*)inFD.GetRowAddress(inFB.GetHostAddress(0),inFD.GetFirstActiveLine()),	//	Dest buffer to be modified
							inFD.GetBytesPerRow(),									//	Dest buffer bytes per raster line (determines max width)
							UWord(inFD.GetVisibleRasterHeight()),					//	Dest buffer total lines in raster (max height)
							inYPos,													//	Vertical line offset into the dest raster where the top edge of the src image will appear
							inXPos,													//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
							glyphCache->GetPreloadedGlyphsRasterAddress(inAttribs),	//	Src buffer
							glyphCache->GetPreloadedGlyphsRasterRowBytes(),			//	Src buffer raster row bytes (determines max width)
							glyphCache->GetPreloadedGlyphsRasterHeightInLines(),	//	Src buffer raster height (max)
							0,														//	Top edge of src image to copy
							glyphCache->GetGlyphHeightInLines(),					//	Height of src image to copy
							glyphCache->GetGlyphWidthInPixels() * glyphIndex,		//	Left edge of src image to copy
							glyphCache->GetGlyphWidthInPixels());					//	Width of src image to copy
	if (!bResult)
		MYWARN("CopyRaster failed at Y=" << inYPos << ", X=" << inXPos << ", " << glyphCache);
	return bResult;
}	//	BurnChar


bool CNTV2CaptionRenderer::BurnString (const std::string &		inString,
										const NTV2Line21Attrs &	inAttribs,
										NTV2Buffer &			inFB,
										const NTV2FormatDesc &	inFD,
										const UWord				inRow,
										const UWord				inCol)
{
	if (inString.empty())
		return true;	//	Nothing to draw

	const string	burnStr	(NTV2CCFont::Utf8ToCCFontByteArray(inString));
	const size_t	numChars(burnStr.length());
	const UWord		row		(inRow < NTV2_CC608_MinRow ? NTV2_CC608_MinRow : (inRow > NTV2_CC608_MaxRow ? NTV2_CC608_MaxRow : inRow));	//	Clamp row
	UWord			col		(inCol < NTV2_CC608_MinCol ? NTV2_CC608_MinCol : (inCol > NTV2_CC608_MaxCol ? NTV2_CC608_MaxCol : inCol));	//	Clamp column
	UWord			yPos(0), xPos(0);

	CNTV2CaptionRendererPtr	renderer (CNTV2CaptionRenderer::GetRenderer(inFD));
	if (!renderer)
		{MYERR("No renderer for " << inFD);  return false;}
#if 1	//	NEW WAY (15% FASTER)
	if (!renderer->GetCharacterRasterOrigin (row, col, yPos, xPos))
		{MYERR("GetCharacterRasterOrigin failed for row=" << row << " col=" << col);  return false;}

	const NTV2GlyphIndexes glyphIndexes (CharCodesToGlyphIndexes(burnStr));
	NTV2_ASSERT(glyphIndexes.size() == numChars);
	const UWord		glyphWidth		(renderer->GetGlyphWidthInPixels());
	UByte *			pDstBuffer		((UByte*)inFD.GetRowAddress(inFB.GetHostAddress(0),inFD.GetFirstActiveLine()));
	const UByte *	pSrcBuffer		(renderer->GetPreloadedGlyphsRasterAddress(inAttribs));
	const ULWord	srcBytesPerLine	(renderer->GetPreloadedGlyphsRasterRowBytes());
	const UWord		srcTotalLines	(renderer->GetPreloadedGlyphsRasterHeightInLines());
	for (size_t pos(0);  pos < glyphIndexes.size();  pos++)
	{	const NTV2GlyphIndex glyphNdx (glyphIndexes.at(pos));
		if (!::CopyRaster (inFD.GetPixelFormat(),				//	Pixel format of both src and dst buffers
						pDstBuffer,								//	Dest buffer to be modified
						inFD.GetBytesPerRow(),					//	Dest buffer bytes per raster line (determines max width)
						UWord(inFD.GetVisibleRasterHeight()),	//	Dest buffer total lines in raster (max height)
						yPos,									//	Vertical line offset into the dest raster where the top edge of the src image will appear
						xPos,									//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
						pSrcBuffer,								//	Src buffer
						srcBytesPerLine,						//	Src buffer raster row bytes (determines max width)
						srcTotalLines,							//	Src buffer raster height (max)
						0,										//	Top edge of src image to copy
						renderer->GetGlyphHeightInLines(),		//	Height of src image to copy
						glyphWidth * glyphNdx,					//	Left edge of src image to copy
						glyphWidth))							//	Width of src image to copy
			{MYERR("CopyRaster failed: glyphs[" << pos << "]=" << glyphNdx << " " << inFD << " " << renderer);  return false;}
		if (col < NTV2_CC608_MaxCol)	//	Any room to increment to the next column?
			{col++;  xPos += glyphWidth;}	//	Yes -- bump column number and xPos
		else if (glyphIndexes.size() >= 2  &&  pos < (glyphIndexes.size() - 2))	//	No -- not at last glyph?
			pos = glyphIndexes.size() - 2;	//	Immediately jump to last glyph, then draw it
	}	//	for each glyph to draw
#else	//	OLD WAY (ABOUT 15% SLOWER)
	for (size_t charNdx(0);  charNdx < numChars;  charNdx++)
	{
		if (!renderer->GetCharacterRasterOrigin (row, col, yPos, xPos))
			return false;
		if (!renderer->BurnChar (UByte(burnStr.at(charNdx)), inAttribs, inFB, inFD, xPos, yPos))
			return false;

		if (col < NTV2_CC608_MaxCol)	//	Any room to increment to the next column?
			col++;	//	Yes -- bump column
			//	No -- any remaining characters will overwrite the last column)
	}	//	for each glyph in the string
#endif
	return true;
}	//	BurnString


bool CNTV2CaptionRenderer::BurnStringAtXY (const string &					inString,
											const NTV2Line21Attributes &	inAttribs,
											NTV2Buffer &					inFB,
											const NTV2FormatDescriptor &	inFBDescriptor,
											const UWord						inXPos,
											const UWord						inYPos)
{
	if (inString.empty())
		return true;
	if (inFB.IsNULL())
		return false;
	if (!inFBDescriptor.IsValid())
		return false;

	const NTV2FrameSize	frameDimensions (inFBDescriptor.GetFullRasterDimensions());
	CNTV2CaptionRendererPtr	renderer (CNTV2CaptionRenderer::GetRenderer(inFBDescriptor));
	if (!renderer)
		return false;

	const string			glyphsToBlit	(NTV2CCFont::Utf8ToCCFontByteArray(inString));
	const size_t			numChars		(glyphsToBlit.length());
	const UWord				yPos			(inYPos);
	UWord					xPos			(inXPos);
	const UWord				charWidth		(renderer->GetGlyphWidthInPixels());
	const UWord				lineHeight		(renderer->GetGlyphHeightInLines());
	const UWord				maxPixelWidth	(UWord(frameDimensions.width()));
	const UWord				maxPixelHeight	(UWord(frameDimensions.height()));

	if (yPos < (maxPixelHeight - lineHeight))
		for (unsigned charNdx(0);  charNdx < numChars;  charNdx++)
		{
			renderer->BurnChar (UByte(glyphsToBlit.at(charNdx)), inAttribs,
								inFB, inFBDescriptor, xPos, yPos);
//								reinterpret_cast<UByte*>(inFBDescriptor.GetWriteableRowAddress(inFB.GetHostAddress(0), inFBDescriptor.GetFirstActiveLine())),
//								frameDimensions, inFBDescriptor.GetPixelFormat(), xPos, yPos, UWord(inFBDescriptor.GetBytesPerRow()));
			if (xPos >= maxPixelWidth)	//	Any room to increment to the next column?
				break;					//	Offscreen -- bail

			xPos += charWidth;
		}	//	for each character in the string
	return true;
}



bool CNTV2CaptionRenderer::Create (CNTV2CaptionRendererPtr &	outCache,
									const NTV2PixelFormat	inPixelFormat,
									const NTV2CCFont &		inCCFont,
									const NTV2FrameSize &	inFrameDimensions)
{
	outCache = AJA_NULL;

	try
	{
		outCache = new CNTV2CaptionRenderer (inPixelFormat, inCCFont, inFrameDimensions);
	}
	catch (const std::bad_alloc &)
	{
	}
	return outCache;

}	//	Create


CNTV2CaptionRenderer::CNTV2CaptionRenderer (const NTV2PixelFormat	inPixelFormat,
											const NTV2CCFont &		inCCFont,
											const NTV2FrameSize &	inFrameDimensions)
	:	mCCFont						(inCCFont),
		mPixelFormat				(inPixelFormat),
		mHRasterPixelsPerGlyphDot	(0),
		mVRasterLinesPerGlyphDot	(0),
		mVCaptionRasterOrigin		(0),
		mHCaptionRasterOrigin		(0),
		mGlyphWidthInPixels			(0),
		mGlyphWidthInBytes			(0),
		mGlyphHeightInLines			(0),
		mBytesPerAttribute			(0),
		mFrameDimensions			(inFrameDimensions)
{
}


CNTV2CaptionRenderer::~CNTV2CaptionRenderer ()
{
	if (IsOpen())
		Close();
}


bool CNTV2CaptionRenderer::Open (void)
{
	const ULWord		nGlyphs						(mCCFont.GetGlyphCount ());
	const UWord			nVisibleRasterPixels		(static_cast <UWord> (mFrameDimensions.width()));
	UWord				nVisibleRasterLines			(0);
	NTV2FrameGeometry	frameGeomToGetVisibleLines	(NTV2_FG_INVALID);
	bool				bResult						(true);

	if (IsOpen())
		Close();

	AJACC_ASSERT (mpRasters.empty());

	if (mFrameDimensions.height() > 4300)		//	UHD2/8K?
	{
		mHRasterPixelsPerGlyphDot	= 6;
		mVRasterLinesPerGlyphDot	= 6;
		frameGeomToGetVisibleLines	= NTV2_FG_4x3840x2160;	//	4320
	}
	else if (mFrameDimensions.height() > 2100)	//	UHD/4K?
	{
		mHRasterPixelsPerGlyphDot	= 3;
		mVRasterLinesPerGlyphDot	= 3;
		frameGeomToGetVisibleLines	= NTV2_FG_4x1920x1080;	//	2160
	}
	else if (mFrameDimensions.height() > 1500)	//	HD 2K 1556?  (obsolete)
	{
		mHRasterPixelsPerGlyphDot	= 2;
		mVRasterLinesPerGlyphDot	= 2;
		frameGeomToGetVisibleLines	= NTV2_FG_2048x1556;	//	1556 (obsolete)
	}
	else if (mFrameDimensions.height() > 1000)//	HD 1080?
	{
		mHRasterPixelsPerGlyphDot	= 2;
		mVRasterLinesPerGlyphDot	= 2;
		frameGeomToGetVisibleLines	= NTV2_FG_1920x1080;	//	1080
	}
	else if (mFrameDimensions.height() > 650)	//	HD 720?
	{
		mHRasterPixelsPerGlyphDot	= 1;
		mVRasterLinesPerGlyphDot	= 1;
		frameGeomToGetVisibleLines	= NTV2_FG_1280x720;		// 720
	}
	else										//	SD 486
	{
		mHRasterPixelsPerGlyphDot	= 1;
		mVRasterLinesPerGlyphDot	= 1;
		frameGeomToGetVisibleLines	= NTV2_FG_720x486;		// 486
		//mVCaptionRasterOrigin	= 46;	//	486 - 2 * 46 = 486 - 92 = 394  ==>  81% of vertical space   ==>  394 / 15 rows = 26 dots per row
	}
	nVisibleRasterLines = UWord(::GetNTV2FrameGeometryHeight(frameGeomToGetVisibleLines));	//	2160

	mGlyphWidthInPixels	= GlyphDotWidthToRasterPixels(mCCFont.GetTotalWidthInDots());
	mGlyphWidthInBytes	= UWord(::CalcRowBytesForFormat(mPixelFormat, mGlyphWidthInPixels));
	mGlyphHeightInLines	= GlyphDotHeightToRasterLines(mCCFont.GetTotalHeightInDots());

	//	Calculate the raster line offset to the top edge of the caption area.
	//	The caption area is 80% of the visible vertical space (i.e., excludes VANC lines).
	//	Then compute the vertical margin that's above and below the caption area.
	//	Then subtract off the margin lines and visible lines from the frame height to get the raster offset.
	//	This will work correctly, even if clients use normal, "tall" or "taller" frame buffers.
	const UWord	captionAreaHeightInRasterLines	(nVisibleRasterLines * 80 / 100);
	const UWord	verticalMarginInRasterLines		((nVisibleRasterLines - captionAreaHeightInRasterLines) / 2);
	mVCaptionRasterOrigin = UWord (mFrameDimensions.height()) - verticalMarginInRasterLines - captionAreaHeightInRasterLines;			//	someday...
	if (captionAreaHeightInRasterLines > (5 + mGlyphHeightInLines * (NTV2_CC608_MaxRow - NTV2_CC608_MinRow + 1)))						//	... but ...
		mVCaptionRasterOrigin += captionAreaHeightInRasterLines - mGlyphHeightInLines * (NTV2_CC608_MaxRow - NTV2_CC608_MinRow + 1);	//	... for now

	//	Calculate the horizontal pixel offset to the left edge of the "zero-th" column.
	//	Must take into account that in a caption row, a non-underlined space always precedes the first character and follows the last character.
	//	Thus, in a full 32-character caption row, the total width is 34 characters, not 32.
	//	Multiply that 34 characters times by the width of a glyph box.
	//	The total horizontal margin is 720 pixels minus that total width.
	//	Half of that margin is the left edge of column zero...
	const UWord	captionAreaWidthInRasterPixels	(nVisibleRasterPixels * 80 / 100);
	const UWord	horizontalMarginInRasterPixels	((nVisibleRasterPixels - captionAreaWidthInRasterPixels) / 2);
	mHCaptionRasterOrigin = UWord(mFrameDimensions.width()) - horizontalMarginInRasterPixels - captionAreaWidthInRasterPixels;				//	someday...
	mHCaptionRasterOrigin = ((UWord(mFrameDimensions.width()) - ((NTV2_CC608_MaxCol - NTV2_CC608_MinCol + 1) * mGlyphWidthInPixels)) / 2);	//	...but for now
	if (mPixelFormat == NTV2_FBF_10BIT_YCBCR  &&  mHCaptionRasterOrigin % 6)
	{
		mHCaptionRasterOrigin /= 6;
		mHCaptionRasterOrigin *= 6;		//	'v210' must be 6-pixel-aligned
		AJACC_ASSERT (mHCaptionRasterOrigin % 6 == 0);
		AJACC_ASSERT (mGlyphWidthInPixels % 6 == 0);
	}

	//	The cache, as currently implemented, is a very short, wide raster that's the height of one glyph (including its top/bottom margins).
	//	Its width is the concatenation of all glyphs (including their left & right margins) rendered in a common display attribute.
	//	Each cached glyph is appended horizontally in the raster left-to-right in increasing NTV2GlyphIndex order.

	mBytesPerAttribute	= ULWord(mGlyphWidthInBytes) * ULWord(nGlyphs) * ULWord(mGlyphHeightInLines);
	if (bResult)
		MYNOTE("Opened " << *this);
	else
		MYERR("Failed to open " << *this);
	return bResult;

}	//	Open


bool CNTV2CaptionRenderer::Close (void)
{
	MYNOTE("Closing " << *this);
	mHRasterPixelsPerGlyphDot	= 0;
	mVRasterLinesPerGlyphDot	= 0;
	mVCaptionRasterOrigin		= 0;
	mHCaptionRasterOrigin		= 0;
	mGlyphWidthInPixels			= 0;
	mGlyphWidthInBytes			= 0;
	mGlyphHeightInLines			= 0;
	mBytesPerAttribute			= 0;

	for (AttribToBufferMapConstIter iter (mpRasters.begin ());  iter != mpRasters.end ();  ++iter)
	{
		UByte *	pRaster	(iter->second);
		if (pRaster)
			delete [] pRaster;
	}
	mpRasters.clear ();
	return true;

}	//	Close


bool CNTV2CaptionRenderer::GetCharacterRasterOrigin (const UWord in608CaptionRow, const UWord in608CaptionCol,
													UWord & outVertLineOffset, UWord & outHorzPixelOffset) const
{
	outVertLineOffset = outHorzPixelOffset = 0;
	if ((in608CaptionRow < 1) || (in608CaptionRow > 15))
		return false;
	if (in608CaptionCol > 33)	//	Allow columns 0 and 33 for the non-underlined spaces that precede and follow columns 1 & 32, respectively
		return false;

	outVertLineOffset  = mVCaptionRasterOrigin  +  (in608CaptionRow - 1) * mGlyphHeightInLines;
	outHorzPixelOffset = mHCaptionRasterOrigin;
	if (in608CaptionCol > 0)
		outHorzPixelOffset += (in608CaptionCol - 1) * mGlyphWidthInPixels;
	else											//	Special case:  column zero...
		outHorzPixelOffset -= mGlyphWidthInPixels;	//	...appears immediately to the left of the caption box
	return true;

}	//	Get608RowColPosition


const UByte * CNTV2CaptionRenderer::GetPreloadedGlyphsRasterAddress (const NTV2Line21Attributes & inAttribs) const
{
	const UByte *	pResult	(AJA_NULL);
	NTV2Line21Attrs	attr	(inAttribs);

	//	Flashing is relevant in the attributes' hash key, but irrelevant to glyph rendering...
	attr.RemoveFlash();
	AJACC_ASSERT (!attr.IsFlashing());

	pResult = GetPreloadedGlyphsRasterForAttribute(attr);
	if (!pResult)
		if (CreatePreloadedGlyphsRasterForAttribute(attr))
			pResult = GetPreloadedGlyphsRasterForAttribute(attr);

	return pResult;
}


bool CNTV2CaptionRenderer::GetGlyphsRaster (NTV2Buffer & outBuffer, const NTV2Line21Attributes & inAttrs) const
{	//	Remove irrelevant Flashing attribute...
	NTV2Line21Attrs	attrs (inAttrs);
	attrs.RemoveFlash();
	AJACC_ASSERT (!attrs.IsFlashing());
	return outBuffer.CopyFrom (GetPreloadedGlyphsRasterForAttribute(attrs), mBytesPerAttribute);
}


NTV2FormatDesc CNTV2CaptionRenderer::GetFormatDescriptor (void) const
{	//	Pre-rendered glyph raster width, in pixels:
	const uint32_t	widthPixels (uint32_t(GetGlyphWidthInPixels()) * uint32_t(mCCFont.GetGlyphCount()));
	const uint32_t rowBytes (::CalcRowBytesForFormat (GetPixelFormat(),	widthPixels));
	const uint32_t linePitch (rowBytes / widthPixels / sizeof(uint32_t));
	NTV2FormatDesc result(GetGlyphHeightInLines(), widthPixels, linePitch);
	result.SetPixelFormat(GetPixelFormat());
	return result;
}


bool CNTV2CaptionRenderer::CreatePreloadedGlyphsRasterForAttribute (const NTV2Line21Attributes & inAttribs) const
{
	if (HasPreloadedGlyphsRasterForAttribute(inAttribs))
		return true;

	const string	myFBFStr(::NTV2FrameBufferFormatToString(mPixelFormat));
	const UWord		nGlyphs	(mCCFont.GetGlyphCount());
	NTV2Line21Attrs	attr	(inAttribs);
	attr.RemoveFlash();
	const ULWord	attrKey	(attr.GetHashKey());
	UByte *			pRaster	(AJA_NULL);
	const bool		bIsHD	(mFrameDimensions.width() > ::GetDisplayWidth(NTV2_FORMAT_525_5994));
	bool			bResult	(true);

	try
		{pRaster = new UByte[mBytesPerAttribute];}
	catch (const std::bad_alloc &)
		{pRaster = AJA_NULL;}

	if (!pRaster)
	{
		MYERR("Failed to allocate " << mBytesPerAttribute << "-byte raster buffer for '" << myFBFStr << "'");
		return false;
	}
	::memset (pRaster, 0, mBytesPerAttribute);

	switch (mPixelFormat)
	{
		case NTV2_FBF_8BIT_YCBCR:
			//	CCFont knows how to do this...
			for (NTV2GlyphIndex glyphNdx(0);  glyphNdx < nGlyphs;  glyphNdx++)
			{
				bResult = mCCFont.RenderGlyph8BitYCbCr (pRaster + mGlyphWidthInBytes * glyphNdx,	//	destination buffer
														ULWord(mGlyphWidthInBytes) * ULWord(nGlyphs),	//	bytesPerRow
														glyphNdx,									//	glyph index
														attr,										//	attribs
														mHRasterPixelsPerGlyphDot,					//	scaledDotWidth
														mVRasterLinesPerGlyphDot);					//	scaledDotHeight
				if (!bResult)
				{
					MYERR("RenderGlyph8BitYCbCr failed for glyph " << glyphNdx << " with attr '" << attr << "' for '" << myFBFStr << "'");
					break;
				}
			}	//	for each glyph
			break;

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
			//	CCFont knows how to do this...
			for (NTV2GlyphIndex glyphNdx(0);  glyphNdx < nGlyphs;  glyphNdx++)
			{
				bResult = mCCFont.RenderGlyph8BitRGB (	mPixelFormat,								//	which RGB pixel format?
														pRaster + mGlyphWidthInBytes * glyphNdx,	//	destination buffer
														ULWord(mGlyphWidthInBytes) * ULWord(nGlyphs),	//	bytesPerRow
														glyphNdx,									//	glyph index
														attr,										//	attribs
														mHRasterPixelsPerGlyphDot,					//	scaledDotWidth
														mVRasterLinesPerGlyphDot,					//	scaledDotHeight
														bIsHD);										//	inIsHD (i.e. use Rec709?)
				if (!bResult)
				{
					MYERR("RenderGlyph8BitRGB failed for glyph " << glyphNdx << " with attr '" << attr << "' for '" << myFBFStr << "'");
					break;
				}
			}	//	for each glyph
			break;

		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:
		{
			//	Convert 8-bit YUV into other YUV formats...
			const string strFromFBF(::NTV2FrameBufferFormatToString(NTV2_FBF_8BIT_YCBCR));
			string strConverterName;
			if (mPixelFormat == NTV2_FBF_10BIT_YCBCR)
				strConverterName = "'ConvertLine_2vuy_to_v210'";
			else if (mPixelFormat == NTV2_FBF_8BIT_YCBCR_YUY2)
				strConverterName = "'ConvertLine_2vuy_to_yuy2'";
			else NTV2_ASSERT(false);

			//	Get an NTV2FormatDescriptor for a given NTV2FrameDimensions and NTV2PixelFormat:
			const NTV2FrameGeometry fg(::GetGeometryFromFrameDimensions(mFrameDimensions));
			if (!NTV2_IS_VALID_NTV2FrameGeometry(fg))
				{MYERR("GetGeometryFromFrameDimensions failed for frame dimensions " << mFrameDimensions);	break;}
			const NTV2FormatDesc tmpFD(::GetStandardFromGeometry(fg), NTV2_FBF_8BIT_YCBCR, ::GetVANCModeForGeometry(fg));
			CNTV2CaptionRendererPtr	renderer2vuy (GetRenderer(tmpFD, true));
			if (renderer2vuy)
			{
				const UByte *	pSrcRaster	(renderer2vuy->GetPreloadedGlyphsRasterAddress(inAttribs));	//	Beware -- this may recurse!
				const UWord		numLines	(renderer2vuy->GetPreloadedGlyphsRasterHeightInLines());
				const ULWord	srcRowBytes	(renderer2vuy->GetPreloadedGlyphsRasterRowBytes());
				const UWord		widthPixels	(renderer2vuy->GetPreloadedGlyphsRasterWidthInPixels());
				const ULWord	dstRowBytes	(ULWord(mGlyphWidthInBytes) * ULWord(mCCFont.GetGlyphCount()));
				AJACC_ASSERT (numLines == GetPreloadedGlyphsRasterHeightInLines ());

				for (ULWord lineNum(0);  lineNum < numLines;  lineNum++)
				{
					UByte *	pDstRasterLine	(pRaster + lineNum * dstRowBytes);
					switch (mPixelFormat)
					{
						case NTV2_FBF_10BIT_YCBCR:		bResult = ::ConvertLine_2vuy_to_v210 (pSrcRaster + lineNum * srcRowBytes,
																							reinterpret_cast<ULWord*>(pDstRasterLine),
																							widthPixels);
														break;
						case NTV2_FBF_8BIT_YCBCR_YUY2:	bResult = ::ConvertLine_2vuy_to_yuy2 (pSrcRaster + lineNum * srcRowBytes,
																							reinterpret_cast<UWord*>(pDstRasterLine),
																							widthPixels);
														break;
						default:						bResult = false;	//	No can do
														break;
					}	//	switch on pixel format
					if (!bResult)
						{MYERR(strConverterName << " failed converting " << strFromFBF << " to " << myFBFStr << " for " << inAttribs);	break;}	//	for loop
				}	//	for each raster line
				if (bResult)
					MYDBG("Converted " << strFromFBF << " prerendered glyphs raster to " << myFBFStr << " for  " << inAttribs);
			}
			else
			{
				MYERR("Unable to create pre-rendered glyphs raster for " << inAttribs << " and " << myFBFStr << " from " << strFromFBF);
				bResult = false;
			}
			break;
		}	//	NTV2_FBF_10BIT_YCBCR

		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_DPX:
		case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_48BIT_RGB:
		{
			//	Convert 8-bit RGB into other RGB formats...
			const string strFromFBF(::NTV2FrameBufferFormatToString(NTV2_FBF_ABGR));
			string strConverterName;
			if (mPixelFormat == NTV2_FBF_10BIT_RGB)
				strConverterName = "'ConvertLine_8bitABGR_to_10bitABGR'";
			else if (mPixelFormat == NTV2_FBF_10BIT_DPX)
				strConverterName = "'ConvertLine_8bitABGR_to_10bitRGBDPX'";
			else if (mPixelFormat == NTV2_FBF_10BIT_DPX_LE)
				strConverterName = "'ConvertLine_8bitABGR_to_10bitRGBDPXLE'";
			else if (mPixelFormat == NTV2_FBF_24BIT_RGB)
				strConverterName = "'ConvertLine_8bitABGR_to_24bitRGB'";
			else if (mPixelFormat == NTV2_FBF_24BIT_BGR)
				strConverterName = "'ConvertLine_8bitABGR_to_24bitBGR'";
			else if (mPixelFormat == NTV2_FBF_48BIT_RGB)
				strConverterName = "'ConvertLine_8bitABGR_to_48bitRGB'";
			else NTV2_ASSERT(false);

			//	Get an NTV2FormatDescriptor for a given NTV2FrameDimensions and NTV2PixelFormat:
			const NTV2FrameGeometry fg(::GetGeometryFromFrameDimensions(mFrameDimensions));
			if (!NTV2_IS_VALID_NTV2FrameGeometry(fg))
				{MYERR("GetGeometryFromFrameDimensions failed for frame dimensions " << mFrameDimensions);	break;}
			const NTV2FormatDesc tmpFD(::GetStandardFromGeometry(fg), NTV2_FBF_ABGR, ::GetVANCModeForGeometry(fg));
			CNTV2CaptionRendererPtr	renderer8bitABGR (GetRenderer(tmpFD, true));
			if (renderer8bitABGR)
			{
				const UByte *	pSrcRaster	(renderer8bitABGR->GetPreloadedGlyphsRasterAddress(inAttribs));	//	Beware -- this may recurse!
				const UWord		numLines	(renderer8bitABGR->GetPreloadedGlyphsRasterHeightInLines());
				const ULWord	srcRowBytes	(renderer8bitABGR->GetPreloadedGlyphsRasterRowBytes());
				const UWord		widthPixels	(renderer8bitABGR->GetPreloadedGlyphsRasterWidthInPixels());
				const ULWord	dstRowBytes	(ULWord(mGlyphWidthInBytes) * ULWord(mCCFont.GetGlyphCount()));
				AJACC_ASSERT (numLines == GetPreloadedGlyphsRasterHeightInLines());
				for (ULWord lineNum(0);  lineNum < numLines;  lineNum++)
				{
					UByte *	pDstRasterLine	(pRaster + lineNum * dstRowBytes);
					switch (mPixelFormat)
					{
						case NTV2_FBF_10BIT_RGB:
							bResult = ::ConvertLine_8bitABGR_to_10bitABGR (pSrcRaster + lineNum * srcRowBytes,
																			reinterpret_cast<ULWord*>(pDstRasterLine),  widthPixels);
							break;
						case NTV2_FBF_10BIT_DPX:
							bResult = ::ConvertLine_8bitABGR_to_10bitRGBDPX (pSrcRaster + lineNum * srcRowBytes,
																			reinterpret_cast<ULWord*>(pDstRasterLine),  widthPixels);
							break;
						case NTV2_FBF_10BIT_DPX_LE:
							bResult = ::ConvertLine_8bitABGR_to_10bitRGBDPXLE (pSrcRaster + lineNum * srcRowBytes,
																				reinterpret_cast<ULWord*>(pDstRasterLine),  widthPixels);
							break;
						case NTV2_FBF_24BIT_RGB:
							bResult = ::ConvertLine_8bitABGR_to_24bitRGB (pSrcRaster + lineNum * srcRowBytes,
																			pDstRasterLine,  widthPixels);
							break;
						case NTV2_FBF_24BIT_BGR:
							bResult = ::ConvertLine_8bitABGR_to_24bitBGR (pSrcRaster + lineNum * srcRowBytes,
																			pDstRasterLine,  widthPixels);
							break;
						case NTV2_FBF_48BIT_RGB:
							bResult = ::ConvertLine_8bitABGR_to_48bitRGB (pSrcRaster + lineNum * srcRowBytes,
																			reinterpret_cast<ULWord*>(pDstRasterLine),  widthPixels);
							break;
						default:
							bResult = false;	//	No can do
							break;
					}	//	switch on pixel format
					if (!bResult)
						{MYERR(strConverterName << " failed converting " << strFromFBF << " to " << myFBFStr << " for " << inAttribs);	break;}	//	for loop
				}
				if (bResult)
					MYDBG("Converted " << strFromFBF << " prerendered glyph raster to " << myFBFStr << " for " << inAttribs);
			}
			else
				{MYERR("Cannot create pre-rendered glyph raster for " << inAttribs << " " << myFBFStr << " from " << strFromFBF); bResult = false;}
			break;
		}	//	case

		default:
			MYWARN("Cannot create pre-rendered glyphs for " << myFBFStr << " -- not implemented");
			bResult = false;
			break;
	}	//	switch on mPixelFormat

	if (bResult)
	{
		try
			{mpRasters.insert(make_pair(attrKey, pRaster));}
		catch (const std::bad_alloc &)
			{bResult = false;}
		if (!bResult)
			MYERR("Insertion failure on " << mBytesPerAttribute << "-byte raster buffer for " << attr << " and '" << myFBFStr << "'");
	}
	if (bResult)
		MYINFO(attr << ":  " << *this);
	if (!bResult && pRaster)
		delete [] pRaster;		//	Delete the raster buffer memory upon failure
	return bResult;

}	//	CreatePreloadedGlyphsRasterForAttribute


bool CNTV2CaptionRenderer::HasPreloadedGlyphsRasterForAttribute (const NTV2Line21Attributes & inAttribs) const
{
	return GetPreloadedGlyphsRasterForAttribute(inAttribs) != AJA_NULL;
}


const UByte * CNTV2CaptionRenderer::GetPreloadedGlyphsRasterForAttribute (const NTV2Line21Attributes & inAttribs) const
{
	AJACC_ASSERT (!inAttribs.IsFlashing());
	AttribToBufferMapConstIter iter (mpRasters.find(inAttribs.GetHashKey()));
	return iter != mpRasters.end()  ?  iter->second  :  AJA_NULL;
}


CNTV2CaptionRenderer & CNTV2CaptionRenderer::operator = (const CNTV2CaptionRenderer & inObjToCopy)
{
	if (&inObjToCopy != this)
		mPixelFormat = inObjToCopy.mPixelFormat;
	return *this;
}


vector <ULWord> CNTV2CaptionRenderer::GetActivePreloadedGlyphsRastersAttributes (void) const
{
	vector <ULWord>	result;
	for (AttribToBufferMapConstIter iter(mpRasters.begin());  iter != mpRasters.end();  ++iter)
		if (iter->second)
			result.push_back(iter->first);
	return result;
}


ostream & CNTV2CaptionRenderer::Print (ostream & oss, const bool inIncludeActives) const
{
	if (!IsOpen())
		return oss << "[Renderer closed]";

	vector<ULWord> attrHashes (GetActivePreloadedGlyphsRastersAttributes());
	oss	<< "[Renderer '" << mCCFont.GetName() << "' " << mFrameDimensions
		<< " " << ::NTV2FrameBufferFormatToString(mPixelFormat)
		<< " scale:" << mHRasterPixelsPerGlyphDot << "x" << mVRasterLinesPerGlyphDot
		<< " glyphs:" << mGlyphWidthInPixels << "x" << mGlyphHeightInLines
		<< "  " << GetNumActivePreloadedGlyphsRasters() << " preloaded rasters (" << GetTotalBytes() << " bytes)";
	if (inIncludeActives)
	{
		if (!attrHashes.empty())
			oss << ": ";
		for (unsigned ndx(0);  ndx < attrHashes.size(); )
		{
			oss << gAllAttributePermutations.GetPermutation(attrHashes.at(ndx));
			if (++ndx < attrHashes.size())
				oss << ", ";
		}
	}
	return oss << "]";
}


ostream & operator << (ostream & inOutStream, const CNTV2CaptionRendererPtr & inCachePtr)
{
	if (inCachePtr)
		return inOutStream << *inCachePtr;
	else
		return inOutStream << "[null]";
}


ostream & operator << (ostream & inOutStream, const CNTV2CaptionRenderer & inCache)
{
	return inCache.Print(inOutStream);
}
