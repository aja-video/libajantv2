/**
	@file		ntv2tshelper.h
	@brief		Implementation of the CNTV2TsHelper class that is fundamental in generating register settings for IPX MPEG2-TS encapsulator.
                This is a class for generating Transport Stream encode / decode programming data, and decoding ingress PAT/PMT and
                PES packets to setup decap for different streams.
 
                This is modified version of the original code from Macnica.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ntv2tshelper.h"

// Constructor
CNTV2TsHelper::CNTV2TsHelper()
{
    double      frames;
    double      d1;
    int32_t     w1;
        
    start_time = 308636;
    fps = 60.0 - 60.0*15.0/1e6;
    frames = fps * (14 * 3600.0 + 3 * fps + 48.0);
    total_time = frames * 1.0 / fps;
    d1 = total_time / 60.0;
    w1 = (int32_t) d1;
    d1 = d1 - (double) w1;


    /*  This came from build_j2k_descriptors we will probably need it for setting timing reg
    int32_t w1;
    double d1;
    double frames;

    // Handle timing
    d1 = (double) j2k_vid_descriptors[desc_num].num_frame_rate / (double) j2k_vid_descriptors[desc_num].den_frame_rate;
    fps = d1 - d1 * 15.0/1e6;
    frames = fps * (14 * 3600.0 + 3 * fps + 48.0);
    total_time = frames * 1.0 / fps;
    d1 = total_time / 60.0;
    w1 = (int32_t) d1;
    d1 = d1 - (double) w1;
    */
            
    // 90ms - to make sure it is at least 100ms
    table_tx_period = 90e-3;
    
    pes_template_len = -1;
    pts_offset = -1;
    j2k_ts_offset = -1;
    auf1_offset = -1;
    auf2_offset = -1;
    adaptation_template_length = -1;
    payload_params = 0;
    ts_gen_tc = 0;
    pmt_program_number = 0;
}


// Destructor
CNTV2TsHelper::~CNTV2TsHelper()
{
}

void CNTV2TsHelper::init(TsEncapStreamData streamData)
{
    tsStreamData = streamData;

}

