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

#undef CABLE_ME_SIMPLE

CKonaIpJsonSetup::CKonaIpJsonSetup()
{
    mIs2110       = false;
    mEnable2022_7 = false;
    mNetworkPathDifferential = 50;
}

bool CKonaIpJsonSetup::readJson(const QJsonObject &json)
{
    mKonaIPParams.mSFPs.clear();
    QJsonArray sfpArray = json["sfps"].toArray();
    for (int sfpIndex = 0; sfpIndex < sfpArray.size(); ++sfpIndex)
    {
        cout << "SFP" << endl;

        QJsonObject sfpObject = sfpArray[sfpIndex].toObject();
        SFPStruct sfpStruct;

        sfpStruct.mSFPDesignator = sfpObject["designator"].toString();
        cout << "SFPDesignator " << sfpStruct.mSFPDesignator.toStdString() << endl;

        sfpStruct.mIPAddress = sfpObject["ipAddress"].toString();
        cout << "IPAddress " << sfpStruct.mIPAddress.toStdString() << endl;

        sfpStruct.mSubnetMask = sfpObject["subnetMask"].toString();
        cout << "SubnetMask " << sfpStruct.mSubnetMask.toStdString() << endl;

        sfpStruct.mGateway = sfpObject["gateway"].toString();
        cout << "Gateway " << sfpStruct.mGateway.toStdString() << endl;

        QString enable2022_7 = sfpObject["enable2022_7"].toString();
        if (!enable2022_7.isEmpty())
        {
            cout << "ERROR: enable2022_7 is now moved to global settings" << endl;
            return false;
        }

        cout << endl;
        mKonaIPParams.mSFPs.append(sfpStruct);
    }

    mKonaIPParams.mReceiveChannels.clear();
    QJsonArray receiveChannelArray = json["receive"].toArray();
    for (int receiveChannelIndex = 0; receiveChannelIndex < receiveChannelArray.size(); ++receiveChannelIndex)
    {
        cout << "ReceiveChannels" << endl;

        QJsonObject receiveChannelObject = receiveChannelArray[receiveChannelIndex].toObject();
        ReceiveStruct receiveStruct;

        receiveStruct.mChannelDesignator = receiveChannelObject["designator"].toString();
        cout << "ChannelDesignator " << receiveStruct.mChannelDesignator.toStdString() << endl;

        receiveStruct.mStream = receiveChannelObject["stream"].toString();
        if (!receiveStruct.mStream.isEmpty())
            cout << "Stream " << receiveStruct.mStream.toStdString() << endl;

        receiveStruct.mlinkASrcPort = receiveChannelObject["linkASrcPort"].toString();
        if (!receiveStruct.mlinkASrcPort.isEmpty())
            cout << "LinkASrcPort " << receiveStruct.mlinkASrcPort.toStdString() << endl;

        receiveStruct.mlinkASrcIPAddress = receiveChannelObject["linkASrcIPAddress"].toString();
        if (!receiveStruct.mlinkASrcIPAddress.isEmpty())
            cout << "LinkASrcIPAddress " << receiveStruct.mlinkASrcIPAddress.toStdString() << endl;

        receiveStruct.mlinkADestIPAddress = receiveChannelObject["linkADestIPAddress"].toString();
            cout << "LinkADestIPAddress " << receiveStruct.mlinkADestIPAddress.toStdString() << endl;

        receiveStruct.mlinkADestPort = receiveChannelObject["linkADestPort"].toString();
            cout << "LinkADestPort " << receiveStruct.mlinkADestPort.toStdString() << endl;

        receiveStruct.mlinkAFilter = receiveChannelObject["linkAFilter"].toString();
            cout << "LinkAFilter " << receiveStruct.mlinkAFilter.toStdString() << endl;

        receiveStruct.mlinkBSrcPort = receiveChannelObject["linkBSrcPort"].toString();
        if (!receiveStruct.mlinkBSrcPort.isEmpty())
            cout << "LinkBSrcPort " << receiveStruct.mlinkBSrcPort.toStdString() << endl;

        receiveStruct.mlinkBSrcIPAddress = receiveChannelObject["linkBSrcIPAddress"].toString();
        if (!receiveStruct.mlinkBSrcIPAddress.isEmpty())
            cout << "LinkBSrcIAddress " << receiveStruct.mlinkBSrcIPAddress.toStdString() << endl;

        receiveStruct.mlinkBDestIPAddress = receiveChannelObject["linkBDestIPAddress"].toString();
        if (!receiveStruct.mlinkBDestIPAddress.isEmpty())
            cout << "LinkBDestIPAddress " << receiveStruct.mlinkBDestIPAddress.toStdString() << endl;

        receiveStruct.mlinkBDestPort = receiveChannelObject["linkBDestPort"].toString();
        if (!receiveStruct.mlinkBDestPort.isEmpty())
            cout << "LinkBDestPort " << receiveStruct.mlinkBDestPort.toStdString() << endl;

        receiveStruct.mlinkBFilter = receiveChannelObject["linkBFilter"].toString();
        if (!receiveStruct.mlinkBFilter.isEmpty())
            cout << "LinkBFilter " << receiveStruct.mlinkBFilter.toStdString() << endl;

        QString networkPathDifferential = receiveChannelObject["networkPathDifferential"].toString();
        if (!networkPathDifferential.isEmpty())
        {
            cout << "ERROR: networkPathDifferential is now part of global settings" << endl;
            return false;
        }

        receiveStruct.mPlayoutDelay = receiveChannelObject["playoutDelay"].toString();
        if (!receiveStruct.mPlayoutDelay.isEmpty())
            cout << "PlayoutDelay " << receiveStruct.mPlayoutDelay.toStdString() << endl;

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

        receiveStruct.mPayloadLen = receiveChannelObject["payloadLen"].toString();
        if (!receiveStruct.mPayloadLen.isEmpty())
            cout << "Payload Len " << receiveStruct.mPayloadLen.toStdString() << endl;

        receiveStruct.mLastPayloadLen = receiveChannelObject["lastPayloadLen"].toString();
        if (!receiveStruct.mLastPayloadLen.isEmpty())
            cout << "Last Payload Len " << receiveStruct.mLastPayloadLen.toStdString() << endl;

        receiveStruct.mNumAudioChannels = receiveChannelObject["numAudioChannels"].toString();
        if (!receiveStruct.mNumAudioChannels.isEmpty())
            cout << "Number of Audio Channels " << receiveStruct.mNumAudioChannels.toStdString() << endl;

        receiveStruct.mAudioPktInterval = receiveChannelObject["audioPktInterval"].toString();
        if (!receiveStruct.mAudioPktInterval.isEmpty())
            cout << "Audio Packet Interval " << receiveStruct.mAudioPktInterval.toStdString() << endl;

        receiveStruct.mPktsPerLine = receiveChannelObject["pktsPerLine"].toString();
        if (!receiveStruct.mPktsPerLine.isEmpty())
            cout << "Packets per line " << receiveStruct.mPktsPerLine.toStdString() << endl;

        receiveStruct.mLinkAEnable = receiveChannelObject["linkAEnable"].toString();
        if (!receiveStruct.mLinkAEnable.isEmpty())
            cout << "Link A Enable " << receiveStruct.mLinkAEnable.toStdString() << endl;

        receiveStruct.mLinkBEnable = receiveChannelObject["linkBEnable"].toString();
        if (!receiveStruct.mLinkBEnable.isEmpty())
            cout << "Link B Enable " << receiveStruct.mLinkBEnable.toStdString() << endl;

        receiveStruct.mEnable = receiveChannelObject["enable"].toString();
        cout << "Enable " << receiveStruct.mEnable.toStdString() << endl << endl;

        mKonaIPParams.mReceiveChannels.append(receiveStruct);
    }

    mKonaIPParams.mTransmitChannels.clear();
    QJsonArray transmitChannelArray = json["transmit"].toArray();
    for (int transmitChannelIndex = 0; transmitChannelIndex < transmitChannelArray.size(); ++transmitChannelIndex)
    {
        cout << "TransmitChannels" << endl;

        QJsonObject transmitChannelObject = transmitChannelArray[transmitChannelIndex].toObject();
        TransmitStruct transmitStruct;

        transmitStruct.mChannelDesignator = transmitChannelObject["designator"].toString();
        cout << "ChannelDesignator " << transmitStruct.mChannelDesignator.toStdString() << endl;

        transmitStruct.mStream = transmitChannelObject["stream"].toString();
        if (!transmitStruct.mStream.isEmpty())
            cout << "Stream " << transmitStruct.mStream.toStdString() << endl;

        transmitStruct.mlinkALocalPort = transmitChannelObject["linkALocalPort"].toString();
            cout << "LinkALocalPort " << transmitStruct.mlinkALocalPort.toStdString() << endl;

        transmitStruct.mlinkARemoteIPAddress = transmitChannelObject["linkARemoteIPAddress"].toString();
            cout << "LinkARemoteIPAddress " << transmitStruct.mlinkARemoteIPAddress.toStdString() << endl;

        transmitStruct.mlinkARemotePort = transmitChannelObject["linkARemotePort"].toString();
            cout << "LinkARemotePort " << transmitStruct.mlinkARemotePort.toStdString() << endl;

        transmitStruct.mlinkBLocalPort = transmitChannelObject["linkBLocalPort"].toString();
        if (!transmitStruct.mlinkBLocalPort.isEmpty())
            cout << "LinkBLocalPort " << transmitStruct.mlinkBLocalPort.toStdString() << endl;

        transmitStruct.mlinkBRemoteIPAddress = transmitChannelObject["linkBRemoteIPAddress"].toString();
        if (!transmitStruct.mlinkBRemoteIPAddress.isEmpty())
            cout << "LinkBRemoteIPAddress " << transmitStruct.mlinkBRemoteIPAddress.toStdString() << endl;

        transmitStruct.mlinkBRemotePort = transmitChannelObject["linkBRemotePort"].toString();
        if (!transmitStruct.mlinkBRemoteIPAddress.isEmpty())
            cout << "LinkBRemotePort " << transmitStruct.mlinkBRemotePort.toStdString() << endl;

        transmitStruct.mSSRC = transmitChannelObject["ssrc"].toString();
        if (!transmitStruct.mSSRC.isEmpty())
            cout << "SSRC " << transmitStruct.mSSRC.toStdString() << endl;

        transmitStruct.mTOS = transmitChannelObject["tos"].toString();
        if (!transmitStruct.mTOS.isEmpty())
            cout << "TOS " << transmitStruct.mTOS.toStdString() << endl;

        transmitStruct.mTTL = transmitChannelObject["ttl"].toString();
        if (!transmitStruct.mTTL.isEmpty())
            cout << "TTL " << transmitStruct.mTTL.toStdString() << endl;

        transmitStruct.mPayload = transmitChannelObject["payload"].toString();
        if (!transmitStruct.mPayload.isEmpty())
            cout << "Payload " << transmitStruct.mPayload.toStdString() << endl;

        transmitStruct.mVideoFormat = transmitChannelObject["videoFormat"].toString();
        if (!transmitStruct.mVideoFormat.isEmpty())
            cout << "Video format " << transmitStruct.mVideoFormat.toStdString() << endl;

        transmitStruct.mPayloadLen = transmitChannelObject["payloadLen"].toString();
        if (!transmitStruct.mPayloadLen.isEmpty())
            cout << "Payload Len " << transmitStruct.mPayloadLen.toStdString() << endl;

        transmitStruct.mLastPayloadLen = transmitChannelObject["lastPayloadLen"].toString();
        if (!transmitStruct.mLastPayloadLen.isEmpty())
            cout << "Last Payload Len " << transmitStruct.mLastPayloadLen.toStdString() << endl;

        transmitStruct.mPktsPerLine = transmitChannelObject["pktsPerLine"].toString();
        if (!transmitStruct.mPktsPerLine.isEmpty())
            cout << "Packets per line " << transmitStruct.mPktsPerLine.toStdString() << endl;

        transmitStruct.mNumAudioChannels = transmitChannelObject["numAudioChannels"].toString();
        if (!transmitStruct.mNumAudioChannels.isEmpty())
            cout << "Number of Audio Channels " << transmitStruct.mNumAudioChannels.toStdString() << endl;

        transmitStruct.mFirstAudioChannel = transmitChannelObject["firstAudioChannel"].toString();
        if (!transmitStruct.mFirstAudioChannel.isEmpty())
            cout << "First Audio Channel " << transmitStruct.mFirstAudioChannel.toStdString() << endl;

        transmitStruct.mAudioPktInterval = transmitChannelObject["audioPktInterval"].toString();
        if (!transmitStruct.mAudioPktInterval.isEmpty())
            cout << "Audio Packet Interval " << transmitStruct.mAudioPktInterval.toStdString() << endl;

        transmitStruct.mLinkAEnable = transmitChannelObject["linkAEnable"].toString();
        if (!transmitStruct.mLinkAEnable.isEmpty())
            cout << "Link A Enable " << transmitStruct.mLinkAEnable.toStdString() << endl;

        transmitStruct.mLinkBEnable = transmitChannelObject["linkBEnable"].toString();
        if (!transmitStruct.mLinkBEnable.isEmpty())
            cout << "Link B Enable " << transmitStruct.mLinkBEnable.toStdString() << endl;

        transmitStruct.mEnable = transmitChannelObject["enable"].toString();
        cout << "Enable " << transmitStruct.mEnable.toStdString() << endl << endl;

        mKonaIPParams.mTransmitChannels.append(transmitStruct);
    }
    return true;
}

bool CKonaIpJsonSetup::openJson(QString fileName)
{
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
        qjv = json.value("networkPathDifferential");
        if (qjv != QJsonValue::Undefined)
        {
            mNetworkPathDifferential = qjv.toString().toUInt();
        }
        else
        {
            mNetworkPathDifferential = 50;
        }
    }

    return readJson(json);
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

    if (mKonaIPParams.mSFPs.size() < 1)
    {
        {
            cerr << "## ERROR:  Need To Specify at Least 1 SFP" << endl;
            return false;
        }
    }

    QListIterator<SFPStruct> sfpIter(mKonaIPParams.mSFPs);
    while (sfpIter.hasNext())
    {
        SFPStruct sfp = sfpIter.next();
        if ( sfp.mSFPDesignator == "linkA")
        {
            bool rv = config2022.SetNetworkConfiguration (SFP_LINK_A,
                                                          sfp.mIPAddress.toStdString(),
                                                          sfp.mSubnetMask.toStdString(),
                                                          sfp.mGateway.toStdString());
            if (!rv)
            {
                cerr << "Error: " << config2022.getLastError() << endl;
                return false;
            }
        }
        else if ( sfp.mSFPDesignator == "linkB")
        {
            bool rv = config2022.SetNetworkConfiguration (SFP_LINK_B,
                                                          sfp.mIPAddress.toStdString(),
                                                          sfp.mSubnetMask.toStdString(),
                                                          sfp.mGateway.toStdString());
            if (!rv)
            {
                cerr << "Error: " << config2022.getLastError() << endl;
                return false;
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

    QListIterator<ReceiveStruct> receiveIter(mKonaIPParams.mReceiveChannels);
    while (receiveIter.hasNext())
    {
        cerr << "## receiveIter did" << endl;

        ReceiveStruct receive = receiveIter.next();
        rx_2022_channel rxChannelConfig;
        bool ok;
        NTV2Channel channel                 = getChannel(receive.mChannelDesignator);

        if (mEnable2022_7)
        {
            rxChannelConfig.linkAEnable = true;
            rxChannelConfig.linkBEnable = true;
        }
        else
        {
            if (!receive.mLinkAEnable.isEmpty())
                rxChannelConfig.linkAEnable = (getEnable(receive.mLinkAEnable));
            if (!receive.mLinkBEnable.isEmpty())
                rxChannelConfig.linkBEnable = (getEnable(receive.mLinkBEnable));
        }
        rxChannelConfig.primarySourceIP     = receive.mlinkASrcIPAddress.toStdString();
        rxChannelConfig.primarySourcePort   = receive.mlinkASrcPort.toUInt();
        rxChannelConfig.primaryDestIP       = receive.mlinkADestIPAddress.toStdString();
        rxChannelConfig.primaryDestPort     = receive.mlinkADestPort.toUInt();
        rxChannelConfig.primaryRxMatch      = receive.mlinkAFilter.toUInt(&ok, 16);
        rxChannelConfig.secondarySourceIP   = receive.mlinkBSrcIPAddress.toStdString();
        rxChannelConfig.secondarySourcePort = receive.mlinkBSrcPort.toUInt();
        rxChannelConfig.secondaryDestIP     = receive.mlinkBDestIPAddress.toStdString();
        rxChannelConfig.secondaryDestPort   = receive.mlinkBDestPort.toUInt();
        rxChannelConfig.secondaryRxMatch    = receive.mlinkBFilter.toUInt(&ok, 16);
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

    QListIterator<TransmitStruct> transmitIter(mKonaIPParams.mTransmitChannels);
    while (transmitIter.hasNext())
    {
        cerr << "## transmitIter did" << endl;

        TransmitStruct transmit = transmitIter.next();
        tx_2022_channel txChannelConfig;

        NTV2Channel channel                 = getChannel(transmit.mChannelDesignator);
        if (mEnable2022_7)
        {
            txChannelConfig.linkAEnable = true;
            txChannelConfig.linkBEnable = true;
        }
        else
        {
            if (!transmit.mLinkAEnable.isEmpty())
                txChannelConfig.linkAEnable = (getEnable(transmit.mLinkAEnable));
            if (!transmit.mLinkBEnable.isEmpty())
                txChannelConfig.linkBEnable = (getEnable(transmit.mLinkBEnable));
        }
        txChannelConfig.primaryLocalPort    = transmit.mlinkALocalPort.toUInt();
        txChannelConfig.primaryRemoteIP     = transmit.mlinkARemoteIPAddress.toStdString();
        txChannelConfig.primaryRemotePort   = transmit.mlinkARemotePort.toUInt();
        txChannelConfig.secondaryLocalPort  = transmit.mlinkBLocalPort.toUInt();
        txChannelConfig.secondaryRemoteIP   = transmit.mlinkBRemoteIPAddress.toStdString();
        txChannelConfig.secondaryRemotePort = transmit.mlinkBRemotePort.toUInt();
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


#ifdef CABLE_ME_SIMPLE
    //config2110.SetIGMPVersion(eIGMPVersion_2);
#endif

    if (!mPTPMasterAddr.isEmpty())
    {
        device.SetReference(NTV2_REFERENCE_SFP1_PTP);
        config2110.SetPTPMaster(mPTPMasterAddr.toStdString());
    }

    config2110.Set2110_4K_Mode(m4KMode);

    if (mKonaIPParams.mSFPs.size() < 1)
    {
        cerr << "## ERROR:  Need To Specify at Least 1 SFP" << endl;
        return false;
    }

    QListIterator<SFPStruct> sfpIter(mKonaIPParams.mSFPs);
    while (sfpIter.hasNext())
    {
        SFPStruct sfp = sfpIter.next();

        if ( sfp.mSFPDesignator == "linkA")
        {
            config2110.SetNetworkConfiguration (SFP_LINK_A, sfp.mIPAddress.toStdString(), sfp.mSubnetMask.toStdString());
        }
        else if ( sfp.mSFPDesignator == "linkB")
        {
            config2110.SetNetworkConfiguration (SFP_LINK_B, sfp.mIPAddress.toStdString(), sfp.mSubnetMask.toStdString());

        }
    }

    cerr << "## receiveIter" << endl;
    QListIterator<ReceiveStruct> receiveIter(mKonaIPParams.mReceiveChannels);

#ifdef CABLE_ME_SIMPLE
    if (receiveIter.hasNext())
    {
        device.Connect(NTV2_XptHDMIOutQ1Input, NTV2_XptSDIIn1);
        device.Connect(NTV2_XptFrameBuffer2Input,NTV2_XptSDIIn1);
        device.SetFrameBufferFormat (NTV2_CHANNEL2, NTV2_FBF_10BIT_YCBCR);

        // audio
        device.SetAudioSystemInputSource (NTV2_AUDIOSYSTEM_1, NTV2_AUDIO_EMBEDDED, NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1);
        device.SetNumberAudioChannels (8);
        device.SetAudioRate (NTV2_AUDIO_48K, NTV2_AUDIOSYSTEM_1);
        device.SetAudioBufferSize (NTV2_AUDIO_BUFFER_BIG, NTV2_AUDIOSYSTEM_1);
        device.SetAudioLoopBack (NTV2_AUDIO_LOOPBACK_ON, NTV2_AUDIOSYSTEM_1);
        device.WriteRegister(kRegAudioOutputSourceMap,0);
    }
#endif

    rx_2110Config rxChannelConfig;
    while (receiveIter.hasNext())
    {
        cerr << "## receiveIter did" << endl;

        ReceiveStruct receive = receiveIter.next();

        bool ok;
        NTV2Channel channel = getChannel(receive.mChannelDesignator);
        NTV2Stream stream;
        if (receive.mStream == "audio1")
        {
            stream = NTV2_AUDIO1_STREAM;
        }
        else if (receive.mStream == "video")
        {
            stream = NTV2_VIDEO_STREAM;
        }
        else
        {
            cerr << "Invalid RX Stream: " << receive.mStream.toStdString() << endl;
            return false;
        }

        // If linkB is on use it otherwise assume we are always dealing with linkA
        // (RX does not support both at the moment)
        bool linkBOn = false;
        if (!receive.mLinkBEnable.isEmpty())
            linkBOn = getEnable(receive.mLinkBEnable);

        eSFP link;
        if (linkBOn)
            link = SFP_LINK_B;
        else
            link = SFP_LINK_A;

        bool enable = getEnable(receive.mEnable);
        if (!enable)
        {
            bool rv = config2110.DisableRxStream (link, channel, stream);
            if (!rv)
            {
                cerr << "DisableRxStream: FAILED: " << config2110.getLastError() << endl;
                return false;
            }
            else
                cout << "DisableRxStream: ok" << endl;
        }

        rxChannelConfig.rxMatch      = receive.mlinkAFilter.toUInt(&ok, 16);
        rxChannelConfig.sourceIP     = receive.mlinkASrcIPAddress.toStdString();
        rxChannelConfig.destIP       = receive.mlinkADestIPAddress.toStdString();
        rxChannelConfig.sourcePort   = receive.mlinkASrcPort.toUInt();
        rxChannelConfig.destPort     = receive.mlinkADestPort.toUInt();
        rxChannelConfig.SSRC         = receive.mSSRC.toUInt();
        rxChannelConfig.VLAN         = receive.mVLAN.toUInt();
        rxChannelConfig.payloadType  = receive.mPayload.toUInt();
        rxChannelConfig.videoFormat  = CNTV2DemoCommon::GetVideoFormatFromString(receive.mVideoFormat.toStdString(), allowedVideoFormatTypes);
        rxChannelConfig.videoSamples = VPIDSampling_YUV_422;

        if (!receive.mPayloadLen.isEmpty())
            rxChannelConfig.payloadLen = receive.mPayloadLen.toUInt();
        if (!receive.mLastPayloadLen.isEmpty())
            rxChannelConfig.lastPayloadLen = receive.mLastPayloadLen.toUInt();
        if (!receive.mPktsPerLine.isEmpty())
            rxChannelConfig.pktsPerLine = receive.mPktsPerLine.toUInt();
        if (!receive.mNumAudioChannels.isEmpty())
            rxChannelConfig.numAudioChannels  = receive.mNumAudioChannels.toUInt();
        if (!receive.mAudioPktInterval.isEmpty())
            rxChannelConfig.audioPacketInterval = (receive.mAudioPktInterval.toUInt() == 1000) ? PACKET_INTERVAL_1mS :  PACKET_INTERVAL_125uS;

        //dumpRx2110Config (channel, stream, rxChannelConfig);
        bool rv = config2110.EnableRxStream (link, channel, stream, rxChannelConfig);
        if (!rv)
        {
            cerr << "EnableRxStream: FAILED: " << config2110.getLastError() << endl;
            return false;
        }
        else
            cout << "EnableRxStream: ok" << endl;
    }

    cerr << "## transmitIter" << endl;

    QListIterator<TransmitStruct> transmitIter(mKonaIPParams.mTransmitChannels);

#ifdef CABLE_ME_SIMPLE
    if (transmitIter.hasNext())
    {
        // video
        device.SetEveryFrameServices(NTV2_OEM_TASKS);
        device.Connect(NTV2_XptSDIOut1Input,NTV2_XptFrameBuffer1YUV);
    }
#endif

    while (transmitIter.hasNext())
    {
        cerr << "## transmitIter did" << endl;

        TransmitStruct transmit = transmitIter.next();
        tx_2110Config txChannelConfig;

        NTV2Channel channel          = getChannel(transmit.mChannelDesignator);
        txChannelConfig.localPort[0] = transmit.mlinkALocalPort.toUInt();
        txChannelConfig.remoteIP[0]  = transmit.mlinkARemoteIPAddress.toStdString();
        txChannelConfig.remotePort[0]= transmit.mlinkARemotePort.toUInt();
        txChannelConfig.localPort[1] = transmit.mlinkBLocalPort.toUInt();
        txChannelConfig.remoteIP[1]  = transmit.mlinkBRemoteIPAddress.toStdString();
        txChannelConfig.remotePort[1]= transmit.mlinkBRemotePort.toUInt();
        txChannelConfig.payloadType  = transmit.mPayload.toUInt();
        txChannelConfig.ssrc         = transmit.mSSRC.toUInt();
        txChannelConfig.tos          = transmit.mTOS.toUInt();
        txChannelConfig.ttl          = transmit.mTTL.toUInt();
        txChannelConfig.videoFormat  = CNTV2DemoCommon::GetVideoFormatFromString(transmit.mVideoFormat.toStdString(), allowedVideoFormatTypes);
        txChannelConfig.videoSamples = VPIDSampling_YUV_422;

        if (!transmit.mNumAudioChannels.isEmpty())
            txChannelConfig.numAudioChannels  = transmit.mNumAudioChannels.toUInt();
        if (!transmit.mFirstAudioChannel.isEmpty())
            txChannelConfig.firstAudioChannel = transmit.mFirstAudioChannel.toUInt();
        if (!transmit.mAudioPktInterval.isEmpty())
            txChannelConfig.audioPacketInterval = (transmit.mAudioPktInterval.toUInt() == 1000) ? PACKET_INTERVAL_1mS :  PACKET_INTERVAL_125uS;

        if (!transmit.mPayloadLen.isEmpty())
            txChannelConfig.payloadLen = transmit.mPayloadLen.toUInt();
        if (!transmit.mLastPayloadLen.isEmpty())
            txChannelConfig.lastPayLoadLen = transmit.mLastPayloadLen.toUInt();
        if (!transmit.mPktsPerLine.isEmpty())
            txChannelConfig.pktsPerLine = transmit.mPktsPerLine.toUInt();

        NTV2Stream stream;
        if (transmit.mStream == "video")
        {
            stream = NTV2_VIDEO_STREAM;
        }
        else if (transmit.mStream == "audio1")
        {
            stream = NTV2_AUDIO1_STREAM;
        }
        else if (transmit.mStream == "audio2")
        {
            stream = NTV2_AUDIO2_STREAM;
        }
        else if (transmit.mStream == "audio3")
        {
            stream = NTV2_AUDIO3_STREAM;
        }
        else if (transmit.mStream == "audio4")
        {
            stream = NTV2_AUDIO4_STREAM;
        }
        else
        {
            cout << "Error: Tx stream <" << transmit.mStream.toStdString() << "> is invalid" << endl;
            continue;
        }

        config2110.SetTxChannelConfiguration (channel, stream, txChannelConfig);
        config2110.SetTxChannelEnable(channel, stream, getEnable(transmit.mLinkAEnable), getEnable(transmit.mLinkBEnable));
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
    qDebug() << "  SSRC                "<< rxConfig.SSRC;
    qDebug() << "  VLAN                "<< rxConfig.VLAN;
    qDebug() << "  payloadType         "<< rxConfig.payloadType;
    qDebug() << "  videoFormat         "<< rxConfig.videoFormat;
    qDebug() << "  videoSamples        "<< rxConfig.videoSamples;
    qDebug() << "  payloadLen          "<< rxConfig.payloadLen;
    qDebug() << "  lastPayloadLen      "<< rxConfig.lastPayloadLen;
    qDebug() << "  pktsPerLine         "<< rxConfig.pktsPerLine;
    qDebug() << "  numAudioChannels    "<< rxConfig.numAudioChannels;
    qDebug() << "  audioPacketInterval "<< rxConfig.audioPacketInterval << endl;
}
