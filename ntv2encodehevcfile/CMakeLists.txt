project(ntv2encodehevcfile)

message("added ntv2encodehevcfile")

set(NTV2ENCODEHEVCFILE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(HEVC_M31_CODEC_ROOT ${AJA_LIB_NTV2_ROOT}/codecs/hevc/m31)

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

set(NTV2ENCODEHEVCFILE_TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services
	${HEVC_M31_CODEC_ROOT})

set(NTV2ENCODEHEVCFILE_HEADERS
	${HEVC_M31_HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2demohevccommon.h
	ntv2encodehevcfileac.h
	ntv2encodehevcfile.h)
set(NTV2ENCODEHEVCFILE_SOURCES
	${HEVC_M31_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2demohevccommon.cpp
	ntv2encodehevcfile.cpp
	ntv2encodehevcfileac.cpp
	main.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(CORE_SERVICES_FRAMEWORK CoreServices)
    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(IOKIT_FRAMEWORK IoKit)

	set(TARGET_LINK_LIBS
		${CORE_SERVICES_FRAMEWORK}
		${APPLICATION_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK}
		${IOKIT_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
endif()

set(NTV2ENCODEHEVCFILE_TARGET_SOURCES
	${NTV2ENCODEHEVCFILE_SOURCES}
	${NTV2ENCODEHEVCFILE_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2encodehevcfile ${NTV2ENCODEHEVCFILE_TARGET_SOURCES})
add_dependencies(ntv2encodehevcfile ajantv2)
target_include_directories(ntv2encodehevcfile PUBLIC ${NTV2ENCODEHEVCFILE_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2encodehevcfile PUBLIC ajantv2 ${TARGET_LINK_LIBS})

install(TARGETS ntv2encodehevcfile
	ARCHIVE DESTINATION ${AJA_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${AJA_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${AJA_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${AJA_INSTALL_FRAMEWORKDIR})
