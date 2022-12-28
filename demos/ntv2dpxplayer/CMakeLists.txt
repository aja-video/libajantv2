project(ntv2dpxplayer)

if (NOT AJA_BUILD_QT)
    aja_message(STATUS "AJA_BUILD_QT not set. Ignoring project ${project}...")
    return()
endif()

find_package(Qt5 HINTS ${AJA_QT_DIR} COMPONENTS
    Core
    REQUIRED)

set(QT_LIBRARIES
    Qt5::Core)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2recorddpx
	${AJA_LIBRARIES_ROOT}
	${AJA_LIB_NTV2_ROOT}/includes
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
endif()

set(DEMO_APP_SOURCES
	ntv2dpxplayer.h
	ntv2dpxplayer.cpp
	main.cpp
)
set(DEMO_COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h
)
set(TARGET_SOURCES
	${DEMO_APP_SOURCES}
	${DEMO_COMMON_SOURCES}
)

aja_add_executable(ntv2dpxplayer PUBLIC
	"${TARGET_SOURCES}"
	ajantv2
	"${TARGET_INCLUDE_DIRS}"
	"${TARGET_LINK_LIBS};${QT_LIBRARIES};ajantv2" "")
set(BUILD_TARGETS ntv2dpxplayer)
if (AJA_BUILD_SHARED)
	set(BUILD_TARGETS ${BUILD_TARGETS} ntv2dpxplayer-dl)
	aja_add_executable(ntv2dpxplayer-dl PUBLIC
		"${TARGET_SOURCES}"
		ajantv2-dl
		"${TARGET_INCLUDE_DIRS}"
		"${TARGET_LINK_LIBS};${QT_LIBRARIES};ajantv2-dl" "")
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
	install(FILES ${DEMO_COMMON_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/demos)
	install(FILES ${DEMO_APP_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2dpxplayer)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2dpxplayer)
endif()
if (AJA_INSTALL_MISC)
	install(FILES Makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2dpxplayer)
endif()
