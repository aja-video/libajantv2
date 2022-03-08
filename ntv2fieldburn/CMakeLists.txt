project(ntv2fieldburn)

message("added ntv2fieldburn")

set(NTV2FIELDBURN_TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2FIELDBURN_HEADERS
	ntv2fieldburn.h)
set(NTV2FIELDBURN_SOURCES
	ntv2fieldburn.cpp
	main.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(EXTRA_LIBRARIES ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(EXTRA_LIBRARIES dl pthread rt)
endif()

set(NTV2FIELDBURN_TARGET_SOURCES
	${NTV2FIELDBURN_SOURCES}
	${NTV2FIELDBURN_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2fieldburn ${NTV2FIELDBURN_TARGET_SOURCES})
add_dependencies(ntv2fieldburn ajantv2)
target_include_directories(ntv2fieldburn PUBLIC ${NTV2FIELDBURN_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2fieldburn PUBLIC ajantv2 ${EXTRA_LIBRARIES})

if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_APPS_HEADERS} DESTINATION ${AJA_INSTALL_INCLUDEDIR}/ajaapps/crossplatform/demoapps)
	install(FILES ${DEMO_APPS_SOURCES} DESTINATION ${AJA_INSTALL_INCLUDEDIR}/ajaapps/crossplatform/demoapps)
	install(FILES ${NTV2FIELDBURN_HEADERS} DESTINATION ${AJA_INSTALL_INCLUDEDIR}/ajaapps/crossplatform/demoapps/ntv2fieldburn)
	install(FILES ${NTV2FIELDBURN_SOURCES} DESTINATION ${AJA_INSTALL_INCLUDEDIR}/ajaapps/crossplatform/demoapps/ntv2fieldburn)
endif()

install(TARGETS ntv2fieldburn
	ARCHIVE DESTINATION ${AJA_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${AJA_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${AJA_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${AJA_INSTALL_FRAMEWORKDIR})
