/**
	@file		ntv2_2022_registers.h
	@brief		Defines the Sarek board's registers.
	@copyright	(C) 2014-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef REGISTERS_2110_H
#define REGISTERS_2110_H

#include "ntv2registersmb.h"

#define SAREK_4175_TX_PACKETIZER_1      (0x200000/4)
#define SAREK_4175_TX_PACKETIZER_2      (0x201000/4)
#define SAREK_4175_TX_PACKETIZER_3      (0x202000/4)
#define SAREK_4175_TX_PACKETIZER_4      (0x203000/4)

#define SAREK_3190_TX_PACKETIZER_0      (0x220000/4)
#define SAREK_3190_TX_PACKETIZER_1      (0x221000/4)
#define SAREK_3190_TX_PACKETIZER_2      (0x222000/4)
#define SAREK_3190_TX_PACKETIZER_3      (0x223000/4)
#define SAREK_3190_TX_PACKETIZER_4      (0x224000/4)
#define SAREK_3190_TX_PACKETIZER_5      (0x225000/4)
#define SAREK_3190_TX_PACKETIZER_6      (0x226000/4)
#define SAREK_3190_TX_PACKETIZER_7      (0x227000/4)
#define SAREK_3190_TX_PACKETIZER_8      (0x228000/4)
#define SAREK_3190_TX_PACKETIZER_9      (0x229000/4)
#define SAREK_3190_TX_PACKETIZER_10     (0x22a000/4)
#define SAREK_3190_TX_PACKETIZER_11     (0x22b000/4)
#define SAREK_3190_TX_PACKETIZER_12     (0x22c000/4)
#define SAREK_3190_TX_PACKETIZER_13     (0x22d000/4)
#define SAREK_3190_TX_PACKETIZER_14     (0x22e000/4)
#define SAREK_3190_TX_PACKETIZER_15     (0x22f000/4)

#define SAREK_ANC_TX_PACKETIZER_1       (0x304000/4)
#define SAREK_ANC_TX_PACKETIZER_2       (0x305000/4)
#define SAREK_ANC_TX_PACKETIZER_3       (0x306000/4)
#define SAREK_ANC_TX_PACKETIZER_4       (0x307000/4)

#define SAREK_4175_RX_DEPACKETIZER_1    (0x208000/4)
#define SAREK_4175_RX_DEPACKETIZER_2    (0x209000/4)
#define SAREK_4175_RX_DEPACKETIZER_3    (0x20a000/4)
#define SAREK_4175_RX_DEPACKETIZER_4    (0x20b000/4)
#define SAREK_3190_RX_DEPACKETIZER_1    (0x20c000/4)
#define SAREK_3190_RX_DEPACKETIZER_2    (0x20d000/4)
#define SAREK_3190_RX_DEPACKETIZER_3    (0x20e000/4)
#define SAREK_3190_RX_DEPACKETIZER_4    (0x20f000/4)

#define SAREK_2110_VIDEO_FRAMER_0       (0x210000/4)
#define SAREK_2110_AUDIO_FRAMER_0       (0x212000/4)
#define SAREK_2110_VIDEO_FRAMER_1       (0x213000/4)
#define SAREK_2110_AUDIO_FRAMER_1       (0x214000/4)
#define SAREK_2110_TX_ARBITRATOR        (0x215000/4)
#define SAREK_2110_DECAPSULATOR_1_TOP   (0x211000/4)
#define SAREK_2110_AUDIO_STREAMSELECT   (0x230000/4)

/////////////////////////////////////////////////////////////////////
//
// 4175 Packeteizer
//
/////////////////////////////////////////////////////////////////////

#define kReg4175_pkt_ctrl                       (0x0000/4)
#define kReg4175_pkt_width                      (0x0010/4)
#define kReg4175_pkt_height                     (0x0018/4)
#define kReg4175_pkt_vid_fmt                    (0x0020/4)
#define kReg4175_pkt_pkts_per_line              (0x0028/4)
#define kReg4175_pkt_payload_len                (0x0030/4)
#define kReg4175_pkt_payload_len_last           (0x0038/4)
#define kReg4175_pkt_ssrc                       (0x0040/4)
#define kReg4175_pkt_payload_type               (0x0048/4)
#define kReg4175_pkt_bpc_reg                    (0x0050/4)
#define kReg4175_pkt_chan_num                   (0x0058/4)
#define kReg4175_pkt_tx_pkt_cnt                 (0x0060/4)
#define kReg4175_pkt_tx_pkt_cnt_valid           (0x0064/4)
#define kReg4175_pkt_pix_per_pkt                (0x0068/4)
#define kReg4175_pkt_stat_reset                 (0x0070/4)
#define kReg4175_pkt_interlace_ctrl             (0x0078/4)

/////////////////////////////////////////////////////////////////////
//
// 4175 Depacketizer
//
/////////////////////////////////////////////////////////////////////

#define kReg4175_depkt_control                  (0x0000/4)
#define kReg4175_depkt_width_o                  (0x0010/4)
#define kReg4175_depkt_height_o                 (0x0018/4)
#define kReg4175_depkt_vid_fmt_o                (0x0020/4)
#define kReg4175_depkt_pkts_per_line_o          (0x0028/4)
#define kReg4175_depkt_payload_len_o            (0x0030/4)
#define kReg4175_depkt_payload_len_last_o       (0x0038/4)
#define kReg4175_depkt_bpc_reg_o                (0x0040/4)
#define kReg4175_depkt_rx_pkt_cnt_o             (0x0048/4)
#define kReg4175_depkt_rx_pkt_cnt_valid_o       (0x0050/4)
#define kReg4175_depkt_stat_reset_o             (0x0054/4)
#define kReg4175_depkt_rx_field_cnt             (0x0058/4)
#define kReg4175_depkt_rx_pkt_cnt               (0x005C/4)
#define kReg4175_depkt_rx_byte_cnt              (0x0060/4)
#define kReg4175_depkt_bytes_per_line           (0x0064/4)
#define kReg4175_depkt_rows_per_field           (0x0068/4)
#define kReg4175_depkt_version_id               (0x006C/4)
#define kReg4175_depkt_sequence_err             (0x007C/4)

/////////////////////////////////////////////////////////////////////
//
// 3190 Packeteizer
//
/////////////////////////////////////////////////////////////////////

#define kReg3190_pkt_ctrl                       (0x0000/4)
#define kReg3190_pkt_num_samples                (0x0010/4)
#define kReg3190_pkt_num_audio_channels         (0x0018/4)
#define kReg3190_pkt_payload_len                (0x0020/4)
#define kReg3190_pkt_chan_num                   (0x0028/4)
#define kReg3190_pkt_payload_type               (0x0030/4)
#define kReg3190_pkt_ssrc                       (0x0038/4)
#define kReg3190_pkt_tx_pkt_cnt                 (0x0040/4)


/////////////////////////////////////////////////////////////////////
//
// 3190 Depacketizer
//
/////////////////////////////////////////////////////////////////////

#define kReg3190_depkt_enable                   0
#define kReg3190_depkt_config                   1
#define kReg3190_depkt_rx_pkt_count             2

/////////////////////////////////////////////////////////////////////
//
// Framer
//
/////////////////////////////////////////////////////////////////////

#define kRegFramer_control              (0x0000/4)
#define kRegFramer_status               (0x0004/4)
#define kRegFramer_channel_access       (0x0008/4)
#define kRegFramer_sys_config           (0x000c/4)
#define kRegFramer_version              (0x0010/4)
#define kRegFramer_src_mac_lo           (0x0014/4)
#define kRegFramer_src_mac_hi           (0x0018/4)
#define kRegFramer_peak_buf_level       (0x001c/4)
#define kRegFramer_rx_pkt_cnt           (0x0020/4)
#define kRegFramer_drop_pkt_cnt         (0x0024/4)
#define kRegFramer_stat_reset           (0x0030/4)

// channel
#define kRegFramer_chan_ctrl            (0x0080/4)
#define kRegFramer_dest_mac_lo          (0x0084/4)
#define kRegFramer_dest_mac_hi          (0x0088/4)
#define kRegFramer_vlan_tag_info        (0x008c/4)
#define kRegFramer_ip_hdr_media         (0x0090/4)
#define kRegFramer_ip_hdr_fec           (0x0094/4)
#define kRegFramer_src_ip               (0x0098/4)
#define kRegFramer_dst_ip               (0x00a8/4)
#define kRegFramer_udp_src_port         (0x00b8/4)
#define kRegFramer_udp_dst_port         (0x00bc/4)
#define kRegFramer_tk_pkt_cnt           (0x00c0/4)
#define kRegFramer_chan_stat_reset      (0x00c4/4)

/////////////////////////////////////////////////////////////////////
//
// Decapsulator
//
/////////////////////////////////////////////////////////////////////

#define kRegDecap_chan_enable            0
#define kRegDecap_match_reserved         1
#define kRegDecap_match_src_ip           2
#define kRegDecap_match_dst_ip           3
#define kRegDecap_match_udp_src_port     4
#define kRegDecap_match_udp_dst_port     5
#define kRegDecap_match_payload          6
#define kRegDecap_match_ssrc             7
#define kRegDecap_match_sel              8
#define kRegDecap_unused                 9
#define kRegDecap_rx_payload            10
#define kRegDecap_rx_ssrc               11
#define kRegDecap_rx_pkt_cnt            12
#define kRegDecap_reordered_pkt_cnt     13
#define kRegDecap_unused2               14
#define kRegDecap_descriptiom           15


#endif // REGISTERS_2110_H
