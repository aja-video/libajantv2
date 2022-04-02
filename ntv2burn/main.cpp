/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn/main.cpp
	@brief		Demonstration application that "burns" timecode into frames captured from SDI input,
				and playout those modified frames to SDI output.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2burn.h"
#include "ajabase/system/systemtime.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
static bool	gGlobalQuit		(false);	///< @brief	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


/**
	@brief		Main entry point for 'ntv2burn' demo application.
	@param[in]	argc	Number arguments specified on the command line, including the path to the executable.
	@param[in]	argv	Array of 'const char' pointers, one for each argument.
	@return		Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	AJAStatus		status			(AJA_STATUS_SUCCESS);	//	Result status
	char *			pDeviceSpec		(AJA_NULL);				//	Which device to use
	char *			pVidSource		(AJA_NULL);				//	Video input source string
	char *			pTcSource		(AJA_NULL);				//	Time code source string
	char *			pPixelFormat	(AJA_NULL);				//	Pixel format spec
	char *			pInFramesSpec	(AJA_NULL);				//	Input AutoCirculate frames spec
	char *			pOutFramesSpec	(AJA_NULL);				//	Output AutoCirculate frames spec
	int				noAudio			(0);					//	Disable audio?
	int				noVideo			(0);					//	Disable video?
	int				noAnc			(0);					//	Disable use of Anc Extractor/Inserter?
	int				doMultiChannel	(0);					//  Set the board up for multi-channel/format
	poptContext		optionsContext;							//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		#if !defined(NTV2_DEPRECATE_16_0)	//	--board option is deprecated!
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"(deprecated)"	},
		#endif
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",			"index#, serial#, or model"	},
		{"input",		'i',	POPT_ARG_STRING,	&pVidSource,	0,	"video input",					"{'?' to list}"		},
		{"tcsource",	't',	POPT_ARG_STRING,	&pTcSource,		0,	"time code source",				"{'?' to list}"		},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",				AJA_NULL},
		{"novideo",		0,		POPT_ARG_NONE,		&noVideo,		0,	"disable video?",				AJA_NULL},
		{"noanc",		0,		POPT_ARG_NONE,		&noAnc,			0,	"disable anc?",					AJA_NULL},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format",					"'?' or 'list' to list"},
		{"iframes",		0,		POPT_ARG_STRING,	&pInFramesSpec,	0,	"input AutoCirc frames",		"num[@min] or min-max"},
		{"oframes",		0,		POPT_ARG_STRING,	&pOutFramesSpec,0,	"output AutoCirc frames",		"num[@min] or min-max"},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multi-channel/format?",	AJA_NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	if (::poptGetNextOpt(optionsContext) < -1)
	{
		cerr << "## ERROR:  Bad command line argument(s)" << endl;
		return 1;
	}
	optionsContext = ::poptFreeContext(optionsContext);
	const string	deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	const string	vidSourceStr	(pVidSource ? CNTV2DemoCommon::ToLower(pVidSource) : "");
	const string	tcSourceStr		(pTcSource ? CNTV2DemoCommon::ToLower(pTcSource) : "");
	const string	legalDevices	(CNTV2DemoCommon::GetDeviceStrings());
	BurnConfig config(deviceSpec);

	if (deviceSpec == "?"  ||  deviceSpec == "list")
		{cout << legalDevices << endl;  return 0;}
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		{cout << "## ERROR:  No such device '" << deviceSpec << "'" << endl << legalDevices;  return 1;}

	//	Select video source...
	const string	legalSources(CNTV2DemoCommon::GetInputSourceStrings(NTV2_INPUTSOURCES_ALL, deviceSpec));
	if (vidSourceStr == "?"  ||  vidSourceStr == "list")
		{cout << legalSources << endl;  return 0;}
	if (!vidSourceStr.empty())
	{
		config.fInputSource = CNTV2DemoCommon::GetInputSourceFromString(vidSourceStr);
		if (!NTV2_IS_VALID_INPUT_SOURCE(config.fInputSource))
			{cerr << "## ERROR:  Input source '" << vidSourceStr << "' not one of these:" << endl << legalSources << endl;	return 1;}
	}	//	if video source specified

	//	Select time code source...
	const string	legalTCSources(CNTV2DemoCommon::GetTCIndexStrings(TC_INDEXES_ALL, deviceSpec));
	if (tcSourceStr == "?"  ||  tcSourceStr == "list")
		{cout << legalTCSources << endl;  return 0;}
	if (!tcSourceStr.empty())
	{
		config.fTimecodeSource = CNTV2DemoCommon::GetTCIndexFromString(tcSourceStr);
		if (!NTV2_IS_VALID_TIMECODE_INDEX(config.fTimecodeSource))
			{cerr << "## ERROR:  Timecode source '" << tcSourceStr << "' not one of these:" << endl << legalTCSources << endl;	return 1;}
	}

	//	Pixel format
	const string pixelFormatStr (pPixelFormat  ? pPixelFormat :  "1");
	const string legalFBFs (CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	if (!pixelFormatStr.empty())
	{
		config.fPixelFormat = CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
		if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT(config.fPixelFormat))
			{cerr << "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl << legalFBFs << endl;  return 2;}
	}

	//	InputFrames
	string framesSpec(pInFramesSpec ? pInFramesSpec : "");
	static const string	legalFrmSpec("{frameCount}[@{firstFrameNum}]  or  {firstFrameNum}-{lastFrameNum}");
	if (!framesSpec.empty())
	{	const string parseResult(config.fInputFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad '--iframes' spec '" << framesSpec << "'" << endl << "## " << parseResult << endl;  return 1;}
	}
	if (!config.fInputFrames.valid())
		{cerr << "## ERROR:  Bad '--iframes' spec '" << framesSpec << "'" << endl << "## Expected " << legalFrmSpec << endl;  return 1;}

	//	OutputFrames
	framesSpec = pOutFramesSpec ? pOutFramesSpec : "";
	if (!framesSpec.empty())
	{	const string parseResult(config.fOutputFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad '--oframes' spec '" << framesSpec << "'" << endl << "## " << parseResult << endl;  return 1;}
	}
	if (!config.fOutputFrames.valid())
		{cerr << "## ERROR:  Bad '--oframes' spec '" << framesSpec << "'" << endl << "## Expected " << legalFrmSpec << endl;  return 1;}

	if (noVideo  &&  noAudio)
		{cerr	<< "## ERROR:  '--novideo' and '--noaudio' cannot both be specified" << endl;  return 1;}
	config.fSuppressAudio	= noAudio ? true : false;
	config.fSuppressVideo	= noVideo ? true : false;
	config.fSuppressAnc		= noAnc ? true : false;
	config.fDoMultiFormat	= doMultiChannel ? true : false;

	//	Instantiate the NTV2Burn object...
	NTV2Burn burner (config);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the NTV2Burn instance...
	status = burner.Init();
	if (!AJA_SUCCESS (status))
		{cerr << "## ERROR:  Initialization failed, status=" << status << endl;  return 4;}

	//	Start the burner's capture and playout threads...
	burner.Run();

	//	Loop until told to stop...
	cout	<< "           Capture  Playout  Capture  Playout" << endl
			<< "   Frames   Frames   Frames   Buffer   Buffer" << endl
			<< "Processed  Dropped  Dropped    Level    Level" << endl;
	do
	{
		AUTOCIRCULATE_STATUS inputStatus, outputStatus;
		burner.GetStatus (inputStatus, outputStatus);
		cout	<< setw(9) << inputStatus.GetProcessedFrameCount()
				<< setw(9) << inputStatus.GetDroppedFrameCount()
				<< setw(9) << outputStatus.GetDroppedFrameCount()
				<< setw(9) << inputStatus.GetBufferLevel()
				<< setw(9) << outputStatus.GetBufferLevel()
				<< "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;

	return AJA_SUCCESS(status) ? 0 : 2;	//	Return zero upon success -- otherwise 2

}	//	main
