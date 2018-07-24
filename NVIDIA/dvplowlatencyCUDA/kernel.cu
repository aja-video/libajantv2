#include "cuda.h"
#include "cuda_runtime.h"
#include "cudaUtils.h"

// Utility macros
#define DIVUP(A,B) ( (A)%(B) == 0 ? (A)/(B) : ((A) / (B) + 1) )

// The thread block size
#define BLOCK_SIZE_W 32
#define BLOCK_SIZE_H 32

surface<void, 2> inSurfRef;
surface<void, 2> outSurfRef;
__global__ void Copy_kernel(unsigned int width, unsigned int height)
									  
{
	// Indices into the image data
    unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;  
    unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	if (x < width && y < height) {
		uchar4 data; 

		// Read from input surface texture
		surf2Dread(&data, inSurfRef, x * 4, y);

		// Write to outputsurface texture
		surf2Dwrite(data, outSurfRef, x * 4, y);
	}
}

extern "C" void CopyVideoInputToOuput(cudaArray *pIn, cudaArray *pOut, 
	                                  unsigned int width, unsigned int height)
{
	// Bind arrays to surface reference
	checkCudaErrors(cudaBindSurfaceToArray(inSurfRef, pIn));
	checkCudaErrors(cudaBindSurfaceToArray(outSurfRef, pOut));

	// Set the block size
    dim3 BlockSz(BLOCK_SIZE_W, BLOCK_SIZE_H, 1);

    // Set the grid size
    dim3 GridSz(DIVUP(width, BLOCK_SIZE_W), DIVUP(height, BLOCK_SIZE_H), 1);

	// Execute the kernel
    Copy_kernel<<<GridSz,BlockSz>>>(width, height);

    // Wait for kernel processing to complete for all threads.
    cuCtxSynchronize();
}

