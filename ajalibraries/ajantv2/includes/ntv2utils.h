/**
	@file		ntv2utils.h
	@brief		Declares numerous NTV2 utility functions.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2UTILS_H
#define NTV2UTILS_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#include "videodefines.h"
#include "ntv2publicinterface.h"
#include "ntv2m31publicinterface.h"
#include "ntv2signalrouter.h"
#include <string>
#include <iostream>
#include <vector>
#if defined (AJALinux)
	#include <stdint.h>
#endif


//////////////////////////////////////////////////////
//	BEGIN SECTION MOVED FROM 'videoutilities.h'
//////////////////////////////////////////////////////
#define DEFAULT_PATT_GAIN  0.9		// some patterns pay attention to this...
#define HD_NUMCOMPONENTPIXELS_2K  2048
#define HD_NUMCOMPONENTPIXELS_1080_2K  2048
#define HD_NUMCOMPONENTPIXELS_1080  1920
#define CCIR601_10BIT_BLACK  64
#define CCIR601_10BIT_WHITE  940
#define CCIR601_10BIT_CHROMAOFFSET  512

#define CCIR601_8BIT_BLACK  16
#define CCIR601_8BIT_WHITE  235
#define CCIR601_8BIT_CHROMAOFFSET  128

// line pitch is in bytes.
#define FRAME_0_BASE (0x0)
#define FRAME_1080_10BIT_LINEPITCH (1280*4)
#define FRAME_1080_8BIT_LINEPITCH (1920*2)
#define FRAME_QUADHD_10BIT_SIZE (FRAME_1080_10BIT_LINEPITCH*2160)
#define FRAME_QUADHD_8BIT_SIZE (FRAME_1080_8BIT_LINEPITCH*2160)
#define FRAME_BASE(frameNumber,frameSize) (frameNumber*frameSize) 

/**
	@param[in]	inPixelFormat		Specifies the pixel format.
	@param[in]	inPixelWidth		Specifies the width of the line, in pixels.
	@return		The minimum number of bytes necessary to accommodate the given number of pixels in the specified pixel format.
**/
AJAExport uint32_t CalcRowBytesForFormat (const NTV2FrameBufferFormat inPixelFormat, const uint32_t inPixelWidth);
AJAExport void UnPack10BitYCbCrBuffer (uint32_t* packedBuffer, uint16_t* ycbcrBuffer, uint32_t numPixels);
AJAExport void PackTo10BitYCbCrBuffer (const uint16_t *ycbcrBuffer, uint32_t *packedBuffer, const uint32_t numPixels);
AJAExport void MakeUnPacked10BitYCbCrBuffer (uint16_t* buffer, uint16_t Y , uint16_t Cb , uint16_t Cr,uint32_t numPixels);
AJAExport void ConvertLineTo8BitYCbCr (uint16_t * ycbcr10BitBuffer, uint8_t * ycbcr8BitBuffer,	uint32_t numPixels);
AJAExport void ConvertUnpacked10BitYCbCrToPixelFormat (uint16_t *unPackedBuffer, uint32_t *packedBuffer, uint32_t numPixels, NTV2FrameBufferFormat pixelFormat,
														bool bUseSmpteRange=false, bool bAlphaFromLuma=false);
AJAExport void MaskUnPacked10BitYCbCrBuffer (uint16_t* ycbcrUnPackedBuffer, uint16_t signalMask , uint32_t numPixels);
AJAExport void StackQuadrants (uint8_t* pSrc, uint32_t srcWidth, uint32_t srcHeight, uint32_t srcRowBytes, uint8_t* pDst);
AJAExport void CopyFromQuadrant (uint8_t* srcBuffer, uint32_t srcHeight, uint32_t srcRowBytes, uint32_t srcQuadrant, uint8_t* dstBuffer,  uint32_t quad13Offset=0);
AJAExport void CopyToQuadrant (uint8_t* srcBuffer, uint32_t srcHeight, uint32_t srcRowBytes, uint32_t dstQuadrant, uint8_t* dstBuffer, uint32_t quad13Offset=0);
//////////////////////////////////////////////////////
//	END SECTION MOVED FROM 'videoutilities.h'
//////////////////////////////////////////////////////

/**
	@brief	An ordered sequence of UWord (uint16_t) values.
**/
typedef	std::vector <UWord>					UWordSequence;
typedef	UWordSequence::const_iterator		UWordSequenceConstIter;
typedef	UWordSequence::iterator				UWordSequenceIter;

AJAExport std::ostream & operator << (std::ostream & inOutStream, const UWordSequence & inData);

/**
	@brief		Unpacks a line of NTV2_FBF_10BIT_YCBCR video into 16-bit-per-component YUV data.
	@param[in]	pIn10BitYUVLine		A valid, non-NULL pointer to the start of the line that contains the NTV2_FBF_10BIT_YCBCR data
									to be converted.
	@param[out]	out16BitYUVLine		Receives the unpacked 16-bit-per-component YUV data. The sequence is cleared before filling.
									The UWord sequence will be Cb0, Y0, Cr0, Y1, Cb1, Y2, Cr1, Y3, Cb2, Y4, Cr2, Y5, . . .
	@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
	@return		True if successful;  otherwise false.
**/
AJAExport bool		UnpackLine_10BitYUVtoUWordSequence (const void * pIn10BitYUVLine, UWordSequence & out16BitYUVLine, const ULWord inNumPixels);


#if !defined (NTV2_DEPRECATE)
	AJAExport NTV2_DEPRECATED	void UnPackLineData (const ULWord * pIn10BitYUVLine, UWord * pOut16BitYUVLine, const ULWord inNumPixels);	///< @deprecated	Replaced by UnpackLine_10BitYUVto16BitYUV.
	AJAExport NTV2_DEPRECATED	void PackLineData (const UWord * pIn16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels);		///< @deprecated	Replaced by PackLine_16BitYUVto10BitYUV.
#endif	//	NTV2_DEPRECATE

/**
	@brief	Unpacks a line of 10-bit-per-component YCbCr video into 16-bit-per-component YCbCr (NTV2_FBF_10BIT_YCBCR) data.
	@param[in]	pIn10BitYUVLine		A valid, non-NULL pointer to the input buffer that contains the packed 10-bit-per-component YUV data
									to be converted into 16-bit-per-component YUV.
	@param[out]	pOut16BitYUVLine	A valid, non-NULL pointer to the output buffer to receive the unpacked 16-bit-per-component YUV data.
	@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
**/
AJAExport void UnpackLine_10BitYUVto16BitYUV (const ULWord * pIn10BitYUVLine, UWord * pOut16BitYUVLine, const ULWord inNumPixels);

/**
	@brief	Packs a line of 16-bit-per-component YCbCr (NTV2_FBF_10BIT_YCBCR) video into 10-bit-per-component YCbCr data.
	@param[in]	pIn16BitYUVLine		A valid, non-NULL pointer to the input buffer that contains the 16-bit-per-component YUV data to be
									converted into 10-bit-per-component YUV.
	@param[out]	pOut10BitYUVLine	A valid, non-NULL pointer to the output buffer to receive the packed 10-bit-per-component YUV data.
	@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
**/
AJAExport void PackLine_16BitYUVto10BitYUV (const UWord * pIn16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels);

AJAExport void RePackLineDataForYCbCrDPX(ULWord *packedycbcrLine, ULWord numULWords);
AJAExport void UnPack10BitDPXtoRGBAlpha10BitPixel(RGBAlpha10BitPixel* rgba10BitBuffer,ULWord* DPXLinebuffer ,ULWord numPixels, bool bigEndian);
AJAExport void UnPack10BitDPXtoForRP215withEndianSwap(UWord* rawrp215Buffer,ULWord* DPXLinebuffer ,ULWord numPixels);
AJAExport void UnPack10BitDPXtoForRP215(UWord* rawrp215Buffer,ULWord* DPXLinebuffer ,ULWord numPixels);
AJAExport void MaskYCbCrLine(UWord* ycbcrLine, UWord signalMask , ULWord numPixels);
AJAExport void Make10BitBlackLine(UWord* lineData,UWord numPixels=1920);
AJAExport void Make10BitWhiteLine(UWord* lineData,UWord numPixels=1920);
AJAExport void Fill10BitYCbCrVideoFrame(PULWord _baseVideoAddress,
										NTV2Standard standard,
										NTV2FrameBufferFormat frameBufferFormat,
										YCbCr10BitPixel color,
										bool vancEnabled=false,
										bool twoKby1080=false,
							 bool wideVANC=false);
AJAExport void Make8BitBlackLine(UByte* lineData,UWord numPixels=1920,NTV2FrameBufferFormat=NTV2_FBF_8BIT_YCBCR);
AJAExport void Make8BitWhiteLine(UByte* lineData,UWord numPixels=1920,NTV2FrameBufferFormat=NTV2_FBF_8BIT_YCBCR);
AJAExport void Make10BitLine(UWord* lineData, UWord Y , UWord Cb , UWord Cr,UWord numPixels=1920);
AJAExport void Make8BitLine(UByte* lineData, UByte Y , UByte Cb , UByte Cr,ULWord numPixels=1920,NTV2FrameBufferFormat=NTV2_FBF_8BIT_YCBCR);
AJAExport void Fill8BitYCbCrVideoFrame(PULWord _baseVideoAddress,
									   NTV2Standard standard,
									   NTV2FrameBufferFormat frameBufferFormat,
									   YCbCrPixel color,
									   bool vancEnabled=false,
									   bool twoKby1080=false,
									   bool wideVANC=false);
