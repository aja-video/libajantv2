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

using std::endl;
using std::cout;
using std::cerr;

NTV2Channel getChannel(QString channelDesignator);
bool getEnable(QString enableBoolString);
MACAddr toMAC(QString mac);

CKonaIpJsonSetup::CKonaIpJsonSetup()
{
    enable2022_7 = false;
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

        sfpStruct.mIPAddress = sfpObject["IPAddress"].toString();
        cout << "IPAddress " << sfpStruct.mIPAddress.toStdString() << endl;

        sfpStruct.mSubnetMask = sfpObject["SubnetMask"].toString();
        cout << "SubnetMask " << sfpStruct.mSubnetMask.toStdString() << endl;

        sfpStruct.mRouter = sfpObject["Router"].toString();
        cout << "Router " << sfpStruct.mRouter.toStdString() << endl;

        sfpStruct.mEnable2022_7 = sfpObject["Enable2022_7"].toString();
        if (!sfpStruct.mEnable2022_7.isEmpty())
            cout << "Enable2022_7 " << sfpStruct.mEnable2022_7.toStdString() << endl << endl;
        else
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

        receiveStruct.mSrcPort = receiveChannelObject["srcPort"].toString();
        if (!receiveStruct.mSrcPort.isEmpty())
            cout << "Src Port " << receiveStruct.mSrcPort.toStdString() << endl;

        receiveStruct.mSrcIPAddress = receiveChannelObject["srcIPAddress"].toString();
        if (!receiveStruct.mSrcIPAddress.isEmpty())
            cout << "Src IP Address " << receiveStruct.mSrcIPAddress.toStdString() << endl;

        receiveStruct.mPrimaryDestIPAddress = receiveChannelObject["primaryDestIPAddress"].toString();
        cout << "PrimaryDestIPAddress " << receiveStruct.mPrimaryDestIPAddress.toStdString() << endl;

        receiveStruct.mPrimaryDestPort = receiveChannelObject["primaryDestPort"].toString();
        cout << "PrimaryDestPort " << receiveStruct.mPrimaryDestPort.toStdString() << endl;

        receiveStruct.mPrimaryFilter = receiveChannelObject["primaryFilter"].toString();
        cout << "PrimaryFilter " << receiveStruct.mPrimaryFilter.toStdString() << endl;

        receiveStruct.mSecondaryDestIPAddress = receiveChannelObject["secondaryDestIPAddress"].toString();
        if (!receiveStruct.mSecondaryDestIPAddress.isEmpty())
            cout << "SecondaryDestIPAddress " << receiveStruct.mSecondaryDestIPAddress.toStdString() << endl;

        receiveStruct.mSecondaryDestPort = receiveChannelObject["secondaryDestPort"].toString();
        if (!receiveStruct.mSecondaryDestPort.isEmpty())
            cout << "SecondaryDestPort " << receiveStruct.mSecondaryDestPort.toStdString() << endl;

        receiveStruct.mSecondaryFilter = receiveChannelObject["secondaryFilter"].toString();
        if (!receiveStruct.mSecondaryFilter.isEmpty())
            cout << "SecondaryFilter " << receiveStruct.mSecondaryFilter.toStdString() << endl;

        receiveStruct.mNetworkPathDifferential = receiveChannelObject["networkPathDifferential"].toString();
        if (!receiveStruct.mNetworkPathDifferential.isEmpty())
            cout << "NetworkPathDifferential " << receiveStruct.mNetworkPathDifferential.toStdString() << endl;

        receiveStruct.mPlayoutDelay = receiveChannelObject["playoutDelay"].toString();
        if (!receiveStruct.mPlayoutDelay.isEmpty())
            cout << "PlayoutDelay " << receiveStruct.mPlayoutDelay.toStdString() << endl;

        receiveStruct.mVLAN = receiveChannelObject["vlan"].toString();
        if (!receiveStruct.mVLAN.isEmpty())
            cout << "VLAN " << receiveStruct.mVLAN.toStdString() << endl;

        receiveStruct.mSSRC = receiveChannelObject["ssrc"].toString();
        if (!receiveStruct.mSSRC.isEmpty())
            cout << "SSRC " << receiveStruct.mSSRC.toStdString() << endl;

        receiveStruct.mEnable = receiveChannelObject["Enable"].toString();
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

        transmitStruct.mPrimaryLocalPort = transmitChannelObject["primaryLocalPort"].toString();
        cout << "PrimaryLocalPort " << transmitStruct.mPrimaryLocalPort.toStdString() << endl;

        transmitStruct.mPrimaryRemoteIPAddress = transmitChannelObject["primaryRemoteIPAddress"].toString();
        cout << "PrimaryRemoteIPAddress " << transmitStruct.mPrimaryRemoteIPAddress.toStdString() << endl;

        transmitStruct.mPrimaryRemotePort = transmitChannelObject["primaryRemotePort"].toString();
        cout << "PrimaryRemotePort " << transmitStruct.mPrimaryRemotePort.toStdString() << endl;

        transmitStruct.mPrimaryRemoteMac = transmitChannelObject["primaryRemoteMac"].toString();
        cout << "PrimaryRemoteMac " << transmitStruct.mPrimaryRemoteMac.toStdString() << endl;

        transmitStruct.mPrimaryAutoMac = transmitChannelObject["primaryAutoMac"].toString();
        cout << "PrimaryAutoMac " << transmitStruct.mPrimaryAutoMac.toStdString() << endl;

        transmitStruct.mSecondaryLocalPort = transmitChannelObject["secondaryLocalPort"].toString();
        if (!transmitStruct.mSecondaryLocalPort.isEmpty())
            cout << "SecondaryLocalPort " << transmitStruct.mSecondaryLocalPort.toStdString() << endl;

        transmitStruct.mSecondaryRemoteIPAddress = transmitChannelObject["secondaryRemoteIPAddress"].toString();
        if (!transmitStruct.mSecondaryRemoteIPAddress.isEmpty())
            cout << "SecondaryRemoteIPAddress " << transmitStruct.mSecondaryRemoteIPAddress.toStdString() << endl;

        transmitStruct.mSecondaryRemoteIPAddress = transmitChannelObject["secondaryRemotePort"].toString();
        if (!transmitStruct.mSecondaryRemoteIPAddress.isEmpty())
        cout << "SecondaryRemotePort " << transmitStruct.mSecondaryRemotePort.toStdString() << endl;

        transmitStruct.mSecondaryRemoteMac = transmitChannelObject["secondaryRemoteMac"].toString();
        if (!transmitStruct.mSecondaryRemoteMac.isEmpty())
            cout << "SecondaryRemoteMac " << transmitStruct.mSecondaryRemoteMac.toStdString() << endl;

        transmitStruct.mSecondaryAutoMac = transmitChannelObject["secondaryAutoMac"].toString();
        if (!transmitStruct.mSecondaryAutoMac.isEmpty())
            cout << "SecondaryAutoMac " << transmitStruct.mSecondaryAutoMac.toStdString() << endl;

        transmitStruct.mVideoFormat = transmitChannelObject["videoFormat"].toString();
        if (!transmitStruct.mVideoFormat.isEmpty())
            cout << "Video format " << transmitStruct.mVideoFormat.toStdString() << endl;

        transmitStruct.mEnable = transmitChannelObject["Enable"].toString();
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

    return readJson(json);
}

