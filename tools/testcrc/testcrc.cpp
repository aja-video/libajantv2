/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/testcrc/testcrc.cpp
	@brief		Implements 'testcrc' command-line tool.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.
**/

#include <stdio.h>
#include <iostream>
#include <string>
#include <signal.h>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ajabase/common/options_popt.h"

static const int s_RefBufferExtraSize(1024);

using namespace std;

#ifdef MSWindows
#pragma warning(disable : 4996)
#endif

#ifdef AJALinux
#include "ntv2linuxpublicinterface.h"
#endif

static int s_iTestCount = 0;

static NTV2Channel gChannelByIndex[] = {
	NTV2_CHANNEL_INVALID,	//	Never indexed by zero
	NTV2_CHANNEL1, NTV2_CHANNEL2, NTV2_CHANNEL3, NTV2_CHANNEL4,
	NTV2_CHANNEL5, NTV2_CHANNEL6, NTV2_CHANNEL7, NTV2_CHANNEL8
};

void SignalHandler(int signal)
{
	(void) signal;
	s_iTestCount = (-1);
}

int main(int argc, const char ** argv)
{
	bool mbLog					(false);
	bool mbOutputImageToFile	(false);
	bool mbParseError			(false);
	bool mbTestError			(false);
	bool mbUseReferenceFile		(false);
	bool mbVerbose				(false);

	char* pFrameIndex		(NULL);
	char* pDeviceSpec		(NULL);
	char* pLogFileName		(NULL);
	char* pOutputFileName	(NULL);
	char* pRefFileName		(NULL);
	char* pVerbose			(NULL);

	int mDeviceIndex		(0);
	int mInVideoChannel		(1);
	int mFirstFrameIndex	(0);
	int mLastFrameIndex		(7);
	int mQuiet				(0);
	int mVerboseCount		(20);

	char msOutFileName[MAX_PATH];
	char msRefFileName[MAX_PATH];
	char msLogFileName[MAX_PATH];

	FILE* mpLogFile(NULL);
	FILE* mpOutFile(NULL);
	FILE* mpRefFile(NULL);

	NTV2Channel mVideoChannel	= NTV2_CHANNEL1;

	AUTOCIRCULATE_TRANSFER mTransfer;
	AUTOCIRCULATE_STATUS mACStatus;

	const struct poptOption userOptionsTable[] =
	{
        { "board", 'b', POPT_ARG_STRING, &pDeviceSpec, 0, "Which device to use", "index#" },
		{ "channel", 'c', POPT_ARG_INT, &mInVideoChannel, 0, "Which video channel to use", NULL },
		{ "frameIndex", 'f', POPT_ARG_STRING, &pFrameIndex, 0, "F/L  F = first frame index L = last frame index", NULL },
		{ "logFileName", 'l', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pLogFileName, 'l', "[NAME] = log file name(testcrc.log)", NULL },
		{ "outputFileName", 'o', POPT_ARG_STRING, &pOutputFileName, 'o', "[NAME] = output image file name (testout.bin)", NULL },
		{ "quiet", 'q', POPT_ARG_NONE, &mQuiet, 0, "No output", NULL },
		{ "referenceFile", 'r', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pRefFileName, 'r', "[NAME] = reference image file name (testref.bin)", NULL },
		{ "testCount", 't', POPT_ARG_INT, &s_iTestCount, 0, "Number of frames to process", NULL },
		{ "verbose", 'v', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, &pVerbose, 'v', "[N] verbose output, N = number of diffs default = 20", NULL },
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
		case 'l':
			if (!pLogFileName)
			{
				pLogFileName = (char *)"testcrc.log";
			}
			break;
		case 'o':
			if (!pOutputFileName)
			{
				pOutputFileName = (char *)"testcrcout.bin";
			}
			break;
		case 'r':
			if (!pRefFileName)
			{
				pRefFileName = (char *)"testcrcref.bin";
			}
			break;
		case 'v':
			if (!pVerbose)
			{
				pVerbose = (char *)"20";
			}
			break;
		}
	}
	optionsContext = ::poptFreeContext(optionsContext);

	const std::string deviceSpec(pDeviceSpec ? pDeviceSpec : "");
	if (!deviceSpec.empty())
	{
		int tempData0 = 0;
		ULWord tempDataCount = 0;
		tempDataCount = sscanf(deviceSpec.c_str(), "%d", &tempData0);
		if (tempDataCount == 1)
		{
			mDeviceIndex = (ULWord)tempData0;
		}
		else
		{
			fprintf(stderr, "## ERROR:  Missing device index\n");
			mbParseError = true;
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
			mFirstFrameIndex = (ULWord)tempData0;
			mLastFrameIndex = (ULWord)tempData1;
		}
		else
		{
			fprintf(stderr, "## ERROR:  Missing frame index\n");
			mbParseError = true;
		}
	}

	const std::string logFileNameString(pLogFileName ? pLogFileName : "");
	if (!logFileNameString.empty())
	{
		strcpy(msLogFileName, logFileNameString.c_str());
		mbLog = true;
	}

	const std::string outputFileNameString(pOutputFileName ? pOutputFileName : "");
	if (!outputFileNameString.empty())
	{
		strcpy(msOutFileName, outputFileNameString.c_str());
		mbOutputImageToFile = true;
	}

	const std::string referenceFileNameString(pRefFileName ? pRefFileName : "");
	if (!referenceFileNameString.empty())
	{
		strcpy(msRefFileName, referenceFileNameString.c_str());
		mbUseReferenceFile = true;
	}

	const std::string verboseString(pVerbose ? pVerbose : "");
	if (!verboseString.empty())
	{
		ULWord tempData0 = 0;
		mbVerbose = true;
		if (sscanf(verboseString.c_str(), "%u", &tempData0) == 1)
		{
			mVerboseCount = tempData0;
		}
	}

	try
	{
		signal(SIGINT, SignalHandler);

		if(mbParseError)
		{
			throw 0;
		}

		if(mbLog)
		{
			mpLogFile = fopen(msLogFileName, "a");
			if(mpLogFile == NULL)
			{
				fprintf (stderr, "## ERROR:  Cannot open log file '%s'\n", msLogFileName);
				throw 0;
			}
		}

		NTV2DeviceInfo boardInfo;
		CNTV2DeviceScanner ntv2BoardScan;
		if(ntv2BoardScan.GetNumDevices() <= (ULWord)mDeviceIndex)
		{
			fprintf (stderr, "## ERROR:  Opening device %d failed\n", mDeviceIndex);
			throw 0;
		}
		boardInfo = ntv2BoardScan.GetDeviceInfoList()[mDeviceIndex];

		CNTV2Card mDevice(boardInfo.deviceIndex);

		// find the board
		if(mDevice.IsOpen() == false)
		{
			if (mDevice.IsIPDevice())
			{
				uint32_t mbTimeoutCount = 0;
				while (!mDevice.IsMBSystemReady() || mbTimeoutCount > 10000)
				{
					mbTimeoutCount++;
				}
			}
			fprintf (stderr, "## ERROR:  Opening device %d failed\n", mDeviceIndex);
			throw 0;
		}
		NTV2DeviceID eBoardID = mDevice.GetDeviceID();

		mDevice.SetEveryFrameServices(NTV2_OEM_TASKS);

		int numFrameStores = ::NTV2DeviceGetNumFrameStores(eBoardID);

		if (mInVideoChannel < 1)
			mInVideoChannel = 1;
		if (mInVideoChannel > numFrameStores)
			mInVideoChannel = numFrameStores;

		if (numFrameStores < mInVideoChannel)
		{
			fprintf (stderr, "## ERROR:  This device does not support the frame store specified\n");
			throw 0;
		}
		mVideoChannel = gChannelByIndex[mInVideoChannel];

		NTV2VideoFormat videoFormat;
		mDevice.GetVideoFormat(videoFormat);

		NTV2FrameBufferFormat frameBufferFormat;
		mDevice.GetFrameBufferFormat(mVideoChannel, frameBufferFormat);

		NTV2FrameGeometry frameGeomety;
		mDevice.GetFrameGeometry(frameGeomety);

		int iFrameSize = ::NTV2DeviceGetFrameBufferSize(eBoardID, frameGeomety, frameBufferFormat);
		int iFrameCount = ::NTV2DeviceGetNumberFrameBuffers(eBoardID, frameGeomety, frameBufferFormat);

		if(mFirstFrameIndex >= iFrameCount)
		{
			mFirstFrameIndex = iFrameCount - 1;
		}
		if(mLastFrameIndex >= iFrameCount)
		{
			mLastFrameIndex = iFrameCount - 1;
		}
		if(mFirstFrameIndex < 0)
		{
			mFirstFrameIndex = 0;
		}
		if(mLastFrameIndex < 0)
		{
			mLastFrameIndex  = 0;
		}

		ULWord* pVidSrcBuffer = (ULWord*)new char[iFrameSize];
		ULWord*	pVidRefBuffer = (ULWord*)new char[iFrameSize + s_RefBufferExtraSize];

		memset(pVidSrcBuffer, 0, iFrameSize);
		memset(pVidRefBuffer, 0, iFrameSize + s_RefBufferExtraSize);

		if(mbUseReferenceFile)
		{
			mpRefFile = fopen(msRefFileName, "rb");
			if(mpRefFile == NULL)
			{
				fprintf (stderr, "## ERROR:  Cannot open reference image file '%s'\n", msRefFileName);
				throw 0;
			}

			int iSize = (int) fread(pVidRefBuffer, 4, iFrameSize/4, mpRefFile);
			if(ferror(mpRefFile))
			{
				fprintf (stderr, "## ERROR:  Cannot read reference image file '%s'\n", msRefFileName);
				throw 0;
			}

			fclose(mpRefFile);

			iSize *= 4;
			if(iSize < iFrameSize)
			{
				iFrameSize = iSize;
			}
		}

		NTV2VANCMode vMode;
		mDevice.GetVANCMode(vMode);
		ULWord ulActiveVideoSize = GetVideoActiveSize(videoFormat, frameBufferFormat, vMode);
		if((ULWord)iFrameSize > ulActiveVideoSize)
		{
			iFrameSize = ulActiveVideoSize;
		}

		mDevice.SubscribeInputVerticalEvent(mVideoChannel);

		mDevice.AutoCirculateStop(mVideoChannel);
		mDevice.WaitForInputFieldID(NTV2_FIELD0, mVideoChannel);
		mDevice.AutoCirculateStop(mVideoChannel);
		mDevice.WaitForInputFieldID(NTV2_FIELD0, mVideoChannel);

		mDevice.AutoCirculateInitForInput(mVideoChannel,
										  0,
										  NTV2_AUDIOSYSTEM_INVALID,
										  0,
										  1,
										  mFirstFrameIndex,
										  mLastFrameIndex);

		mDevice.AutoCirculateStart(mVideoChannel);
		mDevice.WaitForInputFieldID(NTV2_FIELD0, mVideoChannel);

		int iSrc = 0;
		int iTest = 0;
		int iError = 0;
		int iFrame = 0;
		int iDiffs = 0;
		int iVCount = 0;
		bool bError = false;
		bool bTestFormat = false;

		while((s_iTestCount == 0) || (iTest < s_iTestCount))
		{
			mDevice.AutoCirculateGetStatus(mVideoChannel, mACStatus);
			if(mACStatus.acBufferLevel > 1)
			{
				memset(pVidSrcBuffer, 0, ulActiveVideoSize);
				mTransfer.SetVideoBuffer(pVidSrcBuffer, ulActiveVideoSize);
				if (!mDevice.AutoCirculateTransfer(mVideoChannel, mTransfer))
				{
					fprintf (stderr, "## ERROR:  TransferWithAutoCirculate failed!\n");
				}

				iFrame = mTransfer.acTransferStatus.acTransferFrame;

				if(iTest == 0)
				{
					if(!mbUseReferenceFile)
					{
						memcpy(pVidRefBuffer, pVidSrcBuffer, ulActiveVideoSize);
					}

					if(mbOutputImageToFile)
					{
						mpOutFile = fopen(msOutFileName, "wb");
						if(mpOutFile == NULL)
						{
							fprintf (stderr, "error: can not open output image file %s\n", msOutFileName);
							throw 0;
						}
						
						fwrite(pVidRefBuffer, 4, iFrameSize/4, mpOutFile);
						if(ferror(mpOutFile))
						{
							fprintf (stderr, "error: can not write output image file %s\n", msOutFileName);
							throw 0;
						}
						
						fclose(mpOutFile);
					}
				}

				iVCount = 0;
				bError = false;
				for(iSrc = 0; iSrc < iFrameSize/4; iSrc++)
				{
					if(pVidSrcBuffer[iSrc] != pVidRefBuffer[iSrc])
					{
						bError = true;
						mbTestError = true;

						if(iVCount < mVerboseCount)
						{
							if(!mQuiet && mbVerbose)
							{
								if(bTestFormat)
								{
									printf("\n");
								}
								bTestFormat = false;
								fprintf (stderr, "frm %9d  buf %2d  off %08x  srx %08x %08x %08x\n",
										 iTest, iFrame, iSrc*4, pVidSrcBuffer[iSrc], pVidRefBuffer[iSrc],
										 pVidSrcBuffer[iSrc]^pVidRefBuffer[iSrc]);
							}
							if(mpLogFile != NULL)
							{
								fprintf(mpLogFile, "bad frame %9d  fb %2d  off %08x  src %08x  ref %08x  xor %08x\n",
										iTest, iFrame, iSrc*4, pVidSrcBuffer[iSrc], pVidRefBuffer[iSrc],
										pVidSrcBuffer[iSrc]^pVidRefBuffer[iSrc]);
							}
							if(mVerboseCount > 0)
							{
								iVCount++;
							}
						}
						
						iDiffs++;
					}

					if((s_iTestCount != 0) && (iTest > s_iTestCount))
					{
						break;
					}
				}

				if(iDiffs > 0)
				{
					if(!mQuiet && mbVerbose)
					{
						printf("frm %9d  fb %2d  diffs %9d\n", iTest, iFrame, iDiffs);
					}
					if(mpLogFile != NULL)
					{
						fprintf(mpLogFile, "frm %9d  fb %2d  diffs %9d\n", iTest, iFrame, iDiffs);
						fflush(mpLogFile);
					}
				}

				iTest++;
				if(bError)
				{
					iError++;
					if(mpLogFile != NULL)
					{
						fflush(mpLogFile);
					}
				}

				if(!mQuiet)
				{
					printf ("frames/errors %d/%d\r", iTest, iError);
					fflush(stdout);
					bTestFormat = true;
				}
			}
			else
			{
				mDevice.WaitForInputVerticalInterrupt(mVideoChannel);
			}
		}	//	while loop

		mDevice.AutoCirculateStop(mVideoChannel);
		mDevice.UnsubscribeInputVerticalEvent(mVideoChannel);

		if(!mQuiet && bTestFormat)
		{
			printf("\n");
		}

		if(mpLogFile != NULL)
		{
			fprintf(mpLogFile, "\ntested %d frames\n", iTest);
			fclose(mpLogFile);
			mpLogFile = NULL;
		}

		if(pVidSrcBuffer != NULL)
		{
			delete [] pVidSrcBuffer;
		}
		if(pVidRefBuffer != NULL)
		{
			delete [] pVidRefBuffer;
		}

		if(mbTestError)
		{
			return iError;
		}
		return 0;
	}
	catch(...)
	{
	}

	return -1;
}
