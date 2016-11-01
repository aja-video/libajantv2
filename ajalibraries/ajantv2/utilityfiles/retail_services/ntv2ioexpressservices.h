//
//  ntv2ioexpressservices.h
//
//  Copyright (c) 2011 AJA Video, Inc. All rights reserved.
//

#ifndef _IoExpressServices_
#define _IoExpressServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class IoExpressServices
//-------------------------------------------------------------------------------------------------------
class IoExpressServices : public DeviceServices
{
	
public:
	IoExpressServices();
	~IoExpressServices() {}
	
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
									NTV2VideoFormat fbVideoFormat,
									NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif