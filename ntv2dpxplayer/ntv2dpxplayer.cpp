#if defined(AJA_WINDOWS)
#include <windows.h>
#else
#include "sys/types.h"
#include "sys/stat.h"
//#include "sys/dirent.h"
#endif

#include "ntv2recorddpx.h"
#include "ntv2playbackdpx.h"

#include <string>
#include <sstream>
#include <ajabase/system/systemtime.h>
#include "ntv2dpxplayer.h"

bool doesDirectoryExist(QString path);
bool isDirectoryEmpty(QString path);
QString findNextSequenceDirectory(QString currentDirectory);


NTV2DPXPlayer::NTV2DPXPlayer(QWidget *parent, Qt::WFlags flags, const char *singleIPAddr)
: QDialog(parent, flags),m_playbackDPXThread(NULL),m_recordThread(NULL)
{
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

	NewBoardChoice(0);
	AddSignalsAndSlots();

	RecallSettings();
	SetupScrubSlider(0);
	stop();
}


NTV2DPXPlayer::~NTV2DPXPlayer()
{
	stop();
	SaveSettings();

}


void NTV2DPXPlayer::RecallSettings()
{
	QSettings settings(QSettings::UserScope, "aja" , "NTV2Player");
	m_videoPlaybackFormatCombo->setCurrentIndex(settings.value("CurrentVideoFormatIndex").toInt());
	if ( settings.value("PreviewInput").toBool() == true )
		m_previewWhenIdle->setCheckState(Qt::Checked);
	else
		m_previewWhenIdle->setCheckState(Qt::Unchecked);

	if ( settings.value("StopOnDrop").toBool() == true )
		m_stopOnDrop->setCheckState(Qt::Checked);
	else
		m_stopOnDrop->setCheckState(Qt::Unchecked);

	int numWorkingFolders = settings.value("NumPlaybackWorkingFolders").toInt();
	settings.beginReadArray("PlaybackFolders");
	for (int i = 0; i < numWorkingFolders; ++i) {
		settings.setArrayIndex(i);
		QString currentFolderString = settings.value("PlaybackFolderName").toString();
		m_currentPlaybackFolderComboBox->addItem(currentFolderString);
	}
	settings.endArray();

	int currentIndex = settings.value("CurrentPlaybackFolderIndex").toInt();
	m_currentPlaybackFolderComboBox->setCurrentIndex(currentIndex);

	numWorkingFolders = settings.value("NumRecordWorkingFolders").toInt();
	settings.beginReadArray("RecordFolders");
	for (int i = 0; i < numWorkingFolders; ++i) {
		settings.setArrayIndex(i);
		QString currentFolderString = settings.value("RecordFolderName").toString();
		m_currentRecordFolderComboBox->addItem(currentFolderString);
	}
	settings.endArray();

	currentIndex = settings.value("CurrentRecordFolderIndex").toInt();
	m_currentRecordFolderComboBox->setCurrentIndex(currentIndex);


}



#include <QtDebug>
void NTV2DPXPlayer::SaveSettings()
{
	QSettings settings(QSettings::UserScope, "aja" , "NTV2Player");
	settings.setValue("CurrentVideoFormatIndex",m_videoPlaybackFormatCombo->currentIndex());
	settings.setValue("StopOnDrop",m_stopOnDrop->isChecked());
	settings.setValue("PreviewInput",m_previewWhenIdle->isChecked());

	settings.setValue("CurrentPlaybackFolderIndex",m_currentPlaybackFolderComboBox->currentIndex());
	int numWorkingFolders = m_currentPlaybackFolderComboBox->count();
	settings.setValue("NumPlaybackWorkingFolders",numWorkingFolders);
	settings.beginWriteArray("PlaybackFolders");
	for (int i = 0; i <numWorkingFolders; ++i) {
		settings.setArrayIndex(i);
		settings.setValue("PlaybackFolderName", m_currentPlaybackFolderComboBox->itemText(i));
	}
	settings.endArray();


	settings.setValue("CurrentRecordFolderIndex",m_currentRecordFolderComboBox->currentIndex());
	numWorkingFolders = m_currentRecordFolderComboBox->count();
	settings.setValue("NumRecordWorkingFolders",numWorkingFolders);
	settings.beginWriteArray("RecordFolders");
	for (int i = 0; i <numWorkingFolders; ++i) {
		settings.setArrayIndex(i);
		settings.setValue("RecordFolderName", m_currentRecordFolderComboBox->itemText(i));
	}
	settings.endArray();


}


