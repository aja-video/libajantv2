/**
    @file       ntv2config2110.cpp
    @brief      Implements the CNTV2Config2110 class.
    @copyright  (C) 2014-2018 AJA Video Systems, Inc.   Proprietary and confidential information.
**/

#include "ntv2config2110.h"
#include "ntv2endian.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include <sstream>
#include <algorithm>

#if defined (AJALinux) || defined (AJAMac)
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#endif

uint32_t CNTV2Config2110::v_packetizers[4] = {SAREK_4175_TX_PACKETIZER_1, SAREK_4175_TX_PACKETIZER_2, SAREK_4175_TX_PACKETIZER_3, SAREK_4175_TX_PACKETIZER_4};
uint32_t CNTV2Config2110::a_packetizers[16] ={SAREK_3190_TX_PACKETIZER_0, SAREK_3190_TX_PACKETIZER_1, SAREK_3190_TX_PACKETIZER_2, SAREK_3190_TX_PACKETIZER_3,
                                              SAREK_3190_TX_PACKETIZER_4, SAREK_3190_TX_PACKETIZER_5, SAREK_3190_TX_PACKETIZER_6, SAREK_3190_TX_PACKETIZER_7,
                                              SAREK_3190_TX_PACKETIZER_8, SAREK_3190_TX_PACKETIZER_9, SAREK_3190_TX_PACKETIZER_10,SAREK_3190_TX_PACKETIZER_11,
                                              SAREK_3190_TX_PACKETIZER_12,SAREK_3190_TX_PACKETIZER_13,SAREK_3190_TX_PACKETIZER_14,SAREK_3190_TX_PACKETIZER_15};
uint32_t CNTV2Config2110::m_packetizers[4] = {SAREK_ANC_TX_PACKETIZER_1,  SAREK_ANC_TX_PACKETIZER_2,  SAREK_ANC_TX_PACKETIZER_3,  SAREK_ANC_TX_PACKETIZER_4};

using namespace std;

void tx_2110Config::init()
{
    for (int i=0; i < 2; i++)
    {
        localPort[i]   = 0;
        remotePort[i]  = 0;
        remoteIP[i].erase();
    }
    videoFormat         = NTV2_FORMAT_UNKNOWN;
    videoSamples        = VPIDSampling_YUV_422;
    payloadLen          = 0;
    lastPayLoadLen      = 0;
    pktsPerLine         = 0;
    ttl                 = 0x40;
    tos                 = 0x64;
    ssrc                = 1000;
    numAudioChannels    = 0;
    firstAudioChannel   = 0;
    audioPacketInterval = PACKET_INTERVAL_1mS;
}

bool tx_2110Config::operator != ( const tx_2110Config &other )
{
    return !(*this == other);
}

bool tx_2110Config::operator == ( const tx_2110Config &other )
{
    if ((localPort              == other.localPort)             &&
        (remotePort             == other.remotePort)            &&
        (remoteIP               == other.remoteIP)              &&
        (videoFormat            == other.videoFormat)           &&
        (videoSamples           == other.videoSamples)          &&
        (numAudioChannels       == other.numAudioChannels)      &&
        (firstAudioChannel      == other.firstAudioChannel)     &&
        (audioPacketInterval    == other.audioPacketInterval))
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
    rxMatch             = 0;
    sourceIP.erase();
    destIP.erase();
    sourcePort          = 0;
    destPort            = 0;
    ssrc                = 1000;
    vlan                = 1;
    payloadType         = 0;
    videoFormat         = NTV2_FORMAT_UNKNOWN;
    videoSamples        = VPIDSampling_YUV_422;
    payloadLen          = 0;
    lastPayloadLen      = 0;
    pktsPerLine         = 0;
    numAudioChannels    = 2;
    audioPacketInterval = PACKET_INTERVAL_1mS;
}

bool rx_2110Config::operator != ( const rx_2110Config &other )
{
    return (!(*this == other));
}

