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
    CNTV2Config2110     * config2110;

    NetworkData2110			m2110NetworkLast;
    TransmitVideoData2110   m2110TxVideoDataLast;
    TransmitAudioData2110   m2110TxAudioDataLast;
    ReceiveVideoData2110    m2110RxVideoDataLast;
    ReceiveAudioData2110    m2110RxAudioDataLast;

    NTV2VideoFormat         mFb1VideoFormatLast;
};


#endif
