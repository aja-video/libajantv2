//
//  ntv2konalheplusservices.h
//
//  Copyright (c) 2011 AJA Video, Inc. All rights reserved.
//

#ifndef _KonaLHePlusServices_
#define _KonaLHePlusServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class KonaLHePlusServices
//-------------------------------------------------------------------------------------------------------
class KonaLHePlusServices : public DeviceServices
{
	
public:
	KonaLHePlusServices();
	~KonaLHePlusServices() {}
	
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat(
										NTV2VideoFormat fbVideoFormat,
										NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif