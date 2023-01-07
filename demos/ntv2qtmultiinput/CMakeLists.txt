project(ntv2qtmultiinput)

if (NOT AJA_QT_ENABLED)
	aja_message(STATUS "AJA_QT_ENABLED not set. Ignoring project ${PROJECT_NAME}...")
    return()
endif()

find_package(Qt5 HINTS ${AJA_QT_DIR} COMPONENTS Core Multimedia Widgets REQUIRED)
set(QT_LIBRARIES Qt5::Core Qt5::Multimedia Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${AJA_LIBRARIES_ROOT}
	${AJA_LIB_NTV2_ROOT}/includes
    ${AJA_LIB_GUI_ROOT}
    ${AJA_DEMO_COMMON_ROOT})

set(NTV2QTMULTIINPUT_SOURCES
    main.cpp
    ntv2qtmultiinput.h
    ntv2qtmultiinput.cpp
    ntv2qtmultiinput.qrc
    ${AJA_DEMO_COMMON_ROOT}/ntv2framegrabber.h
    ${AJA_DEMO_COMMON_ROOT}/ntv2framegrabber.cpp
    ${AJA_DEMO_COMMON_ROOT}/ntv2democommon.h
    ${AJA_DEMO_COMMON_ROOT}/ntv2democommon.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    remove_definitions(
        -DUNICODE
        -D_UNICODE)
    set(TARGET_COMPILE_DEFS
        -D_MBCS
        -DNOMINMAX)
    aja_make_version_rc(
        ${AJA_PRODUCT_NAME}
        ${AJA_PRODUCT_DESC}
        ${CMAKE_MODULE_PATH}/bundle/windows/win-ver.rc.in
        win-ver.rc
        ${CMAKE_CURRENT_SOURCE_DIR}/app.ico)
    set(NTV2QTMULTIINPUT_SOURCES
        ${NTV2QTMULTIINPUT_SOURCES}
        win-ver.rc)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_MACOSX_BUNDLE ON)
    set(NTV2QTMULTIINPUT_SOURCES ${NTV2QTMULTIINPUT_SOURCES} app.icns)

    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices)
    find_library(IOKIT_FRAMEWORK IoKit)

    set(TARGET_LINK_LIBS
        ${APPLICATION_SERVICES_FRAMEWORK}
        ${IOKIT_FRAMEWORK})
endif()

set(TARGET_SOURCES
	${NTV2QTMULTIINPUT_SOURCES})
set(TARGET_DEPS ajagui ajantv2)
set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS}
    ajantv2 ajagui ${QT_LIBRARIES})

aja_add_executable(ntv2qtmultiinput PUBLIC
    "${TARGET_SOURCES}"
    "${TARGET_DEPS}"
    "${TARGET_INCLUDE_DIRS}"
    "${TARGET_LINK_LIBS}"
    "${TARGET_COMPILE_DEFS}")
set(BUILD_TARGETS ntv2qtmultiinput)
# if (AJA_BUILD_SHARED)
#     set(BUILD_TARGETS ${BUILD_TARGETS} ntv2qtmultiinput-dl)
#     set(TARGET_DEPS ajagui-dl ajantv2-dl)
#     aja_add_executable(ntv2qtmultiinput-dl PUBLIC
#         "${TARGET_SOURCES}"
#         "${TARGET_DEPS}"
#         "${TARGET_INCLUDE_DIRS}"
#         "${TARGET_LINK_LIBS}"
#         "${TARGET_COMPILE_DEFS}")
# endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(ntv2qtmultiinput PROPERTIES WIN32_EXECUTABLE TRUE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    aja_make_version_elf(ntv2qtmultiinput
        ${AJA_PRODUCT_NAME}
        ${AJA_PRODUCT_DESC}
        ${CMAKE_MODULE_PATH}/bundle/linux/lin-ver.json.in
        lin-ver.json
        app.ico)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    aja_make_version_plist(ntv2qtmultiinput
        ${AJA_PRODUCT_NAME}
        "AjLg"
        ${CMAKE_MODULE_PATH}/bundle/mac/Info.plist.in
        ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
        app.icns)
endif()
if (AJA_CODE_SIGN)
    aja_code_sign(${BUILD_TARGETS})
endif()
if (AJA_DEPLOY_QT_LIBS)
    include(AJAQtHelpers)
    aja_deploy_qt_libs(ntv2qtmultiinput)
endif()
install(TARGETS ${BUILD_TARGETS}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (AJA_INSTALL_SOURCES)
    list(REMOVE_ITEM NTV2QTMULTIINPUT_SOURCES win-ver.rc)
    install(FILES ${AJA_LIB_GUI_ROOT}/ajaapplication.qrc
        DESTINATION ${CMAKE_INSTALL_PREFIX}/ajalibraries/ajainternal/gui)
    install(FILES ${NTV2QTMULTIINPUT_HEADERS} DESTINATION
        ${CMAKE_INSTALL_PREFIX}/demos/ntv2qtmultiinput)
    install(FILES ${NTV2QTMULTIINPUT_SOURCES}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2qtmultiinput)
endif()
if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt
        DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2qtmultiinput)
endif()
if (AJA_INSTALL_MISC)
    install(FILES Makefile      DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2qtmultiinput)
    install(FILES app.icns      DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2qtmultiinput)
    install(FILES app.ico       DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2qtmultiinput)
    install(FILES splash.png    DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/ntv2qtmultiinput)
endif()
