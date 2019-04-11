//
//  ntv2classip2022services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _ClassIP2022Services_
#define _ClassIP2022Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class ClassIP2022Services
//-------------------------------------------------------------------------------------------------------
class ClassIP2022Services : public DeviceServices
{
	
public:
	ClassIP2022Services(NTV2DeviceID deviceID);
	~ClassIP2022Services();
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
    
protected:
    CNTV2Config2022 *       config;

    NTV2Mode				mFb1ModeLast;
    NTV2VideoFormat			mFb1VideoFormatLast;
};


#endif
