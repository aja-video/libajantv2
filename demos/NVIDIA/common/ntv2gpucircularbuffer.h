/* SPDX-License-Identifier: MIT */
//
// Copyright (C) 2012 AJA Video Systems, Inc.
// Proprietary and Confidential information.
//
#ifndef NTV2GPUCIRCULARBUFFER_H
#define NTV2GPUCIRCULARBUFFER_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2rp188.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include "ntv2audiodefines.h"
#include "ajabase/common/circularbuffer.h"

#include "ntv2texture.h"
#include "ntv2rendertotexture.h"

typedef struct AVTextureBuffer {
	CNTV2Texture*			texture;
	CNTV2RenderToTexture*	renderToTexture;
	ULWord*					videoBuffer;
	ULWord					videoBufferSize;
	ULWord*					audioBuffer;
	ULWord					audioBufferSize;
	ULWord					audioRecordSize;
	RP188_STRUCT			rp188Data;
	RP188_STRUCT			rp188Data2;
	
	UByte*			videoBufferUnaligned;
	int64_t			currentTime;

} AVTextureBuffer;

class CNTV2GpuCircularBuffer
{
public:
	CNTV2GpuCircularBuffer();
	virtual ~CNTV2GpuCircularBuffer();
	
	void Allocate(ULWord numFrames, ULWord videoWriteSize,
		ULWord width, ULWord height, bool withAudio, bool withRenderToTexture,
		size_t alignment);
	
	void Abort();
	
	AVTextureBuffer* StartProduceNextBuffer();
	void EndProduceNextBuffer();
	AVTextureBuffer* StartConsumeNextBuffer();
	void EndConsumeNextBuffer();


	AVTextureBuffer* mAVTextureBuffers;
private:
	ULWord mNumFrames;
	bool mAbort;
	AJACircularBuffer<AVTextureBuffer*> mAVCircularBuffer;
	
};

#endif 

