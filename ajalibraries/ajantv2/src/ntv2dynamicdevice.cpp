/**
	@file		ntv2dynamicdevice.cpp
	@brief		Implementations of DMA-related CNTV2Card methods.
	@copyright	(C) 2004-2020 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2bitfile.h"
#include "ntv2bitmanager.h"
#include <algorithm>
#include <vector>

static CNTV2BitManager s_BitManager;

bool
CNTV2Card::IsDynamicDevice(void)
{
	ULWord reg[BITSTREAM_NUM_REGISTERS];

	if (!IsOpen())
		return false;

	// see if we can get bitstream status
	if (!BitstreamStatus(reg, BITSTREAM_NUM_REGISTERS))
		return false;

	// the bitstream version cannot be 0
	if (reg[BITSTREAM_VERSION] == 0)
		return false;

	return true;
}

std::vector<NTV2DeviceID>
CNTV2Card::GetDynamicDeviceList(void)
{
	ULWord reg[BITSTREAM_NUM_REGISTERS];
	std::vector<NTV2DeviceID> devIDList;

	if (!IsOpen())
		return devIDList;

	// get current design ID and version
	if (!BitstreamStatus(reg, BITSTREAM_NUM_REGISTERS))
		return devIDList;

	if (reg[BITSTREAM_VERSION] == 0)
		return devIDList;

	ULWord designID = CNTV2Bitfile::GetDesignID(reg[BITSTREAM_VERSION]);
	ULWord designVersion = CNTV2Bitfile::GetDesignVersion(reg[BITSTREAM_VERSION]);

	if (designID == 0) 
		return devIDList;

	// get current bitfile ID and version
	NTV2DeviceID deviceID = GetDeviceID();
    ULWord bitfileID = CNTV2Bitfile::ConvertToBitfileID(deviceID);
	UWord bitfileVersion = 0;
	GetRunningFirmwareRevision(bitfileVersion);

	if ((deviceID == 0) || (bitfileID == 0))
		return devIDList;

	// get the clear file matching current bitfile
	NTV2_POINTER clearStream;
	if (!s_BitManager.GetBitStream(clearStream,
								   designID,
								   designVersion,
								   bitfileID,
								   bitfileVersion,
								   NTV2_BITFILE_FLAG_CLEAR) || !clearStream)
		return devIDList;

	// build the deviceID list
	NTV2BitfileInfoList infoList = s_BitManager.GetBitfileInfoList();
	NTV2BitfileInfoListIter il;
	for (il = infoList.begin(); il != infoList.end(); ++il)
	{
		if ((il->designID == designID) &&
			(il->designVersion == designVersion) &&
			((il->bitfileFlags & NTV2_BITFILE_FLAG_PARTIAL) != 0))
		{
			NTV2DeviceID devID = CNTV2Bitfile::ConvertToDeviceID(il->designID, il->bitfileID);
			if (std::find(devIDList.begin(), devIDList.end(), devID) == devIDList.end())
				devIDList.push_back(devID);
		}
	}

	return devIDList;
}

bool
CNTV2Card::CanLoadDynamicDevice(NTV2DeviceID inDeviceID)
{
    std::vector<NTV2DeviceID> deviceList = GetDynamicDeviceList();
    if (std::find(deviceList.begin(), deviceList.end(), inDeviceID) == deviceList.end())
        return false;
    return true;
}

bool
CNTV2Card::LoadDynamicDevice(NTV2DeviceID inDeviceID)
{
	ULWord reg[BITSTREAM_NUM_REGISTERS];

	if (!IsOpen())
		return false;

	// get current design ID and version
	if (!BitstreamStatus(reg, BITSTREAM_NUM_REGISTERS))
		return false;

	if (reg[BITSTREAM_VERSION] == 0)
		return false;

	ULWord designID = CNTV2Bitfile::GetDesignID(reg[BITSTREAM_VERSION]);
	ULWord designVersion = CNTV2Bitfile::GetDesignVersion(reg[BITSTREAM_VERSION]);

	if (designID == 0) 
		return false;

	NTV2DeviceID deviceID = GetDeviceID();
	ULWord bitfileID = CNTV2Bitfile::ConvertToBitfileID(deviceID);
	UWord bitfileVersion = 0;
	GetRunningFirmwareRevision(bitfileVersion);

	if ((deviceID == 0) || (bitfileID == 0))
		return false;

	// get the clear file matching current bitfile
	NTV2_POINTER clearStream;
	if (!s_BitManager.GetBitStream(clearStream,
								   designID,
								   designVersion,
								   bitfileID,
								   bitfileVersion,
								   NTV2_BITFILE_FLAG_CLEAR) || !clearStream)
		return false;

	// get the partial file matching the inDeviceID
	NTV2_POINTER partialStream;
	if (!s_BitManager.GetBitStream(partialStream,
								   designID,
								   designVersion,
								   CNTV2Bitfile::ConvertToBitfileID(inDeviceID),
								   0xff,
								   NTV2_BITFILE_FLAG_PARTIAL) || !partialStream)
		return false;

	// load the clear bitstream
	if (!BitstreamWrite (clearStream, true, true))
		return false;
	// load the partial bitstream
	if (!BitstreamWrite (partialStream, false, true))
		return false;

	return true;
}

bool
CNTV2Card::AddDynamicBitfile(const std::string & inBitfilePath)
{
	return s_BitManager.AddFile(inBitfilePath);
}

bool
CNTV2Card::AddDynamicDirectory(const std::string & inDirectory)
{
	return s_BitManager.AddDirectory(inDirectory);
}
