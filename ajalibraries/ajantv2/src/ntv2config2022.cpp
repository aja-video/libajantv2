/**
    @file		ntv2config2022.cpp
    @brief		Implements the CNTV2Config2022 class.
    @copyright	(C) 2014-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2config2022.h"
#include "ntv2configts2022.h"
#include "ntv2endian.h"
#include "ntv2utils.h"
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
    linkAEnable         = true;
    linkBEnable         = false;

    primaryRemoteIP.erase();
    primaryLocalPort    = 0;
    primaryRemotePort   = 0;

    secondaryRemoteIP.erase();
    secondaryLocalPort  = 0;
    secondaryRemotePort = 0;

    tos                 = 0x64;
    ttl                 = 0x80;
    ssrc                = 0;
}

bool tx_2022_channel::operator != ( const tx_2022_channel &other )
{
    return !(*this == other);
}

bool tx_2022_channel::operator == ( const tx_2022_channel &other )
{
    if ((linkAEnable			= other.linkAEnable)			&&
		(linkBEnable			= other.linkBEnable)			&&
		
		(primaryLocalPort       == other.primaryLocalPort)      &&
        (primaryRemoteIP        == other.primaryRemoteIP)       &&
        (primaryRemotePort      == other.primaryRemotePort)     &&

        (secondaryLocalPort     == other.secondaryLocalPort)    &&
        (secondaryRemoteIP      == other.secondaryRemoteIP)     &&
        (secondaryRemotePort    == other.secondaryRemotePort))
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
    linkAEnable         = true;
    linkBEnable         = false;

    primaryRxMatch  = 0;
    primarySourceIP.erase();
    primaryDestIP.erase();
    primarySourcePort = 0;
    primaryDestPort = 0;
    primaryVlan = 0;

    secondaryRxMatch  = 0;
    secondarySourceIP.erase();
    secondaryDestIP.erase();
    secondarySourcePort = 0;
    secondaryDestPort = 0;
    secondaryVlan = 0;

    ssrc = 0;
    playoutDelay = 50;
}

bool rx_2022_channel::operator != ( const rx_2022_channel &other )
{
    return !(*this == other);
}

bool rx_2022_channel::operator == ( const rx_2022_channel &other )
{
	if ((linkAEnable			== other.linkAEnable)			&&
		(linkBEnable			== other.linkBEnable)			&&
		
		(primaryRxMatch			== other.primaryRxMatch)		&&
        (primarySourceIP		== other.primarySourceIP)		&&
        (primaryDestIP			== other.primaryDestIP)			&&
        (primarySourcePort		== other.primarySourcePort)		&&
        (primaryDestPort		== other.primaryDestPort)		&&
        (primaryVlan			== other.primaryVlan)			&&
		
        (secondaryRxMatch		== other.secondaryRxMatch)		&&
        (secondarySourceIP		== other.secondarySourceIP)		&&
        (secondaryDestIP		== other.secondaryDestIP)		&&
        (secondarySourcePort	== other.secondarySourcePort)	&&
        (secondaryDestPort		== other.secondaryDestPort)		&&
        (secondaryVlan			== other.secondaryVlan)			&&
		
        (ssrc                   == other.ssrc)                  &&
        (playoutDelay			== other.playoutDelay))
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
    selectionMode = j2kDecoderConfig::eProgSel_Default;
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

    _biDirectionalChannels = false;

    _tstreamConfig = NULL;
    if (_is2022_2)
    {
        _tstreamConfig = new CNTV2ConfigTs2022(device);
    }

    _isIoIp = (device.GetDeviceID() == DEVICE_ID_IOIP_2022);
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

    bool rv = SetNetworkConfiguration(port, ip, subnet, gateway);
    return rv;
}

bool CNTV2Config2022::SetNetworkConfiguration (eSFP port, string localIPAddress, string netmask, string gateway)
{
    if (!mDevice.IsMBSystemReady())
    {
        mIpErrorCode = NTV2IpErrNotReady;
        return false;
    }

    if (!mDevice.IsMBSystemValid())
    {
        mIpErrorCode = NTV2IpErrSoftwareMismatch;
        return false;
    }

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
        if (_isIoIp)
            mDevice.WriteRegister(kReg2022_6_tx_sys_mem_conf     + core6, 0x10);    // IoIP
        else
            mDevice.WriteRegister(kReg2022_6_tx_sys_mem_conf     + core6, 0x04);    // KonaIP
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

    bool rv = SetMBNetworkConfiguration (port, localIPAddress, netmask, gateway);
    return rv;
}

bool CNTV2Config2022::SetNetworkConfiguration (string localIPAddress0, string netmask0, string gateway0,
                                                string localIPAddress1, string netmask1, string gateway1)
{

    bool rv = SetNetworkConfiguration(SFP_TOP, localIPAddress0, netmask0, gateway0);
    if (!rv) return false;

    rv = SetNetworkConfiguration(SFP_BOTTOM, localIPAddress1, netmask1, gateway1);
    return rv;
}

bool  CNTV2Config2022::DisableNetworkInterface(eSFP port)
{
    return DisableNetworkConfiguration(port);
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

    bool enabled_7  = false;
    uint32_t unused = 0;
    Get2022_7_Mode(enabled_7,unused);

    bool linkA =  rxConfig.linkAEnable;
    bool linkB =  rxConfig.linkBEnable;
    if (enabled_7)
    {
        linkA = true;
        linkB = true;
    }

    if (linkA && (GetLinkActive(SFP_TOP) == false))
    {
        mIpErrorCode = NTV2IpErrLinkANotConfigured;
        return false;
    }

    if (linkB && (GetLinkActive(SFP_BOTTOM) == false))
    {
        mIpErrorCode = NTV2IpErrLinkBNotConfigured;
        return false;
    }

    if (_is2022_7)
    {
        // select secondary channel
        rv = SelectRxChannel(channel, SFP_BOTTOM, baseAddr);
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

        // vlan
        WriteChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, rxConfig.secondaryVlan);

        // matching
        SetRxMatch(channel,SFP_BOTTOM,rxConfig.secondaryRxMatch);

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_rx_control, baseAddr);

        // update IGMP subscriptions
        uint8_t ip0 = (destIp & 0xff000000)>> 24;
        if ((ip0 >= 224 && ip0 <= 239) && linkB)
        {
            // is multicast
            bool enabled = false;
            GetRxChannelEnable(channel,enabled);
            if (rxConfig.secondaryRxMatch & RX_MATCH_2022_SOURCE_IP)
                SetIGMPGroup(SFP_BOTTOM, channel, NTV2_VIDEO_STREAM, destIp, sourceIp, enabled);
            else
                SetIGMPGroup(SFP_BOTTOM, channel, NTV2_VIDEO_STREAM, destIp, 0, enabled);
        }
        else
        {
            UnsetIGMPGroup(SFP_BOTTOM, channel, NTV2_VIDEO_STREAM);
        }

         SetRxLinkState(channel, linkA, linkB);
    }
    else
    {
        SetRxLinkState(channel, true, false);
    }

    // select primary channel
    rv = SelectRxChannel(channel, SFP_TOP, baseAddr);
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
    WriteChannelRegister(kReg2022_6_rx_match_ssrc + baseAddr, rxConfig.ssrc);

    // vlan
    WriteChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, rxConfig.primaryVlan);

    // matching
    SetRxMatch(channel,SFP_TOP,rxConfig.primaryRxMatch);

    // playout delay in 27MHz clocks or 90kHz clocks
    uint32_t delay;
    delay = (_is2022_2) ? (rxConfig.playoutDelay * 90) << 9 : rxConfig.playoutDelay * 27000;
    WriteChannelRegister(kReg2022_6_rx_playout_delay + baseAddr, delay);

    // network path differential
    if (_is2022_2 || (enabled_7 == false))
    {
        WriteChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, 0);
    }

    // some constants
    WriteChannelRegister(kReg2022_6_rx_chan_timeout        + baseAddr, 0x12ffffff);
    WriteChannelRegister(kReg2022_6_rx_media_pkt_buf_size  + baseAddr, 0x0000ffff);
    if (_isIoIp)
        WriteChannelRegister(kReg2022_6_rx_media_buf_base_addr + baseAddr, 0xC0000000 + (0x10000000 * channel));    // IoIp
    else
        WriteChannelRegister(kReg2022_6_rx_media_buf_base_addr + baseAddr, 0x10000000 * channel);                   // KonaIp

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
       if (rxMatch & RX_MATCH_2022_DEST_IP)     pllMatch |= PLL_MATCH_DEST_IP;
       if (rxMatch & RX_MATCH_2022_SOURCE_IP)   pllMatch |= PLL_MATCH_SOURCE_IP;
       if (rxMatch & RX_MATCH_2022_DEST_PORT)   pllMatch |= PLL_MATCH_DEST_PORT;
       if (rxMatch & RX_MATCH_2022_SOURCE_PORT) pllMatch |= PLL_MATCH_SOURCE_PORT;
       pllMatch |= PLL_MATCH_ES_PID;    // always set for TS PCR
       mDevice.WriteRegister(kRegPll_Match   + SAREK_PLL, pllMatch);

    }

    // update IGMP subscriptions
    uint8_t ip0 = (destIp & 0xff000000)>> 24;
    if ((ip0 >= 224 && ip0 <= 239) && linkA)
    {
        // is multicast
        bool enabled = false;
        GetRxChannelEnable(channel,enabled);
        if (rxConfig.primaryRxMatch & RX_MATCH_2022_SOURCE_IP)
            SetIGMPGroup(SFP_TOP, channel, NTV2_VIDEO_STREAM, destIp, sourceIp, enabled);
        else
            SetIGMPGroup(SFP_TOP, channel, NTV2_VIDEO_STREAM, destIp, 0, enabled);
    }
    else
    {
        UnsetIGMPGroup(SFP_TOP, channel, NTV2_VIDEO_STREAM);
    }

    return rv;
}

bool  CNTV2Config2022::GetRxChannelConfiguration(const NTV2Channel channel, rx_2022_channel & rxConfig)
{
    uint32_t    baseAddr;
    uint32_t    val;
    bool        rv;

    //get link enables
    GetRxLinkState(channel,rxConfig.linkAEnable, rxConfig.linkBEnable);

    if (_is2022_7)
    {
        // Select secondary channel
        rv = SelectRxChannel(channel, SFP_BOTTOM, baseAddr);
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

        // vlan
        ReadChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, &val);
        rxConfig.secondaryVlan = val & 0xffff;

        // matching
        GetRxMatch(channel, SFP_BOTTOM, rxConfig.secondaryRxMatch);
    }
    else
    {
        rxConfig.linkAEnable = true;
        rxConfig.linkBEnable = false;
    }

    // select primary channel
    rv = SelectRxChannel(channel, SFP_TOP, baseAddr);
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
    ReadChannelRegister(kReg2022_6_rx_match_ssrc + baseAddr, &rxConfig.ssrc);

    // vlan
    ReadChannelRegister(kReg2022_6_rx_match_vlan + baseAddr, &val);
    rxConfig.primaryVlan = val & 0xffff;

    // matching
    GetRxMatch(channel, SFP_TOP, rxConfig.primaryRxMatch);

    // playout delay in ms
    ReadChannelRegister(kReg2022_6_rx_playout_delay + baseAddr,  &val);
    rxConfig.playoutDelay = (_is2022_2) ? (val >>9)/90 : val/27000;



    return true;
}

bool CNTV2Config2022::SetRxChannelEnable(const NTV2Channel channel, bool enable)
{
    uint32_t    baseAddr;
    bool        rv;
    bool        disableIGMP;

    //get link enables
    bool linkAEnable;
    bool linkBEnable;
    GetRxLinkState(channel,linkAEnable, linkBEnable);

    if (enable && linkAEnable)
    {
        if (GetLinkActive(SFP_TOP) == false)
        {
            mIpErrorCode = NTV2IpErrLinkANotConfigured;
            return false;
        }
    }

    if (enable && linkBEnable)
    {
        if (GetLinkActive(SFP_BOTTOM) == false)
        {
            mIpErrorCode = NTV2IpErrLinkBNotConfigured;
            return false;
        }
    }

    if (enable && _biDirectionalChannels)
    {
        bool txEnabled;
        GetTxChannelEnable(channel, txEnabled);
        if (txEnabled)
        {
            // disable tx channel
            SetTxChannelEnable(channel,false);
        }
        mDevice.SetSDITransmitEnable(channel, false);
    }

    if (linkBEnable)
    {
        // IGMP subscription for secondary channel
        GetIGMPDisable(SFP_BOTTOM, disableIGMP);
        if (!disableIGMP)
        {
            EnableIGMPGroup(SFP_BOTTOM,channel,NTV2_VIDEO_STREAM,enable);
        }
    }
    else
    {
        EnableIGMPGroup(SFP_BOTTOM,channel,NTV2_VIDEO_STREAM,false);
    }

    if (linkAEnable)
    {
        // IGMP subscription for primary channel
        GetIGMPDisable(SFP_TOP, disableIGMP);
        if (!disableIGMP)
        {
            EnableIGMPGroup(SFP_TOP,channel,NTV2_VIDEO_STREAM,enable);
        }
    }
    else
    {
        EnableIGMPGroup(SFP_TOP,channel,NTV2_VIDEO_STREAM,false);
    }

    rv = SelectRxChannel(channel, SFP_TOP, baseAddr);
    if (!rv) return false;
    if (enable & linkAEnable)
    {
        uint8_t match;
        GetRxMatch(channel,SFP_TOP,match);
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, (uint32_t)match);
    }
    else
    {
        // disables rx
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, 0);
    }

    rv = SelectRxChannel(channel, SFP_BOTTOM, baseAddr);
    if (!rv) return false;
    if (enable & linkBEnable)
    {
        uint8_t match;
        GetRxMatch(channel,SFP_BOTTOM,match);
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, (uint32_t)match);
    }
    else
    {
        // disables rx
        WriteChannelRegister(kReg2022_6_rx_match_sel + baseAddr, 0);
    }

    // always on
    rv = SelectRxChannel(channel, SFP_TOP, baseAddr);
    WriteChannelRegister(kReg2022_6_rx_chan_enable + baseAddr, 0x01);

    return rv;
}

bool CNTV2Config2022::GetRxChannelEnable(const NTV2Channel channel, bool & enabled)
{
    uint32_t baseAddr;

    enabled = false;

    // select primary channel
    bool rv = SelectRxChannel(channel, SFP_TOP, baseAddr);
    if (!rv) return false;

    uint32_t val;
    rv = ReadChannelRegister(kReg2022_6_rx_match_sel + baseAddr,&val);
    if (!rv) return false;
    if (val)
    {
        enabled = true;
        return true;
    }

    // select secondary channel
    rv = SelectRxChannel(channel, SFP_BOTTOM, baseAddr);
    if (!rv) return false;

    rv = ReadChannelRegister(kReg2022_6_rx_match_sel + baseAddr,&val);
    if (!rv) return false;
    if (val)
    {
        enabled = true;
        return true;
    }

    return true;
}

bool CNTV2Config2022::SetTxChannelConfiguration(const NTV2Channel channel, const tx_2022_channel & txConfig)
{
    uint32_t    baseAddr;
    uint32_t    hi;
    uint32_t    lo;


    uint32_t    destIp;

    bool        rv;

    if (txConfig.linkAEnable && (GetLinkActive(SFP_TOP) == false))
    {
        mIpErrorCode = NTV2IpErrLinkANotConfigured;
        return false;
    }

    if (txConfig.linkBEnable && (GetLinkActive(SFP_BOTTOM) == false))
    {
        mIpErrorCode = NTV2IpErrLinkBNotConfigured;
        return false;
    }


    if (_is2022_7)
    {
        // Select secondary channel
        rv = SelectTxChannel(channel, SFP_BOTTOM, baseAddr);
        if (!rv) return false;

        // hold off access while we update channel regs
        ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

        // initialise
        uint32_t val = (txConfig.tos << 8) | txConfig.ttl;
        WriteChannelRegister(kReg2022_6_tx_ip_header + baseAddr, val);
        WriteChannelRegister(kReg2022_6_tx_ssrc + baseAddr, txConfig.ssrc);

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

        // Get or generate a Mac address if we have 2022-7 enabled.
        if (txConfig.linkBEnable)
        {
            rv = GetMACAddress(SFP_BOTTOM, channel, NTV2_VIDEO_STREAM, txConfig.secondaryRemoteIP,hi,lo);
            if (!rv) return false;
            WriteChannelRegister(kReg2022_6_tx_dest_mac_low_addr + baseAddr,lo);
            WriteChannelRegister(kReg2022_6_tx_dest_mac_hi_addr  + baseAddr,hi);
        }

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);

        SetTxLinkState(channel, txConfig.linkAEnable,txConfig.linkBEnable);
    }
    else
    {
        SetTxLinkState(channel, true, false);
    }

    // select primary channel
    rv = SelectTxChannel(channel, SFP_TOP, baseAddr);
    if (!rv) return false;

    // hold off access while we update channel regs
    ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

    // initialise
    uint32_t val = (txConfig.tos << 8) | txConfig.ttl;
    WriteChannelRegister(kReg2022_6_tx_ip_header + baseAddr, val);
    WriteChannelRegister(kReg2022_6_tx_ssrc + baseAddr, txConfig.ssrc);
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

    if (txConfig.linkAEnable)
    {
        rv = GetMACAddress(SFP_TOP, channel, NTV2_VIDEO_STREAM, txConfig.primaryRemoteIP,hi,lo);
        if (!rv) return false;
        WriteChannelRegister(kReg2022_6_tx_dest_mac_low_addr + baseAddr,lo);
        WriteChannelRegister(kReg2022_6_tx_dest_mac_hi_addr  + baseAddr,hi);
    }

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
        rv = SelectTxChannel(channel, SFP_BOTTOM, baseAddr);
        if (!rv) return false;

        //get link enables
        GetTxLinkState(channel,txConfig.linkAEnable, txConfig.linkBEnable);

        ReadChannelRegister(kReg2022_6_tx_ip_header + baseAddr,&val);
        txConfig.ttl = val & 0xff;
        txConfig.tos = (val & 0xff00) >> 8;
        ReadChannelRegister(kReg2022_6_tx_ssrc + baseAddr,&txConfig.ssrc);

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
    }
    else
    {
        txConfig.linkAEnable = true;
        txConfig.linkBEnable = false;
    }
    // Select primary channel
    rv = SelectTxChannel(channel, SFP_TOP, baseAddr);
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
    return true;
}

bool CNTV2Config2022::SetTxChannelEnable(const NTV2Channel channel, bool enable)
{
    uint32_t    baseAddr;
    bool        rv;
    uint32_t    localIp;

    //get link enables
    bool linkAEnable;
    bool linkBEnable;
    GetTxLinkState(channel,linkAEnable, linkBEnable);

    if (enable && linkAEnable)
    {
        if (GetLinkActive(SFP_TOP) == false)
        {
            mIpErrorCode = NTV2IpErrLinkANotConfigured;
            return false;
        }
    }

    if (enable && linkBEnable)
    {
        if (GetLinkActive(SFP_BOTTOM) == false)
        {
            mIpErrorCode = NTV2IpErrLinkBNotConfigured;
            return false;
        }
    }

    if (_biDirectionalChannels)
    {
        bool rxEnabled;
        GetRxChannelEnable(channel,rxEnabled);
        if (rxEnabled)
        {
            // disable rx channel
            SetRxChannelEnable(channel,false);
        }
        mDevice.SetSDITransmitEnable(channel, true);
    }

    // select primary channel
    rv = SelectTxChannel(channel, SFP_TOP, baseAddr);
    if (!rv) return false;

    if (!enable)
    {
        rv = SelectTxChannel(channel, SFP_TOP, baseAddr);
        if (!rv) return false;

        WriteChannelRegister(kReg2022_6_tx_chan_enable    + baseAddr,0x0);   // disables channel
    }

    // hold off access while we update channel regs
    ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

    WriteChannelRegister(kReg2022_6_tx_hitless_config   + baseAddr,0x0);  // 0 enables hitless mode

    if (enable && linkAEnable)
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

        WriteChannelRegister(kReg2022_6_tx_link_enable   + baseAddr,0x01);  // enables tx over mac1/mac2
    }
    else
    {
            WriteChannelRegister(kReg2022_6_tx_link_enable   + baseAddr,0x00);  // enables tx over mac1/mac2
    }

    // enable  register updates
    ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);

    if (_is2022_7)
    {
        // Select secondary channel
        rv = SelectTxChannel(channel, SFP_BOTTOM, baseAddr);
        if (!rv) return false;

        // hold off access while we update channel regs
        ChannelSemaphoreClear(kReg2022_6_tx_control, baseAddr);

        WriteChannelRegister(kReg2022_6_tx_hitless_config   + baseAddr,0x0);  // 0 enables hitless mode

        if (enable && linkBEnable)
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1,&localIp);
            WriteChannelRegister(kReg2022_6_tx_src_ip_addr + baseAddr,NTV2EndianSwap32(localIp));

            // enable
            WriteChannelRegister(kReg2022_6_tx_link_enable   + baseAddr,0x01);  // enables tx over mac1/mac2
        }
        else
        {
            // disable
            WriteChannelRegister(kReg2022_6_tx_link_enable   + baseAddr,0x0);   // disables tx over mac1/mac2
        }

        // enable  register updates
        ChannelSemaphoreSet(kReg2022_6_tx_control, baseAddr);
    }

    if (enable)
    {
        SelectTxChannel(channel, SFP_TOP, baseAddr);
        WriteChannelRegister(kReg2022_6_tx_chan_enable + baseAddr,0x01);  // enables channel
    }

    return true;
}

bool CNTV2Config2022::GetTxChannelEnable(const NTV2Channel channel, bool & enabled)
{
    uint32_t baseAddr;
    uint32_t val;
    bool rv;

    enabled = false;

    rv = SelectTxChannel(channel, SFP_TOP, baseAddr);
    if (!rv) return false;

    ReadChannelRegister(kReg2022_6_tx_chan_enable + baseAddr, &val);
    if (val == 0x01)
    {
        enabled = true;
    }

    return true;
}

bool CNTV2Config2022::Set2022_7_Mode(bool enable, uint32_t rx_networkPathDifferential)
{
    if (!mDevice.IsMBSystemReady())
    {
        mIpErrorCode = NTV2IpErrNotReady;
        return false;
    }

    if (!_is2022_7)
    {
        mIpErrorCode = NTV2IpErr2022_7NotSupported;
        return false;
    }

    bool old_enable = false;
    uint32_t unused = 0;
    Get2022_7_Mode(old_enable, unused);

    bool enableChange = (old_enable != enable);

    SetDualLinkMode(enable);

    if (_numRxChans)
    {
        uint32_t baseAddr;
        SelectRxChannel(NTV2_CHANNEL1, SFP_TOP, baseAddr);
        if (enableChange)
        {
            // reset
            WriteChannelRegister(kReg2022_6_rx_reset + baseAddr, 0x01);
            WriteChannelRegister(kReg2022_6_rx_reset + baseAddr, 0x00);
        }
        if (enable)
        {
            uint32_t delay = rx_networkPathDifferential * 27000;
            // network path differential in 27MHz clocks
            WriteChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, delay);
        }
        else
        {
            WriteChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, 0);
        }
    }

    if (_numTxChans && enableChange)
    {
        // save
        uint32_t addr;
        mDevice.ReadRegister(kReg2022_6_tx_src_ip_addr + SAREK_2022_6_TX_CORE_0,&addr);

        // reset the tx core
        uint32_t baseAddr;
        SelectTxChannel(NTV2_CHANNEL1, SFP_TOP, baseAddr);
        WriteChannelRegister(kReg2022_6_tx_reset + baseAddr, 0x01);
        WriteChannelRegister(kReg2022_6_tx_reset + baseAddr, 0x00);

        // restore everything
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

        // initialise constants
        mDevice.WriteRegister(kReg2022_6_tx_sys_mem_conf     + SAREK_2022_6_TX_CORE_0, 0x04);
        mDevice.WriteRegister(kReg2022_6_tx_hitless_config   + SAREK_2022_6_TX_CORE_0, 0x01); // disable

        // source ip address
        mDevice.WriteRegister(kReg2022_6_tx_src_ip_addr      + SAREK_2022_6_TX_CORE_0,addr);

        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_low_addr + SAREK_2022_6_TX_CORE_0,boardLo);
        mDevice.WriteRegister(kReg2022_6_tx_pri_mac_hi_addr  + SAREK_2022_6_TX_CORE_0,boardHi);

        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_low_addr + SAREK_2022_6_TX_CORE_0,boardLo2);
        mDevice.WriteRegister(kReg2022_6_tx_sec_mac_hi_addr  + SAREK_2022_6_TX_CORE_0,boardHi2);
    }

    return true;
}

bool  CNTV2Config2022::Get2022_7_Mode(bool & enable, uint32_t & rx_networkPathDifferential)
{
    enable = false;
    rx_networkPathDifferential = 0;

    if (!_is2022_7)
    {
        mIpErrorCode = NTV2IpErr2022_7NotSupported;
        return false;
    }

    GetDualLinkMode(enable);

    if (_numRxChans)
    {
        // network path differential in ms
        uint32_t val;
        uint32_t baseAddr;
        SelectRxChannel(NTV2_CHANNEL1, SFP_TOP, baseAddr);
        ReadChannelRegister(kReg2022_6_rx_network_path_differential + baseAddr, &val);
        rx_networkPathDifferential = val/27000;
    }
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
        mIpErrorCode = NTV2IpErrInvalidIGMPVersion;
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
        mIpErrorCode = tsConfig.getLastErrorCode();
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
        mIpErrorCode = tsConfig.getLastErrorCode();
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
        mIpErrorCode = tsConfig.getLastErrorCode();
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
        mIpErrorCode = tsConfig.getLastErrorCode();
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


bool CNTV2Config2022::SelectRxChannel(NTV2Channel channel, eSFP link, uint32_t & baseAddr)
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

    if (link == SFP_BOTTOM)
        channelIndex |= 0x80000000;

    // select channel
    SetChannel(kReg2022_6_rx_channel_access + baseAddr, channelIndex);

    return true;
}

bool CNTV2Config2022::SelectTxChannel(NTV2Channel channel, eSFP link, uint32_t & baseAddr)
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

    if (link == SFP_BOTTOM)
        channelIndex |= 0x80000000;

    // select channel
    SetChannel(kReg2022_6_tx_channel_access + baseAddr, channelIndex);

    return true;
}

std::string CNTV2Config2022::getLastError()
{
    return NTV2IpErrorEnumToString(getLastErrorCode());
}

NTV2IpError CNTV2Config2022::getLastErrorCode()
{
    NTV2IpError error = mIpErrorCode;
    mIpErrorCode = NTV2IpErrNone;
    return error;
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


bool CNTV2Config2022::GetMACAddress(eSFP port, NTV2Channel channel, NTV2Stream stream, string remoteIP, uint32_t & hi, uint32_t & lo)
{
    uint32_t destIp = inet_addr(remoteIP.c_str());
    destIp = NTV2EndianSwap32(destIp);

    uint32_t    mac;
    MACAddr     macaddr;

     // is remote address muticast?
    uint8_t ip0 = (destIp & 0xff000000)>> 24;
    if (ip0 >= 224 && ip0 <= 239)
    {
        // multicast - generate MAC
        mac = destIp & 0x7fffff;  // lower 23 bits

        macaddr.mac[0] = 0x01;
        macaddr.mac[1] = 0x00;
        macaddr.mac[2] = 0x5e;
        macaddr.mac[3] =  mac >> 16;
        macaddr.mac[4] = (mac & 0xffff) >> 8;
        macaddr.mac[5] =  mac & 0xff;
    }
    else
    {
        // unicast - get MAC from ARP
        string macAddr;
        bool rv;
        // is destination on the same subnet?
        IPVNetConfig nc;
        GetNetworkConfiguration(port,nc);
        if ( (destIp & nc.ipc_subnet) != (nc.ipc_ip & nc.ipc_subnet))
        {
            struct in_addr addr;
            addr.s_addr  = NTV2EndianSwap32(nc.ipc_gateway);
            string gateIp = inet_ntoa(addr);
            rv = GetRemoteMAC(gateIp, port, channel, stream, macAddr);
        }
        else
        {
            rv = GetRemoteMAC(remoteIP, port, channel, stream, macAddr);
        }
        if (!rv)
        {
            SetTxChannelEnable(channel, false); // stop transmit
            mIpErrorCode = NTV2IpErrCannotGetMacAddress;
            return false;
        }

        istringstream ss(macAddr);
        string token;
        int i=0;
        while (i < 6)
        {
            getline (ss, token, ':');
            macaddr.mac[i++] = (uint8_t)strtoul(token.c_str(),NULL,16);
        }
    }

    hi  = macaddr.mac[0]  << 8;
    hi += macaddr.mac[1];

    lo  = macaddr.mac[2] << 24;
    lo += macaddr.mac[3] << 16;
    lo += macaddr.mac[4] << 8;
    lo += macaddr.mac[5];

    return true;
}

bool CNTV2Config2022::GetSFPMSAData(eSFP port, SFPMSAData & data)
{
    return GetSFPInfo(port,data);
}

bool CNTV2Config2022::GetLinkStatus(eSFP port, sLinkStatus & linkStatus)
{
    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkStatus,&val);
    uint32_t val2;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekSFPStatus,&val2);

    if (port == SFP_BOTTOM)
    {
        linkStatus.linkUp          = (val  & LINK_B_UP) ? true : false;
        linkStatus.SFP_present     = (val2 & SFP_2_NOT_PRESENT) ? false : true;
        linkStatus.SFP_rx_los      = (val2 & SFP_2_RX_LOS) ? true : false;
        linkStatus.SFP_tx_fault    = (val2 & SFP_2_TX_FAULT) ? true : false;
    }
    else
    {
        linkStatus.linkUp          = (val  & LINK_A_UP) ? true : false;
        linkStatus.SFP_present     = (val2 & SFP_1_NOT_PRESENT) ? false : true;
        linkStatus.SFP_rx_los      = (val2 & SFP_1_RX_LOS) ? true : false;
        linkStatus.SFP_tx_fault    = (val2 & SFP_1_TX_FAULT) ? true : false;
    }

    return true;
}

bool CNTV2Config2022:: Get2022ChannelRxStatus(NTV2Channel channel, s2022RxChannelStatus & chanStatus)
{
    uint32_t addr;

    SelectRxChannel(channel,SFP_BOTTOM,addr);
    ReadChannelRegister(addr + kReg2022_6_rx_sec_recv_pkt_cnt, &chanStatus.secondaryRxPackets);
    ReadChannelRegister(addr + kReg2022_6_rx_link_valid_media_pkt_cnt, &chanStatus.secondaryValidRxPackets);

    SelectRxChannel(channel,SFP_TOP,addr);
    ReadChannelRegister(addr + kReg2022_6_rx_pri_recv_pkt_cnt, &chanStatus.primaryRxPackets);
    ReadChannelRegister(addr + kReg2022_6_rx_link_valid_media_pkt_cnt, &chanStatus.primaryValidRxPackets);

    return true;
}
