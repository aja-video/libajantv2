

#include "os_include.h"
#include <iostream>

#include "AJA_SDIInOut.h"
#include "GLSink.h"


PFNGLWAITMARKERAMDPROC          glWaitMarkerAMD;
PFNGLWRITEMARKERAMDPROC         glWriteMarkerAMD;
PFNGLMAKEBUFFERSRESIDENTAMDPROC glMakeBuffersResidentAMD;
PFNGLBUFFERBUSADDRESSAMDPROC    glBufferBusAddressAMD;

#define NUM_BUFFERS 2


// Define initial window size
int g_nWidth  = 800;
int g_nHeight = 600;

GLSink*         g_pSink     = NULL;
AJA_SDIInOut*   g_pSDIInOut = NULL;


void Resize(unsigned int uiWidth, unsigned int uiHeight)
{
    g_nWidth  = uiWidth;
    g_nHeight = uiHeight;

    if (g_pSink)
    {
        g_pSink->resize(uiWidth, uiHeight);
    }
}


int main(int argc, char** argv)
{
    // Indicet if p2p is to be used
    bool bUseP2P = true;

    // Indicate if AutoCirculate is to be used
    bool bUseAutoCirculate = false;

    // Create interface to AJA SDI board 0
    g_pSDIInOut = new AJA_SDIInOut(0);

    if (!g_pSDIInOut->openCard(NUM_BUFFERS, bUseAutoCirculate, bUseP2P))
    {
        std::cerr<<"Failed to open Kona3G!"<<std::endl;
        return 0;
    }

    // Create first sdi input channel. This one will have the id 0
    // 0:                   Use SDI In 1
    // NTV2_FBF_24BIT_RGB:  RGB8 FB
    if (!g_pSDIInOut->setupInputChannel(0, NTV2_FBF_24BIT_RGB))
    {
         std::cerr<<"No Input signal on SDI Input 1!"<<std::endl;
         return 0;
    }

    // Get information on FB configuration on the SDI board. Based on teh FB configuration
    // of the SDI board, the corresponding GL formats are returned.
    int          nIntFormat = g_pSDIInOut->getIntFormat(0);
    int          nExtFormat = g_pSDIInOut->getExtFormat(0);
    int          nType      = g_pSDIInOut->getType(0);
    unsigned int uiFBWidth  = g_pSDIInOut->getFramebufferWidth(0);
    unsigned int uiFBHeight = g_pSDIInOut->getFramebufferHeight(0);

    // Set window size according to SDI input signal
    g_nWidth  = uiFBWidth;
    g_nHeight = uiFBHeight;

    Display *pDsp = XOpenDisplay(":0.0");

    int attrib[] = { GLX_RGBA,
                     GLX_RED_SIZE, 8,
                     GLX_GREEN_SIZE, 8,
                     GLX_BLUE_SIZE, 8,
                     GLX_DOUBLEBUFFER,
                     GLX_DEPTH_SIZE, 24,
                     None };

    XSetWindowAttributes attr;
    unsigned long        mask;
    Window               root;
    XVisualInfo         *visinfo;

    int nScreen = DefaultScreen( pDsp );
    root = RootWindow( pDsp, nScreen );

    visinfo = glXChooseVisual( pDsp, nScreen, attrib );

    if (!visinfo) {
        printf("Error: couldn't get an RGBA, Double-buffered visual\n");
        return false;
    }

    /* window attributes */
    attr.border_pixel     = 0;
    attr.colormap         = XCreateColormap( pDsp, root, visinfo->visual, AllocNone);
    attr.event_mask       = StructureNotifyMask | ExposureMask | KeyPressMask;
    attr.background_pixel = 0;
    mask                  = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    Window Win = XCreateWindow( pDsp, root, 0, 0, g_nWidth, g_nHeight, 0, visinfo->depth, InputOutput, visinfo->visual, mask, &attr );

    /* set hints and properties */
    XSizeHints  sizehints;

    sizehints.x      = 0;
    sizehints.y      = 0;
    sizehints.width  = g_nWidth;
    sizehints.height = g_nHeight;
    sizehints.flags  = USSize | USPosition;

    XSetNormalHints(pDsp, Win, &sizehints);
    XSetStandardProperties(pDsp, Win, "SDIPlayback", "SDIPlayback", None, (char **)NULL, 0, &sizehints);

    GLXContext context = glXCreateContext( pDsp, visinfo, NULL, GL_TRUE );

    if (!context) {
        printf("Error: glXCreateContext failed\n");
        return false;
    }

    XFree(visinfo);
    XMapWindow(pDsp, Win);

    glXMakeCurrent(pDsp, Win, context);

    if (glewInit() != GLEW_OK)
       return 0;

    // check if AMD_pinned_memory and AMD_BUS_ADDRESSABLE_MEMORY are supported
    int             nNumExtensions        = 0;
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
            glMakeBuffersResidentAMD = (PFNGLMAKEBUFFERSRESIDENTAMDPROC) glXGetProcAddress((GLubyte*)"glMakeBuffersResidentAMD");
            glBufferBusAddressAMD    = (PFNGLBUFFERBUSADDRESSAMDPROC)    glXGetProcAddress((GLubyte*)"glBufferBusAddressAMD");
            glWaitMarkerAMD          = (PFNGLWAITMARKERAMDPROC)          glXGetProcAddress((GLubyte*)"glWaitMarkerAMD");
            glWriteMarkerAMD         = (PFNGLWRITEMARKERAMDPROC)         glXGetProcAddress((GLubyte*)"glWriteMarkerAMD");
        }
    }

    bool bDone = false;

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
       while (XPending(pDsp) > 0)
       {

           XEvent event;
           XNextEvent(pDsp, &event);

           switch (event.type)
           {
              case Expose:
                break;
              case ConfigureNotify:
                Resize(event.xconfigure.width, event.xconfigure.height);
                break;
               case KeyPress:
               {
                    char buffer[10];
                    XLookupString(&event.xkey, buffer, sizeof(buffer), NULL, NULL);
                    if (buffer[0] == 27)
                    {
                        bDone = true;
                    }
               }
            }
        }

        g_pSink->draw();
        glXSwapBuffers(pDsp, Win);
    }

    if (g_pSink)
        g_pSink->release();

    g_pSDIInOut->stop();
    g_pSDIInOut->closeCard();

    delete g_pSink;
    delete g_pSDIInOut;

    return 0;
}
