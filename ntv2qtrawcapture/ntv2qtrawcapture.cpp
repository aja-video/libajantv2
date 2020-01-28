/**
	@file		ntv2qtrawcapture.cpp
	@brief		Implementation of the NTV2QtRawCapture class.
	@copyright	(C) 2014-2020 AJA Video Systems, Inc. All rights reserved.
**/


#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif

//	Includes
#include "ajapreviewwidget.h"
#include "ajabase/common/types.h"
#include "ntv2devicefeatures.h"
#include "ntv2qtrawcapture.h"
#include "ntv2rawframegrabber.h"
#include "ntv2utils.h"


using namespace std;


//	Globals
static const char * sRecordStyleFlashOn		= "QRadioButton {color: rgb(255, 0, 0); font:bold}";
static const char * sRecordStyleFlashOff	= "QRadioButton {color: rgb(000, 0, 0); font:bold}";
static const char * sRecordStyleIdle		= "QRadioButton {color: rgb(000, 0, 0)}";


NTV2QtRawCapture::NTV2QtRawCapture (QWidget * parent, Qt::WindowFlags flags)
	:	QDialog (parent, flags)
{
	//	Provide a default title for the window if none has been specified
	if (objectName ().isEmpty ())
		setObjectName (QString::fromUtf8 ("Dialog"));

	//	Create and configure the UI controls from top to bottom

	//	Populate the device combo box with all the detectable devices
	mBoardChoiceCombo = new QComboBox;
	if (mNTV2Scanner.GetNumDevices () == 0)
		mBoardChoiceCombo->addItem (tr ("No Devices Found"));
	else
	{
		NTV2DeviceInfoList & deviceList	(mNTV2Scanner.GetDeviceInfoList ());

		for (NTV2DeviceInfoList::const_iterator iter = deviceList.begin (); iter != deviceList.end (); ++iter)
		{
			const NTV2DeviceInfo	info	(*iter);
			mBoardChoiceCombo->addItem (tr (info.deviceIdentifier.c_str ()));
		}
	}
	mBoardChoiceCombo->setCurrentIndex (0);

 	mVideoPreviewWidget = new AJAPreviewWidget (this);
	mVideoPreviewWidget->setFixedWidth  (AJAPREVIEW_WIDGET_X);
	mVideoPreviewWidget->setFixedHeight (AJAPREVIEW_WIDGET_Y);

	mStopRadioButton = new QRadioButton (tr ("Stop"));
	mStopRadioButton->setChecked (true);

	mRecordRadioButton = new QRadioButton (tr ("Record"));

	mPreviewWhenIdleCheckBox = new QCheckBox ("Preview when idle", this);
	mPreviewWhenIdleCheckBox->setChecked (true);

	mIncrementSequenceCheckBox = new QCheckBox ("Increment sequence number", this);
	mIncrementSequenceCheckBox->setChecked (true);

	QLabel * recordFolderLabel	= new QLabel ("Record Folder:   ");
	mRecordFolderComboBox		= new QComboBox (this);
	mRecordFolderComboBox->setMaxCount (15);

	mAddRecordFolderButton = new QPushButton (tr("+"), this);
	mAddRecordFolderButton->setMaximumWidth (20);
	mAddRecordFolderButton->setAutoDefault (false);			// Prevent activation by the Enter key

	mDeleteRecordFolderButton = new QPushButton (tr("-"), this);
	mDeleteRecordFolderButton->setMaximumWidth (20);
	mDeleteRecordFolderButton->setAutoDefault (false);		// Prevent activation by the Enter key

	QHBoxLayout * recordFolderLayout = new QHBoxLayout;		// Group the record folder controls horizontally
	recordFolderLayout->addWidget (recordFolderLabel);
	recordFolderLayout->addWidget (mRecordFolderComboBox, Qt::AlignLeft);
	recordFolderLayout->addWidget (mAddRecordFolderButton);
	recordFolderLayout->addWidget (mDeleteRecordFolderButton);

	//	Tell Qt how to arrange the controls in the window
	QVBoxLayout *	layout	(new QVBoxLayout);
	layout->addWidget (mBoardChoiceCombo);
	layout->addWidget (mVideoPreviewWidget);
	layout->addWidget (mStopRadioButton);
	layout->addWidget (mRecordRadioButton);
	layout->addWidget (mPreviewWhenIdleCheckBox);
	layout->addWidget (mIncrementSequenceCheckBox);
	layout->addLayout (recordFolderLayout);
	layout->addStretch (1);
	setLayout (layout);

	//	Instantiate a class to manage the capture process
	mRawFrameGrabber = new NTV2RawFrameGrabber (this);

	//	Restore the control settings from the last time the appliaction was run
	RecallSettings ();

	//	Configure the capture instance, and start the cature ptocess
	if (mRawFrameGrabber)
	{
		NTV2DeviceInfo	info;
		mNTV2Scanner.GetDeviceInfo (mBoardChoiceCombo->currentIndex (), info, false);
		mRawFrameGrabber->SetDeviceIndex (info.deviceIndex);
		mRawFrameGrabber->SetRecording (false);
		mRawFrameGrabber->SetRecordPath (mRecordFolderComboBox->currentText ().toStdString ());
		mRawFrameGrabber->SetPreviewWhenIdle (mPreviewWhenIdleCheckBox->isChecked () ? true : false);
		mRawFrameGrabber->SetIncrementSequence (mIncrementSequenceCheckBox->isChecked () ? true : false);
		mRawFrameGrabber->start ();
	}

	//	Associate user interface events with the routines that handle them
    QObject::connect (mBoardChoiceCombo,			SIGNAL (currentIndexChanged (int)),			this,					SLOT (RequestBoardChange (const int)));
	QObject::connect (mStopRadioButton,				SIGNAL (released ()),						this,					SLOT (RecordStateChanged ()));
	QObject::connect (mRecordRadioButton,			SIGNAL (released ()),						this,					SLOT (RecordStateChanged ()));
	QObject::connect (mPreviewWhenIdleCheckBox,		SIGNAL (stateChanged (int)),				this,					SLOT (PreviewWhenIdleChanged (int)));
	QObject::connect (mIncrementSequenceCheckBox,	SIGNAL (stateChanged (int)),				this,					SLOT (IncrementSequenceChanged (int)));
    QObject::connect (mRecordFolderComboBox,		SIGNAL (currentIndexChanged(int)),			this,					SLOT (NewRecordFolderChoice (int)));
    QObject::connect (mAddRecordFolderButton,		SIGNAL (clicked ()),						this,					SLOT (AddRecordFolder ()));
    QObject::connect (mDeleteRecordFolderButton,	SIGNAL (clicked ()),						this,					SLOT (DeleteRecordFolder ()));
	QObject::connect (mRawFrameGrabber,				SIGNAL (NewFrame (const QImage &, bool)),	mVideoPreviewWidget,	SLOT (updateFrame (const QImage &, bool)));
	QObject::connect (mRawFrameGrabber,				SIGNAL (NewStatusString (const QString)),	mVideoPreviewWidget,	SLOT (updateStatusString (const QString)));

	//	Start a system timer for animating user interface contols
	mTimerID = startTimer (100);

}	//	constructor


