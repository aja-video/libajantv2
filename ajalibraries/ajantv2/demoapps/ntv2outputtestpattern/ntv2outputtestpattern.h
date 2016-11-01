/**
	@file		ntv2outputtestpattern.cpp
	@brief		Header file for NTV2OutputTestPattern demonstration class
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2OUTPUT_TEST_PATTERN_H
#define _NTV2OUTPUT_TEST_PATTERN_H

#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ajastuff/common/types.h"
#include "ajastuff/common/testpatterngen.h"
#include "ajastuff/common/videotypes.h"
#include "ajastuff/system/process.h"


/**
	@brief	I DMA a test pattern into an AJA device's frame store for steady-state playout.
			This demonstrates how to perform direct DMA from a host frame buffer.
	@note	I do not use AutoCirculate to transfer frame data to the device.
**/

class NTV2OutputTestPattern
{
	//	Public Instance Methods
	public:

		/**
			@brief		Constructs me using the given configuration settings.
			@note		I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inDeviceSpecifier	Specifies the AJA device to use. Defaults to "0", the first device found.
			@param[in]	inChannel			Specifies the channel (frame store) to use. Defaults to NTV2_CHANNEL1.
		**/
		NTV2OutputTestPattern (const std::string &	inDeviceSpecifier	= "0",
							   const NTV2Channel	inChannel			= NTV2_CHANNEL1);

		~NTV2OutputTestPattern (void);

		/**
			@brief	Initializes me and prepares me to Run.
			@return	AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
		**/
		AJAStatus		Init (void);

		/**
			@brief		Displays the requested test pattern on the output.
			@param[in]	testPatternIndex	Specifies which test pattern to display.
											Defaults to 0 (100% color bars).
			@return	AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
			@note		Do not call this method without first calling my Init method.
		**/
		AJAStatus		EmitPattern (const UWord testPatternIndex = 0);


	//	Protected Instance Methods
	protected:
		/**
			@brief	Sets up my AJA device to play video.
			@return	AJA_STATUS_SUCCESS if successful; otherwise another AJAStatus code if unsuccessful.
		**/
		AJAStatus		SetUpVideo (void);

		/**
			@brief	Sets up board routing for playout.
		**/
		void			RouteOutputSignal (void);


	//	Private Member Data
	private:
		CNTV2Card					mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;			///< @brief	My device identifier
		const std::string			mDeviceSpecifier;	///< @brief	Specifies the device I should use
		const NTV2Channel			mOutputChannel;		///< @brief	The channel I'm using
		NTV2VideoFormat				mVideoFormat;		///< @brief	My video format
		NTV2FrameBufferFormat		mPixelFormat;		///< @brief	My pixel format
		NTV2EveryFrameTaskMode		mSavedTaskMode;		///< @brief Used to restore the previous task mode
		bool						mVancEnabled;		///< @brief	VANC enabled?
		bool						mWideVanc;			///< @brief	Wide VANC?

};	//	NTV2OutputTestPattern

#endif	//	_NTV2OUTPUT_TEST_PATTERN_H
