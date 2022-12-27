/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2capture8k.h
	@brief		Declares the NTV2Capture class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2CAPTURE_H
#define _NTV2CAPTURE_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ajabase/common/videotypes.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"


/**
	@brief	Instances of me capture frames in real time from a video signal provided to an input of an AJA device.
**/

class NTV2Capture8K
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given settings.
			@note	I'm not completely initialized and ready to use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use.  Defaults to "0" (first device found).
			@param[in]	inWithAudio			If true (the default), capture audio in addition to video;  otherwise, don't capture audio.
			@param[in]	inChannel			Specifies the channel to use. Defaults to NTV2_CHANNEL1.
			@param[in]	inPixelFormat		Specifies the pixel format to use for the device's frame buffers. Defaults to 8-bit YUV.
			@param[in]	inDoLvlABConversion	Specifies if level-A/B conversion should be done or not.  Defaults to false (no conversion).
			@param[in]	inMultiFormat		If true, enables multiformat/multichannel mode if the device supports it, and won't acquire
											or release the device. If false (the default), acquires/releases exclusive use of the device.
			@param[in]	inWithAnc			If true, captures ancillary data using the new AutoCirculate APIs (if the device supports it).
											Defaults to false.
			@param[in]	inDoTsiRouting		If true, routes for TSI;  otherwise routes for quad-quad.  Defaults to false.
		**/
		NTV2Capture8K (	const std::string			inDeviceSpecifier	= "0",
						const bool					inWithAudio			= true,
						const NTV2Channel			inChannel			= NTV2_CHANNEL1,
						const NTV2FrameBufferFormat	inPixelFormat		= NTV2_FBF_8BIT_YCBCR,
						const bool					inDoLvlABConversion	= false,
						const bool					inMultiFormat		= false,
                        const bool					inWithAnc			= false,
                        const bool					inDoTsiRouting		= false);

		virtual						~NTV2Capture8K ();

		/**
			@brief	Initializes me and prepares me to Run.
		**/
		virtual AJAStatus			Init (void);

		/**
			@brief	Runs me.
			@note	Do not call this method without first calling my Init method.
		**/
		virtual AJAStatus			Run (void);

		/**
			@brief	Gracefully stops me from running.
		**/
		virtual void				Quit (void);

		/**
			@brief	Provides status information about my input (capture) process.
			@param[out]	outGoodFrames		Receives the number of successfully captured frames.
			@param[out]	outDroppedFrames	Receives the number of dropped frames.
			@param[out]	outBufferLevel		Receives the buffer level (number of captured frames ready to be transferred to the host).
		**/
		virtual void				GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel);


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
		virtual void			SetupInputAutoCirculate (void);

		/**
			@brief	Starts my capture thread.
		**/
		virtual void			StartProducerThread (void);

		/**
			@brief	Repeatedly captures frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			CaptureFrames (void);

		/**
			@brief	Starts my frame consumer thread.
		**/
		virtual void			StartConsumerThread (void);

		/**
			@brief	Repeatedly consumes frames from the circular buffer (until global quit flag set).
		**/
		virtual void			ConsumeFrames (void);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the consumer thread's static callback function that gets called when the consumer thread runs.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will point to the NTV2Capture instance.)
		**/
		static void	ConsumerThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the capture thread's static callback function that gets called when the capture thread runs.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will point to the NTV2Capture instance.)
		**/
		static void	ProducerThreadStatic (AJAThread * pThread, void * pContext);


	//	Private Member Data
	private:
		typedef	AJACircularBuffer <AVDataBuffer *>	MyCircularBuffer;

		AJAThread					mConsumerThread;		///< @brief	My consumer thread object -- consumes the captured frames.
		AJAThread					mProducerThread;		///< @brief	My producer thread object -- does the frame capturing
		CNTV2Card					mDevice;				///< @brief	My CNTV2Card instance. This is what I use to talk to the device.
		NTV2DeviceID				mDeviceID;				///< @brief	My device identifier
		const std::string			mDeviceSpecifier;		///< @brief	The device specifier string
		const bool					mWithAudio;				///< @brief	Capture and playout audio?
		NTV2Channel					mInputChannel;			///< @brief	My input channel
		NTV2InputSource				mInputSource;			///< @brief	The input source I'm using
		NTV2VideoFormat				mVideoFormat;			///< @brief	My video format
		NTV2FrameBufferFormat		mPixelFormat;			///< @brief	My pixel format
		NTV2EveryFrameTaskMode		mSavedTaskMode;			///< @brief	Used to restore prior every-frame task mode
		NTV2AudioSystem				mAudioSystem;			///< @brief	The audio system I'm using
		bool						mDoLevelConversion;		///< @brief	Demonstrates a level A to level B conversion
		bool						mDoMultiFormat;			///< @brief	Demonstrates how to configure the board for multi-format
		bool						mGlobalQuit;			///< @brief	Set "true" to gracefully stop
		bool						mWithAnc;				///< @brief	Capture custom anc data?
        uint32_t					mVideoBufferSize;		///< @brief	My video buffer size, in bytes
		uint32_t					mAudioBufferSize;		///< @brief	My audio buffer size, in bytes
        bool						mDoTsiRouting;          ///< @brief	Input is TSI
        uint32_t					mAncBufferSize;
		AVDataBuffer				mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///< @brief	My host buffers
		MyCircularBuffer			mAVCircularBuffer;		///< @brief	My ring buffer object

		AUTOCIRCULATE_TRANSFER		mInputTransfer;			///< @brief	My A/C input transfer info

};	//	NTV2Capture

#endif	//	_NTV2CAPTURE_H
