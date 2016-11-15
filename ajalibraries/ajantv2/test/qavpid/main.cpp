/**
	@file		vpid_test/main.cpp
	@brief		Set up a bunch of board configuraqtions and check if the VPID is corrtct.
	@copyright	Copyright (C) 2012-2016 AJA Video Systems, Inc.  All rights reserved.
**/

//	Includes
#include <signal.h>
#include <iostream>
#include <iomanip>

#include "ajatypes.h"
#include "ntv2devicescanner.h"
#include "ntv2signalrouter.h"
#include "ntv2utils.h"

#include "ajabase/common/options_popt.h"
#include "ajabase/system/systemtime.h"

#include "testentry.h"
#include "tests.h"

using namespace std;


//	Globals

int main (int argc, const char ** argv)
{
	CNTV2Card		mDevice;
	char *			pDeviceSpec			(NULL);		//	Which device to use
	poptContext		optionsContext; 				//	Context for parsing command line arguments
	int				bVerbose			(false);
	int				bQuiet				(false);
	int				bSingleStep			(false);	//	Wait for keyboard input before next test
	int				iStartOnTest		(0);		//	Start with this test
	int				iExitOnTest			(-1);		//	End with this test
	int				iTestOnly			(-1);		//	Run only this test number
	int				testNum				(-1);		//	Which test is running
	ULWord			hwVPID				(0);		//	VPID value read from hardware

	//	Command line option descriptions:
	const struct poptOption userOptionsTable [] =
	{
		{ "quiet",		'a', POPT_ARG_NONE,		&bQuiet,		0,	"Quiet output" },
		{ "device",		'd', POPT_ARG_STRING,	&pDeviceSpec,	0,	"Which device to use" },
		{ "end",		'e', POPT_ARG_INT,		&iExitOnTest,	0,	"End after test (number)" },
		{ "one",		'o', POPT_ARG_INT,		&bSingleStep,	0,	"One test at a time" },
		{ "start",		's', POPT_ARG_INT,		&iStartOnTest,	0,	"Start with this test" },
		{ "test",		't', POPT_ARG_INT,		&iTestOnly,		0,	"Test only (number)" },
		{ "verbose",	'v', POPT_ARG_INT,		&bVerbose,		0,	"Verbose output" },

		POPT_AUTOHELP
		POPT_TABLEEND
	};

	//	Read command line arguments...
	optionsContext = ::poptGetContext (NULL, argc, argv, userOptionsTable, 0);
	::poptGetNextOpt (optionsContext);
	optionsContext = ::poptFreeContext (optionsContext);

	std::string mDeviceSpecifier = pDeviceSpec ? pDeviceSpec : "0";

	//	Open the device...
	if (!CNTV2DeviceScanner::GetFirstDeviceFromArgument(mDeviceSpecifier, mDevice))
	{
		cerr << "## ERROR:  Device '" << mDeviceSpecifier << "' not found" << endl;  return AJA_STATUS_OPEN;
	}

	//	Read all the tests
	TestVector	testList;
	FillTestList (testList);

	if (iStartOnTest > 0)
		iStartOnTest--;				//  Convert one based user input to zero based

	if (iExitOnTest > 0)
		iExitOnTest--;				//  Convert one based user input to zero based
	else
		iExitOnTest = testList.size () - 1;


	if (iTestOnly > 0)
	{
		iTestOnly--;				//	Convert one based user input to zero based
		iExitOnTest = iTestOnly;	//	Force exit at end of loop
		iStartOnTest = iTestOnly;
		testNum = iTestOnly;
		if (!bQuiet)
			cout << "Preparing to run test " << dec << (iTestOnly + 1) << " once" << endl;
	}
	else
	{
		testNum = 0;
		if (!bQuiet)
			cout << "Preparing to run " << dec << (iExitOnTest - iStartOnTest + 1) << " tests" << endl;	//	Don't count the end of list entry
	}

	for (TestVector::const_iterator iter = testList.begin (); iter < testList.end (); ++iter, ++testNum)
	{
		TestEntry	test	= *iter;
		bool		isRGB	= false;

		if (testNum < iStartOnTest)
			continue;

		if (iTestOnly >= 0)
			test = testList [iTestOnly];

		if (test.routing == MaxRouting)
		{
			cout << "All tests complete" << endl;
			break;
		}

		if (bVerbose)
			cout << "Starting test #" << dec << (testNum + 1) << endl;

		//	Clear any state that may have been set by the last test
		mDevice.SetSmpte372 (0);

		mDevice.SetSDITransmitEnable (NTV2_CHANNEL1, true);
		mDevice.SetSDITransmitEnable (NTV2_CHANNEL2, true);

		mDevice.SetSDIOutLevelAtoLevelBConversion (0, false);
		mDevice.SetSDIOutLevelAtoLevelBConversion (1, false);
		mDevice.SetSDIOutLevelAtoLevelBConversion (2, false);
		mDevice.SetSDIOutLevelAtoLevelBConversion (3, false);

		mDevice.SetTsiFrameEnable (false, NTV2_CHANNEL1);
		mDevice.SetTsiFrameEnable (false, NTV2_CHANNEL2);

		//	Setup video format
		mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL1);

		//	Setup frame buffer format
		mDevice.SetFrameBufferFormat (NTV2_CHANNEL1, test.pixelFormat);
		isRGB = NTV2_IS_FBF_RGB (test.pixelFormat);

		//	Setup routing
		CNTV2SignalRouter	router;
		router.Reset ();

		switch (test.routing)
		{
			case SingleLink270:
			case SingleLink1_5:
			case SingleLink3_0A:
				router.AddConnection (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);
				if (::IsRGBFormat (test.pixelFormat))
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, true);
				else
					mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, false);
				break;
			case DualLink1_5:
				if (isRGB)
				{
					router.AddConnection (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
					router.AddConnection (NTV2_XptSDIOut1Input,			NTV2_XptDuallinkOut1);
					router.AddConnection (NTV2_XptSDIOut2Input,			NTV2_XptDuallinkOut1DS2);
				}
				else
				{
					mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
					mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);
					router.AddConnection (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_XptSDIOut2Input, NTV2_XptFrameBuffer2YUV);
					mDevice.SetSmpte372 (1);
				}
				break;
			case SingleLink3_0B:
				mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, false);
				mDevice.SetSDIOutLevelAtoLevelBConversion (NTV2_CHANNEL1, true);
				if (isRGB)
				{
					router.AddConnection (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
					router.AddConnection (NTV2_XptSDIOut1Input,			NTV2_XptDuallinkOut1);
					router.AddConnection (NTV2_XptSDIOut1InputDS2,		NTV2_XptDuallinkOut1DS2);
				}
				else
				{
					mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
					mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);

					router.AddConnection (NTV2_XptSDIOut1Input,		NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_XptSDIOut1InputDS2,	NTV2_XptFrameBuffer2YUV);
				}
				break;
			case SingleLink3RGB_A:
				mDevice.SetSDIOutRGBLevelAConversion (NTV2_CHANNEL1, true);
				router.AddConnection (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
				router.AddConnection (NTV2_XptSDIOut1Input,			NTV2_XptDuallinkOut1);
				router.AddConnection (NTV2_XptSDIOut1InputDS2,		NTV2_XptDuallinkOut1DS2);
				break;
			case QuadLink1_5:
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL3);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, test.pixelFormat);
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL4);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, test.pixelFormat);

				if (isRGB)
				{
					cout << "RGB can't be quad 1.5 in test " << dec << (testNum + 1) << ", skipping" << endl;
					continue;
				}
				else
				{
					router.AddConnection (NTV2_XptSDIOut1Input, NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_XptSDIOut2Input, NTV2_XptFrameBuffer2YUV);
					router.AddConnection (NTV2_XptSDIOut3Input, NTV2_XptFrameBuffer3YUV);
					router.AddConnection (NTV2_XptSDIOut4Input, NTV2_XptFrameBuffer4YUV);
				}
				break;
			case DualLink3_0:
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);

				if (isRGB)
				{
					cout << "RGB can't be dual 3.0 in test " << dec << (testNum + 1) << ", skipping" << endl;
					continue;
				}
				else
				{
					router.AddConnection (NTV2_XptSDIOut1Input,		NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_XptSDIOut1InputDS2,	NTV2_XptFrameBuffer2YUV);
				}
				break;
			case QuadLink3_0A:
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL3);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, test.pixelFormat);
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL4);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, test.pixelFormat);

				if (isRGB)
				{
					router.AddConnection (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
					router.AddConnection (NTV2_XptSDIOut1Input,			NTV2_XptDuallinkOut1);
					router.AddConnection (NTV2_XptSDIOut1InputDS2,		NTV2_XptDuallinkOut1DS2);

					router.AddConnection (NTV2_XptDualLinkOut2Input,	NTV2_XptFrameBuffer2RGB);
					router.AddConnection (NTV2_XptSDIOut2Input,			NTV2_XptDuallinkOut2);
					router.AddConnection (NTV2_XptSDIOut2InputDS2,		NTV2_XptDuallinkOut2DS2);

					router.AddConnection (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer3RGB);
					router.AddConnection (NTV2_XptSDIOut3Input,			NTV2_XptDuallinkOut3);
					router.AddConnection (NTV2_XptSDIOut3InputDS2,		NTV2_XptDuallinkOut3DS2);

					router.AddConnection (NTV2_XptDualLinkOut4Input,	NTV2_XptFrameBuffer4RGB);
					router.AddConnection (NTV2_XptSDIOut4Input,			NTV2_XptDuallinkOut4);
					router.AddConnection (NTV2_XptSDIOut4InputDS2,		NTV2_XptDuallinkOut4DS2);
				}
				else
				{
					router.AddConnection (NTV2_XptSDIOut1Input,		NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_XptSDIOut2Input,		NTV2_XptFrameBuffer2YUV);
					router.AddConnection (NTV2_XptSDIOut3Input,		NTV2_XptFrameBuffer3YUV);
					router.AddConnection (NTV2_XptSDIOut4Input,		NTV2_XptFrameBuffer4YUV);
				}
				break;
			case QuadLink3_0B:
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL3);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, test.pixelFormat);
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL4);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, test.pixelFormat);

				mDevice.SetSDIOutLevelAtoLevelBConversion (0, true);
				mDevice.SetSDIOutLevelAtoLevelBConversion (1, true);
				mDevice.SetSDIOutLevelAtoLevelBConversion (2, true);
				mDevice.SetSDIOutLevelAtoLevelBConversion (3, true);

				if (isRGB)
				{
					router.AddConnection (NTV2_XptDualLinkOut1Input,	NTV2_XptFrameBuffer1RGB);
					router.AddConnection (NTV2_XptSDIOut1Input,			NTV2_XptDuallinkOut1);
					router.AddConnection (NTV2_XptSDIOut1InputDS2,		NTV2_XptDuallinkOut1DS2);

					router.AddConnection (NTV2_XptDualLinkOut2Input,	NTV2_XptFrameBuffer2RGB);
					router.AddConnection (NTV2_XptSDIOut2Input,			NTV2_XptDuallinkOut2);
					router.AddConnection (NTV2_XptSDIOut2InputDS2,		NTV2_XptDuallinkOut2DS2);

					router.AddConnection (NTV2_XptDualLinkOut3Input,	NTV2_XptFrameBuffer3RGB);
					router.AddConnection (NTV2_XptSDIOut3Input,			NTV2_XptDuallinkOut3);
					router.AddConnection (NTV2_XptSDIOut3InputDS2,		NTV2_XptDuallinkOut3DS2);

					router.AddConnection (NTV2_XptDualLinkOut4Input,	NTV2_XptFrameBuffer4RGB);
					router.AddConnection (NTV2_XptSDIOut4Input,			NTV2_XptDuallinkOut4);
					router.AddConnection (NTV2_XptSDIOut4InputDS2,		NTV2_XptDuallinkOut4DS2);
				}
				else
				{
					router.AddConnection (NTV2_XptSDIOut1Input,		NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_XptSDIOut2Input,		NTV2_XptFrameBuffer2YUV);
					router.AddConnection (NTV2_XptSDIOut3Input,		NTV2_XptFrameBuffer3YUV);
					router.AddConnection (NTV2_XptSDIOut4Input,		NTV2_XptFrameBuffer4YUV);
				}
				break;
			case QuadLink1_5_TSI:
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);

				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL1);
				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL2);

				if (isRGB)
				{
					cout << "RGB can't be quad 1.5 TSI in test " << dec << (testNum + 1) << ", skipping" << endl;
					continue;
				}
				else
				{
					router.AddConnection (NTV2_Xpt425Mux1AInput,	NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_Xpt425Mux1BInput,	NTV2_XptFrameBuffer1_425YUV);
					router.AddConnection (NTV2_Xpt425Mux2AInput,	NTV2_XptFrameBuffer2YUV);
					router.AddConnection (NTV2_Xpt425Mux2BInput,	NTV2_XptFrameBuffer2_425YUV);

					router.AddConnection (NTV2_XptSDIOut1Input,		NTV2_Xpt425Mux1AYUV);
					router.AddConnection (NTV2_XptSDIOut2Input,		NTV2_Xpt425Mux1BYUV);
					router.AddConnection (NTV2_XptSDIOut3Input,		NTV2_Xpt425Mux2AYUV);
					router.AddConnection (NTV2_XptSDIOut4Input,		NTV2_Xpt425Mux2BYUV);
				}
				break;
			case DualLink3_0_TSI:
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);

				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL1);
				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL2);

				if (isRGB)
				{
					cout << "RGB can't be dual link 3.0 TSI in test " << dec << (testNum + 1) << ", skipping" << endl;
					continue;
				}
				else
				{
					router.AddConnection (NTV2_Xpt425Mux1AInput,	NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_Xpt425Mux1BInput,	NTV2_XptFrameBuffer1_425YUV);
					router.AddConnection (NTV2_Xpt425Mux2AInput,	NTV2_XptFrameBuffer2YUV);
					router.AddConnection (NTV2_Xpt425Mux2BInput,	NTV2_XptFrameBuffer2_425YUV);

					router.AddConnection (NTV2_XptSDIOut1Input,		NTV2_Xpt425Mux1AYUV);
					router.AddConnection (NTV2_XptSDIOut1InputDS2,	NTV2_Xpt425Mux1BYUV);
					router.AddConnection (NTV2_XptSDIOut2Input,		NTV2_Xpt425Mux2AYUV);
					router.AddConnection (NTV2_XptSDIOut2InputDS2,	NTV2_Xpt425Mux2BYUV);
				}
				break;
			case QuadLink3_0A_TSI:
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);

				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL1);
				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL2);

				if (isRGB)
				{
					router.AddConnection (NTV2_Xpt425Mux1AInput,		NTV2_XptFrameBuffer1RGB);
					router.AddConnection (NTV2_Xpt425Mux1BInput,		NTV2_XptFrameBuffer1_425RGB);
					router.AddConnection (NTV2_Xpt425Mux2AInput,		NTV2_XptFrameBuffer2RGB);
					router.AddConnection (NTV2_Xpt425Mux2BInput,		NTV2_XptFrameBuffer2_425RGB);

					router.AddConnection (NTV2_XptDualLinkOut1Input,	NTV2_Xpt425Mux1ARGB);
					router.AddConnection (NTV2_XptDualLinkOut2Input,	NTV2_Xpt425Mux1BRGB);
					router.AddConnection (NTV2_XptDualLinkOut3Input,	NTV2_Xpt425Mux2ARGB);
					router.AddConnection (NTV2_XptDualLinkOut4Input,	NTV2_Xpt425Mux2BRGB);

					router.AddConnection (NTV2_XptSDIOut1Input,			NTV2_XptDuallinkOut1);
					router.AddConnection (NTV2_XptSDIOut1InputDS2,		NTV2_XptDuallinkOut1DS2);
					router.AddConnection (NTV2_XptSDIOut2Input,			NTV2_XptDuallinkOut2);
					router.AddConnection (NTV2_XptSDIOut2InputDS2,		NTV2_XptDuallinkOut2DS2);
					router.AddConnection (NTV2_XptSDIOut3Input,			NTV2_XptDuallinkOut3);
					router.AddConnection (NTV2_XptSDIOut3InputDS2,		NTV2_XptDuallinkOut3DS2);
					router.AddConnection (NTV2_XptSDIOut4Input,			NTV2_XptDuallinkOut4);
					router.AddConnection (NTV2_XptSDIOut4InputDS2,		NTV2_XptDuallinkOut4DS2);
				}
				else
				{
					router.AddConnection (NTV2_Xpt425Mux1AInput,	NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_Xpt425Mux1BInput,	NTV2_XptFrameBuffer1_425YUV);
					router.AddConnection (NTV2_Xpt425Mux2AInput,	NTV2_XptFrameBuffer2YUV);
					router.AddConnection (NTV2_Xpt425Mux2BInput,	NTV2_XptFrameBuffer2_425YUV);

					router.AddConnection (NTV2_XptSDIOut1Input,		NTV2_Xpt425Mux1AYUV);
					router.AddConnection (NTV2_XptSDIOut2Input,		NTV2_Xpt425Mux1BYUV);
					router.AddConnection (NTV2_XptSDIOut3Input,		NTV2_Xpt425Mux2AYUV);
					router.AddConnection (NTV2_XptSDIOut4Input,		NTV2_Xpt425Mux2BYUV);
				}
				break;
			case QuadLink3_0B_TSI:
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL2);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL2, test.pixelFormat);
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL3);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL3, test.pixelFormat);
				mDevice.SetVideoFormat (test.videoFormat, false, false, NTV2_CHANNEL4);
				mDevice.SetFrameBufferFormat (NTV2_CHANNEL4, test.pixelFormat);

				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL1);
				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL2);
				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL3);
				mDevice.SetTsiFrameEnable (true, NTV2_CHANNEL4);

				mDevice.SetSDIOutLevelAtoLevelBConversion (0, true);
				mDevice.SetSDIOutLevelAtoLevelBConversion (1, true);
				mDevice.SetSDIOutLevelAtoLevelBConversion (2, true);
				mDevice.SetSDIOutLevelAtoLevelBConversion (3, true);

				if (isRGB)
				{
					cout << "RGB can't be quad link 3.0 Level B TSI in test " << dec << (testNum + 1) << ", skipping" << endl;
					continue;
				}
				else
				{
					router.AddConnection (NTV2_Xpt425Mux1AInput,	NTV2_XptFrameBuffer1YUV);
					router.AddConnection (NTV2_Xpt425Mux1BInput,	NTV2_XptFrameBuffer1_425YUV);
					router.AddConnection (NTV2_Xpt425Mux2AInput,	NTV2_XptFrameBuffer2YUV);
					router.AddConnection (NTV2_Xpt425Mux2BInput,	NTV2_XptFrameBuffer2_425YUV);
					router.AddConnection (NTV2_Xpt425Mux3AInput,	NTV2_XptFrameBuffer3YUV);
					router.AddConnection (NTV2_Xpt425Mux3BInput,	NTV2_XptFrameBuffer3_425YUV);
					router.AddConnection (NTV2_Xpt425Mux4AInput,	NTV2_XptFrameBuffer4YUV);
					router.AddConnection (NTV2_Xpt425Mux4BInput,	NTV2_XptFrameBuffer4_425YUV);

					router.AddConnection (NTV2_XptSDIOut1Input,		NTV2_Xpt425Mux1AYUV);
					router.AddConnection (NTV2_XptSDIOut1InputDS2,	NTV2_Xpt425Mux1BYUV);
					router.AddConnection (NTV2_XptSDIOut2Input,		NTV2_Xpt425Mux2AYUV);
					router.AddConnection (NTV2_XptSDIOut2InputDS2,	NTV2_Xpt425Mux2BYUV);
					router.AddConnection (NTV2_XptSDIOut3Input,		NTV2_Xpt425Mux3AYUV);
					router.AddConnection (NTV2_XptSDIOut3InputDS2,	NTV2_Xpt425Mux3BYUV);
					router.AddConnection (NTV2_XptSDIOut4Input,		NTV2_Xpt425Mux4AYUV);
					router.AddConnection (NTV2_XptSDIOut4InputDS2,	NTV2_Xpt425Mux4BYUV);
				}
				break;
			default:
				cout << "Unknown routing " << dec << test.routing << ", skipping test " << dec << (testNum + 1) << endl;
				continue;
				break;
		}

		mDevice.ApplySignalRoute (router, true);

		//	Pause for effect (about 10 VBIs)
		mDevice.WaitForOutputVerticalInterrupt (NTV2_CHANNEL1, 10);

		//	Read back VPID
		mDevice.ReadRegister (kRegSDIOut1VPIDA, &hwVPID);
		if (test.vpidLink1_DS1 != hwVPID)
		{
			cout << "Error test " << dec << (testNum + 1) << ", VPID Link 1 DS 1 was " << hex << hwVPID;
			cout << ", expected " << hex << test.vpidLink1_DS1 << endl;
		}

		mDevice.ReadRegister (kRegSDIOut1VPIDB, &hwVPID);
		if (test.vpidLink1_DS2 != hwVPID)
		{
			cout << "Error test " << dec << (testNum + 1) << ", VPID Link 1 DS 2 was " << hex << hwVPID;
			cout << ", expected " << hex << test.vpidLink1_DS2 << endl;
		}

		mDevice.ReadRegister (kRegSDIOut2VPIDA, &hwVPID);
		if (test.vpidLink2_DS1 != hwVPID)
		{
			cout << "Error test " << dec << (testNum + 1) << ", VPID Link 2 DS 1 was " << hex << hwVPID;
			cout << ", expected " << hex << test.vpidLink2_DS1 << endl;
		}

		mDevice.ReadRegister (kRegSDIOut2VPIDB, &hwVPID);
		if (test.vpidLink2_DS2 != hwVPID)
		{
			cout << "Error test " << dec << (testNum + 1) << ", VPID Link 2 DS 2 was " << hex << hwVPID;
			cout << ", expected " << hex << test.vpidLink2_DS2 << endl;
		}

		mDevice.ReadRegister (kRegSDIOut3VPIDA, &hwVPID);
		if (test.vpidLink3_DS1 != hwVPID)
		{
			cout << "Error test " << dec << (testNum + 1) << ", VPID Link 3 DS 1 was " << hex << hwVPID;
			cout << ", expected " << hex << test.vpidLink3_DS1 << endl;
		}

		mDevice.ReadRegister (kRegSDIOut4VPIDA, &hwVPID);
		if (test.vpidLink4_DS1 != hwVPID)
		{
			cout << "Error test " << dec << (testNum + 1) << ", VPID Link 4 DS 1 was " << hex << hwVPID;
			cout << ", expected " << hex << test.vpidLink4_DS1 << endl;
		}

		if (testNum == iExitOnTest)
		{
			cout << "Exiting after running test " << dec << (testNum + 1) << endl;
			break;
		}

		if (bSingleStep)
			getchar ();
	}

	return 0;
}	//	main

