/**
    @file		ntv2config2022.cpp
    @brief		Implements the CNTV2Config2022 class.
	@copyright	(C) 2014-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
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

void reset_rx2022Config(rx2022Config& structure)
{
    structure.rxc_enable  = 0;
    structure.rxc_primaryRxMatch = 0;
    structure.rxc_primarySourceIp = 0;
    structure.rxc_primaryDestIp = 0;
    structure.rxc_primarySourcePort = 0;
    structure.rxc_primaryDestPort  = 0;
    structure.rxc_primarySsrc = 0;
    structure.rxc_primaryVlan = 0;
    structure.rxc_secondaryRxMatch = 0;
    structure.rxc_secondarySourceIp = 0;
    structure.rxc_secondaryDestIp = 0;
    structure.rxc_secondarySourcePort = 0;
    structure.rxc_secondaryDestPort  = 0;
    structure.rxc_secondarySsrc = 0;
    structure.rxc_secondaryVlan = 0;
    structure.rxc_networkPathDiff = 50;
    structure.rxc_playoutDelay = 50;
}

void reset_tx2022Config(tx2022Config& structure)
{
    structure.txc_enable  = 0;
    structure.txc_primaryLocalPort = 0;
    structure.txc_primaryRemoteIp = 0;
    structure.txc_primaryRemotePort = 0;
    structure.txc_primaryRemoteMAC_lo = 0;
    structure.txc_primaryRemoteMAC_hi  = 0;
    structure.txc_primaryAutoMac = 0;
    structure.txc_secondaryLocalPort = 0;
    structure.txc_secondaryRemoteIp = 0;
    structure.txc_secondaryRemotePort = 0;
    structure.txc_secondaryRemoteMAC_lo = 0;
    structure.txc_secondaryRemoteMAC_hi = 0;
    structure.txc_secondaryAutoMac  = 0;
}

void reset_IPVNetConfig(IPVNetConfig &structure)
{
    structure.ipc_gateway = 0;
    structure.ipc_ip = 0;
    structure.ipc_subnet = 0;
}

AJAExport bool equal_tx2022Config(const tx2022Config& a, const tx2022Config& b)
{
    if ((a.txc_enable                   == b.txc_enable)                &&
        (a.txc_primaryLocalPort         == b.txc_primaryLocalPort)      &&
        (a.txc_primaryRemoteIp          == b.txc_primaryRemoteIp)       &&
        (a.txc_primaryRemotePort        == b.txc_primaryRemotePort)     &&
        (a.txc_primaryRemoteMAC_lo      == b.txc_primaryRemoteMAC_lo)   &&
        (a.txc_primaryRemoteMAC_hi      == b.txc_primaryRemoteMAC_hi)   &&
        (a.txc_primaryAutoMac           == b.txc_primaryAutoMac)        &&
        (a.txc_secondaryLocalPort       == b.txc_secondaryLocalPort)    &&
        (a.txc_secondaryRemoteIp        == b.txc_secondaryRemoteIp)     &&
        (a.txc_secondaryRemotePort      == b.txc_secondaryRemotePort)   &&
        (a.txc_secondaryRemoteMAC_lo    == b.txc_secondaryRemoteMAC_lo) &&
        (a.txc_secondaryRemoteMAC_hi    == b.txc_secondaryRemoteMAC_hi) &&
        (a.txc_secondaryAutoMac         == b.txc_secondaryAutoMac))
    {
        return true;
    }
    else
    {
        return false;
    }
}

AJAExport bool equal_rx2022Config(const rx2022Config& a, const rx2022Config& b)
{
    if ((a.rxc_enable                   == b.rxc_enable)                &&
        (a.rxc_primaryRxMatch           == b.rxc_primaryRxMatch)        &&
        (a.rxc_primarySourceIp          == b.rxc_primarySourceIp)       &&
        (a.rxc_primaryDestIp            == b.rxc_primaryDestIp)         &&
        (a.rxc_primarySourcePort        == b.rxc_primarySourcePort)     &&
        (a.rxc_primaryDestPort          == b.rxc_primaryDestPort)       &&
        (a.rxc_primarySsrc              == b.rxc_primarySsrc)           &&
        (a.rxc_primaryVlan              == b.rxc_primaryVlan)           &&
        (a.rxc_secondaryRxMatch         == b.rxc_secondaryRxMatch)      &&
        (a.rxc_secondarySourceIp        == b.rxc_secondarySourceIp)     &&
        (a.rxc_secondaryDestIp          == b.rxc_secondaryDestIp)       &&
        (a.rxc_secondarySourcePort      == b.rxc_secondarySourcePort)   &&
        (a.rxc_secondaryDestPort        == b.rxc_secondaryDestPort)     &&
        (a.rxc_secondarySsrc            == b.rxc_secondarySsrc)         &&
        (a.rxc_secondaryVlan            == b.rxc_secondaryVlan)         &&
        (a.rxc_networkPathDiff          == b.rxc_networkPathDiff)       &&
        (a.rxc_playoutDelay             == b.rxc_playoutDelay))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool equal_IPVNetConfig(const IPVNetConfig& a, const IPVNetConfig& b)
{
    if ((a.ipc_gateway  == b.ipc_gateway)   &&
        (a.ipc_ip       == b.ipc_ip)        &&
        (a.ipc_subnet   == b.ipc_subnet))
    {
        return true;
    }
    else
    {
        return false;
    }
}

CNTV2Config2022::CNTV2Config2022(CNTV2Card & device) : CNTV2MBController(device)
{
    uint32_t features    = GetFeatures();

    _numTx0Chans = (features & (SAREK_TX0_MASK)) >> 28;
    _numRx0Chans = (features & (SAREK_RX0_MASK)) >> 24;
    _numTx1Chans = (features & (SAREK_TX1_MASK)) >> 20;
    _numRx1Chans = (features & (SAREK_RX1_MASK)) >> 16;

    _numRxChans  = _numRx0Chans + _numRx1Chans;
    _numTxChans  = _numTx0Chans + _numTx1Chans;

    _is2022_6   = ((features & SAREK_2022_6) != 0);
    _is2022_2   = ((features & SAREK_2022_2) != 0);
    _is2022_7   = ((features & SAREK_2022_7) != 0);
    _is_txTop34 = ((features & SAREK_TX_TOP34) != 0);

    _biDirectionalChannels = false;
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

bool  CNTV2Config2022::SetNetworkConfiguration (eSFP port, string localIPAddress, string netmask, string gateway)
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

    bool rv = AcquireMailbox();
    if (rv)
    {
        rv = SetMBNetworkConfiguration (port, localIPAddress, netmask, gateway);
        ReleaseMailbox();
    }
    return rv;
}

bool  CNTV2Config2022::SetNetworkConfiguration (string localIPAddress0, string netmask0, string gateway0,
                                                string localIPAddress1, string netmask1, string gateway1)
{

    SetNetworkConfiguration(SFP_TOP, localIPAddress0, netmask0, gateway0);
    SetNetworkConfiguration(SFP_BOTTOM, localIPAddress1, netmask1, gateway1);

    return true;
}

bool  CNTV2Config2022::GetNetworkConfiguration(eSFP port, IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    GetNetworkConfiguration(port, ip, subnet, gateway);

    netConfig.ipc_ip      = NTV2EndianSwap32((uint32_t)inet_addr(ip.c_str()));
    netConfig.ipc_subnet  = NTV2EndianSwap32((uint32_t)inet_addr(subnet.c_str()));
    netConfig.ipc_gateway = NTV2EndianSwap32((uint32_t)inet_addr(gateway.c_str()));

    return true;
}

bool  CNTV2Config2022::GetNetworkConfiguration(eSFP port, string & localIPAddress, string & subnetMask, string & gateway)
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

bool  CNTV2Config2022::GetNetworkConfiguration(std::string & localIPAddress0, std::string & subnetMask0, std::string & gateway0,
                                               std::string & localIPAddress1, std::string & subnetMask1, std::string & gateway1)
{

    GetNetworkConfiguration(SFP_TOP, localIPAddress0, subnetMask0, gateway0);
    GetNetworkConfiguration(SFP_BOTTOM, localIPAddress1, subnetMask1, gateway1);

    return true;
}

bool  CNTV2Config2022::SetRxChannelConfiguration(NTV2Channel channel,const rx_2022_channel &rxConfig)
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

        uint8_t ip0 = (destIp & 0xff000000)>> 24;
        int offset = (int)channel;
        if (ip0 >= 224 && ip0 <= 239)
        {
            mDevice.WriteRegister(kRegSarekIGMP4 + offset + SAREK_REGS, destIp);
        }
        else
        {
            mDevice.WriteRegister(kRegSarekIGMP4 + offset + SAREK_REGS, 0);
        }

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

        // if already enabled, make sure IGMP subscriptions are updated
        bool enabled = false;
        GetRxChannelEnable(channel,enabled);
        if (enabled)
        {
            rv = AcquireMailbox();
            if (rv)
            {
                //rv = JoinIGMPGroup(SFP_BOTTOM, (NTV2Channel)(int)(channel+2), rxConfig.secondaryDestIP);
                rv = JoinIGMPGroup(SFP_BOTTOM, channel, rxConfig.secondaryDestIP);
                ReleaseMailbox();
            }
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
        CNTV2ConfigTs2022 tsConfig(mDevice);
        tsConfig.SetupTsForDecode();
        tsConfig.SetupJ2KDecoder();
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
}

bool  CNTV2Config2022::GetRxChannelConfiguration( NTV2Channel channel, rx_2022_channel & rxConfig)
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

bool  CNTV2Config2022::SetRxChannelEnable(NTV2Channel channel, bool enable, bool enable2022_7)
{
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
        WriteChannelRegister(kReg2022_6_rx_chan_enable + baseAddr, 0x01);
    }
    else
    {
        WriteChannelRegister(kReg2022_6_rx_chan_enable + baseAddr, 0x0);
    }

    return rv;
}

bool CNTV2Config2022::GetRxChannelEnable(NTV2Channel channel, bool & enabled)
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

bool  CNTV2Config2022::SetTxChannelConfiguration(NTV2Channel channel, const tx_2022_channel & txConfig)
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

bool  CNTV2Config2022::GetTxChannelConfiguration(NTV2Channel channel, tx_2022_channel & txConfig)
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

bool CNTV2Config2022::SetTxChannelEnable(NTV2Channel channel, bool enable, bool enable2022_7)
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

bool  CNTV2Config2022::GetTxChannelEnable(NTV2Channel channel, bool & enabled)
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

bool  CNTV2Config2022::GetIGMPDisable(eSFP port, bool & disabled)
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


/////////////////////////////////////////////////////////////////////////////////
//
//
//
/////////////////////////////////////////////////////////////////////////////////

uint32_t CNTV2Config2022::GetFeatures()
{
    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekFwCfg, &val);
    return val;
}


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

    uint32_t channelPS = 0;
    if (!primaryChannel)
        channelPS = 0x80000000;

    // select channel
    SetChannel(kReg2022_6_rx_channel_access + baseAddr, channelIndex, channelPS);

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

    uint32_t channelPS = 0;
    if (!primaryChannel)
        channelPS = 0x80000000;

    // select channel
    SetChannel(kReg2022_6_tx_channel_access + baseAddr, channelIndex, channelPS);

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

