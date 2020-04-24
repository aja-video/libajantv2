/**
	@file		ntv2player4k.h
	@brief		Header file for NTV2Player4K demonstration class
	@copyright	(C) 2013-2020 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2PLAYER4K_H
#define _NTV2PLAYER4K_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ajabase/common/circularbuffer.h"
#include "ajaanc/includes/ancillarydata.h"
#include "ajaanc/includes/ancillarydata_hdr_sdr.h"
#include "ajaanc/includes/ancillarydata_hdr_hdr10.h"
#include "ajaanc/includes/ancillarydata_hdr_hlg.h"
#include "ajabase/system/thread.h"


/**
	@brief	This class is used to configure the device for 4K playback.
**/
typedef struct Player4KConfig
{
	public:
		std::string				fDeviceSpecifier;	///< @brief	Specifies the AJA device to use.
		NTV2AudioSystem			fAudioSystem;		///< @brief	Specifies the audio system to use (use NTV2_AUDIOSYSTEM_INVALID for no audio).
		NTV2Channel				fOutputChannel;		///< @brief	Specifies the channel to use.
		NTV2FrameBufferFormat	fPixelFormat;		///< @brief	Specifies the pixel format to use for the device's frame buffers.
		NTV2VideoFormat			fVideoFormat;		///< @brief	Specifies the video format to use.
		AJAAncillaryDataType	fSendAncType;		///< @brief	Specifies the HDR anc data packet to transmit, if any.
		bool					fDoHDMIOutput;		///< @brief	If true, enables HDMI output;  otherwise, disables it.
		bool					fDoMultiChannel;	///< @brief	If true, enables device-sharing;  otherwise takes exclusive control of the device.
		bool					fDoTsiRouting;		///< @brief	If true, enables two sample interleave routing, else squares.
		bool					fDoRGBOnWire;		///< @brief	If true, enables RGB on the wire, else CSCs convert to YCbCr.
		bool					fDoLinkGrouping;	///< @brief If true, enables 6/12G output mode

		/**
			@brief	Constructs a default generator configuration.
		**/
		inline explicit	Player4KConfig ()
			:	fDeviceSpecifier	("0"),
				fAudioSystem		(NTV2_AUDIOSYSTEM_1),
				fOutputChannel		(NTV2_CHANNEL1),
				fPixelFormat		(NTV2_FBF_8BIT_YCBCR),
				fVideoFormat		(NTV2_FORMAT_4x1920x1080p_2997),
				fSendAncType		(AJAAncillaryDataType_Unknown),
				fDoHDMIOutput		(false),
				fDoMultiChannel		(false),
				fDoTsiRouting		(false),
				fDoRGBOnWire		(false),
				fDoLinkGrouping		(false)
		{
		}

		inline bool	WithAudio(void) const	{return NTV2_IS_VALID_AUDIO_SYSTEM(fAudioSystem);}	///< @return	True if playing audio, false if not.

		/**
			@brief		Renders a human-readable representation of me into the given output stream.
			@param		strm	The output stream.
			@return		A reference to the output stream.
		**/
		std::ostream &	Print (std::ostream & strm) const;
}	Player4KConfig;

/**
	@brief		Renders a human-readable representation of a Player4KConfig into an output stream.
	@param		strm	The output stream.
	@return		A reference to the output stream.
**/
inline std::ostream &	operator << (std::ostream & strm, const Player4KConfig & inObj)		{return inObj.Print(strm);}


/**
	@brief	I am an object that can play out a test pattern (with timecode) to an output of an AJA device
			with or without audio tone in real time. I make use of the AJACircularBuffer, which simplifies
			implementing a producer/consumer model, in which a "producer" thread generates the test pattern
			frames, and a "consumer" thread (i.e., the "play" thread) sends those frames to the AJA device.
			I demonstrate how to embed timecode into an SDI output signal using AutoCirculate during playout.
**/

class NTV2Player4K
{
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
			@brief	Sets up bi-directional SDI transmitters
		**/
		void			SetupSDITransmitters(const NTV2Channel startChannel, const uint32_t numChannels);

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
            @brief	Sets up board routing from the Color Space Converters to the 2xSDI outputs.
		**/
        void			RouteCscTo2xSDIOut (void);

        /**
            @brief	Sets up board routing from the Color Space Converters to the 4xSDI outputs.
        **/
        void			RouteCscTo4xSDIOut (void);

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
            @brief	Sets up board routing from the Two Sample Interleave muxes to the 2xSDI outputs.
		**/
        void			RouteTsiMuxTo2xSDIOut (void);

        /**
            @brief	Sets up board routing from the Two Sample Interleave muxes to the 4xSDI outputs.
        **/
        void			RouteTsiMuxTo4xSDIOut (void);

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
		void			StartConsumerThread (void);

		/**
			@brief	Repeatedly plays out frames using AutoCirculate (until global quit flag set).
		**/
		void			ConsumeFrames (void);

		/**
			@brief	Starts my test pattern producer thread.
		**/
		void			StartProducerThread (void);

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
		Player4KConfig			mConfig;				///< @brief	My configuration.
		AJAThread				mConsumerThread;		///< @brief	My playout (consumer) thread object
		AJAThread				mProducerThread;		///< @brief	My generator (producer) thread object
		uint32_t				mCurrentFrame;			///< @brief	My current frame number (used to generate timecode)
		ULWord					mCurrentSample;			///< @brief	My current audio sample (maintains audio tone generator state)
		double					mToneFrequency;			///< @brief	My current audio tone frequency, in Hertz
		CNTV2Card				mDevice;				///< @brief	My CNTV2Card instance
		NTV2DeviceID			mDeviceID;				///< @brief	My device (model) identifier
		NTV2EveryFrameTaskMode	mSavedTaskMode;			///< @brief	Used to restore the previous task mode
		bool					mGlobalQuit;			///< @brief	Set "true" to gracefully stop
		AJATimeCodeBurn			mTCBurner;				///< @brief	My timecode burner
		uint32_t				mVideoBufferSize;		///< @brief	My video buffer size, in bytes
		uint32_t				mAudioBufferSize;		///< @brief	My audio buffer size, in bytes
		uint8_t **				mTestPatternBuffers;	///< @brief	My array of test pattern buffers
		uint32_t				mNumTestPatterns;		///< @brief	Number of test patterns to cycle through
		AVDataBuffer			mHostBuffers[CIRCULAR_BUFFER_SIZE];	///< @brief	My host buffers
		AJACircularBuffer <AVDataBuffer *>	mAVCircularBuffer;		///< @brief	My ring buffer

};	//	NTV2Player4K

#endif	//	_NTV2PLAYER4K_H
