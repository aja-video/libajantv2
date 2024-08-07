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
    int (*buffer_queue)(struct ntv2_stream *stream, int index);
    int (*buffer_link)(struct ntv2_stream *stream, int from_index, int to_index);
    int (*buffer_complete)(struct ntv2_stream *stream, int index);
    int (*buffer_flush)(struct ntv2_stream *stream, int index);
    int (*buffer_release)(struct ntv2_stream *stream, int index);
};

struct ntv2_stream_buffer {
    NTV2StreamBuffer    user_buffer;        // user mode stream buffer data
    void*               dma_buffer;         // kernel mode buffer data
    bool                queued;             // buffer has been queued
    bool                linked;             // buffer has descriptors linked into chain
    bool                completed;          // buffer transfer complete
    bool                flushed;            // buffer flushed from queue
    bool                released;           // buffer released from queue
    bool                error;              // buffer transfer error
    uint32_t            ds_index;           // buffer descriptor index
    uint32_t            ds_count;           // buffer descriptor count
};

struct ntv2_stream {
	int					index;              // stream index
	char				name[NTV2_STREAM_STRING_SIZE];
	Ntv2SystemContext* 	system_context;     // system context for ops
    NTV2Channel         channel;            // stream channel
    Ntv2Semaphore       state_sema;         // stream action semaphore
    void*               owner;              // stream user mode owner
    void*               dma_engine;         // stream dma engine

    enum ntv2_stream_state      stream_state;       // stream desired state
    enum ntv2_stream_state      engine_state;       // stream actual state
    struct ntv2_stream_ops      stream_ops;         // stream kernel operations
    struct ntv2_stream_buffer   stream_buffers[NTV2_STREAM_NUM_BUFFERS];    // stream buffer array
    Ntv2Event                   wait_events[NTV2_STREAM_WAIT_CLIENTS];      // stream wait event array
    bool                        wait_inuse[NTV2_STREAM_WAIT_CLIENTS];       // stream in use array
    uint32_t                    head_index;         // buffer queue head (add here)
    uint32_t                    tail_index;         // buffer queue tail (remove here)
    uint32_t                    active_index;       // currently active buffer
    uint32_t                    next_index;         // next active buffer
    uint32_t                    init_advance;       // number of syncs to start stream
    uint32_t                    init_count;         // current number of syncs
    NTV2StreamChannel           user_channel;       // user mode stream channel data
};

#ifdef __cplusplus
extern "C"
{
#endif

	// stream management functions
struct ntv2_stream *ntv2_stream_open(Ntv2SystemContext* sys_con,
                                     const char *name, int index);
void ntv2_stream_close(struct ntv2_stream *ntv2_str);

Ntv2Status ntv2_stream_configure(struct ntv2_stream *ntv2_str,
                                 struct ntv2_stream_ops *stream_ops,
                                 void* dma_engine,
                                 uint32_t init_advance);

Ntv2Status ntv2_stream_enable(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_disable(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_interrupt(struct ntv2_stream *ntv2_str, uint64_t stream_bytes);

// stream io functions
Ntv2Status ntv2_stream_channel_initialize(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_release(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_start(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_stop(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_flush(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_status(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);
Ntv2Status ntv2_stream_channel_wait(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel);

Ntv2Status ntv2_stream_buffer_queue(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamBuffer* pBuffer);
Ntv2Status ntv2_stream_buffer_release(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamBuffer* pBuffer);
Ntv2Status ntv2_stream_buffer_status(struct ntv2_stream *ntv2_str, NTV2StreamBuffer* pBuffer);

// stream interrupt function
Ntv2Status ntv2_stream_channel_advance(struct ntv2_stream *ntv2_str);

#ifdef __cplusplus
}
#endif

#endif
