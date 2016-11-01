#ifndef NTV2DPXPLAYER_H
#define NTV2DPXPLAYER_H

#include <QtGui>
#include <vector>
#include "ajatypes.h"
#include "ajapreviewwidget.h"
#include "ntv2card.h"
#include "ntv2boardscan.h"

#include "ntv2record.h"
#include "ntv2playback.h"

#include "ajainternal/gui/ajacombobox.h"
#include "ajainternal/gui/ajaslider.h"
#include "ajainternal/gui/ajaspinbox.h"
#include "ajainternal/gui/ajapushbutton.h"


 
class NTV2DPXPlayer : public QDialog
{
	Q_OBJECT

public:
	NTV2DPXPlayer(QWidget *parent = 0, Qt::WFlags flags = 0, const char *singleIPAddr = NULL);
	~NTV2DPXPlayer();

	void AddDeviceEnumeratorGroupBox();
	void AddVideoPreviewGroupBox();
	void AddVideoRecordFormatGroupBox();
	void AddFolderGroupBox();
	void AddStatusEnumeratorGroupBox();
	void AddItemsToLayoutManager();
	void AddSignalsAndSlots();
	void SetupStyleSheetSettings();
	void UpdateBoardEnumerator();
	void UpdateVideoFormatEnumerator();
	void AddTransportControls();
	void AddScrubSlider();
	void SetupScrubSlider(int32_t numFrames);
	void openFile(const QString &fileName);

	const QString	GetQuicktimeMovieFileString() { return GetCurrentPlaybackDirectoryQString();}

	const QString GetCurrentPlaybackDirectoryQString();
	const std::string GetCurrentPlaybackDirectoryStdString();
	const QString GetCurrentRecordDirectoryQString();
	const std::string GetCurrentRecordDirectoryStdString();


protected:
	void RecallSettings();
	void SaveSettings();

	void addPlaybackFolder(QString dirString);
	void addRecordFolder(QString dirString);

	void stopPlay();
	void stopRecord();

private slots:
	void NewBoardChoice(int boardIndex);
	void UpdateTimer();
	void play();
	void preview();
	void record();
	void stop();
	void next();
	void previous();
	void setPaused(bool pause);
	void open();
	void scrubToFrame(int frame);
	void addPlaybackFolder();
	void addRecordFolder();
	void deleteCurrentPlaybackFolder();
	void deleteCurrentRecordFolder();
	void NewSequenceFileTypeChoice(int sequenceFileTypeIndex);
	void NewFileFormatChoice(int newFormatIndex);

private:

	QGroupBox*		m_deviceEnumeratorGroupBox;
	AjaComboBox*	m_deviceChoiceCombo;
	QPushButton*	m_discorveryButton;
	AjaComboBox*	m_hostnameMaskCombo;


	QGroupBox*		m_videoSetupGroupBox;
	QLabel*			m_videoInputStatusLabel;
	AjaComboBox*	m_videoPlaybackFormatCombo;
	AjaComboBox*	m_sequenceFileTypeCombo;
	QCheckBox*		m_stopOnDrop;					 
	QCheckBox*		m_previewWhenIdle;					 

	QGroupBox*		m_transportControlGroupBox;
	QPushButton*	m_openButton;
	AjaPushButton*	m_playButton;
	AjaPushButton*	m_recordButton;
	QPushButton*	m_pauseButton;
	QPushButton*	m_stopButton;
	AjaPushButton*	m_nextButton;
	AjaPushButton*	m_previousButton;
	QHBoxLayout*	m_buttonsLayout;

	QGroupBox*		m_scrubSliderGroupBox;
	AjaSlider*		m_scrubSlider;
	AjaSpinBox*		m_spinBox;

	QGroupBox*		m_folderGroupBox;
	AjaComboBox*	m_currentPlaybackFolderComboBox;
	QPushButton*	m_addPlaybackFolderButton;
	QPushButton*	m_removePlaybackFolderButton;
	AjaComboBox*	m_currentRecordFolderComboBox;
	QPushButton*	m_addRecordFolderButton;
	QPushButton*	m_removeRecordFolderButton;

	QGroupBox*		m_statusEnumeratorGroupBox;
	QLabel*			m_currentMovieLabel;
	QLabel*			m_currentFramesProcessedLabel;
	QLabel*			m_currentFramesDroppedLabel;

	QGroupBox*		m_videoPreviewGroupBox;
	AJAPreviewWidget* m_videoPreviewWidget;

	uint32_t		m_boardIndex;
	bool			m_boardOpened;


	int32_t					m_activeEditRegister;
	char*					m_singleIPAddr;

	NTV2Playback*			m_playbackDPXThread;
	NTV2Record*				m_recordThread;
};

#endif // NTV2DPXPLAYER_H
