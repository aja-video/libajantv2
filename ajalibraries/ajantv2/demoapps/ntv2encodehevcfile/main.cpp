/**
 @file		ntv2encodehevcfile/main.cpp
 @brief		Demonstration application to capture frames from SDI input.
 @copyright	Copyright (C) 2012-2015 AJA Video Systems, Inc.  All rights reserved.
 **/


//	Includes
#include "ntv2utils.h"
#include "ajatypes.h"
#include "ntv2m31publicinterface.h"
#include "ajastuff/common/options_popt.h"
#include "ajastuff/system/systemtime.h"
#include "../democlasses/ntv2encodehevcfile.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

const int kNoSelection      (1000000000);


//	Globals
static bool	gGlobalQuit 	(false);	//	Set this "true" to exit gracefully
static int	printPresets 	(void);
static int  printFormats    (void);

static int gNumCodecPresets = 0;
const M31VideoPreset kCodecPreset[] = {
    M31_FILE_1280X720_420_8_50p,
	M31_FILE_1280X720_420_8_5994p,
    M31_FILE_1280X720_420_8_60p,

    M31_FILE_1280X720_422_10_50p,
    M31_FILE_1280X720_422_10_5994p,
    M31_FILE_1280X720_422_10_60p,

    M31_FILE_1920X1080_420_8_2398p,
    M31_FILE_1920X1080_420_8_24p,
    M31_FILE_1920X1080_420_8_25p,
    M31_FILE_1920X1080_420_8_2997p,
    M31_FILE_1920X1080_420_8_30p,
    M31_FILE_1920X1080_420_8_50i,
    M31_FILE_1920X1080_420_8_5994i,
    M31_FILE_1920X1080_420_8_50p,
	M31_FILE_1920X1080_420_8_5994p,
    M31_FILE_1920X1080_420_8_60p,

    M31_FILE_1920X1080_422_10_2398p,
    M31_FILE_1920X1080_422_10_24p,
    M31_FILE_1920X1080_422_10_25p,
    M31_FILE_1920X1080_422_10_2997p,
    M31_FILE_1920X1080_422_10_30p,
    M31_FILE_1920X1080_422_10_50i,
    M31_FILE_1920X1080_422_10_5994i,
    M31_FILE_1920X1080_422_10_50p,
    M31_FILE_1920X1080_422_10_5994p,
    M31_FILE_1920X1080_422_10_60p,

    M31_FILE_2048X1080_420_8_2398p,
    M31_FILE_2048X1080_420_8_24p,
    M31_FILE_2048X1080_420_8_25p,
    M31_FILE_2048X1080_420_8_2997p,
    M31_FILE_2048X1080_420_8_30p,
    M31_FILE_2048X1080_420_8_50p,
	M31_FILE_2048X1080_420_8_5994p,
    M31_FILE_2048X1080_420_8_60p,

    M31_FILE_2048X1080_422_10_2398p,
    M31_FILE_2048X1080_422_10_24p,
    M31_FILE_2048X1080_422_10_25p,
    M31_FILE_2048X1080_422_10_2997p,
    M31_FILE_2048X1080_422_10_30p,
    M31_FILE_2048X1080_422_10_50p,
    M31_FILE_2048X1080_422_10_5994p,
    M31_FILE_2048X1080_422_10_60p,

    M31_FILE_3840X2160_420_8_2398p,
    M31_FILE_3840X2160_420_8_24p,
    M31_FILE_3840X2160_420_8_25p,
    M31_FILE_3840X2160_420_8_2997p,
    M31_FILE_3840X2160_420_8_30p,
    M31_FILE_3840X2160_420_8_50p,
	M31_FILE_3840X2160_420_8_5994p,
    M31_FILE_3840X2160_420_8_60p,

    M31_FILE_3840X2160_420_10_50p,
    M31_FILE_3840X2160_420_10_5994p,
    M31_FILE_3840X2160_420_10_60p,

    M31_FILE_3840X2160_422_8_50p,
    M31_FILE_3840X2160_422_8_5994p,
    M31_FILE_3840X2160_422_8_60p,

    M31_FILE_3840X2160_422_10_2398p,
    M31_FILE_3840X2160_422_10_24p,
    M31_FILE_3840X2160_422_10_25p,
    M31_FILE_3840X2160_422_10_2997p,
    M31_FILE_3840X2160_422_10_30p,
    M31_FILE_3840X2160_422_10_50p,
    M31_FILE_3840X2160_422_10_5994p,
    M31_FILE_3840X2160_422_10_60p,
};

static int gNumCodecFormats = 0;
const NTV2FrameBufferFormat kCodecFormat[] = {
    NTV2_FBF_8BIT_YCBCR_420PL,
    NTV2_FBF_10BIT_YCBCR_420PL,
    NTV2_FBF_8BIT_YCBCR_422PL,
    NTV2_FBF_10BIT_YCBCR_422PL
};

