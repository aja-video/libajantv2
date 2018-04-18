#include "konaipjsonsetup.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2democommon.h"

#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"


using std::endl;
using std::cout;
using std::cerr;

NTV2Channel getChannel(QString channelDesignator);
bool getEnable(QString enableBoolString);

CKonaIpJsonSetup::CKonaIpJsonSetup()
{
    mIs2110       = false;
    mEnable2022_7 = false;
    mNetworkPathDifferential = 50;
}

bool CKonaIpJsonSetup::openJson(QString fileName)
{
    bool result = false;

    QFile loadFile(fileName);
    if ( !loadFile.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open json file.");
        return false;

    }
    QByteArray saveData = loadFile.readAll();

    QJsonParseError err;
    QJsonDocument loadDoc = (QJsonDocument::fromJson(saveData,&err));
    if (err.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON ERROR" << err.errorString() << "offset=" << err.offset;
        saveData[err.offset = 0];
        qDebug() << saveData;
        return false;
    }

    const QJsonObject & json = loadDoc.object();
    QJsonValue qjv = json.value("protocol");
    if (qjv != QJsonValue::Undefined)
    {
        QString protocol = qjv.toString();
        if (protocol == "2110")
        {
            mIs2110 = true;
            cout << "Protocol 2110 " << endl;
        }
        else
            cout << "Protocol 2022 " << endl;
    }

    if (mIs2110)
    {
        qjv = json.value("PTPMaster");
        if (qjv != QJsonValue::Undefined)
        {
            mPTPMasterAddr = qjv.toString();
            cout << "PTP Master Address " << mPTPMasterAddr.toStdString() << endl;
        }
        qjv = json.value("4KMode");
        if (qjv != QJsonValue::Undefined)
        {
            m4KMode = getEnable(qjv.toString());
        }
        else
        {
            m4KMode = false;
        }
        cout << "4K Mode " << m4KMode << endl;

        result = readJson2110(json);
    }
    else
    {
        qjv = json.value("enable2022_7");
        if (qjv != QJsonValue::Undefined)
        {
            mEnable2022_7 = getEnable(qjv.toString());
        }
        else
        {
            mEnable2022_7 = false;
        }
        cout << "2022-7 mode " << mEnable2022_7 << endl;

        qjv = json.value("networkPathDifferential");
        if (qjv != QJsonValue::Undefined)
        {
            mNetworkPathDifferential = qjv.toString().toUInt();
        }
        else
        {
            mNetworkPathDifferential = 50;
        }
        cout << "NetworkPathDifferential " << mNetworkPathDifferential << endl;

        result = readJson2022(json);
    }

    return result;
}


bool CKonaIpJsonSetup::readJson2022(const QJsonObject &json)
{
    mKonaIP2022Params.mSFPs.clear();
    QJsonArray sfpArray = json["sfps"].toArray();
    for (int sfpIndex = 0; sfpIndex < sfpArray.size(); ++sfpIndex)
    {
        cout << "SFP" << endl;

        QJsonObject sfpObject = sfpArray[sfpIndex].toObject();
        SFPStruct sfpStruct;

        sfpStruct.mSfpDesignator = sfpObject["designator"].toString();
        if (!sfpStruct.mSfpDesignator.isEmpty())
            cout << "SFPDesignator " << sfpStruct.mSfpDesignator.toStdString() << endl;

        sfpStruct.mIPAddress = sfpObject["ipAddress"].toString();
        if (!sfpStruct.mIPAddress.isEmpty())
            cout << "IPAddress " << sfpStruct.mIPAddress.toStdString() << endl;

        sfpStruct.mSubnetMask = sfpObject["subnetMask"].toString();
        if (!sfpStruct.mSubnetMask.isEmpty())
            cout << "SubnetMask " << sfpStruct.mSubnetMask.toStdString() << endl;

        sfpStruct.mGateway = sfpObject["gateway"].toString();
        if (!sfpStruct.mGateway.isEmpty())
            cout << "Gateway " << sfpStruct.mGateway.toStdString() << endl;

        sfpStruct.mEnable = sfpObject["enable"].toString();
        if (!sfpStruct.mEnable.isEmpty())
            cout << "Enable " << sfpStruct.mEnable.toStdString() << endl;
        cout << endl;

        mKonaIP2022Params.mSFPs.append(sfpStruct);
    }

    mKonaIP2022Params.mReceive2022Channels.clear();
    QJsonArray receiveChannelArray = json["receive2022"].toArray();
    for (int receiveChannelIndex = 0; receiveChannelIndex < receiveChannelArray.size(); ++receiveChannelIndex)
    {
        cout << "Receive2022Channels" << endl;

        QJsonObject receiveChannelObject = receiveChannelArray[receiveChannelIndex].toObject();
        ReceiveStruct2022 receive2022Struct;

        receive2022Struct.mChannelDesignator = receiveChannelObject["designator"].toString();
        if (!receive2022Struct.mChannelDesignator.isEmpty())
            cout << "ChannelDesignator " << receive2022Struct.mChannelDesignator.toStdString() << endl;

        receive2022Struct.mSfp1SrcIPAddress = receiveChannelObject["sfp1SrcIPAddress"].toString();
        if (!receive2022Struct.mSfp1SrcIPAddress.isEmpty())
            cout << "SFP1SrcIPAddress " << receive2022Struct.mSfp1SrcIPAddress.toStdString() << endl;

        receive2022Struct.mSfp1SrcPort = receiveChannelObject["sfp1SrcPort"].toString();
        if (!receive2022Struct.mSfp1SrcPort.isEmpty())
            cout << "SFP1SrcPort " << receive2022Struct.mSfp1SrcPort.toStdString() << endl;

        receive2022Struct.mSfp1DestIPAddress = receiveChannelObject["sfp1DestIPAddress"].toString();
        if (!receive2022Struct.mSfp1DestIPAddress.isEmpty())
            cout << "SFP1DestIPAddress " << receive2022Struct.mSfp1DestIPAddress.toStdString() << endl;

        receive2022Struct.mSfp1DestPort = receiveChannelObject["sfp1DestPort"].toString();
        if (!receive2022Struct.mSfp1DestPort.isEmpty())
            cout << "SFP1DestPort " << receive2022Struct.mSfp1DestPort.toStdString() << endl;

        receive2022Struct.mSfp1Filter = receiveChannelObject["sfp1Filter"].toString();
        if (!receive2022Struct.mSfp1Filter.isEmpty())
            cout << "SFP1Filter " << receive2022Struct.mSfp1Filter.toStdString() << endl;

        receive2022Struct.mSfp2SrcIPAddress = receiveChannelObject["sfp2SrcIPAddress"].toString();
        if (!receive2022Struct.mSfp2SrcIPAddress.isEmpty())
            cout << "SFP2SrcIPAddress " << receive2022Struct.mSfp2SrcIPAddress.toStdString() << endl;

        receive2022Struct.mSfp2SrcPort = receiveChannelObject["sfp2SrcPort"].toString();
        if (!receive2022Struct.mSfp2SrcPort.isEmpty())
            cout << "SFP2SrcPort " << receive2022Struct.mSfp2SrcPort.toStdString() << endl;

        receive2022Struct.mSfp2DestIPAddress = receiveChannelObject["sfp2DestIPAddress"].toString();
        if (!receive2022Struct.mSfp2DestIPAddress.isEmpty())
            cout << "SFP1DestIPAddress " << receive2022Struct.mSfp2DestIPAddress.toStdString() << endl;

        receive2022Struct.mSfp2DestPort = receiveChannelObject["sfp2DestPort"].toString();
        if (!receive2022Struct.mSfp2DestPort.isEmpty())
            cout << "SFP1DestPort " << receive2022Struct.mSfp2DestPort.toStdString() << endl;

        receive2022Struct.mSfp2Filter = receiveChannelObject["sfp2Filter"].toString();
        if (!receive2022Struct.mSfp2Filter.isEmpty())
            cout << "SFP1Filter " << receive2022Struct.mSfp2Filter.toStdString() << endl;

        receive2022Struct.mPlayoutDelay = receiveChannelObject["playoutDelay"].toString();
        if (!receive2022Struct.mPlayoutDelay.isEmpty())
            cout << "PlayoutDelay " << receive2022Struct.mPlayoutDelay.toStdString() << endl;

        receive2022Struct.mVLAN = receiveChannelObject["vlan"].toString();
        if (!receive2022Struct.mVLAN.isEmpty())
            cout << "VLAN " << receive2022Struct.mVLAN.toStdString() << endl;

        receive2022Struct.mSSRC = receiveChannelObject["ssrc"].toString();
        if (!receive2022Struct.mSSRC.isEmpty())
            cout << "SSRC " << receive2022Struct.mSSRC.toStdString() << endl;

        receive2022Struct.mSfp1Enable = receiveChannelObject["sfp1Enable"].toString();
        if (!receive2022Struct.mSfp1Enable.isEmpty())
            cout << "SFP1 Enable " << receive2022Struct.mSfp1Enable.toStdString() << endl;

        receive2022Struct.mSfp2Enable = receiveChannelObject["sfp2Enable"].toString();
        if (!receive2022Struct.mSfp2Enable.isEmpty())
            cout << "SFP2 Enable " << receive2022Struct.mSfp2Enable.toStdString() << endl;

        receive2022Struct.mEnable = receiveChannelObject["enable"].toString();
        if (!receive2022Struct.mEnable.isEmpty())
            cout << "Enable " << receive2022Struct.mEnable.toStdString() << endl;
        cout << endl;

        mKonaIP2022Params.mReceive2022Channels.append(receive2022Struct);
    }

    mKonaIP2022Params.mTransmit2022Channels.clear();
    QJsonArray transmitChannelArray = json["transmit2022"].toArray();
    for (int transmitChannelIndex = 0; transmitChannelIndex < transmitChannelArray.size(); ++transmitChannelIndex)
    {
        cout << "Transmit2022Channels" << endl;

        QJsonObject transmitChannelObject = transmitChannelArray[transmitChannelIndex].toObject();
        TransmitStruct2022 transmitStruct2022;

        transmitStruct2022.mChannelDesignator = transmitChannelObject["designator"].toString();
        if (!transmitStruct2022.mChannelDesignator.isEmpty())
            cout << "ChannelDesignator " << transmitStruct2022.mChannelDesignator.toStdString() << endl;

        transmitStruct2022.mSfp1RemoteIPAddress = transmitChannelObject["sfp1RemoteIPAddress"].toString();
        if (!transmitStruct2022.mSfp1RemoteIPAddress.isEmpty())
            cout << "SFP1RemoteIPAddress " << transmitStruct2022.mSfp1RemoteIPAddress.toStdString() << endl;

        transmitStruct2022.mSfp1RemotePort = transmitChannelObject["sfp1RemotePort"].toString();
        if (!transmitStruct2022.mSfp1RemotePort.isEmpty())
            cout << "SFP1RemotePort " << transmitStruct2022.mSfp1RemotePort.toStdString() << endl;

        transmitStruct2022.mSfp1LocalPort = transmitChannelObject["sfp1LocalPort"].toString();
        if (!transmitStruct2022.mSfp1LocalPort.isEmpty())
            cout << "SFP1LocalPort " << transmitStruct2022.mSfp1LocalPort.toStdString() << endl;

        transmitStruct2022.mSfp2RemoteIPAddress = transmitChannelObject["sfp2RemoteIPAddress"].toString();
        if (!transmitStruct2022.mSfp2RemoteIPAddress.isEmpty())
            cout << "SFP2RemoteIPAddress " << transmitStruct2022.mSfp2RemoteIPAddress.toStdString() << endl;

        transmitStruct2022.mSfp2RemotePort = transmitChannelObject["sfp2RemotePort"].toString();
        if (!transmitStruct2022.mSfp2RemoteIPAddress.isEmpty())
            cout << "SFP2RemotePort " << transmitStruct2022.mSfp2RemotePort.toStdString() << endl;

        transmitStruct2022.mSfp2LocalPort = transmitChannelObject["sfp2LocalPort"].toString();
        if (!transmitStruct2022.mSfp2LocalPort.isEmpty())
            cout << "SFP2LocalPort " << transmitStruct2022.mSfp2LocalPort.toStdString() << endl;

        transmitStruct2022.mTOS = transmitChannelObject["tos"].toString();
        if (!transmitStruct2022.mTOS.isEmpty())
            cout << "TOS " << transmitStruct2022.mTOS.toStdString() << endl;

        transmitStruct2022.mTTL = transmitChannelObject["ttl"].toString();
        if (!transmitStruct2022.mTTL.isEmpty())
            cout << "TTL " << transmitStruct2022.mTTL.toStdString() << endl;

        transmitStruct2022.mSSRC = transmitChannelObject["ssrc"].toString();
        if (!transmitStruct2022.mSSRC.isEmpty())
            cout << "SSRC " << transmitStruct2022.mSSRC.toStdString() << endl;

        transmitStruct2022.mSfp1Enable = transmitChannelObject["sfp1Enable"].toString();
        if (!transmitStruct2022.mSfp1Enable.isEmpty())
            cout << "SFP1 Enable " << transmitStruct2022.mSfp1Enable.toStdString() << endl;

        transmitStruct2022.mSfp2Enable = transmitChannelObject["sfp2Enable"].toString();
        if (!transmitStruct2022.mSfp2Enable.isEmpty())
            cout << "SFP2 Enable " << transmitStruct2022.mSfp2Enable.toStdString() << endl;

        transmitStruct2022.mEnable = transmitChannelObject["enable"].toString();
        if (!transmitStruct2022.mEnable.isEmpty())
            cout << "Enable " << transmitStruct2022.mEnable.toStdString() << endl;
        cout << endl;

        mKonaIP2022Params.mTransmit2022Channels.append(transmitStruct2022);
    }
    return true;
}

