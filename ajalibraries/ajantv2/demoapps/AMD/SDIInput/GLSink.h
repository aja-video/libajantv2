
#pragma once

#include "defines.h"

class SyncedBuffer;
class GLTransferBuffers;

class GLSink
{
public:

    GLSink();
    ~GLSink();

    void            initGL();
    void            resize(unsigned int w, unsigned int h);
    bool            createDownStream(unsigned int uiNumBuffers, unsigned int w, unsigned int h, int nIntFormat, int nExtFormat, int nType, bool bUseP2P = false);

    void            draw();

    void            release();

    SyncedBuffer*   getInputBuffer() { return m_pSyncBuffer; };

private:

    unsigned int            m_uiWindowWidth;
    unsigned int            m_uiWindowHeight;

    unsigned int            m_uiTextureWidth;
    unsigned int            m_uiTextureHeight;
    unsigned int            m_uiTexture;

    bool                    m_bUseP2P;

    float                   m_fAspectRatio;

    int                     m_nIntFormat;
    int                     m_nExtFormat;
    int                     m_nType;

    unsigned int            m_uiQuad;

    GLTransferBuffers*      m_pInputBuffer;
    SyncedBuffer*           m_pSyncBuffer;
    TransferFrame*          m_pFrameInfo;
};
