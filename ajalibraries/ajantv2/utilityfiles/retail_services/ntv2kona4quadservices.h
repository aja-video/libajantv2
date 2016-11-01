//
//  ntv2kona4quadservices.h
//
//  Copyright (c) 2014 AJA Video, Inc. All rights reserved.
//

#ifndef _Kona4QuadServices_
#define _Kona4QuadServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Kona4QuadServices
//-------------------------------------------------------------------------------------------------------
class Kona4QuadServices : public DeviceServices
{
	
public:
	Kona4QuadServices();
	~Kona4QuadServices() {}
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
};


#endif