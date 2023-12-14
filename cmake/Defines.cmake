include_guard(GLOBAL)

# Common preprocessor defines
if(CMAKE_BUILD_TYPE MATCHES Debug)
    list(APPEND AJANTV2_TARGET_COMPILE_DEFS
        -DAJA_DEBUG
        -D_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    list(APPEND AJANTV2_TARGET_COMPILE_DEFS
        -DNDEBUG)
else()
    list(APPEND AJANTV2_TARGET_COMPILE_DEFS
        -DNDEBUG)
endif()

# Platform-specific preprocessor defines
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND AJANTV2_TARGET_COMPILE_DEFS
        -DAJA_WINDOWS
        -DMSWindows
        -D_WINDOWS
        -D_CONSOLE
        -DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        -D_SCL_SECURE_NO_WARNINGS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND AJANTV2_TARGET_COMPILE_DEFS
        -DAJALinux
        -DAJA_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND AJANTV2_TARGET_COMPILE_DEFS
        -DAJAMac
        -DAJA_MAC
        -D__STDC_CONSTANT_MACROS)
elseif (CMAKE_SYSTEM_NAME STREQUAL "BareMetal")
    list(APPEND AJANTV2_TARGET_COMPILE_DEFS
      -DNTV2_USE_STDINT
      -DAJABareMetal
      -DAJA_BAREMETAL)
endif()
