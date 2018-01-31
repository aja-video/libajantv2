/**
		@file			ntv2qtplaycorder.cpp
		@brief			Implementation file for the NTV2QtPlaycorder demonstration application.
						This is the UI for an application that captures and plays back files.
		@copyright		Copyright 2013 AJA Video Systems, Inc. All rights reserved. 
**/

#if defined(AJA_WINDOWS)
#include <windows.h>
#else
#include "sys/types.h"
#include "sys/stat.h"
//#include "sys/dirent.h"
#endif

#include <string>
#include <sstream>

#include <QtDebug>

#include "ajabase/system/file_io.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/thread.h"
#include "ajapreviewwidget.h"
#include "ntv2playcorder.h"		// The worker class
#include "ntv2qtplaycorder.h"	// The UI

static const char* sPlayStyleInactive		= "QPushButton {color: white; background-color:darkgreen}";
static const char* sPlayStyleActiveHigh		= "QPushButton {color: rgb(0, 255, 0); background-color:black}";
static const char* sPlayStyleActiveLow		= "QPushButton {color: rgb(0, 128, 0); background-color:black}";

static const char* sRecordStyleInactive		= "QPushButton {color: white; background-color:darkred}";
static const char* sRecordStyleActiveHigh	= "QPushButton {color: rgb(255, 0, 0); background-color:black}";
static const char* sRecordStyleActiveLow	= "QPushButton {color: rgb(128, 0, 0); background-color:black}";

QString findNextSequenceDirectory(QString currentDirectory);


NTV2QtPlaycorder::NTV2QtPlaycorder(QWidget *parent, Qt::WindowFlags flags, const char *singleIPAddr)
	:	QDialog(parent, flags),
		mBoardIndex(0),
		mAjaFileIO(NULL),
		mPlaycorder(NULL),
		mPreviewing(false),
		mRecording(false),
		mPlaying(false),
		mPreviewCircularBuffer(NULL),
		mPreviewThread(NULL),
		mGlobalQuit(false)
{
	(void) singleIPAddr;

	mAjaFileIO		= new AJAFileIO ();
	mPlaycorder		= IPlaycorder::GetIPlaycorder ();
	mPlaycorder->Init (AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y);
	mPreviewCircularBuffer = mPlaycorder->GetPreviewCircularBuffer ();

	AddDeviceEnumeratorGroupBox();
	AddVideoPreviewGroupBox();
	AddVideoRecordFormatGroupBox();
	AddFolderGroupBox();
	AddTransportControls();
	AddScrubSlider();
	AddStatusEnumeratorGroupBox();
	AddItemsToLayoutManager();	
	SetupStyleSheetSettings();
	UpdateBoardEnumerator();
	UpdateVideoFormatEnumerator();

	AddSignalsAndSlots();

	RecallSettings();
	mPlaycorder->SetRecordPath (GetCurrentRecordDirectoryStdString ());
	mPlaycorder->SetPlayPath   (GetCurrentPlaybackDirectoryStdString ());

	NewBoardChoice(mDeviceChoiceCombo->currentIndex());

	SetupScrubSlider(0);
	stop();

	mPlaycorder->StartRecording ();
	StartPreviewThread ();
}


NTV2QtPlaycorder::~NTV2QtPlaycorder()
{
	stop();
	SaveSettings();

	mGlobalQuit = true;
	while (mPreviewThread-> Active ())
		AJATime::Sleep (10);

	delete mPreviewThread;

	delete mPlaycorder;
	delete mAjaFileIO;
}


void NTV2QtPlaycorder::RecallSettings()
{
	QSettings settings(QSettings::UserScope, "aja" , "NTV2Player");
	mDeviceChoiceCombo->setCurrentIndex(settings.value("CurrentDeviceIndex").toInt());
	mVideoPlaybackFormatCombo->setCurrentIndex(settings.value("CurrentVideoFormatIndex").toInt());
	if ( settings.value("PreviewInput").toBool() == true )
		mPreviewWhenIdle->setCheckState(Qt::Checked);
	else
		mPreviewWhenIdle->setCheckState(Qt::Unchecked);

	if ( settings.value("StopOnDrop").toBool() == true )
		mStopOnDrop->setCheckState(Qt::Checked);
	else
		mStopOnDrop->setCheckState(Qt::Unchecked);

	int numWorkingFolders = settings.value("NumPlaybackWorkingFolders").toInt();
	settings.beginReadArray("PlaybackFolders");
	for (int i = 0; i < numWorkingFolders; ++i) {
		settings.setArrayIndex(i);
		QString currentFolderString = settings.value("PlaybackFolderName").toString();
		mCurrentPlaybackFolderComboBox->addItem(currentFolderString);
	}
	settings.endArray();

	int currentIndex = settings.value("CurrentPlaybackFolderIndex").toInt();
	mCurrentPlaybackFolderComboBox->setCurrentIndex(currentIndex);

	numWorkingFolders = settings.value("NumRecordWorkingFolders").toInt();
	settings.beginReadArray("RecordFolders");
	for (int i = 0; i < numWorkingFolders; ++i) {
		settings.setArrayIndex(i);
		QString currentFolderString = settings.value("RecordFolderName").toString();
		mCurrentRecordFolderComboBox->addItem(currentFolderString);
	}
	settings.endArray();

	currentIndex = settings.value("CurrentRecordFolderIndex").toInt();
	mCurrentRecordFolderComboBox->setCurrentIndex(currentIndex);
}


