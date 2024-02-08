/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2enumerateboards/main.cpp
	@brief		Demonstration application to enumerate the AJA devices for the host system,
				printing information about each device.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2democommon.h"
#include "ntv2devicescanner.h"

using namespace std;


/**
	@brief	Prints a variety of information about the given device.
	@param[in]	inCard	The CNTV2Card instance for the device of interest.
**/
static int ShowDeviceInfo (CNTV2Card & inCard)
{
	//	Get detailed device information...
	const NTV2DeviceID deviceID (inCard.GetDeviceID());
	NTV2VideoFormatSet videoFormats;
	NTV2PixelFormats pixelFormats;
	string serial;

			//	Print the display name...
	cout	<< "Device '" << inCard.GetDisplayName() << "'" << endl

			//	Print device ID and serial number...
			<< "\t" << "Device ID: " << xHEX0N(deviceID,8) << endl
			<< "\t" << "Serial Number: '" << (inCard.GetSerialNumberString(serial) ? serial : serial) << "'" << endl

			//	Print additional info gleaned from the device features API...
			<< "\t" << ::NTV2DeviceGetNumVideoInputs(deviceID) << " SDI Input(s)" << endl
			<< "\t" << ::NTV2DeviceGetNumVideoOutputs(deviceID) << " SDI Output(s)" << endl
			<< "\t" << ::NTV2DeviceGetNumHDMIVideoInputs(deviceID) << " HDMI Input(s)" << endl
			<< "\t" << ::NTV2DeviceGetNumHDMIVideoOutputs(deviceID) << " HDMI Output(s)" << endl
			<< "\t" << ::NTV2DeviceGetNumAnalogVideoInputs(deviceID) << " Analog Input(s)" << endl
			<< "\t" << ::NTV2DeviceGetNumAnalogVideoOutputs(deviceID) << " Analog Output(s)" << endl
			<< "\t" << ::NTV2DeviceGetNumEmbeddedAudioInputChannels(deviceID) << " channel(s) of Embedded Audio Input" << endl
			<< "\t" << ::NTV2DeviceGetNumEmbeddedAudioOutputChannels(deviceID) << " channel(s) of Embedded Audio Output" << endl;

	//	Show its video and pixel format capabilities:
	inCard.GetSupportedVideoFormats(videoFormats);
	NTV2DeviceGetSupportedPixelFormats (deviceID, pixelFormats);
	cout	<< "\t" << videoFormats << endl
			<< "\t" << pixelFormats << endl;
	return 0;
}


//	Main Program

int main (int argc, const char ** argv)
{	(void)argc;	(void)argv;
	int		showVersion	(0);
	char *	pDeviceSpec	(AJA_NULL);
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		{"version",	  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",	AJA_NULL					},
		{"device",	'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",		"index#, serial#, or model"	},
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}
	if (!popt.otherArgs().empty())
		{cerr << "## ERROR: Unexpected argument(s): " << popt.otherArgs() << endl;  return 2;}
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	//	Device
	CNTV2Card device;
	const string deviceSpec (pDeviceSpec ? pDeviceSpec : "");
	if (!deviceSpec.empty())
	{
		if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
			return 2;
		else if (CNTV2DeviceScanner::GetFirstDeviceFromArgument(deviceSpec, device))
			return ShowDeviceInfo(device);	//	Show info for a single device
		else
			{cerr << "## ERROR: Failed to open '" << deviceSpec << "'";  return 2;}
	}	//	if -d option used

	//	Discover the AJA device(s) on the local host...
	CNTV2DeviceScanner scanner;
	const size_t deviceCount (scanner.GetNumDevices());

	cout << "AJA NTV2 SDK " << ::NTV2Version() << " supports devices:  " << ::NTV2GetSupportedDevices() << endl;

	//	Print the results of the scan...
	if (deviceCount)
	{
		cout << deviceCount << " AJA device(s) found" << endl;
		uint32_t deviceIndex(0);
		while (CNTV2DeviceScanner::GetDeviceAtIndex(deviceIndex, device))
		{
			if (deviceIndex)
				cout << endl << endl << endl;
			ShowDeviceInfo(device);
			deviceIndex++;
		}	//	for each device
	}	//	if deviceCount > 0
	else
		cout << "No AJA devices found" << endl;

	return deviceCount ? 0 : 1;

}	//	main
