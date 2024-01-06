/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2player8k/main.cpp
	@brief		Demonstration application that plays synthesized 8K/UHD2 video.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2player8k.h"
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
	char *			pVideoFormat	(AJA_NULL);		//	Video format to use
	int				channelNumber	(1);			//	Channel/FrameStore to use
	int				doMultiFormat	(0);			//	MultiFormat mode?
	int				showVersion		(0);			//	Show version?
	int				numAudioLinks	(1);			//	Number of audio systems for multi-link audio
	int				useHDMIOut		(0);			//	Enable HDMI output?
	int				doRGBOnWire		(0);			//	Route the output to put RGB on the wire
	int				doSquares		(0);			//	Don't route output thru Tsi Muxes
	AJADebug::Open();

	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",			"1-8"						},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",	AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"		},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"video format to produce",	"'?' or 'list' to list"		},
		{"audioLinks",	'a',	POPT_ARG_INT,		&numAudioLinks,	0,	"# audio systems to link",	"1-4  0=silence"			},
		{"hdmi",		'h',	POPT_ARG_NONE,		&useHDMIOut,	0,	"enable HDMI output?",		AJA_NULL					},
		{"rgb",			'r',	POPT_ARG_NONE,		&doRGBOnWire,	0,	"RGB on SDI?",				AJA_NULL					},
		{"squares",		's',	POPT_ARG_NONE,		&doSquares,		0,	"use square routing?",		AJA_NULL					},
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
	config.fVideoFormat = videoFormatStr.empty()	?	NTV2_FORMAT_4x3840x2160p_2398
													:	CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr, VIDEO_FORMATS_4KUHD);
	if (videoFormatStr == "?"  ||  videoFormatStr == "list")
		{cout	<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_8KUHD2, deviceSpec) << endl;  return 0;}
	else if (!videoFormatStr.empty()  &&  !NTV2_IS_8K_VIDEO_FORMAT(config.fVideoFormat))
	{	cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_8KUHD2, deviceSpec) << endl;
		return 2;
	}

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

	//	Anc Playback
	config.fDoMultiFormat	= doMultiFormat	? true	: false;	//	Multiformat mode?
	config.fDoHDMIOutput	= useHDMIOut	? true	: false;
	config.fDoTsiRouting	= doSquares		? false	: true;
	config.fDoRGBOnWire		= doRGBOnWire	? true	: false;
	config.fNumAudioLinks	= UWord(numAudioLinks);

	//	Instantiate and initialize the NTV2Player8K object...
	NTV2Player8K player(config);
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

	cout	<< "   Frames   Frames   Buffer" << endl
			<< "   Played  Dropped    Level" << endl;
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
