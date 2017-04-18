/**
    @file		ntv2config2022.cpp
    @brief		Implements the CNTV2Config2022 class.
	@copyright	(C) 2014-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2config2022.h"
#include "ntv2configts2022.h"
#include "ntv2endian.h"
#include "ntv2card.h"
#include <sstream>

#if defined (AJALinux) || defined (AJAMac)
	#include <stdlib.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

using namespace std;

void tx_2022_channel::init()
{
    primaryLocalPort    = 0;
    primaryRemoteIP.erase();
    primaryRemotePort   = 0;
    primaryAutoMAC      = false;
    memset(primaryRemoteMAC.mac, 0, sizeof(MACAddr));
    secondaryLocalPort  = 0;
    secondaryRemoteIP.erase();
    secondaryRemotePort = 0;
    secondaryAutoMAC    = false;
    memset(secondaryRemoteMAC.mac, 0, sizeof(MACAddr));
}

bool tx_2022_channel::eq_MACAddr(const MACAddr& a)
{
    return (memcmp(primaryRemoteMAC.mac, a.mac, 6) == 0);
}

bool tx_2022_channel::operator != ( const tx_2022_channel &other )
{
    return !(*this == other);
}

bool tx_2022_channel::operator == ( const tx_2022_channel &other )
{
    if ((primaryLocalPort       == other.primaryLocalPort)      &&
        (primaryRemoteIP        == other.primaryRemoteIP)       &&
        (primaryRemotePort      == other.primaryRemotePort)     &&
        (primaryAutoMAC         == other.primaryAutoMAC)        &&
        (eq_MACAddr(other.primaryRemoteMAC))                    &&

        (secondaryLocalPort     == other.secondaryLocalPort)    &&
        (secondaryRemoteIP      == other.secondaryRemoteIP)     &&
        (secondaryRemotePort    == other.secondaryRemotePort)   &&
        (secondaryAutoMAC       == other.secondaryAutoMAC)      &&
        (eq_MACAddr(other.secondaryRemoteMAC)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void rx_2022_channel::init()
{
    primaryRxMatch  = 0;
    primarySourceIP.erase();
    primaryDestIP.erase();
    primarySourcePort = 0;
    primaryDestPort = 0;
    primarySsrc = 0;
    primaryVlan = 0;
    secondaryRxMatch  = 0;
    secondarySourceIP.erase();
    secondaryDestIP.erase();
    secondarySourcePort = 0;
    secondaryDestPort = 0;
    secondarySsrc = 0;
    secondaryVlan = 0;
    networkPathDiff = 50;
    playoutDelay = 50;
}

bool rx_2022_channel::operator != ( const rx_2022_channel &other )
{
    return !(*this == other);
}

bool rx_2022_channel::operator == ( const rx_2022_channel &other )
{
    if ((primaryRxMatch        == other.primaryRxMatch)       &&
        (primarySourceIP       == other.primarySourceIP)      &&
        (primaryDestIP         == other.primaryDestIP)        &&
        (primarySourcePort     == other.primarySourcePort)    &&
        (primaryDestPort       == other.primaryDestPort)      &&
        (primarySsrc           == other.primarySsrc)          &&
        (primaryVlan           == other.primaryVlan)          &&
        (secondaryRxMatch      == other.secondaryRxMatch)     &&
        (secondarySourceIP     == other.secondarySourceIP)    &&
        (secondaryDestIP       == other.secondaryDestIP)      &&
        (secondarySourcePort   == other.secondarySourcePort)  &&
        (secondaryDestPort     == other.secondaryDestPort)    &&
        (secondarySsrc         == other.secondarySsrc)        &&
        (secondaryVlan         == other.secondaryVlan)        &&
        (networkPathDiff       == other.networkPathDiff)      &&
        (playoutDelay          == other.playoutDelay))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void rx2022Config::init()
{
    rxc_enable  = 0;
    rxc_primaryRxMatch = 0;
    rxc_primarySourceIp = 0;
    rxc_primaryDestIp = 0;
    rxc_primarySourcePort = 0;
    rxc_primaryDestPort  = 0;
    rxc_primarySsrc = 0;
    rxc_primaryVlan = 0;
    rxc_secondaryRxMatch = 0;
    rxc_secondarySourceIp = 0;
    rxc_secondaryDestIp = 0;
    rxc_secondarySourcePort = 0;
    rxc_secondaryDestPort  = 0;
    rxc_secondarySsrc = 0;
    rxc_secondaryVlan = 0;
    rxc_networkPathDiff = 50;
    rxc_playoutDelay = 50;
}

bool rx2022Config::operator != ( const rx2022Config &other )
{
    return (!(*this == other));
}

bool rx2022Config::operator == ( const rx2022Config &other )
{
    if ((rxc_enable                   == other.rxc_enable)                &&
        (rxc_primaryRxMatch           == other.rxc_primaryRxMatch)        &&
        (rxc_primarySourceIp          == other.rxc_primarySourceIp)       &&
        (rxc_primaryDestIp            == other.rxc_primaryDestIp)         &&
        (rxc_primarySourcePort        == other.rxc_primarySourcePort)     &&
        (rxc_primaryDestPort          == other.rxc_primaryDestPort)       &&
        (rxc_primarySsrc              == other.rxc_primarySsrc)           &&
        (rxc_primaryVlan              == other.rxc_primaryVlan)           &&
        (rxc_secondaryRxMatch         == other.rxc_secondaryRxMatch)      &&
        (rxc_secondarySourceIp        == other.rxc_secondarySourceIp)     &&
        (rxc_secondaryDestIp          == other.rxc_secondaryDestIp)       &&
        (rxc_secondarySourcePort      == other.rxc_secondarySourcePort)   &&
        (rxc_secondaryDestPort        == other.rxc_secondaryDestPort)     &&
        (rxc_secondarySsrc            == other.rxc_secondarySsrc)         &&
        (rxc_secondaryVlan            == other.rxc_secondaryVlan)         &&
        (rxc_networkPathDiff          == other.rxc_networkPathDiff)       &&
        (rxc_playoutDelay             == other.rxc_playoutDelay))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void tx2022Config::init()
{
    txc_enable  = 0;
    txc_primaryLocalPort = 0;
    txc_primaryRemoteIp = 0;
    txc_primaryRemotePort = 0;
    txc_primaryRemoteMAC_lo = 0;
    txc_primaryRemoteMAC_hi  = 0;
    txc_primaryAutoMac = 0;
    txc_secondaryLocalPort = 0;
    txc_secondaryRemoteIp = 0;
    txc_secondaryRemotePort = 0;
    txc_secondaryRemoteMAC_lo = 0;
    txc_secondaryRemoteMAC_hi = 0;
    txc_secondaryAutoMac  = 0;
}

bool tx2022Config::operator != ( const tx2022Config &other )
{
    return (!(*this == other));
}

bool tx2022Config::operator == ( const tx2022Config &other )
{
    if ((txc_enable                   == other.txc_enable)                &&
        (txc_primaryLocalPort         == other.txc_primaryLocalPort)      &&
        (txc_primaryRemoteIp          == other.txc_primaryRemoteIp)       &&
        (txc_primaryRemotePort        == other.txc_primaryRemotePort)     &&
        (txc_primaryRemoteMAC_lo      == other.txc_primaryRemoteMAC_lo)   &&
        (txc_primaryRemoteMAC_hi      == other.txc_primaryRemoteMAC_hi)   &&
        (txc_primaryAutoMac           == other.txc_primaryAutoMac)        &&
        (txc_secondaryLocalPort       == other.txc_secondaryLocalPort)    &&
        (txc_secondaryRemoteIp        == other.txc_secondaryRemoteIp)     &&
        (txc_secondaryRemotePort      == other.txc_secondaryRemotePort)   &&
        (txc_secondaryRemoteMAC_lo    == other.txc_secondaryRemoteMAC_lo) &&
        (txc_secondaryRemoteMAC_hi    == other.txc_secondaryRemoteMAC_hi) &&
        (txc_secondaryAutoMac         == other.txc_secondaryAutoMac))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void j2kEncoderConfig::init()
{
    videoFormat     = NTV2_FORMAT_720p_5994;
    ullMode         = false;
    bitDepth        = 10;
    chromaSubsamp   = kJ2KChromaSubSamp_422_Standard;
    mbps            = 200;
    streamType      = kJ2KStreamTypeStandard;
    audioChannels   = 2;
    pmtPid          = 255;
    videoPid        = 256;
    pcrPid          = 257;
    audio1Pid       = 258;
}

bool j2kEncoderConfig::operator != ( const j2kEncoderConfig &other )
{
    return (!(*this == other));
}

bool j2kEncoderConfig::operator == ( const j2kEncoderConfig &other )
{
    if ((videoFormat        == other.videoFormat)       &&
        (ullMode            == other.ullMode)           &&
        (bitDepth           == other.bitDepth)          &&
        (chromaSubsamp      == other.chromaSubsamp)     &&
        (mbps               == other.mbps)              &&
        (streamType         == other.streamType)        &&
        (audioChannels      == other.audioChannels)     &&
        (pmtPid             == other.pmtPid)            &&
        (videoPid           == other.videoPid)          &&
        (pcrPid             == other.pcrPid)            &&
        (audio1Pid          == other.audio1Pid))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void j2kDecoderConfig::init()
{
    selectionMode = eProgSel_Default;
    programNumber = 1;
    programPID    = 0;
    audioNumber   = 1;
}

bool j2kDecoderConfig::operator != ( const j2kDecoderConfig &other )
{
    return (!(*this == other));
}

bool j2kDecoderConfig::operator == ( const j2kDecoderConfig &other )
{
    if ((selectionMode      == other.selectionMode)     &&
        (programNumber      == other.programNumber)     &&
        (programPID         == other.programPID)        &&
        (audioNumber        == other.audioNumber))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void j2kDecoderStatus::init()
{
    numAvailablePrograms = 0;
    numAvailableAudios       = 0;
    availableProgramNumbers.clear();
    availableProgramPIDs.clear();
    availableAudioPIDs.clear();
}


//////////////////////////////////////////////////////////////////////////////////
//
//  CNTV2Config2022
//
//////////////////////////////////////////////////////////////////////////////////

CNTV2Config2022::CNTV2Config2022(CNTV2Card & device) : CNTV2MBController(device)
{
    uint32_t features    = getFeatures();

    _numTx0Chans = (features & (SAREK_TX0_MASK)) >> 28;
    _numRx0Chans = (features & (SAREK_RX0_MASK)) >> 24;
    _numTx1Chans = (features & (SAREK_TX1_MASK)) >> 20;
    _numRx1Chans = (features & (SAREK_RX1_MASK)) >> 16;

    _numRxChans  = _numRx0Chans + _numRx1Chans;
    _numTxChans  = _numTx0Chans + _numTx1Chans;

    _is2022_6   = ((features & SAREK_2022_6)   != 0);
    _is2022_2   = ((features & SAREK_2022_2)   != 0);
    _is2022_7   = ((features & SAREK_2022_7)   != 0);
    _is_txTop34 = ((features & SAREK_TX_TOP34) != 0);
    _hasPTP     = ((features & SAREK_PTP_PLL)  != 0);

    _biDirectionalChannels = false;

    _tstreamConfig = NULL;
    if (_is2022_2)
    {
        _tstreamConfig = new CNTV2ConfigTs2022(device);
    }
}

CNTV2Config2022::~CNTV2Config2022()
{
    if (_is2022_2)
    {
        delete _tstreamConfig;
    }
}

bool CNTV2Config2022::SetNetworkConfiguration(eSFP port, const IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    struct in_addr addr;
    addr.s_addr = (uint32_t)netConfig.ipc_ip;
    ip = inet_ntoa(addr);
    addr.s_addr = (uint32_t)netConfig.ipc_subnet;
    subnet = inet_ntoa(addr);
    addr.s_addr = (uint32_t)netConfig.ipc_gateway;
    gateway = inet_ntoa(addr);

    SetNetworkConfiguration(port, ip, subnet, gateway);
    return true;
}

bool CNTV2Config2022::SetNetworkConfiguration (eSFP port, string localIPAddress, string netmask, string gateway)
{
    uint32_t addr = inet_addr(localIPAddress.c_str());
    addr = NTV2EndianSwap32(addr);

    uint32_t macLo;
    uint32_t macHi;

    // get primaray mac address
    uint32_t macAddressRegister = SAREK_REGS + kRegSarekMAC;
    mDevice.ReadRegister(macAddressRegister, &macHi);
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, &macLo);

    uint32_t boardHi = (macHi & 0xffff0000) >>16;
    uint32_t boardLo = ((macHi & 0x0000ffff) << 16) + ((macLo & 0xffff0000) >> 16);

    // get secondary mac address
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, &macHi);
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, &macLo);

    uint32_t boardHi2 = (macHi & 0xffff0000) >>16;
    uint32_t boardLo2 = ((macHi & 0x0000ffff) << 16) + ((macLo & 0xffff0000) >> 16);

    uint32_t core6;
    uint32_t core2;

    if (port == SFP_TOP)
    {
        core6 = (_is_txTop34) ? SAREK_2022_6_TX_CORE_1 : SAREK_2022_6_TX_CORE_0;
        core2 = SAREK_2022_2_TX_CORE_0;
    }
    else
    {
        core6 = (_is_txTop34) ? SAREK_2022_6_TX_CORE_0 : SAREK_2022_6_TX_CORE_1;
        core2 = SAREK_2022_2_TX_CORE_1;
    }

    if (_is2022_6)
    {
        // initialise constants
        mDevice.WriteRegister(kReg2022_6_tx_sys_mem_conf     + core6, 0x04);
        mDevice.WriteRegister(kReg2022_6_tx_hitless_config   + core6, 0x01); // disable

        // source ip address
        mDevice.WriteRegister(kReg2022_6_tx_src_ip_addr      + core6,addr);

        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_low_addr + core6,boardLo);
        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_hi_addr  + core6,boardHi);

        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_low_addr + core6,boardLo2);
        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_hi_addr  + core6,boardHi2);
    }
    else
    {
        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_low_addr + core2,boardLo);
        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_hi_addr  + core2,boardHi);

        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_low_addr + core2,boardLo2);
        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_hi_addr  + core2,boardHi2);
    }

    if (_hasPTP)
    {
        ConfigurePTP(port,localIPAddress);
    }

    bool rv = AcquireMailbox();
    if (rv)
    {
        rv = SetMBNetworkConfiguration (port, localIPAddress, netmask, gateway);
        ReleaseMailbox();
    }
    return rv;
}

bool CNTV2Config2022::SetNetworkConfiguration (string localIPAddress0, string netmask0, string gateway0,
                                                string localIPAddress1, string netmask1, string gateway1)
{

    SetNetworkConfiguration(SFP_TOP, localIPAddress0, netmask0, gateway0);
    SetNetworkConfiguration(SFP_BOTTOM, localIPAddress1, netmask1, gateway1);

    return true;
}

bool CNTV2Config2022::GetNetworkConfiguration(eSFP port, IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    GetNetworkConfiguration(port, ip, subnet, gateway);

    netConfig.ipc_ip      = NTV2EndianSwap32((uint32_t)inet_addr(ip.c_str()));
    netConfig.ipc_subnet  = NTV2EndianSwap32((uint32_t)inet_addr(subnet.c_str()));
    netConfig.ipc_gateway = NTV2EndianSwap32((uint32_t)inet_addr(gateway.c_str()));

    return true;
}

bool CNTV2Config2022::GetNetworkConfiguration(eSFP port, string & localIPAddress, string & subnetMask, string & gateway)
{
    struct in_addr addr;

    if (port == SFP_TOP)
    {
        uint32_t val;
        mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0,&val);
        addr.s_addr = val;
        localIPAddress = inet_ntoa(addr);

        mDevice.ReadRegister(SAREK_REGS + kRegSarekNET0,&val);
        addr.s_addr = val;
        subnetMask = inet_ntoa(addr);

        mDevice.ReadRegister(SAREK_REGS + kRegSarekGATE0,&val);
        addr.s_addr = val;
        gateway = inet_ntoa(addr);
    }
    else
    {
        uint32_t val;
        mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1,&val);
        addr.s_addr = val;
        localIPAddress = inet_ntoa(addr);

        mDevice.ReadRegister(SAREK_REGS + kRegSarekNET1,&val);
        addr.s_addr = val;
        subnetMask = inet_ntoa(addr);

        mDevice.ReadRegister(SAREK_REGS + kRegSarekGATE1,&val);
        addr.s_addr = val;
        gateway = inet_ntoa(addr);
    }
    return true;
}

bool CNTV2Config2022::GetNetworkConfiguration(std::string & localIPAddress0, std::string & subnetMask0, std::string & gateway0,
                                               std::string & localIPAddress1, std::string & subnetMask1, std::string & gateway1)
{

    GetNetworkConfiguration(SFP_TOP, localIPAddress0, subnetMask0, gateway0);
    GetNetworkConfiguration(SFP_BOTTOM, localIPAddress1, subnetMask1, gateway1);

    return true;
}

bool CNTV2Config2022::SetRxChannelConfiguration(const NTV2Channel channel,const rx_2022_channel &rxConfig)
{
    uint32_t    baseAddr;
    bool        rv;

    if (_is2022_7)
    {
        // select secondary channel
        rv = SelectRxChannel(channel, false, baseAddr);
        if (!rv) return false;

        // hold off access while we update channel regs
        ChannelSemaphoreClear(kReg2022_6_rx_control, baseAddr);

        // source ip address
        uint32_t sourceIp = inet_addr(rxConfig.secondarySourceIP.c_str());
        sourceIp = NTV2EndianSwap32(sourceIp);
        WriteChannelRegister(kReg2022_6_rx_match_src_ip_addr + baseAddr, sourceIp);

        // dest ip address
        uint32_t destIp = inet_addr(rxConfig.secondaryDestIP.c_str());
        destIp = NTV2EndianSwap32(destIp);
        WriteChannelRegister(kReg2022_6_rx_match_dest_ip_addr + baseAddr, destIp);

        // source port
        WriteChannelRegister(kReg2022_6_rx_match_src_port + baseAddr, rxConfig.secondarySourcePort);

        // dest port
        WriteChannelRegister(kReg2022_6_rx_match_dest_port + baseAddr, rxConfig.secondaryDestPort);

        // ssrc
        WriteChannelRegister(kReg2022_6_rx_match_ssrc + baseAddr, rxConfig.secondarySsrc);

        // vlan
        WriteChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, rxConfig.secondaryVlan);

        // matching
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, rxConfig.secondaryRxMatch);

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_rx_control, baseAddr);

        // update IGMP subscriptions
        uint8_t ip0 = (destIp & 0xff000000)>> 24;
        if (ip0 >= 224 && ip0 <= 239)
        {
            // is multicast
            bool enabled = false;
            GetRxChannelEnable(channel,enabled);
            SetIGMPGroup(SFP_BOTTOM, channel, NTV2_VIDEO_STREAM, destIp, enabled);
        }
        else
        {
            UnsetIGMPGroup(SFP_BOTTOM, channel, NTV2_VIDEO_STREAM);
        }
    }

    // select primary channel
    rv = SelectRxChannel(channel, true, baseAddr);
    if (!rv) return false;

    // hold off access while we update channel regs
    ChannelSemaphoreClear(kReg2022_6_rx_control, baseAddr);

    // source ip address
    uint32_t sourceIp = inet_addr(rxConfig.primarySourceIP.c_str());
    sourceIp = NTV2EndianSwap32(sourceIp);
    WriteChannelRegister(kReg2022_6_rx_match_src_ip_addr + baseAddr, sourceIp);

    // dest ip address
    uint32_t destIp = inet_addr(rxConfig.primaryDestIP.c_str());
    destIp = NTV2EndianSwap32(destIp);
    WriteChannelRegister(kReg2022_6_rx_match_dest_ip_addr + baseAddr, destIp);

    // source port
    WriteChannelRegister(kReg2022_6_rx_match_src_port + baseAddr, rxConfig.primarySourcePort);

    // dest port
    WriteChannelRegister(kReg2022_6_rx_match_dest_port + baseAddr, rxConfig.primaryDestPort);

    // ssrc
    WriteChannelRegister(kReg2022_6_rx_match_ssrc + baseAddr, rxConfig.primarySsrc);

    // vlan
    WriteChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, rxConfig.primaryVlan);

    // matching
    WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, rxConfig.primaryRxMatch);

    // playout delay in 27MHz clocks or 90kHz clocks
    uint32_t delay = (_is2022_2) ? (rxConfig.playoutDelay * 90) << 9 : rxConfig.playoutDelay * 27000;
    WriteChannelRegister(kReg2022_6_rx_playout_delay + baseAddr, delay);

    // network path differential in 27MHz or 90kHz clocks
    delay = (_is2022_2) ? (rxConfig.networkPathDiff * 90) << 9 : rxConfig.networkPathDiff * 27000;
    WriteChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, delay);

    // some constants
    WriteChannelRegister(kReg2022_6_rx_chan_timeout        + baseAddr, 0x0000ffff);
    WriteChannelRegister(kReg2022_6_rx_media_pkt_buf_size  + baseAddr, 0x0000ffff);
    WriteChannelRegister(kReg2022_6_rx_media_buf_base_addr + baseAddr, 0x10000000 * channel);

    // enable  register updates
    ChannelSemaphoreSet(kReg2022_6_rx_control, baseAddr);

    if (_is2022_2)
    {
        // setup PLL
       mDevice.WriteRegister(kRegPll_Config  + SAREK_PLL, PLL_CONFIG_PCR,PLL_CONFIG_PCR);
       mDevice.WriteRegister(kRegPll_SrcIp   + SAREK_PLL, sourceIp);
       mDevice.WriteRegister(kRegPll_SrcPort + SAREK_PLL, rxConfig.primarySourcePort);
       mDevice.WriteRegister(kRegPll_DstIp   + SAREK_PLL, destIp);
       mDevice.WriteRegister(kRegPll_DstPort + SAREK_PLL, rxConfig.primaryDestPort);

       uint32_t rxMatch  = rxConfig.primaryRxMatch;
       uint32_t pllMatch = 0;
       if (rxMatch & RX_MATCH_DEST_IP)     pllMatch |= PLL_MATCH_DEST_IP;
       if (rxMatch & RX_MATCH_SOURCE_IP)   pllMatch |= PLL_MATCH_SOURCE_IP;
       if (rxMatch & RX_MATCH_DEST_PORT)   pllMatch |= PLL_MATCH_DEST_PORT;
       if (rxMatch & RX_MATCH_SOURCE_PORT) pllMatch |= RX_MATCH_SOURCE_PORT;
       pllMatch |= PLL_MATCH_ES_PID;    // always set for TS PCR
       mDevice.WriteRegister(kRegPll_Match   + SAREK_PLL, pllMatch);

	}

    // update IGMP subscriptions
    eSFP port = GetRxPort(channel);
    uint8_t ip0 = (destIp & 0xff000000)>> 24;
    if (ip0 >= 224 && ip0 <= 239)
    {
        // is multicast
        bool enabled = false;
        GetRxChannelEnable(channel,enabled);
        SetIGMPGroup(port, channel, NTV2_VIDEO_STREAM, destIp, enabled);
    }
    else
    {
        UnsetIGMPGroup(port, channel, NTV2_VIDEO_STREAM);
    }

    return rv;
}

bool  CNTV2Config2022::GetRxChannelConfiguration(const NTV2Channel channel, rx_2022_channel & rxConfig)
{
    uint32_t    baseAddr;
    uint32_t    val;
    bool        rv;

    if (_is2022_7)
    {
        // Select secondary channel
        rv = SelectRxChannel(channel, false, baseAddr);
        if (!rv) return false;

        // source ip address
        ReadChannelRegister(kReg2022_6_rx_match_src_ip_addr + baseAddr, &val);
        struct in_addr in;
        in.s_addr = NTV2EndianSwap32(val);
        char * ip = inet_ntoa(in);
        rxConfig.secondarySourceIP = ip;

        // dest ip address
        ReadChannelRegister(kReg2022_6_rx_match_dest_ip_addr + baseAddr, &val);
        in.s_addr = NTV2EndianSwap32(val);
        ip = inet_ntoa(in);
        rxConfig.secondaryDestIP = ip;

        // source port
        ReadChannelRegister(kReg2022_6_rx_match_src_port + baseAddr, &rxConfig.secondarySourcePort);

        // dest port
        ReadChannelRegister(kReg2022_6_rx_match_dest_port + baseAddr, &rxConfig.secondaryDestPort);

        // ssrc
        ReadChannelRegister(kReg2022_6_rx_match_ssrc + baseAddr, &rxConfig.secondarySsrc);       // PSM fix this, this is a shared reg

        // vlan
        ReadChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, &val);
        rxConfig.secondaryVlan = val & 0xffff;

        // matching
        ReadChannelRegister(kReg2022_6_rx_match_sel + baseAddr, &rxConfig.secondaryRxMatch);
    }

    // select primary channel
    rv = SelectRxChannel(channel, true, baseAddr);
    if (!rv) return false;

    // source ip address
    ReadChannelRegister(kReg2022_6_rx_match_src_ip_addr + baseAddr, &val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    rxConfig.primarySourceIP = ip;

    // dest ip address
    ReadChannelRegister(kReg2022_6_rx_match_dest_ip_addr + baseAddr, &val);
    in.s_addr = NTV2EndianSwap32(val);
    ip = inet_ntoa(in);
    rxConfig.primaryDestIP = ip;

    // source port
    ReadChannelRegister(kReg2022_6_rx_match_src_port + baseAddr, &rxConfig.primarySourcePort);

    // dest port
    ReadChannelRegister(kReg2022_6_rx_match_dest_port + baseAddr, &rxConfig.primaryDestPort);

    // ssrc
    ReadChannelRegister(kReg2022_6_rx_match_ssrc + baseAddr, &rxConfig.primarySsrc);

    // vlan
    ReadChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, &val);
    rxConfig.primaryVlan = val & 0xffff;

    // matching
    ReadChannelRegister(kReg2022_6_rx_match_sel + baseAddr, &rxConfig.primaryRxMatch);

    // playout delay in ms
    ReadChannelRegister(kReg2022_6_rx_playout_delay + baseAddr,  &val);
    rxConfig.playoutDelay = (_is2022_2) ? (val >>9)/90 : val/27000;

    // network path differential in ms
    ReadChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, &val);
    rxConfig.playoutDelay = (_is2022_2) ? (val>>9)/90 : val/27000;

    return true;
}

bool CNTV2Config2022::SetRxChannelEnable(const NTV2Channel channel, bool enable, bool enable2022_7)
{
    uint32_t    baseAddr;
    bool        rv;
    bool        disableIGMP;
    eSFP        port;

    if (enable && _biDirectionalChannels)
    {
        bool txEnabled;
        GetTxChannelEnable(channel,txEnabled);
        if (txEnabled)
        {
            // disable tx channel
            SetTxChannelEnable(channel,false, enable2022_7);
        }
        mDevice.SetSDITransmitEnable(channel, false);
    }

    // select primary channel
    rv = SelectRxChannel(channel, true, baseAddr);
    if (!rv) return false;

    if (_is2022_7 && enable2022_7)
    {
        // IGMP subscription for secondary channel
        port = SFP_BOTTOM;
        GetIGMPDisable(port, disableIGMP);

        if (!disableIGMP)
        {
            EnableIGMPGroup(port,channel,NTV2_VIDEO_STREAM,enable);
        }
    }

    // IGMP subscription for primary channel
    port = GetRxPort(channel);
    GetIGMPDisable(port, disableIGMP);

    if (!disableIGMP)
    {
        EnableIGMPGroup(port,channel,NTV2_VIDEO_STREAM,enable);
    }

    if (enable)
    {
        WriteChannelRegister(kReg2022_6_rx_chan_enable + baseAddr, 0x01);
    }
    else
    {
        WriteChannelRegister(kReg2022_6_rx_chan_enable + baseAddr, 0x0);
    }

    return rv;
}

bool CNTV2Config2022::GetRxChannelEnable(const NTV2Channel channel, bool & enabled)
{
    uint32_t baseAddr;

    // select primary channel
    bool rv = SelectRxChannel(channel, true, baseAddr);
    if (!rv) return false;

    uint32_t val;
    rv = ReadChannelRegister(kReg2022_6_rx_chan_enable + baseAddr,&val);
    if (rv)
    {
        enabled = (val != 0x00);
    }

    return rv;
}

bool CNTV2Config2022::SetTxChannelConfiguration(const NTV2Channel channel, const tx_2022_channel & txConfig)
{
    uint32_t    baseAddr;
    uint32_t    val;
    uint32_t    hi;
    uint32_t    lo;
    MACAddr     macaddr;
    uint8_t     ip0;
    uint32_t    destIp;
    uint32_t    mac;
    bool        rv;

    // select channel

    if (_is2022_7)
    {
        // Select secondary channel
        rv = SelectTxChannel(channel, false, baseAddr);
        if (!rv) return false;

        // hold off access while we update channel regs
        ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

        // initialise
        WriteChannelRegister(kReg2022_6_tx_ip_header + baseAddr, 0x6480);
        if (_is2022_6)
        {
            WriteChannelRegister(kReg2022_6_tx_video_para_config + baseAddr, 0x01);     // include video timestamp
        }
        else
        {
            WriteChannelRegister(kReg2022_2_tx_ts_config + baseAddr,0x0e);
        }

        // dest ip address
        destIp = inet_addr(txConfig.secondaryRemoteIP.c_str());
        destIp = NTV2EndianSwap32(destIp);
        WriteChannelRegister(kReg2022_6_tx_dest_ip_addr + baseAddr,destIp);

        // source port
        WriteChannelRegister(kReg2022_6_tx_udp_src_port + baseAddr,txConfig.secondaryLocalPort);

        // dest port
        WriteChannelRegister(kReg2022_6_tx_udp_dest_port + baseAddr,txConfig.secondaryRemotePort);

        // auto MAC setting
        mDevice.ReadRegister(kRegSarekTxAutoMAC + SAREK_REGS,&val);
        if (txConfig.secondaryAutoMAC)
        {
            val |= (1 << channel);
        }
        else
        {
            val &= ~(1 << channel);
        }
        mDevice.WriteRegister(kRegSarekTxAutoMAC + SAREK_REGS,val);

        // dest MAC
        if (txConfig.secondaryAutoMAC)
        {
            // is remote address muticast
            ip0 = (destIp & 0xff000000)>> 24;
            if (ip0 >= 224 && ip0 <= 239)
            {
               // generate multicast MAC
                mac = destIp & 0x7fffff;  // lower 23 bits

                macaddr.mac[0] = 0x01;
                macaddr.mac[1] = 0x00;
                macaddr.mac[2] = 0x5e;
                macaddr.mac[3] =  mac >> 16;
                macaddr.mac[4] = (mac & 0xffff) >> 8;
                macaddr.mac[5] =  mac & 0xff;

                hi  = macaddr.mac[0]  << 8;
                hi += macaddr.mac[1];

                lo  = macaddr.mac[2] << 24;
                lo += macaddr.mac[3] << 16;
                lo += macaddr.mac[4] << 8;
                lo += macaddr.mac[5];
            }
            else
            {
                // get MAC from ARP
                string macAddr;
                rv = AcquireMailbox();
                if (rv)
                {
                    rv = GetRemoteMAC(txConfig.secondaryRemoteIP,macAddr);
                    ReleaseMailbox();
                }
                if (!rv)
                {
                    mError = "Failed to retrieve MAC address from ARP table";
                    macAddr = "0:0:0:0:0:0";
                }

                istringstream ss(macAddr);
                string token;
                int i=0;
                while (i < 6)
                {
                    getline (ss, token, ':');
                    macaddr.mac[i++] = (uint8_t)strtoul(token.c_str(),NULL,16);
                }

                hi  = macaddr.mac[0]  << 8;
                hi += macaddr.mac[1];

                lo  = macaddr.mac[2] << 24;
                lo += macaddr.mac[3] << 16;
                lo += macaddr.mac[4] << 8;
                lo += macaddr.mac[5];
            }
        }
        else
        {
            // use supplied MAC
            hi  = txConfig.secondaryRemoteMAC.mac[0]  << 8;
            hi += txConfig.secondaryRemoteMAC.mac[1];

            lo  = txConfig.secondaryRemoteMAC.mac[2] << 24;
            lo += txConfig.secondaryRemoteMAC.mac[3] << 16;
            lo += txConfig.secondaryRemoteMAC.mac[4] << 8;
            lo += txConfig.secondaryRemoteMAC.mac[5];
        }

        WriteChannelRegister(kReg2022_6_tx_dest_mac_low_addr + baseAddr,lo);
        WriteChannelRegister(kReg2022_6_tx_dest_mac_hi_addr  + baseAddr,hi);

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);
    }

    // select primary channel
    rv = SelectTxChannel(channel, true, baseAddr);
    if (!rv) return false;

    // hold off access while we update channel regs
    ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

    // initialise
    WriteChannelRegister(kReg2022_6_tx_ip_header + baseAddr, 0x6480);
    if (_is2022_6)
    {
        WriteChannelRegister(kReg2022_6_tx_video_para_config + baseAddr, 0x01);     // include video timestamp
    }
    else
    {
        WriteChannelRegister(kReg2022_2_tx_ts_config + baseAddr,0x0e);
    }

    // dest ip address
    destIp = inet_addr(txConfig.primaryRemoteIP.c_str());
    destIp = NTV2EndianSwap32(destIp);
    WriteChannelRegister(kReg2022_6_tx_dest_ip_addr + baseAddr,destIp);

    // source port
    WriteChannelRegister(kReg2022_6_tx_udp_src_port + baseAddr,txConfig.primaryLocalPort);

    // dest port
    WriteChannelRegister(kReg2022_6_tx_udp_dest_port + baseAddr,txConfig.primaryRemotePort);

    // auto MAC setting
    uint32_t autoMacReg;
    mDevice.ReadRegister(kRegSarekTxAutoMAC + SAREK_REGS,&autoMacReg);
    if (txConfig.primaryAutoMAC)
    {
        autoMacReg |= (1 << channel);
    }
    else
    {
        autoMacReg &= ~(1 << channel);
    }
    mDevice.WriteRegister(kRegSarekTxAutoMAC + SAREK_REGS,autoMacReg);

    // dest MAC
    if (txConfig.primaryAutoMAC)
    {
        // is remote address muticast
        ip0 = (destIp & 0xff000000)>> 24;
        if (ip0 >= 224 && ip0 <= 239)
        {
           // generate multicast MAC
            mac = destIp & 0x7fffff;  // lower 23 bits

            macaddr.mac[0] = 0x01;
            macaddr.mac[1] = 0x00;
            macaddr.mac[2] = 0x5e;
            macaddr.mac[3] =  mac >> 16;
            macaddr.mac[4] = (mac & 0xffff) >> 8;
            macaddr.mac[5] =  mac & 0xff;

            hi  = macaddr.mac[0]  << 8;
            hi += macaddr.mac[1];

            lo  = macaddr.mac[2] << 24;
            lo += macaddr.mac[3] << 16;
            lo += macaddr.mac[4] << 8;
            lo += macaddr.mac[5];
        }
        else
        {
            // get MAC from ARP
            string macAddr;
            rv = AcquireMailbox();
            if (rv)
            {
                rv = GetRemoteMAC(txConfig.primaryRemoteIP,macAddr);
                ReleaseMailbox();
            }
            if (!rv)
            {
                mError = "Failed to retrieve MAC address from ARP table";
                macAddr = "0:0:0:0:0:0";
            }

            istringstream ss(macAddr);
            string token;
            int i=0;
            while (i < 6)
            {
                getline (ss, token, ':');
                macaddr.mac[i++] = (uint8_t)strtoul(token.c_str(),NULL,16);
            }

            hi  = macaddr.mac[0]  << 8;
            hi += macaddr.mac[1];

            lo  = macaddr.mac[2] << 24;
            lo += macaddr.mac[3] << 16;
            lo += macaddr.mac[4] << 8;
            lo += macaddr.mac[5];
        }
    }
    else
    {
        // use supplied MAC
        hi  = txConfig.primaryRemoteMAC.mac[0]  << 8;
        hi += txConfig.primaryRemoteMAC.mac[1];

        lo  = txConfig.primaryRemoteMAC.mac[2] << 24;
        lo += txConfig.primaryRemoteMAC.mac[3] << 16;
        lo += txConfig.primaryRemoteMAC.mac[4] << 8;
        lo += txConfig.primaryRemoteMAC.mac[5];
    }

    WriteChannelRegister(kReg2022_6_tx_dest_mac_low_addr + baseAddr,lo);
    WriteChannelRegister(kReg2022_6_tx_dest_mac_hi_addr  + baseAddr,hi);

    // enable  register updates
    ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);

    return rv;
}

bool CNTV2Config2022::GetTxChannelConfiguration(const NTV2Channel channel, tx_2022_channel & txConfig)
{
    uint32_t    baseAddr;
    uint32_t    val;
    bool        rv;

    if (_is2022_7)
    {
        // select secondary channel
        rv = SelectTxChannel(channel, false, baseAddr);
        if (!rv) return false;

        // dest ip address
        ReadChannelRegister(kReg2022_6_tx_dest_ip_addr + baseAddr,&val);
        struct in_addr in;
        in.s_addr = NTV2EndianSwap32(val);
        char * ip = inet_ntoa(in);
        txConfig.secondaryRemoteIP = ip;

        // source port
        ReadChannelRegister(kReg2022_6_tx_udp_src_port + baseAddr,&txConfig.secondaryLocalPort);

        // dest port
        ReadChannelRegister(kReg2022_6_tx_udp_dest_port + baseAddr,&txConfig.secondaryRemotePort);

        // dest MAC
        uint32_t hi;
        uint32_t lo;
        ReadChannelRegister(kReg2022_6_tx_dest_mac_low_addr + baseAddr, &lo);
        ReadChannelRegister(kReg2022_6_tx_dest_mac_hi_addr  + baseAddr, &hi);

        txConfig.secondaryRemoteMAC.mac[0] = (hi >> 8) & 0xff;
        txConfig.secondaryRemoteMAC.mac[1] =  hi        & 0xff;
        txConfig.secondaryRemoteMAC.mac[2] = (lo >> 24) & 0xff;
        txConfig.secondaryRemoteMAC.mac[3] = (lo >> 16) & 0xff;
        txConfig.secondaryRemoteMAC.mac[4] = (lo >> 8)  & 0xff;
        txConfig.secondaryRemoteMAC.mac[5] =  lo        & 0xff;

        mDevice.ReadRegister(kRegSarekTxAutoMAC + SAREK_REGS,&val);
        txConfig.secondaryAutoMAC = ((val & (1 << channel)) != 0);
    }

    // Select primary channel
    rv = SelectTxChannel(channel, true, baseAddr);
    if (!rv) return false;

    // dest ip address
    ReadChannelRegister(kReg2022_6_tx_dest_ip_addr + baseAddr,&val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    txConfig.primaryRemoteIP = ip;

    // source port
    ReadChannelRegister(kReg2022_6_tx_udp_src_port + baseAddr,&txConfig.primaryLocalPort);

    // dest port
    ReadChannelRegister(kReg2022_6_tx_udp_dest_port + baseAddr,&txConfig.primaryRemotePort);

    // dest MAC
    uint32_t hi;
    uint32_t lo;
    ReadChannelRegister(kReg2022_6_tx_dest_mac_low_addr + baseAddr, &lo);
    ReadChannelRegister(kReg2022_6_tx_dest_mac_hi_addr  + baseAddr, &hi);

    txConfig.primaryRemoteMAC.mac[0] = (hi >> 8) & 0xff;
    txConfig.primaryRemoteMAC.mac[1] =  hi        & 0xff;
    txConfig.primaryRemoteMAC.mac[2] = (lo >> 24) & 0xff;
    txConfig.primaryRemoteMAC.mac[3] = (lo >> 16) & 0xff;
    txConfig.primaryRemoteMAC.mac[4] = (lo >> 8)  & 0xff;
    txConfig.primaryRemoteMAC.mac[5] =  lo        & 0xff;

    mDevice.ReadRegister(kRegSarekTxAutoMAC + SAREK_REGS,&val);
    txConfig.primaryAutoMAC = ((val & (1 << channel)) != 0);

    return true;
}

bool CNTV2Config2022::SetTxChannelEnable(const NTV2Channel channel, bool enable, bool enable2022_7)
{
    uint32_t    baseAddr;
    bool        rv;
    uint32_t    localIp;

    if (enable && _biDirectionalChannels)
    {
        bool rxEnabled;
        GetRxChannelEnable(channel,rxEnabled);
        if (rxEnabled)
        {
            // disable rx channel
            SetRxChannelEnable(channel,false, enable2022_7);
        }
        mDevice.SetSDITransmitEnable(channel, true);
    }

    if (_is2022_7 && enable2022_7)
    {
        // Select secondary channel
        rv = SelectTxChannel(channel, false, baseAddr);
        if (!rv) return false;

        // hold off access while we update channel regs
        ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

        if (enable)
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1,&localIp);
            WriteChannelRegister(kReg2022_6_tx_src_ip_addr + baseAddr,NTV2EndianSwap32(localIp));

            // enables
            WriteChannelRegister(kReg2022_6_tx_tx_enable   + baseAddr,0x01);  // enables tx over mac1/mac2
            WriteChannelRegister(kReg2022_6_tx_chan_enable + baseAddr,0x01);  // enables channel
        }
        else
        {
            // disable
            WriteChannelRegister(kReg2022_6_tx_tx_enable   + baseAddr,0x0);   // disables tx over mac1/mac2
            WriteChannelRegister(kReg2022_6_tx_chan_enable + baseAddr,0x0);   // disables channel
        }

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);
    }


    // select primary channel
    rv = SelectTxChannel(channel, true, baseAddr);
    if (!rv) return false;

    // hold off access while we update channel regs
    ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

    if (enable)
    {
        if (GetTxPort(channel) == SFP_TOP)
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0,&localIp);
        }
        else
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1,&localIp);
        }
        WriteChannelRegister(kReg2022_6_tx_src_ip_addr + baseAddr,NTV2EndianSwap32(localIp));

        if (_is2022_7 && enable2022_7)
        {
            WriteChannelRegister(kReg2022_6_tx_hitless_config   + baseAddr,0x0);  // 0 enables hitless mode
        }
        else
        {
            WriteChannelRegister(kReg2022_6_tx_hitless_config   + baseAddr,0x01); // 1 disables hitless mode
        }

        // enables
        WriteChannelRegister(kReg2022_6_tx_tx_enable   + baseAddr,0x01);  // enables tx over mac1/mac2
        WriteChannelRegister(kReg2022_6_tx_chan_enable + baseAddr,0x01);  // enables channel
    }
    else
    {
        // disable
        WriteChannelRegister(kReg2022_6_tx_hitless_config + baseAddr,0x01);  // 1 disables hitless mode
        WriteChannelRegister(kReg2022_6_tx_tx_enable      + baseAddr,0x01);  // enables tx over mac1/mac2
        WriteChannelRegister(kReg2022_6_tx_chan_enable    + baseAddr,0x0);   // disables channel
    }

    // enable  register updates
    ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);

    return true;
}

bool CNTV2Config2022::GetTxChannelEnable(const NTV2Channel channel, bool & enabled)
{
    uint32_t baseAddr;

    // select primary channel
    bool rv = SelectTxChannel(channel, true, baseAddr);
    if (!rv) return false;

    uint32_t val;
    ReadChannelRegister(kReg2022_6_tx_chan_enable + baseAddr, &val);
    enabled = (val == 0x01);

    return true;
}

bool  CNTV2Config2022::SetPTPMaster(std::string ptpMaster)
{
    uint32_t addr = inet_addr(ptpMaster.c_str());
    addr = NTV2EndianSwap32(addr);
    return mDevice.WriteRegister(kRegPll_PTP_MstrIP + SAREK_PLL, addr);
}

bool CNTV2Config2022::GetPTPMaster(std::string & ptpMaster)
{
    uint32_t val;
    mDevice.ReadRegister(kRegPll_PTP_MstrIP + SAREK_PLL, &val);
    val = NTV2EndianSwap32(val);

    struct in_addr addr;
    addr.s_addr = val;
    ptpMaster = inet_ntoa(addr);

    return true;
}

bool CNTV2Config2022::SetIGMPDisable(eSFP port, bool disable)
{
    uint32_t val = (disable) ? 1 : 0;
    if (port == SFP_TOP )
    {
        mDevice.WriteRegister(SAREK_REGS + kSarekRegIGMPDisable,val);
    }
    else
    {
        mDevice.WriteRegister(SAREK_REGS + kSarekRegIGMPDisable2,val);
    }
    return true;
}

bool CNTV2Config2022::GetIGMPDisable(eSFP port, bool & disabled)
{
    uint32_t val;
    if (port == SFP_TOP )
    {
        mDevice.ReadRegister(SAREK_REGS + kSarekRegIGMPDisable,&val);
    }
    else
    {
        mDevice.ReadRegister(SAREK_REGS + kSarekRegIGMPDisable2,&val);
    }

    disabled = (val == 1) ? true : false;

    return true;
}

bool CNTV2Config2022::SetIGMPVersion(eIGMPVersion_t version)
{
    uint32_t mbversion;
    switch (version)
    {
    case eIGMPVersion_2:
        mbversion = 2;
        break;
    case eIGMPVersion_3:
        mbversion = 3;
        break;
    default:
        mError = "Invalid IGMP version";
        return false;
    }
    return CNTV2MBController::SetIGMPVersion(mbversion);
}

bool CNTV2Config2022::GetIGMPVersion(eIGMPVersion_t & version)
{
    uint32_t version32;
    bool rv = mDevice.ReadRegister(SAREK_REGS + kRegSarekIGMPVersion,&version32);
    version =  (version32 == 2) ? eIGMPVersion_2 : eIGMPVersion_3;
    return rv;
}

bool CNTV2Config2022::SetJ2KEncoderConfiguration(const NTV2Channel channel, const j2kEncoderConfig & j2kConfig)
{
    if (_is2022_2)
    {
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.SetupJ2KEncoder(channel, j2kConfig);
        mError = tsConfig.getLastError();
        return rv;
    }
    return false;
}

bool CNTV2Config2022::GetJ2KEncoderConfiguration(const NTV2Channel channel, j2kEncoderConfig &j2kConfig)
{
    if (_is2022_2)
    {
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.ReadbackJ2KEncoder(channel, j2kConfig);
        mError = tsConfig.getLastError();
        return rv;
    }
    return false;
}

bool CNTV2Config2022::SetJ2KDecoderConfiguration(const  j2kDecoderConfig & j2kConfig)
{
    if (_is2022_2)
    {
        mDevice.SetAudioSystemInputSource(NTV2_AUDIOSYSTEM_1,NTV2_AUDIO_AES,NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1);
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.SetupJ2KDecoder(j2kConfig);
        mError = tsConfig.getLastError();
        return rv;
    }
    return false;
}

bool CNTV2Config2022::GetJ2KDecoderConfiguration(j2kDecoderConfig & j2kConfig)
{
    if (_is2022_2)
    {
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.ReadbackJ2KDecoder(j2kConfig);
        mError = tsConfig.getLastError();
        return rv;
    }
    return false;
}

bool CNTV2Config2022::GetJ2KDecoderStatus(j2kDecoderStatus & j2kStatus)
{
    if (_is2022_2)
    {
        CNTV2ConfigTs2022 tsConfig(mDevice);
        bool rv = tsConfig.GetJ2KDecoderStatus(j2kStatus);
        return rv;
    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////

eSFP  CNTV2Config2022::GetRxPort(NTV2Channel chan)
{
    if ((uint32_t)chan >= _numRx0Chans)
    {
        return SFP_BOTTOM;
    }
    else
    {
        return SFP_TOP;
    }
}

eSFP  CNTV2Config2022::GetTxPort(NTV2Channel chan)
{
    if (!_is_txTop34)
    {
        if ((uint32_t)chan >= _numTx0Chans)
        {
            return SFP_BOTTOM;
        }
        else
        {
            return SFP_TOP;
        }
    }
    else
    {
        if ((uint32_t)chan >= _numTx0Chans)
        {
            return SFP_TOP;
        }
        else
        {
            return SFP_BOTTOM;
        }
    }
}


bool CNTV2Config2022::SelectRxChannel(NTV2Channel channel, bool primaryChannel, uint32_t & baseAddr)
{
    uint32_t iChannel = (uint32_t) channel;
    uint32_t channelIndex = iChannel;

    if (iChannel > _numRxChans)
        return false;

    if (_is2022_6)
    {
        if (iChannel >= _numRx0Chans)
        {
            channelIndex = iChannel - _numRx0Chans;
            baseAddr  = SAREK_2022_6_RX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2022_6_RX_CORE_0;
        }
    }
    else
    {
        if (iChannel >= _numRx0Chans)
        {
            channelIndex = iChannel - _numRx0Chans;
            baseAddr  = SAREK_2022_2_RX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2022_2_RX_CORE_0;
        }
    }

    if (!primaryChannel)
        channelIndex |= 0x80000000;

    // select channel
    SetChannel(kReg2022_6_rx_channel_access + baseAddr, channelIndex);

    return true;
}

bool CNTV2Config2022::SelectTxChannel(NTV2Channel channel, bool primaryChannel, uint32_t & baseAddr)
{
    uint32_t iChannel = (uint32_t) channel;
    uint32_t channelIndex = iChannel;

    if (iChannel > _numTxChans)
        return false;

    if (_is2022_6)
    {
        if (iChannel >= _numTx0Chans)
        {
            channelIndex = iChannel - _numTx0Chans;
            baseAddr  = SAREK_2022_6_TX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2022_6_TX_CORE_0;
        }
    }
    else
    {
        if (iChannel >= _numTx0Chans)
        {
            channelIndex = iChannel - _numTx0Chans;
            baseAddr  = SAREK_2022_2_TX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2022_2_TX_CORE_0;
        }
    }

    if (!primaryChannel)
        channelIndex |= 0x80000000;

    // select channel
    SetChannel(kReg2022_6_tx_channel_access + baseAddr, channelIndex);

    return true;
}

bool  CNTV2Config2022::ConfigurePTP (eSFP port, string localIPAddress)
{
    uint32_t macLo;
    uint32_t macHi;

    // get primaray mac address
    uint32_t macAddressRegister = SAREK_REGS + kRegSarekMAC;
    if (port != SFP_TOP)
    {
		macAddressRegister += 2;
	}
    mDevice.ReadRegister(macAddressRegister, &macHi);
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, &macLo);

    uint32_t alignedMACHi = macHi >> 16;
    uint32_t alignedMACLo = (macLo >> 16) | ( (macHi & 0xffff) << 16);

    uint32_t addr = inet_addr(localIPAddress.c_str());
    addr = NTV2EndianSwap32(addr);

	// configure pll
    WriteChannelRegister(kRegPll_PTP_LclMacLo   + SAREK_PLL, alignedMACLo);
    WriteChannelRegister(kRegPll_PTP_LclMacHi   + SAREK_PLL, alignedMACHi);

    WriteChannelRegister(kRegPll_PTP_EventUdp   + SAREK_PLL, 0x0000013f);
    WriteChannelRegister(kRegPll_PTP_MstrMcast  + SAREK_PLL, 0xe0000181);
    WriteChannelRegister(kRegPll_PTP_LclIP      + SAREK_PLL, addr);

    //WriteChannelRegister(kRegPll_PTP_LclClkIdLo + SAREK_PLL, (0xfe << 24) | ((macHi & 0x000000ff) << 16) | (macLo >> 16));
    //WriteChannelRegister(kRegPll_PTP_LclClkIdHi + SAREK_PLL, (macHi & 0xffffff00) | 0xff);

    return true;
}

string CNTV2Config2022::getLastError()
{
    string astring = mError;
    mError.clear();
    return astring;
}

void CNTV2Config2022::ChannelSemaphoreSet(uint32_t controlReg, uint32_t baseAddr)
{
    uint32_t val;
    ReadChannelRegister(controlReg + baseAddr, &val);
    WriteChannelRegister(controlReg + baseAddr, val | VOIP_SEMAPHORE_SET);
}

void CNTV2Config2022::ChannelSemaphoreClear(uint32_t controlReg, uint32_t baseAddr)
{
    uint32_t val;
    ReadChannelRegister(controlReg + baseAddr, &val);
    WriteChannelRegister(controlReg + baseAddr, val & VOIP_SEMAPHORE_CLEAR);
}

