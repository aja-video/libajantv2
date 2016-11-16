

#pragma once

class SyncedBuffer;
class GLTransferBuffers;
class RenderTarget;

class GLSource
{
public:

    GLSource();
    ~GLSource();

    void            initGL();
    void            resize(unsigned int w, unsigned int h);
    bool            createUpStream(unsigned int uiNumBuffers, unsigned int w, unsigned int h, int nIntFormat, int nExtFormat, int nType, bool bUseP2P = false);
    bool            setRemoteSDIMemory(unsigned int uiNumBuffersAvailable, unsigned long long* pBufferBusAddress, unsigned long long* pMarkerBusAddress);
    void            draw();

    SyncedBuffer*   getOutputBuffer() { return (SyncedBuffer*) m_pSyncBuffer; };

private:

    unsigned int    m_uiWindowWidth;
    unsigned int    m_uiWindowHeight;

    unsigned int    m_uiBufferWidth;
    unsigned int    m_uiBufferHeight;

    unsigned int    m_uiRTIndex;

    int             m_nIntFormat;
    int             m_nExtFormat;
    int             m_nType;

    bool            m_bUseP2P;

    unsigned int    m_uiCube;

    float           m_fRotationAngle;
    float           m_fFieldOfView;

    RenderTarget*   m_pRenderTarget[2];

    GLTransferBuffers*   m_pOutputBuffer;
    SyncedBuffer*        m_pSyncBuffer;
    TransferFrame*       m_pFrameInfo;
};
