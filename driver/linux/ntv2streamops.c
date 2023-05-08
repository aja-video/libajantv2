/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2streamops.c
//
//==========================================================================

#if defined(CONFIG_SMP)
#define __SMP__
#endif

#include "ntv2streamops.h"

Ntv2Status ntv2_stream_initialize(struct ntv2_stream *ntv2_str)
{
    Ntv2Status status = NTV2_STATUS_SUCCESS;

    
    
    // initialize state




    if (ntv2_str->enabled)
    {
        if (status == NTV2_STATUS_SUCCESS)
        {
            ntv2_str->stream_state = ntv2_stream_state_initialized;
        }
        else
        {
            ntv2_str->stream_state = ntv2_stream_state_error;
        }
    }
    else
    {
        ntv2_str->stream_state = ntv2_stream_state_disabled;
    }

    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_start(struct ntv2_stream *ntv2_str)
{
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_stop(struct ntv2_stream *ntv2_str)
{
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_stream_program(struct ntv2_stream *ntv2_str)
{
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_buffer_prepare(struct ntv2_stream_buffer *ntv2_buffer)
{
    return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_buffer_release(struct ntv2_stream_buffer *ntv2_buffer)
{
    return NTV2_STATUS_SUCCESS;
}
