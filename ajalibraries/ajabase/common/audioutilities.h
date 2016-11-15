/**
	@file		audioutilities.h
	@copyright	Copyright (C) 2012-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declaration of AJA_GenerateAudioTone function.
**/

#ifndef AJA_AUDIOUTILS_H
#define AJA_AUDIOUTILS_H

#include "public.h"

#define AJA_MAX_AUDIO_CHANNELS 16

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

uint32_t AJA_EXPORT AJA_GenerateAudioTone (
							uint32_t*	audioBuffer,
							uint32_t	numSamples,
							uint32_t	numChannels,
							uint32_t	numBits,
							uint32_t&	cycleSample,
							double		sampleRate,
							double*		amplitude,
							double*		frequency,
							bool		endianConvert);


#endif
