//--------------------------------------------------------------------------------------------------
//    File:          NTV2QtOGLPreview.cpp
//
//    Copyright:     Copyright 2012 AJA Video Systems, Inc. All rights reserved.
//--------------------------------------------------------------------------------------------------
#include "ntv2qtoglpreview.h"
#include "ntv2boardscan.h"
#include "ntv2card.h"

#include <string>
#include <sstream>

#include <QLabel>
#include <QHBoxLayout>
#include <QMessageBox>

NTV2QtOGLPreview::NTV2QtOGLPreview(QWidget *parent)
    : QDialog(parent),
	  mFrameGrabber(NULL),
	  mVideoPreview(NULL),
	  mGLWidget(NULL),
	  mPipelineEngine(NULL),
	  mBoardNumber(0)
{
	
	resize(AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y);
	//create a shared GL context, via a GLWidget, where all the OpenGL resources will be allocated.
    mGLWidget = new COglWidget();
	//mGLWidget->hide();

}


NTV2QtOGLPreview::~NTV2QtOGLPreview()
{

	delete mGLWidget;

}


AJAStatus NTV2QtOGLPreview::Init()
{
	mPipelineEngine = new OglPipelineEngine(mGLWidget);
	//create the frame grabber
	mFrameGrabber = new NTV2OglCapture(mBoardNumber, mPipelineEngine->GetGpuTransfer());		
	//create the preview widget with that shared OpenGL context
	mVideoPreview = new COglCaptureViewer(this, mGLWidget, mPipelineEngine->GetGpuTransfer());	
	
	//connect the two via the GPU
	mPipelineEngine->ConnectViaGpuQueue(mFrameGrabber, mVideoPreview);	

	AddVideoPreviewGroupBox();
	AddDeviceEnumeratorGroupBox();
	UpdateBoardEnumerator();
	
	AddItemsToLayoutManager();
	mPipelineEngine->InitEngine();	
    
    mGLWidget->doneCurrent();
    mGLWidget->context()->moveToThread(mVideoPreview);

    //start the engine(this call initializes all the GPU objects as well)
    mPipelineEngine->StartEngine();

	return AJA_STATUS_SUCCESS;
}


AJAStatus NTV2QtOGLPreview::Deinit()
{
	//stop the engine
	if( mPipelineEngine )
	{
  		mPipelineEngine->StopEngine();
		mPipelineEngine->DeinitEngine();
		delete mPipelineEngine;
		mPipelineEngine = NULL;
	}

	delete mVideoPreview;
	delete mFrameGrabber;	
	
	if ( mNTV2Card.BoardOpened())
	{
		mNTV2Card.SetEveryFrameServices(mPreviousFrameServices);
	}

	return AJA_STATUS_SUCCESS;
}


void NTV2QtOGLPreview::done( int r )
{
    QDialog::done( r );
    close();
}

void NTV2QtOGLPreview::closeEvent(QCloseEvent *evt)
{
	Deinit();

	QDialog::closeEvent( evt );
}


void NTV2QtOGLPreview::AddVideoPreviewGroupBox()
{
	mVideoPreviewGroupBox = new QGroupBox();

	mGLWidget->setFixedWidth(AJAPREVIEW_WIDGET_X);
	mGLWidget->setFixedHeight(AJAPREVIEW_WIDGET_Y);
	
}


void NTV2QtOGLPreview::AddDeviceEnumeratorGroupBox()
{
	mDeviceEnumeratorGroupBox = new QGroupBox();

	QLabel* hostDeviceLabel = new QLabel(tr("Host/Board"),this);
	mDeviceChoiceCombo = new QComboBox(this);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(hostDeviceLabel);
	layout->addWidget(mDeviceChoiceCombo,Qt::AlignLeft);
	mDeviceEnumeratorGroupBox->setLayout(layout);
}


bool NTV2QtOGLPreview::UpdateBoardEnumerator()
{
	// Enumerate the boards in the system
	CNTV2DeviceScanner boardScan;
	if (boardScan.GetNumDevices () == 0)
	{
		mDeviceChoiceCombo->addItem (tr ("No Boards Found"));
		QMessageBox::warning(0,"OOPS", "No Boards Found");
		return false;

	}
	else
	{
		NTV2DeviceInfoList & boardList	(boardScan.GetDeviceInfoList ());

		for (NTV2DeviceInfoList::const_iterator iter = boardList.begin (); iter != boardList.end (); ++iter)
		{
			const NTV2DeviceInfo	info	(*iter);
			mDeviceChoiceCombo->addItem (tr (info.deviceIdentifier.c_str ()));
		}
	}

	// Open the requested board to Reserve it
	NTV2DeviceInfo boardInfo = boardScan.GetDeviceInfoList()[mBoardNumber];
	ULWord boardNumber = boardInfo.deviceIndex;

	if (mNTV2Card.Open (boardNumber))
	{
		if (!mNTV2Card.AcquireStreamForApplicationWithReference(AJA_FOURCC('D','E','M','O'), (uint32_t)AJAProcess::GetPid()))
		{
			//We have not acquired the board continue until something changes
			QMessageBox::warning(0,"OOPS", "Can't Acquire Board");
			mNTV2Card.Close();
			return false;
		}
		else
		{
			//Save the current state before we change it
			mNTV2Card.GetEveryFrameServices(&mPreviousFrameServices);
			//Since this is an OEM demo we will set the OEM service level
			mNTV2Card.SetEveryFrameServices(NTV2_OEM_TASKS);
		}
	}
	else
	{
		QMessageBox::warning(0,"OOPS", "Can't Open Board");
		return false;

	}

	mDeviceChoiceCombo->setCurrentIndex (mBoardNumber);

	return true;

}


void NTV2QtOGLPreview::AddItemsToLayoutManager()
{
	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(mGLWidget);
	mVideoPreviewGroupBox->setLayout(layout);

	QVBoxLayout *mainLayout = new QVBoxLayout;


	mainLayout->addWidget(mDeviceEnumeratorGroupBox);
	mainLayout->addWidget(mGLWidget);
	mainLayout->addWidget(mVideoPreviewGroupBox);
	setLayout(mainLayout);
}


void NTV2QtOGLPreview::slotAudioCheckBox(int inputNum)
{
	(void) inputNum;
}


void NTV2QtOGLPreview::boardChanged(int index)
{
	Deinit();

	mBoardNumber = index;
}
