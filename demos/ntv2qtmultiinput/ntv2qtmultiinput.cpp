/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2qtmultiinput.cpp
	@brief		Implementation of the NTV2QtMultiInput class.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include "ntv2qtmultiinput.h"
#include "ntv2devicefeatures.h"

//
//	This array maps SDI input numbers 0 thru 3 to an NTV2InputSource...
//
static const NTV2InputSource	gInputNumberToInputSource [] =
{
	NTV2_INPUTSOURCE_SDI1,
	NTV2_INPUTSOURCE_SDI2,
	NTV2_INPUTSOURCE_SDI3,
	NTV2_INPUTSOURCE_SDI4,
	NTV2_INPUTSOURCE_SDI5,
	NTV2_INPUTSOURCE_SDI6,
	NTV2_INPUTSOURCE_SDI7,
	NTV2_INPUTSOURCE_SDI8,

};


NTV2QtMultiInput::NTV2QtMultiInput (QWidget * parent, Qt::WindowFlags flags)
	:	QDialog			(parent, flags)
{
	::memset (mDeviceInputCounts, 0, sizeof(mDeviceInputCounts));
	::memset (mFrameGrabbers, 0, sizeof(mFrameGrabbers));

	//	Build the device selector popup menu...
	CNTV2Card card;
	mDeviceChoicePopupMenu = new QComboBox;
	for (ULWord ndx(0);  CNTV2DeviceScanner::GetDeviceAtIndex(ndx, card);  ndx++)
	{
		mDeviceInputCounts[ndx] = card.features().GetNumVideoInputs();
		mDeviceChoicePopupMenu->addItem(card.GetDisplayName().c_str());
	}	//	 for each AJA device found
	if (!mDeviceChoicePopupMenu->count())
		mDeviceChoicePopupMenu->addItem("No Devices Found");

	//	Default to the first quad-input device (if any)...
	uint32_t numInputs(4);

	for (ULWord ndx(0);  ndx < ULWord(mDeviceChoicePopupMenu->count());  ndx++)
		if (mDeviceInputCounts[ndx] == 8)
			numInputs = 8;	//	NOTE: hardcode for now
	QObject::connect (mDeviceChoicePopupMenu, SIGNAL(currentIndexChanged(int)),
						this, SLOT(RequestDeviceChange(const int)));
	mDeviceChoicePopupMenu->setCurrentIndex(0);

	//	Set up each of the preview widgets and frame grabber threads...
	QSignalMapper * signalMapper = new QSignalMapper(this);
	for (uint32_t inputNumber(0);  inputNumber < numInputs;  inputNumber++)
	{
		mPreviewGroupBoxes[inputNumber]   = new QGroupBox(this);
		mPreviewWidgets[inputNumber]      = new AJAPreviewWidget(this);
		mPreviewWidgets[inputNumber]->setMinimumWidth (AJAPREVIEW_WIDGET_X / (numInputs/2));
		mPreviewWidgets[inputNumber]->setMinimumHeight (AJAPREVIEW_WIDGET_Y / (numInputs/2));
		mPreviewWidgets[inputNumber]->setSizePolicy (QSizePolicy ::Expanding, QSizePolicy ::Expanding);
 		mFrameGrabbers[inputNumber]       = new NTV2FrameGrabber(this);
		mInputLabels[inputNumber]         = new QLabel("", this);
		mWithAudioCheckBoxes[inputNumber] = new QCheckBox("Audio", this);
		signalMapper->setMapping (mWithAudioCheckBoxes [inputNumber], inputNumber);
		#if defined (INCLUDE_AJACC)
			mWithCaptionsCheckBoxes [inputNumber] = new QCheckBox ("CC", this);
		#endif

		QVBoxLayout * layout = new QVBoxLayout;
		layout->addWidget (mPreviewWidgets [inputNumber]);
		layout->addWidget (mInputLabels [inputNumber]);

		QHBoxLayout *	bottomLayout (new QHBoxLayout);
		bottomLayout->setContentsMargins (0, 0, 0, 0);
		bottomLayout->addWidget (mWithAudioCheckBoxes [inputNumber]);
		#if defined (INCLUDE_AJACC)
			bottomLayout->addWidget (mWithCaptionsCheckBoxes [inputNumber]);
		#endif
		layout->addLayout (bottomLayout);

		mPreviewGroupBoxes[inputNumber]->setLayout (layout);

		connect (mFrameGrabbers[inputNumber], SIGNAL(newFrame(const QImage&, bool)),
				mPreviewWidgets[inputNumber], SLOT(updateFrame(const QImage&, bool)));

		connect (mFrameGrabbers[inputNumber], SIGNAL(newStatusString(const QString)),
				mInputLabels[inputNumber], SLOT(setText(const QString)));

		connect (mWithAudioCheckBoxes [inputNumber], SIGNAL(clicked()),
				signalMapper, SLOT(map()));
		#if defined (INCLUDE_AJACC)
			//connect (mWithCaptionsCheckBoxes [inputNumber], SIGNAL (clicked ()), signalMapper, SLOT (map ()));
		#endif
	}	//	for each video input

	//	Map all "with audio" checkbox clicks to a single handler...
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect (signalMapper, SIGNAL (mappedInt (int)), this, SLOT (SlotAudioCheckBox (int)));
#else
    connect (signalMapper, SIGNAL (mapped (int)), this, SLOT (SlotAudioCheckBox (int)));
#endif

	//	Lay out everything...
	QGridLayout * glayout = new QGridLayout;
	if (numInputs == 4)
	{
		glayout->addWidget (mPreviewGroupBoxes [0], 0, 0);
		glayout->addWidget (mPreviewGroupBoxes [1], 0, 1);
		glayout->addWidget (mPreviewGroupBoxes [2], 1, 0);
		glayout->addWidget (mPreviewGroupBoxes [3], 1, 1);
	}
	else
	{
		glayout->addWidget (mPreviewGroupBoxes [0], 0, 0);
		glayout->addWidget (mPreviewGroupBoxes [1], 0, 1);
		glayout->addWidget (mPreviewGroupBoxes [2], 0, 2);
		glayout->addWidget (mPreviewGroupBoxes [3], 0, 3);
		glayout->addWidget (mPreviewGroupBoxes [4], 1, 0);
		glayout->addWidget (mPreviewGroupBoxes [5], 1, 1);
		glayout->addWidget (mPreviewGroupBoxes [6], 1, 2);
		glayout->addWidget (mPreviewGroupBoxes [7], 1, 3);
	}

	QVBoxLayout * vlayout = new QVBoxLayout;
	vlayout->addWidget (mDeviceChoicePopupMenu);
	vlayout->addLayout (glayout);

	setLayout (vlayout);

	//	Set up the frame grabber threads...
	for (uint32_t inputNumber (0);  inputNumber < numInputs;  inputNumber++)
	{
		mFrameGrabbers [inputNumber]->SetInputSource (gInputNumberToInputSource [inputNumber]);
		mFrameGrabbers [inputNumber]->SetWithAudio (false);
		mFrameGrabbers [inputNumber]->start ();
	}

	RequestDeviceChange(0);

}	//	constructor


