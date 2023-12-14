/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2streamops.h
// Purpose:	 ntv2 stream operations
//
///////////////////////////////////////////////////////////////

#ifndef NTV2STREAMOPS_HEADER
#define NTV2STREAMOPS_HEADER

#include "ntv2stream.h"


struct ntv2_kernel_buffer {
};
 

struct ntv2_kernel_stream {
    
};


Ntv2Status ntv2_stream_initialize(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_start(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_stop(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_stream_program(struct ntv2_stream *ntv2_str);
Ntv2Status ntv2_buffer_prepare(struct ntv2_stream_buffer *ntv2_buffer);
Ntv2Status ntv2_buffer_release(struct ntv2_stream_buffer *ntv2_buffer);

#endif

