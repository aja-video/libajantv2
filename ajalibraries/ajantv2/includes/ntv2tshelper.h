/**
	@file		ntv2tshelper.h
	@brief		Declares the CNTV2TsHelper class that is fundamental in generating register settings for IPX MPEG2-TS encapsulator.
                This is a class for generating Transport Stream encode / decode programming data, and decoding ingress PAT/PMT and 
                PES packets to setup decap for different streams. 
 
                This is modified version of the original code from Macnica.
 **/

#ifndef NTV2TSHELPER_H
#define NTV2TSHELPER_H

#include <stdint.h>
#include <map>
#include <stdio.h>

#define MAX_PROGS                   2
#define MAX_STREAM_DESCRIPTORS      8
#define NUM_J2K_CHANNELS            4
#define PID_LIST_SIZE               32
#define PAT_TABLE_SIZE              32

typedef enum
{
    kJ2KStreamTypeStandard,
    kJ2KStreamTypeEvertz
} J2KStreamType;

typedef enum
{
    kJ2KChromaSubSamp_444,
    kJ2KChromaSubSamp_422_444,
    kJ2KChromaSubSamp_422_Standard
} J2KChromaSubSampling;

typedef enum
{
    kJ2KCodeBlocksize_32x32,
    kJ2KCodeBlocksize_32x64,
    kJ2KCodeBlocksize_64x32 = 4,
    kJ2KCodeBlocksize_64x64,
    kJ2KCodeBlocksize_128x32 = 12
} J2KCodeBlocksize;

typedef struct TsEncapStreamData
{
    J2KStreamType   j2kStreamType;
    uint32_t        width;
    uint32_t        height;
    uint32_t        denFrameRate;
    uint32_t        numFrameRate;
    uint32_t        programPid;
    uint32_t        videoPid;
    uint32_t        pcrPid;
    uint32_t        audio1Pid;
    bool            interlaced;
    bool            doPCR;
} TsEncapStreamData;



// Need NULL packet generation

typedef struct ts_packet_tds
{
    bool            transport_error;
    bool            payload_unit_start;
    bool            transport_priority;
    int32_t         pid;
    int32_t         transport_scrambling_control;
    int32_t         adaptation_field_control;
    int32_t         continuity_counter;
    int32_t         packet_start_code_prefix;
    bool            adaptation_field_present;
    int32_t         adaptation_field_length;
    bool            discontinuity_indicator;
    bool            random_access_indicator;
    bool            elementery_stream_priority_indicator;
    int32_t         flags_5;
    bool            do_pcr;
    bool            pcr_flag;
    uint64_t        pcr_base;
    int32_t         pcr_extension;
    bool            opcr_flag;
    bool            splicing_point_flag;
    bool            transport_private_data_flag;
    bool            adaptation_field_extension_flag;
    int32_t         stream_id;
    bool            pes_header_present;
    int32_t         pes_packet_length;
    int32_t         pes_scrambling_control;
    bool            pes_priority;
    bool            data_alignment_indicator;
    bool            copyright;
    bool            original_copy;
    int32_t         flags_7;
    int32_t         pes_data_header_length;
    uint64_t        pts;
    bool            j2k_esh;
    int32_t         frat_denominator;
    int32_t         frat_numerator;
    bool            interlaced_video;
    uint32_t        max_br;
    uint32_t        auf1;
    uint32_t        auf2;
    int32_t         fic;
    int32_t         fio;
    int32_t         hh;
    int32_t         mm;
    int32_t         ss;
    int32_t         ff;
    int32_t         bcol_colcr;
    uint8_t         payload[188];
    int32_t         int_payload[188];
} ts_packet;

typedef struct pat_table_tds
{
    bool            used;
    bool            payload_unit_start;     // from TS Header
    int32_t         pointer_field;
    int32_t         table_id;
    int32_t         section_length;
    int32_t         transport_stream_id;
    int32_t         version_number;
    bool            current_next;
    int32_t         section_num;
    int32_t         last_section_num;
    int32_t         number_of_progs;
    int32_t         program_num[MAX_PROGS];
    int32_t         program_pid[MAX_PROGS];
    int32_t         different_pat_cnt;
    uint32_t        crc;
    bool            errs_done;
    uint8_t         payload[256];           // IMPORTANT NOTE: MUST include the TS Header so that payload_unit_start can be extracted
} pat_table_type;

