//
//  ntv2kona4ufcservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Kona4UfcServices_
#define _Kona4UfcServices_

#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Io4KUfcServices
//-------------------------------------------------------------------------------------------------------
class Kona4UfcServices : public DeviceServices
{
	
public:
	Kona4UfcServices();
	virtual ~Kona4UfcServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