bool CKonaIpJsonSetup::readJson2110(const QJsonObject &json)
{
    mKonaIP2110Params.mSFPs.clear();
    QJsonArray sfpArray = json["sfps"].toArray();
    for (int sfpIndex = 0; sfpIndex < sfpArray.size(); ++sfpIndex)
    {
        cout << "SFP" << endl;

        QJsonObject sfpObject = sfpArray[sfpIndex].toObject();
        SFPStruct sfpStruct;

        sfpStruct.mSfpDesignator = sfpObject["designator"].toString();
        if (!sfpStruct.mSfpDesignator.isEmpty())
            cout << "SFPDesignator " << sfpStruct.mSfpDesignator.toStdString() << endl;

        sfpStruct.mIPAddress = sfpObject["ipAddress"].toString();
        if (!sfpStruct.mIPAddress.isEmpty())
            cout << "IPAddress " << sfpStruct.mIPAddress.toStdString() << endl;

        sfpStruct.mSubnetMask = sfpObject["subnetMask"].toString();
        if (!sfpStruct.mSubnetMask.isEmpty())
            cout << "SubnetMask " << sfpStruct.mSubnetMask.toStdString() << endl;

        sfpStruct.mGateway = sfpObject["gateway"].toString();
        if (!sfpStruct.mGateway.isEmpty())
            cout << "Gateway " << sfpStruct.mGateway.toStdString() << endl;

        sfpStruct.mEnable = sfpObject["enable"].toString();
        if (!sfpStruct.mEnable.isEmpty())
            cout << "Enable " << sfpStruct.mEnable.toStdString() << endl;
        cout << endl;

        mKonaIP2110Params.mSFPs.append(sfpStruct);
    }

    mKonaIP2110Params.mReceiveVideo2110Channels.clear();
    QJsonArray receiveVideoChannelArray = json["receiveVideo2110"].toArray();
    for (int receiveChannelIndex = 0; receiveChannelIndex < receiveVideoChannelArray.size(); ++receiveChannelIndex)
    {
        cout << "Receive2110VideoChannels" << endl;

        QJsonObject receiveChannelObject = receiveVideoChannelArray[receiveChannelIndex].toObject();
        ReceiveStructVideo2110 receiveStruct;

        receiveStruct.mChannelDesignator = receiveChannelObject["designator"].toString();
        if (!receiveStruct.mChannelDesignator.isEmpty())
            cout << "ChannelDesignator " << receiveStruct.mChannelDesignator.toStdString() << endl;

        receiveStruct.mSfp1SrcIPAddress = receiveChannelObject["sfp1SrcIPAddress"].toString();
        if (!receiveStruct.mSfp1SrcIPAddress.isEmpty())
            cout << "SFP1SrcIPAddress " << receiveStruct.mSfp1SrcIPAddress.toStdString() << endl;

        receiveStruct.mSfp1SrcPort = receiveChannelObject["sfp1SrcPort"].toString();
        if (!receiveStruct.mSfp1SrcPort.isEmpty())
            cout << "SFP1SrcPort " << receiveStruct.mSfp1SrcPort.toStdString() << endl;

        receiveStruct.mSfp1DestIPAddress = receiveChannelObject["sfp1DestIPAddress"].toString();
        if (!receiveStruct.mSfp1DestIPAddress.isEmpty())
            cout << "SFP1DestIPAddress " << receiveStruct.mSfp1DestIPAddress.toStdString() << endl;

        receiveStruct.mSfp1DestPort = receiveChannelObject["sfp1DestPort"].toString();
        if (!receiveStruct.mSfp1DestPort.isEmpty())
            cout << "SFP1DestPort " << receiveStruct.mSfp1DestPort.toStdString() << endl;

        receiveStruct.mSfp1Filter = receiveChannelObject["sfp1Filter"].toString();
        if (!receiveStruct.mSfp1Filter.isEmpty())
            cout << "SFP1Filter " << receiveStruct.mSfp1Filter.toStdString() << endl;

        receiveStruct.mSfp2SrcIPAddress = receiveChannelObject["sfp2SrcIPAddress"].toString();
        if (!receiveStruct.mSfp2SrcIPAddress.isEmpty())
            cout << "SFP2SrcIAddress " << receiveStruct.mSfp2SrcIPAddress.toStdString() << endl;

        receiveStruct.mSfp2SrcPort = receiveChannelObject["sfp2SrcPort"].toString();
        if (!receiveStruct.mSfp2SrcPort.isEmpty())
            cout << "SFP2SrcPort " << receiveStruct.mSfp2SrcPort.toStdString() << endl;

        receiveStruct.mSfp2DestIPAddress = receiveChannelObject["sfp2DestIPAddress"].toString();
        if (!receiveStruct.mSfp2DestIPAddress.isEmpty())
            cout << "SFP2DestIPAddress " << receiveStruct.mSfp2DestIPAddress.toStdString() << endl;

        receiveStruct.mSfp2DestPort = receiveChannelObject["sfp2DestPort"].toString();
        if (!receiveStruct.mSfp2DestPort.isEmpty())
            cout << "SFP2DestPort " << receiveStruct.mSfp2DestPort.toStdString() << endl;

        receiveStruct.mSfp2Filter = receiveChannelObject["sfp2Filter"].toString();
        if (!receiveStruct.mSfp2Filter.isEmpty())
            cout << "SFP2Filter " << receiveStruct.mSfp2Filter.toStdString() << endl;

        receiveStruct.mVLAN = receiveChannelObject["vlan"].toString();
        if (!receiveStruct.mVLAN.isEmpty())
            cout << "VLAN " << receiveStruct.mVLAN.toStdString() << endl;

        receiveStruct.mSSRC = receiveChannelObject["ssrc"].toString();
        if (!receiveStruct.mSSRC.isEmpty())
            cout << "SSRC " << receiveStruct.mSSRC.toStdString() << endl;

        receiveStruct.mPayload = receiveChannelObject["payload"].toString();
        if (!receiveStruct.mPayload.isEmpty())
            cout << "Payload " << receiveStruct.mPayload.toStdString() << endl;

        receiveStruct.mVideoFormat = receiveChannelObject["videoFormat"].toString();
        if (!receiveStruct.mVideoFormat.isEmpty())
            cout << "Video Format " << receiveStruct.mVideoFormat.toStdString() << endl;

        receiveStruct.mSfp1Enable = receiveChannelObject["sfp1Enable"].toString();
        if (!receiveStruct.mSfp1Enable.isEmpty())
            cout << "SFP1 Enable " << receiveStruct.mSfp1Enable.toStdString() << endl;

        receiveStruct.mSfp2Enable = receiveChannelObject["sfp2Enable"].toString();
        if (!receiveStruct.mSfp2Enable.isEmpty())
            cout << "SFP2 Enable " << receiveStruct.mSfp2Enable.toStdString() << endl;

        receiveStruct.mEnable = receiveChannelObject["enable"].toString();
        if (!receiveStruct.mEnable.isEmpty())
            cout << "Enable " << receiveStruct.mEnable.toStdString() << endl;
        cout << endl;

        mKonaIP2110Params.mReceiveVideo2110Channels.append(receiveStruct);
    }

    mKonaIP2110Params.mReceiveAudio2110Channels.clear();
    QJsonArray receiveAudioChannelArray = json["receiveAudio2110"].toArray();
    for (int receiveChannelIndex = 0; receiveChannelIndex < receiveAudioChannelArray.size(); ++receiveChannelIndex)
    {
        cout << "Receive2110AudioChannels" << endl;

        QJsonObject receiveChannelObject = receiveAudioChannelArray[receiveChannelIndex].toObject();
        ReceiveStructAudio2110 receiveStruct;

        receiveStruct.mChannelDesignator = receiveChannelObject["designator"].toString();
        if (!receiveStruct.mChannelDesignator.isEmpty())
            cout << "ChannelDesignator " << receiveStruct.mChannelDesignator.toStdString() << endl;

        receiveStruct.mStream = receiveChannelObject["stream"].toString();
        if (!receiveStruct.mStream.isEmpty())
            cout << "Stream " << receiveStruct.mStream.toStdString() << endl;

        receiveStruct.mSfp1SrcIPAddress = receiveChannelObject["sfp1SrcIPAddress"].toString();
        if (!receiveStruct.mSfp1SrcIPAddress.isEmpty())
            cout << "SFP1SrcIPAddress " << receiveStruct.mSfp1SrcIPAddress.toStdString() << endl;

        receiveStruct.mSfp1SrcPort = receiveChannelObject["sfp1SrcPort"].toString();
        if (!receiveStruct.mSfp1SrcPort.isEmpty())
            cout << "SFP1SrcPort " << receiveStruct.mSfp1SrcPort.toStdString() << endl;

        receiveStruct.mSfp1DestIPAddress = receiveChannelObject["sfp1DestIPAddress"].toString();
        if (!receiveStruct.mSfp1DestIPAddress.isEmpty())
            cout << "SFP1DestIPAddress " << receiveStruct.mSfp1DestIPAddress.toStdString() << endl;

        receiveStruct.mSfp1DestPort = receiveChannelObject["sfp1DestPort"].toString();
        if (!receiveStruct.mSfp1DestPort.isEmpty())
            cout << "SFP1DestPort " << receiveStruct.mSfp1DestPort.toStdString() << endl;

        receiveStruct.mSfp1Filter = receiveChannelObject["sfp1Filter"].toString();
        if (!receiveStruct.mSfp1Filter.isEmpty())
            cout << "SFP1Filter " << receiveStruct.mSfp1Filter.toStdString() << endl;

        receiveStruct.mSfp2SrcIPAddress = receiveChannelObject["sfp2SrcIPAddress"].toString();
        if (!receiveStruct.mSfp2SrcIPAddress.isEmpty())
            cout << "SFP2SrcIAddress " << receiveStruct.mSfp2SrcIPAddress.toStdString() << endl;

        receiveStruct.mSfp2SrcPort = receiveChannelObject["sfp2SrcPort"].toString();
        if (!receiveStruct.mSfp2SrcPort.isEmpty())
            cout << "SFP2SrcPort " << receiveStruct.mSfp2SrcPort.toStdString() << endl;

        receiveStruct.mSfp2DestIPAddress = receiveChannelObject["sfp2DestIPAddress"].toString();
        if (!receiveStruct.mSfp2DestIPAddress.isEmpty())
            cout << "SFP2DestIPAddress " << receiveStruct.mSfp2DestIPAddress.toStdString() << endl;

        receiveStruct.mSfp2DestPort = receiveChannelObject["sfp2DestPort"].toString();
        if (!receiveStruct.mSfp2DestPort.isEmpty())
            cout << "SFP2DestPort " << receiveStruct.mSfp2DestPort.toStdString() << endl;

        receiveStruct.mSfp2Filter = receiveChannelObject["sfp2Filter"].toString();
        if (!receiveStruct.mSfp2Filter.isEmpty())
            cout << "SFP2Filter " << receiveStruct.mSfp2Filter.toStdString() << endl;

        receiveStruct.mVLAN = receiveChannelObject["vlan"].toString();
        if (!receiveStruct.mVLAN.isEmpty())
            cout << "VLAN " << receiveStruct.mVLAN.toStdString() << endl;

        receiveStruct.mSSRC = receiveChannelObject["ssrc"].toString();
        if (!receiveStruct.mSSRC.isEmpty())
            cout << "SSRC " << receiveStruct.mSSRC.toStdString() << endl;

        receiveStruct.mPayload = receiveChannelObject["payload"].toString();
        if (!receiveStruct.mPayload.isEmpty())
            cout << "Payload " << receiveStruct.mPayload.toStdString() << endl;

        receiveStruct.mNumAudioChannels = receiveChannelObject["numAudioChannels"].toString();
        if (!receiveStruct.mNumAudioChannels.isEmpty())
            cout << "Num Audio Channels " << receiveStruct.mNumAudioChannels.toStdString() << endl;

        receiveStruct.mAudioPktInterval = receiveChannelObject["audioPktInterval"].toString();
        if (!receiveStruct.mAudioPktInterval.isEmpty())
            cout << "Audio Packet Interval " << receiveStruct.mAudioPktInterval.toStdString() << endl;

        receiveStruct.mSfp1Enable = receiveChannelObject["sfp1Enable"].toString();
        if (!receiveStruct.mSfp1Enable.isEmpty())
            cout << "SFP1 Enable " << receiveStruct.mSfp1Enable.toStdString() << endl;

        receiveStruct.mSfp2Enable = receiveChannelObject["sfp2Enable"].toString();
        if (!receiveStruct.mSfp2Enable.isEmpty())
            cout << "SFP2 Enable " << receiveStruct.mSfp2Enable.toStdString() << endl;

        receiveStruct.mEnable = receiveChannelObject["enable"].toString();
        if (!receiveStruct.mEnable.isEmpty())
            cout << "Enable " << receiveStruct.mEnable.toStdString() << endl;
        cout << endl;

        mKonaIP2110Params.mReceiveAudio2110Channels.append(receiveStruct);
    }

    mKonaIP2110Params.mTransmitVideo2110Channels.clear();
    QJsonArray transmitVideoChannelArray = json["transmitVideo2110"].toArray();
    for (int transmitChannelIndex = 0; transmitChannelIndex < transmitVideoChannelArray.size(); ++transmitChannelIndex)
    {
        cout << "Transmit2110VideoChannels" << endl;

        QJsonObject transmitChannelObject = transmitVideoChannelArray[transmitChannelIndex].toObject();
        TransmitStructVideo2110 transmitStruct;

        transmitStruct.mChannelDesignator = transmitChannelObject["designator"].toString();
        if (!transmitStruct.mChannelDesignator.isEmpty())
            cout << "ChannelDesignator " << transmitStruct.mChannelDesignator.toStdString() << endl;

        transmitStruct.mSfp1RemoteIPAddress = transmitChannelObject["sfp1RemoteIPAddress"].toString();
        if (!transmitStruct.mSfp1RemoteIPAddress.isEmpty())
            cout << "SFP1RemoteIPAddress " << transmitStruct.mSfp1RemoteIPAddress.toStdString() << endl;

        transmitStruct.mSfp1RemotePort = transmitChannelObject["sfp1RemotePort"].toString();
        if (!transmitStruct.mSfp1RemotePort.isEmpty())
            cout << "SFP1RemotePort " << transmitStruct.mSfp1RemotePort.toStdString() << endl;

        transmitStruct.mSfp1LocalPort = transmitChannelObject["sfp1LocalPort"].toString();
        if (!transmitStruct.mSfp1LocalPort.isEmpty())
            cout << "SFP1LocalPort " << transmitStruct.mSfp1LocalPort.toStdString() << endl;

        transmitStruct.mSfp2RemoteIPAddress = transmitChannelObject["sfp2RemoteIPAddress"].toString();
        if (!transmitStruct.mSfp2RemoteIPAddress.isEmpty())
            cout << "SFP2RemoteIPAddress " << transmitStruct.mSfp2RemoteIPAddress.toStdString() << endl;

        transmitStruct.mSfp2RemotePort = transmitChannelObject["sfp2RemotePort"].toString();
        if (!transmitStruct.mSfp2RemoteIPAddress.isEmpty())
            cout << "SFP2RemotePort " << transmitStruct.mSfp2RemotePort.toStdString() << endl;

        transmitStruct.mSfp2LocalPort = transmitChannelObject["sfp2LocalPort"].toString();
        if (!transmitStruct.mSfp2LocalPort.isEmpty())
            cout << "SFP2LocalPort " << transmitStruct.mSfp2LocalPort.toStdString() << endl;

        transmitStruct.mVideoFormat = transmitChannelObject["videoFormat"].toString();
        if (!transmitStruct.mVideoFormat.isEmpty())
            cout << "Video format " << transmitStruct.mVideoFormat.toStdString() << endl;

        transmitStruct.mPayload = transmitChannelObject["payload"].toString();
        if (!transmitStruct.mPayload.isEmpty())
            cout << "Payload " << transmitStruct.mPayload.toStdString() << endl;

        transmitStruct.mSSRC = transmitChannelObject["ssrc"].toString();
        if (!transmitStruct.mSSRC.isEmpty())
            cout << "SSRC " << transmitStruct.mSSRC.toStdString() << endl;

        transmitStruct.mTTL = transmitChannelObject["ttl"].toString();
        if (!transmitStruct.mTTL.isEmpty())
            cout << "TTL " << transmitStruct.mTTL.toStdString() << endl;

        transmitStruct.mSfp1Enable = transmitChannelObject["sfp1Enable"].toString();
        if (!transmitStruct.mSfp1Enable.isEmpty())
            cout << "SFP1 Enable " << transmitStruct.mSfp1Enable.toStdString() << endl;

        transmitStruct.mSfp2Enable = transmitChannelObject["sfp2Enable"].toString();
        if (!transmitStruct.mSfp2Enable.isEmpty())
            cout << "SFP2 Enable " << transmitStruct.mSfp2Enable.toStdString() << endl;

        transmitStruct.mEnable = transmitChannelObject["enable"].toString();
        if (!transmitStruct.mSfp2Enable.isEmpty())
            cout << "Enable " << transmitStruct.mEnable.toStdString() << endl;
        cout << endl;

        mKonaIP2110Params.mTransmitVideo2110Channels.append(transmitStruct);
    }

    mKonaIP2110Params.mTransmitAudio2110Channels.clear();
    QJsonArray transmitAudioChannelArray = json["transmitAudio2110"].toArray();
    for (int transmitChannelIndex = 0; transmitChannelIndex < transmitVideoChannelArray.size(); ++transmitChannelIndex)
    {
        cout << "Transmit2110AudioChannels" << endl;

        QJsonObject transmitChannelObject = transmitAudioChannelArray[transmitChannelIndex].toObject();
        TransmitStructAudio2110 transmitStruct;

        transmitStruct.mChannelDesignator = transmitChannelObject["designator"].toString();
        if (!transmitStruct.mChannelDesignator.isEmpty())
            cout << "ChannelDesignator " << transmitStruct.mChannelDesignator.toStdString() << endl;

        transmitStruct.mStream = transmitChannelObject["stream"].toString();
        if (!transmitStruct.mStream.isEmpty())
            cout << "Stream " << transmitStruct.mStream.toStdString() << endl;

        transmitStruct.mSfp1RemoteIPAddress = transmitChannelObject["sfp1RemoteIPAddress"].toString();
        if (!transmitStruct.mSfp1RemoteIPAddress.isEmpty())
            cout << "SFP1RemoteIPAddress " << transmitStruct.mSfp1RemoteIPAddress.toStdString() << endl;

        transmitStruct.mSfp1RemotePort = transmitChannelObject["sfp1RemotePort"].toString();
        if (!transmitStruct.mSfp1RemotePort.isEmpty())
            cout << "SFP1RemotePort " << transmitStruct.mSfp1RemotePort.toStdString() << endl;

        transmitStruct.mSfp1LocalPort = transmitChannelObject["sfp1LocalPort"].toString();
        if (!transmitStruct.mSfp1LocalPort.isEmpty())
            cout << "SFP1LocalPort " << transmitStruct.mSfp1LocalPort.toStdString() << endl;

        transmitStruct.mSfp2RemoteIPAddress = transmitChannelObject["sfp2RemoteIPAddress"].toString();
        if (!transmitStruct.mSfp2RemoteIPAddress.isEmpty())
            cout << "SFP2RemoteIPAddress " << transmitStruct.mSfp2RemoteIPAddress.toStdString() << endl;

        transmitStruct.mSfp2RemotePort = transmitChannelObject["sfp2RemotePort"].toString();
        if (!transmitStruct.mSfp2RemoteIPAddress.isEmpty())
            cout << "SFP2RemotePort " << transmitStruct.mSfp2RemotePort.toStdString() << endl;

        transmitStruct.mSfp2LocalPort = transmitChannelObject["sfp2LocalPort"].toString();
        if (!transmitStruct.mSfp2LocalPort.isEmpty())
            cout << "SFP2LocalPort " << transmitStruct.mSfp2LocalPort.toStdString() << endl;

        transmitStruct.mNumAudioChannels = transmitChannelObject["numAudioChannels"].toString();
        if (!transmitStruct.mNumAudioChannels.isEmpty())
            cout << "Num Audio Channels " << transmitStruct.mNumAudioChannels.toStdString() << endl;

        transmitStruct.mFirstAudioChannel = transmitChannelObject["firstAudioChannel"].toString();
        if (!transmitStruct.mFirstAudioChannel.isEmpty())
            cout << "First Audio Channel " << transmitStruct.mFirstAudioChannel.toStdString() << endl;

        transmitStruct.mAudioPktInterval = transmitChannelObject["audioPktInterval"].toString();
        if (!transmitStruct.mAudioPktInterval.isEmpty())
            cout << "Audio Packet Interval " << transmitStruct.mAudioPktInterval.toStdString() << endl;

        transmitStruct.mPayload = transmitChannelObject["payload"].toString();
        if (!transmitStruct.mPayload.isEmpty())
            cout << "Payload " << transmitStruct.mPayload.toStdString() << endl;

        transmitStruct.mSSRC = transmitChannelObject["ssrc"].toString();
        if (!transmitStruct.mSSRC.isEmpty())
            cout << "SSRC " << transmitStruct.mSSRC.toStdString() << endl;

        transmitStruct.mTTL = transmitChannelObject["ttl"].toString();
        if (!transmitStruct.mTTL.isEmpty())
            cout << "TTL " << transmitStruct.mTTL.toStdString() << endl;

        transmitStruct.mSfp1Enable = transmitChannelObject["sfp1Enable"].toString();
        if (!transmitStruct.mSfp1Enable.isEmpty())
            cout << "SFP1 Enable " << transmitStruct.mSfp1Enable.toStdString() << endl;

        transmitStruct.mSfp2Enable = transmitChannelObject["sfp2Enable"].toString();
        if (!transmitStruct.mSfp2Enable.isEmpty())
            cout << "SFP2 Enable " << transmitStruct.mSfp2Enable.toStdString() << endl;

        transmitStruct.mEnable = transmitChannelObject["enable"].toString();
        if (!transmitStruct.mSfp2Enable.isEmpty())
            cout << "Enable " << transmitStruct.mEnable.toStdString() << endl;
        cout << endl;

        mKonaIP2110Params.mTransmitAudio2110Channels.append(transmitStruct);
    }

    return true;
}

