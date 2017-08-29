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

}SFPStruct;

typedef struct
{
    QString mChannelDesignator;
    QString mLinkAEnable;
    QString mLinkBEnable;
    QString mStream;
    QString mPrimarySrcPort;
    QString mPrimarySrcIPAddress;
    QString mPrimaryDestIPAddress;
    QString mPrimaryDestPort;
    QString mPrimaryFilter;
    QString mSecondarySrcPort;
    QString mSecondarySrcIPAddress;
    QString mSecondaryDestIPAddress;
    QString mSecondaryDestPort;
    QString mSecondaryFilter;
    QString mNetworkPathDifferential;
    QString mPlayoutDelay;
    QString mVLAN;
    QString mSSRC;
    QString mPayload;
    QString mVideoFormat;
    QString mEnable;
    QString mPayloadLen;
    QString mLastPayloadLen;
    QString mPktsPerLine;
}ReceiveStruct;

typedef struct
{
    QString mChannelDesignator;
    QString mLinkAEnable;
    QString mLinkBEnable;
    QString mStream;
    QString mPrimaryLocalPort;
    QString mPrimaryRemoteIPAddress;
    QString mPrimaryRemotePort;
    QString mSecondaryLocalPort;
    QString mSecondaryRemoteIPAddress;
    QString mSecondaryRemotePort;
    QString mSSRC;
    QString mTOS;
    QString mTTL;
    QString mPayload;
    QString mVideoFormat;
    QString mEnable;
    QString mPayloadLen;
    QString mLastPayloadLen;
    QString mPktsPerLine;
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
    bool setupBoard(std::string deviceSpec);

protected:
    bool setupBoard2110(std::string deviceSpec);
    bool setupBoard2022(std::string deviceSpec);
    bool readJson(const QJsonObject &json);

    KonaIPParamSetupStruct mKonaIPParams;

private:
    bool        mEnable2022_7;
    bool        mIs2110;
    uint32_t    mNetworkPathDifferential;
    QString     mPTPMasterAddr;
};

#endif // KONAIPBOARD_H
