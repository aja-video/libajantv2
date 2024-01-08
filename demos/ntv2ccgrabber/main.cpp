/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2ccgrabber/main.cpp
	@brief		Demonstration application that grabs closed-captioning data from frames captured
				from SDI input (using AutoCirculate), and writes the captions to standard output.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/pnp/pnp.h"
#include "ntv2ccgrabber.h"
#include "ntv2utils.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#if defined (MSWindows) || defined (AJAWindows)
	#define	SIGQUIT	SIGBREAK
#endif

using namespace std;


//	Globals
static bool		gGlobalQuit		(false);	//	Set this "true" to exit gracefully
static AJAPnp	gPlugAndPlay;				//	To detect device disconnects


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
	gPlugAndPlay.Uninstall ();
}


/**
	@brief	This function gets called whenever an AJA device is attached or detached to/from the host.
	@param[in]	inMessage	Specifies if a device was attached or detached.
	@param[in]	pUserData	The client data associated with the callback.
**/
static void PnpCallback (const AJAPnpMessage inMessage, void * pUserData)		//	static
{
	(void) pUserData;
	if (inMessage == AJA_Pnp_DeviceAdded || inMessage == AJA_Pnp_DeviceRemoved)
	{
		static unsigned	sAttaches	(0);
		static unsigned	sDetaches	(0);
		if (inMessage == AJA_Pnp_DeviceAdded)
			sAttaches++;
		if (inMessage == AJA_Pnp_DeviceRemoved)
			sDetaches++;
		if (sAttaches > 1 || inMessage == AJA_Pnp_DeviceRemoved)
		{
			SignalHandler (SIGQUIT);
			cerr << "## WARNING:  Terminating 'ntv2ccgrabber' due to device " << (inMessage == AJA_Pnp_DeviceAdded ? "attach" : "detach") << endl;
		}
	}

}	//	PnpCallback



