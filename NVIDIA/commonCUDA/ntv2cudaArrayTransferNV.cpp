// CUDA array transfer class

#include "ntv2cudaArrayTransferNV.h"
#include "systemtime.h"
#include <assert.h>
#include <string>
#include <map>

#if defined(AJALinux)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

using namespace std;

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

static void timeStampSetup(SyncInfo *sync, DVPBufferHandle *src, DVPBufferHandle *dst)
{
	sync->releaseValue++;
	// Zero height so no blit occurs, buffer handles are ignored
	DVP_SAFE_CALL(
		dvpMemcpyLined(
		*src,
		0,
		0,
		DVP_TIMEOUT_IGNORED,
		*dst,
		sync->syncObj,
		sync->releaseValue,
		0,
		0));
}

static void timeStampAcquire(SyncInfo *sync, DVPBufferHandle *src, DVPBufferHandle *dst)
{
	if (sync->releaseValue)
	{
		sync->acquireValue++;
		// Zero height so no blit occurs, buffer handles are ignored
		DVP_SAFE_CALL(
			dvpMemcpyLined(
			*src,
			sync->syncObj,
			sync->acquireValue,
			DVP_TIMEOUT_IGNORED,
			*dst,
			0,
			0,
			0,
			0));
	}
}

CNTV2cudaArrayTransferNV *CreateNTV2cudaArrayTransferNV()
{
	return new CNTV2cudaArrayTransferNV();
}

#define MEM_RD32(a) (*(const volatile unsigned int *)(a))
#define MEM_WR32(a, d) do { *(volatile unsigned int *)(a) = (d); } while (0)

CNTV2cudaArrayTransferNV::CNTV2cudaArrayTransferNV()
{
}

CNTV2cudaArrayTransferNV::~CNTV2cudaArrayTransferNV()
{
}

bool CNTV2cudaArrayTransferNV::Init()
{

	DVP_SAFE_CALL(dvpInitCUDAContext(DVP_DEVICE_FLAGS_SHARE_APP_CONTEXT));

	DVP_SAFE_CALL(dvpGetRequiredConstantsCUDACtx(&_bufferAddrAlignment,
			      &_bufferGPUStrideAlignment,
			      &_semaphoreAddrAlignment,
			      &_semaphoreAllocSize,
			      &_semaphorePayloadSize,
			      &_semaphorePayloadSize));

	CUCHK(cuCtxGetCurrent(&ctx));

	return true;
}

