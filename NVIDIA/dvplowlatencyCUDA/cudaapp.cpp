
/********************************************************************************************
oglapp:
	A CUDA example that demonstrates usage of GPU Direct for Video for low latency video I/O + gpu processing. 
	The example is hard-coded for 720p 59.94Hz video format.
	The demo expects a 720p 59.94 video on channel 1(if bidirectional the code makes it an input)
	The demo outputs the composite on channel 3(if bidirectional the code makes it an output)
	The amount of rendering in the sample will determine the in-out latency. With the current rendering load, the latency is 
	at 3 frames. 
***********************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS 1
#include "ajatypes.h"

#include "event.h"
#include "thread.h"
#include "systemtime.h"

#include <math.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <signal.h>

#define USE_WINDOW
#define DO_EFFECT

#include "cudaUtils.h"

#ifdef USE_WINDOW
//  Includes standard OpenGL headers.  Include before CUDA<>OpenGL interop headers.
#include "oglview.h"
// Include this here rather than in cudaUtils to prevent everything that includes cudaUtils.h from needing to include gl.h
#include <cudaGL.h>
#include <cuda_gl_interop.h>
#else
#pragma comment(linker, "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
#include <string.h>
#endif

#include "simplecudavio.h"


// CUDA Function Declaration
extern "C" void CopyVideoInputToOuput(cudaArray *pIn, cudaArray *pOut,
	                                  unsigned int width, unsigned int height);

extern "C" void ConvertRGBAToGreyscale(cudaArray *rgbaImage,
	                                   cudaArray *greyImage,
	                                   unsigned int width,
	                                   unsigned int height);

extern "C" void DoSobel(cudaArray *greyImage,
	                    cudaArray *outImage,
	                    unsigned int width,
	                    unsigned int height,
	                    unsigned int widthStep);

using namespace std;
const int RING_BUFFER_SIZE = 2;

int gWidth;
int gHeight;

#ifdef USE_WINDOW
#ifdef AJA_WINDOWS
static HWND hWnd;
#else
Display *dpy;
Window win;
GLXContext ctx;
#endif

// OpenGL view object
COglView *oglview;
GLuint viewTex;
cudaGraphicsResource *viewTexInCuda;
#endif

#ifndef TRUE
#define TRUE 1
#endif

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

// CUDA
CUcontext cudaCtx;
cudaChannelFormatDesc cudaChannelDesc;

static bool get4KInputFormat(NTV2VideoFormat & videoFormat);

void closeApp();

void SignalHandler(int signal)
{
	(void) signal;
	gbDone = true;
}

#if defined(USE_WINDOW) && defined(AJA_WINDOWS)
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

#if defined(USE_WINDOW) && !defined(AJA_WINDOWS)
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
#ifdef USE_WINDOW
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
        className, "GPUDirect for Video CUDA Example", dwStyle,
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
}

//
// initApp() - Initialize application processing state
//             Assumes OpenGL already initialized.
//
void initApp()
{
	// Initialize CUDA state
	initCUDA();

#ifdef USE_WINDOW
	// Create texture to blit into OpenGL view
	glGenTextures(1, &viewTex);
	glBindTexture(GL_TEXTURE_2D, viewTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	assert(glGetError() == GL_NO_ERROR);

	// Initialize view texture
	GLubyte* data_ptr = NULL;
	const GLubyte* data;
	ULWord size = gWidth * gHeight * 4;  // Assuming RGBA
	data_ptr = (UByte*)malloc(size * sizeof(float));
	memset(data_ptr, 0, size);
	data = data_ptr;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gWidth, gHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	assert(glGetError() == GL_NO_ERROR);

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
	SetDCBrushColor(GetDC(hWnd), 0x0000ffff);  // 0x00bbggrr
	glColor3f(1.0f, 0.0f, 0.0);
	wglUseFontBitmaps(GetDC(hWnd), 0, 255, 1000);
#endif
#endif
}


void closeApp() 
{
#ifdef USE_WINDOW
	// Unregister view texture with CUDA
	checkCudaErrors(cudaGraphicsUnregisterResource(viewTexInCuda));

	// Delete OpenGL view texture
	glDeleteTextures(1, &viewTex);
#endif
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

		__int64 start, end;
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

		__int64 start, end;
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
// Name: WinMain() / main()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
#if defined(USE_WINDOW) && defined(AJA_WINDOWS)
int WINAPI WinMain( HINSTANCE hInstance,
		    HINSTANCE hPrevInstance,
		    LPSTR     lpCmdLine,
		    int       nCmdShow )
#else
int main(int argc, char *argv[])
#endif
{
	odprintf("CUDA Example v0.2");

#if defined(USE_WINDOW) && defined(AJA_WINDOWS)
	HANDLE hThread = GetCurrentThread();
	SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
#endif

	gbDone = false;

	signal(SIGINT, SignalHandler);

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
    indesc.videoFormat = videoFormat;
	indesc.bufferFormat = frameBufferFormat;
    indesc.channel = NTV2_CHANNEL1;
    indesc.type = VIO_IN;
    capture = new CCudaVideoIO(&indesc);

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
	playout = new CCudaVideoIO(&outdesc);

	// Assign GPU circular buffer for output
	playout->SetGpuCircularBuffer(outGpuCircularBuffer);

	ULWord numFramesIn = RING_BUFFER_SIZE;
	inGpuCircularBuffer->Allocate(numFramesIn,gWidth*gHeight*4,
		gWidth, gHeight, false, false, 4096, NTV2_TEXTURE_TYPE_CUDA_ARRAY);

	ULWord numFramesOut = RING_BUFFER_SIZE;
	outGpuCircularBuffer->Allocate(numFramesOut, gWidth*gHeight * 4,
		gWidth, gHeight, false, /*false*/true, 4096, NTV2_TEXTURE_TYPE_CUDA_ARRAY);

	// Initialize input DVP transfer
	gpuTransferIN = CreateNTV2cudaArrayTransferNV();
	
	gpuTransferIN->Init();
	gpuTransferIN->SetSize(gWidth, gHeight);
	gpuTransferIN->SetNumChunks(1);

	// Initialize output DVP transfer
	gpuTransferOUT = CreateNTV2cudaArrayTransferNV();

	gpuTransferOUT->Init();
	gpuTransferOUT->SetSize(gWidth, gHeight);
	gpuTransferOUT->SetNumChunks(1);

	// Assign DVP transfer
	capture->SetGpuTransfer(gpuTransferIN);
	playout->SetGpuTransfer(gpuTransferOUT);

	// Register textures and buffers with DVP transfer
	for( ULWord i = 0; i < numFramesIn; i++ ) {
		gpuTransferIN->RegisterTexture(inGpuCircularBuffer->mAVTextureBuffers[i].texture);
		gpuTransferIN->RegisterInputBuffer((uint8_t*)(inGpuCircularBuffer->mAVTextureBuffers[i].videoBuffer));
	}
	
	for( ULWord i = 0; i < numFramesOut; i++ ) {
		gpuTransferOUT->RegisterTexture(outGpuCircularBuffer->mAVTextureBuffers[i].texture);
		gpuTransferOUT->RegisterOutputBuffer((uint8_t*)(outGpuCircularBuffer->mAVTextureBuffers[i].videoBuffer));
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

	// Allocate CUDA array for linearized 32F RAW image in device memory
	cudaArray *rawImageArray;
	cudaChannelFormatDesc cudaChannelDescFloat;
	cudaChannelDescFloat = cudaCreateChannelDesc(32, 0, 0, 0, cudaChannelFormatKindFloat);
	checkCudaErrors(cudaMallocArray(&rawImageArray, &cudaChannelDescFloat, gWidth, gHeight, cudaArraySurfaceLoadStore));

	while (!gbDone) {
#ifdef USE_WINDOW
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
#endif
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

		gpuTransferIN->AcquireTexture(inTexture);
		gpuTransferOUT->AcquireTexture(outFrameData->texture);

#ifdef USE_WINDOW
		// Map OpenGL view texture into CUDA
		checkCudaErrors(cudaGraphicsMapResources(1, &viewTexInCuda, 0));
		cudaArray *mappedOGLViewTextureArray;
		checkCudaErrors(cudaGraphicsSubResourceGetMappedArray(&mappedOGLViewTextureArray, (cudaGraphicsResource_t)viewTexInCuda, 0, 0));
#endif
		// Do CUDA processing on input video frame
		cudaEventRecord(start, 0);

#ifdef DO_EFFECT
		// Convert frame to monochrome
		ConvertRGBAToGreyscale(inTexture->GetCudaArray(), rawImageArray, gWidth, gHeight);

		// Perform Sobel edge detection
		int monochromeImagePitch = sizeof(float) * gWidth;
		DoSobel(rawImageArray,
			    outTexture->GetCudaArray(),
			    gWidth,
				gHeight,
				monochromeImagePitch);
#else
		//  Execute CUDA kernel here to copy result to OpenGL view texture.
		CopyVideoInputToOuput(inTexture->GetCudaArray(), outTexture->GetCudaArray(), gWidth, gHeight);
#endif

		cudaEventRecord(stop, 0);
		cudaEventSynchronize(stop);

#ifdef USE_WINDOW
		//  Execute CUDA kernel here to copy result to OpenGL view texture.
		CopyVideoInputToOuput(outTexture->GetCudaArray(), mappedOGLViewTextureArray, gWidth, gHeight);

		// Unmap OpenGL capture texture from CUDA
		cudaGraphicsUnmapResources(1, &viewTexInCuda, 0);
#endif
		gpuTransferOUT->ReleaseTexture(outFrameData->texture);
		gpuTransferIN->ReleaseTexture(inTexture);	

		// Calculate GPU processing time
		cudaEventElapsedTime(&durationProcessGPU, start, stop); 

#ifdef USE_WINDOW
		// Blit rendered results to onscreen window
		oglview->render(viewTex,
			durationCaptureGPU, durationProcessGPU, durationPlayoutGPU);
#endif
		// Release input buffer
		inGpuCircularBuffer->EndConsumeNextBuffer();

		// Release output buffer
		outGpuCircularBuffer->EndProduceNextBuffer();
	} // while !gbDone

	// Destroy CUDA timing events
	cudaEventDestroy(start); 
	cudaEventDestroy(stop);

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

#ifdef USE_WINDOW
	oglview->uninit();
	delete oglview;

#ifdef AJA_WINDOWS
	ReleaseDC(hWnd, GetDC(hWnd));
	DestroyWindow(hWnd);
#endif
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

