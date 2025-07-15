/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2qtpreview.cpp
	@brief		Implementation of the NTV2QtPreview class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.
**/


#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif
#include "ntv2qtpreview.h"
#include "ajabase/system/debug.h"
#include "ntv2utils.h"

using namespace std;

//	Convenience macros for EZ logging:
#define	FGFAIL(_expr_)		AJA_sERROR  (AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	FGWARN(_expr_)		AJA_sWARNING(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	FGDBG(_expr_)		AJA_sDEBUG	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	FGNOTE(_expr_)		AJA_sNOTICE	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	FGINFO(_expr_)		AJA_sINFO	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)


typedef QListIterator <QAbstractButton *>	QButtonIterator;


NTV2QtPreview::NTV2QtPreview (QWidget * parent, Qt::WindowFlags flags)
	:	QDialog (parent, flags)
{
	if (objectName ().isEmpty ())
		setObjectName (QString::fromUtf8 ("Dialog"));

	mBoardChoiceCombo = new QComboBox;

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
	mWithAudioCheckBox = new QCheckBox ("With Audio", this);
    mCheckFor4kCheckBox = new QCheckBox ("Check for 4K Input", this);
    mCheckFixedReference = new QCheckBox ("Fixed Reference", this);

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
		for (QButtonIterator iter(mInputButtonGroup->buttons());  iter.hasNext();  )
			layout->addWidget (iter.next());
		layout->addWidget (mWithAudioCheckBox);
        layout->addWidget (mCheckFor4kCheckBox);
        layout->addWidget (mCheckFixedReference);
	#endif	//	!defined (INCLUDE_AJACC)

	layout->addStretch (1);
	setLayout (layout);

    connect (mBoardChoiceCombo,	SIGNAL(currentIndexChanged(int)),		this,					SLOT(RequestDeviceChange(const int)));
	connect (mInputButtonGroup,	SIGNAL(idReleased(int)),				this,					SLOT(inputChanged(int)));
	connect (mWithAudioCheckBox,	SIGNAL(stateChanged(int)),			this,					SLOT(withAudioChanged(int)));
    connect (mCheckFixedReference,	SIGNAL(toggled(bool)),				this,					SLOT(fixedRefChanged(bool)));
	connect (mCheckFor4kCheckBox,	SIGNAL(stateChanged(int)),			this,					SLOT(checkFor4kChanged(int)));
	connect (mFrameGrabber,		SIGNAL(newFrame(const QImage&, bool)),	mVideoPreviewWidget,	SLOT(updateFrame(const QImage &, bool)));
	connect (mFrameGrabber,		SIGNAL(newStatusString(const QString)),	mVideoPreviewWidget,	SLOT(updateStatusString(const QString)));
	#if defined (INCLUDE_AJACC)
		connect (mFrameGrabber,		SIGNAL (captionScreenChanged (const ushort *)),	mVideoPreviewWidget,	SLOT (updateCaptionScreen (const ushort *)));
		connect (mCaptionButtonGroup,	SIGNAL (buttonReleased (int)),					mFrameGrabber,			SLOT (changeCaptionChannel (int)));
	#endif	//	defined (INCLUDE_AJACC)

	mFrameGrabber->SetInputSource (NTV2_INPUTSOURCE_INVALID);
	mFrameGrabber->start ();
	mTimerID = startTimer (100);

	mPnp.Install (PnpCallback, this, AJA_Pnp_PciVideoDevices);
	devicesChanged();

}	//	constructor


NTV2QtPreview::~NTV2QtPreview ()
{
	mPnp.Uninstall();
	delete mFrameGrabber;

}	//	destructor


void NTV2QtPreview::RequestDeviceChange (const int inDeviceIndexNum)
{
	//	Notify my frame grabber to change devices...
	if (mFrameGrabber)
		mFrameGrabber->SetDeviceIndex(inDeviceIndexNum);
	FGNOTE("Device changed to " << inDeviceIndexNum);

}	//	RequestDeviceChange


