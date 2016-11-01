/**
	@file		ntv2player.cpp
	@brief		Header file for NTV2Player demonstration class
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2PLAYER_H
#define _NTV2PLAYER_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ajastuff/common/circularbuffer.h"
#include "ajastuff/system/thread.h"


/**
	@brief	I am an object that can play out a test pattern (with timecode) to an output of an AJA device
			with or without audio tone in real time. I make use of the AJACircularBuffer, which simplifies
			implementing a producer/consumer model, in which a "producer" thread generates the test pattern
			frames, and a "consumer" thread (i.e., the "play" thread) sends those frames to the AJA device.
			I demonstrate how to embed timecode into an SDI output signal using AutoCirculate during playout.
**/

class NTV2Player
{
	public:
		/**
			@brief Signature of a function call for requesting frames to be played.
		**/
		typedef AJAStatus (NTV2PlayerCallback)(void * pInstance, const AVDataBuffer * const playData);

	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use. Defaults to "0", the first device found.
			@param[in]	inWithAudio			If true, include audio tone in the output signal;  otherwise, omit it.
											Defaults to "true".
			@param[in]	inChannel			Specifies the channel to use. Defaults to NTV2_CHANNEL1.
			@param[in]	inPixelFormat		Specifies the pixel format to use for the device's frame buffers.
											Defaults to 8-bit YUV.
			@param[in]	inOutputDestination	Specifies which output to playout to. Defaults to SDI2.
			@param[in]	inVideoFormat		Specifies the video format to use. Defaults to 1080i5994.
			@param[in]	inWithVanc			If true, enable VANC; otherwise disable VANC. Defaults to false.
			@param[in]	inLevelConversion	If true, demonstrate level A to B conversion; otherwise don't. Defaults to false.
			@param[in]	inDoMultiFormat		If true, use multi-format mode; otherwise use uniformat mode. Defaults to false (uniformat mode).
		**/
								NTV2Player (const std::string &			inDeviceSpecifier	= "0",
											const bool					inWithAudio			= true,
											const NTV2Channel			inChannel			= NTV2_CHANNEL1,
											const NTV2FrameBufferFormat	inPixelFormat		= NTV2_FBF_8BIT_YCBCR,
											const NTV2OutputDestination	inOutputDestination	= NTV2_OUTPUTDESTINATION_SDI2,
											const NTV2VideoFormat		inVideoFormat		= NTV2_FORMAT_1080i_5994,
											const bool					inWithVanc			= false,
											const bool					inLevelConversion	= false,
											const bool					inDoMultiFormat		= false);

		virtual					~NTV2Player (void);

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
			@return	True if I'm running;  otherwise false.
		**/
		virtual bool			IsRunning (void) const				{return !mGlobalQuit;}

