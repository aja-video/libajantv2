#include "workerthreads.h"
#include "dialog.h"

#include "ajabase/common/testpatterngen.h"
#include "ajabase/common/timer.h"
#include "ajabase/system/process.h"
#include "ntv2utils.h"

#include <QString>
#include <QFile>
#include <QElapsedTimer>
#include <QDateTime>
#include <QDebug>

#include "tiffio.h"

typedef uint32_t RGBCoefsInt;
typedef struct {
    uint16_t rCoef;
    uint16_t gCoef;
    uint16_t bCoef;
    uint16_t aCoef;
} RGBA16BitCoefs;
typedef struct {
    uint16_t rCoef;
    uint16_t gCoef;
    uint16_t bCoef;
} RGB16BitCoefs;

extern RGBCoefsInt lut3DInts[33][33][33];
extern RGBA16BitCoefs lut3DRGBA16Ints[33][33][33];
extern RGB16BitCoefs lut3DRGB16Ints[33][33][33];
static AJA_PixelFormat GetAJAPixelFormat (const NTV2FrameBufferFormat format);
static bool get4KInputFormat (NTV2VideoFormat & videoFormat);

#define NUMINPUTBUFFERS 5
static const ULWord	kAppSignature	AJA_FOURCC ('D','E','M','O');
//
//	This array maps NTV2Channels to an NTV2InputSource...
//
static const NTV2InputSource	gInputChannelNumberToInputSource [] =
{
    NTV2_INPUTSOURCE_SDI1,
    NTV2_INPUTSOURCE_SDI2,
    NTV2_INPUTSOURCE_SDI3,
    NTV2_INPUTSOURCE_SDI4,
    NTV2_INPUTSOURCE_SDI5,
    NTV2_INPUTSOURCE_SDI6,
    NTV2_INPUTSOURCE_SDI7,
    NTV2_INPUTSOURCE_SDI8,

};

CWorker::CWorker(Dialog *d)
    : mInited(false),
      mDialog(d),
      mBoardIndex(0),
      mInputChannel(NTV2_CHANNEL1),
      mInputVideoFormat(NTV2_FORMAT_UNKNOWN),
      mPixelFormat(NTV2_FBF_ABGR),
      mNTV2Card(NULL),
      mCaptureWorker(NULL),
      mCaptureWorkerThread(NULL),
      mCLOGLPreviewWorker(NULL),
      mCLOGLPreviewWorkerThread(NULL),
      mCaptureCircularBuffer(NULL),
      mPostEncodeCircularBuffer(NULL),
      mExiting(false)
{

}

CWorker::~CWorker()
{

    shutDown();

}

void CWorker::doWork()
{
    mRestart = true;

    while (!mExiting)
    {
        if ( mRestart)
        {
            qDebug() << "restart";

            shutDown();

            createPipelineThreads();
            setupBoard();
            createCircularBuffers();
            connectSignalsAndSlots();
            startThreads();
            updateInputVideoFormat(NTV2VideoFormatToString(mInputVideoFormat).c_str());

            qDebug() << "restarted";

            mRestart = false;

        }
        else
        {
            // Check for Change in Input
            if ( checkForInputChanged() )
            {
                qDebug() << "Input Changed";
                mRestart = true;
            }

            QThread::msleep(100);

        }
    }

    emit finished();
}

void CWorker::connectSignalsAndSlots()
{
    if (mCLOGLPreviewWorker )
    {

        connect(this,SIGNAL(updateInputVideoFormat(QString)),mDialog->inputVideoFormatLabel,SLOT(setText(QString)));
        connect(mCLOGLPreviewWorker,SIGNAL(updatePreview(int,int,int,void*,int)),mDialog->openGLWidget,SLOT(updateTexture(int,int,int,void*,int)));
        connect(mCaptureWorker,SIGNAL(numFramesDropped(int)),mDialog,SLOT(updateFramesDropped(int)));
        connect(mCaptureWorker,SIGNAL(numFramesCaptured(int)),mDialog,SLOT(updateFramesCaptured(int)));

    }


}
void CWorker::disconnectSignalsAndSlots()
{
    if (mCLOGLPreviewWorker )
    {
        disconnect(this,SIGNAL(updateInputVideoFormat(QString)),mDialog->inputVideoFormatLabel,SLOT(setText(QString)));
        disconnect(mCLOGLPreviewWorker,SIGNAL(updatePreview(int,int,int,void*,int)),mDialog->openGLWidget,SLOT(updateTexture(int,int,int,void*,int)));
        disconnect(mCaptureWorker,SIGNAL(numFramesDropped(int)),mDialog,SLOT(updateFramesDropped(int)));
        disconnect(mCaptureWorker,SIGNAL(numFramesCaptured(int)),mDialog,SLOT(updateFramesCaptured(int)));
    }

}