bool CKonaIpJsonSetup::setupBoard(std::string deviceSpec)
{
        return setupBoard2022(deviceSpec);
}

bool CKonaIpJsonSetup::setupBoard2022(std::string deviceSpec)
{
    CNTV2Card mDevice;
    CNTV2DeviceScanner::GetFirstDeviceFromArgument (deviceSpec, mDevice);
    if (!mDevice.IsOpen())
        {cerr << "## ERROR:  No devices found " << deviceSpec.c_str() << endl;  return false;}
    //if (!mDevice.IsKonaIPDevice ())
    //    {cerr << "## ERROR:  Not a KONA IP device" << endl;  return false;}

    //	Read MicroBlaze Uptime in Seconds, to see if it's running...
    while (!mDevice.IsDeviceReady ())
    {
        cout << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << endl;
        mDevice.SleepMs (1000);
        if (mDevice.IsDeviceReady ())
            cout << "## NOTE:  Device is ready" << endl;
    }

    enable2022_7 = false;
    CNTV2Config2022	config2022 (mDevice);

    if (mKonaIPParams.mSFPs.size() < 1)
    {
        {cerr << "## ERROR:  Need To Specify at Least 1 SFP" << endl;  return false;}
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

    cerr << "## receiveIter" << endl;

    QListIterator<ReceiveStruct> receiveIter(mKonaIPParams.mReceiveChannels);
    while (receiveIter.hasNext())
    {
        cerr << "## receiveIter did" << endl;

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
    cerr << "## transmitIter" << endl;

    QListIterator<TransmitStruct> transmitIter(mKonaIPParams.mTransmitChannels);
    while (transmitIter.hasNext())
    {
        cerr << "## transmitIter did" << endl;

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
