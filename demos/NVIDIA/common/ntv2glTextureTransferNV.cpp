/* SPDX-License-Identifier: MIT */
/*
  This software is provided by AJA Video, Inc. "AS IS"
  with no express or implied warranties.
*/


#include "ntv2glTextureTransferNV.h"
#include "gpustuff/include/DVPAPI.h"
#include "gpustuff/include/dvpapi_gl.h"
#include <assert.h>
#include <string>
#include <map>

#if defined(AJALinux)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

using namespace std;

#define TIME_MEASUREMENTS


struct TimeInfo{
	uint64_t cardToSysMemStart;
	uint64_t cardToSysMemEnd;
	uint64_t sysMemToGpuStart;
	uint64_t sysMemToGpuEnd;
	uint64_t gpuToSysMemStart;	
	uint64_t gpuToSysMemEnd;	
	uint64_t sysMemToCardStart;
	uint64_t sysMemToCardEnd;
	float cardToGpuTime;
	float gpuToCardTime;
};


struct SyncInfo {
    volatile uint32_t *sem;
    volatile uint32_t *semOrg;
    volatile uint32_t releaseValue;
    volatile uint32_t acquireValue;
    DVPSyncObjectHandle syncObj;
};

struct BufferDVPInfo {
	DVPBufferHandle handle;
	SyncInfo sysMemSyncInfo;
	SyncInfo gpuSyncInfo;
	uint32_t currentChunk;
};

class CNTV2glTextureTransferNV : public CNTV2glTextureTransfer{
public:
	CNTV2glTextureTransferNV();
	virtual ~CNTV2glTextureTransferNV();
	
	virtual bool Init();
	virtual void Destroy();
	
	virtual void ThreadPrep(); //this has to be called in the thread where the transfers will be performed
	virtual void ThreadCleanup();//this has to be called in the thread where the transfers will be performed

