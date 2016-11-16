
#include "os_include.h"
#include <GL/glew.h>

#include "FormatInfo.h"
#include "SyncedBuffer.h"
#include "RenderTarget.h"
#include "GLTransferBuffers.h"
#include "GLSource.h"



GLSource::GLSource()
{
    m_uiWindowWidth  = 0;
    m_uiWindowHeight = 0;

    m_uiBufferWidth  = 0;
    m_uiBufferHeight = 0;

    m_uiCube = 0;

    m_uiRTIndex = 0;

    m_bUseP2P = false;

    m_fRotationAngle = 0.0f;

    m_nIntFormat = GL_RGB8;
    m_nExtFormat = GL_RGB;
    m_nType      = GL_UNSIGNED_BYTE;

    m_fFieldOfView = 60.0f;

    m_pRenderTarget[0]  = NULL;
    m_pRenderTarget[1]  = NULL;
    m_pOutputBuffer     = NULL;
    m_pSyncBuffer       = NULL;
    m_pFrameInfo        = NULL;
}


GLSource::~GLSource()
{
    if (m_pRenderTarget[0])
        delete m_pRenderTarget[0];

    if (m_pRenderTarget[1])
        delete m_pRenderTarget[1];

    if (m_pOutputBuffer)
        delete m_pOutputBuffer;

    if (m_pSyncBuffer)
        delete m_pSyncBuffer;

    if (m_pFrameInfo)
        delete [] m_pFrameInfo;
}



