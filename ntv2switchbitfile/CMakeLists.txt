project(ntv2switchbitfile)

set(NTV2SWITCHBITFILE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${AJA_LIBRARIES_ROOT}
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2SWITCHBITFILE_SOURCES
	main.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_library(FOUNDATION_FRAMEWORK Foundation)
    set(TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
endif()

set(TARGET_SOURCES
	${NTV2SWITCHBITFILE_SOURCES}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2switchbitfile ${TARGET_SOURCES})
add_dependencies(ntv2switchbitfile ajantv2)
target_include_directories(ntv2switchbitfile PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2switchbitfile PUBLIC ajantv2 ${TARGET_LINK_LIBS})

if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_APPS_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${DEMO_APPS_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${NTV2SWITCHBITFILE_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2switchbitfile)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2switchbitfile)
endif()

install(TARGETS ntv2switchbitfile
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if (AJA_CODE_SIGN)
    aja_code_sign(ntv2switchbitfile)
endif()
