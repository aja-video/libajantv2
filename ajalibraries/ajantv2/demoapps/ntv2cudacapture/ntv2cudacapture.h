#ifndef NTV2CudaCapture_H
#define NTV2CudaCapture_H

#include "gpustuff/include/cudaTransfer.h"
#include "gpustuff/utility/videoprocessor.h"

#include <QtGui>
#include <QGLWidget>
#include <GL/glu.h>
#include "ajastuff/common/public.h"
#include "ajastuff/common/types.h"
#include "ajastuff/common/buffer.h"

#include "ntv2card.h"

#include "time.h"
#include <ctime>
#include <cuda.h>




class NTV2CudaCapture : public CVideoProcessor<CCudaObject>
{
	Q_OBJECT

	public:	
	NTV2CudaCapture(int deviceIndex,  ICudaTransfer *gpuTransfer,   bool rgbCapture = true);
	~NTV2CudaCapture();

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


	CCudaObject										*mCudaObjects;
	CCpuObject										*mCpuObjects;
	int					mNumCudaObjects;

	CNTV2Card								mNTV2Card;
	bool									mRestart;
	uint16_t								mBoardIndex;
	NTV2BoardID								mBoardID;
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


	uint32_t			mNumFramesConsumed;
	uint32_t			mNumFramesDropped;

	bool				mRGBCapture;

};


#endif
