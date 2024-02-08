/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2encodehevcfile.h
	@brief		Declares the NTV2EncodeHEVCFile class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2ENCODEHEVCFILE_H
#define _NTV2ENCODEHEVCFILE_H

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

#define RAWFILEPATH             "ElephantsDream.yuv"
#define RAWFILEWIDTH            1920
#define RAWFILEHEIGHT           1080
#define M31PRESET               M31_FILE_1920X1080_420_8_24p

/**
	@brief	Instances of me capture frames in real time from a video signal provided to an input of
			an AJA device.
**/

class NTV2EncodeHEVCFile
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
            @param[in]	inFilePath			Specifies the path to the file to be written.
            @param[in]	inFrameWidth		Specifies the frame width to use, in pixels.
            @param[in]	inFrameHeight		Specifies the frame height to use, in lines.
			@param[in]	inM31Preset			Specifies the m31 preset to use.
											Defaults to 8-bit 1280x720 5994p.
        **/
		NTV2EncodeHEVCFile (const std::string			inDeviceSpecifier	= "0",
							const NTV2Channel			inChannel			= NTV2_CHANNEL1,
                            const std::string           inFilePath          = "",
                            const uint32_t              inFrameWidth        = 0,
                            const uint32_t              inFrameHeight       = 0,
							const M31VideoPreset		inM31Preset			= M31PRESET);

        virtual					~NTV2EncodeHEVCFile ();

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
			@brief	Sets up my circular buffers.
		**/
		virtual void			SetupHostBuffers (void);

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
			@brief	Default do-nothing function for processing the captured frames.
		**/
        virtual AJAStatus		ProcessVideoFrame (AVHevcDataBuffer * pSrcFrame, AVHevcDataBuffer * pDstFrame);

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


    //	Private Member Data
	private:
        AJAThread					mFileInputThread;       ///	File reader input thread
        AJAThread					mVideoProcessThread;	///	Video processing thread
        AJAThread					mCodecRawThread;		///	Codec raw transfer
        AJAThread					mCodecHevcThread;		///	Codec hevc transfer thread
        AJAThread					mVideoFileThread;		///	Video file writer thread
        CNTV2m31 *					mM31;					/// Object used to interface to m31
        CNTV2DemoHevcCommon *       mHevcCommon;            /// HEVC common class

        CNTV2Card					mDevice;				///	CNTV2Card instance
        NTV2DeviceID				mDeviceID;				///	Device identifier
		const std::string			mDeviceSpecifier;		///	The device specifier string
        const std::string			mFileName;              /// The RAW file name
        uint32_t					mFrameWidth;            ///	RAW frame width
        uint32_t					mFrameHeight;           ///	RAW frame height

        NTV2Channel                 mInputChannel;			///	Input channel
        M31Channel                  mEncodeChannel;         /// Encoder channel
		M31VideoPreset				mPreset;				/// M31 HEVC Preset 
        NTV2VideoFormat				mVideoFormat;			///	Video format
        NTV2FrameBufferFormat		mPixelFormat;			///	Pixel format
		bool						mQuad;					/// VideoFormat is quad
		bool						mInterlaced;			/// Video is interlaced
        bool						mMultiStream;			/// Demonstrates how to configure the board for multi-stream
		NTV2EveryFrameTaskMode		mSavedTaskMode;			/// Used to restore prior every-frame task mode

		bool						mLastFrame;				///	Set "true" to signal last frame
		bool						mLastFrameInput;		///	Set "true" to signal last frame captured from input
		bool						mLastFrameRaw;			///	Set "true" to signal last frame transfered to codec
		bool						mLastFrameHevc;			///	Set "true" to signal last frame transfered from codec
        bool						mLastFrameVideo;		///	Set "true" to signal last frame of video written to disk
        bool						mGlobalQuit;			///	Set "true" to gracefully stop
		uint32_t					mQueueSize;				///	My queue size
        uint32_t					mVideoBufferSize;		///	My video buffer size (bytes)
        uint32_t                    mPicInfoBufferSize;     /// My picture info buffer size (bytes)
        uint32_t                    mEncInfoBufferSize;     /// My encoded info buffer size (bytes)

        AVHevcDataBuffer                        mFileInputBuffer [VIDEO_RING_SIZE];         ///	My file input buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mFileInputCircularBuffer;                   ///	My file input ring

        AVHevcDataBuffer                        mVideoRawBuffer [VIDEO_RING_SIZE];			///	My video raw buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mVideoRawCircularBuffer;					///	My video raw ring

        AVHevcDataBuffer                        mVideoHevcBuffer [VIDEO_RING_SIZE];			///	My video hevc buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mVideoHevcCircularBuffer;					///	My video hevc ring
	
        uint32_t					mVideoInputFrameCount;                                  /// Input thread frame counter
        uint32_t					mVideoProcessFrameCount;                                /// Process thread frame counter
        uint32_t					mCodecRawFrameCount;                                    /// Raw thread frame counter
        uint32_t					mCodecHevcFrameCount;                                   /// HEVC thread frame counter
        uint32_t					mVideoFileFrameCount;                                   /// Video file thread frame counter

        AJATimeBase                 mTimeBase;                                              /// Timebase for timecode string
        AJATimeCode                 mTimeCode;                                              /// Timecode string generator
        AJATimeCodeBurn             mTimeCodeBurn;                                          /// Timecode video burner

};	//	NTV2EncodeHEVCFile

#endif	//	_NTV2ENCODEHEVCFILE_H
