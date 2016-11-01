
#include "os_include.h"

#include <GL/glew.h>

#if defined(WIN32)
    #include <GL/wglew.h>
    #define GETPROC GetProcAddress
#elif defined (LINUX)
    #define GETPROC dlsym
#endif


#include "adl_prototypes.h"
#include "GLWindow.h"


using namespace std;


// Memory Allocation function needed for ADL
void* __stdcall ADL_Alloc ( int iSize )
{
    void* lpBuffer = malloc ( iSize );
    return lpBuffer;
}

// Memory Free function needed for ADL
void __stdcall ADL_Free ( void* lpBuffer )
{
    if ( NULL != lpBuffer )
    {
        free ( lpBuffer );
        lpBuffer = NULL;
    }
}


static ADLFunctions g_AdlCalls = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


#ifdef WIN32
void MyDebugFunc(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
    MessageBox(NULL, message, "GL Debug Message", MB_ICONWARNING | MB_OK);
}
#endif


GLWindow::GLWindow(const char *strWinName, const char* strClsaaName)
{
    m_strClassName = strClsaaName;
    m_strWinName   = strWinName;

    m_hDC   = NULL;
    m_hGLRC = NULL;
    m_hWnd  = NULL;

    m_uiWidth = 800;
    m_uiHeight = 600;

    m_uiPosX = 0;
    m_uiPosY = 0;

    m_uiNumGPU = 0;

    m_bADLReady   = false;
    m_bFullScreen = false;
}


GLWindow::~GLWindow(void)
{
    vector<DisplayData*>::iterator itr;

    for (itr = m_DisplayInfo.begin(); itr != m_DisplayInfo.end(); ++itr)
    {
        if ((*itr))
        {
            delete (*itr);
        }
    }

    if (g_AdlCalls.hDLL)
    {
        g_AdlCalls.ADL_Main_Control_Destroy();
    }

    destroy();
}


bool GLWindow::init()
{
    int				nNumDisplays = 0;
	int				nNumAdapters = 0;
    int             nCurrentBusNumber = 0;
	LPAdapterInfo   pAdapterInfo = NULL;
    unsigned int    uiCurrentGPUId     = 0;
    unsigned int    uiCurrentDisplayId = 0;

    // load all required ADL functions
    if (!setupADL())
        return false;

    // Determine how many adapters and displays are in the system
	g_AdlCalls.ADL_Adapter_NumberOfAdapters_Get(&nNumAdapters);

	if (nNumAdapters > 0)
	{
		pAdapterInfo = (LPAdapterInfo)malloc ( sizeof (AdapterInfo) * nNumAdapters );
        memset ( pAdapterInfo,'\0', sizeof (AdapterInfo) * nNumAdapters );
	}

	g_AdlCalls.ADL_Adapter_AdapterInfo_Get (pAdapterInfo, sizeof (AdapterInfo) * nNumAdapters);

    // Loop through all adapters 
	for (int i = 0; i < nNumAdapters; ++i)
	{
		int				nAdapterIdx; 
		int				nAdapterStatus;
		
		nAdapterIdx = pAdapterInfo[i].iAdapterIndex;

		g_AdlCalls.ADL_Adapter_Active_Get(nAdapterIdx, &nAdapterStatus);

		if (nAdapterStatus)
		{
			LPADLDisplayInfo	pDisplayInfo = NULL;

			g_AdlCalls.ADL_Display_DisplayInfo_Get(nAdapterIdx, &nNumDisplays, &pDisplayInfo, 0);

			for (int j = 0; j < nNumDisplays; ++j)
			{
				// check if display is connected and mapped
				if (pDisplayInfo[j].iDisplayInfoValue & ADL_DISPLAY_DISPLAYINFO_DISPLAYCONNECTED)
                {
					// check if display is mapped on adapter
					if (pDisplayInfo[j].iDisplayInfoValue & ADL_DISPLAY_DISPLAYINFO_DISPLAYMAPPED && pDisplayInfo[j].displayID.iDisplayLogicalAdapterIndex == nAdapterIdx)
					{
                        if (nCurrentBusNumber == 0)
                        {
                            // Found first GPU in the system
                            ++m_uiNumGPU;
                            nCurrentBusNumber = pAdapterInfo[nAdapterIdx].iBusNumber;
                        }
                        else if (nCurrentBusNumber != pAdapterInfo[nAdapterIdx].iBusNumber)
                        {
                            // found new GPU
                            ++m_uiNumGPU;
                            ++uiCurrentGPUId;
                            nCurrentBusNumber = pAdapterInfo[nAdapterIdx].iBusNumber;
                        }

                        ++uiCurrentDisplayId;

                        

                        // Found first mapped display
                        DisplayData* pDsp = new DisplayData;
                        
                        pDsp->uiGPUId               = uiCurrentGPUId;
                        pDsp->uiDisplayId           = uiCurrentDisplayId;
                        pDsp->uiDisplayLogicalId    = pDisplayInfo[j].displayID.iDisplayLogicalIndex;
                        pDsp->strDisplayname        = string(pAdapterInfo[i].strDisplayName);
                        pDsp->nOriginX              = 0;
                        pDsp->nOriginY              = 0;
                        pDsp->uiWidth               = 0;
                        pDsp->uiHeight              = 0;

#ifdef WIN32
                        DEVMODE DevMode;
                        EnumDisplaySettings(pAdapterInfo[i].strDisplayName, ENUM_CURRENT_SETTINGS, &DevMode);

                        pDsp->nOriginX             = DevMode.dmPosition.x;
                        pDsp->nOriginY             = DevMode.dmPosition.y;
                        pDsp->uiWidth               = DevMode.dmPelsWidth;
                        pDsp->uiHeight              = DevMode.dmPelsHeight;
#endif

                        m_DisplayInfo.push_back(pDsp);
                    }
                }
            }
        }
    }

    return true;
}


