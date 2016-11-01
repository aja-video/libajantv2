/**
	@file		ntv2verticalfilter.h
	@brief		Declares the VerticalFilterLine and FieldInterpolateLine functions for
				vertically filtering interlaced video with a 1/4,1/2,1/4 filter.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef VERTICALFILTER_H
#define VERTICALFILTER_H

#include "ajaexport.h"
#include "ajatypes.h"
#include "videodefines.h"
#include "fixed.h"


AJAExport void	VerticalFilterLine (	RGBAlphaPixel *	topLine,
										RGBAlphaPixel *	midLine,
										RGBAlphaPixel *	bottomLine,
										RGBAlphaPixel *	destLine,
										LWord			numPixels	);

AJAExport void	FieldInterpolateLine (	RGBAlphaPixel *	topLine,
										RGBAlphaPixel *	bottomLine,
										RGBAlphaPixel *	destLine,
										LWord			numPixels	);

#endif	//	VERTICALFILTER_H
