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
	
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
								NTV2VideoFormat fbVideoFormat,
								NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif
