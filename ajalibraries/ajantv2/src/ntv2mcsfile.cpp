/**
	@file		ntv2mcsfile.cpp
	@brief		Implementation of CNTV2MCSfile class.
	@copyright	(C) 2010-2020 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/
#include "ntv2mcsfile.h"
#include "ntv2bitfile.h"
#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#include <algorithm>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
	#include "string.h"
#endif
#include "ntv2utils.h"
#include <string.h>
#include <fcntl.h>
#if !defined (AJAMac) && !defined (AJALinux)
	#include <io.h>
#endif
#include <time.h>
using namespace std;



CNTV2MCSfile::CNTV2MCSfile()
{
	Init();	//	Initialize everything
}

CNTV2MCSfile::~CNTV2MCSfile()
{
	Close();
}


void CNTV2MCSfile::Close(void)
{
	if (mMCSFileStream.is_open())
		mMCSFileStream.close();
}	


void CNTV2MCSfile::Init(void)
{
	Close();
}

bool CNTV2MCSfile::isReady()
{
	return true;
//	if (mFileLines.size() > 0)
//		return true;
//	else
//		return false;
}

bool CNTV2MCSfile::Open(const string & inMCSFileName)
{
	Close();
	mCommentString.clear();
	struct stat	fsinfo;
	::stat(inMCSFileName.c_str(), &fsinfo);
	mFileSize = uint32_t(fsinfo.st_size);
	
	struct tm * fileTimeInfo;
	fileTimeInfo = localtime(&fsinfo.st_ctime);

	time_t rawGenerationTime;
	struct tm * generationTimeInfo;
	time(&rawGenerationTime);
	generationTimeInfo = localtime(&rawGenerationTime);
	char buff[1000];
	sprintf(buff, "Generation Time: %s  Original MCS Time: %s\n", asctime(generationTimeInfo), asctime(fileTimeInfo));
	mCommentString = buff;
	mMCSFileStream.open(inMCSFileName.c_str(), std::ios::in);

	if (mMCSFileStream.fail())
		return false;

	GetFileByteStream();
	GetMCSInfo();

	if (mMCSFileStream.is_open())
		mMCSFileStream.close();

	return true;

}	//	Open

bool CNTV2MCSfile::GetMCSHeaderInfo(const std::string & inMCSFileName)
{
	Close();
	mMCSFileStream.open(inMCSFileName.c_str(), std::ios::in);

	if (mMCSFileStream.fail())
		return false;

	GetFileByteStream(50);
	GetMCSInfo();

	Close();
	return true;
}

void CNTV2MCSfile::GetMCSInfo()
{
	uint16_t mainPartitionAddress = 0x0000, mainPartitionOffset = 0x0000;
	std::vector<uint8_t> mainBitfilePartition;
	GetPartition(mainBitfilePartition, mainPartitionAddress, mainPartitionOffset, false);
	if (mainBitfilePartition.size() > 0)
	{
		CNTV2Bitfile bitfileInfo;
		bitfileInfo.ParseHeaderFromBuffer(&mainBitfilePartition[0]);
		mBitfileDate = bitfileInfo.GetDate();
		mBitfileDesignName = bitfileInfo.GetDesignName();
		mBitfilePartName = bitfileInfo.GetPartName();
		mBitfileTime = bitfileInfo.GetTime();
	}
	
	mMCSInfoString = mFileLines[0];
}

std::string CNTV2MCSfile::GetBitfileDateString()
{
	return mBitfileDate;
}

std::string CNTV2MCSfile::GetBitfileDesignString()
{
	return mBitfileDesignName;
}

std::string CNTV2MCSfile::GetBitfilePartNameString()
{
	return mBitfilePartName;
}

std::string CNTV2MCSfile::GetBitfileTimeString()
{
	return mBitfileTime;
}

std::string CNTV2MCSfile::GetMCSPackageVersionString()
{
	return mMCSInfoString.substr(mMCSInfoString.find("PACKAGE_NUMBER"), mMCSInfoString.find("DATE") - mMCSInfoString.find("PACKAGE_NUMBER") - 1);
}

std::string CNTV2MCSfile::GetMCSPackageDateString()
{
	return mMCSInfoString.substr(mMCSInfoString.find("DATE") + 5, mMCSInfoString.npos - mMCSInfoString.find("DATE") + 5);
}

bool CNTV2MCSfile::InsertBitFile(const string & inBitFileName, const string & inMCSFileName, const string & inUserMessage)
{
	CNTV2Bitfile	bitfile;
	CNTV2MCSfile	mcsFile;
	char iRecord[100];
	uint64_t recordSize = 0;
	UWord baseAddress = 0x0000;
	UWord ExtendedBaseAddress = 0x0000;
	UByte checksum = 0;
	UByte recordType = 0x00;

	if (!Open(inMCSFileName))
	{
		cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  mcsFile '" << inMCSFileName << "' not found" << endl;
		return false;
	}

	//Read in the bitfile
	if (!bitfile.Open(inBitFileName))
	{
		cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Bitfile '" << inBitFileName << "' not found" << endl;
		return false;
	}

	uint32_t	bitfileLength(bitfile.GetFileStreamLength());
	unsigned char *	bitfileBuffer(new unsigned char[bitfileLength + 512]);
	if (bitfileBuffer == NULL)
	{
		cerr << "## ERROR:  CNTV2FirmwareInstallerThread:  Unable to allocate bitfile buffer" << endl;
		return false;
	}

	::memset(bitfileBuffer, 0xFF, bitfileLength + 512);
	const unsigned	readBytes(bitfile.GetFileByteStream(bitfileBuffer, bitfileLength));
	const string	designName(bitfile.GetDesignName());
	if (readBytes != bitfileLength)
	{
		delete [] bitfileBuffer;
		cerr << "## ERROR:  InsertBitFile:  Invalid bitfile length, read " << readBytes << " bytes, expected " << bitfileLength << endl;
		return false;
	}

	//First we will write out the bitfile then add the date and comment and then the mcs file
	uint64_t bytesLeftToWrite = bitfileLength;
	while(bytesLeftToWrite > 0)
	{
		recordSize = bytesLeftToWrite > 16 ? 16 : bytesLeftToWrite;
		int i = 0;
		int index = 0;
		checksum = 0;

		if (baseAddress == 0x0000)
		{
			//Insert ELAR
			std::string ELARString(":02000004000000");
			sprintf(&ELARString[9], "%04X", ExtendedBaseAddress);
			for (i = 1; i < 13; i++)
			{
				checksum += ELARString[i] - 0x30;
			}
			checksum = (~checksum) + 1;
			std::vector<std::string>::iterator fileItr;
			sprintf(&ELARString[13], "%02X", checksum);
			IRecordOutput(ELARString.c_str());
			ExtendedBaseAddress++;
			checksum = 0;
		}

		iRecord[0] = ':';

		sprintf(&iRecord[1], "%02X", UByte(recordSize));
		checksum += (UByte)recordSize;

		UWord addr = baseAddress;
		UByte aa = ((addr >> 8) & 0xff);
		sprintf(&iRecord[3], "%02X", aa);
		checksum += aa;

		aa = ((addr)& 0xff);
		sprintf(&iRecord[5], "%02X", aa);
		checksum += aa;

		sprintf(&iRecord[7], "%02X", recordType);

		index = 9;

		while (i < (int)recordSize)
		{
			unsigned char dd = *bitfileBuffer;
			sprintf(&iRecord[index], "%02X", dd);
			checksum += dd;
			i++;
			index += 2;
			bitfileBuffer++;
			bytesLeftToWrite--;
		}

		baseAddress += 0x0010;
		checksum = (~checksum) + 1;
		sprintf(&iRecord[index], "%02X", checksum);

		IRecordOutput(iRecord);
	}

	//Insert the date at the 3rd to last partition in Bank 2
	if (!inUserMessage.empty())
		mCommentString.append(inUserMessage);
	uint32_t commentSize = static_cast<uint32_t>(mCommentString.length());
	bytesLeftToWrite = commentSize;
	//32M - 3*256*1024
	ExtendedBaseAddress = 0x01f4;
	baseAddress = 0x0000;
	uint32_t commentIndex = 0;
	while (bytesLeftToWrite > 0)
	{
		recordSize = bytesLeftToWrite > 16 ? 16 : bytesLeftToWrite;
		int i = 0;
		int index = 0;
		checksum = 0;

		if (ExtendedBaseAddress == 0x01f4)
		{
			//Insert ELAR
			std::string ELARString(":02000004000000");
			sprintf(&ELARString[9], "%04X", ExtendedBaseAddress);
			for (i = 1; i < 13; i++)
			{
				checksum += ELARString[i] - 0x30;
			}
			checksum = (~checksum) + 1;
			std::vector<std::string>::iterator fileItr;
			sprintf(&ELARString[13], "%02X", checksum);
			IRecordOutput(ELARString.c_str());
			ExtendedBaseAddress++;
			checksum = 0;
		}

		iRecord[0] = ':';

		sprintf(&iRecord[1], "%02X", UByte(recordSize));
		checksum += (UByte)recordSize;

		UWord addr = baseAddress;
		UByte aa = ((addr >> 8) & 0xff);
		sprintf(&iRecord[3], "%02X", aa);
		checksum += aa;

		aa = ((addr)& 0xff);
		sprintf(&iRecord[5], "%02X", aa);
		checksum += aa;

		sprintf(&iRecord[7], "%02X", recordType);

		index = 9;
		while (i < (int)recordSize)
		{
			unsigned char dd = mCommentString.at(commentIndex);
			sprintf(&iRecord[index], "%02X", dd);
			checksum += dd;
			i++;
			index += 2;
			bytesLeftToWrite--;
			commentIndex++;
		}

		baseAddress += 0x0010;
		checksum = (~checksum) + 1;
		sprintf(&iRecord[index], "%02X", checksum);

		IRecordOutput(iRecord);
	}


	//Finished with bitfile now just read a line and output a line from the mcs file
	//32M offset is assumed to be the start of SOC stuff
	if (!FindExtendedLinearAddressRecord(0x0200))
		return false;
	mCurrentLocation = mBaseELARLocation;
	while (mCurrentLocation != mFileLines.end())
	{
		IRecordOutput(mCurrentLocation->c_str());
		mCurrentLocation++;
	}
	if (bitfileBuffer)
		delete [] bitfileBuffer;
	return true;
}

void CNTV2MCSfile::IRecordOutput(const char *pIRecord)
{
	//_setmode(_fileno(stdout), _O_U8TEXT);
	printf("%s\n", pIRecord);
}

// Get maximum of fileLength bytes worth of configuration stream data in buffer.
// Return value:
//   number of bytes of data copied into buffer. This can be <= bufferLength
//   zero means configuration stream has finished.
//   Return value < 0 indicates error
uint32_t CNTV2MCSfile::GetFileByteStream(uint32_t numberOfLines)
{
    const uint32_t maxNumLines = 2000000;

	std::string line;

	if (!mMCSFileStream.is_open())
		return 0;

	mMCSFileStream.seekg(0, std::ios::beg);
	if (numberOfLines == 0)
	{
        mFileLines.resize(maxNumLines);
        numberOfLines = maxNumLines;
	}
	else
	{
		mFileLines.resize(numberOfLines+1);
	}

	uint32_t index = 0;
	mMCSFileStream.sync_with_stdio(false);
	while (getline(mMCSFileStream, line) && index < numberOfLines)
	{
		mFileLines[index] = line;
		index++;
	}
    if (numberOfLines < maxNumLines)
		mFileLines[index] = ":00000001FF";
	return mFileSize;

}	//	GetFileByteStream

bool CNTV2MCSfile::FindExtendedLinearAddressRecord(uint16_t address /*= 0x0000*/)
{
	std::string ELARString(":02000004000000");
	sprintf(&ELARString[9], "%04X", address);
	uint8_t checksum = 0;
	for (int i = 1; i < 13; i++)
	{
		checksum += ELARString[i] - 0x30;
	}
 	checksum = (~checksum) + 1;
	std::vector<std::string>::iterator fileItr;
	sprintf(&ELARString[13], "%02X", checksum);

    // Do a search for a match, don't search on the checksum
    std::string needle(ELARString, 0, 13);
    std::vector<std::string>::iterator it = mFileLines.begin();
    mBaseELARLocation = mFileLines.end();
    while(it != mFileLines.end())
    {
        std::string hay(*it, 0, 13);
        if (needle == hay)
        {
            mBaseELARLocation = it;
            break;
        }
        ++it;
    }

	if (mBaseELARLocation != mFileLines.end())
		return true;
	else
		return false;
}

