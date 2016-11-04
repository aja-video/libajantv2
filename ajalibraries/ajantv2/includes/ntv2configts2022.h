/**
    @file		ntv2configts2022.h
    @brief		Declares the CNTV2ConfigTs2022 class.
	@copyright	(C) 2014-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2_2022CONFIGTS_H
#define NTV2_2022CONFIGTS_H

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2registers2022.h"
#include "ntv2mbcontroller.h"
#include "ntv2tshelper.h"
#include <string.h>


// Encoder part numbers
#define     ENCODE_TS_TIMER                 0
#define     ENCODE_TS_J2K_ENCODER           1
#define     ENCODE_TS_MPEG_J2K_ENCAP        2
#define     ENCODE_TS_AES_ENCAP             3
#define     ENCODE_TS_MPEG_AES_ENCAP        4
#define     ENCODE_TS_MPEG_ANC_ENCAP        6
#define     ENCODE_TS_MPEG_PCR              7

// Decoder part numbers
#define     DECODE_TS_MPEG_J2K_DECAP        0
#define     DECODE_TS_J2K_DECODER           1
#define     DECODE_TS_MPEG_AES_DECAP        2
#define     DECODE_TS_AES_DECAP             3
#define     DECODE_TS_MPEG_ANC_DECAP        5


#define     PES_HDR_LOOKUP                  0x0
#define     PES_HDR_LEN                     0xc0
#define     PTS_OFFSET                      0xc1
#define     J2K_TS_OFFSET                   0xc2
#define     AUF1_OFFSET                     0xc3
#define     AUF2_OFFSET                     0xc4
#define     J2K_TS_LOAD                     0xd0
#define     TS_GEN_TC                       0xd1
#define     HOST_EN                         0xe0
#define     INTERLACED_VIDEO                0xe1
#define     PAYLOAD_PARAMS                  0xe2
#define     LOOPBACK                        0xe3
#define     PAT_TABLE_LOOKUP                0x100
#define     PAT_PMT_PERIOD                  0x1f0
#define     PMT_TABLE_LOOKUP                0x200
#define     ADAPTATION_LOOKUP               0x300
#define     ADAPTATION_HDR_LENGTH           0x3f0

class j2k_encode_2022_channel
{
public:
    j2k_encode_2022_channel()
    {
        videoFormat     = NTV2_FORMAT_720p_5994;
        ullMode         = false;
        bitDepth        = 10;
        chromaSubsamp   = kJ2KChromaSubSamp_422_Standard;
        codeBlocksize   = kJ2KCodeBlocksize_32x32;
        mbps            = 200;
        streamType      = kJ2KStreamTypeStandard;
    }

    bool operator != ( const j2k_encode_2022_channel &other ) {
        return !(*this == other);
    }

    bool operator == ( const j2k_encode_2022_channel &other )
    {
        if ((videoFormat        == other.videoFormat)       &&
            (ullMode            == other.ullMode)           &&
            (bitDepth           == other.bitDepth)          &&
            (chromaSubsamp      == other.chromaSubsamp)     &&
            (codeBlocksize      == other.codeBlocksize)     &&
            (mbps               == other.mbps)              &&
            (streamType         == other.streamType))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

public:
    NTV2VideoFormat         videoFormat;        ///< @brief	Specifies the video format for J2K encode.
    bool                    ullMode;            ///< @brief	Specifies the ull mode for J2K encode.
    uint32_t                bitDepth;           ///< @brief	Specifies the bit depth for J2K encode.
    J2KChromaSubSampling    chromaSubsamp;      ///< @brief	Specifies the chroma sub sampling for J2K encode.
    J2KCodeBlocksize        codeBlocksize;      ///< @brief	Specifies the code block size for J2K encode.
    uint32_t                mbps;               ///< @brief	Specifies the mbits per-second for J2K encode.
    J2KStreamType           streamType;         ///< @brief	Specifies the stream type for J2K encode.
};

struct j2kEncodeConfig
{
    NTV2VideoFormat         j2k_videoFormat;
    bool                    j2k_ullMode;
    uint32_t                j2k_bitDepth;
    J2KChromaSubSampling    j2k_chromaSubsamp;
    J2KCodeBlocksize        j2k_codeBlocksize;
    uint32_t                j2k_mbps;
    J2KStreamType           j2k_streamType;
};


class j2k_decode_2022_channel
{
public:
    j2k_decode_2022_channel()
    {
        pid     = 0;
    }

    bool operator != ( const j2k_decode_2022_channel &other ) {
        return !(*this == other);
    }

    bool operator == ( const j2k_decode_2022_channel &other )
    {
        return pid == other.pid;
    }

public:
    uint32_t                pid;                ///< @brief	Specifies the pid for J2K decode.
};

struct j2kDecodeConfig
{
    uint32_t                j2k_pid;
};

/**
    @brief	The CNTV2ConfigTs2022 class is the interface to Kona-IP SMPTE 2022 J2K encoder and TS chips
**/

