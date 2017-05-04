/**
	@file		common.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Private include file for all ajabase sources.
**/

#ifndef AJA_COMMON_H
#define AJA_COMMON_H

#if defined(AJA_WINDOWS)
	#pragma warning(disable:4996)
	#pragma warning(disable:4800)
#endif

#include "ajabase/common/public.h"
#include "ajabase/system/debug.h"

/**
 *	Replace a substring within a string with a new string
 *
 *  @param[in]  str  The string to modify
 *  @param[in]  from The substring to look for
 *  @param[in]  to   The string to replace the substring with
 *	@return		Reference to the modified STL string.
 */
std::string& AJA_EXPORT replace(std::string& str, const std::string& from, const std::string& to);

#endif	//	AJA_COMMON_H
