/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2capture4k/main.cpp
	@brief		Demonstration application to capture frames from SDI input.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2utils.h"
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/systemtime.h"
#include "ntv2capture4k.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
static bool	gGlobalQuit		(false);	//	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	char *			pPixelFormat	(AJA_NULL);				//	Pixel format argument
	char *			pDeviceSpec		(AJA_NULL);				//	Device specifier string, if any
	uint32_t		channelNumber	(1);					//	Number of the channel to use
	int				numAudioLinks	(1);		//	Number of audio systems for multi-link audio
	int				doMultiFormat	(0);					//	Multi-format mode?
	int				doSquareRouting	(0);					//	Square routing?
	poptContext		optionsContext; 						//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		#if !defined(NTV2_DEPRECATE_16_0)	//	--board option is deprecated!
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"(deprecated)"	},
		#endif
		{"audioLinks",	'a',	POPT_ARG_INT,		&numAudioLinks,		0,	"how many audio systems to control for multi-link audio",	"0=silence or 1-4"},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#, serial#, or model"		},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"which pixel format to use",	"'?' or 'list' to list"			},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,	0,	"which channel to use",			"1 thru 8"						},
		{"multiFomat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"Configure multi-format",		AJA_NULL						},
		{"squares",		's',	POPT_ARG_NONE,		&doSquareRouting,	0,	"use Square routing?",				AJA_NULL						},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	const string				deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
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
	
	int	doTsiRouting	(1);		//  default route the output through the Tsi Muxes
	if (doSquareRouting)
		doTsiRouting = 0;

	if (channelNumber < 1 || channelNumber > 8)
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 2;}

	//	Instantiate the NTV2Capture object, using the specified AJA device...
	NTV2Capture4K	capturer (pDeviceSpec ? string (pDeviceSpec) : "0",
							  numAudioLinks,					//	Multi-link, 1 channel or 0 channels
							  ::GetNTV2ChannelForIndex(channelNumber - 1),	//	Channel
							  pixelFormat,						//	FB pixel format
							  false,							//	A/B conversion?
							  doMultiFormat ? true : false,		//	MultiFormat mode?
							  true,								//	With custom anc?
							  doTsiRouting ? true : false);		//	TSI?

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2Capture instance...
	if (AJA_FAILURE(capturer.Init()))
		{cerr << "## ERROR:  Initialization failed" << endl;  return 1;}

	//	Run the capturer...
	capturer.Run();

	cout	<< "           Capture  Capture" << endl
			<< "   Frames   Frames   Buffer" << endl
			<< "Processed  Dropped    Level" << endl;
	//	Poll its status until stopped...
	do
	{
		ULWord	framesProcessed, framesDropped, bufferLevel;
		capturer.GetACStatus (framesProcessed, framesDropped, bufferLevel);
		cout << setw(9) << framesProcessed << setw(9) << framesDropped << setw(9) << bufferLevel << "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop til quit time

	cout << endl;
	return 0;

}	//	main
