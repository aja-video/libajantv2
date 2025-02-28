/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2card.cpp
	@brief		Implements several CNTV2Card methods. Other implementation files are 'ntv2audio.cpp',
				'ntv2dma.cpp', 'ntv2register.cpp', ...
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2devicefeatures.h"
#include "ntv2card.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include <sstream>
#include "ajabase/common/common.h"
//#include "ajabase/system/info.h"	//	for AJASystemInfo

using namespace std;


// Default Constructor
CNTV2Card::CNTV2Card ()
	:	mDevCap(driverInterface())
{
	_boardOpened = false;
}

CNTV2Card::CNTV2Card (const UWord inDeviceIndex, const string & inHostName)
	:	mDevCap(driverInterface())
{
	string hostName(inHostName);
	aja::strip(hostName);
	_boardOpened = false;
	bool openOK = hostName.empty()	?  CNTV2DriverInterface::Open(inDeviceIndex) :	CNTV2DriverInterface::Open(hostName);
	if (openOK)
	{
		if (IsBufferSizeSetBySW())
		{
			NTV2Framesize fbSize;
			GetFrameBufferSize (NTV2_CHANNEL1, fbSize);
			SetFrameBufferSize (fbSize);
		}
		else
		{
			NTV2FrameGeometry fg;
			NTV2FrameBufferFormat format;

			GetFrameGeometry (fg);
			GetFrameBufferFormat (NTV2_CHANNEL1, format);

			_ulFrameBufferSize = ::NTV2DeviceGetFrameBufferSize (GetDeviceID (), fg, format);
			_ulNumFrameBuffers = ::NTV2DeviceGetNumberFrameBuffers (GetDeviceID (), fg, format);
		}
	}
}

// Destructor
CNTV2Card::~CNTV2Card ()
{
	if (IsOpen ())
		Close ();

}	//	destructor


Word CNTV2Card::GetDeviceVersion (void)
{
	ULWord	status	(0);
	return ReadRegister (kRegStatus, status) ? (status & 0xF) : -1;
}


string CNTV2Card::GetDeviceVersionString (void)
{
	ostringstream	oss;
	oss << ::NTV2DeviceIDToString(GetDeviceID());
	return oss.str();
}


string CNTV2Card::GetModelName (void)
{
	return ::NTV2DeviceIDToString(GetDeviceID(), /*retail?*/ GetDeviceID() == DEVICE_ID_IO4KPLUS
															&&	IsSupported(kDeviceHasMicrophoneInput));
}

string CNTV2Card::GetDisplayName (void)
{
	ostringstream oss;
	if (IsRemote())
	{
		auto params = CNTV2DriverInterface::ConnectParams();
		if (params.hasKey("displayname"))
		{
			oss << params.valueForKey("displayname");
		}
		else if (params.hasKey(kNTV2PluginRegInfoKey_LongName))
		{
			oss << params.valueForKey(kNTV2PluginRegInfoKey_LongName);
		}
		else if (params.hasKey(kNTV2PluginRegInfoKey_ShortName))
		{
			oss << params.valueForKey(kNTV2PluginRegInfoKey_ShortName);
		}
	}
	else
	{
		oss << GetModelName() << " - " << GetIndexNumber();
	}
	return oss.str();
}


string CNTV2Card::GetFPGAVersionString (const NTV2XilinxFPGA inFPGA)
{
	ULWord			numBytes	(0);
	string			dateStr, timeStr;
	ostringstream	oss;

	if (inFPGA == eFPGAVideoProc && GetInstalledBitfileInfo (numBytes, dateStr, timeStr))
		oss << dateStr << " at " << timeStr;
	else
		oss << "Unavailable";

	return oss.str ();
}


Word CNTV2Card::GetPCIFPGAVersion (void)
{
	ULWord	status	(0);
	return ReadRegister (48, status) ? ((status >> 8) & 0xFF) : -1;
}


string CNTV2Card::GetPCIFPGAVersionString (void)
{
	const UWord		version (static_cast<UWord>(GetPCIFPGAVersion()));
	ostringstream	oss;
	oss << hex << version;
	return oss.str ();
}

string CNTV2Card::GetDescription (void) const
{
	if (!IsOpen())
		return string();
	if (IsRemote())
		return CNTV2DriverInterface::GetDescription();

	CNTV2Card * pCard = (CNTV2Card*)(this);	//	Ugly: *sigh* If only most CNTV2Card member functions were 'const'
	ostringstream oss;
	string hostName("localhost"), snStr;
//	AJASystemInfo sysInfo (AJA_SystemInfoMemoryUnit_Megabytes, AJA_SystemInfoSection_System);
//	sysInfo.GetValue(AJA_SystemInfoTag_System_Name, hostName);
	oss << pCard->GetModelName();
	if (!pCard->GetSerialNumberString(snStr))
		snStr.clear();
	if (!snStr.empty())
		oss << " '" << snStr << "'";
	if (!hostName.empty())
		oss << " on '" << hostName << "'";
	oss << " at index " << DEC(GetIndexNumber());
	return oss.str();
}

