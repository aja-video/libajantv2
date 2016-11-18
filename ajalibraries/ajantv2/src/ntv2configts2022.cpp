/**
    @file		ntv2config2022.cpp
    @brief		Implements the CNTV2Config2022 class.
	@copyright	(C) 2014-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2configts2022.h"
#include "ntv2endian.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include <sstream>

#if defined (AJALinux) || defined (AJAMac)
	#include <stdlib.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#else
#pragma warning(disable:4800)
#endif

using namespace std;


CNTV2ConfigTs2022::CNTV2ConfigTs2022(CNTV2Card & device) : CNTV2MBController(device)
{
    uint32_t features    = GetFeatures();

    _is2022_6   = (bool)(features & SAREK_2022_6);
    _is2022_2   = (bool)(features & SAREK_2022_2);

    // init config structs
    for (int channel = NTV2_CHANNEL1; channel < NTV2_MAX_NUM_CHANNELS; channel++)
    {
        // encode defaults
        _j2kEncodeConfig[channel].j2k_videoFormat = NTV2_FORMAT_UNKNOWN;  // an indication channel has not been configured yet
        _j2kEncodeConfig[channel].j2k_ullMode = false;
        _j2kEncodeConfig[channel].j2k_bitDepth = 8;
        _j2kEncodeConfig[channel].j2k_chromaSubsamp = kJ2KChromaSubSamp_422_Standard;
        _j2kEncodeConfig[channel].j2k_codeBlocksize = kJ2KCodeBlocksize_32x32;
        _j2kEncodeConfig[channel].j2k_mbps = 200;
        _j2kEncodeConfig[channel].j2k_streamType = kJ2KStreamTypeStandard;
        _j2kEncodeConfig[channel].j2k_pmtPid = 255;
        _j2kEncodeConfig[channel].j2k_videoPid = 256;
        _j2kEncodeConfig[channel].j2k_pcrPid = 257;
        _j2kEncodeConfig[channel].j2k_audio1Pid = 258;

        // decode defaults
        _j2kDecodeConfig[channel].j2k_pid = 0;
    }
}

bool CNTV2ConfigTs2022::SetupForEncode(const NTV2Channel channel, const j2k_encode_2022_channel &j2kEncodeChannel)
{
    _j2kEncodeConfig[channel].j2k_videoFormat       = j2kEncodeChannel.videoFormat;
    _j2kEncodeConfig[channel].j2k_ullMode           = j2kEncodeChannel.ullMode;
    _j2kEncodeConfig[channel].j2k_bitDepth          = j2kEncodeChannel.bitDepth;
    _j2kEncodeConfig[channel].j2k_chromaSubsamp     = j2kEncodeChannel.chromaSubsamp;
    _j2kEncodeConfig[channel].j2k_codeBlocksize     = j2kEncodeChannel.codeBlocksize;
    _j2kEncodeConfig[channel].j2k_mbps              = j2kEncodeChannel.mbps;
    _j2kEncodeConfig[channel].j2k_streamType        = j2kEncodeChannel.streamType;
    _j2kEncodeConfig[channel].j2k_pmtPid            = j2kEncodeChannel.pmtPid;
    _j2kEncodeConfig[channel].j2k_videoPid          = j2kEncodeChannel.videoPid;
    _j2kEncodeConfig[channel].j2k_pcrPid            = j2kEncodeChannel.pcrPid;
    _j2kEncodeConfig[channel].j2k_audio1Pid         = j2kEncodeChannel.audio1Pid;

    // setup the J2K encoder
    if (!SetupJ2KEncoder(channel))
        return false;

    // setup the TS
    if (!SetupTsForEncode(channel))
        return false;

    return true;
}


bool CNTV2ConfigTs2022::SetupJ2KEncoder(const NTV2Channel channel)
{
    NTV2VideoFormat         videoFormat;
    bool                    ullMode;
    uint32_t                bitDepth;
    J2KChromaSubSampling    subSamp;
    J2KCodeBlocksize        codeBlocksize;
    uint32_t                mbps;

    uint32_t                bitRateMsb;
    uint32_t                bitRateLsb;
    uint32_t                rtConstant;
    uint32_t                ull;

    // Subband removal Component 0, 1 and 2
    uint32_t                sb_rmv_c0 = 0;
    uint32_t                sb_rmv_c1 = 0;
    uint32_t                sb_rmv_c2 = 0;

    uint32_t                regulator_type = 0;
    uint32_t                Rsiz = 258;
    uint32_t                reg_cap = 0;
    uint32_t                clk_fx_freq = 200;
    uint32_t                marker = 0;
    uint32_t                guard_bit = 1;
    uint32_t                prog_order = 0;
    uint32_t                fdwt_type = 1;
    uint32_t                mct = 0;
    uint32_t                num_comp = 3;
    uint32_t                num_levels = 5;

    uint32_t                GOB[8] =        {03, 03, 03, 03, 03, 03, 03, 03};
    uint32_t                QS_C0[8] =      {12, 13, 14, 15, 16, 16, 16, 16};
    uint32_t                QS_C1[8] =      {12, 13, 14, 15, 16, 16, 16, 16};
    uint32_t                QS_C2[8] =      {12, 13, 14, 15, 16, 16, 16, 16};

    // Get our variable user params
    GetJ2KEncodeVideoFormat(channel, videoFormat);
    GetJ2KEncodeUllMode(channel, ullMode);
    GetJ2KEncodeBitDepth(channel, bitDepth);
    GetJ2KEncodeChromaSubsamp(channel, subSamp);
    GetJ2KEncodeCodeBlocksize(channel, codeBlocksize);
    GetJ2KEncodeMbps(channel, mbps);

    // Calculate height and width based on video format
    NTV2Standard standard = GetNTV2StandardFromVideoFormat(videoFormat);
    NTV2FormatDescriptor fd = GetFormatDescriptor(standard,NTV2_FBF_10BIT_YCBCR,false,false,false);
    uint32_t    width = fd.GetRasterWidth();
    uint32_t    height = fd.GetVisibleRasterHeight();

    // Calculate framerate based on video format
    NTV2FrameRate   frameRate = GetNTV2FrameRateFromVideoFormat(videoFormat);
    uint32_t    framesPerSecNum;
    uint32_t    framesPerSecDen;
    uint32_t    fieldsPerSec;

    GetFramesPerSecond (frameRate, framesPerSecNum, framesPerSecDen);
    if (framesPerSecDen == 1001)
        framesPerSecDen = 1000;
    fieldsPerSec = framesPerSecNum/framesPerSecDen;
    if (! NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(videoFormat))
        fieldsPerSec*=2;

    printf("CNTV2ConfigTs2022::SetupJ2KEncoder width=%d, height=%d fpsNum=%d,\n", width, height, fieldsPerSec);

    if (ullMode)
    {
        ull = 1;
        bitRateMsb = ((((mbps*1000000)/fieldsPerSec)/8)/9) >> 16;
        bitRateLsb = ((((mbps*1000000)/fieldsPerSec)/8)/9) & 0xFFFF;
        rtConstant = (((clk_fx_freq*149000)/200)/fieldsPerSec)/9;
    }
    else
    {
        ull = 0;
        bitRateMsb = (((mbps*1000000)/fieldsPerSec)/8) >> 16;
        bitRateLsb = (((mbps*1000000)/fieldsPerSec)/8) & 0xFFFF;
        rtConstant = (((clk_fx_freq*149000)/200)/fieldsPerSec);
    }


    for (uint32_t config=0; config < 3; config++)
    {
        J2kSetParam(channel, config, 0x00, width);
        J2kSetParam(channel, config, 0x01, height);
        J2kSetParam(channel, config, 0x02, bitDepth);
        J2kSetParam(channel, config, 0x03, guard_bit);
        J2kSetParam(channel, config, 0x04, subSamp);
        J2kSetParam(channel, config, 0x05, num_comp);
        J2kSetParam(channel, config, 0x06, mct);
        J2kSetParam(channel, config, 0x07, fdwt_type);
        J2kSetParam(channel, config, 0x08, num_levels);
        J2kSetParam(channel, config, 0x09, codeBlocksize);

        J2kSetParam(channel, config, 0x0A, prog_order);
        J2kSetParam(channel, config, 0x0B, bitRateMsb);
        J2kSetParam(channel, config, 0x0C, bitRateLsb);
        J2kSetParam(channel, config, 0x0D, rtConstant);
        J2kSetParam(channel, config, 0x0E, marker);
        J2kSetParam(channel, config, 0x0F, ull);
        J2kSetParam(channel, config, 0x10, sb_rmv_c0);
        J2kSetParam(channel, config, 0x11, sb_rmv_c1);
        J2kSetParam(channel, config, 0x12, sb_rmv_c2);
        J2kSetParam(channel, config, 0x13, regulator_type);
        J2kSetParam(channel, config, 0x14, Rsiz);
        J2kSetParam(channel, config, 0x15, reg_cap);
    }

    if (ullMode)
    {
        mError = "Setup J2K Failed because ull mode not yet supported";
        return false;
    }

    for (uint32_t config=0; config < 3; config++)
    {
        for (uint32_t lvl=0; lvl < 7; lvl++)
        {
            J2kSetParam(channel, config, 0x80+lvl, GOB[lvl]);
            J2kSetParam(channel, config, 0x88+lvl, QS_C0[lvl]*0x800);
            J2kSetParam(channel, config, 0x90+lvl, QS_C1[lvl]*0x800);
            J2kSetParam(channel, config, 0x98+lvl, QS_C2[lvl]*0x800);
        }
    }

    // Set T2 to record mode
    J2kSetMode(channel, 2, 0x10);

    return true;
}


bool CNTV2ConfigTs2022::SetupJ2KDecoder()
{
    mError = "SetupJ2KDecoder not yet implemented";
    return false;
}


bool CNTV2ConfigTs2022::SetupTsForEncode(const NTV2Channel channel)
{
    // first setup TS for the encoder
    if (!SetupEncodeTsJ2KEncoder(channel))
        return false;

    // program TS timer
    if (!SetupEncodeTsTimer(channel))
        return false;

    // program TS for mpeg j2k encapsulator
    if (!SetupEncodeTsMpegJ2kEncap(channel))
        return false;

    // program TS for aes encapsulator
    if (!SetupEncodeTsAesEncap(channel))
        return false;

    // program TS for mpeg aes encapsulator
    if (!SetupEncodeTsMpegAesEncap(channel))
        return false;

    return true;
}


bool CNTV2ConfigTs2022::SetupTsForDecode()
{
    mError = "SetupTsForDecode not yet implemented";
    return false;
}

// Private functions

// Setup individual TS encode parts
bool CNTV2ConfigTs2022::SetupEncodeTsTimer(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);

    printf("CNTV2ConfigTs2022::SetupEncodeTsTimer\n");

    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_TIMER) + kRegTsTimerJ2kTsLoad, (0x103110));
    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_TIMER) + kRegTsTimerJ2kTsGenTc, (0x3aa));
    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_TIMER) + kRegTsTimerJ2kTsPtsMux, (0x1));

    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsJ2KEncoder(const NTV2Channel channel)
{
    NTV2VideoFormat videoFormat;
    uint32_t addr = GetIpxTsAddr(channel);

    printf("CNTV2ConfigTs2022::SetupEncodeTsJ2KEncoder\n");

    GetJ2KEncodeVideoFormat(channel, videoFormat);
    if (NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(videoFormat))
    {
        // progressive format
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderInterlacedVideo, (0x0));
    }
    else
    {
        // interlaced format
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderInterlacedVideo, (0x1));
    }

    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderHostEn, (0x7));
    mDevice.WriteRegister(addr + (0x800*ENCODE_TS_J2K_ENCODER) + kRegTsJ2kEncoderHostEn, (0x1));

    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsMpegJ2kEncap(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);

    printf("CNTV2ConfigTs2022::SetupEncodeTsMpegJ2kEncap\n");

    if (!GenerateTransactionTableForMpegJ2kEncap(channel))
    {
        mError = "SetupEncodeTsMpegJ2kEncap could not generate transaction table";
        return false;
    }

    // Program Transaction Table
    for (int32_t index=0; index < _transactionCount; index++)
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_MPEG_J2K_ENCAP) + (_transactionTable[index][0]), _transactionTable[index][1]);
        //printf("SetupEncodeTsMpegJ2kEncap - addr=%08x, val=%08x\n",
        //       addr + (0x800*ENCODE_TS_MPEG_J2K_ENCAP) + (_transactionTable[index][0]),  _transactionTable[index][1]);
    }
    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsAesEncap(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);

    // Program registers
    for (uint32_t index=0; index < numTsAesEncapEntries; index++)
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_AES_ENCAP) + (tsAesEncapTable[index].reg), tsAesEncapTable[index].value);
        printf("SetTsAesEncap - reg=%08x, val=%08x\n",
               addr + (0x800*ENCODE_TS_AES_ENCAP) + (tsAesEncapTable[index].reg), tsAesEncapTable[index].value);
    }
    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsMpegAesEncap(const NTV2Channel channel)
{
    uint32_t addr = GetIpxTsAddr(channel);

    // Program registers
    for (uint32_t index=0; index < numTsMpegAesEncapEntries; index++)
    {
        mDevice.WriteRegister(addr + (0x800*ENCODE_TS_MPEG_AES_ENCAP) + (tsMpegAesEncapTable[index].reg), tsMpegAesEncapTable[index].value);
        printf("SetTsMpegAesEncap - reg=%08x, val=%08x\n",
               addr + (0x800*ENCODE_TS_MPEG_AES_ENCAP) + (tsMpegAesEncapTable[index].reg), tsMpegAesEncapTable[index].value);
    }
    return true;
}


bool CNTV2ConfigTs2022::SetupEncodeTsMpegAncEncap()
{
    mError = "SetupEncodeTsMpegAncEncap not yet implemented";
    return false;
}


// Setup individual TS decode parts
bool CNTV2ConfigTs2022::SetupDecodeTsMpegJ2kDecap()
{
    mError = "SetupDecodeTsMpegJ2kDecap not yet implemented";
    return false;
}


bool CNTV2ConfigTs2022::SetupDecodeTsJ2KDecoder()
{
    mError = "SetupDecodeTsJ2KDecoder not yet implemented";
    return false;
}


bool CNTV2ConfigTs2022::SetupDecodeTsMpegAesDecap()
{
    mError = "SetupDecodeTsMpegAesDecap not yet implemented";
    return false;
}


bool CNTV2ConfigTs2022::SetupDecodeTsAesDecap()
{
    mError = "SetupDecodeTsAesDecap not yet implemented";
    return false;
}


bool CNTV2ConfigTs2022::SetupDecodeTsMpegAncDecap()
{
    mError = "SetupDecodeTsMpegAncDecap not yet implemented";
    return false;
}


uint32_t CNTV2ConfigTs2022::GetFeatures()
{
    uint32_t val;
    mDevice.ReadRegister(SAREK_REGS + kRegSarekFwCfg, &val);
    return val;
}


string CNTV2ConfigTs2022::getLastError()
{
    string astring = mError;
    mError.clear();
    return astring;
}


bool CNTV2ConfigTs2022::J2kCanAcceptCmd(const NTV2Channel channel)
{
    uint32_t val;
    uint32_t addr = GetIpxJ2KAddr(channel);

    // Read T0 Main CSR Register
    mDevice.ReadRegister(addr + kRegJ2kT0FIFOCsr, &val);

    // Check CF bit note this is bit reversed from documentation
    // so we check 6 instead of 25
    if(val & BIT(6))
        return false;
    else
        return true;
}


void CNTV2ConfigTs2022::J2kSetMode(const NTV2Channel channel, uint32_t tier, uint32_t mode)
{
    uint32_t addr = GetIpxJ2KAddr(channel);

    mDevice.WriteRegister(addr + (tier*0x40) + kRegJ2kT0MainCsr, mode);
    printf("J2kSetMode - %d wrote 0x%08x to MAIN CSR in tier %d\n", channel, mode, tier);
}


void CNTV2ConfigTs2022::J2kSetParam (const NTV2Channel channel, uint32_t config, uint32_t param, uint32_t value)
{
    uint32_t val;
    uint32_t addr = GetIpxJ2KAddr(channel);

    //printf("J2kSetParam - ch=%d config=0x%08x param=0x%08x value=0x%08x\n", channel, config, param, value);

    while(!J2kCanAcceptCmd(channel))
    {
         printf("J2kSetParam - command fifo full\n");
    }

    val = 0x70000000 + (param<<16) + (config&0x7)*0x2000 + param;
    mDevice.WriteRegister(addr + kRegJ2kT0CmdFIFO, val);

    //printf("J2kSetParam - wrote 0x%08x to CMD FIFO\n", val);

    while(!J2kCanAcceptCmd(channel))
    {
         printf("J2kSetParam - command fifo full\n");
    }

    val = 0x7f000000 + (param<<16) + value;
    mDevice.WriteRegister(addr + kRegJ2kT0CmdFIFO, val);
    //printf("J2kSetParam - wrote 0x%08x to CMD FIFO\n", val);
}


bool CNTV2ConfigTs2022::GenerateTransactionTableForMpegJ2kEncap(const NTV2Channel channel)
{
    NTV2VideoFormat     videoFormat;
    //J2KStreamType       streamType;
    //bool                interlaced;

    int32_t             j2kTsReg;
    int32_t             w1;

    TsEncapStreamData   streamData;

    printf("CNTV2ConfigTs2022::GenerateTransactionTableForMpegJ2kEncap\n");

    // Get our variable user params
    GetJ2KEncodeVideoFormat(channel, videoFormat);
    GetJ2KEncodeStreamType(channel, streamData.j2kStreamType);

    streamData.interlaced = !NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(videoFormat);

    // Calculate height and width based on video format
    NTV2Standard standard = GetNTV2StandardFromVideoFormat(videoFormat);
    NTV2FormatDescriptor fd = GetFormatDescriptor(standard,NTV2_FBF_10BIT_YCBCR,false,false,false);
    streamData.width = fd.GetRasterWidth();
    streamData.height = fd.GetVisibleRasterHeight();

    // Calculate framerate based on video format
    NTV2FrameRate   frameRate = GetNTV2FrameRateFromVideoFormat(videoFormat);
    GetFramesPerSecond (frameRate, streamData.numFrameRate, streamData.denFrameRate);

    // set the PIDs for all streams
    GetJ2KEncodePMTPid(channel, streamData.programPid);
    GetJ2KEncodeVideoPid(channel, streamData.videoPid);
    GetJ2KEncodePCRPid(channel, streamData.pcrPid);
    GetJ2KEncodeAudio1Pid(channel, streamData.audio1Pid);
    streamData.doPCR = false;

    _tsHelper.init(streamData);

    if (_tsHelper.setup_tables(streamData.j2kStreamType, streamData.width, streamData.height, streamData.denFrameRate, streamData.numFrameRate, streamData.interlaced))
    {
        return false;
    }

    _tsHelper.gen_pes_lookup();
    _tsHelper.gen_adaptation_lookup();
    _tsHelper.set_payload_params();
    _tsHelper.set_time_regs();
    j2kTsReg = (_tsHelper.gen_template.hh & 0xff) << 16;
    j2kTsReg |= (_tsHelper.gen_template.mm & 0xff) << 8;
    j2kTsReg |= _tsHelper.gen_template.hh & 0xff;
    _transactionCount = 0;

    printf("Set HOST_EN to 7\n\n");
    _transactionTable[_transactionCount][0] = HOST_EN;
    _transactionTable[_transactionCount++][1] = 7;

    printf("Host Register Settings:\n\n");
    printf("Payload Parameters = 0x%x\n", _tsHelper.payload_params);
    _transactionTable[_transactionCount][0] = PAYLOAD_PARAMS;
    _transactionTable[_transactionCount++][1] = _tsHelper.payload_params;
    printf("Interlaced Video = %i\n", _tsHelper.j2k_vid_descriptors[0].interlaced_video);
    _transactionTable[_transactionCount][0] = INTERLACED_VIDEO;
    _transactionTable[_transactionCount++][1] = _tsHelper.j2k_vid_descriptors[0].interlaced_video;
    printf("TS Packet Generation TC value = %i (0x%x)\n", _tsHelper.ts_gen_tc, _tsHelper.ts_gen_tc);
    _transactionTable[_transactionCount][0] = TS_GEN_TC;
    _transactionTable[_transactionCount++][1] = _tsHelper.ts_gen_tc;
    printf("PAT/PMT Transmission Period = %i (0x%x)\n", _tsHelper.pat_pmt_period, _tsHelper.pat_pmt_period);
    _transactionTable[_transactionCount][0] = PAT_PMT_PERIOD;
    _transactionTable[_transactionCount++][1] = _tsHelper.pat_pmt_period | 0x01000000;
    printf("J2K TimeStamp = 0x%x\n\n", j2kTsReg);
    _transactionTable[_transactionCount][0] = J2K_TS_LOAD;
    _transactionTable[_transactionCount++][1] = j2kTsReg;

    // This is for Evertz compatibility for now we just write the PES_HDR id and set the length to 4
    // otherwise we use the generated table
    int32_t pesHDRLength = 4;
    if (streamData.j2kStreamType == kJ2KStreamTypeStandard)
    {
        pesHDRLength = _tsHelper.pes_template_len;
    }
    printf("PES Template Length = %i, Data:\n", pesHDRLength);
    _transactionTable[_transactionCount][0] = PES_HDR_LEN;
    _transactionTable[_transactionCount++][1] = pesHDRLength;
    for (w1 = 0; w1 < pesHDRLength; w1++)
    {
        printf("0x%02x, ", _tsHelper.pes_template.payload[w1]);
        if (!((w1 + 1) % 16))
            printf("\n");
        _transactionTable[_transactionCount][0] = PES_HDR_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = _tsHelper.pes_template.payload[w1];
    }
    printf("\n\n");

    if (streamData.j2kStreamType == kJ2KStreamTypeEvertz)
    {
        printf("PTS Offset = 0x%02x, J2K TS Offset = 0x%02x, auf1 offset = 0x%02x, auf2 offset = 0x%02x\n\n",
               0xff, 0xff, 0xff, 0xff);
        _transactionTable[_transactionCount][0] = PTS_OFFSET;
        _transactionTable[_transactionCount++][1] = 0xff;
        _transactionTable[_transactionCount][0] = J2K_TS_OFFSET;
        _transactionTable[_transactionCount++][1] = 0xff;
        _transactionTable[_transactionCount][0] = AUF1_OFFSET;
        _transactionTable[_transactionCount++][1] = 0xff;
        _transactionTable[_transactionCount][0] = AUF2_OFFSET;
        _transactionTable[_transactionCount++][1] = 0xff;
    }
    else
    {
        printf("PTS Offset = 0x%02x, J2K TS Offset = 0x%02x, auf1 offset = 0x%02x, auf2 offset = 0x%02x\n\n",
               _tsHelper.pts_offset, _tsHelper.j2k_ts_offset, _tsHelper.auf1_offset, _tsHelper.auf2_offset);
        _transactionTable[_transactionCount][0] = PTS_OFFSET;
        _transactionTable[_transactionCount++][1] = _tsHelper.pts_offset;
        _transactionTable[_transactionCount][0] = J2K_TS_OFFSET;
        _transactionTable[_transactionCount++][1] = _tsHelper.j2k_ts_offset;
        _transactionTable[_transactionCount][0] = AUF1_OFFSET;
        _transactionTable[_transactionCount++][1] = _tsHelper.auf1_offset;
        _transactionTable[_transactionCount][0] = AUF2_OFFSET;
        _transactionTable[_transactionCount++][1] = _tsHelper.auf2_offset;
    }

    printf("Adaptation Template Length = %i, Data:\n", _tsHelper.adaptation_template_length + 6);
    _transactionTable[_transactionCount][0] = ADAPTATION_HDR_LENGTH;
    _transactionTable[_transactionCount++][1] = _tsHelper.adaptation_template_length + 6;
    for (w1 = 0; w1 < _tsHelper.adaptation_template_length + 6; w1++)
    {
        if (w1 < _tsHelper.adaptation_template_length)
        {
            printf("0x%02x, ", _tsHelper.adaptation_template.int_payload[w1]);
            _transactionTable[_transactionCount][0] = ADAPTATION_LOOKUP + w1;
            _transactionTable[_transactionCount++][1] = _tsHelper.adaptation_template.int_payload[w1];
        }
        else
        {
            printf("0x%02x, ", (w1 - _tsHelper.adaptation_template_length) | 0x8000);

            _transactionTable[_transactionCount][0] = ADAPTATION_LOOKUP + w1;
            _transactionTable[_transactionCount++][1] = ((w1 - _tsHelper.adaptation_template_length) << 8) | 0x800;

        }
        if (!((w1 + 1) % 16))
            printf("\n");
    }    
    printf("\n\n");
    printf("PAT Table:\n");
    for (w1 = 0; w1 < 188; w1++)
    {
        printf("0x%02x, ", _tsHelper.pat_table[0].payload[w1]);
        if (!((w1 + 1) % 16))
            printf("\n");
        _transactionTable[_transactionCount][0] = PAT_TABLE_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = _tsHelper.pat_table[0].payload[w1];
    }
    printf("\n\nPMT Table:\n");
    for (w1 = 0; w1 < 188; w1++)
    {
        printf("0x%02x, ", _tsHelper.pmt_tables[_tsHelper.pmt_program_number].payload[w1]);
        if (!((w1 + 1) % 16))
            printf("\n");
        _transactionTable[_transactionCount][0] = PMT_TABLE_LOOKUP + w1;
        _transactionTable[_transactionCount++][1] = _tsHelper.pmt_tables[_tsHelper.pmt_program_number].payload[w1];
    }
    printf("\n\n");

    // Last entry is HOST_EN to on
    printf("Set HOST_EN to 1\n\n");
    _transactionTable[_transactionCount][0] = HOST_EN;
    _transactionTable[_transactionCount++][1] = 1;


    return true;

}

uint32_t CNTV2ConfigTs2022::GetIpxJ2KAddr(const NTV2Channel channel)
{
    uint32_t addr = SAREK_J2K_ENCODER_1;
    
    if (channel == NTV2_CHANNEL2)
    {
        addr = SAREK_J2K_ENCODER_2;
    }
    return addr;
}


uint32_t CNTV2ConfigTs2022::GetIpxTsAddr(const NTV2Channel channel)
{
    uint32_t addr = SAREK_TS_ENCODER_1;
    
    if (channel == NTV2_CHANNEL2)
    {
        addr = SAREK_TS_ENCODER_2;
    }
    return addr;
}



