/**
	@file		ntv2konaflashprogram.cpp
	@brief		Implementation of CNTV2KonaFlashProgram class.
	@copyright	(C) 2010-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2konaflashprogram.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2endian.h"
#include "ntv2registers2022.h"

#ifdef MSWindows
	#pragma warning(disable: 4305) // Initialization warnings.
	#pragma warning(disable: 4309)
	#pragma warning(disable: 4800)
	#pragma warning(disable: 4996)
#endif
static unsigned char signature[8] = {0xFF,0xFF,0xFF,0xFF,0xAA,0x99,0x55,0x66};
static unsigned char head13[]   = { 0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01 };


using namespace std;

CNTV2KonaFlashProgram::CNTV2KonaFlashProgram ()
	:	CNTV2Card (),
		_bitFileBuffer		(NULL),
		_customFileBuffer	(NULL),
		_bitFileSize		(0),
		_flashSize			(0),
		_bankSize			(0),
		_sectorSize			(0),
        _mainOffset			(0),
		_failSafeOffset		(0),
        _macOffset          (0),
        _mcsInfoOffset      (0),
        _licenseOffset      (0),
        _soc1Offset         (0),
        _soc2Offset         (0),
        _numSectorsMain		(0),
        _numSectorsSOC1     (0),
        _numSectorsSOC2     (0),
		_numSectorsFailSafe	(0),
		_numBytes			(0),
        _flashID            (MAIN_FLASHBLOCK),
		_deviceID			(0),
		_bQuiet				(false),
        _mcsStep(0)
{
}

CNTV2KonaFlashProgram::CNTV2KonaFlashProgram (const UWord boardNumber, const bool displayErrorMessage, const UWord ulBoardType)
	:	CNTV2Card (boardNumber),
        _bitFileBuffer      (NULL),
        _customFileBuffer   (NULL),
        _bitFileSize        (0),
        _flashSize          (0),
        _bankSize           (0),
        _sectorSize         (0),
        _mainOffset         (0),
        _failSafeOffset     (0),
        _macOffset          (0),
        _mcsInfoOffset      (0),
        _licenseOffset      (0),
        _soc1Offset         (0),
        _soc2Offset         (0),
        _numSectorsMain     (0),
        _numSectorsSOC1     (0),
        _numSectorsSOC2     (0),
        _numSectorsFailSafe (0),
        _numBytes           (0),
        _flashID            (MAIN_FLASHBLOCK),
        _deviceID           (0),
        _bQuiet             (false),
        _mcsStep            (0)
{
	(void) displayErrorMessage;	//	unused
	(void) ulBoardType;			//	unused
	SetDeviceProperties();
}

CNTV2KonaFlashProgram::~CNTV2KonaFlashProgram()
{
	if (_bitFileBuffer)
		delete [] _bitFileBuffer;
	if (_customFileBuffer)
		delete [] _customFileBuffer;
}

bool CNTV2KonaFlashProgram::SetBoard(UWord boardNumber, NTV2DeviceType boardType, uint32_t index)
{
	if (!Open (boardNumber, false, boardType))
		return false;

	// if board is a sarek with microblaze - ensure access to flash
	CNTV2Card	device;
	CNTV2DeviceScanner::GetDeviceAtIndex(boardNumber, device);
	if (device.IsKonaIPDevice())
	{
		uint32_t regVal = 0;
		device.ReadRegister(SAREK_REGS + kRegSarekFwCfg, &regVal);
		if (regVal & SAREK_MB_PRESENT)
		{
			// take access
			device.WriteRegister(SAREK_REGS + kRegSarekSpiSelect, 0x01);
		}
	}

	if (!SetDeviceProperties())
		return false;

	//For manufacturing use the leds to code the board number
	uint32_t ledMask = BIT(16)+BIT(17);
	uint32_t ledShift = 16;
	return WriteRegister(kRegGlobalControl, index, ledMask, ledShift);

}

bool CNTV2KonaFlashProgram::SetDeviceProperties()
{
	bool status;
	_spiDeviceID = ReadDeviceID();
	if (::NTV2DeviceHasSPIv2(_boardID))
	{
		_flashSize = 16 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 64 * 1024;
		_numSectorsMain = _flashSize / _sectorSize / 2;
		_numSectorsFailSafe = (_flashSize / _sectorSize / 2) - 1;
		_mainOffset = 0;
		_failSafeOffset = 8 * 1024 * 1024;
		_macOffset = _bankSize - (2 * _sectorSize);
		status = true;
	}
	else if (::NTV2DeviceHasSPIv3(_boardID))
	{
		if (_spiDeviceID == 0x010220)
		{
			//This is actually SPI v4 but needed this for NAB 2016
			_flashSize = 64 * 1024 * 1024;
			_bankSize = 16 * 1024 * 1024;
			_sectorSize = 256 * 1024;
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
			_flashSize = 32 * 1024 * 1024;
			_bankSize = 16 * 1024 * 1024;
			_sectorSize = 64 * 1024;
			_numSectorsMain = _flashSize / _sectorSize / 2;
			_numSectorsFailSafe = (_flashSize / _sectorSize / 2) - 1;
			_mainOffset = 0;
			_failSafeOffset = 0;// but is really 16*1024*1024;
			_macOffset = _bankSize - (2 * _sectorSize);
			_mcsInfoOffset = _bankSize - (3 * _sectorSize);
			_licenseOffset = _bankSize - (4* _sectorSize);
			status = true;
		}
	}
	else if (::NTV2DeviceHasSPIv4(_boardID) || _spiDeviceID == 0x010220)
	{
		//SPIV4 is a bigger SPIv3 2x
		_flashSize = 64 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 256 * 1024;
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
	}
	else
	{
		//This includes legacy boards such as LHi, Corvid 1...
		//SPI is devided up into 4 logical blocks of 4M each
		//Without history explained main is at offset 0 and failsafe is at offset 12
		_flashSize = 16 * 1024 * 1024;
		_bankSize = 16 * 1024 * 1024;
		_sectorSize = 64 * 1024;
		_numSectorsMain = _flashSize / _sectorSize / 4;
		_numSectorsFailSafe = (_flashSize / _sectorSize / 4) - 1;
		_mainOffset = 0;
		_failSafeOffset = 12 * 1024 * 1024;
		_macOffset = _bankSize - (2 * _sectorSize);
		status = true;
	}

	return status;
}

// SetBitFile(char *bitFileName)
// Sanity check on bitFileName and set filepointer to start
// of programming data - signature on bitfiles
void CNTV2KonaFlashProgram::SetBitFile(const char *bitFileName, FlashBlockID blockNumber)
{
	if ( _bitFileBuffer != NULL )
	{
		delete [] _bitFileBuffer;
		_bitFileBuffer = NULL;
	}
	_bitFileName = bitFileName;

	if ( blockNumber == AUTO_FLASHBLOCK )
		DetermingFlashTypeAndBlockNumberFromFileName(bitFileName);
	else if ( blockNumber >= MAIN_FLASHBLOCK && blockNumber <= FAILSAFE_FLASHBLOCK )
		_flashID = blockNumber;
	else
		throw "Invalid block number";

	FILE* pFile = 0;
	struct stat fsinfo;
	stat(bitFileName, &fsinfo);
	_bitFileSize = fsinfo.st_size;
	pFile = fopen(bitFileName, "rb");
	if(pFile)
	{
		// +_256 for fastFlash Programming
		_bitFileBuffer = new unsigned char[_bitFileSize+512];
		memset(_bitFileBuffer, 0xFF, _bitFileSize+512);

		fseek(pFile, 0, SEEK_SET);
		fread(_bitFileBuffer, 1, _bitFileSize, pFile);
		fclose(pFile);

		// Parse header to make sure this is a xilinx bitfile.
		if ( !ParseHeader((char *)_bitFileBuffer) )
			throw "Can't Parse Header";

	}
	else
	{
		throw "Bit file can't be opened";
	}

	if (!SetDeviceProperties())
		throw "Device Not Recognized";

// 	if( ((_designName.find("CORVID88") != string::npos) && (_boardID != DEVICE_ID_CORVID88)) ||
// 		((_designName.find("corvid1pcie") != string::npos) && (_boardID != DEVICE_ID_CORVID1)) ||
// 		((_designName.find("corvid1_3Gpcie") != string::npos) && (_boardID != DEVICE_ID_CORVID3G)) ||
// 		((_designName.find("top_c22") != string::npos) && (_boardID != DEVICE_ID_CORVID22)) ||
// 		((_designName.find("corvid24") != string::npos) && (_boardID != DEVICE_ID_CORVID24)) ||
// 		((_designName.find("corvid_44") != string::npos) && (_boardID != DEVICE_ID_CORVID44)) ||
// 		((_designName.find("chekov_00") != string::npos) && (_boardID != DEVICE_ID_IOEXPRESS)) ||
// 		((_designName.find("top_IO_TX") != string::npos) && (_boardID != DEVICE_ID_IOXT)) ||
// 		((_designName.find("IO_XT_4K") != string::npos) && (_boardID != DEVICE_ID_IO4K && _boardID != DEVICE_ID_IO4KUFC)) ||
// 		((_designName.find("K3G") != string::npos) &&  (_boardID != DEVICE_ID_KONA3G && _boardID != DEVICE_ID_KONA3GQUAD)) ||
// 		((_designName.find("kona_4") != string::npos) && (_boardID != DEVICE_ID_KONA4 && _boardID != DEVICE_ID_KONA4UFC)) ||
// 		((_designName.find("lhe") != string::npos) && (_boardID != DEVICE_ID_LHE_PLUS)) ||
// 		((_designName.find("top_pike") != string::npos) && (_boardID != DEVICE_ID_LHI)) ||
// 		((_designName.find("t_tap") != string::npos) && (_boardID != DEVICE_ID_TTAP)) )
// 	{
// 		throw "Incorrect BoardID";
// 	}

}

void CNTV2KonaFlashProgram::DetermingFlashTypeAndBlockNumberFromFileName(const char* bitFileName)
{

	_flashID = MAIN_FLASHBLOCK;
	if (strstr(bitFileName, "_fs_") != NULL)
		_flashID = FAILSAFE_FLASHBLOCK;
}

// parse header - return 'true' if header looks OK
bool CNTV2KonaFlashProgram::ParseHeader(char* headerAddress)
{
	bool headerOK = false;
	UWord fieldLen;
	char *p = headerAddress;

	do
	{
		// make sure 1st 13 bytes are what we expect
		if ( strncmp(p, (const char *)head13, 13) != 0 )
			break;

		p += 13;	// skip over header header

		// the next byte should be 'a'
		if (*p++ != 'a')
			break;

		fieldLen = htons(*((UWord *)p));		// the next 2 bytes are the length of the FileName (including /0)

		p += 2;							// now pointing at the beginning of the file name

		_designName = p;					// grab design name

		p += fieldLen;					// skip over file name - now pointing to beginning of 'b' field

		// the next byte should be 'b'
		if (*p++ != 'b')
			break;

		fieldLen = htons(*((UWord *)p));		// the next 2 bytes are the length of the Part Name (including /0)

		p += 2;							// now pointing at the beginning of the part name

		_partName = p;						// grab part name

		p += fieldLen;					// skip over part name - now pointing to beginning of 'c' field

		// the next byte should be 'c'
		if (*p++ != 'c')
			break;

		fieldLen = htons(*((UWord *)p));		// the next 2 bytes are the length of the date string (including /0)

		p += 2;							// now pointing at the beginning of the date string

		_date = p;						// grab date string

		p += fieldLen;					// skip over date string - now pointing to beginning of 'd' field

		// the next byte should be 'd'
		if (*p++ != 'd')
			break;

		fieldLen = htons(*((UWord *)p));		// the next 2 bytes are the length of the time string (including /0)

		p += 2;							// now pointing at the beginning of the time string

		_time = p;						// grab time string

		p += fieldLen;					// skip over time string - now pointing to beginning of 'e' field

		// the next byte should be 'e'
		if (*p++ != 'e')
			break;

		_numBytes = htonl(*((uint32_t *)p));		// the next 4 bytes are the length of the raw program data

		if ( _partName[0] == '5' || _partName[0] == '6' || _partName[0] == '7')
		{
			// still waiting for xilinx to explain this fully
			if(_partName[0] == '5' || (_partName[0] == '6' && _partName[1] == 'v'))
				p += 48;							// now pointing at the beginning of the identifier
			else if(_partName[0] == '7' && _partName[1] == 'k')
				p += 48;
			else
				p += 16;
		}
		else
		{
			p += 4;							// now pointing at the beginning of the identifier
		}

		// make sure the sync word is what we expect
		if ( strncmp(p, (const char*)signature, 8) != 0 )
			break;

		// if we made it this far it must be an OK Header - and it is parsed
		headerOK = true;

	} while(0);

	return headerOK;
}

bool CNTV2KonaFlashProgram::ReadHeader(FlashBlockID blockID)
{
	uint32_t baseAddress = GetBaseAddressForProgramming(blockID);
	SetFlashBlockIDBank(blockID);

	uint32_t* bitFilePtr =  new uint32_t[MAXBITFILE_HEADERSIZE/4];
	uint32_t dwordSizeCount = MAXBITFILE_HEADERSIZE/4;
	for ( uint32_t count = 0; count < dwordSizeCount; count++, baseAddress += 4 )
	{
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, &bitFilePtr[count]);
	}
	bool status = ParseHeader((char*)bitFilePtr);
	delete [] bitFilePtr;
	//Make sure to reset bank to lower
	SetBankSelect(BANK_0);

	return status;
}

bool CNTV2KonaFlashProgram::ReadInfoString()
{
	if (_spiDeviceID != 0x010220)
		return false;
	uint32_t baseAddress = _mcsInfoOffset;//GetBaseAddressForProgramming(MCS_INFO_BLOCK);
	SetFlashBlockIDBank(MCS_INFO_BLOCK);

	uint32_t* mcsInfoPtr = new uint32_t[MAXMCSINFOSIZE / 4];
	uint32_t dwordSizeCount = MAXMCSINFOSIZE / 4;
	for (uint32_t count = 0; count < dwordSizeCount; count++, baseAddress += 4)
	{
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, &mcsInfoPtr[count]);
		if (mcsInfoPtr[count] == 0)
			break;
	}
	_mcsInfo = (char*)mcsInfoPtr;
	delete[] mcsInfoPtr;
	//Make sure to reset bank to lower
	SetBankSelect(BANK_0);

	return true;
}

void CNTV2KonaFlashProgram::Program(bool verify)
{
	if ( _bitFileBuffer == NULL )
		throw "Bit File not Open";

	if (IsOpen ())
	{
		uint32_t baseAddress = GetBaseAddressForProgramming(_flashID);

		EraseBlock(_flashID);

		SetFlashBlockIDBank(_flashID);

		uint32_t* bitFilePtr = (uint32_t*)_bitFileBuffer;
		uint32_t twoFixtysixBlockSizeCount = (_bitFileSize+256)/256;
		int32_t percentComplete = 0;
		for ( uint32_t count = 0; count < twoFixtysixBlockSizeCount; count++, baseAddress += 256, bitFilePtr += 64 )
		{
			FastProgramFlash256(baseAddress, bitFilePtr);
			percentComplete = (count*100)/twoFixtysixBlockSizeCount;
			if(!_bQuiet)
			{
				printf("Program status: %i%%\r", percentComplete);
				fflush(stdout);
			}
		}
		if(!_bQuiet)
			printf("Program status: 100%%                  \n");

		// Protect Device
		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x1C);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();

		if (verify)
		{ 
  			if ( !VerifyFlash(_flashID) )
			{
				SetBankSelect(BANK_0);
  				throw "Program Didn't Verify";
			}
		}
		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x9C);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		SetBankSelect(BANK_0);
	}
	else
		throw "Board Can't be opened";
}

bool CNTV2KonaFlashProgram::ProgramFlashValue(uint32_t address, uint32_t value)
{
	WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, value);
	WriteRegister(kRegXenaxFlashAddress, address);
	WriteRegister(kRegXenaxFlashControlStatus, PAGEPROGRAM_COMMAND);
	WaitForFlashNOTBusy();

	return true;
}

bool CNTV2KonaFlashProgram::FastProgramFlash256(uint32_t address, uint32_t* buffer)
{
	WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	for ( uint32_t count=0; count < 64; count++ )
	{
		WriteRegister(kRegXenaxFlashDIN, *buffer++);
	}
	WriteRegister(kRegXenaxFlashAddress, address);
	WriteRegister(kRegXenaxFlashControlStatus, PAGEPROGRAM_COMMAND);
	WaitForFlashNOTBusy();

	return true;
}

uint32_t CNTV2KonaFlashProgram::ReadDeviceID()
{
	uint32_t deviceID = 0;
	if (IsOpen ())
	{
		WriteRegister(kRegXenaxFlashControlStatus, READID_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(61, &deviceID);
	}
	return (deviceID & 0xFFFFFF);
}

void CNTV2KonaFlashProgram::EraseBlock()
{
	if (IsOpen())
	{
		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x0);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();

		uint32_t numSectors = GetNumberOfSectors(_flashID);
		for (uint32_t sectorCount = 0; sectorCount < numSectors; sectorCount++ )
		{
			uint32_t address = GetSectorAddressForSector(_flashID, sectorCount);
			EraseSector(address);
		}
		//if ( !CheckFlashErased(flashBlockNumber))
		//	throw "Erase didn't work";

	}
	else
		throw "Board Not Open";
}

void CNTV2KonaFlashProgram::EraseBlock(FlashBlockID blockID)
{
	if (IsOpen ())
	{
		SetFlashBlockIDBank(blockID);

		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x0);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		int32_t percentComplete = 0;

		uint32_t numSectors = GetNumberOfSectors(blockID);
        WriteRegister(kVRegFlashSize,numSectors);
		for (uint32_t sectorCount = 0; sectorCount < numSectors; sectorCount++ )
		{
            WriteRegister(kVRegFlashStatus,sectorCount);
			uint32_t address = GetSectorAddressForSector(blockID, sectorCount);
			/*for ( int32_t i=0; i<4; i++, address += _sectorSize)*/
			EraseSector(address);
			percentComplete = (sectorCount*100)/numSectors;
			if(!_bQuiet)
			{
				printf("Erase status: %i%%\r", percentComplete);
				fflush(stdout);
			}
		}
        WriteRegister(kVRegFlashStatus,numSectors);
		if(!_bQuiet)
			printf("Erase status: 100%%             \n");
		//if ( !CheckFlashErasedWithBlockID(flashBlockNumber))
			//throw "Erase didn't work";
	}
	else
		throw "Board Not Open";

	SetBankSelect(BANK_0);
}

