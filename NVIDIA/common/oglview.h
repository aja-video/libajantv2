/* SPDX-License-Identifier: MIT */

#ifndef _OGLVIEW_
#define _OGLVIEW_

#if defined( AJAMac ) || defined( AJALinux )
#include <gl/glew.h>
//#include <gl/glxew.h>
#include <gl/glut.h>
#include <gl/gl.h>
#include <gl/glx.h>
//#include <gl/glu.h>
#else
#include <Windows.h>
#include <gl/glew.h>
#include <gl/wglew.h>
#include <gl/gl.h>
#endif

#include <ntv2debug.h>

// OpenGL View Description
typedef struct oglViewDesc {
#ifdef AJA_WINDOWS
	HDC hDC;	// Device context
#else
        Display *dpy;
        Window win;
        GLXContext ctx;
#endif
	int mWidth;      // Width
	int mHeight;
} oglViewDesc;

typedef class COglView {

public:

	COglView();
	COglView(oglViewDesc *desc);
	~COglView();

	bool init(void);
	void uninit(void);
	void resize(GLuint w, GLuint h);
	void render(GLuint renderedTexture, 
		        float durationCapture,
		        float durationDraw,
		        float durationPlayout);
	void initScene();

private:
#ifdef AJA_WINDOWS
	HDC hDC;
	HGLRC hGLRC;
#else
        Display *dpy;
        Window win;
        GLXContext ctx;
#endif

	GLuint mWidth;
	GLuint mHeight;

	GLuint mVertShader;
	GLuint mFragShader;
	GLuint mOutputProgram;

} COglView;

void CheckGLErrors();

#endif

