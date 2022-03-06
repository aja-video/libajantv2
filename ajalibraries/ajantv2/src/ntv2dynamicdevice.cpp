/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dynamicdevice.cpp
	@brief		Implementations of DMA-related CNTV2Card methods.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2bitfile.h"
#include "ntv2bitfilemanager.h"
#include "ajabase/system/debug.h"

using namespace std;

#define DDFAIL(__x__)	AJA_sERROR	(AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)
#define DDWARN(__x__)	AJA_sWARNING(AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)
#define DDNOTE(__x__)	AJA_sNOTICE (AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)
#define DDINFO(__x__)	AJA_sINFO	(AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)
#define DDDBG(__x__)	AJA_sDEBUG	(AJA_DebugUnit_Firmware,	AJAFUNC << ": " << __x__)


static CNTV2BitfileManager s_BitfileManager;


bool CNTV2Card::IsDynamicDevice (void)
{
	NTV2ULWordVector reg;

	if (!IsOpen())
		return false;	//	device not open

	if (!BitstreamStatus(reg))
		return false;	//	can't get bitstream status

	if (!reg[BITSTREAM_VERSION])
		return false;	//	Bitstream version is zero

	return true;
}

bool CNTV2Card::IsDynamicFirmwareLoaded (void)
{
	if (!IsDynamicDevice())
		return false;
	
	ULWord value(0);
	if (!ReadRegister(kVRegBaseFirmwareDeviceID, value))
		return false;
	return GetDeviceID() == NTV2DeviceID(value)  ?  false  :  true;
}

NTV2DeviceID CNTV2Card::GetBaseDeviceID (void)
{
	if (!IsDynamicDevice())
		return DEVICE_ID_INVALID;
	
	ULWord value(0);
	if (!ReadRegister(kVRegBaseFirmwareDeviceID, value))
		return DEVICE_ID_INVALID;
	return NTV2DeviceID(value);
}

NTV2DeviceIDList CNTV2Card::GetDynamicDeviceList (void)
{
	NTV2DeviceIDList	result;
	const NTV2DeviceIDSet devs(GetDynamicDeviceIDs());
	for (NTV2DeviceIDSetConstIter it(devs.begin());	 it != devs.end();	++it)
		result.push_back(*it);
	return result;
}

NTV2DeviceIDSet CNTV2Card::GetDynamicDeviceIDs (void)
{
	NTV2DeviceIDSet result;
	if (!IsOpen())
		return result;

	const NTV2DeviceID currentDeviceID (GetDeviceID());
	if (currentDeviceID == 0)
		return result;

	//	Get current design ID and version...
	NTV2ULWordVector reg;
	if (!BitstreamStatus(reg))
		return result;

	if (reg[BITSTREAM_VERSION] == 0)
		return result;

	ULWord currentUserID(0), currentDesignID(0), currentDesignVersion(0), currentBitfileID(0), currentBitfileVersion(0);

	if (GetRunningFirmwareUserID(currentUserID)  &&  currentUserID)
	{	//	The new way
		currentDesignID			= NTV2BitfileHeaderParser::GetDesignID(currentUserID);
		currentDesignVersion	= NTV2BitfileHeaderParser::GetDesignVersion(currentUserID);
		currentBitfileID		= NTV2BitfileHeaderParser::GetBitfileID(currentUserID);
		currentBitfileVersion	= NTV2BitfileHeaderParser::GetBitfileVersion(currentUserID);
	}
	else
	{	//	The old way
		currentDesignID			= NTV2BitfileHeaderParser::GetDesignID(reg[BITSTREAM_VERSION]);
		currentDesignVersion	= NTV2BitfileHeaderParser::GetDesignVersion(reg[BITSTREAM_VERSION]);
		currentBitfileID		= CNTV2Bitfile::ConvertToBitfileID(currentDeviceID);
		currentBitfileVersion	= 0xff; // ignores bitfile version
	}

	if (!currentDesignID)
		return result;

	//	Get the clear file matching current bitfile...
	NTV2_POINTER clearStream;
	if (!s_BitfileManager.GetBitStream (clearStream,
										currentDesignID,
										currentDesignVersion,
										currentBitfileID,
										currentBitfileVersion,
										NTV2_BITFILE_FLAG_CLEAR) || !clearStream)
		return result;

	//	Build the deviceID set...
	const NTV2BitfileInfoList & infoList (s_BitfileManager.GetBitfileInfoList());
	for (NTV2BitfileInfoListConstIter it(infoList.begin());	 it != infoList.end();	++it)
		if (it->designID == currentDesignID)
			if (it->designVersion == currentDesignVersion)
				if (it->bitfileFlags & NTV2_BITFILE_FLAG_PARTIAL)
				{
					const NTV2DeviceID devID (CNTV2Bitfile::ConvertToDeviceID(it->designID, it->bitfileID));
					if (result.find(devID) == result.end())
						result.insert(devID);
				}
	return result;
}

