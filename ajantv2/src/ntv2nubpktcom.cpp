/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubpktcom.cpp
	@brief		Implementation of shared NTV2 "nub" packet handling functions.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(AJALinux ) || defined(AJAMac)
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/time.h>
	#include <netdb.h>
#endif

#ifdef MSWindows
	#include <WinSock2.h>
	typedef int socklen_t;
#endif

#include "ajatypes.h"
