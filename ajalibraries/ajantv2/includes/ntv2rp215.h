/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2rp215.h
	@deprecated	This module was retroactively deprecated to SDK 15.3 in SDK 16.3.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#ifndef __NTV2_RP215_
#define __NTV2_RP215_


#include "ajatypes.h"
#ifdef MSWindows
	#include "stdio.h"
	#define nil NULL
#endif
#ifdef AJALinux
	#define nil NULL
#endif
#include "ajatypes.h"
#include "ntv2enums.h"


#if !defined(NTV2_DEPRECATE_15_3)

	#define RP215_PAYLOADSIZE 215  


	/**
		@deprecated	The CNTV2RP215Decoder class was retroactively deprecated to SDK 15.3 in SDK 16.3.
	**/
	class CNTV2RP215Decoder
	{
		public:
			 CNTV2RP215Decoder(ULWord* pFrameBufferBaseAddress,NTV2VideoFormat videoFormat,NTV2FrameBufferFormat fbFormat);
			~CNTV2RP215Decoder();

			bool Locate();
			bool Extract();
	
		private:
			ULWord*					_frameBufferBasePointer;
			NTV2VideoFormat			_videoFormat;
			NTV2FrameBufferFormat	_fbFormat;
			Word					_lineNumber;
			Word					_pixelNumber;

			UByte _rp215RawBuffer[RP215_PAYLOADSIZE];
	};

#endif	//	!defined(NTV2_DEPRECATE_15_3)

#endif	// __NTV2_RP215_
