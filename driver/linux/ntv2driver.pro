TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
DEFINES += AJALinux
DEFINES += AJA_LINUX

INCLUDEPATH += ../../ajalibraries/ajantv2/includes/

SOURCES += \
    ./hevcapi.c \
    ./hevccommand.c \
    ./hevcdriver.c \
    ./hevcinterrupt.c \
    ./hevcparams.c \
    ./hevcregister.c \
    ./hevcstream.c \
    ./ntv2commonreg.c \
    ./ntv2displayid.c \
    ./ntv2dma.c \
    ./ntv2driver.c \
    ./ntv2driverautocirculate.c \
    ./ntv2driverdbgmsgctl.c \
    ./ntv2driverrp188.c \
    ./ntv2driverstatus.c \
    ./ntv2drivertask.c \
    ./ntv2genlock.c \
    ./ntv2hdmiedid.c \
    ./ntv2hdmiin.c \
    ./ntv2hdmiin4.c \
    ./ntv2hdmiout4.c \
    ./ntv2infoframe.c \
    ./ntv2kona2.c \
    ./ntv2serial.c \
    ./registerio.c \
    ../ntv2kona.c \
    ../ntv2system.c \
    ../ntv2vpid.c \
    ../ntv2xpt.c \
    ../../ajalibraries/ajantv2/src/ntv2vpidfromspec.cpp \
    ../../ajalibraries/ajantv2/src/ntv2devicefeatures.cpp \
	../../ajalibraries/ajantv2/src/ntv2devicefeatures.hpp \
    ../ntv2rp188.c \
    ../ntv2anc.c \
	../ntv2setup.c \
    ../ntv2anc.c \
    ../ntv2audio.c \
    ../ntv2autocirc.c \
    ../ntv2commonreg.c \
    ../ntv2displayid.c \
    ../ntv2genlock.c \
    ../ntv2hdmiedid.c \
    ../ntv2hdmiin.c \
    ../ntv2hdmiin4.c \
    ../ntv2hdmiout4.c \
    ../ntv2infoframe.c \
    ../ntv2kona.c \
    ../ntv2mcap.c \
    ../ntv2pciconfig.c \
    ../ntv2rp188.c \
    ../ntv2setup.c \
    ../ntv2system.c \
    ../ntv2video.c \
    ../ntv2vpid.c \
    ../ntv2xpt.c

DISTFILES += \
    ./TAGS

HEADERS += \
    ./driverdbg.h \
    ./fs1wait.h \
    ./hevccommand.h \
    ./hevccommon.h \
    ./hevcconstants.h \
    ./hevcdriver.h \
    ./hevcinterrupt.h \
    ./hevcparams.h \
    ./hevcpublic.h \
    ./hevcregister.h \
    ./hevcstream.h \
    ./ntv2commonreg.h \
    ./ntv2displayid.h \
    ./ntv2dma.h \
    ./ntv2driver.h \
    ./ntv2driverautocirculate.h \
    ./ntv2driverbigphysarea.h \
    ./ntv2driverdbgmsgctl.h \
    ./ntv2driverrp188.h \
    ./ntv2driverstatus.h \
    ./ntv2drivertask.h \
    ./ntv2genlock.h \
    ./ntv2genregs.h \
    ./ntv2hdmiedid.h \
    ./ntv2hdmiin.h \
    ./ntv2hdmiin4.h \
    ./ntv2hdmiout4.h \
    ./ntv2hin4reg.h \
    ./ntv2hinreg.h \
    ./ntv2hout4reg.h \
    ./ntv2infoframe.h \
    ./ntv2kona2.h \
    ./ntv2serial.h \
    ./ntv2system.h \
    ./registerio.h \
    ../ntv2kona.h \
    ../ntv2system.h \
    ../ntv2vpid.h \
    ../ntv2xpt.h \
    ../ntv2xptlookup.h \
    ../../ajalibraries/ajantv2/includes/ntv2vpidfromspec.h \
    ../../ajalibraries/ajantv2/includes/ntv2devicefeatures.h \
    ../../ajalibraries/ajantv2/includes/ntv2devicefeatures.hh \
    ../ntv2rp188.h \
    ../ntv2anc.h \
	../ntv2setup.h \
    ../ntv2anc.h \
    ../ntv2audio.h \
    ../ntv2autocirc.h \
    ../ntv2autofunc.h \
    ../ntv2commonreg.h \
    ../ntv2displayid.h \
    ../ntv2genlock.h \
    ../ntv2genregs.h \
    ../ntv2hdmiedid.h \
    ../ntv2hdmiin.h \
    ../ntv2hdmiin4.h \
    ../ntv2hdmiout4.h \
    ../ntv2hin4reg.h \
    ../ntv2hinreg.h \
    ../ntv2hout4reg.h \
    ../ntv2infoframe.h \
    ../ntv2kona.h \
    ../ntv2mcap.h \
    ../ntv2pciconfig.h \
    ../ntv2rp188.h \
    ../ntv2setup.h \
    ../ntv2system.h \
    ../ntv2video.h \
    ../ntv2vpid.h \
    ../ntv2xpt.h \
    ../ntv2xptlookup.h \
    ../../ajalibraries/ajantv2/includes/ntv2enums.h
