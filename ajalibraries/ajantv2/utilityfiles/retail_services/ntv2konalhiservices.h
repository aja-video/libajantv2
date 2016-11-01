//
//  ntv2konalhiservices.h
//
//  Copyright (c) 2011 AJA Video, Inc. All rights reserved.
//

#ifndef _KonaLHiServices_
#define _KonaLHiServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class KonaLHiServices
//-------------------------------------------------------------------------------------------------------
class KonaLHiServices : public DeviceServices
{
	
public:
	KonaLHiServices();
	~KonaLHiServices() {}
	
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
								NTV2VideoFormat fbVideoFormat,
								NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif