/**
	@file		ntv2resample.h
	@brief		Declares a number of pixel resampling functions.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/
#ifndef RESAMPLE_H
#define RESAMPLE_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "fixed.h"
#include "videodefines.h"


// ReSampleLine
// RGBAlphaPixel Version
AJAExport
void ReSampleLine(RGBAlphaPixel *Input, 
				  RGBAlphaPixel *Output,
				  UWord startPixel,
				  UWord endPixel,
				  LWord numInputPixels,
				  LWord numOutputPixels);


// ReSampleLine
// Word Version
AJAExport
void ReSampleLine(Word *Input, 
			      Word *Output,
			      UWord startPixel,
			      UWord endPixel,
			      LWord numInputPixels,
			      LWord numOutputPixels);
					  

// ReSampleLine
// Word Version
AJAExport
void ReSampleYCbCrSampleLine(Word *Input, 
			                 Word *Output,
			                 LWord numInputPixels,
			                 LWord numOutputPixels);

// ReSampleLine
// Word Version
AJAExport
void ReSampleAudio(Word *Input, 
				  Word *Output,
				  UWord startPixel,
				  UWord endPixel,
				  LWord numInputPixels,
				  LWord numOutputPixels,
				  Word channelInterleaveMulitplier=1);

#endif	//	RESAMPLE_H
