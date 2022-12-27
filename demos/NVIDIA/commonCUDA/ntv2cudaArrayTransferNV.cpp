/* SPDX-License-Identifier: MIT */
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
	CUCHK(cuCtxGetCurrent(&ctx));

	return true;
}

void CNTV2cudaArrayTransferNV::Destroy()
{
	for( map<GLuint, TimeInfo *>::iterator itr = _bufferTimeInfoMap.begin();
		 itr != _bufferTimeInfoMap.end();
		 itr++ )
	{	
		delete itr->second;
	}
	_bufferTimeInfoMap.clear();
}

void CNTV2cudaArrayTransferNV::RegisterTexture(CNTV2Texture* texture) const
{
	TimeInfo* timeInfo = new TimeInfo;
	memset(timeInfo, 0, sizeof(TimeInfo));
	_bufferTimeInfoMap[texture->GetIndex()] = timeInfo;
}

void CNTV2cudaArrayTransferNV::UnregisterTexture(CNTV2Texture* texture) const
{
	TimeInfo* timeinfo = GetTimeInfo(texture);
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
}

void CNTV2cudaArrayTransferNV::UnregisterOutputBuffer(uint8_t* buffer) const
{
}

void CNTV2cudaArrayTransferNV::CopyNextChunkBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const
{
}

void CNTV2cudaArrayTransferNV::CopyBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const
{
	size_t pitch = texture->GetWidth() * 4;
#ifdef AJA_RDMA
	checkCudaErrors(cudaMemcpy2DToArray(texture->GetCudaArray(), 0, 0, buffer, pitch, texture->GetWidth() * 4, texture->GetHeight(), cudaMemcpyDeviceToDevice));
#else
	checkCudaErrors(cudaMemcpy2DToArray(texture->GetCudaArray(), 0, 0, buffer, pitch, texture->GetWidth() * 4, texture->GetHeight(), cudaMemcpyHostToDevice));
#endif
}

void CNTV2cudaArrayTransferNV::CopyNextChunkTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const
{
}
void CNTV2cudaArrayTransferNV::CopyTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const
{
	size_t pitch = texture->GetWidth() * 4;
#ifdef AJA_RDMA
	checkCudaErrors(cudaMemcpy2DFromArray(buffer, pitch, texture->GetCudaArray(), 0, 0, texture->GetWidth() * 4, texture->GetHeight(), cudaMemcpyDeviceToDevice));
#else
	checkCudaErrors(cudaMemcpy2DFromArray(buffer, pitch, texture->GetCudaArray(), 0, 0, texture->GetWidth() * 4, texture->GetHeight(), cudaMemcpyDeviceToHost));
#endif
}

void CNTV2cudaArrayTransferNV::BeforeRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
	//if (currentChunk == 0)
	{
		TimeInfo* timeinfo = GetTimeInfo(texture);
		timeinfo->cardToSysMemStart = AJATime::GetSystemMicroseconds();
	}
}

void CNTV2cudaArrayTransferNV::AfterRecordTransfer(uint8_t* buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
#ifdef TIME_MEASUREMENTS
	TimeInfo* timeinfo = GetTimeInfo(texture);
	timeinfo->cardToSysMemEnd = AJATime::GetSystemMicroseconds();
	timeinfo->sysMemToGpuStart = AJATime::GetSystemMicroseconds();
#endif

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
	timeinfo->sysMemToGpuEnd = AJATime::GetSystemMicroseconds();
	timeinfo->cardToGpuTime = float(timeinfo->sysMemToGpuEnd - timeinfo->cardToSysMemStart) / 1000;
#endif
}

void CNTV2cudaArrayTransferNV::BeforePlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
#ifdef TIME_MEASUREMENTS
	TimeInfo* timeinfo = GetTimeInfo(texture);
	timeinfo->gpuToSysMemStart = AJATime::GetSystemMicroseconds();
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

#ifdef TIME_MEASUREMENTS
	timeinfo->gpuToSysMemEnd = AJATime::GetSystemMicroseconds();
	timeinfo->sysMemToCardStart = AJATime::GetSystemMicroseconds();
#endif
}

void CNTV2cudaArrayTransferNV::AfterPlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const
{
#ifdef TIME_MEASUREMENTS
	TimeInfo* timeinfo = GetTimeInfo(texture);
	timeinfo->sysMemToCardEnd = AJATime::GetSystemMicroseconds();
	timeinfo->gpuToCardTime = float(timeinfo->sysMemToCardEnd - timeinfo->gpuToSysMemStart) / 1000;
#endif
}
