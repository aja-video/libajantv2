project(ntv2playbackdpx)

message("added ntv2playbackdpx")

set(NTV2PLAYBACKDPX_TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2PLAYBACKDPX_HEADERS
	ntv2playbackdpx.h)
set(NTV2PLAYBACKDPX_SOURCES
	ntv2playbackdpx.cpp)

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

set(NTV2PLAYBACKDPX_TARGET_SOURCES
	${NTV2PLAYBACKDPX_SOURCES}
	${NTV2PLAYBACKDPX_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2playbackdpx ${NTV2PLAYBACKDPX_TARGET_SOURCES})
add_dependencies(ntv2playbackdpx ajantv2)
target_include_directories(ntv2playbackdpx PUBLIC ${NTV2PLAYBACKDPX_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2playbackdpx PUBLIC ajantv2 ${EXTRA_LIBRARIES})
