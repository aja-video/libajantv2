/* SPDX-License-Identifier: MIT */
/*
  This software is provided by AJA Video, Inc. "AS IS"
  with no express or implied warranties.
*/


#include "ntv2glTextureTransferNV.h"
#include "ajastuff/system/systemtime.h"
#include <assert.h>
#include <string>
#include <map>

#if defined(AJALinux)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

using namespace std;

//
// Return nanosecond clock value.
//
static uint64_t GetNanoClock()
{
	LARGE_INTEGER now;
	static LARGE_INTEGER frequency;
	static int gotfrequency = 0;
	uint64_t seconds, nsec;

	QueryPerformanceCounter(&now);
	if (gotfrequency == 0) {
		QueryPerformanceFrequency(&frequency);
		gotfrequency = 1;
	}

	seconds = now.QuadPart / frequency.QuadPart;
	nsec = (1000000000I64 * (now.QuadPart - (seconds * frequency.QuadPart))) / frequency.QuadPart;

	return seconds * 1000000000I64 + nsec;
}

CNTV2glTextureTransferNV *CreateNTV2glTextureTransferNV()
{
	return new CNTV2glTextureTransferNV();
}

static void fail(DVPStatus hr)
{
    odprintf("DVP Failed with status %X\n", hr);
    exit(0);
}

#define DVP_SAFE_CALL(cmd) { \
    DVPStatus hr = (cmd); \
    if (DVP_STATUS_OK != hr) { \
        odprintf("Fail on line %d\n", __LINE__); \
        fail(hr); \
    } \
}

#define MEM_RD32(a) (*(const volatile unsigned int *)(a))
#define MEM_WR32(a, d) do { *(volatile unsigned int *)(a) = (d); } while (0)

CNTV2glTextureTransferNV::CNTV2glTextureTransferNV()
{
}

CNTV2glTextureTransferNV::~CNTV2glTextureTransferNV()
{
}

bool CNTV2glTextureTransferNV::Init()
{
	DVP_SAFE_CALL(dvpInitGLContext(0));

	DVP_SAFE_CALL(dvpGetRequiredConstantsGLCtx(&_bufferAddrAlignment,
			      &_bufferGPUStrideAlignment,
			      &_semaphoreAddrAlignment,
			      &_semaphoreAllocSize,
			      &_semaphorePayloadSize,
			      &_semaphorePayloadSize));

    /*_glctx = */wglGetCurrentContext();

	return true;
}

void CNTV2glTextureTransferNV::Destroy()
{
	for( map<uint8_t*, BufferDVPInfo*>::iterator itr = _dvpInfoMap.begin();
		 itr != _dvpInfoMap.end();
		 itr++ )
	{

		DVP_SAFE_CALL(dvpUnbindFromGLCtx(itr->second->handle));

		DVP_SAFE_CALL(dvpFreeBuffer(itr->second->handle));
		DVP_SAFE_CALL(dvpFreeSyncObject(itr->second->gpuSyncInfo.syncObj));
		DVP_SAFE_CALL(dvpFreeSyncObject(itr->second->sysMemSyncInfo.syncObj));
		
		free((void*)(itr->second->gpuSyncInfo.semOrg));
		free((void*)(itr->second->sysMemSyncInfo.semOrg));
		
		delete itr->second;
	}
	
	_dvpInfoMap.clear();

	for( map<GLuint, DVPBufferHandle>::iterator itr = _bufferHandleMap.begin();
		 itr != _bufferHandleMap.end();
		 itr++ )
	{	
		DVP_SAFE_CALL(dvpFreeBuffer(itr->second));
		
	}
	_dvpInfoMap.clear();
	for( map<GLuint, TimeInfo *>::iterator itr = _bufferTimeInfoMap.begin();
		 itr != _bufferTimeInfoMap.end();
		 itr++ )
	{	
		delete itr->second;
	}
	_bufferTimeInfoMap.clear();

	DVP_SAFE_CALL(dvpCloseCUDAContext());
}