unsigned int GLWindow::getNumDisplaysOnGPU(unsigned int uiGPU)
{
    unsigned int uiNumDsp = 0;

    vector<DisplayData*>::iterator itr;

    for (itr = m_DisplayInfo.begin(); itr != m_DisplayInfo.end(); ++itr)
    {
        if ((*itr)->uiGPUId == uiGPU)
        {
            ++uiNumDsp;
        }
    }

    return uiNumDsp;
}



unsigned int GLWindow::getDisplayOnGPU(unsigned int uiGPU, unsigned int n)
{
    unsigned int uiCurrentDsp = 0;

    vector<DisplayData*>::iterator itr;

    for (itr = m_DisplayInfo.begin(); itr != m_DisplayInfo.end(); ++itr)
    {
        if ((*itr)->uiGPUId == uiGPU)
        {
            if (uiCurrentDsp == n)
            {
                return (*itr)->uiDisplayId;
            }

            ++uiCurrentDsp;
        }
    }

    return 0;
}


void GLWindow::resize(unsigned int uiWidth, unsigned int uiHeight)
{
    m_uiWidth  = uiWidth;
    m_uiHeight = uiHeight;
}



//----------------------------------------------------------------------------------------------
//  Windows implementation functions
//----------------------------------------------------------------------------------------------

#ifdef WIN32

