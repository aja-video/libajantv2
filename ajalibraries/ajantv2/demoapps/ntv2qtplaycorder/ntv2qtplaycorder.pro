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
#			for makefile: qmake -spec macx-g++ ntv2qtplaycorder.pro
#
# Linux:	qmake -unix ntv2qtplaycorder.pro
#
# Note: We use the scope unix:!macx for unixes that are not Mac OS X
# -----------------------------------------------------------------------------

# Common across platforms

TEMPLATE = app

TARGET = ntv2qtplaycorder

DEPENDPATH += $$PWD
UI_DIR  += $$PWD/generatedfiles
RCC_DIR += $$PWD/generatedfiles
MOC_DIR += $$PWD/generatedfiles

# QtComponents needed (NOTE: core and gui are included by default)
QT += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

win32{
    NTV2_DIR = $$PWD/../../
    contains(QMAKE_TARGET.arch, x86_64) {
        DESTDIR = $$PWD/../../../bin
    } else {
        DESTDIR = $$PWD/../../../bin
    }
    AJA_API = $$PWD/../../../ajaapi
    CONFIG += debug_and_release
}
macx{
	NTV2_DIR = ../..
	AJA_API = ../../../ajaapi
	CONFIG += debug
}
unix:!macx{
	NTV2_DIR = $$system(pwd)/../..
	DESTDIR = $$NTV2_DIR/../bin
	AJA_API = $$NTV2_DIR/../ajaapi
	DEBUG_TEST_FLAG = $$(AJA_DEBUG)
}
AJA_3RDPARTY = $$(AJA_3RDPARTY_DIR)


INCLUDEPATH += $$AJA_API
INCLUDEPATH += $$NTV2_DIR/classes
INCLUDEPATH += $$NTV2_DIR/includes
INCLUDEPATH += $$NTV2_DIR/commonclasses
INCLUDEPATH += $$NTV2_DIR/democlasses

# Windows specific
win32 {
	DEFINES +=  MSWindows
	DEFINES +=  AJA_WINDOWS
	DEFINES +=  AJADLL
	DEFINES +=  AJA_STUFF_OBJ
	DEFINES +=  QT_LARGEFILE_SUPPORT
	DEFINES -= UNICODE


	#disable all-warnings-on flags, which is the default (i.e. -Wall -W)
	QMAKE_CFLAGS_WARN_ON	=
	QMAKE_CXXFLAGS_WARN_ON	=

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

	# Run on 10.6 or higher...
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6

	DEFINES += AJA_MAC
	DEFINES += AJAMac
	DEFINES += __STDC_CONSTANT_MACROS

	#disable all-warnings-on flags, which is the default (i.e. -Wall -W)
	QMAKE_CFLAGS_WARN_ON	=
	QMAKE_CXXFLAGS_WARN_ON	=

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

	# Set the custom plist
	QMAKE_INFO_PLIST = Info.plist

	ICON = app.icns
}


# Unix specific
unix:!macx {
	DEFINES += AJA_LINUX
	DEFINES += AJALinux
	DEFINES += __STDC_CONSTANT_MACROS

	INCLUDEPATH += $$NTV2_DIR/linuxclasses

	QMAKE_LIBDIR += $$AJA_API/../lib
	QMAKE_CLEAN += $$DESTDIR/$$TARGET $$NTV2_DIR/bin/$$TARGET
}

# stuff to turn on if CONFIG contains debug
build_pass:CONFIG(release, debug|release) {
	DESTDIR = ../../bin
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
		QtMenu.path = Contents/Resources
		QMAKE_BUNDLE_DATA += QtMenu

		QMAKE_LIBDIR += ../../../bin
		LIBS += -lclasses
		LIBS += -lajastuff
	}

	unix:!macx {
		# Ignore build_pass an use environment to choose debug or release
		contains( DEBUG_TEST_FLAG, 1 ) {
			QMAKE_CFLAGS += -g
			QMAKE_CXXFLAGS += -g
			LIBS += -Wl,-Bstatic -lajastuffd -laja -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajastuff/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
			OBJECTS_DIR -= release
			OBJECTS_DIR += debug
		} else {
			LIBS += -Wl,-Bstatic -lajastuff -laja -Wl,-Bdynamic
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
		QtMenu.path = Contents/Resources
		QMAKE_BUNDLE_DATA += QtMenu

		QMAKE_LIBDIR += ../../../bin
		LIBS += -lclassesd
		LIBS += -lajastuffd
	}

	unix:!macx {
		# Ignore build_pass an use environment to choose debug or release
		contains( DEBUG_TEST_FLAG, 1 ) {
			QMAKE_CFLAGS += -g
			QMAKE_CXXFLAGS += -g
			LIBS += -Wl,-Bstatic -lajastuffd -laja -Wl,-Bdynamic
			makestufflib.commands = make -C $$AJA_API/ajastuff/build
			QMAKE_EXTRA_TARGETS += makestufflib
			PRE_TARGETDEPS += makestufflib
			makeajalib.commands = make -C $$NTV2_DIR/classes
			QMAKE_EXTRA_TARGETS += makeajalib
			PRE_TARGETDEPS += makeajalib
		} else {
			QMAKE_LIBDIR += $$NTV2_DIR/lib
			LIBS += -Wl,-Bstatic -lajastuff -laja -Wl,-Bdynamic
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
SOURCES += $$PWD/ntv2qtplaycorder.cpp
HEADERS += $$PWD/ntv2qtplaycorder.h

HEADERS += $$NTV2_DIR/commonclasses/ntv2videoformataja.h
SOURCES += $$NTV2_DIR/commonclasses/ntv2videoformataja.cpp

HEADERS += $$NTV2_DIR/democlasses/ajapreviewwidget.h
SOURCES += $$NTV2_DIR/democlasses/ajapreviewwidget.cpp
HEADERS += $$NTV2_DIR/democlasses/iplaycorder.h
HEADERS += $$NTV2_DIR/democlasses/ntv2capture.h
SOURCES += $$NTV2_DIR/democlasses/ntv2capture.cpp
HEADERS += $$NTV2_DIR/democlasses/ntv2playcorder.h
SOURCES += $$NTV2_DIR/democlasses/ntv2playcorder.cpp
HEADERS += $$NTV2_DIR/democlasses/ntv2player.h
SOURCES += $$NTV2_DIR/democlasses/ntv2player.cpp

HEADERS += $$AJA_API/ajastuff/common/dpx_hdr.h
SOURCES += $$AJA_API/ajastuff/common/dpx_hdr.cpp
HEADERS += $$AJA_API/ajastuff/common/wavewriter.h
SOURCES += $$AJA_API/ajastuff/common/wavewriter.cpp


#resources
RESOURCES += ntv2qtplaycorder.qrc