void NTV2DPXPlayer::AddDeviceEnumeratorGroupBox()
{
	m_deviceEnumeratorGroupBox = new QGroupBox();

	QLabel* hostDeviceLabel = new QLabel(tr("Host/Board"),this); 
	m_deviceChoiceCombo = new AjaComboBox(this);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(hostDeviceLabel);
	layout->addWidget(m_deviceChoiceCombo,Qt::AlignLeft);
	m_deviceEnumeratorGroupBox->setLayout(layout);
}

void NTV2DPXPlayer::AddVideoPreviewGroupBox()
{
	m_videoPreviewGroupBox = new QGroupBox();
	m_videoPreviewWidget = new AJAPreviewWidget(this);
	m_videoPreviewWidget->setFixedWidth(AJAPREVIEW_WIDGET_X);
	m_videoPreviewWidget->setFixedHeight(AJAPREVIEW_WIDGET_Y);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(m_videoPreviewWidget);

	m_videoPreviewGroupBox->setLayout(layout);

	//QImage image(":/resources/splash.png");
	//m_videoPreviewWidget->updateFrame(image,true);

}


void NTV2DPXPlayer::AddVideoRecordFormatGroupBox()
{
	m_videoSetupGroupBox = new QGroupBox();
	QLabel* videoInputStatusString = new QLabel(tr("Video Input Status"),this); 
	m_videoInputStatusLabel = new QLabel(tr("1920x1080p_2400"),this); 
	QLabel* videoPlaybackFormatString = new QLabel(tr("Playback Video Format"),this); 
	m_videoPlaybackFormatCombo = new AjaComboBox(this);
	QLabel* m_sequenceFileTypeString = new QLabel(tr("Record Sequence File Type"),this); 
	m_sequenceFileTypeCombo = new AjaComboBox(this);
	m_stopOnDrop = new QCheckBox("Stop on Dropped Frame",this);
	m_previewWhenIdle = new QCheckBox("Preview Input When Idle",this);

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(videoInputStatusString,0,0);
	layout->addWidget(m_videoInputStatusLabel,0,1);
	layout->addWidget(videoPlaybackFormatString,1,0);
	layout->addWidget(m_videoPlaybackFormatCombo,1,1);
	layout->addWidget(m_sequenceFileTypeString,2,0);
	layout->addWidget(m_sequenceFileTypeCombo,2,1);
	layout->addWidget(m_stopOnDrop,3,0);
	layout->addWidget(m_previewWhenIdle,3,1);

	m_videoSetupGroupBox->setLayout(layout);

}

void NTV2DPXPlayer::AddFolderGroupBox()
{
	m_folderGroupBox = new QGroupBox();
	QVBoxLayout *topLayout = new QVBoxLayout;


	QHBoxLayout* playbackFolderLayout = new QHBoxLayout;
	QLabel* currentPlaybackFolderLabel = new QLabel("Playback Folder: ");
	m_currentPlaybackFolderComboBox = new AjaComboBox(this);
	m_currentPlaybackFolderComboBox->setMaxCount( 15 );
	m_addPlaybackFolderButton = new QPushButton(tr("+"),this);
	m_addPlaybackFolderButton->setMaximumWidth(20);
	m_removePlaybackFolderButton = new QPushButton(tr("-"),this);
	m_removePlaybackFolderButton->setMaximumWidth(20);
	playbackFolderLayout->addWidget(currentPlaybackFolderLabel);
	playbackFolderLayout->addWidget(m_currentPlaybackFolderComboBox,Qt::AlignLeft);
	playbackFolderLayout->addWidget(m_addPlaybackFolderButton);
	playbackFolderLayout->addWidget(m_removePlaybackFolderButton);

	QHBoxLayout* recordFolderLayout = new QHBoxLayout;
	QLabel* currentRecordFolderLabel = new QLabel("Record Folder:   ");
	m_currentRecordFolderComboBox = new AjaComboBox(this);
	m_currentRecordFolderComboBox->setMaxCount( 15 );
	m_addRecordFolderButton = new QPushButton(tr("+"),this);
	m_addRecordFolderButton->setMaximumWidth(20);
	m_removeRecordFolderButton = new QPushButton(tr("-"),this);
	m_removeRecordFolderButton->setMaximumWidth(20);
	recordFolderLayout->addWidget(currentRecordFolderLabel);
	recordFolderLayout->addWidget(m_currentRecordFolderComboBox,Qt::AlignLeft);
	recordFolderLayout->addWidget(m_addRecordFolderButton);
	recordFolderLayout->addWidget(m_removeRecordFolderButton);

	topLayout->addLayout(playbackFolderLayout);
	topLayout->addLayout(recordFolderLayout);

	m_folderGroupBox->setLayout(topLayout);
}

