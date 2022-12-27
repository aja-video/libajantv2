/* SPDX-License-Identifier: MIT */
#include "oglview.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <string.h>

//
// G L S L  S H A D E R S
//

//
// Vertex Shader: VertexShader 
//
// Simple vertex shader to map the vertex coordinates by the projection
// and view matrices and simply pass through the texture coordinates to
// the bound fragment shader.
//
const char *VertexShader =
"#version 400\n"
"\n"
"in vec3 in_vert_coord;\n"
"in vec2 in_tex_coord;\n"
"uniform mat4 ViewMatrix;\n"
"uniform mat4 ProjMatrix;\n"
"\n"
"out vec2 tex_coord;\n"
"\n"
"void main() {\n"
"	vec4 pos = vec4( in_vert_coord, 1.0);\n"
"	tex_coord = in_tex_coord;\n"
"   gl_Position = ProjMatrix * ViewMatrix * pos;\n"
"}\n"
"";

//
// Fragment Shader: FragmentShader 
//
// Simple fragment shader that performs a texture lookup into
// a 4-component float texture and returns the resulting texel
// as a 4-component float.  This shader is used in this sample
// to blit the composite scene from an 8-bit FBO-attached
// texture into the OpenGL view graphics window.
//
const char *FragmentShader =
"// Needed to use integer textures:\n"
"#extension GL_EXT_gpu_shader4 : enable\n"
"in  vec2 tex_coord;\n"
"uniform sampler2D tex;\n"
"\n"
"void main(out vec4 color) {\n"
"	color = texture2D(tex, tex_coord);\n"
"}\n"
"";


void failGL(GLuint res)
{
	fprintf(stderr, "GL Failed with value %X\n", res);
}

#define GL_CHECK() {             \
    GLuint tmp = glGetError();    \
    if(GL_NO_ERROR != tmp)     \
        failGL(tmp);     \
}

void CheckGLErrors()
{
	GLuint error = glGetError();

	if (error != GL_NO_ERROR)
	{
#if _DEBUG
		char* errString = (char*)gluErrorString(error);
		odprintf("%s", errString);
#else
		exit(0);
#endif
	}
}
#ifdef AJA_WINDOWS
void setupPixelformat(HDC hDC)
{
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),	/* size of this pfd */
		1,				/* version num */
		PFD_DRAW_TO_WINDOW |		/* support window */
		PFD_DOUBLEBUFFER |              /* double buffered */
		PFD_SUPPORT_OPENGL,		/* support OpenGL */
		PFD_TYPE_RGBA,			/* color type */
		8,				/* 8-bit color depth */
		0, 0, 0, 0, 0, 0,		/* color bits (ignored) */
		0,				/* no alpha buffer */
		0,				/* alpha bits (ignored) */
		0,				/* no accumulation buffer */
		0, 0, 0, 0,			/* accum bits (ignored) */
		0,				/* depth buffer (filled below)*/
		0,				/* no stencil buffer */
		0,				/* no auxiliary buffers */
		PFD_MAIN_PLANE,			/* main layer */
		0,				/* reserved */
		0, 0, 0,			/* no layer, visible, damage masks */
	};
	int SelectedPixelFormat;
	BOOL retVal;

	SelectedPixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (SelectedPixelFormat == 0) {
		fprintf(stderr, "ChoosePixelFormat failed\n");

	}

	retVal = SetPixelFormat(hDC, SelectedPixelFormat, &pfd);
	if (retVal != TRUE) {
		fprintf(stderr, "SetPixelFormat failed\n");

	}
}
#endif
COglView::COglView(oglViewDesc *desc) :
#ifdef AJA_WINDOWS
	hDC(desc->hDC),
	mWidth(1920),
	mHeight(1080)

#else
        dpy(desc->dpy),
        win(desc->win),
        ctx(desc->ctx),
		mWidth(desc->mWidth),
		mHeight(desc->mHeight)
#endif


{
#ifdef AJA_WINDOWS
	// Setup pixel format
	setupPixelformat(hDC);

	// Create OpenGL rendering context
	hGLRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hGLRC);
#else
        glXMakeCurrent(dpy, win, ctx);
#endif

	assert(GL_NO_ERROR == glGetError());
}

COglView::~COglView()
{
#ifdef AJA_WINDOWS
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hGLRC);
#endif
}

