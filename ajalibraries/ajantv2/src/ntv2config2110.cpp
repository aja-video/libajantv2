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
#include <math.h>
#endif

using namespace std;

static const int rxtx2110Streams[SAREK_MAX_CHANS][NTV2_MAX_NUM_STREAMS] = {
    {   NTV2_VIDEO_STREAM,        NTV2_AUDIO1_STREAM,         NTV2_AUDIO2_STREAM,        NTV2_METADATA_STREAM},
    {   NTV2_VIDEO_STREAM + 0x10, NTV2_AUDIO1_STREAM + 0x10 , NTV2_AUDIO2_STREAM + 0x10, NTV2_METADATA_STREAM + 0x10},
    {   NTV2_VIDEO_STREAM + 0x20, NTV2_AUDIO1_STREAM + 0x20 , NTV2_AUDIO2_STREAM + 0x20, NTV2_METADATA_STREAM + 0x20},
    {   NTV2_VIDEO_STREAM + 0x30, NTV2_AUDIO1_STREAM + 0x30 , NTV2_AUDIO2_STREAM + 0x30, NTV2_METADATA_STREAM + 0x30}};

void tx_2110Config::init()
{
    localPort    = 0;
    remotePort   = 0;
    remoteIP.erase();
    autoMAC      = false;
    memset(remoteMAC.mac, 0, sizeof(MACAddr));
    videoFormat  = NTV2_FORMAT_UNKNOWN;
    videoSamples = VPIDSampling_YUV_422;
}

bool tx_2110Config::eq_MACAddr(const MACAddr& a)
{
    return (memcmp(remoteMAC.mac, a.mac, 6) == 0);
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
        (autoMAC         == other.autoMAC)        &&
        (eq_MACAddr(other.remoteMAC))             &&
        (videoFormat     == other.videoFormat)    &&
        (videoSamples    == other.videoSamples))

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
    rxMatch     = 0;
    sourceIP    = "";
    destIP      = "";
    sourcePort  = 0;
    destPort    = 0;
    SSRC        = 0;
    VLAN        = 0;
    videoFormat   = NTV2_FORMAT_UNKNOWN;
    videoSamples  = VPIDSampling_YUV_422;

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
            (videoFormat       == other.videoFormat)    &&
            (videoSamples      == other.videoSamples))
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

    ConfigurePTP(port,localIPAddress);

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

bool CNTV2Config2110::DisableRxChannel(const NTV2Channel channel)
{
    // disable IGMP subscription
    eSFP port = GetRxPort(channel);
    bool disableIGMP;
    GetIGMPDisable(port, disableIGMP);
    if (!disableIGMP)
    {
        EnableIGMPGroup(port,channel,NTV2_VIDEO_STREAM,false);
        EnableIGMPGroup(port,channel,NTV2_AUDIO1_STREAM,false);
    }

    // disable decapsulator channel
    DisableDecapsulatorStream(channel,NTV2_VIDEO_STREAM);
    DisableDecapsulatorStream(channel,NTV2_VIDEO_STREAM);

    return true;
}

bool CNTV2Config2110::EnableRxChannel(const NTV2Channel channel,const rx_2110Config & videoConfig,const rx_2110Config & audioConfig)
{
    // disable decasulator
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel);
    mDevice.WriteRegister(kRegDecap_module_ctrl + decapBaseAddr, 0x00);

    SetupDecapsulator(channel,NTV2_VIDEO_STREAM,videoConfig);
    SetupDecapsulator(channel,NTV2_AUDIO1_STREAM,audioConfig);

    // enable Decapsulator
    mDevice.WriteRegister(kRegDecap_module_ctrl + decapBaseAddr, 0x01);

    // wait for lock
    bool rv = WaitDecapsulatorLock(channel,NTV2_VIDEO_STREAM);
    if (!rv)
    {
        mError = "Video Decaspsulator failed to lock to source";
        return false;
    }
    // setup depacketizer
    SetupDepacketizer(channel, NTV2_VIDEO_STREAM, videoConfig);
    EnableDecapsulatorStream(channel,NTV2_VIDEO_STREAM);

    // wait for lock
    rv = WaitDecapsulatorLock(channel,NTV2_AUDIO1_STREAM);
    if (!rv)
    {
        mError = "Audio Decaspsulator failed to lock to source";
        return false;
    }
    // setup depacketizer
    SetupDepacketizer(channel, NTV2_AUDIO1_STREAM, audioConfig);
    EnableDecapsulatorStream(channel,NTV2_AUDIO1_STREAM);
	return true;
}

