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
    void	setNetConfig(eSFP  port);
	void	setRxConfig(NTV2Channel channel);
	void	setTxConfig(NTV2Channel channel);
	void	setIPError(NTV2Channel channel, uint32_t configType, uint32_t val);

	
	bool	notEqual(const rx_2022_channel & hw_channel, const rx2022Config & virtual_config);
	bool	notEqual(const tx_2022_channel & hw_channel, const tx2022Config & virtual_config);
	void	printRxConfig(rx_2022_channel chan);
	void	printTxConfig(tx_2022_channel chan);
	void	printEncoderConfig(j2kEncoderConfig modelConfig, j2kEncoderConfig encoderConfig);
	void	printDecoderConfig(j2kDecoderConfig modelConfig, j2kDecoderConfig encoderConfig);

	CNTV2Config2022     * target;
};


#endif
