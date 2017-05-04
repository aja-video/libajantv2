/**
    @file		common.cpp
	@copyright	Copyright (C) 2015 AJA Video Systems, Inc.  All rights reserved.
    @brief		Generic helper functions.
**/

#include "common.h"

#include <string>

std::string& replace(std::string& str, const std::string& from, const std::string& to)
{
    if (!from.empty())
        for (size_t pos = 0; (pos = str.find(from, pos)) != std::string::npos; pos += to.size())
            str.replace(pos, from.size(), to);
    return str;
}