void CWorker::shutDown()
{
    disconnectSignalsAndSlots();
    killPipelineThreads();
    if ( mCaptureCircularBuffer)
    {
        delete mCaptureCircularBuffer;
        mCaptureCircularBuffer = NULL;
    }
    if ( mNTV2Card)
    {
        mNTV2Card->ReleaseStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ()));

        mNTV2Card->SetEveryFrameServices (mSavedTaskMode);
        mNTV2Card->Close();
        delete mNTV2Card;
        mNTV2Card = NULL;
    }

}

void CWorker::createPipelineThreads()
{
    mCaptureWorkerThread = new QThread;
    mCaptureWorker = new CaptureWorker();
    mCaptureWorker->moveToThread(mCaptureWorkerThread);
    connect(mCaptureWorkerThread, &QThread::finished, mCaptureWorker, &QObject::deleteLater);
    connect(mCaptureWorker, SIGNAL(finished()), mCaptureWorker, SLOT(deleteLater()));
    connect(mCaptureWorkerThread, SIGNAL(started()), mCaptureWorker, SLOT(capture()));

    mCaptureWorker->setInputChannel(mInputChannel);

    mCLOGLPreviewWorkerThread = new QThread;
    mCLOGLPreviewWorker = new CLOGLPreviewWorker();
    mCLOGLPreviewWorker->moveToThread(mCLOGLPreviewWorkerThread);
    connect(mCLOGLPreviewWorkerThread, &QThread::finished, mCLOGLPreviewWorker, &QObject::deleteLater);
    connect(mCLOGLPreviewWorker, SIGNAL(finished()), mCLOGLPreviewWorker, SLOT(deleteLater()));
    connect(mCLOGLPreviewWorkerThread, SIGNAL(started()), mCLOGLPreviewWorker, SLOT(preview()));

    mCaptureWorker->setOpenGLPreviewWidget(mOpenGLPreviewWidget);



}

void CWorker::startThreads()
{
    if ( mCaptureWorkerThread )
        mCaptureWorkerThread->start();
    if ( mCLOGLPreviewWorkerThread )
        mCLOGLPreviewWorkerThread->start();

}





void CWorker::killPipelineThreads()
{
    if  ( mCaptureCircularBuffer )
        mCaptureCircularBuffer->Abort();


    if ( mCaptureWorker )
    {
        mCaptureWorker->prepareExit();
    }
    if ( mCLOGLPreviewWorker )
    {
        mCLOGLPreviewWorker->prepareExit();
    }

    if ( mCaptureWorker )
    {

        mCaptureWorkerThread->quit();
        mCaptureWorkerThread->wait();
        delete mCaptureWorkerThread;
        mCaptureWorkerThread = NULL;
        mCaptureWorker = NULL;

    }


    if ( mCLOGLPreviewWorker )
    {

        mCLOGLPreviewWorkerThread->quit();
        mCLOGLPreviewWorkerThread->wait();
        delete mCLOGLPreviewWorkerThread;
        mCLOGLPreviewWorkerThread = NULL;
        mCLOGLPreviewWorker = NULL;

    }

}



void  CWorker::setPixelFormat(NTV2FrameBufferFormat pixelFormat)
{
    if ( pixelFormat != mPixelFormat)
    {
        mPixelFormat = pixelFormat;
        mRestart = true;
    }
}

void CWorker::setChannel(NTV2Channel channel)
{
    if ( channel != mInputChannel)
    {
        mInputChannel = channel;
        mRestart = true;
    }

}

void  CWorker::setCardIndex(int cardIndex)
{

    if ( cardIndex != mBoardIndex)
    {
        mBoardIndex = cardIndex;
        mRestart = true;
    }
}



void CWorker::createCircularBuffers()
{
    AJA_PixelFormat ajaPixelFormat = GetAJAPixelFormat(mPixelFormat);
    uint32_t rowBytes = AJA_CalcRowBytesForFormat(ajaPixelFormat,mWidth);
    uint32_t frameSize = mHeight*rowBytes;

    mCaptureCircularBuffer = new CAVCircularBuffer();
    mCaptureCircularBuffer->Allocate(NUMINPUTBUFFERS, ajaPixelFormat,frameSize , mWidth, mHeight, true, 4096);
    mCaptureWorker->setCircularBuffer(mCaptureCircularBuffer);
    mCLOGLPreviewWorker->setPreviewCircularBuffer(mCaptureCircularBuffer);


}

void CWorker::deleteCircularBuffers()
{

}


void CWorker::grabTIFF()
{
    qDebug() <<  "grabbing TIFF";
    mCaptureWorker->grabTIFFFile();

}

bool CWorker::checkForInputChanged()
{
    if ( mNTV2Card == NULL )
        return false;

    bool bChanged = false;
    NTV2VideoFormat vf = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1,false);

    if (NTV2_IS_QUAD_FRAME_FORMAT(mInputVideoFormat)  )
    {
        NTV2VideoFormat vf2 = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI2,true);
        NTV2VideoFormat vf3 = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI3, true);
        NTV2VideoFormat vf4 = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI4, true);

        if ((vf == vf2) && (vf == vf3) && (vf == vf4))
        {
            NTV2VideoFormat fourKFormat = vf;
            if (get4KInputFormat(fourKFormat))
                if ( fourKFormat != mInputVideoFormat)
                    bChanged = true;
        }
        else
            bChanged = true;

    }
    else
    {
        if ( vf != mInputVideoFormat)
            bChanged =  true;
    }


    return bChanged;
}

