/**
	@file		ntv2hdrsetup/main.cpp
	@brief		Demonstration application that shows how to enable HDR capabilities of 
                HDMI out.
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
#include "ntv2democommon.h"
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

    int              eotf            (0);                      //  Eotf to enable 0,1,2,3
    int              luminance       (0);                      //  Luminanace
    int              dolbyVision     (0);                      //  Enable dolby vision bit?
    int				noHdr           (0);						//	Disable hdr?

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
        {"device",      'd',	POPT_ARG_STRING,	&pDeviceSpec, 0,	"which device to use",     "index#, serial#, or model"	},
        {"eotf",        'e', POPT_ARG_INT,    &eotf,        0, "EOTF to use",             "0 (Trad Gamma SD), 1 (Trad Gamma HD), 2 (ST 2084), 3 (HLG)"},
        {"luminance",   'l', POPT_ARG_INT,    &luminance,   0, "luminance",               "0 (Non-Constant), 1 (Constant)"},
        {"dolbyvision",   0, POPT_ARG_NONE,   &dolbyVision, 1, "enable dolby vision bit", NULL},
        {"nohdr",         0, POPT_ARG_NONE,   &noHdr,       1, "disable hdr",             NULL},
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

    device.SetHDMIHDRConstantLuminance(luminance == 0 ? false : true);
    device.SetHDMIHDRElectroOpticalTransferFunction(uint8_t(eotf));
    device.EnableHDMIHDRDolbyVision(dolbyVision == 0 ? false : true);
    device.EnableHDMIHDR(noHdr == 1 ? false : true);

    // Loop until a key is pressed, that way user can inspect the changes with watcher
    CNTV2DemoCommon::WaitForEnterKeyPress();

    device.SetEveryFrameServices (savedTaskMode);										//	Restore prior service level
    device.ReleaseStreamForApplication (kAppSignature, static_cast <uint32_t> (AJAProcess::GetPid ()));	//	Release the device

	return AJA_SUCCESS (status) ? 0 : 2;	//	Return zero upon success -- otherwise 2

}	//	main