bool CNTV2KonaFlashProgram::EraseSector(uint32_t sectorAddress)
{
	WriteRegister(kRegXenaxFlashAddress, sectorAddress);
	WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();	
	WriteRegister(kRegXenaxFlashControlStatus, SECTORERASE_COMMAND);
	bool status = WaitForFlashNOTBusy();

	return status;
}

bool CNTV2KonaFlashProgram::EraseChip(UWord chip)
{
	(void) chip;	//	unused
	WriteRegister(kRegXenaxFlashControlStatus,0);
	WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x0);
	WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashControlStatus, CHIPERASE_COMMAND);
	bool status = WaitForFlashNOTBusy();

	return status;
}

bool CNTV2KonaFlashProgram::VerifyFlash(FlashBlockID flashID)
{
	uint32_t errorCount = 0;
	uint32_t baseAddress = GetBaseAddressForProgramming(flashID);

	uint32_t* bitFilePtr = (uint32_t*)_bitFileBuffer;
	uint32_t dwordSizeCount = (_bitFileSize+4)/4;
	int32_t percentComplete = 0;
	for (uint32_t count = 0; count < dwordSizeCount; count += 100, baseAddress += 400, bitFilePtr += 100)//count++, baseAddress += 4 )
	{
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, &flashValue);
		uint32_t bitFileValue = *bitFilePtr;//*bitFilePtr++;
		if ( flashValue != bitFileValue)
		{
			printf("Error %d E(%08X),R(%08X)\n", count, bitFileValue, flashValue);
			errorCount++;
			if ( errorCount > 1 )
				break;
		}
		percentComplete = (count*100)/dwordSizeCount;
		if(!_bQuiet)
		{
			printf("Program verify: %i%%\r", percentComplete);
			fflush(stdout);
		}
	}

	if ( errorCount )
	{
		if(!_bQuiet)
			printf("Program verify failed: %i%%\n", percentComplete);
		return false;
	}
	else
	{
		if(!_bQuiet)
			printf("Program verify: 100%%                    \n");
	}

	return true;
}

