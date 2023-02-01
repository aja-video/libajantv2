/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA boards.
//
////////////////////////////////////////////////////////////
//
// Filename: driverdbg.h
// Purpose:	 Macros used for debugging the ntv2driver.
// Notes:
//
///////////////////////////////////////////////////////////////

#define MSG(string, args...) printk(KERN_ALERT  string , ##args)

#define SHOW_DMA_CTL_STRUCT(p) \
do { \
   MSG("DMAControl Struct %p\n" \
	   "\tframeNumber\t%d\n" \
	   "\tframeBuffer\t%p\n" \
	   "\tframeOffsetSrc\t%d\n" \
	   "\tframeOffsetDest\t%d\n" \
	   "\tnumBytes\t%d\n" \
	   "\tdownSample\t%d\n" \
	   "\tlinePitch\t%d\n" \
	   "\tpoll\t%d\n", \
	   &p, \
	   p.frameNumber, p.frameBuffer, p.frameOffsetSrc, p.frameOffsetDest, \
	   p.numBytes, p.downSample, p.linePitch, p.poll ); \
} while (0)

#define SHOW_AUTOCIRCULATE_DATA_STRUCT(p) \
do { \
   MSG("AutoCirculateData Struct %p\n" \
	   "\tcommand\t%d\n" \
	   "\tchannelSpec\t%d\n" \
	   "\tlVal1\t%ld\n" \
	   "\tlVal2\t%ld\n" \
	   "\tbVal1\t%d\n" \
	   "\tbVal2\t%d\n" \
	   "\tpvVal1\t%p\n" \
	   "\tpvVal2\t%p\n" \
	   &p, \
	   p.eCommand, p.channelSpec, p.lVal1, p.lVal2, \
	   p.bVal1, p.bVal2, p.pvVal1, p.pvVal2 ); \
} while (0)

#define SHOW_INTERNAL_AUTOCIRCULATE_STRUCT(p) \
do { \
   MSG("Internal AutoCirculate Struct 0x%p\n" \
	   "\tstate\t%d\n" \
	   "\tchannelSpec\t%d\n" \
	   "\trecording\t%ld\n" \
	   "\tcurrentFrame\t%ld\n" \
	   "\tstartFrame\t%ld\n" \
	   "\tendFrame\t%ld\n" \
	   "\tactiveFrame\t%ld\n" \
	   "\tactiveFrameRegister\t%ld\n" \
	   "\tcirculateWithAudio\t%d\n" \
	   "\tcirculateWithRP188\t%d\n" \
	   "\tcirculateWithColorCorrection\t%d\n"	\
	   "\tcirculateWithVidProc\t%d\n"			\
	   "\tcirculateWithCustomAncData\t%d\n"		\
	   "\tenableFbfChange\t%d\n" \
	   "\tenableFboChange\t%d\n"	\
	   "\tstartTimeStamp\t0x%llx\n" \
	   "\tstartAudioClockTimeStamp\t0x%llx\n" \
	   "\tframesProcessed\t%ld\n" \
	   "\tdroppedFrames\t%ld\n" \
	   "\tPlus some frameStampStuff\n", \
	   p, \
	   p->state, \
	   p->channelSpec, \
	   (long)p->recording, \
	   (long)p->currentFrame, \
	   (long)p->startFrame, \
	   (long)p->endFrame, \
	   (long)p->activeFrame, \
	   (long)p->activeFrameRegister, \
	   (int)p->circulateWithAudio, \
	   (int)p->circulateWithRP188, \
	   (int)p->circulateWithColorCorrection,	\
	   (int)p->circulateWithVidProc,			\
	   (int)p->circulateWithCustomAncData,		\
	   (int)p->enableFbfChange, \
	   (int)p->enableFboChange, \
	   p->startTimeStamp, \
	   p->startAudioClockTimeStamp, \
	   (long)p->framesProcessed, \
	   (long)p->droppedFrames \
	   ); \
} while (0)