void CNTV2Config2110::SetupDecapsulator(const NTV2Channel channel, NTV2Stream stream, const rx_2110Config & rxConfig)
{
    uint32_t  decapBaseAddr        = GetDecapsulatorAddress(channel);
    eSFP      port                 = GetRxPort(channel);


    // make IGMP subsciption if needed
    uint32_t destIp = inet_addr(rxConfig.destIP.c_str());
    destIp = NTV2EndianSwap32(destIp);

    uint8_t ip0 = (destIp & 0xff000000)>> 24;

    if (ip0 >= 224 && ip0 <= 239)
    {
        // is multicast
        SetIGMPGroup(port, channel, stream, destIp, true);
    }
    else
    {
        UnsetIGMPGroup(port, channel, stream);
    }

    // disable decapsulator channel
    DisableDecapsulatorStream(channel,stream);

    // wait for FIFO to flush
    mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1,30);
    // reset the depacketizer
    ResetDepacketizer(channel,stream);

    // Decapulator select channel
    SelectRxDecapsulatorChannel(channel, stream, decapBaseAddr);

    // clear interruppts
    WriteChannelRegister(kRegDecap_int_clear + decapBaseAddr,0x7);

    // hold off access while we update channel regs
    AcquireDecapsulatorControlAccess(decapBaseAddr);

    // source ip address
    uint32_t sourceIp = inet_addr(rxConfig.sourceIP.c_str());
    sourceIp = NTV2EndianSwap32(sourceIp);
    WriteChannelRegister(kRegDecap_match_src_ip0 + decapBaseAddr, sourceIp);

    // dest ip address

    WriteChannelRegister(kRegDecap_match_dst_ip0 + decapBaseAddr, destIp);

    // source port
    WriteChannelRegister(kRegDecap_match_udp_src_port + decapBaseAddr, rxConfig.sourcePort);

    // dest port
    WriteChannelRegister(kRegDecap_match_udp_dst_port + decapBaseAddr, rxConfig.destPort);

    // ssrc
    WriteChannelRegister(kRegDecap_match_ssrc + decapBaseAddr, rxConfig.SSRC);

    // vlan
    WriteChannelRegister(kRegDecap_match_vlan + decapBaseAddr, rxConfig.VLAN);

    // payload type
    if (stream == NTV2_VIDEO_STREAM)
    {
        WriteChannelRegister(kRegDecap_match_payload_ip_type + decapBaseAddr,0x10000000 | rxConfig.payloadType);
        WriteChannelRegister(kRegDecap_chan_timeout + decapBaseAddr,156250000);
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        WriteChannelRegister(kRegDecap_match_payload_ip_type + decapBaseAddr,0x20000000 | rxConfig.payloadType);
        WriteChannelRegister(kRegDecap_chan_timeout + decapBaseAddr,156250000 * 2);
    }

    // matching
    WriteChannelRegister(kRegDecap_match_sel + decapBaseAddr, rxConfig.rxMatch);



    // enable  register updates
    ReleaseDecapsulatorControlAccess(decapBaseAddr);
}

void  CNTV2Config2110::DisableDecapsulatorStream(NTV2Channel channel, NTV2Stream stream)
{
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel);
    uint32_t val = 0x00;
    if (stream == NTV2_VIDEO_STREAM)
    {
        val = 0x04;
    }

    // disable decapsulator channel
    SelectRxDecapsulatorChannel(channel, stream, decapBaseAddr);
    WriteChannelRegister(kRegDecap_chan_ctrl + decapBaseAddr, val);
}

