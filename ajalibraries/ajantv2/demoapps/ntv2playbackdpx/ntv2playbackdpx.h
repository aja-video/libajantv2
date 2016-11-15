#ifndef NTV2PLAYBACKDPX_H
#define NTV2PLAYBACKDPX_H



#include "ajabase/common/types.h"
#include "ajabase/common/buffer.h"

#include "ntv2enums.h"
#include "ntv2card.h"
#include "ntv2boardscan.h"
#include "ntv2devicescanner.h"
#include "ajacircularbuffer.h"
#include "ntv2playbackthread.h"


class PlayBackToBoardThread;
class PlayPreviewThread;

typedef struct {
	AJABuffer			videoBuffer;
	AJABuffer			audioBuffer;
} AJAPlaybackDataBuffer;

typedef struct {
    uint32_t*			videoBuffer;
    uint32_t			videoBufferSize;

} AJAVideoPreviewBuffer;

const int sPlaybackDPXCircularBufferSize = 16;

typedef struct {
	uint32_t			  offset;
	uint32_t			  size;
	uint32_t			  width;
	uint32_t		      height;
////	NTV4VideoRate		  rate;
    NTV2VideoFormat		  videoFormat;
    NTV2FrameBufferFormat		  pixelFormat;
    uint32_t            numPitsPerCompent;
} DPXInfo;

class NTV2PlaybackDPX : public NTV2PlaybackThread
{
	Q_OBJECT

public:
    NTV2PlaybackDPX(int deviceIndex,const std::string& directoryName, NTV2VideoFormat videoFormat);
    ~NTV2PlaybackDPX();

	void playback();
    void preview();

	void	setPause(bool pause);
	void	nextFrame();
	void	previousFrame();
	void	setCurrentFrame(int32_t frame);

	int32_t  getNumberOfFrames();
	int32_t  getCurrentFrameNumber();
	uint32_t getFramesRead();
	uint32_t getFramesDropped();

signals:
	void newFrameSignal(const QImage &image,bool clear);
	void newStatusString(QString status);

protected:
	void run();

    void openDevice();
    void closeDevice();
	void previewFrame(uint8_t* videoBuffer);
    bool setupFileList();
	void setupCircularBuffer();
    void setupOutputVideo();
    void setupAutoCirculate();
	void readDPXFiles();

	void getDPXFileInfo(QString fileNameStr);



private:

    NTV2VideoFormat                 mVideoFormat;
    NTV2FrameBufferFormat		 mPixelFormat;

    uint32_t                                mRasterImageSize;
    uint32_t                                mRasterWidth;
    uint32_t                                mRasterHeight;
    bool                                      mSmallFile;		// buffered reads/writes for small files

    DPXInfo                                mDPXInfo;

    bool                                        mPause;
    int32_t                                     mCurrentFrameNumber;
    int32_t                                     mNumFramesRead;

    int                                             mSelectedDevice;
    std::string                                 mDirectoryPath;
    std::string                                 mTrackName;

    uint32_t                                    mQueueSize;


	AJAPlaybackDataBuffer							mAVDataBuffer[sPlaybackDPXCircularBufferSize];
	CAJACircularBuffer<AJAPlaybackDataBuffer*>	    mPlaybackCircularBuffer;

    AJAVideoPreviewBuffer							mPreviewDataBuffer[sPlaybackDPXCircularBufferSize];
    CAJACircularBuffer<AJAVideoPreviewBuffer*>	    mPreviewCircularBuffer;

    QImage*                                    mQImagePool[sPlaybackDPXCircularBufferSize];
    uint32_t                                     mImagePoolIndex;

    uint32_t                                     mNumFiles;
    QFileInfoList                              mFileList;

    bool                                             mUseSSE;
    CNTV2DeviceScanner                  mNTV2Scanner;				///	My AJA device scanner
    CNTV2Card                                   mDevice;
    NTV2EveryFrameTaskMode          mPreviousFrameServices;		/// Used to restore the previous task mode

    NTV2Crosspoint				mChannelSpec;				///	The AutoCirculate channel spec I'm using
    AUTOCIRCULATE_TRANSFER_STRUCT           mOutputTransferStruct;					///	My A/C output transfer info
    AUTOCIRCULATE_TRANSFER_STATUS_STRUCT    mOutputTransferStatusStruct;			///	My A/C output status

    PlayBackToBoardThread*          mPlaybacktoBoardThraed;
    PlayPreviewThread*                  mPlayPreviewThread;


    int                                         mAbort;

};

class PlayBackToBoardThread : public QThread
{

public:
    PlayBackToBoardThread(NTV2PlaybackDPX* playbackDPXThread,QObject *parent = 0);
	~PlayBackToBoardThread();

protected:
	void run();

    NTV2PlaybackDPX* mPlaybackDPXThread;
};
class PlayPreviewThread : public QThread
{

public:
    PlayPreviewThread(NTV2PlaybackDPX* playbackDPXThread,QObject *parent = 0);
    ~PlayPreviewThread();

protected:
    void run();

    NTV2PlaybackDPX* mPlaybackDPXThread;
};
#endif
