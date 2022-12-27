/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2fieldburn/main.cpp
	@brief		Demonstration application to capture frames from the SDI input as two distinct fields
				in separate, non-contiguous memory locations, "burn" a timecode window into each field,
				and recombine the modified fields for SDI playout.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2fieldburn.h"
#include "ajabase/system/systemtime.h"
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
	char *		pDeviceSpec 	(AJA_NULL);	//	Which device to use
	char *		pPixelFormat	(AJA_NULL);	//	Pixel format spec
	uint32_t	inputNumber		(1);		//	Which input to use (1-8, defaults to 1)
	int			noAudio			(0);		//	Disable audio?
	int			noFieldMode		(0);		//	Disable AutoCirculate Field Mode?
	int			doMultiChannel	(0);		//  Set the board up for multi-channel/format
	poptContext	optionsContext;				//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		#if !defined(NTV2_DEPRECATE_16_0)	//	--board option is deprecated!
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",		"(deprecated)"	},
		#endif
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",		"index#, serial#, or model"},
		{"input",		'i',	POPT_ARG_INT,		&inputNumber,	0,	"which SDI input to use",	"1-8"},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disables audio",			AJA_NULL},
		{"nofield",		0,		POPT_ARG_NONE,		&noFieldMode,	0,	"disables field mode",		AJA_NULL},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format",				"'?' or 'list' to list"},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"multiformat mode?",		AJA_NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	if (inputNumber > 8 || inputNumber < 1)
		{cerr << "## ERROR:  Input '" << inputNumber << "' not 1 thru 8" << endl;	return 1;}

	//	Devices
	const string	legalDevices	(CNTV2DemoCommon::GetDeviceStrings());
	const string	deviceSpec		(pDeviceSpec  ? pDeviceSpec  : "0");
	if (deviceSpec == "?" || deviceSpec == "list")
		{cout << legalDevices << endl;  return 0;}
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		{cout << "## ERROR:  No such device '" << deviceSpec << "'" << endl << legalDevices;  return 1;}

	//	Pixel format
	NTV2PixelFormat	pixelFormat		(NTV2_FBF_8BIT_YCBCR);
	const string	pixelFormatStr	(pPixelFormat  ? pPixelFormat :  "1");
	const string	legalFBFs		(CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	if (!pixelFormatStr.empty())
	{
		pixelFormat = CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
		if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT(pixelFormat))
			{cerr << "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl << legalFBFs << endl;  return 2;}
	}

	//	Instantiate our NTV2FieldBurn object...
	NTV2FieldBurn	burner (deviceSpec,										//	Which device?
							(noAudio ? false : true),						//	Include audio?
							(noFieldMode ? false : true),					//	Field mode?
							pixelFormat,									//	Frame buffer format
							::GetNTV2InputSourceForIndex(inputNumber - 1),	//	Which input source?
							doMultiChannel ? true : false);					//  Set the device up for multi-channel/format?

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif
	const string hdg1 ("           Capture  Playout  Capture  Playout");
	const string hdg2a("   Fields   Fields   Fields   Buffer   Buffer");
	const string hdg2b("   Frames   Frames   Frames   Buffer   Buffer");
	const string hdg3 ("Processed  Dropped  Dropped    Level    Level");
	const string hdg2 (noFieldMode ? hdg2b : hdg2a);

	//	Initialize the NTV2FieldBurn instance...
	if (AJA_FAILURE(burner.Init()))
		{cerr << "## ERROR: Initialization failed" << endl;  return 2;}

	//	Start the burner's capture and playout threads...
	burner.Run ();

	//	Loop until someone tells us to stop...
	cout << hdg1 << endl << hdg2 << endl << hdg3 << endl;
	do
	{
		ULWord	totalFrames(0),  inputDrops(0),  outputDrops(0),  inputBufferLevel(0), outputBufferLevel(0);
		burner.GetStatus (totalFrames, inputDrops, outputDrops, inputBufferLevel, outputBufferLevel);
		cout	<< setw(9) << totalFrames << setw(9) << inputDrops << setw(9) << outputDrops << setw(9) << inputBufferLevel
				<< setw(9) << outputBufferLevel << "\r" << flush;
		AJATime::Sleep(500);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;
	return 0;

}	//	main
