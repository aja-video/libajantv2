/**
	@file		ntv2ccplayer/main.cpp
	@brief		Demonstration application that uses AutoCirculate to playout frames to SDI output
				generated in host memory that contain CEA-608 (SD) or CEA-708 (HD) captions.
				The caption text can be self-generated, piped in from standard input, or read from
				any number of text files specified in the command line.
	@copyright	Copyright (C) 2012-2016 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ajastuff/common/options_popt.h"
#include "ajastuff/system/systemtime.h"
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
	if (gChannelNames.empty ())
		for (unsigned enumVal (0);  enumVal < NTV2_CC608_XDS; )
		{
			gChannelNames += ::NTV2Line21ChannelToStr (static_cast <NTV2Line21Channel> (enumVal++));
			if (enumVal < NTV2_CC608_XDS)
				gChannelNames += inDelimiterStr;
			else
				break;
		}
	return gChannelNames;

}	//	GetLine21ChannelNames



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
	AJAStatus		status			(AJA_STATUS_SUCCESS);
	char *			pDeviceSpec		(NULL);				//	Which device?
	char *			pCaptionChannel	(NULL);				//	Caption channel of interest (cc1, cc2 ... text1, text2, ...)
	char *			pMode			(NULL);				//	Kind of 608 display (roll, paint, pop)?
	char *			pVideoFormat	(NULL);				//	Video format (525, 625, etc.)?
	char *			pPixelFormat	(NULL);				//	Pixel format (2vuy, argb, etc.?
	char *			pEndAction		(NULL);				//	End action (quit, repeat, idle)?
	int				noAudio			(0);				//	Disable audio tone?
	int				noTimecode		(0);				//	Disable timecode?
	int				doMultiChannel	(0);				//	Enable multi-format?
	uint32_t		channelNumber	(1);				//	Number of the channel to use
	uint32_t		charsPerMinute	(500);				//	Desired character enqueue rate, in characters per minute
	int				bEmitStats		(0);				//	Emit stats while running?
	int				bBreakNewLines	(0);				//	Newlines break rows instead of treated as whitespace?
	int				bForceVanc		(0);				//	Force use of Vanc?
	int				bSuppressLine21	(0);				//	Suppress line 21 waveform (SD only)?
	StringList		pathList;							//	List of text files (paths) to "play"
	poptContext		optionsContext; 					//	Context for parsing command line arguments
	CCGeneratorConfig	generatorConfig;

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{"board",		'b',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,		0,	"which device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,		0,	"device channel to use",		"1..8"},
		{"format",		'f',	POPT_ARG_STRING,	&pVideoFormat,		0,	"video format to produce",		"'?' or 'list' to list"},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,		0,	"pixel format to use",			"'?' or 'list' to list"},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,			0,	"disable audio tone?",			NULL},
		{"notimecode",	0,		POPT_ARG_NONE,		&noTimecode,		0,	"disable timecode?",			NULL},
		{"multiChannel",'m',	POPT_ARG_NONE,		&doMultiChannel,	0,	"enable multi-chl/fmt?",		NULL},

		{"end",			'e',	POPT_ARG_STRING,	&pEndAction,		0,	"end action",					"exit|loop|idle"},
		{"rate",		'r',	POPT_ARG_INT,		&charsPerMinute,	0,	"caption rate",					"chars/min"},
		{"608mode",		0,		POPT_ARG_STRING,	&pMode,				0,	"608 caption mode",				"roll|roll4|roll3|roll2|paint|pop"},
		{"608chan",		0,		POPT_ARG_STRING,	&pCaptionChannel,	0,	"608 caption channel",			::GetLine21ChannelNames ().c_str ()},
		{"stats",		's',	POPT_ARG_NONE,		&bEmitStats,		0,	"show queue stats?",			NULL},
		{"newline",		0,		POPT_ARG_NONE,		&bBreakNewLines,	0,	"newlines break rows?",			NULL},
		{"vanc",		'v',	POPT_ARG_NONE,		&bForceVanc,		0,	"force use of vanc",			NULL},
		{"noline21",	'n',	POPT_ARG_NONE,		&bSuppressLine21,	0,	"disable line 21 wvfrm?",		NULL},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);

	const char * pStr	(::poptGetArg (optionsContext));
	while (pStr)
	{
		generatorConfig.fFilesToPlay.push_back (string (pStr));
		pStr = ::poptGetArg (optionsContext);
	}	//	for each file path argument
	optionsContext = ::poptFreeContext (optionsContext);

	CCPlayerConfig		playerConfig	(pDeviceSpec ? string (pDeviceSpec) : "0");

	const string		deviceSpec		(pDeviceSpec ? pDeviceSpec : "0");
	const string		videoFormatStr	(pVideoFormat  ?  pVideoFormat  :  "");
	playerConfig.fVideoFormat = NTV2_FORMAT_1080i_5994;
	if (!videoFormatStr.empty ())
	{
		if (CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr, false) != NTV2_FORMAT_UNKNOWN)
			playerConfig.fVideoFormat = CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr, false);
		else if (CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr, true) != NTV2_FORMAT_UNKNOWN)
			playerConfig.fVideoFormat = CNTV2DemoCommon::GetVideoFormatFromString (videoFormatStr, true);
	}
	if (videoFormatStr == "?" || videoFormatStr == "list")
		{cout << CNTV2DemoCommon::GetVideoFormatStrings (BOTH_VIDEO_FORMATS, deviceSpec) << endl;  return 0;}
	else if (!videoFormatStr.empty () && playerConfig.fVideoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings (BOTH_VIDEO_FORMATS, deviceSpec) << endl;
		return 2;
	}
	if (bSuppressLine21 && !NTV2_IS_SD_VIDEO_FORMAT (playerConfig.fVideoFormat))
		cerr	<< "## WARNING:  '--noline21' (-n) option specified with non-SD video format '" << ::NTV2VideoFormatToString (playerConfig.fVideoFormat) << "'" << endl;

	const string				pixelFormatStr	(pPixelFormat  ?  pPixelFormat  :  "");
	playerConfig.fPixelFormat  =  pixelFormatStr.empty ()  ?  NTV2_FBF_10BIT_YCBCR  :  CNTV2DemoCommon::GetPixelFormatFromString (pixelFormatStr);
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty () && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT (playerConfig.fPixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings (PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	if (channelNumber > 0 && channelNumber < 9)
		playerConfig.fOutputChannel = NTV2Channel (channelNumber - 1);
	else
		{cerr << "## ERROR:  Bad channel number '" << channelNumber << "'" << endl;	return 1;}

	const string	sCaptionMode	(CNTV2DemoCommon::ToLower (pMode ? pMode : ""));
	if (sCaptionMode.empty () || sCaptionMode == "roll" || sCaptionMode == "roll4")
		generatorConfig.fCaptionMode = NTV2_CC608_CapModeRollUp4;
	else if (sCaptionMode == "roll3")
		generatorConfig.fCaptionMode = NTV2_CC608_CapModeRollUp3;
	else if (sCaptionMode == "roll2")
		generatorConfig.fCaptionMode = NTV2_CC608_CapModeRollUp2;
	else if (sCaptionMode == "paint")
		generatorConfig.fCaptionMode = NTV2_CC608_CapModePaintOn;
	else if (sCaptionMode == "pop")
		generatorConfig.fCaptionMode = NTV2_CC608_CapModePopOn;
	else
		{cerr << "## ERROR:  Bad '608mode' parameter '" << sCaptionMode << "'" << endl;	return 1;}

	const string	sEndAction		(CNTV2DemoCommon::ToLower (pEndAction ? pEndAction : ""));
	if (sEndAction.empty () || sEndAction == "quit" || sEndAction == "exit" || sEndAction == "terminate" || sEndAction == "end")
		generatorConfig.fEndAction = AtEndAction_Quit;
	else if (sEndAction == "loop" || sEndAction == "repeat")
		generatorConfig.fEndAction = AtEndAction_Repeat;
	else if (sEndAction == "sleep" || sEndAction == "idle" || sEndAction == "rest")
		generatorConfig.fEndAction = AtEndAction_Idle;
	else
		{cerr << "## ERROR:  Bad 'end' parameter '" << sEndAction << "' -- expected 'loop|repeat | rest|sleep|idle | quit|end|exit|terminate'" << endl;	return 1;}

	generatorConfig.fCaptionChannel = (pCaptionChannel ? ::StrToNTV2Line21Channel (string (pCaptionChannel)) : NTV2_CC608_CC1);
	if (!IsValidLine21Channel (generatorConfig.fCaptionChannel) || IsLine21XDSChannel (generatorConfig.fCaptionChannel))
		{cerr << "## ERROR:  Bad '608chan' value '" << pCaptionChannel << "' -- expected '" << ::GetLine21ChannelNames () << "'" << endl;	return 1;}

	if (IsLine21TextChannel (generatorConfig.fCaptionChannel))
	{
		generatorConfig.fCaptionMode = NTV2_CC608_CapModeUnknown;
		if (pMode)
			cerr	<< "## WARNING:  Caption mode '" << sCaptionMode << "' reset to 'unknown' for '"
					<< ::NTV2Line21ChannelToStr (generatorConfig.fCaptionChannel) << "'" << endl;
	}

	generatorConfig.fNewLinesAreNewRows = bBreakNewLines ? true : false;
	if (IsLine21TextChannel (generatorConfig.fCaptionChannel))
		generatorConfig.fNewLinesAreNewRows = true;

	generatorConfig.fCharsPerMinute = static_cast <double> (charsPerMinute);

	playerConfig.fEmitStats = bEmitStats ? true : false;
	playerConfig.fDoMultiFormat = doMultiChannel ? true : false;
	playerConfig.fForceVanc = bForceVanc ? true : false;
	playerConfig.fSuppressLine21 = bSuppressLine21 ? true : false;
	playerConfig.fSuppressAudio = noAudio ? true : false;
	playerConfig.fSuppressTimecode = noTimecode ? true : false;
	playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;

	/*
		//
		//	NOTE:	This is an example of how to inject different captions into separate caption channels.
		//
		playerConfig.fChannelGenerators.clear ();

		generatorConfig.fFilesToPlay.clear ();
		generatorConfig.fFilesToPlay.push_back (string ("CEA608English.txt"));
		generatorConfig.fEndAction			= AtEndAction_Idle;
		generatorConfig.fCaptionMode		= NTV2_CC608_CapModeRollUp2;
		generatorConfig.fCaptionChannel		= NTV2_CC608_CC1;
		generatorConfig.fNewLinesAreNewRows	= true;
		generatorConfig.fCharsPerMinute		= 1000;
		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;

		generatorConfig.fFilesToPlay.clear ();
		generatorConfig.fFilesToPlay.push_back (string ("CEA608Spanish.txt"));
		generatorConfig.fEndAction			= AtEndAction_Repeat;
		generatorConfig.fCaptionMode		= NTV2_CC608_CapModeRollUp3;
		generatorConfig.fCaptionChannel		= NTV2_CC608_CC2;
		generatorConfig.fNewLinesAreNewRows	= true;
		generatorConfig.fCharsPerMinute		= 700;
		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;

		generatorConfig.fFilesToPlay.clear ();
		generatorConfig.fFilesToPlay.push_back (string ("CEA608French.txt"));
		generatorConfig.fEndAction			= AtEndAction_Idle;
		generatorConfig.fCaptionMode		= NTV2_CC608_CapModeRollUp4;
		generatorConfig.fCaptionChannel		= NTV2_CC608_CC3;
		generatorConfig.fNewLinesAreNewRows	= true;
		generatorConfig.fCharsPerMinute		= 500;
		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;

		generatorConfig.fFilesToPlay.clear ();
		generatorConfig.fFilesToPlay.push_back (string ("CEA608German.txt"));
		generatorConfig.fEndAction			= AtEndAction_Repeat;
		generatorConfig.fCaptionMode		= NTV2_CC608_CapModeRollUp4;
		generatorConfig.fCaptionChannel		= NTV2_CC608_CC4;
		generatorConfig.fNewLinesAreNewRows	= true;
		generatorConfig.fCharsPerMinute		= 300;
		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;

		generatorConfig.fFilesToPlay.clear ();
		generatorConfig.fFilesToPlay.push_back (string ("txt1"));
		generatorConfig.fEndAction			= AtEndAction_Idle;
		generatorConfig.fCaptionMode		= NTV2_CC608_CapModeUnknown;
		generatorConfig.fCaptionChannel		= NTV2_CC608_Text1;
		generatorConfig.fNewLinesAreNewRows	= true;
		generatorConfig.fCharsPerMinute		= 200;
		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;

		generatorConfig.fFilesToPlay.clear ();
		generatorConfig.fFilesToPlay.push_back (string ("txt2"));
		generatorConfig.fEndAction			= AtEndAction_Repeat;
		generatorConfig.fCaptionMode		= NTV2_CC608_CapModeUnknown;
		generatorConfig.fCaptionChannel		= NTV2_CC608_Text2;
		generatorConfig.fNewLinesAreNewRows	= true;
		generatorConfig.fCharsPerMinute		= 200;
		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;

		generatorConfig.fFilesToPlay.clear ();
		generatorConfig.fFilesToPlay.push_back (string ("txt3"));
		generatorConfig.fEndAction			= AtEndAction_Idle;
		generatorConfig.fCaptionMode		= NTV2_CC608_CapModeUnknown;
		generatorConfig.fCaptionChannel		= NTV2_CC608_Text3;
		generatorConfig.fNewLinesAreNewRows	= true;
		generatorConfig.fCharsPerMinute		= 200;
		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;

		generatorConfig.fFilesToPlay.clear ();
		generatorConfig.fFilesToPlay.push_back (string ("txt4"));
		generatorConfig.fEndAction			= AtEndAction_Repeat;
		generatorConfig.fCaptionMode		= NTV2_CC608_CapModeUnknown;
		generatorConfig.fCaptionChannel		= NTV2_CC608_Text4;
		generatorConfig.fNewLinesAreNewRows	= true;
		generatorConfig.fCharsPerMinute		= 200;
		playerConfig.fChannelGenerators [generatorConfig.fCaptionChannel] = generatorConfig;
	*/

	//	Instantiate the CC player object...
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
