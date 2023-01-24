# https://stackoverflow.com/questions/60854495/qt5-cmake-include-all-libraries-into-executable
# get absolute path to qmake, then use it to find windeployqt executable
function(aja_deploy_qt_libs target)
    if (WIN32)
        find_package(Qt5Core HINTS ${AJA_QT_DIR} REQUIRED)
        get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${_qt_bin_dir}/windeployqt.exe"
                --verbose 1
                --no-svg
                --no-angle
                --no-opengl
                --no-opengl-sw
                --no-compiler-runtime
                --no-system-d3d-compiler
                --release
                \"$<TARGET_FILE:${target}>\"
            COMMENT "Deploying Qt Release libraries for target '${target}' via winqtdeploy ..."
        )

        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${_qt_bin_dir}/windeployqt.exe"
                    --verbose 1
                    --no-svg
                    --no-angle
                    --no-opengl
                    --no-opengl-sw
                    --no-compiler-runtime
                    --no-system-d3d-compiler
                    --debug
                    \"$<TARGET_FILE:${target}>\"
                COMMENT "Deploying Qt Debug libraries for target '${target}' via winqtdeploy ..."
            )
        endif()
    elseif (APPLE)
        find_package(Qt5Core HINTS ${AJA_QT_DIR} REQUIRED)
        get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        get_target_property(_is_target_bundle ${target} MACOSX_BUNDLE)
        message(STATUS "Attempting to macdeployqt ${target} -- ${_qt_bin_dir}")
        if (_is_target_bundle)
            message(STATUS "macOS bundle: ${target}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${_qt_bin_dir}/macdeployqt $<TARGET_BUNDLE_DIR:${target}>/ -always-overwrite
                COMMENT "Deploying Qt Frameworks into target .app bundle: '${target}' (macdeployqt)"
            )
        else()
            message(STATUS "macOS binary: ${target}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${_qt_bin_dir}/macdeployqt $<TARGET_FILE_DIR:${target}>/ -executable=$<TARGET_FILE_DIR:${target}>/${target} -always-overwrite
                COMMENT "Deploying Qt Frameworks into target binary: '${target}' (macdeployqt)"
            )
        endif()
    elseif (UNIX AND NOT APPLE)
        message(STATUS "ajalinuxdeployqt: ${target}")
        get_filename_component(_lin_deploy_qt_path "${AJA_NTV2_ROOT}/tools/qt/ajalinuxdeployqt.py" REALPATH)
        if (EXISTS ${_lin_deploy_qt_path})
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${Python3_EXECUTABLE}
                    ${_lin_deploy_qt_path}
                    "--app=$<TARGET_FILE:${target}>"
                    "--create_qt_conf"
                    "--qt_conf_plugins_dir=plugins"
                    "--lib_rpath=\"$ORIGIN/qtlibs\""
                    "--qt_libs_opath=$<TARGET_FILE_DIR:${target}>/qtlibs"
                    "--qt_plugins_opath=$<TARGET_FILE_DIR:${target}>/plugins"
                    "--qtversion=5.13.2"
                    "--verbose"
                COMMAND_EXPAND_LISTS
                VERBATIM
                COMMENT "Deploying Qt Frameworks in target app: '${target}' (ajalinuxdeployqt)"
            )
            # Have CMake set RPATH even though the ajalinuxdeployqt script already does it.
            # If we omit this step CMake Install gets confused that the RPATH set at build time
            # no longer matches what it sees at install time.
            set_target_properties(${target} PROPERTIES INSTALL_RPATH "$ORIGIN/qtlibs")

            install(FILES $<TARGET_FILE_DIR:${target}>/qtlibs DESTINATION ${CMAKE_INSTALL_BINDIR})
            install(FILES $<TARGET_FILE_DIR:${target}>/plugins DESTINATION ${CMAKE_INSTALL_BINDIR})
            install(FILES $<TARGET_FILE_DIR:${target}>/qt.conf DESTINATION ${CMAKE_INSTALL_BINDIR})
        else()
            message(STATUS "WARNING -- AJA Linux Deploy Qt script not found: ${_lin_deploy_qt_path}")
        endif()
    endif()
endfunction(aja_deploy_qt_libs)