AJAExport void Fill4k8BitYCbCrVideoFrame(PULWord _baseVideoAddress,
									   NTV2FrameBufferFormat frameBufferFormat,
									   YCbCrPixel color,
									   bool vancEnabled=false,
									   bool b4k=false,
									   bool wideVANC=false);
AJAExport void CopyRGBAImageToFrame(ULWord* pSrcBuffer, ULWord srcHeight, ULWord srcWidth, 
									ULWord* pDstBuffer, ULWord dstHeight, ULWord dstWidth);

/**
	@brief	Copies all or part of a source raster image into another raster at a given position.
	@param[in]	inPixelFormat			Specifies the NTV2FrameBufferFormat of the destination buffer.
										(Note that many pixel formats are not currently supported.)
	@param		pDstBuffer				Specifies the starting address of the destination buffer to be modified. Must be non-NULL.
	@param[in]	inDstBytesPerLine		The number of bytes per raster line of the destination buffer. Note that this value
										is used to compute the maximum pixel width of the destination raster. Also note that
										some pixel formats set constraints on this value (e.g., NTV2_FBF_10BIT_YCBCR requires
										this be a multiple of 16, while NTV2_FBF_8BIT_YCBCR requires an even number).
										Must exceed zero.
	@param[in]	inDstTotalLines			The total number of raster lines to set to legal black. Must exceed zero.
	@return		True if successful;  otherwise false.
**/
AJAExport bool	SetRasterLinesBlack (const NTV2FrameBufferFormat	inPixelFormat,
										UByte *						pDstBuffer,
										const UWord					inDstBytesPerLine,
										const UWord					inDstTotalLines);

/**
	@brief	Copies all or part of a source raster image into another raster at a given position.
	@param[in]	inPixelFormat			Specifies the NTV2FrameBufferFormat of both the destination and source buffers.
										(Note that many pixel formats are not currently supported.)
	@param		pDstBuffer				Specifies the starting address of the destination buffer to be modified. Must be non-NULL.
	@param[in]	inDstBytesPerLine		The number of bytes per raster line of the destination buffer. Note that this value
										is used to compute the maximum pixel width of the destination raster. Also note that
										some pixel formats set constraints on this value (e.g., NTV2_FBF_10BIT_YCBCR requires
										this be a multiple of 16, while NTV2_FBF_8BIT_YCBCR requires an even number).
										Must exceed zero.
	@param[in]	inDstTotalLines			The maximum height of the destination buffer, in raster lines. Must exceed zero.
	@param[in]	inDstVertLineOffset		Specifies the vertical line offset into the destination raster where the top edge
										of the source image will appear. This value must be less than the inDstTotalLines
										value (i.e., at least one line of the source must appear in the destination).
	@param[in]	inDstHorzPixelOffset	Specifies the horizontal pixel offset into the destination raster where the left
										edge of the source image will appear. This value must be less than the maximum
										width of the destination raster (as stipulated by the inDstBytesPerLine parameter).
										Thus, at least one pixel of the source (the leftmost edge) must appear in the destination
										(at the right edge). Note that some pixel formats set constraints on this value (e.g.,
										NTV2_FBF_10BIT_YCBCR requires this be a multiple of 6, while NTV2_FBF_8BIT_YCBCR requires
										this to be even).
	@param[in]	pSrcBuffer				Specifies the starting address of the source buffer to be copied from. Must be non-NULL.
	@param[in]	inSrcBytesPerLine		The number of bytes per raster line of the source buffer. Note that this value is used
										to compute the maximum pixel width of the source raster. Also note that some pixel formats
										set constraints on this value (e.g., NTV2_FBF_10BIT_YCBCR requires this be a multiple
										of 16, while NTV2_FBF_8BIT_YCBCR requires this to be even). Must exceed zero.
	@param[in]	inSrcTotalLines			The maximum height of the source buffer, in raster lines. Must exceed zero.
	@param[in]	inSrcVertLineOffset		Specifies the top edge of the source image to copy. This value must be less than
										the inSrcTotalLines value.
	@param[in]	inSrcVertLinesToCopy	Specifies the height of the source image to copy, in lines. This value can be larger
										than what's possible. The function guarantees that lines past the bottom edge of the
										source raster will not be accessed. It is not an error to specify zero, although
										nothing will be copied.
	@param[in]	inSrcHorzPixelOffset	Specifies the left edge of the source image to copy. This value must be less than the
										maximum width of the source raster (as stipulated by the inSrcBytesPerLine parameter).
										Note that some pixel formats set constraints on this value (e.g., NTV2_FBF_10BIT_YCBCR
										requires this be a multiple of 6, while NTV2_FBF_8BIT_YCBCR requires this to be even).
	@param[in]	inSrcHorzPixelsToCopy	Specifies the width of the source image to copy, in pixels. This value can be larger
										than what's possible. This function will ensure that pixels past the right edge of the
										source raster will not be accessed. It is not an error to specify zero, although nothing
										will be copied. Note that some pixel formats set constraints on this value(e.g.,
										NTV2_FBF_10BIT_YCBCR requires this be a multiple of 6, while NTV2_FBF_8BIT_YCBCR requires
										this to be even).
	@return		True if successful;  otherwise false.
	@note		The source and destination buffers MUST have the same pixel format.
	@note		The source and destination buffers must NOT point to the same buffer.
	@note		The use of unsigned values precludes positioning the source raster above the top line of the destination raster,
				or to the left of the destination raster's left edge. This function will, however, clip the source raster if it
				overhangs the bottom and/or right edge of the destination raster.
**/
AJAExport bool	CopyRaster (const NTV2FrameBufferFormat	inPixelFormat,
							UByte *						pDstBuffer,
							const UWord					inDstBytesPerLine,
							const UWord					inDstTotalLines,
							const UWord					inDstVertLineOffset,
							const UWord					inDstHorzPixelOffset,
							const UByte *				pSrcBuffer,
							const UWord					inSrcBytesPerLine,
							const UWord					inSrcTotalLines,
							const UWord					inSrcVertLineOffset,
							const UWord					inSrcVertLinesToCopy,
							const UWord					inSrcHorzPixelOffset,
							const UWord					inSrcHorzPixelsToCopy);

AJAExport NTV2Standard GetNTV2StandardFromScanGeometry (UByte geometry, bool progressiveTransport);
AJAExport NTV2Standard GetNTV2StandardFromVideoFormat (const NTV2VideoFormat inVideoFormat);
#if defined (NTV2_DEPRECATE)
	#define	GetHdmiV2StandardFromVideoFormat(__vf__)	::GetNTV2StandardFromVideoFormat (__vf__)
#else
	AJAExport NTV2V2Standard	GetHdmiV2StandardFromVideoFormat (NTV2VideoFormat videoFormat);
#endif

//#if !defined (NTV2_DEPRECATE_12_6)
	AJAExport ULWord GetVideoActiveSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat,
										const bool inVANCenabled = false, const bool inWideVANC = false);
	AJAExport ULWord GetVideoWriteSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat,
										const bool inVANCenabled, const bool inWideVANC);
//#endif	//	NTV2_DEPRECATE_12_6

/**
	@return		The minimum number of bytes required to store a single frame of video in the given frame buffer format
				having the given video format, including VANC lines, if any.
	@param[in]	inVideoFormat	Specifies the video format.
	@param[in]	inFBFormat		Specifies the frame buffer format.
	@param[in]	inVancMode		Optionally specifies the VANC mode. Defaults to OFF (no VANC lines).
**/
AJAExport ULWord GetVideoActiveSize (const NTV2VideoFormat inVideoFormat,
									const NTV2FrameBufferFormat inFBFormat,
									const NTV2VANCMode inVancMode = NTV2_VANCMODE_OFF);


/**
	@brief		Identical to the ::GetVideoActiveSize function, except rounds the result up to the nearest 4096-byte multiple.
	@return		The number of bytes required to store a single frame of video in the given frame buffer format
				having the given video format, including VANC lines, if any, rounded up to the nearest 4096-byte multiple.
	@param[in]	inVideoFormat	Specifies the video format.
	@param[in]	inFBFormat		Specifies the frame buffer format.
	@param[in]	inVancMode		Optionally specifies the VANC mode. Defaults to OFF (no VANC lines).
**/
AJAExport ULWord GetVideoWriteSize (const NTV2VideoFormat inVideoFormat,  const NTV2FrameBufferFormat inFBFormat,  const NTV2VANCMode inVancMode = NTV2_VANCMODE_OFF);

AJAExport NTV2VideoFormat GetQuarterSizedVideoFormat(NTV2VideoFormat videoFormat);
AJAExport NTV2VideoFormat GetQuadSizedVideoFormat(NTV2VideoFormat videoFormat);
AJAExport NTV2FrameGeometry GetQuarterSizedGeometry(NTV2FrameGeometry geometry);
AJAExport NTV2FrameGeometry Get4xSizedGeometry(NTV2FrameGeometry geometry);

AJAExport double GetFramesPerSecond(NTV2FrameRate frameRate);
AJAExport double GetFrameTime(NTV2FrameRate frameRate);

