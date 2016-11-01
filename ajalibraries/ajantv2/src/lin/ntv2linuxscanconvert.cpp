/////////////////////////////////////////////////////////////////////////////
// CNTV2ScanConvert.cpp 
// 
// Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//
// General Purpose Class to convert graphics to video to download to 
// HD-NTV or SD-NTV Card. This class derives from CNTV2Card. It also has a routine
// to download test patterns to the board.

#include "ntv2linuxscanconvert.h"
#include "ntv2utils.h"
#include "verticalfilter.h"
#include "ntv2resample.h"
#include "ntv2transcode.h"

#include <fstream>
#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CNTV2ScanConvert
// General Purpose Class to convert graphics to video to download to 
// HD-NTV or SD-NTV Card. This class derives from CNTV2Card

CNTV2ScanConvert::CNTV2ScanConvert(UWord boardNumber, bool displayErrorMessage, UWord ulBoardType)
	:   CNTV2Card(boardNumber, displayErrorMessage, ulBoardType)
{
	_workBuffer = NULL;
}

CNTV2ScanConvert::~CNTV2ScanConvert()
{
	if ( _workBuffer != NULL )
	{
		delete [] _workBuffer;
	}

	// board closed in CNTV2Card destructor
}


// ScanConvertQImage
// Convert QImage for ntv2 card.
// if channel = NTV2_CHANNEL1 we can use
// onboard colorspace converter
// if channel = NTV2_CHANNEL2 we need to colorspace convert in software
// This is some truly ugly code...it started with good intentions
// but went bad somewhere along the line.

