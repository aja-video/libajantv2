/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2outputtestpattern.cpp
	@brief		Header file for NTV2OutputTestPattern demonstration class
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2OUTPUT_TEST_PATTERN_H
#define _NTV2OUTPUT_TEST_PATTERN_H

#include "ntv2democommon.h"


/**
	@brief	Configures an NTV2OutputTestPattern instance.
**/
typedef struct TestPatConfig : public PlayerConfig
{
	public:
		std::string		fTestPatternName;		///< @brief	Name of the test pattern to use

		/**
			@brief	Constructs a default CCPlayer configuration.
		**/
		inline explicit	TestPatConfig (const std::string & inDeviceSpecifier	= "0")
			:	PlayerConfig		(inDeviceSpecifier),
				fTestPatternName	("100% ColorBars")
		{
		}

		AJALabelValuePairs Get (const bool inCompact = false) const;

}	TestPatConfig;

std::ostream &	operator << (std::ostream & ioStrm, const TestPatConfig & inObj);


/**
	@brief	I generate and transfer a test pattern into an AJA device's frame buffer for steady-state
			playout using NTV2TestPatternGen::DrawTestPattern and CNTV2Card::DMAWriteFrame.
**/
class NTV2OutputTestPattern
{
	//	Public Instance Methods
	public:

		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inConfig		Specifies the configuration parameters.
		**/
		NTV2OutputTestPattern (const TestPatConfig & inConfig);

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
		TestPatConfig			mConfig;			///< @brief	My configuration settings
		CNTV2Card				mDevice;			///< @brief	My CNTV2Card instance
		NTV2DeviceID			mDeviceID;			///< @brief	My device identifier
		NTV2TaskMode			mSavedTaskMode;		///< @brief For restoring previous task mode
		NTV2XptConnections		mSavedConnections;	///< @brief	For restoring previous routing

};	//	NTV2OutputTestPattern

#endif	//	_NTV2OUTPUT_TEST_PATTERN_H
