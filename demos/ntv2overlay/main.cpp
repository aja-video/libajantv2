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
	char *	pDeviceSpec		(AJA_NULL);		//	Which device to use
	char *	pInputSrcSpec	(AJA_NULL);		//	SDI source spec
	char *	pOutputDest		(AJA_NULL);		//	SDI output spec (sdi1 ... sdi8)
	int		doMultiFormat	(0);			//	MultiFormat mode?
	int		showVersion		(0);			//	Show version?
	int		verbose			(0);			//	Verbose mode?
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",	AJA_NULL					},
		{"input",		'i',	POPT_ARG_STRING,	&pInputSrcSpec,	0,	"input to use",             "1-8, ?=list"				},
		{"output",		'o',	POPT_ARG_STRING,	&pOutputDest,	0,	"output to use",			"'?' or 'list' to list"		},
		{"verbose",		0,		POPT_ARG_NONE,		&verbose,		0,	"verbose mode?",			AJA_NULL					},
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
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		return 1;

	OverlayConfig config(deviceSpec);

	//	Input source
	const string inputSourceStr	(pInputSrcSpec ? CNTV2DemoCommon::ToLower(string(pInputSrcSpec)) : "");
	const string legalSources (CNTV2DemoCommon::GetInputSourceStrings(NTV2_IOKINDS_ALL, pDeviceSpec ? deviceSpec : ""));
	config.fInputSource = CNTV2DemoCommon::GetInputSourceFromString(inputSourceStr, NTV2_IOKINDS_ALL, pDeviceSpec ? deviceSpec : "");
	if (inputSourceStr == "?" || inputSourceStr == "list")
		{cout << legalSources << endl;  return 0;}
	if (!inputSourceStr.empty()  &&  !NTV2_IS_VALID_INPUT_SOURCE(config.fInputSource))
		{cerr << "## ERROR:  Input source '" << inputSourceStr << "' not one of:" << endl << legalSources << endl;	return 1;}

	//	Output destination
	const string outputDestStr (pOutputDest ? CNTV2DemoCommon::ToLower(string(pOutputDest)) : "");
	const string legalOutputs (CNTV2DemoCommon::GetOutputDestinationStrings(NTV2_IOKINDS_ALL, pDeviceSpec ? deviceSpec : ""));
	config.fOutputDest = CNTV2DemoCommon::GetOutputDestinationFromString(outputDestStr, NTV2_IOKINDS_ALL, pDeviceSpec ? deviceSpec : "");
	if (outputDestStr == "?" || outputDestStr == "list")
		{cout << legalOutputs << endl;  return 0;}
	if (!outputDestStr.empty() && !NTV2_IS_VALID_OUTPUT_DEST(config.fOutputDest))
		{cerr << "## ERROR:  Output '" << outputDestStr << "' not of:" << endl << legalOutputs << endl;	return 1;}

	config.fDoMultiFormat	= doMultiFormat ? true : false;
	config.fVerbose			= verbose ? true  : false;

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
		cout	<< setw(9) << ++counter
				<< "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;
	return 0;

}	//	main
