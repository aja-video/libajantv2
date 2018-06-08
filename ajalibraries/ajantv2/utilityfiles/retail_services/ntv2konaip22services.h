//
//  ntv2konaip22services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _KonaIP22Services_
#define _KonaIP22Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class KonaIP22Services
//-------------------------------------------------------------------------------------------------------
class KonaIP22Services : public DeviceServices
{
	
public:
    KonaIP22Services();
	~KonaIP22Services();
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();

protected:
    CNTV2Config2022     * config;

    NTV2Mode				mFb1ModeLast;
};


#endif
