cmake_minimum_required(VERSION 3.10)

project(libajantv2)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(common/CMakeOptions)
include(ntv2/NTV2CompilerFlags)
include(ntv2/NTV2Defines)
include(ntv2/NTV2Helpers)
include(ntv2/NTV2Version)

option(AJANTV2_BUILD_OPENSOURCE "Build libajantv2 as open-source (MIT license)?" OFF)
option(AJANTV2_BUILD_SHARED     "Build libajantv2 shared libraries?" OFF)
option(AJANTV2_BUILD_DEMOS      "Build libajantv2 demo apps?" ON)
option(AJANTV2_BUILD_TOOLS      "Build libajantv2 tests?" ON)
option(AJANTV2_BUILD_TESTS      "Build libajantv2 tests?" ON)

aja_add_subdirectory(ajantv2)
if (AJA_BUILD_DEMOS)
    aja_add_subdirectory(demos)
endif()
if (AJA_BUILD_TOOLS)
    aja_add_subdirectory(tools)
endif()
