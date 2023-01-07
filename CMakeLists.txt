cmake_minimum_required(VERSION 3.10)

project(libajantv2)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(common/CMakeOptions)
include(ntv2/NTV2CompilerFlags)
include(ntv2/NTV2Defines)
include(ntv2/NTV2Helpers)
include(ntv2/NTV2Version)

option(AJA_NTV2_BUILD_OPENSOURCE "Build NTV2 SDK as open-source (MIT license)?" OFF)
option(AJA_NTV2_BUILD_SHARED     "Build NTV2 shared libraries?" OFF)
option(AJA_NTV2_DEPLOY_QT_LIBS   "Run winqtdeploy/macqtdeploy/patchelf to fix-up Qt library paths and deploy in build directory?" ON)
option(AJA_NTV2_INSTALL_SOURCES  "Deploy sources into CMake install directory?" OFF)
option(AJA_NTV2_INSTALL_HEADERS  "Deploy headers into CMake install directory?" ON)
option(AJA_NTV2_INSTALL_LIBS     "Deploy libs into CMake install directory?" OFF)
option(AJA_NTV2_INSTALL_CMAKE    "Deploy CMake build files (i.e. CMakeLists.txt, etc.) into CMake install directory?" ON)
option(AJA_NTV2_INSTALL_MISC     "Deploy misc build files for apps and libs into CMake install directory?" ON)
option(AJA_NTV2_SDK_GEN          "Run the ajantv2 `sdkgen` script before building (not available in NTV2 open-source)?" OFF)

# AJA CI system options, for internal use only
option(AJA_NTV2_CODE_SIGN               "Code sign binary outputs?" OFF) # for AJA internal CI builds only
option(AJA_NTV2_CI_USE_CLANG            "Build NTV2 SDK with LLVM Clang compiler?" OFF)
option(AJA_NTV2_CI_BUILD                "Enabled if building from a continuous integration system." OFF)
option(AJA_NTV2_CI_LINUX_GCC_NEW_ABI    "Build NTV2 SDK with GCC 5 or later?" OFF)

aja_add_subdirectory(ajantv2)
aja_add_subdirectory(demos)
aja_add_subdirectory(tools)
