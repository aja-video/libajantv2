/**
    @file		common.cpp
	@copyright	Copyright (C) 2015 AJA Video Systems, Inc.  All rights reserved.
    @brief		Generic helper functions.
**/

#include "common.h"

#include <algorithm>
#include <sstream>
#include <string>

std::string& replace(std::string& str, const std::string& from, const std::string& to)
{
    if (!from.empty())
    {
        for (size_t pos = 0; (pos = str.find(from, pos)) != std::string::npos; pos += to.size())
        {
            str.replace(pos, from.size(), to);
        }
    }
    return str;
}

int string_to_int(const std::string str, int fallback)
{
    int number = fallback;
    if (str.length() > 1 &&
        str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        const std::string  hexStr(str.substr(2));
        std::istringstream iss(hexStr);
        iss >> std::hex >> number;
        if (iss.fail())
        {
            number = fallback;
        }
    }
    else
    {
        std::istringstream iss(str);
        iss >> number;
        if (iss.fail())
        {
            number = fallback;
        }
    }
    return number;
}

std::string int_to_string(int i)
{
    std::ostringstream oss;
    oss << i;
    return oss.str();
}

void split(const std::string& str, const char delim, std::vector<std::string>& elems)
{
    std::stringstream ss(str);
    std::string item;
    while(std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
}

std::vector<std::string> split(const std::string& str, const char delim)
{
    std::vector<std::string> elems;
    split(str, delim, elems);
    return elems;
}

std::string& lower(std::string& str)
{
    std::transform (str.begin (), str.end (), str.begin (), ::tolower);
    return str;
}

std::string& upper(std::string& str)
{
    std::transform (str.begin (), str.end (), str.begin (), ::toupper);
    return str;
}

std::string& lstrip(std::string& str, const std::string ws)
{
    str.erase(0, str.find_first_not_of(ws));
    return str;
}

std::string& rstrip(std::string& str, const std::string ws)
{
    if(str.length() > 0)
    {
        str.erase(str.find_last_not_of(ws)+1,str.length()-1);
    }
    return str;
}

std::string& strip(std::string& str, const std::string ws)
{
    lstrip(str,ws);
    rstrip(str,ws);
    return str;
}

std::string join(std::vector<std::string> parts, const std::string delim)
{
    std::ostringstream oss;
    std::vector<std::string>::iterator it;
    int count=0;
    for(it=parts.begin();it!= parts.end();++it)
    {
        if (count==0)
            oss << *it;
        else
            oss << delim << *it;

        count++;
    }
    return oss.str();
}
