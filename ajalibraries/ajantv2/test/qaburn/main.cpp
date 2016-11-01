/**
	@file		main.cpp
	@brief		Demonstration application that "burns" timecode into frames captured from SDI input,
				and playout those modified frames to SDI output.
	@copyright	Copyright (C) 2012-2014 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajastuff/common/options_popt.h"
#include "qaburn.h"
#include <signal.h>
#include <iostream>
#include <iomanip>


//	Globals
static bool	gGlobalQuit		(false);	///	Set this "true" to exit gracefully

static int printPixelFormats (void);


/**
	@brief	Returns the given string after converting it to lower case.
	@param[in]	inStr	Specifies the string to be converted to lower case.
	@return		The given string converted to lower-case.
	@note		This function won't work if the input string contains non-ASCII characters!
**/
static string ToLower (const string & inStr)
{
	string	result (inStr);
	std::transform (result.begin (), result.end (), result.begin (), ::tolower);
	return result;
}


void SignalHandler (int signal)
{
	gGlobalQuit = true;
	AJATime::Sleep(2000);																//	...and give the device a dozen frames or so to lock to the input signal
}

typedef struct
{
	const string		fName;
	const NTV2TCIndex	fEnum;
}	TCSourceMapping;

typedef struct
{
	const string			fName;
	const NTV2InputSource	fEnum;
}	SourceMapping;

static const TCSourceMapping	gTCSourceMappings [] =	{{"sdi1",	NTV2_TCINDEX_SDI1},
														{"sdi2",	NTV2_TCINDEX_SDI2},
														{"sdi3",	NTV2_TCINDEX_SDI3},
														{"sdi4",	NTV2_TCINDEX_SDI4},
														{"sltc1",	NTV2_TCINDEX_SDI1_LTC},
														{"sltc2",	NTV2_TCINDEX_SDI2_LTC},
														{"ltc1",	NTV2_TCINDEX_LTC1},
														{"ltc2",	NTV2_TCINDEX_LTC2},
														{"",		NTV2_TCINDEX_SDI1}};

static const SourceMapping		gSourceMappings []	=	{{"sdi1",	NTV2_INPUTSOURCE_SDI1},
														{"sdi2",	NTV2_INPUTSOURCE_SDI2},
														{"sdi3",	NTV2_INPUTSOURCE_SDI3},
														{"sdi4",	NTV2_INPUTSOURCE_SDI4},
														{"sdi5",	NTV2_INPUTSOURCE_SDI5},
														{"sdi6",	NTV2_INPUTSOURCE_SDI6},
														{"sdi7",	NTV2_INPUTSOURCE_SDI7},
														{"sdi8",	NTV2_INPUTSOURCE_SDI8},
														{"hdmi",	NTV2_INPUTSOURCE_HDMI},
														{"analog",	NTV2_INPUTSOURCE_ANALOG},
														{"anlg",	NTV2_INPUTSOURCE_ANALOG},
														{"",		NTV2_NUM_INPUTSOURCES}};


typedef map <string, NTV2TCIndex>		TCSourceMap;
typedef TCSourceMap::const_iterator		TCSourceMapConstIter;
static TCSourceMap						gTCSourceMap;

typedef map <string, NTV2InputSource>	SourceMap;
typedef SourceMap::const_iterator		SourceMapConstIter;
static SourceMap						gSourceMap;

class MapMaker
{
	public:
		MapMaker ()
		{
			for (size_t ndx (0);  ndx < sizeof (gTCSourceMappings) / sizeof (TCSourceMapping);  ndx++)
				if (!gTCSourceMappings [ndx].fName.empty ())
					gTCSourceMap [gTCSourceMappings [ndx].fName] = gTCSourceMappings [ndx].fEnum;
			for (size_t ndx (0);  ndx < sizeof (gSourceMappings) / sizeof (SourceMapping);  ndx++)
				if (!gSourceMappings [ndx].fName.empty ())
					gSourceMap [gSourceMappings [ndx].fName] = gSourceMappings [ndx].fEnum;
		}
};	//	MapMaker

static ostream & operator << (ostream & inOutStream, const TCSourceMap & inObj)
{
	for (TCSourceMapConstIter iter (inObj.begin ());  ;  )
	{
		inOutStream << iter->first;
		if (++iter != inObj.end ())
			inOutStream << "|";
		else
			return inOutStream;
	}
}

static ostream & operator << (ostream & inOutStream, const SourceMap & inObj)
{
	for (SourceMapConstIter iter (inObj.begin ());  ;  )
	{
		inOutStream << iter->first;
		if (++iter != inObj.end ())
			inOutStream << "|";
		else
			return inOutStream;
	}
}

static MapMaker	gMapMakerSingleton;


/**
	@brief		Main entry point for 'ntv2burn' demo application.
	@param[in]	argc	Number arguments specified on the command line, including the path to the executable.
	@param[in]	argv	Array of 'const char' pointers, one for each argument.
	@return		Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	AJAStatus		status			(AJA_STATUS_SUCCESS);	//	Result status
	char *			pDeviceSpec		(NULL);					//	Which device to use
	char *			pAudioSystem	(NULL);					//	Which audio system to use
	char *			pVideoSource	(NULL);					//	Which video input to use
	char *			pPixelFormat	(NULL);					//	Which pixel format to use
	int				burnTimecode	(0);					//	With burn?
	char *			pWhatToList		(NULL);					//	Value of --list argument
	poptContext		optionsContext;							//	Context for parsing command line arguments
	uint32_t		useDL			(0);					//	Use DL vs CSC
	uint32_t		useAB			(0);					//	Use DL vs CSC


	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"device",		0,		POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"input",		'i',	POPT_ARG_STRING,	&pVideoSource,	0,	"video input to use",		"{'?' or 'list'} to list"	},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"{'?' or 'list'} to list"	},
		{"audio",		'a',	POPT_ARG_STRING,	&pAudioSystem,	0,	"audio system to use",		"1-8 or 'none'"				},
		{"burn",	    't',	POPT_ARG_NONE,		&burnTimecode,	0,	"burn-in timecode?",		NULL	},
		{"useDL",		'd',	POPT_ARG_NONE,		&useDL,			0,	"use DL vs CSC for RGB",	NULL	},
		{"useAB",		'g',	POPT_ARG_NONE,		&useAB,			0,	"use 3Ga->3Gb out conv",	NULL	},
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

	const string				deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	const string				videoSourceStr	(pVideoSource ? CNTV2DemoCommon::ToLower (pVideoSource) : "");
	const string				pixelFormatStr	(pPixelFormat  ?  pPixelFormat  :  "");
	const NTV2FrameBufferFormat	pixelFormat		(pixelFormatStr.empty () ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString (pixelFormatStr));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty () && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT (pixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	Select video source...
	const NTV2InputSource	inputSource		(videoSourceStr.empty () ? NTV2_INPUTSOURCE_SDI1 : CNTV2DemoCommon::GetInputSourceFromString (videoSourceStr));
	if (videoSourceStr == "?" || videoSourceStr == "list")
		{cout << CNTV2DemoCommon::GetInputSourceStrings (INPUT_SOURCES_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty () && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT (pixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	const string		audioSystemStr	(pAudioSystem  ?  pAudioSystem  :  "1");
	NTV2AudioSystem		audioSystem		(audioSystemStr.empty () ? NTV2_AUDIOSYSTEM_1 : CNTV2DemoCommon::GetAudioSystemFromString (audioSystemStr));
	if (audioSystemStr == "?" || audioSystemStr == "list")
		{cout << CNTV2DemoCommon::GetAudioSystemStrings (deviceSpec) << endl;  return 0;}
	else if (audioSystemStr == "0" || audioSystemStr == "none")
		audioSystem = NTV2_AUDIOSYSTEM_INVALID;	//	No audio
	else if (!audioSystemStr.empty () && !NTV2_IS_VALID_AUDIO_SYSTEM (audioSystem))
	{
		cerr	<< "## ERROR:  Invalid '--audio' value '" << audioSystemStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetAudioSystemStrings (deviceSpec) << endl;
		return 2;
	}

	//	Instantiate the QABurn object...
	QABurn	burner	(	pDeviceSpec ? string (pDeviceSpec) : "0",		//	Which device?
						inputSource,									//	Which input source?
						audioSystem,									//	Which audio system?
						pixelFormat,									//	Which pixel format?
						burnTimecode ? true : false,					//	Burn timcode in frame
						useAB ? true : false,							//	Use 3Ga->3Gb converters?
						useDL ? true : false);							//	Use dual-link RGB?

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Initialize the QABurn instance...
	status = burner.Init ();
	if (!AJA_SUCCESS (status))
		{cout << "## ERROR:  Initialization failed, status=" << status << endl;		return 1;}

	//	Start the burner's capture and playout threads...
	burner.Run ();

	//	Loop until told to stop...
	cout	<< "           Capture  Playout  Capture  Playout" << endl
			<< "   Frames   Frames   Frames   Buffer   Buffer" << endl
			<< "Processed  Dropped  Dropped    Level    Level" << endl;
	do
	{
		ULWord	inputFramesProcessed(0), inputFramesDropped(0), inputBufferLevel(0), outputFramesDropped(0), outputBufferLevel(0);
		burner.GetACStatus (inputFramesProcessed, inputFramesDropped, inputBufferLevel, outputFramesDropped, outputBufferLevel);

		cout	<< setw (9) << inputFramesProcessed
				<< setw (9) << inputFramesDropped
				<< setw (9) << outputFramesDropped
				<< setw (9) << inputBufferLevel
				<< setw (9) << outputBufferLevel
				<< "\r" << flush;
		AJATime::Sleep (2000);
	} while (!gGlobalQuit);		//	loop til signaled

	cout << endl;

	return AJA_SUCCESS (status) ? 0 : 2;	//	Return zero upon success -- otherwise 2

}	//	main


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
