/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/testaux/testaux.cpp
	@brief		Implements 'testaux' command.
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
#include "ntv2testpatterngen.h"
#include "ntv2utils.h"
#include "ajabase/common/options_popt.h"

using namespace std;

#ifdef MSWindows
#pragma warning(disable : 4996)
#endif

#ifdef AJALinux
#include "ntv2linuxpublicinterface.h"
#endif

static int s_iTestCount = 0;

static NTV2Channel channelByIndex[] = {
	NTV2_CHANNEL_INVALID,	//	Never indexed by zero
	NTV2_CHANNEL1, NTV2_CHANNEL2, NTV2_CHANNEL3, NTV2_CHANNEL4,
	NTV2_CHANNEL5, NTV2_CHANNEL6, NTV2_CHANNEL7, NTV2_CHANNEL8
};

// spd infoframe
static uint8_t auxSPDf0[NTV2_HDMIAuxDataSize] {
	0x80, 0x83, 0x01, 0x19, 0xd9, 0x41, 0x4a, 0x41,
	0x20, 0x41, 0x55, 0x58, 0x20, 0x49, 0x6e, 0x66,
	0x6f, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x20, 0x30,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x09, 0x00, 0x00
};

static uint8_t auxSPDf1[NTV2_HDMIAuxDataSize] {
	0x80, 0x83, 0x01, 0x19, 0xd8, 0x41, 0x4a, 0x41,
	0x20, 0x41, 0x55, 0x58, 0x20, 0x49, 0x6e, 0x66,
	0x6f, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x20, 0x31,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x09, 0x00, 0x00
};

