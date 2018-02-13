/**
    @file       ntv2config2110.cpp
    @brief      Implements the CNTV2Config2110 class.
    @copyright  (C) 2014-2018 AJA Video Systems, Inc.   Proprietary and confidential information.
**/

#include "ntv2config2110.h"
#include "ntv2endian.h"
#include "ntv2card.h"
#include "ntv2formatdescriptor.h"
#include <sstream>
#include <algorithm>

#if defined (AJALinux) || defined (AJAMac)
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#endif

uint32_t CNTV2Config2110::v_packetizers[4] = {SAREK_4175_TX_PACKETIZER_1,SAREK_4175_TX_PACKETIZER_2,SAREK_4175_TX_PACKETIZER_3,SAREK_4175_TX_PACKETIZER_4};
uint32_t CNTV2Config2110::a_packetizers[16] ={SAREK_3190_TX_PACKETIZER_0, SAREK_3190_TX_PACKETIZER_1, SAREK_3190_TX_PACKETIZER_2, SAREK_3190_TX_PACKETIZER_3,
                                              SAREK_3190_TX_PACKETIZER_4, SAREK_3190_TX_PACKETIZER_5, SAREK_3190_TX_PACKETIZER_6, SAREK_3190_TX_PACKETIZER_7,
                                              SAREK_3190_TX_PACKETIZER_8, SAREK_3190_TX_PACKETIZER_9, SAREK_3190_TX_PACKETIZER_10,SAREK_3190_TX_PACKETIZER_11,
                                              SAREK_3190_TX_PACKETIZER_12,SAREK_3190_TX_PACKETIZER_13,SAREK_3190_TX_PACKETIZER_14,SAREK_3190_TX_PACKETIZER_15};
uint32_t CNTV2Config2110::m_packetizers[4] = {SAREK_ANC_TX_PACKETIZER_1, SAREK_ANC_TX_PACKETIZER_2, SAREK_ANC_TX_PACKETIZER_3, SAREK_ANC_TX_PACKETIZER_4};

using namespace std;

void tx_2110Config::init()
{
    for (int i=0; i < 2; i++)
    {
        localPort[i]   = 0;
        remotePort[i]  = 0;
        remoteIP[i].erase();
    }
    videoFormat    = NTV2_FORMAT_UNKNOWN;
    videoSamples   = VPIDSampling_YUV_422;
    payloadLen     = 0;
    lastPayLoadLen = 0;
    pktsPerLine    = 0;
    ttl            = 0x80;
    tos            = 0x64;
    ssrc           = 1000;
    numAudioChannels  = 0;
    firstAudioChannel = 0;
    audioPacketInterval = PACKET_INTERVAL_125uS;
}

bool tx_2110Config::operator != ( const tx_2110Config &other )
{
    return !(*this == other);
}

bool tx_2110Config::operator == ( const tx_2110Config &other )
{
    if ((localPort       == other.localPort)      &&
        (remotePort      == other.remotePort)     &&
        (remoteIP        == other.remoteIP)       &&
        (videoFormat     == other.videoFormat)    &&
        (videoSamples    == other.videoSamples)   &&
        (numAudioChannels == other.numAudioChannels) &&
        (firstAudioChannel == other.firstAudioChannel) &&
        (audioPacketInterval == other.audioPacketInterval))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void rx_2110Config::init()
{
    rxMatch        = 0;
    sourceIP.erase();
    destIP.erase();
    sourcePort     = 0;
    destPort       = 0;
    SSRC           = 1000;
    VLAN           = 1;
    payloadType    = 0;
    videoFormat    = NTV2_FORMAT_UNKNOWN;
	numAudioChannels  = 2;
    audioSamplesPerPkt = 48;
}

bool rx_2110Config::operator != ( const rx_2110Config &other )
{
    return (!(*this == other));
}

bool rx_2110Config::operator == ( const rx_2110Config &other )
{
    if (    (rxMatch           == other.rxMatch)        &&
            (sourceIP          == other.sourceIP)       &&
            (destIP            == other.destIP)         &&
            (sourcePort        == other.sourcePort)     &&
            (destPort          == other.destPort)       &&
            (SSRC              == other.SSRC)           &&
            (VLAN              == other.VLAN)           &&
            (videoFormat       == other.videoFormat))
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

    bool rv = SetNetworkConfiguration(port, ip, subnet, gateway);
    return rv;
}

bool CNTV2Config2110::SetNetworkConfiguration (eSFP port, string localIPAddress, string netmask, string gateway)
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


    uint32_t core;
    core = SAREK_2110_VIDEO_FRAMER_0;
    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi);
    core = SAREK_2110_AUDIO_FRAMER_0;
    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi);

    core = SAREK_2110_VIDEO_FRAMER_1;
    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo2);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi2);
    core = SAREK_2110_AUDIO_FRAMER_1;
    mDevice.WriteRegister(kRegFramer_src_mac_lo + core,boardLo2);
    mDevice.WriteRegister(kRegFramer_src_mac_hi + core,boardHi2);

    bool rv = SetMBNetworkConfiguration (port, localIPAddress, netmask, gateway);
    if (!rv) return false;

    if (port == SFP_TOP)
    {
        ConfigurePTP(port,localIPAddress);
    }

    return true;
}

bool CNTV2Config2110::SetNetworkConfiguration (string localIPAddress0, string netmask0, string gateway0,
                                               string localIPAddress1, string netmask1, string gateway1)
{

    bool rv = SetNetworkConfiguration(SFP_TOP, localIPAddress0, netmask0, gateway0);
    if (!rv) return false;

    rv = SetNetworkConfiguration(SFP_BOTTOM, localIPAddress1, netmask1, gateway1);
    return rv;
}

