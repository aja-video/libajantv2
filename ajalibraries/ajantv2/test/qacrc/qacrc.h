////////////////////////////////////////////////////////
//
// qacrc.h
//
////////////////////////////////////////////////////////

#include <stdio.h>
#include <iostream>
#include <string>
#include <signal.h>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2status.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ajastuff/common/circularbuffer.h"
#include "ajastuff/system/thread.h"
#include "ajastuff/common/timecodeburn.h"
#include "ntv2rp188.h"
#include "../demoapps/ntv2democommon.h"
#include "ajastuff/common/buffer.h"

using namespace std;

NTV2_STRUCT_BEGIN(QACRC_DATA)
	NTV2_HEADER		qacrcHeader;
		bool mbSource;
		bool mbAudioData;
		bool mbAudioLevel;
		bool mbLog;
		bool mbOutputImageToFile;
		bool mbTestError;
		bool mbTrickMode;
		bool mbUseReferenceFile;
		bool mbVerbose;
		bool mbVideoData;
		bool mbUseCSC;
		bool mbQuiet;
		bool mbManualMode;
		bool mbAudioAlignment;
		bool mbDoLevelConversion;
		bool mbEnableVanc;
		bool mbWideVanc;
		bool mbLockToInput;
		bool mbWithAnc;
		bool mbLogVideoErrors;
		bool mbLogAudioErrors;
		bool mbLogAncErrors;
		bool mbUseStaticImage;
		bool mbBurnTimecode;
		bool mbUseReferenceAsLock;
		int mAudioDepth;
		int mDeviceIndex;
		int mFirstFrameIndex;
		int mLastFrameIndex;
		int mVerboseCount;
		int mVideoRangeLow;
		int mVideoRangeHigh;
		int mWiggleRoom;
		int mDataInspectionLevel;
		uint32_t mPlayFrames;
		uint32_t mSkipFrames;
		uint32_t mTestCount;
		uint32_t mAudioChannelFirst;
		uint32_t mAudioChannelLast;
		uint32_t mAudioLevel;
		uint64_t AudioSum[16];

		std::string msRefFileName;
		std::string msLogFileName;
		std::string msOutputFileName;

		NTV2Channel mVideoChannel;
		NTV2Crosspoint mChannelSpec;
		NTV2AudioSystem mAudioSystem;
		NTV2VideoFormat mVideoFormat;
		NTV2FrameBufferFormat mPixelFormat;
	NTV2_TRAILER		qacrcTrailer;
	
	explicit		QACRC_DATA();

	NTV2_BEGIN_PRIVATE
		inline explicit	QACRC_DATA(const QACRC_DATA & inObj) : qacrcHeader(0xFEFEFEFE, 0) { (void)inObj; }
		inline QACRC_DATA &	operator = (const QACRC_DATA & inRHS){ (void)inRHS; return *this; }
	NTV2_END_PRIVATE

NTV2_STRUCT_END	(QACRC_DATA)


class QACrc
{
	public:
		QACrc();

		virtual ~QACrc(void);

		/**
		@brief	Initialize
		**/
		virtual AJAStatus		Init(QACRC_DATA & inQaCrcData);
		virtual AJAStatus		Run(void);
		virtual void			Quit(void);
		virtual bool			IsRunning(void) const{ return !mbGlobalQuit; }
		virtual void GetStatus(ULWord & outFramesProcessed, ULWord & outFramesDropped,
											ULWord & outBufferLevel, uint32_t & outVideoErrorCount,
											uint32_t & outAudioErrorCount, uint32_t & outAncF1ErrorCount,
											uint32_t & outAncF2ErrorCount, uint32_t & outVideoErrorInsertionCount);

	protected:
		/**
		@brief	Sets up everything I need to play video.
		**/
		virtual AJAStatus		SetUpVideo(void);

		/**
		@brief	Sets up everything I need to play audio.
		**/
		virtual AJAStatus		SetUpAudio(void);

		/**
		@brief	Sets up device routing for playout.
		**/
		virtual void			RouteOutputSignal(void);

		/**
		@brief	Sets up device routing for capture.
		**/
		virtual void			RouteInputSignal(void);

		/**
		@brief	Sets up my circular buffers.
		**/
		virtual void			SetUpHostBuffers(void);

		virtual AJAStatus		AddHostBuffers(const uint32_t inNumberOfBuffersToAdd);

		/**
		@brief	Initializes playout AutoCirculate.
		**/
		virtual void			SetUpOutputAutoCirculate(void);

		/**
		@brief	Initializes playout AutoCirculate.
		**/
		virtual void			SetUpInputAutoCirculate(void);

		/**
		@brief	Creates video pattern buffers.
		**/
		virtual AJAStatus		GenerateReferenceVideo(void);

