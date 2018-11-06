//
//  ntv2konalhiservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
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
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
};


#endif
