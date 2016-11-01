//
// Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//
#ifndef NTV2FILE_H
#define NTV2FILE_H

#define NTV2FILE_CURRVER 1

typedef struct {
    NTV2VideoFormat        videoFormat;
    NTV2FrameBufferFormat  frameBufferFormat;
    ULWord                 frameSize;               // in bytes, depends on videoformat and framebufferformat
    ULWord                 numFrames;               
	ULWord                 withAudio;               // == true if audio is desired.
	NTV2AudioRate		   audioRate;
	ULWord				   withRP188;
    UByte                  reserved[4096-(sizeof(NTV2VideoFormat)+sizeof(NTV2FrameBufferFormat)+sizeof(ULWord)+sizeof(ULWord)+sizeof(ULWord)+sizeof(NTV2AudioRate)+sizeof(ULWord) + sizeof(ULWord))];
	ULWord				   version;					// This must be last
} NTV2DiskFileInfo;



// Audio is stored with each frame. The following defines
#define NTV2_AUDIOSIZE_48K	(48*1024)  
#define NTV2_AUDIOSIZE_96K	(96*1024)  

#endif 
