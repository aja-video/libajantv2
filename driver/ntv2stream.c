/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//========================================================================
//
//  ntv2stream.c
//  cd /home/davids/projects/gallifrey/ajadriver/linux; make -k
//==========================================================================

#include "ntv2system.h"
#include "ntv2stream.h"

/* debug messages */
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002
#define NTV2_DEBUG_STREAM_STATE		    0x00000004
#define NTV2_DEBUG_STREAM_CONFIG		0x00000008

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	((ntv2_active_mask & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_STREAM_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_STREAM_ERROR(string, ...)		    NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_STREAM_STATE(string, ...)		    NTV2_MSG_PRINT(NTV2_DEBUG_STREAM_STATE, string, __VA_ARGS__)
#define NTV2_MSG_STREAM_CONFIG(string, ...)		    NTV2_MSG_PRINT(NTV2_DEBUG_STREAM_CONFIG, string, __VA_ARGS__)

static uint32_t ntv2_debug_mask = 0xffffffff;
static uint32_t ntv2_user_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;
static uint32_t ntv2_active_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;

static uint32_t queue_next(uint32_t index)
{
    return (index + 1) % NTV2_STREAM_NUM_BUFFERS;
}

static uint32_t queue_prev(uint32_t index)
{
    return (index + NTV2_STREAM_NUM_BUFFERS - 1) % NTV2_STREAM_NUM_BUFFERS;
}

static bool queue_empty(uint32_t head_index, uint32_t tail_index)
{
    return head_index == tail_index;
}

static bool queue_full(uint32_t head_index, uint32_t tail_index)
{
    return head_index == queue_next(tail_index);
}

static void channel_state(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    // initialize status
    if (pChannel == NULL)
        return;

    // initialize status
    pChannel->mQueueDepth = 0;
    pChannel->mBufferCookie = 0;
    pChannel->mStartTime = 0;
    pChannel->mStopTime = 0;
    pChannel->mBufferCount = 0;
    pChannel->mRepeatCount = 0;

    if (ntv2_str != NULL)
    {
        // get stream state
        if (ntv2_str->stream_state == ntv2_stream_state_disabled)
        {
            pChannel->mStreamState = NTV2_STREAM_CHANNEL_STATE_DISABLED;
        }
        else if (ntv2_str->stream_state == ntv2_stream_state_initialized)
        {
            pChannel->mStreamState = NTV2_STREAM_CHANNEL_STATE_IDLE;
        }
        else if (ntv2_str->stream_state == ntv2_stream_state_idle)
        {
            pChannel->mStreamState = NTV2_STREAM_CHANNEL_STATE_IDLE;
        }
        else if (ntv2_str->stream_state == ntv2_stream_state_active)
        {
            pChannel->mStreamState = NTV2_STREAM_CHANNEL_STATE_ACTIVE;
        }
        else
        {
            pChannel->mStreamState = NTV2_STREAM_CHANNEL_STATE_ERROR;
        }

        // get status
        pChannel->mQueueDepth = 0;
        pChannel->mBufferCookie = 0;
        pChannel->mStartTime = 0;
        pChannel->mStopTime = 0;
        pChannel->mBufferCount = 0;
        pChannel->mRepeatCount = 0;
    }
}

static void buffer_state(struct ntv2_stream_buffer *ntv2_buf, NTV2StreamBuffer* pBuffer)
{
    // initialize status
    if (pBuffer == NULL)
        return;

    // initialize buffer status

    // get buffer status
    if (ntv2_buf != NULL)
    {
    }
}

struct ntv2_stream *ntv2_stream_open(Ntv2SystemContext* sys_con,
                                     const char *name, int index)
{
	struct ntv2_stream *ntv2_str = NULL;
    int i;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_str = (struct ntv2_stream *)ntv2MemoryAlloc(sizeof(struct ntv2_stream));
	if (ntv2_str == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_stream instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_str, 0, sizeof(struct ntv2_stream));

	ntv2_str->index = index;
#if defined(MSWindows)
	sprintf(ntv2_str->name, "%s%d", name, index);
#else
	snprintf(ntv2_str->name, NTV2_STREAM_STRING_SIZE, "%s%d", name, index);
#endif
	ntv2_str->system_context = sys_con;

	ntv2SemaphoreOpen(&ntv2_str->state_sema, sys_con, 1);

    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventOpen(&ntv2_str->wait_events[i], sys_con);
        ntv2_str->wait_inuse[i] = false;
    }

	NTV2_MSG_STREAM_INFO("%s: open ntv2_stream\n", ntv2_str->name);

	return ntv2_str;
}

void ntv2_stream_close(struct ntv2_stream *ntv2_str)
{
    int i;
    
	if (ntv2_str == NULL) 
		return;

	NTV2_MSG_STREAM_INFO("%s: close ntv2_stream\n", ntv2_str->name);

	ntv2_stream_disable(ntv2_str);

    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventClose(&ntv2_str->wait_events[i]);
    }

	ntv2SemaphoreClose(&ntv2_str->state_sema);

	memset(ntv2_str, 0, sizeof(struct ntv2_stream));
	ntv2MemoryFree(ntv2_str, sizeof(struct ntv2_stream));
}

