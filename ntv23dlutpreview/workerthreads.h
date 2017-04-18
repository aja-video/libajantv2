#ifndef CWORKERTHREADS_H
#define CWORKERTHREADS_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include "dialog.h"
#include <cloglpreviewwidget.h>

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

#define NEW_AUTO_CIRCULATE_API


#include "ntv2enums.h"
#include "ntv2card.h"
#include "ntv2boardscan.h"
#include "ntv2utils.h"
#include "cavcircularbuffer.h"
#include "cloglpreview.h"

class CaptureWorker;
class CLOGLPreviewWorker;
class CatchEncodedFramesWorker;

class CJ2KSequenceWriterWorker;

class CAVCircularBuffer;
class Dialog;



class CWorker : public QObject
{
    Q_OBJECT
public:
    explicit CWorker(Dialog *d);
    ~CWorker();

    void prepareExit() { mExiting = true; }
    void setPixelFormat(NTV2FrameBufferFormat pixelFormat);
    void setChannel(NTV2Channel channel);
    void setCardIndex(int cardIndex);
    void setOpenGLPreviewWidget(    COpenGLPreviewWidget* openGLPreviewWidget) { mOpenGLPreviewWidget = openGLPreviewWidget;}
    void grabTIFF();

signals:
    void finished();
    void updateInputVideoFormat(QString videoFormat);


public slots:
    void doWork();


private:
    void shutDown();
    void createPipelineThreads();
    void startThreads();
    void killPipelineThreads();
    void createCircularBuffers();
    void deleteCircularBuffers();
    bool checkForInputChanged();
    void setupBoard();
    void connectSignalsAndSlots();
    void disconnectSignalsAndSlots();

    Dialog* mDialog;
    COpenGLPreviewWidget* mOGLPreviewWidget;

    bool mInited;
    bool mExiting;
    bool mRestart;
    int  mBoardIndex;
    NTV2Channel mInputChannel;


    uint32_t mWidth;
    uint32_t mHeight;

    CaptureWorker* mCaptureWorker;
    QThread* mCaptureWorkerThread;

    CLOGLPreviewWorker* mCLOGLPreviewWorker;
    QThread* mCLOGLPreviewWorkerThread;

    NTV2VideoFormat mInputVideoFormat;
    NTV2FrameBufferFormat mPixelFormat;

    CNTV2Card* mNTV2Card;
    NTV2EveryFrameTaskMode		mSavedTaskMode;			///< @brief	Used to restore the previous task mode
    CAVCircularBuffer* mCaptureCircularBuffer;
    CAVCircularBuffer* mPostEncodeCircularBuffer;



    COpenGLPreviewWidget* mOpenGLPreviewWidget;

};

class CaptureWorker : public QObject
{
    Q_OBJECT
public:
    explicit CaptureWorker();
    ~CaptureWorker();

     void prepareExit() { mExiting = true;  }

    bool setupAutoCirculate();

    uint32_t getWidth() { return mWidth; }
    uint32_t getHeight() { return mHeight; }

    void setCircularBuffer(CAVCircularBuffer* captureCircularBuffer) { mCaptureCircularBuffer = captureCircularBuffer; }
    void setCardObject(CNTV2Card* ntv2Card) { mNTV2Card = ntv2Card; }
    void setPixelFormat(NTV2FrameBufferFormat pixelFormat) { mPixelFormat = pixelFormat;}
    void setInputChannel(NTV2Channel channel) { mInputChannel = channel; }
    void setOpenGLPreviewWidget(    COpenGLPreviewWidget* openGLPreviewWidget) { mOpenGLPreviewWidget = openGLPreviewWidget;}

signals:
    void finished();
    void numFramesDropped(int frameDropped);
    void numFramesCaptured(int framesCaptured);


public slots:
    void capture();
    void grabTIFFFile();

//    void setDestination(int dest);

private:

    virtual bool    testOpenCL();
    void writeTIFFFile(uint32_t* videoFrame);

    bool mInited;
    bool mExiting;
    bool doGrab;

    NTV2FrameBufferFormat mPixelFormat;
    uint32_t mWidth;
    uint32_t mHeight;


    CNTV2Card* mNTV2Card;
     CAVCircularBuffer* mCaptureCircularBuffer;
    NTV2Channel mInputChannel;
    AUTOCIRCULATE_TRANSFER	mInputTransfer;

    COpenGLPreviewWidget* mOpenGLPreviewWidget;

    cl::Device* mDefaultDevice;
    cl::Context* mContextCL;
    cl::CommandQueue* mQueue;

    cl::Program* mOpenCL_Kernel;

//    cl::Program* mRGB10_3DLUTKernel;
};




#endif // CWORKERTHREADS_H
