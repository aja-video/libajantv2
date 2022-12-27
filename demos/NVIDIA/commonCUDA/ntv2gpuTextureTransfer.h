/* SPDX-License-Identifier: MIT */
/*
  This software is provided by AJA Video, Inc. "AS IS"
  with no express or implied warranties.
*/

#ifndef _NTV2_GPU_TEXTURE_TRANSFER_
#define _NTV2_GPU_TEXTURE_TRANSFER_

#include "ntv2errorlist.h"
#include "ntv2texture.h"
#include "ntv2rendertotexture.h"

//#include "opengl.h"
#include "ajatypes.h"
#include "ntv2publicinterface.h"

#include <assert.h>
#include <string>

/* An abstract class representing the method by which a frame of video is
   transferred from the AJA hardware to a GPU texture.*/
class CNTV2gpuTextureTransfer {
public:	
	virtual ~CNTV2gpuTextureTransfer(){};
	
	/* Subclasses should override to do any initialization.
	   Caller should call Init() before anything else.*/
	virtual bool Init() = 0;
	
	/* Subclasses override to undo the work of Init().
	   Caller should call Destroy() before the end of
	   the life of the object.*/
	virtual void Destroy() = 0;
	
	/* Called to set the size of the video in pixels.*/
	virtual void SetSize( ULWord width, ULWord height ) = 0;
	
	/* Subclasses override this function to do any transfer initialization that are execution thread specific 
		and which should be done once before any transfers are performed.
	   Caller should call ThreadCleanup() before the same thread exits. */	   
	virtual void ThreadPrep() = 0; 

	/* Subclasses override this function to undo any transfer initialization that are execution thread specific.
	   and which should be done once after all transfers are performed.
	   Caller should call ThreadCleanup() in the same thread as ThreadPrep() is called. */
	virtual void ThreadCleanup() = 0;

	/* Some transfer schemes require a one-time, per-texture initialization,
	   those schemes should override this function to do that.*/
	virtual void RegisterTexture(CNTV2Texture* texture) const = 0;

	/* Some transfer schemes require a one-time, per buffer initialization,
	   those schemes should override these functions to do that.*/
	virtual void RegisterInputBuffer(uint8_t* buffer) const = 0;
	virtual void RegisterOutputBuffer(uint8_t* buffer) const = 0;
	
	/* Some transfer schemes require a one-time, per-texture initialization,
	   those schemes should override this function to do that.*/
	virtual void UnregisterTexture(CNTV2Texture* texture) const = 0;

	/* Some transfer schemes require a one-time, per buffer initialization,
	   those schemes should override these functions to do that.*/
	virtual void UnregisterInputBuffer(uint8_t* buffer) const = 0;
	virtual void UnregisterOutputBuffer(uint8_t* buffer) const = 0;


	/* In asynchronous transfer schemes where, subclasses override AcquireTexture
	   to wait (or have opengl operations wait) until the texture is available to use. */
	virtual void AcquireTexture(CNTV2Texture* texture) const = 0;

	/* In asynchronous transfer schemes where, subclasses override ReleaseTexture
	   to signal that OpenGL is done using the texture. */
	virtual void ReleaseTexture(CNTV2Texture* texture) const = 0;
	
	/* Subclasses should override these functions to set options on the
	   AUTOCIRCULATE_TRANSFER_STRUCT just before the call to TransferWithAutocirculate. */
	virtual void ModifyTransferStructForRecord(AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const = 0;
	virtual void ModifyTransferStructForPlayback(AUTOCIRCULATE_TRANSFER_STRUCT* transferStruct) const = 0;

	/* Caller should call BeforeRecordTransfer before every AJA transfer call to prep the
	   buffer texture and render-target for transfer from the AJA hardware to the GPU. */
	virtual void BeforeRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const = 0;
	
	/* Caller should call AfterRecordTransfer after every call to AJA transfer meant for copying from AJA hardware
	   to the GPU.  Subclasses should override to handle the buffer, texture and render-target.
	   For instance, this function could copy from the buffer to the texture. */
	virtual void AfterRecordTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const = 0;
	
	/* Caller should call BeforePlaybackTransfer before every call to AJA transfer to prep the
	   buffer texture and render-target for the transfer from the GPU to the AJA hardware.  For instance, this function
	   could copy from the texture to the buffer. */
	virtual void BeforePlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const = 0;

	/* Caller should call AfterPlaybackTransfer after every call to AJA transfer meant for copying from the GPU to
	   AJA hardware.  Subclasses should override to handle the buffer, texture and render-target after playback. */
	virtual void AfterPlaybackTransfer(uint8_t *buffer, CNTV2Texture* texture, CNTV2RenderToTexture* renderToTexture) const = 0;

	/* Caller should use this method to set the number of chunks used in GPU transfers.
		Multiple chunks are used when overlapping GPU transfers with the I/O DMAs */
	virtual void SetNumChunks(ULWord numChunks) = 0;

	/* Caller should use this method to match the number of chunks used with TransferWithAutoCirculate with the number of chunks 
		used by this class. Multiple chunks are used when overlapping gpu transfers with the I/O DMAs */
	virtual ULWord GetNumChunks() const = 0;
	
	/* Should return time in milliseconds to transfer the video frame from AJA hardware to the GPU*/
	virtual float GetCardToGpuTime(const CNTV2Texture* texture) const = 0;

	/* Should return time in milliseconds to transfer the video frame from GPU to AJA hardware*/
	virtual float GetGpuToCardTime(const CNTV2Texture* texture) const = 0;
};

#endif


