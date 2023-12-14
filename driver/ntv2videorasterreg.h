/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2videorasterreg.h
// Purpose:  frame raster registers
//
///////////////////////////////////////////////////////////////

#ifndef NTV2_VIDEORASTERREG_H
#define NTV2_VIDEORASTERREG_H

#include "ntv2commonreg.h"
#include "ntv2virtualregisters.h"
#include "ntv2enums.h"

/* global control register map (csi number) */
NTV2_REG(ntv2_reg_global_control,                           0, 377, 378, 379, 380, 381, 382, 383);
   NTV2_FLD(ntv2_fld_global_control_frame_rate,                 3, 0);
   NTV2_FLD(ntv2_fld_global_control_geometry,                   4, 3);
   NTV2_FLD(ntv2_fld_global_control_standard,                   3, 7);
   NTV2_FLD(ntv2_fld_global_control_ref_source,                 3, 10);
   NTV2_FLD(ntv2_fld_global_control_smpte_372_enable,           1, 15);
   NTV2_FLD(ntv2_fld_global_control_reg_sync,                   2, 20);
   NTV2_FLD(ntv2_fld_global_control_frame_rate_high,            1, 22);
   NTV2_FLD(ntv2_fld_global_control_quad_tsi_enable,            1, 24);

NTV2_REG(ntv2_reg_global_control2,                          267);
   NTV2_FLD(ntv2_fld_global_control_ref_source2,                1, 0);
   NTV2_FLD(ntv2_fld_global_control_quad_mode,                  1, 3);
   NTV2_FLD(ntv2_fld_global_control_quad_mode2,                 1, 12);
   NTV2_FLD(ntv2_fld_global_control_smpte_372_enable4,          1, 13);
   NTV2_FLD(ntv2_fld_global_control_smpte_372_enable6,          1, 14);
   NTV2_FLD(ntv2_fld_global_control_smpte_372_enable8,          1, 15);
   NTV2_FLD(ntv2_fld_global_control_independent_mode,           1, 16);
   NTV2_FLD(ntv2_fld_global_control_425_fb12,                   1, 20);
   NTV2_FLD(ntv2_fld_global_control_425_fb34,                   1, 21);
   NTV2_FLD(ntv2_fld_global_control_425_fb56,                   1, 22);
   NTV2_FLD(ntv2_fld_global_control_425_fb78,                   1, 23);

NTV2_REG(ntv2_reg_global_control3,                          108);
   NTV2_FLD(ntv2_fld_global_control_quad_quad_mode,             1, 2);
   NTV2_FLD(ntv2_fld_global_control_quad_quad_mode2,            1, 3);
   NTV2_FLD(ntv2_fld_global_control_quad_quad_squares_mode,     1, 4);
   NTV2_FLD(ntv2_fld_global_control_frame_pulse_enable,         1, 6);
   NTV2_FLD(ntv2_fld_global_control_frame_pulse_ref_select,     4, 8);

/* channel control register map */
NTV2_REG(ntv2_reg_channel_control,                          1, 5, 257, 260, 384, 388, 392, 396);
   NTV2_FLD(ntv2_fld_channel_control_capture_enable,            1, 0);
   NTV2_FLD(ntv2_fld_channel_control_pixel_format,              4, 1);
   NTV2_FLD(ntv2_fld_channel_control_alpha_input2,              1, 5);
   NTV2_FLD(ntv2_fld_channel_control_pixel_format_high,         1, 6);
   NTV2_FLD(ntv2_fld_channel_control_channel_disable,           1, 7);
   NTV2_FLD(ntv2_fld_channel_control_write_back,                1, 8);
   NTV2_FLD(ntv2_fld_channel_control_frame_orientation,         1, 10);
   NTV2_FLD(ntv2_fld_channel_control_quarter_size_mode,         1, 11);
   NTV2_FLD(ntv2_fld_channel_control_frame_buffer_mode,         1, 12);
   NTV2_FLD(ntv2_fld_channel_control_down_convert_input,        1, 14);
   NTV2_FLD(ntv2_fld_channel_control_video_input_select,        1, 15);
   NTV2_FLD(ntv2_fld_channel_control_dither_8bit_input,         1, 16);
   NTV2_FLD(ntv2_fld_channel_control_quality,                   1, 17);
   NTV2_FLD(ntv2_fld_channel_control_encode_psf,                1, 18);
   NTV2_FLD(ntv2_fld_channel_control_frame_size,                2, 20);
   NTV2_FLD(ntv2_fld_channel_control_compressed,                1, 22);
   NTV2_FLD(ntv2_fld_channel_control_rgb_8b10b_convert,         1, 23);
   NTV2_FLD(ntv2_fld_channel_control_vblank_rgb_range,          1, 24);
   NTV2_FLD(ntv2_fld_channel_control_quality2,                  2, 25);
   NTV2_FLD(ntv2_fld_channel_control_ch1_black_output,          1, 27);
   NTV2_FLD(ntv2_fld_channel_control_sony_sr_express,           1, 28);
   NTV2_FLD(ntv2_fld_channel_control_frame_size_software,       1, 29);
   NTV2_FLD(ntv2_fld_channel_control_vanc_shift,                1, 31);

/* channel input/output frame number */
NTV2_REG(ntv2_reg_channel_output_frame,                     3, 7, 258, 261, 385, 389, 393, 397);
NTV2_REG(ntv2_reg_channel_input_frame,                      4, 8, 259, 262, 386, 390, 394, 398);

/* frame raster widget hardware register map (widget offset) */
NTV2_CON(ntv2_reg_videoraster_linepitch,                    0);     /**< Frame buffer line length and pitch */
   NTV2_FLD(ntv2_fld_videoraster_linepitch_pitch,               16, 0);     /**< Frame buffer line pitch (bytes) */
   NTV2_FLD(ntv2_fld_videoraster_linepitch_length,              16, 16);    /**< Frame buffer line length (bytes) */

NTV2_CON(ntv2_reg_videoraster_roisize,                      1);     /**< ROI size */
   NTV2_FLD(ntv2_fld_videoraster_roisize_h,                     16,  0);    /**< ROI horizontal size */
   NTV2_FLD(ntv2_fld_videoraster_roisize_v,                     16, 16);    /**< ROI vertical size */

NTV2_CON(ntv2_reg_videoraster_roifield1startaddress,        2);     /**< Field 1 frame buffer start address */

NTV2_CON(ntv2_reg_videoraster_roifield2startaddress,        3);     /**< Field 2 frame buffer start address */

NTV2_CON(ntv2_reg_videoraster_roifield1offset,              4);     /**< ROI field 1 offset */
   NTV2_FLD(ntv2_fld_videoraster_roifield1offset_h,             16,  0);    /**< ROI field 1 horizontal offset */
   NTV2_FLD(ntv2_fld_videoraster_roifield1offset_v,             16, 16);    /**< ROI field 1 vertical offset */

NTV2_CON(ntv2_reg_videoraster_roifield2offset,              5);     /**< ROI field 2 offset */
   NTV2_FLD(ntv2_fld_videoraster_roifield2offset_h,             16,  0);    /**< ROI field 2 horizontal offset */
   NTV2_FLD(ntv2_fld_videoraster_roifield2offset_v,             16, 16);    /**< ROI field 2 vertical offset */
        
NTV2_CON(ntv2_reg_videoraster_videoh,                       6);     /**< Video horizontal pixels per line*/
   NTV2_FLD(ntv2_fld_videoraster_videoh_active,                 16,  0);    /**< Video active horizontal pixels per line */
   NTV2_FLD(ntv2_fld_videoraster_videoh_total,                  16, 16);    /**< Video total horizontal pixels per line */

NTV2_CON(ntv2_reg_videoraster_videofid,                     7);     /**< Video FID bit transition lines */
   NTV2_FLD(ntv2_fld_videoraster_videofid_high,                 16,  0);    /**< Video FID low to high line number */
   NTV2_FLD(ntv2_fld_videoraster_videofid_low,                  16, 16);    /**< Video FID high to low line number */

