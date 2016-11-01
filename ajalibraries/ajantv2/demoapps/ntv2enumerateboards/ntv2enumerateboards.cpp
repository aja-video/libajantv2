/**
	@file		ntv2enumerateboards.cpp
	@brief		Implementation of NTV2EnumerateDevices class
	@copyright	Copyright (C) 2013-2016 AJA Video Systems, Inc.  All rights reserved.
**/


// Platform dependent includes
#include "ajatypes.h"

// SDK includes
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ntv2enumerateboards.h"



NTV2EnumerateDevices::NTV2EnumerateDevices ()
	:	mDeviceScanner	(false)
{
	mDeviceScanner.ScanHardware ();

}	//	constructor



NTV2EnumerateDevices::~NTV2EnumerateDevices ()
{
}	//	destructor



size_t NTV2EnumerateDevices::GetDeviceCount (void) const
{
	return mDeviceScanner.GetNumDevices ();

}	//	GetDeviceCount



std::string NTV2EnumerateDevices::GetDescription (uint32_t inDeviceIndex) const
{
	std::string	result;

	if (inDeviceIndex < mDeviceScanner.GetNumDevices ())
		result = ::NTV2DeviceIDToString (mDeviceScanner.GetDeviceInfoList () [inDeviceIndex].deviceID);

	return result;

}	//	GetDescription



bool NTV2EnumerateDevices::GetDeviceInfo (uint32_t inDeviceIndex, NTV2DeviceInfo & boardInfo) const
{
	if (inDeviceIndex < mDeviceScanner.GetNumDevices ())
	{
		boardInfo = mDeviceScanner.GetDeviceInfoList ()[inDeviceIndex];
		return true;	//	Success
	}
	else
	{
		::memset (&boardInfo, 0, sizeof (boardInfo));
		return false;	//	Fail
	}

}	//	GetDeviceInfo
