/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2konaflashprogram.cpp
	@brief		Implementation of CNTV2KonaFlashProgram class.
	@copyright	(C) 2010-2022 AJA Video Systems, Inc.
**/
#include "ntv2konaflashprogram.h"
#include "ntv2endian.h"
#include "ntv2registersmb.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/common/common.h"
#include <ctime>
#ifdef MSWindows
	#pragma warning(disable: 4305) // Initialization warnings.
	#pragma warning(disable: 4309)
	#pragma warning(disable: 4800)
	#pragma warning(disable: 4996)
#endif

#define ENUM_CASE_RETURN_VAL_OR_ENUM_STR(condition, retail_name, enum_name)\
	case(enum_name): return condition ? retail_name : #enum_name

using namespace std;

#define KFPDBUG(__x__)	AJA_sDEBUG (AJA_DebugUnit_Firmware, AJAFUNC << ": " << __x__)
#define KFPWARN(__x__)	AJA_sWARNING (AJA_DebugUnit_Firmware, AJAFUNC << ": " << __x__)
#define KFPERR(__x__)	do {ostringstream oss;  oss << AJAFUNC << ": " << __x__;  cerr << "## ERROR:    " << oss.str() << endl;  AJA_sERROR  (AJA_DebugUnit_Firmware, oss.str());} while(false)
#define KFPNOTE(__x__)	do {ostringstream oss;  oss << AJAFUNC << ": " << __x__;  if (!_bQuiet) cout << "## NOTE:  "    << oss.str() << endl;  AJA_sNOTICE (AJA_DebugUnit_Firmware, oss.str());} while(false)


string MacAddr::AsString(void) const
{
	ostringstream	oss;
	oss << xHEX0N(uint16_t(mac[0]),2) << ":" << xHEX0N(uint16_t(mac[1]),2) << ":" << xHEX0N(uint16_t(mac[2]),2)
		<< ":" << xHEX0N(uint16_t(mac[3]),2) << ":" << xHEX0N(uint16_t(mac[4]),2) << ":" << xHEX0N(uint16_t(mac[5]),2);
	return oss.str();
}

static CNTV2FlashProgress gNullUpdater;

CNTV2FlashProgress &	CNTV2FlashProgress::nullUpdater = gNullUpdater;

string CNTV2KonaFlashProgram::FlashBlockIDToString (const FlashBlockID inID, const bool inShortDisplay)
{
	switch (inID)
	{
		ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inShortDisplay, "Main",		MAIN_FLASHBLOCK);
		ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inShortDisplay, "FailSafe",	FAILSAFE_FLASHBLOCK);
		ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inShortDisplay, "Auto",		AUTO_FLASHBLOCK);
		ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inShortDisplay, "SOC1",		SOC1_FLASHBLOCK);
		ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inShortDisplay, "SOC2",		SOC2_FLASHBLOCK);
		ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inShortDisplay, "Mac",			MAC_FLASHBLOCK);
		ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inShortDisplay, "MCS",			MCS_INFO_BLOCK);
		ENUM_CASE_RETURN_VAL_OR_ENUM_STR(inShortDisplay, "License",		LICENSE_BLOCK);
	}
	return "";
}


CNTV2KonaFlashProgram::CNTV2KonaFlashProgram ()
	:	CNTV2Card (),
		_bitFileSize		(0),
		_flashSize			(0),
		_bankSize			(0),
		_sectorSize			(0),
		_mainOffset			(0),
		_failSafeOffset		(0),
		_macOffset			(0),
		_mcsInfoOffset		(0),
		_licenseOffset		(0),
		_soc1Offset			(0),
		_soc2Offset			(0),
		_numSectorsMain		(0),
		_numSectorsSOC1		(0),
		_numSectorsSOC2		(0),
		_numSectorsFailSafe (0),
		_numBytes			(0),
		_flashID			(MAIN_FLASHBLOCK),
		_deviceID			(0),
		_bQuiet				(false),
		_mcsStep			(0),
		_failSafePadding	(0),
		_spiFlash			(AJA_NULL),
		_hasExtendedCommandSupport	(false)
{
}

CNTV2KonaFlashProgram::CNTV2KonaFlashProgram (const UWord boardNumber)
	:	CNTV2Card (boardNumber),
		_bitFileSize		(0),
		_flashSize			(0),
		_bankSize			(0),
		_sectorSize			(0),
		_mainOffset			(0),
		_failSafeOffset		(0),
		_macOffset			(0),
		_mcsInfoOffset		(0),
		_licenseOffset		(0),
		_soc1Offset			(0),
		_soc2Offset			(0),
		_numSectorsMain		(0),
		_numSectorsSOC1		(0),
		_numSectorsSOC2		(0),
		_numSectorsFailSafe (0),
		_numBytes			(0),
		_flashID			(MAIN_FLASHBLOCK),
		_deviceID			(0),
		_bQuiet				(false),
		_mcsStep			(0),
		_failSafePadding	(0),
		_spiFlash			(AJA_NULL),
		_hasExtendedCommandSupport	(false)
{
	SetDeviceProperties();
}

CNTV2KonaFlashProgram::~CNTV2KonaFlashProgram()
{
	if (_spiFlash)
		delete _spiFlash;
}

void CNTV2KonaFlashProgram::SetQuietMode()
{
	_bQuiet = true;
	if (_spiFlash)
	{
		_spiFlash->SetVerbosity(false);
	}
}

bool CNTV2KonaFlashProgram::WriteCommand(_FLASH_COMMAND inCommand)
{
	ULWord theCommand = (ULWord)inCommand;
	if(_hasExtendedCommandSupport && inCommand == WRITESTATUS_COMMAND)
		theCommand |= BIT_16;
	
	return WriteRegister(kRegXenaxFlashControlStatus, theCommand);
}

bool CNTV2KonaFlashProgram::SetMBReset()
{
	if (!IsIPDevice())
		return false;
	bool resetOK(false);
	//	Hold MB in reset
	if ((GetDeviceID() == DEVICE_ID_IOIP_2022) ||
		(GetDeviceID() == DEVICE_ID_IOIP_2110) ||
		(GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12))
			resetOK = WriteRegister(SAREK_REGS + kRegSarekControl, 0x02);
	else if (GetDeviceID() == DEVICE_ID_KONAIP_2022 ||
			 GetDeviceID() == DEVICE_ID_KONAIP_2110 ||
			 GetDeviceID() == DEVICE_ID_KONAIP_2110_RGB12)
		resetOK = WriteRegister(SAREK_REGS + kRegSarekControl, 0x01);
	//	Take SPI bus control
	return resetOK && WriteRegister(SAREK_REGS + kRegSarekSpiSelect, 0x01);
}

bool CNTV2KonaFlashProgram::IsInstalledFWRunning (bool & outIsRunning, ostream & outMsgs)
{
	UWord	runningYear(0), runningMonth(0), runningDay(0);
	outIsRunning = false;

	//	Get running FW date...
	if (!GetRunningFirmwareDate (runningYear, runningMonth, runningDay))
	{
		if (::NTV2DeviceCanReportRunningFirmwareDate(GetDeviceID()))
			outMsgs << "## WARNING:  Failed to get running firmware date/time" << endl;
		return false;
	}

	//	Convert running FW date to time_t...
	std::tm tm;  ::memset(&tm, 0, sizeof(tm));	//	zero
	tm.tm_year	= int(runningYear) - 1900;		//	Year
	tm.tm_mon	= int(runningMonth) - 1;		//	Month
	tm.tm_mday	= int(runningDay);				//	Day
	tm.tm_hour	= 11;	//	near mid-day
	tm.tm_isdst	= 0;	//	Standard time (not DST)
	std::time_t tRunning (std::mktime(&tm));

	//	Read & parse Main installed FW header...
	if (!ReadHeader(MAIN_FLASHBLOCK))
		{outMsgs << "## WARNING:  Failed to ReadHeader or ParseHeader" << endl; return false;}

	string installedBuildDate(GetDate());
	//	TEST: same as running FW date:		ostringstream oss;  oss << DEC(runningYear) << "/" << DEC0N(runningMonth,2) << "/" << DEC0N(runningDay+0,2); installedBuildDate = oss.str();
	//	TEST: 1 day past running FW date:	ostringstream oss;  oss << DEC(runningYear) << "/" << DEC0N(runningMonth,2) << "/" << DEC0N(runningDay+1,2); installedBuildDate = oss.str();
	//	TEST: 2 days past running FW date:	ostringstream oss;  oss << DEC(runningYear) << "/" << DEC0N(runningMonth,2) << "/" << DEC0N(runningDay+2,2); installedBuildDate = oss.str();
	if (installedBuildDate.empty()  ||  installedBuildDate.length() < 10  ||  installedBuildDate.at(4) != '/')
		{outMsgs << "## WARNING:  Bad installed firmware date '" << installedBuildDate << "'" << endl;  return false;}

	//	Convert installed FW date to time_t...
	tm.tm_year	= int(aja::stol(installedBuildDate.substr(0, 4))) - 1900;	//	Year
	tm.tm_mon	= int(aja::stol(installedBuildDate.substr(5, 2))) - 1;		//	Month
	tm.tm_mday	= int(aja::stol(installedBuildDate.substr(8, 2)));			//	Day
	tm.tm_hour	= 11;	//	near mid-day
	tm.tm_isdst	= 0;	//	Standard time (not DST)
	std::time_t tInstalled (std::mktime(&tm));

	//	Calculate seconds between the two dates...
	ULWord secsApart (ULWord(::difftime(tInstalled, tRunning)));
	if (secsApart == 0)
		outIsRunning = true;		//	Same date
	else if (secsApart <= 86400)	//	Call them equal even within a day apart
		{outMsgs << "## WARNING:  Installed firmware date is 1 day past running firmware date" << endl;  outIsRunning = true;}
	return true;
}

bool CNTV2KonaFlashProgram::SetBoard(UWord boardNumber, uint32_t index)
{
	if (!AsNTV2DriverInterfaceRef(*this).Open(boardNumber))
		return false;

	if (!SetDeviceProperties())
		return false;

	//For manufacturing use the leds to code the board number
	uint32_t ledMask = BIT(16)+BIT(17);
	uint32_t ledShift = 16;
	return WriteRegister(kRegGlobalControl, index, ledMask, ledShift);

}

