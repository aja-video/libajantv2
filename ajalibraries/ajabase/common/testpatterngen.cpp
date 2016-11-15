/**
	@file		testpatterngen.cpp
	@copyright	Copyright (C) 2010-2015 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJATestPatternGen class.
**/

#include "types.h"
#include "videoutilities.h"
#include "testpatterngen.h"
#include "testpatterndata.h"
#include "math.h"

#ifdef AJA_LINUX
#include <string.h> //for memcpy
#endif


uint32_t MakeSineWaveVideo(double radians, bool bChroma,double Gain);

//*********************************************************************************

// CTestPattern

AJATestPatternGen::AJATestPatternGen() :
	_sliderValue(DEFAULT_PATT_GAIN),
	_signalMask(AJA_SIGNALMASK_ALL)
{
	_bayerPhase = AJA_BayerColorPhase_RedGreen;
}


AJATestPatternGen::~AJATestPatternGen()
{

}


// DrawTestPattern()
//	Note: "dSlider" is expected to range between 0.0 and 1.0
//
bool AJATestPatternGen::DrawTestPattern( AJATestPatternSelect pattNum, uint32_t frameWidth, uint32_t frameHeight, AJA_PixelFormat pixelFormat, AJATestPatternBuffer &testPatternBuffer )
{
	bool bResult = false;

	// Save this away for worker methods.
	_patternNumber = pattNum;
	_frameWidth  = frameWidth;
	_frameHeight = frameHeight;
	_pixelFormat = pixelFormat;

	_linePitch     = AJA_CalcRowBytesForFormat(_pixelFormat, _frameWidth);		// number of BYTES per line of frame buffer format
	_dataLinePitch = AJA_CalcRowBytesForFormat(AJA_PixelFormat_YCbCr10, _frameWidth);			// number of BYTES per line of test pattern data (always stored as 10-bit YCbCr)
	_bufferSize = _linePitch*_frameHeight;
	if(_bufferSize == 0)
		return bResult;

	if ( testPatternBuffer.size() != _bufferSize )
		testPatternBuffer.resize(_bufferSize);

	_pTestPatternBuffer = &testPatternBuffer[0];

	_pPackedLineBuffer = new uint32_t[_frameWidth*2];
	_pUnPackedLineBuffer = new uint16_t[(_frameWidth+1)*2]; // add 1 for 720p
	// Fill Unpacked buffer with Black.
	AJA_MakeUnPacked10BitYCbCrBuffer( _pUnPackedLineBuffer, CCIR601_10BIT_BLACK , CCIR601_10BIT_CHROMAOFFSET , CCIR601_10BIT_CHROMAOFFSET ,_frameWidth );

	switch (pattNum)
	{
		case AJA_TestPatt_ColorBars100:
		case AJA_TestPatt_ColorBars75:
		case AJA_TestPatt_Ramp:
		case AJA_TestPatt_MultiBurst:
		case AJA_TestPatt_LineSweep:
		case AJA_TestPatt_CheckField:
		case AJA_TestPatt_FlatField:
		case AJA_TestPatt_MultiPattern:
			bResult = DrawSegmentedTestPattern();
			break;

		case AJA_TestPatt_Black:
			bResult = DrawYCbCrFrame(CCIR601_10BIT_BLACK,CCIR601_10BIT_CHROMAOFFSET,CCIR601_10BIT_CHROMAOFFSET);
			break;
		case AJA_TestPatt_White:
			bResult = DrawYCbCrFrame(CCIR601_10BIT_WHITE,CCIR601_10BIT_CHROMAOFFSET,CCIR601_10BIT_CHROMAOFFSET);
			break;
		case AJA_TestPatt_Border:
			bResult = DrawBorderFrame();
			break;
		case AJA_TestPatt_LinearRamp:
			DrawLinearRampFrame();
			bResult = true;
			break;
		case AJA_TestPatt_SlantRamp:
			DrawSlantRampFrame();
			bResult = true;
			break;
		case AJA_TestPatt_ZonePlate:
			DrawZonePlateFrame();
			bResult = true;
			break;
		case AJA_TestPatt_ColorQuadrant:
			DrawColorQuandrantFrame();
			bResult = true;
			break;
		case AJA_TestPatt_ColorQuadrantBorder:
			DrawQuandrantBorderFrame();
			bResult = true;
			break;
		default:	// unknown test pattern ID?
			break;
	}

	delete [] _pPackedLineBuffer;
	delete [] _pUnPackedLineBuffer;

	return bResult;
}


