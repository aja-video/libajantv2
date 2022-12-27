/* SPDX-License-Identifier: MIT */
// Video I/O Helper Class Definition

#include <ajatypes.h>
#include <ntv2card.h>
#include <ntv2devicefeatures.h>
#include <ntv2devicescanner.h>
#include <ntv2utils.h>

#include "ntv2gpucircularbuffer.h"
#include "ntv2glTextureTransferNV.h"

// Type / Direction
typedef enum {
   VIO_OUT,
   VIO_IN
} VIO_TYPE;

// GPU Object Description
typedef struct vioDesc {
    NTV2VideoFormat videoFormat;		// Video format
	NTV2FrameBufferFormat bufferFormat; // Frame buffer format
	NTV2Channel channel;                // Channel
    VIO_TYPE type;                      // Type: input or output
} vioDesc;

/* 
	CGpuVideoIO : a class that encapsulates methods for simple video capture 
	and playback into an OpenGL texture using GPUDirect for Video.
*/

typedef class CGpuVideoIO {
public:

	CGpuVideoIO();
	CGpuVideoIO(vioDesc *desc);
	~CGpuVideoIO();

	void WaitForCaptureStart();

	bool Capture();

	bool Playout();

	void SetGpuTransfer(CNTV2glTextureTransfer* transfer);
	CNTV2glTextureTransfer* GetGpuTransfer();

	void SetGpuCircularBuffer(CNTV2GpuCircularBuffer* gpuCircularBuffer);
	CNTV2GpuCircularBuffer*  GetGpuCircularBuffer();

private:

	CNTV2Card*                           mBoard;

	NTV2Channel                          mChannel;

	CNTV2glTextureTransfer*				mGPUTransfer;
	CNTV2GpuCircularBuffer*				mGPUCircularBuffer;

	ULWord                              mActiveVideoSize;
	ULWord                              mActiveVideoHeight;
	ULWord                              mActiveVideoPitch;
	ULWord                              mTransferLines;
	ULWord                              mTransferSize;
	ULWord*                             mpVidBufferSource;

	ULWord                              mFrameNumber;

} CGpuVideoIO;
