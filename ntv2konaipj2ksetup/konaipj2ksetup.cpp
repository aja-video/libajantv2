#include "konaipj2ksetup.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"
#include "ntv2devicescanner.h"
#include "ntv2configts2022.h"


CKonaIpJ2kJsonReader::CKonaIpJ2kJsonReader()
{
    initMaps();
}

bool CKonaIpJ2kJsonReader::readJson(const QJsonObject &json)
{
    mKonaIpJ2kParams.mDecoder.clear();
    QJsonArray decoderArray = json["decoder"].toArray();
    for (int decoderIndex = 0; decoderIndex < decoderArray.size(); ++decoderIndex)
    {
        std::cout << "Decoder" << std::endl;

        QJsonObject pDecoder = decoderArray[decoderIndex].toObject();
        DecoderStruct decoderStruct;

        decoderStruct.selectionMode = pDecoder["SelectionMode"].toInt();
        std::cout << "SelectionMode    " << decoderStruct.selectionMode << std::endl;
        decoderStruct.programNumber = pDecoder["ProgramNumber"].toInt();
        std::cout << "ProgramNumber    " << decoderStruct.selectionMode << std::endl;
        decoderStruct.programPID = pDecoder["ProgramPID"].toInt();
        std::cout << "ProgramPID       " << decoderStruct.selectionMode << std::endl;
        decoderStruct.audioNumber = pDecoder["AudioNumber"].toInt();
        std::cout << "AudioNumber      " << decoderStruct.selectionMode << std::endl;
        mKonaIpJ2kParams.mDecoder.append(decoderStruct);
    }

    mKonaIpJ2kParams.mEncoder.clear();
    QJsonArray encoderArray = json["encoder"].toArray();
    for (int encoderIndex = 0; encoderIndex < encoderArray.size(); ++encoderIndex)
    {
        std::cout << "Encoder" << std::endl;

        QJsonObject pEncoder = encoderArray[encoderIndex].toObject();
        EncoderStruct encoderStruct;

        encoderStruct.channels = pEncoder["Channels"].toInt();
        std::cout << "Channels       " << encoderStruct.channels << std::endl;

        QString vf =  pEncoder["VideoFormat"].toString();
        encoderStruct.videoFormat = videoFormatMap[vf];
        std::cout << "VideoFormat    " << vf.toStdString().c_str() << std::endl;

        QString st =  pEncoder["StreamType"].toString();
        encoderStruct.streamType = streamTypeMap[st];
        std::cout << "StreamType     " << st.toStdString().c_str() << std::endl;

        QString css =  pEncoder["ChromaSampling"].toString();
        encoderStruct.chromaSubSampling = chromaSubSamplingMap[css];
        std::cout << "ChromaSampling " << css.toStdString().c_str() << std::endl;

        encoderStruct.bitDepth = pEncoder["BitDepth"].toInt();
        std::cout << "BitDepth       " << encoderStruct.bitDepth << std::endl;
        encoderStruct.mbps = pEncoder["Mbps"].toInt();
        std::cout << "Mbps           " << encoderStruct.mbps << std::endl;
        encoderStruct.audioChannels = pEncoder["AudioChannels"].toInt();
        std::cout << "AudioChannels  " << encoderStruct.audioChannels << std::endl;

        encoderStruct.programPid = pEncoder["ProgramPid"].toInt();
        std::cout << "ProgramPid     " << encoderStruct.programPid << std::endl;
        encoderStruct.videoPid = pEncoder["VideoPid"].toInt();
        std::cout << "VideoPid       " << encoderStruct.videoPid << std::endl;
        encoderStruct.pcrPid = pEncoder["PcrPid"].toInt();
        std::cout << "PcrPid         " << encoderStruct.pcrPid << std::endl;
        encoderStruct.audio1Pid = pEncoder["Audio1Pid"].toInt();
        std::cout << "Audio1Pid      " << encoderStruct.audio1Pid << std::endl << std::endl;
        encoderStruct.ullMode = false;

        mKonaIpJ2kParams.mEncoder.append(encoderStruct);
    }
    return true;
}

bool CKonaIpJ2kJsonReader::openJson(QString fileName)
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

void CKonaIpJ2kJsonReader::printVideoFormatMap()
{
    QMap<QString, NTV2VideoFormat>::iterator i;
    for (i = videoFormatMap.begin(); i != videoFormatMap.end(); ++i)
    {
        QString str = i.key();
        std::cout << str.toStdString() << std::endl;
    }
}

void CKonaIpJ2kJsonReader::initMaps()
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

    chromaSubSamplingMap["444"] = 0;
    chromaSubSamplingMap["422-444"] = 1;
    chromaSubSamplingMap["422-Standard"] = 2;

    streamTypeMap["Standard"] = 0;
    streamTypeMap["Non-elsm"] = 1;

}


