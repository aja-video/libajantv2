//
//  ntv2kona3Gquadservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Kona3GQuadServices_
#define _Kona3GQuadServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Kona3GQuadServices
//-------------------------------------------------------------------------------------------------------
class Kona3GQuadServices : public DeviceServices
{
	
public:
	Kona3GQuadServices();
	~Kona3GQuadServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