int main (int argc, const char ** argv)
{
	const string	legalChannels	(NTV2CCGrabber::GetLine21ChannelNames("|"));
	const string	legal608Sources	(CCGrabberConfig::CaptionDataSrcToString(kCaptionDataSrc_INVALID));
	const string	legalOutputModes(CCGrabberConfig::OutputModeToString(kOutputMode_INVALID));
	const string	legalFramesSpec	("{frameCount}[@{firstFrameNum}]  or  {firstFrameNum}-{lastFrameNum}");
	char *			pDeviceSpec		(AJA_NULL);				//	Device spec
	char *			pInputSrcSpec	(AJA_NULL);				//	SDI source spec
	char *			pTimecodeSpec	(AJA_NULL);				//	Timecode source spec
	char *			pPixelFormat	(AJA_NULL);				//	Pixel format spec
	char *			pCaptionChannel	(AJA_NULL);				//	Caption channel of interest (cc1, cc2 ... text1, text2, ...)
	char *			pCaptionSource	(AJA_NULL);				//	Caption source of interest (line21, 608vanc, 608anc ...)
	char *			pOutputMode		(AJA_NULL);				//	Output mode (stream, screen, file ...)
	char *			pFramesSpec		(AJA_NULL);				//	AutoCirculate frames spec
	int				channelNumber	(1);					//	Channel (framestore) spec
	int				burnCaptions	(0);					//	Burn-in captions?
	int				doMultiFormat	(0);					//	MultiFormat mode?
	int				useVanc			(0);					//	Use Vanc (tall frame) geometry?
	int				grabAudio		(0);					//	Grab audio?
	int				showVersion		(0);					//	Show version?
	AJADebug::Open();

	::setlocale (LC_ALL, "");	//	Might have to emit UTF-8 Unicode

	gPlugAndPlay.Install (PnpCallback, 0, AJA_Pnp_PciVideoDevices);

	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,		0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,		0,	"channel to use",			"1-8"						},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,		0,	"use multi-format/channel",	AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,		0,	"pixel format to use",		"'?' or 'list' to list"		},
		{"frames",		  0,	POPT_ARG_STRING,	&pFramesSpec,		0,	"frames to AutoCirculate",	"num[@min] or min-max"		},
		{"input",		'i',	POPT_ARG_STRING,	&pInputSrcSpec,		0,	"which SDI input",			"1-8, ?=list"				},
		{"tcsource",	't',	POPT_ARG_STRING,	&pTimecodeSpec,		0,	"timecode source",			"'?' or 'list' to list"		},
		{"608chan",		  0,	POPT_ARG_STRING,	&pCaptionChannel,	0,	"608 cap chan to monitor",	legalChannels.c_str()		},
		{"608src",		  0,	POPT_ARG_STRING,	&pCaptionSource,	0,	"608 source to use",		legal608Sources.c_str()		},
		{"output",		  0,	POPT_ARG_STRING,	&pOutputMode,		0,	"608 output mode",			legalOutputModes.c_str()	},
		{"burn",		'b',	POPT_ARG_NONE,		&burnCaptions,		0,	"burn-in captions",			AJA_NULL					},
		{"vanc",		'v',	POPT_ARG_NONE,		&useVanc,			0,	"use vanc geometry",		AJA_NULL					},
		{"audio",		'a',	POPT_ARG_NONE,		&grabAudio,			0,	"also capture audio",		AJA_NULL					},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}

	const string	deviceSpec		(pDeviceSpec   ? pDeviceSpec : "0");
	const string	inputSourceStr	(pInputSrcSpec ? CNTV2DemoCommon::ToLower(string(pInputSrcSpec)) : "");
	const string	tcSourceStr		(pTimecodeSpec ? CNTV2DemoCommon::ToLower(string(pTimecodeSpec)) : "");
	const string	pixelFormatStr	(pPixelFormat  ? pPixelFormat :  "");
	const string	framesSpec		(pFramesSpec   ? pFramesSpec  :  "");
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	//	Device
	const string	legalDevices(CNTV2DemoCommon::GetDeviceStrings());
	if (deviceSpec == "?" || deviceSpec == "list")
		{cout << legalDevices << endl;  return 0;}
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		{cout << "## ERROR:  No such device '" << deviceSpec << "'" << endl << legalDevices;  return 1;}

	CCGrabberConfig	config(deviceSpec);

	//	Channel
	if ((channelNumber < 1)  ||  (channelNumber > 8))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 1;}
	config.fInputChannel = NTV2Channel(channelNumber - 1);

	//	Input source
	const string	legalSources(CNTV2DemoCommon::GetInputSourceStrings(NTV2_IOKINDS_SDI, deviceSpec));
	if (inputSourceStr == "?" || inputSourceStr == "list")
		{cout << legalSources << endl;  return 0;}
	if (!inputSourceStr.empty())
	{
		config.fInputSource = CNTV2DemoCommon::GetInputSourceFromString(inputSourceStr);
		if (!NTV2_IS_VALID_INPUT_SOURCE(config.fInputSource))
			{cerr << "## ERROR:  Input source '" << inputSourceStr << "' not one of these:" << endl << legalSources << endl;	return 1;}
	}	//	if input source specified

	//	Timecode source
	const string	legalTCSources(CNTV2DemoCommon::GetTCIndexStrings(TC_INDEXES_ALL, deviceSpec));
	if (tcSourceStr == "?" || tcSourceStr == "list")
		{cout << legalTCSources << endl;  return 0;}
	if (!tcSourceStr.empty())
	{
		config.fTimecodeSrc = CNTV2DemoCommon::GetTCIndexFromString(tcSourceStr);
		if (!NTV2_IS_VALID_TIMECODE_INDEX(config.fTimecodeSrc))
			{cerr << "## ERROR:  Timecode source '" << tcSourceStr << "' not one of these:" << endl << legalTCSources << endl;	return 1;}
	}

	//	Pixel format
	const string	legalFBFs(CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty())
	{
		config.fPixelFormat = CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
		if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT(config.fPixelFormat))
			{cerr << "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl << legalFBFs << endl;  return 2;}
	}

	//	Caption channel
	if (pCaptionChannel)
	{
		config.fCaptionChannel = ::StrToNTV2Line21Channel(string(pCaptionChannel));
		if (config.fCaptionChannel == NTV2_CC608_ChannelMax)
			{cerr << "## ERROR:  Bad '608chan' value '" << pCaptionChannel << "' -- expected '" << legalChannels << "'" << endl;	return 1;}
	}

	//	Caption Source
	const string	captionSrcStr(pCaptionSource ? pCaptionSource : "");
	if (captionSrcStr == "?" || captionSrcStr == "list")
		{cout << "## NOTE: Legal --608src values: " << legal608Sources << endl;  return 0;}
	else if (!captionSrcStr.empty())
	{
		config.fCaptionSrc = CCGrabberConfig::StringToCaptionDataSrc(captionSrcStr);
		if (!IS_VALID_CaptionDataSrc(config.fCaptionSrc))
			{cerr << "## ERROR:  Bad '608src' value '" << captionSrcStr << "' -- expected '" << legal608Sources << "'" << endl;	return 1;}
	}

	//	Output mode
	const string	outputModeStr(pOutputMode ? pOutputMode : "");
	if (outputModeStr == "?" || outputModeStr == "list")
		{cout << "## NOTE: Legal --output values: " << legalOutputModes << endl;  return 0;}
	else if (!outputModeStr.empty())
	{
		config.fOutputMode = CCGrabberConfig::StringToOutputMode(outputModeStr);
		if (!IS_VALID_OutputMode(config.fOutputMode))
			{cerr << "## ERROR:  Bad 'output' value '" << outputModeStr << "' -- expected '" << legalOutputModes << "'" << endl;	return 1;}
	}

	if (!config.GetNumSourceSpecs())
	{
		cerr << "## WARNING:  No input channel, or input source, or timecode source specified -- will use NTV2_INPUTSOURCE_SDI1, NTV2_CHANNEL1" << endl;
		config.fInputChannel = NTV2_CHANNEL1;
		config.fInputSource = NTV2_INPUTSOURCE_SDI1;
	}

	//	AutoCirculate frames
	if (!framesSpec.empty())
	{
		const string parseResult(config.fFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## " << parseResult << endl;  return 1;}
	}
	if (!config.fFrames.valid())
		{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## Expected " << legalFramesSpec << endl;  return 1;}

	//	Configure the grabber...
	config.fBurnCaptions	= burnCaptions	? true : false;
	config.fDoMultiFormat	= doMultiFormat	? true : false;
	config.fUseVanc			= useVanc		? true : false;
	config.fWithAudio		= grabAudio		? true : false;

	NTV2CCGrabber ccGrabber (config);

	//	Initialize the ccGrabber instance...
	AJAStatus status (ccGrabber.Init());
	if (AJA_FAILURE(status))
		{cerr << "## ERROR:  'ntv2ccgrabber' initialization failed with status " << status << endl;	return 2;}

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Run the ccGrabber...
	if (AJA_FAILURE(ccGrabber.Run()))
		{cerr << "## ERROR:  'ntv2ccgrabber' capture thread failed to run -- check for AutoCirculate messages in AJALogger?" << endl; return 2;}

	//	Loop until someone tells us to stop...
	while (!gGlobalQuit)
	{
		const char	keyPressed	(CNTV2DemoCommon::ReadCharacterPress());
		if (keyPressed == 'q' || keyPressed == 'Q')
			SignalHandler(SIGQUIT);
		else if (keyPressed >= '1' && keyPressed <= '9')
			ccGrabber.SetCaptionDisplayChannel(NTV2Line21Channel(keyPressed - '1'));
		else if (keyPressed == '?')
			cerr << endl << "## HELP:  1=CC1 2=CC2 3=CC3 4=CC4 5=Txt1 6=Txt2 7=Txt3 8=Txt4 Q=Quit H=HUD O=Output S=608Src P=PixFmt ?=Help" << endl;
		else if (keyPressed == 'h' || keyPressed == 'H')
			ccGrabber.ToggleHUD();
		else if (keyPressed == 'v' || keyPressed == 'V')
			ccGrabber.ToggleVANC();
		else if (keyPressed == 'o' || keyPressed == 'O')
			ccGrabber.SwitchOutput();
		else if (keyPressed == 'p' || keyPressed == 'P')
			ccGrabber.SwitchPixelFormat();
		else if (keyPressed == 's' || keyPressed == 'S')
			ccGrabber.Switch608Source();
		AJATime::Sleep(500);
	}
	cerr << endl;
	return 0;

}	//	main
