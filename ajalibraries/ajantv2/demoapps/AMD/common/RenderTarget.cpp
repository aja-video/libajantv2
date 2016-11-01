#include <GL/glew.h>

#include "RenderTarget.h"

RenderTarget::RenderTarget(void)
{
    m_uiBufferId        = 0;
    m_uiBufferWidth     = 0;
    m_uiBufferHeight    = 0;
    m_nBufferFormat     = 0;
}


RenderTarget::~RenderTarget(void)
{
    deleteBuffer();
}


bool RenderTarget::createBuffer(unsigned int uiWidth, unsigned int uiHeight, int nBufferFormat, int nExtFormat, int nType)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (m_uiBufferId == 0)
    {
        m_uiBufferWidth  = uiWidth;
        m_uiBufferHeight = uiHeight;
        m_nBufferFormat  = nBufferFormat;
        m_nExtFormat     = nExtFormat;
        m_nType          = nType;

        // Setup texture to be used as color attachment
        glGenTextures(1, &m_uiColorTex);

        glBindTexture(GL_TEXTURE_2D, m_uiColorTex);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    	
        glTexImage2D(GL_TEXTURE_2D, 0, m_nBufferFormat, m_uiBufferWidth, m_uiBufferHeight, 0, m_nExtFormat, m_nType, 0);

        // Create FBO with color and depth attachment
        glGenFramebuffers(1,  &m_uiBufferId);
        glGenRenderbuffers(1, &m_uiDepthBuffer);

        glBindFramebuffer(GL_FRAMEBUFFER, m_uiBufferId);

        glBindRenderbuffer(GL_RENDERBUFFER, m_uiDepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_uiBufferWidth, m_uiBufferHeight);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_uiColorTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, m_uiDepthBuffer);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        {  
            return true;
        }
    }
    
    return false;
}


void RenderTarget::deleteBuffer()
{
    if (m_uiColorTex)
        glDeleteTextures(1, &m_uiColorTex);

    if (m_uiDepthBuffer)
        glDeleteRenderbuffers(1, &m_uiDepthBuffer);

    if (m_uiBufferId)
        glDeleteFramebuffers(1, &m_uiBufferId);

    m_uiBufferId = 0;
}


void RenderTarget::bind(GLenum nTarget)
{
    if (m_uiBufferId)
    {
        glBindFramebuffer(nTarget, m_uiBufferId);
    }
}


void RenderTarget::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void RenderTarget::draw()
{
    int nViewport[4];

    glGetIntegerv(GL_VIEWPORT, nViewport);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_uiBufferId);
    glBlitFramebuffer(0, 0, m_uiBufferWidth, m_uiBufferHeight, nViewport[0], nViewport[1], nViewport[2], nViewport[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}


unsigned int RenderTarget::getBufferHeight()
{
    if (m_uiBufferId)
    {
        return m_uiBufferHeight;
    }

    return 0;
}


unsigned int RenderTarget::getBufferWidth()
{
    if (m_uiBufferId)
    {
        return m_uiBufferWidth;
    }

    return 0;
}


int RenderTarget::getBufferFormat()
{
    if (m_uiBufferId)
    {
        return m_nBufferFormat;
    }

    return 0;
}
