//
//  ntv2corvid44services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Corvid44Services_
#define _Corvid44Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Corvid44Services
//-------------------------------------------------------------------------------------------------------
class Corvid44Services : public DeviceServices
{
	
public:
	Corvid44Services();
	~Corvid44Services() {}
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
