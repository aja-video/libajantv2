/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2card.cpp
	@brief		Implements several CNTV2Card methods. Other implementation files are 'ntv2audio.cpp', 'ntv2dma.cpp', 'ntv2register.cpp', ...
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2devicefeatures.h"
#include "ntv2card.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include <sstream>
#include "ajabase/common/common.h"

using namespace std;


// Default Constructor
CNTV2Card::CNTV2Card ()
{
	_boardOpened = false;

	#if defined (NTV2_DEPRECATE)
		//InitNTV2ColorCorrection ();
	#endif	//	defined (NTV2_DEPRECATE)
}

CNTV2Card::CNTV2Card (const UWord inDeviceIndex, const string & inHostName)
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

#if !defined(NTV2_DEPRECATE_14_3)
CNTV2Card::CNTV2Card (const UWord boardNumber, const bool displayErrorMessage, const UWord ulBoardType, const char *hostname)
{
	(void) displayErrorMessage;
	(void) ulBoardType;
	string hostName(hostname ? hostname : "");
	aja::strip(hostName);
	_boardOpened = false;
	bool openOK = hostName.empty()	?  CNTV2DriverInterface::Open(boardNumber) :  CNTV2DriverInterface::Open(hostName);
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
}	//	constructor
#endif	//	!defined(NTV2_DEPRECATE_14_3)

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
	ostringstream	oss;
	oss << ::NTV2DeviceIDToString(GetDeviceID(), GetDeviceID() == DEVICE_ID_IO4KPLUS ? DeviceHasMicInput() : false);
	return oss.str();
}

string CNTV2Card::GetDisplayName (void)
{
	ostringstream	oss;
	oss << GetModelName() << " - " << GetIndexNumber();
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


bool CNTV2Card::IS_CHANNEL_INVALID (const NTV2Channel inChannel) const
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return true;
	return false;
}


bool CNTV2Card::IS_OUTPUT_SPIGOT_INVALID (const UWord inOutputSpigot) const
{
	if (inOutputSpigot >= ::NTV2DeviceGetNumVideoOutputs(_boardID))
	{
		if (NTV2DeviceCanDoWidget(_boardID, NTV2_WgtSDIMonOut1) && inOutputSpigot == 4)
			return false;	//	Io4K Monitor Output exception
		return true;		//	Invalid
	}
	return false;
}


bool CNTV2Card::IS_INPUT_SPIGOT_INVALID (const UWord inInputSpigot) const
{
	if (inInputSpigot >= ::NTV2DeviceGetNumVideoInputs (_boardID))
		return true;
	return false;
}


uint64_t CNTV2Card::GetSerialNumber (void)
{
	const uint64_t	lo(GetSerialNumberLow()),	hi(GetSerialNumberHigh());
	const uint64_t	result((hi << 32) | lo);
	return result;
}


string CNTV2Card::SerialNum64ToString (const uint64_t inSerialNumber)	//	Class method
{
	const ULWord	serialNumHigh	(inSerialNumber >> 32);
	const ULWord	serialNumLow	(inSerialNumber & 0x00000000FFFFFFFF);
	char			serialNum [9];

	serialNum[0] = char((serialNumLow  & 0x000000FF)	  );
	serialNum[1] = char((serialNumLow  & 0x0000FF00) >>	 8);
	serialNum[2] = char((serialNumLow  & 0x00FF0000) >> 16);
	serialNum[3] = char((serialNumLow  & 0xFF000000) >> 24);
	serialNum[4] = char((serialNumHigh & 0x000000FF)	  );
	serialNum[5] = char((serialNumHigh & 0x0000FF00) >>	 8);
	serialNum[6] = char((serialNumHigh & 0x00FF0000) >> 16);
	serialNum[7] = char((serialNumHigh & 0xFF000000) >> 24);
	serialNum[8] = '\0';

	for (unsigned ndx(0);  ndx < 8;	 ndx++)
	{
		if (serialNum[ndx] == 0)
		{
			if (ndx == 0)
				return "";		//	No characters: no serial number
			break;	//	End of string -- stop scanning
		}

		//	Allow only 0-9, A-Z, a-z, blank, and dash only.
		if ( ! ( ( (serialNum[ndx] >= '0') && (serialNum[ndx] <= '9') ) ||
				 ( (serialNum[ndx] >= 'A') && (serialNum[ndx] <= 'Z') ) ||
				 ( (serialNum[ndx] >= 'a') && (serialNum[ndx] <= 'z') ) ||
				   (serialNum[ndx] == ' ') || (serialNum[ndx] == '-') ) )
			return "";		//	Invalid character -- assume no Serial Number programmed...
	}
	return serialNum;

}	//	SerialNum64ToString