bool AJATestPatternGen::DrawTestPattern(AJATestPatternSelect pattNum, uint32_t frameWidth, uint32_t frameHeight, 
										AJA_PixelFormat pixelFormat, AJA_BayerColorPhase phase, AJATestPatternBuffer &testPatternBuffer )
{
	_bayerPhase = phase;
	return DrawTestPattern(pattNum, frameWidth, frameHeight, pixelFormat, testPatternBuffer);
}


bool AJATestPatternGen::DrawSegmentedTestPattern()
{
	bool bResult = true;
	bool b4K = false;

		// which video standard are we?
	int standard;  // this standard is used to index the SegmentTestPatternData
	if		(_frameWidth == 1920 && _frameHeight == 1080)
		standard = 0;			// aka "NTV2_STANDARD_1080"

	else if (_frameWidth == 2048 && _frameHeight == 1080)
		standard = 0;			// aka "NTV2_STANDARD_1080"

	else if (_frameWidth == 1280 && _frameHeight == 720)
		standard = 1;			// aka "NTV2_STANDARD_720"

	else if (_frameWidth == 720 && _frameHeight == 486)
		standard = 2;			// aka "NTV2_STANDARD_525"

	else if (_frameWidth == 720 && _frameHeight == 576)
		standard = 3;			// aka "NTV2_STANDARD_625"

	else if (_frameWidth == 2048 && _frameHeight == 1556)
		standard = 5;			// aka "NTV2_STANDARD2_K"

	else if (_frameWidth == 3840 && _frameHeight == 2160)
	{
		//kludge for now.
		standard = 0;			// aka "NTV2_STANDARD_1080"
		b4K = true;
	}
	else if (_frameWidth == 4096 && _frameHeight == 2160)
	{
		//kludge for now.
		standard = 0;			// aka "NTV2_STANDARD_1080"
		b4K = true;
	}

	else
		return false;


		// find the appropriate test pattern descriptor
	SegmentTestPatternData *pTestPatternSegmentData = &NTV2TestPatternSegments[_patternNumber];

		// walk through the segments
	for (int segmentCount = 0; segmentCount < NumTestPatternSegments; segmentCount++ )
	{
		SegmentDescriptor* segmentDescriptor = &pTestPatternSegmentData->segmentDescriptor[standard][segmentCount];
		uint32_t* data = segmentDescriptor->data;
		if ( data != NULL )
		{

			// copy the test pattern line to the local "_pPackedLineBuffer"
			memcpy(_pPackedLineBuffer, data, _dataLinePitch);

			if (_frameWidth == HD_NUMCOMPONENTPIXELS_2K || _frameWidth == (HD_NUMCOMPONENTPIXELS_2K*2) )
			{
				AJA_UnPack10BitYCbCrBuffer(_pPackedLineBuffer, _pUnPackedLineBuffer, HD_NUMCOMPONENTPIXELS_1080);
				AJA_ReSampleYCbCrSampleLine((int16_t*)_pUnPackedLineBuffer, (int16_t*)_pUnPackedLineBuffer, 1920, 2048);
			}
			else
				AJA_UnPack10BitYCbCrBuffer(_pPackedLineBuffer, _pUnPackedLineBuffer, _frameWidth);

			int startLine = segmentDescriptor->startLine;
			int numLines  = (segmentDescriptor->endLine - startLine) + 1;
			if ( b4K )
			{
				// total kludge....just stretch out 1080 pattern.
				startLine *= 2;
				numLines *= 2;
				
				// stretch line by copying pixels.
				uint16_t* pLineSrc  = &_pUnPackedLineBuffer[_frameWidth-1];
				uint16_t* pLineDest = &_pUnPackedLineBuffer[_frameWidth*2-1];
				for ( uint32_t count = 0; count < _frameWidth/4; count ++)
				{
					uint16_t y2  = *pLineSrc--;
					uint16_t cr1 = *pLineSrc--;
					uint16_t y1  = *pLineSrc--;
					uint16_t cb1 = *pLineSrc--;
					*pLineDest-- = y2;	
					*pLineDest-- = cr1; 
					*pLineDest-- = y1;
					*pLineDest-- = cb1;
					*pLineDest-- = y2;
					*pLineDest-- = cr1;
					*pLineDest-- = y1;
					*pLineDest-- = cb1;
						
				}
			}

			// go through hoops to mask out undesired components
			if (_patternNumber == AJA_TestPatt_MultiBurst || _patternNumber == AJA_TestPatt_LineSweep)
			{
				_signalMask = AJA_SIGNALMASK_Y;		// just assume that Multiburst and LineSweep are "Y Only"
				AJA_MaskUnPacked10BitYCbCrBuffer(_pUnPackedLineBuffer, (uint16_t)_signalMask , _frameWidth);
			}

				// now RE-pack, according to the desired pixel format
			AJA_ConvertUnpacked10BitYCbCrToPixelFormat(_pUnPackedLineBuffer, _pPackedLineBuffer, _frameWidth, _pixelFormat);


				// copy and repeat for as many lines as called for in segment
			for (int line = 0; line < numLines; line++)
			{

				int currentLine = startLine+line;
				WriteLineToBuffer( _pixelFormat, _bayerPhase, currentLine,_frameWidth, _linePitch, _pTestPatternBuffer,_pPackedLineBuffer); 

			}
		}
	}

	return bResult;
}

