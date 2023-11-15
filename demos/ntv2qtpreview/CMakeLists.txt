project(ntv2qtpreview)

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

set(AJA_LIB_GUI_ROOT
        ${LIBAJAQT_DIR}/gui)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${AJA_LIBRARIES_ROOT}
	${AJA_LIB_NTV2_ROOT}/includes
)

set(NTV2QTPREVIEW_SOURCES
    main.cpp
    ntv2qtpreview.h
    ntv2qtpreview.cpp
    ntv2qtpreview.qrc
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
        "ntv2qtpreview.exe"
		${LIBAJANTV2_CMAKE_DIR}/bundle/windows/win-ver.rc.in
        win-ver.rc)
    list(APPEND NTV2QTPREVIEW_SOURCES
        win-ver.rc)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS dl pthread rt)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_MACOSX_BUNDLE ON)
    set(NTV2QTPREVIEW_SOURCES ${NTV2QTPREVIEW_SOURCES} app.icns)

    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices)
    find_library(IOKIT_FRAMEWORK IoKit)

    set(TARGET_LINK_LIBS
        ${APPLICATION_SERVICES_FRAMEWORK}
        ${IOKIT_FRAMEWORK})
endif()

set(TARGET_SOURCES
	${NTV2QTPREVIEW_SOURCES})
set(TARGET_DEPS ajantv2)
set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS}
    ajantv2 ${TARGET_QT_LIBS})

add_executable(${PROJECT_NAME} ${TARGET_SOURCES})
add_dependencies(${PROJECT_NAME} ${TARGET_DEPS})
target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LINK_LIBS})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${TARGET_COMPILE_DEFS})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(ntv2qtpreview PROPERTIES WIN32_EXECUTABLE TRUE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    aja_versionize_linux(
        ${PROJECT_NAME}
        ${PROJECT_NAME}
        ${PROJECT_NAME}
        app.ico)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    aja_versionize_mac(ntv2qtpreview
        com.aja.ntv2qtpreview
        "AjLg"
        "APPL"
        "ntv2qtpreview"
        app.icns)
endif()
if (AJA_CODE_SIGN)
    aja_code_sign(${PROJECT_NAME})
endif()
if (AJA_QT_DEPLOY)
    aja_deploy_qt_libs_to_dest(ntv2qtpreview ${CMAKE_INSTALL_FULL_BINDIR})
endif()
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (AJA_INSTALL_SOURCES)
    list(REMOVE_ITEM NTV2QTPREVIEW_SOURCES win-ver.rc)
    install(FILES ${AJA_LIB_GUI_ROOT}/ajaapplication.qrc
        DESTINATION ${CMAKE_INSTALL_PREFIX}/ajalibraries/ajainternal/gui)
    install(FILES ${NTV2QTPREVIEW_HEADERS} 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtpreview)
    install(FILES ${NTV2QTPREVIEW_SOURCES}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtpreview)
endif()
if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtpreview)
endif()
if (AJA_INSTALL_MISC)
    install(FILES Makefile
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtpreview)
    install(FILES app.icns
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtpreview)
    install(FILES app.ico
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtpreview)
    install(FILES splash.png
        DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos/ntv2qtpreview)
endif()
