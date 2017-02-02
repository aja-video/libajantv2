#ifndef KONAIPBOARD_H
#define KONAIPBOARD_H

#include "ntv2card.h"

#include <QJsonObject>
#include <QList>
#include <QMap>



typedef  struct {
    uint32_t channels;
    NTV2VideoFormat videoFormat;
    uint32_t bitDepth;
    uint32_t streamType;
    uint32_t chromaSubSampling;
    uint32_t mbps;
    uint32_t programPid;
    uint32_t videoPid;
    uint32_t pcrPid;
    uint32_t audio1Pid;
    bool ullMode;
} KonaIPParamSetupStruct;


class CKonaIpEncoderJsonReader
{
public:
    CKonaIpEncoderJsonReader();
    bool openJson(QString fileName);
    void printVideoFormatMap();

    KonaIPParamSetupStruct* getKonaIParams() { return &mKonaIPParams; }

protected:
    bool readJson(const QJsonObject &json);

    KonaIPParamSetupStruct mKonaIPParams;
    void initMaps();

    QMap<QString, NTV2VideoFormat> videoFormatMap;
    QMap<QString, uint32_t> streamTypeMap;
    QMap<QString, uint32_t> chromaSubSamplingMap;

};

class CKonaIPEncoderSetup
{
public:
    CKonaIPEncoderSetup();

    bool setupBoard(std::string pDeviceSpec,KonaIPParamSetupStruct* pKonaIPParams);

protected:


};
#endif // KONAIPBOARD_H
