/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2outputtestpattern/main.cpp
	@brief		Simple demonstration application to display a fixed test pattern on an AJA device's output.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2outputtestpattern.h"
#include "ajabase/common/common.h"


using namespace std;


int main (int argc, const char ** argv)
{
	char *			pDeviceSpec		(AJA_NULL);		//	Device specifier string, if any
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format argument
	char *			pTestPattern	(AJA_NULL);		//	Test pattern argument
	char *			pVancMode		(AJA_NULL);		//	VANC mode argument
	char *			pVideoFormat	(AJA_NULL);		//	Video format to use
	int				channelNumber	(1);			//	Channel/FrameStore to use
	int				showVersion		(0);			//	Show version?
	AJADebug::Open();

	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",			"1-8"						},
		{"pattern",		'p',	POPT_ARG_STRING,	&pTestPattern,	0,	"test pattern to show",		"0-15, name or '?' to list"	},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"video format to produce",	"'?' or 'list' to list"},
		{"pixelFormat",	  0,	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"},
		{"vanc",		  0,	POPT_ARG_STRING,	&pVancMode,		0,	"vanc mode",				"off|none|0|on|tall|1|taller|tallest|2"},
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

	TestPatConfig config(deviceSpec);

	//	Channel
	if ((channelNumber < 1)  ||  (channelNumber > 8))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 1;}
	config.fOutputChannel = NTV2Channel(channelNumber - 1);

	//	VideoFormat
	const string videoFormatStr (pVideoFormat  ?  pVideoFormat  :  "");
	config.fVideoFormat = videoFormatStr.empty()	?	NTV2_FORMAT_1080i_5994
													:	CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr);
	if (videoFormatStr == "?"  ||  videoFormatStr == "list")
		{cout	<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_NON_4KUHD, deviceSpec) << endl;  return 0;}
	else if (!videoFormatStr.empty()  &&  config.fVideoFormat == NTV2_FORMAT_UNKNOWN)
	{	cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_NON_4KUHD, deviceSpec) << endl;
		return 2;
	}

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

	//	Pattern
	string tpSpec(pTestPattern  ?  pTestPattern  :  "");
	aja::lower(tpSpec);
	if (tpSpec == "?" || tpSpec == "list")
		{cout << CNTV2DemoCommon::GetTestPatternStrings() << endl;  return 0;}
	if (!tpSpec.empty())
	{
		config.fTestPatternName = CNTV2DemoCommon::GetTestPatternNameFromString(tpSpec);
		if (config.fTestPatternName.empty())
		{
			cerr	<< "## ERROR:  Invalid '--pattern' value '" << tpSpec << "' -- expected values:" << endl
					<< CNTV2DemoCommon::GetTestPatternStrings() << endl;
			return 2;
		}
	}

	//	VANC Mode
	string vancStr (pVancMode ? pVancMode : "");
	aja::lower(vancStr);
	if (vancStr == "?" || vancStr == "list")
		{cout << CNTV2DemoCommon::GetVANCModeStrings() << endl;  return 0;}
	if (videoFormatStr.empty()  &&  !vancStr.empty())
		{cerr << "## ERROR: '--vanc' option also requires --videoFormat option" << endl;  return 2;}
	if (!vancStr.empty())
	{
		config.fVancMode = CNTV2DemoCommon::GetVANCModeFromString(vancStr);
		if (!NTV2_IS_VALID_VANCMODE(config.fVancMode))
		{	cerr	<< "## ERROR:  Invalid '--vanc' value '" << vancStr << "' -- expected values: " << endl
					<< CNTV2DemoCommon::GetVANCModeStrings() << endl;
			return 2;
		}
	}

	//	Create the object that will display the test pattern...
	NTV2OutputTestPattern player(config);
	AJAStatus status = player.Init();
	if (AJA_FAILURE(status))
		{cout << "## ERROR:  Initialization failed: " << ::AJAStatusToString(status) << endl;	return 1;}

	//	Write the test pattern to the device and make it visible on the output...
	status = player.EmitPattern();
	if (AJA_FAILURE(status))
		{cout << "## ERROR:  EmitPattern failed: " << ::AJAStatusToString(status) << endl;	return 2;}

	//	Pause and wait for user to press Return or Enter...
	cout << "## NOTE:  Press Enter or Return to exit..." << endl;
	cin.get();

	return 0;

}	// main
