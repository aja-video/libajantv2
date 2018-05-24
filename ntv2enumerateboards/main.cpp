/**
	@file		ntv2enumerateboards/main.cpp
	@brief		Demonstration application to enumerate the AJA devices for the host system.
				Shows two ways of dynamically getting a device's features.
	@copyright	Copyright (C) 2012-2015 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ajatypes.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ntv2enumerateboards.h"
#include "ajabase/system/debug.h"
#include <iostream>
#include <iomanip>


using namespace std;


//	Main Program

int main (int argc, const char ** argv)
{
	(void) argc;
	(void) argv;
	AJADebug::Open();

	//	Create an instance of a class that can scan the hardware for AJA devices...
	NTV2EnumerateDevices	deviceEnumerator;
	const size_t			deviceCount	(deviceEnumerator.GetDeviceCount ());

	#if defined (AJA_NTV2_SDK_VERSION_MAJOR)
		cout	<< "AJA NTV2 SDK version " << AJA_NTV2_SDK_VERSION_MAJOR << "." << AJA_NTV2_SDK_VERSION_MINOR << "." << AJA_NTV2_SDK_VERSION_POINT
				<< " (0x" << hex << setw (8) << setfill ('0') << AJA_NTV2_SDK_VERSION << dec << ")" << " build " << AJA_NTV2_SDK_BUILD_NUMBER
				<< " built on " << AJA_NTV2_SDK_BUILD_DATETIME << endl;
		cout << "Devices supported:  " << ::NTV2GetSupportedDevices () << endl;
	#else
		cout	<< "Unknown AJA NTV2 SDK version" << endl;
	#endif

	//	Print the results of the scan...
	if (deviceCount)
	{
		cout << deviceCount << " AJA device(s) found:" << endl;

		for (uint32_t deviceIndex = 0; ; deviceIndex++)
		{
			string deviceDescription (deviceEnumerator.GetDescription (deviceIndex));
			if (deviceDescription.empty ())
				break;

			//	Print the device number and name...
			cout << "AJA device " << deviceIndex << " is called '" << deviceDescription.c_str () << "'" << endl;

			//	Get detailed device information for current device...
			NTV2DeviceInfo	deviceInfo;
			if (deviceEnumerator.GetDeviceInfo (deviceIndex, deviceInfo))
			{
				//	'deviceInfo' contains all the info you need about the device. Some examples:
				cout	<< endl
						<< "Using NTV2DeviceInfo:" << endl
						<< "This device has a deviceID of 0x" << hex << setw (8) << setfill ('0') << deviceInfo.deviceID << dec << endl;

				//	Note:	numVidInputs and numVidOutputs implies SDI inputs & outputs...
				cout	<< "This device has " << deviceInfo.numVidInputs << " SDI Input(s)" << endl
						<< "This device has " << deviceInfo.numVidOutputs << " SDI Output(s)" << endl;

				cout	<< "This device has " << deviceInfo.numHDMIVidInputs << " HDMI Input(s)" << endl
						<< "This device has " << deviceInfo.numHDMIVidOutputs << " HDMI Output(s)" << endl;

				cout	<< "This device has " << deviceInfo.numAnlgVidInputs << " Analog Input(s)" << endl
						<< "This device has " << deviceInfo.numAnlgVidOutputs << " Analog Output(s)" << endl;

				cout	<< "This device has " << deviceInfo.numUpConverters << " Up-Converter(s)" << endl
						<< "This device has " << deviceInfo.numDownConverters << " Down-Converter(s)" << endl;

				cout	<< "This device has " << deviceInfo.numEmbeddedAudioInputChannels << " Channel(s) of Embedded Audio Input" << endl
						<< "This device has " << deviceInfo.numEmbeddedAudioOutputChannels << " Channel(s) of Embedded Audio Output" << endl;

				#if defined (AJA_NTV2_SDK_VERSION_AT_LEAST)
					#if AJA_NTV2_SDK_VERSION_AT_LEAST (12, 0)
						if (deviceInfo.multiFormat)
							cout	<< "This device can handle different signal formats on each input/output" << endl;
					#endif
				#endif	//	AJA_NTV2_SDK_VERSION_AT_LEAST

				//	Another way to get this information is to open a device and call the device features API directly
				//	based on boardID...
				cout << "Using CNTV2Card:" << endl;

				CNTV2Card	ntv2Card;
				if (ntv2Card.Open (deviceInfo.deviceIndex, false))
				{
					const NTV2DeviceID	deviceID		(ntv2Card.GetDeviceID ());
					NTV2VideoFormatSet	videoFormats;

					cout	<< "This device still has " << ::NTV2DeviceGetNumVideoInputs (deviceID) << " SDI Input(s)" << endl
							<< "This device still has " << ::NTV2DeviceGetNumVideoOutputs (deviceID) << " SDI Output(s)" << endl;

					//	What video formats does it support?
					ntv2Card.GetSupportedVideoFormats (videoFormats);
					cout << endl << videoFormats << endl;

					#if defined (AJA_NTV2_SDK_VERSION_AT_LEAST)
						#if AJA_NTV2_SDK_VERSION_AT_LEAST (11, 4)
							cout << "This device " << (::NTV2DeviceCanDoAudioDelay(deviceID) ? "can" : "cannot") << " delay audio" << endl;
						#else
							cout << "This SDK does not support the NTV2DeviceCanDoAudioDelay function call" << endl;
						#endif
					#endif	//	AJA_NTV2_SDK_VERSION_AT_LEAST
					ntv2Card.Close ();
				}	//	if Open succeeded

				cout << endl << endl << endl;

			}	//	if GetDeviceInfo succeeded
		}	//	for each device
	}	//	if deviceCount > 0
	else
		cout << "No AJA devices found" << endl;

	return 0;

}	//	main
