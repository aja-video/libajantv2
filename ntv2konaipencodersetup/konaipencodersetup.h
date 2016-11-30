#ifndef KONAIPBOARD_H
#define KONAIPBOARD_H

#include "ntv2card.h"

#include <QJsonObject>
#include <QList>
#include <QMap>



typedef  struct {
    NTV2VideoFormat videoFormat;
    uint32_t numBits;
    uint32_t streamType;
    uint32_t chromaSubSampling;
    uint32_t codeBlockSize;
    uint32_t Mbps;
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
    QMap<QString, uint32_t> codeBlockSizeMap;

};

class CKonaIPEncoderSetup
{
public:
    CKonaIPEncoderSetup();

    bool setupBoard(std::string pDeviceSpec,KonaIPParamSetupStruct* pKonaIPParams);

protected:


};
#endif // KONAIPBOARD_H
