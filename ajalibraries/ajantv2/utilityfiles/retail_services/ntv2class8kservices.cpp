//
//  ntv2class8kservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2class8kservices.h"


//-------------------------------------------------------------------------------------------------------
//	class Class8kServices
//-------------------------------------------------------------------------------------------------------

Class8kServices::Class8kServices(NTV2DeviceID devID) : Class4kServices(devID)
{
}


Class8kServices::~Class8kServices()
{
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceXPointPlayback ()
{
	// supper class handles non-8K stuff
	Class4kServices::SetDeviceXPointPlayback();
}

	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceXPointCapture ()
{
	// supper class handles non-8K stuff
	Class4kServices::SetDeviceXPointCapture();
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void Class8kServices::SetDeviceMiscRegisters ()
{
	// supper class handles non-8K stuff
	Class4kServices::SetDeviceMiscRegisters();
}

