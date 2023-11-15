project(gtest)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../ajantv2/includes)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
elseif (CMAKE_SYSTEM_NAME STREQUAL "BareMetal")
  # noop
endif()

set(DEMO_APP_SOURCES
	main.cpp
  gtest.cpp
)
set(DEMO_COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h
)
set(TARGET_SOURCES
	${DEMO_APP_SOURCES}
	${DEMO_COMMON_SOURCES}
)

add_executable(${PROJECT_NAME} ${TARGET_SOURCES})
add_dependencies(${PROJECT_NAME} ajantv2)
target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LINK_LIBS} ajantv2)

