/**
    @file		linux/infoimpl.h
	@copyright	Copyright (C) 2009-2019 AJA Video Systems, Inc.  All rights reserved.
    @brief		Declares the AJASystemInfoImpl class.
**/

#ifndef AJA_INFO_IMPL_H
#define AJA_INFO_IMPL_H

#include "ajabase/common/common.h"
#include "ajabase/system/info.h"

class AJASystemInfoImpl
{
public:

    AJASystemInfoImpl(int units);
    virtual ~AJASystemInfoImpl();

    virtual AJAStatus Rescan(AJASystemInfoSections sections);

    std::map<int, std::string> mLabelMap;
    std::map<int, std::string> mValueMap;

    int mMemoryUnits;
};

#endif	//	AJA_INFO_IMPL_H
