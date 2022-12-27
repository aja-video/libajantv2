/* SPDX-License-Identifier: MIT */
// Video I/O Helper Class Definition

#ifndef SIMPLE_CUDA_VIO_
#define SIMPLE_CUDA_VIO_

//#include "ntv2cudaArrayTransferNV.h"

#include "simplegpuvio.h"

/* 
	CCudaVideoIO : a class that encapsulates methods for simple video capture 
	and playback into a CUDA array/buffer using GPUDirect for Video.
*/

typedef class CCudaVideoIO : public CGpuVideoIO {
public:

	CCudaVideoIO() {};
	CCudaVideoIO(vioDesc *desc) : CGpuVideoIO(desc) {};
	~CCudaVideoIO() {};

private:

} CCudaVideoIO;

#endif
