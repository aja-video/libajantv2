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
    void   setNetConfig(eSFP  port);
    void   setRxConfig(NTV2Channel channel);
    void   setTxConfig(NTV2Channel channel);
	void   setIPError(NTV2Channel channel, uint32_t configType, uint32_t val);
    void   getIPError(NTV2Channel channel, uint32_t configType, uint32_t & val);

    bool   notEqual(const rx_2022_channel & hw_channel, const rx2022Config & virtual_config);
    bool   notEqual(const tx_2022_channel & hw_channel, const tx2022Config & virtual_config);

    CNTV2Config2022     * target;
};


#endif
