// CUDA array transfer class

#ifndef _NTV2_CUDA_ARRAY_TRANSFER_NV_
#define _NTV2_CUDA_ARRAY_TRANSFER_NV_

#include "ntv2gpuTextureTransferNV.h"

#define TIME_MEASUREMENTS

class CNTV2cudaArrayTransferNV : public CNTV2gpuTextureTransferNV{
public:
	CNTV2cudaArrayTransferNV();
	~CNTV2cudaArrayTransferNV();

	bool Init();
	void Destroy();

	void RegisterTexture(CNTV2Texture* texture) const;

	void UnregisterTexture(CNTV2Texture* texture) const;

	void UnregisterInputBuffer(uint8_t* buffer) const;
	void UnregisterOutputBuffer(uint8_t* buffer) const;

	void BeforeRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const;
	void AfterRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const;

	void BeforePlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const;
	void AfterPlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const;

	float GetCardToGpuTime(const CNTV2Texture* texture) const;

	float GetGpuToCardTime(const CNTV2Texture* texture) const;

private:
	typedef struct TimeInfo{

		SyncInfo cardToSysMemStart;
		SyncInfo cardToSysMemEnd;
		SyncInfo sysMemToGpuStart;
		SyncInfo sysMemToGpuEnd;
		SyncInfo gpuToSysMemStart;
		SyncInfo gpuToSysMemEnd;
		SyncInfo sysMemToCardStart;
		SyncInfo sysMemToCardEnd;
		float cardToGpuTime;
		float gpuToCardTime;
	};

	mutable std::map<GLuint, TimeInfo*> _bufferTimeInfoMap;

	BufferDVPInfo* GetBufferDVPInfo(uint8_t *buffer) const;

	TimeInfo* GetTimeInfo(const CNTV2Texture* texture) const;

	void CopyNextChunkBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const;
	void CopyBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const;
	void CopyNextChunkTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const;
	void CopyTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const;

	CUcontext ctx;
};

CNTV2cudaArrayTransferNV *CreateNTV2cudaArrayTransferNV();

#endif

