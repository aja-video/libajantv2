project(ntv2konaipjsonsetup)

if (NOT AJA_QT_ENABLED)
	message(STATUS "AJA_QT_ENABLED not set. Ignoring project ${PROJECT_NAME}...")
    return()
endif()

find_package(Qt5 HINTS ${AJA_QT_DIR} COMPONENTS Core Widgets REQUIRED)
set(QT_LIBRARIES Qt5::Core Qt5::Widgets)

list(APPEND TARGET_LINK_LIBS ${QT_LIBRARIES})

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../ajantv2/includes)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3 -msse4.1 -msse4.2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3 -msse4.1 -msse4.2")
	endif()

	set_source_files_properties(Info.plist PROPERTIES MACOSX_PACKAGE_LOCATION Contents)

	find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
	find_library(CORE_SERVICES_FRAMEWORK CoreServices)
	find_library(FOUNDATION_FRAMEWORK Foundation)
	find_library(IOKIT_FRAMEWORK IoKit)

	list(APPEND TARGET_LINK_LIBS
		${CORE_FOUNDATION_FRAMEWORK}
		${CORE_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK}
		${IOKIT_FRAMEWORK}
	)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND TARGET_LINK_LIBS dl pthread rt)
endif()

set(DEMO_APP_SOURCES
	keywords.h
	konaipjsonparse.h
	konaipjsonparse.cpp
	konaipjsonsetup.h
	konaipjsonsetup.cpp
	main.cpp
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
	install(FILES ${DEMO_COMMON_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/demos)
	install(FILES ${DEMO_APP_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
endif()
if (AJA_INSTALL_MISC)
	install(FILES Makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_12.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_6_2p2.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_6rx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_6tx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_7rx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES konaipsetup2022_7tx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES ntv2konaipjsonsetup.pro DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES t21102rx_2tx.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES t2110_combo-1.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
	install(FILES t2110_combo-2.json DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2konaipjsonsetup)
endif()
