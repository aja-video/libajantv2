#pragma once

#include "os_include.h"

#include <string>
#include <vector>


#ifdef LINUX
#include "GL/glxew.h"
#endif


class GLWindow
{
public:
    GLWindow(const char* strWinName, const char *strClsaaName);
    ~GLWindow();

    // setups ADL and enumerates Displays
    bool            init();

    // create a window but does not open it
    bool            create(unsigned int uiWidth, unsigned int uiHeight, unsigned int uiDspIndex = 1, bool bDecoration = true);
    void            destroy();

    // creates an OpenGL context for this window
    bool            createContext();
    void            deleteContext();
    
    // Opens window
    void            open();

    // stores the new window size
    void            resize(unsigned int uiWidth, unsigned int uiHeight);

    void            makeCurrent();
    void            swapBuffers();

    DC              getDC()             { return m_hDC; };
    WINDOW          getWnd()            { return m_hWnd;};

    unsigned int    getWidth()          { return m_uiWidth; };
    unsigned int    getHeight()         { return m_uiHeight; };

    // returns the number of GPUs in the system
    unsigned int    getNumGPUs()        { return m_uiNumGPU; };
    // returns the number of displays mapped on GPU uiGPU
    unsigned int    getNumDisplaysOnGPU(unsigned int uiGPU);
    // returns the DisplayID of Display n on GPU uiGPU
    // n=0 will return the first display on GPU uiGPU if available
    // n=1 will return the second display on GPU uiGPU if available ...
    unsigned int    getDisplayOnGPU(unsigned int uiGPU, unsigned int n=0);

private:

    typedef struct
    {
        unsigned int    uiGPUId;
        unsigned int    uiDisplayId;
        unsigned int    uiDisplayLogicalId;
        int             nOriginX;
        int             nOriginY;
        unsigned int    uiWidth;
        unsigned int    uiHeight;
        std::string     strDisplayname;
    } DisplayData;


    bool            setupADL();
    DisplayData*    getDisplayData(unsigned int uiDspId);


    DC                          m_hDC;
    GLCTX                       m_hGLRC;
    WINDOW                      m_hWnd;

 #ifdef LINUX
    unsigned int                m_uiScreen;
    XVisualInfo*                m_vi;
#endif
    unsigned int                m_uiWidth;
    unsigned int                m_uiHeight;
    unsigned int                m_uiPosX;
    unsigned int                m_uiPosY;

    bool                        m_bADLReady;
    bool                        m_bFullScreen;

    unsigned int                m_uiNumGPU;
    std::vector<DisplayData*>   m_DisplayInfo;

    std::string                 m_strWinName;
    std::string                 m_strClassName; 
};