void CWorker::setupBoard()
{
    if ( mNTV2Card)
    {
        mNTV2Card->Close();
        delete mNTV2Card;
        mNTV2Card = NULL;
    }
    mNTV2Card = new CNTV2Card(mBoardIndex);
    if ( mNTV2Card->IsDeviceReady() == false)
    {
        delete mNTV2Card;
        mNTV2Card = NULL;

    }

    if ( mNTV2Card)
    {
        if (!mNTV2Card->AcquireStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ())))
        {
            qDebug() << "couldn't acquire board";
            return;
        }

        mNTV2Card->GetEveryFrameServices (&mSavedTaskMode);	//	Save the current state before we change it
        mNTV2Card->SetEveryFrameServices (NTV2_OEM_TASKS);	//	Since this is an OEM demo we will set the OEM service level

        mNTV2Card->SetMultiFormatMode(false);
        mNTV2Card->SetReference(NTV2_REFERENCE_INPUT1);

        //	Not multiformat:  Route all possible SDI outputs to CSC video output (RGB) or FrameStore output (YUV)...
        mNTV2Card->ClearRouting ();

        if (mNTV2Card->GetDeviceID() == DEVICE_ID_CORVID88)
        {
            // Set up for possible 4k capture and playback
            mCLOGLPreviewWorker->setOutputChannel(NTV2_CHANNEL5);
            mNTV2Card->EnableChannel(NTV2_CHANNEL1);
            mNTV2Card->EnableChannel(NTV2_CHANNEL2);
            mNTV2Card->EnableChannel(NTV2_CHANNEL3);
            mNTV2Card->EnableChannel(NTV2_CHANNEL4);
            mNTV2Card->EnableChannel(NTV2_CHANNEL5);
            mNTV2Card->EnableChannel(NTV2_CHANNEL6);
            mNTV2Card->EnableChannel(NTV2_CHANNEL7);
            mNTV2Card->EnableChannel(NTV2_CHANNEL8);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL1, false);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL2, false);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL3, false);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL4, false);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL5, true);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL6, true);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL7, true);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL8, true);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL1, mPixelFormat);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL2, mPixelFormat);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL3, mPixelFormat);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL4, mPixelFormat);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL5, mPixelFormat);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL6, mPixelFormat);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL7, mPixelFormat);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL8, mPixelFormat);
            mNTV2Card->SetMode(NTV2_CHANNEL1,NTV2_MODE_CAPTURE);
            mNTV2Card->SetMode(NTV2_CHANNEL2,NTV2_MODE_CAPTURE);
            mNTV2Card->SetMode(NTV2_CHANNEL3,NTV2_MODE_CAPTURE);
            mNTV2Card->SetMode(NTV2_CHANNEL4,NTV2_MODE_CAPTURE);
            mNTV2Card->SetMode(NTV2_CHANNEL5,NTV2_MODE_DISPLAY);
            mNTV2Card->SetMode(NTV2_CHANNEL6,NTV2_MODE_DISPLAY);
            mNTV2Card->SetMode(NTV2_CHANNEL7,NTV2_MODE_DISPLAY);
            mNTV2Card->SetMode(NTV2_CHANNEL8,NTV2_MODE_DISPLAY);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL1);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL2);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL3);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL4);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL5);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL6);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL7);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL8);

            for ( int i=0; i<10; i++)
                mNTV2Card->WaitForOutputVerticalInterrupt();

            mInputVideoFormat = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1,true);
            NTV2VideoFormat vf2 = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI2,true);
            NTV2VideoFormat vf3 = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI3, true);
            NTV2VideoFormat vf4 = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI4, true);

            if ((mInputVideoFormat == vf2) && (mInputVideoFormat == vf3) && (mInputVideoFormat == vf4))
            {
                NTV2VideoFormat fourKFormat = mInputVideoFormat;
                if (get4KInputFormat(fourKFormat))
                    mInputVideoFormat = fourKFormat;

            }

            // Setup Routing:
            if ( NTV2_IS_FBF_RGB(mPixelFormat))
            {
                // Input
                mNTV2Card->Connect (NTV2_XptCSC1VidInput,NTV2_XptSDIIn1);
                mNTV2Card->Connect (NTV2_XptFrameBuffer1Input,NTV2_XptCSC1VidRGB);
                mNTV2Card->Connect (NTV2_XptCSC2VidInput,NTV2_XptSDIIn2);
                mNTV2Card->Connect (NTV2_XptFrameBuffer2Input,NTV2_XptCSC2VidRGB);
                mNTV2Card->Connect (NTV2_XptCSC3VidInput,NTV2_XptSDIIn3);
                mNTV2Card->Connect (NTV2_XptFrameBuffer3Input,NTV2_XptCSC3VidRGB);
                mNTV2Card->Connect (NTV2_XptCSC4VidInput,NTV2_XptSDIIn4);
                mNTV2Card->Connect (NTV2_XptFrameBuffer4Input,NTV2_XptCSC4VidRGB);


                // Output
                mNTV2Card->Connect (NTV2_XptDualLinkOut5Input,	NTV2_XptFrameBuffer5RGB);
                mNTV2Card->Connect (NTV2_XptDualLinkOut6Input,	NTV2_XptFrameBuffer6RGB);
                mNTV2Card->Connect (NTV2_XptDualLinkOut7Input,	NTV2_XptFrameBuffer7RGB);
                mNTV2Card->Connect (NTV2_XptDualLinkOut8Input,	NTV2_XptFrameBuffer8RGB);
                mNTV2Card->Connect (NTV2_XptSDIOut5Input,		NTV2_XptDuallinkOut5);
                mNTV2Card->Connect(NTV2_XptSDIOut5InputDS2,	NTV2_XptDuallinkOut5DS2);
                mNTV2Card->Connect (NTV2_XptSDIOut6Input,		NTV2_XptDuallinkOut6);
                mNTV2Card->Connect(NTV2_XptSDIOut6InputDS2,	NTV2_XptDuallinkOut6DS2);
                mNTV2Card->Connect (NTV2_XptSDIOut7Input,		NTV2_XptDuallinkOut7);
                mNTV2Card->Connect (NTV2_XptSDIOut7InputDS2,	NTV2_XptDuallinkOut7DS2);
                mNTV2Card->Connect (NTV2_XptSDIOut8Input,		NTV2_XptDuallinkOut8);
                mNTV2Card->Connect(NTV2_XptSDIOut8InputDS2,	NTV2_XptDuallinkOut8DS2);


            }
            else
            {
                mNTV2Card->Connect (::GetFrameBufferInputXptFromChannel (NTV2_CHANNEL1), ::GetSDIInputOutputXptFromChannel (NTV2_CHANNEL1));

            }

            if (mInputVideoFormat == NTV2_FORMAT_UNKNOWN)
                mInputVideoFormat = NTV2_FORMAT_720p_5994;

            mNTV2Card->SetVideoFormat(mInputVideoFormat,false,false,NTV2_CHANNEL1);
            mNTV2Card->SetVideoFormat(mInputVideoFormat,false,false,NTV2_CHANNEL5);

        }
        else
        {
            mCLOGLPreviewWorker->setOutputChannel(NTV2_CHANNEL3);
            mNTV2Card->EnableChannel(NTV2_CHANNEL1);
            mNTV2Card->EnableChannel(NTV2_CHANNEL3);

            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL1, false);
            mNTV2Card->SetSDITransmitEnable(NTV2_CHANNEL3, true);
            mNTV2Card->SetMode(NTV2_CHANNEL1,NTV2_MODE_CAPTURE);
            mNTV2Card->SetMode(NTV2_CHANNEL3,NTV2_MODE_DISPLAY);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL1);
            mNTV2Card->SetColorSpaceRGBBlackRange(NTV2_RGBBLACKRANGE_0_0x3FF,NTV2_CHANNEL3);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL1, mPixelFormat);
            mNTV2Card->SetFrameBufferFormat(NTV2_CHANNEL3, mPixelFormat);

            for ( int i=0; i<10; i++)
                mNTV2Card->WaitForOutputVerticalInterrupt();

            mInputVideoFormat = mNTV2Card->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1,true);

            // Setup Routing:
            if ( NTV2_IS_FBF_RGB(mPixelFormat))
            {
                mNTV2Card->Connect (::GetCSCInputXptFromChannel (NTV2_CHANNEL1), ::GetSDIInputOutputXptFromChannel (NTV2_CHANNEL1));
                mNTV2Card->Connect (::GetFrameBufferInputXptFromChannel (NTV2_CHANNEL1), ::GetCSCOutputXptFromChannel (NTV2_CHANNEL1, false/*isKey*/, true/*isRGB*/));

                mNTV2Card->Connect (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer3RGB);
                mNTV2Card->Connect (NTV2_XptSDIOut3Input,		NTV2_XptDuallinkOut3);
                mNTV2Card->Connect(NTV2_XptSDIOut3InputDS2,	NTV2_XptDuallinkOut3DS2);
            }
            else
            {
                mNTV2Card->Connect (::GetFrameBufferInputXptFromChannel (NTV2_CHANNEL1), ::GetSDIInputOutputXptFromChannel (NTV2_CHANNEL1));

            }
            if (mInputVideoFormat == NTV2_FORMAT_UNKNOWN)
                mInputVideoFormat = NTV2_FORMAT_720p_5994;
                mNTV2Card->SetVideoFormat(mInputVideoFormat,false,false,NTV2_CHANNEL1);
                mNTV2Card->SetVideoFormat(mInputVideoFormat,false,false,NTV2_CHANNEL3);

        }

         NTV2FrameDimensions frameDimensions = mNTV2Card->GetActiveFrameDimensions(mInputChannel);
        mWidth = frameDimensions.GetWidth();
        mHeight = frameDimensions.GetHeight();
    }
    else
    {
        mWidth = 1280;
        mHeight = 720;
        mInputVideoFormat = NTV2_FORMAT_720p_5994;

    }

    mCaptureWorker->setCardObject(mNTV2Card);
    mCaptureWorker->setPixelFormat(mPixelFormat);
    mCLOGLPreviewWorker->setCardObject(mNTV2Card);
    mCLOGLPreviewWorker->setWidth(mWidth);
    mCLOGLPreviewWorker->setHeight(mHeight);
    AJA_PixelFormat ajapf = GetAJAPixelFormat(mPixelFormat);
    mCLOGLPreviewWorker->setPixelFormat(ajapf);
}