NTV2QtRawCapture::~NTV2QtRawCapture ()
{
	//	Disallow user interface update events during shut down
	QObject::disconnect (mRawFrameGrabber,			SIGNAL (NewFrame (const QImage &, bool)),	mVideoPreviewWidget,	SLOT (updateFrame (const QImage &, bool)));
	QObject::disconnect (mRawFrameGrabber,			SIGNAL (NewStatusString (const QString)),	mVideoPreviewWidget,	SLOT (updateStatusString (const QString)));

	mRawFrameGrabber->SetRecording (false);

	delete mRawFrameGrabber;

	//	Save user interface control state, which will be restored the next time the application runs
	SaveSettings ();

}	//	destructor


void NTV2QtRawCapture::RequestBoardChange (const int inBoardIndexNum)
{
	//	Notify my frame grabber to change devices...
	NTV2DeviceInfo	info;
	mNTV2Scanner.GetDeviceInfo (inBoardIndexNum, info, false);
	if (mRawFrameGrabber)
		mRawFrameGrabber->SetDeviceIndex (info.deviceIndex);

}	//	RequestBoardChange


void NTV2QtRawCapture::RecordStateChanged (void)
{
	//	User has clicked on the Record or Stop button
	if (mRecordRadioButton->isChecked ())
	{
		mRawFrameGrabber->SetRecording (true);
	}
	else if (mStopRadioButton->isChecked ())
	{
		mRawFrameGrabber->SetRecording (false);
	}

	return;

}	//	RunStateChanged


void NTV2QtRawCapture::PreviewWhenIdleChanged (const int inState)
{
	//	Set the new preview when idle state
	if (mRawFrameGrabber)
	{
		mRawFrameGrabber->SetPreviewWhenIdle (inState == Qt::Checked ? true : false);
	}

}	//	PreviewWhenIdleChanged


void NTV2QtRawCapture::IncrementSequenceChanged (const int inState)
{
	//	Set the file name increment state
	if (mRawFrameGrabber)
	{
		mRawFrameGrabber->SetIncrementSequence (inState == Qt::Checked ? true : false);
	}

}	//	IncrementSequenceChanged


