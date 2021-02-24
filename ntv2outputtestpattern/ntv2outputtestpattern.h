/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2outputtestpattern.cpp
	@brief		Header file for NTV2OutputTestPattern demonstration class
	@copyright	(C) 2013-2021 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2OUTPUT_TEST_PATTERN_H
#define _NTV2OUTPUT_TEST_PATTERN_H

#include "ntv2card.h"


/**
	@brief	I generate and transfer a test pattern into an AJA device's frame buffer for steady-state
			playout using NTV2TestPatternGen::DrawTestPattern and CNTV2Card::DMAWriteFrame.
**/

class NTV2OutputTestPattern
{
	//	Public Instance Methods
	public:

		/**
			@brief		Constructs me using the given configuration settings.
			@note		I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use.
											Defaults to "0", the first device found.
			@param[in]	inTestPatternSpec	Optionally specifies the test pattern (or flat-field color) to use.
											If empty (the default), uses 100% bars.
			@param[in]	inVideoFormat		If not NTV2_FORMAT_UNKNOWN, specifies the video format to use.
											If NTV2_FORMAT_UNKNOWN (the default), uses the FrameStore's current
											video format.
			@param[in]	inPixelFormat		Optionally specifies the pixel format to use. Defaults to 8-bit YCBCr.
			@param[in]	inChannel			Optionally specifies which FrameStore to use, a zero-based index number.
											Defaults to NTV2_CHANNEL1.
			@param[in]	inVancMode			Optionally specifies the ::NTV2VANCMode to use. Defaults to NTV2_VANCMODE_OFF.
		**/
		NTV2OutputTestPattern (	const std::string &		inDeviceSpecifier	= "0",
								const std::string &		inTestPatternSpec	= "",
								const NTV2VideoFormat	inVideoFormat		= NTV2_FORMAT_UNKNOWN,
								const NTV2PixelFormat	inPixelFormat		= NTV2_FBF_8BIT_YCBCR,
								const NTV2Channel		inChannel			= NTV2_CHANNEL1,
								const NTV2VANCMode		inVancMode			= NTV2_VANCMODE_OFF);

		~NTV2OutputTestPattern (void);

		/**
			@brief		Initializes me and prepares me to Run.
			@return		AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
		**/
		AJAStatus		Init (void);

		/**
			@brief		Generates, transfers and displays the test pattern on the output.
			@return		AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
			@note		Do not call this method without first calling my Init method.
		**/
		AJAStatus		EmitPattern (void);


	//	Protected Instance Methods
	protected:
		/**
			@brief		Sets up my AJA device to play video.
			@return		AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
		**/
		AJAStatus		SetUpVideo (void);

		/**
			@brief	Sets up board routing for playout.
		**/
		void			RouteOutputSignal (void);


	//	Private Member Data
	private:
		CNTV2Card				mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID			mDeviceID;			///< @brief	My device identifier
		const std::string		mDeviceSpecifier;	///< @brief	Specifies the device I should use
		const std::string		mTestPatternSpec;	///< @brief	Specifies the test pattern I should use
		const NTV2Channel		mOutputChannel;		///< @brief	The channel I'm using
		const NTV2PixelFormat	mPixelFormat;		///< @brief	My pixel format
		NTV2VideoFormat			mVideoFormat;		///< @brief	My video format
		NTV2TaskMode			mSavedTaskMode;		///< @brief For restoring previous task mode
		NTV2XptConnections		mSavedConnections;	///< @brief	For restoring previous routing
		NTV2VANCMode			mVancMode;			///< @brief	My VANC mode

};	//	NTV2OutputTestPattern

#endif	//	_NTV2OUTPUT_TEST_PATTERN_H
