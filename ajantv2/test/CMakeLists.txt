project(ut_ajantv2)

set(TEST_XML_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(TARGET_INCLUDE_DIRS
	${doctest_SOURCE_DIR}/doctest
	../ajantv2
	../ajantv2/includes)

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

if (NOT TARGET ut_ajantv2)
	add_executable(ut_ajantv2 ut_ajantv2.cpp)
	add_dependencies(ut_ajantv2 doctest::doctest ajantv2)
	target_include_directories(ut_ajantv2 PUBLIC doctest::doctest ${TARGET_INCLUDE_DIRS})
	target_link_libraries(ut_ajantv2 PUBLIC ajantv2 doctest::doctest_with_main ${TARGET_LINK_LIBS})
	doctest_discover_tests(${PROJECT_NAME} JUNIT_OUTPUT_DIR ${TEST_XML_DIR})
endif()

if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/ajantv2/test)
endif()
