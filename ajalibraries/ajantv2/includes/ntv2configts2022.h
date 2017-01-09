/**
    @file		ntv2configts2022.h
    @brief		Declares the CNTV2ConfigTs2022 class.
	@copyright	(C) 2014-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2_2022CONFIGTS_H
#define NTV2_2022CONFIGTS_H

#include "ntv2card.h"
#include "ntv2enums.h"
#include "ntv2registers2022.h"
#include "ntv2mbcontroller.h"
#include "ntv2tshelper.h"
#include "ntv2config2022.h"
#include <string.h>

// Encoder part numbers
#define     ENCODE_TS_TIMER                 0
#define     ENCODE_TS_J2K_ENCODER           1
#define     ENCODE_TS_MPEG_J2K_ENCAP        2
#define     ENCODE_TS_AES_ENCAP             3
#define     ENCODE_TS_MPEG_AES_ENCAP        4
#define     ENCODE_TS_MPEG_ANC_ENCAP        6
#define     ENCODE_TS_MPEG_PCR_ENCAP        7

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
#define     PACKET_RATE                     0xca
#define     HOST_EN                         0xe0
#define     INTERLACED_VIDEO                0xe1
#define     PAYLOAD_PARAMS                  0xe2
#define     LOOPBACK                        0xe3
#define     PAT_TABLE_LOOKUP                0x100
#define     PAT_PMT_PERIOD                  0x1f0
#define     PMT_TABLE_LOOKUP                0x200
#define     ADAPTATION_LOOKUP               0x300
#define     ADAPTATION_HDR_LENGTH           0x3f0


/**
    @brief	The CNTV2ConfigTs2022 class is the interface to Kona-IP SMPTE 2022 J2K encoder and TS chips
**/

class AJAExport CNTV2ConfigTs2022 : public CNTV2MBController
{
public:
    CNTV2ConfigTs2022 (CNTV2Card & device);

    // Setup both the TS and J2K encoder using the class params
    bool    SetupForEncode(const NTV2Channel channel, const j2kEncoderConfig &j2kEncodeChannel);

    // Setup the J2K encoder
    bool    SetupJ2KEncoder(const NTV2Channel channel);

    // Setup the J2K decoder
    bool    SetupJ2KDecoder(const  j2kDecoderConfig & config);
    bool    ReadbackJ2KDecoder(j2kDecoderConfig &config);
    bool    GetJ2KDecoderStatus(j2kDecoderStatus &status);

    // Setup the TS encode parts
    bool    SetupTsForEncode(const NTV2Channel channel);

    // If method returns false call this to get details
    std::string getLastError();


 private:
    // Set/Get individual J2K encode high level params (currently stored in virtual registers)

