//
//  ntv2io4kplusservices.h
//
//  Copyright (c) 2013 AJA Video, Inc. All rights reserved.
//

#ifndef _Io4KPlusServices_
#define _Io4KPlusServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Io4KServices
//-------------------------------------------------------------------------------------------------------
class Io4KPlusServices : public DeviceServices
{
	
public:
	Io4KPlusServices();
	~Io4KPlusServices() {}
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
									NTV2VideoFormat fbVideoFormat,
									NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif
