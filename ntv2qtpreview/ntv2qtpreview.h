/**
	@file		ntv2qtpreview.h
	@brief		Header file for NTV2QtPreview demo application.
				Demonstrates how to capture audio/video from NTV2-based AJA devices.
	@copyright	(C) 2013-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2QTPREVIEW_H
#define NTV2QTPREVIEW_H


#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif

#include "ajapreviewwidget.h"
#include "ntv2framegrabber.h"
#include "ntv2devicescanner.h"
#include "ajabase/pnp/pnp.h"


/**
	@brief	I am a QDialog that displays, in a two-up layout, whatever stereo video is present on the
			SDI inputs of an AJA device. I also have a popup menu for choosing a different capture device.
**/
class NTV2QtPreview : public QDialog
{
	Q_OBJECT

	//	Instance Methods
	public:
        NTV2QtPreview (QWidget * parent = NULL, Qt::WindowFlags flags = 0);
		~NTV2QtPreview ();

	//	Private Instance Methods
	private:
		void timerEvent (QTimerEvent * event);
		void updateInputs (void);

		static void	PnpCallback (AJAPnpMessage inMessage, void * pUserData);

	private slots:
		/**
			@brief	This gets called when the user selects a different device from the device selector popup menu.
			@param	inDeviceIndexNum	Specifies which item in the device selector popup menu was chosen,
										where 0 is the first item.
		**/
		void	RequestDeviceChange (const int inDeviceIndexNum);

		void	devicesChanged ();				///< @brief	This gets called when an AJA device is attached or detached to/from the host.
		void	inputChanged (int id);			///< @brief	This gets called when a different NTV2InputSource is selected by the user.
		void	withAudioChanged (int state);	///< @brief	This gets called when the Audio checkbox is toggled.
		void	checkFor4kChanged (int state);	///< @brief	This gets called when the 4K checkbox is toggled.

	//	Instance Data
	private:
		QDialogButtonBox *		mCloseBox;					///< @brief	My "Close" button
		QCheckBox *				mWithAudioCheckBox;			///< @brief	My "with audio" checkbox
		QCheckBox *				mCheckFor4kCheckBox;		///< @brief	My "check for 4K" checkbox
		QComboBox *				mBoardChoiceCombo;			///< @brief	My device selector popup menu
		QButtonGroup *			mInputButtonGroup;			///< @brief	My input selection radio buttons (Off|SDI1|2|3|4|5|6|7|8|HDMI|Analog)
		AJAPreviewWidget *		mVideoPreviewWidget;		///< @brief	My AJAPreviewWidget
		AJAPnp					mPnp;						///< @brief	Used to detect device removal/addition
		QPixmap					mPixmap;					///< @brief	My QPixmap
		int						mTimerID;					///< @brief	My timer ID
		NTV2FrameGrabber *		mFrameGrabber;				///< @brief	My frame grabber
		#if defined (INCLUDE_AJACC)
			QButtonGroup *		mCaptionButtonGroup;		///< @brief	My caption channel radio buttons (Off|CC1|CC2|CC3|CC4|TXT1|TXT2|TXT3|TXT4)
		#endif	//	defined (INCLUDE_AJACC)

};	//	class NTV2QtPreview

#endif	//	NTV2QTPREVIEW_H
