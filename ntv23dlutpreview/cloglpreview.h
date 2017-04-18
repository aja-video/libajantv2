#ifndef cloglpreview_H
#define cloglpreview_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include "dialog.h"
#include <QtMultimedia>

#include "ajaanc/includes/ancillarydata.h"
#include "ajaanc/includes/ancillarydata_hdr_sdr.h"
#include "ajaanc/includes/ancillarydata_hdr_hdr10.h"
#include "ajaanc/includes/ancillarydata_hdr_hlg.h"

class CLOGLPreviewWorker : public QObject
{
    Q_OBJECT
public:
    explicit CLOGLPreviewWorker();
    ~CLOGLPreviewWorker();

    void prepareExit() { mExiting = true;  }
    void setPreviewCircularBuffer(CAVCircularBuffer* captureCircularBuffer) { mCaptureCircularBuffer = captureCircularBuffer; }
    void setCardObject(CNTV2Card* ntv2Card) { mNTV2Card = ntv2Card; }
    void setOutputChannel(NTV2Channel channel) { mOutputChannel = channel; }
    void setWidth(uint32_t w) { mWidth = w; }
    void setHeight(uint32_t h) { mHeight = h; }
    void setPixelFormat(AJA_PixelFormat pf) { mPixelFormat = pf ; }

 signals:
    void finished();
    void updatePreview(int pixelFormat, int width, int height, void* buffer,int bufferSize);



public slots:
    void preview();

private:
    bool setupAutoCirculate ();


    uint64_t mImageCount = 0;

    AJA_PixelFormat mPixelFormat;
    uint32_t mWidth;
    uint32_t mHeight;

    CNTV2Card* mNTV2Card;
    NTV2Channel mOutputChannel;
    CAVCircularBuffer* mCaptureCircularBuffer;
    AUTOCIRCULATE_TRANSFER	mOutputTransfer;
    AJAAncillaryDataType		mAncType;

    bool mInited;
    bool mExiting;

};




#endif // cloglpreview_H