void  CNTV2Config2110::EnableDecapsulatorStream(NTV2Channel channel, NTV2Stream stream)
{
    // enable the decapsulator
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel);
    AcquireDecapsulatorControlAccess(decapBaseAddr);
    SelectRxDecapsulatorChannel(channel, stream, decapBaseAddr);
    uint32_t val = 0x01;
    if (stream == NTV2_VIDEO_STREAM)
    {
        val = 0x05;
    }
    WriteChannelRegister(kRegDecap_chan_ctrl + decapBaseAddr, val);
    ReleaseDecapsulatorControlAccess(decapBaseAddr);
}

bool  CNTV2Config2110::WaitDecapsulatorLock(const NTV2Channel channel, NTV2Stream stream)
{
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel);

    SelectRxDecapsulatorChannel(channel, stream, decapBaseAddr);

    uint32_t lock;
    int timeout = 150;
    do
    {
        if (--timeout <=0 )
        {
            WriteChannelRegister(kRegDecap_int_clear + decapBaseAddr,0x7);
            return false;
        }

        mDevice.WaitForOutputVerticalInterrupt();
        ReadChannelRegister(kRegDecap_int_status + decapBaseAddr,&lock);

    } while ((lock & BIT(0)) == 0);

    WriteChannelRegister(kRegDecap_int_clear + decapBaseAddr,0x7);

    return true;
}

bool  CNTV2Config2110::WaitDecapsulatorUnlock(NTV2Stream & stream, bool & unlock, bool & timeout)
{
    unlock  = false;
    timeout = false;
    uint32_t  decapBaseAddr   = GetDecapsulatorAddress(NTV2_CHANNEL1);

    int ticks = 150;
    do
    {
        mDevice.WaitForOutputVerticalInterrupt();

        uint32_t interrupt = 0;
        mDevice.ReadRegister(kRegDecap_chan_int_grp_ored + decapBaseAddr,&interrupt);
        if (interrupt)
        {
            mDevice.ReadRegister(kRegDecap_chan_int_grp_0 + decapBaseAddr,&interrupt);
            if (interrupt & BIT(0))
                stream = NTV2_VIDEO_STREAM;
            else
                stream = NTV2_AUDIO1_STREAM;
            SelectRxDecapsulatorChannel(NTV2_CHANNEL1, stream, decapBaseAddr);
            uint32_t lock = 0;
            ReadChannelRegister(kRegDecap_int_status + decapBaseAddr,&lock);
            if ( lock & BIT(1) )
                unlock = true;
            if ( lock &   BIT(2) )
                timeout = true;
            WriteChannelRegister(kRegDecap_int_clear + decapBaseAddr,0x7);
            return true;
        }

    } while (--ticks <= 0);

    return false;
}

void  CNTV2Config2110::ResetDepacketizer(const NTV2Channel channel, NTV2Stream stream)
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
    mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1,30);
    mDevice.WriteRegister(kRegSarekRxReset + SAREK_REGS, 0x0);
    mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1,30);
}

void  CNTV2Config2110::SetupDepacketizer(const NTV2Channel channel, NTV2Stream stream, const rx_2110Config & rxConfig)
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
        mDevice.WriteRegister(kRegPll_DecVidStd + SAREK_PLL, val);
    }

    uint32_t  depacketizerBaseAddr = GetDepacketizerAddress(channel,stream);

    mDevice.WriteRegister(kReg4175_depkt_control + depacketizerBaseAddr, 0x00);

    if (stream == NTV2_VIDEO_STREAM)
    {
        // setup 4175 depacketizer

        NTV2VideoFormat fmt = rxConfig.videoFormat;
        NTV2FormatDescriptor fd(fmt,NTV2_FBF_10BIT_YCBCR);

        // width
        uint32_t width = fd.GetRasterWidth();
        mDevice.WriteRegister(kReg4175_depkt_width + depacketizerBaseAddr,width);

        // height
        uint32_t height = fd.GetRasterHeight();
        mDevice.WriteRegister(kReg4175_depkt_height + depacketizerBaseAddr,height);

        // video format = sampling
        int vf;
        int componentsPerPixel;
        int componentsPerUnit;

        VPIDSampling vs = rxConfig.videoSamples;
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
            vf = 2;
            componentsPerPixel = 2;
            componentsPerUnit  = 4;
            break;
        }
        mDevice.WriteRegister(kReg4175_depkt_vid_fmt + depacketizerBaseAddr,vf);

        const int bitsPerComponent = 10;
        const int pixelsPerClock = 1;
        int activeLine_root    = width * componentsPerPixel * bitsPerComponent;
        int activeLineLength   = activeLine_root/8;
        int pixelGroup_root    = bitsPerComponent * componentsPerUnit;