CaptureWorker::CaptureWorker()
    : mInited(false),
      mWidth(0),
      mHeight(0),
      mPixelFormat(NTV2_FBF_10BIT_RGB),
      mNTV2Card(NULL),
      mInputChannel(NTV2_CHANNEL1),
      mCaptureCircularBuffer(NULL),
      doGrab(false),
      mExiting(false)
{

}

CaptureWorker::~CaptureWorker()
{
    if (mNTV2Card)
    {
        mNTV2Card->SetMode(mInputChannel,NTV2_MODE_DISPLAY);
        mNTV2Card->AutoCirculateStop (mInputChannel);
    }
}
void CaptureWorker::grabTIFFFile()
{
    doGrab = true;
}


void CaptureWorker::capture()
{
    //
    assert(mCaptureCircularBuffer != NULL);

#if 1
    assert(mNTV2Card != NULL);
    setupAutoCirculate();

    mNTV2Card->SubscribeInputVerticalEvent(mInputChannel);
    mNTV2Card->AutoCirculateStart (mInputChannel);
    NTV2FrameDimensions frameDimensions;
    mNTV2Card->GetActiveFrameDimensions(frameDimensions);
    mWidth = frameDimensions.GetWidth();
    mHeight = frameDimensions.GetHeight();
    uint32_t rowBytes = AJA_CalcRowBytesForFormat(GetAJAPixelFormat(mPixelFormat), mWidth);

    while ( mOpenGLPreviewWidget->isReady() == false)
    {
        mNTV2Card->WaitForOutputVerticalInterrupt();
        qDebug() << "Capture Starting";
    }
    qDebug() << "Capture Started";

    mDefaultDevice = mOpenGLPreviewWidget->getDefaultDevice();
    mContextCL = mOpenGLPreviewWidget->getContextCL();
    mQueue = mOpenGLPreviewWidget->getOpenCLQueue();
    const char* kernelName;
    uint32_t lutSize;
    void* lutData;
    if ( mPixelFormat == NTV2_FBF_10BIT_RGB )
    {
        mOpenCL_Kernel = mOpenGLPreviewWidget->getRGB10_3DLUT_10Bit_Kernel();
        kernelName = "RGB10_3DLUT_10BIT";
        lutSize = 33*33*33*4;
        lutData = lut3DInts;

    }
    else
    {
        mOpenCL_Kernel = mOpenGLPreviewWidget->getRGB48_3DLUT_16Bit_Kernel();
        kernelName = "RGB48_3DLUT_16BIT";
        lutSize = 33*33*33*6;
        lutData = lut3DRGB16Ints;
    }

//
    cl_int result = CL_SUCCESS;
    cl::Buffer*  imageBuffer;
    cl::Buffer*  lutBuffer;
    uint8_t* inputBuffer;

    imageBuffer = new cl::Buffer(*mContextCL,CL_MEM_READ_WRITE,rowBytes*mHeight);
    lutBuffer   = new cl::Buffer(*mContextCL,CL_MEM_READ_WRITE,lutSize);
    inputBuffer = new uint8_t[rowBytes*mHeight];

    mInited = true;
    bool bypass = false;
    while (!mExiting)
    {
        AUTOCIRCULATE_STATUS	acStatus;
        mNTV2Card->AutoCirculateGetStatus (mInputChannel, acStatus);
        if (acStatus.acState == NTV2_AUTOCIRCULATE_RUNNING && acStatus.acBufferLevel > 1)
        {

            AJAAVBuffer* frameData = mCaptureCircularBuffer->StartProduceNextBuffer();
            if ( frameData == NULL )
                continue;
            AJATimer timer;
            timer.Start();

            mInputTransfer.SetBuffers ((ULWord*)frameData->videoBuffer, rowBytes*mHeight,
                                       NULL, 0,
                                       NULL, 0);

            mNTV2Card->AutoCirculateTransfer (mInputChannel, mInputTransfer);

//            AJATestPatternBuffer tpBuffer;
//            AJATestPatternGen tp;

//            tp.DrawTestPattern((AJATestPatternSelect)0, mWidth, mHeight, AJA_PixelFormat_RGB10, tpBuffer);
//            memcpy(inBuffer,tpBuffer.data(),rowBytes*mHeight);

            if ( bypass )
            {
                ;// memcpy(frameData->videoBuffer,inputBuffer,rowBytes*mHeight);
            }
            else
            {
                cl_int res = CL_SUCCESS;
                res = mQueue->enqueueWriteBuffer(*imageBuffer,CL_TRUE,0,rowBytes*mHeight,frameData->videoBuffer);
                if ( res != CL_SUCCESS )
                    qDebug() << "enqueueWriteBuffer inputBuffer failed";

                res = mQueue->enqueueWriteBuffer(*lutBuffer,CL_TRUE,0,lutSize,lutData);
                if ( res != CL_SUCCESS )
                    qDebug() << "enqueueWriteBuffer lutData failed";

                cl::Kernel currentKernel;
                currentKernel = cl::Kernel(*mOpenCL_Kernel, kernelName);
                currentKernel.setArg(0, *imageBuffer);
                currentKernel.setArg(1, *lutBuffer);
                result = mQueue->enqueueNDRangeKernel(currentKernel, cl::NullRange,cl::NDRange(mHeight*mWidth),cl::NullRange);
                assert(result == CL_SUCCESS);
                mQueue->enqueueReadBuffer(*imageBuffer,CL_TRUE,0,rowBytes*mHeight,frameData->videoBuffer);
                mQueue->finish();
            }
            if (doGrab)
            {
                writeTIFFFile((uint32_t*)frameData->videoBuffer);
                doGrab = false;
            }

            emit numFramesDropped(mInputTransfer.acTransferStatus.acFramesDropped);
            emit numFramesCaptured(mInputTransfer.acTransferStatus.acFramesProcessed);

            frameData->audioRecordSize = 0;
 //           qDebug("   Producer: %d",timer.ElapsedTime());
            timer.Stop();
            mCaptureCircularBuffer->EndProduceNextBuffer();

        }
        else
        {
            mNTV2Card->WaitForInputVerticalInterrupt (mInputChannel);
        }
    }

#else
    mWidth = 1280;
    mHeight = 720;
    while (!mExiting)
    {

        AJAAVBuffer* frameData = mCaptureCircularBuffer->StartProduceNextBuffer();

        QThread::msleep(1000);

        AJATestPatternBuffer tpBuffer;
        AJATestPatternGen tp;

        tp.DrawTestPattern((AJATestPatternSelect)tpNum, mWidth, mHeight, GetAJAPixelFormat( mPixelFormat), tpBuffer);
        if ( ++tpNum == AJA_TestPatt_All)
            tpNum = 0;

        uint8_t* buffer = (uint8_t*)frameData->videoBuffer;
        memcpy(buffer,tpBuffer.data(),tpBuffer.size());

        updatePreview((int)mPixelFormat,mWidth,mHeight,frameData->videoBuffer,frameData->videoBufferSize);

        mCaptureCircularBuffer->EndProduceNextBuffer();
    }
#endif
    qDebug() << "capture exiting";

    emit finished();
}


