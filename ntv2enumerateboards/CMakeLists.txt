project(ntv2enumerateboards)

message("added ntv2enumerateboards")

set(TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2ENUMERATEBOARDS_HEADERS
	ntv2enumerateboards.h)
set(NTV2ENUMERATEBOARDS_SOURCES
	ntv2enumerateboards.cpp
	main.cpp)

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
	${NTV2ENUMERATEBOARDS_SOURCES}
	${NTV2ENUMERATEBOARDS_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2enumerateboards ${TARGET_SOURCES})
add_dependencies(ntv2enumerateboards ajantv2)
target_include_directories(ntv2enumerateboards PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2enumerateboards PUBLIC ajantv2 ${TARGET_LINK_LIBS})

if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_APPS_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${DEMO_APPS_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${NTV2ENUMERATEBOARDS_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2enumerateboards)
	install(FILES ${NTV2ENUMERATEBOARDS_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2enumerateboards)
endif()

install(TARGETS ntv2enumerateboards
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX})
