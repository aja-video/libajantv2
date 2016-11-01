/**
	@file		qacrc/main.cpp
	@brief		testcrc take 2
	@copyright	Copyright (C) 2012-2015 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ntv2utils.h"
#include "ajastuff/common/options_popt.h"
#include "qacrc.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#include "ajastuff/system/systemtime.h"
#include "../../demoapps/ntv2democommon.h"


//	Globals
static bool		gGlobalQuit		(false);	//	Set this "true" to exit gracefully

#if defined (AJA_WINDOWS)
BOOL SignalHandler(DWORD inSignal)
#else
static void SignalHandler (int inSignal)
#endif
{
	(void) inSignal;
	gGlobalQuit = true;
#if defined (AJA_WINDOWS)
	return true;
#endif
}


int main (int argc, const char ** argv)
{	
	QACRC_DATA crcData;

	int source(0);
	int quiet(0);
	int manualMode(0);
	int mAudioAlignment(0);
	int videoChannel(1);
	int useAncFirmware(0);
	int logType(0);
	int useStaticImage(0);
	int burnTimeCode(0);
	char* pAudioData(NULL);
	char* pFrameIndex(NULL);
	char* pAudioLevel(NULL);
	char* pDataRange(NULL);
	char* pVideoFormat(NULL);
	char* pPixelFormat(NULL);
	char* pDeviceSpec(NULL);
	char* pLogFileName(NULL);
	char* pOutputFileName(NULL);
	char* pTrickMode(NULL);
	//char* pRefFileName(NULL);
	char* pVerbose(NULL);

	const struct poptOption userOptionsTable[] =
	{
		{ "audioData", 'a', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pAudioData, 'a', "[N]  test audio data, N = bit depth(16, 20, 24)", NULL },
		{ "board", 'b', POPT_ARG_STRING, &pDeviceSpec, 0, "Which device to use", "index#" },
		{ "channel", 'c', POPT_ARG_INT, &crcData.mAudioChannelFirst, 0, "Which audio channel to use", "number of the audio channel" },
		{ "dataInspectionLevel", 'd', POPT_ARG_INT, &crcData.mDataInspectionLevel, 0, "0=bytexbyte default, 1=macro compare", NULL },
		{ "audioLevel", 'e', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pAudioLevel, 'e', "[N]  test audio level", NULL },
		{ "frameIndex", 'f', POPT_ARG_STRING, &pFrameIndex, 0, "F/L  F = first frame index L = last frame index", NULL },
		{ "dataRange", 'g', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pDataRange, 'g', "[L/H]  test video data L/H = data range", NULL },
		{ "videoFormat", 'i', POPT_ARG_STRING, &pVideoFormat, 0, "Which video format to use", NULL },
		{ "pixelFormat", 'j', POPT_ARG_STRING, &pPixelFormat, 0, "Which pixel format to use", NULL },
		{ "useAncFirmware", 'k', POPT_ARG_NONE, &useAncFirmware, 0, "Use the firmware extractor/inserter", NULL },
		{ "logFileName", 'l', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pLogFileName, 'l', "[NAME] = log file name(testcrc.log)", NULL },
		{ "manualMode", 'm', POPT_ARG_NONE, &manualMode, 0, "Manual mode: no routing or audio setup", NULL },
		{ "videoChannel", 'n', POPT_ARG_INT, &videoChannel, 0, "Which video channel to use", NULL },
		{ "outputFileName", 'o', POPT_ARG_STRING, &pOutputFileName, 'o', "[NAME] = output image file name (testout.bin)", NULL },
		{ "trickMode", 'p', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pTrickMode, 0, "P = play frame interval, [S] = number of dropped frames default = 1", NULL },
		{ "quiet", 'q', POPT_ARG_NONE, &quiet, 0, "No output", NULL },
		{ "useReferenceAsLock", 'r', POPT_ARG_INT, &crcData.mbUseReferenceAsLock, 'r', "Lock to reference signal", NULL },
		//{ "referenceFile", 'r', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pRefFileName, 'r', "[NAME] = reference image file name (testref.bin)", NULL },
		{ "source", 's', POPT_ARG_NONE, &source, 0, "Make this the source instance", NULL },
		{ "testCount", 't', POPT_ARG_INT, &crcData.mTestCount, 0, "Number of frames to process", NULL },
		{ "audioAlignment", 'u', POPT_ARG_NONE, &mAudioAlignment, 0, "Enable audio channel alignment errors", NULL },
		{ "verbose", 'v', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pVerbose, 'v', "[N] number of diffs, default = 20", NULL },
		{ "wiggleRoom", 'w', POPT_ARG_INT, &crcData.mWiggleRoom, 0, "Only report errors greater than N (set to 3 for RGB via CSC)", NULL },
		{ "staticImage", 'x', POPT_ARG_NONE, &useStaticImage, 0, "Do not step through the pattern - allows scope crc checking", NULL },
		{ "burnTimeCode", 'y', POPT_ARG_NONE, &burnTimeCode, 0, "Burn Timecode onto output, static image only", NULL },
		{ "logType", 'z', POPT_ARG_INT, &logType, 0, "Select what to log: 0 = all, 1 = Video, 2 = Audio, 3 = Anc", NULL },
		POPT_AUTOHELP
		POPT_TABLEEND
};

	//	Read command line arguments...
	poptContext		optionsContext;
	optionsContext = ::poptGetContext(NULL, argc, argv, userOptionsTable, 0);

	int rc;
	while ((rc = ::poptGetNextOpt(optionsContext)) >= 0)
	{
		switch (rc)
		{
		case 'g':
			if (!pDataRange)
			{
				pDataRange = "04/fb";
			}
			break;
		case 'a':
			if (!pAudioData)
			{
				pAudioData = "16";
			}
			break;
		case 'e':
			if (!pAudioLevel)
			{
				pAudioLevel = "100";
			}
			break;
		case 'l':
			if (!pLogFileName)
			{
				pLogFileName = "testcrc.log";
			}
			break;
		case 'o':
			if (!pOutputFileName)
			{
				pOutputFileName = "testcrcout.bin";
			}
			break;
// 		case 'r':
// 			if (!pRefFileName)
// 			{
// 				pRefFileName = "testcrcref.bin";
// 			}
// 			break;
		case 'v':
			if (!pVerbose)
			{
				pVerbose = "20";
			}
			break;
		}
	}
	optionsContext = ::poptFreeContext(optionsContext);

	crcData.mbSource = source ? true : false;
	crcData.mVideoChannel = (NTV2Channel)(videoChannel - 1);
	crcData.mAudioSystem = ::NTV2ChannelToAudioSystem(crcData.mVideoChannel);
	crcData.mbQuiet = quiet ? true : false;
	crcData.mbManualMode = manualMode ? true : false;
	crcData.mbAudioAlignment = mAudioAlignment ? true : false;
	crcData.mbWithAnc = useAncFirmware ? true : false;
	crcData.mbUseStaticImage = useStaticImage ? true : false;
	crcData.mbBurnTimecode = burnTimeCode ? true : false;

	if (crcData.mbSource)
	{
		crcData.mbVideoData = true;
		crcData.mbAudioData = true;
		crcData.mbWithAnc = true;
	}

	switch (logType)
	{
	case 1:
		crcData.mbLogAudioErrors = false;
		crcData.mbLogAncErrors = false;
		break;
	case 2:
		crcData.mbLogVideoErrors = false;
		crcData.mbLogAncErrors = false;
		break;
	case 3:
		crcData.mbLogVideoErrors = false;
		crcData.mbLogAudioErrors = false;
		break;
	default:
		crcData.mbLogVideoErrors = true;
		crcData.mbLogAudioErrors = true;
		crcData.mbLogAncErrors = true;		
	}

	const std::string audioDataString(pAudioData ? pAudioData : "");
	if (!audioDataString.empty())
	{
		int tempData0 = 0;
		tempData0 = 0;
		if (sscanf(audioDataString.c_str(), "%d", &tempData0) == 1)
		{
			if ((tempData0 != 16) && (tempData0 != 20) && (tempData0 != 24))
			{
				fprintf(stderr, "## ERROR:  Bad audio depth\n");
			}
			crcData.mAudioDepth = tempData0;
		}
		crcData.mbAudioData = true;
		crcData.mbAudioLevel = false;
	}

	const std::string deviceSpec(pDeviceSpec ? pDeviceSpec : "");
	if (!deviceSpec.empty())
	{
		int tempData0 = 0;
		ULWord tempDataCount = 0;
		tempDataCount = sscanf(deviceSpec.c_str(), "%d", &tempData0);
		if (tempDataCount == 1)
		{
			crcData.mDeviceIndex = (ULWord)tempData0;
		}
		else
		{
			fprintf(stderr, "## ERROR:  Missing device index\n");
		}
	}

	const std::string audioLevelString(pAudioLevel ? pAudioLevel : "");
	if (!audioLevelString.empty())
	{
		ULWord tempData0 = 0;
		crcData.mbAudioLevel = true;
		crcData.mbAudioData = false;
		if (sscanf(audioLevelString.c_str(), "%u", &tempData0) == 1)
		{
			crcData.mAudioLevel = tempData0;
		}
	}

	const std::string frameIndexString(pFrameIndex ? pFrameIndex : "");
	if (!frameIndexString.empty())
	{
		int tempData0 = 0;
		int tempData1 = 0;
		ULWord tempDataCount = sscanf(frameIndexString.c_str(), "%d/%d", &tempData0, &tempData1);
		if (tempDataCount == 2)
		{
			crcData.mFirstFrameIndex = (ULWord)tempData0;
			crcData.mLastFrameIndex = (ULWord)tempData1;
		}
		else
		{
			fprintf(stderr, "## ERROR:  Missing frame index\n");
		}
	}

	const std::string dataRangeString(pDataRange ? pDataRange : "");
	if (!dataRangeString.empty())
	{
		int tempData0 = 0;
		int tempData1 = 0;
		ULWord tempDataCount = sscanf(dataRangeString.c_str(), "%x/%x", &tempData0, &tempData1);
		if (tempDataCount == 2)
		{
			crcData.mVideoRangeLow = (ULWord)tempData0;
			crcData.mVideoRangeHigh = (ULWord)tempData1;
		}
		crcData.mbVideoData = true;
	}

// 	crcData.msRefFileName = pRefFileName ? pRefFileName : "";
// 	if (!crcData.msRefFileName.empty())
// 	{
// 		crcData.mbUseReferenceFile = true;
// 	}

	crcData.msLogFileName = pLogFileName ? pLogFileName : "";
	if (!crcData.msLogFileName.empty())
	{
		crcData.mbLog = true;
	}

	crcData.msOutputFileName = pOutputFileName ? pOutputFileName : "";
	if (!crcData.msOutputFileName.empty())
	{
		crcData.mbOutputImageToFile = true;
	}

	const std::string trickModeString(pTrickMode ? pTrickMode : "");
	if (!trickModeString.empty())
	{
		int tempData0 = 0;
		int tempData1 = 0;
		ULWord tempDataCount = sscanf(trickModeString.c_str(), "%d/%d", &tempData0, &tempData1);
		if (tempDataCount == 2)
		{
			crcData.mPlayFrames = (ULWord)tempData0;
			crcData.mSkipFrames = (ULWord)tempData1;
		}
		else if (tempDataCount == 1)
		{
			crcData.mPlayFrames = (ULWord)tempData0;
			crcData.mSkipFrames = 1;
		}
		else
		{
			fprintf(stderr, "## ERROR:  Missing data range index\n");
		}
		crcData.mbTrickMode = true;
	}

	const std::string verboseString(pVerbose ? pVerbose : "");
	if (!verboseString.empty())
	{
		ULWord tempData0 = 0;
		if (sscanf(verboseString.c_str(), "%u", &tempData0) == 1)
		{
			crcData.mVerboseCount = tempData0;
		}
	}

	const string	videoFormatStr(pVideoFormat ? pVideoFormat : "");
	NTV2VideoFormat	videoFormat(videoFormatStr.empty() ? NTV2_FORMAT_1080i_5994 : CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr));
	if (videoFormat == NTV2_FORMAT_UNKNOWN)
		videoFormat = videoFormatStr.empty() ? NTV2_FORMAT_1080i_5994 : CNTV2DemoCommon::GetVideoFormatFromString(videoFormatStr, true);
	if (videoFormatStr == "?" || videoFormatStr == "list")
	{
		cout << CNTV2DemoCommon::GetVideoFormatStrings(BOTH_VIDEO_FORMATS, deviceSpec) << endl;  return 0;
	}
	else if (!videoFormatStr.empty() && videoFormat == NTV2_FORMAT_UNKNOWN)
	{
		cerr << "## ERROR:  Invalid '--videoFormat' value '" << videoFormatStr << "' -- expected values:" << endl
			<< CNTV2DemoCommon::GetVideoFormatStrings(BOTH_VIDEO_FORMATS, deviceSpec) << endl;
		return 2;
	}
	else if (videoFormatStr.empty() && !crcData.mbSource)
	{
		videoFormat = NTV2_FORMAT_UNKNOWN;
	}
	crcData.mVideoFormat = videoFormat;

	const string				pixelFormatStr(pPixelFormat ? pPixelFormat : "");
	const NTV2FrameBufferFormat	pixelFormat(pixelFormatStr.empty() ? NTV2_FBF_10BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr));
	if (pixelFormatStr == "?" || pixelFormatStr == "list")
	{
		cout << CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;  return 0;
	}
	else if (!pixelFormatStr.empty() && !NTV2_IS_VALID_FRAME_BUFFER_FORMAT(pixelFormat))
	{
		cerr << "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
			<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}
	crcData.mPixelFormat = pixelFormat;

#if defined (AJA_WINDOWS)
	SetConsoleCtrlHandler(SignalHandler, true);
#else
	::signal(SIGINT, SignalHandler);
#endif
#if defined (AJAMac)
	::signal(SIGHUP, SignalHandler);
	::signal(SIGQUIT, SignalHandler);
#endif

	QACrc qaCrcer;
	if(AJA_SUCCESS(qaCrcer.Init(crcData)))
		qaCrcer.Run();
	else
	{
		qaCrcer.Quit();
		return -1;
	}

	if (crcData.mbSource)
	{
		cout << "   Frames   Buffer   Frames   VError" << endl
			<< "Processed    Level  Dropped   Insert" << endl;
		do
		{
			ULWord	framesProcessed, framesDropped, bufferLevel, videoErrors, audioErrors, ancF1Errors, ancF2Errors, videoErrorInsertionCount = 0;

			//	Poll the player's status...
			qaCrcer.GetStatus(framesProcessed, framesDropped, bufferLevel, videoErrors, audioErrors, ancF1Errors, ancF2Errors, videoErrorInsertionCount);
			cout << setw(9) << framesProcessed << setw(9) << bufferLevel << setw(9) << framesDropped << setw(9) << videoErrorInsertionCount << "\r" << flush;
			AJATime::Sleep(100);
		} while (qaCrcer.IsRunning() && !gGlobalQuit);	//	loop til done
	}
	else
	{
		cout << "   Frames   Buffer   Frames    Video    Audio    AncF1    AncF2     VIns" << endl
			<< "Processed    Level  Dropped   Errors   Errors   Errors   Errors   Detect" << endl;
		do
		{
			ULWord	framesProcessed, framesDropped, bufferLevel, videoErrors, audioErrors, ancF1Errors, ancF2Errors, videoErrorInsertionCount = 0;

			//	Poll the player's status...
			qaCrcer.GetStatus(framesProcessed, framesDropped, bufferLevel, videoErrors, audioErrors, ancF1Errors, ancF2Errors, videoErrorInsertionCount);
			cout << setw(9) << framesProcessed << setw(9) << bufferLevel << setw(9) << framesDropped << setw(9) << videoErrors << setw(9) << audioErrors << setw(9) << ancF1Errors << setw(9) << ancF2Errors << setw(9) << videoErrorInsertionCount << "\r" << flush;
			AJATime::Sleep(100);
		} while (qaCrcer.IsRunning() && !gGlobalQuit);	//	loop til done
	}

	qaCrcer.Quit();

	cout << endl;

	return 0;
 
}	//	main