NTV2_CON(ntv2_reg_videoraster_field1active,                 8);     /**< Field 1 active lines */
   NTV2_FLD(ntv2_fld_videoraster_field1active_start,            16,  0);    /**< Video field 1 active start line number */
   NTV2_FLD(ntv2_fld_videoraster_field1active_end,              16,  16);   /**< Video field 1 active end line number */

NTV2_CON(ntv2_reg_videoraster_field2active,                 9);     /**< Field 2 active lines */
   NTV2_FLD(ntv2_fld_videoraster_field2active_start,            16,  0);    /**< Video field 2 active start line number */
   NTV2_FLD(ntv2_fld_videoraster_field2active_end,              16,  16);   /**< Video field 2 active end line number */

NTV2_CON(ntv2_reg_videoraster_control,                      10);    /**< Control register */
   NTV2_FLD(ntv2_fld_videoraster_control_mode,                  2,   0);    /**< Frame buffer io mode */
       NTV2_CON(ntv2_con_videoraster_mode_display,                  0x0);           /**< Frame buffer mode display */
       NTV2_CON(ntv2_con_videoraster_mode_capture,                  0x1);           /**< Frame buffer mode capture */
       NTV2_CON(ntv2_con_videoraster_mode_disable,                  0x2);           /**< Frame buffer mode disable */
   NTV2_FLD(ntv2_fld_videoraster_control_double,                1,   2);    /**< Pixel double enable */
   NTV2_FLD(ntv2_fld_videoraster_control_fillbit,               1,   3);    /**< Bit value for extra pixel bits in 10 or 12 bit data */
   NTV2_FLD(ntv2_fld_videoraster_control_dither,                1,   4);    /**< Dither 10 to 8 bit conversion (default is truncate/round) */
   NTV2_FLD(ntv2_fld_videoraster_control_8b10bconvert,          1,   5);    /**< Pixel 8bit / 10bit conversion control */
       NTV2_CON(ntv2_con_videoraster_8b10bconvert_round,            0x0);           /**< Capture 8b10b round */
       NTV2_CON(ntv2_con_videoraster_8b10bconvert_truncate,         0x1);           /**< Capture 8b10b truncate */
       NTV2_CON(ntv2_con_videoraster_8b10bconvert_msb,              0x0);           /**< Display 8b10b copy MSB to LSB */
       NTV2_CON(ntv2_con_videoraster_8b10bconvert_b00,              0x1);           /**< Display 8b10b LSB = 0b00 */
   NTV2_FLD(ntv2_fld_videoraster_control_progressive,           1,   6);    /**< Scan type : progressive(1) / interlaced(0) */
   NTV2_FLD(ntv2_fld_videoraster_control_twosampleinterleave,   1,   7);    /**< Two-pixel interleave mode */
       NTV2_CON(ntv2_con_videoraster_twosampleinterleave_disable,   0x0);       /**< Disable two-pixel interleave */
       NTV2_CON(ntv2_con_videoraster_twosampleinterleave_enable,    0x1);       /**< Enable two-pixel interleave */
   NTV2_FLD(ntv2_fld_videoraster_control_imageformat,           8,   8);    /**< image format */
   NTV2_FLD(ntv2_fld_videoraster_control_pixelclock,            3,   16);   /**< pixel clock select */
       NTV2_CON(ntv2_con_videoraster_pixelclock_2700,               0x0);           /**< Pixel clock 27 MHz */
       NTV2_CON(ntv2_con_videoraster_pixelclock_7418,               0x1);           /**< Pixel clock 74.1758 MHz */
       NTV2_CON(ntv2_con_videoraster_pixelclock_7425,               0x2);           /**< Pixel clock 74.25 MHz */
       NTV2_CON(ntv2_con_videoraster_pixelclock_14835,              0x3);           /**< Pixel clock 148.3516 MHz */
       NTV2_CON(ntv2_con_videoraster_pixelclock_14850,              0x4);           /**< Pixel clock 148.5 MHz */
   NTV2_FLD(ntv2_fld_videoraster_control_regsync,               2,   20);   /**< Register update synchronization */ 
       NTV2_CON(ntv2_con_videoraster_regsync_frame,                 0x0);           /**< Register update sync to frame */
       NTV2_CON(ntv2_con_videoraster_regsync_field,                 0x1);           /**< Register update sync to field */
       NTV2_CON(ntv2_con_videoraster_regsync_immediate,             0x2);           /**< Register update immediate */
       NTV2_CON(ntv2_con_videoraster_regsync_external,              0x3);           /**< Register update from syncgen */
   NTV2_FLD(ntv2_fld_videoraster_control_disable_auto_update,   1,   31);   /**< Disable automatic update */ 