NTV2QtMultiInput::~NTV2QtMultiInput ()
{
	qDebug ("## NOTE:  Leaving");
}


void NTV2QtMultiInput::SlotAudioCheckBox (const int inputNum)
{
	mFrameGrabbers [inputNum]->SetWithAudio (mWithAudioCheckBoxes [inputNum]->isChecked ());
}


void NTV2QtMultiInput::SlotCaptionsCheckBox (const int inputNum)
{
	(void) inputNum;
	//mFrameGrabbers [inputNum]->changeCaptionChannel (mWithAudioCheckBoxes [inputNum]->isChecked () ? NTV2_CC608_CC1 : NTV2_CC608_ChannelInvalid);
}


void NTV2QtMultiInput::RequestDeviceChange (const int inDeviceIndexNum)
{
	//	Get the board number and type...
	CNTV2DeviceScanner	scanner;
	if (scanner.GetNumDevices ())
	{
		NTV2DeviceInfo	deviceInfo	(scanner.GetDeviceInfoList () [inDeviceIndexNum]);

		//	Notify each frame grabber to change devices...
		for (uint32_t inputNumber (0);  inputNumber < mDeviceInputCounts [inDeviceIndexNum];  inputNumber++)
			if (mFrameGrabbers [inputNumber])
				mFrameGrabbers [inputNumber]->SetDeviceIndex (deviceInfo.deviceIndex);
		qDebug ("## NOTE:  Device changed to %d", inDeviceIndexNum);
	}

}	//	RequestDeviceChange