bool rx_2110Config::operator == ( const rx_2110Config &other )
{
    if (    (rxMatch           == other.rxMatch)            &&
            (sourceIP          == other.sourceIP)           &&
            (destIP            == other.destIP)             &&
            (sourcePort        == other.sourcePort)         &&
            (destPort          == other.destPort)           &&
            (ssrc              == other.ssrc)               &&
            (vlan              == other.vlan)               &&
            (videoFormat       == other.videoFormat)        &&
            (videoSamples      == other.videoSamples)       &&
            (numAudioChannels == other.numAudioChannels)    &&
            (audioPacketInterval == other.audioPacketInterval))
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

bool CNTV2Config2110::SetNetworkConfiguration(const eSFP sfp, const IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    struct in_addr addr;
    addr.s_addr = (uint32_t)netConfig.ipc_ip;
    ip = inet_ntoa(addr);
    addr.s_addr = (uint32_t)netConfig.ipc_subnet;
    subnet = inet_ntoa(addr);
    addr.s_addr = (uint32_t)netConfig.ipc_gateway;
    gateway = inet_ntoa(addr);

    bool rv = SetNetworkConfiguration(sfp, ip, subnet, gateway);
    return rv;
}

bool CNTV2Config2110::GetNetworkConfiguration(const eSFP sfp, IPVNetConfig & netConfig)
{
    string ip, subnet, gateway;
    GetNetworkConfiguration(sfp, ip, subnet, gateway);

    netConfig.ipc_ip      = NTV2EndianSwap32((uint32_t)inet_addr(ip.c_str()));
    netConfig.ipc_subnet  = NTV2EndianSwap32((uint32_t)inet_addr(subnet.c_str()));
    netConfig.ipc_gateway = NTV2EndianSwap32((uint32_t)inet_addr(gateway.c_str()));

    return true;
}

bool CNTV2Config2110::SetNetworkConfiguration(const eSFP sfp, const std::string localIPAddress, const std::string subnetMask, const std::string gateway)
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

    // get sfp1 mac address
    uint32_t macAddressRegister = SAREK_REGS + kRegSarekMAC;
    mDevice.ReadRegister(macAddressRegister, &macHi);
    macAddressRegister++;
    mDevice.ReadRegister(macAddressRegister, &macLo);

    uint32_t boardHi = (macHi & 0xffff0000) >>16;
    uint32_t boardLo = ((macHi & 0x0000ffff) << 16) + ((macLo & 0xffff0000) >> 16);

    // get sfp2 mac address
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

    bool rv = SetMBNetworkConfiguration (sfp, localIPAddress, subnetMask, gateway);
    if (!rv) return false;

    if (sfp == SFP_1)
    {
        ConfigurePTP(sfp,localIPAddress);
    }

    return true;
}

bool CNTV2Config2110::GetNetworkConfiguration(const eSFP sfp, std::string & localIPAddress, std::string & subnetMask, std::string & gateway)
{
    struct in_addr addr;

    if (sfp == SFP_1)
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

bool  CNTV2Config2110::DisableNetworkInterface(const eSFP sfp)
{
    return CNTV2MBController::DisableNetworkInterface(sfp);
}

bool CNTV2Config2110::DisableRxStream(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream)
{
    if (GetSFPActive(sfp) == false)
    {
        mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        return false;
    }

    // disable IGMP subscription
    bool disableIGMP;
    GetIGMPDisable(sfp, disableIGMP);
    if (!disableIGMP)
    {
        EnableIGMPGroup(sfp, channel, stream, false);
    }

    DisableDecapsulatorStream(sfp, channel, stream);
    DisableDepacketizerStream(channel, stream);
    return true;
}

bool CNTV2Config2110::EnableRxStream(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, rx_2110Config & rxConfig)
{
    if (GetSFPActive(sfp) == false)
    {
        mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        return false;
    }

    // make IGMP subsciption if needed
    uint32_t destIp = inet_addr(rxConfig.destIP.c_str());
    destIp = NTV2EndianSwap32(destIp);

    uint32_t srcIp = inet_addr(rxConfig.sourceIP.c_str());
    srcIp = NTV2EndianSwap32(srcIp);

    uint8_t ip0 = (destIp & 0xff000000)>> 24;
    if (ip0 >= 224 && ip0 <= 239)
    {
        // is multicast
        SetIGMPGroup(sfp, channel, stream, destIp, srcIp, true);
    }
    else
    {
        UnsetIGMPGroup(sfp, channel, stream);
    }

    DisableDecapsulatorStream(sfp, channel, stream);
    DisableDepacketizerStream(channel,stream);

    ResetDepacketizerStream(channel,stream);
    SetupDepacketizerStream(channel,stream,rxConfig);
    SetupDecapsulatorStream(sfp, channel, stream, rxConfig);

    EnableDepacketizerStream(channel, stream);
    EnableDecapsulatorStream(sfp, channel, stream);

    return true;
}

void  CNTV2Config2110::SetupDecapsulatorStream(eSFP sfp, NTV2Channel channel, NTV2Stream stream, rx_2110Config & rxConfig)
{
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp,channel,stream);

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
    mDevice.WriteRegister(kRegDecap_match_ssrc + decapBaseAddr, rxConfig.ssrc);

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

void  CNTV2Config2110::DisableDecapsulatorStream(eSFP sfp, NTV2Channel channel, NTV2Stream stream)
{
    // disable decasulator
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp,channel,stream);
    mDevice.WriteRegister(kRegDecap_chan_enable + decapBaseAddr, 0x00);
}

