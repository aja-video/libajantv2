project(ut_ajabase)

set(TEST_XML_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(DOCTEST_INCLUDE_DIR ${LIBAJANTV2_DIR}/thirdparty/doctest/doctest)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	../ajantv2
	../ajantv2/includes
	${DOCTEST_INCLUDE_DIR}
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(TARGET_COMPILE_DEFS
		-DNOMINMAX)
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

if (NOT TARGET ut_ajabase)
	add_executable(ut_ajabase main.cpp ${DOCTEST_INCLUDE_DIR}/doctest.h)
	add_dependencies(ut_ajabase ajantv2)
	target_compile_definitions(ut_ajabase PRIVATE ${TARGET_COMPILE_DEFS})
	target_include_directories(ut_ajabase PRIVATE ${TARGET_INCLUDE_DIRS})
	target_link_libraries(ut_ajabase PRIVATE ajantv2 ${TARGET_LINK_LIBS})
	# doctest_discover_tests(${PROJECT_NAME} JUNIT_OUTPUT_DIR ${TEST_XML_DIR})
endif()
