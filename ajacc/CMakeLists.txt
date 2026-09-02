
cmake_minimum_required(VERSION 3.10)

project(ajacc)

option(AJACC_BUILD_SHARED "Build shared library target" OFF)

option(AJACC_DISABLE_TESTS "Build the unit tests" OFF)

set(LIBAJACC_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
list(APPEND CMAKE_MODULE_PATH "${LIBAJACC_CMAKE_DIR}")
list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)

include(CMakeOptions)
include(Defines)
include(Version)
include(Helpers)

# Cascade top-level options
if(DEFINED AJA_BUILD_SHARED)
    set(AJACC_BUILD_SHARED ${AJA_BUILD_SHARED})
endif()

if(DEFINED AJA_DISABLE_TESTS)
	set (AJACC_DISABLE_TESTS ${AJA_DISABLE_TESTS})
endif()

set(TARGET_INCLUDE_DIRS
    ../libajantv2
    ../libajantv2/ajaanc/includes
    ../libajantv2/ajabase
    ../libajantv2/ajantv2
    ../libajantv2/ajantv2/includes
    includes
)

set(AJACC_HEADERS
    includes/ajacc.h
    includes/ccfont.h
    includes/ntv2caption608dataqueue.h
    includes/ntv2caption608message.h
    includes/ntv2caption608messagequeue.h
    includes/ntv2caption608types.h
    includes/ntv2caption708service.h
    includes/ntv2caption708serviceblockqueue.h
    includes/ntv2caption708serviceinfo.h
    includes/ntv2caption708window.h
    includes/ntv2captiondecodechannel608.h
    includes/ntv2captiondecoder608.h
    includes/ntv2captiondecoder708.h
    includes/ntv2captionencoder608.h
    includes/ntv2captionencoder708.h
    includes/ntv2captionlogging.h
    includes/ntv2captionrenderer.h
    includes/ntv2captiontranslator608to708.h
    includes/ntv2captiontranslator708to708.h
    includes/ntv2captiontranslatorchannel608to708.h
    includes/ntv2line21captioner.h
    includes/ntv2smpteancdata.h
    includes/ntv2srt.h
    includes/ntv2xdscaptiondecodechannel608.h)

set(AJACC_SOURCES
    src/ccfont.cpp
    src/ntv2caption608dataqueue.cpp
    src/ntv2caption608message.cpp
    src/ntv2caption608messagequeue.cpp
    src/ntv2caption608types.cpp
    src/ntv2caption708service.cpp
    src/ntv2caption708serviceblockqueue.cpp
    src/ntv2caption708serviceinfo.cpp
    src/ntv2caption708window.cpp
    src/ntv2captiondecodechannel608.cpp
    src/ntv2captiondecoder608.cpp
    src/ntv2captiondecoder708.cpp
    src/ntv2captionencoder608.cpp
    src/ntv2captionencoder708.cpp
    src/ntv2captionlogging.cpp
    src/ntv2captionrenderer.cpp
    src/ntv2captiontranslator608to708.cpp
    src/ntv2captiontranslator708to708.cpp
    src/ntv2captiontranslatorchannel608to708.cpp
    src/ntv2line21captioner.cpp
    src/ntv2smpteancdata.cpp
    src/ntv2srt.cpp
    src/ntv2xdscaptiondecodechannel608.cpp)

set(TARGET_SOURCES ${AJACC_SOURCES} ${AJACC_HEADERS})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND TARGET_COMPILE_DEFS
        -D_MBCS
        -DCRT_NONSTDC_DEPRECATE
        -D_WINSOCK_DEPRECATED_NO_WARNINGS
        -DAJA_NO_AUTOIMPORT)
    list(APPEND TARGET_COMPILE_DEFS_DYNAMIC
        -DAJA_WINDLL)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
endif()

list(APPEND TARGET_COMPILE_DEFS_STATIC
    ${TARGET_COMPILE_DEFS}
    -DAJASTATIC)
list(APPEND TARGET_COMPILE_DEFS_DYNAMIC
    ${TARGET_COMPILE_DEFS}
    -DAJADLL
    -DAJADLL_BUILD)

if (NOT TARGET ${PROJECT_NAME})
    if (AJACC_BUILD_SHARED)
        message(STATUS "AJA Version: ${AJA_NTV2_VER_STR}")

        add_library(${PROJECT_NAME} SHARED ${TARGET_SOURCES})
        add_dependencies(${PROJECT_NAME} ajantv2)
        target_compile_definitions(${PROJECT_NAME} PUBLIC
            ${TARGET_COMPILE_DEFS_DYNAMIC}
            ${AJACC_TARGET_COMPILE_DEFS})
        target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LINK_LIBS} ajantv2)
        if (AJA_CODE_SIGN)
            aja_code_sign(${PROJECT_NAME})
        endif()
        set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${AJA_NTV2_VER_STR}")
    else()
        add_library(${PROJECT_NAME} STATIC ${TARGET_SOURCES})
        add_dependencies(${PROJECT_NAME} ajantv2)
        target_compile_definitions(${PROJECT_NAME} PUBLIC
            ${TARGET_COMPILE_DEFS_STATIC}
            ${AJACC_TARGET_COMPILE_DEFS})
        target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PUBLIC ${TARGET_LINK_LIBS} ajantv2)
    endif()
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		aja_get_msvc_tools_version()
		if(CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreaded" OR
		   CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDebug")
			set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_vs${AJA_MSVC_TOOLSET_VERSION}_MT")
		elseif(CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDLL" OR
			   CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDebugDLL")
			set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_vs${AJA_MSVC_TOOLSET_VERSION}_MD")
		endif()
	endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d")
endif()

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (AJA_INSTALL_HEADERS)
    install(FILES ${AJACC_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajacc/includes)
endif()
if (AJA_INSTALL_SOURCES)
    install(FILES ${AJACC_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajacc/src)
endif()
if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajacc)
endif()

if (NOT AJACC_DISABLE_TESTS)
    message(STATUS "Adding libajacc tests")
    include(CTest)
    add_subdirectory(test)
endif()
