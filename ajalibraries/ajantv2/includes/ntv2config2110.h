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

#define IP_STRSIZE						32

enum NTV2PacketInterval
{
    PACKET_INTERVAL_125uS,
    PACKET_INTERVAL_1mS
};

/**
    @brief	Structs and enums that define the virtual config data used by services and the Control panel to set and maintain 2110 configuration
**/

typedef enum
{
    kNetworkVData2110      	= 'n210',           // 4CC of network config virtual data
    kTransmitVData2110     	= 't210',           // 4CC of transmit config virtual data
    kReceiveVData2110     	= 'r210',           // 4CC of receive config virtual data
    kMetadataVData2110     	= 'm210',           // 4CC of metadata config virtual data
} VDataTag2110 ;


typedef struct
{
    char                    remoteIP[IP_STRSIZE][2];
    uint32_t                localPort[2];
    uint32_t                remotePort[2];
    uint16_t                payloadType;
    uint32_t                ssrc;
} TxNetworkChVData2110;

typedef struct
{
    char                    remoteIP[IP_STRSIZE][2];
    uint32_t                localPort[2];
    uint32_t                remotePort[2];
    uint16_t                payloadType;
    uint32_t                ssrc;
    uint8_t                 numAudioChannels;
    uint8_t                 firstAudioChannel;
    NTV2PacketInterval      audioPacketInterval;
} TxAudioChVData2110;

typedef struct
{
    uint32_t                numVideoStreams;
    TxNetworkChVData2110    txVideo[1];
    uint32_t                numAudioStreams;
    TxAudioChVData2110      txAudio[4];
} TxChVData2110;

typedef struct
{
    char                    sourceIP[IP_STRSIZE];
    char                    destIP[IP_STRSIZE];
    uint32_t                sourcePort;
    uint32_t                destPort;
    uint32_t                rxMatch;
    uint32_t                ssrc;
    uint16_t                vlan;
    uint16_t                payloadType;
} RxNetworkChVData2110;

typedef struct
{
    char                    sourceIP[IP_STRSIZE];
    char                    destIP[IP_STRSIZE];
    uint32_t                sourcePort;
    uint32_t                destPort;
    uint32_t                rxMatch;
    uint32_t                ssrc;
    uint16_t                vlan;
    uint16_t                payloadType;
    uint8_t                 numAudioChannels;
    NTV2PacketInterval      audioPacketInterval;
} RxAudioChVData2110;

typedef struct
{
    uint32_t                numVideoStreams;
    RxNetworkChVData2110    rxVideo[1];
    uint32_t                numAudioStreams;
    RxAudioChVData2110      rxAudio[4];	
} RxChVData2110;


typedef struct
{
    char                    ipAddress[IP_STRSIZE];
    char                    subnetMask[IP_STRSIZE];
    char                    gateWay[IP_STRSIZE];
} SFPVData2110;

typedef struct
{
    uint32_t                id;
    char                    ptpMasterIP[IP_STRSIZE];
    uint32_t                numSFPs;
    SFPVData2110            link[2];
} NetworkVData2110;

typedef struct
{
    uint32_t                id;
    uint32_t                numTxChannels;
    TxChVData2110           txCh[4];
} TransmitVData2110; 

typedef struct
{
    uint32_t                id;
    uint32_t                numRxChannels;
    RxChVData2110           rxCh[4];
} ReceiveVData2110;

typedef struct
{
    uint32_t                id;
} MetadataVData2110;



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
    NTV2PacketInterval  audioPacketInterval;
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
    NTV2PacketInterval  audioPacketInterval;
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

    bool        SetNetworkConfiguration(eSFP link, const IPVNetConfig & netConfig);
    bool        SetNetworkConfiguration(eSFP link, std::string localIPAddress, std::string subnetMask, std::string gateway = "");
    bool        DisableNetworkInterface(eSFP link);

    bool        GetNetworkConfiguration(eSFP link, IPVNetConfig & netConfig);
    bool        GetNetworkConfiguration(eSFP link, std::string & localIPAddress, std::string & subnetMask, std::string & gateway);

    bool        EnableRxStream(const eSFP link, const NTV2Channel channel, const NTV2Stream stream, rx_2110Config &rxConfig);
    bool        DisableRxStream(const eSFP link, const NTV2Channel channel, const NTV2Stream stream);
    bool        GetRxStreamConfiguration(const eSFP link, const NTV2Channel channel, NTV2Stream stream, rx_2110Config & rxConfig);
    bool        GetRxStreamEnable(const eSFP link, const NTV2Channel channel, NTV2Stream stream, bool & enabled);
	bool        GetRxByteCount( const NTV2Channel channel, NTV2Stream stream, uint32_t &bytes);

    bool        SetTxChannelConfiguration(const NTV2Channel channel, const NTV2Stream stream, const tx_2110Config & txConfig);
    bool        GetTxChannelConfiguration(const NTV2Channel channel, const NTV2Stream stream, tx_2110Config & txConfig);
    bool        SetTxChannelEnable(const NTV2Channel channel, const NTV2Stream stream, bool enableLinkA, bool enableLinkB = false);
    bool        GetTxChannelEnable(const NTV2Channel channel, const NTV2Stream stream, bool & linkAEnabled, bool & linkBEnabled);

    bool        SetPTPMaster(std::string ptpMaster);
    bool        GetPTPMaster(std::string & ptpMaster);

    std::string GetTxSDP(NTV2Channel chan, NTV2Stream stream);
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
    bool        SetIGMPDisable(eSFP link, bool disable);
    bool        GetIGMPDisable(eSFP link, bool & disabled);

    bool        SetIGMPVersion(eIGMPVersion_t version);
    bool        GetIGMPVersion(eIGMPVersion_t & version);

    void        SetBiDirectionalChannels(bool bidirectional) { _biDirectionalChannels = bidirectional;}
    bool        GetBiDirectionalChannels() {return _biDirectionalChannels;}

    bool        GetMACAddress(eSFP port, NTV2Channel channel, NTV2Stream stream, std::string remoteIP, uint32_t & hi, uint32_t & lo);

    bool        GetSFPMSAData(eSFP port, SFPMSAData & data);
    bool        GetLinkStatus(eSFP port, sLinkStatus & linkStatus);

    static uint32_t  get2110TxStream(NTV2Channel ch, NTV2Stream str );
    static bool      decompose2110TxVideoStream(uint32_t istream, NTV2Channel & ch, NTV2Stream & str);
    static bool      decompose2110TxAudioStream(uint32_t istream, NTV2Channel & ch, NTV2Stream & str);
    static uint32_t  GetDecapsulatorAddress(eSFP link, NTV2Channel channel, NTV2Stream stream);

    // If method returns false call this to get details
    std::string getLastError();
    NTV2IpError getLastErrorCode();

    static uint32_t v_packetizers[4];
    static uint32_t a_packetizers[16];
    static uint32_t m_packetizers[4];

protected:
    uint32_t    GetFramerAddress(eSFP link, NTV2Channel channel, NTV2Stream stream);
    void        SelectTxFramerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t baseAddr);
    void        AcquireFramerControlAccess(uint32_t baseAddr);
    void        ReleaseFramerControlAccess(uint32_t baseAddr);

    void        EnableFramerStream(const eSFP link, const NTV2Channel channel, const NTV2Stream stream, bool enable);
    bool        SetFramerStream(const eSFP link, const NTV2Channel channel, const NTV2Stream stream, const tx_2110Config & txConfig);
    void        GetFramerStream(NTV2Channel channel, NTV2Stream stream,eSFP link, tx_2110Config  & txConfig);
    void        SetArbiter(const eSFP link, const NTV2Channel channel, const NTV2Stream stream, bool enable);
    void        GetArbiter(const eSFP link, const NTV2Channel channel, const NTV2Stream stream, bool & enable);

    void        DisableDepacketizerStream(NTV2Channel channel, NTV2Stream stream);
    void        EnableDepacketizerStream(NTV2Channel channel, NTV2Stream stream);
    void        DisableDecapsulatorStream(eSFP link, NTV2Channel channel, NTV2Stream stream);
    void        EnableDecapsulatorStream(eSFP link, NTV2Channel channel, NTV2Stream stream);

    void        SetupDecapsulatorStream(eSFP link, NTV2Channel channel, NTV2Stream stream, rx_2110Config &rxConfig);

    void        SetupDepacketizerStream(const NTV2Channel channel, NTV2Stream stream, const rx_2110Config & rxConfig);
    void        ResetDepacketizerStream(const NTV2Channel channel, NTV2Stream stream);
    uint32_t    GetDepacketizerAddress(NTV2Channel channel, NTV2Stream stream);
    bool        SetTxPacketizerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t  & baseAddr);

    bool		ConfigurePTP(eSFP link, std::string localIPAddress);

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
