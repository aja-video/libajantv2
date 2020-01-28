/**
	@file		ntv2framegrabber.h
	@brief		Header file for the NTV2RawFrameGrabber class.
	@copyright	(C) 2013-2020 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef NTV2RAWFRAMEGRABBER_H
#define NTV2RAWFRAMEGRABBER_H


#include <QBasicTimer>
#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QThread>
#include <QtMultimedia>


//	Includes
#include "ajabase/common/circularbuffer.h"
#include "ajabase/common/types.h"
#include "ntv2card.h"
#include "ntv2democommon.h"


//	Forward Declarations
class AJAThread;
class NTV2RawDngWriter;


/**
	@brief	I am a QThread that captures video in raw format from NTV2-compatible AJA camers
			and uses Qt signals to emit ARGB video frames.
**/

class NTV2RawFrameGrabber : public QThread
{
	Q_OBJECT

	//	Public Instance Methods
	public:
		NTV2RawFrameGrabber						(QObject * parent = NULL);
		~NTV2RawFrameGrabber					(void);

		/**
			@brief	Returns the zero-based index of the device currently selected.
		**/
		virtual UWord GetDeviceIndex			(void) const;

		/**
			@brief	Specifies from which device to capture the raw video.
			@param[in]	inDeviceIndex		The zero-based index of the device to use.
		**/
		virtual void SetDeviceIndex				(const UWord inDeviceIndex);

		/**
			@brief	Specifies how to generate the sequence number of the file names of captured frames.
			@param[in]	inIncrementSequence	If true, the sequence part of the filename will be incremnted
											by one for each frame captured.  If false, the sequence number
											will be held constant, and the same file name will be used
											for each frame captured.
		**/
		virtual void SetIncrementSequence		(const bool inIncrementSequence);

		/**
			@brief	Specifies a timecode string to be displayed in the user interface.
			@param[in]	inBytes				A pointer to a debayered ARGB image captured from the input device.
			@param[in]	inSize				The number of bytes in the ARGB image.
			@note	This function will emit the frame to the preview window widget
		**/
		virtual void SetNewFrame				(const uint8_t * inBytes, const uint32_t inSize);

		/**
			@brief	Specifies a status message to be displayed in the user interface.
			@param[in]	inStatusString		A string containing the statuw to display.
			@note	This function will emit a new status string to the preview window widget
		**/
		virtual void SetNewStatusString		(const std::string 	inStatusString);

		/**
			@brief	Specifies what to display in the application's preview window.
			@param[in]	inPreviewWhenIdle	If true, displays the input video in the prewiew window, even
											when not capturing to the storage device.  If false, the AJA
											logo will be displayed when not capturing.
		**/
		virtual void SetPreviewWhenIdle 		(const bool inPreviewWhenIdle);

		/**
			@brief	Sets the absolute path to the folder where captured frames will be written.
			@param[in]	inFilePath			A string containing an absolute path to the folder.
		**/
		virtual void SetRecordPath				(const std::string inFilePath);

		/**
			@brief	Specifies if captured frames should be written to storage or not.
			@param[in]	inRecording			If true, captured frames will be written to files on the
											storage medium;  otherwise, they won't be written.
		**/
		virtual void SetRecording				(const bool inRecording);


	signals:
		//	Used by the Qt framework to connect events to function calls.
		void NewFrame							(const QImage & image, bool clear);
		void NewStatusString					(QString status);


	//	Protected Instance Methods
	protected:
		//	Used by the Qt framework to call the worker function for this class in its own thread
		virtual void run						(void);


	//	Private Instance Methods
	private:
		/**
			@brief	Returns true if the SDI inputs are receiving a valid and stable input signal, else false.
		**/
		bool CheckForValidInput					(void);

		/**
			@return	The video format present on the SDI input source, or NTV2_FORMAT_UNKNOWN if absent.
			@note	Sets the member variable mInputLinkCount to the number of inputs with the format of SDI 1.
		**/
		NTV2VideoFormat GetInputVideoFormat		(void);

