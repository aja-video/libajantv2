//
//  ntv2corvid44services.h
//
//  Copyright (c) 2014 AJA Video, Inc. All rights reserved.
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
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
};


#endif