bool CNTV2ScanConvert::ScanConvertQImage( QImage *pImage, 
										 NTV2Channel channel,
										 NTV2OutputFilter verticalFilter,
										 bool convertAspect,
										 bool autoSize) 
{
	UWord numLines,numPixels,frameBufferLinePitch;
	RGBAlphaPixel *RGBALinePtr[4];
	ULWord *frameBuffer;

	if ( pImage->depth() != 32 )
	{
		*pImage = pImage->convertDepth(32);
	}

	MapFrameBuffers();
	GetBaseAddress((NTV2Channel)channel,&frameBuffer);

	if ( _workBuffer == NULL )
	{ 
		_workBuffer = new char[WORKBUFFERSIZE];
	}

	RECT inputRect;
	inputRect.left = inputRect.top = 0;
	inputRect.right = pImage->width();
	inputRect.bottom = pImage->height();
	RECT outputRect = inputRect;
	SetupGeometry(inputRect,outputRect,numLines,numPixels,frameBufferLinePitch,convertAspect,autoSize);

	if ( autoSize &&
		 (inputRect.right != pImage->width() || 
		 inputRect.bottom != pImage->height()) )
	{
		// Need to resize bitmap
		*pImage = pImage->smoothScale(inputRect.right,inputRect.bottom);
	}

	// Initialize Buffer to All Black
	memset(_workBuffer,0,WORKBUFFERSIZE);

	RGBAlphaPixel *RGBBlackLine = (RGBAlphaPixel *)(_workBuffer);
	RGBAlphaPixel *RGBFilteredLine = (RGBAlphaPixel *)(_workBuffer+sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));
	RGBAlphaPixel *RGBResampledLine = (RGBAlphaPixel *)(_workBuffer+2*sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));
	RGBAlphaPixel *RGBLineBuffer = (RGBAlphaPixel *)(_workBuffer+3*sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));

	// This 4 pointers make up a circular buffer for use in vertical filtering.
	RGBALinePtr[0] = (RGBAlphaPixel *)(_workBuffer+4*sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));
	RGBALinePtr[1] = (RGBAlphaPixel *)(_workBuffer+5*sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));
	RGBALinePtr[2] = (RGBAlphaPixel *)(_workBuffer+6*sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));
	RGBALinePtr[3] = (RGBAlphaPixel *)(_workBuffer+7*sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));

	UWord* YCbCrLine = (UWord *)(_workBuffer+8*sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));
	ULWord* YCbCrPackedLine = (ULWord *)(_workBuffer+9*sizeof(RGBAlphaPixel)*(BUFFERLINEPITCH));

	if ( channel == NTV2_CHANNEL2 )
	{
		// no colorspace converter so need all this extra stuff.
		Create10BitblackCbYCrLine(YCbCrLine);
		PackLineData(YCbCrLine, YCbCrPackedLine,numPixels);
	}
	else
	{
		// reset from what was set in SetupGeometry.
		frameBufferLinePitch = numPixels;
	}

	// Fill top with black
	int lineCount;
	for ( lineCount = 0; lineCount < outputRect.top; lineCount++ )
	{
		if ( channel == NTV2_CHANNEL1 )
		{
			memcpy(frameBuffer,
					   RGBBlackLine,
					   numPixels*sizeof(RGBAlphaPixel));
		}
		else
		{
			memcpy(frameBuffer,
					   YCbCrPackedLine,
					   frameBufferLinePitch*sizeof(ULWord));
		}
		frameBuffer += frameBufferLinePitch;
	}

	int inputLine = inputRect.top;

	memcpy(&RGBLineBuffer[outputRect.left],
		       (RGBAlphaPixel *)pImage->scanLine(inputLine),
			   (outputRect.right-outputRect.left)*sizeof(RGBAlphaPixel));
	memcpy(RGBALinePtr[(inputLine)&0x3] ,RGBLineBuffer,BUFFERLINEPITCH*sizeof(RGBAlphaPixel));

	for ( ; lineCount < (outputRect.bottom) ; lineCount++ ,inputLine++)
	{
		// load vertical filter 
		if ( lineCount == (outputRect.bottom-1) )
		{
			// last line needs to be filtered with black
			memcpy(RGBALinePtr[(inputLine+1)&0x3] ,RGBBlackLine,BUFFERLINEPITCH*sizeof(RGBAlphaPixel));
		}
		else
		{
			// Copy over data to center of linebuffer
			memcpy(&RGBLineBuffer[outputRect.left],
				       (RGBAlphaPixel *)pImage->scanLine(inputLine+1),
					   (outputRect.right-outputRect.left)*sizeof(RGBAlphaPixel));
			memcpy(RGBALinePtr[(inputLine+1)&0x3] ,RGBLineBuffer,BUFFERLINEPITCH*sizeof(RGBAlphaPixel));
		}

		switch ( verticalFilter )
		{
		case NTV2OUTPUTFILTER_NONE:
			memcpy(RGBFilteredLine,
				       RGBALinePtr[(inputLine)&0x3],
					   numPixels*sizeof(RGBAlphaPixel));
			break;

		case NTV2OUTPUTFILTER_VERTICAL:
			VerticalFilterLine(RGBALinePtr[(inputLine-1)&0x3],
										RGBALinePtr[(inputLine)&0x3],
										RGBALinePtr[(inputLine+1)&0x3],
										RGBFilteredLine,
										numPixels);
			break;

		case NTV2OUTPUTFILTER_FIELD1:
			if ( lineCount & 0x1)
			{
				// Field 2 -> Interpolate
				FieldInterpolateLine(RGBALinePtr[(inputLine-1)&0x3],
											RGBALinePtr[(inputLine+1)&0x3],
											RGBFilteredLine,
											numPixels);
			}
			else
			{
				// Field 1 -> Pass Unaltered
				memcpy(RGBFilteredLine,
						   RGBALinePtr[(inputLine)&0x3],
						   numPixels*sizeof(RGBAlphaPixel));
			}
			break;

		case NTV2OUTPUTFILTER_FIELD2:		
			if ( lineCount & 0x1)
			{
				// Field 2 -> Pass Unaltered
				memcpy(RGBFilteredLine,
						   RGBALinePtr[(inputLine)&0x3],
						   numPixels*sizeof(RGBAlphaPixel));
			}
			else
			{
				// Field 1 -> Interpolate
				// Field 2 -> Interpolate
				FieldInterpolateLine(RGBALinePtr[(inputLine-1)&0x3],
											RGBALinePtr[(inputLine+1)&0x3],
											RGBFilteredLine,
											numPixels);
			}
			break;
		}

		if ( convertAspect )
		{
			ReSampleLine(RGBFilteredLine,
				         RGBResampledLine,
					     0,
					     numPixels,
						 numPixels,
						 NUMCOMPONENTPIXELS); //??????
		}
		else
		{
			memcpy(RGBResampledLine,
				       RGBFilteredLine,
					   numPixels*sizeof(RGBAlphaPixel));
		}

		if ( channel == NTV2_CHANNEL2 )
		{
			// no colorspace converter so need all this extra stuff.

            bool bIsSD;
            IsSDStandard(&bIsSD);   // support both SD and HD formats for Xena2
			ConvertLineToYCbCr422(RGBResampledLine, 
						   YCbCrLine, 
						   numPixels ,
						   0,
                           bIsSD);			
			PackLineData(YCbCrLine, YCbCrPackedLine,numPixels);
			memcpy(frameBuffer,
				   YCbCrPackedLine,
				   frameBufferLinePitch*sizeof(ULWord));
		}
		else
		{
			memcpy(frameBuffer,
					   RGBResampledLine,
					   numPixels*sizeof(RGBAlphaPixel));
		}

		frameBuffer += frameBufferLinePitch;
	}

	if ( channel == NTV2_CHANNEL2 )
	{
		// no colorspace converter so need all this extra stuff.
		Create10BitblackCbYCrLine(YCbCrLine);
		PackLineData(YCbCrLine, YCbCrPackedLine,numPixels);
	}

	// Fill bottom with black
	for ( ; lineCount < numLines; lineCount++ )
	{
		if ( channel == NTV2_CHANNEL1 )
		{
			memcpy(frameBuffer,
					   RGBBlackLine,
					   numPixels*sizeof(RGBAlphaPixel));
		}
		else
		{
			memcpy(frameBuffer,
					   YCbCrPackedLine,
					   frameBufferLinePitch*sizeof(ULWord));
		}
		frameBuffer += frameBufferLinePitch;
	}

	UnmapFrameBuffers();

	if ( channel == NTV2_CHANNEL2 )
	{
		SetFrameBufferFormat((NTV2Channel)channel, NTV2_FBF_10BIT_YCBCR);
	}
	else
	{
		SetFrameBufferFormat((NTV2Channel)channel, NTV2_FBF_ARGB);
	}

	FlipFlopPage((NTV2Channel)channel);
	return true;
}


