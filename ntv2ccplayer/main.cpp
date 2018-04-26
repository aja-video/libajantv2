/**
	@file		ntv2ccplayer/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout frames to SDI output
				generated in host memory that contain CEA-608 (SD) or CEA-708 (HD) captions.
				The caption text can be self-generated, piped in from standard input, or read from
				any number of text files specified in the command line.
	@copyright	Copyright (C) 2012-2018 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/system/systemtime.h"
#include "ntv2ccplayer.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#if defined (AJAMac) || defined (AJALinux)
	#include <unistd.h>
	#include <termios.h>
#elif defined (MSWindows) || defined (AJAWindows)
	#define	SIGQUIT	SIGBREAK
#endif


using namespace std;


//	Globals
static bool		gQuitFlag			(false);		//	Set this "true" to exit gracefully
static bool		gQuitImmediately	(false);		//	Set this "true" to exit ungracefully


/**
	@brief	This function implements the "power off" switch for the program.
			It's called in one of two ways:
				a)	ctrl-C or ctrl-break interrupt;
				b)	by the NTV2CCPlayer object's caption generator thread (if "end action" is "quit").
	@param[in]	inSignal	Specifies the signal. If it's SIG_AJA_STOP, quit gracefully.
**/
void SignalHandler (int inSignal)
{
	gQuitImmediately = (inSignal != SIG_AJA_STOP);
	gQuitFlag = true;
}


static string GetLine21ChannelNames (string inDelimiterStr = "|")
{
	static string	gChannelNames;
	if (gChannelNames.empty())
		for (unsigned enumVal(0);  enumVal < NTV2_CC608_XDS; )
		{
			gChannelNames += ::NTV2Line21ChannelToStr(static_cast <NTV2Line21Channel> (enumVal++));
			if (enumVal < NTV2_CC608_XDS)
				gChannelNames += inDelimiterStr;
			else
				break;
		}
//cerr << endl << "gChannelNames='" << gChannelNames << "'" << endl;
	return gChannelNames;

}	//	GetLine21ChannelNames


static StringList Split (string inStr, const string inDelimiter = string(","))
{
	StringList	result;
	size_t	pos	(inStr.find (inDelimiter));
	while (pos != string::npos)
	{
		const string	piece	(inStr.substr (0, pos));
		result.push_back (piece);
		inStr.erase (0, pos+1);
		pos = inStr.find (inDelimiter);
	}
	result.push_back (inStr);
	return result;
}

static void PrintStringList (const StringList & inStrList)
{
	for (StringListConstIter it (inStrList.begin());  it != inStrList.end();  )
	{
		cerr << *it;
		if (++it != inStrList.end ())
			cerr << ", ";
	}
}


/**
	@brief	Plays out video containing closed-captions generated internally (no file arguments) or from
			one or more text files (or from standard input).
	@param[in]	argc	The number of arguments on the command line, including the executable itself.
	@param[in]	argv	An array of const pointers to character strings, one for each argument.
	@return		Zero if successful.
	@note	Specify the hyphen character ('-') as a file argument to read caption data from standard input.
**/
int main (int argc, const char ** argv)
{
	AJAStatus			status			(AJA_STATUS_SUCCESS);
	char *				pDeviceSpec		(NULL);				//	Which device?
	char *				pCaptionChannel	(NULL);				//	Caption channel of interest (cc1, cc2 ... text1, text2, ...)
	char *				pMode			(NULL);				//	Kind of 608 display (roll, paint, pop)?
	char *				pVideoFormat	(NULL);				//	Video format (525, 625, etc.)?
	char *				pPixelFormat	(NULL);				//	Pixel format (2vuy, argb, etc.?
	char *				pEndAction		(NULL);				//	End action (quit, repeat, idle)?
	char *				pCaptionRate	(NULL);				//	Caption rate (chars/min)
	int					noAudio			(0);				//	Disable audio tone?
	int					noTimecode		(0);				//	Disable timecode?
	int					doMultiChannel	(0);				//	Enable multi-format?
	uint32_t			channelNumber	(1);				//	Number of the channel to use
	int					bEmitStats		(0);				//	Emit stats while running?
	int					bBreakNewLines	(0);				//	Newlines break rows instead of treated as whitespace?
	int					bForceVanc		(0);				//	Force use of Vanc?
	int					bSuppressLine21	(0);				//	Suppress line 21 waveform (SD only)?
	StringList			pathList;							//	List of text files (paths) to "play"
	poptContext			optionsContext; 					//	Context for parsing command line arguments

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		//	Device config options...
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,		0,	"device channel to use",		"1..8"},
		{"format",		'f',	POPT_ARG_STRING,	&pVideoFormat,		0,	"video format to produce",		"'?' or 'list' to list"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,		0,	"pixel format to use",			"'?' or 'list' to list"},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,			0,	"disable audio tone?",			NULL},
		{"notimecode",	0,		POPT_ARG_NONE,		&noTimecode,		0,	"disable timecode?",			NULL},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,	0,	"enable multi-chl/fmt?",		NULL},

		//	CCPlayer global config options...
		{"stats",		's',	POPT_ARG_NONE,		&bEmitStats,		0,	"show queue stats?",			NULL},
		{"vanc",		'v',	POPT_ARG_NONE,		&bForceVanc,		0,	"force use of vanc",			NULL},
		{"noline21",	'n',	POPT_ARG_NONE,		&bSuppressLine21,	0,	"disable line 21 wvfrm?",		NULL},

		//	Per-caption-channel options -- specify more than one by separating with comma (e.g., --end loop,idle,idle,loop  --608chan cc1,cc2,tx3,tx4)
		{"end",			'e',	POPT_ARG_STRING,	&pEndAction,		0,	"end action",					"exit|loop|idle,..."},
		{"rate",		'r',	POPT_ARG_STRING,	&pCaptionRate,		0,	"caption rate",					"chars/min,..."},
		{"608mode",		0,		POPT_ARG_STRING,	&pMode,				0,	"608 caption mode",				"roll|roll4|roll3|roll2|paint|pop,..."},
		{"608chan",		0,		POPT_ARG_STRING,	&pCaptionChannel,	0,	"608 caption channel",			::GetLine21ChannelNames().c_str ()},
		{"newline",		0,		POPT_ARG_NONE,		&bBreakNewLines,	0,	"newlines break rows?",			NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);

	const char * pStr	(::poptGetArg (optionsContext));
	while (pStr)
	{
		pathList.push_back (string (pStr));
		pStr = ::poptGetArg (optionsContext);
	}	//	for each file path argument
	optionsContext = ::poptFreeContext (optionsContext);

	//	Board/Device
	const string			deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");

	//	Channel
	NTV2Channel				channel			(NTV2_CHANNEL1);			//	Default to channel 1
	if (channelNumber > 0 && channelNumber < 9)
		channel = NTV2Channel (channelNumber - 1);
	else
		{cerr << "## ERROR:  Bad channel number '" << channelNumber << "'" << endl;	return 1;}

	//	Video Format
	NTV2VideoFormat			videoFormat		(NTV2_FORMAT_1080i_5994);	//	Default to 1080i5994
	NTV2FrameBufferFormat	pixelFormat		(NTV2_FBF_10BIT_YCBCR);		//	Default to 10bitYUV
	const string			videoFormatStr	(pVideoFormat  ?  pVideoFormat  :  "");
	if (videoFormatStr == "?" || videoFormatStr == "list")
		{cout << CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	if (!videoFormatStr.empty ())
		videoFormat = CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr, VIDEO_FORMATS_ALL);
	else if (!videoFormatStr.empty ()  &&  !NTV2_IS_VALID_VIDEO_FORMAT (videoFormat))
	{
		cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings (VIDEO_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}
	if (bSuppressLine21 && !NTV2_IS_SD_VIDEO_FORMAT (videoFormat))
		cerr	<< "## WARNING:  '--noline21' (-n) option specified with non-SD video format '" << ::NTV2VideoFormatToString (videoFormat) << "'" << endl;

	//	Pixel Format
	const string	pixelFormatStr	(pPixelFormat  ?  pPixelFormat  :  "");
	pixelFormat  =  pixelFormatStr.empty ()  ?  NTV2_FBF_10BIT_YCBCR  :  CNTV2DemoCommon::GetPixelFormatFromString (pixelFormatStr);
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty () && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT (pixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	Configure the player...
	CCPlayerConfig		playerConfig	(pDeviceSpec ? string (pDeviceSpec) : "0");
	playerConfig.fVideoFormat		= videoFormat;
	playerConfig.fPixelFormat		= pixelFormat;
	playerConfig.fOutputChannel		= channel;
	playerConfig.fEmitStats			= bEmitStats		? true : false;
	playerConfig.fDoMultiFormat		= doMultiChannel	? true : false;
	playerConfig.fForceVanc			= bForceVanc		? true : false;
	playerConfig.fSuppressLine21	= bSuppressLine21	? true : false;
	playerConfig.fSuppressAudio		= noAudio			? true : false;
	playerConfig.fSuppressTimecode	= noTimecode		? true : false;

	cerr	<< "CCPlayer config:  '" << ::NTV2VideoFormatToString (videoFormat) << "', " << ::NTV2FrameBufferFormatToString (pixelFormat)
			<< ", NTV2_CHANNEL" << (channel+1) << ", stats=" << (bEmitStats?"Y":"N") << ", multiChan=" << (doMultiChannel?"Y":"N")
			<< ", forceVANC=" << (bForceVanc?"Y":"N") << ", noLine21=" << (bSuppressLine21?"Y":"N") << ", noAudio=" << (noAudio?"Y":"N")
			<< ", noTC=" << (noTimecode?"Y":"N") << endl;

	//	NOTE:	From one command line invocation, you can inject different captions into separate caption channels:
	//	./bin/ntv2ccplayer	--device kona4  --channel 3  --stats  --608chan cc1,cc2,cc3,cc4,tx1,tx2,tx3,tx4
	//						--608mode pop,paint,roll3,roll4  --rate 1000,700,500,300,200
	//						--end idle,loop,idle,loop,idle,loop,idle,loop
	//						English.txt  Spanish.txt  French.txt  German.txt  txt1  txt2  txt3  txt4

	//	Users can play one or more caption channels by specifying more than one, separating each with a comma:		--608chan cc2,cc4,tx1,tx2
	//	You can vary the mode, end-action and rate for each caption channel in the same way:						--608mode pop,paint,roll  --rate 1500,800,300,600
	const StringList	sCaptionChannels	(::Split (CNTV2DemoCommon::ToLower (pCaptionChannel	? pCaptionChannel	: "CC1")));
	const StringList	sCaptionModes		(::Split (CNTV2DemoCommon::ToLower (pMode			? pMode				: "roll4")));
	const StringList	sEndActions			(::Split (CNTV2DemoCommon::ToLower (pEndAction		? pEndAction		: "exit")));
	const StringList	sCaptionRates		(::Split (CNTV2DemoCommon::ToLower (pCaptionRate	? pCaptionRate		: "500")));
	size_t				ndx					(0);
	size_t				fileNdx				(0);

	for (StringListConstIter iter (sCaptionChannels.begin());  iter != sCaptionChannels.end();  ++iter, ++ndx)
	{
		CCGeneratorConfig	generatorConfig;

		generatorConfig.fCaptionChannel = ::StrToNTV2Line21Channel (*iter);
		if (!IsValidLine21Channel (generatorConfig.fCaptionChannel) || IsLine21XDSChannel (generatorConfig.fCaptionChannel))
			{cerr << "## ERROR:  Bad '608chan' value '" << pCaptionChannel << "' -- expected '" << ::GetLine21ChannelNames () << "'" << endl;	return 1;}

		const string	sCaptionMode	(ndx < sCaptionModes.size() ? sCaptionModes.at(ndx) : sCaptionModes.back());
		if (sCaptionMode == "")				generatorConfig.fCaptionMode = NTV2_CC608_CapModeUnknown;
		else if (sCaptionMode == "pop")		generatorConfig.fCaptionMode = NTV2_CC608_CapModePopOn;
		else if (sCaptionMode == "paint")	generatorConfig.fCaptionMode = NTV2_CC608_CapModePaintOn;
		else if (sCaptionMode == "roll2")	generatorConfig.fCaptionMode = NTV2_CC608_CapModeRollUp2;
		else if (sCaptionMode == "roll3")	generatorConfig.fCaptionMode = NTV2_CC608_CapModeRollUp3;
		else if (sCaptionMode == "roll" || sCaptionMode == "roll4")	generatorConfig.fCaptionMode = NTV2_CC608_CapModeRollUp4;
		else	{cerr << "## ERROR:  Bad '608mode' parameter '" << sCaptionMode << "'" << endl;	return 1;}
	
		if (IsLine21TextChannel (generatorConfig.fCaptionChannel))
			generatorConfig.fCaptionMode = NTV2_CC608_CapModeUnknown;	//	Must use unknown caption mode for TX caption channels
	
		generatorConfig.fNewLinesAreNewRows = bBreakNewLines ? true : false;
		if (IsLine21TextChannel (generatorConfig.fCaptionChannel))
			generatorConfig.fNewLinesAreNewRows = true;	//	Tx1/Tx2/Tx3/Tx4 always break rows on newLine chars

		const string	sEndAction	(ndx < sEndActions.size() ? sEndActions.at(ndx) : sEndActions.back());
		if (sEndAction.empty () || sEndAction == "quit" || sEndAction == "exit" || sEndAction == "terminate" || sEndAction == "end")
			generatorConfig.fEndAction = AtEndAction_Quit;
		else if (sEndAction == "loop" || sEndAction == "repeat")
			generatorConfig.fEndAction = AtEndAction_Repeat;
		else if (sEndAction == "sleep" || sEndAction == "idle" || sEndAction == "rest")
			generatorConfig.fEndAction = AtEndAction_Idle;
		else
			{cerr << "## ERROR:  Bad 'end' parameter '" << sEndAction << "' -- expected 'loop|repeat | rest|sleep|idle | quit|end|exit|terminate'" << endl;	return 1;}

		const string	sCaptionRate	(ndx < sCaptionRates.size() ? sCaptionRates.at(ndx) : sCaptionRates.back());
		istringstream iss(sCaptionRate);
		uint32_t	charsPerMin;
		iss >> charsPerMin;
		generatorConfig.fCharsPerMinute = static_cast <double> (charsPerMin);

		if (!pathList.empty())
		{
			generatorConfig.fFilesToPlay.push_back (fileNdx < pathList.size() ? pathList.at(fileNdx) : pathList.back());
			++fileNdx;
			if ((ndx+1) == sCaptionChannels.size())
				while (fileNdx < pathList.size())
					generatorConfig.fFilesToPlay.push_back (pathList.at (fileNdx++));
		}
		cerr	<< *iter << ":  mode=" << sCaptionMode << ", newLinesAreNewRows=" << (generatorConfig.fNewLinesAreNewRows?"Y":"N")
				<< ", endAction=" << sEndAction << ", rate=" << charsPerMin << ", " << generatorConfig.fFilesToPlay.size() << " file(s):  ";
		PrintStringList (generatorConfig.fFilesToPlay);
		cerr	<< endl;

		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;	//	Add this generator to the player configuration
	}	//	for each specified caption channel


	//	Instantiate the CC player object, and pass it the config object...
	NTV2CCPlayer	player (playerConfig);

	::signal (SIGINT, ::SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, ::SignalHandler);
		::signal (SIGQUIT, ::SignalHandler);
	#endif

	//	Initialize the player...
	status = player.Init ();
	if (AJA_FAILURE (status))
		{cerr << "## ERROR:  Player initialization failed, status=" << status << endl;	return 2;}

	if (bEmitStats)
		cout	<< "  Current  Current    Total    Total    Total    Total      Max         " << endl
				<< " Messages    Bytes Messages    Bytes Messages    Bytes    Queue  Dropped" << endl
				<< "   Queued   Queued Enqueued Enqueued Dequeued Dequeued    Depth   Frames" << endl;

	//	Run the player...
	player.Run ();

	size_t	msgsQued (0), bytesQued (0), totMsgsEnq (0), totBytesEnq (0), totMsgsDeq (0), totBytesDeq (0), maxQueDepth (0), droppedFrames (0);

	//	Loop until we're told to stop...
	while (!gQuitFlag)
	{
		//	Poll the player's encoder's status...
		player.GetStatus (msgsQued, bytesQued, totMsgsEnq, totBytesEnq, totMsgsDeq, totBytesDeq, maxQueDepth, droppedFrames);

		if (bEmitStats)
			cout	<<	setw (9) << msgsQued
					<<	setw (9) << bytesQued
					<<	setw (9) << totMsgsEnq
					<<	setw (9) << totBytesEnq
					<<	setw (9) << totMsgsDeq
					<<	setw (9) << totBytesDeq
					<<	setw (9) << maxQueDepth
					<<	setw (9) << droppedFrames
					<< "\r" << flush;

		AJATime::Sleep (2000);

	}	//	loop til gQuitFlag

	//  Ask the player to stop...
	player.Quit (gQuitImmediately);

	player.GetStatus (msgsQued, bytesQued, totMsgsEnq, totBytesEnq, totMsgsDeq, totBytesDeq, maxQueDepth, droppedFrames);
	cout	<< endl
			<<	msgsQued	<< " message(s) left in queue"	<< endl
			<<	bytesQued	<< " byte(s) left in queue"		<< endl
			<<	totMsgsEnq	<< " total message(s) enqueued"	<< endl
			<<	totBytesEnq	<< " total byte(s) enqueued"	<< endl
			<<	totMsgsDeq	<< " total message(s) dequeued"	<< endl
			<<	totBytesDeq	<< " total byte(s) dequeued"	<< endl
			<<	maxQueDepth	<< " maximum message(s) enqueued" << endl;

	return 0;

}	//	main