string CNTV2Card::GetDriverVersionString (void)
{
	static const string sDriverBuildTypes []	= {"", "b", "a", "d"};
	UWord				versions[4]				= {0, 0, 0, 0};
	ULWord				versBits(0);
	if (!GetDriverVersionComponents (versions[0], versions[1], versions[2], versions[3]))
		return string();	//	fail
	if (!ReadRegister (kVRegDriverVersion, versBits))
		return string();	//	fail

	const string & dabr (sDriverBuildTypes[versBits >> 30]);	//	Bits 31:30 == build type
	ostringstream oss;
	oss << DEC(versions[0]) << "." << DEC(versions[1]) << "." << DEC(versions[2]);
	if (dabr.empty())
		oss << "." << DEC(versions[3]);
	else
		oss << dabr << DEC(versions[3]);
	return oss.str();

}	//	GetDriverVersionString


bool CNTV2Card::GetDriverVersionComponents (UWord & outMajor, UWord & outMinor, UWord & outPoint, UWord & outBuild)
{
	outMajor = outMinor = outPoint = outBuild = 0;
	ULWord	driverVersionULWord (0);
	if (!ReadRegister (kVRegDriverVersion, driverVersionULWord))
		return false;
	if (!driverVersionULWord)	//	If zero --- pre-15.0 driver?
		return false;

	//	The normal 15.0+ way of decoding the 32-bit driver version value:
	outMajor = UWord(NTV2DriverVersionDecode_Major(driverVersionULWord));
	outMinor = UWord(NTV2DriverVersionDecode_Minor(driverVersionULWord));
	outPoint = UWord(NTV2DriverVersionDecode_Point(driverVersionULWord));
	outBuild = UWord(NTV2DriverVersionDecode_Build(driverVersionULWord));
	return true;
}


ULWord CNTV2Card::GetSerialNumberLow (void)
{
	ULWord	serialNum	(0);
	return ReadRegister (kRegReserved54, serialNum) ? serialNum : 0;	//	Read EEPROM shadow of Serial Number
}


ULWord CNTV2Card::GetSerialNumberHigh (void)
{
	ULWord	serialNum	(0);
	return ReadRegister (kRegReserved55, serialNum) ? serialNum : 0;	//	Read EEPROM shadow of Serial Number
}

uint64_t CNTV2Card::GetSerialNumber (void)
{
	const uint64_t	lo(GetSerialNumberLow()),	hi(GetSerialNumberHigh());
	const uint64_t	result((hi << 32) | lo);
	return result;
}


string CNTV2Card::SerialNum64ToString (const uint64_t inSerialNumber)	//	Class method
{
	return ::SerialNum64ToString(inSerialNumber);
}	//	SerialNum64ToString


bool CNTV2Card::GetSerialNumberString (string & outSerialNumberString)
{
	ULWord spiFlashVers(0);
	if (!GetNumericParam(kDeviceGetSPIFlashVersion, spiFlashVers))
		return false;
    const NTV2DeviceID deviceID(GetDeviceID());
    bool hasLPPC = NTV2DeviceHasLPProductCode(deviceID);
    if (hasLPPC || spiFlashVers >= 6)
    {
        //	Newer devices use 4 regs:  kRegReserved54 thru kRegReserved57
        ULWord serialArray[] = {0,0,0,0};
        ReadRegister(hasLPPC ? kRegReserved54 : kRegReserved56, serialArray[0]);
        ReadRegister(hasLPPC ? kRegReserved55 : kRegReserved57, serialArray[1]);
        ReadRegister(hasLPPC ? kRegReserved56 : kRegReserved54, serialArray[2]);
        ReadRegister(hasLPPC ? kRegReserved57 : kRegReserved55, serialArray[3]);
        outSerialNumberString.clear();
        for (int serialIndex = 0;  serialIndex < 4;  serialIndex++)
            if (serialArray[serialIndex] != 0xffffffff)
                for (int i = 0;  i < 4;  i++)
                {
                    const char tempChar(((serialArray[serialIndex] >> (i*8)) & 0xff));
                    if (tempChar > 0 && tempChar != '.')
                        outSerialNumberString.push_back(tempChar);
                }
    }
	else
	{	//	Older devices use 2 regs:  kRegReserved54 & kRegReserved55
		outSerialNumberString = ::SerialNum64ToString(GetSerialNumber());
		if (outSerialNumberString.empty())
			{outSerialNumberString = "INVALID?";  return false;}

		//	Prepend 5/6/7 for Io4K+/IoIP/IoX3, respectively...
		if (deviceID == DEVICE_ID_IO4KPLUS)
			outSerialNumberString = "5" + outSerialNumberString;	//	Io4K+/DNXIV: prepend "5"
		else if (deviceID == DEVICE_ID_IOIP_2022 ||
                 deviceID == DEVICE_ID_IOIP_2110 ||
                 deviceID == DEVICE_ID_IOIP_2110_RGB12)
			outSerialNumberString = "6" + outSerialNumberString;	//	IoIP: prepend "6"
		else if (deviceID == DEVICE_ID_IOX3)
			outSerialNumberString = "7" + outSerialNumberString;	//	IoX3: prepend "7"
	}
	return true;
}	//	GetSerialNumberString


