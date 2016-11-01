#ifndef NTV2OGLCAPTURE_H
#define NTV2OGLCAPTURE_H

#include "gpustuff/include/oglTransfer.h"
#include "gpustuff/utility/videoprocessor.h"


#include <QtGui>
#include <QGLWidget>
#include <GL/glu.h>
#include "ajastuff/common/public.h"
#include "ajastuff/common/types.h"
#include "ajastuff/common/buffer.h"
#include "time.h"
#include <ctime>


#include "ntv2card.h"
//This class responsibility is to create the GPU objects, circular buffer and manage the capture into these objects
class NTV2OglCapture : public CVideoProcessor<COglObject>
{
	Q_OBJECT

	public:
	//NTV2OglCapture(int deviceIndex, QGLContext* oglContext,IOglTransfer *gpuTransfer,  bool rgbCapture = true);
	NTV2OglCapture(int deviceIndex, IOglTransfer *gpuTransfer,   bool rgbCapture = true);
	~NTV2OglCapture();

	uint32_t getFramesDropped();


protected:
	virtual bool Init();
	virtual bool Deinit();
	//these are called in the thread
	virtual bool Process();
	virtual bool SetupThread();
	virtual bool CleanupThread();
private:
	bool SetupInput();
	void SetupAutoCirculate();
	void StartAutoCirculate();
	void StopAutoCirculate();

	NTV2VideoFormat GetVideoFormatFromInputSource();

	COglObject								*mOglObjects;
	CCpuObject								*mCpuObjects;
	int										mNumOglObjects;

	CNTV2Card								mNTV2Card;
	bool									mRestart;
	uint16_t								mBoardIndex;
	NTV2DeviceID							mBoardID;
	NTV2Crosspoint				            mChannelSpec;
	NTV2VideoFormat							mCurrentVideoFormat;
	NTV2VideoFormat							mLastVideoFormat;
	bool									mCheckFor4K;

	uint32_t								mVideoFormatDebounceCounter;
	bool									mFormatProgressive;
	NTV2InputSource							mInputSource;
	uint32_t                                mMinFrame; //Start frame for ac
	uint32_t                                mMaxFrame; //End frame for ac
	uint32_t								mHeight;
	uint32_t								mWidth;
	NTV2FrameBufferFormat                   mFrameBufferFormat;
	AUTOCIRCULATE_TRANSFER_STRUCT           mTransferStruct;
	AUTOCIRCULATE_TRANSFER_STATUS_STRUCT    mTransferStatusStruct;
	NTV2EveryFrameTaskMode					mRestoreMode;

	AJABuffer								mVideoBuffer;
	bool									mbWithAudio;
	uint32_t								mNumAudioChannels;



	bool				mRGBCapture;


	uint32_t			mNumFramesConsumed;
	uint32_t			mNumFramesDropped;

};


#endif