bool CNTV2ScanConvert::ScanConvertFile(const char* fileName,
 						               NTV2Channel channel,
						               NTV2OutputFilter verticalFilter,
						               bool convertAspect,
						               bool autoSize )
{
	bool status = true;
	string file = fileName;

	if ( file.find(".yuv") == string::npos )
	{
		QImage image(fileName);
		if ( !image.isNull() )
		{
			ScanConvertQImage(&image,
							  channel,
							  verticalFilter,
							  convertAspect,
							  autoSize);
		}
	}
	else
	{
		status = ScanConvertYUVFile(fileName,channel,verticalFilter);
	}
	return status;	
}


bool CNTV2ScanConvert::ScanConvertYUVFile(const char* fileName,
										 NTV2Channel channel,
			  						     NTV2OutputFilter verticalFilter)
{
	ifstream inFile(fileName,ios::in | ios::binary);

	if ( inFile )
	{
		ULWord *frameBuffer;
		ULWord linePitchBytes;
		ULWord numLines,numPixels;

		MapFrameBuffers();			
		GetBaseAddress((NTV2Channel)channel,&frameBuffer);

		inFile.seekg(0,ios::end );
		int fileSize = inFile.tellg();
		inFile.seekg(0,ios::beg );

		switch ( fileSize )
		{
		case HD_NUMACTIVELINES_1080*HD_YCBCRLINEPITCH_1080*4:
			numLines = HD_NUMACTIVELINES_1080;
			numPixels = HD_NUMCOMPONENTPIXELS_1080;
			linePitchBytes = HD_YCBCRLINEPITCH_1080*4;
			break;

		case HD_NUMACTIVELINES_720*HD_YCBCRLINEPITCH_720*4:
			numLines = HD_NUMACTIVELINES_720;
			numPixels = HD_NUMCOMPONENTPIXELS_720;
			linePitchBytes = HD_YCBCRLINEPITCH_720*4;
			break;

		case NUMACTIVELINES_525*YCBCRLINEPITCH_SD*4:
			numLines = NUMACTIVELINES_525;
			numPixels = NUMCOMPONENTPIXELS;
			linePitchBytes = YCBCRLINEPITCH_SD*4;
			break;

		case NUMACTIVELINES_625*YCBCRLINEPITCH_SD*4:
			numLines = NUMACTIVELINES_625;
			numPixels = NUMCOMPONENTPIXELS;
			linePitchBytes = YCBCRLINEPITCH_SD*4;
			break;

		default:
			// File not recognized
			inFile.close();
			return false;
			break;			
		}

		if ( verticalFilter == NTV2OUTPUTFILTER_NONE || 
			 verticalFilter == NTV2OUTPUTFILTER_VERTICAL)
		{
			inFile.read((char*)frameBuffer,(streamsize)fileSize);
		}
		else
		{
			char* readBufferField1 = new char[linePitchBytes];
			char* readBufferField2 = new char[linePitchBytes];
			char* readBufferInterpolated = new char[linePitchBytes];
			inFile.read(readBufferField1,linePitchBytes);
			inFile.read(readBufferField2,linePitchBytes);

			for ( ULWord lineCount = 0; lineCount < numLines/2; lineCount++ )
			{
				switch ( verticalFilter )
				{
				case NTV2OUTPUTFILTER_FIELD1:
					memcpy(frameBuffer,readBufferField1,linePitchBytes);
					frameBuffer += linePitchBytes/4;
					memcpy(frameBuffer,readBufferField1,linePitchBytes);
					break;

				case NTV2OUTPUTFILTER_FIELD2:		
					memcpy(frameBuffer,readBufferField2,linePitchBytes);
					frameBuffer += linePitchBytes/4;
					memcpy(frameBuffer,readBufferField2,linePitchBytes);
					break;
					
				case NTV2OUTPUTFILTER_NONE:		// Nothing to do
				case NTV2OUTPUTFILTER_VERTICAL:	// Nothing to do
					break;
				}

				frameBuffer += linePitchBytes/4;
				inFile.read(readBufferField1,linePitchBytes);
				inFile.read(readBufferField2,linePitchBytes);
			}
			delete [] readBufferField1;
			delete [] readBufferField2;
			delete [] readBufferInterpolated;
		}

		inFile.close();
		UnmapFrameBuffers();
		SetFrameBufferFormat((NTV2Channel)channel, NTV2_FBF_10BIT_YCBCR);
		FlipFlopPage((NTV2Channel)channel);
		return true;
	}
	else
	{
		return false;
	}
}


