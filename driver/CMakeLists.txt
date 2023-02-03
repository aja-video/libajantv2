set(AJADRIVER_COMMON_HEADERS
	ntv2anc.h
	ntv2audio.h
	ntv2autocirc.h
	ntv2autofunc.h
	ntv2commonreg.h
	ntv2displayid.h
	ntv2genlock.h
	ntv2genregs.h
	ntv2hdmiedid.h
	ntv2hdmiin.h
	ntv2hdmiin4.h
	ntv2hdmiout4.h
	ntv2hin4reg.h
	ntv2hinreg.h
	ntv2hout4reg.h
	ntv2infoframe.h
	ntv2kona.h
	ntv2mcap.h
	ntv2pciconfig.h
	ntv2rp188.h
	ntv2setup.h
	ntv2system.h
	ntv2video.h
	ntv2vpid.h
	ntv2xpt.h
	ntv2xptlookup.h)
set(AJADRIVER_COMMON_SOURCES
	ntv2anc.c
	ntv2audio.c
	ntv2autocirc.c
	ntv2commonreg.c
	ntv2displayid.c
	ntv2genlock.c
	ntv2hdmiedid.c
	ntv2hdmiin.c
	ntv2hdmiin4.c
	ntv2hdmiout4.c
	ntv2infoframe.c
	ntv2kona.c
	ntv2mcap.c
	ntv2pciconfig.c
	ntv2rp188.c
	ntv2setup.c
	ntv2system.c
	ntv2video.c
	ntv2vpid.c
	ntv2xpt.c)

# ajadriver target sources
set(AJADRIVER_TARGET_SOURCES
	${AJADRIVER_COMMON_HEADERS}
	${AJADRIVER_COMMON_SOURCES})

# ajadriver/linux
set(AJADRIVER_LINUX_HEADERS
	linux/driverdbg.h
	linux/fs1wait.h
	linux/ntv2dma.h
	linux/ntv2driverautocirculate.h
	linux/ntv2driverbigphysarea.h
	linux/ntv2driverdbgmsgctl.h
	linux/ntv2driver.h
	linux/ntv2driverstatus.h
	linux/ntv2drivertask.h
	linux/ntv2kona2.h
	linux/ntv2serial.h
	linux/registerio.h)
set(AJADRIVER_LINUX_SOURCES
	linux/ntv2dma.c
	linux/ntv2driverautocirculate.c
	linux/ntv2driver.c
	linux/ntv2driverdbgmsgctl.c
	linux/ntv2driverstatus.c
	linux/ntv2drivertask.c
	linux/ntv2kona2.c
	linux/ntv2serial.c
	linux/registerio.c)

if (NOT AJANTV2_BUILD_OPENSOURCE)
	# add HEVC sources
	set(AJADRIVER_LINUX_HEADERS
		${AJADRIVER_LINUX_HEADERS}
		linux/hevccommand.h
		linux/hevccommon.h
		linux/hevcconstants.h
		linux/hevcdriver.h
		linux/hevcinterrupt.h
		linux/hevcparams.h
		linux/hevcpublic.h
		linux/hevcregister.h
		linux/hevcstream.h)
	set(AJADRIVER_LINUX_SOURCES
		${AJADRIVER_LINUX_SOURCES}
		linux/hevcapi.c
		linux/hevccommand.c
		linux/hevcdriver.c
		linux/hevcinterrupt.c
		linux/hevcparams.c
		linux/hevcregister.c
		linux/hevcstream.c)
endif()

set(AJADRIVER_LINUX_BUILD_FILES
	linux/Makefile
	linux/nvidia-ko-to-module-symvers)
set(AJADRIVER_LINUX_LOAD_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/bin/load_ajantv2
	${CMAKE_CURRENT_SOURCE_DIR}/bin/unload_ajantv2)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(AJADRIVER_TARGET_SOURCES
		${AJADRIVER_TARGET_SOURCES}
		${AJADRIVER_LINUX_HEADERS}
		${AJADRIVER_LINUX_SOURCES})
endif()

