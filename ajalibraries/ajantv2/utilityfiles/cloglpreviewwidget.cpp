#include "cloglpreviewwidget.h"
#include <QDebug>
#include <assert.h>
#include <QElapsedTimer>
#include <QTimer>

#include <QImageReader>
#include <QFile>
#include <QDragEnterEvent>
#include <QMimeData>

#include <QMessageBox>

void testOpenCL();

#ifdef AJALinux
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

static const GLfloat g_vertex_buffer_data[] = {
    -1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
    1.0f,  1.0f, 0.0f, 1.0f

};
static const GLushort g_element_buffer_data[] = { 0, 1, 2, 3 };


//AJA_PixelFormat_YCbCr8
//AJA_PixelFormat_ARGB8
//AJA_PixelFormat_RGB10
//AJA_PixelFormat_RGB_DPX
//AJA_PixelFormat_RGB_DPX_LE
//AJA_PixelFormat_YCbCr10
COpenGLPreviewWidget::COpenGLPreviewWidget(QWidget *parent )
    : QOpenGLWidget(parent),mWidth(1280),mHeight(720),mPixelFormat(AJA_PixelFormat_RGB10),mInitialized(false),inputImage(NULL),outputImage(NULL),inputBuffer(NULL)
{
    //    QTimer *timer = new QTimer(this);
    //    connect(timer, SIGNAL(timeout()), this, SLOT(updateTexture()));
    //    timer->start(1000);
    initialize3DLUT("passthrough.cube");
    setAcceptDrops(true);
}

COpenGLPreviewWidget::~COpenGLPreviewWidget()
{

}

bool COpenGLPreviewWidget::isSupportedByHostGPU()
{
    // do test only once
    static int isCompatible = -1;
    if (isCompatible != -1)
        return isCompatible;

    COpenGLPreviewWidget w;
    w.resize(10,10);
    w.move(-200,-200);
    w.show();	// results in initializeGL being called
    isCompatible = w.mInitialized;
    w.doneCurrent();
    return isCompatible;
}

void COpenGLPreviewWidget::setAJAPixelFormat(AJA_PixelFormat pixelFormat,int width, int height )
{
    initializeTextures(pixelFormat,width,height);
    //    qDebug () << "pf " << (int)pixelFormat <<"w " << width << "h " << height;
}

bool COpenGLPreviewWidget::initializeTextures(AJA_PixelFormat pixelFormat,int width, int height)
{
    QMutexLocker locker(&textureMutex);

    mPixelFormat = pixelFormat;
    mWidth = width;
    mHeight = height;


    //
    // Initialize/Reinitialize OpenCL memory objects and OpenGL textures
    //

    if ( inputImage)
    {
        delete inputImage;
        inputImage = NULL;
    }
    if ( inputBuffer)
    {
        delete inputBuffer;
        inputBuffer = NULL;
    }
    if ( outputImage )
    {
        delete outputImage;
        outputImage = NULL;
    }

    glBindTexture(GL_TEXTURE_2D, rgbPreviewTexture);
    glTexImage2D(
                GL_TEXTURE_2D, 0,           /* target, level */
                GL_RGBA8,                    /* internal format */
                mWidth, mHeight, 0,           /* width, height, border */
                GL_BGRA, GL_UNSIGNED_BYTE,   /* external format, type */
                NULL
                );

    if (GL_NO_ERROR != glGetError())
        return false;

    //
    // Initialize / Reinitialize OpenCL buffers and images
    //

    // Create OpenCL Image2D for input
    cl::ImageFormat imgfmt;
    cl_int result;
    switch(mPixelFormat)
    {
    case AJA_PixelFormat_RGB_DPX:
    case AJA_PixelFormat_RGB_DPX_LE:
    case AJA_PixelFormat_RGB10:
        imgfmt.image_channel_data_type = CL_UNSIGNED_INT32;
        imgfmt.image_channel_order = CL_R;
        inputImage = new cl::Image2D(contextCL, CL_MEM_READ_ONLY, imgfmt, mWidth, mHeight, 0, 0, &result);
        break;
    case AJA_PixelFormat_RGB10_3DLUT:
        imgfmt.image_channel_data_type = CL_UNSIGNED_INT32;
        imgfmt.image_channel_order = CL_R;
        inputImage = new cl::Image2D(contextCL, CL_MEM_READ_ONLY, imgfmt, mWidth, mHeight, 0, 0, &result);
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,33*33*33*4);
        break;
    case AJA_PixelFormat_YCbCr8:
    case AJA_PixelFormat_YUY28:
        imgfmt.image_channel_data_type = CL_UNSIGNED_INT8;
        imgfmt.image_channel_order = CL_RG;
        inputImage = new cl::Image2D(contextCL, CL_MEM_READ_ONLY, imgfmt, mWidth, mHeight, 0, 0, &result);
        break;
    case AJA_PixelFormat_ARGB8:
    case AJA_PixelFormat_RGBA8:
    case AJA_PixelFormat_ABGR8:
        imgfmt.image_channel_data_type = CL_UNSIGNED_INT8;
        imgfmt.image_channel_order = CL_RGBA;
        inputImage = new cl::Image2D(contextCL, CL_MEM_READ_ONLY, imgfmt, mWidth, mHeight, 0, 0, &result);
        break;
    case AJA_PixelFormat_RGB8_PACK:
    case AJA_PixelFormat_BGR8_PACK:
    {
        uint32_t rowBytes = AJA_CalcRowBytesForFormat(mPixelFormat, mWidth);
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*mHeight);
    }
        break;
    case AJA_PixelFormat_YCbCr10:
    {
        uint32_t rowBytes = AJA_CalcRowBytesForFormat(mPixelFormat, mWidth);
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*mHeight);
    }
        break;

    case AJA_PixelFormat_RGB16:
    {
        uint32_t rowBytes = AJA_CalcRowBytesForFormat(mPixelFormat, mWidth);
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*mHeight);
    }
        break;

    case AJA_PixelFormat_S0226_720p50:
    {
        uint32_t rowBytes = 4950;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*750);
    }
        break;
    case AJA_PixelFormat_S0226_720p60:
    {
        uint32_t rowBytes = 4125;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*750);
    }
        break;
    case AJA_PixelFormat_S0226_1080i30:
    {
        uint32_t rowBytes = 5500;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*1125);
    }
        break;

    case AJA_PixelFormat_S0226_1080i25:
    {
        uint32_t rowBytes = 6600;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*1125);
    }
        break;

    case AJA_PixelFormat_S0226_1080p30:
    {
        uint32_t rowBytes = 5500;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*1125);
    }
        break;

    case AJA_PixelFormat_S0226_1080p25:
    {
        uint32_t rowBytes = 6600;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*1125);
    }
        break;

    case AJA_PixelFormat_S0226_1080p24:
    {
        uint32_t rowBytes = 6875;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*1125);
    }
        break;

    case AJA_PixelFormat_S0226_525i30:
    {
        uint32_t rowBytes = 2145;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*525);
    }
        break;

    case AJA_PixelFormat_S0226_625i25:
    {
        uint32_t rowBytes = 2160;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*625);
    }
        break;

    case AJA_PixelFormat_RFC4175_1080p:
    {
        uint32_t rowBytes = 4800;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*1080);
    }
        break;

    case AJA_PixelFormat_RFC4175_1080i:
    {
        uint32_t rowBytes = 4800;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*540);
    }
        break;

    case AJA_PixelFormat_RFC4175_720p:
    {
        uint32_t rowBytes = 3200;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*720);
    }
        break;

    case AJA_PixelFormat_RFC4175_525i30:
    {
        uint32_t rowBytes = 1800;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*243);
    }
        break;

    case AJA_PixelFormat_RFC4175_625i25:
    {
        uint32_t rowBytes = 1800;
        inputBuffer = new cl::Buffer(contextCL,CL_MEM_READ_ONLY,rowBytes*288);
    }
        break;

    case AJA_PixelFormat_Unknown:
    default:
        break;
    }

    // Create output image from OpenGL 2D texture