/**
	@brief		Answers with the given frame rate, in frames per second, as two components:
				the numerator and denominator of the fractional rate.
	@param[in]	inFrameRate				Specifies the frame rate of interest.
	@param[out]	outFractionNumerator	Receives the numerator of the fractional frame rate.
										This will be zero if the function returns false.
	@param[out]	outFractionDenominator	Receives the denominator of the fractional frame rate.
										If the function is successful, this will be either 1000 or 1001.
										This will be zero if the function returns false.
	@return		True if successful;  otherwise false.
**/
AJAExport bool GetFramesPerSecond (const NTV2FrameRate inFrameRate, ULWord & outFractionNumerator, ULWord & outFractionDenominator);

/**
	@brief	Returns the number of audio samples for a given video frame rate, audio sample rate, and frame number.
			This is useful since AJA devices use fixed audio sample rates (typically 48KHz), and some video frame
			rates will necessarily result in some frames having more audio samples than others.
	@param[in]	frameRate		Specifies the video frame rate.
	@param[in]	audioRate		Specifies the audio sample rate. Must be one of NTV2_AUDIO_48K or NTV2_AUDIO_96K.
	@param[in]	cadenceFrame	Optionally specifies a frame number for maintaining proper cadence in a frame sequence,
								for those video frame rates that don't accommodate an even number of audio samples.
								Defaults to zero.
	@param[in]	smpte372Enabled	Specifies that 1080p60, 1080p5994 or 1080p50 is being used. In this mode, the device's
								framerate might be NTV2_FRAMERATE_3000, but since 2 links are coming out, the video rate
								is effectively NTV2_FRAMERATE_6000. Defaults to false.
	@return	The number of audio samples.
**/
AJAExport ULWord				GetAudioSamplesPerFrame (NTV2FrameRate frameRate, NTV2AudioRate audioRate, ULWord cadenceFrame=0,bool smpte372Enabled=false);
AJAExport LWord64				GetTotalAudioSamplesFromFrameNbrZeroUpToFrameNbr (NTV2FrameRate frameRate, NTV2AudioRate audioRate, ULWord frameNbrNonInclusive);
AJAExport ULWord				GetVaricamRepeatCount (NTV2FrameRate sequenceRate, NTV2FrameRate playRate, ULWord cadenceFrame=0);
AJAExport ULWord				GetScaleFromFrameRate (NTV2FrameRate playFrameRate);
AJAExport NTV2FrameRate			GetFrameRateFromScale (long scale, long duration, NTV2FrameRate playFrameRate);
AJAExport NTV2FrameRate			GetNTV2FrameRateFromVideoFormat (NTV2VideoFormat videoFormat);
AJAExport ULWord				GetNTV2FrameGeometryWidth (NTV2FrameGeometry geometry);
AJAExport ULWord				GetNTV2FrameGeometryHeight (NTV2FrameGeometry geometry);
AJAExport ULWord				GetDisplayWidth (NTV2VideoFormat videoFormat);
AJAExport ULWord				GetDisplayHeight (NTV2VideoFormat videoFormat);
AJAExport NTV2ConversionMode	GetConversionMode (NTV2VideoFormat inFormat, NTV2VideoFormat outFormat);
AJAExport NTV2VideoFormat		GetInputForConversionMode (NTV2ConversionMode conversionMode);
AJAExport NTV2VideoFormat		GetOutputForConversionMode (NTV2ConversionMode conversionMode);

AJAExport NTV2Channel			GetNTV2ChannelForIndex (const ULWord inIndex0);
AJAExport ULWord				GetIndexForNTV2Channel (const NTV2Channel inChannel);

AJAExport NTV2Crosspoint		GetNTV2CrosspointChannelForIndex (const ULWord inIndex0);
AJAExport ULWord				GetIndexForNTV2CrosspointChannel (const NTV2Crosspoint inChannel);
AJAExport NTV2Crosspoint		GetNTV2CrosspointInputForIndex (const ULWord inIndex0);
AJAExport ULWord				GetIndexForNTV2CrosspointInput (const NTV2Crosspoint inChannel);
AJAExport NTV2Crosspoint		GetNTV2CrosspointForIndex (const ULWord inIndex0);
AJAExport ULWord				GetIndexForNTV2Crosspoint (const NTV2Crosspoint inChannel);

AJAExport bool					IsNTV2CrosspointInput (const NTV2Crosspoint inChannel);
AJAExport bool					IsNTV2CrosspointOutput (const NTV2Crosspoint inChannel);
AJAExport std::string			NTV2CrosspointToString (const NTV2Crosspoint inChannel);

AJAExport NTV2VideoFormat		GetTransportCompatibleFormat (const NTV2VideoFormat inFormat, const NTV2VideoFormat inTargetFormat);
AJAExport bool					IsTransportCompatibleFormat (const NTV2VideoFormat inFormat1, const NTV2VideoFormat inFormat2);

AJAExport NTV2InputSource		GetNTV2InputSourceForIndex (const ULWord inIndex0);				//	0-based index
AJAExport ULWord				GetIndexForNTV2InputSource (const NTV2InputSource inValue);		//	0-based index


AJAExport NTV2Channel			NTV2CrosspointToNTV2Channel (const NTV2Crosspoint inCrosspointChannel);

/**
	@brief		Converts the given NTV2Channel value into the equivalent input NTV2Crosspoint.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent input NTV2Crosspoint value.
**/
AJAExport NTV2Crosspoint		NTV2ChannelToInputCrosspoint (const NTV2Channel inChannel);

/**
	@brief		Converts the given NTV2Channel value into the equivalent output NTV2Crosspoint.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent output NTV2Crosspoint value.
**/
AJAExport NTV2Crosspoint		NTV2ChannelToOutputCrosspoint (const NTV2Channel inChannel);

/**
	@brief		Converts the given NTV2Channel value into the equivalent input INTERRUPT_ENUMS value.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent input INTERRUPT_ENUMS value.
**/
AJAExport INTERRUPT_ENUMS		NTV2ChannelToInputInterrupt (const NTV2Channel inChannel);

/**
	@brief		Converts the given NTV2Channel value into the equivalent output INTERRUPT_ENUMS value.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent output INTERRUPT_ENUMS value.
**/
AJAExport INTERRUPT_ENUMS		NTV2ChannelToOutputInterrupt (const NTV2Channel inChannel);

/**
	@brief		Converts the given NTV2Channel value into the equivalent NTV2TCIndex value.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@param[in]	inEmbeddedLTC	Specify true for embedded LTC. Defaults to false.
	@return		The equivalent NTV2TCIndex value.
**/
AJAExport NTV2TCIndex			NTV2ChannelToTimecodeIndex (const NTV2Channel inChannel, const bool inEmbeddedLTC = false);

/**
	@brief		Converts the given NTV2TCIndex value into the appropriate NTV2Channel value.
	@param[in]	inTCIndex		Specifies the NTV2TCIndex to be converted.
	@return		The equivalent NTV2Channel value.
**/
AJAExport NTV2Channel			NTV2TimecodeIndexToChannel (const NTV2TCIndex inTCIndex);

/**
	@brief		Converts the given NTV2TCIndex value into the appropriate NTV2InputSource value.
	@param[in]	inTCIndex		Specifies the NTV2TCIndex to be converted.
	@return		The equivalent NTV2InputSource value.
**/
AJAExport NTV2InputSource		NTV2TimecodeIndexToInputSource (const NTV2TCIndex inTCIndex);


#define	NTV2ChannelToCaptureCrosspoint	NTV2ChannelToInputCrosspoint
#define	NTV2ChannelToIngestCrosspoint	NTV2ChannelToInputCrosspoint
#define	NTV2ChannelToInputChannelSpec	NTV2ChannelToInputCrosspoint
#define	NTV2ChannelToPlayoutCrosspoint	NTV2ChannelToOutputCrosspoint
#define	NTV2ChannelToOutputChannelSpec	NTV2ChannelToOutputCrosspoint


/**
	@brief		Converts the given NTV2Framesize value into an exact byte count.
	@param[in]	inFrameSize		Specifies the NTV2Framesize to be converted.
	@return		The equivalent number of bytes.
**/
AJAExport ULWord	NTV2FramesizeToByteCount (const NTV2Framesize inFrameSize);

/**
	@brief		Converts the given NTV2BufferSize value into its exact byte count.
	@param[in]	inBufferSize	Specifies the NTV2AudioBufferSize to be converted.
	@return		The equivalent number of bytes.
**/
AJAExport ULWord	NTV2AudioBufferSizeToByteCount (const NTV2AudioBufferSize inBufferSize);

/**
	@brief		Converts the given NTV2Channel value into its equivalent NTV2EmbeddedAudioInput.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent NTV2EmbeddedAudioInput value.
**/
AJAExport NTV2EmbeddedAudioInput NTV2ChannelToEmbeddedAudioInput (const NTV2Channel inChannel);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2EmbeddedAudioInput value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@return		The equivalent NTV2EmbeddedAudioInput value.
**/
AJAExport NTV2EmbeddedAudioInput NTV2InputSourceToEmbeddedAudioInput (const NTV2InputSource inInputSource);

/**
	@param[in]	inInputSource	Specifies the NTV2InputSource.
	@return		The NTV2AudioSource that corresponds to the given NTV2InputSource.
**/
AJAExport NTV2AudioSource NTV2InputSourceToAudioSource (const NTV2InputSource inInputSource);

