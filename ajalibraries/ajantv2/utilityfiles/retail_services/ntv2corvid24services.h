//
//  ntv2corvid24services.h
//
//  Copyright (c) 2012 AJA Video, Inc. All rights reserved.
//

#ifndef _Corvid24Services_
	#define _Corvid24Services_


	#include "ntv2deviceservices.h"

	//-------------------------------------------------------------------------------------------------------
	//	class Corvid24Services
	//-------------------------------------------------------------------------------------------------------
	class Corvid24Services : public DeviceServices
	{
		public:
							Corvid24Services ();
			virtual inline	~Corvid24Services ()	{}
			
			virtual void	SetDeviceXPointPlayback ();
			virtual void	SetDeviceXPointCapture ();
			virtual void	SetDeviceMiscRegisters ();
	};


#endif
