/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2overlay/main.cpp
	@brief		Demonstration application that overlays an image (that has transparency) on top of incoming video.
	@copyright	(C) 2022-2023 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2overlay.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
static bool	gGlobalQuit		(false);	///< @brief	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	char *	pDeviceSpec	(AJA_NULL);	//	Which device to use
	int		showVersion	(0);		//	Show version?
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	//	Device
	const string deviceSpec (pDeviceSpec ? pDeviceSpec : "0");
	OverlayConfig config(deviceSpec);
	config.fInputSource		= NTV2_INPUTSOURCE_SDI1;	//	Use NTV2_INPUTSOURCE_HDMI1 or NTV2_INPUTSOURCE_SDI1
	config.fPixelFormat		= NTV2_FBF_ARGB;			//	Use NTV2_FBF_8BIT_YCBCR or NTV2_FBF_ARGB
	config.fDoMultiFormat	= true;

	//	Instantiate the NTV2Overlay object...
	NTV2Overlay overlayer (config);

	//	Initialize the NTV2Burn instance...
	AJAStatus status (overlayer.Init());
	if (AJA_FAILURE(status))
		{cerr << "## ERROR:  Initialization failed, status=" << status << endl;  return 4;}

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Start the burner's capture and playout threads...
	overlayer.Run();

	//	Loop until told to stop...
	ULWord counter(0);
	do
	{
		cout << setw(9) << ++counter << "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;
	return 0;

}	//	main