static uint8_t auxDisable[NTV2_HDMIAuxDataSize] {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void SignalHandler(int signal)
{
	(void) signal;
	s_iTestCount = (-1);
}

int main(int argc, const char ** argv)
{
	bool parseError	= false;

	char* pFrameIndex = NULL;
	char* pDeviceSpec = NULL;

	int deviceIndex = 0;
	int outChannel = 1;
	int firstFrameIndex = 0;
	int lastFrameIndex = 7;

	NTV2Channel videoChannel = NTV2_CHANNEL1;
	NTV2TestPatternSelect testPattern = NTV2_TestPatt_MultiPattern;

	const struct poptOption userOptionsTable[] =
	{
        { "board", 'b', POPT_ARG_STRING, &pDeviceSpec, 0, "Which device to use", "index#" },
        { "channel", 'c', POPT_ARG_INT, &outChannel, 0, "Which video channel to use", NULL },
		{ "frameIndex", 'f', POPT_ARG_STRING, &pFrameIndex, 0, "F/L  F = first frame index L = last frame index", NULL },
		{ "testCount", 't', POPT_ARG_INT, &s_iTestCount, 0, "Number of frames to process", NULL },
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	poptContext		optionsContext;
	optionsContext = ::poptGetContext(NULL, argc, argv, userOptionsTable, 0);

	int rc;
	while ((rc = ::poptGetNextOpt(optionsContext)) >= 0)
	{
	}
	::poptFreeContext(optionsContext);

	const std::string deviceSpec(pDeviceSpec ? pDeviceSpec : "");
	if (!deviceSpec.empty())
	{
		int tempData0 = 0;
		ULWord tempDataCount = 0;
		tempDataCount = sscanf(deviceSpec.c_str(), "%d", &tempData0);
		if (tempDataCount == 1)
		{
			deviceIndex = (ULWord)tempData0;
		}
		else
		{
			fprintf(stderr, "## ERROR:  Missing device index\n");
			parseError = true;
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
			firstFrameIndex = (ULWord)tempData0;
			lastFrameIndex = (ULWord)tempData1;
		}
		else
		{
			fprintf(stderr, "## ERROR:  Missing frame index\n");
			parseError = true;
		}
	}

	try
	{
		signal(SIGINT, SignalHandler);

		if(parseError)
		{
			throw 0;
		}

		NTV2DeviceInfo boardInfo;
		CNTV2DeviceScanner ntv2BoardScan;
		if(ntv2BoardScan.GetNumDevices() <= (ULWord)deviceIndex)
		{
			fprintf (stderr, "## ERROR:  Opening device %d failed\n", deviceIndex);
			throw 0;
		}
		boardInfo = ntv2BoardScan.GetDeviceInfoList()[deviceIndex];

		CNTV2Card device(boardInfo.deviceIndex);

		// find the board
		if(device.IsOpen() == false)
		{
			fprintf (stderr, "## ERROR:  Opening device %d failed\n", deviceIndex);
			throw 0;
		}
		NTV2DeviceID deviceID = device.GetDeviceID();

		device.SetEveryFrameServices(NTV2_OEM_TASKS);

		int numFrameStores = ::NTV2DeviceGetNumFrameStores(deviceID);

		if (outChannel < 1)
			outChannel = 1;
		if (outChannel > numFrameStores)
			outChannel = numFrameStores;

		if (numFrameStores < outChannel)
		{
			fprintf (stderr, "## ERROR:  This device does not support the frame store specified\n");
			throw 0;
		}
		videoChannel = channelByIndex[outChannel];

		NTV2VideoFormat videoFormat;
		device.GetVideoFormat(videoFormat, videoChannel);

		NTV2FrameBufferFormat frameBufferFormat;
		device.GetFrameBufferFormat(videoChannel, frameBufferFormat);

		NTV2FrameGeometry frameGeomety;
		device.GetFrameGeometry(frameGeomety);

		NTV2VANCMode vancMode;
		device.GetVANCMode(vancMode, videoChannel);

		NTV2FormatDescriptor formatDesc (videoFormat, frameBufferFormat, vancMode);
		int frameCount = ::NTV2DeviceGetNumberFrameBuffers(deviceID, frameGeomety, frameBufferFormat);

		if (firstFrameIndex >= frameCount)
			firstFrameIndex = frameCount - 1;
		if (lastFrameIndex >= frameCount)
			lastFrameIndex = frameCount - 1;
		if (firstFrameIndex < 0)
			firstFrameIndex = 0;
		if (lastFrameIndex < 0)
			lastFrameIndex  = 0;

		NTV2Buffer			vidSrcBuffer (formatDesc.GetTotalBytes());
		NTV2TestPatternGen	testPatternGen;
		if (!testPatternGen.DrawTestPattern (testPattern, formatDesc, vidSrcBuffer))
		{
			cerr << "## ERROR:  DrawTestPattern failed, formatDesc: " << formatDesc << endl;
			throw 0;
		}

		AUTOCIRCULATE_TRANSFER transfer;
		AUTOCIRCULATE_STATUS acStatus;
		transfer.SetVideoBuffer(vidSrcBuffer, vidSrcBuffer.GetByteCount());

		device.SubscribeOutputVerticalEvent(videoChannel);
		device.AutoCirculateStop(videoChannel);
		device.WaitForOutputFieldID(NTV2_FIELD0, videoChannel);
		device.AutoCirculateStop(videoChannel);
		device.WaitForOutputFieldID(NTV2_FIELD0, videoChannel);

		device.AutoCirculateInitForOutput(videoChannel,
										  0,
										  NTV2_AUDIOSYSTEM_INVALID,
										  AUTOCIRCULATE_WITH_HDMIAUX,
										  1,
										  firstFrameIndex,
										  lastFrameIndex);

		device.WaitForOutputFieldID(NTV2_FIELD0, videoChannel);

		int iTest = 0;
		while ((s_iTestCount == 0) || (iTest < s_iTestCount))
		{
			device.AutoCirculateGetStatus(videoChannel, acStatus);
			if (acStatus.GetNumAvailableOutputFrames() > 1)
			{
				if ((iTest & 0x100) != 0)
					transfer.acHDMIAuxData = NTV2Buffer(auxSPDf0, 32);
				else
					transfer.acHDMIAuxData = NTV2Buffer(auxSPDf1, 32);
				if (!device.AutoCirculateTransfer(videoChannel, transfer))
					fprintf (stderr, "## ERROR:  TransferWithAutoCirculate failed!\n");

				iTest++;
				if (iTest == 3)
					device.AutoCirculateStart(videoChannel);

				cout << "frames " << DEC(iTest) << "\r" << flush;
			}
			else
			{
				device.WaitForOutputVerticalInterrupt(videoChannel);
			}
		}	//	while loop

		cout << endl;

		// disable hdr aux
		transfer.acHDMIAuxData = NTV2Buffer(auxDisable, 32);
		device.AutoCirculateTransfer(videoChannel, transfer);
		device.AutoCirculateGetStatus(videoChannel, acStatus);
		while (acStatus.GetBufferLevel() > 1)
		{
			device.WaitForOutputFieldID(NTV2_FIELD0, videoChannel);
			device.AutoCirculateGetStatus(videoChannel, acStatus);
		}

		// shutdown
		device.AutoCirculateStop(videoChannel);
		device.UnsubscribeOutputVerticalEvent(videoChannel);

		return 0;
	}
	catch(...)
	{
	}

	return -1;
}
