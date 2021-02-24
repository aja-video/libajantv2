/* SPDX-License-Identifier: MIT */
/**
	@file	ntv2fieldburn.h
	@brief	Header file for the NTV2FieldBurn demonstration class.
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2FIELDBURN_H
#define _NTV2FIELDBURN_H

#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2democommon.h"
#include "ajabase/common/types.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"
#include "ajabase/common/timecodeburn.h"


/**
	@brief	Instances of me can capture frames from a video signal provided to an input of an AJA device.
			The frame is captured as two fields stored in non-contiguous system memory buffers.
			I burn timecode into those fields, then deliver both of them to an output of the same AJA device ... in real time.
			I make use of the AJACircularBuffer, which simplifies implementing a producer/consumer model,
			I use a "consumer" thread to deliver burned-in frames to the AJA device output, and a "producer"
			thread to capture raw frames from the AJA device input.
			I also demonstrate how to detect if an SDI input has embedded timecode, and if so, how AutoCirculate
			makes it available. I also show how to embed timecode into an SDI output signal using AutoCirculate
			during playout.
**/

class NTV2FieldBurn
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use. Defaults to "0", the first device found.
			@param[in]	inWithAudio			If true (the default), include audio in the output signal;  otherwise, omit it.
			@param[in]	inFieldMode			If true (the default), use AutoCirculate's "Field Mode" (introduced in SDK 15.1);
											otherwise use "Frame Mode".
			@param[in]	inPixelFormat		Specifies the pixel format to use for the device's frame buffers. Defaults to 8-bit YUV.
			@param[in]	inInputSource		Specifies which input to capture from. Defaults to SDI1.
			@param[in]	inDoMultiFormat		If true, use multi-format mode; otherwise use uniformat mode. Defaults to false (uniformat mode).
		**/
						NTV2FieldBurn (const std::string &			inDeviceSpecifier	= "0",
										const bool					inWithAudio			= true,
										const bool					inFieldMode			= true,
										const NTV2FrameBufferFormat	inPixelFormat		= NTV2_FBF_8BIT_YCBCR,
										const NTV2InputSource		inInputSource		= NTV2_INPUTSOURCE_SDI1,
										const bool					inDoMultiFormat		= false);
		virtual 		~NTV2FieldBurn ();

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
			@brief	Provides status information about my input (capture) and output (playout) processes.
			@param[out]	outNumProcessed		Receives the number of fields/frames successfully processed.
			@param[out]	outCaptureDrops		Receives the number of dropped capture fields/frames.
			@param[out]	outPlayoutDrops		Receives the number of dropped playout fields/frames.
			@param[out]	outCaptureLevel		Receives the capture driver buffer level.
			@param[out]	outPlayoutLevel		Receives the playout driver buffer level.
		**/
		virtual void			GetStatus (ULWord & outNumProcessed, ULWord & outCaptureDrops, ULWord & outPlayoutDrops,
											ULWord & outCaptureLevel, ULWord & outPlayoutLevel);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need for capturing and playing video.
		**/
		virtual AJAStatus		SetupVideo (void);

		/**
			@brief	Sets up everything I need for capturing and playing audio.
		**/
		virtual AJAStatus		SetupAudio (void);

		/**
			@brief	Sets up board routing for capture.
		**/
		virtual void			RouteInputSignal (void);

		/**
			@brief	Sets up board routing for playout.
		**/
		virtual void			RouteOutputSignal (void);

		/**
			@brief	Sets up my circular buffers.
		**/
		virtual AJAStatus		SetupHostBuffers (void);

		/**
			@brief	Starts my playout thread.
		**/
		virtual void			StartPlayThread (void);

		/**
			@brief	Repeatedly plays out frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			PlayFrames (void);

		/**
			@brief	Starts my capture thread.
		**/
		virtual void			StartCaptureThread (void);

		/**
			@brief	Repeatedly captures frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			CaptureFrames (void);


		/**
			@brief	Returns true if the current input signal has timecode embedded in it; otherwise returns false.
		**/
		virtual bool			InputSignalHasTimecode (void);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the playout thread's static callback function that gets called when the playout thread runs.
					This function gets "Attached" to the playout thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the playout thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2FieldBurn instance.)
		**/
		static void				PlayThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the capture thread's static callback function that gets called when the capture thread runs.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2FieldBurn instance.)
		**/
		static void				CaptureThreadStatic (AJAThread * pThread, void * pContext);

		typedef	AJACircularBuffer<NTV2FrameData*>	NTV2CircularBuffer;

	//	Private Member Data
	private:
		AJAThread					mPlayThread;		///< @brief	My playout thread object
		AJAThread					mCaptureThread;		///< @brief	My capture thread object
		CNTV2Card					mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;			///< @brief	My device identifier
		const std::string			mDeviceSpecifier;	///< @brief	Specifies which device I should use
		NTV2Channel					mInputChannel;		///< @brief	The input channel I'm using
		NTV2Channel					mOutputChannel;		///< @brief	The output channel I'm using
		NTV2InputSource				mInputSource;		///< @brief	The input source I'm using
		NTV2OutputDestination		mOutputDestination;	///< @brief	The output I'm using
		NTV2VideoFormat				mVideoFormat;		///< @brief	My video format
		NTV2FrameBufferFormat		mPixelFormat;		///< @brief	My pixel format
		NTV2FormatDescriptor		mFormatDescriptor;	///< @brief	Description of the board's frame geometry
		NTV2EveryFrameTaskMode		mSavedTaskMode;		///< @brief	We will restore the previous state
		NTV2VANCMode				mVancMode;			///< @brief	VANC mode
		NTV2AudioSystem				mAudioSystem;		///< @brief	The audio system I'm using
		const bool					mIsFieldMode;		///< @brief	True if Field Mode, false if Frame Mode
		bool						mGlobalQuit;		///< @brief	Set "true" to gracefully stop
		bool						mDoMultiChannel;	///< @brief	Set the board up for multi-format
		AJATimeCodeBurn				mTCBurner;			///< @brief	My timecode burner
		NTV2ChannelList				mTCOutputs;			///< @brief	My output timecode destinations
		NTV2FrameDataArray			mHostBuffers;		///< @brief	My host buffers
		NTV2CircularBuffer			mAVCircularBuffer;	///< @brief	My ring buffer object

};	//	NTV2FieldBurn

#endif	//	_NTV2FIELDBURN_H