//      int pixelGroupSize     = pixelGroup_root/8;
        int bytesPerCycle_root = pixelsPerClock * bitsPerComponent * componentsPerPixel;
//      int bytesPerCycle      = bytesPerCycle_root/8;
        int lcm                = LeastCommonMultiple(pixelGroup_root,bytesPerCycle_root)/8;
        int payloadLength_root =  min(activeLineLength,1376)/lcm;
        int payloadLength      = payloadLength_root * lcm;
        float pktsPerLine      = ((float)activeLineLength)/((float)payloadLength);
        int ipktsPerLine       = (int)ceil(pktsPerLine);
        int payloadLengthLast  = activeLineLength - (payloadLength * (ipktsPerLine -1));

        // pkts per line
        mDevice.WriteRegister(kReg4175_depkt_pkts_per_line + depacketizerBaseAddr,ipktsPerLine);

        // payload length
        mDevice.WriteRegister(kReg4175_depkt_payload_len + depacketizerBaseAddr,payloadLength);

        // payload length last
        mDevice.WriteRegister(kReg4175_depkt_payload_len_last + depacketizerBaseAddr,payloadLengthLast);

        // end setup 4175 depacketizer
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        // setup 3190 depacketizer

        // num samples
        mDevice.WriteRegister(kReg3190_depkt_num_samples + depacketizerBaseAddr,48);

        // audio channels
        mDevice.WriteRegister(kReg3190_depkt_num_audio_chans + depacketizerBaseAddr,2);
    }

    // enable depacketizer
    mDevice.WriteRegister(kReg4175_depkt_control + depacketizerBaseAddr, 0x80);
    mDevice.WriteRegister(kReg4175_depkt_control + depacketizerBaseAddr, 0x81);
}

bool  CNTV2Config2110::GetRxChannelConfiguration(const NTV2Channel channel, NTV2Stream stream, rx_2110Config & rxConfig)
{
    uint32_t    val;

    // get address
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel);

    // select channel
    SelectRxDecapsulatorChannel(channel, stream, decapBaseAddr);

    // source ip address
    ReadChannelRegister(kRegDecap_match_src_ip0 + decapBaseAddr, &val);
    struct in_addr in;
    in.s_addr = NTV2EndianSwap32(val);
    char * ip = inet_ntoa(in);
    rxConfig.sourceIP = ip;

    // dest ip address
    ReadChannelRegister(kRegDecap_match_dst_ip0 + decapBaseAddr, &val);
    in.s_addr = NTV2EndianSwap32(val);
    ip = inet_ntoa(in);
    rxConfig.destIP = ip;

    // source port
    ReadChannelRegister(kRegDecap_match_udp_src_port + decapBaseAddr, &rxConfig.sourcePort);

    // dest port
    ReadChannelRegister(kRegDecap_match_udp_dst_port + decapBaseAddr, &rxConfig.destPort);

    // ssrc
    ReadChannelRegister(kRegDecap_match_ssrc + decapBaseAddr, &rxConfig.SSRC);

    // vlan
    ReadChannelRegister(kRegDecap_match_vlan + decapBaseAddr, &val);
    rxConfig.VLAN = val & 0xffff;

    // payload type
    ReadChannelRegister(kRegDecap_match_payload_ip_type + decapBaseAddr,&val);
    rxConfig.payloadType = val & 0x7f;

    // matching
    ReadChannelRegister(kRegDecap_match_sel + decapBaseAddr, &rxConfig.rxMatch);

    if (stream == NTV2_VIDEO_STREAM)
    {
        // depacketizer
        uint32_t depackBaseAddr = GetDepacketizerAddress(channel, stream);

        // sampling
        mDevice.ReadRegister(kReg4175_depkt_vid_fmt + depackBaseAddr,&val);
        val = val & 0x3;
        VPIDSampling vs;
        switch(val)
        {
        case 0:
            vs = VPIDSampling_GBR_444;
            break;
        case 1:
            vs = VPIDSampling_YUV_444;
            break;
        case 2:
        default:
            vs = VPIDSampling_YUV_422;
            break;
        }
        rxConfig.videoSamples = vs;

        // format
#if 0
        mDevice.ReadRegister(SAREK_PLL + kRegPll_DecVidStd, &val);
       NTV2FrameRate       fr  = NTV2FrameRate((val & 0xf00) >> 8);
       NTV2FrameGeometry   fg  = NTV2FrameGeometry((val & 0xf0) >> 4);
       NTV2Standard        std = NTV2Standard(val & 0x0f);
       bool               is2K = (val & BIT(13));

       NTV2FormatDescriptor fd;
#endif
    }

    return true;
}

