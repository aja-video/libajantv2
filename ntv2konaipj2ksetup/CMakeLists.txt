project(ntv2konaipj2ksetup)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
set(QT_LIBRARIES Qt5::Core Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(NTV2KONAIPJ2KSETUP_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(AJANTV2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajantv2)
set(NTV2KONAIPJ2KSETUP_TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajantv2/includes
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajaretail/includes
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajaretail/services)

set(NTV2KONAIPJ2KSETUP_HEADERS
	konaipj2ksetup.h)
set(NTV2KONAIPJ2KSETUP_SOURCES
	konaipj2ksetup.cpp
	main.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(-DMSWindows -DAJA_WINDOWS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_definitions(-DAJAMac -DAJA_MAC)

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
set(NTV2KONAIPJ2KSETUP_TARGET_SOURCES
	${NTV2KONAIPJ2KSETUP_SOURCES}
	${NTV2KONAIPJ2KSETUP_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2konaipj2ksetup ${NTV2KONAIPJ2KSETUP_TARGET_SOURCES})
target_include_directories(ntv2konaipj2ksetup PUBLIC ${NTV2KONAIPJ2KSETUP_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2konaipj2ksetup PUBLIC ajantv2 ${EXTRA_LIBRARIES} ${QT_LIBRARIES})
