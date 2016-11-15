/**
	@file		ntv2qtmultiinput.h
	@brief		Header file for NTV2QtMultiInput demonstration application. Demonstrates how to
				capture several SDI AV streams from NTV2-based AJA devices (e.g., Kona3G in Quad
				mode, and Corvid24). Shows how to make use of the NTV2FrameGrabber class.
	@copyright	Copyright 2013 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef NTV2QTMULTIINPUT_H
#define NTV2QTMULTIINPUT_H


#include "ajatypes.h"
#include "ajapreviewwidget.h"
#include "ntv2card.h"
#include "ntv2devicescanner.h"
#include "ntv2framegrabber.h"
#include "ajabase/common/types.h"


const uint32_t kInvalidBoardIndexNumber	(9999);
const uint32_t kMaxNumInputs			(8);	



/**
	@brief	I am a QDialog that displays, in a 4-up (or 8-up) layout, whatever video is present on each
			SDI input of an AJA 4-input (or 8-input) device. I also have a popup menu for choosing a different
			capture device.
**/
class NTV2QtMultiInput : public QDialog
{
	Q_OBJECT

	//	Instance Methods
	public:
        NTV2QtMultiInput (QWidget * parent = 0, Qt::WindowFlags flags = 0);
		~NTV2QtMultiInput ();

	public slots:
		/**
			@brief	This gets called when any "audio" checkbox is clicked.
			@param	inputNum	Specifies which input's "with audio" checkbox was clicked.
		**/
		void	SlotAudioCheckBox (const int inputNum);

		/**
			@brief	This gets called when the user selects a different device from the device selector popup menu.
			@param	inDeviceIndexNum	Specifies which item in the device selector popup menu was chosen (0 is first item).
		**/
		void	RequestDeviceChange (const int inDeviceIndexNum);

		/**
			@brief	This gets called when any "CC" checkbox is clicked.
			@param	inputNum	Specifies which input's "CC" checkbox was clicked.
		**/
		void	SlotCaptionsCheckBox (const int inputNum);


	//	Instance Data
	private:
		QComboBox *			mDeviceChoicePopupMenu;					///< @brief	Device selector popup menu
		QGroupBox *			mPreviewGroupBoxes [kMaxNumInputs];		///< @brief	Four Qt group boxes
		QLabel *			mInputLabels [kMaxNumInputs];			///< @brief Label text for each input.
		QCheckBox *			mWithAudioCheckBoxes [kMaxNumInputs];	///< @brief	4 or 8 "with audio" check-boxes
		AJAPreviewWidget *	mPreviewWidgets [kMaxNumInputs];		///< @brief	4 or 8 AJAPreviewWidget objects
		NTV2FrameGrabber *	mFrameGrabbers [kMaxNumInputs];			///< @brief	4 or 8 NTV2FrameGrabber threads
		UWord				mDeviceInputCounts [99];				///< @brief	Number of SDI inputs per AJA device found
		#if defined (INCLUDE_AJACC)
			QCheckBox *		mWithCaptionsCheckBoxes [kMaxNumInputs];///< @brief	4 or 8 "captions" check-boxes
		#endif

};	//	NTV2QtMultiInput


#endif // NTV2QTMULTIINPUT_H