bool CNTV2KonaFlashProgram::WaitForFlashNOTBusy()
{
    bool busy  = true;

    do
    {
        uint32_t regValue;
		ReadRegister(kRegXenaxFlashControlStatus, &regValue);
        if( !(regValue & BIT(8)) )
        {
            busy = false;
            break;
        }
    } while(busy == true);//( (busy == true) && (count < 100) );

    return !busy;  // Return true if wait was successful
}

bool CNTV2KonaFlashProgram::CheckFlashErasedWithBlockID(FlashBlockID flashID)
{
	bool status = true;
	uint32_t baseAddress = GetBaseAddressForProgramming(flashID);
	uint32_t numSectors = GetNumberOfSectors(flashID);
	uint32_t dwordSizeCount = (numSectors*_sectorSize)/4;
	int32_t percentComplete = 0;
	SetFlashBlockIDBank(flashID);

	for ( uint32_t count = 0; count < dwordSizeCount; count++, baseAddress += 4 )
	{
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, &flashValue);
		if ( flashValue != 0xFFFFFFFF )
		{
			count = dwordSizeCount;
			status = false;
			continue;
		}
		percentComplete = (count*100)/dwordSizeCount;
		if(!_bQuiet)
		{
			printf("Erase verify: %i%%\r", percentComplete);
			fflush(stdout);
		}
	}
	if(!_bQuiet && status == true)
		printf("Erase verify: 100%%                    \n");

	SetBankSelect(BANK_0);

	return status;
}

