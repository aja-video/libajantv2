/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolbycapture.h
	@brief		Declares the NTV2DolbyCapture class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2DOLBYCAPTURE_H
#define _NTV2DOLBYCAPTURE_H

#include "ntv2democommon.h"
#include "ajabase/system/thread.h"


/**
	@brief	This class is used to configure an NTV2Capture instance.
**/
typedef struct DolbyCaptureConfig : CaptureConfig
{
	public:
		std::string	fAudioDataFilePath;	///< @brief	Optional path to Audio binary data file
		std::string	fDolbyDataFilePath;	///< @brief	Optional path to Dolby binary data file
		bool		fDoAudioFilter;		///< @brief	If true, capture only audio anc
		bool		fDoFrameStats;		///< @brief if true, output per frame statistics

		/**
			@brief	Constructs a default DolbyCaptureConfig
		**/
		inline explicit	DolbyCaptureConfig (const std::string & inDeviceSpec = "0")
			:	CaptureConfig(inDeviceSpec),
				fDoAudioFilter	(false),
				fDoFrameStats	(false)
		{
		}
		AJALabelValuePairs	Get (const bool inCompact = false) const;
} DolbyCaptureConfig;

std::ostream &	operator << (std::ostream & ioStrm, const DolbyCaptureConfig & inObj);


/**
	@brief	I capture HDMI Dolby audio from an HDMI input of an AJA device.
**/
class NTV2DolbyCapture
{
	//	Public Instance Methods
	public:
		/**
			@brief		Constructs me using the given settings.
			@param[in]	inConfig	Specifies how to configure capture.
			@note		I'm not completely initialized and ready to use until after my Init method has been called.
		**/
		NTV2DolbyCapture (const DolbyCaptureConfig & inConfig);

		virtual						~NTV2DolbyCapture ();

		/**
			@brief	Initializes me and prepares me to Run.
		**/
		virtual AJAStatus			Init (void);

		/**
			@brief	Runs me.
			@note	Call this method only after calling Init and it returned AJA_STATUS_SUCCESS.
		**/
		virtual AJAStatus			Run (void);

		/**
			@brief	Gracefully stops me from running.
		**/
		virtual void				Quit (void);

		/**
			@brief	Provides status information about my input (capture) process.
			@param[out]	outGoodFrames		Receives the number of successfully captured frames.
			@param[out]	outDroppedFrames	Receives the number of dropped frames.
			@param[out]	outBufferLevel		Receives the buffer level (number of captured frames ready to be transferred to the host).
		**/
		virtual void				GetACStatus (ULWord & outGoodFrames, ULWord & outDroppedFrames, ULWord & outBufferLevel);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need for capturing video.
		**/
		virtual AJAStatus		SetupVideo (void);

		/**
			@brief	Sets up everything I need for capturing audio.
		**/
		virtual AJAStatus		SetupAudio (void);

		/**
			@brief	Sets up device routing for capture.
		**/
		virtual bool			RouteInputSignal (void);

		/**
			@brief	Sets up my circular buffers.
		**/
		virtual void			SetupHostBuffers (void);

		/**
			@brief	Starts my capture thread.
		**/
		virtual void			StartProducerThread (void);

		/**
			@brief	Repeatedly captures frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			CaptureFrames (void);

		/**
			@brief	Starts my frame consumer thread.
		**/
		virtual void			StartConsumerThread (void);

		/**
			@brief	Repeatedly consumes frames from the circular buffer (until global quit flag set).
		**/
		virtual void			ConsumeFrames (void);

		/**
			@brief		Recover audio from ancillary data.
			@returns	The number of audio bytes copied into outAudioBuffer.
		**/
		virtual uint32_t		RecoverAudio (const NTV2Buffer & inAncBuffer, const uint32_t inAncSize, NTV2Buffer & outAudioBuffer);

		/**
			@brief		Recover Dolby data from the given audio data.
			@param[in]	inAudioBuffer	A valid buffer containing the captured audio data.
			@param[in]	inAudioSize		Specifies the size of the given audio data buffer.
			@param[out]	outDolbyBuffer	The buffer to receive recovered Dolby data.
			@returns	The number of 16-bit words copied into outDolbyBuffer.
		**/
		virtual uint32_t		RecoverDolby (const NTV2Buffer & inAudioBuffer, const uint32_t inAudioSize, NTV2Buffer & outDolbyBuffer);

	//	Protected Class Methods
	protected:
		/**
			@brief	This is the consumer thread's static callback function that gets called when the consumer thread runs.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the consumer thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will point to the NTV2DolbyCapture instance.)
		**/
		static void	ConsumerThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the capture thread's static callback function that gets called when the capture thread runs.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will point to the NTV2DolbyCapture instance.)
		**/
		static void	ProducerThreadStatic (AJAThread * pThread, void * pContext);


	//	Private Member Data
	private:
		AJAThread			mConsumerThread;	///< @brief	My consumer thread object -- consumes the captured frames.
		AJAThread			mProducerThread;	///< @brief	My producer thread object -- does the frame capturing
		CNTV2Card			mDevice;			///< @brief	My CNTV2Card instance. This is what I use to talk to the device.
		NTV2DeviceID		mDeviceID;			///< @brief	My device identifier
		DolbyCaptureConfig	mConfig;			///< @brief	My operating configuration
		NTV2VideoFormat		mVideoFormat;		///< @brief	My video format
		NTV2FormatDesc		mFormatDesc;		///< @brief	Describes my video/pixel format
		NTV2TaskMode		mSavedTaskMode;		///< @brief	Used to restore prior every-frame task mode
		NTV2AudioSystem		mAudioSystem;		///< @brief	The audio system I'm using (if any)
		NTV2FrameDataArray	mHostBuffers;		///< @brief	My host buffers
		FrameDataRingBuffer	mAVCircularBuffer;	///< @brief	My ring buffer object
		bool				mGlobalQuit;		///< @brief	Set "true" to gracefully stop
		uint32_t			mDolbyState;		///< @brief Dolby recovery state
		uint32_t			mDolbyLength;		///< @brief Dolby recovery burst length

};	//	NTV2DolbyCapture

#endif	//	_NTV2CAPTURE_H
