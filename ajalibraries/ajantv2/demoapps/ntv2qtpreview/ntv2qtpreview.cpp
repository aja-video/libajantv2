/**
	@file		ntv2qtpreview.cpp
	@brief		Implementation of the NTV2QtPreview class.
	@copyright	(C) 2013-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/


#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif

#include "ntv2qtpreview.h"
#include "ntv2devicefeatures.h"
#include "ajastuff/common/types.h"
#include "ntv2utils.h"


using namespace std;


typedef QListIterator <QAbstractButton *>	QButtonIterator;


NTV2QtPreview::NTV2QtPreview (QWidget * parent, Qt::WindowFlags flags)
	:	QDialog (parent, flags)
{
	if (objectName ().isEmpty ())
		setObjectName (QString::fromUtf8 ("Dialog"));

	mBoardChoiceCombo = new QComboBox;
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

	//	Input selection radio button group...
	mInputButtonGroup = new QButtonGroup ();
	mInputButtonGroup->addButton (new QRadioButton (tr ("Off")), NTV2_INPUTSOURCE_INVALID);
	for (unsigned ndx (0);  ndx < 8;  ndx++)
		mInputButtonGroup->addButton (new QRadioButton ((QString ("SDI %1").arg (string (1, char (ndx + '1')).c_str ()))), ::GetNTV2InputSourceForIndex (ndx));
	mInputButtonGroup->addButton (new QRadioButton (tr ("HDMI")), NTV2_INPUTSOURCE_HDMI);
	mInputButtonGroup->addButton (new QRadioButton (tr ("Analog")), NTV2_INPUTSOURCE_ANALOG);
	mInputButtonGroup->button (NTV2_INPUTSOURCE_INVALID)->setChecked (true);

	//	Checkboxes...
	mWithAudioCheckBox = new QCheckBox ("With Audio", this);
	mCheckFor4kCheckBox = new QCheckBox ("Check for 4K Input", this);

 	mVideoPreviewWidget = new AJAPreviewWidget (this);
	mVideoPreviewWidget->setFixedWidth (QTPREVIEW_WIDGET_X);
	mVideoPreviewWidget->setFixedHeight (QTPREVIEW_WIDGET_Y);

	mFrameGrabber = new NTV2FrameGrabber (this);
	mFrameGrabber->SetDeviceIndex (mBoardChoiceCombo->currentIndex ());
	mFrameGrabber->SetInputSource (NTV2_INPUTSOURCE_SDI1);

	QVBoxLayout *	layout	(new QVBoxLayout);
	layout->addWidget (mBoardChoiceCombo);
	layout->addWidget (mVideoPreviewWidget);

	#if defined (INCLUDE_AJACC)
		QVBoxLayout *	bottomLeftLayout	(new QVBoxLayout);
		bottomLeftLayout->setContentsMargins (0, 0, 0, 0);

		for (QButtonIterator iter (mInputButtonGroup->buttons());  iter.hasNext ();  )
			bottomLeftLayout->addWidget (iter.next());

		bottomLeftLayout->addWidget (mWithAudioCheckBox);
		bottomLeftLayout->addWidget (mCheckFor4kCheckBox);
		bottomLeftLayout->addStretch (1);

		QVBoxLayout *	bottomRightLayout	(new QVBoxLayout);
		bottomRightLayout->setContentsMargins (0, 0, 0, 0);

		mCaptionButtonGroup = new QButtonGroup ();
		mCaptionButtonGroup->addButton (new QRadioButton (tr ("CC Off")), NTV2_CC608_ChannelInvalid);
		for (unsigned ndx (1);  ndx <= 8;  ndx++)
			mCaptionButtonGroup->addButton (new QRadioButton (QString ("%1%2").arg (string (ndx < 5 ? "CC" : "TXT").c_str(), string (1, char ((ndx < 5 ? ndx : ndx - 4) + '0')).c_str())),
											NTV2Line21Channel (ndx-1));
		mCaptionButtonGroup->button (NTV2_CC608_ChannelInvalid)->setChecked (true);

		for (QButtonIterator iter (mCaptionButtonGroup->buttons());  iter.hasNext ();  )
			bottomRightLayout->addWidget (iter.next());

		QHBoxLayout *	bottomLayout		(new QHBoxLayout);
		bottomLayout->setContentsMargins (0, 0, 0, 0);

		bottomLayout->addLayout (bottomLeftLayout);
		bottomLayout->addLayout (bottomRightLayout);
		layout->addLayout (bottomLayout);
	#else	//	!defined (INCLUDE_AJACC)
		for (QButtonIterator iter (mInputButtonGroup->buttons());  iter.hasNext ();  )
			layout->addWidget (iter.next());
		layout->addWidget (mWithAudioCheckBox);
		layout->addWidget (mCheckFor4kCheckBox);
	#endif	//	!defined (INCLUDE_AJACC)

	layout->addStretch (1);
	setLayout (layout);

    QObject::connect (mBoardChoiceCombo,	SIGNAL (currentIndexChanged (int)),				this,					SLOT (RequestDeviceChange (const int)));
	QObject::connect (mInputButtonGroup,	SIGNAL (buttonReleased (int)),					this,					SLOT (inputChanged (int)));
	QObject::connect (mWithAudioCheckBox,	SIGNAL (stateChanged (int)),					this,					SLOT (withAudioChanged (int)));
	QObject::connect (mCheckFor4kCheckBox,	SIGNAL (stateChanged (int)),					this,					SLOT (checkFor4kChanged (int)));
			 connect (mFrameGrabber,		SIGNAL (newFrame (const QImage &, bool)),		mVideoPreviewWidget,	SLOT (updateFrame (const QImage &, bool)));
			 connect (mFrameGrabber,		SIGNAL (newStatusString (const QString)),		mVideoPreviewWidget,	SLOT (updateStatusString (const QString)));
	#if defined (INCLUDE_AJACC)
			 connect (mFrameGrabber,		SIGNAL (captionScreenChanged (const ushort *)),	mVideoPreviewWidget,	SLOT (updateCaptionScreen (const ushort *)));
	QObject::connect (mCaptionButtonGroup,	SIGNAL (buttonReleased (int)),					mFrameGrabber,			SLOT (changeCaptionChannel (int)));
	#endif	//	defined (INCLUDE_AJACC)

	mFrameGrabber->SetInputSource (NTV2_NUM_INPUTSOURCES);
	mFrameGrabber->start ();
	mTimerID = startTimer (100);

	mPnp.Install (PnpCallback, this, AJA_Pnp_PciVideoDevices);

}	//	constructor


NTV2QtPreview::~NTV2QtPreview ()
{
	mPnp.Uninstall ();
	delete mFrameGrabber;

}	//	destructor


void NTV2QtPreview::RequestDeviceChange (const int inDeviceIndexNum)
{
	//	Notify my frame grabber to change devices...
	if (mFrameGrabber)
		mFrameGrabber->SetDeviceIndex (inDeviceIndexNum);
	qDebug ("## NOTE:  Device changed to %d", inDeviceIndexNum);

}	//	RequestDeviceChange


void NTV2QtPreview::inputChanged (int inputRadioButtonId)
{
	const NTV2InputSource	chosenInputSource	(static_cast <NTV2InputSource> (inputRadioButtonId));

	CNTV2Card	device;
	CNTV2DeviceScanner::GetDeviceAtIndex (mBoardChoiceCombo->currentIndex (), device);

	const NTV2DeviceID		deviceID			(device.GetDeviceID ());

	if (!NTV2_IS_VALID_INPUT_SOURCE (chosenInputSource))
	{
		mFrameGrabber->SetInputSource (NTV2_INPUTSOURCE_INVALID);
		qDebug ("## DEBUG:  NTV2QtPreview::inputChanged:  off");
	}
	else if (::NTV2DeviceCanDoInputSource (deviceID, chosenInputSource))
	{
		mFrameGrabber->SetInputSource (chosenInputSource);
		cerr << "## DEBUG:  NTV2QtPreview::inputChanged:  " << ::NTV2InputSourceToString (chosenInputSource) << endl;
	}

}	//	inputChanged


void NTV2QtPreview::withAudioChanged (int state)
{
	mFrameGrabber->SetWithAudio (state == Qt::Checked ? true : false);

}	//	withAudioChanged


void NTV2QtPreview::checkFor4kChanged (int state)
{
	mFrameGrabber->CheckFor4kInput (state == Qt::Checked ? true : false);

}	//	checkFor4kChanged


void NTV2QtPreview::updateInputs (void)
{
	CNTV2Card	ntv2Card (mBoardChoiceCombo->currentIndex (), false);
	if (ntv2Card.IsOpen ())
	{
		const NTV2DeviceID	deviceID	(ntv2Card.GetDeviceID ());

		for (QButtonIterator iter (mInputButtonGroup->buttons());  iter.hasNext ();  )
		{
			QRadioButton *			pButton			(reinterpret_cast <QRadioButton*> (iter.next ()));
			const NTV2InputSource	inputSource		(static_cast <NTV2InputSource> (mInputButtonGroup->id (pButton)));
			if (NTV2_IS_VALID_INPUT_SOURCE (inputSource))
			{
				const bool				hasInputSource	(::NTV2DeviceCanDoInputSource (deviceID, inputSource));
				const string			videoFormatStr	(hasInputSource ? ::NTV2VideoFormatToString (ntv2Card.GetInputVideoFormat (inputSource)) : "");
				const QString			buttonLabel		(QString ("%1   %2").arg (::NTV2InputSourceToString (inputSource, true).c_str(), videoFormatStr.empty() ? "No Detected Input" : videoFormatStr.c_str()));
				pButton->setText (buttonLabel);
				pButton->setEnabled (hasInputSource);
			}
		}

		#if defined (INCLUDE_AJACC)
			const bool	hasCustomAnc	(::NTV2DeviceCanDoCustomAnc (deviceID));
			for (QButtonIterator iter (mCaptionButtonGroup->buttons());  iter.hasNext ();  )
				iter.next()->setEnabled (hasCustomAnc);
		#endif	//	defined (INCLUDE_AJACC)

	}	//	if board opened ok

}	//	updateInputs


void NTV2QtPreview::timerEvent (QTimerEvent * event)
{
	if (event->timerId () == mTimerID)
		updateInputs ();
	else
		QWidget::timerEvent (event);

}	//	timerEvent


void NTV2QtPreview::devicesChanged (void)
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


void NTV2QtPreview::PnpCallback (AJAPnpMessage inMessage, void * pUserData)		//  static
{
	if (pUserData)
	{
		if (inMessage == AJA_Pnp_DeviceAdded  ||  inMessage == AJA_Pnp_DeviceRemoved)
		{
			NTV2QtPreview *	pMainWindow	(reinterpret_cast <NTV2QtPreview *>(pUserData));
			pMainWindow->devicesChanged ();
		}
	}

} // PnpCallback
