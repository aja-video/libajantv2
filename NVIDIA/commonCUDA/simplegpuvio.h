/* SPDX-License-Identifier: MIT */
// Basic GPU Video I/O Helper Class Definition

#ifndef SIMPLE_GPU_VIO_
#define SIMPLE_GPU_VIO_

#include <ajatypes.h>
#include <ntv2card.h>
#include <ntv2devicefeatures.h>
#include <ntv2devicescanner.h>
#include <ntv2utils.h>

#include "ntv2gpucircularbuffer.h"
#include "ntv2gpuTextureTransfer.h"

// Type / Direction
typedef enum {
	VIO_OUT,
	VIO_IN
} VIO_TYPE;

// GPU Object Description
typedef struct vioDesc {
	uint32_t deviceIndex;				// Device index
	NTV2VideoFormat videoFormat;		// Video format
	NTV2FrameBufferFormat bufferFormat; // Frame buffer format
	NTV2Channel channel;                // Channel
	VIO_TYPE type;                      // Type: input or output
} vioDesc;

/* 
	CGpuVideoIO : a class that encapsulates methods for simple video capture 
	and playback into a GPU texture using GPUDirect for Video.
*/

typedef class CGpuVideoIO {
public:

	CGpuVideoIO();
	CGpuVideoIO(vioDesc *desc);
	~CGpuVideoIO();

	void WaitForCaptureStart();

	bool Capture();

	bool Playout();

	void SetGpuTransfer(CNTV2gpuTextureTransfer* transfer);
	CNTV2gpuTextureTransfer* GetGpuTransfer();

	void SetGpuCircularBuffer(CNTV2GpuCircularBuffer* gpuCircularBuffer);
	CNTV2GpuCircularBuffer*  GetGpuCircularBuffer();

protected:

	CNTV2Card*                          mBoard;

	NTV2Channel                         mChannel;
		
	CNTV2gpuTextureTransfer*			mGPUTransfer;
	CNTV2GpuCircularBuffer*				mGPUCircularBuffer;

	ULWord                              mActiveVideoSize;
	ULWord                              mActiveVideoHeight;
	ULWord                              mActiveVideoPitch;
	ULWord                              mTransferLines;
	ULWord                              mTransferSize;
	ULWord*                             mpVidBufferSource;

	ULWord                              mFrameNumber;

} CGpuVideoIO;

#endif
