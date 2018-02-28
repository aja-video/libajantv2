//
//  ntv2ttapservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _TTapServices_
#define _TTapServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class TTapServices
//-------------------------------------------------------------------------------------------------------
class TTapServices : public DeviceServices
{
	
public:
	TTapServices();
	~TTapServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