bool CNTV2Card::IS_CHANNEL_INVALID (const NTV2Channel inChannel) const
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return true;
	return false;
}


bool CNTV2Card::IS_OUTPUT_SPIGOT_INVALID (const UWord inOutputSpigot)
{
	if (inOutputSpigot >= UWord(GetNumSupported(kDeviceGetNumVideoOutputs)))
	{
		const ULWordSet itms (GetSupportedItems(kNTV2EnumsID_WidgetID));
		if (itms.find(ULWord(NTV2_WgtSDIMonOut1)) != itms.end()  &&  inOutputSpigot == 4)
			return false;	//	Io4K Monitor Output exception
		return true;		//	Invalid
	}
	return false;
}


bool CNTV2Card::IS_INPUT_SPIGOT_INVALID (const UWord inInputSpigot)
{
	if (inInputSpigot >= UWord(GetNumSupported(kDeviceGetNumVideoInputs)))
		return true;
	return false;
}

bool CNTV2Card::IS_HDMI_INPUT_SPIGOT_INVALID (const UWord inInputHDMIPort)
{
	if (inInputHDMIPort >= UWord(GetNumSupported(kDeviceGetNumHDMIVideoInputs)))
		return true;
	return false;
}

ULWord CNTV2Card::DeviceGetNumberFrameBuffers (void)
{
	if (IsSupported(kDeviceCanDoStackedAudio))
	{
		ULWord totalFrames = GetNumSupported(kDeviceGetActiveMemorySize) / 0x800000;
		totalFrames -= GetNumSupported(kDeviceGetNumAudioSystems) * (IsSupported(kDeviceCanDo12gRouting) ? 4 : 1);
		if (_boardID == DEVICE_ID_IOX3  ||  _boardID == DEVICE_ID_KONA5_8K_MV_TX)
			totalFrames -= 6;   //  Lop off 48MB for HDMI MultiViewer
		return totalFrames;
	}
	return ::NTV2DeviceGetNumberFrameBuffers(_boardID); //  Handle non-stacked-audio devices
}

bool CNTV2Card::GetInstalledBitfileInfo (ULWord & outNumBytes, std::string & outDateStr, std::string & outTimeStr)
{
	outDateStr.clear ();
	outTimeStr.clear ();
	outNumBytes = 0;

	if (!_boardOpened)
		return false;	//	Bail if I'm not open

	BITFILE_INFO_STRUCT		bitFileInfo;
	::memset (&bitFileInfo, 0, sizeof (bitFileInfo));
	bitFileInfo.whichFPGA = eFPGAVideoProc;

	//	Call the OS specific method...
	if (!DriverGetBitFileInformation (bitFileInfo, NTV2_VideoProcBitFile))
		return false;

	//	Fill in our OS independent data structure...
	outDateStr = reinterpret_cast <char *> (&bitFileInfo.dateStr [0]);
	outTimeStr = reinterpret_cast <char *> (&bitFileInfo.timeStr [0]);
	outNumBytes = bitFileInfo.numBytes;
	return true;
}

string CNTV2Card::GetBitfileInfoString (const BITFILE_INFO_STRUCT & inBitFileInfo)
{
	ostringstream oss;
	// format like: "date time name"
	oss << inBitFileInfo.dateStr << " " << inBitFileInfo.timeStr << " ";
	if (NTV2BitfileType(inBitFileInfo.bitFileType) == NTV2_BITFILE_IO4KPLUS_MAIN  &&  IsSupported(kDeviceHasMicrophoneInput))
		oss << "DNxIV";
	else
		oss << ::NTV2BitfileTypeToString(NTV2BitfileType(inBitFileInfo.bitFileType), true);
	return oss.str();
}

bool CNTV2Card::IsFailSafeBitfileLoaded (bool & outIsSafeBoot)
{
	outIsSafeBoot = false;
	if (!IsSupported(kDeviceCanReportFailSafeLoaded))
		return false;
	return CNTV2DriverInterface::ReadRegister(kRegCPLDVersion, outIsSafeBoot, BIT(4), 4);
}