#define SHOW_FRAME_STAMP_STRUCT(p) \
do { \
   MSG("FRAME_STAMP_STRUCT\t%p\n" \
	   "\tchannelSpec\t\t%d\n" \
	   "\tframeTime\t\t%lld\n" \
	   "\tframe\t\t\t%ld\n" \
	   "\taudioClockTimeStamp\t%lld\n" \
	   "\taudioExpectedAddress\t%ld\n" \
	   "\taudioInStartAddress\t%ld\n" \
	   "\taudioInStopAddress\t%ld\n" \
	   "\taudioOutStopAddress\t%ld\n" \
	   "\taudioOutStartAddress\t%ld\n" \
	   "\tbytesRead\t\t%ld\n" \
	   "\tstartSample\t\t%ld\n" \
	   "\t------------------\t\n" \
	   "\tcurrentTime\t\t\t%lld\n" \
	   "\tcurrentFrame\t\t\t%ld\n" \
	   "\tRP188 Data unavailable\n" \
	   "\tcurrentFrameTime\t\t%lld\n" \
	   "\taudioClockCurrentTime\t\t%lld\n" \
	   "\tcurrentAudioExpectedAddress\t%ld\n" \
	   "\tcurrentAudioStartAddress\t%ld\n" \
	   "\tcurrentFieldCount\t\t%ld\n" \
	   "\tcurrentLineCount\t\t%ld\n" \
	   "\tcurrentReps\t\t\t%ld\n" \
	   "\tcurrenthUser\t\t\t%ld\n", \
		p, \
		p->channelSpec, p->frameTime, (long)p->frame, \
		p->audioClockTimeStamp, (long)p->audioExpectedAddress, \
		(long)p->audioInStartAddress, (long)p->audioInStopAddress, \
		(long)p->audioOutStopAddress, (long)p->audioOutStartAddress, \
		(long)p->bytesRead, (long)p->startSample, \
		\
		p->currentTime, (long)p->currentFrame, p->currentFrameTime, \
		p->audioClockCurrentTime, (long)p->currentAudioExpectedAddress, \
		(long)p->currentAudioStartAddress, \
		(long)p->currentFieldCount, (long)p->currentLineCount,\
		(long)p->currentReps, (long)p->currenthUser\
	   ); \
} while (0)

#define SHOW_INTERNAL_FRAME_STAMP_STRUCT(p) \
do { \
   MSG("INTERNAL_FRAME_STAMP_STRUCT\t%p\n" \
	   "\tframeTime\t\t%lld\n" \
	   "\taudioClockTimeStamp\t%lld\n" \
	   "\taudioExpectedAddress\t%ld\n" \
	   "\taudioInStartAddress\t%ld\n" \
	   "\taudioInStopAddress\t%ld\n" \
	   "\taudioOutStopAddress\t%ld\n" \
	   "\taudioOutStartAddress\t%ld\n" \
	   "\tbytesRead\t\t%ld\n" \
	   "\tstartSample\t\t%ld\n" \
	   "\t------------------\t\n" \
	   "\tRP188 Data unavailable\n" \
	   "\tvalidCount\t\t%ld\n" \
	   "\trepeatCount\t\t%ld\n" \
	   "\thUser\t\t\t%ld\n", \
		p, \
		p->frameTime, \
		p->audioClockTimeStamp, (long)p->audioExpectedAddress, \
		(long)p->audioInStartAddress, (long)p->audioInStopAddress, \
		(long)p->audioOutStopAddress, (long)p->audioOutStartAddress, \
		(long)p->bytesRead, (long)p->startSample, \
		\
		(long)p->validCount, (long)p->repeatCount,\
		(long)p->hUser\
	   ); \
} while (0)

#define SHOW_DMA_IOCTL(s) \
{ \
	MSG("Received " #s " for boardNumber %d\n", boardNumber); \
	SHOW_DMA_CTL_STRUCT(param); \
	char *bufInfo = (param.frameBuffer) ? "User mode buffer\n" : "Driver allocated buffer\n"; \
	MSG("%s", bufInfo); \
}

#define SHOULD_SHOW_DMA_DEBUG_MESSAGE(frameNumber, audioFrameNumber) \
(  \
	(MsgsEnabled(NTV2_DRIVER_DMA_AUDIO_DEBUG_MESSAGES) && ((frameNumber) == (audioFrameNumber))) \
||	(MsgsEnabled(NTV2_DRIVER_DMA_VIDEO_DEBUG_MESSAGES) && ((frameNumber) != (audioFrameNumber))) \
)


