project(ut_ajantv2)

include(AJAHelpers)

message("added ajantv2 unit tests")

set(TARGET_INCLUDE_DIRS
	${APP_ROOT}/..
	${AJA_LIBRARIES_ROOT}
	${NON_AJA_ROOT})

set(ARGPARSE_SOURCES
	${NON_AJA_ROOT}/argparse/argparse.h
	${NON_AJA_ROOT}/argparse/argparse.c)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
	find_library(CORE_SERVICES_FRAMEWORK CoreServices)
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(TARGET_LINK_LIBS
		${CORE_FOUNDATION_FRAMEWORK}
		${CORE_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
endif()

add_executable(ut_ajantv2
	ut_ajantv2.cpp)
add_dependencies(ut_ajantv2 ajantv2)
target_include_directories(ut_ajantv2 PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ajantv2 PUBLIC ajantv2 ${TARGET_LINK_LIBS})

add_executable(ut_ntv2card
	${ARGPARSE_SOURCES}
	ut_ntv2card.cpp
	test_support.hpp)
add_dependencies(ut_ntv2card ajantv2 ntv2qa)
target_include_directories(ut_ntv2card PRIVATE
	${TARGET_INCLUDE_DIRS}
	${AJA_QA_ROOT}/libs)
target_link_libraries(ut_ntv2card PRIVATE ajantv2 ntv2qa ${TARGET_LINK_LIBS})
set(UT_NTV2CARD_JSON
	json/sdi_ycbcr8.json
	json/sdi_ycbcr10.json)
foreach(j IN LISTS UT_NTV2CARD_JSON)
	post_build_copy_file(ut_ntv2card
		${CMAKE_CURRENT_SOURCE_DIR}/${j}
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/${j})
endforeach()

add_executable(ut_ntv2bitfile
	${ARGPARSE_SOURCES}
	ut_ntv2bitfile.cpp
	test_support.hpp)
add_dependencies(ut_ntv2bitfile ajantv2)
target_include_directories(ut_ntv2bitfile PRIVATE ${TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ntv2bitfile PRIVATE ajantv2 ${TARGET_LINK_LIBS})
post_build_copy_file(ut_ntv2bitfile
	${CMAKE_CURRENT_SOURCE_DIR}/json/ntv2_classic_firmware.json
	$<TARGET_FILE_DIR:${PROJECT_NAME}>/json/ntv2_classic_firmware.json)
