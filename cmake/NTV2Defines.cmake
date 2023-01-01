include(include/Helpers)

# Common preprocessor defines
if(CMAKE_BUILD_TYPE MATCHES Debug)
    aja_message(STATUS "Build Type: Debug")
    list(APPEND AJA_NTV2_TARGET_COMPILE_DEFS
        -DAJA_DEBUG
        -D_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    aja_message(STATUS "Build Type: Release with Debug Symbols")
    list(APPEND AJA_NTV2_TARGET_COMPILE_DEFS
        -DNDEBUG)
else()
    aja_message(STATUS "Build Type: Release")
    list(APPEND AJA_NTV2_TARGET_COMPILE_DEFS
        -DNDEBUG)
endif()
# Platform-specific preprocessor defines
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND AJA_NTV2_TARGET_COMPILE_DEFS
        -DAJA_WINDOWS
        -DMSWindows
        -D_WINDOWS
        -D_CONSOLE
        -DUNICODE
        -D_UNICODE
        -DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        -D_SCL_SECURE_NO_WARNINGS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND AJA_NTV2_TARGET_COMPILE_DEFS
        -DAJALinux
        -DAJA_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND AJA_NTV2_TARGET_COMPILE_DEFS
        -DAJAMac
        -DAJA_MAC
        -D__STDC_CONSTANT_MACROS)
endif()
