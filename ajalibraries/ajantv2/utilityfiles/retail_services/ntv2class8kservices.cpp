//
//  ntv2class8kservices.cpp
//
//  Copyright (c) 2019 AJA Video, Inc. All rights reserved.
//

#include "ntv2class8kservices.h"


//--------------------------------------------------------------------------------------------------
//	class Class8kServices
//--------------------------------------------------------------------------------------------------

Class8kServices::Class8kServices(NTV2DeviceID devID) : 
	Class4kServices(devID)
{
}


Class8kServices::~Class8kServices()
{
}


//--------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceXPointPlayback ()
{
	bool b8K = NTV2_IS_8K_VIDEO_FORMAT_TEMP(mFb1VideoFormat);
	if (!b8K)
		return Class4kServices::SetDeviceXPointPlayback();
}

	
//--------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceXPointCapture ()
{
	bool b8K = NTV2_IS_8K_VIDEO_FORMAT_TEMP(mFb1VideoFormat);
	if (!b8K)
		return Class4kServices::SetDeviceXPointCapture();
}


//--------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//--------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceMiscRegisters ()
{
	bool b8K = NTV2_IS_8K_VIDEO_FORMAT_TEMP(mFb1VideoFormat);
	if (!b8K)
		Class4kServices::SetDeviceMiscRegisters();
}