/**
 @brief		Converts a given NTV2InputSource to its equivalent NTV2Crosspoint value.
 @param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
 @return		The equivalent NTV2Crosspoint value.
 **/
AJAExport NTV2Crosspoint NTV2InputSourceToChannelSpec (const NTV2InputSource inInputSource);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2Channel value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@return		The equivalent NTV2Channel value.
**/
AJAExport NTV2Channel NTV2InputSourceToChannel (const NTV2InputSource inInputSource);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2ReferenceSource value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@return		The equivalent NTV2ReferenceSource value.
**/
AJAExport NTV2ReferenceSource NTV2InputSourceToReferenceSource (const NTV2InputSource inInputSource);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2AudioSystem value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@return		The equivalent NTV2AudioSystem value.
**/
AJAExport NTV2AudioSystem NTV2InputSourceToAudioSystem (const NTV2InputSource inInputSource);

/**
	@brief		Converts a given NTV2InputSource to its equivalent NTV2TimecodeIndex value.
	@param[in]	inInputSource	Specifies the NTV2InputSource to be converted.
	@param[in]	inEmbeddedLTC	Specify true for embedded ATC LTC. Defaults to false.
	@return		The equivalent NTV2TimecodeIndex value.
**/
AJAExport NTV2TimecodeIndex NTV2InputSourceToTimecodeIndex (const NTV2InputSource inInputSource, const bool inEmbeddedLTC = false);

/**
	@brief		Converts the given NTV2Channel value into its equivalent NTV2AudioSystem.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent NTV2AudioSystem value.
**/
AJAExport NTV2AudioSystem NTV2ChannelToAudioSystem (const NTV2Channel inChannel);

/**
	@brief		Converts the given NTV2Channel value into its ordinary equivalent NTV2InputSource.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent NTV2InputSource value.
**/
AJAExport NTV2InputSource NTV2ChannelToInputSource (const NTV2Channel inChannel);

/**
	@brief		Converts a given NTV2OutputDestination to its equivalent NTV2Channel value.
	@param[in]	inOutputDest	Specifies the NTV2OutputDestination to be converted.
	@return		The equivalent NTV2Channel value.
**/
AJAExport NTV2Channel NTV2OutputDestinationToChannel (const NTV2OutputDestination inOutputDest);

/**
	@brief		Converts the given NTV2Channel value into its ordinary equivalent NTV2OutputDestination.
	@param[in]	inChannel		Specifies the NTV2Channel to be converted.
	@return		The equivalent NTV2OutputDestination value.
**/
AJAExport NTV2OutputDestination NTV2ChannelToOutputDestination (const NTV2Channel inChannel);


/**
	@brief	Compares two frame rates and returns true if they are "compatible" (with respect to a multiformat-capable device).
	@param[in]	inFrameRate1	Specifies one of the frame rates to be compared.
	@param[in]	inFrameRate2	Specifies another frame rate to be compared.
**/
AJAExport bool IsMultiFormatCompatible (const NTV2FrameRate inFrameRate1, const NTV2FrameRate inFrameRate2);

/**
	@brief	Compares two video formats and returns true if they are "compatible" (with respect to a multiformat-capable device).
	@param[in]	inFormat1	Specifies one of the video formats to be compared.
	@param[in]	inFormat2	Specifies another video format to be compared.
**/
AJAExport bool IsMultiFormatCompatible (const NTV2VideoFormat inFormat1, const NTV2VideoFormat inFormat2);

AJAExport bool IsPSF(NTV2VideoFormat format);
AJAExport bool IsProgressivePicture(NTV2VideoFormat format);
AJAExport bool IsProgressiveTransport(NTV2VideoFormat format);
AJAExport bool IsProgressiveTransport(NTV2Standard format);
AJAExport bool IsRGBFormat(NTV2FrameBufferFormat format);
AJAExport bool IsYCbCrFormat(NTV2FrameBufferFormat format);
AJAExport bool IsAlphaChannelFormat(NTV2FrameBufferFormat format);
AJAExport bool Is2KFormat(NTV2VideoFormat format);
AJAExport bool Is4KFormat(NTV2VideoFormat format);
AJAExport bool IsRaw(NTV2FrameBufferFormat format);
AJAExport bool Is8BitFrameBufferFormat(NTV2FrameBufferFormat fbFormat);
AJAExport bool IsVideoFormatA(NTV2VideoFormat format);
AJAExport bool IsVideoFormatB(NTV2VideoFormat format);


AJAExport int  RecordCopyAudio(PULWord pAja, PULWord pSR, int iStartSample, int iNumBytes, int iChan0,
							   int iNumChans, bool bKeepAudio24Bits);

/**
	@brief	Fills the given buffer with 32-bit (ULWord) audio tone samples.
	@param		pAudioBuffer		If non-NULL, must be a valid pointer to the buffer to be filled with audio samples,
									and must be at least  4 x numSamples x numChannels  bytes in size.
									Callers may specify NULL to have the function return the required size of the buffer.
	@param		inOutCurrentSample	On entry, specifies the sample where waveform generation is to resume.
									If audioBuffer is non-NULL, on exit, receives the sample number where waveform generation left off.
									Callers should specify zero for the first invocation of this function.
	@param[in]	inNumSamples		Specifies the number of samples to generate.
	@param[in]	inSampleRate		Specifies the sample rate, in samples per second.
	@param[in]	inAmplitude			Specifies the amplitude of the generated tone.
	@param[in]	inFrequency			Specifies the frequency of the generated tone, in cycles per second (Hertz).
	@param[in]	inNumBits			Specifies the number of bits per sample. Should be between 8 and 32 (inclusive).
	@param[in]	inByteSwap			If true, byte-swaps each 32-bit sample before copying it into the destination buffer.
	@param[in]	inNumChannels		Specifies the number of audio channels to produce.
	@return		The total number of bytes written into the destination buffer (or if audioBuffer is NULL, the minimum
				required size of the destination buffer, in bytes).
**/
AJAExport ULWord	AddAudioTone (	ULWord *		pAudioBuffer,
									ULWord &		inOutCurrentSample,
									const ULWord	inNumSamples,
									const double	inSampleRate,
									const double	inAmplitude,
									const double	inFrequency,
									const ULWord	inNumBits,
									const bool		inByteSwap,
									const ULWord	inNumChannels);

/**
	@brief	Fills the given buffer with 32-bit (ULWord) audio tone samples with a different frequency in each audio channel.
	@param		pAudioBuffer		If non-NULL, must be a valid pointer to the buffer to be filled with audio samples,
									and must be at least  4 x numSamples x numChannels  bytes in size.
									Callers may specify NULL to have the function return the required size of the buffer.
	@param		inOutCurrentSample	On entry, specifies the sample where waveform generation is to resume.
									If audioBuffer is non-NULL, on exit, receives the sample number where waveform generation left off.
									Callers should specify zero for the first invocation of this function.
	@param[in]	inNumSamples		Specifies the number of samples to generate.
	@param[in]	inSampleRate		Specifies the sample rate, in samples per second.
	@param[in]	pInAmplitudes		A valid, non-NULL pointer to an array of per-channel amplitude values.
									This array must contain at least "inNumChannels" entries.
	@param[in]	pInFrequencies		A valid, non-NULL pointer to an array of per-channel frequency values, in cycles per second (Hertz).
									This array must contain at least "inNumChannels" entries.
	@param[in]	inNumBits			Specifies the number of bits per sample. Should be between 8 and 32 (inclusive).
	@param[in]	inByteSwap			If true, byte-swaps each 32-bit sample before copying it into the destination buffer.
	@param[in]	inNumChannels		Specifies the number of audio channels to produce.
	@return		The total number of bytes written into the destination buffer (or if audioBuffer is NULL, the minimum
				required size of the destination buffer, in bytes).
**/
AJAExport ULWord	AddAudioTone (	ULWord *		pAudioBuffer,
									ULWord &		inOutCurrentSample,
									const ULWord	inNumSamples,
									const double	inSampleRate,
									const double *	pInAmplitudes,
									const double *	pInFrequencies,
									const ULWord	inNumBits,
									const bool		inByteSwap,
									const ULWord	inNumChannels);

/**
	@brief	Fills the given buffer with 16-bit (UWord) audio tone samples.
	@param		pAudioBuffer		If non-NULL, must be a valid pointer to the buffer to be filled with audio samples,
									and must be at least numSamples*2*numChannels bytes in size.
									Callers may specify NULL to have the function return the required size of the buffer.
	@param		inOutCurrentSample	On entry, specifies the sample where waveform generation is to resume.
									If audioBuffer is non-NULL, on exit, receives the sample number where waveform generation left off.
									Callers should specify zero for the first invocation of this function.
	@param[in]	inNumSamples		Specifies the number of samples to generate.
	@param[in]	inSampleRate		Specifies the sample rate, in samples per second.
	@param[in]	inAmplitude			Specifies the amplitude of the generated tone.
	@param[in]	inFrequency			Specifies the frequency of the generated tone, in cycles per second (Hertz).
	@param[in]	inNumBits			Specifies the number of bits per sample. Should be between 8 and 16 (inclusive).
	@param[in]	inByteSwap			If true, byte-swaps each 16-bit sample before copying it into the destination buffer.
	@param[in]	inNumChannels		Specifies the number of audio channels to produce.
	@return		The total number of bytes written into the destination buffer (or if audioBuffer is NULL, the minimum
				required size of the destination buffer, in bytes).
**/
AJAExport ULWord	AddAudioTone (	UWord *			pAudioBuffer,
									ULWord &		inOutCurrentSample,
									const ULWord	inNumSamples,
									const double	inSampleRate,
									const double	inAmplitude,
									const double	inFrequency,
									const ULWord	inNumBits,
									const bool		inByteSwap,
									const ULWord	inNumChannels);