void NTV2QtPlaycorder::SaveSettings()
{
	QSettings settings(QSettings::UserScope, "aja" , "NTV2Player");
	settings.setValue("CurrentDeviceIndex", mDeviceChoiceCombo->currentIndex());
	settings.setValue("CurrentVideoFormatIndex",mVideoPlaybackFormatCombo->currentIndex());
	settings.setValue("StopOnDrop",mStopOnDrop->isChecked());
	settings.setValue("PreviewInput",mPreviewWhenIdle->isChecked());

	settings.setValue("CurrentPlaybackFolderIndex",mCurrentPlaybackFolderComboBox->currentIndex());
	int numWorkingFolders = mCurrentPlaybackFolderComboBox->count();
	settings.setValue("NumPlaybackWorkingFolders",numWorkingFolders);
	settings.beginWriteArray("PlaybackFolders");
	for (int i = 0; i <numWorkingFolders; ++i) {
		settings.setArrayIndex(i);
		settings.setValue("PlaybackFolderName", mCurrentPlaybackFolderComboBox->itemText(i));
	}
	settings.endArray();

	settings.setValue("CurrentRecordFolderIndex",mCurrentRecordFolderComboBox->currentIndex());
	numWorkingFolders = mCurrentRecordFolderComboBox->count();
	settings.setValue("NumRecordWorkingFolders",numWorkingFolders);
	settings.beginWriteArray("RecordFolders");
	for (int i = 0; i <numWorkingFolders; ++i) {
		settings.setArrayIndex(i);
		settings.setValue("RecordFolderName", mCurrentRecordFolderComboBox->itemText(i));
	}
	settings.endArray();
}


void NTV2QtPlaycorder::AddDeviceEnumeratorGroupBox()
{
	mDeviceEnumeratorGroupBox = new QGroupBox();

	QLabel* hostDeviceLabel = new QLabel(tr("Host/Board"),this); 
	mDeviceChoiceCombo = new QComboBox(this);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(hostDeviceLabel);
	layout->addWidget(mDeviceChoiceCombo,Qt::AlignLeft);
	mDeviceEnumeratorGroupBox->setLayout(layout);
}

void NTV2QtPlaycorder::AddVideoPreviewGroupBox()
{
	mVideoPreviewGroupBox = new QGroupBox();
	mVideoPreviewWidget = new AJAPreviewWidget(this);
	mVideoPreviewWidget->setFixedWidth(AJAPREVIEW_WIDGET_X);
	mVideoPreviewWidget->setFixedHeight(AJAPREVIEW_WIDGET_Y);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(mVideoPreviewWidget);

	mVideoPreviewGroupBox->setLayout(layout);
}


void NTV2QtPlaycorder::AddVideoRecordFormatGroupBox()
{
	mVideoSetupGroupBox = new QGroupBox();
	QLabel* videoInputStatusString = new QLabel(tr("Video Input Status"),this); 
	mVideoInputStatusLabel = new QLabel(tr("1920x1080p_2400"),this); 
	QLabel* videoPlaybackFormatString = new QLabel(tr("Playback Video Format"),this); 
	mVideoPlaybackFormatCombo = new QComboBox(this);
	QLabel* mSequenceFileTypeString = new QLabel(tr("Record Sequence File Type"),this); 
	mSequenceFileTypeCombo = new QComboBox(this);
	mStopOnDrop = new QCheckBox("Stop on Dropped Frame",this);
	mPreviewWhenIdle = new QCheckBox("Preview Input When Idle",this);

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(videoInputStatusString,0,0,Qt::AlignRight);
	layout->addWidget(mVideoInputStatusLabel,0,1,Qt::AlignLeft);
	layout->addWidget(videoPlaybackFormatString,1,0,Qt::AlignRight);
	layout->addWidget(mVideoPlaybackFormatCombo,1,1,Qt::AlignLeft);
	layout->addWidget(mSequenceFileTypeString,2,0,Qt::AlignRight);
	layout->addWidget(mSequenceFileTypeCombo,2,1,Qt::AlignLeft);
	layout->addWidget(mStopOnDrop,3,0,Qt::AlignRight);
	layout->addWidget(mPreviewWhenIdle,3,1,Qt::AlignLeft);

	mVideoSetupGroupBox->setLayout(layout);

}

