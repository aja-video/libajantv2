
#include "os_include.h"
#include <GL/glew.h>

#if defined (WIN32)
#include <GL/wglew.h>
#endif

#include "FormatInfo.h"
#include "SyncedBuffer.h"
#include "GLTransferBuffers.h"
#include "GLSink.h"

#define OneSecond 1000*1000*1000L


GLSink::GLSink()
{
    m_uiWindowWidth  = 0;
    m_uiWindowHeight = 0;

    m_uiTextureWidth  = 0;
    m_uiTextureHeight = 0;
    m_uiTexture       = 0;

    m_nIntFormat = GL_RGB8;
    m_nExtFormat = GL_RGB;
    m_nType      = GL_UNSIGNED_BYTE;

    m_uiQuad = 0;

    m_bUseP2P = false;

    m_fAspectRatio = 1.0f;

    m_pInputBuffer = NULL;
    m_pSyncBuffer  = NULL;
    m_pFrameInfo   = NULL;
}


GLSink::~GLSink()
{
    // make sure now other thread is blocked
    release();

    if (m_pInputBuffer)
        delete m_pInputBuffer;

    if (m_pSyncBuffer)
        delete m_pSyncBuffer;

    if (m_pFrameInfo)
        delete [] m_pFrameInfo;
}


void GLSink::initGL()
{
    const float pArray [] = { -0.5f, -0.5f, 0.0f,       0.0f, 1.0f,
                               0.5f, -0.5f, 0.0f,       1.0f, 1.0f,
                               0.5f,  0.5f, 0.0f,       1.0f, 0.0f,
                              -0.5f,  0.5f, 0.0f,       0.0f, 0.0f 
                            };

    glClearColor(0.0f, 0.2f, 0.8f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glShadeModel(GL_SMOOTH);

    glPolygonMode(GL_FRONT, GL_FILL);
    
    // create quad that is used to map SDi texture to
    glGenBuffers(1, &m_uiQuad);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiQuad);

    glBufferData(GL_ARRAY_BUFFER, 20*sizeof(float), pArray, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_uiQuad);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT,   5*sizeof(float), 0);
    glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), (char*)NULL + 3*sizeof(float));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void GLSink::resize(unsigned int w, unsigned int h)
{
    m_uiWindowWidth  = w;
    m_uiWindowHeight = h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

//    glOrtho(-(double)w/(double)h*0.5, (double)w/(double)h*0.5, 0.5, -0.5, -1.0, 1.0);  // frame buffer bottom up
    glOrtho(-(double)w/(double)h*0.5, (double)w/(double)h*0.5, -0.5, 0.5, -1.0, 1.0);  // frame buffer top down

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


bool GLSink::createDownStream(unsigned int uiNumBuffers, unsigned int w, unsigned int h, int nIntFormat, int nExtFormat, int nType, bool bUseP2P)
{
    m_uiTextureWidth  = w;
    m_uiTextureHeight = h;

    m_nIntFormat = nIntFormat;
    m_nExtFormat = nExtFormat;
    m_nType      = nType;

    m_fAspectRatio = (float)w/(float)h;

    // check if format is supported
    if ((FormatInfo::getInternalFormatSize(m_nIntFormat) * FormatInfo::getExternalFormatSize(m_nExtFormat, m_nType)) == 0)
        return false;

    m_bUseP2P = bUseP2P;

    // Create texture that will be used to store video that comes from SDI
    glGenTextures(1, &m_uiTexture);

    glBindTexture(GL_TEXTURE_2D, m_uiTexture);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexImage2D(GL_TEXTURE_2D, 0, m_nIntFormat, m_uiTextureWidth, m_uiTextureHeight, 0, m_nExtFormat, m_nType, NULL);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Create buffer that is used for data transfer buffers
    m_pInputBuffer = new GLTransferBuffers;

    if (!m_pInputBuffer->createBuffers(uiNumBuffers, m_uiTextureWidth*m_uiTextureHeight*FormatInfo::getExternalFormatSize(m_nExtFormat, m_nType), GL_PIXEL_UNPACK_BUFFER, m_bUseP2P))
    {
        return false;
    }

    // Creat a synchronization buffer that manages the frame data to sync with SDI Thread
    m_pSyncBuffer = new SyncedBuffer;
    m_pSyncBuffer->createSyncedBuffer(uiNumBuffers);

    // Create Frame information. The frame information is used to exchange data
    // between the SDI and the render thread. Access to the TransferFrame elements
    // is synchronized by the m_pSyncBuffer. The SDI thread will fill such an element
    // to indicate that a transfer to the gpu has started. The render thread will
    // consume those lements. As soon as the render thread can access one, it will start
    // rendering using the buffer that contains the frame.
    m_pFrameInfo = new TransferFrame[uiNumBuffers];

    ZeroMemory(m_pFrameInfo, sizeof(TransferFrame)*uiNumBuffers);

    for (unsigned int i = 0; i < uiNumBuffers; ++i)
    {
        if (m_bUseP2P)
        {
            m_pFrameInfo[i].uiGfxBufferBusAddress = m_pInputBuffer->getBufferBusAddress(i);
            m_pFrameInfo[i].uiGfxMarkerBusAddress = m_pInputBuffer->getMarkerBusAddress(i);
        }
        else
        {
            m_pFrameInfo[i].pData = m_pInputBuffer->getPinnedMemoryPtr(i);
        }

        // Add frame data to the sync buffer to control access
        m_pSyncBuffer->setBufferMemory(i, (char*)&(m_pFrameInfo[i]));
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, FormatInfo::getAlignment(m_uiTextureWidth, m_nExtFormat, m_nType));

    return true;
}


void GLSink::draw()
{
    unsigned int   uiBufferIdx;
    TransferFrame* pSDIFrameInfo = NULL;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update stream 
    glBindTexture(GL_TEXTURE_2D, m_uiTexture);

    // block until new frame is available
    if (m_bUseP2P)
    {
        // getBufferForReading will block until a frame that was produced by the 
        // SDI Thread is in the queue. The frame contains the information to which
        // buffer address the frame was written.
        uiBufferIdx = m_pSyncBuffer->getBufferForReading((void*&)pSDIFrameInfo);

        // Bind buffer that corresponds to the bus address define in pSDIFrameInfo
        if (m_pInputBuffer->bindBuffer(pSDIFrameInfo->uiGfxBufferBusAddress))
        {
            // Only wait for a marker if we got a valid bus address.
            m_pInputBuffer->waitMarker(pSDIFrameInfo->uiWaitMarkerValue);
        }
    }
    else
    {
        // getBufferForReading will block until a frame that was produced by the 
        // SDI Thread is in the queue. In case of pinned memory the SDI Thread
        // has written the frame into the pinned memory location associated with
        // buffer uiBufferIdx.
        uiBufferIdx = m_pSyncBuffer->getBufferForReading((void*&)pSDIFrameInfo);

        m_pInputBuffer->bindBuffer(uiBufferIdx);
    }
    
    // Copy buffer into texture
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_uiTextureWidth, m_uiTextureHeight, m_nExtFormat, m_nType, NULL);

    // Insert fence to determine when we the bound buffer can be reused.
    GLsync Fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    glPushMatrix();
   
    // Scale quad to the AR of the incoming texture
    glScalef(m_fAspectRatio, 1.0f, 1.0f);
   
    // Draw quad with mapped texture
    glBindBuffer(GL_ARRAY_BUFFER, m_uiQuad);
    glDrawArrays(GL_QUADS, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);

    if (glIsSync(Fence))
    {
        glClientWaitSync(Fence, GL_SYNC_FLUSH_COMMANDS_BIT, OneSecond);
        glDeleteSync(Fence);
    }

    // Release buffer and allow the SDI thread to use this buffer
    // as destination again.
    m_pSyncBuffer->releaseReadBuffer();
}


void GLSink::release()
{
    if (m_pSyncBuffer)
        m_pSyncBuffer->releaseReadBuffer();
}
