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
    
    generate_fbw = 1;
    fbw_bandwidth = 80e6;
    
    start_time = 308636;
    fps = 60.0 - 60.0*15.0/1e6;
    frames = fps * (14 * 3600.0 + 3 * fps + 48.0);
    total_time = frames * 1.0 / fps;
    d1 = total_time / 60.0;
    w1 = (int32_t) d1;
    d1 = d1 - (double) w1;

    gen_template.transport_error = 0;
    gen_template.payload_unit_start = 0;
    gen_template.transport_priority = 0;
    gen_template.pid = 57;
    gen_template.transport_scrambling_control = 0;
    gen_template.adaptation_field_control = 1;
    gen_template.continuity_counter = 3;
    gen_template.packet_start_code_prefix = 1;
    gen_template.adaptation_field_length = 0;
    gen_template.discontinuity_indicator = 0;
    gen_template.random_access_indicator = 0;
    gen_template.elementery_stream_priority_indicator = 0;
    gen_template.flags_5 = 0x10;
    gen_template.stream_id = 0xbd;
    gen_template.pes_packet_length = 0;
    gen_template.pes_scrambling_control = 0;
    gen_template.pes_priority = 0;
    gen_template.data_alignment_indicator = 1;
    gen_template.copyright = 0;
    gen_template.original_copy = 0;
    gen_template.flags_7 = 0x80;
    gen_template.pes_data_header_length = 5;
    gen_template.pts = start_time;
    gen_template.interlaced_video = 0;
    gen_template.j2k_esh = 0;
    gen_template.frat_denominator = 1000;
    gen_template.frat_numerator = 60000;
    gen_template.interlaced_video = 0;
    gen_template.max_br = 75000000;
    gen_template.auf1 = 0;
    gen_template.auf2 = 0;
    gen_template.fic = 2;
    gen_template.fio = 1;
    gen_template.hh = 16;
    gen_template.mm = 49;
    gen_template.ss = 27;
    gen_template.ff = 23;
    gen_template.bcol_colcr = 0;
    frm_cnt = 0;
    
    adaptation_template.do_pcr = 0;
    
    for (w1 = 0; w1 < 256; w1++)
    {
        pat_table[w1].used = 0;
        pat_table[w1].errs_done = 0;
        pat_table[w1].different_pat_cnt = 0;
    }
    
    for (w1 = 0; w1 < MAX_PROGS; w1++)
    {
        pmt_tables[w1].used = 0;
        pmt_tables[w1].errs_done = 0;
        pmt_tables[w1].different_pmt_cnt = 0;
    }
    
    for (w1 = 0; w1 < PID_LIST_SIZE; w1++)
    {
        prog_pid_list[w1].used = 0;
        prog_pid_list[w1].pcr_pid = 0;
        prog_pid_list[w1].j2k_stream = 0;
        prog_pid_list[w1].non_j2k_stream = 0;
        prog_pid_list[w1].pat_table = 0;
        prog_pid_list[w1].pmt_table = 0;
        prog_pid_list[w1].cont_cnt = -1;
        prog_pid_list[w1].found = 0;
        prog_pid_list[w1].pcr_pid_check = 0;
        prog_pid_list[w1].j2k_stream_check = 0;
        prog_pid_list[w1].stream_check = 0;
    }
    
    pid_list_cnt = 0;
    j2k_channel_cnt = 0;
    for (w1 = 0; w1 < NUM_J2K_CHANNELS; w1++)
        j2k_vid_descriptors[w1].used = 0;
    
    // 90ms - to make sure it is at least 100ms
    table_tx_period = 90e-3;
    
    fbw_total_frames = 0;
    fbw_total_packets = 0;
    fbw_frame_rate = -1;
    fbw_pkts_per_frame = -1;
    pes_template_len = -1;
    pts_offset = -1;
    j2k_ts_offset = -1;
    auf1_offset = -1;
    auf2_offset = -1;
    adaptation_template_length = -1;
    payload_params = 0;
    ts_bitrate = 80e6;
    sys_clk = 125e6;
    ts_gen_tc = 0;
    pat_pmt_period = 0;	// don't send
    pmt_program_number = 0;
}


// Destructor
CNTV2TsHelper::~CNTV2TsHelper()
{
}


int32_t CNTV2TsHelper::setup_tables(J2KStreamType streamType, uint32_t width, uint32_t height, int32_t denFrameRate, int32_t numFrameRate, bool interlaced)
{
    int32_t err;
    
    // Set up default PAT Table
    pat_table[0].payload_unit_start     = 1;
    pat_table[0].transport_stream_id    = 1;
    pat_table[0].version_number         = 0;
    pat_table[0].current_next           = 1;
    pat_table[0].section_num            = 0;
    pat_table[0].last_section_num       = 0;
    pat_table[0].number_of_progs        = 1;
    pat_table[0].program_num[0]         = 1;
    pat_table[0].program_pid[0]         = 0x100;
    pat_table[0].used                   = 1;
    
    // There is a single program.  Set up the PMT for this program
    pmt_tables[1].payload_unit_start                    = 1;
    pmt_tables[1].program_num                           = 1;
    pmt_tables[1].version_number                        = 0;
    pmt_tables[1].current_next                          = 1;
    pmt_tables[1].pcr_pid                               = 0x101;	// J2K Stream PID
    pmt_tables[1].prog_info_length                      = 0;
    pmt_tables[1].num_streams                           = 1;
    pmt_tables[1].stream_descriptors[0].j2k_stream      = 1;
    pmt_tables[1].stream_descriptors[0].j2k_channel_num = 0;
    pmt_tables[1].stream_descriptors[0].elementary_pid  = 0x101;
    pmt_tables[1].used = 1;
    num_progs = 1;
    
    // Set up J2K Descriptor
    if (streamType == kJ2KStreamTypeEvertz)
    {
        j2k_vid_descriptors[0].profile_level    = 0x101;
        j2k_vid_descriptors[0].max_bit_rate     = 213000000;
    }
    else
    {
        j2k_vid_descriptors[0].profile_level    = 0x102;
        j2k_vid_descriptors[0].max_bit_rate     = 160000000;
    }

    j2k_vid_descriptors[0].horizontal_size  = width;
    if (interlaced)
    {
        j2k_vid_descriptors[0].vertical_size    = height/2;
    }
    else
    {
        j2k_vid_descriptors[0].vertical_size    = height;
    }

    j2k_vid_descriptors[0].max_buffer_size  = 1250000;
    j2k_vid_descriptors[0].den_frame_rate   = denFrameRate;
    j2k_vid_descriptors[0].num_frame_rate   = numFrameRate;
    j2k_vid_descriptors[0].color_spec       = 3;
    j2k_vid_descriptors[0].still_mode       = 0;
    j2k_vid_descriptors[0].interlaced_video = interlaced;
    j2k_vid_descriptors[0].stream_type      = streamType;
    j2k_vid_descriptors[0].private_data_len = 0;
    j2k_vid_descriptors[0].used             = 1;
    j2k_channel_cnt = 1;
    
    // Build Tables
    err = build_all_tables();
    
    return (err);
}


// Generates a lookup table for PES packets
int32_t CNTV2TsHelper::gen_pes_lookup(void)
{
    uint8_t esh[48];
    int32_t w1, w5;
    int32_t bpnt;
    
    pes_template.transport_error = gen_template.transport_error;
    pes_template.payload_unit_start = gen_template.payload_unit_start;
    pes_template.transport_priority = gen_template.transport_priority;
    pes_template.pid = gen_template.pid;
    pes_template.transport_scrambling_control = gen_template.transport_scrambling_control;
    pes_template.adaptation_field_control = gen_template.adaptation_field_control;
    pes_template.continuity_counter = gen_template.continuity_counter;
    pes_template.packet_start_code_prefix = gen_template.packet_start_code_prefix;
    pes_template.adaptation_field_length = gen_template.adaptation_field_length;
    pes_template.discontinuity_indicator = gen_template.discontinuity_indicator;
    pes_template.random_access_indicator = gen_template.random_access_indicator;
    pes_template.elementery_stream_priority_indicator = gen_template.elementery_stream_priority_indicator;
    pes_template.flags_5 = gen_template.flags_5;
    pes_template.stream_id = gen_template.stream_id;
    pes_template.pes_packet_length = gen_template.pes_packet_length;
    pes_template.pes_scrambling_control = gen_template.pes_scrambling_control;
    pes_template.pes_priority = gen_template.pes_priority;
    pes_template.data_alignment_indicator = gen_template.data_alignment_indicator;
    pes_template.copyright = gen_template.copyright;
    pes_template.original_copy = gen_template.original_copy;
    pes_template.flags_7 = gen_template.flags_7;
    pes_template.pes_data_header_length = gen_template.pes_data_header_length;
    pes_template.pts = gen_template.pts;
    pes_template.interlaced_video = gen_template.interlaced_video;
    pes_template.j2k_esh = gen_template.j2k_esh;
    pes_template.frat_denominator = gen_template.frat_denominator;
    pes_template.frat_numerator = gen_template.frat_numerator;
    pes_template.interlaced_video = gen_template.interlaced_video;
    pes_template.max_br = gen_template.max_br;
    pes_template.auf1 = gen_template.auf1;
    pes_template.auf2 = gen_template.auf2;
    pes_template.fic = gen_template.fic;
    pes_template.fio = gen_template.fio;
    pes_template.hh = gen_template.hh;
    pes_template.mm = gen_template.mm;
    pes_template.ss = gen_template.ss;
    pes_template.ff = gen_template.ff;
    pes_template.bcol_colcr = gen_template.bcol_colcr;
    
    // Setup template fields based on tables
    pes_template.pid = j2k_vid_descriptors[0].associated_pid;
    pes_template.frat_denominator = j2k_vid_descriptors[0].den_frame_rate;
    pes_template.frat_numerator = j2k_vid_descriptors[0].num_frame_rate;
    pes_template.interlaced_video = j2k_vid_descriptors[0].interlaced_video;
    pes_template.bcol_colcr = j2k_vid_descriptors[0].color_spec;
    
    pes_template.auf1 = 0;
    pes_template.auf2 = 0;
    pes_template.hh = 0;
    pes_template.mm = 0;
    pes_template.ss = 0;
    pes_template.ff = 0;
    
    pes_template.payload_unit_start = 1;
    
    // Copy in payload
    for (w1 = 0; w1 < 170; w1++)
        pes_template.payload[w1] = 0;
    pes_template.pts = 0;
    pes_template.continuity_counter = 0;
    
    ts_packet_bin[0] = 0x47;
    ts_packet_bin[1] = (uint8_t) (pes_template.transport_error << 7);
    ts_packet_bin[1] |= (uint8_t) (pes_template.payload_unit_start << 6);
    ts_packet_bin[1] |= (uint8_t) (pes_template.transport_priority << 5);
    ts_packet_bin[1] |= (uint8_t) ((pes_template.pid >> 8) & 0x1f);
    ts_packet_bin[2] = (uint8_t) (pes_template.pid & 0xff);
    ts_packet_bin[3] = (uint8_t) (pes_template.transport_scrambling_control << 6);
    ts_packet_bin[3] |= (uint8_t) (pes_template.adaptation_field_control << 4);
    ts_packet_bin[3] |= (uint8_t) (pes_template.continuity_counter & 0xf);
    bpnt = 4;
    
    // PES Header
    if (pes_template.payload_unit_start)
    {
        ts_packet_bin[4] = (uint8_t) ((pes_template.packet_start_code_prefix >> 16) & 0xff);
        ts_packet_bin[5] = (uint8_t) ((pes_template.packet_start_code_prefix >> 8) & 0xff);
        ts_packet_bin[6] = (uint8_t) (pes_template.packet_start_code_prefix & 0xff);
        ts_packet_bin[7] = (uint8_t) (pes_template.stream_id);
        ts_packet_bin[8] = (uint8_t) ((pes_template.pes_packet_length >> 8) & 0xff);
        ts_packet_bin[9] = (uint8_t) (pes_template.pes_packet_length & 0xff);
        ts_packet_bin[10] = 0x80;
        ts_packet_bin[10] |= (uint8_t) (pes_template.pes_scrambling_control << 4);
        ts_packet_bin[10] |= (uint8_t) (pes_template.pes_priority << 3);
        ts_packet_bin[10] |= (uint8_t) (pes_template.data_alignment_indicator << 2);
        ts_packet_bin[10] |= (uint8_t) (pes_template.copyright << 1);
        ts_packet_bin[10] |= (uint8_t) (pes_template.original_copy);
        ts_packet_bin[11] = (uint8_t) (pes_template.flags_7);
        ts_packet_bin[12] = (uint8_t) (pes_template.pes_data_header_length);
        pts_offset = 13;
        ts_packet_bin[13] = 0x21;
        ts_packet_bin[14] = 0x0;
        ts_packet_bin[15] = 0x1;
        ts_packet_bin[16] = 0x0;
        ts_packet_bin[17] = 0x1;
        ts_packet_bin[13] |= (uint8_t) ((pes_template.pts >> 29) & 0xe);
        ts_packet_bin[14] |= (uint8_t) ((pes_template.pts >> 22) & 0xff);
        ts_packet_bin[15] |= (uint8_t) ((pes_template.pts >> 14) & 0xfe);
        ts_packet_bin[16] |= (uint8_t) ((pes_template.pts >> 7) & 0xff);
        ts_packet_bin[17] |= (uint8_t) ((pes_template.pts << 1) & 0xfe);
        bpnt = 18;
        
        esh[0] = 0x65;
        esh[1] = 0x6c;
        esh[2] = 0x73;
        esh[3] = 0x6d;
        esh[4] = 0x66;
        esh[5] = 0x72;
        esh[6] = 0x61;
        esh[7] = 0x74;
        esh[8] = (uint8_t) ((pes_template.frat_denominator >> 8) & 0xff);
        esh[9] = (uint8_t) (pes_template.frat_denominator & 0xff);
        esh[10] = (uint8_t) ((pes_template.frat_numerator >> 8) & 0xff);
        esh[11] = (uint8_t) (pes_template.frat_numerator & 0xff);
        esh[12] = 0x62;
        esh[13] = 0x72;
        esh[14] = 0x61;
        esh[15] = 0x74;
        esh[16] = (uint8_t) (pes_template.max_br >> 24);
        esh[17] = (uint8_t) ((pes_template.max_br >> 16) & 0xff);
        esh[18] = (uint8_t) ((pes_template.max_br >> 8) & 0xff);
        esh[19] = (uint8_t) (pes_template.max_br & 0xff);
        auf1_offset = 20 + bpnt;
        auf2_offset = 0xff;
        esh[20] = (uint8_t) (pes_template.auf1 >> 24);
        esh[21] = (uint8_t) ((pes_template.auf1 >> 16) & 0xff);
        esh[22] = (uint8_t) ((pes_template.auf1 >> 8) & 0xff);
        esh[23] = (uint8_t) (pes_template.auf1 & 0xff);
        if (pes_template.interlaced_video)
        {
            auf2_offset = 24 + bpnt;
            esh[24] = (uint8_t) (pes_template.auf2 >> 24);
            esh[25] = (uint8_t) ((pes_template.auf2 >> 16) & 0xff);
            esh[26] = (uint8_t) ((pes_template.auf2 >> 8) & 0xff);
            esh[27] = (uint8_t) (pes_template.auf2 & 0xff);
            esh[28] = 0x66;
            esh[29] = 0x69;
            esh[30] = 0x65;
            esh[31] = 0x6c;
            esh[32] = (uint8_t) (pes_template.fic & 0xff);
            esh[33] = (uint8_t) (pes_template.fio & 0xff);
            esh[34] = 0x74;
            esh[35] = 0x63;
            esh[36] = 0x6f;
            esh[37] = 0x64;
            j2k_ts_offset = 38 + bpnt;
            esh[38] = (uint8_t) (pes_template.hh & 0xff);
            esh[39] = (uint8_t) (pes_template.mm & 0xff);
            esh[40] = (uint8_t) (pes_template.ss & 0xff);
            esh[41] = (uint8_t) (pes_template.ff & 0xff);
            esh[42] = 0x62;
            esh[43] = 0x63;
            esh[44] = 0x6f;	// NOTE: Type in Rec. ITU-T H.222.0 standard shows this as 0x68
            esh[45] = 0x6c;
            esh[46] = (uint8_t) (pes_template.bcol_colcr & 0xff);
            esh[47] = 0x0;
        }
        else
        {
            esh[24] = 0x74;
            esh[25] = 0x63;
            esh[26] = 0x6f;
            esh[27] = 0x64;
            j2k_ts_offset = 28 + bpnt;
            esh[28] = (uint8_t) (pes_template.hh & 0xff);
            esh[29] = (uint8_t) (pes_template.mm & 0xff);
            esh[30] = (uint8_t) (pes_template.ss & 0xff);
            esh[31] = (uint8_t) (pes_template.ff & 0xff);
            esh[32] = 0x62;
            esh[33] = 0x63;
            esh[34] = 0x6f;
            esh[35] = 0x6c;
            esh[36] = (uint8_t) (pes_template.bcol_colcr & 0xff);
            esh[37] = 0xff;
        }
        if (pes_template.interlaced_video)
        {
            for (w5 = 0; w5 < 48; w5++)
                ts_packet_bin[bpnt++] = esh[w5];
        }
        else
        {
            for (w5 = 0; w5 < 38; w5++)
                ts_packet_bin[bpnt++] = esh[w5];
        }
    }
    
    // Move data pack to payload field
    for (w1 = 0; w1 < 188; w1++)
        pes_template.payload[w1] = ts_packet_bin[w1];
    
    pes_template_len = bpnt;
    
    return 0;
}


