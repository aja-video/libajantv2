/**
	@file		guid.h
	@copyright	Copyright (C) 2015 AJA Video Systems, Inc.  All rights reserved.
	@brief		Generates a new, unique UUID as an STL string.
**/

#ifndef AJA_GUID_H
#define AJA_GUID_H

#include <string>
#include "public.h"

extern "C"
{
	#if defined(AJA_WINDOWS)
		#include <Rpc.h>
	#elif defined(AJA_LINUX)
		#include <stdio.h>
	#else
		#include <uuid/uuid.h>
	#endif
}

/**
 *	Generates a uuid.
 *
 *	@return		An STL string that contains the new UUID.
 */
std::string AJA_EXPORT CreateGuid (void);


#endif	//	AJA_GUID_H