typedef struct stream_descriptor_tds
{
    int32_t         stream_type;
    bool            j2k_stream;
    int32_t         j2k_channel_num;
    int32_t         elementary_pid;
    int32_t         es_info_length;
    uint8_t         es_descriptor[256];
} stream_descriptor;

// NOTE: Current limitation is 1 section per program
typedef struct pmt_table_tds
{
    bool            used;
    bool            payload_unit_start;     // from TS Header
    int32_t         pointer_field;
    int32_t         table_id;
    int32_t         section_length;
    int32_t         program_num;
    int32_t         version_number;
    int32_t         current_next;
    int32_t         section_num;
    int32_t         last_section_num;
    int32_t         pcr_pid;
    int32_t         prog_info_length;
    uint8_t         prog_descriptor[256];
    int32_t         num_streams;
    int32_t         stream_desc_len;
    stream_descriptor stream_descriptors[MAX_STREAM_DESCRIPTORS];
    bool            errs_done;
    uint8_t         payload[256];           // IMPORTANT NOTE: MUST include the TS Header so that payload_unit_start can be extracted
    int32_t         different_pmt_cnt;
    uint32_t        crc;
} pmt_table;

typedef struct prog_pid_list_tds
{
    bool            used;
    int32_t         pid;
    bool            pcr_pid;
    int32_t         assoc_prog_number;
    int32_t         assoc_stream_number;
    bool            j2k_stream;
    bool            non_j2k_stream;
    bool            pat_table;
    bool            pmt_table;
    int32_t         j2k_channel_num;
    int32_t         cont_cnt;               // Used for checking captures
    bool            found;                  // Used for checking captures
    bool            pcr_pid_check;
    bool            j2k_stream_check;
    bool            stream_check;
} prog_pid_list_type;

typedef struct j2k_vid_descriptor_tds
{
    bool            used;
    int32_t         associated_pid;
    int32_t         associated_pmt_pid;
    int32_t         associated_pmt_prog;
    int32_t         associated_pmt_stream;
    int32_t         descriptor_tag;
    int32_t         descriptor_length;
    int32_t         profile_level;
    uint32_t        horizontal_size;
    uint32_t        vertical_size;
    uint32_t        max_bit_rate;
    uint32_t        max_buffer_size;
    int32_t         den_frame_rate;
    int32_t         num_frame_rate;
    int32_t         color_spec;
    J2KStreamType   stream_type;
    bool            still_mode;
    bool            interlaced_video;
    int32_t         private_data_len;
    uint8_t         private_data[256];
    uint8_t         payload[300];
} j2k_vid_descriptor_type;

class CNTV2TsHelper
{
public:
    
    CNTV2TsHelper ();  // class constructor
    ~CNTV2TsHelper (); // class destructor

    void        init(TsEncapStreamData streamData);


    int32_t     setup_tables(J2KStreamType streamType, uint32_t width, uint32_t height, int32_t denFrameRate, int32_t numFrameRate, bool interlaced);
    int32_t     build_all_tables(void);
    int32_t     build_j2k_descriptor(int32_t desc_num);
    int32_t     build_pat(int32_t section);
    int32_t     build_pmt(int32_t prog, int32_t pid);
    int32_t     gen_pes_lookup(void);
    int32_t     gen_adaptation_lookup(void);
    int32_t     set_payload_params(void);
    int32_t     set_time_regs(void);
    int32_t     clear_tables(void);
    
    ts_packet                   gen_template;
    ts_packet                   pes_template;
    ts_packet                   adaptation_template;
    
    int32_t                     pes_template_len;
    int32_t                     pts_offset;
    int32_t                     j2k_ts_offset;
    int32_t                     auf1_offset;
    int32_t                     auf2_offset;
    
    int32_t                     payload_params;
    
    int32_t                     adaptation_template_length;
    
