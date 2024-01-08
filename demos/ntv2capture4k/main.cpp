/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2capture4k/main.cpp
	@brief		Demonstration application to capture frames from SDI input.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2capture4k.h"
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
	int				channelNumber	(1);			//	Channel/FrameStore to use
	int				doMultiFormat	(0);			//	MultiFormat mode?
	int				showVersion		(0);			//	Show version?
	int				doQuadRouting	(0);			//	Quad/Square routing (i.e. not TSI)?
	int				numAudioLinks	(1);			//	Number of audio systems for multi-link audio
	AJADebug::Open();

	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",			"1-8"						},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",	AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"		},
		{"squares",		's',	POPT_ARG_NONE,		&doQuadRouting,	0,	"use quad routing?",		AJA_NULL					},
		{"audioLinks",	  0,	POPT_ARG_INT,		&numAudioLinks,	0,	"# multilink aud systems",	"0=silence or 1-4"			},
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

	CaptureConfig config(deviceSpec);

	//	Channel
	if ((channelNumber < 1)  ||  (channelNumber > 8))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 1;}
	config.fInputChannel = NTV2Channel(channelNumber - 1);

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

	//	Audio
	if (numAudioLinks < 0)
		{cerr << "## ERROR:  invalid '--audioLinks' value '" << numAudioLinks << "' -- negative" << endl;  return 1;}
	if (numAudioLinks > 4)
		{cerr << "## ERROR:  invalid '--audioLinks' value '" << numAudioLinks << "' -- exceeds 4" << endl;  return 1;}
	if (numAudioLinks != 1)
		config.fNumAudioLinks = UWord(numAudioLinks);

	config.fWithAudio		= config.fNumAudioLinks ? true : false;	//	Enable audio if numLinks > 0, disable if zero
	config.fDoTSIRouting	= !doQuadRouting;						//	TSI?
	config.fWithAnc			= true;									//	Always capture anc
	config.fDoMultiFormat	= doMultiFormat ? true : false;			//	Multiformat mode?

	//	Instantiate and initialize the NTV2Capture4K object...
	NTV2Capture4K capturer(config);
	AJAStatus status = capturer.Init();
	if (AJA_FAILURE(status))
		{cout << "## ERROR:  Initialization failed: " << ::AJAStatusToString(status) << endl;	return 1;}

	::signal (SIGINT, SignalHandler);
	#if defined(AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Run it...
	capturer.Run();

	cout	<< "   Frames   Frames   Buffer" << endl
			<< " Captured  Dropped    Level" << endl;
	do
	{	//	Poll its status until stopped...
		ULWord	framesProcessed, framesDropped, bufferLevel;
		capturer.GetACStatus (framesProcessed, framesDropped, bufferLevel);
		cout << setw(9) << framesProcessed << setw(9) << framesDropped << setw(9) << bufferLevel << "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop til done

	cout << endl;
	return 0;

}	//	main
