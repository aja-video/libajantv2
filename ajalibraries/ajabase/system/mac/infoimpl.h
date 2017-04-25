/**
    @file		mac/infoimpl.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
    @brief		Declares the AJASystemInfoImpl class.
**/

#ifndef AJA_INFO_IMPL_H
#define AJA_INFO_IMPL_H

#include "ajabase/common/common.h"

class AJASystemInfoImpl
{
public:

    AJASystemInfoImpl();
    virtual ~AJASystemInfoImpl();

    virtual AJAStatus Rescan();

    std::map<int, std::string> mDescMap;
    std::map<int, std::string> mValueMap;
};

#endif	//	AJA_INFO_IMPL_H
