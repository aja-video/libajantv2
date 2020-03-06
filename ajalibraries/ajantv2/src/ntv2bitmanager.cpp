/**
	@file		ntv2bitmanager.cpp
	@brief		Implementation of CNTV2BitManager class.
	@copyright	(C) 2019 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
**/
#include "ntv2bitmanager.h"
#include "ntv2bitfile.h"
//#include "ntv2card.h"
//#include "ntv2utils.h"
#include "ajabase/system/file_io.h"
#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
#endif
#include <map>

using namespace std;


CNTV2BitManager::CNTV2BitManager()
{
}

CNTV2BitManager::~CNTV2BitManager()
{
	Clear();
}

bool CNTV2BitManager::AddFile (const std::string & inBitfilePath)
{
	AJAFileIO Fio;
	CNTV2Bitfile Bitfile;
	NTV2BitfileInfo Info;

	// open bitfile
    if (!Fio.FileExists(inBitfilePath))
        return false;
    if (!Bitfile.Open(inBitfilePath))
        return false;

	// get bitfile information
	Info.bitfilePath = inBitfilePath;
	Info.designName = Bitfile.GetDesignName();
	Info.designID = Bitfile.GetDesignID();
	Info.designVersion = Bitfile.GetDesignVersion();
	Info.bitfileID = Bitfile.GetBitfileID();
	Info.bitfileVersion = Bitfile.GetBitfileVersion();
    if (Bitfile.IsTandem())
        Info.bitfileFlags = NTV2_BITFILE_FLAG_TANDEM;
    else if (Bitfile.IsClear())
        Info.bitfileFlags = NTV2_BITFILE_FLAG_CLEAR;
    else if (Bitfile.IsPartial())
        Info.bitfileFlags = NTV2_BITFILE_FLAG_PARTIAL;
    else
        Info.bitfileFlags = 0;
	Info.deviceID = Bitfile.GetDeviceID();

	// check for reconfigurable bitfile
    if ((Info.designID == 0) || (Info.designID > 0xfe))
        return false;
    if (Info.designVersion > 0xfe)
        return false;
    if ((Info.bitfileID == 0) || (Info.bitfileID > 0xfe))
        return false;
    if (Info.bitfileVersion > 0xfe)
        return false;
    if (Info.bitfileFlags == 0)
        return false;
    if (Info.deviceID == 0)
        return false;

	// add to list
	_bitfileList.push_back(Info);

	return true;
}

bool CNTV2BitManager::AddDirectory (const std::string & inDirectory)
{
	AJAFileIO Fio;
	std::vector<std::string> fileContainer;
	std::vector<std::string>::iterator fcIter;

	// check for good directory
    if (Fio.DoesDirectoryExist(inDirectory) != AJA_STATUS_SUCCESS)
        return false;

	// get bitfiles
	Fio.ReadDirectory(inDirectory, "*.bit", fileContainer);

	// add bitfiles
	for (fcIter = fileContainer.begin(); fcIter != fileContainer.end(); fcIter++)
	{
        AddFile(*fcIter);
	}
	
	return true;
}

void CNTV2BitManager::Clear (void)
{
	_bitfileList.clear();
	_bitstreamList.clear();
}

size_t CNTV2BitManager::GetNumBitfiles (void)
{
	return _bitfileList.size();
}

NTV2BitfileInfoList & CNTV2BitManager::GetBitfileInfoList (void)
{
	return _bitfileList;
}

bool CNTV2BitManager::GetBitStream (NTV2_POINTER & bitstream,
									ULWord designID,
									ULWord designVersion,
									ULWord bitfileID,
									ULWord bitfileVersion,
									ULWord bitfileFlags)
{
	int size = (int)GetNumBitfiles();
	int max = size;
	int i;

	for (i = 0; i < size; i++)
	{
		// search for bitstream
        NTV2BitfileInfo info = _bitfileList[i];
        if ((designID == info.designID) &&
            (designVersion == info.designVersion) &&
            (bitfileID == info.bitfileID) &&
            ((bitfileFlags & info.bitfileFlags) != 0))
		{
            if (bitfileVersion == info.bitfileVersion)
				break;
            if ((max >= size) || (info.bitfileVersion > _bitfileList[max].bitfileVersion))
				max = i;
		}
	}

	// looking for latest version?
	if ((bitfileVersion == 0xff) && (max < size))
		i = max;

	// find something?
	if (i == size)
		return false;
	
	// read in bitstream
	if (!ReadBitstream(i))
		return false;

	bitstream = _bitstreamList[i];
	return true;
}

bool CNTV2BitManager::ReadBitstream(int index)
{
	CNTV2Bitfile Bitfile;
    NTV2_POINTER Bitstream;
	unsigned char* address;
	unsigned size;

	// already in cache
    if ((index < (int)_bitstreamList.size()) && (_bitstreamList[index].GetByteCount() > 0))
        return true;

	// open bitfile to get bitstream
    if (!Bitfile.Open(_bitfileList[index].bitfilePath))
        return false;

	// allocate bitstream buffer
	size = Bitfile.GetProgramStreamLength();
    if (size == 0)
        return false;
    Bitstream.Allocate (size);

	// read bitstream from bitfile
    address = (unsigned char*)Bitstream.GetHostAddress(0);
    if (address == NULL)
        return false;
    if (!Bitfile.GetProgramByteStream (address, size))
        return false;

    if (index >= (int)_bitstreamList.size())
        _bitstreamList.resize(index + 1);

    _bitstreamList[index] = Bitstream;

	return true;
}



