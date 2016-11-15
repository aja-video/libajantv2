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
	CONFIG += dll
	CONFIG -= flat
	DESTDIR = ../bin
    Release:TARGET = ajabasedll
    Debug:TARGET = ajabasedlld
    Release:OBJECTS_DIR=./ReleaseDLL
	Debug:OBJECTS_DIR = ./DebugDLL
	DEFINES += AJA_WINDOWS AJA_WINDLL AJA_DLL_BUILD
	Debug:DEFINES += AJA_DEBUG
	QMAKE_CXXFLAGS -= -Zm200 -Zc:wchar_t-
	QMAKE_CXXFLAGS_DEBUG += -Od -RTC1 -W3 /MP
	QMAKE_CXXFLAGS_WARN_ON = 
	DEFINES += _MBCS
}

unix {
	CONFIG += dll
	DESTDIR = ../lib
    TARGET = ajabased
	DEFINES += AJA_DEBUG AJA_LINUX
	QMAKE_LFLAGS += -shared -Wl
	QMAKE_POST_LINK += cp ./*.so $$DESTDIR 
}

include(ajabase.pri)
