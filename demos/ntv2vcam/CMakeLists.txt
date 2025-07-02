# SPDX-License-Identifier: MIT
#
#    @file		CMakeLists.txt
#    @brief		CMake build instructions for NTV2VCAM demo.
#    @copyright	(C) 2025 AJA Video Systems, Inc.  All rights reserved.
#

project(ntv2vcam)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../ajantv2/includes)

set(DEMO_APP_SOURCES
    main.cpp
    ntv2vcam.cpp
    ntv2vcam.h
)

set(DEMO_COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND DEMO_APP_SOURCES v4l2loopback.h)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA REQUIRED alsa)

    list(APPEND TARGET_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})

    link_directories(${ALSA_LIBRARY_DIRS})
    list(APPEND TARGET_LINK_LIBS ${ALSA_LIBRARIES} rt pthread dl)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND DEMO_APP_SOURCES ntv2vcam.def)
    add_subdirectory(baseclasses)

    list(APPEND TARGET_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/baseclasses)

    list(APPEND TARGET_LINK_LIBS BaseClasses strmiids winmm ole32 oleaut32 uuid)
endif()

set(TARGET_SOURCES
	${DEMO_APP_SOURCES}
	${DEMO_COMMON_SOURCES}
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(${PROJECT_NAME} ${TARGET_SOURCES})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_library(${PROJECT_NAME} SHARED ${TARGET_SOURCES})
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LINK_LIBS} ajantv2)

if (AJA_CODE_SIGN)
    aja_code_sign(${PROJECT_NAME})
endif()

install(TARGETS ${PROJECT_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_COMMON_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos)
	install(FILES ${DEMO_APP_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2vcam)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2vcam)
endif()
if (AJA_INSTALL_MISC)
	install(FILES Makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2vcam)
endif()
