project(ntv2cudacapture)

if (NOT AJA_QT_ENABLED)
	message(STATUS "AJA_QT_ENABLED not set. Ignoring project ${PROJECT_NAME}...")
    return()
endif()

include(QtHelpers)
aja_find_qt_modules(Core)

list(APPEND TARGET_LINK_LIBS ${TARGET_QT_LIBS})

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
	ntv2cudacapture.h
	ntv2cudacapture.cpp
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

if (AJA_CODE_SIGN)
    aja_code_sign(${PROJECT_NAME})
endif()

install(TARGETS ${PROJECT_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_COMMON_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos)
	install(FILES ${DEMO_APP_SOURCES} 	 DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2cudacapture)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2cudacapture)
endif()
