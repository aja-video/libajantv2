//
//  ntv2konahdmiservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _KonaHDMIServices_
#define _KonaHDMIServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class KonaHDMIServices
//-------------------------------------------------------------------------------------------------------
class KonaHDMIServices : public DeviceServices
{
	
public:
	KonaHDMIServices();
	~KonaHDMIServices() {}
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