bool AJATestPatternGen::DrawYCbCrFrame(uint16_t Y, uint16_t Cb, uint16_t Cr)
{
	// Make a BlackLine
	AJA_MakeUnPacked10BitYCbCrBuffer( _pUnPackedLineBuffer, Y , Cb , Cr ,_frameWidth );
	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(_pUnPackedLineBuffer, _pPackedLineBuffer, _frameWidth, _pixelFormat);

	for (uint32_t line = 0; line < _frameHeight; line++)
	{
		WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,_pPackedLineBuffer); 
#if 0
		memcpy(_pTestPatternBuffer, _pPackedLineBuffer, _linePitch);
		_pTestPatternBuffer += _linePitch;
#endif
	}

	return true;
}

bool AJATestPatternGen::DrawLinearRampFrame()
{

	// Ramp from 0x40-0x3AC
	uint16_t value = 0x40;
	for ( uint16_t pixel = 0; pixel < _frameWidth; pixel++ )
	{
		_pUnPackedLineBuffer[pixel*2] = value;
		_pUnPackedLineBuffer[pixel*2+1] = value;
		value++;
		if ( value > 0x3AC )
			value = 0x40;
	}
	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(_pUnPackedLineBuffer, _pPackedLineBuffer,_frameWidth,_pixelFormat);

	for ( uint32_t line = 0; line < _frameHeight ; line++ )
	{
		WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,_pPackedLineBuffer); 

	}
	return true;

}


bool AJATestPatternGen::DrawSlantRampFrame()
{
	// Ramp from 0x40-0x3AC
	for ( uint32_t line = 0; line < _frameHeight; line++ )
	{
		uint16_t value = (line%(0x3AC-0x40))+0x40;

		for ( uint16_t pixel = 0; pixel < _frameWidth; pixel++ )
		{
			_pUnPackedLineBuffer[pixel*2] = value;
			_pUnPackedLineBuffer[pixel*2+1] = value;
			value++;
			if ( value > 0x3AC )
				value = 0x40;
		}
		AJA_ConvertUnpacked10BitYCbCrToPixelFormat(_pUnPackedLineBuffer, _pPackedLineBuffer,_frameWidth,_pixelFormat);
		WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,_pPackedLineBuffer); 

	}

	return true;
}

bool AJATestPatternGen::DrawBorderFrame()
{
	uint32_t* pPackedWhiteLineBuffer= new uint32_t[_frameWidth*2];
	uint32_t* pPackedEdgeLineBuffer= new uint32_t[_frameWidth*2];
	uint16_t* pUnPackedEdgeBuffer= new uint16_t[_frameWidth*2];
	uint16_t* pUnPackedWhiteBuffer= new uint16_t[_frameWidth*2];

	AJA_MakeUnPacked10BitYCbCrBuffer(pUnPackedEdgeBuffer,CCIR601_10BIT_BLACK,CCIR601_10BIT_CHROMAOFFSET,CCIR601_10BIT_CHROMAOFFSET,_frameWidth);
	AJA_MakeUnPacked10BitYCbCrBuffer(pUnPackedWhiteBuffer,CCIR601_10BIT_WHITE,CCIR601_10BIT_CHROMAOFFSET,CCIR601_10BIT_CHROMAOFFSET,_frameWidth);

	// Put in White Edge.
	pUnPackedEdgeBuffer[0] = CCIR601_10BIT_CHROMAOFFSET;
	pUnPackedEdgeBuffer[+1] = CCIR601_10BIT_WHITE;
	pUnPackedEdgeBuffer[+2] = CCIR601_10BIT_CHROMAOFFSET;
	pUnPackedEdgeBuffer[(_frameWidth)*2-1] = CCIR601_10BIT_WHITE;
	pUnPackedEdgeBuffer[(_frameWidth)*2-2] = CCIR601_10BIT_CHROMAOFFSET;

	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(pUnPackedWhiteBuffer, pPackedWhiteLineBuffer,_frameWidth,_pixelFormat);
	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(pUnPackedEdgeBuffer, pPackedEdgeLineBuffer,_frameWidth,_pixelFormat);

	for ( uint32_t line = 0; line < _frameHeight; line++ )
	{
		if ( line == 0 || line == (_frameHeight-1))
			::memcpy(_pTestPatternBuffer,pPackedWhiteLineBuffer,_linePitch);
		else
			::memcpy(_pTestPatternBuffer,pPackedEdgeLineBuffer,_linePitch);

		_pTestPatternBuffer += _linePitch;
	}

	delete [] pPackedWhiteLineBuffer;
	delete [] pPackedEdgeLineBuffer;
	delete [] pUnPackedEdgeBuffer;
	delete [] pUnPackedWhiteBuffer;

	return true;
}

const double kPi = 3.1415926535898;
bool AJATestPatternGen::DrawZonePlateFrame()
{

	double pattScale = (kPi*.5 ) / (_frameWidth + 1);

	for ( uint32_t line = 0; line < _frameHeight; line++ )
	{

		for ( uint16_t pixel = 0; pixel < _frameWidth; pixel++ )
		{
			double xDist = (double)pixel - ((double)_frameWidth  / 2.0);
			double yDist = (double)line - ((double)_frameHeight / 2.0);
			double r = ((xDist * xDist) + (yDist * yDist)) * pattScale;

			_pUnPackedLineBuffer[pixel*2+1] = MakeSineWaveVideo(r, false,_sliderValue);
			_pUnPackedLineBuffer[pixel*2] = MakeSineWaveVideo(r, true,_sliderValue);

		}
		AJA_ConvertUnpacked10BitYCbCrToPixelFormat(_pUnPackedLineBuffer, _pPackedLineBuffer,_frameWidth,_pixelFormat);
		WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,_pPackedLineBuffer); 

	}

	return true;

}