bool CNTV2Config2110::GetRxChannelEnable(const NTV2Channel channel, NTV2Stream stream, bool & enabled)
{
    // get address
    uint32_t  decapBaseAddr = GetDecapsulatorAddress(channel);

    // select channel
    SelectRxDecapsulatorChannel(channel, stream, decapBaseAddr);

    uint32_t val;
    ReadChannelRegister(kRegDecap_chan_ctrl + decapBaseAddr,&val);
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
    uint32_t    hi;
    uint32_t    lo;
    MACAddr     macaddr;
    uint8_t     ip0;
    uint32_t    destIp;
    uint32_t    mac;
    bool        rv	(true);

    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(channel,stream);

    // select channel
    SelectTxFramerChannel(channel, stream, baseAddrFramer);

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

    WriteChannelRegister(kRegFramer_dest_mac_lo  + baseAddrFramer,lo);
    WriteChannelRegister(kRegFramer_dest_mac_hi  + baseAddrFramer,hi);

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // end framer setup

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

        // pkts per line
        mDevice.WriteRegister(kReg4175_pkt_pkts_per_line + baseAddrPacketizer,ipktsPerLine);

        // payload length
        mDevice.WriteRegister(kReg4175_pkt_payload_len + baseAddrPacketizer,payloadLength);

        // payload length last
        mDevice.WriteRegister(kReg4175_pkt_payload_len_last + baseAddrPacketizer,payloadLengthLast);

        // payload type
        mDevice.WriteRegister(kReg4175_pkt_payload_type + baseAddrPacketizer,100);

        // pix per pkt
        int ppp = (payloadLength/pixelGroupSize) * 2;   // as per JeffL
        mDevice.WriteRegister(kReg4175_pkt_pix_per_pkt + baseAddrPacketizer,ppp);

        // interlace
        int ilace = (interlaced) ? 0x01 : 0x00;
        mDevice.WriteRegister(kReg4175_pkt_interlace_ctrl + baseAddrPacketizer,ilace);

        // end setup 4175 packetizer


    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        // setup 4175 packetizer

        // num samples
        mDevice.WriteRegister(kReg3190_pkt_num_samples + baseAddrPacketizer,48);

        // audio channels
        mDevice.WriteRegister(kReg3190_pkt_num_audio_channels + baseAddrPacketizer,2);

        // payload length
        mDevice.WriteRegister(kReg3190_pkt_payload_len + baseAddrPacketizer,0x120);

        // payload type
        mDevice.WriteRegister(kReg3190_pkt_payload_type + baseAddrPacketizer,101);

        // ssrc
        mDevice.WriteRegister(kReg3190_pkt_ssrc + baseAddrPacketizer,0);

    }
    return rv;
}

