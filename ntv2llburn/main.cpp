/**
	@file		ntv2llburn/main.cpp
	@brief		Demonstration application that "burns" timecode into frames captured from SDI input,
				and playout those modified frames to SDI output.
	@copyright	Copyright (C) 2012-2018 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ntv2llburn.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
static bool	gGlobalQuit		(false);	///	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
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
														{"hdmi",	NTV2_INPUTSOURCE_HDMI1},
														{"analog",	NTV2_INPUTSOURCE_ANALOG1},
														{"anlg",	NTV2_INPUTSOURCE_ANALOG1},
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
	@brief		Main entry point for 'ntv2llburn' demo application.
	@param[in]	argc	Number of arguments specified on the command line, including the path to the executable.
	@param[in]	argv	Array of arguments.
	@return		Result code, which must be zero if successful, or non-zero for failure.
**/
int main (int argc, const char ** argv)
{
	AJAStatus		status			(AJA_STATUS_SUCCESS);		//	Result status
	char *			pDeviceSpec		(NULL);						//	Which device to use
	char *			pVidSource		(NULL);						//	Video input source string
	char *			pTcSource		(NULL);						//	Time code source string
	NTV2InputSource	vidSource		(NTV2_INPUTSOURCE_SDI1);	//	Video source
	NTV2TCIndex		tcSource		(NTV2_TCINDEX_SDI1);		//	Time code source
	int				noAudio			(0);						//	Disable audio?
	int				useRGB			(0);						//	Use 10-bit RGB instead of 8-bit YCbCr?
	int				doMultiChannel	(0);						//  Set the board up for multi-channel/format
	int				doAnc			(0);						//	Use the Anc Extractor/Inserter
	poptContext		optionsContext;								//	Context for parsing command line arguments
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"which device to use",				"index#, serial#, or model"	},
		{"input",		'i',	POPT_ARG_STRING,	&pVidSource,	0,	"video input",						"{'?' to list}"		},
		{"tcsource",	't',	POPT_ARG_STRING,	&pTcSource,		0,	"time code source",					"{'?' to list}"		},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",					NULL},
		{"rgb",			0,		POPT_ARG_NONE,		&useRGB,		0,	"use RGB10 frame buffer?",			NULL},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,0,	"use multichannel/multiformat?",	NULL},
		{"anc",			'a',	POPT_ARG_NONE,		&doAnc,			0,	"use Anc data extractor/inserter",	NULL},
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
	}

	//	Instantiate the NTV2Burn object...
	NTV2LLBurn	burner (pDeviceSpec ? pDeviceSpec : "0",					//	Which device?
						(noAudio ? false : true),							//	Include audio?
						useRGB ? NTV2_FBF_10BIT_RGB : NTV2_FBF_8BIT_YCBCR,	//	Use RGB frame buffer format?
						vidSource,											//	Which video input source?
						tcSource,											//	Which time code source?
						doMultiChannel ? true : false,						//  Set the board up for multi-channel/format
						doAnc ? true : false);								//	Use the Anc Extractor/Inserter

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

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

	return AJA_SUCCESS (status) ? 0 : 2;	//	Return zero upon success -- otherwise 2

}	//	main