void CNTV2KonaFlashProgram::SRecordOutput (const char *pSRecord)
{
	printf ("%s\n", pSRecord);
}

bool CNTV2KonaFlashProgram::CreateSRecord()
{
	uint32_t baseAddress = 0;
	char sRecord[100];
	uint32_t partitionOffset = 0;

	SRecordOutput("S0030000FC");

	for ( uint32_t count = 0; count < _flashSize; count+=32)
	{
		if ((::NTV2DeviceHasSPIv3(_boardID) || ::NTV2DeviceHasSPIv4(_boardID)) && count % _bankSize == 0)
		{
			baseAddress = 0;
			partitionOffset += count;
			switch (partitionOffset)
			{
			default:
			case 0x00000000:
				SetBankSelect(BANK_0);
				break;
			case 0x01000000:
				SetBankSelect(BANK_1);
				break;
			case 0x02000000:
				SetBankSelect(BANK_2);
				break;
			case 0x03000000:
				SetBankSelect(BANK_3);
				break;
			}
		}

		uint32_t recordSize = 32;
		if((_flashSize - count) < recordSize)
			recordSize = _flashSize - count;


		UByte checksum = 0;

		sRecord[0] = 'S';
		sRecord[1] = '3';

		UWord cc = (recordSize + 5);
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
			WriteRegister(kRegXenaxFlashControlStatus,READFAST_COMMAND);
			WaitForFlashNOTBusy();
			uint32_t flashValue;
			ReadRegister(kRegXenaxFlashDOUT,&flashValue);
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
		if (::NTV2DeviceHasSPIv3(_boardID) || ::NTV2DeviceHasSPIv4(_boardID))
		{
			SetBankSelect(bankID);
		}

		uint32_t recordSize = 32;
		if ((_flashSize - count) < recordSize)
			recordSize = _flashSize - count;

		UByte checksum = 0;

		sRecord[0] = 'S';
		sRecord[1] = '3';

		UWord cc = (recordSize + 5);
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
			WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
			WaitForFlashNOTBusy();
			uint32_t flashValue;
			ReadRegister(kRegXenaxFlashDOUT, &flashValue);
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

			SleepMs(100);

			uint32_t flashValue;
			ReadRegister(kRegFS1I2C1Data, &flashValue);

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
	if(!IsKonaIPDevice())
		return false;

	uint32_t baseAddress = _macOffset;

	EraseBlock(MAC_FLASHBLOCK);

	SetFlashBlockIDBank(MAC_FLASHBLOCK);


	uint32_t lo = 0;
	lo |= (mac1->mac[0] << 24) & 0xff000000;
	lo |= (mac1->mac[1] << 16) & 0x00ff0000;
	lo |= (mac1->mac[2] << 8)  & 0x0000ff00;
	lo |=  mac1->mac[3]        & 0x000000ff;

	uint32_t hi = 0;
	hi |= (mac1->mac[4] << 24) & 0xff000000;
	hi |= (mac1->mac[5] << 16) & 0x00ff0000;

	uint32_t lo2 = 0;
	lo2 |= (mac2->mac[0] << 24) & 0xff000000;
	lo2 |= (mac2->mac[1] << 16) & 0x00ff0000;
	lo2 |= (mac2->mac[2] << 8)  & 0x0000ff00;
	lo2 |=  mac2->mac[3]	    & 0x000000ff;

	uint32_t hi2 = 0;
	hi2 |= (mac2->mac[4] << 24) & 0xff000000;
	hi2 |= (mac2->mac[5] << 16) & 0x00ff0000;


	ProgramFlashValue(baseAddress, lo);
	baseAddress += 4;
	ProgramFlashValue(baseAddress, hi);
	baseAddress += 4;
	ProgramFlashValue(baseAddress, lo2);
	baseAddress += 4;
	ProgramFlashValue(baseAddress, hi2);

	WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x9C);
	WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();

	SetBankSelect(BANK_0);

	return true;
}