void  CNTV2Config2110::EnableDecapsulatorStream(eSFP sfp, NTV2Channel channel, NTV2Stream stream)
{
    // enable decasulator
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp,channel,stream);
    mDevice.WriteRegister(kRegDecap_chan_enable + decapBaseAddr, 0x01);
}

void  CNTV2Config2110::ResetPacketizerStream(const NTV2Channel channel, NTV2Stream stream)
{
    if (stream == NTV2_VIDEO_STREAM)
    {

        uint32_t    val;
        uint32_t    bit;

        switch(channel)
        {
            default:
            case NTV2_CHANNEL1:
                bit = BIT(16);
                break;
            case NTV2_CHANNEL2:
                bit = BIT(17);
                break;
            case NTV2_CHANNEL3:
                bit = BIT(18);
                break;
            case NTV2_CHANNEL4:
                bit = BIT(19);
                break;
        }

        // read/modify/write reset bit for a given channel
        mDevice.ReadRegister(kRegSarekRxReset + SAREK_REGS, &val);

        // Set reset bit
        val |= bit;
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, val);

        // Wait just a bit
        #if defined(AJAWindows) || defined(MSWindows)
            ::Sleep (50);
        #else
            usleep (50 * 1000);
        #endif

        // Unset reset bit
        val &= ~bit;
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, val);
    }
}

void  CNTV2Config2110::ResetDepacketizerStream(const NTV2Channel channel, NTV2Stream stream)
{
    if (stream == NTV2_VIDEO_STREAM)
    {
        uint32_t    val;
        uint32_t    bit;

        switch(channel)
        {
        default:
        case NTV2_CHANNEL1:
            bit = BIT(0);
            break;
        case NTV2_CHANNEL2:
            bit = BIT(1);
            break;
        case NTV2_CHANNEL3:
            bit = BIT(2);
            break;
        case NTV2_CHANNEL4:
            bit = BIT(3);
            break;
        }

        // read/modify/write reset bit for a given channel
        mDevice.ReadRegister(kRegSarekRxReset + SAREK_REGS, &val);

        // Set reset bit
        val |= bit;
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, val);

        // Wait just a bit
        #if defined(AJAWindows) || defined(MSWindows)
            ::Sleep (50);
        #else
            usleep (50 * 1000);
        #endif

        // Unset reset bit
        val &= ~bit;
        mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, val);
    }
}

void  CNTV2Config2110::SetupDepacketizerStream(const NTV2Channel channel, NTV2Stream stream, const rx_2110Config & rxConfig)
{
    if (stream == NTV2_VIDEO_STREAM)
    {
        SetVideoFormatForRxTx(channel, (NTV2VideoFormat) rxConfig.videoFormat, true);
    }

    else if (stream == NTV2_AUDIO1_STREAM)
    {
        // setup 3190 depacketizer
        uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(channel,stream);

        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr, 0x00);
        uint32_t num_samples = (rxConfig.audioPacketInterval == PACKET_INTERVAL_125uS) ? 6 : 48;
		uint32_t num_channels = rxConfig.numAudioChannels;
        uint32_t val = (num_samples << 8) + num_channels;
        mDevice.WriteRegister(kReg3190_depkt_config + depacketizerBaseAddr,val);

        // audio channels
        mDevice.WriteRegister(kReg3190_depkt_enable + depacketizerBaseAddr,0x01);
    }
}