void CNTV2cudaArrayTransferNV::Destroy()
{
	for (map<uint8_t*, BufferDVPInfo*>::iterator itr = _dvpInfoMap.begin();
		itr != _dvpInfoMap.end();
		itr++)
	{

		DVP_SAFE_CALL(dvpUnbindFromCUDACtx(itr->second->handle));

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

BufferDVPInfo* CNTV2cudaArrayTransferNV::GetBufferDVPInfo(uint8_t *buffer) const
{
	assert(_height > 0 && _width > 0);

	map<uint8_t*, BufferDVPInfo*>::iterator itr = _dvpInfoMap.find(buffer);

	if (itr == _dvpInfoMap.end())
	{
		BufferDVPInfo* info = new BufferDVPInfo;

		DVPSysmemBufferDesc desc;

		uint32_t bufferStride = _width * 4;
		bufferStride += _bufferGPUStrideAlignment - 1;
		bufferStride &= ~(_bufferGPUStrideAlignment - 1);

		uint32_t size = _height * bufferStride;

		desc.width = _width;
		desc.height = _height;
		desc.stride = bufferStride;
		desc.size = size;

		desc.format = DVP_RGBA;
		desc.type = DVP_UNSIGNED_BYTE;
		desc.bufAddr = buffer;

		DVP_SAFE_CALL(dvpCreateBuffer(&desc, &(info->handle)));

		DVP_SAFE_CALL(dvpBindToCUDACtx(info->handle));

		InitSyncInfo(&(info->sysMemSyncInfo));
		InitSyncInfo(&(info->gpuSyncInfo));

		info->currentChunk = 0;
		_dvpInfoMap[buffer] = info;

		return info;
	}
	else
		return itr->second;
}

void CNTV2cudaArrayTransferNV::RegisterTexture(CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle;
	
	DVP_SAFE_CALL(dvpCreateGPUCUDAArray((CUarray)texture->GetCudaArray(),
			                            &textureBufferHandle));

#ifdef TIME_MEASUREMENTS
	TimeInfo *timeInfo = new TimeInfo;
	memset(timeInfo, 0, sizeof(TimeInfo));

	InitSyncInfo(&timeInfo->cardToSysMemStart);
	InitSyncInfo(&timeInfo->cardToSysMemEnd);
	InitSyncInfo(&timeInfo->sysMemToGpuStart);
	InitSyncInfo(&timeInfo->sysMemToGpuEnd);
	InitSyncInfo(&timeInfo->gpuToSysMemStart); 
	InitSyncInfo(&timeInfo->gpuToSysMemEnd);
	InitSyncInfo(&timeInfo->sysMemToCardStart);
	InitSyncInfo(&timeInfo->sysMemToCardEnd);

	_bufferTimeInfoMap[texture->GetIndex()] = timeInfo;
#endif
	_bufferHandleMap[texture->GetIndex()] = textureBufferHandle;
}

void CNTV2cudaArrayTransferNV::UnregisterTexture(CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	TimeInfo *timeinfo = GetTimeInfo(texture);
	DVP_SAFE_CALL(dvpFreeBuffer(textureBufferHandle));
	_bufferHandleMap.erase(texture->GetIndex());
	_bufferTimeInfoMap.erase(texture->GetIndex());
	delete	timeinfo;
}

float CNTV2cudaArrayTransferNV::GetCardToGpuTime(const CNTV2Texture* texture) const
{
	TimeInfo *info = GetTimeInfo(texture);
	if (info == 0)
	{
		return 0;
	}
	return info->cardToGpuTime;
}

float CNTV2cudaArrayTransferNV::GetGpuToCardTime(const CNTV2Texture* texture) const
{
	TimeInfo *info = GetTimeInfo(texture);
	if (info == 0)
	{
		return 0;
	}
	return info->gpuToCardTime;
}

CNTV2cudaArrayTransferNV::TimeInfo* CNTV2cudaArrayTransferNV::GetTimeInfo(const CNTV2Texture* texture) const
{
	map<GLuint, TimeInfo*>::iterator itr = _bufferTimeInfoMap.find(texture->GetIndex());
	if (itr == _bufferTimeInfoMap.end())
	{
		assert(false);
		return 0;
	}
	return itr->second;
}

void CNTV2cudaArrayTransferNV::UnregisterInputBuffer(uint8_t* buffer) const
{
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	DVP_SAFE_CALL(dvpUnbindFromCUDACtx(info->handle));
	DVP_SAFE_CALL(dvpFreeBuffer(info->handle));
	DVP_SAFE_CALL(dvpFreeSyncObject(info->gpuSyncInfo.syncObj));
	DVP_SAFE_CALL(dvpFreeSyncObject(info->sysMemSyncInfo.syncObj));
		
	free((void*)(info->gpuSyncInfo.semOrg));
	free((void*)(info->sysMemSyncInfo.semOrg));
	_dvpInfoMap.erase(buffer);
	delete info;
}

void CNTV2cudaArrayTransferNV::UnregisterOutputBuffer(uint8_t* buffer) const
{
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	DVP_SAFE_CALL(dvpUnbindFromCUDACtx(info->handle));

	DVP_SAFE_CALL(dvpFreeBuffer(info->handle));
	DVP_SAFE_CALL(dvpFreeSyncObject(info->gpuSyncInfo.syncObj));
	DVP_SAFE_CALL(dvpFreeSyncObject(info->sysMemSyncInfo.syncObj));
		
	free((void*)(info->gpuSyncInfo.semOrg));
	free((void*)(info->sysMemSyncInfo.semOrg));
	_dvpInfoMap.erase(buffer);
	delete info;
}

void CNTV2cudaArrayTransferNV::CopyNextChunkBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	
	if (info->currentChunk == 0)
	{
		// Make sure the rendering API is finished using the buffer and block further usage
		DVP_SAFE_CALL(dvpMapBufferWaitDVP(textureBufferHandle));
#ifdef TIME_MEASUREMENTS
		TimeInfo *timeinfo = GetTimeInfo(texture);

		timeStampAcquire(&(timeinfo->sysMemToGpuStart), &info->handle, &textureBufferHandle);
		timeStampSetup(&(timeinfo->sysMemToGpuEnd), &info->handle, &textureBufferHandle);
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

void CNTV2cudaArrayTransferNV::CopyBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );

	// Make sure the rendering API is finished using the buffer and block further usage
	DVP_SAFE_CALL(dvpMapBufferWaitDVP(textureBufferHandle));

	// Initiate the system memory to GPU copy
	
	info->sysMemSyncInfo.acquireValue++;
	info->gpuSyncInfo.releaseValue++;

#ifdef TIME_MEASUREMENTS
	TimeInfo *timeinfo = GetTimeInfo(texture);

	timeStampAcquire(&(timeinfo->sysMemToGpuStart), &info->handle, &textureBufferHandle);
	timeStampSetup(&(timeinfo->sysMemToGpuEnd), &info->handle, &textureBufferHandle);
#endif

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

void CNTV2cudaArrayTransferNV::CopyNextChunkTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	if (info->currentChunk == 0)
	{
		// Make sure the rendering API is finished using the buffer and block further usage
		DVP_SAFE_CALL(dvpMapBufferWaitDVP(textureBufferHandle));
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
void CNTV2cudaArrayTransferNV::CopyTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	
    
	// Make sure the rendering API is finished using the buffer and block further usage
    DVP_SAFE_CALL(dvpMapBufferWaitDVP(textureBufferHandle));

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

void CNTV2cudaArrayTransferNV::BeforeRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	// Before TransferWithAutoCirculate that records to main memory,
	// we have to wait for any DMA from main memory to the GPU that
	// might need that same piece of main memory.
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );

	if ( info->gpuSyncInfo.acquireValue)
	{
		DVP_SAFE_CALL(dvpSyncObjClientWaitPartial(
			info->gpuSyncInfo.syncObj, info->gpuSyncInfo.acquireValue, DVP_TIMEOUT_IGNORED));

#ifdef TIME_MEASUREMENTS
		if(info->currentChunk == 0)
		{
			TimeInfo *timeinfo = GetTimeInfo(texture);

			timeStampSetup(&(timeinfo->cardToSysMemStart), &(info->handle), &textureBufferHandle);
			timeStampAcquire(&(timeinfo->cardToSysMemStart), &(info->handle), &textureBufferHandle);
			timeStampSetup(&(timeinfo->cardToSysMemEnd), &(info->handle), &textureBufferHandle);
		}
#endif

	}
}

void CNTV2cudaArrayTransferNV::AfterRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	// After TransferWithAutoCirculate call to record to main memory,
	// we have to signal that transfer is complete so that code that
	// waits for frame to complete can continue.
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );

#ifdef TIME_MEASUREMENTS
	TimeInfo *timeinfo = GetTimeInfo(texture);
	timeStampAcquire(&(timeinfo->cardToSysMemEnd), &info->handle, &textureBufferHandle);
	timeStampSetup(&(timeinfo->sysMemToGpuStart), &info->handle, &textureBufferHandle);
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

#ifdef TIME_MEASUREMENTS
	timeStampAcquire(&(timeinfo->sysMemToGpuEnd), &info->handle, &textureBufferHandle);

	uint64_t start = 0, end = 0;
	DVP_SAFE_CALL(dvpSyncObjCompletion(timeinfo->cardToSysMemStart.syncObj, &start));
	DVP_SAFE_CALL(dvpSyncObjCompletion(timeinfo->sysMemToGpuEnd.syncObj, &end));

	timeinfo->cardToGpuTime = float(end - start) / 1000000;
#endif
}

void CNTV2cudaArrayTransferNV::BeforePlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo(buffer);

#ifdef TIME_MEASUREMENTS
	if (info->currentChunk == 0)
	{
		TimeInfo *timeinfo = GetTimeInfo(texture);

		timeStampSetup(&(timeinfo->gpuToSysMemStart), &(info->handle), &textureBufferHandle);
		timeStampAcquire(&(timeinfo->gpuToSysMemStart), &info->handle, &textureBufferHandle);
		timeStampSetup(&(timeinfo->gpuToSysMemEnd), &info->handle, &textureBufferHandle);
	}
#endif

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
	if ( info->gpuSyncInfo.acquireValue)
	{
		DVP_SAFE_CALL(dvpSyncObjClientWaitPartial(
			info->gpuSyncInfo.syncObj, info->gpuSyncInfo.acquireValue, DVP_TIMEOUT_IGNORED));
	}

#ifdef TIME_MEASUREMENTS
	TimeInfo *timeinfo = GetTimeInfo(texture);
	if (info->currentChunk == 0)
	{
		timeStampSetup(&(timeinfo->sysMemToCardStart), &(info->handle), &textureBufferHandle);
	}
	if(info->currentChunk == _numChunks-1)
	{
		timeStampAcquire(&(timeinfo->gpuToSysMemEnd), &(info->handle), &textureBufferHandle);
		timeStampAcquire(&(timeinfo->sysMemToCardStart), &(info->handle), &textureBufferHandle);
		timeStampSetup(&(timeinfo->sysMemToCardEnd), &(info->handle), &textureBufferHandle);
	}
#endif	
}

void CNTV2cudaArrayTransferNV::AfterPlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	// After TransferWithAutoCirculate call to playback from main memory,
	// we have to signal that transfer is complete so that code that
	// waits for frame to complete can continue.
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );

#ifdef TIME_MEASUREMENTS
	if (info->currentChunk == _numChunks - 1)
	{
		TimeInfo *timeinfo = GetTimeInfo(texture);
		timeStampAcquire(&(timeinfo->sysMemToCardEnd), &info->handle, &textureBufferHandle);

		uint64_t start = 0, end = 0;
		DVP_SAFE_CALL(dvpSyncObjCompletion(timeinfo->gpuToSysMemStart.syncObj, &start));
		DVP_SAFE_CALL(dvpSyncObjCompletion(timeinfo->sysMemToCardEnd.syncObj, &end));

		timeinfo->gpuToCardTime = float(end - start) / 1000000;
	}
#endif

	info->sysMemSyncInfo.releaseValue++;
	info->gpuSyncInfo.acquireValue++;
	MEM_WR32(info->sysMemSyncInfo.sem, info->sysMemSyncInfo.releaseValue);
}