bool CKonaIpJsonSetup::setupBoard(std::string deviceSpec)
{
    if (mIs2110)
    {
        return setupBoard2110(deviceSpec);
    }
    else
    {
        return setupBoard2022(deviceSpec);
    }
}

bool CKonaIpJsonSetup::setupBoard2022(std::string deviceSpec)
{
    return true;

    CNTV2Card mDevice;
    CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, mDevice);
    if (!mDevice.IsOpen())
    {
        cerr << "## ERROR:  No devices found " << deviceSpec.c_str() << endl;
        return false;
    }
    //if (!mDevice.IsKonaIPDevice ())
    //    {cerr << "## ERROR:  Not a KONA IP device" << endl;  return false;}

    //	Wait for device ready
    while (!mDevice.IsMBSystemReady())
    {
        cout << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << endl;
        mDevice.SleepMs (1000);
        if (mDevice.IsMBSystemReady ())
        {
            cout << "## NOTE:  Device is ready" << endl;
        }
    }

    if (!mDevice.IsMBSystemValid())
    {
        cerr << "## ERROR: board firmware package is incompatible with this application" << endl;
        return false;
    }

    CNTV2Config2022	config2022 (mDevice);

    if (mKonaIP2022Params.mSFPs.size() < 1)
    {
        cerr << "## ERROR:  Need To Specify at Least 1 SFP" << endl;
        return false;
    }

    QListIterator<SFPStruct> sfpIter(mKonaIP2022Params.mSFPs);
    while (sfpIter.hasNext())
    {
        SFPStruct sfp = sfpIter.next();

        if ( sfp.mSfpDesignator == "sfp1")
        {
            bool enable;
            if (sfp.mEnable.isEmpty())
                enable = true;
            else
                enable = getEnable(sfp.mEnable);

            if (enable)
            {
                bool rv = config2022.SetNetworkConfiguration (SFP_1,
                                                              sfp.mIPAddress.toStdString(),
                                                              sfp.mSubnetMask.toStdString(),
                                                              sfp.mGateway.toStdString());
                if (!rv)
                {
                    cerr << "Error: " << config2022.getLastError() << endl;
                    return false;
                }
            }
            else
            {
                config2022.DisableNetworkInterface(SFP_1);
            }
        }
        else if ( sfp.mSfpDesignator == "sfp2")
        {
            bool enable;
            if (sfp.mEnable.isEmpty())
                enable = true;
            else
                enable = getEnable(sfp.mEnable);

            if (enable)
            {
                bool rv = config2022.SetNetworkConfiguration (SFP_2,
                                                              sfp.mIPAddress.toStdString(),
                                                              sfp.mSubnetMask.toStdString(),
                                                              sfp.mGateway.toStdString());
                if (!rv)
                {
                    cerr << "Error: " << config2022.getLastError() << endl;
                    return false;
                }
            }
            else
            {
                config2022.DisableNetworkInterface(SFP_2);
            }
        }
    }

    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekFwCfg, &val);
    bool supports2022_7 = ((val & SAREK_2022_7) != 0);
    if (supports2022_7)
    {
        bool rv = config2022.Set2022_7_Mode(mEnable2022_7,mNetworkPathDifferential);
        if (!rv)
        {
            cerr << "Error: " << config2022.getLastError() << endl;
            return false;
        }
    }

    cerr << "## receiveIter" << endl;

    QListIterator<ReceiveStruct2022> receiveIter(mKonaIP2022Params.mReceive2022Channels);
    while (receiveIter.hasNext())
    {
        cerr << "## receiveIter did" << endl;

        ReceiveStruct2022 receive = receiveIter.next();
        rx_2022_channel rxChannelConfig;
        bool ok;
        NTV2Channel channel = getChannel(receive.mChannelDesignator);

        if (mEnable2022_7)
        {
            rxChannelConfig.sfp1Enable = true;
            rxChannelConfig.sfp2Enable = true;
        }
        else
        {
            if (!receive.mSfp1Enable.isEmpty())
                rxChannelConfig.sfp1Enable = (getEnable(receive.mSfp1Enable));
            if (!receive.mSfp2Enable.isEmpty())
                rxChannelConfig.sfp2Enable = (getEnable(receive.mSfp2Enable));
        }
        rxChannelConfig.sfp1SourceIP        = receive.mSfp1SrcIPAddress.toStdString();
        rxChannelConfig.sfp1SourcePort      = receive.mSfp1SrcPort.toUInt();
        rxChannelConfig.sfp1DestIP          = receive.mSfp1DestIPAddress.toStdString();
        rxChannelConfig.sfp1DestPort        = receive.mSfp1DestPort.toUInt();
        rxChannelConfig.sfp1RxMatch         = receive.mSfp1Filter.toUInt(&ok, 16);
        rxChannelConfig.sfp2SourceIP        = receive.mSfp2SrcIPAddress.toStdString();
        rxChannelConfig.sfp2SourcePort      = receive.mSfp2SrcPort.toUInt();
        rxChannelConfig.sfp2DestIP          = receive.mSfp2DestIPAddress.toStdString();
        rxChannelConfig.sfp2DestPort        = receive.mSfp2DestPort.toUInt();
        rxChannelConfig.sfp2RxMatch         = receive.mSfp2Filter.toUInt(&ok, 16);
        rxChannelConfig.playoutDelay        = receive.mPlayoutDelay.toUInt();
        rxChannelConfig.ssrc                = receive.mSSRC.toUInt();

        bool rv = config2022.SetRxChannelConfiguration (channel, rxChannelConfig);
        if (!rv)
        {
            cerr << "Error (config2022.SetRxChannelConfiguration) " << config2022.getLastError() << endl;
            return false;
        }
        rv = config2022.SetRxChannelEnable (channel, getEnable(receive.mEnable));
        if (!rv)
        {
            cerr << "Error (config2022.SetRxChannelConfiguration) " << config2022.getLastError() << endl;
            return false;
        }
    }
    cerr << "## transmitIter" << endl;

    QListIterator<TransmitStruct2022> transmitIter(mKonaIP2022Params.mTransmit2022Channels);
    while (transmitIter.hasNext())
    {
        cerr << "## transmitIter did" << endl;

        TransmitStruct2022 transmit = transmitIter.next();
        tx_2022_channel txChannelConfig;

        NTV2Channel channel = getChannel(transmit.mChannelDesignator);
        if (mEnable2022_7)
        {
            txChannelConfig.sfp1Enable = true;
            txChannelConfig.sfp2Enable = true;
        }
        else
        {
            if (!transmit.mSfp1Enable.isEmpty())
                txChannelConfig.sfp1Enable = (getEnable(transmit.mSfp1Enable));
            if (!transmit.mSfp2Enable.isEmpty())
                txChannelConfig.sfp2Enable = (getEnable(transmit.mSfp2Enable));
        }
        txChannelConfig.sfp1LocalPort       = transmit.mSfp1LocalPort.toUInt();
        txChannelConfig.sfp1RemoteIP        = transmit.mSfp1RemoteIPAddress.toStdString();
        txChannelConfig.sfp1RemotePort      = transmit.mSfp1RemotePort.toUInt();
        txChannelConfig.sfp2LocalPort       = transmit.mSfp2LocalPort.toUInt();
        txChannelConfig.sfp2RemoteIP        = transmit.mSfp2RemoteIPAddress.toStdString();
        txChannelConfig.sfp2RemotePort      = transmit.mSfp2RemotePort.toUInt();
        txChannelConfig.ssrc                = transmit.mSSRC.toUInt();
        txChannelConfig.tos                 = transmit.mTOS.toUInt();
        txChannelConfig.ttl                 = transmit.mTTL.toUInt();

        bool rv = config2022.SetTxChannelConfiguration (channel, txChannelConfig);
        if (!rv)
        {
            cerr << "Error (config2022.SetTxChannelConfiguration) " << config2022.getLastError() << endl;
            return false;
        }
        rv = config2022.SetTxChannelEnable (channel, getEnable(transmit.mEnable));
        if (!rv)
        {
            cerr << "Error (config2022.SetTxChannelEnable) " << config2022.getLastError() << endl;
            return false;
        }
    }

    return true;
}