bool CNTV2KonaFlashProgram::ReadMACAddresses(MacAddr & mac1, MacAddr & mac2)
{
	uint32_t lo;
	uint32_t hi;
	uint32_t lo2;
	uint32_t hi2;

	if(!IsKonaIPDevice())
		return false;

	uint32_t baseAddress = GetBaseAddressForProgramming(MAC_FLASHBLOCK);
	SetFlashBlockIDBank(MAC_FLASHBLOCK);

	WriteRegister(kRegXenaxFlashAddress, baseAddress);
	WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
	WaitForFlashNOTBusy();
	ReadRegister(kRegXenaxFlashDOUT, &lo);
	baseAddress += 4;

	WriteRegister(kRegXenaxFlashAddress, baseAddress);
	WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
	WaitForFlashNOTBusy();
	ReadRegister(kRegXenaxFlashDOUT, &hi);
	baseAddress += 4;

	WriteRegister(kRegXenaxFlashAddress, baseAddress);
	WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
	WaitForFlashNOTBusy();
	ReadRegister(kRegXenaxFlashDOUT, &lo2);
	baseAddress += 4;

	WriteRegister(kRegXenaxFlashAddress, baseAddress);
	WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
	WaitForFlashNOTBusy();
	ReadRegister(kRegXenaxFlashDOUT, &hi2);

	SetBankSelect(BANK_0);


	if (lo == 0xffffffff && hi == 0xffffffff && lo2 == 0xffffffff && hi2 == 0xffffffff)
		return false;

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

	return true;
}

bool
CNTV2KonaFlashProgram::ProgramLicenseInfo(std::string licenseString)
{
	if(!IsKonaIPDevice())
		return false;

	EraseBlock(LICENSE_BLOCK);

	SetFlashBlockIDBank(LICENSE_BLOCK);

	ULWord sectorAddress = GetBaseAddressForProgramming(LICENSE_BLOCK);

    int len =  (int)licenseString.length();
    int words = (len/4) + 2;
    char * data8     = (char*)malloc(words*4);
    ULWord  * data32 = (ULWord*)data8;
    memset(data8,0x0,words*4);
    strcat(data8,licenseString.c_str());

	SetBankSelect(BANK_1);

    for(int i = 0; i < words; i++)
	{
        ProgramFlashValue(sectorAddress, data32[i]);
        sectorAddress += 4;
	}

    free(data8);

	// Protect Device
	WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x1C);
	WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
	WaitForFlashNOTBusy();
	WriteRegister(kRegXenaxFlashDIN, 0x9C);
	WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
	WaitForFlashNOTBusy();
	SetBankSelect(BANK_0);

	return true;
}

bool CNTV2KonaFlashProgram::ReadLicenseInfo(std::string& serialString)
{
#define MAX_SIZE 100

    ULWord license[MAX_SIZE];
    memset (license,0x0,sizeof(license));

	if(!IsKonaIPDevice())
		return false;

	uint32_t baseAddress = GetBaseAddressForProgramming(LICENSE_BLOCK);
	SetFlashBlockIDBank(LICENSE_BLOCK);

    bool terminated = false;
    bool good = false;
    for(int i = 0; i < MAX_SIZE; i++)
	{
		WriteRegister(kRegXenaxFlashAddress,baseAddress);
		WriteRegister(kRegXenaxFlashControlStatus,READFAST_COMMAND);
		WaitForFlashNOTBusy();
        ReadRegister(kRegXenaxFlashDOUT,&license[i]);
        if (license[i] == 0xffffffff)
        {
            good = true; // uninitialized memory
            break;
        }
        if (license[i] == 0)
        {
            good       = true;
            terminated = true;
            break;
        }
        baseAddress += 4;
	}

    std::string res;
    if (terminated)
    {
        res = (char*)license;
    }

    serialString = res;
    return good;
}

bool CNTV2KonaFlashProgram::SetBankSelect( BankSelect bankNumber )
{
	if (::NTV2DeviceHasSPIv3(_boardID) || ::NTV2DeviceHasSPIv4(_boardID))
	{
		WriteRegister(kRegXenaxFlashAddress, (uint32_t)bankNumber);
		WriteRegister(kRegXenaxFlashControlStatus, BANKSELECT_COMMMAND);
		WaitForFlashNOTBusy();
	}
	return true;
}

