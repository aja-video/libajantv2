//--------------------------------------------------------------------------------------------------
//    File:          ntv2qtoglpreview.h
//
//    Description:   
//    Copyright:     Copyright 2013 AJA Video Systems, Inc. All rights reserved.
//--------------------------------------------------------------------------------------------------
#ifndef NTV2QOGLPreview_H
#define NTV2QOGLPreview_H

#include "gpustuff/utility/oglPipeline.h"


#include <QDialog>
#include <QGroupBox>
#include <QComboBox>
#include "ajastuff/common/public.h"
#include "ajastuff/system/thread.h"
#include "ajastuff/system/systemtime.h"
#include "ajastuff/common/timer.h"
#include "ajastuff/system/process.h"

#include "ntv2oglcapture.h"
#include "gpustuff/utility/oglcaptureviewer.h"

#define AJAPREVIEW_WIDGET_X (960)
#define AJAPREVIEW_WIDGET_Y (540)

class NTV2QtOGLPreview : public QDialog
{
	Q_OBJECT

public:
    NTV2QtOGLPreview(QWidget *parent = 0);
	~NTV2QtOGLPreview();

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
	COglCaptureViewer		*mVideoPreview;

	//shared widget that acts instead of shared OGL context
	COglWidget				*mGLWidget;		
	
	//the connection queues
	OglPipelineEngine		*mPipelineEngine;

	// Just used to Acquire Board
	int						mBoardNumber;
	CNTV2Card				mNTV2Card;
	NTV2EveryFrameTaskMode	mPreviousFrameServices;			/// We will restore the previous state


};

#endif // NTV2QtOGLPreview_H

