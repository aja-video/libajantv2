//--------------------------------------------------------------------------------------------------
//    File:          NTV2QtOglCaptureAndProcess.h
//
//    Description:   
//    Copyright:     Copyright 2012 AJA Video Systems, Inc. All rights reserved.
//--------------------------------------------------------------------------------------------------
#ifndef NTV2QtOglCaptureAndProcess_H
#define NTV2QtOglCaptureAndProcess_H

#include "gpustuff/utility/oglPipeline.h"

#include <QDialog>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include "ajabase/common/public.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/common/timer.h"
#include "ajabase/system/process.h"

#include "ntv2oglcapture.h"
#include "ntv2ogloutput.h"
#include "gpustuff/utility/oglpassthruviewer.h"

#define AJAPREVIEW_WIDGET_X (960)
#define AJAPREVIEW_WIDGET_Y (540)

class NTV2QtOglCaptureAndProcess : public QDialog
{
	Q_OBJECT

public:
    NTV2QtOglCaptureAndProcess(QWidget *parent = 0);
	~NTV2QtOglCaptureAndProcess();

	AJAStatus	Init();
	AJAStatus	Deinit();

public slots:
	void slotAudioCheckBox(int inputNum);
	void boardChanged(int index);	

protected:
	void AddVideoPreviewGroupBox();
	void AddDeviceEnumeratorGroupBox();
	bool UpdateBoardEnumerator();
	
	void AddItemsToLayoutManager();
    void closeEvent(QCloseEvent *evt);

    void done( int r );
 
private:
	QGroupBox*		mDeviceEnumeratorGroupBox;
	QComboBox*		mDeviceChoiceCombo;

	QGroupBox*		mVideoPreviewGroupBox;


	//the processors
	NTV2OglCapture			*mFrameGrabber;
	NTV2OglOutput			*mFrameOutput;
	COglPassthruViewer		*mVideoPreviewWidget;

    //shared widget that acts instead of shared OGL context
    COglWidget				*mGLWidget;


	//the connection queues
	OglPipelineEngine		*mPipelineEngine;

	// Just used to Acquire Board
	int						mBoardNumber;
	CNTV2Card				mNTV2Card;
	NTV2EveryFrameTaskMode	mPreviousFrameServices;			/// We will restore the previous state
		
};

#endif // NTV2QtOglCaptureAndProcess_H

