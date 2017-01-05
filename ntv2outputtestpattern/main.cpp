/**
	@file		ntv2outputtestpattern/main.cpp
	@brief		Demonstration application to display test patterns on an AJA device's output using
				direct DMA (i.e., without using AutoCirculate).
	@copyright	Copyright (C) 2012-2017 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajabase/common/types.h"
#include "ajabase/common/options_popt.h"
#include "ntv2outputtestpattern.h"
#include <iostream>
#include <iomanip>

using namespace std;


//
//	Main program
//
int main (int argc, const char ** argv)
{
	AJAStatus	status				(AJA_STATUS_SUCCESS);	//	Result status
	char *		pDeviceSpec			(NULL);					//	Which device to use
	uint32_t	channelNumber		(1);					//	Which channel to use
	uint32_t	testPatternIndex	(0);					//	Which test pattern to display
	poptContext	optionsContext;								//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption	userOptionsTable []	=
	{
		{"board",	'b',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"device",	'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"channel",	'c',	POPT_ARG_INT,		&channelNumber,		0,	"which channel to use",			"1 thru 8"					},
		{"pattern",	'p',	POPT_ARG_INT,		&testPatternIndex,	0,	"which test pattern to show",	"test pattern number"		},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	//  Translate the channel number into a NTV2_CHANNELx enum...
	const NTV2Channel	channel	(::GetNTV2ChannelForIndex (channelNumber - 1));
	if (!NTV2_IS_VALID_CHANNEL (channel))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 2;}

	//  Create the object that will display the test pattern...
	NTV2OutputTestPattern outputTestPattern (pDeviceSpec ? string (pDeviceSpec) : "0", channel);

	//  Make sure the requested device can be acquired...
	status = outputTestPattern.Init (); 
	if (AJA_FAILURE (status))
		{cerr << "## ERROR:  Init failed, status=" << status << endl;  return 2;}

	//  Write the test pattern to the board and make it visible on the output...
	status = outputTestPattern.EmitPattern (testPatternIndex);
	if (AJA_FAILURE (status))
		{cerr << "## ERROR:  EmitPattern failed, status=" << status << endl;  return 2;}

	cout << "## NOTE:  Press Enter or Return to exit..." << endl;
	cin.get ();

	return 0;

}	// main