BufferDVPInfo* CNTV2glTextureTransferNV::GetBufferDVPInfo(uint8_t *buffer) const
{
	assert( _height > 0 && _width > 0 );
	
	map<uint8_t*, BufferDVPInfo*>::iterator itr = _dvpInfoMap.find(buffer);
	
	if( itr == _dvpInfoMap.end() )
	{
		BufferDVPInfo* info = new BufferDVPInfo;
		
		DVPSysmemBufferDesc desc;
		
		uint32_t bufferStride = _width*4;
		bufferStride += _bufferGPUStrideAlignment-1;
		bufferStride &= ~(_bufferGPUStrideAlignment-1);
		
		uint32_t size = _height * bufferStride;
		
		desc.width = _width;
		desc.height = _height;
		desc.stride = bufferStride;
		desc.size = size;
		
		desc.format = DVP_RGBA;
		desc.type = DVP_UNSIGNED_BYTE;
		desc.bufAddr = buffer;
		
		DVP_SAFE_CALL(dvpCreateBuffer( &desc, &(info->handle) ));

		DVP_SAFE_CALL(dvpBindToGLCtx(info->handle));
			
		InitSyncInfo(&(info->sysMemSyncInfo));
		InitSyncInfo(&(info->gpuSyncInfo));
		
		info->currentChunk = 0;
		_dvpInfoMap[buffer] = info;
		
		return info;
	}
	else
		return itr->second;
}

void CNTV2glTextureTransferNV::RegisterTexture(CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle;
	
	DVP_SAFE_CALL(dvpCreateGPUTextureGL(
			texture->GetIndex(),
			&textureBufferHandle));

#ifdef TIME_MEASUREMENTS
	TimeInfo *timeInfo = new TimeInfo;
	memset(timeInfo, 0, sizeof(TimeInfo));
	_bufferTimeInfoMap[texture->GetIndex()] = timeInfo;
#endif
	_bufferHandleMap[texture->GetIndex()] = textureBufferHandle;
}

void CNTV2glTextureTransferNV::UnregisterTexture(CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	TimeInfo *timeinfo = GetTimeInfo(texture);
	DVP_SAFE_CALL(dvpFreeBuffer(textureBufferHandle));
	_bufferHandleMap.erase(texture->GetIndex());
	_bufferTimeInfoMap.erase(texture->GetIndex());
	delete	timeinfo;
}

float CNTV2glTextureTransferNV::GetCardToGpuTime(const CNTV2Texture* texture) const
{
	TimeInfo *info = GetTimeInfo(texture);
	if (info == 0)
	{
		return 0;
	}
	return info->cardToGpuTime * 1000;
}

float CNTV2glTextureTransferNV::GetGpuToCardTime(const CNTV2Texture* texture) const
{
	TimeInfo *info = GetTimeInfo(texture);
	if (info == 0)
	{
		return 0;
	}
	return info->gpuToCardTime * 1000;
}

CNTV2glTextureTransferNV::TimeInfo* CNTV2glTextureTransferNV::GetTimeInfo(const CNTV2Texture* texture) const
{
	map<GLuint, TimeInfo*>::iterator itr = _bufferTimeInfoMap.find(texture->GetIndex());
	if (itr == _bufferTimeInfoMap.end())
	{
		assert(false);
		return 0;
	}
	return itr->second;
}


void CNTV2glTextureTransferNV::UnregisterInputBuffer(uint8_t* buffer) const
{
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	DVP_SAFE_CALL(dvpUnbindFromGLCtx(info->handle));
	DVP_SAFE_CALL(dvpFreeBuffer(info->handle));
	DVP_SAFE_CALL(dvpFreeSyncObject(info->gpuSyncInfo.syncObj));
	DVP_SAFE_CALL(dvpFreeSyncObject(info->sysMemSyncInfo.syncObj));
		
	free((void*)(info->gpuSyncInfo.semOrg));
	free((void*)(info->sysMemSyncInfo.semOrg));
	_dvpInfoMap.erase(buffer);
	delete info;
}