    pat_table_type              pat_table[PAT_TABLE_SIZE];
    pmt_table                   pmt_tables[MAX_PROGS];
    int32_t                     pmt_program_number;
    int32_t                     j2k_channel_cnt;
    j2k_vid_descriptor_type     j2k_vid_descriptors[NUM_J2K_CHANNELS];
    prog_pid_list_type          prog_pid_list[PID_LIST_SIZE];
    int32_t                     pid_list_cnt;
    uint64_t                    start_time;
    double                      fps;
    double                      total_time;
    
    // Variables used for generating fixed bandwidth streams
    bool                        generate_fbw;               // Default 1 - enables NULL insertion for fixed bandwidth generation
    double                      fbw_bandwidth;              // This is the total bandwidth of the TS stream to be generated (default 80 Mbps)
    double                      fbw_frame_rate;
    double                      fbw_pkts_per_frame;
    int32_t                     fbw_total_frames;           // Frame counter
    int32_t                     fbw_total_packets;          // Packet counter
    
    double                      table_tx_period;
    
    int32_t                     ts_gen_tc;                  // TS Packet Generation Period register
    int32_t                     pat_pmt_period;             // Period of transmission register
    
    TsEncapStreamData           tsStreamData;

private:
    
    int32_t                     bytes16(int32_t src, uint8_t *dest);
    int32_t                     bytes32(uint32_t src, uint8_t *dest);
    uint32_t                    chksum_crc32(uint8_t *data, int32_t len);
    int32_t                     search_pid_list(int32_t pid);
};

typedef struct TsVideoStreamData
{
    J2KStreamType   j2kStreamType;
    uint32_t        width;
    uint32_t        height;
    uint32_t        denFrameRate;
    uint32_t        numFrameRate;
    bool            interlaced;
    bool            doPcr;
} TsVideoStreamData;

class TSGenerator
{
    public:
        // Input
        uint16_t _tsId;
        uint8_t _version;

        // Generated packet
        uint8_t _pkt[188];

    public:
        TSGenerator()
        {
            init();
            initPacket();
        }

        ~TSGenerator()
        {
        }

        void init()
        {
            _tsId = 1;
            _version = 1;
        }

        void initPacket()
        {
            for ( int i = 0; i < 188; i++ )
            {
                _pkt[i] = 0xff;
            }
        }

