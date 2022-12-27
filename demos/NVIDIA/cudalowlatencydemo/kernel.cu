#include "cuda.h"
#include "cuda_runtime.h"
#include "cudaUtils.h"

// Utility macros
#define DIVUP(A,B) ( (A)%(B) == 0 ? (A)/(B) : ((A) / (B) + 1) )

// The thread block size
#define BLOCK_SIZE_W 32
#define BLOCK_SIZE_H 32

__global__ void Copy_kernel(cudaSurfaceObject_t inputSurfObj, cudaSurfaceObject_t outputSurfObj,
	                        unsigned int width, unsigned int height)
{
	// Indices into the image data
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;  
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	if (x < width && y < height) {
		uchar4 data; 

		// Read from input surface texture
		surf2Dread(&data, inputSurfObj, x * 4, y);

		// Write to outputsurface texture
		surf2Dwrite(data, outputSurfObj, x * 4, y);
	}
}

extern "C" void CopyVideoInputToOuput(cudaSurfaceObject_t inputSurfObj, cudaSurfaceObject_t outputSurfObj,
	                                  unsigned int width, unsigned int height)
{
	// Set the block size
    dim3 BlockSz(BLOCK_SIZE_W, BLOCK_SIZE_H, 1);

    // Set the grid size
    dim3 GridSz(DIVUP(width, BLOCK_SIZE_W), DIVUP(height, BLOCK_SIZE_H), 1);

	// Execute the kernel
    Copy_kernel<<<GridSz,BlockSz>>>(inputSurfObj, outputSurfObj, width, height);

    // Wait for kernel processing to complete for all threads.
    cuCtxSynchronize();
}

__global__ void k_cuda_process_RGB16(cudaSurfaceObject_t inputSurfObj, cudaSurfaceObject_t outputSurfObj, 
	                                 const int width, const int height)
{
	int cx = (blockIdx.x * blockDim.x) + threadIdx.x;
	int cy = (blockIdx.y * blockDim.y) + threadIdx.y;

	if (cx >= width || cy >= height) return;

	// Read from input surface texture
	unsigned short r, g, b;
	surf2Dread(&b, inputSurfObj, (cx + 0) * 3, cy);
	surf2Dread(&g, inputSurfObj, (cx + 1) * 3, cy);
	surf2Dread(&r, inputSurfObj, (cx + 2) * 3, cy);

#if 0
	float4 data;

	// Convert to float
	data.z = (float)b / 65535.0f;
	data.y = (float)g / 65535.0f;
	data.x = (float)r / 65535.0f;
	data.w = (float)1.0f;
#else
	uchar4 data;

	// Convert to unsigned byte
	data.z = (int)((float)b / 65535.0f * 255.0);
	data.y = (int)((float)g / 65535.0f * 255.0);
	data.x = (int)((float)r / 65535.0f * 255.0);
	data.w = (int)((float)1.0f);

#endif

	// Write to outputsurface texture
	surf2Dwrite(data, outputSurfObj, cx * 4, cy);
}

extern "C" void CudaProcessRGB16(cudaSurfaceObject_t inputSurfObj, cudaSurfaceObject_t outputSurfObj,
	                             unsigned int width, unsigned int height)
{
	// Set the block size
	dim3 BlockSz(BLOCK_SIZE_W, BLOCK_SIZE_H, 1);

	// Set the grid size
	dim3 GridSz(DIVUP(width, BLOCK_SIZE_W), DIVUP(height, BLOCK_SIZE_H), 1);

	// Execute the kernel
	k_cuda_process_RGB16 << <GridSz, BlockSz >> >(inputSurfObj, outputSurfObj, width, height);

	// Wait for kernel processing to complete for all threads.
	cuCtxSynchronize();
}

__global__ void k_cuda_process_RGB10(cudaSurfaceObject_t inputSurfObj, cudaSurfaceObject_t outputSurfObj, 
	                                 const int width, const int height)
{
	int cx = (blockIdx.x * blockDim.x) + threadIdx.x;
	int cy = (blockIdx.y * blockDim.y) + threadIdx.y;

	if (cx >= width || cy >= height) return;

	// Read from input surface texture
	unsigned int inPixel;
	surf2Dread(&inPixel, inputSurfObj, cx * 4, cy);

	// Convert to float, scale to [0,255], convert back to unsigned char/byte
#if 1
	float4 data;
    data.x = (float)((inPixel & 0x3FF) / 1023.0f);
	data.y = (float)(((inPixel >> 10) & 0x3FF) / 1023.0f);
	data.z = (float)(((inPixel >> 20) & 0x3FF) / 1023.0f);
	data.w = 1.0;

	// Write to outputsurface texture
	surf2Dwrite(data, outputSurfObj, cx * 16, cy);
#else
	uchar4 data;
	data.z = (int)((((float)(inPixel & 0x3FF)) / 1023.0f) * 255);
	data.y = (int)((((float)(((inPixel >> 10)) & 0x3FF)) / 1023.0f) * 255);
	data.x = (int)((((float)(((inPixel >> 20)) & 0x3FF)) / 1023.0f) * 255);
	data.w = 255;

	// Write to outputsurface texture
	surf2Dwrite(data, outSurfRef, cx * 4, cy);
#endif


}

extern "C" void CudaProcessRGB10(cudaSurfaceObject_t inputSurfObj, cudaSurfaceObject_t outputSurfObj,
	                             unsigned int width, unsigned int height)
{
	// Bind arrays to surface reference
	//checkCudaErrors(cudaBindSurfaceToArray(inSurfRef, pIn));
	//checkCudaErrors(cudaBindSurfaceToArray(outSurfRef, pOut));

	// Set the block size
	dim3 BlockSz(BLOCK_SIZE_W, BLOCK_SIZE_H, 1);

	// Set the grid size
	dim3 GridSz(DIVUP(width, BLOCK_SIZE_W), DIVUP(height, BLOCK_SIZE_H), 1);

	// Execute the kernel
	k_cuda_process_RGB10 << <GridSz, BlockSz >> >(inputSurfObj, outputSurfObj, width, height);

	// Wait for kernel processing to complete for all threads.
	cuCtxSynchronize();
}
