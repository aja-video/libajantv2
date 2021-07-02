project(ntv2encodehevc)

set(NTV2ENCODEHEVC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(AJALIBRARIES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ajalibraries)
set(DEMO_COMMON_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../)
set(AJANTV2_ROOT ${AJALIBRARIES_ROOT}/ajantv2)
set(HEVC_M31_CODEC_ROOT ${AJANTV2_ROOT}/codecs/hevc/m31)

set(HEVC_M31_HEADERS
	${HEVC_M31_CODEC_ROOT}/ntv2m31.h
	${HEVC_M31_CODEC_ROOT}/ntv2m31cparam.h
	${HEVC_M31_CODEC_ROOT}/ntv2m31ehparam.h
	${HEVC_M31_CODEC_ROOT}/ntv2m31vaparam.h
	${HEVC_M31_CODEC_ROOT}/ntv2m31vinparam.h
	${HEVC_M31_CODEC_ROOT}/ntv2m31viparam.h)

set(HEVC_M31_SOURCES
	${HEVC_M31_CODEC_ROOT}/ntv2m31.cpp
	${HEVC_M31_CODEC_ROOT}/ntv2m31cparam.cpp
	${HEVC_M31_CODEC_ROOT}/ntv2m31ehparam.cpp
	${HEVC_M31_CODEC_ROOT}/ntv2m31vaparam.cpp
	${HEVC_M31_CODEC_ROOT}/ntv2m31vinparam.cpp
	${HEVC_M31_CODEC_ROOT}/ntv2m31viparam.cpp)

set(NTV2ENCODEHEVC_TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${AJALIBRARIES_ROOT}
	${AJANTV2_ROOT}/includes
	${HEVC_M31_CODEC_ROOT}
	${AJALIBRARIES_ROOT}/ajaretail/includes
	${AJALIBRARIES_ROOT}/ajaretail/services)

set(NTV2ENCODEHEVC_HEADERS
	${HEVC_M31_HEADERS}
	${DEMO_COMMON_ROOT}/ntv2demohevccommon.h
	ntv2encodehevc.h)
set(NTV2ENCODEHEVC_SOURCES
	${HEVC_M31_SOURCES}
	${DEMO_COMMON_ROOT}/ntv2demohevccommon.cpp
	ntv2encodehevc.cpp
	main.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(-DMSWindows -DAJA_WINDOWS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_definitions(-DAJAMac -DAJA_MAC)

	find_library(CORE_SERVICES_FRAMEWORK CoreServices)
    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(IOKIT_FRAMEWORK IoKit)

	set(EXTRA_LIBRARIES
		${CORE_SERVICES_FRAMEWORK}
		${APPLICATION_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK}
		${IOKIT_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_definitions(-DAJALinux -DAJA_LINUX)
	set(EXTRA_LIBRARIES dl pthread rt)
endif()
set(NTV2ENCODEHEVC_TARGET_SOURCES
	${NTV2ENCODEHEVC_SOURCES}
	${NTV2ENCODEHEVC_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2encodehevc ${NTV2ENCODEHEVC_TARGET_SOURCES})
target_include_directories(ntv2encodehevc PUBLIC ${NTV2ENCODEHEVC_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2encodehevc PUBLIC ajantv2 ${EXTRA_LIBRARIES})
