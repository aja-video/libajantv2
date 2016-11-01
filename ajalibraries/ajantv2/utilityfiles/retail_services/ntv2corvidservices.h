//
//  ntv2corvidservices.h
//
//  Copyright (c) 2011 AJA Video, Inc. All rights reserved.
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
	
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat(
								NTV2VideoFormat fbVideoFormat,
								NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif