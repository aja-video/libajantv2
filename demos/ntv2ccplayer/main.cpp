/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2ccplayer/main.cpp
	@brief		Complex demonstration application that plays synthesized video to SDI output
				containing NTSC captions derived from standard input or text files.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2ccplayer.h"
#include <signal.h>
#if defined(MSWindows) || defined(AJAWindows)
	#define	SIGQUIT	SIGBREAK
#endif


using namespace std;


//	Globals
static bool	gGlobalQuit		(false);	//	Set this "true" to exit gracefully
static bool	gQuitImmediately(false);	//	Set this "true" to exit ungracefully


void SignalHandler (int inSignal)
{
	gQuitImmediately = (inSignal != SIG_AJA_STOP);
	gGlobalQuit = true;
}


static string GetLine21ChannelNames (const string inDelimiterStr = "|")
{
	static NTV2StringList gChannelNames;
	if (gChannelNames.empty())
		for (unsigned enumVal(0);  enumVal < NTV2_CC608_XDS; )
			gChannelNames.push_back(::NTV2Line21ChannelToStr(NTV2Line21Channel(enumVal++)));
	return aja::join(gChannelNames, inDelimiterStr);
}


int main (int argc, const char ** argv)
{
	char *			pDeviceSpec		(AJA_NULL);		//	Device specifier string, if any
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format argument
	char *			pFramesSpec		(AJA_NULL);		//	AutoCirculate frames spec
	char *			pVideoFormat	(AJA_NULL);		//	Video format to use
	char *			pCaptionChan	(AJA_NULL);		//	Caption channel of interest (cc1, cc2 ... text1, text2, ...)
	char *			pOutputDest		(AJA_NULL);		//	Output connector of interest (sdi1 ... sdi8)
	char *			pMode			(AJA_NULL);		//	Kind of 608 display (roll, paint, pop)?
	char *			pTestPattern	(AJA_NULL);		//	Test pattern to use
	char *			pEndAction		(AJA_NULL);		//	End action (quit, repeat, idle)?
	char *			pCaptionRate	(AJA_NULL);		//	Caption rate (chars/min)
	int				channelNumber	(1);			//	Channel/FrameStore to use
	int				doMultiFormat	(0);			//	MultiFormat mode?
	int				showVersion		(0);			//	Show version?
	int				noAudio			(0);			//	Disable audio tone?
	int				noTimecode		(0);			//	Disable timecode?
	int				forceRTP		(0);			//	Force RTP?
	int				bEmitStats		(0);			//	Emit stats while running?
	int				breakNewLines	(0);			//	Newlines break rows instead of treated as whitespace?
	int				forceVanc		(0);			//	Force use of Vanc?
	int				noLine21		(0);			//	Suppress line 21 waveform (SD only)?
	int				no608			(0);			//	Don't transmit CEA608 packets?
	int				no708			(0);			//	Don't transmit CEA708 packets (HD only)?
	int				doRGBOnWire		(0);			//	Dual-link RGB output? (YUV is default)
	int				doSquares		(0);			//	For UHD/4K, do squares? Otherwise, default to TSI.
	const string	L21ChanNames	(::GetLine21ChannelNames());
	AJADebug::Open();

	//	Command line option descriptions:
	const CNTV2DemoCommon::PoptOpts optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"channel",		'c',	POPT_ARG_INT,		&channelNumber,	0,	"channel to use",			"1-8"						},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",	AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"		},
		{"frames",		  0,	POPT_ARG_STRING,	&pFramesSpec,	0,	"frames to AutoCirculate",	"num[@min] or min-max"		},
		{"videoFormat",	'v',	POPT_ARG_STRING,	&pVideoFormat,	0,	"video format to produce",	"'?' or 'list' to list"		},
		{"noaudio",		  0,	POPT_ARG_NONE,		&noAudio,		0,	"disable audio tone",		AJA_NULL					},

		{"output",		'o',	POPT_ARG_STRING,	&pOutputDest,	0,	"output connector to use",	"'?' or 'list' to list"		},
		{"pattern",		  0,	POPT_ARG_STRING,	&pTestPattern,	0,	"test pattern to use",		"'?' or 'list' to list"		},
		{"notimecode",	  0,	POPT_ARG_NONE,		&noTimecode,	0,	"disable timecode?",		AJA_NULL					},
		{"rgb",			  0,	POPT_ARG_NONE,		&doRGBOnWire,	0,	"RGB on SDI?",				AJA_NULL					},
		{"squares",		  0,	POPT_ARG_NONE,		&doSquares,		0,	"use square routing?",		AJA_NULL					},

		{"stats",		's',	POPT_ARG_NONE,		&bEmitStats,	0,	"show queue stats?",		AJA_NULL					},
		{"vanc",		  0,	POPT_ARG_NONE,		&forceVanc,		0,	"force use of vanc",		AJA_NULL					},
		{"noline21",	'n',	POPT_ARG_NONE,		&noLine21,		0,	"disable line 21 wvfrm?",	AJA_NULL					},
		{"no608",		  0,	POPT_ARG_NONE,		&no608,			0,	"don't xmit 608 packets?",	AJA_NULL					},
		{"no708",		  0,	POPT_ARG_NONE,		&no708,			0,	"don't xmit 708 packets?",	AJA_NULL					},

		//	Per-caption-channel options -- specify more than one by separating with comma (e.g., --end loop,idle,idle,loop  --608chan cc1,cc2,tx3,tx4)
		{"end",			'e',	POPT_ARG_STRING,	&pEndAction,	0,	"end action",				"exit|loop|idle,..."		},
		{"rate",		'r',	POPT_ARG_STRING,	&pCaptionRate,	0,	"caption rate",				"chars/min,..."				},
		{"608mode",		  0,	POPT_ARG_STRING,	&pMode,			0,	"608 caption mode",			"roll[N]|paint|pop,..."		},
		{"608chan",		  0,	POPT_ARG_STRING,	&pCaptionChan,	0,	"608 caption channel",		L21ChanNames.c_str()		},
		{"newline",		  0,	POPT_ARG_NONE,		&breakNewLines,	0,	"newlines break rows?",		AJA_NULL					},
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	NTV2StringList pathList (popt.otherArgs());	//	List of text files (paths) to "play"
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	//	Device
	const string deviceSpec (pDeviceSpec ? pDeviceSpec : "0");
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		return 1;

	CCPlayerConfig config(deviceSpec);

	//	Channel
	if ((channelNumber < 1)  ||  (channelNumber > 8))
		{cerr << "## ERROR:  Invalid channel number " << channelNumber << " -- expected 1 thru 8" << endl;  return 1;}
	config.fOutputChannel = NTV2Channel(channelNumber - 1);

	//	VideoFormat
	const string videoFormatStr (pVideoFormat  ?  pVideoFormat  :  "");
	config.fVideoFormat = videoFormatStr.empty()	?	NTV2_FORMAT_1080i_5994
													:	CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr, VIDEO_FORMATS_ALL);
	if (videoFormatStr == "?"  ||  videoFormatStr == "list")
		{cout	<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!videoFormatStr.empty()  &&  !NTV2_IS_VALID_VIDEO_FORMAT(config.fVideoFormat))
	{	cerr	<< "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetVideoFormatStrings(VIDEO_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}
	if (noLine21 && !NTV2_IS_SD_VIDEO_FORMAT(config.fVideoFormat))
		cerr	<< "## WARNING:  '--noline21' (-n) option specified with non-SD video format '" << ::NTV2VideoFormatToString(config.fVideoFormat) << "'" << endl;
	if (no708 && NTV2_IS_SD_VIDEO_FORMAT(config.fVideoFormat))
		cerr	<< "## WARNING:  '--no708' (-n) option specified with SD video format '" << ::NTV2VideoFormatToString(config.fVideoFormat) << "'" << endl;
	if (noLine21 && no608 && NTV2_IS_SD_VIDEO_FORMAT(config.fVideoFormat))
		cerr	<< "## WARNING:  '--noline21' and '--no608' options will result in no captions in SD" << endl;
	if (no608 && no708 && !NTV2_IS_SD_VIDEO_FORMAT(config.fVideoFormat))
		cerr	<< "## WARNING:  '--no608' and '--no708' options will result in no captions in HD" << endl;
	if (NTV2_IS_SD_VIDEO_FORMAT(config.fVideoFormat))
		no708 = true;
	else
		noLine21 = true;

	//	Pixel Format
	const string pixelFormatStr (pPixelFormat  ?  pPixelFormat  :  "");
	config.fPixelFormat = pixelFormatStr.empty() ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
	if (pixelFormatStr == "?"  ||  pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;}
	else if (!pixelFormatStr.empty()  &&  !NTV2_IS_VALID_FRAME_BUFFER_FORMAT(config.fPixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	AutoCirculate Frames
	static const string	legalFramesSpec ("{frameCount}[@{firstFrameNum}]  or  {firstFrameNum}-{lastFrameNum}");
	const string framesSpec (pFramesSpec ? pFramesSpec : "");
	if (!framesSpec.empty())
	{
		const string parseResult(config.fFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## " << parseResult << endl;  return 1;}
	}
	if (!config.fFrames.valid())
		{cerr << "## ERROR:  Bad 'frames' spec '" << framesSpec << "'\n## Expected " << legalFramesSpec << endl;  return 1;}

	//	Output Spigot
	const string legalOutputs (CNTV2DemoCommon::GetOutputDestinationStrings(deviceSpec));
	const string outputDestStr (pOutputDest ? CNTV2DemoCommon::ToLower(string(pOutputDest)) : "");
	NTV2OutputDestination outputSpigot(NTV2_OUTPUTDESTINATION_INVALID);
	if (outputDestStr == "?" || outputDestStr == "list")
		{cout << legalOutputs << endl;  return 0;}
	if (!outputDestStr.empty())
	{
		outputSpigot = CNTV2DemoCommon::GetOutputDestinationFromString(outputDestStr);
		if (!NTV2_IS_VALID_OUTPUT_DEST(outputSpigot))
			{cerr << "## ERROR:  Output '" << outputDestStr << "' not of:" << endl << legalOutputs << endl;	return 1;}
	}	//	if output spigot specified

	if (!NTV2_IS_VALID_OUTPUT_DEST(outputSpigot)  &&  !NTV2_IS_VALID_CHANNEL(config.fOutputChannel))
	{
		config.fOutputChannel = NTV2_CHANNEL1;
		outputSpigot = ::NTV2ChannelToOutputDestination(config.fOutputChannel);
	}
	else if (!NTV2_IS_VALID_OUTPUT_DEST(outputSpigot)  &&  NTV2_IS_VALID_CHANNEL(config.fOutputChannel))
		outputSpigot = ::NTV2ChannelToOutputDestination(config.fOutputChannel);
	else if (NTV2_IS_VALID_OUTPUT_DEST(outputSpigot)  &&  !NTV2_IS_VALID_CHANNEL(config.fOutputChannel))
		config.fOutputChannel = ::NTV2OutputDestinationToChannel(outputSpigot);

	//	Test Pattern
	string testPattern(pTestPattern  ?  pTestPattern  :  "");
	if (testPattern == "?" || testPattern == "list")
		{cout << CNTV2DemoCommon::GetTestPatternStrings() << endl;  return 0;}
	if (!testPattern.empty())
	{
		testPattern = CNTV2DemoCommon::GetTestPatternNameFromString(testPattern);
		if (testPattern.empty())
		{
			cerr	<< "## ERROR:  Invalid '--pattern' value '" << pTestPattern << "' -- expected values:" << endl
					<< CNTV2DemoCommon::GetTestPatternStrings() << endl;
			return 2;
		}
	}
	else
		testPattern = "Flat Field";

	//	NOTE:	From one command line invocation, you can inject different captions into separate caption channels.
	//			For example:
	//	./bin/ntv2ccplayer	--device kona4  --channel 3  --stats  --608chan cc1,cc2,cc3,cc4,tx1,tx2,tx3,tx4
	//						--608mode pop,paint,roll3,roll4  --rate 1000,700,500,300,200
	//						--end idle,loop,idle,loop,idle,loop,idle,loop
	//						English.txt  Spanish.txt  French.txt  German.txt  txt1  txt2  txt3  txt4

	//	Users can play one or more caption channels by specifying more than one, separating each with a comma:
	//			--608chan cc2,cc4,tx1,tx2
	//	You can vary the mode, end-action and rate for each caption channel in the same way:
	//			--608mode pop,paint,roll  --rate 1500,800,300,600
	const NTV2StringList	sCaptionChannels(aja::split(CNTV2DemoCommon::ToLower(pCaptionChan	? pCaptionChan	: "CC1"), ','));
	const NTV2StringList	sCaptionModes	(aja::split(CNTV2DemoCommon::ToLower(pMode			? pMode			: "roll4"), ','));
	const NTV2StringList	sEndActions		(aja::split(CNTV2DemoCommon::ToLower(pEndAction		? pEndAction	: "exit"), ','));
	const NTV2StringList	sCaptionRates	(aja::split(CNTV2DemoCommon::ToLower(pCaptionRate	? pCaptionRate	: "500"), ','));
	size_t					ndx				(0);
	size_t					fileNdx			(0);

	for (NTV2StringListConstIter iter (sCaptionChannels.begin());  iter != sCaptionChannels.end();  ++iter, ++ndx)
	{
		CCGenConfig	generatorConfig;

		//	Caption Channel
		generatorConfig.fCaptionChannel = ::StrToNTV2Line21Channel (*iter);
		if (!IsValidLine21Channel (generatorConfig.fCaptionChannel) || IsLine21XDSChannel (generatorConfig.fCaptionChannel))
			{cerr << "## ERROR:  Bad '608chan' value '" << pCaptionChan << "' -- expected '" << ::GetLine21ChannelNames () << "'" << endl;	return 1;}

		//	Caption Mode
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

		//	Newline	
		generatorConfig.fNewLinesAreNewRows = breakNewLines ? true : false;
		if (IsLine21TextChannel (generatorConfig.fCaptionChannel))
			generatorConfig.fNewLinesAreNewRows = true;	//	Tx1/Tx2/Tx3/Tx4 always break rows on newLine chars

		//	End Action
		const string	sEndAction	(ndx < sEndActions.size() ? sEndActions.at(ndx) : sEndActions.back());
		if (sEndAction.empty () || sEndAction == "quit" || sEndAction == "exit" || sEndAction == "terminate" || sEndAction == "end")
			generatorConfig.fEndAction = AtEndAction_Quit;
		else if (sEndAction == "loop" || sEndAction == "repeat")
			generatorConfig.fEndAction = AtEndAction_Repeat;
		else if (sEndAction == "sleep" || sEndAction == "idle" || sEndAction == "rest")
			generatorConfig.fEndAction = AtEndAction_Idle;
		else
			{cerr << "## ERROR:  Bad 'end' parameter '" << sEndAction << "' -- expected 'loop|repeat | rest|sleep|idle | quit|end|exit|terminate'" << endl;	return 1;}

		//	Caption Rate
		const string	sCaptionRate	(ndx < sCaptionRates.size() ? sCaptionRates.at(ndx) : sCaptionRates.back());
		istringstream iss(sCaptionRate);
		uint32_t	charsPerMin;
		iss >> charsPerMin;
		generatorConfig.fCharsPerMinute = double(charsPerMin);

		if (!pathList.empty())
		{	//	Text Files to Play
			generatorConfig.fFilesToPlay.push_back (fileNdx < pathList.size() ? pathList.at(fileNdx) : pathList.back());
			++fileNdx;
			if ((ndx+1) == sCaptionChannels.size())
				while (fileNdx < pathList.size())
					generatorConfig.fFilesToPlay.push_back (pathList.at (fileNdx++));
		}
		config.fCapChanGenConfigs [generatorConfig.fCaptionChannel] = generatorConfig;	//	Add this generator to the player configuration
	}	//	for each specified caption channel

	config.fTestPatternName		= testPattern;
	config.fOutputDest			= outputSpigot;
	config.fForceRTP			= uint16_t(forceRTP);
	config.fVancMode			= forceVanc		? NTV2_VANCMODE_TALLER : NTV2_VANCMODE_OFF;
	config.fDoMultiFormat		= doMultiFormat	? true	: false;			//	Multiformat mode?
	config.fDoTsiRouting		= doSquares		? false	: true;
	config.fDoRGBOnWire			= doRGBOnWire	? true	: false;
	config.fSuppressLine21		= noLine21		? true : false;
	config.fSuppress608			= no608			? true : false;
	config.fSuppress708			= no708			? true : false;
	config.fSuppressAudio		= noAudio		? true : false;
	config.fSuppressTimecode	= noTimecode	? true : false;
	config.fEmitStats			= bEmitStats	? true : false;

	//	Instantiate and initialize the NTV2CCPlayer object...
	NTV2CCPlayer player(config);
	AJAStatus status = player.Init();
	if (AJA_FAILURE(status))
		{cout << "## ERROR:  Initialization failed: " << ::AJAStatusToString(status) << endl;	return 1;}

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Run it...
	player.Run();

	size_t	msgsQued (0), bytesQued (0), totMsgsEnq (0), totBytesEnq (0), totMsgsDeq (0), totBytesDeq (0), maxQueDepth (0), droppedFrames (0);
	if (bEmitStats)
		cout	<< "  Current  Current    Total    Total    Total    Total      Max         " << endl
				<< " Messages    Bytes Messages    Bytes Messages    Bytes    Queue  Dropped" << endl
				<< "   Queued   Queued Enqueued Enqueued Dequeued Dequeued    Depth   Frames" << endl;

	//	Loop until we're told to stop...
	do
	{	//	Poll the player's encoder's status until stopped...
		player.GetStatus (msgsQued, bytesQued, totMsgsEnq, totBytesEnq, totMsgsDeq, totBytesDeq, maxQueDepth, droppedFrames);
		if (bEmitStats)
			cout	<<	setw(9) << msgsQued
					<<	setw(9) << bytesQued
					<<	setw(9) << totMsgsEnq
					<<	setw(9) << totBytesEnq
					<<	setw(9) << totMsgsDeq
					<<	setw(9) << totBytesDeq
					<<	setw(9) << maxQueDepth
					<<	setw(9) << droppedFrames  << "\r" << flush;
	} while (!gGlobalQuit);	//	loop til done

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
