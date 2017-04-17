/**
    @file		ntv2config2022.h
    @brief		Declares the CNTV2Config2022 class.
	@copyright	(C) 2014-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2_2022CONFIG_H
#define NTV2_2022CONFIG_H

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2registers2022.h"
#include "ntv2mbcontroller.h"
#include "ntv2tshelper.h"
#include <string.h>

#define RX_MATCH_VLAN                   BIT(0)
#define RX_MATCH_SOURCE_IP              BIT(1)
#define RX_MATCH_DEST_IP                BIT(2)
#define RX_MATCH_SOURCE_PORT            BIT(3)
#define RX_MATCH_DEST_PORT              BIT(4)
#define RX_MATCH_SSRC                   BIT(5)

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


/**
    @brief	Configures a SMPTE 2022 Transmit Channel.
**/

class tx_2022_channel
{
public:
    tx_2022_channel() { init(); }

    void init();

    bool eq_MACAddr(const MACAddr& a);
    
    bool operator != ( const tx_2022_channel &other );
    bool operator == ( const tx_2022_channel &other );
    
public:
    uint32_t	primaryLocalPort;		///< @brief	Specifies the local (source) port number.
    std::string	primaryRemoteIP;        ///< @brief	Specifies remote (destination) IP address.
    uint32_t	primaryRemotePort;		///< @brief	Specifies the remote (destination) port number.
    bool		primaryAutoMAC;         ///< @brief	If true, MAC address is generated for multicast remoteIP address, or fetched from ARP table
    MACAddr		primaryRemoteMAC;		///< @brief	Specifies the MAC address of the remote (target) device. Ignored if autoMAC is true.

    uint32_t	secondaryLocalPort;		///< @brief	Specifies the local (source) port number.
    std::string	secondaryRemoteIP;      ///< @brief	Specifies remote (destination) IP address.
    uint32_t	secondaryRemotePort;	///< @brief	Specifies the remote (destination) port number.
    bool		secondaryAutoMAC;       ///< @brief	If true, MAC address is generated for multicast remoteIP address, or fetched from ARP table
    MACAddr		secondaryRemoteMAC;		///< @brief	Specifies the MAC address of the remote (target) device. Ignored if autoMAC is true.
};

/**
    @brief	Configures a SMPTE 2022 Receive Channel.
**/

class rx_2022_channel
{
public:
    rx_2022_channel() { init(); }

    void init();

    bool operator != ( const rx_2022_channel &other );
    bool operator == ( const rx_2022_channel &other );
    
public:
    uint32_t	primaryRxMatch;         ///< @brief	Bitmap of rxMatch criteria used
    std::string	primarySourceIP;		///< @brief	Specifies the source (sender) IP address (if RX_MATCH_SOURCE_IP set). If it's in the multiclass range, then
                                        ///			by default, the IGMP multicast group will be joined (see CNTV2Config2022::SetIGMPDisable).
    std::string	primaryDestIP;			///< @brief	Specifies the destination (target) IP address (if RX_MATCH_DEST_IP set)
    uint32_t	primarySourcePort;		///< @brief	Specifies the source (sender) port number (if RX_MATCH_SOURCE_PORT set)
    uint32_t	primaryDestPort;		///< @brief	Specifies the destination (target) port number (if RX_MATCH_DEST_PORT set)
    uint32_t	primarySsrc;            ///< @brief	Specifies the SSRC identifier (if RX_MATCH_SSRC set)
    uint16_t	primaryVlan;            ///< @brief	Specifies the VLAN TCI (if RX_MATCH_VLAN set)

    uint32_t	secondaryRxMatch;       ///< @brief	Bitmap of rxMatch criteria used
    std::string	secondarySourceIP;		///< @brief	Specifies the source (sender) IP address (if RX_MATCH_SOURCE_IP set). If it's in the multiclass range, then
                                        ///			by default, the IGMP multicast group will be joined (see CNTV2Config2022::SetIGMPDisable).
    std::string	secondaryDestIP;        ///< @brief	Specifies the destination (target) IP address (if RX_MATCH_DEST_IP set)
    uint32_t	secondarySourcePort;	///< @brief	Specifies the source (sender) port number (if RX_MATCH_SOURCE_PORT set)
    uint32_t	secondaryDestPort;		///< @brief	Specifies the destination (target) port number (if RX_MATCH_DEST_PORT set)
    uint32_t	secondarySsrc;          ///< @brief	Specifies the SSRC identifier (if RX_MATCH_SSRC set)
    uint16_t	secondaryVlan;          ///< @brief	Specifies the VLAN TCI (if RX_MATCH_VLAN set)

