project(ntv2konaipjsonsetup)

aja_require(ntv2konaipjsonsetup AJA_BUILD_QT)

find_package(Qt5 HINTS ${AJA_QT_DIR} COMPONENTS Core Widgets REQUIRED)
set(QT_LIBRARIES Qt5::Core Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(TARGET_LINK_LIBS ${QT_LIBRARIES})

set(NTV2KONAIPJSONSETUP_HEADERS
	konaipjsonparse.h
	konaipjsonsetup.h
	keywords.h)
set(NTV2KONAIPJSONSETUP_SOURCES
	konaipjsonparse.cpp
	main.cpp
	konaipjsonsetup.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3 -msse4.1 -msse4.2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3 -msse4.1 -msse4.2")
	endif()

	set_source_files_properties(Info.plist PROPERTIES MACOSX_PACKAGE_LOCATION Contents)

	find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(CORE_SERVICES_FRAMEWORK CoreServices)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(IOKIT_FRAMEWORK IoKit)

    set(TARGET_LINK_LIBS
		${TARGET_LINK_LIBS}
        ${CORE_FOUNDATION_FRAMEWORK}
		${CORE_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK}
        ${IOKIT_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS
		${TARGET_LINK_LIBS}
		dl pthread rt)
endif()

set(TARGET_SOURCES
	${NTV2KONAIPJSONSETUP_HEADERS}
	${NTV2KONAIPJSONSETUP_SOURCES}
	${DEMO_APPS_HEADERS}
	${DEMO_APPS_SOURCES})

aja_add_executable(ntv2konaipjsonsetup PUBLIC
	"${TARGET_SOURCES}"
	ajantv2
	"${TARGET_INCLUDE_DIRS}"
	"${TARGET_LINK_LIBS};ajantv2" "")
set(BUILD_TARGETS ntv2konaipjsonsetup)
if (AJA_BUILD_SHARED)
	set(BUILD_TARGETS ${BUILD_TARGETS} ntv2konaipjsonsetup-dl)
	aja_add_executable(ntv2konaipjsonsetup-dl PUBLIC
		"${TARGET_SOURCES}"
		ajantv2-dl
		"${TARGET_INCLUDE_DIRS}"
		"${TARGET_LINK_LIBS};ajantv2-dl" "")
endif()
if (AJA_CODE_SIGN)
    aja_code_sign(${BUILD_TARGETS})
endif()
if (AJA_DEPLOY_QT_LIBS)
    include(AJAQtHelpers)
    aja_deploy_qt_libs(ntv2konaipjsonsetup)
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
	install(FILES ${NTV2KONAIPJSONSETUP_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES ${NTV2KONAIPJSONSETUP_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
endif()
if (AJA_INSTALL_MISC)
	install(FILES Makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_12.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_6_2p2.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_6rx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_6tx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_7rx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_7tx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES ntv2konaipjsonsetup.pro DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES t21102rx_2tx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES t2110_combo-1.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
	install(FILES t2110_combo-2.json DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2konaipjsonsetup)
endif()
