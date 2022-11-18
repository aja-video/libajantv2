project(ntv2oglcapture)

aja_require(ntv2oglcapture AJA_BUILD_QT)

find_package(OpenGL REQUIRED)
set(OPENGL_LIBRARIES OpenGL::GLU)

find_package(Qt5 HINTS ${AJA_QT_DIR} COMPONENTS Core OpenGL Widgets REQUIRED)
set(QT_LIBRARIES Qt5::Core Qt5::OpenGL Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_GPUSTUFF_ROOT}/include
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(TARGET_LINK_LIBS
	${QT_LIBRARIES}
	${OPENGL_LIBRARIES})

set(NTV2OGLCAPTURE_HEADERS
	ntv2oglcapture.h)
set(NTV2OGLCAPTURE_SOURCES
	ntv2oglcapture.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	find_library(OPENGL_FRAMEWORK OpenGL)
	set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} ${FOUNDATION_FRAMEWORK} ${OPENGL_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} dl pthread rt)
endif()

set(TARGET_SOURCES
	${NTV2OGLCAPTURE_HEADERS}
	${NTV2OGLCAPTURE_SOURCES}
	${DEMO_APPS_HEADERS}
	${DEMO_APPS_SOURCES})

aja_add_executable(ntv2oglcapture PUBLIC
	"${TARGET_SOURCES}"
	ajantv2
	"${TARGET_INCLUDE_DIRS}"
	"${TARGET_LINK_LIBS};ajantv2" "")
set(BUILD_TARGETS ntv2oglcapture)
if (AJA_BUILD_SHARED)
	set(BUILD_TARGETS ${BUILD_TARGETS} ntv2oglcapture-dl)
	aja_add_executable(ntv2oglcapture-dl PUBLIC
		"${TARGET_SOURCES}"
		ajantv2-dl
		"${TARGET_INCLUDE_DIRS}"
		"${TARGET_LINK_LIBS};ajantv2-dl" "")
endif()
if (AJA_CODE_SIGN)
    aja_code_sign(${BUILD_TARGETS})
endif()
if (AJA_DEPLOY_LIBS)
    include(AJAQtHelpers)
    aja_deploy_qt_libs(ntv2oglcapture)
endif()
install(TARGETS ${BUILD_TARGETS}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (AJA_INSTALL_SOURCES)
	install(FILES ${DEMO_APPS_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${DEMO_APPS_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps)
	install(FILES ${NTV2OGLCAPTURE_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2oglcapture)
	install(FILES ${NTV2OGLCAPTURE_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2oglcapture)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/ntv2oglcapture)
endif()
