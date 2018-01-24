/**
	@file		ntv2fieldburn/main.cpp
	@brief		Demonstration application to capture frames from the SDI input as two distinct fields
				in separate, non-contiguous memory locations, "burn" a timecode window into each field,
				and recombine the modified fields for SDI playout.
	@copyright	Copyright (C) 2013-2018 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2fieldburn.h"
#include "ajabase/system/systemtime.h"
#include <signal.h>
#include <iostream>
#include <iomanip>


//	Globals
static bool	gGlobalQuit		(false);	///< @brief	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	AJAStatus	status			(AJA_STATUS_SUCCESS);	//	Result status
	char *		pDeviceSpec 	(NULL);					//	Which device to use
	uint32_t	inputNumber		(1);					//	Number of the input to use (1-8, defaults to 1)
	int			noAudio			(0);					//	Disable audio?
	int			useRGB			(0);					//	Use 10-bit RGB instead of 8-bit YCbCr?
	int			doMultiChannel	(0);					//  Set the board up for multi-channel/format
	poptContext	optionsContext;							//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#, serial#, or model"	},
		{"input",		'i',	POPT_ARG_INT,		&inputNumber,	0,	"which SDI input to use",		"1 - 8"						},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disables audio",				NULL						},
		{"rgb",			0,		POPT_ARG_NONE,		&useRGB,		0,	"use RGB10 instead of YUV8?",	NULL						},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multichannel/multiformat?",	NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	if (inputNumber > 8 || inputNumber < 1)
		{cerr << "## ERROR:  Input '" << inputNumber << "' not 1 thru 8" << endl;	return 1;}

	//	Instantiate our NTV2FieldBurn object...
	NTV2FieldBurn	burner (pDeviceSpec ? string (pDeviceSpec) : "0",			//	Which device?
							(noAudio ? false : true),							//	Include audio?
							useRGB ? NTV2_FBF_10BIT_RGB : NTV2_FBF_8BIT_YCBCR,	//	Use RGB frame buffer format?
							::GetNTV2InputSourceForIndex (inputNumber - 1),		//	Which input source?
							doMultiChannel ? true : false);						//  Set the device up for multi-channel/format?

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2FieldBurn instance...
	status = burner.Init ();
	if (AJA_SUCCESS (status))
	{
		ULWord	totalFrames (0),  inputDrops (0),  outputDrops (0),  inputBufferLevel (0), outputBufferLevel (0);

		//	Start the burner's capture and playout threads...
		burner.Run ();

		//	Loop until someone tells us to stop...
		cout	<< "           Capture  Playout  Capture  Playout" << endl
				<< "   Frames   Frames   Frames   Buffer   Buffer" << endl
				<< "Processed  Dropped  Dropped    Level    Level" << endl;
		do
		{
			burner.GetStatus (totalFrames, inputDrops, outputDrops, inputBufferLevel, outputBufferLevel);
			cout	<< setw (9) << totalFrames << setw (9) << inputDrops << setw (9) << outputDrops << setw (9) << inputBufferLevel
					<< setw (9) << outputBufferLevel << "\r" << flush;
			AJATime::Sleep (500);
		} while (gGlobalQuit == false);	//	loop until signaled

		cout << endl;

	}	//	if Init succeeded
	else
		cout << "Burn initialization failed with status " << status << endl;

	return 1;

}	//	main
