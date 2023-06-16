cmake_minimum_required(VERSION 3.10)

# For CMAKE_MSVC_RUNTIME_LIBRARY compatability. Per note: This must be before the first project() 
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

project(libajantv2)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_MODULE_PATH)
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
endif()

if (NOT AJANTV2_VERSION_FILENAME)
    set(AJANTV2_VERSION_FILENAME ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt)
endif()

include(GNUInstallDirs)
include(CMakeOptions)
include(CompilerFlags)
include(WindowsHelpers)
include(ntv2/Defines)
include(ntv2/Version)
include(ntv2/Helpers)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if (DEFINED AJA_MSVC_RUNTIME_LIBRARY)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "${AJA_MSVC_RUNTIME_LIBRARY}" PARENT_SCOPE)
	endif()
endif()

option(AJANTV2_BUILD_OPENSOURCE "Build libajantv2 as open-source (MIT license)?" ON)
option(AJANTV2_BUILD_SHARED     "Build libajantv2 shared libraries?" OFF)
option(AJANTV2_BUILD_DEMOS      "Build libajantv2 demo apps?" OFF)
option(AJANTV2_BUILD_TOOLS      "Build libajantv2 tools?" OFF)
option(AJANTV2_BUILD_TESTS      "Build libajantv2 tests?" OFF)
option(AJANTV2_BUILD_DOCS       "Build libajantv2 docs?" OFF)
option(AJANTV2_BUILD_PLUGINS    "Build libajantv2 plugins?" OFF)
option(AJANTV2_BUILD_DRIVER     "Build Linux driver?" OFF)

if(NOT AJANTV2_BUILD_OPENSOURCE)
    if (NOT NTV2_PROPRIETARY_DIR)
        set(NTV2_PROPRIETARY_DIR ${CMAKE_SOURCE_DIR}/proprietary)
    endif()
endif()

# Cascade top-level options
if(DEFINED AJA_BUILD_SHARED)
    set(AJANTV2_BUILD_SHARED ${AJA_BUILD_SHARED})
endif()

if(DEFINED AJA_BUILD_TESTS)
	set(AJANTV2_BUILD_TESTS ${AJA_BUILD_TESTS})
endif()

if(DEFINED AJA_BUILD_DOCS)
    set(AJANTV2_BUILD_DOCS ${AJA_BUILD_DOCS})
endif()

if(DEFINED AJA_BUILD_TOOLS)
    set(AJANTV2_BUILD_TOOLS ${AJA_BUILD_TOOLS})
endif()

if(DEFINED AJA_BUILD_PLUGINS)
    set(AJANTV2_BUILD_PLUGINS ${AJA_BUILD_PLUGINS})
endif()

add_subdirectory(ajantv2)
if (AJANTV2_BUILD_DEMOS)
    add_subdirectory(demos)
endif()
if (AJANTV2_BUILD_DOCS)
    add_subdirectory(docs)
endif()
if (AJANTV2_BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()
if (AJANTV2_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if (AJANTV2_BUILD_TESTS)
    include(CTest)
    if (NOT TARGET doctest::doctest)
        set (DOCTEST_PATH ${CMAKE_SOURCE_DIR}/thirdparty/doctest)
        add_subdirectory(${DOCTEST_PATH})
        include(${DOCTEST_PATH}/scripts/cmake/doctest.cmake)
    endif()
    add_subdirectory(ajantv2/test)
endif()

if (AJANTV2_BUILD_DRIVER)
    add_subdirectory(driver)
endif()

if(AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2)
endif()

