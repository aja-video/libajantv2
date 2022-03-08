/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2llburn/main.cpp
	@brief		Demonstration application that "burns" timecode into frames captured from SDI input,
				and playout those modified frames to SDI output.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
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
	char *			pDeviceSpec		(AJA_NULL);	//	Which device to use
	char *			pPixelFormat	(AJA_NULL);	//	Pixel format spec
	char *			pVidSource		(AJA_NULL);	//	Video input source string
	char *			pTcSource		(AJA_NULL);	//	Time code source string
	int				noAudio			(0);		//	Disable audio?
	int				doMultiChannel	(0);		//  Set the board up for multi-channel/format
	int				doAnc			(0);		//	Use the Anc Extractor/Inserter
	int				doHanc			(0);		//	Use the Anc Extractor/Inserter with Audio
	poptContext		optionsContext;				//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		#if !defined(NTV2_DEPRECATE_16_0)	//	--board option is deprecated!
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"(deprecated)"	},
		#endif
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"index#, serial#, or model"},
		{"input",		'i',	POPT_ARG_STRING,	&pVidSource,	0,	"video input",						"{'?' to list}"},
		{"tcsource",	't',	POPT_ARG_STRING,	&pTcSource,		0,	"time code source",					"{'?' to list}"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format",						"'?' or 'list' to list"},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",					AJA_NULL},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multichannel/multiformat?",	AJA_NULL},
		{"anc",			'a',	POPT_ARG_NONE,		&doAnc,			0,	"use Anc data extractor/inserter",	AJA_NULL},
		{"hanc",		'h',	POPT_ARG_NONE,		&doHanc,		0,	"use Anc data extractor/inserter with audio",	AJA_NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
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
	NTV2InputSource	vidSource (NTV2_INPUTSOURCE_SDI1);	//	Video source
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
	NTV2TCIndex		tcSource (NTV2_TCINDEX_SDI1);	//	Time code source
	const string	legalTCSources(CNTV2DemoCommon::GetTCIndexStrings(TC_INDEXES_ALL, deviceSpec));
	if (tcSourceStr == "?" || tcSourceStr == "list")
		{cout << legalTCSources << endl;  return 0;}
	if (!tcSourceStr.empty())
	{
		tcSource = CNTV2DemoCommon::GetTCIndexFromString(tcSourceStr);
		if (!NTV2_IS_VALID_TIMECODE_INDEX(tcSource))
			{cerr << "## ERROR:  Timecode source '" << tcSourceStr << "' not one of these:" << endl << legalTCSources << endl;	return 1;}
	}

	//	Pixel format
	NTV2PixelFormat	pixelFormat		(NTV2_FBF_8BIT_YCBCR);
	const string	pixelFormatStr	(pPixelFormat  ? pPixelFormat :  "1");
	const string	legalFBFs		(CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	if (!pixelFormatStr.empty())
	{
		pixelFormat = CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
		if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT(pixelFormat))
			{cerr << "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl << legalFBFs << endl;  return 2;}
	}

	//	Instantiate the NTV2Burn object...
	NTV2LLBurn	burner (deviceSpec,						//	Which device?
						(noAudio ? false : true),		//	Include audio?
						pixelFormat,					//	Frame buffer format
						vidSource,						//	Video input source
						tcSource,						//	Timecode source
						doMultiChannel ? true : false,	//  Multi-channel/format?
						doAnc ? true : false,			//	Use Anc Extractor/Inserter?
						doHanc? true : false);			//	Use Hanc Extractor/Inserter?

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2Burn instance...
	if (AJA_FAILURE(burner.Init()))
		{cerr << "## ERROR:  Initialization failed" << endl;  return 1;}

	//	Start the burner's capture and playout threads...
	burner.Run();

	//	Loop until told to stop...
	cout	<< "   Frames   Frames" << endl
			<< "Processed  Dropped" << endl;
	while (!gGlobalQuit)
	{
		ULWord	framesProcessed, framesDropped;
		burner.GetStatus (framesProcessed, framesDropped);
		cout	<< setw(9) << framesProcessed
				<< setw(9) << framesDropped
				<< "\r" << flush;
		AJATime::Sleep(2000);
	}	//	loop until signaled

	cout << endl;
	return 0;

}	//	main
