/* SPDX-License-Identifier: MIT */
/**
	@file	ntv2burn.h
	@brief	Header file for the NTV2Burn demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2BURN_H
#define _NTV2BURN_H

#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include "ntv2democommon.h"
#include "ajabase/common/types.h"
#include "ajabase/common/circularbuffer.h"
#include "ajabase/system/thread.h"
#include "ajabase/common/timecodeburn.h"
#include "ajabase/system/info.h"

/**
	@brief	Configures an NTV2Player instance.
**/
typedef struct BurnConfig
{
	public:
		std::string						fDeviceSpec;		///< @brief	The AJA device to use
		NTV2Channel						fInputChannel;		///< @brief	The input channel to use
		NTV2Channel						fOutputChannel;		///< @brief	The output channel to use
		NTV2InputSource					fInputSource;		///< @brief	The device input connector to use
		CNTV2DemoCommon::ACFrameRange	fInputFrames;		///< @brief	Ingest frame count or range
		CNTV2DemoCommon::ACFrameRange	fOutputFrames;		///< @brief	Playout frame count or range
		NTV2PixelFormat					fPixelFormat;		///< @brief	The pixel format to use
		NTV2TCIndex						fTimecodeSource;	///< @brief	Timecode source to use
		bool							fDoMultiFormat;		///< @brief	If true, enables device-sharing;  otherwise takes exclusive control of the device.
		bool							fSuppressAudio;		///< @brief	If true, suppress audio;  otherwise include audio
		bool							fSuppressVideo;		///< @brief	If true, suppress video;  otherwise include video
		bool							fSuppressAnc;		///< @brief	If true, suppress anc;  otherwise include anc

		/**
			@brief	Constructs a default Player configuration.
		**/
		inline explicit	BurnConfig (const std::string & inDeviceSpecifier	= "0")
			:	fDeviceSpec			(inDeviceSpecifier),
				fInputChannel		(NTV2_CHANNEL1),
				fOutputChannel		(NTV2_CHANNEL3),
				fInputSource		(NTV2_INPUTSOURCE_SDI1),
				fInputFrames		(7),
				fOutputFrames		(7),
				fPixelFormat		(NTV2_FBF_8BIT_YCBCR),
				fTimecodeSource		(NTV2_TCINDEX_SDI1),
				fDoMultiFormat		(false),
				fSuppressAudio		(false),
				fSuppressVideo		(false),
				fSuppressAnc		(false)
		{
		}

		inline bool	WithAudio(void) const		{return !fSuppressAudio;}	///< @return	True if streaming audio, false if not.
		inline bool	WithVideo(void) const		{return !fSuppressVideo;}	///< @return	True if streaming video, false if not.
		inline bool	WithAnc(void) const			{return !fSuppressAnc;}		///< @return	True if streaming audio, false if not.
		inline bool WithTimecode(void) const	{return NTV2_IS_VALID_TIMECODE_INDEX(fTimecodeSource);}	///< @return	True if valid TC source

		/**
			@brief		Renders a human-readable representation of me.
			@param[in]	inCompact	If true, setting values are printed in a more compact form. Defaults to false.
			@return		A list of label/value pairs.
		**/
		AJALabelValuePairs Get (const bool inCompact = false) const;

}	BurnConfig;

/**
	@brief		Renders a human-readable representation of a BurnConfig into an output stream.
	@param		strm	The output stream.
	@param[in]	inObj	The configuration to be rendered into the output stream.
	@return		A reference to the specified output stream.
**/
inline std::ostream &	operator << (std::ostream & strm, const BurnConfig & inObj)	{return strm << AJASystemInfo::ToString(inObj.Get());}


/**
	@brief	Instances of me can capture frames from a video signal provided to an input of an AJA device,
			burn timecode into those frames, then deliver them to an output of the same AJA device, all in real time.
			I make use of the AJACircularBuffer, which simplifies implementing a producer/consumer model,
			in which a "consumer" thread delivers burned-in frames to the AJA device output, and a "producer"
			thread captures raw frames from the AJA device input.
			I also demonstrate how to detect if an SDI input has embedded timecode, and if so, how AutoCirculate
			makes it available. I also show how to embed timecode into an SDI output signal using AutoCirculate
			during playout.
**/

