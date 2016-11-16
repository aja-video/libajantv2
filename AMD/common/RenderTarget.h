#pragma once


class RenderTarget
{
public:
    RenderTarget();
    ~RenderTarget();

    // Create FBO with specified dimension and format
    bool    createBuffer(unsigned int nWidth, unsigned int nHeight, int nBufferFormat, int nExtFormat, int nType);
    // Delete FBO and storage
    void    deleteBuffer();

    // Bind FBO
    void    bind(GLenum nTarget = GL_FRAMEBUFFER);
    // Release FBO
    void    unbind();

    // Draws color attachment as texture into a screen aligned quad
    void    draw();

    int             getBufferFormat();
    unsigned int    getBufferWidth(); 
    unsigned int    getBufferHeight();

private:

    GLuint          m_uiBufferId;
    GLuint          m_uiColorTex;
    GLuint          m_uiDepthBuffer;
    unsigned int    m_uiBufferWidth;
    unsigned int    m_uiBufferHeight;
    unsigned int    m_uiQuad;

    int             m_nBufferFormat;
    int             m_nExtFormat;
    int             m_nType;
};
