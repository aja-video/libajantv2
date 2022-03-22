project(ntv2playcorder)

message("added ntv2playcorder")

set(TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2PLAYCORDER_HEADERS
	ntv2playcorder.h
	iplaycorder.h)
set(NTV2PLAYCORDER_SOURCES
	ntv2playcorder.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
endif()

set(TARGET_SOURCES
	${NTV2PLAYCORDER_SOURCES}
	${NTV2PLAYCORDER_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2playcorder ${TARGET_SOURCES})
add_dependencies(ntv2playcorder ajantv2)
target_include_directories(ntv2playcorder PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2playcorder PUBLIC ajantv2 ${TARGET_LINK_LIBS})

install(TARGETS ntv2playcorder
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX})
