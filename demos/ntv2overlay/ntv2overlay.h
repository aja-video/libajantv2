/* SPDX-License-Identifier: MIT */
/**
	@file	ntv2overlay.h
	@brief	Header file for the NTV2Overlay demonstration class.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2OVERLAY_H
#define _NTV2OVERLAY_H

#include "ntv2card.h"
#include "ntv2formatdescriptor.h"
#include "ntv2democommon.h"
#include "ajabase/system/thread.h"

typedef BurnConfig	OverlayConfig;


/**
	@brief	I output live input video overlaid with an image that has transparency.
**/
class NTV2Overlay
{
	//	Public Instance Methods
	public:
		/**
			@brief		Constructs me using the given configuration settings.
			@param[in]	inConfig		Specifies the configuration parameters.
			@note		I'm not completely initialized and ready for use until after my Init method has been called.
		**/
							NTV2Overlay (const OverlayConfig & inConfig);
		virtual				~NTV2Overlay ();

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
		
		virtual AJAStatus		SetupHostBuffers (void);

		/**
			@brief	Sets up my overlay "bug" image buffer.
		**/
		virtual AJAStatus		SetupOverlayBug (void);

		/**
			@brief		Wait for a stable input signal, and return it.
			@return		The input video format. Guaranteed to be valid unless terminating.
		**/
		virtual NTV2VideoFormat	WaitForStableInputSignal (void);

		/**
			@brief	Performs input widget signal routing.
			@return	True if successful.
		**/
		virtual bool			RouteInputSignal (void);

		/**
			@brief	Performs output widget signal routing.
			@return	True if successful.
		**/
		virtual bool			RouteOutputSignal (void);

		/**
			@brief	Starts my playout thread.
		**/
		virtual void			StartOutputThread (void);

		/**
			@brief	Repeatedly plays out frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			OutputThread (void);

		/**
			@brief	Starts my capture thread.
		**/
		virtual void			StartInputThread (void);

		/**
			@brief	Repeatedly captures frames using AutoCirculate (until global quit flag set).
		**/
		virtual void			InputThread (void);

		/**
			@returns	The zero-based index number of the Mixer/Keyer widget to use.
		**/
		virtual inline UWord	MixerNum (void)				{return mConfig.fInputChannel / 2;}


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the playout thread's static callback function that gets called when the playout thread runs.
					This function gets "Attached" to the playout thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the playout thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Overlay instance.)
		**/
		static void				OutputThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the capture thread's static callback function that gets called when the capture thread runs.
					This function gets "Attached" to the AJAThread instance.
			@param[in]	pThread		Points to the AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2Overlay instance.)
		**/
		static void				InputThreadStatic (AJAThread * pThread, void * pContext);

	//	Private Member Data
	private:
		typedef AJACircularBuffer<NTV2FrameData*>	CircularBuffer;
		OverlayConfig			mConfig;			///< @brief	My configuration info
		AJAThread				mPlayThread;		///< @brief	My output thread object
		AJAThread				mCaptureThread;		///< @brief	My input thread object
		CNTV2Card				mDevice;			///< @brief	My CNTV2Card instance
		NTV2VideoFormat			mVideoFormat;		///< @brief	Input video format (for change detection)
		NTV2LHIHDMIColorSpace	mHDMIColorSpace;	///< @brief	HDMI input colorspace (for change detection)
		NTV2Buffer				mBug;				///< @brief	Overlay "bug" image buffer
		NTV2RasterInfo			mBugRasterInfo;		///< @brief	Overlay "bug" raster info
		NTV2TaskMode			mSavedTaskMode;		///< @brief	For restoring prior state
		NTV2XptConnections		mInputConnections;	///< @brief	My capture routing connections
		NTV2XptConnections		mOutputConnections;	///< @brief	My output routing connections
		CircularBuffer			mFrameDataRing;		///< @brief	AJACircularBuffer that controls frame data access by producer/consumer threads
		bool					mGlobalQuit;		///< @brief	Set "true" to gracefully stop

};	//	NTV2Overlay

#endif	//	_NTV2OVERLAY_H