bool CNTV2Card::CanWarmBootFPGA (bool & outCanWarmBoot)
{
	outCanWarmBoot = false; //	Definitely can't
	if (!IsSupported(kDeviceCanDoWarmBootFPGA))
		return false;

	ULWord	version(0);
	if (!ReadRegister(kRegCPLDVersion, version, BIT(0)|BIT(1)))
		return false;	//	Fail
	if (version != 3)
		outCanWarmBoot = true;	//	Definitely can
	return true;
}


NTV2BreakoutType CNTV2Card::GetBreakoutHardware (void)
{
	NTV2BreakoutType	result		(NTV2_BreakoutNone);
	ULWord				audioCtlReg (0);	//	The Audio Control Register tells us what's connected

	if (IsOpen ()  &&  ReadRegister (kRegAud1Control, audioCtlReg))
	{
		const bool	bPhonyKBox	(false);	//	For debugging

		switch (_boardID)
		{
			case DEVICE_ID_KONA3G:
			case DEVICE_ID_KONA3GQUAD:
			case DEVICE_ID_IO4K:
			case DEVICE_ID_KONA4:
			case DEVICE_ID_KONA4UFC:
			case DEVICE_ID_KONA5:
			case DEVICE_ID_KONA5_8KMK:
			case DEVICE_ID_KONA5_8K:
			case DEVICE_ID_KONA5_2X4K:
			case DEVICE_ID_KONA5_3DLUT:
			case DEVICE_ID_KONA5_OE1:
			case DEVICE_ID_KONA5_8K_MV_TX:
				//	Do we have a K3G-Box?
				if ((audioCtlReg & kK2RegMaskKBoxDetect) || bPhonyKBox)
					result = NTV2_K3GBox;
				else
					result = NTV2_BreakoutCableBNC;
				break;
			case DEVICE_ID_KONALHEPLUS:
				//	Do we have a KL-Box?
				if ((audioCtlReg & kK2RegMaskKBoxDetect) || bPhonyKBox)
					result = NTV2_KLBox;
				else
					result = NTV2_BreakoutCableXLR;		// no BNC breakout cable available
				break;
			case DEVICE_ID_KONALHI:
				//	Do we have a KLHi-Box?
				if ((audioCtlReg & kK2RegMaskKBoxDetect) || bPhonyKBox)
					result = NTV2_KLHiBox;
				else
					result = NTV2_BreakoutCableXLR;		// no BNC breakout cable available
				break;
			case DEVICE_ID_KONAX:
				// Do we have a BOB?
				if(IsBreakoutBoardConnected())
					result = NTV2_BreakoutBoard;
				break;
			default:
				break;
		}
	}
	return result;
}

bool CNTV2Card::GetLPExternalPortURLString (string & outURLString)
{
	if (!NTV2DeviceHasLPProductCode(GetDeviceID()))
		return false;
	uint32_t portIP(0);
	int ipOctet(0);
	ostringstream tempString;
	ReadRegister(kRegLPRJ45IP, portIP);
	if (portIP == 0)
		return false;
	
	outURLString.clear();
	tempString << "http://";
	ipOctet = (portIP & 0xFF000000) >> 24;
	tempString << ipOctet << ".";
	ipOctet = (portIP & 0x00FF0000) >> 16;
	tempString << ipOctet << ".";
	ipOctet = (portIP & 0x0000FF00) >> 8;
	tempString << ipOctet << ".";
	ipOctet = (portIP & 0x000000FF);
	tempString << ipOctet;
	
	outURLString = tempString.str();
	return true;
}

bool CNTV2Card::GetLPTunnelPortURLString (string & outURLString)
{
	if (!NTV2DeviceHasLPProductCode(GetDeviceID()))
		return false;
	uint32_t portIP(0);
	int ipOctet(0);
	ostringstream tempString;
	ReadRegister(kRegLPTunnelIP, portIP);
	if (portIP == 0)
		return false;
	
	outURLString.clear();
	tempString << "http://";
	ipOctet = (portIP & 0xFF000000) >> 24;
	tempString << ipOctet << ".";
	ipOctet = (portIP & 0x00FF0000) >> 16;
	tempString << ipOctet << ".";
	ipOctet = (portIP & 0x0000FF00) >> 8;
	tempString << ipOctet << ".";
	ipOctet = (portIP & 0x000000FF);
	tempString << ipOctet;
	
	outURLString = tempString.str();
	return true;
}

