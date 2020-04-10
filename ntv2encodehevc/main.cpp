/**
	@file		ntv2encodehevc/main.cpp
	@brief		Demonstration application to capture frames from SDI input.
	@copyright	(C) 2012-2020 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ntv2utils.h"
#include "ajatypes.h"
#include "ntv2m31publicinterface.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/systemtime.h"
#include "ntv2encodehevc.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
static const M31VideoPreset kCodecPreset[] = {
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

	M31_FILE_4096X2160_420_10_5994p,
	M31_FILE_4096X2160_420_10_60p,
	M31_FILE_4096X2160_422_10_50p,
	M31_FILE_4096X2160_422_10_5994p_IF,
	M31_FILE_4096X2160_422_10_60p_IF,

	M31_VIF_1280X720_420_8_50p,
	M31_VIF_1280X720_420_8_5994p,
	M31_VIF_1280X720_420_8_60p,
	
	M31_VIF_1280X720_422_10_50p,
	M31_VIF_1280X720_422_10_5994p,
	M31_VIF_1280X720_422_10_60p,
	
	M31_VIF_1920X1080_420_8_50p,
	M31_VIF_1920X1080_420_8_5994p,
	M31_VIF_1920X1080_420_10_60p,
	
	M31_VIF_1920X1080_422_10_5994p,
	M31_VIF_1920X1080_422_10_60p,
	
	M31_VIF_3840X2160_420_8_50p,
	M31_VIF_3840X2160_420_8_5994p,
	M31_VIF_3840X2160_420_8_60p,
	
	M31_VIF_3840X2160_420_10_50p,
	M31_VIF_3840X2160_420_10_5994p,
	M31_VIF_3840X2160_420_10_60p,
	
	M31_VIF_3840X2160_422_10_50p,
	M31_VIF_3840X2160_422_10_5994p,
	M31_VIF_3840X2160_422_10_60p,
};

static const NTV2FrameBufferFormat kCodecFormat[] = {
	NTV2_FBF_8BIT_YCBCR_420PL2,
	NTV2_FBF_10BIT_YCBCR_420PL2,
	NTV2_FBF_8BIT_YCBCR_422PL2,
	NTV2_FBF_10BIT_YCBCR_422PL2
};

static const size_t	gNumCodecPresets	(sizeof(kCodecPreset)/sizeof(M31VideoPreset));
static const size_t	gNumCodecFormats	(sizeof(kCodecFormat)/sizeof(NTV2FrameBufferFormat));
static const size_t	kNoSelection		(1000000000);
static bool			gGlobalQuit			(false);	//	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}

static int printPresets (void)
{
	cout << "M31 Presets" << endl;
	for (size_t ndx(0);  ndx < gNumCodecPresets;  ndx++)
	{
		const string presetStr (::NTV2M31VideoPresetToString (kCodecPreset[ndx], true));
		if (!presetStr.empty())
			cout << ndx << ":  " << presetStr << endl;
	}
	return 0;
}

static int printFormats (void)
{
	cout << "M31 Formats" << endl;
	for (size_t ndx(0);  ndx < gNumCodecFormats;  ndx++)
	{
		const string pixFormatStr (::NTV2FrameBufferFormatToString (kCodecFormat[ndx], true));
		if (!pixFormatStr.empty())
			cout << ndx << ":  " << pixFormatStr << endl;
	}
	return 0;
}


int main (int argc, const char ** argv)
{
	M31VideoPreset			m31Preset		(M31_NUMVIDEOPRESETS);				//	Codec preset
	NTV2FrameBufferFormat	pixelFormat		(NTV2_FBF_NUMFRAMEBUFFERFORMATS);	//	Codec frame format
	char *					pWhatToList		(AJA_NULL);							//	Value of --list argument
	char *					pDeviceSpec		(AJA_NULL);							//	Device specifier string, if any
	size_t					codecFormat		(kNoSelection);						//	Codec format index
	size_t					codecPreset		(kNoSelection);						//	Codec preset index
	ULWord					channelNumber	(1);								//	Number of the input channel to use
	int						codecQuad		(0);								//	Codec quad mode
	int						timeCode		(0);								//	Timecode burn mode
	int						tsiData			(0);								//	Two sample interleave mode
	int						infoData		(0);								//	Picture and encoded information
	size_t					maxFrames		(kNoSelection);						//	Maximum number of catpured frames
	UWord					audioChannels	(0);								//	Number of audio channels
	poptContext				optionsContext;										//	Context for parsing command line arguments

	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"audio",		'a',	POPT_ARG_SHORT,			&audioChannels,	0,	"number of audio channels",		"1-16"},
		{"device",		'd',	POPT_ARG_STRING,		&pDeviceSpec,	0,	"device to use",				"index#, serial#, or model"},
		{"channel",		'c',	POPT_ARG_INT,			&channelNumber,	0,	"channel to use",				"1-4"},
		{"format",		'f',	POPT_ARG_INT,			&codecFormat,	0,	"format to use",				"use '-lf' for list"},
		{"preset",		'p',	POPT_ARG_INT,			&codecPreset,	0,	"codec preset to use",			"use '-lp' for list"},
		{"quad",		'q',	POPT_ARG_NONE,			&codecQuad,		0,	"interpret 4 inputs as UHD",	""},
		{"list",		'l',	POPT_ARG_STRING,		&pWhatToList,	0,	"list options",					"p or f (presets or formats)"},
		{"tcb",			'b',	POPT_ARG_NONE,			&timeCode,		0,	"add timecode burn to video",	""},
		{"tsi",			't',	POPT_ARG_NONE,			&tsiData,		0,	"two sample interleave mode",	""},
		{"info",		'i',	POPT_ARG_NONE,			&infoData,		0,	"write encoded info file",		""},
		{"maxframes",	'x',	POPT_ARG_INT |
								POPT_ARGFLAG_OPTIONAL,	&maxFrames,		0,	"limit output file frames",		""},
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	
	//	Read command line arguments...
	optionsContext = ::poptGetContext (AJA_NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);
	
	if (pWhatToList && string(pWhatToList) == "p")
		return printPresets ();
	else if (pWhatToList && string(pWhatToList) == "f")
		return printFormats ();
	else if (pWhatToList)
		{cerr << "## ERROR:  Invalid argument to --list option, expected 'p' or 'f'" << endl;  return 2;}

	if ((codecFormat != kNoSelection) && (codecFormat >= int(gNumCodecFormats)))
		{cerr << "## ERROR:  Invalid M31 format " << codecFormat << " -- expected 0 thru " << (gNumCodecFormats) << endl;  return 2;}

	if (codecFormat < int(gNumCodecFormats))
		pixelFormat = kCodecFormat[codecFormat];

	if ((codecPreset != kNoSelection) && (codecPreset >= int(gNumCodecPresets)))
		{cerr << "## ERROR:  Invalid M31 preset " << codecPreset << " -- expected 0 thru " << (gNumCodecPresets) << endl;  return 2;}

	if (codecPreset < int(gNumCodecPresets))
		m31Preset = kCodecPreset[codecPreset];

	const NTV2Channel inputChannel (::GetNTV2ChannelForIndex(channelNumber - 1));
	if (!NTV2_IS_VALID_CHANNEL(inputChannel)  ||  inputChannel > NTV2_CHANNEL4)
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 4" << endl;  return 2;}

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Instantiate the NTV2Capture object, using the specified AJA device...
	const uint32_t hevcMaxFrames (maxFrames >= kNoSelection ? 0xffffffff : uint32_t(maxFrames));
	NTV2EncodeHEVC hevcEncoder (pDeviceSpec ? string (pDeviceSpec) : "0", inputChannel,
								m31Preset, pixelFormat, codecQuad ? true : false,
								audioChannels, timeCode ? true : false,
								infoData ? true : false, tsiData ? true : false, hevcMaxFrames);
	
	//	Initialize the NTV2Capture instance...
	if (AJA_FAILURE(hevcEncoder.Init()))
		{cerr << "## ERROR:  Initialization failed" << endl;  return 1;}
	
	cout	<< endl
			<< "Capture: " << ::NTV2M31VideoPresetToString (hevcEncoder.GetCodecPreset(), false) << endl
			<< endl;

	//	Run the capturer...
	hevcEncoder.Run();
	
	cout	<< "           Capture  Capture" << endl
			<< "   Frames   Frames   Buffer" << endl
			<< "Processed  Dropped    Level" << endl;

	//	Loop til time to stop...
	do
	{
		AVHevcStatus inputStatus;
		hevcEncoder.GetStatus(inputStatus);
		cout << setw(9) << inputStatus.framesProcessed
			 << setw(9) << inputStatus.framesDropped
			 << setw(9) << inputStatus.bufferLevel
			 << "\r" << flush;
		AJATime::Sleep(1000);
	} while (!gGlobalQuit);
	cout << endl;
	
	//	Quit the capturer...
	hevcEncoder.Quit();

	return 0;
	
}	//	main
