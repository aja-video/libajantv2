#include "konaipjsonsetup.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2democommon.h"
#include "ntv2endian.h"

#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"

#if defined (AJALinux) || defined (AJAMac)
#include <arpa/inet.h>
#endif

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

	// default is 2110 unless specified by protocol
	mIs2110 = true;

    const QJsonObject & json = loadDoc.object();
    QJsonValue qjv = json.value("protocol");
    if (qjv != QJsonValue::Undefined)
    {
        QString protocol = qjv.toString();
		if (protocol == "2022")
        {
			mIs2110 = false;
        }
	}

    if (mIs2110)
    {
		cout << "-----Protocol2110-----" << endl;
        result = parse2110.SetJson(json, true);
    }
    else
    {
		cout << "-----Protocol2022-----" << endl;
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
    //if (!mDevice.IsIPDevice())
    //    {cerr << "## ERROR:  Not an IP device" << endl;  return false;}

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
    mDevice.ReadRegister(SAREK_REGS + kRegSarekFwCfg, val);
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
		cerr << "## receiveIter " << endl;

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
		cerr << "## transmitIter " << endl;

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
    //if (!mDevice.IsIPDevice())
    //    {cerr << "## ERROR:  Not an IP device" << endl;  return false;}

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

    // fetch parsed newtwork struct
    NetworkData2110 net2110 = parse2110.m_net2110;

    // Only if a non zero domain was passed in
    if (net2110.ptpDomain)
    {
        config2110.SetPTPDomain(net2110.ptpDomain);
    }

    // Only if a non zero grandMasterID was passed in
    if (net2110.ptpPreferredGMID[0] || net2110.ptpPreferredGMID[1] || net2110.ptpPreferredGMID[2] || net2110.ptpPreferredGMID[3] ||
        net2110.ptpPreferredGMID[4] || net2110.ptpPreferredGMID[5] || net2110.ptpPreferredGMID[6] || net2110.ptpPreferredGMID[7])
    {
        config2110.SetPTPPreferredGrandMasterId(net2110.ptpPreferredGMID);
    }

    device.SetReference(NTV2_REFERENCE_SFP1_PTP);
    config2110.Set4KModeEnable(net2110.setup4k);
	config2110.SetAudioCombineEnable(net2110.audioCombine);
    config2110.SetPTPDomain(net2110.ptpDomain);
    config2110.SetPTPPreferredGrandMasterId(net2110.ptpPreferredGMID);

    for (uint32_t i = 0; i < net2110.numSFPs; i++)
    {
		cerr << "## network " << i+1 << endl;

        eSFP sfp = SFP_1;
        if (i > 0)
            sfp = SFP_2;

        bool rv;
        if (net2110.sfp[i].enable)
        {
            rv =  config2110.SetNetworkConfiguration(sfp,
                                                     net2110.sfp[i].ipAddress,
                                                     net2110.sfp[i].subnetMask,
                                                     net2110.sfp[i].gateWay);
            if (!rv)
            {
                cerr << "Error: " << config2110.getLastError() << endl;
                return false;
            }
        }
        else
        {
            config2110.DisableNetworkInterface(sfp);
        }
    }

    rx_2110Config rxChannelConfig;

    // fetch parsed receive video struct
    ReceiveVideoData2110 receiveVideo2110 = parse2110.m_receiveVideo2110;
    for (uint32_t i = 0; i < receiveVideo2110.numRxVideoChannels; i++)
    {
		cerr << "## receiveVideo " << i+1 << endl;

        rxChannelConfig.init();

        // If sfp2 is on use it otherwise assume we are always dealing with sfp1
        // (RX does not support both at the moment)
        eSFP sfp = SFP_INVALID;
        if (receiveVideo2110.rxVideoCh[i].sfpEnable[0])
            sfp = SFP_1;
        else if (receiveVideo2110.rxVideoCh[i].sfpEnable[1])
            sfp = SFP_2;

        // Until we support -7, only use one link
        if (sfp == SFP_1)
        {
            rxChannelConfig.sourceIP    = receiveVideo2110.rxVideoCh[i].sourceIP[0];
            rxChannelConfig.destIP      = receiveVideo2110.rxVideoCh[i].destIP[0];
            rxChannelConfig.sourcePort  = receiveVideo2110.rxVideoCh[i].sourcePort[0];
            rxChannelConfig.destPort    = receiveVideo2110.rxVideoCh[i].destPort[0];
        }
        else
        {
            rxChannelConfig.sourceIP    = receiveVideo2110.rxVideoCh[i].sourceIP[1];
            rxChannelConfig.destIP      = receiveVideo2110.rxVideoCh[i].destIP[1];
            rxChannelConfig.sourcePort  = receiveVideo2110.rxVideoCh[i].sourcePort[1];
            rxChannelConfig.destPort    = receiveVideo2110.rxVideoCh[i].destPort[1];
        }
        rxChannelConfig.ssrc            = receiveVideo2110.rxVideoCh[i].vlan;
        rxChannelConfig.vlan            = receiveVideo2110.rxVideoCh[i].ssrc;
        rxChannelConfig.payloadType     = receiveVideo2110.rxVideoCh[i].payloadType;

        rxChannelConfig.videoFormat     = receiveVideo2110.rxVideoCh[i].videoFormat;
        rxChannelConfig.videoSamples    = VPIDSampling_YUV_422;

        // Set RX match based on non zero passed in params
        rxChannelConfig.rxMatch = 0;
        uint32_t ip = 0;
        ip = inet_addr(rxChannelConfig.sourceIP.c_str());
        ip = NTV2EndianSwap32(ip);
        if (ip)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_SOURCE_IP;
        ip = inet_addr(rxChannelConfig.destIP.c_str());
        ip = NTV2EndianSwap32(ip);
        if (ip)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_DEST_IP;
        if (rxChannelConfig.sourcePort)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_SOURCE_PORT;
        if (rxChannelConfig.destPort)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_DEST_PORT;
        if (rxChannelConfig.vlan)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_VLAN;
        if (rxChannelConfig.payloadType)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_PAYLOAD;
        if (rxChannelConfig.ssrc)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_SSRC;


		bool rv;
		rv = config2110.SetRxStreamConfiguration (sfp,
                                                  receiveVideo2110.rxVideoCh[i].stream,
                                                  rxChannelConfig);
        if (!rv)
        {
            cerr << "SetRxStreamConfiguration: FAILED: " << config2110.getLastError() << endl;
            return false;
        }

        rv = config2110.SetRxStreamEnable(sfp,
                                          receiveVideo2110.rxVideoCh[i].stream,
                                          receiveVideo2110.rxVideoCh[i].enable);
        if (!rv)
        {
            cerr << "SetRxStreamEnable: FAILED: " << config2110.getLastError() << endl;
            return false;
        }
    }

    // fetch parsed receive audio struct
    ReceiveAudioData2110 receiveAudio2110 = parse2110.m_receiveAudio2110;
    for (uint32_t i = 0; i < receiveAudio2110.numRxAudioChannels; i++)
    {
		cerr << "## receiveAudio " << i+1 << endl;

        rxChannelConfig.init();

        // If sfp2 is on use it otherwise assume we are always dealing with sfp1
        // (RX does not support both at the moment)
        eSFP sfp = SFP_INVALID;
        if (receiveAudio2110.rxAudioCh[i].sfpEnable[0])
            sfp = SFP_1;
        else if (receiveAudio2110.rxAudioCh[i].sfpEnable[1])
            sfp = SFP_2;

        // Until we support -7, only use one link
        if (sfp == SFP_1)
        {
            rxChannelConfig.sourceIP    = receiveAudio2110.rxAudioCh[i].sourceIP[0];
            rxChannelConfig.destIP      = receiveAudio2110.rxAudioCh[i].destIP[0];
            rxChannelConfig.sourcePort  = receiveAudio2110.rxAudioCh[i].sourcePort[0];
            rxChannelConfig.destPort    = receiveAudio2110.rxAudioCh[i].destPort[0];
        }
        else
        {
            rxChannelConfig.sourceIP    = receiveAudio2110.rxAudioCh[i].sourceIP[1];
            rxChannelConfig.destIP      = receiveAudio2110.rxAudioCh[i].destIP[1];
            rxChannelConfig.sourcePort  = receiveAudio2110.rxAudioCh[i].sourcePort[1];
            rxChannelConfig.destPort    = receiveAudio2110.rxAudioCh[i].destPort[1];
        }
        rxChannelConfig.ssrc            = receiveAudio2110.rxAudioCh[i].vlan;
        rxChannelConfig.vlan            = receiveAudio2110.rxAudioCh[i].ssrc;
        rxChannelConfig.payloadType     = receiveAudio2110.rxAudioCh[i].payloadType;
        rxChannelConfig.numAudioChannels    = receiveAudio2110.rxAudioCh[i].numAudioChannels;
        rxChannelConfig.audioPktInterval    = receiveAudio2110.rxAudioCh[i].audioPktInterval;

        // Set RX match based on non zero passed in params
        rxChannelConfig.rxMatch = 0;
        uint32_t ip = 0;
        ip = inet_addr(rxChannelConfig.sourceIP.c_str());
        ip = NTV2EndianSwap32(ip);
        if (ip)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_SOURCE_IP;
        ip = inet_addr(rxChannelConfig.destIP.c_str());
        ip = NTV2EndianSwap32(ip);
        if (ip)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_DEST_IP;
        if (rxChannelConfig.sourcePort)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_SOURCE_PORT;
        if (rxChannelConfig.destPort)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_DEST_PORT;
        if (rxChannelConfig.vlan)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_VLAN;
        if (rxChannelConfig.payloadType)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_PAYLOAD;
        if (rxChannelConfig.ssrc)
            rxChannelConfig.rxMatch |= RX_MATCH_2110_SSRC;


		bool rv;
		rv = config2110.SetRxStreamConfiguration (sfp,
                                                  receiveAudio2110.rxAudioCh[i].stream,
                                                  rxChannelConfig);
        if (!rv)
        {
            cerr << "SetRxStreamConfiguration: FAILED: " << config2110.getLastError() << endl;
            return false;
        }

        rv = config2110.SetRxStreamEnable(sfp,
                                          receiveAudio2110.rxAudioCh[i].stream,
                                          receiveAudio2110.rxAudioCh[i].enable);
        if (!rv)
        {
            cerr << "SetRxStreamEnable: FAILED: " << config2110.getLastError() << endl;
            return false;
        }
    }

	// fetch parsed receive anc struct
	ReceiveAncData2110 receiveAnc2110 = parse2110.m_receiveAnc2110;
	for (uint32_t i = 0; i < receiveAnc2110.numRxAncChannels; i++)
	{
		cerr << "## receiveAnc " << i+1 << endl;

		rxChannelConfig.init();

		// If sfp2 is on use it otherwise assume we are always dealing with sfp1
		// (RX does not support both at the moment)
		eSFP sfp = SFP_INVALID;
		if (receiveAnc2110.rxAncCh[i].sfpEnable[0])
			sfp = SFP_1;
		else if (receiveAnc2110.rxAncCh[i].sfpEnable[1])
			sfp = SFP_2;

		// Until we support -7, only use one link
		if (sfp == SFP_1)
		{
			rxChannelConfig.sourceIP    = receiveAnc2110.rxAncCh[i].sourceIP[0];
			rxChannelConfig.destIP      = receiveAnc2110.rxAncCh[i].destIP[0];
			rxChannelConfig.sourcePort  = receiveAnc2110.rxAncCh[i].sourcePort[0];
			rxChannelConfig.destPort    = receiveAnc2110.rxAncCh[i].destPort[0];
		}
		else
		{
			rxChannelConfig.sourceIP    = receiveAnc2110.rxAncCh[i].sourceIP[1];
			rxChannelConfig.destIP      = receiveAnc2110.rxAncCh[i].destIP[1];
			rxChannelConfig.sourcePort  = receiveAnc2110.rxAncCh[i].sourcePort[1];
			rxChannelConfig.destPort    = receiveAnc2110.rxAncCh[i].destPort[1];
		}
		rxChannelConfig.ssrc            = receiveAnc2110.rxAncCh[i].vlan;
		rxChannelConfig.vlan            = receiveAnc2110.rxAncCh[i].ssrc;
		rxChannelConfig.payloadType     = receiveAnc2110.rxAncCh[i].payloadType;

		// Set RX match based on non zero passed in params
		rxChannelConfig.rxMatch = 0;
		uint32_t ip = 0;
		ip = inet_addr(rxChannelConfig.sourceIP.c_str());
		ip = NTV2EndianSwap32(ip);
		if (ip)
			rxChannelConfig.rxMatch |= RX_MATCH_2110_SOURCE_IP;
		ip = inet_addr(rxChannelConfig.destIP.c_str());
		ip = NTV2EndianSwap32(ip);
		if (ip)
			rxChannelConfig.rxMatch |= RX_MATCH_2110_DEST_IP;
		if (rxChannelConfig.sourcePort)
			rxChannelConfig.rxMatch |= RX_MATCH_2110_SOURCE_PORT;
		if (rxChannelConfig.destPort)
			rxChannelConfig.rxMatch |= RX_MATCH_2110_DEST_PORT;
		if (rxChannelConfig.vlan)
			rxChannelConfig.rxMatch |= RX_MATCH_2110_VLAN;
		if (rxChannelConfig.payloadType)
			rxChannelConfig.rxMatch |= RX_MATCH_2110_PAYLOAD;
		if (rxChannelConfig.ssrc)
			rxChannelConfig.rxMatch |= RX_MATCH_2110_SSRC;


#if 0
		bool rv;
		rv = config2110.SetRxStreamConfiguration (sfp,
												  receiveAnc2110.rxAncCh[i].stream,
												  rxChannelConfig);
		if (!rv)
		{
			cerr << "SetRxStreamConfiguration: FAILED: " << config2110.getLastError() << endl;
			return false;
		}

		rv = config2110.SetRxStreamEnable(sfp,
										  receiveAnc2110.rxAncCh[i].stream,
										  receiveAnc2110.rxAncCh[i].enable);
		if (!rv)
		{
			cerr << "SetRxStreamEnable: FAILED: " << config2110.getLastError() << endl;
			return false;
		}
#endif
	}

    tx_2110Config txChannelConfig;

    // fetch parsed transmit video struct
    TransmitVideoData2110 transmitVideo2110 = parse2110.m_transmitVideo2110;
    for (uint32_t i = 0; i < transmitVideo2110.numTxVideoChannels; i++)
    {
		cerr << "## transmitVideo " << i+1 << endl;

        bool rv;
        txChannelConfig.init();

        txChannelConfig.localPort[0]    = transmitVideo2110.txVideoCh[i].localPort[0];
        txChannelConfig.remoteIP[0]     = transmitVideo2110.txVideoCh[i].remoteIP[0];
        txChannelConfig.remotePort[0]   = transmitVideo2110.txVideoCh[i].remotePort[0];
        txChannelConfig.localPort[1]    = transmitVideo2110.txVideoCh[i].localPort[1];
        txChannelConfig.remoteIP[1]     = transmitVideo2110.txVideoCh[i].remoteIP[1];
        txChannelConfig.remotePort[1]   = transmitVideo2110.txVideoCh[i].remotePort[1];

        txChannelConfig.payloadType     = transmitVideo2110.txVideoCh[i].payloadType;
        txChannelConfig.ssrc            = transmitVideo2110.txVideoCh[i].ssrc;
        txChannelConfig.ttl             = transmitVideo2110.txVideoCh[i].ttl;
        txChannelConfig.videoFormat     = transmitVideo2110.txVideoCh[i].videoFormat;
        txChannelConfig.videoSamples    = VPIDSampling_YUV_422;

        rv = config2110.SetTxStreamConfiguration(transmitVideo2110.txVideoCh[i].stream,
                                                 txChannelConfig);
        if (!rv)
        {
            cerr << "SetTxStreamConfiguration Video: FAILED: " << config2110.getLastError() << endl;
            return false;
        }

        if (transmitVideo2110.txVideoCh[i].enable)
        {
            rv = config2110.SetTxStreamEnable(transmitVideo2110.txVideoCh[i].stream,
                                              transmitVideo2110.txVideoCh[i].sfpEnable[0],
                                              transmitVideo2110.txVideoCh[i].sfpEnable[1]);
            if (!rv)
            {
                cerr << "SetTxStreamEnable Audio: FAILED: " << config2110.getLastError() << endl;
                return false;
            }
        }
    }

    // fetch parsed transmit audio struct
    TransmitAudioData2110 transmitAudio2110 = parse2110.m_transmitAudio2110;
    for (uint32_t i = 0; i < transmitAudio2110.numTxAudioChannels; i++)
    {
		cerr << "## transmitAudio " << i+1 << endl;

        bool rv;
        txChannelConfig.init();

        txChannelConfig.localPort[0]    = transmitAudio2110.txAudioCh[i].localPort[0];
        txChannelConfig.remoteIP[0]     = transmitAudio2110.txAudioCh[i].remoteIP[0];
        txChannelConfig.remotePort[0]   = transmitAudio2110.txAudioCh[i].remotePort[0];
        txChannelConfig.localPort[1]    = transmitAudio2110.txAudioCh[i].localPort[1];
        txChannelConfig.remoteIP[1]     = transmitAudio2110.txAudioCh[i].remoteIP[1];
        txChannelConfig.remotePort[1]   = transmitAudio2110.txAudioCh[i].remotePort[1];

        txChannelConfig.payloadType     = transmitAudio2110.txAudioCh[i].payloadType;
        txChannelConfig.ssrc            = transmitAudio2110.txAudioCh[i].ssrc;
        txChannelConfig.ttl             = transmitAudio2110.txAudioCh[i].ttl;

        txChannelConfig.numAudioChannels    = transmitAudio2110.txAudioCh[i].numAudioChannels;
        txChannelConfig.firstAudioChannel   = transmitAudio2110.txAudioCh[i].firstAudioChannel;
        txChannelConfig.audioPktInterval    = transmitAudio2110.txAudioCh[i].audioPktInterval;

        txChannelConfig.channel         = transmitAudio2110.txAudioCh[i].channel;

        rv = config2110.SetTxStreamConfiguration(transmitAudio2110.txAudioCh[i].stream,
                                                 txChannelConfig);
        if (!rv)
        {
            cerr << "SetTxStreamConfiguration Audio: FAILED: " << config2110.getLastError() << endl;
            return false;
        }

        if (transmitAudio2110.txAudioCh[i].enable)
        {
            rv = config2110.SetTxStreamEnable(transmitAudio2110.txAudioCh[i].stream,
                                              transmitAudio2110.txAudioCh[i].sfpEnable[0],
                                              transmitAudio2110.txAudioCh[i].sfpEnable[1]);
            if (!rv)
            {
                cerr << "SetTxStreamEnable Audio: FAILED: " << config2110.getLastError() << endl;
                return false;
            }
        }
    }

	// fetch parsed transmit anc struct
	TransmitAncData2110 transmitAnc2110 = parse2110.m_transmitAnc2110;
	for (uint32_t i = 0; i < transmitAnc2110.numTxAncChannels; i++)
	{
		cerr << "## transmitAnc " << i+1 << endl;

		bool rv;
		txChannelConfig.init();

		txChannelConfig.localPort[0]    = transmitAnc2110.txAncCh[i].localPort[0];
		txChannelConfig.remoteIP[0]     = transmitAnc2110.txAncCh[i].remoteIP[0];
		txChannelConfig.remotePort[0]   = transmitAnc2110.txAncCh[i].remotePort[0];
		txChannelConfig.localPort[1]    = transmitAnc2110.txAncCh[i].localPort[1];
		txChannelConfig.remoteIP[1]     = transmitAnc2110.txAncCh[i].remoteIP[1];
		txChannelConfig.remotePort[1]   = transmitAnc2110.txAncCh[i].remotePort[1];

		txChannelConfig.payloadType     = transmitAnc2110.txAncCh[i].payloadType;
		txChannelConfig.ssrc            = transmitAnc2110.txAncCh[i].ssrc;
		txChannelConfig.ttl             = transmitAnc2110.txAncCh[i].ttl;

		rv = config2110.SetTxStreamConfiguration(transmitAnc2110.txAncCh[i].stream,
												 txChannelConfig);
		if (!rv)
		{
			cerr << "SetTxStreamConfiguration Anc: FAILED: " << config2110.getLastError() << endl;
			return false;
		}

		if (transmitAnc2110.txAncCh[i].enable)
		{
			rv = config2110.SetTxStreamEnable(transmitAnc2110.txAncCh[i].stream,
											  transmitAnc2110.txAncCh[i].sfpEnable[0],
											  transmitAnc2110.txAncCh[i].sfpEnable[1]);
			if (!rv)
			{
				cerr << "SetTxStreamEnable Anc: FAILED: " << config2110.getLastError() << endl;
				return false;
			}
		}
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
