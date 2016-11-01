/**
	@file		ntv2qtstereopreview.cpp
	@brief		Implementation of the NTV2QtStereoPreview class.
	@copyright	Copyright 2013 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2qtstereopreview.h"
#include "ajastuff/common/types.h"



NTV2QtStereoPreview::NTV2QtStereoPreview (QWidget * parent, Qt::WindowFlags flags)
    :	QDialog	(parent, flags), mStereoCapture(NULL)
{
	resize (AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y);
	mBoardChoiceCombo = new QComboBox;
    if (mNTV2BoardScan.GetNumDevices ())
	{
        NTV2DeviceInfoList &	boardList	(mNTV2BoardScan.GetDeviceInfoList ());

        for (NTV2DeviceInfoList::const_iterator iter = boardList.begin (); iter != boardList.end (); ++iter)
		{
            const NTV2DeviceInfo	info	(*iter);
            mBoardChoiceCombo->addItem (tr (info.deviceIdentifier.c_str()));
		}
	}
	else
		mBoardChoiceCombo->addItem (tr ("No Boards Found"));

	mBoardChoiceCombo->setCurrentIndex (0);
	QObject::connect (mBoardChoiceCombo, SIGNAL (currentIndexChanged (int)), this, SLOT (RequestBoardChange (const int)));

	mVideoPreviewWidget = new AJAPreviewWidget (this);
	mVideoPreviewWidget->setFixedWidth (AJAPREVIEW_WIDGET_X);
	mVideoPreviewWidget->setFixedHeight (AJAPREVIEW_WIDGET_Y);

	QVBoxLayout *	vlayout	(new QVBoxLayout);
	vlayout->addWidget (mBoardChoiceCombo);
	vlayout->addWidget (mVideoPreviewWidget);

	setLayout (vlayout);

	RequestBoardChange (mBoardChoiceCombo->currentIndex ());

}	//	constructor


NTV2QtStereoPreview::~NTV2QtStereoPreview ()
{
}	//	destructor


void NTV2QtStereoPreview::RequestBoardChange (const int inBoardIndexNum)
{
	if (mStereoCapture)
	{
		killTimer (mTimerID);
		disconnect (mStereoCapture, SIGNAL (newFrame (const QImage &, bool)), mVideoPreviewWidget, SLOT (updateFrame (const QImage &, bool)));
		disconnect (mStereoCapture, SIGNAL (newStatusString (const QString)), mVideoPreviewWidget, SLOT (updateStatusString (const QString)));
		delete mStereoCapture;
		mStereoCapture = NULL;
	}

	mStereoCapture = new NTV2StereoCapture ();
	if (!mStereoCapture)
		qDebug ("## ERROR:  Cannot create NTV2StereoCapture object for board %d", inBoardIndexNum);
	else if (mStereoCapture->Init (mNTV2BoardScan, inBoardIndexNum) != AJA_STATUS_SUCCESS)
	{
		qDebug ("## ERROR:  NTV2StereoCapture::Init failed for board %d", inBoardIndexNum);
		delete mStereoCapture;
		mStereoCapture = NULL;
	}
	else
	{
		mStereoCapture->Run ();

		connect (mStereoCapture, SIGNAL (newFrame (const QImage &, bool)), mVideoPreviewWidget, SLOT (updateFrame (const QImage &, bool)));
		connect (mStereoCapture, SIGNAL (newStatusString (const QString)), mVideoPreviewWidget, SLOT (updateStatusString (const QString)));

		mTimerID = startTimer (100);
	}

}	//	RequestBoardChange


void NTV2QtStereoPreview::timerEvent (QTimerEvent * event)
{
	if (event->timerId () == mTimerID)
	{
	}
	else
	{
		QWidget::timerEvent (event);
	}

}	//	timerEvent
