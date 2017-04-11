#ifndef OGLPREVIEWWIDGET_H
#define OGLPREVIEWWIDGET_H

#include <QObject>
#include <QMutex>
#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#if defined __APPLE__ || defined(MACOSX)
#include <QOpenGLFunctions_4_1_Core>
#define OGL_SUBCLASS QOpenGLFunctions_4_1_Core
#else
#include <QOpenGLFunctions_3_1>
#define OGL_SUBCLASS QOpenGLFunctions_3_1
#endif

#include "ajabase/common/testpatterngen.h"

#if defined __APPLE__ || defined(MACOSX)
#pragma OPENCL EXTENSION CL_APPLE_gl_sharing : enable
#include <OpenCL/opencl.h>
#include "cl.hpp"
#include <OpenCL/cl_gl_ext.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLDevice.h>
#else
#include <CL/cl.hpp>
#endif

class COpenGLPreviewWidget : public QOpenGLWidget, OGL_SUBCLASS
{
    Q_OBJECT
public:
    explicit COpenGLPreviewWidget(QWidget *parent = 0);
    ~COpenGLPreviewWidget();

    void setAJAPixelFormat(AJA_PixelFormat pixelFormat,int width, int height );
    void setRenderReady(bool ready) { mInitialized = ready; }
    static bool isSupportedByHostGPU();
    virtual void   initialize3DLUT(QString fileName);
    virtual void   readLUT16(QString fileName);
    bool isReady() { return mInitialized; }
    cl::Device* getDefaultDevice() { return &default_device; }
    cl::Context* getContextCL() { return & contextCL; }
    cl::CommandQueue* getOpenCLQueue() { return &queue; }
    cl::Program* getRGB10_3DLUT_10Bit_Kernel() { return &mRGB10_3DLUT_10Bit_Kernel; }
    cl::Program* getRGB48_3DLUT_16Bit_Kernel() { return &mRGB48_3DLUT_16Bit_Kernel; }


signals:
    void droppedFile(QString fileName);

public slots:
    void updateTexture();
    void updateTexture(int pixelFormat, int width, int height, void* buffer,int bufferSize);

protected:
    virtual void	initializeGL();
    virtual bool	initializeTextures(AJA_PixelFormat pixelFormat,int width, int height);
    virtual void	paintGL();
    virtual void	resizeGL(int w, int h) ;
    virtual void	paintEvent(QPaintEvent* event);

    virtual bool    initializeCL();
    virtual bool    testOpenCL();

    virtual bool   createKernel(cl::Program& program,const QString name);
    virtual void   runCLKernel();


    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

    void createPreviewShaderProgram();

    QOpenGLShaderProgram* mShaderProgram;

    GLuint mVAO;
    GLuint mPositionBufferObject;
    GLuint mElementBufferObject;
    GLuint rgbPreviewTexture;
    GLint mRGBPreviewUniformTexture;
    GLint mAttributesPosition;

    int mWidth;
    int mHeight;
    AJA_PixelFormat mPixelFormat;
	bool mInitialized;

    cl::Device default_device;
    cl::Context contextCL;
    cl::CommandQueue queue;
    cl::Program mCbYCr8Kernel;
    cl::Program mCbYCr8AltKernel;
    cl::Program mCbYCr10Kernel;
    cl::Program mRGBA8Kernel;
    cl::Program mARGB8Kernel;
    cl::Program mABGR8Kernel;
    cl::Program mRGB10Kernel;
    cl::Program mRGB10_3DLUTKernel;
    cl::Program mRGB10_3DLUT_10Bit_Kernel;
    cl::Program mRGB48_3DLUT_16Bit_Kernel;
    cl::Program mRGB10DPXLEKernel;
    cl::Program mRGB10DPXBEKernel;
    cl::Program mRGB8Kernel;
    cl::Program mBGR8Kernel;
    cl::Program mRGB16Kernel;    
    cl::Program mS2022_6_720p50Kernel;
    cl::Program mS2022_6_720p60Kernel;
    cl::Program mS2022_6_1080iKernel;
    cl::Program mS2022_6_1080i25Kernel;
    cl::Program mS2022_6_1080p30Kernel;
    cl::Program mS2022_6_1080p25Kernel;
    cl::Program mS2022_6_1080p24Kernel;
    cl::Program mS2022_6_525Kernel;
    cl::Program mS2022_6_625Kernel;
    cl::Program mRFC4175_1080pKernel;
    cl::Program mRFC4175_1080iKernel;
    cl::Program mRFC4175_720pKernel;
    cl::Program mRFC4175_525Kernel;
    cl::Program mRFC4175_625pKernel;

    cl::Image2D* inputImage;
#if !defined(CL_VERSION_1_2)
    cl::Image2DGL* outputImage;
#else
    cl::ImageGL* outputImage;
#endif
    cl::Buffer*  inputBuffer;

    cl::Program::Sources sources;

    QMutex textureMutex;

};

#endif // OGLPREVIEWWIDGET_H
