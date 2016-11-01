//
//  ntv2kona4ufcservices.h
//
//  Copyright (c) 2014 AJA Video, Inc. All rights reserved.
//

#ifndef _Kona4UfcServices_
#define _Kona4UfcServices_

#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Io4KUfcServices
//-------------------------------------------------------------------------------------------------------
class Kona4UfcServices : public DeviceServices
{
	
public:
	Kona4UfcServices();
	~Kona4UfcServices() {}
	
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
};


#endif
