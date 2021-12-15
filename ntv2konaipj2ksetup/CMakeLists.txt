project(ntv2konaipj2ksetup)

message("added ntv2konaipj2ksetup")

find_package(Qt5 HINTS ${AJA_QT_DIR} COMPONENTS Core Widgets REQUIRED)
set(QT_LIBRARIES Qt5::Core Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(NTV2KONAIPJ2KSETUP_TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2KONAIPJ2KSETUP_HEADERS
	konaipj2ksetup.h)
set(NTV2KONAIPJ2KSETUP_SOURCES
	konaipj2ksetup.cpp
	main.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set_source_files_properties(Info.plist PROPERTIES MACOSX_PACKAGE_LOCATION Contents)

	find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(CORE_SERVICES_FRAMEWORK CoreServices)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(IOKIT_FRAMEWORK IoKit)

    set(EXTRA_LIBRARIES
        ${CORE_FOUNDATION_FRAMEWORK}
		${CORE_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK}
        ${IOKIT_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(EXTRA_LIBRARIES dl pthread rt)
endif()

set(NTV2KONAIPJ2KSETUP_TARGET_SOURCES
	${NTV2KONAIPJ2KSETUP_SOURCES}
	${NTV2KONAIPJ2KSETUP_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2konaipj2ksetup ${NTV2KONAIPJ2KSETUP_TARGET_SOURCES})
add_dependencies(ntv2konaipj2ksetup ajantv2)
target_include_directories(ntv2konaipj2ksetup PUBLIC ${NTV2KONAIPJ2KSETUP_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2konaipj2ksetup PUBLIC ajantv2 ${EXTRA_LIBRARIES} ${QT_LIBRARIES})

if (AJA_DEPLOY_LIBS)
    include(QtHelpers)
    deploy_qt_libs(ntv2konaipj2ksetup)
endif()
