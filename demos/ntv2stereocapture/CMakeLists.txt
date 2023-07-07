project(ntv2stereocapture)

include(QtHelpers)
aja_find_qt_modules(Core Widgets)

list(APPEND TARGET_LINK_LIBS ${TARGET_QT_LIBS} ajantv2)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../ajantv2/includes)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	list(APPEND TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND TARGET_LINK_LIBS dl pthread rt)
endif()

set(DEMO_APP_SOURCES
	ntv2stereocapture.h
	ntv2stereocapture.cpp
)
set(DEMO_COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h
)
set(TARGET_SOURCES
	${DEMO_APP_SOURCES}
	${DEMO_COMMON_SOURCES}
)


if (AJA_BUILD_SHARED)
	set(BUILD_TARGETS ${BUILD_TARGETS} ntv2stereocapture-dl)
    add_executable(${PROJECT_NAME}-dl PUBLIC ${TARGET_SOURCES})
    add_dependencies(ajantv2-dl)
    target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LINK_LIBS})
else()
    set(BUILD_TARGETS ${PROJECT_NAME})
    add_executable(${PROJECT_NAME} PUBLIC ${TARGET_SOURCES})
    add_dependencies(ajantv2)
    target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LINK_LIBS})
endif()

if (AJA_CODE_SIGN)
    aja_code_sign(${BUILD_TARGETS})
endif()
install(TARGETS ${BUILD_TARGETS}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_COMMON_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos)
	install(FILES ${DEMO_APP_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2stereocapture)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2stereocapture)
endif()
if (AJA_INSTALL_MISC)
	install(FILES Makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2stereocapture)
endif()
