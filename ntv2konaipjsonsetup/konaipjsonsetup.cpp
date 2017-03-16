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
#include "ntv2config2022.h"
#include "ntv2config2110.h"

NTV2Channel getChannel(QString channelDesignator);
bool getEnable(QString enableBoolString);
MACAddr toMAC(QString mac);

CKonaIpJsonSetup::CKonaIpJsonSetup()
{
    is2110       = false;
    enable2022_7 = false;
}

bool CKonaIpJsonSetup::readJson(const QJsonObject &json)
{
    mKonaIPParams.mSFPs.clear();
    QJsonArray sfpArray = json["sfps"].toArray();
    for (int sfpIndex = 0; sfpIndex < sfpArray.size(); ++sfpIndex)
    {
        std::cout << "SFP" << std::endl;

        QJsonObject sfpObject = sfpArray[sfpIndex].toObject();
        SFPStruct sfpStruct;
        sfpStruct.mSFPDesignator = sfpObject["designator"].toString();
        std::cout << "SFPDesignator " << sfpStruct.mSFPDesignator.toStdString() << std::endl;

        sfpStruct.mIPAddress = sfpObject["IPAddress"].toString();
        std::cout << "IPAddress " << sfpStruct.mIPAddress.toStdString() << std::endl;

        sfpStruct.mSubnetMask = sfpObject["SubnetMask"].toString();
        std::cout << "SubnetMask " << sfpStruct.mSubnetMask.toStdString() << std::endl;

        sfpStruct.mRouter = sfpObject["Router"].toString();
        std::cout << "Router " << sfpStruct.mRouter.toStdString() << std::endl;

        sfpStruct.mEnable2022_7 = sfpObject["Enable2022_7"].toString();
        if (!sfpStruct.mEnable2022_7.isEmpty())
            std::cout << "Enable2022_7 " << sfpStruct.mEnable2022_7.toStdString() << std::endl << std::endl;
        else
            std::cout << std::endl;

        mKonaIPParams.mSFPs.append(sfpStruct);
    }

    mKonaIPParams.mReceiveChannels.clear();
    QJsonArray receiveChannelArray = json["receive"].toArray();
    for (int receiveChannelIndex = 0; receiveChannelIndex < receiveChannelArray.size(); ++receiveChannelIndex)
    {
        std::cout << "ReceiveChannels" << std::endl;

        QJsonObject receiveChannelObject = receiveChannelArray[receiveChannelIndex].toObject();
        ReceiveStruct receiveStruct;

        receiveStruct.mChannelDesignator = receiveChannelObject["designator"].toString();
        std::cout << "ChannelDesignator " << receiveStruct.mChannelDesignator.toStdString() << std::endl;

        receiveStruct.mStream = receiveChannelObject["stream"].toString();
        if (!receiveStruct.mStream.isEmpty())
            std::cout << "Stream " << receiveStruct.mStream.toStdString() << std::endl;

        receiveStruct.mSrcPort = receiveChannelObject["srcPort"].toString();
        if (!receiveStruct.mSrcPort.isEmpty())
            std::cout << "Src Port " << receiveStruct.mSrcPort.toStdString() << std::endl;

        receiveStruct.mSrcIPAddress = receiveChannelObject["srcIPAddress"].toString();
        if (!receiveStruct.mSrcIPAddress.isEmpty())
            std::cout << "Src IP Address " << receiveStruct.mSrcIPAddress.toStdString() << std::endl;

        receiveStruct.mPrimaryDestIPAddress = receiveChannelObject["primaryDestIPAddress"].toString();
        std::cout << "PrimaryDestIPAddress " << receiveStruct.mPrimaryDestIPAddress.toStdString() << std::endl;

        receiveStruct.mPrimaryDestPort = receiveChannelObject["primaryDestPort"].toString();
        std::cout << "PrimaryDestPort " << receiveStruct.mPrimaryDestPort.toStdString() << std::endl;

        receiveStruct.mPrimaryFilter = receiveChannelObject["primaryFilter"].toString();
        std::cout << "PrimaryFilter " << receiveStruct.mPrimaryFilter.toStdString() << std::endl;

        receiveStruct.mSecondaryDestIPAddress = receiveChannelObject["secondaryDestIPAddress"].toString();
        if (!receiveStruct.mSecondaryDestIPAddress.isEmpty())
            std::cout << "SecondaryDestIPAddress " << receiveStruct.mSecondaryDestIPAddress.toStdString() << std::endl;

        receiveStruct.mSecondaryDestPort = receiveChannelObject["secondaryDestPort"].toString();
        if (!receiveStruct.mSecondaryDestPort.isEmpty())
            std::cout << "SecondaryDestPort " << receiveStruct.mSecondaryDestPort.toStdString() << std::endl;

        receiveStruct.mSecondaryFilter = receiveChannelObject["secondaryFilter"].toString();
        if (!receiveStruct.mSecondaryFilter.isEmpty())
            std::cout << "SecondaryFilter " << receiveStruct.mSecondaryFilter.toStdString() << std::endl;

        receiveStruct.mNetworkPathDifferential = receiveChannelObject["networkPathDifferential"].toString();
        if (!receiveStruct.mNetworkPathDifferential.isEmpty())
            std::cout << "NetworkPathDifferential " << receiveStruct.mNetworkPathDifferential.toStdString() << std::endl;

        receiveStruct.mPlayoutDelay = receiveChannelObject["playoutDelay"].toString();
        if (!receiveStruct.mPlayoutDelay.isEmpty())
            std::cout << "PlayoutDelay " << receiveStruct.mPlayoutDelay.toStdString() << std::endl;

        receiveStruct.mVLAN = receiveChannelObject["vlan"].toString();
        if (!receiveStruct.mVLAN.isEmpty())
            std::cout << "VLAN " << receiveStruct.mVLAN.toStdString() << std::endl;

        receiveStruct.mSSRC = receiveChannelObject["ssrc"].toString();
        if (!receiveStruct.mSSRC.isEmpty())
            std::cout << "SSRC " << receiveStruct.mSSRC.toStdString() << std::endl;

        receiveStruct.mPayload = receiveChannelObject["payload"].toString();
        if (!receiveStruct.mPayload.isEmpty())
            std::cout << "Payload " << receiveStruct.mPayload.toStdString() << std::endl;

        receiveStruct.mVideoFormat = receiveChannelObject["videoFormat"].toString();
        if (!receiveStruct.mVideoFormat.isEmpty())
            std::cout << "Video Format " << receiveStruct.mVideoFormat.toStdString() << std::endl;

        receiveStruct.mEnable = receiveChannelObject["Enable"].toString();
        std::cout << "Enable " << receiveStruct.mEnable.toStdString() << std::endl << std::endl;

        mKonaIPParams.mReceiveChannels.append(receiveStruct);
    }

    mKonaIPParams.mTransmitChannels.clear();
    QJsonArray transmitChannelArray = json["transmit"].toArray();
    for (int transmitChannelIndex = 0; transmitChannelIndex < transmitChannelArray.size(); ++transmitChannelIndex)
    {
        std::cout << "TransmitChannels" << std::endl;

        QJsonObject transmitChannelObject = transmitChannelArray[transmitChannelIndex].toObject();
        TransmitStruct transmitStruct;

        transmitStruct.mChannelDesignator = transmitChannelObject["designator"].toString();
        std::cout << "ChannelDesignator " << transmitStruct.mChannelDesignator.toStdString() << std::endl;

        transmitStruct.mStream = transmitChannelObject["stream"].toString();
        if (!transmitStruct.mStream.isEmpty())
            std::cout << "Stream " << transmitStruct.mStream.toStdString() << std::endl;

        transmitStruct.mPrimaryLocalPort = transmitChannelObject["primaryLocalPort"].toString();
        std::cout << "PrimaryLocalPort " << transmitStruct.mPrimaryLocalPort.toStdString() << std::endl;

        transmitStruct.mPrimaryRemoteIPAddress = transmitChannelObject["primaryRemoteIPAddress"].toString();
        std::cout << "PrimaryRemoteIPAddress " << transmitStruct.mPrimaryRemoteIPAddress.toStdString() << std::endl;

        transmitStruct.mPrimaryRemotePort = transmitChannelObject["primaryRemotePort"].toString();
        std::cout << "PrimaryRemotePort " << transmitStruct.mPrimaryRemotePort.toStdString() << std::endl;

        transmitStruct.mPrimaryRemoteMac = transmitChannelObject["primaryRemoteMac"].toString();
        std::cout << "PrimaryRemoteMac " << transmitStruct.mPrimaryRemoteMac.toStdString() << std::endl;

        transmitStruct.mPrimaryAutoMac = transmitChannelObject["primaryAutoMac"].toString();
        std::cout << "PrimaryAutoMac " << transmitStruct.mPrimaryAutoMac.toStdString() << std::endl;

        transmitStruct.mSecondaryLocalPort = transmitChannelObject["secondaryLocalPort"].toString();
        if (!transmitStruct.mSecondaryLocalPort.isEmpty())
            std::cout << "SecondaryLocalPort " << transmitStruct.mSecondaryLocalPort.toStdString() << std::endl;

        transmitStruct.mSecondaryRemoteIPAddress = transmitChannelObject["secondaryRemoteIPAddress"].toString();
        if (!transmitStruct.mSecondaryRemoteIPAddress.isEmpty())
            std::cout << "SecondaryRemoteIPAddress " << transmitStruct.mSecondaryRemoteIPAddress.toStdString() << std::endl;

        transmitStruct.mSecondaryRemoteIPAddress = transmitChannelObject["secondaryRemotePort"].toString();
        if (!transmitStruct.mSecondaryRemoteIPAddress.isEmpty())
        std::cout << "SecondaryRemotePort " << transmitStruct.mSecondaryRemotePort.toStdString() << std::endl;

        transmitStruct.mSecondaryRemoteMac = transmitChannelObject["secondaryRemoteMac"].toString();
        if (!transmitStruct.mSecondaryRemoteMac.isEmpty())
            std::cout << "SecondaryRemoteMac " << transmitStruct.mSecondaryRemoteMac.toStdString() << std::endl;

        transmitStruct.mSecondaryAutoMac = transmitChannelObject["secondaryAutoMac"].toString();
        if (!transmitStruct.mSecondaryAutoMac.isEmpty())
            std::cout << "SecondaryAutoMac " << transmitStruct.mSecondaryAutoMac.toStdString() << std::endl;

        transmitStruct.mVideoFormat = transmitChannelObject["videoFormat"].toString();
        if (!transmitStruct.mVideoFormat.isEmpty())
            std::cout << "Video format " << transmitStruct.mVideoFormat.toStdString() << std::endl;

        transmitStruct.mEnable = transmitChannelObject["Enable"].toString();
        std::cout << "Enable " << transmitStruct.mEnable.toStdString() << std::endl << std::endl;

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
            is2110 = true;
            std::cout << "Protocol 2110 " << std::endl;
        }
        else
            std::cout << "Protocol 2022 " << std::endl;

    }

    qjv = json.value("PTPMaster");
    if (qjv != QJsonValue::Undefined)
    {
       PTPMasterAddr = qjv.toString();
       std::cout << "PTP Master Address " << PTPMasterAddr.toStdString() << std::endl;
    }

    return readJson(json);
}