void NTV2QtPlaycorder::AddFolderGroupBox()
{
	mFolderGroupBox = new QGroupBox();
	QVBoxLayout *topLayout = new QVBoxLayout;


	QHBoxLayout* playbackFolderLayout = new QHBoxLayout;
	QLabel* currentPlaybackFolderLabel = new QLabel("Playback Folder: ");
	mCurrentPlaybackFolderComboBox = new QComboBox(this);
	mCurrentPlaybackFolderComboBox->setMaxCount( 15 );
	mAddPlaybackFolderButton = new QPushButton(tr("+"),this);
	mAddPlaybackFolderButton->setMaximumWidth(20);
	mAddPlaybackFolderButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	mRemovePlaybackFolderButton = new QPushButton(tr("-"),this);
	mRemovePlaybackFolderButton->setMaximumWidth(20);
	mRemovePlaybackFolderButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	playbackFolderLayout->addWidget(currentPlaybackFolderLabel);
	playbackFolderLayout->addWidget(mCurrentPlaybackFolderComboBox,Qt::AlignLeft);
	playbackFolderLayout->addWidget(mAddPlaybackFolderButton);
	playbackFolderLayout->addWidget(mRemovePlaybackFolderButton);

	QHBoxLayout* recordFolderLayout = new QHBoxLayout;
	QLabel* currentRecordFolderLabel = new QLabel("Record Folder:   ");
	mCurrentRecordFolderComboBox = new QComboBox(this);
	mCurrentRecordFolderComboBox->setMaxCount( 15 );
	mAddRecordFolderButton = new QPushButton(tr("+"),this);
	mAddRecordFolderButton->setMaximumWidth(20);
	mAddRecordFolderButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	mRemoveRecordFolderButton = new QPushButton(tr("-"),this);
	mRemoveRecordFolderButton->setMaximumWidth(20);
	mRemoveRecordFolderButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	recordFolderLayout->addWidget(currentRecordFolderLabel);
	recordFolderLayout->addWidget(mCurrentRecordFolderComboBox,Qt::AlignLeft);
	recordFolderLayout->addWidget(mAddRecordFolderButton);
	recordFolderLayout->addWidget(mRemoveRecordFolderButton);

	topLayout->addLayout(playbackFolderLayout);
	topLayout->addLayout(recordFolderLayout);

	mFolderGroupBox->setLayout(topLayout);
}

void NTV2QtPlaycorder::AddStatusEnumeratorGroupBox()
{
	mStatusEnumeratorGroupBox = new QGroupBox();
	QVBoxLayout *topLayout = new QVBoxLayout;

	mCurrentFramesProcessedLabel = new QLabel();	
	mCurrentFramesDroppedLabel = new QLabel();	

	QVBoxLayout* statusLayout = new QVBoxLayout();
	statusLayout->addWidget(mCurrentFramesProcessedLabel);
	statusLayout->addWidget(mCurrentFramesDroppedLabel);

	topLayout->addLayout(statusLayout);
	mStatusEnumeratorGroupBox->setLayout(topLayout);
}

