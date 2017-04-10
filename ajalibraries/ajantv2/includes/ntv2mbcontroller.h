#ifndef NTV2MBCONTROLLER_H
#define NTV2MBCONTROLLER_H

#include "ntv2card.h"
#include "ntv2mailbox.h"
#include <iostream>
#include <vector>

enum eMBCmd
{
    MB_CMD_SET_NET      = 0,
    MB_CMD_GET_MAC_FROM_ARP_TABLE = 3,
    MB_CMD_SEND_ARP_REQ        = 4,
    MB_CMD_UNKNOWN             = 5,
    MB_CMD_SET_IGMP_VERSION    = 6
};

enum eSFP
{
    SFP_TOP,
    SFP_BOTTOM,
    SFP_MAX_NUM_SFPS,
    SFP_INVALID		= SFP_MAX_NUM_SFPS
};

#define	NTV2_IS_VALID_SFP(__sfp__)		(((__sfp__) >= SFP_TOP)  &&  ((__sfp__) < SFP_INVALID))

enum eArpState
{
    ARP_ERROR,
    ARP_VALID,
    ARP_INCOMPLETE,
    ARP_NOT_FOUND
};

typedef enum
{
    eIGMPVersion_2,
    eIGMPVersion_3,
    eIGMPVersion_Default = eIGMPVersion_3
} eIGMPVersion_t;

typedef struct
{
    uint8_t	mac[6];
} MACAddr;


// IGMP Control Block
#define IGMPPCB_REG_STATE     0
#define IGMPCB_REG_ADDR       1
#define IGMPCB_SIZE           2

#define IGMPCB_STATE_USED     BIT(0)
#define IGMPCB_STATE_ENABLED  BIT(1)

class IPVNetConfig
{
public:
    IPVNetConfig() { init(); }

    void init();

    bool operator == ( const IPVNetConfig &other );
    bool operator != ( const IPVNetConfig &other );

    uint32_t    ipc_ip;
    uint32_t    ipc_subnet;
    uint32_t    ipc_gateway;
};

class AJAExport CNTV2MBController : public CNTV2MailBox
{
public:
    CNTV2MBController(CNTV2Card & device);

protected:
    // all these methods block until response received or timeout
    bool SetMBNetworkConfiguration (eSFP port, std::string ipaddr, std::string netmask,std::string gateway);
    bool GetRemoteMAC(std::string remote_IPAddress, std::string & MACaddress);
    bool SetIGMPVersion(uint32_t version);

    void SetIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream, uint32_t ipaddr, bool enable);
    void UnsetIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream);
    void EnableIGMPGroup(eSFP port, NTV2Channel channel, NTV2Stream stream, bool enable);

private:
    eArpState GetRemoteMACFromArpTable(std::string remote_IPAddress, std::string & MACaddress);
    bool SendArpRequest(std::string remote_IPAddress);

    void splitResponse(const std::string response, std::vector<std::string> & results);
    bool getDecimal(const std::string & resp, const std::string & parm, uint32_t & result);
    bool getHex(const std::string & resp, const std::string & parm, uint32_t &result);
    bool getString(const std::string & resp, const std::string & parm, std::string & result);
    uint32_t getIGMPCBOffset(eSFP port, NTV2Channel channel, NTV2Stream stream);

private:
};

#endif // NTV2MBCONTROLLER_H
