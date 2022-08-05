/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2enumerateboards/main.cpp
	@brief		Demonstration application to enumerate the AJA devices for the host system.
				Shows two ways of dynamically getting a device's features.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
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

#define fCLK(__x__,__w__,__p__) std::dec << std::fixed << std::setw(__w__) << std::setprecision(__p__) << (__x__)
static inline string NumStr(const size_t num)	{ostringstream oss;	oss << num;  return oss.str();}
static string NiceNum(const size_t num, const char delim = ',')
{
	string result, s(NumStr(num));
	while (s.length() > 3)
	{
		if (result.empty())
			result = s.substr(s.length()-3, 3);
		else
			result = s.substr(s.length()-3, 3) + delim + result;
		s.erase(s.length()-3, 3);
	}
	if (result.empty())
		result = s;
	else
		result = s + delim + result;
	return result;
}

//	Main Program

int main (int argc, const char ** argv)
{
	(void) argc;
	(void) argv;
	AJADebug::Open();

	typedef vector<NTV2FrameRate>	FrameRates;
	typedef vector<NTV2AudioRate>	AudioRates;
	typedef vector<size_t>			NumChannels;
	static const FrameRates	FRs = {	NTV2_FRAMERATE_1500, NTV2_FRAMERATE_2398, NTV2_FRAMERATE_2400, NTV2_FRAMERATE_2500,
									NTV2_FRAMERATE_2997, NTV2_FRAMERATE_3000, NTV2_FRAMERATE_4795, NTV2_FRAMERATE_4800,
									NTV2_FRAMERATE_5000, NTV2_FRAMERATE_5994, NTV2_FRAMERATE_6000 };
	static const AudioRates ARs = {	NTV2_AUDIO_48K, NTV2_AUDIO_96K, NTV2_AUDIO_192K };
	static const NumChannels NumChls = { 8, 16 };
	static const uint64_t KiloSamplesPerSec [] = {48, 96, 192, 0};

	static const size_t MaxBufferBytes(65280 * 4 * 16);	//	Just under 4MB:	65280 [samples] * 4 [bytes/sample] * 16 [channels]
	static const uint64_t KiloClockTicksPerSec(48);

	//	Permute frame rate, audio rate, and number of audio channels.
	//	For each frame rate...
	for (size_t FRndx(0);  FRndx < FRs.size();  FRndx++)
	{
		const NTV2FrameRate frameRate(FRs.at(FRndx));
		const double frameTime(::GetFrameTime(frameRate));

		//	For each audio rate...
		for (size_t ARndx(0);  ARndx < ARs.size();  ARndx++)
		{
			const NTV2AudioRate audioRate(ARs.at(ARndx));
			const double samplesPerSec(::GetAudioSamplesPerSecond(audioRate));

			//	For 8 or 16 audio channels...
			for (size_t NCndx(0);  NCndx < NumChls.size();  NCndx++)
			{
				const size_t numChannels (NumChls.at(NCndx)), MaxBufferSamples(MaxBufferBytes / 4 / numChannels);
				const double maxBuffSecs (double(MaxBufferSamples) / samplesPerSec);
				const size_t maxWholeFrames (size_t(maxBuffSecs / frameTime));
				const bool MaxBufferSamplesHasRemainder(MaxBufferBytes % (4 * numChannels));
				assert(!MaxBufferSamplesHasRemainder);

				cout << endl << endl
					<< numChannels << "-channel   " << ::NTV2AudioRateToString(audioRate,true) << "   " << ::NTV2FrameRateToString(frameRate,true) << " fps   " << string(100, '=') << endl
					<< "BUFFER MAX: " << NiceNum(MaxBufferBytes) << " bytes   " << NiceNum(MaxBufferSamples) << " samples   "
						<< maxBuffSecs << " secs   " << NiceNum(size_t(maxBuffSecs * 48000.0)) << " ticks   " << maxWholeFrames << " frames   " << (48000.0 * frameTime) << " ticks/frame" << endl
					<< setw(8) << "Frame" << setw(12) << "Samples" << setw(14) << "NumAudBytes"
					<< setw(14) << "AbsByteOff" << setw(14) << "AbsSampCnt" << setw(8) << "AbsClk"
					<< setw(14) << "AbsTicks" << setw(14) << "StartByteOff" << setw(14) << "EndByteOff" << endl;

				double absPlayClock(0.0);
				uint64_t absPlayByteOffset(0), absPlaySamples(0), absPlayTicks(0);

				//	For the first N frames...
				for (size_t frm(0);  frm < (maxWholeFrames*2+5);  frm++)
				{
					const size_t numSamples (size_t(::GetAudioSamplesPerFrame(frameRate, audioRate, frm)));
					const size_t numAudioBytes (numSamples * 4 * numChannels);
					const size_t lastByteOffset (absPlayByteOffset % MaxBufferBytes);
					const size_t endByteOffset (lastByteOffset + numAudioBytes);
					const bool willWrap (endByteOffset > MaxBufferBytes);

					cout	<< setw(8) << frm << setw(12) << NiceNum(numSamples) << setw(14) << NiceNum(numAudioBytes)
							<< setw(14) << NiceNum(absPlayByteOffset) << setw(14) << NiceNum(absPlaySamples) << fCLK(absPlayClock,8,3)
							<< setw(14) << NiceNum(absPlayTicks) << setw(14) << NiceNum(lastByteOffset) << setw(14) << NiceNum(endByteOffset);
					if (willWrap)
					{	const size_t len1(MaxBufferBytes-lastByteOffset), len2(endByteOffset-MaxBufferBytes);
						cout	<< "    Wrap Segment Lengths:  " << NiceNum(len1) << "  " << NiceNum(len2);
					}
					cout << endl;

					//	Advance the offsets and clock...
					absPlayByteOffset += numAudioBytes;
					absPlaySamples += numSamples;
					absPlayClock = double(absPlaySamples)/samplesPerSec;
					absPlayTicks = absPlaySamples * KiloClockTicksPerSec / KiloSamplesPerSec[audioRate];
				}	//	for each frame
			}	//	for 8 or 16 channel audio
		}	//	for each audio rate
	}	//	for each frame rate
	return 0;


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
