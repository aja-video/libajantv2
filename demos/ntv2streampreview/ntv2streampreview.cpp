/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streampreview.cpp
	@brief		Implementation of the NTV2StreamPreview class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.
**/


#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif

#include "ntv2streampreview.h"
#include "ntv2devicefeatures.h"
#include "ajabase/common/types.h"
#include "ntv2utils.h"


using namespace std;


typedef QListIterator <QAbstractButton *>	QButtonIterator;


NTV2StreamPreview::NTV2StreamPreview (QWidget * parent, Qt::WindowFlags flags)
	:	QDialog (parent, flags)
{
	if (objectName().isEmpty())
		setObjectName(QString::fromUtf8("Dialog"));

	mBoardChoiceCombo = new QComboBox;
	for (ULWord ndx(0);  ndx < 100;  ndx++)
	{
		CNTV2Card device;
		if (CNTV2DeviceScanner::GetDeviceAtIndex (ndx, device))
			mBoardChoiceCombo->addItem(device.GetDisplayName().c_str());
		else if (ndx == 0)
			{mBoardChoiceCombo->addItem("No Devices Found");  break;}
		else
			break;
	}
	mBoardChoiceCombo->setCurrentIndex (0);

	//	Input selection radio button group...
	mInputButtonGroup = new QButtonGroup ();
	mInputButtonGroup->addButton (new QRadioButton (tr ("Off")), NTV2_INPUTSOURCE_INVALID);
	for (unsigned ndx (0);  ndx < 8;  ndx++)
		mInputButtonGroup->addButton (new QRadioButton ((QString ("SDI %1").arg (string (1, char (ndx + '1')).c_str ()))), ::GetNTV2InputSourceForIndex(ndx, NTV2_IOKINDS_SDI));
	for (unsigned ndx (0);  ndx < 4;  ndx++)
		mInputButtonGroup->addButton (new QRadioButton ((QString ("HDMI %1").arg (string (1, char (ndx + '1')).c_str ()))), ::GetNTV2InputSourceForIndex(ndx, NTV2_IOKINDS_HDMI));
	mInputButtonGroup->addButton (new QRadioButton (tr ("Analog")), ::GetNTV2InputSourceForIndex(0, NTV2_IOKINDS_ANALOG));
	mInputButtonGroup->button (NTV2_INPUTSOURCE_INVALID)->setChecked (true);

	//	Checkboxes...
    mCheckFixedReference = new QCheckBox ("Fixed Reference", this);

 	mVideoPreviewWidget = new AJAPreviewWidget (this);
	mVideoPreviewWidget->setFixedWidth (STREAMPREVIEW_WIDGET_X);
	mVideoPreviewWidget->setFixedHeight (STREAMPREVIEW_WIDGET_Y);

	mStreamGrabber = new NTV2StreamGrabber (this);
	mStreamGrabber->SetDeviceIndex (mBoardChoiceCombo->currentIndex ());
	mStreamGrabber->SetInputSource (NTV2_INPUTSOURCE_SDI1);

	QVBoxLayout *	layout	(new QVBoxLayout);
	layout->addWidget (mBoardChoiceCombo);
	layout->addWidget (mVideoPreviewWidget);

	for (QButtonIterator iter (mInputButtonGroup->buttons());  iter.hasNext ();  )
		layout->addWidget (iter.next());
    layout->addWidget (mCheckFixedReference);

	layout->addStretch (1);
	setLayout (layout);

    QObject::connect (mBoardChoiceCombo,	SIGNAL (currentIndexChanged (int)),				this,					SLOT (RequestDeviceChange (const int)));
	QObject::connect (mInputButtonGroup,	SIGNAL (buttonReleased (int)),					this,					SLOT (inputChanged (int)));
    QObject::connect (mCheckFixedReference,	SIGNAL (toggled (bool)),                        this,					SLOT (fixedRefChanged (bool)));
			 connect (mStreamGrabber,		SIGNAL (newFrame (const QImage &, bool)),		mVideoPreviewWidget,	SLOT (updateFrame (const QImage &, bool)));
			 connect (mStreamGrabber,		SIGNAL (newStatusString (const QString)),		mVideoPreviewWidget,	SLOT (updateStatusString (const QString)));

	mStreamGrabber->SetInputSource (NTV2_NUM_INPUTSOURCES);
	mStreamGrabber->start ();
	mTimerID = startTimer (100);

	mPnp.Install (PnpCallback, this, AJA_Pnp_PciVideoDevices);

}	//	constructor


