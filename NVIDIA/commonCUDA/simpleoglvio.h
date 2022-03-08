/* SPDX-License-Identifier: MIT */
// Video I/O Helper Class Definition

#ifndef SIMPLE_OGL_VIO_
#define SIMPLE_OGL_VIO_

#include "ntv2glTextureTransferNV.h"

#include "simplegpuvio.h"

/* 
	COglVideoIO : a class that encapsulates methods for simple video capture 
	and playback into an OpenGL texture using GPUDirect for Video.
*/

typedef class COglVideoIO : public CGpuVideoIO {
public:

	COglVideoIO() {};
	COglVideoIO(vioDesc *desc) : CGpuVideoIO(desc) {};
	~COglVideoIO() {};

private:

} COglVideoIO;

#endif
