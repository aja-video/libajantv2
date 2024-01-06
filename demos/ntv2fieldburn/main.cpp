/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2fieldburn/main.cpp
	@brief		Demonstration application that "burns" timecode into separate fields captured from interlaced
				SDI input video, and plays out the modified fields to SDI output.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2fieldburn.h"
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
	char *			pDeviceSpec 	(AJA_NULL);		//	Which device to use
	char *			pInputSrcSpec	(AJA_NULL);		//	SDI source spec
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format spec
	int				doMultiFormat	(0);			//	MultiFormat mode?
	int				showVersion		(0);			//	Show version?
	int				noAudio			(0);			//	Disable audio?
	int				noFieldMode		(0);			//	Disable AutoCirculate Field Mode?
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",	AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"		},
		{"input",		'i',	POPT_ARG_STRING,	&pInputSrcSpec,	0,	"SDI input to use",			"1-8, ?=list"				},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",			AJA_NULL					},
		{"nofield",		0,		POPT_ARG_NONE,		&noFieldMode,	0,	"disables field mode",		AJA_NULL					},
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

	BurnConfig config(deviceSpec);

	//	Input source
	const string	legalSources(CNTV2DemoCommon::GetInputSourceStrings(NTV2_IOKINDS_ALL, deviceSpec));
	const string inputSourceStr	(pInputSrcSpec ? CNTV2DemoCommon::ToLower(string(pInputSrcSpec)) : "");
	if (inputSourceStr == "?" || inputSourceStr == "list")
		{cout << legalSources << endl;  return 0;}
	if (!inputSourceStr.empty())
	{
		config.fInputSource = CNTV2DemoCommon::GetInputSourceFromString(inputSourceStr);
		if (!NTV2_IS_VALID_INPUT_SOURCE(config.fInputSource))
			{cerr << "## ERROR:  Input source '" << inputSourceStr << "' not one of:" << endl << legalSources << endl;	return 1;}
	}	//	if input source specified

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

	config.fDoMultiFormat	= doMultiFormat ? true : false;
	config.fSuppressAudio	= noAudio ? true : false;
	config.fIsFieldMode		= noFieldMode ? false : true;
	config.fTimecodeSource	= ::NTV2ChannelToTimecodeIndex(::NTV2InputSourceToChannel(config.fInputSource));

	//	Instantiate the NTV2FieldBurn object...
	NTV2FieldBurn burner (config);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif
	const string	hdg1 ("           Capture  Playout  Capture  Playout"),
					hdg2a("   Fields   Fields   Fields   Buffer   Buffer"),
					hdg2b("   Frames   Frames   Frames   Buffer   Buffer"),
					hdg3 ("Processed  Dropped  Dropped    Level    Level"),
					hdg2 (noFieldMode ? hdg2b : hdg2a);

	//	Initialize the NTV2FieldBurn instance...
	AJAStatus status (burner.Init());
	if (AJA_FAILURE (status))
		{cerr << "## ERROR:  Initialization failed, status=" << status << endl;  return 4;}

	//	Start the burner's capture and playout threads...
	burner.Run();

	//	Loop until told to stop...
	cout << hdg1 << endl << hdg2 << endl << hdg3 << endl;
	do
	{
		ULWord	totalFrames(0),  inputDrops(0),  outputDrops(0),  inputBufferLevel(0), outputBufferLevel(0);
		burner.GetStatus (totalFrames, inputDrops, outputDrops, inputBufferLevel, outputBufferLevel);
		cout	<< setw(9) << totalFrames << setw(9) << inputDrops << setw(9) << outputDrops
				<< setw(9) << inputBufferLevel << setw(9) << outputBufferLevel << "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;
	return 0;

}	//	main
