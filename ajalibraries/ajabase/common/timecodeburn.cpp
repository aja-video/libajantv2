/**
	@file		timecodeburn.cpp
	@copyright	Copyright (C) 2012-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJATimeCodeBurn class.
**/
#include "videoutilities.h"
#include "timecodeburn.h"
#include "timecodeburndata.h"
#include <string.h> //for memcpy
#include "stdio.h"

AJATimeCodeBurn::AJATimeCodeBurn(void)
{
	_bRendered = false;
	_pCharRenderMap = NULL;
}

AJATimeCodeBurn::~AJATimeCodeBurn(void)
{
	if (_pCharRenderMap != NULL)
	{
		delete []_pCharRenderMap;
		_pCharRenderMap = NULL;
	}
}

bool AJATimeCodeBurn::BurnTimeCode (char * pBaseVideoAddress, const char * pTimeCodeString , const uint32_t percentY)
{
	if ( !_bRendered )
		return false;

	uint32_t timeCodeLength = (uint32_t)strlen(pTimeCodeString);
	if ( (int)timeCodeLength > kTCMaxTCChars )
		return false;

	if(percentY == 0)
	{
		_charPositionY = (_charRenderHeight * 8) / 10;								// assume 80% of the way down the screen
	}
	else
	{
		_charPositionY = (_charRenderHeight * percentY) / 100;
	}

	// 10-bit YUV has to start on a 16-byte boundary in order to match the "cadence"
	if (_charRenderPixelFormat == AJA_PixelFormat_YCbCr10)
		_charPositionX &= ~0x0f;


	char *pFrameBuff = pBaseVideoAddress + (_charPositionY * _rowBytes) + _charPositionX;

	for ( uint32_t i=0; i < timeCodeLength; i++)
	{
		char currentChar = pTimeCodeString[i];
		uint32_t digitOffset;
		if ( currentChar >= '0' && currentChar <= '9' )
			digitOffset = currentChar - '0';
		else if ( currentChar == ':' )
			digitOffset = kTCDigColon;
		else if ( currentChar == ';' )
			digitOffset = kTCDigSemicolon;
		else digitOffset = kTCDigSpace;

		CopyDigit (digitOffset , pFrameBuff);
		pFrameBuff += _charWidthBytes;
	}

	return true;
}

void AJATimeCodeBurn::CopyDigit (int digitOffset,char *pFrameBuff)
{
	char *pDigit = (_pCharRenderMap + (digitOffset * _charWidthBytes * _charHeightLines));

	for (int y = 0; y < _charHeightLines; y++)
	{
		char *pSrc = (pDigit + (y * _charWidthBytes));
		char *pDst = (pFrameBuff + (y * _rowBytes));

		memcpy(pDst, pSrc, _charWidthBytes);
	}
}