static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
    AJAStatus               status			(AJA_STATUS_SUCCESS);
    M31VideoPreset          m31Preset		(M31_NUMVIDEOPRESETS);              //  Codec preset
    NTV2Channel             inputChannel    (NTV2_MAX_NUM_CHANNELS);            //  Input channel
    char *                  pWhatToList		(NULL);                             //	Value of --list argument
    char *                  pDeviceSpec		(NULL);                             //	Device specifier string, if any
    int                     codecFormat     (kNoSelection);                     //  Codec format index
    int                     codecPreset     (kNoSelection);                     //  Codec preset index
    int                     channelNumber	(kNoSelection);                     //	Number of the input channel to use
    int                     infoData        (0);                                //  Picture and encoded information
    poptContext             optionsContext;                                     //	Context for parsing command line arguments
    AVHevcStatus            inputStatus;
	
    gNumCodecPresets = sizeof(kCodecPreset)/sizeof(M31VideoPreset);
    gNumCodecFormats = sizeof(kCodecFormat)/sizeof(NTV2FrameBufferFormat);

	//
	//	Command line option descriptions:
	//
    const struct poptOption userOptionsTable [] =
    {
        {"device",		'd',	POPT_ARG_STRING,        &pDeviceSpec,	0,	"device to use",                "index#, serial#, or model"},
        {"channel",	    'c',	POPT_ARG_INT,           &channelNumber,	0,	"channel to use",               "1-4"},
        {"format",		'f',	POPT_ARG_INT,           &codecFormat,	0,	"format to use",                "use '-lf' for list"},
        {"preset",		'p',	POPT_ARG_INT,           &codecPreset,	0,	"codec preset to use",          "use '-lp' for list"},
        {"list",	    'l',	POPT_ARG_STRING,        &pWhatToList,	0,	"list options",                 "p or f (presets or formats)"},
        {"info",        'i',	POPT_ARG_NONE,          &infoData,      0,	"write encoded info file",      ""},
        POPT_AUTOHELP
		POPT_TABLEEND
	};
	
	//
	//	Read command line arguments...
	//
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);
	
    if (pWhatToList && string (pWhatToList) == "p")
		return printPresets ();
    else if (pWhatToList && string (pWhatToList) == "f")
        return printFormats ();
    else if (pWhatToList)
        { cerr << "## ERROR:  Invalid argument to --list option, expected 'p' or 'f'" << endl;  return 2; }

    if ((codecFormat != kNoSelection) && (codecFormat >= gNumCodecFormats))
        { cerr << "## ERROR:  Invalid M31 format " << codecFormat << " -- expected 0 thru " << (gNumCodecFormats) << endl;  return 2; }

    if ((codecPreset != kNoSelection) && (codecPreset >= gNumCodecPresets))
        { cerr << "## ERROR:  Invalid M31 preset " << codecPreset << " -- expected 0 thru " << (gNumCodecPresets) << endl;  return 2; }

    if (codecPreset < gNumCodecPresets)
        m31Preset = kCodecPreset[codecPreset];

    switch (channelNumber)
	{
    case kNoSelection: { break; }
    case 1: { inputChannel = NTV2_CHANNEL1; break; }
    case 2: { inputChannel = NTV2_CHANNEL2; break; }
    case 3: { inputChannel = NTV2_CHANNEL3; break; }
    case 4: { inputChannel = NTV2_CHANNEL4; break; }
    default: { cerr << "## ERROR:  Invalid channel " << inputChannel << " -- expected 1 thru 4" << endl;  return 2; }
	}

    ::signal (SIGINT, SignalHandler);
#if defined (AJAMac)
    ::signal (SIGHUP, SignalHandler);
    ::signal (SIGQUIT, SignalHandler);
#endif

    //
    //	Instantiate the NTV2EncodeHEVCFile object, using the specified AJA device (note we don't do audio in file to file or timecode burn)
    //
    NTV2EncodeHEVCFile hevcFileEncoder (pDeviceSpec ? string (pDeviceSpec) : "0", inputChannel,
                                        RAWFILEPATH, RAWFILEWIDTH, RAWFILEHEIGHT, M31PRESET);
        
    //	Initialize the NTV2EncodeHEVCFile instance
    status = hevcFileEncoder.Init ();
    if (!AJA_SUCCESS (status))
    { cerr << "## ERROR:  Capture initialization failed with status " << status << endl;	return 1; }
        
    //  Get the current codec preset
    m31Preset = hevcFileEncoder.GetCodecPreset ();
        
    const string presetStr (::NTV2M31VideoPresetToString (m31Preset, false));
    cout << endl << "Capture: " << presetStr << endl << endl;
        
    //	Run the capturer
    hevcFileEncoder.Run ();
        
    //	Poll its status until stopped
    bool						firstTimeAround	(true);
        
    //	Loop until someone tells us to stop
    while (!gGlobalQuit)
    {
        hevcFileEncoder.GetStatus (&inputStatus);
        if (firstTimeAround)
        {
            cout << "           Capture  Capture" << endl
            << "   Frames   Frames   Buffer" << endl
            << "Processed  Dropped    Level" << endl;
            firstTimeAround = false;
        }
            
        cout << setw (9) << inputStatus.framesProcessed
        << setw (9) << inputStatus.framesDropped
        << setw (9) << inputStatus.bufferLevel
        << "\r" << flush;
            
        AJATime::Sleep (1000);
            
    }	//	loop til quit time
        
    cout << endl;
        
    //	Quit the capturer
    hevcFileEncoder.Quit ();
	
	return AJA_SUCCESS (status) ? 0 : 1;
	
}	//	main


static int printPresets (void)
{
	cout << "M31 Presets" << endl;
	for (int ndx (0); ndx < gNumCodecPresets; ndx++)
	{
        const string presetStr (::NTV2M31VideoPresetToString (kCodecPreset[ndx], true));
		if (!presetStr.empty ())
			cout << ndx << ":  " << presetStr << endl;
	}

	return 0;
}


static int printFormats (void)
{
    cout << "M31 Formats" << endl;
    for (int ndx (0); ndx < gNumCodecFormats; ndx++)
    {
        const string pixFormatStr (::NTV2FrameBufferFormatToString (kCodecFormat[ndx], true));
        if (!pixFormatStr.empty ())
            cout << ndx << ":  " << pixFormatStr << endl;
    }
    return 0;
}