bool CKonaIpJsonSetup::setupBoard2110(std::string deviceSpec)
{
    CNTV2Card device;
    CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, device);
    if (!device.IsOpen())
    {cerr << "## ERROR:  No devices found " << deviceSpec.c_str() << endl;  return false;}
    //if (!mDevice.IsKonaIPDevice ())
    //    {cerr << "## ERROR:  Not a KONA IP device" << endl;  return false;}

    //	Read MicroBlaze Uptime in Seconds, to see if it's running...
    while (!device.IsMBSystemReady())
    {
        cout << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << endl;
        device.SleepMs (1000);
        if (device.IsMBSystemReady ())
        {
            cout << "## NOTE:  Device is ready" << endl;
            if (device.IsMBSystemValid())
            {
                cerr << "## ERROR: board firmware package is incompatible with this application" << endl;
                return false;
            }
        }
    }

    CNTV2Config2110	config2110 (device);
	const NTV2VideoFormatKinds allowedVideoFormatTypes(::NTV2DeviceCanDo4KVideo(device.GetDeviceID()) ? VIDEO_FORMATS_ALL : VIDEO_FORMATS_NON_4KUHD);

    if (!mPTPMasterAddr.isEmpty())
    {
        device.SetReference(NTV2_REFERENCE_SFP1_PTP);
        config2110.SetPTPMaster(mPTPMasterAddr.toStdString());
    }

    config2110.Set4KModeEnable(m4KMode);

    if (mKonaIP2110Params.mSFPs.size() < 1)
    {
        cerr << "## ERROR:  Need To Specify at Least 1 SFP" << endl;
        return false;
    }

    QListIterator<SFPStruct> sfpIter(mKonaIP2110Params.mSFPs);
    while (sfpIter.hasNext())
    {
        SFPStruct sfp = sfpIter.next();

        if ( sfp.mSfpDesignator == "sfp1")
        {
            bool enable;
            if (sfp.mEnable.isEmpty())
                enable = true;
            else
                enable = getEnable(sfp.mEnable);

            if (enable)
            {
                bool rv = config2110.SetNetworkConfiguration (SFP_1,
                                                              sfp.mIPAddress.toStdString(),
                                                              sfp.mSubnetMask.toStdString(),
                                                              sfp.mGateway.toStdString());
                if (!rv)
                {
                    cerr << "Error: " << config2110.getLastError() << endl;
                    return false;
                }
            }
            else
            {
                config2110.DisableNetworkInterface(SFP_1);
            }
        }
        else if ( sfp.mSfpDesignator == "sfp2")
        {
            bool enable;
            if (sfp.mEnable.isEmpty())
                enable = true;
            else
                enable = getEnable(sfp.mEnable);

            if (enable)
            {
                bool rv = config2110.SetNetworkConfiguration (SFP_2,
                                                              sfp.mIPAddress.toStdString(),
                                                              sfp.mSubnetMask.toStdString(),
                                                              sfp.mGateway.toStdString());
                if (!rv)
                {
                    cerr << "SetNetworkConfiguration Error: " << config2110.getLastError() << endl;
                    return false;
                }
            }
            else
            {
                config2110.DisableNetworkInterface(SFP_2);
            }
        }
    }

    cerr << "## receiveVideoIter" << endl;
    QListIterator<ReceiveStructVideo2110> receive2110VideoIter(mKonaIP2110Params.mReceiveVideo2110Channels);

    rx_2110Config rxChannelConfig;
    while (receive2110VideoIter.hasNext())
    {
        cerr << "## receiveVideoIter did" << endl;

        ReceiveStructVideo2110 receiveVideo = receive2110VideoIter.next();

        rxChannelConfig.init();
        bool ok;
        NTV2Channel channel = getChannel(receiveVideo.mChannelDesignator);
        NTV2Stream stream = NTV2_VIDEO_STREAM;

        // If sfp2 is on use it otherwise assume we are always dealing with sfp1
        // (RX does not support both at the moment)
        bool sfp2On = false;
        if (!receiveVideo.mSfp2Enable.isEmpty())
            sfp2On = getEnable(receiveVideo.mSfp2Enable);

        eSFP sfp;
        if (sfp2On)
            sfp = SFP_2;
        else
            sfp = SFP_1;

        bool enable = getEnable(receiveVideo.mEnable);
        if (!enable)
        {
            bool rv = config2110.DisableRxStream (sfp, channel, stream);
            if (!rv)
            {
                cerr << "DisableRxStream: FAILED: " << config2110.getLastError() << endl;
                return false;
            }
        }

        // Until we support -7, only use one link
        if (sfp == SFP_1)
        {
            rxChannelConfig.rxMatch      = receiveVideo.mSfp1Filter.toUInt(&ok, 16);
            rxChannelConfig.sourceIP     = receiveVideo.mSfp1SrcIPAddress.toStdString();
            rxChannelConfig.destIP       = receiveVideo.mSfp1DestIPAddress.toStdString();
            rxChannelConfig.sourcePort   = receiveVideo.mSfp1SrcPort.toUInt();
            rxChannelConfig.destPort     = receiveVideo.mSfp1DestPort.toUInt();
        }
        else
        {
            rxChannelConfig.rxMatch      = receiveVideo.mSfp2Filter.toUInt(&ok, 16);
            rxChannelConfig.sourceIP     = receiveVideo.mSfp2SrcIPAddress.toStdString();
            rxChannelConfig.destIP       = receiveVideo.mSfp2DestIPAddress.toStdString();
            rxChannelConfig.sourcePort   = receiveVideo.mSfp2SrcPort.toUInt();
            rxChannelConfig.destPort     = receiveVideo.mSfp2DestPort.toUInt();
        }
        rxChannelConfig.ssrc         = receiveVideo.mSSRC.toUInt();
        rxChannelConfig.vlan         = receiveVideo.mVLAN.toUInt();
        rxChannelConfig.payloadType  = receiveVideo.mPayload.toUInt();

        if (!receiveVideo.mVideoFormat.isEmpty())
        {
            rxChannelConfig.videoFormat  = CNTV2DemoCommon::GetVideoFormatFromString(receiveVideo.mVideoFormat.toStdString(), allowedVideoFormatTypes);
            rxChannelConfig.videoSamples = VPIDSampling_YUV_422;
        }

        //dumpRx2110Config (channel, stream, rxChannelConfig);
        bool rv = config2110.EnableRxStream (sfp, channel, stream, rxChannelConfig);
        if (!rv)
        {
            cerr << "EnableRxVideoStream: FAILED: " << config2110.getLastError() << endl;
            return false;
        }
    }

    cerr << "## receiveAudioIter" << endl;
    QListIterator<ReceiveStructAudio2110> receive2110AudioIter(mKonaIP2110Params.mReceiveAudio2110Channels);

    while (receive2110AudioIter.hasNext())
    {
        cerr << "## receiveAudioIter did" << endl;

        ReceiveStructAudio2110 receiveAudio = receive2110AudioIter.next();

        rxChannelConfig.init();
        bool ok;
        NTV2Channel channel = getChannel(receiveAudio.mChannelDesignator);

        NTV2Stream stream = NTV2_AUDIO1_STREAM;
        if (receiveAudio.mStream == "audio1")
        {
            stream = NTV2_AUDIO1_STREAM;
        }
        else if (receiveAudio.mStream == "audio2")
        {
            stream = NTV2_AUDIO2_STREAM;
        }
        else if (receiveAudio.mStream == "audio3")
        {
            stream = NTV2_AUDIO3_STREAM;
        }
        else if (receiveAudio.mStream == "audio4")
        {
            stream = NTV2_AUDIO4_STREAM;
        }
        else
        {
            cout << "Error: Rx stream <" << receiveAudio.mStream.toStdString() << "> is invalid" << endl;
        }

        // If sfp2 is on use it otherwise assume we are always dealing with sfp1
        // (RX does not support both at the moment)
        bool sfp2On = false;
        if (!receiveAudio.mSfp2Enable.isEmpty())
            sfp2On = getEnable(receiveAudio.mSfp2Enable);

        eSFP sfp;
        if (sfp2On)
            sfp = SFP_2;
        else
            sfp = SFP_1;

        bool enable = getEnable(receiveAudio.mEnable);
        if (!enable)
        {
            bool rv = config2110.DisableRxStream (sfp, channel, stream);
            if (!rv)
            {
                cerr << "DisableRxStream: FAILED: " << config2110.getLastError() << endl;
                return false;
            }
        }

        // Until we support -7, only use one link
        if (sfp == SFP_1)
        {
            rxChannelConfig.rxMatch      = receiveAudio.mSfp1Filter.toUInt(&ok, 16);
            rxChannelConfig.sourceIP     = receiveAudio.mSfp1SrcIPAddress.toStdString();
            rxChannelConfig.destIP       = receiveAudio.mSfp1DestIPAddress.toStdString();
            rxChannelConfig.sourcePort   = receiveAudio.mSfp1SrcPort.toUInt();
            rxChannelConfig.destPort     = receiveAudio.mSfp1DestPort.toUInt();
        }
        else
        {
            rxChannelConfig.rxMatch      = receiveAudio.mSfp2Filter.toUInt(&ok, 16);
            rxChannelConfig.sourceIP     = receiveAudio.mSfp2SrcIPAddress.toStdString();
            rxChannelConfig.destIP       = receiveAudio.mSfp2DestIPAddress.toStdString();
            rxChannelConfig.sourcePort   = receiveAudio.mSfp2SrcPort.toUInt();
            rxChannelConfig.destPort     = receiveAudio.mSfp2DestPort.toUInt();
        }
        rxChannelConfig.ssrc                = receiveAudio.mSSRC.toUInt();
        rxChannelConfig.vlan                = receiveAudio.mVLAN.toUInt();
        rxChannelConfig.payloadType         = receiveAudio.mPayload.toUInt();
        rxChannelConfig.numAudioChannels    = receiveAudio.mNumAudioChannels.toUInt();
        rxChannelConfig.audioPacketInterval = (eNTV2PacketInterval)receiveAudio.mAudioPktInterval.toUInt();

        //dumpRx2110Config (channel, stream, rxChannelConfig);
        bool rv = config2110.EnableRxStream (sfp, channel, stream, rxChannelConfig);
        if (!rv)
        {
            cerr << "EnableRxAudioStream: FAILED: " << config2110.getLastError() << endl;
            return false;
        }
    }

    cerr << "## transmitVideoIter" << endl;
    QListIterator<TransmitStructVideo2110> transmitVideoIter(mKonaIP2110Params.mTransmitVideo2110Channels);

    tx_2110Config txChannelConfig;
    while (transmitVideoIter.hasNext())
    {
        cerr << "## transmitVideoIter did" << endl;

        TransmitStructVideo2110 transmitVideo = transmitVideoIter.next();

        txChannelConfig.init();
        NTV2Channel channel = getChannel(transmitVideo.mChannelDesignator);
        NTV2Stream stream = NTV2_VIDEO_STREAM;

        txChannelConfig.localPort[0] = transmitVideo.mSfp1LocalPort.toUInt();
        txChannelConfig.remoteIP[0]  = transmitVideo.mSfp1RemoteIPAddress.toStdString();
        txChannelConfig.remotePort[0]= transmitVideo.mSfp1RemotePort.toUInt();
        txChannelConfig.localPort[1] = transmitVideo.mSfp2LocalPort.toUInt();
        txChannelConfig.remoteIP[1]  = transmitVideo.mSfp2RemoteIPAddress.toStdString();
        txChannelConfig.remotePort[1]= transmitVideo.mSfp2RemotePort.toUInt();
        txChannelConfig.payloadType  = transmitVideo.mPayload.toUInt();
        txChannelConfig.ssrc         = transmitVideo.mSSRC.toUInt();
        txChannelConfig.ttl          = transmitVideo.mTTL.toUInt();
        txChannelConfig.videoFormat  = CNTV2DemoCommon::GetVideoFormatFromString(transmitVideo.mVideoFormat.toStdString(), allowedVideoFormatTypes);
        txChannelConfig.videoSamples = VPIDSampling_YUV_422;

        config2110.SetTxStreamConfiguration (channel, stream, txChannelConfig);
        config2110.SetTxStreamEnable(channel, stream, getEnable(transmitVideo.mSfp1Enable), getEnable(transmitVideo.mSfp2Enable));
    }

    cerr << "## transmitAudioIter" << endl;
    QListIterator<TransmitStructAudio2110> transmitAudioIter(mKonaIP2110Params.mTransmitAudio2110Channels);

    while (transmitVideoIter.hasNext())
    {
        cerr << "## transmitAudioIter did" << endl;

        TransmitStructAudio2110 transmitAudio = transmitAudioIter.next();

        txChannelConfig.init();
        NTV2Channel channel = getChannel(transmitAudio.mChannelDesignator);

        NTV2Stream stream = NTV2_AUDIO1_STREAM;
        if (transmitAudio.mStream == "audio1")
        {
            stream = NTV2_AUDIO1_STREAM;
        }
        else if (transmitAudio.mStream == "audio2")
        {
            stream = NTV2_AUDIO2_STREAM;
        }
        else if (transmitAudio.mStream == "audio3")
        {
            stream = NTV2_AUDIO3_STREAM;
        }
        else if (transmitAudio.mStream == "audio4")
        {
            stream = NTV2_AUDIO4_STREAM;
        }
        else
        {
            cout << "Error: Tx stream <" << transmitAudio.mStream.toStdString() << "> is invalid" << endl;
        }

        txChannelConfig.localPort[0] = transmitAudio.mSfp1LocalPort.toUInt();
        txChannelConfig.remoteIP[0]  = transmitAudio.mSfp1RemoteIPAddress.toStdString();
        txChannelConfig.remotePort[0]= transmitAudio.mSfp1RemotePort.toUInt();
        txChannelConfig.localPort[1] = transmitAudio.mSfp2LocalPort.toUInt();
        txChannelConfig.remoteIP[1]  = transmitAudio.mSfp2RemoteIPAddress.toStdString();
        txChannelConfig.remotePort[1]= transmitAudio.mSfp2RemotePort.toUInt();
        txChannelConfig.payloadType  = transmitAudio.mPayload.toUInt();
        txChannelConfig.ssrc         = transmitAudio.mSSRC.toUInt();
        txChannelConfig.ttl          = transmitAudio.mTTL.toUInt();

        txChannelConfig.numAudioChannels    = transmitAudio.mNumAudioChannels.toUInt();
        txChannelConfig.firstAudioChannel   = transmitAudio.mFirstAudioChannel.toUInt();
        txChannelConfig.audioPacketInterval = (eNTV2PacketInterval)transmitAudio.mAudioPktInterval.toUInt();

        config2110.SetTxStreamConfiguration (channel, stream, txChannelConfig);
        config2110.SetTxStreamEnable(channel, stream, getEnable(transmitAudio.mSfp1Enable), getEnable(transmitAudio.mSfp2Enable));
    }

    return true;
}