bool AJATimeCodeBurn::RenderTimeCodeFont( AJA_PixelFormat pixelFormat, uint32_t numPixels, uint32_t numLines )
{
	bool bResult = true;

	// see if we've already rendered this format/size
	if (_bRendered && _pCharRenderMap != NULL && pixelFormat == _charRenderPixelFormat && (int)numLines == _charRenderHeight && (int)numPixels == _charRenderWidth)
	{
		return bResult;			// already rendered...
	}

	else
	{
		// these are the pixel formats we know how to do...
		bool bFormatOK = true;
		_rowBytes = AJA_CalcRowBytesForFormat(pixelFormat,numPixels);
		int bytesPerPixel;
		switch (pixelFormat)
		{
		case AJA_PixelFormat_YCbCr8:
			bytesPerPixel = 2;
			break;

		case AJA_PixelFormat_YUY28:
			bytesPerPixel = 2;
			break;

		case AJA_PixelFormat_ABGR8:
			bytesPerPixel = 4;
			break;

		case AJA_PixelFormat_ARGB8:
			bytesPerPixel = 4;
			break;

		case AJA_PixelFormat_RGBA8:
			bytesPerPixel = 4;
			break;

		case AJA_PixelFormat_YCbCr10:
			bytesPerPixel = 3;		// not really - we'll have to override this wherever "bytesPerPixel" is used below...
			break;

		case AJA_PixelFormat_RGB_DPX:
			bytesPerPixel = 4;
			break;

		case AJA_PixelFormat_RGB10:
			bytesPerPixel = 4;
			break;

		case AJA_PixelFormat_RGB8_PACK:
		case AJA_PixelFormat_BGR8_PACK:
			bytesPerPixel = 3;
			break;

		case AJA_PixelFormat_YCBCR10_420PL:
		case AJA_PixelFormat_YCBCR10_422PL:
			bytesPerPixel = 2;		// not really - we'll have to override this wherever "bytesPerPixel" is used below...
			break;

		case AJA_PixelFormat_YCBCR8_420PL:
		case AJA_PixelFormat_YCBCR8_422PL:
			bytesPerPixel = 1;
			break;
		
		default:
			bFormatOK = false;
			break;
		}

		if (bFormatOK)
		{
			// scale the characters based on the frame size they'll be used in
			int dotScale = 1;					// SD scale
			if (numLines > 900)
				dotScale = 3;					// HD 1080
			else if (numLines > 650)
				dotScale = 2;					// HD 720

			int dotWidth  = 1 * dotScale;		// pixels per "dot"
			int dotHeight = 2 * dotScale;		// frame lines per "dot"

			// exceptions: if this is DVCProHD or HDV, we're working with horizontally-scaled pixels. Tweak the dotWidth to compensate.
			if (numLines > 900 && numPixels <= 1440)
				dotWidth = 2;			// 1280x1080 or 1440x1080

			//			else if (numLines > 650 && numPixels < 1100)
			//				dotWidth = 1;			// 960x720

			int charWidthBytes  = kTCDigitDotWidth  * dotWidth * bytesPerPixel;
			if (pixelFormat == AJA_PixelFormat_YCbCr10)
				charWidthBytes  = (kTCDigitDotWidth * dotWidth * 16) / 6;		// note: assumes kDigitDotWidth is evenly divisible by 6!
			if ((pixelFormat == AJA_PixelFormat_YCBCR10_420PL) ||
				(pixelFormat == AJA_PixelFormat_YCBCR10_422PL))
				charWidthBytes  = (kTCDigitDotWidth * dotWidth * 5) / 4;		// note: assumes kDigitDotWidth is evenly divisible by 4!

			int charHeightLines = kTCDigitDotHeight * dotHeight;

			// if we had a previous render map, free it now
			if (_pCharRenderMap != NULL)
			{
				delete []_pCharRenderMap;
				_pCharRenderMap = NULL;
			}

			// malloc space for a new render map
			_pCharRenderMap = new char[(kTCMaxTCChars * charWidthBytes * charHeightLines)];
			if (_pCharRenderMap != NULL)
			{
				char *pRenderMap = _pCharRenderMap;

				// for each character...
				for (int c = 0; c < kTCMaxTCChars; c++)
				{
					// for each scan line...
					for (int y = 0; y < kTCDigitDotHeight; y++)
					{
						// each rendered line is duplicated N times
						for (int ydup = 0; ydup < dotHeight; ydup++)
						{
							// each dot...
							for (int x = 0; x < kTCDigitDotWidth; x++)
							{
								char dot = CharMap[c][y][x];

								if (pixelFormat == AJA_PixelFormat_YCbCr8)
								{
									char val = 0;
									switch (dot)
									{
									case 0:		val = (char)0x10;		break;
									case 1:		val = (char)0x4c;		break;
									case 2:		val = (char)0x86;		break;
									case 3:		val = (char)0xc0;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = (char)0x80;		// C
										*pRenderMap++ = val;		// Y
									}
								}

								else if (pixelFormat == AJA_PixelFormat_YUY28)
								{
									char val = 0;
									switch (dot)
									{
									case 0:		val = (char)0x10;		break;
									case 1:		val = (char)0x4c;		break;
									case 2:		val = (char)0x86;		break;
									case 3:		val = (char)0xc0;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = val;		// Y
										*pRenderMap++ = (char)0x80;		// C
									}
								}

								else if (pixelFormat == AJA_PixelFormat_YCbCr10)
								{
									int val = 0;
									switch (dot)
									{
									case 0:		val =  64;		break;
									case 1:		val = 356;		break;
									case 2:		val = 648;		break;
									case 3:		val = 940;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										writeV210Pixel (&pRenderMap, ((x * dotWidth) + xdup), 0x200, val);
									}
								}

								else if (pixelFormat == AJA_PixelFormat_ABGR8 || pixelFormat == AJA_PixelFormat_ARGB8 )
								{
									char val = 0;
									switch (dot)
									{
									case 0:		val = (char)0x00;		break;
									case 1:		val = (char)0x55;		break;
									case 2:		val = (char)0xaa;		break;
									case 3:		val = (char)0xff;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = 0;			// A
										*pRenderMap++ = val;		// R
										*pRenderMap++ = val;		// G
										*pRenderMap++ = val;		// B
									}
								}

								else if ( pixelFormat == AJA_PixelFormat_RGBA8 )
								{
									char val = 0;
									switch (dot)
									{
									case 0:		val = (char)0x00;		break;
									case 1:		val = (char)0x55;		break;
									case 2:		val = (char)0xaa;		break;
									case 3:		val = (char)0xff;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = val;		// R
										*pRenderMap++ = val;		// G
										*pRenderMap++ = val;		// B
										*pRenderMap++ = 0;			// A
									}
								}

								else if ( pixelFormat == AJA_PixelFormat_RGB8_PACK || pixelFormat == AJA_PixelFormat_BGR8_PACK )
								{
									char val = 0;
									switch (dot)
									{
									case 0:		val = (char)0x00;		break;
									case 1:		val = (char)0x55;		break;
									case 2:		val = (char)0xaa;		break;
									case 3:		val = (char)0xff;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = val;		// R
										*pRenderMap++ = val;		// G
										*pRenderMap++ = val;		// B
									}
								}

								else if (pixelFormat == AJA_PixelFormat_RGB_DPX)
								{
									int val = 0;
									switch (dot)
									{
									case 0:		val =  64;		break;	// 0x40
									case 1:		val = 356;		break;	// 0x164
									case 2:		val = 648;		break;	// 0x288
									case 3:		val = 940;		break;	// 0x3ac
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ =                        ((val & 0x3fc) >> 2);
										*pRenderMap++ = ((val & 0x003) << 6) | ((val & 0x3f0) >> 4);
										*pRenderMap++ = ((val & 0x00f) << 4) | ((val & 0x3c0) >> 6);
										*pRenderMap++ = ((val & 0x03f) << 2);
									}
								}
								else if (pixelFormat == AJA_PixelFormat_RGB10)
								{
									int val = 0;
									switch (dot)
									{
									case 0:		val = 0x000;		break;
									case 1:		val = 0x154;		break;
									case 2:		val = 0x2a8;		break;
									case 3:		val = 0x3ff;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ =                         ((val & 0x0ff));
										*pRenderMap++ = ((val & 0x03f) << 2) |  ((val & 0x300) >> 8);
										*pRenderMap++ = ((val & 0x00f) << 4) |  ((val & 0x3c0) >> 6);
										*pRenderMap++ = ((val & 0x3f0) >> 4) | 0x300;
									}
								}

								else if ((pixelFormat == AJA_PixelFormat_YCBCR10_420PL) ||
										 (pixelFormat == AJA_PixelFormat_YCBCR10_422PL))
								{
									int val = 0;
									switch (dot)
									{
									case 0:		val =  64;		break;
									case 1:		val = 356;		break;
									case 2:		val = 648;		break;
									case 3:		val = 940;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										writeYCbCr10PackedPlanerPixel (&pRenderMap, ((x * dotWidth) + xdup), val);
									}
								}

								else if ((pixelFormat == AJA_PixelFormat_YCBCR8_420PL) ||
										 (pixelFormat == AJA_PixelFormat_YCBCR8_422PL))
								{
									char val = 0;
									switch (dot)
									{
									case 0:		val = (char)0x10;		break;
									case 1:		val = (char)0x4c;		break;
									case 2:		val = (char)0x86;		break;
									case 3:		val = (char)0xc0;		break;
									}

									// each rendered pixel is duplicated N times
									for (int xdup = 0; xdup < dotWidth; xdup++)
									{
										*pRenderMap++ = val;		// Y
									}
								}

							}
						}
					}
				}

				_bRendered = true;
				_charRenderPixelFormat    = pixelFormat;
				_charRenderHeight = numLines;
				_charRenderWidth  = numPixels;

				// character sizes
				_charWidthBytes   = charWidthBytes;
				_charHeightLines  = charHeightLines;

				// burn-in offset
				int byteWidth = (numPixels * bytesPerPixel);
				if (pixelFormat == AJA_PixelFormat_YCbCr10)
					byteWidth  = (numPixels * 16) / 6;		// in 10-bit YUV, 6 pixels = 16 bytes
				if ((pixelFormat == AJA_PixelFormat_YCBCR10_420PL) ||
					(pixelFormat == AJA_PixelFormat_YCBCR10_422PL))
					byteWidth  = (numPixels * 5) / 4;		// in 10-bit YUV packed planer, 4 pixels = 5 bytes

				_charPositionX = (byteWidth - (kTCNumBurnInChars * charWidthBytes)) / 2;		// assume centered
			}
		}

		else	// we don't know how to do this pixel format...
			bResult = false;
	}

	return bResult;
}


