//-------------------------------------------------------------------------------------------
//	konaipjsonparse.cpp
//  Implementation of ip json parser for 2110.
//
//	Copyright (C) 2018 AJA Video Systems, Inc.  
//	Proprietary and Confidential information.  All rights reserved.
//-------------------------------------------------------------------------------------------
#include "konaipjsonparse.h"

const int kStrMax			= IP_STRSIZE-1;

//-------------------------------------------------------------------------------------------------
//	CKonaIpJsonParse2110
//-------------------------------------------------------------------------------------------------

CKonaIpJsonParse2110::CKonaIpJsonParse2110()
{
    m_verbose = false;
}

CKonaIpJsonParse2110::~CKonaIpJsonParse2110()
{
}

bool CKonaIpJsonParse2110::SetJson(const QJsonObject& topObj, bool verbose)
{
    if (topObj.isEmpty())
        return false;

    bool success = true;
    m_verbose = verbose;

    memset(&m_net2110, 0, sizeof(NetworkData2110));
    memset(&m_receiveVideo2110, 0, sizeof(ReceiveVideoData2110));
    memset(&m_receiveAudio2110, 0, sizeof(ReceiveAudioData2110));
    memset(&m_transmitVideo2110, 0, sizeof(TransmitVideoData2110));
    memset(&m_transmitAudio2110, 0, sizeof(TransmitAudioData2110));

    //
    // Network
    //
    if (topObj.contains("network2110"))
    {
        QJsonObject obj2110 = topObj["network2110"].toObject();
        success = SetJsonNetwork(obj2110) && success;
    }

    if (topObj.contains("receiveVideo2110"))
    {
        QJsonArray receiveVideoArray = topObj["receiveVideo2110"].toArray();
        success = SetJsonReceiveVideo(receiveVideoArray) && success;
    }

    if (topObj.contains("receiveAudio2110"))
    {
        QJsonArray receiveAudioArray = topObj["receiveAudio2110"].toArray();
        success = SetJsonReceiveAudio(receiveAudioArray) && success;
    }

    if (topObj.contains("transmitVideo2110"))
    {
        QJsonArray transmitVideoArray = topObj["transmitVideo2110"].toArray();
        success = SetJsonTransmitVideo(transmitVideoArray) && success;
    }

    if (topObj.contains("transmitAudio2110"))
    {
        QJsonArray transmitAudioArray = topObj["transmitAudio2110"].toArray();
        success = SetJsonTransmitAudio(transmitAudioArray) && success;
    }

    QJsonObject newTopObj;
    newTopObj.insert("network2110", m_netJson);
    newTopObj.insert("receiveVideo2110", m_receiveVideoJson);
    newTopObj.insert("receiveAudio2110", m_receiveAudioJson);
    newTopObj.insert("transmitVideo2110", m_transmitVideoJson);
    newTopObj.insert("transmitAudio2110", m_transmitAudioJson);
    m_topJson = newTopObj;

    return success;
}

bool CKonaIpJsonParse2110::SetJsonNetwork(const QJsonObject& topObj)
{
    NetworkData2110 n2110;
    bool success = JsonToStructNetwork(topObj, n2110);

    if (success)
    {
        n2110.id = n2110.id+1;
        memcpy(&m_net2110, &n2110, sizeof(NetworkData2110));
        m_netJson = topObj;
    }

    return success;
}

bool CKonaIpJsonParse2110::SetJsonReceiveVideo(const QJsonArray& jsonArray)
{
    ReceiveVideoData2110 rVideo2110;
    bool success = JsonToStructReceiveVideo(jsonArray, rVideo2110);

    if (success)
    {
        rVideo2110.id = rVideo2110.id+1;
        memcpy(&m_receiveVideo2110, &rVideo2110, sizeof(ReceiveVideoData2110));
        m_receiveVideoJson = jsonArray;
    }

    return success;
}

