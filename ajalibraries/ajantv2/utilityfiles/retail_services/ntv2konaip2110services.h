//
//  ntv2konaip2110services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _KonaIP2110Services_
#define _KonaIP2110Services_


#include "ntv2deviceservices.h"

//-------------------------------------------------------------------------------------------------------
//	class KonaIP2110Services
//-------------------------------------------------------------------------------------------------------
class KonaIP2110Services : public DeviceServices
{
	
public:
	KonaIP2110Services();
    ~KonaIP2110Services();
	
	virtual void UpdateAutoState (void);
	virtual void SetDeviceXPointPlayback ();
	virtual void SetDeviceXPointCapture ();
	virtual void SetDeviceMiscRegisters ();

protected:

    bool NotEqual(const NetworkData2110 & netData2110a, const NetworkData2110 & netData2110b);
    bool NotEqual(const TransmitVideoData2110 & txVideoData2110a, const TransmitVideoData2110 & txVideoData2110b);
    bool NotEqual(const TransmitAudioData2110 & txAudioData2110a, const TransmitAudioData2110 & txAudioData2110b);
    bool NotEqual(const ReceiveVideoData2110 & rxVideoData2110a, const ReceiveVideoData2110 & rxVideoData2110b);
    bool NotEqual(const ReceiveAudioData2110 & rxAudioData2110a, const ReceiveAudioData2110 & rxAudioData2110b);

    CNTV2Config2110     * config2110;

    NetworkData2110			m2110NetworkLast;
    TransmitVideoData2110   m2110TxVideoDataLast;
    TransmitAudioData2110   m2110TxAudioDataLast;
    ReceiveVideoData2110    m2110RxVideoDataLast;
    ReceiveAudioData2110    m2110RxAudioDataLast;
};


#endif
