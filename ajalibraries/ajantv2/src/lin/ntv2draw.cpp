/////////////////////////////////////////////////////////////////////////////
// NTV2Draw.cpp
// NOTE: Not intended to be a drawing package
//       Just to show how to write to HD-NTV or SD-NTV
//       FrameBuffer
//
// Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//

#include <iostream>
#include <stdlib.h>
#include "ntv2draw.h"

CNTV2Draw::CNTV2Draw(UWord boardNumber, bool displayError, NTV2DeviceType boardType)
	:   CNTV2Card(boardNumber, displayError, boardType) 
{
	if ( IsOpen() )
	{
		if (!IsXilinxProgrammed())
		{
	   		cout << "Xilinx not programmed, unable to draw." << endl;
	   		exit(-1);
		}

		_pixelColor = blackPixel;

		SetPCIAccessFrame(NTV2_CHANNEL1,0);                      // Set PCI and Output Frames equal
		SetOutputFrame(NTV2_CHANNEL1,0);                         // This will allow you to see the output
																 // as it is writing.
		SetFrameBufferFormat(NTV2_CHANNEL1,NTV2_FBF_ARGB);       // Set to ARGB mode

		// If we can't get the base address probably because the module was
		// loaded without memory mapping enabled, bail immediately.
		if (!GetBaseAddress(NTV2_CHANNEL1,(ULWord**)(&_baseAddress)))
		{
			DisplayNTV2Error("Framebuffers not mapped, unable to draw.  Aborting.\n");
			exit(0);
		}

		GetActiveFrameDimensions(_frameBufferSize);
		_linePitch = _frameBufferSize.Width() * sizeof(RGBAlphaPixel);
	}


}

CNTV2Draw::~CNTV2Draw()
{
	if ( IsOpen() )
	{
		// Set back up for ping ponging.
		SetPCIAccessFrame(NTV2_CHANNEL1,1);
	}

	// board closed in CNTV2Card desctructor
}

void CNTV2Draw::SetPixelColor(RGBAlphaPixel newColor)
{
	_pixelColor = newColor;
}

void CNTV2Draw::SetPixel(UWord x, UWord y)
{
	RGBAlphaPixel* address = (RGBAlphaPixel*)((unsigned long)_baseAddress  +
		                                      y*_linePitch +
											  x*sizeof(RGBAlphaPixel));

	*address = _pixelColor;
}


// From Graphics Gems
void CNTV2Draw::DrawLine(UWord x1, UWord y1, UWord x2, UWord y2)
{
    Word d, x, y, ax, ay, sx, sy, dx, dy;

    dx = x2-x1;  ax = ABS(dx)<<1;  sx = SGN(dx);
    dy = y2-y1;  ay = ABS(dy)<<1;  sy = SGN(dy);

    x = x1;
    y = y1;
    if (ax>ay) {		/* x dominant */
	d = ay-(ax>>1);
	for (;;) {
	    SetPixel(x, y);
	    if (x==x2) return;
	    if (d>=0) {
		y += sy;
		d -= ax;
	    }
	    x += sx;
	    d += ay;
	}
    }
    else {			/* y dominant */
	d = ax-(ay>>1);
	for (;;) {
	    SetPixel(x, y);
	    if (y==y2) return;
	    if (d>=0) {
		x += sx;
		d -= ay;
	    }
	    y += sy;
	    d += ax;
	}
    }
}


void CNTV2Draw::DrawCircle (Word xCenter, Word yCenter, Word radius )
{

   DrawEllipse(xCenter,yCenter,radius,radius);
}


void CNTV2Draw::DrawEllipse (LWord xCenter, LWord yCenter, LWord xRadius, LWord yRadius )
{
  LWord x, y, d, de, dse, xRadius2, yRadius2, re, k1, k2;

  if (xRadius==0)
  {
     VertLine (xCenter, yCenter-yRadius, yCenter+yRadius);
      return;
  }
  if (yRadius==0)
  {
     HorzLine (xCenter-xRadius, xCenter+xRadius, yCenter);
     return;
  }

  x = 0;
  y = yRadius;

  xRadius2 = xRadius*xRadius;
  yRadius2 = yRadius*yRadius;

  /* First octant */

  d = yRadius2-xRadius2*yRadius+xRadius2/4;
  de = 3*yRadius2;
  dse = de+xRadius2*(-2*yRadius+2);

  k1 = 2*yRadius2;
  k2 = k1+2*xRadius2;
  re = xRadius2/2+yRadius2;

  SetPixel (xCenter+x, yCenter+y);
  SetPixel (xCenter+x, yCenter-y);

  while (xRadius2*y-yRadius2*x>re)
  {
     if (d<0)
     {
	d += de;
	de += k1;
	dse += k1;
	x++;
     }
     else
     {
	d += dse;
	de += k1;
	dse += k2;
	x++;
	y--;
     }
     SetPixel (xCenter+x, yCenter+y);
     SetPixel (xCenter-x, yCenter+y);
     if (y!=0)
     {
	SetPixel (xCenter+x, yCenter-y);
	SetPixel (xCenter-x, yCenter-y);
     }
  }

  /* Second octant */

  d = yRadius2*(x*x+x-xRadius2)+yRadius2/4+xRadius2*(y*y-2*y+1);
  dse = xRadius2*(-2*y+3);
  de = dse+yRadius2*(2*x+2);
  k1 = 2*xRadius2;
  k2 = k1+2*yRadius2;

  while (y>0)
  {
     if (d<0)
     {
	d += de;
	de += k2;
	dse += k1;
	x++;
	y--;
     }
     else
     {
	d += dse;
	de += k1;
	dse += k1;
	y--;
     }
     SetPixel (xCenter+x, yCenter+y);
     SetPixel (xCenter-x, yCenter+y);
     if (y!=0)
     {
	SetPixel (xCenter+x, yCenter-y);
	SetPixel (xCenter-x, yCenter-y);
     }
  }
}

void CNTV2Draw::VertLine (UWord x, UWord y1, UWord y2)
{
   if (y2<y1) {SWAP (y1,y2);}
   do
     SetPixel (x, y1++);
   while (y1<=y2);
}

void CNTV2Draw::HorzLine (UWord x1, UWord x2, UWord y)
{
   if (x2<x1) {SWAP (x1,x2);}
   do
     SetPixel (x1++,y);
   while (x1<=x2);
}


void CNTV2Draw::DrawRectangle(UWord x1,UWord y1,UWord x2,UWord y2)
{
	HorzLine(x1,x2,y1);
	HorzLine(x1,x2,y2);
	VertLine(x1,y1,y2);
	VertLine(x2,y1,y2);

}
// Fill Buffer with current pixel Color
void CNTV2Draw::FillBuffer()
{
	for (ULWord lineCount = 0; lineCount < _frameBufferSize.Height(); lineCount++ )
	{
		for (ULWord pixelCount = 0; pixelCount < _frameBufferSize.Width(); pixelCount++ )
		{
			SetPixel(pixelCount,lineCount);
		}

	}
}

// Clear Buffer to Black
void CNTV2Draw::EraseBuffer()
{
	memset(_baseAddress,0,
		   _frameBufferSize.Height() * _frameBufferSize.Width() * sizeof(RGBAlphaPixel));

}
