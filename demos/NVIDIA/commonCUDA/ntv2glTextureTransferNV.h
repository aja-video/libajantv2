/* SPDX-License-Identifier: MIT */
/*
  This software is provided by AJA Video, Inc. "AS IS"
  with no express or implied warranties.
*/

#ifndef _NTV2_GL_TEXTURE_TRANSFER_NV_
#define _NTV2_GL_TEXTURE_TRANSFER_NV_

#include "ntv2gpuTextureTransferNV.h"

#define TIME_MEASUREMENTS

class CNTV2glTextureTransferNV : public CNTV2gpuTextureTransferNV{
public:
	CNTV2glTextureTransferNV(NTV2TextureType __type);
	CNTV2glTextureTransferNV();
	~CNTV2glTextureTransferNV();

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

	BufferDVPInfo* GetBufferDVPInfo(uint8_t *buffer) const;

	TimeInfo* GetTimeInfo(const CNTV2Texture* texture) const;

	mutable std::map<GLuint, TimeInfo*> _bufferTimeInfoMap;

	void CopyNextChunkBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const;
	void CopyBufferToTexture(uint8_t* buffer, CNTV2Texture* texture) const;
	void CopyNextChunkTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const;
	void CopyTextureToBuffer(CNTV2Texture* texture, uint8_t* buffer) const;

};

CNTV2glTextureTransferNV *CreateNTV2glTextureTransferNV();

#endif

