project(ut_ajabase)

message("added ajabase unit tests")

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

add_executable(ut_ajabase ${APP_TARGET_SOURCES})
add_dependencies(ut_ajabase ajantv2)
target_include_directories(ut_ajabase PUBLIC ${APP_TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ajabase PUBLIC ajantv2 ${EXTRA_LIBRARIES})
