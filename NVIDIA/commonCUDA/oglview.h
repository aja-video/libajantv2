/* SPDX-License-Identifier: MIT */
#ifndef _OGLVIEW_
#define _OGLVIEW_

#if defined( AJAMac ) || defined( AJALinux )
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glx.h>
#else
#include <Windows.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/gl.h>
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

