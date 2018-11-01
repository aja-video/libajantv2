//
//  ntv2ioip2022services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _IoIP2022Services_
#define _IoIP2022Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class IoIP2022Services
//-------------------------------------------------------------------------------------------------------
class IoIP2022Services : public DeviceServices
{
	
public:
	IoIP2022Services();
	~IoIP2022Services();
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
    
protected:
    CNTV2Config2022 *       config;

    NTV2Mode				mFb1ModeLast;
    NTV2VideoFormat			mFb1VideoFormatLast;
};


#endif