// write a 16 bit tiff RGB file.
void CaptureWorker::writeTIFFFile(uint32_t* videoFrame)
{
//    QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save File"),
//                               ".",
//                               tr("TIFF Files (*.tif)"));

//    if ( fileName.length() == 0 ) return;
    QDateTime t = QDateTime::currentDateTime ();
    QString s = t.toString("yy.MM.dd.hh.mm.ss.zzz");
    s += ".tiff";
    qDebug() << s;

    TIFF *tif= TIFFOpen(s.toLocal8Bit(), "w");
    if ( tif == NULL  ) return;

    uint32_t sampleperpixel = 3;
    TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, mWidth);  // set the width of the image
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, mHeight);    // set the height of the image
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);   // set number of channels per pixel
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);    // set the size of the channels
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image.
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    tsize_t linebytes = sampleperpixel * 2 * mWidth;

    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
      unsigned char *buf = NULL;
    //    Allocating memory to store the pixels of current row
    if (TIFFScanlineSize(tif) == linebytes)
        buf =(unsigned char *)_TIFFmalloc(linebytes);
    else
        buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(tif));


    uint32_t* rgbBuffer = ( uint32_t*)videoFrame;
    for (uint32_t  lineNumber=0; lineNumber<mHeight; lineNumber++)
    {
        uint32_t* rgbLineBuffer = (uint32_t*)(rgbBuffer+(lineNumber*mWidth));
        uint16_t* tiffLinePtr = (uint16_t*)buf;
        for ( uint32_t pixelNumber = 0; pixelNumber<mWidth; pixelNumber++ )
        {
            //            uint16_t r = *rgbLineBuffer++;
            //            uint16_t g = *rgbLineBuffer++;
            //            uint16_t b = *rgbLineBuffer++;
            uint32_t value = *rgbLineBuffer;
            uint16_t r = (value&0x3FF)<<6;
            uint16_t g = ((value>>10)&0x3FF)<<6;
            uint16_t b = ((value>>20)&0x3FF)<<6;
            *tiffLinePtr++ = r;
            *tiffLinePtr++ = g;
            *tiffLinePtr++ = b;
            rgbLineBuffer++;

        }

        if (TIFFWriteScanline(tif, buf, lineNumber, 0) < 0)
            break;
    }
    TIFFClose(tif);
}