void NTV2QtPreview::inputChanged (int inputRadioButtonId)
{
	const NTV2InputSource chosenInputSource(NTV2InputSource(inputRadioButtonId+0));

	CNTV2Card device;
	if (!CNTV2DeviceScanner::GetDeviceAtIndex (ULWord(mBoardChoiceCombo->currentIndex()), device))
		{FGNOTE("No device at " << mBoardChoiceCombo->currentIndex()); return;}

	if (!NTV2_IS_VALID_INPUT_SOURCE(chosenInputSource))
	{
		mFrameGrabber->SetInputSource(NTV2_INPUTSOURCE_INVALID);
		FGNOTE("off");
	}
	else if (device.features().CanDoInputSource(chosenInputSource))
	{
		mFrameGrabber->SetInputSource(chosenInputSource);
		FGNOTE(::NTV2InputSourceToString(chosenInputSource));
	}
	else FGWARN("input source " << inputRadioButtonId << " unsupported by device");

}	//	inputChanged


void NTV2QtPreview::withAudioChanged (int state)
{
	mFrameGrabber->SetWithAudio (state == Qt::Checked ? true : false);

}	//	withAudioChanged


void NTV2QtPreview::fixedRefChanged (bool checked)
{
    mFrameGrabber->SetFixedReference(checked);

}


void NTV2QtPreview::checkFor4kChanged (int state)
{
	mFrameGrabber->CheckFor4kInput (state == Qt::Checked ? true : false);

}	//	checkFor4kChanged


void NTV2QtPreview::updateInputs (void)
{
	CNTV2Card ntv2Card (mBoardChoiceCombo->currentIndex());
	if (ntv2Card.IsOpen())
	{
		for (QButtonIterator iter(mInputButtonGroup->buttons());  iter.hasNext();  )
		{
			QRadioButton *			pButton			(reinterpret_cast<QRadioButton*>(iter.next()));
			const NTV2InputSource	inputSource		(NTV2InputSource(mInputButtonGroup->id(pButton)));
			if (NTV2_IS_VALID_INPUT_SOURCE(inputSource))
			{
				const bool		hasInputSource	(ntv2Card.features().CanDoInputSource(inputSource));
				const string	videoFormatStr	(hasInputSource ? ::NTV2VideoFormatToString(ntv2Card.GetInputVideoFormat(inputSource)) : "");
				const QString	buttonLabel		(QString("%1   %2").arg(::NTV2InputSourceToString(inputSource, true).c_str(), videoFormatStr.empty() ? "No Detected Input" : videoFormatStr.c_str()));
				pButton->setText(buttonLabel);
				pButton->setEnabled(hasInputSource);
			}
		}   //  for each radio button
		#if defined (INCLUDE_AJACC)
			const bool hasCustomAnc (mDevice.features().CanDoCustomAnc());
			for (QButtonIterator iter (mCaptionButtonGroup->buttons());  iter.hasNext();  )
				iter.next()->setEnabled(hasCustomAnc);
		#endif	//	defined (INCLUDE_AJACC)
	}	//	if board opened ok
}	//	updateInputs


void NTV2QtPreview::timerEvent (QTimerEvent * event)
{
	if (event->timerId() == mTimerID)
		updateInputs();
	else
		QWidget::timerEvent(event);
}	//	timerEvent


void NTV2QtPreview::devicesChanged (void)
{
	FGNOTE("");
	mInputButtonGroup->button (NTV2_INPUTSOURCE_INVALID)->setChecked(true);
	inputChanged (NTV2_INPUTSOURCE_INVALID);	//	necessary?

	mBoardChoiceCombo->clear();
	for (ULWord ndx(0);  ndx < 100;  ndx++)
	{
		CNTV2Card device(ndx);
		if (device.IsOpen())
			mBoardChoiceCombo->addItem(tr(device.GetDisplayName().c_str()), QVariant());
		else if (!ndx)
			{mBoardChoiceCombo->addItem(tr("No Devices Found"));  break;}
		else
			break;
	}
	mBoardChoiceCombo->setCurrentIndex(0);
}	//	devicesChanged


void NTV2QtPreview::PnpCallback (AJAPnpMessage inMessage, void * pUserData)		//  static
{
	if (pUserData)
		if (inMessage == AJA_Pnp_DeviceAdded  ||  inMessage == AJA_Pnp_DeviceRemoved)
		{
			NTV2QtPreview *	pMainWindow	(reinterpret_cast<NTV2QtPreview*>(pUserData));
			pMainWindow->devicesChanged();
		}
}	//	PnpCallback
