//
// Copyright (C) 2015 AJA Video Systems, Inc.
// Proprietary and Confidential information.
//
#ifndef AJAAVCIRCULARBUFFER_H
#define AJAAVCIRCULARBUFFER_H

#include "ajatypes.h"
#include "ajabase/common/videotypes.h"
#include "ajabase/common/circularbuffer.h"


typedef struct AJAAVBuffer {
    uint8_t*					videoBuffer;
    uint8_t*                    videoBufferUnaligned;
    uint32_t					videoBufferSize;
    uint8_t*					audioBuffer;
    uint32_t					audioBufferSize;
    uint32_t					audioRecordSize;
    uint32_t                    width;
    uint32_t                    height;
    AJA_PixelFormat       pixelFormat;
    bool                         compressedVideo;
    uint64_t                    timeStamp1;
    uint64_t                    timeStamp2;

} AJAAVBuffer;

class CAVCircularBuffer
{
public:
    CAVCircularBuffer();
    virtual ~CAVCircularBuffer();
	
    void Allocate(uint32_t numFrames,
                 AJA_PixelFormat pixelFormat,
                 uint32_t videoWriteSize,
                 uint32_t width,
                 uint32_t height,
                 bool withAudio,
                 size_t alignment);
	
	void Abort();
	
    AJAAVBuffer* StartProduceNextBuffer();
	void EndProduceNextBuffer();
    AJAAVBuffer* StartConsumeNextBuffer();
	void EndConsumeNextBuffer();



private:
	ULWord _numFrames;
	bool _abort;
    AJACircularBuffer<AJAAVBuffer*> _avCircularBuffer;
    AJAAVBuffer* _avBuffers;
};

#endif //AJAAVCIRCULARBUFFER_H