bool CNTV2KonaFlashProgram::SetDeviceProperties()
{
	bool knownChip = false;
	bool status = false;
	
	_deviceID = ReadDeviceID();

//	  case 0x00202018://STMircro
//	  case 0x00012018://CYPRESS S25FL128
//	  case 0x00C22018://Macronix
//		  T-Tap
//		  IoExpress
//		  IoXT
//		  Kona Lhe+
//		  Kona 3G
//		  Kona 3G Quad
//		  Corvid 1
//		  Corvid 22
//		  Corvid 24
//		  Corvid 3G

//	  case 0x00010220://CYPRESS f25fl512
//		  Kona IP 2022
//		  Kona IP 2110
//		  IoIP 2022
//		  IoIP 2110
//		  Io4K+

//	  case 0x009d6019://ISSI
//		  No Product 6/27/18

//	  case 0x00C84018://GIGADEVICE GD25Q127CFIG
//	  case 0x00EF4018://WINBOND W25Q128
//		  T-Tap
//		  IoExpress
//		  IoXT
//		  Kona Lhe+
//		  Kona 3G
//		  Kona 3G Quad
//		  Corvid 1
//		  Corvid 22
//		  Corvid 24
//		  Corvid 3G

//	  case 0x00010219://CYPRESS S25FL256
//		  Kona 1
//		  Kona HDMI
//		  Kona 4
//		  Kona 4 UFC
//		  Corvid 44
//		  Corvid 88
//		  Corvid HBR
//		  Corvid HEVC

	switch(_deviceID)
	{
	case 0x00202018://STMircro
	case 0x00C22018://Macronix
		_flashSize = 16 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 256 * 1024;
		_failSafePadding = 1;
		knownChip = true;
		break;
	case 0x00010220://CYPRESS f25fl512
		_flashSize = 64 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 256 * 1024;
		_failSafePadding = 1;
		knownChip = true;
		break;
	case 0x009d6019://ISSI
		_flashSize = 64 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 64 * 1024;
		_failSafePadding = 4;
		knownChip = true;
		break;
	case 0x0020ba20://Micron MT25QL512ABB
		_flashSize = 64 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 64 * 1024;
		_failSafePadding = 4;
		_hasExtendedCommandSupport = true;
		knownChip = true;
		break;
	case 0x00C84018://GIGADEVICE GD25Q127CFIG
	case 0x00EF4018://WINBOND W25Q128
	case 0x00012018://CYPRESS S25FL128
		_flashSize = 16 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 64 * 1024;
		_failSafePadding = 4;
		knownChip = true;
		break;
	case 0x00010219://CYPRESS S25FL256
		_flashSize = 32 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 64 * 1024;
		_failSafePadding = 4;
		knownChip = true;
		break;
	default:
		_flashSize = 0;
		_bankSize = 0;
		_sectorSize = 0;
		knownChip = false;
		break;
	}

	if(!knownChip)
		return false;
	
	switch(::NTV2DeviceGetSPIFlashVersion(GetDeviceID()))
	{
	default:
	case 1:
		//This includes legacy boards such as LHi, Corvid 1...
		//SPI is devided up into 4 logical blocks of 4M each
		//Without history explained main is at offset 0 and failsafe is at offset 12
		_numSectorsMain = _flashSize / _sectorSize / 4;
		_numSectorsFailSafe = (_flashSize / _sectorSize / 4) - 1;
		_mainOffset = 0;
		_failSafeOffset = 12 * 1024 * 1024;
		_macOffset = _bankSize - (2 * _sectorSize);
		status = true;
		break;
	case 2:
		_numSectorsMain = _flashSize / _sectorSize / 2;
		_numSectorsFailSafe = (_flashSize / _sectorSize / 2) - _failSafePadding;
		_mainOffset = 0;
		_failSafeOffset = 8 * 1024 * 1024;
		_macOffset = _bankSize - (2 * _sectorSize);
		status = true;
		break;
	case 3:
		if (_deviceID == 0x010220)
		{
			//This is actually SPI v4 but needed this for spoofing Kona4
			_numSectorsMain = _flashSize / _sectorSize / 4;
			_numSectorsFailSafe = (_flashSize / _sectorSize / 4) - 3;
			_numSectorsSOC1 = _flashSize / _sectorSize / 4;
			_numSectorsSOC2 = _flashSize / _sectorSize / 4;
			_mainOffset = 0;
			_soc1Offset = 0;
			_soc2Offset = 0;
			_failSafeOffset = 0;// but is really 16*1024*1024;
			_macOffset = _bankSize - (2 * _sectorSize);
			_mcsInfoOffset = _bankSize - (3*_sectorSize);
			_licenseOffset = _bankSize - (4* _sectorSize);
			status = true;
		}
		else
		{
			//SPIV3 This gets a little weird both main and failsafe have an offset of 0
			//and the real offset is controlled by a bank selector switch in firmware
			_numSectorsMain = _flashSize / _sectorSize / 2;
			_numSectorsFailSafe = (_flashSize / _sectorSize / 2) - _failSafePadding;
			_mainOffset = 0;
			_failSafeOffset = 0;// but is really 16*1024*1024;
			_macOffset = _bankSize - (2 * _sectorSize);
			_mcsInfoOffset = _bankSize - (3 * _sectorSize);
			_licenseOffset = _bankSize - (4* _sectorSize);
			status = true;
		}
		break;
	case 4:
		//SPIV4 is a bigger SPIv3 2x
		_numSectorsMain = _flashSize / _sectorSize / 4;
		_numSectorsFailSafe = (_flashSize / _sectorSize / 4) - 4;
		_numSectorsSOC1 = _flashSize / _sectorSize / 4;
		_numSectorsSOC2 = _flashSize / _sectorSize / 4;
		_mainOffset = 0;
		_soc1Offset = 0;
		_soc2Offset = 0;
		_failSafeOffset = 0;// but is really 16*1024*1024;
		_macOffset = _bankSize - (2 * _sectorSize);
		_mcsInfoOffset = _bankSize - (3 * _sectorSize);
		_licenseOffset = _bankSize - (4* _sectorSize);
		status = true;
		break;
	case 5:
	case 6:
		_numSectorsMain = _flashSize / _sectorSize / 2;
		_numSectorsFailSafe = (_flashSize / _sectorSize / 2) - _failSafePadding;
		_mainOffset = 0;
		_failSafeOffset = 0;// but is really 32*1024*1024;
		status = true;
		break;
	}

	if (_spiFlash)
	{
		delete _spiFlash;
		_spiFlash = AJA_NULL;
	}

	if (DEVICE_IS_IOIP(_boardID))
	{
		_spiFlash = new CNTV2AxiSpiFlash(GetIndexNumber(), !_bQuiet);
	}

	return status;
}

bool CNTV2KonaFlashProgram::SetBitFile (const string & inBitfileName, ostream & outMsgs, const FlashBlockID blockID)
{
	_bitFileBuffer.Deallocate();
	_bitFileName = inBitfileName;

	if (blockID == AUTO_FLASHBLOCK)
		DetermineFlashTypeAndBlockNumberFromFileName(inBitfileName);
	else if (blockID >= MAIN_FLASHBLOCK	 &&	 blockID <= FAILSAFE_FLASHBLOCK)
		_flashID = blockID;
	else
		{outMsgs << "Invalid flash block ID " << DEC(blockID);  return false;}

	FILE* pFile = AJA_NULL;
	struct stat fsinfo;
	stat(inBitfileName.c_str(), &fsinfo);
	_bitFileSize = uint32_t(fsinfo.st_size);
	pFile = fopen(inBitfileName.c_str(), "rb");
	if (!pFile)
		{outMsgs << "Cannot open bitfile '" << inBitfileName << "'";  return false;}

	// +_256 for fastFlash Programming
	if (!_bitFileBuffer.Allocate(_bitFileSize+512))
		{outMsgs << "Allocate " << DEC(_bitFileSize+512) << "-byte buffer failed";  return false;}
	_bitFileBuffer.Fill(0xFFFFFFFF);

	fseek(pFile, 0, SEEK_SET);
	fread(_bitFileBuffer, 1, _bitFileSize, pFile);
	fclose(pFile);

	// Parse header to make sure this is a xilinx bitfile.
	if (!_parser.ParseHeader(_bitFileBuffer, outMsgs))
		return false;

	if (!SetDeviceProperties())
		{outMsgs << "Device not recognized";  return false;}
	return true;
}

void CNTV2KonaFlashProgram::DetermineFlashTypeAndBlockNumberFromFileName (const string & bitFileName)
{
	_flashID = MAIN_FLASHBLOCK;
	if (bitFileName.find("_fs_") != string::npos)
		_flashID = FAILSAFE_FLASHBLOCK;
}

bool CNTV2KonaFlashProgram::ReadHeader(FlashBlockID blockID)
{
	uint32_t baseAddress (GetBaseAddressForProgramming(blockID));
	SetFlashBlockIDBank(blockID);
	NTV2Buffer bitFileHeader(MAXBITFILE_HEADERSIZE);
	const uint32_t dwordSizeCount (bitFileHeader.GetByteCount() / 4);
	for (uint32_t count(0);  count < dwordSizeCount;  count++, baseAddress += 4)
	{
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, bitFileHeader.U32(int(count)));
	}
	ostringstream msgs;
	const bool status (_parser.ParseHeader(bitFileHeader, msgs));
	SetBankSelect(BANK_0);	//	Make sure to reset bank to lower
	return status;
}

bool CNTV2KonaFlashProgram::ReadInfoString()
{
	if (_spiFlash)
	{
		vector<uint8_t> mcsInfoData;
		bool oldVerboseMode = _spiFlash->GetVerbosity();
		_spiFlash->SetVerbosity(false);
		uint32_t offset = _spiFlash->Offset(SPI_FLASH_SECTION_MCSINFO);
		if (_spiFlash->Read(offset, mcsInfoData, MAXMCSINFOSIZE))
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			_mcsInfo.assign(mcsInfoData.begin(), mcsInfoData.end());
		}
		else
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			return false;
		}
	}
	else
	{
		if (_deviceID != 0x010220 || !IsIPDevice())
			return false;
		uint32_t baseAddress = _mcsInfoOffset;
		SetFlashBlockIDBank(MCS_INFO_BLOCK);

		NTV2Buffer mcsInfoPtr(MAXMCSINFOSIZE);
		uint32_t dwordSizeCount (MAXMCSINFOSIZE / 4);
		for (uint32_t count(0);  count < dwordSizeCount;  count++, baseAddress += 4)
		{
			WriteRegister(kRegXenaxFlashAddress, baseAddress);
			WriteCommand(READFAST_COMMAND);
			WaitForFlashNOTBusy();
			ReadRegister(kRegXenaxFlashDOUT, mcsInfoPtr.U32(int(count)));
			if (mcsInfoPtr.U32(int(count)) == 0)
				break;
		}
		_mcsInfo = reinterpret_cast<const char*>(mcsInfoPtr.GetHostPointer());
		SetBankSelect(BANK_0);	//	Make sure to reset bank to lower
	}
	//	Fix up _mcsInfo...
	size_t ffPos(_mcsInfo.find("\xFF\xFF"));
	if (ffPos != string::npos)
		_mcsInfo = _mcsInfo.substr(0, ffPos);	//	Lop off "\xFF\xFF...", if present
	return true;
}