// Generates the adaptation packet lookup table
int32_t CNTV2TsHelper::gen_adaptation_lookup(void)
{
    int32_t w1;
    int32_t bpnt;
    
    adaptation_template.transport_error = gen_template.transport_error;
    adaptation_template.payload_unit_start = gen_template.payload_unit_start;
    adaptation_template.transport_priority = gen_template.transport_priority;
    adaptation_template.pid = gen_template.pid;
    adaptation_template.transport_scrambling_control = gen_template.transport_scrambling_control;
    adaptation_template.adaptation_field_control = gen_template.adaptation_field_control;
    adaptation_template.continuity_counter = gen_template.continuity_counter;
    adaptation_template.packet_start_code_prefix = gen_template.packet_start_code_prefix;
    adaptation_template.adaptation_field_length = gen_template.adaptation_field_length;
    adaptation_template.discontinuity_indicator = gen_template.discontinuity_indicator;
    adaptation_template.random_access_indicator = gen_template.random_access_indicator;
    adaptation_template.elementery_stream_priority_indicator = gen_template.elementery_stream_priority_indicator;
    adaptation_template.flags_5 = gen_template.flags_5;
    adaptation_template.stream_id = gen_template.stream_id;
    adaptation_template.pes_packet_length = gen_template.pes_packet_length;
    adaptation_template.pes_scrambling_control = gen_template.pes_scrambling_control;
    adaptation_template.pes_priority = gen_template.pes_priority;
    adaptation_template.data_alignment_indicator = gen_template.data_alignment_indicator;
    adaptation_template.copyright = gen_template.copyright;
    adaptation_template.original_copy = gen_template.original_copy;
    adaptation_template.flags_7 = gen_template.flags_7;
    adaptation_template.pes_data_header_length = gen_template.pes_data_header_length;
    adaptation_template.pts = gen_template.pts;
    adaptation_template.interlaced_video = gen_template.interlaced_video;
    adaptation_template.j2k_esh = gen_template.j2k_esh;
    adaptation_template.frat_denominator = gen_template.frat_denominator;
    adaptation_template.frat_numerator = gen_template.frat_numerator;
    adaptation_template.interlaced_video = gen_template.interlaced_video;
    adaptation_template.max_br = gen_template.max_br;
    adaptation_template.auf1 = gen_template.auf1;
    adaptation_template.auf2 = gen_template.auf2;
    adaptation_template.fic = gen_template.fic;
    adaptation_template.fio = gen_template.fio;
    adaptation_template.hh = gen_template.hh;
    adaptation_template.mm = gen_template.mm;
    adaptation_template.ss = gen_template.ss;
    adaptation_template.ff = gen_template.ff;
    adaptation_template.bcol_colcr = gen_template.bcol_colcr;
    
    // Setup template fields based on tables
    adaptation_template.pid = j2k_vid_descriptors[0].associated_pid;
    adaptation_template.frat_denominator = j2k_vid_descriptors[0].den_frame_rate;
    adaptation_template.frat_numerator = j2k_vid_descriptors[0].num_frame_rate;
    adaptation_template.interlaced_video = j2k_vid_descriptors[0].interlaced_video;
    adaptation_template.bcol_colcr = j2k_vid_descriptors[0].color_spec;
    
    adaptation_template.auf1 = 0;
    adaptation_template.auf2 = 0;
    adaptation_template.hh = 0;
    adaptation_template.mm = 0;
    adaptation_template.ss = 0;
    adaptation_template.ff = 0;
    
    adaptation_template.payload_unit_start = 0;
    adaptation_template.pts = 0;
    adaptation_template.continuity_counter = 0;
    
    adaptation_template.adaptation_field_control = 3;
    adaptation_template.adaptation_field_length = 0;
    for (w1 = 0; w1 < 188; w1++)
        adaptation_template.payload[w1] = 0xff;	// Stuffing
    adaptation_template.continuity_counter = 0;
    
    ts_packet_int[0] = 0x47;
    ts_packet_int[1] = (adaptation_template.transport_error << 7);
    ts_packet_int[1] |= (adaptation_template.payload_unit_start << 6);
    ts_packet_int[1] |= (adaptation_template.transport_priority << 5);
    ts_packet_int[1] |= ((adaptation_template.pid >> 8) & 0x1f);
    ts_packet_int[2] = (adaptation_template.pid & 0xff);
    ts_packet_int[3] = (adaptation_template.transport_scrambling_control << 6);
    ts_packet_int[3] |= (adaptation_template.adaptation_field_control << 4);
    ts_packet_int[3] |= (adaptation_template.continuity_counter & 0xf);
    bpnt = 4;
    ts_packet_int[4] = (adaptation_template.adaptation_field_length);
    ts_packet_int[5] = (adaptation_template.discontinuity_indicator << 7);
    ts_packet_int[5] |= (adaptation_template.random_access_indicator << 6);
    ts_packet_int[5] |= (adaptation_template.elementery_stream_priority_indicator << 5);
    ts_packet_int[5] |= (adaptation_template.flags_5);
    bpnt = 6;
    
    if (adaptation_template.do_pcr)
    {
        ts_packet_int[6] = 0x800;
        ts_packet_int[7] = 0x900;
        ts_packet_int[8] = 0xa00;
        ts_packet_int[9] = 0xb00;
        ts_packet_int[10] = 0xc00;
        ts_packet_int[11] = 0xd00;
        bpnt = 12;
    }
    
    for (w1 = bpnt; w1 < 188; w1++)
        ts_packet_int[w1] = 0xff;
    
    // Move data pack to payload field
    for (w1 = 0; w1 < 188; w1++)
        adaptation_template.int_payload[w1] = ts_packet_int[w1];
    
    adaptation_template_length = bpnt;
    
    return 0;
}


