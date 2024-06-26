project(ntv2firmwareinstaller)

set(DEMOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../demos)
set(AJANTV2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../ajantv2)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${DEMOS_DIR}
	${AJANTV2_DIR}/includes
	${AJANTV2_DIR}/utilityfiles/public)

set(DEMO_APPS_HEADERS
	${DEMOS_DIR}/ntv2democommon.h)
set(DEMO_APPS_SOURCES
	${DEMOS_DIR}/ntv2democommon.cpp)
set(AJA_NTV2_UTILITY_PUBLIC_SOURCES
	${AJANTV2_DIR}/utilityfiles/public/ntv2firmwareinstallerthread.h
	${AJANTV2_DIR}/utilityfiles/public/ntv2firmwareinstallerthread.cpp)
set(NTV2FIRMWAREINSTALLER_SOURCES
	main.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
endif()

set(TARGET_SOURCES
	${DEMO_APPS_HEADERS}
	${DEMO_APPS_SOURCES}
	${AJA_NTV2_UTILITY_PUBLIC_SOURCES}
	${NTV2FIRMWAREINSTALLER_SOURCES})

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
	install(FILES ${DEMO_APPS_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos)
	install(FILES ${DEMO_APPS_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos)
	install(FILES ${AJA_NTV2_UTILITY_PUBLIC_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/ajantv2/utilityfiles/public)
	install(FILES ${NTV2FIRMWAREINSTALLER_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/tools/ntv2firmwareinstaller)
	install(FILES ${NTV2FIRMWAREINSTALLER_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/tools/ntv2firmwareinstaller)
endif()
if (AJA_INSTALL_MISC)
	install(FILES Makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/tools/ntv2firmwareinstaller)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/tools/ntv2firmwareinstaller)
endif()
