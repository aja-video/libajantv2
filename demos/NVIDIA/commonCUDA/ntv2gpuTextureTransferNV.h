/* SPDX-License-Identifier: MIT */
/*
  This software is provided by AJA Video, Inc. "AS IS"
  with no express or implied warranties.
*/

#ifndef _NTV2_GPU_TEXTURE_TRANSFER_NV_
#define _NTV2_GPU_TEXTURE_TRANSFER_NV_

#include "ntv2gpuTextureTransfer.h"

#include "cuda_runtime_api.h"
#include "cuda_gl_interop.h"

class CNTV2gpuTextureTransferNV : public CNTV2gpuTextureTransfer{
public:
	CNTV2gpuTextureTransferNV();
	virtual ~CNTV2gpuTextureTransferNV();

	// Subclasses should override to do any initialization.
	// Caller should call Init() before anything else.
	virtual bool Init() = 0;

	// Subclasses override to undo the work of Init().
	// Caller should call Destroy() before the end of the life of the object.
	virtual void Destroy() = 0;

	virtual void ThreadPrep(); //this has to be called in the thread where the transfers will be performed
	virtual void ThreadCleanup();//this has to be called in the thread where the transfers will be performed

	virtual void ModifyTransferStructForRecord(AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const;
	virtual void ModifyTransferStructForPlayback(AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const;

	// Override for one-time, per-texture initialization.
	virtual void RegisterTexture(CNTV2Texture* texture) const = 0;

	virtual void RegisterInputBuffer(uint8_t* buffer) const;
	virtual void RegisterOutputBuffer(uint8_t* buffer) const;

	// Override for one-time, per buffer deinitialization.
	virtual void UnregisterInputBuffer(uint8_t* buffer) const = 0;
	virtual void UnregisterOutputBuffer(uint8_t* buffer) const = 0;

	// Override
	virtual void BeforeRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const = 0;
	virtual void AfterRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const = 0;

	// Override
	virtual void BeforePlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const = 0;
	virtual void AfterPlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const = 0;

	virtual void AcquireTexture(CNTV2Texture* texture) const;
	virtual void ReleaseTexture(CNTV2Texture* texture) const;

	virtual ULWord GetNumChunks() const;
	virtual void SetNumChunks(ULWord numChunks);

	virtual void SetSize(ULWord width, ULWord height);

protected:
	uint32_t _bufferAddrAlignment;
	uint32_t _bufferGPUStrideAlignment;
	uint32_t _semaphoreAddrAlignment;
	uint32_t _semaphoreAllocSize;
	uint32_t _semaphorePayloadOffset;
	uint32_t _semaphorePayloadSize;
	uint32_t _numChunks; //specifies the number of chunks used in the transfers. Used for overlapped GPU and Video I/O transfers

	void WaitForGpuDma(uint8_t *buffer) const;
	void SignalSysMemDmaFinished(uint8_t *buffer) const;

	// Override to provide per scheme functionality
	virtual void CopyBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const = 0;
	virtual void CopyTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const = 0;

	// Override to provide per scheme functionality
	virtual void CopyNextChunkBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const = 0;
	virtual void CopyNextChunkTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const = 0;

	ULWord GetBufferSize() const;

	ULWord _width;
	ULWord _height;

	CNTV2TextureType _type;

//	HGLRC _glctx;
};

#endif

