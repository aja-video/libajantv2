if (NOT AJANTV2_BUILD_DEMOS)
    message(STATUS "Skipping libajantv2 demos...")
    return()
endif()

add_subdirectory(ntv2burn)
add_subdirectory(ntv2burn4kquadrant)
add_subdirectory(ntv2capture)
add_subdirectory(ntv2capture4k)
add_subdirectory(ntv2capture8k)
# # add_subdirectory(ntv2cudacapture) # requires gpustuff
add_subdirectory(ntv2dolbycapture)
add_subdirectory(ntv2dolbyplayer)
# # # add_subdirectory(ntv2encodehevc) # requires HEVC codecs
# # # add_subdirectory(ntv2encodehevcfile) # requires HEVC codecs
# # # add_subdirectory(ntv2encodehevcvif) # requires HEVC codecs
add_subdirectory(ntv2enumerateboards)
add_subdirectory(ntv2fieldburn)
add_subdirectory(ntv2hdrsetup)
if(AJA_QT_ENABLED)
    add_subdirectory(ntv2konaipj2ksetup)
    add_subdirectory(ntv2konaipjsonsetup)
endif()

add_subdirectory(ntv2llburn)
# # add_subdirectory(ntv2oglcapture) # requires gpustuff
# # add_subdirectory(ntv2ogldpxreader) # requires gpustuff
# # add_subdirectory(ntv2ogloutput) # requires gpustuff
add_subdirectory(ntv2outputtestpattern)
# # # add_subdirectory(ntv2playcorder)
add_subdirectory(ntv2player)
add_subdirectory(ntv2player4k)
add_subdirectory(ntv2player8k)
add_subdirectory(ntv2streamplayer)
# # add_subdirectory(ntv2qtplaycorder)
# # # add_subdirectory(ntv2qtrawcapture)
# # # add_subdirectory(ntv2qtstereopreview)
add_subdirectory(ntv2switchbitfile)

if(NOT AJANTV2_BUILD_OPENSOURCE)
    if(AJA_QT_ENABLED)
        add_subdirectory(ntv2qtmultiinput) 
        add_subdirectory(ntv2qtpreview) # optionally requires libajacc
        add_subdirectory(ntv2streampreview) # optionally requires libajacc
    endif()

    add_subdirectory(ntv2ccgrabber)  
    add_subdirectory(ntv2ccplayer)
endif()
    
# # add_subdirectory(ntv2testOGL)
if (AJA_BUILD_AMD)
    add_subdirectory(AMD)
endif()
if (AJA_BUILD_NVIDIA)
    add_subdirectory(NVIDIA)
endif()

if(AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/demos)
endif()
