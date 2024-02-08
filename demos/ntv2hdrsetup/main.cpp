/* SPDX-License-Identifier: MIT */
/**
    @file       ntv2hdrsetup/main.cpp
    @brief      Demonstration application that shows how to enable HDR capabilities of 
                HDMI out.
    @copyright  Copyright (C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


// Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/common/types.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"

#include "ntv2democommon.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


// Globals
const uint32_t kAppSignature (NTV2_FOURCC('H','d','r','s'));


/**
    @brief      Main entry point for 'ntv2hdrsetup' demo application.
    @param[in]  argc    Number arguments specified on the command line, including the path to the executable.
    @param[in]  argv    Array of 'const char' pointers, one for each argument.
    @return     Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	char *	pDeviceSpec		(AJA_NULL);	//	Which device to use
	int		eotf			(0);		//	EOTF to enable 0,1,2,3
	int		constLuminance	(0);		//	Constant luminanace?
	int		dolbyVision		(0);		//	Enable dolby vision bit?
	int		verbose			(0);		//	Verbose mode?
	int		noHDR			(0);		//	Disable hdr?
	int		showVersion		(0);		//	Show version and exit?
	AJADebug::Open();

	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"device to use",			"index#, serial#, or model"},
		{"eotf",		'e',	POPT_ARG_INT,		&eotf,				0,	"0=TradGammaSDR 1=TradGammaHDR 2=ST2084 3=HLG",	"EOTF to use"},
		{"luminance",	'l',	POPT_ARG_NONE,		&constLuminance,	0,	"constant luminance?",		AJA_NULL	},
		{"dolbyvision",	 0,		POPT_ARG_NONE,		&dolbyVision,		0,	"enable DolbyVision bit?",	AJA_NULL	},
		{"nohdr",		 0,		POPT_ARG_NONE,		&noHDR,				0,	"disable HDMI HDR output?",	AJA_NULL	},
		{"verbose",		'v',	POPT_ARG_NONE,		&verbose,			0,	"verbose output?",			AJA_NULL	},
		{"version",		 0,		POPT_ARG_NONE,		&showVersion,		0,	"show version & exit",		AJA_NULL	},
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 1;}
	if (showVersion)
		{cout << "NTV2HDRSetup, NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	if ((eotf < 0)  ||  (eotf > 3))
		{cerr << "## ERROR:  Bad EOTF value '" << DEC(eotf) << "' -- expected 0, 1, 2 or 3" << endl; return 2;}

	//	Open the device...
	CNTV2Card device;
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (pDeviceSpec ? pDeviceSpec : "0", device))
		{cerr << "## ERROR:  Device '" << pDeviceSpec << "' not found" << endl; return 3;}

	if (!device.IsDeviceReady(false))
		{cerr << "## ERROR:  Device '" << pDeviceSpec << "' not ready" << endl; return 4;}

	if (!NTV2DeviceCanDoHDMIHDROut(device.GetDeviceID()))
		{cerr << "## ERROR:  Device '" << pDeviceSpec << "' does not support HDMI HDR" << endl; return 5;}

	//	Acquire the device...
	ULWord  appSignature (0);
	int32_t appPID       (0);
	NTV2EveryFrameTaskMode savedTaskMode(NTV2_TASK_MODE_INVALID);
	device.GetEveryFrameServices(savedTaskMode);			//	Save the current device state
	device.GetStreamingApplication(appSignature, appPID);	//	Who currently "owns" the device?
	if (!device.AcquireStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid())))
	{
		cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
		return 6;	//	Device reserved by other app
	}
	device.SetEveryFrameServices(NTV2_OEM_TASKS);	//	Set the OEM service level

	if (verbose)
	{
		static const string sEOTFs[] = {"Traditional Gamma SDR", "Traditional Gamma HDR", "ST-2084", "HLG", ""};
		cout << "## NOTE:  Options specified:" << endl
			<<	"\tEOTF:        " << DEC(eotf) << " (" << sEOTFs[eotf] << ")" << endl
			<<	"\tLuminance:   " << (constLuminance ? "Constant" : "Non-Constant") << endl
			<<	"\tDolbyVision: " << (dolbyVision ? "Enabled" : "Disabled") << endl;
		if (noHDR)
			cout << "## WARNING:  --nohdr option will disable HDMI HDR output" << endl;
	}

	//	Load up the digital primitives with some reasonable values...
	HDRRegValues registerValues;
	::setHDRDefaultsForBT2020(registerValues);
	registerValues.electroOpticalTransferFunction = uint8_t(eotf);
	device.SetHDRData(registerValues);

	//	Setup HDR values based on passed args...
	device.SetHDMIHDRConstantLuminance(bool(constLuminance));
	device.SetHDMIHDRElectroOpticalTransferFunction(uint8_t(eotf));

	//	Enabling this will allow dolby vision containing frames to properly display out of HDMI
	device.EnableHDMIHDRDolbyVision(bool(dolbyVision));

	//	The master switch for HDMI HDR output
	device.EnableHDMIHDR(noHDR ? false : true);

	//	Loop until a key is pressed, that way user can inspect the changes with watcher
	CNTV2DemoCommon::WaitForEnterKeyPress();

	device.SetEveryFrameServices(savedTaskMode);	//	Restore prior tasks mode
	device.ReleaseStreamForApplication (kAppSignature, int32_t(AJAProcess::GetPid()));	//	Release the device

	return 0;	//	Success!

}	//	main
