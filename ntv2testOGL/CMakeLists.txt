project(ntv2testOGL)

message("added ntv2testOGL")

set(NTV2TESTOGL_TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2TESTOGL_SOURCES
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

set(NTV2TESTOGL_TARGET_SOURCES
	${NTV2TESTOGL_SOURCES}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2testOGL ${NTV2TESTOGL_TARGET_SOURCES})
add_dependencies(ntv2testOGL ajantv2)
target_include_directories(ntv2testOGL PUBLIC ${NTV2TESTOGL_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2testOGL PUBLIC ajantv2 ${EXTRA_LIBRARIES})
