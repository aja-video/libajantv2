/**
	@file		ntv2qtstereopreview.h
	@brief		Header file for NTV2QtStereoPreview demo application.
	@copyright	(C) 2013-2020 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef NTV2QTSTEREOPREVIEW_H
#define NTV2QTSTEREOPREVIEW_H

#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif

#include <QDialog>
#include "ajatypes.h"
#include "ajapreviewwidget.h"
#include "ntv2card.h"
#include "ntv2devicescanner.h"
#include "ntv2stereocapture.h"


/**
	@brief	I am a QDialog that displays, in a two-up layout, whatever stereo video is present on the
			SDI inputs of an AJA device.
**/
class NTV2QtStereoPreview : public QDialog
{
	Q_OBJECT

	//	Instance Methods
	public:
        NTV2QtStereoPreview (QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~NTV2QtStereoPreview ();

	public slots:
		/**
			@brief	This gets called when the user selects a different device from the device selector
					popup menu.
			@param	inBoardIndexNum		Specifies which item in the device selector popup menu was chosen,
										where 0 is the first item.
		**/
		void	RequestBoardChange (const int inBoardIndexNum);

	private:
		void timerEvent (QTimerEvent * event);


	//	Instance Data
	private:
		QComboBox *				mBoardChoiceCombo;		///	My device selector popup menu
		AJAPreviewWidget *		mVideoPreviewWidget;	///	My preview widget
        CNTV2DeviceScanner		mNTV2BoardScan;			///	My AJA device scanner
		NTV2StereoCapture *		mStereoCapture;			///	This does all the heavy lifting
		int						mTimerID;				///	A timer that fires every so often

};	//	NTV2QtStereoPreview


#endif // NTV2QTSTEREOPREVIEW_H
