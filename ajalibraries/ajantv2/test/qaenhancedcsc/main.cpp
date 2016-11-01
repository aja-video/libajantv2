/**
	@file		qaenhancedcsc/main.cpp
	@brief		Everything you wanted to know about the NTV4 CSC, but were afraid to ask.
	@copyright	Copyright (C) 2012-2015 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include "ajatypes.h"
#include "ntv2devicescanner.h"
#include "ntv2enhancedcsc.h"
#include "ntv2utils.h"
#include "ajastuff/common/options_popt.h"
#include <signal.h>
#include <iostream>
#include <iomanip>
#include "ajastuff/system/systemtime.h"

using namespace std;

//	Globals
static bool		gGlobalQuit		(false);	//	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}

static void dumpState (CNTV2EnhancedCSC & mCSC)
{
	printf("  Input Pixel Format %x\n", mCSC.GetInputPixelFormat ());
	printf("  Output Pixel Format %x\n", mCSC.GetOutputPixelFormat ());
	printf("  Chroma Filter Select %x\n", mCSC.GetChromaFilterSelect ());
	printf("  Chroma Edge Control %x\n", mCSC.GetChromaEdgeControl ());
	printf("  Key Source %x\n", mCSC.GetKeySource ());
	printf("  Key Output Range %x\n", mCSC.GetKeyOutputRange ());
	printf("  Key Input Offset %x\n", mCSC.GetKeyInputOffset ());
	printf("  Key Output Offset %x\n", mCSC.GetKeyOutputOffset ());
	printf("  Key Gain %f\n\n", mCSC.GetKeyGain ());

	printf("  Matrix Pre Offset 0 %x\n",		mCSC.Matrix ().GetOffset (NTV2CSCOffsetIndex_Pre0));
	printf("  Matrix Pre Offset 1 %x\n",		mCSC.Matrix ().GetOffset (NTV2CSCOffsetIndex_Pre1));
	printf("  Matrix Pre Offset 2 %x\n\n",		mCSC.Matrix ().GetOffset (NTV2CSCOffsetIndex_Pre2));

	printf("  Matrix Coeff A0 %f\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_A0));
	printf("  Matrix Coeff A1 %f\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_A1));
	printf("  Matrix Coeff A2 %f\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_A2));
	printf("  Matrix Coeff B0 %f\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_B0));
	printf("  Matrix Coeff B1 %f\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_B1));
	printf("  Matrix Coeff B2 %f\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_B2));
	printf("  Matrix Coeff C0 %f\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_C0));
	printf("  Matrix Coeff C1 %f\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_C1));
	printf("  Matrix Coeff C2 %f\n\n",			mCSC.Matrix ().GetCoefficient (NTV2CSCCoeffIndex_C2));

	printf("  Matrix Post Offset A %x\n",		mCSC.Matrix ().GetOffset (NTV2CSCOffsetIndex_PostA));
	printf("  Matrix Post Offset B %x\n",		mCSC.Matrix ().GetOffset (NTV2CSCOffsetIndex_PostB));
	printf("  Matrix Post Offset C %x\n\n", 	mCSC.Matrix ().GetOffset (NTV2CSCOffsetIndex_PostC));
}


int main (int argc, const char ** argv)
{
	CNTV2Card				mDevice;
	char *					pDeviceSpec(NULL);							//	Which device to use
	uint32_t				channelNumber(1);							//	Number of the channel to use
	poptContext				optionsContext; 									//	Context for parsing command line arguments
	char *					pLogFileName(NULL);
	bool bVerbose(false);
	bool bQuiet(false);

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{ "channel", 'c', POPT_ARG_INT, &channelNumber, 0, "which channel to use" },
		{ "device", 'd', POPT_ARG_STRING, &pDeviceSpec, 0, "which device to use" },
		{ "log", 'o', POPT_ARG_STRING, &pLogFileName, 0, "Log to a file" },
		{ "verbose", 'v', POPT_ARG_NONE, &bVerbose, 0, "Verbose output" },
		{ "quiet", 'a', POPT_ARG_NONE, &bQuiet, 0, "Quiet output" },

		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

#if 0
	if (pWhatToList && string (pWhatToList) == "vf")
		return printVideoFormats ();
	else if (pWhatToList && string (pWhatToList) == "pf")
		return printPixelFormats ();
	else if (pWhatToList)
		{cerr << "## ERROR:  Invalid '--list' parameter '" << string (pWhatToList) << "' -- expected 'vf' or 'pf'" << endl;  return 2;}
#endif

	if (channelNumber < 1 || channelNumber > 8)
		{cerr << "## ERROR:  Invalid channel number '" << channelNumber << "' -- expected 1 thru 8" << endl;  return 2;}

	const NTV2Channel			channel		(::GetNTV2ChannelForIndex (channelNumber - 1));

	std::string mDeviceSpecifier = pDeviceSpec ? pDeviceSpec : "0";

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(mDeviceSpecifier, mDevice))
	{
		cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;
	}

	CNTV2EnhancedCSC mCSC (mDevice);

	//	Read the hardware values
	mCSC.GetFromHardware (channel);

	printf("Initial CSC state for channel %d\n", channel);
	dumpState (mCSC);

	mCSC.Matrix ().InitMatrix (NTV2_Unity_Matrix);
	mCSC.SendToHardware (channel);

	printf("After setting Matrix to Unity for channel %d\n", channel);
	mCSC.GetFromHardware (channel);
	dumpState (mCSC);

	mCSC.Matrix ().SetOffset (NTV2CSCOffsetIndex_PostC, 0x2000);
	mCSC.SendToHardware (channel);

	printf("After setting Post C offset to 0x2000 for channel %d\n", channel);
	mCSC.GetFromHardware (channel);
	dumpState (mCSC);

	mCSC.Matrix ().InitMatrix (NTV2_YCbCr_to_GBRFull_Rec709_Matrix);
	mCSC.SendToHardware (channel);

	printf("After setting Matrix to YCbCr to GBR Full 709 for channel %d\n", channel);
	mCSC.GetFromHardware (channel);
	dumpState (mCSC);

	NTV2ColorSpaceMethod cscMethod;
	cscMethod = mDevice.GetColorSpaceMethod (channel);
	printf("ColorSpaceMethod for channel %d is %d\n", channel, cscMethod);

	mDevice.SetColorSpaceMethod (NTV2_CSC_Method_Enhanced_4K, channel);
	cscMethod = mDevice.GetColorSpaceMethod (channel);
	printf("ColorSpaceMethod after setting 4K for channel %d is %d\n", channel, cscMethod);
	mCSC.GetFromHardware (channel);
	dumpState (mCSC);

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	return 0;
 
}	//	main

