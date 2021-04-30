
#ifndef _CNTV2_TEXTURE_
#define _CNTV2_TEXTURE_

#include "ntv2errorlist.h"

#include "ajatypes.h"
#include "opengl.h"

#include "cudaUtils.h"

// Include this here rather than in cudaUtils to prevent everything that includes cudaUtils.h from needing to include gl.h
#include <cudaGL.h>
#include <cuda_gl_interop.h>

typedef enum NTV2TextureType {
	NTV2_TEXTURE_TYPE_OPENGL_TEXTURE,
	NTV2_TEXTURE_TYPE_CUDA_ARRAY
} CNTV2TextureType;

class CNTV2Texture {
public:
	CNTV2Texture();
	explicit CNTV2Texture(ULWord unit);
	~CNTV2Texture();
	
	void InitWithBitmap(const GLubyte* inData,
						ULWord inWidth,
						ULWord inHeight,
						bool mipmaps = false);
	
	void InitWithDataFile(const char* filename,
						  ULWord inWidth,
						  ULWord inHeight,
						  bool mipmaps = false);

	void InitWithCudaArray(ULWord inWidth,
		                   ULWord inHeight,
		                   ULWord index);

	void Use() const;
	void Destroy();
	
	GLuint GetUnit() const;
	GLuint GetIndex() const;
	ULWord GetWidth() const;
	ULWord GetHeight() const;

	CNTV2TextureType GetType() const;

	cudaArray* GetCudaArray() const;
	cudaGraphicsResource* GetGLTexInCUDA() const;

	CNTV2ErrorList& GetErrorList() const;
	void Error(const std::string& message) const;

private:
	NTV2TextureType mType;

	ULWord mWidth;
	ULWord mHeight;

	// OpenGL Texture
	GLuint mIndex;
	GLuint mUnit;

	cudaGraphicsResource* mGLTexInCUDA;
	
	// CUDA Array
	cudaChannelFormatDesc mChannelDesc;
	cudaArray_t mArray;

	mutable CNTV2ErrorList mErrorList;
};


#endif

