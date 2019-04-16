//
//  ntv2kona3Gservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Kona3GServices_
#define _Kona3GServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Kona3GServices
//-------------------------------------------------------------------------------------------------------
class Kona3GServices : public DeviceServices
{
	
public:
	Kona3GServices();
	virtual ~Kona3GServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