bool CNTV2Config2110::GetTxChannelConfiguration(const NTV2Channel channel, NTV2Stream stream, tx_2110Config & txConfig)
{
    uint32_t    val;

    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(channel,stream);

    // Select channel
    SelectTxFramerChannel(channel, stream, baseAddrFramer);

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
    ReadChannelRegister(kRegFramer_dest_mac_lo + baseAddrFramer, &lo);
    ReadChannelRegister(kRegFramer_dest_mac_hi  + baseAddrFramer, &hi);

    txConfig.remoteMAC.mac[0] = (hi >> 8) & 0xff;
    txConfig.remoteMAC.mac[1] =  hi        & 0xff;
    txConfig.remoteMAC.mac[2] = (lo >> 24) & 0xff;
    txConfig.remoteMAC.mac[3] = (lo >> 16) & 0xff;
    txConfig.remoteMAC.mac[4] = (lo >> 8)  & 0xff;
    txConfig.remoteMAC.mac[5] =  lo        & 0xff;

    mDevice.ReadRegister(kRegSarekTxAutoMAC + SAREK_REGS,&val);
    txConfig.autoMAC = ((val & (1 << channel)) != 0);

    if (stream == NTV2_VIDEO_STREAM)
    {
        // DAC TODO - video format and sampling
    }

    return true;
}

bool CNTV2Config2110::SetTxChannelEnable(const NTV2Channel channel, NTV2Stream stream, bool enable)
{
    // ** Framer
    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(channel,stream);

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
        WriteChannelRegister(kRegFramer_chan_ctrl   + baseAddrFramer,0x01);  // enables tx over mac1/mac2
    }
    else
    {
        // disable
        WriteChannelRegister(kRegFramer_chan_ctrl    + baseAddrFramer,0x0);   // disables channel
    }

    // enable  register updates
    ReleaseFramerControlAccess(baseAddrFramer);

    // ** Framer end

    // ** Packetizer
    uint32_t packetizerBaseAddr;
    SetTxPacketizerChannel(channel,stream,packetizerBaseAddr);

    // this works for 4174 and 3190, the pkt_ctrl reg is 0x0
    if (enable)
    {
        mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x00);
        mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x80);
        mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x81);
    }
    else
    {
        mDevice.WriteRegister(kReg4175_pkt_ctrl + packetizerBaseAddr, 0x00);
    }
    return true;
}

bool CNTV2Config2110::GetTxChannelEnable(const NTV2Channel channel, NTV2Stream stream, bool & enabled)
{
    // get frame address
    uint32_t baseAddrFramer = GetFramerAddress(channel,stream);

    // select channel
    SelectTxFramerChannel(channel, stream, baseAddrFramer);

    uint32_t val;
    ReadChannelRegister(kRegFramer_chan_ctrl + baseAddrFramer, &val);
    enabled = (val & 0x01);

    return true;
}

bool  CNTV2Config2110::SetPTPMaster(std::string ptpMaster)
{
    uint32_t addr = inet_addr(ptpMaster.c_str());
    addr = NTV2EndianSwap32(addr);
    return mDevice.WriteRegister(kRegPll_PTP_MstrIP + SAREK_PLL, addr);
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

uint32_t CNTV2Config2110::GetDecapsulatorAddress(NTV2Channel channel)
{
    uint32_t iChannel = (uint32_t) channel;

    if (iChannel > _numRxChans)
    {
        return SAREK_2110_DECAPSULATOR_1_TOP;   // default
    }

    if (iChannel >= _numRx0Chans)
    {
       return SAREK_2110_DECAPSULATOR_2_BOT;
    }
    else
    {
        return SAREK_2110_DECAPSULATOR_1_TOP;
    }
}

void CNTV2Config2110::ResetDecapsulator(NTV2Channel channel)
{
    uint32_t decapAddress = GetDecapsulatorAddress(channel);
    mDevice.WriteRegister(kRegDecap_control + decapAddress,0x01);   // resets all channels and streams
    mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1,3);
}

void  CNTV2Config2110::SelectRxDecapsulatorChannel(NTV2Channel channel, NTV2Stream stream, uint32_t  baseAddr)
{
    // select channel
    uint32_t iStream = get2110Stream(channel,stream);
    SetChannel(kRegDecap_channel_access + baseAddr, iStream);
}

