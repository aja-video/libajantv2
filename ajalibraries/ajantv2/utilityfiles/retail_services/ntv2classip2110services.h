//
//  ntv2classip2110services.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _ClassIP2110Services_
#define _ClassIP2110Services_

#include "ntv2class4kservices.h"

//-------------------------------------------------------------------------------------------------------
//	class ClassIP2110Services
//-------------------------------------------------------------------------------------------------------
class ClassIP2110Services : public Class4kServices
{
	
public:
	ClassIP2110Services(NTV2DeviceID devID);
	virtual ~ClassIP2110Services();

	virtual void SetDeviceXPointPlayback();
	virtual void SetDeviceXPointCapture();
	virtual void SetDeviceMiscRegisters();
	
	virtual void Init();
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
	uint32_t				startAudioCounter;
	uint32_t				endAudioCounter;
};


#endif
