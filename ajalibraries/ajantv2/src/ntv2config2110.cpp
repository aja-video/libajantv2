/**
    @file		ntv2config2110.cpp
    @brief		Implements the CNTV2Config2110 class.
    @copyright	(C) 2014-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2config2110.h"
#include "ntv2endian.h"
#include "ntv2card.h"
#include "ntv2formatdescriptor.h"
#include <sstream>

#if defined (AJALinux) || defined (AJAMac)
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using namespace std;

void tx_2110Config_stream::init()
{
    localPort    = 0;
    remoteIP.erase();
    remotePort   = 0;
    autoMAC      = false;
    memset(remoteMAC.mac, 0, sizeof(MACAddr));
}

bool tx_2110Config_stream::eq_MACAddr(const MACAddr& a)
{
    return (memcmp(remoteMAC.mac, a.mac, 6) == 0);
}

bool tx_2110Config_stream::operator != ( const tx_2110Config_stream &other )
{
    return !(*this == other);
}

bool tx_2110Config_stream::operator == ( const tx_2110Config_stream &other )
{
    if ((localPort       == other.localPort)      &&
        (remoteIP        == other.remoteIP)       &&
        (remotePort      == other.remotePort)     &&
        (autoMAC         == other.autoMAC)        &&
        (eq_MACAddr(other.remoteMAC)))

    {
        return true;
    }
    else
    {
        return false;
    }
}

void rx_2110Config_stream::init()
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

bool rx_2110Config_stream::operator != ( const rx_2110Config_stream &other )
{
    return !(*this == other);
}

bool rx_2110Config_stream::operator == ( const rx_2110Config_stream &other )
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

void rx2110Config::init()
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

bool rx2110Config::operator != ( const rx2110Config &other )
{
    return (!(*this == other));
}

bool rx2110Config::operator == ( const rx2110Config &other )
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

void tx2110Config::init()
{
    txc_enable       = 0;
    txc_localPort    = 0;
    txc_remoteIp     = 0;
    txc_remotePort   = 0;
    txc_remoteMAC_lo = 0;
    txc_remoteMAC_hi = 0;
    txc_autoMac      = 0;
}

bool tx2110Config::operator != ( const tx2110Config &other )
{
    return (!(*this == other));
}

bool tx2110Config::operator == ( const tx2110Config &other )
{
    if ((txc_enable                == other.txc_enable)         &&
            (txc_localPort         == other.txc_localPort)      &&
            (txc_remoteIp          == other.txc_remoteIp)       &&
            (txc_remotePort        == other.txc_remotePort)     &&
            (txc_remoteMAC_lo      == other.txc_remoteMAC_lo)   &&
            (txc_remoteMAC_hi      == other.txc_remoteMAC_hi)   &&
            (txc_autoMac           == other.txc_autoMac))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////////////
//
//  CNTV2Config2110
//
//////////////////////////////////////////////////////////////////////////////////

CNTV2Config2110::CNTV2Config2110(CNTV2Card & device) : CNTV2MBController(device)
{
    uint32_t features    = getFeatures();

    _numTx0Chans = (features & (SAREK_TX0_MASK)) >> 28;
    _numRx0Chans = (features & (SAREK_RX0_MASK)) >> 24;
    _numTx1Chans = (features & (SAREK_TX1_MASK)) >> 20;
    _numRx1Chans = (features & (SAREK_RX1_MASK)) >> 16;

    _numRxChans  = _numRx0Chans + _numRx1Chans;
    _numTxChans  = _numTx0Chans + _numTx1Chans;

    _biDirectionalChannels = false;
}

CNTV2Config2110::~CNTV2Config2110()
{
}

bool CNTV2Config2110::SetNetworkConfiguration(eSFP port, const IPVNetConfig & netConfig)
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

bool CNTV2Config2110::SetNetworkConfiguration (eSFP port, string localIPAddress, string netmask, string gateway)
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
#if 0
    // get secondary mac address
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, &macHi);
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, &macLo);

    uint32_t boardHi2 = (macHi & 0xffff0000) >>16;
    uint32_t boardLo2 = ((macHi & 0x0000ffff) << 16) + ((macLo & 0xffff0000) >> 16);
#endif

    uint32_t core;

    if (port == SFP_TOP)
    {
        core = SAREK_2110_FRAMER_1_TOP;
    }
    else
    {
        core = SAREK_2110_FRAMER_2_BOT;
    }

    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi);

    bool rv = AcquireMailbox();
    if (rv)
    {
        rv = SetMBNetworkConfiguration (port, localIPAddress, netmask, gateway);
        ReleaseMailbox();
    }
    return rv;
}

bool CNTV2Config2110::SetNetworkConfiguration (string localIPAddress0, string netmask0, string gateway0,
                                               string localIPAddress1, string netmask1, string gateway1)
{

    SetNetworkConfiguration(SFP_TOP, localIPAddress0, netmask0, gateway0);
    SetNetworkConfiguration(SFP_BOTTOM, localIPAddress1, netmask1, gateway1);

    return true;
}

bool CNTV2Config2110::GetNetworkConfiguration(eSFP port, IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    GetNetworkConfiguration(port, ip, subnet, gateway);

    netConfig.ipc_ip      = NTV2EndianSwap32((uint32_t)inet_addr(ip.c_str()));
    netConfig.ipc_subnet  = NTV2EndianSwap32((uint32_t)inet_addr(subnet.c_str()));
    netConfig.ipc_gateway = NTV2EndianSwap32((uint32_t)inet_addr(gateway.c_str()));

    return true;
}

bool CNTV2Config2110::GetNetworkConfiguration(eSFP port, string & localIPAddress, string & subnetMask, string & gateway)
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

bool CNTV2Config2110::GetNetworkConfiguration(std::string & localIPAddress0, std::string & subnetMask0, std::string & gateway0,
                                              std::string & localIPAddress1, std::string & subnetMask1, std::string & gateway1)
{

    GetNetworkConfiguration(SFP_TOP, localIPAddress0, subnetMask0, gateway0);
    GetNetworkConfiguration(SFP_BOTTOM, localIPAddress1, subnetMask1, gateway1);

    return true;
}

bool CNTV2Config2110::SetRxChannelConfiguration(const NTV2Channel channel,const rx_2110Config_stream &rxConfig)
{
#if 0
    uint32_t    baseAddr;
    bool        rv;

    // select primary channel
    rv = SelectRxChannel(channel, true, baseAddr);
    if (!rv) return false;

    // hold off access while we update channel regs
    AcquireFramerCtrlAccess(baseAddr);

    // source ip address
    uint32_t sourceIp = inet_addr(rxConfig.primarySourceIP.c_str());
    sourceIp = NTV2EndianSwap32(sourceIp);
    WriteChannelRegister(kReg2110_6_rx_match_src_ip_addr + baseAddr, sourceIp);

    // dest ip address
    uint32_t destIp = inet_addr(rxConfig.primaryDestIP.c_str());
    destIp = NTV2EndianSwap32(destIp);
    WriteChannelRegister(kReg2110_6_rx_match_dest_ip_addr + baseAddr, destIp);

    uint8_t ip0 = (destIp & 0xff000000)>> 24;
    int offset = (int)channel;
    if (ip0 >= 224 && ip0 <= 239)
    {
        // is multicast
        mDevice.WriteRegister(kRegSarekIGMP0 + offset + SAREK_REGS, destIp);
    }
    else
    {
        mDevice.WriteRegister(kRegSarekIGMP0 + offset + SAREK_REGS, 0);
    }

    // source port
    WriteChannelRegister(kReg2110_6_rx_match_src_port + baseAddr, rxConfig.primarySourcePort);

    // dest port
    WriteChannelRegister(kReg2110_6_rx_match_dest_port + baseAddr, rxConfig.primaryDestPort);

    // ssrc
    WriteChannelRegister(kReg2110_6_rx_match_ssrc + baseAddr, rxConfig.primarySsrc);

    // vlan
    WriteChannelRegister(kReg2110_6_rx_match_vlan + baseAddr, rxConfig.primaryVlan);

    // matching
    WriteChannelRegister(kReg2110_6_rx_match_sel + baseAddr, rxConfig.primaryRxMatch);

    // playout delay in 27MHz clocks or 90kHz clocks
    uint32_t delay = (_is2110_2) ? (rxConfig.playoutDelay * 90) << 9 : rxConfig.playoutDelay * 27000;
    WriteChannelRegister(kReg2110_6_rx_playout_delay + baseAddr, delay);

    // network path differential in 27MHz or 90kHz clocks
    delay = (_is2110_2) ? (rxConfig.networkPathDiff * 90) << 9 : rxConfig.networkPathDiff * 27000;
    WriteChannelRegister(kReg2110_6_rx_network_path_differential + baseAddr, delay);

    // some constants
    WriteChannelRegister(kReg2110_6_rx_chan_timeout        + baseAddr, 0x0000ffff);
    WriteChannelRegister(kReg2110_6_rx_media_pkt_buf_size  + baseAddr, 0x0000ffff);
    WriteChannelRegister(kReg2110_6_rx_media_buf_base_addr + baseAddr, 0x10000000 * channel);

    // enable  register updates
    ChannelSemaphoreSet(kReg2110_6_rx_control, baseAddr);

    if (_is2110_2)
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

    // if already enabled, make sure IGMP subscriptions are updated
    bool enabled = false;
    GetRxChannelEnable(channel,enabled);
    if (enabled)
    {
        eSFP port = GetRxPort(channel);
        rv = AcquireMailbox();
        if (rv)
        {
            rv = JoinIGMPGroup(port, channel, rxConfig.primaryDestIP);
            ReleaseMailbox();
        }
    }
    return rv;
#endif
    return false;
}

bool  CNTV2Config2110::GetRxChannelConfiguration(const NTV2Channel channel, rx_2110Config_stream & rxConfig)
{
#if 0
    uint32_t    baseAddr;
    uint32_t    val;
    bool        rv;

    if (_is2110_7)
    {
        // Select secondary channel
        rv = SelectRxChannel(channel, false, baseAddr);
        if (!rv) return false;

        // source ip address
        ReadChannelRegister(kReg2110_6_rx_match_src_ip_addr + baseAddr, &val);
        struct in_addr in;
        in.s_addr = NTV2EndianSwap32(val);
        char * ip = inet_ntoa(in);
        rxConfig.secondarySourceIP = ip;

        // dest ip address
        ReadChannelRegister(kReg2110_6_rx_match_dest_ip_addr + baseAddr, &val);
        in.s_addr = NTV2EndianSwap32(val);
        ip = inet_ntoa(in);
        rxConfig.secondaryDestIP = ip;

        // source port
        ReadChannelRegister(kReg2110_6_rx_match_src_port + baseAddr, &rxConfig.secondarySourcePort);

        // dest port
        ReadChannelRegister(kReg2110_6_rx_match_dest_port + baseAddr, &rxConfig.secondaryDestPort);

        // ssrc
        ReadChannelRegister(kReg2110_6_rx_match_ssrc + baseAddr, &rxConfig.secondarySsrc);       // PSM fix this, this is a shared reg

        // vlan
        ReadChannelRegister(kReg2110_6_rx_match_vlan + baseAddr, &val);
        rxConfig.secondaryVlan = val & 0xffff;

        // matching
        ReadChannelRegister(kReg2110_6_rx_match_sel + baseAddr, &rxConfig.secondaryRxMatch);
    }

    // select primary channel
    rv = SelectRxChannel(channel, true, baseAddr);
    if (!rv) return false;

    // source ip address
    ReadChannelRegister(kReg2110_6_rx_match_src_ip_addr + baseAddr, &val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    rxConfig.primarySourceIP = ip;

    // dest ip address
    ReadChannelRegister(kReg2110_6_rx_match_dest_ip_addr + baseAddr, &val);
    in.s_addr = NTV2EndianSwap32(val);
    ip = inet_ntoa(in);
    rxConfig.primaryDestIP = ip;

    // source port
    ReadChannelRegister(kReg2110_6_rx_match_src_port + baseAddr, &rxConfig.primarySourcePort);

    // dest port
    ReadChannelRegister(kReg2110_6_rx_match_dest_port + baseAddr, &rxConfig.primaryDestPort);

    // ssrc
    ReadChannelRegister(kReg2110_6_rx_match_ssrc + baseAddr, &rxConfig.primarySsrc);

    // vlan
    ReadChannelRegister(kReg2110_6_rx_match_vlan + baseAddr, &val);
    rxConfig.primaryVlan = val & 0xffff;

    // matching
    ReadChannelRegister(kReg2110_6_rx_match_sel + baseAddr, &rxConfig.primaryRxMatch);

    // playout delay in ms
    ReadChannelRegister(kReg2110_6_rx_playout_delay + baseAddr,  &val);
    rxConfig.playoutDelay = (_is2110_2) ? (val >>9)/90 : val/27000;

    // network path differential in ms
    ReadChannelRegister(kReg2110_6_rx_network_path_differential + baseAddr, &val);
    rxConfig.playoutDelay = (_is2110_2) ? (val>>9)/90 : val/27000;

    return true;
#endif
    return false;
}

bool CNTV2Config2110::SetRxChannelEnable(const NTV2Channel channel, bool enable, bool enable2110_7)
{
#if 0
    uint32_t    baseAddr;
    bool        rv;
    bool        disableIGMP;
    eSFP        port;
    uint32_t    ip;
    int         offset;
    struct      sockaddr_in sin;

    if (enable && _biDirectionalChannels)
    {
        bool txEnabled;
        GetTxChannelEnable(channel,txEnabled);
        if (txEnabled)
        {
            // disable tx channel
            SetTxChannelEnable(channel,false, enable2110_7);
        }
        mDevice.SetSDITransmitEnable(channel, false);
    }

    // select primary channel
    rv = SelectRxChannel(channel, true, baseAddr);
    if (!rv) return false;

    if (_is2110_7 && enable2110_7)
    {
        // IGMP subscription for secondary channel
        port = SFP_BOTTOM;
        GetIGMPDisable(port, disableIGMP);

        if (!disableIGMP)
        {
            // join/leave multicast group if necessary
            offset = (int)channel;
            mDevice.ReadRegister(kRegSarekIGMP4 + offset + SAREK_REGS, &ip);
            if (ip != 0)
            {
                // is mutlicast
                sin.sin_addr.s_addr = NTV2EndianSwap32(ip);
                char * ipaddr = inet_ntoa(sin.sin_addr);

                rv = AcquireMailbox();
                if (rv)
                {
                    if (enable)
                    {
                        //rv = JoinIGMPGroup(port, (NTV2Channel)(int)(channel+2), ipaddr);
                        rv = JoinIGMPGroup(port, channel, ipaddr);
                    }
                    else
                    {
                        //rv = LeaveIGMPGroup(port, (NTV2Channel)(int)(channel+2), ipaddr);
                        rv = LeaveIGMPGroup(port, channel, ipaddr);
                    }
                    ReleaseMailbox();
                }
            }
        }
    }

    // IGMP subscription for primary channel
    port = GetRxPort(channel);
    GetIGMPDisable(port, disableIGMP);

    if (!disableIGMP)
    {
        // join/leave multicast group if necessary
        offset = (int)channel;
        mDevice.ReadRegister(kRegSarekIGMP0 + offset + SAREK_REGS, &ip);
        if (ip != 0)
        {
            // is mutlicast
            sin.sin_addr.s_addr = NTV2EndianSwap32(ip);
            char * ipaddr = inet_ntoa(sin.sin_addr);

            rv = AcquireMailbox();
            if (rv)
            {
                if (enable)
                {
                    rv = JoinIGMPGroup(port, channel, ipaddr);
                }
                else
                {
                    rv = LeaveIGMPGroup(port, channel, ipaddr);
                }
                ReleaseMailbox();
                // continue but return error code
            }
        }
    }

    if (enable)
    {
        WriteChannelRegister(kReg2110_6_rx_chan_enable + baseAddr, 0x01);
    }
    else
    {
        WriteChannelRegister(kReg2110_6_rx_chan_enable + baseAddr, 0x0);
    }
    return rv;
#endif
    return false;
}

bool CNTV2Config2110::GetRxChannelEnable(const NTV2Channel channel, bool & enabled)
{
#if 0
    uint32_t baseAddr;

    // select primary channel
    bool rv = SelectRxChannel(channel, true, baseAddr);
    if (!rv) return false;

    uint32_t val;
    rv = ReadChannelRegister(kReg2110_6_rx_chan_enable + baseAddr,&val);
    if (rv)
    {
        enabled = (val != 0x00);
    }
    return rv;
#endif
    return false;
}

int _lcm(int a,int b)
{
    int m = a;
    int n = b;
    while (m != n)
    {
        if (m < n)
            m += a;
        else
            n +=b;
    }
    return m;
}

bool CNTV2Config2110::SetTxChannelConfiguration(const NTV2Channel channel, uint32_t stream, const tx_2110Config_stream & txConfig)
{
    uint32_t    baseAddrFramer;
    uint32_t    val;
    uint32_t    hi;
    uint32_t    lo;
    MACAddr     macaddr;
    uint8_t     ip0;
    uint32_t    destIp;
    uint32_t    mac;
    bool        rv;

    // select channel
    rv = SelectTxChannel(channel, stream, baseAddrFramer);
    if (!rv) return false;

    // setup framer
    // hold off access while we update channel regs
    AcquireFramerControlAccess(baseAddrFramer);

    // dest ip address
    destIp = inet_addr(txConfig.remoteIP.c_str());
    destIp = NTV2EndianSwap32(destIp);
    WriteChannelRegister(kRegFramer_dst_ip + baseAddrFramer,destIp);

    // source port
    WriteChannelRegister(kRegFramer_udp_src_port + baseAddrFramer,txConfig.localPort);

    // dest port
    WriteChannelRegister(kRegFramer_udp_dst_port + baseAddrFramer,txConfig.remotePort);

    // auto MAC setting
    uint32_t autoMacReg;
    mDevice.ReadRegister(kRegSarekTxAutoMAC + SAREK_REGS,&autoMacReg);
    if (txConfig.autoMAC)
    {
        autoMacReg |= (1 << channel);
    }
    else
    {
        autoMacReg &= ~(1 << channel);
    }
    mDevice.WriteRegister(kRegSarekTxAutoMAC + SAREK_REGS,autoMacReg);

    // dest MAC
    if (txConfig.autoMAC)
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
                rv = GetRemoteMAC(txConfig.remoteIP,macAddr);
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
        hi  = txConfig.remoteMAC.mac[0]  << 8;
        hi += txConfig.remoteMAC.mac[1];

        lo  = txConfig.remoteMAC.mac[2] << 24;
        lo += txConfig.remoteMAC.mac[3] << 16;
        lo += txConfig.remoteMAC.mac[4] << 8;
        lo += txConfig.remoteMAC.mac[5];
    }

    WriteChannelRegister(kRegFramer_udp_dst_port  + baseAddrFramer,lo);
    WriteChannelRegister(kRegFramer_dest_mac_hi  + baseAddrFramer,hi);

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // end framer setup

    // setup 4175 packetizer

    uint32_t baseAddrPacketizer = SAREK_4175_TX_PACKETIZER_1 + (0x1000 * (int)channel);
    NTV2VideoFormat fmt = txConfig.videoFormat;
    bool interlaced = !NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(fmt);
    NTV2FormatDescriptor fd(fmt,NTV2_FBF_10BIT_YCBCR);

    // width
    uint32_t width = fd.GetRasterWidth();
    mDevice.WriteRegister(kReg2110_pkt_width + baseAddrPacketizer,width);

    // height
    uint32_t height = fd.GetRasterHeight();
    mDevice.WriteRegister(kReg2110_pkt_height + baseAddrPacketizer,height);

    // video format = sampling
    int vf;
    int componentsPerPixel;
    int componentsPerUnit;

    VPIDSampling vs = txConfig.videoSamples;
    switch(vs)
    {
    case VPIDSampling_GBR_444:
        vf = 0;
        componentsPerPixel = 3;
        componentsPerUnit  = 3;
        break;
    case VPIDSampling_YUV_444:
        vf = 1;
        componentsPerPixel = 3;
        componentsPerUnit  = 3;
        break;
    default:
    case VPIDSampling_YUV_422:
        componentsPerPixel = 2;
        componentsPerUnit  = 4;

        vf = 2;
        break;
    }
    mDevice.WriteRegister(kReg2110_pkt_vid_fmt + baseAddrPacketizer,vf);

    const int bitsPerComponent = 10;
    const int pixelsPerClock = 1;
    int activeLine_root    = width * componentsPerPixel * bitsPerComponent;
    int activeLineLength   = activeLine_root/8;
    int pixelGroup_root    = bitsPerComponent * componentsPerUnit;
    int pixelGroupSize     = pixelGroup_root/8;
    int bytesPerCycle_root = pixelsPerClock * bitsPerComponent * componentsPerPixel;
    int bytesPerCycle      = bytesPerCycle_root/8;
    int lcm                = _lcm(pixelGroup_root,bytesPerCycle_root)/8;
    int payloadLength_root =  min(activeLineLength,1376)/lcm;
    int payloadLength      = payloadLength_root * lcm;
    float pktsPerLine      = ((float)activeLineLength)/((float)payloadLength);
    int ipktsPerLine       = (int)ceil(pktsPerLine);
    int payloadLengthLast  = activeLineLength - (payloadLength * (ipktsPerLine -1));

    // pkts per line
    mDevice.WriteRegister(kReg2110_pkt_pkts_per_line + baseAddrPacketizer,ipktsPerLine);

    // payload length
    mDevice.WriteRegister(kReg2110_pkt_payload_len + baseAddrPacketizer,payloadLength);

    // payload length last
    mDevice.WriteRegister(kReg2110_pkt_payload_len_last + baseAddrPacketizer,payloadLengthLast);

    // payload type
    mDevice.WriteRegister(kReg2110_pkt_payload_type + baseAddrPacketizer,100);

    // channel/stream number
    mDevice.WriteRegister(kReg2110_pkt_chan_num + baseAddrPacketizer,stream);

    // pix per pkt
    int ppp = (payloadLength/pixelGroupSize) * 2;   // as per JeffL
    mDevice.WriteRegister(kReg2110_pkt_pix_per_pkt + baseAddrPacketizer,ppp);

    // interlace
    int ilace = (interlaced) ? 0x01 : 0x00;
    mDevice.WriteRegister(kReg2110_pkt_interlace_ctrl + baseAddrPacketizer,ilace);

    // end setup 4175 packetizer
    return rv;
}

bool CNTV2Config2110::GetTxChannelConfiguration(const NTV2Channel channel, uint32_t stream, tx_2110Config_stream & txConfig)
{
    uint32_t    baseAddrFramer;
    uint32_t    val;
    bool        rv;

    // Select channel
    rv = SelectTxChannel(channel, stream, baseAddrFramer);
    if (!rv) return false;

    // dest ip address
    ReadChannelRegister(kRegFramer_dst_ip + baseAddrFramer,&val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    txConfig.remoteIP = ip;

    // source port
    ReadChannelRegister(kRegFramer_udp_src_port + baseAddrFramer,&txConfig.localPort);

    // dest port
    ReadChannelRegister(kRegFramer_udp_dst_port + baseAddrFramer,&txConfig.remotePort);

    // dest MAC
    uint32_t hi;
    uint32_t lo;
    ReadChannelRegister(kRegFramer_udp_dst_port + baseAddrFramer, &lo);
    ReadChannelRegister(kRegFramer_dest_mac_hi  + baseAddrFramer, &hi);

    txConfig.remoteMAC.mac[0] = (hi >> 8) & 0xff;
    txConfig.remoteMAC.mac[1] =  hi        & 0xff;
    txConfig.remoteMAC.mac[2] = (lo >> 24) & 0xff;
    txConfig.remoteMAC.mac[3] = (lo >> 16) & 0xff;
    txConfig.remoteMAC.mac[4] = (lo >> 8)  & 0xff;
    txConfig.remoteMAC.mac[5] =  lo        & 0xff;

    mDevice.ReadRegister(kRegSarekTxAutoMAC + SAREK_REGS,&val);
    txConfig.autoMAC = ((val & (1 << channel)) != 0);

    return true;
}

bool CNTV2Config2110::SetTxChannelEnable(const NTV2Channel channel, uint32_t stream, bool enable)
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
            SetRxChannelEnable(channel,stream,false);
        }
        mDevice.SetSDITransmitEnable(channel, true);
    }

    // select channel
    rv = SelectTxChannel(channel, stream, baseAddr);
    if (!rv) return false;

    // hold off access while we update channel regs
    AcquireFramerControlAccess(baseAddr);

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
        WriteChannelRegister(kRegFramer_src_ip + baseAddr,NTV2EndianSwap32(localIp));

        // enable
        WriteChannelRegister(kRegFramer_chan_ctrl   + baseAddr,0x01);  // enables tx over mac1/mac2
    }
    else
    {
        // disable
        WriteChannelRegister(kRegFramer_chan_ctrl    + baseAddr,0x0);   // disables channel
    }

    // enable  register updates
    ReleaseFramerControlAccess(baseAddr);

    return true;
}

bool CNTV2Config2110::GetTxChannelEnable(const NTV2Channel channel, uint32_t stream, bool & enabled)
{
    uint32_t baseAddr;

    // select primary channel
    bool rv = SelectTxChannel(channel, stream, baseAddr);
    if (!rv) return false;

    uint32_t val;
    ReadChannelRegister(kRegFramer_chan_ctrl + baseAddr, &val);
    enabled = (val == 0x01);

    return true;
}

bool CNTV2Config2110::SetIGMPDisable(eSFP port, bool disable)
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

bool CNTV2Config2110::GetIGMPDisable(eSFP port, bool & disabled)
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

bool CNTV2Config2110::SetIGMPVersion(eIGMPVersion_t version)
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

bool CNTV2Config2110::GetIGMPVersion(eIGMPVersion_t & version)
{
    uint32_t version32;
    bool rv = mDevice.ReadRegister(SAREK_REGS + kRegSarekIGMPVersion,&version32);
    version =  (version32 == 2) ? eIGMPVersion_2 : eIGMPVersion_3;
    return rv;
}

/////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////

eSFP  CNTV2Config2110::GetRxPort(NTV2Channel chan)
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

eSFP  CNTV2Config2110::GetTxPort(NTV2Channel chan)
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


bool CNTV2Config2110::SelectRxChannel(NTV2Channel channel, bool primaryChannel, uint32_t & baseAddr)
{
#if 0
    uint32_t iChannel = (uint32_t) channel;
    uint32_t channelIndex = iChannel;

    if (iChannel > _numRxChans)
        return false;

    if (_is2110_6)
    {
        if (iChannel >= _numRx0Chans)
        {
            channelIndex = iChannel - _numRx0Chans;
            baseAddr  = SAREK_2110_6_RX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2110_6_RX_CORE_0;
        }
    }
    else
    {
        if (iChannel >= _numRx0Chans)
        {
            channelIndex = iChannel - _numRx0Chans;
            baseAddr  = SAREK_2110_2_RX_CORE_1;
        }
        else
        {
            channelIndex = iChannel;
            baseAddr  = SAREK_2110_2_RX_CORE_0;
        }
    }

    uint32_t channelPS = 0;
    if (!primaryChannel)
        channelPS = 0x80000000;

    // select channel
    SetChannel(kReg2110_6_rx_channel_access + baseAddr, channelIndex, channelPS);
#endif
    return true;
}

bool CNTV2Config2110::SelectTxChannel(NTV2Channel channel, uint32_t stream, uint32_t & baseAddrFramer)
{
    uint32_t iChannel = (uint32_t) channel;
    uint32_t channelIndex = iChannel;

    if (iChannel > _numTxChans)
        return false;

    if (iChannel >= _numTx0Chans)
    {
        channelIndex = iChannel - _numTx0Chans;
        baseAddrFramer  = SAREK_2110_FRAMER_2_BOT;
    }
    else
    {
        channelIndex = iChannel;
        baseAddrFramer  = SAREK_2110_FRAMER_1_TOP;
    }

    // select channel
    SetChannel(kRegFramer_channel_access + baseAddrFramer, stream);

    return true;
}

bool  CNTV2Config2110::ConfigurePTP (eSFP port, string localIPAddress)
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

string CNTV2Config2110::getLastError()
{
    string astring = mError;
    mError.clear();
    return astring;
}

void CNTV2Config2110::AcquireFramerControlAccess(uint32_t baseAddr)
{
    uint32_t val;
    WriteChannelRegister(kRegFramer_control + baseAddr, 0x00);
    // DAC TODO - wait for access

}

void CNTV2Config2110::ReleaseFramerControlAccess(uint32_t baseAddr)
{
    WriteChannelRegister(kRegFramer_control + baseAddr, 0x02);
}