std::string CNTV2KonaFlashProgram::Program(bool fullVerify)
{
	if (!_bitFileBuffer)
		return "Bitfile not open";

	if (!IsOpen())
		return "Device not open";

	uint32_t baseAddress(GetBaseAddressForProgramming(_flashID));
	switch (_flashID)
	{
		case MAIN_FLASHBLOCK:		WriteRegister(kVRegFlashState, kProgramStateEraseMainFlashBlock);		break;
		case FAILSAFE_FLASHBLOCK:	WriteRegister(kVRegFlashState, kProgramStateEraseFailSafeFlashBlock);	break;
		case SOC1_FLASHBLOCK:		WriteRegister(kVRegFlashState, kProgramStateEraseBank3);				break;
		case SOC2_FLASHBLOCK:		WriteRegister(kVRegFlashState, kProgramStateEraseBank4);				break;
		default: break;
	}

	EraseBlock(_flashID);

	SetFlashBlockIDBank(_flashID);

	uint32_t* bitFilePtr = _bitFileBuffer;
	uint32_t twoFixtysixBlockSizeCount ((_bitFileSize + 256) / 256);
	uint32_t percentComplete(0);
	WriteRegister(kVRegFlashState, kProgramStateProgramFlash);
	WriteRegister(kVRegFlashSize, twoFixtysixBlockSizeCount);
	for (uint32_t count(0);  count < twoFixtysixBlockSizeCount;  count++, baseAddress += 256, bitFilePtr += 64)
	{
		if (::NTV2DeviceGetSPIFlashVersion(_boardID) >= 5  &&  baseAddress == _bankSize)
		{
			baseAddress = 0;
			SetBankSelect(_flashID == FAILSAFE_FLASHBLOCK ? BANK_3 : BANK_1);
		}
		FastProgramFlash256(baseAddress, bitFilePtr);
		percentComplete = (count*100)/twoFixtysixBlockSizeCount;

		WriteRegister(kVRegFlashStatus, count);
		if (!_bQuiet)
			cout << "Program status: " << DEC(percentComplete) << "%  \r" << flush;
	}
	if (!_bQuiet)
		cout << "Program status: 100%				   " << endl;

	SetBankSelect(BANK_0);
	if (!VerifyFlash(_flashID, fullVerify))
	{
		SetBankSelect(BANK_0);
		return "Program Didn't Verify";
	}
	
	// Protect Device
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x1C);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();

	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x9C);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	SetBankSelect(BANK_0);
	
	SetWarmBootFirmwareReload(true);
	return "";
}

bool CNTV2KonaFlashProgram::ProgramFlashValue(uint32_t address, uint32_t value)
{
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, value);
	WriteRegister(kRegXenaxFlashAddress, address);
	WriteCommand(PAGEPROGRAM_COMMAND);
	WaitForFlashNOTBusy();

	return true;
}

bool CNTV2KonaFlashProgram::FastProgramFlash256(uint32_t address, uint32_t* buffer)
{
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	for ( uint32_t count=0; count < 64; count++ )
	{
		WriteRegister(kRegXenaxFlashDIN, *buffer++);
	}
	WriteRegister(kRegXenaxFlashAddress, address);
	WriteCommand(PAGEPROGRAM_COMMAND);
	WaitForFlashNOTBusy();
	
	return true;
}

uint32_t CNTV2KonaFlashProgram::ReadDeviceID()
{
	uint32_t deviceID = 0;
	if (IsOpen ())
	{
		WriteCommand(READID_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, deviceID);
	}
	return (deviceID & 0xFFFFFF);
}

bool CNTV2KonaFlashProgram::EraseBlock (FlashBlockID blockID)
{
	if (!IsOpen())
		return false;
	SetFlashBlockIDBank(blockID);

	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x0);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	uint32_t percentComplete = 0;

	uint32_t numSectors = GetNumberOfSectors(blockID);
	WriteRegister(kVRegFlashSize,numSectors);

	uint32_t baseAddress = GetBaseAddressForProgramming(blockID);
	uint32_t bankCount = 0;
	for (uint32_t sectorCount = 0; sectorCount < numSectors; sectorCount++ )
	{
		if (::NTV2DeviceGetSPIFlashVersion(_boardID) >= 5  &&  sectorCount*_sectorSize == _bankSize)
		{
			switch(blockID)
			{
			default:
			case MAIN_FLASHBLOCK:
				SetBankSelect(BANK_1);
				break;
			case FAILSAFE_FLASHBLOCK:
				SetBankSelect(BANK_3);
				break;
			}
			bankCount++;
		}
		EraseSector(baseAddress + ((sectorCount - (_numSectorsMain* bankCount)) * _sectorSize));
		percentComplete = (sectorCount*100)/numSectors;
		WriteRegister(kVRegFlashStatus, sectorCount);
		if (!_bQuiet)
			cout << "Erase status: " << DEC(percentComplete) << "%\r" << flush;
	}
	WriteRegister(kVRegFlashStatus,numSectors);
	if (!_bQuiet)
		cout << "Erase status: 100%				" << endl;
	//if ( !CheckFlashErasedWithBlockID(flashBlockNumber))
		//throw "Erase didn't work";
	return SetBankSelect(BANK_0);
}

bool CNTV2KonaFlashProgram::EraseSector (uint32_t sectorAddress)
{
	WriteRegister(kRegXenaxFlashAddress, sectorAddress);
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();	
	WriteCommand(SECTORERASE_COMMAND);
	return WaitForFlashNOTBusy();
}

bool CNTV2KonaFlashProgram::EraseChip (UWord chip)
{	(void) chip;	//	unused
	WriteRegister(kRegXenaxFlashControlStatus,0);
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x0);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteCommand(CHIPERASE_COMMAND);
	return WaitForFlashNOTBusy();
}

bool CNTV2KonaFlashProgram::VerifyFlash (FlashBlockID flashID, bool fullVerify)
{
	uint32_t errorCount = 0;
	uint32_t baseAddress = GetBaseAddressForProgramming(flashID);
	uint32_t* bitFilePtr = _bitFileBuffer;
	uint32_t dwordSizeCount ((_bitFileSize + 4) / 4);
	uint32_t percentComplete(0), lastPercentComplete(999);
	
	SetBankSelect(_flashID == FAILSAFE_FLASHBLOCK  ?  (::NTV2DeviceGetSPIFlashVersion(_boardID) >= 5 ? BANK_2 : BANK_1)  :  BANK_0);
	WriteRegister(kVRegFlashState, kProgramStateVerifyFlash);
	WriteRegister(kVRegFlashSize, dwordSizeCount);
	for (uint32_t count = 0; count < dwordSizeCount; )
	{
		if (::NTV2DeviceGetSPIFlashVersion(_boardID) >= 5  &&  baseAddress == _bankSize)
		{
			baseAddress = 0;
			SetBankSelect(_flashID == FAILSAFE_FLASHBLOCK  ?  BANK_3  :  BANK_1);
		}
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, flashValue);
		uint32_t bitFileValue = *bitFilePtr;
		if (flashValue != bitFileValue)
		{
			cerr << "Error " << DEC(count) << " E(" << HEX0N(bitFileValue,8) << "),R(" << HEX0N(flashValue,8) << ")" << endl;
			errorCount++;
			if (errorCount > 1)
				break;
		}
		percentComplete = (count*100)/dwordSizeCount;
		WriteRegister(kVRegFlashStatus, count);
		if (!_bQuiet)
		{
			if (percentComplete != lastPercentComplete)
			{
				cout << "Program verify: " << DEC(percentComplete) << "%\r" << flush;
				lastPercentComplete = percentComplete;
			}
		}
		count += fullVerify ? 1 : 64;
		baseAddress += fullVerify ? 4 : 256;
		bitFilePtr +=  fullVerify ? 1 : 64;
	}

	SetBankSelect(BANK_0);

	if (errorCount)
	{
		if (!_bQuiet)
			cout << "Program verify failed: " << DEC(percentComplete) << "%" << endl;
		return false;
	}
	if (!_bQuiet)
		cout << "Program verify: 100%					 " << endl;
	return true;
}

bool CNTV2KonaFlashProgram::ReadFlash (NTV2Buffer & outBuffer, const FlashBlockID inFlashID, CNTV2FlashProgress & inFlashProgress)
{
	uint32_t baseAddress(GetBaseAddressForProgramming(inFlashID));
	const uint32_t numDWords((_bitFileSize+4)/4);
	if (outBuffer.GetByteCount() < numDWords*4)
	{
		if (outBuffer.GetByteCount()  &&  !outBuffer.IsAllocatedBySDK())
			{KFPERR("Unable to resize target buffer (not alloc'd by SDK)");  return false;}
		if (!outBuffer.Allocate(numDWords * 4))
			{KFPERR("Failed to allocate " << DEC(numDWords*4) << "-byte target buffer");  return false;}
	}

	size_t lastPercent(0), percent(0);
	inFlashProgress.UpdatePercentage(lastPercent);
	switch (_flashID)
	{
		default:
		case MAIN_FLASHBLOCK:
			SetBankSelect(BANK_0);
			break;
		case FAILSAFE_FLASHBLOCK:
			SetBankSelect(::NTV2DeviceGetSPIFlashVersion(_boardID) >= 5 ? BANK_2 : BANK_1);
			break;
	}
	WriteRegister(kVRegFlashState, kProgramStateVerifyFlash);
	WriteRegister(kVRegFlashSize, numDWords);
	KFPDBUG("About to read " << xHEX0N(numDWords*4,8) << "(" << DEC(numDWords*4) << ") bytes from '" << FlashBlockIDToString(inFlashID, /*compact*/true) << "' address " << xHEX0N(baseAddress,8));
	for (uint32_t dword(0);	 dword < numDWords;	 )
	{
			if (::NTV2DeviceGetSPIFlashVersion(_boardID) >= 5  &&  baseAddress == _bankSize)
		{
			baseAddress = 0;
			switch (_flashID)
			{
				default:
				case MAIN_FLASHBLOCK:
					SetBankSelect(BANK_1);
					break;
				case FAILSAFE_FLASHBLOCK:
					SetBankSelect(BANK_3);
					break;
			}
		}
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, flashValue);
		outBuffer.U32(int(dword)) = flashValue;
		WriteRegister(kVRegFlashStatus, dword);

		dword += 1;
		percent = dword * 100 / numDWords;
		if (percent != lastPercent)
			if (!inFlashProgress.UpdatePercentage(percent))
				{SetBankSelect(BANK_0);	 KFPERR("Cancelled at " << DEC(percent) << "% addr=" << xHEX0N(baseAddress,8) << " dword=" << DEC(dword));  return false;}
		lastPercent = percent;
		if ((dword % 0x10000) == 0) cerr << xHEX0N(dword,8) << " of " << xHEX0N(numDWords,8) << endl;
		baseAddress += 4;
	}
	SetBankSelect(BANK_0);
	inFlashProgress.UpdatePercentage(100);
	KFPNOTE("Successfully read " << xHEX0N(numDWords*4,8) << "(" << DEC(numDWords*4) << ") bytes from '" << FlashBlockIDToString(inFlashID, /*compact*/true) << "' address " << xHEX0N(baseAddress,8));
	return true;
}

bool CNTV2KonaFlashProgram::WaitForFlashNOTBusy()
{
	bool busy  = true;
	int i = 0;
	uint32_t regValue;
	while (i < 1)
	{
		ReadRegister(kRegBoardID, regValue);
		i++;
	}
	regValue = 0;
	do
	{
		ReadRegister(kRegXenaxFlashControlStatus, regValue);
		if (!(regValue & BIT(8)))
		{
			busy = false;
			break;
		}
	} while (busy);

	return !busy;  // Return true if wait was successful
}

