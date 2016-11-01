/**
	@file		ntv2player4k.h
	@brief		Header file for NTV2Player4K demonstration class
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2PLAYER4K_H
#define _NTV2PLAYER4K_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ajastuff/common/circularbuffer.h"


class AJAThread;


/**
	@brief	This class is used to configure the device for 4K playback.
**/
typedef struct Player4KConfig
{
	public:
		std::string				fDeviceSpecifier;		///<	Specifies the AJA device to use.
		bool					fWithAudio;				///<	If true, include audio tone in the output signal;  otherwise, omit it.
		NTV2Channel				fChannel;				///<	Specifies the channel to use.
		NTV2FrameBufferFormat	fPixelFormat;			///<	Specifies the pixel format to use for the device's frame buffers.
		NTV2VideoFormat			fVideoFormat;			///<	Specifies the video format to use.
		bool					fUseHDMIOut;			///<	If true, enables an HDMI output signal;  otherwise, disables it.
		bool					fDoMultiChannel;		///<	If true, enables multiple player 4k instances to share a board.
		bool					fDoTsiRouting;			///<	If true, enables two sample interleave routing, else squares.
		bool					fDoRGBOnWire;			///<	If true, enables RGB on the wire, else CSCs convert to YCbCr.

		/**
			@brief	Constructs a default generator configuration.
		**/
		inline explicit	Player4KConfig ()
			:	fDeviceSpecifier	("0"),
				fWithAudio			(true),
				fChannel			(NTV2_CHANNEL1),
				fPixelFormat		(NTV2_FBF_8BIT_YCBCR),
				fVideoFormat		(NTV2_FORMAT_4x1920x1080p_2997),
				fUseHDMIOut			(false),
				fDoMultiChannel		(false),
				fDoTsiRouting		(false),
				fDoRGBOnWire		(false)
		{
		}
}	Player4KConfigConfig;


/**
	@brief	I am an object that can play out a test pattern (with timecode) to an output of an AJA device
			with or without audio tone in real time. I make use of the AJACircularBuffer, which simplifies
			implementing a producer/consumer model, in which a "producer" thread generates the test pattern
			frames, and a "consumer" thread (i.e., the "play" thread) sends those frames to the AJA device.
			I demonstrate how to embed timecode into an SDI output signal using AutoCirculate during playout.
**/

class NTV2Player4K
{
	public:
		/**
			@brief Signature of a function call for requesting frames to be played.
		**/
		typedef AJAStatus (NTV2Player4KCallback)(void * pInstance, const AVDataBuffer * const playData);

	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
		**/
						NTV2Player4K (const Player4KConfig & inConfiguration);

		virtual			~NTV2Player4K (void);

		/**
			@brief	Initializes me and prepares me to Run.
		**/
		AJAStatus		Init (void);

		/**
			@brief	Runs me.
			@note	Do not call this method without first calling my Init method.
		**/
		AJAStatus		Run (void);

		/**
			@brief	Gracefully stops me from running.
		**/
		void			Quit (void);

		/**
			@brief	Provides status information about my input (capture) and output (playout) processes.
			@param[out]	outOutputStatus		Receives status information about my output (playout) process.
		**/
		void			GetACStatus (AUTOCIRCULATE_STATUS & outOutputStatus);

		/**
			@brief	Returns the current callback function for requesting frames to be played.
		**/
		virtual void	GetCallback (void ** const pInstance, NTV2Player4KCallback ** const callback);

		/**
			@brief	Sets a callback function for requesting frames to be played.
		**/
		virtual bool	SetCallback (void * const pInstance, NTV2Player4KCallback * const callback);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need to play video.
		**/
		AJAStatus		SetUpVideo (void);

		/**
			@brief	Sets up everything I need to play audio.
		**/
		AJAStatus		SetUpAudio (void);

		/**
			@brief	Sets up board routing for playout.
		**/
		void			RouteOutputSignal (void);

		/**
			@brief	Sets up board routing for the 4K Down Converter to the SDI Monitor (if available).
		**/
		void			Route4KDownConverter (void);

		/**
			@brief	Sets up board routing output via the HDMI (if available).
		**/
		void			RouteHDMIOutput (void);

		/**
			@brief	Sets up board routing from the Frame Stores to the Dual Link out.
		**/
		void			RouteFsToDLOut (void);

		/**
			@brief	Sets up board routing from the Frame Stores to the Color Space Converters.
		**/
		void			RouteFsToCsc (void);

		/**
			@brief	Sets up board routing from the Frame Stores to the SDI outputs.
		**/
		void			RouteFsToSDIOut (void);

		/**
			@brief	Sets up board routing from the Frame Stores to the Two Sample Interleave muxes.
		**/
		void			RouteFsToTsiMux (void);

		/**
			@brief	Sets up board routing from the Dual Link outputs to the SDI outputs.
		**/
		void			RouteDLOutToSDIOut (void);

		/**
			@brief	Sets up board routing from the Color Space Converters to the SDI outputs.
		**/
		void			RouteCscToSDIOut (void);

		/**
			@brief	Sets up board routing from the Color Space Converters to the Dual Link outputs.
		**/
		void			RouteCscToDLOut (void);

		/**
			@brief	Sets up board routing from the Two Sample Interleave muxes to the Dual Link outputs.
		**/
		void			RouteTsiMuxToDLOut (void);

