/* SPDX-License-Identifier: MIT */
//
// Copyright (C) 2012 AJA Video Systems, Inc.
// Proprietary and Confidential information.
//
#include "ntv2gpucircularbuffer.h"
#include "ajabase/common/circularbuffer.h"
#define NTV2_AUDIOSIZE_MAX (0x100000)
#include <assert.h>

CNTV2GpuCircularBuffer::CNTV2GpuCircularBuffer() :
	mAVTextureBuffers(NULL), mNumFrames(0), mAbort(false)
{
}

CNTV2GpuCircularBuffer::~CNTV2GpuCircularBuffer()
{
	if ( mAVTextureBuffers )
	{
		for ( ULWord i=0; i<mNumFrames; i++ )
		{
			if(mAVTextureBuffers[i].videoBuffer)
			{
				delete [] mAVTextureBuffers[i].videoBufferUnaligned;
				mAVTextureBuffers[i].videoBufferUnaligned = NULL;
				mAVTextureBuffers[i].videoBuffer = NULL;
			}
 			if(mAVTextureBuffers[i].texture)
			{
				delete mAVTextureBuffers[i].texture;
				mAVTextureBuffers[i].texture = NULL;
			}
			if(mAVTextureBuffers[i].renderToTexture)
			{
				delete mAVTextureBuffers[i].renderToTexture;
				mAVTextureBuffers[i].renderToTexture = NULL;
			}
			if(mAVTextureBuffers[i].audioBuffer)
				delete [] mAVTextureBuffers[i].audioBuffer;
		}
		
		if ( mAVTextureBuffers )
			delete [] mAVTextureBuffers;
		mAVTextureBuffers = NULL;
	}
}

void CNTV2GpuCircularBuffer::Allocate(ULWord numFrames,
									  ULWord videoWriteSize,
									  ULWord width,
									  ULWord height,
									  bool withAudio,
									  bool withRenderToTexture,
									  size_t alignment)
{
	assert( mNumFrames == 0 );
	assert( numFrames > 0 );
	assert( videoWriteSize >=0 && videoWriteSize < 1e8 );
	assert( width > 0 && width < 1e6 );
	assert( height > 0 && height < 1e6 );
	
	mNumFrames = numFrames;
	
	mAVCircularBuffer.SetAbortFlag(&mAbort);
	
	mAVTextureBuffers = new AVTextureBuffer[mNumFrames];
	memset(mAVTextureBuffers, 0, sizeof(AVTextureBuffer)*mNumFrames);
	
	for ( ULWord i=0; i<mNumFrames; i++ )
	{
		CNTV2Texture* texture = new CNTV2Texture;
		texture->InitWithBitmap(NULL, width, height, false);
		mAVTextureBuffers[i].texture = texture;
		
		if ( withRenderToTexture )
		{
			CNTV2RenderToTexture* renderToTexture = new CNTV2RenderToTexture;
			renderToTexture->SetTexture(texture);
			mAVTextureBuffers[i].renderToTexture = renderToTexture;
		}
		
		mAVTextureBuffers[i].videoBufferSize = videoWriteSize;
		mAVTextureBuffers[i].videoBufferUnaligned = new UByte[videoWriteSize + alignment - 1];
		uint64_t val = (uint64_t)(mAVTextureBuffers[i].videoBufferUnaligned);
		val += alignment-1;
		val &= ~(alignment-1);
		mAVTextureBuffers[i].videoBuffer = (ULWord*) val;
		
		if ( withAudio )
		{
			mAVTextureBuffers[i].audioBuffer = new ULWord[NTV2_AUDIOSIZE_MAX/sizeof(ULWord)];
			mAVTextureBuffers[i].audioBufferSize = NTV2_AUDIOSIZE_MAX;			// this will change each frame
		} 
		else
		{
			mAVTextureBuffers[i].audioBuffer = NULL;
			mAVTextureBuffers[i].audioBufferSize = 0; // this will change each frame
		}
		mAVCircularBuffer.Add(&mAVTextureBuffers[i]);
	}
}

void CNTV2GpuCircularBuffer::Abort()
{
	mAbort = true;
}

AVTextureBuffer* CNTV2GpuCircularBuffer::StartProduceNextBuffer()
{
	return mAVCircularBuffer.StartProduceNextBuffer();
}

void CNTV2GpuCircularBuffer::EndProduceNextBuffer()
{
	mAVCircularBuffer.EndProduceNextBuffer();
}

AVTextureBuffer* CNTV2GpuCircularBuffer::StartConsumeNextBuffer()
{
	return mAVCircularBuffer.StartConsumeNextBuffer();
}

void CNTV2GpuCircularBuffer::EndConsumeNextBuffer()
{
	mAVCircularBuffer.EndConsumeNextBuffer();
}

