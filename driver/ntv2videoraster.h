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
	Ntv2SpinLock		state_lock;

	Ntv2Thread 			monitor_task;
	bool				monitor_enable;
	Ntv2Event			monitor_event;

    uint32_t            global_control[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            global_control2[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            global_control3[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            channel_control[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            output_frame[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            input_frame[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            master_index[NTV2_VIDEORASTER_MAX_WIDGETS];
    uint32_t            frame_size[NTV2_VIDEORASTER_MAX_WIDGETS];
    bool                mode_change[NTV2_VIDEORASTER_MAX_WIDGETS];

    uint32_t            widget_base;
    uint32_t            widget_size;
    uint32_t            num_widgets;
};

#ifdef __cplusplus
extern "C"
{
#endif

struct ntv2_videoraster *ntv2_videoraster_open(Ntv2SystemContext* sys_con,
                                               const char *name, int index);
void ntv2_videoraster_close(struct ntv2_videoraster *ntv2_raster);

Ntv2Status ntv2_videoraster_configure(struct ntv2_videoraster *ntv2_raster, uint32_t widget_base, uint32_t widget_size, uint32_t num_widgets);

Ntv2Status ntv2_videoraster_enable(struct ntv2_videoraster *ntv2_raster);
Ntv2Status ntv2_videoraster_disable(struct ntv2_videoraster *ntv2_raster);

Ntv2Status ntv2_videoraster_update_input_frame(struct ntv2_videoraster *ntv2_raster, NTV2Channel channel, uint32_t frame_number);
Ntv2Status ntv2_videoraster_update_output_frame(struct ntv2_videoraster *ntv2_raster, NTV2Channel channel, uint32_t frame_number);

#ifdef __cplusplus
}
#endif

#endif
