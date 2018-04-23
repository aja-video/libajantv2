#ifndef KONAIPBOARD_H
#define KONAIPBOARD_H

#include "ntv2card.h"
#include "konaipjsonparse.h"

#include <QJsonObject>
#include <QList>

#include "ntv2config2022.h"
#include "ntv2config2110.h"

typedef struct
{
    QString mSfpDesignator;
    QString mIPAddress;
    QString mSubnetMask;
    QString mGateway;
    QString mEnable;
}SFPStruct;

typedef struct
{
    QString mChannelDesignator;
    QString mSfp1SrcIPAddress;
    QString mSfp1SrcPort;
    QString mSfp1DestIPAddress;
    QString mSfp1DestPort;
    QString mSfp1Filter;
    QString mSfp2SrcIPAddress;
    QString mSfp2SrcPort;
    QString mSfp2DestIPAddress;
    QString mSfp2DestPort;
    QString mSfp2Filter;
    QString mPlayoutDelay;
    QString mVLAN;
    QString mSSRC;
    QString mSfp1Enable;
    QString mSfp2Enable;
    QString mEnable;
}ReceiveStruct2022;

typedef struct
{
    QString mChannelDesignator;
    QString mSfp1RemoteIPAddress;
    QString mSfp1RemotePort;
    QString mSfp1LocalPort;
    QString mSfp2RemoteIPAddress;
    QString mSfp2RemotePort;
    QString mSfp2LocalPort;
    QString mTOS;
    QString mTTL;
    QString mSSRC;
    QString mSfp1Enable;
    QString mSfp2Enable;
    QString mEnable;
}TransmitStruct2022;

typedef  struct {
    QList<SFPStruct> mSFPs;
    QList<ReceiveStruct2022> mReceive2022Channels;
    QList<TransmitStruct2022> mTransmit2022Channels;
} KonaIP2022ParamSetupStruct;


class CKonaIpJsonSetup
{
public:
    CKonaIpJsonSetup();

    bool openJson(QString fileName);
    bool setupBoard(std::string deviceSpec);

protected:
    bool setupBoard2110(std::string deviceSpec);
    bool setupBoard2022(std::string deviceSpec);
    bool readJson2022(const QJsonObject &json);

    KonaIP2022ParamSetupStruct mKonaIP2022Params;

private:
    bool                    mEnable2022_7;
    bool                    mIs2110;
    bool                    m4KMode;
    uint32_t                mNetworkPathDifferential;
    QString                 mPTPMasterAddr;

    CKonaIpJsonParse2110    parse2110;
};

#endif // KONAIPBOARD_H
