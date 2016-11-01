/**
	@file		ntv2card.cpp
	@brief		Partially implements the CNTV2Card class. Other implementation files are 'ntv2audio.cpp', 'ntv2dma.cpp', and 'ntv2register.cpp'.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2devicefeatures.h"
#include "ntv2card.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include <sstream>

using namespace std;


// Default Constructor
CNTV2Card::CNTV2Card ()
{
	_boardOpened = false;

    #if defined (NTV2_DEPRECATE)
        //InitNTV2ColorCorrection ();
		InitNTV2TestPattern ();
	#endif	//	defined (NTV2_DEPRECATE)
}

// Constructor that Opens Board
CNTV2Card::CNTV2Card (const UWord boardNumber, const bool displayErrorMessage, const UWord ulBoardType, const char *hostname)
{
	_boardOpened = false;
	const NTV2DeviceType	eUseBoardType	(static_cast <NTV2DeviceType> (ulBoardType));

	if (Open (boardNumber, displayErrorMessage, eUseBoardType, hostname))
	{
		if (IsBufferSizeSetBySW ())
		{
			NTV2Framesize fbSize;
			GetFrameBufferSize (NTV2_CHANNEL1, &fbSize);
			SetFrameBufferSize (fbSize);
		}
		else
		{
			NTV2FrameGeometry fg;
			NTV2FrameBufferFormat format;

			GetFrameGeometry (&fg);
			GetFrameBufferFormat (NTV2_CHANNEL1, &format);

			_ulFrameBufferSize = ::NTV2DeviceGetFrameBufferSize (GetDeviceID (), fg, format);
			_ulNumFrameBuffers = ::NTV2DeviceGetNumberFrameBuffers (GetDeviceID (), fg, format);
		}
	}

    #if defined (NTV2_DEPRECATE)
        //InitNTV2ColorCorrection ();
		InitNTV2TestPattern ();
	#endif	//	defined (NTV2_DEPRECATE)
 }	//	constructor


// Destructor
CNTV2Card::~CNTV2Card ()
{
#if 0
	#if defined (NTV2_DEPRECATE)
		FreeNTV2ColorCorrection ();
	#endif	//	defined (NTV2_DEPRECATE)
#endif
	if (IsOpen ())
		Close ();

}	//	destructor


NTV2DeviceID CNTV2Card::GetDeviceID (void)
{
	ULWord	value	(0);
	if (_boardOpened && ReadRegister (kRegBoardID, &value))
	{
		const NTV2DeviceID	currentValue (static_cast <NTV2DeviceID> (value));
		if (currentValue != _boardID)
			cerr	<< "## WARNING:  GetDeviceID:  0x" << hex << this << dec << ":  NTV2DeviceID " << ::NTV2DeviceIDToString (currentValue)
					<< " read from register " << kRegBoardID << " doesn't match _boardID " << ::NTV2DeviceIDToString (_boardID) << endl;
		return currentValue;
	}
	else
		return DEVICE_ID_NOTFOUND;
}


Word CNTV2Card::GetDeviceVersion (void)
{
	ULWord	status	(0);
	return ReadRegister (kRegStatus, &status) ? (status & 0xF) : -1;
}


string CNTV2Card::GetDeviceVersionString (void)
{
	ostringstream	oss;
	oss << ::NTV2DeviceIDToString (GetDeviceID ());
	return oss.str ();
}


string CNTV2Card::GetDisplayName (void)
{
	ostringstream	oss;
	oss << ::NTV2DeviceIDToString (GetDeviceID ()) << " - " << GetIndexNumber ();
	return oss.str ();
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
	return ReadRegister (48, &status) ? ((status >> 8) & 0xFF) : -1;
}


string CNTV2Card::GetPCIFPGAVersionString (void)
{
	const UWord		version	(GetPCIFPGAVersion ());
	ostringstream	oss;
	oss << hex << version;
	return oss.str ();
}


string CNTV2Card::GetDriverVersionString (void)
{
	stringstream	oss;

	#if defined (MSWindows)
		// Bits 3-0		minor version
		// Bits 7-4		major version
		// Bits 11-8	point version
		// Bit	12		reserved
		// Bit	13		64 bit flag
		// Bit  14		beta flag
		// Bit  15		debug flag
		// Bits	31-16	build version
		ULWord			versionInfo	(0);
		GetDriverVersion (&versionInfo);

		const ULWord	ulMajor	((versionInfo >>  4) & 0xf);
		const ULWord	ulMinor	((versionInfo >>  0) & 0xf);
		const ULWord	ulPoint	((versionInfo >>  8) & 0xf);
		const ULWord	ulBuild	((versionInfo >> 16) & 0xffff);

		if ((ulMajor < 6) ||
			((ulMajor == 6) && (ulMinor < 5)) ||
			((ulMajor == 6) && (ulMinor == 5) && (ulBuild == 0)))
		{
			oss	<< dec << ulMajor
				<< "." << dec << ulMinor
				<< ((versionInfo & (BIT_14)) ? " beta " : ".") << dec << ulPoint
				<< ((versionInfo & (BIT_13)) ? " 64bit" : "")
				<< ((versionInfo & (BIT_15)) ? " debug" : "");
		}
		else
			oss << dec << ulMajor
				<< "." << dec << ulMinor
				<< "." << dec << ulPoint
				<< ((versionInfo & (BIT_14)) ? " beta " : " build ") << dec << ulBuild
				<< ((versionInfo & (BIT_13)) ? " 64bit" : "")
				<< ((versionInfo & (BIT_15)) ? " debug" : "");

	#elif defined (AJALinux)

		// Bit  15     Debug
		// Bit  14     Beta Version flag (Bits 13-8 interpreted as "beta x"
		// Bits 13-8   sub-minor Version (or beta version #)
		// Bits 7-4    Major Version  - new hardware types to support, etc.
		// Bits 3-0    Minor Version  
		ULWord			versionInfo	(0);
		GetDriverVersion (&versionInfo);

		oss	<< dec << ((versionInfo >> 4) & 0xF) << "." << dec << (versionInfo & 0xF)	//	Major and Minor version
			<< "." << dec << ((versionInfo >> 8) & 0x3F);								//	Point version

		if (versionInfo & 0xFFFF000)
			oss << "." << dec << ((versionInfo >> 16) & 0xFFFF);						//	Build Version, if present

		if (versionInfo & (BIT_14))
			oss << " Beta";																//	Beta Version

		if (versionInfo & BIT_15)
			oss << " Debug";															//	Debug Version

	#elif defined (AJAMac)

		NumVersion	version	=	{0, 0, 0, 0};
		CNTV2MacDriverInterface::GetDriverVersion (&version);
		oss	<< uint32_t (version.majorRev) << "." << uint32_t (version.minorAndBugRev) << "." << uint32_t (version.stage)
			<< ", Interface " << uint32_t (AJA_MAC_DRIVER_INTERFACE_VERSION);

	#else
	#endif

	return oss.str ();

}	//	GetDriverVersionString


bool CNTV2Card::GetDriverVersionComponents (UWord & outMajor, UWord & outMinor, UWord & outPoint, UWord & outBuild)
{
	bool	result		(false);

	outMajor = outMinor = outPoint = outBuild = 0;

	#if defined (MSWindows)
		ULWord	versionInfo	(0);
		result = GetDriverVersion (&versionInfo);
		outMajor = (versionInfo >>  4) & 0xF;
		outMinor = (versionInfo >>  0) & 0xF;
		outPoint = (versionInfo >>  8) & 0xF;
		outBuild = (versionInfo >> 16) & 0xFFFF;
	#elif defined (AJALinux)
		ULWord	versionInfo	(0);
		result = GetDriverVersion (&versionInfo);
		outMajor = (versionInfo >>  4) & 0xF;
		outMinor = (versionInfo >>  0) & 0xF;
		outPoint = (versionInfo >>  8) & 0x3F;
		outBuild = (versionInfo >> 16) & 0xFFFF;
	#elif defined (AJAMac)
		NumVersion	version	= {0, 0, 0, 0};
		CNTV2MacDriverInterface::GetDriverVersion (&version);
		outMajor = version.majorRev;
		outMinor = version.minorAndBugRev;
		outPoint = version.stage;
		result = true;
	#endif
	return result;
}


ULWord CNTV2Card::GetSerialNumberLow (void)
{
	ULWord	serialNum	(0);
	return ReadRegister (54, &serialNum) ? serialNum : 0;	//	Read EEPROM shadow of Serial Number
}


ULWord CNTV2Card::GetSerialNumberHigh (void)
{
	ULWord	serialNum	(0);
	return ReadRegister (55, &serialNum) ? serialNum : 0;	//	Read EEPROM shadow of Serial Number
}


bool CNTV2Card::IS_CHANNEL_INVALID (const NTV2Channel inChannel) const
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return true;
	return false;
}


bool CNTV2Card::IS_OUTPUT_SPIGOT_INVALID (const UWord inOutputSpigot) const
{
	if (inOutputSpigot >= ::NTV2DeviceGetNumVideoOutputs (_boardID))
	{
		if ((_boardID == DEVICE_ID_IO4KUFC || _boardID == DEVICE_ID_IO4KUFC) && inOutputSpigot == 4)
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
	const uint64_t	lo (GetSerialNumberLow ()),  hi (GetSerialNumberHigh ());
	uint64_t		result	((hi << 32) | lo);
	return result;
}


string CNTV2Card::SerialNum64ToString (const uint64_t inSerialNumber)	//	Class method
{
	const ULWord	serialNumHigh	(inSerialNumber >> 32);
	const ULWord	serialNumLow	(inSerialNumber & 0x00000000FFFFFFFF);
    char			serialNum [9];

	serialNum[0] = ((serialNumLow  & 0x000000FF)      );
	serialNum[1] = ((serialNumLow  & 0x0000FF00) >>  8);
	serialNum[2] = ((serialNumLow  & 0x00FF0000) >> 16);
	serialNum[3] = ((serialNumLow  & 0xFF000000) >> 24);
	serialNum[4] = ((serialNumHigh & 0x000000FF)      );
	serialNum[5] = ((serialNumHigh & 0x0000FF00) >>  8);
	serialNum[6] = ((serialNumHigh & 0x00FF0000) >> 16);
	serialNum[7] = ((serialNumHigh & 0xFF000000) >> 24);
	serialNum[8] = '\0';

	for (unsigned ndx (0);  ndx < 8;  ndx++)
	{
		if (serialNum [ndx] == 0)
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
	outSerialNumberString = SerialNum64ToString (GetSerialNumber ());
	if (outSerialNumberString.empty ())
	{
		outSerialNumberString = "INVALID?";
		return false;
	}
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


bool CNTV2Card::GetInput1Autotimed (void)
{
	ULWord	status	(0);
	ReadRegister (kRegInputStatus, &status);
	return !(status & BIT_3);
}


bool CNTV2Card::GetInput2Autotimed (void)
{
	ULWord	status	(0);
	ReadRegister (kRegInputStatus, &status);
	return !(status & BIT_11);
}


bool CNTV2Card::GetAnalogInputAutotimed (void)
{
	ULWord	value	(0);
	ReadRegister (kRegAnalogInputStatus, &value, kRegMaskInputStatusLock, kRegShiftInputStatusLock);
	return value == 1;
}


bool CNTV2Card::GetHDMIInputAutotimed (void)
{
	ULWord	value	(0);
	ReadRegister (kRegHDMIInputStatus, &value, kRegMaskInputStatusLock, kRegShiftInputStatusLock);
	return value == 1;
}


bool CNTV2Card::GetInputAutotimed (int inInputNum)
{
	bool bResult = false;

	ULWord status;
	ReadInputStatusRegister(&status);

	switch (inInputNum)
	{
		case 0:	bResult = !(status & BIT_3);	break;
		case 1: bResult = !(status & BIT_11);	break;
	}
	
	return bResult;
}


NTV2BreakoutType CNTV2Card::GetBreakoutHardware (void)
{
	NTV2BreakoutType	result		(NTV2_BreakoutNone);
	ULWord				audioCtlReg	(0);	//	The Audio Control Register tells us what's connected

	if (IsOpen ()  &&  ReadRegister (kRegAud1Control, &audioCtlReg))
	{
		const bool	bPhonyKBox	(false);	//	For debugging

		switch (_boardID)
		{
			case DEVICE_ID_KONA3G:
			case DEVICE_ID_KONA3GQUAD:
			case DEVICE_ID_IO4K:
			case DEVICE_ID_KONA4:
			case DEVICE_ID_KONA4UFC:
				//	Do we have a K3G-Box?
				if ((audioCtlReg & kK2RegMaskKBoxDetect) || bPhonyKBox)
					result = NTV2_K3GBox;
				else
					result = NTV2_BreakoutCableBNC;
				break;
			case DEVICE_ID_LHE_PLUS:
				//	Do we have a KL-Box?
				if ((audioCtlReg & kK2RegMaskKBoxDetect) || bPhonyKBox)
					result = NTV2_KLBox;
				else
					result = NTV2_BreakoutCableXLR;		// no BNC breakout cable available
				break;
			case DEVICE_ID_LHI:
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


ostream & operator << (ostream & inOutStr, const NTV2AudioChannelPairs & inSet)
{
	for (NTV2AudioChannelPairsConstIter iter (inSet.begin ());  iter != inSet.end ();  ++iter)
		inOutStr << (iter != inSet.begin () ? ", " : "") << ::NTV2AudioChannelPairToString (*iter, true);
	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioChannelQuads & inSet)
{
	for (NTV2AudioChannelQuadsConstIter iter (inSet.begin ());  iter != inSet.end ();  ++iter)
		inOutStr << (iter != inSet.begin () ? ", " : "") << ::NTV2AudioChannelQuadToString (*iter, true);
	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2AudioChannelOctets & inSet)
{
	for (NTV2AudioChannelOctetsConstIter iter (inSet.begin ());  iter != inSet.end ();  ++iter)
		inOutStr << (iter != inSet.begin () ? ", " : "") << ::NTV2AudioChannelOctetToString (*iter, true);
	return inOutStr;
}


ostream &	operator << (ostream & inOutStr, const NTV2DoubleArray & inVector)
{
	for (NTV2DoubleArrayConstIter iter (inVector.begin ());  iter != inVector.end ();  ++iter)
		inOutStr << *iter << endl;
	return inOutStr;
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::GetBitFileInformation (ULWord &	outNumBytes, string & outDateStr, string & outTimeStr, const NTV2XilinxFPGA inFPGA)
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
		ULWord numBoards = 0;
		CNTV2Card ntv2Card;

		for (ULWord boardCount = 0;  boardCount < NTV2_MAXBOARDS;  boardCount++)
		{
			if (ntv2Card.Open (boardCount))
			{
				numBoards++;
				ntv2Card.Close ();
			}
			else
				break;
		}
		return numBoards;
	}

	NTV2BoardType CNTV2Card::GetBoardType (void) const
	{
		return _boardType;
	}

	NTV2BoardSubType CNTV2Card::GetBoardSubType (void)
	{
		return BOARDSUBTYPE_NONE;
	}

	bool CNTV2Card::SetBoard (UWord inDeviceIndexNumber)
	{
		return Open (inDeviceIndexNumber);
	}

	string CNTV2Card::GetBoardIDString (void)
	{
		const ULWord	boardID	(static_cast <ULWord> (GetDeviceID ()));
		ostringstream	oss;
		oss << hex << boardID;
		return oss.str ();
	}

	void CNTV2Card::GetBoardIDString(std::string & outString)
	{
		outString = GetBoardIDString();
	}	///< @deprecated	Obsolete. Convert the result of GetDeviceID() into a hexa string instead.

#endif	//	!defined (NTV2_DEPRECATE)
