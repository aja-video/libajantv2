cmake_minimum_required(VERSION 3.10)

project(libajantv2)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(NTV2BuildOptions)
include(NTV2CompilerFlags)
include(NTV2Defines)
include(NTV2Version)

aja_add_subdirectory(ajantv2)
aja_add_subdirectory(demos)