class NTV2Burn
{
	//	Public Instance Methods
	public:
		/**
			@brief		Constructs me using the given configuration settings.
			@param[in]	inConfig		Specifies the configuration parameters.
			@note		I'm not completely initialized and ready for use until after my Init method has been called.
		**/
							NTV2Burn (const BurnConfig & inConfig);
		virtual				~NTV2Burn ();

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
			@param[out]	outInputStatus		Receives the input status.
			@param[out]	outOutputStatus		Receives the output status.
		**/
		virtual void			GetStatus (AUTOCIRCULATE_STATUS & outInputStatus, AUTOCIRCULATE_STATUS & outOutputStatus);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up everything I need for capturing and playing video.
		**/
		virtual AJAStatus		SetupVideo (void);

		/**
			@brief	Sets up everything I need for capturing and playing audio.
		**/
		virtual AJAStatus		SetupAudio (void);

		/**
			@brief	Sets up board routing for capture.
		**/
		virtual void			RouteInputSignal (void);

		/**
			@brief	Sets up board routing for playout.
		**/
		virtual void			RouteOutputSignal (void);

		/**
			@brief	Sets up my circular buffers.
		**/
		virtual AJAStatus		SetupHostBuffers (void);

		/**
			@brief	Starts my playout thread.
		**/
		virtual void			StartPlayThread (void);

		/**
			@brief	Repeatedly plays out frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			PlayFrames (void);

		/**
			@brief	Starts my capture thread.
		**/
		virtual void			StartCaptureThread (void);

		/**
			@brief	Repeatedly captures frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			CaptureFrames (void);


		/**
			@brief	Returns true if the current input signal has timecode embedded in it; otherwise returns false.
		**/
		virtual bool			InputSignalHasTimecode (void);


		/**
			@brief	Returns true if there is a valid LTC signal on my device's primary analog LTC input port; otherwise returns false.
		**/
		virtual bool			AnalogLTCInputHasTimecode (void);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the playout thread's static callback function that gets called when the playout thread runs.
					This function gets "Attached" to the playout thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the playout thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Burn instance.)
		**/
		static void				PlayThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the capture thread's static callback function that gets called when the capture thread runs.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Burn instance.)
		**/
		static void				CaptureThreadStatic (AJAThread * pThread, void * pContext);


	//	Private Member Data
	private:
		typedef AJACircularBuffer<NTV2FrameData*>	CircularBuffer;
		BurnConfig			mConfig;			///< @brief	My configuration info
		AJAThread			mPlayThread;		///< @brief	My playout thread object
		AJAThread			mCaptureThread;		///< @brief	My capture thread object
		CNTV2Card			mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID		mDeviceID;			///< @brief	Keep my device ID handy
		NTV2VideoFormat		mVideoFormat;		///< @brief	Format of video being ingested & played
		NTV2FormatDesc		mFormatDesc;		///< @brief	Describes raster images
		NTV2TaskMode		mSavedTaskMode;		///< @brief	For restoring prior state
		NTV2OutputDest		mOutputDest;		///< @brief	The desired output connector to use
		NTV2AudioSystem		mAudioSystem;		///< @brief	The audio system I'm using
		AJATimeCodeBurn		mTCBurner;			///< @brief	My timecode burner
		NTV2TCIndexes		mTCOutputs;			///< @brief	My output timecode destinations
		NTV2FrameDataArray	mHostBuffers;		///< @brief	My host buffers
		CircularBuffer		mFrameDataRing;		///< @brief	AJACircularBuffer that controls frame data access by producer/consumer threads
		bool				mGlobalQuit;		///< @brief	Set "true" to gracefully stop

};	//	NTV2Burn

#endif	//	_NTV2BURN_H
