/* SPDX-License-Identifier: MIT */
/**
	@file		gtest/main.cpp
	@brief		Demonstration application that "burns" timecode into frames captured from SDI input,
				and playout those modified frames to SDI output.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "gtest.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
bool	gGlobalQuit		(false);	///	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


/**
	@brief		Main entry point for 'gtest' demo application.
	@return		Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	char *			pDeviceSpec		((char *)"0");		//	Which device to use
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format spec
	AJADebug::Open();

  printf("gtest starting\n");
	//	Device
	const string deviceSpec (pDeviceSpec ? pDeviceSpec : "0");
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		return 1;

	GTestConfig config(deviceSpec);

	//	Pixel Format
	const string pixelFormatStr (pPixelFormat  ?  pPixelFormat  :  "");
	config.fPixelFormat = pixelFormatStr.empty() ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
	if (pixelFormatStr == "?"  ||  pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty()  &&  !NTV2_IS_VALID_FRAME_BUFFER_FORMAT(config.fPixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	Instantiate the GTest object...
	GTest gtst (config);
  printf("gtest instantiated\n");

	::signal (SIGINT, SignalHandler);

	//	Initialize the NTV2LLBurn instance...
	AJAStatus status (gtst.Init());
	if (AJA_FAILURE (status))
		{cerr << "## ERROR:  Initialization failed, status=" << status << endl;  return 4;}
  else
    printf("gtest initialized\n");

#if 0
	//	Start the gtest's playout threads...
	gtst.Run();

	//	Loop until told to stop...
	cout	<< "   Frames   Frames" << endl
			<< "Processed  Dropped" << endl;
	do
	{
		ULWord	framesProcessed, framesDropped;
		gtst.GetStatus (framesProcessed, framesDropped);
		cout	<< setw(9) << framesProcessed
				<< setw(9) << framesDropped
				<< "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;
#endif
	return 0;

}	//	main
