/**
	@file		ntv2hdrsetup/main.cpp
	@brief		Demonstration application that shows how to enable HDR capabilities of 
				the HDMI out.
	@copyright	Copyright (C) 2012-2017 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/common/types.h"
#include "ajabase/system/process.h"
#include "ajabase/system/systemtime.h"
#include "ntv2card.h"
#include "ntv2devicescanner.h"
//#include "ntv2hdrsetup.h"
#include <signal.h>
#include <iostream>
#include <iomanip>


//	Globals
static bool	gGlobalQuit		(false);	///	Set this "true" to exit gracefully

const uint32_t kAppSignature (AJA_FOURCC ('H','d','r','s'));

static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


/**
	@brief		Main entry point for 'ntv2hdrsetup' demo application.
	@param[in]	argc	Number arguments specified on the command line, including the path to the executable.
	@param[in]	argv	Array of 'const char' pointers, one for each argument.
	@return		Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	AJAStatus		status			(AJA_STATUS_SUCCESS);		//	Result status
	char *			pDeviceSpec		(NULL);						//	Which device to use
	char *			pVidSource		(NULL);						//	Video input source string
	char *			pTcSource		(NULL);						//	Time code source string
	int				noAudio			(0);						//	Disable audio?
	int				useRGB			(0);						//	Use 10-bit RGB instead of 8-bit YCbCr?
	poptContext		optionsContext;								//	Context for parsing command line arguments
	NTV2InputSource	vidSource		(NTV2_INPUTSOURCE_SDI1);	//	Video source
	NTV2TCIndex		tcSource		(NTV2_TCINDEX_SDI1);		//	Time code source
	int				doMultiChannel	(0);						//  Set the board up for multi-channel/format

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"index#, serial#, or model"	},
        //{"input",		'i',	POPT_ARG_STRING,	&pVidSource,	0,	"video input",						"{'?' to list}"		},
        //{"tcsource",	't',	POPT_ARG_STRING,	&pTcSource,		0,	"time code source",					"{'?' to list}"		},
        //{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",					NULL},
        //{"rgb",			0,		POPT_ARG_NONE,		&useRGB,		0,	"use RGB10 frame buffer?",			NULL},
        //{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multichannel/multiformat?",	NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	if (::poptGetNextOpt (optionsContext) < -1)
	{
		cerr << "## ERROR:  Bad command line argument(s)" << endl;
		return 1;
	}
	optionsContext = ::poptFreeContext (optionsContext);
/*
    //	Select video source...
    if (pVidSource)
    {
        const string	videoSource	(CNTV2DemoCommon::ToLower (pVidSource));
        if (gSourceMap.find (videoSource) == gSourceMap.end ())
            {cerr << "## ERROR:  Video source '" << videoSource << "' not one of these: " << gSourceMap << endl;	return 1;}
        vidSource = gSourceMap [videoSource];
    }	//	if video source specified

    //	Select time code source...
    if (pTcSource)
    {
        const string	timecodeSource	(CNTV2DemoCommon::ToLower (pTcSource));
        if (gTCSourceMap.find (timecodeSource) == gTCSourceMap.end ())
            {cerr << "## ERROR:  Timecode source '" << timecodeSource << "' not one of these: " << gTCSourceMap << endl;	return 1;}
        tcSource = gTCSourceMap [timecodeSource];
    }*/

    /*
	//	Instantiate the NTV2Burn object...
	NTV2LLBurn	burner (pDeviceSpec ? pDeviceSpec : "0",					//	Which device?
						(noAudio ? false : true),							//	Include audio?
						useRGB ? NTV2_FBF_10BIT_RGB : NTV2_FBF_8BIT_YCBCR,	//	Use RGB frame buffer format?
						vidSource,											//	Which video input source?
						tcSource,											//	Which time code source?
						doMultiChannel ? true : false);						//  Set the board up for multi-channel/format
*/
	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

    CNTV2Card device;
    ULWord	appSignature	(0);
    int32_t	appPID			(0);
    NTV2EveryFrameTaskMode savedTaskMode;

    //	Open the device...
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (pDeviceSpec ? pDeviceSpec : "0", device))
        {cerr << "## ERROR:  Device '" << pDeviceSpec << "' not found" << endl;  return 2;}

    if (!device.IsDeviceReady ())
        {cerr << "## ERROR:  Device '" << pDeviceSpec << "' not ready" << endl;  return 2;}


    device.GetEveryFrameServices (savedTaskMode);				//	Save the current device state
    device.GetStreamingApplication (&appSignature, &appPID);	//	Who currently "owns" the device?

    if (!device.AcquireStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ())))
    {
        cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
        return AJA_STATUS_BUSY;		//	Some other app is using the device
    }
    device.SetEveryFrameServices (NTV2_OEM_TASKS);			//	Set the OEM service level

    // setup HDR values based on passed args here


    // Loop until ctrl-c is used, that way user can inspect the changes with watcher
    // if they want
    while(gGlobalQuit == false)
    {
        AJATime::Sleep (250);
    }

    device.SetEveryFrameServices (savedTaskMode);										//	Restore prior service level
    device.ReleaseStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ()));	//	Release the device

    /*
	//	Initialize the NTV2Burn instance...
	status = burner.Init ();
	if (AJA_SUCCESS (status))
	{
		//	Start the burner's capture and playout threads...
		burner.Run ();

		//	Loop until told to stop...
		cout	<< "   Frames   Frames" << endl
				<< "Processed  Dropped" << endl;
		while (!gGlobalQuit)
		{
			ULWord	framesProcessed, framesDropped;
			burner.GetStatus (framesProcessed, framesDropped);
			cout	<< setw (9) << framesProcessed
					<< setw (9) << framesDropped
					<< "\r" << flush;
			AJATime::Sleep (2000);
		}	//	loop until signaled

		cout << endl;
	}
	else
		cerr << "## ERROR:  Initialization failed, status=" << status << endl;
    */

	return AJA_SUCCESS (status) ? 0 : 2;	//	Return zero upon success -- otherwise 2

}	//	main
