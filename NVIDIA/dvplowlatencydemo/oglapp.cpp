/* SPDX-License-Identifier: MIT */

/********************************************************************************************
oglapp:
	An OpenGL example that demonstrates usage of GPU Direct for Video for low latency video I/O + gpu processing. 
	The example is hard-coded for 720p 59.94Hz video format.
	The demo expects a 720p 59.94 video on channel 1(if bidirectional the code makes it an input)
	The demo outputs the composite on channel 3(if bidirectional the code makes it an output)
	The amount of rendering in the sample will determine the in-out latency. With the current rendering load, the latency is 
	at 3 frames. 
***********************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS 1
#include "ajatypes.h"

#include "ajabase/system/event.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/systemtime.h"

#include <math.h>
#include <iostream>
#include <fstream>
#include <assert.h>

#if defined(AJALinux)
#include <string.h>
#endif

#include "simplegpuvio.h"

#include "oglview.h"

#include "fbo.h"

#include "AJA_Logo_Small.h"

#include "ntv2formatdescriptor.h"

using namespace std;
const int RING_BUFFER_SIZE = 2;



int gWidth;
int gHeight;

struct MyVertex
{
	float x, y;
	float s, t;
};

static const MyVertex g_video_vertex_buffer_data[] = {
	      { -1.0f, -1.0f,  1.0f,  0.0f},
	      {  1.0f, -1.0f,  0.0f,  0.0f},
	      { -1.0f,  1.0f,  1.0f,  1.0f},
	      {  1.0f,  1.0f,  0.0f,  1.0f}
};

static const GLushort g_video_element_buffer_data[] = { 0, 1, 2, 3 };

static const MyVertex g_geom_vertex_buffer_data[] = {
	      { -2.5f, -1.0f,  1.0f,  1.0f},
	      {  2.5f, -1.0f,  0.0f,  1.0f},
	      { -2.5f,  1.0f,  1.0f,  0.0f},
	      {  2.5f,  1.0f,  0.0f,  0.0f}
};

static const GLushort g_geom_element_buffer_data[] = { 0, 1, 2, 3 };

#ifdef AJA_WINDOWS
static HWND hWnd;
#else
Display *dpy;
Window win;
GLXContext ctx;
#endif

#ifndef TRUE
#define TRUE 1
#endif

// OpenGL view object
COglView *oglview;

// Video I/O object(s)
CGpuVideoIO *capture;
CGpuVideoIO *playout;

// GPU Circular Buffers
CNTV2GpuCircularBuffer *inGpuCircularBuffer;
CNTV2GpuCircularBuffer *outGpuCircularBuffer;

// DVP transfer
CNTV2glTextureTransfer* gpuTransfer;

// Threads
AJAThread CaptureThread;
AJAThread PlayoutThread;

void	CaptureThreadFunc(AJAThread * pThread, void * pContext);
void	PlayoutThreadFunc(AJAThread * pThread, void * pContext);

// Events
AJAEvent gCaptureThreadReady(TRUE);
AJAEvent gCaptureThreadDestroy(TRUE);
AJAEvent gCaptureThreadDone(TRUE);
AJAEvent gPlayoutThreadReady(TRUE);
AJAEvent gPlayoutThreadDestroy(TRUE);
AJAEvent gPlayoutThreadDone(TRUE);

bool gbDone;

// Timers
float durationCaptureCPU;
float durationPlayoutCPU;
float durationDrawCPU;

float durationCaptureGPU;
float durationPlayoutGPU;
float durationDrawGPU;

GLuint windowTexture;
GLuint drawTimeQuery;

// GLSL shaders and programs
GLuint videoVertShader;
GLuint videoFragShader;
GLuint videoRenderProgram;

GLuint geomVertShader;
GLuint geomFragShader;
GLuint geomRenderProgram;

// Attributes
GLint videoPosition;
GLint videoTexCoord;
GLint geomPosition;
GLint geomTexCoord;

// Buffer objects
GLuint geomVertexBufferObject;
GLuint geomElementBufferObject;

GLuint videoVertexBufferObject;
GLuint videoElementBufferObject;

// Texture objects
GLuint logoTex;

static bool get4KInputFormat(NTV2VideoFormat & videoFormat);
//
// G L S L  S H A D E R S
//

//
// Vertex Shader: VideoVertexShader 
//
// Simple vertex shader to map the vertex coordinates by the projection
// and view matrices and simply pass through the texture coordinates to
// the bound fragment shader.
//
const char *VideoVertexShader =
"#version 400\n"
"\n"
"in vec2 in_vert_coord;\n"
"in vec2 in_tex_coord;\n"
"uniform mat4 ViewMatrix;\n"
"uniform mat4 ProjMatrix;\n"
"\n"
"out vec2 tex_coord;\n"
"\n"
"void main() {\n"
"	vec4 pos = vec4(-in_vert_coord.x, in_vert_coord.y, 0.0 , 1.0);\n"
"	tex_coord = in_tex_coord;\n"
"   gl_Position = ProjMatrix * ViewMatrix * pos;\n"
"}\n"
"";

//
// Fragment Shader: VideoFragmentShader 
//
// Simple fragment shader that performs a texture lookup into
// a 4-component float texture and returns the resulting texel
// as a 4-component float.  This shader is used in this sample
// to blit the input video into the background of the scene
//
const char *VideoFragmentShader =
"#version 400\n"
"// Needed to use integer textures:\n"
"#extension GL_EXT_gpu_shader4 : enable\n"
"in  vec2 tex_coord;\n"
"uniform sampler2D tex;\n"
"out vec4 color;\n"
"\n"
"void main(void) {\n"
"	color = texture2D(tex, tex_coord);\n"
"}\n"
"";

//
// Vertex Shader: GeomVertexShader 
//
// Vertex shader to map the vertex coordinates by the projection
// and view matrices and perform lighting calculations.
//
const char *GeomVertexShader =
"#version 400\n"
"mat4 view_frustum(\n"
"	float angle_of_view,\n"
"	float aspect_ratio,\n"
"	float z_near,\n"
"	float z_far\n"
"	) {\n"
"	return mat4(\n"
"		vec4(1.0 / tan(angle_of_view), 0.0, 0.0, 0.0),\n"
"		vec4(0.0, aspect_ratio / tan(angle_of_view), 0.0, 0.0),\n"
"		vec4(0.0, 0.0, (z_far + z_near) / (z_far - z_near), 1.0),\n"
"		vec4(0.0, 0.0, -2.0*z_far*z_near / (z_far - z_near), 0.0)\n"
"		);\n"
"}\n"
"\n"
"mat4 scale(float x, float y, float z)\n"
"{\n"
"	return mat4(\n"
"		vec4(x, 0.0, 0.0, 0.0),\n"
"		vec4(0.0, y, 0.0, 0.0),\n"
"		vec4(0.0, 0.0, z, 0.0),\n"
"		vec4(0.0, 0.0, 0.0, 1.0)\n"
"		);\n"
"}\n"
"\n"
"mat4 translate(float x, float y, float z)\n"
"{\n"
"	return mat4(\n"
"		vec4(1.0, 0.0, 0.0, 0.0),\n"
"		vec4(0.0, 1.0, 0.0, 0.0),\n"
"		vec4(0.0, 0.0, 1.0, 0.0),\n"
"		vec4(x, y, z, 1.0)\n"
"		);\n"
"}\n"
"\n"
"mat4 rotate_x(float theta)\n"
"{\n"
"	return mat4(\n"
"		vec4(1.0, 0.0, 0.0, 0.0),\n"
"		vec4(0.0, cos(theta), sin(theta), 0.0),\n"
"		vec4(0.0, -sin(theta), cos(theta), 0.0),\n"
"		vec4(0.0, 0.0, 0.0, 1.0)\n"
"		);\n"
"}\n"
"mat4 rotate_y(float theta)\n"
"{\n"
"	return mat4(\n"
"		vec4( cos(theta),  0.0,  sin(theta),  0.0),\n"
"		vec4(        0.0,  1.0f,        0.0,  0.0),\n"
"		vec4(-sin(theta),  0.0,  cos(theta),  0.0),\n"
"		vec4(        0.0,  0.0,         0.0,  1.0)\n"
"		);\n"
"}\n"
"\n"
"\n"
"in vec2 in_vert_coord;\n"
"in vec2 in_tex_coord;\n"
"uniform mat4 ViewMatrix;\n"
"uniform mat4 ProjMatrix;\n"
"uniform float angle;\n"
"out vec2 tex_coord;"
"\n"
"void main(void) {\n"
"\n"
"   // Transform the vertex position to model view space\n"
"   gl_Position = ProjMatrix * ViewMatrix * translate(0.0, -1.0, 0.0) * rotate_y(angle) * rotate_x(0) *  scale(75.0, 75.0, 75.0) * vec4(in_vert_coord, 0.0, 1.0); \n"
"\n"
"   // Pass through the texture coordinates\n"
"   tex_coord = in_tex_coord;\n"
"}\n"
"";

//
// Fragment Shader: GeomFragmentShader 
//
// Fragment shader to perform per-pixel lighting calculations.
//
const char *GeomFragmentShader =
"#version 400\n"
"#extension GL_EXT_gpu_shader4 : enable\n"
"in vec2 tex_coord;\n"
"uniform sampler2D logotex;\n"
"out vec4 color;\n"
"void main(void) {\n"
"\n"
"   // Calculate the final color\n"
"   color = texture2D(logotex, tex_coord);\n"
"}\n"
"";


void renderToWindowGL();
void closeApp();

#ifdef AJA_WINDOWS
LRESULT APIENTRY
WndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    GLuint res = 0;
    switch (message) {
    case WM_CREATE:

		oglViewDesc viewDesc;
		viewDesc.hDC = GetDC(hWnd);
		viewDesc.mWidth = gWidth;
		viewDesc.mHeight = gHeight;
		oglview = new COglView(&viewDesc);

		// Initialize OpenGL
		oglview->init();

		return 0;
    case WM_DESTROY: 

        PostQuitMessage(0);
		
		printf("done.\n");
        return 0;
    case WM_SIZE:
        //resize(LOWORD(lParam), HIWORD(lParam));
		oglview->resize(gWidth, gHeight);
		return 0;
    case WM_CHAR:
        switch ((int)wParam) {
        case VK_ESCAPE:
			gbDone = true;
            return 0;
        default:
            break;
        }
        break;
    default:
        break;
    }

    /* Deal with any unprocessed messages */
    return DefWindowProc(hWnd, message, wParam, lParam);

}
#endif