bool CNTV2Card::GetSerialNumberString (string & outSerialNumberString)
{
	outSerialNumberString = SerialNum64ToString(GetSerialNumber());
	if (outSerialNumberString.empty())
	{
		outSerialNumberString = "INVALID?";
		return false;
	}

	const NTV2DeviceID deviceID(GetDeviceID());
	if (deviceID == DEVICE_ID_IO4KPLUS)							//	Io4K+/DNxIV?
		outSerialNumberString = "5" + outSerialNumberString;	//		prepend with "5"
	else if (deviceID == DEVICE_ID_IOIP_2022 ||
			 deviceID == DEVICE_ID_IOIP_2110 ||
			 deviceID == DEVICE_ID_IOIP_2110_RGB12)				//	IoIP/DNxIP?
		outSerialNumberString = "6" + outSerialNumberString;	//		prepend with "6"
	else if (deviceID == DEVICE_ID_IOX3)
		outSerialNumberString = "7" + outSerialNumberString;	//		prepend with "7"
	return true;

}	//	GetSerialNumberString


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
	if (NTV2BitfileType(inBitFileInfo.bitFileType) == NTV2_BITFILE_IO4KPLUS_MAIN  &&  DeviceIsDNxIV())
		oss << "DNxIV";
	else
		oss << ::NTV2BitfileTypeToString(NTV2BitfileType(inBitFileInfo.bitFileType), true);
	return oss.str();
}

bool CNTV2Card::IsFailSafeBitfileLoaded (bool & outIsSafeBoot)
{
	outIsSafeBoot = false;
	if (!::NTV2DeviceCanReportFailSafeLoaded(_boardID))
		return false;
	return CNTV2DriverInterface::ReadRegister(kRegCPLDVersion, outIsSafeBoot, BIT(4), 4);
}


bool CNTV2Card::CanWarmBootFPGA (bool & outCanWarmBoot)
{
	outCanWarmBoot = false; //	Definitely can't
	if (!::NTV2DeviceCanDoWarmBootFPGA(_boardID))
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
			default:
				break;
		}
	}
	return result;
}


//////////	Device Features

bool CNTV2Card::DeviceCanDoFormat (NTV2FrameRate		inFrameRate,
									NTV2FrameGeometry	inFrameGeometry, 
									NTV2Standard		inStandard)
{
	return ::NTV2DeviceCanDoFormat (GetDeviceID(), inFrameRate, inFrameGeometry, inStandard);
}

bool CNTV2Card::DeviceCanDo3GOut (UWord index0)
{
	return ::NTV2DeviceCanDo3GOut (GetDeviceID(), index0);
}

bool CNTV2Card::DeviceCanDoLTCEmbeddedN (UWord index0)
{
	return ::NTV2DeviceCanDoLTCEmbeddedN (GetDeviceID(), index0);
}

ULWord	CNTV2Card::DeviceGetFrameBufferSize (void)
{
	return ::NTV2DeviceGetFrameBufferSize (GetDeviceID());		//	Revisit for 2MB granularity
}

