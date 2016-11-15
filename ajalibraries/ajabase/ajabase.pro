# ----------------------------------------------------------------
# This file is used to generate makefiles and projec file for NTV4
# ----------------------------------------------------------------

TEMPLATE = lib

CONFIG -= qt
QT = 

DEFINES -= UNICODE	
DEFINES -= QT_LARGEFILE_SUPPORT

INCLUDEPATH = ..
DEPENDPATH += .

win32 {
	CONFIG += staticlib
	CONFIG -= flat
	DESTDIR = ../lib
    Release:TARGET = ajabase
    Debug:TARGET = ajabased
	DEFINES += AJA_WINDOWS
	Debug:DEFINES += AJA_DEBUG
	QMAKE_CXXFLAGS -= -Zm200 -Zc:wchar_t-
	QMAKE_CXXFLAGS_DEBUG += -Od -RTC1 -W3 /MP /MTd
	QMAKE_CXXFLAGS_RELEASE += /MT
	QMAKE_CXXFLAGS_WARN_ON = 
	DEFINES += _MBCS
}

unix {
	CONFIG += staticlib
	DESTDIR = .../lib
    TARGET = ajabased
	DEFINES += AJA_DEBUG AJA_LINUX
	QMAKE_LFLAGS += -shared -Wl
	QMAKE_POST_LINK += cp ./*.so $$DESTDIR 
}

include(ajabase.pri)