void NTV2DPXPlayer::AddStatusEnumeratorGroupBox()
{
	m_statusEnumeratorGroupBox = new QGroupBox();
	QVBoxLayout *topLayout = new QVBoxLayout;



	m_currentFramesProcessedLabel = new QLabel();	
	m_currentFramesDroppedLabel = new QLabel();	

	QVBoxLayout* statusLayout = new QVBoxLayout();
	statusLayout->addWidget(m_currentFramesProcessedLabel);
	statusLayout->addWidget(m_currentFramesDroppedLabel);

	topLayout->addLayout(statusLayout);
	m_statusEnumeratorGroupBox->setLayout(topLayout);


	m_deviceEnumeratorGroupBox->setLayout(topLayout);
}
void NTV2DPXPlayer::AddTransportControls()
{
	m_transportControlGroupBox = new QGroupBox();

	m_playButton = new AjaPushButton();
	//m_playButton->setIconSize(iconSize);
	m_playButton->setObjectName("playPauseButton"); 
	m_playButton->setCheckable(true); 
	m_playButton->setChecked(false); 
	m_playButton->setFocusPolicy(Qt::NoFocus); 
	m_playButton->setMaximumWidth(80); 
	m_playButton->setToolTip(tr("Play/Pause"));
	connect(m_playButton, SIGNAL(clicked()), this, SLOT(play()));

	m_nextButton = new AjaPushButton();
	m_nextButton->setObjectName("forwardButton");
	m_nextButton->setFocusPolicy(Qt::NoFocus);
	m_nextButton->setMaximumWidth(80);
	m_nextButton->setToolTip(tr("Next Frame"));
	connect(m_nextButton, SIGNAL(clicked()), this, SLOT(next()));

	m_previousButton = new AjaPushButton();
	m_previousButton->setObjectName("backwardButton");
	m_previousButton->setFocusPolicy(Qt::NoFocus);
	m_previousButton->setMaximumWidth(80);
	m_previousButton->setToolTip(tr("Previous Frame"));
	connect(m_previousButton, SIGNAL(clicked()), this, SLOT(previous()));

	m_stopButton = new AjaPushButton();
	m_stopButton->setObjectName("stopButton");
	m_stopButton->setToolTip("Stop");
	m_stopButton->setFocusPolicy(Qt::NoFocus);
	m_stopButton->setMaximumWidth(80);
	connect(m_stopButton, SIGNAL(clicked()), this, SLOT(stop()));
 

	m_recordButton = new AjaPushButton();
	m_recordButton->setObjectName("recordButton");
	m_recordButton->setToolTip("Record");
	m_recordButton->setFocusPolicy(Qt::NoFocus);
	m_recordButton->setMaximumWidth(80);
	connect(m_recordButton, SIGNAL(clicked()), this, SLOT(record()));


	m_buttonsLayout = new QHBoxLayout;
	m_buttonsLayout->addStretch();
	//m_buttonsLayout->addWidget(m_openButton);
	m_buttonsLayout->addWidget(m_playButton);
	//m_buttonsLayout->addWidget(m_pauseButton);
	m_buttonsLayout->addWidget(m_previousButton);
	m_buttonsLayout->addWidget(m_nextButton);
	m_buttonsLayout->addWidget(m_stopButton);
	m_buttonsLayout->addWidget(m_recordButton);
	m_buttonsLayout->addStretch();

	m_transportControlGroupBox->setLayout(m_buttonsLayout);


}


