/**
@file		AMDSDIOutput/main.cpp
@brief		Demonstration application to playback frames from GPU to SDI using GMA.
@copyright	Copyright (C) 2012-2018 AJA Video Systems, Inc.  All rights reserved.
**/

#include <string>

#include <GL/glew.h>
#include <GL/wglew.h>

#include "resource.h"
#include "SyncedBuffer.h"
#include "AJA_SDIInOut.h"
#include "GLSource.h"

PFNGLWAITMARKERAMDPROC          glWaitMarkerAMD;
PFNGLWRITEMARKERAMDPROC         glWriteMarkerAMD;
PFNGLMAKEBUFFERSRESIDENTAMDPROC glMakeBuffersResidentAMD;
PFNGLBUFFERBUSADDRESSAMDPROC    glBufferBusAddressAMD;


#define NUM_BUFFERS 3

HWND			g_hWnd = NULL;
HDC				g_hDC  = NULL;

// Define initial window size
int g_nWidth  = 1280;
int g_nHeight =  720;

GLSource*       g_pSource;
AJA_SDIInOut*   g_pSDIInOut = NULL;


HGLRC            createContext();
bool             OpenWindow(LPCSTR cClassName, LPCSTR cWindowName );
void             CloseWindow();
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


// OGL Debug function to catch gl errors
void MyDebugFunc(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
    MessageBox(NULL, message, "GL Debug Message", MB_ICONWARNING | MB_OK);
}



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    bool            bDone = false;
    WNDCLASSEX      wndclass;
    const LPCSTR    cClassName  = "OGL";
    const LPCSTR    cWindowName = "SDI Output";

    // Register WindowClass
    wndclass.cbSize         = sizeof(WNDCLASSEX);
    wndclass.style          = CS_OWNDC;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = (HINSTANCE)GetModuleHandle(NULL);
    wndclass.hIcon          = (HICON)LoadImage(hInst,  MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, LR_DEFAULTSIZE, LR_DEFAULTSIZE, NULL);
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = NULL;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = cClassName;
    wndclass.hIconSm        = (HICON)LoadImage(hInst,  MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, LR_DEFAULTSIZE, LR_DEFAULTSIZE, NULL);

    if (!RegisterClassEx(&wndclass))
        return WM_QUIT;

    // Indicet if p2p is to be used
    bool bUseP2P = true;

    // Indicate if AutoCirculate is to be used
    bool bUseAutoCirculate = true;

    // Create Interface class to AJA SDI board 0
    g_pSDIInOut = new AJA_SDIInOut(0);

    // open SDI board 0
    if (!g_pSDIInOut->openCard(NUM_BUFFERS, bUseAutoCirculate, bUseP2P))
        bDone = true;

//	NTV2VideoFormat vidFormat = NTV2_FORMAT_720p_5994;
//	NTV2VideoFormat vidFormat = NTV2_FORMAT_1080p_2398;
	NTV2VideoFormat vidFormat = NTV2_FORMAT_4x1920x1080p_2398;
//	NTV2VideoFormat vidFormat = NTV2_FORMAT_1080p_5994_A;

