//
//  ntv2io4kufcservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Io4KUFCServices_
#define _Io4KUFCServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Io4KUfcServices
//-------------------------------------------------------------------------------------------------------
class Io4KUfcServices : public DeviceServices
{
	
public:
	Io4KUfcServices();
	virtual ~Io4KUfcServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
