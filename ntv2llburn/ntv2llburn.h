/* SPDX-License-Identifier: MIT */
/**
	@file	ntv2llburn.h
	@brief	Header file for the low latency NTV2Burn demonstration class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
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
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use. Defaults to "0", the first device found.
			@param[in]	inWithAudio			If true, include audio in the output signal; otherwise, omit it.
											Defaults to "true".
			@param[in]	inPixelFormat		Specifies the pixel format to use for the device's frame buffers. Defaults to 8-bit YUV.
			@param[in]	inInputSource		Specifies which input to capture video from. Defaults to SDI1.
			@param[in]	inTCIndex			Specifies the timecode of interest. Defaults to whatever is found embedded in the input video.
			@param[in]	inDoMultiChannel	If true, enables multichannel mode (if the device supports it), and won't acquire
											or release the device. If false (the default), acquires/releases exclusive use of the device.
			@param[in]	inWithAnc			If true, capture & play ancillary data. Defaults to false.
			@param[in]	inWithHanc			If true, capture & play HANC data. Defaults to false.
		**/
							NTV2LLBurn (const std::string &			inDeviceSpecifier	= "0",
										const bool					inWithAudio			= true,
										const NTV2FrameBufferFormat	inPixelFormat		= NTV2_FBF_8BIT_YCBCR,
										const NTV2InputSource		inInputSource		= NTV2_INPUTSOURCE_SDI1,
										const NTV2TCIndex			inTCIndex			= NTV2_TCINDEX_SDI1,
										const bool					inDoMultiChannel	= false,
										const bool					inWithAnc			= false,
										const bool					inWithHanc			= false);
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
		AJAThread				mRunThread;				///< @brief	My worker thread object

		CNTV2Card				mDevice;				///< @brief	My CNTV2Card instance
		NTV2DeviceID			mDeviceID;				///< @brief	My device identifier
		const std::string		mDeviceSpecifier;		///< @brief	Specifies the device I should use
		bool					mWithAudio;				///< @brief	Capture and playout audio?
		NTV2Channel				mInputChannel;			///< @brief	The input channel I'm using
		NTV2Channel				mOutputChannel;			///< @brief	The output channel I'm using
		NTV2InputSource			mInputSource;			///< @brief	The input source I'm using
		NTV2TCIndex				mTimecodeIndex;			///< @brief	The timecode of interest
		NTV2OutputDestination	mOutputDestination;		///< @brief	The output I'm using
		NTV2VideoFormat			mVideoFormat;			///< @brief	My video format
		NTV2PixelFormat			mPixelFormat;			///< @brief	My pixel format
		NTV2TaskMode			mSavedTaskMode;			///< @brief	Previous task mode to restore
		NTV2VANCMode			mVancMode;				///< @brief	VANC mode
		NTV2AudioSystem			mAudioSystem;			///< @brief	The audio system I'm using
		bool					mDoMultiChannel;		///< @brief	Set the board up for multi-format
		bool					mWithAnc;				///< @brief	Capture and Playout packetized ANC data
		bool					mWithHanc;				///< @brief	Capture and Playout packetized ANC data with audio

		bool					mGlobalQuit;			///< @brief	Set "true" to gracefully stop
		AJATimeCodeBurn			mTCBurner;				///< @brief	My timecode burner
		NTV2ChannelSet			mRP188Outputs;			///< @brief	SDI outputs into which I'll inject timecode

		NTV2_POINTER			mpHostVideoBuffer;		///< @brief My host video buffer for burning in the timecode
		NTV2_POINTER			mpHostAudioBuffer;		///< @brief My host audio buffer for the samples matching the video buffer
		NTV2_POINTER			mpHostF1AncBuffer;		///< @brief My host Anc buffer (F1)
		NTV2_POINTER			mpHostF2AncBuffer;		///< @brief My host Anc buffer (F2)

		uint32_t				mAudioInLastAddress;	///< @brief My record of the location of the last audio sample captured
		uint32_t				mAudioOutLastAddress;	///< @brief My record of the location of the last audio sample played

		uint32_t				mFramesProcessed;		///< @brief My count of the number of burned frames produced
		uint32_t				mFramesDropped;			///< @brief My count of the number of dropped frames

};	//	NTV2LLBurn

#endif	//	_NTV2LLBURN_H
