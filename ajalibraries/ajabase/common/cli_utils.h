/**
	@file		cli_utils.h
	@copyright	Copyright (C) 2011-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declaration of GetNumber, GetString and GetCharAsInt functions.
**/

#ifndef __CLI_UTILS_H
#define __CLI_UTILS_H

// Includes
#include "ajabase/common/types.h"
#include <iostream>
#include <string>


// Defines
#define INVALID_CHOICE(c) \
	cout << "The choice '" << char(c) << "' is not valid" << endl


// Declarations
bool GetNumber(const std::string& prompt, uint32_t& value);
bool GetString(const std::string& prompt, std::string& value);
bool GetCharAsInt(const std::string& prompt, int& value);


#endif	// __CLI_UTILS_H
