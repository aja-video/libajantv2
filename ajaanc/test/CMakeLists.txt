project(ut_ajaanc)

set(TEST_XML_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DOCTEST_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/doctest/doctest)

set(TARGET_INCLUDE_DIRS
	${DOCTEST_INCLUDE_DIR}
	../../ajantv2
	../../ajantv2/includes
	${LIBAJACC_DIR}/includes
)

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

if (NOT TARGET ut_ajaanc)
	add_executable(ut_ajaanc main.cpp ${DOCTEST_INCLUDE_DIR}/doctest.h)
	add_dependencies(ut_ajaanc ajacc ajantv2)
	target_include_directories(ut_ajaanc PUBLIC ${TARGET_INCLUDE_DIRS})
	target_link_libraries(ut_ajaanc PUBLIC ajacc ajantv2 ${TARGET_LINK_LIBS})
endif()