bool  CNTV2Config2110::DisableNetworkInterface(eSFP port)
{
    return DisableNetworkConfiguration(port);
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

bool CNTV2Config2110::DisableRxStream(const NTV2Channel channel, const NTV2Stream stream)
{
    if (GetLinkActive(SFP_TOP) == false)
    {
        mIpErrorCode = NTV2IpErrLinkANotConfigured;
        return false;
    }

    // disable IGMP subscription
    eSFP port = GetRxPort(channel);
    bool disableIGMP;
    GetIGMPDisable(port, disableIGMP);
    if (!disableIGMP)
    {
        EnableIGMPGroup(port,channel,stream,false);
    }

    DisableDepacketizerStream(channel,stream);
    DisableDecapsulatorStream(channel,stream);
    return true;
}

bool CNTV2Config2110::EnableRxStream(const NTV2Channel channel, const NTV2Stream stream, rx_2110Config & rxConfig)
{
    if (GetLinkActive(SFP_TOP) == false)
    {
        mIpErrorCode = NTV2IpErrLinkANotConfigured;
        return false;
    }

    // make IGMP subsciption if needed
    eSFP port = GetRxPort(channel);

    uint32_t destIp = inet_addr(rxConfig.destIP.c_str());
    destIp = NTV2EndianSwap32(destIp);

    uint32_t srcIp = inet_addr(rxConfig.sourceIP.c_str());
    srcIp = NTV2EndianSwap32(srcIp);

    uint8_t ip0 = (destIp & 0xff000000)>> 24;
    if (ip0 >= 224 && ip0 <= 239)
    {
        // is multicast
        SetIGMPGroup(port, channel, stream, destIp, srcIp, true);
    }
    else
    {
        UnsetIGMPGroup(port, channel, stream);
    }

    DisableDepacketizerStream(channel,stream);
    DisableDecapsulatorStream(channel,stream);

    SetupDecapsulatorStream(channel,stream, rxConfig);
    //ResetDepacketizer(channel,stream);
    SetupDepacketizerStream(channel,stream,rxConfig);

    EnableDecapsulatorStream(channel,stream);
    EnableDepacketizerStream(channel,stream);

    return true;
}

void  CNTV2Config2110::SetupDecapsulatorStream(NTV2Channel channel, NTV2Stream stream, rx_2110Config & rxConfig)
{
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel,stream);

    // source ip address
    uint32_t sourceIp = inet_addr(rxConfig.sourceIP.c_str());
    sourceIp = NTV2EndianSwap32(sourceIp);
    mDevice.WriteRegister(kRegDecap_match_src_ip + decapBaseAddr, sourceIp);

    // dest ip address
    uint32_t destIp = inet_addr(rxConfig.destIP.c_str());
    destIp = NTV2EndianSwap32(destIp);
    mDevice.WriteRegister(kRegDecap_match_dst_ip + decapBaseAddr, destIp);

    // source port
    mDevice.WriteRegister(kRegDecap_match_udp_src_port + decapBaseAddr, rxConfig.sourcePort);

    // dest port
    mDevice.WriteRegister(kRegDecap_match_udp_dst_port + decapBaseAddr, rxConfig.destPort);

    // ssrc
    mDevice.WriteRegister(kRegDecap_match_ssrc + decapBaseAddr, rxConfig.SSRC);

    // vlan
    //WriteRegister(kRegDecap_match_vlan + decapBaseAddr, rxConfig.VLAN);

    // payload type
    mDevice. WriteRegister(kRegDecap_match_payload + decapBaseAddr, rxConfig.payloadType);

    // matching
    mDevice.WriteRegister(kRegDecap_match_sel + decapBaseAddr, rxConfig.rxMatch);
}

void  CNTV2Config2110::DisableDepacketizerStream(NTV2Channel channel, NTV2Stream stream)
{
    uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(channel,stream);
    if (stream == NTV2_VIDEO_STREAM)
    {
        mDevice.WriteRegister(kReg4175_depkt_control + depacketizerBaseAddr, 0x00);
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr, 0x00);
    }
}

void  CNTV2Config2110::EnableDepacketizerStream(NTV2Channel channel, NTV2Stream stream)
{
    uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(channel,stream);
    if (stream == NTV2_VIDEO_STREAM)
    {
        mDevice.WriteRegister(kReg4175_depkt_control + depacketizerBaseAddr, 0x01);
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr, 0x01);
    }
}

void  CNTV2Config2110::DisableDecapsulatorStream(NTV2Channel channel, NTV2Stream stream)
{
    // disable decasulator
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel,stream);
    mDevice.WriteRegister(kRegDecap_chan_enable + decapBaseAddr, 0x00);
}

void  CNTV2Config2110::EnableDecapsulatorStream(NTV2Channel channel, NTV2Stream stream)
{
    // enable decasulator
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel,stream);
    mDevice.WriteRegister(kRegDecap_chan_enable + decapBaseAddr, 0x01);
}

void  CNTV2Config2110::ResetDepacketizerStream(const NTV2Channel channel, NTV2Stream stream)
{
    (void) channel;
    if (stream == NTV2_AUDIO1_STREAM)
    {
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, 0x2);
    }
    else
    {
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, 0x1);
    }
    // Wait
    #if defined(AJAWindows) || defined(MSWindows)
        ::Sleep (500);
    #else
        usleep (500 * 1000);
    #endif

    mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, 0x0);

    // Wait
    #if defined(AJAWindows) || defined(MSWindows)
        ::Sleep (500);
    #else
        usleep (500 * 1000);
    #endif
}

void  CNTV2Config2110::SetupDepacketizerStream(const NTV2Channel channel, NTV2Stream stream, const rx_2110Config & rxConfig)
{
    if (stream == NTV2_VIDEO_STREAM)
    {
        NTV2VideoFormat fmt = rxConfig.videoFormat;
        NTV2FormatDescriptor fd(fmt,NTV2_FBF_10BIT_YCBCR);

        NTV2FrameRate       fr  = GetNTV2FrameRateFromVideoFormat(fmt);
        NTV2FrameGeometry   fg  = fd.GetFrameGeometry();
        NTV2Standard        std = fd.GetVideoStandard();
        bool               is2K = fd.Is2KFormat();

        uint32_t val = ( (((uint32_t) fr) << 8) |
                         (((uint32_t) fg) << 4) |
                          ((uint32_t) std ) );
        if (is2K) val += BIT(13);

        // setup PLL
        switch(channel)
        {
        case NTV2_CHANNEL1:
            mDevice.WriteRegister(kRegRxVideoDecode1 + SAREK_2110_TX_ARBITRATOR, val);
            break;
        case NTV2_CHANNEL2:
            mDevice.WriteRegister(kRegRxVideoDecode2 + SAREK_2110_TX_ARBITRATOR, val);
            break;
        case NTV2_CHANNEL3:
            mDevice.WriteRegister(kRegRxVideoDecode3 + SAREK_2110_TX_ARBITRATOR, val);
            break;
        case NTV2_CHANNEL4:
            mDevice.WriteRegister(kRegRxVideoDecode4 + SAREK_2110_TX_ARBITRATOR, val);
            break;
        default:
            break;
        }
    }

    if (stream == NTV2_AUDIO1_STREAM)
    {
        // setup 3190 depacketizer
        uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(channel,stream);

        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr, 0x00);

        uint32_t num_samples  = rxConfig.audioSamplesPerPkt;
		uint32_t num_channels = rxConfig.numAudioChannels;
        uint32_t val = (num_samples << 8) + num_channels;
        mDevice.WriteRegister(kReg3190_depkt_config + depacketizerBaseAddr,val);

        // audio channels
        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr,0x01);
    }
}

bool  CNTV2Config2110::GetRxStreamConfiguration(const NTV2Channel channel, NTV2Stream stream, rx_2110Config & rxConfig)
{
    uint32_t    val;

    // get address,strean
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel,stream);

    // source ip address
    mDevice.ReadRegister(kRegDecap_match_src_ip + decapBaseAddr, &val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    rxConfig.sourceIP = ip;

    // dest ip address
    mDevice.ReadRegister(kRegDecap_match_dst_ip + decapBaseAddr, &val);
    in.s_addr = NTV2EndianSwap32(val);
    ip = inet_ntoa(in);
    rxConfig.destIP = ip;

    // source port
    mDevice.ReadRegister(kRegDecap_match_udp_src_port + decapBaseAddr, &rxConfig.sourcePort);

    // dest port
    mDevice.ReadRegister(kRegDecap_match_udp_dst_port + decapBaseAddr, &rxConfig.destPort);

    // ssrc
    mDevice.ReadRegister(kRegDecap_match_ssrc + decapBaseAddr, &rxConfig.SSRC);

    // vlan
    //mDevice.ReadRegister(kRegDecap_match_vlan + decapBaseAddr, &val);
    //rxConfig.VLAN = val & 0xffff;

    // payload type
    mDevice.ReadRegister(kRegDecap_match_payload + decapBaseAddr,&val);
    rxConfig.payloadType = val & 0x7f;

    // matching
    mDevice.ReadRegister(kRegDecap_match_sel + decapBaseAddr, &rxConfig.rxMatch);

    if (stream == NTV2_VIDEO_STREAM)
    {
#if 0
        // format
        uint32_t val;
        switch(channel)
        {
        case NTV2_CHANNEL1:
            mDevice.ReadRegister(kRegRxVideoDecode4 + SAREK_2110_TX_ARBITRATOR, &val);
            break;
        case NTV2_CHANNEL2:
            mDevice.ReadRegister(kRegRxVideoDecode4 + SAREK_2110_TX_ARBITRATOR, &val);
            break;
        case NTV2_CHANNEL3:
            mDevice.ReadRegister(kRegRxVideoDecode4 + SAREK_2110_TX_ARBITRATOR, &val);
            break;
        case NTV2_CHANNEL4:
            mDevice.ReadRegister(kRegRxVideoDecode4 + SAREK_2110_TX_ARBITRATOR, &val);
            break;
        }

           NTV2FrameRate       fr  = NTV2FrameRate((val & 0xf00) >> 8);
           NTV2FrameGeometry   fg  = NTV2FrameGeometry((val & 0xf0) >> 4);
           NTV2Standard        std = NTV2Standard(val & 0x0f);
           bool               is2K = (val & BIT(13));

           NTV2FormatDescriptor fd;
#endif
    }

    return true;
}

bool CNTV2Config2110::GetRxStreamEnable(const NTV2Channel channel, NTV2Stream stream, bool & enabled)
{
    // get address
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel,stream);

    uint32_t val;
    mDevice.ReadRegister(kRegDecap_chan_enable + decapBaseAddr,&val);
    enabled = (val & 0x01);

    return true;
}

int CNTV2Config2110::LeastCommonMultiple(int a,int b)
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

bool CNTV2Config2110::SetTxChannelConfiguration(const NTV2Channel channel, NTV2Stream stream, const tx_2110Config & txConfig)
{
    bool        rv = true;

    if (GetLinkActive(SFP_TOP) == false)
    {
        mIpErrorCode = NTV2IpErrLinkANotConfigured;
        return false;
    }

    SetFramerStream(channel,stream,SFP_LINK_A,txConfig);
    SetFramerStream(channel,stream,SFP_LINK_B,txConfig);

    // packetizer
    uint32_t baseAddrPacketizer;
    SetTxPacketizerChannel(channel,stream,baseAddrPacketizer);

    if (stream == NTV2_VIDEO_STREAM)
    {
        // setup 4175 packetizer

        NTV2VideoFormat fmt = txConfig.videoFormat;
        bool interlaced = !NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(fmt);
        NTV2FormatDescriptor fd(fmt,NTV2_FBF_10BIT_YCBCR);

        // width
        uint32_t width = fd.GetRasterWidth();
        mDevice.WriteRegister(kReg4175_pkt_width + baseAddrPacketizer,width);

        // height
        uint32_t height = fd.GetRasterHeight();
        if (interlaced)
        {
            height /= 2;
        }
        mDevice.WriteRegister(kReg4175_pkt_height + baseAddrPacketizer,height);

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
        mDevice.WriteRegister(kReg4175_pkt_vid_fmt + baseAddrPacketizer,vf);

        const int bitsPerComponent = 10;
        const int pixelsPerClock = 1;
        int activeLine_root    = width * componentsPerPixel * bitsPerComponent;
        int activeLineLength   = activeLine_root/8;
        int pixelGroup_root    = bitsPerComponent * componentsPerUnit;
        int pixelGroupSize     = pixelGroup_root/8;
        int bytesPerCycle_root = pixelsPerClock * bitsPerComponent * componentsPerPixel;
        //      int bytesPerCycle      = bytesPerCycle_root/8;
        int lcm                = LeastCommonMultiple(pixelGroup_root,bytesPerCycle_root)/8;
        int payloadLength_root =  min(activeLineLength,1376)/lcm;
        int payloadLength      = payloadLength_root * lcm;
        float pktsPerLine      = ((float)activeLineLength)/((float)payloadLength);
        int ipktsPerLine       = (int)ceil(pktsPerLine);

        int payloadLengthLast  = activeLineLength - (payloadLength * (ipktsPerLine -1));

        if (txConfig.payloadLen != 0)
            payloadLength       = txConfig.payloadLen;
        if (txConfig.lastPayLoadLen != 0)
            payloadLengthLast   = txConfig.lastPayLoadLen;
        if (txConfig.pktsPerLine != 0)
            ipktsPerLine        = txConfig.pktsPerLine;

        // pkts per line
        mDevice.WriteRegister(kReg4175_pkt_pkts_per_line + baseAddrPacketizer,ipktsPerLine);

        // payload length
        mDevice.WriteRegister(kReg4175_pkt_payload_len + baseAddrPacketizer,payloadLength);

        // payload length last
        mDevice.WriteRegister(kReg4175_pkt_payload_len_last + baseAddrPacketizer,payloadLengthLast);

        // payload type
        mDevice.WriteRegister(kReg4175_pkt_payload_type + baseAddrPacketizer,txConfig.payloadType);

        // SSRC
        mDevice.WriteRegister(kReg4175_pkt_ssrc + baseAddrPacketizer,txConfig.ssrc);

        // pix per pkt
        int ppp = (payloadLength/pixelGroupSize) * 2;   // as per JeffL
        mDevice.WriteRegister(kReg4175_pkt_pix_per_pkt + baseAddrPacketizer,ppp);

        // interlace
        int ilace = (interlaced) ? 0x01 : 0x00;
        mDevice.WriteRegister(kReg4175_pkt_interlace_ctrl + baseAddrPacketizer,ilace);

        // end setup 4175 packetizer
        SetTxFormat(channel, txConfig.videoFormat);
    }
    else
    {
        // audio setup 3190 packetizer

        uint32_t audioChans = txConfig.numAudioChannels;
        uint32_t samples    = (txConfig.audioPacketInterval == PACKET_INTERVAL_125uS) ? 6 : 48;
        uint32_t plength    = audioChans * samples * 3;

        // audio select
        uint32_t aselect = ((uint32_t)txConfig.firstAudioChannel << 16 ) + (audioChans-1);
        uint32_t offset  =  get2110TxStream(channel,stream) * 4;
        mDevice.WriteRegister(SAREK_2110_AUDIO_STREAMSELECT + offset,aselect);

        // num samples
        mDevice.WriteRegister(kReg3190_pkt_num_samples + baseAddrPacketizer, samples);

        // audio channels - zero-based (i.e. 0 = 1 channel)
        mDevice.WriteRegister(kReg3190_pkt_num_audio_channels + baseAddrPacketizer, audioChans);

        // payload length
        mDevice.WriteRegister(kReg3190_pkt_payload_len + baseAddrPacketizer, plength);

        // payload type
        mDevice.WriteRegister(kReg3190_pkt_payload_type + baseAddrPacketizer, txConfig.payloadType);

        // ssrc
        mDevice.WriteRegister(kReg3190_pkt_ssrc + baseAddrPacketizer,txConfig.ssrc);
    }

    // Generate and push the SDP
    GenSDP(channel,stream);

    return rv;
}

bool CNTV2Config2110::SetFramerStream(NTV2Channel channel, NTV2Stream stream, eSFP link, const tx_2110Config & txConfig)
{
    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(link, channel,stream);

    // select channel
    SelectTxFramerChannel(channel, stream, baseAddrFramer);

    // setup framer
    // hold off access while we update channel regs
    AcquireFramerControlAccess(baseAddrFramer);

    uint32_t val = (txConfig.tos << 8) | txConfig.ttl;
    WriteChannelRegister(kRegFramer_ip_hdr_media + baseAddrFramer, val);

    int index = (int)link;
    // dest ip address
    uint32_t destIp = inet_addr(txConfig.remoteIP[index].c_str());
    destIp = NTV2EndianSwap32(destIp);
    WriteChannelRegister(kRegFramer_dst_ip + baseAddrFramer,destIp);

    // source port
    WriteChannelRegister(kRegFramer_udp_src_port + baseAddrFramer,txConfig.localPort[index]);

    // dest port
    WriteChannelRegister(kRegFramer_udp_dst_port + baseAddrFramer,txConfig.remotePort[index]);

    // MAC address
    uint32_t    hi;
    uint32_t    lo;
    bool rv = GetMACAddress(link,channel,stream,txConfig.remoteIP[index],hi,lo);
    if (!rv) return false;
    WriteChannelRegister(kRegFramer_dest_mac_lo  + baseAddrFramer,lo);
    WriteChannelRegister(kRegFramer_dest_mac_hi  + baseAddrFramer,hi);

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // end framer setup
    return true;
}

bool CNTV2Config2110::GetTxChannelConfiguration(const NTV2Channel channel, NTV2Stream stream, tx_2110Config & txConfig)
{

    GetFramerStream(channel,stream,SFP_LINK_A,txConfig);
    GetFramerStream(channel,stream,SFP_LINK_B,txConfig);

    // select packetizer
    uint32_t baseAddrPacketizer;
    SetTxPacketizerChannel(channel,stream,baseAddrPacketizer);

    uint32_t val;
    if (stream == NTV2_VIDEO_STREAM)
    {
        // payload type
        mDevice.ReadRegister(kReg4175_pkt_payload_type + baseAddrPacketizer, &val);
        txConfig.payloadType = (uint16_t)val;

        // SSRC
        mDevice.ReadRegister(kReg4175_pkt_ssrc + baseAddrPacketizer,&txConfig.ssrc);

        uint32_t width;
        mDevice.ReadRegister(kReg4175_pkt_width + baseAddrPacketizer, &width);

        uint32_t height;
        mDevice.ReadRegister(kReg4175_pkt_height + baseAddrPacketizer, &height);

        // pkts per line
        mDevice.ReadRegister(kReg4175_pkt_pkts_per_line + baseAddrPacketizer,&txConfig.pktsPerLine);

        // payload length
        mDevice.ReadRegister(kReg4175_pkt_payload_len + baseAddrPacketizer,&txConfig.payloadLen);

        // payload length last
        mDevice.ReadRegister(kReg4175_pkt_payload_len_last + baseAddrPacketizer,&txConfig.lastPayLoadLen);

        // pix per pkt
        uint32_t ppp;
        mDevice.ReadRegister(kReg4175_pkt_pix_per_pkt + baseAddrPacketizer,&ppp);

        // interlace
        uint32_t  ilace;
        mDevice.ReadRegister(kReg4175_pkt_interlace_ctrl + baseAddrPacketizer,&ilace);

        GetTxFormat(channel, txConfig.videoFormat);
    }
    else
    {
        // audio - payload type
        mDevice.ReadRegister(kReg3190_pkt_payload_type + baseAddrPacketizer, &val);
        txConfig.payloadType = (uint16_t)val;

        // ssrc
        mDevice.ReadRegister(kReg3190_pkt_ssrc + baseAddrPacketizer, &txConfig.ssrc);

        // audio select
        uint32_t offset  =  get2110TxStream(channel,stream) * 4;
        uint32_t aselect;
        mDevice.ReadRegister(SAREK_2110_AUDIO_STREAMSELECT + offset,&aselect);

        txConfig.firstAudioChannel = (aselect >> 16) & 0xff;
        txConfig.numAudioChannels  = (aselect & 0xff) + 1;

        // packet interval
        uint32_t samples;
        mDevice.ReadRegister(kReg3190_pkt_num_samples + baseAddrPacketizer, &samples);
        txConfig.audioPacketInterval = (samples == 6) ? PACKET_INTERVAL_125uS : PACKET_INTERVAL_1mS;
    }

    return true;
}

void CNTV2Config2110::GetFramerStream(NTV2Channel channel, NTV2Stream stream,eSFP link, tx_2110Config  & txConfig)
{
    int index = (int)link;

    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(link,channel,stream);

    // Select channel
    SelectTxFramerChannel(channel, stream, baseAddrFramer);

    uint32_t    val;
    ReadChannelRegister(kRegFramer_ip_hdr_media + baseAddrFramer,&val);
    txConfig.ttl = val & 0xff;
    txConfig.tos = (val & 0xff00) >> 8;

    // dest ip address
    ReadChannelRegister(kRegFramer_dst_ip + baseAddrFramer,&val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    txConfig.remoteIP[index] = ip;

    // source port
    ReadChannelRegister(kRegFramer_udp_src_port + baseAddrFramer,&txConfig.localPort[index]);

    // dest port
    ReadChannelRegister(kRegFramer_udp_dst_port + baseAddrFramer,&txConfig.remotePort[index]);
}


bool CNTV2Config2110::SetTxChannelEnable(const NTV2Channel channel, NTV2Stream stream,bool enableLinkA, bool enableLinkB)
{
    if (GetLinkActive(SFP_TOP) == false)
    {
        mIpErrorCode = NTV2IpErrLinkANotConfigured;
        return false;
    }

    EnableFramerStream(channel,stream,SFP_LINK_A,enableLinkA);
    EnableFramerStream(channel,stream,SFP_LINK_B,enableLinkB);
    SetArbiter(channel,stream,SFP_LINK_A,enableLinkA);
    SetArbiter(channel,stream,SFP_LINK_B,enableLinkB);


    // ** Packetizer
    uint32_t packetizerBaseAddr;
    SetTxPacketizerChannel(channel,stream,packetizerBaseAddr);

    if (enableLinkA || enableLinkB)
    {
        // enable
        mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x00);
        mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x80);
        mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x81);
    }
    else
    {
        // disable
        mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x00);
    }

    GenSDP(channel,stream);

    return true;
}

void CNTV2Config2110::EnableFramerStream(NTV2Channel channel, NTV2Stream stream,eSFP link, bool enable)
{
    // ** Framer
    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(link,channel,stream);

    // select channel
    SelectTxFramerChannel(channel, stream, baseAddrFramer);

    // hold off access while we update channel regs
    AcquireFramerControlAccess(baseAddrFramer);

    if (enable)
    {
        uint32_t    localIp;
        if (GetTxPort(channel) == SFP_TOP)
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0,&localIp);
        }
        else
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1,&localIp);
        }

        WriteChannelRegister(kRegFramer_src_ip + baseAddrFramer,NTV2EndianSwap32(localIp));

        // enable
        WriteChannelRegister(kRegFramer_chan_ctrl + baseAddrFramer,0x01);  // enables tx over mac1/mac2
    }
    else
    {
        // disable
        WriteChannelRegister(kRegFramer_chan_ctrl + baseAddrFramer,0x0);   // disables channel
    }

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // ** Framer end
}