void NTV2DPXPlayer::AddScrubSlider()
{
	m_scrubSliderGroupBox = new QGroupBox();
	m_spinBox = new AjaSpinBox();
	m_spinBox->setMinimum(0);
	m_spinBox->setMaximum(10000);
	m_spinBox->setKeyboardTracking(false);
	m_spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

	m_scrubSlider = new AjaSlider(Qt::Horizontal);
	m_scrubSlider->setRange(0, 1);
	m_scrubSlider->setValue(0);

	connect(m_scrubSlider, SIGNAL(valueChanged(int)),
		m_spinBox, SLOT(setValue(int)));
	connect(m_spinBox, SIGNAL(valueChanged(int)),
		this, SLOT(scrubToFrame(int)));
	connect(m_scrubSlider, SIGNAL(valueChanged(int)),
		this, SLOT(scrubToFrame(int)));

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(m_spinBox);
	layout->addWidget(m_scrubSlider);
	setLayout(layout);

	m_scrubSliderGroupBox->setLayout(layout);

}


void NTV2DPXPlayer::AddSignalsAndSlots()
{
	connect(m_deviceChoiceCombo,SIGNAL(currentIndexChanged(int)),SLOT(NewBoardChoice(int)));
	connect(m_addPlaybackFolderButton,	SIGNAL(clicked()), this, SLOT(addPlaybackFolder()));
	connect(m_removePlaybackFolderButton, SIGNAL(clicked()), this, SLOT(deleteCurrentPlaybackFolder()));	
	connect(m_addRecordFolderButton,	SIGNAL(clicked()), this, SLOT(addRecordFolder()));
	connect(m_removeRecordFolderButton, SIGNAL(clicked()), this, SLOT(deleteCurrentRecordFolder()));	
	connect(m_sequenceFileTypeCombo,SIGNAL(currentIndexChanged(int)),SLOT(NewSequenceFileTypeChoice(int)));
	connect(m_sequenceFileTypeCombo,SIGNAL(currentIndexChanged(int)),SLOT(NewFileFormatChoice(int)));

	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(UpdateTimer()));
	timer->start(200);

}


void NTV2DPXPlayer::AddItemsToLayoutManager()
{
	QVBoxLayout *mainLayout = new QVBoxLayout;

	mainLayout->addWidget(m_deviceEnumeratorGroupBox);
	mainLayout->addWidget(m_videoPreviewGroupBox);
	mainLayout->addWidget(m_videoSetupGroupBox);
	mainLayout->addWidget(m_folderGroupBox);
	mainLayout->addWidget(m_transportControlGroupBox);
	mainLayout->addWidget(m_scrubSliderGroupBox);
	mainLayout->addWidget(m_statusEnumeratorGroupBox);
	setLayout(mainLayout);
}

void NTV2DPXPlayer::SetupStyleSheetSettings()
{
	//m_playButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
	//m_pauseButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");	
	//m_stopButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");	
	//m_nextButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
	//m_previousButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
	//m_openButton->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
	//m_scrubSlider->setStyleSheet("QPushButton{ background-color: rgb(36,148,188) ; }");
}

void NTV2DPXPlayer::UpdateBoardEnumerator()
{
#ifdef TODO
	NTV4DeviceFactory		factory;
	DeviceContainerType		deviceContainer;
#endif
	m_deviceChoiceCombo->clear();

	m_deviceChoiceCombo->addItem(tr("No Boards Installed"));

}

void NTV2DPXPlayer::NewFileFormatChoice(int newFormatIndex)
{
	stopRecord();
}

