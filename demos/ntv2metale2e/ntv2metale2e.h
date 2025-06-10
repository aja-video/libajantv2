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
	@brief	Outputs live input video overlaid with image having transparency.
**/
class NTV2Overlay
{
	//	Public Instance Methods
	public:
						NTV2Overlay (const OverlayConfig & inConfig);	///< @brief	Construct from OverlayConfig
		virtual			~NTV2Overlay ();				///< @brief	My destructor
		AJAStatus		Init (void);					///< @brief	Prepares me to Run()
		AJAStatus		Run (void);						///< @brief	Runs me (only after Init called)
		void			Quit (void);					///< @brief	Gracefully stops me

	//	Protected Instance Methods
	protected:
		AJAStatus		SetupVideo (void);				///< @brief	Performs all video setup
		AJAStatus		SetupAudio (void);				///< @brief	Performs all audio setup
		AJAStatus		SetupCaptureBuffers (void);		///< @brief	Allocates capture buffers & ring
		void			ReleaseCaptureBuffers (void);	///< @brief	Frees capture buffers & ring
		AJAStatus		SetupOverlayBug (void);			///< @brief	Sets up overlay "bug"
		NTV2VideoFormat	WaitForStableInputSignal (void);///< @brief	Waits for stable input signal
		bool			IsInputSignalRGB (void);		///< @returns	true if input signal is RGB
		void			RouteInputSignal (void);		///< @brief	Performs input routing
		void			RouteOverlaySignal (void);		///< @brief	Performs overlay routing
		void			RouteOutputSignal (void);		///< @brief	Performs output routing
		void			StartOutputThread (void);		///< @brief	Starts output thread
		void			OutputThread (void);			///< @brief	The output/playout thread function
		void			StartInputThread (void);		///< @brief	Starts input thread
		void			InputThread (void);				///< @brief	The input/capture thread function
		inline UWord	MixerNum (void)	{return mConfig.fInputChannel / 2;}	///< @returns	zero-based Mixer index number

	//	Protected Class Methods
	protected:
		static void	OutputThreadStatic (AJAThread * pThread, void * pInstance);	/// @brief	Static output/playout thread function
		static void	InputThreadStatic (AJAThread * pThread, void * pInstance);	/// @brief	Static input/capture thread function

	//	Private Member Data
	private:
		OverlayConfig			mConfig;			///< @brief	My configuration info
		AJAThread				mPlayThread;		///< @brief	My output thread object
		AJAThread				mCaptureThread;		///< @brief	My input thread object
		CNTV2Card				mDevice;			///< @brief	My CNTV2Card instance
		NTV2VideoFormat			mVideoFormat;		///< @brief	Input video format (for change detection)
		NTV2Buffer				mBug;				///< @brief	Overlay "bug" image buffer
		NTV2RasterInfo			mBugRasterInfo;		///< @brief	Overlay "bug" raster info
		NTV2TaskMode			mSavedTaskMode;		///< @brief	For restoring prior state
		NTV2FrameDataArray		mBuffers;			///< @brief	Buffers used in mAVCircularBuffer
		FrameDataRingBuffer		mAVCircularBuffer;	///< @brief	AJACircularBuffer (producer/consumer buffer ring)
		const NTV2PixelFormat	mInputPixFormat;	///< @brief	Capture buffer pixel format
		const NTV2PixelFormat	mOutputPixFormat;	///< @brief	Output buffer pixel format
		bool					mGlobalQuit;		///< @brief	Set "true" to gracefully stop

};	//	NTV2Overlay

#endif	//	_NTV2OVERLAY_H
