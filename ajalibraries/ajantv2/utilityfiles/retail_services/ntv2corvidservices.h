//
//  ntv2corvidservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _CorvidServices_
#define _CorvidServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class CorvidServices
//-------------------------------------------------------------------------------------------------------
class CorvidServices : public DeviceServices
{
	
public:
	CorvidServices();
	~CorvidServices() {}
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
	virtual NTV2VideoFormat GetSelectedInputVideoFormat(
								NTV2VideoFormat fbVideoFormat,
								NTV2ColorSpaceMode* inputFormatSelect=NULL);
};


#endif