bool CKonaIpJsonSetup::setupBoard(std::string deviceSpec)
{
    if (is2110)
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
        {std::cerr << "## ERROR:  No devices found " << deviceSpec.c_str() << std::endl;  return false;}
    //if (!mDevice.IsKonaIPDevice ())
    //    {std::cerr << "## ERROR:  Not a KONA IP device" << std::endl;  return false;}

    //	Read MicroBlaze Uptime in Seconds, to see if it's running...
    while (!mDevice.IsDeviceReady ())
    {
        std::cout << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << std::endl;
        mDevice.SleepMs (1000);
        if (mDevice.IsDeviceReady ())
            std::cout << "## NOTE:  Device is ready" << std::endl;
    }

    enable2022_7 = false;
    CNTV2Config2022	config2022 (mDevice);

    if (mKonaIPParams.mSFPs.size() < 1)
    {
        {std::cerr << "## ERROR:  Need To Specify at Least 1 SFP" << std::endl;  return false;}
    }

    QListIterator<SFPStruct> sfpIter(mKonaIPParams.mSFPs);
    while (sfpIter.hasNext())
    {
        SFPStruct sfp = sfpIter.next();

        if ( sfp.mSFPDesignator == "top")
        {
            config2022.SetNetworkConfiguration (SFP_TOP,    sfp.mIPAddress.toStdString(), sfp.mSubnetMask.toStdString());
            enable2022_7 = getEnable(sfp.mEnable2022_7);
        }
        else
        if ( sfp.mSFPDesignator == "bottom")
        {
            config2022.SetNetworkConfiguration (SFP_BOTTOM,    sfp.mIPAddress.toStdString(), sfp.mSubnetMask.toStdString());

        }
    }

    std::cerr << "## receiveIter" << std::endl;

    QListIterator<ReceiveStruct> receiveIter(mKonaIPParams.mReceiveChannels);
    while (receiveIter.hasNext())
    {
        std::cerr << "## receiveIter did" << std::endl;

        ReceiveStruct receive = receiveIter.next();
        rx_2022_channel rxChannelConfig;
        bool ok;
        NTV2Channel channel = getChannel(receive.mChannelDesignator);
        rxChannelConfig.primaryDestIP = receive.mPrimaryDestIPAddress.toStdString();
        rxChannelConfig.primaryDestPort = receive.mPrimaryDestPort.toUInt();
        rxChannelConfig.primaryRxMatch = receive.mPrimaryFilter.toUInt(&ok, 16);
        rxChannelConfig.secondaryDestIP = receive.mSecondaryDestIPAddress.toStdString();
        rxChannelConfig.secondaryDestPort = receive.mSecondaryDestPort.toUInt();
        rxChannelConfig.secondaryRxMatch = receive.mSecondaryFilter.toUInt(&ok, 16);
        rxChannelConfig.networkPathDiff = receive.mNetworkPathDifferential.toUInt();
        rxChannelConfig.playoutDelay = receive.mPlayoutDelay.toUInt();

        config2022.SetRxChannelConfiguration (channel, rxChannelConfig);
        config2022.SetRxChannelEnable (channel, getEnable(receive.mEnable),enable2022_7);
    }
    std::cerr << "## transmitIter" << std::endl;

    QListIterator<TransmitStruct> transmitIter(mKonaIPParams.mTransmitChannels);
    while (transmitIter.hasNext())
    {
        std::cerr << "## transmitIter did" << std::endl;

        TransmitStruct transmit = transmitIter.next();
        tx_2022_channel txChannelConfig;

        NTV2Channel channel = getChannel(transmit.mChannelDesignator);
        txChannelConfig.primaryLocalPort = transmit.mPrimaryLocalPort.toUInt();
        txChannelConfig.primaryRemoteIP = transmit.mPrimaryRemoteIPAddress.toStdString();
        txChannelConfig.primaryRemotePort = transmit.mPrimaryRemotePort.toUInt();
        txChannelConfig.primaryRemoteMAC =  toMAC(transmit.mPrimaryRemoteMac);
        txChannelConfig.primaryAutoMAC =  getEnable(transmit.mPrimaryAutoMac);
        txChannelConfig.secondaryLocalPort = transmit.mSecondaryLocalPort.toUInt();
        txChannelConfig.secondaryRemoteIP = transmit.mSecondaryRemoteIPAddress.toStdString();
        txChannelConfig.secondaryRemotePort = transmit.mSecondaryRemotePort.toUInt();
        txChannelConfig.secondaryRemoteMAC =  toMAC(transmit.mSecondaryRemoteMac);
        txChannelConfig.secondaryAutoMAC =  getEnable(transmit.mSecondaryAutoMac);


        config2022.SetTxChannelConfiguration (channel, txChannelConfig);
        config2022.SetTxChannelEnable (channel, getEnable(transmit.mEnable),enable2022_7);
    }

    return true;
}

