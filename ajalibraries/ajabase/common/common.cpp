/**
    @file		common.cpp
    @copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
    @brief		Generic helper functions.
**/

#include "common.h"

#include <algorithm>
#include <sstream>
#include <string>

#include <stdlib.h>

namespace aja
{

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

int stoi(const std::string& str, std::size_t* idx, int base)
{
    return (int)aja::stol(str, idx, base);
}

long stol(const std::string& str, std::size_t* idx, int base)
{
    char* pEnd = NULL;
    long retVal = ::strtol(str.c_str(), &pEnd, base);
    if (idx && pEnd)
    {
        *idx = pEnd - str.c_str();
    }
    return retVal;
}

long long stoll(const std::string& str, std::size_t* idx, int base)
{
    return (long long)aja::stol(str, idx, base);
}

unsigned long stoul(const std::string& str, std::size_t* idx, int base)
{
    char* pEnd = NULL;
    unsigned long retVal = ::strtoul(str.c_str(), &pEnd, base);
    if (idx && pEnd)
    {
        *idx = pEnd - str.c_str();
    }
    return retVal;
}

unsigned long long stoull(const std::string& str, std::size_t* idx, int base)
{
    return (unsigned long long)aja::stoul(str, idx, base);
}

float stof(const std::string& str, std::size_t* idx)
{
    return (float)aja::stod(str, idx);
}

double stod(const std::string& str, std::size_t* idx)
{
    char* pEnd = NULL;
    double retVal = ::strtod(str.c_str(), &pEnd);
    if (idx && pEnd)
    {
        *idx = pEnd - str.c_str();
    }
    return retVal;
}

long double stold(const std::string& str, std::size_t* idx)
{
    return (long double)aja::stod(str, idx);
}

std::string to_string(bool val)
{
    return val ? "true" : "false";
}

std::string to_string(int val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}

std::string to_string(long val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}

std::string to_string(long long val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}

std::string to_string(unsigned val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}

std::string to_string(unsigned long val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}

std::string to_string(unsigned long long val)
{
    std::ostringstream oss; oss << val;
    return oss.str();
}

std::string to_string(float val)
{
    std::ostringstream oss;
    oss.precision(6);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << val;
    return oss.str();
}

std::string to_string(double val)
{
    std::ostringstream oss;
    oss.precision(6);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << val;
    return oss.str();
}

std::string to_string(long double val)
{
    std::ostringstream oss;
    oss.precision(6);
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << val;
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

    // if last character in string matches the split delim add an empty string
    if (str.length() > 0 && str[str.length()-1] == delim)
    {
        elems.push_back("");
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

std::string& lstrip(std::string& str, const std::string& ws)
{
    str.erase(0, str.find_first_not_of(ws));
    return str;
}

std::string& rstrip(std::string& str, const std::string& ws)
{
    if(str.length() > 0)
    {
        str.erase(str.find_last_not_of(ws)+1,str.length()-1);
    }
    return str;
}

std::string& strip(std::string& str, const std::string& ws)
{
    lstrip(str,ws);
    rstrip(str,ws);
    return str;
}

std::string join(std::vector<std::string> parts, const std::string& delim)
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

} //end aja namespace