bool CNTV2ScanConvert::ScanConvertYUVFileToQImage(char* fileName,
										          QImage* pImage)
{
	ifstream inFile(fileName,ios::in | ios::binary);

	if ( inFile )
	{
		ULWord linePitchBytes;
		ULWord numLines,numPixels;

		inFile.seekg(0,ios::end );
		int fileSize = inFile.tellg();
		inFile.seekg(0,ios::beg );

		switch ( fileSize )
		{
		case HD_NUMACTIVELINES_1080*HD_YCBCRLINEPITCH_1080*4:
			numLines = HD_NUMACTIVELINES_1080;
			numPixels = HD_NUMCOMPONENTPIXELS_1080;
			linePitchBytes = HD_YCBCRLINEPITCH_1080*4;
			break;

		case HD_NUMACTIVELINES_720*HD_YCBCRLINEPITCH_720*4:
			numLines = HD_NUMACTIVELINES_720;
			numPixels = HD_NUMCOMPONENTPIXELS_720;
			linePitchBytes = HD_YCBCRLINEPITCH_720*4;
			break;

		case NUMACTIVELINES_525*YCBCRLINEPITCH_SD*4:
			numLines = NUMACTIVELINES_525;
			numPixels = NUMCOMPONENTPIXELS;
			linePitchBytes = YCBCRLINEPITCH_SD*4;
			break;

		case NUMACTIVELINES_625*YCBCRLINEPITCH_SD*4:
			numLines = NUMACTIVELINES_625;
			numPixels = NUMCOMPONENTPIXELS;
			linePitchBytes = YCBCRLINEPITCH_SD*4;
			break;

		default:
			// File not recognized
			inFile.close();
			return false;
			break;			
		}

		pImage->create(numPixels,numLines,32);
		char* readBuffer = new char[linePitchBytes];
		UWord* unpackedBuffer = new UWord[numPixels*2];
		bool  bIsSD;
			
		IsSDStandard(&bIsSD);

		for ( ULWord lineCount = 0; lineCount < numLines; lineCount++ )
		{
			inFile.read((char*)readBuffer,linePitchBytes);
			UnPackLineData((ULWord*) readBuffer, unpackedBuffer,  numPixels);
			ConvertLinetoRGB(unpackedBuffer, (RGBAlphaPixel *)pImage->scanLine(lineCount),numPixels, bIsSD);		
		}

		inFile.close();
		delete [] readBuffer;
		delete [] unpackedBuffer;
		return true;
	}
	else
	{
		return false;
	}
}


