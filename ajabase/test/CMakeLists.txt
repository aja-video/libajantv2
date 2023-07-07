project(ut_ajabase)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${LIBAJANTV2_DIR}
	${THIRDPARTY_DIR}/doctest/doctest
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

set(TARGET_SOURCES main.cpp)

add_executable(ut_ajabase ${TARGET_SOURCES})
add_dependencies(ut_ajabase ajantv2)
target_compile_definitions(ut_ajabase PRIVATE ${TARGET_COMPILE_DEFS})
target_include_directories(ut_ajabase PRIVATE ${TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ajabase PRIVATE ajantv2 ${TARGET_LINK_LIBS})
