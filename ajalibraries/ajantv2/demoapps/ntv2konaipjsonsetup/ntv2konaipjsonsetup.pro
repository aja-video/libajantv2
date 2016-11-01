# -----------------------------------------------------------------------------
# Cross platform project used to generate platform specific makefiles/projects.
#
# To generate makefile/projects:
# Make sure the NTV2_SDK_DIR environment variable is set for Windows and Mac
# Make sure the AJA_API_DIR environment variable is set for Windows and Mac and Linux
# Make sure the AJA_3RDPARTY_DIR environment variable is set for Windows and Mac and Linux
# Make sure the AJA_NTV2_ROOT environment variable is set for Linux
#
#
# Windows:	use the VisualStudio addon, from Qt menu select
#			'Open Qt Project File (.pro)...' and select this file.
#
# Mac:		for xcode project: qmake -spec macx-xcode qtcontrolpanel.pro
#			for makefile: qmake -spec macx-g++ ntv2konaipjsonsetup.pro
#
# Linux:	qmake -unix ntv2konaipjsonsetup.pro
#
# Note: We use the scope unix:!macx for unixes that are not Mac OS X
# -----------------------------------------------------------------------------

# Common across platforms

TEMPLATE = app

TARGET = ntv2konaipjsonsetup

DEPENDPATH += $$PWD
UI_DIR	+= $$PWD/generatedfiles
RCC_DIR += $$PWD/generatedfiles
MOC_DIR += $$PWD/generatedfiles

QT += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}

win32{
	NTV2_DIR = $$PWD/../../
	DESTDIR = $$PWD/../../../bin
	AJA_API = $$PWD/../../../ajaapi
	CONFIG += debug_and_release
}
macx{
	NTV2_DIR = ../..
	DESTDIR = $$NTV2_DIR/../bin
	AJA_API = ../../../ajaapi
	CONFIG += debug
	CONFIG -= app_bundle
}
unix:!macx{
	NTV2_DIR = $$system(pwd)/../..
	DESTDIR = $$NTV2_DIR/../bin
	AJA_API = $$NTV2_DIR/../ajaapi
	DEBUG_TEST_FLAG = $$(AJA_DEBUG)
	DEPRECATE_FLAG = $$(NTV2_DEPRECATE)
}
AJA_3RDPARTY = $$(AJA_3RDPARTY_DIR)


INCLUDEPATH += $$AJA_API
INCLUDEPATH += $$NTV2_DIR/classes
INCLUDEPATH += $$NTV2_DIR/includes
INCLUDEPATH += $$NTV2_DIR/democlasses

# Windows specific
win32 {
    CONFIG += console

	DEFINES += MSWindows
	DEFINES += AJA_WINDOWS
	DEFINES += AJADLL
	DEFINES += AJA_WINDLL
	DEFINES += QT_LARGEFILE_SUPPORT
	DEFINES -= UNICODE


	#disable all-warnings-on flags, which is the default (i.e. -Wall -W)
	#QMAKE_CFLAGS_WARN_ON =
	#QMAKE_CXXFLAGS_WARN_ON =

	INCLUDEPATH += $$NTV2_DIR/winclasses

	LIBS += -lsetupapi
	LIBS += -lwinmm
	LIBS += -lImm32
	LIBS += -lshlwapi
	LIBS += -lurlmon
	LIBS += -lRpcrt4
	LIBS += -lWs2_32
}


# Mac specific
macx {
	# only build for Intel not PowerPC
	CONFIG += x86_64

	# Run on 10.8 or higher...
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8

	DEFINES += AJA_MAC
	DEFINES += AJAMac
		DEFINES += __STDC_CONSTANT_MACROS
	#	DEFINES += NTV2_DEPRECATE

	#disable all-warnings-on flags, which is the default (i.e. -Wall -W)
	QMAKE_CFLAGS_WARN_ON =
	QMAKE_CXXFLAGS_WARN_ON =

	#instead, turn on "most" warnings, plus some extra
	QMAKE_CFLAGS			+= -Wmost
	QMAKE_CFLAGS			+= -Wno-trigraphs
	QMAKE_CFLAGS			+= -Wno-four-char-constants
	QMAKE_CFLAGS			+= -Wno-unknown-pragmas
	QMAKE_CXXFLAGS			+= -Wmost
	QMAKE_CXXFLAGS			+= -Wno-trigraphs
	QMAKE_CXXFLAGS			+= -Wno-four-char-constants
	QMAKE_CXXFLAGS			+= -Wno-unknown-pragmas

	INCLUDEPATH += $$NTV2_DIR/macclasses

	LIBS += -framework ApplicationServices
	LIBS += -framework IOKit
	# LIBS += -lSMDK-Mac.3.3
	# LIBS += -lmp4decMT
}


