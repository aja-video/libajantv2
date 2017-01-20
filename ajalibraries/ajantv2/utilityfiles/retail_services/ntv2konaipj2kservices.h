//
//  ntv2konaipj2kservices.h
//
//  Copyright (c) 2017 AJA Video, Inc. All rights reserved.
//

#ifndef _KonaIPJ2kServices_
#define _KonaIPJ2kServices_


#include "ntv2deviceservices.h"
#include "ntv2config2022.h"

//-------------------------------------------------------------------------------------------------------
//	class KonaIPJ2kServices
//-------------------------------------------------------------------------------------------------------
class KonaIPJ2kServices : public DeviceServices
{
	
public:
    KonaIPJ2kServices();
    ~KonaIPJ2kServices();
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);

protected:
    void   setNetConfig(eSFP  port);

    CNTV2Config2022     * target;
};


#endif