bool CNTV2Config2110::GetTxChannelEnable(const NTV2Channel channel, NTV2Stream stream,bool & linkAEnabled, bool & linkBEnabled)
{
    GetArbiter(channel,stream,SFP_LINK_A,linkAEnabled);
    GetArbiter(channel,stream,SFP_LINK_B,linkBEnabled);
    return true;
}

bool  CNTV2Config2110::SetPTPMaster(std::string ptpMaster)
{
    uint32_t addr = inet_addr(ptpMaster.c_str());
    addr = NTV2EndianSwap32(addr);
    if (addr != 0 && addr != 0xffffffff)
    {
        mDevice.WriteRegister(kRegPll_PTP_MstrIP + SAREK_PLL, addr);
        mDevice.WriteRegister(kRegPll_PTP_Match + SAREK_PLL, 0x09);
        return true;
    }
    else
    {
        mDevice.WriteRegister(kRegPll_PTP_MstrIP + SAREK_PLL, 0);
        mDevice.WriteRegister(kRegPll_PTP_Match + SAREK_PLL, 0x07);
        return false;
    }
}

bool CNTV2Config2110::GetPTPMaster(std::string & ptpMaster)
{
    uint32_t val;
    mDevice.ReadRegister(kRegPll_PTP_MstrIP + SAREK_PLL, &val);
    val = NTV2EndianSwap32(val);

    struct in_addr addr;
    addr.s_addr = val;
    ptpMaster = inet_ntoa(addr);

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
        mIpErrorCode = NTV2IpErrInvalidIGMPVersion;
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

uint32_t CNTV2Config2110::GetDecapsulatorAddress(NTV2Channel channel,NTV2Stream stream)
{
    switch (channel)
    {
    case NTV2_CHANNEL1:
        if (stream == NTV2_VIDEO_STREAM)
            return SAREK_2110_DECAPSULATOR_0;
        else
            return SAREK_2110_DECAPSULATOR_0 + 16;
    case NTV2_CHANNEL2:
        if (stream == NTV2_VIDEO_STREAM)
            return SAREK_2110_DECAPSULATOR_0 + 32;
        else
            return SAREK_2110_DECAPSULATOR_0 + 48;
    case NTV2_CHANNEL3:
        if (stream == NTV2_VIDEO_STREAM)
            return SAREK_2110_DECAPSULATOR_1;
        else
            return SAREK_2110_DECAPSULATOR_1 + 16;
    case NTV2_CHANNEL4:
        if (stream == NTV2_VIDEO_STREAM)
            return SAREK_2110_DECAPSULATOR_1 + 32;
        else
            return SAREK_2110_DECAPSULATOR_1 + 48;
    }
}

uint32_t CNTV2Config2110::GetDepacketizerAddress(NTV2Channel channel, NTV2Stream stream)
{
    static uint32_t v_depacketizers[4] = {SAREK_4175_RX_DEPACKETIZER_1,SAREK_4175_RX_DEPACKETIZER_2,SAREK_4175_RX_DEPACKETIZER_2,SAREK_4175_RX_DEPACKETIZER_4};
    static uint32_t a_depacketizers[4] = {SAREK_3190_RX_DEPACKETIZER_1,SAREK_3190_RX_DEPACKETIZER_2,SAREK_3190_RX_DEPACKETIZER_3,SAREK_3190_RX_DEPACKETIZER_4};

    uint32_t iChannel = (uint32_t) channel;

    if (stream == NTV2_VIDEO_STREAM)
    {
        return v_depacketizers[iChannel];
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        return  a_depacketizers[iChannel];
    }

    return SAREK_4175_RX_DEPACKETIZER_1;
}

uint32_t CNTV2Config2110::GetFramerAddress(eSFP link, NTV2Channel channel, NTV2Stream stream)
{
    if (link == SFP_LINK_B)
    {
        if (stream == NTV2_VIDEO_STREAM)
            return SAREK_2110_VIDEO_FRAMER_1;
        else
            return SAREK_2110_AUDIO_FRAMER_1;
    }
    else
    {
        if (stream == NTV2_VIDEO_STREAM)
            return SAREK_2110_VIDEO_FRAMER_0;
        else
            return SAREK_2110_AUDIO_FRAMER_0;
    }
}

void CNTV2Config2110::SelectTxFramerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t baseAddrFramer)
{
    // select channel
    uint32_t iStream = get2110TxStream(channel,stream);
    SetChannel(kRegFramer_channel_access + baseAddrFramer, iStream);
}

bool CNTV2Config2110::SetTxPacketizerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t & baseAddrPacketizer)
{
    uint32_t iChannel = (uint32_t) channel;

    if (iChannel > _numTxChans)
        return false;

    uint32_t iStream = get2110TxStream(channel,stream);

    switch (stream)
    {
    case NTV2_VIDEO_STREAM:
        baseAddrPacketizer  = v_packetizers[iChannel];
        mDevice.WriteRegister(kReg4175_pkt_chan_num + baseAddrPacketizer, iStream);
        break;
    case NTV2_AUDIO1_STREAM:
    case NTV2_AUDIO2_STREAM:
    case NTV2_AUDIO3_STREAM:
    case NTV2_AUDIO4_STREAM:
        baseAddrPacketizer  = a_packetizers[iStream];
        mDevice.WriteRegister(kReg3190_pkt_chan_num + baseAddrPacketizer, iStream);
        break;
    default:
        return false;
    }
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
    mDevice.WriteRegister(kRegPll_PTP_LclMacLo   + SAREK_PLL, alignedMACLo);
    mDevice.WriteRegister(kRegPll_PTP_LclMacHi   + SAREK_PLL, alignedMACHi);

    mDevice.WriteRegister(kRegPll_PTP_EventUdp   + SAREK_PLL, 0x0000013f);
    mDevice.WriteRegister(kRegPll_PTP_MstrMcast  + SAREK_PLL, 0xe0000181);
    mDevice.WriteRegister(kRegPll_PTP_LclIP      + SAREK_PLL, addr);
    uint32_t val;
    mDevice.ReadRegister(kRegPll_PTP_MstrIP      + SAREK_PLL, &val);
    if (val == 0)
        mDevice.WriteRegister(kRegPll_PTP_Match  + SAREK_PLL, 0x1);
    else
        mDevice.WriteRegister(kRegPll_PTP_Match  + SAREK_PLL, 0x9);
    mDevice.WriteRegister(kRegPll_Config         + SAREK_PLL, PLL_CONFIG_PTP | PLL_CONFIG_DCO_MODE);

    //WriteChannelRegister(kRegPll_PTP_LclClkIdLo + SAREK_PLL, (0xfe << 24) | ((macHi & 0x000000ff) << 16) | (macLo >> 16));
    //WriteChannelRegister(kRegPll_PTP_LclClkIdHi + SAREK_PLL, (macHi & 0xffffff00) | 0xff);

    return true;
}