void CNTV2Config2110::SetVideoFormatForRxTx(const NTV2Channel channel, const NTV2VideoFormat format, const bool rx)
{
    NTV2FormatDescriptor fd(format, NTV2_FBF_10BIT_YCBCR);

    NTV2FrameRate       fr  = GetNTV2FrameRateFromVideoFormat(format);
    NTV2FrameGeometry   fg  = fd.GetFrameGeometry();
    NTV2Standard        std = fd.GetVideoStandard();
    bool               is2K = fd.Is2KFormat();

    uint32_t val = ( (((uint32_t) fr) << 8) |
                     (((uint32_t) fg) << 4) |
                      ((uint32_t) std ) );
    if (is2K)
        val += BIT(13);

    if (NTV2_IS_PSF_VIDEO_FORMAT (format))
        val += BIT(15);

    uint32_t reg, reg2;
    if (rx)
    {
        reg = channel - NTV2_CHANNEL1 + kRegRxVideoDecode1;
        reg2 = channel - NTV2_CHANNEL1 + kRegRxNtv2VideoDecode1;
    }
    else
    {
        reg = channel - NTV2_CHANNEL1 + kRegTxVideoDecode1;
        reg2 = channel - NTV2_CHANNEL1 + kRegTxNtv2VideoDecode1;
    }

    // Write the format for the firmware and also tuck it away in NTV2 flavor so we can retrieve it easily
    mDevice.WriteRegister(reg + SAREK_2110_TX_ARBITRATOR, val);
    mDevice.WriteRegister(reg2 + SAREK_REGS2, format);

}

void CNTV2Config2110::GetVideoFormatForRxTx(const NTV2Channel channel, NTV2VideoFormat & format, uint32_t & hwFormat, const bool rx)
{
    uint32_t reg, reg2, value;
    if (rx)
    {
        reg = channel - NTV2_CHANNEL1 + kRegRxVideoDecode1;
        reg2 = channel - NTV2_CHANNEL1 + kRegRxNtv2VideoDecode1;
    }
    else
    {
        reg = channel - NTV2_CHANNEL1 + kRegTxVideoDecode1;
        reg2 = channel - NTV2_CHANNEL1 + kRegTxNtv2VideoDecode1;
    }

    // Write the format for the firmware and also tuck it away in NTV2 flavor so we can retrieve it easily
    mDevice.ReadRegister(reg + SAREK_2110_TX_ARBITRATOR, &value);
    hwFormat = value;
    mDevice.ReadRegister(reg2 + SAREK_REGS2, &value);
    format = (NTV2VideoFormat)value;
}

bool  CNTV2Config2110::GetRxStreamConfiguration(const eSFP sfp, const NTV2Channel channel, NTV2Stream stream, rx_2110Config & rxConfig)
{
    uint32_t    val;

    // get address,strean
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp, channel, stream);

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
    mDevice.ReadRegister(kRegDecap_match_ssrc + decapBaseAddr, &rxConfig.ssrc);

    // vlan
    //mDevice.ReadRegister(kRegDecap_match_vlan + decapBaseAddr, &val);
    //rxConfig.VLAN = val & 0xffff;

    // payload type
    mDevice.ReadRegister(kRegDecap_match_payload + decapBaseAddr, &val);
    rxConfig.payloadType = val & 0x7f;

    // matching
    mDevice.ReadRegister(kRegDecap_match_sel + decapBaseAddr, &rxConfig.rxMatch);

    uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(channel, stream);

    if (stream == NTV2_VIDEO_STREAM)
    {
        NTV2VideoFormat     format;
        uint32_t            hwFormat;

        GetVideoFormatForRxTx(channel, format, hwFormat, true);
        rxConfig.videoFormat = format;
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        uint32_t samples;
        mDevice.ReadRegister(kReg3190_depkt_config + depacketizerBaseAddr, &samples);
        rxConfig.audioPacketInterval = (((samples >> 8) & 0xff) == 6) ? PACKET_INTERVAL_125uS : PACKET_INTERVAL_1mS;
        rxConfig.numAudioChannels = samples & 0xff;
    }

    return true;
}

bool CNTV2Config2110::GetRxStreamEnable(const eSFP sfp, const NTV2Channel channel, NTV2Stream stream, bool & enabled)
{
    // get address
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(sfp, channel, stream);

    uint32_t val;
    mDevice.ReadRegister(kRegDecap_chan_enable + decapBaseAddr,&val);
    enabled = (val & 0x01);

    return true;
}

bool CNTV2Config2110::GetRxPacketCount(const NTV2Channel channel, NTV2Stream stream, uint32_t &packets)
{
    uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(channel,stream);
    if (stream == NTV2_VIDEO_STREAM)
    {
        mDevice.ReadRegister(kReg4175_depkt_rx_pkt_cnt+ depacketizerBaseAddr, &packets);
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        // TODO:
        // mDevice.ReadRegister(kReg3190_rx_byte_cnt + depacketizerBaseAddr, &packets);
        packets = 0;
    }

    return true;
}

