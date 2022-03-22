project(ntv2cudacapture)

message("added ntv2cudacapture")

find_package(Qt5 HINTS ${AJA_QT_DIR} COMPONENTS
    Core
    REQUIRED)

set(QT_LIBRARIES
    Qt5::Core)

set(TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2CUDACAPTURE_HEADERS
	ntv2cudacapture.h)
set(NTV2CUDACAPTURE_SOURCES
	ntv2cudacapture.cpp)

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
	set(TARGET_LINK_LIBS cuda cudart dl pthread rt)
endif()

set(TARGET_SOURCES
	${NTV2CUDACAPTURE_SOURCES}
	${NTV2CUDACAPTURE_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2cudacapture ${TARGET_SOURCES})
add_dependencies(ntv2cudacapture ajantv2)
target_include_directories(ntv2cudacapture PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2cudacapture PUBLIC ajantv2 ${TARGET_LINK_LIBS} ${QT_LIBRARIES})

install(TARGETS ntv2cudacapture
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX})
