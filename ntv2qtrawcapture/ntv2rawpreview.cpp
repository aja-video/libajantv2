/**
	@file		ntv2rawpreview.cpp
	@brief		Implementation if the NTV2RawPreview class.
	@copyright	(C) 2014-2020 AJA Video Systems, Inc.  All rights reserved.
**/

#include <stdio.h>
#include "ajabase/system/memory.h"
#include "ajabase/system/systemtime.h"
#include "ntv2rawframegrabber.h"
#include "ntv2rawpreview.h"
#include "ntv2rp188.h"


// Unpack raw 10 bit values from the source to 16 bit values in the destination
static void Unpack10BitRaw (uint16_t* pDstBuffer, uint8_t* pSrcBuffer, uint32_t byteCount)
{
	uint32_t	state	= 0;
	uint16_t	out16	= 0;
	uint8_t		in8		= 0;

	while( byteCount )
	{
		switch( state )
		{
			case 0:
				out16 = *pSrcBuffer++;
				break;

			case 1:
				in8 = *pSrcBuffer++;
				out16 = (out16 << 2) | (in8 >> 6);
				*pDstBuffer++ = out16;
				out16 = in8 & 0x3F;
				break;

			case 2:
				in8 = *pSrcBuffer++;
				out16 = (out16 << 4) | (in8 >> 4);
				*pDstBuffer++ = out16;
				out16 = in8 & 0x0F;
				break;

			case 3:
				in8 = *pSrcBuffer++;
				out16 = (out16 << 6) | (in8 >> 2);
				*pDstBuffer++ = out16;
				out16 = in8 & 0x03;
				break;

			case 4:
				in8 = *pSrcBuffer++;
				out16 = (out16 << 8) | in8;
				*pDstBuffer++ = out16;
				out16 = 0;
				break;

			default:
				printf("ENTERED BAD STATE %d\n", state);
				break;
		}

		byteCount--;
		state++;
		if (state == 5)
			state = 0;
	}

	return;
}


NTV2RawPreview::NTV2RawPreview (NTV2RawFrameGrabber * pParent, AJACircularBuffer <AVDataBuffer *> & circularBuffer) :
	mpParent			(pParent),
	mCircularBuffer		(circularBuffer),
	mpUnpackedRawBuffer	(NULL),
	mpDebayeredFrame	(NULL)
{
	mpUnpackedRawBuffer = (uint16_t *)AJAMemory::AllocateAligned (4096 * 2160 * sizeof(uint16_t), 4096);
	if (!mpUnpackedRawBuffer)
		printf("NTV2RawPreview alloc of unpacked buffer failed\n");

	mpDebayeredFrame = (uint8_t *)AJAMemory::AllocateAligned (1200 * 2160 * sizeof(uint32_t), 4096);
	if (!mpDebayeredFrame)
		printf("NTV2RawPreview alloc of debayered frame failed\n");
}


NTV2RawPreview::~NTV2RawPreview (void)
{
    AJAMemory::FreeAligned (mpDebayeredFrame);
	mpDebayeredFrame = NULL;

    AJAMemory::FreeAligned (mpUnpackedRawBuffer);
	mpUnpackedRawBuffer = NULL;
}


bool NTV2RawPreview::ThreadLoop (void)
{
	if (mCircularBuffer.GetCircBufferCount () == 0)
	{
		AJATime::Sleep (5);
		return true;		// There aren't any frames to display
	}

	//	Throw away input frames until only the most recent is left
	//	We can't display in real time, but this will keep the preview
	//	monitor close to the frame being captured.
	while (mCircularBuffer.GetCircBufferCount () > 1)
	{
		mCircularBuffer.StartConsumeNextBuffer ();
		mCircularBuffer.EndConsumeNextBuffer ();
	}

	//	Now get the frame we'll display
	AVDataBuffer *	playData	(mCircularBuffer.StartConsumeNextBuffer ());

	if (playData)
	{
		uint64_t	sourceLineWidth	= (uint64_t) playData->fVideoBufferUnaligned;
		int32_t		imageWidth		= (sourceLineWidth == 3840) ? 1920 : 2048;
		int32_t		imageHeight 	= 1080;

		std::string		currentTcStr;
		RP188_STRUCT	currentRP188	(playData->fRP188Data);
		CRP188			inputRP188Info	(currentRP188);
		inputRP188Info.GetRP188Str		(currentTcStr);

		mpParent->SetNewStatusString (currentTcStr);

		Unpack10BitRaw (mpUnpackedRawBuffer, (uint8_t *) playData->fVideoBuffer, playData->fVideoBufferSize );

		uint8_t * pBits = mpDebayeredFrame;
		for (int32_t lineCount = 0; lineCount < imageHeight; lineCount++)
		{
			uint16_t * lineBuffer = &mpUnpackedRawBuffer [lineCount * sourceLineWidth * 2];

			for (int32_t pixelCount = 0; pixelCount < imageWidth; pixelCount++)
			{
				uint8_t g1 = lineBuffer [pixelCount * 2] >> 2;
				uint8_t g2 = lineBuffer [sourceLineWidth + (pixelCount * 2) + 1] >> 2;
				uint8_t b0 = lineBuffer [(pixelCount * 2) + 1] >> 2;
				uint8_t r0 = lineBuffer [sourceLineWidth + (pixelCount * 2)] >> 2;

				*pBits++ = b0;
				*pBits++ = (g1+g2) / 2;
				*pBits++ = r0;
				*pBits++ = 0xFF;	// Alpha
			}
		}

		mpParent->SetNewFrame (mpDebayeredFrame, imageWidth);

		//	Signal that the frame has been "consumed"...
		mCircularBuffer.EndConsumeNextBuffer ();
	}
	else
	{
		printf("Preview null playdata %d entries\n", mCircularBuffer.GetCircBufferCount ());
	}

	return true;
}

