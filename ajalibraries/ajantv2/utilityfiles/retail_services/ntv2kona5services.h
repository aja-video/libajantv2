//
//  ntv2kona5services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Kona5Services_
#define _Kona5Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Io4KPlusServices
//-------------------------------------------------------------------------------------------------------
class Kona5Services : public DeviceServices
{
	
public:
	Kona5Services();
	~Kona5Services();
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
    virtual NTV2VideoFormat GetSelectedInputVideoFormat (NTV2VideoFormat fbVideoFormat,
                                                         NTV2ColorSpaceMode* inputColorSpace=NULL);
};


#endif