bool AJATestPatternGen::DrawColorQuandrantFrame()
{
	uint32_t* pPackedUpperLineBuffer= new uint32_t[_frameWidth*2];
	uint16_t* pUnPackedUpperLineBuffer= new uint16_t[_frameWidth*2];
	uint32_t* pPackedLowerLineBuffer= new uint32_t[_frameWidth*2];
	uint16_t* pUnPackedLowerLineBuffer= new uint16_t[_frameWidth*2];

	// Colors for the quadrants are from SMPTE 435-1-2009 section 6.4.2
	static const unsigned char fullRange = 235;
	static const unsigned char midRange  = 187;
	static const unsigned char lowRange  = 140;
	AJA_RGBAlphaPixel rgbaPixel;
	AJA_YCbCr10BitPixel yCbCrPixel;
	rgbaPixel.Alpha = 0;  // Upper left - yellow
	rgbaPixel.Red = fullRange;
	rgbaPixel.Green = fullRange;
	rgbaPixel.Blue = lowRange;
	AJA_HDConvertRGBAlphatoYCbCr(&rgbaPixel, &yCbCrPixel);
	AJA_MakeUnPacked10BitYCbCrBuffer(pUnPackedUpperLineBuffer,yCbCrPixel.y,yCbCrPixel.cb,yCbCrPixel.cr,_frameWidth/2);
	rgbaPixel.Alpha = 0;  // Upper right - blue
	rgbaPixel.Red = midRange;
	rgbaPixel.Green = fullRange;
	rgbaPixel.Blue = fullRange;
	AJA_HDConvertRGBAlphatoYCbCr(&rgbaPixel, &yCbCrPixel);
	AJA_MakeUnPacked10BitYCbCrBuffer(&pUnPackedUpperLineBuffer[_frameWidth],yCbCrPixel.y,yCbCrPixel.cb,yCbCrPixel.cr,_frameWidth/2);
	rgbaPixel.Alpha = 0;  // Lower left - green
	rgbaPixel.Red = lowRange;
	rgbaPixel.Green = fullRange;
	rgbaPixel.Blue = lowRange;
	AJA_HDConvertRGBAlphatoYCbCr(&rgbaPixel, &yCbCrPixel);
	AJA_MakeUnPacked10BitYCbCrBuffer(pUnPackedLowerLineBuffer,yCbCrPixel.y,yCbCrPixel.cb,yCbCrPixel.cr,_frameWidth/2);
	rgbaPixel.Alpha = 0;  // Lower right - pink
	rgbaPixel.Red = fullRange;
	rgbaPixel.Green = lowRange;
	rgbaPixel.Blue = midRange;
	AJA_HDConvertRGBAlphatoYCbCr(&rgbaPixel, &yCbCrPixel);
	AJA_MakeUnPacked10BitYCbCrBuffer(&pUnPackedLowerLineBuffer[_frameWidth],yCbCrPixel.y,yCbCrPixel.cb,yCbCrPixel.cr,_frameWidth/2);

	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(pUnPackedUpperLineBuffer, pPackedUpperLineBuffer,_frameWidth,_pixelFormat);
	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(pUnPackedLowerLineBuffer, pPackedLowerLineBuffer,_frameWidth,_pixelFormat);

	uint32_t line;
	for ( line = 0; line < _frameHeight/2; line++ )
	{
		WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,pPackedUpperLineBuffer); 
	}
	for ( line = 0; line < _frameHeight/2; line++ )
	{
		WriteLineToBuffer( _pixelFormat, _bayerPhase, line + (_frameHeight/2),_frameWidth, _linePitch, _pTestPatternBuffer,pPackedLowerLineBuffer); 
	}

	delete [] pUnPackedUpperLineBuffer;
	delete [] pPackedUpperLineBuffer;
	delete [] pUnPackedLowerLineBuffer;
	delete [] pPackedLowerLineBuffer;

	return true;

}
bool AJATestPatternGen::DrawQuandrantBorderFrame()
{
	uint32_t* pPackedRedLineBuffer= new uint32_t[_frameWidth*2];
	uint16_t* pUnPackedRedLineBuffer= new uint16_t[_frameWidth*2];
	uint32_t* pPackedBlueLineBuffer= new uint32_t[_frameWidth*2];
	uint16_t* pUnPackedBlueLineBuffer= new uint16_t[_frameWidth*2];
	uint32_t* pPackedMagentaGreenLineBuffer= new uint32_t[_frameWidth*2];
	uint16_t* pUnPackedMagentaGreenLineBuffer= new uint16_t[_frameWidth*2];

	AJA_RGBAlphaPixel rgbaRedPixel;
	rgbaRedPixel.Alpha = 0;
	rgbaRedPixel.Red = 0xFF;
	rgbaRedPixel.Green = 0;
	rgbaRedPixel.Blue = 0;
	AJA_RGBAlphaPixel rgbaGreenPixel;
	rgbaGreenPixel.Alpha = 0;
	rgbaGreenPixel.Red = 0;
	rgbaGreenPixel.Green = 0xFF;
	rgbaGreenPixel.Blue = 0;
	AJA_RGBAlphaPixel rgbaBluePixel;
	rgbaBluePixel.Alpha = 0;
	rgbaBluePixel.Red = 0;
	rgbaBluePixel.Green = 0;
	rgbaBluePixel.Blue = 0xFF;
	AJA_RGBAlphaPixel rgbaMagentaPixel;
	rgbaMagentaPixel.Alpha = 0;
	rgbaMagentaPixel.Red = 0xFF;
	rgbaMagentaPixel.Green = 0;
	rgbaMagentaPixel.Blue = 0xFF;
	AJA_YCbCr10BitPixel yCbCrRedPixel;
	AJA_HDConvertRGBAlphatoYCbCr(&rgbaRedPixel, &yCbCrRedPixel);
	AJA_YCbCr10BitPixel yCbCrGreenPixel;
	AJA_HDConvertRGBAlphatoYCbCr(&rgbaGreenPixel, &yCbCrGreenPixel);
	AJA_YCbCr10BitPixel yCbCrBluePixel;
	AJA_HDConvertRGBAlphatoYCbCr(&rgbaBluePixel, &yCbCrBluePixel);
	AJA_YCbCr10BitPixel yCbCrMagentaPixel;
	AJA_HDConvertRGBAlphatoYCbCr(&rgbaMagentaPixel, &yCbCrMagentaPixel);

	AJA_MakeUnPacked10BitYCbCrBuffer(pUnPackedRedLineBuffer,yCbCrRedPixel.y,yCbCrRedPixel.cb,yCbCrRedPixel.cr,_frameWidth);

	AJA_MakeUnPacked10BitYCbCrBuffer(pUnPackedBlueLineBuffer,yCbCrBluePixel.y,yCbCrBluePixel.cb,yCbCrBluePixel.cr,_frameWidth);
	AJA_MakeUnPacked10BitYCbCrBuffer(pUnPackedMagentaGreenLineBuffer,0x40,0x200,0x200,_frameWidth);
	pUnPackedMagentaGreenLineBuffer[0] = yCbCrMagentaPixel.cb;
	pUnPackedMagentaGreenLineBuffer[1] = yCbCrMagentaPixel.y;
	pUnPackedMagentaGreenLineBuffer[2] = yCbCrMagentaPixel.cr;

	pUnPackedMagentaGreenLineBuffer[_frameWidth-1] = yCbCrGreenPixel.y;
	pUnPackedMagentaGreenLineBuffer[_frameWidth-4] = yCbCrGreenPixel.cb;
	pUnPackedMagentaGreenLineBuffer[_frameWidth-3] = yCbCrGreenPixel.y;
	pUnPackedMagentaGreenLineBuffer[_frameWidth-2] = yCbCrGreenPixel.cr;

	pUnPackedMagentaGreenLineBuffer[_frameWidth] = yCbCrMagentaPixel.cb;
	pUnPackedMagentaGreenLineBuffer[_frameWidth+1] = yCbCrMagentaPixel.y;
	pUnPackedMagentaGreenLineBuffer[_frameWidth+2] = yCbCrMagentaPixel.cr;

	pUnPackedMagentaGreenLineBuffer[_frameWidth*2-1] = yCbCrGreenPixel.y;
	pUnPackedMagentaGreenLineBuffer[_frameWidth*2-4] = yCbCrGreenPixel.cb;
	pUnPackedMagentaGreenLineBuffer[_frameWidth*2-3] = yCbCrGreenPixel.y;
	pUnPackedMagentaGreenLineBuffer[_frameWidth*2-2] = yCbCrGreenPixel.cr;


	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(pUnPackedRedLineBuffer, pPackedRedLineBuffer,_frameWidth,_pixelFormat);
	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(pUnPackedBlueLineBuffer, pPackedBlueLineBuffer,_frameWidth,_pixelFormat);
	AJA_ConvertUnpacked10BitYCbCrToPixelFormat(pUnPackedMagentaGreenLineBuffer, pPackedMagentaGreenLineBuffer,_frameWidth,_pixelFormat);

	uint32_t line;
	for ( line = 0; line < _frameHeight; line++ )
	{
		if ( line == 0 )
		{
			WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,pPackedRedLineBuffer); 
		}
		else if ( line == _frameHeight/2 )
		{
			WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,pPackedBlueLineBuffer); 
		}
		else if ( line == (_frameHeight-1) )
		{
			WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,pPackedRedLineBuffer); 
		}
		else
		{
			WriteLineToBuffer( _pixelFormat, _bayerPhase, line,_frameWidth, _linePitch, _pTestPatternBuffer,pPackedMagentaGreenLineBuffer); 
		}
	}

	delete [] pPackedRedLineBuffer;
	delete [] pUnPackedRedLineBuffer;
	delete [] pPackedBlueLineBuffer;
	delete [] pUnPackedBlueLineBuffer;
	delete [] pPackedMagentaGreenLineBuffer;
	delete [] pUnPackedMagentaGreenLineBuffer;

	return true;
}



