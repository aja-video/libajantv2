/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn4kquadrant/main.cpp
	@brief		Demonstration application to "burn" timecode into frames captured from SDI input,
				and play out the modified frames to SDI output.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
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
	char *			pDeviceSpec		(AJA_NULL);		//	Which device to use for input
	char *			pOutDevSpec 	(AJA_NULL);		//	Which device to use for output
	char *			pTcSource		(AJA_NULL);		//	Time code source string
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format spec
	int				showVersion		(0);			//	Show version?
	int				noAudio			(0);			//	Disable audio?
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",	AJA_NULL					},
		{"input",		'i',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"input device to use",	"index#, serial#, or model"	},
		{"output",		'o',	POPT_ARG_STRING,	&pOutDevSpec,	0,	"output device",		"index#, serial#, or model"	},
		{"tcsource",	't',	POPT_ARG_STRING,	&pTcSource,		0,	"time code source",		"'?' to list"				},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",		AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",	"'?' or 'list' to list"		},
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

	BurnConfig config (deviceSpec);

	//	Devices
	config.fDeviceSpec2 = pOutDevSpec ? pOutDevSpec : "1";
	if (!CNTV2DemoCommon::IsValidDevice(config.fDeviceSpec2))
		{cout << "## ERROR:  No such output device '" << config.fDeviceSpec2 << "'" << endl;  return 1;}

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

	//	Timecode source...
	const string	legalTCSources(CNTV2DemoCommon::GetTCIndexStrings(TC_INDEXES_ALL, deviceSpec));
	const string	tcSourceStr		(pTcSource ? CNTV2DemoCommon::ToLower(pTcSource) : "");
	if (tcSourceStr == "?"  ||  tcSourceStr == "list")
		{cout << legalTCSources << endl;  return 0;}
	if (!tcSourceStr.empty())
	{
		config.fTimecodeSource = CNTV2DemoCommon::GetTCIndexFromString(tcSourceStr);
		if (!NTV2_IS_VALID_TIMECODE_INDEX(config.fTimecodeSource))
			{cerr << "## ERROR:  Timecode source '" << tcSourceStr << "' not one of these:" << endl << legalTCSources << endl;	return 1;}
	}

	config.fSuppressAudio	= noAudio ? true  : false;
	config.fInputFrames.setCountOnly(5);
	config.fOutputFrames.setCountOnly(5);

	//	Instantiate the NTV2Burn4KQuadrant object...
	NTV2Burn4KQuadrant burner (config);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2Burn4KQuadrant instance...
	AJAStatus status (burner.Init());
	if (AJA_FAILURE(status))
		{cerr << "## ERROR:  Initialization failed, status=" << status << endl;  return 4;}

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