int CNTV2Card::GetSFPURLs(std::vector<std::string> & OutSFPURLStrings)
{
	if (!NTV2DeviceHasLPProductCode(GetDeviceID()) || NTV2DeviceGetNum25GSFPs(GetDeviceID()) == 0)
		return 0;
	
	UWord numSFPs = NTV2DeviceGetNum25GSFPs(GetDeviceID());
	
	for (int i = 0; i < numSFPs; i++)
	{
		uint32_t portIP(0);
		int ipOctet(0);
		ostringstream tempString;
		ReadRegister(kRegLPSFP1IP+i, portIP);
		if (portIP == 0)
			return false;
		
		tempString << "http://";
		ipOctet = (portIP & 0xFF000000) >> 24;
		tempString << ipOctet << ".";
		ipOctet = (portIP & 0x00FF0000) >> 16;
		tempString << ipOctet << ".";
		ipOctet = (portIP & 0x0000FF00) >> 8;
		tempString << ipOctet << ".";
		ipOctet = (portIP & 0x000000FF);
		tempString << ipOctet;
		
		OutSFPURLStrings.push_back(tempString.str());
	}
	return OutSFPURLStrings.size();
}

#if !defined(NTV2_DEPRECATE_16_3)
//////////	Device Features
	bool CNTV2Card::DeviceCanDoFormat (NTV2FrameRate		inFrameRate,
										NTV2FrameGeometry	inFrameGeometry, 
										NTV2Standard		inStandard)
	{
		return ::NTV2DeviceCanDoFormat (GetDeviceID(), inFrameRate, inFrameGeometry, inStandard);
	}

	ULWord	CNTV2Card::DeviceGetFrameBufferSize (void)
	{
		return ::NTV2DeviceGetFrameBufferSize (GetDeviceID());		//	Revisit for 2MB granularity
	}

	ULWord	CNTV2Card::DeviceGetAudioFrameBuffer (void)
	{
		return ::NTV2DeviceGetAudioFrameBuffer (GetDeviceID());		//	Revisit for 2MB granularity
	}

	ULWord	CNTV2Card::DeviceGetAudioFrameBuffer2 (void)
	{
		return ::NTV2DeviceGetAudioFrameBuffer2 (GetDeviceID());		//	Revisit for 2MB granularity
	}

	ULWord	CNTV2Card::DeviceGetFrameBufferSize (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat)
	{
		return ::NTV2DeviceGetFrameBufferSize (GetDeviceID(), inFrameGeometry, inFBFormat); //	Revisit for 2MB granularity
	}

	ULWord	CNTV2Card::DeviceGetNumberFrameBuffers (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat)
	{
		return ::NTV2DeviceGetNumberFrameBuffers (GetDeviceID(), inFrameGeometry, inFBFormat);	//	Revisit for 2MB granularity
	}

	ULWord	CNTV2Card::DeviceGetAudioFrameBuffer (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat)
	{
		return ::NTV2DeviceGetAudioFrameBuffer (GetDeviceID(), inFrameGeometry, inFBFormat);	//	Revisit for 2MB granularity
	}

	ULWord	CNTV2Card::DeviceGetAudioFrameBuffer2 (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat)
	{
		return ::NTV2DeviceGetAudioFrameBuffer2 (GetDeviceID(), inFrameGeometry, inFBFormat);	//	Revisit for 2MB granularity
	}

	bool CNTV2Card::DeviceCanDoVideoFormat (const NTV2VideoFormat inVideoFormat)
	{	const ULWordSet itms (GetSupportedItems(kNTV2EnumsID_VideoFormat));
		return itms.find(ULWord(inVideoFormat)) != itms.end();
	}

	bool CNTV2Card::DeviceCanDoFrameBufferFormat (const NTV2FrameBufferFormat inFBFormat)
	{	const ULWordSet itms (GetSupportedItems(kNTV2EnumsID_PixelFormat));
		return itms.find(ULWord(inFBFormat)) != itms.end();
	}

	bool CNTV2Card::DeviceCanDoWidget (const NTV2WidgetID inWidgetID)
	{
		const ULWordSet wgtIDs (GetSupportedItems(kNTV2EnumsID_WidgetID));
		return wgtIDs.find(inWidgetID) != wgtIDs.end();
	}
	
	bool CNTV2Card::DeviceCanDoConversionMode (const NTV2ConversionMode inConversionMode)
	{	const ULWordSet itms (GetSupportedItems(kNTV2EnumsID_ConversionMode));
		return itms.find(ULWord(inConversionMode)) != itms.end();
	}

	bool CNTV2Card::DeviceCanDoDSKMode (const NTV2DSKMode inDSKMode)
	{	const ULWordSet itms (GetSupportedItems(kNTV2EnumsID_DSKMode));
		return itms.find(ULWord(inDSKMode)) != itms.end();
	}

	bool CNTV2Card::DeviceCanDoInputSource (const NTV2InputSource inInputSource)
	{	const ULWordSet itms (GetSupportedItems(kNTV2EnumsID_InputSource));
		return itms.find(ULWord(inInputSource)) != itms.end();
	}