#if defined (AJALinux)
//
// Wait for notify event.
//
static Bool
WaitForNotify(Display * d, XEvent * e, char *arg)
{
  return (e->type == MapNotify) && (e->xmap.window == (Window) arg);
}
#endif

void initWindow(void)
{
#ifdef AJA_WINDOWS
    WNDCLASS wndClass;
    static char *className = "GPUD4V";
    HINSTANCE hInstance = GetModuleHandle(NULL);
    RECT rect;
    DWORD dwStyle = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    /* Define and register the window class */
    wndClass.style = CS_OWNDC;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance,
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = NULL;
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = className;
    RegisterClass(&wndClass);

    /* Figure out a default size for the window */
//    SetRect(&rect, 0, 0, gWidth, gHeight);
	SetRect(&rect, 0, 0, 1920, 1080);
	AdjustWindowRect(&rect, dwStyle, FALSE);

    /* Create a window of the previously defined class */
    hWnd = CreateWindow(
        className, "GPUDirect for Video OpenGL Example", dwStyle,
        rect.left, rect.top, rect.right, rect.bottom,
        NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd,SW_SHOW);
#else
    int screen;
    XVisualInfo *vi;
    XSetWindowAttributes swa;
    XEvent event;
    Colormap cmap;
    unsigned long mask;
    GLXFBConfig *configs, config;
    int numConfigs;
    int config_list[] = { GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, 
                          GLX_DOUBLEBUFFER, GL_TRUE,
                          GLX_RENDER_TYPE, GLX_RGBA_BIT,
                          GLX_RED_SIZE, 8,
                          GLX_GREEN_SIZE, 8,
                          GLX_BLUE_SIZE, 8,
                          GLX_FLOAT_COMPONENTS_NV, GL_FALSE,
                          None };

    // Notify Xlib that the app is multithreaded.
    XInitThreads();

    // Open X display
    dpy = XOpenDisplay(NULL);

    if (!dpy) {
        cout << "Error: could not open display" << endl;
        exit(1);
    }

    // Get screen.
    screen = DefaultScreen(dpy);
  
    // Find required framebuffer configuration
    configs = glXChooseFBConfig(dpy, screen, config_list, &numConfigs);
  
    if (!configs) {
        cout << "CreatePBuffer(): Unable to find a matching FBConfig." << endl;
        exit(1);
    }

    // Find a config with the right number of color bits.
    int i;
    for (i = 0; i < numConfigs; i++) {
        int attr;
    
        if (glXGetFBConfigAttrib(dpy, configs[i], GLX_RED_SIZE, &attr)) {
            cout << "glXGetFBConfigAttrib(GLX_RED_SIZE) failed!" << endl;
            exit(1);
        }
        if (attr != 8)
            continue;
    
        if (glXGetFBConfigAttrib(dpy, configs[i], GLX_GREEN_SIZE, &attr)) {
            cout << "glXGetFBConfigAttrib(GLX_GREEN_SIZE) failed!" << endl;
            exit(1);
        }
        if (attr != 8)
            continue;
    
        if (glXGetFBConfigAttrib(dpy, configs[i], GLX_BLUE_SIZE, &attr)) {
            cout << "glXGetFBConfigAttrib(GLX_BLUE_SIZE) failed!" << endl;
            exit(1);
        }
        if (attr != 8)
            continue;
    
        if (glXGetFBConfigAttrib(dpy, configs[i], GLX_ALPHA_SIZE, &attr)) {
            cout << "glXGetFBConfigAttrib(GLX_ALPHA_SIZE) failed" << endl;
            exit(1);
        }
        if (attr != 8)
            continue;
    
        break;
    }
  
    if (i == numConfigs) {
        cout << "No 8-bit FBConfigs found." << endl;
        exit(1);
    }
  
    config = configs[i];
  
    // Don't need the config list anymore so free it.
    XFree(configs);
    configs = NULL;
  
    // Create a context for the onscreen window.
    ctx = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, 0, true);
  
    // Get visual from FB config.
    if ((vi = glXGetVisualFromFBConfig(dpy, config)) == NULL) {
        cout << "Couldn't find visual for onscreen window." << endl;
        exit(1);
    }

    // Create color map.
    if (!(cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
			         vi->visual, AllocNone))) {
        cout << "XCreateColormap failed!" << endl;
        exit(1);
    }
  
    // Create window.
    swa.colormap = cmap;
    swa.border_pixel = 0;
    swa.background_pixel = 1;
    swa.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask |
                     KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                     PointerMotionMask ;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
    win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 
		        0, 0, gWidth, gHeight, 0,
		        vi->depth, InputOutput, vi->visual,
		        mask, &swa);
  
    // Map window.
    XMapWindow(dpy, win);
    XIfEvent(dpy, &event, WaitForNotify, (char *) win);
  
    // Set window colormap.
    XSetWMColormapWindows(dpy, win, &win, 1);

    // Make OpenGL rendering context current
    glXMakeCurrent(dpy, win, ctx);

    // Create OpenGL view
    oglViewDesc viewDesc;
    viewDesc.dpy = dpy;
    viewDesc.win = win;
    viewDesc.ctx = ctx;
	viewDesc.mWidth = gWidth;
    viewDesc.mHeight = gHeight;

    oglview = new COglView(&viewDesc);

    // Initialize OpenGL
    oglview->init();
#endif
}

void drawApp(int source, int target)
{
	int width = gWidth;
	int height = gHeight;

	static 	float angle = 0.0;
	static int frame_count = 0;

	// Increment rotation angle
	frame_count += 1;
	angle = frame_count * 3.1415927f / 90.0f;

	double halfWinWidth = width / 2.0;
	double halfWinHeight = height / 2.0;
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, width, height);

	assert(glGetError() == GL_NO_ERROR);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor3f(1.0f, 1.0f, 1.0f);

	// Draw the background as the source texture and render some geometry on top
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	assert(glGetError() == GL_NO_ERROR);

	//  Use GLSL output shader program
	glUseProgram(videoRenderProgram);

	// Set uniform variables
	GLfloat view_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);
	glUniformMatrix4fv(glGetUniformLocation(videoRenderProgram, "ViewMatrix"), 1, GL_FALSE, view_matrix);

	GLfloat proj_matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, proj_matrix);
	glUniformMatrix4fv(glGetUniformLocation(videoRenderProgram, "ProjMatrix"), 1, GL_FALSE, proj_matrix);

	GLuint tex;
	tex = glGetUniformLocation(videoRenderProgram, "tex");
	glUniform1i(tex, 0);

	// Bind texture object 	
	glBindTexture(GL_TEXTURE_2D, source);

	glBindBuffer(GL_ARRAY_BUFFER, videoVertexBufferObject);
	glEnableVertexAttribArray(videoPosition);
	glVertexAttribPointer(
		videoPosition,               /* attribute */
		2,                           /* size */
		GL_FLOAT,                    /* type */
		GL_FALSE,                    /* normalized? */
		sizeof(MyVertex),            /* stride */
		(void*)0                     /* array buffer offset */
		);

	glEnableVertexAttribArray(videoTexCoord);
	glVertexAttribPointer(
		videoTexCoord,              /* attribute */
		2,                          /* size */
		GL_FLOAT,                   /* type */
		GL_FALSE,                   /* normalized? */
		sizeof(MyVertex),           /* stride */
		(void*)8                    /* array buffer offset */
		);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, videoElementBufferObject);
	glDrawElements(
		GL_TRIANGLE_STRIP,          /* mode */
		4,                          /* count */
		GL_UNSIGNED_SHORT,          /* type */
		(void*)0                    /* element array buffer offset */
		);

	glDisableVertexAttribArray(videoPosition);
	glDisableVertexAttribArray(videoTexCoord);

	glClear(GL_DEPTH_BUFFER_BIT);

	assert(glGetError() == GL_NO_ERROR);

	glUseProgram(geomRenderProgram);

	// Draw the geometry	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-halfWinWidth, halfWinWidth, halfWinHeight, -halfWinHeight, -1000.0, 1000.0),
		    gluLookAt(0, 0.5, 1, 0, 0, 0, 0, 1, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	assert(glGetError() == GL_NO_ERROR);

	// Set uniform variables
	glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);
	glUniformMatrix4fv(glGetUniformLocation(geomRenderProgram, "ViewMatrix"), 1, GL_FALSE, view_matrix);

	glGetFloatv(GL_PROJECTION_MATRIX, proj_matrix);
	glUniformMatrix4fv(glGetUniformLocation(geomRenderProgram, "ProjMatrix"), 1, GL_FALSE, proj_matrix);

	GLuint loc;
	loc = glGetUniformLocation(geomRenderProgram, "angle");
	glUniform1f(loc, angle);

	GLuint tex2;
	tex2 = glGetUniformLocation(geomRenderProgram, "logotex");
	glUniform1i(tex2, 0);

	assert(glGetError() == GL_NO_ERROR);

	// Bind texture object 	
	glBindTexture(GL_TEXTURE_2D, logoTex);

	glBindBuffer(GL_ARRAY_BUFFER, geomVertexBufferObject);
	glEnableVertexAttribArray(geomPosition);
	glVertexAttribPointer(
		geomPosition,               /* attribute */
		2,                          /* size */
		GL_FLOAT,                   /* type */
		GL_FALSE,                   /* normalized? */
		sizeof(MyVertex),           /* stride */
		(void*)0                    /* array buffer offset */
		);

	glEnableVertexAttribArray(geomTexCoord);
	glVertexAttribPointer(
		geomTexCoord,               /* attribute */
		2,                          /* size */
		GL_FLOAT,                   /* type */
		GL_FALSE,                   /* normalized? */
		sizeof(MyVertex),           /* stride */
		(void*)8                    /* array buffer offset */
		);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geomElementBufferObject);
	glDrawElements(
		GL_TRIANGLE_STRIP,          /* mode */
		4,                          /* count */
		GL_UNSIGNED_SHORT,          /* type */
		(void*)0                    /* element array buffer offset */
		);

	glDisableVertexAttribArray(geomPosition);
	glDisableVertexAttribArray(geomTexCoord);

	glUseProgram(0);

	assert(glGetError() == GL_NO_ERROR);
}

GLuint compileShader(GLuint *vertShader, const GLchar *vertShaderStr,
	                 GLuint *fragShader, const GLchar *fragShaderStr)
{
	GLint val;
	GLint res;
	GLuint outputProgram = 0;

	// Create GLSL vertex shader to blit the composited texture
	// image into the OpenGL view graphics window.
	*vertShader = glCreateShader(GL_VERTEX_SHADER);

	// Initialize GLSL vertex shader
	glShaderSourceARB(*vertShader, 1, (const GLchar **)&vertShaderStr, NULL);

	// Compile vertex shader
	glCompileShaderARB(*vertShader);

	// Check for errors
	glGetShaderiv(*vertShader, GL_COMPILE_STATUS, &val);
	if (!val) {
		char infoLog[10000];
		glGetShaderInfoLog(*vertShader, 10000, NULL, infoLog);
		odprintf("Failed to load GLSL vertex shader, INFO:\n\n%s\n", infoLog);
		return 0;
	}

	// Create GLSL fragment shader to blit the composited texture
	// image into the OpenGL view graphics window.
	*fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Initialize GLSL shader for output image texture from FBO
	glShaderSourceARB(*fragShader, 1, (const GLchar **)&fragShaderStr, NULL);

	// Compile fragment shader
	glCompileShaderARB(*fragShader);

	// Check for errors
	glGetShaderiv(*fragShader, GL_COMPILE_STATUS, &val);
	if (!val) {
		char infoLog[10000];
		glGetShaderInfoLog(*fragShader, 10000, NULL, infoLog);
		odprintf("Failed to load GLSL fragment shader for output image texture from FBO, INFO:\n\n%s\n", infoLog);
		return 0;
	}

	// Create shader program for output image texture from FBO
	outputProgram = glCreateProgram();

	// Attach vertex shader to program
	glAttachShader(outputProgram, *vertShader);

	// Attach fragment shader to program
	glAttachShader(outputProgram, *fragShader);

	// Link shader program
	glLinkProgram(outputProgram);

	// Check for link errors
	glGetProgramiv(outputProgram, GL_LINK_STATUS, &res);
	if (!res) {
		odprintf("Failed to link GLSL output image texture program\n");
		GLint infoLength;
		glGetProgramiv(outputProgram, GL_INFO_LOG_LENGTH, &infoLength);
		if (infoLength) {
			char *buf;
			buf = (char *)malloc(infoLength);
			if (buf) {
				glGetProgramInfoLog(outputProgram, infoLength, NULL, buf);
				odprintf("Program Log: \n");
				odprintf("%s", buf);
				free(buf);
			}
		}
		return 0;
	}

	return outputProgram;
}

//
// initApp() - Initialize application processing state
//             Assumes OpenGL already initialized.
//
void initApp()
{
	// Compile shader program for rendering background video
	videoRenderProgram = compileShader(&videoVertShader, VideoVertexShader,
		                               &videoFragShader, VideoFragmentShader);
	assert(videoRenderProgram);

	// Query attributes
	videoPosition = glGetAttribLocation(videoRenderProgram, "in_vert_coord");
	videoTexCoord = glGetAttribLocation(videoRenderProgram, "in_tex_coord");

	// Compile shader program for rendering spinning quad geometry
	geomRenderProgram = compileShader(&geomVertShader, GeomVertexShader,
		                              &geomFragShader, GeomFragmentShader);
	assert(geomRenderProgram);

	// Query attributes
	geomPosition = glGetAttribLocation(geomRenderProgram, "in_vert_coord");
	geomTexCoord = glGetAttribLocation(geomRenderProgram, "in_tex_coord");

	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_DEPTH_TEST);

	glColor3f(1.0, 1.0, 1.0);

	glClearColor(.5, .5, .5, 1.0);
	glClearDepth(1.0);

	// Create vertex buffer object for rendering the video
	glGenBuffers(1, &videoVertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, videoVertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_video_vertex_buffer_data), g_video_vertex_buffer_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Create element buffer object for rendering the video
	glGenBuffers(1, &videoElementBufferObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, videoElementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_video_element_buffer_data), g_video_element_buffer_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create vertex buffer object for rendering the geometry
	glGenBuffers(1, &geomVertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, geomVertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_geom_vertex_buffer_data), g_geom_vertex_buffer_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Create element buffer object for rendering the geometry
	glGenBuffers(1, &geomElementBufferObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geomElementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_geom_element_buffer_data), g_geom_element_buffer_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create logo texture
	glGenTextures(1, &logoTex);
	glBindTexture(GL_TEXTURE_2D, logoTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	assert(glGetError() == GL_NO_ERROR);

	// Load logo texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gimp_image.width, gimp_image.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, gimp_image.pixel_data);
	assert(glGetError() == GL_NO_ERROR);

#ifdef AJA_WINDOWS
	// Create bitmap font display list
	SelectObject(GetDC(hWnd), GetStockObject(SYSTEM_FONT));
	SetDCBrushColor(GetDC(hWnd), 0x0000ffff);  // 0x00bbggrr
	glColor3f(1.0f, 0.0f, 0.0);
	wglUseFontBitmaps(GetDC(hWnd), 0, 255, 1000);
#endif
	// Create timer query
	glGenQueries(1, &drawTimeQuery);
	assert(GL_NO_ERROR == glGetError());
	glBeginQuery(GL_TIME_ELAPSED_EXT, drawTimeQuery);
	glEndQuery(GL_TIME_ELAPSED_EXT);
}


void closeApp() 
{
	glDeleteQueries(1, &drawTimeQuery);
}

typedef struct captureArgs {
	CGpuVideoIO *vio;
} captureArgs;

typedef struct playoutArgs {
	CGpuVideoIO *vio;
} playoutArgs;

//-----------------------------------------------------------------------------
// Name: Capture()
// Desc: Capture thread function
//-----------------------------------------------------------------------------
void	CaptureThreadFunc(AJAThread * pThread, void * pContext)
{
	CGpuVideoIO* vio = (CGpuVideoIO*)(pContext);

	// Initialization
	vio->GetGpuTransfer()->ThreadPrep();

	// Signal capture thread is ready
	gCaptureThreadReady.Signal();

	// Loop until destroy event signaled
	bool bDone = false;
	while (!bDone) {
		AJAStatus status =  gCaptureThreadDestroy.WaitForSignal(0);
		if (AJA_STATUS_SUCCESS == status)
		{
			bDone = true;
			vio->GetGpuCircularBuffer()->Abort();
			break;
		}

        int64_t start, end;
		start = AJATime::GetSystemMicroseconds();

		// Do frame capture
		vio->Capture();

		end = AJATime::GetSystemMicroseconds();
		durationCaptureCPU = (float)(end - start);
		durationCaptureCPU /= 1000.0f;
	}

	// Cleanup
	vio->GetGpuTransfer()->ThreadCleanup();

	gCaptureThreadDone.Signal();

//	return false;
}

