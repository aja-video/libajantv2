/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2llburn/main.cpp
	@brief		Demonstration application that "burns" timecode into frames captured from SDI input,
				and playout those modified frames to SDI output.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
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
	char *			pDeviceSpec		(AJA_NULL);		//	Which device to use
	char *			pInputSrcSpec	(AJA_NULL);		//	SDI source spec
	char *			pTcSource		(AJA_NULL);		//	Time code source string
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format spec
	int				doMultiFormat	(0);			//	MultiFormat mode?
	int				showVersion		(0);			//	Show version?
	int				noAudio			(0);			//	Disable audio?
	int				doAnc			(0);			//	Use the Anc Extractor/Inserter
	int				doHanc			(0);			//	Use the Anc Extractor/Inserter with Audio
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",			AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",				"index#, serial#, or model"	},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",		AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",			"'?' or 'list' to list"		},
		{"input",		'i',	POPT_ARG_STRING,	&pInputSrcSpec,	0,	"SDI input to use",				"1-8, ?=list"				},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",				AJA_NULL					},
		{"anc",			'a',	POPT_ARG_NONE,		&doAnc,			0,	"use Anc ext/ins",				AJA_NULL					},
		{"hanc",		'h',	POPT_ARG_NONE,		&doHanc,		0,	"use Anc ext/ins with audio",	AJA_NULL					},
		{"tcsource",	't',	POPT_ARG_STRING,	&pTcSource,		0,	"time code source",				"'?' to list"				},
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

	BurnConfig config(deviceSpec);

	//	Input source
	const string	legalSources(CNTV2DemoCommon::GetInputSourceStrings(NTV2_IOKINDS_ALL, deviceSpec));
	const string inputSourceStr	(pInputSrcSpec ? CNTV2DemoCommon::ToLower(string(pInputSrcSpec)) : "");
	if (inputSourceStr == "?" || inputSourceStr == "list")
		{cout << legalSources << endl;  return 0;}
	if (!inputSourceStr.empty())
	{
		config.fInputSource = CNTV2DemoCommon::GetInputSourceFromString(inputSourceStr);
		if (!NTV2_IS_VALID_INPUT_SOURCE(config.fInputSource))
			{cerr << "## ERROR:  Input source '" << inputSourceStr << "' not one of:" << endl << legalSources << endl;	return 1;}
	}	//	if input source specified

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

	//	Timecode source...
	const string	legalTCSources(CNTV2DemoCommon::GetTCIndexStrings(TC_INDEXES_ALL, deviceSpec));
	const string	tcSourceStr		(pTcSource ? CNTV2DemoCommon::ToLower(pTcSource) : "");
	if (tcSourceStr == "?"  ||  tcSourceStr == "list")
		{cout << legalTCSources << endl;  return 0;}
	if (!tcSourceStr.empty())
	{
		config.fTimecodeSource = CNTV2DemoCommon::GetTCIndexFromString(tcSourceStr);
		if (!NTV2_IS_VALID_TIMECODE_INDEX(config.fTimecodeSource))
			{cerr << "## ERROR:  Timecode source '" << tcSourceStr << "' not one of these:" << endl << legalTCSources << endl;	return 1;}
	}

	config.fDoMultiFormat	= doMultiFormat ? true : false;
	config.fSuppressAudio	= noAudio ? true  : false;
	config.fWithAnc			= doAnc   ? true  : false;
	config.fWithHanc		= doHanc  ? true  : false;

	//	Instantiate the NTV2LLBurn object...
	NTV2LLBurn burner (config);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2LLBurn instance...
	AJAStatus status (burner.Init());
	if (AJA_FAILURE (status))
		{cerr << "## ERROR:  Initialization failed, status=" << status << endl;  return 4;}

	//	Start the burner's capture and playout threads...
	burner.Run();

	//	Loop until told to stop...
	cout	<< "   Frames   Frames" << endl
			<< "Processed  Dropped" << endl;
	do
	{
		ULWord	framesProcessed, framesDropped;
		burner.GetStatus (framesProcessed, framesDropped);
		cout	<< setw(9) << framesProcessed
				<< setw(9) << framesDropped
				<< "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;
	return 0;

}	//	main