        uint32_t chksum_crc32(unsigned char *data, int len)
        {
            uint32_t crc_table[256] =
            {
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

        void dump()
        {
            for (int i=0; i<188; i++)
            {
                if (i % 16 == 15)
                    printf("0x%02x\n", _pkt[i]);
                else
                    printf("0x%02x ", _pkt[i]);
            }
            printf("\n");
        }

    protected:
        void put16( uint16_t val, int &pos )
        {
            _pkt[pos++] = (uint8_t)(val>>8);
            _pkt[pos++] = val;
        }

        void put32( uint32_t val, int &pos )
        {
            _pkt[pos++] = (uint8_t)(val>>24);
            _pkt[pos++] = (uint8_t)(val>>16);
            _pkt[pos++] = (uint8_t)(val>>8);
            _pkt[pos++] = val;
        }
};

class PATGen : public TSGenerator
{
    public:
        // Input
        std::map <uint16_t, uint16_t> _progNumToPID;

    public:
        PATGen()
        {
            initLocal();
        }

        ~PATGen()
        {
        }

        void initLocal()
        {
            _progNumToPID.clear();
        }

        bool makePacket()
        {
            initPacket();
            int pos = 0;

            // Header
            _pkt[pos++] = 0x47;         // sync byte
            _pkt[pos++] = 1<<6;         // payload unit start indicator
            _pkt[pos++] = 0;            // PID for PAT = 0
            _pkt[pos++] = 0x10;         // Continuity Counter must increment when transmitted
            _pkt[pos++] = 0;            // pointer

            int crcStart = pos;

            _pkt[pos++] = 0; // table id = 0

            int length = 9 + (4*_progNumToPID.size());
            put16( (uint16_t)0xb000 + (length & 0x3ff), pos); // syntax indicator, reserved, length
            put16( _tsId, pos );

            _pkt[pos++] = 0xc1 + ((_version & 0x1f)<< 1); // version, current/next
            put16( 0, pos ); // section number = last section number = 0

            std::map <uint16_t, uint16_t>::const_iterator it = _progNumToPID.begin();
            while (it != _progNumToPID.end())
            {
                put16( it->first, pos );
                put16( (uint16_t)0xe000 + (it->second & 0x1fff), pos );
                it++;
            }
            int crcEnd = pos - 1;

            int crc = chksum_crc32(_pkt + crcStart, crcEnd - crcStart + 1 );
            put32( crc, pos );
            return true;
        }
};

class PMTGen : public TSGenerator
{
    public:
        TsVideoStreamData   _videoStreamData;
        std::map <uint16_t, uint16_t> _progNumToPID;
        std::map <uint16_t, uint16_t> _pcrNumToPID;
        std::map <uint16_t, uint16_t> _elemNumToPID;
        std::map <uint16_t, uint16_t> _audioNumToPID;

    public:
        PMTGen()
        {
            initLocal();
        }

        ~PMTGen()
        {
        }

        void initLocal()
        {
            _progNumToPID.clear();
            _pcrNumToPID.clear();
            _elemNumToPID.clear();
            _audioNumToPID.clear();
        }

        bool makePacket()
        {
            initPacket();
            int pos = 0;
            int len, lengthPos, j2kLengthPos, audioLengthPos;

            // Header
            _pkt[pos++] = 0x47;                                             // sync byte
            _pkt[pos] = 1<<6;                                               // payload unit start indicator
            _pkt[pos++] |= (uint8_t) ((_progNumToPID[1] >> 8) & 0x1f);      // PID for PMT
            _pkt[pos++] =  (uint8_t) (_progNumToPID[1] & 0xff);             // PID for PMT
            _pkt[pos++] = 0x10;                                             // Continuity Counter must increment when transmitted
            _pkt[pos++] = 0;                                                // pointer

            int crcStart = pos;

            _pkt[pos++] = 2;                                                // table id = 0
            lengthPos = pos;                                                // need to come back and fill in length so save position (assume < 256)
            _pkt[pos++] = 0xb0;
            pos++;

            put16(0x01, pos);                                               // program number

            _pkt[pos++] = 0xc1 + ((_version & 0x1f)<< 1);                   // version, current/next
            put16( 0, pos );                                                // section number = last section number = 0

            _pkt[pos] = 0xe0;                                               // PCR pid and reserved bits
            _pkt[pos++] |= (uint8_t) ((_pcrNumToPID[1] >> 8) & 0x1f);
            _pkt[pos++] =  (uint8_t) (_pcrNumToPID[1] & 0xff);

            _pkt[pos++] = 0xf0;                                             // reserved bits and program info length
            _pkt[pos++] = 0x00;                                             // reserved bits and program info length

            // Do the streams

            // J2K
            _pkt[pos++] = 0x21;                                             // J2K Type
            _pkt[pos] = 0xe0;                                               // elementary pid and reserved bits
            _pkt[pos++] |= (uint8_t) ((_elemNumToPID[1] >> 8) & 0x1f);
            _pkt[pos++] =  (uint8_t) (_elemNumToPID[1] & 0xff);

            j2kLengthPos = pos;                                             // need to come back and fill in descriptor length so save position
            pos+=2;

            len = makeJ2kDescriptor(pos);                                   // generate the J2K descriptor

            _pkt[j2kLengthPos] = 0xf0;                                      // fill in the length and reserved bits now
            _pkt[j2kLengthPos++] |= (uint8_t) ((len >> 8) & 0x1f);
            _pkt[j2kLengthPos] =  (uint8_t) (len & 0xff);

            // Audio
            _pkt[pos++] = 0x06;                                             // Audio Type
            _pkt[pos] = 0xe0;                                               // audio pid and reserved bits
            _pkt[pos++] |= (uint8_t) ((_audioNumToPID[1] >> 8) & 0x1f);
            _pkt[pos++] =  (uint8_t) (_audioNumToPID[1] & 0xff);

            audioLengthPos = pos;                                           // need to come back and fill in descriptor length so save position
            pos+=2;

            len = makeAudioDescriptor(pos);                                 // generate the audio descriptor

            _pkt[audioLengthPos] = 0xf0;                                    // fill in the length and reserved bits now
            _pkt[audioLengthPos++] |= (uint8_t) ((len >> 8) & 0x1f);
            _pkt[audioLengthPos] =  (uint8_t) (len & 0xff);

            // now we know the length so fill that in
            _pkt[lengthPos] = 0xb0;
            _pkt[lengthPos++] |= (uint8_t) (((pos-crcStart+1) >> 8) & 0x1f);
            _pkt[lengthPos] =  (uint8_t) ((pos-crcStart+1) & 0xff);

            int crcEnd = pos - 1;

            int crc = chksum_crc32(_pkt + crcStart, crcEnd - crcStart + 1 );
            put32( crc, pos );
            return true;
        }

        int makeJ2kDescriptor(int &pos)
        {
            int         startPos = pos;
            uint32_t    profileLevel;
            uint32_t    maxBitRate;
            uint32_t    height;

            if (_videoStreamData.j2kStreamType == kJ2KStreamTypeStandard)
            {
                profileLevel    = 0x102;
                maxBitRate      = 160000000;
            }
            else
            {
                profileLevel    = 0x101;
                maxBitRate      = 213000000;
            }

            if (_videoStreamData.interlaced) height = _videoStreamData.height/2;
            else height = _videoStreamData.height;

            // Header
            _pkt[pos++] = 0x32;                                             // descriptor tag
            _pkt[pos++] = 24;                                               // descriptor length
            put16( profileLevel, pos );                                     // profile level

            if (_videoStreamData.j2kStreamType == kJ2KStreamTypeStandard)
            {
                // Standard stream
                put32( _videoStreamData.width, pos );                       // width
                put32( height, pos );                                       // height
                put32( maxBitRate, pos );                                   // max bit rate
                put32( 1250000, pos );                                      // max buffer size (constant for now)
                put16( _videoStreamData.denFrameRate, pos );                // frame rate
                put16( _videoStreamData.numFrameRate, pos );
                _pkt[pos++] = 3;                                            // color spec
                _pkt[pos++] = _videoStreamData.interlaced <<6;              // interlaced and still mode
            }
            else
            {
                // Evertz stream
                put16( 0x0100, pos );
                put16( _videoStreamData.width, pos );                       // width
                put16( height, pos );                                       // height
                if (_videoStreamData.interlaced)
                    put16( height, pos );                                   // height again for interlaced
                else
                    put16( 0, pos );                                        // otherwise nothing
                put32( maxBitRate, pos );                                   // max bit rate (constant for now)
                _pkt[pos++] = 0;
                _pkt[pos++] = 0;
                _pkt[pos++] = 0x05;
                _pkt[pos++] = 0x33;
                put16( _videoStreamData.denFrameRate, pos );                // frame rate
                put16( _videoStreamData.numFrameRate, pos );
                _pkt[pos++] = 0;
                _pkt[pos++] = 0;
            }

            return pos-startPos;
        }

        int makeAudioDescriptor(int &pos)
        {
            int         startPos = pos;

            // Header
            _pkt[pos++] = 0x0a;                                             // descriptor tag
            _pkt[pos++] = 4;                                                // descriptor length
            _pkt[pos++] = 0x45;                                             // "E"
            _pkt[pos++] = 0x4e;                                             // "N"
            _pkt[pos++] = 0x47;                                             // "G"
            _pkt[pos++] = 0;

            _pkt[pos++] = 0x05;                                             // descriptor tag
            _pkt[pos++] = 6;                                                // length
            _pkt[pos++] = 0x42;                                             // "B"
            _pkt[pos++] = 0x53;                                             // "S"
            _pkt[pos++] = 0x53;                                             // "S"
            _pkt[pos++] = 0x44;                                             // "D"
            _pkt[pos++] = 0;
            _pkt[pos++] = 0x20;

            return pos-startPos;
        }
};

#endif
