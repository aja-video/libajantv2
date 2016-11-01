#ifndef NTV2OglOutput_H
#define NTV2OglOutput_H

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

class NTV2OglOutput : public CVideoProcessor<COglObject>
{
	Q_OBJECT

	public:
	NTV2OglOutput(int deviceIndex, uint32_t numBitsPerComponent, IOglTransfer *gpuTransfer);
	~NTV2OglOutput();

	uint32_t getFramesDropped();


protected:
	virtual bool Init();
	virtual bool Deinit();
	//these are called in the thread
	virtual bool Process();
	virtual bool SetupThread();
	virtual bool CleanupThread();
private:
	bool SetupOutput();
	void SetupAutoCirculate();
	void StartAutoCirculate();
	void StopAutoCirculate();

	COglObject								*mOglObjects;
	CCpuObject								*mCpuObjects;
	int										mNumOglObjects;

	CNTV2Card								mNTV2Card;
	bool									mRestart;	
	uint16_t								mBoardIndex;
	NTV2DeviceID							mBoardID;
	NTV2Channel								mChannel;
	NTV2Crosspoint				            mChannelSpec;
	NTV2VideoFormat							mVideoFormat;
	bool									mCheckFor4K;

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

	uint32_t			mNumBitsPerComponent;
};



#endif
