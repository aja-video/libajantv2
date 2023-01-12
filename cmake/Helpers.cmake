function(aja_ntv2_log_build_info)
	string(TIMESTAMP _build_time "%A, %B %d, %Y at %H:%M:%S GMT-8")
	message(STATUS "---\n\nAJA NTV2 SDK ${AJA_NTV2_VER_STR} ${AJA_NTV2_SDK_BUILD_TYPE}${AJA_NTV2_SDK_BUILD_NUMBER}\nBuilt on: ${_build_time}\n")
	if (AJA_GIT_COMMIT_HASH_SHORT)
		message(STATUS "Built from commit: ${AJA_GIT_COMMIT_HASH_SHORT}")
	endif()
	if (AJANTV2_BUILD_OPENSOURCE)
		message(STATUS "Building open-source AJA NTV2 SDK and apps")
	else()
		message(STATUS "Building proprietary AJA NTV2 SDK and apps")
	endif()
	message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
	message(STATUS "Arch: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
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
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		message(STATUS "macOS Architectures: ${CMAKE_OSX_ARCHITECTURES}")
		message(STATUS "macOS Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	else()
		message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
	endif()
	message(STATUS "Python 2: ${Python2_EXECUTABLE}")
	message(STATUS "Python 3: ${Python3_EXECUTABLE}")
	message(STATUS "Git: ${GIT_EXECUTABLE}")
endfunction(aja_ntv2_log_build_info)

function(aja_ntv2_sdk_gen target_deps)
	add_custom_command(
		TARGET ${target_deps} PRE_BUILD
		COMMAND
			${Python2_EXECUTABLE} ${AJA_NTV2_ROOT}/ajalibraries/ajantv2/sdkgen/ntv2sdkgen.py
			--verbose --unused --ajantv2 ajalibraries/ajantv2 --ohh ajalibraries/ajantv2/includes --ohpp ajalibraries/ajantv2/src
		WORKING_DIRECTORY ${AJA_NTV2_ROOT}
		COMMENT "Running ntv2sdkgen script...")
endfunction(aja_ntv2_sdk_gen)

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