uint32_t CNTV2KonaFlashProgram::ReadBankSelect()
{
	uint32_t bankNumber = 0;
	if (::NTV2DeviceHasSPIv3(_boardID) || ::NTV2DeviceHasSPIv4(_boardID))
	{
		WriteRegister(kRegXenaxFlashControlStatus, READBANKSELECT_COMMAND);
		WaitForFlashNOTBusy();
		ReadRegister(kRegXenaxFlashDOUT, &bankNumber);
	}
	return bankNumber&0xf;
}

bool CNTV2KonaFlashProgram::SetMCSFile(const char *sMCSFileName)
{
	printf("Parsing MCS File\n");
	return _mcsFile.Open(sMCSFileName);
}

bool CNTV2KonaFlashProgram::ProgramFromMCS(bool verify)
{
	if (!_mcsFile.isReady())
    {
        printf("Bit File not Open\n");
        return false;
    }

	if (IsOpen())
	{
		bool hasErasedSOCs = false;
		uint16_t linearOffsetToBankOffset = 0x0000;
		uint16_t basePartitionAddress = linearOffsetToBankOffset;
		bool bPartitionValid = true;
		uint32_t partitionCount = 0;
		while (bPartitionValid)
		{
            WriteRegister(kVRegFlashState,kProgramStateCalculating);
            WriteRegister(kVRegFlashSize,MCS_STEPS);
            WriteRegister(kVRegFlashStatus,NextMcsStep());

            uint16_t partitionOffset = 0;
			FlashBlockID blockID = MAIN_FLASHBLOCK;
			ParsePartitionFromFileLines(basePartitionAddress, partitionOffset);
			if (basePartitionAddress < 0x0100)
			{
				blockID = MAIN_FLASHBLOCK;
				linearOffsetToBankOffset = 0x0000;
				//Program Main
				printf("Erase Main Bitfile Bank\n");
                WriteRegister(kVRegFlashState, kProgramStateEraseMainFlashBlock);
				EraseBlock(MAIN_FLASHBLOCK);
                WriteRegister(kVRegFlashState, kProgramStateProgramFlash);
			}
			else if (basePartitionAddress >= 0x0100 && basePartitionAddress < 0x0200)
			{
				blockID = MCS_INFO_BLOCK;
				linearOffsetToBankOffset = 0x0100;
				//Program Comment
                printf("Erase Package Info Block\n");
                WriteRegister(kVRegFlashState, kProgramStateErasePackageInfo);
				EraseBlock(MCS_INFO_BLOCK);
                WriteRegister(kVRegFlashState, kProgramStateProgramPackageInfo);
			}
			else if (basePartitionAddress >= 0x0200 && basePartitionAddress < 0x0400)
			{
				if (!hasErasedSOCs)
				{
					printf("Erase SOC Bank 1\n");
                    WriteRegister(kVRegFlashState, kProgramStateEraseBank3);
					EraseBlock(SOC1_FLASHBLOCK);
					printf("Erase SOC Bank 2\n");
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
			uint32_t programOffset = (uint32_t)baseOffset << 16 | partitionOffset;

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
			uint32_t dwordsPerBLock = blockSize / 4;
			uint32_t totalBlockCount = static_cast<uint32_t>((_partitionBuffer.size() + blockSize) / blockSize);
			int32_t percentComplete = 0;

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
				WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
				WaitForFlashNOTBusy();

				for (uint32_t dwordCount = 0; dwordCount < dwordsPerBLock; dwordCount++)
				{
					uint32_t partitionValue = 0xFFFFFFFF;
					if (remainderBytes >= 4)
					{
						partitionValue = (uint32_t)_partitionBuffer[bufferIndex] << 24 |
							(uint32_t)_partitionBuffer[bufferIndex + 1] << 16 |
							(uint32_t)_partitionBuffer[bufferIndex + 2] << 8 |
							(uint32_t)_partitionBuffer[bufferIndex + 3];
						bufferIndex += 4;
						remainderBytes -= 4;
					}
					else
					{
						switch (remainderBytes)
						{
						case 3:
							partitionValue = 0xff;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex] << 24;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex + 1] << 16;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex + 2] << 8;
							break;
						case 2:
							partitionValue = 0xffff;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex] << 24;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex + 1] << 16;
							break;
						case 1:
							partitionValue = 0xffffff;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex] << 24;
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
				WriteRegister(kRegXenaxFlashControlStatus, PAGEPROGRAM_COMMAND);

				WaitForFlashNOTBusy();

				baseAddress += blockSize;

				percentComplete = (blockCount * 100) / totalBlockCount;
				if (!_bQuiet)
				{
					printf("Partition %d program status: %i%%\r", partitionCount, percentComplete);
					fflush(stdout);
				}
			}
			if (!_bQuiet)
				printf("Partition %d program status: 100%%                  \n", partitionCount);

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
                    printf("Verify Error\n");
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
		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x1C);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();

		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x9C);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		SetBankSelect(BANK_0);
	}
	else
    {
        printf("Board Can't be opened\n");
        return false;
    }

    return true;
}

