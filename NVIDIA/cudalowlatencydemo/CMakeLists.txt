# NOTE: Ensure that the CUDA Toolkit v11.4 or later is installed and in your PATH.
include(AJAHelpers)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
	aja_message(STATUS "skipping cudalowlatencydemo (requires Linux)")
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

project(cudalowlatencydemo LANGUAGES CUDA CXX)

set(TARGET_COMPILE_DEFS
	-DAJA_LINUX
	-DAJALinux
	-DAJA_RDMA=1)

set(TARGET_INCLUDE_DIRS
	${AJA_APPS_ROOT}/crossplatform/demoapps/NVIDIA/commonCUDA
	${AJA_APPS_ROOT}/crossplatform/demoapps
	${AJA_LIBRARIES_ROOT}
	${AJA_LIB_BASE_ROOT}/system
	${AJA_LIB_NTV2_ROOT}/includes
	${CUDA_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIRS}
	${GLEW_INCLUDE_DIRS})

set(DEMO_APPS_SOURCES
	${AJA_APPS_ROOT}/crossplatform/demoapps/ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${AJA_APPS_ROOT}/crossplatform/demoapps/ntv2democommon.h)

set(CUDALOWLATENCYDEMO_BUILD_FILES
	Makefile
	kernel.cu)
set(CUDALOWLATENCYDEMO_SOURCES
	cudaapp.cpp)

set(NVIDIA_COMMONCUDA_SOURCES
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/assertion.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/cudaUtils.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/gpuvio.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2errorlist.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2errorlist.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransfer.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransferNV.cpp
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpucircularbuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpucircularbuffer.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2rendertotexture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2rendertotexture.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2texture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2texture.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/oglview.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/oglview.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/opengl.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simplecudavio.cpp
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simplecudavio.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simplegpuvio.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simplegpuvio.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simpleoglvio.cpp
	# ${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simpleoglvio.h
)

set(TARGET_SOURCES
	${CUDALOWLATENCYDEMO_SOURCES}
	${NVIDIA_COMMONCUDA_SOURCES}
	${DEMO_APPS_HEADERS}
	${DEMO_APPS_SOURCES})

	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_library(FOUNDATION_FRAMEWORK Foundation)
    set(TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(TARGET_LINK_LIBS X11 OpenGL::GL OpenGL::GLU GLEW::GLEW cuda dl pthread rt)
	# cuda_include_directories(${TARGET_INCLUDE_DIRS})
	# cuda_compile(CUDA_OBJ kernel.cu)
	add_executable(cudalowlatencydemo ${TARGET_SOURCES} kernel.cu)
	add_dependencies(cudalowlatencydemo ${CUDA_OBJ} ajantv2)
	target_compile_definitions(cudalowlatencydemo PRIVATE ${TARGET_COMPILE_DEFS})
	target_include_directories(cudalowlatencydemo PRIVATE ${TARGET_INCLUDE_DIRS})
	target_link_libraries(cudalowlatencydemo PRIVATE ajantv2 ${TARGET_LINK_LIBS})
	# add_custom_command(
	# 	TARGET cudalowlatencydemo
	# 	PRE_BUILD
	# 	COMMAND ${CUDA_HOST_COMPILER} -c -I${AJA_APPS_ROOT}/crossplatform/demoapps/NVIDIA/common -I${AJA_APPS_ROOT}/crossplatform/demoapps/NVIDIA/commonCUDA -I${AJA_APPS_ROOT}/crossplatform/demoapps -I${AJA_LIBRARIES_ROOT} -I${AJA_LIB_NTV2_ROOT}/includes -I${CUDA_INCLUDE_DIRS} -I${OPENGL_INCLUDE_DIRS} -I${GLEW_INCLUDE_DIRS} -I${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA kernel.cu
	# 	DEPENDS kernel.cu
	# 	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	# 	VERBATIM)
endif()

if (AJA_INSTALL_SOURCES)
	install(FILES ${CUDALOWLATENCYDEMO_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/NVIDIA/cudalowlatencydemo)
	install(FILES ${CUDALOWLATENCYDEMO_BUILD_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/NVIDIA/cudalowlatencydemo)
	install(FILES ${NVIDIA_COMMONCUDA_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajaapps/crossplatform/demoapps/NVIDIA/commonCUDA)
endif()