bool CNTV2KonaFlashProgram::CheckFlashErasedWithBlockID (FlashBlockID flashID)
{
	bool status = true;
	uint32_t baseAddress = GetBaseAddressForProgramming(flashID);
	uint32_t numSectors = GetNumberOfSectors(flashID);
	uint32_t dwordSizeCount = (numSectors*_sectorSize)/4;
	uint32_t percentComplete = 0;
	SetFlashBlockIDBank(flashID);

	for (uint32_t count = 0;  count < dwordSizeCount;  count++, baseAddress += 4)
	{
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, flashValue);
		if ( flashValue != 0xFFFFFFFF )
		{
			count = dwordSizeCount;
			status = false;
			continue;
		}
		percentComplete = (count*100)/dwordSizeCount;
		if(!_bQuiet)
			cout << "Erase verify: " << DEC(percentComplete) << "%\r" << flush;
	}
	if(!_bQuiet && status == true)
		cout << "Erase verify: 100%					   " << endl;

	SetBankSelect(BANK_0);

	return status;
}

void CNTV2KonaFlashProgram::SRecordOutput (const char *pSRecord)
{
	cout << pSRecord << endl;
}

bool CNTV2KonaFlashProgram::CreateSRecord(bool bChangeEndian)
{
	uint32_t baseAddress = 0;
	char sRecord[100];
	uint32_t partitionOffset = 0;

	SRecordOutput("S0030000FC");

	for ( uint32_t count = 0; count < _flashSize; count+=32)
	{
		if (ROMHasBankSelect() && count % _bankSize == 0)
		{
			baseAddress = 0;
			partitionOffset = count;
			switch (partitionOffset)
			{
				default:
				case 0x00000000:	SetBankSelect(BANK_0);	break;
				case 0x01000000:	SetBankSelect(BANK_1);	break;
				case 0x02000000:	SetBankSelect(BANK_2);	break;
				case 0x03000000:	SetBankSelect(BANK_3);	break;
			}
		}

		uint32_t recordSize = 32;
		if((_flashSize - count) < recordSize)
			recordSize = _flashSize - count;


		UByte checksum = 0;

		sRecord[0] = 'S';
		sRecord[1] = '3';

		uint32_t cc (recordSize + 5);
		sprintf(&sRecord[2], "%02x", cc);
		checksum += cc;

		uint32_t addr = baseAddress+partitionOffset;
		UWord aa = ((addr >> 24) &0xff);
		sprintf(&sRecord[4], "%02x", aa);
		checksum += aa;

		aa = ((addr >> 16) & 0xff);
		sprintf (&sRecord[6],"%02x", aa);
		checksum += aa;

		aa = ((addr >> 8) & 0xff);
		sprintf (&sRecord[8],"%02x", aa);
		checksum += aa;

		aa = (addr & 0xff);
		sprintf (&sRecord[10],"%02x", aa);
		checksum += aa;

		uint32_t i = 0;
		int32_t index = 12;
		while(i < recordSize)
		{
			WriteRegister(kRegXenaxFlashAddress,baseAddress);
			WriteCommand(READFAST_COMMAND);
			WaitForFlashNOTBusy();
			uint32_t flashValue;
			ReadRegister(kRegXenaxFlashDOUT, flashValue);
			if(bChangeEndian)
				flashValue = NTV2EndianSwap32(flashValue);

			UWord dd = (flashValue & 0xff);
			sprintf(&sRecord[index], "%02x", dd);
			checksum += dd;

			dd = ((flashValue >> 8) & 0xff);
			sprintf(&sRecord[index+2], "%02x", dd);
			checksum += dd;

			dd = ((flashValue >> 16) & 0xff);
			sprintf(&sRecord[index+4], "%02x", dd);
			checksum += dd;

			dd = ((flashValue >> 24) & 0xff);
			sprintf(&sRecord[index+6], "%02x", dd);
			checksum += dd;

			i += 4;
			index += 8;
			baseAddress += 4;
		}
		checksum = ~checksum;
		sprintf(&sRecord[index], "%02x", checksum);

		SRecordOutput(sRecord);
	}

	SetBankSelect(BANK_0);

	SRecordOutput ("S705FFF001000A");

	return true;
}

bool CNTV2KonaFlashProgram::CreateBankRecord(BankSelect bankID)
{
	uint32_t baseAddress = 0;
	char sRecord[100];
	uint32_t partitionOffset = 0;

	SRecordOutput("S0030000FC");

	for (uint32_t count = 0; count < _bankSize; count += 32)
	{
		if (ROMHasBankSelect())
		{
			SetBankSelect(bankID);
		}

		uint32_t recordSize = 32;
		if ((_flashSize - count) < recordSize)
			recordSize = _flashSize - count;

		UByte checksum = 0;

		sRecord[0] = 'S';
		sRecord[1] = '3';

		UWord cc (UWord(recordSize) + 5);
		sprintf(&sRecord[2], "%02x", cc);
		checksum += cc;

		uint32_t addr = baseAddress + partitionOffset;
		UWord aa = ((addr >> 24) & 0xff);
		sprintf(&sRecord[4], "%02x", aa);
		checksum += aa;

		aa = ((addr >> 16) & 0xff);
		sprintf(&sRecord[6], "%02x", aa);
		checksum += aa;

		aa = ((addr >> 8) & 0xff);
		sprintf(&sRecord[8], "%02x", aa);
		checksum += aa;

		aa = (addr & 0xff);
		sprintf(&sRecord[10], "%02x", aa);
		checksum += aa;

		uint32_t i = 0;
		int32_t index = 12;
		while (i < recordSize)
		{
			WriteRegister(kRegXenaxFlashAddress, baseAddress);
			WriteCommand(READFAST_COMMAND);
			WaitForFlashNOTBusy();
			uint32_t flashValue;
			ReadRegister(kRegXenaxFlashDOUT, flashValue);
			//flashValue = NTV2EndianSwap32(flashValue);

			UWord dd = (flashValue & 0xff);
			sprintf(&sRecord[index], "%02x", dd);
			checksum += dd;

			dd = ((flashValue >> 8) & 0xff);
			sprintf(&sRecord[index + 2], "%02x", dd);
			checksum += dd;

			dd = ((flashValue >> 16) & 0xff);
			sprintf(&sRecord[index + 4], "%02x", dd);
			checksum += dd;

			dd = ((flashValue >> 24) & 0xff);
			sprintf(&sRecord[index + 6], "%02x", dd);
			checksum += dd;

			i += 4;
			index += 8;
			baseAddress += 4;
		}
		checksum = ~checksum;
		sprintf(&sRecord[index], "%02x", checksum);

		SRecordOutput(sRecord);
	}

	SetBankSelect(BANK_0);

	SRecordOutput("S705FFF001000A");

	return true;
}

bool CNTV2KonaFlashProgram::CreateEDIDIntelRecord()
{
	char iRecord[100];
	int32_t recordSize = 16;
	UWord baseAddress = 0x0000;
	UByte checksum = 0;
	UByte recordType = 0x00;
	UByte byteCount = 0x10;

	uint32_t i2cVal = 0x02000050;

	for(int32_t x = 0; x < 16; x++)
	{
		int32_t i= 0;
		int32_t index = 0;
		checksum = 0;

		iRecord[0] = ':';

		sprintf(&iRecord[1], "%02x", byteCount);
		checksum += byteCount;

		UWord addr = baseAddress;
		UByte aa = ((addr >> 8) & 0xff);
		sprintf(&iRecord[3], "%02x", aa);
		checksum += aa;
		
		aa = ((addr) & 0xff);
		sprintf(&iRecord[5], "%02x", aa);
		checksum += aa;

		sprintf (&iRecord[7], "%02x", recordType);

		index = 9;

		while(i<recordSize)
		{
			WriteRegister(kRegFS1I2C1Address, i2cVal);

			AJATime::Sleep(100);

			uint32_t flashValue;
			ReadRegister(kRegFS1I2C1Data, flashValue);

			UByte dd = ((flashValue >> 8) & 0xff);
			sprintf(&iRecord[index], "%02x", dd);
			checksum += dd;

			i++;
			index+=2;
			i2cVal += 0x00000100;
		}

		baseAddress += 0x0010;
		checksum = (checksum ^ 0xFF)+1;
		sprintf(&iRecord[index], "%02x", checksum);

		SRecordOutput(iRecord);
	}

	SRecordOutput(":00000001FF");

	return true;

}

bool CNTV2KonaFlashProgram::ProgramMACAddresses(MacAddr * mac1, MacAddr * mac2)
{
	if(!IsIPDevice())
		return false;

	if (!mac1  ||  !mac2)
		return false;

	if (_spiFlash)
	{
		vector<uint8_t> macData;
		macData.push_back(mac1->mac[3]);
		macData.push_back(mac1->mac[2]);
		macData.push_back(mac1->mac[1]);
		macData.push_back(mac1->mac[0]);
		macData.push_back(0);
		macData.push_back(0);
		macData.push_back(mac1->mac[5]);
		macData.push_back(mac1->mac[4]);

		macData.push_back(mac2->mac[3]);
		macData.push_back(mac2->mac[2]);
		macData.push_back(mac2->mac[1]);
		macData.push_back(mac2->mac[0]);
		macData.push_back(0);
		macData.push_back(0);
		macData.push_back(mac2->mac[5]);
		macData.push_back(mac2->mac[4]);

		bool oldVerboseMode = _spiFlash->GetVerbosity();
		_spiFlash->SetVerbosity(false);
		uint32_t offset = _spiFlash->Offset(SPI_FLASH_SECTION_MAC);
		_spiFlash->Erase(offset, uint32_t(macData.size()));
		if (_spiFlash->Write(offset, macData, uint32_t(macData.size())))
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			return true;
		}
		else
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			return false;
		}
	}
	else
	{
		uint32_t baseAddress = _macOffset;

		EraseBlock(MAC_FLASHBLOCK);

		SetFlashBlockIDBank(MAC_FLASHBLOCK);


		uint32_t lo = 0;
		lo |= uint32_t((mac1->mac[0]) << 24) & 0xff000000;
		lo |= uint32_t((mac1->mac[1]) << 16) & 0x00ff0000;
		lo |= uint32_t((mac1->mac[2]) << 8)  & 0x0000ff00;
		lo |= uint32_t(mac1->mac[3])         & 0x000000ff;

		uint32_t hi = 0;
		hi |= uint32_t((mac1->mac[4]) << 24) & 0xff000000;
		hi |= uint32_t((mac1->mac[5]) << 16) & 0x00ff0000;

		uint32_t lo2 = 0;
		lo2 |= uint32_t((mac2->mac[0]) << 24) & 0xff000000;
		lo2 |= uint32_t((mac2->mac[1]) << 16) & 0x00ff0000;
		lo2 |= uint32_t((mac2->mac[2]) << 8)  & 0x0000ff00;
		lo2 |= uint32_t(mac2->mac[3])         & 0x000000ff;

		uint32_t hi2 = 0;
		hi2 |= uint32_t((mac2->mac[4]) << 24) & 0xff000000;
		hi2 |= uint32_t((mac2->mac[5]) << 16) & 0x00ff0000;


		ProgramFlashValue(baseAddress, lo);
		baseAddress += 4;
		ProgramFlashValue(baseAddress, hi);
		baseAddress += 4;
		ProgramFlashValue(baseAddress, lo2);
		baseAddress += 4;
		ProgramFlashValue(baseAddress, hi2);

		WriteCommand(WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x9C);
		WriteCommand(WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();

		SetBankSelect(BANK_0);

		return true;
	}
}

