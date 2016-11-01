//--------------------------------------------------------------------------------------------------
//    File:          NTV2QtCudaPreview.cpp
//
//    Copyright:     Copyright 2012 AJA Video Systems, Inc. All rights reserved.
//--------------------------------------------------------------------------------------------------
#include "ntv2qtcudapreview.h"
#include "ntv2boardscan.h"
#include "ntv2card.h"

#include <string>
#include <sstream>

#include <QLabel>
#include <QHBoxLayout>
#include <QMessageBox>


NTV2QtCudaPreview::NTV2QtCudaPreview(QWidget *parent)
    : QDialog(parent),
	  mFrameGrabber(NULL),
	  mVideoPreviewWidget(NULL),
	  mPipelineEngine(NULL),
	  mBoardNumber(0)
{
	
	resize(AJAPREVIEW_WIDGET_X, AJAPREVIEW_WIDGET_Y);
    //create a shared GL context, via a GLWidget, where all the OpenGL resources will be allocated.
    mGLWidget = new COglWidget();

	//create a CUDA context, where all CUDA resources will be allocated.

	CUCHK(cuInit(0));
	CUCHK(cuCtxCreate(&mCudaCtx,0,0));
}

NTV2QtCudaPreview::~NTV2QtCudaPreview()
{
	CUCHK(cuCtxDestroy(mCudaCtx));
    delete mGLWidget;

}

AJAStatus NTV2QtCudaPreview::Init()
{
	mPipelineEngine = new CudaPipelineEngine(mCudaCtx);
	//create the frame grabber
	mFrameGrabber = new NTV2CudaCapture(mBoardNumber, mPipelineEngine->GetGpuTransfer());		
	//create the preview widget with that shared OpenGL context
    mVideoPreviewWidget = new CCudaCaptureViewer(this, mGLWidget,mCudaCtx, mPipelineEngine->GetGpuTransfer());
	
	//connect the two via the GPU
	mPipelineEngine->ConnectViaGpuQueue(mFrameGrabber, mVideoPreviewWidget);	

	AddVideoPreviewGroupBox();
	AddDeviceEnumeratorGroupBox();
	UpdateBoardEnumerator();
	
	AddItemsToLayoutManager();
	mPipelineEngine->InitEngine();

    mGLWidget->doneCurrent();
    mGLWidget->context()->moveToThread(mVideoPreviewWidget);

	//start the engine(this call initializes all the GPU objects as well)
	mPipelineEngine->StartEngine();

	return AJA_STATUS_SUCCESS;
}

AJAStatus NTV2QtCudaPreview::Deinit()
{
	//stop the engine
	if( mPipelineEngine )
	{
  		mPipelineEngine->StopEngine();
		mPipelineEngine->DeinitEngine();
		delete mPipelineEngine;	
		mPipelineEngine = NULL;
	}
	if( mVideoPreviewWidget )
	{
		delete mVideoPreviewWidget;
		mVideoPreviewWidget = NULL;
	}
	if( mFrameGrabber )
	{
		delete mFrameGrabber;
		mFrameGrabber = NULL;
	}
	
	if ( mNTV2Card.BoardOpened())
	{
		mNTV2Card.SetEveryFrameServices(mPreviousFrameServices);
	}

	return AJA_STATUS_SUCCESS;
}

void NTV2QtCudaPreview::done( int r ) 
{
    QDialog::done( r );
    close();
}

void NTV2QtCudaPreview::closeEvent(QCloseEvent *evt)
{
	Deinit();

	QDialog::closeEvent( evt );
}

void NTV2QtCudaPreview::AddVideoPreviewGroupBox()
{
	mVideoPreviewGroupBox = new QGroupBox();

    mGLWidget->setFixedWidth(AJAPREVIEW_WIDGET_X);
    mGLWidget->setFixedHeight(AJAPREVIEW_WIDGET_Y);
    //mVideoPreviewWidget->setFixedWidth(AJAPREVIEW_WIDGET_X);
    //mVideoPreviewWidget->setFixedHeight(AJAPREVIEW_WIDGET_Y);
	
}
void NTV2QtCudaPreview::AddDeviceEnumeratorGroupBox()
{
	mDeviceEnumeratorGroupBox = new QGroupBox();

	QLabel* hostDeviceLabel = new QLabel(tr("Host/Board"),this); 
	mDeviceChoiceCombo = new QComboBox(this);

	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(hostDeviceLabel);
	layout->addWidget(mDeviceChoiceCombo,Qt::AlignLeft);
	mDeviceEnumeratorGroupBox->setLayout(layout);
}

bool NTV2QtCudaPreview::UpdateBoardEnumerator()
{
	// Although this shows how to enumerate boards the program only runs with 
	// the first board found.
	CNTV2BoardScan boardScan;
	if (boardScan.GetNumBoards () == 0)
	{
		mDeviceChoiceCombo->addItem (tr ("No Boards Found"));
		QMessageBox::warning(0,"OOPS", "No Boards Found");
		return false;

	}
	else
	{
		BoardInfoList & boardList	(boardScan.GetBoardList ());

		for (BoardInfoList::const_iterator iter = boardList.begin (); iter != boardList.end (); ++iter)
		{
			const OurBoardInfo	info	(*iter);
			mDeviceChoiceCombo->addItem (tr (info.boardIdentifier));
		}
	}

	// Just Open 1st Board to Reserve it
	OurBoardInfo boardInfo = boardScan.GetBoardList()[mBoardNumber];
	NTV2BoardType boardType = boardInfo.boardType;
	ULWord boardNumber = boardInfo.boardNumber;

	if (mNTV2Card.Open (boardNumber, false, boardType))
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

void NTV2QtCudaPreview::AddItemsToLayoutManager()
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


void NTV2QtCudaPreview::slotAudioCheckBox(int inputNum)
{
	(void) inputNum;
}

void NTV2QtCudaPreview::boardChanged(int index)
{
	Deinit();

	mBoardNumber = index;
}
