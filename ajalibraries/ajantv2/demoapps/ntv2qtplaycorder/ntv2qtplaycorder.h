/**
		@file			ntv2qtplaycorder.h 
		@brief			Header file for the NTV2QtPlaycorder demonstration application.
						This is the UI for an application that captures and plays back files.
		@copyright		Copyright 2013 AJA Video Systems, Inc. All rights reserved. 
**/

#ifndef NTV2QTPLAYCORDER_H
#define NTV2QTPLAYCORDER_H

#include <QtCore>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtWidgets>
#else
    #include <QtGui>
#endif
#include <vector>

#include "ajastuff/common/types.h"
#include "ajapreviewwidget.h"
#include "iplaycorder.h"


class AJAFileIO;
class AJAThread;

 
class NTV2QtPlaycorder : public QDialog
{
	Q_OBJECT

	public:

    NTV2QtPlaycorder(QWidget *parent = 0, Qt::WindowFlags flags = 0, const char *singleIPAddr = NULL);
	~NTV2QtPlaycorder();

	protected:

	// Start of QT UI member variables

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

	const QString GetCurrentPlaybackDirectoryQString();
	const std::string GetCurrentPlaybackDirectoryStdString();
	const QString GetCurrentRecordDirectoryQString();
	const std::string GetCurrentRecordDirectoryStdString();

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
	void scrubToFrame(int frame);
	void addPlaybackFolder();
	void addRecordFolder();
	void deleteCurrentPlaybackFolder();
	void deleteCurrentRecordFolder();
	void NewPlaybackFolderChoice(int newPlaybackFolderIndex);
	void NewRecordFolderChoice(int newRecordFolderIndex);
	void NewSequenceFileTypeChoice(int sequenceFileTypeIndex);
	void NewFileFormatChoice(int newFormatIndex);

	signals:

	void newFrameSignal(const QImage & image, bool clear);

	private:

	QGroupBox*			mDeviceEnumeratorGroupBox;
	QComboBox*			mDeviceChoiceCombo;
	QPushButton*		mDiscorveryButton;
	QComboBox*			mHostnameMaskCombo;


	QGroupBox*			mVideoSetupGroupBox;
	QLabel*				mVideoInputStatusLabel;
	QComboBox*			mVideoPlaybackFormatCombo;
	QComboBox*			mSequenceFileTypeCombo;
	QCheckBox*			mStopOnDrop;					 
	QCheckBox*			mPreviewWhenIdle;					 

	QGroupBox*			mTransportControlGroupBox;
	QPushButton*		mOpenButton;
	QPushButton*		mPlayButton;
	QPushButton*		mRecordButton;
	QPushButton*		mPauseButton;
	QPushButton*		mStopButton;
	QPushButton*		mNextButton;
	QPushButton*		mPreviousButton;
	QHBoxLayout*		mButtonsLayout;

	QGroupBox*			mScrubSliderGroupBox;
	QSlider*			mScrubSlider;
	QSpinBox*			mSpinBox;

	QGroupBox*			mFolderGroupBox;
	QComboBox*			mCurrentPlaybackFolderComboBox;
	QPushButton*		mAddPlaybackFolderButton;
	QPushButton*		mRemovePlaybackFolderButton;
	QComboBox*			mCurrentRecordFolderComboBox;
	QPushButton*		mAddRecordFolderButton;
	QPushButton*		mRemoveRecordFolderButton;

	QGroupBox*			mStatusEnumeratorGroupBox;
	QLabel*				mCurrentMovieLabel;
	QLabel*				mCurrentFramesProcessedLabel;
	QLabel*				mCurrentFramesDroppedLabel;

	QGroupBox*			mVideoPreviewGroupBox;
	AJAPreviewWidget*	mVideoPreviewWidget;

	// End of QT UI member variables

	uint32_t			mBoardIndex;

	char*				mSingleIPAddr;

	AJAFileIO*			mAjaFileIO;
	IPlaycorder*		mPlaycorder;

	bool				mPreviewing;
	bool				mRecording;
	bool				mPlaying;
	AJACircularBuffer<IPlaycorder::PreviewFrameInfo *> *	mPreviewCircularBuffer;

	AJAThread*			mPreviewThread;
	bool				mGlobalQuit;
	void				StartPreviewThread ();
	static	void		PreviewThreadStatic (AJAThread * pThread, void * pContext);
	void				PreviewThread ();

	uint32_t			mClipLength;
};

#endif // NTV2QTPLAYCORDER_H

