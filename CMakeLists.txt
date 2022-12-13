cmake_minimum_required(VERSION 3.10)

project(libajantv2)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(AJABuildOptions)
include(AJACommonDefines)
include(AJACommonPaths)
include(AJACommonFlags)
include(AJAHelpers)

aja_add_subdirectory(ajantv2)