#if !defined(CL_VERSION_1_2)
    outputImage = new cl::Image2DGL(contextCL, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, rgbPreviewTexture, &result);
#else
    outputImage = new cl::ImageGL(contextCL, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, rgbPreviewTexture, &result);
#endif

    return (result == CL_SUCCESS);
}

void	COpenGLPreviewWidget::initializeGL()
{
    bool status = initializeOpenGLFunctions();
    if  (status == false)
    {
        qDebug() << "fail initializeOpenGLFunctions";
        return;
    }

    createPreviewShaderProgram();

    glClearColor(.0f,.3f,.3f,1.0f);

    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    glGenBuffers(1, &mPositionBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, mPositionBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &mElementBufferObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBufferObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_element_buffer_data), g_element_buffer_data, GL_STATIC_DRAW);

    glGenTextures(1, &rgbPreviewTexture);
    glBindTexture(GL_TEXTURE_2D, rgbPreviewTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    //glViewport(0,0,mWidth,mHeight);
    if (GL_NO_ERROR != glGetError()) {
        qDebug() << "fail texture bind";
        return;
    }

    if (initializeCL() == false) {
        qDebug() << "fail initializeCL";
        return;
    }

    if (initializeTextures(mPixelFormat,mWidth,mHeight) == false) {
        qDebug() << "fail initializeTextures";
        return;
    }

    mInitialized = true;
}

void COpenGLPreviewWidget::paintEvent(QPaintEvent* event)
{
    QOpenGLWidget::paintEvent(event);
}

void	COpenGLPreviewWidget::paintGL()
{
    QMutexLocker locker(&textureMutex);


    // For some reason, paintGL() is called before the pixel format is
    // set or initial data is loaded.  In this case simply return
    if (mPixelFormat == AJA_PixelFormat_Unknown)
        return;

    //qDebug() << "painting";

    // First Run Kernel to convert incoming pixelformat to rgbPreviewTexture
    runCLKernel();

    // Draw result
    assert (GL_NO_ERROR == glGetError());
    mShaderProgram->bind();

    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rgbPreviewTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );
    glUniform1i(mRGBPreviewUniformTexture, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mPositionBufferObject);
    glVertexAttribPointer(
                mAttributesPosition,  /* attribute */
                4,                                /* size */
                GL_FLOAT,                         /* type */
                GL_FALSE,                         /* normalized? */
                sizeof(GLfloat)*4,                /* stride */
                (void*)0                          /* array buffer offset */
                );
    glEnableVertexAttribArray(mAttributesPosition);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBufferObject);
    glDrawElements(
                GL_TRIANGLE_STRIP,  /* mode */
                4,                  /* count */
                GL_UNSIGNED_SHORT,  /* type */
                (void*)0            /* element array buffer offset */
                );

    glDisableVertexAttribArray(mAttributesPosition);
    assert (GL_NO_ERROR == glGetError());
}

void	COpenGLPreviewWidget::resizeGL(int w, int h)
{
    (void)w; (void) h;
    //qDebug() << "resize" << w << " x " << h;
    if (mInitialized)
        glViewport(0,0,width(),height());
}

