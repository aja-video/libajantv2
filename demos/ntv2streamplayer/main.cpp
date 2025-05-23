/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2streamplayer/main.cpp
	@brief		Demonstration application that plays synthesized SD/HD video.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2streamplayer.h"
#include <signal.h>


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
	char *			pDeviceSpec		(AJA_NULL);		//	Device specifier string, if any
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format argument
	char *			pFramesSpec		(AJA_NULL);		//	AutoCirculate frames spec
	char *			pAncFilePath	(AJA_NULL);		//	Anc data filepath to play
	char *			pVideoFormat	(AJA_NULL);		//	Video format to use
	int				channelNumber	(1);			//	Channel/FrameStore to use
	int				doMultiFormat	(0);			//	MultiFormat mode?
	int				showVersion		(0);			//	Show version?
	AJADebug::Open();

	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",			"1-8"						},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",	AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"		},
		{"frames",		  0,	POPT_ARG_STRING,	&pFramesSpec,	0,	"frames to Stream",			"num[@min] or min-max"		},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"video format to produce",	"'?' or 'list' to list"		},
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

	PlayerConfig config(deviceSpec);

	//	Channel
	if ((channelNumber < 1)  ||  (channelNumber > 8))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 1;}
	config.fOutputChannel = NTV2Channel(channelNumber - 1);

	//	VideoFormat
	const string videoFormatStr (pVideoFormat  ?  pVideoFormat  :  "");
	config.fVideoFormat = videoFormatStr.empty()	?	NTV2_FORMAT_1080i_5994
													:	CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr, VIDEO_FORMATS_ALL);
	if (videoFormatStr == "?"  ||  videoFormatStr == "list")
		{cout	<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_ALL, pDeviceSpec ? deviceSpec : "") << endl;  return 0;}
	else if (!videoFormatStr.empty()  &&  config.fVideoFormat == NTV2_FORMAT_UNKNOWN)
	{	cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	Pixel Format
	const string pixelFormatStr (pPixelFormat  ?  pPixelFormat  :  "");
	config.fPixelFormat = pixelFormatStr.empty() ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
	if (pixelFormatStr == "?"  ||  pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, pDeviceSpec ? deviceSpec : "") << endl;  return 0;}
	else if (!pixelFormatStr.empty()  &&  !NTV2_IS_VALID_FRAME_BUFFER_FORMAT(config.fPixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
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

	//	Anc Playback
	string ancFilePath (pAncFilePath ? pAncFilePath : "");
	ancFilePath = aja::strip(ancFilePath);

	config.fAncDataFilePath		= ancFilePath;
	config.fOutputDest			= ::NTV2ChannelToOutputDestination(config.fOutputChannel);
	config.fSuppressAudio		= true;
	config.fSuppressVideo		= false;
	config.fDoMultiFormat		= doMultiFormat	? true	: false;			//	Multiformat mode?
	config.fTransmitLTC			= false;

	//	Instantiate and initialize the NTV2StreamPlayer object...
	NTV2StreamPlayer player(config);
	AJAStatus status = player.Init();
	if (AJA_FAILURE(status))
		{cout << "## ERROR:  Initialization failed: " << ::AJAStatusToString(status) << endl;	return 1;}

	::signal (SIGINT, SignalHandler);
	#if defined(AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Run it...
	player.Run();

	cout	<< "   Active   Repeat    Queue" << endl
			<< "    Count    Count    Depth" << endl;
	do
	{	//	Poll its status until stopped...
		NTV2StreamChannel strStatus;
		player.GetStreamStatus(strStatus);
		cout	<< setw(9) << strStatus.mActiveCount
				<< setw(9) << strStatus.mRepeatCount
				<< setw(9) << strStatus.GetQueueDepth() << "\r" << flush;
		AJATime::Sleep(2000);
	} while (player.IsRunning() && !gGlobalQuit);	//	loop til done

	cout << endl;
	return 0;

}	//	main
