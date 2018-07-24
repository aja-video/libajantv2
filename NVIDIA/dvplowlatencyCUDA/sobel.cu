#include "cuda.h"
#include "cuda_runtime.h"
#include "cudaUtils.h"

texture <float, 2, cudaReadModeElementType> inTex;
surface<void, 2> outSurfRef;

//Kernel for x direction sobel
__global__ void implement_x_sobel(int width, int height, int widthStep)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    // Make sure that thread is inside image bounds
    if(x<width && y<height)
    {
		uchar4 rgba;

		float output_value_x = (-1 * tex2D(inTex, x - 1, y - 1)) + (0 * tex2D(inTex, x, y - 1)) + (1 * tex2D(inTex, x + 1, y - 1))
			                 + (-2 * tex2D(inTex, x - 1, y))     + (0 * tex2D(inTex, x, y))     + (2 * tex2D(inTex, x + 1, y))
			                 + (-1 * tex2D(inTex, x - 1, y + 1)) + (0 * tex2D(inTex, x, y + 1)) + (1 * tex2D(inTex, x + 1, y + 1));

		float output_value_y = (-1 * tex2D(inTex, x - 1, y - 1)) + (-2 * tex2D(inTex, x, y - 1)) + (1 * tex2D(inTex, x + 1, y - 1))
			                 + (0 * tex2D(inTex, x - 1, y)) + (0 * tex2D(inTex, x, y)) + (0 * tex2D(inTex, x + 1, y))
			                 + (-1 * tex2D(inTex, x - 1, y + 1)) + (2 * tex2D(inTex, x, y + 1)) + (1 * tex2D(inTex, x + 1, y + 1));

		float output_value = sqrt((output_value_x * output_value_x) + (output_value_y * output_value_y));

		rgba.x = output_value * 255;
		rgba.y = output_value * 255;
		rgba.z = output_value * 255;
		rgba.w = output_value * 255;

		surf2Dwrite(rgba, outSurfRef, x * 4, y);
    }
}

extern "C" void DoSobel(cudaArray *greyImage, cudaArray *outImage, unsigned int width, unsigned int height, unsigned int widthStep)
{
	// Bind arrays to surface reference
	checkCudaErrors(cudaBindTextureToArray(inTex, greyImage));
	checkCudaErrors(cudaBindSurfaceToArray(outSurfRef, outImage));
	
	const dim3 blocksize(16,16);
	const dim3 gridsize((width + blocksize.x - 1) / blocksize.x, 
		                (height + blocksize.y - 1) / blocksize.y, 
						1);

    implement_x_sobel<<<gridsize,blocksize>>>(width, height, widthStep/sizeof(float));

	cudaDeviceSynchronize();
}
