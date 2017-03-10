#ifndef KONAIPBOARD_H
#define KONAIPBOARD_H

#include "ntv2card.h"

#include <QJsonObject>
#include <QList>

typedef struct
{
    QString mSFPDesignator;
    QString mIPAddress;
    QString mSubnetMask;
    QString mRouter;
    QString mEnable2022_7;
}SFPStruct;

typedef struct
{
    QString mChannelDesignator;
    QString mPrimaryDestIPAddress;
    QString mPrimaryDestPort;
    QString mPrimaryFilter;
    QString mSecondaryDestIPAddress;
    QString mSecondaryDestPort;
    QString mSecondaryFilter;
    QString mNetworkPathDifferential;
    QString mPlayoutDelay;
    QString mEnable;
}ReceiveStruct;

typedef struct
{
    QString mChannelDesignator;
    QString mPrimaryLocalPort;
    QString mPrimaryRemoteIPAddress;
    QString mPrimaryRemotePort;
    QString mPrimaryRemoteMac;
    QString mPrimaryAutoMac;
    QString mSecondaryLocalPort;
    QString mSecondaryRemoteIPAddress;
    QString mSecondaryRemotePort;
    QString mSecondaryRemoteMac;
    QString mSecondaryAutoMac;
    QString mEnable;
}TransmitStruct;

typedef  struct {
    QList<SFPStruct> mSFPs;
    QList<ReceiveStruct> mReceiveChannels;
    QList<TransmitStruct> mTransmitChannels;
} KonaIPParamSetupStruct;


class CKonaIpJsonSetup
{
public:
    CKonaIpJsonSetup();

    bool openJson(QString fileName);
    bool setupBoard(std::string deviceSpec,KonaIPParamSetupStruct* pKonaIPParams);

    KonaIPParamSetupStruct* getKonaIParams() { return &mKonaIPParams; }

protected:
    bool readJson(const QJsonObject &json);

    KonaIPParamSetupStruct mKonaIPParams;

private:
    bool enable2022_7;
    bool is2110;
};

#endif // KONAIPBOARD_H
