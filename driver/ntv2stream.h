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
    ntv2_stream_state_released,
    ntv2_stream_state_idle,
    ntv2_stream_state_active,
    ntv2_stream_state_error
};

#define NTV2_STREAM_OPS_FAIL    0
#define NTV2_STREAM_OPS_SUCCESS 1

struct ntv2_stream_ops {
    int (*stream_initialize)(struct ntv2_stream *stream);
    int (*stream_start)(struct ntv2_stream *stream);
    int (*stream_stop)(struct ntv2_stream *stream);
    int (*stream_release)(struct ntv2_stream *stream);
    int (*stream_advance)(struct ntv2_stream *stream);
    int (*buffer_prepare)(struct ntv2_stream *stream, int index);
    int (*buffer_link)(struct ntv2_stream *stream, int from_index, int to_index);
    int (*buffer_complete)(struct ntv2_stream *stream, int index);
    int (*buffer_flush)(struct ntv2_stream *stream, int index);
    int (*buffer_release)(struct ntv2_stream *stream, int index);
};

struct ntv2_stream_buffer {
    NTV2StreamBuffer    user_buffer;
    void*               dma_buffer;
    bool                prepared;
    bool                linked;
    bool                completed;
    bool                flushed;
    bool                released;
    uint32_t            ds_index;
    uint32_t            ds_count;
};

struct ntv2_stream {
	int					index;
	char				name[NTV2_STREAM_STRING_SIZE];
	Ntv2SystemContext* 	system_context;
    NTV2Channel         channel;
    Ntv2Semaphore       state_sema;
    bool                to_host;
    void*               owner;
    void*               dma_engine;

    enum ntv2_stream_state      stream_state;
    enum ntv2_stream_state      engine_state;
    struct ntv2_stream_ops      stream_ops;
    struct ntv2_stream_buffer   stream_buffers[NTV2_STREAM_NUM_BUFFERS];
    Ntv2Event                   wait_events[NTV2_STREAM_WAIT_CLIENTS];
    bool                        wait_inuse[NTV2_STREAM_WAIT_CLIENTS];
    uint32_t                    head_index;         // buffer queue head
    uint32_t                    tail_index;         // buffer queue tail
    uint32_t                    active_index;       // currently active buffer
    uint32_t                    next_index;         // next active buffer
    uint64_t                    queue_count;        // buffers queued
    uint64_t                    release_count;      // buffers released
};


struct ntv2_stream *ntv2_stream_open(Ntv2SystemContext* sys_con,
                                     const char *name, int index);
void ntv2_stream_close(struct ntv2_stream *ntv2_str);

Ntv2Status ntv2_stream_configure(struct ntv2_stream *ntv2_str,
                                 struct ntv2_stream_ops *stream_ops,
                                 void* dma_engine,
                                 bool to_host);

Ntv2Status ntv2_stream_enable(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_disable(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_interrupt(struct ntv2_stream *ntv2_str, uint64_t stream_bytes);

Ntv2Status ntv2_stream_channel_initialize(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_release(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_start(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_stop(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_flush(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_status(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_wait(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_advance(struct ntv2_stream *ntv2_str);

Ntv2Status ntv2_stream_buffer_queue(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamBuffer* pBuffer);
Ntv2Status ntv2_stream_buffer_release(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamBuffer* pBuffer);
Ntv2Status ntv2_stream_buffer_status(struct ntv2_stream *ntv2_str, NTV2StreamBuffer* pBuffer);

#endif
