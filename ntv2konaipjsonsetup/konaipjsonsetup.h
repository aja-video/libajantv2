#ifndef KONAIPBOARD_H
#define KONAIPBOARD_H

#include "ntv2card.h"

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
    QString mVLAN;
    QString mSSRC;
    QString mPayload;
    QString mVideoFormat;
    QString mSfp1Enable;
    QString mSfp2Enable;
    QString mEnable;
}ReceiveStructVideo2110;

typedef struct
{
    QString mChannelDesignator;
    QString mStream;
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
    QString mVLAN;
    QString mSSRC;
    QString mPayload;
    QString mNumAudioChannels;
    QString mAudioPktInterval;
    QString mSfp1Enable;
    QString mSfp2Enable;
    QString mEnable;
}ReceiveStructAudio2110;

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

typedef struct
{
    QString mChannelDesignator;
    QString mSfp1RemoteIPAddress;
    QString mSfp1RemotePort;
    QString mSfp1LocalPort;
    QString mSfp2RemoteIPAddress;
    QString mSfp2RemotePort;
    QString mSfp2LocalPort;
    QString mVideoFormat;
    QString mPayload;
    QString mSSRC;
    QString mTTL;
    QString mSfp1Enable;
    QString mSfp2Enable;
    QString mEnable;
}TransmitStructVideo2110;

typedef struct
{
    QString mChannelDesignator;
    QString mStream;
    QString mSfp1RemoteIPAddress;
    QString mSfp1RemotePort;
    QString mSfp1LocalPort;
    QString mSfp2RemoteIPAddress;
    QString mSfp2RemotePort;
    QString mSfp2LocalPort;
    QString mPayload;
    QString mSSRC;
    QString mTTL;
    QString mNumAudioChannels;
    QString mFirstAudioChannel;
    QString mAudioPktInterval;
    QString mSfp1Enable;
    QString mSfp2Enable;
    QString mEnable;
}TransmitStructAudio2110;

typedef  struct {
    QList<SFPStruct> mSFPs;
    QList<ReceiveStruct2022> mReceive2022Channels;
    QList<TransmitStruct2022> mTransmit2022Channels;
} KonaIP2022ParamSetupStruct;

typedef  struct {
    QList<SFPStruct> mSFPs;
    QList<ReceiveStructVideo2110> mReceiveVideo2110Channels;
    QList<ReceiveStructAudio2110> mReceiveAudio2110Channels;
    QList<TransmitStructVideo2110> mTransmitVideo2110Channels;
    QList<TransmitStructAudio2110> mTransmitAudio2110Channels;
} KonaIP2110ParamSetupStruct;


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
    bool readJson2110(const QJsonObject &json);
    void dumpRx2110Config(const NTV2Channel channel, const NTV2Stream stream, rx_2110Config & rxConfig);

    KonaIP2022ParamSetupStruct mKonaIP2022Params;
    KonaIP2110ParamSetupStruct mKonaIP2110Params;

private:
    bool        mEnable2022_7;
    bool        mIs2110;
    bool        m4KMode;
    uint32_t    mNetworkPathDifferential;
    QString     mPTPMasterAddr;
};

#endif // KONAIPBOARD_H