void CNTV2glTextureTransferNV::UnregisterOutputBuffer(uint8_t* buffer) const
{
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	DVP_SAFE_CALL(dvpUnbindFromGLCtx(info->handle));

	DVP_SAFE_CALL(dvpFreeBuffer(info->handle));
	DVP_SAFE_CALL(dvpFreeSyncObject(info->gpuSyncInfo.syncObj));
	DVP_SAFE_CALL(dvpFreeSyncObject(info->sysMemSyncInfo.syncObj));
		
	free((void*)(info->gpuSyncInfo.semOrg));
	free((void*)(info->sysMemSyncInfo.semOrg));
	_dvpInfoMap.erase(buffer);
	delete info;
}


void CNTV2glTextureTransferNV::CopyNextChunkBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	
	if(info->currentChunk == 0)
	{
		// Make sure the rendering API is finished using the buffer and block further usage
		DVP_SAFE_CALL(dvpMapBufferWaitDVP(textureBufferHandle));

#ifdef TIME_MEASUREMENTS		
		TimeInfo *timeinfo = GetTimeInfo(texture);
#if 1
		glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->sysMemToGpuStart);
		assert(glGetError() == GL_NO_ERROR);
#else
		// Convert to nanoseconds
		timeinfo->sysMemToGpuStart = GetNanoClock(); // AJATime::GetSystemMilliseconds() * 1000;
#endif
#endif

	}

	const uint32_t numLinesPerCopy = (uint32_t)((float)_height/(float)_numChunks);
	ULWord copiedLines = info->currentChunk*numLinesPerCopy;
	
	// Initiate the system memory to GPU copy

	uint32_t linesRemaining = _height-copiedLines;
	uint32_t linesToCopy = (linesRemaining > numLinesPerCopy ? numLinesPerCopy : linesRemaining);
		
	info->sysMemSyncInfo.acquireValue++;
	info->gpuSyncInfo.releaseValue++;
	
	DVP_SAFE_CALL(
		dvpMemcpyLined(
		info->handle,
		info->sysMemSyncInfo.syncObj,
		info->sysMemSyncInfo.acquireValue,
		DVP_TIMEOUT_IGNORED,
		textureBufferHandle,
		info->gpuSyncInfo.syncObj,
		info->gpuSyncInfo.releaseValue,
		copiedLines,
		linesToCopy));
	
	copiedLines += linesToCopy;
	info->currentChunk++;
	if(info->currentChunk == _numChunks)
	{
		DVP_SAFE_CALL(dvpMapBufferEndDVP(textureBufferHandle));
		info->currentChunk = 0;
	}
}
void CNTV2glTextureTransferNV::CopyBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	
	// Make sure the rendering API is finished using the buffer and block further usage
	DVP_SAFE_CALL(dvpMapBufferWaitDVP(textureBufferHandle));

#ifdef TIME_MEASUREMENTS
	TimeInfo *timeinfo = GetTimeInfo(texture);
#if 1
	glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->sysMemToGpuStart);
	GLenum val = GL_NO_ERROR;
	val = glGetError();
	//assert(glGetError() == GL_NO_ERROR);
#else
	// Convert to nanoseconds
	timeinfo->sysMemToGpuStart = GetNanoClock(); // AJATime::GetSystemMilliseconds() * 1000;
#endif
#endif


	// Initiate the system memory to GPU copy
	
	info->sysMemSyncInfo.acquireValue++;
	info->gpuSyncInfo.releaseValue++;
	
	DVP_SAFE_CALL(
		dvpMemcpyLined(
		info->handle,
		info->sysMemSyncInfo.syncObj,
		info->sysMemSyncInfo.acquireValue,
		DVP_TIMEOUT_IGNORED,
		textureBufferHandle,
		info->gpuSyncInfo.syncObj,
		info->gpuSyncInfo.releaseValue,
		0,
		_height));
		
	
	DVP_SAFE_CALL(dvpMapBufferEndDVP(textureBufferHandle));
}