AJAExport 
ULWord	AddAudioTestPattern (ULWord*             audioBuffer,
							 ULWord&            currentSample,
							 ULWord             numSamples,
							 ULWord             modulus,
							 bool               endianConvert,
							 ULWord   		    numChannels);

#if !defined (NTV2_DEPRECATE)
	AJAExport bool BuildRoutingTableForOutput (CNTV2SignalRouter &		outRouter,
												NTV2Channel				channel,
												NTV2FrameBufferFormat	fbf,
												bool					convert		= false,	// ignored
												bool					lut			= false,
												bool					dualLink	= false,
												bool					keyOut		= false);

	AJAExport bool BuildRoutingTableForInput (CNTV2SignalRouter &		outRouter,
											   NTV2Channel				channel,
											   NTV2FrameBufferFormat	fbf,
											   bool						withKey		= false,
											   bool						lut			= false,
											   bool						dualLink	= false,
											   bool						EtoE		= true);

	AJAExport bool BuildRoutingTableForInput (CNTV2SignalRouter &		outRouter,
											   NTV2Channel				channel,
											   NTV2FrameBufferFormat	fbf,
											   bool						convert,  // Turn on the conversion module
											   bool						withKey,  // only supported for NTV2_CHANNEL1 for rgb formats with alpha
											   bool						lut,	  // not supported
											   bool						dualLink, // assume coming in RGB(only checked for NTV2_CHANNEL1
											   bool						EtoE);

	AJAExport bool BuildRoutingTableForInput (CNTV2SignalRouter &		outRouter,
											   NTV2InputSource			inputSource,
											   NTV2Channel				channel,
											   NTV2FrameBufferFormat	fbf,
											   bool						convert,  // Turn on the conversion module
											   bool						withKey,  // only supported for NTV2_CHANNEL1 for rgb formats with alpha
											   bool						lut,	  // not supported
											   bool						dualLink, // assume coming in RGB(only checked for NTV2_CHANNEL1
											   bool						EtoE);

	AJAExport ULWord ConvertFusionAnalogToTempCentigrade (ULWord adc10BitValue);

	AJAExport ULWord ConvertFusionAnalogToMilliVolts (ULWord adc10BitValue, ULWord millivoltsResolution);
#endif	//	!defined (NTV2_DEPRECATE)


/**
	@brief	This provides additional information about a video frame for a given video standard or format and pixel format,
			including the total number of lines, number of pixels per line, line pitch, and which line contains the start
			of active video.
**/
typedef struct NTV2FormatDescriptor
{
	ULWord	numLines;			///< @brief	Height -- total number of lines
	ULWord	numPixels;			///< @brief	Width -- total number of pixels per line
	ULWord	linePitch;			///< @brief	Number of 32-bit words per line
	ULWord	firstActiveLine;	///< @brief	First active line of video (0 if NTV2_VANCMODE_OFF)

	/**
		@brief	My default constructor initializes me in an "invalid" state.
	**/
	explicit		NTV2FormatDescriptor ();	///< @brief	My default constructor

	/**
		@brief	Construct from line and pixel count, plus line pitch.
		@param[in]	inNumLines			Specifies the total number of lines.
		@param[in]	inNumPixels			Specifies the total number of pixels.
		@param[in]	inLinePitch			Specifies the line pitch as the number of 32-bit words per line.
		@param[in]	inFirstActiveLine	Optionally specifies the first active line of video, where zero is the first (top) line. Defaults to zero.
	**/
	explicit 		NTV2FormatDescriptor (	const ULWord inNumLines,
											const ULWord inNumPixels,
											const ULWord inLinePitch,
											const ULWord inFirstActiveLine = 0);
//#if !defined (NTV2_DEPRECATE_12_6)
	/**
		@brief		Constructs me from the given video standard, pixel format, whether or not a 2K format is in use, and VANC settings.
		@param[in]	inVideoStandard			Specifies the video standard being used.
		@param[in]	inFrameBufferFormat		Specifies the pixel format of the frame buffer.
		@param[in]	inVANCenabled			Specifies if VANC is enabled or not. Defaults to false.
		@param[in]	in2Kby1080				Specifies if a 2K format is in use or not. Defaults to false.
		@param[in]	inWideVANC				Specifies if "taller VANC" is enabled or not. Defaults to false.
	**/
	explicit		NTV2FormatDescriptor (	const NTV2Standard			inVideoStandard,
											const NTV2FrameBufferFormat	inFrameBufferFormat,
											const bool					inVANCenabled	= false,
											const bool					in2Kby1080		= false,
											const bool					inWideVANC		= false);

	/**
		@brief		Constructs me from the given video format, pixel format and VANC settings.
		@param[in]	inVideoFormat			Specifies the video format being used.
		@param[in]	inFrameBufferFormat		Specifies the pixel format of the frame buffer.
		@param[in]	inVANCenabled			Specifies if VANC is enabled or not. Defaults to false.
		@param[in]	inWideVANC				Specifies if "taller VANC" is enabled or not. Defaults to false.
	**/
	explicit		NTV2FormatDescriptor (	const NTV2VideoFormat		inVideoFormat,
											const NTV2FrameBufferFormat	inFrameBufferFormat,
											const bool					inVANCenabled	= false,
											const bool					inWideVANC		= false);
//#endif	//	!defined (NTV2_DEPRECATE_12_6)

	/**
		@brief		Constructs me from the given video standard, pixel format, and VANC settings.
		@param[in]	inStandard				Specifies the video standard being used.
		@param[in]	inFrameBufferFormat		Specifies the pixel format of the frame buffer.
		@param[in]	inVancMode				Specifies the VANC mode. Defaults to OFF.
	**/
	explicit		NTV2FormatDescriptor (	const NTV2Standard			inStandard,
											const NTV2FrameBufferFormat	inFrameBufferFormat,
											const NTV2VANCMode			inVancMode		= NTV2_VANCMODE_OFF);

	/**
		@brief		Constructs me from the given video format, pixel format and VANC settings.
		@param[in]	inVideoFormat			Specifies the video format being used.
		@param[in]	inFrameBufferFormat		Specifies the pixel format of the frame buffer.
		@param[in]	inVANCenabled			Specifies if VANC is enabled or not. Defaults to false.
		@param[in]	inWideVANC				Specifies if "taller VANC" is enabled or not. Defaults to false.
	**/
	explicit		NTV2FormatDescriptor (	const NTV2VideoFormat		inVideoFormat,
											const NTV2FrameBufferFormat	inFrameBufferFormat,
											const NTV2VANCMode			inVancMode);

	inline bool		IsValid (void) const				{return numLines && numPixels && linePitch;}		///< @return	True if valid;  otherwise false.
	inline bool		IsVANC (void) const					{return firstActiveLine > 0;}						///< @return	True if VANC geometry;  otherwise false.
	inline bool		IsPlanar (void) const				{return NTV2_IS_FBF_PLANAR (mPixelFormat);}			///< @return	True if planar format;  otherwise false.
	inline ULWord	GetTotalRasterBytes (void) const	{return numLines * linePitch * sizeof (ULWord);}	///< @return	The total number of bytes required to hold the raster.
	inline ULWord	GetVisibleRasterBytes (void) const	{return (numLines - firstActiveLine) * linePitch * sizeof (ULWord);}	///< @return	The total number of bytes required to hold only the visible raster.
	inline ULWord	GetBytesPerRow (void) const			{return linePitch * sizeof (ULWord);}				///< @return	The number of bytes per raster row.
	inline ULWord	GetRasterWidth (void) const			{return numPixels;}									///< @return	The width of the raster, in pixels.

	/**
		@return	The height of the raster, in lines.
		@param[in]	inVisibleOnly	Specify true to return just the visible height;  otherwise false (the default) to return the full height.
	**/
	inline ULWord	GetRasterHeight (const bool inVisibleOnly = false) const		{return inVisibleOnly ? numLines - firstActiveLine : numLines;}

	/**
		@return		The full height of the raster, in lines (including VANC, if any).
	**/
	inline ULWord	GetFullRasterHeight (void) const								{return numLines;}

	/**
		@return		The zero-based index number of the first active (visible) line in the raster. This will be zero for non-VANC rasters.
	**/
	inline ULWord	GetFirstActiveLine (void) const									{return firstActiveLine;}

	/**
		@return		The visible height of the raster, in lines (excluding VANC, if any).
	**/
	inline ULWord	GetVisibleRasterHeight (void) const								{return numLines - firstActiveLine;}

	/**
		@return		A pointer to the start of the given row in the given buffer, or NULL if row index is bad
					(using my description of the buffer contents).
		@param[in]	pInStartAddress		A pointer to the raster buffer.
		@param[in]	inRowIndex0			Specifies the row of interest in the buffer, where zero is the topmost row.
	**/
	inline const void *	GetRowAddress (const void * pInStartAddress, const ULWord inRowIndex0) const	{const UByte *	pStart ((const UByte *) pInStartAddress); return inRowIndex0 < numLines ? pStart + inRowIndex0 * GetBytesPerRow () : NULL;}

	/**
		@return		A pointer to the start of the first visible row in the given buffer, or NULL if invalid
					(using my description of the buffer contents).
		@param[in]	pInStartAddress		A pointer to the raster buffer.
	**/
	inline UByte *					GetTopVisibleRowAddress (UByte * pInStartAddress) const				{return (UByte *) GetRowAddress (pInStartAddress, firstActiveLine);}

	/**
		@brief		Compares two buffers line-by-line (using my description of the buffer contents).
		@param[in]	pInStartAddress1		A valid, non-NULL pointer to the start of the first raster buffer.
		@param[in]	pInStartAddress2		A valid, non-NULL pointer to the start of the second raster buffer.
		@param[out]	outFirstChangedRowNum	Receives the zero-based row number of the first row that's different,
											or 0xFFFFFFFF if identical.
		@return		True if successful;  otherwise false.
	**/
	bool							GetFirstChangedRow (const void * pInStartAddress1, const void * pInStartAddress2, ULWord & outFirstChangedRowNum) const;

	/**
		@brief		Compares two buffers line-by-line (using my description of the buffer contents).
		@param[out]	outDiffs	Receives the ordered sequence of line offsets of the lines that differed.
								This will be empty if the two buffers are identical (or if an error occurs).
		@param[in]	pInBuffer1	Specifies the non-NULL address of the first memory buffer whose contents are to be compared.
		@param[in]	pInBuffer2	Specifies the non-NULL address of the second memory buffer whose contents are to be compared.
		@param[in]	inMaxLines	Optionally specifies the maximum number of lines to compare. If zero, all lines are compared.
								Defaults to zero (all lines).
		@return		True if successful;  otherwise false.
		@note		The buffers must be large enough to accommodate my video standard/format or else a memory access violation will occur.
	**/
	bool							GetChangedLines (NTV2RasterLineOffsets & outDiffs, const void * pInBuffer1, const void * pInBuffer2, const ULWord inMaxLines = 0) const;

	/**
		@return	The full-raster NTV2FrameDimensions (including VANC lines, if any).
	**/
	inline NTV2FrameDimensions		GetFullRasterDimensions (void) const					{return NTV2FrameDimensions (GetRasterWidth(), GetRasterHeight(false));}

	/**
		@return	The visible NTV2FrameDimensions (excluding VANC lines, if any).
	**/
	inline NTV2FrameDimensions		GetVisibleRasterDimensions (void) const					{return NTV2FrameDimensions (GetRasterWidth(), GetRasterHeight(true));}

	/**
		@return	True if I'm equal to the given NTV2FormatDescriptor.
		@param[in]	inRHS	The right-hand-side operand that I'll be compared with.
	**/
	bool							operator == (const NTV2FormatDescriptor & inRHS) const;

	/**
		@brief		Writes a human-readable description of me into the given output stream.
		@param		inOutStream		The output stream to be written into.
		@param[in]	inDetailed		If true (the default), writes a detailed description;  otherwise writes a brief one.
		@return		The output stream I was handed.
	**/
	std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = true) const;

	inline NTV2Standard				GetVideoStandard (void) const	{return mStandard;}							///< @return	The video standard I was created with.
	inline NTV2VideoFormat			GetVideoFormat (void) const		{return mVideoFormat;}						///< @return	The video format I was created with.
	inline NTV2FrameBufferFormat	GetPixelFormat (void) const		{return mPixelFormat;}						///< @return	The pixel format I was created with.
	inline NTV2VANCMode				GetVANCMode (void) const		{return mVancMode;}							///< @return	The VANC mode I was created with.
	inline bool						Is2KFormat (void) const			{return m2Kby1080;}							///< @return	True if I was created with a 2Kx1080 video format.
	inline bool						IsQuadRaster (void) const		{return NTV2_IS_QUAD_STANDARD(mStandard) || NTV2_IS_4K_VIDEO_FORMAT(mVideoFormat);}	///< @return	True if I was created with a 4K/UHD video format or standard.
	inline bool						IsTallVanc (void) const			{return mVancMode == NTV2_VANCMODE_TALL;}	///< @return	True if I was created with just "tall" VANC.
	inline bool						IsTallerVanc (void) const		{return mVancMode == NTV2_VANCMODE_TALLER;}	///< @return	True if I was created with "taller" VANC.

	void							MakeInvalid (void);		///< @brief	Resets me into an invalid (NULL) state.

	//	Private Member Data
	private:
		NTV2Standard			mStandard;		///< @brief	My originating video standard
		NTV2VideoFormat			mVideoFormat;	///< @brief	My originating video format (if known)
		NTV2FrameBufferFormat	mPixelFormat;	///< @brief	My originating frame buffer format
		NTV2VANCMode			mVancMode;		///< @brief	My originating VANC mode
		bool					m2Kby1080;		///< @brief	My originating 2Kx1080 setting
		ULWord					mLinePitch[4];	///< @brief	Number of 32-bit words per line for other planes
		UWord					mNumPlanes;		///< @brief	Number of planes

} NTV2FormatDescriptor;