NTV2Channel getChannel(QString channelDesignator)
{
    NTV2Channel channel = NTV2_CHANNEL1;

    if ( channelDesignator == "channel2")
        channel = NTV2_CHANNEL2;
    else if ( channelDesignator == "channel3")
        channel = NTV2_CHANNEL3;
    else if ( channelDesignator == "channel4")
        channel = NTV2_CHANNEL4;

    return channel;
}

bool getEnable(QString enableBoolString)
{
    return (enableBoolString == "true");
}


void CKonaIpJsonSetup::dumpRx2110Config(const NTV2Channel channel, const NTV2Stream stream, rx_2110Config & rxConfig)
{
    qDebug() << "Rx2110Config for channel " << channel << "and stream " << stream;

    qDebug() << "  rxMatch             " << rxConfig.rxMatch;
    qDebug() << "  sourceIP            " << QString::fromStdString(rxConfig.sourceIP);
    qDebug() << "  destIP              " << QString::fromStdString(rxConfig.destIP);
    qDebug() << "  sourcePort          "<< rxConfig.sourcePort;
    qDebug() << "  destPort            "<< rxConfig.destPort;
    qDebug() << "  ssrc                "<< rxConfig.ssrc;
    qDebug() << "  vlan                "<< rxConfig.vlan;
    qDebug() << "  payloadType         "<< rxConfig.payloadType;
    qDebug() << "  videoFormat         "<< rxConfig.videoFormat;
    qDebug() << "  videoSamples        "<< rxConfig.videoSamples;
    qDebug() << "  payloadLen          "<< rxConfig.payloadLen;
    qDebug() << "  lastPayloadLen      "<< rxConfig.lastPayloadLen;
    qDebug() << "  pktsPerLine         "<< rxConfig.pktsPerLine;
    qDebug() << "  numAudioChannels    "<< rxConfig.numAudioChannels;
    qDebug() << "  audioPacketInterval "<< rxConfig.audioPacketInterval << endl;
}
