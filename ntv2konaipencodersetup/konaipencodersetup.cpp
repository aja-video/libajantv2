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

    mKonaIPParams.channels = pjson["Channels"].toInt();
    std::cout << "Channels       " << mKonaIPParams.channels << std::endl;

    mKonaIPParams.videoFormat = videoFormatMap[vf];
    std::cout << "VideoFormat    " << vf.toStdString().c_str() << std::endl;

    QString st =  pjson["StreamType"].toString();
    mKonaIPParams.streamType = streamTypeMap[st];
    std::cout << "StreamType     " << st.toStdString().c_str() << std::endl;

    QString css =  pjson["ChromaSampling"].toString();
    mKonaIPParams.chromaSubSampling = chromaSubSamplingMap[css];
    std::cout << "ChromaSampling " << css.toStdString().c_str() << std::endl;

    mKonaIPParams.bitDepth = pjson["BitDepth"].toInt();
    std::cout << "BitDepth       " << mKonaIPParams.bitDepth << std::endl;
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
    bool rv;
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

    CNTV2Config2022     config2022 (mDevice);
    j2kEncoderConfig    encoderCfg;

    // retrieve encode params
    encoderCfg.videoFormat     = (NTV2VideoFormat)pKonaIPParams->videoFormat;
    encoderCfg.ullMode         = 0;
    encoderCfg.bitDepth        = pKonaIPParams->bitDepth;
    encoderCfg.chromaSubsamp   = (J2KChromaSubSampling)pKonaIPParams->chromaSubSampling;
    encoderCfg.streamType      = (J2KStreamType)pKonaIPParams->streamType;
    encoderCfg.mbps            = pKonaIPParams->mbps;
    encoderCfg.pmtPid          = pKonaIPParams->programPid;
    encoderCfg.videoPid        = pKonaIPParams->videoPid;
    encoderCfg.pcrPid          = pKonaIPParams->pcrPid;
    encoderCfg.audio1Pid       = pKonaIPParams->audio1Pid;

    // For the J2K encoder we only configure output channels NTV2_CHANNEL1 and NTV2_CHANNEL2
    if (pKonaIPParams->channels & 1)
    {
        rv = config2022.SetJ2KEncoderConfiguration(NTV2_CHANNEL1, encoderCfg);
    }

    if (pKonaIPParams->channels & 2)
    {
        rv = config2022.SetJ2KEncoderConfiguration(NTV2_CHANNEL2, encoderCfg);
    }

    std::cerr << "## NOTE:  Encoder is setup and running" << std::endl;

    return true;
}

void CKonaIpEncoderJsonReader::initMaps()
{

    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_525_5994, false))] = NTV2_FORMAT_525_5994;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_625_5000, false))] = NTV2_FORMAT_625_5000;

    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080i_5000, false))] = NTV2_FORMAT_1080i_5000;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080i_5994, false))] = NTV2_FORMAT_1080i_5994;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080i_6000, false))] = NTV2_FORMAT_1080i_6000;

    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2398,   false))] = NTV2_FORMAT_1080p_2398;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2400,   false))] = NTV2_FORMAT_1080p_2400;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2500,   false))] = NTV2_FORMAT_1080p_2500;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_2997,   false))] = NTV2_FORMAT_1080p_2997;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_3000,   false))] = NTV2_FORMAT_1080p_3000;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_5000_A, false))] = NTV2_FORMAT_1080p_5000_A;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_5994_A, false))] = NTV2_FORMAT_1080p_5994_A;
    videoFormatMap[QString::fromStdString(NTV2VideoFormatToString(NTV2_FORMAT_1080p_6000_A, false))] = NTV2_FORMAT_1080p_6000_A;

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
    chromaSubSamplingMap["422-Standard"] = 2;

    streamTypeMap["Standard"] = 0;
    streamTypeMap["Non-elsm"] = 1;

}