void NTV2DPXPlayer::UpdateVideoFormatEnumerator()
{
	m_videoPlaybackFormatCombo->addItem("Same As Input",QVariant((qlonglong)NTV2_FORMAT_UNKNOWN));
	//m_videoPlaybackFormatCombo->addItem("Same As DPX",QVariant((qlonglong)NTV4_VideoFormat_Size));

#if TODO
	for ( int formatNumber = 1; formatNumber < NTV4_VideoFormat_Size; formatNumber++)
	{
		QString formatString = m_CorvidSetup.getFormatInfo((NTV4VideoFormat)formatNumber).c_str();
		if ( formatString.size() != 0 )
			m_videoPlaybackFormatCombo->addItem(formatString,QVariant(formatNumber));
	}
#endif
	m_sequenceFileTypeCombo->addItem("RGB DPX LE",QVariant((int)NTV2_FBF_10BIT_DPX_LITTLEENDIAN));
	m_sequenceFileTypeCombo->addItem("RGB DPX BE",QVariant((int)NTV2_FBF_10BIT_DPX));
	m_sequenceFileTypeCombo->addItem("YCbCr DPX BE",QVariant((int)NTV2_FBF_10BIT_YCBCR_DPX));
	connect(m_sequenceFileTypeCombo,SIGNAL(currentIndexChanged(int)),SLOT(NewFileFormatChoice(int)));


}


void NTV2DPXPlayer::SetupScrubSlider(int32_t numFrames)
{
	if ( GetCurrentPlaybackDirectoryQString() != "" )
	{
		if ( numFrames > 0 )
		{
			m_spinBox->setMinimum(0);
			m_spinBox->setMaximum(numFrames);
			m_scrubSlider->setRange(0, numFrames);
			m_scrubSlider->setValue(0);
			m_scrubSlider->setDisabled(false);
			m_spinBox->setDisabled(false);
		}
		else
		{
			m_spinBox->setMinimum(0);
			m_spinBox->setMaximum(1);
			m_scrubSlider->setRange(0, 1);
			m_scrubSlider->setValue(0);
			m_scrubSlider->setDisabled(true);
			m_spinBox->setDisabled(true);
		}
	}
}


// SLOTS::::

void NTV2DPXPlayer::NewBoardChoice(int boardIndex)
{
	m_boardIndex = boardIndex;
#if 0
	AJAStatus status = m_CorvidSetup.initializeDevice(m_boardIndex);
	if ( AJA_FAILURE(status))
		m_boardOpened = false;
	else
		m_boardOpened = true;
#endif
}


void NTV2DPXPlayer::NewSequenceFileTypeChoice(int sequenceFileTypeIndex)
{
	stop();
}

