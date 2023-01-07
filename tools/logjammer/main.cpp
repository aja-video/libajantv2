/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/logjammer/main.cpp
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.
**/

#include "ajabase/common/options_popt.h"
#include "ajabase/system/debug.h"
#include <ajabase/system/process.h>
#include <ajabase/common/timer.h>
#include <ajabase/system/thread.h>
#include <ajabase/system/systemtime.h>
#include <iostream>
#include <signal.h>
#include <string>

using namespace std;

#define	CONTAINS(__s__,__sub__)		((__s__).find(__sub__) != string::npos)
#define	STARTSWITH(__s__,__sub__)	((__s__).find(__sub__) == 0)


static uint32_t gStatKey(0);


static void SignalHandler (int inSignal)
{	(void)inSignal;
	if (gStatKey)
		for (uint32_t ndx(0);  ndx < 3;  ndx++)
			AJADebug::StatFree(gStatKey + ndx);
	AJADebug::Close();
	exit(1);
}

static uint32_t CheckSumValue (const string & inStr)
{
	uint32_t result(0);
	for (size_t ndx(0);  ndx < inStr.size();  ndx++)
		result += uint32_t(inStr.at(ndx));
	return result;
}


int main(int argc, const char *argv[])
{
	int				index		(4);		//	Message classification debug index (defaults to 4)
	int				msgsPerSec	(0);		//	Message log rate, messages per second (defaults to 0 -- use stdin)
	int				statNum		(0);		//	Stat key
	char *			pSeverity	(nullptr);	//	Severity string
	poptContext		optionsContext;			//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"index",		'i',	POPT_ARG_INT,		&index,			0,	"debug index to log into",		"0 thru 84"					},
		{"severity",	's',	POPT_ARG_STRING,	&pSeverity,		0,	"message severity to use",		"deb|inf|not|warn|err|..."	},
		{"rate",		'r',	POPT_ARG_INT,		&msgsPerSec,	0,	"message log rate",				"messages per second"		},
		{"stat",		0,		POPT_ARG_INT,		&statNum,		0,	"stat key to use",				"1 thru 255"				},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (nullptr, argc, argv, userOptionsTable, 0);
	if (::poptGetNextOpt (optionsContext) < -1)
		{cerr << "## ERROR:  Bad command line argument(s)" << endl;		return 1;}
	optionsContext = ::poptFreeContext (optionsContext);

	const string	severityStr (pSeverity ? pSeverity : "debug");
	AJADebugSeverity severity(AJA_DebugSeverity_Emergency);

	if (index < 0)
		{cerr << "## ERROR: Invalid index '" << index << "' -- must be greater than 0" << endl;	return 1;}

	if (STARTSWITH(severityStr, "eme"))
		severity = AJA_DebugSeverity_Emergency;
	else if (STARTSWITH(severityStr, "ale"))
		severity = AJA_DebugSeverity_Alert;
	else if (STARTSWITH(severityStr, "ass"))
		severity = AJA_DebugSeverity_Assert;
	else if (STARTSWITH(severityStr, "err"))
		severity = AJA_DebugSeverity_Error;
	else if(STARTSWITH(severityStr, "war"))
		severity = AJA_DebugSeverity_Warning;
	else if (STARTSWITH(severityStr, "not"))
		severity = AJA_DebugSeverity_Notice;
	else if (STARTSWITH(severityStr, "inf"))
		severity = AJA_DebugSeverity_Info;
	else if (STARTSWITH(severityStr, "deb"))
		severity = AJA_DebugSeverity_Debug;
	else
	{
		cerr << "## ERROR: Invalid severity '" << severityStr << " -- use emer[gency] | alert | assert | err[or] | warn[ing] | not[ice] | info | debug" << endl;
		return 1;
	}

	// Open AJA debug logging facility
	AJADebug::Open();

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif
	if (statNum >= int(AJADebug::StatsCapacity()))
		{cerr << "## WARNING: Stat ID " << statNum << " exceeds " << AJADebug::StatsCapacity() << " -- disabled";  statNum = 0;}

	gStatKey = uint32_t(statNum);
	if (gStatKey)
		for (uint32_t ndx(0);  ndx < 3;  ndx++)
		{
			if (!AJADebug::StatIsAllocated(gStatKey + ndx))
				AJADebug::StatAllocate(gStatKey + ndx);
			AJADebug::StatReset(gStatKey + ndx);
		}

	if (msgsPerSec)
	{
		const int32_t microsecsPerMsg (msgsPerSec  ?  1000000 / int32_t(msgsPerSec)  :  0);
		uint32_t lineTally(0);
		static const char cMsg[AJA_DebugSeverity_Size] = {'E', 'A', 'a', 'e', 'W', 'N', 'I', 'D'};
		while (++lineTally)
		{
			if (gStatKey) AJADebug::StatTimerStart(gStatKey);
				ostringstream oss;  oss << string(1+lineTally%128, cMsg[severity]) << " " << lineTally;
				AJADebug::Report(index, severity, "logjammer/main.cpp", __LINE__, oss.str());
				AJATime::SleepInMicroseconds(microsecsPerMsg);
			if (gStatKey) AJADebug::StatTimerStop(gStatKey);			//	10
			if (gStatKey) if (AJA_FAILURE(AJADebug::StatCounterIncrement(gStatKey+1)))	//	11
				cerr << "## StatSetValue " << (gStatKey+1) << " failed" << endl;
			if (gStatKey) if (AJA_FAILURE(AJADebug::StatSetValue(gStatKey+2, CheckSumValue(oss.str()))))	//	12
				cerr << "## StatSetValue " << (gStatKey+2) << " failed" << endl;	//	12
		}
	}
	else for (string line;  getline(cin, line);)
	{
		if (gStatKey) AJADebug::StatTimerStart(gStatKey);
			AJADebug::Report(index, severity, "logjammer/main.cpp", __LINE__, line);
		if (gStatKey) AJADebug::StatTimerStop(gStatKey);
		if (gStatKey) AJADebug::StatCounterIncrement(gStatKey+1);
			if (gStatKey) AJADebug::StatSetValue(gStatKey+2, CheckSumValue(line));
	}

#if 0	//	For testing AJADebug performance
    AJATimer t(AJATimerPrecisionNanoseconds);
    string units = AJATimer::PrecisionName(t.Precision(), true);
    while(1)
    {
    #if 1
        t.Start();
        AJA_sREPORT(AJA_DebugUnit_Unknown, AJA_DebugSeverity_Debug, "message via c++ streams");
        uint32_t cppT = t.ElapsedTime();

        t.Start();
        AJA_REPORT(AJA_DebugUnit_Unknown, AJA_DebugSeverity_Debug, "message via c vaargs");
        uint32_t cT = t.ElapsedTime();

        t.Start();
        AJA_sREPORT(AJA_DebugUnit_Unknown, AJA_DebugSeverity_Debug, "message via c++ streams: " << 42 << " now with args");
        uint32_t cppArgsT = t.ElapsedTime();

        t.Start();
        AJA_REPORT(AJA_DebugUnit_Unknown, AJA_DebugSeverity_Debug, "message via c vaargs: %d now with args", 42);
        uint32_t cArgsT = t.ElapsedTime();

        t.Start();
        AJAProcess::GetPid();
        uint32_t pidT = t.ElapsedTime();

        t.Start();
        AJAThread::GetThreadId();
        uint32_t tidT = t.ElapsedTime();

        AJA_REPORT(AJA_DebugUnit_StatsGeneric, AJA_DebugSeverity_Info, "== results ==");
        AJA_sREPORT(AJA_DebugUnit_StatsGeneric,AJA_DebugSeverity_Info, "    cpp time: " << cppT << " c time: " << cT << " (all in " << units << ")");
        AJA_REPORT(AJA_DebugUnit_StatsGeneric, AJA_DebugSeverity_Info, "    cpp time with args: %d c time with args: %d (all in %s)", cppArgsT, cArgsT, units.c_str());
        AJA_sREPORT(AJA_DebugUnit_StatsGeneric,AJA_DebugSeverity_Info, "    time to get pid: " << pidT << " time to get tid: " << tidT << " (all in " << units << ")");
    #endif
        //AJATime::Sleep(200);
    }
#endif	//	AJADebug performance testing

    AJADebug::Close();
    return 0;
}
