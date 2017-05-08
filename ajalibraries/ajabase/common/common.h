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

static const std::string AJA_WHITESPACE(" \t\n\r");

/**
 *	Replace a substring within a string with a new string
 *
 *  @param[in,out]  str  The string to modify
 *  @param[in]      from The substring to look for
 *  @param[in]      to   The string to replace the substring with
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string& replace(std::string& str, const std::string& from, const std::string& to);

/**
 *	Convert the string to an int if possible
 *
 *  @param[in]  str      The string to get the int value of
 *  @param[in]  fallback A default value to return in the case of a failure
 *	@return		The int value of the input str or fallback in the case of failure
 */
AJA_EXPORT int string_to_int(const std::string str, int fallback = 0);

/**
 *	Place an int into a string
 *
 *  @param[in]  i The int to convert to a string
 *	@return		A string representing the passed int
 */
AJA_EXPORT std::string int_to_string(int i);

/**
 *	Splits a string into substrings at a character delimeter
 *
 *  @param[in]   str   The string to split into parts
 *  @param[in]   delim The character delimeter to split the string at
 *  @param[out]  elems A vector of strings that contains all the substrings
 *	@return		 void
 */
AJA_EXPORT void split(const std::string& str, const char delim, std::vector<std::string>& elems);

/**
 *	Splits a string into substrings at a character delimeter
 *
 *  @param[in]  str   The string to split into parts
 *  @param[in]  delim The character delimeter to split the string at
 *	@return		A vector of strings that contains all the substrings
 */
AJA_EXPORT std::vector<std::string> split(const std::string& str, const char delim);

/**
 *	Converts the passed string to lowercase
 *
 *  @param[in,out]  str   The string to make lowercase
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string& lower(std::string& str);

/**
 *	Converts the passed string to uppercase
 *
 *  @param[in,out]  str   The string to make uppercase
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string& upper(std::string& str);

/**
 *	Strips the leading whitespace characters from the string
 *
 *  @param[in,out]  str  The string to strip leading characters from
 *  @param[in]      ws   The whitespace characters to strip
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string& lstrip(std::string& str, const std::string ws=AJA_WHITESPACE);

/**
 *	Strips the trailing whitespace characters from the string
 *
 *  @param[in,out]  str  The string to strip trailing characters from
 *  @param[in]      ws   The whitespace characters to strip
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string& rstrip(std::string& str, const std::string ws=AJA_WHITESPACE);

/**
 *	Strips the leading & trailing whitespace characters from the string
 *
 *  @param[in,out]  str  The string to strip leading & trailing characters from
 *  @param[in]      ws   The whitespace characters to strip
 *	@return		    Reference to the modified STL string
 */
AJA_EXPORT std::string& strip(std::string& str, const std::string ws=AJA_WHITESPACE);

/**
 *	Join a vector of strings separated by a string delimeter
 *
 *  @param[in]  parts  The vector of strings to join together
 *  @param[in]  delim  The string delimeter that will separate the strings
 *	@return		The joined string made up of the parts concatinated with delimeter string
 */
AJA_EXPORT std::string join(std::vector<std::string> parts, const std::string delim=" ");

#endif	//	AJA_COMMON_H
