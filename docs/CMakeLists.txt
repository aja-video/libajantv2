# https://stackoverflow.com/questions/34878276/build-doxygen-from-cmake-script
project(ntv2docs)

find_package(Doxygen COMPONENTS dot)
if (NOT DOXYGEN_FOUND)
    message(STATUS "skipping docs (Doxygen not found!)")
    return()
endif()

find_program(GRAPHVIZ_DOT_PATH dot)
if (NOT GRAPHVIZ_DOT_PATH)
    message(STATUS "skipping docs (GraphViz dot command not found!)")
    return()
else()
    message(STATUS "GraphViz dot Location: ${GRAPHVIZ_DOT_PATH}")
endif()

set(NTV2_DOXYGEN_SDK_VERSION "${AJA_NTV2_VER_STR}")

# Attempt to locate qhelpgenerator for generation of Qt-related docs
set(_qthelpgenerator_executable "qhelpgenerator")
if (AJA_QT_ENABLED)
    find_package(Qt5Core HINTS ${AJA_QT_DIR} REQUIRED)
    get_target_property(_qmake_location Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_location}" DIRECTORY)
    if (EXISTS ${_qt_bin_dir} AND EXISTS ${_qt_bin_dir}/qhelpgenerator)
        set(QT_HELPGEN_PATH ${_qt_bin_dir}/qhelpgenerator)
    else()
        message(STATUS "qhelpgenerator not found in ${_qt_bin_dir}")
    endif()
endif()

# Generate doxygen config file from template
set(AJA_DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(DOXY_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/ntv2.doxy)
configure_file(config.doxy.in ${DOXY_CONFIG} @ONLY)

# Run doxygen and output docs to ${CMAKE_BINARY_DIR}/ajalibraries/docs/html
add_custom_target(
    ntv2docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} "${DOXY_CONFIG}"
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
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/aja_ntv2_documentation.url DESTINATION ${CMAKE_INSTALL_PREFIX}/docs)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        configure_file("${CMAKE_MODULE_PATH}/bundle/mac/AJA NTV2 Documentation.webloc.in" "AJA NTV2 Documentation.webloc")
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/AJA NTV2 Documentation.webloc" DESTINATION ${CMAKE_INSTALL_PREFIX}/docs)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        configure_file(${CMAKE_MODULE_PATH}/bundle/linux/desktop/aja_ntv2_documentation.desktop.in aja_ntv2_documentation.desktop)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/aja_ntv2_documentation.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/docs)
    endif()
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION ${CMAKE_INSTALL_PREFIX}/ajalibraries/docs)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ntv2.doxy DESTINATION ${CMAKE_INSTALL_PREFIX}/ajalibraries/docs)
endif()
