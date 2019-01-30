/**
	@file		ntv2utils.cpp
	@brief		Implementations for the NTV2 utility functions.
	@copyright	(C) 2004-2019 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#include "ajatypes.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2registerexpert.h"
#include "ntv2videodefines.h"
#include "ntv2audiodefines.h"
#include "ntv2endian.h"
#include "ntv2debug.h"
#include "ntv2transcode.h"
#include "ntv2devicefeatures.h"
#include "ajabase/system/lock.h"
#if defined(AJALinux)
	#include <string.h>  // For memset
	#include <stdint.h>

#endif
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>


using namespace std;

#if defined (NTV2_DEPRECATE)
	#define	AJA_LOCAL_STATIC	static
#else	//	!defined (NTV2_DEPRECATE)
	#define	AJA_LOCAL_STATIC
#endif	//	!defined (NTV2_DEPRECATE)


//////////////////////////////////////////////////////
//	BEGIN SECTION MOVED FROM 'videoutilities.cpp'
//////////////////////////////////////////////////////

uint32_t CalcRowBytesForFormat (const NTV2FrameBufferFormat inPixelFormat, const uint32_t inPixelWidth)
{
	uint32_t rowBytes = 0;

	switch (inPixelFormat)
	{
	case NTV2_FBF_8BIT_YCBCR:
	case NTV2_FBF_8BIT_YCBCR_YUY2:	
		rowBytes = inPixelWidth * 2;
		break;

	case NTV2_FBF_10BIT_YCBCR:	
	case NTV2_FBF_10BIT_YCBCR_DPX:
		rowBytes = (( inPixelWidth % 48 == 0 ) ? inPixelWidth : (((inPixelWidth / 48 ) + 1) * 48)) * 8 / 3;
		break;
	
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_10BIT_DPX:
    case NTV2_FBF_10BIT_DPX_LE:
	case NTV2_FBF_10BIT_RGB_PACKED:
	case NTV2_FBF_ARGB:	
	case NTV2_FBF_RGBA:
	case NTV2_FBF_ABGR:
		rowBytes = inPixelWidth * 4;
		break;

	case NTV2_FBF_24BIT_RGB:
	case NTV2_FBF_24BIT_BGR:
		rowBytes = inPixelWidth * 3;
		break;

 	case NTV2_FBF_8BIT_DVCPRO:
 		rowBytes = inPixelWidth * 2/4;
		break;

	case NTV2_FBF_48BIT_RGB:
		rowBytes = inPixelWidth * 6;
		break;

	case NTV2_FBF_10BIT_YCBCR_420PL2:
	case NTV2_FBF_10BIT_YCBCR_422PL2:
		rowBytes = inPixelWidth * 20 / 16;
		break;

	case NTV2_FBF_8BIT_YCBCR_420PL2:
	case NTV2_FBF_8BIT_YCBCR_422PL2:
		rowBytes = inPixelWidth;
		break;
		
	case NTV2_FBF_8BIT_YCBCR_420PL3:
	case NTV2_FBF_8BIT_HDV:
	case NTV2_FBF_10BIT_YCBCRA:
	case NTV2_FBF_PRORES:
	case NTV2_FBF_PRORES_DVCPRO:
	case NTV2_FBF_PRORES_HDV:
	case NTV2_FBF_10BIT_ARGB:
	case NTV2_FBF_16BIT_ARGB:
	case NTV2_FBF_8BIT_YCBCR_422PL3:
	case NTV2_FBF_10BIT_RAW_RGB:
	case NTV2_FBF_10BIT_RAW_YCBCR:
	case NTV2_FBF_NUMFRAMEBUFFERFORMATS:
	case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
	case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
		//	TO DO.....add more
		break;
	}

	return rowBytes;
}


bool UnpackLine_10BitYUVtoUWordSequence (const void * pIn10BitYUVLine, UWordSequence & out16BitYUVLine, ULWord inNumPixels)
{
	out16BitYUVLine.clear ();
	const ULWord *	pInputLine	(reinterpret_cast <const ULWord *> (pIn10BitYUVLine));

	if (!pInputLine)
		return false;	//	bad pointer
	if (inNumPixels < 6)
		return false;	//	bad width
	if (inNumPixels % 6)
		inNumPixels -= inNumPixels % 6;

	const ULWord	totalULWords	(inNumPixels * 4 / 6);	//	4 ULWords per 6 pixels

	for (ULWord inputCount (0);  inputCount < totalULWords;  inputCount++)
	{
		out16BitYUVLine.push_back ((pInputLine [inputCount]      ) & 0x3FF);
		out16BitYUVLine.push_back ((pInputLine [inputCount] >> 10) & 0x3FF);
		out16BitYUVLine.push_back ((pInputLine [inputCount] >> 20) & 0x3FF);
	}
	return true;
}


bool UnpackLine_10BitYUVtoUWordSequence (const void * pIn10BitYUVLine, const NTV2FormatDescriptor & inFormatDesc, UWordSequence & out16BitYUVLine)
{
	out16BitYUVLine.clear ();
	const ULWord *	pInputLine	(reinterpret_cast <const ULWord *> (pIn10BitYUVLine));

	if (!pInputLine)
		return false;	//	bad pointer
	if (!inFormatDesc.IsValid ())
		return false;	//	bad formatDesc
	if (inFormatDesc.GetRasterWidth () < 6)
		return false;	//	bad width
	if (inFormatDesc.GetPixelFormat() != NTV2_FBF_10BIT_YCBCR)
		return false;	//	wrong FBF

	for (ULWord inputCount (0);  inputCount < inFormatDesc.linePitch;  inputCount++)
	{
		out16BitYUVLine.push_back ((pInputLine [inputCount]      ) & 0x3FF);
		out16BitYUVLine.push_back ((pInputLine [inputCount] >> 10) & 0x3FF);
		out16BitYUVLine.push_back ((pInputLine [inputCount] >> 20) & 0x3FF);
	}
	return true;
}


// UnPack10BitYCbCrBuffer
// UnPack 10 Bit YCbCr Data to 16 bit Word per component
void UnPack10BitYCbCrBuffer( uint32_t* packedBuffer, uint16_t* ycbcrBuffer, uint32_t numPixels )
{
	for (  uint32_t sampleCount = 0, dataCount = 0; 
		sampleCount < (numPixels*2) ; 
		sampleCount+=3,dataCount++ )
	{
		ycbcrBuffer[sampleCount]   =  packedBuffer[dataCount]&0x3FF;  
		ycbcrBuffer[sampleCount+1] = (packedBuffer[dataCount]>>10)&0x3FF;  
		ycbcrBuffer[sampleCount+2] = (packedBuffer[dataCount]>>20)&0x3FF;  

	}
}

// PackTo10BitYCbCrBuffer
// Pack 16 bit Word per component to 10 Bit YCbCr Data 
void PackTo10BitYCbCrBuffer (const uint16_t * ycbcrBuffer, uint32_t * packedBuffer, const uint32_t numPixels)
{
	for ( uint32_t inputCount=0, outputCount=0; 
		inputCount < (numPixels*2);
		outputCount += 4, inputCount += 12 )
	{
		packedBuffer[outputCount]   = uint32_t (ycbcrBuffer[inputCount+0]) + uint32_t (ycbcrBuffer[inputCount+1]<<10) + uint32_t (ycbcrBuffer[inputCount+2]<<20);
		packedBuffer[outputCount+1] = uint32_t (ycbcrBuffer[inputCount+3]) + uint32_t (ycbcrBuffer[inputCount+4]<<10) + uint32_t (ycbcrBuffer[inputCount+5]<<20);
		packedBuffer[outputCount+2] = uint32_t (ycbcrBuffer[inputCount+6]) + uint32_t (ycbcrBuffer[inputCount+7]<<10) + uint32_t (ycbcrBuffer[inputCount+8]<<20);
		packedBuffer[outputCount+3] = uint32_t (ycbcrBuffer[inputCount+9]) + uint32_t (ycbcrBuffer[inputCount+10]<<10) + uint32_t (ycbcrBuffer[inputCount+11]<<20);
	}
}

void MakeUnPacked10BitYCbCrBuffer( uint16_t* buffer, uint16_t Y , uint16_t Cb , uint16_t Cr,uint32_t numPixels )
{
	// assumes lineData is large enough for numPixels
	for ( uint32_t count = 0; count < numPixels*2; count+=4 )
	{
		buffer[count] = Cb;
		buffer[count+1] = Y;
		buffer[count+2] = Cr;
		buffer[count+3] = Y;
	}	
}


// ConvertLineTo8BitYCbCr
// 10 Bit YCbCr to 8 Bit YCbCr
void ConvertLineTo8BitYCbCr(uint16_t * ycbcr10BitBuffer, uint8_t * ycbcr8BitBuffer,	uint32_t numPixels)
{
	for ( uint32_t pixel=0;pixel<numPixels*2;pixel++)
	{
		ycbcr8BitBuffer[pixel] = ycbcr10BitBuffer[pixel]>>2;
	}

}

//***********************************************************************************************************

// ConvertUnpacked10BitYCbCrToPixelFormat()
//		Converts a line of "unpacked" 10-bit Y/Cb/Cr pixels into a "packed" line in the pixel format
//	for the current frame buffer format.
void ConvertUnpacked10BitYCbCrToPixelFormat(uint16_t *unPackedBuffer, uint32_t *packedBuffer, uint32_t numPixels, NTV2FrameBufferFormat pixelFormat,
											bool bUseSmpteRange, bool bAlphaFromLuma)
{
	bool  bIsSD = false;
	if(numPixels < 1280)
		bIsSD = true;

	switch(pixelFormat) 
	{
		case NTV2_FBF_10BIT_YCBCR:
			PackTo10BitYCbCrBuffer(unPackedBuffer, packedBuffer, numPixels);
			break;

		case NTV2_FBF_10BIT_YCBCR_DPX:
			RePackLineDataForYCbCrDPX(packedBuffer, CalcRowBytesForFormat(NTV2_FBF_10BIT_YCBCR_DPX, numPixels));
			break;

		case NTV2_FBF_8BIT_YCBCR:
			ConvertLineTo8BitYCbCr(unPackedBuffer,(uint8_t*)packedBuffer, numPixels);
			break;

		case NTV2_FBF_8BIT_YCBCR_YUY2:
			ConvertLineTo8BitYCbCr(unPackedBuffer,(uint8_t*)packedBuffer, numPixels);
			Convert8BitYCbCrToYUY2((uint8_t*)packedBuffer, numPixels);
			break;
			
		case NTV2_FBF_10BIT_RGB:
			ConvertLineto10BitRGB(unPackedBuffer,(RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD, bUseSmpteRange);
			PackRGB10BitFor10BitRGB((RGBAlpha10BitPixel*)packedBuffer,numPixels);
			break;

		case NTV2_FBF_10BIT_RGB_PACKED:
			ConvertLineto10BitRGB(unPackedBuffer, (RGBAlpha10BitPixel*)packedBuffer, numPixels, bIsSD, bUseSmpteRange);
			PackRGB10BitFor10BitRGBPacked((RGBAlpha10BitPixel*)packedBuffer, numPixels);
			break;
			
		case NTV2_FBF_10BIT_DPX:
			ConvertLineto10BitRGB(unPackedBuffer,(RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD, bUseSmpteRange);
			PackRGB10BitFor10BitDPX((RGBAlpha10BitPixel*)packedBuffer,numPixels);
			break;

        case NTV2_FBF_10BIT_DPX_LE:
			ConvertLineto10BitRGB(unPackedBuffer,(RGBAlpha10BitPixel*)packedBuffer,numPixels, bIsSD, bUseSmpteRange);
			PackRGB10BitFor10BitDPX((RGBAlpha10BitPixel*)packedBuffer,numPixels, false);
			break;
		
		case NTV2_FBF_ARGB:
			ConvertLinetoRGB(unPackedBuffer,(RGBAlphaPixel*)packedBuffer,numPixels, bIsSD, bUseSmpteRange, bAlphaFromLuma);
			break;
		
		case NTV2_FBF_RGBA:
			ConvertLinetoRGB(unPackedBuffer,(RGBAlphaPixel*)packedBuffer,numPixels, bIsSD, bUseSmpteRange, bAlphaFromLuma);
			ConvertARGBYCbCrToRGBA((UByte*)packedBuffer,numPixels);
			break;
			
		case NTV2_FBF_ABGR:
			ConvertLinetoRGB(unPackedBuffer,(RGBAlphaPixel*)packedBuffer,numPixels, bIsSD, bUseSmpteRange, bAlphaFromLuma);
			ConvertARGBYCbCrToABGR((UByte*)packedBuffer,numPixels);
			break;

		case NTV2_FBF_24BIT_BGR:
			ConvertLinetoRGB(unPackedBuffer,(RGBAlphaPixel*)packedBuffer,numPixels, bIsSD, bUseSmpteRange);
			ConvertARGBToBGR((UByte*)packedBuffer, (UByte*)packedBuffer, numPixels);
			break;

		case NTV2_FBF_24BIT_RGB:
			ConvertLinetoRGB(unPackedBuffer,(RGBAlphaPixel*)packedBuffer,numPixels, bIsSD, bUseSmpteRange);
			ConvertARGBToRGB((UByte*)packedBuffer, (UByte*)packedBuffer, numPixels);
			break;

		case NTV2_FBF_48BIT_RGB:
			ConvertLineto16BitRGB(unPackedBuffer, (RGBAlpha16BitPixel*)packedBuffer, numPixels, bIsSD, bUseSmpteRange);
			Convert16BitARGBTo16BitRGB((RGBAlpha16BitPixel*)packedBuffer, (UWord*)packedBuffer, numPixels);
		
		default:
			// TO DO: add all other formats.

			break;
	}
}

// MaskUnPacked10BitYCbCrBuffer
// Mask Data In place based on signalMask
void MaskUnPacked10BitYCbCrBuffer( uint16_t* ycbcrUnPackedBuffer, uint16_t signalMask , uint32_t numPixels )
{
	uint32_t pixelCount;

	// Not elegant but fairly fast.
	switch ( signalMask )
	{
	case NTV2_SIGNALMASK_NONE:          // Output Black
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}
		break;
	case NTV2_SIGNALMASK_Y:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break;
	case NTV2_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}

		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break; 

	case NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
		}


		break; 
	case NTV2_SIGNALMASK_Cb + NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrUnPackedBuffer[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrUnPackedBuffer[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break; 
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cb + NTV2_SIGNALMASK_Cr:
		// Do nothing
		break; 
	}

}



//--------------------------------------------------------------------------------------------------------------------
//	StackQuadrants()
//
//	Take a 4K source, cut it into 4 quandrants and stack it into the destination. Also handle cases where
//	where source/destination rowBytes/widths are mismatched (eg 4096 -> 3840)
//--------------------------------------------------------------------------------------------------------------------
void StackQuadrants(uint8_t* pSrc, uint32_t srcWidth, uint32_t srcHeight, uint32_t srcRowBytes, 
					 uint8_t* pDst)
{
	(void) srcWidth;
	uint32_t dstSample;
	uint32_t srcSample;
	uint32_t copyRowBytes = srcRowBytes/2;
	uint32_t copyHeight = srcHeight/2;
	uint32_t dstRowBytes = copyRowBytes;
	uint32_t dstHeight = srcHeight/2;
	//uint32_t dstWidth = srcWidth/2;

	// rowbytes for left hand side quadrant
	uint32_t srcLHSQuadrantRowBytes = srcRowBytes/2;

	for (uint32_t quadrant=0; quadrant<4; quadrant++)
	{
		// starting point for source quadrant
		switch (quadrant)
		{
		default:
		case 0: srcSample = 0; break;													// quadrant 0, upper left
		case 1: srcSample = srcLHSQuadrantRowBytes; break;								// quadrant 1, upper right
		case 2: srcSample = (srcRowBytes*copyHeight); break;							// quadrant 2, lower left
		case 3: srcSample = (srcRowBytes*copyHeight) + srcLHSQuadrantRowBytes; break;	// quadrant 3, lower right
		}

		// starting point for destination stack
		dstSample = quadrant * dstRowBytes * dstHeight;

		for (uint32_t row=0; row<copyHeight; row++)
		{
			memcpy(&pDst[dstSample], &pSrc[srcSample], copyRowBytes);
			dstSample += dstRowBytes;
			srcSample += srcRowBytes;
		}
	}
}

// Copy a quater-sized quadrant from a source buffer to a destination buffer
// quad13Offset is almost always zero, but can be used for Quadrants 1, 3 for special offset frame buffers. (e.g. 4096x1080 10Bit YCbCr frame buffers)
void CopyFromQuadrant(uint8_t* srcBuffer, uint32_t srcHeight, uint32_t srcRowBytes, uint32_t srcQuadrant, uint8_t* dstBuffer, uint32_t quad13Offset)
{
	ULWord dstSample = 0;
	ULWord srcSample = 0;
	ULWord dstHeight = srcHeight / 2;
	ULWord dstRowBytes = srcRowBytes / 2;

	// calculate starting point for source of copy, based on source quadrant
	switch (srcQuadrant)
	{
	default:
	case 0: srcSample = 0; break;													// quadrant 0, upper left
	case 1: srcSample = dstRowBytes - quad13Offset; break;							// quadrant 1, upper right
	case 2: srcSample = srcRowBytes*dstHeight; break;								// quadrant 2, lower left
	case 3: srcSample = srcRowBytes*dstHeight + dstRowBytes - quad13Offset; break;	// quadrant 3, lower right
	}

	// for each row
	for (ULWord i=0; i<dstHeight; i++)
	{
		memcpy(&dstBuffer[dstSample], &srcBuffer[srcSample], dstRowBytes);
		dstSample += dstRowBytes;
		srcSample += srcRowBytes;
	}
}

// Copy a source buffer to a quadrant of a 4x-sized destination buffer
// quad13Offset is almost always zero, but can be used for Quadrants 1, 3 for special offset frame buffers. (e.g. 4096x1080 10Bit YCbCr frame buffers)
void CopyToQuadrant(uint8_t* srcBuffer, uint32_t srcHeight, uint32_t srcRowBytes, uint32_t dstQuadrant, uint8_t* dstBuffer, uint32_t quad13Offset)
{
	ULWord dstSample = 0;
	ULWord srcSample = 0;
	ULWord dstRowBytes = srcRowBytes * 2;

	// calculate starting point for destination of copy, based on destination quadrant
	switch (dstQuadrant)
	{
	default:
	case 0: dstSample = 0; break;													// quadrant 0, upper left
	case 1: dstSample = srcRowBytes - quad13Offset; break;							// quadrant 1, upper right
	case 2: dstSample = dstRowBytes*srcHeight; break;								// quadrant 2, lower left
	case 3: dstSample = dstRowBytes*srcHeight + srcRowBytes - quad13Offset; break;	// quadrant 3, lower right
	}

	// for each row
	for (ULWord i=0; i<srcHeight; i++)
	{
		memcpy(&dstBuffer[dstSample], &srcBuffer[srcSample], srcRowBytes);
		dstSample += dstRowBytes;
		srcSample += srcRowBytes;
	}
}
//////////////////////////////////////////////////////
//	END SECTION MOVED FROM 'videoutilities.cpp'
//////////////////////////////////////////////////////


void UnpackLine_10BitYUVto16BitYUV (const ULWord * pIn10BitYUVLine, UWord * pOut16BitYUVLine, const ULWord inNumPixels)
#if !defined (NTV2_DEPRECATE)
{
	::UnPackLineData (pIn10BitYUVLine, pOut16BitYUVLine, inNumPixels);
}

void UnPackLineData (const ULWord * pIn10BitYUVLine, UWord * pOut16BitYUVLine, const ULWord inNumPixels)
#endif	//	!defined (NTV2_DEPRECATE)
{
	NTV2_ASSERT (pIn10BitYUVLine && pOut16BitYUVLine && "UnpackLine_10BitYUVto16BitYUV -- NULL buffer pointer(s)");
	NTV2_ASSERT (inNumPixels && "UnpackLine_10BitYUVto16BitYUV -- Zero pixel count");

	for (ULWord outputCount = 0,  inputCount = 0;
		 outputCount < (inNumPixels * 2);
		 outputCount += 3,  inputCount++)
	{
		pOut16BitYUVLine [outputCount    ] =  pIn10BitYUVLine [inputCount]        & 0x3FF;
		pOut16BitYUVLine [outputCount + 1] = (pIn10BitYUVLine [inputCount] >> 10) & 0x3FF;
		pOut16BitYUVLine [outputCount + 2] = (pIn10BitYUVLine [inputCount] >> 20) & 0x3FF;
	}
}


void PackLine_16BitYUVto10BitYUV (const UWord * pIn16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels)
#if !defined (NTV2_DEPRECATE)
{
	::PackLineData (pIn16BitYUVLine, pOut10BitYUVLine, inNumPixels);
}

void PackLineData (const UWord * pIn16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels)
#endif	//	!defined (NTV2_DEPRECATE)
{
	NTV2_ASSERT (pIn16BitYUVLine && pOut10BitYUVLine && "PackLine_16BitYUVto10BitYUV -- NULL buffer pointer(s)");
	NTV2_ASSERT (inNumPixels && "PackLine_16BitYUVto10BitYUV -- Zero pixel count");

	for (ULWord inputCount = 0,  outputCount = 0;
		  inputCount < (inNumPixels * 2);
		  outputCount += 4,  inputCount += 12)
	{
		pOut10BitYUVLine [outputCount    ] = ULWord (pIn16BitYUVLine [inputCount + 0]) + (ULWord (pIn16BitYUVLine [inputCount + 1]) << 10) + (ULWord (pIn16BitYUVLine [inputCount + 2]) << 20);
		pOut10BitYUVLine [outputCount + 1] = ULWord (pIn16BitYUVLine [inputCount + 3]) + (ULWord (pIn16BitYUVLine [inputCount + 4]) << 10) + (ULWord (pIn16BitYUVLine [inputCount + 5]) << 20);
		pOut10BitYUVLine [outputCount + 2] = ULWord (pIn16BitYUVLine [inputCount + 6]) + (ULWord (pIn16BitYUVLine [inputCount + 7]) << 10) + (ULWord (pIn16BitYUVLine [inputCount + 8]) << 20);
		pOut10BitYUVLine [outputCount + 3] = ULWord (pIn16BitYUVLine [inputCount + 9]) + (ULWord (pIn16BitYUVLine [inputCount +10]) << 10) + (ULWord (pIn16BitYUVLine [inputCount +11]) << 20);
	}	//	for each component in the line
}


bool PackLine_UWordSequenceTo10BitYUV (const UWordSequence & in16BitYUVLine, ULWord * pOut10BitYUVLine, const ULWord inNumPixels)
{
	if (!pOut10BitYUVLine)
		return false;	//	NULL buffer pointer
	if (!inNumPixels)
		return false;	//	Zero pixel count
	if (ULWord(in16BitYUVLine.size()) < inNumPixels*2)
		return false;	//	UWordSequence too small

	for (ULWord inputCount = 0,  outputCount = 0;
		  inputCount < (inNumPixels * 2);
		  outputCount += 4,  inputCount += 12)
	{
		pOut10BitYUVLine[outputCount    ] = ULWord(in16BitYUVLine[inputCount + 0]) + (ULWord(in16BitYUVLine[inputCount + 1]) << 10) + (ULWord(in16BitYUVLine[inputCount + 2]) << 20);
		pOut10BitYUVLine[outputCount + 1] = ULWord(in16BitYUVLine[inputCount + 3]) + (ULWord(in16BitYUVLine[inputCount + 4]) << 10) + (ULWord(in16BitYUVLine[inputCount + 5]) << 20);
		pOut10BitYUVLine[outputCount + 2] = ULWord(in16BitYUVLine[inputCount + 6]) + (ULWord(in16BitYUVLine[inputCount + 7]) << 10) + (ULWord(in16BitYUVLine[inputCount + 8]) << 20);
		pOut10BitYUVLine[outputCount + 3] = ULWord(in16BitYUVLine[inputCount + 9]) + (ULWord(in16BitYUVLine[inputCount +10]) << 10) + (ULWord(in16BitYUVLine[inputCount +11]) << 20);
	}	//	for each component in the line
	return true;
}


bool YUVComponentsTo10BitYUVPackedBuffer (const vector<uint16_t> & inYCbCrLine,  NTV2_POINTER & inFrameBuffer,
											const NTV2FormatDescriptor & inDescriptor,  const UWord inLineOffset)
{
	if (inYCbCrLine.size() < 12)
		return false;	//	Input vector needs at least 12 components
	if (inFrameBuffer.IsNULL())
		return false;	//	NULL frame buffer
	if (!inDescriptor.IsValid())
		return false;	//	Bad format descriptor
	if (ULWord(inLineOffset) >= inDescriptor.GetFullRasterHeight())
		return false;	//	Illegal line offset
	if (inDescriptor.GetPixelFormat() != NTV2_FBF_10BIT_YCBCR)
		return false;	//	Not 'v210' pixel format

	const uint32_t	pixPerLineX2	(inDescriptor.GetRasterWidth() * 2);
	uint32_t *		pOutPackedLine	(NULL);
	if (inFrameBuffer.GetByteCount() < inDescriptor.GetBytesPerRow() * ULWord(inLineOffset+1))
		return false;	//	Buffer too small

	pOutPackedLine = (uint32_t*) inDescriptor.GetRowAddress(inFrameBuffer.GetHostAddress(0), inLineOffset);
	if (pOutPackedLine == NULL)
		return false;	//	Buffer too small

	for (uint32_t inputCount = 0, outputCount = 0;   inputCount < pixPerLineX2;   outputCount += 4, inputCount += 12)
	{
		if ((inputCount+11) >= uint32_t(inYCbCrLine.size()))
			break;	//	Early exit (not fatal)
	#if defined(_DEBUG)	//	'at' throws upon bad index values
		pOutPackedLine[outputCount]   = uint32_t(inYCbCrLine.at(inputCount+0)) | uint32_t(inYCbCrLine.at(inputCount+ 1)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 2)<<20);
		pOutPackedLine[outputCount+1] = uint32_t(inYCbCrLine.at(inputCount+3)) | uint32_t(inYCbCrLine.at(inputCount+ 4)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 5)<<20);
		pOutPackedLine[outputCount+2] = uint32_t(inYCbCrLine.at(inputCount+6)) | uint32_t(inYCbCrLine.at(inputCount+ 7)<<10) | uint32_t(inYCbCrLine.at(inputCount+ 8)<<20);
		pOutPackedLine[outputCount+3] = uint32_t(inYCbCrLine.at(inputCount+9)) | uint32_t(inYCbCrLine.at(inputCount+10)<<10) | uint32_t(inYCbCrLine.at(inputCount+11)<<20);
	#else				//	'operator[]' doesn't throw
		pOutPackedLine[outputCount]   = uint32_t(inYCbCrLine[inputCount+0]) | uint32_t(inYCbCrLine[inputCount+ 1]<<10) | uint32_t(inYCbCrLine[inputCount+ 2]<<20);
		pOutPackedLine[outputCount+1] = uint32_t(inYCbCrLine[inputCount+3]) | uint32_t(inYCbCrLine[inputCount+ 4]<<10) | uint32_t(inYCbCrLine[inputCount+ 5]<<20);
		pOutPackedLine[outputCount+2] = uint32_t(inYCbCrLine[inputCount+6]) | uint32_t(inYCbCrLine[inputCount+ 7]<<10) | uint32_t(inYCbCrLine[inputCount+ 8]<<20);
		pOutPackedLine[outputCount+3] = uint32_t(inYCbCrLine[inputCount+9]) | uint32_t(inYCbCrLine[inputCount+10]<<10) | uint32_t(inYCbCrLine[inputCount+11]<<20);
	#endif
	}
	return true;
}


bool UnpackLine_10BitYUVtoU16s (vector<uint16_t> & outYCbCrLine, const NTV2_POINTER & inFrameBuffer,
								const NTV2FormatDescriptor & inDescriptor, const UWord inLineOffset)
{
	outYCbCrLine.clear();
	if (inFrameBuffer.IsNULL())
		return false;	//	NULL frame buffer
	if (!inDescriptor.IsValid())
		return false;	//	Bad format descriptor
	if (ULWord(inLineOffset) >= inDescriptor.GetFullRasterHeight())
		return false;	//	Illegal line offset
	if (inDescriptor.GetPixelFormat() != NTV2_FBF_10BIT_YCBCR)
		return false;	//	Not 'v210' pixel format
	if (inDescriptor.GetRasterWidth () < 6)
		return false;	//	bad width

	const ULWord *	pInputLine	(reinterpret_cast<const ULWord*>(inDescriptor.GetRowAddress(inFrameBuffer.GetHostPointer(), inLineOffset)));

	for (ULWord inputCount(0);  inputCount < inDescriptor.linePitch;  inputCount++)
	{
		outYCbCrLine.push_back((pInputLine[inputCount]      ) & 0x3FF);
		outYCbCrLine.push_back((pInputLine[inputCount] >> 10) & 0x3FF);
		outYCbCrLine.push_back((pInputLine[inputCount] >> 20) & 0x3FF);
	}
	return true;
}


// RePackLineDataForYCbCrDPX
void RePackLineDataForYCbCrDPX(ULWord *packedycbcrLine, ULWord numULWords )
{
	for ( UWord count = 0; count < numULWords; count++)
	{
		ULWord value = (packedycbcrLine[count])<<2;
		value = (value<<24) + ((value>>24)&0x000000FF) + ((value<<8)&0x00FF0000) + ((value>>8)&0x0000FF00);

		packedycbcrLine[count] = value;
	}
}
// UnPack 10 Bit DPX Format linebuffer to RGBAlpha10BitPixel linebuffer.
void UnPack10BitDPXtoRGBAlpha10BitPixel(RGBAlpha10BitPixel* rgba10BitBuffer,ULWord* DPXLinebuffer ,ULWord numPixels, bool bigEndian)
{
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		ULWord value = DPXLinebuffer[pixel];
		if ( bigEndian)
		{
			rgba10BitBuffer[pixel].Red = ((value&0xC0)>>14) + ((value&0xFF)<<2);
			rgba10BitBuffer[pixel].Green = ((value&0x3F00)>>4) + ((value&0xF00000)>>20);
			rgba10BitBuffer[pixel].Blue = ((value&0xFC000000)>>26) + ((value&0xF0000)>>12);
		}
		else
		{
			rgba10BitBuffer[pixel].Red = (value>>22)&0x3FF;
			rgba10BitBuffer[pixel].Green = (value>>12)&0x3FF;
			rgba10BitBuffer[pixel].Blue = (value>>2)&0x3FF;

		}
	}
}

void UnPack10BitDPXtoForRP215withEndianSwap(UWord* rawrp215Buffer,ULWord* DPXLinebuffer ,ULWord numPixels)
{
	// gets the green component.
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		ULWord value = DPXLinebuffer[pixel];
		rawrp215Buffer[pixel] = ((value&0x3F00)>>4) + ((value&0xF00000)>>20);
	}
}

void UnPack10BitDPXtoForRP215(UWord* rawrp215Buffer,ULWord* DPXLinebuffer ,ULWord numPixels)
{
	// gets the green component.
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		ULWord value = DPXLinebuffer[pixel];
		rawrp215Buffer[pixel] = ((value&0x3F)>>4) + ((value&0xF00000)>>20);
	}
}

// MaskYCbCrLine
// Mask Data In place based on signalMask
void MaskYCbCrLine(UWord* ycbcrLine, UWord signalMask , ULWord numPixels)
{
	ULWord pixelCount;

	// Not elegant but fairly fast.
	switch ( signalMask )
	{
	case NTV2_SIGNALMASK_NONE:          // Output Black
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrLine[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrLine[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrLine[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}
		break;
	case NTV2_SIGNALMASK_Y:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrLine[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break;
	case NTV2_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrLine[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
			ycbcrLine[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}

		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cb:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount+2] = CCIR601_10BIT_CHROMAOFFSET;     // Cr
		}

		break;

	case NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
			ycbcrLine[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrLine[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount]   = CCIR601_10BIT_CHROMAOFFSET;     // Cb
		}


		break;
	case NTV2_SIGNALMASK_Cb + NTV2_SIGNALMASK_Cr:
		for ( pixelCount = 0; pixelCount < (numPixels*2); pixelCount += 4 )
		{
			ycbcrLine[pixelCount+1] = CCIR601_10BIT_BLACK;            // Y
			ycbcrLine[pixelCount+3] = CCIR601_10BIT_BLACK;            // Y
		}


		break;
	case NTV2_SIGNALMASK_Y + NTV2_SIGNALMASK_Cb + NTV2_SIGNALMASK_Cr:
		// Do nothing
		break;
	}

}

void Make10BitBlackLine (UWord * pOutLineData, const UWord inNumPixels)
{
	// Assume 1080 format
	for (UWord count(0);  count < inNumPixels*2;  count+=2)
	{
		pOutLineData[count]   = UWord(CCIR601_10BIT_CHROMAOFFSET);
		pOutLineData[count+1] = UWord(CCIR601_10BIT_BLACK);
	}
}

void Make10BitWhiteLine(UWord* lineData,UWord numPixels)
{
	// assumes lineData is large enough for numPixels
	for ( int count = 0; count < numPixels*2; count+=2 )
	{
		lineData[count] = (UWord)CCIR601_10BIT_CHROMAOFFSET;
		lineData[count+1] = (UWord)CCIR601_10BIT_WHITE;
	}
}

void Make10BitLine(UWord* lineData, UWord Y , UWord Cb , UWord Cr,UWord numPixels)
{
	// assumes lineData is large enough for numPixels
	for ( int count = 0; count < numPixels*2; count+=4 )
	{
		lineData[count] = Cb;
		lineData[count+1] = Y;
		lineData[count+2] = Cr;
		lineData[count+3] = Y;
	}
}

#if !defined(NTV2_DEPRECATE_13_0)
	void Fill10BitYCbCrVideoFrame(PULWord _baseVideoAddress,
								 const NTV2Standard standard,
								 const NTV2FrameBufferFormat frameBufferFormat,
								 const YCbCr10BitPixel color,
								 const bool vancEnabled,
								 const bool twoKby1080,
								 const bool wideVANC)
	{
		NTV2FormatDescriptor fd (standard,frameBufferFormat,vancEnabled,twoKby1080,wideVANC);
		UWord lineBuffer[2048*2];
		Make10BitLine(lineBuffer,color.y,color.cb,color.cr,fd.numPixels);
		for ( UWord i= 0; i<fd.numLines; i++)
		{
			::PackLine_16BitYUVto10BitYUV(lineBuffer,_baseVideoAddress,fd.numPixels);
			_baseVideoAddress += fd.linePitch;
		}
	}
#endif	//	!defined(NTV2_DEPRECATE_13_0)

bool Fill10BitYCbCrVideoFrame (void * pBaseVideoAddress,
								const NTV2Standard inStandard,
								const NTV2FrameBufferFormat inFBF,
								const YCbCr10BitPixel inPixelColor,
								const NTV2VANCMode inVancMode)
{
	if (!pBaseVideoAddress)
		return false;

	const NTV2FormatDescriptor fd (inStandard, inFBF, inVancMode);
	UWord		lineBuffer[2048*2];
	ULWord *	pBaseAddress	(reinterpret_cast<ULWord*>(pBaseVideoAddress));
	Make10BitLine (lineBuffer, inPixelColor.y, inPixelColor.cb, inPixelColor.cr, fd.numPixels);

	for (UWord lineNdx(0);  lineNdx < fd.numLines;  lineNdx++)
	{
		::PackLine_16BitYUVto10BitYUV (lineBuffer, pBaseAddress, fd.numPixels);
		pBaseAddress += fd.linePitch;
	}
	return true;
}


void Make8BitBlackLine(UByte* lineData,UWord numPixels,NTV2FrameBufferFormat fbFormat)
{
	// assumes lineData is large enough for numPixels
	if ( fbFormat == NTV2_FBF_8BIT_YCBCR )
	{
		for ( int count = 0; count < numPixels*2; count+=2 )
		{
			lineData[count] = (UWord)CCIR601_8BIT_CHROMAOFFSET;
			lineData[count+1] = (UWord)CCIR601_8BIT_BLACK;
		}
	}
	else
	{
		// NTV2_FBF_8BIT_YCBCR_YUY2
		for ( int count = 0; count < numPixels*2; count+=2 )
		{
			lineData[count] = (UWord)CCIR601_8BIT_BLACK;
			lineData[count+1] = (UWord)CCIR601_8BIT_CHROMAOFFSET;
		}
	}
}

void Make8BitWhiteLine(UByte* lineData,UWord numPixels,NTV2FrameBufferFormat fbFormat)
{
	// assumes lineData is large enough for numPixels
	// assumes lineData is large enough for numPixels
	if ( fbFormat == NTV2_FBF_8BIT_YCBCR )
	{
		for ( int count = 0; count < numPixels*2; count+=2 )
		{
			lineData[count] = (UWord)CCIR601_8BIT_CHROMAOFFSET;
			lineData[count+1] = (UWord)CCIR601_8BIT_WHITE;
		}
	}
	else
	{
		// NTV2_FBF_8BIT_YCBCR_YUY2
		for ( int count = 0; count < numPixels*2; count+=2 )
		{
			lineData[count] = (UWord)CCIR601_8BIT_WHITE;
			lineData[count+1] = (UWord)CCIR601_8BIT_CHROMAOFFSET;
		}
	}

}

void Make8BitLine(UByte* lineData, UByte Y , UByte Cb , UByte Cr,ULWord numPixels,NTV2FrameBufferFormat fbFormat)
{
	// assumes lineData is large enough for numPixels

	if ( fbFormat == NTV2_FBF_8BIT_YCBCR )
	{
		for ( ULWord count = 0; count < numPixels*2; count+=4 )
		{
			lineData[count] = Cb;
			lineData[count+1] = Y;
			lineData[count+2] = Cr;
			lineData[count+3] = Y;
		}
	}
	else
	{
		for ( ULWord count = 0; count < numPixels*2; count+=4 )
		{
			lineData[count] = Y;
			lineData[count+1] = Cb;
			lineData[count+2] = Y;
			lineData[count+3] = Cr;
		}

	}
}

#if !defined(NTV2_DEPRECATE_13_0)
	void Fill8BitYCbCrVideoFrame(PULWord _baseVideoAddress,
								 NTV2Standard standard,
								 NTV2FrameBufferFormat frameBufferFormat,
								 YCbCrPixel color,
								 bool vancEnabled,
								 bool twoKby1080,
								 bool wideVANC)
	{
		NTV2FormatDescriptor fd (standard,frameBufferFormat,vancEnabled,twoKby1080,wideVANC);
	
		for ( UWord i= 0; i<fd.numLines; i++)
		{
			Make8BitLine((UByte*)_baseVideoAddress,color.y,color.cb,color.cr,fd.numPixels,frameBufferFormat);
			_baseVideoAddress += fd.linePitch;
		}
	}
#endif	//	!defined(NTV2_DEPRECATE_13_0)

bool Fill8BitYCbCrVideoFrame (void * pBaseVideoAddress,  const NTV2Standard inStandard,  const NTV2FrameBufferFormat inFBF,
								const YCbCrPixel inPixelColor,  const NTV2VANCMode inVancMode)
{
	if (!pBaseVideoAddress)
		return false;

	const NTV2FormatDescriptor fd (inStandard, inFBF, inVancMode);
	UByte *		pBaseAddress	(reinterpret_cast<UByte*>(pBaseVideoAddress));

	for (UWord lineNdx(0);  lineNdx < fd.numLines;  lineNdx++)
	{
		Make8BitLine (pBaseAddress, inPixelColor.y, inPixelColor.cb, inPixelColor.cr, fd.numPixels, inFBF);
		pBaseAddress += fd.GetBytesPerRow();
	}
	return true;
}

void Fill4k8BitYCbCrVideoFrame(PULWord _baseVideoAddress,
							 NTV2FrameBufferFormat frameBufferFormat,
							 YCbCrPixel color,
							 bool vancEnabled,
							 bool b4k,
							 bool wideVANC)
{
	(void) vancEnabled;
	(void) wideVANC;
	NTV2FormatDescriptor fd;
	if(b4k)
	{
		fd.numLines = 2160;
		fd.numPixels = 4096;
		fd.firstActiveLine = 0;
		fd.linePitch = 4096*2/4;
	}
	else
	{
		fd.numLines = 2160;
		fd.numPixels = 3840;
		fd.firstActiveLine = 0;
		fd.linePitch = 3840*2/4;
	}

	Make8BitLine((UByte*)_baseVideoAddress,color.y,color.cb,color.cr,fd.numPixels*fd.numLines,frameBufferFormat);
}


// Copy arbrary-sized source image buffer to arbitrary-sized destination frame buffer.
// It will automatically clip and/or pad the source image to center it in the destination frame.
// This will work with any RGBA/RGB frame buffer formats with 4 Bytes/pixel size
void CopyRGBAImageToFrame(ULWord* pSrcBuffer, ULWord srcWidth, ULWord srcHeight,
						  ULWord* pDstBuffer, ULWord dstWidth, ULWord dstHeight)
{
	// all variables are in pixels
	ULWord topPad = 0, bottomPad = 0, leftPad = 0, rightPad = 0;
	ULWord contentHeight = 0;
	ULWord contentWidth = 0;
	ULWord* pSrc = pSrcBuffer;
	ULWord* pDst = pDstBuffer;

	if (dstHeight > srcHeight)
	{
		topPad = (dstHeight - srcHeight) / 2;
		bottomPad = dstHeight - topPad - srcHeight;
	}
	else
		pSrc += ((srcHeight - dstHeight) / 2) * srcWidth;

	if (dstWidth > srcWidth)
	{
		leftPad = (dstWidth - srcWidth) / 2;
		rightPad = dstWidth - srcWidth - leftPad;
	}
	else
		pSrc += (srcWidth - dstWidth) / 2;

	// content
	contentHeight = dstHeight - topPad - bottomPad;
	contentWidth = dstWidth - leftPad - rightPad;

	// top pad
	memset(pDst, 0, topPad * dstWidth * 4);
	pDst += topPad * dstWidth;

	// content
	while (contentHeight--)
	{
		// left
		memset(pDst, 0, leftPad * 4);
		pDst += leftPad;

		// content
		memcpy(pDst, pSrc, contentWidth * 4);
		pDst += contentWidth;
		pSrc += srcWidth;

		// right
		memset(pDst, 0, rightPad * 4);
		pDst += rightPad;
	}

	// bottom pad
	memset(pDst, 0, bottomPad * dstWidth * 4);
}


static bool SetRasterLinesBlack8BitYCbCr (UByte *			pDstBuffer,
											const UWord		inDstBytesPerLine,
											const UWord		inDstTotalLines)
{
	const UWord	dstMaxPixelWidth	(inDstBytesPerLine / 2);	//	2 bytes per pixel for '2vuy'
	UByte *		pLine				(pDstBuffer);
	for (UWord lineNum (0);  lineNum < inDstTotalLines;  lineNum++)
	{
		::Make8BitBlackLine (pLine, dstMaxPixelWidth);
		pLine += inDstBytesPerLine;
	}
	return true;
}


static bool SetRasterLinesBlack10BitYCbCr (UByte *			pDstBuffer,
											const UWord		inDstBytesPerLine,
											const UWord		inDstTotalLines)
{
	const UWord	dstMaxPixelWidth	(inDstBytesPerLine / 16 * 6);
	UByte *		pLine				(pDstBuffer);
	for (UWord lineNum (0);  lineNum < inDstTotalLines;  lineNum++)
	{
		UWord *		pDstUWord	(reinterpret_cast <UWord *> (pLine));
		ULWord *	pDstULWord	(reinterpret_cast <ULWord *> (pLine));
		::Make10BitBlackLine (pDstUWord, dstMaxPixelWidth);
		::PackLine_16BitYUVto10BitYUV (pDstUWord, pDstULWord, dstMaxPixelWidth);
		pLine += inDstBytesPerLine;
	}
	return true;
}


static bool SetRasterLinesBlack8BitRGBs (const NTV2FrameBufferFormat	inPixelFormat,
											UByte *						pDstBuffer,
											const UWord					inDstBytesPerLine,
											const UWord					inDstTotalLines,
											const bool					inIsSD	= false)
{
	UByte *			pLine				(pDstBuffer);
	const UWord		dstMaxPixelWidth	(inDstBytesPerLine / 4);	//	4 bytes per pixel
	YCbCrAlphaPixel	YCbCr = {0 /*Alpha*/,	44/*cr*/,	10/*Y*/,	44/*cb*/};
	RGBAlphaPixel	rgb;
	if (inIsSD)
		::SDConvertYCbCrtoRGBSmpte (&YCbCr, &rgb);
	else
		::HDConvertYCbCrtoRGBSmpte (&YCbCr, &rgb);

	//	Set the first line...
	for (UWord pixNum (0);  pixNum < dstMaxPixelWidth;  pixNum++)
	{
		switch (inPixelFormat)
		{
			case NTV2_FBF_ARGB:	pLine [0] = rgb.Alpha;	pLine [0] = rgb.Red;	pLine [0] = rgb.Green;	pLine [0] = rgb.Blue;	break;
			case NTV2_FBF_RGBA:	pLine [0] = rgb.Red;	pLine [0] = rgb.Green;	pLine [0] = rgb.Blue;	pLine [0] = rgb.Alpha;	break;
			case NTV2_FBF_ABGR:	pLine [0] = rgb.Alpha;	pLine [0] = rgb.Blue;	pLine [0] = rgb.Green;	pLine [0] = rgb.Red;	break;
			default:			return false;
		}
		pLine += 4;		//	4 bytes per pixel
	}

	//	Set the rest...
	pLine = pDstBuffer + inDstBytesPerLine;
	for (UWord lineNum (1);  lineNum < inDstTotalLines;  lineNum++)
	{
		::memcpy (pLine, pDstBuffer, inDstBytesPerLine);
		pLine += inDstBytesPerLine;
	}
	return true;
}


static bool SetRasterLinesBlack10BitRGB (UByte *			pDstBuffer,
											const UWord		inDstBytesPerLine,
											const UWord		inDstTotalLines,
											const bool		inIsSD	= false)
{
	UByte *			pLine				(pDstBuffer + inDstBytesPerLine);
	ULWord *		pPixels				(reinterpret_cast <ULWord *> (pDstBuffer));
	const UWord		dstMaxPixelWidth	(inDstBytesPerLine / 4);	//	4 bytes per pixel
	ULWord			blackOpaque			(0xC0400004);	(void) inIsSD;	//	For now

	//	Set the first line...
	for (UWord pixNum (0);  pixNum < dstMaxPixelWidth;  pixNum++)
		pPixels [pixNum] = blackOpaque;

	//	Set the rest...
	for (UWord lineNum (1);  lineNum < inDstTotalLines;  lineNum++)
	{
		::memcpy (pLine, pDstBuffer, inDstBytesPerLine);
		pLine += inDstBytesPerLine;
	}
	return true;
}


bool SetRasterLinesBlack (const NTV2FrameBufferFormat	inPixelFormat,
							UByte *						pDstBuffer,
							const UWord					inDstBytesPerLine,
							const UWord					inDstTotalLines)
{
	if (!pDstBuffer)					//	NULL buffer
		return false;
	if (inDstBytesPerLine == 0)			//	zero rowbytes
		return false;
	if (inDstTotalLines == 0)			//	zero height
		return false;

	switch (inPixelFormat)
	{
		case NTV2_FBF_10BIT_YCBCR:		return SetRasterLinesBlack10BitYCbCr (pDstBuffer, inDstBytesPerLine, inDstTotalLines);

		case NTV2_FBF_8BIT_YCBCR:		return SetRasterLinesBlack8BitYCbCr (pDstBuffer, inDstBytesPerLine, inDstTotalLines);

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:				return SetRasterLinesBlack8BitRGBs (inPixelFormat, pDstBuffer, inDstBytesPerLine, inDstTotalLines);

		case NTV2_FBF_10BIT_RGB:		return SetRasterLinesBlack10BitRGB (pDstBuffer, inDstBytesPerLine, inDstTotalLines);

		case NTV2_FBF_8BIT_YCBCR_YUY2:
		case NTV2_FBF_10BIT_DPX:
		case NTV2_FBF_10BIT_YCBCR_DPX:
		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_YCBCR_420PL3:
		case NTV2_FBF_8BIT_HDV:
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_10BIT_YCBCRA:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_48BIT_RGB:
		case NTV2_FBF_PRORES:
		case NTV2_FBF_PRORES_DVCPRO:
		case NTV2_FBF_PRORES_HDV:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
		case NTV2_FBF_8BIT_YCBCR_422PL3:
		case NTV2_FBF_10BIT_RAW_RGB:
		case NTV2_FBF_10BIT_RAW_YCBCR:
		case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
        case NTV2_FBF_10BIT_YCBCR_420PL2:
        case NTV2_FBF_10BIT_YCBCR_422PL2:
        case NTV2_FBF_8BIT_YCBCR_420PL2:
        case NTV2_FBF_8BIT_YCBCR_422PL2:
		case NTV2_FBF_NUMFRAMEBUFFERFORMATS:
			return false;
	}
	return false;

}	//	SetRasterLinesBlack


static const UByte * GetReadAddress_2vuy (const UByte * pInFrameBuffer, const UWord inBytesPerVertLine, const UWord inVertLineOffset, const UWord inHorzPixelOffset, const UWord inBytesPerHorzPixel)
{
	const UByte *	pResult	(pInFrameBuffer);
	NTV2_ASSERT (inBytesPerVertLine);
	NTV2_ASSERT ((inHorzPixelOffset & 1) == 0);	//	For '2vuy', horizontal pixel offset must be even!!
	pResult += inBytesPerVertLine * inVertLineOffset;
	pResult += inBytesPerHorzPixel * inHorzPixelOffset;
	return pResult;
}


static UByte * GetWriteAddress_2vuy (UByte * pInFrameBuffer, const UWord inBytesPerVertLine, const UWord inVertLineOffset, const UWord inHorzPixelOffset, const UWord inBytesPerHorzPixel)
{
	UByte *	pResult	(pInFrameBuffer);
	NTV2_ASSERT (inBytesPerVertLine);
	NTV2_ASSERT ((inHorzPixelOffset & 1) == 0);	//	For '2vuy', horizontal pixel offset must be even!!
	pResult += inBytesPerVertLine * inVertLineOffset;
	pResult += inBytesPerHorzPixel * inHorzPixelOffset;
	return pResult;
}


//	This function should work on all 4-byte-per-2-pixel formats
static bool CopyRaster4BytesPer2Pixels (UByte *			pDstBuffer,				//	Dest buffer to be modified
										const UWord		inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
										const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
										const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
										const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
										const UByte *	pSrcBuffer,				//	Src buffer
										const UWord		inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
										const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
										const UWord		inSrcVertLineOffset,	//	Src image top edge
										const UWord		inSrcVertLinesToCopy,	//	Src image height
										const UWord		inSrcHorzPixelOffset,	//	Src image left edge
										const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	if (inDstHorzPixelOffset & 1)					//	dst odd pixel offset
		return false;
	if (inSrcHorzPixelOffset & 1)					//	src odd pixel offset
		return false;

	const UWord	TWO_BYTES_PER_PIXEL	(2);			//	2 bytes per pixel for '2vuy'
	const UWord	dstMaxPixelWidth	(inDstBytesPerLine / TWO_BYTES_PER_PIXEL);
	const UWord	srcMaxPixelWidth	(inSrcBytesPerLine / TWO_BYTES_PER_PIXEL);
	UWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > srcMaxPixelWidth)
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	const UByte *	pSrc	(::GetReadAddress_2vuy (pSrcBuffer, inSrcBytesPerLine, inSrcVertLineOffset, inSrcHorzPixelOffset, TWO_BYTES_PER_PIXEL));
	UByte *			pDst	(::GetWriteAddress_2vuy (pDstBuffer, inDstBytesPerLine, inDstVertLineOffset, inDstHorzPixelOffset, TWO_BYTES_PER_PIXEL));

	for (UWord srcLinesToCopy (numVertLinesToCopy);  srcLinesToCopy > 0;  srcLinesToCopy--)	//	for each src raster line
	{
		UWord			dstPixelsCopied	(0);
		const UByte *	pSavedSrc		(pSrc);
		UByte *			pSavedDst		(pDst);
		for (UWord hPixelsToCopy (numHorzPixelsToCopy);  hPixelsToCopy > 0;  hPixelsToCopy--)	//	for each pixel/column
		{
			pDst[0] = pSrc[0];
			pDst[1] = pSrc[1];
			dstPixelsCopied++;
			if (dstPixelsCopied + inDstHorzPixelOffset >= dstMaxPixelWidth)
				break;	//	Clip to dst raster's right edge
			pDst += TWO_BYTES_PER_PIXEL;
			pSrc += TWO_BYTES_PER_PIXEL;
		}
		pSrc = pSavedSrc;
		pDst = pSavedDst;
		pSrc += inSrcBytesPerLine;
		pDst += inDstBytesPerLine;
	}	//	for each src line to copy
	return true;

}	//	CopyRaster4BytesPer2Pixels


//	This function should work on all 16-byte-per-6-pixel formats
static bool CopyRaster16BytesPer6Pixels (	UByte *			pDstBuffer,				//	Dest buffer to be modified
											const UWord		inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width) -- must be evenly divisible by 16
											const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
											const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
											const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear -- must be evenly divisible by 6
											const UByte *	pSrcBuffer,				//	Src buffer
											const UWord		inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width) -- must be evenly divisible by 16
											const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
											const UWord		inSrcVertLineOffset,	//	Src image top edge
											const UWord		inSrcVertLinesToCopy,	//	Src image height
											const UWord		inSrcHorzPixelOffset,	//	Src image left edge -- must be evenly divisible by 6
											const UWord		inSrcHorzPixelsToCopy)	//	Src image width -- must be evenly divisible by 6
{
	if (inDstHorzPixelOffset % 6)		//	dst pixel offset must be on 6-pixel boundary
		return false;
	if (inSrcHorzPixelOffset % 6)		//	src pixel offset must be on 6-pixel boundary
		return false;
	if (inDstBytesPerLine % 16)			//	dst raster width must be evenly divisible by 16 (width must be multiple of 6)
		return false;
	if (inSrcBytesPerLine % 16)			//	src raster width must be evenly divisible by 16 (width must be multiple of 6)
		return false;
	if (inSrcHorzPixelsToCopy % 6)		//	pixel width of src image portion to copy must be on 6-pixel boundary
		return false;

	const UWord	dstMaxPixelWidth	(inDstBytesPerLine / 16 * 6);
	const UWord	srcMaxPixelWidth	(inSrcBytesPerLine / 16 * 6);
	UWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > srcMaxPixelWidth)
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	NTV2_ASSERT (numHorzPixelsToCopy % 6 == 0);
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * 16 / 6);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * 16 / 6);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * 16 / 6);	//	copy the line
	}

	return true;

}	//	CopyRaster16BytesPer6Pixels


//	This function should work on all 20-byte-per-16-pixel formats
static bool CopyRaster20BytesPer16Pixels (	UByte *			pDstBuffer,				//	Dest buffer to be modified
											const UWord		inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width) -- must be evenly divisible by 20
											const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
											const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
											const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
											const UByte *	pSrcBuffer,				//	Src buffer
											const UWord		inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width) -- must be evenly divisible by 20
											const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
											const UWord		inSrcVertLineOffset,	//	Src image top edge
											const UWord		inSrcVertLinesToCopy,	//	Src image height
											const UWord		inSrcHorzPixelOffset,	//	Src image left edge
											const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	if (inDstHorzPixelOffset % 16)		//	dst pixel offset must be on 16-pixel boundary
		return false;
	if (inSrcHorzPixelOffset % 16)		//	src pixel offset must be on 16-pixel boundary
		return false;
	if (inDstBytesPerLine % 20)			//	dst raster width must be evenly divisible by 20
		return false;
	if (inSrcBytesPerLine % 20)			//	src raster width must be evenly divisible by 20
		return false;
	if (inSrcHorzPixelsToCopy % 16)		//	pixel width of src image portion to copy must be on 16-pixel boundary
		return false;

	const UWord	dstMaxPixelWidth	(inDstBytesPerLine / 20 * 16);
	const UWord	srcMaxPixelWidth	(inSrcBytesPerLine / 20 * 16);
	UWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > srcMaxPixelWidth)
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	NTV2_ASSERT (numHorzPixelsToCopy % 16 == 0);
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * 20 / 16);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * 20 / 16);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * 20 / 16);	//	copy the line
	}

	return true;

}	//	CopyRaster20BytesPer16Pixels


//	This function should work on all 4-byte-per-pixel formats
static bool CopyRaster4BytesPerPixel (	UByte *			pDstBuffer,				//	Dest buffer to be modified
										const UWord		inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
										const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
										const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
										const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear -- must be evenly divisible by 6
										const UByte *	pSrcBuffer,				//	Src buffer
										const UWord		inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
										const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
										const UWord		inSrcVertLineOffset,	//	Src image top edge
										const UWord		inSrcVertLinesToCopy,	//	Src image height
										const UWord		inSrcHorzPixelOffset,	//	Src image left edge
										const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	const UWord	FOUR_BYTES_PER_PIXEL	(4);

	if (inDstBytesPerLine % FOUR_BYTES_PER_PIXEL)	//	dst raster width (in bytes) must be evenly divisible by 4
		return false;
	if (inSrcBytesPerLine % FOUR_BYTES_PER_PIXEL)	//	src raster width (in bytes) must be evenly divisible by 4
		return false;

	const UWord	dstMaxPixelWidth	(inDstBytesPerLine / FOUR_BYTES_PER_PIXEL);
	const UWord	srcMaxPixelWidth	(inSrcBytesPerLine / FOUR_BYTES_PER_PIXEL);
	UWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > srcMaxPixelWidth)
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * FOUR_BYTES_PER_PIXEL);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * FOUR_BYTES_PER_PIXEL);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * FOUR_BYTES_PER_PIXEL);	//	copy the line
	}

	return true;

}	//	CopyRaster4BytesPerPixel


//	This function should work on all 3-byte-per-pixel formats
static bool CopyRaster3BytesPerPixel (	UByte *			pDstBuffer,				//	Dest buffer to be modified
										const UWord		inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
										const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
										const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
										const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear -- must be evenly divisible by 6
										const UByte *	pSrcBuffer,				//	Src buffer
										const UWord		inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
										const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
										const UWord		inSrcVertLineOffset,	//	Src image top edge
										const UWord		inSrcVertLinesToCopy,	//	Src image height
										const UWord		inSrcHorzPixelOffset,	//	Src image left edge
										const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	const UWord	THREE_BYTES_PER_PIXEL	(3);

	if (inDstBytesPerLine % THREE_BYTES_PER_PIXEL)	//	dst raster width (in bytes) must be evenly divisible by 3
		return false;
	if (inSrcBytesPerLine % THREE_BYTES_PER_PIXEL)	//	src raster width (in bytes) must be evenly divisible by 3
		return false;

	const UWord	dstMaxPixelWidth	(inDstBytesPerLine / THREE_BYTES_PER_PIXEL);
	const UWord	srcMaxPixelWidth	(inSrcBytesPerLine / THREE_BYTES_PER_PIXEL);
	UWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > srcMaxPixelWidth)
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * THREE_BYTES_PER_PIXEL);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * THREE_BYTES_PER_PIXEL);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * THREE_BYTES_PER_PIXEL);	//	copy the line
	}

	return true;

}	//	CopyRaster3BytesPerPixel


//	This function should work on all 6-byte-per-pixel formats
static bool CopyRaster6BytesPerPixel (	UByte *			pDstBuffer,				//	Dest buffer to be modified
										const UWord		inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
										const UWord		inDstTotalLines,		//	Dest buffer total raster lines (max height)
										const UWord		inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
										const UWord		inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear -- must be evenly divisible by 6
										const UByte *	pSrcBuffer,				//	Src buffer
										const UWord		inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
										const UWord		inSrcTotalLines,		//	Src buffer total raster lines (max height)
										const UWord		inSrcVertLineOffset,	//	Src image top edge
										const UWord		inSrcVertLinesToCopy,	//	Src image height
										const UWord		inSrcHorzPixelOffset,	//	Src image left edge
										const UWord		inSrcHorzPixelsToCopy)	//	Src image width
{
	const UWord	SIX_BYTES_PER_PIXEL	(6);

	if (inDstBytesPerLine % SIX_BYTES_PER_PIXEL)	//	dst raster width (in bytes) must be evenly divisible by 6
		return false;
	if (inSrcBytesPerLine % SIX_BYTES_PER_PIXEL)	//	src raster width (in bytes) must be evenly divisible by 6
		return false;

	const UWord	dstMaxPixelWidth	(inDstBytesPerLine / SIX_BYTES_PER_PIXEL);
	const UWord	srcMaxPixelWidth	(inSrcBytesPerLine / SIX_BYTES_PER_PIXEL);
	UWord		numHorzPixelsToCopy	(inSrcHorzPixelsToCopy);
	UWord		numVertLinesToCopy	(inSrcVertLinesToCopy);

	if (inDstHorzPixelOffset >= dstMaxPixelWidth)	//	dst past right edge
		return false;
	if (inSrcHorzPixelOffset >= srcMaxPixelWidth)	//	src past right edge
		return false;
	if (inSrcHorzPixelOffset + inSrcHorzPixelsToCopy > srcMaxPixelWidth)
		numHorzPixelsToCopy -= inSrcHorzPixelOffset + inSrcHorzPixelsToCopy - srcMaxPixelWidth;	//	Clip to src raster's right edge
	if (inDstHorzPixelOffset + numHorzPixelsToCopy > dstMaxPixelWidth)
		numHorzPixelsToCopy = inDstHorzPixelOffset + numHorzPixelsToCopy - dstMaxPixelWidth;
	if (inSrcVertLineOffset + inSrcVertLinesToCopy > inSrcTotalLines)
		numVertLinesToCopy -= inSrcVertLineOffset + inSrcVertLinesToCopy - inSrcTotalLines;		//	Clip to src raster's bottom edge
	if (numVertLinesToCopy + inDstVertLineOffset >= inDstTotalLines)
	{
		if (numVertLinesToCopy + inDstVertLineOffset > inDstTotalLines)
			numVertLinesToCopy -= numVertLinesToCopy + inDstVertLineOffset - inDstTotalLines;
		else
			return true;
	}

	for (UWord lineNdx (0);  lineNdx < numVertLinesToCopy;  lineNdx++)	//	for each raster line to copy
	{
		const UByte *	pSrcLine	(pSrcBuffer  +  inSrcBytesPerLine * (inSrcVertLineOffset + lineNdx)  +  inSrcHorzPixelOffset * SIX_BYTES_PER_PIXEL);
		UByte *			pDstLine	(pDstBuffer  +  inDstBytesPerLine * (inDstVertLineOffset + lineNdx)  +  inDstHorzPixelOffset * SIX_BYTES_PER_PIXEL);
		::memcpy (pDstLine, pSrcLine, numHorzPixelsToCopy * SIX_BYTES_PER_PIXEL);	//	copy the line
	}

	return true;

}	//	CopyRaster6BytesPerPixel


bool CopyRaster (const NTV2FrameBufferFormat	inPixelFormat,			//	Pixel format of both src and dst buffers
				UByte *							pDstBuffer,				//	Dest buffer to be modified
				const UWord						inDstBytesPerLine,		//	Dest buffer bytes per raster line (determines max width)
				const UWord						inDstTotalLines,		//	Dest buffer total lines in raster (max height)
				const UWord						inDstVertLineOffset,	//	Vertical line offset into the dest raster where the top edge of the src image will appear
				const UWord						inDstHorzPixelOffset,	//	Horizontal pixel offset into the dest raster where the left edge of the src image will appear
				const UByte *					pSrcBuffer,				//	Src buffer
				const UWord						inSrcBytesPerLine,		//	Src buffer bytes per raster line (determines max width)
				const UWord						inSrcTotalLines,		//	Src buffer total lines in raster (max height)
				const UWord						inSrcVertLineOffset,	//	Src image top edge
				const UWord						inSrcVertLinesToCopy,	//	Src image height
				const UWord						inSrcHorzPixelOffset,	//	Src image left edge
				const UWord						inSrcHorzPixelsToCopy)	//	Src image width
{
	if (!pDstBuffer)					//	NULL buffer
		return false;
	if (!pSrcBuffer)					//	NULL buffer
		return false;
	if (pDstBuffer == pSrcBuffer)		//	src & dst buffers must be different
		return false;
	if (inDstBytesPerLine == 0)			//	zero rowbytes
		return false;
	if (inSrcBytesPerLine == 0)			//	zero rowbytes
		return false;
	if (inDstTotalLines == 0)			//	zero height
		return false;
	if (inSrcTotalLines == 0)			//	zero height
		return false;
	if (inDstVertLineOffset >= inDstTotalLines)		//	dst past bottom edge
		return false;
	if (inSrcVertLineOffset >= inSrcTotalLines)		//	src past bottom edge
		return false;
	switch (inPixelFormat)
	{
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_10BIT_YCBCR_DPX:			return CopyRaster16BytesPer6Pixels (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																					pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																					inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);

		case NTV2_FBF_8BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:			return CopyRaster4BytesPer2Pixels (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																					pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																					inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_10BIT_RGB:				return CopyRaster4BytesPerPixel (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																				pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																				inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);

		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:				return CopyRaster3BytesPerPixel	(pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																				pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																				inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);

		case NTV2_FBF_48BIT_RGB:				return CopyRaster6BytesPerPixel	(pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																				pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																				inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);

		case NTV2_FBF_10BIT_RAW_YCBCR:			return CopyRaster20BytesPer16Pixels (pDstBuffer, inDstBytesPerLine, inDstTotalLines, inDstVertLineOffset, inDstHorzPixelOffset,
																					pSrcBuffer, inSrcBytesPerLine, inSrcTotalLines, inSrcVertLineOffset, inSrcVertLinesToCopy,
																					inSrcHorzPixelOffset, inSrcHorzPixelsToCopy);

		case NTV2_FBF_8BIT_DVCPRO:	//	Lossy
		case NTV2_FBF_8BIT_HDV:		//	Lossy
		case NTV2_FBF_8BIT_YCBCR_420PL3:
		case NTV2_FBF_10BIT_YCBCRA:
		case NTV2_FBF_PRORES:
		case NTV2_FBF_PRORES_DVCPRO:
		case NTV2_FBF_PRORES_HDV:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
		case NTV2_FBF_10BIT_RAW_RGB:
		case NTV2_FBF_8BIT_YCBCR_422PL3:
		case NTV2_FBF_10BIT_YCBCR_420PL3_LE:
		case NTV2_FBF_10BIT_YCBCR_422PL3_LE:
        case NTV2_FBF_10BIT_YCBCR_420PL2:
        case NTV2_FBF_10BIT_YCBCR_422PL2:
        case NTV2_FBF_8BIT_YCBCR_420PL2:
        case NTV2_FBF_8BIT_YCBCR_422PL2:
		case NTV2_FBF_NUMFRAMEBUFFERFORMATS:
			return false;	//	Unsupported
	}
	return false;

}	//	CopyRaster


// GetFramesPerSecond(NTV2FrameRate frameRate)
// seconds per frame.
double GetFramesPerSecond(NTV2FrameRate frameRate)
{
	double framesPerSecond;
    switch (frameRate)
    {
		case NTV2_FRAMERATE_12000:		framesPerSecond = 120.0;			break;
		case NTV2_FRAMERATE_11988:		framesPerSecond = 120.0 / 1.001;	break;
		case NTV2_FRAMERATE_6000:		framesPerSecond = 60.0;				break;
		case NTV2_FRAMERATE_5994:		framesPerSecond = 60.0 / 1.001;		break;
		case NTV2_FRAMERATE_5000:		framesPerSecond = 50.0;				break;
		case NTV2_FRAMERATE_4800:		framesPerSecond = 48.0;				break;
		case NTV2_FRAMERATE_4795:		framesPerSecond = 48.0 / 1.001;		break;
		case NTV2_FRAMERATE_3000:		framesPerSecond = 30.0;				break;
		case NTV2_FRAMERATE_2997:		framesPerSecond = 30.0 / 1.001;		break;
		case NTV2_FRAMERATE_2500:		framesPerSecond = 25.0;				break;
		case NTV2_FRAMERATE_2400:		framesPerSecond = 24.0;				break;
		case NTV2_FRAMERATE_2398:		framesPerSecond = 24.0 / 1.001;		break;
		case NTV2_FRAMERATE_1900:		framesPerSecond = 19.0;				break;
		case NTV2_FRAMERATE_1898:		framesPerSecond = 19.0 / 1.001;		break;
		case NTV2_FRAMERATE_1800:		framesPerSecond = 18.0;				break;
		case NTV2_FRAMERATE_1798:		framesPerSecond = 18.0 / 1.001;		break;
		case NTV2_FRAMERATE_1500:		framesPerSecond = 15.0;				break;
		case NTV2_FRAMERATE_1498:		framesPerSecond = 15.0 / 1.001;		break;

		#if !defined (_DEBUG)
			default:
		#endif
		case NTV2_NUM_FRAMERATES:
		case NTV2_FRAMERATE_UNKNOWN:	framesPerSecond = 30.0 / 1.001;		break;
	}
	return framesPerSecond;
}

// GetFrameTime(NTV2FrameRate frameRate)
// seconds per frame.
double GetFrameTime(NTV2FrameRate frameRate)
{
	double frameTime;
    switch (frameRate)
    {
		case NTV2_FRAMERATE_12000:		frameTime = 1.0   / 120.0;		break;
		case NTV2_FRAMERATE_11988:		frameTime = 1.001 / 120.0;		break;
		case NTV2_FRAMERATE_6000:		frameTime = 1.0   / 60.0;		break;
		case NTV2_FRAMERATE_5994:		frameTime = 1.001 / 60.0;		break;
		case NTV2_FRAMERATE_5000:		frameTime = 1.0   / 50.0;		break;
		case NTV2_FRAMERATE_4800:		frameTime = 1.0   / 48.0;		break;
		case NTV2_FRAMERATE_4795:		frameTime = 1.001 / 48.0;		break;
		case NTV2_FRAMERATE_3000:		frameTime = 1.0   / 30.0;		break;
		case NTV2_FRAMERATE_2997:		frameTime = 1.001 / 30.0;		break;
		case NTV2_FRAMERATE_2500:		frameTime = 1.0   / 25.0;		break;
		case NTV2_FRAMERATE_2400:		frameTime = 1.0   / 24.0;		break;
		case NTV2_FRAMERATE_2398:		frameTime = 1.001 / 24.0;		break;
		case NTV2_FRAMERATE_1900:		frameTime = 1.0   / 19.0;		break;
		case NTV2_FRAMERATE_1898:		frameTime = 1.001 / 19.0;		break;
		case NTV2_FRAMERATE_1800:		frameTime = 1.0   / 18.0;		break;
		case NTV2_FRAMERATE_1798:		frameTime = 1.001 / 18.0;		break;
		case NTV2_FRAMERATE_1500:		frameTime = 1.0   / 15.0;		break;
		case NTV2_FRAMERATE_1498:		frameTime = 1.001 / 15.0;		break;

		#if !defined (_DEBUG)
			default:
		#endif
		case NTV2_NUM_FRAMERATES:
		case NTV2_FRAMERATE_UNKNOWN:	frameTime = 1.001 / 30.0;		break;
	}
	return frameTime;
}


bool GetFramesPerSecond (const NTV2FrameRate inFrameRate, ULWord & outFractionNumerator, ULWord & outFractionDenominator)
{
	switch (inFrameRate)
    {
        case NTV2_FRAMERATE_12000:		outFractionNumerator = 120;     outFractionDenominator = 1;	break;
		case NTV2_FRAMERATE_11988:		outFractionNumerator = 120000;	outFractionDenominator = 1001;	break;
        case NTV2_FRAMERATE_6000:		outFractionNumerator = 60;      outFractionDenominator = 1;	break;
		case NTV2_FRAMERATE_5994:		outFractionNumerator = 60000;	outFractionDenominator = 1001;	break;
        case NTV2_FRAMERATE_5000:		outFractionNumerator = 50;      outFractionDenominator = 1;	break;
        case NTV2_FRAMERATE_4800:		outFractionNumerator = 48;      outFractionDenominator = 1;	break;
		case NTV2_FRAMERATE_4795:		outFractionNumerator = 48000;	outFractionDenominator = 1001;	break;
        case NTV2_FRAMERATE_3000:		outFractionNumerator = 30;      outFractionDenominator = 1;	break;
		case NTV2_FRAMERATE_2997:		outFractionNumerator = 30000;	outFractionDenominator = 1001;	break;
        case NTV2_FRAMERATE_2500:		outFractionNumerator = 25;      outFractionDenominator = 1;	break;
        case NTV2_FRAMERATE_2400:		outFractionNumerator = 24;      outFractionDenominator = 1;	break;
		case NTV2_FRAMERATE_2398:		outFractionNumerator = 24000;	outFractionDenominator = 1001;	break;
        case NTV2_FRAMERATE_1900:		outFractionNumerator = 19;      outFractionDenominator = 1;	break;
		case NTV2_FRAMERATE_1898:		outFractionNumerator = 19000;	outFractionDenominator = 1001;	break;
        case NTV2_FRAMERATE_1800:		outFractionNumerator = 18;      outFractionDenominator = 1;	break;
		case NTV2_FRAMERATE_1798:		outFractionNumerator = 18000;	outFractionDenominator = 1001;	break;
        case NTV2_FRAMERATE_1500:		outFractionNumerator = 15;      outFractionDenominator = 1;	break;
		case NTV2_FRAMERATE_1498:		outFractionNumerator = 15000;	outFractionDenominator = 1001;	break;

		#if !defined (_DEBUG)
			default:
		#endif
		case NTV2_NUM_FRAMERATES:
		case NTV2_FRAMERATE_UNKNOWN:	outFractionNumerator = 0;		outFractionDenominator = 0;		return false;
	}
	return true;
}


NTV2Standard GetNTV2StandardFromScanGeometry(UByte geometry, bool progressiveTransport)
{
	NTV2Standard standard = NTV2_NUM_STANDARDS;

	switch ( geometry )
	{
	case 1:
		standard = NTV2_STANDARD_525;
		break;
	case 2:
		standard = NTV2_STANDARD_625;
		break;
	case 3:
		standard = NTV2_STANDARD_720;
		break;
	case 4:
	case 8:
		if ( progressiveTransport )
			standard = NTV2_STANDARD_1080p;
		else
			standard = NTV2_STANDARD_1080;
		break;
	case 9:
		standard = NTV2_STANDARD_2K;
		break;
	}

	return standard;
}


NTV2VideoFormat GetFirstMatchingVideoFormat (const NTV2FrameRate inFrameRate, const UWord inHeightLines, const UWord inWidthPixels, const bool inIsInterlaced, const bool inIsLevelB)
{
	for (NTV2VideoFormat fmt(NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT);  fmt < NTV2_MAX_NUM_VIDEO_FORMATS;  fmt = NTV2VideoFormat(fmt+1))
		if (inFrameRate == ::GetNTV2FrameRateFromVideoFormat(fmt))
			if (inHeightLines == ::GetDisplayHeight(fmt))
				if (inWidthPixels == ::GetDisplayWidth(fmt))
					if (inIsInterlaced == !::IsProgressiveTransport(fmt))
						if (NTV2_VIDEO_FORMAT_IS_B(fmt) == inIsLevelB)
							return fmt;
	return NTV2_FORMAT_UNKNOWN;
}


NTV2VideoFormat GetQuarterSizedVideoFormat(NTV2VideoFormat videoFormat)
{
	NTV2VideoFormat quarterSizedFormat;

	switch (videoFormat)
	{
        case NTV2_FORMAT_3840x2160psf_2398:
		case NTV2_FORMAT_4x1920x1080psf_2398:	quarterSizedFormat = NTV2_FORMAT_1080psf_2398;   break;
        case NTV2_FORMAT_3840x2160psf_2400:
		case NTV2_FORMAT_4x1920x1080psf_2400:	quarterSizedFormat = NTV2_FORMAT_1080psf_2400;   break;
        case NTV2_FORMAT_3840x2160psf_2500:
		case NTV2_FORMAT_4x1920x1080psf_2500:	quarterSizedFormat = NTV2_FORMAT_1080psf_2500_2; break;
        case NTV2_FORMAT_3840x2160psf_2997:
		case NTV2_FORMAT_4x1920x1080psf_2997:	quarterSizedFormat = NTV2_FORMAT_1080i_5994;   break;	//	NTV2_FORMAT_1080psf_2997
        case NTV2_FORMAT_3840x2160psf_3000:
		case NTV2_FORMAT_4x1920x1080psf_3000:	quarterSizedFormat = NTV2_FORMAT_1080i_6000;   break;	//	NTV2_FORMAT_1080psf_3000
            
        case NTV2_FORMAT_4096x2160psf_2398:
		case NTV2_FORMAT_4x2048x1080psf_2398:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_2398; break;
        case NTV2_FORMAT_4096x2160psf_2400:
		case NTV2_FORMAT_4x2048x1080psf_2400:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_2400; break;
        case NTV2_FORMAT_4096x2160psf_2500:
		case NTV2_FORMAT_4x2048x1080psf_2500:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_2500; break;
		//case NTV2_FORMAT_4x2048x1080psf_2997:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_2997; break;
		//case NTV2_FORMAT_4x2048x1080psf_3000:	quarterSizedFormat = NTV2_FORMAT_1080psf_2K_3000; break;
            
        case NTV2_FORMAT_3840x2160p_2398:
		case NTV2_FORMAT_4x1920x1080p_2398:		quarterSizedFormat = NTV2_FORMAT_1080p_2398; break;
        case NTV2_FORMAT_3840x2160p_2400:
		case NTV2_FORMAT_4x1920x1080p_2400:		quarterSizedFormat = NTV2_FORMAT_1080p_2400; break;
        case NTV2_FORMAT_3840x2160p_2500:
		case NTV2_FORMAT_4x1920x1080p_2500:		quarterSizedFormat = NTV2_FORMAT_1080p_2500; break;
        case NTV2_FORMAT_3840x2160p_2997:
		case NTV2_FORMAT_4x1920x1080p_2997:		quarterSizedFormat = NTV2_FORMAT_1080p_2997; break;
        case NTV2_FORMAT_3840x2160p_3000:
		case NTV2_FORMAT_4x1920x1080p_3000:		quarterSizedFormat = NTV2_FORMAT_1080p_3000; break;
        case NTV2_FORMAT_3840x2160p_5000:
		case NTV2_FORMAT_4x1920x1080p_5000:		quarterSizedFormat = NTV2_FORMAT_1080p_5000_A; break;
        case NTV2_FORMAT_3840x2160p_5994:
		case NTV2_FORMAT_4x1920x1080p_5994:		quarterSizedFormat = NTV2_FORMAT_1080p_5994_A; break;
        case NTV2_FORMAT_3840x2160p_6000:
		case NTV2_FORMAT_4x1920x1080p_6000:		quarterSizedFormat = NTV2_FORMAT_1080p_6000_A; break;
		case NTV2_FORMAT_3840x2160p_5000_B:
		case NTV2_FORMAT_4x1920x1080p_5000_B:		quarterSizedFormat = NTV2_FORMAT_1080p_5000_B; break;
		case NTV2_FORMAT_3840x2160p_5994_B:
		case NTV2_FORMAT_4x1920x1080p_5994_B:		quarterSizedFormat = NTV2_FORMAT_1080p_5994_B; break;
		case NTV2_FORMAT_3840x2160p_6000_B:
		case NTV2_FORMAT_4x1920x1080p_6000_B:		quarterSizedFormat = NTV2_FORMAT_1080p_6000_B; break;

        case NTV2_FORMAT_4096x2160p_2398:
		case NTV2_FORMAT_4x2048x1080p_2398:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_2398; break;
        case NTV2_FORMAT_4096x2160p_2400:
		case NTV2_FORMAT_4x2048x1080p_2400:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_2400; break;
        case NTV2_FORMAT_4096x2160p_2500:
		case NTV2_FORMAT_4x2048x1080p_2500:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_2500; break;
        case NTV2_FORMAT_4096x2160p_2997:
		case NTV2_FORMAT_4x2048x1080p_2997:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_2997; break;
        case NTV2_FORMAT_4096x2160p_3000:
		case NTV2_FORMAT_4x2048x1080p_3000:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_3000; break;
        case NTV2_FORMAT_4096x2160p_4795:
		case NTV2_FORMAT_4x2048x1080p_4795:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_4795_A; break;
        case NTV2_FORMAT_4096x2160p_4800:
		case NTV2_FORMAT_4x2048x1080p_4800:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_4800_A; break;
        case NTV2_FORMAT_4096x2160p_5000:
		case NTV2_FORMAT_4x2048x1080p_5000:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_5000_A; break;
        case NTV2_FORMAT_4096x2160p_5994:
		case NTV2_FORMAT_4x2048x1080p_5994:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_5994_A; break;
        case NTV2_FORMAT_4096x2160p_6000:
		case NTV2_FORMAT_4x2048x1080p_6000:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_6000_A; break;
		case NTV2_FORMAT_4096x2160p_4795_B:
		case NTV2_FORMAT_4x2048x1080p_4795_B:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_4795_B; break;
		case NTV2_FORMAT_4096x2160p_4800_B:
		case NTV2_FORMAT_4x2048x1080p_4800_B:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_4800_B; break;
		case NTV2_FORMAT_4096x2160p_5000_B:
		case NTV2_FORMAT_4x2048x1080p_5000_B:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_5000_B; break;
		case NTV2_FORMAT_4096x2160p_5994_B:
		case NTV2_FORMAT_4x2048x1080p_5994_B:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_5994_B; break;
		case NTV2_FORMAT_4096x2160p_6000_B:
		case NTV2_FORMAT_4x2048x1080p_6000_B:		quarterSizedFormat = NTV2_FORMAT_1080p_2K_6000_B; break;
		// No quarter sized formats for 119.88 or 120 Hz

		default:								quarterSizedFormat = videoFormat; break;
	}

	return quarterSizedFormat;
}


NTV2VideoFormat GetQuadSizedVideoFormat(NTV2VideoFormat videoFormat, bool isSquareDivision)
{
	NTV2VideoFormat quadSizedFormat;

	switch (videoFormat)
	{
    case  NTV2_FORMAT_1080psf_2398:		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2398 : NTV2_FORMAT_3840x2160psf_2398;	break;
    case  NTV2_FORMAT_1080psf_2400:		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2400 : NTV2_FORMAT_3840x2160psf_2400;	break;
    case  NTV2_FORMAT_1080psf_2500_2:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2500 : NTV2_FORMAT_3840x2160psf_2500;	break;
    case  NTV2_FORMAT_1080i_5994:       quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_2997 : NTV2_FORMAT_3840x2160psf_2997;	break;
    case  NTV2_FORMAT_1080i_6000:       quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080psf_3000 : NTV2_FORMAT_3840x2160psf_3000;	break;
                                                                
    case  NTV2_FORMAT_1080psf_2K_2398:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2398 : NTV2_FORMAT_4096x2160psf_2398; break;
    case  NTV2_FORMAT_1080psf_2K_2400:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2400 : NTV2_FORMAT_4096x2160psf_2400; break;
    case  NTV2_FORMAT_1080psf_2K_2500:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080psf_2500 : NTV2_FORMAT_4096x2160psf_2500; break;
    //case NTV2_FORMAT_1080psf_2K_2997:	quaterSizedFormat =  NTV2_FORMAT_4x2048x1080psf_29; break;
    //case NT2_FORMAT_1080psf_2K_3000:e	quaterSizedFormat = NTV2V2_FORMAT_4x2048x1080psf_3000; break;
                                                                
    case  NTV2_FORMAT_1080p_2398:		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2398 : NTV2_FORMAT_3840x2160p_2398; break;
    case  NTV2_FORMAT_1080p_2400: 		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2400 : NTV2_FORMAT_3840x2160p_2400; break;
    case  NTV2_FORMAT_1080p_2500: 		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2500 : NTV2_FORMAT_3840x2160p_2500; break;
    case  NTV2_FORMAT_1080p_2997: 		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_2997 : NTV2_FORMAT_3840x2160p_2997; break;
    case  NTV2_FORMAT_1080p_3000: 		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_3000 : NTV2_FORMAT_3840x2160p_3000; break;
    case  NTV2_FORMAT_1080p_5000_A: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5000 : NTV2_FORMAT_3840x2160p_5000; break;
    case  NTV2_FORMAT_1080p_5994_A: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5994 : NTV2_FORMAT_3840x2160p_5994; break;
    case  NTV2_FORMAT_1080p_6000_A: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_6000 : NTV2_FORMAT_3840x2160p_6000; break;
    case  NTV2_FORMAT_1080p_5000_B:		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5000_B : NTV2_FORMAT_3840x2160p_5000_B; break;
    case  NTV2_FORMAT_1080p_5994_B:		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_5994_B : NTV2_FORMAT_3840x2160p_5994_B; break;
    case  NTV2_FORMAT_1080p_6000_B:		quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x1920x1080p_6000_B : NTV2_FORMAT_3840x2160p_6000_B; break;
                                                                
    case  NTV2_FORMAT_1080p_2K_2398: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2398 : NTV2_FORMAT_4096x2160p_2398; break;
    case  NTV2_FORMAT_1080p_2K_2400: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2400 : NTV2_FORMAT_4096x2160p_2400; break;
    case  NTV2_FORMAT_1080p_2K_2500: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2500 : NTV2_FORMAT_4096x2160p_2500; break;
    case  NTV2_FORMAT_1080p_2K_2997: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_2997 : NTV2_FORMAT_4096x2160p_2997; break;
    case  NTV2_FORMAT_1080p_2K_3000: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_3000 : NTV2_FORMAT_4096x2160p_3000; break;
    case  NTV2_FORMAT_1080p_2K_4795_A: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4795 : NTV2_FORMAT_4096x2160p_4795; break;
    case  NTV2_FORMAT_1080p_2K_4800_A: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4800 : NTV2_FORMAT_4096x2160p_4800; break;
    case  NTV2_FORMAT_1080p_2K_5000_A:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5000 : NTV2_FORMAT_4096x2160p_5000; break;
    case  NTV2_FORMAT_1080p_2K_5994_A:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5994 : NTV2_FORMAT_4096x2160p_5994; break;
    case  NTV2_FORMAT_1080p_2K_6000_A:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_6000 : NTV2_FORMAT_4096x2160p_6000; break;
    case  NTV2_FORMAT_1080p_2K_4795_B: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4795_B : NTV2_FORMAT_4096x2160p_4795_B; break;
    case  NTV2_FORMAT_1080p_2K_4800_B: 	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_4800_B : NTV2_FORMAT_4096x2160p_4800_B; break;
    case  NTV2_FORMAT_1080p_2K_5000_B:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5000_B : NTV2_FORMAT_4096x2160p_5000_B; break;
    case  NTV2_FORMAT_1080p_2K_5994_B:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_5994_B : NTV2_FORMAT_4096x2160p_5994_B; break;
    case  NTV2_FORMAT_1080p_2K_6000_B:	quadSizedFormat = isSquareDivision ? NTV2_FORMAT_4x2048x1080p_6000_B : NTV2_FORMAT_4096x2160p_6000_B; break;


	default:							quadSizedFormat = videoFormat; break;
	}

	return quadSizedFormat;
}

NTV2FrameGeometry GetQuarterSizedGeometry(NTV2FrameGeometry geometry)
{
	switch ( geometry )
	{
		case NTV2_FG_4x1920x1080:
			return NTV2_FG_1920x1080;
		case NTV2_FG_4x2048x1080:
			return NTV2_FG_2048x1080;
		default:
			return geometry;
	}
}


NTV2FrameGeometry Get4xSizedGeometry(NTV2FrameGeometry geometry)
{
	switch ( geometry )
	{
		case NTV2_FG_1920x1080:
			return NTV2_FG_4x1920x1080;
		case NTV2_FG_2048x1080:
			return NTV2_FG_4x2048x1080;
		default:
			return geometry;
	}
}

NTV2Standard GetQuarterSizedStandard(NTV2Standard standard)
{
	switch (standard)
	{
	case NTV2_STANDARD_3840x2160p:
	case NTV2_STANDARD_3840HFR:
		return NTV2_STANDARD_1080p;
	case NTV2_STANDARD_4096x2160p:
	case NTV2_STANDARD_4096HFR:
		return NTV2_STANDARD_2Kx1080p;
	default:
		return standard;
	}
}


NTV2Standard Get4xSizedStandard(NTV2Standard standard, bool bIs4k)
{
	switch (standard)
	{
	case NTV2_STANDARD_1080p:
		return bIs4k ? NTV2_STANDARD_4096x2160p : NTV2_STANDARD_3840x2160p;
	default:
		return standard;
	}
}


NTV2Standard GetNTV2StandardFromVideoFormat (const NTV2VideoFormat inVideoFormat)
{
	NTV2Standard standard = NTV2_STANDARD_INVALID;
	
	switch (inVideoFormat)
    {
		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_1080i_5994:
		case NTV2_FORMAT_1080i_6000:
		case NTV2_FORMAT_1080psf_2398:
		case NTV2_FORMAT_1080psf_2400:
		case NTV2_FORMAT_1080psf_2500_2:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_1080p_5000_B:
		case NTV2_FORMAT_1080p_5994_B:
		case NTV2_FORMAT_1080p_6000_B:
		case NTV2_FORMAT_1080p_2K_4795_B:
		case NTV2_FORMAT_1080p_2K_4800_B:
		case NTV2_FORMAT_1080p_2K_5000_B:
		case NTV2_FORMAT_1080p_2K_5994_B:
		case NTV2_FORMAT_1080p_2K_6000_B:
			standard = NTV2_STANDARD_1080;
			break;
		case NTV2_FORMAT_1080p_2500:
		case NTV2_FORMAT_1080p_2997:
		case NTV2_FORMAT_1080p_3000:
		case NTV2_FORMAT_1080p_2398:
		case NTV2_FORMAT_1080p_2400:
		case NTV2_FORMAT_1080p_5000_A:
		case NTV2_FORMAT_1080p_5994_A:
		case NTV2_FORMAT_1080p_6000_A:
			standard = NTV2_STANDARD_1080p;
			break;
		case NTV2_FORMAT_1080p_2K_2398:
		case NTV2_FORMAT_1080p_2K_2400:
		case NTV2_FORMAT_1080p_2K_2500:
		case NTV2_FORMAT_1080p_2K_2997:
		case NTV2_FORMAT_1080p_2K_3000:
		case NTV2_FORMAT_1080p_2K_4795_A:
		case NTV2_FORMAT_1080p_2K_4800_A:
		case NTV2_FORMAT_1080p_2K_5000_A:
		case NTV2_FORMAT_1080p_2K_5994_A:
		case NTV2_FORMAT_1080p_2K_6000_A:
			standard = NTV2_STANDARD_2Kx1080p;
			break;
        case NTV2_FORMAT_1080psf_2K_2398:
        case NTV2_FORMAT_1080psf_2K_2400:
        case NTV2_FORMAT_1080psf_2K_2500:
            standard = NTV2_STANDARD_2Kx1080i;
            break;

		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_720p_5000:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
		case NTV2_FORMAT_720p_2500:
			standard = NTV2_STANDARD_720;
			break;
		case NTV2_FORMAT_525_5994:
		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_525psf_2997:
			standard = NTV2_STANDARD_525;
			break;
		case NTV2_FORMAT_625_5000:
		case NTV2_FORMAT_625psf_2500:
			standard = NTV2_STANDARD_625 ;
			break;
		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:
			standard = NTV2_STANDARD_2K ;
			break;
		case NTV2_FORMAT_4x1920x1080psf_2398:
		case NTV2_FORMAT_4x1920x1080psf_2400:
		case NTV2_FORMAT_4x1920x1080psf_2500:
		case NTV2_FORMAT_4x1920x1080psf_2997:
		case NTV2_FORMAT_4x1920x1080psf_3000:
		case NTV2_FORMAT_4x1920x1080p_2398:
		case NTV2_FORMAT_4x1920x1080p_2400:
		case NTV2_FORMAT_4x1920x1080p_2500:
		case NTV2_FORMAT_4x1920x1080p_2997:
		case NTV2_FORMAT_4x1920x1080p_3000:
        case NTV2_FORMAT_3840x2160psf_2398:
        case NTV2_FORMAT_3840x2160psf_2400:
        case NTV2_FORMAT_3840x2160psf_2500:
        case NTV2_FORMAT_3840x2160p_2398:
        case NTV2_FORMAT_3840x2160p_2400:
        case NTV2_FORMAT_3840x2160p_2500:
        case NTV2_FORMAT_3840x2160p_2997:
        case NTV2_FORMAT_3840x2160p_3000:
        case NTV2_FORMAT_3840x2160psf_2997:
        case NTV2_FORMAT_3840x2160psf_3000:
			standard = NTV2_STANDARD_3840x2160p;
			break;
		case NTV2_FORMAT_4x1920x1080p_5000:
		case NTV2_FORMAT_4x1920x1080p_5994:
		case NTV2_FORMAT_4x1920x1080p_6000:
        case NTV2_FORMAT_3840x2160p_5000:
        case NTV2_FORMAT_3840x2160p_5994:
        case NTV2_FORMAT_3840x2160p_6000:
			standard = NTV2_STANDARD_3840HFR;
			break;
		case NTV2_FORMAT_4x2048x1080psf_2398:
		case NTV2_FORMAT_4x2048x1080psf_2400:
		case NTV2_FORMAT_4x2048x1080psf_2500:
		case NTV2_FORMAT_4x2048x1080psf_2997:
		case NTV2_FORMAT_4x2048x1080psf_3000:
		case NTV2_FORMAT_4x2048x1080p_2398:
		case NTV2_FORMAT_4x2048x1080p_2400:
		case NTV2_FORMAT_4x2048x1080p_2500:
		case NTV2_FORMAT_4x2048x1080p_2997:
		case NTV2_FORMAT_4x2048x1080p_3000:
        case NTV2_FORMAT_4096x2160psf_2398:
        case NTV2_FORMAT_4096x2160psf_2400:
        case NTV2_FORMAT_4096x2160psf_2500:
        case NTV2_FORMAT_4096x2160p_2398:
        case NTV2_FORMAT_4096x2160p_2400:
        case NTV2_FORMAT_4096x2160p_2500:
        case NTV2_FORMAT_4096x2160p_2997:
        case NTV2_FORMAT_4096x2160p_3000:
        case NTV2_FORMAT_4096x2160psf_2997:
        case NTV2_FORMAT_4096x2160psf_3000:
			standard = NTV2_STANDARD_4096x2160p;
			break;
        case NTV2_FORMAT_4x2048x1080p_4795:
        case NTV2_FORMAT_4x2048x1080p_4800:
		case NTV2_FORMAT_4x2048x1080p_5000:
		case NTV2_FORMAT_4x2048x1080p_5994:
		case NTV2_FORMAT_4x2048x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_11988:
		case NTV2_FORMAT_4x2048x1080p_12000:
        case NTV2_FORMAT_4096x2160p_4795:
        case NTV2_FORMAT_4096x2160p_4800:
        case NTV2_FORMAT_4096x2160p_5000:
        case NTV2_FORMAT_4096x2160p_5994:
        case NTV2_FORMAT_4096x2160p_6000:
        case NTV2_FORMAT_4096x2160p_11988:
        case NTV2_FORMAT_4096x2160p_12000:
			standard = NTV2_STANDARD_4096HFR;
			break;
#if defined (_DEBUG)
	//	Debug builds warn about missing values
		case NTV2_FORMAT_UNKNOWN:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
		case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
		case NTV2_FORMAT_END_2K_DEF_FORMATS:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
        case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
        case NTV2_FORMAT_FIRST_4K_DEF_FORMAT2:
        case NTV2_FORMAT_END_4K_DEF_FORMATS2:
        case NTV2_FORMAT_3840x2160p_5000_B:
        case NTV2_FORMAT_3840x2160p_5994_B:
        case NTV2_FORMAT_3840x2160p_6000_B:
        case NTV2_FORMAT_4096x2160p_4795_B:
        case NTV2_FORMAT_4096x2160p_4800_B:
        case NTV2_FORMAT_4096x2160p_5000_B:
        case NTV2_FORMAT_4096x2160p_5994_B:
        case NTV2_FORMAT_4096x2160p_6000_B:
        case NTV2_FORMAT_4x1920x1080p_5994_B:
        case NTV2_FORMAT_4x1920x1080p_6000_B:
        case NTV2_FORMAT_4x2048x1080p_4795_B:
		case NTV2_FORMAT_4x2048x1080p_4800_B:
		case NTV2_FORMAT_4x2048x1080p_5000_B:
		case NTV2_FORMAT_4x2048x1080p_5994_B:
		case NTV2_FORMAT_4x2048x1080p_6000_B:
			break;	// Unsupported
#else
        default:
            break;
#endif
    }
	
	return standard;
}

//-------------------------------------------------------------------------------------------------------
//	GetNTV2FrameGeometryFromVideoFormat
//-------------------------------------------------------------------------------------------------------
NTV2FrameGeometry GetNTV2FrameGeometryFromVideoFormat(const NTV2VideoFormat inVideoFormat)
{
	NTV2FrameGeometry result = NTV2_FG_INVALID;

	switch (inVideoFormat)
	{
		case NTV2_FORMAT_4x1920x1080psf_2398:
		case NTV2_FORMAT_4x1920x1080psf_2400:
		case NTV2_FORMAT_4x1920x1080psf_2500:
		case NTV2_FORMAT_4x1920x1080psf_2997:
		case NTV2_FORMAT_4x1920x1080psf_3000:
		case NTV2_FORMAT_4x1920x1080p_2398:
		case NTV2_FORMAT_4x1920x1080p_2400:
		case NTV2_FORMAT_4x1920x1080p_2500:
		case NTV2_FORMAT_4x1920x1080p_2997:
		case NTV2_FORMAT_4x1920x1080p_3000:
		case NTV2_FORMAT_4x1920x1080p_5000:
		case NTV2_FORMAT_4x1920x1080p_5994:
		case NTV2_FORMAT_4x1920x1080p_6000:
		case NTV2_FORMAT_3840x2160psf_2398:
		case NTV2_FORMAT_3840x2160psf_2400:
		case NTV2_FORMAT_3840x2160psf_2500:
		case NTV2_FORMAT_3840x2160p_2398:
		case NTV2_FORMAT_3840x2160p_2400:
		case NTV2_FORMAT_3840x2160p_2500:
		case NTV2_FORMAT_3840x2160p_2997:
		case NTV2_FORMAT_3840x2160p_3000:
		case NTV2_FORMAT_3840x2160psf_2997:
		case NTV2_FORMAT_3840x2160psf_3000:
		case NTV2_FORMAT_3840x2160p_5000:
		case NTV2_FORMAT_3840x2160p_5994:
		case NTV2_FORMAT_3840x2160p_6000:
		case NTV2_FORMAT_4x1920x1080p_5000_B:
		case NTV2_FORMAT_4x1920x1080p_5994_B:
		case NTV2_FORMAT_4x1920x1080p_6000_B:
		case NTV2_FORMAT_3840x2160p_5000_B:
		case NTV2_FORMAT_3840x2160p_5994_B:
		case NTV2_FORMAT_3840x2160p_6000_B:
			result = NTV2_FG_4x1920x1080;
			break;

		case NTV2_FORMAT_4x2048x1080psf_2398:
		case NTV2_FORMAT_4x2048x1080psf_2400:
		case NTV2_FORMAT_4x2048x1080psf_2500:
		case NTV2_FORMAT_4x2048x1080p_2398:
		case NTV2_FORMAT_4x2048x1080p_2400:
		case NTV2_FORMAT_4x2048x1080p_2500:
		case NTV2_FORMAT_4x2048x1080p_2997:
		case NTV2_FORMAT_4x2048x1080p_3000:
		case NTV2_FORMAT_4x2048x1080psf_2997:
		case NTV2_FORMAT_4x2048x1080psf_3000:
		case NTV2_FORMAT_4x2048x1080p_4795:
		case NTV2_FORMAT_4x2048x1080p_4800:
		case NTV2_FORMAT_4x2048x1080p_5000:
		case NTV2_FORMAT_4x2048x1080p_5994:
		case NTV2_FORMAT_4x2048x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_11988:
		case NTV2_FORMAT_4x2048x1080p_12000:
		case NTV2_FORMAT_4096x2160psf_2398:
		case NTV2_FORMAT_4096x2160psf_2400:
		case NTV2_FORMAT_4096x2160psf_2500:
		case NTV2_FORMAT_4096x2160p_2398:
		case NTV2_FORMAT_4096x2160p_2400:
		case NTV2_FORMAT_4096x2160p_2500:
		case NTV2_FORMAT_4096x2160p_2997:
		case NTV2_FORMAT_4096x2160p_3000:
		case NTV2_FORMAT_4096x2160psf_2997:
		case NTV2_FORMAT_4096x2160psf_3000:
		case NTV2_FORMAT_4096x2160p_4795:
		case NTV2_FORMAT_4096x2160p_4800:
		case NTV2_FORMAT_4096x2160p_5000:
		case NTV2_FORMAT_4096x2160p_5994:
		case NTV2_FORMAT_4096x2160p_6000:
		case NTV2_FORMAT_4096x2160p_11988:
		case NTV2_FORMAT_4096x2160p_12000:
		case NTV2_FORMAT_4x2048x1080p_4795_B:
		case NTV2_FORMAT_4x2048x1080p_4800_B:
		case NTV2_FORMAT_4x2048x1080p_5000_B:
		case NTV2_FORMAT_4x2048x1080p_5994_B:
		case NTV2_FORMAT_4x2048x1080p_6000_B:
		case NTV2_FORMAT_4096x2160p_4795_B:
		case NTV2_FORMAT_4096x2160p_4800_B:
		case NTV2_FORMAT_4096x2160p_5000_B:
		case NTV2_FORMAT_4096x2160p_5994_B:
		case NTV2_FORMAT_4096x2160p_6000_B:
			result = NTV2_FG_4x2048x1080;
			break;

		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:
			result = NTV2_FG_2048x1556;
			break;

		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_1080i_5994:
		case NTV2_FORMAT_1080i_6000:
		case NTV2_FORMAT_1080psf_2500_2:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_1080psf_2398:
		case NTV2_FORMAT_1080psf_2400:
		case NTV2_FORMAT_1080p_2997:
		case NTV2_FORMAT_1080p_3000:
		case NTV2_FORMAT_1080p_2398:
		case NTV2_FORMAT_1080p_2400:
		case NTV2_FORMAT_1080p_2500:
		case NTV2_FORMAT_1080p_5000_B:
		case NTV2_FORMAT_1080p_5994_B:
		case NTV2_FORMAT_1080p_6000_B:
		case NTV2_FORMAT_1080p_5000_A:
		case NTV2_FORMAT_1080p_5994_A:
		case NTV2_FORMAT_1080p_6000_A:
			result = NTV2_FG_1920x1080;
			break;

		case NTV2_FORMAT_1080p_2K_2398:
		case NTV2_FORMAT_1080p_2K_2400:
		case NTV2_FORMAT_1080p_2K_2500:
		case NTV2_FORMAT_1080psf_2K_2398:
		case NTV2_FORMAT_1080psf_2K_2400:
		case NTV2_FORMAT_1080psf_2K_2500:
		case NTV2_FORMAT_1080p_2K_2997:
		case NTV2_FORMAT_1080p_2K_3000:
		case NTV2_FORMAT_1080p_2K_4795_A:
		case NTV2_FORMAT_1080p_2K_4795_B:
		case NTV2_FORMAT_1080p_2K_4800_A:
		case NTV2_FORMAT_1080p_2K_4800_B:
		case NTV2_FORMAT_1080p_2K_5000_A:
		case NTV2_FORMAT_1080p_2K_5000_B:
		case NTV2_FORMAT_1080p_2K_5994_A:
		case NTV2_FORMAT_1080p_2K_5994_B:
		case NTV2_FORMAT_1080p_2K_6000_A:
		case NTV2_FORMAT_1080p_2K_6000_B:
			result = NTV2_FG_2048x1080;
			break;

		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_720p_2500:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
		case NTV2_FORMAT_720p_5000:
			result = NTV2_FG_1280x720;
			break;

		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_525_5994:
		case NTV2_FORMAT_525psf_2997:
			result = NTV2_FG_720x486;
			break;

		case NTV2_FORMAT_625_5000:
		case NTV2_FORMAT_625psf_2500:
			result = NTV2_FG_720x576;
			break;

#if defined (_DEBUG)
	//	Debug builds warn about missing values
		case NTV2_FORMAT_UNKNOWN:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
		case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
		case NTV2_FORMAT_END_2K_DEF_FORMATS:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
		case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
		case NTV2_FORMAT_END_4K_DEF_FORMATS2:
			break;	// Unsupported
#else
		default:
			break;
#endif
	}

	return result;
}


#if !defined(NTV2_DEPRECATE_13_0)
	// GetVideoActiveSize: returns the number of bytes of active video (including VANC lines, if any)
	ULWord GetVideoActiveSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat, const bool inTallVANC, const bool inTallerVANC)
	{
		return GetVideoActiveSize (inVideoFormat, inFBFormat, NTV2VANCModeFromBools (inTallVANC, inTallerVANC));
	}	//	GetVideoActiveSize

	ULWord GetVideoWriteSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat, const bool inTallVANC, const bool inTallerVANC)
	{
		return ::GetVideoWriteSize (inVideoFormat, inFBFormat, NTV2VANCModeFromBools (inTallVANC, inTallerVANC));
	}
#endif	//	!defined(NTV2_DEPRECATE_13_0)


ULWord GetVideoActiveSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat, const NTV2VANCMode inVancMode)
{
	if (!NTV2_IS_VALID_VANCMODE (inVancMode))
		return 0;
	if (!NTV2_IS_VALID_VIDEO_FORMAT (inVideoFormat))
		return 0;
	if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT (inFBFormat))
		return 0;

	//	Planar formats are special -- and VANC doesn't apply...
	switch (inFBFormat)
	{
        case NTV2_FBF_10BIT_YCBCR_420PL2:	return GetDisplayWidth (inVideoFormat) * GetDisplayHeight (inVideoFormat) * 3 / 2 * 10 / 8;
        case NTV2_FBF_10BIT_YCBCR_422PL2:	return GetDisplayWidth (inVideoFormat) * GetDisplayHeight (inVideoFormat) * 2 * 10 / 8;
        case NTV2_FBF_8BIT_YCBCR_420PL2:	return GetDisplayWidth (inVideoFormat) * GetDisplayHeight (inVideoFormat) * 3 / 2;
        case NTV2_FBF_8BIT_YCBCR_422PL2:	return GetDisplayWidth (inVideoFormat) * GetDisplayHeight (inVideoFormat) * 2;
		default:							break;
	}

	const NTV2Standard	videoStandard	(::GetNTV2StandardFromVideoFormat (inVideoFormat));
	if (!NTV2_IS_VALID_STANDARD (videoStandard))
		return 0;

	const NTV2FormatDescriptor	fd	(videoStandard, inFBFormat, inVancMode);
	if (!fd.IsValid ())
		return 0;

    return fd.linePitch * fd.GetFullRasterHeight () * 4 /*sizeof(ULWord)*/;

}	//	GetVideoActiveSize


// GetVideoWriteSize
// At least in Windows, to get bursting to work between our board and the disk
// system  without going through the file manager cache, you need to open the file
// with FILE_FLAG_NO_BUFFERING flag. With this you must do reads and writes
// on 4096 byte boundaries with most modern disk systems. You could actually
// do 512 on some systems though.
// So this function takes in the videoformat and the framebufferformat
// and gets the framesize you need to write to meet this requirement.
//

ULWord GetVideoWriteSize (const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat, const NTV2VANCMode inVancMode)
{
	ULWord ulSize = ::GetVideoActiveSize (inVideoFormat, inFBFormat, inVancMode);
	if (ulSize % 4096)
		ulSize = ((ulSize / 4096) + 1) * 4096;
	return ulSize;
}


// GetAudioSamplesPerFrame(NTV2FrameRate frameRate, NTV2AudioRate audioRate)
// For a given framerate and audiorate, returns how many audio samples there
// will be in a frame's time. cadenceFrame is only used for 5994 or 2997 @ 48k.
// smpte372Enabled indicates that you are doing 1080p60,1080p5994 or 1080p50
// in this mode the boards framerate might be NTV2_FRAMERATE_3000, but since
// 2 links are coming out, the video rate is actually NTV2_FRAMERATE_6000
ULWord GetAudioSamplesPerFrame(NTV2FrameRate frameRate, NTV2AudioRate audioRate, ULWord cadenceFrame,bool smpte372Enabled)
{
	ULWord audioSamplesPerFrame=0;
	cadenceFrame %= 5;

	if( smpte372Enabled )
	{
		// the video is actually coming out twice as fast as the board rate
		// since there are 2 links.
		switch ( frameRate )
		{
		case NTV2_FRAMERATE_3000:
			frameRate = NTV2_FRAMERATE_6000;
			break;
		case NTV2_FRAMERATE_2997:
			frameRate = NTV2_FRAMERATE_5994;
			break;
		case NTV2_FRAMERATE_2500:
			frameRate = NTV2_FRAMERATE_5000;
			break;
		case NTV2_FRAMERATE_2400:
			frameRate = NTV2_FRAMERATE_4800;
			break;
		case NTV2_FRAMERATE_2398:
			frameRate = NTV2_FRAMERATE_4795;
			break;
		default:
			break;
		}
	}

	if ( audioRate == NTV2_AUDIO_48K)
	{
		switch ( frameRate)
		{
			case NTV2_FRAMERATE_12000:
				audioSamplesPerFrame = 400;
				break;
			case NTV2_FRAMERATE_11988:
				switch ( cadenceFrame )
				{
				case 0:
				case 2:
				case 4:
					audioSamplesPerFrame = 400;
					break;
				case 1:
				case 3:
					audioSamplesPerFrame = 401;
					break;
				}
				break;
			case NTV2_FRAMERATE_6000:
				audioSamplesPerFrame = 800;
				break;
			case NTV2_FRAMERATE_5994:
				switch ( cadenceFrame )
				{
				case 0:
					audioSamplesPerFrame = 800;
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					audioSamplesPerFrame = 801;
					break;
				}
				break;
			case NTV2_FRAMERATE_5000:
				audioSamplesPerFrame = 1920/2;
				break;
			case NTV2_FRAMERATE_4800:
				audioSamplesPerFrame = 1000;
				break;
			case NTV2_FRAMERATE_4795:
				audioSamplesPerFrame = 1001;
				break;
			case NTV2_FRAMERATE_3000:
				audioSamplesPerFrame = 1600;
				break;
			case NTV2_FRAMERATE_2997:
				// depends on cadenceFrame;
				switch ( cadenceFrame )
				{
				case 0:
				case 2:
				case 4:
					audioSamplesPerFrame = 1602;
					break;
				case 1:
				case 3:
					audioSamplesPerFrame = 1601;
					break;
				}
				break;
			case NTV2_FRAMERATE_2500:
				audioSamplesPerFrame = 1920;
				break;
			case NTV2_FRAMERATE_2400:
				audioSamplesPerFrame = 2000;
				break;
			case NTV2_FRAMERATE_2398:
				audioSamplesPerFrame = 2002;
				break;
			case NTV2_FRAMERATE_1500:
				audioSamplesPerFrame = 3200;
				break;
			case NTV2_FRAMERATE_1498:
				// depends on cadenceFrame;
				switch ( cadenceFrame )
				{
				case 0:
					audioSamplesPerFrame = 3204;
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					audioSamplesPerFrame = 3203;
					break;
				}
				break;
			case NTV2_FRAMERATE_1900:	// Not supported yet
			case NTV2_FRAMERATE_1898:	// Not supported yet
			case NTV2_FRAMERATE_1800: 	// Not supported yet
			case NTV2_FRAMERATE_1798:	// Not supported yet
			case NTV2_FRAMERATE_UNKNOWN:
			case NTV2_NUM_FRAMERATES:
				audioSamplesPerFrame = 0;
				break;
		}
	}
	else
	if ( audioRate == NTV2_AUDIO_96K)
	{
		switch ( frameRate)
		{
			case NTV2_FRAMERATE_12000:
				audioSamplesPerFrame = 800;
				break;
			case NTV2_FRAMERATE_11988:
				switch ( cadenceFrame )
				{
				case 0:
				case 1:
				case 2:
				case 3:
					audioSamplesPerFrame = 901;
					break;
				case 4:
					audioSamplesPerFrame = 800;
					break;
				}
				break;
			case NTV2_FRAMERATE_6000:
				audioSamplesPerFrame = 800*2;
				break;
			case NTV2_FRAMERATE_5994:
				switch ( cadenceFrame )
				{
				case 0:
				case 2:
				case 4:
					audioSamplesPerFrame = 1602;
					break;
				case 1:
				case 3:
					audioSamplesPerFrame = 1601;
					break;
				}
				break;
			case NTV2_FRAMERATE_5000:
				audioSamplesPerFrame = 1920;
				break;
			case NTV2_FRAMERATE_4800:
				audioSamplesPerFrame = 2000;
				break;
			case NTV2_FRAMERATE_4795:
				audioSamplesPerFrame = 2002;
				break;
			case NTV2_FRAMERATE_3000:
				audioSamplesPerFrame = 1600*2;
				break;
			case NTV2_FRAMERATE_2997:
				// depends on cadenceFrame;
				switch ( cadenceFrame )
				{
				case 0:
					audioSamplesPerFrame = 3204;
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					audioSamplesPerFrame = 3203;
					break;
				}
				break;
			case NTV2_FRAMERATE_2500:
				audioSamplesPerFrame = 1920*2;
				break;
			case NTV2_FRAMERATE_2400:
				audioSamplesPerFrame = 2000*2;
				break;
			case NTV2_FRAMERATE_2398:
				audioSamplesPerFrame = 2002*2;
				break;
			case NTV2_FRAMERATE_1500:
				audioSamplesPerFrame = 3200*2;
				break;
			case NTV2_FRAMERATE_1498:
				// depends on cadenceFrame;
				switch ( cadenceFrame )
				{
				case 0:
					audioSamplesPerFrame = 3204*2;
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					audioSamplesPerFrame = 3203*2;
					break;
				}
				break;
			case NTV2_FRAMERATE_1900:	// Not supported yet
			case NTV2_FRAMERATE_1898:	// Not supported yet
			case NTV2_FRAMERATE_1800: 	// Not supported yet
			case NTV2_FRAMERATE_1798:	// Not supported yet
			case NTV2_FRAMERATE_UNKNOWN:
			case NTV2_NUM_FRAMERATES:
				audioSamplesPerFrame = 0*2; //haha
				break;
		}
	}

	return audioSamplesPerFrame;
}


// GetTotalAudioSamplesFromFrameNbrZeroUpToFrameNbr(NTV2FrameRate frameRate, NTV2AudioRate audioRate, ULWord frameNbrNonInclusive)
// For a given framerate and audiorate and ending frame number (non-inclusive), returns the total number of audio samples over
// the range of video frames starting at frame number zero up to and not including the passed in frame number, frameNbrNonInclusive.
// Utilizes cadence patterns in function immediately above,  GetAudioSamplesPerFrame().
// No smpte372Enabled support
LWord64 GetTotalAudioSamplesFromFrameNbrZeroUpToFrameNbr(NTV2FrameRate frameRate, NTV2AudioRate audioRate, ULWord frameNbrNonInclusive)
{
	LWord64 numTotalAudioSamples;
	LWord64 numAudioSamplesFromWholeGroups;

	ULWord numWholeGroupsOfFive;
	ULWord numAudioSamplesFromRemainder;
	ULWord remainder;

	numWholeGroupsOfFive = frameNbrNonInclusive/5;
	remainder = frameNbrNonInclusive % 5;

	numTotalAudioSamples = 0;
	numAudioSamplesFromWholeGroups = 0;
	numAudioSamplesFromRemainder = 0;

	if (audioRate == NTV2_AUDIO_48K)
	{
		switch (frameRate)
		{
			case NTV2_FRAMERATE_12000:
				numTotalAudioSamples = 400 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_11988:
				numAudioSamplesFromWholeGroups = ((2*401) + (3*400)) * numWholeGroupsOfFive;
				numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((400 * remainder) + remainder/2);
				numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
				break;
			case NTV2_FRAMERATE_6000:
				numTotalAudioSamples = 800 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_5994:
				// depends on cadenceFrame;
				numAudioSamplesFromWholeGroups = ((1*800) + (4*801)) * numWholeGroupsOfFive;
				numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((801 * remainder) - 1);
				numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
				break;
			case NTV2_FRAMERATE_5000:
				numTotalAudioSamples = 1920/2 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_4800:
				numTotalAudioSamples = 1000 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_4795:
				numTotalAudioSamples = 1001 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_3000:
				numTotalAudioSamples = 1600 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_2997:
				// depends on cadenceFrame;
				numAudioSamplesFromWholeGroups = ((3*1602) + (2*1601)) * numWholeGroupsOfFive;
				numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((1602 * remainder) - remainder/2);
				numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
				break;
			case NTV2_FRAMERATE_2500:
				numTotalAudioSamples = 1920 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_2400:
				numTotalAudioSamples = 2000 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_2398:
				numTotalAudioSamples = 2002 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_1500:
				numTotalAudioSamples = 3200 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_1498:
				// depends on cadenceFrame;
				numAudioSamplesFromWholeGroups = ((1*3204) + (4*3203)) * numWholeGroupsOfFive;
				numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((3203 * remainder) + 1);
				numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
				break;
			case NTV2_FRAMERATE_1900:	// Not supported yet
			case NTV2_FRAMERATE_1898:	// Not supported yet
			case NTV2_FRAMERATE_1800: 	// Not supported yet
			case NTV2_FRAMERATE_1798:	// Not supported yet
			case NTV2_FRAMERATE_UNKNOWN:
			case NTV2_NUM_FRAMERATES:
				numTotalAudioSamples = 0;
				break;
		}
	}
	else
	if (audioRate == NTV2_AUDIO_96K)
	{
		switch (frameRate)
		{
			case NTV2_FRAMERATE_12000:
				numTotalAudioSamples = 800 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_11988:
				numAudioSamplesFromWholeGroups = ((4*801) + (1*800)) * numWholeGroupsOfFive;
				numAudioSamplesFromRemainder = (remainder == 0) ? 0 : (801 * remainder);
				numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
				break;
			case NTV2_FRAMERATE_6000:
				numTotalAudioSamples = (800*2) * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_5994:
				numAudioSamplesFromWholeGroups = ((3*1602) + (2*1601)) * numWholeGroupsOfFive;
				numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((1602 * remainder) - remainder/2);
				numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
				break;
			case NTV2_FRAMERATE_5000:
				numTotalAudioSamples = 1920 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_4800:
				numTotalAudioSamples = 2000 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_4795:
				numTotalAudioSamples = 2002 * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_3000:
				numTotalAudioSamples = (1600*2) * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_2997:
				// depends on cadenceFrame;
				numAudioSamplesFromWholeGroups = ((1*3204) + (4*3203)) * numWholeGroupsOfFive;
				numAudioSamplesFromRemainder = (remainder == 0) ? 0 : ((3203 * remainder) + 1);
				numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
				break;
			case NTV2_FRAMERATE_2500:
				numTotalAudioSamples = (1920*2) * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_2400:
				numTotalAudioSamples = (2000*2) * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_2398:
				numTotalAudioSamples = (2002*2) * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_1500:
				numTotalAudioSamples = (3200*2) * frameNbrNonInclusive;
				break;
			case NTV2_FRAMERATE_1498:
				// depends on cadenceFrame;
				numAudioSamplesFromWholeGroups = ((1*3204*2) + (4*3203*2)) * numWholeGroupsOfFive;
				numAudioSamplesFromRemainder = (remainder == 0) ? 0 : (((3203*2) * remainder) + 2);
				numTotalAudioSamples = numAudioSamplesFromWholeGroups + numAudioSamplesFromRemainder;
				break;
			case NTV2_FRAMERATE_1900:	// Not supported yet
			case NTV2_FRAMERATE_1898:	// Not supported yet
			case NTV2_FRAMERATE_1800: 	// Not supported yet
			case NTV2_FRAMERATE_1798:	// Not supported yet
			case NTV2_FRAMERATE_UNKNOWN:
			case NTV2_NUM_FRAMERATES:
				numTotalAudioSamples = 0*2; //haha
				break;
		}
	}

	return numTotalAudioSamples;
}

// GetVaricamRepeatCount(NTV2FrameRate sequenceRate, NTV2FrameRate playRate, ULWord cadenceFrame)
// For a given sequenceRate and playRate, given the cadenceFrame it returns how many times we
// repeate the frame to output varicam.  If the result is zero then this is an unsupported varicam
// rate.
ULWord GetVaricamRepeatCount(NTV2FrameRate sequenceRate, NTV2FrameRate playRate, ULWord cadenceFrame)
{
	ULWord result = 0;

	switch (playRate)
	{
		case NTV2_FRAMERATE_6000:
			switch (sequenceRate)
			{
				case NTV2_FRAMERATE_1500:
					result = 4;
					break;
				case NTV2_FRAMERATE_2400:			// 24 -> 60					2:3|2:3|2:3 ...
					cadenceFrame %= 2;
					switch (cadenceFrame)
					{
						case 0:
							result = 2;
							break;
						case 1:
							result = 3;
							break;
					}
					break;
				case NTV2_FRAMERATE_2500:			// 25 -> 60					2:3:2:3:2|2:3:2:3:2 ...
					cadenceFrame %= 5;
					switch (cadenceFrame)
					{
						case 0:
						case 2:
						case 4:
							result = 2;
							break;
						case 1:
						case 3:
							result = 3;
							break;
					}
					break;
				case NTV2_FRAMERATE_3000:			// 30 -> 60					2|2|2|2|2|2 ...
					result = 2;
					break;
				case NTV2_FRAMERATE_4800:			// 48 -> 60					2:1:1:1|2:1:1:1 ...
					cadenceFrame %= 4;
					switch (cadenceFrame)
					{
						case 0:
							result = 2;
							break;
						case 1:
						case 2:
						case 3:
							result = 1;
							break;
					}
					break;
				case NTV2_FRAMERATE_5000:			// 50 -> 60					2:1:1:1:1|2:1:1:1:1: ...
					cadenceFrame %= 5;
					switch (cadenceFrame)
					{
						case 0:
							result = 2;
							break;
						case 1:
						case 2:
						case 3:
						case 4:
							result = 1;
							break;
					}
					break;
				case NTV2_FRAMERATE_6000:			// 60 -> 60					1|1|1|1|1|1 ...
					result = 1;
					break;
				default:
					break;
			}
			break;

		case NTV2_FRAMERATE_5994:
			switch (sequenceRate)
			{
				case NTV2_FRAMERATE_1498:
					result = 4;
					break;
				case NTV2_FRAMERATE_2398:			// 23.98 -> 59.94			2:3|2:3|2:3 ...
					cadenceFrame %= 2;
					switch (cadenceFrame)
					{
						case 0:
							result = 2;
							break;
						case 1:
							result = 3;
							break;
					}
					break;
				case NTV2_FRAMERATE_2997:			// 29.97 -> 59.94			2|2|2|2|2|2 ...
					result = 2;
					break;
				case NTV2_FRAMERATE_4795:			// 47.95 -> 59.94			2:1:1:1|2:1:1:1 ...
					cadenceFrame %= 4;
					switch (cadenceFrame)
					{
						case 0:
							result = 2;
							break;
						case 1:
						case 2:
						case 3:
							result = 1;
							break;
					}
					break;
				case NTV2_FRAMERATE_5994:			// 59.94 -> 59.94			1|1|1|1|1|1 ...
					result = 1;
					break;
				default:
					break;
			}
			break;

		case NTV2_FRAMERATE_5000:
			switch (sequenceRate)
			{
				case NTV2_FRAMERATE_2500:			// 25 -> 50					2|2|2|2|2| ...
					result = 2;
					break;
				default:
					break;
			}
			break;

		default:
			break;
	}
	return result;
}

ULWord GetScaleFromFrameRate(NTV2FrameRate frameRate)
{
	switch (frameRate)
	{
		case NTV2_FRAMERATE_12000:		return 12000;
		case NTV2_FRAMERATE_11988:		return 11988;
		case NTV2_FRAMERATE_6000:		return 6000;
		case NTV2_FRAMERATE_5994:		return 5994;
		case NTV2_FRAMERATE_5000:		return 5000;
		case NTV2_FRAMERATE_4800:		return 4800;
		case NTV2_FRAMERATE_4795:		return 4795;
		case NTV2_FRAMERATE_3000:		return 3000;
		case NTV2_FRAMERATE_2997:		return 2997;
		case NTV2_FRAMERATE_2500:		return 2500;
		case NTV2_FRAMERATE_2400:		return 2400;
		case NTV2_FRAMERATE_2398:		return 2398;
		case NTV2_FRAMERATE_1900:		return 1900;
		case NTV2_FRAMERATE_1898:		return 1898;
		case NTV2_FRAMERATE_1800:		return 1800;
		case NTV2_FRAMERATE_1798:		return 1798;
		case NTV2_FRAMERATE_1500:		return 1500;
		case NTV2_FRAMERATE_1498:		return 1498;
		case NTV2_FRAMERATE_UNKNOWN:	break;
	#if defined(_DEBUG)
		case NTV2_NUM_FRAMERATES:		break;
	#else
		default:						break;
	#endif
	}
	return 0;
}

// GetFrameRateFromScale(long scale, long duration, NTV2FrameRate playFrameRate)
// For a given scale value it returns the associated frame rate.  This routine is
// used to calculate and decipher the sequence frame rate.
NTV2FrameRate GetFrameRateFromScale(long scale, long duration, NTV2FrameRate playFrameRate)
{
	NTV2FrameRate result = NTV2_FRAMERATE_6000;

	// Generally the duration is 100 and in that event the scale will tell us for sure what the
	// sequence rate is.
	if (duration == 100)
	{
		switch (scale)
		{
			case 12000:	result = NTV2_FRAMERATE_12000;	break;
			case 11988:	result = NTV2_FRAMERATE_11988;	break;
			case 6000:	result = NTV2_FRAMERATE_6000;	break;
			case 5994:	result = NTV2_FRAMERATE_5994;	break;
			case 5000:	result = NTV2_FRAMERATE_5000;	break;
			case 4800:	result = NTV2_FRAMERATE_4800;	break;
			case 4795:	result = NTV2_FRAMERATE_4795;	break;
			case 3000:	result = NTV2_FRAMERATE_3000;	break;
			case 2997:	result = NTV2_FRAMERATE_2997;	break;
			case 2500:	result = NTV2_FRAMERATE_2500;	break;
			case 2400:	result = NTV2_FRAMERATE_2400;	break;
			case 2398:	result = NTV2_FRAMERATE_2398;	break;
			case 1500:	result = NTV2_FRAMERATE_1500;	break;
			case 1498:	result = NTV2_FRAMERATE_1498;	break;
		}
	}
	else if (duration == 0)
	{
		result = playFrameRate;
	}
	else
	{
		float scaleFloat = scale / duration * (float)100.0;
		long scaleInt = (long) scaleFloat;

		// In this case we need to derive the sequence rate based on scale, duration and what
		// our playback rate is.  So first we break down what we might expect based on our
		// playback rate.  This gives us some room to look at values that are returned and
		// which are not exact based on rounding errors.  We can break this check up into two
		// camps because the assumption is we don't have to worry about playing back 23.98 fps
		// sequence on a 60 fps output and conversly playing back 30 fps sequences on a 59.94
		// fps output.
		switch (playFrameRate)
		{
			case NTV2_FRAMERATE_12000:
			case NTV2_FRAMERATE_6000:
			case NTV2_FRAMERATE_5000:
			case NTV2_FRAMERATE_4800:
			case NTV2_FRAMERATE_3000:
			case NTV2_FRAMERATE_2500:
			case NTV2_FRAMERATE_2400:
			case NTV2_FRAMERATE_1500:
				if (scaleInt <= 1500 + 100)
					result = NTV2_FRAMERATE_1500;
				else if (scaleInt <= 2400 + 50)
					result = NTV2_FRAMERATE_2400;
				else if (scaleInt <= 2500 + 100)
					result = NTV2_FRAMERATE_2500;
				else if (scaleInt <= 3000 + 100)
					result = NTV2_FRAMERATE_3000;
				else if (scaleInt <= 4800 + 100)
					result = NTV2_FRAMERATE_4800;
				else if (scaleInt <= 5000 + 100)
					result = NTV2_FRAMERATE_5000;
				else if (scaleInt <= 6000 + 100)
					result = NTV2_FRAMERATE_6000;
				else
					result = NTV2_FRAMERATE_12000;
				break;

			case NTV2_FRAMERATE_11988:
			case NTV2_FRAMERATE_5994:
			case NTV2_FRAMERATE_4795:
			case NTV2_FRAMERATE_2997:
			case NTV2_FRAMERATE_2398:
			case NTV2_FRAMERATE_1498:
				if (scaleInt <= 1498 + 100)				// add some fudge factor for rounding errors
					result = NTV2_FRAMERATE_1498;
				else if (scaleInt <= 2398 + 100)
					result = NTV2_FRAMERATE_2398;
				else if (scaleInt <= 2997 + 100)
					result = NTV2_FRAMERATE_2997;
				else if (scaleInt <= 4795 + 100)
					result = NTV2_FRAMERATE_4795;
				else if (scaleInt <= 5994 + 100)
					result = NTV2_FRAMERATE_5994;
				else
					result = NTV2_FRAMERATE_11988;
				break;
			default:
				break;
		}
	}
	return result;
}

NTV2FrameRate GetNTV2FrameRateFromVideoFormat(NTV2VideoFormat videoFormat)
{
    NTV2FrameRate frameRate = NTV2_FRAMERATE_UNKNOWN;
	switch ( videoFormat )
	{
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080psf_2500_2:
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_625psf_2500:
	case NTV2_FORMAT_720p_2500:
	case NTV2_FORMAT_1080psf_2K_2500:
	case NTV2_FORMAT_1080p_2K_2500:
	case NTV2_FORMAT_4x1920x1080psf_2500:
	case NTV2_FORMAT_4x1920x1080p_2500:
	case NTV2_FORMAT_4x2048x1080psf_2500:
	case NTV2_FORMAT_4x2048x1080p_2500:
    case NTV2_FORMAT_3840x2160psf_2500:
    case NTV2_FORMAT_4096x2160psf_2500:
    case NTV2_FORMAT_3840x2160p_2500:
    case NTV2_FORMAT_4096x2160p_2500:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_1080p_2K_5000_B:
	case NTV2_FORMAT_3840x2160p_5000_B:
	case NTV2_FORMAT_4096x2160p_5000_B:
	case NTV2_FORMAT_4x1920x1080p_5000_B:
	case NTV2_FORMAT_4x2048x1080p_5000_B:
		frameRate = NTV2_FRAMERATE_2500;
		break;

	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_1080p_2K_5994_A:
	case NTV2_FORMAT_4x1920x1080p_5994:
	case NTV2_FORMAT_4x2048x1080p_5994:
    case NTV2_FORMAT_3840x2160p_5994:
    case NTV2_FORMAT_4096x2160p_5994:
		frameRate = NTV2_FRAMERATE_5994;
		break;

	case NTV2_FORMAT_720p_6000:
	case NTV2_FORMAT_1080p_6000_A:
	case NTV2_FORMAT_1080p_2K_6000_A:
	case NTV2_FORMAT_4x1920x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_6000:
    case NTV2_FORMAT_3840x2160p_6000:
    case NTV2_FORMAT_4096x2160p_6000:
		frameRate = NTV2_FRAMERATE_6000;
		break;

	case NTV2_FORMAT_1080i_6000:
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080p_2K_3000:
	case NTV2_FORMAT_4x1920x1080p_3000:
	case NTV2_FORMAT_4x1920x1080psf_3000:
	case NTV2_FORMAT_4x2048x1080p_3000:
	case NTV2_FORMAT_4x2048x1080psf_3000:
    case NTV2_FORMAT_3840x2160p_3000:
    case NTV2_FORMAT_4096x2160p_3000:
    case NTV2_FORMAT_3840x2160psf_3000:
    case NTV2_FORMAT_4096x2160psf_3000:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_1080p_2K_6000_B:
	case NTV2_FORMAT_3840x2160p_6000_B:
	case NTV2_FORMAT_4096x2160p_6000_B:
	case NTV2_FORMAT_4x1920x1080p_6000_B:
	case NTV2_FORMAT_4x2048x1080p_6000_B:
		frameRate = NTV2_FRAMERATE_3000;
		break;

	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_1080p_2K_5000_A:
	case NTV2_FORMAT_4x1920x1080p_5000:
	case NTV2_FORMAT_4x2048x1080p_5000:
    case NTV2_FORMAT_3840x2160p_5000:
    case NTV2_FORMAT_4096x2160p_5000:
		frameRate = NTV2_FRAMERATE_5000;
		break;

	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_525psf_2997:
	case NTV2_FORMAT_1080p_2K_2997:
	case NTV2_FORMAT_4x1920x1080p_2997:
	case NTV2_FORMAT_4x1920x1080psf_2997:
	case NTV2_FORMAT_4x2048x1080p_2997:
	case NTV2_FORMAT_4x2048x1080psf_2997:
    case NTV2_FORMAT_3840x2160p_2997:
    case NTV2_FORMAT_4096x2160p_2997:
    case NTV2_FORMAT_3840x2160psf_2997:
    case NTV2_FORMAT_4096x2160psf_2997:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_1080p_2K_5994_B:
	case NTV2_FORMAT_3840x2160p_5994_B:
	case NTV2_FORMAT_4096x2160p_5994_B:
	case NTV2_FORMAT_4x1920x1080p_5994_B:
	case NTV2_FORMAT_4x2048x1080p_5994_B:
		frameRate = NTV2_FRAMERATE_2997;
		break;

	case NTV2_FORMAT_525_2398:
	case NTV2_FORMAT_720p_2398:
	case NTV2_FORMAT_1080psf_2K_2398:
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2K_2398:
	case NTV2_FORMAT_4x1920x1080psf_2398:
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_4x2048x1080psf_2398:
	case NTV2_FORMAT_4x2048x1080p_2398:
    case NTV2_FORMAT_3840x2160p_2398:
    case NTV2_FORMAT_4096x2160p_2398:
    case NTV2_FORMAT_3840x2160psf_2398:
    case NTV2_FORMAT_4096x2160psf_2398:
		frameRate = NTV2_FRAMERATE_2398;
		break;

	case NTV2_FORMAT_525_2400:
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2K_2400:
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2K_2400:
	case NTV2_FORMAT_4x1920x1080psf_2400:
	case NTV2_FORMAT_4x1920x1080p_2400:
	case NTV2_FORMAT_4x2048x1080psf_2400:
	case NTV2_FORMAT_4x2048x1080p_2400:
    case NTV2_FORMAT_3840x2160p_2400:
    case NTV2_FORMAT_4096x2160p_2400:
    case NTV2_FORMAT_3840x2160psf_2400:
    case NTV2_FORMAT_4096x2160psf_2400:
		frameRate = NTV2_FRAMERATE_2400;
		break;
	case NTV2_FORMAT_2K_1498:
		frameRate = NTV2_FRAMERATE_1498;
		break;
	case NTV2_FORMAT_2K_1500:
		frameRate = NTV2_FRAMERATE_1500;
		break;
	case NTV2_FORMAT_2K_2398:
		frameRate = NTV2_FRAMERATE_2398;
		break;
	case NTV2_FORMAT_2K_2400:
		frameRate = NTV2_FRAMERATE_2400;
		break;
	case NTV2_FORMAT_2K_2500:
		frameRate = NTV2_FRAMERATE_2500;
		break;

	case NTV2_FORMAT_1080p_2K_4795_A:
	case NTV2_FORMAT_1080p_2K_4795_B:
	case NTV2_FORMAT_4x2048x1080p_4795:
    case NTV2_FORMAT_4096x2160p_4795:
		frameRate = NTV2_FRAMERATE_4795;
		break;
	case NTV2_FORMAT_1080p_2K_4800_A:
	case NTV2_FORMAT_1080p_2K_4800_B:
	case NTV2_FORMAT_4x2048x1080p_4800:
    case NTV2_FORMAT_4096x2160p_4800:
		frameRate = NTV2_FRAMERATE_4800;
		break;

	case NTV2_FORMAT_4x2048x1080p_11988:
    case NTV2_FORMAT_4096x2160p_11988:
		frameRate = NTV2_FRAMERATE_11988;
		break;
	case NTV2_FORMAT_4x2048x1080p_12000:
    case NTV2_FORMAT_4096x2160p_12000:
		frameRate = NTV2_FRAMERATE_12000;
		break;

#if defined (_DEBUG)
	//	Debug builds warn about missing values
	case NTV2_FORMAT_UNKNOWN:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
	case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
	case NTV2_FORMAT_END_2K_DEF_FORMATS:
	case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
	case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
	case NTV2_FORMAT_END_4K_DEF_FORMATS2:
	case NTV2_FORMAT_4096x2160p_4795_B:
	case NTV2_FORMAT_4096x2160p_4800_B:
	case NTV2_FORMAT_4x2048x1080p_4795_B:
	case NTV2_FORMAT_4x2048x1080p_4800_B:
		break;
#else
	default:
		break;	// Unsupported -- fail
#endif
	}

	return frameRate;

}	//	GetNTV2FrameRateFromVideoFormat


NTV2FrameGeometry GetNormalizedFrameGeometry (const NTV2FrameGeometry inFrameGeometry)
{
	switch (inFrameGeometry)
	{
		case NTV2_FG_1920x1080:		//	1080i, 1080p
		case NTV2_FG_1280x720:		//	720p
		case NTV2_FG_720x486:		//	ntsc 525i, 525p60
		case NTV2_FG_720x576:		//	pal 625i
		case NTV2_FG_2048x1080:		//	2k1080p
		case NTV2_FG_2048x1556:		//	2k1556psf
		case NTV2_FG_4x1920x1080:	//	UHD
		case NTV2_FG_4x2048x1080:	//	4K
			return inFrameGeometry;	//	No change, already normalized

															//	525i
		case NTV2_FG_720x508:	return NTV2_FG_720x486;		//	720x486 + tall vanc
		case NTV2_FG_720x514:	return NTV2_FG_720x486;		//	720x486 + taller vanc (extra-wide ntsc)

															//	625i
		case NTV2_FG_720x598:	return NTV2_FG_720x576;		//	pal 625i + tall vanc
		case NTV2_FG_720x612:	return NTV2_FG_720x576;		//	720x576 + taller vanc (extra-wide pal)

															//	720p
		case NTV2_FG_1280x740:	return NTV2_FG_1280x720;	//	1280x720 + tall vanc

															//	1080
		case NTV2_FG_1920x1112:	return NTV2_FG_1920x1080;	//	1920x1080 + tall vanc
		case NTV2_FG_1920x1114:	return NTV2_FG_1920x1080;	//	1920x1080 + taller vanc

															//	2kx1080
		case NTV2_FG_2048x1112:	return NTV2_FG_2048x1080;	//	2048x1080 + tall vanc
		case NTV2_FG_2048x1114:	return NTV2_FG_2048x1080;	//	2048x1080 + taller vanc

															//	2kx1556 film
		case NTV2_FG_2048x1588:	return NTV2_FG_2048x1556;	//	2048x1556 + tall vanc

#if defined (_DEBUG)
		case NTV2_FG_INVALID:	break;
#else
		default:				break;
#endif
	}
	return NTV2_FG_INVALID;	//	fail
}


NTV2FrameGeometry GetVANCFrameGeometry (const NTV2FrameGeometry inFrameGeometry, const NTV2VANCMode inVancMode)
{
	if (!NTV2_IS_VALID_VANCMODE(inVancMode))
		return NTV2_FG_INVALID;	//	Invalid vanc mode
	if (!NTV2_IS_VALID_NTV2FrameGeometry(inFrameGeometry))
		return NTV2_FG_INVALID;	//	Invalid FG
	if (!NTV2_IS_VANCMODE_ON(inVancMode))
		return ::GetNormalizedFrameGeometry(inFrameGeometry);	//	Return normalized

	switch (inFrameGeometry)
	{
		case NTV2_FG_1920x1080:	//	1920x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_1920x1112:	//	1920x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_1920x1114:	//	1920x1080 ::NTV2_VANCMODE_TALLER
			return NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_1920x1112 : NTV2_FG_1920x1114;

		case NTV2_FG_1280x720:	//	1280x720, ::NTV2_VANCMODE_OFF
		case NTV2_FG_1280x740:	//	1280x720 ::NTV2_VANCMODE_TALL
			return NTV2_FG_1280x740;

		case NTV2_FG_720x486:	//	720x486 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x508:	//	720x486 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x514: 	//	720x486 ::NTV2_VANCMODE_TALLER
			return NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_720x508 : NTV2_FG_720x514;

		case NTV2_FG_720x576:	//	720x576 ::NTV2_VANCMODE_OFF
		case NTV2_FG_720x598:	//	720x576 ::NTV2_VANCMODE_TALL
		case NTV2_FG_720x612: 	//	720x576 ::NTV2_VANCMODE_TALLER
			return NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_720x598 : NTV2_FG_720x612;

		case NTV2_FG_2048x1080:	//	2048x1080 ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1112: //	2048x1080 ::NTV2_VANCMODE_TALL
		case NTV2_FG_2048x1114:	//	2048x1080 ::NTV2_VANCMODE_TALLER
			return NTV2_IS_VANCMODE_TALL(inVancMode) ? NTV2_FG_2048x1112 : NTV2_FG_2048x1114;

		case NTV2_FG_2048x1556:	//	2048x1556 film ::NTV2_VANCMODE_OFF
		case NTV2_FG_2048x1588: //	2048x1556 film ::NTV2_VANCMODE_TALL
			return NTV2_FG_2048x1588;

		case NTV2_FG_4x1920x1080:	//	3840x2160 ::NTV2_VANCMODE_OFF
		case NTV2_FG_4x2048x1080:	//	4096x2160 ::NTV2_VANCMODE_OFF
			return inFrameGeometry;	//	no tall or taller geometries!
#if defined (_DEBUG)
		case NTV2_FG_INVALID:	break;
#else
		default:				break;
#endif
	}
	return NTV2_FG_INVALID;	//	fail
}


NTV2FrameGeometry GetGeometryFromStandard (const NTV2Standard inStandard)
{
	switch (inStandard)
	{
		case NTV2_STANDARD_720:			return NTV2_FG_1280x720;	//	720p
		case NTV2_STANDARD_525:			return NTV2_FG_720x486;		//	525i
		case NTV2_STANDARD_625:			return NTV2_FG_720x576;		//	625i

		case NTV2_STANDARD_1080:
		case NTV2_STANDARD_1080p:		return NTV2_FG_1920x1080;	//	1080i, 1080psf, 1080p

		case NTV2_STANDARD_2K:			return NTV2_FG_2048x1556;	//	2048x1556 film

		case NTV2_STANDARD_2Kx1080p:
		case NTV2_STANDARD_2Kx1080i:	return NTV2_FG_2048x1080;	//	2K1080p/i/psf

		case NTV2_STANDARD_3840x2160p:								//	UHD
		case NTV2_STANDARD_3840HFR:		return NTV2_FG_4x1920x1080;	//	HFR UHD

		case NTV2_STANDARD_4096x2160p:								//	4K
		case NTV2_STANDARD_4096HFR:		return NTV2_FG_4x2048x1080;	//	HFR 4K
#if defined (_DEBUG)
		case NTV2_STANDARD_INVALID:	break;
#else
		default:					break;
#endif
	}
	return NTV2_FG_INVALID;
}


bool NTV2DeviceCanDoFormat(NTV2DeviceID		inDeviceID,
						  NTV2FrameRate		inFrameRate,
  			              NTV2FrameGeometry inFrameGeometry,
						  NTV2Standard		inStandard)
{
	//	This implementation is very inefficient, but...
	//	a)	this function is deprecated;
	//	b)	nobody should be calling it (they should be calling NTV2DeviceCanDoVideoFormat instead)
	//	c)	they shouldn't be calling it every frame.
	//	We could make it efficient by creating a static global rate/geometry/standard-to-videoFormat
	//	map, but that has race/deadlock issues.

	const NTV2FrameGeometry	fg	(::GetNormalizedFrameGeometry(inFrameGeometry));
	//	Look for a video format that matches the given frame rate, geometry and standard...
	for (NTV2VideoFormat vFmt((NTV2VideoFormat)NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT);  vFmt < NTV2_MAX_NUM_VIDEO_FORMATS;  vFmt = NTV2VideoFormat(vFmt+1))
	{
		if (!NTV2_IS_VALID_VIDEO_FORMAT(vFmt))
			continue;
		const NTV2FrameRate		fr	(::GetNTV2FrameRateFromVideoFormat(vFmt));
		const NTV2Standard		std	(::GetNTV2StandardFromVideoFormat(vFmt));
		const NTV2FrameGeometry	geo	(::GetNTV2FrameGeometryFromVideoFormat(vFmt));
		if (fr == inFrameRate  &&  std == inStandard  &&  fg == geo)
			return ::NTV2DeviceCanDoVideoFormat(inDeviceID, vFmt);
	}
	return false;
}

ULWord GetNTV2FrameGeometryHeight(NTV2FrameGeometry geometry)
{
	switch (geometry)
	{
		case NTV2_FG_1920x1080:		return 1080;
		case NTV2_FG_1280x720:		return 720;
		case NTV2_FG_720x486:		return 486;
		case NTV2_FG_720x576:		return 576;
		case NTV2_FG_720x508:		return 508;
		case NTV2_FG_720x598:		return 598;
		case NTV2_FG_1920x1112:		return 1112;
		case NTV2_FG_1920x1114:		return 1114;
		case NTV2_FG_1280x740:		return 740;
		case NTV2_FG_2048x1080:		return 1080;
        case NTV2_FG_2048x1556:		return 1556;
		case NTV2_FG_2048x1588:		return 1588;
		case NTV2_FG_2048x1112:		return 1112;
		case NTV2_FG_2048x1114:		return 1114;
		case NTV2_FG_720x514:		return 514;
		case NTV2_FG_720x612:		return 612;
		case NTV2_FG_4x1920x1080:	return 2160;
		case NTV2_FG_4x2048x1080:	return 2160;
        default:					return 0;
	}
}

ULWord GetNTV2FrameGeometryWidth(NTV2FrameGeometry geometry)
{
	switch ( geometry )
	{
        case NTV2_FG_720x486:
        case NTV2_FG_720x576:
        case NTV2_FG_720x508:
        case NTV2_FG_720x598:
		case NTV2_FG_720x514:
		case NTV2_FG_720x612:
			return 720;
        case NTV2_FG_1280x720:
        case NTV2_FG_1280x740:
			return 1280;
        case NTV2_FG_1920x1080:
        case NTV2_FG_1920x1112:
		case NTV2_FG_1920x1114:
			return 1920;
        case NTV2_FG_2048x1080:
		case NTV2_FG_2048x1112:
		case NTV2_FG_2048x1114:
        case NTV2_FG_2048x1556:
		case NTV2_FG_2048x1588:
			return 2048;
		case NTV2_FG_4x1920x1080:
			return 3840;
		case NTV2_FG_4x2048x1080:
			return 4096;
        default:
			return 0;
	}
}


//	Displayable width of format, not counting HANC/VANC
ULWord GetDisplayWidth (const NTV2VideoFormat videoFormat)
{
	ULWord width = 0;

	switch (videoFormat)
	{
		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_525_5994:
		case NTV2_FORMAT_525psf_2997:
		case NTV2_FORMAT_625_5000:
		case NTV2_FORMAT_625psf_2500:
			width = 720;
			break;
		case NTV2_FORMAT_720p_5000:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_720p_2500:
			width = 1280;
			break;
		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_1080i_5994:
		case NTV2_FORMAT_1080i_6000:
		case NTV2_FORMAT_1080psf_2398:
		case NTV2_FORMAT_1080psf_2400:
		case NTV2_FORMAT_1080psf_2500_2:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_1080p_2997:
		case NTV2_FORMAT_1080p_3000:
		case NTV2_FORMAT_1080p_2500:
		case NTV2_FORMAT_1080p_2398:
		case NTV2_FORMAT_1080p_2400:
		case NTV2_FORMAT_1080p_5000_A:
		case NTV2_FORMAT_1080p_5994_A:
		case NTV2_FORMAT_1080p_6000_A:
		case NTV2_FORMAT_1080p_5000_B:
		case NTV2_FORMAT_1080p_5994_B:
		case NTV2_FORMAT_1080p_6000_B:
			width = 1920;
			break;
		case NTV2_FORMAT_1080p_2K_2398:
		case NTV2_FORMAT_1080p_2K_2400:
		case NTV2_FORMAT_1080p_2K_2500:
		case NTV2_FORMAT_1080p_2K_2997:
		case NTV2_FORMAT_1080p_2K_3000:
		case NTV2_FORMAT_1080p_2K_4795_A:
		case NTV2_FORMAT_1080p_2K_4800_A:
		case NTV2_FORMAT_1080p_2K_5000_A:
		case NTV2_FORMAT_1080p_2K_5994_A:
		case NTV2_FORMAT_1080p_2K_6000_A:
		case NTV2_FORMAT_1080p_2K_4795_B:
		case NTV2_FORMAT_1080p_2K_4800_B:
		case NTV2_FORMAT_1080p_2K_5000_B:
		case NTV2_FORMAT_1080p_2K_5994_B:
		case NTV2_FORMAT_1080p_2K_6000_B:
		case NTV2_FORMAT_1080psf_2K_2398:
		case NTV2_FORMAT_1080psf_2K_2400:
		case NTV2_FORMAT_1080psf_2K_2500:
		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:
			width = 2048;
			break;
		case NTV2_FORMAT_4x1920x1080psf_2398:
		case NTV2_FORMAT_4x1920x1080psf_2400:
		case NTV2_FORMAT_4x1920x1080psf_2500:
		case NTV2_FORMAT_4x1920x1080psf_2997:
		case NTV2_FORMAT_4x1920x1080psf_3000:
		case NTV2_FORMAT_4x1920x1080p_2398:
		case NTV2_FORMAT_4x1920x1080p_2400:
		case NTV2_FORMAT_4x1920x1080p_2500:
		case NTV2_FORMAT_4x1920x1080p_2997:
		case NTV2_FORMAT_4x1920x1080p_3000:
		case NTV2_FORMAT_4x1920x1080p_5000:
		case NTV2_FORMAT_4x1920x1080p_5994:
		case NTV2_FORMAT_4x1920x1080p_6000:
        case NTV2_FORMAT_3840x2160psf_2398:
        case NTV2_FORMAT_3840x2160psf_2400:
        case NTV2_FORMAT_3840x2160psf_2500:
        case NTV2_FORMAT_3840x2160p_2398:
        case NTV2_FORMAT_3840x2160p_2400:
        case NTV2_FORMAT_3840x2160p_2500:
        case NTV2_FORMAT_3840x2160p_2997:
        case NTV2_FORMAT_3840x2160p_3000:
        case NTV2_FORMAT_3840x2160psf_2997:
        case NTV2_FORMAT_3840x2160psf_3000:
        case NTV2_FORMAT_3840x2160p_5000:
        case NTV2_FORMAT_3840x2160p_5994:
        case NTV2_FORMAT_3840x2160p_6000:
		case NTV2_FORMAT_3840x2160p_5000_B:
		case NTV2_FORMAT_3840x2160p_5994_B:
		case NTV2_FORMAT_3840x2160p_6000_B:
		case NTV2_FORMAT_4x1920x1080p_5000_B:
		case NTV2_FORMAT_4x1920x1080p_5994_B:
		case NTV2_FORMAT_4x1920x1080p_6000_B:
			width = 3840;
			break;
		case NTV2_FORMAT_4x2048x1080psf_2398:
		case NTV2_FORMAT_4x2048x1080psf_2400:
		case NTV2_FORMAT_4x2048x1080psf_2500:
		case NTV2_FORMAT_4x2048x1080psf_2997:
		case NTV2_FORMAT_4x2048x1080psf_3000:
		case NTV2_FORMAT_4x2048x1080p_2398:
		case NTV2_FORMAT_4x2048x1080p_2400:
		case NTV2_FORMAT_4x2048x1080p_2500:
		case NTV2_FORMAT_4x2048x1080p_2997:
		case NTV2_FORMAT_4x2048x1080p_3000:
		case NTV2_FORMAT_4x2048x1080p_4795:
		case NTV2_FORMAT_4x2048x1080p_4800:
		case NTV2_FORMAT_4x2048x1080p_5000:
		case NTV2_FORMAT_4x2048x1080p_5994:
		case NTV2_FORMAT_4x2048x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_11988:
		case NTV2_FORMAT_4x2048x1080p_12000:
        case NTV2_FORMAT_4096x2160psf_2398:
        case NTV2_FORMAT_4096x2160psf_2400:
        case NTV2_FORMAT_4096x2160psf_2500:
        case NTV2_FORMAT_4096x2160p_2398:
        case NTV2_FORMAT_4096x2160p_2400:
        case NTV2_FORMAT_4096x2160p_2500:
        case NTV2_FORMAT_4096x2160p_2997:
        case NTV2_FORMAT_4096x2160p_3000:
        case NTV2_FORMAT_4096x2160psf_2997:
        case NTV2_FORMAT_4096x2160psf_3000:
        case NTV2_FORMAT_4096x2160p_4795:
        case NTV2_FORMAT_4096x2160p_4800:
        case NTV2_FORMAT_4096x2160p_5000:
        case NTV2_FORMAT_4096x2160p_5994:
        case NTV2_FORMAT_4096x2160p_6000:
        case NTV2_FORMAT_4096x2160p_11988:
        case NTV2_FORMAT_4096x2160p_12000:
		case NTV2_FORMAT_4096x2160p_4795_B:
		case NTV2_FORMAT_4096x2160p_4800_B:
		case NTV2_FORMAT_4096x2160p_5000_B:
		case NTV2_FORMAT_4096x2160p_5994_B:
		case NTV2_FORMAT_4096x2160p_6000_B:
		case NTV2_FORMAT_4x2048x1080p_4795_B:
		case NTV2_FORMAT_4x2048x1080p_4800_B:
		case NTV2_FORMAT_4x2048x1080p_5000_B:
		case NTV2_FORMAT_4x2048x1080p_5994_B:
		case NTV2_FORMAT_4x2048x1080p_6000_B:
			width = 4096;
			break;
#if defined(_DEBUG)
		case NTV2_FORMAT_UNKNOWN:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
		case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
		case NTV2_FORMAT_END_2K_DEF_FORMATS:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
		case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
		case NTV2_FORMAT_END_4K_DEF_FORMATS2:
#else
		default:
#endif
			break;
	}

	return width;

}	//	GetDisplayWidth


//	Displayable height of format, not counting HANC/VANC
ULWord GetDisplayHeight (const NTV2VideoFormat videoFormat)
{
	ULWord height = 0;

	switch (videoFormat)
	{
		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_525_5994:
		case NTV2_FORMAT_525psf_2997:
			height = 486;
			break;
		case NTV2_FORMAT_625_5000:
		case NTV2_FORMAT_625psf_2500:
			height = 576;
			break;
		case NTV2_FORMAT_720p_5000:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_720p_2500:
			height = 720;
			break;
		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_1080i_5994:
		case NTV2_FORMAT_1080i_6000:
		case NTV2_FORMAT_1080psf_2398:
		case NTV2_FORMAT_1080psf_2400:
		case NTV2_FORMAT_1080psf_2500_2:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_1080p_2997:
		case NTV2_FORMAT_1080p_3000:
		case NTV2_FORMAT_1080p_2500:
		case NTV2_FORMAT_1080p_2398:
		case NTV2_FORMAT_1080p_2400:
		case NTV2_FORMAT_1080p_5000_A:
		case NTV2_FORMAT_1080p_5994_A:
		case NTV2_FORMAT_1080p_6000_A:
		case NTV2_FORMAT_1080p_5000_B:
		case NTV2_FORMAT_1080p_5994_B:
		case NTV2_FORMAT_1080p_6000_B:
		case NTV2_FORMAT_1080p_2K_2398:
		case NTV2_FORMAT_1080p_2K_2400:
		case NTV2_FORMAT_1080p_2K_2500:
		case NTV2_FORMAT_1080p_2K_2997:
		case NTV2_FORMAT_1080p_2K_3000:
		case NTV2_FORMAT_1080p_2K_4795_A:
		case NTV2_FORMAT_1080p_2K_4800_A:
		case NTV2_FORMAT_1080p_2K_5000_A:
		case NTV2_FORMAT_1080p_2K_5994_A:
		case NTV2_FORMAT_1080p_2K_6000_A:
		case NTV2_FORMAT_1080p_2K_4795_B:
		case NTV2_FORMAT_1080p_2K_4800_B:
		case NTV2_FORMAT_1080p_2K_5000_B:
		case NTV2_FORMAT_1080p_2K_5994_B:
		case NTV2_FORMAT_1080p_2K_6000_B:
		case NTV2_FORMAT_1080psf_2K_2398:
		case NTV2_FORMAT_1080psf_2K_2400:
		case NTV2_FORMAT_1080psf_2K_2500:
			height = 1080;
			break;
		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:
			height = 1556;
			break;
		case NTV2_FORMAT_4x1920x1080psf_2398:
		case NTV2_FORMAT_4x1920x1080psf_2400:
		case NTV2_FORMAT_4x1920x1080psf_2500:
		case NTV2_FORMAT_4x1920x1080psf_2997:
		case NTV2_FORMAT_4x1920x1080psf_3000:
		case NTV2_FORMAT_4x1920x1080p_2398:
		case NTV2_FORMAT_4x1920x1080p_2400:
		case NTV2_FORMAT_4x1920x1080p_2500:
		case NTV2_FORMAT_4x1920x1080p_2997:
		case NTV2_FORMAT_4x1920x1080p_3000:
		case NTV2_FORMAT_4x2048x1080psf_2398:
		case NTV2_FORMAT_4x2048x1080psf_2400:
		case NTV2_FORMAT_4x2048x1080psf_2500:
		case NTV2_FORMAT_4x2048x1080psf_2997:
		case NTV2_FORMAT_4x2048x1080psf_3000:
		case NTV2_FORMAT_4x2048x1080p_2398:
		case NTV2_FORMAT_4x2048x1080p_2400:
		case NTV2_FORMAT_4x2048x1080p_2500:
		case NTV2_FORMAT_4x2048x1080p_2997:
		case NTV2_FORMAT_4x2048x1080p_3000:
		case NTV2_FORMAT_4x1920x1080p_5000:
		case NTV2_FORMAT_4x1920x1080p_5994:
		case NTV2_FORMAT_4x1920x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_4795:
		case NTV2_FORMAT_4x2048x1080p_4800:
		case NTV2_FORMAT_4x2048x1080p_5000:
		case NTV2_FORMAT_4x2048x1080p_5994:
		case NTV2_FORMAT_4x2048x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_11988:
		case NTV2_FORMAT_4x2048x1080p_12000:
        case NTV2_FORMAT_3840x2160psf_2398:
        case NTV2_FORMAT_3840x2160psf_2400:
        case NTV2_FORMAT_3840x2160psf_2500:
        case NTV2_FORMAT_3840x2160p_2398:
        case NTV2_FORMAT_3840x2160p_2400:
        case NTV2_FORMAT_3840x2160p_2500:
        case NTV2_FORMAT_3840x2160p_2997:
        case NTV2_FORMAT_3840x2160p_3000:
        case NTV2_FORMAT_3840x2160psf_2997:
        case NTV2_FORMAT_3840x2160psf_3000:
        case NTV2_FORMAT_3840x2160p_5000:
        case NTV2_FORMAT_3840x2160p_5994:
        case NTV2_FORMAT_3840x2160p_6000:
        case NTV2_FORMAT_4096x2160psf_2398:
        case NTV2_FORMAT_4096x2160psf_2400:
        case NTV2_FORMAT_4096x2160psf_2500:
        case NTV2_FORMAT_4096x2160p_2398:
        case NTV2_FORMAT_4096x2160p_2400:
        case NTV2_FORMAT_4096x2160p_2500:
        case NTV2_FORMAT_4096x2160p_2997:
        case NTV2_FORMAT_4096x2160p_3000:
        case NTV2_FORMAT_4096x2160psf_2997:
        case NTV2_FORMAT_4096x2160psf_3000:
        case NTV2_FORMAT_4096x2160p_4795:
        case NTV2_FORMAT_4096x2160p_4800:
        case NTV2_FORMAT_4096x2160p_5000:
        case NTV2_FORMAT_4096x2160p_5994:
        case NTV2_FORMAT_4096x2160p_6000:
        case NTV2_FORMAT_4096x2160p_11988:
        case NTV2_FORMAT_4096x2160p_12000:
		case NTV2_FORMAT_3840x2160p_5000_B:
		case NTV2_FORMAT_3840x2160p_5994_B:
		case NTV2_FORMAT_3840x2160p_6000_B:
		case NTV2_FORMAT_4096x2160p_4795_B:
		case NTV2_FORMAT_4096x2160p_4800_B:
		case NTV2_FORMAT_4096x2160p_5000_B:
		case NTV2_FORMAT_4096x2160p_5994_B:
		case NTV2_FORMAT_4096x2160p_6000_B:
		case NTV2_FORMAT_4x1920x1080p_5000_B:
		case NTV2_FORMAT_4x1920x1080p_5994_B:
		case NTV2_FORMAT_4x1920x1080p_6000_B:
		case NTV2_FORMAT_4x2048x1080p_4795_B:
		case NTV2_FORMAT_4x2048x1080p_4800_B:
		case NTV2_FORMAT_4x2048x1080p_5000_B:
		case NTV2_FORMAT_4x2048x1080p_5994_B:
		case NTV2_FORMAT_4x2048x1080p_6000_B:
			height = 2160;
			break;
#if defined(_DEBUG)
		case NTV2_FORMAT_UNKNOWN:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
		case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
		case NTV2_FORMAT_END_2K_DEF_FORMATS:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS2:
		case NTV2_FORMAT_END_4K_TSI_DEF_FORMATS:
		case NTV2_FORMAT_END_4K_DEF_FORMATS2:
#else
		default:
#endif
			break;
	}

	return height;

}	//	GetDisplayHeight


//	NTV2SmpteLineNumber::NTV2SmpteLineNumber (const NTV2Standard inStandard)
//	IMPLEMENTATION MOVED INTO 'ntv2formatdescriptor.cpp'
//	SO AS TO USE SAME LineNumbersF1/LineNumbersF2 TABLES


ULWord NTV2SmpteLineNumber::GetFirstActiveLine (const NTV2FieldID inFieldID) const
{
	if (!NTV2_IS_VALID_FIELD (inFieldID))
		return 0;

	if (inFieldID == NTV2_FIELD0)
		return firstFieldTop ? smpteFirstActiveLine : smpteSecondActiveLine;
	else
		return firstFieldTop ? smpteSecondActiveLine : smpteFirstActiveLine;
}


ostream & NTV2SmpteLineNumber::Print (ostream & inOutStream) const
{
	if (!IsValid ())
		inOutStream << "INVALID ";
	inOutStream	<< "SMPTELineNumber(";
	if (IsValid ())
		inOutStream	<< "1st=" << smpteFirstActiveLine << (firstFieldTop ? "(top)" : "")
				<< ", 2nd=" << smpteSecondActiveLine << (firstFieldTop ? "" : "(top)")
				<< ", std=" << ::NTV2StandardToString (mStandard) << ")";
	else
		inOutStream	<< "INVALID)";
	return inOutStream;
}


string NTV2SmpteLineNumber::PrintLineNumber (const ULWord inLineOffset, const NTV2FieldID inRasterFieldID) const
{
	ostringstream	oss;
	if (NTV2_IS_VALID_FIELD (inRasterFieldID) && !NTV2_IS_PROGRESSIVE_STANDARD (mStandard))
		oss << "F" << (inRasterFieldID == 0 ? "1" : "2") << " ";
	oss << "L" << dec << inLineOffset+GetFirstActiveLine(inRasterFieldID);
	return oss.str();
}

#if !defined (NTV2_DEPRECATE)
	AJA_LOCAL_STATIC const char * NTV2VideoFormatStrings [NTV2_MAX_NUM_VIDEO_FORMATS] =
	{
		"",							//	NTV2_FORMAT_UNKNOWN						//	0		//	not used
		"1080i 50.00",				//	NTV2_FORMAT_1080i_5000					//	1
		"1080i 59.94",				//	NTV2_FORMAT_1080i_5994					//	2
		"1080i 60.00",				//	NTV2_FORMAT_1080i_6000					//	3
		"720p 59.94",				//	NTV2_FORMAT_720p_5994					//	4
		"720p 60.00",				//	NTV2_FORMAT_720p_6000					//	5
		"1080psf 23.98",			//	NTV2_FORMAT_1080psf_2398				//	6
		"1080psf 24.00",			//	NTV2_FORMAT_1080psf_2400				//	7
		"1080p 29.97",				//	NTV2_FORMAT_1080p_2997					//	8
		"1080p 30.00",				//	NTV2_FORMAT_1080p_3000					//	9
		"1080p 25.00",				//	NTV2_FORMAT_1080p_2500					//	10
		"1080p 23.98",				//	NTV2_FORMAT_1080p_2398					//	11
		"1080p 24.00",				//	NTV2_FORMAT_1080p_2400					//	12
		"2048x1080p 23.98",			//	NTV2_FORMAT_1080p_2K_2398				//	13
		"2048x1080p 24.00",			//	NTV2_FORMAT_1080p_2K_2400				//	14
		"2048x1080psf 23.98",       //	NTV2_FORMAT_1080psf_2K_2398				//	15
		"2048x1080psf 24.00",		//	NTV2_FORMAT_1080psf_2K_2400				//	16
		"720p 50",					//	NTV2_FORMAT_720p_5000					//	17
		"1080p 50.00b",				//	NTV2_FORMAT_1080p_5000					//	18
		"1080p 59.94b",				//	NTV2_FORMAT_1080p_5994					//	19
		"1080p 60.00b",				//	NTV2_FORMAT_1080p_6000					//	20
		"720p 23.98",				//	NTV2_FORMAT_720p_2398					//	21
		"720p 25.00",				//	NTV2_FORMAT_720p_2500					//	22
		"1080p 50.00a",				//	NTV2_FORMAT_1080p_5000_A				//	23
		"1080p 59.94a",				//	NTV2_FORMAT_1080p_5994_A				//	24
		"1080p 60.00a",				//	NTV2_FORMAT_1080p_6000_A				//	25
		"2048x1080p 25.00",         //	NTV2_FORMAT_1080p_2K_2500				//	26
		"2048x1080psf 25.00",       //	NTV2_FORMAT_1080psf_2K_2500				//	27
		"1080psf 25",				//	NTV2_FORMAT_1080psf_2500_2				//	28
		"1080psf 29.97",			//	NTV2_FORMAT_1080psf_2997_2				//	29
		"1080psf 30",				//	NTV2_FORMAT_1080psf_3000_2				//	30
		"",							//	NTV2_FORMAT_END_HIGH_DEF_FORMATS		//	31		// not used
		"525i 59.94",				//	NTV2_FORMAT_525_5994					//	32
		"625i 50.00",				//	NTV2_FORMAT_625_5000					//	33
		"525 23.98",				//	NTV2_FORMAT_525_2398					//	34
		"525 24.00",				//	NTV2_FORMAT_525_2400					//	35		// not used
		"525psf 29.97",				//	NTV2_FORMAT_525psf_2997					//	36
		"625psf 25",				//	NTV2_FORMAT_625psf_2500					//	37
		"",							//	NTV2_FORMAT_END_STANDARD_DEF_FORMATS	//	38		// not used
		"",							//											//	39		// not used
		"",							//											//	40		// not used
		"",							//											//	41		// not used
		"",							//											//	42		// not used
		"",							//											//	43		// not used
		"",							//											//	44		// not used
		"",							//											//	45		// not used
		"",							//											//	46		// not used
		"",							//											//	47		// not used
		"",							//											//	48		// not used
		"",							//											//	49		// not used
		"",							//											//	50		// not used
		"",							//											//	51		// not used
		"",							//											//	52		// not used
		"",							//											//	53		// not used
		"",							//											//	54		// not used
		"",							//											//	55		// not used
		"",							//											//	56		// not used
		"",							//											//	57		// not used
		"",							//											//	58		// not used
		"",							//											//	59		// not used
		"",							//											//	60		// not used
		"",							//											//	61		// not used
		"",							//											//	62		// not used
		"",							//											//	63		// not used
		"2048x1556psf 14.98",		//	NTV2_FORMAT_2K_1498						//	64
		"2048x1556psf 15.00",		//	NTV2_FORMAT_2K_1500						//	65
		"2048x1556psf 23.98",		//	NTV2_FORMAT_2K_2398						//	66
		"2048x1556psf 24.00",		//	NTV2_FORMAT_2K_2400						//	67
		"2048x1556psf 25.00",		//	NTV2_FORMAT_2K_2500						//	68
		"",							//	NTV2_FORMAT_END_2K_DEF_FORMATS			//	69		// not used
		"",							//											//	70		// not used
		"",							//											//	71		// not used
		"",							//											//	72		// not used
		"",							//											//	73		// not used
		"",							//											//	74		// not used
		"",							//											//	75		// not used
		"",							//											//	76		// not used
		"",							//											//	77		// not used
		"",							//											//	78		// not used
		"",							//											//	79		// not used
		"4x1920x1080psf 23.98",		//	NTV2_FORMAT_4x1920x1080psf_2398			//	80
		"4x1920x1080psf 24.00",		//	NTV2_FORMAT_4x1920x1080psf_2400			//	81
		"4x1920x1080psf 25.00",		//	NTV2_FORMAT_4x1920x1080psf_2500			//	82
		"4x1920x1080p 23.98",		//	NTV2_FORMAT_4x1920x1080p_2398			//	83
		"4x1920x1080p 24.00",		//	NTV2_FORMAT_4x1920x1080p_2400			//	84
		"4x1920x1080p 25.00",		//	NTV2_FORMAT_4x1920x1080p_2500			//	85
		"4x2048x1080psf 23.98",		//	NTV2_FORMAT_4x2048x1080psf_2398			//	86
		"4x2048x1080psf 24.00",		//	NTV2_FORMAT_4x2048x1080psf_2400			//	87
		"4x2048x1080psf 25.00",		//	NTV2_FORMAT_4x2048x1080psf_2500			//	88
		"4x2048x1080p 23.98",		//	NTV2_FORMAT_4x2048x1080p_2398			//	89
		"4x2048x1080p 24.00",		//	NTV2_FORMAT_4x2048x1080p_2400			//	90
		"4x2048x1080p 25.00",		//	NTV2_FORMAT_4x2048x1080p_2500			//	91
		"4x1920x1080p 29.97",		//	NTV2_FORMAT_4x1920x1080p_2997			//	92
		"4x1920x1080p 30.00",		//	NTV2_FORMAT_4x1920x1080p_3000			//	93
		"4x1920x1080psf 29.97",		//	NTV2_FORMAT_4x1920x1080psf_2997			//	94		//	not supported
		"4x1920x1080psf 30.00",		//	NTV2_FORMAT_4x1920x1080psf_3000			//	95		//	not supported
		"4x2048x1080p 29.97",		//	NTV2_FORMAT_4x2048x1080p_2997			//	96
		"4x2048x1080p 30.00",		//	NTV2_FORMAT_4x2048x1080p_3000			//	97
		"4x2048x1080psf 29.97",		//	NTV2_FORMAT_4x2048x1080psf_2997			//	98		//	not supported
		"4x2048x1080psf 30.00",		//	NTV2_FORMAT_4x2048x1080psf_3000			//	99		//	not supported
		"4x1920x1080p 50.00",		//	NTV2_FORMAT_4x1920x1080p_5000			//	100
		"4x1920x1080p 59.94",		//	NTV2_FORMAT_4x1920x1080p_5994			//	101
		"4x1920x1080p 60.00",		//	NTV2_FORMAT_4x1920x1080p_6000			//	102
		"4x2048x1080p 50.00",		//	NTV2_FORMAT_4x2048x1080p_5000			//	103
		"4x2048x1080p 59.94",		//	NTV2_FORMAT_4x2048x1080p_5994			//	104
		"4x2048x1080p 60.00",		//	NTV2_FORMAT_4x2048x1080p_6000			//	105
		"4x2048x1080p 47.95",		//	NTV2_FORMAT_4x2048x1080p_4795			//	106
		"4x2048x1080p 48.00",		//	NTV2_FORMAT_4x2048x1080p_4800			//	107
		"4x2048x1080p 119.88",		//	NTV2_FORMAT_4x2048x1080p_11988			//	108
		"4x2048x1080p 120.00",		//	NTV2_FORMAT_4x2048x1080p_12000			//	109
		"2048x1080p 60.00a",		//	NTV2_FORMAT_1080p_2K_6000_A				//	110	//	NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT2
		"2048x1080p 59.94a",		//	NTV2_FORMAT_1080p_2K_5994_A				//	111
		"2048x1080p 29.97",			//	NTV2_FORMAT_1080p_2K_2997				//	112
		"2048x1080p 30.00",			//	NTV2_FORMAT_1080p_2K_3000				//	113
		"2048x1080p 50.00a",		//	NTV2_FORMAT_1080p_2K_5000_A				//	114
		"2048x1080p 47.95a",		//	NTV2_FORMAT_1080p_2K_4795_A				//	115
		"2048x1080p 48.00a",		//	NTV2_FORMAT_1080p_2K_4800_A				//	116
		"2048x1080p 47.95b",		// 	NTV2_FORMAT_1080p_2K_4795_B,			// 117
		"2048x1080p 48.00b",		// 	NTV2_FORMAT_1080p_2K_4800_B,			// 118
		"2048x1080p 50.00b",		// 	NTV2_FORMAT_1080p_2K_5000_B,			// 119
		"2048x1080p 59.94b",		// 	NTV2_FORMAT_1080p_2K_5994_B,			// 120
		"2048x1080p 60.00b",		// 	NTV2_FORMAT_1080p_2K_6000_B,			// 121
	};

	AJA_LOCAL_STATIC const char * NTV2VideoStandardStrings [NTV2_NUM_STANDARDS] =
	{
		"1080i",					//	NTV2_STANDARD_1080						//	0
		"720p",						//	NTV2_STANDARD_720						//	1
		"525",						//	NTV2_STANDARD_525						//	2
		"625",						//	NTV2_STANDARD_625						//	3
		"1080p",					//	NTV2_STANDARD_1080p						//	4
		"2k"						//	NTV2_STANDARD_2K						//	5
	};


	AJA_LOCAL_STATIC const char * NTV2PixelFormatStrings [NTV2_FBF_NUMFRAMEBUFFERFORMATS] =
	{
		"10BIT_YCBCR",						//	NTV2_FBF_10BIT_YCBCR			//	0
		"8BIT_YCBCR",						//	NTV2_FBF_8BIT_YCBCR				//	1
		"ARGB",								//	NTV2_FBF_ARGB					//	2
		"RGBA",								//	NTV2_FBF_RGBA					//	3
		"10BIT_RGB",						//	NTV2_FBF_10BIT_RGB				//	4
		"8BIT_YCBCR_YUY2",					//	NTV2_FBF_8BIT_YCBCR_YUY2		//	5
		"ABGR",								//	NTV2_FBF_ABGR					//	6
		"10BIT_DPX",						//	NTV2_FBF_10BIT_DPX				//	7
		"10BIT_YCBCR_DPX",					//	NTV2_FBF_10BIT_YCBCR_DPX		//	8
		"",									//	NTV2_FBF_8BIT_DVCPRO			//	9
		"I420",								//	NTV2_FBF_8BIT_YCBCR_420PL3		//	10
		"",									//	NTV2_FBF_8BIT_HDV				//	11
		"24BIT_RGB",						//	NTV2_FBF_24BIT_RGB				//	12
		"24BIT_BGR",						//	NTV2_FBF_24BIT_BGR				//	13
		"",									//	NTV2_FBF_10BIT_YCBCRA			//	14
        "DPX_LITTLEENDIAN",					//	NTV2_FBF_10BIT_DPX_LE           //	15
		"48BIT_RGB",						//	NTV2_FBF_48BIT_RGB				//	16
		"",									//	NTV2_FBF_PRORES					//	17
		"",									//	NTV2_FBF_PRORES_DVCPRO			//	18
		"",									//	NTV2_FBF_PRORES_HDV				//	19
		"",									//	NTV2_FBF_10BIT_RGB_PACKED		//	20
		"",									//	NTV2_FBF_10BIT_ARGB				//	21
		"",									//	NTV2_FBF_16BIT_ARGB				//	22
		"",									//	NTV2_FBF_8BIT_YCBCR_422PL3		//	23
		"10BIT_RAW_RGB",					//	NTV2_FBF_10BIT_RAW_RGB			//	24
		"10BIT_RAW_YCBCR"					//	NTV2_FBF_10BIT_RAW_YCBCR		//	25
	};
#endif	//	!defined (NTV2_DEPRECATE)



#if !defined (NTV2_DEPRECATE)
	//	More UI-friendly versions of above (used in Cables app)...
	AJA_LOCAL_STATIC const char * frameBufferFormats [NTV2_FBF_NUMFRAMEBUFFERFORMATS+1] =
	{
		"10 Bit YCbCr",						//	NTV2_FBF_10BIT_YCBCR			//	0
		"8 Bit YCbCr - UYVY",				//	NTV2_FBF_8BIT_YCBCR				//	1
		"8 Bit ARGB",						//	NTV2_FBF_ARGB					//	2
		"8 Bit RGBA",						//	NTV2_FBF_RGBA					//	3
		"10 Bit RGB",						//	NTV2_FBF_10BIT_RGB				//	4
		"8 Bit YCbCr - YUY2",				//	NTV2_FBF_8BIT_YCBCR_YUY2		//	5
		"8 Bit ABGR",						//	NTV2_FBF_ABGR					//	6
		"10 Bit RGB - DPX compatible",		//	NTV2_FBF_10BIT_DPX				//	7
		"10 Bit YCbCr - DPX compatible",	//	NTV2_FBF_10BIT_YCBCR_DPX		//	8
		"8 Bit DVCPro YCbCr - UYVY",		//	NTV2_FBF_8BIT_DVCPRO			//	9
		"8 Bit YCbCr 420 3-plane [I420]",	//	NTV2_FBF_8BIT_YCBCR_420PL3		//	10
		"8 Bit HDV YCbCr - UYVY",			//	NTV2_FBF_8BIT_HDV				//	11
		"24 Bit RGB",						//	NTV2_FBF_24BIT_RGB				//	12
		"24 Bit BGR",						//	NTV2_FBF_24BIT_BGR				//	13
		"10 Bit YCbCrA",					//	NTV2_FBF_10BIT_YCBCRA			//	14
		"10 Bit RGB - DPX LE",              //	NTV2_FBF_10BIT_DPX_LE           //	15
		"48 Bit RGB",						//	NTV2_FBF_48BIT_RGB				//	16
		"10 Bit YCbCr - Compressed",		//	NTV2_FBF_PRORES					//	17
		"10 Bit YCbCr DVCPro - Compressed",	//	NTV2_FBF_PRORES_DVCPRO			//	18
		"10 Bit YCbCr HDV - Compressed",	//	NTV2_FBF_PRORES_HDV				//	19
		"10 Bit RGB Packed",				//	NTV2_FBF_10BIT_RGB_PACKED		//	20
		"10 Bit ARGB",						//	NTV2_FBF_10BIT_ARGB				//	21
		"16 Bit ARGB",						//	NTV2_FBF_16BIT_ARGB				//	22
		"8 Bit YCbCr 422 3-plane [Y42B]",	//	NTV2_FBF_8BIT_YCBCR_422PL3		//	23
		"10 Bit Raw RGB",					//	NTV2_FBF_10BIT_RGB				//	24
		"10 Bit Raw YCbCr",					//	NTV2_FBF_10BIT_YCBCR			//	25
		"10 Bit YCbCr 420 3-plane LE",		//	NTV2_FBF_10BIT_YCBCR_420PL3_LE	//	26
		"10 Bit YCbCr 422 3-plane LE",		//	NTV2_FBF_10BIT_YCBCR_422PL3_LE	//	27
		"10 Bit YCbCr 420 2-Plane",			//	NTV2_FBF_10BIT_YCBCR_420PL2		//	28
		"10 Bit YCbCr 422 2-Plane",			//	NTV2_FBF_10BIT_YCBCR_422PL2		//	29
		"8 Bit YCbCr 420 2-Plane",			//	NTV2_FBF_8BIT_YCBCR_420PL2		//	30
		"8 Bit YCbCr 422 2-Plane",			//	NTV2_FBF_8BIT_YCBCR_422PL2		//	31
		""									//	NTV2_FBF_INVALID				//	32
	};
#endif	//	!defined (NTV2_DEPRECATE)


//	More UI-friendly versions of above (used in Cables app)...
AJA_LOCAL_STATIC const char * m31Presets [M31_NUMVIDEOPRESETS] =
{
    "FILE 720x480 420 Planar 8 Bit 59.94i",             // M31_FILE_720X480_420_8_5994i         // 0
    "FILE 720x480 420 Planar 8 Bit 59.94p",             // M31_FILE_720X480_420_8_5994p         // 1
    "FILE 720x480 420 Planar 8 Bit 60i",                // M31_FILE_720X480_420_8_60i           // 2
    "FILE 720x480 420 Planar 8 Bit 60p",                // M31_FILE_720X480_420_8_60p           // 3
    "FILE 720x480 422 Planar 10 Bit 59.94i",            // M31_FILE_720X480_422_10_5994i        // 4
    "FILE 720x480 422 Planar 10 Bit 59.94p",            // M31_FILE_720X480_422_10_5994p        // 5
    "FILE 720x480 422 Planar 10 Bit 60i",               // M31_FILE_720X480_422_10_60i          // 6
    "FILE 720x480 422 Planar 10 Bit 60p",               // M31_FILE_720X480_422_10_60p          // 7

    "FILE 720x576 420 Planar 8 Bit 50i",                // M31_FILE_720X576_420_8_50i           // 8
    "FILE 720x576 420 Planar 8 Bit 50p",                // M31_FILE_720X576_420_8_50p           // 9
    "FILE 720x576 422 Planar 10 Bit 50i",               // M31_FILE_720X576_422_10_50i          // 10
    "FILE 720x576 422 Planar 10 Bit 50p",               // M31_FILE_720X576_422_10_50p          // 11

    "FILE 1280x720 420 Planar 8 Bit 2398p",             // M31_FILE_1280X720_420_8_2398p        // 12
    "FILE 1280x720 420 Planar 8 Bit 24p",               // M31_FILE_1280X720_420_8_24p          // 13
    "FILE 1280x720 420 Planar 8 Bit 25p",               // M31_FILE_1280X720_420_8_25p          // 14
    "FILE 1280x720 420 Planar 8 Bit 29.97p",            // M31_FILE_1280X720_420_8_2997p        // 15
    "FILE 1280x720 420 Planar 8 Bit 30p",               // M31_FILE_1280X720_420_8_30p          // 16
    "FILE 1280x720 420 Planar 8 Bit 50p",               // M31_FILE_1280X720_420_8_50p          // 17
    "FILE 1280x720 420 Planar 8 Bit 59.94p",            // M31_FILE_1280X720_420_8_5994p        // 18
    "FILE 1280x720 420 Planar 8 Bit 60p",               // M31_FILE_1280X720_420_8_60p          // 19
    
    "FILE 1280x720 422 Planar 10 Bit 2398p",            // M31_FILE_1280X720_422_10_2398p       // 20
    "FILE 1280x720 422 Planar 10 Bit 25p",              // M31_FILE_1280X720_422_10_24p         // 21
    "FILE 1280x720 422 Planar 10 Bit 25p",              // M31_FILE_1280X720_422_10_25p         // 22
    "FILE 1280x720 422 Planar 10 Bit 29.97p",           // M31_FILE_1280X720_422_10_2997p       // 23
    "FILE 1280x720 422 Planar 10 Bit 30p",              // M31_FILE_1280X720_422_10_30p         // 24
    "FILE 1280x720 422 Planar 10 Bit 50p",              // M31_FILE_1280X720_422_10_50p         // 25
    "FILE 1280x720 422 Planar 10 Bit 59.94p",           // M31_FILE_1280X720_422_10_5994p       // 26
    "FILE 1280x720 422 Planar 10 Bit 60p",              // M31_FILE_1280X720_422_10_60p         // 27

    "FILE 1920x1080 420 Planar 8 Bit 2398p",            // M31_FILE_1920X1080_420_8_2398p       // 28
    "FILE 1920x1080 420 Planar 8 Bit 24p",              // M31_FILE_1920X1080_420_8_24p         // 29
    "FILE 1920x1080 420 Planar 8 Bit 25p",              // M31_FILE_1920X1080_420_8_25p         // 30
    "FILE 1920x1080 420 Planar 8 Bit 29.97p",           // M31_FILE_1920X1080_420_8_2997p       // 31
    "FILE 1920x1080 420 Planar 8 Bit 30p",              // M31_FILE_1920X1080_420_8_30p         // 32
    "FILE 1920x1080 420 Planar 8 Bit 50i",              // M31_FILE_1920X1080_420_8_50i         // 33
    "FILE 1920x1080 420 Planar 8 Bit 50p",              // M31_FILE_1920X1080_420_8_50p         // 34
    "FILE 1920x1080 420 Planar 8 Bit 59.94i",           // M31_FILE_1920X1080_420_8_5994i       // 35
    "FILE 1920x1080 420 Planar 8 Bit 59.94p",           // M31_FILE_1920X1080_420_8_5994p       // 36
    "FILE 1920x1080 420 Planar 8 Bit 60i",              // M31_FILE_1920X1080_420_8_60i         // 37
    "FILE 1920x1080 420 Planar 8 Bit 60p",              // M31_FILE_1920X1080_420_8_60p         // 38
    
    "FILE 1920x1080 422 Planar 10 Bit 2398p",           // M31_FILE_1920X1080_422_10_2398p      // 39
    "FILE 1920x1080 422 Planar 10 Bit 24p",             // M31_FILE_1920X1080_422_10_24p        // 40
    "FILE 1920x1080 422 Planar 10 Bit 25p",             // M31_FILE_1920X1080_422_10_25p        // 41
    "FILE 1920x1080 422 Planar 10 Bit 29.97p",          // M31_FILE_1920X1080_422_10_2997p      // 42
    "FILE 1920x1080 422 Planar 10 Bit 30p",             // M31_FILE_1920X1080_422_10_30p        // 43
    "FILE 1920x1080 422 Planar 10 Bit 50i",             // M31_FILE_1920X1080_422_10_50i        // 44
    "FILE 1920x1080 422 Planar 10 Bit 50p",             // M31_FILE_1920X1080_422_10_50p        // 45
    "FILE 1920x1080 422 Planar 10 Bit 59.94i",          // M31_FILE_1920X1080_422_10_5994i      // 46
    "FILE 1920x1080 422 Planar 10 Bit 59.94p",          // M31_FILE_1920X1080_422_10_5994p      // 47
    "FILE 1920x1080 422 Planar 10 Bit 60i",             // M31_FILE_1920X1080_422_10_60i        // 48
    "FILE 1920x1080 422 Planar 10 Bit 60p",             // M31_FILE_1920X1080_422_10_60p        // 49

    "FILE 2048x1080 420 Planar 8 Bit 2398p",            // M31_FILE_2048X1080_420_8_2398p       // 50
    "FILE 2048x1080 420 Planar 8 Bit 24p",              // M31_FILE_2048X1080_420_8_24p         // 51
    "FILE 2048x1080 420 Planar 8 Bit 25p",              // M31_FILE_2048X1080_420_8_25p         // 52
    "FILE 2048x1080 420 Planar 8 Bit 29.97p",           // M31_FILE_2048X1080_420_8_2997p       // 53
    "FILE 2048x1080 420 Planar 8 Bit 30p",              // M31_FILE_2048X1080_420_8_30p         // 54
    "FILE 2048x1080 420 Planar 8 Bit 50p",              // M31_FILE_2048X1080_420_8_50p         // 55
    "FILE 2048x1080 420 Planar 8 Bit 59.94p",           // M31_FILE_2048X1080_420_8_5994p       // 56
    "FILE 2048x1080 420 Planar 8 Bit 60p",              // M31_FILE_2048X1080_420_8_60p         // 57
    
    "FILE 2048x1080 422 Planar 10 Bit 2398p",           // M31_FILE_2048X1080_422_10_2398p      // 58
    "FILE 2048x1080 422 Planar 10 Bit 24p",             // M31_FILE_2048X1080_422_10_24p        // 59
    "FILE 2048x1080 422 Planar 10 Bit 25p",             // M31_FILE_2048X1080_422_10_25p        // 60
    "FILE 2048x1080 422 Planar 10 Bit 29.97p",          // M31_FILE_2048X1080_422_10_2997p      // 61
    "FILE 2048x1080 422 Planar 10 Bit 30p",             // M31_FILE_2048X1080_422_10_30p        // 62
    "FILE 2048x1080 422 Planar 10 Bit 50p",             // M31_FILE_2048X1080_422_10_50p        // 63
    "FILE 2048x1080 422 Planar 10 Bit 59.94p",          // M31_FILE_2048X1080_422_10_5994p      // 64
    "FILE 2048x1080 422 Planar 10 Bit 60p",             // M31_FILE_2048X1080_422_10_60p        // 65

    "FILE 3840x2160 420 Planar 8 Bit 2398p",            // M31_FILE_3840X2160_420_8_2398p       // 66
    "FILE 3840x2160 420 Planar 8 Bit 24p",              // M31_FILE_3840X2160_420_8_24p         // 67
    "FILE 3840x2160 420 Planar 8 Bit 25p",              // M31_FILE_3840X2160_420_8_25p         // 68
    "FILE 3840x2160 420 Planar 8 Bit 29.97p",           // M31_FILE_3840X2160_420_8_2997p       // 69
    "FILE 3840x2160 420 Planar 8 Bit 30p",              // M31_FILE_3840X2160_420_8_30p         // 70
    "FILE 3840x2160 420 Planar 8 Bit 50p",              // M31_FILE_3840X2160_420_8_50p         // 71
    "FILE 3840x2160 420 Planar 8 Bit 59.94p",           // M31_FILE_3840X2160_420_8_5994p       // 72
    "FILE 3840x2160 420 Planar 8 Bit 60p",              // M31_FILE_3840X2160_420_8_60p         // 73

    "FILE 3840x2160 420 Planar 10 Bit 50p",             // M31_FILE_3840X2160_420_10_50p        // 74
    "FILE 3840x2160 420 Planar 10 Bit 59.94p",          // M31_FILE_3840X2160_420_10_5994p      // 75
    "FILE 3840x2160 420 Planar 10 Bit 60p",             // M31_FILE_3840X2160_420_10_60p        // 76
  
    "FILE 3840x2160 422 Planar 8 Bit 2398p",            // M31_FILE_3840X2160_422_8_2398p       // 77
    "FILE 3840x2160 422 Planar 8 Bit 24p",              // M31_FILE_3840X2160_422_8_24p         // 78
    "FILE 3840x2160 422 Planar 8 Bit 25p",              // M31_FILE_3840X2160_422_8_25p         // 79
    "FILE 3840x2160 422 Planar 8 Bit 29.97p",           // M31_FILE_3840X2160_422_8_2997p       // 80
    "FILE 3840x2160 422 Planar 8 Bit 30p",              // M31_FILE_3840X2160_422_8_30p         // 81
    "FILE 3840x2160 422 Planar 8 Bit 50p",              // M31_FILE_3840X2160_422_8_60p         // 82
    "FILE 3840x2160 422 Planar 8 Bit 59.94p",           // M31_FILE_3840X2160_422_8_5994p       // 83
    "FILE 3840x2160 422 Planar 8 Bit 60p",              // M31_FILE_3840X2160_422_8_60p         // 84
    
    "FILE 3840x2160 422 Planar 10 Bit 2398p",           // M31_FILE_3840X2160_422_10_2398p      // 85
    "FILE 3840x2160 422 Planar 10 Bit 24p",             // M31_FILE_3840X2160_422_10_24p        // 86
    "FILE 3840x2160 422 Planar 10 Bit 25p",             // M31_FILE_3840X2160_422_10_25p        // 87
    "FILE 3840x2160 422 Planar 10 Bit 29.97p",          // M31_FILE_3840X2160_422_10_2997p      // 88
    "FILE 3840x2160 422 Planar 10 Bit 30p",             // M31_FILE_3840X2160_422_10_30p        // 89
    "FILE 3840x2160 422 Planar 10 Bit 50p",             // M31_FILE_3840X2160_422_10_50p        // 90
    "FILE 3840x2160 422 Planar 10 Bit 59.94p",          // M31_FILE_3840X2160_422_10_5994p      // 91
    "FILE 3840x2160 422 Planar 10 Bit 60p",             // M31_FILE_3840X2160_422_10_60p        // 92
    
    "FILE 4096x2160 420 Planar 10 Bit 5994p",           // M31_FILE_4096X2160_420_10_5994p,     // 93
    "FILE 4096x2160 420 Planar 10 Bit 60p",             // M31_FILE_4096X2160_420_10_60p,       // 94
    "FILE 4096x2160 422 Planar 10 Bit 50p",             // M31_FILE_4096X2160_422_10_50p,       // 95
    "FILE 4096x2160 422 Planar 10 Bit 5994p IOnly",     // M31_FILE_4096X2160_422_10_5994p_IO,  // 96
    "FILE 4096x2160 422 Planar 10 Bit 60p IOnly",       // M31_FILE_4096X2160_422_10_60p_IO,    // 97
    
    "VIF 720x480 420 Planar 8 Bit 59.94i",              // M31_VIF_720X480_420_8_5994i          // 98
    "VIF 720x480 420 Planar 8 Bit 59.94p",              // M31_VIF_720X480_420_8_5994p          // 99
    "VIF 720x480 420 Planar 8 Bit 60i",                 // M31_VIF_720X480_420_8_60i            // 100
    "VIF 720x480 420 Planar 8 Bit 60p",                 // M31_VIF_720X480_420_8_60p            // 101
    "VIF 720x480 422 Planar 10 Bit 59.94i",             // M31_VIF_720X480_422_10_5994i         // 102
    "VIF 720x480 422 Planar 10 Bit 59.94p",             // M31_VIF_720X480_422_10_5994p         // 103
    "VIF 720x480 422 Planar 10 Bit 60i",                // M31_VIF_720X480_422_10_60i           // 104
    "VIF 720x480 422 Planar 10 Bit 60p",                // M31_VIF_720X480_422_10_60p           // 105

    "VIF 720x576 420 Planar 8 Bit 50i",                 // M31_VIF_720X576_420_8_50i            // 106
    "VIF 720x576 420 Planar 8 Bit 50p",                 // M31_VIF_720X576_420_8_50p            // 107
    "VIF 720x576 422 Planar 10 Bit 50i",                // M31_VIF_720X576_422_10_50i           // 108
    "VIF 720x576 422 Planar 10 Bit 50p",                // M31_VIF_720X576_422_10_50p           // 109

    "VIF 1280x720 420 Planar 8 Bit 50p",                // M31_VIF_1280X720_420_8_50p           // 110
    "VIF 1280x720 420 Planar 8 Bit 59.94p",             // M31_VIF_1280X720_420_8_5994p         // 111
    "VIF 1280x720 420 Planar 8 Bit 60p",                // M31_VIF_1280X720_420_8_60p           // 112
    "VIF 1280x720 422 Planar 10 Bit 50p",               // M31_VIF_1280X720_422_10_50p          // 113
    "VIF 1280x720 422 Planar 10 Bit 59.94p",            // M31_VIF_1280X720_422_10_5994p        // 114
    "VIF 1280x720 422 Planar 10 Bit 60p",               // M31_VIF_1280X720_422_10_60p          // 115

    "VIF 1920x1080 420 Planar 8 Bit 50i",               // M31_VIF_1920X1080_420_8_50i          // 116
    "VIF 1920x1080 420 Planar 8 Bit 50p",               // M31_VIF_1920X1080_420_8_50p          // 117
    "VIF 1920x1080 420 Planar 8 Bit 59.94i",            // M31_VIF_1920X1080_420_8_5994i        // 118
    "VIF 1920x1080 420 Planar 8 Bit 59.94p",            // M31_VIF_1920X1080_420_8_5994p        // 119
    "VIF 1920x1080 420 Planar 8 Bit 60i",               // M31_VIF_1920X1080_420_8_60i          // 120
    "VIF 1920x1080 420 Planar 8 Bit 60p",               // M31_VIF_1920X1080_420_8_60p          // 121
    "VIF 1920x1080 420 Planar 10 Bit 50i",              // M31_VIF_1920X1080_420_10_50i         // 122
    "VIF 1920x1080 420 Planar 10 Bit 50p",              // M31_VIF_1920X1080_420_10_50p         // 123
    "VIF 1920x1080 420 Planar 10 Bit 59.94i",           // M31_VIF_1920X1080_420_10_5994i       // 124
    "VIF 1920x1080 420 Planar 10 Bit 59.94p",           // M31_VIF_1920X1080_420_10_5994p       // 125
    "VIF 1920x1080 420 Planar 10 Bit 60i",              // M31_VIF_1920X1080_420_10_60i         // 126
    "VIF 1920x1080 420 Planar 10 Bit 60p",              // M31_VIF_1920X1080_420_10_60p         // 127
    "VIF 1920x1080 422 Planar 10 Bit 59.94i",           // M31_VIF_1920X1080_422_10_5994i       // 128
    "VIF 1920x1080 422 Planar 10 Bit 59.94p",           // M31_VIF_1920X1080_422_10_5994p       // 129
    "VIF 1920x1080 422 Planar 10 Bit 60i",              // M31_VIF_1920X1080_422_10_60i         // 130
    "VIF 1920x1080 422 Planar 10 Bit 60p",              // M31_VIF_1920X1080_422_10_60p         // 131
  
    "VIF 3840x2160 420 Planar 8 Bit 30p",               // M31_VIF_3840X2160_420_8_30p          // 132
    "VIF 3840x2160 420 Planar 8 Bit 50p",               // M31_VIF_3840X2160_420_8_50p          // 133
    "VIF 3840x2160 420 Planar 8 Bit 59.94p",            // M31_VIF_3840X2160_420_8_5994p        // 134
    "VIF 3840x2160 420 Planar 8 Bit 60p",               // M31_VIF_3840X2160_420_8_5994p        // 135
    "VIF 3840x2160 420 Planar 10 Bit 50p",              // M31_VIF_3840X2160_420_8_60p          // 136
    "VIF 3840x2160 420 Planar 10 Bit 59.94p",           // M31_VIF_3840X2160_420_8_60p          // 137
    "VIF 3840x2160 420 Planar 10 Bit 60p",              // M31_VIF_3840X2160_420_10_5994p       // 138
    
    "VIF 3840x2160 422 Planar 10 Bit 30p",              // M31_VIF_3840X2160_422_10_30p         // 139
    "VIF 3840x2160 422 Planar 10 Bit 50p",              // M31_VIF_3840X2160_422_10_50p         // 140
    "VIF 3840x2160 422 Planar 10 Bit 59.94p",           // M31_VIF_3840X2160_422_10_5994p       // 141
    "VIF 3840x2160 422 Planar 10 Bit 60p",              // M31_VIF_3840X2160_422_10_60p         // 142
};


AJA_LOCAL_STATIC const char * NTV2FrameRateStrings [NTV2_NUM_FRAMERATES] =
{
	"Unknown",							//	NTV2_FRAMERATE_UNKNOWN			//	0
	"60.00",							//	NTV2_FRAMERATE_6000				//	1
	"59.94",							//	NTV2_FRAMERATE_5994				//	2
	"30.00",							//	NTV2_FRAMERATE_3000				//	3
	"29.97",							//	NTV2_FRAMERATE_2997				//	4
	"25.00",							//	NTV2_FRAMERATE_2500				//	5
	"24.00",							//	NTV2_FRAMERATE_2400				//	6
	"23.98",							//	NTV2_FRAMERATE_2398				//	7
	"50.00",							//	NTV2_FRAMERATE_5000				//	8
	"48.00",							//	NTV2_FRAMERATE_4800				//	9
	"47.95",							//	NTV2_FRAMERATE_4795				//	10
	"120.00",							//	NTV2_FRAMERATE_12000			//	11
	"119.88",							//	NTV2_FRAMERATE_11988			//	12
	"15.00",							//	NTV2_FRAMERATE_1500				//	13
	"14.98",							//	NTV2_FRAMERATE_1498				//	14
	"19.00",							//	NTV2_FRAMERATE_1900				//	15
	"18.98",							//	NTV2_FRAMERATE_1898				//	16
	"18.00",							//	NTV2_FRAMERATE_1800				//	17
	"17.98"								//	NTV2_FRAMERATE_1798				//	18
};

// Extracts a channel pair or all channels from the
// NTV2 channel buffer that is retrieved from the hardware.
int RecordCopyAudio(PULWord pAja, PULWord pSR, int iStartSample, int iNumBytes, int iChan0,
                    int iNumChans, bool bKeepAudio24Bits)
{
    const int SAMPLE_SIZE = NTV2_NUMAUDIO_CHANNELS * NTV2_AUDIOSAMPLESIZE;

    // Insurance to prevent bogus array sizes causing havoc
//    if (iNumBytes > 48048)      // 23.98 == 2002 * 24
//        iNumBytes = 48048;

    // Adjust the offset of the first valid channel
    if (iStartSample)
    {
        iChan0 += (NTV2_NUMAUDIO_CHANNELS - iStartSample);
    }

    // Driver records audio to offset 24 bytes
    PULWord pIn = &pAja[NTV2_NUMAUDIO_CHANNELS];
    UWord * puwOut = (UWord *) pSR;

    // If our transfer size has a remainder and our chans are in it,
    // adjust number samples
    int iNumSamples = iNumBytes / SAMPLE_SIZE;
    int iMod = (iNumBytes % SAMPLE_SIZE) / 4;
    if (iMod > iChan0)
        iNumSamples++;
    // else if we have remainder with chans && chans total > number of chans
    // reduce start offset by the number of chans
    else if (iMod && iChan0 >= NTV2_NUMAUDIO_CHANNELS)
    {
        iNumSamples++;
        iChan0 -= NTV2_NUMAUDIO_CHANNELS;
    }
    // else if no remainder but start sample adjustment gives more chans
    // than number of chans, drop the start offset back by num chans
    else if (iChan0 >= NTV2_NUMAUDIO_CHANNELS)
    {
        iChan0 -= NTV2_NUMAUDIO_CHANNELS;
    }

    // Copy incoming audio to the outgoing array
    if (bKeepAudio24Bits)
    {
        for (int s = 0; s < iNumSamples; s++)
        {
            for (int c = iChan0; c < iChan0 + iNumChans; c++)
            {
                *pSR++ = pIn[c];
            }

            pIn += NTV2_NUMAUDIO_CHANNELS;
        }
    }
    else    // convert audio to 16 bits
    {
        for (int s = 0; s < iNumSamples; s++)
        {
            for (int c = iChan0; c < iChan0 + iNumChans; c++)
            {
                *puwOut++ = (UWord) (pIn[c] >> 16);
            }

            pIn += NTV2_NUMAUDIO_CHANNELS;
        }
    }

    return iNumSamples;
}

#include "math.h"
// M_PI is defined on RedHat Linux 9 in math.h
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif


ULWord	AddAudioTone (	ULWord *		pAudioBuffer,
						ULWord &		inOutCurrentSample,
						const ULWord	inNumSamples,
						const double	inSampleRate,
						const double	inAmplitude,
						const double	inFrequency,
						const ULWord	inNumBits,
						const bool		inByteSwap,
						const ULWord	inNumChannels)
{
	double			j			(inOutCurrentSample);
	const double	cycleLength	(inSampleRate / inFrequency);
	const double	scale		(double (ULWord (1 << (inNumBits - 1))) - 1.0);

	if (pAudioBuffer)
	{
		for (ULWord i = 0;  i < inNumSamples;  i++)
		{
			const float	nextFloat	= (float) (::sin (j / cycleLength * (M_PI * 2.0)) * inAmplitude);
			ULWord		value		= static_cast <ULWord> ((nextFloat * scale) + float (0.5));

			if (inByteSwap)
				value = NTV2EndianSwap32 (value);
			//odprintf("%f",(float)(LWord)value/(float)0x7FFFFFFF);

			for (ULWord channel = 0;  channel < inNumChannels;  channel++)
				*pAudioBuffer++ = value;

			j += 1.0;
			if (j > cycleLength)
				j -= cycleLength;
			inOutCurrentSample++;
		}	//	for each sample
	}	//	if pAudioBuffer

	return inNumSamples * 4 * inNumChannels;

}	//	AddAudioTone (ULWord)


ULWord	AddAudioTone (	UWord *			pAudioBuffer,
						ULWord &		inOutCurrentSample,
						const ULWord	inNumSamples,
						const double	inSampleRate,
						const double	inAmplitude,
						const double	inFrequency,
						const ULWord	inNumBits,
						const bool		inByteSwap,
						const ULWord	inNumChannels)
{
	double			j			(inOutCurrentSample);
	const double	cycleLength	(inSampleRate / inFrequency);
	const double	scale		(double (ULWord (1 << (inNumBits - 1))) - 1.0);

	if (pAudioBuffer)
	{
		for (ULWord i = 0;  i < inNumSamples;  i++)
		{
			const float	nextFloat	= float (::sin (j / cycleLength * (M_PI * 2.0)) * inAmplitude);
			UWord		value		= static_cast <UWord> ((nextFloat * scale) + float (0.5));

			if (inByteSwap)
				value = NTV2EndianSwap16 (value);

			for (ULWord channel = 0;  channel < inNumChannels;  channel++)
				*pAudioBuffer++ = value;

			j += 1.0;
			if (j > cycleLength)
				j -= cycleLength;
			inOutCurrentSample++;
		}	//	for each sample
	}	//	if pAudioBuffer

	return inNumSamples * 4 * inNumChannels;

}	//	AddAudioTone (UWord)


ULWord	AddAudioTone (	ULWord *		pAudioBuffer,
						ULWord &		inOutCurrentSample,
						const ULWord	inNumSamples,
						const double	inSampleRate,
						const double *	pInAmplitudes,
						const double *	pInFrequencies,
						const ULWord	inNumBits,
						const bool		inByteSwap,
						const ULWord	inNumChannels)
{
	double			j [kNumAudioChannelsMax];
	double			cycleLength [kNumAudioChannelsMax];
	const double	scale		(double (ULWord (1 << (inNumBits - 1))) - 1.0);

	for (ULWord channel (0);  channel < inNumChannels;  channel++)
	{
		cycleLength[channel] = inSampleRate / pInFrequencies[channel];
		j [channel] = inOutCurrentSample;
	}

	if (pAudioBuffer && pInAmplitudes && pInFrequencies)
	{
		for (ULWord i (0);  i < inNumSamples;  i++)
		{
			for (ULWord channel (0);  channel < inNumChannels;  channel++)
			{
				const float	nextFloat	= float (::sin (j[channel] / cycleLength[channel] * (M_PI * 2.0)) * pInAmplitudes[channel]);
				ULWord		value		= static_cast <ULWord> ((nextFloat * scale) + float (0.5));

				if (inByteSwap)
					value = NTV2EndianSwap32(value);

				*pAudioBuffer++ = value;

				j[channel] += 1.0;
				if (j[channel] > cycleLength[channel])
					j[channel] -= cycleLength[channel];

			}
			inOutCurrentSample++;
		}	//	for each sample
	}	//	if pAudioBuffer && pInFrequencies

	return inNumSamples * 4 * inNumChannels;

}	//	AddAudioTone (per-chl freq & ampl)


ULWord	AddAudioTestPattern (ULWord*            audioBuffer,
							 ULWord&            currentSample,
							 ULWord             numSamples,
							 ULWord             modulus,
							 bool               endianConvert,
							 ULWord   		    numChannels)
{

	for (ULWord i = 0; i <numSamples; i++)
	{
		ULWord value = (currentSample%modulus)<<16;
		if ( endianConvert )
			value = NTV2EndianSwap32(value);
		for (ULWord channel = 0; channel< numChannels; channel++)
		{
			*audioBuffer++ = value;
		}
		currentSample++;
	}
	return numSamples*4*numChannels;

}


std::string NTV2DeviceIDToString (const NTV2DeviceID inValue,	const bool inForRetailDisplay)
{
	switch (inValue)
	{
	#if defined (AJAMac) || defined (MSWindows)
        case DEVICE_ID_KONALHI:                 return inForRetailDisplay ?	"KONA LHi"                  : "KonaLHi";
        case DEVICE_ID_KONALHIDVI:              return inForRetailDisplay ?	"KONA LHi DVI"              : "KonaLHiDVI";
	#endif
	#if defined (AJAMac)
        case DEVICE_ID_IOEXPRESS:               return inForRetailDisplay ?	"IoExpress"                 : "IoExpress";
	#elif defined (MSWindows)
        case DEVICE_ID_IOEXPRESS:               return inForRetailDisplay ?	"KONA IoExpress"            : "IoExpress";
	#else
        case DEVICE_ID_KONALHI:                 return inForRetailDisplay ?	"KONA LHi"                  : "OEM LHi";
        case DEVICE_ID_KONALHIDVI:              return inForRetailDisplay ?	"KONA LHi DVI"              : "OEM LHi DVI";
        case DEVICE_ID_IOEXPRESS:               return inForRetailDisplay ?	"IoExpress"                 : "OEM IoExpress";
	#endif
        case DEVICE_ID_NOTFOUND:                return inForRetailDisplay ?	"AJA Device"                : "(Not Found)";
        case DEVICE_ID_CORVID1:                 return inForRetailDisplay ?	"Corvid 1"                  : "Corvid";
        case DEVICE_ID_CORVID22:                return inForRetailDisplay ?	"Corvid 22"                 : "Corvid22";
        case DEVICE_ID_CORVID3G:                return inForRetailDisplay ?	"Corvid 3G"                 : "Corvid3G";
        case DEVICE_ID_KONA3G:                  return inForRetailDisplay ?	"KONA 3G"                   : "Kona3G";
        case DEVICE_ID_KONA3GQUAD:              return inForRetailDisplay ?	"KONA 3G QUAD"              : "Kona3GQuad";	//	Used to be "KONA 3G" for retail display
        case DEVICE_ID_KONALHEPLUS:             return inForRetailDisplay ?	"KONA LHe+"                 : "KonaLHe+";
        case DEVICE_ID_IOXT:                    return inForRetailDisplay ?	"IoXT"                      : "IoXT";
        case DEVICE_ID_CORVID24:                return inForRetailDisplay ?	"Corvid 24"                 : "Corvid24";
        case DEVICE_ID_TTAP:                    return inForRetailDisplay ?	"T-Tap"                     : "TTap";
		case DEVICE_ID_IO4K:					return inForRetailDisplay ?	"Io4K"						: "Io4K";
		case DEVICE_ID_IO4KUFC:					return inForRetailDisplay ?	"Io4K UFC"					: "Io4KUfc";
		case DEVICE_ID_KONA4:					return inForRetailDisplay ?	"KONA 4"					: "Kona4";
		case DEVICE_ID_KONA4UFC:				return inForRetailDisplay ?	"KONA 4 UFC"				: "Kona4Ufc";
		case DEVICE_ID_CORVID88:				return inForRetailDisplay ?	"Corvid 88"					: "Corvid88";
		case DEVICE_ID_CORVID44:				return inForRetailDisplay ?	"Corvid 44"					: "Corvid44";
		case DEVICE_ID_CORVIDHEVC:				return inForRetailDisplay ?	"Corvid HEVC"				: "CorvidHEVC";
        case DEVICE_ID_KONAIP_2022:             return "KonaIP s2022";
        case DEVICE_ID_KONAIP_4CH_2SFP:			return "KonaIP s2022 2+2";
        case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:	return "KonaIP J2K 1I 1O";
        case DEVICE_ID_KONAIP_2TX_1SFP_J2K:		return "KonaIP J2K 2O";
        case DEVICE_ID_KONAIP_2RX_1SFP_J2K:     return "KonaIP J2K 2I";
        case DEVICE_ID_KONAIP_1RX_1TX_2110:     return "KonaIP s2110 1I 1O";
		case DEVICE_ID_CORVIDHBR:               return inForRetailDisplay ? "Corvid HB-R"               : "CorvidHBR";
        case DEVICE_ID_IO4KPLUS:				return inForRetailDisplay ? "Avid DNxIV"                : "Io4K Plus";
        case DEVICE_ID_IOIP_2022:				return inForRetailDisplay ? "Avid DNxIP s2022"          : "IoIP s2022";
        case DEVICE_ID_IOIP_2110:				return inForRetailDisplay ? "Avid DNxIP s2110"          : "IoIP s2110";
		case DEVICE_ID_KONAIP_2110:             return "KonaIP s2110";
		case DEVICE_ID_KONA1:					return inForRetailDisplay ? "Kona 1"					: "Kona1";
        case DEVICE_ID_KONAHDMI:				return inForRetailDisplay ? "Kona HDMI"					: "KonaHDMI";
		case DEVICE_ID_KONA5:					return inForRetailDisplay ?	"KONA 5"					: "Kona5";
        case DEVICE_ID_KONA5_12G:               return inForRetailDisplay ?	"KONA 5 12G"				: "Kona5 12G";
		case DEVICE_ID_CORVID44_12G:             return inForRetailDisplay ?	"Corvid 44 12G"				: "Corvid44 12G";
#if !defined (_DEBUG)
	    default:					break;
#endif
	}
	return inForRetailDisplay ?	"Unknown" : "???";
}


#if !defined (NTV2_DEPRECATE)
	void GetNTV2RetailBoardString (NTV2BoardID inBoardID, std::string & outString)
	{
		outString = ::NTV2DeviceIDToString (inBoardID, true);
	}

	NTV2BoardType GetNTV2BoardTypeForBoardID (NTV2BoardID inBoardID)
	{
		return BOARDTYPE_NTV2;
	}

	void GetNTV2BoardString (NTV2BoardID inBoardID, string & outName)
	{
		outName = ::NTV2DeviceIDToString (inBoardID);
	}

	std::string NTV2BoardIDToString (const NTV2BoardID inValue,	const bool inForRetailDisplay)
	{
		return NTV2DeviceIDToString (inValue, inForRetailDisplay);
	}

	string frameBufferFormatString (const NTV2FrameBufferFormat inFrameBufferFormat)
	{
		string	result;
		if (inFrameBufferFormat >= 0 && inFrameBufferFormat < NTV2_FBF_NUMFRAMEBUFFERFORMATS)
			result = frameBufferFormats [inFrameBufferFormat];
		return result;
	}
#endif	//	!defined (NTV2_DEPRECATE)


NTV2Channel GetNTV2ChannelForIndex (const ULWord index)
{
	switch(index)
	{
	default:
	case 0:	return NTV2_CHANNEL1;
	case 1:	return NTV2_CHANNEL2;
	case 2:	return NTV2_CHANNEL3;
	case 3:	return NTV2_CHANNEL4;
	case 4: return NTV2_CHANNEL5;
	case 5: return NTV2_CHANNEL6;
	case 6: return NTV2_CHANNEL7;
	case 7: return NTV2_CHANNEL8;
	}
}

ULWord GetIndexForNTV2Channel (const NTV2Channel channel)
{
	switch(channel)
	{
	default:
	case NTV2_CHANNEL1:	return 0;
	case NTV2_CHANNEL2:	return 1;
	case NTV2_CHANNEL3:	return 2;
	case NTV2_CHANNEL4:	return 3;
	case NTV2_CHANNEL5: return 4;
	case NTV2_CHANNEL6: return 5;
	case NTV2_CHANNEL7: return 6;
	case NTV2_CHANNEL8: return 7;
	}
}


NTV2Channel NTV2CrosspointToNTV2Channel (const NTV2Crosspoint inCrosspointChannel)
{
	switch (inCrosspointChannel)
	{
		case NTV2CROSSPOINT_CHANNEL1:	return NTV2_CHANNEL1;
		case NTV2CROSSPOINT_CHANNEL2:	return NTV2_CHANNEL2;
		case NTV2CROSSPOINT_INPUT1:		return NTV2_CHANNEL1;
		case NTV2CROSSPOINT_INPUT2:		return NTV2_CHANNEL2;
		case NTV2CROSSPOINT_MATTE:		return NTV2_CHANNEL_INVALID;
		case NTV2CROSSPOINT_FGKEY:		return NTV2_CHANNEL_INVALID;
		case NTV2CROSSPOINT_CHANNEL3:	return NTV2_CHANNEL3;
		case NTV2CROSSPOINT_CHANNEL4:	return NTV2_CHANNEL4;
		case NTV2CROSSPOINT_INPUT3:		return NTV2_CHANNEL3;
		case NTV2CROSSPOINT_INPUT4:		return NTV2_CHANNEL4;
		case NTV2CROSSPOINT_CHANNEL5:	return NTV2_CHANNEL5;
		case NTV2CROSSPOINT_CHANNEL6:	return NTV2_CHANNEL6;
		case NTV2CROSSPOINT_CHANNEL7:	return NTV2_CHANNEL7;
		case NTV2CROSSPOINT_CHANNEL8:	return NTV2_CHANNEL8;
		case NTV2CROSSPOINT_INPUT5:		return NTV2_CHANNEL5;
		case NTV2CROSSPOINT_INPUT6:		return NTV2_CHANNEL6;
		case NTV2CROSSPOINT_INPUT7:		return NTV2_CHANNEL7;
		case NTV2CROSSPOINT_INPUT8:		return NTV2_CHANNEL8;
		case NTV2CROSSPOINT_INVALID:	return NTV2_CHANNEL_INVALID;
	}
	return NTV2_CHANNEL_INVALID;
}


NTV2Crosspoint GetNTV2CrosspointChannelForIndex (const ULWord index)
{
	switch(index)
	{
	default:
	case 0:	return NTV2CROSSPOINT_CHANNEL1;
	case 1:	return NTV2CROSSPOINT_CHANNEL2;
	case 2:	return NTV2CROSSPOINT_CHANNEL3;
	case 3:	return NTV2CROSSPOINT_CHANNEL4;
	case 4:	return NTV2CROSSPOINT_CHANNEL5;
	case 5:	return NTV2CROSSPOINT_CHANNEL6;
	case 6:	return NTV2CROSSPOINT_CHANNEL7;
	case 7:	return NTV2CROSSPOINT_CHANNEL8;
	}
}

ULWord GetIndexForNTV2CrosspointChannel (const NTV2Crosspoint channel)
{
	switch(channel)
	{
	default:
	case NTV2CROSSPOINT_CHANNEL1:	return 0;
	case NTV2CROSSPOINT_CHANNEL2:	return 1;
	case NTV2CROSSPOINT_CHANNEL3:	return 2;
	case NTV2CROSSPOINT_CHANNEL4:	return 3;
	case NTV2CROSSPOINT_CHANNEL5:	return 4;
	case NTV2CROSSPOINT_CHANNEL6:	return 5;
	case NTV2CROSSPOINT_CHANNEL7:	return 6;
	case NTV2CROSSPOINT_CHANNEL8:	return 7;
	}
}

NTV2Crosspoint GetNTV2CrosspointInputForIndex (const ULWord index)
{
	switch(index)
	{
	default:
	case 0:	return NTV2CROSSPOINT_INPUT1;
	case 1:	return NTV2CROSSPOINT_INPUT2;
	case 2:	return NTV2CROSSPOINT_INPUT3;
	case 3:	return NTV2CROSSPOINT_INPUT4;
	case 4:	return NTV2CROSSPOINT_INPUT5;
	case 5:	return NTV2CROSSPOINT_INPUT6;
	case 6:	return NTV2CROSSPOINT_INPUT7;
	case 7:	return NTV2CROSSPOINT_INPUT8;
	}
}

ULWord GetIndexForNTV2CrosspointInput (const NTV2Crosspoint channel)
{
	switch(channel)
	{
	default:
	case NTV2CROSSPOINT_INPUT1:	return 0;
	case NTV2CROSSPOINT_INPUT2:	return 1;
	case NTV2CROSSPOINT_INPUT3:	return 2;
	case NTV2CROSSPOINT_INPUT4:	return 3;
	case NTV2CROSSPOINT_INPUT5:	return 4;
	case NTV2CROSSPOINT_INPUT6:	return 5;
	case NTV2CROSSPOINT_INPUT7:	return 6;
	case NTV2CROSSPOINT_INPUT8:	return 7;
	}
}

NTV2Crosspoint GetNTV2CrosspointForIndex (const ULWord index)
{
	switch(index)
	{
	default:
	case 0:	return NTV2CROSSPOINT_CHANNEL1;
	case 1:	return NTV2CROSSPOINT_CHANNEL2;
	case 2:	return NTV2CROSSPOINT_CHANNEL3;
	case 3:	return NTV2CROSSPOINT_CHANNEL4;
	case 4:	return NTV2CROSSPOINT_INPUT1;
	case 5:	return NTV2CROSSPOINT_INPUT2;
	case 6:	return NTV2CROSSPOINT_INPUT3;
	case 7:	return NTV2CROSSPOINT_INPUT4;
	case 8:	return NTV2CROSSPOINT_CHANNEL5;
	case 9:	return NTV2CROSSPOINT_CHANNEL6;
	case 10:return NTV2CROSSPOINT_CHANNEL7;
	case 11:return NTV2CROSSPOINT_CHANNEL8;
	case 12:return NTV2CROSSPOINT_INPUT5;
	case 13:return NTV2CROSSPOINT_INPUT6;
	case 14:return NTV2CROSSPOINT_INPUT7;
	case 15:return NTV2CROSSPOINT_INPUT8;
	}
}

ULWord GetIndexForNTV2Crosspoint (const NTV2Crosspoint channel)
{
	switch(channel)
	{
	default:
	case NTV2CROSSPOINT_CHANNEL1:	return 0;
	case NTV2CROSSPOINT_CHANNEL2:	return 1;
	case NTV2CROSSPOINT_CHANNEL3:	return 2;
	case NTV2CROSSPOINT_CHANNEL4:	return 3;
	case NTV2CROSSPOINT_INPUT1:		return 4;
	case NTV2CROSSPOINT_INPUT2:		return 5;
	case NTV2CROSSPOINT_INPUT3:		return 6;
	case NTV2CROSSPOINT_INPUT4:		return 7;
	case NTV2CROSSPOINT_CHANNEL5:	return 8;
	case NTV2CROSSPOINT_CHANNEL6:	return 9;
	case NTV2CROSSPOINT_CHANNEL7:	return 10;
	case NTV2CROSSPOINT_CHANNEL8:	return 11;
	case NTV2CROSSPOINT_INPUT5:		return 12;
	case NTV2CROSSPOINT_INPUT6:		return 13;
	case NTV2CROSSPOINT_INPUT7:		return 14;
	case NTV2CROSSPOINT_INPUT8:		return 15;
	}
}


bool IsNTV2CrosspointInput (const NTV2Crosspoint inChannel)
{
	return NTV2_IS_INPUT_CROSSPOINT (inChannel);
}


bool IsNTV2CrosspointOutput (const NTV2Crosspoint inChannel)
{
	return NTV2_IS_OUTPUT_CROSSPOINT (inChannel);
}


NTV2EmbeddedAudioInput NTV2ChannelToEmbeddedAudioInput (const NTV2Channel inChannel)
{
	NTV2_ASSERT ((NTV2Channel)NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1 == NTV2_CHANNEL1);
	NTV2_ASSERT ((NTV2Channel)NTV2_MAX_NUM_EmbeddedAudioInputs == NTV2_MAX_NUM_CHANNELS);
	return static_cast <NTV2EmbeddedAudioInput> (inChannel);
}


NTV2AudioSystem NTV2ChannelToAudioSystem (const NTV2Channel inChannel)
{
	NTV2_ASSERT ((NTV2Channel)NTV2_AUDIOSYSTEM_1 == NTV2_CHANNEL1);
	NTV2_ASSERT ((NTV2Channel)NTV2_MAX_NUM_AudioSystemEnums == NTV2_MAX_NUM_CHANNELS);
	return static_cast <NTV2AudioSystem> (inChannel);
}


NTV2EmbeddedAudioInput NTV2InputSourceToEmbeddedAudioInput (const NTV2InputSource inInputSource)
{
	static const NTV2EmbeddedAudioInput	gInputSourceToEmbeddedAudioInputs []	= {	/* NTV2_INPUTSOURCE_ANALOG1 */	NTV2_MAX_NUM_EmbeddedAudioInputs,
																					/* NTV2_INPUTSOURCE_HDMI1 */	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1,
																					/* NTV2_INPUTSOURCE_HDMI2 */	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2,
																					/* NTV2_INPUTSOURCE_HDMI3 */	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3,
																					/* NTV2_INPUTSOURCE_HDMI4 */	NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4,
																					/* NTV2_INPUTSOURCE_SDI1 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1,
																					/* NTV2_INPUTSOURCE_SDI2 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2,
																					/* NTV2_INPUTSOURCE_SDI3 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3,
																					/* NTV2_INPUTSOURCE_SDI4 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4,
																					/* NTV2_INPUTSOURCE_SDI5 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_5,
																					/* NTV2_INPUTSOURCE_SDI6 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_6,
																					/* NTV2_INPUTSOURCE_SDI7 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_7,
																					/* NTV2_INPUTSOURCE_SDI8 */		NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_8,
																					/* NTV2_INPUTSOURCE_INVALID */	NTV2_MAX_NUM_EmbeddedAudioInputs};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  inInputSource < (int)(sizeof (gInputSourceToEmbeddedAudioInputs) / sizeof (NTV2EmbeddedAudioInput)))
		return gInputSourceToEmbeddedAudioInputs [inInputSource];
	else
		return NTV2_MAX_NUM_EmbeddedAudioInputs;

}	//	InputSourceToEmbeddedAudioInput


NTV2AudioSource NTV2InputSourceToAudioSource (const NTV2InputSource inInputSource)
{
	if (!NTV2_IS_VALID_INPUT_SOURCE (inInputSource))
		return NTV2_AUDIO_SOURCE_INVALID;
	if (NTV2_INPUT_SOURCE_IS_SDI (inInputSource))
		return NTV2_AUDIO_EMBEDDED;
    else if (NTV2_INPUT_SOURCE_IS_HDMI (inInputSource))
		return NTV2_AUDIO_HDMI;
	else if (NTV2_INPUT_SOURCE_IS_ANALOG (inInputSource))
		return NTV2_AUDIO_ANALOG;
	return NTV2_AUDIO_SOURCE_INVALID;
}


NTV2Crosspoint NTV2ChannelToInputCrosspoint (const NTV2Channel inChannel)
{
	static const NTV2Crosspoint	gChannelToInputChannelSpec []	= {	NTV2CROSSPOINT_INPUT1,	NTV2CROSSPOINT_INPUT2,	NTV2CROSSPOINT_INPUT3,	NTV2CROSSPOINT_INPUT4,
																	NTV2CROSSPOINT_INPUT5,	NTV2CROSSPOINT_INPUT6,	NTV2CROSSPOINT_INPUT7,	NTV2CROSSPOINT_INPUT8, NTV2CROSSPOINT_INVALID};
	if (NTV2_IS_VALID_CHANNEL (inChannel))
		return gChannelToInputChannelSpec [inChannel];
	else
		return NTV2CROSSPOINT_INVALID;
}


NTV2Crosspoint NTV2ChannelToOutputCrosspoint (const NTV2Channel inChannel)
{
	static const NTV2Crosspoint	gChannelToOutputChannelSpec []	= {	NTV2CROSSPOINT_CHANNEL1,	NTV2CROSSPOINT_CHANNEL2,	NTV2CROSSPOINT_CHANNEL3,	NTV2CROSSPOINT_CHANNEL4,
																	NTV2CROSSPOINT_CHANNEL5,	NTV2CROSSPOINT_CHANNEL6,	NTV2CROSSPOINT_CHANNEL7,	NTV2CROSSPOINT_CHANNEL8, NTV2CROSSPOINT_INVALID};
	if (inChannel >= NTV2_CHANNEL1 && inChannel < NTV2_MAX_NUM_CHANNELS)
		return gChannelToOutputChannelSpec [inChannel];
	else
		return NTV2CROSSPOINT_INVALID;
}


INTERRUPT_ENUMS NTV2ChannelToInputInterrupt (const NTV2Channel inChannel)
{
	static const INTERRUPT_ENUMS	gChannelToInputInterrupt []	= {	eInput1, eInput2, eInput3, eInput4, eInput5, eInput6, eInput7, eInput8, eNumInterruptTypes};
	if (NTV2_IS_VALID_CHANNEL (inChannel))
		return gChannelToInputInterrupt [inChannel];
	else
		return eNumInterruptTypes;
}


INTERRUPT_ENUMS NTV2ChannelToOutputInterrupt (const NTV2Channel inChannel)
{
	static const INTERRUPT_ENUMS	gChannelToOutputInterrupt []	= {	eOutput1, eOutput2, eOutput3, eOutput4, eOutput5, eOutput6, eOutput7, eOutput8, eNumInterruptTypes};
	if (NTV2_IS_VALID_CHANNEL (inChannel))
		return gChannelToOutputInterrupt [inChannel];
	else
		return eNumInterruptTypes;
}


static const NTV2TCIndex gChanVITC1[]	= {	NTV2_TCINDEX_SDI1, NTV2_TCINDEX_SDI2, NTV2_TCINDEX_SDI3, NTV2_TCINDEX_SDI4, NTV2_TCINDEX_SDI5, NTV2_TCINDEX_SDI6, NTV2_TCINDEX_SDI7, NTV2_TCINDEX_SDI8};
static const NTV2TCIndex gChanVITC2[]	= {	NTV2_TCINDEX_SDI1_2, NTV2_TCINDEX_SDI2_2, NTV2_TCINDEX_SDI3_2, NTV2_TCINDEX_SDI4_2, NTV2_TCINDEX_SDI5_2, NTV2_TCINDEX_SDI6_2, NTV2_TCINDEX_SDI7_2, NTV2_TCINDEX_SDI8_2};
static const NTV2TCIndex gChanATCLTC[]	= {	NTV2_TCINDEX_SDI1_LTC, NTV2_TCINDEX_SDI2_LTC, NTV2_TCINDEX_SDI3_LTC, NTV2_TCINDEX_SDI4_LTC, NTV2_TCINDEX_SDI5_LTC, NTV2_TCINDEX_SDI6_LTC, NTV2_TCINDEX_SDI7_LTC, NTV2_TCINDEX_SDI8_LTC};


NTV2TCIndex NTV2ChannelToTimecodeIndex (const NTV2Channel inChannel, const bool inEmbeddedLTC, const bool inIsF2)
{
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inEmbeddedLTC ? gChanATCLTC[inChannel] : (inIsF2 ? gChanVITC2[inChannel] : gChanVITC1[inChannel]);
	return NTV2_TCINDEX_INVALID;
}


NTV2TCIndexes GetTCIndexesForSDIConnector (const NTV2Channel inSDI)
{
	NTV2TCIndexes	result;
	if (NTV2_IS_VALID_CHANNEL(inSDI))
		{result.insert(gChanVITC1[inSDI]);	result.insert(gChanVITC2[inSDI]);  result.insert(gChanATCLTC[inSDI]);}
	return result;
}


NTV2Channel NTV2TimecodeIndexToChannel (const NTV2TCIndex inTCIndex)
{
	static const NTV2Channel	gTCIndexToChannel []	= {	NTV2_CHANNEL_INVALID,	NTV2_CHANNEL1,	NTV2_CHANNEL2,	NTV2_CHANNEL3,	NTV2_CHANNEL4,	NTV2_CHANNEL1,	NTV2_CHANNEL2,
															NTV2_CHANNEL1,			NTV2_CHANNEL1,	NTV2_CHANNEL5,	NTV2_CHANNEL6,	NTV2_CHANNEL7,	NTV2_CHANNEL8,
															NTV2_CHANNEL3,			NTV2_CHANNEL4,	NTV2_CHANNEL5,	NTV2_CHANNEL6,	NTV2_CHANNEL7,	NTV2_CHANNEL8,	NTV2_CHANNEL_INVALID};
	return NTV2_IS_VALID_TIMECODE_INDEX (inTCIndex)  ?  gTCIndexToChannel [inTCIndex]  :  NTV2_CHANNEL_INVALID;
}


NTV2InputSource NTV2TimecodeIndexToInputSource (const NTV2TCIndex inTCIndex)
{
	static const NTV2InputSource	gTCIndexToInputSource []	= {	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_SDI1,	NTV2_INPUTSOURCE_SDI2,		NTV2_INPUTSOURCE_SDI3,		NTV2_INPUTSOURCE_SDI4,
																	NTV2_INPUTSOURCE_SDI1,		NTV2_INPUTSOURCE_SDI2,	NTV2_INPUTSOURCE_ANALOG1,	NTV2_INPUTSOURCE_ANALOG1,
																	NTV2_INPUTSOURCE_SDI5,		NTV2_INPUTSOURCE_SDI6,	NTV2_INPUTSOURCE_SDI7,		NTV2_INPUTSOURCE_SDI8,
																	NTV2_INPUTSOURCE_SDI3,		NTV2_INPUTSOURCE_SDI4,	NTV2_INPUTSOURCE_SDI5,		NTV2_INPUTSOURCE_SDI6,
																	NTV2_INPUTSOURCE_SDI7,		NTV2_INPUTSOURCE_SDI8,	NTV2_INPUTSOURCE_INVALID};
	return NTV2_IS_VALID_TIMECODE_INDEX (inTCIndex)  ?  gTCIndexToInputSource [inTCIndex]  :  NTV2_INPUTSOURCE_INVALID;
}


NTV2Crosspoint NTV2InputSourceToChannelSpec (const NTV2InputSource inInputSource)
{
	static const NTV2Crosspoint	gInputSourceToChannelSpec []	= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2CROSSPOINT_INPUT1,
																	/* NTV2_INPUTSOURCE_HDMI1 */		NTV2CROSSPOINT_INPUT1,
																	/* NTV2_INPUTSOURCE_HDMI2 */		NTV2CROSSPOINT_INPUT2,
																	/* NTV2_INPUTSOURCE_HDMI3 */		NTV2CROSSPOINT_INPUT3,
																	/* NTV2_INPUTSOURCE_HDMI4 */		NTV2CROSSPOINT_INPUT4,
																	/* NTV2_INPUTSOURCE_SDI1 */			NTV2CROSSPOINT_INPUT1,
																	/* NTV2_INPUTSOURCE_SDI2 */			NTV2CROSSPOINT_INPUT2,
																	/* NTV2_INPUTSOURCE_SDI3 */			NTV2CROSSPOINT_INPUT3,
																	/* NTV2_INPUTSOURCE_SDI4 */			NTV2CROSSPOINT_INPUT4,
																	/* NTV2_INPUTSOURCE_SDI5 */			NTV2CROSSPOINT_INPUT5,
																	/* NTV2_INPUTSOURCE_SDI6 */			NTV2CROSSPOINT_INPUT6,
																	/* NTV2_INPUTSOURCE_SDI7 */			NTV2CROSSPOINT_INPUT7,
																	/* NTV2_INPUTSOURCE_SDI8 */			NTV2CROSSPOINT_INPUT8,
																	/* NTV2_NUM_INPUTSOURCES */			NTV2_NUM_CROSSPOINTS};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  size_t (inInputSource) < sizeof (gInputSourceToChannelSpec) / sizeof (NTV2Channel))
		return gInputSourceToChannelSpec [inInputSource];
	else
		return NTV2_NUM_CROSSPOINTS;
	
}	//	NTV2InputSourceToChannelSpec


NTV2ReferenceSource NTV2InputSourceToReferenceSource (const NTV2InputSource inInputSource)
{
	static const NTV2ReferenceSource	gInputSourceToReferenceSource []	= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_REFERENCE_ANALOG_INPUT,
																				/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_REFERENCE_HDMI_INPUT1,
																				/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_REFERENCE_HDMI_INPUT2,
																				/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_REFERENCE_HDMI_INPUT3,
																				/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_REFERENCE_HDMI_INPUT4,
																				/* NTV2_INPUTSOURCE_SDI1 */			NTV2_REFERENCE_INPUT1,
																				/* NTV2_INPUTSOURCE_SDI2 */			NTV2_REFERENCE_INPUT2,
																				/* NTV2_INPUTSOURCE_SDI3 */			NTV2_REFERENCE_INPUT3,
																				/* NTV2_INPUTSOURCE_SDI4 */			NTV2_REFERENCE_INPUT4,
																				/* NTV2_INPUTSOURCE_SDI5 */			NTV2_REFERENCE_INPUT5,
																				/* NTV2_INPUTSOURCE_SDI6 */			NTV2_REFERENCE_INPUT6,
																				/* NTV2_INPUTSOURCE_SDI7 */			NTV2_REFERENCE_INPUT7,
																				/* NTV2_INPUTSOURCE_SDI8 */			NTV2_REFERENCE_INPUT8,
																				/* NTV2_NUM_INPUTSOURCES */			NTV2_NUM_REFERENCE_INPUTS};
	if (NTV2_IS_VALID_INPUT_SOURCE (inInputSource)  &&  size_t (inInputSource) < sizeof (gInputSourceToReferenceSource) / sizeof (NTV2ReferenceSource))
		return gInputSourceToReferenceSource [inInputSource];
	else
		return NTV2_NUM_REFERENCE_INPUTS;

}	//	NTV2InputSourceToReferenceSource


NTV2Channel NTV2InputSourceToChannel (const NTV2InputSource inInputSource)
{
	static const NTV2Channel	gInputSourceToChannel []	= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_CHANNEL1,
																/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_CHANNEL1,
																/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_CHANNEL2,
																/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_CHANNEL3,
																/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_CHANNEL4,
																/* NTV2_INPUTSOURCE_SDI1 */			NTV2_CHANNEL1,
																/* NTV2_INPUTSOURCE_SDI2 */			NTV2_CHANNEL2,
																/* NTV2_INPUTSOURCE_SDI3 */			NTV2_CHANNEL3,
																/* NTV2_INPUTSOURCE_SDI4 */			NTV2_CHANNEL4,
																/* NTV2_INPUTSOURCE_SDI5 */			NTV2_CHANNEL5,
																/* NTV2_INPUTSOURCE_SDI6 */			NTV2_CHANNEL6,
																/* NTV2_INPUTSOURCE_SDI7 */			NTV2_CHANNEL7,
																/* NTV2_INPUTSOURCE_SDI8 */			NTV2_CHANNEL8,
																/* NTV2_NUM_INPUTSOURCES */			NTV2_CHANNEL_INVALID};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  size_t (inInputSource) < sizeof (gInputSourceToChannel) / sizeof (NTV2Channel))
		return gInputSourceToChannel [inInputSource];
	else
		return NTV2_MAX_NUM_CHANNELS;

}	//	NTV2InputSourceToChannel


NTV2AudioSystem NTV2InputSourceToAudioSystem (const NTV2InputSource inInputSource)
{
	static const NTV2AudioSystem	gInputSourceToAudioSystem []	= {	/* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_AUDIOSYSTEM_1,
																		/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_AUDIOSYSTEM_1,
																		/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_AUDIOSYSTEM_2,
																		/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_AUDIOSYSTEM_3,
																		/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_AUDIOSYSTEM_4,
																		/* NTV2_INPUTSOURCE_SDI1 */			NTV2_AUDIOSYSTEM_1,
																		/* NTV2_INPUTSOURCE_SDI2 */			NTV2_AUDIOSYSTEM_2,
																		/* NTV2_INPUTSOURCE_SDI3 */			NTV2_AUDIOSYSTEM_3,
																		/* NTV2_INPUTSOURCE_SDI4 */			NTV2_AUDIOSYSTEM_4,
																		/* NTV2_INPUTSOURCE_SDI5 */			NTV2_AUDIOSYSTEM_5,
																		/* NTV2_INPUTSOURCE_SDI6 */			NTV2_AUDIOSYSTEM_6,
																		/* NTV2_INPUTSOURCE_SDI7 */			NTV2_AUDIOSYSTEM_7,
																		/* NTV2_INPUTSOURCE_SDI8 */			NTV2_AUDIOSYSTEM_8,
																		/* NTV2_NUM_INPUTSOURCES */			NTV2_NUM_AUDIOSYSTEMS};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  inInputSource < (int)(sizeof (gInputSourceToAudioSystem) / sizeof (NTV2AudioSystem)))
		return gInputSourceToAudioSystem [inInputSource];
	else
		return NTV2_AUDIOSYSTEM_INVALID;

}	//	NTV2InputSourceToAudioSystem


NTV2TimecodeIndex NTV2InputSourceToTimecodeIndex (const NTV2InputSource inInputSource, const bool inEmbeddedLTC)
{
		static const NTV2TimecodeIndex	gInputSourceToTCIndex []= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_TCINDEX_LTC1,
																	/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_SDI1 */			NTV2_TCINDEX_SDI1,
																	/* NTV2_INPUTSOURCE_SDI2 */			NTV2_TCINDEX_SDI2,
																	/* NTV2_INPUTSOURCE_SDI3 */			NTV2_TCINDEX_SDI3,
																	/* NTV2_INPUTSOURCE_SDI4 */			NTV2_TCINDEX_SDI4,
																	/* NTV2_INPUTSOURCE_SDI5 */			NTV2_TCINDEX_SDI5,
																	/* NTV2_INPUTSOURCE_SDI6 */			NTV2_TCINDEX_SDI6,
																	/* NTV2_INPUTSOURCE_SDI7 */			NTV2_TCINDEX_SDI7,
																	/* NTV2_INPUTSOURCE_SDI8 */			NTV2_TCINDEX_SDI8,
																	/* NTV2_NUM_INPUTSOURCES */			NTV2_TCINDEX_INVALID};
		static const NTV2TimecodeIndex	gInputSourceToLTCIndex []= { /* NTV2_INPUTSOURCE_ANALOG1 */		NTV2_TCINDEX_LTC1,
																	/* NTV2_INPUTSOURCE_HDMI1 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI2 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI3 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_HDMI4 */		NTV2_TCINDEX_INVALID,
																	/* NTV2_INPUTSOURCE_SDI1 */			NTV2_TCINDEX_SDI1_LTC,
																	/* NTV2_INPUTSOURCE_SDI2 */			NTV2_TCINDEX_SDI2_LTC,
																	/* NTV2_INPUTSOURCE_SDI3 */			NTV2_TCINDEX_SDI3_LTC,
																	/* NTV2_INPUTSOURCE_SDI4 */			NTV2_TCINDEX_SDI4_LTC,
																	/* NTV2_INPUTSOURCE_SDI5 */			NTV2_TCINDEX_SDI5_LTC,
																	/* NTV2_INPUTSOURCE_SDI6 */			NTV2_TCINDEX_SDI6_LTC,
																	/* NTV2_INPUTSOURCE_SDI7 */			NTV2_TCINDEX_SDI7_LTC,
																	/* NTV2_INPUTSOURCE_SDI8 */			NTV2_TCINDEX_SDI8_LTC,
																	/* NTV2_NUM_INPUTSOURCES */			NTV2_TCINDEX_INVALID};
	if (inInputSource < NTV2_NUM_INPUTSOURCES  &&  size_t (inInputSource) < sizeof (gInputSourceToTCIndex) / sizeof (NTV2TimecodeIndex))
		return inEmbeddedLTC ? gInputSourceToLTCIndex [inInputSource] : gInputSourceToTCIndex [inInputSource];
	else
		return NTV2_TCINDEX_INVALID;
}


NTV2InputSource NTV2ChannelToInputSource (const NTV2Channel inChannel, const NTV2InputSourceKinds inSourceType)
{
	static const NTV2InputSource	gChannelToSDIInputSource []	=	{	NTV2_INPUTSOURCE_SDI1,		NTV2_INPUTSOURCE_SDI2,		NTV2_INPUTSOURCE_SDI3,		NTV2_INPUTSOURCE_SDI4,
																		NTV2_INPUTSOURCE_SDI5,		NTV2_INPUTSOURCE_SDI6,		NTV2_INPUTSOURCE_SDI7,		NTV2_INPUTSOURCE_SDI8,
																		NTV2_INPUTSOURCE_INVALID	};
	static const NTV2InputSource	gChannelToHDMIInputSource[] =	{	NTV2_INPUTSOURCE_HDMI1,		NTV2_INPUTSOURCE_HDMI2,		NTV2_INPUTSOURCE_HDMI3,		NTV2_INPUTSOURCE_HDMI4,
																		NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,
																		NTV2_INPUTSOURCE_INVALID	};
	static const NTV2InputSource	gChannelToAnlgInputSource[] =	{	NTV2_INPUTSOURCE_ANALOG1,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,
																		NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,	NTV2_INPUTSOURCE_INVALID,
																		NTV2_INPUTSOURCE_INVALID	};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		switch (inSourceType)
		{
			case NTV2_INPUTSOURCES_SDI:		return gChannelToSDIInputSource[inChannel];
			case NTV2_INPUTSOURCES_HDMI:	return gChannelToHDMIInputSource[inChannel];
			case NTV2_INPUTSOURCES_ANALOG:	return gChannelToAnlgInputSource[inChannel];
			default:						break;
		}
	return NTV2_INPUTSOURCE_INVALID;
}


NTV2Channel NTV2OutputDestinationToChannel (const NTV2OutputDestination inOutputDest)
{
	if (!NTV2_IS_VALID_OUTPUT_DEST (inOutputDest))
		return NTV2_CHANNEL_INVALID;

	#if defined (NTV2_DEPRECATE)
		static const NTV2Channel	gOutputDestToChannel []	=	{	NTV2_CHANNEL1,	NTV2_CHANNEL1,
																	NTV2_CHANNEL1,	NTV2_CHANNEL2,	NTV2_CHANNEL3,	NTV2_CHANNEL4,
																	NTV2_CHANNEL5,	NTV2_CHANNEL6,	NTV2_CHANNEL7,	NTV2_CHANNEL8,	NTV2_CHANNEL_INVALID	};
		return gOutputDestToChannel [inOutputDest];
	#else	//	else !defined (NTV2_DEPRECATE)
		switch (inOutputDest)
		{
			default:
			case NTV2_OUTPUTDESTINATION_SDI1:		return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_ANALOG:		return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_SDI2:		return NTV2_CHANNEL2;
			case NTV2_OUTPUTDESTINATION_HDMI:		return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_DUALLINK1:	return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_HDMI_14:		return NTV2_CHANNEL1;
			case NTV2_OUTPUTDESTINATION_DUALLINK2:	return NTV2_CHANNEL2;
			case NTV2_OUTPUTDESTINATION_SDI3:		return NTV2_CHANNEL3;
			case NTV2_OUTPUTDESTINATION_SDI4:		return NTV2_CHANNEL4;
			case NTV2_OUTPUTDESTINATION_SDI5:		return NTV2_CHANNEL5;
			case NTV2_OUTPUTDESTINATION_SDI6:		return NTV2_CHANNEL6;
			case NTV2_OUTPUTDESTINATION_SDI7:		return NTV2_CHANNEL7;
			case NTV2_OUTPUTDESTINATION_SDI8:		return NTV2_CHANNEL8;
			case NTV2_OUTPUTDESTINATION_DUALLINK3:	return NTV2_CHANNEL3;
			case NTV2_OUTPUTDESTINATION_DUALLINK4:	return NTV2_CHANNEL4;
			case NTV2_OUTPUTDESTINATION_DUALLINK5:	return NTV2_CHANNEL5;
			case NTV2_OUTPUTDESTINATION_DUALLINK6:	return NTV2_CHANNEL6;
			case NTV2_OUTPUTDESTINATION_DUALLINK7:	return NTV2_CHANNEL7;
			case NTV2_OUTPUTDESTINATION_DUALLINK8:	return NTV2_CHANNEL8;
		}
	#endif	//	else !defined (NTV2_DEPRECATE)
	return NTV2_CHANNEL_INVALID;
}


NTV2OutputDestination NTV2ChannelToOutputDestination (const NTV2Channel inChannel)
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return NTV2_OUTPUTDESTINATION_INVALID;

	#if defined (NTV2_DEPRECATE)
		static const NTV2OutputDestination	gChannelToOutputDest []	=	{	NTV2_OUTPUTDESTINATION_SDI1,	NTV2_OUTPUTDESTINATION_SDI2,	NTV2_OUTPUTDESTINATION_SDI3,	NTV2_OUTPUTDESTINATION_SDI4,
																			NTV2_OUTPUTDESTINATION_SDI5,	NTV2_OUTPUTDESTINATION_SDI6,	NTV2_OUTPUTDESTINATION_SDI7,	NTV2_OUTPUTDESTINATION_SDI8,
																			NTV2_NUM_OUTPUTDESTINATIONS	};
		return gChannelToOutputDest [inChannel];
	#else	//	else !defined (NTV2_DEPRECATE)
		switch (inChannel)
		{
			default:
			case NTV2_CHANNEL1:		return NTV2_OUTPUTDESTINATION_SDI1;
			case NTV2_CHANNEL2:		return NTV2_OUTPUTDESTINATION_SDI2;
			case NTV2_CHANNEL3:		return NTV2_OUTPUTDESTINATION_SDI3;
			case NTV2_CHANNEL4:		return NTV2_OUTPUTDESTINATION_SDI4;
			case NTV2_CHANNEL5:		return NTV2_OUTPUTDESTINATION_SDI5;
			case NTV2_CHANNEL6:		return NTV2_OUTPUTDESTINATION_SDI6;
			case NTV2_CHANNEL7:		return NTV2_OUTPUTDESTINATION_SDI7;
			case NTV2_CHANNEL8:		return NTV2_OUTPUTDESTINATION_SDI8;
		}
	#endif	//	else !defined (NTV2_DEPRECATE)
}


// if formats are transport equivalent (e.g. 1080i30 / 1080psf30) return the target version of the format
NTV2VideoFormat GetTransportCompatibleFormat (const NTV2VideoFormat inFormat, const NTV2VideoFormat inTargetFormat)
{
	// compatible return target version
	if (::IsTransportCompatibleFormat (inFormat, inTargetFormat))
		return inTargetFormat;

	// not compatible, return original format
	return inFormat;
}


// determine if 2 formats are transport compatible (e.g. 1080i30 / 1080psf30)
bool IsTransportCompatibleFormat (const NTV2VideoFormat inFormat1, const NTV2VideoFormat inFormat2)
{
	if (inFormat1 == inFormat2)
		return true;

	switch (inFormat1)
	{
		case NTV2_FORMAT_1080i_5000:		return inFormat2 == NTV2_FORMAT_1080psf_2500_2;
		case NTV2_FORMAT_1080i_5994:		return inFormat2 == NTV2_FORMAT_1080psf_2997_2;
		case NTV2_FORMAT_1080i_6000:		return inFormat2 == NTV2_FORMAT_1080psf_3000_2;
		case NTV2_FORMAT_1080psf_2500_2:	return inFormat2 == NTV2_FORMAT_1080i_5000;
		case NTV2_FORMAT_1080psf_2997_2:	return inFormat2 == NTV2_FORMAT_1080i_5994;
		case NTV2_FORMAT_1080psf_3000_2:	return inFormat2 == NTV2_FORMAT_1080i_6000;
		default:							break;
	}
	return false;
}


NTV2InputSource GetNTV2InputSourceForIndex (const ULWord inIndex0)
{
	static const NTV2InputSource	sInputSources []	= {	NTV2_INPUTSOURCE_SDI1,	NTV2_INPUTSOURCE_SDI2,	NTV2_INPUTSOURCE_SDI3,	NTV2_INPUTSOURCE_SDI4,
															NTV2_INPUTSOURCE_SDI5,	NTV2_INPUTSOURCE_SDI6,	NTV2_INPUTSOURCE_SDI7,	NTV2_INPUTSOURCE_SDI8};
	if (inIndex0 < sizeof (sInputSources) / sizeof (NTV2InputSource))
		return sInputSources [inIndex0];
	else
		return NTV2_NUM_INPUTSOURCES;
}


NTV2InputSource GetNTV2HDMIInputSourceForIndex (const ULWord inIndex0)
{
	static const NTV2InputSource	sInputSources []	= {	NTV2_INPUTSOURCE_HDMI1,	NTV2_INPUTSOURCE_HDMI2,	NTV2_INPUTSOURCE_HDMI3,	NTV2_INPUTSOURCE_HDMI4};
	if (inIndex0 < sizeof (sInputSources) / sizeof (NTV2InputSource))
		return sInputSources [inIndex0];
	else
		return NTV2_NUM_INPUTSOURCES;
}


ULWord GetIndexForNTV2InputSource (const NTV2InputSource inValue)
{
	static const ULWord	sInputSourcesIndexes []	= {	0,							//	NTV2_INPUTSOURCE_ANALOG1,
													0, 1, 2, 3,					//	NTV2_INPUTSOURCE_HDMI1 ... NTV2_INPUTSOURCE_HDMI4,
													0, 1, 2, 3, 4, 5, 6, 7 };	//	NTV2_INPUTSOURCE_SDI1 ... NTV2_INPUTSOURCE_SDI8
	if (static_cast <size_t> (inValue) < sizeof (sInputSourcesIndexes) / sizeof (ULWord))
		return sInputSourcesIndexes [inValue];
	else
		return 0xFFFFFFFF;

}	//	GetIndexForNTV2InputSource


ULWord NTV2FramesizeToByteCount (const NTV2Framesize inFrameSize)
{
	static ULWord	gFrameSizeToByteCount []	= {	2 /* NTV2_FRAMESIZE_2MB */,		4 /* NTV2_FRAMESIZE_4MB */,		8 /* NTV2_FRAMESIZE_8MB */,		16 /* NTV2_FRAMESIZE_16MB */,
													6 /* NTV2_FRAMESIZE_6MB */,		10 /* NTV2_FRAMESIZE_10MB */,	12 /* NTV2_FRAMESIZE_12MB */,	14 /* NTV2_FRAMESIZE_14MB */,
													18 /* NTV2_FRAMESIZE_18MB */,	20 /* NTV2_FRAMESIZE_20MB */,	22 /* NTV2_FRAMESIZE_22MB */,	24 /* NTV2_FRAMESIZE_24MB */,
													26 /* NTV2_FRAMESIZE_26MB */,	28 /* NTV2_FRAMESIZE_28MB */,	30 /* NTV2_FRAMESIZE_30MB */,	32 /* NTV2_FRAMESIZE_32MB */,
													0	};
	if (inFrameSize < NTV2_MAX_NUM_Framesizes  &&  inFrameSize < (int)(sizeof (gFrameSizeToByteCount) / sizeof (ULWord)))
		return gFrameSizeToByteCount [inFrameSize] * 1024 * 1024;
	else
		return 0;

}	//	NTV2FramesizeToByteCount


ULWord NTV2AudioBufferSizeToByteCount (const NTV2AudioBufferSize inBufferSize)
{													//	NTV2_AUDIO_BUFFER_STANDARD	NTV2_AUDIO_BUFFER_BIG	NTV2_AUDIO_BUFFER_MEDIUM	NTV2_AUDIO_BUFFER_BIGGER	NTV2_AUDIO_BUFFER_INVALID
	static ULWord	gBufferSizeToByteCount []	=	{	1 * 1024 * 1024,			4 * 1024 * 1024,		2 * 1024 * 1024,			3 * 1024 * 1024,			0	};
	if (NTV2_IS_VALID_AUDIO_BUFFER_SIZE (inBufferSize))
		return gBufferSizeToByteCount [inBufferSize];
	return 0;
}

typedef	std::set<NTV2FrameRate>					NTV2FrameRates;
typedef NTV2FrameRates::const_iterator			NTV2FrameRatesConstIter;
typedef std::vector<NTV2FrameRates>				NTV2FrameRateFamilies;
typedef NTV2FrameRateFamilies::const_iterator	NTV2FrameRateFamiliesConstIter;

static NTV2FrameRateFamilies	sFRFamilies;
static AJALock					sFRFamMutex;


static bool CheckFrameRateFamiliesInitialized (void)
{
	if (!sFRFamMutex.IsValid())
		return false;

	AJAAutoLock autoLock (&sFRFamMutex);
	if (sFRFamilies.empty())
	{
		NTV2FrameRates	FR1498, FR1500, FR2398, FR2400, FR2500;
		FR1498.insert(NTV2_FRAMERATE_1498); FR1498.insert(NTV2_FRAMERATE_2997); FR1498.insert(NTV2_FRAMERATE_5994); FR1498.insert(NTV2_FRAMERATE_11988);
		sFRFamilies.push_back(FR1498);
		FR1500.insert(NTV2_FRAMERATE_1500); FR1500.insert(NTV2_FRAMERATE_3000); FR1500.insert(NTV2_FRAMERATE_6000); FR1500.insert(NTV2_FRAMERATE_12000);
		sFRFamilies.push_back(FR1500);
		FR2398.insert(NTV2_FRAMERATE_2398); FR2398.insert(NTV2_FRAMERATE_4795);
		sFRFamilies.push_back(FR2398);
		FR2400.insert(NTV2_FRAMERATE_2400); FR2400.insert(NTV2_FRAMERATE_4800);
		sFRFamilies.push_back(FR2400);
		FR2500.insert(NTV2_FRAMERATE_2500); FR2500.insert(NTV2_FRAMERATE_5000);
		sFRFamilies.push_back(FR2500);
	}
	return !sFRFamilies.empty();
}


NTV2FrameRate GetFrameRateFamily (const NTV2FrameRate inFrameRate)
{
	if (CheckFrameRateFamiliesInitialized())
		for (NTV2FrameRateFamiliesConstIter it(sFRFamilies.begin());  it != sFRFamilies.end();  ++it)
		{
			const NTV2FrameRates &	family (*it);
			NTV2FrameRatesConstIter	iter(family.find(inFrameRate));
			if (iter != family.end())
				return *(family.begin());
		}
	return NTV2_FRAMERATE_INVALID;
}


bool IsMultiFormatCompatible (const NTV2FrameRate inFrameRate1, const NTV2FrameRate inFrameRate2)
{
	if (inFrameRate1 == inFrameRate2)
		return true;

	if (!NTV2_IS_SUPPORTED_NTV2FrameRate(inFrameRate1) || !NTV2_IS_SUPPORTED_NTV2FrameRate(inFrameRate2))
		return false;

	const NTV2FrameRate	frFamily1 (GetFrameRateFamily(inFrameRate1));
	const NTV2FrameRate	frFamily2 (GetFrameRateFamily(inFrameRate2));

	if (!NTV2_IS_SUPPORTED_NTV2FrameRate(frFamily1)  ||  !NTV2_IS_SUPPORTED_NTV2FrameRate(frFamily2))
		return false;	//	Probably uninitialized

	return frFamily1 == frFamily2;

}	//	IsMultiFormatCompatible (NTV2FrameRate)


AJAExport bool IsMultiFormatCompatible (const NTV2VideoFormat inFormat1, const NTV2VideoFormat inFormat2)
{
	if (inFormat1 == NTV2_FORMAT_UNKNOWN || inFormat2 == NTV2_FORMAT_UNKNOWN)
		return false;
	return ::IsMultiFormatCompatible (::GetNTV2FrameRateFromVideoFormat (inFormat1), ::GetNTV2FrameRateFromVideoFormat (inFormat2));

}	//	IsMultiFormatCompatible (NTV2VideoFormat)


AJAExport bool IsPSF(NTV2VideoFormat format)
{
	return NTV2_IS_PSF_VIDEO_FORMAT (format);
}


AJAExport bool IsProgressivePicture(NTV2VideoFormat format)
{
	return NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE (format);
}


AJAExport bool IsProgressiveTransport(NTV2VideoFormat format)
{
	NTV2Standard standard = GetNTV2StandardFromVideoFormat(format);
	return IsProgressiveTransport(standard);
}


AJAExport bool IsProgressiveTransport(NTV2Standard standard)
{
	return NTV2_IS_PROGRESSIVE_STANDARD (standard);
}


AJAExport bool IsRGBFormat(NTV2FrameBufferFormat format)
{
	return NTV2_IS_FBF_RGB (format);
}


AJAExport bool IsYCbCrFormat(NTV2FrameBufferFormat format)
{
	return !NTV2_IS_FBF_RGB (format);	// works for now
}


AJAExport bool IsAlphaChannelFormat(NTV2FrameBufferFormat format)
{
	return NTV2_FBF_HAS_ALPHA (format);
}


AJAExport bool Is2KFormat(NTV2VideoFormat format)
{
	return NTV2_IS_2K_1080_VIDEO_FORMAT (format) || NTV2_IS_2K_VIDEO_FORMAT (format);
}


AJAExport bool Is4KFormat(NTV2VideoFormat format)
{
	return NTV2_IS_4K_4096_VIDEO_FORMAT (format) || NTV2_IS_4K_QUADHD_VIDEO_FORMAT (format);
}


AJAExport bool IsRaw(NTV2FrameBufferFormat frameBufferFormat)
{
	return NTV2_FBF_IS_RAW (frameBufferFormat);
}


AJAExport bool Is8BitFrameBufferFormat(NTV2FrameBufferFormat format)
{
	return NTV2_IS_FBF_8BIT (format);
}


AJAExport bool IsVideoFormatA (NTV2VideoFormat format)
{
	return NTV2_VIDEO_FORMAT_IS_A (format);
}


AJAExport bool IsVideoFormatB(NTV2VideoFormat format)
{
	return NTV2_IS_3Gb_FORMAT (format);
}

AJAExport bool IsVideoFormatJ2KSupported(NTV2VideoFormat format)
{
    return NTV2_VIDEO_FORMAT_IS_J2K_SUPPORTED (format);
}


#if !defined (NTV2_DEPRECATE)
	//
	// BuildRoutingTableForOutput
	// Relative to FrameStore
	// NOTE: convert ignored for now do to excessive laziness
	//
	// Its possible that if you are using this for channel 1 and
	// 2 you could use up resources and unexpected behavior will ensue.
	// for instance, if channel 1 routing uses up both color space converters
	// and then you try to setup channel 2 to use a color space converter.
	// it will overwrite channel 1's routing.
	bool BuildRoutingTableForOutput(CNTV2SignalRouter & outRouter,
									NTV2Channel channel,
									NTV2FrameBufferFormat fbf,
									bool convert, // ignored
									bool lut,
									bool dualLink,
									bool keyOut)	// only check for RGB Formats
									// NOTE: maybe add key out???????
	{
		(void) convert;
		bool status = false;
		bool canHaveKeyOut = false;
		switch (fbf)
		{
		case NTV2_FBF_8BIT_YCBCR_420PL3:
			// not supported
			break;

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
			canHaveKeyOut = true;
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_48BIT_RGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( lut && dualLink )
				{
					// first hook up framestore 1 to lut1,
					outRouter.addWithValue(GetXptLUTInputSelectEntry(),NTV2_XptFrameBuffer1RGB);

					// then hook up lut1 to dualLink
					outRouter.addWithValue(GetDuallinkOutInputSelectEntry(),NTV2_XptLUT1RGB);

					// then hook up dualLink to SDI 1 and SDI 2
					outRouter.addWithValue(GetSDIOut1InputSelectEntry(),NTV2_XptDuallinkOut1);
					status = outRouter.addWithValue(GetSDIOut2InputSelectEntry(),NTV2_XptDuallinkOut1);

				}
				else
					if ( lut )
					{
						// first hook up framestore 1 to lut1,
						outRouter.addWithValue(GetXptLUTInputSelectEntry(),NTV2_XptFrameBuffer1RGB);

						// then hook up lut1 to csc1
						outRouter.addWithValue(GetCSC1VidInputSelectEntry(),NTV2_XptLUT1RGB);

						// then hook up csc1 to sdi 1
						status = outRouter.addWithValue(GetSDIOut1InputSelectEntry(),NTV2_XptCSC1VidYUV);
						if ( keyOut && canHaveKeyOut)
						{
							status = outRouter.addWithValue(GetSDIOut1InputSelectEntry(),NTV2_XptCSC1KeyYUV);
						}
					}
					else
						if ( dualLink)
						{
							// hook up framestore 1 lut duallink
							outRouter.addWithValue(GetDuallinkOutInputSelectEntry(),NTV2_XptFrameBuffer1RGB);

							// then hook up dualLink to SDI 1 and SDI 2
							outRouter.addWithValue(GetSDIOut1InputSelectEntry(),NTV2_XptDuallinkOut1);
							status = outRouter.addWithValue(GetSDIOut2InputSelectEntry(),NTV2_XptDuallinkOut1);
						}
						else
						{
							outRouter.addWithValue(::GetCSC1VidInputSelectEntry(),NTV2_XptFrameBuffer1RGB);
							status = outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptCSC1VidYUV);
							if ( keyOut && canHaveKeyOut)
							{
								status = outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptCSC1KeyYUV);

							}

						}
			}
			else // if channel 2
			{

				// arbitrarily don't support duallink on channel 2
				if ( lut )
				{
					// first hook up framestore 2 to lut2,
					outRouter.addWithValue(::GetXptLUT2InputSelectEntry(),NTV2_XptFrameBuffer2RGB);

					// then hook up lut2 to csc2
					outRouter.addWithValue(::GetCSC2VidInputSelectEntry(),NTV2_XptLUT2RGB);

					// then hook up csc2 to sdi 2 and any other output
					outRouter.addWithValue(::GetSDIOut1InputSelectEntry(), NTV2_XptCSC2VidYUV);
					outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
					outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
					status = outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptCSC2VidYUV);
				}
				else
				{
					outRouter.addWithValue(::GetCSC2VidInputSelectEntry(),NTV2_XptFrameBuffer2RGB);
					outRouter.addWithValue(::GetSDIOut1InputSelectEntry(), NTV2_XptCSC2VidRGB);
					outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptCSC2VidRGB);
					outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptCSC2VidRGB);
					status = outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptCSC2VidRGB);
				}
			}
			break;

		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_HDV:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( lut && dualLink )
				{
					// first uncompress it.
					outRouter.addWithValue(::GetCompressionModInputSelectEntry(),NTV2_XptFrameBuffer1YUV);

					// then send it to color space converter 1
					outRouter.addWithValue(::GetCSC1VidInputSelectEntry(),NTV2_XptCompressionModule);

					// color space convert 1 to lut 1
					outRouter.addWithValue(::GetXptLUTInputSelectEntry(),NTV2_XptCSC1VidRGB);

					// lut 1 to duallink in
					outRouter.addWithValue(::GetDuallinkOutInputSelectEntry(),NTV2_XptLUT1RGB);

					// then hook up dualLink to SDI 1 and SDI 2
					outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptDuallinkOut1);
					status = outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptDuallinkOut1);

				}
				else  // if channel 2
					if ( lut )
					{
						// first uncompress it.
						outRouter.addWithValue(::GetCompressionModInputSelectEntry(),NTV2_XptFrameBuffer1YUV);

						// then send it to color space converter 1
						outRouter.addWithValue(::GetCSC1VidInputSelectEntry(),NTV2_XptCompressionModule);

						// color space convert 1 to lut 1
						outRouter.addWithValue(::GetXptLUTInputSelectEntry(),NTV2_XptCSC1VidRGB);

						// lut 1 to color space convert 2
						outRouter.addWithValue(::GetCSC2VidInputSelectEntry(),NTV2_XptLUT1RGB);

						// color space converter 2 to outputs
						outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptCSC2VidYUV);
						outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
						outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptCSC2VidYUV);
						status = outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptCSC2VidYUV);
					}
					else
					{
						// just send it straight out SDI 1 and other outputs
						outRouter.addWithValue(::GetCompressionModInputSelectEntry(),NTV2_XptFrameBuffer1YUV);
						outRouter.addWithValue(::GetSDIOut2InputSelectEntry(), NTV2_XptCompressionModule);
						outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptCompressionModule);
						outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptCompressionModule);
						status = outRouter.addWithValue(::GetSDIOut1InputSelectEntry(),NTV2_XptCompressionModule);
					}
			}
			else
			{
				// channel 2....an excersize for the user:)
				outRouter.addWithValue(::GetCompressionModInputSelectEntry(),NTV2_XptFrameBuffer1YUV);
				outRouter.addWithValue(::GetSDIOut2InputSelectEntry(), NTV2_XptFrameBuffer2YUV);
				outRouter.addWithValue(::GetHDMIOutInputSelectEntry(), NTV2_XptFrameBuffer2YUV);
				outRouter.addWithValue(::GetAnalogOutInputSelectEntry(), NTV2_XptFrameBuffer2YUV);
				status = outRouter.addWithValue(::GetSDIOut2InputSelectEntry(),NTV2_XptFrameBuffer2YUV);
			}
			break;

		default:
			// YCbCr Stuff
			if ( channel == NTV2_CHANNEL1)
			{
				if ( lut && dualLink )
				{
					// frame store 1 to color space converter 1
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptFrameBuffer1YUV);

					// color space convert 1 to lut 1
					outRouter.addWithValue (::GetXptLUTInputSelectEntry (), NTV2_XptCSC1VidRGB);

					// lut 1 to duallink in
					outRouter.addWithValue (::GetDuallinkOutInputSelectEntry (), NTV2_XptLUT1RGB);

					// then hook up dualLink to SDI 1 and SDI 2
					outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptDuallinkOut1);
					status = outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptDuallinkOut1);

				}
				else
				if ( lut )
				{
					// frame store 1 to color space converter 1
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptFrameBuffer1YUV);

					// color space convert 1 to lut 1
					outRouter.addWithValue (::GetXptLUTInputSelectEntry (), NTV2_XptCSC1VidRGB);

					// lut 1 to color space convert 2
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptLUT1RGB);

					// color space converter 2 to outputs
					outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptCSC2VidYUV);
					outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptCSC2VidYUV);
					outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptCSC2VidYUV);
					status = outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptCSC2VidYUV);
				}
				else
				{
					// just send it straight outputs
					outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptFrameBuffer1YUV);
					outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptFrameBuffer1YUV);
					status = outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptFrameBuffer1YUV);
				}
			}

			else if ( channel == NTV2_CHANNEL2)
			{
				// channel 2....an excersize for the user:)
				outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptFrameBuffer2YUV);
				outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptFrameBuffer2YUV);
				status = outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptFrameBuffer2YUV);
			}

			else if ( channel == NTV2_CHANNEL3)
			{
				// channel 3....an excersize for the user:)
				outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptFrameBuffer3YUV);
				outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptFrameBuffer3YUV);
				status = outRouter.addWithValue (::GetSDIOut3InputSelectEntry (), NTV2_XptFrameBuffer3YUV);
			}

			else if ( channel == NTV2_CHANNEL4)
			{
				// channel 4....an excersize for the user:)
				outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptFrameBuffer4YUV);
				outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptFrameBuffer4YUV);
				status = outRouter.addWithValue (::GetSDIOut4InputSelectEntry (), NTV2_XptFrameBuffer4YUV);
			}
			break;
		}


		return status;
	}

	//
	// BuildRoutingTableForInput
	// Relative to FrameStore
	//
	bool BuildRoutingTableForInput(CNTV2SignalRouter & outRouter,
								   NTV2Channel channel,
								   NTV2FrameBufferFormat fbf,
								   bool withKey,  // only supported for NTV2_CHANNEL1 for rgb formats with alpha
								   bool lut,	  // not supported
								   bool dualLink, // assume coming in RGB(only checked for NTV2_CHANNEL1
								   bool EtoE)
	{
		(void) lut;
		bool status = false;
		if (fbf == NTV2_FBF_8BIT_YCBCR_420PL3)
			return status;

		if ( EtoE)
		{
			if ( dualLink )
			{
				outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);
			}
			else if ( channel == NTV2_CHANNEL1)
			{
				outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
			}
			else if ( channel == NTV2_CHANNEL2)
			{
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);
			}
			else if ( channel == NTV2_CHANNEL3)
			{
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn3);
			}
			else if ( channel == NTV2_CHANNEL4)
			{
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn4);
			}
		}

		switch (fbf)
		{

			case NTV2_FBF_ARGB:
			case NTV2_FBF_RGBA:
			case NTV2_FBF_ABGR:
			case NTV2_FBF_10BIT_ARGB:
			case NTV2_FBF_16BIT_ARGB:
				if ( channel == NTV2_CHANNEL1)
				{
					if ( withKey)
					{
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						outRouter.addWithValue (::GetCSC1KeyInputSelectEntry (), NTV2_XptSDIIn2);
						outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 1);
						status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
					}
					else
					{
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 0);
						status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
					}
				}
				else
				{
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
				}
				break;
			case NTV2_FBF_10BIT_RGB:
			case NTV2_FBF_10BIT_RGB_PACKED:
			case NTV2_FBF_10BIT_DPX:
            case NTV2_FBF_10BIT_DPX_LE:
			case NTV2_FBF_24BIT_BGR:
			case NTV2_FBF_24BIT_RGB:
				if ( channel == NTV2_CHANNEL1)
				{
					if ( dualLink )
					{
						status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptDuallinkIn1);
					}
					else
					{
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
					}
				}
				else
				{
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
				}
				break;
			case NTV2_FBF_8BIT_DVCPRO:
			case NTV2_FBF_8BIT_HDV:
				if ( channel == NTV2_CHANNEL1)
				{
					status = outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn1);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCompressionModule);
				}
				else
				{
					status = outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn2);
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCompressionModule);
				}
				break;

			default:
				if( dualLink )
				{
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn1);
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn2);
				}
				else if ( channel == NTV2_CHANNEL1)
				{
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn1);
				}
				else if ( channel == NTV2_CHANNEL2)
				{
					status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn2);
				}
				else if ( channel == NTV2_CHANNEL3)
				{
					status = outRouter.addWithValue (::GetFrameBuffer3InputSelectEntry (), NTV2_XptSDIIn3);
				}
				else if ( channel == NTV2_CHANNEL4)
				{
					status = outRouter.addWithValue (::GetFrameBuffer4InputSelectEntry (), NTV2_XptSDIIn4);
				}
				break;
		}

		return status;

	}

	//
	// BuildRoutingTableForInput
	// Relative to FrameStore
	//
	bool BuildRoutingTableForInput(CNTV2SignalRouter & outRouter,
								   NTV2Channel channel,
								   NTV2FrameBufferFormat fbf,
								   bool convert,  // Turn on the conversion module
								   bool withKey,  // only supported for NTV2_CHANNEL1 for rgb formats with alpha
								   bool lut,	  // not supported
								   bool dualLink, // assume coming in RGB(only checked for NTV2_CHANNEL1
								   bool EtoE)
	{
		(void) convert;
		(void) lut;
		bool status = false;
		if (fbf == NTV2_FBF_8BIT_YCBCR_420PL3)
			return status;

		if ( EtoE)
		{
			if ( dualLink)
			{
				outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);

			}
			else
				if ( channel == NTV2_CHANNEL1)
				{
					outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
				}
				else
				{
					outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);
				}
		}

		switch (fbf)
		{

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( withKey)
				{
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					outRouter.addWithValue (::GetCSC1KeyInputSelectEntry (), NTV2_XptSDIIn2);
					outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 1);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);

				}
				else
				{
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 0);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
				}
			}
			else
			{
				outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
			}
			break;
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_24BIT_RGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( dualLink )
				{
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptDuallinkIn1);
				}
				else
				{
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
				}
			}
			else
			{
				outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
			}
			break;
		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_HDV:
			if ( channel == NTV2_CHANNEL1)
			{
				status = outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn1);
				status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCompressionModule);
			}
			else
			{
				status = outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn2);
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCompressionModule);
			}
			break;

		default:
			if ( channel == NTV2_CHANNEL1)
			{
				status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn1);
			}
			else
			{
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn2);
			}
			break;
		}

		return status;

	}

	//
	// BuildRoutingTableForInput
	// Includes input and channel now
	//
	bool BuildRoutingTableForInput(CNTV2SignalRouter & outRouter,
								   NTV2InputSource inputSource,
								   NTV2Channel channel,
								   NTV2FrameBufferFormat fbf,
								   bool convert,  // Turn on the conversion module
								   bool withKey,  // only supported for NTV2_CHANNEL1 for rgb formats with alpha
								   bool lut,	  // not supported
								   bool dualLink, // assume coming in RGB(only checked for NTV2_CHANNEL1
								   bool EtoE)
	{
		(void) convert;
		(void) lut;
		bool status = false;
		if (fbf == NTV2_FBF_8BIT_YCBCR_420PL3)
			return status;

		if ( EtoE)
		{
			if ( dualLink)
			{
				outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
				outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);

			}
			else
			{
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetSDIOut1InputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetSDIOut2InputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetAnalogOutInputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetHDMIOutInputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}
			}
		}

		switch (fbf)
		{

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_16BIT_ARGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( withKey)
				{
					outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					outRouter.addWithValue (::GetCSC1KeyInputSelectEntry (), NTV2_XptSDIIn2);
					outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 1);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);

				}
				else
				{
					//outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
					switch ( inputSource )
					{
					case NTV2_INPUTSOURCE_SDI1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						break;
					case NTV2_INPUTSOURCE_SDI2:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn2);
						break;
					case NTV2_INPUTSOURCE_ANALOG1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptAnalogIn);
						break;
					case NTV2_INPUTSOURCE_HDMI1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptHDMIIn1);
						break;
					default:
						return false;
					}

					outRouter.addWithValue (::GetCSC1KeyFromInput2SelectEntry (), 0);
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
				}
			}
			else
			{
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}

				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
			}
			break;
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_RGB_PACKED:
		case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_10BIT_DPX_LE:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_24BIT_RGB:
			if ( channel == NTV2_CHANNEL1)
			{
				if ( dualLink )
				{
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (),NTV2_XptDuallinkIn1);
				}
				else
				{
					switch ( inputSource )
					{
					case NTV2_INPUTSOURCE_SDI1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn1);
						break;
					case NTV2_INPUTSOURCE_SDI2:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptSDIIn2);
						break;
					case NTV2_INPUTSOURCE_ANALOG1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptAnalogIn);
						break;
					case NTV2_INPUTSOURCE_HDMI1:
						outRouter.addWithValue (::GetCSC1VidInputSelectEntry (), NTV2_XptHDMIIn1);
						break;
					default:
						return false;
					}
					status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCSC1VidRGB);
				}
			}
			else
			{
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetCSC2VidInputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCSC2VidRGB);
			}
			break;
		case NTV2_FBF_8BIT_DVCPRO:
		case NTV2_FBF_8BIT_HDV:
			switch ( inputSource )
			{
			case NTV2_INPUTSOURCE_SDI1:
				outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn1);
				break;
			case NTV2_INPUTSOURCE_SDI2:
				outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptSDIIn2);
				break;
			case NTV2_INPUTSOURCE_ANALOG1:
				outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptAnalogIn);
				break;
			case NTV2_INPUTSOURCE_HDMI1:
				outRouter.addWithValue (::GetCompressionModInputSelectEntry (), NTV2_XptHDMIIn1);
				break;
			default:
				return false;
			}
			if ( channel == NTV2_CHANNEL1)
			{
				status = outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptCompressionModule);
			}
			else
			{
				status = outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptCompressionModule);
			}
			break;

		default:
			if ( channel == NTV2_CHANNEL1)
			{
				//status = outRouter.addWithValue(FrameBuffer1InputSelectEntry,NTV2_XptSDIIn1);
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetFrameBuffer1InputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}
			}
			else
			{
				switch ( inputSource )
				{
				case NTV2_INPUTSOURCE_SDI1:
					outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn1);
					break;
				case NTV2_INPUTSOURCE_SDI2:
					outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptSDIIn2);
					break;
				case NTV2_INPUTSOURCE_ANALOG1:
					outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptAnalogIn);
					break;
				case NTV2_INPUTSOURCE_HDMI1:
					outRouter.addWithValue (::GetFrameBuffer2InputSelectEntry (), NTV2_XptHDMIIn1);
					break;
				default:
					return false;
				}
			}
			break;
		}

		return status;
	 }


	// The 10-bit value converted from hex to decimal represents degrees Kelvin.
	// However, the system generates a value that is 5 deg C high. The decimal value
	// minus 273 deg minus 5 deg should be the degrees Centigrade. This is only accurate
	// to 5 deg C, supposedly.

	// These functions used to be a lot more complicated.  Hardware changes
	// reduced them to simple offset & scaling. - STC
	//
	ULWord ConvertFusionAnalogToTempCentigrade(ULWord adc10BitValue)
	{
		// Convert kelvin to centigrade and subtract 5 degrees hot part reports
		// and add empirical 8 degree fudge factor.
		return adc10BitValue -286;

	}

	ULWord ConvertFusionAnalogToMilliVolts(ULWord adc10BitValue, ULWord millivoltsResolution)
	{
		// Different rails have different mv/unit scales.
		return adc10BitValue * millivoltsResolution;
	}
#endif	//	!defined (NTV2_DEPRECATE)


NTV2ConversionMode GetConversionMode( NTV2VideoFormat inFormat, NTV2VideoFormat outFormat)
{
	NTV2ConversionMode cMode = NTV2_CONVERSIONMODE_UNKNOWN;

	switch( inFormat )
	{
		case NTV2_FORMAT_720p_5994:
			if ( outFormat == NTV2_FORMAT_525_5994 )
				cMode = NTV2_720p_5994to525_5994;
			else if ( outFormat == NTV2_FORMAT_1080i_5994)
				cMode = NTV2_720p_5994to1080i_5994;
			else if ( outFormat == NTV2_FORMAT_1080psf_2997_2)
				cMode = NTV2_720p_5994to1080i_5994;
			break;

		case NTV2_FORMAT_720p_5000:
			if ( outFormat == NTV2_FORMAT_625_5000 )
				cMode = NTV2_720p_5000to625_2500;
			else if ( outFormat == NTV2_FORMAT_1080i_5000)	//	NTV2_FORMAT_1080psf_2500
				cMode = NTV2_720p_5000to1080i_2500;
			else if ( outFormat == NTV2_FORMAT_1080psf_2500_2)
				cMode = NTV2_720p_5000to1080i_2500;
			break;

		case NTV2_FORMAT_525_2398:
			if ( outFormat == NTV2_FORMAT_1080psf_2398 )
				cMode = NTV2_525_2398to1080i_2398;
			break;

		case NTV2_FORMAT_525_5994:
			if ( outFormat == NTV2_FORMAT_1080i_5994 )
				cMode = NTV2_525_5994to1080i_5994;
			else if (outFormat == NTV2_FORMAT_1080psf_2997_2)
				cMode = NTV2_525_5994to1080i_5994;
			else if ( outFormat == NTV2_FORMAT_720p_5994 )
				cMode = NTV2_525_5994to720p_5994;
			else if ( outFormat == NTV2_FORMAT_525_5994 )
				cMode = NTV2_525_5994to525_5994;
			else if ( outFormat == NTV2_FORMAT_525psf_2997 )
				cMode = NTV2_525_5994to525psf_2997;
			break;

		case NTV2_FORMAT_625_5000:
			if ( outFormat == NTV2_FORMAT_1080i_5000)	//	NTV2_FORMAT_1080psf_2500
				cMode = NTV2_625_2500to1080i_2500;
			else if ( outFormat == NTV2_FORMAT_1080psf_2500_2)
				cMode = NTV2_625_2500to1080i_2500;
			else if ( outFormat == NTV2_FORMAT_720p_5000 )
				cMode = NTV2_625_2500to720p_5000;
			else if ( outFormat == NTV2_FORMAT_625_5000 )
				cMode = NTV2_625_2500to625_2500;
			else if ( outFormat == NTV2_FORMAT_625psf_2500 )
				cMode = NTV2_625_5000to625psf_2500;
			break;

		case NTV2_FORMAT_720p_6000:
			if ( outFormat == NTV2_FORMAT_1080i_6000)	//	NTV2_FORMAT_1080psf_3000
				cMode = NTV2_720p_6000to1080i_3000;
			else if (outFormat == NTV2_FORMAT_1080psf_3000_2 )
				cMode = NTV2_720p_6000to1080i_3000;
			break;

		case NTV2_FORMAT_1080psf_2398:
			if ( outFormat == NTV2_FORMAT_525_2398 )
				cMode = NTV2_1080i2398to525_2398;
			else if ( outFormat == NTV2_FORMAT_525_5994 )
				cMode = NTV2_1080i2398to525_2997;
			else if ( outFormat == NTV2_FORMAT_720p_2398 )
				cMode = NTV2_1080i_2398to720p_2398;
			else if ( outFormat == NTV2_FORMAT_1080i_5994 )
				cMode = NTV2_1080psf_2398to1080i_5994;
			break;

		case NTV2_FORMAT_1080psf_2400:
			if ( outFormat == NTV2_FORMAT_1080i_6000 )
				cMode = NTV2_1080psf_2400to1080i_3000;
			break;
			
		case NTV2_FORMAT_1080psf_2500_2:
			if ( outFormat == NTV2_FORMAT_625_5000 )
				cMode = NTV2_1080i_2500to625_2500;
			else if ( outFormat == NTV2_FORMAT_720p_5000 )
				cMode = NTV2_1080i_2500to720p_5000;
			else if ( outFormat == NTV2_FORMAT_1080psf_2500_2 )
				cMode = NTV2_1080i_5000to1080psf_2500;
			else if ( outFormat == NTV2_FORMAT_1080psf_2500_2 )
				cMode = NTV2_1080psf_2500to1080i_2500;
			break;
		
		case NTV2_FORMAT_1080p_2398:
			if ( outFormat == NTV2_FORMAT_1080i_5994 )
				cMode = NTV2_1080p_2398to1080i_5994;
			break;
		
		case NTV2_FORMAT_1080p_2400:
			if ( outFormat == NTV2_FORMAT_1080i_6000 )
				cMode = NTV2_1080p_2400to1080i_3000;
			break;
			
		case NTV2_FORMAT_1080p_2500:
			if ( outFormat == NTV2_FORMAT_1080i_5000 )
				cMode = NTV2_1080p_2500to1080i_2500;
			break;

		case NTV2_FORMAT_1080i_5000:
			if ( outFormat == NTV2_FORMAT_625_5000 )
				cMode = NTV2_1080i_2500to625_2500;
			else if ( outFormat == NTV2_FORMAT_720p_5000 )
				cMode = NTV2_1080i_2500to720p_5000;
			else if ( outFormat == NTV2_FORMAT_1080psf_2500_2 )
				cMode = NTV2_1080i_5000to1080psf_2500;
			break;

		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080i_5994:
			if ( outFormat == NTV2_FORMAT_525_5994 )
				cMode = NTV2_1080i_5994to525_5994;
			else if ( outFormat == NTV2_FORMAT_720p_5994 )
				cMode = NTV2_1080i_5994to720p_5994;
			else if ( outFormat == NTV2_FORMAT_1080psf_2997_2 )
				cMode = NTV2_1080i_5994to1080psf_2997;
			break;

		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_1080i_6000:
			if ( outFormat == NTV2_FORMAT_720p_6000 )
				cMode = NTV2_1080i_3000to720p_6000;
			else if ( outFormat == NTV2_FORMAT_1080psf_3000_2 )
				cMode = NTV2_1080i_6000to1080psf_3000;
			break;

		case NTV2_FORMAT_720p_2398:
			if ( outFormat == NTV2_FORMAT_1080psf_2398 )
				cMode = NTV2_720p_2398to1080i_2398;
			break;

		case NTV2_FORMAT_1080p_3000:
			if ( outFormat == NTV2_FORMAT_720p_6000 )
				cMode = NTV2_1080p_3000to720p_6000;
			break;

		default:
			break;
	}

	return cMode;
}

NTV2VideoFormat GetInputForConversionMode(NTV2ConversionMode conversionMode)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;

	switch( conversionMode )
	{
		case NTV2_525_5994to525_5994: inputFormat = NTV2_FORMAT_525_5994; break;
		case NTV2_525_5994to720p_5994: inputFormat = NTV2_FORMAT_525_5994; break;
		case NTV2_525_5994to1080i_5994: inputFormat = NTV2_FORMAT_525_5994; break;
		case NTV2_525_2398to1080i_2398: inputFormat = NTV2_FORMAT_525_2398; break;
		case NTV2_525_5994to525psf_2997: inputFormat = NTV2_FORMAT_525_5994; break;

		case NTV2_625_2500to625_2500: inputFormat = NTV2_FORMAT_625_5000; break;
		case NTV2_625_2500to720p_5000: inputFormat = NTV2_FORMAT_625_5000; break;
		case NTV2_625_2500to1080i_2500: inputFormat = NTV2_FORMAT_625_5000; break;
		case NTV2_625_5000to625psf_2500: inputFormat = NTV2_FORMAT_625_5000; break;

		case NTV2_720p_5000to625_2500: inputFormat = NTV2_FORMAT_720p_5000; break;
		case NTV2_720p_5000to1080i_2500: inputFormat = NTV2_FORMAT_720p_5000; break;
		case NTV2_720p_5994to525_5994: inputFormat = NTV2_FORMAT_720p_5994; break;
		case NTV2_720p_5994to1080i_5994: inputFormat = NTV2_FORMAT_720p_5994; break;
		case NTV2_720p_6000to1080i_3000: inputFormat = NTV2_FORMAT_720p_6000; break;
		case NTV2_720p_2398to1080i_2398: inputFormat = NTV2_FORMAT_720p_2398; break;

		case NTV2_1080i2398to525_2398: inputFormat = NTV2_FORMAT_1080psf_2398; break;
		case NTV2_1080i2398to525_2997: inputFormat = NTV2_FORMAT_1080psf_2398; break;
		case NTV2_1080i_2398to720p_2398: inputFormat = NTV2_FORMAT_1080psf_2398; break;

		case NTV2_1080i_2500to625_2500: inputFormat = NTV2_FORMAT_1080i_5000; break;
		case NTV2_1080i_2500to720p_5000: inputFormat = NTV2_FORMAT_1080i_5000; break;
		case NTV2_1080i_5994to525_5994: inputFormat = NTV2_FORMAT_1080i_5994; break;
		case NTV2_1080i_5994to720p_5994: inputFormat = NTV2_FORMAT_1080i_5994; break;
		case NTV2_1080i_3000to720p_6000: inputFormat = NTV2_FORMAT_1080i_6000; break;
		case NTV2_1080i_5000to1080psf_2500: inputFormat = NTV2_FORMAT_1080i_5000; break;
		case NTV2_1080i_5994to1080psf_2997: inputFormat = NTV2_FORMAT_1080i_5994; break;
		case NTV2_1080i_6000to1080psf_3000: inputFormat = NTV2_FORMAT_1080i_6000; break;
		case NTV2_1080p_3000to720p_6000: inputFormat = NTV2_FORMAT_1080p_3000; break;

		default: inputFormat = NTV2_FORMAT_UNKNOWN; break;
	}
	return inputFormat;
}


NTV2VideoFormat GetOutputForConversionMode(NTV2ConversionMode conversionMode)
{
	NTV2VideoFormat outputFormat = NTV2_FORMAT_UNKNOWN;

	switch( conversionMode )
	{
		case NTV2_525_5994to525_5994: outputFormat = NTV2_FORMAT_525_5994; break;
		case NTV2_525_5994to720p_5994: outputFormat = NTV2_FORMAT_720p_5994; break;
		case NTV2_525_5994to1080i_5994: outputFormat = NTV2_FORMAT_1080i_5994; break;
		case NTV2_525_2398to1080i_2398: outputFormat = NTV2_FORMAT_1080psf_2398; break;
		case NTV2_525_5994to525psf_2997: outputFormat = NTV2_FORMAT_525psf_2997; break;

		case NTV2_625_2500to625_2500: outputFormat = NTV2_FORMAT_625_5000; break;
		case NTV2_625_2500to720p_5000: outputFormat = NTV2_FORMAT_720p_5000; break;
		case NTV2_625_2500to1080i_2500: outputFormat = NTV2_FORMAT_1080i_5000; break;
		case NTV2_625_5000to625psf_2500: outputFormat = NTV2_FORMAT_625psf_2500; break;

		case NTV2_720p_5000to625_2500: outputFormat = NTV2_FORMAT_625_5000; break;
		case NTV2_720p_5000to1080i_2500: outputFormat = NTV2_FORMAT_1080i_5000; break;
		case NTV2_720p_5994to525_5994: outputFormat = NTV2_FORMAT_525_5994; break;
		case NTV2_720p_5994to1080i_5994: outputFormat = NTV2_FORMAT_1080i_5994; break;
		case NTV2_720p_6000to1080i_3000: outputFormat = NTV2_FORMAT_1080i_6000; break;
		case NTV2_720p_2398to1080i_2398: outputFormat = NTV2_FORMAT_1080psf_2398; break;

		case NTV2_1080i2398to525_2398: outputFormat = NTV2_FORMAT_525_2398; break;
		case NTV2_1080i2398to525_2997: outputFormat = NTV2_FORMAT_525_5994; break;
		case NTV2_1080i_2398to720p_2398: outputFormat = NTV2_FORMAT_720p_2398; break;
		//case NTV2_1080i2400to525_2400: outputFormat = NTV2_FORMAT_525_2400; break;

		//case NTV2_1080p2398to525_2398: outputFormat = NTV2_FORMAT_525_2398; break;
		//case NTV2_1080p2398to525_2997: outputFormat = NTV2_FORMAT_525_5994; break;
		//case NTV2_1080p2400to525_2400: outputFormat = NTV2_FORMAT_525_2400; break;

		case NTV2_1080i_2500to625_2500: outputFormat = NTV2_FORMAT_625_5000; break;
		case NTV2_1080i_2500to720p_5000: outputFormat = NTV2_FORMAT_720p_5000; break;
		case NTV2_1080i_5994to525_5994: outputFormat = NTV2_FORMAT_525_5994; break;
		case NTV2_1080i_5994to720p_5994: outputFormat = NTV2_FORMAT_720p_5994; break;
		case NTV2_1080i_3000to720p_6000: outputFormat = NTV2_FORMAT_720p_6000; break;
		case NTV2_1080i_5000to1080psf_2500: outputFormat = NTV2_FORMAT_1080psf_2500_2; break;
		case NTV2_1080i_5994to1080psf_2997: outputFormat = NTV2_FORMAT_1080psf_2997_2; break;
		case NTV2_1080i_6000to1080psf_3000: outputFormat = NTV2_FORMAT_1080psf_3000_2; break;
		case NTV2_1080p_3000to720p_6000: outputFormat = NTV2_FORMAT_720p_6000; break;
		default: outputFormat = NTV2_FORMAT_UNKNOWN; break;
	}
	return outputFormat;
}


ostream & operator << (ostream & inOutStream, const NTV2FrameDimensions inFrameDimensions)
{
	return inOutStream	<< inFrameDimensions.Width() << "Wx" << inFrameDimensions.Height() << "H";
}


ostream & operator << (ostream & inOutStream, const NTV2SmpteLineNumber & inSmpteLineNumber)
{
	return inSmpteLineNumber.Print (inOutStream);
}


string NTV2ChannelToString (const NTV2Channel inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_CHANNEL1:			return inForRetailDisplay ? "Ch1" : "NTV2_CHANNEL1";
		case NTV2_CHANNEL2:			return inForRetailDisplay ? "Ch2" : "NTV2_CHANNEL2";
		case NTV2_CHANNEL3:			return inForRetailDisplay ? "Ch3" : "NTV2_CHANNEL3";
		case NTV2_CHANNEL4:			return inForRetailDisplay ? "Ch4" : "NTV2_CHANNEL4";
		case NTV2_CHANNEL5:			return inForRetailDisplay ? "Ch5" : "NTV2_CHANNEL5";
		case NTV2_CHANNEL6:			return inForRetailDisplay ? "Ch6" : "NTV2_CHANNEL6";
		case NTV2_CHANNEL7:			return inForRetailDisplay ? "Ch7" : "NTV2_CHANNEL7";
		case NTV2_CHANNEL8:			return inForRetailDisplay ? "Ch8" : "NTV2_CHANNEL8";
		case NTV2_MAX_NUM_CHANNELS:	return inForRetailDisplay ? "n/a" : "NTV2_CHANNEL_INVALID";
	}
	return "";
}


string NTV2AudioSystemToString (const NTV2AudioSystem inValue, const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_AUDIO_SYSTEM (inValue))
		oss << (inCompactDisplay ? "AudSys" : "NTV2_AUDIOSYSTEM_") << (inValue + 1);
	else
		oss << (inCompactDisplay ? "NoAudio" : "NTV2_AUDIOSYSTEM_INVALID");
	return oss.str ();
}


string NTV2AudioRateToString (const NTV2AudioRate inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_AUDIO_48K:			return inForRetailDisplay ? "48kHz" : "NTV2_AUDIO_48K";			break;
		case NTV2_AUDIO_96K:			return inForRetailDisplay ? "48kHz" : "NTV2_AUDIO_48K";			break;
		case NTV2_MAX_NUM_AudioRates:	return inForRetailDisplay ? "???" : "NTV2_AUDIO_RATE_INVALID";	break;
	}
	return "";
}


string NTV2AudioBufferSizeToString (const NTV2AudioBufferSize inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_AUDIO_BUFFER_STANDARD:	return inForRetailDisplay ? "1MB" : "NTV2_AUDIO_BUFFER_STANDARD";
		case NTV2_AUDIO_BUFFER_BIG:			return inForRetailDisplay ? "4MB" : "NTV2_AUDIO_BUFFER_BIG";
		case NTV2_MAX_NUM_AudioBufferSizes:	return inForRetailDisplay ? "???" : "NTV2_MAX_NUM_AudioBufferSizes";
	#if !defined (NTV2_DEPRECATE)
		case NTV2_AUDIO_BUFFER_MEDIUM:		return inForRetailDisplay ? "2MB" : "NTV2_AUDIO_BUFFER_MEDIUM";
		case NTV2_AUDIO_BUFFER_BIGGER:		return inForRetailDisplay ? "8MB" : "NTV2_AUDIO_BUFFER_BIGGER";
	#endif	//	!defined (NTV2_DEPRECATE)
	}
	return "";
}


string NTV2AudioLoopBackToString (const NTV2AudioLoopBack inValue, const bool inForRetailDisplay)
{
	if (inValue == NTV2_AUDIO_LOOPBACK_OFF)
		return inForRetailDisplay ? "Off" : "NTV2_AUDIO_LOOPBACK_OFF";
	else if (inValue == NTV2_AUDIO_LOOPBACK_ON)
		return inForRetailDisplay ? "On" : "NTV2_AUDIO_LOOPBACK_ON";
	else
		return "???";
}


string NTV2EmbeddedAudioClockToString (const NTV2EmbeddedAudioClock	inValue, const bool inForRetailDisplay)
{
	if (inValue == NTV2_EMBEDDED_AUDIO_CLOCK_REFERENCE)
		return inForRetailDisplay ? "from device reference" : "NTV2_EMBEDDED_AUDIO_CLOCK_REFERENCE";
	else if (inValue == NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT)
		return inForRetailDisplay ? "from video input" : "NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT";
	else
		return inForRetailDisplay ? "???" : "NTV2_EMBEDDED_AUDIO_CLOCK_INVALID";
}


string NTV2AudioMonitorSelectToString (const NTV2AudioMonitorSelect	inValue, const bool inForRetailDisplay)
{
	if (NTV2_IS_VALID_AUDIO_MONITOR(inValue))
		return ::NTV2AudioChannelPairToString(inValue, inForRetailDisplay);
	return inForRetailDisplay ? "???" : "NTV2_AUDIO_MONITOR_INVALID";
}


string NTV2CrosspointToString (const NTV2Crosspoint inChannel)
{
	std::ostringstream	oss;
	oss	<< (::IsNTV2CrosspointInput (inChannel) ? "Capture " : "Playout ")
		<< (::IsNTV2CrosspointInput (inChannel) ? ::GetIndexForNTV2CrosspointInput (inChannel) : ::GetIndexForNTV2CrosspointChannel (inChannel)) + 1;
	return oss.str ();
}


string NTV2InputCrosspointIDToString (const NTV2InputCrosspointID inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_XptFrameBuffer1Input:		return inForRetailDisplay	? "FB 1"					: "NTV2_XptFrameBuffer1Input";
		case NTV2_XptFrameBuffer1BInput:	return inForRetailDisplay	? "FB 1 B"					: "NTV2_XptFrameBuffer1BInput";
		case NTV2_XptFrameBuffer2Input:		return inForRetailDisplay	? "FB 2"					: "NTV2_XptFrameBuffer2Input";
		case NTV2_XptFrameBuffer2BInput:	return inForRetailDisplay	? "FB 2 B"					: "NTV2_XptFrameBuffer2BInput";
		case NTV2_XptFrameBuffer3Input:		return inForRetailDisplay	? "FB 3"					: "NTV2_XptFrameBuffer3Input";
		case NTV2_XptFrameBuffer3BInput:	return inForRetailDisplay	? "FB 3 B"					: "NTV2_XptFrameBuffer3BInput";
		case NTV2_XptFrameBuffer4Input:		return inForRetailDisplay	? "FB 4"					: "NTV2_XptFrameBuffer4Input";
		case NTV2_XptFrameBuffer4BInput:	return inForRetailDisplay	? "FB 4 B"					: "NTV2_XptFrameBuffer4BInput";
		case NTV2_XptFrameBuffer5Input:		return inForRetailDisplay	? "FB 5"					: "NTV2_XptFrameBuffer5Input";
		case NTV2_XptFrameBuffer5BInput:	return inForRetailDisplay	? "FB 5 B"					: "NTV2_XptFrameBuffer5BInput";
		case NTV2_XptFrameBuffer6Input:		return inForRetailDisplay	? "FB 6"					: "NTV2_XptFrameBuffer6Input";
		case NTV2_XptFrameBuffer6BInput:	return inForRetailDisplay	? "FB 6 B"					: "NTV2_XptFrameBuffer6BInput";
		case NTV2_XptFrameBuffer7Input:		return inForRetailDisplay	? "FB 7"					: "NTV2_XptFrameBuffer7Input";
		case NTV2_XptFrameBuffer7BInput:	return inForRetailDisplay	? "FB 7 B"					: "NTV2_XptFrameBuffer7BInput";
		case NTV2_XptFrameBuffer8Input:		return inForRetailDisplay	? "FB 8"					: "NTV2_XptFrameBuffer8Input";
		case NTV2_XptFrameBuffer8BInput:	return inForRetailDisplay	? "FB 8 B"					: "NTV2_XptFrameBuffer8BInput";
		case NTV2_XptCSC1VidInput:			return inForRetailDisplay	? "CSC 1 Vid"				: "NTV2_XptCSC1VidInput";
		case NTV2_XptCSC1KeyInput:			return inForRetailDisplay	? "CSC 1 Key"				: "NTV2_XptCSC1KeyInput";
		case NTV2_XptCSC2VidInput:			return inForRetailDisplay	? "CSC 2 Vid"				: "NTV2_XptCSC2VidInput";
		case NTV2_XptCSC2KeyInput:			return inForRetailDisplay	? "CSC 2 Key"				: "NTV2_XptCSC2KeyInput";
		case NTV2_XptCSC3VidInput:			return inForRetailDisplay	? "CSC 3 Vid"				: "NTV2_XptCSC3VidInput";
		case NTV2_XptCSC3KeyInput:			return inForRetailDisplay	? "CSC 3 Key"				: "NTV2_XptCSC3KeyInput";
		case NTV2_XptCSC4VidInput:			return inForRetailDisplay	? "CSC 4 Vid"				: "NTV2_XptCSC4VidInput";
		case NTV2_XptCSC4KeyInput:			return inForRetailDisplay	? "CSC 4 Key"				: "NTV2_XptCSC4KeyInput";
		case NTV2_XptCSC5VidInput:			return inForRetailDisplay	? "CSC 5 Vid"				: "NTV2_XptCSC5VidInput";
		case NTV2_XptCSC5KeyInput:			return inForRetailDisplay	? "CSC 5 Key"				: "NTV2_XptCSC5KeyInput";
		case NTV2_XptCSC6VidInput:			return inForRetailDisplay	? "CSC 6 Vid"				: "NTV2_XptCSC6VidInput";
		case NTV2_XptCSC6KeyInput:			return inForRetailDisplay	? "CSC 6 Key"				: "NTV2_XptCSC6KeyInput";
		case NTV2_XptCSC7VidInput:			return inForRetailDisplay	? "CSC 7 Vid"				: "NTV2_XptCSC7VidInput";
		case NTV2_XptCSC7KeyInput:			return inForRetailDisplay	? "CSC 7 Key"				: "NTV2_XptCSC7KeyInput";
		case NTV2_XptCSC8VidInput:			return inForRetailDisplay	? "CSC 8 Vid"				: "NTV2_XptCSC8VidInput";
		case NTV2_XptCSC8KeyInput:			return inForRetailDisplay	? "CSC 8 Key"				: "NTV2_XptCSC8KeyInput";
		case NTV2_XptLUT1Input:				return inForRetailDisplay	? "LUT 1"					: "NTV2_XptLUT1Input";
		case NTV2_XptLUT2Input:				return inForRetailDisplay	? "LUT 2"					: "NTV2_XptLUT2Input";
		case NTV2_XptLUT3Input:				return inForRetailDisplay	? "LUT 3"					: "NTV2_XptLUT3Input";
		case NTV2_XptLUT4Input:				return inForRetailDisplay	? "LUT 4"					: "NTV2_XptLUT4Input";
		case NTV2_XptLUT5Input:				return inForRetailDisplay	? "LUT 5"					: "NTV2_XptLUT5Input";
		case NTV2_XptLUT6Input:				return inForRetailDisplay	? "LUT 6"					: "NTV2_XptLUT6Input";
		case NTV2_XptLUT7Input:				return inForRetailDisplay	? "LUT 7"					: "NTV2_XptLUT7Input";
		case NTV2_XptLUT8Input:				return inForRetailDisplay	? "LUT 8"					: "NTV2_XptLUT8Input";
		case NTV2_XptSDIOut1Standard:		return inForRetailDisplay	? "SDI Out 1 Standard"		: "NTV2_XptSDIOut1Standard";
		case NTV2_XptSDIOut2Standard:		return inForRetailDisplay	? "SDI Out 2 Standard"		: "NTV2_XptSDIOut2Standard";
		case NTV2_XptSDIOut3Standard:		return inForRetailDisplay	? "SDI Out 3 Standard"		: "NTV2_XptSDIOut3Standard";
		case NTV2_XptSDIOut4Standard:		return inForRetailDisplay	? "SDI Out 4 Standard"		: "NTV2_XptSDIOut4Standard";
		case NTV2_XptSDIOut1Input:			return inForRetailDisplay	? "SDI Out 1"				: "NTV2_XptSDIOut1Input";
		case NTV2_XptSDIOut1InputDS2:		return inForRetailDisplay	? "SDI Out 1 DS2"			: "NTV2_XptSDIOut1InputDS2";
		case NTV2_XptSDIOut2Input:			return inForRetailDisplay	? "SDI Out 2"				: "NTV2_XptSDIOut2Input";
		case NTV2_XptSDIOut2InputDS2:		return inForRetailDisplay	? "SDI Out 2 DS2"			: "NTV2_XptSDIOut2InputDS2";
		case NTV2_XptSDIOut3Input:			return inForRetailDisplay	? "SDI Out 3"				: "NTV2_XptSDIOut3Input";
		case NTV2_XptSDIOut3InputDS2:		return inForRetailDisplay	? "SDI Out 3 DS2"			: "NTV2_XptSDIOut3InputDS2";
		case NTV2_XptSDIOut4Input:			return inForRetailDisplay	? "SDI Out 4"				: "NTV2_XptSDIOut4Input";
		case NTV2_XptSDIOut4InputDS2:		return inForRetailDisplay	? "SDI Out 4 DS2"			: "NTV2_XptSDIOut4InputDS2";
		case NTV2_XptSDIOut5Input:			return inForRetailDisplay	? "SDI Out 5"				: "NTV2_XptSDIOut5Input";
		case NTV2_XptSDIOut5InputDS2:		return inForRetailDisplay	? "SDI Out 5 DS2"			: "NTV2_XptSDIOut5InputDS2";
		case NTV2_XptSDIOut6Input:			return inForRetailDisplay	? "SDI Out 6"				: "NTV2_XptSDIOut6Input";
		case NTV2_XptSDIOut6InputDS2:		return inForRetailDisplay	? "SDI Out 6 DS2"			: "NTV2_XptSDIOut6InputDS2";
		case NTV2_XptSDIOut7Input:			return inForRetailDisplay	? "SDI Out 7"				: "NTV2_XptSDIOut7Input";
		case NTV2_XptSDIOut7InputDS2:		return inForRetailDisplay	? "SDI Out 7 DS2"			: "NTV2_XptSDIOut7InputDS2";
		case NTV2_XptSDIOut8Input:			return inForRetailDisplay	? "SDI Out 8"				: "NTV2_XptSDIOut8Input";
		case NTV2_XptSDIOut8InputDS2:		return inForRetailDisplay	? "SDI Out 8 DS2"			: "NTV2_XptSDIOut8InputDS2";
		case NTV2_XptDualLinkIn1Input:		return inForRetailDisplay	? "DL In 1"					: "NTV2_XptDualLinkIn1Input";
		case NTV2_XptDualLinkIn1DSInput:	return inForRetailDisplay	? "DL In 1 DS"				: "NTV2_XptDualLinkIn1DSInput";
		case NTV2_XptDualLinkIn2Input:		return inForRetailDisplay	? "DL In 2"					: "NTV2_XptDualLinkIn2Input";
		case NTV2_XptDualLinkIn2DSInput:	return inForRetailDisplay	? "DL In 2 DS"				: "NTV2_XptDualLinkIn2DSInput";
		case NTV2_XptDualLinkIn3Input:		return inForRetailDisplay	? "DL In 3"					: "NTV2_XptDualLinkIn3Input";
		case NTV2_XptDualLinkIn3DSInput:	return inForRetailDisplay	? "DL In 3 DS"				: "NTV2_XptDualLinkIn3DSInput";
		case NTV2_XptDualLinkIn4Input:		return inForRetailDisplay	? "DL In 4"					: "NTV2_XptDualLinkIn4Input";
		case NTV2_XptDualLinkIn4DSInput:	return inForRetailDisplay	? "DL In 4 DS"				: "NTV2_XptDualLinkIn4DSInput";
		case NTV2_XptDualLinkIn5Input:		return inForRetailDisplay	? "DL In 5"					: "NTV2_XptDualLinkIn5Input";
		case NTV2_XptDualLinkIn5DSInput:	return inForRetailDisplay	? "DL In 5 DS"				: "NTV2_XptDualLinkIn5DSInput";
		case NTV2_XptDualLinkIn6Input:		return inForRetailDisplay	? "DL In 6"					: "NTV2_XptDualLinkIn6Input";
		case NTV2_XptDualLinkIn6DSInput:	return inForRetailDisplay	? "DL In 6 DS"				: "NTV2_XptDualLinkIn6DSInput";
		case NTV2_XptDualLinkIn7Input:		return inForRetailDisplay	? "DL In 7"					: "NTV2_XptDualLinkIn7Input";
		case NTV2_XptDualLinkIn7DSInput:	return inForRetailDisplay	? "DL In 7 DS"				: "NTV2_XptDualLinkIn7DSInput";
		case NTV2_XptDualLinkIn8Input:		return inForRetailDisplay	? "DL In 8"					: "NTV2_XptDualLinkIn8Input";
		case NTV2_XptDualLinkIn8DSInput:	return inForRetailDisplay	? "DL In 8 DS"				: "NTV2_XptDualLinkIn8DSInput";
		case NTV2_XptDualLinkOut1Input:		return inForRetailDisplay	? "DL Out 1"				: "NTV2_XptDualLinkOut1Input";
		case NTV2_XptDualLinkOut2Input:		return inForRetailDisplay	? "DL Out 2"				: "NTV2_XptDualLinkOut2Input";
		case NTV2_XptDualLinkOut3Input:		return inForRetailDisplay	? "DL Out 3"				: "NTV2_XptDualLinkOut3Input";
		case NTV2_XptDualLinkOut4Input:		return inForRetailDisplay	? "DL Out 4"				: "NTV2_XptDualLinkOut4Input";
		case NTV2_XptDualLinkOut5Input:		return inForRetailDisplay	? "DL Out 5"				: "NTV2_XptDualLinkOut5Input";
		case NTV2_XptDualLinkOut6Input:		return inForRetailDisplay	? "DL Out 6"				: "NTV2_XptDualLinkOut6Input";
		case NTV2_XptDualLinkOut7Input:		return inForRetailDisplay	? "DL Out 7"				: "NTV2_XptDualLinkOut7Input";
		case NTV2_XptDualLinkOut8Input:		return inForRetailDisplay	? "DL Out 8"				: "NTV2_XptDualLinkOut8Input";
		case NTV2_XptMixer1BGKeyInput:		return inForRetailDisplay	? "Mixer 1 BG Key"			: "NTV2_XptMixer1BGKeyInput";
		case NTV2_XptMixer1BGVidInput:		return inForRetailDisplay	? "Mixer 1 BG Vid"			: "NTV2_XptMixer1BGVidInput";
		case NTV2_XptMixer1FGKeyInput:		return inForRetailDisplay	? "Mixer 1 FG Key"			: "NTV2_XptMixer1FGKeyInput";
		case NTV2_XptMixer1FGVidInput:		return inForRetailDisplay	? "Mixer 1 FG Vid"			: "NTV2_XptMixer1FGVidInput";
		case NTV2_XptMixer2BGKeyInput:		return inForRetailDisplay	? "Mixer 2 BG Key"			: "NTV2_XptMixer2BGKeyInput";
		case NTV2_XptMixer2BGVidInput:		return inForRetailDisplay	? "Mixer 2 BG Vid"			: "NTV2_XptMixer2BGVidInput";
		case NTV2_XptMixer2FGKeyInput:		return inForRetailDisplay	? "Mixer 2 FG Key"			: "NTV2_XptMixer2FGKeyInput";
		case NTV2_XptMixer2FGVidInput:		return inForRetailDisplay	? "Mixer 2 FG Vid"			: "NTV2_XptMixer2FGVidInput";
		case NTV2_XptMixer3BGKeyInput:		return inForRetailDisplay	? "Mixer 3 BG Key"			: "NTV2_XptMixer3BGKeyInput";
		case NTV2_XptMixer3BGVidInput:		return inForRetailDisplay	? "Mixer 3 BG Vid"			: "NTV2_XptMixer3BGVidInput";
		case NTV2_XptMixer3FGKeyInput:		return inForRetailDisplay	? "Mixer 3 FG Key"			: "NTV2_XptMixer3FGKeyInput";
		case NTV2_XptMixer3FGVidInput:		return inForRetailDisplay	? "Mixer 3 FG Vid"			: "NTV2_XptMixer3FGVidInput";
		case NTV2_XptMixer4BGKeyInput:		return inForRetailDisplay	? "Mixer 4 BG Key"			: "NTV2_XptMixer4BGKeyInput";
		case NTV2_XptMixer4BGVidInput:		return inForRetailDisplay	? "Mixer 4 BG Vid"			: "NTV2_XptMixer4BGVidInput";
		case NTV2_XptMixer4FGKeyInput:		return inForRetailDisplay	? "Mixer 4 FG Key"			: "NTV2_XptMixer4FGKeyInput";
		case NTV2_XptMixer4FGVidInput:		return inForRetailDisplay	? "Mixer 4 FG Vid"			: "NTV2_XptMixer4FGVidInput";
		case NTV2_XptHDMIOutInput:			return inForRetailDisplay	? "HDMI Out"				: "NTV2_XptHDMIOutInput";		//	case NTV2_XptHDMIOutQ1Input:	return inForRetailDisplay ? "HDMI Out Q1" : "NTV2_XptHDMIOutQ1Input";
		case NTV2_XptHDMIOutQ2Input:		return inForRetailDisplay	? "HDMI Out Q2"				: "NTV2_XptHDMIOutQ2Input";
		case NTV2_XptHDMIOutQ3Input:		return inForRetailDisplay	? "HDMI Out Q3"				: "NTV2_XptHDMIOutQ3Input";
		case NTV2_XptHDMIOutQ4Input:		return inForRetailDisplay	? "HDMI Out Q4"				: "NTV2_XptHDMIOutQ4Input";
		case NTV2_Xpt4KDCQ1Input:			return inForRetailDisplay	? "4K DownConv Q1"			: "NTV2_Xpt4KDCQ1Input";
		case NTV2_Xpt4KDCQ2Input:			return inForRetailDisplay	? "4K DownConv Q2"			: "NTV2_Xpt4KDCQ2Input";
		case NTV2_Xpt4KDCQ3Input:			return inForRetailDisplay	? "4K DownConv Q3"			: "NTV2_Xpt4KDCQ3Input";
		case NTV2_Xpt4KDCQ4Input:			return inForRetailDisplay	? "4K DownConv Q4"			: "NTV2_Xpt4KDCQ4Input";
		case NTV2_Xpt425Mux1AInput:			return inForRetailDisplay	? "425Mux 1A"				: "NTV2_Xpt425Mux1AInput";
		case NTV2_Xpt425Mux1BInput:			return inForRetailDisplay	? "425Mux 1B"				: "NTV2_Xpt425Mux1BInput";
		case NTV2_Xpt425Mux2AInput:			return inForRetailDisplay	? "425Mux 2A"				: "NTV2_Xpt425Mux2AInput";
		case NTV2_Xpt425Mux2BInput:			return inForRetailDisplay	? "425Mux 2B"				: "NTV2_Xpt425Mux2BInput";
		case NTV2_Xpt425Mux3AInput:			return inForRetailDisplay	? "425Mux 3A"				: "NTV2_Xpt425Mux3AInput";
		case NTV2_Xpt425Mux3BInput:			return inForRetailDisplay	? "425Mux 3B"				: "NTV2_Xpt425Mux3BInput";
		case NTV2_Xpt425Mux4AInput:			return inForRetailDisplay	? "425Mux 4A"				: "NTV2_Xpt425Mux4AInput";
		case NTV2_Xpt425Mux4BInput:			return inForRetailDisplay	? "425Mux 4B"				: "NTV2_Xpt425Mux4BInput";
		case NTV2_XptAnalogOutInput:		return inForRetailDisplay	? "Analog Out"				: "NTV2_XptAnalogOutInput";
		case NTV2_XptIICT2Input:			return inForRetailDisplay	? "IICT 2"					: "NTV2_XptIICT2Input";
		case NTV2_XptAnalogOutCompositeOut:	return inForRetailDisplay	? "Analog Composite Out"	: "NTV2_XptAnalogOutCompositeOut";
		case NTV2_XptStereoLeftInput:		return inForRetailDisplay	? "Stereo Left"				: "NTV2_XptStereoLeftInput";
		case NTV2_XptStereoRightInput:		return inForRetailDisplay	? "Stereo Right"			: "NTV2_XptStereoRightInput";
		case NTV2_XptProAmpInput:			return inForRetailDisplay	? "Pro Amp"					: "NTV2_XptProAmpInput";
		case NTV2_XptIICT1Input:			return inForRetailDisplay	? "IICT1"					: "NTV2_XptIICT1Input";
		case NTV2_XptWaterMarker1Input:		return inForRetailDisplay	? "Water Marker 1"			: "NTV2_XptWaterMarker1Input";
		case NTV2_XptWaterMarker2Input:		return inForRetailDisplay	? "Water Marker 2"			: "NTV2_XptWaterMarker2Input";
		case NTV2_XptUpdateRegister:		return inForRetailDisplay	? "Update Register"			: "NTV2_XptUpdateRegister";
		case NTV2_XptConversionMod2Input:	return inForRetailDisplay	? "Conversion Mod 2"		: "NTV2_XptConversionMod2Input";
		case NTV2_XptCompressionModInput:	return inForRetailDisplay	? "Compression Module"		: "NTV2_XptCompressionModInput";
		case NTV2_XptConversionModInput:	return inForRetailDisplay	? "Conversion Module"		: "NTV2_XptConversionModInput";
		case NTV2_XptCSC1KeyFromInput2:		return inForRetailDisplay	? "CSC 1 Key From In 2"		: "NTV2_XptCSC1KeyFromInput2";
		case NTV2_XptFrameSync2Input:		return inForRetailDisplay	? "FrameSync2"				: "NTV2_XptFrameSync2Input";
		case NTV2_XptFrameSync1Input:		return inForRetailDisplay	? "FrameSync1"				: "NTV2_XptFrameSync1Input";
		case NTV2_INPUT_CROSSPOINT_INVALID:	return inForRetailDisplay	? "???"						: "NTV2_INPUT_CROSSPOINT_INVALID";
	}
	return string ();

}	//	NTV2InputCrosspointIDToString


string NTV2OutputCrosspointIDToString	(const NTV2OutputCrosspointID inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_XptBlack:						return inForRetailDisplay ? "Black"						: "NTV2_XptBlack";
		case NTV2_XptSDIIn1:					return inForRetailDisplay ? "SDI In 1"					: "NTV2_XptSDIIn1";
		case NTV2_XptSDIIn1DS2:					return inForRetailDisplay ? "SDI In 1 DS2"				: "NTV2_XptSDIIn1DS2";
		case NTV2_XptSDIIn2:					return inForRetailDisplay ? "SDI In 2"					: "NTV2_XptSDIIn2";
		case NTV2_XptSDIIn2DS2:					return inForRetailDisplay ? "SDI In 2 DS2"				: "NTV2_XptSDIIn2DS2";
		case NTV2_XptLUT1YUV:					return inForRetailDisplay ? "LUT 1 YUV"					: "NTV2_XptLUT1YUV";
		case NTV2_XptCSC1VidYUV:				return inForRetailDisplay ? "CSC 1 Vid YUV"				: "NTV2_XptCSC1VidYUV";
		case NTV2_XptConversionModule:			return inForRetailDisplay ? "Conversion Module" 		: "NTV2_XptConversionModule";
		case NTV2_XptCompressionModule:			return inForRetailDisplay ? "Compression Module"		: "NTV2_XptCompressionModule";
		case NTV2_XptFrameBuffer1YUV:			return inForRetailDisplay ? "FB 1 YUV"					: "NTV2_XptFrameBuffer1YUV";
		case NTV2_XptFrameSync1YUV:				return inForRetailDisplay ? "FrameSync 1 YUV"			: "NTV2_XptFrameSync1YUV";
		case NTV2_XptFrameSync2YUV:				return inForRetailDisplay ? "FrameSync 2 YUV"			: "NTV2_XptFrameSync2YUV";
		case NTV2_XptDuallinkOut1:				return inForRetailDisplay ? "DL Out 1"					: "NTV2_XptDuallinkOut1";
		case NTV2_XptDuallinkOut1DS2:			return inForRetailDisplay ? "DL Out 1 DS2"				: "NTV2_XptDuallinkOut1DS2";
		case NTV2_XptDuallinkOut2:				return inForRetailDisplay ? "DL Out 2"					: "NTV2_XptDuallinkOut2";
		case NTV2_XptDuallinkOut2DS2:			return inForRetailDisplay ? "DL Out 2 DS2"				: "NTV2_XptDuallinkOut2DS2";
		case NTV2_XptDuallinkOut3:				return inForRetailDisplay ? "DL Out 3"					: "NTV2_XptDuallinkOut3";
		case NTV2_XptDuallinkOut3DS2:			return inForRetailDisplay ? "DL Out 3 DS2"				: "NTV2_XptDuallinkOut3DS2";
		case NTV2_XptDuallinkOut4:				return inForRetailDisplay ? "DL Out 4"					: "NTV2_XptDuallinkOut4";
		case NTV2_XptDuallinkOut4DS2:			return inForRetailDisplay ? "DL Out 4 DS2"				: "NTV2_XptDuallinkOut4DS2";
		case NTV2_XptAlphaOut:					return inForRetailDisplay ? "Alpha Out"					: "NTV2_XptAlphaOut";
		case NTV2_XptAnalogIn:					return inForRetailDisplay ? "Analog In"					: "NTV2_XptAnalogIn";
		case NTV2_XptHDMIIn1:					return inForRetailDisplay ? "HDMI In 1"					: "NTV2_XptHDMIIn1";
		case NTV2_XptHDMIIn1Q2:					return inForRetailDisplay ? "HDMI In 1 Q2"				: "NTV2_XptHDMIIn1Q2";
		case NTV2_XptHDMIIn1Q3:					return inForRetailDisplay ? "HDMI In 1 Q3"				: "NTV2_XptHDMIIn1Q3";
		case NTV2_XptHDMIIn1Q4:					return inForRetailDisplay ? "HDMI In 1 Q4"				: "NTV2_XptHDMIIn1Q4";
		case NTV2_XptHDMIIn1RGB:				return inForRetailDisplay ? "HDMI In 1 RGB"				: "NTV2_XptHDMIIn1RGB";
		case NTV2_XptHDMIIn1Q2RGB:				return inForRetailDisplay ? "HDMI In 1 Q2 RGB"			: "NTV2_XptHDMIIn1Q2RGB";
		case NTV2_XptHDMIIn1Q3RGB:				return inForRetailDisplay ? "HDMI In 1 Q3 RGB"			: "NTV2_XptHDMIIn1Q3RGB";
		case NTV2_XptHDMIIn1Q4RGB:				return inForRetailDisplay ? "HDMI In 1 Q4 RGB"			: "NTV2_XptHDMIIn1Q4RGB";
		case NTV2_XptHDMIIn2:					return inForRetailDisplay ? "HDMI In 2"					: "NTV2_XptHDMIIn2";
		case NTV2_XptHDMIIn2Q2:					return inForRetailDisplay ? "HDMI In 2 Q2"				: "NTV2_XptHDMIIn2Q2";
		case NTV2_XptHDMIIn2Q3:					return inForRetailDisplay ? "HDMI In 2 Q3"				: "NTV2_XptHDMIIn2Q3";
		case NTV2_XptHDMIIn2Q4:					return inForRetailDisplay ? "HDMI In 2 Q4"				: "NTV2_XptHDMIIn2Q4";
		case NTV2_XptHDMIIn2RGB:				return inForRetailDisplay ? "HDMI In 2 RGB"				: "NTV2_XptHDMIIn2RGB";
		case NTV2_XptHDMIIn2Q2RGB:				return inForRetailDisplay ? "HDMI In 2 Q2 RGB"			: "NTV2_XptHDMIIn2Q2RGB";
		case NTV2_XptHDMIIn2Q3RGB:				return inForRetailDisplay ? "HDMI In 2 Q3 RGB"			: "NTV2_XptHDMIIn2Q3RGB";
		case NTV2_XptHDMIIn2Q4RGB:				return inForRetailDisplay ? "HDMI In 2 Q4 RGB"			: "NTV2_XptHDMIIn2Q4RGB";
		case NTV2_XptHDMIIn3:					return inForRetailDisplay ? "HDMI In 3"					: "NTV2_XptHDMIIn3";
		case NTV2_XptHDMIIn3RGB:				return inForRetailDisplay ? "HDMI In 3 RGB"				: "NTV2_XptHDMIIn3RGB";
		case NTV2_XptHDMIIn4:					return inForRetailDisplay ? "HDMI In 4"					: "NTV2_XptHDMIIn4";
		case NTV2_XptHDMIIn4RGB:				return inForRetailDisplay ? "HDMI In 4 RGB"				: "NTV2_XptHDMIIn4RGB";
		case NTV2_XptDuallinkIn1:				return inForRetailDisplay ? "DL In 1"					: "NTV2_XptDuallinkIn1";
		case NTV2_XptDuallinkIn2:				return inForRetailDisplay ? "DL In 2"					: "NTV2_XptDuallinkIn2";
		case NTV2_XptDuallinkIn3:				return inForRetailDisplay ? "DL In 3"					: "NTV2_XptDuallinkIn3";
		case NTV2_XptDuallinkIn4:				return inForRetailDisplay ? "DL In 4"					: "NTV2_XptDuallinkIn4";
		case NTV2_XptLUT1RGB:					return inForRetailDisplay ? "LUT 1 RGB"					: "NTV2_XptLUT1RGB";
		case NTV2_XptCSC1VidRGB:				return inForRetailDisplay ? "CSC 1 Vid RGB"				: "NTV2_XptCSC1VidRGB";
		case NTV2_XptFrameBuffer1RGB:			return inForRetailDisplay ? "FB 1 RGB"					: "NTV2_XptFrameBuffer1RGB";
		case NTV2_XptFrameSync1RGB:				return inForRetailDisplay ? "FrameSync 1 RGB"			: "NTV2_XptFrameSync1RGB";
		case NTV2_XptFrameSync2RGB:				return inForRetailDisplay ? "FrameSync 2 RGB"			: "NTV2_XptFrameSync2RGB";
		case NTV2_XptLUT2RGB:					return inForRetailDisplay ? "LUT 2 RGB"					: "NTV2_XptLUT2RGB";
		case NTV2_XptCSC1KeyYUV:				return inForRetailDisplay ? "CSC 1 Key YUV"				: "NTV2_XptCSC1KeyYUV";
		case NTV2_XptFrameBuffer2YUV:			return inForRetailDisplay ? "FB 2 YUV"					: "NTV2_XptFrameBuffer2YUV";
		case NTV2_XptFrameBuffer2RGB:			return inForRetailDisplay ? "FB 2 RGB"					: "NTV2_XptFrameBuffer2RGB";
		case NTV2_XptCSC2VidYUV:				return inForRetailDisplay ? "CSC 2 Vid YUV"				: "NTV2_XptCSC2VidYUV";
		case NTV2_XptCSC2VidRGB:				return inForRetailDisplay ? "CSC 2 Vid RGB"				: "NTV2_XptCSC2VidRGB";
		case NTV2_XptCSC2KeyYUV:				return inForRetailDisplay ? "CSC 2 Key YUV"				: "NTV2_XptCSC2KeyYUV";
		case NTV2_XptMixer1VidYUV:				return inForRetailDisplay ? "Mixer 1 Vid YUV"			: "NTV2_XptMixer1VidYUV";
		case NTV2_XptMixer1KeyYUV:				return inForRetailDisplay ? "Mixer 1 Key YUV"			: "NTV2_XptMixer1KeyYUV";
		case NTV2_XptWaterMarkerRGB:			return inForRetailDisplay ? "Water Marker 1 RGB"		: "NTV2_XptWaterMarkerRGB";
		case NTV2_XptWaterMarkerYUV:			return inForRetailDisplay ? "Water Marker 1 YUV"		: "NTV2_XptWaterMarkerYUV";
		case NTV2_XptWaterMarker2RGB:			return inForRetailDisplay ? "Water Marker 2 RGB"		: "NTV2_XptWaterMarker2RGB";
		case NTV2_XptWaterMarker2YUV:			return inForRetailDisplay ? "Water Marker 2 YUV"		: "NTV2_XptWaterMarker2YUV";
		case NTV2_XptIICTRGB:					return inForRetailDisplay ? "IICT RGB"					: "NTV2_XptIICTRGB";
		case NTV2_XptIICT2RGB:					return inForRetailDisplay ? "IICT 2 RGB"				: "NTV2_XptIICT2RGB";
		case NTV2_XptTestPatternYUV:			return inForRetailDisplay ? "Test Pattern YUV"			: "NTV2_XptTestPatternYUV";
		case NTV2_XptDCIMixerVidYUV:			return inForRetailDisplay ? "DCI Mixer Vid YUV"			: "NTV2_XptDCIMixerVidYUV";
		case NTV2_XptDCIMixerVidRGB:			return inForRetailDisplay ? "DCI Mixer Vid RGB"			: "NTV2_XptDCIMixerVidRGB";
		case NTV2_XptMixer2VidYUV:				return inForRetailDisplay ? "Mixer 2 Vid YUV"			: "NTV2_XptMixer2VidYUV";
		case NTV2_XptMixer2KeyYUV:				return inForRetailDisplay ? "Mixer 2 Key YUV"			: "NTV2_XptMixer2KeyYUV";
		case NTV2_XptStereoCompressorOut:		return inForRetailDisplay ? "Stereo Compressor Out"		: "NTV2_XptStereoCompressorOut";
		case NTV2_XptLUT3Out:					return inForRetailDisplay ? "LUT 3 RGB"					: "NTV2_XptLUT3Out";
		case NTV2_XptLUT4Out:					return inForRetailDisplay ? "LUT 4 RGB"					: "NTV2_XptLUT4Out";
		case NTV2_XptFrameBuffer3YUV:			return inForRetailDisplay ? "FB 3 YUV"					: "NTV2_XptFrameBuffer3YUV";
		case NTV2_XptFrameBuffer3RGB:			return inForRetailDisplay ? "FB 3 RGB"					: "NTV2_XptFrameBuffer3RGB";
		case NTV2_XptFrameBuffer4YUV:			return inForRetailDisplay ? "FB 4 YUV"					: "NTV2_XptFrameBuffer4YUV";
		case NTV2_XptFrameBuffer4RGB:			return inForRetailDisplay ? "FB 4 RGB"					: "NTV2_XptFrameBuffer4RGB";
		case NTV2_XptSDIIn3:					return inForRetailDisplay ? "SDI In 3"					: "NTV2_XptSDIIn3";
		case NTV2_XptSDIIn3DS2:					return inForRetailDisplay ? "SDI In 3 DS2"				: "NTV2_XptSDIIn3DS2";
		case NTV2_XptSDIIn4:					return inForRetailDisplay ? "SDI In 4"					: "NTV2_XptSDIIn4";
		case NTV2_XptSDIIn4DS2:					return inForRetailDisplay ? "SDI In 4 DS2"				: "NTV2_XptSDIIn4DS2";
		case NTV2_XptCSC3VidYUV:				return inForRetailDisplay ? "CSC 3 Vid YUV"				: "NTV2_XptCSC3VidYUV";
		case NTV2_XptCSC3VidRGB:				return inForRetailDisplay ? "CSC 3 Vid RGB"				: "NTV2_XptCSC3VidRGB";
		case NTV2_XptCSC3KeyYUV:				return inForRetailDisplay ? "CSC 3 Key YUV"				: "NTV2_XptCSC3KeyYUV";
		case NTV2_XptCSC4VidYUV:				return inForRetailDisplay ? "CSC 4 Vid YUV"				: "NTV2_XptCSC4VidYUV";
		case NTV2_XptCSC4VidRGB:				return inForRetailDisplay ? "CSC 4 Vid RGB"				: "NTV2_XptCSC4VidRGB";
		case NTV2_XptCSC4KeyYUV:				return inForRetailDisplay ? "CSC 4 Key YUV"				: "NTV2_XptCSC4KeyYUV";
		case NTV2_XptCSC5VidYUV:				return inForRetailDisplay ? "CSC 5 Vid YUV"				: "NTV2_XptCSC5VidYUV";
		case NTV2_XptCSC5VidRGB:				return inForRetailDisplay ? "CSC 5 Vid RGB"				: "NTV2_XptCSC5VidRGB";
		case NTV2_XptCSC5KeyYUV:				return inForRetailDisplay ? "CSC 5 Key YUV"				: "NTV2_XptCSC5KeyYUV";
		case NTV2_XptLUT5Out:					return inForRetailDisplay ? "LUT 5 RGB"					: "NTV2_XptLUT5Out";
		case NTV2_XptDuallinkOut5:				return inForRetailDisplay ? "DL Out 5"					: "NTV2_XptDuallinkOut5";
		case NTV2_XptDuallinkOut5DS2:			return inForRetailDisplay ? "DL Out 5 DS2"				: "NTV2_XptDuallinkOut5DS2";
		case NTV2_Xpt4KDownConverterOut:		return inForRetailDisplay ? "4K DownConv Out"			: "NTV2_Xpt4KDownConverterOut";
		case NTV2_Xpt4KDownConverterOutRGB:		return inForRetailDisplay ? "4K DownConv Out RGB"		: "NTV2_Xpt4KDownConverterOutRGB";
		case NTV2_XptFrameBuffer5YUV:			return inForRetailDisplay ? "FB 5 YUV"					: "NTV2_XptFrameBuffer5YUV";
		case NTV2_XptFrameBuffer5RGB:			return inForRetailDisplay ? "FB 5 RGB"					: "NTV2_XptFrameBuffer5RGB";
		case NTV2_XptFrameBuffer6YUV:			return inForRetailDisplay ? "FB 6 YUV"					: "NTV2_XptFrameBuffer6YUV";
		case NTV2_XptFrameBuffer6RGB:			return inForRetailDisplay ? "FB 6 RGB"					: "NTV2_XptFrameBuffer6RGB";
		case NTV2_XptFrameBuffer7YUV:			return inForRetailDisplay ? "FB 7 YUV"					: "NTV2_XptFrameBuffer7YUV";
		case NTV2_XptFrameBuffer7RGB:			return inForRetailDisplay ? "FB 7 RGB"					: "NTV2_XptFrameBuffer7RGB";
		case NTV2_XptFrameBuffer8YUV:			return inForRetailDisplay ? "FB 8 YUV"					: "NTV2_XptFrameBuffer8YUV";
		case NTV2_XptFrameBuffer8RGB:			return inForRetailDisplay ? "FB 8 RGB"					: "NTV2_XptFrameBuffer8RGB";
		case NTV2_XptSDIIn5:					return inForRetailDisplay ? "SDI In 5"					: "NTV2_XptSDIIn5";
		case NTV2_XptSDIIn5DS2:					return inForRetailDisplay ? "SDI In 5 DS2"				: "NTV2_XptSDIIn5DS2";
		case NTV2_XptSDIIn6:					return inForRetailDisplay ? "SDI In 6"					: "NTV2_XptSDIIn6";
		case NTV2_XptSDIIn6DS2:					return inForRetailDisplay ? "SDI In 6 DS2"				: "NTV2_XptSDIIn6DS2";
		case NTV2_XptSDIIn7:					return inForRetailDisplay ? "SDI In 7"					: "NTV2_XptSDIIn7";
		case NTV2_XptSDIIn7DS2:					return inForRetailDisplay ? "SDI In 7 DS2"				: "NTV2_XptSDIIn7DS2";
		case NTV2_XptSDIIn8:					return inForRetailDisplay ? "SDI In 8"					: "NTV2_XptSDIIn8";
		case NTV2_XptSDIIn8DS2:					return inForRetailDisplay ? "SDI In 8 DS2"				: "NTV2_XptSDIIn8DS2";
		case NTV2_XptCSC6VidYUV:				return inForRetailDisplay ? "CSC 6 Vid YUV"				: "NTV2_XptCSC6VidYUV";
		case NTV2_XptCSC6VidRGB:				return inForRetailDisplay ? "CSC 6 Vid RGB"				: "NTV2_XptCSC6VidRGB";
		case NTV2_XptCSC6KeyYUV:				return inForRetailDisplay ? "CSC 6 Key YUV"				: "NTV2_XptCSC6KeyYUV";
		case NTV2_XptCSC7VidYUV:				return inForRetailDisplay ? "CSC 7 Vid YUV"				: "NTV2_XptCSC7VidYUV";
		case NTV2_XptCSC7VidRGB:				return inForRetailDisplay ? "CSC 7 Vid RGB"				: "NTV2_XptCSC7VidRGB";
		case NTV2_XptCSC7KeyYUV:				return inForRetailDisplay ? "CSC 7 Key YUV"				: "NTV2_XptCSC7KeyYUV";
		case NTV2_XptCSC8VidYUV:				return inForRetailDisplay ? "CSC 8 Vid YUV"				: "NTV2_XptCSC8VidYUV";
		case NTV2_XptCSC8VidRGB:				return inForRetailDisplay ? "CSC 8 Vid RGB"				: "NTV2_XptCSC8VidRGB";
		case NTV2_XptCSC8KeyYUV:				return inForRetailDisplay ? "CSC 8 Key YUV"				: "NTV2_XptCSC8KeyYUV";
		case NTV2_XptLUT6Out:					return inForRetailDisplay ? "LUT 6 RGB"					: "NTV2_XptLUT6Out";
		case NTV2_XptLUT7Out:					return inForRetailDisplay ? "LUT 7 RGB"					: "NTV2_XptLUT7Out";
		case NTV2_XptLUT8Out:					return inForRetailDisplay ? "LUT 8 RGB"					: "NTV2_XptLUT8Out";
		case NTV2_XptDuallinkOut6:				return inForRetailDisplay ? "DL Out 6"					: "NTV2_XptDuallinkOut6";
		case NTV2_XptDuallinkOut6DS2:			return inForRetailDisplay ? "DL Out 6 DS2"				: "NTV2_XptDuallinkOut6DS2";
		case NTV2_XptDuallinkOut7:				return inForRetailDisplay ? "DL Out 7"					: "NTV2_XptDuallinkOut7";
		case NTV2_XptDuallinkOut7DS2:			return inForRetailDisplay ? "DL Out 7 DS2"				: "NTV2_XptDuallinkOut7DS2";
		case NTV2_XptDuallinkOut8:				return inForRetailDisplay ? "DL Out 8"					: "NTV2_XptDuallinkOut8";
		case NTV2_XptDuallinkOut8DS2:			return inForRetailDisplay ? "DL Out 8 DS2"				: "NTV2_XptDuallinkOut8DS2";
		case NTV2_XptMixer3VidYUV:				return inForRetailDisplay ? "Mixer 3 Vid YUV"			: "NTV2_XptMixer3VidYUV";
		case NTV2_XptMixer3KeyYUV:				return inForRetailDisplay ? "Mixer 3 Key YUV"			: "NTV2_XptMixer3KeyYUV";
		case NTV2_XptMixer4VidYUV:				return inForRetailDisplay ? "Mixer 4 Vid YUV"			: "NTV2_XptMixer4VidYUV";
		case NTV2_XptMixer4KeyYUV:				return inForRetailDisplay ? "Mixer 4 Key YUV"			: "NTV2_XptMixer4KeyYUV";
		case NTV2_XptDuallinkIn5:				return inForRetailDisplay ? "DL In 5"					: "NTV2_XptDuallinkIn5";
		case NTV2_XptDuallinkIn6:				return inForRetailDisplay ? "DL In 6"					: "NTV2_XptDuallinkIn6";
		case NTV2_XptDuallinkIn7:				return inForRetailDisplay ? "DL In 7"					: "NTV2_XptDuallinkIn7";
		case NTV2_XptDuallinkIn8:				return inForRetailDisplay ? "DL In 8"					: "NTV2_XptDuallinkIn8";
		case NTV2_Xpt425Mux1AYUV:				return inForRetailDisplay ? "425Mux 1a YUV"				: "NTV2_Xpt425Mux1AYUV";
		case NTV2_Xpt425Mux1ARGB:				return inForRetailDisplay ? "425Mux 1a RGB"				: "NTV2_Xpt425Mux1ARGB";
		case NTV2_Xpt425Mux1BYUV:				return inForRetailDisplay ? "425Mux 1b YUV"				: "NTV2_Xpt425Mux1BYUV";
		case NTV2_Xpt425Mux1BRGB:				return inForRetailDisplay ? "425Mux 1b RGB"				: "NTV2_Xpt425Mux1BRGB";
		case NTV2_Xpt425Mux2AYUV:				return inForRetailDisplay ? "425Mux 2a YUV"				: "NTV2_Xpt425Mux2AYUV";
		case NTV2_Xpt425Mux2ARGB:				return inForRetailDisplay ? "425Mux 2a RGB"				: "NTV2_Xpt425Mux2ARGB";
		case NTV2_Xpt425Mux2BYUV:				return inForRetailDisplay ? "425Mux 2b YUV"				: "NTV2_Xpt425Mux2BYUV";
		case NTV2_Xpt425Mux2BRGB:				return inForRetailDisplay ? "425Mux 2b RGB"				: "NTV2_Xpt425Mux2BRGB";
		case NTV2_Xpt425Mux3AYUV:				return inForRetailDisplay ? "425Mux 3a YUV"				: "NTV2_Xpt425Mux3AYUV";
		case NTV2_Xpt425Mux3ARGB:				return inForRetailDisplay ? "425Mux 3a RGB"				: "NTV2_Xpt425Mux3ARGB";
		case NTV2_Xpt425Mux3BYUV:				return inForRetailDisplay ? "425Mux 3b YUV"				: "NTV2_Xpt425Mux3BYUV";
		case NTV2_Xpt425Mux3BRGB:				return inForRetailDisplay ? "425Mux 3b RGB"				: "NTV2_Xpt425Mux3BRGB";
		case NTV2_Xpt425Mux4AYUV:				return inForRetailDisplay ? "425Mux 4a YUV"				: "NTV2_Xpt425Mux4AYUV";
		case NTV2_Xpt425Mux4ARGB:				return inForRetailDisplay ? "425Mux 4a RGB"				: "NTV2_Xpt425Mux4ARGB";
		case NTV2_Xpt425Mux4BYUV:				return inForRetailDisplay ? "425Mux 4b YUV"				: "NTV2_Xpt425Mux4BYUV";
		case NTV2_Xpt425Mux4BRGB:				return inForRetailDisplay ? "425Mux 4b RGB"				: "NTV2_Xpt425Mux4BRGB";
		case NTV2_XptFrameBuffer1_425YUV:		return inForRetailDisplay ? "425FB 1 YUV"				: "NTV2_XptFrameBuffer1_425YUV";
		case NTV2_XptFrameBuffer1_425RGB:		return inForRetailDisplay ? "425FB 1 RGB"				: "NTV2_XptFrameBuffer1_425RGB";
		case NTV2_XptFrameBuffer2_425YUV:		return inForRetailDisplay ? "425FB 2 YUV"				: "NTV2_XptFrameBuffer2_425YUV";
		case NTV2_XptFrameBuffer2_425RGB:		return inForRetailDisplay ? "425FB 2 RGB"				: "NTV2_XptFrameBuffer2_425RGB";
		case NTV2_XptFrameBuffer3_425YUV:		return inForRetailDisplay ? "425FB 3 YUV"				: "NTV2_XptFrameBuffer3_425YUV";
		case NTV2_XptFrameBuffer3_425RGB:		return inForRetailDisplay ? "425FB 3 RGB"				: "NTV2_XptFrameBuffer3_425RGB";
		case NTV2_XptFrameBuffer4_425YUV:		return inForRetailDisplay ? "425FB 4 YUV"				: "NTV2_XptFrameBuffer4_425YUV";
		case NTV2_XptFrameBuffer4_425RGB:		return inForRetailDisplay ? "425FB 4 RGB"				: "NTV2_XptFrameBuffer4_425RGB";
		case NTV2_XptFrameBuffer5_425YUV:		return inForRetailDisplay ? "425FB 5 YUV"				: "NTV2_XptFrameBuffer5_425YUV";
		case NTV2_XptFrameBuffer5_425RGB:		return inForRetailDisplay ? "425FB 5 RGB"				: "NTV2_XptFrameBuffer5_425RGB";
		case NTV2_XptFrameBuffer6_425YUV:		return inForRetailDisplay ? "425FB 6 YUV"				: "NTV2_XptFrameBuffer6_425YUV";
		case NTV2_XptFrameBuffer6_425RGB:		return inForRetailDisplay ? "425FB 6 RGB"				: "NTV2_XptFrameBuffer6_425RGB";
		case NTV2_XptFrameBuffer7_425YUV:		return inForRetailDisplay ? "425FB 7 YUV"				: "NTV2_XptFrameBuffer7_425YUV";
		case NTV2_XptFrameBuffer7_425RGB:		return inForRetailDisplay ? "425FB 7 RGB"				: "NTV2_XptFrameBuffer7_425RGB";
		case NTV2_XptFrameBuffer8_425YUV:		return inForRetailDisplay ? "425FB 8 YUV"				: "NTV2_XptFrameBuffer8_425YUV";
		case NTV2_XptFrameBuffer8_425RGB:		return inForRetailDisplay ? "425FB 8 RGB"				: "NTV2_XptFrameBuffer8_425RGB";
		case NTV2_XptRuntimeCalc:				return inForRetailDisplay ? "Runtime Calc"				: "NTV2_XptRuntimeCalc";
		#if !defined (NTV2_DEPRECATE)
			case NTV2_XptFS1SecondConverter:	return inForRetailDisplay ? "FS 1 2nd Conv"				: "NTV2_XptFS1SecondConverter";
			case NTV2_XptFS1ProcAmp:			return inForRetailDisplay ? "FS 1 ProcAmp"				: "NTV2_XptFS1ProcAmp";
		#endif	//	!defined (NTV2_DEPRECATE)
		default:								break;
	}	//	switch on inValue
	return string ();
}	//	NTV2OutputCrosspointIDToString


string NTV2WidgetIDToString (const NTV2WidgetID inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		case NTV2_WgtFrameBuffer1:			return inCompactDisplay ? "FB1"				: "NTV2_WgtFrameBuffer1";
		case NTV2_WgtFrameBuffer2:			return inCompactDisplay ? "FB2"				: "NTV2_WgtFrameBuffer2";
		case NTV2_WgtFrameBuffer3:			return inCompactDisplay ? "FB3"				: "NTV2_WgtFrameBuffer3";
		case NTV2_WgtFrameBuffer4:			return inCompactDisplay ? "FB4"				: "NTV2_WgtFrameBuffer4";
		case NTV2_WgtCSC1:					return inCompactDisplay ? "CSC1"			: "NTV2_WgtCSC1";
		case NTV2_WgtCSC2:					return inCompactDisplay ? "CSC2"			: "NTV2_WgtCSC2";
		case NTV2_WgtLUT1:					return inCompactDisplay ? "LUT1"			: "NTV2_WgtLUT1";
		case NTV2_WgtLUT2:					return inCompactDisplay ? "LUT2"			: "NTV2_WgtLUT2";
		case NTV2_WgtFrameSync1:			return inCompactDisplay ? "FS1"				: "NTV2_WgtFrameSync1";
		case NTV2_WgtFrameSync2:			return inCompactDisplay ? "FS2"				: "NTV2_WgtFrameSync2";
		case NTV2_WgtSDIIn1:				return inCompactDisplay ? "SDIIn1"			: "NTV2_WgtSDIIn1";
		case NTV2_WgtSDIIn2:				return inCompactDisplay ? "SDIIn2"			: "NTV2_WgtSDIIn2";
		case NTV2_Wgt3GSDIIn1:				return inCompactDisplay ? "3GSDIIn1"		: "NTV2_Wgt3GSDIIn1";
		case NTV2_Wgt3GSDIIn2:				return inCompactDisplay ? "3GSDIIn2"		: "NTV2_Wgt3GSDIIn2";
		case NTV2_Wgt3GSDIIn3:				return inCompactDisplay ? "3GSDIIn3"		: "NTV2_Wgt3GSDIIn3";
		case NTV2_Wgt3GSDIIn4:				return inCompactDisplay ? "3GSDIIn4"		: "NTV2_Wgt3GSDIIn4";
		case NTV2_WgtSDIOut1:				return inCompactDisplay ? "SDIOut1"			: "NTV2_WgtSDIOut1";
		case NTV2_WgtSDIOut2:				return inCompactDisplay ? "SDIOut2"			: "NTV2_WgtSDIOut2";
		case NTV2_WgtSDIOut3:				return inCompactDisplay ? "SDIOut3"			: "NTV2_WgtSDIOut3";
		case NTV2_WgtSDIOut4:				return inCompactDisplay ? "SDIOut4"			: "NTV2_WgtSDIOut4";
		case NTV2_Wgt3GSDIOut1:				return inCompactDisplay ? "3GSDIOut1"		: "NTV2_Wgt3GSDIOut1";
		case NTV2_Wgt3GSDIOut2:				return inCompactDisplay ? "3GSDIOut2"		: "NTV2_Wgt3GSDIOut2";
		case NTV2_Wgt3GSDIOut3:				return inCompactDisplay ? "3GSDIOut3"		: "NTV2_Wgt3GSDIOut3";
		case NTV2_Wgt3GSDIOut4:				return inCompactDisplay ? "3GSDIOut4"		: "NTV2_Wgt3GSDIOut4";
		case NTV2_WgtDualLinkIn1:			return inCompactDisplay ? "DLIn1"			: "NTV2_WgtDualLinkIn1";
		case NTV2_WgtDualLinkV2In1:			return inCompactDisplay ? "DLv2In1"			: "NTV2_WgtDualLinkV2In1";
		case NTV2_WgtDualLinkV2In2:			return inCompactDisplay ? "DLv2In2"			: "NTV2_WgtDualLinkV2In2";
		case NTV2_WgtDualLinkOut1:			return inCompactDisplay ? "DLOut1"			: "NTV2_WgtDualLinkOut1";
		case NTV2_WgtDualLinkOut2:			return inCompactDisplay ? "DLOut2"			: "NTV2_WgtDualLinkOut2";
		case NTV2_WgtDualLinkV2Out1:		return inCompactDisplay ? "DLv2Out1"		: "NTV2_WgtDualLinkV2Out1";
		case NTV2_WgtDualLinkV2Out2:		return inCompactDisplay ? "DLv2Out2"		: "NTV2_WgtDualLinkV2Out2";
		case NTV2_WgtAnalogIn1:				return inCompactDisplay ? "AnlgIn1"			: "NTV2_WgtAnalogIn1";
		case NTV2_WgtAnalogOut1:			return inCompactDisplay ? "AnlgOut1"		: "NTV2_WgtAnalogOut1";
		case NTV2_WgtAnalogCompositeOut1:	return inCompactDisplay ? "AnlgCompOut1"	: "NTV2_WgtAnalogCompositeOut1";
		case NTV2_WgtHDMIIn1:				return inCompactDisplay ? "HDMIIn1"			: "NTV2_WgtHDMIIn1";
		case NTV2_WgtHDMIOut1:				return inCompactDisplay ? "HDMIOut1"		: "NTV2_WgtHDMIOut1";
		case NTV2_WgtUpDownConverter1:		return inCompactDisplay ? "UDC1"			: "NTV2_WgtUpDownConverter1";
		case NTV2_WgtUpDownConverter2:		return inCompactDisplay ? "UDC2"			: "NTV2_WgtUpDownConverter2";
		case NTV2_WgtMixer1:				return inCompactDisplay ? "Mixer1"			: "NTV2_WgtMixer1";
		case NTV2_WgtCompression1:			return inCompactDisplay ? "Compress1"		: "NTV2_WgtCompression1";
		case NTV2_WgtProcAmp1:				return inCompactDisplay ? "ProcAmp1"		: "NTV2_WgtProcAmp1";
		case NTV2_WgtWaterMarker1:			return inCompactDisplay ? "WaterMrkr1"		: "NTV2_WgtWaterMarker1";
		case NTV2_WgtWaterMarker2:			return inCompactDisplay ? "WaterMrkr1"		: "NTV2_WgtWaterMarker2";
		case NTV2_WgtIICT1:					return inCompactDisplay ? "IICT1"			: "NTV2_WgtIICT1";
		case NTV2_WgtIICT2:					return inCompactDisplay ? "IICT2"			: "NTV2_WgtIICT2";
		case NTV2_WgtTestPattern1:			return inCompactDisplay ? "TestPat1"		: "NTV2_WgtTestPattern1";
		case NTV2_WgtGenLock:				return inCompactDisplay ? "GenLock"			: "NTV2_WgtGenLock";
		case NTV2_WgtDCIMixer1:				return inCompactDisplay ? "DCIMixer1"		: "NTV2_WgtDCIMixer1";
		case NTV2_WgtMixer2:				return inCompactDisplay ? "Mixer2"			: "NTV2_WgtMixer2";
		case NTV2_WgtStereoCompressor:		return inCompactDisplay ? "StereoComp"		: "NTV2_WgtStereoCompressor";
		case NTV2_WgtLUT3:					return inCompactDisplay ? "LUT3"			: "NTV2_WgtLUT3";
		case NTV2_WgtLUT4:					return inCompactDisplay ? "LUT4"			: "NTV2_WgtLUT4";
		case NTV2_WgtDualLinkV2In3:			return inCompactDisplay ? "DLv2In3"			: "NTV2_WgtDualLinkV2In3";
		case NTV2_WgtDualLinkV2In4:			return inCompactDisplay ? "DLV2In4"			: "NTV2_WgtDualLinkV2In4";
		case NTV2_WgtDualLinkV2Out3:		return inCompactDisplay ? "DLv2Out3"		: "NTV2_WgtDualLinkV2Out3";
		case NTV2_WgtDualLinkV2Out4:		return inCompactDisplay ? "DLv2Out4"		: "NTV2_WgtDualLinkV2Out4";
		case NTV2_WgtCSC3:					return inCompactDisplay ? "CSC3"			: "NTV2_WgtCSC3";
		case NTV2_WgtCSC4:					return inCompactDisplay ? "CSC4"			: "NTV2_WgtCSC4";
		case NTV2_WgtHDMIIn1v2:				return inCompactDisplay ? "HDMIv2In1"		: "NTV2_WgtHDMIIn1v2";
		case NTV2_WgtHDMIOut1v2:			return inCompactDisplay ? "HDMIv2Out1"		: "NTV2_WgtHDMIOut1v2";
		case NTV2_WgtSDIMonOut1:			return inCompactDisplay ? "SDIMonOut1"		: "NTV2_WgtSDIMonOut1";
		case NTV2_WgtCSC5:					return inCompactDisplay ? "CSC5"			: "NTV2_WgtCSC5";
		case NTV2_WgtLUT5:					return inCompactDisplay ? "LUT5"			: "NTV2_WgtLUT5";
		case NTV2_WgtDualLinkV2Out5:		return inCompactDisplay ? "DLv2Out5"		: "NTV2_WgtDualLinkV2Out5";
		case NTV2_Wgt4KDownConverter:		return inCompactDisplay ? "4KDC"			: "NTV2_Wgt4KDownConverter";
		case NTV2_Wgt3GSDIIn5:				return inCompactDisplay ? "3GSDIIn5"		: "NTV2_Wgt3GSDIIn5";
		case NTV2_Wgt3GSDIIn6:				return inCompactDisplay ? "3GSDIIn6"		: "NTV2_Wgt3GSDIIn6";
		case NTV2_Wgt3GSDIIn7:				return inCompactDisplay ? "3GSDIIn7"		: "NTV2_Wgt3GSDIIn7";
		case NTV2_Wgt3GSDIIn8:				return inCompactDisplay ? "3GSDIIn8"		: "NTV2_Wgt3GSDIIn8";
		case NTV2_Wgt3GSDIOut5:				return inCompactDisplay ? "3GSDIOut5"		: "NTV2_Wgt3GSDIOut5";
		case NTV2_Wgt3GSDIOut6:				return inCompactDisplay ? "3GSDIOut6"		: "NTV2_Wgt3GSDIOut6";
		case NTV2_Wgt3GSDIOut7:				return inCompactDisplay ? "3GSDIOut7"		: "NTV2_Wgt3GSDIOut7";
		case NTV2_Wgt3GSDIOut8:				return inCompactDisplay ? "3GSDIOut8"		: "NTV2_Wgt3GSDIOut8";
		case NTV2_WgtDualLinkV2In5:			return inCompactDisplay ? "DLv2In5"			: "NTV2_WgtDualLinkV2In5";
		case NTV2_WgtDualLinkV2In6:			return inCompactDisplay ? "DLv2In6"			: "NTV2_WgtDualLinkV2In6";
		case NTV2_WgtDualLinkV2In7:			return inCompactDisplay ? "DLv2In7"			: "NTV2_WgtDualLinkV2In7";
		case NTV2_WgtDualLinkV2In8:			return inCompactDisplay ? "DLv2In8"			: "NTV2_WgtDualLinkV2In8";
		case NTV2_WgtDualLinkV2Out6:		return inCompactDisplay ? "DLv2Out6"		: "NTV2_WgtDualLinkV2Out6";
		case NTV2_WgtDualLinkV2Out7:		return inCompactDisplay ? "DLv2Out7"		: "NTV2_WgtDualLinkV2Out7";
		case NTV2_WgtDualLinkV2Out8:		return inCompactDisplay ? "DLv2Out8"		: "NTV2_WgtDualLinkV2Out8";
		case NTV2_WgtCSC6:					return inCompactDisplay ? "CSC6"			: "NTV2_WgtCSC6";
		case NTV2_WgtCSC7:					return inCompactDisplay ? "CSC7"			: "NTV2_WgtCSC7";
		case NTV2_WgtCSC8:					return inCompactDisplay ? "CSC8"			: "NTV2_WgtCSC6";
		case NTV2_WgtLUT6:					return inCompactDisplay ? "LUT6"			: "NTV2_WgtLUT6";
		case NTV2_WgtLUT7:					return inCompactDisplay ? "LUT7"			: "NTV2_WgtLUT7";
		case NTV2_WgtLUT8:					return inCompactDisplay ? "LUT8"			: "NTV2_WgtLUT8";
		case NTV2_WgtMixer3:				return inCompactDisplay ? "Mixer3"			: "NTV2_WgtMixer3";
		case NTV2_WgtMixer4:				return inCompactDisplay ? "Mixer4"			: "NTV2_WgtMixer4";
		case NTV2_WgtFrameBuffer5:			return inCompactDisplay ? "FB5"				: "NTV2_WgtFrameBuffer5";
		case NTV2_WgtFrameBuffer6:			return inCompactDisplay ? "FB6"				: "NTV2_WgtFrameBuffer6";
		case NTV2_WgtFrameBuffer7:			return inCompactDisplay ? "FB7"				: "NTV2_WgtFrameBuffer7";
		case NTV2_WgtFrameBuffer8:			return inCompactDisplay ? "FB8"				: "NTV2_WgtFrameBuffer8";
		case NTV2_WgtHDMIIn1v3:				return inCompactDisplay ? "HDMIv3In1"		: "NTV2_WgtHDMIIn1v3";
		case NTV2_WgtHDMIOut1v3:			return inCompactDisplay ? "HDMIv3Out1"		: "NTV2_WgtHDMIOut1v3";
		case NTV2_Wgt425Mux1:				return inCompactDisplay ? "425Mux1"			: "NTV2_Wgt425Mux1";
		case NTV2_Wgt425Mux2:				return inCompactDisplay ? "425Mux2"			: "NTV2_Wgt425Mux2";
		case NTV2_Wgt425Mux3:				return inCompactDisplay ? "425Mux3"			: "NTV2_Wgt425Mux3";
		case NTV2_Wgt425Mux4:				return inCompactDisplay ? "425Mux4"			: "NTV2_Wgt425Mux4";
		case NTV2_Wgt12GSDIIn1:				return inCompactDisplay ? "12GSDIIn1"		: "NTV2_Wgt12GSDIIn1";
		case NTV2_Wgt12GSDIIn2:				return inCompactDisplay ? "12GSDIIn2"		: "NTV2_Wgt12GSDIIn2";
		case NTV2_Wgt12GSDIIn3:				return inCompactDisplay ? "12GSDIIn3"		: "NTV2_Wgt12GSDIIn3";
		case NTV2_Wgt12GSDIIn4:				return inCompactDisplay ? "12GSDIIn4"		: "NTV2_Wgt12GSDIIn4";
		case NTV2_Wgt12GSDIOut1:			return inCompactDisplay ? "12GSDIOut1"		: "NTV2_Wgt12GSDIOut1";
		case NTV2_Wgt12GSDIOut2:			return inCompactDisplay ? "12GSDIOut2"		: "NTV2_Wgt12GSDIOut2";
		case NTV2_Wgt12GSDIOut3:			return inCompactDisplay ? "12GSDIOut3"		: "NTV2_Wgt12GSDIOut3";
		case NTV2_Wgt12GSDIOut4:			return inCompactDisplay ? "12GSDIOut4"		: "NTV2_Wgt12GSDIOut4";
		case NTV2_WgtHDMIIn1v4:				return inCompactDisplay ? "HDMIv4In1"		: "NTV2_WgtHDMIIn1v4";
		case NTV2_WgtHDMIIn2v4:				return inCompactDisplay ? "HDMIv4In2"		: "NTV2_WgtHDMIIn2v4";
		case NTV2_WgtHDMIIn3v4:				return inCompactDisplay ? "HDMIv4In3"		: "NTV2_WgtHDMIIn3v4";
		case NTV2_WgtHDMIIn4v4:				return inCompactDisplay ? "HDMIv4In4"		: "NTV2_WgtHDMIIn4v4";
		case NTV2_WgtHDMIOut1v4:			return inCompactDisplay ? "HDMIv4Out1"		: "NTV2_WgtHDMIOut1v4";
		case NTV2_WgtModuleTypeCount:		return inCompactDisplay ? "???"				: "???";
	}
	return "";

}	//	NTV2WidgetIDToString


string NTV2TaskModeToString (const NTV2EveryFrameTaskMode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		case NTV2_DISABLE_TASKS:		return inCompactDisplay ? "Disabled"	: "NTV2_DISABLE_TASKS";
		case NTV2_STANDARD_TASKS:		return inCompactDisplay ? "Standard"	: "NTV2_STANDARD_TASKS";
		case NTV2_OEM_TASKS:			return inCompactDisplay ? "OEM"			: "NTV2_OEM_TASKS";
		case NTV2_TASK_MODE_INVALID:	return inCompactDisplay ? "??"			: "NTV2_TASK_MODE_INVALID";
	}
	return "";
}


string NTV2RegNumSetToString (const NTV2RegisterNumberSet & inObj)
{
	ostringstream	oss;
	oss << inObj;
	return oss.str ();
}


ostream & operator << (ostream & inOutStr, const NTV2RegisterNumberSet & inObj)
{
	inOutStr << "[" << inObj.size () << " regs: ";
	for (NTV2RegNumSetConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStr << ::NTV2RegisterNumberToString (NTV2RegisterNumber (*iter));
		if (++iter != inObj.end ())
			inOutStr << ", ";
	}
	return inOutStr << "]";
}


NTV2RegisterNumberSet & operator << (NTV2RegisterNumberSet & inOutSet, const ULWord inRegisterNumber)
{
	inOutSet.insert (inRegisterNumber);
	return inOutSet;
}


string NTV2TCIndexToString (const NTV2TCIndex inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		case NTV2_TCINDEX_DEFAULT:	return inCompactDisplay ? "DEFAULT"		: "NTV2_TCINDEX_DEFAULT";
		case NTV2_TCINDEX_SDI1:		return inCompactDisplay ? "SDI1-VITC"	: "NTV2_TCINDEX_SDI1";
		case NTV2_TCINDEX_SDI2:		return inCompactDisplay ? "SDI2-VITC"	: "NTV2_TCINDEX_SDI2";
		case NTV2_TCINDEX_SDI3:		return inCompactDisplay ? "SDI3-VITC"	: "NTV2_TCINDEX_SDI3";
		case NTV2_TCINDEX_SDI4:		return inCompactDisplay ? "SDI4-VITC"	: "NTV2_TCINDEX_SDI4";
		case NTV2_TCINDEX_SDI1_LTC:	return inCompactDisplay ? "SDI1-LTC"	: "NTV2_TCINDEX_SDI1_LTC";
		case NTV2_TCINDEX_SDI2_LTC:	return inCompactDisplay ? "SDI2-LTC"	: "NTV2_TCINDEX_SDI2_LTC";
		case NTV2_TCINDEX_LTC1:		return inCompactDisplay ? "LTC1"		: "NTV2_TCINDEX_LTC1";
		case NTV2_TCINDEX_LTC2:		return inCompactDisplay ? "LTC2"		: "NTV2_TCINDEX_LTC2";
		case NTV2_TCINDEX_SDI5:		return inCompactDisplay ? "SDI5-VITC"	: "NTV2_TCINDEX_SDI5";
		case NTV2_TCINDEX_SDI6:		return inCompactDisplay ? "SDI6-VITC"	: "NTV2_TCINDEX_SDI6";
		case NTV2_TCINDEX_SDI7:		return inCompactDisplay ? "SDI7-VITC"	: "NTV2_TCINDEX_SDI7";
		case NTV2_TCINDEX_SDI8:		return inCompactDisplay ? "SDI8-VITC"	: "NTV2_TCINDEX_SDI8";
		case NTV2_TCINDEX_SDI3_LTC:	return inCompactDisplay ? "SDI3-LTC"	: "NTV2_TCINDEX_SDI3_LTC";
		case NTV2_TCINDEX_SDI4_LTC:	return inCompactDisplay ? "SDI4-LTC"	: "NTV2_TCINDEX_SDI4_LTC";
		case NTV2_TCINDEX_SDI5_LTC:	return inCompactDisplay ? "SDI5-LTC"	: "NTV2_TCINDEX_SDI5_LTC";
		case NTV2_TCINDEX_SDI6_LTC:	return inCompactDisplay ? "SDI6-LTC"	: "NTV2_TCINDEX_SDI6_LTC";
		case NTV2_TCINDEX_SDI7_LTC:	return inCompactDisplay ? "SDI7-LTC"	: "NTV2_TCINDEX_SDI7_LTC";
		case NTV2_TCINDEX_SDI8_LTC:	return inCompactDisplay ? "SDI8-LTC"	: "NTV2_TCINDEX_SDI8_LTC";
		case NTV2_TCINDEX_SDI1_2:	return inCompactDisplay ? "SDI1-VITC2"	: "NTV2_TCINDEX_SDI1_2";
		case NTV2_TCINDEX_SDI2_2:	return inCompactDisplay ? "SDI2-VITC2"	: "NTV2_TCINDEX_SDI2_2";
		case NTV2_TCINDEX_SDI3_2:	return inCompactDisplay ? "SDI3-VITC2"	: "NTV2_TCINDEX_SDI3_2";
		case NTV2_TCINDEX_SDI4_2:	return inCompactDisplay ? "SDI4-VITC2"	: "NTV2_TCINDEX_SDI4_2";
		case NTV2_TCINDEX_SDI5_2:	return inCompactDisplay ? "SDI5-VITC2"	: "NTV2_TCINDEX_SDI5_2";
		case NTV2_TCINDEX_SDI6_2:	return inCompactDisplay ? "SDI6-VITC2"	: "NTV2_TCINDEX_SDI6_2";
		case NTV2_TCINDEX_SDI7_2:	return inCompactDisplay ? "SDI7-VITC2"	: "NTV2_TCINDEX_SDI7_2";
		case NTV2_TCINDEX_SDI8_2:	return inCompactDisplay ? "SDI8-VITC2"	: "NTV2_TCINDEX_SDI8_2";
		default:					break;
	}
	return inCompactDisplay ? "" : "NTV2_TCINDEX_INVALID";
}


string NTV2AudioChannelPairToString (const NTV2AudioChannelPair inValue, const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_AUDIO_CHANNEL_PAIR (inValue))
		oss << (inCompactDisplay ? "" : "NTV2_AudioChannel")  <<  (inValue * 2 + 1)  <<  (inCompactDisplay ? "-" : "_")  <<  (inValue * 2 + 2);
	else if (!inCompactDisplay)
		oss << "NTV2_AUDIO_CHANNEL_PAIR_INVALID";
	return oss.str ();
}


string NTV2AudioChannelQuadToString (const NTV2Audio4ChannelSelect inValue, const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_AUDIO_CHANNEL_QUAD (inValue))
		oss << (inCompactDisplay ? "" : "NTV2_AudioChannel")  <<  (inValue * 4 + 1)  <<  (inCompactDisplay ? "-" : "_")  <<  (inValue * 4 + 4);
	else if (!inCompactDisplay)
		oss << "NTV2_AUDIO_CHANNEL_QUAD_INVALID";
	return oss.str ();
}


string NTV2AudioChannelOctetToString (const NTV2Audio8ChannelSelect inValue, const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_AUDIO_CHANNEL_OCTET (inValue))
		oss << (inCompactDisplay ? "" : "NTV2_AudioChannel")  <<  (inValue * 8 + 1)  <<  (inCompactDisplay ? "-" : "_")  <<  (inValue * 8 + 8);
	else if (!inCompactDisplay)
		oss << "NTV2_AUDIO_CHANNEL_OCTET_INVALID";
	return oss.str ();
}


string NTV2FramesizeToString (const NTV2Framesize inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		case NTV2_FRAMESIZE_2MB:	return inCompactDisplay ? "2MB"		: "NTV2_FRAMESIZE_2MB";
		case NTV2_FRAMESIZE_4MB:	return inCompactDisplay ? "4MB"		: "NTV2_FRAMESIZE_4MB";
		case NTV2_FRAMESIZE_8MB:	return inCompactDisplay ? "8MB"		: "NTV2_FRAMESIZE_8MB";
		case NTV2_FRAMESIZE_16MB:	return inCompactDisplay ? "16MB"	: "NTV2_FRAMESIZE_16MB";
		case NTV2_FRAMESIZE_6MB:	return inCompactDisplay ? "6MB"		: "NTV2_FRAMESIZE_6MB";
		case NTV2_FRAMESIZE_10MB:	return inCompactDisplay ? "10MB"	: "NTV2_FRAMESIZE_10MB";
		case NTV2_FRAMESIZE_12MB:	return inCompactDisplay ? "12MB"	: "NTV2_FRAMESIZE_12MB";
		case NTV2_FRAMESIZE_14MB:	return inCompactDisplay ? "14MB"	: "NTV2_FRAMESIZE_14MB";
		case NTV2_FRAMESIZE_18MB:	return inCompactDisplay ? "18MB"	: "NTV2_FRAMESIZE_18MB";
		case NTV2_FRAMESIZE_20MB:	return inCompactDisplay ? "20MB"	: "NTV2_FRAMESIZE_20MB";
		case NTV2_FRAMESIZE_22MB:	return inCompactDisplay ? "22MB"	: "NTV2_FRAMESIZE_22MB";
		case NTV2_FRAMESIZE_24MB:	return inCompactDisplay ? "24MB"	: "NTV2_FRAMESIZE_24MB";
		case NTV2_FRAMESIZE_26MB:	return inCompactDisplay ? "26MB"	: "NTV2_FRAMESIZE_26MB";
		case NTV2_FRAMESIZE_28MB:	return inCompactDisplay ? "28MB"	: "NTV2_FRAMESIZE_28MB";
		case NTV2_FRAMESIZE_30MB:	return inCompactDisplay ? "30MB"	: "NTV2_FRAMESIZE_30MB";
		case NTV2_FRAMESIZE_32MB:	return inCompactDisplay ? "32MB"	: "NTV2_FRAMESIZE_32MB";
		default:					break;
	}
	return inCompactDisplay ? "" : "NTV2_FRAMESIZE_INVALID";
}


string NTV2ModeToString (const NTV2Mode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		case NTV2_MODE_DISPLAY:		return inCompactDisplay ? "Output"	: "NTV2_MODE_DISPLAY";
		case NTV2_MODE_CAPTURE:		return inCompactDisplay ? "Input"	: "NTV2_MODE_CAPTURE";
		default:					break;
	}
	return inCompactDisplay ? "" : "NTV2_MODE_INVALID";
}


string NTV2VANCModeToString (const NTV2VANCMode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		case NTV2_VANCMODE_OFF:		return inCompactDisplay ? "off"		: "NTV2_VANCMODE_OFF";
		case NTV2_VANCMODE_TALL:	return inCompactDisplay ? "tall"	: "NTV2_VANCMODE_TALL";
		case NTV2_VANCMODE_TALLER:	return inCompactDisplay ? "taller"	: "NTV2_VANCMODE_TALLER";
		default:					break;
	}
	return inCompactDisplay ? "" : "NTV2_VANCMODE_INVALID";
}


string NTV2EmbeddedAudioInputToString (const NTV2EmbeddedAudioInput inValue,  const bool inCompactDisplay)
{
	ostringstream	oss;
	if (NTV2_IS_VALID_EMBEDDED_AUDIO_INPUT(inValue))
		oss << (inCompactDisplay ? "SDI" : "NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_") << DEC(inValue+1);
	else
		oss << (inCompactDisplay ? "SDI?" : "NTV2_EMBEDDED_AUDIO_INPUT_INVALID");
	return oss.str();
}


string NTV2AudioSourceToString (const NTV2AudioSource inValue,  const bool inCompactDisplay)
{
	switch (inValue)
	{
		case NTV2_AUDIO_EMBEDDED:	return inCompactDisplay ? "SDI"		: "NTV2_AUDIO_EMBEDDED";
		case NTV2_AUDIO_AES:		return inCompactDisplay ? "AES"		: "NTV2_AUDIO_AES";
		case NTV2_AUDIO_ANALOG:		return inCompactDisplay ? "Analog"	: "NTV2_AUDIO_ANALOG";
		case NTV2_AUDIO_HDMI:		return inCompactDisplay ? "HDMI"	: "NTV2_AUDIO_HDMI";
		case NTV2_AUDIO_MIC:		return inCompactDisplay ? "Mic"		: "NTV2_AUDIO_MIC";
		default:					break;
	}
	return inCompactDisplay ? "" : "NTV2_AUDIO_SOURCE_INVALID";
}


string NTV2VideoFormatToString (const NTV2VideoFormat inFormat, const bool inUseFrameRate)
{
	switch (inFormat)
	{
		case NTV2_FORMAT_1080i_5000:	return inUseFrameRate ? "1080i25" 		: "1080i50";
		case NTV2_FORMAT_1080i_5994:	return inUseFrameRate ? "1080i29.97" 	: "1080i59.94";
		case NTV2_FORMAT_1080i_6000:	return inUseFrameRate ? "1080i30" 		: "1080i60";
		case NTV2_FORMAT_720p_5994:		return "720p59.94";
		case NTV2_FORMAT_720p_6000:		return "720p60";
		case NTV2_FORMAT_1080psf_2398:	return "1080sf23.98";
		case NTV2_FORMAT_1080psf_2400:	return "1080sf24";
		case NTV2_FORMAT_1080p_2997:	return "1080p29.97";
		case NTV2_FORMAT_1080p_3000:	return "1080p30";
		case NTV2_FORMAT_1080p_2500:	return "1080p25";
		case NTV2_FORMAT_1080p_2398:	return "1080p23.98";
		case NTV2_FORMAT_1080p_2400:	return "1080p24";
		case NTV2_FORMAT_1080p_2K_2398:	return "2Kp23.98";
		case NTV2_FORMAT_1080p_2K_2400:	return "2Kp24";
		case NTV2_FORMAT_1080psf_2K_2398:	return "2Ksf23.98";
		case NTV2_FORMAT_1080psf_2K_2400:	return "2Ksf24";
		case NTV2_FORMAT_720p_5000:		return "720p50";
		case NTV2_FORMAT_1080p_5000_B:	return "1080p50b";
		case NTV2_FORMAT_1080p_5994_B:	return "1080p59.94b";
		case NTV2_FORMAT_1080p_6000_B:	return "1080p60b";
		case NTV2_FORMAT_720p_2398:		return "720p23.98";
		case NTV2_FORMAT_720p_2500:		return "720p25";
		case NTV2_FORMAT_1080p_5000_A:	return "1080p50a";
		case NTV2_FORMAT_1080p_5994_A:	return "1080p59.94a";
		case NTV2_FORMAT_1080p_6000_A:	return "1080p60a";
		case NTV2_FORMAT_1080p_2K_2500:	return "2Kp25";
		case NTV2_FORMAT_1080psf_2K_2500:	return "2Ksf25";
		case NTV2_FORMAT_1080psf_2500_2:	return "1080sf25";
		case NTV2_FORMAT_1080psf_2997_2:	return "1080sf29.97";
		case NTV2_FORMAT_1080psf_3000_2:	return "1080sf30";
		case NTV2_FORMAT_525_5994:		return inUseFrameRate ? "525i29.97" : "525i59.94";
		case NTV2_FORMAT_625_5000:		return inUseFrameRate ? "625i25" 	: "625i50";
		case NTV2_FORMAT_525_2398:		return "525i23.98";
		case NTV2_FORMAT_525_2400:		return "525i24";
		case NTV2_FORMAT_525psf_2997:	return "525sf29.97";
		case NTV2_FORMAT_625psf_2500:	return "625sf25";
		case NTV2_FORMAT_2K_1498:		return "2Kx1556sf14.98";
		case NTV2_FORMAT_2K_1500:		return "2Kx1556sf15";
		case NTV2_FORMAT_2K_2398:		return "2Kx1556sf23.98";
		case NTV2_FORMAT_2K_2400:		return "2Kx1556sf24";
		case NTV2_FORMAT_2K_2500:		return "2Kx1556sf25";
		case NTV2_FORMAT_4x1920x1080psf_2398:	return "UHDsf23.98";
		case NTV2_FORMAT_4x1920x1080psf_2400:	return "UHDsf24";
		case NTV2_FORMAT_4x1920x1080psf_2500:	return "UHDsf25";
		case NTV2_FORMAT_4x1920x1080p_2398:		return "UHDp23.98";
		case NTV2_FORMAT_4x1920x1080p_2400:		return "UHDp24";
		case NTV2_FORMAT_4x1920x1080p_2500:		return "UHDp25";
		case NTV2_FORMAT_4x2048x1080psf_2398:	return "4Ksf23.98";
		case NTV2_FORMAT_4x2048x1080psf_2400:	return "4Ksf24";
		case NTV2_FORMAT_4x2048x1080psf_2500:	return "4Ksf25";
		case NTV2_FORMAT_4x2048x1080p_2398:		return "4Kp23.98";
		case NTV2_FORMAT_4x2048x1080p_2400:		return "4Kp24";
		case NTV2_FORMAT_4x2048x1080p_2500:		return "4Kp25";
		case NTV2_FORMAT_4x1920x1080p_2997:		return "UHDp29.97";
		case NTV2_FORMAT_4x1920x1080p_3000:		return "UHDp30";
		case NTV2_FORMAT_4x1920x1080psf_2997:	return "UHDsf29.97";
		case NTV2_FORMAT_4x1920x1080psf_3000:	return "UHDsf30";
		case NTV2_FORMAT_4x2048x1080p_2997:		return "4Kp29.97";
		case NTV2_FORMAT_4x2048x1080p_3000:		return "4Kp30";
		case NTV2_FORMAT_4x2048x1080psf_2997:	return "4Ksf29.97";
		case NTV2_FORMAT_4x2048x1080psf_3000:	return "4Ksf30";
		case NTV2_FORMAT_4x1920x1080p_5000:		return "UHDp50";
		case NTV2_FORMAT_4x1920x1080p_5994:		return "UHDp59.94";
		case NTV2_FORMAT_4x1920x1080p_6000:		return "UHDp60";
		case NTV2_FORMAT_4x2048x1080p_5000:		return "4Kp50";
		case NTV2_FORMAT_4x2048x1080p_5994:		return "4Kp59.94";
		case NTV2_FORMAT_4x2048x1080p_6000:		return "4Kp60";
		case NTV2_FORMAT_4x2048x1080p_4795:		return "4Kp47.95";
		case NTV2_FORMAT_4x2048x1080p_4800:		return "4Kp48";
		case NTV2_FORMAT_4x2048x1080p_11988:	return "4Kp119";
		case NTV2_FORMAT_4x2048x1080p_12000:	return "4Kp120";
		case NTV2_FORMAT_1080p_2K_6000_A:	return "2Kp60a";
		case NTV2_FORMAT_1080p_2K_5994_A:	return "2Kp59.94a";
		case NTV2_FORMAT_1080p_2K_2997:		return "2Kp29.97";
		case NTV2_FORMAT_1080p_2K_3000:		return "2Kp30";
		case NTV2_FORMAT_1080p_2K_5000_A:	return "2Kp50a";
		case NTV2_FORMAT_1080p_2K_4795_A:	return "2Kp47.95a";
		case NTV2_FORMAT_1080p_2K_4800_A:	return "2Kp48a";
		case NTV2_FORMAT_1080p_2K_4795_B:	return "2Kp47.95b";
		case NTV2_FORMAT_1080p_2K_4800_B:	return "2Kp48b";
		case NTV2_FORMAT_1080p_2K_5000_B:	return "2Kp50b";
		case NTV2_FORMAT_1080p_2K_5994_B:	return "2Kp59.94b";
		case NTV2_FORMAT_1080p_2K_6000_B:	return "2Kp60b";
		case NTV2_FORMAT_3840x2160psf_2398:	return "UHDsf23.98";
		case NTV2_FORMAT_3840x2160psf_2400:	return "UHDsf23.98";
		case NTV2_FORMAT_3840x2160psf_2500:	return "UHDsf23.98";
		case NTV2_FORMAT_3840x2160p_2398:	return "UHDp23.98";
		case NTV2_FORMAT_3840x2160p_2400:	return "UHDp24";
		case NTV2_FORMAT_3840x2160p_2500:	return "UHDp25";
		case NTV2_FORMAT_3840x2160p_2997:	return "UHDp29.97";
		case NTV2_FORMAT_3840x2160p_3000:	return "UHDp30";
		case NTV2_FORMAT_3840x2160psf_2997:	return "UHDsf29.97";
		case NTV2_FORMAT_3840x2160psf_3000:	return "UHDsf30";
		case NTV2_FORMAT_3840x2160p_5000:	return "UHDp50";
		case NTV2_FORMAT_3840x2160p_5994:	return "UHDp59.94";
		case NTV2_FORMAT_3840x2160p_6000:	return "UHDp60";
		case NTV2_FORMAT_4096x2160psf_2398:	return "4Ksf23.98";
		case NTV2_FORMAT_4096x2160psf_2400:	return "4Ksf24";
		case NTV2_FORMAT_4096x2160psf_2500:	return "4Ksf25";
		case NTV2_FORMAT_4096x2160p_2398:	return "4Kp23.98";
		case NTV2_FORMAT_4096x2160p_2400:	return "4Kp24";
		case NTV2_FORMAT_4096x2160p_2500:	return "4Kp25";
		case NTV2_FORMAT_4096x2160p_2997:	return "4Kp29.97";
		case NTV2_FORMAT_4096x2160p_3000:	return "4Kp30";
		case NTV2_FORMAT_4096x2160psf_2997:	return "4Ksf29.97";
		case NTV2_FORMAT_4096x2160psf_3000:	return "4Ksf30";
		case NTV2_FORMAT_4096x2160p_4795:	return "4Kp47.95";
		case NTV2_FORMAT_4096x2160p_4800:	return "4Kp48";
		case NTV2_FORMAT_4096x2160p_5000:	return "4Kp50";
		case NTV2_FORMAT_4096x2160p_5994:	return "4Kp59.94";
		case NTV2_FORMAT_4096x2160p_6000:	return "4Kp60";
		case NTV2_FORMAT_4096x2160p_11988:	return "4Kp119";
		case NTV2_FORMAT_4096x2160p_12000:	return "4Kp120";
		case NTV2_FORMAT_4x1920x1080p_5000_B:		return "UHDp50b";
		case NTV2_FORMAT_4x1920x1080p_5994_B:		return "UHDp59.94b";
		case NTV2_FORMAT_4x1920x1080p_6000_B:		return "UHDp60b";
		case NTV2_FORMAT_4x2048x1080p_5000_B:		return "4Kp50b";
		case NTV2_FORMAT_4x2048x1080p_5994_B:		return "4Kp59.94b";
		case NTV2_FORMAT_4x2048x1080p_6000_B:		return "4Kp60b";
		case NTV2_FORMAT_4x2048x1080p_4795_B:		return "4Kp47.95b";
		case NTV2_FORMAT_4x2048x1080p_4800_B:		return "4Kp48b";
		case NTV2_FORMAT_3840x2160p_5000_B:	return "UHDp50b";
		case NTV2_FORMAT_3840x2160p_5994_B:	return "UHDp59.94b";
		case NTV2_FORMAT_3840x2160p_6000_B:	return "UHDp60b";
		case NTV2_FORMAT_4096x2160p_4795_B:	return "4Kp47.95b";
		case NTV2_FORMAT_4096x2160p_4800_B:	return "4Kp48b";
		case NTV2_FORMAT_4096x2160p_5000_B:	return "4Kp50b";
		case NTV2_FORMAT_4096x2160p_5994_B:	return "4Kp59.94b";
		case NTV2_FORMAT_4096x2160p_6000_B:	return "4Kp60b";
		default: return "Unknown";
	}
}	//	NTV2VideoFormatToString


string NTV2StandardToString (const NTV2Standard inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_STANDARD_1080:		return inForRetailDisplay ? "1080i"		: "NTV2_STANDARD_1080";
		case NTV2_STANDARD_720:			return inForRetailDisplay ? "720p"		: "NTV2_STANDARD_720";
		case NTV2_STANDARD_525:			return inForRetailDisplay ? "525i"		: "NTV2_STANDARD_525";
		case NTV2_STANDARD_625:			return inForRetailDisplay ? "625i"		: "NTV2_STANDARD_625";
		case NTV2_STANDARD_1080p:		return inForRetailDisplay ? "1080p"		: "NTV2_STANDARD_1080p";
		case NTV2_STANDARD_2K:			return inForRetailDisplay ? "2K"		: "NTV2_STANDARD_2K";
		case NTV2_STANDARD_2Kx1080p:	return inForRetailDisplay ? "2K1080p"	: "NTV2_STANDARD_2Kx1080p";
		case NTV2_STANDARD_2Kx1080i:	return inForRetailDisplay ? "2K1080i"	: "NTV2_STANDARD_2Kx1080i";
		case NTV2_STANDARD_3840x2160p:	return inForRetailDisplay ? "UHD"		: "NTV2_STANDARD_3840x2160p";
		case NTV2_STANDARD_4096x2160p:	return inForRetailDisplay ? "4K"		: "NTV2_STANDARD_4096x2160p";
		case NTV2_STANDARD_3840HFR:		return inForRetailDisplay ? "UHD HFR"	: "NTV2_STANDARD_3840HFR";
		case NTV2_STANDARD_4096HFR:		return inForRetailDisplay ? "4K HFR"	: "NTV2_STANDARD_4096HFR";
		case NTV2_STANDARD_INVALID:		return inForRetailDisplay ? ""			: "NTV2_STANDARD_INVALID";
	}
	return string ();
}


string NTV2FrameBufferFormatToString (const NTV2FrameBufferFormat inValue,	const bool inForRetailDisplay)
{
	if (inForRetailDisplay)
	{
		switch (inValue)
		{
			case NTV2_FBF_10BIT_YCBCR:				return "YUV-10";
			case NTV2_FBF_8BIT_YCBCR:				return "YUV-8";
			case NTV2_FBF_ARGB:						return "RGBA-8";
			case NTV2_FBF_RGBA:						return "ARGB-8";
			case NTV2_FBF_10BIT_RGB:				return "RGB-10";
			case NTV2_FBF_8BIT_YCBCR_YUY2:			return "YUY2-8";
			case NTV2_FBF_ABGR:						return "ABGR-8";
			case NTV2_FBF_10BIT_DPX:				return "RGB-10";
			case NTV2_FBF_10BIT_YCBCR_DPX:			return "YUV-DPX10";
			case NTV2_FBF_8BIT_DVCPRO:				return "DVCProHD";
			case NTV2_FBF_8BIT_YCBCR_420PL3:		return "QRez";
			case NTV2_FBF_8BIT_HDV:					return "HDV";
			case NTV2_FBF_24BIT_RGB:				return "RGB-8";
			case NTV2_FBF_24BIT_BGR:				return "BGR-8";
			case NTV2_FBF_10BIT_YCBCRA:				return "YUVA-10";
			case NTV2_FBF_10BIT_DPX_LE:             return "RGB-L10";
			case NTV2_FBF_48BIT_RGB:				return "RGB-12";
			case NTV2_FBF_PRORES:					return "ProRes-422";
			case NTV2_FBF_PRORES_DVCPRO:			return "ProRes-DVC";
			case NTV2_FBF_PRORES_HDV:				return "ProRes-HDV";
			case NTV2_FBF_10BIT_RGB_PACKED:			return "RGB-P10";
			case NTV2_FBF_10BIT_ARGB:				return "ARGB-10";
			case NTV2_FBF_16BIT_ARGB:				return "ARGB-16";
			case NTV2_FBF_8BIT_YCBCR_422PL3:		return "YUV-P8";
			case NTV2_FBF_10BIT_RAW_RGB:			return "RAW-RGB10";
			case NTV2_FBF_10BIT_RAW_YCBCR:			return "RAW-YUV10";
			case NTV2_FBF_10BIT_YCBCR_420PL3_LE:	return "YUV-P420-L10";
			case NTV2_FBF_10BIT_YCBCR_422PL3_LE:	return "YUV-P-L10";
			case NTV2_FBF_10BIT_YCBCR_420PL2:		return "YUV-P420-10";
			case NTV2_FBF_10BIT_YCBCR_422PL2:		return "YUV-P-10";
			case NTV2_FBF_8BIT_YCBCR_420PL2:		return "YUV-P420-8";
			case NTV2_FBF_8BIT_YCBCR_422PL2:		return "YUV-P-8";
			case NTV2_FBF_INVALID:					return "Unknown";
		}
	}
	else
	{
		switch (inValue)
		{
			case NTV2_FBF_10BIT_YCBCR:				return "NTV2_FBF_10BIT_YCBCR";
			case NTV2_FBF_8BIT_YCBCR:				return "NTV2_FBF_8BIT_YCBCR";
			case NTV2_FBF_ARGB:						return "NTV2_FBF_ARGB";
			case NTV2_FBF_RGBA:						return "NTV2_FBF_RGBA";
			case NTV2_FBF_10BIT_RGB:				return "NTV2_FBF_10BIT_RGB";
			case NTV2_FBF_8BIT_YCBCR_YUY2:			return "NTV2_FBF_8BIT_YCBCR_YUY2";
			case NTV2_FBF_ABGR:						return "NTV2_FBF_ABGR";
			case NTV2_FBF_10BIT_DPX:				return "NTV2_FBF_10BIT_DPX";
			case NTV2_FBF_10BIT_YCBCR_DPX:			return "NTV2_FBF_10BIT_YCBCR_DPX";
			case NTV2_FBF_8BIT_DVCPRO:				return "NTV2_FBF_8BIT_DVCPRO";
			case NTV2_FBF_8BIT_YCBCR_420PL3:		return "NTV2_FBF_8BIT_YCBCR_420PL3";
			case NTV2_FBF_8BIT_HDV:					return "NTV2_FBF_8BIT_HDV";
			case NTV2_FBF_24BIT_RGB:				return "NTV2_FBF_24BIT_RGB";
			case NTV2_FBF_24BIT_BGR:				return "NTV2_FBF_24BIT_BGR";
			case NTV2_FBF_10BIT_YCBCRA:				return "NTV2_FBF_10BIT_YCBCRA";
			case NTV2_FBF_10BIT_DPX_LE:             return "NTV2_FBF_10BIT_DPX_LE";
			case NTV2_FBF_48BIT_RGB:				return "NTV2_FBF_48BIT_RGB";
			case NTV2_FBF_PRORES:					return "NTV2_FBF_PRORES";
			case NTV2_FBF_PRORES_DVCPRO:			return "NTV2_FBF_PRORES_DVCPRO";
			case NTV2_FBF_PRORES_HDV:				return "NTV2_FBF_PRORES_HDV";
			case NTV2_FBF_10BIT_RGB_PACKED:			return "NTV2_FBF_10BIT_RGB_PACKED";
			case NTV2_FBF_10BIT_ARGB:				return "NTV2_FBF_10BIT_ARGB";
			case NTV2_FBF_16BIT_ARGB:				return "NTV2_FBF_16BIT_ARGB";
			case NTV2_FBF_8BIT_YCBCR_422PL3:		return "NTV2_FBF_8BIT_YCBCR_422PL3";
			case NTV2_FBF_10BIT_RAW_RGB:			return "NTV2_FBF_10BIT_RAW_RGB";
			case NTV2_FBF_10BIT_RAW_YCBCR:			return "NTV2_FBF_10BIT_RAW_YCBCR";
			case NTV2_FBF_10BIT_YCBCR_420PL3_LE:	return "NTV2_FBF_10BIT_YCBCR_420PL3_LE";
			case NTV2_FBF_10BIT_YCBCR_422PL3_LE:	return "NTV2_FBF_10BIT_YCBCR_422PL3_LE";
			case NTV2_FBF_10BIT_YCBCR_420PL2:		return "NTV2_FBF_10BIT_YCBCR_420PL2";
			case NTV2_FBF_10BIT_YCBCR_422PL2:		return "NTV2_FBF_10BIT_YCBCR_422PL2";
			case NTV2_FBF_8BIT_YCBCR_420PL2:		return "NTV2_FBF_8BIT_YCBCR_420PL2";
			case NTV2_FBF_8BIT_YCBCR_422PL2:		return "NTV2_FBF_8BIT_YCBCR_422PL2";
			case NTV2_FBF_INVALID:					return "NTV2_FBF_INVALID";
		}
	}
	return string ();
}


string NTV2M31VideoPresetToString (const M31VideoPreset inValue, const bool inForRetailDisplay)
{
	if (inForRetailDisplay)
		return m31Presets [inValue];	//	frameBufferFormatString (inValue);
	
	switch (inValue)
	{
        case M31_FILE_720X480_420_8_5994i:          return "M31_FILE_720X480_420_8_5994i";
        case M31_FILE_720X480_420_8_5994p:          return "M31_FILE_720X480_420_8_5994p";
        case M31_FILE_720X480_420_8_60i:            return "M31_FILE_720X480_420_8_60i";
        case M31_FILE_720X480_420_8_60p:            return "M31_FILE_720X480_420_8_60p";
        case M31_FILE_720X480_422_10_5994i:         return "M31_FILE_720X480_422_10_5994i";
        case M31_FILE_720X480_422_10_5994p:         return "M31_FILE_720X480_422_10_5994p";
        case M31_FILE_720X480_422_10_60i:           return "M31_FILE_720X480_422_10_60i";
        case M31_FILE_720X480_422_10_60p:           return "M31_FILE_720X480_422_10_60p";

        case M31_FILE_720X576_420_8_50i:            return "M31_FILE_720X576_420_8_50i";
        case M31_FILE_720X576_420_8_50p:            return "M31_FILE_720X576_420_8_50p";
        case M31_FILE_720X576_422_10_50i:           return "M31_FILE_720X576_422_10_50i";
        case M31_FILE_720X576_422_10_50p:           return "M31_FILE_720X576_422_10_50p";

        case M31_FILE_1280X720_420_8_2398p:         return "M31_FILE_1280X720_420_8_2398p";
        case M31_FILE_1280X720_420_8_24p:           return "M31_FILE_1280X720_420_8_24p";
        case M31_FILE_1280X720_420_8_25p:           return "M31_FILE_1280X720_420_8_25p";
        case M31_FILE_1280X720_420_8_2997p:         return "M31_FILE_1280X720_420_8_2997p";
        case M31_FILE_1280X720_420_8_30p:           return "M31_FILE_1280X720_420_8_30p";
        case M31_FILE_1280X720_420_8_50p:           return "M31_FILE_1280X720_420_8_50p";
        case M31_FILE_1280X720_420_8_5994p:         return "M31_FILE_1280X720_420_8_5994p";
        case M31_FILE_1280X720_420_8_60p:           return "M31_FILE_1280X720_420_8_60p";
            
        case M31_FILE_1280X720_422_10_2398p:        return "M31_FILE_1280X720_422_10_2398p";
        case M31_FILE_1280X720_422_10_24p:          return "M31_FILE_1280X720_422_10_24p";
        case M31_FILE_1280X720_422_10_25p:          return "M31_FILE_1280X720_422_10_25p";
        case M31_FILE_1280X720_422_10_2997p:        return "M31_FILE_1280X720_422_10_2997p";
        case M31_FILE_1280X720_422_10_30p:          return "M31_FILE_1280X720_422_10_30p";
        case M31_FILE_1280X720_422_10_50p:          return "M31_FILE_1280X720_422_10_50p";
        case M31_FILE_1280X720_422_10_5994p:        return "M31_FILE_1280X720_422_10_5994p";
        case M31_FILE_1280X720_422_10_60p:          return "M31_FILE_1280X720_422_10_60p";

        case M31_FILE_1920X1080_420_8_2398p:        return "M31_FILE_1920X1080_420_8_2398p";
        case M31_FILE_1920X1080_420_8_24p:          return "M31_FILE_1920X1080_420_8_24p";
        case M31_FILE_1920X1080_420_8_25p:          return "M31_FILE_1920X1080_420_8_25p";
        case M31_FILE_1920X1080_420_8_2997p:        return "M31_FILE_1920X1080_420_8_2997p";
        case M31_FILE_1920X1080_420_8_30p:          return "M31_FILE_1920X1080_420_8_30p";
        case M31_FILE_1920X1080_420_8_50i:          return "M31_FILE_1920X1080_420_8_50i";
        case M31_FILE_1920X1080_420_8_50p:          return "M31_FILE_1920X1080_420_8_50p";
        case M31_FILE_1920X1080_420_8_5994i:        return "M31_FILE_1920X1080_420_8_5994i";
        case M31_FILE_1920X1080_420_8_5994p:        return "M31_FILE_1920X1080_420_8_5994p";
        case M31_FILE_1920X1080_420_8_60i:          return "M31_FILE_1920X1080_420_8_60i";
        case M31_FILE_1920X1080_420_8_60p:          return "M31_FILE_1920X1080_420_8_60p";
            
        case M31_FILE_1920X1080_422_10_2398p:       return "M31_FILE_1920X1080_422_10_2398p";
        case M31_FILE_1920X1080_422_10_24p:         return "M31_FILE_1920X1080_422_10_24p";
        case M31_FILE_1920X1080_422_10_25p:         return "M31_FILE_1920X1080_422_10_25p";
        case M31_FILE_1920X1080_422_10_2997p:       return "M31_FILE_1920X1080_422_10_2997p";
        case M31_FILE_1920X1080_422_10_30p:         return "M31_FILE_1920X1080_422_10_30p";
        case M31_FILE_1920X1080_422_10_50i:         return "M31_FILE_1920X1080_422_10_50i";
        case M31_FILE_1920X1080_422_10_50p:         return "M31_FILE_1920X1080_422_10_50p";
        case M31_FILE_1920X1080_422_10_5994i:       return "M31_FILE_1920X1080_422_10_5994i";
        case M31_FILE_1920X1080_422_10_5994p:       return "M31_FILE_1920X1080_422_10_5994p";
        case M31_FILE_1920X1080_422_10_60i:         return "M31_FILE_1920X1080_422_10_60i";
        case M31_FILE_1920X1080_422_10_60p:         return "M31_FILE_1920X1080_422_10_60p";

        case M31_FILE_2048X1080_420_8_2398p:        return "M31_FILE_2048X1080_420_8_2398p";
        case M31_FILE_2048X1080_420_8_24p:          return "M31_FILE_2048X1080_420_8_24p";
        case M31_FILE_2048X1080_420_8_25p:          return "M31_FILE_2048X1080_420_8_25p";
        case M31_FILE_2048X1080_420_8_2997p:        return "M31_FILE_2048X1080_420_8_2997p";
        case M31_FILE_2048X1080_420_8_30p:          return "M31_FILE_2048X1080_420_8_30p";
        case M31_FILE_2048X1080_420_8_50p:          return "M31_FILE_2048X1080_420_8_50p";
        case M31_FILE_2048X1080_420_8_5994p:        return "M31_FILE_2048X1080_420_8_5994p";
        case M31_FILE_2048X1080_420_8_60p:          return "M31_FILE_2048X1080_420_8_60p";
            
        case M31_FILE_2048X1080_422_10_2398p:       return "M31_FILE_2048X1080_422_10_2398p";
        case M31_FILE_2048X1080_422_10_24p:         return "M31_FILE_2048X1080_422_10_24p";
        case M31_FILE_2048X1080_422_10_25p:         return "M31_FILE_2048X1080_422_10_25p";
        case M31_FILE_2048X1080_422_10_2997p:       return "M31_FILE_2048X1080_422_10_2997p";
        case M31_FILE_2048X1080_422_10_30p:         return "M31_FILE_2048X1080_422_10_30p";
        case M31_FILE_2048X1080_422_10_50p:         return "M31_FILE_2048X1080_422_10_50p";
        case M31_FILE_2048X1080_422_10_5994p:       return "M31_FILE_2048X1080_422_10_5994p";
        case M31_FILE_2048X1080_422_10_60p:         return "M31_FILE_2048X1080_422_10_60p";

        case M31_FILE_3840X2160_420_8_2398p:        return "M31_FILE_3840X2160_420_8_2398p";
        case M31_FILE_3840X2160_420_8_24p:          return "M31_FILE_3840X2160_420_8_24p";
        case M31_FILE_3840X2160_420_8_25p:          return "M31_FILE_3840X2160_420_8_25p";
        case M31_FILE_3840X2160_420_8_2997p:        return "M31_FILE_3840X2160_420_8_2997p";
        case M31_FILE_3840X2160_420_8_30p:          return "M31_FILE_3840X2160_420_8_30p";
        case M31_FILE_3840X2160_420_8_50p:          return "M31_FILE_3840X2160_420_8_50p";
        case M31_FILE_3840X2160_420_8_5994p:        return "M31_FILE_3840X2160_420_8_5994p";
        case M31_FILE_3840X2160_420_8_60p:          return "M31_FILE_3840X2160_420_8_60p";
        case M31_FILE_3840X2160_420_10_50p:         return "M31_FILE_3840X2160_420_10_50p";
        case M31_FILE_3840X2160_420_10_5994p:       return "M31_FILE_3840X2160_420_10_5994p";
        case M31_FILE_3840X2160_420_10_60p:         return "M31_FILE_3840X2160_420_10_60p";
            
        case M31_FILE_3840X2160_422_8_2398p:        return "M31_FILE_3840X2160_422_8_2398p";
        case M31_FILE_3840X2160_422_8_24p:          return "M31_FILE_3840X2160_422_8_24p";
        case M31_FILE_3840X2160_422_8_25p:          return "M31_FILE_3840X2160_422_8_25p";
        case M31_FILE_3840X2160_422_8_2997p:        return "M31_FILE_3840X2160_422_8_2997p";
        case M31_FILE_3840X2160_422_8_30p:          return "M31_FILE_3840X2160_422_8_30p";
        case M31_FILE_3840X2160_422_8_50p:          return "M31_FILE_3840X2160_422_8_50p";
        case M31_FILE_3840X2160_422_8_5994p:        return "M31_FILE_3840X2160_422_8_5994p";
        case M31_FILE_3840X2160_422_8_60p:          return "M31_FILE_3840X2160_422_8_60p";
            
        case M31_FILE_3840X2160_422_10_2398p:       return "M31_FILE_3840X2160_422_10_2398p";
        case M31_FILE_3840X2160_422_10_24p:         return "M31_FILE_3840X2160_422_10_24p";
        case M31_FILE_3840X2160_422_10_25p:         return "M31_FILE_3840X2160_422_10_25p";
        case M31_FILE_3840X2160_422_10_2997p:       return "M31_FILE_3840X2160_422_10_2997p";
        case M31_FILE_3840X2160_422_10_30p:         return "M31_FILE_3840X2160_422_10_30p";
        case M31_FILE_3840X2160_422_10_50p:         return "M31_FILE_3840X2160_422_10_50p";
        case M31_FILE_3840X2160_422_10_5994p:       return "M31_FILE_3840X2160_422_10_5994p";
        case M31_FILE_3840X2160_422_10_60p:         return "M31_FILE_3840X2160_422_10_60p";
        
        case M31_FILE_4096X2160_420_10_5994p:       return "M31_FILE_4096X2160_420_10_5994p";
        case M31_FILE_4096X2160_420_10_60p:         return "M31_FILE_4096X2160_420_10_60p";
        case M31_FILE_4096X2160_422_10_50p:         return "M31_FILE_4096X2160_422_10_50p";
        case M31_FILE_4096X2160_422_10_5994p_IF:    return "M31_FILE_4096X2160_422_10_5994p_IO";
        case M31_FILE_4096X2160_422_10_60p_IF:      return "M31_FILE_4096X2160_422_10_60p_IO";
            
        case M31_VIF_720X480_420_8_5994i:           return "M31_VIF_720X480_420_8_5994i";
        case M31_VIF_720X480_420_8_5994p:           return "M31_VIF_720X480_420_8_5994p";
        case M31_VIF_720X480_420_8_60i:             return "M31_VIF_720X480_420_8_60i";
        case M31_VIF_720X480_420_8_60p:             return "M31_VIF_720X480_420_8_60p";
        case M31_VIF_720X480_422_10_5994i:          return "M31_VIF_720X480_422_10_5994i";
        case M31_VIF_720X480_422_10_5994p:          return "M31_VIF_720X480_422_10_5994p";
        case M31_VIF_720X480_422_10_60i:            return "M31_VIF_720X480_422_10_60i";
        case M31_VIF_720X480_422_10_60p:            return "M31_VIF_720X480_422_10_60p";

        case M31_VIF_720X576_420_8_50i:             return "M31_VIF_720X576_420_8_50i";
        case M31_VIF_720X576_420_8_50p:             return "M31_VIF_720X576_420_8_50p";
        case M31_VIF_720X576_422_10_50i:            return "M31_VIF_720X576_422_10_50i";
        case M31_VIF_720X576_422_10_50p:            return "M31_VIF_720X576_422_10_50p";

        case M31_VIF_1280X720_420_8_50p:            return "M31_VIF_1280X720_420_8_50p";
        case M31_VIF_1280X720_420_8_5994p:          return "M31_VIF_1280X720_420_8_5994p";
        case M31_VIF_1280X720_420_8_60p:            return "M31_VIF_1280X720_420_8_60p";
        case M31_VIF_1280X720_422_10_50p:           return "M31_VIF_1280X720_422_10_50p";
        case M31_VIF_1280X720_422_10_5994p:         return "M31_VIF_1280X720_422_10_5994p";
        case M31_VIF_1280X720_422_10_60p:           return "M31_VIF_1280X720_422_10_60p";

        case M31_VIF_1920X1080_420_8_50i:           return "M31_VIF_1920X1080_420_8_50i";
        case M31_VIF_1920X1080_420_8_50p:           return "M31_VIF_1920X1080_420_8_50p";
        case M31_VIF_1920X1080_420_8_5994i:         return "M31_VIF_1920X1080_420_8_5994i";
        case M31_VIF_1920X1080_420_8_5994p:         return "M31_VIF_1920X1080_420_8_5994p";
        case M31_VIF_1920X1080_420_8_60i:           return "M31_VIF_1920X1080_420_8_60i";
        case M31_VIF_1920X1080_420_8_60p:           return "M31_VIF_1920X1080_420_8_60p";
        case M31_VIF_1920X1080_420_10_50i:          return "M31_VIF_1920X1080_420_10_50i";
        case M31_VIF_1920X1080_420_10_50p:          return "M31_VIF_1920X1080_420_10_50p";
        case M31_VIF_1920X1080_420_10_5994i:        return "M31_VIF_1920X1080_420_10_5994i";
        case M31_VIF_1920X1080_420_10_5994p:        return "M31_VIF_1920X1080_420_10_5994p";
        case M31_VIF_1920X1080_420_10_60i:          return "M31_VIF_1920X1080_420_10_60i";
        case M31_VIF_1920X1080_420_10_60p:          return "M31_VIF_1920X1080_420_10_60p";
        case M31_VIF_1920X1080_422_10_5994i:        return "M31_VIF_1920X1080_422_10_5994i";
        case M31_VIF_1920X1080_422_10_5994p:        return "M31_VIF_1920X1080_422_10_5994p";
        case M31_VIF_1920X1080_422_10_60i:          return "M31_VIF_1920X1080_422_10_60i";
        case M31_VIF_1920X1080_422_10_60p:          return "M31_VIF_1920X1080_422_10_60p";

        case M31_VIF_3840X2160_420_8_30p:           return "M31_VIF_3840X2160_420_8_30p";
        case M31_VIF_3840X2160_420_8_50p:           return "M31_VIF_3840X2160_420_8_50p";
        case M31_VIF_3840X2160_420_8_5994p:         return "M31_VIF_3840X2160_420_8_5994p";
        case M31_VIF_3840X2160_420_8_60p:           return "M31_VIF_3840X2160_420_8_60p";
        case M31_VIF_3840X2160_420_10_50p:          return "M31_VIF_3840X2160_420_10_50p";
        case M31_VIF_3840X2160_420_10_5994p:        return "M31_VIF_3840X2160_420_10_5994p";
        case M31_VIF_3840X2160_420_10_60p:          return "M31_VIF_3840X2160_420_10_60p";
        case M31_VIF_3840X2160_422_10_30p:          return "M31_VIF_3840X2160_422_10_30p";
        case M31_VIF_3840X2160_422_10_50p:          return "M31_VIF_3840X2160_422_10_50p";
        case M31_VIF_3840X2160_422_10_5994p:        return "M31_VIF_3840X2160_422_10_5994p";
        case M31_VIF_3840X2160_422_10_60p:          return "M31_VIF_3840X2160_422_10_60p";

		case M31_NUMVIDEOPRESETS:	break;
	}
	return string ();
}


string NTV2FrameGeometryToString (const NTV2FrameGeometry inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_FG_1920x1080:				return inForRetailDisplay ? "1920x1080"		: "NTV2_FG_1920x1080";
		case NTV2_FG_1280x720:				return inForRetailDisplay ?	"1280x720"		: "NTV2_FG_1280x720";
		case NTV2_FG_720x486:				return inForRetailDisplay ?	"720x486"		: "NTV2_FG_720x486";
		case NTV2_FG_720x576:				return inForRetailDisplay ?	"720x576"		: "NTV2_FG_720x576";
		case NTV2_FG_1920x1114:				return inForRetailDisplay ?	"1920x1114"		: "NTV2_FG_1920x1114";
		case NTV2_FG_2048x1114:				return inForRetailDisplay ?	"2048x1114"		: "NTV2_FG_2048x1114";
		case NTV2_FG_720x508:				return inForRetailDisplay ?	"720x508"		: "NTV2_FG_720x508";
		case NTV2_FG_720x598:				return inForRetailDisplay ?	"720x598"		: "NTV2_FG_720x598";
		case NTV2_FG_1920x1112:				return inForRetailDisplay ?	"1920x1112"		: "NTV2_FG_1920x1112";
		case NTV2_FG_1280x740:				return inForRetailDisplay ?	"1280x740"		: "NTV2_FG_1280x740";
		case NTV2_FG_2048x1080:				return inForRetailDisplay ?	"2048x1080"		: "NTV2_FG_2048x1080";
		case NTV2_FG_2048x1556:				return inForRetailDisplay ?	"2048x1556"		: "NTV2_FG_2048x1556";
		case NTV2_FG_2048x1588:				return inForRetailDisplay ?	"2048x1588"		: "NTV2_FG_2048x1588";
		case NTV2_FG_2048x1112:				return inForRetailDisplay ?	"2048x1112"		: "NTV2_FG_2048x1112";
		case NTV2_FG_720x514:				return inForRetailDisplay ?	"720x514"		: "NTV2_FG_720x514";
		case NTV2_FG_720x612:				return inForRetailDisplay ?	"720x612"		: "NTV2_FG_720x612";
		case NTV2_FG_4x1920x1080:			return inForRetailDisplay ?	"4x1920x1080"	: "NTV2_FG_4x1920x1080";
		case NTV2_FG_4x2048x1080:			return inForRetailDisplay ?	"4x2048x1080"	: "NTV2_FG_4x2048x1080";
		case NTV2_FG_NUMFRAMEGEOMETRIES:	break;
	}
	return string ();
}


string NTV2FrameRateToString (const NTV2FrameRate inValue,	const bool inForRetailDisplay)
{
	if (inForRetailDisplay)
		return NTV2FrameRateStrings [inValue];
	switch (inValue)
	{
		case NTV2_FRAMERATE_UNKNOWN:	return "NTV2_FRAMERATE_UNKNOWN";
		case NTV2_FRAMERATE_6000:		return "NTV2_FRAMERATE_6000";
		case NTV2_FRAMERATE_5994:		return "NTV2_FRAMERATE_5994";
		case NTV2_FRAMERATE_3000:		return "NTV2_FRAMERATE_3000";
		case NTV2_FRAMERATE_2997:		return "NTV2_FRAMERATE_2997";
		case NTV2_FRAMERATE_2500:		return "NTV2_FRAMERATE_2500";
		case NTV2_FRAMERATE_2400:		return "NTV2_FRAMERATE_2400";
		case NTV2_FRAMERATE_2398:		return "NTV2_FRAMERATE_2398";
		case NTV2_FRAMERATE_5000:		return "NTV2_FRAMERATE_5000";
		case NTV2_FRAMERATE_4800:		return "NTV2_FRAMERATE_4800";
		case NTV2_FRAMERATE_4795:		return "NTV2_FRAMERATE_4795";
		case NTV2_FRAMERATE_12000:		return "NTV2_FRAMERATE_12000";
		case NTV2_FRAMERATE_11988:		return "NTV2_FRAMERATE_11988";
		case NTV2_FRAMERATE_1500:		return "NTV2_FRAMERATE_1500";
		case NTV2_FRAMERATE_1498:		return "NTV2_FRAMERATE_1498";
		case NTV2_FRAMERATE_1900:		return "NTV2_FRAMERATE_1900";
		case NTV2_FRAMERATE_1898:		return "NTV2_FRAMERATE_1898";
		case NTV2_FRAMERATE_1800:		return "NTV2_FRAMERATE_1800";
		case NTV2_FRAMERATE_1798:		return "NTV2_FRAMERATE_1798";
		case NTV2_NUM_FRAMERATES:		break;
	}
	return string ();
}


string NTV2InputSourceToString (const NTV2InputSource inValue,	const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_INPUTSOURCE_ANALOG1:			return inForRetailDisplay ? "Analog1"	: "NTV2_INPUTSOURCE_ANALOG1";
		case NTV2_INPUTSOURCE_HDMI1:			return inForRetailDisplay ? "HDMI1"		: "NTV2_INPUTSOURCE_HDMI1";
		case NTV2_INPUTSOURCE_HDMI2:			return inForRetailDisplay ? "HDMI2"		: "NTV2_INPUTSOURCE_HDMI2";
		case NTV2_INPUTSOURCE_HDMI3:			return inForRetailDisplay ? "HDMI3"		: "NTV2_INPUTSOURCE_HDMI3";
		case NTV2_INPUTSOURCE_HDMI4:			return inForRetailDisplay ? "HDMI4"		: "NTV2_INPUTSOURCE_HDMI4";
		case NTV2_INPUTSOURCE_SDI1:				return inForRetailDisplay ? "SDI1"		: "NTV2_INPUTSOURCE_SDI1";
		case NTV2_INPUTSOURCE_SDI2:				return inForRetailDisplay ? "SDI2"		: "NTV2_INPUTSOURCE_SDI2";
		case NTV2_INPUTSOURCE_SDI3:				return inForRetailDisplay ? "SDI3"		: "NTV2_INPUTSOURCE_SDI3";
		case NTV2_INPUTSOURCE_SDI4:				return inForRetailDisplay ? "SDI4"		: "NTV2_INPUTSOURCE_SDI4";
		case NTV2_INPUTSOURCE_SDI5:				return inForRetailDisplay ? "SDI5"		: "NTV2_INPUTSOURCE_SDI5";
		case NTV2_INPUTSOURCE_SDI6:				return inForRetailDisplay ? "SDI6"		: "NTV2_INPUTSOURCE_SDI6";
		case NTV2_INPUTSOURCE_SDI7:				return inForRetailDisplay ? "SDI7"		: "NTV2_INPUTSOURCE_SDI7";
		case NTV2_INPUTSOURCE_SDI8:				return inForRetailDisplay ? "SDI8"		: "NTV2_INPUTSOURCE_SDI8";
		case NTV2_INPUTSOURCE_INVALID:			return inForRetailDisplay ? ""			: "NTV2_INPUTSOURCE_INVALID";
	}
	return string ();
}


string NTV2OutputDestinationToString (const NTV2OutputDestination inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_OUTPUTDESTINATION_ANALOG:			return inForRetailDisplay ? "Analog"	: "NTV2_OUTPUTDESTINATION_ANALOG";
		case NTV2_OUTPUTDESTINATION_HDMI:			return inForRetailDisplay ? "HDMI"		: "NTV2_OUTPUTDESTINATION_HDMI";
		case NTV2_OUTPUTDESTINATION_SDI1:			return inForRetailDisplay ? "SDI 1"		: "NTV2_OUTPUTDESTINATION_SDI1";
		case NTV2_OUTPUTDESTINATION_SDI2:			return inForRetailDisplay ? "SDI 2"		: "NTV2_OUTPUTDESTINATION_SDI2";
		case NTV2_OUTPUTDESTINATION_SDI3:			return inForRetailDisplay ? "SDI3"		: "NTV2_OUTPUTDESTINATION_SDI3";
		case NTV2_OUTPUTDESTINATION_SDI4:			return inForRetailDisplay ? "SDI4"		: "NTV2_OUTPUTDESTINATION_SDI4";
		case NTV2_OUTPUTDESTINATION_SDI5:			return inForRetailDisplay ? "SDI5"		: "NTV2_OUTPUTDESTINATION_SDI5";
		case NTV2_OUTPUTDESTINATION_SDI6:			return inForRetailDisplay ? "SDI6"		: "NTV2_OUTPUTDESTINATION_SDI6";
		case NTV2_OUTPUTDESTINATION_SDI7:			return inForRetailDisplay ? "SDI7"		: "NTV2_OUTPUTDESTINATION_SDI7";
		case NTV2_OUTPUTDESTINATION_SDI8:			return inForRetailDisplay ? "SDI8"		: "NTV2_OUTPUTDESTINATION_SDI8";
		#if !defined (NTV2_DEPRECATE)
			case NTV2_OUTPUTDESTINATION_HDMI_14:	return inForRetailDisplay ? "HDMI 1.4"	: "NTV2_OUTPUTDESTINATION_HDMI_14";
			case NTV2_OUTPUTDESTINATION_DUALLINK1:	return inForRetailDisplay ? "SDI1 DL"	: "NTV2_OUTPUTDESTINATION_DUALLINK1";
			case NTV2_OUTPUTDESTINATION_DUALLINK2:	return inForRetailDisplay ? "SDI2 DL"	: "NTV2_OUTPUTDESTINATION_DUALLINK2";
			case NTV2_OUTPUTDESTINATION_DUALLINK3:	return inForRetailDisplay ? "SDI3 DL"	: "NTV2_OUTPUTDESTINATION_DUALLINK3";
			case NTV2_OUTPUTDESTINATION_DUALLINK4:	return inForRetailDisplay ? "SDI4 DL"	: "NTV2_OUTPUTDESTINATION_DUALLINK4";
			case NTV2_OUTPUTDESTINATION_DUALLINK5:	return inForRetailDisplay ? "SDI5 DL"	: "NTV2_OUTPUTDESTINATION_DUALLINK5";
			case NTV2_OUTPUTDESTINATION_DUALLINK6:	return inForRetailDisplay ? "SDI6 DL"	: "NTV2_OUTPUTDESTINATION_DUALLINK6";
			case NTV2_OUTPUTDESTINATION_DUALLINK7:	return inForRetailDisplay ? "SDI7 DL"	: "NTV2_OUTPUTDESTINATION_DUALLINK7";
			case NTV2_OUTPUTDESTINATION_DUALLINK8:	return inForRetailDisplay ? "SDI8 DL"	: "NTV2_OUTPUTDESTINATION_DUALLINK8";
		#endif	//	!defined (NTV2_DEPRECATE)
		case NTV2_NUM_OUTPUTDESTINATIONS:			break;
	}
	return string ();
}


string NTV2ReferenceSourceToString (const NTV2ReferenceSource inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_REFERENCE_EXTERNAL:		return inForRetailDisplay ? "Reference In"	: "NTV2_REFERENCE_EXTERNAL";
		case NTV2_REFERENCE_INPUT1:			return inForRetailDisplay ? "Input 1"		: "NTV2_REFERENCE_INPUT1";
		case NTV2_REFERENCE_INPUT2:			return inForRetailDisplay ? "Input 2"		: "NTV2_REFERENCE_INPUT2";
		case NTV2_REFERENCE_FREERUN:		return inForRetailDisplay ? "Free Run"		: "NTV2_REFERENCE_FREERUN";
		case NTV2_REFERENCE_ANALOG_INPUT:	return inForRetailDisplay ? "Analog In"		: "NTV2_REFERENCE_ANALOG_INPUT";
		case NTV2_REFERENCE_HDMI_INPUT:		return inForRetailDisplay ? "HDMI In 1"		: "NTV2_REFERENCE_HDMI_INPUT";
		case NTV2_REFERENCE_INPUT3:			return inForRetailDisplay ? "Input 3"		: "NTV2_REFERENCE_INPUT3";
		case NTV2_REFERENCE_INPUT4:			return inForRetailDisplay ? "Input 4"		: "NTV2_REFERENCE_INPUT4";
		case NTV2_REFERENCE_INPUT5:			return inForRetailDisplay ? "Input 5"		: "NTV2_REFERENCE_INPUT5";
		case NTV2_REFERENCE_INPUT6:			return inForRetailDisplay ? "Input 6"		: "NTV2_REFERENCE_INPUT6";
		case NTV2_REFERENCE_INPUT7:			return inForRetailDisplay ? "Input 7"		: "NTV2_REFERENCE_INPUT7";
		case NTV2_REFERENCE_INPUT8:			return inForRetailDisplay ? "Input 8"		: "NTV2_REFERENCE_INPUT8";
		case NTV2_REFERENCE_SFP1_PCR:		return inForRetailDisplay ? "SFP 1 PCR"		: "NTV2_REFERENCE_SFP1_PCR";
		case NTV2_REFERENCE_SFP1_PTP:		return inForRetailDisplay ? "SFP 1 PTP"		: "NTV2_REFERENCE_SFP1_PTP";
		case NTV2_REFERENCE_SFP2_PCR:		return inForRetailDisplay ? "SFP 2 PCR"		: "NTV2_REFERENCE_SFP2_PCR";
		case NTV2_REFERENCE_SFP2_PTP:		return inForRetailDisplay ? "SFP 2 PTP"		: "NTV2_REFERENCE_SFP2_PTP";
		case NTV2_REFERENCE_HDMI_INPUT2:	return inForRetailDisplay ? "HDMI In 2"		: "NTV2_REFERENCE_HDMI_INPUT2";
		case NTV2_REFERENCE_HDMI_INPUT3:	return inForRetailDisplay ? "HDMI In 3"		: "NTV2_REFERENCE_HDMI_INPUT3";
		case NTV2_REFERENCE_HDMI_INPUT4:	return inForRetailDisplay ? "HDMI In 4"		: "NTV2_REFERENCE_HDMI_INPUT4";
		case NTV2_NUM_REFERENCE_INPUTS:		break;
	}
	return string ();
}



string NTV2RegisterWriteModeToString (const NTV2RegisterWriteMode inValue, const bool inForRetailDisplay)
{
	switch (inValue)
	{
		case NTV2_REGWRITE_SYNCTOFIELD:		return inForRetailDisplay ? "Sync To Field"	: "NTV2_REGWRITE_SYNCTOFIELD";
		case NTV2_REGWRITE_SYNCTOFRAME:		return inForRetailDisplay ? "Sync To Frame" : "NTV2_REGWRITE_SYNCTOFRAME";
		case NTV2_REGWRITE_IMMEDIATE:		return inForRetailDisplay ? "Immediate"		: "NTV2_REGWRITE_IMMEDIATE";
		case NTV2_REGWRITE_SYNCTOFIELD_AFTER10LINES:	break;
	}
	return string ();
}


static const char * NTV2InterruptEnumStrings (const INTERRUPT_ENUMS inInterruptEnumValue)
{
	static const char *	sInterruptEnumStrings []	= {	"eOutput1",
														"eInterruptMask",
														"eInput1",
														"eInput2",
														"eAudio",
														"eAudioInWrap",
														"eAudioOutWrap",
														"eDMA1",
														"eDMA2",
														"eDMA3",
														"eDMA4",
														"eChangeEvent",
														"eGetIntCount",
														"eWrapRate",
														"eUart1Tx",
														"eUart1Rx",
														"eAuxVerticalInterrupt",
														"ePushButtonChange",
														"eLowPower",
														"eDisplayFIFO",
														"eSATAChange",
														"eTemp1High",
														"eTemp2High",
														"ePowerButtonChange",
														"eInput3",
														"eInput4",
														"eUart2Tx",
														"eUart2Rx",
														"eHDMIRxV2HotplugDetect",
														"eInput5",
														"eInput6",
														"eInput7",
														"eInput8",
														"eInterruptMask2",
														"eOutput2",
														"eOutput3",
														"eOutput4",
														"eOutput5",
														"eOutput6",
														"eOutput7",
                                                        "eOutput8"};
    if (inInterruptEnumValue >= eOutput1 && inInterruptEnumValue < eNumInterruptTypes)
		return sInterruptEnumStrings [inInterruptEnumValue];
	else
		return NULL;

}	//	NTV2InterruptEnumStrings


std::string NTV2InterruptEnumToString (const INTERRUPT_ENUMS inInterruptEnumValue)
{
	const char *	pString	(::NTV2InterruptEnumStrings (inInterruptEnumValue));
	return std::string (pString ? pString : "");
}

std::string NTV2IpErrorEnumToString (const NTV2IpError inIpErrorEnumValue)
{
    switch (inIpErrorEnumValue)
    {
        case NTV2IpErrNone:                         return "";
        case NTV2IpErrInvalidChannel:               return "Invalid channel";
        case NTV2IpErrInvalidFormat:                return "Invalid format";
        case NTV2IpErrInvalidBitdepth:              return "Invalid bit depth";
        case NTV2IpErrInvalidUllHeight:             return "Invalid height in ull mode";
        case NTV2IpErrInvalidUllLevels:             return "Invalid number of levels in ull mode";
        case NTV2IpErrUllNotSupported:              return "Ull mode not supported";
        case NTV2IpErrNotReady:                     return "KonaIP card not ready";
        case NTV2IpErrSoftwareMismatch:             return "Host software does not match device firmware";
        case NTV2IpErrSFP1NotConfigured:            return "SFP 1 not configured";
        case NTV2IpErrSFP2NotConfigured:            return "SFP 2 not configured";
        case NTV2IpErrInvalidIGMPVersion:           return "Invalid IGMP version";
        case NTV2IpErrCannotGetMacAddress:          return "Failed to retrieve MAC address from ARP table";
        case NTV2IpErr2022_7NotSupported:           return "2022-7 not supported for by this firmware";
        case NTV2IpErrWriteSOMToMB:                 return "Could not write SOM to MB";
        case NTV2IpErrWriteSeqToMB:                 return "Could not write sequence number to MB";
        case NTV2IpErrWriteCountToMB:               return "Could not write count to MB";
        case NTV2IpErrTimeoutNoSOM:                 return "MB response timeout (no SOM)";
        case NTV2IpErrTimeoutNoSeq:                 return "MB response timeout (no sequence number)";
        case NTV2IpErrTimeoutNoBytecount:           return "MB response timeout (no bytecount)";
        case NTV2IpErrExceedsFifo:                  return "Response exceeds FIFO length";
        case NTV2IpErrNoResponseFromMB:             return "No response from MB";
        case NTV2IpErrAcquireMBTimeout:             return "AcquireMailBoxLock timeout";
        case NTV2IpErrInvalidMBResponse:            return "Invalid response from MB";
        case NTV2IpErrInvalidMBResponseSize:        return "Invalid response size from MB";
        case NTV2IpErrInvalidMBResponseNoMac:       return "MAC Address not found in response from MB";
        case NTV2IpErrMBStatusFail:                 return "MB Status Failure";
        case NTV2IpErrGrandMasterInfo:              return "PTP Grand Master Info not found";
        case NTV2IpErrSDPTooLong:                   return "SDP too long";
        case NTV2IpErrSDPNotFound:                  return "SDP not found";
        case NTV2IpErrSDPEmpty:                     return "SDP is empty";
        case NTV2IpErrSDPInvalid:                   return "SDP is not valid";
        case NTV2IpErrSDPURLInvalid:                return "Invalid SDP URL";
        case NTV2IpErrSDPNoVideo:                   return "SDP does not contain video";
        case NTV2IpErrSDPNoAudio:                   return "SDP does not contain audio";
        case NTV2IpErrSDPNoANC:                     return "SDP does not contain metadata";
        case NTV2IpErrSFPNotFound:                  return "SFP data not found";
        case NTV2IpErrInvalidConfig:                return "Invalid configuration";
        default:                                    return "Unknown IP error";
    }
}

ostream & operator << (ostream & inOutStream, const RP188_STRUCT & inObj)
{
	return inOutStream	<< "DBB=0x" << hex << setw (8) << setfill ('0') << inObj.DBB
						<< "|HI=0x" << hex << setw (8) << setfill ('0') << inObj.High
						<< "|LO=0x" << hex << setw (8) << setfill ('0') << inObj.Low
						<< dec;
}	//	RP188_STRUCT ostream operator


string NTV2GetBitfileName (const NTV2DeviceID inBoardID)
{
	#if defined (MSWindows)
		switch (inBoardID)
		{
			case DEVICE_ID_NOTFOUND:					break;
			case DEVICE_ID_CORVID1:						return "corvid1_pcie.bit";
			case DEVICE_ID_CORVID22:					return "corvid22_pcie.bit";
			case DEVICE_ID_CORVID24:					return "corvid24_pcie.bit";
			case DEVICE_ID_CORVID3G:					return "corvid3G_pcie.bit";
			case DEVICE_ID_CORVID44:					return "corvid44_pcie.bit";
			case DEVICE_ID_CORVID88:					return "corvid88_pcie.bit";
			case DEVICE_ID_CORVIDHEVC:					return "corvid_hevc.bit";
			case DEVICE_ID_IO4K:						return "io4k_pcie.bit";
			case DEVICE_ID_IO4KUFC:						return "io4k_ufc_pcie.bit";
			case DEVICE_ID_IOEXPRESS:					return "ioexpress_pcie.bit";
			case DEVICE_ID_IOXT:						return "ioxt_pcie.bit";
			case DEVICE_ID_KONA3G:						return "kona3g_pcie.bit";
			case DEVICE_ID_KONA3GQUAD:					return "kona3g_quad_pcie.bit";
			case DEVICE_ID_KONA4:						return "kona4_pcie.bit";
			case DEVICE_ID_KONA4UFC:					return "kona4_ufc_pcie.bit";
            case DEVICE_ID_KONAIP_2022:                 return "kip_s2022.mcs";
			case DEVICE_ID_KONAIP_4CH_2SFP:				return "s2022_56_2p2ch_rxtx.mcs";
            case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:		return "kip_j2k_1i1o.mcs";
            case DEVICE_ID_KONAIP_2TX_1SFP_J2K:			return "kip_j2k_2o.mcs";
			case DEVICE_ID_KONAIP_1RX_1TX_2110:			return "s2110_1rx_1tx.mcs";
			case DEVICE_ID_KONALHEPLUS:					return "lheplus_pcie.bit";
			case DEVICE_ID_KONALHI:						return "lhi_pcie.bit";
			case DEVICE_ID_TTAP:						return "ttap_pcie.bit";
			case DEVICE_ID_IO4KPLUS:					return "io4kplus_pcie.bit";
            case DEVICE_ID_IOIP_2022:					return "ioip_s2022.mcs";
            case DEVICE_ID_IOIP_2110:					return "ioip_s2110.mcs";
            case DEVICE_ID_KONAIP_2110:                 return "kip_s2110.mcs";
			case DEVICE_ID_KONA1:						return "kona1_pcie.bit";
            case DEVICE_ID_KONAHDMI:					return "kona_hdmi_4rx.bit";
			case DEVICE_ID_KONA5:						return "kona5_pcie.bit";
            case DEVICE_ID_KONA5_12G:					return "kona5_12G_pcie.bit";
			case DEVICE_ID_CORVID44_12G:				return "corvid44_12G_pcie.bit";
            default:									return "";
		}
	#else
		switch (inBoardID)
		{
			case DEVICE_ID_NOTFOUND:					break;
			case DEVICE_ID_CORVID1:						return "corvid1pcie.bit";
			case DEVICE_ID_CORVID22:					return "Corvid22.bit";
			case DEVICE_ID_CORVID24:					return "corvid24_quad.bit";
			case DEVICE_ID_CORVID3G:					return "corvid1_3gpcie.bit";
			case DEVICE_ID_CORVID44:					return "corvid_44.bit";
			case DEVICE_ID_CORVID88:					return "corvid_88.bit";
			case DEVICE_ID_CORVIDHEVC:					return "corvid_hevc.bit";
			case DEVICE_ID_IO4K:						return "IO_XT_4K.bit";
			case DEVICE_ID_IO4KUFC:						return "IO_XT_4K_UFC.bit";
			case DEVICE_ID_IOEXPRESS:					return "chekov_00_pcie.bit";
			case DEVICE_ID_IOXT:						return "top_io_tx.bit";
			case DEVICE_ID_KONA3G:						return "k3g_top.bit";
			case DEVICE_ID_KONA3GQUAD:					return "k3g_quad.bit";
			case DEVICE_ID_KONA4:						return "kona_4_quad.bit";
			case DEVICE_ID_KONA4UFC:					return "kona_4_ufc.bit";
            case DEVICE_ID_KONAIP_2022:                 return "kip_s2022.mcs";
			case DEVICE_ID_KONAIP_4CH_2SFP:				return "s2022_56_2p2ch_rxtx.mcs";
            case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:		return "kip_j2k_1i1o.mcs";
            case DEVICE_ID_KONAIP_2TX_1SFP_J2K:			return "kip_j2k_2o.mcs";
            case DEVICE_ID_KONAIP_1RX_1TX_2110:			return "s2110_1rx_1tx.mcs";
            case DEVICE_ID_KONALHEPLUS:					return "lhe_12_pcie.bit";
			case DEVICE_ID_KONALHI:						return "top_pike.bit";
			case DEVICE_ID_TTAP:						return "t_tap_top.bit";
            case DEVICE_ID_IO4KPLUS:					return "io4kp.bit";
            case DEVICE_ID_IOIP_2022:					return "ioip_s2022.mcs";
            case DEVICE_ID_IOIP_2110:					return "ioip_s2110.mcs";
            case DEVICE_ID_KONAIP_2110:                 return "kip_s2110.mcs";
            case DEVICE_ID_KONAHDMI:					return "kona_hdmi_4rx.bit";
            case DEVICE_ID_KONA1:						return "kona1.bit";
            case DEVICE_ID_KONA5:                       return "kona5.bit";
            case DEVICE_ID_KONA5_12G:                       return "kona5_12g.bit";
            default:									return "";
		}
	#endif	//	else not MSWindows
	return "";
}	//	NTV2GetBitfileName


bool NTV2IsCompatibleBitfileName (const string & inBitfileName, const NTV2DeviceID inDeviceID)
{
	const string	deviceBitfileName	(::NTV2GetBitfileName (inDeviceID));
	if (inBitfileName == deviceBitfileName)
		return true;

	switch (inDeviceID)
	{
		case DEVICE_ID_KONA3GQUAD:	return ::NTV2GetBitfileName (DEVICE_ID_KONA3G) == inBitfileName;
		case DEVICE_ID_KONA3G:		return ::NTV2GetBitfileName (DEVICE_ID_KONA3GQUAD) == inBitfileName;

		case DEVICE_ID_KONA4:		return ::NTV2GetBitfileName (DEVICE_ID_KONA4UFC) == inBitfileName;
		case DEVICE_ID_KONA4UFC:	return ::NTV2GetBitfileName (DEVICE_ID_KONA4) == inBitfileName;

		case DEVICE_ID_IO4K:		return ::NTV2GetBitfileName (DEVICE_ID_IO4KUFC) == inBitfileName;
		case DEVICE_ID_IO4KUFC:		return ::NTV2GetBitfileName (DEVICE_ID_IO4K) == inBitfileName;

		default:					break;
	}
	return false;

}	//	IsCompatibleBitfileName


NTV2DeviceID NTV2GetDeviceIDFromBitfileName (const string & inBitfileName)
{
	typedef map <string, NTV2DeviceID>	BitfileName2DeviceID;
	static BitfileName2DeviceID			sBitfileName2DeviceID;
	if (sBitfileName2DeviceID.empty ())
	{
		static	NTV2DeviceID	sDeviceIDs [] =	{	DEVICE_ID_KONA3GQUAD,	DEVICE_ID_KONA3G,	DEVICE_ID_KONA4,		DEVICE_ID_KONA4UFC,	DEVICE_ID_KONALHI,
													DEVICE_ID_KONALHEPLUS,	DEVICE_ID_TTAP,		DEVICE_ID_CORVID1,		DEVICE_ID_CORVID22,	DEVICE_ID_CORVID24,
													DEVICE_ID_CORVID3G,		DEVICE_ID_IOXT,		DEVICE_ID_IOEXPRESS,	DEVICE_ID_IO4K,		DEVICE_ID_IO4KUFC,
                                                    DEVICE_ID_KONA1,		DEVICE_ID_KONAHDMI, DEVICE_ID_KONA5,        DEVICE_ID_KONA5_12G,DEVICE_ID_CORVID44_12G,
													DEVICE_ID_NOTFOUND };
		for (unsigned ndx (0);  ndx < sizeof (sDeviceIDs) / sizeof (NTV2DeviceID);  ndx++)
			sBitfileName2DeviceID [::NTV2GetBitfileName (sDeviceIDs [ndx])] = sDeviceIDs [ndx];
	}
	return sBitfileName2DeviceID [inBitfileName];
}


string NTV2GetFirmwareFolderPath (void)
{
	#if defined (AJAMac)
		return "/Library/Application Support/AJA/Firmware";
	#elif defined (MSWindows)
		HKEY	hKey		(NULL);
		DWORD	bufferSize	(1024);
		char *	lpData		(new char [bufferSize]);

		if (RegOpenKeyExA (HKEY_CURRENT_USER, "Software\\AJA", NULL, KEY_READ, &hKey) == ERROR_SUCCESS
			&& RegQueryValueExA (hKey, "firmwarePath", NULL, NULL, (LPBYTE) lpData, &bufferSize) == ERROR_SUCCESS)
				return string (lpData);
		RegCloseKey (hKey);
		return "";
    #elif defined (AJALinux)
        return "/opt/aja/firmware";
	#else
		return "";
	#endif
}


NTV2DeviceIDSet NTV2GetSupportedDevices (const NTV2DeviceKinds inKinds)
{
	static const NTV2DeviceID	sValidDeviceIDs []	= {	DEVICE_ID_CORVID1,
														DEVICE_ID_CORVID22,
														DEVICE_ID_CORVID24,
														DEVICE_ID_CORVID3G,
														DEVICE_ID_CORVID44,
														DEVICE_ID_CORVID88,
 														DEVICE_ID_CORVIDHBR,
														DEVICE_ID_CORVIDHEVC,
														DEVICE_ID_IO4K,
														DEVICE_ID_IO4KUFC,
														DEVICE_ID_IO4KPLUS,
                                                        DEVICE_ID_IOIP_2022,
                                                        DEVICE_ID_IOIP_2110,
														DEVICE_ID_IOEXPRESS,
														DEVICE_ID_IOXT,
														DEVICE_ID_KONA1,
														DEVICE_ID_KONA3G,
														DEVICE_ID_KONA3GQUAD,
														DEVICE_ID_KONA4,
														DEVICE_ID_KONA4UFC,
														DEVICE_ID_KONA5,
                                                        DEVICE_ID_KONAHDMI,
														DEVICE_ID_KONAIP_2022,
														DEVICE_ID_KONAIP_4CH_2SFP,
														DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K,
														DEVICE_ID_KONAIP_2TX_1SFP_J2K,
														DEVICE_ID_KONAIP_1RX_1TX_2110,
														DEVICE_ID_KONAIP_2110,
														DEVICE_ID_KONALHEPLUS,
														DEVICE_ID_KONALHI,
														DEVICE_ID_KONALHIDVI,
														DEVICE_ID_TTAP,
                                                        DEVICE_ID_KONA5_12G,
														DEVICE_ID_CORVID44_12G,
                                                        DEVICE_ID_NOTFOUND	};
	NTV2DeviceIDSet	result;
	if (inKinds != NTV2_DEVICEKIND_NONE)
		for (unsigned ndx(0);  ndx < sizeof(sValidDeviceIDs) / sizeof(NTV2DeviceID);  ndx++)
		{
			const NTV2DeviceID	deviceID(sValidDeviceIDs[ndx]);
			if (deviceID != DEVICE_ID_NOTFOUND)
			{
				if (inKinds == NTV2_DEVICEKIND_ALL)
					{result.insert (deviceID);	continue;}
				//	else ...
				bool insertIt (false);
				if (insertIt)
					;
				else if (inKinds & NTV2_DEVICEKIND_INPUT  &&  ::NTV2DeviceCanDoCapture(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_OUTPUT  &&  ::NTV2DeviceCanDoPlayback(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_SDI  &&  (::NTV2DeviceGetNumVideoInputs(deviceID)+::NTV2DeviceGetNumVideoOutputs(deviceID)) > 0)
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_HDMI  &&  (::NTV2DeviceGetNumHDMIVideoInputs(deviceID)+::NTV2DeviceGetNumHDMIVideoOutputs(deviceID)) > 0)
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_ANALOG  &&  (::NTV2DeviceGetNumAnalogVideoInputs(deviceID)+::NTV2DeviceGetNumAnalogVideoOutputs(deviceID)) > 0)
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_SFP  &&  ::NTV2DeviceCanDoIP(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_EXTERNAL  &&  ::NTV2DeviceIsExternalToHost(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_4K  &&  ::NTV2DeviceCanDo4KVideo(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_12G  &&  ::NTV2DeviceCanDo12GSDI(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_6G  &&  ::NTV2DeviceCanDo12GSDI(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_CUSTOM_ANC  &&  ::NTV2DeviceCanDoCustomAnc(deviceID))
					insertIt = true;
				else if (inKinds & NTV2_DEVICEKIND_RELAYS  &&  ::NTV2DeviceHasSDIRelays(deviceID))
					insertIt = true;
				if (insertIt)
					result.insert (deviceID);
			}
		}	//	for each supported device
	return result;
}


ostream & operator << (ostream & inOutStr, const NTV2DeviceIDSet & inSet)
{
	for (NTV2DeviceIDSetConstIter iter (inSet.begin ());  iter != inSet.end ();  ++iter)
		inOutStr << (iter != inSet.begin () ? ", " : "") << ::NTV2DeviceIDToString (*iter);
	return inOutStr;
}


std::string NTV2GetVersionString (const bool inDetailed)
{
	ostringstream	oss;

	oss << AJA_NTV2_SDK_VERSION_MAJOR << "." << AJA_NTV2_SDK_VERSION_MINOR << "." << AJA_NTV2_SDK_VERSION_POINT;
	if (!string (AJA_NTV2_SDK_BUILD_TYPE).empty ())
		oss << " " << AJA_NTV2_SDK_BUILD_TYPE << AJA_NTV2_SDK_BUILD_NUMBER;
	#if defined (NTV2_DEPRECATE)
		if (inDetailed)
			oss << " (NTV2_DEPRECATE)";
	#endif
	if (inDetailed)
		oss	<< " built on " << AJA_NTV2_SDK_BUILD_DATETIME;
	return oss.str ();
}


string NTV2RegisterNumberToString (const NTV2RegisterNumber inValue)
{
	string	result	(::NTV2RegisterNameString (inValue));
	if (result.empty ())
	{
		result = CNTV2RegisterExpert::GetDisplayName(inValue);
		if (result.empty())
		{
			ostringstream	oss;	//oss << "0x" << hex << inValue << dec;
		oss << inValue;
			result = oss.str();
	}
	}
	return result;
}


string AutoCircVidProcModeToString (const AutoCircVidProcMode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		case AUTOCIRCVIDPROCMODE_MIX:		return inCompactDisplay ? "Mix"		: "AUTOCIRCVIDPROCMODE_MIX";
		case AUTOCIRCVIDPROCMODE_HORZWIPE:	return inCompactDisplay ? "HWipe"	: "AUTOCIRCVIDPROCMODE_HORZWIPE";
		case AUTOCIRCVIDPROCMODE_VERTWIPE:	return inCompactDisplay ? "VWipe"	: "AUTOCIRCVIDPROCMODE_VERTWIPE";
		case AUTOCIRCVIDPROCMODE_KEY:		return inCompactDisplay ? "Key"		: "AUTOCIRCVIDPROCMODE_KEY";
		case AUTOCIRCVIDPROCMODE_INVALID:	return inCompactDisplay ? "n/a"		: "AUTOCIRCVIDPROCMODE_INVALID";
	}
	return "??";
}


string NTV2ColorCorrectionModeToString (const NTV2ColorCorrectionMode inValue, const bool inCompactDisplay)
{
	switch (inValue)
	{
		case NTV2_CCMODE_OFF:		return inCompactDisplay ? "Off"		: "NTV2_CCMODE_OFF";
		case NTV2_CCMODE_RGB:		return inCompactDisplay ? "RGB"		: "NTV2_CCMODE_RGB";
		case NTV2_CCMODE_YCbCr:		return inCompactDisplay ? "YCbCr"	: "NTV2_CCMODE_YCbCr";
		case NTV2_CCMODE_3WAY:		return inCompactDisplay ? "3way"	: "NTV2_CCMODE_3WAY";
		case NTV2_CCMODE_INVALID:	return inCompactDisplay ? "n/a"		: "NTV2_CCMODE_INVALID";
	}
	return "??";
}

bool convertHDRFloatToRegisterValues(const HDRFloatValues & inFloatValues, HDRRegValues & outRegisterValues)
{
	if ((inFloatValues.greenPrimaryX < 0 || inFloatValues.greenPrimaryX > 1.0) ||
		(inFloatValues.greenPrimaryY < 0 || inFloatValues.greenPrimaryY > 1.0) ||
		(inFloatValues.bluePrimaryX < 0 || inFloatValues.bluePrimaryX > 1.0) ||
		(inFloatValues.bluePrimaryY < 0 || inFloatValues.bluePrimaryY > 1.0) ||
		(inFloatValues.redPrimaryX < 0 || inFloatValues.redPrimaryX > 1.0) ||
		(inFloatValues.redPrimaryY < 0 || inFloatValues.redPrimaryY > 1.0) ||
		(inFloatValues.whitePointX < 0 || inFloatValues.whitePointX > 1.0) ||
        (inFloatValues.whitePointY < 0 || inFloatValues.whitePointY > 1.0) ||
        (inFloatValues.minMasteringLuminance < 0 || inFloatValues.minMasteringLuminance > 6.5535))
		return false;

	outRegisterValues.greenPrimaryX = static_cast<uint16_t>(inFloatValues.greenPrimaryX / 0.00002);
	outRegisterValues.greenPrimaryY = static_cast<uint16_t>(inFloatValues.greenPrimaryY / 0.00002);
	outRegisterValues.bluePrimaryX = static_cast<uint16_t>(inFloatValues.bluePrimaryX / 0.00002);
	outRegisterValues.bluePrimaryY = static_cast<uint16_t>(inFloatValues.bluePrimaryY / 0.00002);
	outRegisterValues.redPrimaryX = static_cast<uint16_t>(inFloatValues.redPrimaryX / 0.00002);
	outRegisterValues.redPrimaryY = static_cast<uint16_t>(inFloatValues.redPrimaryY / 0.00002);
	outRegisterValues.whitePointX = static_cast<uint16_t>(inFloatValues.whitePointX / 0.00002);
	outRegisterValues.whitePointY = static_cast<uint16_t>(inFloatValues.whitePointY / 0.00002);
    outRegisterValues.minMasteringLuminance = static_cast<uint16_t>(inFloatValues.minMasteringLuminance / 0.0001);
    outRegisterValues.maxMasteringLuminance = inFloatValues.maxMasteringLuminance;
    outRegisterValues.maxContentLightLevel = inFloatValues.maxContentLightLevel;
    outRegisterValues.maxFrameAverageLightLevel = inFloatValues.maxFrameAverageLightLevel;
    outRegisterValues.electroOpticalTransferFunction = inFloatValues.electroOpticalTransferFunction;
    outRegisterValues.staticMetadataDescriptorID = inFloatValues.staticMetadataDescriptorID;
	return true;
}

bool convertHDRRegisterToFloatValues(const HDRRegValues & inRegisterValues, HDRFloatValues & outFloatValues)
{
	if ((inRegisterValues.greenPrimaryX > 0xC350) ||
		(inRegisterValues.greenPrimaryY > 0xC350) ||
		(inRegisterValues.bluePrimaryX > 0xC350) ||
		(inRegisterValues.bluePrimaryY > 0xC350) ||
		(inRegisterValues.redPrimaryX > 0xC350) ||
		(inRegisterValues.redPrimaryY > 0xC350) ||
		(inRegisterValues.whitePointX > 0xC350) ||
        (inRegisterValues.whitePointY > 0xC350))
		return false;
	outFloatValues.greenPrimaryX = static_cast<float>(inRegisterValues.greenPrimaryX * 0.00002);
	outFloatValues.greenPrimaryY = static_cast<float>(inRegisterValues.greenPrimaryY * 0.00002);
	outFloatValues.bluePrimaryX = static_cast<float>(inRegisterValues.bluePrimaryX * 0.00002);
	outFloatValues.bluePrimaryY = static_cast<float>(inRegisterValues.bluePrimaryY * 0.00002);
	outFloatValues.redPrimaryX = static_cast<float>(inRegisterValues.redPrimaryX * 0.00002);
	outFloatValues.redPrimaryY = static_cast<float>(inRegisterValues.redPrimaryY * 0.00002);
	outFloatValues.whitePointX = static_cast<float>(inRegisterValues.whitePointX * 0.00002);
	outFloatValues.whitePointY = static_cast<float>(inRegisterValues.whitePointY * 0.00002);
    outFloatValues.minMasteringLuminance = static_cast<float>(inRegisterValues.minMasteringLuminance * 0.0001);
    outFloatValues.maxMasteringLuminance = inRegisterValues.maxMasteringLuminance;
    outFloatValues.maxContentLightLevel = inRegisterValues.maxContentLightLevel;
    outFloatValues.maxFrameAverageLightLevel = inRegisterValues.maxFrameAverageLightLevel;
    outFloatValues.electroOpticalTransferFunction = inRegisterValues.electroOpticalTransferFunction;
    outFloatValues.staticMetadataDescriptorID = inRegisterValues.staticMetadataDescriptorID;
	return true;
}

void setHDRDefaultsForBT2020(HDRRegValues & outRegisterValues)
{
    outRegisterValues.greenPrimaryX = 0x2134;
    outRegisterValues.greenPrimaryY = 0x9BAA;
    outRegisterValues.bluePrimaryX = 0x1996;
    outRegisterValues.bluePrimaryY = 0x08FC;
    outRegisterValues.redPrimaryX = 0x8A48;
    outRegisterValues.redPrimaryY = 0x3908;
    outRegisterValues.whitePointX = 0x3D13;
    outRegisterValues.whitePointY = 0x4042;
    outRegisterValues.maxMasteringLuminance = 0x2710;
    outRegisterValues.minMasteringLuminance = 0x0032;
    outRegisterValues.maxContentLightLevel = 0;
    outRegisterValues.maxFrameAverageLightLevel = 0;
    outRegisterValues.electroOpticalTransferFunction = 0x02;
    outRegisterValues.staticMetadataDescriptorID = 0x00;
}

void setHDRDefaultsForDCIP3(HDRRegValues & outRegisterValues)
{
    outRegisterValues.greenPrimaryX = 0x33C2;
    outRegisterValues.greenPrimaryY = 0x86C4;
    outRegisterValues.bluePrimaryX = 0x1D4C;
    outRegisterValues.bluePrimaryY = 0x0BB8;
    outRegisterValues.redPrimaryX = 0x84D0;
    outRegisterValues.redPrimaryY = 0x3E80;
    outRegisterValues.whitePointX = 0x3D13;
    outRegisterValues.whitePointY = 0x4042;
    outRegisterValues.maxMasteringLuminance = 0x02E8;
    outRegisterValues.minMasteringLuminance = 0x0032;
    outRegisterValues.maxContentLightLevel = 0;
    outRegisterValues.maxFrameAverageLightLevel = 0;
    outRegisterValues.electroOpticalTransferFunction = 0x02;
    outRegisterValues.staticMetadataDescriptorID = 0x00;
}


ostream & operator << (ostream & inOutStr, const NTV2OutputCrosspointIDs & inList)
{
	inOutStr << "[";
	for (NTV2OutputCrosspointIDsConstIter it (inList.begin());  it != inList.end();  )
	{
		inOutStr << ::NTV2OutputCrosspointIDToString (*it);
		++it;
		if (it != inList.end())
			inOutStr << ",";
	}
	inOutStr << "]";
	return inOutStr;
}

/*
static ostream & operator << (ostream & inOutStr, const NTV2InputCrosspointIDs & inList)
{
	inOutStr << "[";
	for (NTV2InputCrosspointIDsConstIter it (inList.begin());  it != inList.end();  )
	{
		inOutStr << ::NTV2InputCrosspointIDToString (*it);
		++it;
		if (it != inList.end())
			inOutStr << ",";
	}
	inOutStr << "]";
	return inOutStr;
}
*/

ostream & operator << (ostream & inOutStream, const NTV2StringSet & inData)
{
	for (NTV2StringSetConstIter it(inData.begin());  it != inData.end();  )
	{
		inOutStream	<< *it;
		if (++it != inData.end())
			inOutStream << ", ";
	}
	return inOutStream;
}


NTV2RegisterReads FromRegNumSet (const NTV2RegNumSet & inRegNumSet)
{
	NTV2RegisterReads	result;
	for (NTV2RegNumSetConstIter it (inRegNumSet.begin());  it != inRegNumSet.end();  ++it)
		result.push_back (NTV2RegInfo (*it));
	return result;
}

NTV2RegNumSet ToRegNumSet (const NTV2RegisterReads & inRegReads)
{
	NTV2RegNumSet	result;
	for (NTV2RegisterReadsConstIter it (inRegReads.begin());  it != inRegReads.end();  ++it)
		result.insert (it->registerNumber);
	return result;
}

bool GetRegNumChanges (const NTV2RegNumSet & inBefore, const NTV2RegNumSet & inAfter, NTV2RegNumSet & outGone, NTV2RegNumSet & outSame, NTV2RegNumSet & outNew)
{
	outGone.clear();  outSame.clear();  outNew.clear();
	set_difference (inBefore.begin(), inBefore.end(), inAfter.begin(), inAfter.end(),  std::inserter(outGone, outGone.begin()));
	set_difference (inAfter.begin(), inAfter.end(), inBefore.begin(), inBefore.end(),  std::inserter(outNew, outNew.begin()));
	set_intersection (inBefore.begin(), inBefore.end(),  inAfter.begin(), inAfter.end(),  std::inserter(outSame, outSame.begin()));
	return true;
}
