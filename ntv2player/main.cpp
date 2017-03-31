/**
	@file		ntv2player/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout frames to SDI output
				generated in host memory containing test pattern and timecode, including audio tone.
	@copyright	Copyright (C) 2012-2017 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ntv2utils.h"
#include "ajabase/common/options_popt.h"
#include "ntv2player.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#include "ajabase/system/systemtime.h"


//	Globals
static bool		gGlobalQuit		(false);	//	Set this "true" to exit gracefully

static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	AJAStatus		status				(AJA_STATUS_SUCCESS);
	char *			pVideoFormat		(NULL);					//	Video format argument
	char *			pPixelFormat		(NULL);					//	Pixel format argument
    char *			pDeviceSpec 		(NULL);					//	Device argument
	uint32_t		channelNumber		(1);					//	Number of the channel to use
	int				noAudio				(0);					//	Disable audio tone?
	int				doMultiChannel		(0);					//	Enable multi-format?
	int				hdrType				(0);
	poptContext		optionsContext; 							//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"index#, serial#, or model"	},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"which video format to use",		"'?' or 'list' to list"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"which pixel format to use",		"'?' or 'list' to list"},
		{"hdrType",		't',	POPT_ARG_INT,		&hdrType,		0,	"which HDR Packet to send",			"1:SDR,2:HDR10,3:HLG"},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,	0,	"which channel to use",				"number of the channel"},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multi-channel/format",			NULL},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio tone",				NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	const string			deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	const string			videoFormatStr	(pVideoFormat  ?  pVideoFormat  :  "");
	const NTV2VideoFormat	videoFormat		(videoFormatStr.empty () ? NTV2_FORMAT_1080i_5994 : CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr));
	if (videoFormatStr == "?" || videoFormatStr == "list")
		{cout << CNTV2DemoCommon::GetVideoFormatStrings (NON_UHD_VIDEO_FORMATS, deviceSpec) << endl;  return 0;}
	else if (!videoFormatStr.empty () && videoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings (NON_UHD_VIDEO_FORMATS, deviceSpec) << endl;
		return 2;
	}

	const string				pixelFormatStr	(pPixelFormat  ?  pPixelFormat  :  "");
	const NTV2FrameBufferFormat	pixelFormat		(pixelFormatStr.empty () ? NTV2_FBF_10BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString (pixelFormatStr));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty () && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT (pixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	if (channelNumber < 1 || channelNumber > 8)
	{
		cerr << "## ERROR:  Invalid channel number '" << channelNumber << "' -- expected 1 thru 8" << endl;
		return 2;
	}

	const NTV2Channel			channel		(::GetNTV2ChannelForIndex (channelNumber - 1));
	const NTV2OutputDestination	outputDest	(::NTV2ChannelToOutputDestination (channel));
	AJAAncillaryDataType sendType = AJAAncillaryDataType_Unknown;
	switch(hdrType)
	{
	case 1:
		sendType = AJAAncillaryDataType_HDR_SDR;
		break;
	case 2:
		sendType = AJAAncillaryDataType_HDR_HDR10;
		break;
	case 3:
		sendType = AJAAncillaryDataType_HDR_HLG;
		break;
	default:
		sendType = AJAAncillaryDataType_Unknown;
		break;
	}

	NTV2Player	player (deviceSpec, (noAudio ? false : true), channel, pixelFormat, outputDest, videoFormat, false, false, doMultiChannel ? true : false, sendType);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the player...
	status = player.Init ();
	if (AJA_SUCCESS (status))
	{
		//	Run the player...
		player.Run ();

		cout	<< "  Playout  Playout   Frames" << endl
				<< "   Frames   Buffer  Dropped" << endl;
		do
		{
			ULWord	framesProcessed, framesDropped, bufferLevel;

			//	Poll the player's status...
			player.GetACStatus (framesProcessed, framesDropped, bufferLevel);
			cout << setw (9) << framesProcessed << setw (9) << bufferLevel << setw (9) << framesDropped << "\r" << flush;
			AJATime::Sleep (2000);
		} while (player.IsRunning () && !gGlobalQuit);	//	loop til done

		cout << endl;

		//  Ask the player to stop
		player.Quit ();

	}	//	if player Init succeeded
	else
		cerr << "Player initialization failed with status " << status << endl;

	return 0;
 
}	//	main
