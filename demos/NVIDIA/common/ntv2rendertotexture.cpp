/* SPDX-License-Identifier: MIT */

#include "ntv2rendertotexture.h"

#include <assert.h>

CNTV2RenderToTexture::CNTV2RenderToTexture() :
	mTexture(NULL),
	mFrameBuffer(0),
	mDepthStencilBuffer(0)
{
}

CNTV2RenderToTexture::~CNTV2RenderToTexture()
{
	Destroy();
}

void CNTV2RenderToTexture::SetTexture(CNTV2Texture* inTexture)
{
	// For now, forbid swapping in a new texture.
	assert(!mTexture);
	mTexture = inTexture;
}

const CNTV2Texture* CNTV2RenderToTexture::GetTexture() const
{
	return mTexture;
}

void CNTV2RenderToTexture::Begin() const
{
	assert(mTexture);
	
	if ( !mFrameBuffer )
	{
		glGenFramebuffers(1, &mFrameBuffer);
		
		glGenRenderbuffers(1, &mDepthStencilBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, mDepthStencilBuffer);
		glRenderbufferStorage(
			GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mTexture->GetWidth(), mTexture->GetHeight());
		
		assert(mFrameBuffer);
	}
	
	GLuint index = mTexture->GetIndex();
	GLuint unit = mTexture->GetUnit();
	GLuint level = 0;
	
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glBindTexture(GL_TEXTURE_2D, index);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, index, level);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthStencilBuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthStencilBuffer);
}

void CNTV2RenderToTexture::End() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CNTV2RenderToTexture::Destroy()
{
	glDeleteFramebuffers(1, &mFrameBuffer);
}



