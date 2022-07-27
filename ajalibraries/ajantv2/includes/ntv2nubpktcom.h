/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubpktcom.h
	@brief		Declares functions that handle NTV2 "nub" packets.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#ifndef __NTV2NUBPKTCOM_H
#define __NTV2NUBPKTCOM_H

#include "ajaexport.h"
#include "ntv2nubtypes.h"

#ifdef MSWindows
	#include <WinSock2.h>
	typedef int socklen_t ;
#else
	#include <sys/socket.h>
#endif

#endif	//	__NTV2NUBPKTCOM_H