NTV2_CON(ntv2_reg_videoraster_pixelskip,                    11);    /**< Play pixel skip */
   NTV2_FLD(ntv2_fld_videoraster_pixelskip_count,               16,  0);    /**< number of pixels to skip */    

NTV2_CON(ntv2_reg_videoraster_videofilla,                   12);    /**< Video fill Y/G  Cb/B */
   NTV2_FLD(ntv2_fld_videoraster_videofilla_blue,               16,  0);    /**< Blue / Cb fill */
   NTV2_FLD(ntv2_fld_videoraster_videofilla_green,              16,  16);   /**< Green / Y fill */

NTV2_CON(ntv2_reg_videoraster_videofillb,                   13);    /**< Video fill Cr/R Alpha */
   NTV2_FLD(ntv2_fld_videoraster_videofillb_red,                16,  0);    /**< Red / Cr fill */
   NTV2_FLD(ntv2_fld_videoraster_videofillb_alpha,              16,  16);   /**< Alpha fill */

NTV2_CON(ntv2_reg_videoraster_roifillalpha,                 14);    /**< Region of Interest Fill - Alpha */
   NTV2_FLD(ntv2_fld_videoraster_roifill_alpha,                 16,  0);    /**< Alpha Fill */

NTV2_CON(ntv2_reg_videoraster_status,                       15);    /**< Frame buffer status */
   NTV2_FLD(ntv2_fld_videoraster_status_fieldid,                1,   0);    /**< Current field ID */
       NTV2_CON(ntv2_con_videoraster_fieldid_field0,                0x0);           /**< Current field is field 0 (first in time) */
       NTV2_CON(ntv2_con_videoraster_fieldid_field1,                0x1);           /**< Current field is field 1 (second in time) */
   NTV2_FLD(ntv2_fld_videoraster_status_linecount,              11,  16);   /**< Current IO line */

NTV2_CON(ntv2_reg_videoraster_timingpreset,                 16);    /**< Output timing preset */
   NTV2_FLD(ntv2_fld_videoraster_timingpreset_offset,           24,  0);    /**< Timing offset in clocks */

NTV2_CON(ntv2_reg_videoraster_vtotal,                       17);    /**< Total Lines */

NTV2_CON(ntv2_reg_videoraster_smpteframepulse,              18);    /**< SMPTE Frame Pulse */
   NTV2_FLD(ntv2_fld_videoraster_smpteframepulse_pixelnumber,   16,  0);    /**< Pixel Number */
   NTV2_FLD(ntv2_fld_videoraster_smpteframepulse_linenumber,    16, 16);    /**< Line Number */

NTV2_CON(ntv2_reg_videoraster_oddlinestartaddress,            19);    /**< UHD odd line addres*/

//NTV2_CON(ntv2_reg_videoraster_offsetgreen,                  19);    /**< Green playback component offset (signed 12 bit) */
//NTV2_CON(ntv2_reg_videoraster_offsetblue,                   20);    /**< Blue playback component offset (signed 12 bit) */
//NTV2_CON(ntv2_reg_videoraster_offsetred,                    21);    /**< Red playback component offset (signed 12 bit) */
//NTV2_CON(ntv2_reg_videoraster_offsetalpha,                  22);    /**< Alpha playback component offset (signed 12 bit) */

#endif