bool CNTV2Config2110::GetSFPMSAData(eSFP port, SFPMSAData & data)
{
    return GetSFPInfo(port,data);
}

bool CNTV2Config2110::GetLinkStatus(eSFP port, sLinkStatus & linkStatus)
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


string CNTV2Config2110::getLastError()
{
    return NTV2IpErrorEnumToString(getLastErrorCode());
}

NTV2IpError CNTV2Config2110::getLastErrorCode()
{
    NTV2IpError error = mIpErrorCode;
    mIpErrorCode = NTV2IpErrNone;
    return error;
}

void CNTV2Config2110::AcquireFramerControlAccess(uint32_t baseAddr)
{
    WriteChannelRegister(kRegFramer_control + baseAddr, 0x00);
    uint32_t val;
    mDevice.ReadRegister(kRegFramer_status + baseAddr,&val);
    while (val & BIT(1))
    {
        // Wait
        #if defined(AJAWindows) || defined(MSWindows)
            ::Sleep (10);
        #else
            usleep (10 * 1000);
        #endif

        mDevice.ReadRegister(kRegFramer_status + baseAddr,&val);
    }
}

void CNTV2Config2110::ReleaseFramerControlAccess(uint32_t baseAddr)
{
    WriteChannelRegister(kRegFramer_control + baseAddr, 0x02);
}

uint32_t CNTV2Config2110::get2110TxStream(NTV2Channel ch,NTV2Stream str)
{
    // this stream number is a core 'channel' number
    uint32_t iStream = 0;
    switch (str)
    {
		case NTV2_VIDEO_STREAM:			iStream = (uint32_t)ch;				break;
		case NTV2_AUDIO1_STREAM:		iStream = (uint32_t)ch * 4;			break;
		case NTV2_AUDIO2_STREAM:		iStream = ((uint32_t)ch * 4) + 1;	break;
		case NTV2_AUDIO3_STREAM:		iStream = ((uint32_t)ch * 4) + 2;	break;
		case NTV2_AUDIO4_STREAM:		iStream = ((uint32_t)ch * 4) + 3;	break;
		case NTV2_METADATA_STREAM:		break;
		case NTV2_MAX_NUM_STREAMS:		break;
    }
    return iStream;
}

bool  CNTV2Config2110::decompose2110TxVideoStream(uint32_t istream, NTV2Channel & ch, NTV2Stream & str)
{
    if (istream > 3)
        return false;

    ch  = (NTV2Channel)istream;
    str = NTV2_VIDEO_STREAM;
    return true;
}

bool  CNTV2Config2110::decompose2110TxAudioStream(uint32_t istream, NTV2Channel & ch, NTV2Stream & str)
{
    if (istream > 15)
        return false;

    int channel = istream / 4;
    int stream  = istream - (channel * 4);
    ch          = (NTV2Channel) channel;
    str         = (NTV2Stream) (stream + NTV2_AUDIO1_STREAM);
    return true;
}

bool CNTV2Config2110::GetMACAddress(eSFP port, NTV2Channel channel, NTV2Stream stream, string remoteIP, uint32_t & hi, uint32_t & lo)
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
            SetTxChannelEnable(channel, stream, false); // stop transmit
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

string CNTV2Config2110::GetTxSDP(NTV2Channel chan, NTV2Stream stream)
{
    int ch = (int)chan;
    int st = (int)stream;
    if (txsdp[ch][st].str().empty())
    {
        GenSDP(chan,stream);
    }
    return txsdp[ch][st].str();
}

string CNTV2Config2110::To_String(int val)
{
    ostringstream oss;
    oss << val;
    return oss.str();
}

bool CNTV2Config2110::GenSDP(NTV2Channel channel, NTV2Stream stream)
{
    int ch = (int)channel;
    int st = (int)stream;

    string filename = "txch" + To_String(ch+1);
    if (stream == NTV2_VIDEO_STREAM)
    {
        filename += "v.sdp";
    }
    else
    {
        filename += "a";
        filename += To_String(st);
        filename += ".sdp";
    }
    stringstream & sdp = txsdp[ch][st];

    sdp.str("");
    sdp.clear();

    // protocol version
    sdp << "v=0" << endl;

    // username session-id  version network-type address-type address
    sdp << "o=- ";

    uint64_t t = GetNTPTimestamp();
    sdp <<  To_String(t);

    sdp << " 0 IN IP4 ";

    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0,&val);
    struct in_addr addr;
    addr.s_addr = val;
    string localIPAddress = inet_ntoa(addr);
    sdp << localIPAddress << endl;

    // session name
    sdp << "s=AJA KonaIP 2110" << endl;

    // time the session is active
    sdp << "t=0 0" <<endl;

    // PTP
    string gmInfo;
    bool rv = FetchGrandMasterInfo(gmInfo);
    gmInfo.erase(remove(gmInfo.begin(), gmInfo.end(), '\n'), gmInfo.end());

    if (stream == NTV2_VIDEO_STREAM)
    {
        GenSDPVideoStream(sdp,channel,gmInfo);
    }
    else
    {
        GenSDPAudioStream(sdp,channel,stream,gmInfo);
    }

    rv = PushSDP(filename,sdp);

    return rv;
}

bool CNTV2Config2110::GenSDPVideoStream(stringstream & sdp, NTV2Channel channel, string gmInfo)
{
    // TODO - fix this to work with link B
    bool enabledA;
    bool enabledB;
    GetTxChannelEnable(channel,NTV2_VIDEO_STREAM,enabledA,enabledB);
    if (!enabledA)
    {
        return true;
    }

    tx_2110Config config;
    GetTxChannelConfiguration(channel, NTV2_VIDEO_STREAM, config);

    uint32_t baseAddrPacketizer;
    SetTxPacketizerChannel(channel,NTV2_VIDEO_STREAM,baseAddrPacketizer);

    uint32_t width;
    mDevice.ReadRegister(kReg4175_pkt_width + baseAddrPacketizer,&width);

    uint32_t height;
    mDevice.ReadRegister(kReg4175_pkt_height + baseAddrPacketizer,&height);

    uint32_t  ilace;
    mDevice.ReadRegister(kReg4175_pkt_interlace_ctrl + baseAddrPacketizer,&ilace);

    if (ilace == 1)
    {
        height *= 2;
    }

    NTV2VideoFormat vfmt;
    GetTxFormat(channel,vfmt);
    NTV2FrameRate frate = GetNTV2FrameRateFromVideoFormat(vfmt);
    string rateString   = rateToString(frate);

    // media name
    sdp << "m=video ";
    sdp << To_String(config.remotePort[0]);     // FIXME
    sdp << " RTP/AVP ";
    sdp << To_String(config.payloadType) << endl;

    // connection information
    sdp << "c=IN IP4 ";
    sdp << config.remoteIP;
    sdp << "/" << To_String(config.ttl) << endl;

    // rtpmap
    sdp << "a=rtpmap:";
    sdp << To_String(config.payloadType);
    sdp << " raw/90000" << endl;

    //fmtp
    sdp << "a=fmtp:";
    sdp << To_String(config.payloadType);
    sdp << " sampling=YCbCr-4:2:2; width=";
    sdp << To_String(width);
    sdp << "; height=";
    sdp << To_String(height);
    sdp << "; exactframerate=";
    sdp << rateString;
    sdp << "; depth=10; TCS=SDR; colorimetry=";
    sdp << ((NTV2_IS_SD_VIDEO_FORMAT(vfmt)) ? "BT601" : "BT709");
    sdp << "; PM=2110GPM; SSN=ST2110-20:2017; TP=2110TPN; ";
    if (!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(vfmt))
    {
        sdp << "interlace";
    }
    else if (NTV2_IS_PSF_VIDEO_FORMAT(vfmt))
    {
        sdp << "interlace segmented";
    }
    sdp << endl;

    // PTP
    sdp << "a=tsrefclk:ptp=IEEE1588-2008:" << gmInfo << endl;
    sdp << "a=mediaclk:direct=0" << endl;
    sdp << "a=mid:VID" << endl;

    return true;
}