bool CNTV2Card::CanLoadDynamicDevice (const NTV2DeviceID inDeviceID)
{
	const NTV2DeviceIDSet devices(GetDynamicDeviceIDs());
	return devices.find(inDeviceID) != devices.end();
}

bool CNTV2Card::LoadDynamicDevice (const NTV2DeviceID inDeviceID)
{
	if (!IsOpen())
		{DDFAIL("Device not open");  return false;}

	const NTV2DeviceID currentDeviceID (GetDeviceID());
	if (!currentDeviceID)
		{DDFAIL("Current device ID is zero");  return false;}

	const string oldDevName (GetDisplayName());

	//	Get current design ID and version...
	NTV2ULWordVector regs;
	if (!BitstreamStatus(regs))
		{DDFAIL("Unable to read current bitstream status for " << oldDevName);  return false;}

	if (!regs[BITSTREAM_VERSION])
		{DDFAIL("Bitstream version is zero for " << oldDevName);  return false;}

	ULWord currentUserID(0), currentDesignID(0), currentDesignVersion(0), currentBitfileID(0), currentBitfileVersion(0);

	if (GetRunningFirmwareUserID(currentUserID)  &&  currentUserID)
	{	// the new way:
		currentDesignID			= NTV2BitfileHeaderParser::GetDesignID(currentUserID);
		currentDesignVersion	= NTV2BitfileHeaderParser::GetDesignVersion(currentUserID);
		currentBitfileID		= NTV2BitfileHeaderParser::GetBitfileID(currentUserID);
		currentBitfileVersion	= NTV2BitfileHeaderParser::GetBitfileVersion(currentUserID);
	}
	else
	{	// the old way:
		currentDesignID			= NTV2BitfileHeaderParser::GetDesignID(regs[BITSTREAM_VERSION]);
		currentDesignVersion	= NTV2BitfileHeaderParser::GetDesignVersion(regs[BITSTREAM_VERSION]);
		currentBitfileID		= CNTV2Bitfile::ConvertToBitfileID(currentDeviceID);
		currentBitfileVersion	= 0xff; // ignores bitfile version
	}

	if (!currentDesignID)
		{DDFAIL("Current design ID is zero for " << oldDevName);  return false;}

	//	Get the clear file matching current bitfile...
	NTV2_POINTER clearStream;
	if (!s_BitfileManager.GetBitStream (clearStream,
										currentDesignID,
										currentDesignVersion,
										currentBitfileID,
										currentBitfileVersion,
										NTV2_BITFILE_FLAG_CLEAR) || !clearStream)
		{DDFAIL("GetBitStream 'clear' failed for " << oldDevName);  return false;}

	//	Get the partial file matching the inDeviceID...
	NTV2_POINTER partialStream;
	if (!s_BitfileManager.GetBitStream (partialStream,
										currentDesignID,
										currentDesignVersion,
										CNTV2Bitfile::ConvertToBitfileID(inDeviceID),
										0xff,
										NTV2_BITFILE_FLAG_PARTIAL) || !partialStream)
		{DDFAIL("GetBitStream 'partial' failed for " << oldDevName);  return false;}

	//	Load the clear bitstream...
	if (!BitstreamWrite (clearStream, true, true))
		{DDFAIL("BitstreamWrite failed writing 'clear' bitstream for " << oldDevName);  return false;}
	//	Load the partial bitstream...
	if (!BitstreamWrite (partialStream, false, true))
		{DDFAIL("BitstreamWrite failed writing 'partial' bitstream for " << oldDevName);  return false;}

	DDNOTE(oldDevName << " dynamically changed to '" << ::NTV2DeviceIDToString(inDeviceID) << "' (" << xHEX0N(inDeviceID,8) << ")");
	return true;
}	//	LoadDynamicDevice

bool CNTV2Card::AddDynamicBitfile (const string & inBitfilePath)
{
	return s_BitfileManager.AddFile(inBitfilePath);
}

bool CNTV2Card::AddDynamicDirectory (const string & inDirectory)
{
	return s_BitfileManager.AddDirectory(inDirectory);
}
