/**
	@file	qaburn.h
	@brief	Header file for the QABurn demonstration class.
	@copyright	Copyright (C) 2012-2014 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _QABURN_H
#define _QABURN_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "../demoapps/ntv2democommon.h"
#include "ntv2task.h"
#include "ntv2utils.h"
#include "ajabase/common/types.h"
#include "ajabase/common/videotypes.h"
#include "ajabase/common/timecode.h"
#include "ajabase/common/timecodeburn.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"


/**
	@brief	Instances of me can capture frames from a video signal provided to an input of an AJA device,
			burn timecode into those frames, then deliver them to an output of the same AJA device, all in real time.
			I make use of the AJACircularBuffer, which simplifies implementing a producer/consumer model,
			in which a "consumer" thread delivers burned-in frames to the AJA device output, and a "producer"
			thread grabs raw frames from the AJA device input.
			I also demonstrate how to detect if an SDI input has embedded timecode, and if so, how AutoCirculate
			makes it available. I also show how to embed timecode into an SDI output signal using AutoCirculate
			during playout.
**/

class QABurn
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use. Defaults to zero, the first device found.
			@param[in]	inInputSource		Specifies which input to capture video from. Defaults to SDI1.
			@param[in]	inAudioSystem		Specifies the audio system to use. Defaults to audio system 1.
											Use NTV2_AUDIOSYSTEM_INVALID to disable audio.
			@param[in]	inPixelFormat		Specifies the pixel format to use for the device's frame buffers. Defaults to 8-bit YUV.
			@param[in]	inWithBurn			If true, burns-in timecode. Defaults to true.
			@param[in]	inDoLevelConversion	If true, uses 3Ga->3Gb converters. Defaults to false.
			@param[in]	inUseDualLink		If true, uses DL RGB conversion. Defaults to false.
		**/
						QABurn (const std::string &			inDeviceSpecifier	= "0",
								const NTV2InputSource		inInputSource		= NTV2_INPUTSOURCE_SDI1,
								const NTV2AudioSystem		inAudioSystem		= NTV2_AUDIOSYSTEM_1,
								const NTV2FrameBufferFormat	inPixelFormat		= NTV2_FBF_8BIT_YCBCR,
								const bool					inWithBurn			= true,
								const bool					inDoLevelConversion	= false,
								const bool					inUseDualLink		= false);

		virtual				~QABurn();
		virtual AJAStatus	Init (void);				/// @brief	Prepares me to Run().
		virtual AJAStatus	Run (void);					/// @brief	Runs me. Be sure you've called Init() first.
		virtual void		Quit (void);				/// @brief	Gracefully stops me.

		/**
			@brief	Provides status information about my input (capture) and output (playout) processes.
			@param[out]	outInputFramesProcessed		Receives my current input processed frame count.
			@param[out]	outInputFramesDropped		Receives my current input dropped frame count.
			@param[out]	outInputBufferLevel			Receives my current input buffer level.
			@param[out]	outOutputFramesDropped		Receives my current output dropped frame count.
			@param[out]	outOutputBufferLevel		Receives my current output buffer level.
		**/
		virtual void		GetACStatus (ULWord & outInputFramesProcessed, ULWord & outInputFramesDropped, ULWord & outInputBufferLevel, ULWord & outOutputFramesDropped, ULWord & outOutputBufferLevel);


	//	Protected Instance Methods
	protected:
		virtual AJAStatus	SetupInputVideo (void);										/// @brief	Sets up everything I need for capturing video.
		virtual AJAStatus	SetupOutputVideo (const NTV2VideoFormat inVideoFormat);		/// @brief	Sets up everything I need for playing video.
		virtual AJAStatus	SetupAudio (void);											/// @brief	Sets up everything I need for capturing and playing audio.
		virtual void		RouteInputSignal (void);									/// @brief	Sets up device routing for capture.
		virtual void		RouteOutputSignal (const NTV2VideoFormat inVideoFormat);	/// @brief	Sets up device routing for playout.
		virtual AJAStatus	SetupHostBuffers (const NTV2VideoFormat inVideoFormat);		/// @brief	Sets up my circular buffers.
		virtual void		ReleaseHostBuffers (void);									/// @brief	Releases all my buffers.
		virtual void		StartPlayThread (void);										/// @brief	Starts my playout thread.
		virtual void		PlayFrames (void);											/// @brief	Repeatedly plays frames til quitting time.
		virtual void		StartCaptureThread (void);									/// @brief	Starts my capture thread.
		virtual void		CaptureFrames (void);										/// @brief	Repeatedly captures frames til quitting time.
		virtual bool		InputSignalHasRP188BypassEnabled (void);					/// @return	True if the current input's RP188 bypass is enabled.
		virtual bool		InputSignalHasTimecode (void);								/// @return	True if the current input is carrying RP188.
		virtual bool		AnalogLTCInputHasTimecode (void);							/// @return	True if the primary analog LTC input port has a valid LTC signal.
		virtual void		DisableRP188Bypass (void);									/// @brief	Disables the current input's RP188 bypass.


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the playout thread's static callback function that gets called when the playout thread runs.
					This function gets "Attached" to the playout thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the playout thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Burn instance.)
		**/
		static void	PlayThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the capture thread's static callback function that gets called when the capture thread runs.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Burn instance.)
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
		AJAThread *					mPlayThread;			///	My playout thread object
		AJAThread *					mCaptureThread;			///	My capture thread object
		AJALock *					mLock;					///	My mutex object
		CNTV2Card					mDevice;				///	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;				///	My device identifier
		const std::string			mDeviceSpecifier;		///	My device specifier
		const bool					mWithBurn;				///	Burn TC into frame?
		NTV2InputSource				mInputSource;			///	My video input source
		NTV2Channel					mInputChannel;			///	The input channel I'm using
		NTV2Channel					mOutputChannel;			///	The output channel I'm using
		NTV2OutputDestination		mOutputDestination;		///	The output I'm using
		NTV2FrameBufferFormat		mPixelFormat;			///	My pixel format
		const NTV2VANCMode			mVancMode;				///	My VANC mode
		NTV2AudioSystem				mAudioSystem;			///	The audio system I'm using (NTV2_AUDIOSYSTEM_INVALID == no audio)
		bool						mDoLevelConversion;		///	Use 3ga->3Gb conversion
		bool						mUseDualLink;			///	Use dual-link for RGB instead of CSC

		bool						mGlobalQuit;			///	Set "true" to gracefully stop
		bool						mTCBounce;				///	Set 'true' to "bounce" burned-in timecode in raster
		AJATimeCodeBurn				mTCBurner;				///	My timecode burner
		NTV2TCIndexes				mTCOutputs;				///	My output timecode destinations
		NTV2TCIndex					mTCSource;				///	My timecode source
		uint32_t					mVideoBufferSize;		///	My video buffer size, in bytes
		uint32_t					mAudioBufferSize;		///	My audio buffer size, in bytes

		AVDataBuffer							mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///	My host buffers
		AJACircularBuffer <AVDataBuffer *>		mAVCircularBuffer;						///	My ring buffer

};	//	QABurn

#endif	//	_QABURN_H