bool CNTV2KonaFlashProgram::ProgramSOC(bool verify )
{
	if (!_mcsFile.isReady())
    {
        printf("Bit File not Open\n");
        return false;
    }

	if (IsOpen())
	{
		printf("Erase SOC Bank 1\n");
 		EraseBlock(SOC1_FLASHBLOCK);
		printf("Erase SOC Bank 2\n");
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
			uint32_t programOffset = (uint32_t)baseOffset << 16 | partitionOffset;

			FlashBlockID blockID = SOC1_FLASHBLOCK;
			if (programOffset >= 0x01000000)
				blockID	 = SOC2_FLASHBLOCK;

			if (_bankSize == 0)
            {
                return true;
            }

			SetFlashBlockIDBank(blockID);
			uint32_t baseAddress = GetBaseAddressForProgramming(blockID) + programOffset;
			uint32_t bufferIndex = 0;
			uint32_t blockSize = 512;
			uint32_t dwordsPerBLock = blockSize / 4;
			uint32_t totalBlockCount = static_cast<uint32_t>((_partitionBuffer.size() + blockSize) / blockSize);
			int32_t percentComplete = 0;
			for (uint32_t blockCount = 0; blockCount < totalBlockCount; blockCount++)
			{
				if (baseAddress == 0x01000000 && blockCount > 0)
				{
					SetFlashBlockIDBank(SOC2_FLASHBLOCK);
					baseAddress = GetBaseAddressForProgramming(blockID);
				}
				uint32_t remainderBytes = static_cast<uint32_t>(_partitionBuffer.size() - bufferIndex);
				WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
				WaitForFlashNOTBusy();

				for (uint32_t dwordCount = 0; dwordCount < dwordsPerBLock; dwordCount++)
				{
					uint32_t partitionValue = 0xFFFFFFFF;
					if (remainderBytes >= 4)
					{
						partitionValue = (uint32_t)_partitionBuffer[bufferIndex] << 24 |
							(uint32_t)_partitionBuffer[bufferIndex + 1] << 16 |
							(uint32_t)_partitionBuffer[bufferIndex + 2] << 8 |
							(uint32_t)_partitionBuffer[bufferIndex + 3];
						bufferIndex += 4;
						remainderBytes -= 4;
					}
					else
					{
						switch (remainderBytes)
						{
						case 3:
							partitionValue = 0xff;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex] << 24;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex + 1] << 16;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex + 2] << 8;
							break;
						case 2:
							partitionValue = 0xffff;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex] << 24;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex + 1] << 16;
							break;
						case 1:
							partitionValue = 0xffffff;
							partitionValue |= (uint32_t)_partitionBuffer[bufferIndex] << 24;
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
				WriteRegister(kRegXenaxFlashControlStatus, PAGEPROGRAM_COMMAND);

				WaitForFlashNOTBusy();

				baseAddress += blockSize;

				percentComplete = (blockCount * 100) / totalBlockCount;
				if (!_bQuiet)
				{
					printf("Partition %d program status: %i%%\r", partitionCount+2, percentComplete);
					fflush(stdout);
				}
			}
			if (!_bQuiet)
				printf("Partition %d program status: 100%%                  \n", partitionCount+2);

            if (verify)
			{ 
                if (!VerifySOCPartition(blockID, programOffset))
				{
					SetBankSelect(BANK_0);
                    printf("Verify failed\n");
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
		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x1C);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		
		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x9C);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		SetBankSelect(BANK_0);
 	}
	else
    {
        printf("Board Can't be opened\n");
        return false;
    }

    return true;
}

static int64_t getFileSize( const char *fileName ) {
#if defined (AJALinux) || defined (AJAMac)
	struct stat st;
	if ( 0 != stat(fileName, &st))
		return -1;

	return (int64_t)st.st_size;
#else
	struct _stat st;
	if (0 != _stat(fileName, &st))
		return -1;

	return (int64_t)st.st_size;
#endif
}

