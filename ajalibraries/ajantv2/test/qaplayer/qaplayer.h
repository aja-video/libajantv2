/**
	@file		qaplayer.cpp
	@brief		Header file for QAPlayer tool
	@copyright	Copyright (C) 2013-2014 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _QAPLAYER_H
#define _QAPLAYER_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "../democlasses/ntv2democommon.h"
#include "ajabase/common/timecodeburn.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"


/**
@brief	This structure encapsulates the control parameters for the QAPlayer class.
**/
typedef struct QAPlayerControl
{
	public:
		UWord					deviceIndex;			///< @brief Specifies the zero-based index number of the AJA device to use.
		bool					withAudio;				///< @brief If true, include audio tone in the output signal;  otherwise, omit it.
		NTV2Channel				channel;				///< @brief Specifies the channel to use.
		NTV2FrameBufferFormat	pixelFormat; 			///< @brief Specifies the pixel format to use for the device's frame buffers.
		NTV2OutputDestination	outputDestination;		///< @brief Specifies which output to playout to. Defaults to SDI2.
		NTV2VideoFormat			videoFormat;			///< @brief Specifies the video format to use.
		bool					withVanc;				///< @brief If true, enable VANC; otherwise disable VANC.
		bool					levelConversion;		///< @brief If true, enable level A to B conversion; otherwise don't
		bool					withBurn;				///< @brief If true, burns in timecode; otherwise no timecode burn-in.
		NTV2ReferenceSource		referenceSource;		///< @brief Specifies the reference source input to use.
		uint32_t				bufferSize;				///< @brief If non-zero, specifies the frame buffer size to use as a multiple of 2MB.
		bool					useDL;					///< @brief Specifies routing RGB output through a dual link, rather than a CSC.
		uint32_t				insertAncLine;			///< @brief If non-zero, specifies the line on which to insert Amc data.
		uint32_t				numberOfAnc;			///< @brief If non-zero, specifies how many 240 byte packets to insert.
		uint32_t				ancBufferSize;			///< @brief If non-zero, specifies the size of the ANC buffers (for both field1 and field 2).
		bool					doNonPCMAudio;			///< @brief If true, make some audio channel pairs carry non-PCM data; otherwise normal audio. No effect if 'withAudio' is false.
		uint32_t				packetsPerLine;			///< @brief Specifies how many packets to insert on a line..

		/**
			@brief	Constructs a default QAPlayerControl.
		**/
		inline explicit QAPlayerControl (void) :
			deviceIndex			(0),
			withAudio			(true),
			channel				(NTV2_CHANNEL1),
			pixelFormat			(NTV2_FBF_8BIT_YCBCR),
			outputDestination	(NTV2_OUTPUTDESTINATION_SDI2),
			videoFormat			(NTV2_FORMAT_1080i_5994),
			withVanc			(false),
			levelConversion		(false),
			withBurn			(true),
			referenceSource		(NTV2_REFERENCE_FREERUN),
			bufferSize			(0),
			useDL				(false),
			insertAncLine		(0),
			numberOfAnc			(0),
			ancBufferSize		(0x12000),
			doNonPCMAudio		(false),
			packetsPerLine		(8)
		{
		}

} QAPlayerControl;

/**
	@brief	I am an object that can play out a test pattern (with timecode) to an output of an AJA device
			with or without audio tone in real time. I make use of the AJACircularBuffer, which simplifies
			implementing a producer/consumer model, in which a "producer" thread generates the test pattern
			frames, and a "consumer" thread (i.e., the "play" thread) sends those frames to the AJA device.
			I demonstrate how to embed timecode into an SDI output signal using AutoCirculate during playout.
**/

class QAPlayer
{
	public:
		/**
			@brief Signature of a function call for requesting frames to be played.
		**/
		typedef AJAStatus (QAPlayerCallback)(void * pInstance, const AVDataBuffer * const playData);

	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given control settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inPlayerControl		Specifies the control parameters I'll use.
		**/
								QAPlayer (const QAPlayerControl & inPlayerControl);

