/**
    @file		ntv2config2110.h
    @brief		Declares the CNTV2Config2110 class.
    @copyright	(C) 2014-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
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

#define RX_MATCH_2110_VLAN                   BIT(0)
#define RX_MATCH_2110_SOURCE_IP              BIT(1)
#define RX_MATCH_2110_DEST_IP                BIT(2)
#define RX_MATCH_2110_SOURCE_PORT            BIT(3)
#define RX_MATCH_2110_DEST_PORT              BIT(4)
#define RX_MATCH_2110_PAYLOAD                BIT(5)
#define RX_MATCH_2110_SSRC                   BIT(6)

#define VOIP_SEMAPHORE_SET              0x2
#define VOIP_SEMAPHORE_CLEAR            0xFFFFFFFD
#define VOIP_PRIMARY_ENABLE             0x7FFFFFFF
#define VOIP_SECONDARY_ENABLE           0x80000000

#define PLL_MATCH_SOURCE_IP             BIT(0)
#define PLL_MATCH_DEST_IP               BIT(1)
#define PLL_MATCH_SOURCE_PORT           BIT(2)
#define PLL_MATCH_DEST_PORT             BIT(3)
#define PLL_MATCH_ES_PID                BIT(4)

#define PLL_CONFIG_PCR                  BIT(0)
#define PLL_CONFIG_PTP                  BIT(1)
#define PLL_CONFIG_DCO_MODE             BIT(28)

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
    std::string	remoteIP;        ///< @brief	Specifies remote (destination) IP address.
    uint32_t	localPort;		 ///< @brief	Specifies the local (source) port number.
    uint32_t	remotePort;		 ///< @brief	Specifies the remote (destination) port number.
    uint16_t    payloadType;
    uint8_t     tos;            // type of service
    uint8_t     ttl;            // time to live
    uint32_t    ssrc;
    NTV2VideoFormat videoFormat;
    VPIDSampling videoSamples;
    uint32_t     pktsPerLine;    // read-only
    uint32_t     payloadLen;     // read0only
    uint32_t     lastPayLoadLen; // read-only
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
    uint32_t	rxMatch;         ///< @brief	Bitmap of rxMatch criteria used
    std::string	sourceIP;		///< @brief	Specifies the source (sender) IP address (if RX_MATCH_2110_SOURCE_IP set). If it's in the multiclass range, then
                                 ///			by default, the IGMP multicast group will be joined (see CNTV2Config2110::SetIGMPDisable).
    std::string	destIP;			///< @brief	Specifies the destination (target) IP address (if RX_MATCH_2110_DEST_IP set)
    uint32_t	sourcePort;		///< @brief	Specifies the source (sender) port number (if RX_MATCH_2110_SOURCE_PORT set)
    uint32_t	destPort;		///< @brief	Specifies the destination (target) port number (if RX_MATCH_2110_DEST_PORT set)
    uint32_t	SSRC;            ///< @brief	Specifies the SSRC identifier (if RX_MATCH_2110_SSRC set)
    uint16_t	VLAN;            ///< @brief	Specifies the VLAN TCI (if RX_MATCH_2110_VLAN set)
    uint16_t    payloadType;
    NTV2VideoFormat videoFormat;
    VPIDSampling    videoSamples;
    uint32_t    payloadLen;
    uint32_t    lastPayloadLen;
    uint32_t    pktsPerLine;
    uint32_t    audioChannels;
    uint32_t    audioSamplesPerPkt;
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

    bool        SetNetworkConfiguration(eSFP port, const IPVNetConfig & netConfig);
    bool        SetNetworkConfiguration(eSFP port, std::string localIPAddress, std::string subnetMask, std::string gateway = "");
    bool        SetNetworkConfiguration(std::string localIPAddress0, std::string subnetMask0, std::string gateway0,
                                        std::string localIPAddress1, std::string subnetMask1, std::string gateway1);
    bool        DisableNetworkInterface(eSFP port);

    bool        GetNetworkConfiguration(eSFP port, IPVNetConfig & netConfig);
    bool        GetNetworkConfiguration(eSFP port, std::string & localIPAddress, std::string & subnetMask, std::string & gateway);
    bool        GetNetworkConfiguration(std::string & localIPAddress0, std::string & subnetMask0, std::string & gateway0,
                                        std::string & localIPAddress1, std::string & subnetMask1, std::string & gateway1);

    bool        EnableRxStream(const NTV2Channel channel, const NTV2Stream stream, rx_2110Config &rxConfig);
    bool        DisableRxStream(const NTV2Channel channel, const NTV2Stream stream);
    bool        GetRxStreamConfiguration(const NTV2Channel channel, NTV2Stream stream, rx_2110Config & rxConfig);
    bool        GetRxStreamEnable(const NTV2Channel channel, NTV2Stream stream, bool & enabled);

    bool        SetTxChannelConfiguration(const NTV2Channel channel, NTV2Stream stream, const tx_2110Config & txConfig);
    bool        GetTxChannelConfiguration(const NTV2Channel channel, NTV2Stream stream, tx_2110Config & txConfig);
    bool        SetTxChannelEnable(const NTV2Channel channel, NTV2Stream stream, bool enable);
    bool        GetTxChannelEnable(const NTV2Channel channel, NTV2Stream stream, bool & enabled);

    bool        SetPTPMaster(std::string ptpMaster);
    bool        GetPTPMaster(std::string & ptpMaster);

    std::string GetTxSDP(NTV2Channel chan);
    bool        GetRxSDP(std::string url, std::string & sdp);
    bool        ExtractRxConfigFromSDP(std::string sdp, NTV2Stream stream, rx_2110Config & rxConfig);

    /**
        @brief		Disables the automatic (default) joining of multicast groups using IGMP, based on remote IP address for Rx Channels
        @param[in]	port                Specifies SFP connector used.
        @param[in]	disable             If true, IGMP messages will not be sent to join multicast groups
        @note       When Rx channels are enabled for multicast IP addresses, by default the multicast group is joined. When Rx Channels
                    are disabled, if the channel is the last user of the group, then the subscription to the multicast group will be ended.
                    When IGMP is disabled, the above actions are not performed,
    **/
    bool        SetIGMPDisable(eSFP port, bool disable);
    bool        GetIGMPDisable(eSFP port, bool & disabled);

    bool        SetIGMPVersion(eIGMPVersion_t version);
    bool        GetIGMPVersion(eIGMPVersion_t & version);

    void        SetBiDirectionalChannels(bool bidirectional) { _biDirectionalChannels = bidirectional;}
    bool        GetBiDirectionalChannels() {return _biDirectionalChannels;}

    bool        GetMACAddress(eSFP port, NTV2Channel channel, NTV2Stream stream, std::string remoteIP, uint32_t & hi, uint32_t & lo);

    bool        GetSFPMSAData(eSFP port, SFPMSAData & data);
    bool        GetLinkStatus(eSFP port, sLinkStatus & linkStatus);

    static uint32_t  get2110TxStream(NTV2Channel ch, NTV2Stream str );
    static bool      decompose2110TxStream(uint32_t istream, NTV2Channel & ch, NTV2Stream & str);
    static uint32_t  GetDecapsulatorAddress(NTV2Channel channel, NTV2Stream stream);

    // If method returns false call this to get details
    std::string getLastError();
    NTV2IpError getLastErrorCode();

    static uint32_t v_packetizers[4];
    static uint32_t a_packetizers[4];
    static uint32_t m_packetizers[4];

