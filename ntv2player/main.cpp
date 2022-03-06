/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2player/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout frames to SDI output
				generated in host memory containing test pattern and timecode, including audio tone.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ntv2utils.h"
#include "ntv2player.h"
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
	char *			pFramesSpec		(AJA_NULL);	//	AutoCirculate frames spec
	char *			pAncFilePath	(AJA_NULL);	//	Anc data filepath
	uint32_t		channelNumber	(1);		//	Number of the channel to use
	int				noAudio			(0);		//	Disable audio tone?
	int				doMultiChannel	(0);		//	Enable multi-format?
	int				hdrType			(0);		//	Transmit HDR anc?
	int				xmitLTC			(0);		//	Use LTC? (Defaults to VITC)
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		#if !defined(NTV2_DEPRECATE_16_0)	//	--board option is deprecated!
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",		"(deprecated)"},
		#endif
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",		"index#, serial#, or model"},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"video format to use",		"'?' or 'list' to list"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"},
		{"frames",		0,		POPT_ARG_STRING,	&pFramesSpec,	0,	"frames to AutoCirculate",	"num[@min] or min-max"},
		{"anc",			'a',	POPT_ARG_STRING,	&pAncFilePath,	0,	"playout prerecorded anc",	"path/to/binary/data/file"},
		{"hdrType",		't',	POPT_ARG_INT,		&hdrType,		0,	"HDR pkt to send",			"0=none 1=SDR 2=HDR10 3=HLG"},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",			"1 thru 8"},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multi-channel/format",	AJA_NULL},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio tone",		AJA_NULL},
		{"ltc",			'l',	POPT_ARG_NONE,		&xmitLTC,		0,	"xmit LTC instead of VITC",	AJA_NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}

	//	Device
	const string deviceSpec (pDeviceSpec ? pDeviceSpec : "0");
	PlayerConfig playerConfig(deviceSpec);

	//	VideoFormat
	const string videoFormatStr (pVideoFormat  ?  pVideoFormat  :  "");
	playerConfig.fVideoFormat = videoFormatStr.empty() ? NTV2_FORMAT_1080i_5994 : CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr);
	if (videoFormatStr == "?"  ||  videoFormatStr == "list")
	{	cout	<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_NON_4KUHD, deviceSpec) << endl;
		return 0;
	}
	else if (!videoFormatStr.empty()  &&  playerConfig.fVideoFormat == NTV2_FORMAT_UNKNOWN)
	{	cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_NON_4KUHD, deviceSpec) << endl;
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

	//	Frames
	const string framesSpec(pFramesSpec ? pFramesSpec : "");
	static const string	legalFramesSpec("{frameCount}[@{firstFrameNum}]  or  {firstFrameNum}-{lastFrameNum}");
	if (!framesSpec.empty())
	{	const string parseResult(playerConfig.fFrames.setFromString(framesSpec));
		if (!parseResult.empty())
		{	cerr	<< "## ERROR:  Bad 'frames' spec '" << framesSpec << "'" << endl
					<< "## " << parseResult << endl;
			return 1;
		}
	}
	if (!playerConfig.fFrames.valid())
	{	cerr	<< "## ERROR:  Bad 'frames' spec '" << framesSpec << "'" << endl
				<< "## Expected " << legalFramesSpec << endl;
		return 1;
	}

	playerConfig.fOutputDestination	= ::NTV2ChannelToOutputDestination(playerConfig.fOutputChannel);
	playerConfig.fSuppressAudio		= noAudio ? true : false;
	playerConfig.fDoMultiFormat		= doMultiChannel ? true : false;
	playerConfig.fTransmitLTC		= xmitLTC ? true : false;

	//	Anc / HDRType
	playerConfig.fTransmitHDRType	= hdrType == 1	? AJAAncillaryDataType_HDR_SDR
													: (hdrType == 2	? AJAAncillaryDataType_HDR_HDR10
																	: (hdrType == 3	? AJAAncillaryDataType_HDR_HLG
																					: AJAAncillaryDataType_Unknown));
	if (pAncFilePath)
		playerConfig.fAncDataFilePath = pAncFilePath;
	if (playerConfig.fTransmitHDRType != AJAAncillaryDataType_Unknown  &&  !playerConfig.fAncDataFilePath.empty())
	{	cerr	<< "## ERROR:  conflicting options '--hdrType' and '--anc'" << endl;
		return 2;
	}

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	NTV2Player	player (playerConfig);

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
