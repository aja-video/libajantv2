include(Helpers)

# if a path contains both Qt5 and Qt6 versions, like some Linux distros do,
# set QT_DEFAULT_MAJOR_VERSION to 5 or 6 to specify the version to use.
# defaults to trying both 6 then 5 if QT_DEFAULT_MAJOR_VERSION is not set
set(PREF_QT_MAJORS Qt6 Qt5)
if (QT_DEFAULT_MAJOR_VERSION)
	set(PREF_QT_MAJORS Qt${QT_DEFAULT_MAJOR_VERSION})
endif()

# handles finding the passed in modules and returning them in a list named
# TARGET_QT_LIBS that can be used in target_link_libraries()
# also sets QT_VERSION, QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH
# NOTE: using a macro instead of a function so don't need to manually set variables in the parent scope
macro(aja_find_qt_modules)
	set(_module_args "${ARGN}")

	find_package(QT NAMES ${PREF_QT_MAJORS} HINTS ${AJA_QT_DIR} COMPONENTS ${_module_args})
	find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${_module_args})

    message(STATUS "aja_find_qt_module(s): ${_module_args}")

    foreach(m IN LISTS _module_args)
		list(APPEND TARGET_QT_LIBS "Qt${QT_VERSION_MAJOR}::${m}")

        if (NOT Qt${QT_VERSION_MAJOR}${m}_FOUND)
            message(STATUS "  ? ${m}")
        else()
            message(STATUS "  + ${m}")
            list(APPEND _modules_found ${m})
        endif()

        list(LENGTH _module_args _args_count)
        list(LENGTH _modules_found _modules_found_count)

        if (_args_count GREATER _modules_found_count)
            set(AJA_QT_FOUND FALSE)
        else()
            set(AJA_QT_FOUND TRUE)
        endif()
	endforeach()

    message(STATUS "Found ${_modules_found_count}/${_args_count} Qt modules: ${_modules_found}")

	list(REMOVE_DUPLICATES TARGET_QT_LIBS)
endmacro(aja_find_qt_modules)

# https://stackoverflow.com/questions/60854495/qt5-cmake-include-all-libraries-into-executable
# get absolute path to qmake, then use it to find windeployqt executable
function(aja_deploy_qt_libs target)
    include(GNUInstallDirs)
    find_package(QT NAMES ${PREF_QT_MAJORS} HINTS ${AJA_QT_DIR} REQUIRED COMPONENTS Core)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    if (WIN32)
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
        if (NOT DEFINED AJAPY_DIR)
            message(WARNING "AJAPY_DIR not found. Not deploying qtlibs")
            return()
        endif()

        get_filename_component(_lin_deploy_qt_path "${AJAPY_DIR}/aja/scripts/cli/linuxdeployqt.py" REALPATH)
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
                    "--qtversion=${QT_VERSION}"
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

function(aja_deploy_qt_libs_to_dest target dest)
    include(GNUInstallDirs)
    find_package(QT NAMES ${PREF_QT_MAJORS} HINTS ${AJA_QT_DIR} REQUIRED COMPONENTS Core)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    if (WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${Python3_EXECUTABLE}
				\"${LIBAJANTV2_CMAKE_DIR}/scripts/qtdeploy.py\"
                \"${_qt_bin_dir}/windeployqt.exe\"
                \"$<TARGET_FILE:${target}>\"
                \"${dest}\"
            COMMENT "Deploying Qt Release libraries for target '${target}' via winqtdeploy ..."
        )

        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${Python3_EXECUTABLE}
					\"${LIBAJANTV2_CMAKE_DIR}/scripts/qtdeploy.py\"
                    \"${_qt_bin_dir}/windeployqt.exe\"
                    \"$<TARGET_FILE:${target}>\"
                    \"${dest}\"
                    "--debug"
                COMMENT "Deploying Qt Debug libraries for target '${target}' via winqtdeploy ..."
            )
        endif()
    elseif (APPLE)
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
        get_filename_component(_lin_deploy_qt_path "${AJAPY_DIR}/aja/scripts/cli/linuxdeployqt.py" REALPATH)
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
                    "--qtversion=${QT_VERSION}"
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
endfunction(aja_deploy_qt_libs_to_dest)
