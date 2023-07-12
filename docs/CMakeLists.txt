# https://stackoverflow.com/questions/34878276/build-doxygen-from-cmake-script
project(ntv2docs)

find_package(Doxygen COMPONENTS dot)
if (NOT DOXYGEN_FOUND)
    message(WARNING "skipping docs (Doxygen not found!)")
    return()
endif()

find_program(GRAPHVIZ_DOT_PATH dot)
if (NOT GRAPHVIZ_DOT_PATH)
    message(WARNING "skipping docs (GraphViz dot command not found!)")
    return()
else()
    message(STATUS "GraphViz dot Location: ${GRAPHVIZ_DOT_PATH}")
endif()

set(NTV2_DOXYGEN_SDK_VERSION "${AJA_NTV2_VER_STR}")

# Attempt to locate qhelpgenerator for generation of Qt-related docs
set(_qthelpgenerator_executable "")
if (AJA_QT_ENABLED)

    # if a path contains both Qt5 and Qt6 versions, like some Linux distros do,
    # set QT_DEFAULT_MAJOR_VERSION to 5 or 6 to specify the version to use.
    # defaults to trying both 6 then 5 if QT_DEFAULT_MAJOR_VERSION is not set
    set(PREF_QT_MAJORS Qt6 Qt5)
    if (QT_DEFAULT_MAJOR_VERSION)
	    set(PREF_QT_MAJORS Qt${QT_DEFAULT_MAJOR_VERSION})
    endif()

    find_package(QT NAMES ${PREF_QT_MAJORS} HINTS ${AJA_QT_DIR} COMPONENTS Core)
    if (QT_VERSION_MAJOR STREQUAL "6")
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Tools)
    else()
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Help)
    endif()

    if (Qt${QT_VERSION_MAJOR}_FOUND)
        get_target_property(_qthelpgenerator_executable Qt${QT_VERSION_MAJOR}::qhelpgenerator IMPORTED_LOCATION)
    endif()

    if (EXISTS ${_qthelpgenerator_executable})
        set(QT_HELPGEN_PATH ${_qthelpgenerator_executable})
    else()
        message(WARNING "qhelpgenerator not found at '${_qthelpgenerator_executable}'")
    endif()
endif()

# Generate doxygen config file from template
set(AJA_DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(DOXY_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/ntv2.doxy)
configure_file(config.doxy.in ${DOXY_CONFIG} @ONLY)

# Run doxygen and output docs to ${CMAKE_BINARY_DIR}/ajalibraries/docs/html
add_custom_target(
    ntv2docs ALL
    DEPENDS ${DOXY_CONFIG}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXY_CONFIG}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating NTV2 SDK documentation with Doxygen"
    VERBATIM)

if (AJA_INSTALL_SOURCES)
    string(CONCAT AJA_NTV2_SDK_DOCS_URL
        "https://sdksupport.aja.com/docs/NTV2SDK_docs_"
        "${AJA_NTV2_SDK_VERSION_MAJOR}_${AJA_NTV2_SDK_VERSION_MINOR}"
        "_${AJA_NTV2_SDK_VERSION_POINT}${AJA_NTV2_SDK_BUILD_TYPE_LONG}/")
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        configure_file(${CMAKE_MODULE_PATH}/bundle/windows/aja_ntv2_documentation.url.in aja_ntv2_documentation.url)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/aja_ntv2_documentation.url DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME})
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        configure_file("${CMAKE_MODULE_PATH}/bundle/mac/AJA NTV2 Documentation.webloc.in" "AJA NTV2 Documentation.webloc")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/AJA NTV2 Documentation.webloc" DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME})
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        configure_file(${CMAKE_MODULE_PATH}/bundle/linux/desktop/aja_ntv2_documentation.desktop.in aja_ntv2_documentation.desktop)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/aja_ntv2_documentation.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME})
    endif()

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
            USE_SOURCE_PERMISSIONS)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ntv2.doxy 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME})
endif()
if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/docs)
endif()
