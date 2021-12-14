project(ut_ajantv2)

message("added ajantv2 unit tests")

set(APP_TARGET_INCLUDE_DIRS
	${APP_ROOT}/..
	${AJA_LIBRARIES_ROOT}
	${NON_AJA_ROOT})

set(APP_SOURCES
	main.cpp)

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

set(UT_AJANTV2_SOURCES
	main.cpp)
set(UT_NTV2CARD_SOURCES
	${NON_AJA_ROOT}/argparse/argparse.h
	${NON_AJA_ROOT}/argparse/argparse.c
	auto_router.h
	auto_router.cpp
	ut_ntv2card.cpp)

add_executable(ut_ajantv2 ${UT_AJANTV2_SOURCES})
add_dependencies(ut_ajantv2 ajantv2)
target_include_directories(ut_ajantv2 PUBLIC ${APP_TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ajantv2 PUBLIC ajantv2 ${EXTRA_LIBRARIES})

add_executable(ut_ntv2card ${UT_NTV2CARD_SOURCES})
add_dependencies(ut_ntv2card ajantv2 ntv2qa)
target_include_directories(ut_ntv2card PUBLIC
	${APP_TARGET_INCLUDE_DIRS}
	${AJA_QA_ROOT}/libs)
target_link_libraries(ut_ntv2card PUBLIC ajantv2 ntv2qa ${EXTRA_LIBRARIES})
