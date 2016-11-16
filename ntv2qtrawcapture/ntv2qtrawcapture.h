/**
	@file		ntv2qtrawcapture.h
	@brief		Header file for NTV2QtRawCapture demo application.
				Demonstrates how to capture raw video from devices like the AJA CION camera.
	@copyright	Copyright 2014 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef NTV2QTRAWCAPTURE_H
#define NTV2QTRAWCAPTURE_H


#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif


//	Includes
#include "ntv2devicescanner.h"

/**
	@page	ntv2qtrawcapture		NTV2QtRawCapture Demo

	This Qt-based GUI demonstration application captures (and previews) raw video from the AJA CION camera
	into DNG files on the host. It demonstrates...
	-	how to use the new NTV2_FBF_10BIT_RAW_YCBCR and NTV2_FBF_10BIT_RAW_RGB raw formats;
	-	how to use the AJARawPreviewWidget, a reusable Qt-based GUI element that displays realtime video
		that's been grabbed from an NTV2RawFrameGrabber object;
	-	how to use the NTV2RawFrameGrabber class, a QThread that captures a raw video stream from the AJA CION,
		and uses Qt signals to emit video frames.
**/


//	Forward Declarations
class AJAPreviewWidget;
class NTV2RawFrameGrabber;


/**
	@brief	I am a QDialog that captures raw video from an AJA CION camera, displays a debayered image on
			the computer monitor, and writes the frames captured to a storage device as DNG files.
**/
class NTV2QtRawCapture : public QDialog
{
	Q_OBJECT


	//	Public Instance Methods
	public:
        NTV2QtRawCapture	(QWidget * parent = NULL, Qt::WindowFlags flags = 0);
		~NTV2QtRawCapture 	();


	//	Private Instance Methods
	private:
		/**
			@brief	Restores the user interface comtrols from a previously saveed state.
		**/
		void	RecallSettings	(void);

		/**
			@brief	Saves the state of the user interface controls to non-volitile storage.
		**/
		void	SaveSettings	(void);

		/**
			@brief	Used to animate controls on the user interface.
		**/
		void	TimerEvent		(QTimerEvent * inEvent);


	private slots:
		/**
			@brief	This is called when the user selects a different device from the device selector
					popup menu.
			@param	inBoardIndexNum			Specifies which item in the device selector popup menu was chosen,
											where 0 is the first item.
		**/
		void	RequestBoardChange			(const int inBoardIndexNum);

		/**
			@brief	These are called when the user clicks on the "+" button to the right if the record folder
					combo box.  The user is prompted to select a new folder by the parameterless version of
					the call, and the selected folder's path is added to the list of folders in the combo box
					by the second version of the call.
			@param	inFolderString			Specifies the path to a new folder to be added to the combo box.
		**/
		void	AddRecordFolder				(void);
		void	AddRecordFolder				(const QString inFolderString);

		/**
			@brief	This is called when the user clicks on the "-" button to the right if the curremt record
					folder combo box.  It will remove the currently selected  entry from the list of folders
					in the combo box.
		**/
		void	DeleteRecordFolder			(void);

		/**
			@brief	This is called when the user chooses a different path from the record folder combo box.
			@param	inNewRecordFolderIndex	Specifies which item in the record folder combo box was chosen,
											where 0 is the first item.
		**/
		void	NewRecordFolderChoice		(const int inNewRecordFolderIndex);

		/**
			@brief	This is called when the user clicks on the Record or Stop buttons.
		**/
		void 	RecordStateChanged 			(void);

		/**
			@brief	This is called when the user clicks on the Preview When Idle button.
					When checked, the camera input will be displayed on the computer monitor, even when not
					recording.  When unchecked, the camera input will be displayed when resording.  Otherwise,
					an AJA logo will be displayed.
		**/
		void 	PreviewWhenIdleChanged		(const int inState);

		/**
			@brief	This is called when the user clicks on the Increment Sequence button.
					When checked, the names of DNG files written to storage will have the numeric part of the
					file name incremented by one each frame.  When unchecked, the numeric part will always be
					zero, thus the same file will be written over and over during a record.  This can be useful
					for diagnosing storage bandwidth issues without filling up the media with files.
		**/
		void 	IncrementSequenceChanged	(const int inState);


	//	Instance Data
	private:

		//	Qt user interface related variables
		QDialogButtonBox *		mCloseBox;					///< @brief	My "Close" button
		QComboBox *				mBoardChoiceCombo;			///< @brief	My device selector popup menu
		AJAPreviewWidget *		mVideoPreviewWidget;		///< @brief	My AJAPreviewWidget

		QRadioButton *			mStopRadioButton;			///< @brief	My "Stop" choice
		QRadioButton *			mRecordRadioButton;			///< @brief	My "Record" choice

		QCheckBox * 			mPreviewWhenIdleCheckBox;	///< @brief	My checkbox to control previewing
		QCheckBox *				mIncrementSequenceCheckBox;	///< @brief	My checkbox to control file name generation

		QComboBox *				mRecordFolderComboBox;		///< @brief	My record folder selection
		QPushButton *			mAddRecordFolderButton;		///< @brief	My control to add a record folder
		QPushButton *			mDeleteRecordFolderButton;	///< @brief	My control to remove a record folder

		int						mTimerID;					///< @brief	My timer ID
		CNTV2DeviceScanner		mNTV2Scanner;				///< @brief	My AJA device scanner
		NTV2RawFrameGrabber *	mRawFrameGrabber;			///< @brief	My frame grabber

};	//	class NTV2QtRawCapture

#endif	//	NTV2QTRAWCAPTURE_H

