//
//  ntv2ioxtservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _IoXTServices_
#define _IoXTServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class IoXTServices
//-------------------------------------------------------------------------------------------------------
class IoXTServices : public DeviceServices
{
	
public:
	IoXTServices();
	~IoXTServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
									NTV2VideoFormat fbVideoFormat,
									NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif
