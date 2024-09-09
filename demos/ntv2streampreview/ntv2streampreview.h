/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streampreview.h
	@brief		Header file for NTV2StreamPreview demo application.
				Demonstrates how to capture audio/video from NTV2-based AJA devices.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2STREAMPREVIEW_H
#define NTV2STREAMPREVIEW_H


#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif

#include "ajapreviewwidget.h"
#include "ntv2streamgrabber.h"
#include "ntv2devicescanner.h"
#include "ajabase/pnp/pnp.h"


/**
	@brief	I am a QDialog that displays whatever video is present on the SDI inputs of an AJA streaming device.
			I also have a popup menu for choosing a different device, and a button group for choosing a different SDI input.
**/
class NTV2StreamPreview : public QDialog
{
	Q_OBJECT

	//	Instance Methods
	public:
        NTV2StreamPreview (QWidget * parent = AJA_NULL, Qt::WindowFlags flags = Qt::WindowFlags());
		~NTV2StreamPreview ();

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
        void	fixedRefChanged (bool checked);

	//	Instance Data
	private:
		QDialogButtonBox *		mCloseBox;					///< @brief	My "Close" button
        QCheckBox *				mCheckFixedReference;
        QComboBox *				mBoardChoiceCombo;			///< @brief	My device selector popup menu
		QButtonGroup *			mInputButtonGroup;			///< @brief	My input selection radio buttons (Off|SDI1|2|3|4|5|6|7|8|HDMI|Analog)
		AJAPreviewWidget *		mVideoPreviewWidget;		///< @brief	My AJAPreviewWidget
		AJAPnp					mPnp;						///< @brief	Used to detect device removal/addition
		QPixmap					mPixmap;					///< @brief	My QPixmap
		int						mTimerID;					///< @brief	My timer ID
		NTV2StreamGrabber *		mStreamGrabber;				///< @brief	My frame grabber

};	//	class NTV2StreamPreview

#endif	//	NTV2STREAMPREVIEW_H
