if (AJANTV2_DISABLE_DRIVER)
    message(STATUS "Skipping libajantv2 driver...")
    return()
endif()

set(AJADRIVER_COMMON_HEADERS
	ntv2anc.h
	ntv2audio.h
	ntv2autocirc.h
	ntv2autofunc.h
	ntv2aux.h
	ntv2commonreg.h
	ntv2displayid.h
	ntv2genlock.h
	ntv2genregs.h
	ntv2genlock2.h
	ntv2gen2regs.h
	ntv2gen2regs_8a34045.h
	ntv2gen2regs_rc32012a.h
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
	ntv2stream.h
	ntv2system.h
	ntv2video.h
	ntv2videoraster.h
	ntv2videorasterreg.h
	ntv2vpid.h
	ntv2xpt.h
	ntv2xptlookup.h)
set(AJADRIVER_COMMON_SOURCES
	ntv2anc.c
	ntv2audio.c
	ntv2autocirc.c
	ntv2aux.c
	ntv2commonreg.c
	ntv2displayid.c
	ntv2genlock.c
	ntv2genlock2.c
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
	ntv2stream.c
	ntv2video.c
	ntv2videoraster.c
	ntv2vpid.c
	ntv2xpt.c)

# ajadriver target sources
set(AJADRIVER_TARGET_SOURCES
	${AJADRIVER_COMMON_HEADERS}
	${AJADRIVER_COMMON_SOURCES})

# ajadriver/linux
set(AJADRIVER_LINUX_HEADERS
	linux/buildenv.h
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
	linux/dkms.conf
	linux/Makefile
	linux/nvidia-ko-to-module-symvers)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(AJADRIVER_TARGET_SOURCES
		${AJADRIVER_TARGET_SOURCES}
		${AJADRIVER_LINUX_HEADERS}
		${AJADRIVER_LINUX_SOURCES})
endif()

set(LIBAJANTV2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../)
set(AJANTV2_ROOT ${LIBAJANTV2_ROOT}/ajantv2)
set(NEEDED_NTV2_INCLUDES
    ${AJANTV2_ROOT}/includes/ajaexport.h
    ${AJANTV2_ROOT}/includes/ajatypes.h
    ${AJANTV2_ROOT}/includes/ntv2audiodefines.h
    ${AJANTV2_ROOT}/includes/ntv2devicefeatures.h
    ${AJANTV2_ROOT}/includes/ntv2devicefeatures.hh
    ${AJANTV2_ROOT}/includes/ntv2driverprocamp.h
    ${AJANTV2_ROOT}/includes/ntv2enums.h
    ${AJANTV2_ROOT}/includes/ntv2fixed.h
    ${AJANTV2_ROOT}/includes/ntv2publicinterface.h
    ${AJANTV2_ROOT}/includes/ntv2version.h
    ${AJANTV2_ROOT}/includes/ntv2videodefines.h
    ${AJANTV2_ROOT}/includes/ntv2virtualregisters.h
    ${AJANTV2_ROOT}/includes/ntv2vpidfromspec.h
    )
set(NEEDED_NTV2_LIN_INCLUDES
    ${AJANTV2_ROOT}/src/lin/ntv2linuxpublicinterface.h
    )
set(NEEDED_NTV2_SOURCES
    ${AJANTV2_ROOT}/src/ntv2devicefeatures.cpp
    ${AJANTV2_ROOT}/src/ntv2driverprocamp.cpp
    ${AJANTV2_ROOT}/src/ntv2vpidfromspec.cpp
    ${AJANTV2_ROOT}/src/ntv2devicefeatures.hpp
    )

if (NOT AJANTV2_DISABLE_DRIVER)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		message(STATUS "Windows driver CMake build not yet implemented!")
		return()
	elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		message(STATUS "macOS driver CMake build not yet implemented!")
		return()
	elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        execute_process(COMMAND uname -r
            OUTPUT_VARIABLE KERNEL_RELEASE
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        set(KERNEL_BUILD_DIR /lib/modules/${KERNEL_RELEASE}/build)

        if(EXISTS ${KERNEL_BUILD_DIR})
            set(BIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")
                add_custom_target(driver_linux ALL
                    COMMENT "Building Linux driver"
                    COMMAND make -C linux
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    SOURCES ${AJADRIVER_TARGET_SOURCES}
                    BYPRODUCTS ${BIN_PATH}/ajantv2.ko
                    )

                add_custom_target(driver_linux_dkms_install
                    COMMENT "Installing Linux DKMS driver"
                    COMMAND make -C linux dkms-install
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    )

                add_custom_target(driver_linux_dkms_uninstall
                    COMMENT "Uninstalling Linux DKMS driver"
                    COMMAND make -C linux dkms-uninstall
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    )

            set(LIN_LOAD_DRIVER_MSG
                "         After building Linux driver can enable with: sudo .${BIN_PATH}/load_ajantv2")
            message(STATUS "AJA:  added target: driver_linux...\n${LIN_LOAD_DRIVER_MSG}")
        else()
            message(WARNING "AJA: Unable to add driver target: Please install kernel headers: ${KERNEL_BUILD_DIR}")
        endif()
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
		"3. `sudo insmod ajantv2.ko`\n\n"
		"Alternately the Linux driver can be installed via the AJA Software Installer for Linux, available from: https://www.aja.com/support\n"
	)

    file(WRITE win/README.md ${WIN_MAC_MIT_DRIVER_README})
    file(WRITE mac/README.md ${WIN_MAC_MIT_DRIVER_README})
endif()

# installing
add_custom_target(ajadriver_install ${AJADRIVER_TARGET_SOURCES})
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_dependencies(ajadriver_install install)
endif()

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
		install(FILES ${AJADRIVER_LINUX_HEADERS}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver/linux
                COMPONENT install_driver)
	endif()
	install(FILES ${AJADRIVER_COMMON_HEADERS}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver
            COMPONENT install_driver)

    install(FILES ${NEEDED_NTV2_INCLUDES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/ajantv2/includes
            COMPONENT install_driver)

    install(FILES ${NEEDED_NTV2_LIN_INCLUDES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/ajantv2/src/lin
            COMPONENT install_driver)
endif()
if (AJA_INSTALL_SOURCES)
	if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		install(FILES ${AJADRIVER_LINUX_SOURCES}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver/linux
                COMPONENT install_driver)
	endif()
	install(FILES ${AJADRIVER_COMMON_SOURCES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver
            COMPONENT install_driver)

    install(FILES ${NEEDED_NTV2_SOURCES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/ajantv2/src
            COMPONENT install_driver)
endif()
if (AJA_INSTALL_CMAKE)
    install(FILES CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver)
endif()
if (AJA_INSTALL_MISC)
	if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		install(FILES ${AJADRIVER_LINUX_BUILD_FILES}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver/linux
                COMPONENT install_driver)
		install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/configure.mk
                DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver
                COMPONENT install_driver)
		install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sdkversion.mk
                DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver
                COMPONENT install_driver)
        install(FILES ${LIBAJANTV2_ROOT}/.gitignore
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2
            COMPONENT install_driver)
        install(FILES ${LIBAJANTV2_ROOT}/driver/linux/.gitignore
            DESTINATION ${CMAKE_INSTALL_PREFIX}/libajantv2/driver/linux
            COMPONENT install_driver)
	endif()
endif()
