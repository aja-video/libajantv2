/**
	@file		ntv2outputtestpattern/main.cpp
	@brief		Demonstration application to display test patterns on an AJA device's output using
				direct DMA (i.e., without using AutoCirculate).
	@copyright	(C) 2012-2020 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajabase/common/types.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/debug.h"
#include "ntv2outputtestpattern.h"
#include "ntv2democommon.h"
#include <iostream>
#include <iomanip>

using namespace std;


//
//	Main program
//
int main (int argc, const char ** argv)
{
	char *		pDeviceSpec		(AJA_NULL);	//	Which device to use
	char *		pTestPattern	(AJA_NULL);	//	Test pattern argument
	char *		pVideoFormat	(AJA_NULL);	//	Video format argument
	ULWord		channelNumber	(1);		//	Which channel to use
	poptContext	optionsContext;				//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption	userOptionsTable []	=
	{
		#if !defined(NTV2_DEPRECATE_16_0)	//	--board option is deprecated!
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",	"(deprecated)"	},
		#endif
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",		"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",		"1-8"	},
		{"pattern",		'p',	POPT_ARG_STRING,	&pTestPattern,	0,	"test pattern to show",	"0-15, name or '?' to list"	},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"video format to use",	"'?' or 'list' to list"},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext(optionsContext);

	//	Device
	const string	deviceSpec	(pDeviceSpec ? pDeviceSpec : "0");
	const string	legalDevices(CNTV2DemoCommon::GetDeviceStrings(NTV2_DEVICEKIND_ALL));
	if (deviceSpec == "?" || deviceSpec == "list")
		{cout << legalDevices << endl;  return 0;}
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		{cout << "## ERROR:  No such device '" << deviceSpec << "'" << endl << legalDevices;  return 1;}

	//	Channel
	const NTV2Channel	channel	(::GetNTV2ChannelForIndex(channelNumber - 1));
	if (!NTV2_IS_VALID_CHANNEL(channel))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 2;}

	//	Pattern
	string tpSpec(pTestPattern  ?  pTestPattern  :  "");
	if (tpSpec == "?" || tpSpec == "list")
		{cout << CNTV2DemoCommon::GetTestPatternStrings() << endl;  return 0;}
	if (!tpSpec.empty())
	{
		tpSpec = CNTV2DemoCommon::GetTestPatternNameFromString(tpSpec);
		if (tpSpec.empty())
		{
			cerr	<< "## ERROR:  Invalid '--pattern' value '" << pTestPattern << "' -- expected values:" << endl
					<< CNTV2DemoCommon::GetTestPatternStrings() << endl;
			return 2;
		}
	}

	//	VideoFormat
	const string vfStr	(pVideoFormat  ?  pVideoFormat  :  "");
	if (vfStr == "?" || vfStr == "list")
		{cout << CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_NON_4KUHD, deviceSpec) << endl;  return 0;}
	if (!vfStr.empty() && CNTV2DemoCommon::GetVideoFormatFromString(vfStr) == NTV2_FORMAT_UNKNOWN)
	{	cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << vfStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_NON_4KUHD, deviceSpec) << endl;
		return 2;
	}
	const NTV2VideoFormat vFmt(vfStr.empty() ? NTV2_FORMAT_UNKNOWN : CNTV2DemoCommon::GetVideoFormatFromString(vfStr));

	//	Create the object that will display the test pattern...
	NTV2OutputTestPattern outputTestPattern (deviceSpec, tpSpec, vFmt, channel);

	//	Make sure the requested device can be acquired...
	if (AJA_FAILURE(outputTestPattern.Init()))
		{cerr << "## ERROR:  Initialization failed" << endl;  return 2;}

	//	Write the test pattern to the device and make it visible on the output...
	if (AJA_FAILURE(outputTestPattern.EmitPattern()))
		{cerr << "## ERROR:  EmitPattern failed" << endl;  return 2;}

	//	Pause and wait for user to press Return or Enter...
	cout << "## NOTE:  Press Enter or Return to exit..." << endl;
	cin.get();

	return 0;

}	// main
