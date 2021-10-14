project(ntv2konaipjsonsetup)

message("added ntv2konaipjsonsetup")

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
set(QT_LIBRARIES Qt5::Core Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(NTV2KONAIPJSONSETUP_TARGET_INCLUDE_DIRS
	${AJA_LIBRARIES_ROOT}
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIB_NTV2_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/includes
	${AJA_LIB_RETAIL_ROOT}/services)

set(NTV2KONAIPJSONSETUP_HEADERS
	konaipjsonparse.h
	konaipjsonsetup.h
	keywords.h)
set(NTV2KONAIPJSONSETUP_SOURCES
	konaipjsonparse.cpp
	main.cpp
	konaipjsonsetup.cpp)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h)

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

    set(EXTRA_LIBRARIES
        ${CORE_FOUNDATION_FRAMEWORK}
		${CORE_SERVICES_FRAMEWORK}
		${FOUNDATION_FRAMEWORK}
        ${IOKIT_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(EXTRA_LIBRARIES dl pthread rt)
endif()

set(NTV2KONAIPJSONSETUP_TARGET_SOURCES
	${NTV2KONAIPJSONSETUP_SOURCES}
	${NTV2KONAIPJSONSETUP_HEADERS}
	${DEMO_APPS_SOURCES}
	${DEMO_APPS_HEADERS})

add_executable(ntv2konaipjsonsetup ${NTV2KONAIPJSONSETUP_TARGET_SOURCES})
add_dependencies(ntv2konaipjsonsetup ajantv2)
target_include_directories(ntv2konaipjsonsetup PUBLIC ${NTV2KONAIPJSONSETUP_TARGET_INCLUDE_DIRS})
target_link_libraries(ntv2konaipjsonsetup PUBLIC ajantv2 ${EXTRA_LIBRARIES} ${QT_LIBRARIES})

if (AJA_DEPLOY_LIBS)
    include(QtHelpers)
    deploy_qt_libs(ntv2konaipjsonsetup)
endif()
