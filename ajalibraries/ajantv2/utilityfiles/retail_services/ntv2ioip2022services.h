//
//  ntv2ioip2022services.h
//
//  Copyright (c) 2013 AJA Video, Inc. All rights reserved.
//

#ifndef _IoIP2022Services_
#define _IoIP2022Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class IoIP2022Services
//-------------------------------------------------------------------------------------------------------
class IoIP2022Services : public DeviceServices
{
	
public:
	IoIP2022Services();
	~IoIP2022Services();
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback (GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture (GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters (NTV2Mode mode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat (
									NTV2VideoFormat fbVideoFormat,
									NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
    
protected:
    bool	notEqual(const rx_2022_channel & hw_channel, const rx2022Config & virtual_config);
    bool	notEqual(const tx_2022_channel & hw_channel, const tx2022Config & virtual_config);

    CNTV2Config2022     * config;
};


#endif