void NTV2QtRawCapture::AddRecordFolder (void)
{
	//	Ask user for new record folder path, then add it to the combo box list
	QString currentDirectoryString = QFileDialog::getExistingDirectory (this,
																		tr("Open Directory"),
																		mRecordFolderComboBox->currentText (),
																		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (!currentDirectoryString.isEmpty ())
	{
		AddRecordFolder (currentDirectoryString);
	}

}	//	AddRecordFolder


void NTV2QtRawCapture::AddRecordFolder (const QString inFolderString)
{
	//	Add the new folder path to the list, them select it as the current foldar
	mRecordFolderComboBox->addItem (inFolderString);
	mRecordFolderComboBox->setCurrentIndex (mRecordFolderComboBox->count () - 1);

}	//	AddRecordFolder


void NTV2QtRawCapture::DeleteRecordFolder (void)
{
	//	Remove the current folder selection from the combo box list.
	int index = mRecordFolderComboBox->currentIndex ();
	if (index != -1)
	{
		mRecordFolderComboBox->removeItem (index);
	}

}	//	DeleteRecordFolder


void NTV2QtRawCapture::NewRecordFolderChoice (const int inNewRecordFolderIndex)
{
	//	Select the user's new record folder choice, and sen the new path to the capture class
	mRecordFolderComboBox->setCurrentIndex (inNewRecordFolderIndex);
	if (mRawFrameGrabber)
	{
		mRawFrameGrabber->SetRecordPath (mRecordFolderComboBox->currentText ().toStdString ());
	}

}	//	NewRecordFolderChoice


void NTV2QtRawCapture::RecallSettings (void)
{
	//	Restore the user interface contol settings from the last time the application ran
	QSettings settings (QSettings::UserScope, "aja", "NTV2QtRawCapture");

	int currentIndex = settings.value ("BoardChoiceIndex").toInt();
	mBoardChoiceCombo->setCurrentIndex (currentIndex);

	if (settings.value ("PreviewWhenIdle").toBool () == true)
		mPreviewWhenIdleCheckBox->setCheckState (Qt::Checked);
	else
		mPreviewWhenIdleCheckBox->setCheckState (Qt::Unchecked);

	if (settings.value ("IncrementSequence").toBool () == true)
		mIncrementSequenceCheckBox->setCheckState (Qt::Checked);
	else
		mIncrementSequenceCheckBox->setCheckState (Qt::Unchecked);

	int numWorkingFolders = settings.value ("NumRecordWorkingFolders").toInt();
	settings.beginReadArray ("RecordFolders");
	for (int i = 0; i < numWorkingFolders; ++i)
	{
		settings.setArrayIndex (i);
		QString currentFolderString = settings.value ("RecordFolderName").toString();
		mRecordFolderComboBox->addItem (currentFolderString);
	}
	settings.endArray ();

	currentIndex = settings.value ("CurrentRecordFolderIndex").toInt();
	mRecordFolderComboBox->setCurrentIndex (currentIndex);

}	//	RecallSettings


void NTV2QtRawCapture::SaveSettings ()
{
	//	Save the current user interface control settings to non-volitile storage
	QSettings settings (QSettings::UserScope, "aja", "NTV2QtRawCapture");

	settings.setValue ("BoardChoiceIndex", mBoardChoiceCombo->currentIndex ());
	settings.setValue ("PreviewWhenIdle", mPreviewWhenIdleCheckBox->isChecked ());
	settings.setValue ("IncrementSequence", mIncrementSequenceCheckBox->isChecked ());

	int numWorkingFolders = mRecordFolderComboBox->count ();
	settings.setValue ("NumRecordWorkingFolders", numWorkingFolders);

	settings.beginWriteArray ("RecordFolders");
	for (int i = 0; i <numWorkingFolders; ++i)
	{
		settings.setArrayIndex (i);
		settings.setValue ("RecordFolderName", mRecordFolderComboBox->itemText (i));
	}
	settings.endArray ();

	settings.setValue ("CurrentRecordFolderIndex", mRecordFolderComboBox->currentIndex ());

}	//	SaveSettings


void NTV2QtRawCapture::TimerEvent (QTimerEvent * inEvent)
{
	static uint32_t timerCounter = 0;

	//	Verify that the timer event is ours
	if (inEvent->timerId () == mTimerID)
	{
		timerCounter++;		//	Timer value may wrap without consequence

		//	Check if currently recording
		if (mRecordRadioButton->isChecked ())
		{
			//	Derive flash rate from number of timer events received
			if (timerCounter & 0x4)
			{
				//	Display record button label in bold red letters
				mRecordRadioButton->setStyleSheet (sRecordStyleFlashOn);
				mRecordRadioButton->setText ("Recording");
			}
			else
			{
				//	Display record button label in bold black letters
				mRecordRadioButton->setStyleSheet (sRecordStyleFlashOff);
				mRecordRadioButton->setText ("Recording");
			}
		}
		else
		{
				//	Display record button label in normal black letters
				mRecordRadioButton->setStyleSheet (sRecordStyleIdle);
				mRecordRadioButton->setText ("Record");
		}
	}
	else
	{
		//	Pass all other events to Qt
		QWidget::timerEvent (inEvent);
	}

}	//	TimerEvent

