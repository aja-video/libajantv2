/**
	@file		ntv2devicescanner.cpp
	@brief		Implementation of CNTV2DeviceScanner.
	@copyright	(C) 2004-2014 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2devicescanner.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include <sstream>
#include <assert.h>
using namespace std;


static string ToLower (const string & inStr)
{
	string	result (inStr);
	std::transform (result.begin (), result.end (), result.begin (), ::tolower);
	return result;
}


CNTV2DeviceScanner::CNTV2DeviceScanner (const bool inScanNow)
{
	if (inScanNow)
		ScanHardware ();
}
CNTV2DeviceScanner::CNTV2DeviceScanner (bool inScanNow, UWord inDeviceMask)
{
	(void)inDeviceMask;
	if (inScanNow)
		ScanHardware ();
}


CNTV2DeviceScanner & CNTV2DeviceScanner::operator = (const CNTV2DeviceScanner & boardScan)
{
	//	Avoid self-assignment...
	if (this != &boardScan)
		DeepCopy (boardScan);

	return *this;	//	operator= must return self reference

}	//	operator=


CNTV2DeviceScanner::CNTV2DeviceScanner (const CNTV2DeviceScanner & boardScan)
{
	DeepCopy (boardScan);
}


void CNTV2DeviceScanner::DeepCopy (const CNTV2DeviceScanner & boardScan)
{
	// Begin with a clear list
	_deviceInfoList.clear();
	
	// Copy over _deviceInfoList
	for (NTV2DeviceInfoListConstIter bilIter (boardScan._deviceInfoList.begin ());  bilIter != boardScan._deviceInfoList.end ();  ++bilIter)
	{
		NTV2DeviceInfo boardInfo;
		
		//	Move over all the easy stuff...
// 		#if !defined (NTV2_DEPRECATE)
// 			boardInfo.boardType = bilIter->boardType;
// 			boardInfo.boardID = bilIter->boardID;
// 			boardInfo.boardSerialNumber = bilIter->boardSerialNumber;
// 			boardInfo.boardNumber = bilIter->boardNumber;
// 			for (ULWord i = 0;  i < sizeof (boardInfo.boardIdentifier);  i++)
// 				boardInfo.boardIdentifier[i] = bilIter->boardIdentifier[i];
// 		#endif	//	!defined (NTV2_DEPRECATE)
		boardInfo.deviceIndex = bilIter->deviceIndex;
		boardInfo.deviceID = bilIter->deviceID;
		boardInfo.pciSlot = bilIter->pciSlot;
		boardInfo.deviceIdentifier = bilIter->deviceIdentifier;
		boardInfo.deviceSerialNumber = bilIter->deviceSerialNumber;

		//	Now copy over each list within the list...
		boardInfo.audioSampleRateList.clear ();
		for (NTV2AudioSampleRateList::const_iterator asrIter (bilIter->audioSampleRateList.begin ());  asrIter != bilIter->audioSampleRateList.end ();  ++asrIter)
			boardInfo.audioSampleRateList.push_back (*asrIter);

		boardInfo.audioNumChannelsList.clear ();
		for (NTV2AudioChannelsPerFrameList::const_iterator ncIter (bilIter->audioNumChannelsList.begin ());  ncIter != bilIter->audioNumChannelsList.end ();  ++ncIter)
			boardInfo.audioNumChannelsList.push_back (*ncIter);

		boardInfo.audioBitsPerSampleList.clear ();
		for (NTV2AudioBitsPerSampleList::const_iterator bpsIter (bilIter->audioBitsPerSampleList.begin ());  bpsIter != bilIter->audioBitsPerSampleList.end ();  ++bpsIter)
			boardInfo.audioBitsPerSampleList.push_back (*bpsIter);

		boardInfo.audioInSourceList.clear ();
		for (NTV2AudioSourceList::const_iterator aslIter (bilIter->audioInSourceList.begin ());  aslIter != bilIter->audioInSourceList.end ();  ++aslIter)
			boardInfo.audioInSourceList.push_back (*aslIter);

		boardInfo.audioOutSourceList.clear ();
		for (NTV2AudioSourceList::const_iterator aoslIter (bilIter->audioOutSourceList.begin ());  aoslIter != bilIter->audioOutSourceList.end ();  ++aoslIter)
			boardInfo.audioOutSourceList.push_back (*aoslIter);

		//	Add this boardInfo struct to the _deviceInfoList...
		_deviceInfoList.push_back (boardInfo);
	}

}	//	DeepCopy

void CNTV2DeviceScanner::ScanHardware(UWord inDeviceMask)
{
	(void) inDeviceMask;
	ScanHardware();
}
void CNTV2DeviceScanner::ScanHardware (void)
{
	GetDeviceInfoList ().clear ();

	for (UWord boardNum = 0;  ;  boardNum++)
	{
		CNTV2Card	tmpDevice;
		if (tmpDevice.Open (boardNum))
		{
			const NTV2DeviceID	deviceID (tmpDevice.GetDeviceID ());

			if (deviceID != DEVICE_ID_NOTFOUND)
			{
				ostringstream	oss;
				NTV2DeviceInfo	info;

				info.deviceIndex		= boardNum;
				info.deviceID			= deviceID;
				info.pciSlot			= tmpDevice.GetPCISlotNumber ();
				info.deviceSerialNumber	= tmpDevice.GetSerialNumber ();

				oss << ::NTV2DeviceIDToString (deviceID) << " - " << boardNum;
				if (info.pciSlot)
					oss << ", Slot " << info.pciSlot;

				info.deviceIdentifier = oss.str ();
#if !defined (NTV2_DEPRECATE)
				strcpy(info.boardIdentifier, oss.str().c_str());
				info.boardID = deviceID;
				info.boardSerialNumber = tmpDevice.GetSerialNumber();
				info.boardNumber = boardNum;
#endif

				SetVideoAttributes (info);
				SetAudioAttributes (info, tmpDevice);

				GetDeviceInfoList ().push_back (info);
			}
			tmpDevice.Close ();
		}	//	if Open succeeded
		else
			break;
	}	//	boardNum loop

}	//	ScanHardware


#if !defined (NTV2_DEPRECATE)
	bool CNTV2DeviceScanner::BoardTypePresent (NTV2BoardType boardType, bool rescan)
	{
		if (rescan)
			ScanHardware (true);

		const NTV2DeviceInfoList & boardList (GetDeviceInfoList ());
		for (NTV2DeviceInfoListConstIter boardIter = boardList.begin ();  boardIter != boardList.end ();  ++boardIter)
			if (boardIter->boardType == boardType)
				return true;	//	Found!

		return false;	//	Not found

	}	//	BoardTypePresent

#endif	//	else !defined (NTV2_DEPRECATE)


bool CNTV2DeviceScanner::DeviceIDPresent (const NTV2DeviceID inDeviceID, const bool inRescan)
{
	if (inRescan)
		ScanHardware ();

	const NTV2DeviceInfoList & deviceInfoList (GetDeviceInfoList ());
	for (NTV2DeviceInfoListConstIter iter (deviceInfoList.begin ());  iter != deviceInfoList.end ();  ++iter)
		if (iter->deviceID == inDeviceID)
			return true;	//	Found!

	return false;	//	Not found

}	//	DeviceIDPresent


bool CNTV2DeviceScanner::GetDeviceInfo (const ULWord inDeviceIndexNumber, NTV2DeviceInfo & outDeviceInfo, const bool inRescan)
{
	if (inRescan)
		ScanHardware ();

	const NTV2DeviceInfoList & deviceList (GetDeviceInfoList ());

	if (inDeviceIndexNumber < deviceList.size ())
	{
		outDeviceInfo = deviceList [inDeviceIndexNumber];
		return outDeviceInfo.deviceIndex == inDeviceIndexNumber;
	}
	else
		return false;	//	No devices with this index number

}	//	GetDeviceInfo


bool CNTV2DeviceScanner::GetDeviceAtIndex (const ULWord inDeviceIndexNumber, CNTV2Card & outDevice)
{
	CNTV2DeviceScanner	scanner;
	return inDeviceIndexNumber < scanner.GetDeviceInfoList ().size () ? outDevice.Open (inDeviceIndexNumber) : false;

}	//	GetDeviceAtIndex


bool CNTV2DeviceScanner::GetFirstDeviceWithID (const NTV2DeviceID inDeviceID, CNTV2Card & outDevice)
{
	CNTV2DeviceScanner			scanner;
	const NTV2DeviceInfoList &	deviceInfoList (scanner.GetDeviceInfoList ());
	for (NTV2DeviceInfoListConstIter iter (deviceInfoList.begin ());  iter != deviceInfoList.end ();  ++iter)
		if (iter->deviceID == inDeviceID)
			return outDevice.Open (iter->deviceIndex);	//	Found!
	return false;	//	Not found

}	//	GetFirstDeviceWithID


bool CNTV2DeviceScanner::GetFirstDeviceWithName (const string inNameSubString, CNTV2Card & outDevice)
{
	CNTV2DeviceScanner			scanner;
	const string				nameSubString	(::ToLower (inNameSubString));
	const NTV2DeviceInfoList &	deviceInfoList	(scanner.GetDeviceInfoList ());

	for (NTV2DeviceInfoListConstIter iter (deviceInfoList.begin ());  iter != deviceInfoList.end ();  ++iter)
	{
		const string	deviceName	(::ToLower (iter->deviceIdentifier));
		if (deviceName.find (nameSubString) != string::npos)
			return outDevice.Open (iter->deviceIndex);	//	Found!
	}
	return false;	//	Not found

}	//	GetFirstDeviceWithName


bool CNTV2DeviceScanner::GetFirstDeviceFromArgument (const std::string inArgument, CNTV2Card & outDevice)
{
	if (inArgument.empty ())
		return false;

	if (inArgument.length () < 3)
	{
		if (::isdigit (inArgument [0]) || (inArgument.length () > 1 && ::isdigit (inArgument [1])))	//	Decimal number?
		{
			ULWord			deviceIndex	(0);
			istringstream	iss			(inArgument);
			iss >> deviceIndex;	//	String to number
			return GetDeviceAtIndex (deviceIndex, outDevice);
		}	//	if 1st char is digit
	}	//	if length is 1 or 2 chars

	if (inArgument.length () == 8)
	{
		CNTV2DeviceScanner			scanner;
		const NTV2DeviceInfoList &	infoList (scanner.GetDeviceInfoList ());

		for (NTV2DeviceInfoListConstIter iter (infoList.begin ());  iter != infoList.end ();  ++iter)
			if (CNTV2Card::SerialNum64ToString (iter->deviceSerialNumber) == inArgument)
				return outDevice.Open (iter->deviceIndex);	//	Found!
	}	//	if exactly 8 chars

	const string			argLowerCase		(::ToLower (inArgument));
	const NTV2DeviceIDSet	supportedDeviceIDs	(::NTV2GetSupportedDevices ());
	for (NTV2DeviceIDSetConstIter devIDiter (supportedDeviceIDs.begin ());  devIDiter != supportedDeviceIDs.end ();  ++devIDiter)
	{
		string	deviceName	(::ToLower (::NTV2DeviceIDToString (*devIDiter, false)));
		if (deviceName.find (argLowerCase) != string::npos)
			return GetFirstDeviceWithID (*devIDiter, outDevice);
	}

	return GetFirstDeviceWithName (inArgument, outDevice);

}	//	GetFirstDeviceFromArgument


ostream &	operator << (ostream & inOutStr, const NTV2DeviceInfoList & inList)
{
	for (NTV2DeviceInfoListConstIter	iter (inList.begin ());  iter != inList.end ();  ++iter)
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
	#if !defined (NTV2_DEPRECATE)
	if (first.boardType							!=	second.boardType)						diffs++;
	#endif	//	!defined (NTV2_DEPRECATE)
	if (first.deviceID							!=	second.deviceID)						diffs++;
	if (first.deviceIndex						!=	second.deviceIndex)						diffs++;
	if (first.deviceSerialNumber				!=	second.deviceSerialNumber)				diffs++;
	if (first.pciSlot							!=	second.pciSlot)							diffs++;
	
	// Needs to be fixed now that deviceIdentifier is a std::string
	//#if defined (AJA_DEBUG)
	//	if (::strncmp (first.deviceIdentifier.c_str (), second.deviceIdentifier.c_str (), first.deviceIdentifier.length ())))	diffs++;
	//	if (diffs)
	//		{cout << "## DEBUG:  " << diffs << " diff(s):" << endl << "#### first ####" << endl << first << "#### second ####" << endl << second << endl;}
	//#endif	//	AJA_DEBUG

	return diffs ? false : true;

}	//	equality operator


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
				if (newInfo.deviceID && newInfo.deviceID != (NTV2DeviceID)0xFFFFFFFF)
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
			if (newIter->deviceID && newIter->deviceID != (NTV2DeviceID) 0xFFFFFFFF)
				outBoardsAdded.push_back (*newIter);
			++newIter;
			continue;	//	Move along
		}	//	old is not valid, new is valid

		assert (false && "should never get here");

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
	for (NTV2AudioSourceListConstIter iter (inList.begin ());  iter != inList.end ();  ++iter)
		switch (*iter)	//	AudioSourceEnum
		{
			case kSourceSDI:	inOutStr << " SDI";		break;
			case kSourceAES:	inOutStr << " AES";		break;
			case kSourceADAT:	inOutStr << " ADAT";	break;
			case kSourceAnalog:	inOutStr << " Analog";	break;
			case kSourceNone:	inOutStr << " None";	break;
			case kSourceAll:	inOutStr << " All";		break;
			default:			inOutStr << " ???";		break;
		}
	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioBitsPerSampleList & inList)
{
	for (NTV2AudioBitsPerSampleListConstIter iter (inList.begin ());  iter != inList.end ();  ++iter)
		inOutStr << " " << *iter;

	return inOutStr;
}


#if !defined (NTV2_DEPRECATE)

	void CNTV2DeviceScanner::DumpBoardInfo (const NTV2DeviceInfo & info)
	{
		#if defined (DEBUG) || defined (_DEBUG) || defined (AJA_DEBUG)
			cout << info << endl;
		#else
			(void) info;
		#endif
	}	//	DumpBoardInfo

#endif	//	!NTV2_DEPRECATE


ostream &	operator << (ostream & inOutStr, const NTV2DeviceInfo & inInfo)
{
	inOutStr	<< "Device Info for '" << inInfo.deviceIdentifier << "'" << endl
				<< "            Device Index Number: " << inInfo.deviceIndex << endl
				#if !defined (NTV2_DEPRECATE)
				<< "                    Device Type: 0x" << hex << inInfo.boardType << dec << endl
				#endif	//	!defined (NTV2_DEPRECATE)
				<< "                      Device ID: 0x" << hex << inInfo.deviceID << dec << endl
				<< "                  Serial Number: 0x" << hex << inInfo.deviceSerialNumber << dec << endl
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
                    << "            3G Level Conversion: " << (inInfo.has3GLevelConversion ? "Y" : "N") << endl
					<< "                         ProRes: " << (inInfo.proResSupport ? "Y" : "N") << endl
					<< "                         SDI 3G: " << (inInfo.sdi3GSupport ? "Y" : "N") << endl
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


#if !defined (NTV2_DEPRECATE)

	void CNTV2DeviceScanner::DumpAudioFormatInfo (const NTV2AudioPhysicalFormat & audioPhysicalFormat)
	{
		#if defined (DEBUG) || defined (AJA_DEBUG)
			cout << audioPhysicalFormat << endl;
		#else
			(void) audioPhysicalFormat;
		#endif
	}	//	DumpAudioFormatInfo

#endif	//	!NTV2_DEPRECATE


// Private methods

void CNTV2DeviceScanner::SetVideoAttributes (NTV2DeviceInfo & info)
{	
	info.numVidInputs			= NTV2DeviceGetNumVideoInputs		(info.deviceID);
	info.numVidOutputs			= NTV2DeviceGetNumVideoOutputs		(info.deviceID);
	info.numAnlgVidOutputs		= NTV2DeviceGetNumAnalogVideoOutputs	(info.deviceID);
	info.numAnlgVidInputs		= NTV2DeviceGetNumAnalogVideoInputs	(info.deviceID);
	info.numHDMIVidOutputs		= NTV2DeviceGetNumHDMIVideoOutputs	(info.deviceID);
	info.numHDMIVidInputs		= NTV2DeviceGetNumHDMIVideoInputs	(info.deviceID);
	info.numInputConverters		= NTV2DeviceGetNumInputConverters	(info.deviceID);
	info.numOutputConverters	= NTV2DeviceGetNumOutputConverters	(info.deviceID);
	info.numUpConverters		= NTV2DeviceGetNumUpConverters		(info.deviceID);
	info.numDownConverters		= NTV2DeviceGetNumDownConverters	(info.deviceID);
	info.downConverterDelay		= NTV2DeviceGetDownConverterDelay	(info.deviceID);
	info.dvcproHDSupport		= NTV2DeviceCanDoDVCProHD			(info.deviceID);
	info.qrezSupport			= NTV2DeviceCanDoQREZ				(info.deviceID);
	info.hdvSupport				= NTV2DeviceCanDoHDV				(info.deviceID);
	info.quarterExpandSupport	= NTV2DeviceCanDoQuarterExpand		(info.deviceID);
	info.colorCorrectionSupport	= NTV2DeviceCanDoColorCorrection	(info.deviceID);
	info.programmableCSCSupport	= NTV2DeviceCanDoProgrammableCSC	(info.deviceID);
	info.rgbAlphaOutputSupport	= NTV2DeviceCanDoRGBPlusAlphaOut	(info.deviceID);
	info.breakoutBoxSupport		= NTV2DeviceCanDoBreakoutBox		(info.deviceID);
	info.vidProcSupport			= NTV2DeviceCanDoVideoProcessing	(info.deviceID);
	info.dualLinkSupport		= NTV2DeviceCanDoDualLink			(info.deviceID);
	info.numDMAEngines			= static_cast <UWord> (NTV2GetNumDMAEngines (info.deviceID));
	info.pingLED				= NTV2DeviceGetPingLED				(info.deviceID);
	info.has2KSupport			= NTV2DeviceCanDo2KVideo			(info.deviceID);
	info.has4KSupport			= NTV2DeviceCanDo4KVideo			(info.deviceID);
    info.has3GLevelConversion   = NTV2DeviceCanDo3GLevelConversion  (info.deviceID);
	info.isoConvertSupport		= NTV2DeviceCanDoIsoConvert			(info.deviceID);
	info.rateConvertSupport		= NTV2DeviceCanDoRateConvert		(info.deviceID);
	info.proResSupport			= NTV2DeviceCanDoProRes				(info.deviceID);
	info.sdi3GSupport			= NTV2DeviceCanDo3GOut				(info.deviceID, 0);
	info.ltcInSupport			= NTV2DeviceGetNumLTCInputs			(info.deviceID) > 0;
	info.ltcOutSupport			= NTV2DeviceGetNumLTCOutputs		(info.deviceID) > 0;
	info.ltcInOnRefPort			= NTV2DeviceCanDoLTCInOnRefPort		(info.deviceID);
	info.stereoOutSupport		= NTV2DeviceCanDoStereoOut			(info.deviceID);
	info.stereoInSupport		= NTV2DeviceCanDoStereoIn			(info.deviceID);
	info.multiFormat			= NTV2DeviceCanDoMultiFormat		(info.deviceID);
	info.numSerialPorts			= NTV2DeviceGetNumSerialPorts		(info.deviceID);
	#if !defined (NTV2_DEPRECATE)
		info.procAmpSupport		= NTV2BoardCanDoProcAmp				(info.deviceID);
	#else
		info.procAmpSupport		= false;
	#endif

}	//	SetVideoAttributes

#if !defined (NTV2_DEPRECATE)
	void CNTV2DeviceScanner::SetAudioAttributes(NTV2DeviceInfo & info, CNTV2Card & inBoard) const
	{
		//	Start with empty lists...
		info.audioSampleRateList.clear();
		info.audioNumChannelsList.clear();
		info.audioBitsPerSampleList.clear();
		info.audioInSourceList.clear();
		info.audioOutSourceList.clear();


		if (::NTV2DeviceCanDoAudioN(info.deviceID, 0))
		{
			ULWord audioControl;
			inBoard.ReadRegister(kRegAud1Control, &audioControl);

			//audioSampleRateList
			info.audioSampleRateList.push_back(k48KHzSampleRate);
			if (::NTV2DeviceCanDoAudio96K(info.deviceID))
				info.audioSampleRateList.push_back(k96KHzSampleRate);

			//audioBitsPerSampleList
			info.audioBitsPerSampleList.push_back(k32bitsPerSample);

			//audioInSourceList
			info.audioInSourceList.push_back(kSourceSDI);
			if (audioControl & BIT(21))
				info.audioInSourceList.push_back(kSourceAES);
			if (::NTV2DeviceCanDoAnalogAudio(info.deviceID))
				info.audioInSourceList.push_back(kSourceAnalog);

			//audioOutSourceList
			info.audioOutSourceList.push_back(kSourceAll);

			//audioNumChannelsList
			if (::NTV2DeviceCanDoAudio2Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels2);
			if (::NTV2DeviceCanDoAudio6Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels6);
			if (::NTV2DeviceCanDoAudio8Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels8);

			info.numAudioStreams = ::NTV2DeviceGetNumAudioStreams(info.deviceID);
		}

		info.numAnalogAudioInputChannels = ::NTV2DeviceGetNumAnalogAudioInputChannels(info.deviceID);
		info.numAESAudioInputChannels = ::NTV2DeviceGetNumAESAudioInputChannels(info.deviceID);
		info.numEmbeddedAudioInputChannels = ::NTV2DeviceGetNumEmbeddedAudioInputChannels(info.deviceID);
		info.numHDMIAudioInputChannels = ::NTV2DeviceGetNumHDMIAudioInputChannels(info.deviceID);
		info.numAnalogAudioOutputChannels = ::NTV2DeviceGetNumAnalogAudioOutputChannels(info.deviceID);
		info.numAESAudioOutputChannels = ::NTV2DeviceGetNumAESAudioOutputChannels(info.deviceID);
		info.numEmbeddedAudioOutputChannels = ::NTV2DeviceGetNumEmbeddedAudioOutputChannels(info.deviceID);
		info.numHDMIAudioOutputChannels = ::NTV2DeviceGetNumHDMIAudioOutputChannels(info.deviceID);
	}
	void CNTV2DeviceScanner::SetAudioAttributes (NTV2DeviceInfo & info, CNTV2Status & inBoard) const
	{	
		//	Start with empty lists...
		info.audioSampleRateList.clear();
		info.audioNumChannelsList.clear();
		info.audioBitsPerSampleList.clear();
		info.audioInSourceList.clear();
		info.audioOutSourceList.clear();


		if (::NTV2DeviceCanDoAudioN(info.deviceID, 0))
		{
			ULWord audioControl;
			inBoard.ReadRegister(kRegAud1Control, &audioControl);

			//audioSampleRateList
			info.audioSampleRateList.push_back(k48KHzSampleRate);
			if (::NTV2DeviceCanDoAudio96K(info.deviceID))
				info.audioSampleRateList.push_back(k96KHzSampleRate);

			//audioBitsPerSampleList
			info.audioBitsPerSampleList.push_back(k32bitsPerSample);

			//audioInSourceList
			info.audioInSourceList.push_back(kSourceSDI);
			if (audioControl & BIT(21))
				info.audioInSourceList.push_back(kSourceAES);
			if (::NTV2DeviceCanDoAnalogAudio(info.deviceID))
				info.audioInSourceList.push_back(kSourceAnalog);

			//audioOutSourceList
			info.audioOutSourceList.push_back(kSourceAll);

			//audioNumChannelsList
			if (::NTV2DeviceCanDoAudio2Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels2);
			if (::NTV2DeviceCanDoAudio6Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels6);
			if (::NTV2DeviceCanDoAudio8Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels8);

			info.numAudioStreams = ::NTV2DeviceGetNumAudioStreams(info.deviceID);
		}

		info.numAnalogAudioInputChannels = ::NTV2DeviceGetNumAnalogAudioInputChannels(info.deviceID);
		info.numAESAudioInputChannels = ::NTV2DeviceGetNumAESAudioInputChannels(info.deviceID);
		info.numEmbeddedAudioInputChannels = ::NTV2DeviceGetNumEmbeddedAudioInputChannels(info.deviceID);
		info.numHDMIAudioInputChannels = ::NTV2DeviceGetNumHDMIAudioInputChannels(info.deviceID);
		info.numAnalogAudioOutputChannels = ::NTV2DeviceGetNumAnalogAudioOutputChannels(info.deviceID);
		info.numAESAudioOutputChannels = ::NTV2DeviceGetNumAESAudioOutputChannels(info.deviceID);
		info.numEmbeddedAudioOutputChannels = ::NTV2DeviceGetNumEmbeddedAudioOutputChannels(info.deviceID);
		info.numHDMIAudioOutputChannels = ::NTV2DeviceGetNumHDMIAudioOutputChannels(info.deviceID);
	}	//	SetAudioAttributes
#else
	void CNTV2DeviceScanner::SetAudioAttributes(NTV2DeviceInfo & info, CNTV2Card & inBoard) const
	{
		//	Start with empty lists...
		info.audioSampleRateList.clear();
		info.audioNumChannelsList.clear();
		info.audioBitsPerSampleList.clear();
		info.audioInSourceList.clear();
		info.audioOutSourceList.clear();


		if (::NTV2DeviceGetNumAudioSystems(info.deviceID))
		{
			ULWord audioControl;
			inBoard.ReadRegister(kRegAud1Control, &audioControl);

			//audioSampleRateList
			info.audioSampleRateList.push_back(k48KHzSampleRate);
			if (::NTV2DeviceCanDoAudio96K(info.deviceID))
				info.audioSampleRateList.push_back(k96KHzSampleRate);

			//audioBitsPerSampleList
			info.audioBitsPerSampleList.push_back(k32bitsPerSample);

			//audioInSourceList
			info.audioInSourceList.push_back(kSourceSDI);
			if (audioControl & BIT(21))
				info.audioInSourceList.push_back(kSourceAES);
			if (::NTV2DeviceCanDoAnalogAudio(info.deviceID))
				info.audioInSourceList.push_back(kSourceAnalog);

			//audioOutSourceList
			info.audioOutSourceList.push_back(kSourceAll);

			//audioNumChannelsList
			if (::NTV2DeviceCanDoAudio2Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels2);
			if (::NTV2DeviceCanDoAudio6Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels6);
			if (::NTV2DeviceCanDoAudio8Channels(info.deviceID))
				info.audioNumChannelsList.push_back(kNumAudioChannels8);

			info.numAudioStreams = ::NTV2DeviceGetNumAudioSystems(info.deviceID);
		}

		info.numAnalogAudioInputChannels = ::NTV2DeviceGetNumAnalogAudioInputChannels(info.deviceID);
		info.numAESAudioInputChannels = ::NTV2DeviceGetNumAESAudioInputChannels(info.deviceID);
		info.numEmbeddedAudioInputChannels = ::NTV2DeviceGetNumEmbeddedAudioInputChannels(info.deviceID);
		info.numHDMIAudioInputChannels = ::NTV2DeviceGetNumHDMIAudioInputChannels(info.deviceID);
		info.numAnalogAudioOutputChannels = ::NTV2DeviceGetNumAnalogAudioOutputChannels(info.deviceID);
		info.numAESAudioOutputChannels = ::NTV2DeviceGetNumAESAudioOutputChannels(info.deviceID);
		info.numEmbeddedAudioOutputChannels = ::NTV2DeviceGetNumEmbeddedAudioOutputChannels(info.deviceID);
		info.numHDMIAudioOutputChannels = ::NTV2DeviceGetNumHDMIAudioOutputChannels(info.deviceID);

	}	//	SetAudioAttributes
#endif


//	Sort functor based on PCI slot number...
static bool gCompareSlot (const NTV2DeviceInfo & b1, const NTV2DeviceInfo & b2)
{
	return b1.deviceIndex < b2.deviceIndex;
}


//	Sort boards in boardInfoList
void CNTV2DeviceScanner::SortDeviceInfoList (void)
{
	std::sort (_deviceInfoList.begin (), _deviceInfoList.end (), gCompareSlot);
}


//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedVideoFormats (const NTV2DeviceID inDeviceID, NTV2VideoFormatSet & outFormats)
{
	bool	isOkay	(true);

	outFormats.clear ();

	for (unsigned formatIndex (1);  formatIndex < NTV2_FORMAT_END_HIGH_DEF_FORMATS2;  formatIndex++)
	{
		const NTV2VideoFormat	videoFormat	(static_cast <NTV2VideoFormat> (formatIndex));

		if (formatIndex == NTV2_FORMAT_END_HIGH_DEF_FORMATS)
			formatIndex = NTV2_FORMAT_FIRST_STANDARD_DEF_FORMAT - 1;
		else if (formatIndex == NTV2_FORMAT_END_STANDARD_DEF_FORMATS)
			formatIndex = NTV2_FORMAT_FIRST_2K_DEF_FORMAT - 1;
		else if (formatIndex == NTV2_FORMAT_END_2K_DEF_FORMATS)
			formatIndex = NTV2_FORMAT_FIRST_4K_DEF_FORMAT - 1;
		else if (formatIndex == NTV2_FORMAT_END_4K_DEF_FORMATS)
			formatIndex = NTV2_FORMAT_FIRST_HIGH_DEF_FORMAT2;
		else if (::NTV2DeviceCanDoVideoFormat (inDeviceID, videoFormat))
		{
			try
			{
				outFormats.insert (videoFormat);
			}
			catch (std::bad_alloc)
			{
				isOkay = false;
				outFormats.clear ();
				break;
			}
		}
	}	//	for each video format

	assert ((isOkay && !outFormats.empty () ) || (!isOkay && outFormats.empty () ));
	return isOkay;

}	//	NTV2DeviceGetSupportedVideoFormats


//	This needs to be moved into a C++ compatible "device features" module:
bool NTV2DeviceGetSupportedPixelFormats (const NTV2DeviceID inDeviceID, NTV2FrameBufferFormatSet & outFormats)
{
	bool	isOkay	(true);

	outFormats.clear ();

	for (unsigned formatIndex (1);  formatIndex < NTV2_FBF_NUMFRAMEBUFFERFORMATS;  formatIndex++)
	{
		const NTV2FrameBufferFormat	pixelFormat	(static_cast <NTV2FrameBufferFormat> (formatIndex));
		if (::NTV2DeviceCanDoFrameBufferFormat (inDeviceID, pixelFormat))
		{
			try
			{
				outFormats.insert (pixelFormat);
			}
			catch (std::bad_alloc)
			{
				isOkay = false;
				outFormats.clear ();
				break;
			}
		}
	}	//	for each pixel format

	assert ((isOkay && !outFormats.empty () ) || (!isOkay && outFormats.empty () ));
	return isOkay;

}	//	NTV2DeviceGetSupportedPixelFormats