		/**
			@brief	Sets up board routing from the Two Sample Interleave muxes to the color Space Convertetrs.
		**/
		void			RouteTsiMuxToCsc (void);

		/**
			@brief	Sets up board routing from the Two Sample Interleave muxes to the SDI outputs.
		**/
		void			RouteTsiMuxToSDIOut (void);	

		/**
			@brief	Sets up my circular buffers.
		**/
		void			SetUpHostBuffers (void);

		/**
			@brief	Creates my test pattern buffers.
		**/
		void			SetUpTestPatternVideoBuffers (void);

		/**
			@brief	Starts my playout thread.
		**/
		void			StartPlayThread (void);

		/**
			@brief	Repeatedly plays out frames using AutoCirculate (until global quit flag set).
		**/
		void			PlayFrames (void);

		/**
			@brief	Starts my test pattern producer thread.
		**/
		void			StartProduceFrameThread (void);

		/**
			@brief	Repeatedly produces test pattern frames (until global quit flag set).
		**/
		void			ProduceFrames (void);

		/**
			@brief	Inserts audio tone (based on my current tone frequency) into the given audio buffer.
			@param[out]	audioBuffer		Specifies a valid, non-NULL pointer to the buffer that is to receive
										the audio tone data.
			@return	Total number of bytes written into the buffer.
		**/
		uint32_t		AddTone (ULWord * audioBuffer);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the playout thread's static callback function that gets called when the playout thread runs.
					This function gets "Attached" to the playout thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the playout thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Player4K instance.)
		**/
		static void		PlayThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the test pattern producer thread's static callback function that gets called when the producer thread runs.
					This function gets "Attached" to the producer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the producer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Player4K instance.)
		**/
		static void		ProduceFrameThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	Returns the RP188 DBB register number to use for the given NTV2OutputDestination.
			@param[in]	inOutputSource	Specifies the NTV2OutputDestination of interest.
			@return	The number of the RP188 DBB register to use for the given output destination.
		**/
		static ULWord	GetRP188RegisterForOutput (const NTV2OutputDestination inOutputSource);


	//	Private Member Data
	private:
		AJAThread *					mPlayThread;				///< @brief	My playout (consumer) thread object
		AJAThread *					mProduceFrameThread;		///< @brief	My generator (producer) thread object
		AJALock *					mLock;						///< @brief	Global mutex to avoid device frame buffer allocation race condition

		uint32_t					mCurrentFrame;				///< @brief	My current frame number (used to generate timecode)
		ULWord						mCurrentSample;				///< @brief	My current audio sample (maintains audio tone generator state)
		double						mToneFrequency;				///< @brief	My current audio tone frequency, in Hertz

		CNTV2Card					mDevice;					///< @brief	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;					///< @brief	My board (model) identifier
		const std::string			mDeviceSpecifier;			///< @brief	Specifies the device I should use
		const bool					mWithAudio;					///< @brief	Capture and playout audio?
		const bool					mUseHDMIOut;				///< @brief	Enable HDMI output?
		const NTV2Channel			mChannel;					///< @brief	The channel I'm using
		NTV2Crosspoint				mChannelSpec;				///< @brief	The AutoCirculate channel spec I'm using
		NTV2VideoFormat				mVideoFormat;				///< @brief	My video format
		NTV2FrameBufferFormat		mPixelFormat;				///< @brief	My pixel format
		NTV2EveryFrameTaskMode		mPreviousFrameServices;		///< @brief	Used to restore the previous task mode
		bool						mVancEnabled;				///< @brief	VANC enabled?
		bool						mWideVanc;					///< @brief	Wide VANC?
		NTV2AudioSystem				mAudioSystem;				///< @brief	The audio system I'm using
		bool						mDoMultiChannel;			///< @brief	Allow more than one player 4k to play
		bool						mDoTsiRouting;				///< @brief	Route the output through the Tsi Muxes
		bool						mDoRGBOnWire;				///< @brief	Route the output through the Dual Link to put RGB on the wire

		bool						mGlobalQuit;				///< @brief	Set "true" to gracefully stop
		AJATimeCodeBurn				mTCBurner;					///< @brief	My timecode burner
		uint32_t					mVideoBufferSize;			///< @brief	My video buffer size, in bytes
		uint32_t					mAudioBufferSize;			///< @brief	My audio buffer size, in bytes

		uint8_t **					mTestPatternVideoBuffers;	///< @brief	My test pattern buffers
		int32_t						mNumTestPatterns;			///< @brief	Number of test patterns to cycle through

		AVDataBuffer							mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///< @brief	My host buffers
		AJACircularBuffer <AVDataBuffer *>		mAVCircularBuffer;						///< @brief	My ring buffer

		AUTOCIRCULATE_TRANSFER           mOutputTransferStruct;					///< @brief	My A/C output transfer info
		AUTOCIRCULATE_TRANSFER_STATUS    mOutputTransferStatusStruct;			///< @brief	My A/C output status

		void *						mInstance;					///< @brief	Instance information for the callback function
		NTV2Player4KCallback *		mPlayerCallback;			///< @brief	Address of callback function

};	//	NTV2Player4K

#endif	//	_NTV2PLAYER4K_H