void COpenGLPreviewWidget::runCLKernel()
{
    if (mInitialized == false)
        return;



    cl_int result = CL_SUCCESS;
    cl::Event ev;
    cl::NDRange globalRange = cl::NDRange(mWidth, mHeight);
    uint16_t lW=8, lH = (mHeight%8) == 0 ? 8 : 6;
    if (mHeight < 300) lH = 1;
    cl::NDRange localRange = cl::NDRange(lW, lH);

    std::vector<cl::Memory> memObjs;
    memObjs.clear();
    memObjs.push_back(*outputImage);

    // Acquire OpenGL textures
    result = queue.enqueueAcquireGLObjects(&memObjs, NULL, &ev);
    ev.wait();
    assert(result == CL_SUCCESS);

    cl::Kernel currentKernel;
    uint32_t argNumber = 0;
    switch ( mPixelFormat)
    {
    case AJA_PixelFormat_RGB10:
        currentKernel = cl::Kernel(mRGB10Kernel, "RGB10");
        currentKernel.setArg(argNumber++, *inputImage);
        break;
    case AJA_PixelFormat_RGB10_3DLUT:
        currentKernel = cl::Kernel(mRGB10_3DLUTKernel, "RGB10_3DLUT");
        currentKernel.setArg(argNumber++, *inputImage);
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_RGB_DPX:
        currentKernel = cl::Kernel(mRGB10DPXBEKernel, "RGB10DPXBE");
        currentKernel.setArg(argNumber++, *inputImage);
        break;
    case AJA_PixelFormat_RGB_DPX_LE:
        currentKernel = cl::Kernel(mRGB10DPXLEKernel, "RGB10DPXLE");
        currentKernel.setArg(argNumber++, *inputImage);
        break;
    case AJA_PixelFormat_ARGB8:
        currentKernel = cl::Kernel(mARGB8Kernel, "ARGB8");
        currentKernel.setArg(argNumber++, *inputImage);
        break;
    case AJA_PixelFormat_RGBA8:
        currentKernel = cl::Kernel(mRGBA8Kernel, "RGBA8");
        currentKernel.setArg(argNumber++, *inputImage);
        break;
    case AJA_PixelFormat_ABGR8:
        currentKernel = cl::Kernel(mABGR8Kernel, "ABGR8");
        currentKernel.setArg(argNumber++, *inputImage);
        break;
    case AJA_PixelFormat_RGB8_PACK:
        currentKernel = cl::Kernel(mRGB8Kernel, "RGB8");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_BGR8_PACK:
        currentKernel = cl::Kernel(mBGR8Kernel, "BGR8");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_RGB16:
        currentKernel = cl::Kernel(mRGB16Kernel, "RGB16");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_YCbCr8:
        currentKernel = cl::Kernel(mCbYCr8Kernel, "CbYCr8ToRGB8");
        currentKernel.setArg(argNumber++, *inputImage);
        break;
    case AJA_PixelFormat_YUY28:
        currentKernel = cl::Kernel(mCbYCr8AltKernel, "CbYCr8AltToRGB8");
        currentKernel.setArg(argNumber++, *inputImage);
        break;
    case AJA_PixelFormat_YCbCr10:
        currentKernel = cl::Kernel(mCbYCr10Kernel, "CbYCr10");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_720p50:
        currentKernel = cl::Kernel(mS2022_6_720p50Kernel, "S2022_6_720p50");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_720p60:
        currentKernel = cl::Kernel(mS2022_6_720p60Kernel, "S2022_6_720p60");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_1080i30:
        currentKernel = cl::Kernel(mS2022_6_1080iKernel, "S2022_6_1080i");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_1080i25:
        currentKernel = cl::Kernel(mS2022_6_1080i25Kernel, "S2022_6_1080i25");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_1080p30:
        currentKernel = cl::Kernel(mS2022_6_1080p30Kernel, "S2022_6_1080p30");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_1080p25:
        currentKernel = cl::Kernel(mS2022_6_1080p25Kernel, "S2022_6_1080p25");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_1080p24:
        currentKernel = cl::Kernel(mS2022_6_1080p24Kernel, "S2022_6_1080p24");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_525i30:
        currentKernel = cl::Kernel(mS2022_6_525Kernel, "S2022_6_525");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_S0226_625i25:
        currentKernel = cl::Kernel(mS2022_6_625Kernel, "S2022_6_625");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_RFC4175_1080p:
        currentKernel = cl::Kernel(mRFC4175_1080pKernel, "RFC4175_1080p");
        currentKernel.setArg(0, *inputBuffer);
        break;
    case AJA_PixelFormat_RFC4175_1080i:
        currentKernel = cl::Kernel(mRFC4175_1080iKernel, "RFC4175_1080i");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_RFC4175_720p:
        currentKernel = cl::Kernel(mRFC4175_720pKernel, "RFC4175_720p");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_RFC4175_525i30:
        currentKernel = cl::Kernel(mRFC4175_525Kernel, "RFC4175_525");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    case AJA_PixelFormat_RFC4175_625i25:
        currentKernel = cl::Kernel(mRFC4175_625pKernel, "RFC4175_625");
        currentKernel.setArg(argNumber++, *inputBuffer);
        break;
    default:
        assert(false);
        break;
    }

    currentKernel.setArg(argNumber, *outputImage);
    result = queue.enqueueNDRangeKernel(currentKernel, cl::NullRange, globalRange, localRange);
    assert(result == CL_SUCCESS);

    // Release OpenGL textures
    result = queue.enqueueReleaseGLObjects(&memObjs, NULL, &ev);
    ev.wait();
    assert(result == CL_SUCCESS);

    queue.finish();
}


