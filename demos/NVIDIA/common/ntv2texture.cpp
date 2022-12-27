/* SPDX-License-Identifier: MIT */

#include "ntv2texture.h"
#include "ntv2debug.h"

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

CNTV2Texture::CNTV2Texture() :
	mWidth(0), mHeight(0), mIndex(0), mUnit(0)
{
}

CNTV2Texture::CNTV2Texture(ULWord unit) :
	mWidth(0), mHeight(0), mIndex(0), mUnit(0)
{
}

CNTV2Texture::~CNTV2Texture()
{
	Destroy();
}

void CNTV2Texture::Error(const std::string& message) const
{
	mErrorList.Error(message);
}

CNTV2ErrorList& CNTV2Texture::GetErrorList() const
{
	return mErrorList;
}

void CNTV2Texture::InitWithDataFile(
	const char* filename,
	ULWord inWidth,
	ULWord inHeight,
	bool mipmaps)
{
	if ( !mIndex )
	{
		glGenTextures(1, &mIndex);
	}
	
	FILE* fp = fopen(filename, "rb");
	if ( fp )
	{
		fseek(fp, 0, SEEK_END);
		ULWord size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		GLubyte* data = (GLubyte*)malloc(size + 1);
		fread(data, 1, size, fp);
		fclose(fp);
		
		InitWithBitmap(data, inWidth, inHeight, mipmaps);
		
		free(data);
	}
	else
	{
		std::string notFoundMessage =
			std::string("Texture data file not found:") + filename +
			"Using default motley texture.";
		Error(notFoundMessage);
		odprintf( "%s\n", notFoundMessage.c_str() );
		
		GLubyte data[] = {0x00, 0x00, 0x00, 0xff,  0x00, 0x00, 0xff, 0xff,
						  0xff, 0x00, 0xff, 0xff,  0xff, 0x55, 0xff, 0xff};
		
		InitWithBitmap(data, 2, 2, false);
	}
}

void CNTV2Texture::InitWithBitmap(
	const GLubyte* inData,
	ULWord inWidth,
	ULWord inHeight,
	bool mipmaps)
{
	if ( !mIndex )
	{
		glGenTextures(1, &mIndex);
	}
	
	Use();
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	mWidth = inWidth;
	mHeight = inHeight;
	
	assert(mWidth > 0 && mWidth < 10000);
	assert(mHeight > 0 && mHeight < 10000);
	
	GLubyte* data_ptr = NULL;
	const GLubyte* data;
	
	ULWord size = mWidth * mHeight * 4;  // Assuming RGBA
	
	if ( !inData )
	{
		data_ptr = (UByte*)malloc(size * sizeof(float));
		memset(data_ptr, 0, size);
		data = data_ptr;
	}
	else
	{
		data = inData;
	}
	
	GLenum format = GL_RGBA;
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		format,
		mWidth,
		mHeight,
		0,
		format,
		GL_UNSIGNED_BYTE,
		data);
	
	if ( mipmaps )
	{
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	if ( data_ptr )
	{
		free(data_ptr);
		data_ptr = NULL;
	}
}

void CNTV2Texture::Use() const
{
	glActiveTexture(GL_TEXTURE0 + GetUnit());
	glBindTexture(GL_TEXTURE_2D, GetIndex());
}

void CNTV2Texture::Destroy()
{
	if ( mIndex > 0 )
	{
		glDeleteTextures(1, &mIndex);
		mIndex = 0;
	}
}

GLuint CNTV2Texture::GetIndex() const
{
	return mIndex;
}

GLuint CNTV2Texture::GetUnit() const
{
	return mUnit;
}

GLuint CNTV2Texture::GetWidth() const
{
	return mWidth;
}

GLuint CNTV2Texture::GetHeight() const
{
	return mHeight;
}