bool CNTV2Config2110::GetRxByteCount(const NTV2Channel channel, NTV2Stream stream, uint32_t &bytes)
{
    uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(channel,stream);
    if (stream == NTV2_VIDEO_STREAM)
    {
        mDevice.ReadRegister(kReg4175_depkt_rx_byte_cnt+ depacketizerBaseAddr, &bytes);
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
		// TODO: 
		// mDevice.ReadRegister(kReg3190_rx_byte_cnt + depacketizerBaseAddr, &bytes);
		bytes = 0;
    }

    return true;
}

bool CNTV2Config2110::GetRxByteCount(const eSFP sfp, uint64_t &bytes)
{
    uint32_t val_lo, val_hi;

    if (sfp == SFP_1)
    {
        mDevice.ReadRegister(SAREK_10G_EMAC_0 + kReg10gemac_rx_bytes_lo, &val_lo);
        mDevice.ReadRegister(SAREK_10G_EMAC_0 + kReg10gemac_rx_bytes_hi, &val_hi);
    }
    else
    {
        mDevice.ReadRegister(SAREK_10G_EMAC_1 + kReg10gemac_rx_bytes_lo, &val_lo);
        mDevice.ReadRegister(SAREK_10G_EMAC_1 + kReg10gemac_rx_bytes_hi, &val_hi);
    }

    bytes = ((uint64_t)val_hi << 32) + val_lo;
    return true;
}

bool CNTV2Config2110::GetTxPacketCount(const NTV2Channel channel, NTV2Stream stream, uint32_t &packets)
{
    if (stream == NTV2_VIDEO_STREAM)
    {
        uint32_t baseAddrPacketizer;
        SetTxPacketizerChannel(channel,NTV2_VIDEO_STREAM,baseAddrPacketizer);

        uint32_t count;
        mDevice.ReadRegister(kReg4175_pkt_tx_pkt_cnt + baseAddrPacketizer,&count);
        packets = count;
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        // TODO:
        packets = 0;
    }

    return true;
}