ULWord	CNTV2Card::DeviceGetNumberFrameBuffers (void)
{
	return ::NTV2DeviceGetNumberFrameBuffers (GetDeviceID());		//	Revisit for 2MB granularity
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
{
	return ::NTV2DeviceCanDoVideoFormat (GetDeviceID(), inVideoFormat);
}

bool CNTV2Card::DeviceCanDoFrameBufferFormat (const NTV2FrameBufferFormat inFBFormat)
{
	return ::NTV2DeviceCanDoFrameBufferFormat (GetDeviceID(), inFBFormat);
}


bool CNTV2Card::DeviceCanDoWidget (const NTV2WidgetID inWidgetID)
{
	return ::NTV2DeviceCanDoWidget (GetDeviceID(), inWidgetID);
}

bool CNTV2Card::DeviceCanDoConversionMode (const NTV2ConversionMode inConversionMode)
{
	return ::NTV2DeviceCanDoConversionMode (GetDeviceID(), inConversionMode);
}

bool CNTV2Card::DeviceCanDoDSKMode (const NTV2DSKMode inDSKMode)
{
	return ::NTV2DeviceCanDoDSKMode (GetDeviceID(), inDSKMode);
}

bool CNTV2Card::DeviceCanDoInputSource (const NTV2InputSource inInputSource)
{
	return ::NTV2DeviceCanDoInputSource (GetDeviceID(), inInputSource);
}

bool CNTV2Card::DeviceCanDoAudioMixer ()
{
	ULWord isMixerSupported = 0;
	ReadRegister(kRegGlobalControl2, isMixerSupported, BIT(18), 18);
	if(isMixerSupported == 1)
		return true;
	return false;
}

bool CNTV2Card::DeviceCanDoHDMIQuadRasterConversion ()
{
	NTV2DeviceID deviceID = GetDeviceID();

	if ((NTV2DeviceGetNumHDMIVideoInputs(deviceID) == 0) &&
			(NTV2DeviceGetNumHDMIVideoOutputs(deviceID) == 0))
		return false;

	if (deviceID == DEVICE_ID_KONAHDMI)
		return false;

	if (DeviceCanDoAudioMixer())
		return false;

	return true;
}

bool CNTV2Card::DeviceIsDNxIV ()
{
	ULWord isMicSupported = 0;
	ReadRegister(kRegGlobalControl2, isMicSupported, BIT(19), 19);
	if(isMicSupported == 1)
		return true;
	return false;
}

bool CNTV2Card::DeviceHasMicInput ()
{
	ULWord isMicSupported = 0;
	ReadRegister(kRegGlobalControl2, isMicSupported, BIT(19), 19);
	if(isMicSupported == 1)
		return true;
	return false;
}

bool CNTV2Card::GetBoolParam (const NTV2BoolParamID inParamID, bool & outValue)
{
	uint32_t	regValue	(0);
	NTV2RegInfo regInfo;

	outValue = false;
	if (GetRegInfoForBoolParam (inParamID, regInfo))
	{
		if (!ReadRegister (regInfo.registerNumber, regValue, regInfo.registerMask, regInfo.registerShift))
			return false;
		outValue = static_cast <bool> (regValue != 0);
		return true;
	}

	//	Call old device features function...
	switch (inParamID)
	{
//		case kDeviceCanChangeEmbeddedAudioClock:		outValue = ::NTV2DeviceCanChangeEmbeddedAudioClock		(GetDeviceID());	break;
		case kDeviceCanChangeFrameBufferSize:			outValue = ::NTV2DeviceCanChangeFrameBufferSize			(GetDeviceID());	break;
		case kDeviceCanDisableUFC:						outValue = ::NTV2DeviceCanDisableUFC					(GetDeviceID());	break;
		case kDeviceCanDo2KVideo:						outValue = ::NTV2DeviceCanDo2KVideo						(GetDeviceID());	break;
		case kDeviceCanDo3GLevelConversion:				outValue = ::NTV2DeviceCanDo3GLevelConversion			(GetDeviceID());	break;
		case kDeviceCanDoRGBLevelAConversion:			outValue = ::NTV2DeviceCanDoRGBLevelAConversion			(GetDeviceID());	break;
		case kDeviceCanDo425Mux:						outValue = ::NTV2DeviceCanDo425Mux						(GetDeviceID());	break;
		case kDeviceCanDo4KVideo:						outValue = ::NTV2DeviceCanDo4KVideo						(GetDeviceID());	break;
		case kDeviceCanDoAESAudioIn:					outValue = ::NTV2DeviceCanDoAESAudioIn					(GetDeviceID());	break;
		case kDeviceCanDoAnalogAudio:					outValue = ::NTV2DeviceCanDoAnalogAudio					(GetDeviceID());	break;
		case kDeviceCanDoAnalogVideoIn:					outValue = ::NTV2DeviceCanDoAnalogVideoIn				(GetDeviceID());	break;
		case kDeviceCanDoAnalogVideoOut:				outValue = ::NTV2DeviceCanDoAnalogVideoOut				(GetDeviceID());	break;
//		case kDeviceCanDoAudio2Channels:				outValue = ::NTV2DeviceCanDoAudio2Channels				(GetDeviceID());	break;
//		case kDeviceCanDoAudio6Channels:				outValue = ::NTV2DeviceCanDoAudio6Channels				(GetDeviceID());	break;
//		case kDeviceCanDoAudio8Channels:				outValue = ::NTV2DeviceCanDoAudio8Channels				(GetDeviceID());	break;
//		case kDeviceCanDoAudio96K:						outValue = ::NTV2DeviceCanDoAudio96K					(GetDeviceID());	break;
//		case kDeviceCanDoAudioDelay:					outValue = ::NTV2DeviceCanDoAudioDelay					(GetDeviceID());	break;
		case kDeviceCanDoBreakoutBox:					outValue = ::NTV2DeviceCanDoBreakoutBox					(GetDeviceID());	break;
		case kDeviceCanDoCapture:						outValue = ::NTV2DeviceCanDoCapture						(GetDeviceID());	break;
//		case kDeviceCanDoColorCorrection:				outValue = ::NTV2DeviceCanDoColorCorrection				(GetDeviceID());	break;
//		case kDeviceCanDoCustomAnc:						outValue = ::NTV2DeviceCanDoCustomAnc					(GetDeviceID());	break;
//		case kDeviceCanDoDSKOpacity:					outValue = ::NTV2DeviceCanDoDSKOpacity					(GetDeviceID());	break;
//		case kDeviceCanDoDualLink:						outValue = ::NTV2DeviceCanDoDualLink					(GetDeviceID());	break;
//		case kDeviceCanDoDVCProHD:						outValue = ::NTV2DeviceCanDoDVCProHD					(GetDeviceID());	break;
//		case kDeviceCanDoEnhancedCSC:					outValue = ::NTV2DeviceCanDoEnhancedCSC					(GetDeviceID());	break;
//		case kDeviceCanDoFrameStore1Display:			outValue = ::NTV2DeviceCanDoFrameStore1Display			(GetDeviceID());	break;
//		case kDeviceCanDoFreezeOutput:					outValue = ::NTV2DeviceCanDoFreezeOutput				(GetDeviceID());	break;
//		case kDeviceCanDoHDMIOutStereo:					outValue = ::NTV2DeviceCanDoHDMIOutStereo				(GetDeviceID());	break;
//		case kDeviceCanDoHDV:							outValue = ::NTV2DeviceCanDoHDV							(GetDeviceID());	break;
//		case kDeviceCanDoHDVideo:						outValue = ::NTV2DeviceCanDoHDVideo						(GetDeviceID());	break;
		case kDeviceCanDoIsoConvert:					outValue = ::NTV2DeviceCanDoIsoConvert					(GetDeviceID());	break;
		case kDeviceCanDoLTC:							outValue = ::NTV2DeviceCanDoLTC							(GetDeviceID());	break;
		case kDeviceCanDoLTCInOnRefPort:				outValue = ::NTV2DeviceCanDoLTCInOnRefPort				(GetDeviceID());	break;
		case kDeviceCanDoMSI:							outValue = ::NTV2DeviceCanDoMSI							(GetDeviceID());	break;
		case kDeviceCanDoMultiFormat:					outValue = ::NTV2DeviceCanDoMultiFormat					(GetDeviceID());	break;
		case kDeviceCanDoPCMControl:					outValue = ::NTV2DeviceCanDoPCMControl					(GetDeviceID());	break;
		case kDeviceCanDoPCMDetection:					outValue = ::NTV2DeviceCanDoPCMDetection				(GetDeviceID());	break;
//		case kDeviceCanDoPIO:							outValue = ::NTV2DeviceCanDoPIO							(GetDeviceID());	break;
		case kDeviceCanDoPlayback:						outValue = ::NTV2DeviceCanDoPlayback					(GetDeviceID());	break;
		case kDeviceCanDoProgrammableCSC:				outValue = ::NTV2DeviceCanDoProgrammableCSC				(GetDeviceID());	break;
		case kDeviceCanDoProgrammableRS422:				outValue = ::NTV2DeviceCanDoProgrammableRS422			(GetDeviceID());	break;
		case kDeviceCanDoProRes:						outValue = ::NTV2DeviceCanDoProRes						(GetDeviceID());	break;
		case kDeviceCanDoQREZ:							outValue = ::NTV2DeviceCanDoQREZ						(GetDeviceID());	break;
		case kDeviceCanDoQuarterExpand:					outValue = ::NTV2DeviceCanDoQuarterExpand				(GetDeviceID());	break;
//		case kDeviceCanDoRateConvert:					outValue = ::NTV2DeviceCanDoRateConvert					(GetDeviceID());	break;
//		case kDeviceCanDoRGBPlusAlphaOut:				outValue = ::NTV2DeviceCanDoRGBPlusAlphaOut				(GetDeviceID());	break;
//		case kDeviceCanDoRP188:							outValue = ::NTV2DeviceCanDoRP188						(GetDeviceID());	break;
//		case kDeviceCanDoSDVideo:						outValue = ::NTV2DeviceCanDoSDVideo						(GetDeviceID());	break;
		case kDeviceCanDoSDIErrorChecks:				outValue = ::NTV2DeviceCanDoSDIErrorChecks				(GetDeviceID());	break;
//		case kDeviceCanDoStackedAudio:					outValue = ::NTV2DeviceCanDoStackedAudio				(GetDeviceID());	break;
//		case kDeviceCanDoStereoIn:						outValue = ::NTV2DeviceCanDoStereoIn					(GetDeviceID());	break;
//		case kDeviceCanDoStereoOut:						outValue = ::NTV2DeviceCanDoStereoOut					(GetDeviceID());	break;
		case kDeviceCanDoThunderbolt:					outValue = ::NTV2DeviceCanDoThunderbolt					(GetDeviceID());	break;
		case kDeviceCanDoVideoProcessing:				outValue = ::NTV2DeviceCanDoVideoProcessing				(GetDeviceID());	break;
		case kDeviceCanMeasureTemperature:				outValue = ::NTV2DeviceCanMeasureTemperature			(GetDeviceID());	break;
		case kDeviceCanReportFrameSize:					outValue = ::NTV2DeviceCanReportFrameSize				(GetDeviceID());	break;
		case kDeviceHasBiDirectionalSDI:				outValue = ::NTV2DeviceHasBiDirectionalSDI				(GetDeviceID());	break;
//		case kDeviceHasColorSpaceConverterOnChannel2:	outValue = ::NTV2DeviceHasColorSpaceConverterOnChannel2 (GetDeviceID());	break;
		case kDeviceHasNWL:								outValue = ::NTV2DeviceHasNWL							(GetDeviceID());	break;
		case kDeviceHasPCIeGen2:						outValue = ::NTV2DeviceHasPCIeGen2						(GetDeviceID());	break;
		case kDeviceHasRetailSupport:					outValue = ::NTV2DeviceHasRetailSupport					(GetDeviceID());	break;
		case kDeviceHasSDIRelays:						outValue = ::NTV2DeviceHasSDIRelays						(GetDeviceID());	break;
//		case kDeviceHasSPIFlash:						outValue = ::NTV2DeviceHasSPIFlash						(GetDeviceID());	break;
//		case kDeviceHasSPIFlashSerial:					outValue = ::NTV2DeviceHasSPIFlashSerial				(GetDeviceID());	break;
		case kDeviceHasSPIv2:							outValue = ::NTV2DeviceHasSPIv2							(GetDeviceID());	break;
		case kDeviceHasSPIv3:							outValue = ::NTV2DeviceHasSPIv3							(GetDeviceID());	break;
		case kDeviceHasSPIv4:							outValue = ::NTV2DeviceHasSPIv4							(GetDeviceID());	break;
//		case kDeviceIs64Bit:							outValue = ::NTV2DeviceIs64Bit							(GetDeviceID());	break;
//		case kDeviceIsDirectAddressable:				outValue = ::NTV2DeviceIsDirectAddressable				(GetDeviceID());	break;
		case kDeviceIsExternalToHost:					outValue = ::NTV2DeviceIsExternalToHost					(GetDeviceID());	break;
		case kDeviceIsSupported:						outValue = ::NTV2DeviceIsSupported						(GetDeviceID());	break;
//		case kDeviceNeedsRoutingSetup:					outValue = ::NTV2DeviceNeedsRoutingSetup				(GetDeviceID());	break;
		case kDeviceSoftwareCanChangeFrameBufferSize:	outValue = ::NTV2DeviceSoftwareCanChangeFrameBufferSize (GetDeviceID());	break;
		case kDeviceCanThermostat:						outValue = ::NTV2DeviceCanThermostat					(GetDeviceID());	break;
		case kDeviceHasHEVCM31:							outValue = ::NTV2DeviceHasHEVCM31						(GetDeviceID());	break;
		case kDeviceHasHEVCM30:							outValue = ::NTV2DeviceHasHEVCM30						(GetDeviceID());	break;
		case kDeviceCanDoVITC2:							outValue = ::NTV2DeviceCanDoVITC2						(GetDeviceID());	break;
		case kDeviceCanDoHDMIHDROut:					outValue = ::NTV2DeviceCanDoHDMIHDROut					(GetDeviceID());	break;
		case kDeviceCanDoJ2K:							outValue = ::NTV2DeviceCanDoJ2K							(GetDeviceID());	break;
		default:										return false;	//	Bad param
	}
	return true;	//	Successfully used old ::NTV2DeviceCanDo function

}	//	GetBoolParam


bool CNTV2Card::GetNumericParam (const NTV2NumericParamID inParamID, uint32_t & outValue)
{
	uint32_t	regValue	(0);
	NTV2RegInfo regInfo;

	outValue = false;
	if (GetRegInfoForNumericParam (inParamID, regInfo))
	{
		if (!ReadRegister (regInfo.registerNumber, regValue, regInfo.registerMask, regInfo.registerShift))
			return false;
		outValue = static_cast <bool> (regValue != 0);
		return true;
	}

	//	Call old device features function...
	switch (inParamID)
	{
		case kDeviceGetActiveMemorySize:				outValue = ::NTV2DeviceGetActiveMemorySize					(GetDeviceID());	break;
		case kDeviceGetDACVersion:						outValue = ::NTV2DeviceGetDACVersion						(GetDeviceID());	break;
		case kDeviceGetDownConverterDelay:				outValue = ::NTV2DeviceGetDownConverterDelay				(GetDeviceID());	break;
		case kDeviceGetHDMIVersion:						outValue = ::NTV2DeviceGetHDMIVersion						(GetDeviceID());	break;
		case kDeviceGetLUTVersion:						outValue = ::NTV2DeviceGetLUTVersion						(GetDeviceID());	break;
		case kDeviceGetMaxAudioChannels:				outValue = ::NTV2DeviceGetMaxAudioChannels					(GetDeviceID());	break;
		case kDeviceGetMaxRegisterNumber:				outValue = ::NTV2DeviceGetMaxRegisterNumber					(GetDeviceID());	break;
		case kDeviceGetMaxTransferCount:				outValue = ::NTV2DeviceGetMaxTransferCount					(GetDeviceID());	break;
		case kDeviceGetNumDMAEngines:					outValue = ::NTV2DeviceGetNumDMAEngines						(GetDeviceID());	break;
		case kDeviceGetNumVideoChannels:				outValue = ::NTV2DeviceGetNumVideoChannels					(GetDeviceID());	break;
		case kDeviceGetPingLED:							outValue = ::NTV2DeviceGetPingLED							(GetDeviceID());	break;
		case kDeviceGetUFCVersion:						outValue = ::NTV2DeviceGetUFCVersion						(GetDeviceID());	break;
		case kDeviceGetNum4kQuarterSizeConverters:		outValue = ::NTV2DeviceGetNum4kQuarterSizeConverters		(GetDeviceID());	break;
		case kDeviceGetNumAESAudioInputChannels:		outValue = ::NTV2DeviceGetNumAESAudioInputChannels			(GetDeviceID());	break;
		case kDeviceGetNumAESAudioOutputChannels:		outValue = ::NTV2DeviceGetNumAESAudioOutputChannels			(GetDeviceID());	break;
		case kDeviceGetNumAnalogAudioInputChannels:		outValue = ::NTV2DeviceGetNumAnalogAudioInputChannels		(GetDeviceID());	break;
		case kDeviceGetNumAnalogAudioOutputChannels:	outValue = ::NTV2DeviceGetNumAnalogAudioOutputChannels		(GetDeviceID());	break;
		case kDeviceGetNumAnalogVideoInputs:			outValue = ::NTV2DeviceGetNumAnalogVideoInputs				(GetDeviceID());	break;
		case kDeviceGetNumAnalogVideoOutputs:			outValue = ::NTV2DeviceGetNumAnalogVideoOutputs				(GetDeviceID());	break;
		case kDeviceGetNumAudioSystems:					outValue = (::NTV2DeviceGetNumAudioSystems					(GetDeviceID()) + (DeviceCanDoAudioMixer() ? 1 : 0));	break;
		case kDeviceGetNumCrossConverters:				outValue = ::NTV2DeviceGetNumCrossConverters				(GetDeviceID());	break;
		case kDeviceGetNumCSCs:							outValue = ::NTV2DeviceGetNumCSCs							(GetDeviceID());	break;
		case kDeviceGetNumDownConverters:				outValue = ::NTV2DeviceGetNumDownConverters					(GetDeviceID());	break;
		case kDeviceGetNumEmbeddedAudioInputChannels:	outValue = ::NTV2DeviceGetNumEmbeddedAudioInputChannels		(GetDeviceID());	break;
		case kDeviceGetNumEmbeddedAudioOutputChannels:	outValue = ::NTV2DeviceGetNumEmbeddedAudioOutputChannels	(GetDeviceID());	break;
		case kDeviceGetNumFrameStores:					outValue = ::NTV2DeviceGetNumFrameStores					(GetDeviceID());	break;
		case kDeviceGetNumFrameSyncs:					outValue = ::NTV2DeviceGetNumFrameSyncs						(GetDeviceID());	break;
		case kDeviceGetNumHDMIAudioInputChannels:		outValue = ::NTV2DeviceGetNumHDMIAudioInputChannels			(GetDeviceID());	break;
		case kDeviceGetNumHDMIAudioOutputChannels:		outValue = ::NTV2DeviceGetNumHDMIAudioOutputChannels		(GetDeviceID());	break;
		case kDeviceGetNumHDMIVideoInputs:				outValue = ::NTV2DeviceGetNumHDMIVideoInputs				(GetDeviceID());	break;
		case kDeviceGetNumHDMIVideoOutputs:				outValue = ::NTV2DeviceGetNumHDMIVideoOutputs				(GetDeviceID());	break;
		case kDeviceGetNumInputConverters:				outValue = ::NTV2DeviceGetNumInputConverters				(GetDeviceID());	break;
		case kDeviceGetNumLUTs:							outValue = ::NTV2DeviceGetNumLUTs							(GetDeviceID());	break;
		case kDeviceGetNumMixers:						outValue = ::NTV2DeviceGetNumMixers							(GetDeviceID());	break;
		case kDeviceGetNumOutputConverters:				outValue = ::NTV2DeviceGetNumOutputConverters				(GetDeviceID());	break;
		case kDeviceGetNumReferenceVideoInputs:			outValue = ::NTV2DeviceGetNumReferenceVideoInputs			(GetDeviceID());	break;
		case kDeviceGetNumSerialPorts:					outValue = ::NTV2DeviceGetNumSerialPorts					(GetDeviceID());	break;
		case kDeviceGetNumUpConverters:					outValue = ::NTV2DeviceGetNumUpConverters					(GetDeviceID());	break;
		case kDeviceGetNumVideoInputs:					outValue = ::NTV2DeviceGetNumVideoInputs					(GetDeviceID());	break;
		case kDeviceGetNumVideoOutputs:					outValue = ::NTV2DeviceGetNumVideoOutputs					(GetDeviceID());	break;
		case kDeviceGetNum2022ChannelsSFP1:				outValue = ::NTV2DeviceGetNum2022ChannelsSFP1				(GetDeviceID());	break;
		case kDeviceGetNum2022ChannelsSFP2:				outValue = ::NTV2DeviceGetNum2022ChannelsSFP2				(GetDeviceID());	break;
		case kDeviceGetNumLTCInputs:					outValue = ::NTV2DeviceGetNumLTCInputs						(GetDeviceID());	break;
		case kDeviceGetNumLTCOutputs:					outValue = ::NTV2DeviceGetNumLTCOutputs						(GetDeviceID());	break;
		default:										return false;	//	Bad param
	}
	return true;	//	Successfully used old ::NTV2DeviceGetNum function

}	//	GetNumericParam


bool CNTV2Card::GetRegInfoForBoolParam (const NTV2BoolParamID inParamID, NTV2RegInfo & outRegInfo)
{
	(void) inParamID;
	outRegInfo.MakeInvalid();
	return false;	//	Needs implementation
}


bool CNTV2Card::GetRegInfoForNumericParam (const NTV2NumericParamID inParamID, NTV2RegInfo & outRegInfo)
{
	(void) inParamID;
	outRegInfo.MakeInvalid();
	return false;	//	Needs implementation
}


/////////// Stream Operators

ostream & operator << (ostream & inOutStr, const NTV2AudioChannelPairs & inSet)
{
	if (inSet.empty())
		inOutStr << "(none)";
	else
		for (NTV2AudioChannelPairsConstIter iter (inSet.begin ());	iter != inSet.end ();  ++iter)
			inOutStr << (iter != inSet.begin() ? ", " : "") << ::NTV2AudioChannelPairToString (*iter, true);
	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioChannelQuads & inSet)
{
	for (NTV2AudioChannelQuadsConstIter iter (inSet.begin ());	iter != inSet.end ();  ++iter)
		inOutStr << (iter != inSet.begin () ? ", " : "") << ::NTV2AudioChannelQuadToString (*iter, true);
	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioChannelOctets & inSet)
{
	for (NTV2AudioChannelOctetsConstIter iter (inSet.begin ());	 iter != inSet.end ();	++iter)
		inOutStr << (iter != inSet.begin () ? ", " : "") << ::NTV2AudioChannelOctetToString (*iter, true);
	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2DoubleArray & inVector)
{
	for (NTV2DoubleArrayConstIter iter (inVector.begin ());	 iter != inVector.end ();  ++iter)
		inOutStr << *iter << endl;
	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2DIDSet & inDIDs)
{
	for (NTV2DIDSetConstIter it (inDIDs.begin());  it != inDIDs.end();	)
	{
		inOutStr << xHEX0N(uint16_t(*it),2);
		if (++it != inDIDs.end())
			inOutStr << ", ";
	}
	return inOutStr;
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::GetBitFileInformation (ULWord & outNumBytes, string & outDateStr, string & outTimeStr, const NTV2XilinxFPGA inFPGA)
	{
		return inFPGA == eFPGAVideoProc ? GetInstalledBitfileInfo (outNumBytes, outDateStr, outTimeStr) : false;
	}

	Word CNTV2Card::GetFPGAVersion (const NTV2XilinxFPGA inFPGA)
	{
		(void) inFPGA;
		return -1;
	}

	UWord CNTV2Card::GetNumNTV2Boards()
	{
		ULWord numBoards(0);
		CNTV2Card ntv2Card;
		for (ULWord boardCount(0);	boardCount < NTV2_MAXBOARDS;  boardCount++)
		{
			if (AsNTV2DriverInterfaceRef(ntv2Card).Open(boardCount))
				numBoards++;	//	Opened, keep going
			else
				break;	//	Failed to open, we're done
		}
		return numBoards;
	}

	NTV2BoardType CNTV2Card::GetBoardType (void) const
	{
		return DEVICETYPE_NTV2;
	}

	NTV2BoardSubType CNTV2Card::GetBoardSubType (void)
	{
		return BOARDSUBTYPE_NONE;
	}

	bool CNTV2Card::SetBoard (UWord inDeviceIndexNumber)
	{
		return CNTV2DriverInterface::Open(inDeviceIndexNumber);
	}

	string CNTV2Card::GetBoardIDString (void)
	{
		const ULWord	boardID (static_cast <ULWord> (GetDeviceID ()));
		ostringstream	oss;
		oss << hex << boardID;
		return oss.str ();
	}

	void CNTV2Card::GetBoardIDString(std::string & outString)
	{
		outString = GetBoardIDString();
	}

#endif	//	!defined (NTV2_DEPRECATE)


NTV2_POINTER CNTV2Card::NULL_POINTER (AJA_NULL, 0);


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

SDRAMAuditor::ULWordSet SDRAMAuditor::CoalesceRegions (const ULWordSequence & inRgn1, const ULWordSequence & inRgn2, const ULWordSequence & inRgn3)
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
	if (_8MB_frames_per_dest_frame == 1)
		{outDestRgns = inSrcRgns;	return true;}	//	Same

	//	For each region...
	for (size_t ndx(0);  ndx < inSrcRgns.size();  ndx++)
	{	const ULWord val(inSrcRgns.at(ndx));
		UWord startBlkOffset(val >> 16), lengthBlks(UWord(val & 0x0000FFFF));	//	<== These are in 8MB block units
		startBlkOffset = startBlkOffset / _8MB_frames_per_dest_frame  +  (startBlkOffset % _8MB_frames_per_dest_frame ? 1 : 0);
		lengthBlks = lengthBlks / _8MB_frames_per_dest_frame;
		outDestRgns.push_back((ULWord(startBlkOffset) << 16) | ULWord(lengthBlks));
	}
	return true;
}

bool SDRAMAuditor::TagAudioBuffers (CNTV2Card & inDevice, const bool inMarkStoppedAudioBuffersFree)
{
	ULWord addr(0);
	bool isReading(false), isWriting(false);
	const UWord	maxNumAudioSystems(::NTV2DeviceGetNumAudioSystems(mDeviceID) + (inDevice.DeviceCanDoAudioMixer() ? 1 : 0));
	for (NTV2AudioSystem audSys(NTV2_AUDIOSYSTEM_1);  audSys < NTV2AudioSystem(maxNumAudioSystems);  audSys = NTV2AudioSystem(audSys+1))
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
	const UWord numChannels	(UWord(::NTV2DeviceGetNumVideoChannels(mDeviceID)) + (inDevice.HasMultiRasterWidget() ? 1 : 0));
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