protected:
    uint32_t    GetFramerAddress(NTV2Channel channel, NTV2Stream stream);
    void        SelectTxFramerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t baseAddr);
    void        AcquireFramerControlAccess(uint32_t baseAddr);
    void        ReleaseFramerControlAccess(uint32_t baseAddr);

    void        EnableDecapsulatorStream(NTV2Channel channel, NTV2Stream stream);
    void        DisableDecapsulatorStream(NTV2Channel channel, NTV2Stream stream);
    void        SetupDecapsulatorStream(NTV2Channel channel, NTV2Stream stream, rx_2110Config &rxConfig);

    void        SetupDepacketizer(const NTV2Channel channel, NTV2Stream stream, const rx_2110Config & rxConfig);
    void        ResetDepacketizer(const NTV2Channel channel, NTV2Stream stream);
    uint32_t    GetDepacketizerAddress(NTV2Channel channel, NTV2Stream stream);
    bool        SetTxPacketizerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t  & baseAddr);

    bool		ConfigurePTP(eSFP port, std::string localIPAddress);

    bool        GenSDP(NTV2Channel channel);
    bool        GenSDPVideoStream(std::stringstream & sdp, NTV2Channel channel, std::string gmInfo);
    bool        GenSDPAudioStream(std::stringstream & sdp, NTV2Channel channel, std::string gmInfo);

private:
    eSFP        GetRxPort(NTV2Channel chan);
    eSFP        GetTxPort(NTV2Channel chan);
    std::string To_String(int val);

    std::vector<std::string> split(const char *str, char delim);

    std::string     rateToString(NTV2FrameRate rate);
    NTV2FrameRate   stringToRate(std::string str);

    int         LeastCommonMultiple(int a,int b);
    int         getDescriptionValue(int startLine, std::string type, std::string & value);
    std::string getVideoDescriptionValue(std::string type);

    std::stringstream txsdp[4]; // one SDP per channel

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