class AJAExport CNTV2ConfigTs2022 : public CNTV2MBController
{
public:
    CNTV2ConfigTs2022 (CNTV2Card & device);

    // Set/Get NTV2 video format for TS and J2K setup
    void    SetJ2KEncodeVideoFormat(const NTV2Channel channel, const NTV2VideoFormat format) {_j2kEncodeConfig[channel].j2k_videoFormat = format;}
    void    GetJ2KEncodeVideoFormat(const NTV2Channel channel, NTV2VideoFormat & format) {format = _j2kEncodeConfig[channel].j2k_videoFormat;}

    // Set/Get ULL mode for J2K setup
    void    SetJ2KEncodeUllMode(const NTV2Channel channel, const bool ull) {_j2kEncodeConfig[channel].j2k_ullMode = ull;}
    void    GetJ2KEncodeUllMode(const NTV2Channel channel, bool & ull) {ull = _j2kEncodeConfig[channel].j2k_ullMode;}

    // Set/Get bit depth for J2K setup
    void    SetJ2KEncodeBitDepth(const NTV2Channel channel, const uint32_t bitDepth) {_j2kEncodeConfig[channel].j2k_bitDepth = bitDepth;}
    void    GetJ2KEncodeBitDepth(const NTV2Channel channel, uint32_t & bitDepth) {bitDepth = _j2kEncodeConfig[channel].j2k_bitDepth;}

    // Set/Get chroma sub sampling for J2K setup
    void    SetJ2KEncodeChromaSubsamp(const NTV2Channel channel, const J2KChromaSubSampling subSamp) {_j2kEncodeConfig[channel].j2k_chromaSubsamp = subSamp;}
    void    GetJ2KEncodeChromaSubsamp(const NTV2Channel channel, J2KChromaSubSampling & subSamp) {subSamp = _j2kEncodeConfig[channel].j2k_chromaSubsamp;}

    // Set/Get code block size for J2K setup
    void    SetJ2KEncodeCodeBlocksize(const NTV2Channel channel, const J2KCodeBlocksize codeBlocksize) {_j2kEncodeConfig[channel].j2k_codeBlocksize = codeBlocksize;}
    void    GetJ2KEncodeCodeBlocksize(const NTV2Channel channel, J2KCodeBlocksize & codeBlocksize) {codeBlocksize = _j2kEncodeConfig[channel].j2k_codeBlocksize;}

    // Set/Get encode rate in MBPS for J2K setup
    void    SetJ2KEncodeMbps(const NTV2Channel channel, const uint32_t mbps) {_j2kEncodeConfig[channel].j2k_mbps = mbps;}
    void    GetJ2KEncodeMbps(const NTV2Channel channel, uint32_t & mbps) {mbps = _j2kEncodeConfig[channel].j2k_mbps;}

    // Set/Get stream type for J2K setup
    void    SetJ2KEncodeStreamType(const NTV2Channel channel, const J2KStreamType streamType) {_j2kEncodeConfig[channel].j2k_streamType = streamType;}
    void    GetJ2KEncodeStreamType(const NTV2Channel channel, J2KStreamType & streamType) {streamType = _j2kEncodeConfig[channel].j2k_streamType;}

    // Set/Get encode rate in MBPS for J2K setup
    void    SetJ2KDecodePid(const NTV2Channel channel, const uint32_t pid) {_j2kDecodeConfig[channel].j2k_pid = pid;}
    void    GetJ2KDecodePid(const NTV2Channel channel, uint32_t & pid) {pid = _j2kDecodeConfig[channel].j2k_pid;}

