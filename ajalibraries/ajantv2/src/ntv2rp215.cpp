/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2rp215.cpp
	@brief		Implements the CNTV2RP215Decoder class. See SMPTE RP215 standard for details.
	@copyright	(C) 2006-2021 AJA Video Systems, Inc.
**/

#include "ntv2rp215.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#ifndef MSWindows
	using namespace std;
#endif

/////////////////////////////////////////////////////////////////////////////
// Line21Captioner definition
/////////////////////////////////////////////////////////////////////////////
#if defined (MSWindows)
#pragma warning(disable: 4800) 
#endif

UWord ancillaryPacketHeader[6] = 
{
	0x000,
	0x3FF,
	0x3FF,
	0x151,
	0x101,
	0x2D7
};

UWord raw215Data[] = 
{
		0x110,
		0x12C,
		0x107,
		0x224,
		0x256,
		0x116,
		0x200,
		0x200,
		0x200,
		0x200,
		0x218,
		0x115,
		0x108,
		0x205,
		0x200,
		0x200,
		0x108,
		0x200,
		0x200,
		0x24B,
		0x23F,
		0x102,
		0x217,
		0x104,
		0x108,
		0x140,
		0x176,
		0x287,
		0x104,
		0x146,
		0x146,
		0x146,
		0x146,
		0x107,
		0x138,
		0x107,
		0x101,
		0x101,
		0x200,
		0x200,
		0x200,
		0x200,
		0x2C3,
		0x104,
		0x214,
		0x104,
		0x120,
		0x143,
		0x2F6,
		0x27E,
		0x200,
		0x200,
		0x200,
		0x200,
		0x200,
		0x120,
		0x120,
		0x120,
		0x120,
		0x108,
		0x104,
		0x104,
		0x200,
		0x137,
		0x132,
		0x239,
		0x236,
		0x138,
		0x137,
		0x120,
		0x120,
		0x143,
		0x132,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x235,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x137,
		0x132,
		0x239,
		0x236,
		0x138,
		0x137,
		0x120,
		0x120,
		0x134,
		0x235,
		0x241,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x132,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x230,
		0x260,
		0x230,
		0x230,
		0x137,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x242,
		0x145,
		0x241,
		0x255,
		0x154,
		0x259,
		0x120,
		0x253,
		0x248,
		0x14F,
		0x250,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x146,
		0x14F,
		0x154,
		0x14F,
		0x24B,
		0x145,
		0x24D,
		0x120,
		0x132,
		0x22D,
		0x242,
		0x255,
		0x152,
		0x242,
		0x241,
		0x24E,
		0x24B,
		0x22D,
		0x255,
		0x253,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x120,
		0x16D,
		0x24E,
		0x13C
};



/////////////////////////////////////////////////////////////////////////////
// Constructor


CNTV2RP215Decoder::CNTV2RP215Decoder (ULWord * pFrameBufferBaseAddress, NTV2VideoFormat videoFormat, NTV2FrameBufferFormat fbFormat)
	:	_frameBufferBasePointer (pFrameBufferBaseAddress),
		_videoFormat			(videoFormat),
		_fbFormat				(fbFormat),
		_lineNumber				(-1),
		_pixelNumber			(-1)
{
}



CNTV2RP215Decoder::~CNTV2RP215Decoder ()
{
}

bool CNTV2RP215Decoder::Locate (void)
{
	bool found = false;

	NTV2FormatDescriptor fd (_videoFormat,_fbFormat);
	UWord* rp215Linebuffer = new UWord[fd.numPixels*2];
	switch (_fbFormat )
	{
	case NTV2_FBF_10BIT_DPX:
		{
			ULWord* frameBuffer = _frameBufferBasePointer;
			for ( Word lineNumber=0; lineNumber<30 && found == false;lineNumber++)
			{
				::UnPack10BitDPXtoForRP215withEndianSwap(rp215Linebuffer,frameBuffer,fd.numPixels);
				for ( UWord pixelNumber=0;pixelNumber<(fd.numPixels-RP215_PAYLOADSIZE);pixelNumber++)
				{
					if ( rp215Linebuffer[0] == 0x000 && 
						 rp215Linebuffer[1] == 0x3ff &&
						 rp215Linebuffer[2] == 0x3ff &&
						 rp215Linebuffer[3] == 0x151 &&
						 rp215Linebuffer[4] == 0x101
						)

					{
						found = true;
						_lineNumber =  lineNumber;
						_pixelNumber = pixelNumber;
					}
				}
				frameBuffer += fd.linePitch;
			}
		}
		break;
	case NTV2_FBF_10BIT_DPX_LE:
		break;
	case NTV2_FBF_10BIT_YCBCR:
		{
			const ULWord* frameBuffer = _frameBufferBasePointer;
			for (int lineNumber = 0; lineNumber < 20 && found == false ; lineNumber ++)
			{
				::UnpackLine_10BitYUVto16BitYUV(frameBuffer, rp215Linebuffer, fd.numPixels);
				if ( rp215Linebuffer[1] == 0x000 && 
					rp215Linebuffer[3] == 0x3ff &&
					rp215Linebuffer[5] == 0x3ff &&
					rp215Linebuffer[7] == 0x151 &&
					rp215Linebuffer[9] == 0x101	   )
				{
					UWord* pBuffer = &rp215Linebuffer[13];
					for ( int i=0; i < RP215_PAYLOADSIZE; i++ )
					{
						_rp215RawBuffer[i] = (UByte)(*pBuffer);
						pBuffer += 2;
					}
					found = true;
					_lineNumber =  lineNumber;
					_pixelNumber = 0;
					odprintf("found l(%d) p(%d)\n",_lineNumber,_pixelNumber);

				}
				frameBuffer += fd.linePitch;

			}
		}
		break;
	default:
		break;
	}
	
	delete [] rp215Linebuffer;

	return found;
}


bool CNTV2RP215Decoder::Extract()
{
	return false;
}


#ifdef MSWindows
#pragma warning(default: 4800)
#endif
