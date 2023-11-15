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
#define NTV2_DEBUG_STREAM_ACTIVE		0x00000010

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
#define NTV2_MSG_STREAM_ACTIVE(string, ...)		    NTV2_MSG_PRINT(NTV2_DEBUG_STREAM_ACTIVE, string, __VA_ARGS__)

//static uint32_t ntv2_debug_mask = 0xffffffff;
//static uint32_t ntv2_user_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;
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
    // leave and emtpy slot
    return head_index == queue_next(queue_next(tail_index));
}

static void channel_status(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    // initialize status
    if (pChannel == NULL)
        return;

    // initialize status
    pChannel->mStreamState = NTV2_STREAM_CHANNEL_STATE_DISABLED;
    pChannel->mBufferCookie = 0;
    pChannel->mStartTime = 0;
    pChannel->mStopTime = 0;
    pChannel->mQueueCount = 0;
    pChannel->mReleaseCount = 0;
    pChannel->mActiveCount = 0;
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
            pChannel->mStreamState = NTV2_STREAM_CHANNEL_STATE_INITIALIZED;
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
        pChannel->mBufferCookie = 0;
        pChannel->mStartTime = 0;
        pChannel->mStopTime = 0;
        pChannel->mQueueCount = ntv2_str->queue_count;
        pChannel->mReleaseCount = ntv2_str->release_count;
        pChannel->mActiveCount = 0;
        pChannel->mRepeatCount = 0;
    }
}

