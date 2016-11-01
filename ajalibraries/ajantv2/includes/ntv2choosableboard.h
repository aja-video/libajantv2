//
//  ntv2choosableboard.h
//
//	Copyright (C) 2005, 2006, 2010 AJA Video Systems, Inc.  
//	Proprietary and Confidential information.
//
//   Types and constants for network-aware choosable board list.
//

#ifndef NTV2CHOOSABLEBOARD_H
#define NTV2CHOOSABLEBOARD_H

#define MAX_LOCAL_BOARDS	12
#define MAX_REMOTE_BOARDS	512
#define MAX_CHOOSABLE_BOARDS ( MAX_LOCAL_BOARDS + MAX_REMOTE_BOARDS)

#define NO_BOARD_CHOSEN (99999)

#define CHOOSABLE_BOARD_STRMAX	(16)
typedef struct 
{
	ULWord			boardNumber;	// Card number, 0 .. 3
	ULWord			boardType;		// e.g. BOARDTYPE_KHD
	NTV2DeviceID	boardID;
	char			description [NTV2_DISCOVER_BOARDINFO_DESC_STRMAX];	// "IPADDR: board identifier"
	char			hostname [CHOOSABLE_BOARD_STRMAX];	// 0 len == local board.
} NTV2ChoosableBoard;

#endif //NTV2CHOOSABLEBOARD_H