/**
	@brief		Writes the given NTV2FormatDescriptor to the specified output stream.
	@param		inOutStream		Specifies the output stream to receive the human-readable representation of the NTV2FormatDescriptor.
	@param[in]	inFormatDesc	Specifies the NTV2FormatDescriptor instance to print to the output stream.
	@return	A non-constant reference to the specified output stream.
**/
inline std::ostream & operator << (std::ostream & inOutStream, const NTV2FormatDescriptor & inFormatDesc)	{return inFormatDesc.Print (inOutStream);}


//#if !defined (NTV2_DEPRECATE_12_6)
	AJAExport NTV2FormatDescriptor GetFormatDescriptor (const NTV2Standard			inVideoStandard,
														const NTV2FrameBufferFormat	inFrameBufferFormat,
														const bool					inVANCenabled	= false,
														const bool					in2Kby1080		= false,
														const bool					inWideVANC		= false);
//#endif	//	!defined (NTV2_DEPRECATE_12_6)


/**
	@brief		Returns a format descriptor that describes a video frame having the given video format and pixel format.
	@param[in]	inVideoFormat			Specifies the video format being used.
	@param[in]	inFrameBufferFormat		Specifies the pixel format of the frame buffer.
	@param[in]	inVANCenabled			Specifies if VANC is enabled or not. Defaults to false.
	@param[in]	inWideVANC				Specifies if "wide VANC" is enabled or not. Defaults to false.
	@return		A format descriptor that describes a video frame having the given video standard and pixel format.
**/
AJAExport NTV2FormatDescriptor GetFormatDescriptor (const NTV2VideoFormat			inVideoFormat,
													 const NTV2FrameBufferFormat	inFrameBufferFormat,
													 const bool						inVANCenabled	= false,
													 const bool						inWideVANC		= false);

AJAExport bool		UnpackLine_10BitYUVtoUWordSequence (const void * pIn10BitYUVLine, const NTV2FormatDescriptor & inFormatDesc, UWordSequence & out16BitYUVLine);


/**
	@brief		Writes the given NTV2FrameDimensions to the specified output stream.
	@param		inOutStream			Specifies the output stream to receive the human-readable representation of the NTV2FrameDimensions.
	@param[in]	inFrameDimensions	Specifies the NTV2FrameDimensions to print to the output stream.
	@return	A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2FrameDimensions inFrameDimensions);


/**
	@brief		Used to describe Start of Active Video (SAV) location and field dominance for a given NTV2Standard.
				(See GetSmpteLineNumber function.)
**/
typedef struct NTV2SmpteLineNumber
{
	ULWord			smpteFirstActiveLine;	///< @brief	SMPTE line number of first (top-most) active line of video
	ULWord			smpteSecondActiveLine;	///< @brief	SMPTE line number of second active line of video
	bool			firstFieldTop;			///< @brief	True if the first field on the wire is the top-most field in the raster (field dominance)
private:
	NTV2Standard	mStandard;				///< @brief	The NTV2Standard I was constructed from

public:
	/**
		@brief	Constructs me from a given NTV2Standard.
		@param[in]	inStandard		Specifies the NTV2Standard.
	**/
	explicit		NTV2SmpteLineNumber (const NTV2Standard inStandard = NTV2_STANDARD_INVALID);

	/**
		@return		True if valid;  otherwise false.
	**/
	inline bool		IsValid (void) const	{return NTV2_IS_VALID_STANDARD (mStandard) && smpteFirstActiveLine;}

	/**
		@brief	Returns the SMPTE line number of the Start of Active Video (SAV).
		@param[in]	inRasterFieldID		Specifies a valid raster field ID (not the wire field ID).
										Defaults to NTV2_FIELD0 (i.e. first field) of the raster.
										Use NTV2_FIELD1 for the starting line of a PsF frame.
	**/
	ULWord			GetFirstActiveLine (const NTV2FieldID inRasterFieldID = NTV2_FIELD0) const;

	/**
		@return	True if I'm equal to the given NTV2SmpteLineNumber.
		@param[in]	inRHS	The right-hand-side operand that I'll be compared with.
	**/
	bool			operator == (const NTV2SmpteLineNumber & inRHS) const;

	/**
		@brief		Writes a human-readable description of me into the given output stream.
		@param		inOutStream		The output stream to be written into.
		@return		The output stream I was handed.
	**/
	std::ostream &	Print (std::ostream & inOutStream) const;

	/**
		@param[in]	inLineOffset		Specifies a line offset to add to the printed SMPTE line number.
										Defaults to zero.
		@param[in]	inRasterFieldID		Specifies a valid raster field ID (not the wire field ID).
										Defaults to NTV2_FIELD0 (i.e. first field) of the raster.
		@return		A string containing the human-readable line number.
	**/
	std::string		PrintLineNumber (const ULWord inLineOffset = 0, const NTV2FieldID inRasterFieldID = NTV2_FIELD0) const;

private:
	explicit inline	NTV2SmpteLineNumber (const ULWord inFirstActiveLine,
										const ULWord inSecondActiveLine,
										const bool inFirstFieldTop,
										const NTV2Standard inStandard)
					:	smpteFirstActiveLine	(inFirstActiveLine),
						smpteSecondActiveLine	(inSecondActiveLine),
						firstFieldTop			(inFirstFieldTop),
						mStandard				(inStandard)	{	}

} NTV2SmpteLineNumber;


