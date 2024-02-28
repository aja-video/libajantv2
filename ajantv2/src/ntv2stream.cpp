/* SPDX-License-Identifier: MIT */
/**
        @file		ntv2stream.cpp
        @brief		Implementation most of CNTV2Card's streaming functions.
        @copyright	(C) 2010-2023 AJA Video Systems, Inc.	 All rights reserved.
**/
#include "ntv2card.h"
#include "ntv2publicinterface.h"
#include "ntv2driverinterface.h"

ULWord CNTV2Card::StreamChannelInitialize (const NTV2Channel inChannel)
{
	NTV2StreamChannel status;
		if (!StreamChannelOps(inChannel, NTV2_STREAM_CHANNEL_INITIALIZE, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}

ULWord CNTV2Card::StreamChannelRelease (const NTV2Channel inChannel)
{
	NTV2StreamChannel status;
		if (!StreamChannelOps(inChannel, NTV2_STREAM_CHANNEL_RELEASE, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}

ULWord CNTV2Card::StreamChannelStart (const NTV2Channel inChannel,
										NTV2StreamChannel& status)
{
	if (!StreamChannelOps(inChannel, NTV2_STREAM_CHANNEL_START, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}

ULWord CNTV2Card::StreamChannelStop (const NTV2Channel inChannel,
										NTV2StreamChannel& status)
{
	if (!StreamChannelOps(inChannel, NTV2_STREAM_CHANNEL_STOP, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}

ULWord CNTV2Card::StreamChannelFlush (const NTV2Channel inChannel,
                                        NTV2StreamChannel& status)
{
    if (!StreamChannelOps(inChannel, NTV2_STREAM_CHANNEL_FLUSH, status))
        return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
    return status.mStatus;
}

ULWord CNTV2Card::StreamChannelStatus (const NTV2Channel inChannel,
										NTV2StreamChannel& status)
{
	if (!StreamChannelOps(inChannel, NTV2_STREAM_CHANNEL_STATUS, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}

ULWord CNTV2Card::StreamChannelWait (const NTV2Channel inChannel,
										NTV2StreamChannel& status)
{
	if (!StreamChannelOps(inChannel, NTV2_STREAM_CHANNEL_WAIT, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}

ULWord CNTV2Card::StreamBufferQueue (const NTV2Channel inChannel,
										NTV2Buffer inBuffer,
										ULWord64 bufferCookie,
										NTV2StreamBuffer& status)
{
	if (!StreamBufferOps(inChannel, inBuffer, bufferCookie, NTV2_STREAM_BUFFER_QUEUE, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}

ULWord CNTV2Card::StreamBufferRelease (const NTV2Channel inChannel,
										NTV2StreamBuffer& status)
{
	if (!StreamBufferOps(inChannel, NTV2Buffer(), 0, NTV2_STREAM_BUFFER_RELEASE, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}

ULWord CNTV2Card::StreamBufferStatus (const NTV2Channel inChannel,
										ULWord64 bufferCookie,
										NTV2StreamBuffer& status)
{
	if (!StreamBufferOps(inChannel, NTV2Buffer(), bufferCookie, NTV2_STREAM_BUFFER_STATUS, status))
		return NTV2_STREAM_STATUS_FAIL | NTV2_STREAM_STATUS_MESSAGE;
	return status.mStatus;
}