// Generates a lookup table for PES packets
int32_t CNTV2TsHelper::gen_pes_lookup(void)
{
    uint8_t esh[48];
    int32_t w1, w5;
    int32_t bpnt;
    
    // Copy in payload
    for (w1 = 0; w1 < 170; w1++)
        pes_template.payload[w1] = 0;

    pes_template.payload[0] = 0x47;
    pes_template.payload[1] = (uint8_t) (0 << 7);                   // transport_error
    pes_template.payload[1] |= (uint8_t) (1 << 6);                  // payload_unit_start
    pes_template.payload[1] |= (uint8_t) (0 << 5);                  // transport_priority
    pes_template.payload[1] |= (uint8_t) ((tsStreamData.videoPid >> 8) & 0x1f);
    pes_template.payload[2] = (uint8_t) (tsStreamData.videoPid & 0xff);
    pes_template.payload[3] |= (uint8_t) (1 << 4);
    bpnt = 4;

    // PES Header
    pes_template.payload[4] = (uint8_t) ((1 >> 16) & 0xff);        // packet_start_code_prefix
    pes_template.payload[5] = (uint8_t) ((1 >> 8) & 0xff);
    pes_template.payload[6] = (uint8_t) (1 & 0xff);
    pes_template.payload[7] = (uint8_t) (0xbd);
    pes_template.payload[8] = 0;
    pes_template.payload[9] = 0;
    pes_template.payload[10] = 0x80;
    pes_template.payload[10] |= (uint8_t) (1 << 2);
    pes_template.payload[11] = 0x80;
    pes_template.payload[12] = 5;
    pts_offset = 13;
    pes_template.payload[13] = 0x21;
    pes_template.payload[14] = 0x0;
    pes_template.payload[15] = 0x1;
    pes_template.payload[16] = 0x0;
    pes_template.payload[17] = 0x1;
    uint64_t pts = 0;
    pes_template.payload[13] |= (uint8_t) ((pts >> 29) & 0xe);
    pes_template.payload[14] |= (uint8_t) ((pts >> 22) & 0xff);
    pes_template.payload[15] |= (uint8_t) ((pts >> 14) & 0xfe);
    pes_template.payload[16] |= (uint8_t) ((pts >> 7) & 0xff);
    pes_template.payload[17] |= (uint8_t) ((pts << 1) & 0xfe);
    bpnt = 18;

    esh[0] = 0x65;
    esh[1] = 0x6c;
    esh[2] = 0x73;
    esh[3] = 0x6d;
    esh[4] = 0x66;
    esh[5] = 0x72;
    esh[6] = 0x61;
    esh[7] = 0x74;
    esh[8] = (uint8_t) ((tsStreamData.denFrameRate >> 8) & 0xff);
    esh[9] = (uint8_t) (tsStreamData.denFrameRate & 0xff);
    esh[10] = (uint8_t) ((tsStreamData.numFrameRate >> 8) & 0xff);
    esh[11] = (uint8_t) (tsStreamData.numFrameRate & 0xff);
    esh[12] = 0x62;
    esh[13] = 0x72;
    esh[14] = 0x61;
    esh[15] = 0x74;
    uint32_t bitRate = 75000000;
    esh[16] = (uint8_t) (bitRate >> 24);
    esh[17] = (uint8_t) ((bitRate >> 16) & 0xff);
    esh[18] = (uint8_t) ((bitRate >> 8) & 0xff);
    esh[19] = (uint8_t) (bitRate & 0xff);
    auf1_offset = 20 + bpnt;
    auf2_offset = 0xff;
    uint32_t offset = 0;
    esh[20] = (uint8_t) (offset >> 24);
    esh[21] = (uint8_t) ((offset >> 16) & 0xff);
    esh[22] = (uint8_t) ((offset >> 8) & 0xff);
    esh[23] = (uint8_t) (offset & 0xff);
    if (tsStreamData.interlaced)
    {
        auf2_offset = 24 + bpnt;
        uint32_t offset2 = 0;
        esh[24] = (uint8_t) (offset2 >> 24);
        esh[25] = (uint8_t) ((offset2 >> 16) & 0xff);
        esh[26] = (uint8_t) ((offset2 >> 8) & 0xff);
        esh[27] = (uint8_t) (offset2 & 0xff);
        esh[28] = 0x66;
        esh[29] = 0x69;
        esh[30] = 0x65;
        esh[31] = 0x6c;
        esh[32] = (uint8_t) (2 & 0xff);
        esh[33] = (uint8_t) (1 & 0xff);
        esh[34] = 0x74;
        esh[35] = 0x63;
        esh[36] = 0x6f;
        esh[37] = 0x64;
        j2k_ts_offset = 38 + bpnt;
        esh[38] = (uint8_t) (0 & 0xff);                 // hh
        esh[39] = (uint8_t) (0 & 0xff);                 // mm
        esh[40] = (uint8_t) (0 & 0xff);                 // ss
        esh[41] = (uint8_t) (0 & 0xff);                 // ff
        esh[42] = 0x62;
        esh[43] = 0x63;
        esh[44] = 0x6f;	// NOTE: Type in Rec. ITU-T H.222.0 standard shows this as 0x68
        esh[45] = 0x6c;
        esh[46] = 3;
        esh[47] = 0x0;
    }
    else
    {
        esh[24] = 0x74;
        esh[25] = 0x63;
        esh[26] = 0x6f;
        esh[27] = 0x64;
        j2k_ts_offset = 28 + bpnt;
        esh[28] = (uint8_t) (0 & 0xff);                 // hh
        esh[29] = (uint8_t) (0 & 0xff);                 // mm
        esh[30] = (uint8_t) (0 & 0xff);                 // ss
        esh[31] = (uint8_t) (0 & 0xff);                 // ff
        esh[32] = 0x62;
        esh[33] = 0x63;
        esh[34] = 0x6f;
        esh[35] = 0x6c;
        esh[36] = 3;
        esh[37] = 0xff;
    }
    if (tsStreamData.interlaced)
    {
        for (w5 = 0; w5 < 48; w5++)
            pes_template.payload[bpnt++] = esh[w5];
    }
    else
    {
        for (w5 = 0; w5 < 38; w5++)
            pes_template.payload[bpnt++] = esh[w5];
    }

    pes_template_len = bpnt;

    return 0;
}

// Sets payload parameter register based on RTL requirements
int32_t CNTV2TsHelper::set_time_regs(void)
{
    double d1, d2;
    
    // First packet rate
    d1 = 80000000 / 8.0 / 188.0;        // Packet Rate
    d1 = 1.0 / d1;                      // Packet Period
    d2 = 1.0 / 125000000;               // Clock Period
    d1 = d1 / d2 - 1.0;                 // One less as it counts from 0
    
    ts_gen_tc = (int32_t) d1;
    
    // Next PAT / PMT Transmission Rate
    d1 = table_tx_period / d2 - 1.0;
    pat_pmt_period = (int32_t) d1;
    
    return 0;
}
