//
//  ntv2corvid88services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Corvid88Services_
#define _Corvid88Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Corvid88Services
//-------------------------------------------------------------------------------------------------------
class Corvid88Services : public DeviceServices
{
	
public:
	Corvid88Services();
	~Corvid88Services() {}
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
