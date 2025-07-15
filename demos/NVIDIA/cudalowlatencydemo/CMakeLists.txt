# NOTE: Ensure that the CUDA Toolkit v11.1 or later is installed and in your PATH.

if (AJA_DISABLE_NVIDIA)
	message(STATUS "skipping cudalowlatencydemo...")
	return()
endif()
if (NOT CMAKE_SYSTEM_NAME MATCHES "^(Windows|Linux)$")
	message(STATUS "skipping cudalowlatencydemo...requires Windows or Linux")
	return()
endif()

find_package(CUDA 11.1)
if (NOT CUDA_FOUND)
	message("skipping cudalowlatencydemo...CUDA not found")
	return()
endif()

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL)
if (NOT OPENGL_FOUND)
	message(STATUS "skipping cudalowlatencydemo...OpenGL not found")
	return()
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	find_package(GLEW)
	if (NOT GLEW_FOUND)
		message(STATUS "skipping cudalowlatencydemo...GLEW not found")
		return()
	endif()
	find_package(X11)
	if (NOT X11_FOUND)
		message(STATUS "skipping cudalowlatencydemo...X11 not found")
		return()
	endif()
endif()

if (NOT CMAKE_CUDA_COMPILER)
	set(CMAKE_CUDA_COMPILER ${CUDA_NVCC_EXECUTABLE})
	message(STATUS "CUDA Compiler override: ${CUDA_NVCC_EXECUTABLE}")
endif()

if (NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    if(CUDA_VERSION VERSION_GREATER_EQUAL "11.8")
        set(CMAKE_CUDA_ARCHITECTURES 50;60;70;80;90)
        message(STATUS "Define default CMAKE_CUDA_ARCHITECTURES=${CMAKE_CUDA_ARCHITECTURES}")
    else()
        message(STATUS "CMAKE_CUDA_ARCHITECTURES not defined")
    endif()  
endif()

project(cudalowlatencydemo LANGUAGES CUDA CXX)

set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${AJA_LIBRARIES_ROOT}
	${AJA_LIB_BASE_ROOT}/system
	${AJA_LIB_NTV2_ROOT}/includes
	${CUDA_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIRS}
	${GLEW_INCLUDE_DIRS})

list(APPEND TARGET_LINK_LIBS OpenGL::GL OpenGL::GLU cuda)

set(DEMO_APPS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../../ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../../ntv2democommon.h)

set(CUDALOWLATENCYDEMO_BUILD_FILES
	Makefile
	kernel.cu)
set(CUDALOWLATENCYDEMO_SOURCES
	cudaapp.cpp)

set(NVIDIA_COMMONCUDA_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/assertion.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/cudaUtils.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2errorlist.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2errorlist.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpucircularbuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpucircularbuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2rendertotexture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2rendertotexture.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2texture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2texture.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/oglview.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/oglview.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/opengl.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simplecudavio.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simplecudavio.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simplegpuvio.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simplegpuvio.h
)

set(TARGET_SOURCES
	${CUDALOWLATENCYDEMO_SOURCES}
	${NVIDIA_COMMONCUDA_SOURCES}
	${DEMO_APPS_HEADERS}
	${DEMO_APPS_SOURCES})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	list(APPEND TARGET_INCLUDE_DIRS ${LIBAJANTV2_DIR}/thirdparty/glew)
	list(APPEND TARGET_LINK_LIBS glew64)
	list(APPEND TARGET_LINK_DIRS
		${LIBAJANTV2_DIR}/thirdparty/glew/lib/x64)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (NOT AJANTV2_DISABLE_RDMA)
    	list(APPEND TARGET_COMPILE_DEFS -DAJA_RDMA=1)
    endif()  
	list(APPEND TARGET_LINK_LIBS GLEW::GLEW X11 dl pthread rt)
endif()

add_executable(${PROJECT_NAME} ${TARGET_SOURCES} kernel.cu)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${TARGET_COMPILE_DEFS})
target_include_directories(${PROJECT_NAME} PRIVATE ${TARGET_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${TARGET_LINK_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ajantv2 ${TARGET_LINK_LIBS})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
	post_build_copy_file(${PROJECT_NAME}
		${LIBAJANTV2_DIR}/thirdparty/glew/bin/x64/glew64.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/glew32.dll) # rename ... doh!
endif()

# Extra sources that were in the commonCUDA dir, but don't seem to actually be required to build cudalowlatencydemo or dvplowlatencydemo.
list(APPEND NVIDIA_COMMONCUDA_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/gpuvio.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simpleoglvio.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simpleoglvio.h)
if (AJA_INSTALL_SOURCES)
	install(FILES ${CUDALOWLATENCYDEMO_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/cudalowlatencydemo)
	install(FILES ${CUDALOWLATENCYDEMO_BUILD_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/cudalowlatencydemo)
	install(FILES ${NVIDIA_COMMONCUDA_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/commonCUDA)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/demos/NVIDIA/cudalowlatencydemo)
endif()

install(TARGETS ${PROJECT_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (AJA_CODE_SIGN)
    aja_code_sign(${PROJECT_NAME})
endif()
