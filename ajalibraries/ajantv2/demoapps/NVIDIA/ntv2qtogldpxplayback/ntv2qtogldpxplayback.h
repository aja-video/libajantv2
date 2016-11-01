//--------------------------------------------------------------------------------------------------
//    File:          NTV2QtOglDPXPlayback.h
//
//    Description:   
//    Copyright:     Copyright 2012 AJA Video Systems, Inc. All rights reserved.
//--------------------------------------------------------------------------------------------------
#ifndef NTV2QtOglDPXPlayback_H
#define NTV2QtOglDPXPlayback_H

#include <QtGui/QDialog>
#include "ajastuff/common/public.h"
#include "ajastuff/system/thread.h"
#include "ajastuff/system/systemtime.h"
#include "ajastuff/common/timer.h"

#include "ntv2ogldpxreader.h"
#include "ntv2ogloutput.h"

#include "gpustuff/utility/oglpassthruviewer.h"
#include "gpustuff/utility/videoprocessingengine.h"
#define AJAPREVIEW_WIDGET_X (960)
#define AJAPREVIEW_WIDGET_Y (540)

class NTV2QtOglDPXPlayback : public QDialog
{
	Q_OBJECT

public:
	NTV2QtOglDPXPlayback(QWidget *parent = 0, Qt::WFlags flags = 0);
	~NTV2QtOglDPXPlayback();

public slots:
	void slotAudioCheckBox(int inputNum);
	void boardChanged(int index);	

protected:
	void AddVideoPreviewGroupBox();
	void AddDeviceEnumeratorGroupBox();
	void UpdateBoardEnumerator();
	
	void AddItemsToLayoutManager();
    void closeEvent(QCloseEvent *evt);

    void done( int r );
 
private:
	QGroupBox*		mDeviceEnumeratorGroupBox;
	QComboBox*		mDeviceChoiceCombo;

	QGroupBox*		  mVideoPreviewGroupBox;


	//the processors
	NTV2OglDPXReader		*mFrameGrabber;
	NTV2OglOutput			*mFrameOutput;
	COglPassthruViewer		*mVideoPreviewWidget;
	QGLWidget				*mGLWidget;		
	IOglTransfer			*mGpuTransfer;
	//the connection queues
	CVideoProcessingEngine<COglObject> mVideoProcessingEngine;
		
};

#endif // NTV2QtOglDPXPlayback_H

