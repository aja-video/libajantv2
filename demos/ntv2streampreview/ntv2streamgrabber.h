/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streamgrabber.h
	@brief		Header file for the NTV2StreamGrabber class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2STREAMGRABBER_H
#define NTV2STREAMGRABBER_H

#include <QBasicTimer>
#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	#include <QtWidgets>
#else
	#include <QtGui>
#endif
#include <QThread>
#include <QtMultimedia>
#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2task.h"
#include "ajabase/common/types.h"
#include "ajabase/system/process.h"
#if defined (INCLUDE_AJACC)
	#include "ajacc/includes/ntv2captiondecoder608.h"
	#include "ajacc/includes/ntv2captiondecoder708.h"
#endif	//	INCLUDE_AJACC

#define STREAMPREVIEW_WIDGET_X (960)
#define STREAMPREVIEW_WIDGET_Y (540)



/**
	@brief	A QThread that captures audio/video from NTV2-compatible AJA devices and uses Qt signals to emit ARGB video frames.
			To simplify things, I assume the input signal is YCbCr, and the input goes through a color space converter (CSC) to an
			ARGB FrameStore. I can also output 2 channels of Audio to the host audio system using Qt's QAudioOutput device.
**/

class NTV2StreamGrabber : public QThread
{
	Q_OBJECT

	//	Instance Methods
	public:
		/**
			@brief	Constructs me.
			@param[in]	pInParentObject		Optionally specifies my parent object. Defaults to NULL (no parent).
		**/
						NTV2StreamGrabber (QObject * pInParentObject = NULL);

		virtual			~NTV2StreamGrabber ();		///< @brief	My destructor.

		/**
			@brief	Sets the input to be used for capture on the AJA device being used.
			@param[in]	inInputSource	Specifies the input source to be used.
		**/
		void			SetInputSource (const NTV2InputSource inInputSource);

		/**
			@brief	Sets the AJA device to be used for capture.
			@param[in]	inDeviceIndex	Specifies the zero-based index number of the device to be used.
		**/
		void			SetDeviceIndex (const UWord inDeviceIndex);

		UWord			GetDeviceIndex (void) const;

        void            SetFixedReference(bool fixed)                                   {mbFixedReference = fixed;}

	protected:

	signals:
		/**
			@brief	This is signaled (called) when a new frame has been captured and is available for display.
			@param[in]	inImage		A QImage that contains the frame image.
			@param[in]	inClear		True if a redraw should take place -- i.e., if the frame is the first of a valid video stream,
									or if there is currently no valid video.
		**/
		void			newFrame (const QImage &inImage, const bool inClear);

		/**
			@brief	This is signaled (called) when my status string changes.
			@param[in]	inStatus	The QString containing the status message.
		**/
		void			newStatusString (const QString & inStatus);

	private slots:

	protected:
		virtual void	run (void);					///< @brief	My thread function.

		bool			SetupInput (void);			///< @brief	Configures my AJA device for capture
		void			StopStream (void);			///< @brief	Stops capturing

		bool CheckForValidInput (void);

		NTV2VideoFormat GetVideoFormatFromInputSource (void);
        NTV2LHIHDMIColorSpace GetColorSpaceFromInputSource (void);

		bool IsInput3Gb (const NTV2InputSource inputSource);

	//	Instance Data
	private:
		bool					mRestart;				///< @brief	Set true to reconfigure me and restart AutoCirculate
		bool					mAbort;					///< @brief	Used in my destructor to immediately cause me to exit
        bool					mbFixedReference;
		CNTV2Card				mNTV2Card;				///< @brief	Used to talk to monitor & control the device
		UWord					mBoardNumber;			///< @brief	Index number of the device I'm using
		NTV2DeviceID			mDeviceID;				///< @brief	Device ID of the device I'm using
        NTV2Channel				mChannel;				///< @brief	Video channel
        NTV2Channel				mStream;				///< @brief	Stream channel
		NTV2VideoFormat			mCurrentVideoFormat;	///< @brief	Current video format seen on selected device input
        NTV2LHIHDMIColorSpace	mCurrentColorSpace;		///< @brief Current color space seen on selected device input
		NTV2VideoFormat			mLastVideoFormat;		///< @brief	Used to detect input video format changes
		ULWord					mDebounceCounter;		///< @brief	Used for detecting stable input video
		bool					mFormatIsProgressive;	///< @brief	True if input video format is progressive (not interlaced)
		NTV2InputSource			mInputSource;			///< @brief	User-selected input source
		NTV2FrameDimensions		mFrameDimensions;		///< @brief	Frame dimensions, pixels X lines
		NTV2FrameBufferFormat	mFrameBufferFormat;		///< @brief	My frame buffer format
		NTV2TaskMode			mSavedTaskMode;			///< @brief	Used to restore the previous task mode
		bool					mDoMultiChannel;		///< @brief	Share the board with other processes?

};	//	class NTV2StreamGrabber


#endif	//	NTV2STREAMGRABBER_H
