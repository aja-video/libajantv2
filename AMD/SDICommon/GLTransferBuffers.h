#pragma once

#ifdef WIN32
#include "GL/glew.h"
#endif

#ifdef LINUX
#include <GL/glxew.h>
#endif

#include "defines.h"


typedef struct
{
    char*           pBasePointer;
    char*           pAlignedPointer;
} AlignedMem;



class GLTransferBuffers
{
public:

    GLTransferBuffers(void);
    ~GLTransferBuffers(void);

    // Creates eithet bus addressabe buffers or pinned memory buffers that can be used for data transfers.
    // uiNumBuffers: Number of buffers to be created
    // uiBufferSize: size of each buffer in bytes
    // uiTarget: Either GL_PIXEL_PACK_BUFFER or GL_PIXEL_UNPACK_BUFFER
    // bUseP2P: if true a bus addressabe buffer is created, if false a pinned memory buffer
    // pPinnedMem: Pointer to memory that is already pinned and that should be reused for this buffer. If NULL new memory is allocated
    bool createBuffers(unsigned int uiNumBuffers, unsigned int uiBufferSize, unsigned int uiTarget, bool bUseP2P = false, AlignedMem *pPinnedMem = NULL);

    // In case of using a p2p PACK_BUFFER, the actual memory used by the buffer is located on a remote device. 
    // assignRemoteMemory passes the bus addresses of this memory to OpenGL and assigns it to the buffers used by this class.
    bool assignRemoteMemory(unsigned int uiNumBuffers, unsigned long long* pBufferBusAddress, unsigned long long* pMarkerBusAddress);

    void   waitMarker(unsigned int uiMarkerValue);
    void   writeMarker(unsigned long long ulBufferBusAddress, unsigned long long ulMarkerBusAddress, unsigned int uiMarkerValue);

    // Bind the buffer that is located at the bus address: ulBusAddress
    bool    bindBuffer(unsigned long long ulBusAddress);
    // Bind buffer with id: uiIdx
    bool    bindBuffer(unsigned int uiIdx);

    // In case of pinned memory returns the pointer to the aligned memory
    AlignedMem*         getPinndeMemory()              { return m_pBufferMemory; };
    // In case of pinned memory buffers, returns the ponter to the pinned system memory
    // that is used by buffer id: uiIdx
    char*               getPinnedMemoryPtr(unsigned int uiIdx);
    // In case of bus addressable memory, returns the bus address of the buffer memory used by
    // the buffer with id:  uiIdx
    GLuint64            getBufferBusAddress(unsigned int uiIdx);
    // In case of bus addressable memory, returns a pointer to the array that contains the buffer
    // bus addresses. The array length is m_uiNumBuffers
    GLuint64*           getBufferBusAddresses()        { return m_pBufferBusAddress; };
    // In case of bus addressable memory, returns the bus address of the marker memory used by
    // the buffer with id:  uiIdx
    GLuint64            getMarkerBusAddress(unsigned int uiIdx);
    // In case of bus addressable memory, returns a pointer to the array that contains the marker
    // bus addresses. The array length is m_uiNumBuffers
    GLuint64*           getMarkerBusAddresses()        { return m_pMarkerBusAddress; };

    unsigned int         getNumBuffers()                { return m_uiNumBuffers; };

private:

    bool                    m_bUseP2P;
    bool                    m_bBufferReady;

    unsigned int            m_uiTarget;
    unsigned int            m_uiNumBuffers;
    unsigned int            m_uiBufferSize;
    unsigned int            m_uiBufferIdx;
    unsigned int*           m_pBuffer;

    bool                    m_bAllocatedPinnedMem;
    AlignedMem*             m_pBufferMemory;
    GLuint64*               m_pBufferBusAddress;
    GLuint64*               m_pMarkerBusAddress;
};
