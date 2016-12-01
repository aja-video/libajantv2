#include "konaipencodersetup.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2configts2022.h"


CKonaIpEncoderJsonReader::CKonaIpEncoderJsonReader()
{
    initMaps();
}

bool CKonaIpEncoderJsonReader::readJson(const QJsonObject &json)
{
    QJsonObject pjson = json["parameters"].toObject();
    QString vf =  pjson["VideoFormat"].toString();
    mKonaIPParams.videoFormat = videoFormatMap[vf];
    std::cout << "VideoFormat    " << vf.toStdString().c_str() << std::endl;

    QString st =  pjson["StreamType"].toString();
    mKonaIPParams.streamType = streamTypeMap[st];
    std::cout << "StreamType     " << st.toStdString().c_str() << std::endl;

    QString css =  pjson["ChromaSampling"].toString();
    mKonaIPParams.chromaSubSampling = chromaSubSamplingMap[css];
    std::cout << "ChromaSampling " << css.toStdString().c_str() << std::endl;

    QString cbs =  pjson["CodeBlockSize"].toString();
    mKonaIPParams.codeBlockSize = codeBlockSizeMap[cbs];
    std::cout << "CodeBlockSize  " << cbs.toStdString().c_str() << std::endl;

    mKonaIPParams.numBits = pjson["NumBits"].toInt();
    std::cout << "NumBits        " << mKonaIPParams.numBits << std::endl;
    mKonaIPParams.mbps = pjson["Mbps"].toInt();
    std::cout << "Mbps           " << mKonaIPParams.mbps << std::endl;
    mKonaIPParams.programPid = pjson["ProgramPid"].toInt();
    std::cout << "ProgramPid     " << mKonaIPParams.programPid << std::endl;
    mKonaIPParams.videoPid = pjson["VideoPid"].toInt();
    std::cout << "VideoPid       " << mKonaIPParams.videoPid << std::endl;
    mKonaIPParams.pcrPid = pjson["PcrPid"].toInt();
    std::cout << "PcrPid         " << mKonaIPParams.pcrPid << std::endl;
    mKonaIPParams.audio1Pid = pjson["Audio1Pid"].toInt();
    std::cout << "Audio1Pid      " << mKonaIPParams.audio1Pid << std::endl << std::endl;
    mKonaIPParams.ullMode = false;
    return true;
}

bool CKonaIpEncoderJsonReader::openJson(QString fileName)
{
    QFile loadFile(fileName);
    if ( !loadFile.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open json file.");
        return false;
    }
    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    return readJson(loadDoc.object());

}

void CKonaIpEncoderJsonReader::printVideoFormatMap()
{
    QMap<QString, NTV2VideoFormat>::iterator i;
    for (i = videoFormatMap.begin(); i != videoFormatMap.end(); ++i)
    {
        QString str = i.key();
        std::cout << str.toStdString() << std::endl;
    }
}

CKonaIPEncoderSetup::CKonaIPEncoderSetup()
{

}