// 10-bit YUV values
const int kYUVBlack10	   =  64;
const int kYUVWhite10	   = 940;
//const int kYUVMidGray10	   = (kYUVWhite10 + kYUVBlack10) / 2;

const int kYUVMinChroma10  =  64;
const int kYUVMaxChroma10  = 960;
//const int kYUVZeroChroma10 = 512;

uint32_t MakeSineWaveVideo(double radians, bool bChroma, double Gain)
{
	int result;


	double Scale;
	double Offset;

	if (!bChroma)
	{
		Scale  = ((float)kYUVWhite10 - (float)kYUVBlack10) / 2.0;
		Offset = ((float)kYUVWhite10 + (float)kYUVBlack10) / 2.0;

		// calculate -cosine value to start Y at minimum value
		result = (int)((sin(radians) * Scale * Gain) + Offset + 0.5);	// convert to 10-bit luma video levels
	}
	else
	{
		Scale  = ((float)kYUVMaxChroma10 - (float)kYUVMinChroma10) / 2.0;
		Offset = ((float)kYUVMaxChroma10 + (float)kYUVMinChroma10) / 2.0;

		// calculate sine value to start C at "zero" value
		result = (int)((sin(radians) * Scale * Gain) + Offset + 0.5);	// convert to 10-bit chroma video levels
	}

	return result;
}
 


//***********************************************************************************************************