bool CNTV2Config2110::GetTxByteCount(const eSFP sfp, uint64_t &bytes)
{
    uint32_t val_lo, val_hi;

    if (sfp == SFP_1)
    {
        mDevice.ReadRegister(SAREK_10G_EMAC_0 + kReg10gemac_tx_bytes_lo, &val_lo);
        mDevice.ReadRegister(SAREK_10G_EMAC_0 + kReg10gemac_tx_bytes_hi, &val_hi);
    }
    else
    {
        mDevice.ReadRegister(SAREK_10G_EMAC_1 + kReg10gemac_tx_bytes_lo, &val_lo);
        mDevice.ReadRegister(SAREK_10G_EMAC_1 + kReg10gemac_tx_bytes_hi, &val_hi);
    }

    bytes = ((uint64_t)val_hi << 32) + val_lo;
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

bool CNTV2Config2110::SetTxStreamConfiguration(const NTV2Channel channel, const NTV2Stream stream, const tx_2110Config & txConfig)
{
    bool        rv = true;

    if (GetSFPActive(SFP_1) == false)
    {
        mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        return false;
    }

    ResetPacketizerStream(channel, stream);

    SetFramerStream(SFP_1, channel, stream, txConfig);
    SetFramerStream(SFP_2, channel, stream, txConfig);

    // packetizer
    uint32_t baseAddrPacketizer;
    SetTxPacketizerChannel(channel,stream,baseAddrPacketizer);

    if (stream == NTV2_VIDEO_STREAM)
    {        
        NTV2VideoFormat fmt = txConfig.videoFormat;

        // Write the video format into the arbitrator
        SetVideoFormatForRxTx(channel, fmt, false);

        // setup 4175 packetizer
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

bool CNTV2Config2110::SetFramerStream(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, const tx_2110Config & txConfig)
{
    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(sfp, channel,stream);

    // select channel
    SelectTxFramerChannel(channel, stream, baseAddrFramer);

    // setup framer
    // hold off access while we update channel regs
    AcquireFramerControlAccess(baseAddrFramer);

    uint32_t val = (txConfig.tos << 8) | txConfig.ttl;
    WriteChannelRegister(kRegFramer_ip_hdr_media + baseAddrFramer, val);

    int index = (int)sfp;
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
    bool rv = GetMACAddress(sfp,channel,stream,txConfig.remoteIP[index],hi,lo);
    if (!rv) return false;
    WriteChannelRegister(kRegFramer_dest_mac_lo  + baseAddrFramer,lo);
    WriteChannelRegister(kRegFramer_dest_mac_hi  + baseAddrFramer,hi);

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // end framer setup
    return true;
}

bool CNTV2Config2110::GetTxStreamConfiguration(const NTV2Channel channel, const NTV2Stream stream, tx_2110Config & txConfig)
{

    GetFramerStream(channel,stream,SFP_1,txConfig);
    GetFramerStream(channel,stream,SFP_2,txConfig);

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

void CNTV2Config2110::GetFramerStream(NTV2Channel channel, NTV2Stream stream, eSFP sfp, tx_2110Config  & txConfig)
{
    int index = (int)sfp;

    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(sfp,channel,stream);

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

bool CNTV2Config2110::SetTxStreamEnable(const NTV2Channel channel, const NTV2Stream stream, bool enableSfp1, bool enableSfp2)
{
    if (enableSfp1 && (GetSFPActive(SFP_1) == false))
    {
        mIpErrorCode = NTV2IpErrSFP1NotConfigured;
        return false;
    }

    if (enableSfp2 && (GetSFPActive(SFP_2) == false))
    {
        mIpErrorCode = NTV2IpErrSFP2NotConfigured;
        return false;
    }

    EnableFramerStream(SFP_1, channel, stream, enableSfp1);
    EnableFramerStream(SFP_2, channel, stream, enableSfp2);
    SetArbiter(SFP_1, channel, stream, enableSfp1);
    SetArbiter(SFP_2, channel, stream, enableSfp2);

    // ** Packetizer
    uint32_t packetizerBaseAddr;
    SetTxPacketizerChannel(channel,stream,packetizerBaseAddr);

    if (enableSfp1 || enableSfp2)
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

void CNTV2Config2110::EnableFramerStream(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, bool enable)
{
    // ** Framer
    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(sfp, channel, stream);

    // select channel
    SelectTxFramerChannel(channel, stream, baseAddrFramer);

    // hold off access while we update channel regs
    AcquireFramerControlAccess(baseAddrFramer);

    if (enable)
    {
        uint32_t localIp;
        if (sfp == SFP_1)
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP0, &localIp);
        }
        else
        {
            mDevice.ReadRegister(SAREK_REGS + kRegSarekIP1, &localIp);
        }

        WriteChannelRegister(kRegFramer_src_ip + baseAddrFramer, NTV2EndianSwap32(localIp));

        // enable
        WriteChannelRegister(kRegFramer_chan_ctrl + baseAddrFramer, 0x01);  // enables tx over mac1/mac2
    }
    else
    {
        // disable
        WriteChannelRegister(kRegFramer_chan_ctrl + baseAddrFramer, 0x0);   // disables channel
    }

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // ** Framer end
}

bool CNTV2Config2110::GetTxStreamEnable(const NTV2Channel channel, const NTV2Stream stream, bool & sfp1Enabled, bool & sfp2Enabled)
{
    GetArbiter(SFP_1, channel, stream, sfp1Enabled);
    GetArbiter(SFP_2, channel, stream, sfp2Enabled);
    return true;
}

bool  CNTV2Config2110::SetPTPMaster(const std::string ptpMaster)
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

bool CNTV2Config2110::GetPTPStatus(PTPStatus & ptpStatus)
{
    uint32_t val = 0;
    mDevice.ReadRegister(SAREK_PLL + kRegPll_Status, &val);

    ptpStatus.PTP_packetStatus      = (val & BIT(16)) ? true : false;
    ptpStatus.PTP_frequencyLocked   = (val & BIT(17)) ? true : false;
    ptpStatus.PTP_phaseLocked       = (val & BIT(18)) ? true : false;

    return true;
}

bool CNTV2Config2110::Set4KMode(bool enable)
{
    if (!mDevice.IsMBSystemReady())
    {
        mIpErrorCode = NTV2IpErrNotReady;
        return false;
    }

    bool old_enable = false;
    Get4KMode(old_enable);
    bool enableChange = (old_enable != enable);

    if (enableChange)
    {
        uint32_t reg;
        reg = kRegArb_4KMode + SAREK_2110_TX_ARBITRATOR;

        uint32_t val;
        mDevice.ReadRegister(reg,&val);
        if (enable)
            val |= BIT(0);
        else
            val &= ~BIT(0);

        mDevice.WriteRegister(reg,val);
    }

    return true;
}

bool  CNTV2Config2110::Get4KMode(bool & enable)
{
    uint32_t reg;
    reg = kRegArb_4KMode + SAREK_2110_TX_ARBITRATOR;

    uint32_t val;
    mDevice.ReadRegister(reg,&val);

    enable = val & 0x01;
    return true;
}

bool CNTV2Config2110::SetIGMPDisable(eSFP sfp, bool disable)
{
    uint32_t val = (disable) ? 1 : 0;
    if (sfp == SFP_1 )
    {
        mDevice.WriteRegister(SAREK_REGS + kSarekRegIGMPDisable,val);
    }
    else
    {
        mDevice.WriteRegister(SAREK_REGS + kSarekRegIGMPDisable2,val);
    }
    return true;
}

bool CNTV2Config2110::GetIGMPDisable(eSFP sfp, bool & disabled)
{
    uint32_t val;
    if (sfp == SFP_1 )
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

uint32_t CNTV2Config2110::GetDecapsulatorAddress(eSFP sfp, NTV2Channel channel, NTV2Stream stream)
{
    uint32_t offset = 0;

    switch (channel)
    {
		case NTV2_CHANNEL1:
			if (stream == NTV2_VIDEO_STREAM)
                offset = 0x00;
			else
                offset = 0x10;
            break;
		case NTV2_CHANNEL2:
			if (stream == NTV2_VIDEO_STREAM)
                offset = 0x20;
			else
                offset = 0x30;
            break;
		case NTV2_CHANNEL3:
			if (stream == NTV2_VIDEO_STREAM)
                offset = 0x40;
			else
                offset = 0x50;
            break;
        case NTV2_CHANNEL4:
			if (stream == NTV2_VIDEO_STREAM)
                offset = 0x60;
            else
                offset = 0x70;
            break;
        case NTV2_CHANNEL5:
		case NTV2_CHANNEL6:
		case NTV2_CHANNEL7:
		case NTV2_CHANNEL8:
		case NTV2_CHANNEL_INVALID:
			break;
    }

    if (sfp == SFP_1)
        return SAREK_2110_DECAPSULATOR_0 + offset;
    else
        return SAREK_2110_DECAPSULATOR_1 + offset;
}

uint32_t CNTV2Config2110::GetDepacketizerAddress(NTV2Channel channel, NTV2Stream stream)
{
    static uint32_t v_depacketizers[4] = {SAREK_4175_RX_DEPACKETIZER_1,SAREK_4175_RX_DEPACKETIZER_2,SAREK_4175_RX_DEPACKETIZER_3,SAREK_4175_RX_DEPACKETIZER_4};
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

uint32_t CNTV2Config2110::GetFramerAddress(eSFP sfp, NTV2Channel channel, NTV2Stream stream)
{
	(void) channel;
    if (sfp == SFP_2)
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

bool  CNTV2Config2110::ConfigurePTP (eSFP sfp, string localIPAddress)
{
    uint32_t macLo;
    uint32_t macHi;

    // get primaray mac address
    uint32_t macAddressRegister = SAREK_REGS + kRegSarekMAC;
    if (sfp != SFP_1)
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

bool CNTV2Config2110::GetSFPMSAData(eSFP sfp, SFPMSAData & data)
{
    return GetSFPInfo(sfp, data);
}

bool CNTV2Config2110::GetLinkStatus(eSFP sfp, SFPStatus & sfpStatus)
{
    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkStatus,&val);
    uint32_t val2;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekSFPStatus,&val2);

    if (sfp == SFP_2)
    {
        sfpStatus.SFP_linkUp        = (val  & LINK_B_UP) ? true : false;
        sfpStatus.SFP_present       = (val2 & SFP_2_NOT_PRESENT) ? false : true;
        sfpStatus.SFP_rxLoss        = (val2 & SFP_2_RX_LOS) ? true : false;
        sfpStatus.SFP_txFault       = (val2 & SFP_2_TX_FAULT) ? true : false;
    }
    else
    {
        sfpStatus.SFP_linkUp        = (val  & LINK_A_UP) ? true : false;
        sfpStatus.SFP_present       = (val2 & SFP_1_NOT_PRESENT) ? false : true;
        sfpStatus.SFP_rxLoss        = (val2 & SFP_1_RX_LOS) ? true : false;
        sfpStatus.SFP_txFault       = (val2 & SFP_1_TX_FAULT) ? true : false;
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
        GetNetworkConfiguration(port, nc);
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
            SetTxStreamEnable(channel, stream, false); // stop transmit
            mIpErrorCode = NTV2IpErrCannotGetMacAddress;
            return false;
        }

        istringstream ss(macAddr);
        string token;
        int i=0;
        while (i < 6)
        {
            getline (ss, token, ':');
            macaddr.mac[i++] = (uint8_t)strtoul(token.c_str(), NULL, 16);
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

string CNTV2Config2110::GetTxSDPUrl(eSFP sfp, NTV2Channel chan, NTV2Stream stream)
{
    return "http://172.16.0.109/txch4v.sdp";
}


string CNTV2Config2110::GetTxSDP(NTV2Channel chan, NTV2Stream stream)
{
    int ch = (int)chan;
    int st = (int)stream;
    if (txsdp[ch][st].str().empty())
    {
        GenSDP(chan, stream);
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
	sdp <<  To_String((int)t);

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
    // TODO - fix this to work with sfp2
    bool enabledA;
    bool enabledB;
    GetTxStreamEnable(channel,NTV2_VIDEO_STREAM,enabledA,enabledB);
    if (!enabledA)
    {
        return true;
    }

    tx_2110Config config;
    GetTxStreamConfiguration(channel, NTV2_VIDEO_STREAM, config);

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
    sdp << config.remoteIP[0];
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
        sdp << "interlace=1; ";
    }
    else if (NTV2_IS_PSF_VIDEO_FORMAT(vfmt))
    {
        sdp << "interlace segmented";
    }
    sdp << endl;

    // PTP
    sdp << "a=ts-refclk:ptp=IEEE1588-2008:" << gmInfo << endl;
    sdp << "a=mediaclk:direct=0" << endl;
    sdp << "a=mid:VID" << endl;

    return true;
}


bool CNTV2Config2110::GenSDPAudioStream(stringstream & sdp, NTV2Channel channel, NTV2Stream stream, string gmInfo)
{
    // TODO - fix this to work with sfp2
    bool enabledA;
    bool enabledB;
    GetTxStreamEnable(channel,stream,enabledA,enabledB);
    if (!enabledA)
    {
        return true;
    }

    tx_2110Config config;
    GetTxStreamConfiguration(channel, stream, config);

    // media name
    sdp << "m=audio ";
    sdp << To_String(config.remotePort[0]);     // FIXME
    sdp << " RTP/AVP ";
    sdp << To_String(config.payloadType) << endl;

    // connection information
    sdp << "c=IN IP4 ";
    sdp << config.remoteIP[0];
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
            if (sampling ==  "YCbCr-4:2:2")
            {
                rxConfig.videoSamples = VPIDSampling_YUV_422;
            }
            string width    = getVideoDescriptionValue("width=");
            string height   = getVideoDescriptionValue("height=");
            string rate     = getVideoDescriptionValue("exactframerate=");
            bool interlace = false;
            vector<string>::iterator it;
            for (it = tokens.begin(); it != tokens.end(); it++)
            {
				// For interlace, we can get one of the following tokens:
				// interlace
				// interlace;
				// interlace=1
				// Note: interlace=0 means 
				if (it->substr( 0, 9 ) != "interlace") 
					continue;

				if (*it == "interlace") {
					interlace=true;
					break;
				}

				if (it->substr(0,10) == "interlace;") {
					interlace=true;
					break;
				}
				if (it->substr(0,11) == "interlace=1") {
					interlace=true;
					break;
				}
            }
            int w = atoi(width.c_str());
            int h = atoi(height.c_str());
            NTV2FrameRate r = stringToRate(rate);
            NTV2VideoFormat vf = ::GetFirstMatchingVideoFormat(r,h,w,interlace,false /* no level B */);
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

void CNTV2Config2110::SetArbiter(const eSFP sfp, const NTV2Channel channel, const NTV2Stream stream, bool enable)
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

    uint32_t bit = (1 << get2110TxStream(channel,stream)) << (int(sfp) * 16);
    if (enable)
        val |= bit;
    else
        val &= ~bit;

    mDevice.WriteRegister(reg,val);
}

void CNTV2Config2110::GetArbiter(const eSFP sfp, NTV2Channel channel, NTV2Stream stream, bool & enable)
{
    uint32_t reg;
    if (stream == NTV2_VIDEO_STREAM)
        reg = kRegArb_video + SAREK_2110_TX_ARBITRATOR;
    else
        reg = kRegArb_audio + SAREK_2110_TX_ARBITRATOR;

    uint32_t val;
    mDevice.ReadRegister(reg,&val);

    uint32_t bit = (1 << get2110TxStream(channel,stream)) << (int(sfp) * 16);
	enable = (val & bit) > 0 ? true : false;
}
