/**	@file system.h
 *	System include for aja api classes.
 *
 *	Copyright (C) 2009 AJA Video Systems, Inc.  Proprietary and Confidential information.  All rights reserved.
 */

#ifndef AJA_SYSTEM_H
#define AJA_SYSTEM_H

#if defined(AJA_WINDOWS)
	#pragma warning (disable:4996)
	#if !defined(_WIN32_WINNT)
		#define _WIN32_WINNT 0x0500
	#endif
	#include <windows.h>
	#include <stdio.h>
	#include <tchar.h>
	#include <winioctl.h>
	#include <setupapi.h>
	#include <initguid.h>
#endif

#ifdef AJA_MAC
	#include <Carbon/Carbon.h>
	#include <CoreServices/CoreServices.h>
#endif

#endif