bool  COpenGLPreviewWidget::initializeCL()
{
    const bool kPrintInfo = false;

    //get all platforms (drivers)
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if(all_platforms.size()==0){
        if (kPrintInfo) qDebug() <<" No platforms found. Check OpenCL installation!\n";
        return false;
    }
    cl::Platform default_platform=all_platforms[0];
    std::string platformStr = default_platform.getInfo<CL_PLATFORM_NAME>();
    if (kPrintInfo) qDebug()  << "Using platform: "<< platformStr.c_str() <<"\n";

    //get default device of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_GPU, &all_devices);
    if(all_devices.size()==0){
        if (kPrintInfo) qDebug() <<" No devices found. Check OpenCL installation!\n";
        return false;
    }

    bool deviceFound = false;
    for (int deviceNum=0; deviceNum<(int)all_devices.size(); deviceNum++)
    {
        std::string deviceStr = all_devices[deviceNum].getInfo<CL_DEVICE_NAME>();
        if (kPrintInfo) qDebug() << "Using device: "<<deviceStr.c_str()<<"\n";
        std::string extentionsStr = all_devices[deviceNum].getInfo<CL_DEVICE_EXTENSIONS>();
        if (kPrintInfo) qDebug() << "Extentions: "<<extentionsStr.c_str();
        cl_device_type deviceType = all_devices[deviceNum].getInfo<CL_DEVICE_TYPE>();
        if (kPrintInfo) qDebug() << "type: "<<(int)deviceType;

        default_device=all_devices[deviceNum];
#if defined __APPLE__ || defined(MACOSX)
        // Get current CGL Context and CGL Share group
        CGLContextObj kCGLContext = CGLGetCurrentContext();
        CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
        // Create CL context properties, add handle & share-group enum !
        cl_context_properties properties[] = {
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
            (cl_context_properties)kCGLShareGroup, 0
        };
        // Create a context with device in the CGL share group
        //cl_context context = clCreateContext(properties, 0, 0, NULL, 0, 0);
#elif defined AJA_WINDOWS
        // Create context from OpenGL context
        cl_context_properties properties[] = {
            CL_GL_CONTEXT_KHR,   (cl_context_properties)wglGetCurrentContext(), // WGL Context
            CL_WGL_HDC_KHR,      (cl_context_properties)wglGetCurrentDC(),      // WGL HDC
            CL_CONTEXT_PLATFORM, (cl_context_properties)default_platform(), // OpenCL platform
            0
        };
#else
        cl_context_properties properties[] = {
            CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
            CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
            CL_CONTEXT_PLATFORM, (cl_context_properties)default_platform(), // OpenCL platform
            0};
#endif
        
        // create context for default device
        //contextCL = cl::Context(CL_DEVICE_TYPE_GPU, properties);
        contextCL = cl::Context(default_device,properties);

        //create queue to which we will push commands for the device.
        queue = cl::CommandQueue(contextCL,default_device);
        
        deviceFound = testOpenCL();
        if (deviceFound == true) {
            std::string defaultDeviceStr = default_device.getInfo<CL_DEVICE_NAME>();
            if (kPrintInfo) qDebug() << "Using device: "<<defaultDeviceStr.c_str()<<"\n";
            break;
        }
    }
    
    if (deviceFound == false) {
        qDebug() <<" No device passed OpenCL test.\n";
        return false;
    }
    
    createKernel(mRGB10DPXLEKernel,":/gpufiles/rgb10dpxle.cl");
    createKernel(mRGB10DPXBEKernel,":/gpufiles/rgb10dpxbe.cl");
    createKernel(mRGB10Kernel,":/gpufiles/rgb10.cl");
    createKernel(mRGB10_3DLUTKernel,":/gpufiles/rgb10_3dlut.cl");
    createKernel(mRGB10_3DLUT_10Bit_Kernel,":/gpufiles/rgb10_3dlut_10bit.cl");
    createKernel(mRGBA8Kernel,":/gpufiles/rgba8.cl");
    createKernel(mARGB8Kernel,":/gpufiles/argb8.cl");
    createKernel(mABGR8Kernel,":/gpufiles/abgr8.cl");
    createKernel(mRGB8Kernel,":/gpufiles/rgb8.cl");
    createKernel(mBGR8Kernel,":/gpufiles/bgr8.cl");
    createKernel(mRGB16Kernel,":/gpufiles/rgb16.cl");
    createKernel(mCbYCr10Kernel,":/gpufiles/cbycr10.cl");
    createKernel(mCbYCr8Kernel,":/gpufiles/cbycr8.cl");
    createKernel(mCbYCr8AltKernel,":/gpufiles/cbycr8alt.cl");
    createKernel(mS2022_6_720p50Kernel,":/gpufiles/s2022_6_720p50.cl");
    createKernel(mS2022_6_720p60Kernel,":/gpufiles/s2022_6_720p60.cl");
    createKernel(mS2022_6_1080iKernel,":/gpufiles/s2022_6_1080i.cl");
    createKernel(mS2022_6_1080i25Kernel,":/gpufiles/s2022_6_1080i25.cl");
    createKernel(mS2022_6_1080p30Kernel,":/gpufiles/s2022_6_1080p30.cl");
    createKernel(mS2022_6_1080p25Kernel,":/gpufiles/s2022_6_1080p25.cl");
    createKernel(mS2022_6_1080p24Kernel,":/gpufiles/s2022_6_1080p24.cl");
    createKernel(mS2022_6_525Kernel,":/gpufiles/s2022_6_525.cl");
    createKernel(mS2022_6_625Kernel,":/gpufiles/s2022_6_625.cl");
    createKernel(mRFC4175_1080pKernel,":/gpufiles/rfc4175_1080p.cl");
    createKernel(mRFC4175_1080iKernel,":/gpufiles/rfc4175_1080i.cl");
    createKernel(mRFC4175_720pKernel,":/gpufiles/rfc4175_720p.cl");
    createKernel(mRFC4175_525Kernel,":/gpufiles/rfc4175_525.cl");
    createKernel(mRFC4175_625pKernel,":/gpufiles/rfc4175_625.cl");

    return true;
}

void COpenGLPreviewWidget::createPreviewShaderProgram()
{
    mShaderProgram = new QOpenGLShaderProgram();

    // Compile vertex shader
    if ( !mShaderProgram->addShaderFromSourceFile( QOpenGLShader::Vertex, ":/gpufiles/simple.vert" ) )
        qCritical() << "Unable to compile vertex shader. Log:" << mShaderProgram->log();

    // Compile fragment shader
    if ( !mShaderProgram->addShaderFromSourceFile( QOpenGLShader::Fragment, ":/gpufiles/simple.frag" ) )
        qCritical() << "Unable to compile fragment shader. Log:" << mShaderProgram->log();

    // Link the shaders together into a program
    if ( !mShaderProgram->link() )
        qCritical() << "Unable to link shader program. Log:" << mShaderProgram->log();


    mRGBPreviewUniformTexture = mShaderProgram->uniformLocation("rgbPreviewTexture");
    mAttributesPosition = mShaderProgram->attributeLocation("position");
}

#include <QFile>
typedef struct {
    double rCoef;
    double gCoef;
    double bCoef;
} RGBCoefs;

typedef struct {
    uint16_t rCoef;
    uint16_t gCoef;
    uint16_t bCoef;
    uint16_t aCoef;
} RGBA16BitCoefs;

typedef uint32_t RGBCoefsInt;

RGBCoefs lut3D[33][33][33]; //[B][G][R]
RGBCoefsInt lut3DInts[33][33][33];
RGBA16BitCoefs lut3DRGBA16Ints[33][33][33];

static uint32_t TestInterpolation(uint32_t inValue,RGBCoefs* pLut3D);
static uint32_t TestInterpolationInteger(uint32_t inValue,RGBCoefsInt* pLut3D);
static void Tetrahedral16bit(int ri,int gi,int bi,int &ro,int &go,int &bo,unsigned long long int * pLut3D);
uint32_t count = 0;

//RGB10 bit Color Bars
// White:  0x3FFFFBFF
// Yellow: 0xFFFFF
// Cyan:   0x3FFFFC00
// Green:  0xFFC00
// Magenta:0x3FF003FF
// Red:    0x3FF
// Blue:   0x3FF00000
// Black:  0x0

#define RX(i)  ((i)&0x3FF)
#define INDEX_R(i) (((i)&0x3FF)>>5)
#define FRACTION_R(i) (((i)&0x3FF)&0x1F)

#define GX(i)  ((i>>10)&0x3FF)
#define INDEX_G(i) (((i>>10)&0x3FF)>>5)
#define FRACTION_G(i) (((i>>10)&0x3FF)&0x1F)

#define BX(i)  ((i>>20)&0x3FF)
#define INDEX_B(i) (((i>>20)&0x3FF)>>5)
#define FRACTION_B(i) (((i>>20)&0x3FF)&0x1F)