bool CNTV2KonaFlashProgram::ReadMACAddresses(MacAddr & mac1, MacAddr & mac2)
{
	uint32_t lo;
	uint32_t hi;
	uint32_t lo2;
	uint32_t hi2;

	if(!IsIPDevice())
		return false;

	if (_spiFlash)
	{
		vector<uint8_t> macData;
		bool oldVerboseMode = _spiFlash->GetVerbosity();
		_spiFlash->SetVerbosity(false);
		uint32_t offset = _spiFlash->Offset(SPI_FLASH_SECTION_MAC);
		if (_spiFlash->Read(offset, macData, 16))
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			if (macData.size() < 16)
				return false;

			mac1.mac[0] = macData.at(3);
			mac1.mac[1] = macData.at(2);
			mac1.mac[2] = macData.at(1);
			mac1.mac[3] = macData.at(0);
			mac1.mac[4] = macData.at(7);
			mac1.mac[5] = macData.at(6);

			mac2.mac[0] = macData.at(8+3);
			mac2.mac[1] = macData.at(8+2);
			mac2.mac[2] = macData.at(8+1);
			mac2.mac[3] = macData.at(8+0);
			mac2.mac[4] = macData.at(8+7);
			mac2.mac[5] = macData.at(8+6);
			return true;
		}
		else
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			return false;
		}
	}
	else
	{
		uint32_t baseAddress = GetBaseAddressForProgramming(MAC_FLASHBLOCK);
		SetFlashBlockIDBank(MAC_FLASHBLOCK);

		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, lo);
		baseAddress += 4;

		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, hi);
		baseAddress += 4;

		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, lo2);
		baseAddress += 4;

		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, hi2);

		SetBankSelect(BANK_0);

		//if (lo == 0xffffffff && hi == 0xffffffff && lo2 == 0xffffffff && hi2 == 0xffffffff)
		   //return false;

		mac1.mac[0] = (lo & 0xff000000) >> 24;
		mac1.mac[1] = (lo & 0x00ff0000) >> 16;
		mac1.mac[2] = (lo & 0x0000ff00) >> 8;
		mac1.mac[3] =  lo & 0x000000ff;
		mac1.mac[4] = (hi & 0xff000000) >> 24;
		mac1.mac[5] = (hi & 0x00ff0000) >> 16;

		mac2.mac[0] = (lo2 & 0xff000000) >> 24;
		mac2.mac[1] = (lo2 & 0x00ff0000) >> 16;
		mac2.mac[2] = (lo2 & 0x0000ff00) >> 8;
		mac2.mac[3] =  lo2 & 0x000000ff;
		mac2.mac[4] = (hi2 & 0xff000000) >> 24;
		mac2.mac[5] = (hi2 & 0x00ff0000) >> 16;
	}

	return true;
}

bool CNTV2KonaFlashProgram::ProgramLicenseInfo (const string & licenseString)
{
	if(!IsIPDevice())
		return false;

	if (_spiFlash)
	{
		vector<uint8_t> licenseData;
		for (string::const_iterator it(licenseString.begin());  it != licenseString.end();  ++it)
			licenseData.push_back(uint8_t(*it));
		licenseData.push_back(0);

		bool oldVerboseMode = _spiFlash->GetVerbosity();
		_spiFlash->SetVerbosity(false);
		uint32_t offset = _spiFlash->Offset(SPI_FLASH_SECTION_LICENSE);
		_spiFlash->Erase(offset, uint32_t(licenseData.size()));
		if (_spiFlash->Write(offset, licenseData, uint32_t(licenseData.size())))
			_spiFlash->SetVerbosity(oldVerboseMode);
		else
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			return false;
		}
	}
	else
	{

		EraseBlock(LICENSE_BLOCK);

		SetFlashBlockIDBank(LICENSE_BLOCK);

		ULWord sectorAddress = GetBaseAddressForProgramming(LICENSE_BLOCK);

		size_t len (licenseString.size());
		size_t words ((len/4) + 2);
		NTV2Buffer data8(words*4);
		ULWord	* data32 = data8;
		data8.Fill(0x0000);
		strcat(data8,licenseString.c_str());

		SetBankSelect(BANK_1);

		for (size_t i(0);  i < words;  i++)
		{
			ProgramFlashValue(sectorAddress, data32[i]);
			sectorAddress += 4;
		}

		// Protect Device
		WriteCommand( WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x1C);
		WriteCommand(WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		WriteCommand(WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x9C);
		WriteCommand(WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		SetBankSelect(BANK_0);
	}
	return true;
}

bool CNTV2KonaFlashProgram::ReadLicenseInfo(string& serialString)
{
	const uint32_t maxSize = 100;

	if(!IsIPDevice())
		return false;

	if (_spiFlash)
	{
		vector<uint8_t> licenseData;
		bool oldVerboseMode = _spiFlash->GetVerbosity();
		uint32_t offset = _spiFlash->Offset(SPI_FLASH_SECTION_LICENSE);
		_spiFlash->SetVerbosity(false);
		if (_spiFlash->Read(offset, licenseData, maxSize))
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			serialString = "";
			if (licenseData.size() < 4)
				return false;
			else if (licenseData[0] == 0xff  &&  licenseData[1] == 0xff  &&  licenseData[2] == 0xff  &&  licenseData[3] == 0xff)
				return false;
			else
			{
				serialString.assign(licenseData.begin(), licenseData.end());

				// remove any trailing nulls
				size_t found = serialString.find('\0');
				if (found != string::npos)
					serialString.resize(found);
			}
		}
		else
		{
			_spiFlash->SetVerbosity(oldVerboseMode);
			return false;
		}
	}
	else
	{
		ULWord license[maxSize];
		memset (license,0x0,sizeof(license));

		uint32_t baseAddress = GetBaseAddressForProgramming(LICENSE_BLOCK);
		SetFlashBlockIDBank(LICENSE_BLOCK);

		bool terminated = false;
		bool good = false;
		for(uint32_t i = 0; i < maxSize; i++)
		{
			WriteRegister(kRegXenaxFlashAddress,baseAddress);
			WriteCommand(READFAST_COMMAND);
			WaitForFlashNOTBusy();
			ReadRegister(kRegXenaxFlashDOUT, license[i]);
			if (license[i] == 0xffffffff)
			{
				good = true; // uninitialized memory
				break;
			}
			if (license[i] == 0)
			{
				good	   = true;
				terminated = true;
				break;
			}
			baseAddress += 4;
		}

		std::string res;
		if (terminated)
			res = reinterpret_cast<char*>(license);

		serialString = res;
		return good;
	}
	return true;
}

bool CNTV2KonaFlashProgram::SetBankSelect( BankSelect bankNumber )
{
	if (ROMHasBankSelect())
	{
		WriteCommand(WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashAddress, uint32_t(bankNumber));
		
		WriteCommand(_hasExtendedCommandSupport ? EXTENDEDADDRESS_COMMAND : BANKSELECT_COMMMAND);
		WaitForFlashNOTBusy();
		KFPDBUG ("selected bank: " << ReadBankSelect());
	}
	return true;
}

uint32_t CNTV2KonaFlashProgram::ReadBankSelect()
{
	uint32_t bankNumber = 0;
	if (ROMHasBankSelect())
	{
		WriteCommand(_hasExtendedCommandSupport ? READEXTENDEDADDRESS_COMMAND : READBANKSELECT_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, bankNumber);
	}
	return bankNumber&0xf;
}

bool CNTV2KonaFlashProgram::SetMCSFile (const string & inMCSFileName)
{
	if (!_bQuiet)
		cout << "Parsing MCS File" << endl;
	return _mcsFile.Open(inMCSFileName.c_str());
}

