/**
	@file		ntv2_2022_registers.h
	@brief		Defines the Sarek board's registers.
	@copyright	(C) 2014-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef REGISTERS_4175_H
#define REGISTERS_4175_H

#include "ntv2registersmb.h"


/////////////////////////////////////////////////////////////////////
//
// 4175 Packeteizer
//
/////////////////////////////////////////////////////////////////////

#define kReg4175_pkt_ctrl                       (0x0000/4)
#define kReg4175_pkt_width                      (0x0010/4)
#define kReg4175_pkt_height                     (0x0018/4)
#define kReg4175_pkt_Vid_fmt                    (0x0020/4)
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
#define kReg4175_pkt__interlace_ctrl            (0x0078/4)

/////////////////////////////////////////////////////////////////////
//
// 4175 Deppacketizer
//
/////////////////////////////////////////////////////////////////////

#define kReg4175_depkt_control                  (0x0000/4)
#define kReg4175_depkt_width                    (0x0010/4)
#define kReg4175_depkt_height                   (0x0018/4)
#define kReg4175_depkt_vid_fmt                  (0x0020/4)
#define kReg4175_depkt_pkts_per_line            (0x0028/4)
#define kReg4175_depkt_payload_len              (0x0030/4)
#define kReg4175_depkt_payload_len_last         (0x0038/4)
#define kReg4175_depkt_bpc_reg                  (0x0040/4)
#define kReg4175_depkt_rx_pkt_cnt               (0x0048/4)
#define kReg4175_depkt_rx_pkt_cnt_valid         (0x0050/4)
#define kReg4175_depkt_stat_reset               (0x0054/4)

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
#define kRegFramer_src_ip_lo            (0x0098/4)

#define kRegFramer_dst_ip_lo            (0x00a8/4)
#define kRegFramer_udp_src_port         (0x00b8/4)

#define kRegFramer_udp_dst_port         (0x00bc/4)
#define kRegFramer_tk_pkt_cnt           (0x00c0/4)
#define kRegFramer_chan_stat_reset      (0x00c4/4)

/////////////////////////////////////////////////////////////////////
//
// Decapsulator
//
/////////////////////////////////////////////////////////////////////

#define kRegDecap_control               (0x0000/4)
#define kRegDecap_status                (0x0004/4)
#define kRegDecap_channel_access        (0x0008/4)
#define kRegDecap_sys_config            (0x000c/4)
#define kRegDecap_version               (0x0010/4)
#define kRegDecap_pkt_lock_window       (0x0014/4)
#define kRegDecap_rx_pkt_cnt            (0x0018/4)
#define kRegDecap_mismatch_pkt_cnt      (0x001c/4)
#define kRegDecap_err_pkt_cnt           (0x0020/4)
#define kRegDecap_stat_reset            (0x0024/4)
#define kRegDecap_peak_buf_lvl          (0x0028/4)
#define kRegDecap_module_ctrl           (0x002c/4)
#define kRegDecap_chan_int_grp_ored     (0x0030/4)
#define kRegDecap_chan_int_grp_0        (0x0034/4)

//channel
#define kRegDecap_chan_ctrl                 (0x0080/4)
#define kRegDecap_chan_timeout              (0x0084/4)
#define kRegDecap_ip_hdr_param              (0x0088/4)
#define kRegDecap_match_vlan                (0x0090/4)
#define kRegDecap_match_dst_ip0             (0x0094/4)
#define kRegDecap_match_src_ip0             (0x00a4/4)
#define kRegDecap_match_udp_src_port        (0x00b4/4)
#define kRegDecap_match_udp_dst_port        (0x00b8/4)
#define kRegDecap_match_ssrc                (0x00bc/4)
#define kRegDecap_match_sel                 (0x00c0/4)
#define kRegDecap_vid_fmt                   (0x00c4/4)
#define kRegDecap_vid_src_fmt               (0x00c8/4)
#define kRegDecap_media_hdr                 (0x00cc/4)
#define kRegDecap_valid_media_pkt_cnt       (0x00d0/4)
#define kRegDecap_valid_media_fec_cnt       (0x00d4/4)
#define kRegDecap_reordered_pkt_cnt         (0x00d8/4)
#define kRegDecap_drop_pkt_cnt              (0x00dc/4)
#define kRegDecap_chan_stat_reset           (0x00e0/4)
#define kRegDecap_pkt_interval              (0x00e4/4)
#define kRegDecap_pkt_interval_network      (0x00e8/4)
#define kRegDecap_match_payload_ip_type     (0x00ec/4)
#define kRegDecap_int_status                (0x00f0/4)
#define kRegDecap_int_mask                  (0x00f4/4)
#define kRegDecap_int_clear                 (0x00f8/4)


#endif // REGISTERS_4175_H
