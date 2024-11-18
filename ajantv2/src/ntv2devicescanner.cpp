/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2devicescanner.cpp
	@brief		Implementation of CNTV2DeviceScanner class.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ajabase/common/common.h"
#include "ajabase/system/lock.h"
#include <sstream>
#include "ajabase/system/info.h"
#include "ajabase/common/json.hpp"
#include <fstream>
#include "ajabase/system/file_io.h"

using namespace std;
using json = nlohmann::json;


#if defined(NTV2_DEPRECATE_17_1)
	//	Abbreviated device info struct
	typedef struct NTV2DeviceInfo
	{
		NTV2DeviceID	deviceID;
		string			serialNumber;
		string			deviceIdentifier;
	} NTV2DeviceInfo;

	typedef vector <NTV2DeviceInfo>		NTV2DeviceInfoList;
	typedef NTV2DeviceInfoList::const_iterator	NTV2DeviceInfoListConstIter;
#else
	bool CNTV2DeviceScanner::IsHexDigit (const char inChr)
	{	static const string sHexDigits("0123456789ABCDEFabcdef");
		return sHexDigits.find(inChr) != string::npos;
	}

	bool CNTV2DeviceScanner::IsDecimalDigit (const char inChr)
	{	static const string sDecDigits("0123456789");
		return sDecDigits.find(inChr) != string::npos;
	}

	bool CNTV2DeviceScanner::IsAlphaNumeric (const char inChr)
	{	static const string sLegalChars("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
		return sLegalChars.find(inChr) != string::npos;
	}

	bool CNTV2DeviceScanner::IsLegalDecimalNumber (const string & inStr, const size_t inMaxLength)
	{
		return aja::is_legal_decimal_number(inStr, inMaxLength);
	}

	uint64_t CNTV2DeviceScanner::IsLegalHexSerialNumber (const string & inStr)	//	0x3236333331375458
	{
		return aja::is_legal_hex_serial_number(inStr);
	}

	bool CNTV2DeviceScanner::IsAlphaNumeric (const string & inStr)
	{
		for (size_t ndx(0);  ndx < inStr.size();  ndx++)
			if (!aja::is_alpha_numeric(inStr.at(ndx)))
				return false;
		return true;
	}
#endif	//	!defined(NTV2_DEPRECATE_17_1)

bool CNTV2DeviceScanner::IsLegalSerialNumber (const string & inStr)
{
	if (inStr.length() != 8  &&  inStr.length() != 9)
		return false;
	return aja::is_alpha_numeric(inStr);
}

static NTV2DeviceInfoList	sDevInfoList;
static AJALock				sDevInfoListLock;

size_t CNTV2DeviceScanner::GetNumDevices (void)
{
	AJAAutoLock tmpLock(&sDevInfoListLock);
	return sDevInfoList.size();
}

#if defined(NTV2_DEPRECATE_17_1)
	void ScanHardware (void)
	{
		AJAAutoLock tmpLock(&sDevInfoListLock);
		sDevInfoList.clear();
		UWord ndx(0);
		do
		{
			CNTV2Card tmpDev(ndx);
			if (!tmpDev.IsOpen())
				break;
			NTV2DeviceInfo info;
			info.deviceID = tmpDev.GetDeviceID();
			tmpDev.GetSerialNumberString(info.serialNumber);
			info.deviceIdentifier = tmpDev.GetDisplayName();
			sDevInfoList.push_back(info);
			ndx++;
		} while (ndx < 16);
	}
#else	//	!defined(NTV2_DEPRECATE_17_1)
CNTV2DeviceScanner::CNTV2DeviceScanner (const bool inScanNow)
{
	if (inScanNow)
		ScanHardware();
}

	#if !defined(NTV2_DEPRECATE_16_3)
		CNTV2DeviceScanner::CNTV2DeviceScanner (bool inScanNow, UWord inDeviceMask)
		{
			(void)inDeviceMask;
			if (inScanNow)
				ScanHardware();
		}
	#endif	//	!defined(NTV2_DEPRECATE_16_3)

NTV2DeviceInfoList	GetDeviceInfoList (void)
{
	AJAAutoLock tmpLock(&sDevInfoListLock);
	return sDevInfoList;
}


void CNTV2DeviceScanner::ScanHardware (void)
{
	AJAAutoLock tmpLock(&sDevInfoListLock);
	sDevInfoList.clear();

	for (UWord boardNum(0);   ;   boardNum++)
	{
		CNTV2Card tmpDev(boardNum);
		if (!tmpDev.IsOpen())
			break;
		const NTV2DeviceID	deviceID (tmpDev.GetDeviceID());

		if (deviceID != DEVICE_ID_NOTFOUND)
		{
			ostringstream	oss;
			NTV2DeviceInfo	info;
			info.deviceIndex		= boardNum;
			info.deviceID			= deviceID;
			tmpDev.GetSerialNumberString(info.serialNumber);

			oss << ::NTV2DeviceIDToString (deviceID, tmpDev.IsSupported(kDeviceHasMicrophoneInput)) << " - " << boardNum;

			info.deviceIdentifier = oss.str();
			SetDeviceAttributes(info, tmpDev);
			SetAudioAttributes(info, tmpDev);
			sDevInfoList.push_back(info);
		}
		tmpDev.Close();
	}	//	boardNum loop

	GetVirtualDeviceList(sDevInfoList);
}	//	ScanHardware

bool CNTV2DeviceScanner::DeviceIDPresent (const NTV2DeviceID inDeviceID, const bool inRescan)
{
	AJAAutoLock tmpLock(&sDevInfoListLock);
	if (inRescan)
		ScanHardware();

	for (NTV2DeviceInfoListConstIter iter(sDevInfoList.begin());  iter != sDevInfoList.end();  ++iter)
		if (iter->deviceID == inDeviceID)
			return true;	//	Found!
	return false;	//	Not found

}	//	DeviceIDPresent


bool CNTV2DeviceScanner::GetDeviceInfo (const ULWord inDeviceIndexNumber, NTV2DeviceInfo & outDeviceInfo, const bool inRescan)
{
	AJAAutoLock tmpLock(&sDevInfoListLock);
	if (inRescan)
		ScanHardware();

	for (NTV2DeviceInfoListConstIter iter(sDevInfoList.begin()); iter != sDevInfoList.end(); ++iter)
		if (iter->deviceIndex == inDeviceIndexNumber)
		{
			outDeviceInfo = *iter;
			return true;	//	Found!
		}
	return false;	//	No devices with this index number

}	//	GetDeviceInfo
#endif	//	!defined(NTV2_DEPRECATE_17_1)

bool CNTV2DeviceScanner::GetDeviceAtIndex (const ULWord inDeviceIndexNumber, CNTV2Card & outDevice)
{
	outDevice.Close();
	AJAAutoLock tmpLock(&sDevInfoListLock);
	ScanHardware();
	for (NTV2DeviceInfoListConstIter iter(sDevInfoList.begin()); iter != sDevInfoList.end(); ++iter)
		if (iter->deviceIndex == inDeviceIndexNumber)
		{
			if (iter->isVirtualDevice)
				return outDevice.Open(iter->vdevUrl);
			else
				return outDevice.Open(UWord(inDeviceIndexNumber));
		}
	return false;	//	No devices with this index number

}	//	GetDeviceAtIndex


bool CNTV2DeviceScanner::GetFirstDeviceWithID (const NTV2DeviceID inDeviceID, CNTV2Card & outDevice)
{
	outDevice.Close();
	AJAAutoLock tmpLock(&sDevInfoListLock);
	ScanHardware();
	for (size_t ndx(0);  ndx < sDevInfoList.size();  ndx++)
		if (sDevInfoList.at(ndx).deviceID == inDeviceID)
		{
			if (sDevInfoList.at(ndx).isVirtualDevice)
				return outDevice.Open(sDevInfoList.at(ndx).vdevUrl);
			else
				return outDevice.Open(UWord(ndx));
		}
	return false;	//	Not found

}	//	GetFirstDeviceWithID


bool CNTV2DeviceScanner::GetFirstDeviceWithName (const string & inNameSubString, CNTV2Card & outDevice)
{
	outDevice.Close();
	AJAAutoLock tmpLock(&sDevInfoListLock);
	ScanHardware();
	string	nameSubString(inNameSubString);  aja::lower(nameSubString);
	for (size_t ndx(0);  ndx < sDevInfoList.size();  ndx++)
	{
		string deviceName(sDevInfoList.at(ndx).deviceIdentifier);  aja::lower(deviceName);
		if (deviceName.find(nameSubString) != string::npos)
		{
			if (sDevInfoList.at(ndx).isVirtualDevice)
				return outDevice.Open(sDevInfoList.at(ndx).vdevUrl);
			else
				return outDevice.Open(UWord(ndx));
		}
	}
	if (nameSubString == "io4kplus")
	{	//	Io4K+ == DNXIV...
		nameSubString = "avid dnxiv";
		for (size_t ndx(0);  ndx < sDevInfoList.size();  ndx++)
		{
			string deviceName(sDevInfoList.at(ndx).deviceIdentifier);  aja::lower(deviceName);
			if (deviceName.find(nameSubString) != string::npos)
			{
				if (sDevInfoList.at(ndx).isVirtualDevice)
					return outDevice.Open(sDevInfoList.at(ndx).vdevUrl);
				else
					return outDevice.Open(UWord(ndx));
			}
		}
	}
	return false;	//	Not found

}	//	GetFirstDeviceWithName


bool CNTV2DeviceScanner::GetFirstDeviceWithSerial (const string & inSerialStr, CNTV2Card & outDevice)
{
	outDevice.Close();
	AJAAutoLock tmpLock(&sDevInfoListLock);
	ScanHardware();
	string searchSerialStr(inSerialStr);  aja::lower(searchSerialStr);
	for (size_t ndx(0);  ndx < sDevInfoList.size();  ndx++)
	{
		string ser(sDevInfoList.at(ndx).serialNumber);  aja::lower(ser);
		if (ser.find(searchSerialStr) != string::npos)
		{
			if (sDevInfoList.at(ndx).isVirtualDevice)
				return outDevice.Open(sDevInfoList.at(ndx).vdevUrl);
			else
				return outDevice.Open(UWord(ndx));
		}
	}
	return false;
}


#if !defined(NTV2_DEPRECATE_17_5)
bool CNTV2DeviceScanner::GetDeviceWithSerial (const uint64_t inSerialNumber, CNTV2Card & outDevice)
{
	const string serNumStr(::SerialNum64ToString(inSerialNumber));
	return GetDeviceWithSerial(serNumStr, outDevice);
}
#endif	//	!defined(NTV2_DEPRECATE_17_5)

bool CNTV2DeviceScanner::GetDeviceWithSerial (const string & inSerialNumber, CNTV2Card & outDevice)
{
	outDevice.Close();
	AJAAutoLock tmpLock(&sDevInfoListLock);
	ScanHardware();
	for (size_t ndx(0);  ndx < sDevInfoList.size();  ndx++)
		if (sDevInfoList.at(ndx).serialNumber == inSerialNumber)
		{
			if (sDevInfoList.at(ndx).isVirtualDevice)
				return outDevice.Open(sDevInfoList.at(ndx).vdevUrl);
			else
				return outDevice.Open(UWord(ndx));
		}
	return false;
}


bool CNTV2DeviceScanner::GetFirstDeviceFromArgument (const string & inArgument, CNTV2Card & outDevice)
{
	outDevice.Close();
	if (inArgument.empty())
		return false;

	//	Special case:  'LIST' or '?'  ---  print an enumeration of available devices to stdout, then bail
	AJAAutoLock tmpLock(&sDevInfoListLock);
	ScanHardware();
	string upperArg(inArgument);  aja::upper(upperArg);
	if (upperArg == "LIST" || upperArg == "?")
	{
		if (sDevInfoList.empty())
			cout << "No devices detected" << endl;
		else
			cout << DEC(sDevInfoList.size()) << " available " << (sDevInfoList.size() == 1 ? "device:" : "devices:") << endl;
		for (size_t ndx(0);  ndx < sDevInfoList.size();  ndx++)
		{
			if (sDevInfoList.at(ndx).isVirtualDevice)
			{
				cout << DECN(ndx, 2) << " | " << setw(8) << "virtual" << " | " << sDevInfoList.at(ndx).vdevUrl << endl;
			}
			else
			{
				cout << DECN(ndx, 2) << " | " << setw(8) << "local";
				const string serNum(sDevInfoList.at(ndx).serialNumber);
				if (!serNum.empty())
					cout << " | " << setw(10) << serNum;
				cout << " | " << setw(16) << ::NTV2DeviceIDToString(sDevInfoList.at(ndx).deviceID);
				cout << endl;
			}
		}
		return false;
	}

	for (NTV2DeviceInfoListConstIter iter(sDevInfoList.begin());  iter != sDevInfoList.end();  ++iter)
	{
		if (to_string(iter->deviceIndex) == inArgument)
			return outDevice.Open(iter->isVirtualDevice ? iter->vdevUrl : inArgument);
		else if (iter->deviceIdentifier == inArgument)
			return outDevice.Open(iter->isVirtualDevice ? iter->vdevUrl : inArgument);
		else if (aja::is_legal_decimal_number(inArgument, inArgument.length())
			&& aja::is_legal_hex_serial_number(inArgument)
			&& iter->deviceID == stoi(inArgument))
				return outDevice.Open(iter->isVirtualDevice ? iter->vdevUrl : inArgument);
		else if (IsLegalSerialNumber(inArgument))
			return outDevice.Open(inArgument);
		else if (inArgument.find("://") != string::npos)
			return outDevice.Open(inArgument);
	}
	return false;
}	//	GetFirstDeviceFromArgument


string CNTV2DeviceScanner::GetDeviceRefName (CNTV2Card & inDevice)
{	//	Name that will find given device via CNTV2DeviceScanner::GetFirstDeviceFromArgument
	if (!inDevice.IsOpen())
		return string();
	//	Nub address 1st...
	if (!inDevice.GetHostName().empty()  &&  inDevice.IsRemote())
		return inDevice.GetHostName();	//	Nub host/device

	//	Serial number 2nd...
	string str;
	if (inDevice.GetSerialNumberString(str))
		return str;

	//	Model name 3rd...
	str = ::NTV2DeviceIDToString(inDevice.GetDeviceID(), false);
	if (!str.empty() &&  str != "???")
		return str;

	//	Index number last...
	ostringstream oss;  oss << DEC(inDevice.GetIndexNumber());
	return oss.str();
}


#if !defined(NTV2_DEPRECATE_17_1)
ostream &	operator << (ostream & inOutStr, const NTV2DeviceInfoList & inList)
{
	for (NTV2DeviceInfoListConstIter iter(inList.begin());  iter != inList.end();  ++iter)
		inOutStr << " " << *iter;
	return inOutStr;

}	//	NTV2DeviceInfoList ostream operator <<


bool NTV2DeviceInfo::operator == (const NTV2DeviceInfo & second) const
{
	const NTV2DeviceInfo &	first	(*this);
	size_t					diffs	(0);

	//	'memcmp' would be simpler, but because NTV2DeviceInfo has no constructor, the unfilled bytes in
	//	its "boardIdentifier" field are indeterminate, making it worthless for accurate comparisons.
	//	"boardSerialNumber" and boardNumber are the only required comparisons, but I also check boardType,
	//	boardID, and pciSlot for good measure...
	if (first.deviceID		!=	second.deviceID)		diffs++;
	if (first.deviceIndex	!=	second.deviceIndex)		diffs++;
	if (first.serialNumber	!=	second.serialNumber)	diffs++;
	if (first.pciSlot		!=	second.pciSlot)			diffs++;

	// Needs to be fixed now that deviceIdentifier is a std::string
	//#if defined (AJA_DEBUG)
	//	if (::strncmp (first.deviceIdentifier.c_str (), second.deviceIdentifier.c_str (), first.deviceIdentifier.length ())))	diffs++;
	//	if (diffs)
	//		{cout << "## DEBUG:  " << diffs << " diff(s):" << endl << "#### first ####" << endl << first << "#### second ####" << endl << second << endl;}
	//#endif	//	AJA_DEBUG

	return diffs ? false : true;

}	//	equality operator

NTV2DeviceInfo::NTV2DeviceInfo()
{
	deviceID						= DEVICE_ID_INVALID;
	deviceIndex						= 0;
	pciSlot							= 0;
	deviceSerialNumber				= 0;
	serialNumber					= "";
	numVidInputs					= 0;
	numVidOutputs					= 0;
	numAnlgVidInputs				= 0;
	numAnlgVidOutputs				= 0;
	numHDMIVidInputs				= 0;
	numHDMIVidOutputs				= 0;
	numInputConverters				= 0;
	numOutputConverters				= 0;
	numUpConverters					= 0;
	numDownConverters				= 0;
	downConverterDelay				= 0;
	isoConvertSupport				= false;
	rateConvertSupport				= false;
	dvcproHDSupport					= false;
	qrezSupport						= false;
	hdvSupport						= false;
	quarterExpandSupport			= false;
	vidProcSupport					= false;
	dualLinkSupport					= false;
	colorCorrectionSupport			= false;
	programmableCSCSupport			= false;
	rgbAlphaOutputSupport			= false;
	breakoutBoxSupport				= false;
	procAmpSupport					= false;
	has2KSupport					= false;
	has4KSupport					= false;
	has8KSupport					= false;
	has3GLevelConversion			= false;
	proResSupport					= false;
	sdi3GSupport					= false;
	sdi12GSupport					= false;
	ipSupport						= false;
	biDirectionalSDI				= false;
	ltcInSupport					= false;
	ltcOutSupport					= false;
	ltcInOnRefPort					= false;
	stereoOutSupport				= false;
	stereoInSupport					= false;
	multiFormat						= false;
	numAudioStreams					= 0;
	numAnalogAudioInputChannels		= 0;
	numAESAudioInputChannels		= 0;
	numEmbeddedAudioInputChannels	= 0;
	numHDMIAudioInputChannels		= 0;
	numAnalogAudioOutputChannels	= 0;
	numAESAudioOutputChannels		= 0;
	numEmbeddedAudioOutputChannels	= 0;
	numHDMIAudioOutputChannels		= 0;
	numDMAEngines					= 0;
	numSerialPorts					= 0;
	pingLED							= 0;
	deviceIdentifier.clear();
	audioSampleRateList.clear();
	audioNumChannelsList.clear();
	audioBitsPerSampleList.clear();
	audioInSourceList.clear();
	audioOutSourceList.clear();
}

NTV2DeviceInfo::NTV2DeviceInfo (const NTV2DeviceInfo & info)
{
	if (&info != this)
		*this = info;
}


bool CNTV2DeviceScanner::CompareDeviceInfoLists (const NTV2DeviceInfoList & inOldList,
											const NTV2DeviceInfoList & inNewList,
											NTV2DeviceInfoList & outBoardsAdded,
											NTV2DeviceInfoList & outBoardsRemoved)
{
	NTV2DeviceInfoListConstIter	oldIter	(inOldList.begin ());
	NTV2DeviceInfoListConstIter	newIter	(inNewList.begin ());

	outBoardsAdded.clear ();
	outBoardsRemoved.clear ();

	while (true)
	{
		if (oldIter == inOldList.end () && newIter == inNewList.end ())
			break;	//	Done -- exit

		if (oldIter != inOldList.end () && newIter != inNewList.end ())
		{
			const NTV2DeviceInfo &  oldInfo (*oldIter),  newInfo (*newIter);

			if (oldInfo != newInfo)
			{
				//	Out with the old...
				outBoardsRemoved.push_back (oldInfo);

				//	In with the new...
				if (newInfo.deviceID && newInfo.deviceID != NTV2DeviceID(0xFFFFFFFF))
					outBoardsAdded.push_back (newInfo);
			}	//	if mismatch

			++oldIter;
			++newIter;
			continue;	//	Move along

		}	//	if both valid

		if (oldIter != inOldList.end () && newIter == inNewList.end ())
		{
			outBoardsRemoved.push_back (*oldIter);
			++oldIter;
			continue;	//	Move along
		}	//	old is valid, new is not valid

		if (oldIter == inOldList.end () && newIter != inNewList.end ())
		{
			if (newIter->deviceID && newIter->deviceID != NTV2DeviceID(0xFFFFFFFF))
				outBoardsAdded.push_back (*newIter);
			++newIter;
			continue;	//	Move along
		}	//	old is not valid, new is valid

		NTV2_ASSERT(false && "should never get here");

	}	//	loop til break

	//	Return 'true' if there were any changes...
	return !outBoardsAdded.empty () || !outBoardsRemoved.empty ();

}	//	CompareDeviceInfoLists


ostream &	operator << (ostream & inOutStr, const NTV2AudioSampleRateList & inList)
{
	for (NTV2AudioSampleRateListConstIter iter (inList.begin ()); iter != inList.end (); ++iter)
		inOutStr << " " << *iter;

	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioChannelsPerFrameList & inList)
{
	for (NTV2AudioChannelsPerFrameListConstIter iter (inList.begin ());  iter != inList.end ();  ++iter)
		inOutStr << " " << *iter;

	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioSourceList & inList)
{
	for (NTV2AudioSourceListConstIter iter(inList.begin());  iter != inList.end();  ++iter)
		switch (*iter)	//	AudioSourceEnum
		{
			case kSourceSDI:	return inOutStr << " SDI";
			case kSourceAES:	return inOutStr << " AES";
			case kSourceADAT:	return inOutStr << " ADAT";
			case kSourceAnalog:	return inOutStr << " Analog";
			case kSourceNone:	return inOutStr << " None";
			case kSourceAll:	return inOutStr << " All";
		}
	return inOutStr << " ???";
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioBitsPerSampleList & inList)
{
	for (NTV2AudioBitsPerSampleListConstIter iter (inList.begin ());  iter != inList.end ();  ++iter)
		inOutStr << " " << *iter;

	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2DeviceInfo & inInfo)
{
	inOutStr	<< "Device Info for '" << inInfo.deviceIdentifier << "'" << endl
				<< "            Device Index Number: " << inInfo.deviceIndex << endl
				<< "                      Device ID: 0x" << hex << inInfo.deviceID << dec << endl
				<< "                  Serial Number: " << inInfo.serialNumber << endl
				<< "                       PCI Slot: 0x" << hex << inInfo.pciSlot << dec << endl
				<< "                   Video Inputs: " << inInfo.numVidInputs << endl
				<< "                  Video Outputs: " << inInfo.numVidOutputs << endl
				#if defined (_DEBUG)
					<< "            Analog Video Inputs: " << inInfo.numAnlgVidInputs << endl
					<< "           Analog Video Outputs: " << inInfo.numAnlgVidOutputs << endl
					<< "              HDMI Video Inputs: " << inInfo.numHDMIVidInputs << endl
					<< "             HDMI Video Outputs: " << inInfo.numHDMIVidOutputs << endl
					<< "               Input Converters: " << inInfo.numInputConverters << endl
					<< "              Output Converters: " << inInfo.numOutputConverters << endl
					<< "                  Up Converters: " << inInfo.numUpConverters << endl
					<< "                Down Converters: " << inInfo.numDownConverters << endl
					<< "           Down Converter Delay: " << inInfo.downConverterDelay << endl
					<< "                       DVCProHD: " << (inInfo.dvcproHDSupport ? "Y" : "N") << endl
					<< "                           Qrez: " << (inInfo.qrezSupport ? "Y" : "N") << endl
					<< "                            HDV: " << (inInfo.hdvSupport ? "Y" : "N") << endl
					<< "                 Quarter Expand: " << (inInfo.quarterExpandSupport ? "Y" : "N") << endl
					<< "                    ISO Convert: " << (inInfo.isoConvertSupport ? "Y" : "N") << endl
					<< "                   Rate Convert: " << (inInfo.rateConvertSupport ? "Y" : "N") << endl
					<< "                        VidProc: " << (inInfo.vidProcSupport ? "Y" : "N") << endl
					<< "                      Dual-Link: " << (inInfo.dualLinkSupport ? "Y" : "N") << endl
					<< "               Color-Correction: " << (inInfo.colorCorrectionSupport ? "Y" : "N") << endl
					<< "               Programmable CSC: " << (inInfo.programmableCSCSupport ? "Y" : "N") << endl
					<< "               RGB Alpha Output: " << (inInfo.rgbAlphaOutputSupport ? "Y" : "N") << endl
					<< "                   Breakout Box: " << (inInfo.breakoutBoxSupport ? "Y" : "N") << endl
					<< "                        ProcAmp: " << (inInfo.procAmpSupport ? "Y" : "N") << endl
					<< "                             2K: " << (inInfo.has2KSupport ? "Y" : "N") << endl
					<< "                             4K: " << (inInfo.has4KSupport ? "Y" : "N") << endl
					<< "                             8K: " << (inInfo.has8KSupport ? "Y" : "N") << endl
                    << "            3G Level Conversion: " << (inInfo.has3GLevelConversion ? "Y" : "N") << endl
					<< "                         ProRes: " << (inInfo.proResSupport ? "Y" : "N") << endl
					<< "                         SDI 3G: " << (inInfo.sdi3GSupport ? "Y" : "N") << endl
					<< "                        SDI 12G: " << (inInfo.sdi12GSupport ? "Y" : "N") << endl
					<< "                             IP: " << (inInfo.ipSupport ? "Y" : "N") << endl
					<< "             SDI Bi-Directional: " << (inInfo.biDirectionalSDI ? "Y" : "N") << endl
					<< "                         LTC In: " << (inInfo.ltcInSupport ? "Y" : "N") << endl
					<< "                        LTC Out: " << (inInfo.ltcOutSupport ? "Y" : "N") << endl
					<< "             LTC In on Ref Port: " << (inInfo.ltcInOnRefPort ? "Y" : "N") << endl
					<< "                     Stereo Out: " << (inInfo.stereoOutSupport ? "Y" : "N") << endl
					<< "                      Stereo In: " << (inInfo.stereoInSupport ? "Y" : "N") << endl
					<< "             Audio Sample Rates: " << inInfo.audioSampleRateList << endl
					<< "           AudioNumChannelsList: " << inInfo.audioNumChannelsList << endl
					<< "         AudioBitsPerSampleList: " << inInfo.audioBitsPerSampleList << endl
					<< "              AudioInSourceList: " << inInfo.audioInSourceList << endl
					<< "             AudioOutSourceList: " << inInfo.audioOutSourceList << endl
					<< "                  Audio Streams: " << inInfo.numAudioStreams << endl
					<< "    Analog Audio Input Channels: " << inInfo.numAnalogAudioInputChannels << endl
					<< "   Analog Audio Output Channels: " << inInfo.numAnalogAudioOutputChannels << endl
					<< "       AES Audio Input Channels: " << inInfo.numAESAudioInputChannels << endl
					<< "      AES Audio Output Channels: " << inInfo.numAESAudioOutputChannels << endl
					<< "  Embedded Audio Input Channels: " << inInfo.numEmbeddedAudioInputChannels << endl
					<< " Embedded Audio Output Channels: " << inInfo.numEmbeddedAudioOutputChannels << endl
					<< "      HDMI Audio Input Channels: " << inInfo.numHDMIAudioInputChannels << endl
					<< "     HDMI Audio Output Channels: " << inInfo.numHDMIAudioOutputChannels << endl
					<< "                    DMA Engines: " << inInfo.numDMAEngines << endl
					<< "                   Serial Ports: " << inInfo.numSerialPorts << endl
				#endif	//	AJA_DEBUG
				<< "";

	return inOutStr;

}	//	NTV2DeviceInfo ostream operator <<


ostream &	operator << (ostream & inOutStr, const NTV2AudioPhysicalFormat & inFormat)
{
	inOutStr	<< "AudioPhysicalFormat:" << endl
				<< "    boardNumber: " << inFormat.boardNumber << endl
				<< "     sampleRate: " << inFormat.sampleRate << endl
				<< "    numChannels: " << inFormat.numChannels << endl
				<< "  bitsPerSample: " << inFormat.bitsPerSample << endl
	#if defined (DEBUG) || defined (AJA_DEBUG)
				<< "       sourceIn: 0x" << hex << inFormat.sourceIn << dec << endl
				<< "      sourceOut: 0x" << hex << inFormat.sourceOut << dec << endl
	#endif	//	DEBUG or AJA_DEBUG
				;

	return inOutStr;

}	//	AudioPhysicalFormat ostream operator <<


std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioPhysicalFormatList & inList)
{
	for (NTV2AudioPhysicalFormatListConstIter iter (inList.begin ()); iter != inList.end (); ++iter)
		inOutStr << *iter;

	return inOutStr;

}	//	AudioPhysicalFormatList ostream operator <<


