project(ntv2nublegacy)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PLUGIN_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(RPCLIB_INCLUDE ${NON_AJA_RPCLIB_ROOT}/include)

set(TARGET_INCLUDE_DIRS
    ${AJA_LIBRARIES_ROOT}
    ${AJA_LIB_NTV2_ROOT}/includes
    ${AJA_DEMO_COMMON_ROOT}
)

set(PLUGIN_HEADERS
	${PLUGIN_ROOT}/ntv2legacycommon.h
)

set(PLUGIN_SOURCES
	${PLUGIN_ROOT}/ntv2legacyclient.cpp
	${PLUGIN_ROOT}/ntv2legacyserver.cpp
	${PLUGIN_ROOT}/ntv2legacycommon.cpp
)

set(PLUGIN_EXTRA_SOURCES
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TARGET_INCLUDE_DIRS
            ${TARGET_INCLUDE_DIRS}
            ${AJA_LIB_NTV2_ROOT}/src/win)

    set(TARGET_LINK_LIBS
        advapi32 shell32 shlwapi user32 version ws2_32 wsock32)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TARGET_INCLUDE_DIRS
            ${TARGET_INCLUDE_DIRS}
            ${AJA_LIB_NTV2_ROOT}/src/lin)

    set(TARGET_LINK_LIBS
            dl pthread rt)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(TARGET_INCLUDE_DIRS
            ${TARGET_INCLUDE_DIRS}
            ${AJA_LIB_NTV2_ROOT}/src/mac)

    find_library(APP_SERVICES_FRAMEWORK ApplicationServices)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(IOKIT_FRAMEWORK IoKit)

    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS}
        ${APP_SERVICES_FRAMEWORK} ${COCOA_FRAMEWORK} ${CORE_FOUNDATION_FRAMEWORK} ${FOUNDATION_FRAMEWORK} ${IOKIT_FRAMEWORK})
endif()

set(TARGET_SOURCES
	${PLUGIN_SOURCES}
	${PLUGIN_HEADERS}
	${PLUGIN_EXTRA_SOURCES})

# don't want the binary to have a 'd' appended to the end for this plug-in
unset(CMAKE_DEBUG_POSTFIX)

add_library(${PROJECT_NAME} SHARED ${TARGET_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${TARGET_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${TARGET_LINK_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ajantv2  ${TARGET_LINK_LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${TARGET_COMPILE_DEFS})
# set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "ntv2nublegacy")    <== this doesn't seem to have any effect!
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(AJA_PLUGINS_DEPLOY_ON_DEBUG AND CMAKE_BUILD_TYPE STREQUAL Debug)
		# put the plug-in in the correct location for NTV2 SDK to find it at runtime
		set(PLUGIN_OUTPATH C:/Program\ Files/AJA/)
		if(EXISTS ${PLUGIN_OUTPATH})
			set_target_properties(${PROJECT_NAME} PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_OUTPATH}
				)
		endif()
	endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(AJA_PLUGINS_DEPLOY_ON_DEBUG AND CMAKE_BUILD_TYPE STREQUAL Debug)
		# put the plug-in in the correct location for NTV2 SDK to find it at runtime
		set(PLUGIN_OUTPATH /opt/aja/)
		if(EXISTS ${PLUGIN_OUTPATH})
			set_target_properties(${PROJECT_NAME} PROPERTIES
				PREFIX ""
				LIBRARY_OUTPUT_NAME "nublegacy"
				LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPATH}
				)
		endif()
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	if(AJA_PLUGINS_DEPLOY_ON_DEBUG AND CMAKE_BUILD_TYPE STREQUAL Debug)
		# put the plug-in in the correct location for NTV2 SDK to find it at runtime
		set(BUNDLE_OUTPUT_DIR /Library/Application\ Support/AJA)
		if(EXISTS ${BUNDLE_OUTPUT_DIR})
			set_target_properties(${PROJECT_NAME} PROPERTIES
				PREFIX ""
				LIBRARY_OUTPUT_NAME "nublegacy"
				LIBRARY_OUTPUT_DIRECTORY ${BUNDLE_OUTPUT_DIR}
				)
		endif()
	endif()

        if(NOT BUNDLE_OUTPUT_DIR)
            set(BUNDLE_OUTPUT_DIR ".")
        endif()
endif()

add_dependencies(${PROJECT_NAME} ajantv2)
