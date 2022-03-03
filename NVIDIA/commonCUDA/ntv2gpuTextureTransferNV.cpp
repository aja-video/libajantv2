/* SPDX-License-Identifier: MIT */
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
}

void CNTV2gpuTextureTransferNV::ThreadCleanup()
{
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

void CNTV2gpuTextureTransferNV::RegisterInputBuffer(uint8_t* buffer) const
{
}

void CNTV2gpuTextureTransferNV::RegisterOutputBuffer(uint8_t* buffer) const
{
}

void CNTV2gpuTextureTransferNV::WaitForGpuDma(uint8_t *buffer) const
{
}

void CNTV2gpuTextureTransferNV::SignalSysMemDmaFinished(uint8_t *buffer) const
{
}

// Functions below to be called in this fashion:
// _dvpTransfer->AcquireTexture(texture);
// ...GL code that uses texture...
// _dvpTransfer->ReleaseTexture(texture);

void CNTV2gpuTextureTransferNV::AcquireTexture(CNTV2Texture* texture) const
{
}

void CNTV2gpuTextureTransferNV::ReleaseTexture(CNTV2Texture* texture) const
{
}


