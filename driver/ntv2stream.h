/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//========================================================================
//
//  ntv2stream.h
//
//==========================================================================

#ifndef NTV2STREAM_H
#define NTV2STREAM_H

#include "ntv2system.h"
#include "ntv2publicinterface.h"

#define NTV2_STREAM_STRING_SIZE     80
#define NTV2_STREAM_NUM_BUFFERS     256
#define NTV2_STREAM_ACTION_TIMEOUT  100000
#define NTV2_STREAM_WAIT_CLIENTS    10

struct ntv2_stream;
struct ntv2_stream_buffer;

enum ntv2_stream_state {
    ntv2_stream_state_unknown,
    ntv2_stream_state_disabled,
    ntv2_stream_state_initialized,
    ntv2_stream_state_idle,
    ntv2_stream_state_active,
    ntv2_stream_state_error
};

struct ntv2_stream_ops {
    Ntv2Status (*stream_initialize)(struct ntv2_stream *stream);
    Ntv2Status (*stream_start)(struct ntv2_stream *stream);
    Ntv2Status (*stream_stop)(struct ntv2_stream *stream);
    Ntv2Status (*stream_program)(struct ntv2_stream *stream);
    Ntv2Status (*buffer_prepare)(struct ntv2_stream_buffer* buffer);
    Ntv2Status (*buffer_release)(struct ntv2_stream_buffer* buffer);
};

struct ntv2_stream_buffer {
    NTV2StreamBuffer    user_buffer;
    void*               kernel_buffer;
    bool                prepared;
    bool                programmed;
    bool                linked;
    bool                released;
};

struct ntv2_stream {
	int					index;
	char				name[NTV2_STREAM_STRING_SIZE];
	Ntv2SystemContext* 	system_context;
    void*               kernel_stream;
    Ntv2Semaphore       state_sema;
    int                 engineIndex;
    bool                to_host;
    bool                enabled;

    enum ntv2_stream_state      stream_state;
    enum ntv2_stream_state      engine_state;
    struct ntv2_stream_ops      stream_ops;
    struct ntv2_stream_buffer   stream_buffers[NTV2_STREAM_NUM_BUFFERS];
    Ntv2Event                   wait_events[NTV2_STREAM_WAIT_CLIENTS];
    bool                        wait_inuse[NTV2_STREAM_WAIT_CLIENTS];
    uint32_t                    head_index;     // buffer queue head
    uint32_t                    tail_index;     // buffer queue tail
    uint32_t                    build_index;    // next buffer for programming
    uint32_t                    active_index;   // currently active buffer
};


struct ntv2_stream *ntv2_stream_open(Ntv2SystemContext* sys_con,
                                     const char *name, int index);
void ntv2_stream_close(struct ntv2_stream *ntv2_str);

Ntv2Status ntv2_stream_configure(struct ntv2_stream *ntv2_str,
                                 struct ntv2_stream_ops *stream_ops,
                                 bool to_host);

Ntv2Status ntv2_stream_enable(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_disable(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_interrupt(struct ntv2_stream *ntv2_str, uint64_t stream_bytes);

Ntv2Status ntv2_stream_channel_initialize(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_start(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_stop(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_flush(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_status(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_wait(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);

Ntv2Status ntv2_stream_buffer_add(struct ntv2_stream *ntv2_str, NTV2StreamBuffer* pBuffer);
Ntv2Status ntv2_stream_buffer_status(struct ntv2_stream *ntv2_str, NTV2StreamBuffer* pBuffer);

#endif
