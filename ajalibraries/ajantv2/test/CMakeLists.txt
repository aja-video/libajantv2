project(ut_ajantv2)

message("added ajantv2 unit tests")

set(APP_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(AJALIBRARIES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../ajalibraries)
set(AJA_DOCTEST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../non-aja/doctest)

set(APP_TARGET_INCLUDE_DIRS
	${APP_ROOT}/..
	${AJALIBRARIES_ROOT}
	${AJA_DOCTEST_ROOT})

set(APP_HEADERS ${AJA_DOCTEST_ROOT}/doctest.h)

set(APP_SOURCES ${APP_ROOT}/main.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
	find_library(CORE_SERVICES_FRAMEWORK CoreServices)
	find_library(FOUNDATION_FRAMEWORK Foundation)
	
	set(EXTRA_LIBRARIES
		${CORE_FOUNDATION_FRAMEWORK}
		${CORE_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(EXTRA_LIBRARIES dl pthread rt)
endif()

set(APP_TARGET_SOURCES
	${APP_HEADERS}
	${APP_SOURCES})

add_executable(ut_ajantv2 ${APP_TARGET_SOURCES})
add_dependencies(ut_ajantv2 ajantv2)
target_include_directories(ut_ajantv2 PUBLIC ${APP_TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ajantv2 PUBLIC ajantv2 ${EXTRA_LIBRARIES})

add_executable(ut_ntv2card ntv2card_test.cpp)
add_dependencies(ut_ntv2card ajantv2)
target_include_directories(ut_ntv2card PUBLIC ${APP_TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ntv2card PUBLIC ajantv2 ${EXTRA_LIBRARIES})
