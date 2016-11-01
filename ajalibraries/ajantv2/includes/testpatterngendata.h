/**
	@file		testpatterngendata.h
	@brief		The NTV2 test pattern data.
	@copyright	(C) 2010-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
	@note		Starting in SDK 12.4, the test pattern data moved from here into testpatterngen.cpp.
**/

#ifndef TESTPATTERNDATA_H
#define TESTPATTERNDATA_H

typedef struct
{
	int					startLine;
	int					endLine;
	const uint32_t *	data;
} SegmentDescriptor;

const uint16_t NumTestPatternSegments = 8;
const uint16_t AJA_NUM_STANDARDS = 6;

typedef struct
{
	const char *		name;
	SegmentDescriptor	segmentDescriptor [AJA_NUM_STANDARDS] [NumTestPatternSegments];

} SegmentTestPatternData;


#endif	//	TESTPATTERNDATA_H
