/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2player4k/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout 4k frames to SDI output
				generated in host memory containing test pattern and timecode, including audio tone.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
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
	int				numAudioLinks	(1);		//	Number of audio systems for multi-link audio
	int				useHDMIOut		(0);		//	Enable HDMI output?
	int				doMultiChannel	(0);		//  More than one instance of player 4k
	int				doRGBOnWire		(0);		//  Route the output to put RGB on the wire
	int				doTsiRouting	(0);		//  Route the output through the Tsi Muxes
	int				hdrType			(0);		//	Custom anc type?
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		#if !defined(NTV2_DEPRECATE_16_0)	//	--board option is deprecated!
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"(deprecated)"},
		#endif
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,		0,	"which video format to use",	"e.g. 'uhd24' or ? to list"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,		0,	"which pixel format to use",	"e.g. 'yuv8' or ? to list"},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,		0,	"which channel to use",			"number of the channel"},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,	0,	"use multi-channel/format",		AJA_NULL},
		{"audioLinks",	'a',	POPT_ARG_INT,		&numAudioLinks,		0,	"# audio systems to link",		"1-4 (0=silence)"},
		{"hdmi",		'h',	POPT_ARG_NONE,		&useHDMIOut,		0,	"enable HDMI output?",			AJA_NULL},
		{"rgb",			'r',	POPT_ARG_NONE,		&doRGBOnWire,		0,	"emit RGB over SDI?",			AJA_NULL},
		{"tsi",			't',	POPT_ARG_NONE,		&doTsiRouting,		0,	"use Tsi routing?",				AJA_NULL},
		{"hdrType",		'x',	POPT_ARG_INT,		&hdrType,			0,	"which HDR Packet to send",		"1:SDR,2:HDR10,3:HLG"},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}

	//	Device
	const string deviceSpec (pDeviceSpec ? pDeviceSpec : "0");
	Player8KConfig playerConfig(deviceSpec);

	//	VideoFormat
	const string videoFormatStr (pVideoFormat  ?  pVideoFormat  :  "");
	playerConfig.fVideoFormat = videoFormatStr.empty() ? NTV2_FORMAT_4x3840x2160p_2398 : CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr, VIDEO_FORMATS_8KUHD2);
	if (videoFormatStr == "?"  ||  videoFormatStr == "list")
	{	cout	<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_8KUHD2, deviceSpec) << endl;
		return 0;
	}
	else if (!videoFormatStr.empty()  &&  !NTV2_IS_8K_VIDEO_FORMAT(playerConfig.fVideoFormat))
	{	cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_8KUHD2, deviceSpec) << endl;
		return 2;
	}

	//	PixelFormat
	const string pixelFormatStr (pPixelFormat  ?  pPixelFormat  :  "");
	playerConfig.fPixelFormat = pixelFormatStr.empty() ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
	if (pixelFormatStr == "?"  ||  pixelFormatStr == "list")
	{	cout	<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 0;
	}
	else if (!pixelFormatStr.empty()  &&  !NTV2_IS_VALID_FRAME_BUFFER_FORMAT(playerConfig.fPixelFormat))
	{	cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	OutputChannel
	if (channelNumber < 1  ||  channelNumber > 8)
	{	cerr	<< "## ERROR:  Invalid channel number '" << channelNumber << "' -- expected 1 thru 8" << endl;
		return 2;
	}
	playerConfig.fOutputChannel = NTV2Channel(channelNumber ? channelNumber - 1 : 0);

	playerConfig.fDoHDMIOutput		= useHDMIOut ? true : false;
	playerConfig.fDoMultiFormat		= doMultiChannel ? true : false;
	playerConfig.fDoTsiRouting		= doTsiRouting ? true : false;
	playerConfig.fDoRGBOnWire		= doRGBOnWire ? true : false;
	playerConfig.fNumAudioLinks		= UWord(numAudioLinks);

	//	Anc / HDRType
	playerConfig.fTransmitHDRType	= hdrType == 1	? AJAAncillaryDataType_HDR_SDR
													: (hdrType == 2	? AJAAncillaryDataType_HDR_HDR10
																	: (hdrType == 3	? AJAAncillaryDataType_HDR_HLG
																					: AJAAncillaryDataType_Unknown));
	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	NTV2Player8K player (playerConfig);

	//	Initialize the player...
	if (AJA_FAILURE(player.Init()))
		{cerr << "## ERROR: Initialization failed" << endl;  return 3;}

	//	Run the player...
	player.Run();

	cout	<< "  Playout  Playout   Frames" << endl
			<< "   Frames   Buffer  Dropped" << endl;
	do
	{	//	Poll the player's status...
		AUTOCIRCULATE_STATUS outputStatus;
		player.GetACStatus(outputStatus);
		cout	<<	DECN(outputStatus.GetProcessedFrameCount(), 9)
				<<	DECN(outputStatus.GetBufferLevel(), 9)
				<<  DECN(outputStatus.GetDroppedFrameCount(), 9) << "\r" << flush;
		AJATime::Sleep(2000);
	} while (player.IsRunning() && !gGlobalQuit);	//	loop til done

	//  Ask the player to stop
	player.Quit();

	cout << endl;
	return 0;

}	//	main
