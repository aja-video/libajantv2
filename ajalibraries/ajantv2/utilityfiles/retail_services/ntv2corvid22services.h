//
//  ntv2corvid22services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Corvid22Services_
#define _Corvid22Services_

#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Corvid22Services
//-------------------------------------------------------------------------------------------------------
class Corvid22Services : public DeviceServices
{
	
public:
	Corvid22Services();
	virtual ~Corvid22Services() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
