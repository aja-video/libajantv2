project(ntv2llburn)

set(NTV2LLBURN_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(AJANTV2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajantv2)
set(NTV2LLBURN_TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajantv2/includes
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajaretail/includes
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries/ajaretail/services)

set(NTV2LLBURN_HEADERS
	ntv2llburn.h)
set(NTV2LLBURN_SOURCES
	ntv2llburn.cpp
	main.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(-DMSWindows -DAJA_WINDOWS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_definitions(-DAJAMac -DAJA_MAC)
        find_library(FOUNDATION_FRAMEWORK Foundation)
        set(EXTRA_LIBRARIES ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_definitions(-DAJALinux -DAJA_LINUX)
	set(EXTRA_LIBRARIES dl pthread rt)
endif()
set(NTV2LLBURN_TARGET_SOURCES
	${NTV2LLBURN_SOURCES}
	${NTV2LLBURN_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2llburn ${NTV2LLBURN_TARGET_SOURCES})
target_include_directories(ntv2llburn PUBLIC ${NTV2LLBURN_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2llburn PUBLIC ajantv2 ${EXTRA_LIBRARIES})