/**
	@brief		Writes the given NTV2SmpteLineNumber to the specified output stream.
	@param		inOutStream			Specifies the output stream to receive the human-readable representation of the NTV2SmpteLineNumber.
	@param[in]	inSmpteLineNumber	Specifies the NTV2SmpteLineNumber instance to print to the output stream.
	@return	A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2SmpteLineNumber & inSmpteLineNumber);


/**
	@brief		For the given video standard, returns the SMPTE-designated line numbers for Field 1 and Field 2 that correspond
				to the start-of-active-video (SAV).
	@param[in]	inStandard	Specifies the video standard of interest.
	@return		The NTV2SmpteLineNumber structure that corresponds to the given video standard.
**/
inline NTV2SmpteLineNumber GetSmpteLineNumber (const NTV2Standard inStandard)	{return NTV2SmpteLineNumber (inStandard);}

#if !defined (NTV2_DEPRECATE)
	extern AJAExport const NTV2FormatDescriptor formatDescriptorTable [NTV2_NUM_STANDARDS] [NTV2_FBF_NUMFRAMEBUFFERFORMATS];
#endif	//	!defined (NTV2_DEPRECATE)


typedef std::set <NTV2DeviceID>			NTV2DeviceIDSet;			///< @brief	A set of NTV2DeviceIDs.
typedef NTV2DeviceIDSet::iterator		NTV2DeviceIDSetIter;		///< @brief	A convenient non-const iterator for NTV2DeviceIDSet.
typedef NTV2DeviceIDSet::const_iterator	NTV2DeviceIDSetConstIter;	///< @brief	A convenient const iterator for NTV2DeviceIDSet.

/**
	@brief	Returns an NTV2DeviceIDSet of devices supported by the SDK.
	@return	An NTV2DeviceIDSet of devices supported by the SDK.
**/
AJAExport NTV2DeviceIDSet NTV2GetSupportedDevices		(void);

AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2DeviceIDSet & inSet);		///<	@brief	Handy ostream writer for NTV2DeviceIDSet.


typedef std::vector <NTV2OutputCrosspointID>		NTV2OutputCrosspointIDs;			///< @brief	An ordered sequence of NTV2OutputCrosspointID values.
typedef NTV2OutputCrosspointIDs::iterator			NTV2OutputCrosspointIDsIter;		///< @brief	A convenient non-const iterator for NTV2OutputCrosspointIDs.
typedef NTV2OutputCrosspointIDs::const_iterator		NTV2OutputCrosspointIDsConstIter;	///< @brief	A convenient const iterator for NTV2OutputCrosspointIDs.

AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2OutputCrosspointIDs & inList);	///<	@brief	Handy ostream writer for NTV2OutputCrosspointIDs.


typedef std::vector <NTV2InputCrosspointID>			NTV2InputCrosspointIDs;				///< @brief	An ordered sequence of NTV2InputCrosspointID values.
typedef NTV2InputCrosspointIDs::iterator			NTV2InputCrosspointIDsIter;			///< @brief	A convenient non-const iterator for NTV2InputCrosspointIDs.
typedef NTV2InputCrosspointIDs::const_iterator		NTV2InputCrosspointIDsConstIter;	///< @brief	A convenient const iterator for NTV2InputCrosspointIDs.

AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2OutputCrosspointIDs & inList);	///<	@brief	Handy ostream writer for NTV2OutputCrosspointIDs.


