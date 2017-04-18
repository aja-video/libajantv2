#include "cloglpreview.h"


#include <QString>
#include <QFile>
#include <QElapsedTimer>
#include <QDateTime>
#include <QDebug>
#include "ajabase/common/videotypes.h"
#include "ajabase/common/timer.h"
#include "ajabase/system/memory.h"
#define NTV2_ANCSIZE_MAX	(0x2000)

#define ANC_SUPPORT

CLOGLPreviewWorker::CLOGLPreviewWorker()
    : mInited(false),
      mExiting(false),
      mWidth(0),
      mHeight(0),
      mNTV2Card(NULL),
      mCaptureCircularBuffer(NULL),
      mOutputChannel(NTV2_CHANNEL3),
      mPixelFormat(AJA_PixelFormat_RGB10),
      mAncType(AJAAncillaryDataType_HDR_SDR)
{

}

CLOGLPreviewWorker::~CLOGLPreviewWorker()
{

}


void CLOGLPreviewWorker::preview()
{
    AUTOCIRCULATE_TRANSFER		mOutputXferInfo;

    mImageCount = 0;
    setupAutoCirculate();
    mNTV2Card->SubscribeOutputVerticalEvent (mOutputChannel);
    while (!mExiting)
    {
        AUTOCIRCULATE_STATUS	outputStatus;
        mNTV2Card->AutoCirculateGetStatus (mOutputChannel, outputStatus);
        //	Check if there's room for another frame on the card...
        if (outputStatus.GetNumAvailableOutputFrames () > 1)
        {

            AJAAVBuffer* frameData = mCaptureCircularBuffer->StartConsumeNextBuffer();
            if ( frameData == NULL )
                continue;
            AJATimer timer;
            timer.Start();

            updatePreview(mPixelFormat,mWidth,mHeight,frameData->videoBuffer,frameData->videoBufferSize);

#ifdef ANC_SUPPORT
            uint32_t*	fAncBuffer = reinterpret_cast <uint32_t *> (AJAMemory::AllocateAligned (NTV2_ANCSIZE_MAX, AJA_PAGE_SIZE)) ;
            uint32_t	fAncBufferSize = NTV2_ANCSIZE_MAX;
            uint32_t	packetSize = 0;
            switch(mAncType)
            {
            case AJAAncillaryDataType_HDR_SDR:
            {
                AJAAncillaryData_HDR_SDR sdrPacket;
                sdrPacket.GenerateTransmitData((uint8_t*)fAncBuffer, fAncBufferSize, packetSize);
                break;
            }
            case AJAAncillaryDataType_HDR_HDR10:
            {
                AJAAncillaryData_HDR_HDR10 hdr10Packet;
                hdr10Packet.GenerateTransmitData((uint8_t*)fAncBuffer, fAncBufferSize, packetSize);
                break;
            }
            case AJAAncillaryDataType_HDR_HLG:
            {
                AJAAncillaryData_HDR_HLG hlgPacket;
                hlgPacket.GenerateTransmitData((uint8_t*)fAncBuffer, fAncBufferSize, packetSize);
                break;
            }
            }
#endif
            mOutputXferInfo.SetVideoBuffer ((ULWord*)frameData->videoBuffer, frameData->videoBufferSize);
            mNTV2Card->AutoCirculateTransfer (mOutputChannel, mOutputXferInfo);

            mImageCount++;

            //            qDebug("   Consumer: %d",timer.ElapsedTime());
            timer.Stop();
            mCaptureCircularBuffer->EndConsumeNextBuffer();
        }
        else
            mNTV2Card->WaitForOutputVerticalInterrupt (mOutputChannel);

    }
    /* Cleanup */
    mNTV2Card->AutoCirculateStop (mOutputChannel);
    mNTV2Card->UnsubscribeOutputVerticalEvent (mOutputChannel);

    emit finished();
}
bool CLOGLPreviewWorker::setupAutoCirculate()
{
    if (mNTV2Card)
    {
        mNTV2Card->AutoCirculateStop (mOutputChannel);
        mNTV2Card->AutoCirculateInitForOutput(mOutputChannel,3,NTV2_AUDIOSYSTEM_INVALID,AUTOCIRCULATE_WITH_ANC,1,40,43);
        mNTV2Card->AutoCirculateStart (mOutputChannel);


        return true;
    }
    else
    {
        return false;
    }
    return false;
}


