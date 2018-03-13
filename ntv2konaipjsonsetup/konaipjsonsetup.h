#ifndef KONAIPBOARD_H
#define KONAIPBOARD_H

#include "ntv2card.h"

#include <QJsonObject>
#include <QList>

#include "ntv2config2022.h"
#include "ntv2config2110.h"

typedef struct
{
    QString mSFPDesignator;
    QString mIPAddress;
    QString mSubnetMask;
    QString mGateway;

}SFPStruct;

typedef struct
{
    QString mChannelDesignator;
    QString mLinkAEnable;
    QString mLinkBEnable;
    QString mStream;
    QString mlinkASrcPort;
    QString mlinkASrcIPAddress;
    QString mlinkADestIPAddress;
    QString mlinkADestPort;
    QString mlinkAFilter;
    QString mlinkBSrcPort;
    QString mlinkBSrcIPAddress;
    QString mlinkBDestIPAddress;
    QString mlinkBDestPort;
    QString mlinkBFilter;
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
    QString mNumAudioChannels;
    QString mAudioPktInterval;
}ReceiveStruct;

typedef struct
{
    QString mChannelDesignator;
    QString mLinkAEnable;
    QString mLinkBEnable;
    QString mStream;
    QString mlinkALocalPort;
    QString mlinkARemoteIPAddress;
    QString mlinkARemotePort;
    QString mlinkBLocalPort;
    QString mlinkBRemoteIPAddress;
    QString mlinkBRemotePort;
    QString mSSRC;
    QString mTOS;
    QString mTTL;
    QString mPayload;
    QString mVideoFormat;
    QString mEnable;
    QString mPayloadLen;
    QString mLastPayloadLen;
    QString mPktsPerLine;
    QString mNumAudioChannels;
    QString mFirstAudioChannel;
    QString mAudioPktInterval;
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
    void dumpRx2110Config(const NTV2Channel channel, const NTV2Stream stream, rx_2110Config & rxConfig);

    KonaIPParamSetupStruct mKonaIPParams;

private:
    bool        mEnable2022_7;
    bool        mIs2110;
    bool        m4KMode;
    uint32_t    mNetworkPathDifferential;
    QString     mPTPMasterAddr;
};

#endif // KONAIPBOARD_H
