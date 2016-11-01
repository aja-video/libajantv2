/**
	@file		main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout frames to SDI output
				generated in host memory containing test pattern and timecode, including audio tone.
	@copyright	Copyright (C) 2012-2014 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ntv2utils.h"
#include "ajastuff/common/options_popt.h"
#include "../qaclasses/qaplayer.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#include "ajastuff/system/systemtime.h"


//	Globals
static bool			gGlobalQuit		(false);	//	Set this "true" to exit gracefully
static const string	gDescription	("NTV2 demonstration application that uses AutoCirculate to playout frames to SDI output\n"
									"generated in host memory containing test pattern and timecode, with or without audio\n"
									"tone. Every few seconds, the test pattern and tone frequency changes.");

static int printPixelFormats (void);
static int printVideoFormats (void);


void SignalHandler (int signal)
{
	gGlobalQuit = true;
	AJATime::Sleep(2000);
}


int main (int argc, const char ** argv)
{
	AJAStatus				status				(AJA_STATUS_SUCCESS);
	NTV2VideoFormat			videoFormat			(NTV2_FORMAT_UNKNOWN);
	NTV2FrameBufferFormat	pixelFormat			(NTV2_FBF_8BIT_YCBCR);
	char *					pWhatToList			(NULL);					//	Value of --list argument
	char *					pReference			(NULL);					//	Value of --ref argument
    uint32_t				deviceIndex 		(0);					//	Which board to use
	uint32_t				channelNumber		(1);					//	Number of the channel to use, 1 thru 8
	uint32_t				withAudio			(0);					//	With audio tone?
	uint32_t				withBurn			(0);					//	Burn timecode into frame
	uint32_t				bufferSize			(0);					//  Set buffer size to user input x2
	uint32_t				useDL				(0);					//	Use DL vs CSC
	uint32_t				useAB				(0);					//	Use DL vs CSC
	uint32_t				insertAncLine		(0);					//	Line number on which to inset Anc data
	uint32_t				numberOfAnc			(0);					//	Number of 240 byte Anc packets to insert
	uint32_t				fieldSize			(0);					//	Size of Anc field buffers
	uint32_t				nonPCMAudio			(0);					//	Non-PCM audio pair testing?
	uint32_t				pktsPerLine			(8);					//	Number of packets to insert per line
	poptContext				optionsContext; 							//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",	    'b',	POPT_ARG_INT,	&deviceIndex,	0,	"Device to use",				"0-based device index"  	},
		{"channel",	    'c',	POPT_ARG_INT,	&channelNumber,	0,	"Channel to use",				"1 thru 8"					},
		{"list",	    'l',	POPT_ARG_STRING,&pWhatToList,	0,	"Format to list",				"vf|pf"						},
		{"videoFormat",	'v',	POPT_ARG_INT,	&videoFormat,	0,	"Video format to use",			"use -lvf to list"			},
		{"pixelFormat",	'p',	POPT_ARG_INT,	&pixelFormat,	0,	"Pixel format to use",			"use -lpf to list"			},
		{"audio",	    'a',	POPT_ARG_NONE,	&withAudio,		0,	"Add audio tone?",				NULL						},
		{"burn",	    't',	POPT_ARG_NONE,	&withBurn,		0,	"Add TC burn?",					NULL						},
		{"ref",		    'r',	POPT_ARG_STRING,&pReference,	0,	"Ref source to use",			"free|ref|hdmi|sdi1...sdiN"	},
		{"size",		's',	POPT_ARG_INT,   &bufferSize,	0,	"Size x 2 = buffer size",		NULL						},
		{"useDL",		'd',	POPT_ARG_NONE,  &useDL,			0,	"Use DL (not CSC) for RGB",		NULL						},
		{"useAB",		'g',	POPT_ARG_NONE,  &useAB,			0,	"Use 3Ga->3Gb outconverters",	NULL						},
		{"insertAnc",	'i',	POPT_ARG_INT,   &insertAncLine,	0,	"Insert Anc data line number",	NULL						},
		{"numberOfAnc",	'n',	POPT_ARG_INT,   &numberOfAnc,	0,	"# of 240-byte Anc packets",	NULL						},
		{"fieldSize",	'f',	POPT_ARG_INT,   &fieldSize,		0,	"Size of Anc field buffers",	NULL						},
		{"nonpcm",		0,		POPT_ARG_INT,   &nonPCMAudio,	0,	"Add non-PCM audio chl pairs?",	NULL						},
		{"pktsPerLine",	'k',	POPT_ARG_INT,   &pktsPerLine,	0,	"Max number of pkts per line",	NULL						},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	if (pWhatToList && string (pWhatToList) == "vf")
		return printVideoFormats ();

	if (pWhatToList && string (pWhatToList) == "pf")
		return printPixelFormats ();

	const string		referenceInput	(pReference ? pReference : "free");
	NTV2ReferenceSource	referenceSource	(NTV2_REFERENCE_FREERUN);
	if (referenceInput == "free")
		;
	else if (referenceInput == "ref")
		referenceSource = NTV2_REFERENCE_EXTERNAL;
	else if (referenceInput.substr (0, 3) == "sdi" && referenceInput.length () == 4)
	{
		const UWord					sdiInputNum (referenceInput [3] - '0');	//	'sdi0' yields 0, 'sdi1' yields 1, ...
		const NTV2ReferenceSource	sdiRefs		[] = {	NTV2_REFERENCE_FREERUN,	NTV2_REFERENCE_INPUT1, NTV2_REFERENCE_INPUT2, NTV2_REFERENCE_INPUT3, NTV2_REFERENCE_INPUT4,
														NTV2_REFERENCE_INPUT5, NTV2_REFERENCE_INPUT6, NTV2_REFERENCE_INPUT7, NTV2_REFERENCE_INPUT8, NTV2_REFERENCE_FREERUN};
		if (sdiInputNum > 8)
			{cerr << "## ERROR:  Bad SDI input number " << sdiInputNum << ", must be between 1 and 8" << endl; return 2;}
		referenceSource = sdiRefs [sdiInputNum];
	}
	else
		{cerr << "## ERROR:  Bad reference source '" << referenceInput << "'" << endl; return 2;}

	QAPlayerControl	control;
	control.deviceIndex			= deviceIndex;
	control.withAudio			= (withAudio ? true : false);
	control.channel				= NTV2Channel (channelNumber - 1);
	control.pixelFormat			= pixelFormat;
	control.outputDestination	= ::NTV2ChannelToOutputDestination (control.channel);
	control.videoFormat			= videoFormat;
	control.withVanc			= false;
	control.levelConversion		= (useAB ? true : false);
	control.withBurn			= (withBurn ? true : false);
	control.referenceSource		= referenceSource;
	control.bufferSize			= bufferSize;
	control.useDL				= (useDL ? true : false);
	control.insertAncLine		= insertAncLine;
	control.numberOfAnc			= numberOfAnc;
	control.ancBufferSize		= fieldSize;
	control.doNonPCMAudio		= nonPCMAudio ? true : false;
	control.packetsPerLine		= pktsPerLine;

	QAPlayer player (control);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAWindows)
		::signal (SIGBREAK, SignalHandler);
	#elif defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	cout << gDescription << endl;

	//	Initialize the player...
	status = player.Init ();
	if (AJA_SUCCESS (status))
	{
		bool	firstTimeAround	(true);

		//	Run the player...
		player.Run ();

		//	Loop until told to stop...
		while (gGlobalQuit == false)
		{
			//	Poll the player's status...
			AUTOCIRCULATE_STATUS	outputStatus;
			player.GetACStatus (outputStatus);

			if (firstTimeAround)
			{
				cout	<< "  Playout  Playout   Frames" << endl
						<< "   Frames   Buffer  Dropped" << endl;
				firstTimeAround = false;
			}

			cout	<<	setw (9) << outputStatus.acFramesProcessed
					<<	setw (9) << outputStatus.acBufferLevel
					<<  setw (9) << outputStatus.acFramesDropped
					<< "\r" << flush;

			AJATime::Sleep (2000);

		}	//	loop til done

		cout << endl;

		//  Ask the player to stop
		player.Quit ();

	}	//	if player Init succeeded
	else
		cerr << "Player initialization failed with status " << status << endl;

	return 0;
 
}	//	main


static int printVideoFormats (void)
{
	cout << "VideoFormats" << endl;

	for (int ndx (0);  ndx < NTV2_FORMAT_END_HIGH_DEF_FORMATS2;  ndx++)
	{
		switch (ndx)
		{
			case NTV2_FORMAT_525_5994:
			case NTV2_FORMAT_625_5000:
			case NTV2_FORMAT_1080i_5000:
			case NTV2_FORMAT_1080i_5994:
			case NTV2_FORMAT_1080i_6000:
			case NTV2_FORMAT_720p_5000:
			case NTV2_FORMAT_720p_5994:
			case NTV2_FORMAT_720p_6000:
			case NTV2_FORMAT_1080p_2398:
			case NTV2_FORMAT_1080p_2400:
			case NTV2_FORMAT_1080p_2500:
			case NTV2_FORMAT_1080p_2997:
			case NTV2_FORMAT_1080p_3000:
			case NTV2_FORMAT_1080p_5000_A:
			case NTV2_FORMAT_1080p_5994_A:
			case NTV2_FORMAT_1080p_6000_A:
			case NTV2_FORMAT_1080p_2K_2398:
			case NTV2_FORMAT_1080p_2K_2400:
			case NTV2_FORMAT_1080p_2K_2500:
			case NTV2_FORMAT_1080p_2K_2997:
			case NTV2_FORMAT_1080p_2K_3000:
			case NTV2_FORMAT_1080p_2K_4795:
			case NTV2_FORMAT_1080p_2K_4800:
			case NTV2_FORMAT_1080p_2K_5000:
			case NTV2_FORMAT_1080p_2K_5994:
			case NTV2_FORMAT_1080p_2K_6000:
			case NTV2_FORMAT_1080psf_2K_2398:
			case NTV2_FORMAT_1080psf_2K_2400:
			{
				const NTV2VideoFormat	format		(static_cast <NTV2VideoFormat> (ndx));
				const string			formatStr	(::NTV2VideoFormatToString (format));
				if (!formatStr.empty ())
					cout << ndx << ":  " << formatStr << endl;
				break;
			}
			default:
				break;
		}
	}
	return 0;
}


static int printPixelFormats (void)
{
	cout << "PixelFormats" << endl;
	for (int ndx (0);  ndx < NTV2_FBF_NUMFRAMEBUFFERFORMATS;  ndx++)
	{
		const string	pixFormatStr	(::NTV2FrameBufferFormatToString (static_cast <NTV2FrameBufferFormat> (ndx), true));
		if (!pixFormatStr.empty () && ndx != NTV2_FBF_48BIT_RGB)
			cout << ndx << ":  " << pixFormatStr << endl;
	}
	return 0;
}