bool GLWindow::create(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDspIndex, bool bDecoration)
{
    RECT        WinSize;
    DWORD		dwExStyle;
	DWORD		dwStyle;
	int			nPixelFormat;

    // Get information for the display with id uiDspId. This is the ID
    // shown by CCC. Use the origin of this display as base position for
    // opening the window. Like this a window can be opened on a particular
    // GPU.
    DisplayData* pDsp = getDisplayData(uiDspIndex);

    if (pDsp)
    {
        m_uiPosX += pDsp->nOriginX;
        m_uiPosY += pDsp->nOriginY;
    }


	if (m_bFullScreen)
	{
		dwExStyle = WS_EX_APPWINDOW;								
		dwStyle   = WS_POPUP;										
		//ShowCursor(FALSE);	
	}
	else
	{
        m_uiWidth  = uiWidth;
        m_uiHeight = uiHeight;

		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;	

        if (bDecoration)
        {
		    dwStyle   = WS_OVERLAPPEDWINDOW;

            // Adjust window size so that the ClientArea has the initial size
            // of uiWidth and uiHeight
            WinSize.bottom = uiHeight; 
            WinSize.left   = 0;
            WinSize.right  = uiWidth;
            WinSize.top    = 0;

            AdjustWindowRect(&WinSize, WS_OVERLAPPEDWINDOW, false);

            m_uiWidth  = WinSize.right  - WinSize.left;
            m_uiHeight = WinSize.bottom - WinSize.top;    
        }
        else
            dwStyle   = WS_POPUP;
	}

	m_hWnd = CreateWindowEx( dwExStyle,
						     m_strClassName.c_str(), 
						     m_strWinName.c_str(),
						     dwStyle,
						     m_uiPosX,
						     m_uiPosY,
						     m_uiWidth,
						     m_uiHeight,
						     NULL,
						     NULL,
						    (HINSTANCE)GetModuleHandle(NULL),
						     NULL);

	if (!m_hWnd)
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


	m_hDC = GetDC(m_hWnd);

	if (!m_hDC)
		return false;

	nPixelFormat = ChoosePixelFormat(m_hDC, &pfd);

    if (!nPixelFormat)
		return false;

	SetPixelFormat(m_hDC, nPixelFormat, &pfd);
	

	return true;
}


void GLWindow::destroy()
{
    if (m_hGLRC)
    {
        wglMakeCurrent(m_hDC, NULL);
        wglDeleteContext(m_hGLRC);
    }

    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
    }
}

void GLWindow::open()
{
    ShowWindow(m_hWnd, SW_SHOW);
	SetForegroundWindow(m_hWnd);
	SetFocus(m_hWnd);

	UpdateWindow(m_hWnd);
}





void GLWindow::makeCurrent()
{
    wglMakeCurrent(m_hDC, m_hGLRC);
}


void GLWindow::swapBuffers()
{
    SwapBuffers(m_hDC);
}


bool GLWindow::createContext()
{
    if (!m_hDC)
        return false;

    m_hGLRC = wglCreateContext(m_hDC);

    if (!m_hGLRC)
        return false;

    wglMakeCurrent( m_hDC, m_hGLRC );

    if (glewInit() != GLEW_OK)
    {
        return false;
    }
   
    if (WGLEW_ARB_create_context)
    {
        wglDeleteContext(m_hGLRC);

        int attribs[] = {
          WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
          WGL_CONTEXT_MINOR_VERSION_ARB, 1,
          WGL_CONTEXT_PROFILE_MASK_ARB , WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#ifdef DEBUG
          WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
          0
        }; 

        m_hGLRC = wglCreateContextAttribsARB(m_hDC, 0, attribs);

        if (m_hGLRC)
        {
            wglMakeCurrent(m_hDC, m_hGLRC);

            if (GLEW_AMD_debug_output)
                glDebugMessageCallbackAMD((GLDEBUGPROCAMD)&MyDebugFunc, NULL);

            return true;            
        }
    }

    return false;
}


void GLWindow::deleteContext()
{
    wglMakeCurrent(m_hDC, NULL);
    wglDeleteContext(m_hGLRC);
}

#endif


//----------------------------------------------------------------------------------------------
//  Linux implementation functions
//----------------------------------------------------------------------------------------------
#ifdef LINUX

