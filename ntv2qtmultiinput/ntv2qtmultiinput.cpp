/**
	@file		ntv2qtmultiinput.cpp
	@brief		Implementation of the NTV2QtMultiInput class.
	@copyright	Copyright 2013 AJA Video Systems, Inc. All rights reserved.
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
	ULWord				initialBoardIndex	(0);
	CNTV2DeviceScanner	deviceScanner;
	const ULWord		numDevicesFound		(deviceScanner.GetNumDevices ());

	::memset (mDeviceInputCounts, 0, sizeof (mDeviceInputCounts));
	::memset (mFrameGrabbers, 0, sizeof (mFrameGrabbers));

	//
	//	Build the device selector popup menu...
	//
	mDeviceChoicePopupMenu = new QComboBox;
	if (numDevicesFound == 0)
		mDeviceChoicePopupMenu->addItem (tr ("No Devices Found"));
	else
	{
		NTV2DeviceInfoList & boardList	(deviceScanner.GetDeviceInfoList ());
		for (NTV2DeviceInfoListConstIter iter (boardList.begin ());  iter != boardList.end ();  iter++)
		{
			mDeviceInputCounts [iter->deviceIndex] = ::NTV2DeviceGetNumVideoInputs (iter->deviceID);
			mDeviceChoicePopupMenu->addItem (tr (iter->deviceIdentifier.c_str ()));
		}	//	 for each AJA device found
	}	//	else at least one AJA device found

	//
	//	Default to the first quad-input device (if any)...
	//
	uint32_t numInputs = 4;

	for (ULWord ndx (0);  ndx < numDevicesFound;  ndx++)
		if (mDeviceInputCounts [ndx] == 8)
			numInputs = 8;	//	NOTE: hardcode for now
	initialBoardIndex = 0;
	QObject::connect (mDeviceChoicePopupMenu, SIGNAL (currentIndexChanged (int)), this, SLOT (RequestDeviceChange (const int)));
	mDeviceChoicePopupMenu->setCurrentIndex (initialBoardIndex);

	//
	//	Map all "with audio" checkbox clicks to a single handler...
	//
	QSignalMapper * signalMapper = new QSignalMapper (this);
	connect (signalMapper, SIGNAL (mapped (int)), this, SLOT (SlotAudioCheckBox (int)));

	//
	//	Set up each of the preview widgets and frame grabber threads...
	//
	for (uint32_t inputNumber = 0;  inputNumber < numInputs;  inputNumber++)
	{
		mPreviewGroupBoxes [inputNumber] = new QGroupBox (this);

		mPreviewWidgets [inputNumber] = new AJAPreviewWidget (this);
		mPreviewWidgets [inputNumber]->setMinimumWidth  (AJAPREVIEW_WIDGET_X / (numInputs/2));
		mPreviewWidgets [inputNumber]->setMinimumHeight  (AJAPREVIEW_WIDGET_Y / (numInputs/2));
		mPreviewWidgets [inputNumber]->setSizePolicy(QSizePolicy ::Expanding, QSizePolicy ::Expanding);
 
		mFrameGrabbers [inputNumber] = new NTV2FrameGrabber (this);
		mInputLabels [inputNumber] = new QLabel("",this);
		mWithAudioCheckBoxes [inputNumber] = new QCheckBox ("Audio", this);
		signalMapper->setMapping (mWithAudioCheckBoxes [inputNumber], inputNumber);
		#if defined (INCLUDE_AJACC)
			mWithCaptionsCheckBoxes [inputNumber] = new QCheckBox ("CC", this);
		#endif

		QVBoxLayout * layout = new QVBoxLayout;
		layout->addWidget (mPreviewWidgets [inputNumber]);
		layout->addWidget (mInputLabels [inputNumber]);

		QHBoxLayout *	bottomLayout		(new QHBoxLayout);
		bottomLayout->setContentsMargins (0, 0, 0, 0);
		bottomLayout->addWidget (mWithAudioCheckBoxes [inputNumber]);
		#if defined (INCLUDE_AJACC)
			bottomLayout->addWidget (mWithCaptionsCheckBoxes [inputNumber]);
		#endif
		layout->addLayout (bottomLayout);

		mPreviewGroupBoxes [inputNumber]->setLayout (layout);

		connect (mFrameGrabbers [inputNumber], SIGNAL (newFrame (const QImage &, bool)),
					mPreviewWidgets [inputNumber], SLOT (updateFrame (const QImage &, bool)));

		connect (mFrameGrabbers [inputNumber], SIGNAL (newStatusString (const QString)),
			mInputLabels [inputNumber], SLOT (setText (const QString)));

		connect (mWithAudioCheckBoxes [inputNumber], SIGNAL (clicked ()), signalMapper, SLOT (map ()));
		#if defined (INCLUDE_AJACC)
			//connect (mWithCaptionsCheckBoxes [inputNumber], SIGNAL (clicked ()), signalMapper, SLOT (map ()));
		#endif
	}	//	for each video input

	//
	//	Lay out everything...
	//
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


	//
	//	Set up the frame grabber threads...
	//
	for (uint32_t inputNumber (0);  inputNumber < numInputs;  inputNumber++)
	{
		mFrameGrabbers [inputNumber]->SetInputSource (gInputNumberToInputSource [inputNumber]);
		mFrameGrabbers [inputNumber]->SetWithAudio (false);
		mFrameGrabbers [inputNumber]->start ();
	}

	RequestDeviceChange (initialBoardIndex);

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