    // Encode Video Format
    void    SetJ2KEncodeVideoFormat(const NTV2Channel channel, const NTV2VideoFormat format) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeVideoFormat1, (uint32_t) format);}
    void    GetJ2KEncodeVideoFormat(const NTV2Channel channel, NTV2VideoFormat & format) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeVideoFormat1, (uint32_t &) format);}

    // Encode ULL mode
    void    SetJ2KEncodeUllMode(const NTV2Channel channel, const uint32_t ull) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeUllMode1, ull);}
    void    GetJ2KEncodeUllMode(const NTV2Channel channel, uint32_t & ull) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeUllMode1, ull);}

    // Encode Bit depth
    void    SetJ2KEncodeBitDepth(const NTV2Channel channel, const uint32_t bitDepth) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeBitDepth1, bitDepth);}
    void    GetJ2KEncodeBitDepth(const NTV2Channel channel, uint32_t & bitDepth) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeBitDepth1, bitDepth);}

    // Encode Chroma sub sampling
    void    SetJ2KEncodeChromaSubsamp(const NTV2Channel channel, const J2KChromaSubSampling subSamp) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeChromaSubSamp1, (uint32_t) subSamp);}
    void    GetJ2KEncodeChromaSubsamp(const NTV2Channel channel, J2KChromaSubSampling & subSamp) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeChromaSubSamp1, (uint32_t &) subSamp);}

    // Encode Code block size
    void    SetJ2KEncodeCodeBlocksize(const NTV2Channel channel, const J2KCodeBlocksize codeBlocksize) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeCodeBlockSize1, (uint32_t) codeBlocksize);}
    void    GetJ2KEncodeCodeBlocksize(const NTV2Channel channel, J2KCodeBlocksize & codeBlocksize) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeCodeBlockSize1, (uint32_t &) codeBlocksize);}

    // Encode rate in MBPS
    void    SetJ2KEncodeMbps(const NTV2Channel channel, const uint32_t mbps) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeMbps1, mbps);}
    void    GetJ2KEncodeMbps(const NTV2Channel channel, uint32_t & mbps) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeMbps1, mbps);}

    // Encode Stream type
    void    SetJ2KEncodeStreamType(const NTV2Channel channel, const J2KStreamType streamType) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeStreamType1, (uint32_t) streamType);}
    void    GetJ2KEncodeStreamType(const NTV2Channel channel, J2KStreamType & streamType) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeStreamType1, (uint32_t &) streamType);}

    // Encode PMT PID
    void    SetJ2KEncodePMTPid(const NTV2Channel channel, const uint32_t pid) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeProgramPid1, pid);}
    void    GetJ2KEncodePMTPid(const NTV2Channel channel, uint32_t & pid) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeProgramPid1, pid);}

    // Encode Video PID
    void    SetJ2KEncodeVideoPid(const NTV2Channel channel, const uint32_t pid) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeVideoPid1, pid);}
    void    GetJ2KEncodeVideoPid(const NTV2Channel channel, uint32_t & pid) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeVideoPid1, pid);}

    // Encode PCR PID
    void    SetJ2KEncodePCRPid(const NTV2Channel channel, const uint32_t pid) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodePcrPid1, pid);}
    void    GetJ2KEncodePCRPid(const NTV2Channel channel, uint32_t & pid) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodePcrPid1, pid);}

    // Encode Audio PID
    void    SetJ2KEncodeAudio1Pid(const NTV2Channel channel, const uint32_t pid) {WriteJ2KConfigVReg(channel, kVRegTxc_2EncodeAudio1Pid1, pid);}
    void    GetJ2KEncodeAudio1Pid(const NTV2Channel channel, uint32_t & pid) {ReadJ2KConfigVReg(channel, kVRegTxc_2EncodeAudio1Pid1, pid);}

    // Setup individual TS encode parts
    bool    SetupEncodeTsTimer(const NTV2Channel channel);
    bool    SetupEncodeTsJ2KEncoder(const NTV2Channel channel);
    bool    SetupEncodeTsMpegJ2kEncap(const NTV2Channel channel);
    bool    SetupEncodeTsMpegPcrEncap(const NTV2Channel channel);
    bool    SetupEncodeTsMpegAesEncap(const NTV2Channel channel);
    bool    SetupEncodeTsAesEncap(const NTV2Channel channel);
    bool    SetupEncodeTsMpegAncEncap(const NTV2Channel channel);

    // Routines to talk to the J2K part
    bool                J2kCanAcceptCmd(const NTV2Channel channel);
    void                J2kSetParam(const NTV2Channel channel, uint32_t config, uint32_t param, uint32_t value);
    void                J2kSetMode(const NTV2Channel channel, uint32_t tier, uint32_t mode);
    uint32_t            GetFeatures();

    void                GenerateTableForMpegJ2kEncap(const NTV2Channel channel);
    void                GenerateTableForMpegPcrEncap(const NTV2Channel channel);
    void                GenerateTableForMpegAesEncap(const NTV2Channel channel);
    uint32_t            GetIpxJ2KAddr(const NTV2Channel channel);
    uint32_t            GetIpxTsAddr(const NTV2Channel channel);

    bool                WriteJ2KConfigVReg(const NTV2Channel channel, const uint32_t vreg, const uint32_t value);
    bool                ReadJ2KConfigVReg(const NTV2Channel channel, const uint32_t vreg,  uint32_t & value);

    bool                _is2022_6;
    bool                _is2022_2;

    int32_t             _transactionTable[1024][2];
    int32_t             _transactionCount;

};	//	CNTV2ConfigTs2022

#endif // NTV2_2022CONFIGTS_H