	virtual void ModifyTransferStructForRecord(AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const;
	virtual void ModifyTransferStructForPlayback(AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const;

	virtual void RegisterTexture(CNTV2Texture* texture) const;
	virtual void RegisterInputBuffer(uint8_t* buffer) const;
	virtual void RegisterOutputBuffer(uint8_t* buffer) const;

	virtual void UnregisterTexture(CNTV2Texture* texture) const;

	virtual void UnregisterInputBuffer(uint8_t* buffer) const;
	virtual void UnregisterOutputBuffer(uint8_t* buffer) const;

	virtual void BeforeRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const;
	virtual void AfterRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const;

	virtual void BeforePlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const;
	virtual void AfterPlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const;


	virtual void AcquireTexture(CNTV2Texture* texture) const;
	virtual void ReleaseTexture(CNTV2Texture* texture) const;

	virtual ULWord GetNumChunks() const;
	virtual void SetNumChunks(ULWord numChunks);
	
	virtual void SetSize(ULWord width, ULWord height);

	virtual float GetCardToGpuTime(const CNTV2Texture* texture) const;

	virtual float GetGpuToCardTime(const CNTV2Texture* texture) const;

private:
	uint32_t _bufferAddrAlignment;
	uint32_t _bufferGPUStrideAlignment;
	uint32_t _semaphoreAddrAlignment;
	uint32_t _semaphoreAllocSize;
	uint32_t _semaphorePayloadOffset;
	uint32_t _semaphorePayloadSize;
	uint32_t _numChunks; //specifies the number of chunks used in the transfers. Used for overlapped GPU and Video I/O transfers

	mutable std::map<uint8_t*, BufferDVPInfo*> _dvpInfoMap;
	mutable std::map<GLuint, DVPBufferHandle> _bufferHandleMap;

	mutable std::map<GLuint, TimeInfo*> _bufferTimeInfoMap;

	void WaitForGpuDma(uint8_t *buffer) const;
	void SignalSysMemDmaFinished(uint8_t *buffer) const;

	virtual void CopyBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const;
	virtual void CopyTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const;

	virtual void CopyNextChunkBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const;
	virtual void CopyNextChunkTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const;
	
	BufferDVPInfo* GetBufferDVPInfo(uint8_t *buffer) const;
	TimeInfo* GetTimeInfo(const CNTV2Texture* texture) const;
	void InitSyncInfo(SyncInfo *si) const;
	
	DVPBufferHandle GetBufferHandleForTexture(CNTV2Texture* texture) const;
	ULWord GetBufferSize() const;	

	ULWord _width;
	ULWord _height;
};

CNTV2glTextureTransfer *CreateNTV2glTextureTransferNV()
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



CNTV2glTextureTransferNV::CNTV2glTextureTransferNV() :
	_bufferAddrAlignment(4096),
	_bufferGPUStrideAlignment(0),
	_semaphoreAddrAlignment(0),
	_semaphoreAllocSize(0),
	_semaphorePayloadSize(0),
	_numChunks(1),
	_width(0),
	_height(0)
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

	DVP_SAFE_CALL(dvpCloseGLContext());
}


//dvpMemcpy functions must be encapsulated with the dvp begin and end calls
//for optimal performance, call these once per thread instead of every frame
//using the InitTransfers and DeinitTransfers methods.

void CNTV2glTextureTransferNV::ThreadPrep() 
{
	DVP_SAFE_CALL(dvpBegin());
}

void CNTV2glTextureTransferNV::ThreadCleanup()
{
	DVP_SAFE_CALL(dvpEnd());
}

ULWord CNTV2glTextureTransferNV::GetBufferSize() const
{
	// Here, we assume RGBA, so four bytes per pixel.
	return 4 * sizeof(uint8_t) * _width * _height;
}

ULWord CNTV2glTextureTransferNV::GetNumChunks() const
{
	return _numChunks;

}
void CNTV2glTextureTransferNV::SetNumChunks(ULWord numChunks)
{
	_numChunks = numChunks;
}

void CNTV2glTextureTransferNV::SetSize( ULWord width, ULWord height )
{
	_width = width;
	_height = height;
}

void CNTV2glTextureTransferNV::ModifyTransferStructForRecord(
	AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const
{
	transferStruct->transferFlags = 0;
	transferStruct->videoBufferSize = GetBufferSize();
}

void CNTV2glTextureTransferNV::ModifyTransferStructForPlayback(
	AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const
{
	transferStruct->transferFlags = 0;
	transferStruct->videoBufferSize = GetBufferSize();
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
		DVP_SAFE_CALL(dvpBindToGLCtx( info->handle ));
		
		InitSyncInfo(&(info->sysMemSyncInfo));
		InitSyncInfo(&(info->gpuSyncInfo));
		
		info->currentChunk = 0;
		_dvpInfoMap[buffer] = info;
		
		return info;
	}
	else
		return itr->second;
}


void CNTV2glTextureTransferNV::InitSyncInfo(SyncInfo *si) const
{
	DVPSyncObjectDesc syncObjectDesc = {0};
    assert((_semaphoreAllocSize != 0) && (_semaphoreAddrAlignment != 0));
    si->semOrg = (uint32_t *) malloc(_semaphoreAllocSize+_semaphoreAddrAlignment-1);
	
	// Correct alignment

	uint64_t val = (uint64_t)si->semOrg;
	val += _semaphoreAddrAlignment - 1;
	val &= ~((uint64_t)_semaphoreAddrAlignment - 1);
	si->sem = (uint32_t *)val;


    // Initialise description
    MEM_WR32(si->sem, 0);
    si->releaseValue = 0;
    si->acquireValue = 0;
    syncObjectDesc.externalClientWaitFunc = NULL;
	syncObjectDesc.flags = 0;
    syncObjectDesc.sem = (uint32_t *)si->sem;
	
    DVP_SAFE_CALL(dvpImportSyncObject(&syncObjectDesc, &si->syncObj));
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

void CNTV2glTextureTransferNV::RegisterInputBuffer(uint8_t* buffer) const
{
	GetBufferDVPInfo( buffer );
}

void CNTV2glTextureTransferNV::RegisterOutputBuffer(uint8_t* buffer) const
{
	GetBufferDVPInfo( buffer );
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

float CNTV2glTextureTransferNV::GetCardToGpuTime(const CNTV2Texture* texture) const
{
	TimeInfo *info = GetTimeInfo(texture);
	if(info == 0)
	{
		return 0;
	}
	return info->cardToGpuTime*1000;
}

float CNTV2glTextureTransferNV::GetGpuToCardTime(const CNTV2Texture* texture) const
{
	TimeInfo *info = GetTimeInfo(texture);
	if(info == 0)
	{
		return 0;
	}
	return info->gpuToCardTime*1000;
}

TimeInfo* CNTV2glTextureTransferNV::GetTimeInfo(const CNTV2Texture* texture) const
{
	map<GLuint, TimeInfo*>::iterator itr = _bufferTimeInfoMap.find(texture->GetIndex());
	if( itr == _bufferTimeInfoMap.end() )
	{
		assert(false);
		return 0;
	}
	
	return itr->second;
}

DVPBufferHandle CNTV2glTextureTransferNV::GetBufferHandleForTexture(CNTV2Texture* texture) const
{
	map<GLuint, DVPBufferHandle>::iterator itr = _bufferHandleMap.find(texture->GetIndex());
	if( itr == _bufferHandleMap.end() )
	{
		assert(false);
		return 0;
	}
	
	return itr->second;
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
		glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->sysMemToGpuStart);
		assert(glGetError() == GL_NO_ERROR);
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
	glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->sysMemToGpuStart);
	assert(glGetError() == GL_NO_ERROR);
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
		glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->gpuToSysMemStart);
		assert(glGetError() == GL_NO_ERROR);
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
	glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->gpuToSysMemStart);
	assert(glGetError() == GL_NO_ERROR);	
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
			glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->cardToSysMemStart);
			assert(glGetError() == GL_NO_ERROR);
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
		glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->cardToSysMemEnd);
		assert(glGetError() == GL_NO_ERROR);
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
			glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->sysMemToCardStart);
			assert(glGetError() == GL_NO_ERROR);
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
		glGetInteger64v(GL_CURRENT_TIME_NV,(GLint64 *)&timeinfo->sysMemToCardEnd);
		assert(glGetError() == GL_NO_ERROR);
	}