void NTV2QtPlaycorder::AddTransportControls()
{
	mTransportControlGroupBox = new QGroupBox();

	mPlayButton = new QPushButton("Play");
	//mPlayButton->setIconSize(iconSize);
	mPlayButton->setObjectName("playPauseButton"); 
	mPlayButton->setCheckable(true); 
	mPlayButton->setChecked(false); 
	mPlayButton->setFocusPolicy(Qt::NoFocus); 
	mPlayButton->setMaximumWidth(80); 
	mPlayButton->setToolTip(tr("Play/Pause"));
	mPlayButton->setStyleSheet(sPlayStyleInactive);
	mPlayButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	connect(mPlayButton, SIGNAL(clicked()), this, SLOT(play()));

	mNextButton = new QPushButton("Next");
	mNextButton->setObjectName("forwardButton");
	mNextButton->setFocusPolicy(Qt::NoFocus);
	mNextButton->setMaximumWidth(80);
	mNextButton->setToolTip(tr("Next Frame"));
	mNextButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	connect(mNextButton, SIGNAL(clicked()), this, SLOT(next()));

	mPreviousButton = new QPushButton("Previous");
	mPreviousButton->setObjectName("backwardButton");
	mPreviousButton->setFocusPolicy(Qt::NoFocus);
	mPreviousButton->setMaximumWidth(80);
	mPreviousButton->setToolTip(tr("Previous Frame"));
	mPreviousButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	connect(mPreviousButton, SIGNAL(clicked()), this, SLOT(previous()));

	mStopButton = new QPushButton("Stop");
	mStopButton->setObjectName("stopButton");
	mStopButton->setToolTip("Stop");
	mStopButton->setFocusPolicy(Qt::NoFocus);
	mStopButton->setMaximumWidth(80);
	mStopButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	connect(mStopButton, SIGNAL(clicked()), this, SLOT(stop()));
 

	mRecordButton = new QPushButton("Record");
	mRecordButton->setObjectName("recordButton");
	mRecordButton->setToolTip("Record");
	mRecordButton->setFocusPolicy(Qt::NoFocus);
	mRecordButton->setMaximumWidth(80);
	mRecordButton->setStyleSheet(sRecordStyleInactive);
	mRecordButton->setAutoDefault(false);	// Needed to prevent activation by the Enter key
	connect(mRecordButton, SIGNAL(clicked()), this, SLOT(record()));


	mButtonsLayout = new QHBoxLayout;
	mButtonsLayout->addStretch();
	//mButtonsLayout->addWidget(mOpenButton);
	mButtonsLayout->addWidget(mPlayButton);
	//mButtonsLayout->addWidget(mPauseButton);
	mButtonsLayout->addWidget(mPreviousButton);
	mButtonsLayout->addWidget(mNextButton);
	mButtonsLayout->addWidget(mStopButton);
	mButtonsLayout->addWidget(mRecordButton);
	mButtonsLayout->addStretch();

	mTransportControlGroupBox->setLayout(mButtonsLayout);
}


void NTV2QtPlaycorder::AddScrubSlider()
{
	mScrubSliderGroupBox = new QGroupBox();
	mSpinBox = new QSpinBox();
	mSpinBox->setMinimum(0);
	mSpinBox->setMaximum(10000);
	mSpinBox->setKeyboardTracking(false);
	mSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

	mScrubSlider = new QSlider(Qt::Horizontal);
	mScrubSlider->setRange(0, 1);
	mScrubSlider->setValue(0);

	connect(mScrubSlider, SIGNAL(valueChanged(int)),
		mSpinBox, SLOT(setValue(int)));
	connect(mSpinBox, SIGNAL(valueChanged(int)),
		this, SLOT(scrubToFrame(int)));
	connect(mScrubSlider, SIGNAL(valueChanged(int)),
		this, SLOT(scrubToFrame(int)));

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(mSpinBox);
	layout->addWidget(mScrubSlider);
	setLayout(layout);

	mScrubSliderGroupBox->setLayout(layout);
}


void NTV2QtPlaycorder::AddSignalsAndSlots()
{
	connect(mDeviceChoiceCombo,SIGNAL(currentIndexChanged(int)),SLOT(NewBoardChoice(int)));
	connect(mAddPlaybackFolderButton,	SIGNAL(clicked()), this, SLOT(addPlaybackFolder()));
	connect(mRemovePlaybackFolderButton, SIGNAL(clicked()), this, SLOT(deleteCurrentPlaybackFolder()));	
	connect(mCurrentPlaybackFolderComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(NewPlaybackFolderChoice(int)));
	connect(mAddRecordFolderButton,	SIGNAL(clicked()), this, SLOT(addRecordFolder()));
	connect(mCurrentRecordFolderComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(NewRecordFolderChoice(int)));
	connect(mRemoveRecordFolderButton, SIGNAL(clicked()), this, SLOT(deleteCurrentRecordFolder()));	
	connect(mSequenceFileTypeCombo,SIGNAL(currentIndexChanged(int)),SLOT(NewSequenceFileTypeChoice(int)));
	connect(mSequenceFileTypeCombo,SIGNAL(currentIndexChanged(int)),SLOT(NewFileFormatChoice(int)));

	connect(this, SIGNAL(newFrameSignal(const QImage &,bool)),
				mVideoPreviewWidget, SLOT(updateFrame(const QImage &,bool)));

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(UpdateTimer()));
	timer->start(200);
}


