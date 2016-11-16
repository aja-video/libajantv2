//--------------------------------------------------------------------------------------------------
//    File:          ntv2qtoglcaptureandprocess.cpp
//
//    Copyright:     Copyright 2012 AJA Video Systems, Inc. All rights reserved.
//--------------------------------------------------------------------------------------------------
#include "ntv2qtoglcaptureandprocess.h"
#include "ntv2boardscan.h"
#include "ntv2card.h"

#include <QHBoxLayout>
#include <QMessageBox>

#include <string>
#include <sstream>


NTV2QtOglCaptureAndProcess::NTV2QtOglCaptureAndProcess(QWidget *parent)
    : QDialog(parent),
      mFrameGrabber			(NULL),
	  mFrameOutput			(NULL),
	  mVideoPreviewWidget	(NULL),
	  mGLWidget				(NULL),
	  mPipelineEngine		(NULL),
	  mBoardNumber			(0)
{

	resize(AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y);
	//create a shared GL context, via a GLWidget, where all the OpenGL resources will be allocated.
    mGLWidget = new COglWidget(this);
    //mGLWidget->hide();

}


NTV2QtOglCaptureAndProcess::~NTV2QtOglCaptureAndProcess()
{

	delete mGLWidget;

}


AJAStatus NTV2QtOglCaptureAndProcess::Init()
{
	mPipelineEngine = new OglPipelineEngine(mGLWidget);

	//create the frame grabber
	mFrameGrabber = new NTV2OglCapture(mBoardNumber,  mPipelineEngine->GetGpuTransfer());
	//create the frame player
	mFrameOutput = new NTV2OglOutput(mBoardNumber, 8, mPipelineEngine->GetGpuTransfer());
	//create the preview widget with that shared OpenGL context
	mVideoPreviewWidget = new COglPassthruViewer(this, mGLWidget, mPipelineEngine->GetGpuTransfer());

	//connect the processors via GPU queue
	mPipelineEngine->ConnectViaGpuQueue(mFrameGrabber, mVideoPreviewWidget);
	mPipelineEngine->ConnectViaGpuQueue(mVideoPreviewWidget, mFrameOutput);

	AddVideoPreviewGroupBox();
	AddDeviceEnumeratorGroupBox();
	UpdateBoardEnumerator();

	AddItemsToLayoutManager();

	//start the engine
	mPipelineEngine->InitEngine();

    mGLWidget->doneCurrent();
    mGLWidget->context()->moveToThread(mVideoPreviewWidget);


	mPipelineEngine->StartEngine();

	return AJA_STATUS_SUCCESS;
}


AJAStatus NTV2QtOglCaptureAndProcess::Deinit()
{
	//stop the engine
    if( mPipelineEngine )
	{
	  	mPipelineEngine->StopEngine();
		mPipelineEngine->DeinitEngine();
		delete mPipelineEngine;
		mPipelineEngine = NULL;
	}

	delete mVideoPreviewWidget;
	delete mFrameGrabber;
	delete mFrameOutput;

	if ( mNTV2Card.IsOpen() )
	{
		mNTV2Card.SetEveryFrameServices(mPreviousFrameServices);
	}

	return AJA_STATUS_SUCCESS;
}


void NTV2QtOglCaptureAndProcess::done( int r )
{
    QDialog::done( r );
    close();
}


void NTV2QtOglCaptureAndProcess::closeEvent(QCloseEvent *evt)
{
	Deinit();

	QDialog::closeEvent( evt );
}


void NTV2QtOglCaptureAndProcess::AddVideoPreviewGroupBox()
{
	mVideoPreviewGroupBox = new QGroupBox();

    mGLWidget->setFixedWidth(AJAPREVIEW_WIDGET_X);
    mGLWidget->setFixedHeight(AJAPREVIEW_WIDGET_Y);

}


void NTV2QtOglCaptureAndProcess::AddDeviceEnumeratorGroupBox()
{
	mDeviceEnumeratorGroupBox = new QGroupBox();

	QLabel* hostDeviceLabel = new QLabel(tr("Host/Board"),this);
	mDeviceChoiceCombo = new QComboBox(this);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(hostDeviceLabel);
	layout->addWidget(mDeviceChoiceCombo,Qt::AlignLeft);
	mDeviceEnumeratorGroupBox->setLayout(layout);
}


bool NTV2QtOglCaptureAndProcess::UpdateBoardEnumerator()
{
	// Although this shows how to enumerate boards the program only runs with
	// the first board found.
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

	// Open requested board to reserve it
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


void NTV2QtOglCaptureAndProcess::AddItemsToLayoutManager()
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


void NTV2QtOglCaptureAndProcess::slotAudioCheckBox(int inputNum)
{
	(void) inputNum;
}


void NTV2QtOglCaptureAndProcess::boardChanged(int index)
{
	Deinit();

	mBoardNumber = index;
}
