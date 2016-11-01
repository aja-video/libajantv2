/////////////////////////////////////////////////////////////////////////////
// ntv2draw.h
// Very simple class to demonstrate interfacing to AJA OEM boards.
// 
// Copyright (C) 2004, 2005 AJA Video Systems, Inc.  Proprietary and Confidential information.
//
#ifndef NTV2DRAW_H
#define NTV2DRAW_H

#include "ntv2card.h"

/* absolute value of a */
#define ABS(a)		(((a)<0) ? -(a) : (a))

/* take binary sign of a, either -1, or 1 if >= 0 */
#define SGN(a)		(((a)<0) ? -1 : 1)

/* swap two numbers, works only for  integers */
#define SWAP(X, Y) (X)^=(Y); (Y)^=(X); (X)^=(Y);   

const RGBAlphaPixel redPixel     = {0x00,0x00,0xFF,0xFF};
const RGBAlphaPixel greenPixel   = {0x00,0xFF,0x00,0xFF};
const RGBAlphaPixel bluePixel    = {0xFF,0x00,0x00,0xFF};
const RGBAlphaPixel cyanPixel    = {0xFF,0xFF,0x00,0xFF};
const RGBAlphaPixel magentaPixel = {0xFF,0x00,0xFF,0xFF};
const RGBAlphaPixel yellowPixel  = {0x00,0xFF,0xFF,0xFF};
const RGBAlphaPixel blackPixel   = {0x00,0x00,0x00,0xFF};
const RGBAlphaPixel whitePixel   = {0xFF,0xFF,0xFF,0xFF};
const RGBAlphaPixel lightgrayPixel   = {0x60,0x60,0x60,0xFF};

#include "ntv2status.h"

class CNTV2Draw : public CNTV2Card
{
public:  // Constructors
	CNTV2Draw(UWord boardNumber, bool displayError, NTV2DeviceType boardType);   
	~CNTV2Draw();

public:  // Methods
	void SetPixelColor(RGBAlphaPixel newColor);
	void SetPixel(UWord x, UWord y);
	void DrawLine(UWord x1, UWord y1, UWord x2, UWord y2);
	void VertLine (UWord x, UWord y1, UWord y2);
	void HorzLine (UWord x1, UWord x2, UWord y);
	void DrawRectangle(UWord x1,UWord y1,UWord x2,UWord y2);
	void DrawCircle (Word xCenter, Word yCenter, Word radius );
	void DrawEllipse (LWord xCenter, LWord yCenter, LWord xRadius, LWord yRadius );
	void EraseBuffer();
	void FillBuffer();

	// For drawing into a framebuffer
	void SetBaseAddress(RGBAlphaPixel* baseAddress) { _baseAddress = baseAddress; }

protected:  // Methods


protected:  // Data
	RGBAlphaPixel		_pixelColor;
	RGBAlphaPixel*		_baseAddress;
	NTV2FrameDimensions	_frameBufferSize;	
	ULWord				_linePitch;           // #bytes til next line
};



#endif