void NTV2QtPlaycorder::AddItemsToLayoutManager()
{
	QVBoxLayout *mainLayout = new QVBoxLayout;

	mainLayout->addWidget(mDeviceEnumeratorGroupBox);
	mainLayout->addWidget(mVideoPreviewGroupBox);
	mainLayout->addWidget(mVideoSetupGroupBox);
	mainLayout->addWidget(mFolderGroupBox);
	mainLayout->addWidget(mTransportControlGroupBox);
	mainLayout->addWidget(mScrubSliderGroupBox);
	mainLayout->addWidget(mStatusEnumeratorGroupBox);
	setLayout(mainLayout);
}

void NTV2QtPlaycorder::SetupStyleSheetSettings()
{
	//mPlayButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
	//mPauseButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");	
	//mStopButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");	
	//mNextButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
	//mPreviousButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
	//mOpenButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
	//mScrubSlider->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
}

void NTV2QtPlaycorder::UpdateBoardEnumerator()
{
	mDeviceChoiceCombo->clear();

	uint32_t deviceCount = mPlaycorder->GetDeviceCount ();
	if (deviceCount == 0)
	{
		mDeviceChoiceCombo->addItem (tr ("No Boards Found"));
	}
	else
	{
		for (uint32_t i = 0; i < deviceCount; i++)
		{
			mDeviceChoiceCombo->addItem (mPlaycorder->GetDeviceString (i).c_str());
		}
	}	
}

void NTV2QtPlaycorder::NewFileFormatChoice(int newFormatIndex)
{
	(void) newFormatIndex;
	stopRecord();
}

void NTV2QtPlaycorder::UpdateVideoFormatEnumerator()
{
	mVideoPlaybackFormatCombo->addItem("Same As Input",QVariant((qlonglong)NTV2_FORMAT_UNKNOWN));
	mVideoPlaybackFormatCombo->addItem("Same As DPX",QVariant((qlonglong)-1));

#if TODO
	for ( int formatNumber = 1; formatNumber < NTV4_VideoFormat_Size; formatNumber++)
	{
		QString formatString = m_CorvidSetup.getFormatInfo((NTV4VideoFormat)formatNumber).c_str();
		if ( formatString.size() != 0 )
			mVideoPlaybackFormatCombo->addItem(formatString,QVariant(formatNumber));
	}
#endif
    mSequenceFileTypeCombo->addItem("RGB DPX LE",QVariant((int)NTV2_FBF_10BIT_DPX_LE));
	mSequenceFileTypeCombo->addItem("RGB DPX BE",QVariant((int)NTV2_FBF_10BIT_DPX));
	mSequenceFileTypeCombo->addItem("YCbCr DPX BE",QVariant((int)NTV2_FBF_10BIT_YCBCR_DPX));
	connect(mSequenceFileTypeCombo,SIGNAL(currentIndexChanged(int)),SLOT(NewFileFormatChoice(int)));
}


void NTV2QtPlaycorder::SetupScrubSlider(int32_t numFrames)
{
	if ( GetCurrentPlaybackDirectoryQString() != "" )
	{
		if ( numFrames > 0 )
		{
			mSpinBox->setMinimum(0);
			mSpinBox->setMaximum(numFrames);
			mScrubSlider->setRange(0, numFrames);
			mScrubSlider->setValue(0);
			mScrubSlider->setDisabled(false);
			mSpinBox->setDisabled(false);
		}
		else
		{
			mSpinBox->setMinimum(0);
			mSpinBox->setMaximum(1);
			mScrubSlider->setRange(0, 1);
			mScrubSlider->setValue(0);
			mScrubSlider->setDisabled(true);
			mSpinBox->setDisabled(true);
		}
	}
}


// SLOTS::::

void NTV2QtPlaycorder::NewBoardChoice(int boardIndex)
{
	if (mBoardIndex == (uint32_t)boardIndex)
		return;

	mBoardIndex = boardIndex;
	stop();
	if (mPlaycorder)
	{
		mPlaycorder->SetDevice (mBoardIndex);
	}
}


void NTV2QtPlaycorder::NewSequenceFileTypeChoice(int sequenceFileTypeIndex)
{
	(void) sequenceFileTypeIndex;
	stop();
}

