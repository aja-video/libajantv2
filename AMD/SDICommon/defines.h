
#pragma once

#ifdef WIN32
#include <GL/glew.h>
#define APIENTRY    WINAPI
#endif

#ifdef LINUX
#include <GL/glxew.h>

typedef long long LONGLONG;

#define APIENTRY
#endif

#define GLX_ARB_get_proc_address 1

#define GL_EXTERNAL_VIRTUAL_MEMORY_AMD          0x9160

#define GL_BUS_ADDRESSABLE_MEMORY_AMD           0x9168
#define GL_EXTERNAL_PHYSICAL_MEMORY_AMD         0x9169

typedef GLvoid (APIENTRY * PFNGLWAITMARKERAMDPROC)          (GLuint buffer, GLuint marker);
typedef GLvoid (APIENTRY * PFNGLWRITEMARKERAMDPROC)         (GLuint buffer, GLuint marker, GLuint64 offset);
typedef GLvoid (APIENTRY * PFNGLMAKEBUFFERSRESIDENTAMDPROC) (GLsizei count, GLuint* buffers, GLuint64* baddrs, GLuint64* maddrs);
typedef GLvoid (APIENTRY * PFNGLBUFFERBUSADDRESSAMDPROC)    (GLenum target, GLsizeiptr size, GLuint64 baddrs, GLuint64 maddrs);


extern PFNGLWAITMARKERAMDPROC          glWaitMarkerAMD;
extern PFNGLWRITEMARKERAMDPROC         glWriteMarkerAMD;
extern PFNGLMAKEBUFFERSRESIDENTAMDPROC glMakeBuffersResidentAMD;
extern PFNGLBUFFERBUSADDRESSAMDPROC    glBufferBusAddressAMD;


#define OneSecond 1000*1000*1000L

#define MAX_SDI_BOARDS  2
#define MAX_STREAMS     4


// data structure to exchange P2P addresses between SDI ond OGL
typedef struct
{
    unsigned int        uiBufferId;
    // Data used if P2P is enabled
    unsigned long long  uiGfxBufferBusAddress;
    unsigned long long  uiGfxMarkerBusAddress;
    unsigned long long  uiSdiBufferBusAddress;
    unsigned long long  uiSdiMarkerBusAddress;
    unsigned int        uiWaitMarkerValue;
    unsigned int        uiWriteMarkerValue;
    // data used if pinned mem is enabled
    void*               pData;
} TransferFrame;