bool CNTV2MCSfile::GetCurrentParsedRecord(IntelRecordInfo &recordInfo)
{
	IntelRecordInfo currentRecordInfo;
	bool status = ParseCurrentRecord(currentRecordInfo);
	if (status)
	{
		recordInfo.byteCount = currentRecordInfo.byteCount;
		recordInfo.recordType = currentRecordInfo.recordType;
		recordInfo.linearAddress = currentRecordInfo.linearAddress;
		memcpy(recordInfo.dataBuffer, currentRecordInfo.dataBuffer, 16);
		recordInfo.checkSum = currentRecordInfo.checkSum;
	}
	return status;
}

bool CNTV2MCSfile::ParseCurrentRecord(IntelRecordInfo &recordInfo)
{
	if (mCurrentLocation->size() == 0)
	{
		recordInfo.recordType = IRT_UNKNOWN;
		return false;
	}

	if (!mCurrentLocation[0].compare(":"))
	{
		recordInfo.recordType = IRT_UNKNOWN;
		return false;
	}

	uint32_t rType = 0;
	uint16_t byteCount16 = 0;
	sscanf(mCurrentLocation[0].c_str(), ":%02hX%04hX%02X", &byteCount16, &recordInfo.linearAddress, &rType);
	recordInfo.byteCount = (uint8_t)byteCount16;
	recordInfo.segmentAddress = 0; //Fix this for the correct base address
	switch (rType)
	{
	case 0x00:
		recordInfo.recordType = IRT_DR;
		break;
	case 0x01:
		recordInfo.recordType = IRT_EOFR;
		break;
	case 0x02:
		recordInfo.recordType = IRT_ESAR;
		break;
	case 0x04:
		recordInfo.recordType = IRT_ELAR;
		sscanf(mCurrentLocation[0].c_str(), ":%02hX%04hX%02X%04hX", &byteCount16, &recordInfo.linearAddress, &rType, &recordInfo.linearAddress);
		recordInfo.byteCount = (uint8_t)byteCount16;
		break;
	default:
		recordInfo.recordType = IRT_UNKNOWN;
		break;
	}

	return true;
}

