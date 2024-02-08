/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2encodehevcfileac.h
	@brief		Declares the NTV2EncodeHEVCFileAc class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2ENCODEHEVCFILEAC_H
#define _NTV2ENCODEHEVCFILEAC_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2demohevccommon.h"

#include "ntv2m31enums.h"

#include "ajabase/common/videotypes.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/common/timebase.h"
#include "ajabase/common/timecode.h"
#include "ajabase/common/timecodeburn.h"
#include "ajabase/system/thread.h"

#include "ntv2m31.h"

#define VIDEO_RING_SIZE			60
#define AUDIO_RING_SIZE			(3*VIDEO_RING_SIZE)


/**
	@brief	Instances of me capture frames in real time from a video signal provided to an input of
			an AJA device.
**/

class NTV2EncodeHEVCFileAc
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given settings.
			@note	I'm not completely initialized and ready to use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use.
											Defaults to "0" (first device found).
			@param[in]	inChannel			Specifies the channel to use.
											Defaults to NTV2_CHANNEL1.
			@param[in]	inM31Preset			Specifies the m31 preset to use.
											Defaults to 8-bit 1280x720 5994p.
            @param[in]	inPixelFormat		Specifies the pixel format to use.
                                            Defaults to NTV2_FBF_10BIT_YCBCR_420PL2.
            @param[in]	inQuadMode  		Specifies UHD mode.
                                            Defaults to HD mode.
            @param[in]	inAudioChannels		Specifies number of audio channels to write to AIFF file.
                                            Defaults to 2 channels.
            @param[in]	inTimeCodeBurn      Add timecode burn.
                                            Defaults to no timecode burn.
            @param[in]	inInfoData          Use picture and encoded information.
                                            Defaults to no info data.
            @param[in]	inMaxFrames			Specifies the maximum number of frames to capture.
            								Defaults to 0xFFFFFFFF (unlimited).
        **/
		NTV2EncodeHEVCFileAc (const std::string			inDeviceSpecifier	= "0",
							const NTV2Channel			inChannel			= NTV2_CHANNEL1,
							const M31VideoPreset		inM31Preset			= M31_FILE_1280X720_420_8_5994p,
                            const NTV2FrameBufferFormat	inPixelFormat		= NTV2_FBF_10BIT_YCBCR_420PL2,
							const bool                  inQuadMode          = false,
							const uint32_t              inAudioChannels     = 0,
							const bool                  inTimeCodeBurn      = false,
							const bool                  inInfoData          = false,
							const uint32_t              inMaxFrames         = 0xffffffff);

        virtual					~NTV2EncodeHEVCFileAc ();

		/**
			@brief	Initializes me and prepares me to Run.
		**/
		virtual AJAStatus		Init (void);

		/**
			@brief	Runs me.
			@note	Do not call this method without first calling my Init method.
		**/
		virtual AJAStatus		Run (void);

		/**
			@brief	Gracefully stops me from running.
		**/
		virtual void			Quit (void);

		/**
			@brief	Provides status information about my input (capture) process.
			@param[out]	outStatus	Receives status information about my input (capture) process.
		**/
        virtual void            GetStatus (AVHevcStatus & outStatus);

        /**
            @brief	Get the codec preset
        **/
        virtual M31VideoPreset	GetCodecPreset (void);

	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need for capturing video.
		**/
		virtual AJAStatus		SetupVideo (void);

		/**
			@brief	Sets up everything I need for capturing audio.
		**/
		virtual AJAStatus		SetupAudio (void);

		/**
			@brief	Sets up device routing for capture.
		**/
		virtual void			RouteInputSignal (void);

		/**
			@brief	Sets up my circular buffers.
		**/
		virtual void			SetupHostBuffers (void);

		/**
			@brief	Initializes AutoCirculate.
		**/
        virtual void			SetupAutoCirculate (void);

		/**
			@brief	Start the video input thread.
		**/
        virtual void			StartVideoInputThread (void);

		/**
			@brief	Start the video process thread.  
		**/
        virtual void			StartVideoProcessThread (void);

		/**
			@brief	Start the codec raw thread.  
		**/
        virtual void			StartCodecRawThread (void);

		/**
			@brief	Start the codec hevc thread.  
		**/
        virtual void			StartCodecHevcThread (void);

		/**
            @brief	Start the video file writer thread.
		**/
        virtual void			StartVideoFileThread (void);

        /**
            @brief	Start the audio file writer thread.
        **/
        virtual void			StartAudioFileThread (void);

        /**
            @brief	Repeatedly captures video frames using AutoCirculate and add them to
					the video input ring.
        **/
        virtual void			VideoInputWorker (void);

        /**
            @brief	Repeatedly removes video frames from the video input ring, calls a
					custom video process method and adds the result to the raw video ring.
        **/
        virtual void			VideoProcessWorker (void);

        /**
            @brief	Repeatedly removes video frames from the raw video ring and transfers
					them to the codec.
        **/
        virtual void			CodecRawWorker (void);

        /**
            @brief	Repeatedly transfers hevc frames from the codec and adds them to the
					hevc ring.
        **/
        virtual void			CodecHevcWorker (void);

        /**
            @brief	Repeatedly removes hevc frame from the hevc ring and writes them to the
					hevc output file.
        **/
        virtual void			VideoFileWorker (void);

        /**
            @brief	Repeatedly removes audio samples from the audio input ring and writes them to the
                    audio output file.
        **/
        virtual void			AudioFileWorker (void);

        /**
			@brief	Default do-nothing function for processing the captured frames.
		**/
        virtual AJAStatus		ProcessVideoFrame (AVHevcDataBuffer * pSrcFrame, AVHevcDataBuffer * pDstFrame, uint32_t frameNumber);

        //	Protected Class Methods
	protected:
		/**
			@brief	This is the video input thread's static callback function that gets called when the thread starts.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
		**/
        static void				VideoInputThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the video process thread's static callback function that gets called when the thread starts.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
		**/
        static void				VideoProcessThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the codec raw thread's static callback function that gets called when the thread starts.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
		**/
        static void				CodecRawThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the codec hevc thread's static callback function that gets called when the thread starts.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
		**/
        static void				CodecHevcThreadStatic (AJAThread * pThread, void * pContext);

		/**
            @brief	This is the video file writer thread's static callback function that gets called when the thread starts.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
		**/
        static void				VideoFileThreadStatic (AJAThread * pThread, void * pContext);

        /**
            @brief	This is the audio file writer thread's static callback function that gets called when the thread starts.
                    This function gets "Attached" to the consumer thread's AJAThread instance.
            @param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
            @param[in]	pContext	Context information to pass to the thread.
        **/
        static void				AudioFileThreadStatic (AJAThread * pThread, void * pContext);

    //	Private Member Data
	private:
        AJAThread					mVideoInputThread;		///	Video input thread
        AJAThread					mVideoProcessThread;	///	Video processing thread
        AJAThread					mCodecRawThread;		///	Codec raw transfer
        AJAThread					mCodecHevcThread;		///	Codec hevc transfer thread
        AJAThread					mVideoFileThread;		///	Video file writer thread
        AJAThread					mAudioFileThread;		///	Audio file writer thread
        CNTV2m31 *					mM31;					/// Object used to interface to m31
        CNTV2DemoHevcCommon *       mHevcCommon;            /// HEVC common class

        CNTV2Card					mDevice;				///	CNTV2Card instance
        NTV2DeviceID				mDeviceID;				///	Device identifier
		const std::string			mDeviceSpecifier;		///	The device specifier string
        bool        				mWithAudio;				///	Capture and playout audio?
        NTV2Channel                 mInputChannel;			///	Input channel
        M31Channel                  mEncodeChannel;         /// Encoder channel
		M31VideoPreset				mPreset;				/// M31 HEVC Preset 
		NTV2InputSource				mInputSource;			///	The input source I'm using
        NTV2VideoFormat				mVideoFormat;			///	Video format
        NTV2FrameBufferFormat		mPixelFormat;			///	Pixel format
		bool						mQuad;					/// VideoFormat is quad
		bool						mInterlaced;			/// Video is interlaced
        bool						mMultiStream;			/// Demonstrates how to configure the board for multi-stream
        bool                        mWithInfo;              /// Demonstrates how to configure picture information mode
        bool                        mWithAnc;               /// Add timecode burn
		NTV2AudioSystem				mAudioSystem;			///	The audio system I'm using
		NTV2EveryFrameTaskMode		mSavedTaskMode;			/// Used to restore prior every-frame task mode
        uint32_t                    mNumAudioChannels;      /// Number of input audio channels
        uint32_t                    mFileAudioChannels;     /// Number of file audio channels
        uint32_t                    mMaxFrames;             /// Maximum number of frames to write to output files

		bool						mLastFrame;				///	Set "true" to signal last frame
		bool						mLastFrameInput;		///	Set "true" to signal last frame captured from input
		bool						mLastFrameRaw;			///	Set "true" to signal last frame transfered to codec
		bool						mLastFrameHevc;			///	Set "true" to signal last frame transfered from codec
        bool						mLastFrameVideo;		///	Set "true" to signal last frame of video written to disk
        bool						mLastFrameAudio;		///	Set "true" to signal last frame of audio written to disk
        bool						mGlobalQuit;			///	Set "true" to gracefully stop
		uint32_t					mQueueSize;				///	My queue size
        uint32_t					mVideoBufferSize;		///	My video buffer size (bytes)
        uint32_t                    mPicInfoBufferSize;     /// My picture info buffer size (bytes)
        uint32_t                    mEncInfoBufferSize;     /// My encoded info buffer size (bytes)
        uint32_t					mAudioBufferSize;		///	My audio buffer size (bytes)

        AVHevcDataBuffer                        mVideoInputBuffer [VIDEO_RING_SIZE];		///	My video input buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mVideoInputCircularBuffer;					///	My video input ring

        AVHevcDataBuffer                        mVideoRawBuffer [VIDEO_RING_SIZE];			///	My video raw buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mVideoRawCircularBuffer;					///	My video raw ring

        AVHevcDataBuffer                        mVideoHevcBuffer [VIDEO_RING_SIZE];			///	My video hevc buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mVideoHevcCircularBuffer;					///	My video hevc ring

        AVHevcDataBuffer						mAudioInputBuffer [AUDIO_RING_SIZE];		///	My audio input buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mAudioInputCircularBuffer;					///	My audio input ring

        uint32_t					mVideoInputFrameCount;                                  /// Input thread frame counter
        uint32_t					mVideoProcessFrameCount;                                /// Process thread frame counter
        uint32_t					mCodecRawFrameCount;                                    /// Raw thread frame counter
        uint32_t					mCodecHevcFrameCount;                                   /// HEVC thread frame counter
        uint32_t					mVideoFileFrameCount;                                   /// Video file thread frame counter
        uint32_t					mAudioFileFrameCount;                                   /// Audio file thread frame counter

        AJATimeBase                 mTimeBase;                                              /// Timebase for timecode string
        AJATimeCode                 mTimeCode;                                              /// Timecode string generator
        AJATimeCodeBurn             mTimeCodeBurn;                                          /// Timecode video burner

};	//	NTV2EncodeHEVCFileAc

#endif	//	_NTV2ENCODEHEVCFILEAC_H
