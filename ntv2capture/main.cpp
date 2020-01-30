/**
	@file		ntv2capture/main.cpp
	@brief		Demonstration application to capture frames from SDI input.
	@copyright	(C) 2012-2020 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2utils.h"
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/systemtime.h"
#include "ntv2capture.h"
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
	AJAStatus		status			(AJA_STATUS_SUCCESS);
	char *			pDeviceSpec		(AJA_NULL);				//	Device specifier string, if any
	char *			pInputSrcSpec	(AJA_NULL);				//	SDI source spec
	char *			pPixelFormat	(AJA_NULL);				//	Pixel format argument
	char *			pFramesSpec		(AJA_NULL);				//	AutoCirculate frames spec
	uint32_t		channelNumber	(1);					//	Channel/FrameStore to use
	int				noAudio			(0);					//	Disable audio capture?
	int				doMultiFormat	(0);					//	Enable multi-format?
	int				doAnc			(0);					//	Enable anc capture?
	poptContext		optionsContext; 						//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#, serial#, or model"	},
		{"channel",	    'c',	POPT_ARG_INT,		&channelNumber,	0,	"which channel to use",			"1 thru 8"					},
		{"input",		'i',	POPT_ARG_STRING,	&pInputSrcSpec,	0,	"which SDI input",				"1-8, ?=list"				},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"which pixel format to use",	"'?' or 'list' to list"		},
		{"frames",		0,		POPT_ARG_STRING,	&pFramesSpec,	0,	"frames to AutoCirc",			"num[@min] or min-max"		},
		{"multiFomat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"Configure multi-format",		AJA_NULL					},
		{"noaudio",		 0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio capture",		AJA_NULL					},
		{"anc",			'a',	POPT_ARG_NONE,		&doAnc,			0,	"enable anc capture",			AJA_NULL					},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext(optionsContext);

	const string	deviceSpec		(pDeviceSpec   ? pDeviceSpec : "0");
	const string	inputSourceStr	(pInputSrcSpec ? CNTV2DemoCommon::ToLower(string(pInputSrcSpec)) : "");
	const string	pixelFormatStr	(pPixelFormat  ? pPixelFormat :  "");
	const string	framesSpec		(pFramesSpec   ? pFramesSpec  :  "");

	//	Device
	const string	legalDevices(CNTV2DemoCommon::GetDeviceStrings());
	if (deviceSpec == "?" || deviceSpec == "list")
		{cout << legalDevices << endl;  return 0;}
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		{cout << "## ERROR:  No such device '" << deviceSpec << "'" << endl << legalDevices;  return 1;}

	CaptureConfig config(deviceSpec);

	//	Channel
	if (channelNumber < 1  ||  channelNumber > 8)
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 1;}
	config.fInputChannel = NTV2Channel(channelNumber - 1);

	//	Input source
	const string legalSources(CNTV2DemoCommon::GetInputSourceStrings(NTV2_INPUTSOURCES_SDI, deviceSpec));
	if (inputSourceStr == "?" || inputSourceStr == "list")
		{cout << legalSources << endl;  return 0;}
	if (!inputSourceStr.empty())
	{
		config.fInputSource = CNTV2DemoCommon::GetInputSourceFromString(inputSourceStr);
		if (!NTV2_IS_VALID_INPUT_SOURCE(config.fInputSource))
			{cerr << "## ERROR:  Input source '" << inputSourceStr << "' not one of:" << endl << legalSources << endl;	return 1;}
	}	//	if input source specified

	//	Pixel Format
	const NTV2PixelFormat	pixelFormat		(pixelFormatStr.empty() ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr));
	if (pixelFormatStr == "?"  ||  pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty()  &&  !NTV2_IS_VALID_FRAME_BUFFER_FORMAT(pixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	AutoCirculate Frames
	static const string	legalFramesSpec("{frameCount}[@{firstFrameNum}]  or  {firstFrameNum}-{lastFrameNum}");
	if (!framesSpec.empty())
	{
		const string parseResult(config.fFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## " << parseResult << endl;  return 1;}
	}
	if (!config.fFrames.valid())
		{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## Expected " << legalFramesSpec << endl;  return 1;}

	//	Instantiate the NTV2Capture object, using the specified AJA device...
	config.fDeviceSpec		= pDeviceSpec ? string(pDeviceSpec) : "0";
	config.fPixelFormat		= pixelFormat;
	config.fABConversion	= false;
	config.fDoMultiFormat	= doMultiFormat ? true : false;
	config.fWithAnc			= doAnc ? true : false;
	config.fWithAudio		= noAudio ? false : true;
	//cerr << "Specified Configuration:" << endl << config << endl;

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	NTV2Capture	capturer(config);

	//	Initialize the NTV2Capture instance...
	status = capturer.Init();
	if (!AJA_SUCCESS(status))
		{cout << "## ERROR:  Capture initialization failed with status " << status << endl;	return 1;}

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

	return AJA_SUCCESS(status) ? 0 : 1;

}	//	main
