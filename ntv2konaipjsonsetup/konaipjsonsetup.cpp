#include "konaipjsonsetup.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2config2022.h"

NTV2Channel getChannel(QString channelDesignator);
bool getEnable(QString enableBoolString);
MACAddr toMAC(QString mac);

CKonaIpJsonSetup::CKonaIpJsonSetup()
{

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
        std::cout << "SFPDesignator " << sfpStruct.mSFPDesignator.toStdString().c_str() << std::endl;

        sfpStruct.mIPAddress = sfpObject["IPAddress"].toString();
        std::cout << "IPAddress " << sfpStruct.mIPAddress.toStdString().c_str() << std::endl;

        sfpStruct.mSubnetMask = sfpObject["SubnetMask"].toString();
        std::cout << "SubnetMask " << sfpStruct.mSubnetMask.toStdString().c_str() << std::endl;

        sfpStruct.mRouter = sfpObject["Router"].toString();
        std::cout << "Router " << sfpStruct.mRouter.toStdString().c_str() << std::endl;

        sfpStruct.mEnable2022_7 = sfpObject["Enable2022_7"].toString();
        std::cout << "Enable2022_7 " << sfpStruct.mEnable2022_7.toStdString().c_str() << std::endl << std::endl;

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
        std::cout << "ChannelDesignator " << receiveStruct.mChannelDesignator.toStdString().c_str() << std::endl;

        receiveStruct.mPrimaryDestIPAddress = receiveChannelObject["primaryDestIPAddress"].toString();
        std::cout << "PrimaryDestIPAddress " << receiveStruct.mPrimaryDestIPAddress.toStdString().c_str() << std::endl;

        receiveStruct.mPrimaryDestPort = receiveChannelObject["primaryDestPort"].toString();
        std::cout << "PrimaryDestPort " << receiveStruct.mPrimaryDestPort.toStdString().c_str() << std::endl;

        receiveStruct.mPrimaryFilter = receiveChannelObject["primaryFilter"].toString();
        std::cout << "PrimaryFilter " << receiveStruct.mPrimaryFilter.toStdString().c_str() << std::endl;

        receiveStruct.mSecondaryDestIPAddress = receiveChannelObject["secondaryDestIPAddress"].toString();
        std::cout << "SecondaryDestIPAddress " << receiveStruct.mSecondaryDestIPAddress.toStdString().c_str() << std::endl;

        receiveStruct.mSecondaryDestPort = receiveChannelObject["secondaryDestPort"].toString();
        std::cout << "SecondaryDestPort " << receiveStruct.mSecondaryDestPort.toStdString().c_str() << std::endl;

        receiveStruct.mSecondaryFilter = receiveChannelObject["secondaryFilter"].toString();
        std::cout << "SecondaryFilter " << receiveStruct.mSecondaryFilter.toStdString().c_str() << std::endl;

        receiveStruct.mNetworkPathDifferential = receiveChannelObject["networkPathDifferential"].toString();
        std::cout << "NetworkPathDifferential " << receiveStruct.mNetworkPathDifferential.toStdString().c_str() << std::endl;

        receiveStruct.mPlayoutDelay = receiveChannelObject["playoutDelay"].toString();
        std::cout << "PlayoutDelay " << receiveStruct.mPlayoutDelay.toStdString().c_str() << std::endl;

        receiveStruct.mEnable = receiveChannelObject["Enable"].toString();
        std::cout << "Enable " << receiveStruct.mEnable.toStdString().c_str() << std::endl << std::endl;

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
        std::cout << "ChannelDesignator " << transmitStruct.mChannelDesignator.toStdString().c_str() << std::endl;

        transmitStruct.mPrimaryLocalPort = transmitChannelObject["primaryLocalPort"].toString();
        std::cout << "PrimaryLocalPort " << transmitStruct.mPrimaryLocalPort.toStdString().c_str() << std::endl;

        transmitStruct.mPrimaryRemoteIPAddress = transmitChannelObject["primaryRemoteIPAddress"].toString();
        std::cout << "PrimaryRemoteIPAddress " << transmitStruct.mPrimaryRemoteIPAddress.toStdString().c_str() << std::endl;

        transmitStruct.mPrimaryRemotePort = transmitChannelObject["primaryRemotePort"].toString();
        std::cout << "PrimaryRemotePort " << transmitStruct.mPrimaryRemotePort.toStdString().c_str() << std::endl;

        transmitStruct.mPrimaryRemoteMac = transmitChannelObject["primaryRemoteMac"].toString();
        std::cout << "PrimaryRemoteMac " << transmitStruct.mPrimaryRemoteMac.toStdString().c_str() << std::endl;

        transmitStruct.mPrimaryAutoMac = transmitChannelObject["primaryAutoMac"].toString();
        std::cout << "PrimaryAutoMac " << transmitStruct.mPrimaryAutoMac.toStdString().c_str() << std::endl;

        transmitStruct.mSecondaryLocalPort = transmitChannelObject["secondaryLocalPort"].toString();
        std::cout << "SecondaryLocalPort " << transmitStruct.mSecondaryLocalPort.toStdString().c_str() << std::endl;

        transmitStruct.mSecondaryRemoteIPAddress = transmitChannelObject["secondaryRemoteIPAddress"].toString();
        std::cout << "SecondaryRemoteIPAddress " << transmitStruct.mSecondaryRemoteIPAddress.toStdString().c_str() << std::endl;

        transmitStruct.mSecondaryRemotePort = transmitChannelObject["secondaryRemotePort"].toString();
        std::cout << "SecondaryRemotePort " << transmitStruct.mSecondaryRemotePort.toStdString().c_str() << std::endl;

        transmitStruct.mSecondaryRemoteMac = transmitChannelObject["secondaryRemoteMac"].toString();
        std::cout << "SecondaryRemoteMac " << transmitStruct.mSecondaryRemoteMac.toStdString().c_str() << std::endl;

        transmitStruct.mSecondaryAutoMac = transmitChannelObject["secondaryAutoMac"].toString();
        std::cout << "SecondaryAutoMac " << transmitStruct.mSecondaryAutoMac.toStdString().c_str() << std::endl;

        transmitStruct.mEnable = transmitChannelObject["Enable"].toString();
        std::cout << "Enable " << transmitStruct.mEnable.toStdString().c_str() << std::endl << std::endl;

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
        }
    }

    return readJson(json);
}

bool CKonaIpJsonSetup::setupBoard(std::string deviceSpec,KonaIPParamSetupStruct* pKonaIPParams)
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

    if (pKonaIPParams->mSFPs.size() < 1)
    {
        {std::cerr << "## ERROR:  Need To Specify at Least 1 SFP" << std::endl;  return false;}
    }

    QListIterator<SFPStruct> sfpIter(pKonaIPParams->mSFPs);
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

    QListIterator<ReceiveStruct> receiveIter(pKonaIPParams->mReceiveChannels);
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

    QListIterator<TransmitStruct> transmitIter(pKonaIPParams->mTransmitChannels);
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