#endif	//	!defined(NTV2_DEPRECATE_16_3)


NTV2Buffer CNTV2Card::NULL_POINTER (AJA_NULL, 0);


bool DeviceCapabilities::CanDoWidget (const NTV2WidgetType inWgtType, const UWord index0)
{
	return CanDoWidget(CNTV2SignalRouter::WidgetIDFromTypeAndChannel(inWgtType, NTV2Channel(index0)));
}


///////////////////////////////////////////////////////////////////////////////////////////////	SDRAMAuditor


bool SDRAMAuditor::AssessDevice (CNTV2Card & inDevice, const bool inMarkStoppedAudioBuffersFree)
{
	mFrameTags.clear();
	mDeviceID = DEVICE_ID_INVALID;
	mNumFrames = 0;
	mIntrinsicSize = 0;
	if (!inDevice.IsOpen())
		return false;

	mDeviceID = inDevice.GetDeviceID();
	const ULWord totalBytes(::NTV2DeviceGetActiveMemorySize(mDeviceID));
	mNumFrames = UWord(totalBytes / m8MB);
	if (totalBytes % m8MB)
		{mNumFrames++;  cerr << DEC(totalBytes % m8MB) << " leftover/spare bytes -- last frame is partial frame" << endl;}
	for (UWord frm(0);  frm < mNumFrames;  frm++)
		mFrameTags.insert(FrameTag(frm, NTV2StringSet()));

	return TagAudioBuffers(inDevice, inMarkStoppedAudioBuffersFree) && TagVideoFrames(inDevice);
}

ostream & SDRAMAuditor::RawDump (ostream & oss) const
{
	for (FrameTagsConstIter it(mFrameTags.begin());  it != mFrameTags.end();  ++it)
	{
		const NTV2StringSet & tags(it->second);
		oss << DEC0N(it->first,3) << ": " << aja::join(tags, ", ") << endl;
	}
	return oss;
}

ULWordSet SDRAMAuditor::CoalesceRegions (const ULWordSequence & inRgn1, const ULWordSequence & inRgn2, const ULWordSequence & inRgn3)
{
	ULWordSet result;	//	Coalesce all regions into this one sorted set
	for (size_t ndx(0);  ndx < inRgn1.size();  ndx++)
		if (result.find(inRgn1.at(ndx)) == result.end())
			result.insert(inRgn1.at(ndx));
	for (size_t ndx(0);  ndx < inRgn2.size();  ndx++)
		if (result.find(inRgn2.at(ndx)) == result.end())
			result.insert(inRgn2.at(ndx));
	for (size_t ndx(0);  ndx < inRgn3.size();  ndx++)
		if (result.find(inRgn3.at(ndx)) == result.end())
			result.insert(inRgn3.at(ndx));
	return result;
}

ostream & SDRAMAuditor::DumpBlocks (ostream & oss) const
{
	ULWordSequence badBlks, freeBlks, goodBlks;
	GetRegions (freeBlks, goodBlks, badBlks);
	ULWordSet rgns (CoalesceRegions(freeBlks, goodBlks, badBlks));	//	Coalesce all regions into this one sorted set

	for (ULWordSetConstIter it(rgns.begin());  it != rgns.end();  ++it)
	{
		const ULWord rgnInfo(*it);
		const UWord startBlk(rgnInfo >> 16), numBlks(UWord(rgnInfo & 0x0000FFFF));
		NTV2StringSet tags;
		GetTagsForFrameIndex (startBlk, tags);
		if (numBlks > 1)
			oss << "Frms " << DEC0N(startBlk,3) << "-" << DEC0N(startBlk+numBlks-1,3) << " : ";
		else
			oss << "Frm  " << DEC0N(startBlk,3) << "     : ";
		if (tags.empty())
			oss << "{free}";
		else
			oss << aja::join(tags, ", ");
		oss << endl;
	}
	return oss;
}

