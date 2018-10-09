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
    virtual NTV2VideoFormat GetSelectedInputVideoFormat (
					NTV2VideoFormat fbVideoFormat,
					NTV2ColorSpaceMode* inputColorSpace=NULL);

protected:
	bool bDoHdmiIn;
	bool bDoHdmiOut;
	bool bDoSdiOut5;
	bool bDoAnalogOut;
	bool bDoCSC5;
	bool bDoLUT5;
	bool bDo12G;
};


#endif