    uint32_t	networkPathDiff;        ///< @brief	Specifies the max accepted delay in milliseconds between 2 steams in hitless operation (0-150).
    uint32_t	playoutDelay;           ///< @brief	Specifies the wait time in milliseconds to SDI playout from incoming packet (0-150).
};


// These structs are used internally be retail services

class rx2022Config
{
public:

    rx2022Config() { init(); }

    void init();

    bool operator == ( const rx2022Config &other );
    bool operator != ( const rx2022Config &other );

    bool        rxc_enable;
    
    uint32_t    rxc_primaryRxMatch;
    uint32_t    rxc_primarySourceIp;
    uint32_t    rxc_primaryDestIp;
    uint32_t    rxc_primarySourcePort;
    uint32_t    rxc_primaryDestPort;
    uint32_t    rxc_primarySsrc;
    uint32_t    rxc_primaryVlan;
    
    uint32_t    rxc_secondaryRxMatch;
    uint32_t    rxc_secondarySourceIp;
    uint32_t    rxc_secondaryDestIp;
    uint32_t    rxc_secondarySourcePort;
    uint32_t    rxc_secondaryDestPort;
    uint32_t    rxc_secondarySsrc;
    uint32_t    rxc_secondaryVlan;

    uint32_t	rxc_networkPathDiff;
    uint32_t	rxc_playoutDelay;
 };

class tx2022Config
{
public:

    tx2022Config() { init(); }

    void init();

    bool operator == ( const tx2022Config &other );
    bool operator != ( const tx2022Config &other );

    bool        txc_enable;
    
    uint32_t    txc_primaryLocalPort;
    uint32_t    txc_primaryRemoteIp;
    uint32_t    txc_primaryRemotePort;
    uint32_t    txc_primaryRemoteMAC_lo;
    uint32_t    txc_primaryRemoteMAC_hi;
    bool        txc_primaryAutoMac;
    
    uint32_t    txc_secondaryLocalPort;
    uint32_t    txc_secondaryRemoteIp;
    uint32_t    txc_secondaryRemotePort;
    uint32_t    txc_secondaryRemoteMAC_lo;
    uint32_t    txc_secondaryRemoteMAC_hi;
    bool        txc_secondaryAutoMac;
};

class j2kEncoderConfig
{
public:
    j2kEncoderConfig() { init(); }

    void init();

    bool operator == ( const j2kEncoderConfig &other );
    bool operator != ( const j2kEncoderConfig &other );

    NTV2VideoFormat         videoFormat;        ///< @brief	Specifies the video format for J2K encode.
    uint32_t                ullMode;            ///< @brief	Specifies the ull mode for J2K encode.
    uint32_t                bitDepth;           ///< @brief	Specifies the bit depth for J2K encode.
    J2KChromaSubSampling    chromaSubsamp;      ///< @brief	Specifies the chroma sub sampling for J2K encode.
    uint32_t                mbps;               ///< @brief	Specifies the mbits per-second for J2K encode.
    J2KStreamType           streamType;         ///< @brief	Specifies the stream type for J2K encode.
    uint32_t                audioChannels;      ///< @brief	Specifies the number of audio channels for J2K encode, a value of 0 indicates no audio.
    uint32_t                pmtPid;             ///< @brief	Specifies the PID for the PMT.
    uint32_t                videoPid;           ///< @brief	Specifies the PID for the video.
    uint32_t                pcrPid;             ///< @brief	Specifies the PID for the PCR.
    uint32_t                audio1Pid;          ///< @brief	Specifies the PID for audio 1.
};

typedef enum
{
    eProgSel_Off,
    eProgSel_AutoFirstProg,
    eProgSel_LowestProgNum,
    eProgSel_SpecificProgNum,
    eProgSel_SpecificProgPID,
    eProgSel_Default = eProgSel_AutoFirstProg,
} eProgSelMode_t;

class j2kDecoderConfig
{
public:
    j2kDecoderConfig() {init();}
    void init();

    bool operator == ( const j2kDecoderConfig &other );
    bool operator != ( const j2kDecoderConfig &other );

    eProgSelMode_t  selectionMode;
    uint32_t        programNumber;
    uint32_t        programPID;
    uint32_t        audioNumber;
};

