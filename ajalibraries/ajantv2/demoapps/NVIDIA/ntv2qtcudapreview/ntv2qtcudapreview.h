//--------------------------------------------------------------------------------------------------
//    File:          NTV2QtCudaPreview.h
//
//    Description:   
//    Copyright:     Copyright 2012 AJA Video Systems, Inc. All rights reserved.
//--------------------------------------------------------------------------------------------------
#ifndef NTV2QtCudaPreview_H
#define NTV2QtCudaPreview_H

#include "gpustuff/utility/cudaPipeline.h"

#include <QDialog>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include "ajastuff/common/public.h"
#include "ajastuff/system/thread.h"
#include "ajastuff/system/systemtime.h"
#include "ajastuff/common/timer.h"
#include "ajastuff/system/process.h"

#include "ntv2cudacapture.h"
#include "gpustuff/utility/cudacaptureviewer.h"

#define AJAPREVIEW_WIDGET_X (960)
#define AJAPREVIEW_WIDGET_Y (540)

class NTV2QtCudaPreview : public QDialog
{
	Q_OBJECT

public:
    NTV2QtCudaPreview(QWidget *parent = 0);
	~NTV2QtCudaPreview();

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
	NTV2CudaCapture			*mFrameGrabber;
	CCudaCaptureViewer		*mVideoPreviewWidget;		
	CUcontext				mCudaCtx;

    //shared widget that acts instead of shared OGL context
    COglWidget				*mGLWidget;


	//the connection queues
	CudaPipelineEngine		*mPipelineEngine;	

	// Just used to Acquire Board
	int						mBoardNumber;
	CNTV2Card				mNTV2Card;
	NTV2EveryFrameTaskMode	mPreviousFrameServices;			/// We will restore the previous state


};


#endif // NTV2QtCudaPreview_H

