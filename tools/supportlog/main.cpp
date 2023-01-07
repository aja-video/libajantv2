/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/supportlog/main.cpp
	@brief		Simple command line application to generate a support log.
	@copyright	(C) 2017-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ntv2devicescanner.h"
#include "ntv2supportlogger.h"
#include "ntv2utils.h"
#include "ajabase/common/common.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/info.h"
#include "ajabase/system/systemtime.h"

using namespace std;


// Globals
static bool gGlobalQuit (false);  /// Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
    (void) inSignal;
    gGlobalQuit = true;
}


int main(int argc, const char ** argv)
{
	char	*pDeviceSpec(AJA_NULL), *pInputFileName(AJA_NULL);
	int		doStdout(0), doSDRAM(0), waitSeconds(0), forceLoad(0), isVerbose(0);
	CNTV2Card device;
	poptContext	optionsContext;	//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"Which device",						"index#, serial#, or model"},
		{"load",		'l',	POPT_ARG_STRING,	&pInputFileName,	0,	"Loads some regs from log file",	AJA_NULL},
		{"forceload",	'f',	POPT_ARG_NONE,		&forceLoad,			0,	"Load onto different device",		AJA_NULL},
		{"stdout",		's',	POPT_ARG_NONE,		&doStdout,			0,	"Dump to stdout instead of file?",	AJA_NULL},
		{"sdram",		'r',	POPT_ARG_NONE,		&doSDRAM,			0,	"Dump device SDRAM to .raw file?",	AJA_NULL},
		{"verbose",		'v',	POPT_ARG_NONE,		&isVerbose,			0,	"Verbose mode?",					AJA_NULL},
		{"wait",		'w',	POPT_ARG_INT,		&waitSeconds,		0,	"Time to wait before capturing",	"seconds"},
		POPT_AUTOHELP
		POPT_TABLEEND
    };

    //	Read command line arguments...
    optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
    if (::poptGetNextOpt(optionsContext) != -1)
		{cerr << "## ERROR: Syntax error in command line" << endl;  return 2;}
    optionsContext = ::poptFreeContext (optionsContext);

	const string inputFile (pInputFileName ? pInputFileName : "");
	const string deviceSpec	(pDeviceSpec ? pDeviceSpec : "0");
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(deviceSpec, device))
		{cerr << "## ERROR: Device '" << deviceSpec << "' failed to open or does not exist" << endl;  return 2;}
	if (doStdout && doSDRAM)
		{cerr << "## ERROR: '--stdout' and '--sdram' options conflict -- use one or the other, but not both" << endl;  return 2;}
	const string deviceName (CNTV2DeviceScanner::GetDeviceRefName(device));

	::signal (SIGINT, SignalHandler);
#if defined (AJAMac)
	::signal (SIGHUP, SignalHandler);
	::signal (SIGQUIT, SignalHandler);
#endif

	if (waitSeconds)
	{
		const uint64_t	startTime(AJATime::GetSystemMilliseconds());
		const uint64_t	endTime(startTime + uint64_t(waitSeconds * 1000)),  endSecond(endTime / 1000);
		uint64_t		curTime(startTime),  lastSecond(curTime / 1000);
		while (curTime < endTime)
		{
			AJATime::SleepInMicroseconds(1000*125);	//	Wait 1/8 of a second
			curTime = AJATime::GetSystemMilliseconds();
	
			//	Output a waiting message if log is being output to a file...
			uint64_t curSecond (curTime / 1000);
			if (curSecond != lastSecond  &&  !doStdout)
			{
				cout << "Capturing " << (doSDRAM ? "logs" : "log") << " in " << (endSecond-curSecond)+1 << " secs...    \r" << flush;
				lastSecond = curSecond;
			}
		}
	}	//	if waitTime > 0

	CNTV2SupportLogger logger(device);

	if (!inputFile.empty())
	{
		logger.LoadFromLog(inputFile, forceLoad ? true : false);
		return 0;	//	Done!
	}

	if (doStdout)
		cout << logger << flush;	//	Write log to stdout...
	else
	{	//	Write log to file...
		ostringstream SupportLogFileName, RamDumpFileName;
		SupportLogFileName << CNTV2SupportLogger::InventLogFilePathAndName(device);
		RamDumpFileName << CNTV2SupportLogger::InventLogFilePathAndName(device, "aja_sdram", "raw");
		ofstream ofs(SupportLogFileName.str());
		if (ofs)
		{
			ofs << logger << flush;
			ofs.close();
		}
		if (isVerbose)
			cout << "## NOTE: Support log for device '" << deviceName << "' written to '" << SupportLogFileName.str() << "'" << endl;
		if (doSDRAM)
		{	ostringstream oss;
			if (!CNTV2SupportLogger::DumpDeviceSDRAM (device, RamDumpFileName.str(), oss))
				cerr << oss.str();
			else if (isVerbose  &&  !oss.str().empty())
				cout << oss.str();
		}
	}
	return 0;
}	//	main
