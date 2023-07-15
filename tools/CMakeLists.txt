if (NOT AJANTV2_BUILD_TOOLS)
    message(STATUS "Skipping libajantv2 tools...")
    return()
endif()

add_subdirectory(logreader)
add_subdirectory(ntv2firmwareinstaller)
add_subdirectory(ntv2thermo)
add_subdirectory(pciwhacker)
add_subdirectory(rdmawhacker)
add_subdirectory(regio)
add_subdirectory(supportlog)


if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/tools)
endif()
