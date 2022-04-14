project(ntv2recorddpx)

set(TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2RECORDDPX_HEADERS
	ntv2recorddpx.h)
set(NTV2RECORDDPX_SOURCES
	ntv2recorddpx.cpp)

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

set(NTV2RECORDDPX_TARGET_SOURCES
	${NTV2RECORDDPX_SOURCES}
	${NTV2RECORDDPX_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2recorddpx ${NTV2RECORDDPX_TARGET_SOURCES})
add_dependencies(ntv2recorddpx ajantv2)
target_include_directories(ntv2recorddpx PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2recorddpx PUBLIC ajantv2 ${TARGET_LINK_LIBS})

if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_APPS_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${DEMO_APPS_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${NTV2CUDACAPTURE_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2recorddpx)
	install(FILES ${NTV2CUDACAPTURE_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2recorddpx)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2recorddpx)
endif()

install(TARGETS ntv2recorddpx
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if (AJA_CODE_SIGN)
    aja_code_sign(ntv2recorddpx)
endif()