void GLSource::initGL()
{
    const float pVertices[] = { -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
                                 0.5f, -0.5f,  0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,
                                -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
                                -0.5f, -0.5f,  0.5f,  -0.5f, -0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,  -0.5f,  0.5f,  0.5f,
                                -0.5f,  0.5f,  0.5f,   0.5f,  0.5f,  0.5f,   0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
                                -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f, -0.5f, -0.5f,  -0.5f, -0.5f, -0.5f };

    const float pColors[]   = { 1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.0f,
                                0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f,
                                1.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f,
                                1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.0f,
                                0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f,
                                1.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f };


    glClearColor(0.0f, 0.2f, 0.8f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glShadeModel(GL_SMOOTH);

    glPolygonMode(GL_FRONT, GL_FILL);
    
    // create cube
    glGenBuffers(1, &m_uiCube);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiCube);

    glBufferData(GL_ARRAY_BUFFER, 144 * sizeof(float), pVertices, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 72 * sizeof(float), 72 * sizeof(float), pColors);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, 0);
    glColorPointer( 3, GL_FLOAT, 0, ((char*)NULL + 72 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}



// Resize only the window. Since we are rendering into a FBO the
// projection matrix does not change on a window resize
void GLSource::resize(unsigned int w, unsigned int h)
{
    m_uiWindowWidth  = w;
    m_uiWindowHeight = h;
}


// Create a FBO that will be used as render target and a synchronized PACK buffer to transfer
// FBO content to the SDI thread
bool GLSource::createUpStream(unsigned int uiNumBuffers, unsigned int w, unsigned int h, int nIntFormat, int nExtFormat, int nType, bool bUseP2P)
{
    m_uiBufferWidth  = w;
    m_uiBufferHeight = h;

    m_nIntFormat = nIntFormat;
    m_nExtFormat = nExtFormat;
    m_nType      = nType;

    // check if format is supported
    if ((FormatInfo::getInternalFormatSize(m_nIntFormat) * FormatInfo::getExternalFormatSize(m_nExtFormat, m_nType)) == 0)
        return false;

    // Create FBO that is used as render target
    m_pRenderTarget[0] = new RenderTarget;
    m_pRenderTarget[0]->createBuffer(m_uiBufferWidth, m_uiBufferHeight, m_nIntFormat, m_nExtFormat, m_nType);

    m_pRenderTarget[1] = new RenderTarget;
    m_pRenderTarget[1]->createBuffer(m_uiBufferWidth, m_uiBufferHeight, m_nIntFormat, m_nExtFormat, m_nType);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(m_fFieldOfView, (float)m_uiBufferWidth/(float)m_uiBufferHeight, 0.1f, 1000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    m_bUseP2P = bUseP2P;

    // create snychronize buffer for data exchange with the SDI thread
    m_pOutputBuffer = new GLTransferBuffers;

    // Create uiNumBuffers synchronize PACK_BUFFER
    if (!m_pOutputBuffer->createBuffers(uiNumBuffers, m_uiBufferWidth * m_uiBufferHeight * FormatInfo::getInternalFormatSize(m_nIntFormat), GL_PIXEL_PACK_BUFFER, m_bUseP2P))
    {
        return false;
    }

    // Creat a synchronization buffer that manages the frame data to sync with SDI Thread
    m_pSyncBuffer = new SyncedBuffer;
    m_pSyncBuffer->createSyncedBuffer(uiNumBuffers);

    // Create Frame information. The frame information is used to exchange data between the SDI and the 
    // render thread. Access to the TransferFrame elements is synchronized by the m_pSyncBuffer. The SDI 
    // thread will fill such an element with information into which buffer on the sdi device the render 
    // thread can write its frame and which marker value is expected to indicate the end of the transfer.
    m_pFrameInfo = new TransferFrame[uiNumBuffers];

    ZeroMemory(m_pFrameInfo, sizeof(TransferFrame)*uiNumBuffers);

    for (unsigned int i = 0; i < uiNumBuffers; ++i)
    {
        if (!m_bUseP2P)
        {
            m_pFrameInfo[i].pData = m_pOutputBuffer->getPinnedMemoryPtr(i);
        }

        // Add frame data to the sync buffer to control access
        m_pSyncBuffer->setBufferMemory(i, (char*)&(m_pFrameInfo[i]));
    }

    glPixelStorei(GL_PACK_ALIGNMENT, FormatInfo::getAlignment(m_uiBufferWidth, m_nExtFormat, m_nType));
    
    return true;
}


bool GLSource::setRemoteSDIMemory(unsigned int uiNumBuffersAvailable, unsigned long long* pBufferBusAddress, unsigned long long* pMarkerBusAddress)
{
    if (!m_pOutputBuffer)
        return false;

    if (!m_pOutputBuffer->assignRemoteMemory(uiNumBuffersAvailable, pBufferBusAddress, pMarkerBusAddress))
        return false;

    return true;    
}



void GLSource::draw()
{
    GLsync         PackFence;
    unsigned int   uiBufferIdx;
    TransferFrame* pSDIFrameInfo = NULL;

    // Draw a rotating cube to FBO
    m_pRenderTarget[m_uiRTIndex]->bind();

    // Set buffer viewport
    glViewport(0, 0, m_uiBufferWidth, m_uiBufferHeight);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    glTranslatef(0.0f, 0.0f, -2.0f);
    glRotatef(m_fRotationAngle, 1.0f, 0.0f, 1.0f);

    glBindBuffer(GL_ARRAY_BUFFER, m_uiCube);

    glDrawArrays(GL_QUADS, 0, 24);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPopMatrix();

    if (m_bUseP2P)
    {
        // getBufferForReading will block until the SDI thread has produced a frame
        // that contains the bus address of the buffer into which the render thread can write.
        uiBufferIdx = m_pSyncBuffer->getBufferForReading((void*&)pSDIFrameInfo);

        // Bind buffer that corresponds to the bus address indicated in pSDIFrameInfo
        m_pOutputBuffer->bindBuffer(pSDIFrameInfo->uiSdiBufferBusAddress);

        // transfer data. This call is non-blocking
        glReadPixels(0, 0, m_uiBufferWidth, m_uiBufferHeight, m_nExtFormat, m_nType, NULL);

        // write marker to notify SDI driver that the transfer has completed.
        m_pOutputBuffer->writeMarker(pSDIFrameInfo->uiSdiBufferBusAddress, pSDIFrameInfo->uiSdiMarkerBusAddress, pSDIFrameInfo->uiWriteMarkerValue);

        // Mark the buffer as ready to be consumed
        m_pSyncBuffer->releaseReadBuffer();

    }
    else
    {
        // getBufferForWriting will block until an empty element in m_pSyncBuffer is available.
        // The render thread will copy the frame into pinned mem and release the buffer to let the
        // SDI thread know that a frame is ready which can be downloaded to the sdi device.
        uiBufferIdx = m_pSyncBuffer->getBufferForWriting((void*&)pSDIFrameInfo);

        m_pOutputBuffer->bindBuffer(uiBufferIdx);

        // transfer data. This call is non-blocking
        glReadPixels(0, 0, m_uiBufferWidth, m_uiBufferHeight, m_nExtFormat, m_nType, NULL);

        PackFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    }

    m_pRenderTarget[m_uiRTIndex]->unbind();

    // Set window viewport
    glViewport(0, 0, m_uiWindowWidth, m_uiWindowHeight);

    // Copy FBO to window
    m_pRenderTarget[m_uiRTIndex]->draw();

    // Wait until ogl operations finnished before releasing the buffers
    if (!m_bUseP2P && glIsSync(PackFence))
    {
        // if Pack buffer is no longer accessed, release buffer and allow other threads
        // to consume it 
        glClientWaitSync(PackFence, GL_SYNC_FLUSH_COMMANDS_BIT, OneSecond);
        glDeleteSync(PackFence);

        // Mark the buffer as ready to be consumed
        m_pSyncBuffer->releaseWriteBuffer();
    }

    m_fRotationAngle += 0.2f;

    m_uiRTIndex = 1 - m_uiRTIndex;
}
    

