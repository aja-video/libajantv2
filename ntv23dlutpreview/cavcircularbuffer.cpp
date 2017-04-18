//
// Copyright (C) 2012 AJA Video Systems, Inc.
// Proprietary and Confidential information.
//
#include "cavcircularbuffer.h"
#include <string.h>


#define AJA_AUDIOSIZE_MAX (0x100000)
#include <assert.h>

CAVCircularBuffer::CAVCircularBuffer() :
    _abort(false), _numFrames(0)
{

}

CAVCircularBuffer::~CAVCircularBuffer()
{

    if ( _avBuffers )
    {
        for ( uint32_t i=0; i<_numFrames; i++ )
        {
            if(_avBuffers[i].videoBuffer)
            {
                delete _avBuffers[i].videoBufferUnaligned;
                _avBuffers[i].videoBufferUnaligned = NULL;
                _avBuffers[i].videoBuffer = NULL;
            }
            if( _avBuffers[i].audioBuffer)
            {
                delete _avBuffers[i].audioBuffer;
                _avBuffers[i].audioBuffer = NULL;
            }

        }

        if ( _avBuffers )
            delete [] _avBuffers;

        _avBuffers = NULL;
    }
}

void CAVCircularBuffer::Allocate(uint32_t numFrames,
                               AJA_PixelFormat pixelFormat,
                               uint32_t videoWriteSize,
                               uint32_t width,
                               uint32_t height,
                               bool withAudio,
                               size_t alignment)
{
	assert( _numFrames == 0 );
	assert( numFrames > 0 );
	assert( videoWriteSize >=0 && videoWriteSize < 1e8 );
	assert( width > 0 && width < 1e6 );
	assert( height > 0 && height < 1e6 );
	
	_numFrames = numFrames;
	
	_avCircularBuffer.SetAbortFlag(&_abort);

    _avBuffers = new AJAAVBuffer[_numFrames];
    memset(_avBuffers, 0, sizeof(_avBuffers)*_numFrames);

	for ( ULWord i=0; i<_numFrames; i++ )
	{	
        _avBuffers[i].width = width;
        _avBuffers[i].height = height;
        _avBuffers[i].pixelFormat = pixelFormat;

        _avBuffers[i].videoBufferSize = videoWriteSize;
        _avBuffers[i].videoBufferUnaligned = new UByte[videoWriteSize + alignment - 1];
        uint64_t val = (uint64_t)(_avBuffers[i].videoBufferUnaligned);
        val += alignment-1;
        val &= ~(alignment-1);
        _avBuffers[i].videoBuffer = (uint8_t*) val;

        if ( withAudio )
        {
            _avBuffers[i].audioBuffer = new uint8_t[AJA_AUDIOSIZE_MAX];
            _avBuffers[i].audioBufferSize = AJA_AUDIOSIZE_MAX;			// this can change each frame
        }
        else
        {
            _avBuffers[i].audioBuffer = NULL;
            _avBuffers[i].audioBufferSize = 0; // this will change each frame
        }
        _avBuffers[i].width = width;
        _avBuffers[i].height = height;
        _avCircularBuffer.Add(&_avBuffers[i]);
	}
}

void CAVCircularBuffer::Abort()
{
	_abort = true;
}

AJAAVBuffer* CAVCircularBuffer::StartProduceNextBuffer()
{
	return _avCircularBuffer.StartProduceNextBuffer();
}

void CAVCircularBuffer::EndProduceNextBuffer()
{
	_avCircularBuffer.EndProduceNextBuffer();
}

AJAAVBuffer* CAVCircularBuffer::StartConsumeNextBuffer()
{
	return _avCircularBuffer.StartConsumeNextBuffer();
}

void CAVCircularBuffer::EndConsumeNextBuffer()
{
	_avCircularBuffer.EndConsumeNextBuffer();
}

