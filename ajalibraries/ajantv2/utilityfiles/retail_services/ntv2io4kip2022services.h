//
//  ntv2io4kip2022services.h
//
//  Copyright (c) 2013 AJA Video, Inc. All rights reserved.
//

#ifndef _Io4KIP2022Services_
#define _Io4KIP2022Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Io4KIP2022Services
//-------------------------------------------------------------------------------------------------------
class Io4KIP2022Services : public DeviceServices
{
	
public:
	Io4KIP2022Services();
	~Io4KIP2022Services();
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
									NTV2VideoFormat fbVideoFormat,
									NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif
