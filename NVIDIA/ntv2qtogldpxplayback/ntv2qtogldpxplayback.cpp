//--------------------------------------------------------------------------------------------------
//    File:          NTV2QtOglDPXPlayback.cpp
//
//    Copyright:     Copyright 2012 AJA Video Systems, Inc. All rights reserved.
//--------------------------------------------------------------------------------------------------
#include "ntv2qtogldpxplayback.h"
#include "ntv2boardscan.h"
#include "ntv2card.h"

#include <string>
#include <sstream>


NTV2QtOglDPXPlayback::NTV2QtOglDPXPlayback(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags),mFrameGrabber(NULL),mVideoPreviewWidget(NULL)
{
	
	resize(AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y);
	//create a shared GL context, via a GLWidget, where all the OpenGL resources will be allocated.
	mGLWidget = new QGLWidget(this);
	mGLWidget->hide();
	//create an OpenGL transfer that will be used to transfer buffers to and from the GPU
	mGpuTransfer = CreateOglTransfer();
	
	//create the frame grabber
	mFrameGrabber = new NTV2OglDPXReader(0, mGLWidget, mGpuTransfer);	
	//create the frame player
	mFrameOutput = new NTV2OglOutput(0, mGLWidget, mGpuTransfer);	
	//create the preview widget with that shared OpenGL context
	mVideoPreviewWidget = new COglPassthruViewer(this, mGLWidget, mGpuTransfer);	
	
	//connect the processors via GPU queue
	mVideoProcessingEngine.ConnectViaGpuQueue(mFrameGrabber, mVideoPreviewWidget);	
	mVideoProcessingEngine.ConnectViaGpuQueue(mVideoPreviewWidget, mFrameOutput);	

	AddVideoPreviewGroupBox();
	AddDeviceEnumeratorGroupBox();
	UpdateBoardEnumerator();
	
	AddItemsToLayoutManager();
	//Initialize the transfer object. This needs a current context
	mGLWidget->makeCurrent();
	mGpuTransfer->Init();
	mGLWidget->doneCurrent();
	//start the engine(this call initializes all the GPU objects as well)
	mVideoProcessingEngine.StartEngine();
}

void NTV2QtOglDPXPlayback::done( int r ) 
{
    QDialog::done( r );
    close();
}

void NTV2QtOglDPXPlayback::closeEvent(QCloseEvent *evt)
{

	//stop the engine
  	mVideoProcessingEngine.StopEngine();		
				
	delete mVideoPreviewWidget;
	delete mFrameGrabber;	
	delete mFrameOutput;
	//Destroy the transfer object. This needs the context current
	mGLWidget->makeCurrent();
	mGpuTransfer->Destroy();	
	mGLWidget->doneCurrent();
	delete mGpuTransfer;
	delete mGLWidget;	
	QDialog::closeEvent( evt );
}

NTV2QtOglDPXPlayback::~NTV2QtOglDPXPlayback()
{

}
void NTV2QtOglDPXPlayback::AddVideoPreviewGroupBox()
{
	mVideoPreviewGroupBox = new QGroupBox();

	mVideoPreviewWidget->setFixedWidth(AJAPREVIEW_WIDGET_X);
	mVideoPreviewWidget->setFixedHeight(AJAPREVIEW_WIDGET_Y);
	
}
void NTV2QtOglDPXPlayback::AddDeviceEnumeratorGroupBox()
{
	mDeviceEnumeratorGroupBox = new QGroupBox();

	QLabel* hostDeviceLabel = new QLabel(tr("Host/Board"),this); 
	mDeviceChoiceCombo = new QComboBox(this);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(hostDeviceLabel);
	layout->addWidget(mDeviceChoiceCombo,Qt::AlignLeft);
	mDeviceEnumeratorGroupBox->setLayout(layout);
}

void NTV2QtOglDPXPlayback::UpdateBoardEnumerator()
{
	CNTV2BoardScan boardScan;
	if (boardScan.GetNumBoards () == 0)
		mDeviceChoiceCombo->addItem (tr ("No Boards Found"));
	else
	{
		BoardInfoList & boardList	(boardScan.GetBoardList ());

		for (BoardInfoList::const_iterator iter = boardList.begin (); iter != boardList.end (); ++iter)
		{
			const OurBoardInfo	info	(*iter);
			mDeviceChoiceCombo->addItem (tr (info.boardIdentifier));
		}
	}
	mDeviceChoiceCombo->setCurrentIndex (0);

}


void NTV2QtOglDPXPlayback::AddItemsToLayoutManager()
{
	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(mVideoPreviewWidget);
	mVideoPreviewGroupBox->setLayout(layout);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	
	
	mainLayout->addWidget(mDeviceEnumeratorGroupBox);
	mainLayout->addWidget(mVideoPreviewWidget);
	mainLayout->addWidget(mVideoPreviewGroupBox);
	setLayout(mainLayout);
}


void NTV2QtOglDPXPlayback::slotAudioCheckBox(int inputNum)
{

}

void NTV2QtOglDPXPlayback::boardChanged(int index)
{

}