// Private methods

void CNTV2DeviceScanner::SetDeviceAttributes (NTV2DeviceInfo & inDeviceInfo, CNTV2Card & inDevice)
{
	inDevice.GetSerialNumberString(inDeviceInfo.serialNumber);
	const ULWordSet wgtIDs (inDevice.GetSupportedItems(kNTV2EnumsID_WidgetID));
	inDeviceInfo.numVidInputs			= inDevice.GetNumSupported(kDeviceGetNumVideoInputs);
	inDeviceInfo.numVidOutputs			= inDevice.GetNumSupported(kDeviceGetNumVideoOutputs);
	inDeviceInfo.numAnlgVidOutputs		= inDevice.GetNumSupported(kDeviceGetNumAnalogVideoOutputs);
	inDeviceInfo.numAnlgVidInputs		= inDevice.GetNumSupported(kDeviceGetNumAnalogVideoInputs);
	inDeviceInfo.numHDMIVidOutputs		= inDevice.GetNumSupported(kDeviceGetNumHDMIVideoOutputs);
	inDeviceInfo.numHDMIVidInputs		= inDevice.GetNumSupported(kDeviceGetNumHDMIVideoInputs);
	inDeviceInfo.numInputConverters		= inDevice.GetNumSupported(kDeviceGetNumInputConverters);
	inDeviceInfo.numOutputConverters	= inDevice.GetNumSupported(kDeviceGetNumOutputConverters);
	inDeviceInfo.numUpConverters		= inDevice.GetNumSupported(kDeviceGetNumUpConverters);
	inDeviceInfo.numDownConverters		= inDevice.GetNumSupported(kDeviceGetNumDownConverters);
	inDeviceInfo.downConverterDelay		= inDevice.GetNumSupported(kDeviceGetDownConverterDelay);
	inDeviceInfo.dvcproHDSupport		= inDevice.IsSupported(kDeviceCanDoDVCProHD);
	inDeviceInfo.qrezSupport			= inDevice.IsSupported(kDeviceCanDoQREZ);
	inDeviceInfo.hdvSupport				= inDevice.IsSupported(kDeviceCanDoHDV);
	inDeviceInfo.quarterExpandSupport	= inDevice.IsSupported(kDeviceCanDoQuarterExpand);
	inDeviceInfo.colorCorrectionSupport	= inDevice.IsSupported(kDeviceCanDoColorCorrection);
	inDeviceInfo.programmableCSCSupport	= inDevice.IsSupported(kDeviceCanDoProgrammableCSC);
	inDeviceInfo.rgbAlphaOutputSupport	= inDevice.IsSupported(kDeviceCanDoRGBPlusAlphaOut);
	inDeviceInfo.breakoutBoxSupport		= inDevice.IsSupported(kDeviceCanDoBreakoutBox);
	inDeviceInfo.vidProcSupport			= inDevice.IsSupported(kDeviceCanDoVideoProcessing);
	inDeviceInfo.dualLinkSupport		= inDevice.IsSupported(kDeviceCanDoDualLink);
	inDeviceInfo.numDMAEngines			= UWord(inDevice.GetNumSupported(kDeviceGetNumDMAEngines));
	inDeviceInfo.pingLED				= inDevice.GetNumSupported(kDeviceGetPingLED);
	inDeviceInfo.has2KSupport			= inDevice.IsSupported(kDeviceCanDo2KVideo);
	inDeviceInfo.has4KSupport			= inDevice.IsSupported(kDeviceCanDo4KVideo);
	inDeviceInfo.has8KSupport			= inDevice.IsSupported(kDeviceCanDo8KVideo);
	inDeviceInfo.has3GLevelConversion   = inDevice.IsSupported(kDeviceCanDo3GLevelConversion);
	inDeviceInfo.isoConvertSupport		= inDevice.IsSupported(kDeviceCanDoIsoConvert);
	inDeviceInfo.rateConvertSupport		= inDevice.IsSupported(kDeviceCanDoRateConvert);
	inDeviceInfo.proResSupport			= inDevice.IsSupported(kDeviceCanDoProRes);
	inDeviceInfo.sdi3GSupport			= wgtIDs.find(NTV2_Wgt3GSDIOut1) != wgtIDs.end();
	inDeviceInfo.sdi12GSupport			= inDevice.IsSupported(kDeviceCanDo12GSDI);
	inDeviceInfo.ipSupport				= inDevice.IsSupported(kDeviceCanDoIP);
	inDeviceInfo.biDirectionalSDI		= inDevice.IsSupported(kDeviceHasBiDirectionalSDI);
	inDeviceInfo.ltcInSupport			= inDevice.GetNumSupported(kDeviceGetNumLTCInputs) > 0;
	inDeviceInfo.ltcOutSupport			= inDevice.GetNumSupported(kDeviceGetNumLTCOutputs) > 0;
	inDeviceInfo.ltcInOnRefPort			= inDevice.IsSupported(kDeviceCanDoLTCInOnRefPort);
	inDeviceInfo.stereoOutSupport		= inDevice.IsSupported(kDeviceCanDoStereoOut);
	inDeviceInfo.stereoInSupport		= inDevice.IsSupported(kDeviceCanDoStereoIn);
	inDeviceInfo.multiFormat			= inDevice.IsSupported(kDeviceCanDoMultiFormat);
	inDeviceInfo.numSerialPorts			= inDevice.GetNumSupported(kDeviceGetNumSerialPorts);
	inDeviceInfo.procAmpSupport			= false;
}

