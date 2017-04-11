/**
	@file		ntv2mbcontroller.cpp
	@brief		Implementation of CNTV2MBController class.
	@copyright	(C) 2015-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2mbcontroller.h"
#include <sstream>

#if defined(AJALinux)
#include <stdlib.h>
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
    uint32_t features = getFeatures();
    if (features & SAREK_MB_PRESENT)
    {
        sprintf((char*)txBuf,"cmd=%d,port=%d,ipaddr=%s,subnet=%s,gateway=%s",
                      (int)MB_CMD_SET_NET,(int)port,ipaddr.c_str(),netmask.c_str(),gateway.c_str());

        bool rv = sendMsg(1000);
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
    else
         return true;
}

bool CNTV2MBController::SetIGMPVersion(uint32_t version)
{
    uint32_t features = getFeatures();
    if (features & SAREK_MB_PRESENT)
    {
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
    else
         return true;
}

bool CNTV2MBController::GetRemoteMAC(std::string remote_IPAddress, string & MACaddress)
{
    uint32_t features = getFeatures();
    if (features & SAREK_MB_PRESENT)
    {
        int count = 30;
        do
        {
            SendArpRequest(remote_IPAddress);
            mDevice.WaitForOutputVerticalInterrupt(NTV2_CHANNEL1,2);
            eArpState as = GetRemoteMACFromArpTable(remote_IPAddress,MACaddress);
            switch (as)
            {
            case ARP_VALID:
                return true;
            case ARP_ERROR:
                return false;
            default:
            case ARP_INCOMPLETE:
            case ARP_NOT_FOUND:
               break;
            }

        } while (--count);

        return false;
    }
    else
        return true;
}

eArpState CNTV2MBController::GetRemoteMACFromArpTable(std::string remote_IPAddress, string & MACaddress)
{
    uint32_t features = getFeatures();
    if (features & SAREK_MB_PRESENT)
    {
        sprintf((char*)txBuf,"cmd=%d,ipaddr=%s",(int)MB_CMD_GET_MAC_FROM_ARP_TABLE,remote_IPAddress.c_str());
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
    else
         return ARP_VALID;
}

bool CNTV2MBController::SendArpRequest(std::string remote_IPAddress)
{
    uint32_t features = getFeatures();
    if (features & SAREK_MB_PRESENT)
    {
        sprintf((char*)txBuf,"cmd=%d,ipaddr=%s",(int)MB_CMD_SEND_ARP_REQ,remote_IPAddress.c_str());
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
    else
        return true;
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

void CNTV2MBController::SetIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream, uint32_t ipaddr, bool enable)
{
    uint32_t offset = getIGMPCBOffset(port,channel,stream);
    mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPCB_REG_ADDR, ipaddr);

    EnableIGMPGroup(port,channel,stream,enable);
}

void CNTV2MBController::UnsetIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream)
{
    uint32_t offset = getIGMPCBOffset(port,channel,stream);
    mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPPCB_REG_STATE, 0);   // block not used
}

void CNTV2MBController::EnableIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream, bool enable)
{
    uint32_t offset = getIGMPCBOffset(port,channel,stream);
    uint32_t val = IGMPCB_STATE_USED;
    if (enable)
    {
        val += IGMPCB_STATE_ENABLED;
    }
    mDevice.WriteRegister(SAREK_REGS2 + IGMP_BLOCK_BASE + offset + IGMPPCB_REG_STATE, val);
}

uint32_t CNTV2MBController::getIGMPCBOffset(eSFP port, NTV2Channel channel, NTV2Stream stream)
{
    struct IGMPCB
    {
        uint32_t state;
        uint32_t addr;
    };
    static IGMPCB igmpcb[SAREK_MAX_PORTS][SAREK_MAX_CHANS][NTV2_MAX_NUM_STREAMS];
    if (NTV2_IS_VALID_SFP(port) && NTV2_IS_VALID_CHANNEL(channel) && NTV2_IS_VALID_STREAM(stream))
    {
        uint32_t offset = uint32_t(&igmpcb[port][channel][stream] - &igmpcb[0][0][0]);
        return offset * 2;    // registers
    }
    return 0;
}