//	These are new with 12.0 SDK...
AJAExport std::string NTV2DeviceIDToString				(const NTV2DeviceID				inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2VideoFormatToString			(const NTV2VideoFormat			inValue,	const bool inUseFrameRate = false);
AJAExport std::string NTV2StandardToString				(const NTV2Standard				inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2FrameBufferFormatToString		(const NTV2FrameBufferFormat	inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2M31VideoPresetToString		(const M31VideoPreset			inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2FrameGeometryToString			(const NTV2FrameGeometry		inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2FrameRateToString				(const NTV2FrameRate			inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2InputSourceToString			(const NTV2InputSource			inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2OutputDestinationToString		(const NTV2OutputDestination	inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2ReferenceSourceToString		(const NTV2ReferenceSource		inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2RegisterWriteModeToString		(const NTV2RegisterWriteMode	inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2InterruptEnumToString			(const INTERRUPT_ENUMS			inInterruptEnumValue);
AJAExport std::string NTV2ChannelToString				(const NTV2Channel				inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2AudioSystemToString			(const NTV2AudioSystem			inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2AudioRateToString				(const NTV2AudioRate			inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2AudioBufferSizeToString		(const NTV2AudioBufferSize		inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2AudioLoopBackToString			(const NTV2AudioLoopBack		inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2AudioMonitorSelectToString	(const NTV2AudioMonitorSelect	inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2EmbeddedAudioClockToString	(const NTV2EmbeddedAudioClock	inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2GetBitfileName				(const NTV2DeviceID				inValue);
AJAExport bool        NTV2IsCompatibleBitfileName		(const std::string & inBitfileName, const NTV2DeviceID inDeviceID);
AJAExport NTV2DeviceID NTV2GetDeviceIDFromBitfileName	(const std::string & inBitfileName);
AJAExport std::string NTV2GetFirmwareFolderPath			(void);
AJAExport std::ostream & operator << (std::ostream & inOutStream, const RP188_STRUCT & inObj);
AJAExport std::string NTV2GetVersionString				(const bool inDetailed = false);
AJAExport std::string NTV2RegisterNumberToString		(const NTV2RegisterNumber		inValue);
AJAExport std::string AutoCircVidProcModeToString		(const AutoCircVidProcMode		inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2ColorCorrectionModeToString	(const NTV2ColorCorrectionMode	inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2InputCrosspointIDToString		(const NTV2InputCrosspointID	inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2OutputCrosspointIDToString	(const NTV2OutputCrosspointID	inValue,	const bool inForRetailDisplay = false);
AJAExport std::string NTV2WidgetIDToString				(const NTV2WidgetID				inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2TaskModeToString				(const NTV2EveryFrameTaskMode	inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2RegNumSetToString				(const NTV2RegisterNumberSet &	inValue);
AJAExport std::string NTV2TCIndexToString				(const NTV2TCIndex				inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2AudioChannelPairToString		(const NTV2AudioChannelPair		inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2AudioChannelQuadToString		(const NTV2Audio4ChannelSelect	inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2AudioChannelOctetToString		(const NTV2Audio8ChannelSelect	inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2FramesizeToString				(const NTV2Framesize			inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2ModeToString					(const NTV2Mode					inValue,	const bool inCompactDisplay = false);
AJAExport std::string NTV2VANCModeToString				(const NTV2VANCMode				inValue,	const bool inCompactDisplay = false);
AJAExport bool	convertHDRFloatToRegisterValues			(const HDRFloatValues & inFloatValues,		HDRRegValues & outRegisterValues);
AJAExport bool	convertHDRRegisterToFloatValues			(const HDRRegValues & inRegisterValues,		HDRFloatValues & outFloatValues);
AJAExport void  setHDRDefaultsForBT2020                 (HDRRegValues & outRegisterValues);
AJAExport void  setHDRDefaultsForDCIP3                  (HDRRegValues & outRegisterValues);

//	New in 12.5 SDK...
typedef std::vector <std::string>		NTV2StringList;
typedef NTV2StringList::const_iterator	NTV2StringListConstIter;
typedef std::set <std::string>			NTV2StringSet;
typedef NTV2StringSet::const_iterator	NTV2StringSetConstIter;

AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2StringSet & inData);

//	Register classifier keys
#define	kRegClass_NULL		std::string ()
#define	kRegClass_Audio		std::string ("kRegClass_Audio")
#define	kRegClass_Video		std::string ("kRegClass_Video")
#define	kRegClass_Anc		std::string ("kRegClass_Anc")
#define	kRegClass_DMA		std::string ("kRegClass_DMA")
#define	kRegClass_Mixer		std::string ("kRegClass_Mixer")
#define	kRegClass_Serial	std::string ("kRegClass_Serial")
#define	kRegClass_Timecode	std::string ("kRegClass_Timecode")
#define	kRegClass_Routing	std::string ("kRegClass_Routing")
#define	kRegClass_Input		std::string ("kRegClass_Input")
#define	kRegClass_Output	std::string ("kRegClass_Output")
#define	kRegClass_Color		std::string ("kRegClass_Color")
#define	kRegClass_Analog	std::string ("kRegClass_Analog")
#define	kRegClass_AES		std::string ("kRegClass_AES")
#define	kRegClass_HDMI		std::string ("kRegClass_HDMI")
#define	kRegClass_VPID		std::string ("kRegClass_VPID")
#define	kRegClass_Timing	std::string ("kRegClass_Timing")
#define	kRegClass_Channel1	std::string ("kRegClass_Channel1")
#define	kRegClass_Channel2	std::string ("kRegClass_Channel2")
#define	kRegClass_Channel3	std::string ("kRegClass_Channel3")
#define	kRegClass_Channel4	std::string ("kRegClass_Channel4")
#define	kRegClass_Channel5	std::string ("kRegClass_Channel5")
#define	kRegClass_Channel6	std::string ("kRegClass_Channel6")
#define	kRegClass_Channel7	std::string ("kRegClass_Channel7")
#define	kRegClass_Channel8	std::string ("kRegClass_Channel8")
#define	kRegClass_ReadOnly	std::string ("kRegClass_ReadOnly")
#define	kRegClass_WriteOnly	std::string ("kRegClass_WriteOnly")
#define	kRegClass_Virtual	std::string ("kRegClass_Virtual")


AJAExport NTV2RegisterReads	FromRegNumSet	(const NTV2RegNumSet &		inRegNumSet);
AJAExport NTV2RegNumSet		ToRegNumSet		(const NTV2RegisterReads &	inRegReads);

/**
	@brief	I provide information about registers and their values.
**/
class AJAExport CNTV2RegisterExpert
{
	public:
		/**
			@param[in]	inRegNum	Specifies the register number.
			@return		A string that contains the name of the register (or empty if unknown).
		**/
		static std::string		GetDisplayName	(const uint32_t inRegNum);

		/**
			@param[in]	inRegNum	Specifies the register number.
			@param[in]	inRegValue	Specifies the 32-bit register value.
			@param[in]	inDeviceID	Optionally specifies an NTV2DeviceID. Defaults to DEVICE_ID_NOTFOUND.
			@return		A string that contains the human-readable rendering of the value of the given register
						(or empty if unknown).
		**/
		static std::string		GetDisplayValue	(const uint32_t inRegNum, const uint32_t inRegValue, const NTV2DeviceID inDeviceID = DEVICE_ID_NOTFOUND);

		/**
			@param[in]	inRegNum	Specifies the register number.
			@param[in]	inClassName	Specifies the class name.
			@return		True if the register is a member of the class;  otherwise false.
		**/
		static bool				IsRegisterInClass (const uint32_t inRegNum, const std::string & inClassName);

		/**
			@param[in]	inRegNum	Specifies the register number.
			@return		True if the register is read-only (i.e., it cannot be written);  otherwise false.
		**/
		static inline bool		IsReadOnly		(const uint32_t inRegNum)	{return IsRegisterInClass (inRegNum, kRegClass_ReadOnly);}

		/**
			@param[in]	inRegNum	Specifies the register number.
			@return		True if the register is write-only (i.e., it cannot be read);  otherwise false.
		**/
		static inline bool		IsWriteOnly		(const uint32_t inRegNum)	{return IsRegisterInClass (inRegNum, kRegClass_WriteOnly);}

		/**
			@return		A set of strings containing the names of all known register classes.
		**/
		static NTV2StringSet	GetAllRegisterClasses (void);

		/**
			@return		A set of strings containing the names of all register classes the given register belongs to.
		**/
		static NTV2StringSet	GetRegisterClasses (const uint32_t inRegNum);

		/**
			@param[in]	inClassName	Specifies the register class.
			@return		A set of register numbers that belong to the specified class. Will be empty if none found.
		**/
		static NTV2RegNumSet	GetRegistersForClass	(const std::string & inClassName);

		/**
			@param[in]	inChannel	Specifies a valid NTV2Channel.
			@return		A set of register numbers that are associated with the given NTV2Channel (class kRegClass_ChannelN).
						Will be empty if an invalid NTV2Channel is specified.
		**/
		static NTV2RegNumSet	GetRegistersForChannel	(const NTV2Channel inChannel);

		/**
			@param[in]	inDeviceID			Specifies a valid NTV2DeviceID.
			@param[in]	inIncludeVirtuals	Specify true to include virtual registers;  otherwise virtual registers
											will be excluded (the default).
			@return		A set of register numbers that are legal for the device having the given NTV2DeviceID.
						Will be empty if an invalid NTV2DeviceID is specified.
		**/
		static NTV2RegNumSet	GetRegistersForDevice (const NTV2DeviceID inDeviceID, const bool inIncludeVirtuals = false);

		/**
			@param[in]	inName			Specifies a non-empty string that contains all or part of a register name.
			@param[in]	inSearchStyle	Specifies the search style. Must be EXACTMATCH (the default), CONTAINS,
										STARTSWITH or ENDSWITH.
			@return		A set of register numbers that match all or part of the given name.
						Empty if none match.
			@note		All searching is performed case-insensitively.
		**/
		static NTV2RegNumSet	GetRegistersWithName (const std::string & inName, const int inSearchStyle = EXACTMATCH);

		/**
			@param[in]	inXptRegNum		Specifies the crosspoint select group register number. Note that it only makes
										sense to pass register numbers having names that start with "kRegXptSelectGroup".
			@param[in]	inMaskIndex		Specifies the mask index, an unsigned value that must be less than 4.
										(0 specifies the least significant byte of the crosspoint register value,
										3 specifies the most significant byte, etc.)
			@return		The NTV2InputCrosspointID of the widget input crosspoint associated with the given "Xpt"
						register and mask index, or NTV2_INPUT_CROSSPOINT_INVALID if there isn't one.
		**/
		static NTV2InputCrosspointID	GetInputCrosspointID (const uint32_t inXptRegNum, const uint32_t inMaskIndex);

		/**
			@brief		Answers with the crosspoint select register and mask information for a given widget input.
			@param[in]	inInputXpt		Specifies the NTV2InputCrosspointID of interest.
			@param[in]	outXptRegNum	Receives the crosspoint select group register number.
			@param[in]	outMaskIndex	Receives the mask index (where 0=0x000000FF, 1=0x0000FF00, 2=0x00FF0000, 3=0xFF000000).
			@return		True if successful;  otherwise false.
		**/
		static bool						GetCrosspointSelectGroupRegisterInfo (const NTV2InputCrosspointID inInputXpt, uint32_t & outXptRegNum, uint32_t & outMaskIndex);

		static const int	CONTAINS	=	0;
		static const int	STARTSWITH	=	1;
		static const int	ENDSWITH	=	2;
		static const int	EXACTMATCH	=	3;

};	//	CNTV2RegisterExpert


//	FUTURE	** THESE WILL BE DISAPPEARING **		Deprecate in favor of the new "NTV2xxxxxxToString" functions...
#define	NTV2CrosspointIDToString	NTV2OutputCrosspointIDToString	///< @deprecated	Use NTV2OutputCrosspointIDToString
#if !defined (NTV2_DEPRECATE)
	AJAExport std::string NTV2V2StandardToString			(const NTV2V2Standard			inValue,	const bool inForRetailDisplay = false);
	extern AJAExport NTV2_DEPRECATED	const char *	NTV2VideoFormatStrings		[];	///< @deprecated	Use NTV2VideoFormatToString instead.
	extern AJAExport NTV2_DEPRECATED	const char *	NTV2VideoStandardStrings	[];	///< @deprecated	Use NTV2StandardToString instead.
	extern AJAExport NTV2_DEPRECATED	const char *	NTV2PixelFormatStrings		[];	///< @deprecated	Use NTV2FrameBufferFormatToString instead.
	extern AJAExport NTV2_DEPRECATED	const char *	NTV2FrameRateStrings		[];	///< @deprecated	Use NTV2FrameRateToString instead.
	extern AJAExport NTV2_DEPRECATED	const char *	frameBufferFormats			[];	///< @deprecated	Use NTV2FrameBufferFormatToString instead.

	AJAExport NTV2_DEPRECATED	std::string		frameBufferFormatString		(NTV2FrameBufferFormat inFrameBufferFormat);		///< @deprecated	Use NTV2FrameBufferFormatToString and pass 'true' for 'inForRetailDisplay'
	AJAExport NTV2_DEPRECATED	void			GetNTV2BoardString			(NTV2BoardID inBoardID, std::string & outString);	///< @deprecated	Use NTV2DeviceIDToString and concatenate a space instead

	AJAExport NTV2_DEPRECATED	std::string		NTV2BoardIDToString			(const NTV2BoardID inValue, const bool inForRetailDisplay = false);	///< @deprecated	Use NTV2DeviceIDToString(NTV2DeviceID,bool) instead.
	AJAExport NTV2_DEPRECATED	void			GetNTV2RetailBoardString	(NTV2BoardID inBoardID, std::string & outString);	///< @deprecated	Use NTV2DeviceIDToString(NTV2DeviceID,bool) instead.
	AJAExport NTV2_DEPRECATED	NTV2BoardType	GetNTV2BoardTypeForBoardID	(NTV2BoardID inBoardID);							///< @deprecated	This function is obsolete because NTV2BoardType is obsolete.
#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2UTILS_H