bool CKonaIPEncoderSetup::setupBoard(std::string pDeviceSpec,KonaIPParamSetupStruct* pKonaIPParams)
{
    CNTV2Card mDevice;
    CNTV2DeviceScanner::GetFirstDeviceFromArgument (pDeviceSpec, mDevice);
    if (!mDevice.IsOpen())
    {std::cerr << "## ERROR:  No devices found" << std::endl;  return false;}
    //if (!mDevice.IsKonaIPDevice ())
    //{std::cerr << "## ERROR:  Not a KONA IP device" << std::endl;  return false;}

    //	Read MicroBlaze Uptime in Seconds, to see if it's running...
    while (!mDevice.IsDeviceReady ())
    {
        std::cerr << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << std::endl;
        mDevice.SleepMs (1000);
        if (mDevice.IsDeviceReady ())
            std::cerr << "## NOTE:  Device is ready" << std::endl;
    }

    CNTV2ConfigTs2022	configTs2022 (mDevice);

    configTs2022.SetJ2KEncodeVideoFormat(NTV2_CHANNEL1, pKonaIPParams->videoFormat);
    configTs2022.SetJ2KEncodeUllMode(NTV2_CHANNEL1, false);
    configTs2022.SetJ2KEncodeBitDepth(NTV2_CHANNEL1, pKonaIPParams->numBits);
    configTs2022.SetJ2KEncodeChromaSubsamp(NTV2_CHANNEL1, (J2KChromaSubSampling)pKonaIPParams->chromaSubSampling );
    configTs2022.SetJ2KEncodeCodeBlocksize(NTV2_CHANNEL1, (J2KCodeBlocksize)pKonaIPParams->codeBlockSize);
    configTs2022.SetJ2KEncodeMbps(NTV2_CHANNEL1, pKonaIPParams->mbps);
    configTs2022.SetJ2KEncodePMTPid(NTV2_CHANNEL1, pKonaIPParams->programPid);
    configTs2022.SetJ2KEncodeVideoPid(NTV2_CHANNEL1, pKonaIPParams->videoPid);
    configTs2022.SetJ2KEncodePCRPid(NTV2_CHANNEL1, pKonaIPParams->pcrPid);
    configTs2022.SetJ2KEncodeAudio1Pid(NTV2_CHANNEL1, pKonaIPParams->audio1Pid);
    configTs2022.SetJ2KEncodeStreamType(NTV2_CHANNEL1, (J2KStreamType)pKonaIPParams->streamType);

    // Now setup the J2K encoder and TS encapsulator for ch1
    bool rv = configTs2022.SetupJ2KEncoder(NTV2_CHANNEL1);
    rv = configTs2022.SetupTsForEncode(NTV2_CHANNEL1);

    // Same Setup for Channel 2
    configTs2022.SetJ2KEncodeVideoFormat(NTV2_CHANNEL2, pKonaIPParams->videoFormat);
    configTs2022.SetJ2KEncodeUllMode(NTV2_CHANNEL2, false);
    configTs2022.SetJ2KEncodeBitDepth(NTV2_CHANNEL2, pKonaIPParams->numBits);
    configTs2022.SetJ2KEncodeChromaSubsamp(NTV2_CHANNEL2, (J2KChromaSubSampling)pKonaIPParams->chromaSubSampling );
    configTs2022.SetJ2KEncodeCodeBlocksize(NTV2_CHANNEL2, (J2KCodeBlocksize)pKonaIPParams->codeBlockSize);
    configTs2022.SetJ2KEncodeMbps(NTV2_CHANNEL2, pKonaIPParams->mbps);
    configTs2022.SetJ2KEncodePMTPid(NTV2_CHANNEL2, pKonaIPParams->programPid);
    configTs2022.SetJ2KEncodeVideoPid(NTV2_CHANNEL2, pKonaIPParams->videoPid);
    configTs2022.SetJ2KEncodePCRPid(NTV2_CHANNEL2, pKonaIPParams->pcrPid);
    configTs2022.SetJ2KEncodeAudio1Pid(NTV2_CHANNEL2, pKonaIPParams->audio1Pid);
    configTs2022.SetJ2KEncodeStreamType(NTV2_CHANNEL2, (J2KStreamType)pKonaIPParams->streamType);

    // Now setup the J2K encoder and TS encapsulator for ch2
    rv = configTs2022.SetupJ2KEncoder(NTV2_CHANNEL2);
    rv = configTs2022.SetupTsForEncode(NTV2_CHANNEL2);

    // Start Encoders
    mDevice.WriteRegister(0x40000,0x01010000);
    std::cerr << "## NOTE:  Encoder is setup and running" << std::endl;

    return true;
}

void CKonaIpEncoderJsonReader::initMaps()
{

    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080i_5000, false))] = NTV2_FORMAT_1080i_5000;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080i_5994, false))] = NTV2_FORMAT_1080i_5994;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080i_6000, false))] = NTV2_FORMAT_1080i_6000;

    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2398, false))] = NTV2_FORMAT_1080p_2398;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2400, false))] = NTV2_FORMAT_1080p_2400;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2500, false))] = NTV2_FORMAT_1080p_2500;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2997, false))] = NTV2_FORMAT_1080p_2997;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_3000, false))] = NTV2_FORMAT_1080p_3000;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_5000, false))] = NTV2_FORMAT_1080p_5000;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_5994, false))] = NTV2_FORMAT_1080p_5994;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_6000, false))] = NTV2_FORMAT_1080p_6000;

    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_720p_2398, false))] = NTV2_FORMAT_720p_2398;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_720p_2500, false))] = NTV2_FORMAT_720p_2500;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_720p_5000, false))] = NTV2_FORMAT_720p_5000;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_720p_5994, false))] = NTV2_FORMAT_720p_5994;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_720p_6000, false))] = NTV2_FORMAT_720p_6000;

    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2K_2398, false))] = NTV2_FORMAT_1080p_2K_2398;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2K_2400, false))] = NTV2_FORMAT_1080p_2K_2400;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2K_2997, false))] = NTV2_FORMAT_1080p_2K_2997;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2K_3000, false))] = NTV2_FORMAT_1080p_2K_3000;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2K_5000, false))] = NTV2_FORMAT_1080p_2K_5000;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2K_5994, false))] = NTV2_FORMAT_1080p_2K_5994;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2K_6000, false))] = NTV2_FORMAT_1080p_2K_6000;

    chromaSubSamplingMap["444"] = 0;
    chromaSubSamplingMap["422-444"] = 1;
    chromaSubSamplingMap["422 standard"] = 2;

    streamTypeMap["Standard"] = 0;
    streamTypeMap["Evertz"] = 1;

    codeBlockSizeMap["32x32"] = 0;
    codeBlockSizeMap["32x64"] = 1;
    codeBlockSizeMap["64x32"] = 4;
    codeBlockSizeMap["64x64"] = 5;
    codeBlockSizeMap["128x32"] = 12;

}