bool CKonaIpEncoderSetup::setupBoard(std::string pDeviceSpec, KonaIPParamJ2KSetupStruct* pKonaIpJ2kParams)
{
    bool rv;
    CNTV2Card mDevice;
    CNTV2DeviceScanner::GetFirstDeviceFromArgument (pDeviceSpec, mDevice);
    if (!mDevice.IsOpen())
    {std::cerr << "## ERROR:  No devices found" << std::endl;  return false;}
    //if (!mDevice.IsKonaIPDevice ())
    //{std::cerr << "## ERROR:  Not a KONA IP device" << std::endl;  return false;}

    //	Read MicroBlaze Uptime in Seconds, to see if it's running...
    while (!mDevice.IsMBSystemReady())
    {
        std::cout << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << std::endl;
        mDevice.SleepMs (1000);
        if (mDevice.IsMBSystemReady ())
        {
            std::cout << "## NOTE:  Device is ready" << std::endl;
            if (!mDevice.IsMBSystemValid())
            {
                std::cerr << "## ERROR: board firmware package is incompatible with this application" << std::endl;
                return false;
            }
        }
    }

    if (pKonaIpJ2kParams->mEncoder.size() == 0)
    {
        std::cerr << "Not setting up encoder, no data" << std::endl;
        return false;
    }

    if (pKonaIpJ2kParams->mEncoder.size() > 1)
    {
        std::cerr << "Multiple encoders not supported at this time" << std::endl;
        return false;
    }

    QListIterator<EncoderStruct> encoderIter(pKonaIpJ2kParams->mEncoder);
    while (encoderIter.hasNext())
    {
        std::cerr << "## encoderIter did" << std::endl;

        EncoderStruct encoder = encoderIter.next();
        CNTV2Config2022     config2022 (mDevice);
        j2kEncoderConfig    encoderCfg;

        // retrieve encode params
        encoderCfg.videoFormat     = (NTV2VideoFormat)encoder.videoFormat;
        encoderCfg.ullMode         = 0;
        encoderCfg.bitDepth        = encoder.bitDepth;
        encoderCfg.chromaSubsamp   = (J2KChromaSubSampling)encoder.chromaSubSampling;
        encoderCfg.streamType      = (J2KStreamType)encoder.streamType;
        encoderCfg.mbps            = encoder.mbps;
        encoderCfg.audioChannels   = encoder.audioChannels;
        encoderCfg.pmtPid          = encoder.programPid;
        encoderCfg.videoPid        = encoder.videoPid;
        encoderCfg.pcrPid          = encoder.pcrPid;
        encoderCfg.audio1Pid       = encoder.audio1Pid;

        // For the J2K encoder we only configure output channels NTV2_CHANNEL1 and NTV2_CHANNEL2
        if (encoder.channels & 1)
        {
            rv = config2022.SetJ2KEncoderConfiguration(NTV2_CHANNEL1, encoderCfg);
        }

        if (encoder.channels & 2)
        {
            rv = config2022.SetJ2KEncoderConfiguration(NTV2_CHANNEL2, encoderCfg);
        }
    }

    std::cerr << "## NOTE:  Encoder is setup and running" << std::endl;

    return true;
}


bool CKonaIpDecoderSetup::setupBoard(std::string pDeviceSpec, KonaIPParamJ2KSetupStruct* pKonaIpJ2kParams)
{
    bool rv;
    CNTV2Card mDevice;
    CNTV2DeviceScanner::GetFirstDeviceFromArgument (pDeviceSpec, mDevice);
    if (!mDevice.IsOpen())
    {std::cerr << "## ERROR:  No devices found" << std::endl;  return false;}
    //if (!mDevice.IsKonaIPDevice ())
    //{std::cerr << "## ERROR:  Not a KONA IP device" << std::endl;  return false;}

    //	Read MicroBlaze Uptime in Seconds, to see if it's running...
    while (!mDevice.IsMBSystemReady())
    {
        std::cout << "## NOTE:  Waiting for device to become ready... (Ctrl-C will abort)" << std::endl;
        mDevice.SleepMs (1000);
        if (mDevice.IsMBSystemReady ())
        {
            std::cout << "## NOTE:  Device is ready" << std::endl;
            if (!mDevice.IsMBSystemValid())
            {
                std::cerr << "## ERROR: board firmware package is incompatible with this application" << std::endl;
                return false;
            }
        }
    }

    if (pKonaIpJ2kParams->mDecoder.size() == 0)
    {
        std::cerr << "Not setting up decoder, no data" << std::endl;
        return false;
    }

    if (pKonaIpJ2kParams->mDecoder.size() > 1)
    {
        std::cerr << "Multiple decoders not supported at this time" << std::endl;
        return false;
    }

    QListIterator<DecoderStruct> decoderIter(pKonaIpJ2kParams->mDecoder);
    while (decoderIter.hasNext())
    {
        std::cerr << "## decoderIter did" << std::endl;

        DecoderStruct decoder = decoderIter.next();
        CNTV2Config2022     config2022 (mDevice);
        j2kDecoderConfig    decoderCfg;

        // retrieve decode params
        decoderCfg.selectionMode    = (j2kDecoderConfig::eProgSelMode_t)decoder.selectionMode;
        decoderCfg.programNumber    = decoder.programNumber;
        decoderCfg.programPID       = decoder.programPID;
        decoderCfg.audioNumber      = decoder.audioNumber;

        rv = config2022.SetJ2KDecoderConfiguration(decoderCfg);
    }

    std::cerr << "## NOTE:  Decoder is setup and running" << std::endl;

    return true;
}