bool GLWindow::create(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDspIndex, bool bDecoration)
{
    XSetWindowAttributes WinAttr;

    int attrList[] = {  GLX_RGBA, GLX_DOUBLEBUFFER,
                        GLX_RED_SIZE, 8,
                        GLX_GREEN_SIZE, 8,
                        GLX_BLUE_SIZE, 8,
                        GLX_DEPTH_SIZE, 24,
                        GLX_STENCIL_SIZE, 8,
                        None };

    // Get information for the display with id uiDspId. This is the ID
    // shown by CCC. Use the origin of this display as base position for
    // opening the window. Like this a window can be opened on a particular
    // GPU.
    DisplayData* pDsp = getDisplayData(uiDspIndex);

    if (!pDsp)
        return false;

    m_uiWidth  = uiWidth;
    m_uiHeight = uiHeight;


    m_hDC = XOpenDisplay(pDsp->strDisplayname.c_str());

    if (m_hDC == NULL)
    {
	printf("Could not open display display \n");
	exit(1);
    }

    m_uiScreen = DefaultScreen(m_hDC);

    /* get an appropriate visual */
    m_vi = glXChooseVisual(m_hDC, m_uiScreen, attrList);

    Window root = RootWindow(m_hDC, m_uiScreen);

    /* window attributes */
    WinAttr.border_pixel = 0;
    WinAttr.colormap = XCreateColormap( m_hDC, root, m_vi->visual, AllocNone);
    WinAttr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
    WinAttr.background_pixel = 0;
    unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    m_hWnd = XCreateWindow(m_hDC, root, 0, 0, m_uiWidth, m_uiHeight, 0, m_vi->depth, InputOutput, m_vi->visual, mask, &WinAttr);

    XSetStandardProperties(m_hDC, m_hWnd, m_strWinName.c_str(), m_strWinName.c_str(), None, NULL, 0, NULL);

	if (!bDecoration)
	{
       static const unsigned MWM_HINTS_DECORATIONS = (1 << 1);
       static const int PROP_MOTIF_WM_HINTS_ELEMENTS = 5;

       typedef struct
       {
	      unsigned long       flags;
	      unsigned long       functions;
	      unsigned long       decorations;
	      long                inputMode;
	      unsigned long       status;
       } PropMotifWmHints;

       PropMotifWmHints motif_hints;
       Atom prop, proptype;
       unsigned long flags = 0;

       /* setup the property */
       motif_hints.flags = MWM_HINTS_DECORATIONS;
       motif_hints.decorations = flags;

       /* get the atom for the property */
       prop = XInternAtom( m_hDC, "_MOTIF_WM_HINTS", True );
       if (!prop) 
	   {
	      /* something went wrong! */
	      return false;
       }
       proptype = prop;

       XChangeProperty( m_hDC, m_hWnd,           /* display, window */
		         prop, proptype,                 /* property, type */
		         32,                             /* format: 32-bit datums */
		         PropModeReplace,                /* mode */
		         (unsigned char *) &motif_hints, /* data */
		         PROP_MOTIF_WM_HINTS_ELEMENTS    /* nelements */
		       );
      
	}
	 
    return true;
}


void GLWindow::destroy()
{
    if (m_hGLRC)
    {
        glXMakeCurrent(m_hDC, NULL, NULL);
        glXDestroyContext(m_hDC, m_hGLRC);
    }
    
    if (m_hWnd)
    {
        XUnmapWindow(m_hDC, m_hWnd);
        XDestroyWindow(m_hDC, m_hWnd);
    }
}


void GLWindow::open()
{
    XMapWindow(m_hDC, m_hWnd);
}


void GLWindow::makeCurrent()
{
    /* connect the glx-context to the window */
    glXMakeCurrent(m_hDC, m_hWnd, m_hGLRC);
}


void GLWindow::swapBuffers()
{
    //SwapBuffers(m_hDC);
    glXSwapBuffers(m_hDC, m_hWnd);
}


bool GLWindow::createContext()
{
	  /* create a GLX context */
    m_hGLRC = glXCreateContext(m_hDC, m_vi, 0, GL_TRUE);
    
    glXMakeCurrent(m_hDC, m_hWnd, m_hGLRC);

    if (glewInit() != GLEW_OK)
    {
        return false;
    }

    return true;
}


void GLWindow::deleteContext()
{
    glXMakeCurrent(m_hDC, NULL, NULL);
    glXDestroyContext(m_hDC, m_hGLRC);
}


#endif


