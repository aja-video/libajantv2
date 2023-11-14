if(NOT DEFINED Python2_EXECUTABLE)
    find_package(Python2 QUIET)
endif()
if(NOT DEFINED Python3_EXECUTABLE)
    find_package(Python3 QUIET)
endif()
if(NOT DEFINED GIT_EXECUTABLE)
	find_package(Git QUIET)
endif()

function(aja_get_git_hash _git_hash _git_hash_short)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
  if(NOT GIT_HASH)
    set(GIT_HASH "NO_GIT_COMMIT_HASH_DEFINED")
  endif()
  execute_process(
	COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_HASH_SHORT
	OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if (NOT GIT_HASH_SHORT)
    set(GIT_HASH "NO_GIT_COMMIT_HASH_DEFINED")
  endif()
  set(${_git_hash} "${GIT_HASH}" PARENT_SCOPE)
  set(${_git_hash_short} "${GIT_HASH_SHORT}" PARENT_SCOPE)
endfunction(aja_get_git_hash)

#! aja_add_subdirectory : Wrapper function to add a subdirectory to the current build.
#
# Allows for adding additional shared logic when adding a subdirectory to the build.
# NOTE: The CMake variadic arg ${ARGN} is used for optional args such as specifying
# a full project name and description, used in later processing of resource files
# (Windows/.rc, macOS/.plist, Linux/.json) for application versioning.
#
# For example:
# aja_add_subdirectory(apps/controlroom "AJA ControlRoom" "AJA Control Room - Ingest & Playout Application")
#
# \argn: A list of arguments
# \arg:target the name of the subdirectory to add
function(aja_add_subdirectory target)
	set(AJA_PRODUCT_NAME "")
	set (variadic_args ${ARGN})
	list(LENGTH variadic_args variadic_count)
	if (${variadic_count} GREATER 0)
		list(GET variadic_args 0 AJA_PRODUCT_NAME)
	endif()
	if (${variadic_count} GREATER 1)
		list(GET variadic_args 1 AJA_PRODUCT_DESC)
	endif()
	set(target_path ${CMAKE_CURRENT_SOURCE_DIR}/${target})
	if (EXISTS ${target_path} AND EXISTS ${target_path}/CMakeLists.txt)
		set(status_msg "adding target: ${target}")
		add_subdirectory(${target_path})
		if (AJA_PRODUCT_NAME)
			set(status_msg "${status_msg} (${AJA_PRODUCT_NAME})")
		endif()
		message(STATUS ${status_msg})
	else()
		set(status_msg "target not found: ${target}")
		if (AJA_PRODUCT_NAME)
			set(status_msg "${status_msg} (${AJA_PRODUCT_NAME})")
		endif()
		message(WARNING ${status_msg})
	endif()
endfunction(aja_add_subdirectory)

#! aja_add_library : Wrapper function to add a library TARGET to a CMake build.
#
# Allows for adding additional shared logic when adding an library TARGET,
# and reducing the amount of code repetition needed in CMakeLists.txt files.
#
# If the target already exists in the current build graph it will not be re-added.
#
# NOTE: Surround list args with double-quotes, otherwise list expansion will
# not be handled properly by CMake.
# eg. aja_add_library(libfoo PUBLIC "${LIST_OF_SOURCES}" ...)
#
# \argn: A list of arguments
# \arg:target the name of the library TARGET to add
# \arg:project optional project name if differs from the target name
# \arg:type the library type, one of: <STATIC|SHARED|MODULE>
# \arg:scope the SCOPE to use when adding includes, link libs, etc. One of: <INTERFACE|PUBLIC|PRIVATE>
# \arg:sources the list of sources to add the the library TARGET
# \arg:deps the list of target dependencies that this TARGET requires
# \arg:includes the list of include directories passed to this library TARGET
# \arg:libs the list of libs that this library TARGET links against
# \arg:defs the list of compile definitions passed to this library TARGET
function(aja_add_library target project type scope sources deps includes libs defs)
	if (NOT TARGET ${target})
		add_library(${target} ${type} ${sources})
		if (deps)
			add_dependencies(${target} ${deps})
		endif()
		target_include_directories(${target} ${scope} ${includes})
		target_link_libraries(${target} ${scope} ${libs})
		target_compile_definitions(${target} ${scope} ${defs})
		# Configure Windows MSVC Runtime Library
		if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
			aja_get_msvc_tools_version()
			if (CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
				if (${type} STREQUAL "STATIC")
					message(STATUS "${target} (MD: Multi-Threaded Debug)")
					set_property(TARGET ${target} PROPERTY
						MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
					set_target_properties(${target} PROPERTIES OUTPUT_NAME "${project}d_vs${AJA_MSVC_TOOLSET_VERSION}_MD")
				elseif(${type} STREQUAL "SHARED")
					message(STATUS "${target} (MDd: Multi-Threaded Debug DLL)")
					set_property(TARGET ${target} PROPERTY
						MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
					set_target_properties(${target} PROPERTIES OUTPUT_NAME "${project}${AJA_NTV2_SDK_VERSION_MAJOR}d_vs${AJA_MSVC_TOOLSET_VERSION}_MDd")
				endif()
			else()
				if (${type} STREQUAL "STATIC")
					message(STATUS "${target} (MT: Multi-Threaded)")
					set_property(TARGET ${target} PROPERTY
						MSVC_RUNTIME_LIBRARY "MultiThreaded")
					set_target_properties(${target} PROPERTIES OUTPUT_NAME "${project}_vs${AJA_MSVC_TOOLSET_VERSION}_MT")
				elseif(${type} STREQUAL "SHARED")
					message(STATUS "${target} (MTd: Multi-Threaded DLL)")
					set_property(TARGET ${target} PROPERTY
						MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
					set_target_properties(${target} PROPERTIES OUTPUT_NAME "${project}${AJA_NTV2_SDK_VERSION_MAJOR}_vs${AJA_MSVC_TOOLSET_VERSION}_MTd")
				endif()
			endif()
		endif()
	endif()
endfunction(aja_add_library)

#! aja_add_executable : Wrapper function to add an executable TARGET to a CMake build.
#
# Allows for adding additional shared logic when adding an executable TARGET,
# and reducing the amount of code repetition needed in CMakeLists.txt files.
#
# If the target already exists in the current build graph it will not be re-added.
#
# NOTE: Surround list args with double-quotes, otherwise list expansion will
# not be handled properly by CMake.
# eg. aja_add_executable(libfoo PUBLIC "${LIST_OF_SOURCES}" ...)
#
# \argn: A list of arguments
# \arg:target the name of the executable TARGET to add
# \arg:scope the SCOPE to use when adding includes, link libs, etc. One of: <INTERFACE|PUBLIC|PRIVATE>
# \arg:sources the list of sources to add the the executable TARGET
# \arg:deps the list of target dependencies that this TARGET requires
# \arg:includes the list of include directories passed to this executable TARGET
# \arg:libs the list of libs that this executable TARGET links against
# \arg:defs the list of compile definitions passed to this executable TARGET
function(aja_add_executable target scope sources deps includes libs defs)
	if (NOT TARGET ${target})
		add_executable(${target} ${sources})
		if (deps)
			add_dependencies(${target} ${deps})
		endif()
		target_include_directories(${target} ${scope} ${includes})
		target_link_libraries(${target} ${scope} ${libs})
		target_compile_definitions(${target} ${scope} ${defs})
	endif()
endfunction(aja_add_executable)

function(post_build_copy_file target src dst)
	# Helper function to copy src file to the dst path after the target build completes.
	# Args:
	#  - target:  the name of the target executable
	#  - src: The path of the src file to copy.
	#  - dst: The destination path where the file will be copied.
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${src} ${dst})
endfunction(post_build_copy_file)

function(aja_versionize_windows product_name product_desc icon_file target_filename rc_file_in rc_file_out)
	if (icon_file)
    	set(AJA_APP_ICON "IDI_ICON1 ICON DISCARDABLE \"${icon_file}\"")
	else()
		set(AJA_APP_ICON "// IDI_ICON1 ICON DISCARDABLE \"\"")
	endif()
    set(AJA_PRODUCT_NAME ${product_name})
	set(AJA_PRODUCT_DESC ${product_desc})
	set(AJA_TARGET_FILENAME ${target_filename})
    configure_file(${rc_file_in} ${rc_file_out} @ONLY)
endfunction(aja_versionize_windows)

function(aja_versionize_linux target product_name product_desc icon_file)
	set(file_in ${LIBAJANTV2_CMAKE_DIR}/bundle/linux/lin-ver.json.in)
	set(file_out linux_info.json)
    set(AJA_APP_ICON ${icon_file})
    set(AJA_PRODUCT_NAME ${product_name})
	set(AJA_PRODUCT_DESC ${product_desc})
    configure_file(${file_in} ${file_out} @ONLY)
	if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		# embed version info as JSON in elf header
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND
			objcopy
			--add-section "aja_app_info=${file_out}"
			"$<TARGET_FILE:${target}>"
			VERBATIM)
	endif()
endfunction(aja_versionize_linux)

# Accepts an optional param for ${icon_file} 
# We don't always have an icon file per plugin, all mac targets need versioning
function(aja_versionize_mac target product_name bundle_id bundle_sig bundle_package_type bundle_name)
    set (var_args "${ARGN}")
    list(LENGTH var_args var_count)
    set(AJA_APP_ICON "")
    if(${var_count} GREATER 0)
        list(GET var_args 0 icon_file)
        # message(STATUS "!!! ${ARGN} ${var_count} ${icon_file}")
        
        get_filename_component(icon_filename ${icon_file} NAME)
	    set(AJA_APP_ICON ${icon_filename})
        set_source_files_properties(${icon_file} PROPERTIES
		    MACOSX_PACKAGE_LOCATION Resources)
    endif()

	set(file_in ${LIBAJANTV2_CMAKE_DIR}/bundle/mac/Info.plist.in)
	set(file_out ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
	set(AJA_PRODUCT_NAME ${product_name})
	set(AJA_BUNDLE_IDENTIFIER ${bundle_id})
	set(AJA_BUNDLE_SIGNATURE ${bundle_sig})
	set(AJA_BUNDLE_PACKAGE_TYPE ${bundle_package_type})
	set(AJA_BUNDLE_NAME ${bundle_name})
	configure_file(${file_in} ${file_out} @ONLY)
	set_source_files_properties(${file_out} PROPERTIES
		MACOSX_PACKAGE_LOCATION Contents)
	set_target_properties(${target} PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${file_out})
endfunction(aja_versionize_mac)

function(aja_code_sign targets)
	# Code sign build targets with aja "pythonlib" helper scripts.
	# NOTE: This functionality is not yet available in ntv2 open-source.
	if (UNIX AND NOT APPLE)
		message(STATUS "Code signing is not available on this platform!")
		return()
	endif()

	set(sign_script_path ${AJAPY_DIR}/aja/scripts/cli/ntv2sign.py)
	foreach(target IN LISTS targets)
		if (EXISTS "${sign_script_path}")
			set(ajapy_path ${AJAPY_DIR}/aja)
			get_filename_component(ajapy_path "${ajapy_path}" REALPATH)
			get_filename_component(sign_script_path "${sign_script_path}" REALPATH)
			message(STATUS "Code Sign: ${target}")
			add_custom_command(TARGET ${target} POST_BUILD
				COMMAND
					${CMAKE_COMMAND} -E env "PYTHONPATH=\"${ajapy_path}\""
					${Python3_EXECUTABLE}
					${sign_script_path}
					--re-sign
					--notarize
					--strict
					$<TARGET_FILE:${target}>
				COMMENT "Signing '$<TARGET_FILE:${target}>' ...")
		endif()
	endforeach()
endfunction(aja_code_sign)

function (aja_get_msvc_tools_version)
        if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
                if (CMAKE_MAJOR_VERSION GREATER_EQUAL 3 AND CMAKE_MINOR_VERSION GREATER_EQUAL 12)
                                # available in CMake 3.12 or later
                                set(AJA_MSVC_TOOLSET_VERSION ${MSVC_TOOLSET_VERSION} PARENT_SCOPE)
                        else()
                        if (MSVC_VERSION EQUAL 1700)
                                # Visual Studio 2012 (VS 11.0 v110 toolset)
                                set(AJA_MSVC_TOOLSET_VERSION 110 PARENT_SCOPE)
                        elseif(MSVC_VERSION EQUAL 1800)
                                # Visual Studio 2013 (VS 12.0 v120 toolset)
                                set(AJA_MSVC_TOOLSET_VERSION 120 PARENT_SCOPE)
                        elseif(MSVC_VERSION EQUAL 1900)
                                # Visual Studio 2015 (VS 14.0 v140 toolset)
                                set(AJA_MSVC_TOOLSET_VERSION 140 PARENT_SCOPE)
                        elseif(MSVC_VERSION GREATER_EQUAL 1910 AND MSVC_VERSION LESS_EQUAL 1919)
                                # VS 2017 (VS 14.1 v141 toolset)
                                set(AJA_MSVC_TOOLSET_VERSION 141 PARENT_SCOPE)
                        elseif(MSVC_VERSION GREATER_EQUAL 1920 AND MSVC_VERSION LESS_EQUAL 1929)
                                # VS 2019 (VS 16.0 v142 toolset)
                                set(AJA_MSVC_TOOLSET_VERSION 142 PARENT_SCOPE)
                        elseif(MSVC_VERSION GREATER_EQUAL 1930 AND MSVC_VERSION LESS_EQUAL 1939)
                                # VS 2022 (VS 17.0 v143 toolset)
                                set(AJA_MSVC_TOOLSET_VERSION 143 PARENT_SCOPE)
                        endif()
                endif()
        endif()
endfunction(aja_get_msvc_tools_version)

function(aja_ntv2_log_build_info)
        string(TIMESTAMP _build_time "%A, %B %d, %Y at %H:%M:%S GMT-8")
        message(STATUS "\n\n---\nAJA NTV2 SDK ${AJA_NTV2_VER_STR} ${AJA_NTV2_SDK_BUILD_TYPE_LONG} (${AJA_GIT_COMMIT_HASH_SHORT})\nBuilt on: ${_build_time}\n---\n")
        message(STATUS "Build Number: ${AJA_NTV2_SDK_BUILD_NUMBER}")
        message(STATUS "Built from commit: ${AJA_GIT_COMMIT_HASH}")
        if(CMAKE_BUILD_TYPE MATCHES Debug)
        message(STATUS "Build Type: Debug")
        elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        message(STATUS "Build Type: Release with Debug Symbols")
        else()
        message(STATUS "Build Type: Release")
        endif()
        if (AJANTV2_BUILD_OPENSOURCE)
                message(STATUS "Building open-source AJA NTV2 SDK and apps")
        else()
                message(STATUS "Building proprietary AJA NTV2 SDK and apps")
        endif()
        message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
        message(STATUS "Processor Arch: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
        message(STATUS "Bits: ${AJA_BITS}")
        message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
        message(STATUS "C++${CMAKE_CXX_STANDARD} C${CMAKE_C_STANDARD}")
        message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
        if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
                message(STATUS "Windows API Version: ${CMAKE_SYSTEM_VERSION}")
                string(REPLACE "." ";" AJA_WIN_API_VER "${CMAKE_SYSTEM_VERSION}")
                list(GET AJA_WIN_API_VER 0 AJA_WIN_API_VER_MAJOR)
                list(GET AJA_WIN_API_VER 1 AJA_WIN_API_VER_MINOR)
                list(GET AJA_WIN_API_VER 2 AJA_WIN_API_VER_BUILD)
                message(STATUS "MSVC Version: ${MSVC_VERSION}")
                aja_get_msvc_tools_version()
                message(STATUS "MSVC_TOOLSET: v${AJA_MSVC_TOOLSET_VERSION}")
				message(STATUS "CMAKE_MSVC RUNTIME LIBRARY: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
                message(STATUS "macOS Architecture(s): ${CMAKE_OSX_ARCHITECTURES}")
                message(STATUS "macOS Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "BareMetal")
        else()
                message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
        endif()
        message(STATUS "Python 2: ${Python2_EXECUTABLE}")
        message(STATUS "Python 3: ${Python3_EXECUTABLE}")
        message(STATUS "Git: ${GIT_EXECUTABLE}")
endfunction(aja_ntv2_log_build_info)