bool CaptureWorker::setupAutoCirculate()
{
    if (mNTV2Card)
    {
//        mNTV2Card->SetNumberAudioChannels (5, (NTV2AudioSystem)mInputChannel);
        mNTV2Card->AutoCirculateStop (mInputChannel);
        mNTV2Card->AutoCirculateInitForInput (mInputChannel,NUMINPUTBUFFERS);
        mNTV2Card->GetFrameBufferFormat(mInputChannel,&mPixelFormat);
        mInputTransfer.acFrameBufferFormat = mPixelFormat;

        return true;
    }
    else
    {
        return false;
    }
}


bool CaptureWorker::testOpenCL()
{
    cl::Program::Sources sources;

    // kernel calculates for each element C=A+B
    QFile f(":gpufiles/add.cl");
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QByteArray ba = f.readAll();

    std::pair<const char*, ::size_t> code(ba.data(),ba.size());
    sources.push_back(code);

    cl::Program program(*mContextCL,sources);
    VECTOR_CLASS<cl::Device> vectorDevice;
    vectorDevice.push_back(*mDefaultDevice);
    if(program.build(vectorDevice)!=CL_SUCCESS){
        std::string errString = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*mDefaultDevice);
        qDebug() <<" Error building: " << errString.c_str() <<"\n";
        return false;
    }

    // create buffers on the device
    cl::Buffer buffer_A(*mContextCL,CL_MEM_READ_WRITE,sizeof(int)*10);
    cl::Buffer buffer_B(*mContextCL,CL_MEM_READ_WRITE,sizeof(int)*10);
    cl::Buffer buffer_C(*mContextCL,CL_MEM_READ_WRITE,sizeof(int)*10);

    int A[]	= { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int B[]	= { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0};
    int CS[]	= { 0, 2, 4, 3, 5, 7, 6, 8,10, 9};	// checksum solution
    int C[]	= {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};	// checksum test init

    //create queue to which we will push commands for the device.
    cl::CommandQueue queue(*mContextCL,*mDefaultDevice);

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
AJA_PixelFormat GetAJAPixelFormat (const NTV2FrameBufferFormat format)
{
    switch (format)
    {
    case NTV2_FBF_NUMFRAMEBUFFERFORMATS:		return AJA_PixelFormat_Unknown;
    case NTV2_FBF_10BIT_YCBCR:					return AJA_PixelFormat_YCbCr10;
    case NTV2_FBF_8BIT_YCBCR:					return AJA_PixelFormat_YCbCr8;
    case NTV2_FBF_ARGB:							return AJA_PixelFormat_ARGB8;
    case NTV2_FBF_RGBA:							return AJA_PixelFormat_RGBA8;
    case NTV2_FBF_10BIT_RGB:					return AJA_PixelFormat_RGB10;
    case NTV2_FBF_8BIT_YCBCR_YUY2:				return AJA_PixelFormat_YUY28;
    case NTV2_FBF_ABGR:							return AJA_PixelFormat_ABGR8;
    case NTV2_FBF_10BIT_DPX:					return AJA_PixelFormat_RGB_DPX;
    case NTV2_FBF_10BIT_YCBCR_DPX:				return AJA_PixelFormat_YCbCr_DPX;
    case NTV2_FBF_8BIT_DVCPRO:					return AJA_PixelFormat_DVCPRO;
    case NTV2_FBF_8BIT_QREZ:					return AJA_PixelFormat_QREZ;
    case NTV2_FBF_8BIT_HDV:						return AJA_PixelFormat_HDV;
    case NTV2_FBF_24BIT_RGB:					return AJA_PixelFormat_RGB8_PACK;
    case NTV2_FBF_24BIT_BGR:					return AJA_PixelFormat_BGR8_PACK;
    case NTV2_FBF_10BIT_YCBCRA:					return AJA_PixelFormat_YCbCrA10;
    case NTV2_FBF_10BIT_DPX_LITTLEENDIAN:		return AJA_PixelFormat_RGB_DPX_LE;
    case NTV2_FBF_48BIT_RGB:					return AJA_PixelFormat_RGB16;
    case NTV2_FBF_PRORES:						return AJA_PixelFormat_PRORES;
    case NTV2_FBF_PRORES_DVCPRO:				return AJA_PixelFormat_PRORES_DVPRO;
    case NTV2_FBF_PRORES_HDV:					return AJA_PixelFormat_PRORES_HDV;
    case NTV2_FBF_10BIT_RGB_PACKED:				return AJA_PixelFormat_RGB10_PACK;
    default:									return AJA_PixelFormat_Unknown;
    }
}


static bool get4KInputFormat (NTV2VideoFormat & videoFormat)
{
    bool	status	(false);
    struct	VideoFormatPair
    {
        NTV2VideoFormat	vIn;
        NTV2VideoFormat	vOut;
    } VideoFormatPairs [] =	{	//			vIn										vOut
    {NTV2_FORMAT_1080psf_2398,		NTV2_FORMAT_4x1920x1080psf_2398},
    {NTV2_FORMAT_1080psf_2400,		NTV2_FORMAT_4x1920x1080psf_2400},
    {NTV2_FORMAT_1080p_2398,		NTV2_FORMAT_4x1920x1080p_2398},
    {NTV2_FORMAT_1080p_2400,		NTV2_FORMAT_4x1920x1080p_2400},
    {NTV2_FORMAT_1080p_2500,		NTV2_FORMAT_4x1920x1080p_2500},
    {NTV2_FORMAT_1080p_2997,		NTV2_FORMAT_4x1920x1080p_2997},
    {NTV2_FORMAT_1080p_3000,		NTV2_FORMAT_4x1920x1080p_3000},
    {NTV2_FORMAT_1080p_5000,		NTV2_FORMAT_4x1920x1080p_5000},
    {NTV2_FORMAT_1080p_5994,		NTV2_FORMAT_4x1920x1080p_5994},
    {NTV2_FORMAT_1080p_6000,		NTV2_FORMAT_4x1920x1080p_6000},
    {NTV2_FORMAT_1080p_2K_2398,		NTV2_FORMAT_4x2048x1080p_2398},
    {NTV2_FORMAT_1080p_2K_2400,		NTV2_FORMAT_4x2048x1080p_2400},
    {NTV2_FORMAT_1080p_2K_2500,		NTV2_FORMAT_4x2048x1080p_2500},
    {NTV2_FORMAT_1080p_2K_2997,		NTV2_FORMAT_4x2048x1080p_2997},
    {NTV2_FORMAT_1080p_2K_3000,		NTV2_FORMAT_4x2048x1080p_3000},
    {NTV2_FORMAT_1080p_2K_5000,		NTV2_FORMAT_4x2048x1080p_5000},
    {NTV2_FORMAT_1080p_2K_5994,		NTV2_FORMAT_4x2048x1080p_5994},
    {NTV2_FORMAT_1080p_2K_6000,		NTV2_FORMAT_4x2048x1080p_6000},

    {NTV2_FORMAT_1080p_5000_A,		NTV2_FORMAT_4x1920x1080p_5000},
    {NTV2_FORMAT_1080p_5994_A,		NTV2_FORMAT_4x1920x1080p_5994},
    {NTV2_FORMAT_1080p_6000_A,		NTV2_FORMAT_4x1920x1080p_6000},

    {NTV2_FORMAT_1080p_2K_5000_A,		NTV2_FORMAT_4x2048x1080p_5000},
    {NTV2_FORMAT_1080p_2K_5994_A,		NTV2_FORMAT_4x2048x1080p_5994},
    {NTV2_FORMAT_1080p_2K_6000_A,		NTV2_FORMAT_4x2048x1080p_6000}

};

    for (size_t formatNdx = 0; formatNdx < sizeof (VideoFormatPairs) / sizeof (VideoFormatPair); formatNdx++)
    {
        if (VideoFormatPairs [formatNdx].vIn == videoFormat)
        {
            videoFormat = VideoFormatPairs [formatNdx].vOut;
            status = true;
        }
    }

    return status;

}	//	get4KInputFormat

#if 0
//tp.DrawTestPattern((AJATestPatternSelect)tpNum, mWidth, mHeight, AJA_PixelFormat_BGR8_PACK, tpBuffer);// mct = 1; ft = CMPTO_444_U8_P210;
tp.DrawTestPattern((AJATestPatternSelect)tpNum, mWidth, mHeight, GetAJAPixelFormat( mPixelFormat), tpBuffer); //mct = 1; ft = CMPTO_444_U8_P012Z;
//tp.DrawTestPattern(AJA_TestPatt_MultiPattern, mWidth, mHeight, AJA_PixelFormat_RGB10, tpBuffer); mct = 1; ft = CMPTO_444_U10U10U10_LSB32LE_P012;
//tp.DrawTestPattern((AJATestPatternSelect)tpNum, mWidth, mHeight, AJA_PixelFormat_YCbCr8, tpBuffer); mct = 0;  ft = CMPTO_422_U8_P1020;
//tp.DrawTestPattern((AJATestPatternSelect)tpNum, mWidth, mHeight, AJA_PixelFormat_YCbCr10, tpBuffer); //mct = 0; ft = CMPTO_422_U10_V210;

#endif