void CNTV2glTextureTransferNV::CopyNextChunkTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	if(info->currentChunk == 0)
	{
		// Make sure the rendering API is finished using the buffer and block further usage
		DVP_SAFE_CALL(dvpMapBufferWaitDVP(textureBufferHandle));


#ifdef TIME_MEASUREMENTS
		TimeInfo *timeinfo = GetTimeInfo(texture);
		DVP_SAFE_CALL(dvpSyncObjCompletion(info->gpuSyncInfo.syncObj,&timeinfo->gpuToSysMemEnd));
		timeinfo->gpuToCardTime = (float)((timeinfo->sysMemToCardEnd - timeinfo->gpuToSysMemStart)*.000000001);
#if 1
		glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->gpuToSysMemStart);
		assert(glGetError() == GL_NO_ERROR);
#else
		// Convert to nanoseconds
		timeinfo->gpuToSysMemStart = GetNanoClock(); // AJATime::GetSystemMilliseconds() * 1000;
#endif
#endif

	}

	const uint32_t numLinesPerCopy = (uint32_t)((float)_height/(float)_numChunks);
	ULWord copiedLines = info->currentChunk*numLinesPerCopy;
	
	// Initiate the GPU to system memory copy

	uint32_t linesRemaining = _height-copiedLines;
	uint32_t linesToCopy = (linesRemaining > numLinesPerCopy ? numLinesPerCopy : linesRemaining);
		
	
	info->gpuSyncInfo.releaseValue++;
	
    DVP_SAFE_CALL(
			dvpMemcpyLined(
				textureBufferHandle,
				info->sysMemSyncInfo.syncObj,
				info->sysMemSyncInfo.acquireValue,
				DVP_TIMEOUT_IGNORED,
				info->handle,
				info->gpuSyncInfo.syncObj,
				info->gpuSyncInfo.releaseValue,
				copiedLines,
				linesToCopy));
	info->sysMemSyncInfo.acquireValue++;
	copiedLines += linesToCopy;
	info->currentChunk++;
	if(info->currentChunk == _numChunks)
	{
		DVP_SAFE_CALL(dvpMapBufferEndDVP(textureBufferHandle));
		info->currentChunk = 0;
	}

}
void CNTV2glTextureTransferNV::CopyTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	
    
	// Make sure the rendering API is finished using the buffer and block further usage
    DVP_SAFE_CALL(dvpMapBufferWaitDVP(textureBufferHandle));

#ifdef TIME_MEASUREMENTS
	TimeInfo *timeinfo = GetTimeInfo(texture);
	DVP_SAFE_CALL(dvpSyncObjCompletion(info->gpuSyncInfo.syncObj,&timeinfo->gpuToSysMemEnd));
	timeinfo->gpuToCardTime = (float)((timeinfo->sysMemToCardEnd - timeinfo->gpuToSysMemStart)*.000000001);
#if 1
	glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->gpuToSysMemStart);
	assert(glGetError() == GL_NO_ERROR);	
#else
	// Convert to nanoseconds
	timeinfo->gpuToSysMemStart = GetNanoClock(); // AJATime::GetSystemMilliseconds() * 1000;
#endif
#endif

	// Initiate the GPU to system memory copy
 	
    info->gpuSyncInfo.releaseValue++;
	
    DVP_SAFE_CALL(
		dvpMemcpyLined(
			textureBufferHandle,
			info->sysMemSyncInfo.syncObj,
			info->sysMemSyncInfo.acquireValue,
			DVP_TIMEOUT_IGNORED,
			info->handle,
			info->gpuSyncInfo.syncObj,
			info->gpuSyncInfo.releaseValue,
			0,
			_height));

	info->sysMemSyncInfo.acquireValue++;
	
    DVP_SAFE_CALL(dvpMapBufferEndDVP(textureBufferHandle));    
}