    // Setup the J2K encoder
    bool    SetupJ2KEncoder(const NTV2Channel channel);

    // Setup the J2K decoder
    bool    SetupJ2KDecoder();

    // Setup the TS encode parts
    bool    SetupTsForEncode(const NTV2Channel channel);

    // Setup the TS decode parts
    bool    SetupTsForDecode();

    // If method returns false call this to get details
    std::string getLastError();


 private:
    // These routines are used to setup all the individual TS parts for encode and decode

    // Setup individual TS encode parts
    bool    SetupEncodeTsTimer(const NTV2Channel channel);
    bool    SetupEncodeTsJ2KEncoder(const NTV2Channel channel);
    bool    SetupEncodeTsMpegJ2kEncap(const NTV2Channel channel);
    bool    SetupEncodeTsAesEncap(const NTV2Channel channel);
    bool    SetupEncodeTsMpegAesEncap(const NTV2Channel channel);
    bool    SetupEncodeTsMpegAncEncap();

    // Setup individual TS decode parts
    bool    SetupDecodeTsMpegJ2kDecap();
    bool    SetupDecodeTsJ2KDecoder();
    bool    SetupDecodeTsMpegAesDecap();
    bool    SetupDecodeTsAesDecap();
    bool    SetupDecodeTsMpegAncDecap();
    
    // Routines to talk to the J2K part
    bool                J2kCanAcceptCmd(const NTV2Channel channel);
    void                J2kSetParam(const NTV2Channel channel, uint32_t config, uint32_t param, uint32_t value);
    void                J2kSetMode(const NTV2Channel channel, uint32_t tier, uint32_t mode);
    uint32_t            GetFeatures();

    bool                GenerateTransactionTableForMpegJ2kEncap(const NTV2Channel channel);
    uint32_t            GetIpxJ2KAddr(const NTV2Channel channel);
    uint32_t            GetIpxTsAddr(const NTV2Channel channel);

    CNTV2TsHelper       _tsHelper;
    bool                _is2022_6;
    bool                _is2022_2;

    j2kEncodeConfig     _j2kEncodeConfig[NTV2_MAX_NUM_CHANNELS];
    j2kDecodeConfig     _j2kDecodeConfig[NTV2_MAX_NUM_CHANNELS];

    int32_t             _transactionTable[1024][2];
    int32_t             _transactionCount;

};	//	CNTV2ConfigTs2022


struct tsSetupReg
{
    uint32_t reg;
    uint32_t value;
};

static const tsSetupReg tsAesEncapTable[] =
{
    {0x000, 0x2},
    {0x008, 0x1},
};
#define numTsAesEncapEntries (sizeof(tsAesEncapTable) / sizeof(tsSetupReg))

static const tsSetupReg tsMpegAesEncapTable[] =
{
    {0x000, 0x47},
    {0x001, 0x41},
    {0x002, 0x2},
    {0x003, 0x10},
    {0x004, 0x0},
    {0x005, 0x0},
    {0x006, 0x1},
    {0x007, 0xbd},
    {0x008, 0x0},
    {0x009, 0x0},
    {0x00a, 0x80},
    {0x00b, 0x80},
    {0x00c, 0x5},
    {0x00d, 0x21},
    {0x00e, 0x0},
    {0x00f, 0x1},
    {0x010, 0x0},
    {0x011, 0x1},
    {0x012, 0x0},
    {0x013, 0x0},
    {0x014, 0x0},
    {0x015, 0x10},

    {0x0c0, 0x16},
    {0x0c1, 0xd},
    {0x0c3, 0x1000012},
    {0x0c4, 0x1000c08},

    {0x300, 0x47},               // adaptation header
    {0x301, 0x1},
    {0x302, 0x2},
    {0x303, 0x30},
    {0x304, 0x0},
    {0x305, 0x0},

    {0x3f0, 0x6},

    {0x0e2, 0x102},
    {0x0e0, 0x1},

};
#define numTsMpegAesEncapEntries (sizeof(tsMpegAesEncapTable) / sizeof(tsSetupReg))



#endif // NTV2_2022CONFIGTS_H