Ntv2Status ntv2_stream_configure(struct ntv2_stream *ntv2_str,
                                 struct ntv2_stream_ops *stream_ops,
                                 bool to_host)
{
	if (ntv2_str == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_STREAM_INFO("%s: configure stream engine\n", ntv2_str->name);

    ntv2_str->stream_ops = *stream_ops;
    ntv2_str->to_host = to_host;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_enable(struct ntv2_stream *ntv2_str)
{
	if (ntv2_str == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_STREAM_STATE("%s: enable stream engine\n", ntv2_str->name);

    ntv2_str->enabled = true;
    ntv2_stream_channel_initialize(ntv2_str, NULL);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_disable(struct ntv2_stream *ntv2_str)
{
	if (ntv2_str == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_STREAM_STATE("%s: disable stream engine\n", ntv2_str->name);

    ntv2_str->enabled = false;
    ntv2_stream_channel_initialize(ntv2_str, NULL);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_initialize(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    Ntv2Status status;
    int i;

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        if (pChannel != NULL)
            pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    // check state
    if (ntv2_str->stream_state == ntv2_stream_state_disabled)
    {
        // user mode cannot initialize when disabled
        if (pChannel != NULL)
        {
            channel_state(ntv2_str, pChannel);
            pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
            ntv2SemaphoreUp(&ntv2_str->state_sema);
            return NTV2_STATUS_SUCCESS;
        }
    }

    // initialize stream engine
    status = (ntv2_str->stream_ops.stream_initialize)(ntv2_str);
    if (status != NTV2_STATUS_SUCCESS)
    {
        if (pChannel != NULL)
        {
            channel_state(ntv2_str, pChannel);
            pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
        }
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return status;
    }

    // release all waiting clients
    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventSignal(&ntv2_str->wait_events[i]);
    }

    // initialize buffer queue
    ntv2_str->head_index = 0;
    ntv2_str->tail_index = 0;
    ntv2_str->build_index = 0;
    ntv2_str->active_index = 0;

    // report status
    if (pChannel != NULL)
    {
        channel_state(ntv2_str, pChannel);
        if (ntv2_str->stream_state == ntv2_stream_state_error)
        {
            pChannel->mStatus = NTV2_STREAM_STATUS_FAIL;
        }
        else
        {
            pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
        }
    }
   
    // get state
    channel_state(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_start(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    Ntv2Status status;

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    // check stream state
    if ((ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        channel_state(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_SUCCESS;
    }

    // start stream engine
    if (ntv2_str->stream_state != ntv2_stream_state_active)
    {
        status = (ntv2_str->stream_ops.stream_start)(ntv2_str);
        if (status != NTV2_STATUS_SUCCESS)
        {
            channel_state(ntv2_str, pChannel);
            pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
            ntv2SemaphoreUp(&ntv2_str->state_sema);
            return status;
        }
    }

    // get state
    channel_state(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_stop(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    Ntv2Status status;

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    // check stream state
    if ((ntv2_str->stream_state == ntv2_stream_state_initialized) ||
        (ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        channel_state(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_SUCCESS;
    }

    // stop stream engine
    if (ntv2_str->stream_state != ntv2_stream_state_idle)
    {
        status = (ntv2_str->stream_ops.stream_stop)(ntv2_str);
        if (status != NTV2_STATUS_SUCCESS)
        {
            pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
            ntv2SemaphoreUp(&ntv2_str->state_sema);
            return status;
        }
    }

    // get state
    channel_state(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_flush(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    Ntv2Status status;
    uint32_t i;
    
    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    // check stream state
    if (ntv2_str->stream_state != ntv2_stream_state_idle)
    {
        channel_state(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_INVALID;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_FAIL;
    }

    // cleanup the queue
    for (i = ntv2_str->head_index; i != ntv2_str->tail_index; i = queue_next(i))
    {
        // do not cleanup the active transfer
        if (i != ntv2_str->active_index)
        {
            // release buffer
            status = (ntv2_str->stream_ops.buffer_release)(&ntv2_str->stream_buffers[i]);
            if (status != NTV2_STATUS_SUCCESS)
            {
            }            
        }
    }

    // reset the indicies
    ntv2_str->head_index = ntv2_str->active_index;
    ntv2_str->tail_index = queue_next(ntv2_str->active_index);
    ntv2_str->build_index = ntv2_str->tail_index;
    
    // program the transfer engine
    status = (ntv2_str->stream_ops.stream_program)(ntv2_str);
    if (status != NTV2_STATUS_SUCCESS)
    {
        channel_state(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return status;
    }

    // get state
    channel_state(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_status(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    // get state
    channel_state(ntv2_str, pChannel);  
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_wait(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    int event_index;
    
    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    for (event_index = 0; event_index < NTV2_STREAM_WAIT_CLIENTS; event_index++)
    {
        if (!ntv2_str->wait_inuse[event_index])
        {
            ntv2_str->wait_inuse[event_index] = true;
            break;
        }
    }
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    
    // wait for stream engine
    if (event_index != NTV2_STREAM_WAIT_CLIENTS)
    {
        ntv2EventClear(&ntv2_str->wait_events[event_index]);
        if (!ntv2EventWaitForSignal(&ntv2_str->wait_events[event_index], NTV2_STREAM_ACTION_TIMEOUT, true))
        {
            pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        }
        ntv2_str->wait_inuse[event_index] = false;
    }
    else
    {
        channel_state(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_SUCCESS;
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    // get state
    channel_state(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_buffer_add(struct ntv2_stream *ntv2_str, NTV2StreamBuffer* pBuffer)
{
    struct ntv2_stream_buffer* str_buf = NULL;
    Ntv2Status status;

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    // check stream state
    if ((ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        buffer_state(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_SUCCESS;
    }

    // check for queue full
    if (queue_full(ntv2_str->head_index, ntv2_str->tail_index))
    {
        buffer_state(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_SUCCESS;
    }
    
    // add the buffer to the queue
    str_buf = &ntv2_str->stream_buffers[ntv2_str->tail_index];
    memset(str_buf, 0, sizeof(struct ntv2_stream_buffer));
    str_buf->user_buffer = *pBuffer;

    // prepare buffer
    status = (ntv2_str->stream_ops.buffer_prepare)(str_buf);
    if (status != NTV2_STATUS_SUCCESS)
    {
        buffer_state(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_INVALID;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return status;
    }

    // update index
    ntv2_str->tail_index = queue_next(ntv2_str->tail_index);

    // program the transfer engine
    status = (ntv2_str->stream_ops.stream_program)(ntv2_str);
    if (status != NTV2_STATUS_SUCCESS)
    {
        buffer_state(str_buf, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return status;
    }

    // get state
    buffer_state(str_buf, pBuffer);
    pBuffer->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_buffer_status(struct ntv2_stream *ntv2_str, NTV2StreamBuffer* pBuffer)
{
    uint32_t i;
    
    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        return NTV2_STATUS_FAIL;
    }

    // find the buffer
    for (i = ntv2_str->head_index; i != ntv2_str->tail_index; i = queue_next(i))
    {
        if (pBuffer->mBufferCookie == ntv2_str->stream_buffers[i].user_buffer.mBufferCookie)
            break;
    }
    if (i == ntv2_str->tail_index)
    {
        // get state
        buffer_state(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_INVALID;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_SUCCESS;
    }
    
    // get state
    buffer_state(&ntv2_str->stream_buffers[i], pBuffer);
    pBuffer->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}




