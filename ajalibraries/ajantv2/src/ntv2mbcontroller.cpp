/**
    @file		ntv2mbcontroller.cpp
    @brief		Implementation of CNTV2MBController class.
    @copyright	(C) 2015-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2mbcontroller.h"
#include <sstream>

#if defined(AJALinux)
#include <stdlib.h>
#include <
#endif

#if defined(MSWindows)
#include <time.h>
#include <windows.h> //I've ommited this line.
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
#endif

using namespace std;

void IPVNetConfig::init()
{
    ipc_gateway = 0;
    ipc_ip = 0;
    ipc_subnet = 0;
}

bool IPVNetConfig::operator != ( const IPVNetConfig &other )
{
    return (!(*this == other));
}

bool IPVNetConfig::operator == ( const IPVNetConfig &other )
{
    if ((ipc_gateway  == other.ipc_gateway)   &&
            (ipc_ip       == other.ipc_ip)        &&
            (ipc_subnet   == other.ipc_subnet))
    {
        return true;
    }
    else
    {
        return false;
    }
}

CNTV2MBController::CNTV2MBController(CNTV2Card &device) : CNTV2MailBox(device)
{
}

bool CNTV2MBController::SetMBNetworkConfiguration (eSFP port, string ipaddr, string netmask, string gateway)
{
   if (!(getFeatures() & SAREK_MB_PRESENT))
       return true;

    bool rv = AcquireMailbox();
    if (!rv) return false;

    sprintf((char*)txBuf,"cmd=%d,port=%d,ipaddr=%s,subnet=%s,gateway=%s",
            (int)MB_CMD_SET_NET,(int)port,ipaddr.c_str(),netmask.c_str(),gateway.c_str());

    rv = sendMsg(1000);
    if (!rv)
    {
        ReleaseMailbox();
        return false;
    }

    string response;
    getResponse(response);
    vector<string> msg;
    splitResponse(response, msg);
    if (msg.size() >=1)
    {
        string status;
        rv = getString(msg[0],"status",status);
        if (rv && (status == "OK"))
        {
            ReleaseMailbox();
            SetLinkActive(port);
            return true;
        }
        else if (rv && (status == "FAIL"))
        {
            if (msg.size() >= 3)
            {
                rv = getString(msg[2],"error",mError);
                ReleaseMailbox();
                return false;
            }
        }
    }

    ReleaseMailbox();
    mError = "Invalid response from MB";
    return false;
}

bool CNTV2MBController::SetIGMPVersion(uint32_t version)
{
    if (!(getFeatures() & SAREK_MB_PRESENT))
        return true;

    sprintf((char*)txBuf,"cmd=%d,version=%d",(int)MB_CMD_SET_IGMP_VERSION,version);
    bool rv = sendMsg(250);
    if (!rv)
    {
        return false;
    }

    string response;
    getResponse(response);
    vector<string> msg;
    splitResponse(response, msg);
    if (msg.size() >=1)
    {
        string status;
        rv = getString(msg[0],"status",status);
        if (rv && (status == "OK"))
        {
            return true;
        }
        else if (rv && (status == "FAIL"))
        {
            if (msg.size() >= 3)
            {
                rv = getString(msg[2],"error",mError);
                return false;
            }
        }
    }

    mError = "Invalid response from MB";
    return false;
}

bool CNTV2MBController::FetchGrandMasterInfo(uint32_t & domain, string & macAddr)
{
    if (!(getFeatures() & SAREK_MB_PRESENT))
        return true;

    sprintf((char*)txBuf,"cmd=%d",(int)MB_CMD_FETCH_GM_INFO);
    bool rv = sendMsg(250);
    if (!rv)
    {
        return false;
    }

    string response;
    getResponse(response);
    vector<string> msg;
    splitResponse(response, msg);
    if (msg.size() >=4)
    {
        string status;
        rv = getString(msg[0],"status",status);
        if (rv && (status == "OK"))
        {
            rv = getString(msg[2],"MAC",macAddr);
            if (rv == false)
            {
                mError = "MAC Address not found in response from MB";
                return false;
            }
            rv = getDecimal(msg[3],"DOMAIN",domain);
            if (rv == false)
            {
                mError = "DOMAIN not found in response from MB";
                return false;
            }
            return true;
        }
        else if (rv && (status == "FAIL"))
        {
            if (msg.size() >= 3)
            {
                rv = getString(msg[2],"error",mError);
                return false;
            }
        }
    }

    mError = "Invalid response from MB";
    return false;
}

bool CNTV2MBController::GetRemoteMAC(std::string remote_IPAddress, eSFP port, NTV2Channel channel, NTV2Stream stream, string & MACaddress)
{
    if ( (getFeatures() & SAREK_MB_PRESENT) == 0)
    return true;

    bool rv = AcquireMailbox();
    if (!rv) return false;

    int count = 30;
    do
    {
        bool rv = SendArpRequest(remote_IPAddress,port);
        if (!rv) return false;

        mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1,2);
        eArpState as = GetRemoteMACFromArpTable(remote_IPAddress,port,channel,stream,MACaddress);
        switch (as)
        {
        case ARP_VALID:
            ReleaseMailbox();
            return true;
        case ARP_ERROR:
            ReleaseMailbox();
            return false;
        default:
        case ARP_INCOMPLETE:
        case ARP_NOT_FOUND:
            break;
        }

    } while (--count);

    ReleaseMailbox();
    return false;
}

eArpState CNTV2MBController::GetRemoteMACFromArpTable(std::string remote_IPAddress, eSFP port, NTV2Channel channel, NTV2Stream stream, string & MACaddress)
{
    if (!(getFeatures() & SAREK_MB_PRESENT))
        return ARP_VALID;

    sprintf((char*)txBuf,"cmd=%d,ipaddr=%s,port=%d,chan=%d,stream=%d",
            (int)MB_CMD_GET_MAC_FROM_ARP_TABLE,
            remote_IPAddress.c_str(),
            (int)port,
            (int)channel,
            (int)stream);
    bool rv = sendMsg(250);
    if (!rv)
    {
        return ARP_ERROR;
    }

    string response;
    getResponse(response);
    vector<string> msg;
    splitResponse(response, msg);
    if (msg.size() >=1)
    {
        string status;
        rv = getString(msg[0],"status",status);
        if (rv && (status == "OK"))
        {
            if (msg.size() != 3)
            {
                mError = "Invalid response size from MB";
                return ARP_ERROR;
            }

            rv = getString(msg[2],"MAC",MACaddress);
            if (rv == false)
            {
                mError = "MAC Address not found in response from MB";
                return ARP_ERROR;
            }
            return ARP_VALID;
        }
        else if (rv && (status == "FAIL"))
        {
            if (msg.size() >= 4)
            {
                uint32_t state;
                rv = getString(msg[2],"error",mError);
                rv = getDecimal(msg[3],"state",state);
                return (eArpState)state;
            }
        }
    }

    mError = "Invalid response from MB";
    return ARP_ERROR;
}

bool CNTV2MBController::SendArpRequest(std::string remote_IPAddress, eSFP port)
{
    if ( (getFeatures() & SAREK_MB_PRESENT) == 0)
        return true;

    sprintf((char*)txBuf,"cmd=%d,ipaddr=%s,port=%d",
            (int)MB_CMD_SEND_ARP_REQ,
            remote_IPAddress.c_str(),
            int(port));

    bool rv = sendMsg(250);
    if (!rv)
    {
        return ARP_ERROR;
    }

    string response;
    getResponse(response);
    vector<string> msg;
    splitResponse(response, msg);
    if (msg.size() >=1)
    {
        string status;
        rv = getString(msg[0],"status",status);
        if (rv && (status == "OK"))
        {
            if (msg.size() != 2)
            {
                mError = "Invalid response size from MB";
                return false;
            }
            return true;
        }
        else if (rv && (status == "FAIL"))
        {
            if (msg.size() >= 4)
            {
                rv = getString(msg[2],"error",mError);
                return false;
            }
        }
    }

    mError = "Invalid response from MB";
    return false;
}

void CNTV2MBController::splitResponse(std::string response, std::vector<std::string> & results)
{
    std::istringstream ss(response);
    std::string token;

    while(std::getline(ss, token, ','))
    {
        results.push_back(token);
    }
}

bool CNTV2MBController::getDecimal(const std::string & resp, const std::string & parm, uint32_t & result)
{
    string val;
    bool rv = getString(resp,parm,val);
    if (rv)
    {
        result = atoi(val.c_str());
        return true;
    }
    return false;
}

bool CNTV2MBController::getHex(const std::string & resp, const std::string & parm, uint32_t & result)
{
    string val;
    bool rv = getString(resp,parm,val);
    if (rv)
    {
        result = strtoul(val.c_str(),NULL,16);
        return true;
    }
    return false;
}

bool CNTV2MBController::getString(const std::string & resp, const std::string & parm, std::string & result)
{
    string match = parm + "=";

    std::string::size_type i = resp.find(match);

    if (i != std::string::npos && i == 0)
    {
        result = resp;
        result.erase(i, match.length());
        return true;
    }
    return false;   // not found
}

void CNTV2MBController::SetIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream, uint32_t mcast_addr, uint32_t src_addr, bool enable)
{
    uint32_t offset = getIGMPCBOffset(port,channel,stream);
    mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_STATE, IGMPCB_STATE_BUSY);
    mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_MCAST_ADDR, mcast_addr);
    mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_SRC_ADDR,   src_addr);

    uint32_t val = IGMPCB_STATE_USED;
    if (enable)
    {
        val += IGMPCB_STATE_ENABLED;
    }
    mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_STATE, val);
}

void CNTV2MBController::UnsetIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream)
{
    uint32_t offset = getIGMPCBOffset(port,channel,stream);
    mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_STATE, 0);   // block not used
}

void CNTV2MBController::EnableIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream, bool enable)
{
    uint32_t val = 0;
    uint32_t offset = getIGMPCBOffset(port,channel,stream);
    mDevice.ReadRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_STATE,&val);
    if (val != 0)
    {
        // is used or busy, so can enable/disable
        mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_STATE, IGMPCB_STATE_BUSY);
        uint32_t val = IGMPCB_STATE_USED;
        if (enable)
        {
            val += IGMPCB_STATE_ENABLED;
        }
        mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_STATE, val);
    }
}

uint32_t CNTV2MBController::getIGMPCBOffset(eSFP port, NTV2Channel channel, NTV2Stream stream)
{
    struct IGMPCB
    {
        uint32_t state;
        uint32_t multicast_addr;
        uint32_t source_addr;
    };

    //static IGMPCB igmpcb[SAREK_MAX_PORTS][SAREK_MAX_CHANS][NTV2_MAX_NUM_STREAMS];
    if (NTV2_IS_VALID_SFP(port) && NTV2_IS_VALID_CHANNEL(channel) && NTV2_IS_VALID_STREAM(stream))
    {
        uint32_t index = (int)stream + NTV2_MAX_NUM_STREAMS * ((int)channel + SAREK_MAX_CHANS * (int)port );
        uint32_t reg   = (index * sizeof(IGMPCB))/4;
        return reg;
    }
    return 0;
}



bool CNTV2MBController::SetTxLinkState(NTV2Channel channel, bool linkAEnable, bool linkBEnable)
{
    uint32_t chan = (uint32_t)channel;

    uint32_t val = 0;
    if (linkAEnable) val |= 0x2;
    if (linkBEnable) val |= 0x1;
    val <<= (chan * 2);

    uint32_t state;
    bool rv = mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkModes, &state);
    if (!rv) return false;
    state   &= ~( 0x3 << (chan * 2) );
    state  |= val;
    rv = mDevice.WriteRegister(SAREK_REGS + kRegSarekLinkModes, state);
    return rv;
}

bool CNTV2MBController::GetTxLinkState(NTV2Channel channel, bool & linkAEnable, bool & linkBEnable)
{
    uint32_t chan = (uint32_t)channel;

    uint32_t state;
    bool rv = mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkModes, &state);
    if (!rv) return false;
    state  &=  ( 0x3 << (chan * 2) );
    state >>= (chan * 2);
    linkAEnable = (state & 0x02) ? true : false;
    linkBEnable = (state & 0x01) ? true : false;
    return true;
}


bool CNTV2MBController::SetRxLinkState(NTV2Channel channel, bool linkAEnable, bool linkBEnable)
{
    uint32_t chan = (uint32_t)channel;

    uint32_t val = 0;
    if (linkAEnable) val |= 0x2;
    if (linkBEnable) val |= 0x1;
    val <<= (chan * 2);

    uint32_t state;
    bool rv = mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkModes, &state);
    if (!rv) return false;
    state   &= ~( (0x3 << (chan * 2)) << 8 );
    state  |= (val << 8);
    rv = mDevice.WriteRegister(SAREK_REGS + kRegSarekLinkModes, state);
    return rv;
}

bool CNTV2MBController::GetRxLinkState(NTV2Channel channel, bool & linkAEnable, bool & linkBEnable)
{
    uint32_t chan = (uint32_t)channel;

    uint32_t state;
    bool rv = mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkModes, &state);
    if (!rv) return false;
    state >>= 8;
    state  &=  ( 0x3 << (chan * 2) );
    state >>= (chan * 2);
    linkAEnable = (state & 0x02) ? true : false;
    linkBEnable = (state & 0x01) ? true : false;
    return true;
}

bool  CNTV2MBController::SetRxMatch(NTV2Channel channel, eSFP link, uint8_t match)
{
    uint32_t chan = (uint32_t)channel;

    uint32_t val;
    if (link == SFP_TOP)
    {
        mDevice.ReadRegister(SAREK_REGS + kRegSarekRxMatchesA, &val);
    }
    else
    {
       mDevice.ReadRegister(SAREK_REGS + kRegSarekRxMatchesB, &val);
    }

    val  &= ~( 0xff << (chan * 8));
    val  |= ( match << (chan * 8) );

    if (link == SFP_TOP)
    {
        mDevice.WriteRegister(SAREK_REGS + kRegSarekRxMatchesA, val);
    }
    else
    {
       mDevice.WriteRegister(SAREK_REGS + kRegSarekRxMatchesB, val);
    }
    return true;
}

bool  CNTV2MBController::GetRxMatch(NTV2Channel channel, eSFP link, uint8_t & match)
{
    uint32_t chan = (uint32_t)channel;

    uint32_t val;
    if (link == SFP_TOP)
    {
        mDevice.ReadRegister(SAREK_REGS + kRegSarekRxMatchesA, &val);
    }
    else
    {
       mDevice.ReadRegister(SAREK_REGS + kRegSarekRxMatchesB, &val);
    }

    val >>= (chan * 8);
    val &=  0xff;
    match = (uint8_t)val;
    return true;
}


bool CNTV2MBController::SetLinkActive(eSFP link)
{
    uint32_t state;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkModes, &state);
    if (link == SFP_BOTTOM)
    {
        state  |= S2022_LINK_B_ACTIVE;
    }
    else
    {
        state  |= S2022_LINK_A_ACTIVE;
    }
    mDevice.WriteRegister(SAREK_REGS + kRegSarekLinkModes, state);
    return true;
}

bool CNTV2MBController::GetLinkActive(eSFP link)
{
    uint32_t state;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkModes, &state);
    if (link == SFP_BOTTOM)
    {
        if (state & S2022_LINK_B_ACTIVE)
            return true;
    }
    else
    {
        if (state & S2022_LINK_A_ACTIVE)
            return true;
    }
    return false;
}

bool CNTV2MBController::SetDualLinkMode(bool enable)
{
    uint32_t state;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkModes, &state);
    if (enable)
    {
        state  |= S2022_DUAL_LINK;
    }
    else
    {
        state &= ~S2022_DUAL_LINK;
    }
    mDevice.WriteRegister(SAREK_REGS + kRegSarekLinkModes, state);
    return true;
}

bool CNTV2MBController::GetDualLinkMode(bool & enable)
{
    uint32_t state;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekLinkModes, &state);
    enable = ((state & S2022_DUAL_LINK) != 0);
    return true;
}


bool CNTV2MBController::SetTxFormat(NTV2Channel chan, NTV2VideoFormat fmt)
{
    uint32_t shift = 8 * (int)chan;
    uint32_t state;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekTxFmts, &state);
    state  &= ~(0xff << shift);
    state  |= (uint8_t(fmt) << shift );
    mDevice.WriteRegister(SAREK_REGS + kRegSarekTxFmts, state);
    return true;
}

bool CNTV2MBController::GetTxFormat(NTV2Channel chan, NTV2VideoFormat & fmt)
{
    uint32_t shift = 8 * (int)chan;
    uint32_t state;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekTxFmts, &state);
    state  &= (0xff << shift);
    state >>= shift;
    fmt = (NTV2VideoFormat)state;
    return true;
}


uint64_t CNTV2MBController::GetNTPTimestamp()
{
#if defined(MSWindows)

    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    struct timeval tv;

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tv.tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tv.tv_usec = (long) (system_time.wMilliseconds * 1000);
#endif

#if defined(AJALinux) || defined(AJAMac)
    struct timeval tv;
    struct timezone tz;

    gettimeofday( &tv, &tz );
#endif

    uint64_t ntpts;
    ntpts = (((uint64_t)tv.tv_sec + 2208988800u) << 32) + ((uint32_t)tv.tv_usec * 4294.967296);
    return (ntpts);
}