if (AJANTV2_BUILD_DRIVER)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		aja_message(STATUS "Windows driver CMake build not yet implemented!")
		return()
	elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		aja_message(STATUS "macOS driver CMake build not yet implemented!")
		return()
	elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(BIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")
		add_custom_target(driver_linux ALL
						  COMMENT "Building Linux driver"
						  COMMAND make -C linux
						  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
						  SOURCES ${AJADRIVER_TARGET_SOURCES}
                          BYPRODUCTS ${BIN_PATH}/ajantv2.ko
                                     ${BIN_PATH}/load_ajantv2
                                     ${BIN_PATH}/unload_ajantv2)

		set(LIN_LOAD_DRIVER_MSG
			"         After building Linux driver can enable with: sudo .${BIN_PATH}/load_ajantv2")
		message(STATUS "AJA:  added target: driver_linux...\n${LIN_LOAD_DRIVER_MSG}")
	endif()

endif()

if (AJANTV2_BUILD_OPENSOURCE)
	# AJA does not yet provide sources for the Mac and Windows drivers under the MIT license.
	# Place this text as a README.md file in the ajadriver/mac and ajadriver/win directories as a placeholder.
	set(WIN_MAC_MIT_DRIVER_README 
		"# Windows & macOS NTV2 Device Driver\n"
		"The NTV2 device driver for Windows and macOS is not available under the MIT license at this time.\n"
		"Please download the AJA Software Installer for your AJA device from: https://www.aja.com/support\n\n"
		"# Linux NTV2 Device Driver\n"
		"To build the MIT-licensed NTV2 device driver on Linux:\n"
		"1. `cd driver/linux && make`\n"
		"2. `cd driver/bin`\n"
		"3. `sudo ./load_ajantv2`\n\n"
		"Alternately the Linux driver can be installed via the AJA Software Installer for Linux, available from: https://www.aja.com/support\n"
	)

    file(WRITE win/README.md ${WIN_MAC_MIT_DRIVER_README})
    file(WRITE mac/README.md ${WIN_MAC_MIT_DRIVER_README})
endif()

#[[
# installing

add_custom_target(ajadriver_install ${AJADRIVER_TARGET_SOURCES})
add_dependencies(ajadriver_install install)

# Windows
if (AJANTV2_BUILD_OPENSOURCE AND AJA_INSTALL_SOURCES)
	install(FILES win/README.md DESTINATION ${CMAKE_INSTALL_PREFIX}/driver/win)
endif()

# macOS
if (AJANTV2_BUILD_OPENSOURCE AND AJA_INSTALL_SOURCES)
	install(FILES mac/README.md DESTINATION ${CMAKE_INSTALL_PREFIX}/driver/mac)
endif()

if (AJA_INSTALL_HEADERS)
	if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		install(FILES ${AJADRIVER_LINUX_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/driver/linux)
	endif()
	install(FILES ${AJADRIVER_COMMON_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/driver)
	install(FILES ${AJA_LIB_NTV2_ROOT}/includes/ntv2driverprocamp.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ajalibraries/ajantv2/includes)
endif()
if (AJA_INSTALL_SOURCES)
	if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		install(FILES ${AJADRIVER_LINUX_SOURCES}              DESTINATION ${CMAKE_INSTALL_PREFIX}/ajadriver/linux)
	endif()
	install(FILES ${AJADRIVER_COMMON_SOURCES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ajadriver)
	install(FILES ${AJA_LIB_NTV2_ROOT}/src/ntv2driverprocamp.cpp DESTINATION ${CMAKE_INSTALL_PREFIX}/ajalibraries/ajantv2/src)
endif()
if (AJA_INSTALL_MISC)
	if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		install(FILES ${AJADRIVER_LINUX_BUILD_FILES}            DESTINATION ${CMAKE_INSTALL_PREFIX}/driver/linux)
		install(FILES ${AJADRIVER_LINUX_LOAD_FILES}             DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
		install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/configure.mk  DESTINATION ${CMAKE_INSTALL_PREFIX}/build)
		install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sdkversion.mk DESTINATION ${CMAKE_INSTALL_PREFIX}/build)
	endif()
endif()
#]]
