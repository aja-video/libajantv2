#ifndef NTV2RECORDDPX_H
#define NTV2RECORDDPX_H



#include "ajastuff/common/types.h"
#include "ntv4/driver/types.h"

#include "ajastuff/system/file_io.h"
#include "ajastuff/common/buffer.h"
#include "ajastuff/common/dpx_hdr.h"
#include "ajastuff/common/wavewriter.h"

#include "time.h"
#include <ctime>
#include "ntv2enums.h"
#include "ntv2recordthread.h"
#include "ajacircularbuffer.h"

typedef struct {
	AJABuffer			videoBuffer;
	AJABuffer			audioBuffer;
	uint32_t			audioRecordSize; /// can be smaller than audioBuffer.getbuffersize()
} AJARecordDPXDataBuffer;

const int sRecordDPXCircularBufferSize = 16;

class WriteDPXFileThread;

class NTV2RecordDPX : public NTV2RecordThread
{
	Q_OBJECT

public:
    NTV2RecordDPX(int deviceIndex,
		          QString directoryName,
				  bool previewOnly, // Preview via Qt to PC Monitor
				  bool withAudio=false,
                  NTV2FrameBufferFormat pixelFormat = NTV2_FBF_10BIT_DPX,
				  bool EtoE = true,
				  bool crop4KInput = true
				  );
    ~NTV2RecordDPX();

	// If you want to set this, should be done before calling start() on thread.
    void			setPixelFormat(NTV2FrameBufferFormat pixelFormat) {mPixelFormat = pixelFormat; }
    NTV2FrameBufferFormat getPixelFormat() {return mPixelFormat;}
	
////	NTV4VideoConfiguration getVideoConfig() {return mVideoConfig;}

	void		writeDPXFiles(uint32_t frameWidth, uint32_t frameHeight);
	uint32_t	mRasterWidth;
	uint32_t	mRasterHeight;

signals:
	void newFrameSignal(const QImage &image,bool clear);
	void newFrameSignalWithStatus(const QImage &image,QString &status, bool clear);
	void newStatusString(QString status);

protected:
	void setupDPXHeader(uint32_t frameWidth, uint32_t frameHeight, uint32_t duration, uint32_t scale);
	void setupCircularBuffer(uint32_t imageSize , uint32_t audioSize);
	void recordFromDevice();
    void setupAutoCirculate();

	void setupAudioRecord();
	void outputAudio(uint32_t* buffer, uint32_t bufferSize);

	void    previewFrame(uint8_t* videoBuffer,QString);
	virtual void run();

private:
	int					mAbort;

	int					mSelectedDevice;

	uint32_t			mQueueSize;

	bool				mSmallFile;		// buffered reads/writes for small files
	uint32_t			mImageSize;
	uint32_t			mAudioSize;

	bool				mEtoE;
	bool				mInputCrop;
    NTV2VideoFormat         mVideoFormat;
    NTV2FrameBufferFormat		    mPixelFormat;

	bool					mUseSSE;

	AJARecordDPXDataBuffer						mAVDataBuffer[sRecordDPXCircularBufferSize];
	CAJACircularBuffer<AJARecordDPXDataBuffer*>	mRecordCircularBuffer;

	QImage*				mQImagePool[sRecordDPXCircularBufferSize];
	uint32_t			mImagePoolIndex;

	DpxHdr				mDPXHeader;

	bool				mWithAudio;
	uint32_t			mNumAudioChannels;
	AJAWavWriter*		mWaveWriter;

	WriteDPXFileThread*	mWriteDPXThread;
};

class WriteDPXFileThread : public QThread
{

public:
    WriteDPXFileThread(NTV2RecordDPX* recordDPXThread,QObject *parent = 0);
	~WriteDPXFileThread();

protected:
	void run();

    NTV2RecordDPX* mRecordDPXThread;
};
#endif
