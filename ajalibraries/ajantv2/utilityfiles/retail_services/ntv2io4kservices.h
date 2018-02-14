//
//  ntv2io4kservices.h
//
//  Copyright (c) 2013 AJA Video, Inc. All rights reserved.
//

#ifndef _Io4KServices_
#define _Io4KServices_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class Io4KServices
//-------------------------------------------------------------------------------------------------------
class Io4KServices : public DeviceServices
{
	
public:
	Io4KServices();
	~Io4KServices() {}
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
									NTV2VideoFormat fbVideoFormat,
									NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
};


#endif