bool CKonaIpJsonSetup::setupBoard2110(std::string deviceSpec)
{
    CNTV2Card mDevice;
    CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, mDevice);
    if (!mDevice.IsOpen())
        {std::cerr << "## ERROR:  No devices found " << deviceSpec.c_str() << std::endl;  return false;}
    //if (!mDevice.IsKonaIPDevice ())
    //    {std::cerr << "## ERROR:  Not a KONA IP device" << std::endl;  return false;}

    //	Read MicroBlaze Uptime in Seconds, to see if it's running...
    while (!mDevice.IsDeviceReady ())
    {
        std::cout << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << std::endl;
        mDevice.SleepMs (1000);
        if (mDevice.IsDeviceReady ())
            std::cout << "## NOTE:  Device is ready" << std::endl;
    }

    CNTV2Config2110	config2110 (mDevice);

    if (!PTPMasterAddr.isEmpty())
    {
        mDevice.SetReference(NTV2_REFERENCE_SFP1_PTP);
        config2110.SetPTPMaster(PTPMasterAddr.toStdString());
    }

    if (mKonaIPParams.mSFPs.size() < 1)
    {
        std::cerr << "## ERROR:  Need To Specify at Least 1 SFP" << std::endl;
        return false;
    }

    QListIterator<SFPStruct> sfpIter(mKonaIPParams.mSFPs);
    while (sfpIter.hasNext())
    {
        SFPStruct sfp = sfpIter.next();

        if ( sfp.mSFPDesignator == "top")
        {
            config2110.SetNetworkConfiguration (SFP_TOP, sfp.mIPAddress.toStdString(), sfp.mSubnetMask.toStdString());
        }
        else
        if ( sfp.mSFPDesignator == "bottom")
        {
            config2110.SetNetworkConfiguration (SFP_BOTTOM, sfp.mIPAddress.toStdString(), sfp.mSubnetMask.toStdString());

        }
    }

    std::cerr << "## receiveIter" << std::endl;

    QListIterator<ReceiveStruct> receiveIter(mKonaIPParams.mReceiveChannels);
    while (receiveIter.hasNext())
    {
        std::cerr << "## receiveIter did" << std::endl;

        ReceiveStruct receive = receiveIter.next();
        rx_2110Config rxChannelConfig;
        bool ok;
        NTV2Channel channel          = getChannel(receive.mChannelDesignator);
        rxChannelConfig.rxMatch      = receive.mPrimaryFilter.toUInt(&ok, 16);
        rxChannelConfig.sourceIP     = receive.mSrcIPAddress.toStdString();
        rxChannelConfig.destIP       = receive.mPrimaryDestIPAddress.toStdString();
        rxChannelConfig.sourcePort   = receive.mSrcPort.toUInt();
        rxChannelConfig.destPort     = receive.mPrimaryDestPort.toUInt();
        rxChannelConfig.SSRC         = receive.mSSRC.toUInt();
        rxChannelConfig.VLAN         = receive.mVLAN.toUInt();
        rxChannelConfig.payloadType      = receive.mPayload.toUInt();
        rxChannelConfig.videoFormat  = CNTV2DemoCommon::GetVideoFormatFromString(receive.mVideoFormat.toStdString());
        rxChannelConfig.videoSamples = VPIDSampling_YUV_422;

        NTV2Stream stream;
        if (receive.mStream == "audio1")
            stream = NTV2_AUDIO1_STREAM;
        else
            stream = NTV2_VIDEO_STREAM;

        bool enable = (getEnable(receive.mEnable));
        bool rv = config2110.SetRxChannelConfiguration (channel, stream, rxChannelConfig,enable);
        if (!rv)
        {
            std::cerr << "FAILED: " << config2110.getLastError() << std::endl;
            return false;
        }
    }
    std::cerr << "## transmitIter" << std::endl;

    QListIterator<TransmitStruct> transmitIter(mKonaIPParams.mTransmitChannels);
    while (transmitIter.hasNext())
    {
        std::cerr << "## transmitIter did" << std::endl;

        TransmitStruct transmit = transmitIter.next();
        tx_2110Config txChannelConfig;

        NTV2Channel channel          = getChannel(transmit.mChannelDesignator);
        txChannelConfig.localPort    = transmit.mPrimaryLocalPort.toUInt();
        txChannelConfig.remoteIP     = transmit.mPrimaryRemoteIPAddress.toStdString();
        txChannelConfig.remotePort   = transmit.mPrimaryRemotePort.toUInt();
        txChannelConfig.remoteMAC    = toMAC(transmit.mPrimaryRemoteMac);
        txChannelConfig.autoMAC      = getEnable(transmit.mPrimaryAutoMac);
        txChannelConfig.videoFormat  = CNTV2DemoCommon::GetVideoFormatFromString(transmit.mVideoFormat.toStdString());
        txChannelConfig.videoSamples = VPIDSampling_YUV_422;

        NTV2Stream stream;
        if (transmit.mStream == "audio1")
            stream = NTV2_AUDIO1_STREAM;
        else
            stream = NTV2_VIDEO_STREAM;

        config2110.SetTxChannelConfiguration (channel, stream, txChannelConfig);
        config2110.SetTxChannelEnable(channel, stream, getEnable(transmit.mEnable));
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
    bool enable = false;
    if ( enableBoolString == "true")
        enable = true;

    return enable;
}

MACAddr toMAC(QString mac)
{
    MACAddr MAC;

    QStringList mlist = mac.split(':');
    int numBytes = mlist.size();
    if (numBytes > 6) numBytes = 6;

    for (int i=0; i< numBytes; i++)
    {
        bool ok;
        MAC.mac[i] = mlist[i].toUShort(&ok,16);
    }

    return MAC;
}
