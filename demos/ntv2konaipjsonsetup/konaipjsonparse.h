/* SPDX-License-Identifier: MIT */
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
#define AbsVal(X)   ((X) < 0 ?  (-X) : (X))

#define  BUILD_DEMO 1

//-------------------------------------------------------------------------------------------------
//	CKonaIpJsonParse2110
//-------------------------------------------------------------------------------------------------
class CKonaIpJsonParse2110
{
public:
    CKonaIpJsonParse2110();
    ~CKonaIpJsonParse2110();

    bool SetJson(const QJsonObject& topObj, bool verbose);
	bool SetJsonProtocol(const QJsonObject& obj);
	bool SetJsonNetwork(const QJsonObject& obj);

    bool SetJsonReceiveVideo(const QJsonArray& jsonArray);
    bool SetJsonReceiveAudio(const QJsonArray& jsonArray);
	bool SetJsonReceiveAnc(const QJsonArray& jsonArray);

    bool SetJsonTransmitVideo(const QJsonArray& jsonArray);
    bool SetJsonTransmitAudio(const QJsonArray& jsonArray);
	bool SetJsonTransmitAnc(const QJsonArray& jsonArray);

	bool JsonToStructNetwork(const QJsonObject& topObj, NetworkData2110& n2110);
    bool StructToJsonNetwork(const NetworkData2110& n2110, QJsonObject& topObj);

    bool JsonToStructReceiveVideo(const QJsonArray& vArray, ReceiveVideoData2110& n2110);
    bool StructToJsonReceiveVideo(const ReceiveVideoData2110& n2110, QJsonArray& topObj);
    bool JsonToStructReceiveAudio(const QJsonArray& aArray, ReceiveAudioData2110& n2110);
    bool StructToJsonReceiveAudio(const ReceiveAudioData2110& n2110, QJsonArray& aArray);
	bool JsonToStructReceiveAnc(const QJsonArray& aArray, ReceiveAncData2110& n2110);
	bool StructToJsonReceiveAnc(const ReceiveAncData2110& n2110, QJsonArray& aArray);

    bool JsonToStructTransmitVideo(const QJsonArray& vArray, TransmitVideoData2110& n2110);
    bool StructToJsonTransmitVideo(const TransmitVideoData2110& n2110, QJsonArray& vArray);
    bool JsonToStructTransmitAudio(const QJsonArray& aArray, TransmitAudioData2110& n2110);
    bool StructToJsonTransmitAudio(const TransmitAudioData2110& n2110, QJsonArray& aArray);
	bool JsonToStructTransmitAnc(const QJsonArray& aArray, TransmitAncData2110& n2110);
	bool StructToJsonTransmitAnc(const TransmitAncData2110& n2110, QJsonArray& aArray);

	bool StructToJson(	const NetworkData2110& net2110,
						const ReceiveVideoData2110& vidRec2110,
						const ReceiveAudioData2110& audRec2110,
						const ReceiveAncData2110& ancRec2110,
						const TransmitVideoData2110& vidTran2110,
						const TransmitAudioData2110& audTran2110,
						const TransmitAncData2110& ancTran2110,
						QJsonObject& topObj);

    bool GetEnable(const std::string enableBoolString);
    QString GetEnable(const bool enabled);
	VPIDSampling GetSampling(const std::string samplingString);
	QString GetSampling(const VPIDSampling sampling);
    NTV2Channel GetChannel(const std::string channelString);
    QString GetChannel(const NTV2Channel channel);
    eSFP GetSfp(const std::string sfpString);
    QString GetSfp(const eSFP sfp);
	NTV2Stream GetAudioStream(const std::string streamString);
	QString GetAudioStream(const NTV2Stream stream);
	NTV2Stream GetAncStream(const std::string streamString);
	QString GetAncStream(const NTV2Stream stream);
	eNTV2PacketInterval GetAudioPktInterval(const std::string streamString);
    QString GetAudioPktInterval(const eNTV2PacketInterval stream);
    NTV2Stream GetVideoStream(const std::string streamString);
    QString GetVideoStream(const NTV2Stream stream);
    void GetGrandMasterID(const std::string str, uint8_t (&id)[8]);
    QString GetGrandMasterID(const uint8_t id[8]);

public:
    bool                    m_verbose;

    QJsonObject             m_topJson;
	QJsonObject             m_protocolJson;
	QJsonObject             m_netJson;
    QJsonArray              m_receiveVideoJson;
    QJsonArray              m_receiveAudioJson;
	QJsonArray              m_receiveAncJson;
	QJsonArray              m_transmitVideoJson;
	QJsonArray              m_transmitAudioJson;
	QJsonArray              m_transmitAncJson;

    NetworkData2110         m_net2110;
    ReceiveVideoData2110    m_receiveVideo2110;
    ReceiveAudioData2110    m_receiveAudio2110;
	ReceiveAncData2110		m_receiveAnc2110;
    TransmitVideoData2110   m_transmitVideo2110;
    TransmitAudioData2110   m_transmitAudio2110;
	TransmitAncData2110		m_transmitAnc2110;
};


#endif	// IP_JSON_PARSE_H
