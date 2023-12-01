/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streamplayer.cpp
	@brief		Header file for NTV2StreamPlayer demonstration class
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2STREAMPLAYER_H
#define _NTV2STREAMPLAYER_H

#include "ntv2democommon.h"
#include "ajabase/system/thread.h"
#include "ajabase/common/timecodeburn.h"


/**
	@brief	I play out SD or HD test pattern (with timecode) to an output of an AJA device with or without
			audio tone in real time. I make use of the AJACircularBuffer, which simplifies implementing a
			producer/consumer model, in which a "producer" thread generates the test pattern frames, and a
			"consumer" thread (i.e., the "play" thread) sends those frames to the AJA device. I also show
			how to embed timecode into an SDI output signal using AutoCirculate during playout.
**/
class NTV2StreamPlayer
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inConfig	Specifies all configuration parameters.
		**/
							NTV2StreamPlayer (const PlayerConfig & inConfig);

		virtual				~NTV2StreamPlayer (void);

		virtual AJAStatus	Init (void);					///< @brief	Initializes me and prepares me to Run.

		/**
			@brief	Runs me.
			@note	Do not call this method without first calling my Init method.
		**/
		virtual AJAStatus	Run (void);

		virtual void		Quit (void);					///< @brief	Gracefully stops me from running.

		virtual bool		IsRunning (void) const	{return !mGlobalQuit;}	///< @return	True if I'm running;  otherwise false.

		/**
			@brief	Provides status information about my output (playout) process.
			@param[out]	outStatus	Receives the ::AUTOCIRCULATE_STATUS information.
		**/
		virtual void		GetStreamStatus (NTV2StreamChannel & outStatus);


	//	Protected Instance Methods
	protected:
		virtual AJAStatus	SetUpVideo (void);				///< @brief	Performs all video setup.
		virtual bool		RouteOutputSignal (void);		///< @brief	Performs all widget/signal routing for playout.
		virtual AJAStatus	SetUpHostBuffers (void);		///< @brief	Sets up my host video & audio buffers.
		virtual AJAStatus	SetUpTestPatternBuffers (void);	///< @brief	Creates my test pattern buffers.
		virtual void		StartConsumerThread (void);		///< @brief	Starts my consumer thread.
		virtual void		ConsumeFrames (void);			///< @brief	My consumer thread that repeatedly plays frames using AutoCirculate (until quit).
		virtual void		StartProducerThread (void);		///< @brief	Starts my producer thread.
		virtual void		ProduceFrames (void);			///< @brief	My producer thread that repeatedly produces video frames.

	//	Protected Class Methods
	protected:
		/**
			@brief	This is the consumer thread's static callback function that gets called when the consumer thread starts.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread (the NTV2StreamPlayer instance).
		**/
		static void			ConsumerThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the producer thread's static callback function that gets called when the producer thread starts.
					This function gets "Attached" to the producer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the producer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread (the NTV2StreamPlayer instance).
		**/
		static void			ProducerThreadStatic (AJAThread * pThread, void * pContext);


	//	Private Member Data
	private:
		struct FrameData
		{
			NTV2Buffer	fVideoBuffer;
			bool		fDataReady;
		};
		typedef std::vector<FrameData> 		FrameDataArray;
		typedef FrameDataArray::iterator	FrameDataArrayIter;
		typedef std::vector<NTV2Buffer>		NTV2Buffers;

		PlayerConfig		mConfig;			///< @brief	My operating configuration
		AJAThread			mConsumerThread;	///< @brief	My playout (consumer) thread object
		AJAThread			mProducerThread;	///< @brief	My generator (producer) thread object
		CNTV2Card			mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID		mDeviceID;			///< @brief	My device (model) identifier
		NTV2TaskMode		mSavedTaskMode;		///< @brief	Used to restore the previous task mode
		ULWord				mCurrentFrame;		///< @brief	My current frame number (for generating timecode)
		ULWord				mCurrentSample;		///< @brief	My current audio sample (tone generator state)
		double				mToneFrequency;		///< @brief	My current audio tone frequency [Hz]
		NTV2AudioSystem		mAudioSystem;		///< @brief	The audio system I'm using (if any)
		NTV2FormatDesc		mFormatDesc;		///< @brief	Describes my video/pixel format

		bool				mGlobalQuit;		///< @brief	Set "true" to gracefully stop
		AJATimeCodeBurn		mTCBurner;			///< @brief	My timecode burner
		FrameDataArray		mHostBuffers;		///< @brief	My host buffers
		ULWord				mProducerIndex;		///< @brief	Producer frame index
		ULWord				mConsumerIndex;		///< @brief	Consumer frame index
		NTV2Buffers			mTestPatRasters;	///< @brief	Pre-rendered test pattern rasters

};	//	NTV2StreamPlayer

#endif	//	_NTV2STREAMPLAYER_H