bool CKonaIpJsonParse2110::SetJsonReceiveAudio(const QJsonArray& jsonArray)
{
    ReceiveAudioData2110 rAudio2110;
    bool success = JsonToStructReceiveAudio(jsonArray, rAudio2110);

    if (success)
    {
        rAudio2110.id = rAudio2110.id+1;
        memcpy(&m_receiveAudio2110, &rAudio2110, sizeof(ReceiveAudioData2110));
        m_receiveAudioJson = jsonArray;
    }

    return success;
}

bool CKonaIpJsonParse2110::SetJsonTransmitVideo(const QJsonArray& jsonArray)
{
    TransmitVideoData2110 tVideo2110;
    bool success = JsonToStructTransmitVideo(jsonArray, tVideo2110);

    if (success)
    {
        tVideo2110.id = tVideo2110.id+1;
        memcpy(&m_transmitVideo2110, &tVideo2110, sizeof(TransmitVideoData2110));
        m_transmitVideoJson = jsonArray;
    }

    return success;
}

bool CKonaIpJsonParse2110::SetJsonTransmitAudio(const QJsonArray& jsonArray)
{
    TransmitAudioData2110 tAudio2110;
    bool success = JsonToStructTransmitAudio(jsonArray, tAudio2110);

    if (success)
    {
        tAudio2110.id = tAudio2110.id+1;
        memcpy(&m_transmitAudio2110, &tAudio2110, sizeof(TransmitAudioData2110));
        m_transmitAudioJson = jsonArray;
    }

    return success;
}

bool CKonaIpJsonParse2110::GetEnable(std::string enableBoolString)
{
    return (enableBoolString == "true");
}

QString CKonaIpJsonParse2110::GetEnable(bool enabled)
{
    if (enabled)
        return "true";
    else
        return "false";
}

NTV2Channel CKonaIpJsonParse2110::GetChannel(std::string channelString)
{
    NTV2Channel chan;

    if (channelString == "channel1")
        chan = NTV2_CHANNEL1;
    else if (channelString == "channel2")
        chan = NTV2_CHANNEL2;
    else if (channelString == "channel3")
        chan = NTV2_CHANNEL3;
    else if (channelString == "channel4")
        chan = NTV2_CHANNEL4;
    else
        chan = NTV2_CHANNEL_INVALID;

    return chan;
}

QString CKonaIpJsonParse2110::GetChannel(NTV2Channel channel)
{
    QString str;

    if (channel == NTV2_CHANNEL1)
        str = "channel1";
    else if (channel == NTV2_CHANNEL2)
        str = "channel2";
    else if (channel == NTV2_CHANNEL3)
        str = "channel3";
    else if (channel == NTV2_CHANNEL4)
        str = "channel4";
    else
        str = "invalid";

    return str;
}

eSFP CKonaIpJsonParse2110::GetSfp(std::string sfpString)
{
    eSFP sfp;

    if (sfpString == "sfp1")
        sfp = SFP_1;
    else if (sfpString == "sfp2")
        sfp = SFP_2;
    else
        sfp = SFP_INVALID;

    return sfp;
}

QString CKonaIpJsonParse2110::GetSfp(eSFP sfp)
{
    QString str;

    if (sfp == SFP_1)
        str = "sfp1";
    else if (sfp == SFP_2)
        str = "sfp2";
    else
        str = "invalid";

    return str;
}

NTV2Stream CKonaIpJsonParse2110::GetStream(std::string streamString)
{
    NTV2Stream stream;

    if (streamString == "video")
        stream = NTV2_VIDEO_STREAM;
    else if (streamString == "audio1")
        stream = NTV2_AUDIO1_STREAM;
    else if (streamString == "audio2")
        stream = NTV2_AUDIO2_STREAM;
    else if (streamString == "audio3")
        stream = NTV2_AUDIO2_STREAM;
    else if (streamString == "audio4")
        stream = NTV2_AUDIO2_STREAM;
    else
        stream = NTV2_STREAM_INVALID;

    return stream;
}

QString CKonaIpJsonParse2110::GetStream(NTV2Stream stream)
{
    QString str;

    if (stream == NTV2_VIDEO_STREAM)
        str = "video";
    else if (stream == NTV2_AUDIO1_STREAM)
        str = "audio1";
    else if (stream == NTV2_AUDIO2_STREAM)
        str = "audio2";
    else if (stream == NTV2_AUDIO3_STREAM)
        str = "audio3";
    else if (stream == NTV2_AUDIO4_STREAM)
        str = "audio4";
    else
        str = "invalid";

    return str;
}


bool CKonaIpJsonParse2110::JsonToStructNetwork(const QJsonObject& topObj, NetworkData2110& n2110)
{
    memset(&n2110, 0, sizeof(NetworkData2110));

    std::cout << "Network2110" << std::endl;

    // ptpMasterIP
    std::string str = topObj["ptpMasterIP"].toString().toStdString();
    if (m_verbose) std::cout << " ptpMasterIP " << str.c_str() << std::endl;
    strncpy(n2110.ptpMasterIP, str.c_str(), kStrMax);

    str = topObj["setup4k"].toString().toStdString();
    if (m_verbose) std::cout << " setup4k " << str.c_str() << std::endl;
    n2110.setup4k = GetEnable(str);

    // sfp
    QJsonArray sfpArray = topObj["sfps"].toArray();
    n2110.numSFPs = MinVal(sfpArray.count(), 2);
    if (n2110.numSFPs == 0)
        return false;

    for (uint32_t i=0; i<n2110.numSFPs; i++)
    {
        if (m_verbose) std::cout << " SFP " << i << std::endl;

        QJsonObject sfpObj = sfpArray[i].toObject();

        str = sfpObj["designator"].toString().toStdString();
        if (m_verbose) std::cout << "  designator " << str.c_str() << std::endl;
        n2110.sfp[i].sfp = GetSfp(str);

        str = sfpObj["ipAddress"].toString().toStdString();
        if (m_verbose) std::cout << "  ipAddress " << str.c_str() << std::endl;
        strncpy(n2110.sfp[i].ipAddress, str.c_str(), kStrMax);

        str = sfpObj["subnetMask"].toString().toStdString();
        if (m_verbose) std::cout << "  subnetMask " << str.c_str() << std::endl;
        strncpy(n2110.sfp[i].subnetMask, str.c_str(), kStrMax);

        str = sfpObj["gateWay"].toString().toStdString();
        if (m_verbose) std::cout << "  gateWay " << str.c_str() << std::endl;
        strncpy(n2110.sfp[i].gateWay, str.c_str(), kStrMax);

        str = sfpObj["enable"].toString().toStdString();
        if (m_verbose) std::cout << "  enable " << str.c_str() << std::endl;
        n2110.sfp[i].enable = GetEnable(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonNetwork(const NetworkData2110& n2110, QJsonObject& topObj)
{
    topObj.insert("ptpMasterIP", QJsonValue(QString(n2110.ptpMasterIP)));
    topObj.insert("setup4k", QJsonValue(QString(GetEnable(n2110.setup4k))));

    QJsonArray sfpArray;

    for (uint32_t i=0; i<n2110.numSFPs; i++)
    {
        QJsonObject obj;
        if (i == 0)
            obj.insert("designator", QJsonValue(QString("sfp1")));
        else
            obj.insert("designator", QJsonValue(QString("sfp2")));

        obj.insert("ipAddress", QJsonValue(QString(n2110.sfp[i].ipAddress)));
        obj.insert("subnetMask", QJsonValue(QString(n2110.sfp[i].subnetMask)));
        obj.insert("gateWay", QJsonValue(QString(n2110.sfp[i].gateWay)));
        obj.insert("enable", QJsonValue(QString(GetEnable(n2110.sfp[i].enable))));
        sfpArray += QJsonValue(obj);
    }
    topObj.insert("sfps", QJsonValue(sfpArray));

    return true;
}

bool CKonaIpJsonParse2110::JsonToStructReceiveVideo(const QJsonArray& vArray, ReceiveVideoData2110& rVideo2110)
{
    memset(&rVideo2110, 0, sizeof(ReceiveVideoData2110));

    std::cout << "ReceiveVideo2110" << std::endl;

    // up to 4 channels
    rVideo2110.numRxVideoChannels = MinVal(vArray.count(), 4);
    if (rVideo2110.numRxVideoChannels == 0)
        return false;

    std::string str;

    for (uint32_t i=0; i<rVideo2110.numRxVideoChannels; i++)
    {
        QJsonObject vObj = vArray[i].toObject();

        rVideo2110.rxVideoCh[i].sourcePort[0]	= vObj["sfp1srcPort"].toInt();
        rVideo2110.rxVideoCh[i].destPort[0]     = vObj["sfp1DestPort"].toInt();
        rVideo2110.rxVideoCh[i].rxMatch[0]		= vObj["sfp1Filter"].toInt();
        str = vObj["sfp1srcIPAddress"].toString().toStdString();
        strncpy(rVideo2110.rxVideoCh[i].sourceIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1DestIPAddress"].toString().toStdString();
        strncpy(rVideo2110.rxVideoCh[i].destIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1Enable"].toString().toStdString();
        rVideo2110.rxVideoCh[i].sfpEnable[0] = GetEnable(str);

        rVideo2110.rxVideoCh[i].sourcePort[1]	= vObj["sfp2srcPort"].toInt();
        rVideo2110.rxVideoCh[i].destPort[1]     = vObj["sfp2DestPort"].toInt();
        rVideo2110.rxVideoCh[i].rxMatch[1]		= vObj["sfp2Filter"].toInt();
        str = vObj["sfp2SourceIP"].toString().toStdString();
        strncpy(rVideo2110.rxVideoCh[i].sourceIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2DestIP"].toString().toStdString();
        strncpy(rVideo2110.rxVideoCh[i].destIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2Enable"].toString().toStdString();
        rVideo2110.rxVideoCh[i].sfpEnable[1] = GetEnable(str);

        rVideo2110.rxVideoCh[i].vlan            = vObj["vlan"].toInt();
        rVideo2110.rxVideoCh[i].ssrc            = vObj["ssrc"].toInt();
        rVideo2110.rxVideoCh[i].payload         = vObj["payload"].toInt();
        str = vObj["enable"].toString().toStdString();
        rVideo2110.rxVideoCh[i].enable = GetEnable(str);
        str = vObj["designator"].toString().toStdString();
        rVideo2110.rxVideoCh[i].channel = GetChannel(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonReceiveVideo(const ReceiveVideoData2110& rVideo2110, QJsonArray& vArray)
{
    for (uint32_t i=0; i<rVideo2110.numRxVideoChannels; i++)
    {
        QJsonObject obj;
        obj.insert("sfp1srcPort",       QJsonValue((int)rVideo2110.rxVideoCh[i].sourcePort[0]));
        obj.insert("sfp1DestPort",      QJsonValue((int)rVideo2110.rxVideoCh[i].destPort[0]));
        obj.insert("sfp1Filter",        QJsonValue((int)rVideo2110.rxVideoCh[i].rxMatch[0]));
        obj.insert("sfp1srcIPAddress",  QJsonValue(QString(rVideo2110.rxVideoCh[i].sourceIP[0])));
        obj.insert("sfp1DestIPAddress", QJsonValue(QString(rVideo2110.rxVideoCh[i].destIP[0])));
        obj.insert("sfp1Enable",        QJsonValue(QString(GetEnable(rVideo2110.rxVideoCh[i].sfpEnable[0]))));

        obj.insert("sfp2srcPort",       QJsonValue((int)rVideo2110.rxVideoCh[i].sourcePort[1]));
        obj.insert("sfp2DestPort",      QJsonValue((int)rVideo2110.rxVideoCh[i].destPort[1]));
        obj.insert("sfp2Filter",        QJsonValue((int)rVideo2110.rxVideoCh[i].rxMatch[1]));
        obj.insert("sfp2srcIPAddress",  QJsonValue(QString(rVideo2110.rxVideoCh[i].sourceIP[1])));
        obj.insert("sfp2DestIPAddress", QJsonValue(QString(rVideo2110.rxVideoCh[i].destIP[1])));
        obj.insert("sfp2Enable",        QJsonValue(QString(GetEnable(rVideo2110.rxVideoCh[i].sfpEnable[1]))));

        obj.insert("vlan",              QJsonValue((int)rVideo2110.rxVideoCh[i].vlan));
        obj.insert("ssrc",              QJsonValue((int)rVideo2110.rxVideoCh[i].ssrc));
        obj.insert("payload",           QJsonValue((int)rVideo2110.rxVideoCh[i].payload));
        obj.insert("enable",            QJsonValue(QString(GetEnable(rVideo2110.rxVideoCh[i].enable))));
        obj.insert("designator",        QJsonValue(QString(GetEnable(rVideo2110.rxVideoCh[i].channel))));

        vArray += QJsonValue(obj);
    }

    return true;
}

bool CKonaIpJsonParse2110::JsonToStructReceiveAudio(const QJsonArray& aArray, ReceiveAudioData2110& rAudio2110)
{
    memset(&rAudio2110, 0, sizeof(ReceiveAudioData2110));

    std::cout << "ReceiveAudio2110" << std::endl;

    // up to 4 channels
    rAudio2110.numRxAudioChannels = MinVal(aArray.count(), 4);
    if (rAudio2110.numRxAudioChannels == 0)
        return false;

    std::string str;

    for (uint32_t i=0; i<rAudio2110.numRxAudioChannels; i++)
    {
        QJsonObject vObj = aArray[i].toObject();

        rAudio2110.rxAudioCh[i].sourcePort[0]	= vObj["sfp1srcPort"].toInt();
        rAudio2110.rxAudioCh[i].destPort[0]     = vObj["sfp1DestPort"].toInt();
        rAudio2110.rxAudioCh[i].rxMatch[0]		= vObj["sfp1Filter"].toInt();
        str = vObj["sfp1srcIPAddress"].toString().toStdString();
        strncpy(rAudio2110.rxAudioCh[i].sourceIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1DestIPAddress"].toString().toStdString();
        strncpy(rAudio2110.rxAudioCh[i].destIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1Enable"].toString().toStdString();
        rAudio2110.rxAudioCh[i].sfpEnable[0] = GetEnable(str);

        rAudio2110.rxAudioCh[i].sourcePort[1]	= vObj["sfp2srcPort"].toInt();
        rAudio2110.rxAudioCh[i].destPort[1]     = vObj["sfp2DestPort"].toInt();
        rAudio2110.rxAudioCh[i].rxMatch[1]		= vObj["sfp2Filter"].toInt();
        str = vObj["sfp2SourceIP"].toString().toStdString();
        strncpy(rAudio2110.rxAudioCh[i].sourceIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2DestIP"].toString().toStdString();
        strncpy(rAudio2110.rxAudioCh[i].destIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2Enable"].toString().toStdString();
        rAudio2110.rxAudioCh[i].sfpEnable[1] = GetEnable(str);

        rAudio2110.rxAudioCh[i].vlan                = vObj["vlan"].toInt();
        rAudio2110.rxAudioCh[i].ssrc                = vObj["ssrc"].toInt();
        rAudio2110.rxAudioCh[i].payload             = vObj["payload"].toInt();
        rAudio2110.rxAudioCh[i].numAudioChannels    = vObj["numAudioChannels"].toInt();
        rAudio2110.rxAudioCh[i].audioPktInterval    = (eNTV2PacketInterval)vObj["audioPktInterval"].toInt();
        str = vObj["enable"].toString().toStdString();
        rAudio2110.rxAudioCh[i].enable = GetEnable(str);
        str = vObj["stream"].toString().toStdString();
        rAudio2110.rxAudioCh[i].stream = GetStream(str);
        str = vObj["designator"].toString().toStdString();
        rAudio2110.rxAudioCh[i].channel = GetChannel(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonReceiveAudio(const ReceiveAudioData2110& rAudio2110, QJsonArray& aArray)
{
    for (uint32_t i=0; i<rAudio2110.numRxAudioChannels; i++)
    {
        QJsonObject obj;
        obj.insert("sfp1srcPort",           QJsonValue((int)rAudio2110.rxAudioCh[i].sourcePort[0]));
        obj.insert("sfp1DestPort",          QJsonValue((int)rAudio2110.rxAudioCh[i].destPort[0]));
        obj.insert("sfp1Filter",            QJsonValue((int)rAudio2110.rxAudioCh[i].rxMatch[0]));
        obj.insert("sfp1srcIPAddress",      QJsonValue(QString(rAudio2110.rxAudioCh[i].sourceIP[0])));
        obj.insert("sfp1DestIPAddress",     QJsonValue(QString(rAudio2110.rxAudioCh[i].destIP[0])));
        obj.insert("sfp1Enable",            QJsonValue(QString(GetEnable(rAudio2110.rxAudioCh[i].sfpEnable[0]))));

        obj.insert("sfp2srcPort",           QJsonValue((int)rAudio2110.rxAudioCh[i].sourcePort[1]));
        obj.insert("sfp2DestPort",          QJsonValue((int)rAudio2110.rxAudioCh[i].destPort[1]));
        obj.insert("sfp2Filter",            QJsonValue((int)rAudio2110.rxAudioCh[i].rxMatch[1]));
        obj.insert("sfp2srcIPAddress",      QJsonValue(QString(rAudio2110.rxAudioCh[i].sourceIP[1])));
        obj.insert("sfp2DestIPAddress",     QJsonValue(QString(rAudio2110.rxAudioCh[i].destIP[1])));
        obj.insert("sfp2Enable",            QJsonValue(QString(GetEnable(rAudio2110.rxAudioCh[i].sfpEnable[1]))));

        obj.insert("vlan",                  QJsonValue((int)rAudio2110.rxAudioCh[i].vlan));
        obj.insert("ssrc",                  QJsonValue((int)rAudio2110.rxAudioCh[i].ssrc));
        obj.insert("payload",               QJsonValue((int)rAudio2110.rxAudioCh[i].payload));
        obj.insert("numAudioChannels",      QJsonValue((int)rAudio2110.rxAudioCh[i].numAudioChannels));
        obj.insert("audioPktInterval",      QJsonValue((int)rAudio2110.rxAudioCh[i].audioPktInterval));

        obj.insert("enable",                QJsonValue(QString(GetEnable(rAudio2110.rxAudioCh[i].enable))));
        obj.insert("stream",                QJsonValue(QString(GetStream(rAudio2110.rxAudioCh[i].stream))));
        obj.insert("designator",            QJsonValue(QString(GetEnable(rAudio2110.rxAudioCh[i].channel))));

        aArray += QJsonValue(obj);
    }

    return true;
}

bool CKonaIpJsonParse2110::JsonToStructTransmitVideo(const QJsonArray& vArray, TransmitVideoData2110& tVideo2110)
{
    memset(&tVideo2110, 0, sizeof(TransmitVideoData2110));

    // up to 4 channels
    tVideo2110.numTxVideoChannels = MinVal(vArray.count(), 4);
    if (tVideo2110.numTxVideoChannels == 0)
        return false;

    std::string str;

    for (uint32_t i=0; i<tVideo2110.numTxVideoChannels; i++)
    {
        QJsonObject vObj = vArray[i].toObject();

        tVideo2110.txVideoCh[i].localPort[0]	= vObj["sfp1LocalPort"].toInt();
        tVideo2110.txVideoCh[i].remotePort[0]   = vObj["sfp1RemotePort"].toInt();
        str = vObj["sfp1RemoteIPAddress"].toString().toStdString();
        strncpy(tVideo2110.txVideoCh[i].remoteIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1Enable"].toString().toStdString();
        tVideo2110.txVideoCh[i].sfpEnable[0] = GetEnable(str);

        tVideo2110.txVideoCh[i].localPort[1]	= vObj["sfp2LocalPort"].toInt();
        tVideo2110.txVideoCh[i].remotePort[1]   = vObj["sfp2RemotePort"].toInt();
        str = vObj["sfp2RemoteIPAddress"].toString().toStdString();
        strncpy(tVideo2110.txVideoCh[i].remoteIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2Enable"].toString().toStdString();
        tVideo2110.txVideoCh[i].sfpEnable[1] = GetEnable(str);

        tVideo2110.txVideoCh[i].ttl             = vObj["ttl"].toInt();
        tVideo2110.txVideoCh[i].ssrc            = vObj["ssrc"].toInt();
        tVideo2110.txVideoCh[i].payload         = vObj["payload"].toInt();
        str = vObj["enable"].toString().toStdString();
        tVideo2110.txVideoCh[i].enable = GetEnable(str);
        str = vObj["designator"].toString().toStdString();
        tVideo2110.txVideoCh[i].channel = GetChannel(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonTransmitVideo(const TransmitVideoData2110& tVideo2110, QJsonArray& vArray)
{
    for (uint32_t i=0; i<tVideo2110.numTxVideoChannels; i++)
    {
        QJsonObject obj;
        obj.insert("sfp1LocalPort",         QJsonValue((int)tVideo2110.txVideoCh[i].localPort[0]));
        obj.insert("sfp1RemoteIPAddress",   QJsonValue(QString(tVideo2110.txVideoCh[i].remoteIP[0])));
        obj.insert("sfp1RemotePort",        QJsonValue((int)tVideo2110.txVideoCh[i].remotePort[0]));
        obj.insert("sfp1Enable",            QJsonValue(QString(GetEnable(tVideo2110.txVideoCh[i].sfpEnable[0]))));

        obj.insert("sfp2LocalPort",         QJsonValue((int)tVideo2110.txVideoCh[i].localPort[1]));
        obj.insert("sfp2RemoteIPAddress",   QJsonValue(QString(tVideo2110.txVideoCh[i].remoteIP[1])));
        obj.insert("sfp2RemotePort",        QJsonValue((int)tVideo2110.txVideoCh[i].remotePort[1]));
        obj.insert("sfp2Enable",            QJsonValue(QString(GetEnable(tVideo2110.txVideoCh[i].sfpEnable[1]))));

        obj.insert("ttl",                   QJsonValue((int)tVideo2110.txVideoCh[i].ttl));
        obj.insert("ssrc",                  QJsonValue((int)tVideo2110.txVideoCh[i].ssrc));
        obj.insert("payload",               QJsonValue((int)tVideo2110.txVideoCh[i].payload));

        obj.insert("enable",                QJsonValue(QString(GetEnable(tVideo2110.txVideoCh[i].enable))));
        obj.insert("designator",            QJsonValue(QString(GetEnable(tVideo2110.txVideoCh[i].channel))));
        vArray += QJsonValue(obj);
    }

    return true;
}

bool CKonaIpJsonParse2110::JsonToStructTransmitAudio(const QJsonArray& aArray, TransmitAudioData2110& tAudio2110)
{
    memset(&tAudio2110, 0, sizeof(TransmitAudioData2110));

    // up to 4 channels
    tAudio2110.numTxAudioChannels = MinVal(aArray.count(), 4);
    if (tAudio2110.numTxAudioChannels == 0)
        return false;

    std::string str;

    for (uint32_t i=0; i<tAudio2110.numTxAudioChannels; i++)
    {
        QJsonObject vObj = aArray[i].toObject();

        tAudio2110.txAudioCh[i].localPort[0]	= vObj["sfp1LocalPort"].toInt();
        tAudio2110.txAudioCh[i].remotePort[0]   = vObj["sfp1RemotePort"].toInt();
        str = vObj["sfp1RemoteIPAddress"].toString().toStdString();
        strncpy(tAudio2110.txAudioCh[i].remoteIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1Enable"].toString().toStdString();
        tAudio2110.txAudioCh[i].sfpEnable[0] = GetEnable(str);

        tAudio2110.txAudioCh[i].localPort[1]	= vObj["sfp2LocalPort"].toInt();
        tAudio2110.txAudioCh[i].remotePort[1]   = vObj["sfp2RemotePort"].toInt();
        str = vObj["sfp2RemoteIPAddress"].toString().toStdString();
        strncpy(tAudio2110.txAudioCh[i].remoteIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2Enable"].toString().toStdString();
        tAudio2110.txAudioCh[i].sfpEnable[1] = GetEnable(str);

        tAudio2110.txAudioCh[i].ttl                 = vObj["ttl"].toInt();
        tAudio2110.txAudioCh[i].ssrc                = vObj["ssrc"].toInt();
        tAudio2110.txAudioCh[i].payload             = vObj["payload"].toInt();
        tAudio2110.txAudioCh[i].numAudioChannels    = vObj["numAudioChannels"].toInt();
        tAudio2110.txAudioCh[i].firstAudioChannel   = vObj["firstAudioChannel"].toInt();
        tAudio2110.txAudioCh[i].audioPktInterval    = (eNTV2PacketInterval)vObj["audioPacketInterval"].toInt();

        str = vObj["enable"].toString().toStdString();
        tAudio2110.txAudioCh[i].enable = GetEnable(str);
        str = vObj["stream"].toString().toStdString();
        tAudio2110.txAudioCh[i].stream = GetStream(str);
        str = vObj["designator"].toString().toStdString();
        tAudio2110.txAudioCh[i].channel = GetChannel(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonTransmitAudio(const TransmitAudioData2110& tAudio2110, QJsonArray& aArray)
{
    for (uint32_t i=0; i<tAudio2110.numTxAudioChannels; i++)
    {
        QJsonObject obj;
        obj.insert("sfp1LocalPort",         QJsonValue((int)tAudio2110.txAudioCh[i].localPort[0]));
        obj.insert("sfp1RemoteIPAddress",   QJsonValue(QString(tAudio2110.txAudioCh[i].remoteIP[0])));
        obj.insert("sfp1RemotePort",        QJsonValue((int)tAudio2110.txAudioCh[i].remotePort[0]));
        obj.insert("sfp1Enable",            QJsonValue(QString(GetEnable(tAudio2110.txAudioCh[i].sfpEnable[0]))));

        obj.insert("sfp2LocalPort",         QJsonValue((int)tAudio2110.txAudioCh[i].localPort[1]));
        obj.insert("sfp2RemoteIPAddress",   QJsonValue(QString(tAudio2110.txAudioCh[i].remoteIP[1])));
        obj.insert("sfp2RemotePort",        QJsonValue((int)tAudio2110.txAudioCh[i].remotePort[1]));
        obj.insert("sfp2Enable",            QJsonValue(QString(GetEnable(tAudio2110.txAudioCh[i].sfpEnable[1]))));

        obj.insert("ttl",                   QJsonValue((int)tAudio2110.txAudioCh[i].ttl));
        obj.insert("ssrc",                  QJsonValue((int)tAudio2110.txAudioCh[i].ssrc));
        obj.insert("payload",               QJsonValue((int)tAudio2110.txAudioCh[i].payload));
        obj.insert("numAudioChannels",      QJsonValue((int)tAudio2110.txAudioCh[i].numAudioChannels));
        obj.insert("firstAudioChannel",     QJsonValue((int)tAudio2110.txAudioCh[i].firstAudioChannel));
        obj.insert("audioPktInterval",      QJsonValue((int)tAudio2110.txAudioCh[i].audioPktInterval));

        obj.insert("enable",                QJsonValue(QString(GetEnable(tAudio2110.txAudioCh[i].enable))));
        obj.insert("stream",                QJsonValue(QString(GetStream(tAudio2110.txAudioCh[i].stream))));
        obj.insert("designator",            QJsonValue(QString(GetEnable(tAudio2110.txAudioCh[i].channel))));
        aArray += QJsonValue(obj);
    }

    return true;
}
