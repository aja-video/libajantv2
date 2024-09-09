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
	string serial;

			//	Print the display name...
	cout	<< "Device " << DEC(inCard.GetIndexNumber()+1) << ":" << endl

			//	Print description, device ID and serial number...
			<< "\t" << "Description: " << inCard.GetDescription() << endl
			<< "\t" << "Name: '" << inCard.GetDisplayName() << "'" << endl
			<< "\t" << "Device ID: " << xHEX0N(deviceID,8) << endl
			<< "\t" << "Serial Number: '" << (inCard.GetSerialNumberString(serial) ? serial : serial) << "'" << endl

			//	Print some additional info gleaned from the device features API...
			<< "\t" << inCard.features().GetNumVideoInputs() << " SDI Input(s)" << endl
			<< "\t" << inCard.features().GetNumVideoOutputs() << " SDI Output(s)" << endl
			<< "\t" << inCard.features().GetNumHDMIVideoInputs() << " HDMI Input(s)" << endl
			<< "\t" << inCard.features().GetNumHDMIVideoOutputs() << " HDMI Output(s)" << endl
			<< "\t" << inCard.features().GetNumAnalogVideoInputs() << " Analog Input(s)" << endl
			<< "\t" << inCard.features().GetNumAnalogVideoOutputs() << " Analog Output(s)" << endl
			<< "\t" << inCard.features().GetNumEmbeddedAudioInputChannels() << " channel(s) of Embedded Audio Input" << endl
			<< "\t" << inCard.features().GetNumEmbeddedAudioOutputChannels() << " channel(s) of Embedded Audio Output" << endl;

	//	Show its video and pixel format capabilities:
	const NTV2VideoFormatSet videoFormats(inCard.features().VideoFormats());
	const NTV2PixelFormats pixelFormats(inCard.features().PixelFormats());
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

	cout << "AJA NTV2 SDK " << ::NTV2Version() << " supports devices:  " << ::NTV2GetSupportedDevices() << endl;

	//	Discover the AJA device(s) on the local host...
	ULWord deviceCount(0);
	while (CNTV2DeviceScanner::GetDeviceAtIndex(deviceCount, device))
	{
		if (deviceCount)
			cout << endl << endl << endl;
		ShowDeviceInfo(device);
		deviceCount++;
	}	//	for each device
	if (!deviceCount)
		cout << "No AJA devices found" << endl;

	return deviceCount ? 0 : 1;

}	//	main