// Sets payload parameter register based on RTL requirements
int32_t CNTV2TsHelper::set_payload_params(void)
{
    payload_params = j2k_vid_descriptors[0].associated_pid;
    return 0;
}


// Sets payload parameter register based on RTL requirements
int32_t CNTV2TsHelper::set_time_regs(void)
{
    double d1, d2;
    
    // First packet rate
    d1 = ts_bitrate / 8.0 / 188.0;      // Packet Rate
    d1 = 1.0 / d1;                      // Packet Period
    d2 = 1.0 / sys_clk;                 // Clock Period
    d1 = d1 / d2 - 1.0;                 // One less as it counts from 0
    
    ts_gen_tc = (int32_t) d1;
    
    // Next PAT / PMT Transmission Rate
    d1 = table_tx_period / d2 - 1.0;
    pat_pmt_period = (int32_t) d1;
    
    return 0;
}


// Converts an integer to 2 bytes
int32_t CNTV2TsHelper::bytes16(int32_t src, uint8_t *dest)
{
    dest[0] = (uint8_t) ((src >> 8) & 0xff);
    dest[1] = (uint8_t) (src & 0xff);
    
    return 0;
}


// Converts an unsigned integer to 4 bytes
int32_t CNTV2TsHelper::bytes32(uint32_t src, uint8_t *dest)
{
    dest[0] = (uint8_t) ((src >> 24) & 0xff);
    dest[1] = (uint8_t) ((src >> 16) & 0xff);
    dest[2] = (uint8_t) ((src >> 8) & 0xff);
    dest[3] = (uint8_t) (src & 0xff);
    
    return 0;
}


