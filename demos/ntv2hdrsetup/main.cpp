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
static bool gGlobalQuit (false);  /// Set this "true" to exit gracefully

const uint32_t kAppSignature (NTV2_FOURCC('H','d','r','s'));

static void SignalHandler (int inSignal)
{
    (void) inSignal;
    gGlobalQuit = true;
}


/**
    @brief      Main entry point for 'ntv2hdrsetup' demo application.
    @param[in]  argc    Number arguments specified on the command line, including the path to the executable.
    @param[in]  argv    Array of 'const char' pointers, one for each argument.
    @return     Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
    AJAStatus   status          (AJA_STATUS_SUCCESS);  // Result status
    char *      pDeviceSpec     (NULL);                // Which device to use
    poptContext optionsContext;                        // Context for parsing command line arguments
    int         eotf            (0);                   // Eotf to enable 0,1,2,3
    int         constluminance  (0);                   // Luminanace
    int         dolbyVision     (0);                   // Enable dolby vision bit?
    int         noHdr           (0);                   // Disable hdr?

    // Command line option descriptions:
    const struct poptOption userOptionsTable [] =
    {
        {"device",     'd', POPT_ARG_STRING, &pDeviceSpec,    0, "which device to use",     "index#, serial#, or model"},
        {"eotf",       'e', POPT_ARG_INT,    &eotf,           0, "EOTF to use",             "0 (Trad Gamma SDR), 1 (Trad Gamma HDR), 2 (ST 2084), 3 (HLG)"},
        {"luminance",  'l', POPT_ARG_INT,    &constluminance, 0, "luminance",               "0 (Non-Constant), 1 (Constant)"},
        {"dolbyvision",  0, POPT_ARG_NONE,   &dolbyVision,    1, "enable dolby vision bit", NULL},
        {"nohdr",        0, POPT_ARG_NONE,   &noHdr,          1, "disable HDMI HDR out",    NULL},
        POPT_AUTOHELP
        POPT_TABLEEND
    };

    // Read command line arguments...
    optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
    if (::poptGetNextOpt (optionsContext) < -1)
        {cerr << "## ERROR:  Bad command line argument(s)" << endl; return 1;}

    optionsContext = ::poptFreeContext (optionsContext);

    ::signal (SIGINT, SignalHandler);
    #if defined (AJAMac)
        ::signal (SIGHUP, SignalHandler);
        ::signal (SIGQUIT, SignalHandler);
    #endif

    CNTV2Card device;
    ULWord  appSignature (0);
    int32_t appPID       (0);
    NTV2EveryFrameTaskMode savedTaskMode;

    // Open the device...
    if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument (pDeviceSpec ? pDeviceSpec : "0", device))
        {cerr << "## ERROR:  Device '" << pDeviceSpec << "' not found" << endl; return 2;}

    if (!device.IsDeviceReady (false))
        {cerr << "## ERROR:  Device '" << pDeviceSpec << "' not ready" << endl; return 2;}

    if (!NTV2DeviceCanDoHDMIHDROut (device.GetDeviceID()))
        {cerr << "## ERROR:  Device '" << pDeviceSpec << "' does not support HDMI HDR" << endl; return 2;}

    if (eotf < 0 || eotf > 3)
        {cerr << "## ERROR:  valid eotf values are 0, 1, 2 or 3" << endl; return 2;}

    device.GetEveryFrameServices (savedTaskMode);			// Save the current device state
    device.GetStreamingApplication (appSignature, appPID);	// Who currently "owns" the device?

    if (!device.AcquireStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid())))
    {
        cerr << "## ERROR:  Unable to acquire device because another app (pid " << appPID << ") owns it" << endl;
        return AJA_STATUS_BUSY;  // Some other app is using the device
    }
    device.SetEveryFrameServices (NTV2_OEM_TASKS);  // Set the OEM service level

    // load up the digital primitives with some reasonable values
    HDRRegValues registerValues;
    setHDRDefaultsForBT2020(registerValues);
    registerValues.electroOpticalTransferFunction = uint8_t(eotf);
    device.SetHDRData(registerValues);

    // setup HDR values based on passed args
    device.SetHDMIHDRConstantLuminance(constluminance == 0 ? false : true);
    device.SetHDMIHDRElectroOpticalTransferFunction(uint8_t(eotf));

    // Enabling this will allow dolby vision containing frames to properly display out of HDMI
    device.EnableHDMIHDRDolbyVision(dolbyVision == 0 ? false : true);

    // The master switch for HDMI HDR output
    device.EnableHDMIHDR(noHdr == 1 ? false : true);

    // Loop until a key is pressed, that way user can inspect the changes with watcher
    CNTV2DemoCommon::WaitForEnterKeyPress();

    device.SetEveryFrameServices (savedTaskMode);  // Restore prior service level
    device.ReleaseStreamForApplication (kAppSignature, static_cast<int32_t>(AJAProcess::GetPid()));  // Release the device

    return AJA_SUCCESS (status) ? 0 : 2;  // Return zero upon success -- otherwise 2

}  // main
