/**
	@file		ntv2stereocapture.h
	@brief		Header file for the NTV2StereoCapture class.
	@copyright	Copyright (C) 2013 AJA Video Systems, Inc. All rights reserved.
**/


#ifndef NTV2STEREOCAPTURE_H
#define NTV2STEREOCAPTURE_H


#include <QtGui>
#include <QThread>

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2task.h"
#include "ntv2utils.h"
#include "ajastuff/common/types.h"
#include "ajastuff/common/videotypes.h"
#include "ajastuff/common/buffer.h"
#include "ajastuff/common/circularbuffer.h"
#include "ajastuff/system/thread.h"
#include "ajastuff/system/process.h"
#include "ajastuff/system/systemtime.h"


/**
	@brief	This structure encapsulates the video buffers used by AutoCirculate.
			In this example, there are a fixed number of them (CIRCULAR_BUFFER_SIZE)
			that comprise the circular buffer used by NTV2StereoCapture's producer/consumer threads.
			The AJACircularBuffer template class greatly simplifies implementing this approach
			to efficiently processing frames.
**/
typedef struct
{
	AJABuffer	leftVideoBuffer;	///	The left video buffer
	AJABuffer	rightVideoBuffer;	///	The right video buffer
} NTV2StereoCaptureBuffer;


const unsigned int	CIRCULAR_BUFFER_SIZE	(10);		//	Specifies how many AVDataBuffers constitute a circular buffer


/**
	@brief	I am a Qt object that knows how to capture and preview stereo video.
**/

class NTV2StereoCapture : public QObject
{
	Q_OBJECT

	//	Instance Methods
	public:
		/**
			@brief	Constructs me using an optional board index number.
			@param	boardNumber			Specifies the zero-based index number of the AJA device to use.
										Defaults to zero, the first device found.
		**/
					NTV2StereoCapture (uint16_t boardNumber = 0);

		virtual		~NTV2StereoCapture ();

		/**
			@brief	Initializes me, and prepares me to preview stereo video using the specified AJA device.
			@param[in]	inBoardScanner	Specifies the AJA board scanner to use.
			@param[in]	inBoardNumber	Specifies the zero-based index number of the AJA device to use.
		**/
        AJAStatus	Init (const CNTV2DeviceScanner & inBoardScanner, const uint32_t inBoardNumber);

		/**
			@brief	Runs me.
			@note	Do not call this method without first calling my Init method.
		**/
		AJAStatus	Run (void);

		/**
			@brief	Gracefully stops me from running.
		**/
		void		Quit (void);

		/**
			@brief	Provides status information about my input (capture) and output (playout) processes.
			@param[out]	leftInputStatus		Receives status information about my (L) input process.
			@param[out]	rightInputStatus	Receives status information about my (R) input process.
		**/
		void		GetACStatus (AUTOCIRCULATE_STATUS_STRUCT & leftInputStatus, AUTOCIRCULATE_STATUS_STRUCT & rightInputStatus);

	signals:
		/**
			@brief	Signals that a new frame is available to preview.
			@param[in]	image	Specifies the QImage to display.
			@param[in]	clear	If true, specifies that the preview pane should be cleared.
		**/
		void		newFrame (const QImage & image, bool clear);

		/**
			@brief	Signals that a new status string is to be displayed.
			@param[in]	status	If true, specifies that the preview pane should be cleared.
		**/
		void		newStatusString (QString status);

	protected:
		/**
			@brief	Sets up everything I need to capture video.
		**/
		AJAStatus	SetupVideo (void);

		/**
			@brief	Sets up board routing for capture.
		**/
		void		RouteInputSignal (void);

		/**
			@brief	Sets up board routing for playout.
		**/
		void		RouteOutputSignal (void);

		/**
			@brief	Sets up my circular buffers.
		**/
		void		SetupHostBuffers (void);

		/**
			@brief	Initializes capture AutoCirculate.
		**/
		void		SetupInputAutoCirculate (void);

		/**
			@brief	Initializes playout AutoCirculate.
		**/
		void		SetupOutputAutoCirculate (void);

		/**
			@brief	Starts my capture (producer) thread.
		**/
		void		StartCaptureThread (void);

		/**
			@brief	Repeatedly captures frames using AutoCirculate (until global quit flag set).
			@param[in]	pThread		Specifies a valid, non-NULL pointer to the capture thread's AJAThread instance.
		**/
		void		CaptureFrames (AJAThread * pThread);

		/**
			@brief	Starts my preview (consumer) thread.
		**/
		void		StartStereoPreviewThread (void);

		/**
			@brief	Repeatedly previews captured frames (until global quit flag set).
			@param[in]	pThread		Specifies a valid, non-NULL pointer to the preview thread's AJAThread instance.
		**/
		void		StereoPreview (AJAThread * pThread);

	//	Class Methods
	protected:
		/**
			@brief	This is the capture thread's static callback function that gets called when the producer thread runs.
					This function gets "Attached" to the producer thread's AJAThread instance.
			@param	pThread		A valid pointer to the capture (producer) thread's AJAThread instance.
			@param	pContext	Context information to pass to the thread.
								(For this application, this will be set to point to the NTV2StereoCapture instance.)
		**/
		static void	CaptureThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the preview thread's static callback function that gets called when the consumer thread runs.
					This function gets "Attached" to the consumer thread's AJAThread instance.
			@param	pThread		A valid pointer to the preview (consumer) thread's AJAThread instance.
			@param	pContext	Context information to pass to the thread.
								(For this application, this will be set to point to the NTV2StereoCapture instance.)
		**/
		static void	StereoPreviewThreadStatic (AJAThread * pThread, void * pContext);

	//	Instance Data
	private:
		AJAThread *					mCaptureThread;			///	My capture (producer) thread
		AJAThread *					mPreviewStereoThread;	///	My preview (consumer) thread
		CNTV2Card					mDevice;				///	My CNTV2Card instance
		NTV2EveryFrameTaskMode		mPreviousFrameServices;	///	Task mode to restore after releasing board
        NTV2DeviceID                mBoardID;				///	My board (model) identifier
		NTV2VideoFormat				mVideoFormat;			///	The format of the video on my board's input
		NTV2FrameBufferFormat		mPixelFormat;			///	My frame buffer pixel format
		bool						mGlobalQuit;			///	Set "true" to gracefully stop
		uint32_t					mVideoBufferSize;		///	Size of my video buffer, in bytes

		NTV2StereoCaptureBuffer							mAVHostBuffer [CIRCULAR_BUFFER_SIZE];	///	My host buffers

		AJACircularBuffer <NTV2StereoCaptureBuffer *>	mAVCircularBuffer;						///	My ring buffer

		AUTOCIRCULATE_TRANSFER_STRUCT					mInput1TransferStruct;					///	My AutoCirculate (L) input transfer struct
		AUTOCIRCULATE_TRANSFER_STRUCT					mInput2TransferStruct;					///	My AutoCirculate (R) input transfer struct
		AUTOCIRCULATE_TRANSFER_STATUS_STRUCT			mInput1TransferStatusStruct;			///	My (L) input status struct
		AUTOCIRCULATE_TRANSFER_STATUS_STRUCT			mInput2TransferStatusStruct;			///	My (R) input status struct

};	//	NTV2StereoCapture

#endif	//	NTV2STEREOCAPTURE_H