// Searches the PID list for a PID.  Returns 1 if it is already there, 0 if not.
int32_t CNTV2TsHelper::search_pid_list(int32_t pid)
{
    int32_t w2, w3;
    
    w3 = 0;
    if (pid_list_cnt)
    {
        for (w2 = 0; w2 < pid_list_cnt; w2++)
        {
            if (prog_pid_list[w2].used && (prog_pid_list[w2].pid == pid))
                w3 = 1;
        }
    }
    
    return w3;
}


// Clears PID List, PAT Tables, and PMT Tables
int32_t CNTV2TsHelper::clear_tables(void)
{
    int32_t w1;
    
    for (w1 = 0; w1 < 256; w1++)
    {
        pat_table[w1].used = 0;
        pat_table[w1].errs_done = 0;
        pat_table[w1].different_pat_cnt = 0;
    }
    
    for (w1 = 0; w1 < MAX_PROGS; w1++)
    {
        pmt_tables[w1].used = 0;
        pmt_tables[w1].errs_done = 0;
        pmt_tables[w1].different_pmt_cnt = 0;
    }
    
    for (w1 = 0; w1 < PID_LIST_SIZE; w1++)
    {
        prog_pid_list[w1].used = 0;
        prog_pid_list[w1].pcr_pid = 0;
        prog_pid_list[w1].j2k_stream = 0;
        prog_pid_list[w1].non_j2k_stream = 0;
        prog_pid_list[w1].pat_table = 0;
        prog_pid_list[w1].pmt_table = 0;
        prog_pid_list[w1].cont_cnt = -1;
        prog_pid_list[w1].found = 0;
        prog_pid_list[w1].pcr_pid_check = 0;
        prog_pid_list[w1].j2k_stream_check = 0;
        prog_pid_list[w1].stream_check = 0;
    }
    
    pid_list_cnt = 0;
    j2k_channel_cnt = 0;
    for (w1 = 0; w1 < NUM_J2K_CHANNELS; w1++)
        j2k_vid_descriptors[w1].used = 0;
    
    return 0;
}


// Builds PAT and PMT tables.  PID List and J2K descriptors get built as part of that process.
int32_t CNTV2TsHelper::build_all_tables(void)
{
    int32_t w1, w2, w3;
    int32_t err;
    int32_t pmt_progs[MAX_PROGS];
    int32_t pmt_pids[MAX_PROGS];
    int32_t prog_cnt;
    
    prog_cnt = 0;
    
    err = 0;
    
    // First do PAT Table, and build a list of PMT table prog/pid pairs for PMT generation
    w2 = 0;
    for (w1 = 0; w1 < 256; w1++)
    {
        if (pat_table[w1].used)
        {
            w2 = 1;
            if (build_pat(w1))
            {
                printf("ERROR Building PAT Table Section %i\n", w1);
                err = 1;
            }
            else
            {
                if (pat_table[w1].number_of_progs)
                {
                    for (w3 = 0; w3 < pat_table[w1].number_of_progs; w3++)
                    {
                        pmt_progs[prog_cnt] = pat_table[w1].program_num[w3];
                        pmt_pids[prog_cnt] = pat_table[w1].program_pid[w3];
                        prog_cnt++;
                        if (prog_cnt >= MAX_PROGS)
                        {
                            printf("ERROR: Number of programs limited to %i.  More than that number are defined in the PAT descriptor.\n", MAX_PROGS);
                            err = 1;
                            prog_cnt = 0;
                            w3 = pat_table[w1].number_of_progs;
                        }
                    }
                }
            }
        }
    }
    if (!w2)
    {
        printf("ERROR - No valid PAT sections found.\n");
        err = 1;
    }
    if (err)
        return (1);
    
    // Now build PMT tables
    if (!prog_cnt)
    {
        printf ("ERROR: No programs defined in PAT table.\n");
        return (1);
    }
    
    pmt_program_number = pmt_progs[0];	// 1 program for current hardware
    
    w2 = 0;
    for (w1 = 0; w1 < prog_cnt; w1++)
    {
        if (!pmt_tables[pmt_progs[w1]].used)
        {
            printf("ERROR: PMT table for program %i, PID %i (0x%x) required by the PAT is not set up.\n", pmt_progs[w1], pmt_pids[w1], pmt_pids[w1]);
            err = 1;
        }
        else
        {
            w2 = 1;
            if (build_pmt(pmt_progs[w1], pmt_pids[w1]))
            {
                printf("ERROR building PMT for Program %i, PID %i (0x%x).\n", pmt_progs[w1], pmt_pids[w1], pmt_pids[w1]);
                err = 1;
            }
        }
    }
    
    if (!w2)
    {
        printf("ERROR - No valid PMT programs found.\n");
        err = 1;
    }
    
    return (err);
}


