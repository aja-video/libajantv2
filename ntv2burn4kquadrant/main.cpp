/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn4kquadrant/main.cpp
	@brief		Demonstration application to "burn" timecode into frames captured from SDI input,
				and play out the modified frames to SDI output.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2burn4kquadrant.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
static bool	gGlobalQuit		(false);	//	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	AJAStatus	status				(AJA_STATUS_SUCCESS);	//	Result status
	char *		pInputDeviceSpec 	(AJA_NULL);				//	Which device to use for capture
	char *		pOutputDeviceSpec 	(AJA_NULL);				//	Which device to use for playout
	char *		pTimecodeSpec		(AJA_NULL);				//	Timecode source spec
	char *		pPixelFormat		(AJA_NULL);				//	Pixel format spec
	int			noAudio				(0);					//	Disable audio?
	poptContext	optionsContext;								//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"input",	'i',	POPT_ARG_STRING,	&pInputDeviceSpec,	0,	"input device",		"index#, serial#, or model"	},
		{"output",	'o',	POPT_ARG_STRING,	&pOutputDeviceSpec,	0,	"output device",	"index#, serial#, or model"	},
		{"tcsource",'t',	POPT_ARG_STRING,	&pTimecodeSpec,		0,	"timecode source",	"'?' or 'list' to list"		},
		{"noaudio",	0,		POPT_ARG_NONE,		&noAudio,			0,	"disable audio?",	AJA_NULL					},
		{"pixelFormat",'p',	POPT_ARG_STRING,	&pPixelFormat,		0,	"pixel format",		"'?' or 'list' to list"		},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	//	Devices
	const string	legalDevices		(CNTV2DemoCommon::GetDeviceStrings());
	const string	inputDeviceSpec		(pInputDeviceSpec  ? pInputDeviceSpec  : "0");
	const string	outputDeviceSpec	(pOutputDeviceSpec ? pOutputDeviceSpec : "1");
	if (inputDeviceSpec == "?" || inputDeviceSpec == "list"  ||  outputDeviceSpec == "?" || outputDeviceSpec == "list")
		{cout << legalDevices << endl;  return 0;}
	if (!CNTV2DemoCommon::IsValidDevice(inputDeviceSpec))
		{cout << "## ERROR:  No such input device '" << inputDeviceSpec << "'" << endl << legalDevices;  return 1;}
	if (!CNTV2DemoCommon::IsValidDevice(outputDeviceSpec))
		{cout << "## ERROR:  No such output device '" << outputDeviceSpec << "'" << endl << legalDevices;  return 1;}

	//	Timecode source
	const string	tcSourceStr		(pTimecodeSpec ? CNTV2DemoCommon::ToLower(string(pTimecodeSpec)) : "");
	const string	legalTCSources	(CNTV2DemoCommon::GetTCIndexStrings(TC_INDEXES_ALL, inputDeviceSpec));
	NTV2TCIndex		tcSource		(NTV2_TCINDEX_SDI1);
	if (tcSourceStr == "?" || tcSourceStr == "list")
		{cout << legalTCSources << endl;  return 0;}
	if (!tcSourceStr.empty())
	{
		tcSource = CNTV2DemoCommon::GetTCIndexFromString(tcSourceStr);
		if (!NTV2_IS_VALID_TIMECODE_INDEX(tcSource))
			{cerr << "## ERROR:  Timecode source '" << tcSourceStr << "' not one of:" << endl << legalTCSources << endl;	return 1;}
	}

	//	Pixel format
	NTV2PixelFormat	pixelFormat		(NTV2_FBF_8BIT_YCBCR);
	const string	pixelFormatStr	(pPixelFormat  ? pPixelFormat :  "1");
	const string	legalFBFs		(CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, inputDeviceSpec));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, inputDeviceSpec) << endl;  return 0;}
	if (!pixelFormatStr.empty())
	{
		pixelFormat = CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
		if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT(pixelFormat))
			{cerr << "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl << legalFBFs << endl;  return 2;}
	}

	//	Instantiate our NTV2Burn4KQuadrant object...
	NTV2Burn4KQuadrant	burner (inputDeviceSpec,		//	Which device will be the input device?
								outputDeviceSpec,		//	Which device will be the output device?
								noAudio ? false : true,	//	Include audio?
								pixelFormat,			//	Frame buffer format
								tcSource);				//	Which time code source?				

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2Burn4KQuadrant instance...
	status = burner.Init();
	if (AJA_FAILURE(status))
		{cerr << "## ERROR: Initialization failed" << endl;  return 2;}

	//	Start the burner's capture and playout threads...
	burner.Run();

	//	Loop until someone tells us to stop...
	cout	<< "           Capture  Playout  Capture  Playout" << endl
			<< "   Frames   Frames   Frames   Buffer   Buffer" << endl
			<< "Processed  Dropped  Dropped    Level    Level" << endl;
	do
	{
		AUTOCIRCULATE_STATUS inputStatus, outputStatus;
		burner.GetACStatus (inputStatus, outputStatus);
		cout	<< setw(9) << inputStatus.acFramesProcessed
				<< setw(9) << inputStatus.acFramesDropped
				<< setw(9) << outputStatus.acFramesDropped
				<< setw(9) << inputStatus.acBufferLevel
				<< setw(9) << outputStatus.acBufferLevel
				<< "\r" << flush;
		AJATime::Sleep(1000);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;
	return 0;

}	//	main
