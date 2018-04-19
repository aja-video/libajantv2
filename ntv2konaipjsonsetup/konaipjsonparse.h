//-------------------------------------------------------------------------------------------
//	konaipjsonparse.h
//  Implementation of ip json parser for 2110.
//
//	Copyright (C) 2018 AJA Video Systems, Inc.  
//	Proprietary and Confidential information.  All rights reserved.
//-------------------------------------------------------------------------------------------

#ifndef IP_JSON_PARSE_H
#define IP_JSON_PARSE_H

#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include "ntv2config2022.h"
#include "ntv2config2110.h"

#define MaxVal(X,Y) ((X) < (Y) ? (Y) : (X))
#define MinVal(X,Y) ((X) < (Y) ? (X) : (Y))

//-------------------------------------------------------------------------------------------------
//	CKonaIpJsonParse2110
//-------------------------------------------------------------------------------------------------
class CKonaIpJsonParse2110
{
public:
    CKonaIpJsonParse2110();
    ~CKonaIpJsonParse2110();

    bool SetJson(const QJsonObject& topObj, bool verbose);
    bool SetJsonNetwork(const QJsonObject& obj);
    bool SetJsonReceiveVideo(const QJsonArray& jsonArray);
    bool SetJsonReceiveAudio(const QJsonArray& jsonArray);
    bool SetJsonTransmitVideo(const QJsonArray& jsonArray);
    bool SetJsonTransmitAudio(const QJsonArray& jsonArray);

    bool JsonToStructNetwork(const QJsonObject& topObj, NetworkData2110& n2110);
    bool StructToJsonNetwork(const NetworkData2110& n2110, QJsonObject& topObj);

    bool JsonToStructReceiveVideo(const QJsonArray& vArray, ReceiveVideoData2110& n2110);
    bool StructToJsonReceiveVideo(const ReceiveVideoData2110& n2110, QJsonArray& topObj);

    bool JsonToStructReceiveAudio(const QJsonArray& aArray, ReceiveAudioData2110& n2110);
    bool StructToJsonReceiveAudio(const ReceiveAudioData2110& n2110, QJsonArray& aArray);

    bool JsonToStructTransmitVideo(const QJsonArray& vArray, TransmitVideoData2110& n2110);
    bool StructToJsonTransmitVideo(const TransmitVideoData2110& n2110, QJsonArray& vArray);

    bool JsonToStructTransmitAudio(const QJsonArray& aArray, TransmitAudioData2110& n2110);
    bool StructToJsonTransmitAudio(const TransmitAudioData2110& n2110, QJsonArray& aArray);

    bool GetEnable(std::string enableBoolString);
    QString GetEnable(bool enabled);
    NTV2Channel GetChannel(std::string channelString);
    QString GetChannel(NTV2Channel channel);
    eSFP GetSfp(std::string sfpString);
    QString GetSfp(eSFP sfp);
    NTV2Stream GetStream(std::string streamString);
    QString GetStream(NTV2Stream stream);

public:
    bool                    m_verbose;

    QJsonObject             m_topJson;
    QJsonObject             m_netJson;
    QJsonArray              m_receiveVideoJson;
    QJsonArray              m_receiveAudioJson;
    QJsonArray              m_transmitVideoJson;
    QJsonArray              m_transmitAudioJson;

    NetworkData2110         m_net2110;
    ReceiveVideoData2110    m_receiveVideo2110;
    ReceiveAudioData2110    m_receiveAudio2110;
    TransmitVideoData2110   m_transmitVideo2110;
    TransmitAudioData2110   m_transmitAudio2110;
};


#endif	// IP_JSON_PARSE_H