void COpenGLPreviewWidget::initialize3DLUT(QString fileName)
{
    uint32_t v;
#if 0
    qDebug() << "generating passthrough";
    //////passthrough lut generated.
    for ( int b=0; b<33; b++)
    {
        for ( int g=0;g<33; g++)
        {
            for ( int r=0; r<33; r++ )
            {
                RGBCoefs c;
                c.rCoef = ((float)(r<<5))/1024.0;
                c.gCoef =  ((float)(g<<5))/1024.0;
                c.bCoef =  ((float)(b<<5))/1024.0;
                //                qDebug() << c.rCoef << c.gCoef << c.bCoef;

                lut3D[b][g][r] = c;

                lut3DInts[b][g][r] = (((uint32_t)(c.bCoef*1023.0))<<20) + (((uint32_t)(c.gCoef*1023.0))<<10) + ((uint32_t)(c.rCoef*1023.0));;
            }
        }
    }


#else
    QFile inputFile(fileName);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        RGBCoefs* coefPtr = &lut3D[0][0][0];
        RGBCoefsInt* coefIntPtr = &lut3DInts[0][0][0];
        RGBA16BitCoefs* coefInt16Ptr = &lut3DRGBA16Ints[0][0][0];
        uint32_t tableEntries = 0;
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if ( line.startsWith("TITLE"))
                continue;
            if ( line.startsWith("LUT_3D_SIZE"))
                continue;
            if ( line.startsWith("LUT_3D_INPUT_RANGE"))
                continue;
            if ( line.startsWith("#"))
                continue;

            QStringList list = line.split(" ",QString::SkipEmptyParts);
            if ( list.size() != 3)
                continue;

            QString rString = list.at(0);
            coefPtr->rCoef = rString.toDouble();
            QString gString = list.at(1);
            coefPtr->gCoef = gString.toDouble();
            QString bString = list.at(2);
            coefPtr->bCoef = bString.toDouble();
            *coefIntPtr++ = (((uint32_t)(coefPtr->bCoef*1023.9))<<20) + (((uint32_t)(coefPtr->gCoef*1023.9))<<10) + ((uint32_t)(coefPtr->rCoef*1023.9));
            coefInt16Ptr->rCoef =(uint16_t)(coefPtr->bCoef*65535.9);
            coefInt16Ptr->gCoef =(uint16_t)(coefPtr->gCoef*65535.9);
            coefInt16Ptr->bCoef =(uint16_t)(coefPtr->bCoef*65535.9);
            coefInt16Ptr++;
            coefPtr++;

            tableEntries++;
            if ( tableEntries > (33*33*33))
                break;
            //
        }
        if( tableEntries != (33*33*33))
        {
            QMessageBox msgBox;
            msgBox.setText("3DLut not Valid, Needs to be 33 Point .cube file");
            msgBox.exec();
        }
        inputFile.close();

    }

//    for ( int r=0;r<1024;r++)
//    {
//        uint32_t v;
//        v = TestInterpolation(r<<20, (RGBCoefs*)lut3D);
//        qDebug() << hex << r << (v>>20);
//        v = TestInterpolationInteger(r<<20, (RGBCoefsInt*)lut3DInts);
//        qDebug() << hex << r << (v>>20);

//    }

#endif
    qDebug()  <<  lut3D[31][31][31].bCoef <<  lut3D[31][31][31].gCoef <<  lut3D[31][31][31].rCoef;
    qDebug() <<  hex <<  lut3DInts[31][31][31];
    qDebug()  <<  lut3D[32][32][32].bCoef <<  lut3D[32][32][32].gCoef <<  lut3D[32][32][32].rCoef;
    qDebug() <<  hex <<  lut3DInts[32][32][32];
    v = TestInterpolation(0x3FFFFFFF, (RGBCoefs*)lut3D);
    qDebug() << hex << v;
    v = TestInterpolationInteger(0x3FFFFFFF, (RGBCoefsInt*)lut3DInts);
    qDebug() << hex << v;

    int red = 0xFFFF;
    int green = 0xFFFF;
    int blue = 0xFFFF;
    Tetrahedral16bit(red,green,blue,red,green,blue,(unsigned long long int *) lut3DRGBA16Ints);
    qDebug() << hex << red << green << blue;

}

void COpenGLPreviewWidget::readLUT16(QString fileName)
{
    QFile inputFile(fileName);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        if ( inputFile.size() == 287496)
        {
            inputFile.read((char*)lut3DRGBA16Ints,287496) ;

            for ( int b=0; b<33; b++)
            {
                for ( int g=0;g<33; g++)
                {
                    for ( int r=0; r<33; r++ )
                    {
                        uint16_t ri,gi,bi;
                        ri = (lut3DRGBA16Ints[b][g][r].rCoef>>6)&0x3FF;
                        gi = (lut3DRGBA16Ints[b][g][r].gCoef>>6)&0x3FF;
                        bi = (lut3DRGBA16Ints[b][g][r].bCoef>>6)&0x3FF;
                        uint32_t value = ri + (gi<<10) + (bi<<20);
                        lut3DInts[b][g][r] = value;
                    }
                }
            }
        }
        inputFile.close();
        int red = 0xFFFF;
        int green = 0xFFFF;
        int blue = 0xFFFF;
        Tetrahedral16bit(red,green,blue,red,green,blue,(unsigned long long int *) lut3DRGBA16Ints);
        qDebug() << hex << red << green << blue;
    }
}

void	COpenGLPreviewWidget::updateTexture()
{
    //        QFile f("c:/2022_6_1080i2997_1.dat");
    //        f.open(QIODevice::ReadOnly);
    //        QByteArray ba = f.read(1024*1024*8);
    //         updateTexture(AJA_PixelFormat_YCBCR10_420PL,1920,1080,ba.data(),(int)5500*1125);
    QFile inputFile("W-ARAPAHO-33-LOG.cube");
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        RGBCoefs* coefPtr = &lut3D[0][0][0];
        RGBCoefsInt* coefIntPtr = &lut3DInts[0][0][0];
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if ( line.startsWith("TITLE"))
                continue;
            if ( line.startsWith("LUT_3D_SIZE"))
                continue;
            if ( line.startsWith("LUT_3D_INPUT_RANGE"))
                continue;
            if ( line.startsWith("#"))
                continue;

            QStringList list = line.split(" ",QString::SkipEmptyParts);
            if ( list.size() != 3)
                continue;
            QString rString = list.at(0);
            coefPtr->rCoef = rString.toFloat();
            QString gString = list.at(1);
            coefPtr->gCoef = gString.toFloat();
            QString bString = list.at(2);
            coefPtr->bCoef = bString.toFloat();

            *coefIntPtr++ = (((uint32_t)(coefPtr->bCoef*1023.0))<<20) + (((uint32_t)(coefPtr->gCoef*1023.0))<<10) + ((uint32_t)(coefPtr->rCoef*1023.0));
            coefPtr++;
            //
        }
        //        for ( int i = 0; i < 10; i++ )
        //        {
        //            RGBCoefs c = lut3D[0][0][i];
        //            qDebug() << c.rCoef << c.gCoef << c.bCoef;
        //        }
        inputFile.close();
    }


    AJATestPatternBuffer tpBuffer;
    AJATestPatternGen tp;

    tp.DrawTestPattern((AJATestPatternSelect)count, mWidth, mHeight, AJA_PixelFormat_RGB10, tpBuffer);

    //    uint32_t* t = (uint32_t*)tpBuffer.data();
    //    for ( int i = 0; i < mWidth*mHeight; i++ )
    //    {

    ////        uint32_t valueByFloat = TestInterpolation(*t,(RGBCoefs*)lut3D);
    //        uint32_t valueByInt = TestInterpolationInteger(*t,(RGBCoefsInt*)lut3DInts);
    ////        if ( valueByFloat != valueByInt)
    ////            qDebug() << hex << RX(valueByFloat) << RX(valueByInt);
    //        *t = valueByInt;
    //        t++;

    //    }

    updateTexture(mPixelFormat,mWidth,mHeight,tpBuffer.data(),(int)tpBuffer.size());


    count++;
    if ( count == AJA_TestPatt_All)
        count = 0;

}