		virtual					~QAPlayer (void);

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
			@param[out]	outOutputStatus	Receives status information about my output (playout) process.
		**/
		virtual void			GetACStatus (AUTOCIRCULATE_STATUS & outputStatus);

		/**
			@brief	Returns the current callback function for requesting frames to be played.
		**/
		virtual void			GetCallback (void ** const pInstance, QAPlayerCallback ** const callback);

		/**
			@brief	Sets a callback function for requesting frames to be played.
		**/
		virtual bool			SetCallback (void * const pInstance, QAPlayerCallback * const callback);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need to play video.
		**/
		virtual AJAStatus		SetUpVideo (void);
		virtual AJAStatus		SetUp4kVideo(void);

		/**
			@brief	Sets up everything I need to play audio.
		**/
		virtual AJAStatus		SetUpAudio (void);

		/**
			@brief	Sets up device routing for playout.
		**/
		virtual void			RouteOutputSignal (void);

		virtual void			Route4kOutputSignal(void);

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
			@brief	Returns the equivalent AJA_PixelFormat for the given NTV2FrameBufferFormat.
			@param[in]	format		Specifies the NTV2FrameBufferFormat to be converted into an equivalent AJA_PixelFormat.
		**/
		static AJA_PixelFormat	GetAJAPixelFormat (const NTV2FrameBufferFormat format);

		/**
			@brief	Returns the equivalent AJA_FrameRate for the given NTV2FrameRate.
			@param[in]	frameRate	Specifies the NTV2FrameRate to be converted into an equivalent AJA_FrameRate.
		**/
		static AJA_FrameRate	GetAJAFrameRate (const NTV2FrameRate frameRate);

		/**
			@brief	Returns the equivalent TimecodeFormat for the given NTV2FrameRate.
			@param[in]	frameRate	Specifies the NTV2FrameRate to be converted into an equivalent TimecodeFormat.
		**/
		static TimecodeFormat	NTV2FrameRate2TimecodeFormat (const NTV2FrameRate inFrameRate);

		/**
			@brief	Returns the RP188 DBB register number to use for the given NTV2OutputDestination.
			@param[in]	inOutputSource	Specifies the NTV2OutputDestination of interest.
			@return	The number of the RP188 DBB register to use for the given output destination.
		**/
		static ULWord			GetRP188RegisterForOutput (const NTV2OutputDestination inOutputSource);


	//	Private Member Data
	private:
		QAPlayerControl				mConfig;					///< @brief	My configuration, as passed to me from main()
		AJAThread *					mConsumerThread;			///	My playout (consumer) thread object
		AJAThread *					mProducerThread;			///	My generator (producer) thread object

		uint32_t					mFramesProduced;			///	My current frame number (used to generate timecode)
		ULWord						mCurrentSample;				///	My current audio sample (maintains audio tone generator state)

		CNTV2Card					mDevice;					///	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;					///	My device (model) identifier
		NTV2EveryFrameTaskMode		mSavedTaskMode;				/// Used to restore the prior task mode
		bool						mVancEnabled;				///	VANC enabled?
		bool						mWideVanc;					///	Wide VANC?
		NTV2AudioSystem				mAudioSystem;				///	The audio system I'm using

		bool						mGlobalQuit;				///	Set "true" to gracefully stop
		AJATimeCodeBurn				mTCBurner;					///	My timecode burner
		uint32_t					mVideoBufferSize;			///	My video buffer size, in bytes

		uint8_t **					mTestPatternVideoBuffers;	///	My test pattern buffers
		int32_t						mNumTestPatterns;			///	Number of test patterns to cycle through

		AVDataBuffer						mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///	My host buffers
		AJACircularBuffer <AVDataBuffer *>	mAVCircularBuffer;						///	My ring buffer

		void *						mInstance;					/// Instance information for the callback function
		QAPlayerCallback *			mPlayerCallback;			/// Address of callback function

		uint8_t *					mpF1AncBuffer;				/// My field 1 ancillary buffer size
		uint8_t *					mpF2AncBuffer;				/// My field 2 ancillary buffer size

		uint32_t					mPacketsPerLine;			/// How many packets to insert before wrapping to the next line
};	//	QAPlayer

#endif	//	_QAPLAYER_H