//-----------------------------------------------------------------------------
// Name: Playout()
// Desc: Playout thread function
//-----------------------------------------------------------------------------
void	PlayoutThreadFunc(AJAThread * pThread, void * pContext)
{

	CGpuVideoIO* vio = (CGpuVideoIO*)(pContext);

	// Initialization
	vio->GetGpuTransfer()->ThreadPrep();

	// Signal capture thread is ready
	gPlayoutThreadReady.Signal();

	// Loop until destroy event signaled
	bool bDone = false;
	while (!bDone) {
		AJAStatus status = gPlayoutThreadDestroy.WaitForSignal(0);
		if (AJA_STATUS_SUCCESS == status)
		{
			bDone = true;
			vio->GetGpuCircularBuffer()->Abort();
			break;
		}

        int64_t start, end;
		start = AJATime::GetSystemMicroseconds();

		// Do frame playout
		vio->Playout();

		end = AJATime::GetSystemMicroseconds();
		durationPlayoutCPU = (float)(end - start);
		durationPlayoutCPU /= 1000.0f;
	}

	// Cleanup
	vio->GetGpuTransfer()->ThreadCleanup();

	gPlayoutThreadDone.Signal();

}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
#ifdef AJA_WINDOWS
int WINAPI WinMain( HINSTANCE hInstance,
		    HINSTANCE hPrevInstance,
		    LPSTR     lpCmdLine,
		    int       nCmdShow )
#else
int main(int argc, char *argv[])
#endif
{
	//////////////////////////////////
	odprintf("OGL Example v0.2");

#ifdef AJA_WINDOWS
	HANDLE hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
#endif
	// Just default to current input.
	// No dynamic checking of inputs.
	NTV2VideoFormat videoFormat;
	CNTV2Card ntv2Card(0);
	if (ntv2Card.IsOpen() == false)
		return false;
	ntv2Card.SetMultiFormatMode(false);

	ntv2Card.SetSDITransmitEnable(NTV2_CHANNEL1, false);
	ntv2Card.SetSDITransmitEnable(NTV2_CHANNEL2, false);
	ntv2Card.SetSDITransmitEnable(NTV2_CHANNEL3, false);
	ntv2Card.SetSDITransmitEnable(NTV2_CHANNEL4, false);

	videoFormat = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1,true);
	NTV2VideoFormat vf2 = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI2,true);
	NTV2VideoFormat vf3 = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI3, true);
	NTV2VideoFormat vf4 = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI4, true);

	if ((videoFormat == vf2) && (videoFormat == vf3) && (videoFormat == vf4))
	{
		NTV2VideoFormat fourKFormat = videoFormat;
		if (get4KInputFormat(fourKFormat))
			videoFormat = fourKFormat;
		//NOTE: check for Corvid88 or 8 channel board.
	}

	if (videoFormat == NTV2_FORMAT_UNKNOWN)
		videoFormat = NTV2_FORMAT_720p_5994;

	NTV2FrameBufferFormat frameBufferFormat = NTV2_FBF_ABGR;
	NTV2FormatDescriptor fd (videoFormat, frameBufferFormat);
	gWidth = fd.numPixels;
	gHeight = fd.numLines;

	// Create Window
    initWindow();

    // Initialize application
    initApp();

    // Create GPU circular buffers
    inGpuCircularBuffer = new CNTV2GpuCircularBuffer();
    outGpuCircularBuffer = new CNTV2GpuCircularBuffer();

    // Initialize video input
    vioDesc indesc;
    indesc.videoFormat = videoFormat;
	indesc.bufferFormat = frameBufferFormat;
    indesc.channel = NTV2_CHANNEL1;
    indesc.type = VIO_IN;
    capture = new CGpuVideoIO(&indesc);

    // Assign GPU circular buffer for input
    capture->SetGpuCircularBuffer(inGpuCircularBuffer);

	// Initialize video output
	vioDesc outdesc;
	outdesc.videoFormat = videoFormat;
	outdesc.bufferFormat = frameBufferFormat;
	if (NTV2_IS_QUAD_FRAME_FORMAT(videoFormat))
		outdesc.channel = NTV2_CHANNEL5; // QuadHD or 4K
	else
		outdesc.channel = NTV2_CHANNEL3;  

	outdesc.type = VIO_OUT;
	playout = new CGpuVideoIO(&outdesc);

	// Assign GPU circular buffer for output
	playout->SetGpuCircularBuffer(outGpuCircularBuffer);

	ULWord numFramesIn = RING_BUFFER_SIZE;
	inGpuCircularBuffer->Allocate(numFramesIn,gWidth*gHeight*4,
		gWidth, gHeight, false, false, 4096);

	ULWord numFramesOut = RING_BUFFER_SIZE;
	outGpuCircularBuffer->Allocate(numFramesOut, gWidth*gHeight*4,
		gWidth, gHeight, false, /*false*/true, 4096);

	// Initialize DVP transfer
	gpuTransfer = CreateNTV2glTextureTransferNV();
	
	gpuTransfer->Init();
	gpuTransfer->SetSize(gWidth, gHeight);
	gpuTransfer->SetNumChunks(1);

	// Assign DVP transfer
	capture->SetGpuTransfer(gpuTransfer);
	playout->SetGpuTransfer(gpuTransfer);

	// Register textures and buffers with DVP transfer
	for( ULWord i = 0; i < numFramesIn; i++ ) {
		gpuTransfer->RegisterTexture(inGpuCircularBuffer->mAVTextureBuffers[i].texture);
		gpuTransfer->RegisterInputBuffer((uint8_t*)(inGpuCircularBuffer->mAVTextureBuffers[i].videoBuffer));
	}
	
	for( ULWord i = 0; i < numFramesOut; i++ ) {
		gpuTransfer->RegisterTexture(outGpuCircularBuffer->mAVTextureBuffers[i].texture);
		gpuTransfer->RegisterOutputBuffer((uint8_t*)(outGpuCircularBuffer->mAVTextureBuffers[i].videoBuffer));
	}


	// Wait for capture to start
	capture->WaitForCaptureStart();

	//
	// Create capture thread
	//
	CaptureThread.Attach(CaptureThreadFunc, capture);
	CaptureThread.SetPriority(AJA_ThreadPriority_High);
	CaptureThread.Start();

	// Wait for initialization of the capture thread to complete before proceeding
#ifdef AJA_WINDOWS
	gCaptureThreadReady.WaitForSignal(INFINITE);
#else
	gCaptureThreadReady.WaitForSignal(0);
#endif

	//
	// Create playout thread
	//
	PlayoutThread.Attach(PlayoutThreadFunc, playout);
	PlayoutThread.SetPriority(AJA_ThreadPriority_High);
	PlayoutThread.Start();

	// Wait for initialization of the capture thread to complete before proceeding
#ifdef AJA_WINDOWS
	gPlayoutThreadReady.WaitForSignal(INFINITE);
#else
	gPlayoutThreadReady.WaitForSignal(0);
#endif

	gbDone = false;
	while (!gbDone) {
#ifdef AJA_WINDOWS
		MSG        msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE) {
			if (GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
#else
                XEvent event;
                while(XCheckWindowEvent(dpy, win, 0xffffffff, &event) == true) {
                    switch(event.type) {
                    case KeyPress:
	            {
	                XKeyPressedEvent *kpe  = (XKeyPressedEvent *)&event;
                        //	printf("keycode = %d\n", kpe->keycode);
	                if (kpe->keycode == 9) {
			    gbDone = true;
	                }
	            }
	            break;
                    case ConfigureNotify:
	            {
	                XConfigureEvent *ce = (XConfigureEvent *)&event;
			oglview->resize(ce->width, ce->height);
			  //width = ce->width;
			  //height = ce->height;
	            }
	            break;
                    default:
	                ;
	                //	printf("Event: %d\n", event.type);
                    } // switch
                 }  // XCheckWindowEvent
#endif
		// Render

		// Get input buffer
		AVTextureBuffer* inFrameData = inGpuCircularBuffer->StartConsumeNextBuffer();
		CNTV2Texture* inTexture = inFrameData->texture;

		// This gives the time to upload to the GPU for some past frame
		durationCaptureGPU = gpuTransfer->GetCardToGpuTime(inTexture);
        int64_t start, end;
		start = AJATime::GetSystemMicroseconds();

		// Get output buffer
		AVTextureBuffer* outFrameData = outGpuCircularBuffer->StartProduceNextBuffer();
		outFrameData->currentTime = inFrameData->currentTime;
		CNTV2RenderToTexture *outRenderToTexture = outFrameData->renderToTexture;
		// This gives the time to doawnload from the GPU for some past frame
		durationPlayoutGPU = gpuTransfer->GetGpuToCardTime(outRenderToTexture->GetTexture());

		// Measure the actual time it took to draw on the GPU. This result is for the previous frame
		GLint queryAvailable = 0;
		glGetQueryObjectiv(drawTimeQuery, GL_QUERY_RESULT_AVAILABLE, &queryAvailable);
		assert(glGetError() == GL_NO_ERROR);
		if (queryAvailable)
		{
			GLuint64EXT timeElapsed = 0;
			glGetQueryObjectui64vEXT(drawTimeQuery, GL_QUERY_RESULT, &timeElapsed);
			assert(glGetError() == GL_NO_ERROR);
			durationDrawGPU = timeElapsed*.000001f;
		}

		gpuTransfer->AcquireTexture(inTexture);
		gpuTransfer->AcquireTexture(outFrameData->texture);
		glBeginQuery(GL_TIME_ELAPSED_EXT, drawTimeQuery);
		assert(glGetError() == GL_NO_ERROR);
		outRenderToTexture->Begin();

		drawApp((int)(inTexture->GetIndex()), (int)(outRenderToTexture->GetTexture()->GetIndex()));

		outRenderToTexture->End();
		glEndQuery(GL_TIME_ELAPSED_EXT);
		assert(glGetError() == GL_NO_ERROR);
		gpuTransfer->ReleaseTexture(outFrameData->texture);
		gpuTransfer->ReleaseTexture(inTexture);

		windowTexture = outRenderToTexture->GetTexture()->GetIndex();

		end = AJATime::GetSystemMicroseconds();
		durationDrawCPU = (float)(end - start);
		durationDrawCPU /= 1000.0f;

		// Blit rendered results to onscreen window
		oglview->render(windowTexture,
			durationCaptureGPU, durationDrawGPU, durationPlayoutGPU);

		// Release input buffer
		inGpuCircularBuffer->EndConsumeNextBuffer();

		// Release output buffer
		outGpuCircularBuffer->EndProduceNextBuffer();
	} // while !gbDone

	// Terminate capture and playout threads
	gPlayoutThreadDestroy.Signal();
	gCaptureThreadDestroy.Signal();
	gPlayoutThreadDone.WaitForSignal();
	gCaptureThreadDone.WaitForSignal();

	if (inGpuCircularBuffer) {
		inGpuCircularBuffer->Abort();
	}

	if (outGpuCircularBuffer) {
		outGpuCircularBuffer->Abort();
	}

	delete capture;

	delete playout;

	// Unregister textures and buffers with DVP transfer
	for (ULWord i = 0; i < numFramesIn; i++) {
		gpuTransfer->UnregisterTexture(inGpuCircularBuffer->mAVTextureBuffers[i].texture);
		gpuTransfer->UnregisterInputBuffer((uint8_t*)(inGpuCircularBuffer->mAVTextureBuffers[i].videoBuffer));
	}

	for (ULWord i = 0; i < numFramesOut; i++) {
		gpuTransfer->UnregisterTexture(outGpuCircularBuffer->mAVTextureBuffers[i].texture);
		gpuTransfer->UnregisterOutputBuffer((uint8_t*)(outGpuCircularBuffer->mAVTextureBuffers[i].videoBuffer));
	}

	gpuTransfer->Destroy();

	delete gpuTransfer;

	closeApp();

	oglview->uninit();

	delete oglview;

#ifdef AJA_WINDOWS
	ReleaseDC(hWnd, GetDC(hWnd));

	DestroyWindow(hWnd);
#endif

	return TRUE;
}

