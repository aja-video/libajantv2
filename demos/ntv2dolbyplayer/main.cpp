/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolbyplayer/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout video and Doly audio to HDMI.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ntv2dolbyplayer.h"
#include <signal.h>


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
	char *			pDeviceSpec		(AJA_NULL);		//	Device specifier string, if any
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format argument
	char *			pFramesSpec		(AJA_NULL);		//	AutoCirculate frames spec
    char *			pDolbyFile		(AJA_NULL);		//	Optional path to Dolby audio file
	char *			pVideoFormat	(AJA_NULL);		//	Video format to use
	int				channelNumber	(2);			//	Channel/FrameStore to use
	int				doMultiFormat	(0);			//	MultiFormat mode?
	int				showVersion		(0);			//	Show version?
	int				noAudio			(0);			//	Disable audio tone?
	int				noVideo			(0);			//	Disable video?
	int				doRamp			(0);			//	Enable audio ramp
	AJAStatus status;

	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",			AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",				"index#, serial#, or model"	},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",				"2-4"						},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",		AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",			"'?' or 'list' to list"		},
		{"frames",		  0,	POPT_ARG_STRING,	&pFramesSpec,	0,	"frames to AutoCirculate",		"num[@min] or min-max"		},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"video format to produce",		"'?' or 'list' to list"		},
		{"dolby",		  0,	POPT_ARG_STRING,	&pDolbyFile,	0,	"dolby audio to play",			"path to binary file"		},
		{"ramp",		'r',	POPT_ARG_NONE,		&doRamp,		0,	"audio data ramp",				AJA_NULL					},
		{"noaudio",		  0,	POPT_ARG_NONE,		&noAudio,		0,	"disable audio",				AJA_NULL					},
		{"novideo",		  0,	POPT_ARG_NONE,		&noVideo,		0,	"disable video",				AJA_NULL					},

		POPT_AUTOHELP
		POPT_TABLEEND
	};

	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	const string deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		return 1;

	DolbyPlayerConfig config(deviceSpec);

	//	Channel
	if ((channelNumber < 2)  ||  (channelNumber > 4))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 2 thru 4" << endl;  return 1;}
	config.fOutputChannel = NTV2Channel(channelNumber - 1);

	//	VideoFormat
	const string videoFormatStr	(pVideoFormat  ?  pVideoFormat  :  "");
	config.fVideoFormat = videoFormatStr.empty()	?	NTV2_FORMAT_1080i_5994
													:	CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr, VIDEO_FORMATS_SDHD | VIDEO_FORMATS_4KUHD, deviceSpec);
	if (videoFormatStr == "?" || videoFormatStr == "list")
		{cout << CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_SDHD | VIDEO_FORMATS_4KUHD, pDeviceSpec ? deviceSpec : "") << endl;  return 0;}
	else if (!videoFormatStr.empty() && config.fVideoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_SDHD | VIDEO_FORMATS_4KUHD, deviceSpec) << endl;
		return 2;
	}

	//	Pixel Format
	const string pixelFormatStr	(pPixelFormat  ?  pPixelFormat  :  "");
	config.fPixelFormat = (pixelFormatStr.empty () ? NTV2_FBF_10BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString (pixelFormatStr));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, pDeviceSpec ? deviceSpec : "") << endl;  return 0;}
	else if (!pixelFormatStr.empty () && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT (config.fPixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	AutoCirculate Frames
	static const string	legalFramesSpec ("{frameCount}[@{firstFrameNum}]  or  {firstFrameNum}-{lastFrameNum}");
	const string framesSpec (pFramesSpec ? pFramesSpec : "");
	if (!framesSpec.empty())
	{
		const string parseResult(config.fFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## " << parseResult << endl;  return 1;}
	}
	if (!config.fFrames.valid())
		{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## Expected " << legalFramesSpec << endl;  return 1;}
		
	if (noVideo  &&  noAudio)
		{cerr	<< "## ERROR:  conflicting options '--novideo' and '--noaudio'" << endl;  return 1;}
		
	if (noAudio  &&  doRamp)
		{cerr	<< "## ERROR:  conflicting options '--noaudio' and '--ramp'" << endl;  return 1;}
		
	if (noAudio  &&  pDolbyFile)
		{cerr	<< "## ERROR:  conflicting options '--noaudio' and '--dolby'" << endl;  return 1;}
		
	if (doRamp  &&  pDolbyFile)
		{cerr	<< "## ERROR:  conflicting options '--ramp' and '--dolby'" << endl;  return 1;}

	//remaining options for this demo
	config.fSuppressAudio	= noAudio		? true	: false;
	config.fSuppressVideo	= noVideo		? true	: false;
	config.fDoMultiFormat	= doMultiFormat	? true	: false;	//	Multiformat mode?
	config.fDoRamp 			= doRamp		? true  : false;	//  inDoRamp
	config.fDoHDMIOutput	= true;  
	config.fDolbyFilePath = pDolbyFile  ?  pDolbyFile  :  "";

	NTV2DolbyPlayer	player (config);
	status = player.Init();
	if (AJA_FAILURE(status))
		{cout << "## ERROR:  Initialization failed: " << ::AJAStatusToString(status) << endl;	return 1;}

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Run the player...
	player.Run();

	cout	<< "  Playout  Playout   Frames" << endl
			<< "   Frames   Buffer  Dropped" << endl;
	do
	{	//	Poll its status until stopped...
		AUTOCIRCULATE_STATUS outputStatus;
		player.GetACStatus(outputStatus);
		cout	<< setw(9) << outputStatus.GetProcessedFrameCount()
				<< setw(9) << outputStatus.GetDroppedFrameCount()
				<< setw(9) << outputStatus.GetBufferLevel() << "\r" << flush;
		AJATime::Sleep(2000);
	} while (player.IsRunning() && !gGlobalQuit);	//	loop til done

	cout << endl;
	return 0;
 
}	//	main
