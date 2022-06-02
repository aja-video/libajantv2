project(ut_ajaanc)

set(TARGET_INCLUDE_DIRS
	${APP_ROOT}/..
	${AJALIBRARIES_ROOT}
	${NON_AJA_ROOT})

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

set(APP_TARGET_SOURCES
	main.cpp)

add_executable(ut_ajaanc ${APP_TARGET_SOURCES})
add_dependencies(ut_ajaanc ajacc ajantv2)
target_include_directories(ut_ajaanc PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(ut_ajaanc PUBLIC ajacc ajantv2 ${TARGET_LINK_LIBS})