bool CNTV2Config2110::GenSDPAudioStream(stringstream & sdp, NTV2Channel channel, NTV2Stream stream, string gmInfo)
{
    // TODO - fix this to work with link B
    bool enabledA;
    bool enabledB;
    GetTxChannelEnable(channel,stream,enabledA,enabledB);
    if (!enabledA)
    {
        return true;
    }

    tx_2110Config config;
    GetTxChannelConfiguration(channel, stream, config);

    // media name
    sdp << "m=audio ";
    sdp << To_String(config.remotePort[0]);     // FIXME
    sdp << " RTP/AVP ";
    sdp << To_String(config.payloadType) << endl;

    // connection information
    sdp << "c=IN IPV4 ";
    sdp << config.remoteIP;
    sdp << "/" << To_String(config.ttl) << endl;

    // rtpmap
    sdp << "a=rtpmap:";
    sdp << To_String(config.payloadType);
    sdp << " L24/48000/";
    sdp << To_String(config.numAudioChannels) << endl;

    //fmtp
    sdp << "a=fmtp:";
    sdp << To_String(config.payloadType);
    sdp << " channel-order=SMPTE2110.(";
    switch (config.numAudioChannels)
    {
    case 2:
        sdp << "ST)";
        break;
    case 4:
        sdp << "SGRP)";
        break;
    case 8:
    default:
        sdp << "SGRP,SGRP)";
        break;
    case 12:
        sdp << "SGRP,SGRP,SGRP)";
        break;
    case 16:
        sdp << "SGRP,SGRP,SGRP,SGRP)";
        break;
    }
    sdp << endl;

    if (config. audioPacketInterval == PACKET_INTERVAL_125uS)
        sdp << "a=ptime:0.125" << endl;
    else
        sdp << "a=ptime:1.000" << endl;

    sdp << "a=tsrefclk:ptp=IEEE1588-2008:" << gmInfo << endl;
    sdp << "a=mediaclk:direct=0" << endl;
    sdp << "a=mid:AUD" << endl;

    return true;
}

bool  CNTV2Config2110::GetRxSDP(std::string url, std::string & sdp)
{
    return GetSDP(url, sdp);
}


int CNTV2Config2110::getDescriptionValue(int startLine, string type, string & value)
{
    for (unsigned i(startLine);  i < sdpLines.size();  i++)
    {
        string line = sdpLines[i];
        size_t pos = line.find(type);
        if (pos != string::npos)
        {
            value = line.substr(pos + type.size() + 1);
            return i;
        }
    }
    return -1; // not found
}

string CNTV2Config2110::getVideoDescriptionValue(string type)
{
    vector<string>::iterator it;
    for (it = tokens.begin(); it != tokens.end(); it++)
    {
        string line = *it;
        size_t pos = line.find(type);
        if (pos != string::npos)
        {
            line = line.substr(pos + type.size());
            line.erase(remove(line.begin(), line.end(), ';'), line.end());
            return  line;
        }
    }
    string result;
    return result; // not found
}

vector<string> CNTV2Config2110::split(const char *str, char delim)
{
    vector<string> result;
    do
    {
        const char * begin = str;
        while(*str != delim && *str)
        {
            str++;
        }
        result.push_back(string(begin, str));
    } while (0 != *str++);
    return result;
}

bool CNTV2Config2110::ExtractRxConfigFromSDP(std::string sdp, NTV2Stream stream, rx_2110Config & rxConfig)
{
    if (sdp.empty())
    {
        mIpErrorCode = NTV2IpErrSDPEmpty;
        return false;
    }

    uint32_t rxMatch = 0;

    // break into a vector of lines and then into tokenw

    sdpLines.clear();
    stringstream ss(sdp);
    string to;

    while(getline(ss,to,'\n'))
    {
        sdpLines.push_back(to);
    }

    // rudimentary check it is an sdp file
    int index;
    string value;

    // is this really an SDP
    index = getDescriptionValue(0,"v=",value);
    if (index == -1)
    {
        mIpErrorCode = NTV2IpErrSDPInvalid;
        return false;
    }

    // originator
    index = getDescriptionValue(index,"o=",value);
    if (index == -1)
    {
        mIpErrorCode = NTV2IpErrSDPInvalid;
        return false;
    }

    tokens = split(value.c_str(), ' ');
    if ((tokens.size() >= 6) && (tokens[3] == "IN") && (tokens[4] == "IP4"))
    {
        if (!tokens[5].empty())
        {
            rxConfig.sourceIP = tokens[5];
            rxMatch |= RX_MATCH_2110_SOURCE_IP;
        }
    }

    int rv = getDescriptionValue(0,"c=IN",value);
    if (rv >= index)
    {
        tokens = split(value.c_str(), ' ');
        if (tokens.size() >= 2)
        {
            tokens = split(tokens[1].c_str(), '/');
            if ((tokens.size() >= 1) && !tokens[0].empty())
            {
                rxConfig.destIP = tokens[0];
                rxMatch |= RX_MATCH_2110_DEST_IP;
            }
        }
    }

    if (stream == NTV2_VIDEO_STREAM)
    {
        index = getDescriptionValue(index,"m=video",value);
        if (index == -1)
        {
            // does not contain video
            mIpErrorCode = NTV2IpErrSDPNoVideo;
            return false;
        }
        tokens = split(value.c_str(), ' ');
        if ((tokens.size() >= 1) && !tokens[0].empty())
        {
            rxConfig.destPort    = atoi(tokens[0].c_str());
            rxMatch |= RX_MATCH_2110_DEST_PORT;
        }
        if ((tokens.size() >= 3) && !tokens[2].empty())
        {
            rxConfig.payloadType = atoi(tokens[2].c_str());
            rxMatch |= RX_MATCH_2110_PAYLOAD;
        }

        int rv = getDescriptionValue(index,"c=IN",value);
        if (rv >= index)
        {
            // this overwrites if found before
            tokens = split(value.c_str(), ' ');
            if (tokens.size() >= 2)
            {
                tokens = split(tokens[1].c_str(), '/');
                if ((tokens.size() >= 1) && !tokens[0].empty())
                {
                    rxConfig.destIP = tokens[0];
                    rxMatch |= RX_MATCH_2110_DEST_IP;
                }
            }
        }

        rv = getDescriptionValue(index,"a=rtpmap",value);
        if (rv > index)
        {
            tokens = split(value.c_str(), ' ');
            if ((tokens.size() >= 1) && !tokens[0].empty())
            {
                rxConfig.payloadType = atoi(tokens[0].c_str());
                rxMatch |= RX_MATCH_2110_PAYLOAD;
            }
        }

        rv = getDescriptionValue(index,"a=fmtp",value);
        if (rv > index)
        {
            tokens = split(value.c_str(), ' ');
            string sampling = getVideoDescriptionValue("sampling=");
            string width    = getVideoDescriptionValue("width=");
            string height   = getVideoDescriptionValue("height=");
            string rate     = getVideoDescriptionValue("exactframerate=");
            bool interlace = false;
            vector<string>::iterator it;
            for (it = tokens.begin(); it != tokens.end(); it++)
            {
                if (*it == "interlace")
                {
                    interlace = true;
                }
            }
            int w = atoi(width.c_str());
            int h = atoi(height.c_str());
            NTV2FrameRate r = stringToRate(rate);
            NTV2VideoFormat vf = ::GetFirstMatchingVideoFormat(r,h,w,interlace);
            rxConfig.videoFormat = vf;
        }
        rxConfig.rxMatch = rxMatch;
        return true;
    }
    else
    {
        // audio stream
        index = getDescriptionValue(index,"m=audio",value);
        if (index == -1)
        {
            // does not contain audio
            mIpErrorCode = NTV2IpErrSDPNoAudio;
            return false;
        }

        tokens = split(value.c_str(), ' ');
        if ((tokens.size() >= 1) && !tokens[0].empty())
        {
            rxConfig.destPort    = atoi(tokens[0].c_str());
            rxMatch |= RX_MATCH_2110_DEST_PORT;
        }

        if ((tokens.size() >= 3) && !tokens[2].empty())
        {
            rxConfig.payloadType = atoi(tokens[2].c_str());
            rxMatch |= RX_MATCH_2110_PAYLOAD;
        }

        int rv = getDescriptionValue(index,"c=IN",value);
        if (rv >= index)
        {
            // this overwrites if found before
            tokens = split(value.c_str(), ' ');
            if ((tokens.size() >= 2))
            {
                tokens = split(tokens[1].c_str(), '/');
                if ((tokens.size() >= 1)&& !tokens[0].empty())
                {
                    rxConfig.destIP = tokens[0];
                    rxMatch |= RX_MATCH_2110_DEST_IP;
                }
            }
        }

        rv = getDescriptionValue(index,"a=rtpmap",value);
        if (rv > index)
        {
            tokens = split(value.c_str(), ' ');
            if ((tokens.size() >= 1)&& !tokens[0].empty())
            {
                rxConfig.payloadType = atoi(tokens[0].c_str());
                rxMatch |= RX_MATCH_2110_PAYLOAD;
            }
            if ((tokens.size() >= 2))
            {
                tokens = split(tokens[1].c_str(), '/');
                if ((tokens.size() >= 3) && !tokens[2].empty())
                {
					rxConfig.numAudioChannels = atoi(tokens[2].c_str());
                }
            }
        }
        rxConfig.rxMatch = rxMatch;
        return true;
    }
    return false;
}


std::string CNTV2Config2110::rateToString(NTV2FrameRate rate)
{
    string rateString;
    switch (rate)
    {
     default:
     case   NTV2_FRAMERATE_UNKNOWN  :
        rateString = "00";
        break;
     case   NTV2_FRAMERATE_6000     :
        rateString = "60";
        break;
     case   NTV2_FRAMERATE_5994     :
        rateString = "60000/1001";
        break;
     case   NTV2_FRAMERATE_3000     :
        rateString = "30";
        break;
     case   NTV2_FRAMERATE_2997     :
        rateString = "30000/1001";
        break;
     case   NTV2_FRAMERATE_2500     :
        rateString = "25";
        break;
     case   NTV2_FRAMERATE_2400     :
        rateString = "24";
        break;
     case   NTV2_FRAMERATE_2398     :
        rateString = "24000/1001";
        break;
     case   NTV2_FRAMERATE_5000     :
        rateString = "50";
        break;
     case   NTV2_FRAMERATE_4800     :
        rateString = "48";
        break;
     case   NTV2_FRAMERATE_4795     :
        rateString = "48000/1001";
        break;
     case   NTV2_FRAMERATE_12000    :
        rateString = "12";
        break;
     case   NTV2_FRAMERATE_11988    :
        rateString = "12000/1001";
        break;
     case   NTV2_FRAMERATE_1500     :
        rateString = "15";
        break;
     case   NTV2_FRAMERATE_1498     :
        rateString = "1500/1001";
        break;
    }
    return rateString;
}

NTV2FrameRate CNTV2Config2110::stringToRate(std::string rateString)
{
    NTV2FrameRate rate;
    if (rateString == "60")
        rate = NTV2_FRAMERATE_6000;
    else if (rateString == "60000/1001")
        rate = NTV2_FRAMERATE_5994;
    else if (rateString == "30")
        rate = NTV2_FRAMERATE_3000;
    else if (rateString == "30000/1001")
        rate = NTV2_FRAMERATE_2997;
    else if (rateString == "25")
        rate = NTV2_FRAMERATE_2500;
    else if (rateString == "24")
        rate = NTV2_FRAMERATE_2400;
    else if (rateString == "24000/1001")
        rate = NTV2_FRAMERATE_2398;
    else if (rateString == "50")
        rate = NTV2_FRAMERATE_5000;
    else if (rateString == "48")
        rate = NTV2_FRAMERATE_4800;
    else if (rateString == "48000/1001")
        rate = NTV2_FRAMERATE_4795;
    else if (rateString == "12")
        rate = NTV2_FRAMERATE_12000;
    else if (rateString == "12000/1001")
        rate = NTV2_FRAMERATE_11988;
    else if (rateString == "15")
        rate = NTV2_FRAMERATE_1500;
    else if (rateString == "1500/1001")
        rate = NTV2_FRAMERATE_1498;
    else
        rate = NTV2_FRAMERATE_UNKNOWN;
    return rate;
}

void CNTV2Config2110::SetArbiter(NTV2Channel channel, NTV2Stream stream,eSFP link,bool enable)
{
    uint32_t reg;
    if (stream == NTV2_VIDEO_STREAM)
    {
        reg = kRegArb_video + SAREK_2110_TX_ARBITRATOR;
    }
    else
    {
        reg = kRegArb_audio +  SAREK_2110_TX_ARBITRATOR;
    }
    uint32_t val;
    mDevice.ReadRegister(reg,&val);

    uint32_t bit = (1 << get2110TxStream(channel,stream)) << (int(link) * 16);
    if (enable)
        val |= bit;
    else
        val &= ~bit;

    mDevice.WriteRegister(reg,val);
}

void CNTV2Config2110::GetArbiter(NTV2Channel channel, NTV2Stream stream,eSFP link,bool & enable)
{
    uint32_t reg;
    if (stream == NTV2_VIDEO_STREAM)
        reg = kRegArb_video + SAREK_2110_TX_ARBITRATOR;
    else
        reg = kRegArb_audio + SAREK_2110_TX_ARBITRATOR;

    uint32_t val;
    mDevice.ReadRegister(reg,&val);

    uint32_t bit = (1 << get2110TxStream(channel,stream)) << (int(link) * 16);
    enable = (val & bit);
}
