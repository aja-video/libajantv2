//
//  ntv2io4kufcservices.h
//
//  Copyright (c) 2014 AJA Video, Inc. All rights reserved.
//

#ifndef _Io4KUFCServices_
#define _Io4KUFCServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Io4KUfcServices
//-------------------------------------------------------------------------------------------------------
class Io4KUfcServices : public DeviceServices
{
	
public:
	Io4KUfcServices();
	~Io4KUfcServices() {}
	
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
									NTV2VideoFormat fbVideoFormat,
									NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif
