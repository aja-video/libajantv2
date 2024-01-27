option(AJA_INSTALL_SOURCES "Deploy sources into CMake install directory?" OFF)
option(AJA_INSTALL_HEADERS "Deploy headers into CMake install directory?" ON)
option(AJA_INSTALL_LIBS    "Deploy libs into CMake install directory?" OFF)
option(AJA_INSTALL_CMAKE   "Deploy CMake build files (i.e. CMakeLists.txt, etc.) into CMake install directory?" ON)
option(AJA_INSTALL_MISC    "Deploy misc build files for apps and libs into CMake install directory?" ON)
option(AJA_QT_ENABLED      "Build AJA QT Targets (apps/demos)" OFF)
option(AJA_QT_DEPLOY  "Run winqtdeploy/macqtdeploy/patchelf to fix-up Qt library paths and deploy in build directory?" ON)

# AJA CI system options, for internal use only
option(AJA_CODE_SIGN            "Code sign binary outputs?" OFF) # for AJA internal CI builds only
option(AJA_CI_USE_CLANG         "Build NTV2 SDK with LLVM Clang compiler?" OFF)
option(AJA_CI_BUILD             "Enabled if building from a continuous integration system." OFF)
option(AJA_CI_LINUX_GCC_NEW_ABI "Build NTV2 SDK with GCC 5 or later?" OFF)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()
if (NOT DEFINED CMAKE_C_STANDARD)
    set(CMAKE_C_STANDARD 99)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(AJA_BITS 64)
else()
    set(AJA_BITS 32)
endif()

# macOS Build Settings
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # TODO(paulh): Generate macOS Universal (multi-architecture) builds.
    # This will require Qt6+ for the AJA Retail apps.
    # e.g. set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    if (DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
        set(CMAKE_OSX_DEPLOYMENT_TARGET $ENV{MACOSX_DEPLOYMENT_TARGET} CACHE STRING "Minimum macOS deployment version" FORCE)
    else()
        if (NOT CMAKE_OSX_DEPLOYMENT_TARGET)
            set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version" FORCE)
        endif()
    endif()
    # Get the macOS SDK version
    get_filename_component(MACOS_SDK_NAME ${CMAKE_OSX_SYSROOT} NAME_WLE)
    string(REPLACE "MacOSX" "" MACOS_SDK_VERSION ${MACOS_SDK_NAME})
    string(REPLACE "." ";" MACOS_SDK_VERSION_LIST ${MACOS_SDK_VERSION})
    list(GET MACOS_SDK_VERSION_LIST 0 MACOS_SDK_VERSION_MAJOR)
    list(GET MACOS_SDK_VERSION_LIST 1 MACOS_SDK_VERSION_MINOR)
    set(MACOS_FRAMEWORKS_DIR ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks)
endif()

option (AJA_FORCE_ANSI_COLORS "Force ANSI terminal coloration when building with Ninja and GNU/Clang?" ON)
if (${AJA_FORCE_ANSI_COLORS} AND ${CMAKE_GENERATOR} STREQUAL "Ninja")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif()
endif()
