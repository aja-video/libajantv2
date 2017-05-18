/**
	@file		public.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Master header for the ajabase library.
**/

#ifndef AJA_PUBLIC_H
#define AJA_PUBLIC_H

#include <string>
#include <list>
#include <vector>
#include <map>

#include "ajabase/common/types.h"
#include "ajabase/common/export.h"

/**
	@page	ajabase	The AJABase Library

	This library is a handy collection of platform-independent classes, templates that provide several practical, basic services
	including...

	- Memory Management
		- AJARefPtr: automatic reference-counted pointers
		- AJACircularBuffer: a circular ring of fixed-size buffers, useful for video streaming
		- AJAMemory
		- AJABuffer
	- File Management
		- AJADiskStatus:  for reporting free space
		- AJAFileIO:  platform-independent host file I/O
	- Timing
		- AJAPerformance and AJATimer: used for timing purposes
		- AJATimeBase:  a high-resolution time base, with many conversion capabilities
		- AJATime
	- Threading and Synchronization
		- AJAThread:  thread creation and management
		- AJAAtomic: atomic swap, increment or decrement operations
		- AJADebug:  debug logging
		- AJAEvent:  event signaling
		- AJALock and AJAAutoLock:  mutexes and critical sections
	- Video and Audio
		- AJAPixelFormat:  describes many popular pixel buffer (raster) formats
		- AJATestPatternGen:  for generating test patterns in a variety of pixel (raster) formats
		- AJATimeCodeBurn:  "burns" a timecode into a raster
		- AJAWavWriterAudioFormat:  writes audio samples into a .WAV file
		- AJA_GenerateAudioTone:  generates audio samples
		- AJATimeCode
		- AJADPXFileIO and DpxHdr
	- Miscellaneous
        - AJASystemInfo: get common information about the system
		- the "popt library" for parsing command line arguments
		- CreateGuid: create globally unique identifier strings
		- AJAPersistence:  save/retrieve integer/string/blob data to/from a host user's registry/preferences store using string keys
		- sqlite3:  the SQLite mini RDBMS
		- AJAPnp:  get notified when AJA devices get attached/detached to/from the host
**/

#endif	//	AJA_PUBLIC_H
