//
//  ntv2class4kservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _Class4kServices_
#define _Class4kServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Class4kServices
//-------------------------------------------------------------------------------------------------------
class Class4kServices : public DeviceServices
{
public:
	Class4kServices(NTV2DeviceID devID);
	~Class4kServices();
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();

protected:
	bool mHasHdmiIn;
	bool mHasHdmiOut;
	bool mHasSdiOut5;
	bool mHasAnalogOut;
	bool mHasCSC5;
	bool mHasLUT5;
	bool mHas12G;
	bool mHas4kQuarter;
};


#endif