/////note register AJA_PixelFormat with QMetaDataType world.
void COpenGLPreviewWidget::updateTexture(int pixelFormat, int width, int height, void* buffer,int bufferSize)
{
    if (mInitialized == false)
    {
        qDebug() << "updateTexture called before initializeCL";
        return;
    }

    cl_int res = CL_SUCCESS;

    // If anything has changed reinitialize textures and buffers
    AJA_PixelFormat ajaPixelFormat = (AJA_PixelFormat)pixelFormat;
    //    if ( AJA_PixelFormat_RGB10 == ajaPixelFormat) ajaPixelFormat = AJA_PixelFormat_RGB10_3DLUT;

    if ( ajaPixelFormat != mPixelFormat || width != mWidth || height != mHeight)
    {
        setAJAPixelFormat(ajaPixelFormat,width,height);
    }

    QElapsedTimer timer;
    timer.start();

    cl::size_t<3> origin;
    origin[0] = 0; origin[1] = 0; origin[2] = 0;
    cl::size_t<3> region;
    region[0] = mWidth; region[1] = mHeight; region[2] = 1;

    switch ( ajaPixelFormat)
    {
    case AJA_PixelFormat_RGB_DPX :
    case AJA_PixelFormat_RGB_DPX_LE :
    case AJA_PixelFormat_RGB10 :
    case AJA_PixelFormat_ARGB8 :
    case AJA_PixelFormat_RGBA8 :
    case AJA_PixelFormat_ABGR8 :
    case AJA_PixelFormat_YCbCr8:
    case AJA_PixelFormat_YUY28:

        // copy input buffer to the device
        res = queue.enqueueWriteImage(*inputImage, CL_TRUE, origin, region, 0, 0, buffer, 0, NULL);
        if ( res != CL_SUCCESS )
            qDebug() << "enqueueWriteImage failed";

        break;
    case AJA_PixelFormat_YCbCr10:
    case AJA_PixelFormat_RGB8_PACK:
    case AJA_PixelFormat_BGR8_PACK:
    case AJA_PixelFormat_RGB16:
    case AJA_PixelFormat_S0226_720p50:
    case AJA_PixelFormat_S0226_720p60:
    case AJA_PixelFormat_S0226_1080i30:
    case AJA_PixelFormat_S0226_1080p30:
    case AJA_PixelFormat_S0226_1080p25:
    case AJA_PixelFormat_S0226_1080p24:
    case AJA_PixelFormat_S0226_525i30:
    case AJA_PixelFormat_S0226_625i25:
    case AJA_PixelFormat_RFC4175_1080p:
    case AJA_PixelFormat_RFC4175_1080i:
    case AJA_PixelFormat_RFC4175_720p:
    case AJA_PixelFormat_RFC4175_525i30:
    case AJA_PixelFormat_RFC4175_625i25:
        res = queue.enqueueWriteBuffer(*inputBuffer,CL_TRUE,0,bufferSize,buffer);
        if ( res != CL_SUCCESS )
            qDebug() << "enqueueWriteBuffer failed";
        break;
    case AJA_PixelFormat_RGB10_3DLUT :
        // copy input buffer to the device
        res = queue.enqueueWriteImage(*inputImage, CL_TRUE, origin, region, 0, 0, buffer, 0, NULL);
        if ( res != CL_SUCCESS )
            qDebug() << "enqueueWriteImage failed";
        res = queue.enqueueWriteBuffer(*inputBuffer,CL_TRUE,0,33*33*33*4,lut3DInts);
        if ( res != CL_SUCCESS )
            qDebug() << "enqueueWriteBuffer failed";

    default:
        break;
    }

    //qDebug() << "updateTexture " << timer.elapsed() << " milliseconds";
    assert (GL_NO_ERROR == glGetError());

    update();
}


bool COpenGLPreviewWidget::createKernel(cl::Program& program,const QString name)
{
    cl::Program::Sources sources;

    QFile f(name);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QByteArray ba = f.readAll();

    std::pair<const char*, ::size_t> code(ba.data(),ba.size());
    sources.push_back(code);

    program =  cl::Program(contextCL, sources);
    VECTOR_CLASS<cl::Device> vectorDevice;
    vectorDevice.push_back(default_device);
    if(program.build(vectorDevice)!=CL_SUCCESS){
        std::string errString = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device);
        qDebug() <<" Error building: " << errString.c_str() <<"\n";
        return false;
    }
    return true;
}


bool COpenGLPreviewWidget::testOpenCL()
{  
    cl::Program::Sources sources;

    // kernel calculates for each element C=A+B
    QFile f(":gpufiles/add.cl");
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QByteArray ba = f.readAll();

    std::pair<const char*, ::size_t> code(ba.data(),ba.size());
    sources.push_back(code);

    cl::Program program(contextCL,sources);
    VECTOR_CLASS<cl::Device> vectorDevice;
    vectorDevice.push_back(default_device);
    if(program.build(vectorDevice)!=CL_SUCCESS){
        std::string errString = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device);
        qDebug() <<" Error building: " << errString.c_str() <<"\n";
        return false;
    }

    // create buffers on the device
    cl::Buffer buffer_A(contextCL,CL_MEM_READ_WRITE,sizeof(int)*10);
    cl::Buffer buffer_B(contextCL,CL_MEM_READ_WRITE,sizeof(int)*10);
    cl::Buffer buffer_C(contextCL,CL_MEM_READ_WRITE,sizeof(int)*10);

    int A[]	= { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int B[]	= { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0};
    int CS[]	= { 0, 2, 4, 3, 5, 7, 6, 8,10, 9};	// checksum solution
    int C[]	= {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};	// checksum test init

    //create queue to which we will push commands for the device.
    cl::CommandQueue queue(contextCL,default_device);

    //write arrays A and B to the device
    queue.enqueueWriteBuffer(buffer_A,CL_TRUE,0,sizeof(int)*10,A);
    queue.enqueueWriteBuffer(buffer_B,CL_TRUE,0,sizeof(int)*10,B);


    //run the kernel
#if 0
    cl::KernelFunctor simple_add(cl::Kernel(program,"simple_add"),queue,cl::NullRange,cl::NDRange(10),cl::NullRange);
    simple_add(buffer_A,buffer_B,buffer_C);
#else
    //alternative way to run the kernel
    cl_int result = CL_SUCCESS;
    cl::Kernel kernel_add=cl::Kernel(program,"simple_add");
    kernel_add.setArg(0,buffer_A);
    kernel_add.setArg(1,buffer_B);
    kernel_add.setArg(2,buffer_C);
    result = queue.enqueueNDRangeKernel(kernel_add,cl::NullRange,cl::NDRange(10),cl::NullRange);
    queue.finish();
#endif
    //read result C from the device to array C
    queue.enqueueReadBuffer(buffer_C,CL_TRUE,0,sizeof(int)*10,C);

    //qDebug() <<" result: \n";
    bool bSuccess = true;
    for(int i=0;i<10;i++)
    {
        //qDebug() <<C[i]<<" ";
        bSuccess = bSuccess==true && C[i]==CS[i];
    }

    return bSuccess;
}

void COpenGLPreviewWidget::dragEnterEvent( QDragEnterEvent *ev )
{
    if (ev->mimeData()->hasFormat("text/uri-list"))
        ev->acceptProposedAction();
    else
        ev->setAccepted( false );
}


void COpenGLPreviewWidget::dropEvent ( QDropEvent * ev )
{
    QList<QUrl> urls = ev->mimeData()->urls();
    if (urls.isEmpty())
        return;

    QString fileName = urls.first().toLocalFile();
    if (fileName.isEmpty())
        return;

    emit droppedFile(fileName);


}

//RGB10 bit Color Bars
// White:  0x3FEFFBFE
// Yellow: 0xFFFFF
// Cyan:   0x3FFFFC00
// Green:  0xFFC00
// Magenta:0x3FF003FF
// Red:    0x3FF
// Blue:   0x3FF00000
// Black:  0x0

#define RX(i)  ((i)&0x3FF)
#define INDEX_R(i) (((i)&0x3FF)>>5)
#define FRACTION_R(i) (((i)&0x3FF)&0x1F)

#define GX(i)  ((i>>10)&0x3FF)
#define INDEX_G(i) (((i>>10)&0x3FF)>>5)
#define FRACTION_G(i) (((i>>10)&0x3FF)&0x1F)

#define BX(i)  ((i>>20)&0x3FF)
#define INDEX_B(i) (((i>>20)&0x3FF)>>5)
#define FRACTION_B(i) (((i>>20)&0x3FF)&0x1F)


uint32_t TestInterpolation(uint32_t inValue,RGBCoefs* pLut3D)
{
    //    inValue = 0x3FFFFC00;
    uint32_t returnValue;

    uint32_t currentPixel = inValue;
    uint32_t nR = INDEX_R(currentPixel);
    double fR = ((FRACTION_R(currentPixel))/32.0);
    uint32_t nG = INDEX_G(currentPixel);
    double fG = (FRACTION_G(currentPixel))/32.0;
    uint32_t nB = INDEX_B(currentPixel);
    double fB = (FRACTION_B(currentPixel))/32.0;

    //    RGBCoefs coefsRGB = lut3D[nB][nG][nR];
    //    RGBCoefs coefsR1GB = lut3D[nB][nG][nR+1];
    //    RGBCoefs coefsRG1B = lut3D[nB][nG+1][nR];
    //    RGBCoefs coefsRGB1 = lut3D[nB+1][nG][nR];
    //    RGBCoefs coefsR1G1B = lut3D[nB][nG+1][nR+1];
    //    RGBCoefs coefsR1GB1 = lut3D[nB+1][nG][nR+1];
    //    RGBCoefs coefsRG1B1 = lut3D[nB+1][nG+1][nR];
    //    RGBCoefs coefsR1G1B1 = lut3D[nB+1][nG+1][nR+1];


    RGBCoefs p1,p2,p3,p4;
    p1 = *(pLut3D + (nB*33*33)+(nG*33)+nR);
    p4 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR+1));
    double f1,f2,f3,f4;
    if ( fG >= fB && fB >= fR )
    {
        // T1
        p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
        f1 = 1.0-fG;
        f2 = fG-fB;
        f3 = fB-fR;
        f4 = fR;

    }
    else if ( fB > fR && fR > fG)
    {
        // T2
        p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
        f1 = 1.0-fB;
        f2 = fB-fR;
        f3 = fR-fG;
        f4 = fG;

    }
    else if ( fB > fG && fG >= fR )
    {
        // T3
        p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
        f1 = 1.0-fB;
        f2 = fB-fG;
        f3 = fG-fR;
        f4 = fR;
    }
    else if ( fR >= fG && fG > fB )
    {
        // T4
        p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
        p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
        f1 = 1.0-fR;
        f2 = fR-fG;
        f3 = fG-fB;
        f4 = fB;
    }
    else if ( fG > fR && fR >=fB )
    {
        // T5
        p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
        p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
        f1 = 1.0-fG;
        f2 = fG-fR;
        f3 = fR-fB;
        f4 = fB;

    }
    else
    {
        // T6
        p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
        f1 = 1.0-fR;
        f2 = fR-fB;
        f3 = fB-fG;
        f4 = fG;
    }

    float R,G,B;
    R = f1*p1.rCoef + f2*p2.rCoef + f3*p3.rCoef + f4*p4.rCoef;
    G = f1*p1.gCoef + f2*p2.gCoef + f3*p3.gCoef + f4*p4.gCoef;
    B = f1*p1.bCoef + f2*p2.bCoef + f3*p3.bCoef + f4*p4.bCoef;

    int32_t red = (uint32_t)(R*1023.9);
    if ( red > 1023) red = 1023;
    if ( red < 0 )   red = 0;
    int32_t green = (uint32_t)(G*1023.9);
    if ( green > 1023) green = 1023;
    if ( green < 0 )   green = 0;
    int32_t blue = (uint32_t)(B*1023.9);
    if ( blue > 1023) blue = 1023;
    if ( blue < 0 )   blue = 0;

    returnValue = ( (blue << 20) + (green<<10) + red);

    return returnValue;
}