bool CNTV2KonaFlashProgram::ProgramFromMCS(bool verify)
{
	if (!_mcsFile.isReady())
		{cerr << "MCS bitfile not open" << endl;  return false;}
	if (!IsOpen())
		{cerr << "Device not open" << endl;	return false;}

	if (_spiFlash)
	{
		if (!_mcsFile.isReady())
			{cerr << "MCS file not ready" << endl;	return false;}

		// now the main FPGA part
		_flashID = MAIN_FLASHBLOCK;

		vector<uint8_t> fpgaData;
		uint16_t fpgaPartitionOffset = 0;

		_mcsFile.GetPartition(fpgaData, 0x0000, fpgaPartitionOffset, false);
		_bitFileSize = uint32_t(fpgaData.size());
		// +_256 for fastFlash Programming
		_bitFileBuffer.Allocate(_bitFileSize + 512);
		_bitFileBuffer.Fill(0xFFFFFFFF);
		::memcpy(_bitFileBuffer, &fpgaData[0], _bitFileSize);

		// Parse header to make sure this is a xilinx bitfile.
		ostringstream msgs;
		if (!_parser.ParseHeader(_bitFileBuffer, msgs))
			{cerr << "Can't parse header" << endl << msgs.str() << endl;  return false;}

		// handle the fpga part
		string msg = Program(verify);
		// handle the SOC part
		return ProgramSOC(verify);
	}

	bool hasErasedSOCs = false;
	uint16_t linearOffsetToBankOffset = 0x0000;
	uint16_t basePartitionAddress = linearOffsetToBankOffset;
	bool bPartitionValid = true;
	uint32_t partitionCount = 0;
	while (bPartitionValid)
	{
		WriteRegister(kVRegFlashState,kProgramStateCalculating);
		WriteRegister(kVRegFlashSize,MCS_STEPS);
		WriteRegister(kVRegFlashStatus,ULWord(NextMcsStep()));

		uint16_t partitionOffset = 0;
		FlashBlockID blockID = MAIN_FLASHBLOCK;
		ParsePartitionFromFileLines(basePartitionAddress, partitionOffset);
		if (basePartitionAddress < 0x0100)
		{
			blockID = MAIN_FLASHBLOCK;
			linearOffsetToBankOffset = 0x0000;
			//Program Main
			if (!_bQuiet)
				cout << "Erase Main Bitfile Bank" << endl;
			WriteRegister(kVRegFlashState, kProgramStateEraseMainFlashBlock);
			EraseBlock(MAIN_FLASHBLOCK);
			WriteRegister(kVRegFlashState, kProgramStateProgramFlash);
		}
		else if (basePartitionAddress >= 0x0100 && basePartitionAddress < 0x0200)
		{
			blockID = MCS_INFO_BLOCK;
			linearOffsetToBankOffset = 0x0100;
			//Program Comment
			if (!_bQuiet)
				cout << "Erase Package Info Block" << endl;
			WriteRegister(kVRegFlashState, kProgramStateErasePackageInfo);
			EraseBlock(MCS_INFO_BLOCK);
			WriteRegister(kVRegFlashState, kProgramStateProgramPackageInfo);
		}
		else if (basePartitionAddress >= 0x0200 && basePartitionAddress < 0x0400)
		{
			if (!hasErasedSOCs)
			{
				if (!_bQuiet)
					cout << "Erase SOC Bank 1" << endl;
				WriteRegister(kVRegFlashState, kProgramStateEraseBank3);
				EraseBlock(SOC1_FLASHBLOCK);
				if (!_bQuiet)
					cout << "Erase SOC Bank 2" << endl;
				WriteRegister(kVRegFlashState, kProgramStateEraseBank4);
				EraseBlock(SOC2_FLASHBLOCK);
				hasErasedSOCs = true;
			}
			if (basePartitionAddress >= 0x0200 && basePartitionAddress < 0x0300)
			{
				blockID = SOC1_FLASHBLOCK;
				linearOffsetToBankOffset = 0x0200;
				WriteRegister(kVRegFlashState, kProgramStateProgramBank3);
			}
			else
			{
				blockID = SOC2_FLASHBLOCK;
				linearOffsetToBankOffset = 0x0300;
				WriteRegister(kVRegFlashState, kProgramStateProgramBank4);
			}
		}
		else
		{
			break;
		}

		uint16_t baseOffset = basePartitionAddress - linearOffsetToBankOffset;
		uint32_t programOffset = uint32_t(baseOffset) << 16 | partitionOffset;

		if (_bankSize == 0)
		{
			bPartitionValid = false;
			break;
		}

		SetFlashBlockIDBank(blockID);

		uint32_t baseAddress = GetBaseAddressForProgramming(blockID) + programOffset;
		if (blockID == MCS_INFO_BLOCK)
			baseAddress = _mcsInfoOffset;
		uint32_t bufferIndex = 0;
		uint32_t blockSize = 512;
		uint32_t dwordsPerBlock = blockSize / 4;
		uint32_t totalBlockCount ((uint32_t(_partitionBuffer.size()) + blockSize) / blockSize);
		uint32_t percentComplete = 0;

		WriteRegister(kVRegFlashSize, totalBlockCount);
		for (uint32_t blockCount = 0; blockCount < totalBlockCount; blockCount++)
		{
			WriteRegister(kVRegFlashStatus,blockCount);
			if (baseAddress == 0x01000000 && blockCount > 0 && blockID == SOC1_FLASHBLOCK)
			{
				blockID = SOC2_FLASHBLOCK;
				SetFlashBlockIDBank(blockID);
				baseAddress = GetBaseAddressForProgramming(blockID);
				WriteRegister(kVRegFlashState, kProgramStateProgramBank4);
			}
			uint32_t remainderBytes = static_cast<uint32_t>(_partitionBuffer.size() - bufferIndex);
			WriteCommand(WRITEENABLE_COMMAND);
			WaitForFlashNOTBusy();

			for (uint32_t dwordCount = 0; dwordCount < dwordsPerBlock; dwordCount++)
			{
				uint32_t partitionValue = 0xFFFFFFFF;
				if (remainderBytes >= 4)
				{
					partitionValue =	uint32_t(_partitionBuffer[bufferIndex + 0]) << 24 |
										uint32_t(_partitionBuffer[bufferIndex + 1]) << 16 |
										uint32_t(_partitionBuffer[bufferIndex + 2]) << 8 |
										uint32_t(_partitionBuffer[bufferIndex + 3]);
					bufferIndex += 4;
					remainderBytes -= 4;
				}
				else
				{
					switch (remainderBytes)
					{
						case 3:
							partitionValue = 0xff;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 0]) << 24;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 1]) << 16;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 2]) << 8;
							break;
						case 2:
							partitionValue = 0xffff;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 0]) << 24;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 1]) << 16;
							break;
						case 1:
							partitionValue = 0xffffff;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 0]) << 24;
							break;
						default:
							break;
					}
					remainderBytes = 0;
				}
				partitionValue = NTV2EndianSwap32(partitionValue);
				WriteRegister(kRegXenaxFlashDIN, partitionValue);
			}
			WriteRegister(kRegXenaxFlashAddress, baseAddress);
			WriteCommand(PAGEPROGRAM_COMMAND);

			WaitForFlashNOTBusy();

			baseAddress += blockSize;

			percentComplete = (blockCount * 100) / totalBlockCount;
			if (!_bQuiet)
				cout << "Partition " << DEC(partitionCount) << " program status: " << DEC(percentComplete) << "%\r" << flush;
		}
		if (!_bQuiet)
			cout << "Partition " << DEC(partitionCount) << " program status: 100%					" << endl;

		if (verify)
		{
			switch (blockID)
			{
				default:
				case MAIN_FLASHBLOCK:
					WriteRegister(kVRegFlashState,kProgramStateVerifyFlash);
					break;
				case SOC1_FLASHBLOCK:
					WriteRegister(kVRegFlashState,kProgramStateVerifyBank3);
					break;
				case SOC2_FLASHBLOCK:
					WriteRegister(kVRegFlashState,kProgramStateVerifyBank4);
					break;
				case MCS_INFO_BLOCK:
					WriteRegister(kVRegFlashState,kProgramStateVerifyPackageInfo);
					break;
			}

			if (!VerifySOCPartition(blockID, programOffset))
			{
				SetBankSelect(BANK_0);
				cerr << "Verify Error" << endl;
				return false;
			}
		}
		partitionCount++;

		IntelRecordInfo recordInfo;
		_mcsFile.GetCurrentParsedRecord(recordInfo);
		if (recordInfo.recordType != IRT_ELAR)
			bPartitionValid = false;
		else
			basePartitionAddress = recordInfo.linearAddress;

	}

	//Protect Device
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x1C);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();

	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x9C);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	SetBankSelect(BANK_0);

	SetWarmBootFirmwareReload(true);
	return true;
}

bool CNTV2KonaFlashProgram::ProgramSOC (const bool verify)
{
	if (!_mcsFile.isReady())
		{cerr << "MCS bitfile not open" << endl;  return false;}

	if (_spiFlash)
	{
		if (!IsOpen())
			{cerr << "Device not open" << endl;  return false;}
		if (!_mcsFile.isReady())
			{cerr << "MCS bitfile not ready" << endl;  return false;}

		vector<uint8_t> ubootData;
		vector<uint8_t> imageData;
		vector<uint8_t> mcsInfoData;
		uint16_t ubootPartitionOffset = 0;
		uint16_t imagePartitionOffset = 0;
		uint16_t mcsInfoPartitionOffset = 0;
		_mcsFile.GetPartition(ubootData, 0x0400, ubootPartitionOffset, false);
		if (ubootData.empty())
			{cerr << "Could not find uboot data in MCS file" << endl;  return false;}

		_mcsFile.GetPartition(imageData, 0x0410, imagePartitionOffset, false);
		if (imageData.empty())
			{cerr << "Could not find kernel data in MCS file" << endl;  return false;}

		_mcsFile.GetPartition(mcsInfoData, 0x05F4, mcsInfoPartitionOffset, false);
		if (mcsInfoData.empty())
			{cerr << "Could not find mcs info in MCS file" << endl;  return false;}

		uint32_t ubootFlashOffset = _spiFlash->Offset(SPI_FLASH_SECTION_UBOOT);
		uint32_t imageFlashOffset = _spiFlash->Offset(SPI_FLASH_SECTION_KERNEL);
		uint32_t mcsFlashOffset	  = _spiFlash->Offset(SPI_FLASH_SECTION_MCSINFO);

		uint32_t ubootSize = uint32_t(ubootData.size());
		uint32_t imageSize = uint32_t(imageData.size());
		uint32_t mcsInfoSize = uint32_t(mcsInfoData.size());

		// erase uboot
		_spiFlash->Erase(ubootFlashOffset, ubootSize);

		// write uboot
		_spiFlash->Write(ubootFlashOffset, ubootData, ubootSize);

		// verify uboot
		if (verify)
			_spiFlash->Verify(ubootFlashOffset, ubootData);

		// erase image
		_spiFlash->Erase(imageFlashOffset, imageSize);

		// write image
		_spiFlash->Write(imageFlashOffset, imageData, imageSize);

		// verify image
		if (verify)
			_spiFlash->Verify(imageFlashOffset, imageData);

		// erase mcs info
		_spiFlash->Erase(mcsFlashOffset, mcsInfoSize);

		// write mcs info
		_spiFlash->Write(mcsFlashOffset, mcsInfoData, mcsInfoSize);

		// verify mcs info
		if (verify)
			_spiFlash->Verify(mcsFlashOffset, mcsInfoData);
		return true;
	}	//	if _spiFlash

	//	Not _spiFlash:
	if (!IsOpen())
		{cerr << "Device not open" << endl;  return false;}
	if (!_bQuiet)
		cout << "Erase SOC Bank 1" << endl;
	EraseBlock(SOC1_FLASHBLOCK);
	if (!_bQuiet)
		cout << "Erase SOC Bank 2" << endl;
	EraseBlock(SOC2_FLASHBLOCK);

	//1st partition is assumed to be at 32M mark
	//the 32bit address is 0x02000000
	//the ELAR address is 0x0200
	uint16_t partition32M = 0x0200;
	uint16_t basePartitionAddress = partition32M;
	bool bPartitionValid = true;
	uint32_t partitionCount = 0;
	while (bPartitionValid)
	{
		uint16_t partitionOffset = 0;
		ParsePartitionFromFileLines(basePartitionAddress, partitionOffset);
		uint16_t baseOffset = basePartitionAddress - partition32M;
		uint32_t programOffset = uint32_t(baseOffset) << 16 | partitionOffset;

		FlashBlockID blockID = SOC1_FLASHBLOCK;
		if (programOffset >= 0x01000000)
			blockID	 = SOC2_FLASHBLOCK;

		if (_bankSize == 0)
			return true;

		SetFlashBlockIDBank(blockID);
		uint32_t baseAddress = GetBaseAddressForProgramming(blockID) + programOffset;
		uint32_t bufferIndex = 0;
		uint32_t blockSize = 512;
		uint32_t dwordsPerBlock = blockSize / 4;
		uint32_t totalBlockCount = static_cast<uint32_t>((_partitionBuffer.size() + blockSize) / blockSize);
		uint32_t percentComplete = 0;
		for (uint32_t blockCount = 0; blockCount < totalBlockCount; blockCount++)
		{
			if (baseAddress == 0x01000000 && blockCount > 0)
			{
				SetFlashBlockIDBank(SOC2_FLASHBLOCK);
				baseAddress = GetBaseAddressForProgramming(blockID);
			}
			uint32_t remainderBytes = static_cast<uint32_t>(_partitionBuffer.size() - bufferIndex);
			WriteCommand(WRITEENABLE_COMMAND);
			WaitForFlashNOTBusy();

			for (uint32_t dwordCount = 0; dwordCount < dwordsPerBlock; dwordCount++)
			{
				uint32_t partitionValue = 0xFFFFFFFF;
				if (remainderBytes >= 4)
				{
					partitionValue =	uint32_t(_partitionBuffer[bufferIndex + 0]) << 24 |
										uint32_t(_partitionBuffer[bufferIndex + 1]) << 16 |
										uint32_t(_partitionBuffer[bufferIndex + 2]) << 8 |
										uint32_t(_partitionBuffer[bufferIndex + 3]);
					bufferIndex += 4;
					remainderBytes -= 4;
				}
				else
				{
					switch (remainderBytes)
					{
						case 3:
							partitionValue = 0xff;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 0]) << 24;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 1]) << 16;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 2]) << 8;
							break;
						case 2:
							partitionValue = 0xffff;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 0]) << 24;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 1]) << 16;
							break;
						case 1:
							partitionValue = 0xffffff;
							partitionValue |= uint32_t(_partitionBuffer[bufferIndex + 0]) << 24;
							break;
						default:
							break;
					}
					remainderBytes = 0;
				}
				partitionValue = NTV2EndianSwap32(partitionValue);
				WriteRegister(kRegXenaxFlashDIN, partitionValue);
			}	//	for dwordCount
			WriteRegister(kRegXenaxFlashAddress, baseAddress);
			WriteCommand(PAGEPROGRAM_COMMAND);

			WaitForFlashNOTBusy();

			baseAddress += blockSize;

			percentComplete = (blockCount * 100) / totalBlockCount;
			if (!_bQuiet)
				cout << "Partition " << DEC(partitionCount+2) << " program status: " << DEC(percentComplete) << "%\r" << flush;
		}	//	for blockCount
		if (!_bQuiet)
			cout << "Partition " << DEC(partitionCount+2) << " program status: 100%					" << endl;

		if (verify  &&  !VerifySOCPartition(blockID, programOffset))
		{
			SetBankSelect(BANK_0);
			cerr << "Verify failed" << endl;
			return false;
		}

		partitionCount++;
		IntelRecordInfo recordInfo;
		_mcsFile.GetCurrentParsedRecord(recordInfo);
		if (recordInfo.recordType != IRT_ELAR)
			bPartitionValid = false;
		else
			basePartitionAddress = recordInfo.linearAddress;
	}	//	while bPartitionValid

	//Protect Device
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x1C);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();

	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x9C);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	SetBankSelect(BANK_0);
	return true;
}