//	NTV2FrameBufferFormat fbFormat = NTV2_FBF_24BIT_RGB;
	NTV2FrameBufferFormat fbFormat = NTV2_FBF_ABGR;
   
    // Create sdi output channel. This one will have the id 0
    // 0:                   Use SDI Out 1
    // NTV2_FBF_24BIT_RGB:  RGB8 FB

	if (!g_pSDIInOut->setupOutputChannel(0, fbFormat, vidFormat))
	{
		bDone = true;
		MessageBox(NULL, "Could not configure SDI output!", cWindowName, MB_OK | MB_ICONERROR);
	}

    // Get information on FB configuratiomn of channel 0 of the SDI board. 
    int          nIntFormat = g_pSDIInOut->getIntFormat(0);
    int          nExtFormat = g_pSDIInOut->getExtFormat(0);
    int          nType      = g_pSDIInOut->getType(0);
    unsigned int uiFBWidth  = g_pSDIInOut->getFramebufferWidth(0);
    unsigned int uiFBHeight = g_pSDIInOut->getFramebufferHeight(0);

    // indicate that the window size will match the size of the SDI FB
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

    if (!OpenWindow(cClassName, cWindowName))
        bDone = true;

    // check if AMD_pinned_memory is supported
    std::string strExt = (char*)glGetString(GL_EXTENSIONS);

    if (strExt.find("AMD_pinned_memory") < strExt.size() && strExt.find("AMD_bus_addressable_memory") < strExt.size())
    {
        // Load extension
        glMakeBuffersResidentAMD = (PFNGLMAKEBUFFERSRESIDENTAMDPROC) wglGetProcAddress("glMakeBuffersResidentAMD");
        glBufferBusAddressAMD    = (PFNGLBUFFERBUSADDRESSAMDPROC)    wglGetProcAddress("glBufferBusAddressAMD");
        glWaitMarkerAMD          = (PFNGLWAITMARKERAMDPROC)          wglGetProcAddress("glWaitMarkerAMD");
        glWriteMarkerAMD         = (PFNGLWRITEMARKERAMDPROC)         wglGetProcAddress("glWriteMarkerAMD");

        // Create GL source to feed the SDI thread
        g_pSource = new GLSource;
        g_pSource->initGL();
        g_pSource->resize(uiFBWidth, uiFBHeight);

        // Create buffer that will contain the frame that will be transferred to the SDI device
        if (!g_pSource->createUpStream(NUM_BUFFERS, uiFBWidth, uiFBHeight, nIntFormat, nExtFormat, nType, bUseP2P))
        {
            bDone = true;
            MessageBox(NULL, "Could not create output buffer!", cWindowName, MB_OK | MB_ICONERROR);
        }

		g_pSource->resize(g_nWidth, g_nHeight);

		// Pass the output buffer of the viewer to SDI
        g_pSDIInOut->setSyncBuffer(0, g_pSource->getOutputBuffer());

        if (bUseP2P)
        {
            unsigned long long* pBufferBusAddress = NULL;
            unsigned long long* pMarkerBusAddress = NULL;

            // The SDI driver allocated memory on teh sdi device that will be used by OGL. Get the physical
            // address of those SDI surfaces for the surface and the marker.
            unsigned int uiNumRemoteBuffers = g_pSDIInOut->getBusAddresses(0, pBufferBusAddress, pMarkerBusAddress);

            // Hand the bus addresses to ogl. setRemoteMemory will assign them to teh ogl buffer objects 
            if (!g_pSource->setRemoteSDIMemory(uiNumRemoteBuffers, pBufferBusAddress, pMarkerBusAddress))
            {
                bDone = true;
                MessageBox(NULL, "Could not set remote memory!", cWindowName, MB_OK | MB_ICONERROR);
            }
        }
    }
    else
    {
        bDone = true;
    }

    if (!bDone)
    {
        // Start SDI output thread for channel 0
        g_pSDIInOut->start();
    }

    // Run message loop
    while (!bDone)
    {       
        MSG	    Msg;

        // Draw to Render target, transfer buffer to SDI and copy buffer into window
        g_pSource->draw();

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
   
    // Stop SDI channel thread
    g_pSDIInOut->stop();
    g_pSDIInOut->closeCard();

    delete g_pSDIInOut;
    delete g_pSource;

    return WM_QUIT;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int nLastx = 0;
    static int nLasty = 0;

    switch (uMsg)
    {
        char c;

        case WM_CHAR:
            c = (char)wParam;

            if (c == VK_ESCAPE)
                 PostQuitMessage(0);

		        return 0;


        case WM_CREATE:
            return 0;

        case WM_SIZE:
            g_nWidth  = LOWORD(lParam);
            g_nHeight = HIWORD(lParam);

            if (g_pSource)
                g_pSource->resize(g_nWidth, g_nHeight);
		    
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