// write a single chroma/luma pair of components
// note that it's up to the caller to make sure that the chroma is the correct Cb or Cr,
// and is properly timed with adjacent pixels!
void AJATimeCodeBurn::writeV210Pixel (char **pBytePtr, int x, int c, int y)
{
	char *p = *pBytePtr;

	// the components in each v210 6-pixel block are laid out like this (note that the UInt32s are swixelled!):
	//
	// Addr: |  3    2    1    0  |  7    6    5    4  | 11   10    9    8  | 15   14   13   12 |
	//       { 00 Cr0   Y0   Cb0    00 Y2   Cb2    Y1    00 Cb4   Y3   Cr2    00 Y5   Cr4   Y4  }
	//
	int cadence = x % 3;

	switch (cadence)
	{
	case 0:		// Cb0/Y0 or Cr2/Y3: we assume that p points to byte 0/8. When we are finished, it will still point to byte 0/8
		p[0] =      c & 0x0FF;									//  c<7:0>
		p[1] = ((   y & 0x03F) << 2) + ((   c & 0x300) >> 8);	//  y<5:0> +  c<9:8>
		p[2] =  (p[2] & 0x0F0)       + ((   y & 0x3C0) >> 6);	// (merge) +  y<9:6>
		break;

	case 1:		// Cr0/Y1 or Cb4/Y4: we assume that p points to byte 0/8. When we are finished, it will point to byte 4/12
		p[2] = ((   c & 0x00F) << 4) +  (p[2] & 0x00F);			//  c<3:0> + (merge)
		p[3] =                         ((   c & 0x3F0) >> 4);	//   '00'  +  c<9:4>
		p[4] =      y & 0x0FF;									//  y<7:0>
		p[5] =  (p[5] & 0x0FC)       + ((   y & 0x300) >> 8);	// (merge) +  y<9:8>
		*pBytePtr += 4;
		break;

	case 2:		// Cb2/Y2 or Cr4/Y5: we assume that p points to byte 4/12. When we are finished, it will point to byte 8/16
		p[1] = ((   c & 0x03F) << 2) +  (p[1] & 0x003);			//  c<5:0> + (merge)
		p[2] = ((   y & 0x00F) << 4) + ((   c & 0x3C0) >> 6);	//  y<3:0> +  c<9:6>
		p[3] =                         ((   y & 0x3F0) >> 4);	//   '00'  +  y<9:4>
		*pBytePtr += 4;
		break;
	}
}


// write a single luma components
void AJATimeCodeBurn::writeYCbCr10PackedPlanerPixel (char **pBytePtr, int x, int y)
{
	char *p = *pBytePtr;

	int cadence = x % 4;

	switch (cadence)
	{
	case 0:
		p[0] = (y & 0x0FF) << 0;;
		p[1] = (y & 0x300) >> 8;
		*pBytePtr += 1;
		break;
	case 1:
		p[0] |= (y & 0x03F) << 2;
		p[1] = (y & 0x3C0) >> 6;
		*pBytePtr += 1;
		break;
	case 2:
		p[0] |= (y & 0x00F) << 4;
		p[1] = (y & 0x3F0) >> 4;
		*pBytePtr += 1;
		break;
	case 3:
		p[0] |= (y & 0x003) << 6;
		p[1] = (y & 0x3FC) >> 2;
		*pBytePtr += 2;
		break;
	}
}