		/**
			@brief	Configures the SDK's autocirculate mechanism for use.
		**/
		void SetupAutoCirculate					(void);

		/**
			@brief	Allocates buffers in host memory to hold raw video frames, and attaches them to a circular buffer.
		**/
		bool SetupHostBuffers					(void);

		/**
			@brief	Configures the device's SDI inputs and routng for capturing raw video frames.
		**/
		bool SetupInput							(void);

		/**
			@brief	Starts a worker thread to write captured frames to storage, and another to display frames on the UI.
		**/
		void StartWorkerThreads					(void);

		/**
			@brief	Stops the autocirculate mechanism, which terminates the capture of raw video frames.
		**/
		void StopAutoCirculate					(void);

		/**
			@brief	Terminates the worker threads to write the frames to storage and display them on the UI.
		**/
		void StopWorkerThreads					(void);


	//	Private Instance Data
	private:
		CNTV2Card								mDevice;										///< @brief	My CNTV2Card instance
		UWord									mDeviceIndex;									///< @brief	Index number of the device I'm using
		NTV2DeviceID							mDeviceID;										///< @brief	My device (model) identifier

		NTV2Channel								mPrimaryChannel;								///< @brief	My input channel for the primary video stream
		NTV2Channel								mSecondaryChannel;								///< @brief	My input channel for secondary  high frame rate video

		NTV2VideoFormat							mCurrentVideoFormat;							///< @brief	The video format currently being received
		NTV2VideoFormat							mLastVideoFormat;								///< @brief	The last vaild video format that was received
		ULWord									mVideoFormatDebounceCounter;					///< @brief	A counter to ensure a stable input is present

		NTV2InputSource							mInputSource;									///< @brief	My video input source (always NTV2_INPUTSOURCE_SDI1)
		NTV2FrameDimensions						mFrameDimensions;								///< @brief	The width & height of the preview window (in pixels & lines)
		NTV2FrameBufferFormat					mFrameBufferFormat;								///< @brief	The frame buffer format needed to capture the raw video

		NTV2EveryFrameTaskMode					mPreviousFrameServices;							///< @brief	We will use this to restore the previous state

		bool									mRecording;										///< @brief	My flag to indicate is recording is active or not
		bool									mPreviewWhenIdle;								///< @brief	My flag to display frames on the UI when not recording
		bool									mIncrementSequence;								///< @brief	My flad to increment the sequence number of captured frames
		std::string								mRecordPath;									///< @brief	My current path to the record folder

		uint64_t								mInputVideoWidth;								///< @brief	The width of the input video as specified by the VPID
		uint64_t								mInputLinkCount;								///< @brief	My count of the number of SDI links to capture

		bool									mAbort;											///< @brief	Set "true" to allow circular buffers to terminate
		bool									mGlobalQuit;									///< @brief	Set "true" to gracefully stop
		bool									mRestart;										///< @brief	Set "true" to reconfigure device for a new format

		NTV2RawDngWriter *						mDngWriterThread;								///< @brief	My consumer to write files to the storage device
		AVDataBuffer							mDngHostBuffer [CIRCULAR_BUFFER_SIZE];			///< @brief	My array of host buffers for the file writer
		AJACircularBuffer <AVDataBuffer *>		mDngCircularBuffer;								///< @brief	My ring buffer object for the file writer

		AJAThread *								mPreviewThread;									///< @brief	My consumer to display ARGB frames on the UI
		AVDataBuffer							mPreviewHostBuffer [CIRCULAR_BUFFER_SIZE];		///< @brief	My array of host buffers for UI preview
		AJACircularBuffer <AVDataBuffer *>		mPreviewCircularBuffer;							///< @brief	My ring buffer object for the UI preview

};	//	class NTV2RawFrameGrabber

#endif	//	NTV2RAWFRAMEGRABBER_H