void CNTV2KonaFlashProgram::ProgramCustom ( const char *sCustomFileName, const uint32_t addr)
{
	static const int32_t MAX_CUSTOM_FILE_SIZE = (8<<20); // 1M
	if (NULL == _customFileBuffer )
		_customFileBuffer = new unsigned char[MAX_CUSTOM_FILE_SIZE];
	size_t customSize = 0;

	uint32_t bank   =  addr / _bankSize;
	uint32_t offset =  addr % _bankSize;
	if (offset + customSize > _bankSize) {
		throw "Custom write spans banks - not supported";
	}
	if (offset % _sectorSize) {
		throw "Write not on sector boundary - not supported";
	}

	{
		int64_t sz = getFileSize(sCustomFileName);
		if (sz < 0) {
			throw "Error getting file size";
		}
	   	if (sz > MAX_CUSTOM_FILE_SIZE) {
			throw "File size greater than max supported size (8M)";
		}

		FILE *fp = fopen(sCustomFileName, "rb");
		if (fp == NULL) {
			throw "Unable to open file";
		}

		customSize = fread(_customFileBuffer, 1, MAX_CUSTOM_FILE_SIZE, fp);
		if (customSize == 0) {
			fclose(fp);
			throw "Couldn't read any data from custom file";
		}
		fclose(fp);
	}
	if (IsOpen())
	{
		static const BankSelect BankIdxToBankSelect[] = {
			BANK_0,
			BANK_1,
			BANK_2,
			BANK_3
		};

		SetBankSelect(BankIdxToBankSelect[bank]);

		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x0);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();

		int32_t customSectors = static_cast<int32_t>((customSize + _sectorSize - 1) / (_sectorSize));
		for (int32_t i=0; i<customSectors; i++) {
			printf("Erasing sectors - %3d of %3d\r", i, customSectors);
			fflush(stdout);
			EraseSector( offset + (i * _sectorSize));
		}

		{
			WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
			WaitForFlashNOTBusy();

			uint32_t blockSize = 512;
			uint32_t dwordsPerBLock = blockSize / 4;
			uint32_t totalBlockCount = static_cast<uint32_t>((customSize + blockSize - 1) / blockSize);
			int32_t percentComplete = 0;
			uint32_t baseAddress = offset;
			int32_t remainderBytes = static_cast<int32_t>(customSize);
			int32_t bufferIndex = 0;
			for (uint32_t blockCount = 0; blockCount < totalBlockCount; blockCount++)
			{
				WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
				WaitForFlashNOTBusy();

				for (uint32_t dwordCount = 0; dwordCount < dwordsPerBLock; dwordCount++)
				{
					uint32_t partitionValue = 0xFFFFFFFF;
					if (remainderBytes >= 4)
					{
						partitionValue = (uint32_t)_customFileBuffer[bufferIndex] << 24 |
							(uint32_t)_customFileBuffer[bufferIndex + 1] << 16 |
							(uint32_t)_customFileBuffer[bufferIndex + 2] << 8 |
							(uint32_t)_customFileBuffer[bufferIndex + 3];
						bufferIndex += 4;
						remainderBytes -= 4;
					}
					else
					{
						switch (remainderBytes)
						{
						case 3:
							partitionValue = 0xff;
							partitionValue |= (uint32_t)_customFileBuffer[bufferIndex] << 24;
							partitionValue |= (uint32_t)_customFileBuffer[bufferIndex + 1] << 16;
							partitionValue |= (uint32_t)_customFileBuffer[bufferIndex + 2] << 8;
							break;
						case 2:
							partitionValue = 0xffff;
							partitionValue |= (uint32_t)_customFileBuffer[bufferIndex] << 24;
							partitionValue |= (uint32_t)_customFileBuffer[bufferIndex + 1] << 16;
							break;
						case 1:
							partitionValue = 0xffffff;
							partitionValue |= (uint32_t)_customFileBuffer[bufferIndex] << 24;
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
				WriteRegister(kRegXenaxFlashControlStatus, PAGEPROGRAM_COMMAND);

				WaitForFlashNOTBusy();

				baseAddress += blockSize;

				percentComplete = (blockCount * 100) / totalBlockCount;
				if (!_bQuiet)
				{
					printf("Program status: %i%% (%4d of %4d blocks)\r", percentComplete, blockCount, totalBlockCount);
					fflush(stdout);
				}
			}
		}

		//Protect Device
		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x1C);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		
		WriteRegister(kRegXenaxFlashControlStatus, WRITEENABLE_COMMAND);
		WaitForFlashNOTBusy();
		WriteRegister(kRegXenaxFlashDIN, 0x9C);
		WriteRegister(kRegXenaxFlashControlStatus, WRITESTATUS_COMMAND);
		WaitForFlashNOTBusy();
		SetBankSelect(BANK_0);
 	}
	else
		throw "Board Can't be opened";
}

bool CNTV2KonaFlashProgram::SetFlashBlockIDBank(FlashBlockID blockID)
{
	switch (blockID)
	{
	case MAIN_FLASHBLOCK:
		return SetBankSelect(BANK_0);
	case FAILSAFE_FLASHBLOCK:
		return SetBankSelect(BANK_1);
	case MCS_INFO_BLOCK:
	case MAC_FLASHBLOCK:
	case LICENSE_BLOCK:
		return SetBankSelect(BANK_1);
	case SOC1_FLASHBLOCK:
		return SetBankSelect(BANK_2);
	case SOC2_FLASHBLOCK:
		return SetBankSelect(BANK_3);
	default:
		return false;
	}
}

void CNTV2KonaFlashProgram::ParsePartitionFromFileLines(uint32_t address, uint16_t & partitionOffset)
{
	_partitionBuffer.clear();
	_partitionBuffer.resize(0);
	bool getnext = false;
	if (address != 0x0000 && address != 0x0200)
		getnext = true;
	_mcsFile.GetPartition(_partitionBuffer, address, partitionOffset, getnext);
	_bankSize = static_cast<uint32_t>(_partitionBuffer.size());
	return;
}

bool CNTV2KonaFlashProgram::VerifySOCPartition(FlashBlockID flashID, uint32_t flashBlockOffset)
{
    SetFlashBlockIDBank(flashID);

	uint32_t errorCount = 0;
    uint32_t baseAddress = flashBlockOffset;

	uint32_t dwordsPerPartition = _bankSize / 4;
	int32_t percentComplete = 0;
	uint32_t bufferIndex = 0;
    WriteRegister(kVRegFlashSize,dwordsPerPartition);
	for (uint32_t dwordCount = 0; dwordCount < dwordsPerPartition; dwordCount += 100)//dwordCount++)
	{
        WriteRegister(kVRegFlashStatus,dwordCount);
		WriteRegister(kRegXenaxFlashAddress, baseAddress);
		WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, &flashValue);
		uint32_t partitionValue = (uint32_t)_partitionBuffer[bufferIndex] << 24 |
			(uint32_t)_partitionBuffer[bufferIndex + 1] << 16 |
			(uint32_t)_partitionBuffer[bufferIndex + 2] << 8 |
			(uint32_t)_partitionBuffer[bufferIndex + 3];
		partitionValue = NTV2EndianSwap32(partitionValue);
		bufferIndex += 400;//4;
		baseAddress += 400;//4;

		if (flashValue != partitionValue)
		{
			printf("Error %d E(%08X),R(%08X)\n", dwordCount, partitionValue, flashValue);
			errorCount++;
 			if (errorCount > 1)
 				break;
		}

		percentComplete = (dwordCount * 100) / dwordsPerPartition;
		if (!_bQuiet)
		{
			printf("Program verify: %i%%\r", percentComplete);
			fflush(stdout);
		}
	}

	if (errorCount)
	{
		if (!_bQuiet)
			printf("Program verify failed: %i%%\n", percentComplete);
		return false;
	}
	else
	{
		if (!_bQuiet)
			printf("Program verify: 100%%                    \n");
	}

	return true;
}

void CNTV2KonaFlashProgram::DisplayData(uint32_t address, uint32_t count)
{
#define WORDS_PER_LINE 4

	uint32_t bank   =  address / _bankSize;
	uint32_t offset =  address % _bankSize;
	
	SetBankSelect((BankSelect)bank);

	char line[1024];
	memset(line, 0, 1024);
	char * pLine = &line[0];
	pLine += sprintf(pLine, "%08x: ", (uint32_t)((bank * _bankSize) + offset));
	
	int32_t lineCount = 0; 
	for (uint32_t i = 0; i < count; i++, offset += 4)
	{
		WriteRegister(kRegXenaxFlashAddress, offset);
		WriteRegister(kRegXenaxFlashControlStatus, READFAST_COMMAND);
		WaitForFlashNOTBusy();
		uint32_t flashValue;
		ReadRegister(kRegXenaxFlashDOUT, &flashValue);
		flashValue = NTV2EndianSwap32(flashValue);
		pLine += sprintf(pLine, "%08x  ", (uint32_t)flashValue);
		if (++lineCount == WORDS_PER_LINE)
		{
			printf("%s\n", line);
			memset(line, 0, 1024);
			pLine = &line[0];
			pLine += sprintf(pLine, "%08x: ", (uint32_t)((bank * _bankSize) + offset + 4));
			lineCount = 0;
		}
	}
	if (lineCount != 0)
	{
		printf("%s\n", line);
	}
}


#ifdef MSWindows
#pragma warning(default: 4800)
#endif
