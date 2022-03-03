project(cudalowlatencydemo)

message("added cudalowlatencydemo")

set(CUDALOWLATENCYDEMO_TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${AJA_LIBRARIES_ROOT}
	${AJA_LIB_NTV2_ROOT}/includes)

set(CUDALOWLATENCYDEMO_BUILD_FILES
	Makefile
	kernel.cu)
set(CUDALOWLATENCYDEMO_SOURCES
	cudaapp.cpp)
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
set(NVIDIA_COMMONCUDA_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/assertion.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/cudaUtils.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/gpuvio.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2cudaArrayTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2errorlist.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2errorlist.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2glTextureTransferNV.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpucircularbuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpucircularbuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransfer.h
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransferNV.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/ntv2gpuTextureTransferNV.h
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
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simpleoglvio.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../commonCUDA/simpleoglvio.h)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_library(FOUNDATION_FRAMEWORK Foundation)
    set(EXTRA_LIBRARIES ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(EXTRA_LIBRARIES cuda cudart dl pthread rt)
endif()

set(CUDALOWLATENCYDEMO_TARGET_SOURCES
	${CUDALOWLATENCYDEMO_SOURCES}
	${NVIDIA_COMMON_SOURCES}
	${NVIDIA_COMMONCUDA_SOURCES})

# add_executable(cudalowlatencydemo ${CUDALOWLATENCYDEMO_TARGET_SOURCES})
# add_dependencies(cudalowlatencydemo ajantv2)
# target_include_directories(cudalowlatencydemo PUBLIC ${CUDALOWLATENCYDEMO_TARGET_INCLUDE_DIRS})
# target_link_libraries(cudalowlatencydemo PUBLIC ajantv2 ${EXTRA_LIBRARIES})

if (AJA_INSTALL_SOURCES)
	install(FILES ${CUDALOWLATENCYDEMO_SOURCES} DESTINATION ${AJA_INSTALL_INCLUDEDIR}/ajaapps/crossplatform/demoapps/NVIDIA/cudalowlatencydemo)
	install(FILES ${CUDALOWLATENCYDEMO_BUILD_FILES} DESTINATION ${AJA_INSTALL_INCLUDEDIR}/ajaapps/crossplatform/demoapps/NVIDIA/cudalowlatencydemo)
	install(FILES ${NVIDIA_COMMON_SOURCES} DESTINATION ${AJA_INSTALL_INCLUDEDIR}/ajaapps/crossplatform/demoapps/NVIDIA/common)
	install(FILES ${NVIDIA_COMMONCUDA_SOURCES} DESTINATION ${AJA_INSTALL_INCLUDEDIR}/ajaapps/crossplatform/demoapps/NVIDIA/commonCUDA)
endif()
