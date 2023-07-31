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

# Options | Resulting flags are:
# MultiThreaded 		| /MT
# MultiThreadedDebug    | /MTd 
# MultiThreadedDLL		| /MD
# MultiThreadedDebugDLL | /MDd
function(aja_set_global_msvc_runtime_library linkage)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		if (${linkage} STREQUAL "MultiThreaded" OR
			${linkage} STREQUAL "MultiThreadedDebug" OR
			${linkage} STREQUAL "MultiThreadedDLL" OR
			${linkage} STREQUAL "MultiThreadedDebugDLL")
			set(CMAKE_MSVC_RUNTIME_LIBRARY "${linkage}" PARENT_SCOPE)
			message(STATUS "Setting CMAKE_MSVC_RUNTIME_LIBRARY to '${linkage}'")
		else()
			message(WARNING "${CMAKE_CURRENT_FUNCTION}: Invalid linkage '${linkage}'. Using CMake default")
		endif()
	endif()
endfunction(aja_set_global_msvc_runtime_library)

function(aja_set_target_msvc_runtime_library target linkage)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		if (${linkage} STREQUAL "MultiThreaded" OR
			${linkage} STREQUAL "MultiThreadedDebug" OR
			${linkage} STREQUAL "MultiThreadedDLL" OR
			${linkage} STREQUAL "MultiThreadedDebugDLL")
            
			set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY ${linkage})
			message(STATUS "${target}: Setting MSVC_RUNTIME_LIBRARY to '${linkage}'")
		else()
			message(WARNING "${CMAKE_CURRENT_FUNCTION}: ${target}: Invalid linkage '${linkage}'. Using CMake default")
		endif()
	endif()
endfunction(aja_set_target_msvc_runtime_library)

# Copied from GoogleTest, which copied it from CMake's Wiki page linked below.
# Tweaks CMake's default compiler/linker settings to suit Google Test's needs.
# This must be a macro(), as inside a function string() can only
# update variables in the function scope.
macro(fix_msvc_crt_static_linkage)
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC|Clang")
    # For MSVC and Clang, CMake sets certain flags to defaults we want to
    # override.
    # This replacement code is taken from sample in the CMake Wiki at
    # https://gitlab.kitware.com/cmake/community/wikis/FAQ#dynamic-replace.
    foreach (flag_var
             CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
             CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
             CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
             CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if (NOT BUILD_SHARED_LIBS AND NOT gtest_force_shared_crt)
        # When Google Test is built as a shared library, it should also use
        # shared runtime libraries.  Otherwise, it may end up with multiple
        # copies of runtime library data in different modules, resulting in
        # hard-to-find crashes. When it is built as a static library, it is
        # preferable to use CRT as static libraries, as we don't have to rely
        # on CRT DLLs being available. CMake always defaults to using shared
        # CRT libraries, so we override that default here.
        string(REPLACE "/MD" "-MT" ${flag_var} "${${flag_var}}")

        # When using Ninja with Clang, static builds pass -D_DLL on Windows.
        # This is incorrect and should not happen, so we fix that here.
        string(REPLACE "-D_DLL" "" ${flag_var} "${${flag_var}}")
      endif()

      # We prefer more strict warning checking for building Google Test.
      # Replaces /W3 with /W4 in defaults.
      string(REPLACE "/W3" "/W4" ${flag_var} "${${flag_var}}")

      # Prevent D9025 warning for targets that have exception handling
      # turned off (/EHs-c- flag). Where required, exceptions are explicitly
      # re-enabled using the cxx_exception_flags variable.
    #   string(REPLACE "/EHsc" "" ${flag_var} "${${flag_var}}")
    endforeach()
  endif()
endmacro(fix_msvc_crt_static_linkage)