static int timercounter = 0;
void NTV2QtPlaycorder::UpdateTimer()
{
	timercounter++;
	if ( mPlaycorder && mPlaycorder->IsPlaying () )
	{
		mRecordButton->setCheckable(false);

		if( mStopOnDrop->isChecked () && (mPlaycorder->GetPlayDropCount () > 0) )
			mPlaycorder->StopPlaying ();

		// Flash play button
		if ( (timercounter & 0x7) == 0 )
		{
			mPlayButton->setStyleSheet(sPlayStyleActiveLow);
		}
		else
		{
			mPlayButton->setStyleSheet(sPlayStyleActiveHigh);
		}

		if ( !mPlaycorder->GetPlayPaused ())
		{
			uint32_t currentFrame = mPlaycorder->GetPlayFrame ();
			mScrubSlider->setValue (currentFrame);
			mSpinBox->setValue (currentFrame);
		}
		//mScrubSlider->setValue(outputStatus.framesProcessed);
		//mCurrentFramesProcessedLabel->setText(QString("Frames Read: %1").arg(outputStatus.framesProcessed));
		//mCurrentFramesDroppedLabel->setText(QString("Frames Dropped: %1").arg(framesDropped));
	}
	else
	{
		if ( mPlaycorder )
		{
			mRecordButton->setCheckable(true);
			//mCurrentFramesProcessedLabel->setText(QString("Frames Written: %1").arg(inputStatus.framesProcessed));
			//mCurrentFramesDroppedLabel->setText(QString("Frames Dropped: %1").arg(framesDropped));

			if ( mRecording )
			{
				if( mStopOnDrop->isChecked () && (mPlaycorder->GetRecordDropCount () > 0) )
					mPlaycorder->StopRecording ();

				// Actually Recording
				if ( (timercounter & 0x7) == 0 )
				{
					mRecordButton->setStyleSheet(sRecordStyleActiveLow);
				}
				else
				{
					mRecordButton->setStyleSheet(sRecordStyleActiveHigh);
				}
			}
			else if ( !mPlaying )
			{
				mRecordButton->setChecked(false);

				if (!mPreviewing && (mPreviewWhenIdle->isChecked () == true))
				{
					mPreviewing = true;
				}
				else if (mPreviewing && (mPreviewWhenIdle->isChecked () == false))
				{
					mPreviewing = false;
				}
			}
		}
	}

	// Update Input Format:
	if (!mPlaying)
	{
		AJA_VideoFormat inFormat = mPlaycorder->GetInputFormat();
		if ( inFormat != AJA_VideoFormat_Unknown )
		{
			QString info( mPlaycorder->GetInputFormatString ().c_str() );
			QString coloredInfo = QString("<h3> <font color=green>%1</br> </font></h3>").arg(info);
			mVideoInputStatusLabel->setText(coloredInfo);
		}
		else
		{
			QString info = "No Input Found";
			QString coloredInfo = QString("<h3> <font color=red>%1</br> </font></h3>").arg(info);
			mVideoInputStatusLabel->setText(coloredInfo);
		}
	}

	if ( !mPreviewing && !mRecording && !mPlaying )
	{
		QImage image(":/resources/splash.png");
		emit newFrameSignal (image, true);
	}
}


