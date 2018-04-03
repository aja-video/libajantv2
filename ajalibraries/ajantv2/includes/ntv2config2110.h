/**
    @file		ntv2config2110.h
    @brief		Declares the CNTV2Config2110 class.
    @copyright	(C) 2014-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2_2110CONFIG_H
#define NTV2_2110CONFIG_H

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2registers2110.h"
#include "ntv2mbcontroller.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

/**
    @brief	Configures a SMPTE 2110 Transmit Channel.
**/

class tx_2110Config
{
public:
    tx_2110Config() { init(); }

    void init();

    bool operator != ( const tx_2110Config &other );
    bool operator == ( const tx_2110Config &other );

public:
    std::string         remoteIP[2];        ///< @brief	Specifies remote (destination) IP address.
    uint32_t            localPort[2];		///< @brief	Specifies the local (source) port number.
    uint32_t            remotePort[2];		///< @brief	Specifies the remote (destination) port number.
    uint16_t            payloadType;
    uint8_t             tos;                // type of service
    uint8_t             ttl;                // time to live
    uint32_t            ssrc;
    NTV2VideoFormat     videoFormat;
    VPIDSampling        videoSamples;
    uint32_t            pktsPerLine;        // read-only
    uint32_t            payloadLen;         // read-only
    uint32_t            lastPayLoadLen;     // read-only
    uint8_t             numAudioChannels;
    uint8_t             firstAudioChannel;
    eNTV2PacketInterval audioPacketInterval;
};

/**
    @brief	Configures a SMPTE 2110 Receive Channel.
**/

class rx_2110Config
{
public:
    rx_2110Config() { init(); }

    void init();

    bool operator != ( const rx_2110Config &other );
    bool operator == ( const rx_2110Config &other );

public:
    uint32_t            rxMatch;            ///< @brief	Bitmap of rxMatch criteria used
    std::string         sourceIP;           ///< @brief	Specifies the source (sender) IP address (if RX_MATCH_2110_SOURCE_IP set). If it's in the multiclass range, then
                                            ///			by default, the IGMP multicast group will be joined (see CNTV2Config2110::SetIGMPDisable).
    std::string         destIP;             ///< @brief	Specifies the destination (target) IP address (if RX_MATCH_2110_DEST_IP set)
    uint32_t            sourcePort;         ///< @brief	Specifies the source (sender) port number (if RX_MATCH_2110_SOURCE_PORT set)
    uint32_t            destPort;           ///< @brief	Specifies the destination (target) port number (if RX_MATCH_2110_DEST_PORT set)
    uint32_t            SSRC;               ///< @brief	Specifies the SSRC identifier (if RX_MATCH_2110_SSRC set)
    uint16_t            VLAN;               ///< @brief	Specifies the VLAN TCI (if RX_MATCH_2110_VLAN set)
    uint16_t            payloadType;
    NTV2VideoFormat     videoFormat;
    VPIDSampling        videoSamples;
    uint32_t            payloadLen;
    uint32_t            lastPayloadLen;
    uint32_t            pktsPerLine;
    uint32_t            numAudioChannels;
    eNTV2PacketInterval audioPacketInterval;
};


/**
    @brief	The CNTV2Config2110 class is the interface to Kona-IP network I/O using SMPTE 2110
**/

class AJAExport CNTV2Config2110 : public CNTV2MBController
{
    friend class CKonaIpJsonSetup;
public:
    CNTV2Config2110 (CNTV2Card & device);
    ~CNTV2Config2110();

    bool        SetNetworkConfiguration(const eSFP sfp, const IPVNetConfig & netConfig);
    bool        GetNetworkConfiguration(const eSFP sfp, IPVNetConfig & netConfig);
    bool        SetNetworkConfiguration(const eSFP sfp, const std::string localIPAddress, const std::string subnetMask, const std::string gateway);
    bool        GetNetworkConfiguration(const eSFP sfp, std::string & localIPAddress, std::string & subnetMask, std::string & gateway);
    bool        DisableNetworkInterface(const eSFP sfp);

    bool        EnableRxStream(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, rx_2110Config &rxConfig);
    bool        DisableRxStream(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream);
    bool        GetRxStreamConfiguration(const eSFP sfp, const NTV2Channel channel, NTV2Stream stream, rx_2110Config & rxConfig);
    bool        GetRxStreamEnable(const eSFP sfp, const NTV2Channel channel, NTV2Stream stream, bool & enabled);
    bool        GetRxPacketCount(const NTV2Channel channel, NTV2Stream stream, uint32_t &packets);
    bool        GetRxByteCount(const NTV2Channel channel, NTV2Stream stream, uint32_t &bytes);
    bool        GetRxByteCount(const eSFP sfp, uint64_t &bytes);

    bool        SetTxStreamConfiguration(const NTV2Channel channel, const NTV2Stream stream, const tx_2110Config & txConfig);
    bool        GetTxStreamConfiguration(const NTV2Channel channel, const NTV2Stream stream, tx_2110Config & txConfig);
    bool        SetTxStreamEnable(const NTV2Channel channel, const NTV2Stream stream, bool enableSfp1, bool enableSfp2 = false);
    bool        GetTxStreamEnable(const NTV2Channel channel, const NTV2Stream stream, bool & sfp1Enabled, bool & sfp2Enabled);
    bool        GetTxPacketCount(const NTV2Channel channel, NTV2Stream stream, uint32_t &packets);
    bool        GetTxByteCount(const eSFP sfp, uint64_t &bytes);

    bool        SetPTPMaster(const std::string ptpMaster);
    bool        GetPTPMaster(std::string & ptpMaster);
    bool        GetPTPStatus(PTPStatus & ptpStatus);

    bool        Set4KMode(bool enable);
    bool        Get4KMode(bool & enable);

