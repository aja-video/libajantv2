/* SPDX-License-Identifier: MIT */
/**
	@file	ntv2llburn.h
	@brief	Header file for the low latency NTV2Burn demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2LLBURN_H
#define _NTV2LLBURN_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ntv2utils.h"
#include "ajabase/common/types.h"
#include "ajabase/common/videotypes.h"
#include "ajabase/common/timecode.h"
#include "ajabase/common/timecodeburn.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include <set>


/**
	@brief	Captures video and audio from a signal provided to an input of an AJA device, burns timecode into the video frames,
			then plays the captured audio and altered video through an output on the same AJA device, all in real time, with
			minimal 3 frame latency. Because of the tight latency requirements, AutoCirculate and a ring buffer are not used.
**/

class NTV2LLBurn
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inConfig		Specifies the configuration parameters.
		**/
							NTV2LLBurn (const BurnConfig & inConfig);
		virtual				~NTV2LLBurn ();

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
			@param[out]	outFramesProcessed		Receives my processed frame count.
			@param[out]	outFramesDropped		Receives my dropped frame count.
		**/
		virtual void		GetStatus (ULWord & outFramesProcessed, ULWord & outFramesDropped);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need for capturing and playing video.
		**/
		virtual AJAStatus	SetupVideo (void);

		/**
			@brief	Sets up everything I need for capturing and playing audio.
		**/
		virtual AJAStatus	SetupAudio (void);

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
			@brief	Starts my main worker thread.
		**/
		virtual void		StartRunThread (void);

		/**
			@brief	Repeatedly captures, burns, and plays frames without using AutoCirculate (until global quit flag set).
		**/
		virtual void		ProcessFrames (void);

		/**
			@brief	Returns true if the current input signal has timecode embedded in it; otherwise returns false.
		**/
		virtual bool		InputSignalHasTimecode (void);

		/**
			@brief	Returns true if there is a valid LTC signal on my device's primary analog LTC input port; otherwise returns false.
		**/
		virtual bool		AnalogLTCInputHasTimecode (void);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the worker thread's static callback function that gets called when the thread runs.
					This function gets "Attached" to the worker thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the worker thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2LLBurn instance.)
		**/
		static void	RunThreadStatic (AJAThread * pThread, void * pContext);


	//	Private Member Data
	private:
		BurnConfig			mConfig;				///< @brief	My configuration info
		AJAThread			mRunThread;				///< @brief	My worker thread object
		CNTV2Card			mDevice;				///< @brief	My CNTV2Card instance
		NTV2DeviceID		mDeviceID;				///< @brief	Keep my device ID handy
		NTV2VideoFormat		mVideoFormat;			///< @brief	Format of video being ingested & played
		NTV2FormatDesc		mFormatDesc;			///< @brief	Describes raster images
		NTV2TaskMode		mSavedTaskMode;			///< @brief	For restoring prior state
		NTV2OutputDest		mOutputDest;			///< @brief	The desired output connector to use
		NTV2AudioSystem		mAudioSystem;			///< @brief	The audio system I'm using
		AJATimeCodeBurn		mTCBurner;				///< @brief	My timecode burner

		bool				mGlobalQuit;			///< @brief	Set "true" to gracefully stop
		NTV2ChannelSet		mRP188Outputs;			///< @brief	SDI outputs into which I'll inject timecode

		NTV2Buffer			mpHostVideoBuffer;		///< @brief My host video buffer for burning in the timecode
		NTV2Buffer			mpHostAudioBuffer;		///< @brief My host audio buffer for the samples matching the video buffer
		NTV2Buffer			mpHostF1AncBuffer;		///< @brief My host Anc buffer (F1)
		NTV2Buffer			mpHostF2AncBuffer;		///< @brief My host Anc buffer (F2)

		uint32_t			mAudioInLastAddress;	///< @brief My record of the location of the last audio sample captured
		uint32_t			mAudioOutLastAddress;	///< @brief My record of the location of the last audio sample played

		uint32_t			mFramesProcessed;		///< @brief My count of the number of burned frames produced
		uint32_t			mFramesDropped;			///< @brief My count of the number of dropped frames

		uint32_t			mInputStartFrame;	///< @brief The input starting frame to use
		uint32_t			mInputEndFrame;		///< @brief The input ending frame to use
		uint32_t			mOutputStartFrame;	///< @brief The output starting frame to use
		uint32_t			mOutputEndFrame;	///< @brief The output ending frame to use

};	//	NTV2LLBurn

#endif	//	_NTV2LLBURN_H
