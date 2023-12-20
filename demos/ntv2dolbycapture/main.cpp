/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dolbycapture/main.cpp
	@brief		Demonstration application to capture frames from SDI input.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2dolbycapture.h"
#include "ajabase/system/process.h"
#include <signal.h>

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
	int			showVersion		(0);			//	Show version?
	char *		pDeviceSpec		(AJA_NULL);		//	Device specifier string, if any
	char *		pPixelFormat	(AJA_NULL);		//	Pixel format argument
	char *		pFramesSpec		(AJA_NULL);		//	AutoCirculate frames spec
	char *		pInputSrcSpec	(AJA_NULL);		//	SDI source spec
	uint32_t	channelNumber	(1);			//	Channel/FrameStore to use
	int			doMultiFormat	(0);			//	Enable multi-format?
	int			doRecordAnc		(0);			//	Record anc
	int			doRecordAudio	(0);			//	Record audio
	int			doRecordDolby	(0);			//	Record dolby
	int			doAudioFilter	(0);			//	Enable anc audio filter?
	int			doFrameData		(0);			//	Output per frame data?

	AJAStatus	status			(AJA_STATUS_SUCCESS);
	AJADebug::Open();

	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version",				AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",			"1-8"						},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",	AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"		},
		{"frames",		0,		POPT_ARG_STRING,	&pFramesSpec,	0,	"frames to AutoCirculate",	"num[@min] or min-max"		},
		{"input",		'i',	POPT_ARG_STRING,	&pInputSrcSpec,	0,	"which HDMI input",			"?=list"					},
		{"anc",			'a',	POPT_ARG_NONE,		&doRecordAnc,	0,	"record all aux to file",	AJA_NULL					},
		{"audio",		'f',	POPT_ARG_NONE,		&doRecordAudio,	0,	"record aux audio only to file",	AJA_NULL			},
		{"dolby",		'g',	POPT_ARG_NONE,		&doRecordDolby,	0,	"record dolby to file",		AJA_NULL					},
		{"audioFilter",	'x',	POPT_ARG_NONE,		&doAudioFilter,	0,	"only capture audio anc",	AJA_NULL					},
		{"frameData",	'y',	POPT_ARG_NONE,		&doFrameData,	0,	"show per-frame dolby stats",		AJA_NULL			},

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

	DolbyCaptureConfig config(deviceSpec);

	//	Channel
	if ((channelNumber < 1)  ||  (channelNumber > 8))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 1;}
	config.fInputChannel = NTV2Channel(channelNumber - 1);

	//	Input source
	const string legalSources (CNTV2DemoCommon::GetInputSourceStrings(NTV2_IOKINDS_HDMI, deviceSpec));
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
	const string pixelFormatStr	(pPixelFormat  ? pPixelFormat :  "");
	config.fPixelFormat = pixelFormatStr.empty() ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
	if (pixelFormatStr == "?"  ||  pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty()  &&  !NTV2_IS_VALID_FRAME_BUFFER_FORMAT(config.fPixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	AutoCirculate Frames
	static const string	legalFramesSpec("{frameCount}[@{firstFrameNum}]  or  {firstFrameNum}-{lastFrameNum}");
	const string framesSpec (pFramesSpec ? pFramesSpec : "");
	if (!framesSpec.empty())
	{
		const string parseResult(config.fFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## " << parseResult << endl;  return 1;}
	}
	if (!config.fFrames.valid())
		{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## Expected " << legalFramesSpec << endl;  return 1;}

	string			recordAncFile	("disabled");
	string			recordAudioFile	("disabled");
	string			recordDolbyFile	("disabled");

	//	Anc Capture
	if (doRecordAnc)
	{	// invent a file name...
		ostringstream fileName;  fileName << "ntv2dolbycapture-" << deviceSpec << "-"
				<< ::NTV2ChannelToString(config.fInputChannel,true) << "-" << AJAProcess::GetPid() << ".anc";
		recordAncFile = fileName.str();
	}

	//	Audio Capture
	if (doRecordAudio)
	{	// invent a file name...
		ostringstream fileName;  fileName << "ntv2dolbycapture-" << deviceSpec << "-"
				<< ::NTV2ChannelToString(config.fInputChannel,true) << "-" << AJAProcess::GetPid() << ".raw";
		recordAudioFile = fileName.str();
	}

	//	Dolby Capture
	if (doRecordDolby)
	{	// invent a file name...
		ostringstream fileName;  fileName << "ntv2dolbycapture-" << deviceSpec << "-"
				<< ::NTV2ChannelToString(config.fInputChannel,true) << "-" << AJAProcess::GetPid() << ".ec3";
		recordDolbyFile = fileName.str();
	}

	config.fAncDataFilePath     = recordAncFile;
	config.fAudioDataFilePath   = recordAudioFile;
	config.fDolbyDataFilePath	= recordDolbyFile;
	config.fDoAudioFilter       = doAudioFilter ? true : false;
	config.fDoFrameData			= doFrameData ? true : false;
	config.fDoMultiFormat       = doMultiFormat ? true : false;
	config.fWithAnc             = doRecordAnc ? true : false;
	config.fWithAudio           = doRecordAudio ? true : false;
	config.fWithDolby           = doRecordDolby ? true : false;

	//	Instantiate and initialize the ntv2dolbycapture object
	NTV2DolbyCapture	capturer(config);
	status = capturer.Init();
	if (AJA_FAILURE(status))
		{cout << "## ERROR:  Initialization failed: " << ::AJAStatusToString(status) << endl;	return 1;}

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Run the capturer...
	capturer.Run();

	cout	<< "   Frames   Frames   Buffer" << endl
			<< " Captured  Dropped    Level" << endl;
	do
	{
	//	Poll its status until stopped...
		ULWord	framesProcessed, framesDropped, bufferLevel;
		capturer.GetACStatus (framesProcessed, framesDropped, bufferLevel);
		cout << setw(9) << framesProcessed << setw(9) << framesDropped << setw(9) << bufferLevel << "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop til quit time

	cout << endl;
	return 0;

}	//	main
