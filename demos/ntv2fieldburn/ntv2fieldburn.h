/* SPDX-License-Identifier: MIT */
/**
	@file	ntv2fieldburn.h
	@brief	Header file for the NTV2FieldBurn demonstration class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2FIELDBURN_H
#define _NTV2FIELDBURN_H

#include "ntv2card.h"
#include "ntv2formatdescriptor.h"
#include "ntv2democommon.h"
#include "ajabase/common/types.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"
#include "ajabase/common/timecodeburn.h"


/**
	@brief	I capture individual fields from an interlaced video signal provided to an SDI input. Each frame is
			captured as two fields in separate host buffers. I burn F1 timecode into the top of F1, and F2 timecode
			into the bottom half of F2. Then I reassemble both fields and play them through an output of the same
			device, with a 7-frame latency (by default).
			I make use of the AJACircularBuffer, which simplifies implementing a producer/consumer model,
			in which a "consumer" thread delivers burned-in frames to the AJA device output, and a "producer"
			thread captures raw frames from the AJA device input.
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
			@param[in]	inConfig		Specifies the configuration parameters.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
		**/
						NTV2FieldBurn (const BurnConfig & inConfig);
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

	//	Private Member Data
	private:
		typedef AJACircularBuffer<NTV2FrameData*>	CircularBuffer;
		BurnConfig			mConfig;			///< @brief	My configuration info
		AJAThread			mPlayThread;		///< @brief	My playout thread object
		AJAThread			mCaptureThread;		///< @brief	My capture thread object
		CNTV2Card			mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID		mDeviceID;			///< @brief	Keep my device ID handy
		NTV2VideoFormat		mVideoFormat;		///< @brief	Format of video being ingested & played
		NTV2FormatDesc		mFormatDesc;		///< @brief	Describes raster images
		NTV2TaskMode		mSavedTaskMode;		///< @brief	For restoring prior state
		NTV2OutputDest		mOutputDest;		///< @brief	The desired output connector to use
		NTV2AudioSystem		mAudioSystem;		///< @brief	The audio system I'm using
		AJATimeCodeBurn		mTCBurner;			///< @brief	My timecode burner
		NTV2ChannelList		mTCOutputs;			///< @brief	My output timecode destinations
		NTV2FrameDataArray	mHostBuffers;		///< @brief	My host buffers
		CircularBuffer		mFrameDataRing;		///< @brief	AJACircularBuffer that controls frame data access by producer/consumer threads
		bool				mGlobalQuit;		///< @brief	Set "true" to gracefully stop

};	//	NTV2FieldBurn

#endif	//	_NTV2FIELDBURN_H