void NTV2QtPlaycorder::addPlaybackFolder()
{
	stop();
	QString currentDirectoryString =QFileDialog::getExistingDirectory(this, tr("Open Directory"),GetCurrentPlaybackDirectoryQString(), QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
	if (!currentDirectoryString.isEmpty())
	{
		addPlaybackFolder(currentDirectoryString);
	}
}


void NTV2QtPlaycorder::addPlaybackFolder(QString dirString)
{
	mCurrentPlaybackFolderComboBox->addItem(dirString);
	mCurrentPlaybackFolderComboBox->setCurrentIndex(mCurrentPlaybackFolderComboBox->count()-1);

}


void NTV2QtPlaycorder::deleteCurrentPlaybackFolder()
{
	int index = mCurrentPlaybackFolderComboBox->currentIndex();
	if ( index != -1 )
	{
		mCurrentPlaybackFolderComboBox->removeItem(index);
	}
}


void NTV2QtPlaycorder::NewPlaybackFolderChoice(int newPlaybackFolderIndex)
{
	mCurrentPlaybackFolderComboBox->setCurrentIndex(newPlaybackFolderIndex);
	if (mPlaycorder)
	{
		mPlaycorder->SetPlayPath (GetCurrentPlaybackDirectoryStdString());
	}
}


void NTV2QtPlaycorder::addRecordFolder()
{
	stop();
	QString currentDirectoryString =QFileDialog::getExistingDirectory(this, tr("Open Directory"),GetCurrentRecordDirectoryQString(), QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
	if (!currentDirectoryString.isEmpty())
	{
		addRecordFolder(currentDirectoryString);
	}
}


void NTV2QtPlaycorder::addRecordFolder(QString dirString)
{
	mCurrentRecordFolderComboBox->addItem(dirString);
	mCurrentRecordFolderComboBox->setCurrentIndex(mCurrentRecordFolderComboBox->count()-1);

}


void NTV2QtPlaycorder::deleteCurrentRecordFolder()
{
	int index = mCurrentRecordFolderComboBox->currentIndex();
	if ( index != -1 )
	{
		mCurrentRecordFolderComboBox->removeItem(index);
	}
}


void NTV2QtPlaycorder::NewRecordFolderChoice(int newRecordFolderIndex)
{
	mCurrentRecordFolderComboBox->setCurrentIndex(newRecordFolderIndex);
	if (mPlaycorder)
	{
		mPlaycorder->SetRecordPath (GetCurrentRecordDirectoryStdString());
	}
}


void NTV2QtPlaycorder::play()
{
	if (mPlaying)
	{
		if (mPlaycorder->GetPlayPaused ())
		{
			mPlaycorder->SetPlayPaused (false);

			mPlayButton->setText("Playing");
		}
		else
		{
			mPlaycorder->SetPlayPaused (true);

			mPlayButton->setText("Paused");
		}

		mPlayButton->setStyleSheet(sPlayStyleActiveHigh);
	}
	else
	{
		// check for path existing and directory empty.
		if ( AJA_STATUS_SUCCESS != mAjaFileIO->DoesDirectoryExist(GetCurrentPlaybackDirectoryStdString()))
		{
			QMessageBox msgBox;
			msgBox.setText("Playback Folder Does Not Exist: First Set Playback Folder");
			msgBox.exec();
			return;
		}

		if ( AJA_STATUS_SUCCESS != mAjaFileIO->DoesDirectoryContain (GetCurrentPlaybackDirectoryStdString(),
																	 "*.DPX"))
		{
			QMessageBox msgBox;
			msgBox.setText("Directory has no DPX Files: Choose a Different Directory");
			msgBox.exec();
			return;
		}

		mPlaycorder->StopRecording ();

		mPreviewing	= false;
		mRecording	= false;
		mPlaying	= true;

		mPlaycorder->SetPlayPaused (false);
		mPlaycorder->StartPlaying (mClipLength);

		mPlayButton->setText("Playing");
		mPlayButton->setStyleSheet("QPushButton {color: rgb(0, 255, 0); background-color:black}");

		SetupScrubSlider (mClipLength);
	}
}


void NTV2QtPlaycorder::preview()
{
	stop();
}


void NTV2QtPlaycorder::record()
{
	if (mRecording)
		return;		// Not implementing pause during record yet

	int index = mSequenceFileTypeCombo->currentIndex();
	QVariant fileTypeVariant = mSequenceFileTypeCombo->itemData(index);

	NTV2VideoFormat selectedVideoFormat;
	QVariant vidformat = mVideoPlaybackFormatCombo->itemData(index);
	selectedVideoFormat = (NTV2VideoFormat)vidformat.toLongLong();

	// check for path existing and directory empty.
	if ( AJA_STATUS_SUCCESS != mAjaFileIO->DoesDirectoryExist(GetCurrentRecordDirectoryStdString()))
	{
		QMessageBox msgBox;
		msgBox.setText("Record Folder Does Not Exist: First Set Record Folder");
		msgBox.exec();
		return;
	}

	QString nextSequenceDirectory = findNextSequenceDirectory(GetCurrentRecordDirectoryQString());
	if ( nextSequenceDirectory.size() < 1 )
	{
		QMessageBox msgBox;
		msgBox.setText("Couldn't Create Sequence Directory");
		msgBox.exec();
		return;
	}

	std::string selectedDPXPath = GetCurrentPlaybackDirectoryStdString();
	addPlaybackFolder(nextSequenceDirectory);

	mPreviewing = false;
	mRecording  = true;

	mPlaycorder->SetRecordPath (nextSequenceDirectory.toStdString());
	mPlaycorder->WriteToStorage (true);

	// Check to see if thread already running
	if ( mPlaycorder && !mPlaycorder->IsRecording () )
	{
		mPlaycorder->StartRecording ();

		qRegisterMetaType<QImage>("QImage");
	}

	mRecordButton->setText("Recording");
	mRecordButton->setStyleSheet(sRecordStyleActiveHigh);

//	mRecord->setDirectoryPath(nextSequenceDirectory);
//	mRecord->setPreview(false); // go straight into record
}


void NTV2QtPlaycorder::stop()
{
	mScrubSlider->setValue(0);
	mScrubSlider->setDisabled(true);
	mSpinBox->setDisabled(true);
	mPlayButton->setChecked(false);
	mRecordButton->setChecked(false);

	stopPlay();
	stopRecord();
}


void NTV2QtPlaycorder::stopPlay()
{
	if ( mPlaycorder && mPlaycorder->IsPlaying () )
	{
		mPlaycorder->StopPlaying ();

		mPlaying	= false;
		mRecording	= false;
		mPreviewing = (mPreviewWhenIdle->isChecked () == true);

		mPlaycorder->StartRecording ();
	}

	mPlayButton->setText("Play");
	mPlayButton->setStyleSheet(sPlayStyleInactive);
}


void NTV2QtPlaycorder::stopRecord()
{
	if ( mPlaycorder )
	{
//		disconnect(mRecord, SIGNAL(newFrameSignal(const QImage &,bool)),
//			mVideoPreviewWidget, SLOT(updateFrame(const QImage &,bool)));
		mRecording = false;
		mPlaycorder->WriteToStorage (false);

		mPreviewing = (mPreviewWhenIdle->isChecked () == true);
	}

	mRecordButton->setText("Record");
	mRecordButton->setStyleSheet(sRecordStyleInactive);
}


void NTV2QtPlaycorder::next()
{
	if (mPlaying && mPlaycorder->GetPlayPaused ())
	{
		uint32_t currentFrame = mPlaycorder->GetPlayFrame ();
		if (currentFrame == (mClipLength - 1))
		{
			currentFrame = 0;
		}
		else
		{
			currentFrame++;
		}

		mPlaycorder->SetPlayFrame (currentFrame);
		mScrubSlider->setValue (currentFrame);
		mSpinBox->setValue (currentFrame);
	}
}


void NTV2QtPlaycorder::previous()
{
	if (mPlaying && mPlaycorder->GetPlayPaused ())
	{
		uint32_t currentFrame = mPlaycorder->GetPlayFrame ();
		if (currentFrame == 0)
		{
			currentFrame = mClipLength - 1;
		}
		else
		{
			currentFrame--;
		}

		mPlaycorder->SetPlayFrame (currentFrame);
		mScrubSlider->setValue (currentFrame);
		mSpinBox->setValue (currentFrame);
	}
}


void NTV2QtPlaycorder::scrubToFrame(int frame)
{
	mPlaycorder->SetPlayFrame (frame);
}	// scrubToFrame


const QString NTV2QtPlaycorder::GetCurrentPlaybackDirectoryQString()
{
	return mCurrentPlaybackFolderComboBox->currentText();
}


const std::string NTV2QtPlaycorder::GetCurrentPlaybackDirectoryStdString()
{
	return mCurrentPlaybackFolderComboBox->currentText().toStdString();

}


const QString NTV2QtPlaycorder::GetCurrentRecordDirectoryQString()
{
	return mCurrentRecordFolderComboBox->currentText();
}

const std::string NTV2QtPlaycorder::GetCurrentRecordDirectoryStdString()
{
	return mCurrentRecordFolderComboBox->currentText().toStdString();

}


void NTV2QtPlaycorder::StartPreviewThread ()
{
	mPreviewThread = new AJAThread ();
	mPreviewThread->Attach (PreviewThreadStatic, this);
	mPreviewThread->Start ();
}

void NTV2QtPlaycorder::PreviewThreadStatic (AJAThread * pThread, void * pContext)	//	static
{
	(void) pThread;

	NTV2QtPlaycorder * pInstance (reinterpret_cast <NTV2QtPlaycorder *> (pContext));
	pInstance->PreviewThread ();
}

QImage previewImage (AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y, QImage::Format_RGB888);

void NTV2QtPlaycorder::PreviewThread ()
{
	while (!mGlobalQuit)
	{
		IPlaycorder::PreviewFrameInfo * frameInfo = mPreviewCircularBuffer->StartConsumeNextBuffer ();
		if (frameInfo )
		{
			if( mPreviewing || mRecording || mPlaying )
			{
				::memcpy ((void*) previewImage.scanLine (0),
						  (void*) frameInfo->pPreviewFrame,
						  frameInfo->previewFrameSize);

				emit newFrameSignal (previewImage, true);
			}

			mPreviewCircularBuffer->EndConsumeNextBuffer ();
		}
	}	//	Loop until quit signaled
}


QString findNextSequenceDirectory(QString currentDirectory)
{
	QString nextSequenceDirectory; 
	uint32_t dirNum;
	for ( dirNum = 0; dirNum < 10000; dirNum++ )
	{
		QChar fillChar = '0';
		QString seqDirString = QString("Clip%1").arg(dirNum,4,10,fillChar);
		nextSequenceDirectory = currentDirectory + '/' + seqDirString;
		QDir dirPath(nextSequenceDirectory);
		if ( dirPath.exists() )
			continue;
		QDir seqDir(currentDirectory);
		seqDir.mkdir(seqDirString);
		break;
	}
	if ( dirNum == 10000 )
		nextSequenceDirectory = "";
	return nextSequenceDirectory;

}

