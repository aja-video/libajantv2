/**
	@file		ntv2player4k/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout 4k frames to SDI output
				generated in host memory containing test pattern and timecode, including audio tone.
	@copyright	(C) 2012-2020 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2player8k.h"
#include "ntv2utils.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#include "ajabase/system/systemtime.h"

using namespace std;


//	Globals
static bool	gGlobalQuit	(false);	//	Set this "true" to exit gracefully

static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	char *			pVideoFormat	(AJA_NULL);	//	Video format argument
	char *			pPixelFormat	(AJA_NULL);	//	Pixel format argument
    char *			pDeviceSpec 	(AJA_NULL);	//	Device argument
	uint32_t		channelNumber	(1);		//	Number of the channel to use
	int				noAudio			(0);		//	Disable audio tone?
	int				useHDMIOut		(0);		//	Enable HDMI output?
	int				doMultiChannel	(0);		//  More than one instance of player 4k
	int				doRGBOnWire		(0);		//  Route the output to put RGB on the wire
	int				doTsiRouting	(0);		//  Route the output through the Tsi Muxes
	int				hdrType			(0);		//	Custom anc type?
	poptContext		optionsContext; 			//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,		0,	"which video format to use",	"e.g. 'uhd24' or ? to list"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,		0,	"which pixel format to use",	"e.g. 'yuv8' or ? to list"},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,		0,	"which channel to use",			"number of the channel"},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,	0,	"use multi-channel/format",		AJA_NULL},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,			0,	"disable audio tone",			AJA_NULL},
		{"hdmi",		'h',	POPT_ARG_NONE,		&useHDMIOut,		0,	"enable HDMI output?",			AJA_NULL},
		{"rgb",			'r',	POPT_ARG_NONE,		&doRGBOnWire,		0,	"emit RGB over SDI?",			AJA_NULL},
		{"tsi",			't',	POPT_ARG_NONE,		&doTsiRouting,		0,	"use Tsi routing?",				AJA_NULL},
		{"hdrType",		'x',	POPT_ARG_INT,		&hdrType,			0,	"which HDR Packet to send",		"1:SDR,2:HDR10,3:HLG"},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	const string			deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	const string			videoFormatStr	(pVideoFormat  ?  pVideoFormat  :  "");
	const NTV2VideoFormat	videoFormat		(videoFormatStr.empty () ? NTV2_FORMAT_4x1920x1080p_2398 : CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr, VIDEO_FORMATS_8KUHD2));
	if (videoFormatStr == "?" || videoFormatStr == "list")
		{cerr << "## NOTE:  List of valid '--videoFormat' parameter values:\n" << CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_8KUHD2, deviceSpec) << endl;  return 0;}
	else if (!videoFormatStr.empty () && !NTV2_IS_QUAD_QUAD_FORMAT (videoFormat))
		{cerr << "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl << CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_8KUHD2, deviceSpec) << endl;  return 2;}

	const string				pixelFormatStr	(pPixelFormat  ?  pPixelFormat  :  "");
	const NTV2FrameBufferFormat	pixelFormat		(pixelFormatStr.empty () ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString (pixelFormatStr));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cerr << "## NOTE:  List of valid '--pixelFormat' parameter values:" << endl << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty () && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT (pixelFormat))
		{cerr << "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 2;}

	Player8KConfig	config;

	config.fDeviceSpecifier	= deviceSpec;
	config.fWithAudio		= noAudio ? false : true;
	config.fChannel			= NTV2Channel (channelNumber - 1);
	config.fPixelFormat		= pixelFormat;
	config.fVideoFormat		= videoFormat;
	config.fUseHDMIOut		= useHDMIOut ? true : false;
	config.fDoMultiChannel	= doMultiChannel ? true : false;
	config.fDoTsiRouting	= doTsiRouting ? true : false;
	config.fDoRGBOnWire		= doRGBOnWire ? true : false;
	switch (hdrType)
	{
		case 1:		config.fSendAncType = AJAAncillaryDataType_HDR_SDR;		break;
		case 2:		config.fSendAncType = AJAAncillaryDataType_HDR_HDR10;	break;
		case 3:		config.fSendAncType = AJAAncillaryDataType_HDR_HLG;		break;
		default:	config.fSendAncType = AJAAncillaryDataType_Unknown;		break;
	}

	NTV2Player8K player (config);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the player...
	if (AJA_FAILURE(player.Init()))
		{cerr << "## ERROR: Initialization failed" << endl;  return 1;}

	//	Run the player...
	player.Run ();

	cout	<< "  Playout  Playout   Frames" << endl
			<< "   Frames   Buffer  Dropped" << endl;

	//	Loop until told to stop...
	do
	{	//	Poll the player's status...
		AUTOCIRCULATE_STATUS outputStatus;
		player.GetACStatus(outputStatus);
		cout	<<	setw(9) << outputStatus.acFramesProcessed
				<<	setw(9) << outputStatus.acBufferLevel
				<<  setw(9) << outputStatus.acFramesDropped
				<< "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop til done

	//  Ask the player to stop
	player.Quit();

	cout << endl;
	return 0;

}	//	main
