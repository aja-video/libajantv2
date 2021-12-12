/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2bitfilemanager.cpp
	@brief		Implementation of CNTV2BitfileManager class.
	@copyright	(C) 2019-2021 AJA Video Systems, Inc.	 All rights reserved.
**/
#include "ntv2bitfilemanager.h"
#include "ntv2bitfile.h"
#include "ntv2utils.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/file_io.h"
#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
#endif
#include <map>

using namespace std;

#define BFMFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)
#define BFMWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)
#define BFMNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)
#define BFMINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)
#define BFMDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)


CNTV2BitfileManager::CNTV2BitfileManager()
{
}

CNTV2BitfileManager::~CNTV2BitfileManager()
{
	Clear();
}

bool CNTV2BitfileManager::AddFile (const string & inBitfilePath)
{
	AJAFileIO Fio;
	CNTV2Bitfile Bitfile;
	NTV2BitfileInfo Info;

	//	Open bitfile...
	if (!Fio.FileExists(inBitfilePath))
		{BFMFAIL("Bitfile path '" << inBitfilePath << "' not found");  return false;}
	if (!Bitfile.Open(inBitfilePath))
		{BFMFAIL("Bitfile '" << inBitfilePath << "' failed to open");  return false;}

	// get bitfile information
	Info.bitfilePath	= inBitfilePath;
	Info.designName		= Bitfile.GetDesignName();
	Info.designID		= Bitfile.GetDesignID();
	Info.designVersion	= Bitfile.GetDesignVersion();
	Info.bitfileID		= Bitfile.GetBitfileID();
	Info.bitfileVersion = Bitfile.GetBitfileVersion();
	if (Bitfile.IsTandem())
		Info.bitfileFlags = NTV2_BITFILE_FLAG_TANDEM;
	else if (Bitfile.IsClear())
		Info.bitfileFlags = NTV2_BITFILE_FLAG_CLEAR;
	else if (Bitfile.IsPartial())
		Info.bitfileFlags = NTV2_BITFILE_FLAG_PARTIAL;
	else
		Info.bitfileFlags = 0;
	Info.deviceID		= Bitfile.GetDeviceID();

	//	Check for reconfigurable bitfile...
	if ((Info.designID == 0) || (Info.designID > 0xfe))
		{BFMFAIL("Invalid design ID " << xHEX0N(Info.designID,8) << " for bitfile '" << inBitfilePath << "'");  return false;}
	if (Info.designVersion > 0xfe)
		{BFMFAIL("Invalid design version " << xHEX0N(Info.designVersion,8) << " for bitfile '" << inBitfilePath << "'");  return false;}
	if ((Info.bitfileID > 0xfe))
		{BFMFAIL("Invalid bitfile ID " << xHEX0N(Info.bitfileID,8) << " for bitfile '" << inBitfilePath << "'");  return false;}
	if (Info.bitfileVersion > 0xfe)
		{BFMFAIL("Invalid bitfile version " << xHEX0N(Info.bitfileVersion,8) << " for bitfile '" << inBitfilePath << "'");  return false;}
	if (Info.bitfileFlags == 0)
		{BFMFAIL("No flags set for bitfile '" << inBitfilePath << "'");  return false;}
	if (Info.deviceID == 0)
		{BFMFAIL("Device ID is zero for bitfile '" << inBitfilePath << "'");  return false;}

	//	Add to list...
	_bitfileList.push_back(Info);
	BFMNOTE("Bitfile '" << inBitfilePath << "' successfully added to bitfile manager");
	return true;
}

bool CNTV2BitfileManager::AddDirectory (const string & inDirectory)
{
	AJAFileIO Fio;

	//	Check if good directory...
	if (AJA_FAILURE(Fio.DoesDirectoryExist(inDirectory)))
		{BFMFAIL("Bitfile directory '" << inDirectory << "' not found");  return false;}

	//	Get bitfiles...
	NTV2StringList fileContainer;
	if (AJA_FAILURE(Fio.ReadDirectory(inDirectory, "*.bit", fileContainer)))
		{BFMFAIL("ReadDirectory '" << inDirectory << "' failed");  return false;}

	// add bitfiles
	const size_t origNum(_bitfileList.size());
	for (NTV2StringListConstIter fcIter(fileContainer.begin());	 fcIter != fileContainer.end();	 ++fcIter)
		AddFile(*fcIter);
	BFMNOTE(DEC(_bitfileList.size() - origNum) << " bitfile(s) added from directory '" << inDirectory << "'");

	return true;
}

