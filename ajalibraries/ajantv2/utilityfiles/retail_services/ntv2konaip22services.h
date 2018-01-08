//
//  ntv2konaip22services.h
//
//  Copyright (c) 2014 AJA Video, Inc. All rights reserved.
//

#ifndef _KonaIP22Services_
#define _KonaIP22Services_


#include "ntv2deviceservices.h"
#include "ntv2config2022.h"

//-------------------------------------------------------------------------------------------------------
//	class KonaIP22Services
//-------------------------------------------------------------------------------------------------------
class KonaIP22Services : public DeviceServices
{
	
public:
    KonaIP22Services();
	~KonaIP22Services();
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);

protected:
    CNTV2Config2022     * config;
};


#endif
