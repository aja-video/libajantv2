
/********************************************************************************************
cudaapp:
	A CUDA example that demonstrates usage of GPU Direct for Video for low latency video I/O + gpu processing. 
	The example is hard-coded for 720p 59.94Hz video format.
	The demo expects a 720p 59.94 video on channel 1(if bidirectional the code makes it an input)
	The demo outputs the composite on channel 3(if bidirectional the code makes it an output)
	The amount of rendering in the sample will determine the in-out latency. With the current rendering load, the latency is 
	at 3 frames. 
***********************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS 1
#define no_init_all

#include "ajatypes.h"

#include "ajabase/system/event.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/systemtime.h"
#ifdef AJA_LINUX
#include "ntv2democommon.h"
#endif

#include <math.h>
#include <iostream>
#include <fstream>
#include <assert.h>

#if defined(AJALinux)
#include <string.h>
#endif

//  Includes standard OpenGL headers.  Include before CUDA<>OpenGL interop headers.
#include "oglview.h"

#include "cudaUtils.h"

// Include this here rather than in cudaUtils to prevent everything that includes cudaUtils.h from needing to include gl.h
#include <cudaGL.h>
#include <cuda_gl_interop.h>

#include "ntv2cudaArrayTransferNV.h"
#include "simplecudavio.h"

#include "oglview.h"
 
// CUDA Function Declaration
extern "C" void CopyVideoInputToOuput(cudaSurfaceObject_t inputSurfObj, 
	                                  cudaSurfaceObject_t outputSurfObj,
	                                  unsigned int width, unsigned int height);

extern "C" void CudaProcessRGB16(cudaSurfaceObject_t inputSurfObj, 
	                             cudaSurfaceObject_t outputSurfObj,
	                             unsigned int width,
	                             unsigned int height);

extern "C" void CudaProcessRGB10(cudaSurfaceObject_t inputSurfObj, 
	                             cudaSurfaceObject_t outputSurfObj,
	                             unsigned int width, unsigned int height);

using namespace std;
const int RING_BUFFER_SIZE = 2;

int gWidth;
int gHeight;

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
CCudaVideoIO *capture;
CCudaVideoIO *playout;

// GPU Circular Buffers
CNTV2GpuCircularBuffer *inGpuCircularBuffer;
CNTV2GpuCircularBuffer *outGpuCircularBuffer;

// DVP transfer
CNTV2cudaArrayTransferNV* gpuTransferIN;
CNTV2cudaArrayTransferNV* gpuTransferOUT;

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
float durationProcessGPU;

GLuint viewTex;
cudaGraphicsResource *viewTexInCuda;

// CUDA
CUcontext cudaCtx;
cudaChannelFormatDesc cudaChannelDesc;
cudaChannelFormatDesc cudaChannelDescIN;
cudaChannelFormatDesc cudaChannelDescOUT;
#if 0
static bool get4KInputFormat(NTV2VideoFormat & videoFormat);
#endif
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
        className, "NTV2 Capture To CUDA Example", dwStyle,
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

//
// initCUDA() - Initialize application CUDA processing state
//
void initCUDA()
{
	// Initialize CUDA
	CUCHK(cuInit(0));

	// Create CUDA context
	CUCHK(cuCtxCreate(&cudaCtx, 0, 0));

	// Make CUDA context current
	CUCHK(cuCtxSetCurrent(cudaCtx));

	// Allocate 8-bit 4-component CUDA arrays in device memory
	cudaChannelDesc = cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);

	// Input is 16-bit 1-component CUDA array in device memory.
	cudaChannelDescIN = cudaCreateChannelDesc(16, 0, 0, 0, cudaChannelFormatKindUnsigned);

	// Output is 32-bit float 4 component CUDA array in device memory.
	cudaChannelDescOUT = cudaCreateChannelDesc(32, 32, 32, 32, cudaChannelFormatKindFloat);
}

//
// initApp() - Initialize application processing state
//             Assumes OpenGL already initialized.
//
void initApp()
{
	// Initialize CUDA state
	initCUDA();

	// Create texture to blit into OpenGL view
	glGenTextures(1, &viewTex);
	glBindTexture(GL_TEXTURE_2D, viewTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	assert(glGetError() == GL_NO_ERROR);

	// Initialize view texture
	GLubyte* data_ptr = NULL;
//	const GLubyte* data;
	ULWord size = gWidth * gHeight * 4;  // Assuming RGBA
	data_ptr = (UByte*)malloc(size * sizeof(float));
	memset(data_ptr, 0, size);
//	data = data_ptr;

	glTexImage2D(
		GL_TEXTURE_2D, 0,           /* target, level */
		GL_RGBA32F,                    /* internal format */
		gWidth, gHeight, 0,         /* width, height, border */
		GL_RGBA, GL_FLOAT,       /* external format, type */
		NULL
	);

	if (data_ptr)
	{
		free(data_ptr);
		data_ptr = NULL;
	}

	// Register view texture with CUDA
	checkCudaErrors(cudaGraphicsGLRegisterImage(&viewTexInCuda, viewTex, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsSurfaceLoadStore));

#ifdef AJA_WINDOWS
	// Create bitmap font display list.  This must be done here because the oglview object does not know the hWnd.
	SelectObject(GetDC(hWnd), GetStockObject(SYSTEM_FONT));
	SetDCBrushColor(GetDC(hWnd), 0x0000ff00);  // 0x00bbggrr
	glColor3f(1.0f, 0.0f, 0.0);
	wglUseFontBitmaps(GetDC(hWnd), 0, 255, 1000);
#endif
}


void closeApp() 
{
	// Unregister view texture with CUDA
	checkCudaErrors(cudaGraphicsUnregisterResource(viewTexInCuda));

	// Delete OpenGL view texture
	glDeleteTextures(1, &viewTex);
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

		ULWord64 start, end;
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

		ULWord64 start, end;
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
int main(int argc, const char *argv[])
#endif
{
	//////////////////////////////////
#ifdef AJA_RDMA
	cout << "CUDA/RDMA Example v0.4" << endl;
#else	
	cout << "CUDA Example v0.4" << endl;
#endif
	
#ifdef AJA_WINDOWS
	HANDLE hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
#endif

	uint32_t deviceIndex(0);	// default device index
	uint32_t inputIndex(0);		// default SDI input index
	uint32_t outputIndex(1);	// default SDI output index

#ifdef AJA_LINUX
	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"device",		'd',	POPT_ARG_INT,	&deviceIndex,	0,	"which device",			"index"	},
		{"input",		'i',	POPT_ARG_INT,	&inputIndex,	0,	"which SDI input",		"index"	},
		{"output",		'o',	POPT_ARG_INT,	&outputIndex,	0,	"which SDI output",		"index"	},
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}
#endif

	NTV2VideoFormat videoFormat(NTV2_FORMAT_UNKNOWN);
	CNTV2Card ntv2Card(deviceIndex);
	if (ntv2Card.IsOpen() == false)
		{ cerr << "## ERROR: cannot open device index " << deviceIndex << endl;  return 2; }

	NTV2TaskMode savedTaskMode(NTV2_DISABLE_TASKS);
	ntv2Card.GetEveryFrameServices(savedTaskMode);
	ntv2Card.SetEveryFrameServices(NTV2_OEM_TASKS);

	NTV2Channel inputChannel = (NTV2Channel)inputIndex;
