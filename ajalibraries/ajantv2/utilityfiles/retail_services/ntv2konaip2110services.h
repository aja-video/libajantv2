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
	KonaIP2110Services(NTV2DeviceID devID);
    ~KonaIP2110Services();

    virtual void Init();
	virtual void SetDeviceXPointPlayback();
	virtual void SetDeviceXPointCapture();
	virtual void SetDeviceMiscRegisters();
	virtual bool ReadDriverState();
	
	void EveryFrameTask2110(CNTV2Config2110* config2110,
							NTV2VideoFormat* videoFormatLast,
							NTV2Mode* modeLast,
							NetworkData2110* s2110NetworkLast,
							TransmitVideoData2110* s2110TxVideoDataLast,
							TransmitAudioData2110* s2110TxAudioDataLast,
							TransmitAncData2110* s2110TxAncDataLast,
							ReceiveVideoData2110* s2110RxVideoDataLast,
							ReceiveAudioData2110* s2110RxAudioDataLast,
							ReceiveAncData2110* s2110RxAncDataLast);
	NTV2VideoFormat ConvertVideoToStreamFormat(NTV2VideoFormat videoFormat);
	NTV2VideoFormat ConvertStreamToVideoFormat(NTV2VideoFormat videoFormat);
	void Print2110Network(const NetworkData2110 m2110Network);

protected:
    CNTV2Config2110 *       config2110;
    
    NetworkData2110			m2110Network;
    TransmitVideoData2110   m2110TxVideoData;
	TransmitAudioData2110   m2110TxAudioData;
	TransmitAncData2110   	m2110TxAncData;
    ReceiveVideoData2110    m2110RxVideoData;
	ReceiveAudioData2110    m2110RxAudioData;
	ReceiveAncData2110    	m2110RxAncData;
    IpStatus2110            m2110IpStatusData;

    NetworkData2110			m2110NetworkLast;
    TransmitVideoData2110   m2110TxVideoDataLast;
	TransmitAudioData2110   m2110TxAudioDataLast;
	TransmitAncData2110   	m2110TxAncDataLast;
    ReceiveVideoData2110    m2110RxVideoDataLast;
	ReceiveAudioData2110    m2110RxAudioDataLast;
	ReceiveAncData2110    	m2110RxAncDataLast;

	NTV2Mode				mFb1ModeLast;
    NTV2VideoFormat         mFb1VideoFormatLast;
    bool 					mHasSdiOut5;					
    bool 					mHasCSC5;					
    bool 					mHasLUT5;					
    bool 					mHas4kQuarter;					
};


#endif