// Builds J2K Descriptor Payload from fields in the structure.
// Returns 1 if there was an error
int32_t CNTV2TsHelper::build_j2k_descriptor(int32_t desc_num)
{
    int32_t w1;
    int32_t err;
    double d1;
    double frames;
    
    err = 0;
    
    if (!j2k_vid_descriptors[desc_num].used)
    {
        printf("ERROR: J2K Descriptor for channel %i not populated, so can't be build.\n", desc_num);
        return (1);
    }
    
    j2k_vid_descriptors[desc_num].descriptor_tag = 0x32;	// Always 0x32
    j2k_vid_descriptors[desc_num].payload[0] = (uint8_t) j2k_vid_descriptors[desc_num].descriptor_tag;
    if (j2k_vid_descriptors[desc_num].descriptor_tag != 0x32)
    {
        printf("ERROR: Expected a J2K Descriptor of 0x32, found 0x%x.\n", j2k_vid_descriptors[desc_num].descriptor_tag);
        err = 1;
    }
    
    if (j2k_vid_descriptors[desc_num].private_data_len > 128)
    {
        printf("WARNING: Limit of 128 bytes on J2K Video Descriptor Private Data for this version of software - setting to 0.\n");
        j2k_vid_descriptors[desc_num].private_data_len = 0;
    }
    
    j2k_vid_descriptors[desc_num].descriptor_length = j2k_vid_descriptors[desc_num].private_data_len + 24;
    j2k_vid_descriptors[desc_num].payload[1] = (uint8_t) j2k_vid_descriptors[desc_num].descriptor_length;
    bytes16(j2k_vid_descriptors[desc_num].profile_level, &j2k_vid_descriptors[desc_num].payload[2]);

    if (j2k_vid_descriptors[desc_num].stream_type == kJ2KStreamTypeEvertz)
    {
        bytes16(0x0100, &j2k_vid_descriptors[desc_num].payload[4]);
        bytes16(j2k_vid_descriptors[desc_num].horizontal_size, &j2k_vid_descriptors[desc_num].payload[6]);
        bytes16(j2k_vid_descriptors[desc_num].vertical_size, &j2k_vid_descriptors[desc_num].payload[8]);
        if (j2k_vid_descriptors[desc_num].interlaced_video)
        {
            bytes16(j2k_vid_descriptors[desc_num].vertical_size, &j2k_vid_descriptors[desc_num].payload[10]);
        }
        else
        {
            j2k_vid_descriptors[desc_num].payload[10] = 0;
            j2k_vid_descriptors[desc_num].payload[11] = 0;
        }
        bytes32(j2k_vid_descriptors[desc_num].max_bit_rate, &j2k_vid_descriptors[desc_num].payload[12]);
        j2k_vid_descriptors[desc_num].payload[16] = 0;
        j2k_vid_descriptors[desc_num].payload[17] = 0;
        j2k_vid_descriptors[desc_num].payload[18] = 0x05;
        j2k_vid_descriptors[desc_num].payload[19] = 0x33;
        bytes16(j2k_vid_descriptors[desc_num].den_frame_rate, &j2k_vid_descriptors[desc_num].payload[20]);
        bytes16(j2k_vid_descriptors[desc_num].num_frame_rate, &j2k_vid_descriptors[desc_num].payload[22]);
        j2k_vid_descriptors[desc_num].payload[24] = 0;
        j2k_vid_descriptors[desc_num].payload[25] = 0;
    }
    else
    {
        bytes32(j2k_vid_descriptors[desc_num].horizontal_size, &j2k_vid_descriptors[desc_num].payload[4]);
        bytes32(j2k_vid_descriptors[desc_num].vertical_size, &j2k_vid_descriptors[desc_num].payload[8]);
        bytes32(j2k_vid_descriptors[desc_num].max_bit_rate, &j2k_vid_descriptors[desc_num].payload[12]);
        bytes32(j2k_vid_descriptors[desc_num].max_buffer_size, &j2k_vid_descriptors[desc_num].payload[16]);
        bytes16(j2k_vid_descriptors[desc_num].den_frame_rate, &j2k_vid_descriptors[desc_num].payload[20]);
        bytes16(j2k_vid_descriptors[desc_num].num_frame_rate, &j2k_vid_descriptors[desc_num].payload[22]);
        j2k_vid_descriptors[desc_num].payload[24] = (uint8_t) j2k_vid_descriptors[desc_num].color_spec;
        j2k_vid_descriptors[desc_num].payload[25] = (uint8_t) (j2k_vid_descriptors[desc_num].still_mode) << 7;
        j2k_vid_descriptors[desc_num].payload[25] |= (uint8_t) (j2k_vid_descriptors[desc_num].interlaced_video) << 6;

    }
    
    if (j2k_vid_descriptors[desc_num].private_data_len)
    {
        for (w1 = 0; w1 < j2k_vid_descriptors[desc_num].private_data_len; w1++)
            j2k_vid_descriptors[desc_num].payload[w1 + 26] = j2k_vid_descriptors[desc_num].private_data[w1];
    }
    
    // Handle timing
    d1 = (double) j2k_vid_descriptors[desc_num].num_frame_rate / (double) j2k_vid_descriptors[desc_num].den_frame_rate;
    fps = d1 - d1 * 15.0/1e6;
    frames = fps * (14 * 3600.0 + 3 * fps + 48.0);
    total_time = frames * 1.0 / fps;
    d1 = total_time / 60.0;
    w1 = (int32_t) d1;
    d1 = d1 - (double) w1;
    
    // Initialize fbw frame rate and packets per frame based on required total bandwidth
    if (generate_fbw)
    {
        fbw_frame_rate = (double) j2k_vid_descriptors[desc_num].num_frame_rate / (double) j2k_vid_descriptors[desc_num].den_frame_rate;
        fbw_pkts_per_frame = (fbw_bandwidth * 1.0 / fbw_frame_rate);	// Bits
        fbw_pkts_per_frame = fbw_pkts_per_frame / 188.0 / 8.0;
    }
    
    return (err);
}


