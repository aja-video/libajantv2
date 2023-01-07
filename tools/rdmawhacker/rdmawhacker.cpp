/**
    @file       rdmawhacker/main.cpp
    @brief      Simple command line application to test DMA read/write speeds to RDMA devices.
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

#ifdef AJA_RDMA
#include <cuda.h>
#include <cuda_runtime_api.h>
#endif

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

int main(int argc, const char ** argv)
{
	uint32_t		dmaEngine			(NTV2_DMA1);	//  DMA engine argument
    uint32_t		dmaNum	 			(0);        	//  number of DMA buffers
    uint32_t		dmaSize 			(0);        	//  DMA size argument
    char *			pDeviceSpec			(NULL);			//	Device argument
	int				doCapture			(0);			//	Do a capture (read from device)
    int				doMultiInstance		(0);			//	Multi-instance mode (share with other apps)
	poptContext		optionsContext;						//	Context for parsing command line arguments

    //	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device",				"index#, serial#, or model"},
		{"engine",		'e',	POPT_ARG_INT,       &dmaEngine,			0,	"which DMA engine",			"1-6, 7=1stAvail"},
		{"capture",     'c',	POPT_ARG_NONE,		&doCapture,			0,	"capture/read instead of play/write default", NULL},
        {"multi",		'm',	POPT_ARG_NONE,		&doMultiInstance,	0,	"multi-instance",			NULL},
        {"numbuf",		'n',	POPT_ARG_INT,       &dmaNum,			0,	"number of buffers", NULL},
        {"size",		's',	POPT_ARG_INT,       &dmaSize,			0,	"DMA transfer size", NULL},
        POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	if (dmaNum == 0)
		dmaNum = 4;

	const string			deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
    CNTV2Card device;
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(deviceSpec, device))
	{
		cerr	<< "## ERROR:  Device '" << deviceSpec << "' failed to open" << endl;
		return 2;
	}

	if (dmaEngine < NTV2_DMA1 || dmaEngine > NTV2_DMA_FIRST_AVAILABLE)
	{
		cerr << "## ERROR:  Invalid DMA engine number '" << dmaEngine << "' -- expected " << int(NTV2_DMA1) << " thru " << int(NTV2_DMA_FIRST_AVAILABLE) << endl;
		return 2;
	}
	if (dmaEngine >= NTV2_DMA1 && dmaEngine <= NTV2_DMA4)
		if (ULWord(dmaEngine) > ::NTV2DeviceGetNumDMAEngines(device.GetDeviceID()))
		{
			cerr << "## ERROR:  Invalid DMA engine '" << dmaEngine << "' for '" << device.GetDisplayName() << "' -- engines 1 thru " << ::NTV2DeviceGetNumDMAEngines(device.GetDeviceID()) << " expected" << endl;
			return 2;
		}

	ULWord					appSignature	(0);
	int32_t					appPID			(0);
	NTV2EveryFrameTaskMode	savedTaskMode	(NTV2_TASK_MODE_INVALID);

	// Save the current device state
	// Who currently "owns" the device?
	device.GetEveryFrameServices(savedTaskMode);
	if (savedTaskMode == NTV2_STANDARD_TASKS  ||  !doMultiInstance)
	{
		device.GetStreamingApplication(appSignature, appPID);
		if (!device.AcquireStreamForApplication(kAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
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
	const string	rw			(doCapture ? " READ " : " WRITE ");
	ULWord**		pBuffer		(NULL);
	double			xferMin		(100000.0);
	double			xferMax		(0.0);
	double			xferTotal	(0.0);
    double          xferRate    (0.0);
	uint64_t		loopCount	(0);
	uint64_t		failureCount(0);
    AJATimer		updateTimer, dmaTimer(AJATimerPrecisionMicroseconds);

#ifdef AJA_RDMA
	unsigned int flag = 1;
	cudaError_t ce;
	CUresult cr;

	pBuffer = new ULWord*[dmaNum];
	memset(pBuffer, 0, sizeof(ULWord*) * dmaNum);

	for (uint32_t i = 0; i < dmaNum; i++)
	{
#ifdef AJA_IGPU
		ce = cudaHostAlloc((void**)&pBuffer[i], byteCount, cudaHostAllocDefault);
#else
		ce = cudaMalloc((void**)&pBuffer[i], byteCount);
#endif
		if (ce != cudaSuccess)
		{
			cerr << "## ERROR: Allocation of GPU buffer failed " << (int)ce << endl;
			return 0;
		}

		cr = cuPointerSetAttribute(&flag, CU_POINTER_ATTRIBUTE_SYNC_MEMOPS,	(CUdeviceptr)pBuffer[i]);
		if (cr != CUDA_SUCCESS)
		{
			cerr << "## ERROR: Cuda set attribute failed " << (int)cr << endl;
			return 0;
		}

		bool bSuccess = device.DMABufferLock(pBuffer[i], byteCount, true, true);
		if (!bSuccess)
		{
			cerr << "## ERROR: GPU buffer lock failed" << endl;
			return 0;
		}
	}
#else	
	cerr << "## ERROR:  Not built with AJA_RDMA" << endl;
	return 0;
#endif	

    cout << endl;
	updateTimer.Start();
	while (!gGlobalQuit)
    {
		dmaTimer.Start();
		ULWord bufIdx = loopCount%dmaNum; 
		if (!device.DmaTransfer(NTV2DMAEngine(dmaEngine), doCapture, dmaEngine, pBuffer[bufIdx], bufIdx, byteCount, true))
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

#ifdef AJA_RDMA
		// touch the cuda buffer to prevent low power mode
		ULWord data;
		cudaMemcpy((void*)&data, (void*)pBuffer[bufIdx], sizeof(ULWord), cudaMemcpyDeviceToHost);
#endif		
		loopCount++;
	}

	device.DMABufferUnlockAll();

#ifdef AJA_RDMA
	for (uint32_t i = 0; i < dmaNum; i++)
	{
		if (pBuffer[i] != NULL)
		{
#ifdef AJA_IGPU
			cudaFreeHost(pBuffer[i]);
#else
			cudaFree(pBuffer[i]);
#endif
		}
	}
#endif

	if (pBuffer != NULL)
		delete [] pBuffer;
					
	//	Report results...
	cout << endl;
	if (failureCount)
		cout << DEC(failureCount) << " of " << DEC(loopCount) << " DMA transfer(s) failed" << endl;
	cout.setf(std::ios::fixed, std::ios::floatfield);
	cout	<< "Max rate: " << setprecision(2) << xferMax << " MB/sec" << endl
			<< "Min rate: " << setprecision(2) << xferMin << " MB/sec" << endl
            << "Avg rate: " << setprecision(2) << double(xferTotal / double(loopCount)) << " MB/sec" << endl << endl;

	//	Cleanup...
	if (savedTaskMode == NTV2_STANDARD_TASKS  ||  !doMultiInstance)
	{
		device.SetSuspendHostAudio(false);
		device.SetEveryFrameServices(savedTaskMode);
		device.ReleaseStreamForApplication(kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));
	}
    return 0;

}	//	main