void CNTV2BitfileManager::Clear (void)
{
	if (!_bitfileList.empty()  ||  !_bitstreamList.empty())
		BFMNOTE(DEC(_bitfileList.size()) << " bitfile(s), " << DEC(_bitstreamList.size()) << " cached bitstream(s) cleared");
	_bitfileList.clear();
	_bitstreamList.clear();
}

size_t CNTV2BitfileManager::GetNumBitfiles (void)
{
	return _bitfileList.size();
}


bool CNTV2BitfileManager::GetBitStream (NTV2_POINTER & outBitstream,
										const ULWord inDesignID,
										const ULWord inDesignVersion,
										const ULWord inBitfileID,
										const ULWord inBitfileVersion,
										const ULWord inBitfileFlags)
{
	size_t numBitfiles (GetNumBitfiles());
	size_t maxNdx (numBitfiles);
	size_t ndx(0);

	for (ndx = 0;  ndx < numBitfiles;  ndx++)
	{	//	Search for bitstream...
		const NTV2BitfileInfo & info (_bitfileList.at(ndx));
		if (inDesignID == info.designID)
			if (inDesignVersion == info.designVersion)
				if (inBitfileID == info.bitfileID)
					if (inBitfileFlags & info.bitfileFlags)
					{
						if (inBitfileVersion == info.bitfileVersion)
							break;
						if ((maxNdx >= numBitfiles) || (info.bitfileVersion > _bitfileList.at(maxNdx).bitfileVersion))
							maxNdx = ndx;
					}
	}

	//	Looking for latest version?
	if ((inBitfileVersion == 0xff)	&&	(maxNdx < numBitfiles))
		ndx = maxNdx;

	//	Find something?
	if (ndx == numBitfiles)
	{	BFMFAIL("No bitstream found for designID=" << xHEX0N(inDesignID,8) << " designVers=" << xHEX0N(inDesignVersion,8)
				<< " bitfileID=" << xHEX0N(inBitfileID,8) << " bitfileVers=" << xHEX0N(inBitfileVersion,8));
		return false;
	}

	//	Read in bitstream...
	if (!ReadBitstream(ndx))
	{	BFMFAIL("No bitstream found for designID=" << xHEX0N(inDesignID,8) << " designVers=" << xHEX0N(inDesignVersion,8)
				<< " bitfileID=" << xHEX0N(inBitfileID,8) << " bitfileVers=" << xHEX0N(inBitfileVersion,8));
		return false;
	}

	outBitstream = _bitstreamList[ndx];
	return true;
}

bool CNTV2BitfileManager::ReadBitstream (const size_t inIndex)
{
	//	Already in cache?
	if ((inIndex < _bitstreamList.size())  &&  !_bitstreamList.at(inIndex).IsNULL())
		return true;	//	Yes

	//	Open bitfile to get bitstream...
	CNTV2Bitfile Bitfile;
	if (!Bitfile.Open(_bitfileList.at(inIndex).bitfilePath))
		{BFMFAIL("Bitfile '" << _bitfileList.at(inIndex).bitfilePath << "' failed to open");  return false;}

	//	Read bitstream from bitfile (will automatically Allocate it)...
	NTV2_POINTER Bitstream;
	if (!Bitfile.GetProgramByteStream(Bitstream))
		{BFMFAIL("GetProgramByteStream failed for bitfile '" << _bitfileList.at(inIndex).bitfilePath << "'");  return false;}

	if (inIndex >= _bitstreamList.size())
		_bitstreamList.resize(inIndex + 1);

	_bitstreamList[inIndex] = Bitstream;
	BFMDBG("Cached " << DEC(Bitstream.GetByteCount()) << "-byte bitstream for '" << _bitfileList.at(inIndex).bitfilePath << "' at index " << DEC(inIndex));
	return true;
}
