/* SPDX-License-Identifier: MIT */

#ifndef _CNTV2_TEXTURE_
#define _CNTV2_TEXTURE_

#include "ntv2errorlist.h"

#include "ajatypes.h"
#include "opengl.h"

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
	
	void Use() const;
	void Destroy();
	
	GLuint GetUnit() const;
	GLuint GetIndex() const;
	ULWord GetWidth() const;
	ULWord GetHeight() const;

	CNTV2ErrorList& GetErrorList() const;
	void Error(const std::string& message) const;

private:
	ULWord mWidth;
	ULWord mHeight;
	GLuint mIndex;
	GLuint mUnit;
	
	mutable CNTV2ErrorList mErrorList;
};


#endif