static bool getFileSize (const string & fileName, size_t & outSizeBytes)
{
	outSizeBytes = 0;
	ifstream ifs(fileName.c_str(),  ios::binary | ios::in);
	if (ifs.fail())
		return false;	//	open failed
	if (!ifs.seekg (0, ios::end))
		return false;	//	seek failed
	ifstream::pos_type curOffset(ifs.tellg());
	if (int(curOffset) == -1)
		return false;
	outSizeBytes = size_t(curOffset);
	return true;
}

bool CNTV2KonaFlashProgram::ProgramCustom (const string &sCustomFileName, const uint32_t addr, ostream & outMsgs)
{
	if (!IsOpen())
		{outMsgs << "Device not open" << endl;  return false;}

	if (_spiFlash)
	{
		vector<uint8_t> writeData;
		size_t sz(0),  maxFlashSize(_spiFlash->Size());
		// open file and read data
		if (!getFileSize(sCustomFileName, sz))
			{outMsgs << "getFileSize failed for '" << sCustomFileName << "'" << endl;  return false;}
		if (sz > maxFlashSize)
			{outMsgs << "File size " << DEC(sz) << " exceeds max flash size " << DEC(maxFlashSize) << endl;  return false;}

		ifstream ifs(sCustomFileName.c_str(),  ios::binary | ios::in);
		if (ifs.fail())
			{outMsgs << "Unable to open file '" << sCustomFileName << "'" << endl;  return false;}

		writeData.resize(sz);
		ifs.read(reinterpret_cast<char*>(&writeData[0]), streamsize(sz));
		if (!ifs.good())
			{outMsgs << "Error reading data from file '" << sCustomFileName << "'" << endl;  return false;}

		// erase flash
		uint32_t writeSize = uint32_t(writeData.size());
		bool eraseGood = _spiFlash->Erase(addr, writeSize);
		if (!eraseGood)
			{outMsgs << "Error erasing sectors, addr=" << xHEX0N(addr,8) << " length=" << DEC(writeSize) << endl;  return false;}

		// write flash
		_spiFlash->Write(addr, writeData, writeSize);
		
		bool result = true, verify = true;
		if (verify)
        {
            result = _spiFlash->Verify(addr, writeData);
        }
		return result;
	}	//	if _spiFlash
	else
	{
		static const size_t MAX_CUSTOM_FILE_SIZE (8<<20); // 1M
		NTV2Buffer customFileBuffer(MAX_CUSTOM_FILE_SIZE);
		size_t customSize(0), sz(0);

		uint32_t bank(addr / _bankSize),  offset(addr % _bankSize);
		if (offset + customSize > _bankSize)
			{outMsgs << "Custom write spans banks -- unsupported";  return false;}
		if (offset % _sectorSize)
			{outMsgs << "Write not on sector boundary -- unsupported";  return false;}
		if (!getFileSize(sCustomFileName, sz))
			{outMsgs << "Error getting file size for '" << sCustomFileName << "'";  return false;}
		if (sz > MAX_CUSTOM_FILE_SIZE)
			{outMsgs << "File size " << DEC(sz) << " exceeds max supported size " << DEC(MAX_CUSTOM_FILE_SIZE);  return false;}

		ifstream ifs(sCustomFileName.c_str(),  ios::binary | ios::in);
		if (!ifs.fail())
			{outMsgs << "Unable to open file '" << sCustomFileName << "'" << endl;  return false;}

		customSize = size_t(ifs.readsome(customFileBuffer, streamsize(customFileBuffer.GetByteCount())));
		if (!customSize)
			{outMsgs << "No data read from custom file '" << sCustomFileName << "'" << endl;  return false;}

		static const BankSelect BankIdxToBankSelect[] = {BANK_0, BANK_1, BANK_2, BANK_3};

		SetBankSelect(BankIdxToBankSelect[bank]);
		WriteCommand(WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x0);
		WriteCommand(WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();

		const uint32_t customSectors ((uint32_t(customSize) + _sectorSize - 1) / (_sectorSize));
		for (uint32_t i(0);  i < customSectors;  i++)
		{
			if (!_bQuiet)
				cout << "Erasing sectors - " << DECN(i,3) << " of " << DECN(customSectors,3) << "\r" << flush;
			EraseSector(offset + (i * _sectorSize));
		}

		WriteCommand(WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();

		uint32_t	blockSize		(512);
		uint32_t	dwordsPerBlock	(blockSize / 4);
		uint32_t	totalBlockCount	((uint32_t(customSize) + blockSize - 1) / blockSize);
		uint32_t	percentComplete	(0);
		uint32_t	baseAddress		(offset);
		size_t		remainderBytes	(customSize);
		uint32_t	bufferIndex		(0);
		for (uint32_t blockCount(0);  blockCount < totalBlockCount;  blockCount++)
		{
			WriteCommand(WRITEENABLE_COMMAND);
			WaitForFlashNOTBusy();

			for (uint32_t dwordCount(0);  dwordCount < dwordsPerBlock;  dwordCount++)
			{
				uint32_t partitionValue = 0xFFFFFFFF;
				if (remainderBytes >= 4)
				{
					partitionValue =	uint32_t(_customFileBuffer[bufferIndex + 0]) << 24
									|	uint32_t(_customFileBuffer[bufferIndex + 1]) << 16
									|	uint32_t(_customFileBuffer[bufferIndex + 2]) << 8
									|	uint32_t(_customFileBuffer[bufferIndex + 3]);
					bufferIndex += 4;
					remainderBytes -= 4;
				}
				else switch (remainderBytes)
				{
					case 3:
						partitionValue = 0xff;
						partitionValue |= uint32_t(_customFileBuffer[bufferIndex + 0]) << 24;
						partitionValue |= uint32_t(_customFileBuffer[bufferIndex + 1]) << 16;
						partitionValue |= uint32_t(_customFileBuffer[bufferIndex + 2]) << 8;
						remainderBytes = 0;
						break;
					case 2:
						partitionValue = 0xffff;
						partitionValue |= uint32_t(_customFileBuffer[bufferIndex + 0]) << 24;
						partitionValue |= uint32_t(_customFileBuffer[bufferIndex + 1]) << 16;
						remainderBytes = 0;
						break;
					case 1:
						partitionValue = 0xffffff;
						partitionValue |= uint32_t(_customFileBuffer[bufferIndex + 0]) << 24;
						remainderBytes = 0;
						break;
					default:
						break;
				}
				partitionValue = NTV2EndianSwap32(partitionValue);
				WriteRegister(kRegXenaxFlashDIN, partitionValue);
			}	//	for each dword
			WriteRegister(kRegXenaxFlashAddress, baseAddress);
			WriteCommand(PAGEPROGRAM_COMMAND);
			WaitForFlashNOTBusy();

			baseAddress += blockSize;

			percentComplete = (blockCount * 100) / totalBlockCount;
			if (!_bQuiet)
				cout << "Program status: " << DEC(percentComplete) << "% (" << DECN(blockCount,4) << " of " << DECN(totalBlockCount,4) << " blocks)\r" << flush;
		}	//	for each block

		//Protect Device
		WriteCommand(WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x1C);
		WriteCommand(WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();

		WriteCommand(WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x9C);
		WriteCommand(WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		SetBankSelect(BANK_0);
	}	//	else !_spiFlash
	return true;
}

bool CNTV2KonaFlashProgram::ProgramKonaxMB (const string &sCustomFileName, const uint32_t addr, ostream & outMsgs)
{
    if (DEVICE_IS_KONAX(_boardID))
    {
        _spiFlash = new CNTV2AxiSpiFlash(GetIndexNumber(), !_bQuiet);
    }
    else
        return false;
    
    return ProgramCustom(sCustomFileName, addr, outMsgs);
}

bool CNTV2KonaFlashProgram::SetFlashBlockIDBank(FlashBlockID blockID)
{
	BankSelect bankID = BANK_0;
	switch (blockID)
	{
	case MAIN_FLASHBLOCK:
		bankID = BANK_0;
		break;
	case FAILSAFE_FLASHBLOCK:
		bankID = ::NTV2DeviceGetSPIFlashVersion(_boardID) >= 5  ?  BANK_2 : BANK_1;
		break;
	case MCS_INFO_BLOCK:
	case MAC_FLASHBLOCK:
	case LICENSE_BLOCK:
		bankID = BANK_1;
		break;
	case SOC1_FLASHBLOCK:
		bankID = BANK_2;
		break;
	case SOC2_FLASHBLOCK:
		bankID = BANK_3;
		break;
	default:
		return false;
	}
	return SetBankSelect(bankID);
}

bool CNTV2KonaFlashProgram::ROMHasBankSelect()
{
	return ::NTV2DeviceROMHasBankSelect(_boardID);
}

void CNTV2KonaFlashProgram::ParsePartitionFromFileLines(uint32_t address, uint16_t & partitionOffset)
{
	_partitionBuffer.clear();
	_partitionBuffer.resize(0);
	bool getnext = false;
	if (address != 0x0000 && address != 0x0200)
		getnext = true;
	_mcsFile.GetPartition(_partitionBuffer, uint16_t(address), partitionOffset, getnext);
	_bankSize = uint32_t(_partitionBuffer.size());
	return;
}

bool CNTV2KonaFlashProgram::VerifySOCPartition(FlashBlockID flashID, uint32_t flashBlockOffset)
{
	SetFlashBlockIDBank(flashID);

	uint32_t errorCount = 0;
	uint32_t baseAddress = flashBlockOffset;

	uint32_t dwordsPerPartition = _bankSize / 4;
	uint32_t percentComplete = 0;
	uint32_t bufferIndex = 0;
	WriteRegister(kVRegFlashSize,dwordsPerPartition);
	for (uint32_t dwordCount = 0; dwordCount < dwordsPerPartition; dwordCount += 100)//dwordCount++)
	{
		WriteRegister(kVRegFlashStatus,dwordCount);
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, flashValue);
		uint32_t partitionValue = uint32_t(_partitionBuffer[bufferIndex + 0]) << 24
								| uint32_t(_partitionBuffer[bufferIndex + 1]) << 16
								| uint32_t(_partitionBuffer[bufferIndex + 2]) << 8
								| uint32_t(_partitionBuffer[bufferIndex + 3]);
		partitionValue = NTV2EndianSwap32(partitionValue);
		bufferIndex += 400;//4;
		baseAddress += 400;//4;

		if (flashValue != partitionValue)
		{
			cerr << "Error " << DEC(dwordCount) << " E(" << xHEX0N(partitionValue,8) << "),R(" << xHEX0N(flashValue,8) << ")" << endl;
			errorCount++;
			if (errorCount > 1)
				break;
		}

		percentComplete = (dwordCount * 100) / dwordsPerPartition;
		if (!_bQuiet)
			cout << "Program verify: " << DEC(percentComplete) << "%\r" << flush;
	}

	if (errorCount)
	{
		if (!_bQuiet)
			cerr << "Program verify failed: " << DEC(percentComplete) << "%" << endl;
		return false;
	}
	else if (!_bQuiet)
		cout << "Program verify: 100%					 " << endl;
	return true;
}

void CNTV2KonaFlashProgram::DisplayData(uint32_t address, uint32_t count)
{
#define WORDS_PER_LINE 4

	uint32_t bank	=  address / _bankSize;
	uint32_t offset =  address % _bankSize;
	SetBankSelect(BankSelect(bank));

	char line[1024];
	memset(line, 0, 1024);
	char * pLine = &line[0];
	pLine += sprintf(pLine, "%08x: ", uint32_t((bank * _bankSize) + offset));
	
	int32_t lineCount = 0; 
	for (uint32_t i = 0; i < count; i++, offset += 4)
	{
		WriteRegister(kRegXenaxFlashAddress, offset);
		WriteCommand(READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, flashValue);
		flashValue = NTV2EndianSwap32(flashValue);
		pLine += sprintf(pLine, "%08x  ", uint32_t(flashValue));
		if (++lineCount == WORDS_PER_LINE)
		{
			if (!_bQuiet)
				cout << line << endl;
			memset(line, 0, 1024);
			pLine = &line[0];
			pLine += sprintf(pLine, "%08x: ", uint32_t((bank * _bankSize) + offset + 4));
			lineCount = 0;
		}
	}
	if (!_bQuiet && lineCount != 0)
		cout << line << endl;
}

bool CNTV2KonaFlashProgram::FullProgram (vector<uint8_t> & dataBuffer)
{
	if (!IsOpen())
		return false;
	uint32_t baseAddress = 0;
	if (!_bQuiet)
		cout << "Erasing ROM" << endl;
	EraseChip();
	BankSelect currentBank = BANK_0;
	SetBankSelect(currentBank);

	uint32_t* bitFilePtr = reinterpret_cast<uint32_t*>(dataBuffer.data());
	uint32_t twoFixtysixBlockSizeCount = uint32_t((dataBuffer.size()+256)/256);
	uint32_t percentComplete = 0;
	WriteRegister(kVRegFlashState, kProgramStateProgramFlash);
	WriteRegister(kVRegFlashSize, twoFixtysixBlockSizeCount);
	for ( uint32_t count = 0; count < twoFixtysixBlockSizeCount; count++, baseAddress += 256, bitFilePtr += 64 )
	{
		if (baseAddress == _bankSize)
		{
			baseAddress = 0;
			switch(currentBank)
			{
				case BANK_0:	currentBank = BANK_1;	break;
				case BANK_1:	currentBank = BANK_2;	break;
				case BANK_2:	currentBank = BANK_3;	break;
				case BANK_3:	currentBank = BANK_0;	break;
			}
			SetBankSelect(currentBank);
		}
		FastProgramFlash256(baseAddress, bitFilePtr);
		percentComplete = (count*100)/twoFixtysixBlockSizeCount;

		WriteRegister(kVRegFlashStatus, count);
		if(!_bQuiet && (count%100 == 0))
			cout << "Program status: " << DEC(percentComplete) << "%\r" << flush;
	}
	if(!_bQuiet)
		cout << "Program status: 100%				   " << endl;

	// Protect Device
	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x1C);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();

	SetBankSelect(BANK_0);

	WriteCommand(WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x9C);
	WriteCommand(WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	SetBankSelect(BANK_0);
	SetWarmBootFirmwareReload(true);
	return true;
}

bool CNTV2KonaFlashProgram::CheckAndFixMACs()
{
	MacAddr mac1, mac2;
	ReadMACAddresses(mac1, mac2);
	if(mac1.mac[1] != 0x0C || mac2.mac[1] != 0x0c)
	{
		if (!_bQuiet)
			cout << "Reprogramming the Mac Addresses!" << endl;
		string serialString;
		GetSerialNumberString(serialString);
		MakeMACsFromSerial(serialString.c_str(), &mac1, &mac2);
		return ProgramMACAddresses(&mac1, &mac2);
	}
	return true;
}

bool CNTV2KonaFlashProgram::MakeMACsFromSerial( const char *sSerialNumber, MacAddr *pMac1, MacAddr *pMac2)
{
	// NOTE: We do both auto if either is auto
	// TODO: Check if this is an IP board, etc etc
	if (strstr(sSerialNumber, "demo") == sSerialNumber)
	{	// If the serial number begins with demo
		int demoNum = 0;
		if (sscanf(sSerialNumber + 4, "%d", &demoNum) != 1)
			return false;
		if ((demoNum < 1) || (demoNum > 128))
			{cerr << "WARNING: Outside serial numbers demo0001 to demo0128" << endl;  return false;}
		pMac2->mac[0] = pMac1->mac[0] = 0x0;
		pMac2->mac[1] = pMac1->mac[1] = 0x0c;
		pMac2->mac[2] = pMac1->mac[2] = 0x17;
		pMac2->mac[3] = pMac1->mac[3] = 0x88;
		pMac2->mac[4] = pMac1->mac[4] = 0x12;
		pMac1->mac[5] = uint8_t((demoNum - 1) * 2);
		pMac2->mac[5] = pMac1->mac[5] + 1;
		return true;
	}
	else if (strstr(sSerialNumber, "1IP") == sSerialNumber)
	{	// If the serial number begins with 1IP
		// 00050 to 08241 (qty 8192) maps to A000 to DFFF (16384 addresses)
		// First 4 bytes are: 00:0c:17:42 and next 2 bytes are computed
		// as mac1=((serNo - 50)*2 + 0xA000) and 
		// mac2 = mac1 + 1
		int serNum = 0;
		if (sscanf(sSerialNumber + 4, "%d", &serNum) != 1)
			return false;
		if ((serNum < 50) || (serNum > 8241))
			{cerr << "WARNING: Outside serial numbers 1IP00050 to 1IP08241" << endl;  return false;}

		int mac16LSBs = (0xA000) + (serNum - 50) * 2;

		pMac2->mac[0] = pMac1->mac[0] = 0x0;
		pMac2->mac[1] = pMac1->mac[1] = 0x0c;
		pMac2->mac[2] = pMac1->mac[2] = 0x17;
		pMac2->mac[3] = pMac1->mac[3] = 0x42;
		pMac2->mac[4] = pMac1->mac[4] = uint8_t(mac16LSBs >> 8);
		pMac2->mac[5] = pMac1->mac[5] = mac16LSBs & 0x0ff;
		// The above byte will always be same for the second mac
		// based on above allocation
		pMac2->mac[5] = pMac1->mac[5] + 1;
		return true;
	}
	else if (strstr(sSerialNumber, "ENG") == sSerialNumber)
	{	// ENG IoIp - if the serial starts with ENG
		// 0000 to 0127 (qty 128) maps to 1B00 to 1BFF (256 addresses)
		// First 4 bytes are: 00:0c:17:88 and next 2 bytes are computed
		// as mac1= (0x1B00) + (serNum * 2) and
		// mac2 = mac1 + 1
		int serNum = 0;
		if (sscanf(sSerialNumber + 5, "%d", &serNum) != 1)
			return false;

		if (serNum > 127)
			{cerr << "WARNING: Outside serial numbers ENG00000 to ENG00127" << endl;  return false;}

		int mac16LSBs = (0x1B00) + (serNum * 2);

		pMac2->mac[0] = pMac1->mac[0] = 0x0;
		pMac2->mac[1] = pMac1->mac[1] = 0x0c;
		pMac2->mac[2] = pMac1->mac[2] = 0x17;
		pMac2->mac[3] = pMac1->mac[3] = 0x88;
		pMac2->mac[4] = pMac1->mac[4] = uint8_t(mac16LSBs >> 8);
		pMac2->mac[5] = pMac1->mac[5] = mac16LSBs & 0xff;
		// The above byte will always be same for the second mac
		// based on above allocation
		pMac2->mac[5] = pMac1->mac[5] + 1;
		return true;
	}
	else if (strstr(sSerialNumber, "6XT") == sSerialNumber)
	{	// IoIP and DNxIP serial numbers
		// IoIP	 Gen 1	[6XT000250 - 6XT008441]
		// DNxIP Gen 1	[6XT200250 - 6XT208441]
		// IoIP	 Gen 2	[6XT100250 - 6XT108441]
		// DNxIP Gen 2	[6XT300250 - 6XT308441]

		int serNum = 0;
		if (sscanf(sSerialNumber + 4, "%d", &serNum) != 1)
			return false;

		if ((serNum < 250) || (serNum > 8441))
			{cerr << "WARNING: Outside serial numbers range 250 to 8441" << endl;  return false;}

		// 0250 to 8441 (qty 8192) maps to 16384 addresses
		// First 3 bytes are: 00:0c:17 and next 3 bytes are computed
		int mac24LSBs = -1;
		if (strstr(sSerialNumber, "6XT0") == sSerialNumber)
		{
			mac24LSBs = (0x48A000) + ((serNum - 250) * 2);
		}
		else if (strstr(sSerialNumber, "6XT2") == sSerialNumber)
		{
			mac24LSBs = (0x48E000) + ((serNum - 250) * 2);
		}
		else if (strstr(sSerialNumber, "6XT1") == sSerialNumber)
		{
			mac24LSBs = (0x4B2000) + ((serNum - 250) * 2);
		}
		else if (strstr(sSerialNumber, "6XT3") == sSerialNumber)
		{
			mac24LSBs = (0x4B6000) + ((serNum - 250) * 2);
		}
		else
		{
			return false;
		}

		pMac2->mac[0] = pMac1->mac[0] = 0x0;
		pMac2->mac[1] = pMac1->mac[1] = 0x0c;
		pMac2->mac[2] = pMac1->mac[2] = 0x17;
		pMac2->mac[3] = pMac1->mac[3] = (mac24LSBs & 0xFF0000) >> 16;
		pMac2->mac[4] = pMac1->mac[4] = (mac24LSBs & 0x00FF00) >>  8;
		pMac2->mac[5] = pMac1->mac[5] = (mac24LSBs & 0x0000FF) >>  0;
		// The above byte will always be same for the second mac
		// based on above allocation
		pMac2->mac[5] = pMac1->mac[5] + 1;
		return true;
	}
	else
		cerr << "Unrecognized or unspecified serial number '" << sSerialNumber << "'" << endl;
	return false;
}



#ifdef MSWindows
#pragma warning(default: 4800)
#endif
