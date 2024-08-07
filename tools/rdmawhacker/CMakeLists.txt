if (AJA_DISABLE_NVIDIA OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
	message(STATUS "skipping rdmawhacker...")
	return()
endif()

find_package(CUDA 11.1)
if (NOT CUDA_FOUND)
	message(STATUS "skipping rdmawhacker (CUDA not found)")
	return()
endif()

if (NOT CMAKE_CUDA_COMPILER)
	set(CMAKE_CUDA_COMPILER ${CUDA_NVCC_EXECUTABLE})
endif()
project(rdmawhacker LANGUAGES CUDA CXX)

set(DEMOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../demos)
set(TARGET_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${DEMOS_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../ajantv2/includes
	${CUDA_INCLUDE_DIRS})

set(DEMO_APPS_SOURCES
	${DEMOS_DIR}/ntv2democommon.cpp)
set(DEMO_APPS_HEADERS
	${DEMOS_DIR}/ntv2democommon.h)

set(RDMAWHACKER_SOURCES
    rdmawhacker.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# noop
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(FOUNDATION_FRAMEWORK Foundation)
	set(TARGET_LINK_LIBS ${FOUNDATION_FRAMEWORK})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TARGET_COMPILE_DEFS
        -DAJA_RDMA)
    set(TARGET_LINK_LIBS cuda ${CUDA_LIBRARIES} dl pthread rt)
endif()

set(TARGET_SOURCES
	${RDMAWHACKER_SOURCES})

message(STATUS "CUDA Libs: ${CUDA_LIBRARIES}")

add_executable(${PROJECT_NAME} ${TARGET_SOURCES})
add_dependencies(${PROJECT_NAME} ajantv2)
target_compile_definitions(${PROJECT_NAME} PRIVATE ${TARGET_COMPILE_DEFS})
target_include_directories(${PROJECT_NAME} PRIVATE ${TARGET_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${TARGET_LINK_LIBS} ajantv2)

if (AJA_CODE_SIGN)
    aja_code_sign(${PROJECT_NAME})
endif()
install(TARGETS ${PROJECT_NAME}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (AJA_INSTALL_SOURCES)
	install(FILES ${RDMAWHACKER_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/tools/rdmawhacker)
endif()
if (AJA_INSTALL_CMAKE)
	install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/tools/rdmawhacker)
endif()
if (AJA_INSTALL_MISC)
	install(FILES Makefile DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/tools/rdmawhacker)
endif()