static void buffer_status(struct ntv2_stream_buffer *ntv2_buf, NTV2StreamBuffer* pBuffer)
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
                                 void* dma_engine,
                                 bool to_host)
{
	if (ntv2_str == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_STREAM_INFO("%s: configure stream engine\n", ntv2_str->name);

    ntv2_str->stream_ops = *stream_ops;
    ntv2_str->dma_engine = dma_engine;
    ntv2_str->to_host = to_host;

    // set state
    ntv2_str->stream_state = ntv2_stream_state_disabled;
    ntv2_str->engine_state = ntv2_stream_state_disabled;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_enable(struct ntv2_stream *ntv2_str)
{
    NTV2StreamChannel channel;

	if (ntv2_str == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_STREAM_STATE("%s: enable stream engine\n", ntv2_str->name);

    ntv2_stream_channel_release(ntv2_str, NULL, &channel);

    // set state
    ntv2_str->stream_state = ntv2_stream_state_released;
    ntv2_str->engine_state = ntv2_stream_state_released;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_disable(struct ntv2_stream *ntv2_str)
{
    NTV2StreamChannel channel;

    if (ntv2_str == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_STREAM_STATE("%s: disable stream engine\n", ntv2_str->name);

    ntv2_stream_channel_release(ntv2_str, NULL, &channel);

    // set state
    ntv2_str->stream_state = ntv2_stream_state_disabled;
    ntv2_str->engine_state = ntv2_stream_state_disabled;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_initialize(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamChannel* pChannel)
{
    int status;
    uint32_t i;

    if ((ntv2_str == NULL) || (pOwner == NULL) || (pChannel == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: initialize no owner or channel\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: initialize timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // check state
    if (ntv2_str->stream_state == ntv2_stream_state_disabled)
    {
        // cannot initialize when disabled
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: initialize stream state disabled\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    if ((ntv2_str->owner != NULL) && (pOwner != ntv2_str->owner))
    {
        // not the current owner
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_OWNER;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: initialize stream not owner\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // initialize stream engine
    status = (ntv2_str->stream_ops.stream_initialize)(ntv2_str);
    if (status != NTV2_STREAM_OPS_SUCCESS)
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: initialize stream failed\n", ntv2_str->name);
        return status;
    }

    // assign the owner
    ntv2_str->owner = pOwner;

    // flush the queue
    for (i = 0; i < NTV2_STREAM_NUM_BUFFERS; i++)
    {
        status = (ntv2_str->stream_ops.buffer_flush)(ntv2_str, i);
        if (status != NTV2_STREAM_OPS_SUCCESS)
        {
            NTV2_MSG_STREAM_ERROR("%s: initialize buffer failed\n", ntv2_str->name);
        }            
    }

    // initialize counts
    ntv2_str->queue_count = 0;

    // release all waiting clients
    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventSignal(&ntv2_str->wait_events[i]);
    }

    // report status
    channel_status(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
   
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    NTV2_MSG_STREAM_STATE("%s: stream initialized\n", ntv2_str->name);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_release(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamChannel* pChannel)
{
    int status;
    int i;
    
    if ((ntv2_str == NULL) || (pChannel == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: release no channel\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: release timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // check owner
    if ((pOwner != NULL) && (pOwner != ntv2_str->owner))
    {
        // not the current owner
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_OWNER;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: release stream not owner\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // release stream engine
    status = (ntv2_str->stream_ops.stream_release)(ntv2_str);
    if (status != NTV2_STREAM_OPS_SUCCESS)
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: release stream failed\n", ntv2_str->name);
        return status;
    }

    // clear the queue
    for (i = 0; i < NTV2_STREAM_NUM_BUFFERS; i++)
    {
        // release buffer
        status = (ntv2_str->stream_ops.buffer_release)(ntv2_str, i);
        if (status != NTV2_STREAM_OPS_SUCCESS)
        {
            NTV2_MSG_STREAM_ERROR("%s: release buffer failed\n", ntv2_str->name);
        }            
    }

    // release all waiting clients
    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventSignal(&ntv2_str->wait_events[i]);
    }

    // initialize buffer queue
    ntv2_str->head_index = 0;
    ntv2_str->tail_index = 0;
    ntv2_str->active_index = 0;
    ntv2_str->next_index = 0;
    ntv2_str->queue_count = 0;
    ntv2_str->release_count = 0;

    // clear the owner
    ntv2_str->owner = NULL;

    // report status
    channel_status(ntv2_str, pChannel);  
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    NTV2_MSG_STREAM_STATE("%s: stream released\n", ntv2_str->name);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_start(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    int i;
    int next;
    int status;

    if ((ntv2_str == NULL) || (pChannel == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: start  no owner or channel\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: start timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // check if stream
    if ((ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_released) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: start stream state disabled\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // check for queue empty
    if (queue_empty(ntv2_str->head_index, ntv2_str->tail_index))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer queue empty\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // check active index linked
    if (ntv2_str->stream_buffers[ntv2_str->active_index].prepared &&
        !ntv2_str->stream_buffers[ntv2_str->active_index].linked)
    {
        status = (ntv2_str->stream_ops.buffer_link )(ntv2_str, ntv2_str->active_index, ntv2_str->active_index);
        if (status != NTV2_STREAM_OPS_SUCCESS)
        {
            channel_status(ntv2_str, pChannel);
            pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
            ntv2SemaphoreUp(&ntv2_str->state_sema);
            NTV2_MSG_STREAM_ERROR("%s: start stream active link failed\n", ntv2_str->name);
            return status;
        }
        ntv2_str->next_index = ntv2_str->active_index;

        // link to next buffer if present
        if (ntv2_str->active_index != ntv2_str->tail_index)
        {
            next = queue_next(ntv2_str->active_index);
            while (ntv2_str->stream_buffers[next].prepared)
            {
                if (!ntv2_str->stream_buffers[next].flushed)
                {
                    // advance hardware
                    status = (ntv2_str->stream_ops.buffer_link)(ntv2_str, ntv2_str->active_index, next);
                    if (status != NTV2_STREAM_OPS_SUCCESS)
                    {
                        channel_status(ntv2_str, pChannel);
                        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
                        ntv2SemaphoreUp(&ntv2_str->state_sema);
                        NTV2_MSG_STREAM_ERROR("%s: channel hardware advance failed\n", ntv2_str->name);
                        return status;
                    }
                    ntv2_str->next_index = next;
                    break;
                }
                if (next == ntv2_str->tail_index)
                {
                    break;
                }
                next = queue_next(next);
            }
        }
    }

    // start stream engine
    status = (ntv2_str->stream_ops.stream_start)(ntv2_str);
    if (status != NTV2_STREAM_OPS_SUCCESS)
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: start failed\n", ntv2_str->name);
        return status;
    }

    // release all waiting clients
    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventSignal(&ntv2_str->wait_events[i]);
    }

    channel_status(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    NTV2_MSG_STREAM_STATE("%s: stream started\n", ntv2_str->name);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_stop(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    int i;
    int status;

    if ((ntv2_str == NULL) || (pChannel == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: stop no channel\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: stop timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // check if stream 
    if ((ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_released) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: stop stream state disabled\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // check stream state
    if ((ntv2_str->stream_state != ntv2_stream_state_active) &&
        (ntv2_str->stream_state != ntv2_stream_state_idle))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: stop stream state not active or idle\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // stop stream engine
    status = (ntv2_str->stream_ops.stream_stop)(ntv2_str);
    if (status != NTV2_STREAM_OPS_SUCCESS)
    {
        ntv2_str->stream_state = ntv2_stream_state_error;
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: stop stream failed\n", ntv2_str->name);
        return status;
    }

    // release all waiting clients
    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventSignal(&ntv2_str->wait_events[i]);
    }

    channel_status(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    NTV2_MSG_STREAM_STATE("%s: stream stopped\n", ntv2_str->name);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_flush(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    uint32_t i;
    int status;
    
    if ((ntv2_str == NULL) || (pChannel == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: flush no channel\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: flush timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // check if stream
    if ((ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_released) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: flush stream bad state\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // check for queue empty
    if (!queue_empty(ntv2_str->head_index, ntv2_str->tail_index))
    {
        // try to flush queue from newest (tail) to oldest buffer (head)
        for (i = ntv2_str->tail_index; i != ntv2_str->head_index; i = queue_prev(i))
        {
            // flush buffer
            status = (ntv2_str->stream_ops.buffer_flush)(ntv2_str, queue_prev(i));
            if (status != NTV2_STREAM_OPS_SUCCESS)
            {
                // done when no more to flush
                break;
            }            
        }
    }

    // release all waiting clients
    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventSignal(&ntv2_str->wait_events[i]);
    }

    // get state
    channel_status(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    NTV2_MSG_STREAM_STATE("%s: stream flushed\n", ntv2_str->name);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_status(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    if ((ntv2_str == NULL) || (pChannel == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: status no channel\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: status timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // get state
    channel_status(ntv2_str, pChannel);  
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_wait(struct ntv2_stream *ntv2_str, NTV2StreamChannel* pChannel)
{
    int event_index;
    
    if ((ntv2_str == NULL) || (pChannel == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: wait no channel\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: wait timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // check if stream
    if ((ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_released) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: wait state disabled\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
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
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
        NTV2_MSG_STREAM_ERROR("%s: wait out of clients\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        channel_status(ntv2_str, pChannel);
        pChannel->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: wait timeout acquiring stream lock 2\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // get state
    channel_status(ntv2_str, pChannel);
    pChannel->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_channel_advance(struct ntv2_stream *ntv2_str)
{
    int i;
    int next;
    int status;

    if ((ntv2_str == NULL) ||
        (ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_released) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        return NTV2_STATUS_SUCCESS;
    }

    // complete current active
    if (ntv2_str->active_index != ntv2_str->next_index)
    {
        // complete buffer
        status = (ntv2_str->stream_ops.buffer_complete)(ntv2_str, ntv2_str->active_index);
        if (status != NTV2_STREAM_OPS_SUCCESS)
        {
            NTV2_MSG_STREAM_ERROR("%s: channel hardware advance failed\n", ntv2_str->name);
            return status;
        }            
    }

    // update active index
    ntv2_str->active_index = ntv2_str->next_index;

    // update next index
    if (ntv2_str->stream_state == ntv2_stream_state_active)
    {
        next = queue_next(ntv2_str->active_index);
        while (ntv2_str->stream_buffers[next].prepared &&
               !ntv2_str->stream_buffers[next].released)
        {
            if (!ntv2_str->stream_buffers[next].flushed)
            {
                // advance hardware
                status = (ntv2_str->stream_ops.buffer_link)(ntv2_str, ntv2_str->active_index, next);
                if (status != NTV2_STREAM_OPS_SUCCESS)
                {
                    NTV2_MSG_STREAM_ERROR("%s: channel advance link failed\n", ntv2_str->name);
                    return status;
                }
                ntv2_str->next_index = next;
                break;
            }
            next = queue_next(next);
        }
    }
    
    // release all waiting clients
    for (i = 0; i < NTV2_STREAM_WAIT_CLIENTS; i++)
    {
        ntv2EventSignal(&ntv2_str->wait_events[i]);
    }

    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_buffer_queue(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamBuffer* pBuffer)
{
    struct ntv2_stream_buffer* str_buf = NULL;
    int status;

    if ((ntv2_str == NULL) || (pOwner == NULL) || (pBuffer == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: buffer queue no owner or buffer\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: buffer add timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // check if stream
    if ((ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_released) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer add stream state disabled\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // check owner
    if (pOwner != ntv2_str->owner)
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_OWNER;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer queue not owner\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // check for queue full
    if (queue_full(ntv2_str->head_index, ntv2_str->tail_index))
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer queue full\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }
    
    // get next free buffer slot
    str_buf = &ntv2_str->stream_buffers[ntv2_str->tail_index];
    if (str_buf->prepared)
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer queue slot not released\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }
    str_buf->user_buffer = *pBuffer;

    // prepare buffer
    status = (ntv2_str->stream_ops.buffer_prepare)(ntv2_str, ntv2_str->tail_index);
    if (status != NTV2_STREAM_OPS_SUCCESS)
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_INVALID;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer add prepare failed\n", ntv2_str->name);
        return status;
    }

    // update queue index
    ntv2_str->tail_index = queue_next(ntv2_str->tail_index);
    ntv2_str->queue_count++;

    // get state
    buffer_status(str_buf, pBuffer);
    pBuffer->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    NTV2_MSG_STREAM_ACTIVE("%s: buffer added\n", ntv2_str->name);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_buffer_release(struct ntv2_stream *ntv2_str, void* pOwner, NTV2StreamBuffer* pBuffer)
{
    int status;
    
    if ((ntv2_str == NULL) || (pOwner == NULL) || (pBuffer == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: buffer release no owner or buffer\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: buffer release timeout acquiring stream lock\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;
    }

    // check if stream
    if ((ntv2_str->stream_state == ntv2_stream_state_disabled) ||
        (ntv2_str->stream_state == ntv2_stream_state_released) ||
        (ntv2_str->stream_state == ntv2_stream_state_error))
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_STATE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer release stream state disabled\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // check owner
    if (pOwner != ntv2_str->owner)
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_OWNER;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer release not owner\n", ntv2_str->name);
        return NTV2_STATUS_SUCCESS;
    }

    // check stream head for release
    if (!ntv2_str->stream_buffers[ntv2_str->head_index].prepared ||
        !ntv2_str->stream_buffers[ntv2_str->head_index].completed ||
        !ntv2_str->stream_buffers[ntv2_str->head_index].flushed)
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_SUCCESS;
    }

    // release buffer
    status = (ntv2_str->stream_ops.buffer_release)(ntv2_str, ntv2_str->head_index);
    if (status != NTV2_STREAM_OPS_SUCCESS)
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_RESOURCE;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        NTV2_MSG_STREAM_ERROR("%s: buffer release failed\n", ntv2_str->name);
        return status;
    }

    // report final status
    buffer_status(&ntv2_str->stream_buffers[ntv2_str->head_index], pBuffer);
    pBuffer->mStatus = NTV2_STREAM_STATUS_SUCCESS;

    // clear buffer
    ntv2_str->stream_buffers[ntv2_str->head_index].prepared = false;
    ntv2_str->stream_buffers[ntv2_str->head_index].linked = false;
    ntv2_str->stream_buffers[ntv2_str->head_index].completed = false;
    ntv2_str->stream_buffers[ntv2_str->head_index].flushed = false;
    ntv2_str->stream_buffers[ntv2_str->head_index].released = true;

    // increment head
    ntv2_str->head_index = queue_next(ntv2_str->head_index);
    
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_buffer_status(struct ntv2_stream *ntv2_str, NTV2StreamBuffer* pBuffer)
{
    uint32_t i;
    
    if ((ntv2_str == NULL) || (pBuffer == NULL))
    {
        NTV2_MSG_STREAM_ERROR("%s: buffer release no owner or buffer\n", ntv2_str->name);
        return NTV2_STATUS_FAIL;        
    }

    // synchronize
    if (!ntv2SemaphoreDown(&ntv2_str->state_sema, NTV2_STREAM_ACTION_TIMEOUT))
    {
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_TIMEOUT;
        NTV2_MSG_STREAM_ERROR("%s: buffer release timeout acquiring stream lock\n", ntv2_str->name);
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
        buffer_status(NULL, pBuffer);
        pBuffer->mStatus = NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_INVALID;
        ntv2SemaphoreUp(&ntv2_str->state_sema);
        return NTV2_STATUS_SUCCESS;
    }
    
    // get state
    buffer_status(&ntv2_str->stream_buffers[i], pBuffer);
    pBuffer->mStatus = NTV2_STREAM_STATUS_SUCCESS;
    ntv2SemaphoreUp(&ntv2_str->state_sema);
    return NTV2_STATUS_SUCCESS;
}




