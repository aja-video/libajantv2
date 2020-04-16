/**
	@file		ntv2player.cpp
	@brief		Header file for NTV2Player demonstration class
	@copyright	(C) 2013-2020 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2PLAYER_H
#define _NTV2PLAYER_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/info.h"
#include "ajaanc/includes/ancillarydata.h"

/**
	@brief	This class is used to configure an NTV2Player instance.
**/
typedef struct PlayerConfig
{
	public:
		std::string						fDeviceSpecifier;	///< @brief	The AJA device to use
		NTV2Channel						fOutputChannel;		///< @brief	The device channel to use
		NTV2OutputDestination			fOutputDestination;	///< @brief	The desired output connector to use
		NTV2VideoFormat					fVideoFormat;		///< @brief	The video format to use
		NTV2FrameBufferFormat			fPixelFormat;		///< @brief	The pixel format to use
		CNTV2DemoCommon::ACFrameRange	fFrames;			///< @brief	AutoCirculate frame count or range
		AJAAncillaryDataType			fTransmitHDRType;	///< @brief	If true, route dual-link RGB output;  otherwise normal YCbCr
		bool							fDoMultiFormat;		///< @brief	If true, use multi-format/multi-channel mode, if device supports it; otherwise normal mode
		bool							fSuppressAudio;		///< @brief	If true, suppress audio;  otherwise generate audio tones
		bool							fTransmitLTC;		///< @brief	If true, embed LTC;  otherwise embed VITC
		bool							fDoLevelConversion;	///< @brief	If true, do A-to-B conversion;  otherwise don't

		/**
			@brief	Constructs a default Player configuration.
		**/
		inline explicit	PlayerConfig (const std::string & inDeviceSpecifier	= "0")
			:	fDeviceSpecifier	(inDeviceSpecifier),
				fOutputChannel		(NTV2_CHANNEL1),
				fOutputDestination	(NTV2_OUTPUTDESTINATION_SDI2),
				fVideoFormat		(NTV2_FORMAT_1080i_5994),
				fPixelFormat		(NTV2_FBF_8BIT_YCBCR),
				fFrames				(7),
				fTransmitHDRType	(AJAAncillaryDataType_Unknown),
				fDoMultiFormat		(false),
				fSuppressAudio		(false),
				fTransmitLTC		(false),
				fDoLevelConversion	(false)
		{
		}
		AJALabelValuePairs Get (const bool inCompact = false) const;

}	PlayerConfig;

std::ostream &	operator << (std::ostream & ioStrm, const PlayerConfig & inObj);


/**
	@brief	I am an object that can play out a test pattern (with timecode) to an output of an AJA device
			with or without audio tone in real time. I make use of the AJACircularBuffer, which simplifies
			implementing a producer/consumer model, in which a "producer" thread generates the test pattern
			frames, and a "consumer" thread (i.e., the "play" thread) sends those frames to the AJA device.
			I also show how to embed timecode into an SDI output signal using AutoCirculate during playout.
**/
class NTV2Player
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inConfig	Specifies all configuration parameters.
		**/
								NTV2Player (const PlayerConfig & inConfig);

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


	//	Protected Instance Methods
	protected:
		/**
			@brief	Performs all video setup.
		**/
		virtual AJAStatus		SetUpVideo (void);

		/**
			@brief	Performs all audio setup.
		**/
		virtual AJAStatus		SetUpAudio (void);

		/**
			@brief	Performs all widget/signal routing for playout.
		**/
		virtual void			RouteOutputSignal (void);

		/**
			@brief	Sets up my host buffers (my mHostBuffers and mAVCircularBuffer members).
		**/
		virtual void			SetUpHostBuffers (void);

		/**
			@brief	Creates my test pattern buffers (in my mTestPatVidBuffers member).
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
			@brief		Inserts audio tone (based on my current tone frequency) into the given NTV2FrameData's audio buffer.
			@param		inFrameData		The NTV2FrameData object having the audio buffer to be filled.
			@return		Total number of bytes written into the buffer.
		**/
		virtual uint32_t		AddTone (NTV2FrameData & inFrameData);

		/**
			@return		True if the given output destination's RP188 bypass is enabled; otherwise false.
			@param[in]	inOutputDest	Specifies the NTV2OutputDestination of interest.
		**/
		virtual bool			OutputDestHasRP188BypassEnabled (const NTV2OutputDestination inOutputDest);

		/**
			@brief	Disables the given SDI output's RP188 bypass.
			@param[in]	inOutputDest	Specifies the NTV2OutputDestination of interest.
		**/
		virtual void			DisableRP188Bypass (const NTV2OutputDestination inOutputDest);


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
			@brief	Returns the RP188 DBB register number to use for the given ::NTV2OutputDestination.
			@param[in]	inOutputSource	Specifies the NTV2OutputDestination of interest.
			@return	The number of the RP188 DBB register to use for the given output destination.
		**/
		static ULWord			GetRP188RegisterForOutput (const NTV2OutputDestination inOutputSource);


	//	Private Member Data
	private:
		typedef AJACircularBuffer<NTV2FrameData*>	MyCircularBuffer;
		typedef std::vector<NTV2_POINTER>			NTV2Buffers;

		AJAThread			mConsumerThread;	///< @brief	My playout (consumer) thread object
		AJAThread			mProducerThread;	///< @brief	My generator (producer) thread object
		CNTV2Card			mDevice;			///< @brief	My CNTV2Card instance. This is what I use to talk to the device.
		NTV2DeviceID		mDeviceID;			///< @brief	My device identifier
		PlayerConfig		mConfig;			///< @brief	My operating configuration
		NTV2FormatDesc		mFormatDesc;		///< @brief	Describes my video/pixel format
		NTV2TaskMode		mSavedTaskMode;		///< @brief	Used to restore prior task mode
		NTV2AudioSystem		mAudioSystem;		///< @brief	The audio system I'm using (if any)
		bool				mGlobalQuit;		///< @brief	Set "true" to gracefully stop
		NTV2FrameDataArray	mHostBuffers;		///< @brief	My host buffers
		MyCircularBuffer	mAVCircularBuffer;	///< @brief	My ring buffer object

		uint32_t			mCurrentFrame;		///< @brief	My current frame number (for generating timecode)
		ULWord				mCurrentSample;		///< @brief	My current audio sample (maintains audio tone generator state)
		double				mToneFrequency;		///< @brief	My current audio tone frequency [Hz]

		AJATimeCodeBurn		mTCBurner;			///< @brief	My timecode burner
		NTV2TCIndexes		mTCIndexes;			///< @brief	Timecode indexes to use
		NTV2Buffers			mTestPatVidBuffers;	///< @brief	An array of video buffers of pre-drawn test patterns

};	//	NTV2Player

#endif	//	_NTV2PLAYER_H
