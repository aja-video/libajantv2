/* SPDX-License-Identifier: MIT */
#ifndef _GPUVIO_
#define _GPUVIO_

// GPU Video I/O Type Definitions

#include <ajatypes.h>
#include <ntv2card.h>
#include <ntv2devicefeatures.h>
#include <ntv2devicescanner.h>
#include <ntv2utils.h>

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

#endif