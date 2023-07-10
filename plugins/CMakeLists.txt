#project(ntv2plugins)
option(AJA_PLUGINS_DEPLOY_ON_DEBUG "Put the plug-ins in the correct location for local development" OFF)
option(AJA_PLUGINS_BUILD_NTV2 "Build the AJA NTV2 Plug-ins" ON)

if (AJA_PLUGINS_BUILD_NTV2)
    message(STATUS "Adding swdevice & nublegacy")
    aja_add_subdirectory(swdevice "AJA SWDevice Plugin")
    aja_add_subdirectory(nublegacy "Legacy Nub Plugin")
endif()

if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/plugins)
endif()
