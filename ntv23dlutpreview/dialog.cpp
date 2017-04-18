#include "dialog.h"
#include <string>
#include "ntv2devicescanner.h"
//
//	This array maps SDI input numbers 0 thru 3 to an NTV2InputSource...
//
static const NTV2InputSource	gInputNumberToInputSource [] =
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


Dialog::Dialog(QWidget *parent) :
    QDialog(parent)

{
    setupUi(this);
    createWorker();
    setupBoardCombo();
    setupPixelFormatCombo();
    pixelFormatChoiceComboBox->setCurrentIndex(0);
    pixelFormatChanged(0);
    mWorkerThread->start();

    connect( cardChoiceComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(cardIndexChanged(int)));
    connect( pixelFormatChoiceComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(pixelFormatChanged(int)));
    connect(openGLWidget,SIGNAL(droppedFile(QString)),this,SLOT(readDroppedFile(QString))); // for dropping j2k files
    connect(grabTiffPushButton,SIGNAL(pressed()),this,SLOT(grabButtonPressed()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateInputs()));
    timer->start(500);

}

Dialog::~Dialog()
{

    killWorker();
}

void Dialog::grabButtonPressed()
{
    qDebug() << "grab button pressed";
    mWorker->grabTIFF();
}

void Dialog::createWorker()
{
    mWorkerThread = new QThread;
    mWorker = new CWorker(this);
    mWorker->moveToThread(mWorkerThread);
    connect(mWorkerThread, &QThread::finished, mWorker, &QObject::deleteLater);
    connect(mWorker, SIGNAL(finished()), mWorker, SLOT(deleteLater()));
    connect(mWorkerThread, SIGNAL(started()), mWorker, SLOT(doWork()));

    mWorker->setOpenGLPreviewWidget(openGLWidget);
}

void Dialog::killWorker()
{
    mWorker->prepareExit();
    mWorkerThread->quit();
    mWorkerThread->wait();
    delete mWorkerThread;

}

void Dialog::setupBoardCombo()
{
    for (ULWord ndx (0);  ndx < 100;  ndx++)
    {
        CNTV2Card	device;
        if (CNTV2DeviceScanner::GetDeviceAtIndex (ndx, device))
            cardChoiceComboBox->addItem (tr (device.GetDisplayName ().c_str ()));
        else if (ndx == 0)
            {cardChoiceComboBox->addItem (tr ("No Devices Found"));	break;}
        else
            break;
    }
    cardChoiceComboBox->setCurrentIndex (0);

}

void Dialog::cardIndexChanged(int newIndex)
{
    mWorker->setCardIndex(newIndex);
}

void Dialog::setupPixelFormatCombo()
{
    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_10BIT_RGB,	true).c_str(),QVariant((qlonglong)NTV2_FBF_10BIT_RGB));
    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_48BIT_RGB,	true).c_str(),QVariant((qlonglong)NTV2_FBF_48BIT_RGB));
//    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_8BIT_YCBCR,	true).c_str(),QVariant((qlonglong)NTV2_FBF_8BIT_YCBCR));
//    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_8BIT_YCBCR_YUY2,	true).c_str(),QVariant((qlonglong)NTV2_FBF_8BIT_YCBCR_YUY2));
//    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_ABGR,	true).c_str(),QVariant((qlonglong)NTV2_FBF_ABGR));
//    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_24BIT_BGR,	true).c_str(),QVariant((qlonglong)NTV2_FBF_24BIT_BGR));
//    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_ARGB,	true).c_str(),QVariant((qlonglong)NTV2_FBF_ARGB));
//    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_10BIT_DPX,	true).c_str(),QVariant((qlonglong)NTV2_FBF_10BIT_DPX));
//    pixelFormatChoiceComboBox->addItem(NTV2FrameBufferFormatToString (NTV2_FBF_10BIT_DPX_LITTLEENDIAN,	true).c_str(),QVariant((qlonglong)NTV2_FBF_10BIT_DPX_LITTLEENDIAN));

}

void Dialog::pixelFormatChanged(int newIndex)
{
    qDebug() << "New Index" << newIndex;
    QVariant pixelFormatVariant = pixelFormatChoiceComboBox->itemData(newIndex);
    NTV2FrameBufferFormat pixelFormat = (NTV2FrameBufferFormat)pixelFormatVariant.toInt();
    mWorker->setPixelFormat(pixelFormat);

}



void Dialog::updateFramesDropped(int frameDropped)
{
    QString framesDroppedStr = QString("%1").arg(frameDropped);
    framesDroppedLabel->setText(framesDroppedStr);

}

void Dialog::updateFramesCaptured(int frameCaptured)
{
    QString framesCapturedStr = QString("%1").arg(frameCaptured);
    framesCapturedLabel->setText(framesCapturedStr);

}

void Dialog::updateInputs()
{
    uint32_t boardIndex = cardChoiceComboBox->currentIndex ();
    CNTV2Card ntv2Card;
    if (ntv2Card.Open(boardIndex) == true)
    {
        NTV2VideoFormat inputVideoFormat;
        inputVideoFormat = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1);
        inputVideoFormatLabel_1->setText(NTV2VideoFormatToString(inputVideoFormat).c_str());
        inputVideoFormat = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI2);
        inputVideoFormatLabel_2->setText(NTV2VideoFormatToString(inputVideoFormat).c_str());
        inputVideoFormat = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI3);
        inputVideoFormatLabel_3->setText(NTV2VideoFormatToString(inputVideoFormat).c_str());
        inputVideoFormat = ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI4);
        inputVideoFormatLabel_4->setText(NTV2VideoFormatToString(inputVideoFormat).c_str());

    }

}

void Dialog::readDroppedFile(QString fileName)
{
    if ( fileName.indexOf(".cube",0,Qt::CaseInsensitive) != -1)
        openGLWidget->initialize3DLUT(fileName);
    if ( fileName.indexOf(".lut16",0,Qt::CaseInsensitive) != -1)
        openGLWidget->readLUT16(fileName);

}