uint32_t CNTV2MCSfile::GetPartition(std::vector<uint8_t> & partitionBuffer, uint16_t baseELARaddress, uint16_t & partitionOffset, bool nextPartition)
{
	if (!isReady())
		return 0;
	IntelRecordInfo recordInfo;

	if (!nextPartition)
	{
		//32M offset is assumed to be the start of SOC stuff
		if (!FindExtendedLinearAddressRecord(baseELARaddress))
			return 0;
		mCurrentLocation = mBaseELARLocation;
	}
	else
	{
		ParseCurrentRecord(recordInfo);
		baseELARaddress = recordInfo.linearAddress;
		mBaseELARLocation = mCurrentLocation;
	}

	uint16_t lastELARAddress = baseELARaddress;
	mCurrentLocation++;
	ParseCurrentRecord(recordInfo);
	if (recordInfo.recordType == IRT_DR)
		partitionOffset = recordInfo.linearAddress;
	while (recordInfo.recordType == IRT_DR)
	{
		std::string temp(mCurrentLocation[0].c_str());
		for (int i = 0; i < recordInfo.byteCount*2; i+=2)
		{
			uint32_t c = 0;
			sscanf(&mCurrentLocation[0].c_str()[9 + i], "%02X", &c);
			partitionBuffer.push_back((uint8_t)(c&0x000000FF));
		}
		mCurrentLocation++;
		ParseCurrentRecord(recordInfo);
		if (recordInfo.recordType == IRT_DR)
			continue;
		else
		{
			//We need to check if this is another ELAR with the same partition
			if (recordInfo.recordType == IRT_ELAR)
			{
				//if this is part of last packet
				lastELARAddress++;
				if (recordInfo.linearAddress == lastELARAddress)
				{
					mCurrentLocation++;
					ParseCurrentRecord(recordInfo);
					continue;
				}
				else
				{
					//We are at the next partition
					return static_cast<uint32_t>(partitionBuffer.size());
				}
			}
			else
				return static_cast<uint32_t>(partitionBuffer.size());
		}
	}

	return static_cast<uint32_t>(partitionBuffer.size());
}