    std::string GetTxSDPUrl(eSFP sfp, NTV2Channel chan, NTV2Stream stream);
    std::string GetTxSDP(NTV2Channel chan, NTV2Stream stream);
    bool        GetRxSDP(std::string url, std::string & sdp);
    bool        ExtractRxConfigFromSDP(std::string sdp, NTV2Stream stream, rx_2110Config & rxConfig);

    /**
        @brief		Disables the automatic (default) joining of multicast groups using IGMP, based on remote IP address for Rx Channels
        @param[in]	sfp                 Specifies SFP connector used.
        @param[in]	disable             If true, IGMP messages will not be sent to join multicast groups
        @note       When Rx channels are enabled for multicast IP addresses, by default the multicast group is joined. When Rx Channels
                    are disabled, if the channel is the last user of the group, then the subscription to the multicast group will be ended.
                    When IGMP is disabled, the above actions are not performed,
    **/
    bool        SetIGMPDisable(eSFP sfp, bool disable);
    bool        GetIGMPDisable(eSFP sfp, bool & disabled);

    bool        SetIGMPVersion(eIGMPVersion_t version);
    bool        GetIGMPVersion(eIGMPVersion_t & version);

    void        SetBiDirectionalChannels(bool bidirectional) { _biDirectionalChannels = bidirectional;}
    bool        GetBiDirectionalChannels() {return _biDirectionalChannels;}

    bool        GetMACAddress(eSFP port, NTV2Channel channel, NTV2Stream stream, std::string remoteIP, uint32_t & hi, uint32_t & lo);

    bool        GetSFPMSAData(eSFP port, SFPMSAData & data);
    bool        GetLinkStatus(eSFP port, SFPStatus & sfpStatus);

    static uint32_t  get2110TxStream(NTV2Channel ch, NTV2Stream str );
    static bool      decompose2110TxVideoStream(uint32_t istream, NTV2Channel & ch, NTV2Stream & str);
    static bool      decompose2110TxAudioStream(uint32_t istream, NTV2Channel & ch, NTV2Stream & str);
    static uint32_t  GetDecapsulatorAddress(eSFP sfp, NTV2Channel channel, NTV2Stream stream);

    // If method returns false call this to get details
    std::string getLastError();
    NTV2IpError getLastErrorCode();

    static uint32_t v_packetizers[4];
    static uint32_t a_packetizers[16];
    static uint32_t m_packetizers[4];

protected:
    uint32_t    GetFramerAddress(eSFP sfp, NTV2Channel channel, NTV2Stream stream);
    void        SelectTxFramerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t baseAddr);
    void        AcquireFramerControlAccess(uint32_t baseAddr);
    void        ReleaseFramerControlAccess(uint32_t baseAddr);

    void        EnableFramerStream(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, bool enable);
    bool        SetFramerStream(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, const tx_2110Config & txConfig);
    void        GetFramerStream(NTV2Channel channel, NTV2Stream stream,eSFP sfp, tx_2110Config  & txConfig);
    void        SetArbiter(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, bool enable);
    void        GetArbiter(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, bool & enable);

    void        DisableDepacketizerStream(NTV2Channel channel, NTV2Stream stream);
    void        EnableDepacketizerStream(NTV2Channel channel, NTV2Stream stream);
    void        DisableDecapsulatorStream(eSFP sfp, NTV2Channel channel, NTV2Stream stream);
    void        EnableDecapsulatorStream(eSFP sfp, NTV2Channel channel, NTV2Stream stream);

    void        SetupDecapsulatorStream(eSFP sfp, NTV2Channel channel, NTV2Stream stream, rx_2110Config &rxConfig);

    void        ResetPacketizerStream(const NTV2Channel channel, NTV2Stream stream);

    void        SetupDepacketizerStream(const NTV2Channel channel, NTV2Stream stream, const rx_2110Config & rxConfig);
    void        ResetDepacketizerStream(const NTV2Channel channel, NTV2Stream stream);
    uint32_t    GetDepacketizerAddress(NTV2Channel channel, NTV2Stream stream);
    bool        SetTxPacketizerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t  & baseAddr);

    void        SetVideoFormatForRxTx(const NTV2Channel channel, const NTV2VideoFormat format, const bool rx);
    void        GetVideoFormatForRxTx(const NTV2Channel channel, NTV2VideoFormat & format, uint32_t & hwFormat, const bool rx);

    bool		ConfigurePTP(eSFP sfp, std::string localIPAddress);

    bool        GenSDP(NTV2Channel channel, NTV2Stream stream);
    bool        GenSDPVideoStream(std::stringstream & sdp, NTV2Channel channel, std::string gmInfo);
    bool        GenSDPAudioStream(std::stringstream & sdp, NTV2Channel channel, NTV2Stream stream, std::string gmInfo);

private:
    std::string To_String(int val);

    std::vector<std::string> split(const char *str, char delim);

    std::string     rateToString(NTV2FrameRate rate);
    NTV2FrameRate   stringToRate(std::string str);

    int         LeastCommonMultiple(int a,int b);
    int         getDescriptionValue(int startLine, std::string type, std::string & value);
    std::string getVideoDescriptionValue(std::string type);

    std::stringstream txsdp[4][5];  // indexed by channel and stream

    uint32_t    _numRx0Chans;
    uint32_t    _numRx1Chans;
    uint32_t    _numTx0Chans;
    uint32_t    _numTx1Chans;
    uint32_t    _numRxChans;
    uint32_t    _numTxChans;
    bool        _biDirectionalChannels;             // logically bi-directional channels

    std::vector<std::string> sdpLines;
    std::vector<std::string> tokens;

};	//	CNTV2Config2110

#endif // NTV2_2110CONFIG_H