bool SDRAMAuditor::GetRegions (ULWordSequence & outFree, ULWordSequence & outUsed, ULWordSequence & outBad) const
{
	outFree.clear();  outUsed.clear();  outBad.clear();
	FrameTagsConstIter it(mFrameTags.begin());
	if (it == mFrameTags.end())
		return true;
	UWord frmStart(it->first), lastFrm(frmStart);
	NTV2StringSet runTags(it->second);
	while (++it != mFrameTags.end())
	{
		const NTV2StringSet & tags(it->second);
		if (tags != runTags)
		{	//	End of current run, start of new run
			if (runTags.empty())
			{
				if (frmStart != lastFrm)
					outFree.push_back((ULWord(frmStart) << 16) | ULWord(lastFrm-frmStart+1));
				else
					outFree.push_back((ULWord(frmStart) << 16) | ULWord(1));
			}
			else if (runTags.size() > 1)
			{
				if (frmStart != lastFrm)
					outBad.push_back((ULWord(frmStart) << 16) | ULWord(lastFrm-frmStart+1));
				else
					outBad.push_back((ULWord(frmStart) << 16) | ULWord(1));
			}
			else
			{
				if (frmStart != lastFrm)
					outUsed.push_back((ULWord(frmStart) << 16) | ULWord(lastFrm-frmStart+1));
				else
					outUsed.push_back((ULWord(frmStart) << 16) | ULWord(1));
			}
			frmStart = lastFrm = it->first;
			runTags = tags;
		}
		else
			lastFrm = it->first;	//	Continue current run
	}
	if (runTags.empty())
	{
		if (frmStart != lastFrm)
			outFree.push_back((ULWord(frmStart) << 16) | ULWord(lastFrm-frmStart+1));
		else
			outFree.push_back((ULWord(frmStart) << 16) | ULWord(1));
	}
	else if (runTags.size() > 1)
	{
		if (frmStart != lastFrm)
			outBad.push_back((ULWord(frmStart) << 16) | ULWord(lastFrm-frmStart+1));
		else
			outBad.push_back((ULWord(frmStart) << 16) | ULWord(1));
	}
	else
	{
		if (frmStart != lastFrm)
			outUsed.push_back((ULWord(frmStart) << 16) | ULWord(lastFrm-frmStart+1));
		else
			outUsed.push_back((ULWord(frmStart) << 16) | ULWord(1));
	}
	return true;
}

bool SDRAMAuditor::GetTagsForFrameIndex (const UWord inIndex, NTV2StringSet & outTags) const
{
	outTags.clear();
	FrameTagsConstIter it(mFrameTags.find(inIndex));
	if (it == mFrameTags.end())
		return false;
	outTags = it->second;
	return true;
}

size_t SDRAMAuditor::GetTagCount (const UWord inIndex) const
{
	FrameTagsConstIter it(mFrameTags.find(inIndex));
	if (it == mFrameTags.end())
		return 0;
	return it->second.size();
}

bool SDRAMAuditor::TranslateRegions (ULWordSequence & outDestRgns, const ULWordSequence & inSrcRgns, const bool inIsQuad, const bool inIsQuadQuad) const
{
	outDestRgns.clear();
	if (inIsQuad && inIsQuadQuad)
		return false;	//	Can't be both
	if (inSrcRgns.empty())
		return true;	//	Empty list, not an error
	const UWord	_8MB_frames_per_dest_frame(UWord(GetIntrinsicFrameByteCount() / m8MB) * (inIsQuad?4:1) * (inIsQuadQuad?16:1));	//	Should result in 1/4/16 or 2/8/32
	if (!_8MB_frames_per_dest_frame)
		return false;	//	Ordinarily won't happen, but possible with "ntv2:" (fake/software) "devices" having small SDRAM complement
	if (_8MB_frames_per_dest_frame == 1)
		{outDestRgns = inSrcRgns;	return true;}	//	Same

	//	For each region...
	for (size_t ndx(0);  ndx < inSrcRgns.size();  ndx++)
	{	const ULWord val(inSrcRgns.at(ndx));
		ULWord startBlkOffset(val >> 16), lengthBlks(val & 0x0000FFFF);		//	<== These are in 8MB block units
		startBlkOffset = startBlkOffset / _8MB_frames_per_dest_frame  +  (startBlkOffset % _8MB_frames_per_dest_frame ? 1 : 0);
		lengthBlks = lengthBlks / _8MB_frames_per_dest_frame;
		outDestRgns.push_back((startBlkOffset << 16) | lengthBlks);
	}
	return true;
}

bool SDRAMAuditor::TagAudioBuffers (CNTV2Card & inDevice, const bool inMarkStoppedAudioBuffersFree)
{
	ULWord addr(0);
	bool isReading(false), isWriting(false);
	const UWord	numAudSystems(UWord(inDevice.GetNumSupported(kDeviceGetNumBufferedAudioSystems)));
	for (NTV2AudioSystem audSys(NTV2_AUDIOSYSTEM_1);  audSys < NTV2AudioSystem(numAudSystems);  audSys = NTV2AudioSystem(audSys+1))
		if (inDevice.GetAudioMemoryOffset (0,  addr,  audSys))
		{	ostringstream tag;
			tag << "Aud" << DEC(audSys+1);
			if (inDevice.IsAudioOutputRunning(audSys, isReading)  &&  isReading)
				tag << " Read";
			if (inDevice.IsAudioInputRunning(audSys, isWriting)  &&  isWriting)
				tag << " Write";
			TagMemoryBlock(addr, m8MB, inMarkStoppedAudioBuffersFree && !isReading && !isWriting ? string() : tag.str());
		}
	return true;
}

