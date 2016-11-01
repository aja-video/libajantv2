/**
	@file		ntv2endian.h
	@copyright	Copyright (C) 2008-2016 AJA Video Systems, Inc.  All rights reserved.
	@brief		Defines a number of handy byte-swapping macros.
**/

#ifndef NTV2ENDIAN_H
#define NTV2ENDIAN_H

#include "ajatypes.h"

// unconditional endian byte swap 

#define NTV2EndianSwap16(value)               \
        (((((UWord)value)<<8) & 0xFF00)   | \
         ((((UWord)value)>>8) & 0x00FF))

#define NTV2EndianSwap32(value)                    \
        (((((ULWord)value)<<24) & 0xFF000000)  | \
         ((((ULWord)value)<< 8) & 0x00FF0000)  | \
         ((((ULWord)value)>> 8) & 0x0000FF00)  | \
         ((((ULWord)value)>>24) & 0x000000FF))

#define NTV2EndianSwap64(value)                                 \
		(((((ULWord64)value)<<56) & 0xFF00000000000000ULL)  | \
		 ((((ULWord64)value)<<40) & 0x00FF000000000000ULL)  | \
		 ((((ULWord64)value)<<24) & 0x0000FF0000000000ULL)  | \
		 ((((ULWord64)value)<< 8) & 0x000000FF00000000ULL)  | \
		 ((((ULWord64)value)>> 8) & 0x00000000FF000000ULL)  | \
		 ((((ULWord64)value)>>24) & 0x0000000000FF0000ULL)  | \
		 ((((ULWord64)value)>>40) & 0x000000000000FF00ULL)  | \
		 ((((ULWord64)value)>>56) & 0x00000000000000FFULL))


#if AJATargetBigEndian
	
	// big-to-host native
	#define NTV2EndianSwap16BtoH(value)               (value)
	#define NTV2EndianSwap16HtoB(value)               (value)
	#define NTV2EndianSwap32BtoH(value)               (value)
	#define NTV2EndianSwap32HtoB(value)               (value)
	
	// little-to-host translate 
	#define NTV2EndianSwap16LtoH(value)               NTV2EndianSwap16(value)
	#define NTV2EndianSwap16HtoL(value)               NTV2EndianSwap16(value)
	#define NTV2EndianSwap32LtoH(value)               NTV2EndianSwap32(value)
	#define NTV2EndianSwap32HtoL(value)               NTV2EndianSwap32(value)
	
	// big-to-host native
	#define NTV2EndianSwap64BtoH(value)               (value)
	#define NTV2EndianSwap64HtoB(value)               (value)
	// little-to-host translate
	#define NTV2EndianSwap64LtoH(value)               NTV2EndianSwap64(value)
	#define NTV2EndianSwap64HtoL(value)               NTV2EndianSwap64(value)

#else	// little-endian target host
	
	// big-to-host translate
	#define NTV2EndianSwap16BtoH(value)               NTV2EndianSwap16(value)
	#define NTV2EndianSwap16HtoB(value)               NTV2EndianSwap16(value)
	#define NTV2EndianSwap32BtoH(value)               NTV2EndianSwap32(value)
	#define NTV2EndianSwap32HtoB(value)               NTV2EndianSwap32(value)
	
	// little-to-host native
	#define NTV2EndianSwap16LtoH(value)               (value)
	#define NTV2EndianSwap16HtoL(value)               (value)
	#define NTV2EndianSwap32LtoH(value)               (value)
	#define NTV2EndianSwap32HtoL(value)               (value)
	
	// big-to-host translate
	#define NTV2EndianSwap64BtoH(value)	              NTV2EndianSwap64(value)
	#define NTV2EndianSwap64HtoB(value)               NTV2EndianSwap64(value)
	// little-to-host native
	#define NTV2EndianSwap64LtoH(value)               (value)
	#define NTV2EndianSwap64HtoL(value)               (value)

#endif

#endif	// NTV2ENDIAN_H