static bool get4KInputFormat(NTV2VideoFormat & videoFormat)
{
	bool	status(false);
	struct	VideoFormatPair
	{
		NTV2VideoFormat	vIn;
		NTV2VideoFormat	vOut;
	} VideoFormatPairs[] = {	// vIn							// vOut
								{ NTV2_FORMAT_1080psf_2398,		NTV2_FORMAT_4x1920x1080psf_2398 },
								{ NTV2_FORMAT_1080psf_2400,		NTV2_FORMAT_4x1920x1080psf_2400 },
								{ NTV2_FORMAT_1080p_2398,		NTV2_FORMAT_4x1920x1080p_2398 },
								{ NTV2_FORMAT_1080p_2400,		NTV2_FORMAT_4x1920x1080p_2400 },
								{ NTV2_FORMAT_1080p_2500,		NTV2_FORMAT_4x1920x1080p_2500 },
								{ NTV2_FORMAT_1080p_2997,		NTV2_FORMAT_4x1920x1080p_2997 },
								{ NTV2_FORMAT_1080p_3000,		NTV2_FORMAT_4x1920x1080p_3000 },
								{ NTV2_FORMAT_1080p_5000_B,		NTV2_FORMAT_4x1920x1080p_5000 },
								{ NTV2_FORMAT_1080p_5994_B,		NTV2_FORMAT_4x1920x1080p_5994 },
								{ NTV2_FORMAT_1080p_6000_B,		NTV2_FORMAT_4x1920x1080p_6000 },
								{ NTV2_FORMAT_1080p_2K_2398,	NTV2_FORMAT_4x2048x1080p_2398 },
								{ NTV2_FORMAT_1080p_2K_2400,	NTV2_FORMAT_4x2048x1080p_2400 },
								{ NTV2_FORMAT_1080p_2K_2500,	NTV2_FORMAT_4x2048x1080p_2500 },
								{ NTV2_FORMAT_1080p_2K_2997,	NTV2_FORMAT_4x2048x1080p_2997 },
								{ NTV2_FORMAT_1080p_2K_3000,	NTV2_FORMAT_4x2048x1080p_3000 },
								{ NTV2_FORMAT_1080p_2K_5000_A,	NTV2_FORMAT_4x2048x1080p_5000 },
								{ NTV2_FORMAT_1080p_2K_5994_A,	NTV2_FORMAT_4x2048x1080p_5994 },
								{ NTV2_FORMAT_1080p_2K_6000_A,	NTV2_FORMAT_4x2048x1080p_6000 },
						
								{ NTV2_FORMAT_1080p_5000_A,		NTV2_FORMAT_4x1920x1080p_5000 },
								{ NTV2_FORMAT_1080p_5994_A,		NTV2_FORMAT_4x1920x1080p_5994 },
								{ NTV2_FORMAT_1080p_6000_A,		NTV2_FORMAT_4x1920x1080p_6000 },
						
								{ NTV2_FORMAT_1080p_2K_5000_A,	NTV2_FORMAT_4x2048x1080p_5000 },
								{ NTV2_FORMAT_1080p_2K_5994_A,	NTV2_FORMAT_4x2048x1080p_5994 },
								{ NTV2_FORMAT_1080p_2K_6000_A,	NTV2_FORMAT_4x2048x1080p_6000 }
	};

	for (size_t formatNdx = 0; formatNdx < sizeof (VideoFormatPairs) / sizeof (VideoFormatPair); formatNdx++)
	{
		if (VideoFormatPairs[formatNdx].vIn == videoFormat)
		{
			videoFormat = VideoFormatPairs[formatNdx].vOut;
			status = true;
		}
	}

	return status;

}	//	get4KInputFormat