class j2kDecoderStatus
{
public:
    j2kDecoderStatus() {init();}
    void init();

    uint32_t              numAvailablePrograms;
    uint32_t              numAvailableAudios;
    std::vector<uint32_t> availableProgramNumbers;
    std::vector<uint32_t> availableProgramPIDs;
    std::vector<uint32_t> availableAudioPIDs;
};

/**
    @brief	The CNTV2Config2022 class is the interface to Kona-IP network I/O using SMPTE 2022
**/

class AJAExport CNTV2Config2022 : public CNTV2MBController
{
public:
    CNTV2Config2022 (CNTV2Card & device);
    ~CNTV2Config2022();

    bool        SetNetworkConfiguration(eSFP port, const IPVNetConfig & netConfig);
    bool        SetNetworkConfiguration(eSFP port, std::string localIPAddress, std::string subnetMask, std::string gateway = "");
    bool        SetNetworkConfiguration(std::string localIPAddress0, std::string subnetMask0, std::string gateway0,
                                        std::string localIPAddress1, std::string subnetMask1, std::string gateway1);

    bool        GetNetworkConfiguration(eSFP port, IPVNetConfig & netConfig);
    bool        GetNetworkConfiguration(eSFP port, std::string & localIPAddress, std::string & subnetMask, std::string & gateway);
    bool        GetNetworkConfiguration(std::string & localIPAddress0, std::string & subnetMask0, std::string & gateway0,
                                        std::string & localIPAddress1, std::string & subnetMask1, std::string & gateway1);

    bool        SetRxChannelConfiguration(const NTV2Channel channel, const rx_2022_channel & rxConfig);
    bool        GetRxChannelConfiguration(const NTV2Channel channel, rx_2022_channel & rxConfig);

    bool        SetRxChannelEnable(const NTV2Channel channel, bool enable, bool enable2022_7);
    bool        GetRxChannelEnable(const NTV2Channel channel, bool & enabled);

    bool        SetTxChannelConfiguration(const NTV2Channel channel, const tx_2022_channel & txConfig);
    bool        GetTxChannelConfiguration(const NTV2Channel channel, tx_2022_channel & txConfig);

    bool        SetTxChannelEnable(const NTV2Channel channel, bool enable, bool enable2022_7);
    bool        GetTxChannelEnable(const NTV2Channel channel, bool & enabled);

    bool        SetJ2KEncoderConfiguration(const NTV2Channel channel, const j2kEncoderConfig & j2kConfig);
    bool        GetJ2KEncoderConfiguration(const NTV2Channel channel, j2kEncoderConfig &j2kConfig);

    bool        SetJ2KDecoderConfiguration(const j2kDecoderConfig & j2kConfig);
    bool        GetJ2KDecoderConfiguration(j2kDecoderConfig &j2kConfig);
    bool        GetJ2KDecoderStatus(j2kDecoderStatus & j2kStatus);

    bool        SetPTPMaster(std::string ptpMaster);
    bool        GetPTPMaster(std::string & ptpMaster);

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

    bool        SelectRxChannel(NTV2Channel channel, bool primaryChannel, uint32_t & baseAddr);
    bool        SelectTxChannel(NTV2Channel channel, bool primaryChannel, uint32_t & baseAddr);

	// If method returns false call this to get details
    std::string getLastError();
	
private:
    void        ChannelSemaphoreSet(uint32_t controlReg, uint32_t baseaddr);
    void        ChannelSemaphoreClear(uint32_t controlReg, uint32_t baseaddr);
    bool		ConfigurePTP(eSFP port, std::string localIPAddress);

    class CNTV2ConfigTs2022 * _tstreamConfig;

    eSFP        GetRxPort(NTV2Channel chan);
    eSFP        GetTxPort(NTV2Channel chan);

    uint32_t    _numRx0Chans;
    uint32_t    _numRx1Chans;
    uint32_t    _numTx0Chans;
    uint32_t    _numTx1Chans;
    uint32_t    _numRxChans;
    uint32_t    _numTxChans;
    bool        _is2022_6;
    bool        _is2022_2;
    bool        _is2022_7;
    bool        _biDirectionalChannels;             // logically bi-directional channels
    bool        _is_txTop34;
    bool        _hasPTP;

};	//	CNTV2Config2022

#endif // NTV2_2022CONFIG_H
