//
//  ntv2ioexpressservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _IoExpressServices_
#define _IoExpressServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class IoExpressServices
//-------------------------------------------------------------------------------------------------------
class IoExpressServices : public DeviceServices
{
	
public:
	IoExpressServices();
	~IoExpressServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