static int timercounter = 0;
void NTV2DPXPlayer::UpdateTimer()
{
	timercounter++;
	if ( m_playbackDPXThread )
	{
		m_recordButton->setCheckable(false);
		uint32_t framesDropped = m_playbackDPXThread->getFramesDropped();
		//		if ( framesDropped && m_stopOnDrop )
		//		{
		//			stop();
		//			QMessageBox msgBox;
		//			msgBox.setText("A Frame has Dropped.");
		//			msgBox.exec();

		//		}
		//		else
		{
			m_scrubSlider->setValue(m_playbackDPXThread->getCurrentFrameNumber());
			m_currentFramesProcessedLabel->setText(QString("Frames Read: %1").arg(m_playbackDPXThread->getFramesRead()));
			m_currentFramesDroppedLabel->setText(QString("Frames Dropped: %1").arg(m_playbackDPXThread->getFramesDropped()));

		}

	}
	else
		if ( m_recordThread )
		{
			m_recordButton->setCheckable(true);
			uint32_t framesDropped = m_recordThread->getFramesDropped();
			//		if ( framesDropped && m_stopOnDrop )
			//		{
			//			stop();

			//			QMessageBox msgBox;
			//			msgBox.setText("A Frame has Dropped.");
			//			msgBox.exec();
			//		}
			//		else
			{
				m_currentFramesProcessedLabel->setText(QString("Frames Written: %1").arg(m_recordThread->getFramesWritten()));
				m_currentFramesDroppedLabel->setText(QString("Frames Dropped: %1").arg(m_recordThread->getFramesDropped()));
			}
#if 1
			if ( m_recordThread->getPreview() == false )
			{
				// Actually Recording
				if ( timercounter & 0x1 )
					m_recordButton->setChecked(true);
				else
					m_recordButton->setChecked(false);
			}
			else
				m_recordButton->setChecked(false);
#endif
			if ( m_previewWhenIdle->isChecked() == false )
			{
				if ( m_recordThread->getPreview() == true )
				{
					disconnect(m_recordThread, SIGNAL(newFrameSignalWithROI(const QImage &,ROIRectList,bool)),
						m_videoPreviewWidget, SLOT(updateFrameWithROI(const QImage &,bool,ROIRectList)));
					disconnect(m_recordThread, SIGNAL(newStatusString(const QString)),
						m_videoPreviewWidget, SLOT(updateStatusString(const QString)));


					delete m_recordThread;
					m_recordThread = NULL;

				}
			}
		}
		else if ( m_previewWhenIdle->isChecked() && m_boardOpened  )
		{
			int index = m_sequenceFileTypeCombo->currentIndex();
			QVariant fileTypeVariant = m_sequenceFileTypeCombo->itemData(index);
			NTV2FrameBufferFormat pixelFormat = (NTV2FrameBufferFormat)fileTypeVariant.toInt();

			int selectedDeviceIndex = m_deviceChoiceCombo->currentIndex();
			m_recordThread = new NTV2RecordDPX(selectedDeviceIndex,NULL,true,false,pixelFormat);

			qRegisterMetaType<QImage>("QImage");
			connect(m_recordThread, SIGNAL(newFrameSignal(const QImage &,bool)),
				m_videoPreviewWidget, SLOT(updateFrame(const QImage &,bool)));

			connect(m_recordThread, SIGNAL(newStatusString(const QString)),
				m_videoPreviewWidget, SLOT(updateStatusString(const QString)));

			m_recordThread->start();
			m_recordThread->setPriority(QThread::TimeCriticalPriority);

		}


		if ( (m_recordThread == NULL) && (m_playbackDPXThread== NULL))
		{
			QImage image(":/resources/splash.png");
			m_videoPreviewWidget->updateFrameWithStatus(image,"",true);

		}
#if TODO
		// Update Input Format:
		NTV4VideoConfiguration vc;
		if (m_boardOpened)
		{
			m_CorvidSetup.determineInputConfig(vc,true);
			NTV4VideoFormat videoFormat;
			vc.GetVideoFormat(videoFormat);
			QString info = m_CorvidSetup.getFormatInfo(videoFormat).c_str();
			QString coloredInfo = QString("<h3> <font color=green>%1</br> </font></h3>").arg(info);
			m_videoInputStatusLabel->setText(coloredInfo);
		}
		else
		{
			QString info = "No Input Found";
			QString coloredInfo = QString("<h3> <font color=red>%1</br> </font></h3>").arg(info);
			m_videoInputStatusLabel->setText(coloredInfo);

		}
#endif


}


void NTV2DPXPlayer::open()
{
	stop();
	QString currentDirectoryString =QFileDialog::getExistingDirectory(this, tr("Open Directory"),GetCurrentPlaybackDirectoryQString(), QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);

	if (!currentDirectoryString.isEmpty())
	{
		m_currentPlaybackFolderComboBox->addItem(currentDirectoryString);
		m_currentPlaybackFolderComboBox->setCurrentIndex(m_currentPlaybackFolderComboBox->count()-1);
		//SetupScrubSlider();
	}


}


