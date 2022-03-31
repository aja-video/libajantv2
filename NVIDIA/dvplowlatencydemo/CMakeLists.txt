include(AJAHelpers)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
	aja_message(STATUS "skipping dvplowlatencydemo (requires Linux)")
	return()
endif()
find_package(CUDA 11.2)
if (NOT CUDA_FOUND)
	aja_message(STATUS "skipping cudalowlatencydemo (CUDA not found)")
	return()
endif()
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL)
if (NOT OPENGL_FOUND)
	aja_message(STATUS "skipping cudalowlatencydemo (OpenGL not found)")
	return()
endif()
find_package(GLEW)
if (NOT GLEW_FOUND)
	aja_message(STATUS "skipping cudalowlatencydemo (GLEW not found)")
	return()
endif()
find_package(X11)
if (NOT X11_FOUND)
	aja_message(STATUS "skipping cudalowlatencydemo (X11 not found)")
	return()
endif()

project(dvplowlatencydemo LANGUAGES CUDA CXX)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${AJA_APPS_ROOT}/crossplatform/demoapps/NVIDIA/common
	${AJA_LIBRARIES_ROOT}
	${AJA_LIBRARIES_ROOT}/gpustuff/include
	${CUDA_INCLUDE_DIRS}
	${AJA_LIB_NTV2_ROOT}/includes)

set(DVPLOWLATENCYDEMO_BUILD_FILES
	Makefile)
set(DVPLOWLATENCYDEMO_HEADERS
	AJA_Logo_Small.h)
set(DVPLOWLATENCYDEMO_SOURCES
	oglapp.cpp)
set(NVIDIA_COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2errorlist.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2errorlist.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2glTextureTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2glTextureTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2glTextureTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2gpucircularbuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2gpucircularbuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2rendertotexture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2rendertotexture.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2texture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../common/ntv2texture.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/oglview.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../common/oglview.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/opengl.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/simplegpuvio.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../common/simplegpuvio.h)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_library(FOUNDATION_FRAMEWORK Foundation)
    set(TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS X11 OpenGL::GL OpenGL::GLU GLEW::GLEW dvp dl pthread rt)
	set(TARGET_LINK_DIRS
		${TARGET_LINK_DIRS}
		${AJA_LIBRARIES_ROOT}/gpustuff/lib/linux/lib64)
endif()

set(TARGET_SOURCES
	${DVPLOWLATENCYDEMO_SOURCES}
	${NVIDIA_COMMON_SOURCES})

add_executable(dvplowlatencydemo ${TARGET_SOURCES})
add_dependencies(dvplowlatencydemo ajantv2)
target_include_directories(dvplowlatencydemo PRIVATE ${TARGET_INCLUDE_DIRS})
target_link_directories(dvplowlatencydemo PRIVATE ${TARGET_LINK_DIRS})
target_link_libraries(dvplowlatencydemo PRIVATE ajantv2 ${TARGET_LINK_LIBS})

if (AJA_INSTALL_SOURCES)
	install(FILES ${DVPLOWLATENCYDEMO_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/NVIDIA/dvplowlatencydemo)
	install(FILES ${DVPLOWLATENCYDEMO_BUILD_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/NVIDIA/dvplowlatencydemo)
	install(FILES ${NVIDIA_COMMON_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/NVIDIA/common)
	install(FILES ${NVIDIA_COMMONCUDA_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/NVIDIA/commonCUDA)
endif()

