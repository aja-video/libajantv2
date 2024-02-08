/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2encodehevcvif.h
	@brief		Declares the NTV2EncodeHEVCVif class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2ENCODEHEVCVIF_H
#define _NTV2ENCODEHEVCVIF_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2demohevccommon.h"
//#include "ajacircularbuffer.h"

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

class NTV2EncodeHEVCVif
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given settings.
			@note	I'm not completely initialized and ready to use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use.
											Defaults to "0" (first device found).
			@param[in]	inM31Preset			Specifies the m31 preset to use.
											Defaults to 8-bit 1280x720 5994p.
            @param[in]	inPixelFormat		Specifies the pixel format to use.
                                            Defaults to NTV2_FBF_10BIT_YCBCR_420PL2.
            @param[in]	inAudioChannels		Specifies number of audio channels to write to AIFF file.
                                            Defaults to 2 channels.
            @param[in]	inInfoData          Use picture and encoded information.
                                            Defaults to no info data.
            @param[in]	inMaxFrames			Specifies the maximum number of frames to capture.
            								Defaults to 0xFFFFFFFF (unlimited).
        **/
		NTV2EncodeHEVCVif (	const std::string			inDeviceSpecifier	= "0",
							const M31VideoPreset		inM31Preset			= M31_FILE_1280X720_420_8_5994p,
                            const NTV2FrameBufferFormat	inPixelFormat		= NTV2_FBF_10BIT_YCBCR_420PL2,
							const uint32_t              inAudioChannels     = 0,
							const bool                  inInfoData          = false,
							const uint32_t              inMaxFrames         = 0xffffffff);

        virtual					~NTV2EncodeHEVCVif ();

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
			@param[out]	outInputStatus		Receives status information about my input (capture) process.
		**/
        virtual void            GetStatus (AVHevcStatus * outInputStatus);

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
			@brief	Start the codec hevc thread.  
		**/
        virtual void			StartCodecHevcThread (void);

		/**
            @brief	Start the audio/video file writer thread.
		**/
        virtual void			StartAVFileThread (void);

        /**
            @brief	Repeatedly captures video frames using AutoCirculate and add them to
					the video input ring.
        **/
        virtual void			VideoInputWorker (void);

        /**
            @brief	Repeatedly transfers hevc frames from the codec and adds them to the
					hevc ring.
        **/
        virtual void			CodecHevcWorker (void);

        /**
            @brief	Repeatedly removes hevc frame from the hevc ring and writes them to the
					hevc output file.
        **/
        virtual void			AVFileWorker (void);

		/**
            @brief	Transfer picture information to the codec.
			@param[in]	pM31		Points to a CNTV2m31 instance.
		**/
		virtual void			TransferPictureInfo(CNTV2m31 *	pM31);

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
        static void				AVFileThreadStatic (AJAThread * pThread, void * pContext);
		
    //	Private Member Data
	private:
        AJAThread					mACInputThread;         ///	AutoCirculate input thread
        AJAThread					mCodecHevcThread;		///	Codec hevc transfer thread
        AJAThread					mAVFileThread;			///	Audio/Video file writer thread
        CNTV2m31 *					mM31;					/// Object used to interface to m31
        CNTV2DemoHevcCommon *       mHevcCommon;            /// HEVC common class

        CNTV2Card					mDevice;				///	CNTV2Card instance
        NTV2DeviceID				mDeviceID;				///	Device identifier
		const std::string			mDeviceSpecifier;		///	The device specifier string
        bool        				mWithAudio;				///	Capture and playout audio?
        NTV2Channel                 mInputChannel;			///	Input channel
        NTV2Channel                 mOutputChannel;			///	Output channel
        M31Channel                  mEncodeChannel;         /// Encoder channel
		M31VideoPreset				mPreset;				/// M31 HEVC Preset 
		NTV2InputSource				mInputSource;			///	The input source I'm using
        NTV2VideoFormat				mInputFormat;			///	Input format
        NTV2VideoFormat				mVideoFormat;			///	Video format
        NTV2FrameBufferFormat		mCapturePixelFormat;	///	Capture Pixel format
        NTV2FrameBufferFormat		mOverlayPixelFormat;	///	Overlay Pixel format
        NTV2FrameBufferFormat		mCodecPixelFormat;		///	Codec Pixel format
		NTV2FrameRate 				mFrameRate;				/// Video frame rate
		NTV2AudioRate 				mAudioRate;				/// Audio sample rate
        bool                        mWithInfo;              /// Demonstrates how to configure picture information mode
		NTV2AudioSystem				mAudioSystem;			///	The audio system I'm using
		NTV2EveryFrameTaskMode		mSavedTaskMode;			/// Used to restore prior every-frame task mode
        uint32_t                    mNumAudioChannels;      /// Number of input audio channels
        uint32_t                    mFileAudioChannels;     /// Number of file audio channels
        uint32_t                    mMaxFrames;             /// Maximum number of frames to write to output files

		bool						mLastFrame;				///	Set "true" to signal last frame
		bool						mLastFrameInput;		///	Set "true" to signal last frame captured from input
		bool						mLastFrameHevc;			///	Set "true" to signal last frame transfered from codec
        bool						mLastFrameVideo;		///	Set "true" to signal last frame of audio/video written to disk
        bool						mGlobalQuit;			///	Set "true" to gracefully stop
		uint32_t					mQueueSize;				///	My queue size
        uint32_t					mVideoBufferSize;		///	My video buffer size (bytes)
        uint32_t                    mPicInfoBufferSize;     /// My picture info buffer size (bytes)
        uint32_t                    mEncInfoBufferSize;     /// My encoded info buffer size (bytes)
        uint32_t					mAudioBufferSize;		///	My audio buffer size (bytes)
        uint32_t					mOverlayBufferSize;		///	My overlay buffer size (bytes)

        AVHevcDataBuffer                        mACInputBuffer [VIDEO_RING_SIZE];           ///	My AC input buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mACInputCircularBuffer;                     ///	My AC input ring

        AVHevcDataBuffer                        mVideoHevcBuffer [VIDEO_RING_SIZE];			///	My video hevc buffers
        AJACircularBuffer <AVHevcDataBuffer *>  mVideoHevcCircularBuffer;					///	My video hevc ring

		AVHevcDataBuffer *			mFrameData;												/// Input frame
		uint32_t *					mSilentBuffer;											/// Audio silent buffer
        uint32_t					mVideoInputFrameCount;									/// Input thread frame counter
        uint32_t					mCodecHevcFrameCount;									/// HEVC thread frame counter
        uint32_t					mAVFileFrameCount;										/// Audio/Video file thread frame counter
		uint32_t					mRawFrameCount;											/// Raw frame count
        uint32_t					mInfoFrameCount;										/// Picture info frame counter

		uint32_t *					mOverlayBuffer [2];										/// Overlay video buffers
		uint32_t					mOverlayFrame [2];										/// Overlay video frames
		uint32_t					mOverlayIndex;											/// Overlay buffer index

        AJATimeBase                 mTimeBase;                                              /// Timebase for picture info

};	//	NTV2EncodeHEVCVIF

#endif	//	_NTV2ENCODEHEVCVIF_H
