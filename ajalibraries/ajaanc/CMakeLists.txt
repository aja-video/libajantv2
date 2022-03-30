project(ajaanc)

set(AJAANC_TARGET_INCLUDE_DIRS
    ${AJA_LIBRARIES_ROOT}
    ${AJA_LIB_ANC_ROOT}/includes
    ${AJA_LIB_BASE_ROOT}/includes
    ${AJA_LIB_NTV2_ROOT}/includes)

set(AJAANC_HEADERS
    includes/ancillarydata_cea608.h
    includes/ancillarydata_cea608_line21.h
    includes/ancillarydata_cea608_vanc.h
    includes/ancillarydata_cea708.h
    includes/ancillarydatafactory.h
    includes/ancillarydata_framestatusinfo524D.h
    includes/ancillarydata_framestatusinfo5251.h
    includes/ancillarydata.h
    includes/ancillarydata_hdr_hdr10.h
    includes/ancillarydata_hdr_hlg.h
    includes/ancillarydata_hdr_sdr.h
    includes/ancillarydata_timecode_atc.h
    includes/ancillarydata_timecode.h
    includes/ancillarydata_timecode_vitc.h
    includes/ancillarylist.h
)

set(AJAANC_SOURCES
    src/ancillarydata_cea608.cpp
    src/ancillarydata_cea608_line21.cpp
    src/ancillarydata_cea608_vanc.cpp
    src/ancillarydata_cea708.cpp
    src/ancillarydata.cpp
    src/ancillarydatafactory.cpp
    src/ancillarydata_framestatusinfo524D.cpp
    src/ancillarydata_framestatusinfo5251.cpp
    src/ancillarydata_hdr_hdr10.cpp
    src/ancillarydata_hdr_hlg.cpp
    src/ancillarydata_hdr_sdr.cpp
    src/ancillarydata_timecode_atc.cpp
    src/ancillarydata_timecode.cpp
    src/ancillarydata_timecode_vitc.cpp
    src/ancillarylist.cpp
)

set(AJAANC_TARGET_SOURCES ${AJAANC_SOURCES} ${AJAANC_HEADERS})

add_library(ajaanc STATIC ${AJAANC_TARGET_SOURCES})
target_include_directories(ajaanc PUBLIC ${AJAANC_TARGET_INCLUDE_DIRS})
target_link_libraries(ajaanc PUBLIC ajantv2)

if (AJA_BUILD_SHARED)
    add_library(ajaancshared SHARED ${AJAANC_TARGET_SOURCES})
    target_include_directories(ajaancshared PUBLIC ${AJAANC_TARGET_INCLUDE_DIRS})
    target_link_libraries(ajaancshared PUBLIC ajantv2shared)
endif()
