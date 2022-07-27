/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2discover.h
	@brief		Declares the NTV nub discovery functions.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef NTV2DISCOVER_H
#define NTV2DISCOVER_H
#if defined(AJALinux ) || defined(AJAMac)
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include "ajaexport.h"
#include "ntv2nubtypes.h"

#endif	//	NTV2DISCOVER_H