uint32_t CNTV2TsHelper::chksum_crc32(uint8_t *data, int32_t len)
{
    uint32_t crc_table[256] = {
        0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
        0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
        0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
        0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
        0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
        0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
        0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
        0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
        0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
        0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
        0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
        0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
        0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
        0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
        0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
        0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
        0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
        0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
        0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
        0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
        0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
        0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
        0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
        0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
        0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
        0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
        0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
        0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
        0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
        0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
        0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
        0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
        0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
        0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
        0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
        0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
        0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
        0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
        0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
        0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
        0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
        0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
        0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
    };
    
    int32_t i;
    uint32_t crc = 0xffffffff;
    
    for (i=0; i<len; i++)
        crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *data++) & 0xff];
    
    return crc;
}


// Builds a PAT Table payload from fields - calculates CRC
int32_t CNTV2TsHelper::build_pat(int32_t section)
{
    int32_t err;
    int32_t pnt;
    int32_t w1;
    int32_t crc_start;
    uint32_t crc_res;
    
    err = 0;
    
    if (!pat_table[section].used)
    {
        printf("ERROR: 'used' bit in section %i not set.\n", section);
        return (1);
    }
    
    // First build TS Header
    pat_table[section].payload[0] = 0x47; 	// Sync byte
    pat_table[section].payload[1] = (uint8_t) (pat_table[section].payload_unit_start << 6);
    pat_table[section].payload[2] = 0;		// PID for PAT
    pat_table[section].payload[3] = 0x10;	// NOTE: Continuity Counter must increment when transmitted
    
    // Now build PAT Table
    if (pat_table[section].payload_unit_start)
    {
        pat_table[section].payload[4] = 0;
        pnt = 5;
    }
    else
        pnt = 4;
    
    crc_start = pnt;
    pat_table[section].table_id = 0;
    pat_table[section].payload[pnt++] = pat_table[section].table_id;
    pat_table[section].payload[pnt] = 0xb0;
    pat_table[section].section_length = 9 + 4 * pat_table[section].number_of_progs;
    pat_table[section].payload[pnt++] |= (uint8_t) ((pat_table[section].section_length >> 8) & 0x3);
    pat_table[section].payload[pnt++] = (uint8_t) (pat_table[section].section_length & 0xff);
    bytes16(pat_table[section].transport_stream_id, &pat_table[section].payload[pnt]);
    pnt += 2;
    pat_table[section].payload[pnt] = 0xc0;
    pat_table[section].payload[pnt] |= (uint8_t) (pat_table[section].version_number & 0x1f);
    pat_table[section].payload[pnt++] |= (uint8_t) (pat_table[section].current_next);
    pat_table[section].payload[pnt++] = (uint8_t) pat_table[section].section_num;
    pat_table[section].payload[pnt++] = (uint8_t) pat_table[section].last_section_num;
    for (w1 = 0; w1 < pat_table[section].number_of_progs; w1++)
    {
        bytes16(pat_table[section].program_num[w1], &pat_table[section].payload[pnt]);
        pnt += 2;
        bytes16(pat_table[section].program_pid[w1], &pat_table[section].payload[pnt]);
        pat_table[section].payload[pnt] |= 0xe0;
        pnt += 2;
    }
    crc_res = chksum_crc32(&pat_table[section].payload[crc_start], pnt - crc_start);
    pat_table[section].crc = crc_res;
    bytes32(crc_res, &pat_table[section].payload[pnt]);
    pnt += 4;
    for (w1 = pnt; w1 < 188; w1++)
        pat_table[section].payload[pnt++] = 0xff;
    
    // If there is no error, add the PAT to the PID list if it isn't there
    if (!err && !search_pid_list(0))
    {
        prog_pid_list[pid_list_cnt].used = 1;
        prog_pid_list[pid_list_cnt].pid = 0;
        prog_pid_list[pid_list_cnt].pat_table = 1;
        pid_list_cnt++;
    }
    return err;
}