		/**
		@brief	Creates audio pattern buffers.
		**/
		virtual AJAStatus		GenerateReferenceAudio(void);

		/**
		@brief	Creates anc pattern buffers.
		**/
		virtual AJAStatus		GenerateReferenceAnc(uint32_t inVideoReferenceBufferIndex, uint32_t inPayloadSize);

		/**
		@brief	Creates my timecode data
		**/
		virtual AJAStatus		GenerateReferenceTimeCode(uint32_t framesProcessed);


		/**
		@brief	Starts my playout thread.
		**/
		virtual void			StartThreads(void);

		/**
		@brief	Repeatedly plays out frames using AutoCirculate (until quit).
		**/
		virtual void			ACSource(void);

		/**
		@brief	Repeatedly captures frames using AutoCirculate (until quit).
		**/
		virtual void			ACSink(void);

		/**
		@brief	This is the sink thread's static callback function that gets called when the consumer thread starts.
		This function gets "Attached" to the consumer thread's AJAThread instance.
		@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
		@param[in]	pContext	Context information to pass to the thread.
		(For this application, this will be set to point to the NTV2Player instance.)
		**/
		static void				SinkThreadStatic(AJAThread * pThread, void * pContext);
		static void				DataInspectionThreadStatic(AJAThread * pThread, void * pContext);

		/**
		@brief	This is the producers thread's static callback function that gets called when the consumer thread starts.
		This function gets "Attached" to the consumer thread's AJAThread instance.
		@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
		@param[in]	pContext	Context information to pass to the thread.
		(For this application, this will be set to point to the NTV2Player instance.)
		**/
		static void				SourceThreadStatic(AJAThread * pThread, void * pContext);

		/**
		@brief	Returns the equivalent TimecodeFormat for the given NTV2FrameRate.
		@param[in]	inFrameRate		Specifies the NTV2FrameRate to be converted into an equivalent TimecodeFormat.
		**/
		static TimecodeFormat	NTV2FrameRate2TimecodeFormat(const NTV2FrameRate inFrameRate);
		virtual AJAStatus		InspectData();

		/**
		@brief		Checks ancillary packet against the video buffer
		@param[in]	pInAnc		A valid pointer to the received ancillary buffer
		@param[in]	ppInOutVid	A pointer to a valid pointer to the received video buffer.
								On return, the pointer will be advanced by the number of bytes checked.
		@param[in]	bField2		True if ancillary data read from field 2; else false
		@return		AJA_STATUS_SUCCESS if ancillary data passes the check; error status if check failed
		**/
		virtual	AJAStatus		CheckAncPacketAndVideo (const uint8_t * pInAnc, uint8_t ** ppInOutVid, const bool bIsField2);

	private:
		bool						mbGlobalQuit;
		bool						mbTestError;

		QACRC_DATA*					mpCrcData;
		typedef AJACircularBuffer <AVDataBuffer *>		aCirculateBuffer;

		AJAThread*	mACThread;
		AJAThread* mTestThread;

		CNTV2Card					mDevice;
		NTV2DeviceID				mDeviceID;
		AVDataBuffer				mAVHostBuffer[CIRCULAR_BUFFER_SIZE];
		aCirculateBuffer			mAVCircularBuffer;
		uint32_t*					mVideoReferenceData;
		uint32_t*					mAudioReferenceData;
		uint8_t*					mField1AncReferenceData;
		uint8_t*					mField2AncReferenceData;
		RP188_STRUCT				mRP188ReferenceData;
		std::string					mRP188String;
		NTV2FrameRate				mFrameRate;
		TimecodeFormat				mTCFormat;
		AJATimeCodeBurn				mTCBurner;
		NTV2FormatDescriptor		mFormatDescriptor;

		AUTOCIRCULATE_TRANSFER		mTransfer;
		AUTOCIRCULATE_STATUS		mACStatus;

		uint32_t					mVideoBufferSize;
		uint32_t					mVideoReferenceBufferSize;
		uint32_t					mAudioBufferSize;
		uint32_t					mAncBufferSize;
		uint32_t					mCurrentAncPayloadSize;
		uint32_t					mAncPayloadSizeLimit;

		uint32_t					mVideoErrorCount;
		uint32_t					mVideoErrorInsertionCount;
		uint32_t					mAudioErrorCount;
		uint32_t					mAudioErrorInsertionCount;
		uint32_t					mAncF1ErrorCount;
		uint32_t					mAncF2ErrorCount;

		uint32_t					mVideoReferenceBufferIndex;
		uint32_t					mVideoReferenceBufferIndexLockCount;
		bool						mVideoReferenceIndexLocked;
		bool						mVideoReferenceIndexNeverLocked;

		AJALock*					mLock;
};