void NTV2DPXPlayer::addPlaybackFolder()
{
	stop();
	QString currentDirectoryString =QFileDialog::getExistingDirectory(this, tr("Open Directory"),GetCurrentPlaybackDirectoryQString(), QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
	if (!currentDirectoryString.isEmpty())
	{
		addPlaybackFolder(currentDirectoryString);
	}
}

void NTV2DPXPlayer::addPlaybackFolder(QString dirString)
{
	m_currentPlaybackFolderComboBox->addItem(dirString);
	m_currentPlaybackFolderComboBox->setCurrentIndex(m_currentPlaybackFolderComboBox->count()-1);

}

void NTV2DPXPlayer::deleteCurrentPlaybackFolder()
{
	int index = m_currentPlaybackFolderComboBox->currentIndex();
	if ( index != -1 )
	{
		m_currentPlaybackFolderComboBox->removeItem(index);
	}
}

void NTV2DPXPlayer::addRecordFolder()
{
	stop();
	QString currentDirectoryString =QFileDialog::getExistingDirectory(this, tr("Open Directory"),GetCurrentRecordDirectoryQString(), QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
	if (!currentDirectoryString.isEmpty())
	{
		addRecordFolder(currentDirectoryString);
	}
}

void NTV2DPXPlayer::addRecordFolder(QString dirString)
{
	m_currentRecordFolderComboBox->addItem(dirString);
	m_currentRecordFolderComboBox->setCurrentIndex(m_currentRecordFolderComboBox->count()-1);

}

void NTV2DPXPlayer::deleteCurrentRecordFolder()
{
	int index = m_currentRecordFolderComboBox->currentIndex();
	if ( index != -1 )
	{
		m_currentRecordFolderComboBox->removeItem(index);
	}
}

void NTV2DPXPlayer::play()
{
	if (!m_boardOpened)
		return;

	if ( m_playbackDPXThread )
	{
		if ( m_playButton->isChecked())
		{
			m_playbackDPXThread->setPause(false);
		}
		else
		{
			m_playbackDPXThread->setPause(true);
		}
		return;
	}
	else
	{
		stopRecord();
	}


	int selectedDeviceIndex = m_deviceChoiceCombo->currentIndex();
	int index = m_videoPlaybackFormatCombo->currentIndex();
	QVariant vidformat = m_videoPlaybackFormatCombo->itemData(index);
	NTV2VideoFormat selectedVideoFormat;
	selectedVideoFormat = (NTV2VideoFormat)vidformat.toLongLong();

	// check for path existing and directory empty.
	if ( false == doesDirectoryExist(GetCurrentPlaybackDirectoryQString()))
	{
		QMessageBox msgBox;
		msgBox.setText("Working Folder Does Not Exist: First Set Working Folder");
		msgBox.exec();
		return;
	}
	if ( true == isDirectoryEmpty(GetCurrentPlaybackDirectoryQString()))
	{
		QMessageBox msgBox;
		msgBox.setText("Directory has no DPX Files: Choose a Different Directory");
		msgBox.exec();
		return;
	}

	std::string selectedDPXPath = GetCurrentPlaybackDirectoryStdString();
	m_playbackDPXThread = new NTV2PlaybackDPX(selectedDeviceIndex,selectedDPXPath,selectedVideoFormat);


	qRegisterMetaType<QImage>("QImage");
	connect(m_playbackDPXThread, SIGNAL(newFrameSignal(const QImage &,bool)),
		m_videoPreviewWidget, SLOT(updateFrame(const QImage &,bool)));

	connect(m_playbackDPXThread, SIGNAL(newStatusString(const QString)),
		m_videoPreviewWidget, SLOT(updateStatusString(const QString)));

	m_playbackDPXThread->start();
	m_playbackDPXThread->setPriority(QThread::TimeCriticalPriority);

	AJATime::Sleep(200);
	SetupScrubSlider(m_playbackDPXThread->getNumberOfFrames());


}

void NTV2DPXPlayer::preview()
{
	stop();
}

void NTV2DPXPlayer::record()
{
	if (!m_boardOpened)
		return;

	int index = m_sequenceFileTypeCombo->currentIndex();
	QVariant fileTypeVariant = m_sequenceFileTypeCombo->itemData(index);
	NTV2FrameBufferFormat pixelFormat = (NTV2FrameBufferFormat)fileTypeVariant.toInt();

	if ( 1 )
	{
		int selectedDeviceIndex = m_deviceChoiceCombo->currentIndex();

		NTV2VideoFormat selectedVideoFormat;
		int index = m_videoPlaybackFormatCombo->currentIndex();
		QVariant vidformat = m_videoPlaybackFormatCombo->itemData(index);
		selectedVideoFormat = (NTV2VideoFormat)vidformat.toLongLong();

		// check for path existing and directory empty.
		if ( false == doesDirectoryExist(GetCurrentRecordDirectoryQString()))
		{
			QMessageBox msgBox;
			msgBox.setText("Working Folder Does Not Exist: First Set Working Folder");
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

		// Check to see if thread already running
		if ( !m_recordThread )
		{
			// If not stop other threads and get this one going
			stop();
			m_recordThread = new NTV2RecordDPX(selectedDeviceIndex,nextSequenceDirectory,true,false,pixelFormat);
			qRegisterMetaType<QImage>("QImage");
			connect(m_recordThread, SIGNAL(newFrameSignal(const QImage &,bool)),
				m_videoPreviewWidget, SLOT(updateFrame(const QImage &,bool)));
			connect(m_recordThread, SIGNAL(newStatusString(const QString)),
				m_videoPreviewWidget, SLOT(updateStatusString(const QString)));

			m_recordThread->start();
			m_recordThread->setPriority(QThread::TimeCriticalPriority);
		}

		m_recordThread->setDirectoryPath(nextSequenceDirectory);
		m_recordThread->setPreview(false); // go straight into record

	}
}


void NTV2DPXPlayer::stop()
{
	m_scrubSlider->setValue(0);
	m_scrubSlider->setDisabled(true);
	m_spinBox->setDisabled(true);
	m_playButton->setChecked(false);
	m_recordButton->setChecked(false);

	stopPlay();
	stopRecord();
}

void NTV2DPXPlayer::stopPlay()
{

	if ( m_playbackDPXThread )
	{
		delete m_playbackDPXThread;
		m_playbackDPXThread = NULL;
	}

}

void NTV2DPXPlayer::stopRecord()
{

	if ( m_recordThread )
	{
		disconnect(m_recordThread, SIGNAL(newFrameSignal(const QImage &,bool)),
			m_videoPreviewWidget, SLOT(updateFrame(const QImage &,bool)));
		delete m_recordThread;
		m_recordThread = NULL;
	}
}

void NTV2DPXPlayer::next()
{
	if ( m_playbackDPXThread )
	{
		m_playbackDPXThread->nextFrame();	
	}
}


void NTV2DPXPlayer::previous()
{
	if ( m_playbackDPXThread )
	{
		m_playbackDPXThread->previousFrame();
	}
}


void NTV2DPXPlayer::setPaused(bool pause)
{
	if ( m_playbackDPXThread )
	{
		m_playbackDPXThread->setPause(pause);
		if ( pause )
		{
			bool c = m_pauseButton->isChecked();
			m_pauseButton->setChecked(true);
			c = m_pauseButton->isChecked();
			m_scrubSlider->setDisabled(false);
			m_spinBox->setDisabled(false);
		}
		else
		{
			m_pauseButton->setChecked(false);
			m_scrubSlider->setDisabled(true);
			m_spinBox->setDisabled(true);
		}
	}
}


void NTV2DPXPlayer::scrubToFrame(int frame)
{
	if ( m_playbackDPXThread )
	{
		m_playbackDPXThread->setCurrentFrame(frame);
	}

}
const QString NTV2DPXPlayer::GetCurrentPlaybackDirectoryQString()
{
	return m_currentPlaybackFolderComboBox->currentText();
}

const std::string NTV2DPXPlayer::GetCurrentPlaybackDirectoryStdString()
{
	return m_currentPlaybackFolderComboBox->currentText().toStdString();

}

const QString NTV2DPXPlayer::GetCurrentRecordDirectoryQString()
{
	return m_currentRecordFolderComboBox->currentText();
}

const std::string NTV2DPXPlayer::GetCurrentRecordDirectoryStdString()
{
	return m_currentRecordFolderComboBox->currentText().toStdString();

}


bool isDirectoryEmpty(QString path)
{
	QDir dirpath(path);
	QStringList dirList = dirpath.entryList();
	if ( dirList.size() > 2 )
		return false;
	else
		return true;
}


bool doesDirectoryExist(QString path)
{
	QDir dirpath(path);
	return dirpath.exists();
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
