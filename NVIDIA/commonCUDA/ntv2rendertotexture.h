/* SPDX-License-Identifier: MIT */
#ifndef _CNTV2CNTV2RenderToTexture_
#define _CNTV2CNTV2RenderToTexture_

#include "opengl.h"
#include "ntv2texture.h"

class CNTV2RenderToTexture {
public:
	CNTV2RenderToTexture();
	virtual ~CNTV2RenderToTexture();
	
	// Call this function before rendering to set
	// the texture to which to render.
	void SetTexture(CNTV2Texture* texture);
	
	// Use this to get the texture if you need it.
	const CNTV2Texture* GetTexture() const;
	
	// Call this function to start rendering to
	// texture.  Subsequent draw calls in OpenGL
	// will draw into the texture.
	void Begin() const;
	
	// Call this function to return rendering
	// target to the screen.
	void End() const;
	
private:
	CNTV2Texture* mTexture;
	mutable GLuint mFrameBuffer;
	mutable GLuint mDepthStencilBuffer;

	
	void Destroy();
};

#endif

