/**
	@file		ntv2llburn/main.cpp
	@brief		Demonstration application that "burns" timecode into frames captured from SDI input,
				and playout those modified frames to SDI output.
	@copyright	Copyright (C) 2012-2019 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2llburn.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
static bool	gGlobalQuit		(false);	///	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


/**
	@brief		Main entry point for 'ntv2llburn' demo application.
	@param[in]	argc	Number of arguments specified on the command line, including the path to the executable.
	@param[in]	argv	Array of arguments.
	@return		Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	AJAStatus		status			(AJA_STATUS_SUCCESS);		//	Result status
	char *			pDeviceSpec		(NULL);						//	Which device to use
	char *			pVidSource		(NULL);						//	Video input source string
	char *			pTcSource		(NULL);						//	Time code source string
	NTV2InputSource	vidSource		(NTV2_INPUTSOURCE_SDI1);	//	Video source
	NTV2TCIndex		tcSource		(NTV2_TCINDEX_SDI1);		//	Time code source
	int				noAudio			(0);						//	Disable audio?
	int				useRGB			(0);						//	Use 10-bit RGB instead of 8-bit YCbCr?
	int				doMultiChannel	(0);						//  Set the board up for multi-channel/format
	int				doAnc			(0);						//	Use the Anc Extractor/Inserter
	poptContext		optionsContext;								//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"index#, serial#, or model"	},
		{"input",		'i',	POPT_ARG_STRING,	&pVidSource,	0,	"video input",						"{'?' to list}"		},
		{"tcsource",	't',	POPT_ARG_STRING,	&pTcSource,		0,	"time code source",					"{'?' to list}"		},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",					NULL},
		{"rgb",			0,		POPT_ARG_NONE,		&useRGB,		0,	"use RGB10 frame buffer?",			NULL},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multichannel/multiformat?",	NULL},
		{"anc",			'a',	POPT_ARG_NONE,		&doAnc,			0,	"use Anc data extractor/inserter",	NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	if (::poptGetNextOpt (optionsContext) < -1)
	{
		cerr << "## ERROR:  Bad command line argument(s)" << endl;
		return 1;
	}
	optionsContext = ::poptFreeContext (optionsContext);
	const string	deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	const string	vidSourceStr	(pVidSource ? CNTV2DemoCommon::ToLower(pVidSource) : "");
	const string	tcSourceStr		(pTcSource ? CNTV2DemoCommon::ToLower(pTcSource) : "");

	const string	legalDevices(CNTV2DemoCommon::GetDeviceStrings(NTV2_DEVICEKIND_ALL));
	if (deviceSpec == "?" || deviceSpec == "list")
		{cout << legalDevices << endl;  return 0;}
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		{cout << "## ERROR:  No such device '" << deviceSpec << "'" << endl << legalDevices;  return 1;}

	//	Select video source...
	const string	legalSources(CNTV2DemoCommon::GetInputSourceStrings(NTV2_INPUTSOURCES_ALL, deviceSpec));
	if (vidSourceStr == "?" || vidSourceStr == "list")
		{cout << legalSources << endl;  return 0;}
	if (!vidSourceStr.empty())
	{
		vidSource = CNTV2DemoCommon::GetInputSourceFromString(vidSourceStr);
		if (!NTV2_IS_VALID_INPUT_SOURCE(vidSource))
			{cerr << "## ERROR:  Input source '" << vidSourceStr << "' not one of these:" << endl << legalSources << endl;	return 1;}
	}	//	if video source specified

	//	Select time code source...
	const string	legalTCSources(CNTV2DemoCommon::GetTCIndexStrings(TC_INDEXES_ALL, deviceSpec));
	if (tcSourceStr == "?" || tcSourceStr == "list")
		{cout << legalTCSources << endl;  return 0;}
	if (!tcSourceStr.empty())
	{
		tcSource = CNTV2DemoCommon::GetTCIndexFromString(tcSourceStr);
		if (!NTV2_IS_VALID_TIMECODE_INDEX(tcSource))
			{cerr << "## ERROR:  Timecode source '" << tcSourceStr << "' not one of these:" << endl << legalTCSources << endl;	return 1;}
	}

	//	Instantiate the NTV2Burn object...
	NTV2LLBurn	burner (pDeviceSpec ? pDeviceSpec : "0",					//	Which device?
						(noAudio ? false : true),							//	Include audio?
						useRGB ? NTV2_FBF_10BIT_RGB : NTV2_FBF_8BIT_YCBCR,	//	Use RGB frame buffer format?
						vidSource,											//	Which video input source?
						tcSource,											//	Which time code source?
						doMultiChannel ? true : false,						//  Set the board up for multi-channel/format
						doAnc ? true : false);								//	Use the Anc Extractor/Inserter

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2Burn instance...
	status = burner.Init ();
	if (AJA_SUCCESS (status))
	{
		//	Start the burner's capture and playout threads...
		burner.Run ();

		//	Loop until told to stop...
		cout	<< "   Frames   Frames" << endl
				<< "Processed  Dropped" << endl;
		while (!gGlobalQuit)
		{
			ULWord	framesProcessed, framesDropped;
			burner.GetStatus (framesProcessed, framesDropped);
			cout	<< setw (9) << framesProcessed
					<< setw (9) << framesDropped
					<< "\r" << flush;
			AJATime::Sleep (2000);
		}	//	loop until signaled

		cout << endl;
	}
	else
		cerr << "## ERROR:  Initialization failed, status=" << status << endl;

	return AJA_SUCCESS (status) ? 0 : 2;	//	Return zero upon success -- otherwise 2

}	//	main