void CNTV2glTextureTransferNV::BeforeRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	// Before TransferWithAutoCirculate that records to main memory,
	// we have to wait for any DMA from main memory to the GPU that
	// might need that same piece of main memory.
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	if ( info->gpuSyncInfo.acquireValue)
	{
		DVP_SAFE_CALL(dvpSyncObjClientWaitPartial(
			info->gpuSyncInfo.syncObj, info->gpuSyncInfo.acquireValue, DVP_TIMEOUT_IGNORED));

#ifdef TIME_MEASUREMENTS
		if(info->currentChunk == 0)
		{
			TimeInfo *timeinfo = GetTimeInfo(texture);
			DVP_SAFE_CALL(dvpSyncObjCompletion(info->gpuSyncInfo.syncObj,&timeinfo->sysMemToGpuEnd));
			timeinfo->cardToGpuTime = (float)((timeinfo->sysMemToGpuEnd - timeinfo->cardToSysMemStart)*.000000001);
#if 0
			glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->cardToSysMemStart);
			assert(glGetError() == GL_NO_ERROR);
#else
			// Convert to nanoseconds
			timeinfo->cardToSysMemStart = GetNanoClock(); // AJATime::GetSystemMilliseconds() * 1000;
#endif
		}
#endif

	}
}

void CNTV2glTextureTransferNV::AfterRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	// After TransferWithAutoCirculate call to record to main memory,
	// we have to signal that transfer is complete so that code that
	// waits for frame to complete can continue.
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );

#ifdef TIME_MEASUREMENTS
	if ( info->gpuSyncInfo.acquireValue )
	{
		TimeInfo *timeinfo = GetTimeInfo(texture);
#if 0
		glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->cardToSysMemEnd);
		assert(glGetError() == GL_NO_ERROR);
#else
		// Convert to nanoseconds
		timeinfo->cardToSysMemEnd = GetNanoClock(); // AJATime::GetSystemMilliseconds() * 1000;
#endif
	}
#endif

	info->sysMemSyncInfo.releaseValue++;
	info->gpuSyncInfo.acquireValue++;
	MEM_WR32(info->sysMemSyncInfo.sem, info->sysMemSyncInfo.releaseValue);

	// Also we copy within the current frame of the circular buffer,
	// from main memory to the texture.
	if(_numChunks > 1)
	{
		CopyNextChunkBufferToTexture(buffer, texture); //this is a partial copy
	}
	else
	{
		CopyBufferToTexture(buffer, texture); 
	}
}

void CNTV2glTextureTransferNV::BeforePlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	// Before TransferWithAutoCirculate to playback, we need to copy
	// the texture from the receiving texture to main memory.
	if(_numChunks > 1)
	{
		CopyNextChunkTextureToBuffer(texture, buffer); //this is a partial copy
	}
	else
	{
		CopyTextureToBuffer(texture, buffer);
	}
	
	// Then wait for the buffer to become available.
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	if ( info->gpuSyncInfo.acquireValue)
	{
		DVP_SAFE_CALL(dvpSyncObjClientWaitPartial(
			info->gpuSyncInfo.syncObj, info->gpuSyncInfo.acquireValue, DVP_TIMEOUT_IGNORED));

#ifdef TIME_MEASUREMENTS
		if(info->currentChunk == 0)
		{
			TimeInfo *timeinfo = GetTimeInfo(texture);
#if 1
			glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->sysMemToCardStart);
			assert(glGetError() == GL_NO_ERROR);
#else
            // Convert to nanoseconds
			timeinfo->sysMemToCardStart = GetNanoClock(); // AJATime::GetSystemMilliseconds() * 1000;
#endif
		}
#endif

	}
	
}

void CNTV2glTextureTransferNV::AfterPlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	// After TransferWithAutoCirculate call to playback from main memory,
	// we have to signal that transfer is complete so that code that
	// waits for frame to complete can continue.
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );

#ifdef TIME_MEASUREMENTS
	if ( info->gpuSyncInfo.acquireValue)
	{
		TimeInfo *timeinfo = GetTimeInfo(texture);
#if 1
		glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->sysMemToCardEnd);
		assert(glGetError() == GL_NO_ERROR);
#else
		// Convert to nanoseconds
		timeinfo->sysMemToCardEnd = GetNanoClock(); // AJATime::GetSystemMilliseconds() * 1000;
#endif
	}
#endif

	info->sysMemSyncInfo.releaseValue++;
	info->gpuSyncInfo.acquireValue++;
	MEM_WR32(info->sysMemSyncInfo.sem, info->sysMemSyncInfo.releaseValue);
}