#define FIX_ONE_33POINT (0x20)
uint32_t TestInterpolationInteger(uint32_t inValue,RGBCoefsInt* pLut3D)
{
    //    inValue = 0x3FFFFC00;
    uint32_t returnValue;

    uint32_t currentPixel = inValue;
    uint32_t nR = INDEX_R(currentPixel);
    uint16_t fR = FRACTION_R(currentPixel);
    uint32_t nG = INDEX_G(currentPixel);
    uint16_t fG = FRACTION_G(currentPixel);
    uint32_t nB = INDEX_B(currentPixel);
    uint16_t fB = FRACTION_B(currentPixel);

    //    RGBCoefs coefsRGB = lut3D[nB][nG][nR];
    //    RGBCoefs coefsR1GB = lut3D[nB][nG][nR+1];
    //    RGBCoefs coefsRG1B = lut3D[nB][nG+1][nR];
    //    RGBCoefs coefsRGB1 = lut3D[nB+1][nG][nR];
    //    RGBCoefs coefsR1G1B = lut3D[nB][nG+1][nR+1];
    //    RGBCoefs coefsR1GB1 = lut3D[nB+1][nG][nR+1];
    //    RGBCoefs coefsRG1B1 = lut3D[nB+1][nG+1][nR];
    //    RGBCoefs coefsR1G1B1 = lut3D[nB+1][nG+1][nR+1];


    RGBCoefsInt p1,p2,p3,p4;
    p1 = *(pLut3D + (nB*33*33)+(nG*33)+nR);
    p4 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR+1));
    uint16_t f1,f2,f3,f4;
    if ( fG >= fB && fB >= fR )
    {
        // T1
        p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
        f1 = FIX_ONE_33POINT-fG;
        f2 = fG-fB;
        f3 = fB-fR;
        f4 = fR;

    }
    else if ( fB > fR && fR > fG)
    {
        // T2
        p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
        f1 = FIX_ONE_33POINT-fB;
        f2 = fB-fR;
        f3 = fR-fG;
        f4 = fG;

    }
    else if ( fB > fG && fG >= fR )
    {
        // T3
        p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
        f1 = FIX_ONE_33POINT-fB;
        f2 = fB-fG;
        f3 = fG-fR;
        f4 = fR;
    }
    else if ( fR >= fG && fG > fB )
    {
        // T4
        p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
        p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
        f1 = FIX_ONE_33POINT-fR;
        f2 = fR-fG;
        f3 = fG-fB;
        f4 = fB;
    }
    else if ( fG > fR && fR >=fB )
    {
        // T5
        p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
        p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
        f1 = FIX_ONE_33POINT-fG;
        f2 = fG-fR;
        f3 = fR-fB;
        f4 = fB;

    }
    else
    {
        // T6
        p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
        f1 = FIX_ONE_33POINT-fR;
        f2 = fR-fB;
        f3 = fB-fG;
        f4 = fG;
    }

    uint32_t R,G,B;
    R = (f1*RX(p1) + f2*RX(p2) + f3*RX(p3) + f4*RX(p4)  +16 )>>5;
    G = (f1*GX(p1) + f2*GX(p2) + f3*GX(p3) + f4*GX(p4)  + 16)>>5;
    B = (f1*BX(p1) + f2*BX(p2) + f3*BX(p3) + f4*BX(p4) +16)>>5;


    returnValue = ( (B<<20) + (G<<10) + R);

    return returnValue;
}
void Tetrahedral16bit(int ri,int gi,int bi,int &ro,int &go,int &bo,unsigned long long int * pLut3D)
{
    int nR = ri>>11;
    int fR = ri-(nR<<11);
    int nG = gi>>11;
    int fG = gi-(nG<<11);
    int nB = bi>>11;
    int fB = bi-(nB<<11);

    int FIX_ONE_33POINT_16BIT=2048;


    unsigned long long int p1,p2,p3,p4;
    p1 = *(pLut3D + (nB*33*33)+(nG*33)+nR);
    p4 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR+1));
    int f1,f2,f3,f4;
    if ( fG >= fB && fB >= fR )
    {
        // T1
        p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
        f1 = FIX_ONE_33POINT_16BIT-fG;
        f2 = fG-fB;
        f3 = fB-fR;
        f4 = fR;

    }
    else if ( fB > fR && fR > fG)
    {
        // T2
        p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
        f1 = FIX_ONE_33POINT_16BIT-fB;
        f2 = fB-fR;
        f3 = fR-fG;
        f4 = fG;

    }
    else if ( fB > fG && fG >= fR )
    {
        // T3
        p2 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG+1)*33)+(nR));
        f1 = FIX_ONE_33POINT_16BIT-fB;
        f2 = fB-fG;
        f3 = fG-fR;
        f4 = fR;
    }
    else if ( fR >= fG && fG > fB )
    {
        // T4
        p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
        p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
        f1 = FIX_ONE_33POINT_16BIT-fR;
        f2 = fR-fG;
        f3 = fG-fB;
        f4 = fB;
    }
    else if ( fG > fR && fR >=fB )
    {
        // T5
        p2 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR));
        p3 = *(pLut3D + ((nB)*33*33)+((nG+1)*33)+(nR+1));
        f1 = FIX_ONE_33POINT_16BIT-fG;
        f2 = fG-fR;
        f3 = fR-fB;
        f4 = fB;

    }
    else
    {
        // T6
        p2 = *(pLut3D + ((nB)*33*33)+((nG)*33)+(nR+1));
        p3 = *(pLut3D + ((nB+1)*33*33)+((nG)*33)+(nR+1));
        f1 = FIX_ONE_33POINT_16BIT-fR;
        f2 = fR-fB;
        f3 = fB-fG;
        f4 = fG;
    }

    ro = (f1*(p1&0xffff) + f2*(p2&0xffff) + f3*(p3&0xffff) + f4*(p4&0xffff)+1024)>>11;
    go = (f1*((p1>>16)&0xffff) + f2*((p2>>16)&0xffff) + f3*((p3>>16)&0xffff) + f4*((p4>>16)&0xffff)+1024)>>11;
    bo = (f1*((p1>>32)&0xffff) + f2*((p2>>32)&0xffff) + f3*((p3>>32)&0xffff) + f4*((p4>>32)&0xffff)+1024)>>11;

}