		/**
			@brief	Provides status information about my output (playout) process.
			@param[out]	outGoodFrames		Receives the number of successfully played frames.
			@param[out]	outDroppedFrames	Receives the number of dropped frames.
			@param[out]	outBufferLevel		Receives the driver's current buffer level.
		**/
		virtual void			GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel);

		/**
			@brief	Returns the current callback function for requesting frames to be played.
		**/
		virtual void			GetCallback (void ** const pInstance, NTV2PlayerCallback ** const callback);

		/**
			@brief	Sets a callback function for requesting frames to be played.
		**/
		virtual bool			SetCallback (void * const pInstance, NTV2PlayerCallback * const callback);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need to play video.
		**/
		virtual AJAStatus		SetUpVideo (void);

		/**
			@brief	Sets up everything I need to play audio.
		**/
		virtual AJAStatus		SetUpAudio (void);

		/**
			@brief	Sets up device routing for playout.
		**/
		virtual void			RouteOutputSignal (void);

		/**
			@brief	Sets up my circular buffers.
		**/
		virtual void			SetUpHostBuffers (void);

		/**
			@brief	Initializes playout AutoCirculate.
		**/
		virtual void			SetUpOutputAutoCirculate (void);

		/**
			@brief	Creates my test pattern buffers.
		**/
		virtual AJAStatus		SetUpTestPatternVideoBuffers (void);

		/**
			@brief	Starts my playout thread.
		**/
		virtual void			StartConsumerThread (void);

		/**
			@brief	Repeatedly plays out frames using AutoCirculate (until quit).
		**/
		virtual void			PlayFrames (void);

		/**
			@brief	Starts my test pattern producer thread.
		**/
		virtual void			StartProducerThread (void);

		/**
			@brief	Repeatedly produces test pattern frames (until global quit flag set).
		**/
		virtual void			ProduceFrames (void);

		/**
			@brief	Inserts audio tone (based on my current tone frequency) into the given audio buffer.
			@param[out]	audioBuffer		Specifies a valid, non-NULL pointer to the buffer that is to receive
										the audio tone data.
			@return	Total number of bytes written into the buffer.
		**/
		virtual uint32_t		AddTone (ULWord * audioBuffer);

		/**
			@brief	Returns true if my current output destination's RP188 bypass is enabled; otherwise returns false.
		**/
		virtual bool			OutputDestHasRP188BypassEnabled (void);

		/**
			@brief	Disables my current output destination's RP188 bypass.
		**/
		virtual void			DisableRP188Bypass (void);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the consumer thread's static callback function that gets called when the consumer thread starts.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Player instance.)
		**/
		static void				ConsumerThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the producer thread's static callback function that gets called when the producer thread starts.
					This function gets "Attached" to the producer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the producer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Player instance.)
		**/
		static void				ProducerThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	Returns the RP188 DBB register number to use for the given NTV2OutputDestination.
			@param[in]	inOutputSource	Specifies the NTV2OutputDestination of interest.
			@return	The number of the RP188 DBB register to use for the given output destination.
		**/
		static ULWord			GetRP188RegisterForOutput (const NTV2OutputDestination inOutputSource);


	//	Private Member Data
	private:
		typedef AJACircularBuffer <AVDataBuffer *>		MyCirculateBuffer;

		AJAThread *					mConsumerThread;			///< @brief	My playout (consumer) thread object
		AJAThread *					mProducerThread;			///< @brief	My generator (producer) thread object
		AJALock *					mLock;						///< @brief	Global mutex to avoid device frame buffer allocation race condition

		uint32_t					mCurrentFrame;				///< @brief	My current frame number (used to generate timecode)
		ULWord						mCurrentSample;				///< @brief	My current audio sample (maintains audio tone generator state)
		double						mToneFrequency;				///< @brief	My current audio tone frequency [Hz]

		const std::string			mDeviceSpecifier;			///< @brief	Specifies the device I should use
		CNTV2Card					mDevice;					///< @brief	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;					///< @brief	My device (model) identifier
		NTV2Channel					mOutputChannel;				///< @brief	The channel I'm using
		const NTV2OutputDestination	mOutputDestination;			///< @brief	The output I'm using
		NTV2VideoFormat				mVideoFormat;				///< @brief	My video format
		NTV2FrameBufferFormat		mPixelFormat;				///< @brief	My pixel format
		NTV2EveryFrameTaskMode		mSavedTaskMode;				///< @brief	Used to restore the prior task mode
		NTV2AudioSystem				mAudioSystem;				///< @brief	The audio system I'm using
		const bool					mWithAudio;					///< @brief	Playout audio?
		bool						mVancEnabled;				///< @brief	VANC enabled?
		bool						mWideVanc;					///< @brief	Wide VANC?
		bool						mEnableVanc;				///< @brief	Enable VANC?
		bool						mGlobalQuit;				///< @brief	Set "true" to gracefully stop
		bool						mDoLevelConversion;			///< @brief	Demonstrates a level A to level B conversion
		bool						mDoMultiChannel;			///< @brief	Demonstrates how to configure the board for multi-format
		AJATimeCodeBurn				mTCBurner;					///< @brief	My timecode burner
		uint32_t					mVideoBufferSize;			///< @brief	My video buffer size, in bytes
		uint32_t					mAudioBufferSize;			///< @brief	My audio buffer size, in bytes

		uint8_t **					mTestPatternVideoBuffers;	///< @brief	My test pattern buffers
		int32_t						mNumTestPatterns;			///< @brief	Number of test patterns to cycle through

		AVDataBuffer				mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///< @brief	My host buffers
		MyCirculateBuffer			mAVCircularBuffer;						///< @brief	My ring buffer

		void *						mCallbackUserData;			///< @brief	User data to be passed to the callback function
		NTV2PlayerCallback *		mCallback;					///< @brief	Address of callback function

};	//	NTV2Player

#endif	//	_NTV2PLAYER_H
