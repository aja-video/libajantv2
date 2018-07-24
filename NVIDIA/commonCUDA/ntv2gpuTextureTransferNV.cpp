/*
  This software is provided by AJA Video, Inc. "AS IS"
  with no express or implied warranties.
*/

#include "ntv2gpuTextureTransferNV.h"
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

#define MEM_RD32(a) (*(const volatile unsigned int *)(a))
#define MEM_WR32(a, d) do { *(volatile unsigned int *)(a) = (d); } while (0)

CNTV2gpuTextureTransferNV::CNTV2gpuTextureTransferNV() :
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

CNTV2gpuTextureTransferNV::~CNTV2gpuTextureTransferNV()
{
}


//dvpMemcpy functions must be encapsulated with the dvp begin and end calls
//for optimal performance, call these once per thread instead of every frame
//using the InitTransfers and DeinitTransfers methods.

void CNTV2gpuTextureTransferNV::ThreadPrep()
{
	DVP_SAFE_CALL(dvpBegin());
}

void CNTV2gpuTextureTransferNV::ThreadCleanup()
{
	DVP_SAFE_CALL(dvpEnd());
}

ULWord CNTV2gpuTextureTransferNV::GetBufferSize() const
{
	// Here, we assume RGBA, so four bytes per pixel.
	return 4 * sizeof(uint8_t) * _width * _height;
}

ULWord CNTV2gpuTextureTransferNV::GetNumChunks() const
{
	return _numChunks;

}
void CNTV2gpuTextureTransferNV::SetNumChunks(ULWord numChunks)
{
	_numChunks = numChunks;
}

void CNTV2gpuTextureTransferNV::SetSize(ULWord width, ULWord height)
{
	_width = width;
	_height = height;
}

void CNTV2gpuTextureTransferNV::ModifyTransferStructForRecord(
	AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const
{
	transferStruct->transferFlags = 0;
	transferStruct->videoBufferSize = GetBufferSize();
}

void CNTV2gpuTextureTransferNV::ModifyTransferStructForPlayback(
	AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const
{
	transferStruct->transferFlags = 0;
	transferStruct->videoBufferSize = GetBufferSize();
}

void CNTV2gpuTextureTransferNV::InitSyncInfo(SyncInfo *si) const
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

BufferDVPInfo* CNTV2gpuTextureTransferNV::GetBufferDVPInfo(uint8_t *buffer) const
{
	map<uint8_t*, BufferDVPInfo*>::iterator itr = _dvpInfoMap.find(buffer);

	return itr->second;
}

void CNTV2gpuTextureTransferNV::RegisterInputBuffer(uint8_t* buffer) const
{
	this->GetBufferDVPInfo( buffer );
}

void CNTV2gpuTextureTransferNV::RegisterOutputBuffer(uint8_t* buffer) const
{
	this->GetBufferDVPInfo( buffer );
}


DVPBufferHandle CNTV2gpuTextureTransferNV::GetBufferHandleForTexture(CNTV2Texture* texture) const
{
	map<GLuint, DVPBufferHandle>::iterator itr = _bufferHandleMap.find(texture->GetIndex());
	if( itr == _bufferHandleMap.end() )
	{
		assert(false);
		return 0;
	}
	return itr->second;
}

void CNTV2gpuTextureTransferNV::WaitForGpuDma(uint8_t *buffer) const
{
	BufferDVPInfo* info = this->GetBufferDVPInfo( buffer );
	if ( info->gpuSyncInfo.acquireValue )
	{
		DVP_SAFE_CALL(dvpSyncObjClientWaitPartial(
			info->gpuSyncInfo.syncObj, info->gpuSyncInfo.acquireValue, DVP_TIMEOUT_IGNORED));
	}
}

void CNTV2gpuTextureTransferNV::SignalSysMemDmaFinished(uint8_t *buffer) const
{
	BufferDVPInfo* info = this->GetBufferDVPInfo( buffer );
	info->sysMemSyncInfo.releaseValue++;
	info->gpuSyncInfo.acquireValue++;
	MEM_WR32(info->sysMemSyncInfo.sem, info->sysMemSyncInfo.releaseValue);
}


// Functions below to be called in this fashion:
// _dvpTransfer->AcquireTexture(texture);
// ...GL code that uses texture...
// _dvpTransfer->ReleaseTexture(texture);

void CNTV2gpuTextureTransferNV::AcquireTexture(CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	DVP_SAFE_CALL(dvpMapBufferWaitAPI(textureBufferHandle));
}

void CNTV2gpuTextureTransferNV::ReleaseTexture(CNTV2Texture* texture) const
{
	DVPBufferHandle textureBufferHandle = GetBufferHandleForTexture(texture);
	DVP_SAFE_CALL(dvpMapBufferEndAPI(textureBufferHandle));
}


