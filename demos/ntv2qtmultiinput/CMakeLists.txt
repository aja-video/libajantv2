project(ntv2qtmultiinput)

include(QtHelpers)
aja_find_qt_modules(Core Multimedia Widgets)
if (NOT AJA_QT_FOUND)
    message(WARNING "One or more Qt modules not found!")
    return()
else()
    list(APPEND TARGET_LINK_LIBS ${TARGET_QT_LIBS})
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(TARGET_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${AJA_LIBRARIES_ROOT}
    ${AJA_LIB_NTV2_ROOT}/includes
)

set(NTV2QTMULTIINPUT_SOURCES
    main.cpp
    ntv2qtmultiinput.h
    ntv2qtmultiinput.cpp
    ntv2qtmultiinput.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/../ajapreviewwidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../ajapreviewwidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../ntv2framegrabber.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../ntv2framegrabber.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../ntv2democommon.cpp)

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
		${LIBAJANTV2_CMAKE_DIR}/bundle/windows/win-ver.rc.in
        win-ver.rc)
    list(APPEND NTV2QTMULTIINPUT_SOURCES
        win-ver.rc)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND TARGET_LINK_LIBS dl pthread rt)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_MACOSX_BUNDLE ON)
    set(NTV2QTMULTIINPUT_SOURCES ${NTV2QTMULTIINPUT_SOURCES} app.icns)

    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices)
    find_library(IOKIT_FRAMEWORK IoKit)

    list(APPEND TARGET_LINK_LIBS
        ${APPLICATION_SERVICES_FRAMEWORK}
        ${IOKIT_FRAMEWORK})
endif()

set(TARGET_SOURCES ${NTV2QTMULTIINPUT_SOURCES})
set(TARGET_DEPS ajantv2)
list(APPEND TARGET_LINK_LIBS ajantv2)

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
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
