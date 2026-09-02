project(ut_ajacc)

set(TARGET_INCLUDE_DIRS
	${LIBAJANTV2_DIR}/thirdparty/doctest/doctest
	${LIBAJANTV2_DIR}/ajantv2
	${LIBAJANTV2_DIR}/ajantv2/includes)

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

if (NOT TARGET ut_ajacc)
	add_executable(ut_ajacc main.cpp)
	add_dependencies(ut_ajacc ajacc ajantv2)
	target_compile_definitions(ut_ajacc PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
	target_include_directories(ut_ajacc PUBLIC ${TARGET_INCLUDE_DIRS})
	target_link_libraries(ut_ajacc PUBLIC ajacc ajantv2 ${TARGET_LINK_LIBS})
endif()
