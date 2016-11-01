/**
	@file		ntv2transcode.cpp
	@brief		Implements a number of pixel format transcoder functions.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2transcode.h"
#include "ntv2endian.h"


bool ConvertLine_2vuy_to_v210 (const UByte * pSrc2vuyLine, ULWord * pDstv210Line, const ULWord inNumPixels)
{
	if (!pSrc2vuyLine || !pDstv210Line || !inNumPixels)
		return false;

	for (UWord inputCount = 0, outputCount = 0;   inputCount < (inNumPixels * 2);   outputCount += 4, inputCount += 12)
	{
		pDstv210Line [outputCount    ] = NTV2EndianSwap32HtoL ((pSrc2vuyLine [inputCount + 0] << 2) + (pSrc2vuyLine [inputCount + 1 ] << 12) + (pSrc2vuyLine [inputCount + 2 ] << 22));
		pDstv210Line [outputCount + 1] = NTV2EndianSwap32HtoL ((pSrc2vuyLine [inputCount + 3] << 2) + (pSrc2vuyLine [inputCount + 4 ] << 12) + (pSrc2vuyLine [inputCount + 5 ] << 22));
		pDstv210Line [outputCount + 2] = NTV2EndianSwap32HtoL ((pSrc2vuyLine [inputCount + 6] << 2) + (pSrc2vuyLine [inputCount + 7 ] << 12) + (pSrc2vuyLine [inputCount + 8 ] << 22));
		pDstv210Line [outputCount + 3] = NTV2EndianSwap32HtoL ((pSrc2vuyLine [inputCount + 9] << 2) + (pSrc2vuyLine [inputCount + 10] << 12) + (pSrc2vuyLine [inputCount + 11] << 22));
	}

	return true;

}	//	ConvertLine_2vuy_to_v210



bool ConvertLine_v210_to_2vuy (const ULWord * pSrcv210Line, UByte * pDst2vuyLine, const ULWord inNumPixels)
{
	if (!pSrcv210Line || !pDst2vuyLine || !inNumPixels)
		return false;

	for (ULWord sampleCount = 0, dataCount = 0;   sampleCount < (inNumPixels * 2);   sampleCount += 3, dataCount++)
	{
		const UByte *	pByte	(reinterpret_cast <const UByte *> (&pSrcv210Line [dataCount]));

		//	Endian-agnostic bit shifting...
		pDst2vuyLine [sampleCount    ] = ((pByte [1] & 0x03) << 6) + (pByte [0] >> 2);		//	High-order 8 bits
		pDst2vuyLine [sampleCount + 1] = ((pByte [2] & 0x0F) << 4) + (pByte [1] >> 4);
		pDst2vuyLine [sampleCount + 2] = ((pByte [3] & 0x3F) << 2) + (pByte [2] >> 6);
	}
	
	return true;

}	//	ConvertLine_v210_to_2vuy


bool ConvertLine_8bitABGR_to_10bitABGR (const UByte * pInSrcLine_8bitABGR,  ULWord * pOutDstLine_10BitABGR, const ULWord inNumPixels)
{
    (void)pInSrcLine_8bitABGR;
    (void)pOutDstLine_10BitABGR;
    (void)inNumPixels;
	NTV2_ASSERT (false && "Needs implementation");
	return false;	//	unimplemented
}


bool ConvertLine_8bitABGR_to_10bitRGBDPX (const UByte * pInSrcLine_8bitABGR,  ULWord * pOutDstLine_10BitDPX, const ULWord inNumPixels)
{
    (void)pInSrcLine_8bitABGR;
    (void)pOutDstLine_10BitDPX;
    (void)inNumPixels;
	NTV2_ASSERT (false && "Needs implementation");
	return false;	//	unimplemented
}


// ConvertLineToYCbCr422
// 8 Bit
void ConvertLineToYCbCr422(RGBAlphaPixel * RGBLine, 
						   UByte* YCbCrLine, 
						   LWord numPixels ,
						   LWord startPixel,
						   bool fUseSDMatrix)
{
	YCbCrPixel YCbCr;
	UByte *pYCbCr = &YCbCrLine[(startPixel&~1)*2];   // startPixel needs to be even

	for ( LWord pixel = 0; pixel < numPixels; pixel++ )
	{
		if(fUseSDMatrix) {
			SDConvertRGBAlphatoYCbCr(&RGBLine[pixel],&YCbCr);
		} else {
			HDConvertRGBAlphatoYCbCr(&RGBLine[pixel],&YCbCr);
		}
		if ( pixel & 0x1 )
		{
			// Just Y
			*pYCbCr++ = YCbCr.y;
		}
		else
		{
			*pYCbCr++ = YCbCr.cb;
			*pYCbCr++ = YCbCr.y;
			*pYCbCr++ = YCbCr.cr;

		}

	}

}
// ConvertLineToYCbCr422
// 10 Bit
void ConvertLineToYCbCr422(RGBAlphaPixel * RGBLine, 
						   UWord* YCbCrLine, 
						   LWord numPixels ,
						   LWord startPixel,
						   bool fUseSDMatrix)
{
	YCbCr10BitPixel YCbCr;
	UWord *pYCbCr = &YCbCrLine[(startPixel&~1)*2];   // startPixel needs to be even

	for ( LWord pixel = 0; pixel < numPixels; pixel++ )
	{
		if(fUseSDMatrix) {
			SDConvertRGBAlphatoYCbCr(&RGBLine[pixel],&YCbCr);
		} else {
			HDConvertRGBAlphatoYCbCr(&RGBLine[pixel],&YCbCr);
		}
		if ( pixel & 0x1 )
		{
			// Just Y
			*pYCbCr++ = YCbCr.y;
		}
		else
		{
			*pYCbCr++ = YCbCr.cb;
			*pYCbCr++ = YCbCr.y;
			*pYCbCr++ = YCbCr.cr;

		}

	}

}



// ConvertLinetoRGB
// 8 Bit Version
void ConvertLinetoRGB(UByte * ycbcrBuffer, 
					  RGBAlphaPixel * rgbaBuffer , 
					  ULWord numPixels,
					  bool fUseSDMatrix,
					  bool fUseSMPTERange)
{
	YCbCrAlphaPixel ycbcrPixel;
	UWord Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to RGBAlphaPixels.
	// 2 RGBAlphaPixels at a time.
	Cb1 = *ycbcrBuffer++;
	Y1 = *ycbcrBuffer++;
	Cr1 = *ycbcrBuffer++;
	for ( ULWord count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (UByte)Cb1;
		ycbcrPixel.y = (UByte)Y1; 
		ycbcrPixel.cr = (UByte)Cr1; 

		if(fUseSDMatrix) {
			if (fUseSMPTERange) {
				SDConvertYCbCrtoRGBSmpte(&ycbcrPixel,&rgbaBuffer[count]);
			} else {
				SDConvertYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
			}
		} else {
			if (fUseSMPTERange) {
				HDConvertYCbCrtoRGBSmpte(&ycbcrPixel,&rgbaBuffer[count]);
			} else {
				HDConvertYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
			}
		}
		// Read lone midde Y;
		ycbcrPixel.y = *ycbcrBuffer++;

		// Read Next full bandwidth sample
		Cb2 = *ycbcrBuffer++;
		Y2 = *ycbcrBuffer++;
		Cr2 = *ycbcrBuffer++;

		// Interpolate and write Inpterpolated RGBAlphaPixel
		ycbcrPixel.cb = (UByte)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (UByte)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			if (fUseSMPTERange) {
				SDConvertYCbCrtoRGBSmpte(&ycbcrPixel,&rgbaBuffer[count+1]);
			} else {
				SDConvertYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
			}
		} else {
			if (fUseSMPTERange) {
				HDConvertYCbCrtoRGBSmpte(&ycbcrPixel,&rgbaBuffer[count+1]);
			} else {
				HDConvertYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
			}
		}
		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;
	}
}


// ConvertLinetoRGB
// 10 Bit YCbCr 8 Bit RGB version
void ConvertLinetoRGB(UWord * ycbcrBuffer, 
					  RGBAlphaPixel * rgbaBuffer,
					  ULWord numPixels,
					  bool fUseSDMatrix,
					  bool fUseSMPTERange,
					  bool fAlphaFromLuma)
{
	YCbCr10BitAlphaPixel ycbcrPixel = {0,0,0,0};
	UWord Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to RGBAlphaPixels.
	// 2 RGBAlphaPixels at a time.
	Cb1 = *ycbcrBuffer++;
	Y1 = *ycbcrBuffer++;
	Cr1 = *ycbcrBuffer++;
	for ( ULWord count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (UWord)Cb1;
		ycbcrPixel.y = (UWord)Y1; 
		ycbcrPixel.cr = (UWord)Cr1; 
		if( fAlphaFromLuma )
			ycbcrPixel.Alpha = (UWord)Y1/4;

		if(fUseSDMatrix) {
			if (fUseSMPTERange) {
				SDConvert10BitYCbCrtoRGBSmpte(&ycbcrPixel,&rgbaBuffer[count]);
			} else {
				SDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
			}
		} else {
			if (fUseSMPTERange) {
				HDConvert10BitYCbCrtoRGBSmpte(&ycbcrPixel,&rgbaBuffer[count]);
			} else {
				HDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
			}
		}
		// Read lone midde Y;
		ycbcrPixel.y = *ycbcrBuffer++;

		// Read Next full bandwidth sample
		// unless we are at the end of a line
		if ( (count + 2 ) >= numPixels )
		{
			Cb2 = (UWord)Cb1;
			Y2 = (UWord)Y1;
			Cr2 = (UWord)Cr1;
		}
		else
		{
			Cb2 = *ycbcrBuffer++;
			Y2 = *ycbcrBuffer++;
			Cr2 = *ycbcrBuffer++;
		}
		// Interpolate and write Inpterpolated RGBAlphaPixel
		ycbcrPixel.cb = (UWord)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (UWord)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			if (fUseSMPTERange) {
				SDConvert10BitYCbCrtoRGBSmpte(&ycbcrPixel,&rgbaBuffer[count+1]);
			} else {
				SDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
			}
		} else {
			if (fUseSMPTERange) {
				HDConvert10BitYCbCrtoRGBSmpte(&ycbcrPixel,&rgbaBuffer[count+1]);
			} else {
				HDConvert10BitYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
			}
		}

		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;

	}
}

// ConvertRGBALineToRGB
// 8 bit RGBA to 8 bit RGB (RGB24)
AJAExport
void ConvertRGBALineToRGB(RGBAlphaPixel * rgbaBuffer,
                          ULWord numPixels)
{
	RGBPixel* rgbLineBuffer = (RGBPixel*) rgbaBuffer;

	for ( ULWord pixel=0; pixel<numPixels; pixel++ )
	{
		UByte R = rgbaBuffer->Red;
		UByte G = rgbaBuffer->Green;
		UByte B = rgbaBuffer->Blue;

		rgbLineBuffer->Red		= R;
		rgbLineBuffer->Green	= G;
		rgbLineBuffer->Blue		= B;

		rgbaBuffer++;
		rgbLineBuffer++;
	}
}

// ConvertRGBALineToBGR
// 8 bit RGBA to 8 bit BGR (BGR24)
// Conversion is done into the same buffer
AJAExport
void ConvertRGBALineToBGR(RGBAlphaPixel * rgbaBuffer,
                          ULWord numPixels)
{
	BGRPixel* bgrLineBuffer = (BGRPixel*) rgbaBuffer;

	for ( ULWord pixel=0; pixel<numPixels; pixel++ )
	{
		UByte B = rgbaBuffer->Blue;
		UByte G = rgbaBuffer->Green;
		UByte R = rgbaBuffer->Red;

		bgrLineBuffer->Blue		= B;
		bgrLineBuffer->Green	= G;
		bgrLineBuffer->Red		= R;

		rgbaBuffer++;
		bgrLineBuffer++;
	}
}

// ConvertLineto10BitRGB
// 10 Bit YCbCr and 10 Bit RGB Version
void ConvertLineto10BitRGB(UWord * ycbcrBuffer, 
					  RGBAlpha10BitPixel * rgbaBuffer,
					  ULWord numPixels,
					  bool fUseSDMatrix,
					  bool fUseSMPTERange)
{
	YCbCr10BitAlphaPixel ycbcrPixel = {0,0,0,0};
	UWord Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to RGBAlphaPixels.
	// 2 RGBAlphaPixels at a time.
	Cb1 = *ycbcrBuffer++;
	Y1 = *ycbcrBuffer++;
	Cr1 = *ycbcrBuffer++;
	for ( ULWord count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (UWord)Cb1;
		ycbcrPixel.y = (UWord)Y1; 
		ycbcrPixel.cr = (UWord)Cr1;

		if(fUseSDMatrix) {
			if (fUseSMPTERange) {
				SDConvert10BitYCbCrto10BitRGBSmpte(&ycbcrPixel,&rgbaBuffer[count]);
			} else {
				SDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,&rgbaBuffer[count]);
			}
		} else {
			if (fUseSMPTERange) {
				HDConvert10BitYCbCrto10BitRGBSmpte(&ycbcrPixel,&rgbaBuffer[count]);
			} else {
				HDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,&rgbaBuffer[count]);
			}
		}
		// Read lone midde Y;
		ycbcrPixel.y = *ycbcrBuffer++;

		// Read Next full bandwidth sample
		// unless we are at the end of a line
		if ( (count + 2 ) >= numPixels )
		{
			Cb2 = (UWord)Cb1;
			Y2 = (UWord)Y1;
			Cr2 = (UWord)Cr1;
		}
		else
		{
			Cb2 = *ycbcrBuffer++;
			Y2 = *ycbcrBuffer++;
			Cr2 = *ycbcrBuffer++;
		}
		// Interpolate and write Inpterpolated RGBAlphaPixel
		ycbcrPixel.cb = (UWord)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (UWord)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			if (fUseSMPTERange) {
				SDConvert10BitYCbCrto10BitRGBSmpte(&ycbcrPixel,&rgbaBuffer[count+1]);
			} else {
				SDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
			}
		} else {
			if (fUseSMPTERange) {
				HDConvert10BitYCbCrto10BitRGBSmpte(&ycbcrPixel,&rgbaBuffer[count+1]);
			} else {
				HDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
			}
		}

		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;
	}
}

// ConvertLineto10BitYCbCrA
// 10 Bit YCbCr to 10 Bit YCbCrA
void ConvertLineto10BitYCbCrA(UWord * ycbcrBuffer, 
					  ULWord* ycbcraBuffer,
					  ULWord numPixels)
{
	for ( ULWord count=0; count < numPixels; count++ )
	{
		ULWord value = CCIR601_10BIT_WHITE<<20; /// Set Alpha to '1';
		value |= (*ycbcrBuffer++<<10); // Cb or Cr
		value |= *ycbcrBuffer++;		// Y
		ycbcraBuffer[count] = value; 
	}
}

// ConvertLineto10BitRGB
// 8 Bit RGBA to  and 10 Bit RGB Packed Version
void ConvertLineto10BitRGB(RGBAlphaPixel * rgbaBuffer, 
					  ULWord * rgb10BitBuffer,
					  ULWord numPixels)
					  
{
	for ( ULWord count = 0; count < numPixels; count++ )
	{
		*rgb10BitBuffer = (rgbaBuffer->Blue<<22) + 
					      (rgbaBuffer->Green<<12) + 
						  (rgbaBuffer->Red<<2);
		rgb10BitBuffer++;
		rgbaBuffer++;
	}
}

// ConvertRGBLineto10BitRGB
// 8 Bit RGB and 10 Bit RGB Version
void ConvertRGBLineto10BitRGB(RGBAlphaPixel * rgbaBuffer, 
					  RGBAlpha10BitPixel * rgba10BitBuffer,
					  ULWord numPixels)
{
	for ( ULWord i = 0; i < numPixels; i++ )
	{
		rgba10BitBuffer[i].Blue  = (rgbaBuffer[i].Blue<<2);
		rgba10BitBuffer[i].Green = (rgbaBuffer[i].Green<<2);
		rgba10BitBuffer[i].Red   = (rgbaBuffer[i].Red<<2);
		rgba10BitBuffer[i].Alpha = (rgbaBuffer[i].Alpha<<2);
	}
 
}



// ConvertLineto8BitYCbCr
// 10 Bit YCbCr to 8 Bit YCbCr
void ConvertLineto8BitYCbCr(UWord * ycbcr10BitBuffer, 
					  UByte * ycbcr8BitBuffer,
					  ULWord numPixels)
{
	for ( ULWord pixel=0;pixel<numPixels*2;pixel++)
	{
		ycbcr8BitBuffer[pixel] = ycbcr10BitBuffer[pixel]>>2;
	}

}

// Converts UYVY(2yuv) -> YUY2(yuv2) in place
void Convert8BitYCbCrToYUY2(UByte * ycbcrBuffer, 
					  ULWord numPixels)
{
	for ( ULWord pixel=0;pixel<numPixels*2;pixel+=4)
	{
		UByte Cb = ycbcrBuffer[pixel];
		UByte Y1 = ycbcrBuffer[pixel+1];
		UByte Cr = ycbcrBuffer[pixel+2];
		UByte Y2 = ycbcrBuffer[pixel+3];
		ycbcrBuffer[pixel] = Y1;
		ycbcrBuffer[pixel+1] = Cb;
		ycbcrBuffer[pixel+2] = Y2;
		ycbcrBuffer[pixel+3] = Cr;
	}
}

// Converts 8 Bit ARGB 8 Bit RGBA in place
void ConvertARGBYCbCrToRGBA(UByte* rgbaBuffer,ULWord numPixels)
{
	for ( ULWord pixel=0;pixel<numPixels*4;pixel+=4)
	{
		UByte B = rgbaBuffer[pixel];
		UByte G = rgbaBuffer[pixel+1];
		UByte R = rgbaBuffer[pixel+2];
		UByte A = rgbaBuffer[pixel+3];
		rgbaBuffer[pixel] = A;
		rgbaBuffer[pixel+1] = R;
		rgbaBuffer[pixel+2] = G;
		rgbaBuffer[pixel+3] = B;
	}
}

// Converts 8 Bit ARGB 8 Bit ABGR in place
void ConvertARGBYCbCrToABGR(UByte* rgbaBuffer,ULWord numPixels)
{
	for ( ULWord pixel=0;pixel<numPixels*4;pixel+=4)
	{
		UByte B = rgbaBuffer[pixel];
		UByte G = rgbaBuffer[pixel+1];
		UByte R = rgbaBuffer[pixel+2];
		UByte A = rgbaBuffer[pixel+3];
		rgbaBuffer[pixel] = R;
		rgbaBuffer[pixel+1] = G;
		rgbaBuffer[pixel+2] = B;
		rgbaBuffer[pixel+3] = A;
	}
}

// Convert 8 Bit ARGB to 8 bit RGB
void ConvertARGBToRGB(UByte *rgbaLineBuffer ,UByte * rgbLineBuffer,ULWord numPixels)
{
	for ( ULWord pixel=0;pixel<numPixels*4;pixel+=4)
	{
		UByte B = rgbaLineBuffer[pixel];
		UByte G = rgbaLineBuffer[pixel+1];
		UByte R = rgbaLineBuffer[pixel+2];
		*rgbLineBuffer++ = R;
		*rgbLineBuffer++ = G;
		*rgbLineBuffer++ = B;
		
	}
}
//KAM
// Convert 16 Bit ARGB to 16 bit RGB
void Convert16BitARGBTo16BitRGBEx(UWord *rgbaLineBuffer ,UWord * rgbLineBuffer,ULWord numPixels)
{
	for ( ULWord pixel=0;pixel<numPixels*4;pixel+=4)
	{
		// TODO: is this ordering correct? seems wrong...
		RGBAlpha16BitPixel* pixelBuffer = (RGBAlpha16BitPixel*)&(rgbaLineBuffer[pixel]);
		UWord B = pixelBuffer->Blue;
		UWord G = pixelBuffer->Green;
		UWord R = pixelBuffer->Red;
		*rgbLineBuffer++ = R;
		*rgbLineBuffer++ = G;
		*rgbLineBuffer++ = B;
	}
}
// KAM
void Convert16BitARGBTo16BitRGB(RGBAlpha16BitPixel *rgbaLineBuffer ,UWord * rgbLineBuffer,ULWord numPixels)
{
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		UWord B = rgbaLineBuffer[pixel].Blue;
		UWord G = rgbaLineBuffer[pixel].Green;
		UWord R = rgbaLineBuffer[pixel].Red;
		*rgbLineBuffer++ = R;
		*rgbLineBuffer++ = G;
		*rgbLineBuffer++ = B;
	}
}

// Convert 8 Bit ARGB to 8 bit BGR
void ConvertARGBToBGR (const UByte * pInRGBALineBuffer, UByte * pOutRGBLineBuffer, const ULWord inNumPixels)
{
	for (ULWord pixel = 0;  pixel < inNumPixels * 4;  pixel += 4)
	{
		UByte B = pInRGBALineBuffer[pixel];
		UByte G = pInRGBALineBuffer[pixel+1];
		UByte R = pInRGBALineBuffer[pixel+2];
		*pOutRGBLineBuffer++ = B;
		*pOutRGBLineBuffer++ = G;
		*pOutRGBLineBuffer++ = R;
		
	}
}

// Pack 10 Bit RGBA to 10 Bit RGB Format for our board
void PackRGB10BitFor10BitRGB(RGBAlpha10BitPixel* rgba10BitBuffer,ULWord numPixels)
{
	ULWord* outputBuffer = (ULWord*)rgba10BitBuffer;
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		UWord Red = rgba10BitBuffer[pixel].Red;
		UWord Green = rgba10BitBuffer[pixel].Green;
		UWord Blue = rgba10BitBuffer[pixel].Blue;
		outputBuffer[pixel] = (Blue<<20) + (Green<<10) + Red;
	}


}

// Pack 10 Bit RGBA to 10 Bit DPX Format for our board
void PackRGB10BitFor10BitDPX(RGBAlpha10BitPixel* rgba10BitBuffer,ULWord numPixels, bool bigEndian)
{
	ULWord* outputBuffer = (ULWord*)rgba10BitBuffer;
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		UWord Red = rgba10BitBuffer[pixel].Red;
		UWord Green = rgba10BitBuffer[pixel].Green;
		UWord Blue = rgba10BitBuffer[pixel].Blue;
		ULWord value = (Red<<22) + (Green<<12) + (Blue<<2);
		if ( bigEndian)
			outputBuffer[pixel] = ((value&0xFF)<<24) + (((value>>8)&0xFF)<<16) + (((value>>16)&0xFF)<<8) + ((value>>24)&0xFF);
		else
			outputBuffer[pixel] =value;
	}


}

// Pack 10 Bit RGBA to NTV2_FBF_10BIT_RGB_PACKED Format for our board
void PackRGB10BitFor10BitRGBPacked(RGBAlpha10BitPixel* rgba10BitBuffer,ULWord numPixels)
{
	ULWord* outputBuffer = (ULWord*)rgba10BitBuffer;
	for ( ULWord pixel=0;pixel<numPixels;pixel++)
	{
		UWord Red = rgba10BitBuffer[pixel].Red;
		UWord Green = rgba10BitBuffer[pixel].Green;
		UWord Blue = rgba10BitBuffer[pixel].Blue;
		ULWord value = (((Red>>2)&0xFF)<<16) + (((Green>>2)&0xFF)<<8) + ((Blue>>2)&0xFF);
		value |= ((Red&0x3)<<28) + ((Green&0x3)<<26) + ((Blue&0x3)<<24);

		outputBuffer[pixel] = value;

	}


}

/* KAM
// ConvertLineto16BitRGB
// 16 Bit Version
void ConvertLineto16BitRGB(UByte * ycbcrBuffer,
					  RGBAlpha16BitPixel * rgbaBuffer,
					  ULWord numPixels,
					  bool fUseSDMatrix)
{
	YCbCrAlphaPixel ycbcrPixel;
	UWord Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to RGBAlpha16BitPixels.
	// 2 RGBAlphaPixels at a time.
	Cb1 = *ycbcrBuffer++;
	Y1 = *ycbcrBuffer++;
	Cr1 = *ycbcrBuffer++;
	for ( ULWord count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (UByte)Cb1;
		ycbcrPixel.y = (UByte)Y1; 
		ycbcrPixel.cr = (UByte)Cr1; 
// warns that ycbcrPixel is used before intilialized.?
		if(fUseSDMatrix) {
			SDConvertYCbCrto16BitRGB(&ycbcrPixel,&rgbaBuffer[count]);
		} else {
			HDConvertYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count]);
		}
		// Read lone midde Y;
		ycbcrPixel.y = *ycbcrBuffer++;

		// Read Next full bandwidth sample
		Cb2 = *ycbcrBuffer++;
		Y2 = *ycbcrBuffer++;
		Cr2 = *ycbcrBuffer++;

		// Interpolate and write Inpterpolated RGBAlphaPixel
		ycbcrPixel.cb = (UByte)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (UByte)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			SDConvertYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
		} else {
			HDConvertYCbCrtoRGB(&ycbcrPixel,&rgbaBuffer[count+1]);
		}
		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;

	}
}
*/