# Unix specific
unix:!macx {
	DEFINES += AJA_LINUX
	DEFINES += AJALinux
	DEFINES += __STDC_CONSTANT_MACROS
	contains( DEPRECATE_FLAG, 1 ) {
		DEFINES += NTV2_DEPRECATE
		message("Building NTV2_DEPRECATE")
	}

	INCLUDEPATH += $$NTV2_DIR/linuxclasses

	QMAKE_LIBDIR += $$AJA_API/../lib
	QMAKE_CLEAN += $$DESTDIR/$$TARGET $$NTV2_DIR/../bin/$$TARGET
}

# stuff to turn on if CONFIG contains debug
build_pass:CONFIG(release, debug|release) {
	OBJECTS_DIR += release

	#release specific per platform settings
	win32{
		CFLAGS -= MT
		CFLAGS += MD
		QMAKE_LFLAGS_RELEASE += "/FORCE:MULTIPLE"
		QMAKE_LIBDIR += $$DESTDIR

		contains(QMAKE_TARGET.arch, x86_64) {
			LIBS += -lajastuffdll_64 -lclassesdll_64
		} else {
			LIBS += -lajastuffdll -lclassesdll
		}
	}

	macx {
		#QtMenu.files = $$(HOME)/qt_menu.nib
		#QtMenu.path = Contents/Resources
		#QMAKE_BUNDLE_DATA += QtMenu

		QMAKE_LIBDIR += ../../../lib
		LIBS += -lclasses
		LIBS += -lajastuff
	}

	unix:!macx {
		# Ignore build_pass an use environment to choose debug or release
		contains( DEBUG_TEST_FLAG, 1 ) {
			QMAKE_CFLAGS += -g
			QMAKE_CXXFLAGS += -g
			LIBS += -Wl,-Bstatic -laja -lajastuffd -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajastuff/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
			OBJECTS_DIR -= release
			OBJECTS_DIR += debug
		} else {
			LIBS += -Wl,-Bstatic -laja -lajastuff -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajastuff/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
		}
	}
}
else {

	DEFINES += AJA_DEBUG
	OBJECTS_DIR += debug


	#debug specific per platform settings
	win32{
		CFLAGS -= MTd
		CFLAGS += MDd
		QMAKE_LFLAGS_DEBUG += "/FORCE:MULTIPLE"
		QMAKE_LIBDIR += $$DESTDIR

		contains(QMAKE_TARGET.arch, x86_64) {
			LIBS += -lajastuffdll_64d -lclassesdll_64d
		} else {
			LIBS += -lajastuffdlld -lclassesdlld
		}
	}

	macx {
		#QtMenu.files = $$(HOME)/qt_menu.nib
		#QtMenu.path = Contents/Resources
		#QMAKE_BUNDLE_DATA += QtMenu

		QMAKE_LIBDIR += ../../../lib
		LIBS += -lclassesd
		LIBS += -lajastuffd
	}

	unix:!macx {
		# Ignore build_pass an use environment to choose debug or release
		contains( DEBUG_TEST_FLAG, 1 ) {
			QMAKE_CFLAGS += -g
			QMAKE_CXXFLAGS += -g
			LIBS += -Wl,-Bstatic -laja -lajastuffd -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajastuff/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
		} else {
			LIBS += -Wl,-Bstatic -laja -lajastuff -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajastuff/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
			OBJECTS_DIR -= debug
			OBJECTS_DIR += release
		}
	}
}

#local sources
SOURCES += $$PWD/main.cpp
SOURCES += $$PWD/konaipjsonsetup.cpp
HEADERS += $$PWD/konaipjsonsetup.h