// Build PMT Table
int32_t CNTV2TsHelper::build_pmt(int32_t prog, int32_t pid)
{
    int32_t err;
    int32_t pnt;
    int32_t w1, w2;
    int32_t crc_start;
    uint32_t crc_res;
    int32_t section_length_psn;
    
    err = 0;
    
    if (!pmt_tables[prog].used)
    {
        printf("ERROR: 'used' bit in PMT program %i not set.\n", prog);
        return (1);
    }
    
    // First build TS Header
    pmt_tables[prog].payload[0] = 0x47; 	// Sync byte
    pmt_tables[prog].payload[1] = (uint8_t) (pmt_tables[prog].payload_unit_start << 6); 
    pmt_tables[prog].payload[1] |= (uint8_t) ((pid >> 8) & 0x1f);
    pmt_tables[prog].payload[2] = (uint8_t) (pid & 0xff);		
    pmt_tables[prog].payload[3] = 0x10;	// NOTE: Continuity Counter must increment when transmitted
    
    // Now build PMT Table
    if (pmt_tables[prog].payload_unit_start)
    {
        pmt_tables[prog].payload[4] = 0;
        pnt = 5;	
    }
    else
        pnt = 4;
    
    crc_start = pnt;
    pmt_tables[prog].table_id = 2;
    pmt_tables[prog].payload[pnt++] = pmt_tables[prog].table_id;
    pmt_tables[prog].payload[pnt] = 0xb0; 
    pnt++;
    section_length_psn = pnt;	// Have to fill in section length later
    pnt++;
    bytes16(pmt_tables[prog].program_num, &pmt_tables[prog].payload[pnt]);
    pnt += 2;
    pmt_tables[prog].payload[pnt] = 0xc0;
    pmt_tables[prog].payload[pnt] |= (uint8_t) (pmt_tables[prog].version_number & 0x1f);
    pmt_tables[prog].payload[pnt++] |= (uint8_t) (pmt_tables[prog].current_next);
    pmt_tables[prog].payload[pnt++] = 0;
    pmt_tables[prog].payload[pnt++] = 0;
    bytes16(pmt_tables[prog].pcr_pid, &pmt_tables[prog].payload[pnt]);
    pmt_tables[prog].payload[pnt] |= 0xe0;	// Set reserved bits
    pnt += 2;
    bytes16(pmt_tables[prog].prog_info_length, &pmt_tables[prog].payload[pnt]);
    pmt_tables[prog].payload[pnt] |= 0xf0;	// Set reserved bits
    pnt += 2;
    if (pmt_tables[prog].prog_info_length > 128)
    {
        printf("ERROR: PMT Program Descriptor length limited to 128 - %i requested.\n", pmt_tables[prog].prog_info_length);
        pmt_tables[prog].prog_info_length = 0;
        err = 1;
    }
    if (pmt_tables[prog].prog_info_length)
    {
        for (w1 = 0; w1 < pmt_tables[prog].prog_info_length; w1++)
            pmt_tables[prog].payload[pnt++] = pmt_tables[prog].prog_descriptor[w1];
    }
    
    // Now do the streams
    for (w1 = 0; w1 < pmt_tables[prog].num_streams; w1++)
    {
        if (pmt_tables[prog].stream_descriptors[w1].j2k_stream) // Build and get the J2K stream if applicable
        {
            pmt_tables[prog].stream_descriptors[w1].stream_type = 0x21;		// J2K Type
            w2 = pmt_tables[prog].stream_descriptors[w1].j2k_channel_num;
            j2k_vid_descriptors[w2].associated_pmt_prog = prog;
            j2k_vid_descriptors[w2].associated_pmt_stream = w1;
            j2k_vid_descriptors[w2].associated_pmt_pid = pid;
            j2k_vid_descriptors[w2].associated_pid = pmt_tables[prog].stream_descriptors[w1].elementary_pid;
            
            if (build_j2k_descriptor(w2))
            {
                printf("ERROR: Can't build PMT table due to error in J2K Descriptor build.\n");
                pmt_tables[prog].stream_descriptors[w1].es_info_length = 0;
                err = 1;
            }
            else
            {
                pmt_tables[prog].stream_descriptors[w1].es_info_length = j2k_vid_descriptors[pmt_tables[prog].stream_descriptors[w1].j2k_channel_num].descriptor_length + 2;
                for (w2 = 0; w2 < pmt_tables[prog].stream_descriptors[w1].es_info_length; w2++)
                    pmt_tables[prog].stream_descriptors[w1].es_descriptor[w2] = j2k_vid_descriptors[pmt_tables[prog].stream_descriptors[w1].j2k_channel_num].payload[w2];	
            }
        }
        pmt_tables[prog].payload[pnt++] = (uint8_t) pmt_tables[prog].stream_descriptors[w1].stream_type;	
        bytes16(pmt_tables[prog].stream_descriptors[w1].elementary_pid, &pmt_tables[prog].payload[pnt]);
        pmt_tables[prog].payload[pnt] |= 0xe0;
        pnt += 2;
        if ((pmt_tables[prog].stream_descriptors[w1].es_info_length + pnt + 4) >= 188)
        {
            printf("ERROR: Can't generate PMT table, as TS Packet length would be exceeded.  PMT limited to 1 TS Packet in current implementation.\n");
            err = 1;
            pmt_tables[prog].stream_descriptors[w1].es_info_length = 0;
        }
        bytes16(pmt_tables[prog].stream_descriptors[w1].es_info_length, &pmt_tables[prog].payload[pnt]);
        pmt_tables[prog].payload[pnt] |= 0xf0;
        pnt += 2;
        if (pmt_tables[prog].stream_descriptors[w1].es_info_length)
        {
            for (w2 = 0; w2 < pmt_tables[prog].stream_descriptors[w1].es_info_length; w2++)
                pmt_tables[prog].payload[pnt++] = pmt_tables[prog].stream_descriptors[w1].es_descriptor[w2];	
        }
    }
    
    pmt_tables[prog].section_length = pnt - section_length_psn - 1 + 4;	// Section field starts 1 byte after len field psn (hence -1), +4 is CRC
    pmt_tables[prog].payload[section_length_psn] = (uint8_t) pmt_tables[prog].section_length;
    
    crc_res = chksum_crc32(&pmt_tables[prog].payload[crc_start], pnt - crc_start);
    pmt_tables[prog].crc = crc_res;
    bytes32(crc_res, &pmt_tables[prog].payload[pnt]);
    pnt += 4;
    for (w1 = pnt; w1 < 188; w1++)
        pmt_tables[prog].payload[pnt++] = 0xff;
    
    // If there is no error, add the PMT to the PID list if it isn't there.  Warn if it is already in the PID list
    if (search_pid_list(pid))
        printf("WARNING: PID for PMT Table already in PID list.\n");
    
    if (!err && !search_pid_list(pid))
    {
        prog_pid_list[pid_list_cnt].used = 1;
        prog_pid_list[pid_list_cnt].pid = pid;
        prog_pid_list[pid_list_cnt].pmt_table = 1;
        prog_pid_list[pid_list_cnt].assoc_prog_number = prog;
        pid_list_cnt++;	
    }
    
    // Now add TS Streams included in this PMT to the PID list
    if (!err)
    {
        for (w1 = 0; w1 < pmt_tables[prog].num_streams; w1++)
        {
            if (search_pid_list(pmt_tables[prog].stream_descriptors[w1].elementary_pid))
                printf("WARNING: PID List for Stream %i in PMT %i (PMT PID = %i) already in PID list.", w1, prog, pid);
            else
            {
                prog_pid_list[pid_list_cnt].used = 1;
                prog_pid_list[pid_list_cnt].pid = pmt_tables[prog].stream_descriptors[w1].elementary_pid;
                if (pmt_tables[prog].stream_descriptors[w1].elementary_pid == pmt_tables[prog].pcr_pid)
                    prog_pid_list[pid_list_cnt].pcr_pid = 1;
                if (pmt_tables[prog].stream_descriptors[w1].j2k_stream)
                    prog_pid_list[pid_list_cnt].j2k_stream = 1;
                else
                    prog_pid_list[pid_list_cnt].non_j2k_stream = 1;
                
                prog_pid_list[pid_list_cnt].assoc_prog_number = prog;
                prog_pid_list[pid_list_cnt].assoc_stream_number = w1;	
                pid_list_cnt++;
            }
        }
    }
    
    return err;
}
