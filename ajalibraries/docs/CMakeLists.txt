# https://stackoverflow.com/questions/34878276/build-doxygen-from-cmake-script
find_package(Doxygen COMPONENTS dot)
if (NOT DOXYGEN_FOUND)
    aja_message(STATUS "skipping docs (Doxygen not found!)")
    return()
endif()

set(NTV2_DOXYGEN_SDK_VERSION "${AJA_NTV2_VER_STR}")

# Attempt to locate qhelpgenerator for generation of Qt-related docs
set(_qthelpgenerator_executable "qhelpgenerator")
if (AJA_QT_DIR)
    find_package(Qt5Core HINTS ${AJA_QT_DIR} REQUIRED)
    get_target_property(_qmake_location Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_location}" DIRECTORY)
    set(QT_HELPGEN_PATH ${_qt_bin_dir}/qhelpgenerator)
endif()

# Generate doxygen config file from template
set(AJA_DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(DOXY_CONFIG ntv2.doxy)
configure_file(config.doxy.in ${DOXY_CONFIG} @ONLY)

# Run doxygen and output docs to ${CMAKE_BINARY_DIR}/ajalibraries/docs/html
add_custom_target(
    doc ALL
    COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/${DOXY_CONFIG}"
    WORKING_DIRECTORY ${AJA_NTV2_ROOT}
    COMMENT "Generating NTV2 SDK documentation with Doxygen"
    VERBATIM)