void CNTV2DeviceScanner::SetAudioAttributes (NTV2DeviceInfo & info, CNTV2Card & inBoard)
{
	//	Start with empty lists...
	info.audioSampleRateList.clear();
	info.audioNumChannelsList.clear();
	info.audioBitsPerSampleList.clear();
	info.audioInSourceList.clear();
	info.audioOutSourceList.clear();


	if (inBoard.GetNumSupported(kDeviceGetNumAudioSystems))
	{
		ULWord audioControl;
		inBoard.ReadRegister(kRegAud1Control, audioControl);

		//audioSampleRateList
		info.audioSampleRateList.push_back(k48KHzSampleRate);
		if (inBoard.IsSupported(kDeviceCanDoAudio96K))
			info.audioSampleRateList.push_back(k96KHzSampleRate);

		//audioBitsPerSampleList
		info.audioBitsPerSampleList.push_back(k32bitsPerSample);

		//audioInSourceList
		info.audioInSourceList.push_back(kSourceSDI);
		if (audioControl & BIT(21))
			info.audioInSourceList.push_back(kSourceAES);
		if (inBoard.IsSupported(kDeviceCanDoAnalogAudio))
			info.audioInSourceList.push_back(kSourceAnalog);

		//audioOutSourceList
		info.audioOutSourceList.push_back(kSourceAll);

		//audioNumChannelsList
		if (inBoard.IsSupported(kDeviceCanDoAudio2Channels))
			info.audioNumChannelsList.push_back(kNumAudioChannels2);
		if (inBoard.IsSupported(kDeviceCanDoAudio6Channels))
			info.audioNumChannelsList.push_back(kNumAudioChannels6);
		if (inBoard.IsSupported(kDeviceCanDoAudio8Channels))
			info.audioNumChannelsList.push_back(kNumAudioChannels8);

		info.numAudioStreams = inBoard.GetNumSupported(kDeviceGetNumAudioSystems);
	}

	info.numAnalogAudioInputChannels	= inBoard.GetNumSupported(kDeviceGetNumAnalogAudioInputChannels);
	info.numAESAudioInputChannels		= inBoard.GetNumSupported(kDeviceGetNumAESAudioInputChannels);
	info.numEmbeddedAudioInputChannels	= inBoard.GetNumSupported(kDeviceGetNumEmbeddedAudioInputChannels);
	info.numHDMIAudioInputChannels		= inBoard.GetNumSupported(kDeviceGetNumHDMIAudioInputChannels);
	info.numAnalogAudioOutputChannels	= inBoard.GetNumSupported(kDeviceGetNumAnalogAudioOutputChannels);
	info.numAESAudioOutputChannels		= inBoard.GetNumSupported(kDeviceGetNumAESAudioOutputChannels);
	info.numEmbeddedAudioOutputChannels	= inBoard.GetNumSupported(kDeviceGetNumEmbeddedAudioOutputChannels);
	info.numHDMIAudioOutputChannels		= inBoard.GetNumSupported(kDeviceGetNumHDMIAudioOutputChannels);

}	//	SetAudioAttributes


