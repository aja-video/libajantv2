project(ntv2konaipjsonsetup)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
set(QT_LIBRARIES Qt5::Core Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(NTV2KONAIPJSONSETUP_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(AJANTV2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajantv2)
set(NTV2KONAIPJSONSETUP_TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajantv2/includes
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajaretail/includes
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajaretail/services)

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
	add_definitions(-DMSWindows -DAJA_WINDOWS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_definitions(-DAJAMac -DAJA_MAC)

	if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -msse3 -msse4.1 -msse4.2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -msse3 -msse4.1 -msse4.2")
	endif()

	set_source_files_properties(Info.plist PROPERTIES MACOSX_PACKAGE_LOCATION Contents)

	find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(CORE_SERVICES_FRAMEWORK CoreServices)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(IOKIT_FRAMEWORK IoKit)

    set(EXTRA_LIBRARIES
        ${CORE_FOUNDATION_FRAMEWORK}
		${CORE_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK}
        ${IOKIT_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_definitions(-DAJALinux -DAJA_LINUX)
	set(EXTRA_LIBRARIES dl pthread rt)
endif()
set(NTV2KONAIPJSONSETUP_TARGET_SOURCES
	${NTV2KONAIPJSONSETUP_SOURCES}
	${NTV2KONAIPJSONSETUP_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2konaipjsonsetup ${NTV2KONAIPJSONSETUP_TARGET_SOURCES})
target_include_directories(ntv2konaipjsonsetup PUBLIC ${NTV2KONAIPJSONSETUP_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2konaipjsonsetup PUBLIC ajantv2 ${EXTRA_LIBRARIES} ${QT_LIBRARIES})
