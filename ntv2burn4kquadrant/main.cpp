/**
	@file		ntv2burn4kquadrant/main.cpp
	@brief		Demonstration application to "burn" timecode into frames captured from SDI input,
				and play out the modified frames to SDI output.
	@copyright	Copyright (C) 2012-2017 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2burn4kquadrant.h"
#include <signal.h>
#include <iostream>
#include <iomanip>


//	Globals
static bool	gGlobalQuit		(false);	//	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	AJAStatus	status				(AJA_STATUS_SUCCESS);	//	Result status
	char *		pInputDeviceSpec 	(NULL);					//	Which device to use for capture
	char *		pOutputDeviceSpec 	(NULL);					//	Which device to use for playout
	char *		pTcSource			(NULL);					//	Time code source string
	int			noAudio				(0);					//	Disable audio?
	int			useRGB				(0);					//	Use 10-bit RGB instead of 8-bit YCbCr?
	poptContext	optionsContext;								//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"input",	'i',	POPT_ARG_STRING,	&pInputDeviceSpec,	0,	"input device",			"index#, serial#, or model"			},
		{"output",	'o',	POPT_ARG_STRING,	&pOutputDeviceSpec,	0,	"output device",		"index#, serial#, or model"			},
		{"tcsource",'t',	POPT_ARG_STRING,	&pTcSource,			0,	"time code source",		"sdi[1-4] | sltc[1-2] | ltc[1-2]"	},
		{"noaudio",	0,		POPT_ARG_NONE,		&noAudio,			0,	"disable audio?",		NULL								},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	//	Pick timecode source...
	const string	tcSourceStr	(CNTV2DemoCommon::ToLower (pTcSource ? pTcSource : ""));
	NTV2TCIndex		tcSource	(NTV2_TCINDEX_SDI1);

	if (tcSourceStr.empty ())			tcSource = NTV2_TCINDEX_SDI1;
	else if (tcSourceStr == "sdi1")		tcSource = NTV2_TCINDEX_SDI1;
	else if (tcSourceStr == "sdi2")		tcSource = NTV2_TCINDEX_SDI2;
	else if (tcSourceStr == "sdi3")		tcSource = NTV2_TCINDEX_SDI3;
	else if (tcSourceStr == "sdi4")		tcSource = NTV2_TCINDEX_SDI4;
	else if (tcSourceStr == "sltc1")	tcSource = NTV2_TCINDEX_SDI1_LTC;
	else if (tcSourceStr == "sltc2")	tcSource = NTV2_TCINDEX_SDI2_LTC;
	else if (tcSourceStr == "ltc1")		tcSource = NTV2_TCINDEX_LTC1;
	else if (tcSourceStr == "ltc2")		tcSource = NTV2_TCINDEX_LTC2;
	else
		{cerr << "## ERROR:  Invalid timecode source '" << tcSourceStr << "' -- expected sdi[1-4]|sltc[1-2]|ltc[1-2]" << endl;  return 2;}

	//	Instantiate our NTV2Burn4KQuadrant object...
	NTV2Burn4KQuadrant	burner (pInputDeviceSpec ? pInputDeviceSpec : "0",			//	Which device will be the input device?
								pOutputDeviceSpec ? pOutputDeviceSpec : "1",		//	Which device will be the output device?
								noAudio ? false : true,								//	Include audio?
								useRGB ? NTV2_FBF_10BIT_RGB : NTV2_FBF_8BIT_YCBCR,	//	Use RGB frame buffer format?
								tcSource);											//	Which time code source?				

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2Burn4KQuadrant instance...
	status = burner.Init ();
	if (AJA_SUCCESS (status))
	{
		AUTOCIRCULATE_STATUS	inputStatus;
		AUTOCIRCULATE_STATUS	outputStatus;

		//	Start the burner's capture and playout threads...
		burner.Run ();

		//	Loop until someone tells us to stop...
		cout	<< "           Capture  Playout  Capture  Playout" << endl
				<< "   Frames   Frames   Frames   Buffer   Buffer" << endl
				<< "Processed  Dropped  Dropped    Level    Level" << endl;
		do
		{
			burner.GetACStatus (inputStatus, outputStatus);

			cout	<< setw (9) << inputStatus.acFramesProcessed
					<< setw (9) << inputStatus.acFramesDropped
					<< setw (9) << outputStatus.acFramesDropped
					<< setw (9) << inputStatus.acBufferLevel
					<< setw (9) << outputStatus.acBufferLevel
					<< "\r" << flush;

			AJATime::Sleep (500);

		} while (!gGlobalQuit);	//	loop until signaled

		cout << endl;

	}	//	if Init succeeded
	else
		cout << "Burn initialization failed with status " << status << endl;

	return 1;

}	//	main
