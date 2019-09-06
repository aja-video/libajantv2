/**
    @file		main.cpp
    @brief		Unittests for the AJA Base Library (using doctest).
    @copyright	Copyright (c) 2017 AJA Video Systems, Inc. All rights reserved.
**/
// for doctest usage see: https://github.com/onqtam/doctest/blob/1.1.4/doc/markdown/tutorial.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL
#include "doctest.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "limits.h"

#include "ntv2card.h"
#include "ntv2spiinterface.h"
#include "ajabase/common/common.h"

#if 0
template
void filename_marker() {} //this is used to easily just around in a GUI with a symbols list
TEST_SUITE("filename" * doctest::description("functions in streams/common/filename.h")) {

    TEST_CASE("constructor")
    {
    }

} //filename
#endif

void spi_marker() {}
TEST_SUITE("spi" * doctest::description("new spi flasher")) {

    TEST_CASE("normal")
    {

    }

} //spi
