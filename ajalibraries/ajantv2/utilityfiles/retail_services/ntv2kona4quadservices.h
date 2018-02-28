//
//  ntv2kona4quadservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
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
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
    virtual NTV2VideoFormat GetSelectedInputVideoFormat (NTV2VideoFormat fbVideoFormat,
                                                         NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif
