//
//  ntv2corvid3gservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Corvid3gServices_
#define _Corvid3gServices_

#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Corvid3GServices
//-------------------------------------------------------------------------------------------------------
class Corvid3GServices : public DeviceServices
{
	
public:
	Corvid3GServices();
	~Corvid3GServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
