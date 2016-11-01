//
//  ntv2ttapservices.h
//
//  Copyright (c) 2012 AJA Video, Inc. All rights reserved.
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
	
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
};


#endif