uint32_t CNTV2Config2110::GetDepacketizerAddress(NTV2Channel channel, NTV2Stream stream)
{
    static uint32_t v_depacketizers[4] = {SAREK_4175_RX_DEPACKETIZER_1,SAREK_4175_RX_DEPACKETIZER_2,SAREK_4175_RX_DEPACKETIZER_2,SAREK_4175_RX_DEPACKETIZER_4};
    static uint32_t a_depacketizers[4] = {SAREK_3190_RX_DEPACKETIZER_1,SAREK_3190_RX_DEPACKETIZER_2,SAREK_3190_RX_DEPACKETIZER_3,SAREK_3190_RX_DEPACKETIZER_4};

    uint32_t iChannel = (uint32_t) channel;

    if (iChannel > _numTxChans)
        return SAREK_4175_RX_DEPACKETIZER_1;

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

uint32_t CNTV2Config2110::GetFramerAddress(NTV2Channel channel, NTV2Stream stream)
{
	(void) stream;
    uint32_t iChannel = (uint32_t) channel;

    if (iChannel > _numTxChans)
    {
        return SAREK_2110_FRAMER_1_TOP;  // default
    }

    if (iChannel >= _numTx0Chans)
    {
        return SAREK_2110_FRAMER_2_BOT;
    }
    else
    {
        return SAREK_2110_FRAMER_1_TOP;
    }
}

void CNTV2Config2110::SelectTxFramerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t baseAddrFramer)
{
    // select channel
    uint32_t iStream = get2110Stream(channel,stream);
    SetChannel(kRegFramer_channel_access + baseAddrFramer, iStream);
}

bool CNTV2Config2110::SetTxPacketizerChannel(NTV2Channel channel, NTV2Stream stream, uint32_t & baseAddrPacketizer)
{
    static uint32_t v_packetizers[4] = {SAREK_4175_TX_PACKETIZER_1,SAREK_4175_TX_PACKETIZER_2,SAREK_4175_TX_PACKETIZER_3,SAREK_4175_TX_PACKETIZER_4};
    static uint32_t a_packetizers[4] = {SAREK_3190_TX_PACKETIZER_1,SAREK_3190_TX_PACKETIZER_2,SAREK_3190_TX_PACKETIZER_3,SAREK_3190_TX_PACKETIZER_4};

    uint32_t iChannel = (uint32_t) channel;

    if (iChannel > _numTxChans)
        return false;

    uint32_t iStream = get2110Stream(channel,stream);

    if (stream == NTV2_VIDEO_STREAM)
    {
        baseAddrPacketizer  = v_packetizers[iChannel];
        mDevice.WriteRegister(kReg4175_pkt_chan_num + baseAddrPacketizer, iStream);
    }
    else if (stream == NTV2_AUDIO1_STREAM)
    {
        baseAddrPacketizer  = a_packetizers[iChannel];
        mDevice.WriteRegister(kReg3190_pkt_chan_num + baseAddrPacketizer, iStream);
    }
    else
        return false;

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
    WriteChannelRegister(kRegPll_PTP_Match      + SAREK_PLL, 0x9);

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
    WriteChannelRegister(kRegFramer_control + baseAddr, 0x00);
    uint32_t val;
    mDevice.ReadRegister(kRegFramer_status + baseAddr,&val);
    while (val & BIT(1))
    {
        mDevice.WaitForOutputVerticalInterrupt();
        mDevice.ReadRegister(kRegFramer_status + baseAddr,&val);
    }
}

void CNTV2Config2110::ReleaseFramerControlAccess(uint32_t baseAddr)
{
    WriteChannelRegister(kRegFramer_control + baseAddr, 0x02);
}

void CNTV2Config2110::AcquireDecapsulatorControlAccess(uint32_t baseAddr)
{
    WriteChannelRegister(kRegDecap_control + baseAddr, 0x00);
    uint32_t val;
    mDevice.ReadRegister(kRegDecap_status + baseAddr,&val);
    while (val & BIT(1))
    {
        mDevice.WaitForOutputVerticalInterrupt();
        mDevice.ReadRegister(kRegDecap_status + baseAddr,&val);
    }
}

void CNTV2Config2110::ReleaseDecapsulatorControlAccess(uint32_t baseAddr)
{
    WriteChannelRegister(kRegDecap_control + baseAddr, 0x02);
}

uint32_t CNTV2Config2110::get2110Stream(NTV2Channel ch,NTV2Stream esc )
{
    return rxtx2110Streams[ch][esc];
}


