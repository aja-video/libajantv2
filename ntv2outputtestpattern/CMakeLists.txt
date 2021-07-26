project(ntv2outputtestpattern)

set(NTV2OUTPUTTESTPATTERN_TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2OUTPUTTESTPATTERN_HEADERS
	ntv2outputtestpattern.h)
set(NTV2OUTPUTTESTPATTERN_SOURCES
	ntv2outputtestpattern.cpp
	main.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(EXTRA_LIBRARIES ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(EXTRA_LIBRARIES dl pthread rt)
endif()

set(NTV2OUTPUTTESTPATTERN_TARGET_SOURCES
	${NTV2OUTPUTTESTPATTERN_SOURCES}
	${NTV2OUTPUTTESTPATTERN_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2outputtestpattern ${NTV2OUTPUTTESTPATTERN_TARGET_SOURCES})
target_include_directories(ntv2outputtestpattern PUBLIC ${NTV2OUTPUTTESTPATTERN_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2outputtestpattern PUBLIC ajantv2 ${EXTRA_LIBRARIES})