bool CNTV2DeviceScanner::GetVirtualDeviceList(NTV2DeviceInfoList& outVirtualDevList)
{
	string vdevPath;
	AJASystemInfo info;
	if (info.GetValue(AJA_SystemInfoTag_Path_PersistenceStoreUser, vdevPath) != AJA_STATUS_SUCCESS)
		return false;
	vdevPath = vdevPath + "/virtualdevices";
	int vdIndex = outVirtualDevList.size();
	std::vector<std::string> vdevFiles;
	AJAFileIO::ReadDirectory(vdevPath, "*.vdev", vdevFiles);
	for (const auto& vdevFile : vdevFiles)
	{
		std::ifstream cfgJsonfile(vdevFile);
		json vdevJson;
		if (cfgJsonfile.is_open())
		{
			try
			{
				vdevJson = json::parse(cfgJsonfile);
			}
			catch (const json::parse_error& e)
			{
				cerr << "JSON parse error: " << e.what() << endl;
				cerr << "Exception id: " << e.id << endl;
				cerr << "Byte position of error: " << e.byte << endl;
				return false;
			}
		}
		else
			return false;
		cfgJsonfile.close();

		NTV2DeviceInfo newVDev;
		newVDev.isVirtualDevice = true;
		newVDev.deviceIndex = vdIndex++;
		newVDev.deviceID = DEVICE_ID_SOFTWARE;
		newVDev.deviceIdentifier = "";

		// There are 3 special keys:
		// plugin - this is required, specifies the name of plug-in to try loading.
		// name - optional, specifies a human readable name for the virtual device,
		//		  defaults to "" if not specified.
		// host - optional, host to use in the plug-in url, defaults to "localhost"
		//        if not specified.
		string hostName;
		auto pluginVal = vdevJson["plugin"];
		auto nameVal = vdevJson["name"];
		auto hostVal = vdevJson["host"];

		if (pluginVal.is_null())
		{
			cerr << "JSON file: '" << vdevFile << "' is missing the required paramater 'plugin'." << endl;
			continue;
		}
		newVDev.vdevUrl = pluginVal.get<std::string>();

		if (!nameVal.is_null())
		{
			ostringstream oss;
			oss << nameVal.get<std::string>() << " - " << newVDev.deviceIndex;

			newVDev.deviceIdentifier = oss.str();
		}

		if (!hostVal.is_null())
		{
			hostName = hostVal.get<std::string>();
		}

		if (hostName.empty())
		{
			hostName = "localhost";
		}

		newVDev.vdevUrl += "://" + hostName + "/?";
		bool isFirstParam = true;
		for (auto it = vdevJson.begin(); it != vdevJson.end(); ++it)
		{
			if (it.key() != "plugin" && it.key() != "name" && it.key() != "host")
			{
				auto paramValStr = to_string(it.value());
				aja::strip(paramValStr, "\"");
				newVDev.vdevUrl += (isFirstParam ? "" : "&") + it.key() + "=" + PercentEncode(paramValStr);
				isFirstParam = false;
			}
		}

		if (!newVDev.deviceIdentifier.empty())
		{
			string displayNameParam = "displayname";
			newVDev.vdevUrl += (isFirstParam ? "" : "&") + displayNameParam + "=" + PercentEncode(newVDev.deviceIdentifier);
		}

		CNTV2Card tmpDev;
		tmpDev.Open(newVDev.vdevUrl);
		if (!tmpDev.IsOpen())
		{
			cerr << "Unable to open device based on JSON file: " << vdevFile << endl;
			continue;
		}
		SetDeviceAttributes(newVDev, tmpDev);
		SetAudioAttributes(newVDev, tmpDev);

		outVirtualDevList.push_back(newVDev);
	}
	return true;
}
#endif	//	!defined(NTV2_DEPRECATE_17_1)