// Load ADL library and get function pointers
bool GLWindow::setupADL()
{
    // check if ADL was already loaded
    if (g_AdlCalls.hDLL)
    {
        return true;
    }

#ifdef WIN32
    g_AdlCalls.hDLL = (void*)LoadLibrary("atiadlxx.dll");

	if (g_AdlCalls.hDLL == NULL)
       g_AdlCalls.hDLL = (void*)LoadLibrary("atiadlxy.dll");
#endif

#ifdef LINUX
    g_AdlCalls.hDLL = dlopen("libatiadlxx.so", RTLD_LAZY|RTLD_GLOBAL);
    
    if (g_AdlCalls.hDLL == NULL)
        g_AdlCalls.hDLL = dlopen("libatiadlxy.so", RTLD_LAZY|RTLD_GLOBAL);
#endif

	if (!g_AdlCalls.hDLL)
		return false;

	// Get proc address of needed ADL functions
	g_AdlCalls.ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GETPROC((HMODULE)g_AdlCalls.hDLL,"ADL_Main_Control_Create");
	if (!g_AdlCalls.ADL_Main_Control_Create)
		return false;

	g_AdlCalls.ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GETPROC((HMODULE)g_AdlCalls.hDLL, "ADL_Main_Control_Destroy");
	if (!g_AdlCalls.ADL_Main_Control_Destroy)
		return false;

	g_AdlCalls.ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GETPROC((HMODULE)g_AdlCalls.hDLL,"ADL_Adapter_NumberOfAdapters_Get");
	if (!g_AdlCalls.ADL_Adapter_NumberOfAdapters_Get)
		return false;

	g_AdlCalls.ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GETPROC((HMODULE)g_AdlCalls.hDLL,"ADL_Adapter_AdapterInfo_Get");
	if (!g_AdlCalls.ADL_Adapter_AdapterInfo_Get)
		return false;

	g_AdlCalls.ADL_Display_DisplayInfo_Get = (ADL_DISPLAY_DISPLAYINFO_GET)GETPROC((HMODULE)g_AdlCalls.hDLL,"ADL_Display_DisplayInfo_Get");
	if (!g_AdlCalls.ADL_Display_DisplayInfo_Get)
		return false;

	g_AdlCalls.ADL_Adapter_Active_Get = (ADL_ADAPTER_ACTIVE_GET)GETPROC((HMODULE)g_AdlCalls.hDLL,"ADL_Adapter_Active_Get");
	if (!g_AdlCalls.ADL_Adapter_Active_Get)
		return false;

	g_AdlCalls.ADL_Display_Position_Get = (ADL_DISPLAY_POSITION_GET)GETPROC((HMODULE)g_AdlCalls.hDLL,"ADL_Display_Position_Get");
	if (!g_AdlCalls.ADL_Display_Position_Get)
		return false;

    g_AdlCalls.ADL_Display_Size_Get = (ADL_DISPLAY_POSITION_GET)GETPROC((HMODULE)g_AdlCalls.hDLL,"ADL_Display_Size_Get");
	if (!g_AdlCalls.ADL_Display_Size_Get)
		return false;
 
    g_AdlCalls.ADL_Display_Modes_Get = (ADL_DISPLAY_MODES_GET)GETPROC((HMODULE)g_AdlCalls.hDLL, "ADL_Display_Modes_Get");
    if (!g_AdlCalls.ADL_Display_Modes_Get)
        return false;


	g_AdlCalls.ADL_Display_Property_Get = (ADL_DISPLAY_PROPERTY_GET)GETPROC((HMODULE)g_AdlCalls.hDLL,"ADL_Display_Property_Get");
	if (!g_AdlCalls.ADL_Display_Property_Get)
		return false;
 
	// Init ADL
	if (g_AdlCalls.ADL_Main_Control_Create(ADL_Alloc, 0) != ADL_OK)
		return false;

	return true;
}



GLWindow::DisplayData* GLWindow::getDisplayData(unsigned int uiDspId)
{
    vector<DisplayData*>::iterator itr;

    for (itr = m_DisplayInfo.begin(); itr != m_DisplayInfo.end(); ++itr)
    {
        if ((*itr)->uiDisplayId == uiDspId)
        {
            return (*itr);
        }
    }

    return NULL;
}

