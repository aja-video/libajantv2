#ifndef KONAIPBOARD_H
#define KONAIPBOARD_H

#include "ntv2card.h"

#include <QJsonObject>
#include <QList>
#include <QMap>

typedef struct
{
    uint32_t channels;
    NTV2VideoFormat videoFormat;
    uint32_t bitDepth;
    uint32_t streamType;
    uint32_t chromaSubSampling;
    uint32_t mbps;
    uint32_t audioChannels;
    uint32_t programPid;
    uint32_t videoPid;
    uint32_t pcrPid;
    uint32_t audio1Pid;
    bool ullMode;
} EncoderStruct;

typedef struct
{
    uint32_t selectionMode;
    uint32_t programNumber;
    uint32_t programPID;
    uint32_t audioNumber;
}DecoderStruct;

typedef  struct
{
    QList<EncoderStruct> mEncoder;
    QList<DecoderStruct> mDecoder;
} KonaIPParamJ2KSetupStruct;


class CKonaIpJ2kJsonReader
{
public:
    CKonaIpJ2kJsonReader();
    bool openJson(QString fileName);
    void printVideoFormatMap();

    KonaIPParamJ2KSetupStruct* getKonaIpJ2kParams() { return &mKonaIpJ2kParams; }

protected:
    bool readJson(const QJsonObject &json);
    void initMaps();

    KonaIPParamJ2KSetupStruct mKonaIpJ2kParams;

    QMap<QString, NTV2VideoFormat> videoFormatMap;
    QMap<QString, uint32_t> streamTypeMap;
    QMap<QString, uint32_t> chromaSubSamplingMap;

};

class CKonaIpEncoderSetup
{
public:
    CKonaIpEncoderSetup(){};

    bool setupBoard(std::string pDeviceSpec, KonaIPParamJ2KSetupStruct* pKonaIpJ2kParams);
};

class CKonaIpDecoderSetup
{
public:
    CKonaIpDecoderSetup(){};

    bool setupBoard(std::string pDeviceSpec, KonaIPParamJ2KSetupStruct* pKonaIpJ2kParams);
};

#endif // KONAIPBOARD_H
