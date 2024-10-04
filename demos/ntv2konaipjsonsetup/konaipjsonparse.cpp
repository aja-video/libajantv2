/* SPDX-License-Identifier: MIT */
//-------------------------------------------------------------------------------------------
//	konaipjsonparse.cpp
//  Implementation of ip json parser for 2110.
//
//	Copyright (C) 2018 AJA Video Systems, Inc.  
//	Proprietary and Confidential information.  All rights reserved.
//-------------------------------------------------------------------------------------------
#include "konaipjsonparse.h"

#ifdef BUILD_DEMO
    #include "ntv2democommon.h"
#endif


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
	memset(&m_receiveAnc2110, 0, sizeof(ReceiveAncData2110));
	memset(&m_transmitVideo2110, 0, sizeof(TransmitVideoData2110));
	memset(&m_transmitAudio2110, 0, sizeof(TransmitAudioData2110));
	memset(&m_transmitAnc2110, 0, sizeof(TransmitAncData2110));

    //
    // Network
    //
	if (topObj.contains("protocol"))
	{
		QJsonObject obj2110 = topObj["protocol"].toObject();
		success = SetJsonProtocol(obj2110) && success;
	}

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

	if (topObj.contains("receiveAnc2110"))
	{
		QJsonArray receiveAncArray = topObj["receiveAnc2110"].toArray();
		success = SetJsonReceiveAnc(receiveAncArray) && success;
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

	if (topObj.contains("transmitAnc2110"))
	{
		QJsonArray transmitAncArray = topObj["transmitAnc2110"].toArray();
		success = SetJsonTransmitAnc(transmitAncArray) && success;
	}

    QJsonObject newTopObj;
	newTopObj.insert("protocol", m_protocolJson);
	newTopObj.insert("network2110", m_netJson);
	newTopObj.insert("receiveVideo2110", m_receiveVideoJson);
	newTopObj.insert("receiveAudio2110", m_receiveAudioJson);
	newTopObj.insert("receiveAnc2110", m_receiveAncJson);
	newTopObj.insert("transmitVideo2110", m_transmitVideoJson);
	newTopObj.insert("transmitAudio2110", m_transmitAudioJson);
	newTopObj.insert("transmitAnc2110", m_transmitAncJson);
	m_topJson = newTopObj;

    return success;
}

bool CKonaIpJsonParse2110::SetJsonProtocol(const QJsonObject& topObj)
{
	m_protocolJson = topObj;
	return true;
}

bool CKonaIpJsonParse2110::SetJsonNetwork(const QJsonObject& topObj)
{
    NetworkData2110 n2110;
    bool success = JsonToStructNetwork(topObj, n2110);

    if (success)
    {
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
        memcpy(&m_receiveAudio2110, &rAudio2110, sizeof(ReceiveAudioData2110));
        m_receiveAudioJson = jsonArray;
    }

    return success;
}

bool CKonaIpJsonParse2110::SetJsonReceiveAnc(const QJsonArray& jsonArray)
{
	ReceiveAncData2110 rAnc2110;
	bool success = JsonToStructReceiveAnc(jsonArray, rAnc2110);

	if (success)
	{
		memcpy(&m_receiveAnc2110, &rAnc2110, sizeof(ReceiveAncData2110));
		m_receiveAncJson = jsonArray;
	}

	return success;
}

bool CKonaIpJsonParse2110::SetJsonTransmitVideo(const QJsonArray& jsonArray)
{
    TransmitVideoData2110 tVideo2110;
    bool success = JsonToStructTransmitVideo(jsonArray, tVideo2110);

    if (success)
    {
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
        memcpy(&m_transmitAudio2110, &tAudio2110, sizeof(TransmitAudioData2110));
        m_transmitAudioJson = jsonArray;
    }

    return success;
}

bool CKonaIpJsonParse2110::SetJsonTransmitAnc(const QJsonArray& jsonArray)
{
	TransmitAncData2110 tAnc2110;
	bool success = JsonToStructTransmitAnc(jsonArray, tAnc2110);

	if (success)
	{
		memcpy(&m_transmitAnc2110, &tAnc2110, sizeof(TransmitAncData2110));
		m_transmitAncJson = jsonArray;
	}

	return success;
}

bool CKonaIpJsonParse2110::GetEnable(const std::string enableBoolString)
{
    return (enableBoolString == "true");
}

QString CKonaIpJsonParse2110::GetEnable(const bool enabled)
{
    if (enabled)
        return "true";
    else
        return "false";
}

VPIDSampling CKonaIpJsonParse2110::GetSampling(const std::string samplingString)
{
	VPIDSampling sampling;

	if (samplingString == "RGB")
		sampling = VPIDSampling_GBR_444;
	else
		sampling = VPIDSampling_YUV_422;

	return sampling;
}

QString CKonaIpJsonParse2110::GetSampling(const VPIDSampling sampling)
{
	QString str;

	if (sampling == VPIDSampling_YUV_422)
		str = "YCbCr-4:2:2";
	else if (sampling == VPIDSampling_GBR_444)
		str = "RGB";
	else
		str = "invalid";

	return str;
}

NTV2Channel CKonaIpJsonParse2110::GetChannel(const std::string channelString)
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

QString CKonaIpJsonParse2110::GetChannel(const NTV2Channel channel)
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

eSFP CKonaIpJsonParse2110::GetSfp(const std::string sfpString)
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

QString CKonaIpJsonParse2110::GetSfp(const eSFP sfp)
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

NTV2Stream CKonaIpJsonParse2110::GetVideoStream(const std::string streamString)
{
    NTV2Stream stream;

    if (streamString == "video1")
        stream = NTV2_VIDEO1_STREAM;
    else if (streamString == "video2")
        stream = NTV2_VIDEO2_STREAM;
    else if (streamString == "video3")
        stream = NTV2_VIDEO3_STREAM;
    else if (streamString == "video4")
        stream = NTV2_VIDEO4_STREAM;
    else
        stream = NTV2_STREAM_INVALID;

    return stream;
}

QString CKonaIpJsonParse2110::GetVideoStream(const NTV2Stream stream)
{
    QString str;

    if (stream == NTV2_VIDEO1_STREAM)
        str = "video1";
    else if (stream == NTV2_VIDEO2_STREAM)
        str = "video2";
    else if (stream == NTV2_VIDEO3_STREAM)
        str = "video3";
    else if (stream == NTV2_VIDEO4_STREAM)
        str = "video4";
    else
        str = "invalid";

    return str;
}

NTV2Stream CKonaIpJsonParse2110::GetAudioStream(const std::string streamString)
{
    NTV2Stream stream;

    if (streamString == "audio1")
        stream = NTV2_AUDIO1_STREAM;
    else if (streamString == "audio2")
        stream = NTV2_AUDIO2_STREAM;
    else if (streamString == "audio3")
        stream = NTV2_AUDIO3_STREAM;
    else if (streamString == "audio4")
        stream = NTV2_AUDIO4_STREAM;
    else
        stream = NTV2_STREAM_INVALID;

    return stream;
}

QString CKonaIpJsonParse2110::GetAudioStream(const NTV2Stream stream)
{
    QString str;

    if (stream == NTV2_AUDIO1_STREAM)
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

NTV2Stream CKonaIpJsonParse2110::GetAncStream(const std::string streamString)
{
	NTV2Stream stream;

	if (streamString == "anc1")
		stream = NTV2_ANC1_STREAM;
	else if (streamString == "anc2")
		stream = NTV2_ANC2_STREAM;
	else if (streamString == "anc3")
		stream = NTV2_ANC3_STREAM;
	else if (streamString == "anc4")
		stream = NTV2_ANC4_STREAM;
	else
		stream = NTV2_STREAM_INVALID;

	return stream;
}

QString CKonaIpJsonParse2110::GetAncStream(const NTV2Stream stream)
{
	QString str;

	if (stream == NTV2_ANC1_STREAM)
		str = "anc1";
	else if (stream == NTV2_ANC2_STREAM)
		str = "anc2";
	else if (stream == NTV2_ANC3_STREAM)
		str = "anc3";
	else if (stream == NTV2_ANC4_STREAM)
		str = "anc4";
	else
		str = "invalid";

	return str;
}

eNTV2PacketInterval CKonaIpJsonParse2110::GetAudioPktInterval(const std::string pktIntervalString)
{
    eNTV2PacketInterval pktInterval;

    if (pktIntervalString == "1000us")
        pktInterval = PACKET_INTERVAL_1mS;
    else if (pktIntervalString == "125us")
        pktInterval = PACKET_INTERVAL_125uS;
    else
        pktInterval = PACKET_INTERVAL_125uS;

    return pktInterval;
}

QString CKonaIpJsonParse2110::GetAudioPktInterval(const eNTV2PacketInterval pktInterval)
{
    QString str;

    if (pktInterval == PACKET_INTERVAL_1mS)
        str = "1000us";
    else if (pktInterval == PACKET_INTERVAL_125uS)
        str = "125us";
    else
        str = "invalid";

    return str;
}

void CKonaIpJsonParse2110::GetGrandMasterID(const std::string str, uint8_t (&id)[8])
{
    QString idstr = QString::fromStdString(str);
    for (int i=0; i<8; i++)
    {
        id[i] = 0;
    }

    if (idstr != "")
    {
        if (idstr.contains('-') || idstr.contains(':'))
        {
            QByteArray vals = QByteArray::fromHex(idstr.toLatin1());
            if (vals.size() == 8)
            {
                for (int i=0; i<8; i++)
                {
                    id[i] = vals[i];
                }
            }
        }
    }
}

QString CKonaIpJsonParse2110::GetGrandMasterID(const uint8_t id[8])
{
	char buf[256];
    snprintf(buf, sizeof(buf), "%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",
            id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
    return QString(buf);
}

bool CKonaIpJsonParse2110::JsonToStructNetwork(const QJsonObject& topObj, NetworkData2110& n2110)
{
    memset(&n2110, 0, sizeof(NetworkData2110));
    std::string str;

    std::cout << "Network2110" << std::endl;

    n2110.ptpDomain = topObj["ptpDomain"].toInt();
    if (m_verbose) std::cout << " ptpDomain " << n2110.ptpDomain << std::endl;

    str = topObj["ptpPreferredGMID"].toString().toStdString();
    if (m_verbose) std::cout << " ptpPreferredGMID " << str.c_str() << std::endl;
    GetGrandMasterID(str, n2110.ptpPreferredGMID);

    str = topObj["setup4k"].toString().toStdString();
    if (m_verbose) std::cout << " setup4k " << str.c_str() << std::endl;
    n2110.setup4k = GetEnable(str);

	str = topObj["multiSDP"].toString().toStdString();
	if (m_verbose) std::cout << " multiSDP " << str.c_str() << std::endl;
	n2110.multiSDP = GetEnable(str);

	str = topObj["audioCombine"].toString().toStdString();
	if (m_verbose) std::cout << " audioCombine " << str.c_str() << std::endl;
	n2110.audioCombine = GetEnable(str);

	n2110.rxMatchOverride = topObj["rxMatchOverride"].toInt();
	if (m_verbose) std::cout << " rxMatchOverride " << n2110.rxMatchOverride << std::endl;

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
        if (m_verbose) std::cout << "  enable " << str.c_str() << std::endl << std::endl;
        n2110.sfp[i].enable = GetEnable(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonNetwork(const NetworkData2110& n2110, QJsonObject& topObj)
{
    topObj.insert("ptpDomain", QJsonValue((int)n2110.ptpDomain));
    topObj.insert("ptpPreferredGMID", QJsonValue(QString(GetGrandMasterID(n2110.ptpPreferredGMID))));

    topObj.insert("setup4k", QJsonValue(QString(GetEnable(n2110.setup4k))));

	topObj.insert("multiSDP", QJsonValue(QString(GetEnable(n2110.multiSDP))));

	topObj.insert("audioCombine", QJsonValue(QString(GetEnable(n2110.audioCombine))));

	topObj.insert("rxMatchOverride", QJsonValue((int)n2110.rxMatchOverride));

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
    
    if (n2110.numSFPs > 0)
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
        if (m_verbose) std::cout << " sfp1srcPort " << rVideo2110.rxVideoCh[i].sourcePort[0] << std::endl;
        rVideo2110.rxVideoCh[i].destPort[0]     = vObj["sfp1DestPort"].toInt();
        if (m_verbose) std::cout << " sfp1DestPort " << rVideo2110.rxVideoCh[i].destPort[0] << std::endl;
        str = vObj["sfp1srcIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1srcIPAddress " << str.c_str() << std::endl;
        strncpy(rVideo2110.rxVideoCh[i].sourceIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1DestIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1DestIPAddress " << str.c_str() << std::endl;
        strncpy(rVideo2110.rxVideoCh[i].destIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1Enable"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1Enable " << str.c_str() << std::endl;
        rVideo2110.rxVideoCh[i].sfpEnable[0] = GetEnable(str);

        rVideo2110.rxVideoCh[i].sourcePort[1]	= vObj["sfp2srcPort"].toInt();
        if (m_verbose) std::cout << " sfp2srcPort " << rVideo2110.rxVideoCh[i].sourcePort[1] << std::endl;
        rVideo2110.rxVideoCh[i].destPort[1]     = vObj["sfp2DestPort"].toInt();
        if (m_verbose) std::cout << " sfp2DestPort " << rVideo2110.rxVideoCh[i].destPort[1] << std::endl;
        str = vObj["sfp2srcIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2srcIPAddress " << str.c_str() << std::endl;
        strncpy(rVideo2110.rxVideoCh[i].sourceIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2DestIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2DestIPAddress " << str.c_str() << std::endl;
        strncpy(rVideo2110.rxVideoCh[i].destIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2Enable"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2Enable " << str.c_str() << std::endl;
        rVideo2110.rxVideoCh[i].sfpEnable[1] = GetEnable(str);

        rVideo2110.rxVideoCh[i].vlan            = vObj["vlan"].toInt();
        if (m_verbose) std::cout << " vlan " << rVideo2110.rxVideoCh[i].vlan << std::endl;
        rVideo2110.rxVideoCh[i].ssrc            = vObj["ssrc"].toInt();
        if (m_verbose) std::cout << " ssrc " << rVideo2110.rxVideoCh[i].ssrc << std::endl;
        rVideo2110.rxVideoCh[i].payloadType     = vObj["payloadType"].toInt();
        if (m_verbose) std::cout << " payloadType " << rVideo2110.rxVideoCh[i].payloadType << std::endl;
        str = vObj["videoFormat"].toString().toStdString();
        if (m_verbose) std::cout << " videoFormat " << str.c_str() << std::endl;
#ifdef BUILD_DEMO
        rVideo2110.rxVideoCh[i].videoFormat = CNTV2DemoCommon::GetVideoFormatFromString(str, VIDEO_FORMATS_ALL);
#else
        rVideo2110.rxVideoCh[i].videoFormat = NTV2_FORMAT_UNKNOWN;
#endif
        str = vObj["enable"].toString().toStdString();
        if (m_verbose) std::cout << " enable " << str.c_str() << std::endl;
        rVideo2110.rxVideoCh[i].enable = GetEnable(str);
        str = vObj["stream"].toString().toStdString();
        if (m_verbose) std::cout << " stream " << str.c_str() << std::endl << std::endl;
        rVideo2110.rxVideoCh[i].stream = GetVideoStream(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonReceiveVideo(const ReceiveVideoData2110& rVideo2110, QJsonArray& vArray)
{
    for (uint32_t i=0; i<rVideo2110.numRxVideoChannels; i++)
    {
        QJsonObject obj;
        obj.insert("sfp1Enable",        QJsonValue(QString(GetEnable(rVideo2110.rxVideoCh[i].sfpEnable[0]))));
		obj.insert("sfp1srcPort",       QJsonValue(static_cast<int>(rVideo2110.rxVideoCh[i].sourcePort[0])));
		obj.insert("sfp1DestPort",      QJsonValue(static_cast<int>(rVideo2110.rxVideoCh[i].destPort[0])));
        obj.insert("sfp1srcIPAddress",  QJsonValue(QString(rVideo2110.rxVideoCh[i].sourceIP[0])));
        obj.insert("sfp1DestIPAddress", QJsonValue(QString(rVideo2110.rxVideoCh[i].destIP[0])));

        obj.insert("sfp2Enable",        QJsonValue(QString(GetEnable(rVideo2110.rxVideoCh[i].sfpEnable[1]))));
		obj.insert("sfp2srcPort",       QJsonValue(static_cast<int>(rVideo2110.rxVideoCh[i].sourcePort[1])));
		obj.insert("sfp2DestPort",      QJsonValue(static_cast<int>(rVideo2110.rxVideoCh[i].destPort[1])));
        obj.insert("sfp2srcIPAddress",  QJsonValue(QString(rVideo2110.rxVideoCh[i].sourceIP[1])));
        obj.insert("sfp2DestIPAddress", QJsonValue(QString(rVideo2110.rxVideoCh[i].destIP[1])));

		obj.insert("vlan",              QJsonValue(static_cast<int>(rVideo2110.rxVideoCh[i].vlan)));
		obj.insert("ssrc",              QJsonValue(static_cast<int>(rVideo2110.rxVideoCh[i].ssrc)));
		obj.insert("payloadType",       QJsonValue(static_cast<int>(rVideo2110.rxVideoCh[i].payloadType)));
        obj.insert("enable",            QJsonValue(QString(GetEnable(rVideo2110.rxVideoCh[i].enable))));
        obj.insert("stream",            QJsonValue(QString(GetVideoStream(rVideo2110.rxVideoCh[i].stream))));

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
        if (m_verbose) std::cout << " sfp1srcPort " << rAudio2110.rxAudioCh[i].sourcePort[0] << std::endl;
        rAudio2110.rxAudioCh[i].destPort[0]     = vObj["sfp1DestPort"].toInt();
        if (m_verbose) std::cout << " sfp1DestPort " << rAudio2110.rxAudioCh[i].destPort[0] << std::endl;
        str = vObj["sfp1srcIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1srcIPAddress " << str.c_str() << std::endl;
        strncpy(rAudio2110.rxAudioCh[i].sourceIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1DestIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1DestIPAddress " << str.c_str() << std::endl;
        strncpy(rAudio2110.rxAudioCh[i].destIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1Enable"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1Enable " << str.c_str() << std::endl;
        rAudio2110.rxAudioCh[i].sfpEnable[0] = GetEnable(str);

        rAudio2110.rxAudioCh[i].sourcePort[1]	= vObj["sfp2srcPort"].toInt();
        if (m_verbose) std::cout << " sfp2srcPort " << rAudio2110.rxAudioCh[i].sourcePort[1] << std::endl;
        rAudio2110.rxAudioCh[i].destPort[1]     = vObj["sfp2DestPort"].toInt();
        if (m_verbose) std::cout << " sfp2DestPort " << rAudio2110.rxAudioCh[i].destPort[1] << std::endl;
        str = vObj["sfp2srcIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2srcIPAddress " << str.c_str() << std::endl;
        strncpy(rAudio2110.rxAudioCh[i].sourceIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2DestIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2DestIPAddress " << str.c_str() << std::endl;
        strncpy(rAudio2110.rxAudioCh[i].destIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2Enable"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2Enable " << str.c_str() << std::endl;
        rAudio2110.rxAudioCh[i].sfpEnable[1] = GetEnable(str);

        rAudio2110.rxAudioCh[i].vlan                = vObj["vlan"].toInt();
        if (m_verbose) std::cout << " vlan " << rAudio2110.rxAudioCh[i].vlan << std::endl;
        rAudio2110.rxAudioCh[i].ssrc                = vObj["ssrc"].toInt();
        if (m_verbose) std::cout << " ssrc " << rAudio2110.rxAudioCh[i].ssrc << std::endl;
        rAudio2110.rxAudioCh[i].payloadType         = vObj["payloadType"].toInt();
        if (m_verbose) std::cout << " payloadType " << rAudio2110.rxAudioCh[i].payloadType << std::endl;
        rAudio2110.rxAudioCh[i].numAudioChannels    = vObj["numAudioChannels"].toInt();
        if (m_verbose) std::cout << " numAudioChannels " << rAudio2110.rxAudioCh[i].numAudioChannels << std::endl;

        str = vObj["audioPktInterval"].toString().toStdString();
        if (m_verbose) std::cout << " audioPktInterval " << str.c_str() << std::endl;
        rAudio2110.rxAudioCh[i].audioPktInterval = GetAudioPktInterval(str);
        str = vObj["enable"].toString().toStdString();
        if (m_verbose) std::cout << " enable " << str.c_str() << std::endl;
        rAudio2110.rxAudioCh[i].enable = GetEnable(str);
        str = vObj["designator"].toString().toStdString();
        if (m_verbose) std::cout << " designator " << str.c_str() << std::endl << std::endl;
        rAudio2110.rxAudioCh[i].channel = GetChannel(str);
        str = vObj["stream"].toString().toStdString();
        if (m_verbose) std::cout << " stream " << str.c_str() << std::endl;
        rAudio2110.rxAudioCh[i].stream = GetAudioStream(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonReceiveAudio(const ReceiveAudioData2110& rAudio2110, QJsonArray& aArray)
{
    for (uint32_t i=0; i<rAudio2110.numRxAudioChannels; i++)
    {
        QJsonObject obj;
        obj.insert("sfp1Enable",            QJsonValue(QString(GetEnable(rAudio2110.rxAudioCh[i].sfpEnable[0]))));
		obj.insert("sfp1srcPort",           QJsonValue(static_cast<int>(rAudio2110.rxAudioCh[i].sourcePort[0])));
		obj.insert("sfp1DestPort",          QJsonValue(static_cast<int>(rAudio2110.rxAudioCh[i].destPort[0])));
        obj.insert("sfp1srcIPAddress",      QJsonValue(QString(rAudio2110.rxAudioCh[i].sourceIP[0])));
        obj.insert("sfp1DestIPAddress",     QJsonValue(QString(rAudio2110.rxAudioCh[i].destIP[0])));

        obj.insert("sfp2Enable",            QJsonValue(QString(GetEnable(rAudio2110.rxAudioCh[i].sfpEnable[1]))));
		obj.insert("sfp2srcPort",           QJsonValue(static_cast<int>(rAudio2110.rxAudioCh[i].sourcePort[1])));
		obj.insert("sfp2DestPort",          QJsonValue(static_cast<int>(rAudio2110.rxAudioCh[i].destPort[1])));
        obj.insert("sfp2srcIPAddress",      QJsonValue(QString(rAudio2110.rxAudioCh[i].sourceIP[1])));
        obj.insert("sfp2DestIPAddress",     QJsonValue(QString(rAudio2110.rxAudioCh[i].destIP[1])));

		obj.insert("vlan",                  QJsonValue(static_cast<int>(rAudio2110.rxAudioCh[i].vlan)));
		obj.insert("ssrc",                  QJsonValue(static_cast<int>(rAudio2110.rxAudioCh[i].ssrc)));
		obj.insert("payloadType",           QJsonValue(static_cast<int>(rAudio2110.rxAudioCh[i].payloadType)));
		obj.insert("numAudioChannels",      QJsonValue(static_cast<int>(rAudio2110.rxAudioCh[i].numAudioChannels)));

        obj.insert("audioPktInterval",      QJsonValue(QString(GetAudioPktInterval(rAudio2110.rxAudioCh[i].audioPktInterval))));
        obj.insert("enable",                QJsonValue(QString(GetEnable(rAudio2110.rxAudioCh[i].enable))));
        obj.insert("designator",            QJsonValue(QString(GetChannel(rAudio2110.rxAudioCh[i].channel))));
        obj.insert("stream",                QJsonValue(QString(GetAudioStream(rAudio2110.rxAudioCh[i].stream))));

        aArray += QJsonValue(obj);
    }

    return true;
}

bool CKonaIpJsonParse2110::JsonToStructReceiveAnc(const QJsonArray& aArray, ReceiveAncData2110& rAnc2110)
{
	memset(&rAnc2110, 0, sizeof(ReceiveAncData2110));

	std::cout << "ReceiveAnc2110" << std::endl;

	// up to 4 channels
	rAnc2110.numRxAncChannels = MinVal(aArray.count(), 4);
	if (rAnc2110.numRxAncChannels == 0)
		return false;

	std::string str;

	for (uint32_t i=0; i<rAnc2110.numRxAncChannels; i++)
	{
		QJsonObject vObj = aArray[i].toObject();

		rAnc2110.rxAncCh[i].sourcePort[0]		= vObj["sfp1srcPort"].toInt();
		if (m_verbose) std::cout << " sfp1srcPort " << rAnc2110.rxAncCh[i].sourcePort[0] << std::endl;
		rAnc2110.rxAncCh[i].destPort[0]			= vObj["sfp1DestPort"].toInt();
		if (m_verbose) std::cout << " sfp1DestPort " << rAnc2110.rxAncCh[i].destPort[0] << std::endl;
		str = vObj["sfp1srcIPAddress"].toString().toStdString();
		if (m_verbose) std::cout << " sfp1srcIPAddress " << str.c_str() << std::endl;
		strncpy(rAnc2110.rxAncCh[i].sourceIP[0], str.c_str(), kStrMax);
		str = vObj["sfp1DestIPAddress"].toString().toStdString();
		if (m_verbose) std::cout << " sfp1DestIPAddress " << str.c_str() << std::endl;
		strncpy(rAnc2110.rxAncCh[i].destIP[0], str.c_str(), kStrMax);
		str = vObj["sfp1Enable"].toString().toStdString();
		if (m_verbose) std::cout << " sfp1Enable " << str.c_str() << std::endl;
		rAnc2110.rxAncCh[i].sfpEnable[0] = GetEnable(str);

		rAnc2110.rxAncCh[i].sourcePort[1]		= vObj["sfp2srcPort"].toInt();
		if (m_verbose) std::cout << " sfp2srcPort " << rAnc2110.rxAncCh[i].sourcePort[1] << std::endl;
		rAnc2110.rxAncCh[i].destPort[1]			= vObj["sfp2DestPort"].toInt();
		if (m_verbose) std::cout << " sfp2DestPort " << rAnc2110.rxAncCh[i].destPort[1] << std::endl;
		str = vObj["sfp2srcIPAddress"].toString().toStdString();
		if (m_verbose) std::cout << " sfp2srcIPAddress " << str.c_str() << std::endl;
		strncpy(rAnc2110.rxAncCh[i].sourceIP[1], str.c_str(), kStrMax);
		str = vObj["sfp2DestIPAddress"].toString().toStdString();
		if (m_verbose) std::cout << " sfp2DestIPAddress " << str.c_str() << std::endl;
		strncpy(rAnc2110.rxAncCh[i].destIP[1], str.c_str(), kStrMax);
		str = vObj["sfp2Enable"].toString().toStdString();
		if (m_verbose) std::cout << " sfp2Enable " << str.c_str() << std::endl;
		rAnc2110.rxAncCh[i].sfpEnable[1] = GetEnable(str);

		rAnc2110.rxAncCh[i].vlan					= vObj["vlan"].toInt();
		if (m_verbose) std::cout << " vlan " << rAnc2110.rxAncCh[i].vlan << std::endl;
		rAnc2110.rxAncCh[i].ssrc					= vObj["ssrc"].toInt();
		if (m_verbose) std::cout << " ssrc " << rAnc2110.rxAncCh[i].ssrc << std::endl;
		rAnc2110.rxAncCh[i].payloadType				= vObj["payloadType"].toInt();
		if (m_verbose) std::cout << " payloadType " << rAnc2110.rxAncCh[i].payloadType << std::endl;

		str = vObj["enable"].toString().toStdString();
		if (m_verbose) std::cout << " enable " << str.c_str() << std::endl;
		rAnc2110.rxAncCh[i].enable = GetEnable(str);
		str = vObj["stream"].toString().toStdString();
		if (m_verbose) std::cout << " stream " << str.c_str() << std::endl;
		rAnc2110.rxAncCh[i].stream = GetAncStream(str);
	}

	return true;
}

bool CKonaIpJsonParse2110::StructToJsonReceiveAnc(const ReceiveAncData2110& rAnc2110, QJsonArray& aArray)
{
	for (uint32_t i=0; i<rAnc2110.numRxAncChannels; i++)
	{
		QJsonObject obj;
		obj.insert("sfp1Enable",            QJsonValue(QString(GetEnable(rAnc2110.rxAncCh[i].sfpEnable[0]))));
		obj.insert("sfp1srcPort",           QJsonValue(static_cast<int>(rAnc2110.rxAncCh[i].sourcePort[0])));
		obj.insert("sfp1DestPort",          QJsonValue(static_cast<int>(rAnc2110.rxAncCh[i].destPort[0])));
		obj.insert("sfp1srcIPAddress",      QJsonValue(QString(rAnc2110.rxAncCh[i].sourceIP[0])));
		obj.insert("sfp1DestIPAddress",     QJsonValue(QString(rAnc2110.rxAncCh[i].destIP[0])));

		obj.insert("sfp2Enable",            QJsonValue(QString(GetEnable(rAnc2110.rxAncCh[i].sfpEnable[1]))));
		obj.insert("sfp2srcPort",           QJsonValue(static_cast<int>(rAnc2110.rxAncCh[i].sourcePort[1])));
		obj.insert("sfp2DestPort",          QJsonValue(static_cast<int>(rAnc2110.rxAncCh[i].destPort[1])));
		obj.insert("sfp2srcIPAddress",      QJsonValue(QString(rAnc2110.rxAncCh[i].sourceIP[1])));
		obj.insert("sfp2DestIPAddress",     QJsonValue(QString(rAnc2110.rxAncCh[i].destIP[1])));

		obj.insert("vlan",                  QJsonValue(static_cast<int>(rAnc2110.rxAncCh[i].vlan)));
		obj.insert("ssrc",                  QJsonValue(static_cast<int>(rAnc2110.rxAncCh[i].ssrc)));
		obj.insert("payloadType",           QJsonValue(static_cast<int>(rAnc2110.rxAncCh[i].payloadType)));

		obj.insert("enable",                QJsonValue(QString(GetEnable(rAnc2110.rxAncCh[i].enable))));
		obj.insert("stream",                QJsonValue(QString(GetAncStream(rAnc2110.rxAncCh[i].stream))));

		aArray += QJsonValue(obj);
	}

	return true;
}

bool CKonaIpJsonParse2110::JsonToStructTransmitVideo(const QJsonArray& vArray, TransmitVideoData2110& tVideo2110)
{
    memset(&tVideo2110, 0, sizeof(TransmitVideoData2110));

    std::cout << "TransmitVideo2110" << std::endl;

    // up to 4 channels
    tVideo2110.numTxVideoChannels = MinVal(vArray.count(), 4);
    if (tVideo2110.numTxVideoChannels == 0)
        return false;

    std::string str;

    for (uint32_t i=0; i<tVideo2110.numTxVideoChannels; i++)
    {
        QJsonObject vObj = vArray[i].toObject();

        tVideo2110.txVideoCh[i].localPort[0]	= vObj["sfp1LocalPort"].toInt();
        if (m_verbose) std::cout << " sfp1LocalPort " << tVideo2110.txVideoCh[i].localPort[0] << std::endl;
        tVideo2110.txVideoCh[i].remotePort[0]   = vObj["sfp1RemotePort"].toInt();
        if (m_verbose) std::cout << " sfp1RemotePort " << tVideo2110.txVideoCh[i].remotePort[0] << std::endl;
        str = vObj["sfp1RemoteIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1RemoteIPAddress " << str.c_str() << std::endl;
        strncpy(tVideo2110.txVideoCh[i].remoteIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1Enable"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1Enable " << str.c_str() << std::endl;
        tVideo2110.txVideoCh[i].sfpEnable[0] = GetEnable(str);

        tVideo2110.txVideoCh[i].localPort[1]	= vObj["sfp2LocalPort"].toInt();
        if (m_verbose) std::cout << " sfp2LocalPort " << tVideo2110.txVideoCh[i].localPort[1] << std::endl;
        tVideo2110.txVideoCh[i].remotePort[1]   = vObj["sfp2RemotePort"].toInt();
        if (m_verbose) std::cout << " sfp2RemotePort " << tVideo2110.txVideoCh[i].remotePort[1] << std::endl;
        str = vObj["sfp2RemoteIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2RemoteIPAddress " << str.c_str() << std::endl;
        strncpy(tVideo2110.txVideoCh[i].remoteIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2Enable"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2Enable " << str.c_str() << std::endl;
        tVideo2110.txVideoCh[i].sfpEnable[1] = GetEnable(str);

        tVideo2110.txVideoCh[i].ttl             = vObj["ttl"].toInt();
        if (m_verbose) std::cout << " ttl " << tVideo2110.txVideoCh[i].ttl << std::endl;
        tVideo2110.txVideoCh[i].ssrc            = vObj["ssrc"].toInt();
        if (m_verbose) std::cout << " ssrc " << tVideo2110.txVideoCh[i].ssrc << std::endl;
        tVideo2110.txVideoCh[i].payloadType     = vObj["payloadType"].toInt();
        if (m_verbose) std::cout << " payloadType " << tVideo2110.txVideoCh[i].payloadType << std::endl;

		str = vObj["sampling"].toString().toStdString();
		if (m_verbose) std::cout << " sampling " << str.c_str() << std::endl;
		tVideo2110.txVideoCh[i].sampling = GetSampling(str);

        str = vObj["videoFormat"].toString().toStdString();
        if (m_verbose) std::cout << " videoFormat " << str.c_str() << std::endl;
#ifdef BUILD_DEMO
        tVideo2110.txVideoCh[i].videoFormat = CNTV2DemoCommon::GetVideoFormatFromString(str, VIDEO_FORMATS_ALL);
#else
        tVideo2110.txVideoCh[i].videoFormat = NTV2_FORMAT_UNKNOWN;
#endif
        str = vObj["enable"].toString().toStdString();
        if (m_verbose) std::cout << " enable " << str.c_str() << std::endl;
        tVideo2110.txVideoCh[i].enable = GetEnable(str);
        str = vObj["stream"].toString().toStdString();
        if (m_verbose) std::cout << " stream " << str.c_str() << std::endl << std::endl;
        tVideo2110.txVideoCh[i].stream = GetVideoStream(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonTransmitVideo(const TransmitVideoData2110& tVideo2110, QJsonArray& vArray)
{
    for (uint32_t i=0; i<tVideo2110.numTxVideoChannels; i++)
    {
        QJsonObject obj;
		obj.insert("sfp1LocalPort",         QJsonValue(static_cast<int>(tVideo2110.txVideoCh[i].localPort[0])));
        obj.insert("sfp1RemoteIPAddress",   QJsonValue(QString(tVideo2110.txVideoCh[i].remoteIP[0])));
		obj.insert("sfp1RemotePort",        QJsonValue(static_cast<int>(tVideo2110.txVideoCh[i].remotePort[0])));
        obj.insert("sfp1Enable",            QJsonValue(QString(GetEnable(tVideo2110.txVideoCh[i].sfpEnable[0]))));

		obj.insert("sfp2LocalPort",         QJsonValue(static_cast<int>(tVideo2110.txVideoCh[i].localPort[1])));
        obj.insert("sfp2RemoteIPAddress",   QJsonValue(QString(tVideo2110.txVideoCh[i].remoteIP[1])));
		obj.insert("sfp2RemotePort",        QJsonValue(static_cast<int>(tVideo2110.txVideoCh[i].remotePort[1])));
        obj.insert("sfp2Enable",            QJsonValue(QString(GetEnable(tVideo2110.txVideoCh[i].sfpEnable[1]))));

		obj.insert("ttl",                   QJsonValue(static_cast<int>(tVideo2110.txVideoCh[i].ttl)));
		obj.insert("ssrc",                  QJsonValue(static_cast<int>(tVideo2110.txVideoCh[i].ssrc)));
		obj.insert("payloadType",           QJsonValue(static_cast<int>(tVideo2110.txVideoCh[i].payloadType)));
		obj.insert("sampling",				QJsonValue(QString(GetSampling(tVideo2110.txVideoCh[i].sampling))));

        obj.insert("enable",                QJsonValue(QString(GetEnable(tVideo2110.txVideoCh[i].enable))));
        obj.insert("stream",                QJsonValue(QString(GetVideoStream(tVideo2110.txVideoCh[i].stream))));

        vArray += QJsonValue(obj);
    }

    return true;
}

bool CKonaIpJsonParse2110::JsonToStructTransmitAudio(const QJsonArray& aArray, TransmitAudioData2110& tAudio2110)
{
    memset(&tAudio2110, 0, sizeof(TransmitAudioData2110));

    std::cout << "TransmitAudio2110" << std::endl;

    // up to 4 channels
    tAudio2110.numTxAudioChannels = MinVal(aArray.count(), 4);
    if (tAudio2110.numTxAudioChannels == 0)
        return false;

    std::string str;

    for (uint32_t i=0; i<tAudio2110.numTxAudioChannels; i++)
    {
        QJsonObject vObj = aArray[i].toObject();

        tAudio2110.txAudioCh[i].localPort[0]	= vObj["sfp1LocalPort"].toInt();
        if (m_verbose) std::cout << " sfp1LocalPort " << tAudio2110.txAudioCh[i].localPort[0] << std::endl;
        tAudio2110.txAudioCh[i].remotePort[0]   = vObj["sfp1RemotePort"].toInt();
        if (m_verbose) std::cout << " sfp1RemotePort " << tAudio2110.txAudioCh[i].remotePort[0] << std::endl;
        str = vObj["sfp1RemoteIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1RemoteIPAddress " << str.c_str() << std::endl;
        strncpy(tAudio2110.txAudioCh[i].remoteIP[0], str.c_str(), kStrMax);
        str = vObj["sfp1Enable"].toString().toStdString();
        if (m_verbose) std::cout << " sfp1Enable " << str.c_str() << std::endl;
        tAudio2110.txAudioCh[i].sfpEnable[0] = GetEnable(str);

        tAudio2110.txAudioCh[i].localPort[1]	= vObj["sfp2LocalPort"].toInt();
        if (m_verbose) std::cout << " sfp2LocalPort " << tAudio2110.txAudioCh[i].localPort[1] << std::endl;
        tAudio2110.txAudioCh[i].remotePort[1]   = vObj["sfp2RemotePort"].toInt();
        if (m_verbose) std::cout << " sfp2RemotePort " << tAudio2110.txAudioCh[i].remotePort[1] << std::endl;
        str = vObj["sfp2RemoteIPAddress"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2RemoteIPAddress " << str.c_str() << std::endl;
        strncpy(tAudio2110.txAudioCh[i].remoteIP[1], str.c_str(), kStrMax);
        str = vObj["sfp2Enable"].toString().toStdString();
        if (m_verbose) std::cout << " sfp2Enable " << str.c_str() << std::endl;
        tAudio2110.txAudioCh[i].sfpEnable[1] = GetEnable(str);

        tAudio2110.txAudioCh[i].ttl                 = vObj["ttl"].toInt();
        if (m_verbose) std::cout << " ttl " << tAudio2110.txAudioCh[i].ttl << std::endl;
        tAudio2110.txAudioCh[i].ssrc                = vObj["ssrc"].toInt();
        if (m_verbose) std::cout << " ssrc " << tAudio2110.txAudioCh[i].ssrc << std::endl;
        tAudio2110.txAudioCh[i].payloadType         = vObj["payloadType"].toInt();
        if (m_verbose) std::cout << " payloadType " << tAudio2110.txAudioCh[i].payloadType << std::endl;
        tAudio2110.txAudioCh[i].numAudioChannels    = vObj["numAudioChannels"].toInt();
        if (m_verbose) std::cout << " numAudioChannels " << tAudio2110.txAudioCh[i].numAudioChannels << std::endl;
        tAudio2110.txAudioCh[i].firstAudioChannel   = vObj["firstAudioChannel"].toInt();
        if (m_verbose) std::cout << " firstAudioChannel " << tAudio2110.txAudioCh[i].firstAudioChannel << std::endl;

        str = vObj["audioPktInterval"].toString().toStdString();
        if (m_verbose) std::cout << " audioPktInterval " << str.c_str() << std::endl;
        tAudio2110.txAudioCh[i].audioPktInterval = GetAudioPktInterval(str);
        str = vObj["enable"].toString().toStdString();
        if (m_verbose) std::cout << " enable " << str.c_str() << std::endl;
        tAudio2110.txAudioCh[i].enable = GetEnable(str);
        str = vObj["stream"].toString().toStdString();
        if (m_verbose) std::cout << " stream " << str.c_str() << std::endl;
        tAudio2110.txAudioCh[i].stream = GetAudioStream(str);
        str = vObj["designator"].toString().toStdString();
        if (m_verbose) std::cout << " designator " << str.c_str() << std::endl << std::endl;
        tAudio2110.txAudioCh[i].channel = GetChannel(str);
    }

    return true;
}

bool CKonaIpJsonParse2110::StructToJsonTransmitAudio(const TransmitAudioData2110& tAudio2110, QJsonArray& aArray)
{
    for (uint32_t i=0; i<tAudio2110.numTxAudioChannels; i++)
    {
        QJsonObject obj;
		obj.insert("sfp1LocalPort",         QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].localPort[0])));
        obj.insert("sfp1RemoteIPAddress",   QJsonValue(QString(tAudio2110.txAudioCh[i].remoteIP[0])));
		obj.insert("sfp1RemotePort",        QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].remotePort[0])));
        obj.insert("sfp1Enable",            QJsonValue(QString(GetEnable(tAudio2110.txAudioCh[i].sfpEnable[0]))));

		obj.insert("sfp2LocalPort",         QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].localPort[1])));
        obj.insert("sfp2RemoteIPAddress",   QJsonValue(QString(tAudio2110.txAudioCh[i].remoteIP[1])));
		obj.insert("sfp2RemotePort",        QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].remotePort[1])));
        obj.insert("sfp2Enable",            QJsonValue(QString(GetEnable(tAudio2110.txAudioCh[i].sfpEnable[1]))));

		obj.insert("ttl",                   QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].ttl)));
		obj.insert("ssrc",                  QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].ssrc)));
		obj.insert("payloadType",           QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].payloadType)));
		obj.insert("numAudioChannels",      QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].numAudioChannels)));
		obj.insert("firstAudioChannel",     QJsonValue(static_cast<int>(tAudio2110.txAudioCh[i].firstAudioChannel)));

        obj.insert("audioPktInterval",      QJsonValue(QString(GetAudioPktInterval(tAudio2110.txAudioCh[i].audioPktInterval))));
        obj.insert("enable",                QJsonValue(QString(GetEnable(tAudio2110.txAudioCh[i].enable))));
        obj.insert("stream",                QJsonValue(QString(GetAudioStream(tAudio2110.txAudioCh[i].stream))));
        obj.insert("designator",            QJsonValue(QString(GetChannel(tAudio2110.txAudioCh[i].channel))));
        aArray += QJsonValue(obj);
    }

    return true;
}

bool CKonaIpJsonParse2110::JsonToStructTransmitAnc(const QJsonArray& aArray, TransmitAncData2110& tAnc2110)
{
	memset(&tAnc2110, 0, sizeof(TransmitAncData2110));

	std::cout << "TransmitAnc2110" << std::endl;

	// up to 4 channels
	tAnc2110.numTxAncChannels = MinVal(aArray.count(), 4);
	if (tAnc2110.numTxAncChannels == 0)
		return false;

	std::string str;

	for (uint32_t i=0; i<tAnc2110.numTxAncChannels; i++)
	{
		QJsonObject vObj = aArray[i].toObject();

		tAnc2110.txAncCh[i].localPort[0]		= vObj["sfp1LocalPort"].toInt();
		if (m_verbose) std::cout << " sfp1LocalPort " << tAnc2110.txAncCh[i].localPort[0] << std::endl;
		tAnc2110.txAncCh[i].remotePort[0]		= vObj["sfp1RemotePort"].toInt();
		if (m_verbose) std::cout << " sfp1RemotePort " << tAnc2110.txAncCh[i].remotePort[0] << std::endl;
		str = vObj["sfp1RemoteIPAddress"].toString().toStdString();
		if (m_verbose) std::cout << " sfp1RemoteIPAddress " << str.c_str() << std::endl;
		strncpy(tAnc2110.txAncCh[i].remoteIP[0], str.c_str(), kStrMax);
		str = vObj["sfp1Enable"].toString().toStdString();
		if (m_verbose) std::cout << " sfp1Enable " << str.c_str() << std::endl;
		tAnc2110.txAncCh[i].sfpEnable[0] = GetEnable(str);

		tAnc2110.txAncCh[i].localPort[1]		= vObj["sfp2LocalPort"].toInt();
		if (m_verbose) std::cout << " sfp2LocalPort " << tAnc2110.txAncCh[i].localPort[1] << std::endl;
		tAnc2110.txAncCh[i].remotePort[1]		= vObj["sfp2RemotePort"].toInt();
		if (m_verbose) std::cout << " sfp2RemotePort " << tAnc2110.txAncCh[i].remotePort[1] << std::endl;
		str = vObj["sfp2RemoteIPAddress"].toString().toStdString();
		if (m_verbose) std::cout << " sfp2RemoteIPAddress " << str.c_str() << std::endl;
		strncpy(tAnc2110.txAncCh[i].remoteIP[1], str.c_str(), kStrMax);
		str = vObj["sfp2Enable"].toString().toStdString();
		if (m_verbose) std::cout << " sfp2Enable " << str.c_str() << std::endl;
		tAnc2110.txAncCh[i].sfpEnable[1] = GetEnable(str);

		tAnc2110.txAncCh[i].ttl						= vObj["ttl"].toInt();
		if (m_verbose) std::cout << " ttl " << tAnc2110.txAncCh[i].ttl << std::endl;
		tAnc2110.txAncCh[i].ssrc					= vObj["ssrc"].toInt();
		if (m_verbose) std::cout << " ssrc " << tAnc2110.txAncCh[i].ssrc << std::endl;
		tAnc2110.txAncCh[i].payloadType				= vObj["payloadType"].toInt();
		if (m_verbose) std::cout << " payloadType " << tAnc2110.txAncCh[i].payloadType << std::endl;

		str = vObj["enable"].toString().toStdString();
		if (m_verbose) std::cout << " enable " << str.c_str() << std::endl;
		tAnc2110.txAncCh[i].enable = GetEnable(str);
		str = vObj["stream"].toString().toStdString();
		if (m_verbose) std::cout << " stream " << str.c_str() << std::endl;
		tAnc2110.txAncCh[i].stream = GetAncStream(str);
	}

	return true;
}

bool CKonaIpJsonParse2110::StructToJsonTransmitAnc(const TransmitAncData2110& tAnc2110, QJsonArray& aArray)
{
	for (uint32_t i=0; i<tAnc2110.numTxAncChannels; i++)
	{
		QJsonObject obj;
		obj.insert("sfp1LocalPort",         QJsonValue(static_cast<int>(tAnc2110.txAncCh[i].localPort[0])));
		obj.insert("sfp1RemoteIPAddress",   QJsonValue(QString(tAnc2110.txAncCh[i].remoteIP[0])));
		obj.insert("sfp1RemotePort",        QJsonValue(static_cast<int>(tAnc2110.txAncCh[i].remotePort[0])));
		obj.insert("sfp1Enable",            QJsonValue(QString(GetEnable(tAnc2110.txAncCh[i].sfpEnable[0]))));

		obj.insert("sfp2LocalPort",         QJsonValue(static_cast<int>(tAnc2110.txAncCh[i].localPort[1])));
		obj.insert("sfp2RemoteIPAddress",   QJsonValue(QString(tAnc2110.txAncCh[i].remoteIP[1])));
		obj.insert("sfp2RemotePort",        QJsonValue(static_cast<int>(tAnc2110.txAncCh[i].remotePort[1])));
		obj.insert("sfp2Enable",            QJsonValue(QString(GetEnable(tAnc2110.txAncCh[i].sfpEnable[1]))));

		obj.insert("ttl",                   QJsonValue(static_cast<int>(tAnc2110.txAncCh[i].ttl)));
		obj.insert("ssrc",                  QJsonValue(static_cast<int>(tAnc2110.txAncCh[i].ssrc)));
		obj.insert("payloadType",           QJsonValue(static_cast<int>(tAnc2110.txAncCh[i].payloadType)));

		obj.insert("enable",                QJsonValue(QString(GetEnable(tAnc2110.txAncCh[i].enable))));
		obj.insert("stream",                QJsonValue(QString(GetAncStream(tAnc2110.txAncCh[i].stream))));
		aArray += QJsonValue(obj);
	}

	return true;
}

bool CKonaIpJsonParse2110::StructToJson(const NetworkData2110& net2110,
										const ReceiveVideoData2110& vidRec2110,
										const ReceiveAudioData2110& audRec2110,
										const ReceiveAncData2110& ancRec2110,
										const TransmitVideoData2110& vidTran2110,
										const TransmitAudioData2110& audTran2110,
										const TransmitAncData2110& ancTran2110,
										QJsonObject& topObj)
{
	QJsonObject netJson;
	memcpy(&m_net2110, &net2110, sizeof(NetworkData2110));
	StructToJsonNetwork(m_net2110, netJson);
	m_netJson = netJson;

	QJsonArray vidRecJson;
	memcpy(&m_receiveVideo2110, &vidRec2110, sizeof(ReceiveVideoData2110));
	StructToJsonReceiveVideo(m_receiveVideo2110, vidRecJson);
	m_receiveVideoJson = vidRecJson;
	QJsonArray audRecJson;
	memcpy(&m_receiveAudio2110, &audRec2110, sizeof(ReceiveAudioData2110));
	StructToJsonReceiveAudio(m_receiveAudio2110, audRecJson);
	m_receiveAudioJson = audRecJson;
	QJsonArray ancRecJson;
	memcpy(&m_receiveAnc2110, &ancRec2110, sizeof(ReceiveAncData2110));
	StructToJsonReceiveAnc(m_receiveAnc2110, ancRecJson);
	m_receiveAncJson = ancRecJson;

	QJsonArray vidTransJson;
	memcpy(&m_transmitVideo2110, &vidTran2110, sizeof(TransmitVideoData2110));
	StructToJsonTransmitVideo(m_transmitVideo2110, vidTransJson);
	m_transmitVideoJson = vidTransJson;
	QJsonArray audTransJson;
	memcpy(&m_transmitAudio2110, &audTran2110, sizeof(TransmitAudioData2110));
	StructToJsonTransmitAudio(m_transmitAudio2110, audTransJson);
	m_transmitAudioJson = audTransJson;
	QJsonArray ancTransJson;
	memcpy(&m_transmitAnc2110, &ancTran2110, sizeof(TransmitAncData2110));
	StructToJsonTransmitAnc(m_transmitAnc2110, ancTransJson);
	m_transmitAncJson = ancTransJson;

	topObj.insert("protocol", QJsonValue(QString("2110")));
	topObj.insert("network2110", m_netJson);
	topObj.insert("receiveVideo2110", m_receiveVideoJson);
	topObj.insert("receiveAudio2110", m_receiveAudioJson);
	topObj.insert("receiveAnc2110", m_receiveAncJson);
	topObj.insert("transmitVideo2110", m_transmitVideoJson);
	topObj.insert("transmitAudio2110", m_transmitAudioJson);
	topObj.insert("transmitAnc2110", m_transmitAncJson);

	return true;
}
