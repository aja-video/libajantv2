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
//	class Kona4QuadServices
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

    bool   notEqualPrimary(const rx_2022_channel & hw_channel, const rx2022Config & virtual_config);
    bool   notEqualSecondary(const rx_2022_channel & hw_channel, const rx2022Config & virtual_config);
    bool   notEqualPrimary(const tx_2022_channel & hw_channel, const tx2022Config & virtual_config);
    bool   notEqualSecondary(const tx_2022_channel & hw_channel, const tx2022Config & virtual_config);
    bool   notEqualMAC(uint32_t lo, uint32_t hi, const MACAddr & macaddr);

    CNTV2Config2022     * target;
};


#endif
