//
//  ntv2io4kip2110services.h
//
//  Copyright (c) 2013 AJA Video, Inc. All rights reserved.
//

#ifndef _Io4KIP2110Services_
#define _Io4KIP2110Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class IoIP2110Services
//-------------------------------------------------------------------------------------------------------
class Io4KIP2110Services : public DeviceServices
{
	
public:
	Io4KIP2110Services();
	~Io4KIP2110Services();
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
									NTV2VideoFormat fbVideoFormat,
									NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif
