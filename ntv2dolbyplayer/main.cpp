/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolby/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout video and Doly audio to HDMI.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ntv2utils.h"
#include "ajabase/common/options_popt.h"
#include "ntv2dolbyplayer.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#include "ajabase/system/systemtime.h"
#include "ajabase/system/file_io.h"

using namespace std;


//	Globals
static bool		gGlobalQuit		(false);	//	Set this "true" to exit gracefully

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
    char *			pDolbyName   	(AJA_NULL);	//	Dolby audio file name
    uint32_t		channelNumber	(2);		//	Number of the channel to use
	int				noAudio			(0);		//	Disable audio tone?
	int				doMultiChannel	(0);		//	Enable multi-format?
	poptContext		optionsContext; 			//	Context for parsing command line arguments
    AJAStatus       status;

	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#, serial#, or model"	},
        {"dolbyFile",	'f',	POPT_ARG_STRING,	&pDolbyName,	0,	"dolby audio to play",			"file name"	},
        {"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"which video format to use",	"'?' or 'list' to list"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"which pixel format to use",	"'?' or 'list' to list"},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,	0,	"which channel to use",			"number of the channel"},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multi-channel/format",		AJA_NULL},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio tone",			AJA_NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	const string			deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	const string			videoFormatStr	(pVideoFormat  ?  pVideoFormat  :  "");
	const NTV2VideoFormat	videoFormat		(videoFormatStr.empty () ? NTV2_FORMAT_1080i_5994 : CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr));
	if (videoFormatStr == "?" || videoFormatStr == "list")
		{cout << CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_NON_4KUHD, deviceSpec) << endl;  return 0;}
	else if (!videoFormatStr.empty () && videoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_NON_4KUHD, deviceSpec) << endl;
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

    if (channelNumber < 2 || channelNumber > 4)
        {cerr << "## ERROR:  Invalid channel number '" << channelNumber << "' -- expected 2 thru 4" << endl;  return 2;}

    AJAFileIO fileIO;
    AJAFileIO* pDolbyFile = NULL;
    const string fileStr	(pDolbyName  ?  pDolbyName  :  "");
    if (!fileStr.empty ())
    {
        status = fileIO.Open(pDolbyName, eAJAReadOnly, 0);
        if (status == AJA_STATUS_SUCCESS)
            pDolbyFile = &fileIO;
    }

	const NTV2Channel			channel		(::GetNTV2ChannelForIndex (channelNumber - 1));

	NTV2DolbyPlayer	player (deviceSpec,						//	inDeviceSpecifier
							(noAudio ? false : true),		//	inWithAudio
							channel,						//	inChannel
							pixelFormat,					//	inPixelFormat
							videoFormat,					//	inVideoFormat
                            doMultiChannel ? true : false,  //	inDoMultiFormat
                            pDolbyFile);                    //  inDolbyFile

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the player...
	if (AJA_FAILURE(player.Init()))
		{cerr << "## ERROR: Initialization failed" << endl;  return 3;}

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

	//  Ask the player to stop
	player.Quit();

	cout << endl;
	return 0;
 
}	//	main
