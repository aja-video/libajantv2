/* SPDX-License-Identifier: MIT */
/**
    @file       pciwhacker/main.cpp
    @brief      Simple command line application to test DMA read/write speeds to AJA devices.
    @copyright  Copyright (C) 2006-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include <cstdlib>
#include <iostream>
#include <csignal>
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ajabase/common/common.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/common/timer.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"

#include "ntv2democommon.h"

using namespace std;

// Globals
static bool gGlobalQuit (false);  /// Set this "true" to exit gracefully

static void SignalHandler (int inSignal)
{
    (void) inSignal;
    gGlobalQuit = true;
}

const uint32_t kAppSignature (NTV2_FOURCC('W','h','k','r'));

void clearConsole()
{
#if defined (MSWindows)
    system("cls");
#else
    system("clear");
#endif
}


int main (int argc, const char ** argv)
{
	uint32_t		dmaEngine			(NTV2_DMA1);	//  DMA engine argument
    uint32_t		dmaSize 			(0);        	//  DMA size argument
    uint32_t		dmaCount 			(0);        	//  DMA count argument
    char *			pDeviceSpec			(AJA_NULL);		//	Device argument
	int				doCapture			(0);			//	Do a capture (read from device)
    int				doLock              (0);			//	Prelock the buffer
    int				doShareDevice		(0);			//	Share device with other apps?
    int				showVersion			(0);			//	Show version info?
	poptContext		optionsContext;						//	Context for parsing command line arguments
	AJADebug::Open();

    //	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"version",		0,		POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"},
		{"engine",		'e',	POPT_ARG_INT,       &dmaEngine,		0,	"which DMA engine",			"1-6, 7=1stAvail"},
		{"capture",     'c',	POPT_ARG_NONE,		&doCapture,		0,	"read? (default is write)",	AJA_NULL},
        {"lock",        'l',	POPT_ARG_NONE,		&doLock,		0,	"prelock buffer?",			AJA_NULL},
        {"multi",		'm',	POPT_ARG_NONE,		&doShareDevice,	0,	"multi-instance?",			"if specified, share device"},
        {"size",		's',	POPT_ARG_INT,       &dmaSize,		0,	"DMA transfer size",		AJA_NULL},
        {"count",		'n',	POPT_ARG_INT,       &dmaCount,		0,	"max xfer count",			AJA_NULL},
        POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	const string deviceSpec (pDeviceSpec ? pDeviceSpec : "0");
    CNTV2Card device;
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(deviceSpec, device))
		{cerr << "## ERROR:  Device '" << deviceSpec << "' failed to open" << endl;  return 2;}

	if (dmaEngine < NTV2_DMA1 || dmaEngine > NTV2_DMA_FIRST_AVAILABLE)
	{
		cerr	<< "## ERROR:  Invalid DMA engine number '" << dmaEngine << "' -- expected " << int(NTV2_DMA1)
				<< " thru " << int(NTV2_DMA_FIRST_AVAILABLE) << endl;
		return 2;
	}
	if (dmaEngine >= NTV2_DMA1 && dmaEngine <= NTV2_DMA4)
		if (ULWord(dmaEngine) > ::NTV2DeviceGetNumDMAEngines(device.GetDeviceID()))
		{
			cerr	<< "## ERROR:  Invalid DMA engine '" << dmaEngine << "' for '" << device.GetDisplayName()
					<< "' -- engines 1 thru " << ::NTV2DeviceGetNumDMAEngines(device.GetDeviceID()) << " expected" << endl;
			return 2;
		}

	ULWord					appSignature	(0);
	int32_t					appPID			(0);
	NTV2EveryFrameTaskMode	savedTaskMode	(NTV2_TASK_MODE_INVALID);

	// Save the current device state
	// Who currently "owns" the device?
	device.GetEveryFrameServices(savedTaskMode);
	if (savedTaskMode == NTV2_STANDARD_TASKS  ||  !doShareDevice)
	{
		device.GetStreamingApplication(appSignature, appPID);
		if (!device.AcquireStreamForApplication(kAppSignature, int32_t(AJAProcess::GetPid())))
		{
			cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
			return AJA_STATUS_BUSY;  // Some other app is using the device
		}

		//	Set the OEM service level...
		device.SetEveryFrameServices(NTV2_OEM_TASKS);
		device.SetSuspendHostAudio(true);
	}
	::signal(SIGINT, SignalHandler);
#if defined (AJAMac)
	::signal(SIGHUP, SignalHandler);
	::signal(SIGQUIT, SignalHandler);
#endif

	NTV2Framesize	frameSize	(NTV2_MAX_NUM_Framesizes);
	device.GetFrameBufferSize	(NTV2_CHANNEL1, frameSize);
    const ULWord	byteCount	(dmaSize? dmaSize : ::NTV2FramesizeToByteCount(frameSize));
	const double	megaBytes	(double(byteCount) / 1024.0 / 1024.0);
	NTV2Buffer		buffer		(byteCount);
	const string	rw			(doCapture ? " READ " : " WRITE ");
	double			xferMin		(100000.0);
	double			xferMax		(0.0);
	double			xferTotal	(0.0);
    double          xferRate    (0.0);
	uint64_t		loopCount	(0);
	uint64_t		failureCount(0);
    AJATimer		updateTimer, dmaTimer(AJATimerPrecisionMicroseconds);

	if (doLock)
		device.DMABufferLock(buffer, /*alsoPrelockSGL*/true);

	buffer.Fill(UByte('x'));
    cout << endl;
	updateTimer.Start();
	while (!gGlobalQuit)
    {
		dmaTimer.Start();
		if (!device.DmaTransfer(NTV2DMAEngine(dmaEngine), doCapture, dmaEngine, buffer, 0, byteCount, true))
			failureCount++;

        const double	mbytesPerSecond	(megaBytes / (double(dmaTimer.ElapsedTime())/1000000.0));
		if (mbytesPerSecond > xferMax)
			xferMax = mbytesPerSecond;
		if (mbytesPerSecond < xferMin)
			xferMin = mbytesPerSecond;
		xferTotal += mbytesPerSecond;
        xferRate += (mbytesPerSecond - xferRate) / 10.0;

		//	Update status info to console every 30 ms or so...
        if (updateTimer.ElapsedTime() > 100  ||  loopCount == 0)
		{
			cout.setf(std::ios::fixed, std::ios::floatfield);
            cout	<< "DMA engine " << int(dmaEngine) << rw << DEC(byteCount) << " bytes  rate: "
                    << setprecision(2) << xferRate << " MB/sec  "
                    << setprecision(2) << xferRate/megaBytes << " xfers/sec\r" << flush;
            updateTimer.Start(); // reset output delay
		}
		loopCount++;
		if (dmaCount)							//	If dmaCount arg non-zero...
			if (ULWord(loopCount) >= dmaCount)	//	... and if loopCount >= dmaCount...
				break;							//	... then reached dmaCount limit -- exit
	}

	if (doLock)
		device.DMABufferUnlock(buffer, /*alsoUnlockSGL*/true);

	//	Report results...
	cout << endl;
	if (failureCount)
		cout << DEC(failureCount) << " of " << DEC(loopCount) << " DMA transfer(s) failed" << endl;
	cout.setf(std::ios::fixed, std::ios::floatfield);
	cout	<< "Max rate: " << setprecision(2) << xferMax << " MB/sec" << endl
			<< "Min rate: " << setprecision(2) << xferMin << " MB/sec" << endl
            << "Avg rate: " << setprecision(2) << double(xferTotal / double(loopCount)) << " MB/sec" << endl << endl;

	//	Cleanup...
	if (savedTaskMode == NTV2_STANDARD_TASKS  ||  !doShareDevice)
	{
		device.SetSuspendHostAudio(false);
		device.SetEveryFrameServices(savedTaskMode);
		device.ReleaseStreamForApplication(kAppSignature, int32_t(AJAProcess::GetPid()));
	}
    return 0;

}	//	main