//	NTV2Channel outputChannel = (NTV2Channel)outputIndex;
	
	ntv2Card.SetMultiFormatMode(false);
	ntv2Card.SetSDITransmitEnable(inputChannel, false);

	uint64_t sTime = AJATime::GetSystemMilliseconds();
	videoFormat = ntv2Card.GetInputVideoFormat(::NTV2ChannelToInputSource(inputChannel), true);
	while (videoFormat == NTV2_FORMAT_UNKNOWN)
	{
		if ((AJATime::GetSystemMilliseconds() - sTime) > 1000)
			break;
		AJATime::Sleep(50);
		videoFormat = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1,true);
	}
	
	if (videoFormat == NTV2_FORMAT_UNKNOWN)
		{ cerr << "## ERROR: no input detected device index " << deviceIndex << " channel index " << inputIndex << endl;  return 2; }

	//  This will work with devices that answer \c true for ::NTV2DeviceCanDo3GLevelConversion
	bool    is3Gb(false);
	ntv2Card.GetSDIInput3GbPresent(is3Gb, inputChannel);
	if (is3Gb)
	{
		switch (videoFormat)
		{
		case NTV2_FORMAT_1080i_5000:
			ntv2Card.SetSDIInLevelBtoLevelAConversion(inputChannel, true);
			videoFormat = NTV2_FORMAT_1080p_5000_A;
			break;
		case NTV2_FORMAT_1080i_5994:
			ntv2Card.SetSDIInLevelBtoLevelAConversion(inputChannel, true);
			videoFormat = NTV2_FORMAT_1080p_5994_A;
			break;
		case NTV2_FORMAT_1080i_6000:
			ntv2Card.SetSDIInLevelBtoLevelAConversion(inputChannel, true);
			videoFormat = NTV2_FORMAT_1080p_6000_A;
			break;
		default:
			ntv2Card.SetSDIInLevelBtoLevelAConversion(inputChannel, false);
			break;
		}
	}
	else
	{
		ntv2Card.SetSDIInLevelBtoLevelAConversion(inputChannel, false);
	}

	ntv2Card.SetReference(::NTV2InputSourceToReferenceSource(::NTV2ChannelToInputSource(inputChannel)));
	ntv2Card.SetVideoFormat (videoFormat);

	// For this application hardcode the frame buffer format to RGB10
	NTV2FrameBufferFormat frameBufferFormat = NTV2_FBF_10BIT_RGB;

	NTV2FormatDescriptor fd(videoFormat, frameBufferFormat);
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
	indesc.deviceIndex = deviceIndex;
    indesc.videoFormat = videoFormat;
	indesc.bufferFormat = frameBufferFormat;
    indesc.channel = NTV2_CHANNEL1;
    indesc.type = VIO_IN;
    capture = new CCudaVideoIO(&indesc);

	// Initialize video output
	vioDesc outdesc;
	outdesc.deviceIndex = deviceIndex;
	outdesc.videoFormat = videoFormat;
	outdesc.bufferFormat = frameBufferFormat;
	outdesc.channel = NTV2_CHANNEL2;
	outdesc.type = VIO_OUT;
	playout = new CCudaVideoIO(&outdesc);

	ULWord numFramesIn = RING_BUFFER_SIZE;
	ULWord numFramesOut = RING_BUFFER_SIZE;
	// For RGB16, input buffer is 6 bytes / pixels, otherwise assume 4 bytes / pixel
	// Will need to fix this for the RGB12 case.
	if (frameBufferFormat == NTV2_FBF_48BIT_RGB)
	{
		inGpuCircularBuffer->Allocate(numFramesIn, gWidth*gHeight * 6,
			gWidth, gHeight, false, false, 4096, NTV2_TEXTURE_TYPE_CUDA_ARRAY);
		outGpuCircularBuffer->Allocate(numFramesOut, gWidth * gHeight * 6,
			gWidth, gHeight, false, /*false*/true, 4096, NTV2_TEXTURE_TYPE_CUDA_ARRAY);
	}
	else
	{
		inGpuCircularBuffer->Allocate(numFramesIn, gWidth*gHeight * 4,
			gWidth, gHeight, false, false, 4096, NTV2_TEXTURE_TYPE_CUDA_ARRAY);
		outGpuCircularBuffer->Allocate(numFramesOut, gWidth * gHeight * 4,
			gWidth, gHeight, false, /*false*/true, 4096, NTV2_TEXTURE_TYPE_CUDA_ARRAY);
	}

    // Assign GPU circular buffer for input
    capture->SetGpuCircularBuffer(inGpuCircularBuffer);

	// Assign GPU circular buffer for output
	playout->SetGpuCircularBuffer(outGpuCircularBuffer);

	// Initialize input transfer
	gpuTransferIN = CreateNTV2cudaArrayTransferNV();
	
	gpuTransferIN->Init();
	gpuTransferIN->SetSize(gWidth, gHeight);
	gpuTransferIN->SetNumChunks(1);

	// Assign input transfer
	capture->SetGpuTransfer(gpuTransferIN);

	// Initialize output transfer
	gpuTransferOUT = CreateNTV2cudaArrayTransferNV();

	gpuTransferOUT->Init();
	gpuTransferOUT->SetSize(gWidth, gHeight);
	gpuTransferOUT->SetNumChunks(1);

	// Assign output transfer
	playout->SetGpuTransfer(gpuTransferOUT);

	// Register textures and buffers for input transfer
	for( ULWord i = 0; i < numFramesIn; i++ ) 
	{
		gpuTransferIN->RegisterTexture(inGpuCircularBuffer->mAVTextureBuffers[i].texture);
	}

	// Register textures and buffers for output transfer
	for (ULWord i = 0; i < numFramesOut; i++) 
	{
		gpuTransferOUT->RegisterTexture(outGpuCircularBuffer->mAVTextureBuffers[i].texture);
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

	// Create CUDA timing events
	cudaEvent_t start, stop;;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);

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
		durationCaptureGPU = gpuTransferIN->GetCardToGpuTime(inTexture);

		// Get output buffer
		AVTextureBuffer* outFrameData = outGpuCircularBuffer->StartProduceNextBuffer();
		outFrameData->currentTime = inFrameData->currentTime;
		CNTV2Texture* outTexture = outFrameData->texture;

		// This gives the time to download from the GPU for some past frame
		durationPlayoutGPU = gpuTransferOUT->GetGpuToCardTime(outTexture);

		// Acquire GPU texture from input FIFO
		gpuTransferIN->AcquireTexture(inTexture);

		// Acquire GPU texture from output FIFO
		gpuTransferOUT->AcquireTexture(outFrameData->texture);

		// Map OpenGL view texture into CUDA
		checkCudaErrors(cudaGraphicsMapResources(1, &viewTexInCuda, 0));
		cudaArray *mappedOGLViewTextureArray;
		checkCudaErrors(cudaGraphicsSubResourceGetMappedArray(&mappedOGLViewTextureArray, (cudaGraphicsResource_t)viewTexInCuda, 0, 0));

		// Specify CUDA surface object type
		struct cudaResourceDesc resDesc;
		memset(&resDesc, 0, sizeof(resDesc));
		resDesc.resType = cudaResourceTypeArray;

		// Create CUDA surface objects
		resDesc.res.array.array = inTexture->GetCudaArray();
		cudaSurfaceObject_t inputSurfObj = 0;
		checkCudaErrors(cudaCreateSurfaceObject(&inputSurfObj, &resDesc));
		resDesc.res.array.array = mappedOGLViewTextureArray;
		cudaSurfaceObject_t outputSurfObj = 0;
		checkCudaErrors(cudaCreateSurfaceObject(&outputSurfObj, &resDesc));
		resDesc.res.array.array = outTexture->GetCudaArray();
		cudaSurfaceObject_t outputSurfObj2 = 0;
		checkCudaErrors(cudaCreateSurfaceObject(&outputSurfObj2, &resDesc));

		// Do CUDA processing on input video frame
		cudaEventRecord(start, 0);

		// Convert RGB16 data and copy to mapped 8-bit OpenGL view texture
		CudaProcessRGB10(inputSurfObj, outputSurfObj, gWidth, gHeight);

		// Execute CUDA kernel here to copy result to OpenGL view texture.
		CopyVideoInputToOuput(inputSurfObj, outputSurfObj2, gWidth, gHeight);

		cudaEventRecord(stop, 0);
		cudaEventSynchronize(stop);

		// Destroy surface objects
		checkCudaErrors(cudaDestroySurfaceObject(inputSurfObj));
		checkCudaErrors(cudaDestroySurfaceObject(outputSurfObj));
		checkCudaErrors(cudaDestroySurfaceObject(outputSurfObj2));

		// Unmap OpenGL capture texture from CUDA
		cudaGraphicsUnmapResources(1, &viewTexInCuda, 0);

		// Release GPU texture in output FIFO
		gpuTransferOUT->ReleaseTexture(outFrameData->texture);

		// Release GPU texture in input FIFO
		gpuTransferIN->ReleaseTexture(inTexture);	

		// Calculate GPU processing time
		cudaEventElapsedTime(&durationProcessGPU, start, stop); 

		// Blit rendered results to onscreen window
		oglview->render(viewTex,
			durationCaptureGPU, durationProcessGPU, durationPlayoutGPU);

		// Release input buffer
		inGpuCircularBuffer->EndConsumeNextBuffer();

		// Release output buffer
		outGpuCircularBuffer->EndProduceNextBuffer();

	} // while !gbDone

	// Destroy CUDA timing events
	cudaEventDestroy(start); 
	cudaEventDestroy(stop);

	// Terminate capture thread
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
		gpuTransferIN->UnregisterTexture(inGpuCircularBuffer->mAVTextureBuffers[i].texture);
		gpuTransferIN->UnregisterInputBuffer((uint8_t*)(inGpuCircularBuffer->mAVTextureBuffers[i].videoBuffer));
	}

	for (ULWord i = 0; i < numFramesOut; i++) {
		gpuTransferOUT->UnregisterTexture(outGpuCircularBuffer->mAVTextureBuffers[i].texture);
		gpuTransferOUT->UnregisterOutputBuffer((uint8_t*)(outGpuCircularBuffer->mAVTextureBuffers[i].videoBuffer));
	}

	gpuTransferIN->Destroy();
	gpuTransferOUT->Destroy();

	delete gpuTransferIN;
	delete gpuTransferOUT;

	closeApp();

	oglview->uninit();

	delete oglview;

#ifdef AJA_WINDOWS
	ReleaseDC(hWnd, GetDC(hWnd));

	DestroyWindow(hWnd);
#endif

	ntv2Card.SetEveryFrameServices(savedTaskMode);

	return TRUE;
}
#if 0
static bool get4KInputFormat(NTV2VideoFormat & videoFormat)
{
	bool	status(false);
	struct	VideoFormatPair
	{
		NTV2VideoFormat	vIn;
		NTV2VideoFormat	vOut;
	}  VideoFormatPairs[] = {	// vIn							// vOut
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
#endif
