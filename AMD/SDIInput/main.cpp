/**
@file		AMDSDIInput/main.cpp
@brief		Demonstration application to capture frames from SDI to GPU using GMA.
@copyright	Copyright (C) 2012-2017 AJA Video Systems, Inc.  All rights reserved.
**/

#include <string>

#include <GL/glew.h>
#include <GL/wglew.h>

#include "resource.h"
#include "AJA_SDIInOut.h"
#include "GLSink.h"


PFNGLWAITMARKERAMDPROC          glWaitMarkerAMD;
PFNGLWRITEMARKERAMDPROC         glWriteMarkerAMD;
PFNGLMAKEBUFFERSRESIDENTAMDPROC glMakeBuffersResidentAMD;
PFNGLBUFFERBUSADDRESSAMDPROC    glBufferBusAddressAMD;

#define NUM_BUFFERS 3

HWND			g_hWnd;
HDC				g_hDC;

// Define initial window size
int g_nWidth  = 960;
int g_nHeight = 540;

GLSink*         g_pSink     = NULL;
AJA_SDIInOut*   g_pSDIInOut = NULL;


bool             OpenWindow(LPCSTR cClassName, LPCSTR cWindowName );
HGLRC            createContext();
void			 CloseWindow();
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



void MyDebugFunc(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
    MessageBox(NULL, message, "GL Debug Message", MB_ICONWARNING | MB_OK);
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    bool            bDone = false;
    WNDCLASSEX      wndclass;
    const LPCSTR    cClassName  = "OGL";
    const LPCSTR    cWindowName = "SDI Playback";

    // Register WindowClass
    wndclass.cbSize         = sizeof(WNDCLASSEX);
    wndclass.style          = CS_OWNDC;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = (HINSTANCE)GetModuleHandle(NULL);
    wndclass.hIcon          = (HICON)LoadImage(hInst,  MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, LR_DEFAULTSIZE, LR_DEFAULTSIZE, NULL);// LoadIcon(NULL, "IDI_ICON1");
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = NULL;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = cClassName;
    wndclass.hIconSm        = (HICON)LoadImage(hInst,  MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, LR_DEFAULTSIZE, LR_DEFAULTSIZE, NULL);

    if (!RegisterClassEx(&wndclass))
        return FALSE;

    // Indicet if p2p is to be used
    bool bUseP2P = true;

    // Indicate if AutoCirculate is to be used
    bool bUseAutoCirculate = false;

    // Create interface to AJA SDI board 0
    g_pSDIInOut = new AJA_SDIInOut(0);

    if (!g_pSDIInOut->openCard(NUM_BUFFERS, bUseAutoCirculate, bUseP2P))
        bDone = false;

//	NTV2FrameBufferFormat fbFormat = NTV2_FBF_24BIT_RGB;
	NTV2FrameBufferFormat fbFormat = NTV2_FBF_ABGR;
	bool bQuad = true;
	
	// Create first sdi input channel. This one will have the id 0
    // 0:                   Use SDI In 1
    // NTV2_FBF_24BIT_RGB:  RGB8 FB
    if (!g_pSDIInOut->setupInputChannel(0, fbFormat, bQuad))
    {
         MessageBox(NULL, "No Input signal on SDI Input 1!", cWindowName, MB_OK | MB_ICONERROR);
         return WM_QUIT;
    }

    // Get information on FB configuration on the SDI board. The formats on both
    // inputs needs to be identical, therefore we read only the config of input 0.
    // The Kona3G currently supports only one VideoFormat on both inputs. So forcing
    // identical FB configs is no real constraint.
    int          nIntFormat = g_pSDIInOut->getIntFormat(0);
    int          nExtFormat = g_pSDIInOut->getExtFormat(0);
    int          nType      = g_pSDIInOut->getType(0);
    unsigned int uiFBWidth  = g_pSDIInOut->getFramebufferWidth(0);
    unsigned int uiFBHeight = g_pSDIInOut->getFramebufferHeight(0);

    // Set window size according to SDI input signal
    g_nWidth  = uiFBWidth;
    g_nHeight = uiFBHeight;

	if (g_nWidth > 1500)
	{
		g_nWidth /= 2;
		g_nHeight /= 2;
	}
	if (g_nWidth > 1500)
	{
		g_nWidth /= 2;
		g_nHeight /= 2;
	}

    // Open window and create GL context
    if (!OpenWindow(cClassName, cWindowName))
        return WM_QUIT;

    // check if AMD_pinned_memory and AMD_BUS_ADDRESSABLE_MEMORY are supported
    int             nNumExtensions = 0;
    bool            bPinnedMemory         = false;
    bool            bBusAddressableMemory = false;
    std::string     strExt;

    glGetIntegerv(GL_NUM_EXTENSIONS, &nNumExtensions);

    for (int i = 0; (i < nNumExtensions) && !(bPinnedMemory && bBusAddressableMemory); ++i)
    {
        strExt = (const char*)glGetStringi(GL_EXTENSIONS, i);

        if (strExt == "GL_AMD_pinned_memory")
        {
            bPinnedMemory = true;
        }
        if (strExt == "GL_AMD_bus_addressable_memory")
        {
            bBusAddressableMemory = true;

            // Load extension
            glMakeBuffersResidentAMD = (PFNGLMAKEBUFFERSRESIDENTAMDPROC) wglGetProcAddress("glMakeBuffersResidentAMD");
            glBufferBusAddressAMD    = (PFNGLBUFFERBUSADDRESSAMDPROC)    wglGetProcAddress("glBufferBusAddressAMD");
            glWaitMarkerAMD          = (PFNGLWAITMARKERAMDPROC)          wglGetProcAddress("glWaitMarkerAMD");
            glWriteMarkerAMD         = (PFNGLWRITEMARKERAMDPROC)         wglGetProcAddress("glWriteMarkerAMD");
        }
    }

    if (bPinnedMemory && bBusAddressableMemory)
    {
        // Create GL sink that will display textures received from SDI
        g_pSink = new GLSink;
        g_pSink->initGL();
        g_pSink->resize(g_nWidth, g_nHeight);

        // Create a synchronized and pinned buffer to downstream texture data to the gpu
        g_pSink->createDownStream(NUM_BUFFERS, uiFBWidth, uiFBHeight, nIntFormat, nExtFormat, nType, bUseP2P);

        // connect input buffer 0 with sdi channel id 0
        g_pSDIInOut->setSyncBuffer(0, g_pSink->getInputBuffer());
        
        // Start capture thread
        g_pSDIInOut->start();
    }
    else
    {
        bDone = true;
    }

    while (!bDone)
    {
        MSG  Msg;

        g_pSink->draw();
        SwapBuffers(g_hDC);

        if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            if (Msg.message == WM_QUIT)
            {
                CloseWindow();
                bDone = true;
            }
            else
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    if (g_pSink)
        g_pSink->release();

    g_pSDIInOut->stop();
    g_pSDIInOut->closeCard();

    delete g_pSink;
    delete g_pSDIInOut;

    CloseWindow();

    UnregisterClass(cClassName, hInst);

    return WM_QUIT;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int  nLastx     = 0;
    static int  nLasty     = 0;

    switch (uMsg)
    {
        char c;

        case WM_CHAR:
            c = (char)wParam;

		    switch (c)
		    {   
            case VK_ESCAPE:
                PostQuitMessage(0);
                break;
		    }

		    return 0;

      case WM_CREATE:
          return 0;

	    case WM_SIZE:
          g_nWidth  = LOWORD(lParam);
          g_nHeight = HIWORD(lParam);

          if (g_pSink)
              g_pSink->resize(g_nWidth, g_nHeight);
		    
		    return 0;

	    case WM_DESTROY:
          PostQuitMessage(0);
		      return 0;

	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


bool OpenWindow(LPCSTR cClassName, LPCSTR cWindowName )
{
    int	    mPixelFormat;
    RECT    WinSize;

    // Adjust window size so that the ClientArea has the initial size
    // of gWidth and gHeight
    WinSize.bottom = g_nHeight; 
    WinSize.left   = 0;
    WinSize.right  = g_nWidth;
    WinSize.top    = 0;

    AdjustWindowRect(&WinSize, WS_OVERLAPPEDWINDOW, false);

    g_nWidth  = WinSize.right  - WinSize.left;
    g_nHeight = WinSize.bottom - WinSize.top;    
    
    g_hWnd = CreateWindow(cClassName, 
						 cWindowName,
						 WS_OVERLAPPEDWINDOW,
						 0,
						 0,
						 g_nWidth,
						 g_nHeight,
						 NULL,
						 NULL,
						 (HINSTANCE)GetModuleHandle(NULL),
						 NULL);

    if (!g_hWnd)
        return FALSE;


    static PIXELFORMATDESCRIPTOR pfd;

    pfd.nSize           = sizeof(PIXELFORMATDESCRIPTOR); 
    pfd.nVersion        = 1; 
    pfd.dwFlags         = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL  | PFD_DOUBLEBUFFER ;
    pfd.iPixelType      = PFD_TYPE_RGBA; 
    pfd.cColorBits      = 24; 
    pfd.cRedBits        = 8; 
    pfd.cRedShift       = 0; 
    pfd.cGreenBits      = 8; 
    pfd.cGreenShift     = 0; 
    pfd.cBlueBits       = 8; 
    pfd.cBlueShift      = 0; 
    pfd.cAlphaBits      = 8;
    pfd.cAlphaShift     = 0; 
    pfd.cAccumBits      = 0; 
    pfd.cAccumRedBits   = 0; 
    pfd.cAccumGreenBits = 0; 
    pfd.cAccumBlueBits  = 0; 
    pfd.cAccumAlphaBits = 0; 
    pfd.cDepthBits      = 24; 
    pfd.cStencilBits    = 8; 
    pfd.cAuxBuffers     = 0; 
    pfd.iLayerType      = PFD_MAIN_PLANE;
    pfd.bReserved       = 0; 
    pfd.dwLayerMask     = 0;
    pfd.dwVisibleMask   = 0; 
    pfd.dwDamageMask    = 0;

    g_hDC = GetDC(g_hWnd);

    if (!g_hDC)
        return FALSE;

    mPixelFormat = ChoosePixelFormat(g_hDC, &pfd);

    if (!mPixelFormat)
        return FALSE;

    SetPixelFormat(g_hDC, mPixelFormat, &pfd);

    if (!createContext())
        return FALSE;

    ShowWindow(g_hWnd, SW_SHOW);

    UpdateWindow(g_hWnd);

    return TRUE;
}



HGLRC createContext()
{
    HGLRC  hGLRC;

    if (!g_hDC)
        return NULL;

    hGLRC = wglCreateContext(g_hDC);
    if (!hGLRC)
        return NULL;

    wglMakeCurrent( g_hDC, hGLRC );

    if (glewInit() != GLEW_OK)
        return NULL;

   
    if (WGLEW_ARB_create_context)
    {
        wglDeleteContext(hGLRC);

        int attribs[] = {
          WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
          WGL_CONTEXT_MINOR_VERSION_ARB, 2,
          WGL_CONTEXT_PROFILE_MASK_ARB , WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#ifdef DEBUG
          WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
          0
        }; 

        hGLRC = wglCreateContextAttribsARB(g_hDC, 0, attribs);

        if (hGLRC)
        {
            wglMakeCurrent(g_hDC, hGLRC);

            if (GLEW_AMD_debug_output)
                glDebugMessageCallbackAMD((GLDEBUGPROCAMD)&MyDebugFunc, NULL);

            return hGLRC;            
        }
    }

    return NULL;
}



void CloseWindow()
{
    if (g_hWnd)
    {
        ReleaseDC(g_hWnd, g_hDC);
        DestroyWindow(g_hWnd);
    }
}