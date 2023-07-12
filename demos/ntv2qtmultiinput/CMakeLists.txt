project(ntv2qtmultiinput)

if (NOT AJA_QT_ENABLED)
	message(STATUS "AJA_QT_ENABLED not set. Ignoring project ${PROJECT_NAME}...")
    return()
endif()


include(QtHelpers)
aja_find_qt_modules(Core Multimedia Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(AJA_DEMO_COMMON_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/../)

set(AJA_LIB_GUI_ROOT
        ${LIBAJAQT_DIR}/gui)

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
    aja_versionize_windows(
        ${PROJECT_NAME}
        ${PROJECT_NAME}
        app.ico
        "ntv2qtmultiinput.exe"
        ${CMAKE_MODULE_PATH}/bundle/windows/win-ver.rc.in
        win-ver.rc)
    list(APPEND NTV2QTMULTIINPUT_SOURCES
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
    ajantv2 ajagui ${TARGET_QT_LIBS})

add_executable(${PROJECT_NAME} ${TARGET_SOURCES})
add_dependencies(${PROJECT_NAME} ${TARGET_DEPS})
target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LINK_LIBS})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${TARGET_COMPILE_DEFS})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(ntv2qtmultiinput PROPERTIES WIN32_EXECUTABLE TRUE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    aja_versionize_linux(
        ${PROJECT_NAME}
        ${PROJECT_NAME}
        ${PROJECT_NAME}
        app.ico)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    aja_versionize_mac(ntv2qtmultiinput
        com.aja.ntv2qtmultiinput
        "AjLg"
        "APPL"
        "ntv2qtmultiinput"
        app.icns)
endif()
if (AJA_CODE_SIGN)
    aja_code_sign(${PROJECT_NAME})
endif()
if (AJA_QT_DEPLOY)
    aja_deploy_qt_libs_to_dest(ntv2qtmultiinput ${CMAKE_INSTALL_FULL_BINDIR})
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
    install(FILES ${NTV2QTMULTIINPUT_HEADERS} 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtmultiinput)
    install(FILES ${NTV2QTMULTIINPUT_SOURCES}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtmultiinput)
endif()
if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtmultiinput)
endif()
if (AJA_INSTALL_MISC)
    install(FILES Makefile      DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtmultiinput)
    install(FILES app.icns      DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtmultiinput)
    install(FILES app.ico       DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtmultiinput)
    install(FILES splash.png    DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtmultiinput)
endif()
