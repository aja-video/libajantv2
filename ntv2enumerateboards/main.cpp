/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2enumerateboards/main.cpp
	@brief		Demonstration application to enumerate the AJA devices for the host system.
				Shows two ways of dynamically getting a device's features.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
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
#include "ntv2testpatterngen.h"
#include "ntv2bft.h"


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
		cout	<< "AJA NTV2 SDK version " << DEC(AJA_NTV2_SDK_VERSION_MAJOR) << "." << DEC(AJA_NTV2_SDK_VERSION_MINOR)
				<< "." << DEC(AJA_NTV2_SDK_VERSION_POINT) << " (" << xHEX0N(AJA_NTV2_SDK_VERSION,8)
				<< ") build " << DEC(AJA_NTV2_SDK_BUILD_NUMBER) << " built on " << AJA_NTV2_SDK_BUILD_DATETIME << endl;
		cout << "Devices supported:  " << ::NTV2GetSupportedDevices() << endl;
	#else
		cout	<< "Unknown AJA NTV2 SDK version" << endl;
	#endif

	//	Print the results of the scan...
	if (deviceCount)
	{
		cout << deviceCount << " AJA device(s) found:" << endl;

		for (uint32_t deviceIndex(0);  deviceIndex < uint32_t(deviceCount);  deviceIndex++)
		{
			//	Get detailed device information...
			CNTV2Card	ntv2Card;
			if (!CNTV2DeviceScanner::GetDeviceAtIndex(deviceIndex, ntv2Card))
				break;	//	No more devices

			const NTV2DeviceID	deviceID	(ntv2Card.GetDeviceID());

			//	Print the device number and display name...
			cout	<< "AJA device " << deviceIndex << " is called '" << ntv2Card.GetDisplayName() << "'" << endl;

			//	The device features API can tell you everything you need to know about the device...
			cout	<< endl
					<< "This device has a deviceID of " << xHEX0N(deviceID,8) << endl;

			cout	<< "This device has " << ::NTV2DeviceGetNumVideoInputs(deviceID) << " SDI Input(s)" << endl
					<< "This device has " << ::NTV2DeviceGetNumVideoOutputs(deviceID) << " SDI Output(s)" << endl;

			cout	<< "This device has " << ::NTV2DeviceGetNumHDMIVideoInputs(deviceID) << " HDMI Input(s)" << endl
					<< "This device has " << ::NTV2DeviceGetNumHDMIVideoOutputs(deviceID) << " HDMI Output(s)" << endl;

			cout	<< "This device has " << ::NTV2DeviceGetNumAnalogVideoInputs(deviceID) << " Analog Input(s)" << endl
					<< "This device has " << ::NTV2DeviceGetNumAnalogVideoOutputs(deviceID) << " Analog Output(s)" << endl;

			cout	<< "This device has " << ::NTV2DeviceGetNumUpConverters(deviceID) << " Up-Converter(s)" << endl
					<< "This device has " << ::NTV2DeviceGetNumDownConverters(deviceID) << " Down-Converter(s)" << endl;

			cout	<< "This device has " << ::NTV2DeviceGetNumEmbeddedAudioInputChannels(deviceID) << " Channel(s) of Embedded Audio Input" << endl
					<< "This device has " << ::NTV2DeviceGetNumEmbeddedAudioOutputChannels(deviceID) << " Channel(s) of Embedded Audio Output" << endl;

			//	What video formats does it support?
			NTV2VideoFormatSet	videoFormats;
			ntv2Card.GetSupportedVideoFormats(videoFormats);
			cout << endl << videoFormats << endl;

			#if defined (AJA_NTV2_SDK_VERSION_AT_LEAST)
				#if AJA_NTV2_SDK_VERSION_AT_LEAST (12, 0)
					if (::NTV2DeviceCanDoMultiFormat(deviceID))
						cout	<< "This device can handle different signal formats on each input/output" << endl;
				#endif
				#if AJA_NTV2_SDK_VERSION_AT_LEAST (11, 4)
					cout << "This device " << (::NTV2DeviceCanDoAudioDelay(deviceID) ? "can" : "cannot") << " delay audio" << endl;
				#else
					cout << "This SDK does not support the NTV2DeviceCanDoAudioDelay function call" << endl;
				#endif
			#endif	//	AJA_NTV2_SDK_VERSION_AT_LEAST

			ntv2Card.Close();
			cout << endl << endl << endl;
		}	//	for each device
	}	//	if deviceCount > 0
	else
		cout << "No AJA devices found" << endl;

	return 0;

}	//	main
