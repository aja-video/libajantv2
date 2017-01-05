/**
	@file		ntv2player4k/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout 4k frames to SDI output
				generated in host memory containing test pattern and timecode, including audio tone.
	@copyright	Copyright (C) 2012-2017 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2player4k.h"
#include "ntv2utils.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#include "ajabase/system/systemtime.h"


//	Globals
static bool			gGlobalQuit		(false);	//	Set this "true" to exit gracefully

static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	AJAStatus		status			(AJA_STATUS_SUCCESS);
	char *			pVideoFormat	(NULL);				//	Video format argument
	char *			pPixelFormat	(NULL);				//	Pixel format argument
    char *			pDeviceSpec 	(NULL);				//	Device argument
	uint32_t		channelNumber	(1);				//	Number of the channel to use
	int				noAudio			(0);				//	Disable audio tone?
	int				useHDMIOut		(0);				//	Enable HDMI output?
	int				doMultiChannel	(0);				//  More than one instance of player 4k
	int				doRGBOnWire		(0);				//  Route the output to put RGB on the wire
	int				doTsiRouting	(0);				//  Route the output through the Tsi Muxes
	poptContext		optionsContext; 					//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,		0,	"which video format to use",	"e.g. '525i2997' or ? to list"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,		0,	"which pixel format to use",	"e.g. 'yuv8' or ? to list"},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,		0,	"which channel to use",			"number of the channel"},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,	0,	"use multi-channel/format",		NULL},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,			0,	"disable audio tone",			NULL},
		{"hdmi",		'h',	POPT_ARG_NONE,		&useHDMIOut,		0,	"enable HDMI output?",			NULL},
		{"rgb",			'r',	POPT_ARG_NONE,		&doRGBOnWire,		0,	"use RGB output?",				NULL},
		{"tsi",			't',	POPT_ARG_NONE,		&doTsiRouting,		0,	"use Tsi routing?",				NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	const string			deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	const string			videoFormatStr	(pVideoFormat  ?  pVideoFormat  :  "");
	const NTV2VideoFormat	videoFormat		(videoFormatStr.empty () ? NTV2_FORMAT_4x1920x1080p_2398 : CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr, UHD_VIDEO_FORMATS));
	if (videoFormatStr == "?" || videoFormatStr == "list")
		{cerr << "## NOTE:  List of valid '--videoFormat' parameter values:\n" << CNTV2DemoCommon::GetVideoFormatStrings (UHD_VIDEO_FORMATS, deviceSpec) << endl;  return 0;}
	else if (!videoFormatStr.empty () && !NTV2_IS_4K_VIDEO_FORMAT (videoFormat))
		{cerr << "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl << CNTV2DemoCommon::GetVideoFormatStrings (UHD_VIDEO_FORMATS, deviceSpec) << endl;  return 2;}

	const string				pixelFormatStr	(pPixelFormat  ?  pPixelFormat  :  "");
	const NTV2FrameBufferFormat	pixelFormat		(pixelFormatStr.empty () ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString (pixelFormatStr));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cerr << "## NOTE:  List of valid '--pixelFormat' parameter values:" << endl << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty () && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT (pixelFormat))
		{cerr << "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 2;}

	if (channelNumber != 1 && channelNumber != 5)
		{cerr << "## ERROR:  Invalid channel number '" << channelNumber << "' -- expected 1 or 5" << endl;  return 2;}

	Player4KConfig	config;

	config.fDeviceSpecifier	= deviceSpec;
	config.fWithAudio		= noAudio ? false : true;
	config.fChannel			= NTV2Channel (channelNumber - 1);
	config.fPixelFormat		= pixelFormat;
	config.fVideoFormat		= videoFormat;
	config.fUseHDMIOut		= useHDMIOut ? true : false;
	config.fDoMultiChannel	= doMultiChannel ? true : false;
	config.fDoTsiRouting	= doTsiRouting ? true : false;
	config.fDoRGBOnWire		= doRGBOnWire ? true : false;

	NTV2Player4K	player 	(config);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the player...
	status = player.Init ();
	if (AJA_FAILURE (status))
		{cerr << "Player initialization failed with status " << status << endl;  return 1;}

	bool	firstTimeAround	(true);

	//	Run the player...
	player.Run ();

	//	Loop until told to stop...
	while (!gGlobalQuit)
	{
		//	Poll the player's status...
		AUTOCIRCULATE_STATUS	outputStatus;
		player.GetACStatus (outputStatus);

		if (firstTimeAround)
		{
			cout	<< "  Playout  Playout   Frames" << endl
					<< "   Frames   Buffer  Dropped" << endl;
			firstTimeAround = false;
		}

		cout	<<	setw (9) << outputStatus.acFramesProcessed
				<<	setw (9) << outputStatus.acBufferLevel
				<<  setw (9) << outputStatus.acFramesDropped
				<< "\r" << flush;

		AJATime::Sleep (2000);

	}	//	loop til done

	cout << endl;

	//  Ask the player to stop
	player.Quit ();

	return 0;

}	//	main
