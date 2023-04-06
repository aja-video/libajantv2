/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn4kquadrant.h
	@brief		Header file for the NTV2Burn4KQuadrant demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2BURN4KQUADRANT_H
#define _NTV2BURN4KQUADRANT_H

#include "ntv2card.h"
//#include "ntv2devicefeatures.h"
//#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
//#include "ntv2task.h"
//#include "ntv2utils.h"
//#include "ntv2rp188.h"
//#include "ajabase/common/types.h"
//#include "ajabase/common/videotypes.h"
//#include "ajabase/common/timecode.h"
#include "ajabase/common/timecodeburn.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"
//#include "ajabase/system/process.h"
//#include "ajabase/system/systemtime.h"


/**
	@brief	Instances of me can capture 4K/UHD video from one 4-channel AJA device, burn timecode into one quadrant,
			then play it to a second 4-channel AJA device ... in real time.
**/

class NTV2Burn4KQuadrant
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@param[in]	inConfig		Specifies the configuration parameters.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
		**/
							NTV2Burn4KQuadrant (const BurnConfig & inConfig);
		virtual				~NTV2Burn4KQuadrant ();

		/**
			@brief	Initializes me and prepares me to Run.
		**/
		virtual AJAStatus	Init (void);

		/**
			@brief	Runs me.
			@note	Do not call this method without first calling my Init method.
		**/
		virtual AJAStatus	Run (void);

		/**
			@brief	Gracefully stops me from running.
		**/
		virtual void		Quit (void);

		/**
			@brief	Provides status information about my input (capture) and output (playout) processes.
			@param[out]	outInputStatus		Receives status information about my input (capture) process.
			@param[out]	outOutputStatus		Receives status information about my output (playout) process.
		**/
		virtual void		GetACStatus (AUTOCIRCULATE_STATUS & outInputStatus, AUTOCIRCULATE_STATUS & outOutputStatus);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need for capturing 4K video.
		**/
		virtual AJAStatus	SetupInputVideo (void);

		/**
			@brief	Sets up everything I need for playing 4K video.
		**/
		virtual AJAStatus	SetupOutputVideo (void);

		/**
			@brief	Sets up everything I need for capturing audio.
		**/
		virtual AJAStatus	SetupInputAudio (void);

		/**
			@brief	Sets up everything I need for playing audio.
		**/
		virtual AJAStatus	SetupOutputAudio (void);

		/**
			@brief	Sets up board routing for capture.
		**/
		virtual void		RouteInputSignal (void);

		/**
			@brief	Sets up board routing for playout.
		**/
		virtual void		RouteOutputSignal (void);

		/**
			@brief	Sets up my circular buffers.
		**/
		virtual AJAStatus	SetupHostBuffers (void);

		/**
			@brief	Starts my playout thread.
		**/
		virtual void		StartPlayThread (void);

		/**
			@brief	Repeatedly plays out frames using AutoCirculate (until global quit flag set).
		**/
		virtual void		PlayFrames (void);

		/**
			@brief	Starts my capture thread.
		**/
		virtual void		StartCaptureThread (void);

		/**
			@brief	Repeatedly captures frames using AutoCirculate (until global quit flag set).
		**/
		virtual void		CaptureFrames (void);


		/**
			@brief	Returns true if the current input signal has timecode embedded in it; otherwise returns false.
		**/
		virtual bool		InputSignalHasTimecode (void);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the playout thread's static callback function that gets called when the playout thread runs.
					This function gets "Attached" to the playout thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the playout thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Burn4KQuadrant instance.)
		**/
		static void	PlayThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the capture thread's static callback function that gets called when the capture thread runs.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Burn4KQuadrant instance.)
		**/
		static void	CaptureThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	Returns the RP188 DBB register number to use for the given NTV2InputSource.
			@param[in]	inInputSource	Specifies the NTV2InputSource of interest.
			@return	The number of the RP188 DBB register to use for the given input source.
		**/
		static ULWord			GetRP188RegisterForInput (const NTV2InputSource inInputSource);

	//	Private Member Data
	private:
		typedef AJACircularBuffer<NTV2FrameData*>	CircularBuffer;
		BurnConfig			mConfig;				///< @brief	My configuration info
		AJAThread			mPlayThread;			///< @brief	My playout thread object
		AJAThread			mCaptureThread;			///< @brief	My capture thread object
		CNTV2Card			mInputDevice;			///< @brief	My CNTV2Card input instance
		CNTV2Card			mOutputDevice;			///< @brief	My CNTV2Card output instance
		NTV2DeviceID		mInputDeviceID;			///< @brief	My device identifier
		NTV2DeviceID		mOutputDeviceID;		///< @brief	My device identifier
		NTV2VideoFormat		mVideoFormat;			///< @brief	My video format
		NTV2TaskMode		mInputSavedTaskMode;	///< @brief	For restoring the input device's prior task mode
		NTV2TaskMode		mOutputSavedTaskMode;	///< @brief	For restoring the output device's prior task mode
		NTV2AudioSystem		mInputAudioSystem;		///< @brief	The input audio system I'm using
		NTV2AudioSystem		mOutputAudioSystem;		///< @brief	The output audio system I'm using
		bool				mSingleDevice;			///< @brief	Using single 8-channel device (4K I/O on one device)?
		bool				mGlobalQuit;			///< @brief	Set "true" to gracefully stop
		AJATimeCodeBurn		mTCBurner;				///< @brief	My timecode burner
		uint32_t			mQueueSize;				///< @brief	My queue size
		NTV2FormatDesc		mFormatDesc;		///< @brief	Describes raster images
		NTV2FrameDataArray	mHostBuffers;			///< @brief	My host buffers
		CircularBuffer		mFrameDataRing;			///< @brief	AJACircularBuffer that controls frame data access by producer/consumer threads

};	//	NTV2Burn4KQuadrant

#endif	//	_NTV2BURN4KQUADRANT_H