bool SDRAMAuditor::TagVideoFrames (CNTV2Card & inDevice)
{
	const UWord numChannels	(UWord(inDevice.GetNumSupported(kDeviceGetNumVideoChannels)) + (inDevice.HasMultiRasterWidget() ? 1 : 0));
	NTV2ChannelSet skipChannels;
	for (NTV2Channel chan(NTV2_CHANNEL1);  chan < NTV2Channel(numChannels);  chan = NTV2Channel(chan+1))
	{
		AUTOCIRCULATE_STATUS acStatus;
		bool isEnabled(false), isMultiFormat(false), isQuad(false), isQuadQuad(false), isSquares(false), isTSI(false);
		ostringstream tag;
		uint64_t addr(0), len(0);
		if (skipChannels.find(chan) != skipChannels.end())
			continue;	//	Skip this channel/framestore
		if (inDevice.AutoCirculateGetStatus (chan, acStatus)  &&  !acStatus.IsStopped())
		{
			uint64_t tmp(0);
			inDevice.GetDeviceFrameInfo(acStatus.GetStartFrame(), chan, mIntrinsicSize, isMultiFormat, isQuad, isQuadQuad, isSquares, isTSI, addr, tmp);
			inDevice.GetDeviceFrameInfo(acStatus.GetEndFrame(), chan, tmp, len);
			tag << "AC" << DEC(chan+1) << (acStatus.IsInput() ? " Write" : " Read");
			TagMemoryBlock(addr, tmp + len - addr, tag.str());
		}	//	if GetStatus succeeded
		else if (inDevice.IsChannelEnabled(chan, isEnabled)  &&  isEnabled)
		{
			NTV2Mode mode(NTV2_MODE_INVALID);
			inDevice.GetMode(chan, mode);
			ULWord frameNum(0);
			if (NTV2_IS_INPUT_MODE(mode))
				inDevice.GetInputFrame(chan, frameNum);
			else
				inDevice.GetOutputFrame(chan, frameNum);
			inDevice.GetDeviceFrameInfo (UWord(frameNum),  chan,  mIntrinsicSize, isMultiFormat, isQuad, isQuadQuad, isSquares, isTSI, addr,  len);
			if (inDevice.IsMultiRasterWidgetChannel(chan))
				tag << "MR" << DEC(chan+1);	//	MultiRaster Viewer
			else
				tag << "Ch" << DEC(chan+1);
			tag << (NTV2_IS_INPUT_MODE(mode) ? " Write" : " Read");
			TagMemoryBlock(addr, len, tag.str());
		}
		if (isSquares && chan == NTV2_CHANNEL1)
			{skipChannels.insert(NTV2_CHANNEL2); skipChannels.insert(NTV2_CHANNEL3); skipChannels.insert(NTV2_CHANNEL4);}
		else if (isSquares && chan == NTV2_CHANNEL5)
			{skipChannels.insert(NTV2_CHANNEL6); skipChannels.insert(NTV2_CHANNEL7); skipChannels.insert(NTV2_CHANNEL8);}
		else if (isQuad  &&  !isQuadQuad  &&  isTSI)
		{
			if (chan == NTV2_CHANNEL1)
				skipChannels.insert(NTV2_CHANNEL2);
			else if (chan == NTV2_CHANNEL3)
				skipChannels.insert(NTV2_CHANNEL4);
			else if (chan == NTV2_CHANNEL5)
				skipChannels.insert(NTV2_CHANNEL6);
			else if (chan == NTV2_CHANNEL7)
				skipChannels.insert(NTV2_CHANNEL8);
		}
	}	//	for each device channel
	if (!mIntrinsicSize)
	{
		NTV2Framesize frmsz(NTV2_FRAMESIZE_8MB);
		inDevice.GetFrameBufferSize(NTV2_CHANNEL1, frmsz);
		mIntrinsicSize = ::NTV2FramesizeToByteCount(frmsz);
	}
	return true;
}

bool SDRAMAuditor::TagMemoryBlock (const ULWord inStartAddr, const ULWord inByteLength, const string & inTag)
{
	if (inStartAddr % m8MB)
		return false;
	if (inByteLength % m8MB)
		return false;
	if (inTag.empty())
		return false;
	const UWord startFrm(UWord(inStartAddr / m8MB)),  frmCnt(UWord(inByteLength / m8MB));
	for (UWord frm(0);  frm < frmCnt;  frm++)
	{
		UWord frameNum(startFrm + frm);
		NTV2StringSet & tags(mFrameTags[frameNum]);
		if (tags.find(inTag) == tags.end())
		{
			tags.insert(inTag);
			if (frameNum >= mNumFrames)
				tags.insert("Invalid");
		}
	}
	return true;
}
