project(ntv2encodehevc)

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

set(TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services
	${HEVC_M31_CODEC_ROOT})

set(NTV2ENCODEHEVC_HEADERS
	${HEVC_M31_HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2demohevccommon.h
	ntv2encodehevc.h)
set(NTV2ENCODEHEVC_SOURCES
	${HEVC_M31_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2demohevccommon.cpp
	ntv2encodehevc.cpp
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

set(TARGET_SOURCES
	${NTV2ENCODEHEVC_SOURCES}
	${NTV2ENCODEHEVC_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

aja_add_executable(ntv2encodehevc PUBLIC
	"${TARGET_SOURCES}"
	ajantv2
	"${TARGET_INCLUDE_DIRS}"
	"${TARGET_LINK_LIBS};ajantv2" "")
set(BUILD_TARGETS ntv2encodehevc)
if (AJA_BUILD_SHARED)
	set(BUILD_TARGETS ${BUILD_TARGETS} ntv2encodehevc-dl)
	aja_add_executable(ntv2encodehevc-dl PUBLIC
		"${TARGET_SOURCES}"
		ajantv2-dl
		"${TARGET_INCLUDE_DIRS}"
		"${TARGET_LINK_LIBS};ajantv2-dl" "")
endif()
if (AJA_CODE_SIGN)
    aja_code_sign(${BUILD_TARGETS})
endif()
install(TARGETS ${BUILD_TARGETS}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_APPS_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${DEMO_APPS_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${NTV2CUDACAPTURE_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2encodehevc)
	install(FILES ${NTV2CUDACAPTURE_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2encodehevc)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2encodehevc)
endif()