void CNTV2ScanConvert::Create10BitblackCbYCrLine(UWord* lineData)
{
	// Assume 1080 format, it'll work for 720 as well

	for ( int count = 0; count < HD_NUMCOMPONENTPIXELS_1080*2; count+=2 )
	{
		lineData[count] = (UWord)CCIR601_10BIT_CHROMAOFFSET;
		lineData[count+1] = (UWord)CCIR601_10BIT_BLACK;
	}		
}


bool CNTV2ScanConvert::SetupGeometry(RECT &inputRect, 
									   RECT &outputRect, 
									   UWord &numLines,
									   UWord &numPixels,
									   UWord &frameBufferLinePitch,
									   bool &convertAspect,
									   bool autoSize)
{
	ULWord maxVertical,maxHorizontal;
	
	if ( GetNumberActiveLines(&maxVertical) == FALSE )
	  return FALSE;

	switch ( maxVertical )
	{
	case HD_NUMACTIVELINES_720:
		maxHorizontal = HD_NUMCOMPONENTPIXELS_720;
		frameBufferLinePitch = HD_YCBCRLINEPITCH_720;
		break;

	case HD_NUMACTIVELINES_1080:
		maxHorizontal = HD_NUMCOMPONENTPIXELS_1080;
		frameBufferLinePitch = HD_YCBCRLINEPITCH_1080;
		break;

	case NUMACTIVELINES_525:
		if ( convertAspect )
		{
			maxHorizontal = MAXSQUAREPIXELS_525;
			frameBufferLinePitch = RGBALPHALINEPITCH_525;
		}
		else
		{
			maxHorizontal = NUMCOMPONENTPIXELS;
			frameBufferLinePitch = YCBCRLINEPITCH_SD;
		}
		break;

	case NUMACTIVELINES_625:
		if ( convertAspect )
		{
			maxHorizontal = MAXSQUAREPIXELS_625;
			frameBufferLinePitch = RGBALPHALINEPITCH_625;
		}
		else
		{
			maxHorizontal = NUMCOMPONENTPIXELS;
			frameBufferLinePitch = YCBCRLINEPITCH_SD;
		}
		break;

	case HD_ROLLNUMLINES:
		maxHorizontal = HD_ROLLNUMPIXELS;
		break;
	}

	numLines = maxVertical;
	numPixels = maxHorizontal;

	if ( autoSize )
	{
		double widthScale = (double)numPixels/(double)inputRect.right;
		double heightScale = (double)numLines/(double)inputRect.bottom;
		double scaleFactor=1.0;

		if ( widthScale < 1.0  || heightScale < 1.0 )
		{
			// Shrink
			if ( widthScale < heightScale )
			{
				scaleFactor = widthScale;
			}
			else
			{
				scaleFactor = heightScale;
			}
		}
		else
		if ( widthScale > 1.0  && heightScale > 1.0 )
		{
			//expand
			if ( widthScale < heightScale )
			{
				scaleFactor = widthScale;
			}
			else
			{
				scaleFactor = heightScale;
			}
		}
		
		if ( scaleFactor != 1.0 )
		{
			inputRect.right = (int)((double)inputRect.right*scaleFactor);
			inputRect.bottom = (int)((double)inputRect.bottom*scaleFactor);
		}
	}

	if ( (inputRect.bottom - inputRect.top) > (int)maxVertical )
	{
		// Clamp to maxVertical
		outputRect.top = 0;
		outputRect.bottom = maxVertical;
	}
	else
	{
		// Center inputlines
		outputRect.top = ( maxVertical - ( inputRect.bottom - inputRect.top) ) / 2;
		outputRect.bottom = outputRect.top + ( inputRect.bottom - inputRect.top);
	}

	if ( (inputRect.right - inputRect.left) > (int)maxHorizontal )
	{
		// Clamp to maxHorizontal
		inputRect.right = inputRect.left + maxHorizontal;
		outputRect.left = 0;
		outputRect.right = maxHorizontal;
	}
	else
	{
		// Center inputPixels
		outputRect.left = (maxHorizontal - (inputRect.right - inputRect.left)) / 2;	
		outputRect.right = outputRect.left + (inputRect.right - inputRect.left);
	}
	return true;
}
