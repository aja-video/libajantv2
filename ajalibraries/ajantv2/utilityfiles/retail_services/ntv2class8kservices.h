//
//  ntv2class8kservices.h
//
//  Copyright (c) 2019 AJA Video, Inc. All rights reserved.
//

#ifndef _Class8kServices_
#define _Class8kServices_


#include "ntv2class4kservices.h"

//--------------------------------------------------------------------------------------------------
//	class Class8kServices
//--------------------------------------------------------------------------------------------------
class Class8kServices : public Class4kServices
{
public:
	Class8kServices(NTV2DeviceID devID);
	~Class8kServices();
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