bool COglView::init(void)
{
	GLint val;
	GLint res;

	// Init glew
	glewInit();
	if (!glewIsSupported("GL_VERSION_2_0 "
		"GL_ARB_pixel_buffer_object "
		"GL_EXT_framebuffer_object "
		)) {
		fprintf(stderr, "Support for necessary OpenGL extensions missing.\n");
	}
	assert(GL_NO_ERROR == glGetError());

	// Hack to detect the presence of G7x. While GL_EXT_geometry_shader4 is not required
	// for this test, it's an easy way to check if the HW is GL 3.0 capable or not.
	if (!strstr((const char *)glGetString(GL_EXTENSIONS), "GL_EXT_geometry_shader4")) {
		printf("Found unsupported config for interop.\n");
	}
	assert(GL_NO_ERROR == glGetError());

	// Don't block waiting for vsync
#ifdef AJA_WINDOWS
	wglSwapIntervalEXT(1);
#else
	//glXSwapIntervalEXT(dpy, win, 0);
#endif

	// Create GLSL vertex shader to blit the composited texture
	// image into the OpenGL view graphics window.
	mVertShader = glCreateShader(GL_VERTEX_SHADER);

	// Initialize GLSL vertex shader
	glShaderSourceARB(mVertShader, 1, (const GLchar **)&VertexShader, NULL);

	// Compile vertex shader
	glCompileShaderARB(mVertShader);

	// Check for errors
	glGetShaderiv(mVertShader, GL_COMPILE_STATUS, &val);
	if (!val) {
		char infoLog[10000];
		glGetShaderInfoLog(mVertShader, 10000, NULL, infoLog);
		odprintf("Failed to load GLSL vertex shader, INFO:\n\n%s\n", infoLog);
		return false;
	}

	// Create GLSL fragment shader to blit the composited texture
	// image into the OpenGL view graphics window.
	mFragShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Initialize GLSL shader for output image texture from FBO
	glShaderSourceARB(mFragShader, 1, (const GLchar **)&FragmentShader, NULL);

	// Compile fragment shader
	glCompileShaderARB(mFragShader);

	// Check for errors
	glGetShaderiv(mFragShader, GL_COMPILE_STATUS, &val);
	if (!val) {
		char infoLog[10000];
		glGetShaderInfoLog(mFragShader, 10000, NULL, infoLog);
		odprintf("Failed to load GLSL fragment shader for output image texture from FBO, INFO:\n\n%s\n", infoLog);
		return false;
	}

	// Create shader program for output image texture from FBO
	mOutputProgram = glCreateProgram();

	// Attach vertex shader to program
	glAttachShader(mOutputProgram, mVertShader);

	// Attach fragment shader to program
	glAttachShader(mOutputProgram, mFragShader);

	// Link shader program
	glLinkProgram(mOutputProgram);

	// Check for link errors
	glGetProgramiv(mOutputProgram, GL_LINK_STATUS, &res);
	if (!res) {
		odprintf("Failed to link GLSL output image texture program\n");
		GLint infoLength;
		glGetProgramiv(mOutputProgram, GL_INFO_LOG_LENGTH, &infoLength);
		if (infoLength) {
			char *buf;
			buf = (char *)malloc(infoLength);
			if (buf) {
				glGetProgramInfoLog(mOutputProgram, infoLength, NULL, buf);
				odprintf("Program Log: \n");
				odprintf("%s", buf);
				free(buf);
			}
		}
		return false;
	}

	assert(glGetError() == GL_NO_ERROR);

	return true;
}

void COglView::uninit(void)
{
#ifdef AJA_WINDOWS
	if (hGLRC) {
		wglMakeCurrent(hDC, hGLRC);
#else
	if (ctx) {
	  glXMakeCurrent(dpy, win, ctx);
#endif
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
}


void COglView::resize(GLuint w, GLuint h)
{
#ifdef AJA_WINDOWS
	mWidth = 1920;
	mHeight = 1080;
#else
	mWidth = w;
	mHeight = h;

#endif
}

void COglView::render(GLuint renderedTexture,
	                  float durationCapture,
	                  float durationDraw,
	                  float durationPlayout)
{
	assert(glGetError() == GL_NO_ERROR);

	//
	// Draw texture contents to graphics window.
	//	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset view parameters
//	glViewport(0, 0, mWidth, mHeight);
	glViewport(0, 0, 1920, 1080);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//  Use GLSL output shader program
	glUseProgram(mOutputProgram);

	// Set uniform variables
	GLfloat view_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);
	glUniformMatrix4fv(glGetUniformLocation(mOutputProgram, "ViewMatrix"), 1, GL_FALSE, view_matrix);

	GLfloat proj_matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, proj_matrix);
	glUniformMatrix4fv(glGetUniformLocation(mOutputProgram, "ProjMatrix"), 1, GL_FALSE, proj_matrix);

	GLuint tex2;
	tex2 = glGetUniformLocation(mOutputProgram, "tex");
	glUniform1i(tex2, 0);

	// Get vertex attribute locations
	GLuint tex_coord2 = glGetAttribLocation(mOutputProgram, "in_tex_coord");

	assert(glGetError() == GL_NO_ERROR);

	// Set draw color
	glColor3f(1.0f, 1.0f, 1.0f);

	// Make sure depth test is disabled.
	glDisable(GL_DEPTH_TEST);

	// Bind texture object 	
	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	assert(glGetError() == GL_NO_ERROR);

	// Draw textured quad in OpenGL view graphics window.
	glBegin(GL_QUADS);
	glVertexAttrib2f(tex_coord2, 1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
	glVertexAttrib2f(tex_coord2, 1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
	glVertexAttrib2f(tex_coord2, 0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
	glVertexAttrib2f(tex_coord2, 0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
	glEnd();

	//GLuint res = glGetError();
	assert(glGetError() == GL_NO_ERROR);

	glBindTexture(GL_TEXTURE_2D, 0);

	// Unbind GLSL shader program
	glUseProgram(0);

	// Draw timing statistics
	char buf[100];
	sprintf(buf, "Card->GPU: %6.3f msec  Draw: %6.3f msec  GPU->Card: %6.3f msec\n", durationCapture, durationDraw, durationPlayout);
	size_t len = strlen(buf);
	glListBase(1000);
	glColor3f(0.0f, 1.0f, 0.0f);
	glRasterPos2f(-1.0f, -0.98f);
	glCallLists((GLsizei)len, GL_UNSIGNED_BYTE, buf);

	glEnable(GL_DEPTH_TEST);

	assert(glGetError() == GL_NO_ERROR);

#ifdef AJA_WINDOWS
	SwapBuffers(hDC);
#else
        glXSwapBuffers(dpy, win);
#endif
}