// KAM - start
// ConvertLinetoRGB
// 10 Bit YCbCr 16 Bit RGB version
void ConvertLineto16BitRGB(UWord * ycbcrBuffer, 
					  RGBAlpha16BitPixel * rgbaBuffer,
					  ULWord numPixels,
					  bool fUseSDMatrix,
					  bool fUseSMPTERange)
{
	YCbCr10BitAlphaPixel ycbcrPixel = {0,0,0,0};
	UWord Cb1,Y1,Cr1,Cb2,Y2,Cr2;

	// take a line(CbYCrYCbYCrY....) to RGBAlpha16Pixels.
	// 2 RGBAlpha16Pixels at a time.
	Cb1 = *ycbcrBuffer++;
	Y1 = *ycbcrBuffer++;
	Cr1 = *ycbcrBuffer++;
	for ( ULWord count = 0; count < numPixels; count+=2 )
	{
		ycbcrPixel.cb = (UWord)Cb1;
		ycbcrPixel.y = (UWord)Y1; 
		ycbcrPixel.cr = (UWord)Cr1; 

		if(fUseSDMatrix) {
			if (fUseSMPTERange) {
				SDConvert10BitYCbCrto10BitRGBSmpte(&ycbcrPixel,(RGBAlpha10BitPixel *)&rgbaBuffer[count]);
			} else {
				SDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,(RGBAlpha10BitPixel *)&rgbaBuffer[count]);
			}
			rgbaBuffer[count].Red = (rgbaBuffer[count].Red) << 6;
			rgbaBuffer[count].Green = (rgbaBuffer[count].Green) << 6;
			rgbaBuffer[count].Blue = (rgbaBuffer[count].Blue) << 6;
		} else {
			if (fUseSMPTERange) {
				HDConvert10BitYCbCrto10BitRGBSmpte(&ycbcrPixel,(RGBAlpha10BitPixel *)&rgbaBuffer[count]);
			} else {
				HDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,(RGBAlpha10BitPixel *)&rgbaBuffer[count]);
			}
			rgbaBuffer[count].Red = (rgbaBuffer[count].Red) << 6;
			rgbaBuffer[count].Green = (rgbaBuffer[count].Green) << 6;
			rgbaBuffer[count].Blue = (rgbaBuffer[count].Blue) << 6;
		}

		// Read lone midde Y;
		ycbcrPixel.y = *ycbcrBuffer++;

		// Read Next full bandwidth sample
		// unless we are at the end of a line
		if ( (count + 2 ) >= numPixels )
		{
			Cb2 = (UWord)Cb1;
			Y2 = (UWord)Y1;
			Cr2 = (UWord)Cr1;
		}
		else
		{
			Cb2 = *ycbcrBuffer++;
			Y2 = *ycbcrBuffer++;
			Cr2 = *ycbcrBuffer++;
		}
		// Interpolate and write Inpterpolated RGBAlpha16Pixel
		ycbcrPixel.cb = (UWord)((Cb1+Cb2)/2);
		ycbcrPixel.cr = (UWord)((Cr1+Cr2)/2);
		if(fUseSDMatrix) {
			if (fUseSMPTERange) {
				SDConvert10BitYCbCrto10BitRGBSmpte(&ycbcrPixel,(RGBAlpha10BitPixel *)&rgbaBuffer[count+1]);
			} else {
				SDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,(RGBAlpha10BitPixel *)&rgbaBuffer[count+1]);
			}
			rgbaBuffer[count+1].Red = (rgbaBuffer[count+1].Red) << 6;
			rgbaBuffer[count+1].Green = (rgbaBuffer[count+1].Green) << 6;
			rgbaBuffer[count+1].Blue = (rgbaBuffer[count+1].Blue) << 6;
		} else {
			if (fUseSMPTERange) {
				HDConvert10BitYCbCrto10BitRGBSmpte(&ycbcrPixel,(RGBAlpha10BitPixel *)&rgbaBuffer[count+1]);
			} else {
				HDConvert10BitYCbCrto10BitRGB(&ycbcrPixel,(RGBAlpha10BitPixel *)&rgbaBuffer[count+1]);
			}
			rgbaBuffer[count+1].Red = (rgbaBuffer[count+1].Red) << 6;
			rgbaBuffer[count+1].Green = (rgbaBuffer[count+1].Green) << 6;
			rgbaBuffer[count+1].Blue = (rgbaBuffer[count+1].Blue) << 6;
		}

		// Setup for next loop
		Cb1 = Cb2;
		Cr1 = Cr2;
		Y1 = Y2;

	}
}
// KAM - end
