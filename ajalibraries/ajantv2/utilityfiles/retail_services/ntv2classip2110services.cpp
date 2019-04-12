//
//  ntv2classip2110services.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "ntv2classip2110services.h"
#include "ajabase/system/systemtime.h"

//-------------------------------------------------------------------------------------------------------
//	class ClassIP2110Services
//-------------------------------------------------------------------------------------------------------
ClassIP2110Services::ClassIP2110Services(NTV2DeviceID devID) :
Class4kServices(devID)
{
    config2110 			= NULL;
	mFb1ModeLast 		= NTV2_MODE_INVALID;
    mFb1VideoFormatLast = NTV2_FORMAT_UNKNOWN;

    Init();
}

ClassIP2110Services::~ClassIP2110Services()
{
	Class4kServices::~Class4kServices();
    if (config2110 != NULL)
    {
        delete config2110;
        config2110 = NULL;
    }
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointPlayback
//-------------------------------------------------------------------------------------------------------
void ClassIP2110Services::SetDeviceXPointPlayback ()
{
	// For now just call base class and let it do everythign, we'll override this function in case we need to
	// do anything special for IP devices
	return Class4kServices::SetDeviceXPointPlayback();
}

	
//-------------------------------------------------------------------------------------------------------
//	SetDeviceXPointCapture
//-------------------------------------------------------------------------------------------------------
void ClassIP2110Services::SetDeviceXPointCapture()
{
	// For now just call base class and let it do everythign, we'll override this function in case we need to
	// do anything special for IP devices
	return Class4kServices::SetDeviceXPointCapture();
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceMiscRegisters
//-------------------------------------------------------------------------------------------------------
void ClassIP2110Services::SetDeviceMiscRegisters()
{
	if (mCard->IsDeviceReady(true) == true)
    {
        if (config2110 == NULL)
        {
            config2110 = new CNTV2Config2110(*mCard);
            Init();
        }

        // Here we protect against Windows fast boot mode where everythign in the agent is cached
        // and restored at a restart or power up.  This will detect that condition because the network
        // in the hardware will not be configued.  In this case make sure we clear out our local cache
        // of the HW settings.
        IPVNetConfig    hwNetConfig1, hwNetConfig2;

        config2110->GetNetworkConfiguration(SFP_1, hwNetConfig1);
        config2110->GetNetworkConfiguration(SFP_2, hwNetConfig2);
        if ((hwNetConfig1.ipc_ip == 0) && (hwNetConfig1.ipc_subnet == 0) && (hwNetConfig1.ipc_gateway == 0) &&
            (hwNetConfig2.ipc_ip == 0) && (hwNetConfig2.ipc_subnet == 0) && (hwNetConfig2.ipc_gateway == 0))
        {
            //printf("Power on state or not configured\n");
            Init();
        }

        // Configure all of the 2110 IP settings
        EveryFrameTask2110(config2110, &mFb1VideoFormatLast, &mFb1ModeLast, &m2110NetworkLast,
                           &m2110TxVideoDataLast, &m2110TxAudioDataLast, &m2110TxAncDataLast,
                           &m2110RxVideoDataLast, &m2110RxAudioDataLast, &m2110RxAncDataLast);
    }

	return Class4kServices::SetDeviceMiscRegisters();
}

//-------------------------------------------------------------------------------------------------------
//	Init
//-------------------------------------------------------------------------------------------------------
void ClassIP2110Services::Init()
{
	memset(&m2110NetworkLast, 0, sizeof(NetworkData2110));
	memset(&m2110TxVideoDataLast, 0, sizeof(TransmitVideoData2110));
	memset(&m2110TxAudioDataLast, 0, sizeof(TransmitAudioData2110));
	memset(&m2110TxAncDataLast, 0, sizeof(TransmitAncData2110));
	memset(&m2110RxVideoDataLast, 0, sizeof(ReceiveVideoData2110));
	memset(&m2110RxAudioDataLast, 0, sizeof(ReceiveAudioData2110));
	memset(&m2110RxAncDataLast, 0, sizeof(ReceiveAncData2110));

	if (config2110 != NULL)
	{
		bool ipServiceEnable, ipServiceForceConfig;

		ipServiceEnable = false;
		while (ipServiceEnable == false)
		{
			AJATime::Sleep(10);

			config2110->SetIPServicesControl(true, false);
			config2110->GetIPServicesControl(ipServiceEnable, ipServiceForceConfig);
		}
	}
}


//-------------------------------------------------------------------------------------------------------
//	ReadDriverState
//-------------------------------------------------------------------------------------------------------
bool ClassIP2110Services::ReadDriverState (void)
{
	bool bChanged = DeviceServices::ReadDriverState();

	if (mDeviceID == DEVICE_ID_KONAIP_2110 ||
		mDeviceID == DEVICE_ID_IOIP_2110 )
	{
		bool bOk;
		bOk = mCard->ReadVirtualData(kNetworkData2110, &m2110Network, sizeof(NetworkData2110));
		if (bOk == false)
		{
			memset(&m2110Network, 0, sizeof(NetworkData2110));
			//printf("Failed to get 2110 Network params\n");
		}

		bOk = mCard->ReadVirtualData(kTransmitVideoData2110, &m2110TxVideoData, sizeof(TransmitVideoData2110));
		if (bOk == false)
		{
			memset(&m2110TxVideoData, 0, sizeof(TransmitVideoData2110));
			//printf("Failed to get 2110 Transmit Video params\n");
		}

		bOk = mCard->ReadVirtualData(kTransmitAudioData2110, &m2110TxAudioData, sizeof(TransmitAudioData2110));
		if (bOk == false)
		{
			memset(&m2110TxAudioData, 0, sizeof(TransmitAudioData2110));
			//printf("Failed to get 2110 Transmit Audio params\n");
		}

		bOk = mCard->ReadVirtualData(kTransmitAncData2110, &m2110TxAncData, sizeof(TransmitAncData2110));
		if (bOk == false)
		{
			memset(&m2110TxAncData, 0, sizeof(TransmitAncData2110));
			//printf("Failed to get 2110 Transmit Anc params\n");
		}

		bOk = mCard->ReadVirtualData(kReceiveVideoData2110, &m2110RxVideoData, sizeof(ReceiveVideoData2110));
		if (bOk == false)
		{
			memset(&m2110RxVideoData, 0, sizeof(ReceiveVideoData2110));
			//printf("Failed to get 2110 Receive Video params\n");
		}

		bOk = mCard->ReadVirtualData(kReceiveAudioData2110, &m2110RxAudioData, sizeof(ReceiveAudioData2110));
		if (bOk == false)
		{
			memset(&m2110RxAudioData, 0, sizeof(ReceiveAudioData2110));
			//printf("Failed to get 2110 Receive Audio params\n");
		}

		bOk = mCard->ReadVirtualData(kReceiveAncData2110, &m2110RxAncData, sizeof(ReceiveAncData2110));
		if (bOk == false)
		{
			memset(&m2110RxAncData, 0, sizeof(ReceiveAncData2110));
			//printf("Failed to get 2110 Receive Anc params\n");
		}

		bOk = mCard->ReadVirtualData(kChStatusData2110, &m2110IpStatusData, sizeof(IpStatus2110));
		if (bOk == false)
		{
			memset(&m2110IpStatusData, 0, sizeof(IpStatus2110));
			//printf("Failed to get 2110 Ip status params\n");
		}
	}

	return bChanged;
}

//-------------------------------------------------------------------------------------------------------
//	EveryFrameTask2110
//-------------------------------------------------------------------------------------------------------
void ClassIP2110Services::EveryFrameTask2110(CNTV2Config2110* config2110,
											 NTV2VideoFormat* videoFormatLast,
											 NTV2Mode* modeLast,
											 NetworkData2110* s2110NetworkLast,
											 TransmitVideoData2110* s2110TxVideoDataLast,
											 TransmitAudioData2110* s2110TxAudioDataLast,
											 TransmitAncData2110* s2110TxAncDataLast,
											 ReceiveVideoData2110* s2110RxVideoDataLast,
											 ReceiveAudioData2110* s2110RxAudioDataLast,
											 ReceiveAncData2110* s2110RxAncDataLast)
{
	bool ipServiceEnable, ipServiceForceConfig;

	mCard->ReadRegister(kRegAud1Counter, startAudioCounter);

	config2110->GetIPServicesControl(ipServiceEnable, ipServiceForceConfig);
	if (ipServiceEnable)
	{
		tx_2110Config txConfig;
		uint32_t vRegConfigStreamRefreshValue;
		uint32_t vRegConfigStreamRefreshMask;

		mCard->ReadRegister(kVRegIpConfigStreamRefresh, vRegConfigStreamRefreshValue);
		vRegConfigStreamRefreshMask = vRegConfigStreamRefreshValue;
		mCard->WriteRegister(kVRegIpConfigStreamRefresh, 0, vRegConfigStreamRefreshMask);

		// Handle reset case

		if ((m2110TxVideoData.numTxVideoChannels == 0) &&
			(m2110TxAudioData.numTxAudioChannels == 0) &&
			(m2110TxAncData.numTxAncChannels == 0) &&
			(m2110RxVideoData.numRxVideoChannels == 0) &&
			(m2110RxAncData.numRxAncChannels == 0) &&
			(m2110RxAudioData.numRxAudioChannels == 0))
		{
			for (uint32_t i=0; i<4; i++)
			{
				if (memcmp(&m2110TxVideoData.txVideoCh[i], &s2110TxVideoDataLast->txVideoCh[i], sizeof(TxVideoChData2110)) != 0)
				{
					printf("TX Video Reset disable %d\n", s2110TxVideoDataLast->txVideoCh[i].stream);
					config2110->SetTxStreamEnable(s2110TxVideoDataLast->txVideoCh[i].stream, false, false);
					s2110TxVideoDataLast->txVideoCh[i] = m2110TxVideoData.txVideoCh[i];
				}
				if (memcmp(&m2110TxAudioData.txAudioCh[i], &s2110TxAudioDataLast->txAudioCh[i], sizeof(TxAudioChData2110)) != 0)
				{
					printf("TX Audio Reset disable %d\n", s2110TxAudioDataLast->txAudioCh[i].stream);
					config2110->SetTxStreamEnable(s2110TxAudioDataLast->txAudioCh[i].stream, false, false);
					s2110TxAudioDataLast->txAudioCh[i] = m2110TxAudioData.txAudioCh[i];
				}
				if (memcmp(&m2110TxAncData.txAncCh[i], &s2110TxAncDataLast->txAncCh[i], sizeof(TxAncChData2110)) != 0)
				{
					printf("TX Anc Reset disable %d\n", s2110TxAncDataLast->txAncCh[i].stream);
					config2110->SetTxStreamEnable(s2110TxAncDataLast->txAncCh[i].stream, false, false);
					s2110TxAncDataLast->txAncCh[i] = m2110TxAncData.txAncCh[i];
				}
				if (memcmp(&m2110RxVideoData.rxVideoCh[i], &s2110RxVideoDataLast->rxVideoCh[i], sizeof(RxVideoChData2110)) != 0)
				{
					printf("RX Video Reset disable %d\n", s2110RxVideoDataLast->rxVideoCh[i].stream);
					config2110->SetRxStreamEnable(SFP_1, s2110RxVideoDataLast->rxVideoCh[i].stream, false);
					config2110->SetRxStreamEnable(SFP_2, s2110RxVideoDataLast->rxVideoCh[i].stream, false);
					s2110RxVideoDataLast->rxVideoCh[i] = m2110RxVideoData.rxVideoCh[i];
				}
				if (memcmp(&m2110RxAudioData.rxAudioCh[i], &s2110RxAudioDataLast->rxAudioCh[i], sizeof(RxAudioChData2110)) != 0)
				{
					printf("RX Audio Reset disable %d\n", s2110RxAudioDataLast->rxAudioCh[i].stream);
					config2110->SetRxStreamEnable(SFP_1, s2110RxAudioDataLast->rxAudioCh[i].stream, false);
					config2110->SetRxStreamEnable(SFP_2, s2110RxAudioDataLast->rxAudioCh[i].stream, false);
					s2110RxAudioDataLast->rxAudioCh[i] = m2110RxAudioData.rxAudioCh[i];
				}
				if (memcmp(&m2110RxAncData.rxAncCh[i], &s2110RxAncDataLast->rxAncCh[i], sizeof(RxAncChData2110)) != 0)
				{
					printf("RX Anc Reset disable %d\n", s2110RxAncDataLast->rxAncCh[i].stream);
					config2110->SetRxStreamEnable(SFP_1, s2110RxAncDataLast->rxAncCh[i].stream, false);
					config2110->SetRxStreamEnable(SFP_2, s2110RxAncDataLast->rxAncCh[i].stream, false);
					s2110RxAncDataLast->rxAncCh[i] = m2110RxAncData.rxAncCh[i];
				}
				m2110IpStatusData.txChStatus[i] = kIpStatusStopped;
				m2110IpStatusData.rxChStatus[i] = kIpStatusStopped;
			}
		}
		else
		{
			// See if any transmit video channels need configuring/enabling
			bool changed = false;
			for (uint32_t i=0; i<m2110TxVideoData.numTxVideoChannels; i++)
			{
				if (memcmp(&m2110TxVideoData.txVideoCh[i], &s2110TxVideoDataLast->txVideoCh[i], sizeof(TxVideoChData2110)) != 0 ||
					*videoFormatLast != mFb1VideoFormat ||
					ipServiceForceConfig)
				{
					// Process the configuration
					txConfig.init();
					txConfig.remoteIP[0] = m2110TxVideoData.txVideoCh[i].remoteIP[0];
					txConfig.remoteIP[1] = m2110TxVideoData.txVideoCh[i].remoteIP[1];
					txConfig.remotePort[0] = m2110TxVideoData.txVideoCh[i].remotePort[0];
					txConfig.remotePort[1] = m2110TxVideoData.txVideoCh[i].remotePort[1];
					txConfig.localPort[0] = m2110TxVideoData.txVideoCh[i].localPort[0];
					txConfig.localPort[1] = m2110TxVideoData.txVideoCh[i].localPort[1];
					txConfig.localPort[0] = m2110TxVideoData.txVideoCh[i].localPort[0];
					txConfig.localPort[1] = m2110TxVideoData.txVideoCh[i].localPort[1];
					txConfig.payloadType = m2110TxVideoData.txVideoCh[i].payloadType;
					txConfig.ttl = 0x40;
					txConfig.tos = 0x64;

					// Video specific
					txConfig.videoFormat = ConvertVideoToStreamFormat(mFb1VideoFormat);
					txConfig.videoSamples = VPIDSampling_YUV_422;

					// Start by turning off the video stream
					//printf("SetTxVideoStream off %d\n", m2110TxVideoData.txVideoCh[i].stream);
					config2110->SetTxStreamEnable(m2110TxVideoData.txVideoCh[i].stream, false, false);
					m2110IpStatusData.txChStatus[i] = kIpStatusStopped;

					if (config2110->SetTxStreamConfiguration(m2110TxVideoData.txVideoCh[i].stream, txConfig) == true)
					{
						//printf("SetTxStreamConfiguration Video OK\n");
						s2110TxVideoDataLast->txVideoCh[i] = m2110TxVideoData.txVideoCh[i];
						SetIPError((NTV2Channel)m2110TxVideoData.txVideoCh[i].stream, kErrNetworkConfig, NTV2IpErrNone);

						// Process the enable
						if (m2110TxVideoData.txVideoCh[i].enable)
						{
							//printf("SetTxVideoStream on %d\n", m2110TxVideoData.txVideoCh[i].stream);
							config2110->SetTxStreamEnable(m2110TxVideoData.txVideoCh[i].stream,
														  (bool)m2110TxVideoData.txVideoCh[i].sfpEnable[0],
														  (bool)m2110TxVideoData.txVideoCh[i].sfpEnable[1]);
							m2110IpStatusData.txChStatus[i] = kIpStatusRunning;
							// Something got congigured and enabled
							changed = true;
						}
					}
					else
					{
						printf("SetTxStreamConfiguration Video ERROR %s\n", config2110->getLastError().c_str());
						SetIPError((NTV2Channel)m2110TxVideoData.txVideoCh[i].stream, kErrNetworkConfig, config2110->getLastErrorCode());
						m2110IpStatusData.txChStatus[i] = kIpStatusFail;
					}
					AgentIsAlive();
				}
			}
			// if any of the channels got reconfigured and we are in a 4K format and doing multi SDP stuff then
			// we need to generate a new multi SDP
			if (changed && m2110Network.multiSDP && Is4KFormat(mFb1VideoFormat))
			{
				printf("SetTxStreamConfiguration generating MULTI SDP\n");
				config2110->GenSDP(SFP_1, NTV2_VIDEO4K_STREAM);
			}

			// See if any transmit audio channels need configuring/enabling
			for (uint32_t i=0; i<m2110TxAudioData.numTxAudioChannels; i++)
			{
				if (memcmp(&m2110TxAudioData.txAudioCh[i], &s2110TxAudioDataLast->txAudioCh[i], sizeof(TxAudioChData2110)) != 0 ||
					ipServiceForceConfig)
				{
					// Process the configuration
					txConfig.init();
					txConfig.remoteIP[0] = m2110TxAudioData.txAudioCh[i].remoteIP[0];
					txConfig.remoteIP[1] = m2110TxAudioData.txAudioCh[i].remoteIP[1];
					txConfig.remotePort[0] = m2110TxAudioData.txAudioCh[i].remotePort[0];
					txConfig.remotePort[1] = m2110TxAudioData.txAudioCh[i].remotePort[1];
					txConfig.localPort[0] = m2110TxAudioData.txAudioCh[i].localPort[0];
					txConfig.localPort[1] = m2110TxAudioData.txAudioCh[i].localPort[1];
					txConfig.localPort[0] = m2110TxAudioData.txAudioCh[i].localPort[0];
					txConfig.localPort[1] = m2110TxAudioData.txAudioCh[i].localPort[1];
					txConfig.payloadType = m2110TxAudioData.txAudioCh[i].payloadType;
					txConfig.ttl = 0x40;
					txConfig.tos = 0x64;

					// Audio specific
					txConfig.channel = m2110TxAudioData.txAudioCh[i].channel;
					txConfig.numAudioChannels = m2110TxAudioData.txAudioCh[i].numAudioChannels;
					txConfig.firstAudioChannel = m2110TxAudioData.txAudioCh[i].firstAudioChannel;
					txConfig.audioPktInterval = m2110TxAudioData.txAudioCh[i].audioPktInterval;

					// Start by turning off the audio stream
					//printf("SetTxAudioStream off %d\n", m2110TxAudioData.txAudioCh[i].stream);
					config2110->SetTxStreamEnable(m2110TxAudioData.txAudioCh[i].stream, false, false);

					if (config2110->SetTxStreamConfiguration(m2110TxAudioData.txAudioCh[i].stream, txConfig) == true)
					{
						//printf("SetTxStreamConfiguration Audio OK\n");
						s2110TxAudioDataLast->txAudioCh[i] = m2110TxAudioData.txAudioCh[i];
						SetIPError((NTV2Channel)m2110TxAudioData.txAudioCh[i].stream, kErrNetworkConfig, NTV2IpErrNone);

						// Process the enable
						if (m2110TxAudioData.txAudioCh[i].enable)
						{
							//printf("SetTxAudioStream on %d\n", m2110TxAudioData.txAudioCh[i].stream);
							config2110->SetTxStreamEnable(m2110TxAudioData.txAudioCh[i].stream,
														  (bool)m2110TxAudioData.txAudioCh[i].sfpEnable[0],
														  (bool)m2110TxAudioData.txAudioCh[i].sfpEnable[1]);
						}
					}
					else
					{
						printf("SetTxStreamConfiguration Audio ERROR %s\n", config2110->getLastError().c_str());
						SetIPError((NTV2Channel)m2110TxAudioData.txAudioCh[i].stream, kErrNetworkConfig, config2110->getLastErrorCode());
					}
					AgentIsAlive();
				}
			}

			// See if any transmit anc channels need configuring/enabling
			for (uint32_t i=0; i<m2110TxAncData.numTxAncChannels; i++)
			{
				if (memcmp(&m2110TxAncData.txAncCh[i], &s2110TxAncDataLast->txAncCh[i], sizeof(TxAncChData2110)) != 0 ||
					ipServiceForceConfig)
				{
					// Process the configuration
					txConfig.init();
					txConfig.remoteIP[0] = m2110TxAncData.txAncCh[i].remoteIP[0];
					txConfig.remoteIP[1] = m2110TxAncData.txAncCh[i].remoteIP[1];
					txConfig.remotePort[0] = m2110TxAncData.txAncCh[i].remotePort[0];
					txConfig.remotePort[1] = m2110TxAncData.txAncCh[i].remotePort[1];
					txConfig.localPort[0] = m2110TxAncData.txAncCh[i].localPort[0];
					txConfig.localPort[1] = m2110TxAncData.txAncCh[i].localPort[1];
					txConfig.localPort[0] = m2110TxAncData.txAncCh[i].localPort[0];
					txConfig.localPort[1] = m2110TxAncData.txAncCh[i].localPort[1];
					txConfig.payloadType = m2110TxAncData.txAncCh[i].payloadType;
					txConfig.ttl = 0x40;
					txConfig.tos = 0x64;

					// Start by turning off the anc stream
					//printf("SetTxAncStream off %d\n", m2110TxAncData.txAncCh[i].stream);
					config2110->SetTxStreamEnable(m2110TxAncData.txAncCh[i].stream, false, false);

					if (config2110->SetTxStreamConfiguration(m2110TxAncData.txAncCh[i].stream, txConfig) == true)
					{
						//printf("SetTxStreamConfiguration Anc OK\n");
						s2110TxAncDataLast->txAncCh[i] = m2110TxAncData.txAncCh[i];
						SetIPError((NTV2Channel)m2110TxAncData.txAncCh[i].stream, kErrNetworkConfig, NTV2IpErrNone);

						// Process the enable
						if (m2110TxAncData.txAncCh[i].enable)
						{
							//printf("SetTxAncStream on %d\n", m2110TxAncData.txAncCh[i].stream);
							config2110->SetTxStreamEnable(m2110TxAncData.txAncCh[i].stream,
														  (bool)m2110TxAncData.txAncCh[i].sfpEnable[0],
														  (bool)m2110TxAncData.txAncCh[i].sfpEnable[1]);
						}
					}
					else
					{
						printf("SetTxStreamConfiguration Anc ERROR %s\n", config2110->getLastError().c_str());
						SetIPError((NTV2Channel)m2110TxAncData.txAncCh[i].stream, kErrNetworkConfig, config2110->getLastErrorCode());
					}
					AgentIsAlive();
				}
			}

			rx_2110Config rxConfig;
			eSFP sfp = SFP_1;

			// See if any receive video channels need configuring/enabling
			for (uint32_t i=0; i<m2110RxVideoData.numRxVideoChannels; i++)
			{
				if (memcmp(&m2110RxVideoData.rxVideoCh[i], &s2110RxVideoDataLast->rxVideoCh[i], sizeof(RxVideoChData2110)) != 0 ||
					*videoFormatLast != mFb1VideoFormat ||
					vRegConfigStreamRefreshValue & (1 << (i+ NTV2_VIDEO1_STREAM)) ||
					ipServiceForceConfig)
				{
					rxConfig.init();
					if (m2110RxVideoData.rxVideoCh[i].sfpEnable[1])
					{
						// Use SFP 2 params
						sfp = SFP_2;
						rxConfig.sourceIP = m2110RxVideoData.rxVideoCh[i].sourceIP[1];
						rxConfig.destIP = m2110RxVideoData.rxVideoCh[i].destIP[1];
						rxConfig.sourcePort = m2110RxVideoData.rxVideoCh[i].sourcePort[1];
						rxConfig.destPort = m2110RxVideoData.rxVideoCh[i].destPort[1];
						sfp = SFP_2;
					}
					else if (m2110RxVideoData.rxVideoCh[i].sfpEnable[0])
					{
						// Use SFP 1 params
						sfp = SFP_1;
						rxConfig.sourceIP = m2110RxVideoData.rxVideoCh[i].sourceIP[0];
						rxConfig.destIP = m2110RxVideoData.rxVideoCh[i].destIP[0];
						rxConfig.sourcePort = m2110RxVideoData.rxVideoCh[i].sourcePort[0];
						rxConfig.destPort = m2110RxVideoData.rxVideoCh[i].destPort[0];
					}
					if (rxConfig.sourceIP != "0.0.0.0")
						rxConfig.rxMatch |= RX_MATCH_2110_SOURCE_IP;
					if (rxConfig.destIP != "0.0.0.0")
						rxConfig.rxMatch |= RX_MATCH_2110_DEST_IP;
					if (rxConfig.sourcePort != 0)
						rxConfig.rxMatch |= RX_MATCH_2110_SOURCE_PORT;
					if (rxConfig.destPort != 0)
						rxConfig.rxMatch |= RX_MATCH_2110_DEST_PORT;
					//if (rxConfig.vlan)
					//	rxConfig.rxMatch |= RX_MATCH_2110_VLAN;
					//if (rxConfig.payloadType)
					//	rxConfig.rxMatch |= RX_MATCH_2110_PAYLOAD;
					//if (rxConfig.ssrc)
					//	rxConfig.rxMatch |= RX_MATCH_2110_SSRC;
					rxConfig.payloadType = m2110RxVideoData.rxVideoCh[i].payloadType;

					// Video specific
					NTV2VideoFormat vFmt;
					if (mFollowInputFormat && (m2110RxVideoData.rxVideoCh[i].videoFormat != NTV2_FORMAT_UNKNOWN))
						vFmt = m2110RxVideoData.rxVideoCh[i].videoFormat;
					else
						vFmt = mFb1VideoFormat;

					rxConfig.videoFormat = ConvertVideoToStreamFormat(vFmt);

					// if format was not converted assume it was not a 4k format and disable 4k mode, otherwise enable it
					if (rxConfig.videoFormat == vFmt)
						config2110->Set4KModeEnable(false);
					else
						config2110->Set4KModeEnable(true);

					rxConfig.videoSamples = VPIDSampling_YUV_422;
					//printf("Format (%d, %d, %d, %d)\n", i, mFollowInputFormat, m2110Network.multiSDP, rxConfig.videoFormat);

					// Start by turning off the video receiver
					//printf("SetRxVideoStream off %d\n", m2110RxVideoData.rxVideoCh[i].stream);
					config2110->SetRxStreamEnable(sfp, m2110RxVideoData.rxVideoCh[i].stream, false);
					m2110IpStatusData.rxChStatus[i] = kIpStatusStopped;

					if (config2110->SetRxStreamConfiguration(sfp, m2110RxVideoData.rxVideoCh[i].stream, rxConfig) == true)
					{
						//printf("SetRxStreamConfiguration Video OK\n");
						s2110RxVideoDataLast->rxVideoCh[i] = m2110RxVideoData.rxVideoCh[i];
						SetIPError((NTV2Channel)m2110RxVideoData.rxVideoCh[i].stream, kErrNetworkConfig, NTV2IpErrNone);

						// Process the enable
						if (m2110RxVideoData.rxVideoCh[i].enable)
						{
							//printf("SetRxVideoStream on %d\n", m2110RxVideoData.rxVideoCh[i].stream);
							config2110->SetRxStreamEnable(sfp, m2110RxVideoData.rxVideoCh[i].stream, true);
							m2110IpStatusData.rxChStatus[i] = kIpStatusRunning;
						}
					}
					else
					{
						printf("SetRxStreamConfiguration Video ERROR %s\n", config2110->getLastError().c_str());
						SetIPError((NTV2Channel)m2110RxVideoData.rxVideoCh[i].stream, kErrNetworkConfig, config2110->getLastErrorCode());
						m2110IpStatusData.rxChStatus[i] = kIpStatusFail;
					}
					AgentIsAlive();
				}
			}

			//*videoFormatLast = mFb1VideoFormat;

			// See if any receive audio channels need configuring/enabling
			for (uint32_t i=0; i<m2110RxAudioData.numRxAudioChannels; i++)
			{
				if (memcmp(&m2110RxAudioData.rxAudioCh[i], &s2110RxAudioDataLast->rxAudioCh[i], sizeof(RxAudioChData2110)) != 0 ||
					vRegConfigStreamRefreshValue & (1 << (i+ NTV2_AUDIO1_STREAM)) ||
					ipServiceForceConfig)
				{
					rxConfig.init();
					if (m2110RxAudioData.rxAudioCh[i].sfpEnable[1])
					{
						// Use SFP 2 params
						sfp = SFP_2;
						rxConfig.sourceIP = m2110RxAudioData.rxAudioCh[i].sourceIP[1];
						rxConfig.destIP = m2110RxAudioData.rxAudioCh[i].destIP[1];
						rxConfig.sourcePort = m2110RxAudioData.rxAudioCh[i].sourcePort[1];
						rxConfig.destPort = m2110RxAudioData.rxAudioCh[i].destPort[1];
						sfp = SFP_2;
					}
					else if (m2110RxAudioData.rxAudioCh[i].sfpEnable[0])
					{
						// Use SFP 1 params
						sfp = SFP_1;
						rxConfig.sourceIP = m2110RxAudioData.rxAudioCh[i].sourceIP[0];
						rxConfig.destIP = m2110RxAudioData.rxAudioCh[i].destIP[0];
						rxConfig.sourcePort = m2110RxAudioData.rxAudioCh[i].sourcePort[0];
						rxConfig.destPort = m2110RxAudioData.rxAudioCh[i].destPort[0];
					}
					if (rxConfig.sourceIP != "0.0.0.0")
						rxConfig.rxMatch |= RX_MATCH_2110_SOURCE_IP;
					if (rxConfig.destIP != "0.0.0.0")
						rxConfig.rxMatch |= RX_MATCH_2110_DEST_IP;
					if (rxConfig.sourcePort != 0)
						rxConfig.rxMatch |= RX_MATCH_2110_SOURCE_PORT;
					if (rxConfig.destPort != 0)
						rxConfig.rxMatch |= RX_MATCH_2110_DEST_PORT;
					//if (rxConfig.vlan)
					//	rxConfig.rxMatch |= RX_MATCH_2110_VLAN;
					//if (rxConfig.payloadType)
					//	rxConfig.rxMatch |= RX_MATCH_2110_PAYLOAD;
					//if (rxConfig.ssrc)
					//	rxConfig.rxMatch |= RX_MATCH_2110_SSRC;
					rxConfig.payloadType = m2110RxAudioData.rxAudioCh[i].payloadType;

					// Audio specific
					rxConfig.numAudioChannels = m2110RxAudioData.rxAudioCh[i].numAudioChannels;
					rxConfig.audioPktInterval = m2110RxAudioData.rxAudioCh[i].audioPktInterval;

					// Start by turning off the audio receiver
					//printf("SetRxAudioStream off %d\n", m2110RxAudioData.rxAudioCh[i].stream);
					config2110->SetRxStreamEnable(sfp, m2110RxAudioData.rxAudioCh[i].stream, false);

					if (config2110->SetRxStreamConfiguration(sfp, m2110RxAudioData.rxAudioCh[i].stream, rxConfig) == true)
					{
						//printf("SetRxStreamConfiguration Audio OK\n");
						s2110RxAudioDataLast->rxAudioCh[i] = m2110RxAudioData.rxAudioCh[i];
						SetIPError(m2110RxAudioData.rxAudioCh[i].channel, kErrNetworkConfig, NTV2IpErrNone);

						// Process the enable
						if (m2110RxAudioData.rxAudioCh[i].enable)
						{
							//printf("SetRxAudioStream on %d\n", m2110RxAudioData.rxAudioCh[i].stream);
							config2110->SetRxStreamEnable(sfp, m2110RxAudioData.rxAudioCh[i].stream, true);
						}
					}
					else
					{
						printf("SetRxStreamConfiguration Audio ERROR %s\n", config2110->getLastError().c_str());
						SetIPError(m2110RxAudioData.rxAudioCh[i].channel, kErrNetworkConfig, config2110->getLastErrorCode());
					}
					AgentIsAlive();
				}
			}

			// See if any receive anc channels need configuring/enabling
			for (uint32_t i=0; i<m2110RxAncData.numRxAncChannels; i++)
			{
				if (memcmp(&m2110RxAncData.rxAncCh[i], &s2110RxAncDataLast->rxAncCh[i], sizeof(RxAncChData2110)) != 0 ||
					vRegConfigStreamRefreshValue & (1 << (i+ NTV2_ANC1_STREAM)) ||
					ipServiceForceConfig)
				{
					rxConfig.init();
					if (m2110RxAncData.rxAncCh[i].sfpEnable[1])
					{
						// Use SFP 2 params
						sfp = SFP_2;
						rxConfig.sourceIP = m2110RxAncData.rxAncCh[i].sourceIP[1];
						rxConfig.destIP = m2110RxAncData.rxAncCh[i].destIP[1];
						rxConfig.sourcePort = m2110RxAncData.rxAncCh[i].sourcePort[1];
						rxConfig.destPort = m2110RxAncData.rxAncCh[i].destPort[1];
						sfp = SFP_2;
					}
					else if (m2110RxAncData.rxAncCh[i].sfpEnable[0])
					{
						// Use SFP 1 params
						sfp = SFP_1;
						rxConfig.sourceIP = m2110RxAncData.rxAncCh[i].sourceIP[0];
						rxConfig.destIP = m2110RxAncData.rxAncCh[i].destIP[0];
						rxConfig.sourcePort = m2110RxAncData.rxAncCh[i].sourcePort[0];
						rxConfig.destPort = m2110RxAncData.rxAncCh[i].destPort[0];
					}
					if (rxConfig.sourceIP != "0.0.0.0")
						rxConfig.rxMatch |= RX_MATCH_2110_SOURCE_IP;
					if (rxConfig.destIP != "0.0.0.0")
						rxConfig.rxMatch |= RX_MATCH_2110_DEST_IP;
					if (rxConfig.sourcePort != 0)
						rxConfig.rxMatch |= RX_MATCH_2110_SOURCE_PORT;
					if (rxConfig.destPort != 0)
						rxConfig.rxMatch |= RX_MATCH_2110_DEST_PORT;
					//if (rxConfig.vlan)
					//	rxConfig.rxMatch |= RX_MATCH_2110_VLAN;
					//if (rxConfig.payloadType)
					//	rxConfig.rxMatch |= RX_MATCH_2110_PAYLOAD;
					//if (rxConfig.ssrc)
					//	rxConfig.rxMatch |= RX_MATCH_2110_SSRC;
					rxConfig.payloadType = m2110RxAncData.rxAncCh[i].payloadType;

					// Start by turning off the anc receiver
					//printf("SetRxAncStream off %d\n", m2110RxAncData.rxAncCh[i].stream);
					config2110->SetRxStreamEnable(sfp, m2110RxAncData.rxAncCh[i].stream, false);

					if (config2110->SetRxStreamConfiguration(sfp, m2110RxAncData.rxAncCh[i].stream, rxConfig) == true)
					{
						//printf("SetRxStreamConfiguration Anc OK\n");
						s2110RxAncDataLast->rxAncCh[i] = m2110RxAncData.rxAncCh[i];
						SetIPError((NTV2Channel)m2110RxAncData.rxAncCh[i].stream, kErrNetworkConfig, NTV2IpErrNone);

						// Process the enable
						if (m2110RxAncData.rxAncCh[i].enable)
						{
							//printf("SetRxAncStream on %d\n", m2110RxAncData.rxAncCh[i].stream);
							config2110->SetRxStreamEnable(sfp, m2110RxAncData.rxAncCh[i].stream, true);
						}
					}
					else
					{
						printf("SetRxStreamConfiguration Anc ERROR %s\n", config2110->getLastError().c_str());
						SetIPError((NTV2Channel)m2110RxAncData.rxAncCh[i].stream, kErrNetworkConfig, config2110->getLastErrorCode());
					}
					AgentIsAlive();
				}
			}
		}

		// See if network needs configuring
		if (memcmp(&m2110Network, s2110NetworkLast, sizeof(NetworkData2110)) != 0 || ipServiceForceConfig)
		{
			*s2110NetworkLast = m2110Network;

			mCard->SetReference(NTV2_REFERENCE_SFP1_PTP);
			config2110->SetPTPDomain(m2110Network.ptpDomain);
			config2110->SetPTPPreferredGrandMasterId(m2110Network.ptpPreferredGMID);
			config2110->SetAudioCombineEnable(m2110Network.audioCombine);

			for (uint32_t i = 0; i < SFP_MAX_NUM_SFPS; i++)
			{
				eSFP sfp = SFP_1;
				if (i > 0)
					sfp = SFP_2;

				bool rv;
				if (m2110Network.sfp[i].enable)
				{
					rv =  config2110->SetNetworkConfiguration(sfp,
															  m2110Network.sfp[i].ipAddress,
															  m2110Network.sfp[i].subnetMask,
															  m2110Network.sfp[i].gateWay);
					if (rv)
					{
						//printf("SetNetworkConfiguration OK\n");
						SetIPError(NTV2_CHANNEL1, kErrNetworkConfig, NTV2IpErrNone);
					}
					else
					{
						printf("SetNetworkConfiguration ERROR %s\n", config2110->getLastError().c_str());
						SetIPError(NTV2_CHANNEL1, kErrNetworkConfig, config2110->getLastErrorCode());
					}
				}
				else
				{
					//printf("DisableNetworkInterface\n");
					config2110->DisableNetworkInterface(sfp);
				}
				AgentIsAlive();
			}
		}

		if (*videoFormatLast != mFb1VideoFormat)
		{
			*videoFormatLast = mFb1VideoFormat;
			mCard->ReadRegister(kRegAud1Counter, endAudioCounter);

			mCard->WriteRegister(kVRegDebugLastFormat, mFb1VideoFormat);
			mCard->WriteRegister(kVRegDebugIPConfigTimeMS, (endAudioCounter-startAudioCounter)/48);
			printf("Format change time = %d ms\n", (endAudioCounter-startAudioCounter)/48);
		}


		// Write status
		mCard->WriteVirtualData(kChStatusData2110, &m2110IpStatusData, sizeof(IpStatus2110));

		// Turn off force config
		config2110->SetIPServicesControl(ipServiceEnable, false);
	}
}


//-------------------------------------------------------------------------------------------------------
//	ConvertVideoToStreamFormat
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat ClassIP2110Services::ConvertVideoToStreamFormat(NTV2VideoFormat videoFormat)
{
	NTV2VideoFormat format;

	switch (videoFormat)
	{
		case NTV2_FORMAT_4x2048x1080p_2398:
		case NTV2_FORMAT_4x2048x1080psf_2398:
			format = NTV2_FORMAT_1080p_2K_2398;
			break;
		case NTV2_FORMAT_4x2048x1080p_2400:
		case NTV2_FORMAT_4x2048x1080psf_2400:
			format = NTV2_FORMAT_1080p_2K_2400;
			break;
		case NTV2_FORMAT_4x2048x1080p_2500:
		case NTV2_FORMAT_4x2048x1080psf_2500:
			format = NTV2_FORMAT_1080p_2K_2500;
			break;
		case NTV2_FORMAT_4x2048x1080p_2997:
		case NTV2_FORMAT_4x2048x1080psf_2997:
			format = NTV2_FORMAT_1080p_2K_2997;
			break;
		case NTV2_FORMAT_4x2048x1080p_3000:
		case NTV2_FORMAT_4x2048x1080psf_3000:
			format = NTV2_FORMAT_1080p_2K_3000;
			break;
		case NTV2_FORMAT_4x2048x1080p_4795:
			format = NTV2_FORMAT_1080p_2K_4795_A;
			break;
		case NTV2_FORMAT_4x2048x1080p_4800:
			format = NTV2_FORMAT_1080p_2K_4800_A;
			break;
		case NTV2_FORMAT_4x2048x1080p_5000:
			format = NTV2_FORMAT_1080p_2K_5000_A;
			break;
		case NTV2_FORMAT_4x2048x1080p_5994:
			format = NTV2_FORMAT_1080p_2K_5994_A;
			break;
		case NTV2_FORMAT_4x2048x1080p_6000:
			format = NTV2_FORMAT_1080p_2K_6000_A;
			break;

		case NTV2_FORMAT_4x1920x1080p_2398:
		case NTV2_FORMAT_4x1920x1080psf_2398:
			format = NTV2_FORMAT_1080p_2398;
			break;
		case NTV2_FORMAT_4x1920x1080p_2400:
		case NTV2_FORMAT_4x1920x1080psf_2400:
			format = NTV2_FORMAT_1080p_2400;
			break;
		case NTV2_FORMAT_4x1920x1080p_2500:
		case NTV2_FORMAT_4x1920x1080psf_2500:
			format = NTV2_FORMAT_1080p_2500;
			break;
		case NTV2_FORMAT_4x1920x1080p_2997:
		case NTV2_FORMAT_4x1920x1080psf_2997:
			format = NTV2_FORMAT_1080p_2997;
			break;
		case NTV2_FORMAT_4x1920x1080p_3000:
		case NTV2_FORMAT_4x1920x1080psf_3000:
			format = NTV2_FORMAT_1080p_3000;
			break;
		case NTV2_FORMAT_4x1920x1080p_5000:
			format = NTV2_FORMAT_1080p_5000_A;
			break;
		case NTV2_FORMAT_4x1920x1080p_5994:
			format = NTV2_FORMAT_1080p_5994_A;
			break;
		case NTV2_FORMAT_4x1920x1080p_6000:
			format = NTV2_FORMAT_1080p_6000_A;
			break;

		default:
			format = videoFormat;
			break;
	}

	return format;
}


//-------------------------------------------------------------------------------------------------------
//	ConvertStreamToVideoFormat
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat ClassIP2110Services::ConvertStreamToVideoFormat(NTV2VideoFormat videoFormat)
{
	NTV2VideoFormat format;

	switch (videoFormat)
	{
		case NTV2_FORMAT_1080p_2K_2398:
			format = NTV2_FORMAT_4x2048x1080p_2398;
			break;
		case NTV2_FORMAT_1080p_2K_2400:
			format = NTV2_FORMAT_4x2048x1080p_2400;
			break;
		case NTV2_FORMAT_1080p_2K_2500:
			format = NTV2_FORMAT_4x2048x1080p_2500;
			break;
		case NTV2_FORMAT_1080p_2K_2997:
			format = NTV2_FORMAT_4x2048x1080p_2997;
			break;
		case NTV2_FORMAT_1080p_2K_3000:
			format = NTV2_FORMAT_4x2048x1080p_3000;
			break;
		case NTV2_FORMAT_1080p_2K_4795_A:
			format = NTV2_FORMAT_4x2048x1080p_4795;
			break;
		case NTV2_FORMAT_1080p_2K_4800_A:
			format = NTV2_FORMAT_4x2048x1080p_4800;
			break;
		case NTV2_FORMAT_1080p_2K_5000_A:
			format = NTV2_FORMAT_4x2048x1080p_5000;
			break;
		case NTV2_FORMAT_1080p_2K_5994_A:
			format = NTV2_FORMAT_4x2048x1080p_5994;
			break;
		case NTV2_FORMAT_1080p_2K_6000_A:
			format = NTV2_FORMAT_4x2048x1080p_6000;
			break;

		case NTV2_FORMAT_1080p_2398:
			format = NTV2_FORMAT_4x1920x1080p_2398;
			break;
		case NTV2_FORMAT_1080p_2400:
			format = NTV2_FORMAT_4x1920x1080p_2400;
			break;
		case NTV2_FORMAT_1080p_2500:
			format = NTV2_FORMAT_4x1920x1080p_2500;
			break;
		case NTV2_FORMAT_1080p_2997:
			format = NTV2_FORMAT_4x1920x1080p_2997;
			break;
		case NTV2_FORMAT_1080p_3000:
			format = NTV2_FORMAT_4x1920x1080p_3000;
			break;
		case NTV2_FORMAT_1080p_5000_A:
			format = NTV2_FORMAT_4x1920x1080p_5000;
			break;
		case NTV2_FORMAT_1080p_5994_A:
			format = NTV2_FORMAT_4x1920x1080p_5994;
			break;
		case NTV2_FORMAT_1080p_6000_A:
			format = NTV2_FORMAT_4x1920x1080p_6000;
			break;

		default:
			format = videoFormat;
			break;
	}

	return format;
}
