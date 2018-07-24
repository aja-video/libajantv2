#include "cuda.h"
#include "cuda_runtime.h"
#include "cudaUtils.h"

surface<void, 2> inSurfRef;
surface<void, 2> outSurfRef;

__global__ void rgba_to_greyscale(unsigned int width, unsigned int height)
{
	// Indices into the image data
	unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
	unsigned int y = blockIdx.y * blockDim.y + threadIdx.y;
	if (x < width && y < height) {
		uchar4 rgba;

		// Read from input surface texture
		surf2Dread(&rgba, inSurfRef, x * 4, y);

		float channelSum = (.299f * rgba.x + .587f * rgba.y + .114f * rgba.z) / 255.0f;

		// Write to output surface texture
		surf2Dwrite(channelSum, outSurfRef, x * 4, y);
	}
}

extern "C" void ConvertRGBAToGreyscale(cudaArray *rgbaImage,
                                       cudaArray *greyImage,
                                       unsigned int width,
                                       unsigned int height)
{
	// Bind arrays to surface reference
	checkCudaErrors(cudaBindSurfaceToArray(inSurfRef, rgbaImage));
	checkCudaErrors(cudaBindSurfaceToArray(outSurfRef, greyImage));

	//You must fill in the correct sizes for the blockSize and gridSize
	//currently only one block with one thread is being launched
	const dim3 blockSize(24, 24, 1);
	const dim3 gridSize((width/16), (height/16) , 1);
	rgba_to_greyscale<<<gridSize, blockSize>>>(width, height);

	// Wait for kernel processing to complete for all threads.
	cuCtxSynchronize();;
}