#endif

	info->sysMemSyncInfo.releaseValue++;
	info->gpuSyncInfo.acquireValue++;
	MEM_WR32(info->sysMemSyncInfo.sem, info->sysMemSyncInfo.releaseValue);
}

void CNTV2glTextureTransferNV::WaitForGpuDma(uint8_t *buffer) const
{
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	if ( info->gpuSyncInfo.acquireValue )
	{
		DVP_SAFE_CALL(dvpSyncObjClientWaitPartial(
			info->gpuSyncInfo.syncObj, info->gpuSyncInfo.acquireValue, DVP_TIMEOUT_IGNORED));
	}
}

void CNTV2glTextureTransferNV::SignalSysMemDmaFinished(uint8_t *buffer) const
{
	BufferDVPInfo* info = GetBufferDVPInfo( buffer );
	info->sysMemSyncInfo.releaseValue++;
	info->gpuSyncInfo.acquireValue++;
	MEM_WR32(info->sysMemSyncInfo.sem, info->sysMemSyncInfo.releaseValue);
}


// Functions below to be called in this fashion:
// _dvpTransfer->AcquireTexture(texture);
// ...GL code that uses texture...
// _dvpTransfer->ReleaseTexture(texture);

void CNTV2glTextureTransferNV::AcquireTexture(CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	DVP_SAFE_CALL(dvpMapBufferWaitAPI(textureBufferHandle));
}

void CNTV2glTextureTransferNV::ReleaseTexture(CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	DVP_SAFE_CALL(dvpMapBufferEndAPI(textureBufferHandle));
}


