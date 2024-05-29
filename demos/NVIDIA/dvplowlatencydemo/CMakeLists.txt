# NOTE: Ensure that the CUDA Toolkit v11.1 or later is installed and in your PATH.

if (AJA_DISABLE_NVIDIA OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	message(STATUS "skipping dvplowlatencydemo...")
	return()
endif()

find_package(CUDA 11.1)
if (NOT CUDA_FOUND)
	message(STATUS "skipping dvplowlatencydemo...CUDA not found")
	return()
endif()

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL)
if (NOT OPENGL_FOUND)
	message(STATUS "skipping dvplowlatencydemo...OpenGL not found")
	return()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	find_package(GLEW)
	if (NOT GLEW_FOUND)
		message(STATUS "skipping dvplowlatencydemo...GLEW not found")
		return()
	endif()
	find_package(X11)
	if (NOT X11_FOUND)
		message(STATUS "skipping dvplowlatencydemo...X11 not found")
		return()
	endif()
endif()

if (NOT CMAKE_CUDA_COMPILER)
	set(CMAKE_CUDA_COMPILER ${CUDA_NVCC_EXECUTABLE})
endif()

project(dvplowlatencydemo LANGUAGES CUDA CXX)

# Use AJA internal copy of GPUDirect if found...
if (EXISTS ${THIRDPARTY_DIR}/nvidia/gpudirect)
	set(NVIDIA_GPUDIRECT_DIR ${THIRDPARTY_DIR}/nvidia/gpudirect)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# ...otherwise look for official installation.
	if (NOT DEFINED NVIDIA_GPUDIRECT_DIR)
		set(NVIDIA_GPUDIRECT_DIR "C:\\Program Files (x86)\\NVIDIA Corporation\\NVIDIA GPU Direct for Video SDK\\win")
	endif()
endif()
if (NOT EXISTS ${NVIDIA_GPUDIRECT_DIR})
	message(STATUS "NVIDIA GPUDirect for Video SDK not found in: ${NVIDIA_GPUDIRECT_DIR}")
	return()
endif()

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../common
	${CMAKE_BINARY_DIR}/thirdparty/gpudirect/include
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajantv2
	${CMAKE_CURRENT_SOURCE_DIR}/../../../ajantv2/includes
	${CUDA_INCLUDE_DIRS})

set(TARGET_LINK_LIBS OpenGL::GL OpenGL::GLU)

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
	list(APPEND TARGET_INCLUDE_DIRS ${LIBAJANTV2_DIR}/thirdparty/glew)
	list(APPEND TARGET_LINK_LIBS dvp glew64)
	list(APPEND TARGET_LINK_DIRS
		${LIBAJANTV2_DIR}/thirdparty/glew/lib/x64
		${CMAKE_BINARY_DIR}/thirdparty/gpudirect/lib/x64)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND TARGET_LINK_LIBS dvp GLEW::GLEW X11 dl pthread rt)
	list(APPEND TARGET_LINK_DIRS ${CMAKE_BINARY_DIR}/thirdparty/gpudirect/lib64)
endif()

set(TARGET_SOURCES
	${DVPLOWLATENCYDEMO_SOURCES}
	${NVIDIA_COMMON_SOURCES})

add_executable(${PROJECT_NAME} ${TARGET_SOURCES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${TARGET_COMPILE_DEFS})
target_include_directories(${PROJECT_NAME} PRIVATE ${TARGET_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${TARGET_LINK_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ajantv2 ${TARGET_LINK_LIBS})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
	# Copy the includes and libs because we don't want the version of stdint.h or glew.lib from the SDK.
	# The version of glew that comes with the SDK is missing symbols that dvplowlatencydemo uses.
	set(_sdk_files
		"include/DVPAPI.h"
		"include/dvpapi_cuda.h"
		"include/dvpapi_d3d9.h"
		"include/dvpapi_d3d10.h"
		"include/dvpapi_d3d11.h"
		"include/dvpapi_gl.h"
		"lib/x64/dvp.lib"
		"lib/x64/dvp_static.lib")

	add_custom_target(copy_gpudirect_sdk)
	foreach(_file ${_sdk_files})
		set(_tgt_path ${CMAKE_BINARY_DIR}/thirdparty/gpudirect)
		message(STATUS "Copying NVIDIA GPUDirect for Video SDK: ${_file} => ${_tgt_path}")
		add_custom_command(TARGET copy_gpudirect_sdk PRE_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
			${NVIDIA_GPUDIRECT_DIR}/${_file} ${_tgt_path}/${_file})
	endforeach()
	add_dependencies(${PROJECT_NAME} copy_gpudirect_sdk)

	post_build_copy_file(${PROJECT_NAME}
		${NVIDIA_GPUDIRECT_DIR}/bin/x64/dvp.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/dvp.dll)
	post_build_copy_file(${PROJECT_NAME}
		${LIBAJANTV2_DIR}/thirdparty/glew/bin/x64/glew64.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/glew32.dll) # rename ... doh!
	post_build_copy_file(${PROJECT_NAME}
		${NVIDIA_GPUDIRECT_DIR}/bin/x64/msvcp100.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/msvcp100.dll)
	post_build_copy_file(${PROJECT_NAME}
		${NVIDIA_GPUDIRECT_DIR}/bin/x64/msvcr100.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/msvcr100.dll)
endif()

install(TARGETS ${PROJECT_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})
if (AJA_INSTALL_SOURCES)
	install(FILES ${DVPLOWLATENCYDEMO_HEADERS} 		DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/dvplowlatencydemo)
	install(FILES ${DVPLOWLATENCYDEMO_SOURCES} 		DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/dvplowlatencydemo)
	install(FILES ${DVPLOWLATENCYDEMO_BUILD_FILES} 	DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/dvplowlatencydemo)
	install(FILES ${NVIDIA_COMMON_SOURCES} 			DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/common)
	install(FILES ${NVIDIA_COMMONCUDA_SOURCES} 		DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/commonCUDA)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/dvplowlatencydemo)
endif()
if (AJA_INSTALL_MISC)
	install(FILES AJA_Logo_Small.png DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/dvplowlatencydemo)
endif()

if (AJA_CODE_SIGN)
    aja_code_sign(${PROJECT_NAME})
endif()