NTV2StreamPreview::~NTV2StreamPreview ()
{
	mPnp.Uninstall ();
	delete mStreamGrabber;

}	//	destructor


void NTV2StreamPreview::RequestDeviceChange (const int inDeviceIndexNum)
{
	//	Notify my frame grabber to change devices...
	if (mStreamGrabber)
		mStreamGrabber->SetDeviceIndex (inDeviceIndexNum);
	qDebug ("## NOTE:  Device changed to %d", inDeviceIndexNum);

}	//	RequestDeviceChange


void NTV2StreamPreview::inputChanged (int inputRadioButtonId)
{
	const NTV2InputSource	chosenInputSource	(static_cast <NTV2InputSource> (inputRadioButtonId));

	CNTV2Card	device;
	CNTV2DeviceScanner::GetDeviceAtIndex (mBoardChoiceCombo->currentIndex(), device);

	if (!NTV2_IS_VALID_INPUT_SOURCE (chosenInputSource))
	{
		mStreamGrabber->SetInputSource (NTV2_INPUTSOURCE_INVALID);
		qDebug ("## DEBUG:  NTV2StreamPreview::inputChanged:  off");
	}
	else if (device.features().CanDoInputSource(chosenInputSource))
	{
		mStreamGrabber->SetInputSource (chosenInputSource);
		cerr << "## DEBUG:  NTV2StreamPreview::inputChanged:  " << ::NTV2InputSourceToString (chosenInputSource) << endl;
	}
}	//	inputChanged


void NTV2StreamPreview::fixedRefChanged (bool checked)
{
    mStreamGrabber->SetFixedReference(checked);

}	//	fixedRefChanged


void NTV2StreamPreview::updateInputs (void)
{
	CNTV2Card	ntv2Card (mBoardChoiceCombo->currentIndex());
	if (ntv2Card.IsOpen())
	{
		for (QButtonIterator iter (mInputButtonGroup->buttons());  iter.hasNext();  )
		{
			QRadioButton * pButton (reinterpret_cast<QRadioButton*>(iter.next()));
			const NTV2InputSource inputSource (NTV2InputSource(mInputButtonGroup->id(pButton)));
			if (NTV2_IS_VALID_INPUT_SOURCE (inputSource))
			{
				const bool		hasInputSource	(ntv2Card.features().CanDoInputSource(inputSource));
				const string	videoFormatStr	(hasInputSource ? ::NTV2VideoFormatToString(ntv2Card.GetInputVideoFormat(inputSource)) : "");
				const QString	buttonLabel		(QString("%1   %2").arg(::NTV2InputSourceToString(inputSource, true).c_str(), videoFormatStr.empty() ? "No Detected Input" : videoFormatStr.c_str()));
				pButton->setText (buttonLabel);
				pButton->setEnabled (hasInputSource);
			}
		}
	}	//	if board opened ok

}	//	updateInputs


void NTV2StreamPreview::timerEvent (QTimerEvent * event)
{
	if (event->timerId() == mTimerID)
		updateInputs();
	else
		QWidget::timerEvent(event);

}	//	timerEvent


void NTV2StreamPreview::devicesChanged (void)
{
	qDebug () << QString ("devicesChanged");
	mInputButtonGroup->button (NTV2_INPUTSOURCE_INVALID)->setChecked (true);
	inputChanged (NTV2_INPUTSOURCE_INVALID);	//	necessary?

	mBoardChoiceCombo->clear ();
	for (ULWord ndx (0);  ndx < 100;  ndx++)
	{
		CNTV2Card	device;
		if (CNTV2DeviceScanner::GetDeviceAtIndex (ndx, device))
			mBoardChoiceCombo->addItem (tr (device.GetDisplayName ().c_str ()));
		else if (ndx == 0)
			{mBoardChoiceCombo->addItem (tr ("No Devices Found"));	break;}
		else
			break;
	}
	mBoardChoiceCombo->setCurrentIndex (0);

}	//	devicesChanged


void NTV2StreamPreview::PnpCallback (AJAPnpMessage inMessage, void * pUserData)		//  static
{
	if (pUserData)
	{
		if (inMessage == AJA_Pnp_DeviceAdded  ||  inMessage == AJA_Pnp_DeviceRemoved)
		{
			NTV2StreamPreview *	pMainWindow	(reinterpret_cast <NTV2StreamPreview *>(pUserData));
			pMainWindow->devicesChanged ();
		}
	}

} // PnpCallback
