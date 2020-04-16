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
	ULWord		channelNumber	(1);		//	Which channel to use
	UWord		testPatternIndex(0);		//	Which test pattern to display
	poptContext	optionsContext;				//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption	userOptionsTable []	=
	{
		#if !defined(NTV2_DEPRECATE_16_0)	//	--board option is deprecated!
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",	"(deprecated)"	},
		#endif
		{"device",	'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"device to use",		"index#, serial#, or model"	},
		{"channel",	'c',	POPT_ARG_INT,		&channelNumber,		0,	"channel to use",		"1-8"	},
		{"pattern",	'p',	POPT_ARG_SHORT,		&testPatternIndex,	0,	"test pattern to show",	"0-15"	},
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
	if (testPatternIndex >= uint32_t(NTV2_TestPatt_All))
		{cerr << "## ERROR:  Invalid test pattern index " << testPatternIndex << " -- expected 0 thru " << (uint32_t(NTV2_TestPatt_All)-1) << endl;  return 2;}

	//	Create the object that will display the test pattern...
	NTV2OutputTestPattern outputTestPattern (deviceSpec, channel);

	//	Make sure the requested device can be acquired...
	if (AJA_FAILURE(outputTestPattern.Init()))
		{cerr << "## ERROR:  Initialization failed" << endl;  return 2;}

	//	Write the test pattern to the board and make it visible on the output...
	if (AJA_FAILURE(outputTestPattern.EmitPattern(testPatternIndex)))
		{cerr << "## ERROR:  EmitPattern failed" << endl;  return 2;}

	//	Pause and wait for user to press Return or Enter...
	cout << "## NOTE:  Press Enter or Return to exit..." << endl;
	cin.get();

	return 0;

}	// main
