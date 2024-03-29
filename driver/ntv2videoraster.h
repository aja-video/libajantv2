/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2videoraster.h
// Purpose:	 frame raster monitor
//
///////////////////////////////////////////////////////////////

#ifndef NTV2VIDEORASTER_HEADER
#define NTV2VIDEORASTER_HEADER

#include "ntv2system.h"

#define NTV2_VIDEORASTER_STRING_SIZE	80
#define NTV2_VIDEORASTER_MAX_WIDGETS    8

struct ntv2_videoraster {
	int					index;
	char				name[NTV2_VIDEORASTER_STRING_SIZE];
	Ntv2SystemContext* 	system_context;
	Ntv2InterruptLock   state_lock;
	bool				monitor_enable;

    uint32_t            global_control[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            global_control2[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            global_control3[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            channel_control[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            output_frame[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            input_frame[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            master_index[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            frame_size[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            frame_length[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            frame_pitch[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            channel_mode[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            videoraster_control[NTV2_VIDEORASTER_MAX_WIDGETS];
    bool                negative_pitch[NTV2_VIDEORASTER_MAX_WIDGETS];
    bool                mode_change[NTV2_VIDEORASTER_MAX_WIDGETS];

    uint32_t            widget_base;
    uint32_t            widget_size;
    uint32_t            num_widgets;
};

struct ntv2_videoraster *ntv2_videoraster_open(Ntv2SystemContext* sys_con,
                                               const char *name, int index);
void ntv2_videoraster_close(struct ntv2_videoraster *ntv2_raster);

Ntv2Status ntv2_videoraster_configure(struct ntv2_videoraster *ntv2_raster, uint32_t widget_base, uint32_t widget_size, uint32_t num_widgets);

Ntv2Status ntv2_videoraster_enable(struct ntv2_videoraster *ntv2_raster);
Ntv2Status ntv2_videoraster_disable(struct ntv2_videoraster *ntv2_raster);

Ntv2Status ntv2_videoraster_update_global(struct ntv2_videoraster *ntv2_raster, uint32_t reg, uint32_t value);
Ntv2Status ntv2_videoraster_update_channel(struct ntv2_videoraster *ntv2_raster, uint32_t index);
Ntv2Status ntv2_videoraster_update_frame(struct ntv2_videoraster *ntv2_raster, uint32_t index, bool input, uint32_t frame_number);

#endif
